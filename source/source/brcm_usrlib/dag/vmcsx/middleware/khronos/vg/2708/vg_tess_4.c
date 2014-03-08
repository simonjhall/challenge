//#define VG_TESS_IDENTIFY_CENTERS
#ifndef __BCM2708A0__
   #define VG_TESS_QPU /* not supported on a0 */
#endif
//#define SIMPENROSE_QDEBUG
#ifdef VG_TESS_QPU
   //#define VG_TESS_DUMP_CLIF
#endif

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
#include "interface/khronos/common/khrn_int_math.h"
#include "middleware/khronos/vg/2708/vg_tess_priv_4.h"
#ifdef VG_TESS_QPU
   #include "middleware/khronos/vg/2708/vg_tess_init_shader_4.h"
   #include "middleware/khronos/vg/2708/vg_tess_term_shader_4.h"
   #include "middleware/khronos/vg/2708/vg_tess_fill_shader_4.h"
   #include "middleware/khronos/vg/2708/vg_tess_stroke_shader_4.h"
#endif
#include "middleware/khronos/common/khrn_hw.h"
#include "middleware/khronos/common/khrn_llat.h"
#include "middleware/khronos/common/2708/khrn_nmem_4.h"
#include "middleware/khronos/common/2708/khrn_prod_4.h"
#if defined(VG_TESS_QPU) && !defined(SIMPENROSE)
   #ifdef VC_QDEBUG
      #include "v3d/verification/tools/2760sim/qdebug.h"
   #endif
   #include "vcfw/drivers/chip/v3d.h"
#endif
#ifndef VG_TESS_QPU
   #include <setjmp.h>
#endif
#ifdef VG_TESS_DUMP_CLIF
   #include <stdio.h>
#endif
#include <stddef.h> /* for offsetof */

#ifdef VG_TESS_QPU
   #define SHADER_IF_SIZE (4 + \
      /* persistent vpm size... */ \
      128 + /* 2 common rows */ \
      (KHRN_HW_QPUS_N * 8)) /* pointer + size per qpu */

   /* on the qpus, blocks are divided up into subblocks of this size. this
    * should match the value in vg_tess_4.qinc */
   #define SUBBLOCK_SIZE 2044
   vcos_static_assert((KHRN_NMEM_GROUP_BLOCK_SIZE / SUBBLOCK_SIZE) >= 1);
   vcos_static_assert((KHRN_NMEM_GROUP_BLOCK_SIZE / SUBBLOCK_SIZE) <= 0x7ff);
#endif

/* hack for cts! */
#define BIAS -1.0e-2f

/******************************************************************************
helpers
******************************************************************************/

/*
   only finds solutions in (0, 1)
   if two solutions are returned, t0 <= t1
*/

static uint32_t solve_quadratic(
   float *t0, float *t1,
   float a, float b, float c)
{
   float u;
   bool ok0, ok1;

   /*
      http://en.wikipedia.org/wiki/Quadratic_equation#Floating_point_implementation

      at^2 + bt + c = 0
      u = -1/2 * (b + (sign(b) * sqrt(b^2 - 4ac)))
      t0 = c / u
      t1 = u / a
   */

   u = (b * b) - (4.0f * a * c);
   if (u < 0.0f) {
      return 0; /* imaginary solutions */
   }
   u = sqrt_(u);
   u = -0.5f * (b + ((b < 0) ? -u : u));
   *t0 = c * nan_recip_(u);
   *t1 = u * nan_recip_(a);

   ok0 = nan_gt_(*t0, 0.0f) && (*t0 < 1.0f);
   ok1 = nan_gt_(*t1, 0.0f) && (*t1 < 1.0f);

   if (ok0) {
      return ok1 ? 2 : 1;
   }
   *t0 = *t1;
   return ok1 ? 1 : 0;
}

/*
   calculates the points that have a "velocity" with an x component of 0

   protip: pass p + 1 to calculate the points that have a "velocity" with a y
   component of 0
*/

static uint32_t get_cubic_zvs(
   float *t0, float *t1,
   const float *p)
{
   /*
      x = ((1 - t)^3 * x0) + (3t(1 - t)^2 * x1) + (3t^2(1 - t) * x2) + (t^3 * x3)
        = (a * t^3) + (b * t^2) + (c * t) + d
   */

   float a = -p[0] + (3.0f * p[2]) - (3.0f * p[4]) + p[6];
   float b = (3.0f * p[0]) - (6.0f * p[2]) + (3.0f * p[4]);
   float c = -(3.0f * p[0]) + (3.0f * p[2]);

   /*
      3at^2 + 2bt + c = 0
   */

   return solve_quadratic(t0, t1, 3.0f * a, 2.0f * b, c);
}

#ifndef VG_TESS_QPU
/*
   calculates the inflection points (points where "acceleration" is parallel to
   "velocity")
*/

static uint32_t get_cubic_ips(
   float *t0, float *t1,
   const float *p)
{
   /*
      p = ((1 - t)^3 * p0) + (3t(1 - t)^2 * p1) + (3t^2(1 - t) * p2) + (t^3 * p3)
        = (a * t^3) + (b * t^2) + (c * t) + d
   */

   float ax = -p[0] + (3.0f * p[2]) - (3.0f * p[4]) + p[6];
   float ay = -p[1] + (3.0f * p[3]) - (3.0f * p[5]) + p[7];
   float bx = (3.0f * p[0]) - (6.0f * p[2]) + (3.0f * p[4]);
   float by = (3.0f * p[1]) - (6.0f * p[3]) + (3.0f * p[5]);
   float cx = -(3.0f * p[0]) + (3.0f * p[2]);
   float cy = -(3.0f * p[1]) + (3.0f * p[3]);

   /*
      6(b cross a)t^2 + 6(c cross a)t + 2(c cross b) = 0
   */

   return solve_quadratic(t0, t1,
      6.0f * ((bx * ay) - (by * ax)),
      6.0f * ((cx * ay) - (cy * ax)),
      2.0f * ((cx * by) - (cy * bx)));
}
#endif

/******************************************************************************
simple geometry
******************************************************************************/

#ifdef WORKAROUND_HW2187
bool vg_tess_dummy(KHRN_FMEM_T *fmem, uint32_t offset)
{
   uint8_t *p;

   /* negate offset x/y */
   offset = -(int32_t)offset;
   if (offset & 0xffff) { offset += 1 << 16; }

   /*
      put a non-degenerate (so it will count as a primitive between shader
      changes) triangle offscreen (so it won't actually do anything)
   */

   p = (uint8_t *)khrn_fmem_cle(fmem, 15);
   if (!p) { return false; }
   ADD_BYTE(p, KHRN_HW_INSTR_COMPRESSED_LIST);
   ADD_BYTE(p, 0x81);
   ADD_WORD(p, offset);
   ADD_WORD(p, offset - 1);
   ADD_WORD(p, offset - (1 << 16));
   ADD_BYTE(p, 0x80);
   vcos_assert(khrn_fmem_is_here(fmem, p));

   return true;
}
#endif

static void *rect_placehold(
   KHRN_FMEM_T *fmem)
{
   return khrn_fmem_cle(fmem, 1 + 26 + 1);
}

static void *rect_do(void *rect, uint32_t min_xy, uint32_t max_xy)
{
   uint8_t *p = (uint8_t *)rect;

   if (((min_xy ^ max_xy) & 0xffff) && ((min_xy ^ max_xy) >> 16)) {
      /*
         two triangles in compressed list
         todo: on b0, use rht? certainly use special inline vg thing
      */

	   Add_byte(p, KHRN_HW_INSTR_COMPRESSED_LIST);
      ADD_BYTE(p, 0x81);
      ADD_WORD(p, min_xy);
      ADD_WORD(p, max_xy);
      ADD_WORD(p, (min_xy & 0xffff) | (max_xy & 0xffff0000));
      ADD_BYTE(p, 0x81);
      ADD_WORD(p, min_xy);
      ADD_WORD(p, (max_xy & 0xffff) | (min_xy & 0xffff0000));
      ADD_WORD(p, max_xy);
      ADD_BYTE(p, 0x80);
   } else {
      /* degenerate: not allowed in compressed list (todo: won't need to handle this special case on b0) */
      memset(p, KHRN_HW_INSTR_NOP, 1 + 26 + 1);
      p += (1 + 26 + 1);
   }
   return p;
}

static bool rect(KHRN_FMEM_T *fmem,
   uint32_t min_xy, uint32_t max_xy)
{
   void *rect = rect_placehold(fmem);
   if (!rect) { return false; }
   rect_do(rect, min_xy, max_xy);
   return true;
}

bool vg_tess_rect(KHRN_FMEM_T *fmem,
   uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
   return rect(fmem, (x << 4) | (y << 20), ((x + width) << 4) | ((y + height) << 20));
}

bool vg_tess_quad(KHRN_FMEM_T *fmem,
   const float *p, const float *clip,
   VG_TESS_QUAD_REP_T *quad_rep)
{
   uint32_t i;
   float a[16]; /* enough room for clipped vertices */
   uint32_t n;

   VG_TESS_QUAD_REP_T quad_rep_temp; /* need a VG_TESS_QUAD_REP_T, so if we weren't given one, use a temporary one */
   if (!quad_rep) { quad_rep = &quad_rep_temp; }

   for (i = 0; i != 8; ++i) {
      a[i] = p[i] + BIAS;
   }

   /*
      clip quad if necessary
   */

   if (/* test against min x/y */
      (a[0] < clip[0]) || (a[1] < clip[1]) ||
      (a[2] < clip[0]) || (a[3] < clip[1]) ||
      (a[4] < clip[0]) || (a[5] < clip[1]) ||
      (a[6] < clip[0]) || (a[7] < clip[1]) ||
      /* test against max x/y */
      (a[0] > clip[2]) || (a[1] > clip[3]) ||
      (a[2] > clip[2]) || (a[3] > clip[3]) ||
      (a[4] > clip[2]) || (a[5] > clip[3]) ||
      (a[6] > clip[2]) || (a[7] > clip[3])) {
      float b[16];
      n = vg_tess_poly_clip(a, b, 4, clip);
   } else {
      n = 4;
   }

   /*
      convert verts to 32-bit xy
   */

   quad_rep->n = n;
   for (i = 0; i != n; ++i) {
      quad_rep->p[i] = (float_to_int_shift(a[i * 2], 4) & 0xffff) | (float_to_int_shift(a[(i * 2) + 1], 4) << 16);
   }

   /*
      vg_tess_quad_rep will do the rest (write verts to control list)
   */

   return vg_tess_quad_rep(fmem, quad_rep);
}

bool vg_tess_quad_rep(KHRN_FMEM_T *fmem,
   VG_TESS_QUAD_REP_T *quad_rep)
{
   if (quad_rep->n != 0) {
      uint8_t *p;
      uint32_t i;

      vcos_assert(quad_rep->n >= 3);

      /*
         write verts out in compressed list
         todo: on b0, use special inline vg thing
      */

      /* todo: possible to get degenerate prims here? not allowed in compressed list */

      p = khrn_fmem_cle(fmem, 1 + ((quad_rep->n - 2) * 13) + 1);
      if (!p) { return false; }
      Add_byte(p, KHRN_HW_INSTR_COMPRESSED_LIST);
      for (i = 0; i != (quad_rep->n - 2); ++i) {
         ADD_BYTE(p, 0x81);
         ADD_WORD(p, quad_rep->p[0]);
         ADD_WORD(p, quad_rep->p[i + 1]);
         ADD_WORD(p, quad_rep->p[i + 2]);
      }
      ADD_BYTE(p, 0x80);
      vcos_assert(khrn_fmem_is_here(fmem, p));
   }
   return true;
}

/******************************************************************************
precomputed tessellation
******************************************************************************/

MEM_HANDLE_T vg_tess_alloc(void)
{
   return mem_alloc_ex(0, 4, (MEM_FLAG_T)(MEM_FLAG_DISCARDABLE | MEM_FLAG_RESIZEABLE | MEM_FLAG_ZERO | MEM_FLAG_INIT
#ifdef VG_TESS_QPU
      | MEM_FLAG_DIRECT /* todo: this may hurt vg_tess_gen performance */
#endif
      ), "vg_tess", MEM_COMPACT_DISCARD);
}

static float get_coord(
   VGPathDatatype datatype, float scale, float bias,
   const void **coords)
{
   switch (datatype) {
   case VG_PATH_DATATYPE_S_8:  return bias + (scale * *((*(const int8_t **)coords)++));
   case VG_PATH_DATATYPE_S_16: return bias + (scale * *((*(const int16_t **)coords)++));
   case VG_PATH_DATATYPE_S_32: return bias + (scale * *((*(const int32_t **)coords)++));
   case VG_PATH_DATATYPE_F:    return bias + (scale * *((*(const float **)coords)++));
   default:                    UNREACHABLE(); return 0.0f;
   }
}

static INLINE uint32_t get_chunk_size(bool normal)
{
   return normal ? sizeof(VG_TESS_CHUNK_NORMAL_T) : sizeof(VG_TESS_CHUNK_CUBIC_T);
}

static bool extend(MEM_HANDLE_T handle, void **last, uint32_t *size, bool normal)
{
   uint32_t new_size = *size + get_chunk_size(normal);
   uint32_t capacity = mem_get_size(handle);
   if (new_size > capacity) {
      if (*size != 0) { mem_unlock(handle); }
      if (!mem_resize_ex(handle, _max(capacity + (capacity >> 1), new_size), MEM_COMPACT_DISCARD) &&
         !mem_resize_ex(handle, new_size, MEM_COMPACT_DISCARD)) {
         verify(mem_resize_ex(handle, 0, MEM_COMPACT_NONE));
         return false;
      }
      *last = (uint8_t *)mem_lock(handle) + *size;
   } else {
      *last = (uint8_t *)*last + get_chunk_size(*(uint8_t *)*last != 0);
   }
   *size = new_size;
   return true;
}

typedef enum {
   NORMAL_TYPE_FIRST,
   NORMAL_TYPE_MIDDLE,
   NORMAL_TYPE_LAST,
   NORMAL_TYPE_LAST_IMPLICIT,
   NORMAL_TYPE_LAST_CLOSE
} NORMAL_TYPE_T;

static bool push_normal(
   MEM_HANDLE_T handle, void **last, uint32_t *size, float x, float y,
   NORMAL_TYPE_T type)
{
   VG_TESS_CHUNK_NORMAL_T *chunk;
   uint32_t i;

   chunk = (VG_TESS_CHUNK_NORMAL_T *)*last;
   if (!chunk || (chunk->n == 0) || (chunk->n == 7) || (type == NORMAL_TYPE_FIRST)) {
      /* no normal chunk at end, normal chunk at end is full, or new subpath (new chunk on new subpath simplifies qpu stuff) */
      if (!extend(handle, last, size, true)) { return false; }
      chunk = (VG_TESS_CHUNK_NORMAL_T *)*last;
      /* chunk already zeroed by MEM_FLAG_ZERO */
   }
   vcos_assert(chunk && (chunk->n < 7) && !chunk->subpath_last_implicit);
   vcos_assert(chunk->n == chunk->explicit_n);

   i = chunk->n++;
   if (type != NORMAL_TYPE_LAST_IMPLICIT) { ++chunk->explicit_n; }
   chunk->xs[i] = x;
   chunk->ys[i] = y;
   switch (type) {
   case NORMAL_TYPE_FIRST:
      chunk->subpath_first = true;
      break;
   case NORMAL_TYPE_MIDDLE:
      vcos_assert(!chunk->subpath_last && !chunk->close);
      break;
   case NORMAL_TYPE_LAST:
      vcos_assert(!chunk->subpath_last && !chunk->close);
      chunk->subpath_last = true;
      break;
   case NORMAL_TYPE_LAST_IMPLICIT:
      vcos_assert(!chunk->close);
      chunk->subpath_last_implicit = true;
      break;
   case NORMAL_TYPE_LAST_CLOSE:
      vcos_assert(!chunk->subpath_last && !chunk->close);
      chunk->subpath_last_implicit = true;
      chunk->subpath_last = true;
      chunk->close = true;
      break;
   default:
      UNREACHABLE();
   }

   return true;
}

static bool push_trivial_subpath(
   MEM_HANDLE_T handle, void **last, uint32_t *size, float x, float y)
{
   return
      push_normal(handle, last, size, x, y, NORMAL_TYPE_FIRST) &&
      push_normal(handle, last, size, x, y, NORMAL_TYPE_LAST) &&
      push_normal(handle, last, size, x, y, NORMAL_TYPE_LAST_IMPLICIT);
}

static bool push_cubic(
   MEM_HANDLE_T handle, void **last, uint32_t *size, const float *p,
   bool subpath_first)
{
   VG_TESS_CHUNK_CUBIC_T *chunk;

   if (!extend(handle, last, size, false)) { return false; }
   chunk = (VG_TESS_CHUNK_CUBIC_T *)*last;
   /* chunk already zeroed by MEM_FLAG_ZERO */

   chunk->subpath_first = subpath_first;
   memcpy(chunk->p, p, sizeof(chunk->p));
   chunk->magic = VG_TESS_CHUNK_CUBIC_MAGIC;

   return true;
}

static void set_subpath_last(void *last)
{
   if (*(uint8_t *)last == 0) {
      VG_TESS_CHUNK_CUBIC_T *chunk = (VG_TESS_CHUNK_CUBIC_T *)last;
      vcos_assert(!chunk->subpath_last);
      chunk->subpath_last = true;
   } else {
      VG_TESS_CHUNK_NORMAL_T *chunk = (VG_TESS_CHUNK_NORMAL_T *)last;
      vcos_assert(!chunk->subpath_last);
      chunk->subpath_last = true;
   }
}

static void extend_bounds_normal(float *bounds, float x, float y)
{
   bounds[0] = _minf(bounds[0], x);
   bounds[1] = _minf(bounds[1], y);
   bounds[2] = _maxf(bounds[2], x);
   bounds[3] = _maxf(bounds[3], y);
}

static void extend_bounds_cubic(float *bounds, const float *p)
{
   float t_zvx0, t_zvx1;
   uint32_t zvxs_count;
   float t_zvy0, t_zvy1;
   uint32_t zvys_count;

   /*
      the calculated bounds will be used by vgPathBounds, which requires tight
      bounds, so we can't just include the control points

      instead, include the start and end points of the cubic...
   */

   bounds[0] = _minf(_minf(bounds[0], p[0]), p[6]);
   bounds[1] = _minf(_minf(bounds[1], p[1]), p[7]);
   bounds[2] = _maxf(_maxf(bounds[2], p[0]), p[6]);
   bounds[3] = _maxf(_maxf(bounds[3], p[1]), p[7]);

   /*
      ...and any turning points
   */

   zvxs_count = get_cubic_zvs(&t_zvx0, &t_zvx1, p);
   if (zvxs_count > 0) {
      float a_x = -p[0] + (3.0f * p[2]) - (3.0f * p[4]) + p[6];
      float b_x = (3.0f * p[0]) - (6.0f * p[2]) + (3.0f * p[4]);
      float c_x = -(3.0f * p[0]) + (3.0f * p[2]);
      float d_x = p[0];
      float x;

      if (zvxs_count > 1) {
         float x = (((((a_x * t_zvx1) + b_x) * t_zvx1) + c_x) * t_zvx1) + d_x;
         bounds[0] = _minf(bounds[0], x);
         bounds[2] = _maxf(bounds[2], x);
      }

      x = (((((a_x * t_zvx0) + b_x) * t_zvx0) + c_x) * t_zvx0) + d_x;
      bounds[0] = _minf(bounds[0], x);
      bounds[2] = _maxf(bounds[2], x);
   }

   zvys_count = get_cubic_zvs(&t_zvy0, &t_zvy1, p + 1);
   if (zvys_count > 0) {
      float a_y = -p[1] + (3.0f * p[3]) - (3.0f * p[5]) + p[7];
      float b_y = (3.0f * p[1]) - (6.0f * p[3]) + (3.0f * p[5]);
      float c_y = -(3.0f * p[1]) + (3.0f * p[3]);
      float d_y = p[1];
      float y;

      if (zvys_count > 1) {
         float y = (((((a_y * t_zvy1) + b_y) * t_zvy1) + c_y) * t_zvy1) + d_y;
         bounds[1] = _minf(bounds[1], y);
         bounds[3] = _maxf(bounds[3], y);
      }

      y = (((((a_y * t_zvy0) + b_y) * t_zvy0) + c_y) * t_zvy0) + d_y;
      bounds[1] = _minf(bounds[1], y);
      bounds[3] = _maxf(bounds[3], y);
   }
}

