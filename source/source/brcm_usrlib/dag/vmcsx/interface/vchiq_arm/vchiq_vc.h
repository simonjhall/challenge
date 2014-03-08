/*=============================================================================
Copyright (c) 2010 Broadcom Europe Ltd. All rights reserved.

Project  :  ARM
Module   :  vchiq_arm

FILE DESCRIPTION:
Header file for VideoCore frontend to VCHIQ module.

==============================================================================*/

#ifndef VCHIQ_VC_H
#define VCHIQ_VC_H

#include "vchiq.h"
#include "vcfw/drivers/chip/arm.h"

extern int vchiq_vc_initialise(const ARM_DRIVER_T *arm, DRIVER_HANDLE_T handle, uint32_t channelbase);

#endif
