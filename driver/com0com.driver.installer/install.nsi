/*
 * $Id: install.nsi,v 1.25 2011/12/18 16:42:23 vfrolov Exp $
 *
 * Copyright (c) 2006-2011 Vyacheslav Frolov
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
 * $Log: install.nsi,v $
 * Revision 1.25  2011/12/18 16:42:23  vfrolov
 * Added ability to build installer with both 32-bit and 64-bit drivers
 *
 * Revision 1.24  2011/12/12 13:17:43  vfrolov
 * Added CNC_INSTALL_SKIP_SETUP_PREINSTALL and CNC_UNINSTALL_SKIP_SETUP_UNINSTALL environment variables
 *
 * Revision 1.23  2011/07/14 15:02:41  vfrolov
 * Added packaging com0com.cat (if file exists)
 *
 * Revision 1.22  2011/07/14 12:12:28  vfrolov
 * Fixed mistaken execution of executables located in installer's folder
 *
 * Revision 1.21  2011/07/12 18:10:11  vfrolov
 * Added launching setupg.exe rather then setupc.exe if .NET is OK
 * Disabled installing ports by default while update
 *
 * Revision 1.20  2011/07/08 10:19:19  vfrolov
 * Added ability to set selections for setup.exe by setting environment variables
 *
 * Revision 1.19  2010/06/01 06:14:09  vfrolov
 * Improved driver updating
 *
 * Revision 1.18  2010/05/27 11:16:46  vfrolov
 * Added ability to put the port to the Ports class
 *
 * Revision 1.17  2009/05/22 11:32:52  vfrolov
 * Added URLInfoAbout, InstallLocation, InstallSource, Language, Version
 * and EstimatedSize to the registry
 * Added Windows version check
 *
 * Revision 1.16  2009/05/21 15:39:34  vfrolov
 * Added DisplayIcon, DisplayVersion, VersionMajor, VersionMinor
 * and QuietUninstallString to the registry
 *
 * Revision 1.15  2009/05/20 13:02:18  vfrolov
 * Changed MUI.nsh to MUI2.nsh
 * Added .NET check and advise
 * Disabled silent installing of linked ports
 *
 * Revision 1.14  2009/01/12 13:16:20  vfrolov
 * Added driver updating
 *
 * Revision 1.13  2008/09/12 12:29:53  vfrolov
 * Added --silent option
 *
 * Revision 1.12  2007/11/30 09:53:37  vfrolov
 * Added license page
 *
 * Revision 1.11  2007/11/23 08:23:29  vfrolov
 * Added popup for uncompatible CPU
 *
 * Revision 1.10  2007/11/22 11:36:41  vfrolov
 * Moved output file to target CPU directory
 * Disabled moving Start Menu shortcuts to all users for Vista
 * Fixed title truncation
 * Added show ReadMe checkbox
 * Added setupg.exe
 *
 * Revision 1.9  2007/11/15 12:12:04  vfrolov
 * Removed Function LaunchSetupCommandPrompt
 * Added MUI_FINISHPAGE_LINK
 *
 * Revision 1.8  2007/10/30 15:06:14  vfrolov
 * Added changing working directory before removing $INSTDIR
 *
 * Revision 1.7  2007/10/25 14:30:27  vfrolov
 * Replaced setup.bat by setupc.exe
 *
 * Revision 1.6  2007/08/08 14:15:16  vfrolov
 * Added missing SetOutPath
 *
 * Revision 1.5  2007/01/22 17:10:32  vfrolov
 * Partially added support for non i386 CPUs
 *
 * Revision 1.4  2006/12/14 08:25:44  vfrolov
 * Added ReadMe.lnk
 *
 * Revision 1.3  2006/11/22 07:58:45  vfrolov
 * Changed uninstall keys
 *
 * Revision 1.2  2006/11/21 11:43:42  vfrolov
 * Added Modern UI
 * Added "CNCA0<->CNCB0" section
 * Added "Launch Setup Command Prompt" on finish page
 *
 * Revision 1.1  2006/10/23 12:26:02  vfrolov
 * Initial revision
 *
 */

