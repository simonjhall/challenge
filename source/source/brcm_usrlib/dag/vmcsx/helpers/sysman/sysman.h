/* ============================================================================
Copyright (c) 2005-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef SYSMAN_H
#define SYSMAN_H

#include "vcinclude/common.h"

/******************************************************************************
 Global Types
 *****************************************************************************/

typedef enum
{
   SYSMAN_BLOCK_MIN,

   SYSMAN_BLOCK_I2C0,
   SYSMAN_BLOCK_I2C1,
   SYSMAN_BLOCK_I2C2,
   SYSMAN_BLOCK_VIDEO_SCALER, // this is _not_ the same as HVS!
   SYSMAN_BLOCK_VPU1,
   SYSMAN_BLOCK_HDMI,
   SYSMAN_BLOCK_USB,
   SYSMAN_BLOCK_VEC,
   SYSMAN_BLOCK_JPEG,
   SYSMAN_BLOCK_H264,
   SYSMAN_BLOCK_V3D,
   SYSMAN_BLOCK_ISP,
   // on videocore4 we have unicam0,1
   SYSMAN_BLOCK_UNICAM0,
   SYSMAN_BLOCK_UNICAM1,
   // on videocore3 we have ccp2(rx),cpi,csi2
   SYSMAN_BLOCK_CCP2RX,
   SYSMAN_BLOCK_CSI2,
   SYSMAN_BLOCK_CPI,
   //
   SYSMAN_BLOCK_DSI0, // this is the 2-lane controller
   SYSMAN_BLOCK_DSI1, // this is the 4-lane controller (2708 only)
   SYSMAN_BLOCK_TRANSPOSER,

   SYSMAN_BLOCK_CCP2TX,
   SYSMAN_BLOCK_CDP, // this is 'common display peripheral' but only exists on 2707

   SYSMAN_BLOCK_ARM,

   SYSMAN_BLOCK_MAX

} SYSMAN_BLOCK_T;

typedef enum
{
   SYSMAN_WAIT_NON_BLOCKING,
   SYSMAN_WAIT_BLOCKING

} SYSMAN_WAIT_T;

typedef struct SYSMAN_HANDLE_S * SYSMAN_HANDLE_T;

/******************************************************************************
 Global Funcs
 *****************************************************************************/

#define SYSMAN_TRACKING

/* Routine to initialise the power manager */
extern int32_t sysman_initialise( void );

extern int32_t sysman_register_user_ext( SYSMAN_HANDLE_T *sysman_handle, const char *fname, int line );

/* Routine used to register to control block clock enables */
#ifdef SYSMAN_TRACKING
#   define sysman_register_user( x ) sysman_register_user_ext( x , __FILE__, __LINE__)
#else
#   define sysman_register_user( x ) sysman_register_user_ext( x )
#endif

/* Routine used to deregister from the sysman */
extern int32_t sysman_deregister_user( SYSMAN_HANDLE_T sysman_handle );

/* Routine to request the users clock requirements */
extern int32_t sysman_set_user_request( SYSMAN_HANDLE_T sysman_handle,
                                        SYSMAN_BLOCK_T block,
                                        uint32_t required,
                                        SYSMAN_WAIT_T wait);

/* Routine to get the state of the block */
extern int32_t sysman_is_enabled( SYSMAN_BLOCK_T block );

// these two fns allow sysman_display to see inside sysman...
extern uint32_t sysman_domains_enabled( void );
extern uint32_t sysman_blocks_enabled( void );

#endif /* SYSMAN_H */

/****************************** End of file **********************************/
