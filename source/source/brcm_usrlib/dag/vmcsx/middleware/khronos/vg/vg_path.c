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
#include "middleware/khronos/vg/vg_path.h"
#include "middleware/khronos/vg/vg_stem.h"
#include "interface/khronos/vg/vg_int_util.h"
#include "interface/khronos/common/khrn_int_math.h"
#include "interface/khronos/common/khrn_int_util.h"

vcos_static_assert(sizeof(VG_PATH_BPRINT_T) <= VG_STEM_SIZE);
vcos_static_assert(alignof(VG_PATH_BPRINT_T) <= VG_STEM_ALIGN);

void vg_path_bprint_term(void *p, uint32_t size)
{
   UNUSED(p);
   UNUSED(size);
}

void vg_path_bprint_from_stem(
   MEM_HANDLE_T handle,
   int32_t format,
   VGPathDatatype datatype,
   uint32_t caps,
   float scale, float bias,
   int32_t segments_capacity,
   int32_t coords_capacity)
{
   VG_PATH_BPRINT_T *path_bprint;

   vcos_assert(vg_is_stem(handle));
   vcos_assert(is_path_format(format) && is_path_datatype(datatype) && !is_zero(scale)); /* checks performed on client-side */

   path_bprint = (VG_PATH_BPRINT_T *)mem_lock(handle);
   path_bprint->format = format;
   path_bprint->datatype = datatype;
   path_bprint->caps = caps;
   path_bprint->scale = scale;
   path_bprint->bias = bias;
   path_bprint->segments_capacity = segments_capacity;
   path_bprint->coords_capacity = coords_capacity;
   mem_unlock(handle);

   mem_set_desc(handle, "VG_PATH_BPRINT_T");
   mem_set_term(handle, vg_path_bprint_term);
}

vcos_static_assert(sizeof(VG_PATH_T) <= VG_STEM_SIZE);
vcos_static_assert(alignof(VG_PATH_T) <= VG_STEM_ALIGN);

void vg_path_term(void *p, uint32_t size)
{
   VG_PATH_T *path = (VG_PATH_T *)p;

   UNUSED(size);

   vg_path_extra_term(path);

   mem_release(path->coords);
   mem_release(path->segments);
}

bool vg_path_from_bprint(MEM_HANDLE_T handle)
{
   VG_PATH_BPRINT_T *path_bprint;
   VGPathDatatype datatype;
   uint32_t caps;
   float scale, bias;
   MEM_HANDLE_T segments_handle, coords_handle;
   VG_PATH_T *path;

   vcos_assert(vg_is_path_bprint(handle));

   /*
      get init params from blueprint
   */

   path_bprint = (VG_PATH_BPRINT_T *)mem_lock(handle);
   datatype = path_bprint->datatype;
   caps = path_bprint->caps;
   scale = path_bprint->scale;
   bias = path_bprint->bias;
   /* requested capacities are ignored; segments/coords are always exactly sized */
   mem_unlock(handle);

   /*
      alloc segments / coords
   */

   segments_handle = mem_alloc_ex(0, 1, (MEM_FLAG_T)(MEM_FLAG_RESIZEABLE | MEM_FLAG_NO_INIT | MEM_FLAG_HINT_GROW), "VG_PATH_T.segments", MEM_COMPACT_DISCARD);
   if (segments_handle == MEM_INVALID_HANDLE) {
      return false;
   }

   coords_handle = mem_alloc_ex(0, 4, (MEM_FLAG_T)(MEM_FLAG_RESIZEABLE | MEM_FLAG_NO_INIT | MEM_FLAG_HINT_GROW), "VG_PATH_T.coords", MEM_COMPACT_DISCARD);
   if (coords_handle == MEM_INVALID_HANDLE) {
      mem_release(segments_handle);
      return false;
   }

   /*
      fill in struct
   */

   path = (VG_PATH_T *)mem_lock(handle);
   path->flags = VG_PATH_FLAG_EMPTY;
   path->datatype = (uint8_t)datatype;
   path->caps = (uint16_t)caps;
   path->scale = scale;
   path->bias = bias;
   path->segments = segments_handle;
   path->coords = coords_handle;
   path->bounds[0] = FLT_MAX;
   path->bounds[1] = FLT_MAX;
   path->bounds[2] = -FLT_MAX;
   path->bounds[3] = -FLT_MAX;
   vg_path_extra_init(path);
   mem_unlock(handle);

   mem_set_desc(handle, "VG_PATH_T");
   mem_set_term(handle, vg_path_term);

   return true;
}

