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
#include "middleware/khronos/vg/vg_bf.h"
#include "middleware/khronos/vg/vg_server.h"
#include "interface/khronos/common/khrn_int_color.h"
#include "interface/khronos/common/khrn_int_util.h"
#include "middleware/khronos/common/2708/khrn_interlock_priv_4.h"
#include "middleware/khronos/common/2708/khrn_worker_4.h"
#ifndef SIMPENROSE
   #include "middleware/khronos/common/2708/khrn_bf_4.h"
#endif
#include <stddef.h> /* for offsetof */

void vg_bf_init(void)
{
#ifndef SIMPENROSE
   khrn_bf_sconv_init();
#endif
}

void vg_bf_term(void)
{
#ifndef SIMPENROSE
   khrn_bf_sconv_term();
#endif
}

/******************************************************************************
slow c implementations for when we don't have a vpu
******************************************************************************/

#ifdef SIMPENROSE

static uint32_t get_rgba(const KHRN_IMAGE_WRAP_T *wrap, uint32_t x, uint32_t y, KHRN_IMAGE_FORMAT_T format)
{
   uint32_t rgba = khrn_image_pixel_to_rgba(wrap->format, khrn_image_wrap_get_pixel(wrap, x, y), IMAGE_CONV_VG);
   return khrn_image_rgba_convert_pre_lin(format, wrap->format, rgba);
}

static bool tile_coord(int32_t *x_io, uint32_t l, VGTilingMode tiling_mode)
{
   int32_t x = *x_io;
   if ((x < 0) || ((uint32_t)x >= l)) {
      switch (tiling_mode) {
      case VG_TILE_FILL:
      {
         return false;
      }
      case VG_TILE_PAD:
      {
         x = clampi(x, 0, l - 1);
         break;
      }
      case VG_TILE_REPEAT:
      {
         x = mod(x, l);
         break;
      }
      case VG_TILE_REFLECT:
      {
         x = mod(x, 2 * l);
         if ((uint32_t)x >= l) { x = (2 * l) - (x + 1); }
         break;
      }
      default:
      {
         UNREACHABLE();
      }
      }
      *x_io = x;
   }
   return true;
}

static void put_channel_masked_rgba(KHRN_IMAGE_WRAP_T *wrap, uint32_t x, uint32_t y, uint32_t rgba, uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format)
{
   if (channel_mask == 0xf) {
      rgba = khrn_image_rgba_convert_l_pre_lin(wrap->format, format, rgba);
   } else {
      uint32_t wrap_rgba;
      rgba = khrn_image_rgba_convert_l_pre_lin((KHRN_IMAGE_FORMAT_T)(wrap->format & ~IMAGE_FORMAT_PRE), format, rgba);
      wrap_rgba = khrn_image_pixel_to_rgba(wrap->format, khrn_image_wrap_get_pixel(wrap, x, y), IMAGE_CONV_VG);
      if (wrap->format & IMAGE_FORMAT_PRE) {
         wrap_rgba = khrn_color_rgba_unpre(wrap_rgba);
      }
      if (!(channel_mask & 0x1)) { rgba = (rgba & ~0x000000ff) | (wrap_rgba & 0x000000ff); }
      if (!(channel_mask & 0x2)) { rgba = (rgba & ~0x0000ff00) | (wrap_rgba & 0x0000ff00); }
      if (!(channel_mask & 0x4)) { rgba = (rgba & ~0x00ff0000) | (wrap_rgba & 0x00ff0000); }
      if (!(channel_mask & 0x8)) { rgba = (rgba & ~0xff000000) | (wrap_rgba & 0xff000000); }
      if (wrap->format & IMAGE_FORMAT_PRE) {
         rgba = khrn_color_rgba_pre(rgba);
      }
   }
   khrn_image_wrap_put_pixel(wrap, x, y, khrn_image_rgba_to_pixel(wrap->format, rgba, IMAGE_CONV_VG));
}

static void color_matrix(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format,
   const float *matrix)
{
   uint32_t i, j;

   vcos_assert(khrn_image_is_color(dst->format) && khrn_image_is_color(src->format));
   vcos_assert((dst_x + width) <= dst->width);
   vcos_assert((dst_y + height) <= dst->height);
   vcos_assert((src_x + width) <= src->width);
   vcos_assert((src_y + height) <= src->height);

   for (j = 0; j != height; ++j) {
      for (i = 0; i != width; ++i) {
         uint32_t rgba = get_rgba(src, src_x + i, src_y + j, format);
         float color[4];
         khrn_color_rgba_to_floats(color, rgba);
         rgba = color_floats_to_rgba(
            (matrix[0] * color[0]) + (matrix[4] * color[1]) + (matrix[8] * color[2]) + (matrix[12] * color[3]) + matrix[16],
            (matrix[1] * color[0]) + (matrix[5] * color[1]) + (matrix[9] * color[2]) + (matrix[13] * color[3]) + matrix[17],
            (matrix[2] * color[0]) + (matrix[6] * color[1]) + (matrix[10] * color[2]) + (matrix[14] * color[3]) + matrix[18],
            (matrix[3] * color[0]) + (matrix[7] * color[1]) + (matrix[11] * color[2]) + (matrix[15] * color[3]) + matrix[19]);
         if (format & IMAGE_FORMAT_PRE) {
            rgba = khrn_color_rgba_clamp_to_a(rgba);
         }
         put_channel_masked_rgba(dst, dst_x + i, dst_y + j, rgba, channel_mask, format);
      }
   }
}

