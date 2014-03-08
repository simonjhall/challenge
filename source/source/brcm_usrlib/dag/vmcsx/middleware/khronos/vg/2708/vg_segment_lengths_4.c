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
#include "middleware/khronos/vg/2708/vg_segment_lengths_4.h"
#include "middleware/khronos/vg/2708/vg_tess_priv_4.h"

/******************************************************************************
helpers
******************************************************************************/

static INLINE bool is_neg(float x)
{
   return !!(float_to_bits(x) & (1 << 31));
}

static bool is_contributing_normal(const void *chunk, uint32_t i)
{
   const VG_TESS_CHUNK_NORMAL_T *c = (const VG_TESS_CHUNK_NORMAL_T *)chunk;
   vcos_assert(c->n != 0);
   return ((i != 0) || !c->subpath_first) && (i < c->explicit_n);
}

static bool is_contributing(const void *chunk, uint32_t i)
{
   return (*(const uint8_t *)chunk == 0) || is_contributing_normal(chunk, i);
}

static INLINE void back(const void **chunk, uint32_t *i)
{
   if (*i == 0) {
      if (((const VG_TESS_CHUNK_CUBIC_T *)*chunk - 1)->magic == VG_TESS_CHUNK_CUBIC_MAGIC) {
         const VG_TESS_CHUNK_CUBIC_T *c = (const VG_TESS_CHUNK_CUBIC_T *)*chunk - 1;
         vcos_assert(c->n == 0);
         *chunk = c;
      } else {
         const VG_TESS_CHUNK_NORMAL_T *c = (const VG_TESS_CHUNK_NORMAL_T *)*chunk - 1;
         vcos_assert(c->n != 0);
         *chunk = c;
         *i = c->n - 1;
      }
   } else {
      vcos_assert(*(const uint8_t *)*chunk != 0);
      --*i;
   }
}

static INLINE void forward_normal(const void **chunk, uint32_t *i)
{
   const VG_TESS_CHUNK_NORMAL_T *c = (const VG_TESS_CHUNK_NORMAL_T *)*chunk;
   vcos_assert(c->n != 0);
   if (++*i == c->n) {
      *chunk = c + 1;
      *i = 0;
   }
}

static INLINE void forward_cubic(const void **chunk, uint32_t *i)
{
   const VG_TESS_CHUNK_CUBIC_T *c = (const VG_TESS_CHUNK_CUBIC_T *)*chunk;
   vcos_assert(c->n == 0);
   vcos_assert(*i == 0);
   *chunk = c + 1;
}

static INLINE void forward(const void **chunk, uint32_t *i)
{
   if (*(const uint8_t *)*chunk == 0) {
      forward_cubic(chunk, i);
   } else {
      forward_normal(chunk, i);
   }
}

static INLINE void forward_segment(const void **chunk, uint32_t *i, uint32_t segment)
{
   if ((segment & VG_SEGMENT_MASK) >= VG_QUAD_TO) {
      uint32_t cubics_n = segment >> 5;
      for (; cubics_n != 0; --cubics_n) {
         forward_cubic(chunk, i);
      }
   } else {
      uint32_t normals_n = segment >> 6;
      for (; normals_n != 0; --normals_n) {
         forward_normal(chunk, i);
      }
   }
}

static void get_prev_p(
   float *prev_x, float *prev_y,
   const void *chunk, uint32_t i)
{
   back(&chunk, &i);
   if (*(const uint8_t *)chunk == 0) {
      const VG_TESS_CHUNK_CUBIC_T *c = (const VG_TESS_CHUNK_CUBIC_T *)chunk;
      *prev_x = c->p[6];
      *prev_y = c->p[7];
   } else {
      const VG_TESS_CHUNK_NORMAL_T *c = (const VG_TESS_CHUNK_NORMAL_T *)chunk;
      *prev_x = c->xs[i];
      *prev_y = c->ys[i];
   }
}

static float get_segment_length(const void *chunk, uint32_t i, uint32_t segment,
   const void **chunk_out, uint32_t *i_out)
{
   float length = 0.0f;
   if ((segment & VG_SEGMENT_MASK) >= VG_QUAD_TO) {
      uint32_t cubics_n = segment >> 5;
      for (; cubics_n != 0; --cubics_n) {
         length += vg_segment_lengths_get_cubic_length(((const VG_TESS_CHUNK_CUBIC_T *)chunk)->p);
         forward_cubic(&chunk, &i);
      }
   } else {
      uint32_t normals_n = segment >> 6;
      for (; normals_n != 0; --normals_n) {
         if (is_contributing_normal(chunk, i)) {
            float prev_x, prev_y;
            const VG_TESS_CHUNK_NORMAL_T *c;
            get_prev_p(&prev_x, &prev_y, chunk, i);
            c = (const VG_TESS_CHUNK_NORMAL_T *)chunk;
            length += vg_segment_lengths_get_line_length(prev_x, prev_y, c->xs[i], c->ys[i]);
         }
         forward_normal(&chunk, &i);
      }
   }
   *chunk_out = chunk;
   *i_out = i;
   return length;
}

/******************************************************************************
path length
******************************************************************************/

float vg_segment_lengths_get_length(
   const uint8_t *segments, uint32_t segments_i, uint32_t segments_count,
   const void *chunks)
{
   float length = 0.0f;
   const void *chunk = chunks;
   uint32_t i = 0;
   for (; segments_i != 0; ++segments, --segments_i) {
      forward_segment(&chunk, &i, *segments);
   }
   for (; segments_count != 0; ++segments, --segments_count) {
      length += get_segment_length(chunk, i, *segments, &chunk, &i);
   }
   return length;
}

