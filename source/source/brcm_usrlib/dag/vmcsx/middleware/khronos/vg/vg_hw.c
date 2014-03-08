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
#include "middleware/khronos/vg/vg_hw.h"
#include "middleware/khronos/vg/vg_mask_layer.h"
#include "middleware/khronos/vg/vg_ramp.h"
#include "middleware/khronos/vg/vg_scissor.h"
#include "interface/khronos/common/khrn_int_color.h"
#include "interface/khronos/common/khrn_int_math.h"

bool vg_be_prepare_scissor(VG_SERVER_STATE_SCISSOR_T *scissor, uint32_t *rect, MEM_HANDLE_T *handle, uint32_t width, uint32_t height)
{
   if (scissor) {
      if (scissor->rects_count == 4) {
         rect[0] = scissor->rects[0];
         rect[1] = scissor->rects[1];
         rect[2] = scissor->rects[2];
         rect[3] = scissor->rects[3];
         khrn_clip_rect(
            (int32_t *)rect + 0, (int32_t *)rect + 1, (int32_t *)rect + 2, (int32_t *)rect + 3,
            0, 0, width, height);
         *handle = MEM_INVALID_HANDLE;
      } else {
         if ((scissor->width != width) || (scissor->height != height)) {
            /*
               there is no frame_size_changed callback, so we check here instead

               note that scissor->scissor may be referenced by someone, so we must
               allocate a new one instead of trying to resize the old one
            */

            if (scissor->scissor != MEM_INVALID_HANDLE) {
               mem_release(scissor->scissor);
               scissor->scissor = MEM_INVALID_HANDLE;
            }
            scissor->width = width;
            scissor->height = height;
         }
         if (((scissor->scissor == MEM_INVALID_HANDLE) &&
            ((scissor->scissor = vg_scissor_alloc()) == MEM_INVALID_HANDLE)) ||
            !vg_scissor_retain(scissor->scissor, scissor->bounds, width, height, scissor->rects, scissor->rects_count)) {
            return false;
         }
         rect[0] = scissor->bounds[0];
         rect[1] = scissor->bounds[1];
         rect[2] = _max(scissor->bounds[2] - scissor->bounds[0], 0);
         rect[3] = _max(scissor->bounds[3] - scissor->bounds[1], 0);
         *handle = scissor->scissor;
      }
   } else {
      rect[0] = 0;
      rect[1] = 0;
      rect[2] = width;
      rect[3] = height;
      *handle = MEM_INVALID_HANDLE;
   }
   return true;
}

bool vg_be_prepare_paint_grad_linear(VG_PAINT_T *paint, VG_MAT3X3_T *surface_to_paint)
{
   float p0_x, p0_y, p1_x, p1_y, d_x, d_y, hyp_sq, oo_hyp;

   vcos_assert(paint->type == VG_PAINT_TYPE_LINEAR_GRADIENT);

   p0_x = paint->linear_gradient[0];
   p0_y = paint->linear_gradient[1];
   p1_x = paint->linear_gradient[2];
   p1_y = paint->linear_gradient[3];
   d_x = p1_x - p0_x;
   d_y = p1_y - p0_y;
   hyp_sq = (d_x * d_x) + (d_y * d_y);

   if (hyp_sq < EPS) {
      return true; /* degenerate! */
   }

   /*
      rotate/translate/scale the matrix so p0 = (0, 0) and p1 = (1, 0)
      (and apply biases to get correct sample position)
   */

   oo_hyp = rsqrt_(hyp_sq);

   vg_mat3x3_premul_translate(surface_to_paint, -p0_x, -p0_y);
   vg_mat3x3_premul_rotate_sc(surface_to_paint, -d_y * oo_hyp, d_x * oo_hyp);
   vg_mat3x3_premul_scale(surface_to_paint, oo_hyp, 0.0f);
   vg_mat3x3_postmul_translate(surface_to_paint, 0.5f, 0.5f);                       /* pixel samples at (x + 0.5, y + 0.5) */
   vg_mat3x3_premul_translate(surface_to_paint, 0.5f / (float)VG_RAMP_WIDTH, 0.0f); /* see vg_ramp.c */
#ifndef NDEBUG
   vcos_verify(vg_mat3x3_is_affine(surface_to_paint)); /* why vcos_verify? see vg_mat3x3_is_affine... */
#endif

   return false; /* not degenerate */
}

