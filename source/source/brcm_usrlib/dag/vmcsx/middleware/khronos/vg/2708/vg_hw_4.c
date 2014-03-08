//#define VG_BE_DUMP_UNIFS

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
#include "middleware/khronos/vg/2708/vg_hw_priv_4.h"
#include "middleware/khronos/vg/2708/vg_path_priv_4.h"
#include "middleware/khronos/vg/2708/vg_shader_4.h"
#include "middleware/khronos/vg/2708/vg_tess_priv_4.h"
#include "interface/khronos/vg/vg_int_mat3x3.h"
#include "middleware/khronos/vg/vg_paint.h"
#include "middleware/khronos/vg/vg_ramp.h"
#include "middleware/khronos/vg/vg_scissor.h"
#include "interface/khronos/common/khrn_int_color.h"
#include "interface/khronos/common/khrn_int_math.h"
#include "middleware/khronos/common/2708/khrn_copy_buffer_4.h"
#include "middleware/khronos/common/2708/khrn_fmem_4.h"
#include "middleware/khronos/common/2708/khrn_prod_4.h"
#include "middleware/khronos/common/2708/khrn_interlock_priv_4.h"
#include "middleware/khronos/common/2708/khrn_render_state_4.h"
#include <stddef.h>
#ifdef VG_BE_DUMP_UNIFS
   #include <stdio.h>
#endif

/* for generating unique ids which identify a render state and a snapshot of
 * associated state (eg user->surface matrix) */
static uint64_t glyph_state_id = 0;

static VG_BE_RENDER_STATE_T *current_render_state = NULL;

static void finish(VG_BE_RENDER_STATE_T *render_state)
{
   if (render_state == current_render_state) {
      current_render_state = NULL;
   }
   khrn_render_state_finish(khrn_render_state_get_index_from_data(render_state));
}

static INLINE uint32_t get_width_in_tiles(uint32_t w)
{
   return (w + (KHRN_HW_TILE_WIDTH - 1)) >> KHRN_HW_LOG2_TILE_WIDTH;
}

static INLINE uint32_t get_height_in_tiles(uint32_t h)
{
   return (h + (KHRN_HW_TILE_HEIGHT - 1)) >> KHRN_HW_LOG2_TILE_HEIGHT;
}

/******************************************************************************
uniforms generation helpers
******************************************************************************/

static uint32_t get_fd_unifs_n(VG_SHADER_FD_KEY_T key)
{
   uint32_t unifs_n = 0;
   if (key.scissor) {
      unifs_n += 4;
   }
   if (key.draw_type != VG_SHADER_FD_DRAW_TYPE_IMAGE) {
      /*
         grad/pattern may end up being solid (degenerate parameters / no pattern image)
         ok as solid has fewer uniforms
      */

      switch (key.paint_type) {
      case VG_SHADER_FD_PAINT_TYPE_SOLID:       unifs_n += 1; break;
      case VG_SHADER_FD_PAINT_TYPE_GRAD_LINEAR: unifs_n += 6; break;
      case VG_SHADER_FD_PAINT_TYPE_GRAD_RADIAL: unifs_n += 12; break;
      case VG_SHADER_FD_PAINT_TYPE_PATTERN:     unifs_n += 10 + key.paint_pattern_tile_fill; break; /* assume worst case for now */
      default:                                  UNREACHABLE();
      }
   }
   if (key.draw_type != VG_SHADER_FD_DRAW_TYPE_PAINT) {
      unifs_n += 10 + (key.image_projective ? 3 : 0); /* assume worst case for now */
   }
   if (((key.draw_type != VG_SHADER_FD_DRAW_TYPE_PAINT) &&
      (key.draw_type != VG_SHADER_FD_DRAW_TYPE_STENCIL)) ||
      (key.paint_type != VG_SHADER_FD_PAINT_TYPE_SOLID)) {
      if (key.color_transform) {
         unifs_n += 8;
      }
      unifs_n += 6; /* colorspace conversion. may not need, but assume we will for now. todo: on b0, may not need this many unifs */
   }
   return unifs_n;
}

static uint32_t convert_format_tu(KHRN_IMAGE_FORMAT_T format)
{
   vcos_assert((khrn_image_is_tformat(format) || khrn_image_is_lineartile(format)) && khrn_image_is_color(format));
   switch (format & ~(IMAGE_FORMAT_MEM_LAYOUT_MASK | IMAGE_FORMAT_PRE | IMAGE_FORMAT_LIN)) {
   case ABGR_8888: return 0;
   case XBGR_8888: return 1;
   case RGBA_4444: return 2;
   case RGBA_5551: return 3;
   case RGB_565:   return 4;
   case L_8:       return 5;
   case A_8:       return 6;
   case L_1:       return 12;
   case A_4:       return 13;
   case A_1:       return 14;
   default:        UNREACHABLE(); return 0;
   }
}

static INLINE bool convert_image_quality(VGImageQuality image_quality)
{
   vcos_assert((image_quality == VG_IMAGE_QUALITY_NONANTIALIASED) ||
               (image_quality == VG_IMAGE_QUALITY_FASTER) ||
               (image_quality == VG_IMAGE_QUALITY_BETTER));
   return image_quality != VG_IMAGE_QUALITY_NONANTIALIASED; /* bilinear? */
}

typedef enum {
   TILE_REPEAT,  /* repeat */
   TILE_PAD,     /* clamp to edge */
   TILE_REFLECT, /* mirrored repeat */
   TILE_FILL     /* clamp to border */
} TILE_T;

static TILE_T convert_ramp_spread_mode(VGColorRampSpreadMode ramp_spread_mode)
{
   switch (ramp_spread_mode) {
   case VG_COLOR_RAMP_SPREAD_PAD:     return TILE_PAD;
   case VG_COLOR_RAMP_SPREAD_REPEAT:  return TILE_REPEAT;
   case VG_COLOR_RAMP_SPREAD_REFLECT: return TILE_REFLECT;
   default:                           UNREACHABLE(); return (TILE_T)0;
   }
}

static TILE_T convert_tiling_mode(VGTilingMode tiling_mode)
{
   switch (tiling_mode) {
   case VG_TILE_FILL:    return TILE_FILL;
   case VG_TILE_PAD:     return TILE_PAD;
   case VG_TILE_REPEAT:  return TILE_REPEAT;
   case VG_TILE_REFLECT: return TILE_REFLECT;
   default:              UNREACHABLE(); return (TILE_T)0;
   }
}

static INLINE bool need_child(uint32_t width, uint32_t height, const uint32_t *child_rect)
{
   vcos_assert(((child_rect[0] + child_rect[2]) <= width) && ((child_rect[1] + child_rect[3]) <= height));
   return (child_rect[0] != 0) || (child_rect[1] != 0) || (child_rect[2] != width) || (child_rect[3] != height);
}

/*
   if false is returned, the handle was not added to fixer
   (this is important for scissors and ramps)
*/

static bool add_unifs_tu(VG_BE_RENDER_STATE_T *render_state,
   uint32_t **unifs,
   MEM_HANDLE_T handle, uint32_t offset, uint32_t ramp_i,
   KHRN_IMAGE_FORMAT_T format, uint32_t width, uint32_t height,
   bool bilinear, TILE_T tile,
   bool child, const uint32_t *child_rect)
{
   uint32_t flags;
   vcos_assert(khrn_image_prefer_lt(format, width, height) ?
      khrn_image_is_lineartile(format) : khrn_image_is_tformat(format));
   vcos_assert((width <= KHRN_HW_TEX_SIZE_MAX) && (height <= KHRN_HW_TEX_SIZE_MAX));

#ifndef NDEBUG
   if (ramp_i != ~0) {
      VG_RAMP_T *ramp = (VG_RAMP_T *)mem_lock(handle);
      vcos_assert(mem_get_align(ramp->data) >= KHRN_HW_TEX_ALIGN);
      mem_unlock(handle);
   } else {
      vcos_assert(mem_get_align(handle) >= KHRN_HW_TEX_ALIGN);
   }
#endif
#ifdef SIMPENROSE_RECORD_OUTPUT
   if (is_ramp) {
      record_map_mem_buffer(((VG_RAMP_T *)mem_lock(handle))->data, L_TEXTURE_DATA, RECORD_BUFFER_IS_BOTH, KHRN_HW_TEX_ALIGN);
      mem_unlock(handle);
   } else {
      record_map_mem_buffer(handle, L_TEXTURE_DATA, RECORD_BUFFER_IS_BOTH, KHRN_HW_TEX_ALIGN);
   }
   if (*unifs != unifs_begin) {
      record_map_buffer(khrn_hw_addr(unifs_begin), (*unifs - unifs_begin) * 4, L_UNIFORMS, RECORD_BUFFER_IS_DATA | RECORD_BUFFER_ALLOW_BEFORE_CLIF, 4);
   }
   record_map_buffer(khrn_hw_addr(*unifs), 8, L_TEX_PARAMS, RECORD_BUFFER_IS_DATA | RECORD_BUFFER_ALLOW_BEFORE_CLIF, 4);
   unifs_begin = *unifs + 2;
#endif

   flags =
      (0 << 0) | /* no mipmapping */
      (convert_format_tu(format) << 4) |
      (0 << 8) | /* disable flip-y */
      (0 << 9) | /* disable cube map mode */
      (0 << 10); /* cache swizzle */
   if (ramp_i != ~0)
   {
      if (!khrn_fmem_ramp(render_state->fmem, (*unifs)++, handle, offset + flags, ramp_i))
         return false;
   }
   else
   {
      if (!khrn_fmem_fix(render_state->fmem, (*unifs)++, handle, offset + flags))
         return false;
   }

   *((*unifs)++) =
      (tile << 0) | /* tile s */
      (tile << 2) | /* tile t */
      (!bilinear << 4) | /* min filt */
      (!bilinear << 7) | /* mag filt */
      ((width & 2047) << 8) |
      (0 << 19) | /* etc flip-y */
      ((height & 2047) << 20);

   if (child) {
      vcos_assert(need_child(width, height, child_rect));

      *((*unifs)++) =
         (child_rect[0] << 0) |
         (child_rect[1] << 12) |
         (3 << 30); /* child image offsets */
      *((*unifs)++) =
         ((child_rect[2] & 2047) << 0) |
         ((child_rect[3] & 2047) << 12) |
         (2 << 30); /* child image dimensions */
   } else if (tile == TILE_FILL) {
      *((*unifs)++) = 0; /* dummy uniform for tile fill rgba write */
   }

   return true;
}

static bool add_unifs_tu_image(VG_BE_RENDER_STATE_T *render_state,
   uint32_t **unifs,
   MEM_HANDLE_T image_handle,
   KHRN_IMAGE_FORMAT_T format, uint32_t width, uint32_t height,
   bool bilinear, TILE_T tile,
   bool child, const uint32_t *child_rect)
{
   KHRN_IMAGE_T *image;
   bool success;
   if (!khrn_fmem_interlock(render_state->fmem, image_handle, offsetof(KHRN_IMAGE_T, interlock)))
   {
      return false;
   }
   image = (KHRN_IMAGE_T *)mem_lock(image_handle);
   khrn_interlock_read(&image->interlock, khrn_interlock_user(khrn_render_state_get_index_from_data(render_state)));
   success = add_unifs_tu(render_state, unifs, image->mh_storage, image->offset, ~0, format, width, height, bilinear, tile, child, child_rect);
   mem_unlock(image_handle);
   return success;
}

/*
   if false is returned, the scissor handle was not added to fixer
*/

static bool add_unifs_scissor(VG_BE_RENDER_STATE_T *render_state,
   uint32_t **unifs,
   MEM_HANDLE_T scissor_handle)
{
   bool result;
   *((*unifs)++) = float_to_bits(recip_((float)render_state->frame_width));
   *((*unifs)++) = float_to_bits(recip_((float)render_state->frame_height));
   result = add_unifs_tu(render_state, unifs,
      scissor_handle, 0, ~0,
      vg_scissor_get_format(render_state->frame_width, render_state->frame_height), render_state->frame_width, render_state->frame_height,
      false, TILE_PAD, /* tile mode shouldn't matter */
      false, NULL);
   mem_unretain(scissor_handle);    /* TODO: hack - we end up retaining handle twice */
   return result;
}

/* todo: shouldn't need on b0 */
static void get_tu_width_height(uint32_t *width_height, MEM_HANDLE_T handle)
{
   KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)mem_lock(handle);
   width_height[0] = (image->stride == khrn_image_get_stride(image->format, image->width)) ?
      /* tu will calculate correct stride. give it the actual width */
      image->width :
      /* tu won't calculate correct stride. give it a fake width */
      ((image->stride << 3) / khrn_image_get_bpp(image->format));
   width_height[1] = khrn_image_is_tformat(image->format) ?
      /* make sure height is large enough for tu to choose t-format (the stride
       * stuff above will take care of the width) */
      _max(image->height, (4 << khrn_image_get_log2_utile_height(image->format)) + 1) :
      /* not t-format. no fudging required */
      image->height;
   mem_unlock(handle);
}

static bool add_unifs_paint(VG_BE_RENDER_STATE_T *render_state,
   uint32_t **unifs,
   VG_PAINT_T *paint, VGImageQuality paint_pattern_quality, uint32_t tile_fill_rgba, const VG_MAT3X3_T *surface_to_paint,
   VG_SHADER_FD_KEY_T *key, uint32_t *rgba)
{
   switch (paint->type) {
   case VG_PAINT_TYPE_COLOR:
   {
      *rgba = paint->rgba;
      break;
   }
   case VG_PAINT_TYPE_LINEAR_GRADIENT:
   case VG_PAINT_TYPE_RADIAL_GRADIENT:
   {
      bool degenerate;
      VG_MAT3X3_T a = *surface_to_paint;
      if (paint->type == VG_PAINT_TYPE_LINEAR_GRADIENT) {
         degenerate = vg_be_prepare_paint_grad_linear(paint, &a);
         if (!degenerate) {
            *((*unifs)++) = float_to_bits(a.m[0][2]);
            *((*unifs)++) = float_to_bits(a.m[0][0]);
            *((*unifs)++) = float_to_bits(a.m[0][1]);
         }
      } else {
         float u, v;
         degenerate = vg_be_prepare_paint_grad_radial(paint, &a, &u, &v);
         if (!degenerate) {
            *((*unifs)++) = float_to_bits(a.m[0][2]);
            *((*unifs)++) = float_to_bits(a.m[0][0]);
            *((*unifs)++) = float_to_bits(a.m[0][1]);
            *((*unifs)++) = float_to_bits(a.m[1][0]);
            *((*unifs)++) = float_to_bits(a.m[1][2]);
            *((*unifs)++) = float_to_bits(a.m[1][1]);
            *((*unifs)++) = float_to_bits(u * u);
            *((*unifs)++) = float_to_bits(u);
            *((*unifs)++) = float_to_bits(v);
         }
      }
      if (degenerate) {
         key->paint_type = VG_SHADER_FD_PAINT_TYPE_SOLID;
         *rgba = vg_paint_get_ramp_degenerate_rgba(paint);
      } else {
         if (!vg_paint_retain_ramp(paint)) {
            return false;
         }
         *((*unifs)++) = float_to_bits(((float)paint->ramp_i + 0.5f) * (1.0f / (float)VG_RAMP_HEIGHT));
         if (!add_unifs_tu(render_state, unifs,
            paint->ramp, 0, paint->ramp_i,
            VG_RAMP_FORMAT, VG_RAMP_WIDTH, VG_RAMP_HEIGHT,
            true, convert_ramp_spread_mode(vg_paint_get_ramp_spread_mode(paint)),
            false, NULL)) {
            vg_ramp_unretain(paint->ramp); /* ramp not added to fixer, this won't happen automatically */
            return false;
         }
         /* ramp added to fixer now, vg_ramp_unretain will happen automatically */
      }
      break;
   }
   case VG_PAINT_TYPE_PATTERN:
   {
      if (paint->pattern == MEM_INVALID_HANDLE) {
         /*
            behave as if paint type was VG_PAINT_TYPE_COLOR
         */

         key->paint_type = VG_SHADER_FD_PAINT_TYPE_SOLID;
         *rgba = paint->rgba;
      } else {
         VG_MAT3X3_T a = *surface_to_paint;
         KHRN_IMAGE_FORMAT_T format;
         bool bilinear;
         uint32_t child_rect[4];
         MEM_HANDLE_T image_handle = vg_be_prepare_paint_pattern(paint, &a, &format, NULL, &bilinear, child_rect, &tile_fill_rgba);
         uint32_t width_height[2];
         bool child;
         get_tu_width_height(width_height, image_handle); /* todo: shouldn't need on b0 */
         child = need_child(width_height[0], width_height[1], child_rect);
         key->paint_pattern_child = child;
         key->paint_pattern_rgbl = !!(format & (IMAGE_FORMAT_RGB | IMAGE_FORMAT_L));
         key->paint_pattern_pre = !key->paint_pattern_rgbl || /* we replicate alpha in shader */
            !(format & IMAGE_FORMAT_A) || (format & IMAGE_FORMAT_PRE); /* avoid pre in shader for unpre images without alpha */
         if ((key->draw_type == VG_SHADER_FD_DRAW_TYPE_PAINT) || (key->draw_type == VG_SHADER_FD_DRAW_TYPE_STENCIL)) {
            key->colorspace_convert = !!(format & IMAGE_FORMAT_LIN);
         }
         *((*unifs)++) = float_to_bits(a.m[0][2]);
         *((*unifs)++) = float_to_bits(a.m[0][0]);
         *((*unifs)++) = float_to_bits(a.m[0][1]);
         *((*unifs)++) = float_to_bits(a.m[1][0]);
         *((*unifs)++) = float_to_bits(a.m[1][1]);
         *((*unifs)++) = float_to_bits(a.m[1][2]);
         if (paint->pattern_tiling_mode == VG_TILE_FILL) {
            *((*unifs)++) = tile_fill_rgba;
         }
         if (!add_unifs_tu_image(render_state, unifs,
            image_handle,
            format, width_height[0], width_height[1],
            convert_image_quality(paint_pattern_quality) && bilinear, convert_tiling_mode(paint->pattern_tiling_mode),
            child, child_rect)) { return false; }
      }
      break;
   }
   default:
   {
      UNREACHABLE();
   }
   }
   return true;
}

static bool add_unifs_image_fd(VG_BE_RENDER_STATE_T *render_state,
   uint32_t **unifs,
   MEM_HANDLE_T handle, const uint32_t *rect, VGImageQuality image_quality, const VG_MAT3X3_T *surface_to_image,
   VG_SHADER_FD_KEY_T *key)
{
   KHRN_IMAGE_FORMAT_T format;
   bool bilinear;
   MEM_HANDLE_T image_handle = vg_be_prepare_image(handle, &format, NULL, &bilinear);
   uint32_t width_height[2];
   bool child;
   VG_MAT3X3_T a;
   get_tu_width_height(width_height, image_handle); /* todo: shouldn't need on b0 */
   child = need_child(width_height[0], width_height[1], rect);
   key->image_child = child;
   key->image_rgbl = !!(format & (IMAGE_FORMAT_RGB | IMAGE_FORMAT_L));
   key->image_pre = !key->image_rgbl || /* we replicate alpha in shader */
      !(format & IMAGE_FORMAT_A) || (format & IMAGE_FORMAT_PRE); /* avoid pre in shader for unpre images without alpha */
   if ((key->draw_type == VG_SHADER_FD_DRAW_TYPE_IMAGE) || (key->draw_type == VG_SHADER_FD_DRAW_TYPE_MULTIPLY)) {
      key->colorspace_convert = !!(format & IMAGE_FORMAT_LIN);
   }
   a = *surface_to_image;
   vg_mat3x3_postmul_translate(&a, 0.5f, 0.5f); /* pixel samples at (x + 0.5, y + 0.5) */
   vg_mat3x3_premul_scale(&a, recip_((float)rect[2]), recip_((float)rect[3]));
   vcos_assert(key->image_projective || vg_mat3x3_is_affine(&a)); /* todo: could this fail due to fp evilness? */
   if (key->image_projective) {
      *((*unifs)++) = float_to_bits(a.m[2][2]);
      *((*unifs)++) = float_to_bits(a.m[2][0]);
      *((*unifs)++) = float_to_bits(a.m[2][1]);
   }
   *((*unifs)++) = float_to_bits(a.m[1][2]);
   *((*unifs)++) = float_to_bits(a.m[1][0]);
   *((*unifs)++) = float_to_bits(a.m[1][1]);
   *((*unifs)++) = float_to_bits(a.m[0][0]);
   *((*unifs)++) = float_to_bits(a.m[0][1]);
   *((*unifs)++) = float_to_bits(a.m[0][2]);
   return add_unifs_tu_image(render_state, unifs,
      image_handle,
      format, width_height[0], width_height[1],
      convert_image_quality(image_quality) && bilinear, TILE_PAD,
      child, rect);
}