;--------------------------------

  !include "MUI2.nsh"
  !include "WinVer.nsh"
  !include "x64.nsh"
  !include WordFunc.nsh
  !insertmacro VersionCompare
  !include "FileFunc.nsh"

;--------------------------------

!ifndef OUTPUT_FILE
  !ifndef ADD_TARGET_CPU_i386
    !ifndef ADD_TARGET_CPU_x64
      !ifndef ADD_TARGET_CPU_ia64
        !if "$%BUILD_DEFAULT_TARGETS%" == "-386"
          !define OUTPUT_FILE "..\i386\setup.exe"
          !define ADD_TARGET_CPU_i386
        !else if "$%BUILD_DEFAULT_TARGETS%" == "-AMD64"
          !define OUTPUT_FILE "..\amd64\setup.exe"
          !define ADD_TARGET_CPU_amd64
        !else if "$%BUILD_DEFAULT_TARGETS%" == "-amd64"
          !define OUTPUT_FILE "..\amd64\setup.exe"
          !define ADD_TARGET_CPU_amd64
        !else if "$%BUILD_DEFAULT_TARGETS%" == "-IA64"
          !define OUTPUT_FILE "..\ia64\setup.exe"
          !define ADD_TARGET_CPU_ia64
        !else if "$%BUILD_DEFAULT_TARGETS%" == "-ia64"
          !define OUTPUT_FILE "..\ia64\setup.exe"
          !define ADD_TARGET_CPU_ia64
        !else
          !define OUTPUT_FILE "..\i386\setup.exe"
          !define ADD_TARGET_CPU_i386
          !Warning "Using target CPU i386"
        !endif
      !endif
    !endif
  !endif
!endif

!ifndef CONFIGURATION
    !define CONFIGURATION "Release"
!endif

!ifndef OUTPUT_FILE
  !Error "Not defined OUTPUT_FILE"
!endif

;--------------------------------

Function GetDotNETVersion
  Push $0
  Push $1

  System::Call "mscoree::GetCORVersion(w .r0, i ${NSIS_MAX_STRLEN}, *i) i .r1 ?u"
  StrCmp $1 0 +2
    StrCpy $0 "v0"

  StrCpy $0 $0 "" 1 # skip "v"

  Pop $1
  Exch $0
FunctionEnd

;--------------------------------

Function AdviseDotNETVersion
  IfSilent 0 +2
    return

  Push $0
  Push $1
  Push $2

  Call GetDotNETVersion
  Pop $0

  StrCpy $1 "2.0"

  ${VersionCompare} $0 $1 $2
  ${If} $2 == 2
    MessageBox MB_OK|MB_ICONINFORMATION \
      "To use GUI-based Setup utility you will need to$\ninstall Microsoft .NET Framework v$1 or newer."
  ${EndIf}

  Pop $2
  Pop $1
  Pop $0
FunctionEnd

;--------------------------------

Function LaunchSetup
  IfSilent 0 +2
    return

  Push $0
  Push $1
  Push $2

  Call GetDotNETVersion
  Pop $0

  StrCpy $1 "2.0"

  ${VersionCompare} $0 $1 $2
  ${If} $2 == 2
    Exec '"$INSTDIR\setupc.exe"'
  ${Else}
    Exec '"$INSTDIR\setupg.exe"'
  ${EndIf}

  Pop $2
  Pop $1
  Pop $0
FunctionEnd

;--------------------------------

!macro MoveFileToDetails file

  Push $0
  Push $1
  Push $2
  Push $3

  StrCpy $0 "${file}"

  FileOpen $1 $0 r
  IfErrors +9

    FileRead $1 $2
    IfErrors +7

    StrCpy $3 $2 2 -2
    StrCmp $3 "$\r$\n" 0 +2
      StrCpy $2 $2 -2

    StrCmp $2 "" +2
      DetailPrint $2

    Goto -7

  FileClose $1
  Delete $0

  Pop $3
  Pop $2
  Pop $1
  Pop $0

!macroend

;--------------------------------

