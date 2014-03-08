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
#include "middleware/khronos/common/2708/khrn_prod_4.h"
#include "middleware/khronos/common/khrn_image.h"
#include "interface/khronos/common/khrn_int_util.h"

/*
   is the format ok for rendering to? this is only for color buffer formats, not
   z/stencil formats etc
*/

bool khrn_image_is_ok_for_render_target(KHRN_IMAGE_FORMAT_T format, bool ignore_mem_layout)
{
   vcos_assert(khrn_image_is_color(format));
   switch (format & ~(IMAGE_FORMAT_MEM_LAYOUT_MASK | IMAGE_FORMAT_PRE | IMAGE_FORMAT_LIN)) {
   case ABGR_8888:
   case XBGR_8888:
   case RGB_565:
      /* need tformat on a0 for vg, but allow rso anyway as it's faster and will
       * only appear when we build with RSO_FRAMEBUFFER defined (in which case
       * we should be aware that some stuff might not work) */
      return ignore_mem_layout || khrn_image_is_tformat(format) || khrn_image_is_rso(format)
#ifndef __BCM2708A0__
         || khrn_image_is_lineartile(format)
#endif
         ;
   default:
      return false;
   }
}

/*
   should only be called if khrn_image_is_ok_for_render_target(true) says yes.
   checks the other image properties, eg memory layout, stride, size, alignment.
   again, this is only for color buffers, not z/stencil etc
*/

bool khrn_image_can_use_as_render_target(KHRN_IMAGE_T *image)
{
   vcos_assert(khrn_image_is_ok_for_render_target(image->format, true));
   if (/* check the memory layout */
      !khrn_image_is_ok_for_render_target(image->format, false) ||
#ifdef __BCM2708A0__
      /* we pad width/height up to multiple of non-ms tile size (at least in
       * non-ms mode) when setting up tlb, so there had better be enough space... */
      (image->stride < (int32_t)khrn_image_get_stride(image->format, round_up(image->width, KHRN_HW_TILE_WIDTH))) ||
      (mem_get_size(image->mh_storage) < khrn_image_get_size(image->format,
      round_up(image->width, KHRN_HW_TILE_WIDTH), round_up(image->height, KHRN_HW_TILE_HEIGHT))) ||
      /* need for vg hw-2371 workaround */
      (khrn_image_get_align(image) < KHRN_HW_TEX_ALIGN) ||
#endif
      (image->stride & (KHRN_HW_TLB_ALIGN - 1)) ||
      (khrn_image_get_align(image) < KHRN_HW_TLB_ALIGN)) {
      vcos_assert(!(image->flags & IMAGE_FLAG_RENDER_TARGET));
      return false;
   }
   image->flags |= IMAGE_FLAG_RENDER_TARGET;
   return true;
}

/*
   fudge the image parameters to make it ok for the requested usage. the render
   target usage includes z/stencil/mask buffers as well as color buffers
*/

void khrn_image_platform_fudge(
   KHRN_IMAGE_FORMAT_T *format,
   uint32_t *padded_width, uint32_t *padded_height,
   uint32_t *align, uint32_t *stagger,
   KHRN_IMAGE_CREATE_FLAG_T flags)
{
   if (flags & IMAGE_CREATE_FLAG_TEXTURE) {
      *format = khrn_image_to_tf_format(*format);
      if (khrn_image_prefer_lt(*format, *padded_width, *padded_height)) {
         *format = khrn_image_to_lt_format(*format);
      }
      *align = _max(*align, KHRN_HW_TEX_ALIGN);
   }
   if (flags & IMAGE_CREATE_FLAG_RENDER_TARGET) {
#ifdef __BCM2708A0__
      if (khrn_image_is_lineartile(*format)) { /* no lt on a0 */
         *format = khrn_image_to_tf_format(*format);
      }
      /* we pad width/height up to multiple of non-ms tile size (at least in
       * non-ms mode) when setting up tlb, make sure there's enough space */
      *padded_width = round_up(*padded_width, KHRN_HW_TILE_WIDTH);
      *padded_height = round_up(*padded_height, KHRN_HW_TILE_HEIGHT);
      /* need for vg hw-2371 workaround. only need for color buffers, doesn't
       * really matter if we increase alignment of other stuff though */
      if (khrn_image_is_color(*format)) {
         *align = _max(*align, KHRN_HW_TEX_ALIGN);
      }
#endif
      /* if it looks like a color buffer, check it's one of the supported
       * formats */
      vcos_assert(!khrn_image_is_color(*format) ||
         (*format == A_8_RSO) || /* A_8_RSO used for mask buffers */
         (*format == COL_32_TLBD) || /* COL_32_TLBD used for multisample buffers */
         khrn_image_is_ok_for_render_target(*format, false));
      /* stride needs to be a multiple of KHRN_HW_TLB_ALIGN. this is trivially
       * satisfied for lineartile and tformat as utiles are 16 bytes wide. need
       * to pad raster order widths though... */
      if (khrn_image_is_rso(*format)) {
         *padded_width = round_up(*padded_width, (KHRN_HW_TLB_ALIGN * 8) / khrn_image_get_bpp(*format));
      }
      *align = _max(*align, KHRN_HW_TLB_ALIGN);
   }
   if (flags & IMAGE_CREATE_FLAG_DISPLAY) {
      *align = _max(*align, 4096); /* need 4k alignment for scaler */
   }
}
