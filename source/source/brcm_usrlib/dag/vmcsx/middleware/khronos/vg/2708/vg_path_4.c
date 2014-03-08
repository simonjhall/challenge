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
#include "middleware/khronos/vg/2708/vg_path_priv_4.h"
#include "middleware/khronos/vg/2708/vg_segment_lengths_4.h"
#include "middleware/khronos/vg/2708/vg_tess_priv_4.h"
#include "interface/khronos/vg/vg_int_util.h"

void vg_path_extra_init(VG_PATH_T *path)
{
   path->flags |= VG_PATH_FLAG_BOUNDS_VALID;
   path->extra.n.tess = MEM_INVALID_HANDLE;
   path->extra.n.u.glyph.fill_rep = NULL;
   path->extra.n.u.glyph.stroke_rep = NULL;
}

static void discard_interp_to(VG_PATH_T *path);

void vg_path_extra_term(VG_PATH_T *path)
{
   if (path->flags & VG_PATH_FLAG_INTERP_TO) {
      discard_interp_to(path);
   }
   vcos_assert(!(path->flags & VG_PATH_FLAG_INTERP_TO));
   vcos_assert(!(path->flags & VG_PATH_FLAG_INTERP_FROM)); /* dsts should be holding references to us! */
   if (path->extra.n.tess != MEM_INVALID_HANDLE) {
      mem_release(path->extra.n.tess);
   }
}

uint32_t vg_path_get_segments_size(VG_PATH_T *path)
{
   if (path->flags & VG_PATH_FLAG_INTERP_TO) {
      uint32_t size = mem_get_size(((VG_PATH_T *)mem_lock(path->extra.interp_to.from_a))->segments);
      mem_unlock(path->extra.interp_to.from_a);
      return size;
   } else {
      return mem_get_size(path->segments);
   }
}

uint32_t vg_path_get_coords_size(VG_PATH_T *path)
{
   if (path->flags & VG_PATH_FLAG_INTERP_TO) {
      /* todo: this sucks... */
      uint32_t coords_count = 0;
      VG_PATH_T *begin = (VG_PATH_T *)mem_lock(path->extra.interp_to.from_a);
      uint32_t segments_count = mem_get_size(begin->segments);
      const uint8_t *segments = (const uint8_t *)mem_lock(begin->segments);
      for (; segments_count != 0; ++segments, --segments_count) {
         uint32_t segment = *segments & VG_SEGMENT_MASK;
         vcos_assert(segment < VG_SCCWARC_TO);
         if (segment >= VG_QUAD_TO) { segment = VG_CUBIC_TO; }
         else if (segment >= VG_LINE_TO) { segment = VG_LINE_TO; }
         coords_count += get_segment_coords_count(segment);
      }
      mem_unlock(begin->segments);
      mem_unlock(path->extra.interp_to.from_a);
      return coords_count * get_path_datatype_size((VGPathDatatype)path->datatype);
   } else {
      return mem_get_size(path->coords);
   }
}

bool vg_path_read_immediate(VG_PATH_T *path, VG_PATH_RW_T rw)
{
   UNUSED(rw);

   return !(path->flags & VG_PATH_FLAG_INTERP_TO) || vg_path_bake_interp_to(path);
}

bool vg_path_write_immediate(VG_PATH_T *path, VG_PATH_RW_T rw)
{
   UNUSED(rw);

   return (!(path->flags & VG_PATH_FLAG_INTERP_TO) || vg_path_bake_interp_to(path)) &&
          (!(path->flags & VG_PATH_FLAG_INTERP_FROM) || vg_path_bake_interps_from(path));
}

bool vg_path_are_bounds_valid(VG_PATH_T *path)
{
   if (path->flags & VG_PATH_FLAG_BOUNDS_VALID) { return true; }
   if (path->flags & VG_PATH_FLAG_INTERP_TO) { return false; } /* don't want to force a bake just to get bounds! */
   if (vg_path_tess_retain(path)) {
      mem_unretain(path->extra.n.tess);
      return true;
   }
   return false;
}

