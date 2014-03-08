/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VG_TESS_H
#define VG_TESS_H

#include "interface/khronos/vg/vg_int_config.h"

extern void vg_tess_subd_cubic(float *q, float *r, const float *p, float t);

/* generates up to 4 cubics that approximate the given arc */
extern uint32_t vg_tess_arc_to_cubics(
   float *p,
   float p0_x, float p0_y, float p1_x, float p1_y, float p1_m_p0_x, float p1_m_p0_y,
   float r_x, float r_y, float angle,
   bool large, bool cw);

typedef struct {
   float pattern[VG_CONFIG_MAX_DASH_COUNT]; /* dash points in [0, 1], must be monotonic increasing and last must be 1 */
   uint32_t pattern_count;                  /* must be even */
   float oo_scale;

   float phase;      /* start phase, must be in [0, 1] */
   uint32_t phase_i; /* pattern index corresponding to start phase */
   bool phase_reset; /* reset phase to start phase at the start of each subpath? */
} VG_TESS_STROKE_DASH_T;

extern bool vg_tess_stroke_dash_prepare(VG_TESS_STROKE_DASH_T *dash,
   const float *pattern, uint32_t pattern_n, float phase, bool phase_reset);

extern void vg_tess_clerp_approx(float *x, float *y, float x0, float y0, float x1, float y1, float t);

/* a should have space for at least (a_n + 4) verts. b should have space for at least (a_n + 3) verts */
extern uint32_t vg_tess_poly_clip(float *a, float *b, uint32_t a_n, const float *clip);

#endif