static void quad_to_cubic(float *p, const float *quad_p)
{
   p[0] = quad_p[0];
   p[1] = quad_p[1];
   p[2] = (quad_p[0] + (2.0f * quad_p[2])) * (1.0f / 3.0f);
   p[3] = (quad_p[1] + (2.0f * quad_p[3])) * (1.0f / 3.0f);
   p[4] = ((2.0f * quad_p[2]) + quad_p[4]) * (1.0f / 3.0f);
   p[5] = ((2.0f * quad_p[3]) + quad_p[5]) * (1.0f / 3.0f);
   p[6] = quad_p[4];
   p[7] = quad_p[5];
}

static bool vg_tess_gen(
   MEM_HANDLE_T tess_handle, float *bounds,
   VGPathDatatype datatype, float scale, float bias,
   uint8_t *segments, uint32_t segments_count,
   const void *coords)
{
   void *tess_last = NULL;
   uint32_t tess_size = 0;

   float s_x = 0.0f, s_y = 0.0f;
   float o_x = 0.0f, o_y = 0.0f;
   float p_x = 0.0f, p_y = 0.0f;
   bool subpath_first = true;

   for (; segments_count != 0; ++segments, --segments_count) {
      #define GET_COORD() get_coord(datatype, scale, bias, &coords)

      uint32_t prev_tess_size = tess_size;
      uint32_t normals_n = 0, cubics_n = 0;

      uint32_t segment = *segments;

      switch (segment & VG_SEGMENT_MASK) {
      case VG_CLOSE_PATH:
      {
         if (subpath_first) {
            vcos_assert((s_x == o_x) && (o_x == p_x) && (s_y == o_y) && (o_y == p_y));
            if (!push_trivial_subpath(tess_handle, &tess_last, &tess_size, s_x, s_y)) { return false; }
            if (bounds) { extend_bounds_normal(bounds, s_x, s_y); }
            normals_n += 3;
         } else {
            if (!push_normal(tess_handle, &tess_last, &tess_size, s_x, s_y, NORMAL_TYPE_LAST_CLOSE)) { return false; }
            /* point already included in bounds */
            ++normals_n;
         }

         o_x = s_x;  o_y = s_y;
         p_x = s_x;  p_y = s_y;
         subpath_first = true;
         break;
      }
      case VG_MOVE_TO:
      {
         float x = GET_COORD();
         float y = GET_COORD();
         if (segment & VG_RELATIVE) {
            x += o_x;  y += o_y;
         }

         if (!subpath_first) {
            set_subpath_last(tess_last);
            if (!push_normal(tess_handle, &tess_last, &tess_size, s_x, s_y, NORMAL_TYPE_LAST_IMPLICIT)) { return false; }
            /* point already included in bounds */
            ++normals_n;
         }

         s_x = x;  s_y = y;
         o_x = x;  o_y = y;
         p_x = x;  p_y = y;
         subpath_first = true;
         break;
      }
      case VG_LINE_TO:
      case VG_HLINE_TO:
      case VG_VLINE_TO:
      {
         float x, y;
         if ((segment & VG_SEGMENT_MASK) == VG_VLINE_TO) {
            x = o_x;
         } else {
            x = GET_COORD();
            if (segment & VG_RELATIVE) {
               x += o_x;
            }
         }
         if ((segment & VG_SEGMENT_MASK) == VG_HLINE_TO) {
            y = o_y;
         } else {
            y = GET_COORD();
            if (segment & VG_RELATIVE) {
               y += o_y;
            }
         }

         if (subpath_first) {
            vcos_assert((s_x == o_x) && (s_y == o_y));
            if (!push_normal(tess_handle, &tess_last, &tess_size, s_x, s_y, NORMAL_TYPE_FIRST)) { return false; }
            if (bounds) { extend_bounds_normal(bounds, s_x, s_y); }
            ++normals_n;
         }
         if (!push_normal(tess_handle, &tess_last, &tess_size, x, y, NORMAL_TYPE_MIDDLE)) { return false; }
         if (bounds) { extend_bounds_normal(bounds, x, y); }
         ++normals_n;

         o_x = x;  o_y = y;
         p_x = x;  p_y = y;
         subpath_first = false;
         break;
      }
      case VG_QUAD_TO:
      case VG_CUBIC_TO:
      case VG_SQUAD_TO:
      case VG_SCUBIC_TO:
      {
         float p[8];
         if (((segment & VG_SEGMENT_MASK) == VG_QUAD_TO) || ((segment & VG_SEGMENT_MASK) == VG_SQUAD_TO)) {
            float quad_p[6];

            quad_p[0] = o_x;  quad_p[1] = o_y;
            if ((segment & VG_SEGMENT_MASK) == VG_SQUAD_TO) {
               quad_p[2] = (2.0f * o_x) - p_x;
               quad_p[3] = (2.0f * o_y) - p_y;
            } else {
               quad_p[2] = GET_COORD();
               quad_p[3] = GET_COORD();
               if (segment & VG_RELATIVE) {
                  quad_p[2] += o_x;  quad_p[3] += o_y;
               }
            }
            quad_p[4] = GET_COORD();
            quad_p[5] = GET_COORD();
            if (segment & VG_RELATIVE) {
               quad_p[4] += o_x;  quad_p[5] += o_y;
            }

            p_x = quad_p[2];  p_y = quad_p[3];

            quad_to_cubic(p, quad_p);
         } else {
            p[0] = o_x;  p[1] = o_y;
            if ((segment & VG_SEGMENT_MASK) == VG_SCUBIC_TO) {
               p[2] = (2.0f * o_x) - p_x;
               p[3] = (2.0f * o_y) - p_y;
            } else {
               p[2] = GET_COORD();
               p[3] = GET_COORD();
               if (segment & VG_RELATIVE) {
                  p[2] += o_x;  p[3] += o_y;
               }
            }
            p[4] = GET_COORD();
            p[5] = GET_COORD();
            p[6] = GET_COORD();
            p[7] = GET_COORD();
            if (segment & VG_RELATIVE) {
               p[4] += o_x;  p[5] += o_y;
               p[6] += o_x;  p[7] += o_y;
            }

            p_x = p[4];  p_y = p[5];
         }

         if (!push_cubic(tess_handle, &tess_last, &tess_size, p, subpath_first)) { return false; }
         if (bounds) { extend_bounds_cubic(bounds, p); }
         ++cubics_n;

         o_x = p[6];  o_y = p[7];
         subpath_first = false;
         break;
      }
      case VG_SCCWARC_TO:
      case VG_SCWARC_TO:
      case VG_LCCWARC_TO:
      case VG_LCWARC_TO:
      {
         float p[4 * 8]; /* up to 4 cubics */
         uint32_t count, i;

         float r_x = GET_COORD();
         float r_y = GET_COORD();
         float angle = GET_COORD();
         float x = GET_COORD();
         float y = GET_COORD();
         float rel_x = x;
         float rel_y = y;
         if (segment & VG_RELATIVE) {
            x += o_x;  y += o_y;
         } else {
            rel_x -= o_x;  rel_y -= o_y;
         }

         r_x = absf_(r_x);  r_y = absf_(r_y); /* spec section 8.4: negative radii replaced by absolute values */
         angle = angle * (PI / 180.0f); /* convert angle to radians */

         count = vg_tess_arc_to_cubics(p, o_x, o_y, x, y, rel_x, rel_y, r_x, r_y, angle,
            /* large */
            ((segment & VG_SEGMENT_MASK) == VG_LCCWARC_TO) ||
            ((segment & VG_SEGMENT_MASK) == VG_LCWARC_TO),
            /* cw */
            ((segment & VG_SEGMENT_MASK) == VG_SCWARC_TO) ||
            ((segment & VG_SEGMENT_MASK) == VG_LCWARC_TO));
         vcos_assert((count != 0) && (count <= 4));

         for (i = 0; i != (count * 8); i += 8) {
            if (!push_cubic(tess_handle, &tess_last, &tess_size, p + i, subpath_first && (i == 0))) { return false; }
            if (bounds) { extend_bounds_cubic(bounds, p + i); }
         }
         cubics_n += count;

         o_x = x;  o_y = y;
         p_x = x;  p_y = y;
         subpath_first = false;
         break;
      }
      default:
      {
         UNREACHABLE(); /* should be picked up on client side in vgAppendPathData */
      }
      }

      segment &= VG_SEGMENT_MASK | VG_RELATIVE;
      if ((segment & VG_SEGMENT_MASK) >= VG_QUAD_TO) {
         vcos_assert(normals_n == 0);
         vcos_assert(cubics_n <= 4);
         vcos_assert(((tess_size - prev_tess_size) % sizeof(VG_TESS_CHUNK_CUBIC_T)) == 0);
         vcos_assert(((tess_size - prev_tess_size) / sizeof(VG_TESS_CHUNK_CUBIC_T)) == cubics_n);
         segment |= cubics_n << 5;
      } else {
         uint32_t chunks_n;
         vcos_assert(normals_n <= 3);
         vcos_assert(cubics_n == 0);
         vcos_assert(((tess_size - prev_tess_size) % sizeof(VG_TESS_CHUNK_NORMAL_T)) == 0);
         chunks_n = (tess_size - prev_tess_size) / sizeof(VG_TESS_CHUNK_NORMAL_T);
         vcos_assert(chunks_n <= 1);
         segment |= (chunks_n << 5) | (normals_n << 6);
      }
      *segments = (uint8_t)segment;

      #undef GET_COORD
   }

   if (!subpath_first) {
      set_subpath_last(tess_last);
      if (!push_normal(tess_handle, &tess_last, &tess_size, s_x, s_y, NORMAL_TYPE_LAST_IMPLICIT)) { return false; }
      /* point already included in bounds */
   }

   if (tess_size != 0) { mem_unlock(tess_handle); }
   verify(mem_resize_ex(tess_handle, tess_size, MEM_COMPACT_NONE)); /* shrink to fit */

   return true;
}

bool vg_tess_retain(
   MEM_HANDLE_T tess_handle, float *bounds,
   VGPathDatatype datatype, float scale, float bias,
   MEM_HANDLE_T segments_handle, MEM_HANDLE_T coords_handle)
{
   if (!mem_retain(tess_handle)) {
      if (!vg_tess_gen(
         tess_handle, bounds, datatype, scale, bias,
         (uint8_t *)mem_lock(segments_handle), mem_get_size(segments_handle), mem_lock(coords_handle))) {
         mem_unlock(segments_handle);
         mem_unlock(coords_handle);
         mem_unretain(tess_handle);
         return false;
      }
      mem_unlock(segments_handle);
      mem_unlock(coords_handle);
   }
   return true;
}

/******************************************************************************
streamy allocator
******************************************************************************/

#ifndef VG_TESS_QPU

typedef struct {
   void *p; /* allocate from here */
   uint32_t size; /* how much left in current block? */
} ALLOCATOR_T;

static void allocator_init(ALLOCATOR_T *allocator)
{
   allocator->p = NULL; /* not strictly necessary */
   allocator->size = 0;
}

static void *alloc(ALLOCATOR_T *allocator, uint32_t size,
   KHRN_NMEM_GROUP_T *nmem_group)
{
   vcos_assert(size <= KHRN_NMEM_GROUP_BLOCK_SIZE);
   vcos_assert(!(size & 0x3)); /* keep everything 4-byte aligned */

   /*
      try allocating from current block
   */

   if (size <= allocator->size) {
      void *p = allocator->p;
      allocator->p = (uint8_t *)allocator->p + size;
      allocator->size -= size;
      return p;
   }

   /*
      allocate a new block and try again
   */

   if (!nmem_group) {
      return NULL;
   }

   allocator->p = khrn_nmem_group_alloc(nmem_group, NULL, 0);
   allocator->size = allocator->p ? KHRN_NMEM_GROUP_BLOCK_SIZE : 0;
   return alloc(allocator, size, NULL);
}

static void unalloc(ALLOCATOR_T *allocator, uint32_t size)
{
   vcos_assert((allocator->size + size) <= KHRN_NMEM_GROUP_BLOCK_SIZE);
   vcos_assert(!(size & 0x3)); /* keep everything 4-byte aligned */

   allocator->p = (uint8_t *)allocator->p - size;
   allocator->size += size;
}

#endif

/******************************************************************************
rep + fix stuff
******************************************************************************/

#ifndef VG_TESS_QPU

struct LIST_HEADER_S;
typedef struct LIST_HEADER_S LIST_HEADER_T;
struct LIST_HEADER_S {
   LIST_HEADER_T *next;
   uint32_t len; /* # of vertices */
   uint32_t v[1]; /* flexible array */
};

vcos_static_assert(alignof(LIST_HEADER_T) == 4); /* keep everything 4-byte aligned */

#endif

union REP_U;
typedef union REP_U REP_T;
union REP_U {
   struct {
#ifdef VG_TESS_QPU
      uint8_t *ind_branch_dest;
#else
      LIST_HEADER_T *h;
      uint32_t n;
#endif
      uint32_t min_xy, max_xy; /* only used by fills */
      float user_a, user_b, user_c, user_d; /* extra storage for user-defined stuff */
   } in_use;
   REP_T *free_next;
};

vcos_static_assert(alignof(REP_T) == 4); /* keep everything 4-byte aligned */

#ifndef VG_TESS_QPU

struct LISTS_FOOTER_S;
typedef struct LISTS_FOOTER_S LISTS_FOOTER_T;
struct LISTS_FOOTER_S {
   union {
      uint8_t *branch;
      uint32_t branch_addr;
   } u;
   LISTS_FOOTER_T *next;
   void *p; /* pointer to start of lists */
};

vcos_static_assert(alignof(LISTS_FOOTER_T) == 4); /* keep everything 4-byte aligned */

#define FILL_CENTER_XY_UNWRITTEN 0xbfffbfff /* top-rightiest point possible. shouldn't ever generate this point (the outer clip rectangle falls a little short of this) */

#endif

static uint8_t *rep_placehold(
   KHRN_FMEM_T *fmem)
{
   return khrn_fmem_cle(fmem, 5);
}

#ifndef VG_TESS_QPU

static LISTS_FOOTER_T *rep_do(ALLOCATOR_T *allocator, KHRN_NMEM_GROUP_T *nmem_group,
   uint8_t *branch,
   REP_T *r, bool fill)
{
   uint32_t size, rounded_up_size;
   uint8_t *p;
   LISTS_FOOTER_T *f;
   uint32_t i;
   LIST_HEADER_T *h;
   uint32_t center_xy = 0; /* initialise to avoid warning */

   size = (r->in_use.n * 10) + 1;
   rounded_up_size = round_up(size, 4);
   p = (uint8_t *)alloc(allocator, rounded_up_size + sizeof(LISTS_FOOTER_T), nmem_group);
   if (!p) {
      return NULL;
   }
   p += rounded_up_size - size;

   f = (LISTS_FOOTER_T *)(p + size);
   f->u.branch = branch;
   f->p = p;

   for (i = 0, h = r->in_use.h; i != r->in_use.n; ++i, h = h->next) {
      ADD_BYTE(p, KHRN_HW_INSTR_VG_COORD_LIST);
      ADD_BYTE(p, fill ? 6 /* triangle fan */ : 4); /* triangles */
      ADD_WORD(p, h->len);
#if defined(SIMPENROSE_RECORD_OUTPUT) && defined(SIMPENROSE_RECORD_BINNING)
      record_map_buffer(khrn_hw_addr(h->v), h->len * 4, L_XY_COORDS, RECORD_BUFFER_IS_BOTH, 4);
#endif
      ADD_WORD(p, khrn_hw_addr(khrn_hw_alias_direct(h->v)));
      if (fill) {
         if (h->v[0] == FILL_CENTER_XY_UNWRITTEN) {
            h->v[0] = center_xy;
         } else {
            center_xy = h->v[0];
         }
      }
   }
   vcos_assert(!h); /* r->in_use.n should be consistent with the actual number of lists */

   ADD_BYTE(p, KHRN_HW_INSTR_BRANCH);
   vcos_assert(p == (uint8_t *)f);

#if defined(SIMPENROSE_RECORD_OUTPUT) && defined(SIMPENROSE_RECORD_BINNING)
   record_map_buffer(khrn_hw_addr(p - size), size + 4, L_INSTRUCTIONS, RECORD_BUFFER_IS_BOTH, 1);
#endif

   return f;
}

/* todo: fix pass now unnecessary? */
static void fix(LISTS_FOOTER_T *f)
{
   for (; f; f = f->next) {
      uint8_t *branch;
      branch = f->u.branch;
      ADD_BYTE(branch, KHRN_HW_INSTR_BRANCH);
      ADD_WORD(branch, khrn_hw_addr(khrn_hw_alias_direct(f->p)));
      f->u.branch_addr = khrn_hw_addr(khrn_hw_alias_direct(branch));
   }
}

#endif

/******************************************************************************
clif dumping
******************************************************************************/

#ifdef VG_TESS_DUMP_CLIF

typedef struct {
   FILE *f;
   uint32_t i;
} DUMP_CLIF_T;

/* rest below... */

#endif

/******************************************************************************
TESS_T allocing/freeing
******************************************************************************/

#define TESSS_N_MAX 16 /* must be >= the number of render states! */

typedef enum {
   TESS_STATE_STARTED,
   TESS_STATE_WAIT_CALLED,
   TESS_STATE_WAIT_DONE,
   TESS_STATE_FINISH_LLAT_DONE
} TESS_STATE_T;

typedef struct {
   REP_T *reps;
#ifdef VG_TESS_QPU
   bool flipbit;
#endif
   union {
      struct {
         bool ok;
#ifdef VG_TESS_QPU
         bool flipped_bit;
#endif
         TESS_STATE_T state;
         REP_T *r_head, *r_tail; /* REP_T free list */
         KHRN_NMEM_GROUP_T nmem_group;
         bool nmem_entered;
         uint32_t nmem_pos;
#ifndef VG_TESS_QPU
         ALLOCATOR_T v_allocator; /* for actual vertex data */
         ALLOCATOR_T prim_list_allocator; /* for VG_COORD_LIST control list segments */
         LISTS_FOOTER_T *f_head;
#endif
#ifdef VG_TESS_DUMP_CLIF
         DUMP_CLIF_T dc;
#endif
      } in_use;
      VG_TESS_HANDLE_T free_next;
   } u;
} TESS_T;

static uint32_t tess_reps_n;

static uint32_t tesss_n; /* kept track of for debugging purposes only */
static TESS_T tesss[TESSS_N_MAX];
static VG_TESS_HANDLE_T tesss_free_head;

static INLINE VG_TESS_HANDLE_T tess_to_handle(TESS_T *tess)
{
   vcos_assert(tess >= tesss);
   vcos_assert(tess < (tesss + TESSS_N_MAX));
   return (VG_TESS_HANDLE_T)(uintptr_t)tess;
}

static INLINE TESS_T *tess_from_handle(VG_TESS_HANDLE_T tess_handle)
{
   TESS_T *tess = (TESS_T *)(uintptr_t)tess_handle;
   vcos_assert(tess >= tesss);
   vcos_assert(tess < (tesss + TESSS_N_MAX));
   return tess;
}

static VG_TESS_HANDLE_T alloc_tess_handle(void)
{
   VG_TESS_HANDLE_T tess_handle = tesss_free_head;
   if (tess_handle != VG_TESS_HANDLE_INVALID) {
      ++tesss_n;
      tesss_free_head = tess_from_handle(tess_handle)->u.free_next;
   }
   return tess_handle;
}

static void free_tess_handle(VG_TESS_HANDLE_T tess_handle)
{
   --tesss_n;
   tess_from_handle(tess_handle)->u.free_next = tesss_free_head;
   tesss_free_head = tess_handle;
}

/******************************************************************************
REP_T alloc stuff
******************************************************************************/

/* we always leave at least one REP_T in the free list. this makes things alot
 * simpler... */

static void add_rep(TESS_T *tess, REP_T *r)
{
   tess->u.in_use.r_tail->free_next = r;
   khrn_barrier();
   tess->u.in_use.r_tail = r;
   /* don't notify master -- the calling function will do that */
}

static REP_T *alloc_rep(TESS_T *tess, KHRN_FMEM_T *fmem)
{
   REP_T *r = tess->u.in_use.r_head;
   while (r == tess->u.in_use.r_tail) {
      if (!tess->u.in_use.ok) {
         return NULL;
      }
      khrn_sync_master_wait();
   }
   khrn_barrier();
   tess->u.in_use.r_head = r->free_next;
#ifdef VG_TESS_QPU
   /* create storage for indirect branch and bb stuff and alignment*/
   /* todo - stick this into the rep structure once the shaders have been fettled */
   r->in_use.ind_branch_dest = (uint8_t *)khrn_fmem_junk(fmem, 5 + 29 + 2, 4);
   if (!r->in_use.ind_branch_dest) {
      add_rep(tess, r);
      return NULL;
   }
#else
   UNUSED(fmem);
#endif
   return r;
}

