/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited.
All rights reserved.

Project  :  VideoCore Software Host Interface (Host-side functions)
Module   :  Software host interface (host-side)
File     :  $Id: //software/customers/broadcom/mobcom/vc4/rel/interface/vmcs_host/vc_vchi_dispmanx.h#4 $
Revision :  $Revision: #4 $

FILE DESCRIPTION
Display manager service API (VCHI native).
=============================================================================*/

#ifndef VC_VCHI_DISPMANX_H
#define VC_VCHI_DISPMANX_H

#include "interface/peer/vc_vchi_dispmanx_common.h"

#define VC_NUM_HOST_RESOURCES 64
#define DISPMANX_MSGFIFO_SIZE 1024
#define DISPMANX_CLIENT_NAME MAKE_FOURCC("DISP")
#define DISPMANX_NOTIFY_NAME MAKE_FOURCC("UPDH")

//Or with command to indicate we don't need a response
#define DISPMANX_NO_REPLY_MASK (1<<31)

typedef struct {
   char     description[32];
   uint32_t width;
   uint32_t height;
   uint32_t aspect_pixwidth;
   uint32_t aspect_pixheight;
   uint32_t fieldrate_num;
   uint32_t fieldrate_denom;
   uint32_t fields_per_frame;
   uint32_t transform;        
} GET_MODES_DATA_T;

typedef struct {
   int32_t  response;
   uint32_t width;
   uint32_t height;
   uint32_t transform;
   uint32_t input_format;
} GET_INFO_DATA_T;

//Attributes changes flag mask
#define ELEMENT_CHANGE_LAYER          (1<<0)
#define ELEMENT_CHANGE_OPACITY        (1<<1)
#define ELEMENT_CHANGE_DEST_RECT      (1<<2)
#define ELEMENT_CHANGE_SRC_RECT       (1<<3)
#define ELEMENT_CHANGE_MASK_RESOURCE  (1<<4)
#define ELEMENT_CHANGE_TRANSFORM      (1<<5)

#endif
