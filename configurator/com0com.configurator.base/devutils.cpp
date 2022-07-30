/*
 * $Id: devutils.cpp,v 1.21 2012/01/10 11:24:27 vfrolov Exp $
 *
 * Copyright (c) 2006-2012 Vyacheslav Frolov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * $Log: devutils.cpp,v $
 * Revision 1.21  2012/01/10 11:24:27  vfrolov
 * Added ability to repeate waiting for no pending device
 * installation activities
 *
 * Revision 1.20  2011/12/15 15:51:48  vfrolov
 * Fixed types
 *
 * Revision 1.19  2011/07/15 16:09:05  vfrolov
 * Disabled MessageBox() for silent mode and added default processing
 *
 * Revision 1.18  2011/07/13 17:39:55  vfrolov
 * Fixed result treatment of UpdateDriverForPlugAndPlayDevices()
 *
 * Revision 1.17  2010/07/29 12:18:43  vfrolov
 * Fixed waiting stuff
 *
 * Revision 1.16  2010/07/15 18:11:09  vfrolov
 * Fixed --wait option for Ports class
 *
 * Revision 1.15  2010/07/12 18:14:44  vfrolov
 * Fixed driver update duplication
 *
 * Revision 1.14  2010/06/07 07:03:31  vfrolov
 * Added wrapper UpdateDriver() for UpdateDriverForPlugAndPlayDevices()
 *
 * Revision 1.13  2010/05/27 11:16:46  vfrolov
 * Added ability to put the port to the Ports class
 *
 * Revision 1.12  2009/09/21 08:54:05  vfrolov
 * Added DI_NEEDRESTART check
 *
 * Revision 1.11  2009/02/16 10:36:16  vfrolov
 * Done --silent option more silent
 *
 * Revision 1.10  2009/02/11 07:35:21  vfrolov
 * Added --no-update option
 *
 * Revision 1.9  2007/11/27 16:35:49  vfrolov
 * Added state check before enabling
 *
 * Revision 1.8  2007/10/01 15:01:35  vfrolov
 * Added pDevInstID parameter to InstallDevice()
 *
 * Revision 1.7  2007/09/25 12:42:49  vfrolov
 * Fixed update command (bug if multiple pairs active)
 * Fixed uninstall command (restore active ports on cancell)
 *
 * Revision 1.6  2007/09/14 12:58:44  vfrolov
 * Removed INSTALLFLAG_FORCE
 * Added UpdateDriverForPlugAndPlayDevices() retrying
 *
 * Revision 1.5  2007/02/15 08:48:45  vfrolov
 * Fixed 1658441 - Installation Failed
 * Thanks to Michael A. Smith
 *
 * Revision 1.4  2006/11/10 14:07:40  vfrolov
 * Implemented remove command
 *
 * Revision 1.3  2006/11/02 16:20:44  vfrolov
 * Added usage the fixed port numbers
 *
 * Revision 1.2  2006/10/13 10:26:35  vfrolov
 * Some defines moved to ../include/com0com.h
 * Changed name of device object (for WMI)
 *
 * Revision 1.1  2006/07/28 12:16:42  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"
#include "msg.h"
#include "devutils.h"
#include "utils.h"

///////////////////////////////////////////////////////////////
struct EnumParams {
  EnumParams() {
    pDevProperties = NULL;
    pDevCallBack = NULL;
    pDevCallBackParam = NULL;
    pStack = NULL;
    count = 0;
    pRebootRequired = NULL;
  }

  PCDevProperties pDevProperties;
  PCNC_DEV_CALLBACK pDevCallBack;
  void *pDevCallBackParam;
  Stack *pStack;
  int count;
  BOOL *pRebootRequired;
};

typedef EnumParams *PEnumParams;

struct DevParams {
  DevParams(PEnumParams _pEnumParams) {
    pEnumParams = _pEnumParams;
  }

  PEnumParams pEnumParams;
  DevProperties devProperties;
};

typedef DevParams *PDevParams;
///////////////////////////////////////////////////////////////
static const char *SetStr(char **ppDst, const char *pSrc)
{
  if (*ppDst) {
    LocalFree(*ppDst);
    *ppDst = NULL;
  }

  if (pSrc) {
    int len = lstrlen(pSrc) + 1;

    *ppDst = (char *)LocalAlloc(LPTR, len * sizeof(pSrc[0]));

    if (*ppDst) {
      SNPRINTF(*ppDst, len, "%s", pSrc);
    } else {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      ShowLastError(MB_OK|MB_ICONSTOP, "LocalAlloc(%lu)", (unsigned long)(len * sizeof(pSrc[0])));
    }
  }

  return *ppDst;
}
///////////////////////////////////////////////////////////////
const char *DevProperties::DevId(const char *_pDevId)
{
  return SetStr(&pDevId, _pDevId);
}

const char *DevProperties::PhObjName(const char *_pPhObjName)
{
  return SetStr(&pPhObjName, _pPhObjName);
}

const char *DevProperties::Location(const char *_pLocation)
{
  return SetStr(&pLocation, _pLocation);
}
///////////////////////////////////////////////////////////////
typedef int CNC_DEV_ROUTINE(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PDevParams pDevParams);

typedef CNC_DEV_ROUTINE *PCNC_DEV_ROUTINE;

static int EnumDevices(
    PCNC_ENUM_FILTER pFilter,
    DWORD flags,
    PCNC_DEV_ROUTINE pDevRoutine,
    PEnumParams pEnumParams)
{
  HDEVINFO hDevInfo;

  hDevInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES|flags);

  if (hDevInfo == INVALID_HANDLE_VALUE) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiGetClassDevs()");
    return IDCANCEL;
  }

  SP_DEVINFO_DATA devInfoData;

  devInfoData.cbSize = sizeof(devInfoData);

  int res = IDCONTINUE;

  for (int i = 0 ; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData) ; i++) {
    char hardwareId[40];

    if (!SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)hardwareId, sizeof(hardwareId), NULL)) {
      memset(hardwareId, 0, sizeof(hardwareId));
      SNPRINTF(hardwareId, sizeof(hardwareId)/sizeof(hardwareId[0]), "UNKNOWN HARDWAREID" "\0");
    }

    if (!pFilter(hardwareId))
      continue;

    const char *pHardwareId = hardwareId;

    if (pEnumParams->pDevProperties && pEnumParams->pDevProperties->DevId()) {
      while (lstrcmpi(pHardwareId, pEnumParams->pDevProperties->DevId()) != 0) {
        pHardwareId = pHardwareId + lstrlen(pHardwareId) + 1;

        if (!*pHardwareId) {
          pHardwareId = NULL;
          break;
        }
      }

      if (!pHardwareId)
        continue;
    }

    DevParams devParams(pEnumParams);

    if (!devParams.devProperties.DevId(pHardwareId)) {
      res = IDCANCEL;
      break;
    }

    char location[40];

    if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_LOCATION_INFORMATION, NULL, (PBYTE)location, sizeof(location), NULL)) {
      if (!devParams.devProperties.Location(location)) {
        res = IDCANCEL;
        break;
      }
    }

    if (pEnumParams->pDevProperties &&
        pEnumParams->pDevProperties->Location() && (!devParams.devProperties.Location() ||
        lstrcmpi(devParams.devProperties.Location(), pEnumParams->pDevProperties->Location())))
    {
      continue;
    }

    char name[40];

    if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, NULL, (PBYTE)name, sizeof(name), NULL)) {
      if (!devParams.devProperties.PhObjName(name)) {
        res = IDCANCEL;
        break;
      }
    }

    if (pEnumParams->pDevProperties &&
        pEnumParams->pDevProperties->PhObjName() && (!devParams.devProperties.PhObjName() ||
        lstrcmpi(devParams.devProperties.PhObjName(), pEnumParams->pDevProperties->PhObjName())))
    {
      continue;
    }

    res = pDevRoutine(hDevInfo, &devInfoData, &devParams);

    if (res != IDCONTINUE)
      break;
  }

  DWORD err = GetLastError();

  SetupDiDestroyDeviceInfoList(hDevInfo);

  SetLastError(err);

  return res;
}
///////////////////////////////////////////////////////////////
static bool UpdateRebootRequired(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, BOOL *pRebootRequired)
{
  if (!pRebootRequired)
    return TRUE;

  if (*pRebootRequired)
    return TRUE;

  ULONG status = 0;
  ULONG problem = 0;

  *pRebootRequired =
      CM_Get_DevNode_Status(&status, &problem, pDevInfoData->DevInst, 0) == CR_SUCCESS &&
      (status & DN_NEED_RESTART) != 0;

  //if (*pRebootRequired)
  //  Trace("Enumerated status=0x%lX problem=0x%lX\n", status, problem);

  if (*pRebootRequired)
    return TRUE;

  SP_DEVINSTALL_PARAMS installParams;

  memset(&installParams, 0, sizeof(installParams));
  installParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

  if (!SetupDiGetDeviceInstallParams(hDevInfo, pDevInfoData, &installParams)) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiGetDeviceInstallParams()");
    return FALSE;
  }

  *pRebootRequired = (installParams.Flags & (DI_NEEDREBOOT|DI_NEEDRESTART)) ? TRUE : FALSE;

  //if (*pRebootRequired)
  //  Trace("Enumerated Flags=0x%lX\n", installParams.Flags);

  return TRUE;
}
///////////////////////////////////////////////////////////////
static bool ChangeState(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, DWORD stateChange)
{
  SP_PROPCHANGE_PARAMS propChangeParams;

  memset(&propChangeParams, 0, sizeof(propChangeParams));
  propChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
  propChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
  propChangeParams.StateChange = stateChange;
  propChangeParams.Scope = DICS_FLAG_CONFIGSPECIFIC;
  propChangeParams.HwProfile = 0;

  if (!SetupDiSetClassInstallParams(hDevInfo, pDevInfoData, (SP_CLASSINSTALL_HEADER *)&propChangeParams, sizeof(propChangeParams))) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiSetClassInstallParams()");
    return FALSE;
  }

  if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hDevInfo, pDevInfoData)) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiCallClassInstaller(DIF_PROPERTYCHANGE)");
    return FALSE;
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
static bool IsDisabled(PSP_DEVINFO_DATA pDevInfoData)
{
  ULONG status = 0;
  ULONG problem = 0;

  if (CM_Get_DevNode_Status(&status, &problem, pDevInfoData->DevInst, 0) != CR_SUCCESS)
    return FALSE;

  return (status & DN_HAS_PROBLEM) != 0 && problem == CM_PROB_DISABLED;
}
///////////////////////////////////////////////////////////////
static bool IsEnabled(PSP_DEVINFO_DATA pDevInfoData)
{
  ULONG status = 0;
  ULONG problem = 0;

  if (CM_Get_DevNode_Status(&status, &problem, pDevInfoData->DevInst, 0) != CR_SUCCESS)
    return FALSE;

  return (status & (DN_HAS_PROBLEM|DN_NEED_RESTART)) == 0;
}
///////////////////////////////////////////////////////////////
static CNC_DEV_ROUTINE EnumDevice;
static int EnumDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, PDevParams pDevParams)
{
  /*
  Trace("Enumerated %s %s %s\n",
        pDevParams->devProperties.Location(),
        pDevParams->devProperties.DevId(),
        pDevParams->devProperties.PhObjName());
  */

  int res = IDCONTINUE;

  if (pDevParams->pEnumParams->pDevCallBack) {
    if (!pDevParams->pEnumParams->pDevCallBack(         hDevInfo,
                                                        pDevInfoData,
                                                        &pDevParams->devProperties,
                                                        pDevParams->pEnumParams->pRebootRequired,
                                                        pDevParams->pEnumParams->pDevCallBackParam))
    {
      res = IDCANCEL;
    }
  }

  pDevParams->pEnumParams->count++;

  return res;
}