#ifndef __BCM2708A0__
static bool add_unifs_image_md(VG_BE_RENDER_STATE_T *render_state,
   uint32_t **unifs,
   MEM_HANDLE_T handle, int32_t x, int32_t y,
   bool *no_alpha)
{
   KHRN_IMAGE_FORMAT_T format;
   MEM_HANDLE_T image_handle = vg_be_prepare_image(handle, &format, NULL, NULL);
   uint32_t width_height[2];
   get_tu_width_height(width_height, image_handle); /* todo: shouldn't need on b0 */
   *no_alpha = !(format & IMAGE_FORMAT_A);
   *((*unifs)++) = float_to_bits(0.5f - (float)y);
   *((*unifs)++) = float_to_bits(recip_((float)width_height[1]));
   *((*unifs)++) = float_to_bits(0.5f - (float)x);
   *((*unifs)++) = float_to_bits(recip_((float)width_height[0]));
   return add_unifs_tu_image(render_state, unifs,
      image_handle,
      format, width_height[0], width_height[1],
      false, TILE_PAD, /* tile mode shouldn't matter */
      false, NULL);
}
#endif

static void add_unifs_color_transform(uint32_t **unifs,
   const float *color_transform)
{
   *((*unifs)++) = float_to_bits(color_transform[3]);
   *((*unifs)++) = float_to_bits(color_transform[0]);
   *((*unifs)++) = float_to_bits(color_transform[1]);
   *((*unifs)++) = float_to_bits(color_transform[2]);
   *((*unifs)++) = float_to_bits(color_transform[7]);
   *((*unifs)++) = float_to_bits(color_transform[4]);
   *((*unifs)++) = float_to_bits(color_transform[5]);
   *((*unifs)++) = float_to_bits(color_transform[6]);
}

/* todo: on b0, do colorspace conversion stuff using general-purpose tu lookups */

static void ltify(void *dst, const void *src)
{
   KHRN_IMAGE_WRAP_T dst_wrap, src_wrap;
   khrn_image_wrap(&dst_wrap, L_8_LT, 256, 1, 256, dst);
   khrn_image_wrap(&src_wrap, L_8_RSO, 256, 1, 256, (void *)src); /* casting away const, but we won't actually modify */
   khrn_image_wrap_copy_region(&dst_wrap, 0, 0, 256, 1, &src_wrap, 0, 0, IMAGE_CONV_GL);
}

static void *s_to_lin, *lin_to_s;

static void *colorspace_convert_init(void *p)
{
   vcos_assert(!((uintptr_t)p & (KHRN_HW_TEX_ALIGN - 1)));
   s_to_lin = p;
   lin_to_s = (uint8_t *)p + round_up(2048, KHRN_HW_TEX_ALIGN);
   ltify(s_to_lin, COLOR_S_TO_LIN);
   ltify(lin_to_s, COLOR_LIN_TO_S);
   return (uint8_t *)p + round_up(2048, KHRN_HW_TEX_ALIGN) + 2048;
}

static void add_unifs_colorspace_convert(uint32_t **unifs,
   bool frame_lin)
{
   void *p = frame_lin ? s_to_lin : lin_to_s;
   uint32_t unif_0, unif_1;
#ifdef SIMPENROSE_RECORD_OUTPUT
   record_map_buffer(khrn_hw_addr(p), 2048, L_TEXTURE_DATA, RECORD_BUFFER_IS_BOTH, KHRN_HW_TEX_ALIGN);
   if (*unifs != unifs_begin) {
      record_map_buffer(khrn_hw_addr(unifs_begin), (*unifs - unifs_begin) * 4, L_UNIFORMS, RECORD_BUFFER_IS_DATA | RECORD_BUFFER_ALLOW_BEFORE_CLIF, 4);
   }
   record_map_buffer(khrn_hw_addr(*unifs), 8, L_TEX_PARAMS, RECORD_BUFFER_IS_DATA | RECORD_BUFFER_ALLOW_BEFORE_CLIF, 4);
   record_map_buffer(khrn_hw_addr(*unifs + 2), 8, L_TEX_PARAMS, RECORD_BUFFER_IS_DATA | RECORD_BUFFER_ALLOW_BEFORE_CLIF, 4);
   record_map_buffer(khrn_hw_addr(*unifs + 4), 8, L_TEX_PARAMS, RECORD_BUFFER_IS_DATA | RECORD_BUFFER_ALLOW_BEFORE_CLIF, 4);
   unifs_begin = *unifs + 6;
#endif
   unif_0 =
      (0 << 0) | /* no mipmapping */
      (convert_format_tu(L_8_LT) << 4) |
      (0 << 8) | /* disable flip-y */
      (0 << 9) | /* disable cube map mode */
      (0 << 10) | /* cache swizzle */
      khrn_hw_addr(p);
   unif_1 =
      (TILE_PAD << 0) | /* tile s */
      (TILE_PAD << 2) | /* tile t */
      (1 << 4) | /* min filt (nearest) */
      (1 << 7) | /* mag filt (nearest) */
      (256 << 8) | /* width */
      (0 << 19) | /* etc flip-y */
      (1 << 20); /* height */
   *((*unifs)++) = unif_0;
   *((*unifs)++) = unif_1;
   *((*unifs)++) = unif_0;
   *((*unifs)++) = unif_1;
   *((*unifs)++) = unif_0;
   *((*unifs)++) = unif_1;
}

/******************************************************************************
static shaders
******************************************************************************/

#ifdef __BCM2708A0__
#define FULL_LOAD_SHADER_EXTRA_THRSWS 4
static const uint32_t FULL_LOAD_SHADER[] = {
   /* generated by "qasm -mplain -x -ta0" */
   0x009e7000, 0x200009e7, /* nop                  ; nop               ; thrsw  */
   0x009e7000, 0x100009e7, /* nop                  ; nop                        */
   0x009e7000, 0x100009e7, /* nop                  ; nop                        */
   0x009e7000, 0x200009e7, /* nop                  ; nop               ; thrsw  */
   0x009e7000, 0x100009e7, /* nop                  ; nop                        */
   0x009e7000, 0x100009e7, /* nop                  ; nop                        */
   0x009e7000, 0x200009e7, /* nop                  ; nop               ; thrsw  */
   0x009e7000, 0x100009e7, /* nop                  ; nop                        */
   0x009e7000, 0x100009e7, /* nop                  ; nop                        */
   0x009e7000, 0x200009e7, /* nop                  ; nop               ; thrsw  */
   0x009e7000, 0x100009e7, /* nop                  ; nop                        */
   0x009e7000, 0x100009e7, /* nop                  ; nop                        */
   0x089e9fc0, 0x10020867, /* itof  r1, y_coord    ; nop                        */
   0x8182f3f6, 0xd0024862, /* fadd  r1, r1, f(0.5) ; mov  r2, unif              */
   0x88a60dbf, 0x20024823, /* itof  r0, x_coord    ; mov  r3, unif     ; thrsw  */
   0x219ef1cb, 0xd0024839, /* fadd  r0, r0, f(0.5) ; fmul  t0t, r1, r3          */
   0x209e7002, 0x100049f8, /* nop                  ; fmul  t0s, r0, r2          */
   0x009e7000, 0xa00009e7, /* nop                  ; nop               ; ldtmu0 */
   0x159e7900, 0x40020ba7, /* mov  tlbc, r4        ; nop               ; sbwait */
   0x009e7000, 0x500009e7, /* nop                  ; nop               ; sbdone */
   0x009e7000, 0x300009e7, /* nop                  ; nop               ; thrend */
   0x009e7000, 0x100009e7, /* nop                  ; nop                        */
   0x009e7000, 0x100009e7, /* nop                  ; nop                        */
   };
#endif

#ifdef WORKAROUND_HW2885
#define NOP_SHADER_EXTRA_THRSWS 5
static const uint32_t NOP_SHADER[] = {
   /* generated by "qasm -mplain -x -ta0" */
   0x009e7000, 0x200009e7, /* nop ; nop ; thrsw  */
   0x009e7000, 0x100009e7, /* nop ; nop          */
   0x009e7000, 0x100009e7, /* nop ; nop          */
   0x009e7000, 0x200009e7, /* nop ; nop ; thrsw  */
   0x009e7000, 0x100009e7, /* nop ; nop          */
   0x009e7000, 0x100009e7, /* nop ; nop          */
   0x009e7000, 0x200009e7, /* nop ; nop ; thrsw  */
   0x009e7000, 0x100009e7, /* nop ; nop          */
   0x009e7000, 0x100009e7, /* nop ; nop          */
   0x009e7000, 0x200009e7, /* nop ; nop ; thrsw  */
   0x009e7000, 0x100009e7, /* nop ; nop          */
   0x009e7000, 0x100009e7, /* nop ; nop          */
   0x009e7000, 0x200009e7, /* nop ; nop ; thrsw  */
   0x009e7000, 0x100009e7, /* nop ; nop          */
   0x009e7000, 0x100009e7, /* nop ; nop          */
   0x009e7000, 0x100009e7, /* nop ; nop          */
   0x009e7000, 0x400009e7, /* nop ; nop ; sbwait */
   0x009e7000, 0x500009e7, /* nop ; nop ; sbdone */
   0x009e7000, 0x300009e7, /* nop ; nop ; thrend */
   0x009e7000, 0x100009e7, /* nop ; nop          */
   0x009e7000, 0x100009e7, /* nop ; nop          */
   };
#endif

#ifdef __BCM2708A0__
#define XS_0 \
   X(full_load_shader, FULL_LOAD_SHADER, sizeof(FULL_LOAD_SHADER))
#else
#define XS_0 \
   X(md_clear_shader,                    VG_SHADER_MD_CLEAR,                    VG_SHADER_MD_CLEAR_SIZE) \
   X(md_image_set_shader,                VG_SHADER_MD_IMAGE_SET,                VG_SHADER_MD_IMAGE_SET_SIZE) \
   X(md_image_union_shader,              VG_SHADER_MD_IMAGE_UNION,              VG_SHADER_MD_IMAGE_UNION_SIZE) \
   X(md_image_intersect_shader,          VG_SHADER_MD_IMAGE_INTERSECT,          VG_SHADER_MD_IMAGE_INTERSECT_SIZE) \
   X(md_image_subtract_shader,           VG_SHADER_MD_IMAGE_SUBTRACT,           VG_SHADER_MD_IMAGE_SUBTRACT_SIZE) \
   X(md_image_no_alpha_set_shader,       VG_SHADER_MD_IMAGE_NO_ALPHA_SET,       VG_SHADER_MD_IMAGE_NO_ALPHA_SET_SIZE) \
   X(md_image_no_alpha_union_shader,     VG_SHADER_MD_IMAGE_NO_ALPHA_UNION,     VG_SHADER_MD_IMAGE_NO_ALPHA_UNION_SIZE) \
   X(md_image_no_alpha_intersect_shader, VG_SHADER_MD_IMAGE_NO_ALPHA_INTERSECT, VG_SHADER_MD_IMAGE_NO_ALPHA_INTERSECT_SIZE) \
   X(md_image_no_alpha_subtract_shader,  VG_SHADER_MD_IMAGE_NO_ALPHA_SUBTRACT,  VG_SHADER_MD_IMAGE_NO_ALPHA_SUBTRACT_SIZE) \
   X(md_cvg_set_shader,                  VG_SHADER_MD_CVG_SET,                  VG_SHADER_MD_CVG_SET_SIZE) \
   X(md_cvg_union_shader,                VG_SHADER_MD_CVG_UNION,                VG_SHADER_MD_CVG_UNION_SIZE) \
   X(md_cvg_intersect_shader,            VG_SHADER_MD_CVG_INTERSECT,            VG_SHADER_MD_CVG_INTERSECT_SIZE) \
   X(md_cvg_subtract_shader,             VG_SHADER_MD_CVG_SUBTRACT,             VG_SHADER_MD_CVG_SUBTRACT_SIZE) \
   X(md_cvg_scissor_set_shader,          VG_SHADER_MD_CVG_SCISSOR_SET,          VG_SHADER_MD_CVG_SCISSOR_SET_SIZE) \
   X(md_cvg_scissor_union_shader,        VG_SHADER_MD_CVG_SCISSOR_UNION,        VG_SHADER_MD_CVG_SCISSOR_UNION_SIZE) \
   X(md_cvg_scissor_intersect_shader,    VG_SHADER_MD_CVG_SCISSOR_INTERSECT,    VG_SHADER_MD_CVG_SCISSOR_INTERSECT_SIZE) \
   X(md_cvg_scissor_subtract_shader,     VG_SHADER_MD_CVG_SCISSOR_SUBTRACT,     VG_SHADER_MD_CVG_SCISSOR_SUBTRACT_SIZE)
#endif

#ifdef WORKAROUND_HW2885
#define XS_1 \
   X(nop_shader, NOP_SHADER, sizeof(NOP_SHADER))
#else
#define XS_1
#endif

#define XS XS_0 XS_1 \
   X(fd_clear_shader, VG_SHADER_FD_CLEAR, VG_SHADER_FD_CLEAR_SIZE)

static INLINE uint32_t get_shaders_size(void)
{
   #define X(DST, SRC, SIZE) SIZE +
   return XS 0;
   #undef X
}

#define X(DST, SRC, SIZE) static void *DST;
XS
#undef X

static void *shaders_init(void *p)
{
   vcos_assert(!((uintptr_t)p & 7));
   #define X(DST, SRC, SIZE) \
      DST = p; \
      p = (uint8_t *)p + SIZE; \
      memcpy(DST, SRC, SIZE);
   XS
   #undef X
   return p;
}

#ifdef SIMPENROSE_RECORD_OUTPUT
static void record_map_shaders(void)
{
   #define X(DST, SRC, SIZE) record_map_buffer(khrn_hw_addr(DST), SIZE, L_FRAGMENT_SHADER, RECORD_BUFFER_IS_BOTH, 8);
   XS
   #undef X
}
#endif

#undef XS
#undef XS_1
#undef XS_0

/******************************************************************************
state setting (including actual uniforms generation)
******************************************************************************/

#ifdef __BCM2708A0__
typedef struct {
   uint8_t flags;
   uint8_t stride;
   uint8_t unifs_n;
   uint8_t varys_n;
   uint32_t shader;
   uint32_t unifs;
   uint32_t dummy; /* so sizeof(SHADER_RECORD_T) == 16 */
} SHADER_RECORD_T;

vcos_static_assert(sizeof(SHADER_RECORD_T) == 16);
#endif /* else: use inline shader records */

#ifdef WORKAROUND_HW2366
static SHADER_RECORD_T *dummy_shader_record;

static void *dummy_shader_record_init(void *p)
{
   vcos_assert(!((uintptr_t)p & 15));
   dummy_shader_record = (SHADER_RECORD_T *)p;
   memset(dummy_shader_record, 0, sizeof(SHADER_RECORD_T));
   dummy_shader_record->shader = khrn_hw_addr(dummy_shader_record);
   dummy_shader_record->unifs = khrn_hw_addr(dummy_shader_record);
   return dummy_shader_record + 1;
}

static bool set_dummy_shader(VG_BE_RENDER_STATE_T *render_state)
{
   uint8_t *p;

   render_state->shader = MEM_INVALID_HANDLE;

   p = khrn_fmem_cle(render_state->fmem, 5);
   if (!p) { return false; }
   Add_byte(p, KHRN_HW_INSTR_VG_SHADER);
#ifdef SIMPENROSE_RECORD_OUTPUT
   record_map_buffer(khrn_hw_addr(dummy_shader_record), 12, L_VG_SHADER_RECORD, RECORD_BUFFER_IS_BOTH, 16);
#endif
   ADD_WORD(p, khrn_hw_addr(dummy_shader_record));
   vcos_assert(khrn_fmem_is_here(render_state->fmem, p));

   return true;
}
#endif

static bool set_shader(VG_BE_RENDER_STATE_T *render_state,
   MEM_HANDLE_T shader_handle, const uint64_t *shader, uint32_t *unifs, uint32_t unifs_n)
{
#ifdef __BCM2708A0__
   SHADER_RECORD_T *shader_record;
#endif
   uint8_t *p;

   /* todo: make this work for pointers as well as handles? */
   if ((shader_handle != MEM_INVALID_HANDLE) &&
      (render_state->shader == shader_handle) &&
      (render_state->shader_unifs == unifs)) {
      return true;
   }
   render_state->shader = shader_handle;
   render_state->shader_unifs = unifs;

#ifdef VG_BE_DUMP_UNIFS
   if (shader_handle != MEM_INVALID_HANDLE) {
      printf("unifs (shader handle: 0x%08x) {\n", shader_handle);
   } else {
      printf("unifs (shader: 0x%p) {\n", shader);
   }
   if (unifs_handle_offset.mh_handle != MEM_INVALID_HANDLE) {
      uint32_t *unifs = (uint32_t *)((uint8_t *)mem_lock(unifs_handle_offset.mh_handle) + unifs_handle_offset.offset);
      uint32_t i;
      for (i = 0; i != unifs_n; ++i) {
         printf("0x%08x (%f)\n", unifs[i], float_from_bits(unifs[i]));
      }
      mem_unlock(unifs_handle_offset.mh_handle);
   }
   printf("}\n");
#endif

#ifdef __BCM2708A0__
   /*
      alloc and fill in shader record
   */

   shader_record = (SHADER_RECORD_T *)khrn_fmem_junk(render_state->fmem, sizeof(SHADER_RECORD_T), 16);
   if (!shader_record) { goto error_1; }
   shader_record->flags = 0;
   shader_record->stride = 0;
   shader_record->unifs_n = (uint8_t)unifs_n;
   shader_record->varys_n = 0;
   if (shader_handle != MEM_INVALID_HANDLE) {
      vcos_assert(!shader);
      vcos_assert(mem_get_align(shader_handle) >= 8);
#ifdef SIMPENROSE_RECORD_OUTPUT
      record_map_mem_buffer(shader_handle, L_FRAGMENT_SHADER, RECORD_BUFFER_IS_BOTH, 8);
#endif
      if (!khrn_fmem_fix_special_0(render_state->fmem,
         &shader_record->shader,
         shader_handle, 0)) { goto error_1; } /* todo: no need for add special 0 thing on b0 */
   } else {
      vcos_assert(shader);
      vcos_assert(!((uintptr_t)shader & 0x7));
      if (!khrn_fmem_fix_special_0(render_state->fmem,
         &shader_record->shader,
         MEM_INVALID_HANDLE, khrn_hw_addr(shader))) { goto error_1; } /* todo: no need for this fix on b0 */
   }
   if (unifs == NULL) {
      /* we need a valid pointer as the hardware *will* try to read the
       * uniforms! the clear shader pointer will do (it's sufficiently aligned
       * and will be in the right alias) */
      shader_record->unifs = khrn_hw_addr(fd_clear_shader);
   } else {
      vcos_assert(!((size_t)unifs & 0x3));
      shader_record->unifs = khrn_hw_addr(khrn_hw_alias_direct(unifs));
   }

   /*
      write shader instruction
   */

   p = khrn_fmem_cle(render_state->fmem, 5);
   if (!p) { goto error_0; }
   Add_byte(p, KHRN_HW_INSTR_VG_SHADER);
   vcos_assert(!((size_t)shader_record & 0xf));
#ifdef SIMPENROSE_RECORD_OUTPUT
   record_map_mem_buffer_section(shader_record_handle_offset.mh_handle, shader_record_handle_offset.offset, 12, L_VG_SHADER_RECORD, RECORD_BUFFER_IS_BOTH, 16);
#endif
   ADD_POINTER(p, shader_record);
   vcos_assert(khrn_fmem_is_here(render_state->fmem, p));

   return true;
error_1:
error_0:
   return false;
#else
   /*
      write inline shader instruction
   */

   p = khrn_fmem_cle(render_state->fmem, 9);
   if (!p) { return false; }
   Add_byte(p, KHRN_HW_INSTR_INLINE_VG_SHADER);
   if (shader_handle != MEM_INVALID_HANDLE) {
      vcos_assert(!shader);
      vcos_assert(mem_get_align(shader_handle) >= 8);
#ifdef SIMPENROSE_RECORD_OUTPUT
      record_map_mem_buffer(shader_handle, L_FRAGMENT_SHADER, RECORD_BUFFER_IS_BOTH, 8);
#endif
      if (!khrn_fmem_add_fix_special_0(render_state->fmem, &p,
         shader_handle, 0)) { return false; } /* todo: no need for add special 0 thing on b0 */
   } else {
      vcos_assert(shader);
      vcos_assert(!((uintptr_t)shader & 0x7));
      if (!khrn_fmem_add_fix_special_0(render_state->fmem, &p,
         MEM_INVALID_HANDLE, khrn_hw_addr(shader))) { return false; } /* todo: no need for this fix on b0 */
   }
   if (unifs == NULL) {
      /* we need a valid pointer as the hardware *will* try to read the
       * uniforms! the clear shader pointer will do (it's sufficiently aligned
       * and will be in the right alias) */
      ADD_POINTER(p, fd_clear_shader);
   } else {
      vcos_assert(!((size_t)unifs & 3));
      ADD_POINTER(p, unifs);
   }
   vcos_assert(khrn_fmem_is_here(render_state->fmem, p));

   return true;
#endif
}

