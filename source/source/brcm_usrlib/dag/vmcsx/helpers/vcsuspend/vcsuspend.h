/* ============================================================================
Copyright (c) 2009-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VCSUSPEND_VCSUSPEND_H
#define VCSUSPEND_VCSUSPEND_H

#include <vcinclude/common.h>

//-----------------------------------------------------------------------------

// Suspend error type; used to determine why a suspend failed
// TODO: merge middleware and driver values, as we no longer discrimate between them.
typedef enum
{
   VCSUSPEND_ERR_MIN,
   VCSUSPEND_ERR_DEBUGDEPTH_REACHED,
   VCSUSPEND_ERR_VETOED_BY_MIDDLEWARE,
   VCSUSPEND_ERR_VETOED_BY_DRIVER,
   VCSUSPEND_ERR_MAX,
} VCSUSPEND_ERR_T;

// Callback run levels
typedef enum
{
   VCSUSPEND_RUNLEVEL_MIN,

   /* Special case drivers get one of these run levels */
   VCSUSPEND_RUNLEVEL_INTERRUPTS,
   VCSUSPEND_RUNLEVEL_MULTICORE_SYNC,
   VCSUSPEND_RUNLEVEL_SUB_CLOCK,
   VCSUSPEND_RUNLEVEL_CLOCK,
   VCSUSPEND_RUNLEVEL_CACHE,
   VCSUSPEND_RUNLEVEL_SDRAM,
   VCSUSPEND_RUNLEVEL_GPIO,
   VCSUSPEND_RUNLEVEL_SUPER_CLOCK,

   /* Normal drivers get this run level */
   VCSUSPEND_RUNLEVEL_CHIP_DRIVER_GENERAL,

   VCSUSPEND_RUNLEVEL_SYSMAN,

   /* I'm guessing that dma should be a lower level than filesys because SDCARD uses it? */
   VCSUSPEND_RUNLEVEL_HELPERS_DMALIB,

   VCSUSPEND_RUNLEVEL_FILESYS,
   VCSUSPEND_RUNLEVEL_MIDDLEWARE_GENERAL,
   VCSUSPEND_RUNLEVEL_MIDDLEWARE_DISPMANX,

   /* 3d driver must be at a higher runlevel than the dispmanx */
   VCSUSPEND_RUNLEVEL_MIDDLEWARE_KHRONOS,
   VCSUSPEND_RUNLEVEL_V3D,
   VCSUSPEND_RUNLEVEL_HDMI,

   VCSUSPEND_RUNLEVEL_VCHIQ,
   VCSUSPEND_RUNLEVEL_SDCARD_CONTROL,

   VCSUSPEND_RUNLEVEL_MAX,
} VCSUSPEND_RUNLEVEL_T;

//-----------------------------------------------------------------------------

// Types for callback functions
typedef void *VCSUSPEND_CALLBACK_ARG_T;
typedef int32_t (*VCSUSPEND_CALLBACK_FUNC_T)(uint32_t resume, VCSUSPEND_CALLBACK_ARG_T private_data);

//-----------------------------------------------------------------------------

int32_t vcsuspend_initialise(void);
void    vcsuspend_stop_test_thread(void);
uint32_t vcsuspend_is_enabled(void);
int32_t vcsuspend_register_ext
   (VCSUSPEND_RUNLEVEL_T run_level, VCSUSPEND_CALLBACK_FUNC_T func,
      VCSUSPEND_CALLBACK_ARG_T private_data, char *log_info);
int32_t vcsuspend_unregister
   (VCSUSPEND_RUNLEVEL_T run_level, VCSUSPEND_CALLBACK_FUNC_T func,
      VCSUSPEND_CALLBACK_ARG_T private_data);
int32_t vcsuspend_geterror(VCSUSPEND_ERR_T *err, int32_t *maj, int32_t *min);
int32_t vcsuspend_go(uint32_t do_halt);

#define vcsuspend_register(run_level, func, private_data) \
   vcsuspend_register_ext(run_level, func, private_data, #run_level "," __FILE__)

#define VCSUSPEND_SUSPEND  0
#define VCSUSPEND_HALT     1

#define vcsuspend_suspend()   (vcsuspend_go(VCSUSPEND_SUSPEND))
#define vcsuspend_halt()      (vcsuspend_go(VCSUSPEND_HALT))

#endif

//=============================================================================
