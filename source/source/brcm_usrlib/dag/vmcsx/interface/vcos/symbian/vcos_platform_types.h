/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  osal
File     :  $RCSfile: $
Revision :  $Revision: #1 $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - platform-specific types and defines
=============================================================================*/

#ifndef VCOS_PLATFORM_TYPES_H
#define VCOS_PLATFORM_TYPES_H

#define VCOSPRE_     extern
#define VCOSPOST_
//#define VCCPRE_     EXPORT_C

#include "vcinclude/common.h"
#include <stddef.h> // for size_t

extern "C" void _vcos_assert(const char *file, const char *func, int line, const char *format, ...);

#ifdef __CC_ARM
#define VCOS_ASSERT_MSG(...) _vcos_assert(__FILE__, __func__, __LINE__, __VA_ARGS__)
#else
// Fudge to get past dependency pre-process stage which uses old GCC
#define VCOS_ASSERT_MSG(A...)
#define vcos_assert_msg(A...)
#define vcos_demand_msg(A...)
#define vcos_verify_msg(A...)
#endif

#endif


