/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  chip driver
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - public header file
=============================================================================*/

#ifndef VCOS_STRING_H
#define VCOS_STRING_H

/**
  * \file
  *
  * String functions.
  *
  */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h> // For size_t
#include "interface/vcos/vcos_types.h"
#include "vcos_platform.h"

/** Case insensitive string comparison.
  *
  */

VCOS_INLINE_DECL
int vcos_strcasecmp(const char *s1, const char *s2);

VCOS_INLINE_DECL
int vcos_strncasecmp(const char *s1, const char *s2, size_t n);

VCOSPRE_ int VCOSPOST_ vcos_snprintf(char *buf, size_t buflen, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif
