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

#include "vcinclude/common.h"

#ifndef VCOSPRE_
   #define VCOSPRE_ extern
#endif
#define VCOSPOST_

#ifdef __GNUC__

/* Win32 assert just calls abort under GCC */
#define VCOS_ASSERT_MSG(...) _vcos_assert(__FILE__, __LINE__, __VA_ARGS__)
VCOSPRE_ void VCOSPOST_ _vcos_assert(const char *file, int line, const char *fmt, ...);

#else

#if !defined(_DEBUG) && !defined(NDEBUG)
#define NDEBUG
#endif

#include <crtdbg.h>
#define VCOS_BKPT _CrtDbgBreak()
#define VCOS_ASSERT_MSG(...) (void)((_CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, NULL, __VA_ARGS__) != 1) || (VCOS_BKPT, 0))
/* Pre-define this macro to allow the breakpoint to be conditional based on user input */
#define VCOS_ASSERT_BKPT (void)0
#define VCOS_VERIFY_BKPT (void)0

#endif /* __GNUC__ */

#define VCOS_VERIFY_MSG(...) (VCOS_VERIFY_BKPTS ? VCOS_ASSERT_MSG(__VA_ARGS__) : (void)0)

#endif