static int32_t get_interpolate_coords_count(const uint8_t *begin_segments, const uint8_t *end_segments, uint32_t segments_count)
{
   uint32_t coords_count = 0;
   for (; segments_count != 0; ++begin_segments, ++end_segments, --segments_count) {
      uint32_t begin_segment = *begin_segments & VG_SEGMENT_MASK;
      uint32_t end_segment = *end_segments & VG_SEGMENT_MASK;
      if (begin_segment >= VG_SCCWARC_TO) { begin_segment = VG_SCCWARC_TO; }
      else if (begin_segment >= VG_QUAD_TO) { begin_segment = VG_CUBIC_TO; }
      else if (begin_segment >= VG_LINE_TO) { begin_segment = VG_LINE_TO; }
      if (end_segment >= VG_SCCWARC_TO) { end_segment = VG_SCCWARC_TO; }
      else if (end_segment >= VG_QUAD_TO) { end_segment = VG_CUBIC_TO; }
      else if (end_segment >= VG_LINE_TO) { end_segment = VG_LINE_TO; }
      if (begin_segment != end_segment) { return -1; }
      coords_count += get_segment_coords_count(begin_segment);
   }
   return coords_count;
}

VGErrorCode vg_path_interpolate_slow(VG_PATH_T *dst, VG_PATH_T *begin, VG_PATH_T *end, float t)
{
   uint32_t segments_count;
   int32_t dst_coords_count;
   uint32_t dst_coords_size;
   uint8_t *dst_segments;
   const uint8_t *begin_segments, *end_segments;
   void *dst_coords;
   const void *begin_coords, *end_coords;
   float omt, oo_dst_scale;
   float begin_s_x, begin_s_y, begin_o_x, begin_o_y, begin_p_x, begin_p_y;
   float end_s_x, end_s_y, end_o_x, end_o_y, end_p_x, end_p_y;

   /*
      no need for special-case handling of self-append
   */

   segments_count = mem_get_size(begin->segments);
   if (segments_count == mem_get_size(end->segments)) {
      dst_coords_count = get_interpolate_coords_count((const uint8_t *)mem_lock(begin->segments), (const uint8_t *)mem_lock(end->segments), segments_count);
      mem_unlock(end->segments);
      mem_unlock(begin->segments);
   } else {
      dst_coords_count = -1;
   }
   if (dst_coords_count == -1) {
      /*
         incompatible paths are normally detected on the client, but an
         inconsistency between the client and server has arisen. we're supposed
         to return false here, but vgInterpolatePath has already returned.
         illegal argument seems an appropriate error
      */

      return VG_ILLEGAL_ARGUMENT_ERROR;
   }
   dst_coords_size = dst_coords_count * get_path_datatype_size((VGPathDatatype)dst->datatype);

   if (!mem_resize_ex(dst->segments, mem_get_size(dst->segments) + segments_count, MEM_COMPACT_DISCARD)) {
      return VG_OUT_OF_MEMORY_ERROR;
   }
   if (!mem_resize_ex(dst->coords, mem_get_size(dst->coords) + dst_coords_size, MEM_COMPACT_DISCARD)) {
      verify(mem_resize_ex(dst->segments, mem_get_size(dst->segments) - segments_count, MEM_COMPACT_NONE));
      return VG_OUT_OF_MEMORY_ERROR;
   }

   dst_segments = (uint8_t *)mem_lock(dst->segments) + (mem_get_size(dst->segments) - segments_count);
   begin_segments = (const uint8_t *)mem_lock(begin->segments);
   end_segments = (const uint8_t *)mem_lock(end->segments);
   dst_coords = (uint8_t *)mem_lock(dst->coords) + (mem_get_size(dst->coords) - dst_coords_size);
   begin_coords = mem_lock(begin->coords);
   end_coords = mem_lock(end->coords);
   omt = 1.0f - t;
   oo_dst_scale = (dst->scale == 0.0f) ? FLT_MAX : recip_(dst->scale);
   begin_s_x = 0.0f; begin_s_y = 0.0f;
   begin_o_x = 0.0f; begin_o_y = 0.0f;
   begin_p_x = 0.0f; begin_p_y = 0.0f;
   end_s_x = 0.0f; end_s_y = 0.0f;
   end_o_x = 0.0f; end_o_y = 0.0f;
   end_p_x = 0.0f; end_p_y = 0.0f;
   for (; segments_count != 0; ++dst_segments, ++begin_segments, ++end_segments, --segments_count) {
      uint32_t begin_segment, end_segment, segment;
      float begin_segment_coords[6];
      float end_segment_coords[6];
      uint32_t segment_coords_count, i;

      #define GET_COORD(PATH) get_coord((VGPathDatatype)PATH->datatype, PATH->scale, PATH->bias, &PATH##_coords)
      #define PUT_COORD(X) put_coord((VGPathDatatype)dst->datatype, oo_dst_scale, dst->bias, &dst_coords, X)

      begin_segment = *begin_segments;
      end_segment = *end_segments;

      /*
         store the normalised segment type
      */

      segment = ((t < 0.5f) ? begin_segment : end_segment) & VG_SEGMENT_MASK;
      if (segment < VG_SCCWARC_TO) {
         if (segment >= VG_QUAD_TO) { segment = VG_CUBIC_TO; }
         else if (segment >= VG_LINE_TO) { segment = VG_LINE_TO; }
      }
      *dst_segments = (uint8_t)segment;

      /*
         get the normalised coords of the begin/end segments
      */

      #define GET_COORDS(PATH) \
         do { \
            switch (PATH##_segment & VG_SEGMENT_MASK) { \
            case VG_CLOSE_PATH: \
            { \
               PATH##_o_x = PATH##_s_x;  PATH##_o_y = PATH##_s_y; \
               PATH##_p_x = PATH##_s_x;  PATH##_p_y = PATH##_s_y; \
               break; \
            } \
            case VG_MOVE_TO: \
            { \
               float x = GET_COORD(PATH); \
               float y = GET_COORD(PATH); \
               if (PATH##_segment & VG_RELATIVE) { \
                  x += PATH##_o_x;  y += PATH##_o_y; \
               } \
               PATH##_segment_coords[0] = x; \
               PATH##_segment_coords[1] = y; \
               PATH##_s_x = x;  PATH##_s_y = y; \
               PATH##_o_x = x;  PATH##_o_y = y; \
               PATH##_p_x = x;  PATH##_p_y = y; \
               break; \
            } \
            case VG_LINE_TO: \
            case VG_HLINE_TO: \
            case VG_VLINE_TO: \
            { \
               float x, y; \
               if ((PATH##_segment & VG_SEGMENT_MASK) == VG_VLINE_TO) { \
                  x = PATH##_o_x; \
               } else { \
                  x = GET_COORD(PATH); \
                  if (PATH##_segment & VG_RELATIVE) { \
                     x += PATH##_o_x; \
                  } \
               } \
               if ((PATH##_segment & VG_SEGMENT_MASK) == VG_HLINE_TO) { \
                  y = PATH##_o_y; \
               } else { \
                  y = GET_COORD(PATH); \
                  if (PATH##_segment & VG_RELATIVE) { \
                     y += PATH##_o_y; \
                  } \
               } \
               PATH##_segment_coords[0] = x; \
               PATH##_segment_coords[1] = y; \
               PATH##_o_x = x;  PATH##_o_y = y; \
               PATH##_p_x = x;  PATH##_p_y = y; \
               break; \
            } \
            case VG_QUAD_TO: \
            case VG_CUBIC_TO: \
            case VG_SQUAD_TO: \
            case VG_SCUBIC_TO: \
            { \
               float x, y; /* new p_x/p_y */ \
               if (((PATH##_segment & VG_SEGMENT_MASK) == VG_QUAD_TO) || ((PATH##_segment & VG_SEGMENT_MASK) == VG_SQUAD_TO)) { \
                  if ((PATH##_segment & VG_SEGMENT_MASK) == VG_SQUAD_TO) { \
                     x = (2.0f * PATH##_o_x) - PATH##_p_x; \
                     y = (2.0f * PATH##_o_y) - PATH##_p_y; \
                  } else { \
                     x = GET_COORD(PATH); \
                     y = GET_COORD(PATH); \
                     if (PATH##_segment & VG_RELATIVE) { \
                        x += PATH##_o_x;  y += PATH##_o_y; \
                     } \
                  } \
                  PATH##_segment_coords[4] = GET_COORD(PATH); \
                  PATH##_segment_coords[5] = GET_COORD(PATH); \
                  if (PATH##_segment & VG_RELATIVE) { \
                     PATH##_segment_coords[4] += PATH##_o_x; \
                     PATH##_segment_coords[5] += PATH##_o_y; \
                  } \
                  PATH##_segment_coords[0] = (PATH##_o_x + (2.0f * x)) * (1.0f / 3.0f); \
                  PATH##_segment_coords[1] = (PATH##_o_y + (2.0f * y)) * (1.0f / 3.0f); \
                  PATH##_segment_coords[2] = ((2.0f * x) + PATH##_segment_coords[4]) * (1.0f / 3.0f); \
                  PATH##_segment_coords[3] = ((2.0f * y) + PATH##_segment_coords[5]) * (1.0f / 3.0f); \
               } else { \
                  if ((PATH##_segment & VG_SEGMENT_MASK) == VG_SCUBIC_TO) { \
                     PATH##_segment_coords[0] = (2.0f * PATH##_o_x) - PATH##_p_x; \
                     PATH##_segment_coords[1] = (2.0f * PATH##_o_y) - PATH##_p_y; \
                  } else { \
                     PATH##_segment_coords[0] = GET_COORD(PATH); \
                     PATH##_segment_coords[1] = GET_COORD(PATH); \
                     if (PATH##_segment & VG_RELATIVE) { \
                        PATH##_segment_coords[0] += PATH##_o_x; \
                        PATH##_segment_coords[1] += PATH##_o_y; \
                     } \
                  } \
                  x = GET_COORD(PATH); \
                  y = GET_COORD(PATH); \
                  PATH##_segment_coords[4] = GET_COORD(PATH); \
                  PATH##_segment_coords[5] = GET_COORD(PATH); \
                  if (PATH##_segment & VG_RELATIVE) { \
                     x += PATH##_o_x;  y += PATH##_o_y; \
                     PATH##_segment_coords[4] += PATH##_o_x; \
                     PATH##_segment_coords[5] += PATH##_o_y; \
                  } \
                  PATH##_segment_coords[2] = x; \
                  PATH##_segment_coords[3] = y; \
               } \
               PATH##_o_x = PATH##_segment_coords[4];  PATH##_o_y = PATH##_segment_coords[5]; \
               PATH##_p_x = x;  PATH##_p_y = y; \
               break; \
            } \
            case VG_SCCWARC_TO: \
            case VG_SCWARC_TO: \
            case VG_LCCWARC_TO: \
            case VG_LCWARC_TO: \
            { \
               float x, y; \
               PATH##_segment_coords[0] = GET_COORD(PATH); \
               PATH##_segment_coords[1] = GET_COORD(PATH); \
               PATH##_segment_coords[2] = GET_COORD(PATH); \
               x = GET_COORD(PATH); \
               y = GET_COORD(PATH); \
               if (PATH##_segment & VG_RELATIVE) { \
                  x += PATH##_o_x;  y += PATH##_o_y; \
               } \
               PATH##_segment_coords[3] = x; \
               PATH##_segment_coords[4] = y; \
               PATH##_o_x = x;  PATH##_o_y = y; \
               PATH##_p_x = x;  PATH##_p_y = y; \
               break; \
            } \
            default: \
            { \
               UNREACHABLE(); \
            } \
            } \
         } while (0)
      GET_COORDS(begin);
      GET_COORDS(end);
      #undef GET_COORDS

      /*
         interpolate between the begin/end coords
      */

      segment_coords_count = get_segment_coords_count(segment);
      for (i = 0; i != segment_coords_count; ++i) {
         PUT_COORD((begin_segment_coords[i] * omt) + (end_segment_coords[i] * t));
      }

      #undef PUT_COORD
      #undef GET_COORD
   }
   mem_unlock(end->coords);
   mem_unlock(begin->coords);
   mem_unlock(dst->coords);
   mem_unlock(end->segments);
   mem_unlock(begin->segments);
   mem_unlock(dst->segments);

   return VG_NO_ERROR;
}

bool vg_path_is_empty(MEM_HANDLE_T segments_handle)
{
   /*
      empty paths (that will not generate any geometry) are of the form VG_MOVE_TO*
   */

   uint8_t *segments = (uint8_t *)mem_lock(segments_handle);
   uint32_t segments_count = mem_get_size(segments_handle);
   for (; segments_count != 0; ++segments, --segments_count) {
      if ((*segments & VG_SEGMENT_MASK) != VG_MOVE_TO) {
         mem_unlock(segments_handle);
         return false;
      }
      vcos_assert(!(*segments & ~(VG_SEGMENT_MASK | VG_RELATIVE))); /* none of the high bits should be set */
   }
   mem_unlock(segments_handle);
   return true;
}
