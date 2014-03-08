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
#include "middleware/khronos/vg/vg_scissor.h"
#include "middleware/khronos/common/khrn_image.h"
#include "interface/khronos/common/khrn_int_util.h"
#include <string.h>

MEM_HANDLE_T vg_scissor_alloc(void)
{
   return mem_alloc_ex(
      0, /* don't allocate yet, just get a handle */
      VG_SCISSOR_ALIGN,
      (MEM_FLAG_T)(MEM_FLAG_DISCARDABLE | MEM_FLAG_DIRECT | MEM_FLAG_RESIZEABLE | MEM_FLAG_ZERO | MEM_FLAG_INIT),
      "vg_scissor",
      MEM_COMPACT_DISCARD);
}

static void vg_scissor_gen(
   void *data, uint32_t stride, int32_t *bounds,
   KHRN_IMAGE_FORMAT_T format, uint32_t width, uint32_t height,
   const int32_t *rects, uint32_t rects_count)
{
   KHRN_IMAGE_WRAP_T wrap;
   uint32_t i;

   /* data already initialised to 0 by virtue of MEM_FLAG_ZERO and MEM_FLAG_INIT */
   bounds[0] = width;
   bounds[1] = height;
   bounds[2] = 0;
   bounds[3] = 0;

   khrn_image_wrap(&wrap, format, width, height, stride, data);
   for (i = 0; i != rects_count; i += 4) {
      int32_t rect_x      = rects[i];
      int32_t rect_y      = rects[i + 1];
      int32_t rect_width  = rects[i + 2];
      int32_t rect_height = rects[i + 3];
      khrn_clip_rect(
         &rect_x, &rect_y, &rect_width, &rect_height,
         0, 0, width, height);
      khrn_image_wrap_clear_region(&wrap, rect_x, rect_y, rect_width, rect_height, 0xffffffff, IMAGE_CONV_GL);
      bounds[0] = _min(bounds[0], rect_x);
      bounds[1] = _min(bounds[1], rect_y);
      bounds[2] = _max(bounds[2], rect_x + rect_width);
      bounds[3] = _max(bounds[3], rect_y + rect_height);
   }
}

bool vg_scissor_retain(
   MEM_HANDLE_T handle, int32_t *bounds,
   uint32_t width, uint32_t height,
   const int32_t *rects, uint32_t rects_count)
{
   if (!mem_retain(handle)) {
      KHRN_IMAGE_FORMAT_T format = vg_scissor_get_format(width, height);
      uint32_t stride = khrn_image_get_stride(format, width);
      if (!mem_resize_ex(handle, khrn_image_pad_height(format, height) * stride, MEM_COMPACT_DISCARD)) {
         mem_unretain(handle);
         return false;
      }
      vg_scissor_gen(mem_lock(handle), stride, bounds, format, width, height, rects, rects_count);
      mem_unlock(handle);
   }
   return true;
}