bool vg_path_read_tight_bounds_immediate(MEM_HANDLE_T handle, VG_PATH_T *path)
{
   if (!(path->flags & VG_PATH_FLAG_BOUNDS_VALID) || (path->flags & VG_PATH_FLAG_BOUNDS_LOOSE)) {
      if (!vg_path_tess_retain(path)) {
         return false;
      }
      mem_unretain(path->extra.n.tess);
   }
   vcos_assert((path->flags & VG_PATH_FLAG_BOUNDS_VALID) && !(path->flags & VG_PATH_FLAG_BOUNDS_LOOSE));
   return true;
}

bool vg_path_clear(VG_PATH_T *path)
{
   if (path->flags & VG_PATH_FLAG_INTERP_TO) {
      discard_interp_to(path);
      vcos_assert(mem_get_size(path->segments) == 0);
      vcos_assert(mem_get_size(path->coords) == 0);
      return true;
   }
   if ((path->flags & VG_PATH_FLAG_INTERP_FROM) && !vg_path_bake_interps_from(path)) { return false; }
   verify(mem_resize_ex(path->segments, 0, MEM_COMPACT_NONE));
   verify(mem_resize_ex(path->coords, 0, MEM_COMPACT_NONE));
   return true;
}

static bool ok_for_late_interp(const uint8_t *begin_segments, const uint8_t *end_segments, uint32_t segments_count)
{
   /*
      check two things:
      - no arcs
      - paths are compatible
   */

   for (; segments_count != 0; ++begin_segments, ++end_segments, --segments_count) {
      uint32_t begin_segment = *begin_segments & VG_SEGMENT_MASK;
      uint32_t end_segment = *end_segments & VG_SEGMENT_MASK;
      if (begin_segment >= VG_SCCWARC_TO) { return false; }
      if (begin_segment >= VG_QUAD_TO) { begin_segment = VG_CUBIC_TO; }
      else if (begin_segment >= VG_LINE_TO) { begin_segment = VG_LINE_TO; }
      if (end_segment >= VG_SCCWARC_TO) { return false; }
      if (end_segment >= VG_QUAD_TO) { end_segment = VG_CUBIC_TO; }
      else if (end_segment >= VG_LINE_TO) { end_segment = VG_LINE_TO; }
      if (begin_segment != end_segment) { return false; }
   }
   return true;
}