int EnumDevices(
    PCNC_ENUM_FILTER pFilter,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired,
    PCNC_DEV_CALLBACK pDevCallBack,
    void *pDevCallBackParam)
{
  EnumParams enumParams;

  enumParams.pDevProperties = pDevProperties;
  enumParams.pRebootRequired = pRebootRequired;
  enumParams.pDevCallBack = pDevCallBack;
  enumParams.pDevCallBackParam = pDevCallBackParam;

  if (EnumDevices(pFilter, DIGCF_PRESENT, EnumDevice, &enumParams) != IDCONTINUE)
    return -1;

  return enumParams.count;
}
///////////////////////////////////////////////////////////////
int DisableDevice(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired,
    Stack *pDevPropertiesStack)
{
  if (IsDisabled(pDevInfoData))
    return IDCONTINUE;

  BOOL rebootRequired = FALSE;

  if (!UpdateRebootRequired(hDevInfo, pDevInfoData, &rebootRequired))
    return IDCANCEL;

  //if (rebootRequired && pRebootRequired)
  //  *pRebootRequired = TRUE;

  if (!ChangeState(hDevInfo, pDevInfoData, DICS_DISABLE))
    return IDCANCEL;

  Trace("Disabled %s %s %s\n",
        pDevProperties->Location(),
        pDevProperties->DevId(),
        pDevProperties->PhObjName());

  if (!rebootRequired) {
    if (!UpdateRebootRequired(hDevInfo, pDevInfoData, &rebootRequired))
      return IDCANCEL;

    if (rebootRequired) {
      Trace("Can't stop device %s %s %s\n",
            pDevProperties->Location(),
            pDevProperties->DevId(),
            pDevProperties->PhObjName());

      int res;

      res = ShowMsg(MB_CANCELTRYCONTINUE,
                        "Can't stop device %s %s %s.\n"
                        "Close application that use this device and Try Again.\n"
                        "Or Continue and then reboot system.\n",
                        pDevProperties->Location(),
                        pDevProperties->DevId(),
                        pDevProperties->PhObjName());

      if (res == 0)
        res = IDCONTINUE;

      if (res != IDCONTINUE) {
        if (!ChangeState(hDevInfo, pDevInfoData, DICS_ENABLE))
          return IDCANCEL;

        Trace("Enabled %s %s %s\n",
              pDevProperties->Location(),
              pDevProperties->DevId(),
              pDevProperties->PhObjName());

        return res;
      }

      if (pRebootRequired)
        *pRebootRequired = TRUE;
    }
  }

  if (pDevPropertiesStack) {
    DevProperties *pDevProp = new DevProperties(*pDevProperties);

    if (pDevProp) {
      StackEl *pElem = new StackEl(pDevProp);

      if (pElem)
        pDevPropertiesStack->Push(pElem);
      else
        delete pDevProp;
    }
  }

  return IDCONTINUE;
}
///////////////////////////////////////////////////////////////
static CNC_DEV_ROUTINE DisableDevice;
static int DisableDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, PDevParams pDevParams)
{
  int res = DisableDevice(hDevInfo,
                          pDevInfoData,
                          &pDevParams->devProperties,
                          pDevParams->pEnumParams->pRebootRequired,
                          pDevParams->pEnumParams->pStack);

  if (res == IDCONTINUE)
    pDevParams->pEnumParams->count++;

  return res;
}