/******************************************************************************
io chunk stuff
******************************************************************************/

#define IO_CHUNKS_N 63 /* todo: what should this be? */
#ifdef VG_TESS_QPU
   #define IO_CHUNKS_EXTRA 2 /* how many extra io chunks can a shader read in? */
#endif

typedef struct {
#ifdef VG_TESS_QPU
   uint32_t tess_i;
#else
   TESS_T *tess;
#endif
   uint32_t chunks_a, chunks_b;
   uint32_t chunks_size;
   float t;
   float user_to_surface[6];
   float clip[4]; /* actually clip_inner for strokes */
#ifdef VG_TESS_QPU
   /* same place as done in IO_CHUNK_O_T, which uses -1 as magic done value
    * (flipbit can be 0 or 1) */
   uint32_t flipbit;
#else
   uint32_t dummy;
#endif
} IO_CHUNK_I_T;

typedef enum {
   CAP_BUTT,
   CAP_ROUND,
   CAP_SQUARE
} CAP_T;

typedef enum {
   JOIN_MITER,
   JOIN_ROUND,
   JOIN_BEVEL
} JOIN_T;

typedef struct {
   float line_width;
   float clip_outer[4];
   float miter_limit;
   float oo_theta;
   float cos_theta;
   float cos_theta_inter;
   float sin_theta;
   float oo_flatness_sq;
   float width_hack_clerp;
   float width_hack_round;
   uint32_t cap              : 2; /* CAP_T */
   uint32_t join             : 2; /* JOIN_T */
   uint32_t dashing          : 1; /* bool */
   uint32_t dash_pattern_n   : 5; /* uint32_t */
   uint32_t dash_phase_i     : 4; /* uint32_t */
   uint32_t dash_phase_reset : 1; /* bool */
   float dash_oo_scale;
   float dash_phase;
} IO_CHUNK_STROKE_I2_T;

typedef struct {
   float dash_pattern[VG_CONFIG_MAX_DASH_COUNT];
} IO_CHUNK_STROKE_I3_T;

#ifdef VG_TESS_QPU
typedef struct {
   uint32_t branch;
   /* 4-byte aligned, size at least _max(KHRN_HW_SYSTEM_CACHE_LINE_SIZE - 4, 0)
    * + round_up(sizeof(REP_T), KHRN_HW_SYSTEM_CACHE_LINE_SIZE) */
   uint32_t rep;
   int32_t min_x, max_x, min_y, max_y; /* bounding box (fill only) */
   uint32_t dummy[9];
   /* use -1 as magic done value. same place as flipbit in IO_CHUNK_I_T, which
    * can only be 0 or 1 */
   uint32_t done;
} IO_CHUNK_O_T;
#endif

typedef union {
   IO_CHUNK_I_T i;
   IO_CHUNK_STROKE_I2_T stroke_i2;
   IO_CHUNK_STROKE_I3_T stroke_i3;
#ifdef VG_TESS_QPU
   IO_CHUNK_O_T o;
#endif
} IO_CHUNK_T;

vcos_static_assert(sizeof(IO_CHUNK_I_T) == 64);
vcos_static_assert(sizeof(IO_CHUNK_STROKE_I2_T) == 64);
vcos_static_assert(sizeof(IO_CHUNK_STROKE_I3_T) == 64);
#ifdef VG_TESS_QPU
vcos_static_assert(sizeof(IO_CHUNK_O_T) == 64);
#endif
vcos_static_assert(sizeof(IO_CHUNK_T) == 64);

static IO_CHUNK_T *io_chunks, *io_chunk_alloc, *io_chunk_free;

static INLINE IO_CHUNK_T *prev_io_chunk(IO_CHUNK_T *io_chunk)
{
   return (io_chunk == io_chunks) ? (io_chunks + (IO_CHUNKS_N - 1)) : (io_chunk - 1);
}

static INLINE IO_CHUNK_T *next_io_chunk(IO_CHUNK_T *io_chunk)
{
   return (io_chunk == (io_chunks + (IO_CHUNKS_N - 1))) ? io_chunks : (io_chunk + 1);
}

/* only call from master task */
static IO_CHUNK_T *alloc_io_chunk(void)
{
   IO_CHUNK_T *io_chunk;
   while (next_io_chunk(io_chunk_alloc) == io_chunk_free) {
      khrn_sync_master_wait(); /* wait for one to be freed */
   }
   khrn_barrier();
   io_chunk = io_chunk_alloc;
   io_chunk_alloc = next_io_chunk(io_chunk_alloc);
   return io_chunk;
}

/* only call from llat task (frees oldest unfreed -- io_chunk parameter is optional) */
static void free_io_chunk(IO_CHUNK_T *io_chunk)
{
   vcos_assert(!io_chunk || (io_chunk == io_chunk_free));
   khrn_barrier();
   io_chunk_free = next_io_chunk(io_chunk_free);
   /* caller will notify master task */
}

/******************************************************************************
core message stuff
******************************************************************************/

#define MSGS_SIZE 8192 /* todo: how big should this be? */

typedef uint16_t MSG_TYPE_T;
#define MSG_TYPE_NOP         0 /* used for padding */
#define MSG_TYPE_WAIT        1
#define MSG_TYPE_FILL_STROKE 2
#define MSG_TYPE_REP         3
#define MSG_TYPE_FILL_BBOX   4

typedef struct {
   MSG_TYPE_T type;
   uint16_t size;
} MSG_HEADER_T;

/* can always fit a header in if there is any space at all (fifo is kept 4-byte aligned) */
vcos_static_assert(sizeof(MSG_HEADER_T) == 4);

typedef struct {
   TESS_T *tess;
   void (*callback)(bool, void *);
   void *p;
} MSG_WAIT_T;

typedef struct {
   TESS_T *tess;
#ifdef VG_TESS_QPU
   uint32_t shader;
#else
   bool (*f)(const IO_CHUNK_T *io_chunk, REP_T *r);
#endif
   IO_CHUNK_T *io_chunk; /* first. use next_io_chunk to get next */
   uint32_t io_chunks_n;
   MEM_HANDLE_T chunks_a, chunks_b;
   REP_T *r;
} MSG_FILL_STROKE_T;

typedef struct {
   TESS_T *tess;
   REP_T *r;
#ifndef VG_TESS_QPU
   bool fill;
#endif
   uint8_t *branch;
} MSG_REP_T;

typedef struct {
   TESS_T *tess;
   REP_T *r;
   uint8_t *rect;
} MSG_FILL_BBOX_T;

/* keep fifo 4-byte aligned */
vcos_static_assert(alignof(MSG_WAIT_T) == 4);
vcos_static_assert(alignof(MSG_FILL_STROKE_T) == 4);
vcos_static_assert(alignof(MSG_REP_T) == 4);
vcos_static_assert(alignof(MSG_FILL_BBOX_T) == 4);

static uint8_t *msgs;

static struct {
   /* there's no need for both submit and done unless we're doing the
    * tessellation on the qpus, but always having both keeps things simple */
   uint8_t *post, *submit, *done;
} vg_tess_msg;

/* if wait_enter_nmem_pos == wait_exit_nmem_pos, there are no nmem-holding waits
 * (waits which might directly hold up the nmem fifo) in the tessellation fifo,
 * otherwise:
 * - wait_enter_nmem_pos is the nmem pos of the most recently posted
 *   nmem-holding wait
 * - wait_exit_nmem_pos is "less than" (nmem positions wrap) the nmem positions
 *   of all nmem-holding waits in the fifo
 * normally, wait_enter_nmem_pos is updated by the master task and
 * wait_exit_nmem_pos is updated by the llat task, however, if when
 * wait_enter_nmem_pos is about to be set, there are currently no nmem-holding
 * waits in the fifo, wait_exit_nmem_pos is set to the new wait_enter_nmem_pos
 * minus 1 */
static uint32_t wait_enter_nmem_pos, wait_exit_nmem_pos;

static INLINE uint8_t *next_msg(uint8_t *msg, uint32_t size)
{
   msg += size;
   return (msg == (msgs + MSGS_SIZE)) ? msgs : msg;
}

static INLINE void advance_msg(uint8_t **msg, uint32_t size)
{
   khrn_barrier();
   *msg = next_msg(*msg, size);
}

static INLINE bool room_for_msg(uint8_t *msg_a, uint32_t size, uint8_t *msg_b)
{
   if (msg_b == msgs) { msg_b = msgs + MSGS_SIZE; }
   if ((msg_b <= msg_a) || (msg_b > (msg_a + size))) {
      khrn_barrier();
      return true;
   }
   return false;
}

static INLINE bool more_msgs(uint8_t *msg_a, uint8_t *msg_b)
{
   if (msg_a != msg_b) {
      khrn_barrier();
      return true;
   }
   return false;
}

static INLINE uint32_t get_msg_size_max(uint8_t *msg)
{
   return (msgs + MSGS_SIZE) - msg;
}

/******************************************************************************
message posting (this stuff should only be called from the master task)
******************************************************************************/

static void post_nop(uint32_t size);

static void *post_begin(MSG_TYPE_T type, uint32_t size)
{
   uint32_t size_max;
   MSG_HEADER_T *h;

   size += sizeof(MSG_HEADER_T);
   vcos_assert(!(size & 0x3)); /* everything should by 4-byte aligned */

   size_max = get_msg_size_max(vg_tess_msg.post);
   if (size > size_max) {
      post_nop(size_max);
      size_max = get_msg_size_max(vg_tess_msg.post);
      vcos_assert(size <= size_max);
   }

   while (!room_for_msg(vg_tess_msg.post, size, vg_tess_msg.done)) {
      khrn_sync_master_wait();
   }

   h = (MSG_HEADER_T *)vg_tess_msg.post;
   h->type = type;
   h->size = (uint16_t)size;
   vcos_assert(h->size == size);
   return h + 1;
}

static void post_end(void)
{
   advance_msg(&vg_tess_msg.post, ((MSG_HEADER_T *)vg_tess_msg.post)->size);
   /* the caller will notify llat if necessary */
}

static void post_nop(uint32_t size)
{
   verify(post_begin(MSG_TYPE_NOP, size - sizeof(MSG_HEADER_T)));
   post_end();
}

static void post_wait(TESS_T *tess, void (*callback)(bool, void *), void *p)
{
   MSG_WAIT_T *msg = (MSG_WAIT_T *)post_begin(MSG_TYPE_WAIT, sizeof(MSG_WAIT_T));
   msg->tess = tess;
   msg->callback = callback;
   msg->p = p;
   post_end();
}

static void post_fill_stroke(TESS_T *tess,
#ifdef VG_TESS_QPU
   uint32_t shader,
#else
   bool (*f)(const IO_CHUNK_T *io_chunk, REP_T *r),
#endif
   IO_CHUNK_T *io_chunk, uint32_t io_chunks_n,
   MEM_HANDLE_T chunks_a_handle, MEM_HANDLE_T chunks_b_handle,
   REP_T *r)
{
   MSG_FILL_STROKE_T *msg = (MSG_FILL_STROKE_T *)post_begin(MSG_TYPE_FILL_STROKE, sizeof(MSG_FILL_STROKE_T));
   msg->tess = tess;
#ifdef VG_TESS_QPU
   msg->shader = shader;
#else
   msg->f = f;
#endif
   msg->io_chunk = io_chunk;
   msg->io_chunks_n = io_chunks_n;
   mem_acquire(chunks_a_handle);
   mem_retain(chunks_a_handle);
   msg->chunks_a = chunks_a_handle;
   if (chunks_b_handle != MEM_INVALID_HANDLE) {
      mem_acquire(chunks_b_handle);
      mem_retain(chunks_b_handle);
   }
   msg->chunks_b = chunks_b_handle;
   msg->r = r;
   post_end();
}

#ifndef VG_TESS_QPU
static void post_rep(TESS_T *tess, REP_T *r,
   bool fill,
   uint8_t *branch)
{
   MSG_REP_T *msg = (MSG_REP_T *)post_begin(MSG_TYPE_REP, sizeof(MSG_REP_T));
   msg->tess = tess;
   msg->r = r;
   msg->fill = fill;

   msg->branch = branch;
   post_end();
}

static void post_fill_bbox(TESS_T *tess, REP_T *r,
   uint8_t *rect)
{
   MSG_FILL_BBOX_T *msg = (MSG_FILL_BBOX_T *)post_begin(MSG_TYPE_FILL_BBOX, sizeof(MSG_FILL_BBOX_T));
   msg->tess = tess;
   msg->r = r;
   msg->rect = rect;
   post_end();
}
#endif

/******************************************************************************
init/term
******************************************************************************/

#define NMEM_RESERVATION 2 /* todo: what should this be? */

#if defined(VG_TESS_QPU) && !defined(SIMPENROSE)
static void acquire_callback(void *p);
static void vg_tess_isr(uint32_t flags);
#endif
static void vg_tess_llat_callback(void);

/* initial block of fixed memory that is used for:
 * - static things that are used by the hardware and so need to be in the heap
 *   (colorspace luts, clear shader, etc)
 * - every TESS_T needs a pool of REP_Ts. the pool is initially populated with
 *   REP_Ts from this block (it is replenished with REP_Ts allocated during
 *   tessellation)
 */
static void *stuff;

#ifdef VG_TESS_QPU
static uint32_t init_shader, term_shader, fill_shader, stroke_shader;
static uint32_t io_chunk_unifs;
static uint32_t *shader_if;

#ifndef SIMPENROSE
static DRIVER_HANDLE_T v3d_driver_handle;
#endif
#endif

static uint32_t llat_i;

static void nmem_callback(void *p)
{
   UNUSED(p);
   khrn_llat_notify(llat_i);
}

void *vg_tess_init(uint32_t size)
{
#ifdef VG_TESS_QPU
   void *stuff_alias;
#endif
   uint32_t i;
   REP_T *r;

   stuff = khrn_nmem_alloc_master(false);
   if (!stuff) {
      return NULL;
   }
   vcos_assert(!((uintptr_t)stuff & 4095));

   /* we give the first size bytes of the block away to our caller. the rest is
    * used for:
    * - shader interface memory, shaders, uniforms (if we're doing tessellation on the qpus)
    * - IO_CHUNK_Ts
    * - message fifo
    * - REP_Ts */

#ifdef VG_TESS_QPU
   size = round_up(size, _max(8, KHRN_HW_SYSTEM_CACHE_LINE_SIZE));
   khrn_hw_invalidate_dcache_range((uint8_t *)stuff + size,
      round_up(SHADER_IF_SIZE, 8) +
      VG_TESS_INIT_SHADER_SIZE + VG_TESS_TERM_SHADER_SIZE +
      VG_TESS_FILL_SHADER_SIZE + VG_TESS_STROKE_SHADER_SIZE +
      (IO_CHUNKS_N * sizeof(IO_CHUNK_T)) +
      ((IO_CHUNKS_N + IO_CHUNKS_EXTRA) * 4));
   stuff_alias = khrn_hw_alias_direct(stuff);

   shader_if = (uint32_t *)((uint8_t *)stuff_alias + size);
   shader_if[0] = -2;
   memset(shader_if + 1, 0, SHADER_IF_SIZE - 4);
   size += round_up(SHADER_IF_SIZE, 8);

   init_shader = khrn_hw_addr(stuff_alias) + size + VG_TESS_INIT_SHADER_OFFSET;
   vg_tess_init_shader_link((uint8_t *)stuff_alias + size, khrn_hw_addr(stuff_alias) + size,
      khrn_hw_addr(shader_if));
   size += VG_TESS_INIT_SHADER_SIZE;

   term_shader = khrn_hw_addr(stuff_alias) + size + VG_TESS_TERM_SHADER_OFFSET;
   vg_tess_term_shader_link((uint8_t *)stuff_alias + size, khrn_hw_addr(stuff_alias) + size,
      khrn_hw_addr(shader_if));
   size += VG_TESS_TERM_SHADER_SIZE;

   fill_shader = khrn_hw_addr(stuff_alias) + size + VG_TESS_FILL_SHADER_OFFSET;
   vg_tess_fill_shader_link((uint8_t *)stuff_alias + size, khrn_hw_addr(stuff_alias) + size,
      khrn_hw_addr(shader_if));
   size += VG_TESS_FILL_SHADER_SIZE;

   stroke_shader = khrn_hw_addr(stuff_alias) + size + VG_TESS_STROKE_SHADER_OFFSET;
   vg_tess_stroke_shader_link((uint8_t *)stuff_alias + size, khrn_hw_addr(stuff_alias) + size,
      khrn_hw_addr(shader_if));
   size += VG_TESS_STROKE_SHADER_SIZE;

   io_chunks = (IO_CHUNK_T *)((uint8_t *)stuff_alias + size);
   size += IO_CHUNKS_N * sizeof(IO_CHUNK_T);

   io_chunk_unifs = khrn_hw_addr(stuff_alias) + size;
   for (i = 0; i != (IO_CHUNKS_N + IO_CHUNKS_EXTRA); ++i) {
      *(uint32_t *)((uint8_t *)stuff_alias + size) = khrn_hw_addr(io_chunks + (i % IO_CHUNKS_N));
      size += 4;
   }

   size = round_up(size, _max(4, KHRN_HW_SYSTEM_CACHE_LINE_SIZE));
#else
   size = round_up(size, 4);

   io_chunks = (IO_CHUNK_T *)((uint8_t *)stuff + size);
   size += IO_CHUNKS_N * sizeof(IO_CHUNK_T);
#endif

   msgs = (uint8_t *)stuff + size;
   size += MSGS_SIZE;

   r = (REP_T *)((uint8_t *)stuff + size);
   vcos_assert(size <= KHRN_NMEM_BLOCK_SIZE);
   tess_reps_n = ((KHRN_NMEM_BLOCK_SIZE - size) / TESSS_N_MAX) / sizeof(REP_T);
   vcos_assert(tess_reps_n > 0);
   for (i = 0; i != TESSS_N_MAX; ++i) {
      tesss[i].reps = r;
      r += tess_reps_n;

#ifdef VG_TESS_QPU
      tesss[i].flipbit = false;
#endif
   }

   tesss_n = 0;
   for (i = 0; i != (TESSS_N_MAX - 1); ++i) {
      tesss[i].u.free_next = tess_to_handle(tesss + i + 1);
   }
   tesss[TESSS_N_MAX - 1].u.free_next = VG_TESS_HANDLE_INVALID;
   tesss_free_head = tess_to_handle(tesss + 0);

   io_chunk_alloc = io_chunks;
   io_chunk_free = io_chunks;

   vg_tess_msg.post = msgs;
   vg_tess_msg.submit = msgs;
   vg_tess_msg.done = msgs;

   wait_enter_nmem_pos = 0;
   wait_exit_nmem_pos = 0;

#if defined(VG_TESS_QPU) && !defined(SIMPENROSE)
   {
      V3D_PARAMS_T params = {acquire_callback, NULL, vg_tess_isr};
      verify(v3d_get_func_table()->open(&params, &v3d_driver_handle) == 0);
   }
#endif

   llat_i = khrn_llat_register(vg_tess_llat_callback);
   vcos_assert(llat_i != -1);

   khrn_nmem_register(nmem_callback, NULL);
   khrn_nmem_reserve(NMEM_RESERVATION);

   return stuff;
}

void vg_tess_term(void)
{
   khrn_nmem_reserve(-NMEM_RESERVATION);
   khrn_nmem_unregister(nmem_callback, NULL);
   khrn_llat_wait(); /* make sure there are no spurious notifies hanging around (noone should be posting them now) */
   khrn_llat_unregister(llat_i);
#if defined(VG_TESS_QPU) && !defined(SIMPENROSE)
   verify(v3d_get_func_table()->close(v3d_driver_handle) == 0);
#endif
   vcos_assert(wait_enter_nmem_pos == wait_exit_nmem_pos);
   vcos_assert(vg_tess_msg.submit == vg_tess_msg.post);
   vcos_assert(vg_tess_msg.done == vg_tess_msg.post);
   vcos_assert(io_chunk_free == io_chunk_alloc);
   vcos_assert(tesss_n == 0);
   khrn_nmem_free(stuff, false);
}

/******************************************************************************
clif dumping
******************************************************************************/

#ifdef VG_TESS_DUMP_CLIF