VGErrorCode vg_path_interpolate(
   MEM_HANDLE_T dst_handle, VG_PATH_T *dst,
   MEM_HANDLE_T begin_handle, VG_PATH_T *begin,
   MEM_HANDLE_T end_handle, VG_PATH_T *end,
   float t)
{
   if (!vg_path_write_immediate(dst, VG_PATH_RW_DATA) ||
      !vg_path_read_immediate(begin, VG_PATH_RW_DATA) ||
      !vg_path_read_immediate(end, VG_PATH_RW_DATA)) {
      return VG_OUT_OF_MEMORY_ERROR;
   }

   if ((mem_get_size(dst->segments) == 0) && /* nothing in the dst path at all */
      !(begin->flags & VG_PATH_FLAG_EMPTY)) { /* simplify things by not dealing with this case */
      uint32_t segments_count = mem_get_size(begin->segments);
      if (segments_count == mem_get_size(end->segments)) {
         bool ok = ok_for_late_interp((const uint8_t *)mem_lock(begin->segments), (const uint8_t *)mem_lock(end->segments), segments_count);
         mem_unlock(end->segments);
         mem_unlock(begin->segments);
         if (ok) {
            /* ok to do late interp opt! */

            dst->flags &= ~(VG_PATH_FLAG_EMPTY | VG_PATH_FLAG_BOUNDS_VALID | VG_PATH_FLAG_BOUNDS_LOOSE);
            if (vg_path_are_bounds_valid(begin) &&
               vg_path_are_bounds_valid(end) &&
               (t >= 0.0f) && (t <= 1.0f)) {
               /* union of begin/end bounds. todo: we could do better than this... */
               dst->flags |= VG_PATH_FLAG_BOUNDS_VALID | VG_PATH_FLAG_BOUNDS_LOOSE;
               dst->bounds[0] = _minf(begin->bounds[0], end->bounds[0]);
               dst->bounds[1] = _minf(begin->bounds[1], end->bounds[1]);
               dst->bounds[2] = _maxf(begin->bounds[2], end->bounds[2]);
               dst->bounds[3] = _maxf(begin->bounds[3], end->bounds[3]);
            } else {
               dst->bounds[0] = FLT_MAX;
               dst->bounds[1] = FLT_MAX;
               dst->bounds[2] = -FLT_MAX;
               dst->bounds[3] = -FLT_MAX;
            }
            dst->flags |= VG_PATH_FLAG_INTERP_TO;
            if (dst->extra.n.tess != MEM_INVALID_HANDLE) {
               mem_release(dst->extra.n.tess);
            }
            mem_acquire(begin_handle);
            dst->extra.interp_to.from_a = begin_handle;
            mem_acquire(end_handle);
            dst->extra.interp_to.from_b = end_handle;
            if (!(begin->flags & VG_PATH_FLAG_INTERP_FROM)) {
               begin->flags |= VG_PATH_FLAG_INTERP_FROM;
               begin->extra.n.u.interp_from.head_a = MEM_INVALID_HANDLE;
               begin->extra.n.u.interp_from.head_b = MEM_INVALID_HANDLE;
            }
            dst->extra.interp_to.prev_a = MEM_INVALID_HANDLE;
            dst->extra.interp_to.next_a = begin->extra.n.u.interp_from.head_a;
            if (begin->extra.n.u.interp_from.head_a != MEM_INVALID_HANDLE) {
               ((VG_PATH_T *)mem_lock(begin->extra.n.u.interp_from.head_a))->extra.interp_to.prev_a = dst_handle;
               mem_unlock(begin->extra.n.u.interp_from.head_a);
            }
            begin->extra.n.u.interp_from.head_a = dst_handle;
            if (!(end->flags & VG_PATH_FLAG_INTERP_FROM)) {
               end->flags |= VG_PATH_FLAG_INTERP_FROM;
               end->extra.n.u.interp_from.head_a = MEM_INVALID_HANDLE;
               end->extra.n.u.interp_from.head_b = MEM_INVALID_HANDLE;
            }
            dst->extra.interp_to.prev_b = MEM_INVALID_HANDLE;
            dst->extra.interp_to.next_b = end->extra.n.u.interp_from.head_b;
            if (end->extra.n.u.interp_from.head_b != MEM_INVALID_HANDLE) {
               ((VG_PATH_T *)mem_lock(end->extra.n.u.interp_from.head_b))->extra.interp_to.prev_b = dst_handle;
               mem_unlock(end->extra.n.u.interp_from.head_b);
            }
            end->extra.n.u.interp_from.head_b = dst_handle;
            dst->extra.interp_to.t = t;

            return VG_NO_ERROR;
         } /* else: incompatible or can't do opt due to arcs */
      } /* else: incompatible, but let vg_path_interpolate_slow deal with it */
   } /* else: can't do opt */

   return vg_path_interpolate_slow(dst, begin, end, t);
}

bool vg_path_get_length(MEM_HANDLE_T handle, VG_PATH_T *path, float *length, uint32_t segments_i, uint32_t segments_count)
{
   if (!vg_path_tess_retain(path)) {
      return false;
   }
   *length = vg_segment_lengths_get_length(
      (const uint8_t *)mem_lock(path->segments), segments_i, segments_count,
      mem_lock(path->extra.n.tess));
   mem_unlock(path->extra.n.tess);
   mem_unlock(path->segments);
   mem_unretain(path->extra.n.tess);
   return true;
}

