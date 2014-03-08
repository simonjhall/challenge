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

#include <stddef.h>
#include <linux/types.h>

#define VCOSPRE_ extern
#define VCOSPOST_

#define __STDC_VERSION__ 0

#if !defined(CONFIG_DEBUG_KERNEL)
#define NDEBUG
#endif

#define VCOS_BKPT BUG()

#endif
