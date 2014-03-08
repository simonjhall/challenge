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

#include <assert.h>

#include "vcinclude/vcore.h"
#include "interface/vcos/vcos.h"

/* On this platform we need to be able to release the host-side software resources. */
extern void vc_os_close(void);

/* On the VideoCore platform we have malloc. */
#define HOST_MALLOC_EXISTS

#endif
