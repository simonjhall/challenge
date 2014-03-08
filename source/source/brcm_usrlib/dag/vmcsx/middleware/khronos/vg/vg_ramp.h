/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VG_RAMP_H
#define VG_RAMP_H

#include "middleware/khronos/common/khrn_mem.h"
#include "interface/khronos/include/VG/openvg.h"
#ifdef __VIDEOCORE4__
#include "middleware/khronos/vg/2708/vg_ramp_filler_4.h" /* should #define VG_RAMP_ALIGN */
#else
#include "middleware/khronos/vg/2707b/vg_ramp_filler_3.h" /* should #define VG_RAMP_ALIGN */
#endif

#define VG_RAMP_HEIGHT 4 /* utile height, so no wasted space with lineartile */
#define VG_RAMP_WIDTH 256
#define VG_RAMP_SIZE (VG_RAMP_HEIGHT * VG_RAMP_WIDTH * sizeof(uint32_t))
#define VG_RAMP_FORMAT ABGR_8888_LT

typedef struct {
   uint32_t ref_counts[VG_RAMP_HEIGHT];
   bool valids[VG_RAMP_HEIGHT]; /* slot i is valid iff ((mem_get_size(data) != 0) && valids[i]) */

   MEM_HANDLE_T data;

   MEM_HANDLE_T free_prev;
   MEM_HANDLE_T free_next;
} VG_RAMP_T;

extern bool vg_ramp_alloc(
   MEM_HANDLE_T *handle,
   uint32_t *i);

extern void vg_ramp_acquire(
   MEM_HANDLE_T handle,
   uint32_t i);
extern void vg_ramp_release(
   MEM_HANDLE_T handle,
   uint32_t i);

extern uint32_t vg_ramp_get_rgba(
   MEM_HANDLE_T stops_handle,
   bool last); /* else first */

extern bool vg_ramp_retain(
   MEM_HANDLE_T handle,
   uint32_t i,
   bool pre,
   MEM_HANDLE_T stops_handle);
extern void vg_ramp_unretain(
   MEM_HANDLE_T handle);

#endif
