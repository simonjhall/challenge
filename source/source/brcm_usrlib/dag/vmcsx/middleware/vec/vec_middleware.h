/*=========================================================
 Copyright � 2008 Broadcom Europe Limited. All rights reserved.

 Project  :  VEC (Video EnCoder)
 Module   :  Analogue video output middleware
 File     :  $Id: $

 FILE DESCRIPTION
 Interface for the middleware for driving the analogue TV output.
 =========================================================*/

#ifndef _VEC_MIDDLEWARE_H
#define _VEC_MIDDLEWARE_H

#include "vcinclude/common.h"
//#include "middleware/dispmanx/dispmanx_types.h"

/*************************************************************
 Types
 *************************************************************/
typedef enum VEC_MODE_T_
{
   VEC_MODE_NTSC       = 0,
   VEC_MODE_NTSC_J     = 1, // Japanese version of NTSC - no pedestal.
   VEC_MODE_PAL        = 2,
   VEC_MODE_PAL_M      = 3, // Brazilian version of PAL - 525/60 rather than 625/50, different subcarrier.
   VEC_MODE_FORMAT_MASK = 0x3,
   
   VEC_MODE_YPRPB      = 4,
   VEC_MODE_RGB        = 8,
   VEC_MODE_OUTPUT_MASK = 0xc,

   VEC_MODE_YPRPB_480i = (VEC_MODE_NTSC | VEC_MODE_YPRPB),
   VEC_MODE_RGB_480i   = (VEC_MODE_NTSC | VEC_MODE_RGB),
   VEC_MODE_YPRPB_576i = (VEC_MODE_PAL  | VEC_MODE_YPRPB),
   VEC_MODE_RGB_576i   = (VEC_MODE_PAL  | VEC_MODE_RGB)

} VEC_MODE_T;

typedef enum VEC_ASPECT_T_
{
   // TODO: extend this to allow picture placement/size to be communicated.
   VEC_ASPECT_UNKNOWN  = 0,
   VEC_ASPECT_4_3      = 1,
   VEC_ASPECT_14_9     = 2,
   VEC_ASPECT_16_9     = 3
} VEC_ASPECT_T;

typedef struct VEC_OPTIONS_T_
{
   VEC_ASPECT_T   aspect;
} VEC_OPTIONS_T;

/*************************************************************
 Functions
 *************************************************************/
/***********************************************************
 * Name: vec_middleware_power_on
 * 
 * Arguments: 
 *       VEC_MODE_T     mode           - the mode to start up with.
 *       uint32_t       display_number - display number to attach to.
 *       uint8_t        copyprotect_on - enable copy protection if true.
 *       VEC_OPTIONS_T *options        - additional output options.
 *
 * Description: Initializes the VEC hardware and claims required
 *    resources.
 *
 * Returns: DISPMANX_STATUS_T ( < 0 is fail)
 *
 ***********************************************************/
DISPMANX_STATUS_T vec_middleware_power_on (VEC_MODE_T mode, uint32_t display_number, uint8_t copyprotect_on, const VEC_OPTIONS_T *options);

/***********************************************************
 * Name: vec_middleware_power_off
 * 
 * Arguments: 
 *       void
 * 
 * Description: Disables output and frees resources.
 *
 * Returns: DISPMANX_STATUS_T ( < 0 is fail)
 *
 ***********************************************************/
DISPMANX_STATUS_T vec_middleware_power_off (void);

/***********************************************************
 * Name: vec_middleware_set_mode
 * 
 * Arguments: 
 *       VEC_MODE_T mode  - the new mode to use.
 * 
 * Description: Changes the mode. Side effects on dispman-X
 *    and other areas may depend on the change involved, e.g.
 *    changes that leave the size constant require no change
 *    to dispman-X.
 *
 * Returns: int32_t ( < 0 is fail)
 *
 ***********************************************************/
int32_t vec_middleware_set_mode (VEC_MODE_T mode);

#endif