static void dump_clif_init(DUMP_CLIF_T *dc, bool flipbit)
{
   static uint32_t i = 0;
   char filename[64];
   sprintf(filename, "tess_%u.clif", i++);
   dc->f = fopen(filename, "w");
   vcos_assert(dc->f);
   dc->i = -1;

   fprintf(dc->f,
      "@test_type user\n"
      "\n"
      "@create_output_buf_aligned 4096 output\n"
      "@format blank\n"
      "   %u\n"
      "\n"
      "@createbuf_aligned 4 vg_tess_shader_if\n"
      "@format binary\n"
      "   0x00000000\n"
      "   [output + %u]\n"
      "\n"
      "@createbuf_aligned 8 shaders\n"
      "@format binary\n"
      "%%.set VG_TESS_RTL_SIM, 1\n"
      "%%.set VG_TESS_SEMA, 1\n"
      "%%.set VG_TESS_NO_INT, 1\n"
      "%%.include \"vg_tess_fill_4.qasm\"\n"
      "# dump\n"
      "%%.include \"vg_tess_stroke_4.qasm\"\n"
      "@label annots\n"
      "%%@1\n"
      "%%.include \"vg_tess_fill_4.qasm\"\n"
      "# dump\n"
      "%%.include \"vg_tess_stroke_4.qasm\"\n"
      "%%@2,annots\n"
      "%%.include \"vg_tess_fill_4.qasm\"\n"
      "# dump\n"
      "%%.include \"vg_tess_stroke_4.qasm\"\n"
      "%%@0\n"
      "\n"
      "@createbuf_aligned 8 clear_shader\n"
      "@format binary\n"
      "%%[\n"
      "   .include \"vg_util_4.qinc\"\n"
      "   mov  vw_setup, vpm_setup(64, 1, h32(0))\n"
      "   .rep i, 64\n"
      "      mov  vpm, 0\n"
      "   .endr\n"
      "   empty_vpm_fifo\n"
      "   .rep i, 15\n"
      "      srel  -, 0 # fill/stroke shaders will be stalled at start until we get here\n"
      "   .endr\n"
      "   thrend\n"
      "   nop\n"
      "   nop\n"
      "%%]\n"
      "\n"
      "@add_user_shader [clear_shader] [clear_shader]\n",
      #define SIZE 524288
      SIZE, ((SIZE / SUBBLOCK_SIZE) << 1) | flipbit);
      #undef SIZE
}

static void dump_clif_term(DUMP_CLIF_T *dc)
{
   fclose(dc->f);
}

static uint32_t dump_clif_chunks(DUMP_CLIF_T *dc, uint32_t chunks, uint32_t chunks_size)
{
   uint8_t *chunk = (uint8_t *)khrn_hw_unaddr(chunks), *chunk_end = chunk + chunks_size;
   uint32_t i;

   fprintf(dc->f, "\n"
      "@createbuf_aligned 4 chunks_%08x\n"
      "@format binary\n",
      ++dc->i);
   while (chunk != chunk_end) {
      if (*chunk == 0) {
         VG_TESS_CHUNK_CUBIC_T *c = (VG_TESS_CHUNK_CUBIC_T *)chunk;
         chunk += sizeof(VG_TESS_CHUNK_CUBIC_T);
         fprintf(dc->f,
            "   // cubic chunk\n"
            "   0x%02x 0x%02x 0x%02x 0x%02x // n, subpath_first, subpath_last, dummy\n"
            "   ", c->n, c->subpath_first, c->subpath_last, c->dummy);
         for (i = 0; i != 8; ++i) { fprintf(dc->f, "0x%08x ", float_to_bits(c->p[i])); }
         fprintf(dc->f, "// p\n"
            "   0x%08x // magic\n",
            c->magic);
      } else {
         VG_TESS_CHUNK_NORMAL_T *c = (VG_TESS_CHUNK_NORMAL_T *)chunk;
         chunk += sizeof(VG_TESS_CHUNK_NORMAL_T);
         fprintf(dc->f,
            "   // normal chunk\n"
            "   0x%02x 0x%02x 0x%04x // n, subpath_last_implicit, dummy\n"
            "   ", c->n, c->subpath_last_implicit, c->dummy);
         for (i = 0; i != 7; ++i) { fprintf(dc->f, "0x%08x ", float_to_bits(c->xs[i])); }
         fprintf(dc->f, "// xs\n"
            "   ");
         for (i = 0; i != 7; ++i) { fprintf(dc->f, "0x%08x ", float_to_bits(c->ys[i])); }
         fprintf(dc->f, "// ys\n"
            "   0x%02x 0x%02x 0x%02x 0x%02x // subpath_first, subpath_last, close, explicit_n\n",
            c->subpath_first, c->subpath_last, c->close, c->explicit_n);
      }
   }

   return dc->i;
}

static uint32_t dump_clif_io_chunk_i(DUMP_CLIF_T *dc, uint32_t io_chunk)
{
   IO_CHUNK_I_T *c = (IO_CHUNK_I_T *)khrn_hw_unaddr(io_chunk);
   uint32_t chunks_a = dump_clif_chunks(dc, c->chunks_a, c->chunks_size);
   uint32_t chunks_b = c->chunks_b ? dump_clif_chunks(dc, c->chunks_b, c->chunks_size) : 0;
   uint32_t i;

   fprintf(dc->f, "\n"
      "@createbuf_aligned 4 io_chunk_i_%08x\n"
      "@format binary\n"
      "   0x%08x // tess_i\n"
      "   [chunks_%08x] // chunks_a\n",
      ++dc->i,
      c->tess_i, chunks_a);
   if (c->chunks_b) {
      fprintf(dc->f,
         "   [chunks_%08x] // chunks_b\n",
         chunks_b);
   } else {
      fprintf(dc->f,
         "   0x00000000 // chunks_b\n");
   }
   fprintf(dc->f,
      "   0x%08x // chunks_size\n"
      "   0x%08x // t\n"
      "   ", c->chunks_size, float_to_bits(c->t));
   for (i = 0; i != 6; ++i) { fprintf(dc->f, "0x%08x ", float_to_bits(c->user_to_surface[i])); }
   fprintf(dc->f, "// user_to_surface\n"
      "   ");
   for (i = 0; i != 4; ++i) { fprintf(dc->f, "0x%08x ", float_to_bits(c->clip[i])); }
   fprintf(dc->f, "// clip\n"
      "   0x%08x // flipbit\n",
      c->flipbit);

   return dc->i;
}

static uint32_t dump_clif_io_chunk_stroke_i2(DUMP_CLIF_T *dc, uint32_t io_chunk)
{
   IO_CHUNK_STROKE_I2_T *c = (IO_CHUNK_STROKE_I2_T *)khrn_hw_unaddr(io_chunk);
   uint32_t i;

   fprintf(dc->f, "\n"
      "@createbuf_aligned 4 io_chunk_stroke_i2_%08x\n"
      "@format binary\n"
      "   0x%08x // line_width\n"
      "   ",
      ++dc->i,
      float_to_bits(c->line_width));
   for (i = 0; i != 4; ++i) { fprintf(dc->f, "0x%08x ", float_to_bits(c->clip_outer[i])); }
   fprintf(dc->f, "// clip_outer\n"
      "   0x%08x // miter_limit\n"
      "   0x%08x // oo_theta\n"
      "   0x%08x // cos_theta\n"
      "   0x%08x // cos_theta_inter\n"
      "   0x%08x // sin_theta\n"
      "   0x%08x // oo_flatness_sq\n"
      "   0x%08x // width_hack_clerp\n"
      "   0x%08x // width_hack_round\n"
      "   0x%08x // cap/join/dashing/dash_pattern_n/dash_phase_i/dash_phase_reset\n"
      "   0x%08x // dash_oo_scale\n"
      "   0x%08x // dash_phase\n",
      float_to_bits(c->miter_limit), float_to_bits(c->oo_theta),
      float_to_bits(c->cos_theta), float_to_bits(c->cos_theta_inter),
      float_to_bits(c->sin_theta), float_to_bits(c->oo_flatness_sq),
      float_to_bits(c->width_hack_clerp), float_to_bits(c->width_hack_round),
      float_to_bits((&c->width_hack_round)[1]), /* todo: this is an ugly hack */
      float_to_bits(c->dash_oo_scale), float_to_bits(c->dash_phase));

   return dc->i;
}

static uint32_t dump_clif_io_chunk_stroke_i3(DUMP_CLIF_T *dc, uint32_t io_chunk)
{
   IO_CHUNK_STROKE_I3_T *c = (IO_CHUNK_STROKE_I3_T *)khrn_hw_unaddr(io_chunk);
   uint32_t i;

   fprintf(dc->f, "\n"
      "@createbuf_aligned 4 io_chunk_stroke_i3_%08x\n"
      "@format binary\n"
      "   ",
      ++dc->i);
   for (i = 0; i != 16; ++i) { fprintf(dc->f, "0x%08x ", float_to_bits(c->dash_pattern[i])); }
   fprintf(dc->f, "// dash_pattern\n");

   return dc->i;
}

static uint32_t dump_clif_io_chunk_unifs(DUMP_CLIF_T *dc, uint32_t unifs, uint32_t unifs_n)
{
   uint32_t *u = (uint32_t *)khrn_hw_unaddr(unifs);
   uint32_t io_chunk_i = dump_clif_io_chunk_i(dc, u[0]);
   uint32_t io_chunk_stroke_i2 = (unifs_n >= 2) ? dump_clif_io_chunk_stroke_i2(dc, u[1]) : 0;
   uint32_t io_chunk_stroke_i3 = (unifs_n >= 3) ? dump_clif_io_chunk_stroke_i3(dc, u[2]) : 0;

   fprintf(dc->f, "\n"
      "@createbuf_aligned 4 io_chunk_unifs_%08x\n"
      "@format binary\n"
      "   [io_chunk_i_%08x]\n",
      ++dc->i,
      io_chunk_i);
   if (unifs_n >= 2) {
      fprintf(dc->f,
         "   [io_chunk_stroke_i2_%08x]\n",
         io_chunk_stroke_i2);
   }
   if (unifs_n >= 3) {
      fprintf(dc->f,
         "   [io_chunk_stroke_i3_%08x]\n",
         io_chunk_stroke_i3);
   }

   return dc->i;
}

static void dump_clif_add_shader(DUMP_CLIF_T *dc, uint32_t shader, uint32_t unifs, uint32_t unifs_n)
{
   uint32_t io_chunk_unifs = dump_clif_io_chunk_unifs(dc, unifs, unifs_n);

   fprintf(dc->f, "\n"
      "@add_user_shader [vg_tess_%s_shader] [io_chunk_unifs_%08x] [vg_tess_%s_shader_annotations]\n",
      (shader == fill_shader) ? "fill" : "stroke", io_chunk_unifs,
      (shader == fill_shader) ? "fill" : "stroke");
}

#endif

/******************************************************************************
start/finish/nmem_enter/wait/fix/get_nmem_n
******************************************************************************/

VG_TESS_HANDLE_T vg_tess_start(void)
{
   VG_TESS_HANDLE_T tess_handle;
   TESS_T *tess;
   uint32_t i;

   while ((tess_handle = alloc_tess_handle()) == VG_TESS_HANDLE_INVALID) {
      /* TESSS_N_MAX >= number of render states, and we have at most one tess
       * per render state. the other tesss (those not tied to render states)
       * should all finish given time */
      khrn_sync_master_wait();
   }

   tess = tess_from_handle(tess_handle);
   tess->u.in_use.ok = true;
#ifdef VG_TESS_QPU
   tess->u.in_use.flipped_bit = false;
#endif
   tess->u.in_use.state = TESS_STATE_STARTED;
   for (i = 0; i != (tess_reps_n - 1); ++i) {
      tess->reps[i].free_next = tess->reps + i + 1;
   }
   tess->reps[tess_reps_n - 1].free_next = NULL;
   tess->u.in_use.r_head = tess->reps;
   tess->u.in_use.r_tail = tess->reps + (tess_reps_n - 1);
   khrn_nmem_group_init(&tess->u.in_use.nmem_group, false);
   tess->u.in_use.nmem_entered = false;
#ifndef VG_TESS_QPU
   allocator_init(&tess->u.in_use.v_allocator);
   allocator_init(&tess->u.in_use.prim_list_allocator);
   tess->u.in_use.f_head = NULL;
#endif
#ifdef VG_TESS_DUMP_CLIF
   dump_clif_init(&tess->u.in_use.dc, !tess->flipbit); /* haven't flipped flipbit yet! */
#endif

   return tess_handle;
}

void vg_tess_finish_llat(VG_TESS_HANDLE_T tess_handle)
{
   TESS_T *tess = tess_from_handle(tess_handle);

   vcos_assert(tess->u.in_use.state == TESS_STATE_WAIT_DONE);
   tess->u.in_use.state = TESS_STATE_FINISH_LLAT_DONE;

   khrn_nmem_group_term(&tess->u.in_use.nmem_group);
}

void vg_tess_finish(VG_TESS_HANDLE_T tess_handle)
{
   TESS_T *tess = tess_from_handle(tess_handle);

   vcos_assert(tess->u.in_use.state == TESS_STATE_FINISH_LLAT_DONE);

#ifdef VG_TESS_DUMP_CLIF
   dump_clif_term(&tess->u.in_use.dc);
#endif

   free_tess_handle(tess_handle);
}

void vg_tess_nmem_enter(VG_TESS_HANDLE_T tess_handle)
{
   TESS_T *tess = tess_from_handle(tess_handle);
   uint32_t pos = khrn_nmem_get_enter_pos();

   vcos_assert(tess->u.in_use.state == TESS_STATE_STARTED);

   tess->u.in_use.nmem_entered = true;
   tess->u.in_use.nmem_pos = pos;

   if (wait_exit_nmem_pos == wait_enter_nmem_pos) {
      wait_exit_nmem_pos = pos - 1;
      khrn_barrier();
   }
   wait_enter_nmem_pos = pos;
}

void vg_tess_wait(VG_TESS_HANDLE_T tess_handle, void (*callback)(bool, void *), void *p)
{
   TESS_T *tess = tess_from_handle(tess_handle);

   vcos_assert(tess->u.in_use.state == TESS_STATE_STARTED);
   tess->u.in_use.state = TESS_STATE_WAIT_CALLED;

   post_wait(tess, callback, p);
   khrn_llat_notify(llat_i);
#ifdef SIMPENROSE
   khrn_llat_process();
#endif
}

void vg_tess_fix(VG_TESS_HANDLE_T tess_handle)
{
   TESS_T *tess = tess_from_handle(tess_handle);

   vcos_assert(tess->u.in_use.ok && (tess->u.in_use.state == TESS_STATE_WAIT_DONE));

#ifndef VG_TESS_QPU
   fix(tess->u.in_use.f_head);
#endif
}

uint32_t vg_tess_get_nmem_n(VG_TESS_HANDLE_T tess_handle)
{
   /* todo: n may be being updated asynchronously on another thread, but that's
    * probably ok... */
   return tess_from_handle(tess_handle)->u.in_use.nmem_group.n;
}

/******************************************************************************
fill/stroke
******************************************************************************/

#define FLATNESS 0.25f /* todo: tweak this. check actual flatness of generated stuff */
#define FLATNESS_SQ (FLATNESS * FLATNESS)
#define OO_FLATNESS_SQ (1.0f / FLATNESS_SQ)

#define STROKE_COS_THETA_O2_MIN 0.9238795325112870f /* theta ~45 degrees */
#define STROKE_COS_THETA_O2_MAX 0.9990482220593760f /* theta ~5 degrees (ri uses this) */
#define STROKE_COS_THETA_INTER_MIN (1.0f / SQRT_2) /* theta ~45 degrees */
#define STROKE_COS_THETA_INTER_MAX 0.99939082f     /* theta ~2 degrees */

#ifndef VG_TESS_QPU
static bool do_fill(const IO_CHUNK_T *io_chunk, REP_T *r);
static bool do_stroke(const IO_CHUNK_T *io_chunk, REP_T *r);
#endif


static bool rep(TESS_T *tess,
   KHRN_FMEM_T *fmem,
   REP_T *r, bool fill)
{
   uint8_t *p;

   p = rep_placehold(fmem);
   if (!p) { return false; }
#ifdef VG_TESS_QPU
   UNUSED(fill);
   /* add the branch to the indirection placeholder */
   Add_byte(p, KHRN_HW_INSTR_BRANCH_SUB);
   ADD_WORD(p, khrn_hw_addr(khrn_hw_alias_direct(r->in_use.ind_branch_dest)));
#else
   post_rep(tess, r, fill, p);
#endif
   return true;
}

void *vg_tess_fill(VG_TESS_HANDLE_T tess_handle,
   KHRN_FMEM_T *fmem,
   MEM_HANDLE_T chunks_a_handle, MEM_HANDLE_T chunks_b_handle, float t,
   const VG_MAT3X3_T *user_to_surface,
   const float *clip)
{
   TESS_T *tess = tess_from_handle(tess_handle);
   REP_T *r;
   IO_CHUNK_T *io_chunk;

   vcos_assert(tess->u.in_use.state == TESS_STATE_STARTED);

   r = alloc_rep(tess, fmem);
   if (!r || !tess->u.in_use.ok) {
      return NULL;
   }

   io_chunk = alloc_io_chunk();
   /* tess/chunks_a/chunks_b/chunks_size filled in later */
   io_chunk->i.t = t;
   memcpy(io_chunk->i.user_to_surface, user_to_surface, sizeof(io_chunk->i.user_to_surface));
   io_chunk->i.user_to_surface[2] += BIAS;
   io_chunk->i.user_to_surface[5] += BIAS;
   memcpy(io_chunk->i.clip, clip, sizeof(io_chunk->i.clip));
   /* (VG_TESS_QPU) flipbit filled in later */

   post_fill_stroke(tess,
#ifdef VG_TESS_QPU
      fill_shader,
#else
      do_fill,
#endif
      io_chunk, 1, chunks_a_handle, chunks_b_handle, r);
   if (!rep(tess, fmem, r, true)) {
      r = NULL;
   }
   khrn_llat_notify(llat_i);
   return r;
}

bool vg_tess_fill_rep(VG_TESS_HANDLE_T tess_handle,
   KHRN_FMEM_T *fmem,
   void *fill_rep)
{
   TESS_T *tess = tess_from_handle(tess_handle);

   vcos_assert(tess->u.in_use.state == TESS_STATE_STARTED);

   if (!tess->u.in_use.ok ||
      !rep(tess, fmem, (REP_T *)fill_rep, true)) {
      return false;
   }
#ifndef VG_TESS_QPU
   khrn_llat_notify(llat_i);
#endif
   return true;
}

bool vg_tess_fill_bbox(VG_TESS_HANDLE_T tess_handle,
   KHRN_FMEM_T *fmem,
   void *fill_rep)
{
   uint8_t *rect;
   TESS_T *tess = tess_from_handle(tess_handle);

   vcos_assert(tess->u.in_use.state == TESS_STATE_STARTED);

   if (!tess->u.in_use.ok) return false;
#ifdef VG_TESS_QPU
   rect = rep_placehold(fmem);
#else
   rect = rect_placehold(fmem);
#endif
   if (!rect) {
      return false;
   }
#ifdef VG_TESS_QPU
   Add_byte(rect, KHRN_HW_INSTR_BRANCH_SUB);
   ADD_WORD(rect, khrn_hw_addr(khrn_hw_alias_direct(((REP_T *)(fill_rep))->in_use.ind_branch_dest+5)));
#else
   post_fill_bbox(tess, (REP_T *)fill_rep, rect);
   khrn_llat_notify(llat_i);
#endif
   return true;
}

static INLINE CAP_T convert_cap_style(VGCapStyle cap_style)
{
   vcos_assert((cap_style >= VG_CAP_BUTT) && (cap_style <= VG_CAP_SQUARE));
   return (CAP_T)(CAP_BUTT + (cap_style - VG_CAP_BUTT));
}

static INLINE JOIN_T convert_join_style(VGJoinStyle join_style)
{
   vcos_assert((join_style >= VG_JOIN_MITER) && (join_style <= VG_JOIN_BEVEL));
   return (JOIN_T)(JOIN_MITER + (join_style - VG_JOIN_MITER));
}