bool DisableDevices(
    PCNC_ENUM_FILTER pFilter,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired,
    Stack *pDevPropertiesStack)
{
  EnumParams enumParams;

  int res;

  enumParams.pRebootRequired = pRebootRequired;
  enumParams.pDevProperties = pDevProperties;
  enumParams.pStack = pDevPropertiesStack;

  do {
    res = EnumDevices(pFilter, DIGCF_PRESENT, DisableDevice, &enumParams);
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  if (!WaitNoPendingInstallEvents(10))
    Sleep(1000);

  return TRUE;
}
///////////////////////////////////////////////////////////////
static CNC_DEV_ROUTINE EnableDevice;
static int EnableDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, PDevParams pDevParams)
{
  if (IsEnabled(pDevInfoData))
    return IDCONTINUE;

  if (ChangeState(hDevInfo, pDevInfoData, DICS_ENABLE)) {
    Trace("Enabled %s %s %s\n",
          pDevParams->devProperties.Location(),
          pDevParams->devProperties.DevId(),
          pDevParams->devProperties.PhObjName());
  }

  UpdateRebootRequired(hDevInfo, pDevInfoData, pDevParams->pEnumParams->pRebootRequired);

  return IDCONTINUE;
}

bool EnableDevices(
    PCNC_ENUM_FILTER pFilter,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired)
{
  EnumParams enumParams;

  int res;

  enumParams.pRebootRequired = pRebootRequired;
  enumParams.pDevProperties = pDevProperties;

  do {
    res = EnumDevices(pFilter, DIGCF_PRESENT, EnableDevice, &enumParams);
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
static CNC_DEV_ROUTINE RestartDevice;
static int RestartDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, PDevParams pDevParams)
{
  if (!ChangeState(hDevInfo, pDevInfoData, DICS_PROPCHANGE))
    return IDCANCEL;

  BOOL rebootRequired = FALSE;

  if (!UpdateRebootRequired(hDevInfo, pDevInfoData, &rebootRequired))
    return IDCANCEL;

  if (rebootRequired) {
    Trace("Can't reastart device %s %s %s\n",
          pDevParams->devProperties.Location(),
          pDevParams->devProperties.DevId(),
          pDevParams->devProperties.PhObjName());

    if (pDevParams->pEnumParams->pRebootRequired && !*pDevParams->pEnumParams->pRebootRequired) {
      int res;

      res = ShowMsg(MB_CANCELTRYCONTINUE,
                        "Can't reastart device %s %s %s.\n"
                        "Close application that use this device and Try Again.\n"
                        "Or Continue and then reboot system.\n",
                        pDevParams->devProperties.Location(),
                        pDevParams->devProperties.DevId(),
                        pDevParams->devProperties.PhObjName());

      if (res == 0)
        res = IDCONTINUE;

      if (res != IDCONTINUE) {
        if (!ChangeState(hDevInfo, pDevInfoData, DICS_ENABLE))
          return IDCANCEL;

        return res;
      }

      *pDevParams->pEnumParams->pRebootRequired = TRUE;
    }
  } else {
    Trace("Restarted %s %s %s\n",
        pDevParams->devProperties.Location(),
        pDevParams->devProperties.DevId(),
        pDevParams->devProperties.PhObjName());

    pDevParams->pEnumParams->count++;
  }

  return IDCONTINUE;
}

bool RestartDevices(
    PCNC_ENUM_FILTER pFilter,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired)
{
  EnumParams enumParams;

  int res;

  enumParams.pRebootRequired = pRebootRequired;
  enumParams.pDevProperties = pDevProperties;

  do {
    res = EnumDevices(pFilter, DIGCF_PRESENT, RestartDevice, &enumParams);
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
bool RemoveDevice(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired)
{
  if (!SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, pDevInfoData)) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiCallClassInstaller(DIF_REMOVE, %s, %s)",
                  pDevProperties->DevId(), pDevProperties->PhObjName());
    return FALSE;
  }

  Trace("Removed %s %s %s\n", pDevProperties->Location(), pDevProperties->DevId(), pDevProperties->PhObjName());

  return UpdateRebootRequired(hDevInfo, pDevInfoData, pRebootRequired);
}
///////////////////////////////////////////////////////////////
static CNC_DEV_ROUTINE RemoveDevice;
static int RemoveDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, PDevParams pDevParams)
{
  if (!RemoveDevice(hDevInfo, pDevInfoData, &pDevParams->devProperties, pDevParams->pEnumParams->pRebootRequired))
    return IDCANCEL;

  pDevParams->pEnumParams->count++;

  return IDCONTINUE;
}

