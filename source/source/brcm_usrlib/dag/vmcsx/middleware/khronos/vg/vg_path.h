/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VG_PATH_H
#define VG_PATH_H

#include "middleware/khronos/common/khrn_mem.h"
#include "interface/khronos/include/VG/openvg.h"
#ifdef __VIDEOCORE4__
#include "middleware/khronos/vg/2708/vg_path_filler_4.h" /* should define VG_PATH_EXTRA_T */
#else
#include "middleware/khronos/vg/2707b/vg_path_filler_3.h" /* should define VG_PATH_EXTRA_T */
#endif

typedef struct {
   int32_t format;
   VGPathDatatype datatype;
   uint32_t caps;
   float scale;
   float bias;
   int32_t segments_capacity;
   int32_t coords_capacity;
} VG_PATH_BPRINT_T;

extern void vg_path_bprint_from_stem(
   MEM_HANDLE_T handle,
   int32_t format,
   VGPathDatatype datatype,
   uint32_t caps,
   float scale, float bias,
   int32_t segments_capacity,
   int32_t coords_capacity);

extern void vg_path_bprint_term(void *, uint32_t);

static INLINE bool vg_is_path_bprint(MEM_HANDLE_T handle)
{
   return mem_get_term(handle) == vg_path_bprint_term;
}

#define VG_PATH_FLAG_EMPTY (1 << 0) /* segments/coords would generate an empty tessellation. updated after initialisation by platform-specific code only */

typedef struct {
   uint8_t flags;

   uint8_t datatype;
   uint16_t caps;

   float scale;
   float bias;

   MEM_HANDLE_T segments; /* top 3 bits of each byte are used for storing tess info */
   MEM_HANDLE_T coords;

   /*
      updated after initialisation by platform-specific code only
   */

   float bounds[4];

   VG_PATH_EXTRA_T extra; /* entirely platform-specific */
} VG_PATH_T;

extern bool vg_path_from_bprint(MEM_HANDLE_T handle);

extern void vg_path_term(void *, uint32_t);

static INLINE bool vg_is_path(MEM_HANDLE_T handle)
{
   return mem_get_term(handle) == vg_path_term;
}

/* helpers for backends */
extern VGErrorCode vg_path_interpolate_slow(VG_PATH_T *dst, VG_PATH_T *begin, VG_PATH_T *end, float t);
extern bool vg_path_is_empty(MEM_HANDLE_T segments_handle);

/******************************************************************************
platform-specific implementations
******************************************************************************/

extern void vg_path_extra_init(VG_PATH_T *path);
extern void vg_path_extra_term(VG_PATH_T *path);

/* these may be more efficient than vg_path_read_immediate(path, VG_PATH_RW_*) followed by mem_get_size(path->*) */
extern uint32_t vg_path_get_segments_size(VG_PATH_T *path);
extern uint32_t vg_path_get_coords_size(VG_PATH_T *path);

typedef enum {
   VG_PATH_RW_SEGMENTS = 1 << 0,
   VG_PATH_RW_COORDS   = 1 << 1,
   VG_PATH_RW_DATA     = VG_PATH_RW_SEGMENTS | VG_PATH_RW_COORDS
} VG_PATH_RW_T;

extern bool vg_path_read_immediate(VG_PATH_T *path, VG_PATH_RW_T rw);
extern bool vg_path_write_immediate(VG_PATH_T *path, VG_PATH_RW_T rw);

extern bool vg_path_are_bounds_valid(VG_PATH_T *path);
extern bool vg_path_read_tight_bounds_immediate(MEM_HANDLE_T handle, VG_PATH_T *path);

extern bool vg_path_clear(VG_PATH_T *path);
extern VGErrorCode vg_path_interpolate(
   MEM_HANDLE_T dst_handle, VG_PATH_T *dst,
   MEM_HANDLE_T begin_handle, VG_PATH_T *begin,
   MEM_HANDLE_T end_handle, VG_PATH_T *end,
   float t);

extern bool vg_path_get_length(MEM_HANDLE_T handle, VG_PATH_T *path, float *length, uint32_t segments_i, uint32_t segments_count);
extern bool vg_path_get_p_t_along(MEM_HANDLE_T handle, VG_PATH_T *path, float *p, float *t, uint32_t segments_i, uint32_t segments_count, float distance);

typedef enum {
   VG_PATH_ELEMENT_CAPS = 1 << 0,
   VG_PATH_ELEMENT_DATA = 1 << 1
} VG_PATH_ELEMENT_T;

extern void vg_path_changed(VG_PATH_T *path, VG_PATH_ELEMENT_T elements);

#endif