static INLINE VG_SHADER_FD_DRAW_TYPE_T convert_image_mode(VGImageMode image_mode)
{
   vcos_assert((image_mode >= VG_DRAW_IMAGE_NORMAL) && (image_mode <= VG_DRAW_IMAGE_STENCIL));
   return (VG_SHADER_FD_DRAW_TYPE_T)(VG_SHADER_FD_DRAW_TYPE_IMAGE + (image_mode - VG_DRAW_IMAGE_NORMAL));
}

static INLINE VG_SHADER_FD_PAINT_TYPE_T convert_paint_type(VGPaintType paint_type)
{
   vcos_assert((paint_type >= VG_PAINT_TYPE_COLOR) && (paint_type <= VG_PAINT_TYPE_PATTERN));
   return (VG_SHADER_FD_PAINT_TYPE_T)(VG_SHADER_FD_PAINT_TYPE_SOLID + (paint_type - VG_PAINT_TYPE_COLOR));
}

static INLINE VG_SHADER_FD_BLEND_T convert_blend_mode(VGBlendMode blend_mode)
{
   vcos_assert((blend_mode >= VG_BLEND_SRC) && (blend_mode <= VG_BLEND_ADDITIVE));
   return (VG_SHADER_FD_BLEND_T)(VG_SHADER_FD_BLEND_SRC + (blend_mode - VG_BLEND_SRC));
}

/*
   if false is returned, the scissor handle is unretained by this function
   (otherwise, it is added to fixer and will be automatically unretained later)
*/

static bool set_fd_shader(VG_BE_RENDER_STATE_T *render_state,
   MEM_HANDLE_T scissor_handle,
   VGImageQuality image_paint_pattern_quality,
   MEM_HANDLE_T paint_handle, uint32_t tile_fill_rgba, const VG_MAT3X3_T *surface_to_paint,
   MEM_HANDLE_T image_handle, const uint32_t *image_rect, VGImageMode image_mode, const VG_MAT3X3_T *surface_to_image,
   const float *color_transform,
   VGBlendMode blend_mode, bool coverage, bool masking,
   MEM_HANDLE_T *shader_handle_out, uint32_t **unifs_out, uint32_t *unifs_n_out)
{
   VG_PAINT_T *paint = (VG_PAINT_T *)mem_lock(paint_handle);
   uint32_t *unifs;
   uint32_t *unifs_begin;
   bool solid_paint;
   uint32_t rgba = 0; /* init to avoid warnings */
   uint32_t unifs_n;
   MEM_HANDLE_T shader_handle;

   /*
      partially setup shader key (enough for get_fd_unifs_n)
   */

   VG_SHADER_FD_KEY_T key = { 0 };
   key.scissor = scissor_handle != MEM_INVALID_HANDLE;
   key.draw_type = (image_handle == MEM_INVALID_HANDLE) ? VG_SHADER_FD_DRAW_TYPE_PAINT : convert_image_mode(image_mode);
   if (key.draw_type != VG_SHADER_FD_DRAW_TYPE_IMAGE) {
      key.paint_type = convert_paint_type(paint->type);
      if (paint->type == VG_PAINT_TYPE_PATTERN) {
         key.paint_pattern_tile_fill = paint->pattern_tiling_mode == VG_TILE_FILL;
      }
   }
   if (image_handle != MEM_INVALID_HANDLE) {
      key.image_projective = !vg_mat3x3_is_affine(surface_to_image);
   }
   key.color_transform = !!color_transform;
   key.blend = /* SRC_IN is the same as SRC if the frame doesn't have an alpha channel */
      (!(render_state->frame_format & IMAGE_FORMAT_A) && (blend_mode == VG_BLEND_SRC_IN)) ?
      VG_SHADER_FD_BLEND_SRC : convert_blend_mode(blend_mode);
   key.coverage = coverage;
   key.masking = masking;
   key.pre = !!(render_state->frame_format & IMAGE_FORMAT_PRE);
   key.no_alpha = !(render_state->frame_format & IMAGE_FORMAT_A);

   /*
      allocate enough space for uniforms
      we might allocate too much -- we'll unalloc the extra space later
   */

   unifs = khrn_fmem_junk(render_state->fmem, get_fd_unifs_n(key) * sizeof(uint32_t), 4);
   if (!unifs) {
      mem_unlock(paint_handle);
      if (scissor_handle != MEM_INVALID_HANDLE) { mem_unretain(scissor_handle); }
      return false;
   }
   unifs_begin = unifs;

   /*
      write out unifs and setup rest of key
   */

   if ((scissor_handle != MEM_INVALID_HANDLE) &&
      !add_unifs_scissor(render_state, &unifs,
      scissor_handle)) {
      mem_unlock(paint_handle);
      mem_unretain(scissor_handle);
      return false;
   }

   if (key.draw_type == VG_SHADER_FD_DRAW_TYPE_IMAGE) {
      solid_paint = false;
   } else {
      if (!add_unifs_paint(render_state, &unifs,
         paint, image_paint_pattern_quality, tile_fill_rgba, surface_to_paint, &key, &rgba)) {
         mem_unlock(paint_handle);
         return false;
      }
      solid_paint = key.paint_type == VG_SHADER_FD_PAINT_TYPE_SOLID;
   }

   mem_unlock(paint_handle); /* done with paint now... */

   if ((image_handle != MEM_INVALID_HANDLE) &&
      !add_unifs_image_fd(render_state, &unifs,
      image_handle, image_rect, image_paint_pattern_quality, surface_to_image, &key)) {
      return false;
   }

   key.colorspace_convert ^= !!(render_state->frame_format & IMAGE_FORMAT_LIN);

   if (solid_paint && (key.draw_type == VG_SHADER_FD_DRAW_TYPE_MULTIPLY)) {
      *(unifs++) = key.image_pre ? khrn_color_rgba_pre(rgba) : rgba;
      solid_paint = false;
   }

   if (color_transform) {
      if (solid_paint) {
         rgba = khrn_color_rgba_transform(rgba, color_transform);
         key.color_transform = false;
      } else {
         add_unifs_color_transform(&unifs, color_transform);
      }
   }

   if (key.colorspace_convert) {
      if (solid_paint) {
         rgba = khrn_color_rgba_s_to_lin(rgba);
         key.colorspace_convert = false;
      } else {
         add_unifs_colorspace_convert(&unifs, !!(render_state->frame_format & IMAGE_FORMAT_LIN));
      }
   }

   if (solid_paint) {
      if (key.draw_type == VG_SHADER_FD_DRAW_TYPE_STENCIL) {
         *(unifs++) = (!vg_shader_fd_need_stencil_a((VG_SHADER_FD_BLEND_T)key.blend) &&
         !key.image_pre) ? rgba : khrn_color_rgba_pre(rgba);
      } else {
         *(unifs++) = khrn_color_rgba_pre(rgba); /*(key.pre || (key.blend != VG_SHADER_FD_BLEND_SRC)) ?
            khrn_color_rgba_pre(rgba) : khrn_color_rgba_rz(rgba);*/
      }
   }

   /*
      shrink back unifs allocation (to how much we actually used)
   */

   unifs_n = unifs - unifs_begin;
   vcos_assert(unifs_n <= get_fd_unifs_n(key));
//   khrn_cmem_locked_unalloc_to(render_state->unifs, unifs_locked, unifs);
#ifdef SIMPENROSE_RECORD_OUTPUT
   if (unifs != unifs_begin) {
      record_map_buffer(khrn_hw_addr(unifs_begin), (unifs - unifs_begin) * 4, L_UNIFORMS, RECORD_BUFFER_IS_DATA | RECORD_BUFFER_ALLOW_BEFORE_CLIF, 4);
   }
   record_map_mem_buffer_section(unifs_handle_offset.mh_handle, unifs_handle_offset.offset, unifs_n * 4, L_UNIFORMS, RECORD_BUFFER_IS_CLIF, 4); /* todo: assuming this will mem_lock now */
#endif

   /*
      gen shader
   */

   render_state->extra_thrsws_min = _min(render_state->extra_thrsws_min, vg_shader_fd_get_extra_thrsws(key)); /* todo: remove on b0 */
   shader_handle = vg_shader_fd_get(key);
   if (shader_handle == MEM_INVALID_HANDLE) {
      return false;
   }

   /*
      finally, set shader + unifs
   */

   if (!set_shader(render_state, shader_handle, NULL, unifs_begin, unifs_n)) {
      return false;
   }

   /*
      possibly return handles
   */

   if (shader_handle_out) { *shader_handle_out = shader_handle; }
   if (unifs_out) { *unifs_out = unifs_begin; }
   if (unifs_n_out) { *unifs_n_out = unifs_n; }

   return true;
}

#ifndef __BCM2708A0__

static bool set_md_clear_shader(VG_BE_RENDER_STATE_T *render_state,
   uint32_t value)
{
   uint32_t *unifs = khrn_fmem_junk(render_state->fmem, sizeof(uint32_t), 4), *unifs_begin;
   if (!unifs) { return false; }
   unifs_begin = unifs;
   *(unifs++) = value;
//   vcos_assert(khrn_fmem_is_here(render_state->fmem, unifs));
#ifdef SIMPENROSE_RECORD_OUTPUT
   record_map_mem_buffer_section(unifs_handle_offset.mh_handle, unifs_handle_offset.offset, 4, L_UNIFORMS, RECORD_BUFFER_IS_BOTH, 4);
#endif
   return set_shader(render_state, MEM_INVALID_HANDLE, md_clear_shader, unifs_begin, 1);
}

static bool set_md_image_shader(VG_BE_RENDER_STATE_T *render_state,
   MEM_HANDLE_T image_handle, int32_t image_x, int32_t image_y,
   VGMaskOperation operation)
{
   uint32_t unifs_n;
   uint32_t *unifs, *unifs_begin;
   bool no_alpha;
   void *shader = NULL; /* init to avoid warnings */

   vcos_assert((operation == VG_SET_MASK) || (operation == VG_UNION_MASK) ||
      (operation == VG_INTERSECT_MASK) || (operation == VG_SUBTRACT_MASK));

   /*
      write uniforms
   */

   unifs_n = 6;
   unifs = khrn_fmem_junk(render_state->fmem, unifs_n * sizeof(uint32_t), 4);
   if (!unifs) { return false; }
   unifs_begin = unifs;

   if (!add_unifs_image_md(render_state,
      &unifs,
      image_handle, image_x, image_y, &no_alpha)) {
      return false;
   }

//   vcos_assert(khrn_fmem_is_here(render_state->fmem, unifs));
#ifdef SIMPENROSE_RECORD_OUTPUT
   vcos_assert(unifs == unifs_begin);
   record_map_mem_buffer_section(unifs_handle_offset.mh_handle, unifs_handle_offset.offset, unifs_n * 4, L_UNIFORMS, RECORD_BUFFER_IS_CLIF, 4); /* todo: assuming this will mem_lock now */
#endif

   /*
      pick shader
   */

   render_state->extra_thrsws_min = _min(render_state->extra_thrsws_min, VG_SHADER_MD_IMAGE_EXTRA_THRSWS); /* todo: remove this */
   switch (operation) {
   case VG_SET_MASK:       shader = no_alpha ? md_image_no_alpha_set_shader : md_image_set_shader; break;
   case VG_UNION_MASK:     shader = no_alpha ? md_image_no_alpha_union_shader : md_image_union_shader; break;
   case VG_INTERSECT_MASK: shader = no_alpha ? md_image_no_alpha_intersect_shader : md_image_intersect_shader; break;
   case VG_SUBTRACT_MASK:  shader = no_alpha ? md_image_no_alpha_subtract_shader : md_image_subtract_shader; break;
   default:                UNREACHABLE();
   }

   /*
      set shader and unifs
   */

   return set_shader(render_state, MEM_INVALID_HANDLE, (const uint64_t *)shader, unifs_begin, unifs_n);
}

/*
   if false is returned, the scissor handle is unretained by this function
   (otherwise, it is added to fixer and will be automatically unretained later)
*/

static bool set_md_cvg_shader(VG_BE_RENDER_STATE_T *render_state,
   MEM_HANDLE_T scissor_handle,
   VGMaskOperation operation)
{
   uint32_t unifs_n;
   void *shader = NULL; /* init to avoid warnings */
   uint32_t *unifs = NULL, *unifs_begin = NULL;

   vcos_assert((operation == VG_SET_MASK) || (operation == VG_UNION_MASK) ||
      (operation == VG_INTERSECT_MASK) || (operation == VG_SUBTRACT_MASK));

   /*
      write uniforms and pick shader
   */

   if (scissor_handle == MEM_INVALID_HANDLE) {
      /* no uniforms! */
      unifs_n = 0;

      switch (operation) {
      case VG_SET_MASK:       shader = md_cvg_set_shader; break;
      case VG_UNION_MASK:     shader = md_cvg_union_shader; break;
      case VG_INTERSECT_MASK: shader = md_cvg_intersect_shader; break;
      case VG_SUBTRACT_MASK:  shader = md_cvg_subtract_shader; break;
      default:                UNREACHABLE();
      }
   } else {

      unifs_n = 4;
      unifs = khrn_fmem_junk(render_state->fmem, unifs_n * sizeof(uint32_t), 4);
      if (!unifs) { mem_unretain(scissor_handle); return false; }
      unifs_begin = unifs;

      if (!add_unifs_scissor(render_state, &unifs,
         scissor_handle)) { mem_unretain(scissor_handle); return false; }
      /* fixer will now automatically unretain scissor_handle */

//      vcos_assert(khrn_fmem_is_here(render_state->fmem, unifs));
#ifdef SIMPENROSE_RECORD_OUTPUT
      vcos_assert(unifs == unifs_begin);
      record_map_mem_buffer_section(unifs_handle_offset.mh_handle, unifs_handle_offset.offset, unifs_n * 4, L_UNIFORMS, RECORD_BUFFER_IS_CLIF, 4); /* todo: assuming this will mem_lock now */
#endif

      render_state->extra_thrsws_min = _min(render_state->extra_thrsws_min, VG_SHADER_MD_CVG_SCISSOR_EXTRA_THRSWS); /* todo: remove this */
      switch (operation) {
      case VG_SET_MASK:       shader = md_cvg_scissor_set_shader; break;
      case VG_UNION_MASK:     shader = md_cvg_scissor_union_shader; break;
      case VG_INTERSECT_MASK: shader = md_cvg_scissor_intersect_shader; break;
      case VG_SUBTRACT_MASK:  shader = md_cvg_scissor_subtract_shader; break;
      default:                UNREACHABLE();
      }
   }

   /*
      set shader and unifs
   */

   return set_shader(render_state, MEM_INVALID_HANDLE, (const uint64_t *)shader, unifs_begin, unifs_n);
}

#endif

static bool set_offset(VG_BE_RENDER_STATE_T *render_state,
   int32_t x, int32_t y)
{
   uint32_t offset = (y << 16) | (x & 0xffff);
   if (render_state->offset != offset) {
      uint8_t *p = khrn_fmem_cle(render_state->fmem, 5);
      if (!p) { return false; }
      Add_byte(p, KHRN_HW_INSTR_STATE_VIEWPORT_OFFSET);
      ADD_WORD(p, offset);
      vcos_assert(khrn_fmem_is_here(render_state->fmem, p));

      render_state->offset = offset;
   }
   return true;
}

static bool set_clip(VG_BE_RENDER_STATE_T *render_state,
   const uint32_t *rect)
{
   uint32_t clip_xy, clip_wh;
   vcos_assert((rect[2] != 0) && (rect[3] != 0)); /* hw doesn't like 0 width/height clip rects */
   clip_xy = (rect[1] << 16) | rect[0];
   clip_wh = (rect[3] << 16) | rect[2];
   if ((render_state->clip_xy != clip_xy) || (render_state->clip_wh != clip_wh)) {
      uint8_t *p = khrn_fmem_cle(render_state->fmem, 9);
      if (!p) { return false; }
      Add_byte(p, KHRN_HW_INSTR_STATE_CLIP);
      ADD_WORD(p, clip_xy);
      ADD_WORD(p, clip_wh);
      vcos_assert(khrn_fmem_is_here(render_state->fmem, p));

      render_state->clip_xy = clip_xy;
      render_state->clip_wh = clip_wh;
   }
   return true;
}

static INLINE bool set_clip_to_frame(VG_BE_RENDER_STATE_T *render_state)
{
   uint32_t rect[4];
   rect[0] = 0;
   rect[1] = 0;
   rect[2] = render_state->frame_width;
   rect[3] = render_state->frame_height;
   return set_clip(render_state, rect);
}

static INLINE bool convert_rendering_quality(VGRenderingQuality rendering_quality)
{
   vcos_assert((rendering_quality == VG_RENDERING_QUALITY_NONANTIALIASED) ||
               (rendering_quality == VG_RENDERING_QUALITY_FASTER) ||
               (rendering_quality == VG_RENDERING_QUALITY_BETTER));
   return rendering_quality != VG_RENDERING_QUALITY_NONANTIALIASED; /* aa? */
}

typedef enum {
   CVG_UPDATE_NONZERO,
   CVG_UPDATE_XOR,
   CVG_UPDATE_OR,
   CVG_UPDATE_ZERO
} CVG_UPDATE_T;

static INLINE CVG_UPDATE_T convert_fill_rule(VGFillRule fill_rule)
{
   vcos_assert((fill_rule == VG_EVEN_ODD) || (fill_rule == VG_NON_ZERO));
   return (fill_rule == VG_EVEN_ODD) ? CVG_UPDATE_XOR : CVG_UPDATE_NONZERO;
}

static bool set_cfg(VG_BE_RENDER_STATE_T *render_state,
   bool aa, bool cvg, CVG_UPDATE_T cvg_update)
{
   /* todo: this register will probably change */
   uint32_t cfg =
      (1 << 0) | /* enable forward-facing prims */
      (1 << 1) | /* enable reverse-facing prims */
      (0 << 2) | /* cw prims (doesn't matter what this is set to) */
      (0 << 3) | /* disable depth offset */
      (0 << 4) | /* disable aa points/lines */
      ((aa ? 2 : 0) << 6) | /* ras oversample mode: 16x if aa, 1x otherwise */
      (cvg << 8) | /* coverage pipe enable */
      (cvg_update << 9) | /* coverage update mode */
      (0 << 11) | /* always clear coverage on read */
      (7 << 12) | /* depth-test always */
      (0 << 15) | /* disable z updates */
      (0 << 16) | /* disable early-z */
      (0 << 17); /* disable early-z updates */
   if (render_state->cfg != cfg) {
      uint8_t *p = khrn_fmem_cle(render_state->fmem, 4);
      if (!p) { return false; }
      Add_byte(p, KHRN_HW_INSTR_STATE_CFG);
      ADD_SHORT(p, (uint16_t)(cfg & 0xffff));
      ADD_BYTE(p, (uint8_t)(cfg >> 16));
      vcos_assert(khrn_fmem_is_here(render_state->fmem, p));

      render_state->cfg = cfg;
   }
   return true;
}

