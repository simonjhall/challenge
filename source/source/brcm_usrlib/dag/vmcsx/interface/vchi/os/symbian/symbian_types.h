/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :  VCHI
Module   :  OS adaptaion layer
File     :  $RCSfile: powerman.h,v $
Revision :  $Revision: #3 $

FILE DESCRIPTION:
Contains Symbian-specific types etc.
=============================================================================*/

#ifndef SYMBIAN_TYPES_H_
#define SYMBIAN_TYPES_H_

typedef int                bool_t;
typedef signed char        int8_t;
typedef short int          int16_t;
typedef long int           int32_t;
typedef unsigned char      uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned long int  uint32_t;
typedef uint32_t           fourcc_t;

#ifndef NULL
#define NULL 0
#endif

#endif
