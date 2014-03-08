/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VG_TESS_PRIV_4_H
#define VG_TESS_PRIV_4_H

#include "interface/khronos/vg/vg_int_mat3x3.h"
#include "middleware/khronos/common/2708/khrn_fmem_4.h"
#include "middleware/khronos/vg/vg_tess.h"
#include "interface/khronos/include/VG/openvg.h"

/******************************************************************************
simple geometry
******************************************************************************/

#ifdef WORKAROUND_HW2187
extern bool vg_tess_dummy(KHRN_FMEM_T *fmem, uint32_t offset);
#endif

extern bool vg_tess_rect(KHRN_FMEM_T *fmem,
   uint32_t x, uint32_t y, uint32_t width, uint32_t height);

typedef struct {
   uint32_t n;
   uint32_t p[8];
} VG_TESS_QUAD_REP_T;

extern bool vg_tess_quad(KHRN_FMEM_T *fmem,
   const float *p, const float *clip,
   VG_TESS_QUAD_REP_T *quad_rep);
extern bool vg_tess_quad_rep(KHRN_FMEM_T *fmem,
   VG_TESS_QUAD_REP_T *quad_rep);

/******************************************************************************
precomputed tessellation
******************************************************************************/

/*
   VG_TESS_CHUNK_NORMAL_T/VG_TESS_CHUNK_CUBIC_T are intended to be easy to load
   from the qpus

   provided a and b do not contain arcs:
   interpolate_xys(vg_tess_gen(a), vg_tess_gen(b), t) == vg_tess_gen(interpolate_paths(a, b, t))
   this makes it possible to efficiently handle vgInterpolatePath followed by
   vgDrawPath -- vgInterpolatePath can just remember the 2 source paths, and
   vgDrawPath can perform the interpolation as the chunks are loaded
*/

typedef struct {
   uint8_t n; /* != 0 */
   bool subpath_last_implicit; /* is last vertex also last of subpath? */
   uint16_t dummy;
   float xs[7];
   float ys[7];
   bool subpath_first; /* is first vertex also first of subpath? */
   bool subpath_last; /* is last vertex also last (explicit) of subpath? */
   bool close; /* applies to subpath last vertex */
   uint8_t explicit_n; /* may be 0! */
} VG_TESS_CHUNK_NORMAL_T;

vcos_static_assert(sizeof(VG_TESS_CHUNK_NORMAL_T) == 64);

/* this is a bit hacky. we can walk backwards from p by looking at:
 * ((VG_TESS_CHUNK_CUBIC_T *)p - 1)->magic
 * iff this value is -1, then the previous chunk is a cubic. this works because
 * the value lies where the subpath_first/subpath_last/close/explicit_n fields
 * would lie in a normal chunk, and they can't ever take on the value -1 */
#define VG_TESS_CHUNK_CUBIC_MAGIC -1

typedef struct {
   uint8_t n; /* = 0 */
   bool subpath_first; /* is first vertex of cubic also first of subpath? */
   bool subpath_last; /* is last vertex of cubic also last (explicit) of subpath? */
   uint8_t dummy;
   float p[8];
   uint32_t magic; /* should be set to VG_TESS_CHUNK_CUBIC_MAGIC */
} VG_TESS_CHUNK_CUBIC_T;

vcos_static_assert(sizeof(VG_TESS_CHUNK_CUBIC_T) == 40);

extern MEM_HANDLE_T vg_tess_alloc(void);

extern bool vg_tess_retain(
   MEM_HANDLE_T tess_handle, float *bounds,
   VGPathDatatype datatype, float scale, float bias,
   MEM_HANDLE_T segments_handle, MEM_HANDLE_T coords_handle);

/******************************************************************************
fill/stroke tessellation
******************************************************************************/

extern void *vg_tess_init(uint32_t size);
extern void vg_tess_term(void);

typedef enum {
   VG_TESS_HANDLE_INVALID = 0,
   VG_TESS_HANDLE_FORCE_32BIT = (int)0x80000000
} VG_TESS_HANDLE_T;

extern VG_TESS_HANDLE_T vg_tess_start(void); /* returning VG_TESS_HANDLE_INVALID indicates failure */
extern void vg_tess_finish_llat(VG_TESS_HANDLE_T tess_handle); /* must have called vg_tess_wait (and waited for the callback) before calling. must be called from the llat task */
extern void vg_tess_finish(VG_TESS_HANDLE_T tess_handle); /* must have called vg_tess_finish_llat before calling */

extern void vg_tess_nmem_enter(VG_TESS_HANDLE_T tess_handle); /* call "just before" calling khrn_nmem_enter */
extern void vg_tess_wait(VG_TESS_HANDLE_T tess_handle, void (*callback)(bool, void *), void *p);
extern void vg_tess_fix(VG_TESS_HANDLE_T tess_handle); /* must have called vg_tess_wait (and waited for the callback to indicate success) before calling */

extern uint32_t vg_tess_get_nmem_n(VG_TESS_HANDLE_T tess_handle);

/*
   general rules for vg_tess_fill* and vg_tess_stroke*:
   - bin_handle/bin_locked must only be used over the duration of the call.
     handles from allocations may be remembered, but will not be fixed in place
     until vg_tess_fix is called and must not be used after vg_tess_fix returns
   - if chunks_a_handle/chunks_b_handle are remembered for later use, they must
     be both mem_retained and mem_acquired
*/

extern void *vg_tess_fill(VG_TESS_HANDLE_T tess_handle,
   KHRN_FMEM_T *fmem,
   MEM_HANDLE_T chunks_a_handle, MEM_HANDLE_T chunks_b_handle, float t,
   const VG_MAT3X3_T *user_to_surface,
   const float *clip); /* returning NULL indicates failure */

extern bool vg_tess_fill_rep(VG_TESS_HANDLE_T tess_handle,
   KHRN_FMEM_T *fmem,
   void *fill_rep); /* returning false indicates failure */

extern bool vg_tess_fill_bbox(VG_TESS_HANDLE_T tess_handle,
   KHRN_FMEM_T *fmem,
   void *fill_rep); /* returning false indicates failure */

extern void *vg_tess_stroke(VG_TESS_HANDLE_T tess_handle,
   KHRN_FMEM_T *fmem,
   MEM_HANDLE_T chunks_a_handle, MEM_HANDLE_T chunks_b_handle, float t,
   const float *dash_pattern, uint32_t dash_pattern_n, float dash_phase, bool dash_phase_reset,
   float line_width, VGCapStyle cap_style, VGJoinStyle join_style, float miter_limit,
   const VG_MAT3X3_T *user_to_surface, float scale_max,
   const float *clip_inner, const float *clip_outer); /* returning NULL indicates failure */

extern bool vg_tess_stroke_rep(VG_TESS_HANDLE_T tess_handle,
   KHRN_FMEM_T *fmem,
   void *stroke_rep); /* returning false indicates failure */

/* extra storage for user-defined stuff */
extern void vg_tess_rep_set_user(void *rep, float a, float b, float c, float d);
extern void vg_tess_rep_get_user(void *rep, float *a, float *b, float *c, float *d);

#endif