/******************************************************************************
point/tangent along path
******************************************************************************/

void vg_segment_lengths_get_p_t_along(
   float *p, float *t,
   const uint8_t *segments, uint32_t segments_i, uint32_t segments_count,
   const void *chunks,
   float distance)
{
   uint32_t segment;
   const void *begin_chunk, *chunk, *end_chunk;
   uint32_t begin_i, i, end_i;
   float u;

   vcos_assert(distance >= 0.0f);

   /*
      drop move segments at start/end and handle no segments case
   */

   for (; (segments_count != 0) && ((segments[segments_i] & VG_SEGMENT_MASK) == VG_MOVE_TO); ++segments_i, --segments_count) ;
   for (; (segments_count != 0) && ((segments[segments_i + (segments_count - 1)] & VG_SEGMENT_MASK) == VG_MOVE_TO); --segments_count) ;

   if (segments_count == 0) {
      if (p) { p[0] = 0.0f; p[1] = 0.0f; }
      if (t) { t[0] = 1.0f, t[1] = 0.0f; }
      return;
   }

   /*
      find tess range and segment containing point
   */

   begin_chunk = chunks;
   begin_i = 0;
   for (; segments_i != 0; ++segments, --segments_i) {
      forward_segment(&begin_chunk, &begin_i, *segments);
   }

   chunk = begin_chunk;
   i = begin_i;
   for (;; ++segments, --segments_count) {
      const void *next_chunk;
      uint32_t next_i;
      float l;

      segment = *segments;

      if (segments_count == 1) { break; }

      l = get_segment_length(chunk, i, segment, &next_chunk, &next_i);
      if (distance < l) { break; }
      distance -= l;
      chunk = next_chunk;
      i = next_i;
   }

   end_chunk = chunk;
   end_i = i;
   for (; segments_count != 0; ++segments, --segments_count) {
      forward_segment(&end_chunk, &end_i, *segments);
   }

   /*
      find cubic/line containing point
   */

   u = float_from_bits(0x80000000); /* how far along the cubic/line is the point? in [0, 1], negative means not found */

   if ((segment & VG_SEGMENT_MASK) >= VG_QUAD_TO) {
      uint32_t cubics_n = segment >> 5;
      for (; cubics_n != 0; --cubics_n) {
         u = vg_segment_lengths_get_t_along_cubic(((const VG_TESS_CHUNK_CUBIC_T *)chunk)->p, distance);
         if (!is_neg(u)) { break; }
         distance += u;
         forward_cubic(&chunk, &i);
      }
   } else {
      uint32_t normals_n = segment >> 6;
      for (; normals_n != 0; --normals_n) {
         if (is_contributing_normal(chunk, i)) {
            float prev_x, prev_y;
            const VG_TESS_CHUNK_NORMAL_T *c;
            get_prev_p(&prev_x, &prev_y, chunk, i);
            c = (const VG_TESS_CHUNK_NORMAL_T *)chunk;
            u = vg_segment_lengths_get_t_along_line(prev_x, prev_y, c->xs[i], c->ys[i], distance);
            if (!is_neg(u)) { break; }
            distance += u;
         }
         forward_normal(&chunk, &i);
      }
   }

   if (is_neg(u)) {
      /*
         if we get here, we must have some non-move segments, so there must be
         at least one point for the search to find
      */

      bool search_forward = (chunk == chunks) && (i == 0);
      u = search_forward ? 0.0f : 1.0f;
      for (;;) {
         if (!search_forward) { back(&chunk, &i); }
         if (is_contributing(chunk, i)) { break; }
         if (search_forward) { forward(&chunk, &i); }
      }
   }

   /*
      get position/tangent at point
   */

   if (p) {
      if (*(const uint8_t *)chunk == 0) {
         vg_segment_lengths_get_cubic_p(p, ((const VG_TESS_CHUNK_CUBIC_T *)chunk)->p, u);
      } else {
         float prev_x, prev_y;
         const VG_TESS_CHUNK_NORMAL_T *c;
         get_prev_p(&prev_x, &prev_y, chunk, i);
         c = (const VG_TESS_CHUNK_NORMAL_T *)chunk;
         vg_segment_lengths_get_line_p(p, prev_x, prev_y, c->xs[i], c->ys[i], u);
      }
   }

   if (t) {
      if ((chunk > end_chunk) || ((chunk == end_chunk) && (i >= end_i))) {
         t[0] = 1.0f; t[1] = 0.0f;
      } else {
         for (;;) {
            if ((chunk < begin_chunk) || ((chunk == begin_chunk) && (i < begin_i))) {
               t[0] = 1.0f; t[1] = 0.0f;
               break;
            }

            if (*(const uint8_t *)chunk == 0) {
               if (vg_segment_lengths_get_cubic_t(t, ((const VG_TESS_CHUNK_CUBIC_T *)chunk)->p, u)) {
                  break;
               }
               if (((const VG_TESS_CHUNK_CUBIC_T *)chunk)->subpath_first) {
                  t[0] = 1.0f; t[1] = 0.0f;
                  break;
               }
            } else {
               const VG_TESS_CHUNK_NORMAL_T *c = (const VG_TESS_CHUNK_NORMAL_T *)chunk;
               float prev_x, prev_y;
               if ((i == 0) && c->subpath_first) {
                  t[0] = 1.0f; t[1] = 0.0f;
                  break;
               }
               get_prev_p(&prev_x, &prev_y, chunk, i);
               if (vg_segment_lengths_get_line_t(t, prev_x, prev_y, c->xs[i], c->ys[i])) {
                  break;
               }
            }

            u = 1.0f;
            back(&chunk, &i);
         }
      }
   }
}