!macro EnvToSel section env

  Push $0
  Push $1

  ReadEnvStr $0 ${env}
  StrCpy $0 $0 1
  ${Select} $0
    ${Case2} "Y" "y"
      SectionGetFlags ${section} $0
      IntOp $0 $0 | ${SF_SELECTED}
      SectionSetFlags ${section} $0
    ${Case2} "N" "n"
      SectionGetFlags ${section} $0
      IntOp $1 ${SF_SELECTED} ~
      IntOp $0 $0 & $1
      SectionSetFlags ${section} $0
  ${EndSelect}
  Pop $1
  Pop $0

!macroend

;--------------------------------

; The name of the installer
Name "Null-modem emulator (com0com)"

; The file to write
;!Warning "Using output file '${OUTPUT_FILE}'"
OutFile "${OUTPUT_FILE}"

; The default installation directory
InstallDir $PROGRAMFILES\com0com

; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\com0com" "Install_Dir"

;Vista redirects $SMPROGRAMS to all users without this
RequestExecutionLevel admin

ShowInstDetails show
ShowUninstDetails show

;--------------------------------
; Pages

  !define MUI_WELCOMEPAGE_TITLE_3LINES
  !define MUI_FINISHPAGE_TITLE_3LINES

  !define MUI_FINISHPAGE_RUN
  !define MUI_FINISHPAGE_RUN_TEXT "Launch Setup"
  !define MUI_FINISHPAGE_RUN_FUNCTION LaunchSetup
  !define MUI_FINISHPAGE_RUN_NOTCHECKED

  !define MUI_FINISHPAGE_SHOWREADME readme.txt
  !define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED

  !define MUI_FINISHPAGE_LINK "Visit com0com homepage"
  !define MUI_FINISHPAGE_LINK_LOCATION http://com0com.sourceforge.net/

  !define MUI_FINISHPAGE_NOAUTOCLOSE

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "license.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !define MUI_WELCOMEPAGE_TITLE_3LINES
  !define MUI_FINISHPAGE_TITLE_3LINES
  !define MUI_UNFINISHPAGE_NOAUTOCLOSE

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
; Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------

!macro CpuSection cpu

  !ifdef ADD_TARGET_CPU_${cpu}

    ;!Warning "Adding CPU ${cpu}"

    Section /o "-com0com ${cpu}" sec_com0com_${cpu}

      ; Set output path to the installation directory.
      SetOutPath $INSTDIR

      ; Put target cpu files there
      File "$%C0C_BUILD_PATH%..\driver\com0com.driver.package\com0com.sys"
      File /nonfatal "$%C0C_BUILD_PATH%..\driver\com0com.driver.package\com0com.cat"
      File "$%C0C_BUILD_PATH%..\driver\com0com.driver.package\com0com.inf"
      File "$%C0C_BUILD_PATH%..\driver\com0com.driver.package\cncport.inf"
      File "$%C0C_BUILD_PATH%..\driver\com0com.driver.package\comport.inf"
      File "$%C0C_BUILD_PATH%..\setup\setup.dll"
      File "$%C0C_BUILD_PATH%..\setup\setupc.exe"
      File "$%C0C_BUILD_PATH%..\setup\setupg.exe"

    SectionEnd

  !endif

!macroend

;--------------------------------

!insertmacro CpuSection i386
!insertmacro CpuSection x64
!insertmacro CpuSection ia64

;--------------------------------

