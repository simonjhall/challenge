/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "interface/khronos/common/khrn_int_common.h"
#include "interface/khronos/vg/vg_int_mat3x3.h"
#include "middleware/khronos/vg/vg_tess.h"
#include "interface/khronos/common/khrn_int_math.h"
#include "interface/khronos/common/khrn_int_util.h"

/******************************************************************************
cubic subdivision
******************************************************************************/

void vg_tess_subd_cubic(float *q, float *r, const float *p, float t)
{
   float p0p_x = p[0] + (t * (p[2] - p[0]));
   float p0p_y = p[1] + (t * (p[3] - p[1]));
   float p1p_x = p[2] + (t * (p[4] - p[2]));
   float p1p_y = p[3] + (t * (p[5] - p[3]));
   float p2p_x = p[4] + (t * (p[6] - p[4]));
   float p2p_y = p[5] + (t * (p[7] - p[5]));
   float p0pp_x = p0p_x + (t * (p1p_x - p0p_x));
   float p0pp_y = p0p_y + (t * (p1p_y - p0p_y));
   float p1pp_x = p1p_x + (t * (p2p_x - p1p_x));
   float p1pp_y = p1p_y + (t * (p2p_y - p1p_y));
   float p0ppp_x = p0pp_x + (t * (p1pp_x - p0pp_x));
   float p0ppp_y = p0pp_y + (t * (p1pp_y - p0pp_y));

   if (q) {
      q[0] = p[0];     q[1] = p[1];
      q[2] = p0p_x;    q[3] = p0p_y;
      q[4] = p0pp_x;   q[5] = p0pp_y;
      q[6] = p0ppp_x;  q[7] = p0ppp_y;
   }

   if (r) {
      r[0] = p0ppp_x;  r[1] = p0ppp_y;
      r[2] = p1pp_x;   r[3] = p1pp_y;
      r[4] = p2p_x;    r[5] = p2p_y;
      r[6] = p[6];     r[7] = p[7];
   }
}

/******************************************************************************
arc to cubics
******************************************************************************/

#define ARC_MAGIC 0.5522847f
static const float ARC_P[4 * 8] = {
   1.0f, 0.0f,   1.0f, ARC_MAGIC,    ARC_MAGIC, 1.0f,    0.0f, 1.0f,   /* 0 to pi/2 */
   0.0f, 1.0f,   -ARC_MAGIC, 1.0f,   -1.0f, ARC_MAGIC,   -1.0f, 0.0f,  /* pi/2 to pi */
   -1.0f, 0.0f,  -1.0f, -ARC_MAGIC,  -ARC_MAGIC, -1.0f,  0.0f, -1.0f,  /* pi to 3pi/2 */
   0.0f, -1.0f,  ARC_MAGIC, -1.0f,   1.0f, -ARC_MAGIC,   1.0f, 0.0f }; /* 3pi/2 to 2pi */

#define ARC_EPS 1.0e-5f

static uint32_t unit_arc_to_cubics(float *p, float angle, bool cw)
{
   const float *arc_p;
   uint32_t quadrant;

   /* [-2pi, 2pi] to [0, 2pi] */
   if (cw) { angle = -angle; } /* ys flipped later */
   if (angle < 0.0f) { angle += 2.0f * PI; } /* [-2pi, 0] to [0, 2pi] */

   arc_p = ARC_P;
   for (quadrant = 0; quadrant != 4; ++quadrant, p += 8, arc_p += 8) {
      uint32_t i;
      for (i = 0; i != 8; i += 2) {
         p[i] = arc_p[i];
         p[i + 1] = cw ? -arc_p[i + 1] : arc_p[i + 1]; /* flip ys if cw (angle flipped earlier) */
      }
      if (angle < (PI * 0.5f)) {
         vg_tess_subd_cubic(p, NULL, p, angle * (2.0f / PI));
         return quadrant + 1;
      }
      angle -= PI * 0.5f;
      if (angle < ARC_EPS) {
         return quadrant + 1;
      }
   }
   return 4;
}

static void ellipse_to_unit_circle(
   float *u_x, float *u_y,
   float x, float y,
   float r_x, float r_y, float angle)
{
   /*
      rotate cw by angle, then scale by 1 / r
   */

   float s, c;
   sin_cos_(&s, &c, angle);
   *u_x = ((c * x) + (s * y)) * recip_(r_x);
   *u_y = ((-s * x) + (c * y)) * recip_(r_y);
}

static void line_to_cubic(
   float *p,
   float p0_x, float p0_y, float p1_x, float p1_y)
{
   p[0] = p0_x;  p[1] = p0_y;
   p[2] = p0_x;  p[3] = p0_y;
   p[4] = p1_x;  p[5] = p1_y;
   p[6] = p1_x;  p[7] = p1_y;
}

