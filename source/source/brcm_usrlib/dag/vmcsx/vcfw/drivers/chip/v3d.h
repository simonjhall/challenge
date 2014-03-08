/* ============================================================================
Copyright (c) 2010-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef _V3D_H_
#define _V3D_H_

#include "vcfw/drivers/driver.h"
#include "vcinclude/common.h"

/******************************************************************************
public types
******************************************************************************/

typedef void (*V3D_CALLBACK_T)(void *p);
typedef void (*V3D_MAIN_ISR_T)(uint32_t flags);
typedef void (*V3D_USH_ISR_T)(uint32_t flags);

typedef struct {
   V3D_CALLBACK_T callback;
   V3D_MAIN_ISR_T main_isr;
   V3D_USH_ISR_T ush_isr;
} V3D_PARAMS_T;

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
api
******************************************************************************/

/* routine to acquire control of the main part of the v3d block (the cle + co) */
typedef int32_t (*V3D_ACQUIRE_MAIN)(DRIVER_HANDLE_T handle,
                                    void *callback_p,
                                    uint32_t actual_user_vpm,
                                    uint32_t max_user_vpm,
                                    int force_reset);

/* routine to release control of the main part of the v3d block (the cle + co) */
typedef int32_t (*V3D_RELEASE_MAIN)(DRIVER_HANDLE_T handle,
                                    uint32_t max_user_vpm);

/* routine to acquire control of the user shader queue */
typedef int32_t (*V3D_ACQUIRE_USH)(DRIVER_HANDLE_T handle,
                                   void *callback_p,
                                   uint32_t actual_user_vpm,
                                   uint32_t min_user_vpm,
                                   int force_reset);

/* routine to release control of the user shader queue */
typedef int32_t (*V3D_RELEASE_USH)(DRIVER_HANDLE_T handle,
                                   uint32_t min_user_vpm);

/* routine to acquire control of both the main part of the v3d block (the cle +
 * co) and the user shader queue. this is a bit of a hack to allow isp to
 * workaround hw-2422 on a0, and hw-2959 on b0... */
typedef int32_t (*V3D_ACQUIRE_MAIN_AND_USH)(DRIVER_HANDLE_T handle,
                                            void *callback_p,
                                            uint32_t user_vpm);

/******************************************************************************
system driver struct
******************************************************************************/

typedef struct {
   /* common api */
   COMMON_DRIVER_API(const V3D_PARAMS_T *)

   /* v3d api */
   V3D_ACQUIRE_MAIN acquire_main;
   V3D_RELEASE_MAIN release_main;
   V3D_ACQUIRE_USH acquire_ush;
   V3D_RELEASE_USH release_ush;
   V3D_ACQUIRE_MAIN_AND_USH acquire_main_and_ush;
} V3D_DRIVER_T;

/******************************************************************************
public function declarations
******************************************************************************/

#if defined(_ATHENA_)
uint32_t  v3d_read_reg(uint32_t addr);
#define   v3d_write_reg(addr, X) ((*(volatile unsigned long *)(addr)) = (X))  
#else
#define v3d_read_reg(x)  (x)
#define v3d_write_reg(addr, X)  (addr = (X))
#endif

#ifdef BRCM_V3D_OPT
#define v3d_read(REG) (0)
#define v3d_write(REG, X) (0)
#else
#define v3d_read(REG) (v3d_read_reg(V3D_##REG))
#define v3d_write(REG, X) ((void)(v3d_write_reg(V3D_##REG, X)))
#endif


const V3D_DRIVER_T *v3d_get_func_table(void);

#ifdef __cplusplus
}
#endif //  __cplusplus

#endif // _V3D_H_