void *vg_tess_stroke(VG_TESS_HANDLE_T tess_handle,
   KHRN_FMEM_T *fmem,
   MEM_HANDLE_T chunks_a_handle, MEM_HANDLE_T chunks_b_handle, float t,
   const float *dash_pattern, uint32_t dash_pattern_n, float dash_phase, bool dash_phase_reset,
   float line_width, VGCapStyle cap_style, VGJoinStyle join_style, float miter_limit,
   const VG_MAT3X3_T *user_to_surface, float scale_max,
   const float *clip_inner, const float *clip_outer)
{
   TESS_T *tess = tess_from_handle(tess_handle);
   REP_T *r;
   float flatness, oo_flatness_sq, rad, oo_rad, cos_theta_o2, cos_theta_inter,
      theta, oo_theta, sin_theta, cos_theta, width_hack_clerp, width_hack_round;
   bool dashing;
   VG_TESS_STROKE_DASH_T dash;
   IO_CHUNK_T *io_chunk, *io_chunk2, *io_chunk3;
   uint32_t i;

   vcos_assert(tess->u.in_use.state == TESS_STATE_STARTED);

   r = alloc_rep(tess, fmem);
   if (!r || !tess->u.in_use.ok) {
      return NULL;
   }

   /* derive useful values... */
   flatness = FLATNESS * ((scale_max == 0.0f) ? FLT_MAX : recip_(scale_max));
   oo_flatness_sq = scale_max * scale_max * OO_FLATNESS_SQ;
   rad = line_width * 0.5f;
   oo_rad = recip_(line_width) * 2.0f;
   cos_theta_o2 = clampf((rad - flatness) * oo_rad, STROKE_COS_THETA_O2_MIN, STROKE_COS_THETA_O2_MAX);
   cos_theta_inter = clampf(sqrt_(_maxf((rad * rad) - (flatness * flatness), 0.0f)) * oo_rad, STROKE_COS_THETA_INTER_MIN, STROKE_COS_THETA_INTER_MAX);
   theta = acos_(cos_theta_o2) * 2.0f;
   oo_theta = recip_(theta);
   sin_cos_(&sin_theta, &cos_theta, theta);
   /* we offset the vertices outward by some fraction of the flatness to improve the actual flatness along outer edges */
   width_hack_clerp = 1.0f + (0.165f * (1.0f - cos_theta_o2)); /* lower values will result in less bad inside edges. higher values will result in better outside edges. 0.165 pleases the cts */
   width_hack_round = 1.0f + (0.7f * (1.0f - cos_theta_o2)); /* 0.7 seems to give good results (we only have an outside edge in this case, thus the higher value) */
   dashing = vg_tess_stroke_dash_prepare(&dash, dash_pattern, dash_pattern_n, dash_phase, dash_phase_reset);

   io_chunk = alloc_io_chunk();
   /* tess/chunks_a/chunks_b/chunks_size filled in later */
   io_chunk->i.t = t;
   memcpy(io_chunk->i.user_to_surface, user_to_surface, sizeof(io_chunk->i.user_to_surface));
   io_chunk->i.user_to_surface[2] += BIAS;
   io_chunk->i.user_to_surface[5] += BIAS;
   memcpy(io_chunk->i.clip, clip_inner, sizeof(io_chunk->i.clip));
   /* (VG_TESS_QPU) flipbit filled in later */

   io_chunk2 = alloc_io_chunk();
   io_chunk2->stroke_i2.line_width = line_width;
   memcpy(io_chunk2->stroke_i2.clip_outer, clip_outer, sizeof(io_chunk2->stroke_i2.clip_outer));
   io_chunk2->stroke_i2.miter_limit = miter_limit;
   io_chunk2->stroke_i2.oo_theta = oo_theta;
   io_chunk2->stroke_i2.cos_theta = cos_theta;
   io_chunk2->stroke_i2.cos_theta_inter = cos_theta_inter;
   io_chunk2->stroke_i2.sin_theta = sin_theta;
   io_chunk2->stroke_i2.oo_flatness_sq = oo_flatness_sq;
   io_chunk2->stroke_i2.width_hack_clerp = width_hack_clerp;
   io_chunk2->stroke_i2.width_hack_round = width_hack_round;
   io_chunk2->stroke_i2.cap = convert_cap_style(cap_style);
   io_chunk2->stroke_i2.join = convert_join_style(join_style);
   io_chunk2->stroke_i2.dashing = dashing;
   if (dashing) {
      io_chunk2->stroke_i2.dash_pattern_n = dash.pattern_count;
      io_chunk2->stroke_i2.dash_phase_i = dash.phase_i;
      io_chunk2->stroke_i2.dash_phase_reset = dash.phase_reset;
      io_chunk2->stroke_i2.dash_oo_scale = dash.oo_scale;
      io_chunk2->stroke_i2.dash_phase = dash.phase;

      io_chunk3 = alloc_io_chunk();
      for (i = 0; i != dash.pattern_count; ++i) {
         io_chunk3->stroke_i3.dash_pattern[i] = dash.pattern[i];
      }
   } else {
      io_chunk2->stroke_i2.dash_phase_i = 0;
      io_chunk2->stroke_i2.dash_phase_reset = false; /* doesn't really matter */
   }

   post_fill_stroke(tess,
#ifdef VG_TESS_QPU
      stroke_shader,
#else
      do_stroke,
#endif
      io_chunk, dashing ? 3 : 2, chunks_a_handle, chunks_b_handle, r);
   if (!rep(tess, fmem, r, false)) {
      r = NULL;
   }
   khrn_llat_notify(llat_i);
   return r;
}

bool vg_tess_stroke_rep(VG_TESS_HANDLE_T tess_handle,
   KHRN_FMEM_T *fmem,
   void *stroke_rep)
{
   TESS_T *tess = tess_from_handle(tess_handle);

   vcos_assert(tess->u.in_use.state == TESS_STATE_STARTED);

   if (!tess->u.in_use.ok ||
      !rep(tess, fmem, (REP_T *)stroke_rep, false)) {
      return false;
   }
#ifndef VG_TESS_QPU
   khrn_llat_notify(llat_i);
#endif
   return true;
}

/******************************************************************************
rep user stuff
******************************************************************************/

void vg_tess_rep_set_user(void *rep, float a, float b, float c, float d)
{
   REP_T *r = (REP_T *)rep;
   r->in_use.user_a = a;
   r->in_use.user_b = b;
   r->in_use.user_c = c;
   r->in_use.user_d = d;
}

void vg_tess_rep_get_user(void *rep, float *a, float *b, float *c, float *d)
{
   REP_T *r = (REP_T *)rep;
   *a = r->in_use.user_a;
   *b = r->in_use.user_b;
   *c = r->in_use.user_c;
   *d = r->in_use.user_d;
}

/******************************************************************************
actual tessellation
******************************************************************************/

#ifndef VG_TESS_QPU

/* todo: try other fill/stroke subd methods */

#define LEVEL_MAX 8 /* limit number of cubic segments to 256  */

static void set_list_length(ALLOCATOR_T *allocator, LIST_HEADER_T *h, bool fill)
{
   /* length = number of vertices = length in bytes / 4 bytes */
   h->len = ((uint8_t *)allocator->p - (uint8_t *)h->v) >> 2;
   vcos_assert(h->len >= 3); /* must have at least enough for one triangle */
   if (!fill) {
      vcos_assert((h->len % 3) == 0); /* strokes use triangle lists */
   } /* else: fills use triangle fans */
}

typedef struct {
   const IO_CHUNK_I_T *i;

   LIST_HEADER_T *h; /* header of current list */
   uint32_t n; /* how many lists in total? */

   bool in_subpath;

   int32_t min_x, min_y, max_x, max_y;
   int32_t subpath_sum_x, subpath_sum_y;
   uint32_t subpath_sum_n;

   /* clipping... */
   float x, y; /* most recent point */
   uint32_t cf, zone; /* clip flags / zone of most recent point */
   int32_t exit_edge, zone_delta; /* for adding corners on re-entry */

   jmp_buf error_jmp;
} FILL_T;

static void fill_begin(FILL_T *fill, const IO_CHUNK_T *io_chunk)
{
   fill->i = &io_chunk->i;
   fill->h = NULL;
   fill->n = 0;
   fill->in_subpath = false;
   fill->min_x = 0x7fffffff;
   fill->min_y = 0x7fffffff;
   fill->max_x = 0x80000000;
   fill->max_y = 0x80000000;
   fill->subpath_sum_x = 0;
   fill->subpath_sum_y = 0;
   fill->subpath_sum_n = 0;
}

static void fill_end(FILL_T *fill, REP_T *r)
{
   vcos_assert(fill->n != 0); /* should have output something. empty paths should be culled early (before vg_be_draw_path is called), and clipping should never eliminate all vertices */
   vcos_assert(!fill->in_subpath); /* should be directly preceded by fill_end_subpath */
   r->in_use.h = fill->h;
   r->in_use.n = fill->n;
   r->in_use.min_xy = (fill->min_x & 0xffff) | (fill->min_y << 16);
   r->in_use.max_xy = (fill->max_x & 0xffff) | (fill->max_y << 16);
}

static void fill_vert(FILL_T *fill, int32_t x, int32_t y)
{
   uint32_t xy;
   bool have_prev;
   uint32_t prev_xy = 0; /* avoid uninitialised warning */
   LIST_HEADER_T *h;
   uint32_t *p;

   fill->min_x = _min(fill->min_x, x);  fill->min_y = _min(fill->min_y, y);
   fill->max_x = _max(fill->max_x, x);  fill->max_y = _max(fill->max_y, y);
   fill->subpath_sum_x = _adds(fill->subpath_sum_x, x);
   fill->subpath_sum_y = _adds(fill->subpath_sum_y, y);
   ++fill->subpath_sum_n;
   xy = (x & 0xffff) | (y << 16);

   have_prev = fill->in_subpath;
   if (have_prev) {
      p = (uint32_t *)alloc(&fill->i->tess->u.in_use.v_allocator, sizeof(uint32_t), NULL);
      if (p) {
         *p = xy;
         return;
      }
      /* we have an active list, but we're going to have to start a new one because we're out of space */
      set_list_length(&fill->i->tess->u.in_use.v_allocator, fill->h, true);
      /* ok to start a new list */
      prev_xy = ((uint32_t *)fill->i->tess->u.in_use.v_allocator.p)[-1]; /* with this as the first coord */
   } /* else we don't have an active list yet */

   h = (LIST_HEADER_T *)alloc(&fill->i->tess->u.in_use.v_allocator, offsetof(LIST_HEADER_T, v) + (3 * sizeof(uint32_t)),
      &fill->i->tess->u.in_use.nmem_group);
   if (!h) { longjmp(fill->error_jmp, 1); }
   if (!have_prev) { unalloc(&fill->i->tess->u.in_use.v_allocator, sizeof(uint32_t)); } /* just wanted to make sure there would be enough room for at least one triangle */
   h->next = fill->h;
   /* we write h->len in set_list_length */
   fill->h = h;
   ++fill->n;
   fill->in_subpath = true;
   p = h->v; /* allocated room for verts after header */

   ADD_WORD((uint8_t*)p, FILL_CENTER_XY_UNWRITTEN); /* we'll write all the centers later */
   if (have_prev) {
      ADD_WORD((uint8_t*)p, prev_xy);
   }
   ADD_WORD((uint8_t*)p, xy);
   vcos_assert(p == fill->i->tess->u.in_use.v_allocator.p);
}

static void fill_end_subpath(FILL_T *fill)
{
   int32_t center_x, center_y;
   vcos_assert(fill->in_subpath);
   center_x = fill->subpath_sum_x / (int32_t)fill->subpath_sum_n;
   center_y = fill->subpath_sum_y / (int32_t)fill->subpath_sum_n;
   /* the center needs to be included in the bounding box as coverage without
    * fill can cause the hardware to screw up */
   fill->min_x = _min(fill->min_x, center_x);  fill->min_y = _min(fill->min_y, center_y);
   fill->max_x = _max(fill->max_x, center_x);  fill->max_y = _max(fill->max_y, center_y);
#ifdef VG_TESS_IDENTIFY_CENTERS
   verify(fill_vert(fill, center_x, center_y) &&
          fill_vert(fill, center_x + 48, center_y) &&
          fill_vert(fill, center_x, center_y + 48) &&
          fill_vert(fill, center_x, center_y - 48) &&
          fill_vert(fill, center_x - 48, center_y));
#endif
   set_list_length(&fill->i->tess->u.in_use.v_allocator, fill->h, true);
   fill->h->v[0] = (center_x & 0xffff) | (center_y << 16);
   fill->in_subpath = false;
   fill->subpath_sum_x = 0;
   fill->subpath_sum_y = 0;
   fill->subpath_sum_n = 0;
}

#define CF_LEFT   (1 << 0)
#define CF_BOTTOM (1 << 1)
#define CF_RIGHT  (1 << 2)
#define CF_TOP    (1 << 3)

static INLINE uint32_t get_cf_x(float x, const float *clip)
{
   return ((x < clip[0]) ? CF_LEFT : 0) | ((x > clip[2]) ? CF_RIGHT : 0);
}

static INLINE uint32_t get_cf_y(float y, const float *clip)
{
   return ((y < clip[1]) ? CF_BOTTOM : 0) | ((y > clip[3]) ? CF_TOP : 0);
}

static INLINE uint32_t get_cf(float x, float y, const float *clip)
{
   return get_cf_x(x, clip) | get_cf_y(y, clip);
}

static INLINE uint32_t cf_to_zone(uint32_t cf)
{
   /*
      7     6     5
         +-----+
      0  |     |  4
         |     |
         +-----+
      1     2     3
   */

   return (cf == (CF_LEFT | CF_TOP)) ? 7 : ((_msb(cf) << 1) - (_count(cf) >> 1));
}

/*
   p0 must be outside clip rect
   if line from p0 to p1 crosses clip rect: return true, set p0_x/p0_y to cross point, set cf0 to cross edge clip-flag
   otherwise: return false, set cf0 to partway clip-flags (so cf0 and cf1 have some bits in common)
*/

static bool clip_line(
   float *p0_x_io, float *p0_y_io, uint32_t *cf0_io,
   float p1_x, float p1_y, uint32_t cf1,
   const float *clip)
{
   float p0_x = *p0_x_io;
   float p0_y = *p0_y_io;
   uint32_t cf0 = *cf0_io;
   uint32_t cross_cf = 0;

   vcos_assert(cf0 == get_cf(p0_x, p0_y, clip));
   vcos_assert(cf1 == get_cf(p1_x, p1_y, clip));
   vcos_assert(cf0); /* p0 must be outside clip rect */

   if (cf0 & cf1) {
      return false;
   }

   if (cf0 & (CF_LEFT | CF_RIGHT)) {
      float x;
      cross_cf = cf0 & (CF_LEFT | CF_RIGHT);
      x = (cf0 & CF_LEFT) ? clip[0] : clip[2];
      p0_y += (p1_y - p0_y) * _minf((x - p0_x) * recip_(p1_x - p0_x), 1.0f);
      p0_x = x;
      cf0 = get_cf_y(p0_y, clip);
   }
   vcos_assert(!(cf0 & (CF_LEFT | CF_RIGHT)));

   if (cf0 & cf1) {
      *cf0_io = cf0;
      return false;
   }

   if (cf0 & (CF_BOTTOM | CF_TOP)) {
      float y;
      cross_cf = cf0 & (CF_BOTTOM | CF_TOP);
      y = (cf0 & CF_BOTTOM) ? clip[1] : clip[3];
      p0_x += (p1_x - p0_x) * _minf((y - p0_y) * recip_(p1_y - p0_y), 1.0f);
      p0_y = y;
      cf0 = get_cf_x(p0_x, clip);
   }
   vcos_assert(!(cf0 & (CF_BOTTOM | CF_TOP)));

   if (cf0 & cf1) {
      *cf0_io = cf0;
      return false;
   }

   vcos_assert(!cf0); /* we hit the clip rect, p0 should now be on the edge. todo: could this fail due to fp evilness? */
   vcos_assert(cross_cf); /* we should have hit it somewhere... */
   *p0_x_io = p0_x;
   *p0_y_io = p0_y;
   *cf0_io = cross_cf;
   return true;
}

static void fill_clip_vert(FILL_T *fill, float x, float y, bool subpath_last_implicit)
{
   uint32_t cf = get_cf(x, y, fill->i->clip);
   uint32_t zone = cf_to_zone(cf);

again:

   if (fill->in_subpath) {
      if (fill->cf) {
         float enter_x = fill->x, enter_y = fill->y;
         uint32_t enter_cf = fill->cf;
         if (clip_line(&enter_x, &enter_y, &enter_cf, x, y, cf, fill->i->clip)) {
            int32_t enter_edge, exit_edge, dir;

            /*
               line from previous point (outside) to current point enters clip rect
               add corners from exit to entry edges
            */

            enter_edge = _msb(enter_cf);
            exit_edge = fill->exit_edge;
            enter_edge += (((fill->zone_delta - (2 * (enter_edge - exit_edge))) + 2) >> 3) * 4; /* correct number of cycles around rect */

            dir = (exit_edge < enter_edge) ? 1 : -1;
            for (; exit_edge != enter_edge; exit_edge += dir) {
               int32_t i = exit_edge + (dir >> 1);
               float corner_x = fill->i->clip[(i + 1) & 2];
               float corner_y = fill->i->clip[1 + (i & 2)];
               fill_vert(fill, float_to_int_shift(corner_x, 4), float_to_int_shift(corner_y, 4));
            }

            /*
               add entry point
            */

            fill_vert(fill, float_to_int_shift(enter_x, 4), float_to_int_shift(enter_y, 4));

            if (cf) {
               /*
                  we passed right through the clip rect
                  add exit point
               */

               float exit_x = x, exit_y = y;
               uint32_t exit_cf = cf;
               verify(clip_line(&exit_x, &exit_y, &exit_cf, enter_x, enter_y, 0, fill->i->clip));

               fill_vert(fill, float_to_int_shift(exit_x, 4), float_to_int_shift(exit_y, 4));

               fill->exit_edge = _msb(exit_cf);
               fill->zone_delta = 0;
            }
         } else {
            int32_t d;

            /*
               line from previous point (outside) to current point doesn't enter clip rect
            */

            vcos_assert(cf); /* current point must be outside */
            d = (zone - fill->zone) & 0x7;
            if (d > 4) {
               d -= 8;
            } else if (d == 4) {
               /*
                  either + or - 4, use partway clip-flags to find out which
               */

               d = (((cf_to_zone(enter_cf) - fill->zone) & 0x7) < 4) ? 4 : -4;
            } /* else d < 4: ok */
            fill->zone_delta += d;
         }
      }

      if (!cf) {
         /*
            current point inside clip rect
            just add it
         */

         fill_vert(fill, float_to_int_shift(x, 4), float_to_int_shift(y, 4));
      } else if (!fill->cf) {
         /*
            current point outside clip rect. previous point was inside clip rect
            find the exit point and add it
         */

         float exit_x = x, exit_y = y;
         uint32_t exit_cf = cf;
         verify(clip_line(&exit_x, &exit_y, &exit_cf, fill->x, fill->y, 0, fill->i->clip));

         fill_vert(fill, float_to_int_shift(exit_x, 4), float_to_int_shift(exit_y, 4));

         fill->exit_edge = _msb(exit_cf);
         fill->zone_delta = 0;
      }
   } else {
      if (cf) {
         /*
            first point in subpath is outside clip rect
            to simplify the algorithm, pretend there was a preceding point just inside the clip rect
            at the end of the subpath, the same "just inside" point is added again
         */

         float inside_x = x, inside_y = y;
         if (cf & CF_LEFT)   { inside_x = fill->i->clip[0]; }
         if (cf & CF_BOTTOM) { inside_y = fill->i->clip[1]; }
         if (cf & CF_RIGHT)  { inside_x = fill->i->clip[2]; }
         if (cf & CF_TOP)    { inside_y = fill->i->clip[3]; }
         fill_vert(fill, float_to_int_shift(inside_x, 4), float_to_int_shift(inside_y, 4));

         fill->exit_edge = _msb(cf);
         fill->zone_delta = 0;
      } else {
         fill_vert(fill, float_to_int_shift(x, 4), float_to_int_shift(y, 4));
      }
   }

   fill->x = x;
   fill->y = y;
   fill->cf = cf;
   fill->zone = zone;

   if (subpath_last_implicit && cf) {
      /*
         last point in subpath outside clip rect
         add "just inside" point (see subpath first handling)
      */

      if (cf & CF_LEFT)   { x = fill->i->clip[0]; }
      if (cf & CF_BOTTOM) { y = fill->i->clip[1]; }
      if (cf & CF_RIGHT)  { x = fill->i->clip[2]; }
      if (cf & CF_TOP)    { y = fill->i->clip[3]; }
      cf = 0;

      goto again;
   }
}