static void lookup(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format,
   const uint8_t *red_lut,
   const uint8_t *green_lut,
   const uint8_t *blue_lut,
   const uint8_t *alpha_lut,
   KHRN_IMAGE_FORMAT_T output_format)
{
   uint32_t i, j;

   vcos_assert(khrn_image_is_color(dst->format) && khrn_image_is_color(src->format));
   vcos_assert((dst_x + width) <= dst->width);
   vcos_assert((dst_y + height) <= dst->height);
   vcos_assert((src_x + width) <= src->width);
   vcos_assert((src_y + height) <= src->height);

   for (j = 0; j != height; ++j) {
      for (i = 0; i != width; ++i) {
         uint32_t rgba = get_rgba(src, src_x + i, src_y + j, format);
         rgba =
            ((uint32_t)red_lut[(rgba >> 0) & 0xff] << 0) |
            ((uint32_t)green_lut[(rgba >> 8) & 0xff] << 8) |
            ((uint32_t)blue_lut[(rgba >> 16) & 0xff] << 16) |
            ((uint32_t)alpha_lut[(rgba >> 24) & 0xff] << 24);
         if (output_format & IMAGE_FORMAT_PRE) {
            rgba = khrn_color_rgba_clamp_to_a(rgba);
         }
         put_channel_masked_rgba(dst, dst_x + i, dst_y + j, rgba, channel_mask, output_format);
      }
   }
}

static void lookup_single(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format,
   uint32_t source_channel,
   const uint32_t *lut,
   KHRN_IMAGE_FORMAT_T output_format)
{
   uint32_t i, j;

   vcos_assert(khrn_image_is_color(dst->format) && khrn_image_is_color(src->format));
   vcos_assert((dst_x + width) <= dst->width);
   vcos_assert((dst_y + height) <= dst->height);
   vcos_assert((src_x + width) <= src->width);
   vcos_assert((src_y + height) <= src->height);

   for (j = 0; j != height; ++j) {
      for (i = 0; i != width; ++i) {
         uint32_t rgba = get_rgba(src, src_x + i, src_y + j, format);
         rgba = khrn_color_rgba_flip(lut[(rgba >> (source_channel * 8)) & 0xff]); /* internally use abgr */
         if (output_format & IMAGE_FORMAT_PRE) {
            rgba = khrn_color_rgba_clamp_to_a(rgba);
         }
         put_channel_masked_rgba(dst, dst_x + i, dst_y + j, rgba, channel_mask, output_format);
      }
   }
}