bool vg_path_get_p_t_along(MEM_HANDLE_T handle, VG_PATH_T *path, float *p, float *t, uint32_t segments_i, uint32_t segments_count, float distance)
{
   if (!vg_path_tess_retain(path)) {
      return false;
   }
   vg_segment_lengths_get_p_t_along(
      p, t,
      (const uint8_t *)mem_lock(path->segments), segments_i, segments_count,
      mem_lock(path->extra.n.tess),
      _maxf(distance, 0.0f));
   mem_unlock(path->extra.n.tess);
   mem_unlock(path->segments);
   mem_unretain(path->extra.n.tess);
   return true;
}

void vg_path_changed(VG_PATH_T *path, VG_PATH_ELEMENT_T elements)
{
   if (elements & VG_PATH_ELEMENT_DATA) {
      vcos_assert(!(path->flags & VG_PATH_FLAG_INTERP_FROM)); /* should have called vg_path_bake_interps_from before changing data... */
      if (!(path->flags & VG_PATH_FLAG_INTERP_TO)) {
         path->flags &= ~(VG_PATH_FLAG_EMPTY | VG_PATH_FLAG_BOUNDS_VALID | VG_PATH_FLAG_BOUNDS_LOOSE);
         if (vg_path_is_empty(path->segments)) {
            path->flags |= VG_PATH_FLAG_EMPTY | VG_PATH_FLAG_BOUNDS_VALID;
         }

         path->bounds[0] = FLT_MAX;
         path->bounds[1] = FLT_MAX;
         path->bounds[2] = -FLT_MAX;
         path->bounds[3] = -FLT_MAX;

         if (path->extra.n.tess != MEM_INVALID_HANDLE) {
            mem_release(path->extra.n.tess);
            path->extra.n.tess = MEM_INVALID_HANDLE;
         }

         path->extra.n.u.glyph.fill_rep = NULL;
         path->extra.n.u.glyph.stroke_rep = NULL;
      } /* else: nothing needs to be done: we set everything up in vg_path_interpolate */
   }
}

bool vg_path_tess_retain(VG_PATH_T *path)
{
   vcos_assert(!(path->flags & VG_PATH_FLAG_EMPTY)); /* this case should be handled by the calling code */
   if ((path->flags & VG_PATH_FLAG_INTERP_TO) && !vg_path_bake_interp_to(path)) { return false; }
   if (path->extra.n.tess == MEM_INVALID_HANDLE) {
      path->extra.n.tess = vg_tess_alloc();
      if (path->extra.n.tess == MEM_INVALID_HANDLE) {
         return false;
      }
   }
   if (path->flags & VG_PATH_FLAG_BOUNDS_LOOSE) {
      vcos_assert(path->flags & VG_PATH_FLAG_BOUNDS_VALID);
      path->flags &= ~(VG_PATH_FLAG_BOUNDS_VALID | VG_PATH_FLAG_BOUNDS_LOOSE);
   }
   if (!vg_tess_retain(path->extra.n.tess, (path->flags & VG_PATH_FLAG_BOUNDS_VALID) ? NULL : path->bounds,
      (VGPathDatatype)path->datatype, path->scale, path->bias, path->segments, path->coords)) {
      return false;
   }
   path->flags |= VG_PATH_FLAG_BOUNDS_VALID;
   return true;
}