uint32_t vg_tess_arc_to_cubics(
   float *p,
   float p0_x, float p0_y, float p1_x, float p1_y, float p1_m_p0_x, float p1_m_p0_y,
   float r_x, float r_y, float angle,
   bool large, bool cw)
{
   float u1_x, u1_y;
   float c_x, c_y; /* center */
   float d_sq, s_sq;
   float u0_angle, u1_angle;
   uint32_t count;
   /* coverity [uninit] : Coverity doesn't follow initialisation in vg_mat3x3_set_identity() */
   VG_MAT3X3_T a;
   uint32_t i;

   if ((r_x < EPS) || (r_y < EPS)) {
      /* degenerate ellipse, approximate as line */
      line_to_cubic(p, p0_x, p0_y, p1_x, p1_y);
      return 1;
   }

   /*
      transform p0/p1 - p0 on ellipse - p0 to u0/u1 on unit circle with center c
   */

   /* u0 = (0, 0) */
   ellipse_to_unit_circle(&u1_x, &u1_y, p1_m_p0_x, p1_m_p0_y, r_x, r_y, angle);

   /*
      d = chord length = length(u1 - u0)
      l = distance from middle of chord to c = sqrt(1 - (d/2)^2)
      s = l/d = sqrt(1/d^2 - 1/4)
   */

   d_sq = (u1_x * u1_x) + (u1_y * u1_y);
   if (d_sq < EPS) {
      /* points almost coincident, approximate as line */
      line_to_cubic(p, p0_x, p0_y, p1_x, p1_y);
      return 1;
   }
   s_sq = recip_(d_sq) - 0.25f;
   if (s_sq < 0.0f) {
      /*
         chord length >= 2, points are too far apart for both to be able to lie on ellipse
         spec says should uniformly scale ellipse so that it is just big enough
         this will result in a chord of length 2, with c = (u0 + u1)/2
         scaling ellipse by s will cause chord length to scale by 1/s
         want chord length = 2, so want to scale chord length by 2/d, so want to scale ellipse by d/2
      */

      float scale = 0.5f * sqrt_(d_sq);
      r_x *= scale;
      r_y *= scale;
      ellipse_to_unit_circle(&u1_x, &u1_y, p1_m_p0_x, p1_m_p0_y, r_x, r_y, angle);
      c_x = u1_x * 0.5f;
      c_y = u1_y * 0.5f;
   } else {
      float s;

      /*
         moving a distance l perpendicular to chord from middle of chord m will get us to the two possible centers
         which center to use depends on large/cw
      */

      c_x = u1_x * 0.5f;
      c_y = u1_y * 0.5f;
      s = sqrt_(s_sq);
      if (large ^ cw) {
         c_x += s * u1_y;
         c_y -= s * u1_x;
      } else {
         c_x -= s * u1_y;
         c_y += s * u1_x;
      }
   }

   /*
      approximate arc along unit circle
   */

   u0_angle = atan2_(-c_y, -c_x);
   u1_angle = atan2_(u1_y - c_y, u1_x - c_x);
   count = unit_arc_to_cubics(p, u1_angle - u0_angle, cw);
   vcos_assert((count != 0) && (count <= 4));

   /*
      transform cubics to ellipse

      rotate ccw by u0_angle
      translate by c
      scale by r
      rotate ccw by angle
      translate by p0
   */

   vg_mat3x3_set_identity(&a);
   vg_mat3x3_premul_rotate(&a, u0_angle);
   vg_mat3x3_premul_translate(&a, c_x, c_y);
   vg_mat3x3_premul_scale(&a, r_x, r_y);
   vg_mat3x3_premul_rotate(&a, angle);
   vg_mat3x3_premul_translate(&a, p0_x, p0_y);
#ifndef NDEBUG
   vcos_verify(vg_mat3x3_is_affine(&a)); /* why vcos_verify? see vg_mat3x3_is_affine... */
#endif

   for (i = 0; i != (count * 8); i += 2) {
      vg_mat3x3_affine_transform(&a, p + i, p + i + 1);
   }

   /*
      force first and last points to coincide exactly with p0/p1
   */

   p[0] = p0_x;
   p[1] = p0_y;
   p[(count * 8) - 2] = p1_x;
   p[(count * 8) - 1] = p1_y;

   return count;
}

/******************************************************************************
dash prep
******************************************************************************/