static void fill_chunk_normal(FILL_T *fill, const VG_TESS_CHUNK_NORMAL_T *chunk)
{
   uint32_t i;
   for (i = 0; i != chunk->n; ++i) {
      float x = chunk->xs[i];
      float y = chunk->ys[i];
      vg_mat3x3_affine_transform((const VG_MAT3X3_T *)fill->i->user_to_surface, &x, &y);
      fill_clip_vert(fill, x, y, (i == (chunk->n - 1)) && chunk->subpath_last_implicit);
   }
   if (chunk->subpath_last_implicit) { fill_end_subpath(fill); }
}

static bool fill_cubic_ok(const float *p)
{
   /*
      test is essentially:
      perpendicular distance of control points from line < FLATNESS

      there is a nasty corner case when p0 == p3. the test doesn't tell us
      anything in this case, so we just make sure we always subdivide. we do
      this by making the condition < rather than <=. unfortunately, this means
      we always maximally subdivide degenerate cubics, which seems pretty bad!
      todo: do something about it
   */

   float a_x = p[6] - p[0], a_y = p[7] - p[1];
   float b_x = p[2] - p[0], b_y = p[3] - p[1];
   float c_x = p[4] - p[6], c_y = p[5] - p[7];
   float a_cross_b = (a_x * b_y) - (a_y * b_x);
   float a_cross_c = (a_x * c_y) - (a_y * c_x);
   float d_sq = (a_x * a_x) + (a_y * a_y);
   return _maxf(a_cross_b * a_cross_b, a_cross_c * a_cross_c) < (FLATNESS_SQ * d_sq);
}

static void fill_clip_cubic(FILL_T *fill, const float *p, uint32_t level)
{
   if ((level != LEVEL_MAX) && !fill_cubic_ok(p)) {
      float q[8], r[8];
      vg_tess_subd_cubic(q, r, p, 0.5f);
      fill_clip_cubic(fill, q, level + 1);
      fill_clip_vert(fill, r[0], r[1], false);
      fill_clip_cubic(fill, r, level + 1);
   }
}

static void fill_chunk_cubic(FILL_T *fill, const VG_TESS_CHUNK_CUBIC_T *chunk)
{
   float p[8];
   uint32_t i;

   memcpy(p, chunk->p, sizeof(p));
   for (i = 0; i != 4; ++i) {
      vg_mat3x3_affine_transform((const VG_MAT3X3_T *)fill->i->user_to_surface, p + (i * 2), p + (i * 2) + 1);
   }
   if (chunk->subpath_first) { fill_clip_vert(fill, p[0], p[1], false); }
   fill_clip_cubic(fill, p, 0);
   fill_clip_vert(fill, p[6], p[7], false);
}

typedef struct {
   const IO_CHUNK_I_T *i;
   const IO_CHUNK_STROKE_I2_T *i2;
   const IO_CHUNK_STROKE_I3_T *i3;

   LIST_HEADER_T *h; /* header of current list */
   uint32_t n; /* how many lists in total? */

   /* most recent p/t */
   float x, y, tx, ty;

   /* at the start of a subpath, close_x is set to CLOSE_X_DEGENERATE
    *
    * when the first non-degenerate segment is encountered, if the start of the
    * segment is on, close_x/close_y/close_tx/close_ty are set to the start
    * position/tangent. otherwise, close_x is set to CLOSE_X_NOT_IN_DASH
    *
    * at the end of a subpath, if close_x isn't CLOSE_X_NOT_IN_DASH, we either
    * construct a close join or a start cap */
   #define CLOSE_X_DEGENERATE (float_from_bits(0x7f800002)) /* unusual nans */
   #define CLOSE_X_NOT_IN_DASH (float_from_bits(0x7f800003))
   float close_x, close_y, close_tx, close_ty;

   /* dashing */
   float dash_phase; /* current phase (i2->dash_phase is start phase) */
   uint32_t dash_phase_i;

   /* are we currently processing a cubic or a normal chunk? this affects the
    * behaviour of stroke_dash_clerp */
   bool is_cubic;

   jmp_buf error_jmp;
} STROKE_T;

static void stroke_begin(STROKE_T *stroke, const IO_CHUNK_T *io_chunk)
{
   const IO_CHUNK_T *io_chunk2 = next_io_chunk((IO_CHUNK_T *)io_chunk); /* casting away const is a bit ich, but probably less ich than 2 functions that do the same thing */
   const IO_CHUNK_T *io_chunk3 = next_io_chunk((IO_CHUNK_T *)io_chunk2);
   stroke->i = &io_chunk->i;
   stroke->i2 = &io_chunk2->stroke_i2;
   stroke->i3 = &io_chunk3->stroke_i3;
   stroke->h = NULL;
   stroke->n = 0;
   stroke->dash_phase = io_chunk2->stroke_i2.dash_phase;
   stroke->dash_phase_i = io_chunk2->stroke_i2.dash_phase_i;
}

static void stroke_end(STROKE_T *stroke, REP_T *r)
{
   if (stroke->n != 0) {
      set_list_length(&stroke->i->tess->u.in_use.v_allocator, stroke->h, false);
   }
   r->in_use.h = stroke->h;
   r->in_use.n = stroke->n;
}

static void stroke_tri(STROKE_T *stroke, uint32_t xy0, uint32_t xy1, uint32_t xy2)
{
   LIST_HEADER_T *h;
   uint32_t *p;

   if (stroke->n != 0) {
      p = (uint32_t *)alloc(&stroke->i->tess->u.in_use.v_allocator, 3 * sizeof(uint32_t), NULL);
      if (p) {
         p[0] = xy0; p[1] = xy1; p[2] = xy2;
         return;
      }
      /* we have a list, but we're going to have to start a new one because we're out of space */
      set_list_length(&stroke->i->tess->u.in_use.v_allocator, stroke->h, false);
      /* ok to start a new list */
   } /* else we don't have a list yet */

   h = (LIST_HEADER_T *)alloc(&stroke->i->tess->u.in_use.v_allocator, offsetof(LIST_HEADER_T, v) + (3 * sizeof(uint32_t)),
      &stroke->i->tess->u.in_use.nmem_group);
   if (!h) { longjmp(stroke->error_jmp, 1); }
   h->next = stroke->h;
   /* we write h->len in set_list_length */
   stroke->h = h;
   ++stroke->n;
   p = h->v; /* allocated room for verts after header */

   p[0] = xy0;
   p[1] = xy1;
   p[2] = xy2;
   vcos_assert((p + 3) == stroke->i->tess->u.in_use.v_allocator.p);
}

/* transform, clip, then output triangle */
static void stroke_tr_clip_tri(STROKE_T *stroke, float x0, float y0, float x1, float y1, float x2, float y2)
{
   float a[14], b[14];
   uint32_t n;

   vg_mat3x3_affine_transform((const VG_MAT3X3_T *)stroke->i->user_to_surface, &x0, &y0);
   vg_mat3x3_affine_transform((const VG_MAT3X3_T *)stroke->i->user_to_surface, &x1, &y1);
   vg_mat3x3_affine_transform((const VG_MAT3X3_T *)stroke->i->user_to_surface, &x2, &y2);

   if (/* test against min x/y */
      ((x0 <= stroke->i->clip[0]) && (x1 <= stroke->i->clip[0]) && (x2 <= stroke->i->clip[0])) ||
      ((y0 <= stroke->i->clip[1]) && (y1 <= stroke->i->clip[1]) && (y2 <= stroke->i->clip[1])) ||
      /* test against max x/y */
      ((x0 >= stroke->i->clip[2]) && (x1 >= stroke->i->clip[2]) && (x2 >= stroke->i->clip[2])) ||
      ((y0 >= stroke->i->clip[3]) && (y1 >= stroke->i->clip[3]) && (y2 >= stroke->i->clip[3]))) {
      /* entirely outside inner -- can just discard */
      return;
   }

   if (/* test against min x/y */
      (x0 >= stroke->i2->clip_outer[0]) && (y0 >= stroke->i2->clip_outer[1]) &&
      (x1 >= stroke->i2->clip_outer[0]) && (y1 >= stroke->i2->clip_outer[1]) &&
      (x2 >= stroke->i2->clip_outer[0]) && (y2 >= stroke->i2->clip_outer[1]) &&
      /* test against max x/y */
      (x0 <= stroke->i2->clip_outer[2]) && (y0 <= stroke->i2->clip_outer[3]) &&
      (x1 <= stroke->i2->clip_outer[2]) && (y1 <= stroke->i2->clip_outer[3]) &&
      (x2 <= stroke->i2->clip_outer[2]) && (y2 <= stroke->i2->clip_outer[3])) {
      /* entirely inside outer -- no clipping required */
      stroke_tri(stroke,
         (float_to_int_shift(x0, 4) & 0xffff) | (float_to_int_shift(y0, 4) << 16),
         (float_to_int_shift(x1, 4) & 0xffff) | (float_to_int_shift(y1, 4) << 16),
         (float_to_int_shift(x2, 4) & 0xffff) | (float_to_int_shift(y2, 4) << 16));
      return;
   }

   /* need to clip... */
   a[0] = x0;  a[1] = y0;
   a[2] = x1;  a[3] = y1;
   a[4] = x2;  a[5] = y2;
   n = vg_tess_poly_clip(a, b, 3, stroke->i2->clip_outer);
   if (n != 0) {
      uint32_t i;

      vcos_assert(n >= 3);
      n -= 2;

      for (i = 0; i != (n * 2); i += 2) {
         stroke_tri(stroke,
            (float_to_int_shift(a[0], 4) & 0xffff) | (float_to_int_shift(a[1], 4) << 16),
            (float_to_int_shift(a[i + 2], 4) & 0xffff) | (float_to_int_shift(a[i + 3], 4) << 16),
            (float_to_int_shift(a[i + 4], 4) & 0xffff) | (float_to_int_shift(a[i + 5], 4) << 16));
      }
   }
}

static void stroke_join(STROKE_T *stroke, float x, float y, float tx0, float ty0, float tx1, float ty1)
{
   /* todo: possible gaps around joins */

   if ((tx0 * ty1) > (ty0 * tx1)) {
      /* join is ccw, swap and flip tangents to make an equivalent cw join */
      float temp = tx0;
      tx0 = -tx1;
      tx1 = -temp;
      temp = ty0;
      ty0 = -ty1;
      ty1 = -temp;
   }
   /* now only have to deal with cw joins... */

   switch (stroke->i2->join) {
   case JOIN_ROUND:
   {
      bool first = true;
      for (;;) {
         float tx = (tx0 * stroke->i2->cos_theta) + (ty0 * stroke->i2->sin_theta);
         float ty = (ty0 * stroke->i2->cos_theta) - (tx0 * stroke->i2->sin_theta);
         bool more;
         if (first) {
            tx *= stroke->i2->width_hack_round;
            ty *= stroke->i2->width_hack_round;
         }
         more = (tx1 * ty) > (ty1 * tx);
         if (!more) { tx = tx1; ty = ty1; }
         stroke_tr_clip_tri(stroke, x, y,
            x - (ty0 * stroke->i2->line_width * 0.5f),
            y + (tx0 * stroke->i2->line_width * 0.5f),
            x - (ty * stroke->i2->line_width * 0.5f),
            y + (tx * stroke->i2->line_width * 0.5f));
         if (!more) { break; }
         tx0 = tx; ty0 = ty;
         first = false;
      }
      break;
   }
   case JOIN_MITER:
   {
      float nx, ny;
      float oo_dn, cos_mao2, sin2_mao2;
      float ccw0x, ccw0y;
      float ccw1x, ccw1y;

      /* some initial calcs before scaling tangents */
      nx = tx0 - tx1;
      ny = ty0 - ty1;
      if ((absf_(nx) < 1.0f) && (absf_(ny) < 1.0f)) {
         /* too small; won't normalise nicely. do it the other way */
         nx = -(ty0 + ty1); ny = tx0 + tx1;
      }
      oo_dn = rsqrt_((nx * nx) + (ny * ny));
      nx *= oo_dn; ny *= oo_dn; /* nx/ny is normalised and points from join point towards miter point */
      cos_mao2 = (nx * tx0) + (ny * ty0); /* cos(miter angle / 2). miter angle is the angle at the miter point */
      sin2_mao2 = 1.0f - (cos_mao2 * cos_mao2);

      /* scale tangents, calc ccw points */
      tx0 *= stroke->i2->line_width * 0.5f;
      ty0 *= stroke->i2->line_width * 0.5f;
      tx1 *= stroke->i2->line_width * 0.5f;
      ty1 *= stroke->i2->line_width * 0.5f;
      ccw0x = x - ty0;
      ccw0y = y + tx0;
      ccw1x = x - ty1;
      ccw1y = y + tx1;

      /* triangle to make a bevel join */
      stroke_tr_clip_tri(stroke, x, y, ccw0x, ccw0y, ccw1x, ccw1y);

      /* triangle to turn the bevel join into a miter join (but only if the miter length isn't too long) */
      if (sin2_mao2 > EPS) {
         float oo_sin_mao2 = rsqrt_(sin2_mao2); /* 1 / sin(miter angle / 2) = miter length / stroke width */
         if (oo_sin_mao2 < stroke->i2->miter_limit) {
            float mx = ccw0x + (tx0 * cos_mao2 * oo_sin_mao2);
            float my = ccw0y + (ty0 * cos_mao2 * oo_sin_mao2);
            stroke_tr_clip_tri(stroke, mx, my, ccw0x, ccw0y, ccw1x, ccw1y);
         }
      }
      break;
   }
   case JOIN_BEVEL:
   {
      tx0 *= stroke->i2->line_width * 0.5f;
      ty0 *= stroke->i2->line_width * 0.5f;
      tx1 *= stroke->i2->line_width * 0.5f;
      ty1 *= stroke->i2->line_width * 0.5f;
      stroke_tr_clip_tri(stroke, x, y, x - ty0, y + tx0, x - ty1, y + tx1);
      break;
   }
   default:
   {
      UNREACHABLE();
   }
   }
}

static void stroke_cap(STROKE_T *stroke, float x, float y, float tx, float ty)
{
   tx *= stroke->i2->line_width * 0.5f;
   ty *= stroke->i2->line_width * 0.5f;

   switch (stroke->i2->cap) {
   case CAP_BUTT:
   {
      /* don't need to draw anything for a butt cap */
      break;
   }
   case CAP_ROUND:
   {
      uint32_t n, i;
      float last_tx, last_ty;
      stroke_tr_clip_tri(stroke, x, y, x - ty, y + tx, x + ty, y - tx); /* avoid possible gaps */
      n = (uint32_t)ceil_(PI * stroke->i2->oo_theta);
      last_tx = -tx;
      last_ty = -ty;
      for (i = 0; i != n; ++i) {
         float next_tx = (i == (n - 1)) ? last_tx : ((tx * stroke->i2->cos_theta) + (ty * stroke->i2->sin_theta));
         float next_ty = (i == (n - 1)) ? last_ty : ((ty * stroke->i2->cos_theta) - (tx * stroke->i2->sin_theta));
         if (i == 0) {
            next_tx *= stroke->i2->width_hack_round;
            next_ty *= stroke->i2->width_hack_round;
         }
         stroke_tr_clip_tri(stroke, x, y, x - ty, y + tx, x - next_ty, y + next_tx);
         tx = next_tx; ty = next_ty;
      }
      break;
   }
   case CAP_SQUARE:
   {
      float ccw0x = x - ty,     ccw0y = y + tx;
      float cw0x  = x + ty,     cw0y  = y - tx;
      float ccw1x = ccw0x + tx, ccw1y = ccw0y + ty;
      float cw1x  = cw0x + tx,  cw1y  = cw0y + ty;
      stroke_tr_clip_tri(stroke, ccw0x, ccw0y, cw0x, cw0y, ccw1x, ccw1y);
      stroke_tr_clip_tri(stroke, ccw1x, ccw1y, cw1x, cw1y, cw0x, cw0y);
      break;
   }
   default:
   {
      UNREACHABLE();
   }
   }
}

/* we identify non-cubic ("normal") clerps by tx0 being CLERP_TX0_NORMAL. we
 * don't apply the width hack to these clerps */
#define CLERP_TX0_NORMAL (float_from_bits(0x7f800002)) /* unusual nan */

static void stroke_clerp(STROKE_T *stroke, float x0, float y0, float x1, float y1, float tx0, float ty0, float tx1, float ty1)
{
   float s, ccw0x, ccw0y, cw0x, cw0y, ccw1x, ccw1y, cw1x, cw1y;
   float ax, ay, bx, by, cx, cy, dx, dy;
   float d_cross_a, it, ix, iy;

   if (floats_identical(tx0, CLERP_TX0_NORMAL)) {
      /* non-cubic clerp. don't apply the width hack. real tx0/ty0 are the same
       * as tx1/ty1 */
      tx0 = tx1;
      ty0 = ty1;
   } else {
      /* cubic clerp. apply the width hack. todo: we probably shouldn't apply
       * the width hack to the first and last cross sections of a segment as we
       * may introduce gaps... */
      tx0 *= stroke->i2->width_hack_clerp;
      ty0 *= stroke->i2->width_hack_clerp;
      tx1 *= stroke->i2->width_hack_clerp;
      ty1 *= stroke->i2->width_hack_clerp;
   }

   if ((tx0 * ty1) > (ty0 * tx1)) {
      /* rotate t0 ccw */
      s = -stroke->i2->sin_theta;
   } else {
      s = stroke->i2->sin_theta;
   }
   for (;;) {
      float tx = (tx0 * stroke->i2->cos_theta) + (ty0 * s);
      float ty = (ty0 * stroke->i2->cos_theta) - (tx0 * s);
      if ((tx * ty1 * s) >= (ty * tx1 * s)) {
         /* too far. break out of loop for final bit */
         break;
      }
      /* todo: possible gaps... */
      stroke_tr_clip_tri(stroke, x0, y0,
         x0 - (ty0 * stroke->i2->line_width * 0.5f),
         y0 + (tx0 * stroke->i2->line_width * 0.5f),
         x0 - (ty * stroke->i2->line_width * 0.5f),
         y0 + (tx * stroke->i2->line_width * 0.5f));
      stroke_tr_clip_tri(stroke, x0, y0,
         x0 + (ty0 * stroke->i2->line_width * 0.5f),
         y0 - (tx0 * stroke->i2->line_width * 0.5f),
         x0 + (ty * stroke->i2->line_width * 0.5f),
         y0 - (tx * stroke->i2->line_width * 0.5f));
      tx0 = tx; ty0 = ty;
   }

   tx0 *= stroke->i2->line_width * 0.5f;
   ty0 *= stroke->i2->line_width * 0.5f;
   tx1 *= stroke->i2->line_width * 0.5f;
   ty1 *= stroke->i2->line_width * 0.5f;
   ccw0x = x0 - ty0; ccw0y = y0 + tx0;
   cw0x  = x0 + ty0; cw0y  = y0 - tx0;
   ccw1x = x1 - ty1; ccw1y = y1 + tx1;
   cw1x  = x1 + ty1; cw1y  = y1 - tx1;
   ax = cw0x  - ccw0x; ay = cw0y  - ccw0y;
   bx = ccw1x - ccw0x; by = ccw1y - ccw0y;
   cx = cw1x  - ccw0x; cy = cw1y  - ccw0y;
   dx = cw1x - ccw1x; dy = cw1y - ccw1y;
   d_cross_a = (dx * ay) - (dy * ax);
   if (/* the triangles ccw0, cw0, cw1 and ccw1, cw1, ccw0 don't intersect */
      (((ax * cy) < (ay * cx)) != ((bx * cy) < (by * cx))) ||
      /* or the triangles only just intersect (at worst we will color slightly outside the lines) */
      (absf_(d_cross_a) < EPS)) {
      stroke_tr_clip_tri(stroke, ccw0x, ccw0y, cw0x, cw0y, cw1x, cw1y);
      stroke_tr_clip_tri(stroke, ccw1x, ccw1y, cw1x, cw1y, ccw0x, ccw0y);
   } else {
      /* the triangles ccw0, cw0, cw1 and ccw1, cw1, ccw0 intersect */
      it = ((ax * by) - (ay * bx)) * recip_(d_cross_a);
      ix = ccw1x + (dx * it); iy = ccw1y + (dy * it);
      stroke_tr_clip_tri(stroke, ccw0x, ccw0y, cw0x, cw0y, ix, iy);
      stroke_tr_clip_tri(stroke, ccw1x, ccw1y, cw1x, cw1y, ix, iy);
      stroke_tr_clip_tri(stroke, ccw0x, ccw0y, ccw1x, ccw1y, ix, iy);
      stroke_tr_clip_tri(stroke, cw0x, cw0y, cw1x, cw1y, ix, iy);
   }
}