static void discard_interp_to(VG_PATH_T *path)
{
   VG_PATH_T *begin, *end;

   vcos_assert(path->flags & VG_PATH_FLAG_INTERP_TO);

   /*
      unlink from begin
   */

   begin = (VG_PATH_T *)mem_lock(path->extra.interp_to.from_a);
   if (path->extra.interp_to.prev_a == MEM_INVALID_HANDLE) {
      begin->extra.n.u.interp_from.head_a = path->extra.interp_to.next_a;
   } else {
      ((VG_PATH_T *)mem_lock(path->extra.interp_to.prev_a))->extra.interp_to.next_a = path->extra.interp_to.next_a;
      mem_unlock(path->extra.interp_to.prev_a);
   }
   if (path->extra.interp_to.next_a != MEM_INVALID_HANDLE) {
      ((VG_PATH_T *)mem_lock(path->extra.interp_to.next_a))->extra.interp_to.prev_a = path->extra.interp_to.prev_a;
      mem_unlock(path->extra.interp_to.next_a);
   }
   if ((begin->extra.n.u.interp_from.head_a == MEM_INVALID_HANDLE) &&
      (begin->extra.n.u.interp_from.head_b == MEM_INVALID_HANDLE)) {
      begin->flags &= ~VG_PATH_FLAG_INTERP_FROM;
      begin->extra.n.u.glyph.fill_rep = NULL;
      begin->extra.n.u.glyph.stroke_rep = NULL;
   }
   mem_unlock(path->extra.interp_to.from_a);

   /*
      unlink from end
   */

   end = (VG_PATH_T *)mem_lock(path->extra.interp_to.from_b);
   if (path->extra.interp_to.prev_b == MEM_INVALID_HANDLE) {
      end->extra.n.u.interp_from.head_b = path->extra.interp_to.next_b;
   } else {
      ((VG_PATH_T *)mem_lock(path->extra.interp_to.prev_b))->extra.interp_to.next_b = path->extra.interp_to.next_b;
      mem_unlock(path->extra.interp_to.prev_b);
   }
   if (path->extra.interp_to.next_b != MEM_INVALID_HANDLE) {
      ((VG_PATH_T *)mem_lock(path->extra.interp_to.next_b))->extra.interp_to.prev_b = path->extra.interp_to.prev_b;
      mem_unlock(path->extra.interp_to.next_b);
   }
   if ((end->extra.n.u.interp_from.head_a == MEM_INVALID_HANDLE) &&
      (end->extra.n.u.interp_from.head_b == MEM_INVALID_HANDLE)) {
      end->flags &= ~VG_PATH_FLAG_INTERP_FROM;
      end->extra.n.u.glyph.fill_rep = NULL;
      end->extra.n.u.glyph.stroke_rep = NULL;
   }
   mem_unlock(path->extra.interp_to.from_b);

   /*
      release begin/end and return to normal
   */

   mem_release(path->extra.interp_to.from_a);
   mem_release(path->extra.interp_to.from_b);

   path->flags &= ~VG_PATH_FLAG_INTERP_TO;
   path->extra.n.tess = MEM_INVALID_HANDLE;
   path->extra.n.u.glyph.fill_rep = NULL;
   path->extra.n.u.glyph.stroke_rep = NULL;
}

bool vg_path_bake_interp_to(VG_PATH_T *path)
{
   VG_PATH_T *begin, *end;
   VGErrorCode error;

   vcos_assert(path->flags & VG_PATH_FLAG_INTERP_TO);

   begin = (VG_PATH_T *)mem_lock(path->extra.interp_to.from_a);
   end = (VG_PATH_T *)mem_lock(path->extra.interp_to.from_b);
   error = vg_path_interpolate_slow(path, begin, end, path->extra.interp_to.t);
   mem_unlock(path->extra.interp_to.from_b);
   mem_unlock(path->extra.interp_to.from_a);
   if (error != VG_NO_ERROR) {
      vcos_assert(error == VG_OUT_OF_MEMORY_ERROR); /* we should have already checked path compatibility... */
      return false;
   }

   discard_interp_to(path);
   return true;
}

bool vg_path_bake_interps_from(VG_PATH_T *path)
{
   vcos_assert(path->flags & VG_PATH_FLAG_INTERP_FROM);

   do {
      MEM_HANDLE_T handle = path->extra.n.u.interp_from.head_a;
      if (handle == MEM_INVALID_HANDLE) { handle = path->extra.n.u.interp_from.head_b; }
      if (!vg_path_bake_interp_to((VG_PATH_T *)mem_lock(handle))) {
         mem_unlock(handle);
         return false;
      }
      mem_unlock(handle);
   } while (path->flags & VG_PATH_FLAG_INTERP_FROM);

   return true;
}
