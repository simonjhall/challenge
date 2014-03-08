/*=============================================================================
Copyright (c) 2007 Broadcom Europe Limited.
All rights reserved.

Project  :  VideoCore Software Host Interface (Host-side functions)
Module   :  Khronos (host-side)
File     :  $RCSfile: vcilcs.h,v $
Revision :  $Revision: 1.1.2.1 $

FILE DESCRIPTION
Khronos Service host side API
=============================================================================*/

#ifndef VC_VCHI_KHRONOS_H
#define VC_VCHI_KHRONOS_H

#include "vchost_config.h"
#include "interface/vchi/vchi.h"


/* Initialise Khronos service. This initialises the host side of the interface, 
   it does not send anything to VideoCore. */
VCHPRE_ void VCHPOST_ vc_vchi_khronos_init( VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections );

#endif // ifndef VC_VCHI_KHRONOS_H