bool RemoveDevices(
    PCNC_ENUM_FILTER pFilter,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired)
{
  EnumParams enumParams;

  int res;

  enumParams.pRebootRequired = pRebootRequired;
  enumParams.pDevProperties = pDevProperties;

  do {
    res = EnumDevices(pFilter, 0, RemoveDevice, &enumParams);
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  if (!enumParams.count)
    Trace("No devices found\n");

  return TRUE;
}
///////////////////////////////////////////////////////////////
bool ReenumerateDeviceNode(PSP_DEVINFO_DATA pDevInfoData)
{
  return CM_Reenumerate_DevNode(pDevInfoData->DevInst, 0) == CR_SUCCESS;
}
///////////////////////////////////////////////////////////////
int UpdateDriver(
    const char *pInfFilePath,
    const char *pHardwareId,
    DWORD flags,
    bool mandatory,
    BOOL *pRebootRequired)
{
  DWORD updateErr = ERROR_SUCCESS;

  for (int i = 0 ; i < 10 ; i++) {
    if (UpdateDriverForPlugAndPlayDevices(0, pHardwareId, pInfFilePath, flags, pRebootRequired))
    {
      updateErr = ERROR_SUCCESS;
    } else {
      updateErr = GetLastError();

      if (updateErr == ERROR_SHARING_VIOLATION) {
        Trace(".");
        Sleep(1000);
        continue;
      }
      else
      if (!mandatory) {
        if (updateErr == ERROR_NO_SUCH_DEVINST) {
          updateErr = ERROR_SUCCESS;
        }
        else
        if (updateErr == ERROR_NO_MORE_ITEMS && (flags & INSTALLFLAG_FORCE) == 0) {
          updateErr = ERROR_SUCCESS;
        }
      }
    }

    if (i)
      Trace("\n");

    break;
  }

  if (updateErr != ERROR_SUCCESS) {
    if (updateErr == ERROR_FILE_NOT_FOUND) {
      LONG err;
      HKEY hKey;

      err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, 0, KEY_READ, &hKey);

      if (err == ERROR_SUCCESS)
        RegCloseKey(hKey);

      if (err == ERROR_FILE_NOT_FOUND) {
        int res = ShowMsg(MB_CANCELTRYCONTINUE,
                          "Can't update driver. Possible it's because your Windows registry is corrupted and\n"
                          "there is not the following key:\n"
                          "\n"
                          "HKEY_LOCAL_MACHINE\\" REGSTR_PATH_RUNONCE "\n"
                          "\n"
                          "Continue to add the key to the registry.\n");

        if (res == 0)
          return IDCANCEL;

        if (res == IDCONTINUE) {
          err = RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);

          if (err == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return IDTRYAGAIN;
          } else {
            ShowLastError(MB_OK|MB_ICONSTOP, "RegCreateKeyEx()");
            return IDCANCEL;
          }
        }

        return res;
      }
    }

    return ShowError(MB_CANCELTRYCONTINUE, updateErr,
                     "UpdateDriverForPlugAndPlayDevices(\"%s\", \"%s\", 0x%lX)",
                     pHardwareId, pInfFilePath, (long)flags);
  }

  return IDCONTINUE;
}
///////////////////////////////////////////////////////////////
static int TryInstallDevice(
    const char *pInfFilePath,
    const char *pDevId,
    const char *pDevInstID,
    PCNC_DEV_CALLBACK pDevCallBack,
    void *pDevCallBackParam,
    bool updateDriver,
    BOOL *pRebootRequired)
{
  GUID classGUID;
  char className[32];

  if (!SetupDiGetINFClass(pInfFilePath, &classGUID, className, sizeof(className)/sizeof(className[0]), 0)) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiGetINFClass(%s)", pInfFilePath);
    return IDCANCEL;
  }

  //Trace("className=%s\n", className);

  HDEVINFO hDevInfo;

  hDevInfo = SetupDiCreateDeviceInfoList(&classGUID, 0);

  if (hDevInfo == INVALID_HANDLE_VALUE) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiCreateDeviceInfoList()");
    return IDCANCEL;
  }

  int res = IDCONTINUE;
  SP_DEVINFO_DATA devInfoData;

  devInfoData.cbSize = sizeof(devInfoData);

  if (!pDevInstID) {
    if (StrCmpNI(pDevId, "root\\", 5) == 0) {
      /*
       * root\<enumerator-specific-device-ID>
       */

      if (!SetupDiCreateDeviceInfo(hDevInfo, pDevId + 5, &classGUID, NULL, 0, DICD_GENERATE_ID, &devInfoData))
        res = IDCANCEL;
    } else {
      SetLastError(ERROR_INVALID_DEVINST_NAME);
      res = IDCANCEL;
    }
  }
  else
  if (StrChr(pDevInstID, '\\')) {
    /*
     * <enumerator>\<enumerator-specific-device-ID>\<instance-specific-ID>
     */

    if (!SetupDiCreateDeviceInfo(hDevInfo, pDevInstID, &classGUID, NULL, 0, 0, &devInfoData))
      res = IDCANCEL;

    if (res != IDCONTINUE && GetLastError() == ERROR_DEVINST_ALREADY_EXISTS) {
      char *pTmpDevInstID = NULL;

      if (SetStr(&pTmpDevInstID, pDevInstID)) {
        char *pSave;
        char *p;

        p = STRTOK_R(pTmpDevInstID, "\\", &pSave);

        if (p && !lstrcmp(p, REGSTR_KEY_ROOTENUM)) {
          p = STRTOK_R(NULL, "\\", &pSave);

          if (SetupDiCreateDeviceInfo(hDevInfo, p, &classGUID, NULL, 0, DICD_GENERATE_ID, &devInfoData))
            res = IDCONTINUE;
        }

        SetStr(&pTmpDevInstID, NULL);
      } else {
        SetLastError(ERROR_DEVINST_ALREADY_EXISTS);
      }
    }
  } else {
    /*
     * <enumerator-specific-device-ID>
     */

    if (!SetupDiCreateDeviceInfo(hDevInfo, pDevInstID, &classGUID, NULL, 0, DICD_GENERATE_ID, &devInfoData))
      res = IDCANCEL;
  }

  if (res != IDCONTINUE) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiCreateDeviceInfo()");
    goto exit1;
  }

  char hardwareId[MAX_DEVICE_ID_LEN + 1 + 1];

  SNPRINTF(hardwareId, sizeof(hardwareId)/sizeof(hardwareId[0]) - 1, "%s", pDevId);

  int hardwareIdLen;

  hardwareIdLen = lstrlen(hardwareId) + 1 + 1;
  hardwareId[hardwareIdLen - 1] = 0;

  if (!SetupDiSetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_HARDWAREID,
                                        (LPBYTE)hardwareId, hardwareIdLen * sizeof(hardwareId[0])))
  {
    res = IDCANCEL;
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiSetDeviceRegistryProperty()");
    goto exit1;
  }

  if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE, hDevInfo, &devInfoData)) {
    res = IDCANCEL;
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiCallClassInstaller()");
    goto exit1;
  }

  if (pDevCallBack) {
    DevProperties devProperties;

    if (!devProperties.DevId(pDevId)) {
      res = IDCANCEL;
      goto exit2;
    }

    if (!pDevCallBack(hDevInfo, &devInfoData, &devProperties, NULL, pDevCallBackParam)) {
      res = IDCANCEL;
      goto exit2;
    }
  }

  if (updateDriver)
    res = UpdateDriver(pInfFilePath, pDevId, 0, TRUE, pRebootRequired);

