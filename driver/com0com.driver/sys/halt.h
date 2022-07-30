/*
 * $Id: halt.h,v 1.3 2010/08/09 12:50:58 vfrolov Exp $
 *
 * Copyright (c) 2005-2010 Vyacheslav Frolov
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
 * $Log: halt.h,v $
 * Revision 1.3  2010/08/09 12:50:58  vfrolov
 * Added try DbgBreakPoint() before KeBugCheckEx()
 *
 * Revision 1.2  2010/08/04 10:38:55  vfrolov
 * Minimized PREfast noise
 *
 * Revision 1.1  2008/12/02 16:07:37  vfrolov
 * Initial revision
 *
 */

#ifndef _HALT_H_
#define _HALT_H_

#if defined(_PREFAST_)
  #define HALT_UNLESS3(exp, p1, p2, p3) __assume(exp)
#elif DBG
  #define HALT_UNLESS3(exp, p1, p2, p3) \
    if (!(exp)) { \
      try { \
        DbgPrint("%s (%d):\n", __FILE__, __LINE__); \
        DbgPrint("  HALT_UNLESS3(\"%s\",\n", #exp); \
        DbgPrint("               0x%08X, 0x%08X, 0x%08X);\n", (ULONG)(p1), (ULONG)(p2), (ULONG)(p3)); \
        DbgBreakPoint(); \
      } except (EXCEPTION_EXECUTE_HANDLER) { \
        KeBugCheckEx(0xDEADC0CD, (FILE_ID << 16) + __LINE__, (ULONG_PTR)(p1), (ULONG_PTR)(p2), (ULONG_PTR)(p3)); \
      } \
    }
#else
  #define HALT_UNLESS3(exp, p1, p2, p3)
#endif

#define HALT_UNLESS2(exp, p1, p2) HALT_UNLESS3(exp, p1, p2, 0)
#define HALT_UNLESS1(exp, p1)     HALT_UNLESS3(exp, p1, 0, 0)
#define HALT_UNLESS(exp)          HALT_UNLESS3(exp, 0, 0, 0)

#endif /* _HALT_H_ */