static bool sconv(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y, uint32_t dst_width, uint32_t dst_height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y, uint32_t src_width, uint32_t src_height,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format,
   VGTilingMode tiling_mode, uint32_t tile_fill_rgba,
   int32_t shift_x, int32_t shift_y,
   const int16_t *kernel_x, uint32_t kernel_width,
   const int16_t *kernel_y, uint32_t kernel_height,
   float scale, float bias)
{
   MEM_HANDLE_T temp_handle;
   float *temp;
   uint32_t i, j;
   int32_t kernel_x_sum = 0;
   float tile_fill_color[4];

   vcos_assert(khrn_image_is_color(dst->format) && khrn_image_is_color(src->format));
   vcos_assert((dst_x + dst_width) <= dst->width);
   vcos_assert((dst_y + dst_height) <= dst->height);
   vcos_assert((src_x + src_width) <= src->width);
   vcos_assert((src_y + src_height) <= src->height);

   temp_handle = mem_alloc_ex(src_height * dst_width * 4 * sizeof(float), alignof(float), MEM_FLAG_NO_INIT, "vg_bf_sconv", MEM_COMPACT_DISCARD);
   if (temp_handle == MEM_INVALID_HANDLE) {
      return false;
   }
   temp = (float *)mem_lock(temp_handle);

   for (j = 0; j != src_height; ++j) {
      for (i = 0; i != dst_width; ++i) {
         uint32_t k = ((j * dst_width) + i) * 4, l;
         temp[k + 0] = 0.0f;
         temp[k + 1] = 0.0f;
         temp[k + 2] = 0.0f;
         temp[k + 3] = 0.0f;
         for (l = 0; l != kernel_width; ++l) {
            int32_t x = i + l - shift_x;
            uint32_t rgba = tile_coord(&x, src_width, tiling_mode) ?
               get_rgba(src, src_x + x, src_y + j, format) : tile_fill_rgba;
            float color[4];
            khrn_color_rgba_to_floats(color, rgba);
            temp[k + 0] += kernel_x[kernel_width - 1 - l] * color[0];
            temp[k + 1] += kernel_x[kernel_width - 1 - l] * color[1];
            temp[k + 2] += kernel_x[kernel_width - 1 - l] * color[2];
            temp[k + 3] += kernel_x[kernel_width - 1 - l] * color[3];
         }
      }
   }

   for (i = 0; i != kernel_width; ++i) {
      kernel_x_sum += kernel_x[i];
   }
   khrn_color_rgba_to_floats(tile_fill_color, tile_fill_rgba);
   tile_fill_color[0] *= kernel_x_sum;
   tile_fill_color[1] *= kernel_x_sum;
   tile_fill_color[2] *= kernel_x_sum;
   tile_fill_color[3] *= kernel_x_sum;

   for (j = 0; j != dst_height; ++j) {
      for (i = 0; i != dst_width; ++i) {
         float sum[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
         uint32_t k, rgba;
         for (k = 0; k != kernel_height; ++k) {
            int32_t y = j + k - shift_y;
            float color[4];
            if (tile_coord(&y, src_height, tiling_mode)) {
               uint32_t l = ((y * dst_width) + i) * 4;
               color[0] = temp[l + 0];
               color[1] = temp[l + 1];
               color[2] = temp[l + 2];
               color[3] = temp[l + 3];
            } else {
               color[0] = tile_fill_color[0];
               color[1] = tile_fill_color[1];
               color[2] = tile_fill_color[2];
               color[3] = tile_fill_color[3];
            }
            sum[0] += kernel_y[kernel_height - 1 - k] * color[0];
            sum[1] += kernel_y[kernel_height - 1 - k] * color[1];
            sum[2] += kernel_y[kernel_height - 1 - k] * color[2];
            sum[3] += kernel_y[kernel_height - 1 - k] * color[3];
         }
         rgba = color_floats_to_rgba(
            (sum[0] * scale) + bias, (sum[1] * scale) + bias,
            (sum[2] * scale) + bias, (sum[3] * scale) + bias);
         if (format & IMAGE_FORMAT_PRE) {
            rgba = khrn_color_rgba_clamp_to_a(rgba);
         }
         put_channel_masked_rgba(dst, dst_x + i, dst_y + j, rgba, channel_mask, format);
      }
   }

   mem_unlock(temp_handle);
   mem_release(temp_handle);
   return true;
}

static void conv(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y, uint32_t dst_width, uint32_t dst_height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y, uint32_t src_width, uint32_t src_height,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format,
   VGTilingMode tiling_mode, uint32_t tile_fill_rgba,
   int32_t shift_x, int32_t shift_y,
   const int16_t *kernel, uint32_t kernel_width, uint32_t kernel_height,
   float scale, float bias)
{
   uint32_t i, j;

   vcos_assert(khrn_image_is_color(dst->format) && khrn_image_is_color(src->format));
   vcos_assert((dst_x + dst_width) <= dst->width);
   vcos_assert((dst_y + dst_height) <= dst->height);
   vcos_assert((src_x + src_width) <= src->width);
   vcos_assert((src_y + src_height) <= src->height);

   for (j = 0; j != dst_height; ++j) {
      for (i = 0; i != dst_width; ++i) {
         float sum[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
         uint32_t k, l, rgba;
         for (k = 0; k != kernel_width; ++k) {
            for (l = 0; l != kernel_height; ++l) {
               int32_t x = i + k - shift_x;
               int32_t y = j + l - shift_y;
               uint32_t rgba = (tile_coord(&x, src_width, tiling_mode) && tile_coord(&y, src_height, tiling_mode)) ?
                  get_rgba(src, src_x + x, src_y + y, format) : tile_fill_rgba;
               float color[4];
               khrn_color_rgba_to_floats(color, rgba);
               sum[0] += kernel[((kernel_width - 1 - k) * kernel_height) + (kernel_height - 1 - l)] * color[0];
               sum[1] += kernel[((kernel_width - 1 - k) * kernel_height) + (kernel_height - 1 - l)] * color[1];
               sum[2] += kernel[((kernel_width - 1 - k) * kernel_height) + (kernel_height - 1 - l)] * color[2];
               sum[3] += kernel[((kernel_width - 1 - k) * kernel_height) + (kernel_height - 1 - l)] * color[3];
            }
         }
         rgba = color_floats_to_rgba(
            (sum[0] * scale) + bias, (sum[1] * scale) + bias,
            (sum[2] * scale) + bias, (sum[3] * scale) + bias);
         if (format & IMAGE_FORMAT_PRE) {
            rgba = khrn_color_rgba_clamp_to_a(rgba);
         }
         put_channel_masked_rgba(dst, dst_x + i, dst_y + j, rgba, channel_mask, format);
      }
   }
}

#endif

/******************************************************************************
worker messages
******************************************************************************/

typedef struct {
   MEM_HANDLE_T image;
   uint16_t x, y, width, height;
   uint32_t rgba;
} MSG_CLEAR_REGION_T;

static KHRN_WORKER_CALLBACK_RESULT_T clear_region_callback(
   KHRN_WORKER_CALLBACK_REASON_T reason,
   void *message, uint32_t size)
{
   MSG_CLEAR_REGION_T *msg = (MSG_CLEAR_REGION_T *)message;
   vcos_assert(size == sizeof(MSG_CLEAR_REGION_T));

   switch (reason) {
   case KHRN_WORKER_CALLBACK_REASON_DO_IT:
   {
      khrn_image_clear_region((KHRN_IMAGE_T *)mem_lock(msg->image),
         msg->x, msg->y, msg->width, msg->height, msg->rgba, IMAGE_CONV_VG);
      mem_unlock(msg->image);
      return KHRN_WORKER_CALLBACK_RESULT_DO_IT_OK_ADVANCE_EXIT_POS;
   }
   case KHRN_WORKER_CALLBACK_REASON_CLEANUP:
   {
      mem_release(msg->image);
      return KHRN_WORKER_CALLBACK_RESULT_CLEANUP;
   }
   default:
   {
      UNREACHABLE();
      return (KHRN_WORKER_CALLBACK_RESULT_T)0;
   }
   }
}

typedef enum {
   MSG_TYPE_COPY_REGION,
   MSG_TYPE_COPY_SCISSOR_REGIONS,
   MSG_TYPE_COLOR_MATRIX,
   MSG_TYPE_LOOKUP,
   MSG_TYPE_LOOKUP_SINGLE,
   MSG_TYPE_SCONV,
   MSG_TYPE_CONV
} MSG_TYPE_T;

#define MSG_SCONV_KERNEL_COUNT_MAX ((VG_BF_GBLUR_KERNEL_COUNT_MAX > VG_CONFIG_MAX_SEPARABLE_KERNEL_SIZE) ? VG_BF_GBLUR_KERNEL_COUNT_MAX : VG_CONFIG_MAX_SEPARABLE_KERNEL_SIZE)

typedef struct {
   MSG_TYPE_T type;
   MEM_HANDLE_T dst;
   uint16_t dst_x, dst_y, width, height;
   MEM_HANDLE_T src;
   uint16_t src_x, src_y;
   union {
      struct {
         int32_t scissor_rects[VG_CONFIG_MAX_SCISSOR_RECTS * 4];
         uint32_t scissor_rects_count;
      } copy_scissor_regions;
      struct {
         uint32_t channel_mask;
         KHRN_IMAGE_FORMAT_T format;
#ifdef SIMPENROSE
         float matrix[20];
#else
         int16_t matrix[16];
         int32_t bias[4];
         int32_t shift;
#endif
      } color_matrix;
      struct {
         uint32_t channel_mask;
         KHRN_IMAGE_FORMAT_T format;
         uint8_t red_lut[256], green_lut[256], blue_lut[256], alpha_lut[256];
         KHRN_IMAGE_FORMAT_T output_format;
      } lookup;
      struct {
         uint32_t channel_mask;
         KHRN_IMAGE_FORMAT_T format;
         uint32_t source_channel;
         uint32_t lut[256];
         KHRN_IMAGE_FORMAT_T output_format;
      } lookup_single;
      struct {
         uint16_t src_width, src_height;
         uint32_t channel_mask;
         KHRN_IMAGE_FORMAT_T format;
         VGTilingMode tiling_mode;
         uint32_t tile_fill_rgba;
         int32_t shift_x, shift_y;
#ifndef SIMPENROSE
         bool interm_16;
#endif
         int16_t kernel_y[MSG_SCONV_KERNEL_COUNT_MAX];
         uint32_t kernel_height;
#ifndef SIMPENROSE
         int32_t interm_bias;
         int32_t interm_shift;
#endif
         int16_t kernel_x[MSG_SCONV_KERNEL_COUNT_MAX];
         uint32_t kernel_width;
#ifdef SIMPENROSE
         float scale;
         float bias;
#else
         int64_t bias;
         int32_t shift;
#endif
         MEM_HANDLE_T state;
         bool success;
      } sconv;
      struct {
         uint16_t src_width, src_height;
         uint32_t channel_mask;
         KHRN_IMAGE_FORMAT_T format;
         VGTilingMode tiling_mode;
         uint32_t tile_fill_rgba;
         int32_t shift_x, shift_y;
         int16_t kernel[VG_CONFIG_MAX_KERNEL_SIZE * VG_CONFIG_MAX_KERNEL_SIZE];
         uint32_t kernel_width;
         uint32_t kernel_height;
#ifdef SIMPENROSE
         float scale;
         float bias;
#else
         int32_t bias;
         int32_t shift;
#endif
      } conv;
   } u;
} MSG_T;

#define MSG_SIZE_COPY_REGION          (offsetof(MSG_T, u))
#define MSG_SIZE_COPY_SCISSOR_REGIONS (offsetof(MSG_T, u.copy_scissor_regions) + sizeof(((MSG_T *)0)->u.copy_scissor_regions))
#define MSG_SIZE_COLOR_MATRIX         (offsetof(MSG_T, u.color_matrix) + sizeof(((MSG_T *)0)->u.color_matrix))
#define MSG_SIZE_LOOKUP               (offsetof(MSG_T, u.lookup) + sizeof(((MSG_T *)0)->u.lookup))
#define MSG_SIZE_LOOKUP_SINGLE        (offsetof(MSG_T, u.lookup_single) + sizeof(((MSG_T *)0)->u.lookup_single))
#define MSG_SIZE_SCONV                (offsetof(MSG_T, u.sconv) + sizeof(((MSG_T *)0)->u.sconv))
#define MSG_SIZE_CONV                 (offsetof(MSG_T, u.conv) + sizeof(((MSG_T *)0)->u.conv))

#ifndef SIMPENROSE
static INLINE KHRN_BF_TILE_T convert_tiling_mode(VGTilingMode tiling_mode)
{
   vcos_assert((tiling_mode >= VG_TILE_FILL) && (tiling_mode <= VG_TILE_REFLECT));
   return (KHRN_BF_TILE_T)(BF_TILE_FILL + (tiling_mode - VG_TILE_FILL));
}
#endif

static KHRN_WORKER_CALLBACK_RESULT_T callback(
   KHRN_WORKER_CALLBACK_REASON_T reason,
   void *message, uint32_t size)
{
   MSG_T *msg = (MSG_T *)message;
   vcos_assert(size <= sizeof(MSG_T));

   switch (reason) {
   case KHRN_WORKER_CALLBACK_REASON_DO_IT:
   {
      KHRN_IMAGE_T *dst = (KHRN_IMAGE_T *)mem_lock(msg->dst);
      KHRN_IMAGE_T *src = (KHRN_IMAGE_T *)mem_lock(msg->src);
      KHRN_IMAGE_WRAP_T dst_wrap, src_wrap;
      khrn_image_lock_wrap(dst, &dst_wrap);
      khrn_image_lock_wrap(src, &src_wrap);
      switch (msg->type) {
      case MSG_TYPE_COPY_REGION:
      {
         khrn_image_wrap_copy_region(
            &dst_wrap, msg->dst_x, msg->dst_y,
            msg->width, msg->height,
            &src_wrap, msg->src_x, msg->src_y,
            IMAGE_CONV_VG);
         break;
      }
      case MSG_TYPE_COPY_SCISSOR_REGIONS:
      {
         khrn_image_wrap_copy_scissor_regions(
            &dst_wrap, msg->dst_x, msg->dst_y,
            msg->width, msg->height,
            &src_wrap, msg->src_x, msg->src_y,
            IMAGE_CONV_VG,
            msg->u.copy_scissor_regions.scissor_rects, msg->u.copy_scissor_regions.scissor_rects_count);
         break;
      }
      case MSG_TYPE_COLOR_MATRIX:
      {
#ifdef SIMPENROSE
         color_matrix(
#else
         khrn_bf_color_matrix(
#endif
            &dst_wrap, msg->dst_x, msg->dst_y,
            msg->width, msg->height,
            &src_wrap, msg->src_x, msg->src_y,
            msg->u.color_matrix.channel_mask, msg->u.color_matrix.format,
#ifdef SIMPENROSE
            msg->u.color_matrix.matrix);
#else
            IMAGE_CONV_VG,
            msg->u.color_matrix.matrix, msg->u.color_matrix.bias, msg->u.color_matrix.shift);
#endif
         break;
      }
      case MSG_TYPE_LOOKUP:
      {
#ifdef SIMPENROSE
         lookup(
#else
         khrn_bf_lookup(
#endif
            &dst_wrap, msg->dst_x, msg->dst_y,
            msg->width, msg->height,
            &src_wrap, msg->src_x, msg->src_y,
            msg->u.lookup.channel_mask, msg->u.lookup.format,
#ifndef SIMPENROSE
            IMAGE_CONV_VG,
#endif
            msg->u.lookup.red_lut, msg->u.lookup.green_lut, msg->u.lookup.blue_lut, msg->u.lookup.alpha_lut,
            msg->u.lookup.output_format);
         break;
      }
      case MSG_TYPE_LOOKUP_SINGLE:
      {
#ifdef SIMPENROSE
         lookup_single(
#else
         khrn_bf_lookup_single(
#endif
            &dst_wrap, msg->dst_x, msg->dst_y,
            msg->width, msg->height,
            &src_wrap, msg->src_x, msg->src_y,
            msg->u.lookup_single.channel_mask, msg->u.lookup_single.format,
#ifndef SIMPENROSE
            IMAGE_CONV_VG,
#endif
            msg->u.lookup_single.source_channel,
            msg->u.lookup_single.lut,
            msg->u.lookup_single.output_format);
         break;
      }
      case MSG_TYPE_SCONV:
      {
#ifdef SIMPENROSE
         msg->u.sconv.success = sconv(
#else
         msg->u.sconv.success = khrn_bf_sconv(
#endif
            &dst_wrap, msg->dst_x, msg->dst_y, msg->width, msg->height,
            &src_wrap, msg->src_x, msg->src_y, msg->u.sconv.src_width, msg->u.sconv.src_height,
            msg->u.sconv.channel_mask, msg->u.sconv.format,
#ifdef SIMPENROSE
            msg->u.sconv.tiling_mode, msg->u.sconv.tile_fill_rgba,
            msg->u.sconv.shift_x, msg->u.sconv.shift_y,
            msg->u.sconv.kernel_x, msg->u.sconv.kernel_width,
            msg->u.sconv.kernel_y, msg->u.sconv.kernel_height,
            msg->u.sconv.scale, msg->u.sconv.bias);
#else
            IMAGE_CONV_VG,
            convert_tiling_mode(msg->u.sconv.tiling_mode), msg->u.sconv.tile_fill_rgba,
            msg->u.sconv.shift_x, msg->u.sconv.shift_y,
            msg->u.sconv.interm_16,
            msg->u.sconv.kernel_y, msg->u.sconv.kernel_height, msg->u.sconv.interm_bias, msg->u.sconv.interm_shift,
            msg->u.sconv.kernel_x, msg->u.sconv.kernel_width, msg->u.sconv.bias, msg->u.sconv.shift);
#endif
         break;
      }
      case MSG_TYPE_CONV:
      {
#ifdef SIMPENROSE
         conv(
#else
         khrn_bf_conv(
#endif
            &dst_wrap, msg->dst_x, msg->dst_y, msg->width, msg->height,
            &src_wrap, msg->src_x, msg->src_y, msg->u.conv.src_width, msg->u.conv.src_height,
            msg->u.conv.channel_mask, msg->u.conv.format,
#ifdef SIMPENROSE
            msg->u.conv.tiling_mode, msg->u.conv.tile_fill_rgba,
            msg->u.conv.shift_x, msg->u.conv.shift_y,
            msg->u.conv.kernel, msg->u.conv.kernel_width, msg->u.conv.kernel_height,
            msg->u.conv.scale, msg->u.conv.bias);
#else
            IMAGE_CONV_VG,
            convert_tiling_mode(msg->u.conv.tiling_mode), msg->u.conv.tile_fill_rgba,
            msg->u.conv.shift_x, msg->u.conv.shift_y,
            msg->u.conv.kernel, msg->u.conv.kernel_width, msg->u.conv.kernel_height, msg->u.conv.bias, msg->u.conv.shift);
#endif
         break;
      }
      default:
      {
         UNREACHABLE();
      }
      }
      khrn_image_unlock_wrap(src);
      khrn_image_unlock_wrap(dst);
      mem_unlock(msg->src);
      mem_unlock(msg->dst);
      return KHRN_WORKER_CALLBACK_RESULT_DO_IT_OK_ADVANCE_EXIT_POS;
   }
   case KHRN_WORKER_CALLBACK_REASON_CLEANUP:
   {
      mem_release(msg->src);
      mem_release(msg->dst);
      if (msg->type == MSG_TYPE_SCONV) {
         if (!msg->u.sconv.success) {
            vg_server_state_set_error((VG_SERVER_STATE_T *)mem_lock(msg->u.sconv.state), VG_OUT_OF_MEMORY_ERROR);
            mem_unlock(msg->u.sconv.state);
         }
         mem_release(msg->u.sconv.state); /* this might call vg_bf_term! */
      }
      return KHRN_WORKER_CALLBACK_RESULT_CLEANUP;
   }
   default:
   {
      UNREACHABLE();
      return (KHRN_WORKER_CALLBACK_RESULT_T)0;
   }
   }
}

/******************************************************************************
external interface
******************************************************************************/

static void wr_and_transfer(MEM_HANDLE_T dst_handle, MEM_HANDLE_T src_handle)
{
   KHRN_IMAGE_T *dst = (KHRN_IMAGE_T *)mem_lock(dst_handle),
      *src = (src_handle == MEM_INVALID_HANDLE) ? NULL : (KHRN_IMAGE_T *)mem_lock(src_handle);
   verify(khrn_interlock_write(&dst->interlock, KHRN_INTERLOCK_USER_TEMP));
   if (src) { khrn_interlock_read(&src->interlock, KHRN_INTERLOCK_USER_TEMP); }
   khrn_worker_advance_enter_pos();
   khrn_interlock_transfer(&dst->interlock, KHRN_INTERLOCK_USER_TEMP, KHRN_INTERLOCK_FIFO_WORKER);
   if (src) { khrn_interlock_transfer(&src->interlock, KHRN_INTERLOCK_USER_TEMP, KHRN_INTERLOCK_FIFO_WORKER); mem_unlock(src_handle); }
   mem_unlock(dst_handle);
}

static void msg_init(MSG_T *msg, MSG_TYPE_T type,
   MEM_HANDLE_T dst_handle, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   MEM_HANDLE_T src_handle, uint32_t src_x, uint32_t src_y)
{
   msg->type = type;
   mem_acquire(dst_handle);
   msg->dst = dst_handle;
   msg->dst_x = (uint16_t)dst_x;
   msg->dst_y = (uint16_t)dst_y;
   msg->width = (uint16_t)width;
   msg->height = (uint16_t)height;
   mem_acquire(src_handle);
   msg->src = src_handle;
   msg->src_x = (uint16_t)src_x;
   msg->src_y = (uint16_t)src_y;
}

void vg_bf_clear_region(
   MEM_HANDLE_T handle, uint32_t x, uint32_t y,
   uint32_t width, uint32_t height,
   uint32_t rgba) /* rgba non-lin, unpre */
{
   MSG_CLEAR_REGION_T *msg;
   wr_and_transfer(handle, MEM_INVALID_HANDLE);
   msg = (MSG_CLEAR_REGION_T *)khrn_worker_post_begin(clear_region_callback, sizeof(MSG_CLEAR_REGION_T));
   mem_acquire(handle);
   msg->image = handle;
   msg->x = (uint16_t)x;
   msg->y = (uint16_t)y;
   msg->width = (uint16_t)width;
   msg->height = (uint16_t)height;
   msg->rgba = rgba;
   khrn_worker_post_end();
}

void vg_bf_copy_region(
   MEM_HANDLE_T dst_handle, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   MEM_HANDLE_T src_handle, uint32_t src_x, uint32_t src_y)
{
   MSG_T *msg;
   wr_and_transfer(dst_handle, src_handle);
   msg = (MSG_T *)khrn_worker_post_begin(callback, MSG_SIZE_COPY_REGION);
   msg_init(msg, MSG_TYPE_COPY_REGION, dst_handle, dst_x, dst_y, width, height, src_handle, src_x, src_y);
   khrn_worker_post_end();
}

void vg_bf_copy_scissor_regions(
   MEM_HANDLE_T dst_handle, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   MEM_HANDLE_T src_handle, uint32_t src_x, uint32_t src_y,
   const int32_t *scissor_rects, uint32_t scissor_rects_count)
{
   MSG_T *msg;
   wr_and_transfer(dst_handle, src_handle);
   msg = (MSG_T *)khrn_worker_post_begin(callback, MSG_SIZE_COPY_SCISSOR_REGIONS);
   msg_init(msg, MSG_TYPE_COPY_SCISSOR_REGIONS, dst_handle, dst_x, dst_y, width, height, src_handle, src_x, src_y);
   memcpy(msg->u.copy_scissor_regions.scissor_rects, scissor_rects, scissor_rects_count * sizeof(int32_t));
   msg->u.copy_scissor_regions.scissor_rects_count = scissor_rects_count;
   khrn_worker_post_end();
}

void vg_bf_color_matrix(
   MEM_HANDLE_T dst_handle, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   MEM_HANDLE_T src_handle, uint32_t src_x, uint32_t src_y,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format,
   const float *matrix) /* matrix needs to be clean_float()ed */
{
   MSG_T *msg;
   wr_and_transfer(dst_handle, src_handle);
   msg = (MSG_T *)khrn_worker_post_begin(callback, MSG_SIZE_COLOR_MATRIX);
   msg_init(msg, MSG_TYPE_COLOR_MATRIX, dst_handle, dst_x, dst_y, width, height, src_handle, src_x, src_y);
   msg->u.color_matrix.channel_mask = channel_mask;
   msg->u.color_matrix.format = format;
#ifdef SIMPENROSE
   clean_floats(msg->u.color_matrix.matrix, matrix, 20);
#else
   khrn_bf_color_matrix_prepare(
      msg->u.color_matrix.matrix, msg->u.color_matrix.bias, &msg->u.color_matrix.shift,
      matrix);
#endif
   khrn_worker_post_end();
}

void vg_bf_lookup(
   MEM_HANDLE_T dst_handle, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   MEM_HANDLE_T src_handle, uint32_t src_x, uint32_t src_y,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format,
   const uint8_t *red_lut, const uint8_t *green_lut, const uint8_t *blue_lut, const uint8_t *alpha_lut,
   KHRN_IMAGE_FORMAT_T output_format)
{
   MSG_T *msg;
   wr_and_transfer(dst_handle, src_handle);
   msg = (MSG_T *)khrn_worker_post_begin(callback, MSG_SIZE_LOOKUP);
   msg_init(msg, MSG_TYPE_LOOKUP, dst_handle, dst_x, dst_y, width, height, src_handle, src_x, src_y);
   msg->u.lookup.channel_mask = channel_mask;
   msg->u.lookup.format = format;
   memcpy(msg->u.lookup.red_lut, red_lut, 256);
   memcpy(msg->u.lookup.green_lut, green_lut, 256);
   memcpy(msg->u.lookup.blue_lut, blue_lut, 256);
   memcpy(msg->u.lookup.alpha_lut, alpha_lut, 256);
   msg->u.lookup.output_format = output_format;
   khrn_worker_post_end();
}

void vg_bf_lookup_single(
   MEM_HANDLE_T dst_handle, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   MEM_HANDLE_T src_handle, uint32_t src_x, uint32_t src_y,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format,
   uint32_t source_channel, const uint32_t *lut,
   KHRN_IMAGE_FORMAT_T output_format)
{
   MSG_T *msg;
   wr_and_transfer(dst_handle, src_handle);
   msg = (MSG_T *)khrn_worker_post_begin(callback, MSG_SIZE_LOOKUP_SINGLE);
   msg_init(msg, MSG_TYPE_LOOKUP_SINGLE, dst_handle, dst_x, dst_y, width, height, src_handle, src_x, src_y);
   msg->u.lookup_single.channel_mask = channel_mask;
   msg->u.lookup_single.format = format;
   msg->u.lookup_single.source_channel = source_channel;
   memcpy(msg->u.lookup_single.lut, lut, 256 * sizeof(uint32_t));
   msg->u.lookup_single.output_format = output_format;
   khrn_worker_post_end();
}

void vg_bf_gblur(
   MEM_HANDLE_T dst_handle, uint32_t dst_x, uint32_t dst_y, uint32_t dst_width, uint32_t dst_height,
   MEM_HANDLE_T src_handle, uint32_t src_x, uint32_t src_y, uint32_t src_width, uint32_t src_height,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format,
   VGTilingMode tiling_mode, uint32_t tile_fill_rgba,
   float std_dev_x, float std_dev_y,
   MEM_HANDLE_T state_handle) /* for delayed error reporting */
{
   MSG_T *msg;
   uint32_t kernel_width, kernel_height;
   wr_and_transfer(dst_handle, src_handle);
   msg = (MSG_T *)khrn_worker_post_begin(callback, MSG_SIZE_SCONV);
   msg_init(msg, MSG_TYPE_SCONV, dst_handle, dst_x, dst_y, dst_width, dst_height, src_handle, src_x, src_y);
   msg->u.sconv.src_width = (uint16_t)src_width;
   msg->u.sconv.src_height = (uint16_t)src_height;
   msg->u.sconv.channel_mask = channel_mask;
   msg->u.sconv.format = format;
   msg->u.sconv.tiling_mode = tiling_mode;
   msg->u.sconv.tile_fill_rgba = tile_fill_rgba;
   kernel_width = vg_bf_gblur_kernel_gen(msg->u.sconv.kernel_x, std_dev_x);
   kernel_height = vg_bf_gblur_kernel_gen(msg->u.sconv.kernel_y, std_dev_y);
   msg->u.sconv.shift_x = kernel_width >> 1;
   msg->u.sconv.shift_y = kernel_height >> 1;
#ifndef SIMPENROSE
   msg->u.sconv.interm_16 = false;
#endif
   msg->u.sconv.kernel_height = kernel_height;
#ifndef SIMPENROSE
   msg->u.sconv.interm_bias = 0x4000;
   msg->u.sconv.interm_shift = 15;
#endif
   msg->u.sconv.kernel_width = kernel_width;
#ifdef SIMPENROSE
   msg->u.sconv.scale = 1.0f / (32768.0f * 32768.0f);
   msg->u.sconv.bias = 0.0f;
#else
   msg->u.sconv.bias = 0x4000;
   msg->u.sconv.shift = 15;
#endif
   mem_acquire(state_handle); /* holding a reference to the state also prevents vg_bf_term being called prematurely */
   msg->u.sconv.state = state_handle;
   khrn_worker_post_end();
}

void vg_bf_sconv(
   MEM_HANDLE_T dst_handle, uint32_t dst_x, uint32_t dst_y, uint32_t dst_width, uint32_t dst_height,
   MEM_HANDLE_T src_handle, uint32_t src_x, uint32_t src_y, uint32_t src_width, uint32_t src_height,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format,
   VGTilingMode tiling_mode, uint32_t tile_fill_rgba,
   int32_t shift_x, int32_t shift_y,
   const int16_t *kernel_x, uint32_t kernel_width,
   const int16_t *kernel_y, uint32_t kernel_height,
   float scale, float bias,
   MEM_HANDLE_T state_handle) /* for delayed error reporting */
{
   MSG_T *msg;
   wr_and_transfer(dst_handle, src_handle);
   msg = (MSG_T *)khrn_worker_post_begin(callback, MSG_SIZE_SCONV);
   msg_init(msg, MSG_TYPE_SCONV, dst_handle, dst_x, dst_y, dst_width, dst_height, src_handle, src_x, src_y);
   msg->u.sconv.src_width = (uint16_t)src_width;
   msg->u.sconv.src_height = (uint16_t)src_height;
   msg->u.sconv.channel_mask = channel_mask;
   msg->u.sconv.format = format;
   msg->u.sconv.tiling_mode = tiling_mode;
   msg->u.sconv.tile_fill_rgba = tile_fill_rgba;
   msg->u.sconv.shift_x = shift_x;
   msg->u.sconv.shift_y = shift_y;
#ifdef SIMPENROSE
   memcpy(msg->u.sconv.kernel_x, kernel_x, kernel_width * 2);
   memcpy(msg->u.sconv.kernel_y, kernel_y, kernel_height * 2);
   msg->u.sconv.scale = scale;
   msg->u.sconv.bias = bias;
#else
   msg->u.sconv.interm_16 = khrn_bf_sconv_prepare(
      msg->u.sconv.kernel_y, &msg->u.sconv.interm_bias, &msg->u.sconv.interm_shift,
      msg->u.sconv.kernel_x, &msg->u.sconv.bias, &msg->u.sconv.shift,
      kernel_x, kernel_width, kernel_y, kernel_height, scale, bias);
#endif
   msg->u.sconv.kernel_height = kernel_height;
   msg->u.sconv.kernel_width = kernel_width;
   mem_acquire(state_handle); /* holding a reference to the state also prevents vg_bf_term being called prematurely */
   msg->u.sconv.state = state_handle;
   khrn_worker_post_end();
}

void vg_bf_conv(
   MEM_HANDLE_T dst_handle, uint32_t dst_x, uint32_t dst_y, uint32_t dst_width, uint32_t dst_height,
   MEM_HANDLE_T src_handle, uint32_t src_x, uint32_t src_y, uint32_t src_width, uint32_t src_height,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format,
   VGTilingMode tiling_mode, uint32_t tile_fill_rgba,
   int32_t shift_x, int32_t shift_y,
   const int16_t *kernel, uint32_t kernel_width, uint32_t kernel_height,
   float scale, float bias)
{
   MSG_T *msg;
   wr_and_transfer(dst_handle, src_handle);
   msg = (MSG_T *)khrn_worker_post_begin(callback, MSG_SIZE_CONV);
   msg_init(msg, MSG_TYPE_CONV, dst_handle, dst_x, dst_y, dst_width, dst_height, src_handle, src_x, src_y);
   msg->u.conv.src_width = (uint16_t)src_width;
   msg->u.conv.src_height = (uint16_t)src_height;
   msg->u.conv.channel_mask = channel_mask;
   msg->u.conv.format = format;
   msg->u.conv.tiling_mode = tiling_mode;
   msg->u.conv.tile_fill_rgba = tile_fill_rgba;
   msg->u.conv.shift_x = shift_x;
   msg->u.conv.shift_y = shift_y;
#ifdef SIMPENROSE
   memcpy(msg->u.conv.kernel, kernel, kernel_width * kernel_height * 2);
   msg->u.conv.scale = scale;
   msg->u.conv.bias = bias;
#else
   khrn_bf_conv_prepare(
      msg->u.conv.kernel, &msg->u.conv.bias, &msg->u.conv.shift,
      kernel, kernel_width, kernel_height, scale, bias);
#endif
   msg->u.conv.kernel_width = kernel_width;
   msg->u.conv.kernel_height = kernel_height;
   khrn_worker_post_end();
}

void vg_bf_wait(void)
{
   khrn_worker_wait();
}