/******************************************************************************
render state handling (start, acquire_buffers, abandon, flush, etc)
******************************************************************************/

typedef enum {
   BA_FRAME_RW   = 1 << 0,
   BA_MASK_READ  = 1 << 1,
   BA_MASK_WRITE = 1 << 2,
   BA_MASK_RW    = BA_MASK_READ | BA_MASK_WRITE
} BA_T;

static void tess_wait_and_finish_llat_callback(bool ok, void *p)
{
   VG_TESS_HANDLE_T *tess_handle;
   UNUSED(ok);
   tess_handle = (VG_TESS_HANDLE_T *)p;
   vg_tess_finish_llat(*tess_handle);
   khrn_barrier();
   *tess_handle = VG_TESS_HANDLE_INVALID;
   khrn_sync_notify_master();
}

static void tess_wait_and_finish_llat(VG_TESS_HANDLE_T tess_handle)
{
   vg_tess_wait(tess_handle, tess_wait_and_finish_llat_callback, &tess_handle);
   while (tess_handle != VG_TESS_HANDLE_INVALID) {
      khrn_sync_master_wait();
   }
   khrn_barrier();
}

static void tess_wait_and_finish(VG_TESS_HANDLE_T tess_handle)
{
   tess_wait_and_finish_llat(tess_handle);
   vg_tess_finish(tess_handle);
}

static bool start_cmems(VG_BE_RENDER_STATE_T *render_state)
{
   KHRN_FMEM_T *fmem;
   uint8_t *p;
   uint32_t width_in_tiles, height_in_tiles, state_size;

   /*
      alloc fmem
   */
   fmem = khrn_fmem_init(khrn_render_state_get_index_from_data(render_state));
   if (!fmem) goto error_0;

   /*
      write initial bin instructions
   */

   p = khrn_fmem_cle(fmem, 19);
   if (!p) { goto error_4; }
   Add_byte(p, KHRN_HW_INSTR_STATE_TILE_BINNING_MODE);
   width_in_tiles = get_width_in_tiles(render_state->frame_width);
   height_in_tiles = get_height_in_tiles(render_state->frame_height);
   state_size = height_in_tiles * width_in_tiles * KHRN_HW_TILE_STATE_SIZE;
   if (!khrn_fmem_add_special(fmem, &p,
      KHRN_HW_SPECIAL_BIN_MEM, 0)) { goto error_4; }
   if (!khrn_fmem_add_special(fmem, &p,
      KHRN_HW_SPECIAL_BIN_MEM_SIZE, -(int32_t)state_size)) { goto error_4; }
   if (!khrn_fmem_add_special(fmem, &p,
      KHRN_HW_SPECIAL_BIN_MEM_END, -(int32_t)state_size)) { goto error_4; }
   ADD_BYTE(p, (uint8_t)width_in_tiles);
   ADD_BYTE(p, (uint8_t)height_in_tiles);
   {
      vcos_static_assert(KHRN_HW_CL_BLOCK_SIZE_MIN == 32);
      vcos_static_assert(KHRN_HW_CL_BLOCK_SIZE_MAX == 256);
   }
   ADD_BYTE(p,
      (0 << 0) | /* disable ms mode */
      (0 << 1) | /* disable 64-bit color mode */
      (1 << 2) | /* auto-init state */
      (0 << 3) | /* 32-byte initial block size */
      (3 << 5)); /* 256-byte regular block size */
   Add_byte(p, KHRN_HW_INSTR_PRIMITIVE_LIST_FORMAT);
   ADD_BYTE(p, 0x32); /* xy triangles */
   Add_byte(p, KHRN_HW_INSTR_START_TILE_BINNING);
   vcos_assert(khrn_fmem_is_here(fmem, p));

   /*
      update render state struct
   */

   render_state->fmem = fmem;

   return true;
error_4:
   khrn_fmem_discard(fmem);
error_0:
   return false;
}

#ifdef __BCM2708A0__
static bool hw2371_workaround(VG_BE_RENDER_STATE_T *render_state);
#endif
#ifdef WORKAROUND_HW2885
static bool hw2885_workaround(VG_BE_RENDER_STATE_T *render_state);
#endif
#if defined(__BCM2708A0__) || defined(WORKAROUND_HW2885)
static void abandon(VG_BE_RENDER_STATE_T *render_state);
#endif

static bool start(void)
{
   if (current_render_state &&
      (((vg_tess_get_nmem_n(current_render_state->tess) +
      khrn_fmem_get_nmem_n(current_render_state->fmem)) > 32) /* todo: what should this be? */
#ifdef WORKAROUND_HW2116
      /* on a0, the global state change counter will overflow at 0x2000 as it
       * only has 13 bits. the overflow isn't handled by the hardware at all, so
       * we want to make sure it doesn't happen
       *
       * on b0, when the global state change counter reaches 0x1f01, the
       * hardware flushes all outstanding state changes and resets the counter.
       * but there's a bug that can mean we end up with invalid instructions in
       * the tile lists when this happens. so we want to make sure the counter
       * doesn't reach 0x1f01
       *
       * the counter will increment on every primitive -> state transition,
       * which is essentially every batch of primitives. we flush as soon as we
       * see the batch count is > 0x1ef0; a bit of breathing space because:
       * - there may be some uncounted batches at the start for workarounds (eg
       *   hw-2371, hw-2885)
       * - we only check at the start of each draw command */
      || (current_render_state->batches_n > 0x1ef0)
#endif
      )) {
      vg_be_render_state_flush(current_render_state);
      vcos_assert(!current_render_state);
   }

   if (!current_render_state) {
      MEM_HANDLE_T frame_handle, frame_src_handle, state_handle, mask_handle;
      KHRN_IMAGE_T *frame;
      uint32_t i;

      frame_handle = vg_get_frame();
      frame = (KHRN_IMAGE_T *)mem_lock(frame_handle);
      i = khrn_interlock_render_state_i(khrn_interlock_get_writer(&frame->interlock));
      if ((i != -1) && (khrn_render_state_get_type(i) == KHRN_RENDER_STATE_TYPE_COPY_BUFFER)) {
         /* usurp the copy buffer render state */
         KHRN_COPY_BUFFER_RENDER_STATE_T *cbrs = (KHRN_COPY_BUFFER_RENDER_STATE_T *)khrn_render_state_get_data(i);
         vcos_assert(cbrs->dst == frame_handle);
         vcos_assert(cbrs->src != frame_handle);
         frame_src_handle = cbrs->src;
         khrn_render_state_usurp(i, KHRN_RENDER_STATE_TYPE_VG);
      } else {
         /* either no render state or unrecognised render state type, create a
          * new one */
         i = khrn_render_state_start(KHRN_RENDER_STATE_TYPE_VG);
         frame_src_handle = frame_handle;
      }
      current_render_state = (VG_BE_RENDER_STATE_T *)khrn_render_state_get_data(i);

      /* get the stuff that can fail out of the way first */
      current_render_state->frame_width = frame->width; /* frame_width/height needed by start_cmems() */
      current_render_state->frame_height = frame->height;
      current_render_state->tess = vg_tess_start();
      if ((current_render_state->tess != VG_TESS_HANDLE_INVALID) && !start_cmems(current_render_state)) {
         tess_wait_and_finish(current_render_state->tess);
         current_render_state->tess = VG_TESS_HANDLE_INVALID;
      }
      if (current_render_state->tess == VG_TESS_HANDLE_INVALID) {
         if (frame_src_handle == frame_handle) {
            mem_unlock(frame_handle);
         } else {
            khrn_interlock_release(&frame->interlock, khrn_interlock_user(i));
            mem_unlock(frame_handle);
            mem_release(frame_handle);
            khrn_interlock_release(&((KHRN_IMAGE_T *)mem_lock(frame_src_handle))->interlock, khrn_interlock_user(i));
            mem_unlock(frame_src_handle);
            mem_release(frame_src_handle);
         }
         finish(current_render_state);
         return false;
      }

      state_handle = VG_GET_SERVER_STATE_HANDLE();
      mem_acquire(state_handle);
      current_render_state->state = state_handle;

      if (frame_src_handle == frame_handle) {
         mem_acquire(frame_handle);
      }
      current_render_state->frame = frame_handle;
      current_render_state->frame_src = frame_src_handle;
      /* todo: format asserts (can render to rso/tf 8888/565?) */
      current_render_state->frame_format = frame->format;
      mem_unlock(frame_handle);
      mask_handle = vg_get_mask();
      if (mask_handle != MEM_INVALID_HANDLE) {
         mem_acquire(mask_handle);
      }
      current_render_state->mask = mask_handle;
      if (current_render_state->mask != MEM_INVALID_HANDLE) {
         KHRN_IMAGE_T *mask = (KHRN_IMAGE_T *)mem_lock(current_render_state->mask);
         /* todo: format assert (just one format for mask?) */
         /* todo: assert must match frame size? */
         mem_unlock(current_render_state->mask);
      }

#ifdef __BCM2708A0__
      current_render_state->buffers_acquired = 0; /* possibly set to BA_FRAME_RW after calling hw2371_workaround */
#else
      current_render_state->buffers_acquired = (frame_src_handle == frame_handle) ? 0 : BA_FRAME_RW;
#endif

      current_render_state->frame_clear = false;
      current_render_state->mask_clear = false;

      /* initialise to values that will never occur, so we always output the first state change */
      current_render_state->offset = 0x7fff7fff; /* max offset is a little less than this */
      current_render_state->clip_xy = 0xffffffff;
      current_render_state->clip_wh = 0x00000000; /* 0 w/h clip rects not allowed by hardware */
      current_render_state->cfg = 0xffffffff; /* we never have all bits set */
      current_render_state->shader = MEM_INVALID_HANDLE;

      current_render_state->glyph_fill_shader = MEM_INVALID_HANDLE;
      current_render_state->glyph_stroke_shader = MEM_INVALID_HANDLE;
      current_render_state->glyph_fill_state_id = ++glyph_state_id;
      current_render_state->glyph_stroke_state_id = ++glyph_state_id;

      current_render_state->extra_thrsws_min = VG_SHADER_FD_CLEAR_EXTRA_THRSWS;
#ifndef __BCM2708A0__
      vcos_assert(VG_SHADER_MD_CLEAR_EXTRA_THRSWS == VG_SHADER_FD_CLEAR_EXTRA_THRSWS);
      vcos_assert(VG_SHADER_MD_CVG_EXTRA_THRSWS == VG_SHADER_FD_CLEAR_EXTRA_THRSWS);
#endif
#ifdef WORKAROUND_HW2885
      vcos_assert(NOP_SHADER_EXTRA_THRSWS == VG_SHADER_FD_CLEAR_EXTRA_THRSWS);
#endif
#ifdef WORKAROUND_HW2116
      current_render_state->batches_n = 0;
#endif

#ifdef SIMPENROSE_RECORD_OUTPUT
      khrn_render_state_flush_except(i); /* or we'll get multiple render states trying to use record at the same time. which would be bad */
      record_begin();
      record_map_shaders();
#endif

#ifdef __BCM2708A0__
      if (frame_src_handle != frame_handle) {
         bool success = hw2371_workaround(current_render_state);
         current_render_state->buffers_acquired = BA_FRAME_RW;
         if (!success) {
            abandon(current_render_state);
            return false;
         }
      }
#endif

#ifdef WORKAROUND_HW2885
      if (!hw2885_workaround(current_render_state)) {
         abandon(current_render_state);
         return false;
      }
#endif
   }

   return true;
}

#ifdef __BCM2708A0__
static bool hw2371_workaround(VG_BE_RENDER_STATE_T *render_state)
{
   uint32_t *unifs, *unifs_begin;
   uint32_t width_height[2];

   /*
      we call this if we're about to acquire_buffers(BA_FRAME_RW) but we're not
      going to set frame_clear

      as the full load instructions have issues (see hw-2371), we load the
      previous frame (if necessary) using the tu
   */

   if (render_state->buffers_acquired & BA_FRAME_RW) {
      return true;
   }

   unifs = khrn_fmem_junk(render_state->fmem, 4 * sizeof(uint32_t), 4);
   if (!unifs) { goto error; }
   unifs_begin = unifs;
   get_tu_width_height(width_height, render_state->frame);
   *(unifs++) = float_to_bits(recip_((float)width_height[0]));
   *(unifs++) = float_to_bits(recip_((float)width_height[1]));
   if (!add_unifs_tu_image(render_state,
      &unifs,
      render_state->frame_src,
      render_state->frame_format, width_height[0], width_height[1],
      false, TILE_PAD, /* tile mode shouldn't matter */
      false, NULL)) { goto error; }
   //vcos_assert(khrn_cmem_locked_is_here(render_state->unifs, &unifs_locked, unifs));
#ifdef SIMPENROSE_RECORD_OUTPUT
   vcos_assert(unifs == unifs_begin);
   record_map_mem_buffer_section(unifs_handle_offset.mh_handle, unifs_handle_offset.offset,
      (unifs - (uint32_t *)((uint8_t *)unifs_base + unifs_handle_offset.offset)) * 4,
      L_UNIFORMS, RECORD_BUFFER_IS_CLIF, 4); /* todo: assuming this will mem_lock now */
#endif
   render_state->extra_thrsws_min = _min(render_state->extra_thrsws_min, FULL_LOAD_SHADER_EXTRA_THRSWS);
   if (!set_shader(render_state, MEM_INVALID_HANDLE, full_load_shader, unifs_begin, 4)) { goto error; }
   if (!set_offset(render_state, 0, 0)) { goto error; }
   if (!set_clip_to_frame(render_state)) { goto error; }

   if (!set_cfg(render_state, false, false, (CVG_UPDATE_T)0)) { goto error; }
   if (!vg_tess_rect(render_state->fmem, 0, 0, render_state->frame_width, render_state->frame_height)) { goto error; }

   return true;
error:
   return false;
}
#endif

#ifdef WORKAROUND_HW2885
static bool hw2885_workaround(VG_BE_RENDER_STATE_T *render_state)
{
   if (!set_shader(render_state, MEM_INVALID_HANDLE, nop_shader, NULL, 0)) { goto error; }
   if (!set_offset(render_state, 0, 0)) { goto error; }
   if (!set_clip_to_frame(render_state)) { goto error; }

   if (!set_cfg(render_state, false, false, (CVG_UPDATE_T)0)) { goto error; }
   if (!vg_tess_rect(render_state->fmem, 0, 0,
#ifdef __BCM2708A0__
      /* using really small primitives here seems to provoke hw-2555 (see
       * hw-2927). hw-2555 isn't workaroundable, so there's no real solution,
       * but using larger primitives seems to work. note that hw-2555 was fixed
       * for b0 */
      4, 4
#else
      1, 1
#endif
      )) { goto error; }

   return true;
error:
   return false;
}
#endif

static void acquire_buffers(VG_BE_RENDER_STATE_T *render_state, BA_T ba)
{
   vcos_assert(ba);

   if ((ba & BA_FRAME_RW) && !(render_state->buffers_acquired & BA_FRAME_RW)) {
      KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)mem_lock(render_state->frame);
      if (khrn_interlock_is_invalid(&image->interlock)){
         current_render_state->frame_clear = 1;
         render_state->frame_clear_rgba = 0xff00ffff; /* invalid frame is yellow */
      }
      khrn_interlock_write(&image->interlock,
         khrn_interlock_user(khrn_render_state_get_index_from_data(render_state)));
      mem_unlock(render_state->frame);
      vcos_assert(render_state->frame_src == render_state->frame); /* or we should already have BA_FRAME_RW */
   }
   if (ba & BA_MASK_WRITE) {
      vcos_assert(ba & BA_MASK_READ); /* don't call with just BA_MASK_WRITE */
      if (!(render_state->buffers_acquired & BA_MASK_WRITE)) {
         khrn_interlock_write(&((KHRN_IMAGE_T *)mem_lock(render_state->mask))->interlock,
            khrn_interlock_user(khrn_render_state_get_index_from_data(render_state)));
         mem_unlock(render_state->mask);
      }
   } else if ((ba & BA_MASK_READ) && !(render_state->buffers_acquired & BA_MASK_READ)) {
      khrn_interlock_read(&((KHRN_IMAGE_T *)mem_lock(render_state->mask))->interlock,
         khrn_interlock_user(khrn_render_state_get_index_from_data(render_state)));
      mem_unlock(render_state->mask);
   }
   render_state->buffers_acquired |= ba;
}

static void transfer_buffers(VG_BE_RENDER_STATE_T *render_state)
{
   if (render_state->buffers_acquired & BA_FRAME_RW) {
      khrn_interlock_transfer(&((KHRN_IMAGE_T *)mem_lock(render_state->frame))->interlock,
         khrn_interlock_user(khrn_render_state_get_index_from_data(render_state)), KHRN_INTERLOCK_FIFO_HW_RENDER);
      mem_unlock(render_state->frame);
      if (render_state->frame_src != render_state->frame) {
         khrn_interlock_transfer(&((KHRN_IMAGE_T *)mem_lock(render_state->frame_src))->interlock,
            khrn_interlock_user(khrn_render_state_get_index_from_data(render_state)), KHRN_INTERLOCK_FIFO_HW_RENDER);
         mem_unlock(render_state->frame_src);
      }
   }
   if (render_state->buffers_acquired & BA_MASK_RW) {
      vcos_assert(!(render_state->buffers_acquired & BA_MASK_WRITE) ||
         (render_state->buffers_acquired & BA_MASK_READ)); /* shouldn't have just BA_MASK_WRITE */
      khrn_interlock_transfer(&((KHRN_IMAGE_T *)mem_lock(render_state->mask))->interlock,
         khrn_interlock_user(khrn_render_state_get_index_from_data(render_state)), KHRN_INTERLOCK_FIFO_HW_RENDER);
      mem_unlock(render_state->mask);
   }
}

static void release_buffers(VG_BE_RENDER_STATE_T *render_state)
{
   if (render_state->buffers_acquired & BA_FRAME_RW) {
      khrn_interlock_release(&((KHRN_IMAGE_T *)mem_lock(render_state->frame))->interlock,
         khrn_interlock_user(khrn_render_state_get_index_from_data(render_state)));
      mem_unlock(render_state->frame);
      if (render_state->frame_src != render_state->frame) {
         khrn_interlock_release(&((KHRN_IMAGE_T *)mem_lock(render_state->frame_src))->interlock,
            khrn_interlock_user(khrn_render_state_get_index_from_data(render_state)));
         mem_unlock(render_state->frame_src);
      }
   }
   if (render_state->buffers_acquired & BA_MASK_RW) {
      vcos_assert(!(render_state->buffers_acquired & BA_MASK_WRITE) ||
         (render_state->buffers_acquired & BA_MASK_READ)); /* shouldn't have just BA_MASK_WRITE */
      khrn_interlock_release(&((KHRN_IMAGE_T *)mem_lock(render_state->mask))->interlock,
         khrn_interlock_user(khrn_render_state_get_index_from_data(render_state)));
      mem_unlock(render_state->mask);
   }
}

static void abandon(VG_BE_RENDER_STATE_T *render_state)
{
#ifdef SIMPENROSE_RECORD_OUTPUT
   vcos_assert(0); /* todo */
#endif

   /* the tessellation stuff might have pending writes to stuff in the fmem, so
    * wait for it before discarding the fmem */
   tess_wait_and_finish(render_state->tess);

   khrn_fmem_discard(render_state->fmem);

   release_buffers(render_state);

   if (render_state->mask != MEM_INVALID_HANDLE) {
      mem_release(render_state->mask);
   }
   if (render_state->frame_src != render_state->frame) {
      mem_release(render_state->frame_src);
   }
   mem_release(render_state->frame);

   mem_release(render_state->state);

   finish(render_state);
}

