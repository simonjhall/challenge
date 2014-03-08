/*=============================================================================
Copyright (c) 2006 Broadcom Europe Limited.
Copyright (c) 2002 Alphamosaic Limited.
All rights reserved.

Project  :  VideoCore Software Host Interface (Host-side functions)
Module   :  Host-specific functions
File     :  $RCSfile: vchost_config.h,v $
Revision :  $Revision: #2 $

FILE DESCRIPTION
Platform specific configuration parameters.
=============================================================================*/

#ifndef VCHOST_CONFIG_H
#define VCHOST_CONFIG_H

#include "interface/vcos/vcos.h"

#if 0
/* Types that map onto VideoCore's types of the same name. */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef long int32_t;
typedef unsigned long uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

#ifndef vc_assert
#define vc_assert(cond) vcos_assert(cond)
#endif
#endif

/* On this platform we need to be able to release the host-side software resources. */
extern void vc_os_close(void);

#ifndef VCHPRE_
#define VCHPRE_     extern
#endif
#ifndef VCHPOST_
#define VCHPOST_
#endif
#ifndef VCCPRE_
#define VCCPRE_     
#endif

#endif