/*
   the functions above this comment (stroke_cap, stroke_join, stroke_clerp, etc)
   construct primitives based solely on their non-stroke arguments. the
   functions below this comment inspect/modify the stuff inside stroke (eg
   stroke->x/y)
*/

static void stroke_dash_clerp(STROKE_T *stroke, float x, float y, float tx, float ty)
{
   if (stroke->i2->dashing) {
      float dx = x - stroke->x;
      float dy = y - stroke->y;
      float dos_sq = ((dx * dx) + (dy * dy)) * stroke->i2->dash_oo_scale * stroke->i2->dash_oo_scale;
      float sod = (dos_sq == 0.0f) ? FLT_MAX : rsqrt_(dos_sq);
      float prev_phase = stroke->dash_phase, phase;
      stroke->dash_phase += sod * dos_sq; /* (s/d)(d/s)(d/s) = d/s */

      if ((phase = stroke->i3->dash_pattern[stroke->dash_phase_i]) <= stroke->dash_phase) {
         dx *= sod;
         dy *= sod;
         if (!stroke->is_cubic) {
            stroke->tx = tx;
            stroke->ty = ty;
         }
         do {
            float t = phase - prev_phase;
            float next_x = stroke->x + (t * dx);
            float next_y = stroke->y + (t * dy);
            if (stroke->dash_phase_i & 0x1) {
               /* start cap */
               stroke_cap(stroke, next_x, next_y, -stroke->tx, -stroke->ty);
            } else {
               /* clerp up to end cap */
               stroke_clerp(stroke, stroke->x, stroke->y, next_x, next_y,
                  stroke->is_cubic ? stroke->tx : CLERP_TX0_NORMAL, stroke->ty, stroke->tx, stroke->ty);
               /* end cap */
               stroke_cap(stroke, next_x, next_y, stroke->tx, stroke->ty);
            }
            stroke->x = next_x;
            stroke->y = next_y;
            prev_phase = phase;
            if (++stroke->dash_phase_i == stroke->i2->dash_pattern_n) {
               prev_phase -= 1.0f;
               stroke->dash_phase -= 1.0f;
               stroke->dash_phase_i = 0;
            }
         } while ((phase = stroke->i3->dash_pattern[stroke->dash_phase_i]) <= stroke->dash_phase);
      }

      if (!(stroke->dash_phase_i & 0x1)) {
         stroke_clerp(stroke, stroke->x, stroke->y, x, y,
            stroke->is_cubic ? stroke->tx : CLERP_TX0_NORMAL, stroke->ty, tx, ty);
      }
   } else {
      stroke_clerp(stroke, stroke->x, stroke->y, x, y,
         stroke->is_cubic ? stroke->tx : CLERP_TX0_NORMAL, stroke->ty, tx, ty);
   }

   stroke->x = x;
   stroke->y = y;
   stroke->tx = tx;
   stroke->ty = ty;
}

static void stroke_chunk_normal(STROKE_T *stroke, VG_TESS_CHUNK_NORMAL_T *chunk)
{
   uint32_t i;
   bool close;

   stroke->is_cubic = false;

   close = chunk->close;
   vcos_assert(chunk->subpath_last || !close);

   if (chunk->subpath_first) {
      stroke->x = chunk->xs[0];
      stroke->y = chunk->ys[0];
      stroke->tx = 1.0f;
      stroke->ty = 0.0f;

      stroke->close_x = CLOSE_X_DEGENERATE;

      if (stroke->i2->dash_phase_reset) {
         stroke->dash_phase = stroke->i2->dash_phase;
         stroke->dash_phase_i = stroke->i2->dash_phase_i;
      }
   }

   for (i = (chunk->subpath_first ? 1 : 0); i != chunk->explicit_n; ++i) {
      float x = chunk->xs[i];
      float y = chunk->ys[i];
      float tx = x - stroke->x;
      float ty = y - stroke->y;
      float hyp_sq;

      if ((tx == 0.0f) && (ty == 0.0f)) {
         if (chunk->subpath_last && (i == (chunk->explicit_n - 1)) &&
            floats_identical(stroke->close_x, CLOSE_X_DEGENERATE)) {
            /* last segment of entirely degenerate subpath. let it through, as
             * we need to setup close_x/y/tx/ty, but force close off, as we're
             * just supposed to generate start and end caps for entirely
             * degenerate subpaths */
            close = false;
         } else {
            /* skip degenerate segments so the initial tangent will be that of
             * the first non-degenerate segment, rather than (1, 0) */
            continue;
         }
      }

      hyp_sq = (tx * tx) + (ty * ty);
      if (hyp_sq > EPS) {
         float oo_hyp = rsqrt_(hyp_sq);
         tx *= oo_hyp;
         ty *= oo_hyp;
      } else {
         tx = stroke->tx;
         ty = stroke->ty;
      }

      if (floats_identical(stroke->close_x, CLOSE_X_DEGENERATE)) {
         if (stroke->dash_phase_i & 0x1) {
            stroke->close_x = CLOSE_X_NOT_IN_DASH;
         } else {
            stroke->close_x = stroke->x;
            stroke->close_y = stroke->y;
            stroke->close_tx = tx;
            stroke->close_ty = ty;
         }
      } else if (!(stroke->dash_phase_i & 0x1)) {
         stroke_join(stroke, stroke->x, stroke->y, stroke->tx, stroke->ty, tx, ty);
      }

      stroke_dash_clerp(stroke, x, y, tx, ty);
   }

   if (chunk->subpath_last) {
      if (close &&
         !floats_identical(stroke->close_x, CLOSE_X_NOT_IN_DASH) &&
         !(stroke->dash_phase_i & 0x1)) {
         vcos_assert((stroke->x == stroke->close_x) && (stroke->y == stroke->close_y));
         stroke_join(stroke, stroke->x, stroke->y, stroke->tx, stroke->ty,
            stroke->close_tx, stroke->close_ty);
      } else {
         if (!floats_identical(stroke->close_x, CLOSE_X_NOT_IN_DASH)) {
            stroke_cap(stroke, stroke->close_x, stroke->close_y, -stroke->close_tx, -stroke->close_ty);
         }
         if (!(stroke->dash_phase_i & 0x1)) {
            stroke_cap(stroke, stroke->x, stroke->y, stroke->tx, stroke->ty);
         }
      }
   }
}

static bool stroke_cubic_ok(STROKE_T *stroke, const float *p, const float *t0, const float *t1)
{
   float a_x, a_y, d_sq, c;
   bool inter;
   float b_x, b_y, c_x, c_y, a_cross_b, a_cross_c;

   /* todo: check this */

   /*
      we test two things:
      - the angle between the tangents
      - the flatness
   */

   a_x = p[6] - p[0], a_y = p[7] - p[1];
   d_sq = (a_x * a_x) + (a_y * a_y); /* distance between end points (squared) */

   /*
      the acceptable angle between the tangents depends on whether or not the cross-sections intersect...
   */

   c = (t0[0] * t1[0]) + (t0[1] * t1[1]); /* cos(angle between tangents) */
   inter = d_sq < ((1.0f - (c * c)) * stroke->i2->line_width * stroke->i2->line_width * 0.25f); /* this intersection test is an approximation. todo: is it good enough? */
   if (c < (inter ? stroke->i2->cos_theta_inter : stroke->i2->cos_theta)) {
      return false; /* cos(angle) too small, angle too large */
   }

   /*
      the flatness test is almost the same as for fills:
      perpendicular distance of control points from line <= flatness

      the nasty corner case when p0 == p3 is dealt with differently here. the
      only non-degenerate cubics with p0 == p3 are loops. but (for strokes) we
      split cubics before subdividing so they don't contain loops. so for the
      p0 == p3 corner case, we can just assume the cubic is degenerate and
      return true. so we make the condition <= rather than < as it is for fills
   */

   b_x = p[2] - p[0]; b_y = p[3] - p[1];
   c_x = p[4] - p[6]; c_y = p[5] - p[7];
   a_cross_b = (a_x * b_y) - (a_y * b_x);
   a_cross_c = (a_x * c_y) - (a_y * c_x);
   return (_maxf(a_cross_b * a_cross_b, a_cross_c * a_cross_c) * stroke->i2->oo_flatness_sq) <= d_sq;
}

static void stroke_dash_simple_cubic(STROKE_T *stroke, const float *p, const float *t0, const float *t1, uint32_t level)
{
   if ((level != LEVEL_MAX) && !stroke_cubic_ok(stroke, p, t0, t1)) {
      float q[8], r[8], t[2], hyp_sq;
      vg_tess_subd_cubic(q, r, p, 0.5f);
      t[0] = (p[4] - p[2]) + (p[6] - p[0]);
      t[1] = (p[5] - p[3]) + (p[7] - p[1]);
      hyp_sq = (t[0] * t[0]) + (t[1] * t[1]);
      if (hyp_sq > EPS) {
         float oo_hyp = rsqrt_(hyp_sq);
         t[0] *= oo_hyp;
         t[1] *= oo_hyp;
      } else {
         /* todo: try harder? not necessary to pass the cts... */
         t[0] = t0[0];
         t[1] = t0[1];
      }
      stroke_dash_simple_cubic(stroke, q, t0, t, level + 1);
      stroke_dash_clerp(stroke, r[0], r[1], t[0], t[1]);
      stroke_dash_simple_cubic(stroke, r, t, t1, level + 1);
   }
}

static void get_subd_cubic_t(float *t, const float *q, const float *r, const float *backup_t)
{
   uint32_t i;
   for (i = 0; i != 4; i += 2) {
      float hyp_sq;
      t[0] = r[2 + i] - q[4 - i];
      t[1] = r[3 + i] - q[5 - i];
      hyp_sq = (t[0] * t[0]) + (t[1] * t[1]);
      if (hyp_sq > EPS) {
         float oo_hyp = rsqrt_(hyp_sq);
         t[0] *= oo_hyp;
         t[1] *= oo_hyp;
         return;
      }
   }
   t[0] = backup_t[0];
   t[1] = backup_t[1];
}

static void stroke_dash_cubic(STROKE_T *stroke, const float *p, const float *t0, const float *t1)
{
   float u0, u1;
   uint32_t n;
   float q[8], r[8], u0t[2], u1t[2];

   /* split at inflection points */
   n = get_cubic_ips(&u0, &u1, p);
   if (n == 0) {
      /* no inflection points. possibly a loop? */
      n = get_cubic_zvs(&u0, &u1, p + (absf_(t0[0]) < absf_(t0[1])));
      if (n != 2) {
         stroke_dash_simple_cubic(stroke, p, t0, t1, 0);
         return;
      }
      /* possibly a loop: split it */
      u0 = (u0 + u1) * 0.5f;
      n = 1;
   }
   vg_tess_subd_cubic(q, r, p, u0);
   get_subd_cubic_t(u0t, q, r, t0);
   stroke_dash_simple_cubic(stroke, q, t0, u0t, 1);
   stroke_dash_clerp(stroke, r[0], r[1], u0t[0], u0t[1]);
   if ((n == 1) || (u1 <= u0)) { /* u0 should be <= u1, but fp lameness might result in u1 being slightly less than u0 */
      stroke_dash_simple_cubic(stroke, r, u0t, t1, 1);
      return;
   }
   vg_tess_subd_cubic(q, r, r, (u1 - u0) * recip_(1.0f - u0));
   get_subd_cubic_t(u1t, q, r, u0t);
   stroke_dash_simple_cubic(stroke, q, u0t, u1t, 1);
   stroke_dash_clerp(stroke, r[0], r[1], u1t[0], u1t[1]);
   stroke_dash_simple_cubic(stroke, r, u1t, t1, 1);
}

static bool get_cubic_t(float *t, const float *p, bool first)
{
   uint32_t i;
   for (i = 2; i != 8; i += 2) {
      float hyp_sq;
      t[0] = first ? (p[i] - p[0]) : (p[6] - p[6 - i]);
      t[1] = first ? (p[i + 1] - p[1]) : (p[7] - p[7 - i]);
      hyp_sq = (t[0] * t[0]) + (t[1] * t[1]);
      if (hyp_sq > EPS) {
         float oo_hyp = rsqrt_(hyp_sq);
         t[0] *= oo_hyp;
         t[1] *= oo_hyp;
         return true;
      }
   }
   return false;
}

static void stroke_chunk_cubic(STROKE_T *stroke, VG_TESS_CHUNK_CUBIC_T *chunk)
{
   float t0[2], t1[2];

   stroke->is_cubic = true;

   if (chunk->subpath_first) {
      stroke->x = chunk->p[0];
      stroke->y = chunk->p[1];
      stroke->tx = 1.0f;
      stroke->ty = 0.0f;

      stroke->close_x = CLOSE_X_DEGENERATE;

      if (stroke->i2->dash_phase_reset) {
         stroke->dash_phase = stroke->i2->dash_phase;
         stroke->dash_phase_i = stroke->i2->dash_phase_i;
      }
   } else {
      vcos_assert((stroke->x == chunk->p[0]) && (stroke->y == chunk->p[1]));
   }

   if (!chunk->subpath_last &&
      (chunk->p[0] == chunk->p[2]) && (chunk->p[2] == chunk->p[4]) && (chunk->p[4] == chunk->p[6]) &&
      (chunk->p[1] == chunk->p[3]) && (chunk->p[3] == chunk->p[5]) && (chunk->p[5] == chunk->p[7])) {
      /* skip degenerate segments so the initial tangent will be that of the
       * first non-degenerate segment, rather than (1, 0). don't skip the last
       * segment though, we need to do all the subpath_last stuff */
      return;
   }

   if (!get_cubic_t(t0, chunk->p, true)) {
      t0[0] = stroke->tx;
      t0[1] = stroke->ty;
   }
   if (!get_cubic_t(t1, chunk->p, false)) {
      /* should maybe use t0[0/1] instead of stroke->tx/ty, but it shouldn't
       * matter */
      t1[0] = stroke->tx;
      t1[1] = stroke->ty;
   }

   if (floats_identical(stroke->close_x, CLOSE_X_DEGENERATE)) {
      if (stroke->dash_phase_i & 0x1) {
         stroke->close_x = CLOSE_X_NOT_IN_DASH;
      } else {
         stroke->close_x = stroke->x; /* chunk->p[0/1] would also be fine */
         stroke->close_y = stroke->y;
         stroke->close_tx = t0[0];
         stroke->close_ty = t0[1];
      }
   } else if (!(stroke->dash_phase_i & 0x1)) {
      /* chunk->p[0/1] would be fine in place of stroke->x/y */
      stroke_join(stroke, stroke->x, stroke->y, stroke->tx, stroke->ty, t0[0], t0[1]);
   }

   stroke->tx = t0[0];
   stroke->ty = t0[1];
   stroke_dash_cubic(stroke, chunk->p, t0, t1);
   stroke_dash_clerp(stroke, chunk->p[6], chunk->p[7], t1[0], t1[1]);

   if (chunk->subpath_last) {
      if (!floats_identical(stroke->close_x, CLOSE_X_NOT_IN_DASH)) {
         stroke_cap(stroke, stroke->close_x, stroke->close_y, -stroke->close_tx, -stroke->close_ty);
      }
      if (!(stroke->dash_phase_i & 0x1)) {
         /* chunk->p[6/7], t1[0/1] would be fine in place of stroke->x/y, stroke->tx/ty */
         stroke_cap(stroke, stroke->x, stroke->y, stroke->tx, stroke->ty);
      }
   }
}

static void interpolate_chunk_normal(
   VG_TESS_CHUNK_NORMAL_T *c,
   const VG_TESS_CHUNK_NORMAL_T *a,
   const VG_TESS_CHUNK_NORMAL_T *b,
   float t)
{
   float omt;
   uint32_t i;

   vcos_assert(a->n == b->n);
   vcos_assert(a->subpath_last_implicit == b->subpath_last_implicit);
   vcos_assert(a->subpath_first == b->subpath_first);
   vcos_assert(a->subpath_last == b->subpath_last);
   vcos_assert(a->close == b->close);
   vcos_assert(a->explicit_n == b->explicit_n);

   c->n = a->n;
   c->subpath_last_implicit = a->subpath_last_implicit;
   omt = 1.0f - t;
   for (i = 0; i != a->n; ++i) {
      c->xs[i] = (a->xs[i] * omt) + (b->xs[i] * t);
      c->ys[i] = (a->ys[i] * omt) + (b->ys[i] * t);
   }
   c->subpath_first = a->subpath_first;
   c->subpath_last = a->subpath_last;
   c->close = a->close;
   c->explicit_n = a->explicit_n;
}

static void interpolate_chunk_cubic(
   VG_TESS_CHUNK_CUBIC_T *c,
   const VG_TESS_CHUNK_CUBIC_T *a,
   const VG_TESS_CHUNK_CUBIC_T *b,
   float t)
{
   float omt;
   uint32_t i;

   vcos_assert((a->n == 0) && (b->n == 0));
   vcos_assert(a->subpath_first == b->subpath_first);
   vcos_assert(a->subpath_last == b->subpath_last);
   vcos_assert((a->magic == VG_TESS_CHUNK_CUBIC_MAGIC) && (b->magic == VG_TESS_CHUNK_CUBIC_MAGIC));

   c->n = 0;
   c->subpath_first = a->subpath_first;
   c->subpath_last = a->subpath_last;
   omt = 1.0f - t;
   for (i = 0; i != 8; ++i) {
      c->p[i] = (a->p[i] * omt) + (b->p[i] * t);
   }
   c->magic = VG_TESS_CHUNK_CUBIC_MAGIC; /* this is unlikely to matter, but set it anyway */
}

static bool do_fill(const IO_CHUNK_T *io_chunk, REP_T *r)
{
   FILL_T fill;
   uint8_t *chunk_a, *chunk_a_end, *chunk_b;

   fill_begin(&fill, io_chunk);

   if (setjmp(fill.error_jmp)) {
      return false;
   }

   chunk_a = (uint8_t *)khrn_hw_unaddr(io_chunk->i.chunks_a);
   chunk_a_end = chunk_a + io_chunk->i.chunks_size;
   chunk_b = io_chunk->i.chunks_b ? (uint8_t *)khrn_hw_unaddr(io_chunk->i.chunks_b) : NULL;
   while (chunk_a != chunk_a_end) {
      if (*chunk_a == 0) {
         if (chunk_b) {
            VG_TESS_CHUNK_CUBIC_T c;
            vcos_assert(*chunk_b == 0);
            interpolate_chunk_cubic(&c, (VG_TESS_CHUNK_CUBIC_T *)chunk_a, (VG_TESS_CHUNK_CUBIC_T *)chunk_b, io_chunk->i.t);
            fill_chunk_cubic(&fill, &c);
            chunk_b += sizeof(VG_TESS_CHUNK_CUBIC_T);
         } else {
            fill_chunk_cubic(&fill, (VG_TESS_CHUNK_CUBIC_T *)chunk_a);
         }
         chunk_a += sizeof(VG_TESS_CHUNK_CUBIC_T);
      } else {
         if (chunk_b) {
            VG_TESS_CHUNK_NORMAL_T c;
            vcos_assert(*chunk_b != 0);
            interpolate_chunk_normal(&c, (VG_TESS_CHUNK_NORMAL_T *)chunk_a, (VG_TESS_CHUNK_NORMAL_T *)chunk_b, io_chunk->i.t);
            fill_chunk_normal(&fill, &c);
            chunk_b += sizeof(VG_TESS_CHUNK_NORMAL_T);
         } else {
            fill_chunk_normal(&fill, (VG_TESS_CHUNK_NORMAL_T *)chunk_a);
         }
         chunk_a += sizeof(VG_TESS_CHUNK_NORMAL_T);
      }
   }

   fill_end(&fill, r);

   return true;
}