static void convert_format_frame(KHRN_IMAGE_FORMAT_T format,
   uint32_t *mem_format, uint32_t *pixel_format)
{
   vcos_assert(khrn_image_is_color(format));
   switch (format & IMAGE_FORMAT_MEM_LAYOUT_MASK) {
   case IMAGE_FORMAT_RSO: *mem_format = 0; break;
   case IMAGE_FORMAT_TF:  *mem_format = 1; break;
#ifndef __BCM2708A0__
   case IMAGE_FORMAT_LT:  *mem_format = 2; break;
#endif
   default:               UNREACHABLE();
   }
   switch (format & ~(IMAGE_FORMAT_MEM_LAYOUT_MASK | IMAGE_FORMAT_PRE | IMAGE_FORMAT_LIN)) {
   case ABGR_8888:                           /* blend works differently for xbgr */
   case XBGR_8888: *pixel_format = 1; break; /* other than that, there's no difference */
#ifndef __BCM2708A0__
   case RGB_565:   *pixel_format = 2; break; /* non-dithered. todo: dither? */
#endif
   default:        UNREACHABLE();
   }
}

typedef struct {
   MEM_HANDLE_T state; /* for error reporting */
   MEM_HANDLE_T frame, frame_src, mask;
   VG_TESS_HANDLE_T tess;
} HW_CALLBACK_DATA_T;