bool vg_be_prepare_paint_grad_radial(VG_PAINT_T *paint, VG_MAT3X3_T *surface_to_paint, float *u, float *v)
{
   float c_x, c_y, f_x, f_y, r, oo_r;
   float d_x, d_y, hyp_sq;
   float x; /* new c_x */

   vcos_assert(paint->type == VG_PAINT_TYPE_RADIAL_GRADIENT);

   c_x = paint->radial_gradient[0];
   c_y = paint->radial_gradient[1];
   f_x = paint->radial_gradient[2];
   f_y = paint->radial_gradient[3];
   r = paint->radial_gradient[4];

   if (r < EPS) {
      return true; /* degenerate! */
   }

   /*
      rotate/translate/scale the matrix so f = (0, 0), c_y = 0, and r = 1
      (and apply biases to get correct sample position)
   */

   oo_r = recip_(r);

   d_x = c_x - f_x;
   d_y = c_y - f_y;
   hyp_sq = (d_x * d_x) + (d_y * d_y);

   if (hyp_sq < EPS) {
      /*
         focal point very close to center point
      */

      x = 0.0f;
      vg_mat3x3_premul_translate(surface_to_paint, -f_x, -f_y);
   } else {
      float oo_hyp = rsqrt_(hyp_sq);
      float oo_hyp_99 = oo_hyp * 0.99f;
      if (oo_hyp_99 < oo_r) {
         /*
            hyp > 0.99r
            focal point too far from center point
         */

         x = 0.99f;
         vg_mat3x3_premul_translate(surface_to_paint,
            (d_x * oo_hyp_99 * r) - c_x,
            (d_y * oo_hyp_99 * r) - c_y);
      } else {
         x = (hyp_sq * oo_hyp) * oo_r;
         vg_mat3x3_premul_translate(surface_to_paint, -f_x, -f_y);
      }
      vg_mat3x3_premul_rotate_sc(surface_to_paint, -d_y * oo_hyp, d_x * oo_hyp);
   }
   vg_mat3x3_premul_scale(surface_to_paint, oo_r, oo_r);
   vg_mat3x3_postmul_translate(surface_to_paint, 0.5f, 0.5f); /* pixel samples at (x + 0.5, y + 0.5) */
#ifndef NDEBUG
   vcos_verify(vg_mat3x3_is_affine(surface_to_paint)); /* why vcos_verify? see vg_mat3x3_is_affine... */
#endif

   /*
      u = 1 / (1 - c_x^2)
      v = -u * c_x
   */

   *u = recip_(1 - (x * x));
   *v = -*u * x;

   return false; /* not degenerate */
}

static INLINE bool convert_allowed_quality(uint32_t allowed_quality)
{
   return (allowed_quality & VG_IMAGE_QUALITY_FASTER) ||
          (allowed_quality & VG_IMAGE_QUALITY_BETTER);
}

MEM_HANDLE_T vg_be_prepare_paint_pattern(VG_PAINT_T *paint, VG_MAT3X3_T *surface_to_paint,
   KHRN_IMAGE_FORMAT_T *format, uint32_t *width_height, bool *bilinear, uint32_t *child_rect, uint32_t *tile_fill_rgba)
{
   MEM_HANDLE_T handle, image_handle;
   VG_IMAGE_T *image;

   vcos_assert(paint->type == VG_PAINT_TYPE_PATTERN);

   handle = paint->pattern;
   image = vg_image_lock_adam(child_rect + 0, child_rect + 1, child_rect + 2, child_rect + 3, &handle);

   image_handle = image->image;

   vg_mat3x3_postmul_translate(surface_to_paint, 0.5f, 0.5f); /* pixel samples at (x + 0.5, y + 0.5) */
   vg_mat3x3_premul_scale(surface_to_paint, recip_((float)child_rect[2]), recip_((float)child_rect[3]));
#ifndef NDEBUG
   vcos_verify(vg_mat3x3_is_affine(surface_to_paint)); /* why vcos_verify? see vg_mat3x3_is_affine... */
#endif

   if (format) {
      *format = image->image_format;
   }
   if (width_height) {
      width_height[0] = image->image_width;
      width_height[1] = image->image_height;
   }

   *bilinear = convert_allowed_quality(image->allowed_quality);

   if (image->image_format & IMAGE_FORMAT_LIN) {
      *tile_fill_rgba = khrn_color_rgba_s_to_lin(*tile_fill_rgba);
   }
   if (!(image->image_format & IMAGE_FORMAT_A) || (image->image_format & IMAGE_FORMAT_PRE)) { /* avoid pre in shader for unpre images without alpha */
      *tile_fill_rgba = khrn_color_rgba_pre(*tile_fill_rgba);
   }

   mem_unlock(handle);

   return image_handle;
}

MEM_HANDLE_T vg_be_prepare_image(MEM_HANDLE_T handle,
   KHRN_IMAGE_FORMAT_T *format, uint32_t *width_height, bool *bilinear)
{
   MEM_HANDLE_T image_handle;
   if (vg_is_mask_layer(handle)) {
      VG_MASK_LAYER_T *mask_layer = (VG_MASK_LAYER_T *)mem_lock(handle);
      image_handle = mask_layer->image;
      if (format) {
         *format = vg_mask_layer_get_format(mask_layer->image_width, mask_layer->image_height);
      }
      if (width_height) {
         width_height[0] = mask_layer->image_width;
         width_height[1] = mask_layer->image_height;
      }
      if (bilinear) {
         *bilinear = false;
      }
      mem_unlock(handle);
   } else {
      VG_IMAGE_T *image;
      vcos_assert(vg_is_image(handle));
      image = (VG_IMAGE_T *)mem_lock(handle);
      image_handle = image->image;
      if (format) {
         *format = image->image_format;
      }
      if (width_height) {
         width_height[0] = image->image_width;
         width_height[1] = image->image_height;
      }
      if (bilinear) {
         *bilinear = convert_allowed_quality(image->allowed_quality);
      }
      mem_unlock(handle);
   }
   return image_handle;
}