Section "com0com" sec_com0com

  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Put files there
  File "readme.txt"

  WriteUninstaller "uninstall.exe"

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\com0com "Install_Dir" "$INSTDIR"

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "DisplayName" "Null-modem emulator (com0com)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "Publisher" "Vyacheslav Frolov"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "HelpLink" "http://com0com.sourceforge.net/"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "URLUpdateInfo" "http://com0com.sourceforge.net/"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "URLInfoAbout" "http://com0com.sourceforge.net/"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "Readme" "$INSTDIR\readme.txt"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "DisplayIcon" "$INSTDIR\setupg.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "InstallLocation" "$INSTDIR\"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "InstallSource" "$EXEDIR\"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "Language" $LANGUAGE

  GetDLLVersion "$INSTDIR\com0com.sys" $R0 $R1
  IntOp $R2 $R0 / 0x00010000
  IntOp $R3 $R0 & 0x0000FFFF
  IntOp $R4 $R1 / 0x00010000
  IntOp $R5 $R1 & 0x0000FFFF
  IntOp $R5 $R1 & 0x0000FFFF
  IntOp $R6 $R2 * 0x00000100
  IntOp $R6 $R6 | $R3
  IntOp $R6 $R6 * 0x00000100
  IntOp $R6 $R6 | $R4
  IntOp $R6 $R6 * 0x00000100
  IntOp $R6 $R6 | $R5

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "DisplayVersion" "$R2.$R3.$R4.$R5"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "Version" $R6
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "VersionMajor" $R2
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "VersionMinor" $R3

  ${GetSize} "$INSTDIR" "/S=0B" $R0 $R1 $R2
  IntOp $R0 $R0 / 1024  ; in KBytes

  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "EstimatedSize" $R0

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "NoRepair" 1

  ReadEnvStr $0 "CNC_INSTALL_SKIP_SETUP_PREINSTALL"
  StrCpy $0 $0 1
  ${Select} $0
    ${Case2} "Y" "y"
      DetailPrint 'Skipped "$INSTDIR\setupc.exe" preinstall/update/infclean'
    ${CaseElse}
      GetTempFileName $0

      StrCpy $1 ""
      IfSilent 0 +2
      StrCpy $1 "--silent"

      ExecWait '"$INSTDIR\setupc.exe" $1 --output "$0" preinstall'
      !insertmacro MoveFileToDetails $0

      ExecWait '"$INSTDIR\setupc.exe" $1 --output "$0" update'
      !insertmacro MoveFileToDetails $0

      ExecWait '"$INSTDIR\setupc.exe" $1 --output "$0" infclean'
      !insertmacro MoveFileToDetails $0
  ${EndSelect}

SectionEnd

;--------------------------------

Section "Start Menu Shortcuts" sec_shortcuts

  CreateDirectory "$SMPROGRAMS\com0com"
  CreateShortCut "$SMPROGRAMS\com0com\Setup Command Prompt.lnk" "$INSTDIR\setupc.exe"
  CreateShortCut "$SMPROGRAMS\com0com\Setup.lnk" "$INSTDIR\setupg.exe"
  CreateShortCut "$SMPROGRAMS\com0com\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\com0com\readme.lnk" "$INSTDIR\readme.txt"

SectionEnd

;--------------------------------

Section "CNCA0 <-> CNCB0" sec_CNCxCNC_ports

  GetTempFileName $0

  StrCpy $1 ""
  IfSilent 0 +2
  StrCpy $1 "--silent"

  ExecWait '"$INSTDIR\setupc.exe" $1 --output "$0" install 0 - -'
  !insertmacro MoveFileToDetails $0

SectionEnd

;--------------------------------

Section "COM# <-> COM#" sec_COMxCOM_ports

  GetTempFileName $0

  StrCpy $1 ""
  IfSilent 0 +2
  StrCpy $1 "--silent"

  ExecWait '"$INSTDIR\setupc.exe" $1 --output "$0" install PortName=COM# PortName=COM#'
  !insertmacro MoveFileToDetails $0

SectionEnd

;--------------------------------

