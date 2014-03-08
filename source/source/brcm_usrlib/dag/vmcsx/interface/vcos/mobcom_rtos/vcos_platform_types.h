/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  osal
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - platform-specific types and defines
=============================================================================*/

#ifndef VCOS_PLATFORM_TYPES_H
#define VCOS_PLATFORM_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vcinclude/common.h"

#ifndef VCOSPRE_
   #define VCOSPRE_ extern
#endif
#define VCOSPOST_

#if defined(__GNUC__) || defined(__CC_ARM)

/* Win32 assert just calls abort under GCC */
#ifndef NDEBUG
#define vcos_assert(cond) if (!(cond)){_vcos_assert(__FILE__, __LINE__, #cond);}
#else
#define vcos_assert(cond) (void)0
#endif
VCOSPRE_ void VCOSPOST_ _vcos_assert(const char *file, int line, const char *desc);

#else

#include <crtdbg.h>
#define vcos_assert _ASSERT

#endif /* __GNUC__ */

#ifdef __cplusplus
}
#endif

#endif