static void hw_callback(KHRN_HW_CALLBACK_REASON_T reason, void *data, const uint32_t *specials)
{
   HW_CALLBACK_DATA_T *d = (HW_CALLBACK_DATA_T *)data;

   switch (reason) {
   case KHRN_HW_CALLBACK_REASON_FIXUP:
   {
      vg_tess_fix(d->tess);
      break;
   }
   case KHRN_HW_CALLBACK_REASON_OUT_OF_MEM_LLAT:
   case KHRN_HW_CALLBACK_REASON_BIN_FINISHED_LLAT:
   {
      vg_tess_finish_llat(d->tess);
      break;
   }
   case KHRN_HW_CALLBACK_REASON_OUT_OF_MEM:
   {
      vg_server_state_set_error((VG_SERVER_STATE_T *)mem_lock(d->state), VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(d->state);
      vg_tess_finish(d->tess);
      /* fallthrough */
   }
   case KHRN_HW_CALLBACK_REASON_RENDER_FINISHED:
   {
      if (d->mask != MEM_INVALID_HANDLE) {
         mem_release(d->mask);
      }
      if (d->frame != MEM_INVALID_HANDLE) {
         if (d->frame_src != d->frame) {
            mem_release(d->frame_src);
         }
         mem_release(d->frame);
      }
      mem_release(d->state); /* this might call vg_be_term! */
      break;
   }
   case KHRN_HW_CALLBACK_REASON_BIN_FINISHED:
   {
      vg_tess_finish(d->tess);
      break;
   }
   default:
   {
      UNREACHABLE();
   }
   }
}

#ifdef __BCM2708A0__
static void *dummy_tile;

static void *dummy_tile_init(void *p)
{
   vcos_assert(!((uintptr_t)p & (KHRN_HW_TLB_ALIGN - 1)));
   dummy_tile = p;
   return (uint8_t *)p + (KHRN_HW_TILE_HEIGHT * KHRN_HW_TILE_WIDTH * 4);
}
#endif

void vg_be_render_state_flush(VG_BE_RENDER_STATE_T *render_state)
{
   uint32_t padded_frame_width, padded_frame_height;
   uint8_t *p;
   uint32_t clear_rgba;
   uint32_t frame_mem_format, frame_pixel_format;
#ifndef __BCM2708A0__
   bool load_frame, load_mask, store_frame, store_mask;
#endif
   uint32_t tile_cl_size;
   uint32_t width_in_tiles, height_in_tiles, i, j;
   HW_CALLBACK_DATA_T *data;

   /* we should call acquire_buffers if start succeeds, so we shouldn't ever
    * have a render state with no acquired buffers */
   vcos_assert(render_state->buffers_acquired);

   /* on a0, the frame needs to be a multiple of the tile size (the tlb requires
    * this) */
   padded_frame_width = render_state->frame_width;
   padded_frame_height = render_state->frame_height;
#ifdef __BCM2708A0__
   padded_frame_width = round_up(padded_frame_width, KHRN_HW_TILE_WIDTH);
   padded_frame_height = round_up(padded_frame_height, KHRN_HW_TILE_HEIGHT);
#endif

   /* check frame/mask */
#ifndef NDEBUG
   {
      KHRN_IMAGE_T *frame = (KHRN_IMAGE_T *)mem_lock(render_state->frame);
      vcos_assert(frame->flags & IMAGE_FLAG_RENDER_TARGET);
      vcos_assert(frame->format == render_state->frame_format);
      {
         /* stride needs to be a multiple of KHRN_HW_TLB_ALIGN. this is
          * trivially satisfied for lineartile and tformat. raster order formats
          * are padded */
         uint32_t w = khrn_image_is_rso(frame->format) ?
            round_up(padded_frame_width, (KHRN_HW_TLB_ALIGN * 8) / khrn_image_get_bpp(frame->format)) :
            padded_frame_width;
         vcos_assert(khrn_image_get_space(frame) >= khrn_image_get_size(frame->format, w, padded_frame_height));
         vcos_assert(frame->stride == khrn_image_get_stride(frame->format, w));
      }
      vcos_assert(khrn_image_get_align(frame) >= KHRN_HW_TLB_ALIGN);
      vcos_assert(!(frame->stride & (KHRN_HW_TLB_ALIGN - 1)));
      mem_unlock(render_state->frame);
   }
   if (render_state->mask != MEM_INVALID_HANDLE) {
      KHRN_IMAGE_T *mask = (KHRN_IMAGE_T *)mem_lock(render_state->mask);
      vcos_assert(mask->flags & IMAGE_FLAG_RENDER_TARGET);
      vcos_assert(mask->format == A_8_RSO);
      {
         /* mask is padded so stride is a multiple of KHRN_HW_TLB_ALIGN */
         uint32_t w = round_up(padded_frame_width, KHRN_HW_TLB_ALIGN);
         vcos_assert(khrn_image_get_space(mask) >= khrn_image_get_size(mask->format, w, padded_frame_height));
         vcos_assert(mask->stride == khrn_image_get_stride(mask->format, w));
      }
      vcos_assert(khrn_image_get_align(mask) >= KHRN_HW_TLB_ALIGN);
      vcos_assert(!(mask->stride & (KHRN_HW_TLB_ALIGN - 1)));
      mem_unlock(render_state->mask);
   }
#endif

   /* map frame/mask for recording and set recording frame config */
#ifdef SIMPENROSE_RECORD_OUTPUT
   record_set_frame_config(padded_frame_width, padded_frame_height, khrn_image_get_bpp(render_state->frame_format), khrn_image_is_tformat(render_state->frame_format));
   if (render_state->buffers_acquired & BA_FRAME_RW) {
#ifdef __BCM2708A0__
      if (render_state->frame_clear || (render_state->frame_src != render_state->frame)) {
#endif
         record_map_mem_buffer(((KHRN_IMAGE_T *)mem_lock(render_state->frame))->mh_storage, L_FRAMEBUFFER, RECORD_BUFFER_IS_BOTH, KHRN_HW_TLB_ALIGN);
         mem_unlock(render_state->frame);
#ifdef __BCM2708A0__
      } /* else: already mapped as texture */
#endif
   }
   if (render_state->buffers_acquired & BA_MASK_RW) {
      record_map_mem_buffer(((KHRN_IMAGE_T *)mem_lock(render_state->mask))->mh_storage, L_FRAMEBUFFER, RECORD_BUFFER_IS_BOTH, KHRN_HW_TLB_ALIGN);
      mem_unlock(render_state->mask);
   }
#endif

   /*
      write flush to bin list
   */

   p = khrn_fmem_cle(render_state->fmem, 2);
   if (!p) { goto error_0; }
   /* order of these is somewhat counter-intuitive
    * incr semaphore actually just sets a flag saying "increment semaphore when next flush completes" */
   Add_byte(p, KHRN_HW_INSTR_INCR_SEMAPHORE);
   Add_byte(p, KHRN_HW_INSTR_FLUSH);
   vcos_assert(khrn_fmem_is_here(render_state->fmem, p));

   /*
      generate render list
   */

   if (!khrn_fmem_start_render(render_state->fmem)) { goto error_0; }

   p = khrn_fmem_cle(render_state->fmem,
      /* KHRN_HW_INSTR_STATE_CLEARCOL */ 14 +
#ifdef __BCM2708A0__
      /* KHRN_HW_INSTR_STATE_TILE_RENDERING_MODE */ 11 +
      /* KHRN_HW_INSTR_STATE_TILE_COORDS */ 3 +
      /* KHRN_HW_INSTR_STORE_SUBSAMPLE_EOF */ 1 +
#endif
      /* KHRN_HW_INSTR_STATE_TILE_RENDERING_MODE */ 11 +
#ifndef __BCM2708A0__
      /* KHRN_HW_INSTR_STATE_TILE_COORDS */ 3 +
      /* KHRN_HW_INSTR_STORE_GENERAL */ 7 +
#endif
      /* KHRN_HW_INSTR_PRIMITIVE_LIST_FORMAT */ 2 +
      /* KHRN_HW_INSTR_STATE_DEPTH_OFFSET */ 5 +
      /* KHRN_HW_INSTR_STATE_CLIPZ */ 9 +
      /* KHRN_HW_INSTR_WAIT_SEMAPHORE */ 1 +
      /* KHRN_HW_INSTR_MARKER */ 1);
   if (!p) { goto error_1; }

   /* set clear colors */
   if (render_state->frame_clear) {
      clear_rgba = render_state->frame_clear_rgba;
      if (render_state->frame_format & IMAGE_FORMAT_LIN) { clear_rgba = khrn_color_rgba_s_to_lin(clear_rgba); }
      if (render_state->frame_format & IMAGE_FORMAT_PRE) { clear_rgba = khrn_color_rgba_pre(clear_rgba); }
   } else {
      clear_rgba = 0; /* don't care what the clear color is */
   }
   Add_byte(p, KHRN_HW_INSTR_STATE_CLEARCOL);
   ADD_WORD(p, clear_rgba);
   ADD_WORD(p, clear_rgba);
   ADD_WORD(p, render_state->mask_clear_value << 24); /* z and mask clear values */
   ADD_BYTE(p, 0); /* stencil clear value (combined with z to make coverage) */

#ifdef __BCM2708A0__
   /* always need to do a dummy store to clear the z/stencil to 0. we don't want
    * to overwrite the framebuffer, and there's no nop store on a0, so store to
    * dummy_tile instead */
   Add_byte(p, KHRN_HW_INSTR_STATE_TILE_RENDERING_MODE);
#ifdef SIMPENROSE_RECORD_OUTPUT
   record_map_buffer(khrn_hw_addr(dummy_tile), KHRN_HW_TILE_HEIGHT * KHRN_HW_TILE_WIDTH * 4, L_FRAMEBUFFER, RECORD_BUFFER_IS_BOTH, KHRN_HW_TLB_ALIGN);
#endif
   ADD_WORD(p, khrn_hw_addr(dummy_tile));
   ADD_SHORT(p, KHRN_HW_TILE_WIDTH);
   ADD_SHORT(p, KHRN_HW_TILE_HEIGHT);
   ADD_BYTE(p, (uint8_t)(
      (0 << 0) | /* disable ms mode */
      (0 << 1) | /* disable 64-bit color mode */
      (1 << 2) | /* 32-bit rso */
      (0 << 4))); /* 1x decimate (ie none) */
   ADD_BYTE(p, 0); /* unused */
   Add_byte(p, KHRN_HW_INSTR_STATE_TILE_COORDS);
   ADD_BYTE(p, 0);
   ADD_BYTE(p, 0);
   Add_byte(p, KHRN_HW_INSTR_STORE_SUBSAMPLE_EOF);
#endif

   /* rendering mode setup */
   Add_byte(p, KHRN_HW_INSTR_STATE_TILE_RENDERING_MODE);
   if (render_state->buffers_acquired & BA_FRAME_RW) {
      if (!khrn_fmem_add_fix_image(render_state->fmem,
         &p, khrn_render_state_get_index_from_data(render_state),
         render_state->frame, 0)) { goto error_1; }
   } else {
      /* we're just rendering to the mask buffer. we still need the rendering
       * mode instruction to set up the frame width/height and such, but we're
       * not going to load/store the color buffer, so there's no need to provide
       * a valid address */
      ADD_WORD(p, 0);
   }
   ADD_SHORT(p, (uint16_t)padded_frame_width);
   ADD_SHORT(p, (uint16_t)padded_frame_height);
   convert_format_frame(render_state->frame_format, &frame_mem_format, &frame_pixel_format);
   ADD_BYTE(p, (uint8_t)(
      (0 << 0) | /* disable ms mode */
      (0 << 1) | /* disable 64-bit color mode */
      (frame_pixel_format << 2) |
      (0 << 4) | /* 1x decimate (ie none) */
      (frame_mem_format << 6)));
   ADD_BYTE(p, (uint8_t)(
      (!!(render_state->buffers_acquired & BA_MASK_RW) << 0) | /* mask buffer enable */
      (0 << 1) | /* todo: coverage mode */
      (0 << 2) | /* todo: early-z update dir */
      (0 << 3))); /* todo: early-z disable */

#ifndef __BCM2708A0__
   /* always need to do a dummy store to clear the z/stencil to 0. we can use a
    * nop store on b0 */
   Add_byte(p, KHRN_HW_INSTR_STATE_TILE_COORDS);
   ADD_BYTE(p, 0);
   ADD_BYTE(p, 0);
   Add_byte(p, KHRN_HW_INSTR_STORE_GENERAL);
   ADD_BYTE(p, 0); /* don't store anything */
   ADD_BYTE(p, (uint8_t)(
      (!render_state->frame_clear << 5) | /* clear color */
      (0 << 6) | /* clear z/stencil */
      (!render_state->mask_clear << 7))); /* clear mask */
   ADD_WORD(p, 0 << 3); /* not eof */
#endif

   /* we only use xy triangles. by putting the primitive list format here and at
    * the start of the bin list (prior to START_TILE_BINNING), we won't get any
    * primitive list formats binned */
   Add_byte(p, KHRN_HW_INSTR_PRIMITIVE_LIST_FORMAT);
   ADD_BYTE(p, 0x32); /* xy triangles */

   /* need to set depth offset and clipz even though defaults are fine as gl may
    * have changed them... */
   Add_byte(p, KHRN_HW_INSTR_STATE_DEPTH_OFFSET);
   ADD_SHORT(p, 0); /* factor */
   ADD_SHORT(p, 0); /* units */
   Add_byte(p, KHRN_HW_INSTR_STATE_CLIPZ); /* z is 0.5 for xy stuff */
   ADD_FLOAT(p, 0.0f); /* min z */
   ADD_FLOAT(p, 1.0f); /* max z */

   /* wait for binning to complete before going any further. todo: we could
    * load/clear the first tile before waiting... */
#if defined(SIMPENROSE_RECORD_OUTPUT) && !defined(SIMPENROSE_RECORD_BINNING)
   Add_byte(p, KHRN_HW_INSTR_NOP); /* recorded output won't have binning stage, so don't wait for it! */
   Add_byte(p, KHRN_HW_INSTR_NOP);
#else
   Add_byte(p, KHRN_HW_INSTR_WAIT_SEMAPHORE);
   Add_byte(p, KHRN_HW_INSTR_MARKER); /* khrn_hw assumes there is a MARKER just after the WAIT_SEMAPHORE */
#endif

   vcos_assert(khrn_fmem_is_here(render_state->fmem, p));

   /* render the tiles. for each tile we do:
    * - load frame
    * - tile coords
    * - load mask
    * - tile coords
    * - branch to bin list
    * - store mask
    * - tile coords
    * - store frame
    * (with unnecessary loads/stores/tile coords eliminated) */
#ifdef __BCM2708A0__
   tile_cl_size = 9;
#else
   load_frame = (render_state->buffers_acquired & BA_FRAME_RW) && !render_state->frame_clear;
   load_mask = (render_state->buffers_acquired & BA_MASK_READ) && !render_state->mask_clear;
   store_frame = !!(render_state->buffers_acquired & BA_FRAME_RW);
   store_mask = !!(render_state->buffers_acquired & BA_MASK_WRITE);
   tile_cl_size =
      (load_frame ? (7 + (load_mask ? 10 : 0)) : 0) +
      (load_mask ? 7 : 0) + 8 +
      (store_mask ? (7 + (store_frame ? 3 : 0)) : 0) +
      (store_frame ? 1 : 0);
#endif
   width_in_tiles = get_width_in_tiles(render_state->frame_width);
   height_in_tiles = get_height_in_tiles(render_state->frame_height);
   for (j = 0; j != height_in_tiles; ++j) {
      for (i = 0; i != width_in_tiles; ++i) {
         bool eof = (i == (width_in_tiles - 1)) && (j == (height_in_tiles - 1));

         /* todo: fewer, larger allocations? */
         p = khrn_fmem_cle(render_state->fmem,
            tile_cl_size);
         if (!p) { goto error_1; }

#ifndef __BCM2708A0__
         if (load_frame) {
        	 Add_byte(p, KHRN_HW_INSTR_LOAD_GENERAL);
            ADD_BYTE(p, (uint8_t)(
               (1 << 0) | /* load color */
               (frame_mem_format << 4)));
            ADD_BYTE(p, (uint8_t)(
               /* for some reason, the pixel format in the LOAD/STORE_GENERAL
                * instructions is different to the pixel format in the
                * STATE_TILE_RENDERING_MODE instruction... */
               (((4 - frame_pixel_format) % 3) << 0)));
            if (!khrn_fmem_add_fix_image(render_state->fmem,
               &p, khrn_render_state_get_index_from_data(render_state),
               render_state->frame_src, 0)) { goto error_1; }

            if (load_mask) {
            	Add_byte(p, KHRN_HW_INSTR_STATE_TILE_COORDS);
               ADD_BYTE(p, (uint8_t)i);
               ADD_BYTE(p, (uint8_t)j);
               /* this store is a nop; it's necessary as every tile coords must
                * have a corresponding store (and we need the extra tile coords
                * for the extra load). todo: how many cycles does it take the
                * hardware? */
               Add_byte(p, KHRN_HW_INSTR_STORE_GENERAL);
               ADD_BYTE(p, 0); /* don't store anything */
               ADD_BYTE(p, (uint8_t)(
                  (1 << 5) | /* don't clear color */
                  (1 << 6) | /* don't clear z/stencil */
                  (1 << 7))); /* don't clear mask */
               ADD_WORD(p, 0 << 3); /* not eof */
            }
         }

         if (load_mask) {
        	 Add_byte(p, KHRN_HW_INSTR_LOAD_GENERAL);
            ADD_BYTE(p, (uint8_t)(
               (4 << 0) | /* load mask */
               (0 << 4))); /* raster order */
            ADD_BYTE(p, 0); /* unused */
            if (!khrn_fmem_add_fix_image(render_state->fmem,
               &p, khrn_render_state_get_index_from_data(render_state),
               render_state->mask, 0)) { goto error_1; }
         }
#endif

         Add_byte(p, KHRN_HW_INSTR_STATE_TILE_COORDS);
         ADD_BYTE(p, (uint8_t)i);
         ADD_BYTE(p, (uint8_t)j);

         Add_byte(p, KHRN_HW_INSTR_BRANCH_SUB);
         if (!khrn_fmem_add_special(render_state->fmem,
            &p,
            KHRN_FMEM_SPECIAL_BIN_MEM,
            ((j * width_in_tiles) + i) * KHRN_HW_CL_BLOCK_SIZE_MIN)) { goto error_1; }

#ifndef __BCM2708A0__
         if (store_mask) {
        	 Add_byte(p, KHRN_HW_INSTR_STORE_GENERAL);
            ADD_BYTE(p, (uint8_t)(
               (4 << 0) | /* store mask */
               (0 << 4) | /* raster order */
               (0 << 6))); /* no downsampling */
            ADD_BYTE(p, (uint8_t)(store_frame ?
               /* STORE_SUBSAMPLE will clear buffers */
               ((1 << 5) | /* don't clear color */
               (1 << 6) | /* don't clear z/stencil */
               (1 << 7)) : /* don't clear mask */
               /* we clear buffers */
               ((!render_state->frame_clear << 5) | /* clear color */
               (0 << 6) | /* clear z/stencil */
               (!render_state->mask_clear << 7)))); /* clear mask */
            if (!khrn_fmem_add_fix_image(render_state->fmem,
               &p, khrn_render_state_get_index_from_data(render_state),
               render_state->mask,
               (eof && !store_frame) << 3)) { goto error_1; }

            if (store_frame) {
               /* need another tile coords to do another store... */
            	Add_byte(p, KHRN_HW_INSTR_STATE_TILE_COORDS);
               ADD_BYTE(p, (uint8_t)i);
               ADD_BYTE(p, (uint8_t)j);
            }
         }

         if (store_frame) {
#endif
            /* this will clear all the buffers... */
        	 Add_byte(p, eof ? KHRN_HW_INSTR_STORE_SUBSAMPLE_EOF : KHRN_HW_INSTR_STORE_SUBSAMPLE);
#ifndef __BCM2708A0__
         }
#endif

         vcos_assert(khrn_fmem_is_here(render_state->fmem, p));
      }
   }

   /*
      pass to hw...
   */

   khrn_hw_advance_enter_pos();
   transfer_buffers(render_state);
   vg_tess_nmem_enter(render_state->tess);
   data = (HW_CALLBACK_DATA_T *)khrn_fmem_queue(render_state->fmem,
      KHRN_HW_CC_FLAG_NONE,
      (KHRN_HW_CC_FLAG_T)(KHRN_HW_CC_FLAG_L2 | KHRN_HW_CC_FLAG_IC | KHRN_HW_CC_FLAG_UC | KHRN_HW_CC_FLAG_TU),
#ifdef __BCM2708A0__
      2, /* dummy frame plus real frame */
#else
      1, /* just real frame */
#endif
      /*render_state->fixer,*/
      render_state->extra_thrsws_min * 24, /* chop off unneeded thrsws from the start of every shader */
      height_in_tiles * width_in_tiles * (KHRN_HW_CL_BLOCK_SIZE_MIN + KHRN_HW_TILE_STATE_SIZE),
      16, /* want 64 rows for tessellation user shaders (todo: assuming we're doing tessellation on the qpus) */
      KHRN_HW_VPM_BLOCKS_N, /* main hw doesn't need any vpm for vg */
      KHRN_HW_TYPE_VG,
      hw_callback,
      sizeof(HW_CALLBACK_DATA_T));
   data->state = render_state->state; /* holding a reference to the state also prevents vg_be_term being called prematurely */
   if (render_state->buffers_acquired & BA_FRAME_RW) {
      data->frame = render_state->frame;
      data->frame_src = render_state->frame_src;
   } else {
      mem_release(render_state->frame);
      data->frame = MEM_INVALID_HANDLE;
      vcos_assert(render_state->frame_src == render_state->frame);
   }
   if (render_state->buffers_acquired & BA_MASK_RW) {
      data->mask = render_state->mask;
   } else {
      if (render_state->mask != MEM_INVALID_HANDLE) {
         mem_release(render_state->mask);
      }
      data->mask = MEM_INVALID_HANDLE;
   }
   data->tess = render_state->tess;
   finish(render_state); /* passed everything to hw now... */

   /* when we've finished tessellating, we're ready to kick off the hardware */
   vg_tess_wait(data->tess, khrn_hw_ready, data);

   return;
error_1:
error_0:
   vg_server_state_set_error((VG_SERVER_STATE_T *)mem_lock(render_state->state), VG_OUT_OF_MEMORY_ERROR);
   mem_unlock(render_state->state);
   abandon(render_state);
}

/******************************************************************************
global init/term and simple current state funs
******************************************************************************/

bool vg_be_init(void)
{
   uint32_t size =
      (round_up(2048, KHRN_HW_TEX_ALIGN) + 2048) + /* for colorspace convert (needs tex alignment) */
#ifdef WORKAROUND_HW2366
      sizeof(SHADER_RECORD_T) + /* for dummy shader record (needs 16 byte alignment) */
#endif
#ifdef __BCM2708A0__
      (KHRN_HW_TILE_HEIGHT * KHRN_HW_TILE_WIDTH * 4) + /* for dummy tile (needs tlb alignment) */
#endif
      get_shaders_size(); /* for shaders (need 8 byte alignment) */
   void *base = vg_tess_init(round_up(size, KHRN_HW_SYSTEM_CACHE_LINE_SIZE)), *p;
   if (!base) {
      return false;
   }
   vcos_assert(!((uintptr_t)base & (KHRN_HW_SYSTEM_CACHE_LINE_SIZE - 1)));
   khrn_hw_invalidate_dcache_range(base, size);
   base = khrn_hw_alias_direct(base);
   p = colorspace_convert_init(base);
#ifdef WORKAROUND_HW2366
   p = dummy_shader_record_init(p);
#endif
#ifdef __BCM2708A0__
   p = dummy_tile_init(p);
#endif
   p = shaders_init(p);
   vcos_assert(p == ((uint8_t *)base + size));

   vg_shader_init();
   return true;
}

/*
   this gets called when the last VG_SERVER_SHARED_STATE_T is released

   the hw "message" holds a reference to a VG_SERVER_STATE_T that is only
   released when it's safe for vg_be_term to be called
*/

void vg_be_term(void)
{
   vg_shader_term();
   vg_tess_term();
}

void vg_be_buffers_changed(MEM_HANDLE_T frame_handle, MEM_HANDLE_T mask_handle)
{
   current_render_state = NULL;

   if (frame_handle != MEM_INVALID_HANDLE) {
      uint32_t i = khrn_interlock_render_state_i(khrn_interlock_get_writer(
         &((KHRN_IMAGE_T *)mem_lock(frame_handle))->interlock));
      mem_unlock(frame_handle);
      if ((i != -1) && (khrn_render_state_get_type(i) == KHRN_RENDER_STATE_TYPE_VG)) {
         VG_BE_RENDER_STATE_T *render_state = (VG_BE_RENDER_STATE_T *)khrn_render_state_get_data(i);
         if ((render_state->frame == frame_handle) &&
            (render_state->mask == mask_handle)) {
            current_render_state = render_state;
            vg_be_state_changed(VG_BE_STATE_ELEMENT_ALL);
         }
      }
   }
}

void vg_be_state_changed(VG_BE_STATE_ELEMENT_T elements)
{
   if (current_render_state) {
      if (elements & VG_BE_STATE_ELEMENT_STATE) {
         MEM_ASSIGN(current_render_state->state, VG_GET_SERVER_STATE_HANDLE());
      }

      if (elements & (VG_BE_STATE_ELEMENT_GLYPH_USER_TO_SURFACE | VG_BE_STATE_ELEMENT_FILL_PAINT_TO_USER |
         VG_BE_STATE_ELEMENT_IMAGE_QUALITY | VG_BE_STATE_ELEMENT_SCISSORING |
         VG_BE_STATE_ELEMENT_SCISSOR_RECTS | VG_BE_STATE_ELEMENT_FILL_PAINT |
         VG_BE_STATE_ELEMENT_TILE_FILL_RGBA | VG_BE_STATE_ELEMENT_COLOR_TRANSFORM |
         VG_BE_STATE_ELEMENT_COLOR_TRANSFORM_VALUES | VG_BE_STATE_ELEMENT_BLEND_MODE |
         VG_BE_STATE_ELEMENT_MASKING)) {
         current_render_state->glyph_fill_shader = MEM_INVALID_HANDLE;
      }
      if (elements & (VG_BE_STATE_ELEMENT_GLYPH_USER_TO_SURFACE | VG_BE_STATE_ELEMENT_STROKE_PAINT_TO_USER |
         VG_BE_STATE_ELEMENT_IMAGE_QUALITY | VG_BE_STATE_ELEMENT_SCISSORING |
         VG_BE_STATE_ELEMENT_SCISSOR_RECTS | VG_BE_STATE_ELEMENT_STROKE_PAINT |
         VG_BE_STATE_ELEMENT_TILE_FILL_RGBA | VG_BE_STATE_ELEMENT_COLOR_TRANSFORM |
         VG_BE_STATE_ELEMENT_COLOR_TRANSFORM_VALUES | VG_BE_STATE_ELEMENT_BLEND_MODE |
         VG_BE_STATE_ELEMENT_MASKING)) {
         current_render_state->glyph_stroke_shader = MEM_INVALID_HANDLE;
      }
      if (elements & (VG_BE_STATE_ELEMENT_GLYPH_USER_TO_SURFACE | VG_BE_STATE_ELEMENT_SCISSORING |
         VG_BE_STATE_ELEMENT_SCISSOR_RECTS)) {
         current_render_state->glyph_fill_state_id = ++glyph_state_id;
      }
      if (elements & (VG_BE_STATE_ELEMENT_GLYPH_USER_TO_SURFACE | VG_BE_STATE_ELEMENT_STROKE_LINE_WIDTH |
         VG_BE_STATE_ELEMENT_STROKE_CAP_STYLE | VG_BE_STATE_ELEMENT_STROKE_JOIN_STYLE |
         VG_BE_STATE_ELEMENT_STROKE_MITER_LIMIT | VG_BE_STATE_ELEMENT_STROKE_DASH_PATTERN |
         VG_BE_STATE_ELEMENT_STROKE_DASH_PHASE | VG_BE_STATE_ELEMENT_STROKE_DASH_PHASE_RESET |
         VG_BE_STATE_ELEMENT_SCISSORING | VG_BE_STATE_ELEMENT_SCISSOR_RECTS)) {
         current_render_state->glyph_stroke_state_id = ++glyph_state_id;
      }
   }
}

void vg_be_flush(void)
{
   khrn_render_state_flush_all(KHRN_RENDER_STATE_TYPE_VG);
}

void vg_be_finish(void)
{
   vg_be_flush();
   khrn_hw_wait();
}

void vg_be_unlock(void)
{
}

/******************************************************************************
drawing stuff
******************************************************************************/

static const float CLIP_OUTER[4] = { -1023.0f, -1023.0f, 3070.0f, 3070.0f }; /* 16-bit coords have range -0x4000 to 0xbfff */

static bool retain_path_chunks(
   MEM_HANDLE_T *chunks_a_handle, MEM_HANDLE_T *chunks_b_handle, float *t,
   VG_PATH_T *path)
{
   if (path->flags & VG_PATH_FLAG_INTERP_TO) {
      VG_PATH_T *path_a = (VG_PATH_T *)mem_lock(path->extra.interp_to.from_a), *path_b;
      vcos_assert(path_a->flags & VG_PATH_FLAG_INTERP_FROM);
      if (!vg_path_tess_retain(path_a)) {
         mem_unlock(path->extra.interp_to.from_a);
         return false;
      }
      *chunks_a_handle = path_a->extra.n.tess;
      mem_unlock(path->extra.interp_to.from_a);
      path_b = (VG_PATH_T *)mem_lock(path->extra.interp_to.from_b);
      vcos_assert(path_b->flags & VG_PATH_FLAG_INTERP_FROM);
      if (!vg_path_tess_retain(path_b)) {
         mem_unlock(path->extra.interp_to.from_b);
         mem_unretain(*chunks_a_handle);
         return false;
      }
      *chunks_b_handle = path_b->extra.n.tess;
      mem_unlock(path->extra.interp_to.from_b);
      *t = path->extra.interp_to.t;
   } else {
      if (!vg_path_tess_retain(path)) {
         return false;
      }
      *chunks_a_handle = path->extra.n.tess;
      *chunks_b_handle = MEM_INVALID_HANDLE;
   }
   return true;
}

bool vg_be_mask(
   MEM_HANDLE_T mask_handle,
   VGMaskOperation operation,
   uint32_t dst_x, uint32_t dst_y, uint32_t width, uint32_t height, uint32_t src_x, uint32_t src_y)
{
#ifdef __BCM2708A0__
   /* shouldn't have any configs with mask buffers */
   UNREACHABLE();
   return false;
#else
   bool clear;

   if (!start()) { return false; }

   clear = ((mask_handle == MEM_INVALID_HANDLE) || (operation == VG_SET_MASK)) &&
      (dst_x == 0) && (dst_y == 0) &&
      (width == current_render_state->frame_width) && (height == current_render_state->frame_height) &&
      !(current_render_state->buffers_acquired & BA_MASK_RW);

   acquire_buffers(current_render_state, BA_MASK_RW);

   if (clear) {
      current_render_state->mask_clear = true;
      current_render_state->mask_clear_value = (operation == VG_FILL_MASK) ? 0xff : 0x00;
      if (mask_handle == MEM_INVALID_HANDLE) { return true; }
   }

   if ((mask_handle == MEM_INVALID_HANDLE) ?
      !set_md_clear_shader(current_render_state,
      (operation == VG_FILL_MASK) ? 0xff : 0x00) :
      !set_md_image_shader(current_render_state,
      mask_handle, dst_x - src_x, dst_y - src_y,
      operation)) { goto error; }
   if (!set_offset(current_render_state, 0, 0)) { goto error; }
   if (!set_clip_to_frame(current_render_state)) { goto error; }

   if (!set_cfg(current_render_state, false, false, (CVG_UPDATE_T)0)) { goto error; }
   if (!vg_tess_rect(current_render_state->fmem, dst_x, dst_y, width, height)) { goto error; }

#ifdef WORKAROUND_HW2116
   ++current_render_state->batches_n;
#endif

   return true;
error:
   abandon(current_render_state);
   return false;
#endif
}

#ifndef __BCM2708A0__
static bool is_mask_op_zero_nop(VGMaskOperation operation)
{
   switch (operation) {
   case VG_SET_MASK:       return false;
   case VG_UNION_MASK:     return true;
   case VG_INTERSECT_MASK: return false;
   case VG_SUBTRACT_MASK:  return true;
   default:                UNREACHABLE(); return false;
   }
}
#endif

/* todo: render-to-mask with operation set or intersect is slow as the second
 * pass currently has to go over the entire frame. we could probably track areas
 * of the mask buffer which we know to be 0 and avoid going over those areas in
 * the second pass... */

bool vg_be_render_to_mask(VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle, VG_PATH_T *path, /* should unlock handle if path is non-NULL */
   uint32_t paint_modes, VGMaskOperation operation,
   const uint32_t *clip_rect, MEM_HANDLE_T scissor_handle, /* should unretain scissor_handle if not MEM_INVALID_HANDLE */
   const float *clip, float scale_max, const float *stroke_clip)
{
#ifdef __BCM2708A0__
   /* shouldn't have any configs with mask buffers */
   UNREACHABLE();
   return false;
#else
   MEM_HANDLE_T chunks_a_handle, chunks_b_handle;
   float t;
   bool clear;
   bool scissor_unretained = false;

   if (path) {
      if (!retain_path_chunks(&chunks_a_handle, &chunks_b_handle, &t, path)) {
         if (scissor_handle != MEM_INVALID_HANDLE) { mem_unretain(scissor_handle); }
         mem_unlock(handle);
         return false;
      }
      mem_unlock(handle);
   } else {
      chunks_a_handle = MEM_INVALID_HANDLE;
      chunks_b_handle = MEM_INVALID_HANDLE;
   }

   if (!start()) {
      if (chunks_b_handle != MEM_INVALID_HANDLE) { mem_unretain(chunks_b_handle); }
      if (chunks_a_handle != MEM_INVALID_HANDLE) { mem_unretain(chunks_a_handle); }
      if (scissor_handle != MEM_INVALID_HANDLE) { mem_unretain(scissor_handle); }
      return false;
   }

   /* todo: could make an initial set more efficient by using fast clear and
    * then limiting the area covered by the second pass */
   clear = (chunks_a_handle == MEM_INVALID_HANDLE) &&
      !(current_render_state->buffers_acquired & BA_MASK_RW);

   acquire_buffers(current_render_state, BA_MASK_RW);

   if (clear) {
      current_render_state->mask_clear = true;
      current_render_state->mask_clear_value = (operation == VG_FILL_MASK) ? 0xff : 0x00;
      vcos_assert(chunks_b_handle == MEM_INVALID_HANDLE);
      vcos_assert(chunks_a_handle == MEM_INVALID_HANDLE);
      vcos_assert(scissor_handle == MEM_INVALID_HANDLE);
      return true;
   }

   if (chunks_a_handle == MEM_INVALID_HANDLE) {
      if (!set_md_clear_shader(current_render_state,
         (operation == VG_FILL_MASK) ? 0xff : 0x00)) { goto error; }
      if (!set_offset(current_render_state, 0, 0)) { goto error; }
      if (!set_clip_to_frame(current_render_state)) { goto error; }

      if (!set_cfg(current_render_state, false, false, (CVG_UPDATE_T)0)) { goto error; }
      if (!vg_tess_rect(current_render_state->fmem, 0, 0,
         current_render_state->frame_width, current_render_state->frame_height)) { goto error; }

#ifdef WORKAROUND_HW2116
      ++current_render_state->batches_n;
#endif
   } else {
      vcos_assert(paint_modes); /* otherwise we should be using the clear path */

      if (paint_modes & VG_FILL_PATH) {
         bool aa;
         void *fill_rep;

         vcos_assert(!scissor_unretained);
         scissor_unretained = true; /* set_md_cvg_shader will either unretain scissor_handle itself, or add it to fixer to be unretained later */
         if (!set_md_cvg_shader(current_render_state,
            scissor_handle, operation)) { goto error; }
         if (!set_offset(current_render_state, 0, 0)) { goto error; }
         if (!set_clip(current_render_state, clip_rect)) { goto error; }

         aa = convert_rendering_quality(state->rendering_quality);
         if (!set_cfg(current_render_state, aa, true, convert_fill_rule(state->fill_rule))) { goto error; }
         fill_rep = vg_tess_fill(current_render_state->tess,
            current_render_state->fmem,
            chunks_a_handle, chunks_b_handle, t,
            &state->path_user_to_surface,
            clip);
         if (!fill_rep) { goto error; }

         if (is_mask_op_zero_nop(operation)) {
            /* just go over the areas that might have coverage (like normal) */
            if (!set_cfg(current_render_state, aa, false, (CVG_UPDATE_T)0)) { goto error; }
            if (!vg_tess_fill_bbox(current_render_state->tess, current_render_state->fmem, fill_rep)) { goto error; }
         } else {
            /* need to go over the whole frame as even zero coverage will have
             * an effect. also need to make sure each pixel only gets processed
             * once; 1x aa will ensure this is the case */
            if (!set_clip_to_frame(current_render_state)) { goto error; }
            if (!set_cfg(current_render_state, false, false, (CVG_UPDATE_T)0)) { goto error; }
            if (!vg_tess_rect(current_render_state->fmem, 0, 0,
               current_render_state->frame_width, current_render_state->frame_height)) { goto error; }
         }

#ifdef WORKAROUND_HW2116
         current_render_state->batches_n += 2;
#endif
      }

      if (paint_modes & VG_STROKE_PATH) {
         bool aa;
         void *stroke_rep;

         if (scissor_unretained && (scissor_handle != MEM_INVALID_HANDLE)) { mem_retain(scissor_handle); }
         scissor_unretained = true; /* set_md_cvg_shader will either unretain scissor_handle itself, or add it to fixer to be unretained later */
         if (!set_md_cvg_shader(current_render_state,
            scissor_handle, operation)) { goto error; }
         if (!set_offset(current_render_state, 0, 0)) { goto error; }
         if (!set_clip(current_render_state, clip_rect)) { goto error; }

         aa = convert_rendering_quality(state->rendering_quality);
         if (!set_cfg(current_render_state, aa, true, CVG_UPDATE_OR)) { goto error; }
         stroke_rep = vg_tess_stroke(current_render_state->tess,
            current_render_state->fmem,
            chunks_a_handle, chunks_b_handle, t,
            state->stroke.dash_pattern, state->stroke.dash_pattern_count, state->stroke.dash_phase, state->stroke.dash_phase_reset,
            state->stroke.line_width, state->stroke.cap_style, state->stroke.join_style, state->stroke.miter_limit,
            &state->path_user_to_surface, scale_max,
            clip, CLIP_OUTER);
         if (!stroke_rep) { goto error; }

         if (is_mask_op_zero_nop(operation)) {
            /* just go over the areas that might have coverage (like normal) */
            if (!set_cfg(current_render_state, aa, false, (CVG_UPDATE_T)0)) { goto error; }
            if (!vg_tess_stroke_rep(current_render_state->tess, current_render_state->fmem, stroke_rep)) { goto error; }
         } else {
            /* need to go over the whole frame as even zero coverage will have
             * an effect. also need to make sure each pixel only gets processed
             * once; 1x aa will ensure this is the case */
            /* todo: there's some potential for optimisation in the case where
             * we're both filling and stroking with the intersect operation
             * (there's no need to go over the entire frame *twice*). not sure
             * it's worth it though. also, as mentioned above, an initial set
             * could be made faster by taking advantage of fast clear */
            if (!set_clip_to_frame(current_render_state)) { goto error; }
            if (!set_cfg(current_render_state, false, false, (CVG_UPDATE_T)0)) { goto error; }
            if (!vg_tess_rect(current_render_state->fmem, 0, 0,
               current_render_state->frame_width, current_render_state->frame_height)) { goto error; }
         }

#ifdef WORKAROUND_HW2116
         current_render_state->batches_n += 2;
#endif
      }
   }

   if (!scissor_unretained && (scissor_handle != MEM_INVALID_HANDLE)) { mem_unretain(scissor_handle); }
   if (chunks_b_handle != MEM_INVALID_HANDLE) { mem_unretain(chunks_b_handle); }
   if (chunks_a_handle != MEM_INVALID_HANDLE) { mem_unretain(chunks_a_handle); }
   return true;
error:
   abandon(current_render_state);
   if (!scissor_unretained && (scissor_handle != MEM_INVALID_HANDLE)) { mem_unretain(scissor_handle); }
   if (chunks_b_handle != MEM_INVALID_HANDLE) { mem_unretain(chunks_b_handle); }
   if (chunks_a_handle != MEM_INVALID_HANDLE) { mem_unretain(chunks_a_handle); }
   return false;
#endif
}

bool vg_be_clear(VG_SERVER_STATE_T *state,
   uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
   uint32_t *unifs, rgba;
   uint32_t *unifs_begin;

   if (!start()) { return false; }

   if ((x == 0) && (y == 0) &&
      (width == current_render_state->frame_width) && (height == current_render_state->frame_height) &&
      (!state->scissoring || ((state->scissor.rects_count == 4) &&
      (state->scissor.rects[0] == 0) && (state->scissor.rects[1] == 0) &&
      (state->scissor.rects[2] >= (int32_t)width) && (state->scissor.rects[3] >= (int32_t)height))) &&
      !(current_render_state->buffers_acquired & BA_FRAME_RW)) {
      acquire_buffers(current_render_state, BA_FRAME_RW);

      current_render_state->frame_clear = true;
      current_render_state->frame_clear_rgba = state->clear_rgba;

      return true;
   }

#ifdef __BCM2708A0__
   if (!hw2371_workaround(current_render_state)) {
      abandon(current_render_state);
      return false;
   }
#endif
   acquire_buffers(current_render_state, BA_FRAME_RW);

   unifs = khrn_fmem_junk(current_render_state->fmem, sizeof(uint32_t), 4);
   if (!unifs) { goto error; }
   unifs_begin = unifs;
   rgba = state->clear_rgba;
   if (current_render_state->frame_format & IMAGE_FORMAT_LIN) { rgba = khrn_color_rgba_s_to_lin(rgba); }
   if (current_render_state->frame_format & IMAGE_FORMAT_PRE) { rgba = khrn_color_rgba_pre(rgba); }
   *(unifs++) = rgba;
//   vcos_assert(khrn_fmem_is_here(current_render_state->fmem, unifs));
#ifdef SIMPENROSE_RECORD_OUTPUT
   record_map_mem_buffer_section(unifs_handle_offset.mh_handle, unifs_handle_offset.offset, 4, L_UNIFORMS, RECORD_BUFFER_IS_BOTH, 4);
#endif
   if (!set_shader(current_render_state, MEM_INVALID_HANDLE, fd_clear_shader, unifs_begin, 1)) { goto error; }
   if (!set_offset(current_render_state, 0, 0)) { goto error; }
   if (!set_clip_to_frame(current_render_state)) { goto error; }

   if (!set_cfg(current_render_state, false, false, (CVG_UPDATE_T)0)) { goto error; }
   if (state->scissoring) {
      uint32_t i;
      for (i = 0; i != state->scissor.rects_count; i += 4) {
         int32_t rect_x      = state->scissor.rects[i];
         int32_t rect_y      = state->scissor.rects[i + 1];
         int32_t rect_width  = state->scissor.rects[i + 2];
         int32_t rect_height = state->scissor.rects[i + 3];
         khrn_clip_rect(
            &rect_x, &rect_y, &rect_width, &rect_height,
            x, y, width, height);
         if ((rect_width != 0) && (rect_height != 0) &&
            !vg_tess_rect(current_render_state->fmem, rect_x, rect_y, rect_width, rect_height)) { goto error; }
      }
   } else if (!vg_tess_rect(current_render_state->fmem, x, y, width, height)) { goto error; }
#ifdef WORKAROUND_HW2187
   if (!vg_tess_dummy(current_render_state->fmem, current_render_state->offset)) { goto error; }
#endif

#ifdef WORKAROUND_HW2116
   ++current_render_state->batches_n;
#endif

   return true;
error:
   abandon(current_render_state);
   return false;
}

bool vg_be_draw_path(VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle, VG_PATH_T *path, /* should unlock handle */
   uint32_t paint_modes,
   const uint32_t *clip_rect, MEM_HANDLE_T scissor_handle, /* should unretain scissor_handle if not MEM_INVALID_HANDLE */
   const float *clip, float scale_max, const float *stroke_clip)
{
   MEM_HANDLE_T chunks_a_handle, chunks_b_handle;
   float t;
   bool scissor_unretained = false, masking;

   if (!retain_path_chunks(&chunks_a_handle, &chunks_b_handle, &t, path)) {
      if (scissor_handle != MEM_INVALID_HANDLE) { mem_unretain(scissor_handle); }
      mem_unlock(handle);
      return false;
   }
   mem_unlock(handle);

   if (!start()) {
      if (chunks_b_handle != MEM_INVALID_HANDLE) { mem_unretain(chunks_b_handle); }
      mem_unretain(chunks_a_handle);
      if (scissor_handle != MEM_INVALID_HANDLE) { mem_unretain(scissor_handle); }
      return false;
   }

   masking = state->masking && (current_render_state->mask != MEM_INVALID_HANDLE);

#ifdef __BCM2708A0__
   if (!hw2371_workaround(current_render_state)) {
      abandon(current_render_state);
      if (chunks_b_handle != MEM_INVALID_HANDLE) { mem_unretain(chunks_b_handle); }
      mem_unretain(chunks_a_handle);
      if (scissor_handle != MEM_INVALID_HANDLE) { mem_unretain(scissor_handle); }
      return false;
   }
#endif
   acquire_buffers(current_render_state, (BA_T)(BA_FRAME_RW | (masking ? BA_MASK_READ : 0)));

   if (paint_modes & VG_FILL_PATH) {
      VG_MAT3X3_T surface_to_paint;
      vg_mat3x3_mul(&surface_to_paint, &state->path_user_to_surface, &state->fill_paint_to_user);
      if (vg_mat3x3_affine_try_invert(&surface_to_paint)) {
         bool aa;
         void *fill_rep;

         #define SET_FD_SHADER() set_fd_shader(current_render_state, \
            scissor_handle, \
            state->image_quality, \
            state->fill_paint, state->tile_fill_rgba, &surface_to_paint, \
            MEM_INVALID_HANDLE, NULL, (VGImageMode)0, NULL, \
            state->color_transform ? state->color_transform_values : NULL, \
            state->blend_mode, true, masking, \
            NULL, NULL, NULL)

#ifndef WORKAROUND_HW2366
         vcos_assert(!scissor_unretained);
         scissor_unretained = true; /* set_fd_shader will either unretain scissor_handle itself, or add it to fixer to be unretained later */
         if (!SET_FD_SHADER()) { goto error; }
#endif
         if (!set_offset(current_render_state, 0, 0)) { goto error; }
         if (!set_clip(current_render_state, clip_rect)) { goto error; }

#ifdef WORKAROUND_HW2366
         if (!set_dummy_shader(current_render_state)) { goto error; }
#endif
         aa = convert_rendering_quality(state->rendering_quality);
         if (!set_cfg(current_render_state, aa, true, convert_fill_rule(state->fill_rule))) { goto error; }
         fill_rep = vg_tess_fill(current_render_state->tess,
            current_render_state->fmem,
            chunks_a_handle, chunks_b_handle, t,
            &state->path_user_to_surface,
            clip);
         if (!fill_rep) { goto error; }
#ifdef WORKAROUND_HW2187
         if (!vg_tess_dummy(current_render_state->fmem, current_render_state->offset)) { goto error; }
#endif

#ifdef WORKAROUND_HW2366
         vcos_assert(!scissor_unretained);
         scissor_unretained = true; /* set_fd_shader will either unretain scissor_handle itself, or add it to fixer to be unretained later */
         if (!SET_FD_SHADER()) { goto error; }
#endif
         if (!set_cfg(current_render_state, aa, false, convert_fill_rule(state->fill_rule) /* workaround for hw-2385 */)) { goto error; }
         if (!vg_tess_fill_bbox(current_render_state->tess, current_render_state->fmem, fill_rep)) { goto error; }
#ifdef WORKAROUND_HW2187
         if (!vg_tess_dummy(current_render_state->fmem, current_render_state->offset)) { goto error; }
#endif

#ifdef WORKAROUND_HW2116
         current_render_state->batches_n += 2;
#endif

         #undef SET_FD_SHADER
      }
   }

   if (paint_modes & VG_STROKE_PATH) {
      VG_MAT3X3_T surface_to_paint;
      vg_mat3x3_mul(&surface_to_paint, &state->path_user_to_surface, &state->stroke_paint_to_user);
      if (vg_mat3x3_affine_try_invert(&surface_to_paint)) {
         bool aa;
         void *stroke_rep;

         #define SET_FD_SHADER() set_fd_shader(current_render_state, \
            scissor_handle, \
            state->image_quality, \
            state->stroke_paint, state->tile_fill_rgba, &surface_to_paint, \
            MEM_INVALID_HANDLE, NULL, (VGImageMode)0, NULL, \
            state->color_transform ? state->color_transform_values : NULL, \
            state->blend_mode, true, masking, \
            NULL, NULL, NULL)

#ifndef WORKAROUND_HW2366
         if (scissor_unretained && (scissor_handle != MEM_INVALID_HANDLE)) { mem_retain(scissor_handle); }
         scissor_unretained = true; /* set_fd_shader will either unretain scissor_handle itself, or add it to fixer to be unretained later */
         if (!SET_FD_SHADER()) { goto error; }
#endif
         if (!set_offset(current_render_state, 0, 0)) { goto error; }
         if (!set_clip(current_render_state, clip_rect)) { goto error; }

#ifdef WORKAROUND_HW2366
         if (!set_dummy_shader(current_render_state)) { goto error; }
#endif
         aa = convert_rendering_quality(state->rendering_quality);
         if (!set_cfg(current_render_state, aa, true, CVG_UPDATE_OR)) { goto error; }
         stroke_rep = vg_tess_stroke(current_render_state->tess,
            current_render_state->fmem,
            chunks_a_handle, chunks_b_handle, t,
            state->stroke.dash_pattern, state->stroke.dash_pattern_count, state->stroke.dash_phase, state->stroke.dash_phase_reset,
            state->stroke.line_width, state->stroke.cap_style, state->stroke.join_style, state->stroke.miter_limit,
            &state->path_user_to_surface, scale_max,
            clip, CLIP_OUTER);
         if (!stroke_rep) { goto error; }
#ifdef WORKAROUND_HW2187
         if (!vg_tess_dummy(current_render_state->fmem, current_render_state->offset)) { goto error; }
#endif

#ifdef WORKAROUND_HW2366
         if (scissor_unretained && (scissor_handle != MEM_INVALID_HANDLE)) { mem_retain(scissor_handle); }
         scissor_unretained = true; /* set_fd_shader will either unretain scissor_handle itself, or add it to fixer to be unretained later */
         if (!SET_FD_SHADER()) { goto error; }
#endif
         if (!set_cfg(current_render_state, aa, false, CVG_UPDATE_OR /* workaround for hw-2385 */)) { goto error; }
         if (!vg_tess_stroke_rep(current_render_state->tess, current_render_state->fmem, stroke_rep)) { goto error; }
#ifdef WORKAROUND_HW2187
         if (!vg_tess_dummy(current_render_state->fmem, current_render_state->offset)) { goto error; }
#endif

#ifdef WORKAROUND_HW2116
         current_render_state->batches_n += 2;
#endif

         #undef SET_FD_SHADER
      }
   }

   if (!scissor_unretained && (scissor_handle != MEM_INVALID_HANDLE)) { mem_unretain(scissor_handle); }
   if (chunks_b_handle != MEM_INVALID_HANDLE) { mem_unretain(chunks_b_handle); }
   mem_unretain(chunks_a_handle);
   return true;