Function .onInit

  ; Check CPU

  ${If} ${RunningX64}
    !ifdef ADD_TARGET_CPU_x64
      SectionGetFlags ${sec_com0com_x64} $0
      IntOp $0 $0 | ${SF_SELECTED}
      SectionSetFlags ${sec_com0com_x64} $0
    !else
      MessageBox MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION \
        "This package does not include 64-bit driver required for your system.$\n$\nContinue?" \
        /SD IDNO IDYES +2
      Abort
    !endif
  ${Else}
    !ifdef ADD_TARGET_CPU_i386
      SectionGetFlags ${sec_com0com_i386} $0
      IntOp $0 $0 | ${SF_SELECTED}
      SectionSetFlags ${sec_com0com_i386} $0
    !else
      MessageBox MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION \
        "This package does not include 32-bit driver required for your system.$\n$\nContinue?" \
        /SD IDNO IDYES +2
      Abort
    !endif
  ${EndIf}

  ; Check Windows version

  ${IfNot} ${AtLeastWin2000}
    MessageBox MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION \
      "The driver cannot run under below Windows 2000 System.$\n$\nContinue?" \
      /SD IDNO IDYES +2
    Abort
  ${EndIf}

  ; Disable installing ports by default if update or silent install

  ClearErrors
  ReadRegStr $0 HKLM SOFTWARE\com0com "Install_Dir"
  IfErrors 0 disable_ports_begin

  IfSilent disable_ports_begin 0
  Goto disable_ports_end

  disable_ports_begin:
    SectionGetFlags ${sec_CNCxCNC_ports} $0
    IntOp $1 ${SF_SELECTED} ~
    IntOp $0 $0 & $1
    SectionSetFlags ${sec_CNCxCNC_ports} $0
    SectionGetFlags ${sec_COMxCOM_ports} $0
    IntOp $1 ${SF_SELECTED} ~
    IntOp $0 $0 & $1
    SectionSetFlags ${sec_COMxCOM_ports} $0
  disable_ports_end:

  ; Set selections from enviroment

  !insertmacro EnvToSel ${sec_shortcuts}     "CNC_INSTALL_START_MENU_SHORTCUTS"
  !insertmacro EnvToSel ${sec_CNCxCNC_ports} "CNC_INSTALL_CNCA0_CNCB0_PORTS"
  !insertmacro EnvToSel ${sec_COMxCOM_ports} "CNC_INSTALL_COMX_COMX_PORTS"

FunctionEnd

;--------------------------------

Function .onInstSuccess
  Call AdviseDotNETVersion
FunctionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ReadEnvStr $0 "CNC_UNINSTALL_SKIP_SETUP_UNINSTALL"
  StrCpy $0 $0 1
  ${Select} $0
    ${Case2} "Y" "y"
      DetailPrint 'Skipped "$INSTDIR\setupc.exe" uninstall'
    ${CaseElse}
      GetTempFileName $0

      StrCpy $1 ""
      IfSilent 0 +2
      StrCpy $1 "--silent"

      ExecWait '"$INSTDIR\setupc.exe" $1 --output "$0" uninstall'
      !insertmacro MoveFileToDetails $0
  ${EndSelect}

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com"
  DeleteRegKey HKLM SOFTWARE\com0com

  ; Remove files and uninstaller
  Delete $INSTDIR\readme.txt
  Delete $INSTDIR\com0com.inf
  Delete $INSTDIR\cncport.inf
  Delete $INSTDIR\comport.inf
  Delete $INSTDIR\com0com.sys
  Delete $INSTDIR\com0com.cat
  Delete $INSTDIR\setup.dll
  Delete $INSTDIR\setupc.exe
  Delete $INSTDIR\setupg.exe
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\com0com\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\com0com"
  SetOutPath $TEMP
  RMDir "$INSTDIR"

SectionEnd

;--------------------------------

  ;Language strings
  LangString DESC_sec_com0com ${LANG_ENGLISH} "\
    Install com0com files. \
    "
  LangString DESC_sec_shortcuts ${LANG_ENGLISH} "\
    Add shortcuts to the Start Menu. \
    "
  LangString DESC_sec_CNCxCNC_ports ${LANG_ENGLISH} "\
    Install a pair of linked ports by CNCPorts class installer. \
    The CNCPorts class installer sets the port names to CNCA0 and CNCB0. \
    "
  LangString DESC_sec_COMxCOM_ports ${LANG_ENGLISH} "\
    Install a pair of linked ports by Ports class installer. \
    The Ports class installer selects the COM port number for each port and \
    sets the port name to COM#, where # is the selected port number. \
    "

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_com0com} $(DESC_sec_com0com)
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_shortcuts} $(DESC_sec_shortcuts)
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_CNCxCNC_ports} $(DESC_sec_CNCxCNC_ports)
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_COMxCOM_ports} $(DESC_sec_COMxCOM_ports)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