static bool do_stroke(const IO_CHUNK_T *io_chunk, REP_T *r)
{
   STROKE_T stroke;
   uint8_t *chunk_a, *chunk_a_end, *chunk_b;

   stroke_begin(&stroke, io_chunk);

   if (setjmp(stroke.error_jmp)) {
      return false;
   }

   chunk_a = (uint8_t *)khrn_hw_unaddr(io_chunk->i.chunks_a);
   chunk_a_end = chunk_a + io_chunk->i.chunks_size;
   chunk_b = io_chunk->i.chunks_b ? (uint8_t *)khrn_hw_unaddr(io_chunk->i.chunks_b) : NULL;
   while (chunk_a != chunk_a_end) {
      if (*chunk_a == 0) {
         if (chunk_b) {
            VG_TESS_CHUNK_CUBIC_T c;
            vcos_assert(*chunk_b == 0);
            interpolate_chunk_cubic(&c, (VG_TESS_CHUNK_CUBIC_T *)chunk_a, (VG_TESS_CHUNK_CUBIC_T *)chunk_b, io_chunk->i.t);
            stroke_chunk_cubic(&stroke, &c);
            chunk_b += sizeof(VG_TESS_CHUNK_CUBIC_T);
         } else {
            stroke_chunk_cubic(&stroke, (VG_TESS_CHUNK_CUBIC_T *)chunk_a);
         }
         chunk_a += sizeof(VG_TESS_CHUNK_CUBIC_T);
      } else {
         if (chunk_b) {
            VG_TESS_CHUNK_NORMAL_T c;
            vcos_assert(*chunk_b != 0);
            interpolate_chunk_normal(&c, (VG_TESS_CHUNK_NORMAL_T *)chunk_a, (VG_TESS_CHUNK_NORMAL_T *)chunk_b, io_chunk->i.t);
            stroke_chunk_normal(&stroke, &c);
            chunk_b += sizeof(VG_TESS_CHUNK_NORMAL_T);
         } else {
            stroke_chunk_normal(&stroke, (VG_TESS_CHUNK_NORMAL_T *)chunk_a);
         }
         chunk_a += sizeof(VG_TESS_CHUNK_NORMAL_T);
      }
   }

   stroke_end(&stroke, r);

   return true;
}

#endif

/******************************************************************************
message processing (llat task)
******************************************************************************/

#ifdef VG_TESS_QPU

static bool feed(void)
{
   TESS_T *tess;
   void *p;
#ifndef SIMPENROSE
   uint32_t halted;
#endif

   if (shader_if[0] & (1 << 31)) {
      /* either not running or no request */
      return false;
   }

   tess = tesss + shader_if[0];
   shader_if[0] = -1;

   p = tess->u.in_use.ok ? khrn_nmem_group_alloc(&tess->u.in_use.nmem_group, NULL, 0) : NULL;
   vcos_assert(!((uintptr_t)p & (KHRN_HW_SYSTEM_CACHE_LINE_SIZE - 1)));
   if (p) {
      #define SIZE (KHRN_NMEM_GROUP_BLOCK_SIZE & ~(KHRN_HW_SYSTEM_CACHE_LINE_SIZE - 1))
      khrn_hw_invalidate_dcache_range(p, SIZE);
      p = khrn_hw_alias_direct(p);
      vcos_assert(!(khrn_hw_addr(p) & 0xfff)); /* we use the bottom 12 bits for other info... */
      shader_if[1] =
         /* bits 12 through 31: pointer to start of block */
         khrn_hw_addr(p) |
         /* bits 1 through 11: number of subblocks */
         ((SIZE / SUBBLOCK_SIZE) << 1) |
         /* bit 0: flipbit */
         tess->flipbit;
      #undef SIZE
   } else {
      tess->u.in_use.ok = false;
      shader_if[1] = tess->flipbit;
   }

#ifndef SIMPENROSE
   khrn_hw_full_memory_barrier();
   do {
      halted = v3d_read(DBQHLT);
   } while (!halted); /* wait until the qpu stops */
#ifdef VC_QDEBUG
   /* assume that if all qpus are stopped we're in the qdebug loop in
    * submit_user_shader. todo: this isn't going to be the case on a 1 qpu
    * system! */
   { vcos_static_assert(KHRN_HW_QPUS_N != 1); }
   if (_count(halted) != KHRN_HW_QPUS_N) {
#endif
      vcos_assert(_count(halted) == 1); /* exactly 1 qpu should be stopped */
      v3d_write(DBQRUN, halted); /* set it going again */
#ifdef VC_QDEBUG
   }
#endif
#endif

   return true; /* responded to a request */
}

#ifdef SIMPENROSE
static bool can_submit_user_shader(void)
{
   return true;
}

static void submit_user_shader(uint32_t shader, uint32_t unifs)
{
   unsigned int const *const *annotations;
   static uint32_t qpu_num = 0;
   if (shader == init_shader) { annotations = vg_tess_init_shader_annotations; }
   else if (shader == term_shader) { annotations = vg_tess_term_shader_annotations; }
   else if (shader == fill_shader) { annotations = vg_tess_fill_shader_annotations; }
   else if (shader == stroke_shader) { annotations = vg_tess_stroke_shader_annotations; }
   else { UNREACHABLE(); annotations = NULL; }
   qpu_num = (qpu_num + 1) % KHRN_HW_QPUS_N;
#ifdef SIMPENROSE_QDEBUG
   if ((shader != init_shader) && (shader != term_shader)) {
      simpenrose_debug_annotated_user_shader(shader, annotations, 0, unifs, 0, qpu_num);
   } else
#endif
      simpenrose_do_annotated_user_shader(shader, annotations, 0, unifs, 0, qpu_num);
}

static uint32_t acquire_n = 0;

static int vg_tess_isr(void)
{
   return feed() ? 1 : 0; /* ignore next breakpoint if it asked for more memory */
}

static bool acquire_v3d(void)
{
   if (acquire_n++ == 0) {
      simpenrose_set_qpu_isr(vg_tess_isr);

      vcos_assert(shader_if[0] == -2);
      submit_user_shader(init_shader, 0);
      vcos_assert(shader_if[0] == -1);
   }
   return true;
}

static void release_v3d(void)
{
   if (--acquire_n == 0) {
      vcos_assert(shader_if[0] == -1);
      submit_user_shader(term_shader, 0);
      vcos_assert(shader_if[0] == -2);

      simpenrose_set_qpu_isr(NULL);
   }
}

#if defined(SIMPENROSE_RECORD_OUTPUT) && defined(SIMPENROSE_RECORD_BINNING)
static void record_map_tess(const uint32_t *p)
{
   for (;;) {
      const uint32_t *q = p;
      if (*q != 0x10010101) {
         do {
            ++q;
         } while (q[-1] != 0xbfff0000);
      }
      vcos_assert((*q == 0x10010101) || (*q == KHRN_HW_INSTR_RETURN));
      record_map_buffer(khrn_hw_addr(p), ((q - p) + (1 + (*q == 0x10010101))) * 4, L_INSTRUCTIONS, RECORD_BUFFER_IS_BOTH, 1);
      if (*q == KHRN_HW_INSTR_RETURN) { break; }
      p = (const uint32_t *)khrn_hw_unaddr(q[1]);
   }
}
#endif
#else
static bool can_submit_user_shader(void)
{
   return (v3d_read(SRQCS) & 0x7f) != KHRN_HW_USER_QUEUE_LENGTH;
}

static void submit_user_shader(uint32_t shader, uint32_t unifs)
{
#ifdef VC_QDEBUG
   if ((shader != init_shader) && (shader != term_shader)) {
      v3d_write(DBQHLT, -1);
   }
#endif
   khrn_hw_full_memory_barrier();
   v3d_write(SRQUA, unifs);
   v3d_write(SRQPC, shader);
   vcos_assert(!(v3d_read(SRQCS) & (1 << 7))); /* check we didn't overflow the fifo */
#ifdef VC_QDEBUG
   if ((shader != init_shader) && (shader != term_shader)) {
      uint32_t i = KHRN_HW_QPUS_N - 1; /* user shaders scheduled "top-down" */
      qdebug_set_base(shader);
      do {
         qdebug_attach(&i);
      } while (feed());
      v3d_write(DBQRUN, -1);
   }
#endif
}

static bool waiting_for_acquire_callback = false; /* just a (probably unnecessary) optimisation */
static uint32_t acquire_n = 0;
/* if acquire_n is 0, we're waiting for a term shader, otherwise, we're waiting
 * for an init shader (and acquire_n should be 1) */
static bool waiting_for_init_term_shader = false;

static void acquire_callback(void *p)
{
   UNUSED(p);
   waiting_for_acquire_callback = false;
   khrn_llat_notify(llat_i);
}

static void vg_tess_isr(uint32_t flags)
{
   v3d_write(DBQITC, flags);
   khrn_hw_full_memory_barrier();

   khrn_llat_notify(llat_i);
}

static bool acquire_v3d(void)
{
   if (waiting_for_init_term_shader) {
      vcos_assert(!acquire_n || (acquire_n == 1));
      if (acquire_n && (shader_if[0] == -1)) {
         /* init shader is done... */
         waiting_for_init_term_shader = false;
         return true;
      }
      return false;
   }

   if (waiting_for_acquire_callback) {
      return false;
   }
   khrn_barrier();
   waiting_for_acquire_callback = true;
   if (v3d_get_func_table()->acquire_ush(v3d_driver_handle, NULL, 16, 16, 0)) {
      return false;
   }
   waiting_for_acquire_callback = false;

   if (acquire_n++ != 0) {
      return true;
   }

   /* this stuff should have been left in a sensible state by the last guy... */
   vcos_assert(!(v3d_read(SRQCS) & (1 << 7))); /* queue error bit shouldn't be set */
   vcos_assert(!(((v3d_read(SRQCS) >> 16) ^ (v3d_read(SRQCS) >> 8)) & 0xff)); /* requested count should match completed count */
   v3d_write(SRQCS, (1 << 7) | (1 << 8) | (1 << 16)); /* reset error bit and counts just incase... */

   vcos_assert(!v3d_read(DBQITC)); /* interrupts should be low */
   v3d_write(DBQITC, -1); /* clear them just incase */
   v3d_write(DBQITE, -1); /* make sure qpu interrupts are enabled */

   /* kick off init shader to restore persistent state */
   vcos_assert(shader_if[0] == -2);
   vcos_assert(can_submit_user_shader());
   /* unifs aren't used, but the pointer needs to be valid and in the right
    * alias or hardware can lockup! */
   submit_user_shader(init_shader, io_chunk_unifs);

   waiting_for_init_term_shader = true;
   return false;
}

static void release_v3d(void)
{
   if (--acquire_n == 0) {
      /* kick off term shader to save persistent state */
      vcos_assert(shader_if[0] == -1);
      vcos_assert(can_submit_user_shader());
      /* unifs aren't used, but the pointer needs to be valid and in the right
       * alias or hardware can lockup! */
      submit_user_shader(term_shader, io_chunk_unifs);

      waiting_for_init_term_shader = true;
   } else {
      v3d_get_func_table()->release_ush(v3d_driver_handle, 16);
   }
}

static void finish_release(void)
{
   if (waiting_for_init_term_shader &&
      !acquire_n && (shader_if[0] == -2)) {
      uint32_t srqcs;

      /* term shader is done... */
      waiting_for_init_term_shader = false;

      /* although we've seen all the "done" writes from our shaders, one might
       * still be running and generate an interrupt. wait until all shaders have
       * really finished (the qpu scheduler counts requested and completed
       * shaders) and there are no outstanding qpu interrupts (this should take
       * very few cycles, so just busy loop) */
      do {
         srqcs = v3d_read(SRQCS);
      } while ((((srqcs >> 8) ^ (srqcs >> 16)) & 0xff) || v3d_read(DBQITC));

      /* now actually release the hardware */
      v3d_get_func_table()->release_ush(v3d_driver_handle, 16);
   }
}
#endif

#endif

static uint32_t get_nmem_pos(void)
{
   /*
      we want to return the most recent nmem pos we can with the requirement
      that everything ahead of it will eventually complete even if we stall the
      tessellation fifo

      if there are no nmem-holding waits in the tessellation fifo, we can wait
      for anything in the nmem fifo (we require that vg_tess_nmem_enter be
      called before khrn_nmem_enter, so we are guaranteed to see the wait in the
      tessellation fifo if we see the corresponding entry in the nmem fifo)

      if there are nmem-holding waits in the tessellation fifo, we can wait for
      the last nmem pos that left the tessellation fifo, and anything ahead of
      it
   */

   uint32_t enter_pos = khrn_nmem_get_enter_pos();
   uint32_t wait_enter_pos = wait_enter_nmem_pos, wait_exit_pos;
   khrn_barrier();
   wait_exit_pos = wait_exit_nmem_pos;
   return (wait_enter_pos == wait_exit_pos) ? enter_pos : (wait_exit_pos + 1);
}

static void vg_tess_llat_callback(void)
{
   bool notify_master = false;

#if defined(VG_TESS_QPU) && !defined(SIMPENROSE)
   feed(); /* provide qpus with memory if needed */
   finish_release(); /* handle term shader finishing */
#endif

   while (more_msgs(vg_tess_msg.submit, vg_tess_msg.post)) {
      MSG_HEADER_T *h = (MSG_HEADER_T *)vg_tess_msg.submit;
      switch (h->type) {
      case MSG_TYPE_NOP:
      case MSG_TYPE_WAIT:
      case MSG_TYPE_REP:
      case MSG_TYPE_FILL_BBOX: break;
      case MSG_TYPE_FILL_STROKE:
      {
         MSG_FILL_STROKE_T *msg = (MSG_FILL_STROKE_T *)(h + 1);
#ifndef VG_TESS_QPU
         REP_T *r;
#else
         static bool skip = false;
         uint32_t unifs;
#endif

         if (msg->tess->u.in_use.ok &&
            khrn_nmem_should_wait(false, get_nmem_pos())) {
            goto submit_loop_end;
         }

#ifdef VG_TESS_QPU
         if (!skip) {
            /* to keep things simple, we always acquire (we're never supposed to
             * give up on an acquire, and we unconditionally release in cleanup) */
            if (!acquire_v3d()) {
               goto submit_loop_end;
            }
#endif

            /* we always want to lock chunks_a/chunks_b -- we unconditionally
             * unlock them when we cleanup (in the loop after this one) */
            msg->io_chunk->i.chunks_a = khrn_hw_addr(mem_lock(msg->chunks_a));
            msg->io_chunk->i.chunks_b = (msg->chunks_b == MEM_INVALID_HANDLE) ? 0 : khrn_hw_addr(mem_lock(msg->chunks_b));
#ifdef VG_TESS_QPU
         }
#endif

         if (!msg->tess->u.in_use.ok) {
#ifdef VG_TESS_QPU
            skip = false;
            msg->io_chunk->o.done = -1; /* pretend the shader is done */
#endif
            break;
         }

#ifdef VG_TESS_QPU
         if (!can_submit_user_shader()) {
            skip = true; /* skip the stuff before this next time (so we don't acquire v3d twice etc) */
            goto submit_loop_end;
         }
         skip = false;

         msg->io_chunk->i.tess_i = msg->tess - tesss;
#else
         msg->io_chunk->i.tess = msg->tess;
#endif
         vcos_assert((msg->chunks_b == MEM_INVALID_HANDLE) || (mem_get_size(msg->chunks_b) == mem_get_size(msg->chunks_a)));
         msg->io_chunk->i.chunks_size = mem_get_size(msg->chunks_a);
#ifdef VG_TESS_QPU
         if (!msg->tess->u.in_use.flipped_bit) {
            msg->tess->flipbit = !msg->tess->flipbit;
            msg->tess->u.in_use.flipped_bit = true;
         }
         msg->io_chunk->i.flipbit = msg->tess->flipbit;

         unifs = io_chunk_unifs + ((msg->io_chunk - io_chunks) * 4);
#ifdef VG_TESS_DUMP_CLIF
         dump_clif_add_shader(&msg->tess->u.in_use.dc, msg->shader, unifs, msg->io_chunks_n);
#endif
         submit_user_shader(msg->shader, unifs);
#else
         r = (REP_T *)alloc(&msg->tess->u.in_use.v_allocator, sizeof(REP_T),
            &msg->tess->u.in_use.nmem_group);
         if (!r || !msg->f(msg->io_chunk, msg->r)) {
            msg->tess->u.in_use.ok = false;
            notify_master = true; /* might be waiting in alloc_rep (not strictly necessary as we'll notify when this message is cleaned up) */
            break;
         }
         msg->r = r;
#endif

         break;
      }
      default:
      {
         UNREACHABLE();
      }
      }
      vg_tess_msg.submit = next_msg(vg_tess_msg.submit, h->size);
   }
submit_loop_end:

   while (vg_tess_msg.done != vg_tess_msg.submit) {
      MSG_HEADER_T *h = (MSG_HEADER_T *)vg_tess_msg.done;
      switch (h->type) {
      case MSG_TYPE_NOP: break;
      case MSG_TYPE_WAIT:
      {
         MSG_WAIT_T *msg = (MSG_WAIT_T *)(h + 1);
         bool ok;

         vcos_assert(msg->tess->u.in_use.state == TESS_STATE_WAIT_CALLED);

         ok = msg->tess->u.in_use.ok;

         if (msg->tess->u.in_use.nmem_entered) {
            wait_exit_nmem_pos = msg->tess->u.in_use.nmem_pos; /* todo: (VG_TESS_QPU) could put in submit loop */
         }

         khrn_barrier();
         msg->tess->u.in_use.state = TESS_STATE_WAIT_DONE;

         msg->callback(ok, msg->p);

         break;
      }
      case MSG_TYPE_FILL_STROKE:
      {
         MSG_FILL_STROKE_T *msg = (MSG_FILL_STROKE_T *)(h + 1);

#ifdef VG_TESS_QPU
         if (msg->io_chunk->o.done != -1) {
            goto done_loop_end;
         }
         khrn_barrier();

         release_v3d();

         if (msg->tess->u.in_use.ok) {
            uint8_t *p = msg->r->in_use.ind_branch_dest;
            uint32_t min_xy, max_xy;

            Add_byte(p, KHRN_HW_INSTR_BRANCH);
            ADD_WORD(p, msg->io_chunk->o.branch);
            min_xy = (msg->io_chunk->o.min_x & 0xffff) | (msg->io_chunk->o.min_y << 16);
            max_xy = (msg->io_chunk->o.max_x & 0xffff) | (msg->io_chunk->o.max_y << 16);
            p = rect_do(p, min_xy, max_xy);
            Add_byte(p, KHRN_HW_INSTR_RETURN);
            /* rep will be a direct pointer. we've allocated enough space to put
             * the REP_T on its own cache line, so we can use a normal alias for it */
            vcos_assert(!(msg->io_chunk->o.rep & 0x3)); /* should be 4-byte aligned */
            vcos_assert((_max(KHRN_HW_SYSTEM_CACHE_LINE_SIZE - 4, 0) +
               round_up(sizeof(REP_T), KHRN_HW_SYSTEM_CACHE_LINE_SIZE)) <= 60); /* if this fires, need to allocate more space for rep in shaders */
            add_rep(msg->tess, (REP_T *)khrn_hw_alias_normal(khrn_hw_unaddr(
               round_up(msg->io_chunk->o.rep, KHRN_HW_SYSTEM_CACHE_LINE_SIZE))));
         }
#else
         if (msg->tess->u.in_use.ok) {
            add_rep(msg->tess, msg->r);
         }
#endif

         if (msg->chunks_b != MEM_INVALID_HANDLE) {
            mem_unlock(msg->chunks_b);
            mem_unretain(msg->chunks_b);
            mem_release(msg->chunks_b);
         }
         mem_unlock(msg->chunks_a);
         mem_unretain(msg->chunks_a);
         mem_release(msg->chunks_a);

         free_io_chunk(msg->io_chunk);
         while (--msg->io_chunks_n) {
            free_io_chunk(NULL);
         }

         break;
      }
#ifndef VG_TESS_QPU
      case MSG_TYPE_REP:
      {
         MSG_REP_T *msg = (MSG_REP_T *)(h + 1);
         LISTS_FOOTER_T *f;

         if (!msg->tess->u.in_use.ok) { /* (VG_TESS_QPU) we don't actually need to check this, but may as well be consistent */
            break;
         }

         f = rep_do(&msg->tess->u.in_use.prim_list_allocator, &msg->tess->u.in_use.nmem_group, msg->branch, msg->r, msg->fill);
         if (!f) {
            msg->tess->u.in_use.ok = false;
            break;
         }

         f->next = msg->tess->u.in_use.f_head;
         msg->tess->u.in_use.f_head = f;
         break;
      }
      case MSG_TYPE_FILL_BBOX:
      {
         MSG_FILL_BBOX_T *msg = (MSG_FILL_BBOX_T *)(h + 1);

         if (!msg->tess->u.in_use.ok) { /* we don't actually need to check this, but may as well be consistent */
            break;
         }

         rect_do(msg->rect, msg->r->in_use.min_xy, msg->r->in_use.max_xy);

         break;
      }
#endif
      default:
      {
         UNREACHABLE();
      }
      }
      advance_msg(&vg_tess_msg.done, h->size);
      notify_master = true; /* alloc_rep potentially waiting for add_rep or !ok, alloc_io_chunk potentially waiting for free_io_chunk, post_begin potentially waiting for advance_msg */
   }
done_loop_end:

   if (notify_master) {
      khrn_sync_notify_master();
   }
}