error:
   abandon(current_render_state);
   if (!scissor_unretained && (scissor_handle != MEM_INVALID_HANDLE)) { mem_unretain(scissor_handle); }
   if (chunks_b_handle != MEM_INVALID_HANDLE) { mem_unretain(chunks_b_handle); }
   mem_unretain(chunks_a_handle);
   return false;
}

bool vg_be_draw_image(VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
   const float *p,
   const uint32_t *clip_rect, MEM_HANDLE_T scissor_handle) /* should unretain scissor_handle if not MEM_INVALID_HANDLE */
{
   bool scissor_unretained = false, masking;
   VG_MAT3X3_T surface_to_image;
   bool projective;
   VGImageMode image_mode;
   VG_MAT3X3_T surface_to_paint;

   if (!start()) {
      if (scissor_handle != MEM_INVALID_HANDLE) { mem_unretain(scissor_handle); }
      return false;
   }

   masking = state->masking && (current_render_state->mask != MEM_INVALID_HANDLE);

#ifdef __BCM2708A0__
   if (!hw2371_workaround(current_render_state)) {
      abandon(current_render_state);
      if (scissor_handle != MEM_INVALID_HANDLE) { mem_unretain(scissor_handle); }
      return false;
   }
#endif
   acquire_buffers(current_render_state, (BA_T)(BA_FRAME_RW | (masking ? BA_MASK_READ : 0)));

   surface_to_image = state->image_user_to_surface;
   projective = !vg_mat3x3_is_affine(&surface_to_image);
   image_mode = projective ? VG_DRAW_IMAGE_NORMAL : state->image_mode;
   if (image_mode != VG_DRAW_IMAGE_NORMAL) {
      vg_mat3x3_mul(&surface_to_paint, &surface_to_image, &state->fill_paint_to_user);
   }
   if ((projective ? vg_mat3x3_try_invert(&surface_to_image) : vg_mat3x3_affine_try_invert(&surface_to_image)) &&
      ((image_mode == VG_DRAW_IMAGE_NORMAL) || vg_mat3x3_affine_try_invert(&surface_to_paint))) {
      uint32_t rect[4];
      bool aa;
      bool single_pass;
      VG_TESS_QUAD_REP_T quad_rep;

      rect[0] = x;
      rect[1] = y;
      rect[2] = width;
      rect[3] = height;

      aa = convert_rendering_quality(state->rendering_quality);
      single_pass = !aa || !(projective || vg_mat3x3_is_shear(&state->image_user_to_surface));

      #define SET_FD_SHADER() set_fd_shader(current_render_state, \
         scissor_handle, \
         state->image_quality, \
         state->fill_paint, state->tile_fill_rgba, &surface_to_paint, \
         handle, rect, image_mode, &surface_to_image, \
         state->color_transform ? state->color_transform_values : NULL, \
         state->blend_mode, !single_pass, masking, \
         NULL, NULL, NULL)

#ifndef WORKAROUND_HW2366
      vcos_assert(!scissor_unretained);
      scissor_unretained = true; /* set_fd_shader will either unretain scissor_handle itself, or add it to fixer to be unretained later */
      if (!SET_FD_SHADER()) { goto error; }
#endif
      if (!set_offset(current_render_state, 0, 0)) { goto error; }
      if (!set_clip(current_render_state, clip_rect)) { goto error; }

#ifdef WORKAROUND_HW2366
      if (!set_dummy_shader(current_render_state)) { goto error; }
#endif
      if (!set_cfg(current_render_state, aa, !single_pass, CVG_UPDATE_XOR)) { goto error; }
      if (!vg_tess_quad(current_render_state->fmem, p, CLIP_OUTER, &quad_rep)) { goto error; }
#ifdef WORKAROUND_HW2187
      if (!vg_tess_dummy(current_render_state->fmem, current_render_state->offset)) { goto error; }
#endif

#ifdef WORKAROUND_HW2366
      vcos_assert(!scissor_unretained);
      scissor_unretained = true; /* set_fd_shader will either unretain scissor_handle itself, or add it to fixer to be unretained later */
      if (!SET_FD_SHADER()) { goto error; }
#endif
      if (!single_pass) {
         if (!set_cfg(current_render_state, aa, false, CVG_UPDATE_XOR /* workaround for hw-2385 */)) { goto error; }
         if (!vg_tess_quad_rep(current_render_state->fmem, &quad_rep)) { goto error; }
#ifdef WORKAROUND_HW2187
         if (!vg_tess_dummy(current_render_state->fmem, current_render_state->offset)) { goto error; }
#endif
      }

#ifdef WORKAROUND_HW2116
      current_render_state->batches_n += 2;
#endif

      #undef SET_FD_SHADER
   }

   if (!scissor_unretained && (scissor_handle != MEM_INVALID_HANDLE)) { mem_unretain(scissor_handle); }
   return true;
error:
   abandon(current_render_state);
   if (!scissor_unretained && (scissor_handle != MEM_INVALID_HANDLE)) { mem_unretain(scissor_handle); }
   return false;
}

static bool tr_ok(void *rep, float tr_x, float tr_y, int32_t *offset_x, int32_t *offset_y)
{
   float x, y, x_max, y_max;
   vg_tess_rep_get_user(rep, &x, &y, &x_max, &y_max);
   x = tr_x - x;
   y = tr_y - y;
   if ((absf_(x) > x_max) || (absf_(y) > y_max)) {
      return false;
   }
   *offset_x = float_to_int_shift(x, 4);
   *offset_y = float_to_int_shift(y, 4);
   return true;
}

static uint64_t unpack_uint64(const uint32_t *x)
{
   return x[0] | ((uint64_t)x[1] << 32);
}

static void pack_uint64(uint32_t *x, uint64_t y)
{
   x[0] = (uint32_t)y;
   x[1] = (uint32_t)(y >> 32);
}

static bool set_glyph_fill_shader(VG_SERVER_STATE_T *state, MEM_HANDLE_T scissor_handle,
   bool masking)
{
   VG_MAT3X3_T surface_to_paint;
   if (current_render_state->glyph_fill_shader == MEM_INVALID_HANDLE) {
      vg_mat3x3_mul(&surface_to_paint, &state->glyph_user_to_surface, &state->fill_paint_to_user);
   }
   if ((current_render_state->glyph_fill_shader != MEM_INVALID_HANDLE) ||
      vg_mat3x3_affine_try_invert(&surface_to_paint)) {
      if (current_render_state->glyph_fill_shader == MEM_INVALID_HANDLE) {
         if (scissor_handle != MEM_INVALID_HANDLE) { mem_retain(scissor_handle); }
         if (!set_fd_shader(current_render_state, scissor_handle,
            state->image_quality, state->fill_paint, state->tile_fill_rgba,
            &surface_to_paint, MEM_INVALID_HANDLE, NULL, (VGImageMode)0, NULL,
            state->color_transform ? state->color_transform_values : NULL,
            state->blend_mode, true, masking, &current_render_state->glyph_fill_shader,
            &current_render_state->glyph_fill_unifs,
            &current_render_state->glyph_fill_unifs_n)) { goto error; }
      } else if (!set_shader(current_render_state, current_render_state->glyph_fill_shader,
         NULL, current_render_state->glyph_fill_unifs,
         current_render_state->glyph_fill_unifs_n)) { goto error; }
   }
   return true;
error:
   return false;
}