exit2:

  if (res != IDCONTINUE) {
    if (!SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, &devInfoData))
      ShowLastError(MB_OK|MB_ICONWARNING, "SetupDiCallClassInstaller()");
  }

exit1:

  SetupDiDestroyDeviceInfoList(hDevInfo);

  return res;
}

bool InstallDevice(
    const char *pInfFilePath,
    const char *pDevId,
    const char *pDevInstID,
    PCNC_DEV_CALLBACK pDevCallBack,
    void *pDevCallBackParam,
    bool updateDriver,
    BOOL *pRebootRequired)
{
  int res;

  do {
    res = TryInstallDevice(pInfFilePath, pDevId, pDevInstID, pDevCallBack, pDevCallBackParam, updateDriver, pRebootRequired);
  } while (res == IDTRYAGAIN);

  return res == IDCONTINUE;
}
///////////////////////////////////////////////////////////////
bool WaitNoPendingInstallEvents(int timeLimit, bool repeate)
{
  typedef DWORD  (WINAPI *PWAITNOPENDINGINSTALLEVENTS)(IN DWORD);
  static PWAITNOPENDINGINSTALLEVENTS pWaitNoPendingInstallEvents = NULL;

  if(!pWaitNoPendingInstallEvents) {
    HMODULE hModule = GetModuleHandle("setupapi.dll");

    if (!hModule)
      return FALSE;

    pWaitNoPendingInstallEvents =
        (PWAITNOPENDINGINSTALLEVENTS)GetProcAddress(hModule, "CMP_WaitNoPendingInstallEvents");

    if (!pWaitNoPendingInstallEvents)
      return FALSE;
  }

  if (int(DWORD(timeLimit * 1000)/1000) != timeLimit)
    timeLimit = -1;

  bool inTrace = FALSE;
  DWORD startTime = GetTickCount();

  for (int count = 0 ;;) {
    DWORD res = pWaitNoPendingInstallEvents(0);

    if (res == WAIT_OBJECT_0) {
      if (++count < 5) {
        Sleep(100);
        continue;
      }

      if (inTrace)
        Trace(" OK\n");

      SetLastError(ERROR_SUCCESS);
      break;
    }

    count = 0;

    if (res != WAIT_TIMEOUT) {
      ShowLastError(MB_OK|MB_ICONWARNING, "CMP_WaitNoPendingInstallEvents()");
      if (inTrace)
        Trace(" FAIL\n");
      return FALSE;
    }

    DWORD timeElapsed = GetTickCount() - startTime;

    if (timeLimit != -1 && timeElapsed >= DWORD(timeLimit * 1000)) {
      if (inTrace) {
        Trace(" timeout\n");
        inTrace = FALSE;
      }

      if (!Silent() && repeate) {
        if (ShowMsg(MB_YESNO,
            "The device installation activities are still pending.\n"
            "Continue to wait?\n") == IDYES)
        {
          startTime = GetTickCount();
        } else {
          repeate = FALSE;
        }

        continue;
      }

      SetLastError(ERROR_TIMEOUT);
      break;
    }

    if (!inTrace) {
      if (timeLimit != -1)
        Trace("Waiting for no pending device installation activities (%u secs) ", (unsigned)timeLimit);
      else
        Trace("Waiting for no pending device installation activities (perpetually) ");

      inTrace = TRUE;
    }

    Sleep(1000);

    Trace(".");
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
