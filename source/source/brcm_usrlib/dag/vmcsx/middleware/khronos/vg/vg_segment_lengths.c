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
#include "middleware/khronos/vg/vg_segment_lengths.h"
#include "middleware/khronos/vg/vg_tess.h"
#include "interface/khronos/common/khrn_int_math.h"
#include "interface/khronos/common/khrn_int_util.h"

#define LEVEL_MAX 8
#define L_ERROR 1.0e-2f /* percentage error */
#define D_ERROR 5.0e-2f /* percentage error */

static INLINE bool is_neg(float x)
{
   return !!(float_to_bits(x) & (1 << 31));
}

float vg_segment_lengths_get_line_length(float p0_x, float p0_y, float p1_x, float p1_y)
{
   float d_x = p1_x - p0_x;
   float d_y = p1_y - p0_y;
   return sqrt_((d_x * d_x) + (d_y * d_y));
}

static float get_cubic_length(const float *p, uint32_t level)
{
   float l0 = vg_segment_lengths_get_line_length(p[0], p[1], p[6], p[7]);
   float l1 = vg_segment_lengths_get_line_length(p[0], p[1], p[2], p[3]) +
              vg_segment_lengths_get_line_length(p[2], p[3], p[4], p[5]) +
              vg_segment_lengths_get_line_length(p[4], p[5], p[6], p[7]);
   float l = (l0 + l1) * 0.5f;
   float l_error = l1 - l0;
   float q[8], r[8];

   if ((level == LEVEL_MAX) || (l_error <= (l * L_ERROR))) {
      return l;
   }

   vg_tess_subd_cubic(q, r, p, 0.5f);
   return get_cubic_length(q, level + 1) + get_cubic_length(r, level + 1);
}

float vg_segment_lengths_get_cubic_length(const float *p)
{
   return get_cubic_length(p, 0);
}

float vg_segment_lengths_get_t_along_line(float p0_x, float p0_y, float p1_x, float p1_y, float distance)
{
   float d_x = p1_x - p0_x;
   float d_y = p1_y - p0_y;
   float l_sq = (d_x * d_x) + (d_y * d_y);
   return ((distance * distance) < l_sq) ? _minf(distance * rsqrt_(l_sq), 1.0f) : -sqrt_(l_sq);
}

static float get_t_along_cubic(
   const float *p,
   float distance,
   uint32_t level)
{
   float l0 = vg_segment_lengths_get_line_length(p[0], p[1], p[6], p[7]);
   float a = vg_segment_lengths_get_line_length(p[0], p[1], p[2], p[3]);
   float b = vg_segment_lengths_get_line_length(p[2], p[3], p[4], p[5]);
   float c = vg_segment_lengths_get_line_length(p[4], p[5], p[6], p[7]);
   float l1 = a + b + c;
   float l = (l0 + l1) * 0.5f;
   float l_error = l1 - l0;
   float q[8], r[8];
   float t, u;

   if ((level == LEVEL_MAX) || (l_error <= (l * L_ERROR))) {
      float d_error;

      if (distance >= l) {
         return -l;
      }

      d_error = absf_(a - b) + absf_(b - c);
      if ((level == LEVEL_MAX) || (d_error <= (l * D_ERROR))) {
         return _minf(distance * recip_(l), 1.0f);
      }
   }

   vg_tess_subd_cubic(q, r, p, 0.5f);
   t = get_t_along_cubic(q, distance, level + 1);
   if (!is_neg(t)) { return t * 0.5f; }
   u = get_t_along_cubic(r, distance + t, level + 1);
   if (!is_neg(u)) { return 0.5f + (u * 0.5f); }
   return t + u;
}

float vg_segment_lengths_get_t_along_cubic(const float *p, float distance)
{
   return get_t_along_cubic(p, distance, 0);
}

void vg_segment_lengths_get_line_p(float *p, float p0_x, float p0_y, float p1_x, float p1_y, float t)
{
   float omt = 1.0f - t;
   p[0] = (omt * p0_x) + (t * p1_x);
   p[1] = (omt * p0_y) + (t * p1_y);
}

void vg_segment_lengths_get_cubic_p(float *p, const float *q, float t)
{
   float a_x = -q[0] + (3.0f * q[2]) - (3.0f * q[4]) + q[6];
   float a_y = -q[1] + (3.0f * q[3]) - (3.0f * q[5]) + q[7];
   float b_x = (3.0f * q[0]) - (6.0f * q[2]) + (3.0f * q[4]);
   float b_y = (3.0f * q[1]) - (6.0f * q[3]) + (3.0f * q[5]);
   float c_x = -(3.0f * q[0]) + (3.0f * q[2]);
   float c_y = -(3.0f * q[1]) + (3.0f * q[3]);
   float d_x = q[0];
   float d_y = q[1];
   p[0] = (((((a_x * t) + b_x) * t) + c_x) * t) + d_x;
   p[1] = (((((a_y * t) + b_y) * t) + c_y) * t) + d_y;
}

bool vg_segment_lengths_get_line_t(float *t, float p0_x, float p0_y, float p1_x, float p1_y)
{
   float d_x = p1_x - p0_x;
   float d_y = p1_y - p0_y;
   float hyp_sq = (d_x * d_x) + (d_y * d_y);
   float oo_hyp;
   if (hyp_sq < EPS) {
      return false;
   }
   oo_hyp = rsqrt_(hyp_sq);
   t[0] = d_x * oo_hyp;
   t[1] = d_y * oo_hyp;
   return true;
}

bool vg_segment_lengths_get_cubic_t(float *t, const float *p, float u)
{
   float q[8], r[8];
   uint32_t i;

   vg_tess_subd_cubic(q, r, p, u);

   for (i = 0; i != 6; i += 2) {
      float q_d_x = q[6] - q[4 - i];
      float q_d_y = q[7] - q[5 - i];
      float q_hyp_sq = (q_d_x * q_d_x) + (q_d_y * q_d_y);
      float r_d_x = r[2 + i] - r[0];
      float r_d_y = r[3 + i] - r[1];
      float r_hyp_sq = (r_d_x * r_d_x) + (r_d_y * r_d_y);
      if (_maxf(q_hyp_sq, r_hyp_sq) < EPS) {
         continue;
      }
      if (q_hyp_sq > r_hyp_sq) {
         float oo_hyp = rsqrt_(q_hyp_sq);
         t[0] = q_d_x * oo_hyp;
         t[1] = q_d_y * oo_hyp;
      } else {
         float oo_hyp = rsqrt_(r_hyp_sq);
         t[0] = r_d_x * oo_hyp;
         t[1] = r_d_y * oo_hyp;
      }
      return true;
   }
   return false;
}
