/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef KHRN_MISC_H
#define KHRN_MISC_H

//if you want KHRN_USE_VCHIQ define it in the platform makefile
//e.g in vcfw/platform/broadcom/2763dbrev2/2763dbrev2_linux.mk
//#ifndef KHRN_USE_VCHIQ
//#define KHRN_USE_VCHIQ
//#endif

#include "middleware/khronos/common/khrn_hw.h"

#include "middleware/khronos/dispatch/khrn_dispatch.h"

#include "interface/khronos/common/khrn_int_misc_impl.h"

#include "interface/khronos/include/GLES/gl.h"
#include "interface/khronos/include/GLES/glext.h"
#include "interface/khronos/glxx/glxx_int_attrib.h"
#include "interface/khronos/include/VG/openvg.h"
#include "interface/khronos/include/VG/vgext.h"
#include "interface/khronos/include/VG/vgu.h"
#include "interface/khronos/vg/vg_int.h"
#include "interface/khronos/vg/vg_int_mat3x3.h"
#include "interface/khronos/include/EGL/egl.h"
#include "interface/khronos/include/EGL/eglext.h"
#include "interface/khronos/common/khrn_int_image.h"
#include "interface/khronos/egl/egl_int.h"

typedef struct {
   KHRONOS_DISPATCH_FUNC *dispatch;
   KHRN_SYNC_MASTER_WAIT_FUNC *sync_master_wait;
   KHRN_SPECIFY_EVENT_FUNC *specify_event;
   KHRN_DO_SUSPEND_RESUME_FUNC *do_suspend_resume;

#define KHRN_IMPL_STRUCT
#include "interface/khronos/glxx/gl11_int_impl.h"
#include "interface/khronos/glxx/gl20_int_impl.h"
#include "interface/khronos/glxx/glxx_int_impl.h"
#include "interface/khronos/vg/vg_int_impl.h"
#include "interface/khronos/egl/egl_int_impl.h"
#include "interface/khronos/common/khrn_int_misc_impl.h"
#undef KHRN_IMPL_STRUCT
} KHRONOS_FUNC_TABLE_T;

typedef const KHRONOS_FUNC_TABLE_T *KHRONOS_GET_FUNC_TABLE_FUNC(void);

#ifdef USE_VCHIQ_ARM
extern void khrn_misc_set_connection_pid(uint32_t pid_0, uint32_t pid_1);
#endif

#endif