bool vg_tess_stroke_dash_prepare(VG_TESS_STROKE_DASH_T *dash,
   const float *pattern, uint32_t pattern_n, float phase, bool phase_reset)
{
   float sum;
   uint32_t i;
   float oo_scale;
   uint32_t phase_i;

   /*
      find length of dash pattern
   */

   sum = 0.0f;
   for (i = 0; i != pattern_n; ++i) {
      sum += pattern[i];
   }

   if (sum < EPS) { return false; } /* no dashing */

   oo_scale = recip_(sum);

   /*
      starting phase
   */

   phase = mod_one_(phase * oo_scale);
   if (phase < 0.0f) { phase += 1.0f; }
   vcos_assert((phase >= 0.0f) && (phase <= 1.0f));

   /*
      convert dash pattern and find starting index
   */

   phase_i = 0;
   sum = 0.0f;
   for (i = 0; i != (pattern_n - 1); ++i) {
      sum = _minf(sum + (pattern[i] * oo_scale), 1.0f);
      dash->pattern[i] = sum;
      if (phase >= sum) {
         phase_i = i + 1;
      }
   }

   dash->pattern[pattern_n - 1] = 1.0f;
   dash->pattern_count = pattern_n;
   dash->oo_scale = oo_scale;
   dash->phase = phase;
   dash->phase_i = phase_i;
   dash->phase_reset = phase_reset;

   return true;
}

/******************************************************************************
clerp approximation
******************************************************************************/

void vg_tess_clerp_approx(float *x, float *y, float x0, float y0, float x1, float y1, float t)
{
   float c = (x0 * x1) + (y0 * y1), s;
   t = t * t; /* because cos looks like a quadratic around x = 0 (ie this approximation is better when the angle is smaller) */
   c = (1.0f - t) + (t * c);
   s = sqrt_(_maxf(1.0f - (c * c), 0.0f)); /* 1 - c^2 should be >= 0, but we might end up slightly less than 0 due to fp lameness... */
   if ((x0 * y1) < (y0 * x1)) { s = -s; }
   *x = (x0 * c) - (y0 * s);
   *y = (y0 * c) + (x0 * s);
}

/******************************************************************************
polygon clipper
******************************************************************************/

/* return value either 0 or >= 3 and <= (a_n + 4) */
uint32_t vg_tess_poly_clip(float *a, float *b, uint32_t a_n, const float *clip)
{
   /* maths says the maximum number of vertices we can generate is a_n + 4 (the
    * polygon is convex, so when clipping against an edge, we either do nothing
    * or add 2 vertices and take away at least one), but with floating point,
    * who knows? the caller is only prepared to deal with (in particular, will
    * only have allocated enough space at a and b for) a_n + 4 vertices, so make
    * sure we don't generate more than that... */
   uint32_t b_n_max = (a_n + 4) << 1;

   uint32_t i;
   for (i = 0; i != 4; ++i) {
      float e = clip[i];
      uint32_t b_n = 0, j;
      float *temp;

      for (j = 0; j != (a_n * 2); j += 2) {
         float p0_x = a[j];
         float p0_y = a[j + 1];
         float v0 = (i & 0x1) ? p0_y : p0_x;
         bool in0 = (i & 0x2) ? (v0 <= e) : (v0 >= e);

         uint32_t k = ((j + 2) == (a_n * 2)) ? 0 : (j + 2);
         float p1_x = a[k];
         float p1_y = a[k + 1];
         float v1 = (i & 0x1) ? p1_y : p1_x;
         bool in1 = (i & 0x2) ? (v1 <= e) : (v1 >= e);

         if (in0) {
            if (b_n == b_n_max) { return 0; }
            b[b_n++] = p0_x;
            b[b_n++] = p0_y;
         }

         /*
            must do calculation in consistent direction to avoid gaps between tris
         */

         if (in0 && !in1) {
            float t, omt;
            if (b_n == b_n_max) { return 0; }
            t = _minf((e - v0) * recip_(v1 - v0), 1.0f);
            omt = 1.0f - t;
            b[b_n++] = (i & 0x1) ? ((omt * p0_x) + (t * p1_x)) : e;
            b[b_n++] = (i & 0x1) ? e : ((omt * p0_y) + (t * p1_y));
         }

         if (!in0 && in1) {
            float t, omt;
            if (b_n == b_n_max) { return 0; }
            t = _minf((e - v1) * recip_(v0 - v1), 1.0f);
            omt = 1.0f - t;
            b[b_n++] = (i & 0x1) ? ((omt * p1_x) + (t * p0_x)) : e;
            b[b_n++] = (i & 0x1) ? e : ((omt * p1_y) + (t * p0_y));
         }
      }

      temp = a;
      a = b;
      b = temp;

      a_n = b_n >> 1;
   }

   return a_n;
}