static bool set_glyph_stroke_shader(VG_SERVER_STATE_T *state, MEM_HANDLE_T scissor_handle,
   bool masking)
{
   VG_MAT3X3_T surface_to_paint;

   if (current_render_state->glyph_stroke_shader == MEM_INVALID_HANDLE) {
      vg_mat3x3_mul(&surface_to_paint, &state->glyph_user_to_surface, &state->stroke_paint_to_user);
   }
   if ((current_render_state->glyph_stroke_shader != MEM_INVALID_HANDLE) ||
      vg_mat3x3_affine_try_invert(&surface_to_paint)) {

      if (current_render_state->glyph_stroke_shader == MEM_INVALID_HANDLE) {
         if (scissor_handle != MEM_INVALID_HANDLE) { mem_retain(scissor_handle); }
         if (!set_fd_shader(current_render_state, scissor_handle, state->image_quality,
            state->stroke_paint, state->tile_fill_rgba, &surface_to_paint,
            MEM_INVALID_HANDLE, NULL, (VGImageMode)0, NULL,
            state->color_transform ? state->color_transform_values : NULL,
            state->blend_mode, true, masking, &current_render_state->glyph_stroke_shader,
            &current_render_state->glyph_stroke_unifs,
            &current_render_state->glyph_stroke_unifs_n)) { goto error; }
      } else if (!set_shader(current_render_state, current_render_state->glyph_stroke_shader,
         NULL, current_render_state->glyph_stroke_unifs,
         current_render_state->glyph_stroke_unifs_n)) { goto error; }
   }
   return true;
error:
   return false;
}

static bool glyph_fill_tess (VG_SERVER_STATE_T *state, VG_PATH_T *path, float tr_x, float tr_y,
   MEM_HANDLE_T scissor_handle, const float *clip, MEM_HANDLE_T *chunks_handle, bool masking)
{
   bool aa;
   int32_t offset_x, offset_y;

   aa = convert_rendering_quality(state->rendering_quality);
   if (!set_cfg(current_render_state, aa, true, convert_fill_rule(state->fill_rule))) { goto error; }
   if (path->extra.n.u.glyph.fill_rep && (unpack_uint64(path->extra.n.u.glyph.fill_state_id) == current_render_state->glyph_fill_state_id) &&
      tr_ok(path->extra.n.u.glyph.fill_rep, tr_x, tr_y, &offset_x, &offset_y)) {
      if (!set_offset(current_render_state, offset_x, offset_y)) { goto error; }
      if (!vg_tess_fill_rep(current_render_state->tess, current_render_state->fmem, path->extra.n.u.glyph.fill_rep)) { goto error; }
   } else {
      VG_MAT3X3_T user_to_surface;
      float x_max, y_max, midway_clip[4];
      if (!set_offset(current_render_state, 0, 0) || !vg_path_tess_retain(path)) { goto error; }
      *chunks_handle = path->extra.n.tess;
      user_to_surface = state->glyph_user_to_surface;
      vg_mat3x3_premul_translate(&user_to_surface, tr_x, tr_y);
      x_max = _minf(clip[0] - CLIP_OUTER[0], CLIP_OUTER[2] - clip[2]) * 0.5f;
      y_max = _minf(clip[1] - CLIP_OUTER[1], CLIP_OUTER[3] - clip[3]) * 0.5f;
      midway_clip[0] = clip[0] - x_max;
      midway_clip[1] = clip[1] - y_max;
      midway_clip[2] = clip[2] + x_max;
      midway_clip[3] = clip[3] + y_max;
      path->extra.n.u.glyph.fill_rep = vg_tess_fill(current_render_state->tess,
         current_render_state->fmem,
         *chunks_handle, MEM_INVALID_HANDLE, 0.0f,
         &user_to_surface,
         midway_clip);
      if (!path->extra.n.u.glyph.fill_rep) { goto error; }
      vg_tess_rep_set_user(path->extra.n.u.glyph.fill_rep, tr_x, tr_y, x_max, y_max);
      pack_uint64(path->extra.n.u.glyph.fill_state_id, current_render_state->glyph_fill_state_id);
   }
   vcos_assert(path->extra.n.u.glyph.fill_rep);
#ifdef WORKAROUND_HW2187
   if (!vg_tess_dummy(current_render_state->fmem, current_render_state->offset)) { goto error; }
#endif

#ifdef WORKAROUND_HW2366
   if (!set_glyph_fill_shader(state, scissor_handle, masking)) { goto error; }
#endif
   if (!set_cfg(current_render_state, aa, false, convert_fill_rule(state->fill_rule) /* workaround for hw-2385 */)) { goto error; }
   if (!vg_tess_fill_bbox(current_render_state->tess, current_render_state->fmem, path->extra.n.u.glyph.fill_rep)) { goto error; }
#ifdef WORKAROUND_HW2187
   if (!vg_tess_dummy(current_render_state->fmem, current_render_state->offset)) { goto error; }
#endif
   return true;
error:
   return false;
}

static bool glyph_stroke_tess (VG_SERVER_STATE_T *state, VG_PATH_T *path, float tr_x, float tr_y,
   MEM_HANDLE_T scissor_handle, const float *clip, float scale_max,
   MEM_HANDLE_T *chunks_handle, bool masking)
{
   bool aa;
   int32_t offset_x, offset_y;

   aa = convert_rendering_quality(state->rendering_quality);
   if (!set_cfg(current_render_state, aa, true, CVG_UPDATE_OR)) { goto error; }
   if (path->extra.n.u.glyph.stroke_rep && (unpack_uint64(path->extra.n.u.glyph.stroke_state_id) == current_render_state->glyph_stroke_state_id) &&
      tr_ok(path->extra.n.u.glyph.stroke_rep, tr_x, tr_y, &offset_x, &offset_y)) {
      if (!set_offset(current_render_state, offset_x, offset_y)) { goto error; }
      if (!vg_tess_stroke_rep(current_render_state->tess, current_render_state->fmem, path->extra.n.u.glyph.stroke_rep)) { goto error; }
   } else {
      VG_MAT3X3_T user_to_surface;
      float x_max, y_max, midway_clip_inner[4], midway_clip_outer[4];
      if (!set_offset(current_render_state, 0, 0) || !vg_path_tess_retain(path)) { goto error; }
      *chunks_handle = path->extra.n.tess;
      user_to_surface = state->glyph_user_to_surface;
      vg_mat3x3_premul_translate(&user_to_surface, tr_x, tr_y);
      x_max = _minf(clip[0] - CLIP_OUTER[0], CLIP_OUTER[2] - clip[2]) * 0.5f;
      y_max = _minf(clip[1] - CLIP_OUTER[1], CLIP_OUTER[3] - clip[3]) * 0.5f;
      midway_clip_inner[0] = clip[0] - x_max;
      midway_clip_inner[1] = clip[1] - y_max;
      midway_clip_inner[2] = clip[2] + x_max;
      midway_clip_inner[3] = clip[3] + y_max;
      midway_clip_outer[0] = CLIP_OUTER[0] + x_max;
      midway_clip_outer[1] = CLIP_OUTER[1] + y_max;
      midway_clip_outer[2] = CLIP_OUTER[2] - x_max;
      midway_clip_outer[3] = CLIP_OUTER[3] - y_max;
      path->extra.n.u.glyph.stroke_rep = vg_tess_stroke(current_render_state->tess,
         current_render_state->fmem,
         *chunks_handle, MEM_INVALID_HANDLE, 0.0f,
         state->stroke.dash_pattern, state->stroke.dash_pattern_count, state->stroke.dash_phase, state->stroke.dash_phase_reset,
         state->stroke.line_width, state->stroke.cap_style, state->stroke.join_style, state->stroke.miter_limit,
         &user_to_surface, scale_max,
         midway_clip_inner, midway_clip_outer);
      if (!path->extra.n.u.glyph.stroke_rep) { goto error; }
      vg_tess_rep_set_user(path->extra.n.u.glyph.stroke_rep, tr_x, tr_y, x_max, y_max);
      pack_uint64(path->extra.n.u.glyph.stroke_state_id, current_render_state->glyph_stroke_state_id);
   }
   vcos_assert(path->extra.n.u.glyph.stroke_rep);
#ifdef WORKAROUND_HW2187
      if (!vg_tess_dummy(current_render_state->fmem, current_render_state->offset)) { goto error; }
#endif
#ifdef WORKAROUND_HW2366
      if (!set_glyph_stroke_shader(state, scissor_handle, masking)) { goto error; }
#endif
   if (!set_cfg(current_render_state, aa, false, CVG_UPDATE_OR /* workaround for hw-2385 */)) { goto error; }
   if (!vg_tess_stroke_rep(current_render_state->tess, current_render_state->fmem, path->extra.n.u.glyph.stroke_rep)) { goto error; }
#ifdef WORKAROUND_HW2187
      if (!vg_tess_dummy(current_render_state->fmem, current_render_state->offset)) { goto error; }
#endif
   return true;
error:
   return false;
}

static bool vg_be_draw_glyph_path_monolithic(VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle, VG_PATH_T *path, /* should unlock handle */
   uint32_t paint_modes,
   float tr_x, float tr_y,
   const uint32_t *clip_rect, MEM_HANDLE_T scissor_handle,
   const float *clip, float scale_max)
{
   MEM_HANDLE_T chunks_handle_a = MEM_INVALID_HANDLE;
   MEM_HANDLE_T chunks_handle_b = MEM_INVALID_HANDLE;
   bool masking;

   if (((path->flags & VG_PATH_FLAG_INTERP_TO) && !vg_path_bake_interp_to(path)) ||
      ((path->flags & VG_PATH_FLAG_INTERP_FROM) && !vg_path_bake_interps_from(path))) {
      mem_unlock(handle);
      return false;
   }

   if (!start()) {
      mem_unlock(handle);
      return false;
   }

   masking = state->masking && (current_render_state->mask != MEM_INVALID_HANDLE);

#ifdef __BCM2708A0__
   if (!hw2371_workaround(current_render_state)) {
      abandon(current_render_state);
      mem_unlock(handle);
      return false;
   }
#endif
   acquire_buffers(current_render_state, (BA_T)(BA_FRAME_RW | (masking ? BA_MASK_READ : 0)));
   vg_mat3x3_affine_transform_t(&state->glyph_user_to_surface, &tr_x, &tr_y); /* convert translation to surface-space */

   if (paint_modes & (VG_FILL_PATH)) {
#ifndef WORKAROUND_HW2366
      if (!set_glyph_fill_shader(state, scissor_handle, masking)) { goto error; }
#endif
      if (!set_clip(current_render_state, clip_rect)) { goto error; }
#ifdef WORKAROUND_HW2366
      if (!set_dummy_shader(current_render_state)) { goto error; }
#endif
      if (!glyph_fill_tess(state, path, tr_x, tr_y, scissor_handle, clip, &chunks_handle_a, masking)) {
         goto error;
      }
#ifdef WORKAROUND_HW2116
      current_render_state->batches_n += 2;
#endif
   }
   if (paint_modes & (VG_STROKE_PATH)) {
#ifndef WORKAROUND_HW2366
      if (!set_glyph_stroke_shader(state, scissor_handle, masking)) { goto error; }
#endif
      if (!set_clip(current_render_state, clip_rect)) { goto error; }
#ifdef WORKAROUND_HW2366
      if (!set_dummy_shader(current_render_state)) { goto error; }
#endif
      if (!glyph_stroke_tess(state, path, tr_x, tr_y, scissor_handle, clip, scale_max, &chunks_handle_b, masking)) {
         goto error;
      }
#ifdef WORKAROUND_HW2116
      current_render_state->batches_n += 2;
#endif
   }

   if (chunks_handle_a != MEM_INVALID_HANDLE) { mem_unretain(chunks_handle_a); }
   if (chunks_handle_b != MEM_INVALID_HANDLE) { mem_unretain(chunks_handle_b); }
   mem_unlock(handle);
   return true;
error:
   abandon(current_render_state);
   if (chunks_handle_a != MEM_INVALID_HANDLE) { mem_unretain(chunks_handle_a); }
   if (chunks_handle_b != MEM_INVALID_HANDLE) { mem_unretain(chunks_handle_b); }
   mem_unlock(handle);
   return false;
}

bool vg_be_draw_glyph_path_init(VG_SERVER_STATE_T *state,
   uint32_t paint_modes,
   const uint32_t *clip_rect, MEM_HANDLE_T scissor_handle)
{
#ifndef __BCM2708A0__
   bool masking;
#ifdef WORKAROUND_HW2366
   vcos_assert (0);
#endif
   vcos_assert(paint_modes);
   if ((paint_modes & (VG_FILL_PATH | VG_STROKE_PATH)) == (VG_FILL_PATH | VG_STROKE_PATH)) {
      return true;
   }

   if (!start()) {
      return false;
   }

   masking = state->masking && (current_render_state->mask != MEM_INVALID_HANDLE);
   acquire_buffers(current_render_state, (BA_T)(BA_FRAME_RW | (masking ? BA_MASK_READ : 0)));

   if (paint_modes & VG_FILL_PATH) {
      if (!set_glyph_fill_shader(state, scissor_handle, masking)) { goto error; }
      if (!set_clip(current_render_state, clip_rect)) { goto error; }
   }  else  {
      if (!set_glyph_stroke_shader(state, scissor_handle, masking)) { goto error; }
      if (!set_clip(current_render_state, clip_rect)) { goto error; }
   }
   return true;
error:
   abandon(current_render_state);
   return false;
#else
   /* __BCM2708A0__ */
   return true;
#endif
}

bool vg_be_draw_glyph_path(VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle, VG_PATH_T *path, /* should unlock handle */
   uint32_t paint_modes,
   float tr_x, float tr_y,
   const uint32_t *clip_rect, MEM_HANDLE_T scissor_handle,
   const float *clip, float scale_max)
{
#ifndef __BCM2708A0__
   MEM_HANDLE_T chunks_handle = MEM_INVALID_HANDLE;
#ifdef WORKAROUND_HW2366
   vcos_assert(0);
#endif

  if ((paint_modes & (VG_FILL_PATH | VG_STROKE_PATH)) == (VG_FILL_PATH | VG_STROKE_PATH)) {

      /* todo - optimise the (assumed rare) case of filling then stroking a set of glyphs */
      return vg_be_draw_glyph_path_monolithic(state, handle, path, /* should unlock handle */
         paint_modes, tr_x, tr_y, clip_rect, scissor_handle, clip, scale_max);
   }

   if (((path->flags & VG_PATH_FLAG_INTERP_TO) && !vg_path_bake_interp_to(path)) ||
      ((path->flags & VG_PATH_FLAG_INTERP_FROM) && !vg_path_bake_interps_from(path))) {
      mem_unlock(handle);
      return false;
   }

   /* we do this again here to make sure the hw-2116 workaround batch count
    * check happens frequently enough. todo: not nice */
   if (!start()) {
      mem_unlock(handle);
      return false;
   }
   vg_mat3x3_affine_transform_t(&state->glyph_user_to_surface, &tr_x, &tr_y); /* convert translation to surface-space */

   if (paint_modes & VG_FILL_PATH) {
      if (!glyph_fill_tess(state, path, tr_x, tr_y, scissor_handle, clip, &chunks_handle, false)) {
         goto error;
      }
   } else {
      /* VG_STROKE_PATH */
      if (!glyph_stroke_tess(state, path, tr_x, tr_y, scissor_handle, clip, scale_max, &chunks_handle, false)) {
         goto error;
      }
   }
#ifdef WORKAROUND_HW2116
   current_render_state->batches_n += 2;
#endif
   if (chunks_handle != MEM_INVALID_HANDLE) { mem_unretain(chunks_handle); }
   mem_unlock(handle);
   return true;
error:
   abandon(current_render_state);
   if (chunks_handle != MEM_INVALID_HANDLE) { mem_unretain(chunks_handle); }
   mem_unlock(handle);
   return false;

#else
   /* __BCM2708A0__ */
   return vg_be_draw_glyph_path_monolithic(state,
      handle, path, /* should unlock handle */
      paint_modes,
      tr_x, tr_y,
      clip_rect, scissor_handle,
      clip, scale_max);
#endif
}

bool vg_be_draw_glyph_image(VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
   float tr_x, float tr_y,
   const float *p,
   const uint32_t *clip_rect, MEM_HANDLE_T scissor_handle)
{
   bool masking;
   VG_MAT3X3_T surface_to_image;
   VGImageMode image_mode;
   VG_MAT3X3_T surface_to_paint;

   /*
      todo: we don't really want to call set_fd_shader every time...
   */

   if (!start()) { return false; }

   masking = state->masking && (current_render_state->mask != MEM_INVALID_HANDLE);

#ifdef __BCM2708A0__
   if (!hw2371_workaround(current_render_state)) {
      abandon(current_render_state);
      return false;
   }
#endif
   acquire_buffers(current_render_state, (BA_T)(BA_FRAME_RW | (masking ? BA_MASK_READ : 0)));

   surface_to_image = state->glyph_user_to_surface;
   image_mode = state->image_mode;
   if (image_mode != VG_DRAW_IMAGE_NORMAL) {
      vg_mat3x3_mul(&surface_to_paint, &surface_to_image, &state->fill_paint_to_user);
   }
   vg_mat3x3_postmul_translate(&surface_to_image, tr_x, tr_y);
   if (vg_mat3x3_affine_try_invert(&surface_to_image) &&
      ((image_mode == VG_DRAW_IMAGE_NORMAL) || vg_mat3x3_affine_try_invert(&surface_to_paint))) {
      uint32_t rect[4];
      bool aa;
      VG_TESS_QUAD_REP_T quad_rep;

      rect[0] = x;
      rect[1] = y;
      rect[2] = width;
      rect[3] = height;

      /*
         todo: single-pass mode if possible (see vg_be_draw_image)
      */

      #define SET_FD_SHADER() set_fd_shader(current_render_state, \
         scissor_handle, \
         state->image_quality, \
         state->fill_paint, state->tile_fill_rgba, &surface_to_paint, \
         handle, rect, image_mode, &surface_to_image, \
         state->color_transform ? state->color_transform_values : NULL, \
         state->blend_mode, true, masking, \
         NULL, NULL, NULL)

#ifndef WORKAROUND_HW2366
      if (scissor_handle != MEM_INVALID_HANDLE) { mem_retain(scissor_handle); }
      if (!SET_FD_SHADER()) { goto error; }
#endif
      if (!set_offset(current_render_state, 0, 0)) { goto error; }
      if (!set_clip(current_render_state, clip_rect)) { goto error; }

#ifdef WORKAROUND_HW2366
      if (!set_dummy_shader(current_render_state)) { goto error; }
#endif
      aa = convert_rendering_quality(state->rendering_quality);
      if (!set_cfg(current_render_state, aa, true, CVG_UPDATE_XOR)) { goto error; }
      if (!vg_tess_quad(current_render_state->fmem, p, CLIP_OUTER, &quad_rep)) { goto error; }
#ifdef WORKAROUND_HW2187
      if (!vg_tess_dummy(current_render_state->fmem, current_render_state->offset)) { goto error; }
#endif

#ifdef WORKAROUND_HW2366
      if (scissor_handle != MEM_INVALID_HANDLE) { mem_retain(scissor_handle); }
      if (!SET_FD_SHADER()) { goto error; }
#endif
      if (!set_cfg(current_render_state, aa, false, CVG_UPDATE_XOR /* workaround for hw-2385 */)) { goto error; }
      if (!vg_tess_quad_rep(current_render_state->fmem, &quad_rep)) { goto error; }
#ifdef WORKAROUND_HW2187
      if (!vg_tess_dummy(current_render_state->fmem, current_render_state->offset)) { goto error; }
#endif

#ifdef WORKAROUND_HW2116
      current_render_state->batches_n += 2;
#endif

      #undef SET_FD_SHADER
   }

   return true;
error:
   abandon(current_render_state);
   return false;
}
