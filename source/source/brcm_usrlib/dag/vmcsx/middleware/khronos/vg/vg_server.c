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
#include "middleware/khronos/vg/vg_bf.h"
#include "interface/khronos/vg/vg_int_config.h"
#include "middleware/khronos/vg/vg_font.h"
#include "middleware/khronos/vg/vg_image.h"
#include "middleware/khronos/vg/vg_mask_layer.h"
#include "middleware/khronos/vg/vg_paint.h"
#include "middleware/khronos/vg/vg_path.h"
#include "middleware/khronos/vg/vg_ramp.h"
#include "middleware/khronos/vg/vg_scissor.h"
#include "middleware/khronos/vg/vg_server.h"
#include "middleware/khronos/vg/vg_stem.h"
#include "interface/khronos/vg/vg_int_util.h"
#include "interface/khronos/common/khrn_int_color.h"
#include "middleware/khronos/common/khrn_interlock.h"
#include "interface/khronos/common/khrn_int_math.h"
#include "interface/khronos/common/khrn_int_util.h"
#include "middleware/khronos/egl/egl_disp.h"
#include "middleware/khronos/egl/egl_platform.h"
#include "middleware/khronos/egl/egl_server.h"
#include "interface/khronos/include/EGL/egl.h"
#include "interface/khronos/include/EGL/eglext.h"
#include <string.h>

/* must be after EGL/eglext.h */
#if EGL_BRCM_global_image
   #include "middleware/khronos/ext/egl_brcm_global_image.h"
#endif

/* code-reviewed stuff... */
#include "middleware/khronos/vg/vg_server_cr.c"

static uint32_t init_count = 0;
static bool term_outstanding = false;

static bool first_glyph = true;

/* frame_handle/mask_handle correspond to the active buffers of the current
 * surface (kept up-to-date via vg_buffers_changed). we don't hold references to
 * them -- they are only used during regular vg calls, when they should be
 * valid */
static MEM_HANDLE_T frame_handle;
static uint32_t frame_width, frame_height;
static MEM_HANDLE_T mask_handle;

/* used when the paint is set to VG_INVALID_HANDLE: vgSetPaint(..., VG_INVALID_HANDLE) */
static MEM_HANDLE_T default_paint_handle;

static bool init(void)
{
   default_paint_handle = vg_paint_alloc();
   if (default_paint_handle == MEM_INVALID_HANDLE) {
      return false;
   }
   if (!vg_be_init()) {
      mem_release(default_paint_handle);
      return false;
   }
   vg_bf_init();
   return true;
}

static void term(void)
{
   vg_bf_term();
   vg_be_term();
   mem_release(default_paint_handle);
}

/******************************************************************************
shared state
******************************************************************************/

void vg_server_shared_state_term(void *p, uint32_t size)
{
   VG_SERVER_SHARED_STATE_T *shared_state = (VG_SERVER_SHARED_STATE_T *)p;

   UNUSED(size);

   vg_set_term(&shared_state->objects);

   if (--init_count == 0) {
      /* don't call term() directly here -- it might not be safe */
      vcos_assert(!term_outstanding);
      term_outstanding = true;
      khrn_sync_notify_master();
   }
}

MEM_HANDLE_T vg_server_shared_state_alloc(void)
{
   MEM_HANDLE_T handle;
   VG_SERVER_SHARED_STATE_T *shared_state;

   if (init_count == 0) {
      if (term_outstanding) {
         term_outstanding = false;
      } else if (!init()) {
         return MEM_INVALID_HANDLE;
      }
   }
   ++init_count;

   /*
      alloc struct
   */

   handle = MEM_ALLOC_STRUCT_EX(VG_SERVER_SHARED_STATE_T, MEM_COMPACT_DISCARD);
   if (handle == MEM_INVALID_HANDLE) {
      if (--init_count == 0) {
         term();
      }
      return MEM_INVALID_HANDLE;
   }

   /*
      init objects set
   */

   shared_state = (VG_SERVER_SHARED_STATE_T *)mem_lock(handle);
   if (!vg_set_init(&shared_state->objects, 256)) {
      mem_unlock(handle);
      mem_release(handle);
      if (--init_count == 0) {
         term();
      }
      return MEM_INVALID_HANDLE;
   }
   mem_unlock(handle);

   mem_set_term(handle, vg_server_shared_state_term);

   return handle;
}

/******************************************************************************
state
******************************************************************************/

void vg_server_state_term(void *p, uint32_t size)
{
   VG_SERVER_STATE_T *state = (VG_SERVER_STATE_T *)p;

   UNUSED(size);

   mem_release(state->shared_state);

   mem_release(state->stroke_paint);
   mem_release(state->fill_paint);

   if (state->scissor.scissor != MEM_INVALID_HANDLE) {
      mem_release(state->scissor.scissor);
   }
}

MEM_HANDLE_T vg_server_state_alloc(MEM_HANDLE_T shared_state_handle, uint64_t pid)
{
   MEM_HANDLE_T handle;
   VG_SERVER_STATE_T *state;

   /*
      alloc struct
   */

   handle = MEM_ALLOC_STRUCT_EX(VG_SERVER_STATE_T, MEM_COMPACT_DISCARD);
   if (handle == MEM_INVALID_HANDLE) {
      return MEM_INVALID_HANDLE;
   }

   state = (VG_SERVER_STATE_T *)mem_lock(handle);

   /*
      fill in rasterisation
   */

   state->glyph_origin[0] = 0.0f;
   state->glyph_origin[1] = 0.0f;

   vg_mat3x3_set_identity(&state->path_user_to_surface);
   vg_mat3x3_set_identity(&state->image_user_to_surface);
   vg_mat3x3_set_identity(&state->glyph_user_to_surface);
   vg_mat3x3_set_identity(&state->fill_paint_to_user);
   vg_mat3x3_set_identity(&state->stroke_paint_to_user);

   state->fill_rule = VG_EVEN_ODD;

   state->stroke.line_width = 1.0f;
   state->stroke.cap_style = VG_CAP_BUTT;
   state->stroke.join_style = VG_JOIN_MITER;
   state->stroke.miter_limit = 4.0f;

   state->stroke.dash_pattern_count = 0;
   state->stroke.dash_phase = 0.0f;
   state->stroke.dash_phase_reset = false;

   state->image_quality = VG_IMAGE_QUALITY_FASTER;
   state->image_mode = VG_DRAW_IMAGE_NORMAL;

   state->scissoring = false;
   state->scissor.rects_count = 0;
   state->scissor.scissor = MEM_INVALID_HANDLE;

   state->rendering_quality = VG_RENDERING_QUALITY_BETTER;

   mem_acquire(default_paint_handle);
   state->fill_paint = default_paint_handle;
   mem_acquire(default_paint_handle);
   state->stroke_paint = default_paint_handle;
   state->tile_fill_rgba = 0x00000000;
   state->clear_rgba = 0x00000000;

   state->color_transform = false;
   state->color_transform_values[0] = 1.0f;
   state->color_transform_values[1] = 1.0f;
   state->color_transform_values[2] = 1.0f;
   state->color_transform_values[3] = 1.0f;
   state->color_transform_values[4] = 0.0f;
   state->color_transform_values[5] = 0.0f;
   state->color_transform_values[6] = 0.0f;
   state->color_transform_values[7] = 0.0f;

   state->blend_mode = VG_BLEND_SRC_OVER;
   state->masking = false;

   /*
      fill in image filters
   */

   state->filter_format_linear = false;
   state->filter_format_pre = false;
   state->filter_channel_mask = VG_RED | VG_GREEN | VG_BLUE | VG_ALPHA;

   /*
      fill in misc
   */

   state->error = VG_NO_ERROR;

   /*
      fill in pid
   */

   state->pid = pid;

   /*
      fill in shared state
   */

   mem_acquire(shared_state_handle);
   state->shared_state = shared_state_handle;

   mem_unlock(handle);

   mem_set_term(handle, vg_server_state_term);

   return handle;
}

/******************************************************************************
helpers
******************************************************************************/

static uint32_t get_transform_coords_count(const uint8_t *segments, uint32_t segments_count)
{
   uint32_t coords_count = 0;
   for (; segments_count != 0; ++segments, --segments_count) {
      uint32_t segment = *segments & VG_SEGMENT_MASK;
      if ((segment == VG_HLINE_TO) || (segment == VG_VLINE_TO)) { segment = VG_LINE_TO; }
      coords_count += get_segment_coords_count(segment);
   }
   return coords_count;
}

static KHRN_IMAGE_FORMAT_T get_external_format(VGImageFormat format)
{
   switch (format) {
   case VG_sRGBX_8888:     return RGBX_8888_RSO;
   case VG_sXRGB_8888:     return XRGB_8888_RSO;
   case VG_sBGRX_8888:     return BGRX_8888_RSO;
   case VG_sXBGR_8888:     return XBGR_8888_RSO;
   case VG_sRGBA_8888:     return RGBA_8888_RSO;
   case VG_sARGB_8888:     return ARGB_8888_RSO;
   case VG_sBGRA_8888:     return BGRA_8888_RSO;
   case VG_sABGR_8888:     return ABGR_8888_RSO;
   case VG_sRGBA_8888_PRE: return (KHRN_IMAGE_FORMAT_T)(RGBA_8888_RSO | IMAGE_FORMAT_PRE);
   case VG_sARGB_8888_PRE: return (KHRN_IMAGE_FORMAT_T)(ARGB_8888_RSO | IMAGE_FORMAT_PRE);
   case VG_sBGRA_8888_PRE: return (KHRN_IMAGE_FORMAT_T)(BGRA_8888_RSO | IMAGE_FORMAT_PRE);
   case VG_sABGR_8888_PRE: return (KHRN_IMAGE_FORMAT_T)(ABGR_8888_RSO | IMAGE_FORMAT_PRE);
   case VG_sRGB_565:       return RGB_565_RSO;
   case VG_sBGR_565:       return BGR_565_RSO;
   case VG_sRGBA_5551:     return RGBA_5551_RSO;
   case VG_sARGB_1555:     return ARGB_1555_RSO;
   case VG_sBGRA_5551:     return BGRA_5551_RSO;
   case VG_sABGR_1555:     return ABGR_1555_RSO;
   case VG_sRGBA_4444:     return RGBA_4444_RSO;
   case VG_sARGB_4444:     return ARGB_4444_RSO;
   case VG_sBGRA_4444:     return BGRA_4444_RSO;
   case VG_sABGR_4444:     return ABGR_4444_RSO;
   case VG_sL_8:           return L_8_RSO;
   case VG_lRGBX_8888:     return (KHRN_IMAGE_FORMAT_T)(RGBX_8888_RSO | IMAGE_FORMAT_LIN);
   case VG_lXRGB_8888:     return (KHRN_IMAGE_FORMAT_T)(XRGB_8888_RSO | IMAGE_FORMAT_LIN);
   case VG_lBGRX_8888:     return (KHRN_IMAGE_FORMAT_T)(BGRX_8888_RSO | IMAGE_FORMAT_LIN);
   case VG_lXBGR_8888:     return (KHRN_IMAGE_FORMAT_T)(XBGR_8888_RSO | IMAGE_FORMAT_LIN);
   case VG_lRGBA_8888:     return (KHRN_IMAGE_FORMAT_T)(RGBA_8888_RSO | IMAGE_FORMAT_LIN);
   case VG_lARGB_8888:     return (KHRN_IMAGE_FORMAT_T)(ARGB_8888_RSO | IMAGE_FORMAT_LIN);
   case VG_lBGRA_8888:     return (KHRN_IMAGE_FORMAT_T)(BGRA_8888_RSO | IMAGE_FORMAT_LIN);
   case VG_lABGR_8888:     return (KHRN_IMAGE_FORMAT_T)(ABGR_8888_RSO | IMAGE_FORMAT_LIN);
   case VG_lRGBA_8888_PRE: return (KHRN_IMAGE_FORMAT_T)(RGBA_8888_RSO | IMAGE_FORMAT_LIN | IMAGE_FORMAT_PRE);
   case VG_lARGB_8888_PRE: return (KHRN_IMAGE_FORMAT_T)(ARGB_8888_RSO | IMAGE_FORMAT_LIN | IMAGE_FORMAT_PRE);
   case VG_lBGRA_8888_PRE: return (KHRN_IMAGE_FORMAT_T)(BGRA_8888_RSO | IMAGE_FORMAT_LIN | IMAGE_FORMAT_PRE);
   case VG_lABGR_8888_PRE: return (KHRN_IMAGE_FORMAT_T)(ABGR_8888_RSO | IMAGE_FORMAT_LIN | IMAGE_FORMAT_PRE);
   case VG_lL_8:           return (KHRN_IMAGE_FORMAT_T)(L_8_RSO | IMAGE_FORMAT_LIN);
   case VG_A_8:            return (KHRN_IMAGE_FORMAT_T)(A_8_RSO | IMAGE_FORMAT_LIN);
   case VG_BW_1:           return (KHRN_IMAGE_FORMAT_T)(L_1_RSO | IMAGE_FORMAT_LIN);
   case VG_A_4:            return (KHRN_IMAGE_FORMAT_T)(A_4_RSO | IMAGE_FORMAT_LIN);
   case VG_A_1:            return (KHRN_IMAGE_FORMAT_T)(A_1_RSO | IMAGE_FORMAT_LIN);
   default:                UNREACHABLE(); return (KHRN_IMAGE_FORMAT_T)0;
   }
}

static INLINE VGHandle vg_handle_from_mem_handle(MEM_HANDLE_T handle)
{
   return (VGHandle)handle;
}

static INLINE MEM_HANDLE_T vg_handle_to_mem_handle(VGHandle vg_handle)
{
   return (MEM_HANDLE_T)(vg_handle + (MEM_INVALID_HANDLE - VG_INVALID_HANDLE));
}

#ifndef NDEBUG

static bool is_object_ok(
   VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle,
   OBJECT_TYPE_T allowed_types)
{
   return contains_object(state, handle) && (allowed_types & get_object_type(handle));
}

#endif

static void get_transformed_bounds(
   float *transformed_bounds,
   const float *bounds,
   const VG_MAT3X3_T *a)
{
   float x_min_x, x_min_y, x_max_x, x_max_y;
   float y_min_x, y_min_y, y_max_x, y_max_y;
   float x_a, x_b, x_c, x_d;
   float y_a, y_b, y_c, y_d;

   vcos_assert(!nan_gt_(bounds[0], bounds[2]));
   vcos_assert(!nan_gt_(bounds[1], bounds[3]));
#ifndef NDEBUG
   vcos_verify(vg_mat3x3_is_affine(a)); /* why vcos_verify? see vg_mat3x3_is_affine... */
#endif

   x_min_x = a->m[0][0] * bounds[0];
   x_min_y = a->m[0][1] * bounds[1];
   x_max_x = a->m[0][0] * bounds[2];
   x_max_y = a->m[0][1] * bounds[3];
   y_min_x = a->m[1][0] * bounds[0];
   y_min_y = a->m[1][1] * bounds[1];
   y_max_x = a->m[1][0] * bounds[2];
   y_max_y = a->m[1][1] * bounds[3];

   x_a = x_min_x + x_min_y;
   x_b = x_max_x + x_min_y;
   x_c = x_max_x + x_max_y;
   x_d = x_min_x + x_max_y;
   y_a = y_min_x + y_min_y;
   y_b = y_max_x + y_min_y;
   y_c = y_max_x + y_max_y;
   y_d = y_min_x + y_max_y;

   transformed_bounds[0] = _minf(_minf(_minf(x_a, x_b), x_c), x_d) + a->m[0][2];
   transformed_bounds[1] = _minf(_minf(_minf(y_a, y_b), y_c), y_d) + a->m[1][2];
   transformed_bounds[2] = _maxf(_maxf(_maxf(x_a, x_b), x_c), x_d) + a->m[0][2];
   transformed_bounds[3] = _maxf(_maxf(_maxf(y_a, y_b), y_c), y_d) + a->m[1][2];
}

static INLINE void get_offset_bounds(
   float *transformed_bounds,
   const float *bounds,
   float tr_x, float tr_y)
{
   transformed_bounds[0] = bounds[0] + tr_x;
   transformed_bounds[1] = bounds[1] + tr_y;
   transformed_bounds[2] = bounds[2] + tr_x;
   transformed_bounds[3] = bounds[3] + tr_y;
}

static bool prepare_scissor(VG_SERVER_STATE_SCISSOR_T *scissor,
   uint32_t *clip_rect, MEM_HANDLE_T *scissor_handle, float *clip)
{
   if (!vg_be_prepare_scissor(scissor, clip_rect, scissor_handle, frame_width, frame_height)) {
      return false;
   }
   clip[0] = (float)clip_rect[0];
   clip[1] = (float)clip_rect[1];
   clip[2] = (float)(clip_rect[0] + clip_rect[2]);
   clip[3] = (float)(clip_rect[1] + clip_rect[3]);
   return true;
}

static INLINE float get_scale_max(const VG_MAT3X3_T *a)
{
   float s0, s1;
   vg_mat3x3_rsq(a, NULL, &s0, &s1);
   return _maxf(s0, s1);
}

static INLINE float get_stroke_deviation_max(const VG_SERVER_STATE_STROKE_T *stroke, float scale_max)
{
   /*
      corner of square cap is width/sqrt(2) from end
      actual stroke and all other caps lie within width/2
   */

   float x = (stroke->cap_style == VG_CAP_SQUARE) ? 0.70710678f : 0.5f;

   /*
      miter joins can jut out by a factor of miter limit
      round/bevel joins don't jut out
   */

   if (stroke->join_style == VG_JOIN_MITER) {
      x = _maxf(x, stroke->miter_limit * 0.5f);
   }

   return x * stroke->line_width * scale_max;
}

static INLINE void get_stroke_clip(
   float *stroke_clip,
   const float *clip,
   const VG_SERVER_STATE_STROKE_T *stroke, float scale_max)
{
   float stroke_deviation_max = get_stroke_deviation_max(stroke, scale_max);
   stroke_clip[0] = clip[0] - stroke_deviation_max;
   stroke_clip[1] = clip[1] - stroke_deviation_max;
   stroke_clip[2] = clip[2] + stroke_deviation_max;
   stroke_clip[3] = clip[3] + stroke_deviation_max;
}

static INLINE bool bounds_intersect(const float *a, const float *b)
{
   return (a[0] < b[2]) && (a[1] < b[3]) &&
          (b[0] < a[2]) && (b[1] < a[3]);
}

typedef enum {
   STATE_ELEMENT_GLYPH_ORIGIN            = VG_BE_STATE_ELEMENT_GLYPH_ORIGIN,
   STATE_ELEMENT_PATH_USER_TO_SURFACE    = VG_BE_STATE_ELEMENT_PATH_USER_TO_SURFACE,
   STATE_ELEMENT_IMAGE_USER_TO_SURFACE   = VG_BE_STATE_ELEMENT_IMAGE_USER_TO_SURFACE,
   STATE_ELEMENT_GLYPH_USER_TO_SURFACE   = VG_BE_STATE_ELEMENT_GLYPH_USER_TO_SURFACE,
   STATE_ELEMENT_FILL_PAINT_TO_USER      = VG_BE_STATE_ELEMENT_FILL_PAINT_TO_USER,
   STATE_ELEMENT_STROKE_PAINT_TO_USER    = VG_BE_STATE_ELEMENT_STROKE_PAINT_TO_USER,
   STATE_ELEMENT_FILL_RULE               = VG_BE_STATE_ELEMENT_FILL_RULE,
   STATE_ELEMENT_STROKE_LINE_WIDTH       = VG_BE_STATE_ELEMENT_STROKE_LINE_WIDTH,
   STATE_ELEMENT_STROKE_CAP_STYLE        = VG_BE_STATE_ELEMENT_STROKE_CAP_STYLE,
   STATE_ELEMENT_STROKE_JOIN_STYLE       = VG_BE_STATE_ELEMENT_STROKE_JOIN_STYLE,
   STATE_ELEMENT_STROKE_MITER_LIMIT      = VG_BE_STATE_ELEMENT_STROKE_MITER_LIMIT,
   STATE_ELEMENT_STROKE_DASH_PATTERN     = VG_BE_STATE_ELEMENT_STROKE_DASH_PATTERN,
   STATE_ELEMENT_STROKE_DASH_PHASE       = VG_BE_STATE_ELEMENT_STROKE_DASH_PHASE,
   STATE_ELEMENT_STROKE_DASH_PHASE_RESET = VG_BE_STATE_ELEMENT_STROKE_DASH_PHASE_RESET,
   STATE_ELEMENT_IMAGE_QUALITY           = VG_BE_STATE_ELEMENT_IMAGE_QUALITY,
   STATE_ELEMENT_IMAGE_MODE              = VG_BE_STATE_ELEMENT_IMAGE_MODE,
   STATE_ELEMENT_SCISSORING              = VG_BE_STATE_ELEMENT_SCISSORING,
   STATE_ELEMENT_SCISSOR_RECTS           = VG_BE_STATE_ELEMENT_SCISSOR_RECTS,
   STATE_ELEMENT_RENDERING_QUALITY       = VG_BE_STATE_ELEMENT_RENDERING_QUALITY,
   STATE_ELEMENT_FILL_PAINT              = VG_BE_STATE_ELEMENT_FILL_PAINT,
   STATE_ELEMENT_STROKE_PAINT            = VG_BE_STATE_ELEMENT_STROKE_PAINT,
   STATE_ELEMENT_TILE_FILL_RGBA          = VG_BE_STATE_ELEMENT_TILE_FILL_RGBA,
   STATE_ELEMENT_CLEAR_RGBA              = VG_BE_STATE_ELEMENT_CLEAR_RGBA,
   STATE_ELEMENT_COLOR_TRANSFORM         = VG_BE_STATE_ELEMENT_COLOR_TRANSFORM,
   STATE_ELEMENT_COLOR_TRANSFORM_VALUES  = VG_BE_STATE_ELEMENT_COLOR_TRANSFORM_VALUES,
   STATE_ELEMENT_BLEND_MODE              = VG_BE_STATE_ELEMENT_BLEND_MODE,
   STATE_ELEMENT_MASKING                 = VG_BE_STATE_ELEMENT_MASKING,
   STATE_ELEMENT_FILTER_FORMAT_LINEAR    = VG_BE_STATE_ELEMENT_FILTER_FORMAT_LINEAR,
   STATE_ELEMENT_FILTER_FORMAT_PRE       = VG_BE_STATE_ELEMENT_FILTER_FORMAT_PRE,
   STATE_ELEMENT_FILTER_CHANNEL_MASK     = VG_BE_STATE_ELEMENT_FILTER_CHANNEL_MASK,
   STATE_ELEMENT_PIXEL_LAYOUT            = VG_BE_STATE_ELEMENT_PIXEL_LAYOUT
} STATE_ELEMENT_T;

static void state_changed(VG_SERVER_STATE_T *state, STATE_ELEMENT_T elements)
{
   if (elements & STATE_ELEMENT_SCISSOR_RECTS) {
      if (state->scissor.scissor != MEM_INVALID_HANDLE) {
         mem_release(state->scissor.scissor);
         state->scissor.scissor = MEM_INVALID_HANDLE;
      }
   }

   vg_be_state_changed((VG_BE_STATE_ELEMENT_T)elements);
}

typedef enum {
   PATH_ELEMENT_CAPS = VG_PATH_ELEMENT_CAPS,
   PATH_ELEMENT_DATA = VG_PATH_ELEMENT_DATA
} PATH_ELEMENT_T;

static INLINE void path_changed(VG_SERVER_STATE_T *state, MEM_HANDLE_T handle, VG_PATH_T *path, PATH_ELEMENT_T elements)
{
   UNUSED(state);
   UNUSED(handle);

   vg_path_changed(path, (VG_PATH_ELEMENT_T)elements);
}

typedef enum {
   PAINT_ELEMENT_TYPE                = 1 << 0,
   PAINT_ELEMENT_RGBA                = 1 << 1,
   PAINT_ELEMENT_LINEAR_GRADIENT     = 1 << 2,
   PAINT_ELEMENT_RADIAL_GRADIENT     = 1 << 3,
   PAINT_ELEMENT_RAMP_SPREAD_MODE    = 1 << 4,
   PAINT_ELEMENT_RAMP_PRE            = 1 << 5,
   PAINT_ELEMENT_RAMP_STOPS          = 1 << 6,
   PAINT_ELEMENT_PATTERN_TILING_MODE = 1 << 7,
   PAINT_ELEMENT_PATTERN             = 1 << 8
} PAINT_ELEMENT_T;

static void paint_changed(VG_SERVER_STATE_T *state, MEM_HANDLE_T handle, VG_PAINT_T *paint, PAINT_ELEMENT_T elements)
{
   if ((elements & (PAINT_ELEMENT_RAMP_PRE | PAINT_ELEMENT_RAMP_STOPS)) && (paint->ramp != MEM_INVALID_HANDLE)) {
      vg_ramp_release(paint->ramp, paint->ramp_i);
      paint->ramp = MEM_INVALID_HANDLE;
   }

   if (state->fill_paint == handle) {
      vg_be_state_changed(VG_BE_STATE_ELEMENT_FILL_PAINT);
   }
   if (state->stroke_paint == handle) {
      vg_be_state_changed(VG_BE_STATE_ELEMENT_STROKE_PAINT);
   }
}

/******************************************************************************
internal interface
******************************************************************************/

MEM_HANDLE_T vg_get_image(VG_SERVER_STATE_T *state, VGImage vg_handle, EGLint *error)
{
   MEM_HANDLE_T handle;
   OBJECT_TYPE_T type;
   handle = vg_handle_to_mem_handle(vg_handle);
   if (!contains_object(state, handle)) {
      *error = EGL_BAD_PARAMETER;
      return MEM_INVALID_HANDLE;
   }
   type = get_object_type(handle);
   if (!((OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE) & type)) {
      if (type == OBJECT_TYPE_IMAGE_BPRINT) {
         if (!vg_image_from_bprint(handle)) {
            *error = EGL_BAD_ALLOC;
            return MEM_INVALID_HANDLE;
         }
      } else if (type == OBJECT_TYPE_CHILD_IMAGE_BPRINT) {
         if (!vg_child_image_from_bprint(handle)) {
            *error = EGL_BAD_ALLOC;
            return MEM_INVALID_HANDLE;
         }
      } else {
         *error = EGL_BAD_PARAMETER;
         return MEM_INVALID_HANDLE;
      }
   }
   return handle;
}

void vg_buffers_changed(MEM_HANDLE_T f_handle, MEM_HANDLE_T m_handle)
{
   frame_handle = f_handle;
   if (f_handle != MEM_INVALID_HANDLE) {
      KHRN_IMAGE_T *frame = (KHRN_IMAGE_T *)mem_lock(f_handle);
      frame_width = frame->width;
      frame_height = frame->height;
      mem_unlock(f_handle);
   }
   mask_handle = m_handle;

   vg_be_buffers_changed(f_handle, m_handle);
}

void vg_state_changed(void)
{
   vg_be_state_changed(VG_BE_STATE_ELEMENT_ALL);
}

MEM_HANDLE_T vg_get_frame(void)
{
   return frame_handle;
}

MEM_HANDLE_T vg_get_mask(void)
{
   return mask_handle;
}

void vg_unlock(void)
{
   VG_FORCE_UNLOCK_SERVER_STATE();
   vg_be_unlock();
}

void vg_maybe_term(void)
{
   if (term_outstanding) {
      term_outstanding = false; /* term() might indirectly call vg_maybe_term(), so clear flag first... */
      term();
   }
}

/******************************************************************************
api misc
******************************************************************************/

void vgClearError_impl(void)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   state->error = VG_NO_ERROR;
   VG_UNLOCK_SERVER_STATE();
}

void vgSetError_impl(VGErrorCode error)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   set_error(state, error);
   VG_UNLOCK_SERVER_STATE();
}

VGErrorCode vgGetError_impl(void)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   VGErrorCode error = state->error;
   state->error = VG_NO_ERROR;
   VG_UNLOCK_SERVER_STATE();
   return error;
}

void vgFlush_impl(void)
{
   vg_be_flush();
#ifdef KHRN_COMMAND_MODE_DISPLAY
   /* todo: this feels hacky. can we do it on the client side in egl's flush callback? */
   {
      EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
      MEM_HANDLE_T shandle = state->vgsurface;
      EGL_SERVER_SURFACE_T *surface = (EGL_SERVER_SURFACE_T *)mem_lock(shandle);
      if((surface->win!=EGL_PLATFORM_WIN_NONE)&&(surface->buffers==1)) {
         egl_disp_next(surface->disp_handle, surface->mh_color[0], surface->win, 0);
      }
      mem_unlock(shandle);
   }
#endif
}

VGuint vgFinish_impl(void)
{
   vg_be_finish();
#ifdef KHRN_COMMAND_MODE_DISPLAY
   {
      EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
      MEM_HANDLE_T shandle = state->vgsurface;
      EGL_SERVER_SURFACE_T *surface = (EGL_SERVER_SURFACE_T *)mem_lock(shandle);
      if((surface->win!=EGL_PLATFORM_WIN_NONE)&&(surface->buffers==1)) {
         egl_disp_next(surface->disp_handle, surface->mh_color[0], surface->win, 0);
      }
      mem_unlock(shandle);
   }
#endif
   vg_bf_wait();
   return 0;
}

VGuint vgCreateStems_impl(
   VGuint count,
   VGHandle *vg_handles)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   uint32_t i;
   for (i = 0; i != count; ++i) {
      MEM_HANDLE_T handle = vg_stem_alloc();
      if (handle == MEM_INVALID_HANDLE) {
         VG_UNLOCK_SERVER_STATE();
         return i;
      }

      if (!insert_object(state, handle)) {
         mem_release(handle);
         VG_UNLOCK_SERVER_STATE();
         return i;
      }
      mem_release(handle); /* handle now owned by objects set */

      *(vg_handles++) = vg_handle_from_mem_handle(handle);
   }

   VG_UNLOCK_SERVER_STATE();
   return count;
}

void vgDestroyStem_impl(VGHandle vg_handle)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   vcos_assert(is_object_ok(state, handle, OBJECT_TYPE_STEM));

   delete_object(state, handle);

   VG_UNLOCK_SERVER_STATE();
}

/******************************************************************************
api get/set
******************************************************************************/

void vgSetiv_impl(
   VGParamType param_type,
   VGint count,
   const VGint *values)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   switch (param_type) {
   /*
      scalar param types
   */

   #define CASE(PARAM_TYPE, DO, ELEMENT) \
      case PARAM_TYPE: \
      { \
         vcos_assert(count == 1); \
         DO; \
         state_changed(state, ELEMENT); \
         break; \
      }
   CASE(VG_FILL_RULE, state->fill_rule = (VGFillRule)values[0], STATE_ELEMENT_FILL_RULE)
   CASE(VG_IMAGE_QUALITY, state->image_quality = (VGImageQuality)values[0], STATE_ELEMENT_IMAGE_QUALITY)
   CASE(VG_RENDERING_QUALITY, state->rendering_quality = (VGRenderingQuality)values[0], STATE_ELEMENT_RENDERING_QUALITY)
   CASE(VG_BLEND_MODE, state->blend_mode = (VGBlendMode)values[0], STATE_ELEMENT_BLEND_MODE)
   CASE(VG_IMAGE_MODE, state->image_mode = (VGImageMode)values[0], STATE_ELEMENT_IMAGE_MODE)
   CASE(VG_COLOR_TRANSFORM, state->color_transform = clean_boolean(values[0]), STATE_ELEMENT_COLOR_TRANSFORM)
   CASE(VG_STROKE_CAP_STYLE, state->stroke.cap_style = (VGCapStyle)values[0], STATE_ELEMENT_STROKE_CAP_STYLE)
   CASE(VG_STROKE_JOIN_STYLE, state->stroke.join_style = (VGJoinStyle)values[0], STATE_ELEMENT_STROKE_JOIN_STYLE)
   CASE(VG_STROKE_DASH_PHASE_RESET, state->stroke.dash_phase_reset = clean_boolean(values[0]), STATE_ELEMENT_STROKE_DASH_PHASE_RESET)
   CASE(VG_MASKING, state->masking = clean_boolean(values[0]), STATE_ELEMENT_MASKING)
   CASE(VG_SCISSORING, state->scissoring = clean_boolean(values[0]), STATE_ELEMENT_SCISSORING)
   CASE(VG_FILTER_FORMAT_LINEAR, state->filter_format_linear = clean_boolean(values[0]), STATE_ELEMENT_FILTER_FORMAT_LINEAR)
   CASE(VG_FILTER_FORMAT_PREMULTIPLIED, state->filter_format_pre = clean_boolean(values[0]), STATE_ELEMENT_FILTER_FORMAT_PRE)
   CASE(VG_FILTER_CHANNEL_MASK, state->filter_channel_mask = values[0], STATE_ELEMENT_FILTER_CHANNEL_MASK)
   CASE(VG_TILE_FILL_COLOR, state->tile_fill_rgba = values[0], STATE_ELEMENT_TILE_FILL_RGBA)
   CASE(VG_CLEAR_COLOR, state->clear_rgba = values[0], STATE_ELEMENT_CLEAR_RGBA)
   #undef CASE

   /*
      vector param types
   */

   case VG_SCISSOR_RECTS:
   {
      uint32_t i;
      vcos_assert(!(count & 0x3));
      vcos_assert(count <= (VG_CONFIG_MAX_SCISSOR_RECTS * 4));
      for (i = 0; i != count; i += 4) {
         state->scissor.rects[i + 0] = values[i + 0];
         state->scissor.rects[i + 1] = values[i + 1];
         state->scissor.rects[i + 2] = _max(values[i + 2], 0);
         state->scissor.rects[i + 3] = _max(values[i + 3], 0);
      }
      state->scissor.rects_count = count;
      state_changed(state, STATE_ELEMENT_SCISSOR_RECTS);
      break;
   }

   /*
      client shouldn't send any other param types
   */

   default:
      UNREACHABLE();
   }

   VG_UNLOCK_SERVER_STATE();
}

void vgSetfv_impl(
   VGParamType param_type,
   VGint count,
   const VGfloat *values)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   switch (param_type) {
   /*
      scalar param types
   */

   #define CASE(PARAM_TYPE, DO, ELEMENT) \
      case PARAM_TYPE: \
      { \
         vcos_assert(count == 1); \
         DO; \
         state_changed(state, ELEMENT); \
         break; \
      }
   CASE(VG_STROKE_LINE_WIDTH, state->stroke.line_width = clean_float(values[0]), STATE_ELEMENT_STROKE_LINE_WIDTH)
   CASE(VG_STROKE_MITER_LIMIT, state->stroke.miter_limit = _maxf(clean_float(values[0]), 1.0f), STATE_ELEMENT_STROKE_MITER_LIMIT)
   CASE(VG_STROKE_DASH_PHASE, state->stroke.dash_phase = clean_float(values[0]), STATE_ELEMENT_STROKE_DASH_PHASE)
   #undef CASE

   /*
      vector param types
   */

   case VG_COLOR_TRANSFORM_VALUES:
   {
      uint32_t i;
      vcos_assert(count == 8);
      for (i = 0; i != 4; ++i) { state->color_transform_values[i] = clampf(values[i], -127.0f, 127.0f); }
      for (i = 4; i != 8; ++i) { state->color_transform_values[i] = clampf(values[i], -1.0f, 1.0f); }
      state_changed(state, STATE_ELEMENT_COLOR_TRANSFORM_VALUES);
      break;
   }
   case VG_STROKE_DASH_PATTERN:
   {
      uint32_t i;
      vcos_assert(count <= VG_CONFIG_MAX_DASH_COUNT);
      count = count & ~0x1;
      for (i = 0; i != count; ++i) {
         state->stroke.dash_pattern[i] = _maxf(clean_float(values[i]), 0.0f);
      }
      state->stroke.dash_pattern_count = count;
      state_changed(state, STATE_ELEMENT_STROKE_DASH_PATTERN);
      break;
   }
   case VG_GLYPH_ORIGIN:
   {
      vcos_assert(count == 2);
      state->glyph_origin[0] = values[0];
      state->glyph_origin[1] = values[1];
      state_changed(state, STATE_ELEMENT_GLYPH_ORIGIN);
      break;
   }

   /*
      client shouldn't send any other param types
   */

   default:
      UNREACHABLE();
   }

   VG_UNLOCK_SERVER_STATE();
}

void vgGetfv_impl(
   VGParamType param_type,
   VGint count,
   VGfloat *values)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   switch (param_type) {
   /*
      vector param types
   */

   case VG_GLYPH_ORIGIN:
   {
      vcos_assert(count == 2);
      values[0] = state->glyph_origin[0];
      values[1] = state->glyph_origin[1];
      break;
   }

   /*
      client shouldn't request any other param types
   */

   default:
      UNREACHABLE();
   }

   VG_UNLOCK_SERVER_STATE();
}

void vgSetParameteriv_impl(
   VGHandle vg_handle,
   VG_CLIENT_OBJECT_TYPE_T client_object_type,
   VGint param_type,
   VGint count,
   const VGint *values)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   switch (client_object_type) {
   case VG_CLIENT_OBJECT_TYPE_PAINT:
   {
      MEM_HANDLE_T handle;
      VG_PAINT_T *paint;

      handle = vg_handle_to_mem_handle(vg_handle);
      if (!check_object(state, handle, OBJECT_TYPE_PAINT)) {
         break;
      }

      paint = (VG_PAINT_T *)mem_lock(handle);

      switch (param_type) {
      /*
         settable scalar param types
      */

      #define CASE(PARAM_TYPE, DO, ELEMENT) \
         case PARAM_TYPE: \
         { \
            vcos_assert(count == 1); \
            DO; \
            paint_changed(state, handle, paint, ELEMENT); \
            break; \
         }
      CASE(VG_PAINT_TYPE, paint->type = (VGPaintType)values[0], PAINT_ELEMENT_TYPE)
      CASE(VG_PAINT_COLOR_RAMP_SPREAD_MODE, vg_paint_set_ramp_spread_mode(paint, (VGColorRampSpreadMode)values[0]), PAINT_ELEMENT_RAMP_SPREAD_MODE)
      CASE(VG_PAINT_COLOR_RAMP_PREMULTIPLIED, paint->ramp_pre = clean_boolean(values[0]), PAINT_ELEMENT_RAMP_PRE)
      CASE(VG_PAINT_PATTERN_TILING_MODE, paint->pattern_tiling_mode = (VGTilingMode)values[0], PAINT_ELEMENT_PATTERN_TILING_MODE)
      CASE(VG_PAINT_COLOR, paint->rgba = values[0], PAINT_ELEMENT_RGBA);
      #undef CASE

      /*
         client shouldn't send any other param types
      */

      default:
         UNREACHABLE();
      }

      mem_unlock(handle);

      break;
   }

   /*
      client shouldn't send any other object types
   */

   default:
   {
      UNREACHABLE();
   }
   }

   VG_UNLOCK_SERVER_STATE();
}

void vgSetParameterfv_impl(
   VGHandle vg_handle,
   VG_CLIENT_OBJECT_TYPE_T client_object_type,
   VGint param_type,
   VGint count,
   const VGfloat *values)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   switch (client_object_type) {
   case VG_CLIENT_OBJECT_TYPE_PAINT:
   {
      MEM_HANDLE_T handle;
      VG_PAINT_T *paint;

      handle = vg_handle_to_mem_handle(vg_handle);
      if (!check_object(state, handle, OBJECT_TYPE_PAINT)) {
         break;
      }

      paint = (VG_PAINT_T *)mem_lock(handle);

      switch (param_type) {
      /*
         vector param types
      */

      case VG_PAINT_COLOR_RAMP_STOPS:
      {
         vcos_assert((count % 5) == 0);
         vcos_assert(count <= (VG_CONFIG_MAX_COLOR_RAMP_STOPS * 5));
         if (count == 0) {
            if (paint->ramp_stops != MEM_INVALID_HANDLE) {
               mem_release(paint->ramp_stops);
               paint->ramp_stops = MEM_INVALID_HANDLE;
            }
         } else {
            float *ramp_stops;
            uint32_t i;
            if (paint->ramp_stops == MEM_INVALID_HANDLE) {
               paint->ramp_stops = mem_alloc_ex(count * sizeof(float), alignof(float), (MEM_FLAG_T)(MEM_FLAG_RESIZEABLE | MEM_FLAG_NO_INIT | MEM_FLAG_HINT_GROW), "VG_PAINT_T.ramp_stops", MEM_COMPACT_DISCARD);
               if (paint->ramp_stops == MEM_INVALID_HANDLE) {
                  set_error(state, VG_OUT_OF_MEMORY_ERROR);
                  break;
               }
            } else if (!mem_resize_ex(paint->ramp_stops, count * sizeof(float), MEM_COMPACT_DISCARD)) {
               set_error(state, VG_OUT_OF_MEMORY_ERROR);
               break;
            }
            ramp_stops = (float *)mem_lock(paint->ramp_stops);
            for (i = 0; i != count; i += 5) {
               ramp_stops[i + 0] = clean_float(values[i + 0]);
               ramp_stops[i + 1] = clampf(values[i + 1], 0.0f, 1.0f);
               ramp_stops[i + 2] = clampf(values[i + 2], 0.0f, 1.0f);
               ramp_stops[i + 3] = clampf(values[i + 3], 0.0f, 1.0f);
               ramp_stops[i + 4] = clampf(values[i + 4], 0.0f, 1.0f);
            }
            mem_unlock(paint->ramp_stops);
         }
         paint_changed(state, handle, paint, PAINT_ELEMENT_RAMP_STOPS);
         break;
      }
      case VG_PAINT_LINEAR_GRADIENT:
      {
         vcos_assert(count == 4);
         clean_floats(paint->linear_gradient, values, 4);
         paint_changed(state, handle, paint, PAINT_ELEMENT_LINEAR_GRADIENT);
         break;
      }
      case VG_PAINT_RADIAL_GRADIENT:
      {
         vcos_assert(count == 5);
         clean_floats(paint->radial_gradient, values, 5);
         paint_changed(state, handle, paint, PAINT_ELEMENT_RADIAL_GRADIENT);
         break;
      }

      /*
         client shouldn't send any other param types
      */

      default:
         UNREACHABLE();
      }

      mem_unlock(handle);

      break;
   }

   /*
      client shouldn't send any other object types
   */

   default:
   {
      UNREACHABLE();
   }
   }

   VG_UNLOCK_SERVER_STATE();
}

bool vgGetParameteriv_impl(
   VGHandle vg_handle,
   VG_CLIENT_OBJECT_TYPE_T client_object_type,
   VGint param_type,
   VGint count,
   VGint *values)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   switch (client_object_type) {
   case VG_CLIENT_OBJECT_TYPE_PATH:
   {
      MEM_HANDLE_T handle;
      VG_PATH_T *path;

      handle = vg_handle_to_mem_handle(vg_handle);
      if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
         VG_UNLOCK_SERVER_STATE();
         return false;
      }

      path = (VG_PATH_T *)mem_lock(handle);

      switch (param_type) {
      /*
         scalar param types
      */

      case VG_PATH_NUM_SEGMENTS:
      {
         vcos_assert(count == 1);
         values[0] = vg_path_get_segments_size(path);
         break;
      }
      case VG_PATH_NUM_COORDS:
      {
         vcos_assert(count == 1);
         values[0] = vg_path_get_coords_size(path) / get_path_datatype_size((VGPathDatatype)path->datatype);
         break;
      }

      /*
         client shouldn't request any other param types
      */

      default:
         UNREACHABLE();
      }

      mem_unlock(handle);

      break;
   }
   case VG_CLIENT_OBJECT_TYPE_FONT:
   {
      MEM_HANDLE_T handle;
      VG_FONT_T *font;

      handle = vg_handle_to_mem_handle(vg_handle);
      if (!check_object(state, handle, OBJECT_TYPE_FONT)) {
         VG_UNLOCK_SERVER_STATE();
         return false;
      }

      font = (VG_FONT_T *)mem_lock(handle);

      switch (param_type) {
      /*
         scalar param types
      */

      case VG_FONT_NUM_GLYPHS:
      {
         vcos_assert(count == 1);
         values[0] = font->count;
         break;
      }

      /*
         client shouldn't request any other param types
      */

      default:
         UNREACHABLE();
      }

      mem_unlock(handle);

      break;
   }

   /*
      client shouldn't send any other object types
   */

   default:
   {
      UNREACHABLE();
   }
   }

   VG_UNLOCK_SERVER_STATE();
   return true;
}

/******************************************************************************
api matrices
******************************************************************************/

void vgLoadMatrix_impl(VGMatrixMode matrix_mode, const VG_MAT3X3_T *matrix)
{
#ifdef BCG_FB_LAYOUT
   const VG_MAT3X3_T flip_m = {
      { { 1.0f,   0.0f,  0.0f },
        { 0.0f,  -1.0f,  0.0f },
        { 0.0f,   0.0f,  1.0f } }
   };
#endif /* BCG_FB_LAYOUT */
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   switch (matrix_mode) {
   case VG_MATRIX_PATH_USER_TO_SURFACE:
   {
#ifdef BCG_FB_LAYOUT
      vg_mat3x3_mul(&state->path_user_to_surface, &flip_m, matrix);
      vg_mat3x3_premul_translate(&state->path_user_to_surface, 0.0f, (float)frame_height);
#else
      state->path_user_to_surface = *matrix;
#endif /* BCG_FB_LAYOUT */
      state_changed(state, STATE_ELEMENT_PATH_USER_TO_SURFACE);
      break;
   }
   case VG_MATRIX_IMAGE_USER_TO_SURFACE:
   {
#ifdef BCG_FB_LAYOUT
      vg_mat3x3_mul(&state->image_user_to_surface, &flip_m, matrix);
      vg_mat3x3_premul_translate(&state->image_user_to_surface, 0.0f, (float)frame_height);
#else
      state->image_user_to_surface = *matrix;
#endif /* BCG_FB_LAYOUT */
      state_changed(state, STATE_ELEMENT_IMAGE_USER_TO_SURFACE);
      break;
   }
   case VG_MATRIX_GLYPH_USER_TO_SURFACE:
   {
#ifdef BCG_FB_LAYOUT
      vg_mat3x3_mul(&state->glyph_user_to_surface, &flip_m, matrix);
      vg_mat3x3_premul_translate(&state->glyph_user_to_surface, 0.0f, (float)frame_height);
#else
      state->glyph_user_to_surface = *matrix;
#endif /* BCG_FB_LAYOUT */
      state_changed(state, STATE_ELEMENT_GLYPH_USER_TO_SURFACE);
      break;
   }
   case VG_MATRIX_FILL_PAINT_TO_USER:
   {
      state->fill_paint_to_user = *matrix;
      state_changed(state, STATE_ELEMENT_FILL_PAINT_TO_USER);
      break;
   }
   case VG_MATRIX_STROKE_PAINT_TO_USER:
   {
      state->stroke_paint_to_user = *matrix;
      state_changed(state, STATE_ELEMENT_STROKE_PAINT_TO_USER);
      break;
   }
   default:
   {
      UNREACHABLE();
   }
   }
   VG_UNLOCK_SERVER_STATE();
}

/******************************************************************************
api mask/clear
******************************************************************************/

void vgMask_impl(
   VGHandle vg_handle, /* theoretically image under vg 1.0 */
   VGMaskOperation operation,
   VGint dst_x, VGint dst_y,
   VGint width, VGint height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   int32_t src_x = 0, src_y = 0;

   if (!is_mask_operation(operation) || (width <= 0) || (height <= 0)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if ((operation == VG_CLEAR_MASK) || (operation == VG_FILL_MASK)) {
      handle = MEM_INVALID_HANDLE;
      khrn_clip_rect(&dst_x, &dst_y, &width, &height, 0, 0, frame_width, frame_height);
   } else {
      uint32_t child_x = 0, child_y = 0, child_width, child_height;

      handle = vg_handle_to_mem_handle(vg_handle);
      if (!check_object(state, handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE | OBJECT_TYPE_MASK_LAYER))) {
         VG_UNLOCK_SERVER_STATE();
         return;
      }

      if (vg_is_mask_layer(handle)) {
         VG_MASK_LAYER_T *mask_layer = (VG_MASK_LAYER_T *)mem_lock(handle);
         child_width = mask_layer->image_width;
         child_height = mask_layer->image_height;
         mem_unlock(handle);
      } else {
         vcos_assert(vg_is_image(handle) || vg_is_child_image(handle));
         vg_image_lock_adam(&child_x, &child_y, &child_width, &child_height, &handle);
         mem_unlock(handle);
      }

      khrn_clip_rect2(
         &dst_x, &dst_y, &src_x, &src_y, &width, &height,
         0, 0, frame_width, frame_height,
         0, 0, child_width, child_height);

      src_x += child_x;
      src_y += child_y;
   }

   if ((mask_handle != MEM_INVALID_HANDLE) && /* all parameters have been checked now... */
      !vg_be_mask(handle, operation, dst_x, dst_y, width, height, src_x, src_y)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
   }
   VG_UNLOCK_SERVER_STATE();
}

void vgRenderToMask_impl(
   VGPath vg_handle,
   VGbitfield paint_modes,
   VGMaskOperation operation)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_PATH_T *path = NULL;
   uint32_t clip_rect[4];
   MEM_HANDLE_T scissor_handle = MEM_INVALID_HANDLE;
   float clip[4];
   float scale_max = 0.0f;
   float stroke_clip[4];

   if (!is_paint_modes(paint_modes) || !is_mask_operation(operation)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if (mask_handle == MEM_INVALID_HANDLE) { /* all parameters have been checked now... */
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if ((operation != VG_CLEAR_MASK) && (operation != VG_FILL_MASK)) {
      path = (VG_PATH_T *)mem_lock(handle);
      if (path->flags & VG_PATH_FLAG_EMPTY) {
         paint_modes = 0;
      } else {
         bool bounds_valid = vg_path_are_bounds_valid(path);
         float surface_bounds[4];
         if (bounds_valid) {
            get_transformed_bounds(surface_bounds, path->bounds, &state->path_user_to_surface);
         }
         if (!prepare_scissor(state->scissoring ? &state->scissor : NULL, clip_rect, &scissor_handle, clip)) {
            set_error(state, VG_OUT_OF_MEMORY_ERROR);
            mem_unlock(handle);
            VG_UNLOCK_SERVER_STATE();
            return;
         }
         if ((clip_rect[2] == 0) || (clip_rect[3] == 0)) {
            paint_modes = 0;
         }
         if (paint_modes & VG_STROKE_PATH) {
            if (operation == VG_SET_MASK) {
               paint_modes &= ~VG_FILL_PATH;
            }
            if (state->stroke.line_width > 0.0f) {
               scale_max = get_scale_max(&state->path_user_to_surface);
               get_stroke_clip(stroke_clip, clip, &state->stroke, scale_max);
               if (bounds_valid && !bounds_intersect(surface_bounds, stroke_clip)) {
                  paint_modes = 0;
               }
            } else {
               paint_modes = ((operation == VG_UNION_MASK) || (operation == VG_SUBTRACT_MASK)) ?
                  (paint_modes & ~VG_STROKE_PATH) : 0;
            }
         }
         if ((paint_modes & VG_FILL_PATH) && bounds_valid && !bounds_intersect(surface_bounds, clip)) {
            paint_modes = ((operation == VG_UNION_MASK) || (operation == VG_SUBTRACT_MASK)) ?
               (paint_modes & ~VG_FILL_PATH) : 0;
         }
      }
      if (!paint_modes) {
         if (scissor_handle != MEM_INVALID_HANDLE) { mem_unretain(scissor_handle); }
         mem_unlock(handle);
         if ((operation == VG_UNION_MASK) || (operation == VG_SUBTRACT_MASK)) {
            VG_UNLOCK_SERVER_STATE();
            return;
         }
         vcos_assert((operation == VG_SET_MASK) || (operation == VG_INTERSECT_MASK));
         operation = VG_CLEAR_MASK;
         path = NULL;
         scissor_handle = MEM_INVALID_HANDLE;
      }
   }

   if (!vg_be_render_to_mask(state, handle, path, paint_modes, operation, clip_rect, scissor_handle, clip, scale_max, stroke_clip)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
   }
   VG_UNLOCK_SERVER_STATE();
}

void vgCreateMaskLayer_impl(
   VGHandle vg_handle,
   VGint width, VGint height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   vcos_assert(is_object_ok(state, handle, OBJECT_TYPE_STEM));

   vg_mask_layer_bprint_from_stem(handle, width, height); /* just create a blueprint for now -- we'll create the actual object when we need it */

   VG_UNLOCK_SERVER_STATE();
}

void vgDestroyMaskLayer_impl(
   VGMaskLayer vg_handle)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, (OBJECT_TYPE_T)(OBJECT_TYPE_MASK_LAYER_BPRINT | OBJECT_TYPE_MASK_LAYER))) { /* allow blueprints to be destroyed */
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   delete_object(state, handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgFillMaskLayer_impl(
   VGMaskLayer vg_handle,
   VGint x, VGint y,
   VGint width, VGint height,
   VGfloat value)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_MASK_LAYER_T *mask_layer;

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_MASK_LAYER)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   mask_layer = (VG_MASK_LAYER_T *)mem_lock(handle);

   if ((x < 0) || (y < 0) || (width <= 0) || (height <= 0) ||
      /* x + width / y + height will fit in uint32_t (may not fit in int32_t) */
      (((uint32_t)(x + width)) > mask_layer->image_width) ||
      (((uint32_t)(y + height)) > mask_layer->image_height) ||
      (value < 0.0f) || (value > 1.0f)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   vg_bf_clear_region(mask_layer->image, x, y, width, height, color_floats_to_rgba(1.0f, 1.0f, 1.0f, value));

   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgCopyMask_impl(
   VGMaskLayer dst_vg_handle,
   VGint dst_x, VGint dst_y,
   VGint src_x, VGint src_y,
   VGint width, VGint height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T dst_handle;
   VG_MASK_LAYER_T *dst;

   if ((width <= 0) || (height <= 0)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst_handle = vg_handle_to_mem_handle(dst_vg_handle);
   if (!check_object(state, dst_handle, OBJECT_TYPE_MASK_LAYER)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if (mask_handle == MEM_INVALID_HANDLE) { /* all parameters have been checked now... */
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst = (VG_MASK_LAYER_T *)mem_lock(dst_handle);

   khrn_clip_rect2(
      &dst_x, &dst_y, &src_x, &src_y, &width, &height,
      0, 0, dst->image_width, dst->image_height,
      0, 0, frame_width, frame_height);

   vg_bf_copy_region(dst->image, dst_x, dst_y, width, height, mask_handle, src_x, src_y);

   mem_unlock(dst_handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgClear_impl(
   VGint x, VGint y,
   VGint width, VGint height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   if ((width <= 0) || (height <= 0)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   khrn_clip_rect(&x, &y, &width, &height, 0, 0, frame_width, frame_height);

   if (!vg_be_clear(state, x, y, width, height)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
   }
   VG_UNLOCK_SERVER_STATE();
}

/******************************************************************************
api path
******************************************************************************/

void vgCreatePath_impl(
   VGHandle vg_handle,
   VGint format,
   VGPathDatatype datatype,
   VGfloat scale, VGfloat bias,
   VGint segments_capacity,
   VGint coords_capacity,
   VGbitfield caps)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   vcos_assert(is_object_ok(state, handle, OBJECT_TYPE_STEM));

   vg_path_bprint_from_stem(handle,
      format, datatype, caps,
      scale, bias,
      segments_capacity, coords_capacity); /* just create a blueprint for now -- we'll create the actual object when we need it */

   VG_UNLOCK_SERVER_STATE();
}

void vgClearPath_impl(
   VGPath vg_handle,
   VGbitfield caps)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, (OBJECT_TYPE_T)(OBJECT_TYPE_PATH_BPRINT | OBJECT_TYPE_PATH))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if (vg_is_path(handle)) {
      VG_PATH_T *path = (VG_PATH_T *)mem_lock(handle);
      if (vg_path_clear(path)) {
         path->caps = (uint16_t)caps;
         path_changed(state, handle, path, (PATH_ELEMENT_T)(PATH_ELEMENT_CAPS | PATH_ELEMENT_DATA));
      } else {
         set_error(state, VG_OUT_OF_MEMORY_ERROR);
      }
      mem_unlock(handle);
   } else {
      VG_PATH_BPRINT_T *path_bprint;
      vcos_assert(vg_is_path_bprint(handle));
      path_bprint = (VG_PATH_BPRINT_T *)mem_lock(handle);
      path_bprint->caps = caps;
      mem_unlock(handle);
   }

   VG_UNLOCK_SERVER_STATE();
}

void vgDestroyPath_impl(
   VGPath vg_handle)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, (OBJECT_TYPE_T)(OBJECT_TYPE_PATH_BPRINT | OBJECT_TYPE_PATH))) { /* allow blueprints to be destroyed */
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   delete_object(state, handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgRemovePathCapabilities_impl(
   VGPath vg_handle,
   VGbitfield caps)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, (OBJECT_TYPE_T)(OBJECT_TYPE_PATH_BPRINT | OBJECT_TYPE_PATH))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if (vg_is_path(handle)) {
      VG_PATH_T *path = (VG_PATH_T *)mem_lock(handle);
      path->caps = (uint16_t)(path->caps & ~caps);
      path_changed(state, handle, path, PATH_ELEMENT_CAPS);
      mem_unlock(handle);
   } else {
      VG_PATH_BPRINT_T *path_bprint;
      vcos_assert(vg_is_path_bprint(handle));
      path_bprint = (VG_PATH_BPRINT_T *)mem_lock(handle);
      path_bprint->caps &= ~caps;
      mem_unlock(handle);
   }

   VG_UNLOCK_SERVER_STATE();
}

void vgAppendPath_impl(
   VGPath dst_vg_handle,
   VGPath src_vg_handle)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T dst_handle, src_handle;
   VG_PATH_T *dst, *src;
   uint32_t segments_count, coords_count, dst_coords_size;
   void *dst_coords;
   const void *src_coords;
   float oo_dst_scale;

   dst_handle = vg_handle_to_mem_handle(dst_vg_handle);
   src_handle = vg_handle_to_mem_handle(src_vg_handle);
   if (!check_object(state, dst_handle, OBJECT_TYPE_PATH) ||
      !check_object(state, src_handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst = (VG_PATH_T *)mem_lock(dst_handle);
   src = (VG_PATH_T *)mem_lock(src_handle);

   if (!(dst->caps & VG_PATH_CAPABILITY_APPEND_TO) ||
      !(src->caps & VG_PATH_CAPABILITY_APPEND_FROM)) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(src_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   /*
      no need for special-case handling of self-append
   */

   if (!vg_path_write_immediate(dst, VG_PATH_RW_DATA) ||
      !vg_path_read_immediate(src, VG_PATH_RW_DATA)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(src_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   segments_count = mem_get_size(src->segments);
   coords_count = mem_get_size(src->coords) / get_path_datatype_size((VGPathDatatype)src->datatype);
   dst_coords_size = coords_count * get_path_datatype_size((VGPathDatatype)dst->datatype);

   if (!mem_resize_ex(dst->segments, mem_get_size(dst->segments) + segments_count, MEM_COMPACT_DISCARD)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(src_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }
   if (!mem_resize_ex(dst->coords, mem_get_size(dst->coords) + dst_coords_size, MEM_COMPACT_DISCARD)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      verify(mem_resize_ex(dst->segments, mem_get_size(dst->segments) - segments_count, MEM_COMPACT_NONE));
      mem_unlock(src_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   memcpy((uint8_t *)mem_lock(dst->segments) + (mem_get_size(dst->segments) - segments_count), mem_lock(src->segments), segments_count);
   mem_unlock(src->segments);
   mem_unlock(dst->segments);

   dst_coords = (uint8_t *)mem_lock(dst->coords) + (mem_get_size(dst->coords) - dst_coords_size);
   src_coords = mem_lock(src->coords);
   oo_dst_scale = recip_(dst->scale);
   for (; coords_count != 0; --coords_count) {
      put_coord((VGPathDatatype)dst->datatype, oo_dst_scale, dst->bias, &dst_coords,
         get_coord((VGPathDatatype)src->datatype, src->scale, src->bias, &src_coords));
   }
   mem_unlock(src->coords);
   mem_unlock(dst->coords);

   path_changed(state, dst_handle, dst, PATH_ELEMENT_DATA);

   mem_unlock(src_handle);
   mem_unlock(dst_handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgAppendPathData_impl(
   VGPath vg_handle,
   VGPathDatatype datatype,
   VGint segments_count, const VGubyte *segments,
   VGuint coords_size, const void *coords)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_PATH_T *path;
   void *path_coords;

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path = (VG_PATH_T *)mem_lock(handle);

   if (path->datatype != datatype) {
      /*
         this can happen if we destroy the path and reuse the handle when
         creating another path all before this vgAppendPathData command gets to
         the server. an appropriate error is therefore bad handle
      */

      set_error(state, VG_BAD_HANDLE_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if (!(path->caps & VG_PATH_CAPABILITY_APPEND_TO)) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if (!vg_path_write_immediate(path, VG_PATH_RW_DATA)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if (!mem_resize_ex(path->segments, mem_get_size(path->segments) + segments_count, MEM_COMPACT_DISCARD)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }
   if (!mem_resize_ex(path->coords, mem_get_size(path->coords) + coords_size, MEM_COMPACT_DISCARD)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      verify(mem_resize_ex(path->segments, mem_get_size(path->segments) - segments_count, MEM_COMPACT_NONE));
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   memcpy((uint8_t *)mem_lock(path->segments) + (mem_get_size(path->segments) - segments_count), segments, segments_count);
   mem_unlock(path->segments);

   path_coords = (uint8_t *)mem_lock(path->coords) + (mem_get_size(path->coords) - coords_size);
   if (path->datatype == VG_PATH_DATATYPE_F) {
      clean_floats((float *)path_coords, (const float *)coords, coords_size / sizeof(float));
   } else {
      memcpy(path_coords, coords, coords_size);
   }
   mem_unlock(path->coords);

   path_changed(state, handle, path, PATH_ELEMENT_DATA);

   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgModifyPathCoords_impl(
   VGPath vg_handle,
   VGPathDatatype datatype,
   VGuint coords_offset, VGuint coords_size, const void *coords)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_PATH_T *path;
   void *path_coords;

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path = (VG_PATH_T *)mem_lock(handle);

   if (path->datatype != datatype) {
      /*
         this can happen if we destroy the path and reuse the handle when
         creating another path all before this vgModifyPathCoords command gets
         to the server. an appropriate error is therefore bad handle
      */

      set_error(state, VG_BAD_HANDLE_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if ((coords_offset + coords_size) > vg_path_get_coords_size(path)) {
      /*
         some inconsistency has arisen. this can happen when multiple client
         threads fiddle with the same object without appropriate vgFlush()ing,
         or if we ran out of memory in something like vgAppendPath

         i believe the appropriate error is illegal argument
      */

      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if (!(path->caps & VG_PATH_CAPABILITY_MODIFY)) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if (!vg_path_write_immediate(path, VG_PATH_RW_COORDS)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path_coords = (uint8_t *)mem_lock(path->coords) + coords_offset;
   if (path->datatype == VG_PATH_DATATYPE_F) {
      clean_floats((float *)path_coords, (const float *)coords, coords_size / sizeof(float));
   } else {
      memcpy(path_coords, coords, coords_size);
   }
   mem_unlock(path->coords);

   path_changed(state, handle, path, PATH_ELEMENT_DATA);

   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgTransformPath_impl(
   VGPath dst_vg_handle,
   VGPath src_vg_handle)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T dst_handle, src_handle;
   VG_PATH_T *dst, *src;
   uint32_t segments_count, coords_count, dst_coords_size;
   uint8_t *dst_segments;
   const uint8_t *src_segments;
   void *dst_coords;
   const void *src_coords;
   float oo_dst_scale, s_x, s_y, o_x, o_y;

   dst_handle = vg_handle_to_mem_handle(dst_vg_handle);
   src_handle = vg_handle_to_mem_handle(src_vg_handle);
   if (!check_object(state, dst_handle, OBJECT_TYPE_PATH) ||
      !check_object(state, src_handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst = (VG_PATH_T *)mem_lock(dst_handle);
   src = (VG_PATH_T *)mem_lock(src_handle);

   if (!(dst->caps & VG_PATH_CAPABILITY_TRANSFORM_TO) ||
      !(src->caps & VG_PATH_CAPABILITY_TRANSFORM_FROM)) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(src_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   /*
      no need for special-case handling of self-append
   */

   if (!vg_path_write_immediate(dst, VG_PATH_RW_DATA) ||
      !vg_path_read_immediate(src, VG_PATH_RW_DATA)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(src_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   segments_count = mem_get_size(src->segments);
   coords_count = get_transform_coords_count((const uint8_t *)mem_lock(src->segments), segments_count);
   mem_unlock(src->segments);
   dst_coords_size = coords_count * get_path_datatype_size((VGPathDatatype)dst->datatype);

   if (!mem_resize_ex(dst->segments, mem_get_size(dst->segments) + segments_count, MEM_COMPACT_DISCARD)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(src_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }
   if (!mem_resize_ex(dst->coords, mem_get_size(dst->coords) + dst_coords_size, MEM_COMPACT_DISCARD)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      verify(mem_resize_ex(dst->segments, mem_get_size(dst->segments) - segments_count, MEM_COMPACT_NONE));
      mem_unlock(src_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst_segments = (uint8_t *)mem_lock(dst->segments) + (mem_get_size(dst->segments) - segments_count);
   src_segments = (const uint8_t *)mem_lock(src->segments);
   dst_coords = (uint8_t *)mem_lock(dst->coords) + (mem_get_size(dst->coords) - dst_coords_size);
   src_coords = mem_lock(src->coords);
   oo_dst_scale = recip_(dst->scale);
   s_x = 0.0f; s_y = 0.0f;
   o_x = 0.0f; o_y = 0.0f;
   for (; segments_count != 0; ++dst_segments, ++src_segments, --segments_count) {
      #define GET_COORD() get_coord((VGPathDatatype)src->datatype, src->scale, src->bias, &src_coords)
      #define PUT_COORD(X) put_coord((VGPathDatatype)dst->datatype, oo_dst_scale, dst->bias, &dst_coords, X)

      uint32_t segment = *src_segments;

      /*
         store the transformed coords
      */

      switch (segment & VG_SEGMENT_MASK) {
      case VG_CLOSE_PATH:
      {
         o_x = s_x;  o_y = s_y;
         break;
      }
      case VG_MOVE_TO:
      case VG_LINE_TO:
      case VG_QUAD_TO:
      case VG_CUBIC_TO:
      case VG_SQUAD_TO:
      case VG_SCUBIC_TO:
      {
         float x = 0.0f;
         float y = 0.0f;
         uint32_t segment_coords_count = get_segment_coords_count(segment & VG_SEGMENT_MASK);
         for (; segment_coords_count != 0; segment_coords_count -= 2) {
            float t_x, t_y;
            x = GET_COORD();
            y = GET_COORD();
            t_x = x; t_y = y;
            if (segment & VG_RELATIVE) {
               x += o_x;  y += o_y;
               vg_mat3x3_affine_transform_t(&state->path_user_to_surface, &t_x, &t_y);
            } else {
               vg_mat3x3_affine_transform(&state->path_user_to_surface, &t_x, &t_y);
            }
            PUT_COORD(t_x);
            PUT_COORD(t_y);
         }
         if ((segment & VG_SEGMENT_MASK) == VG_MOVE_TO) {
            s_x = x;  s_y = y;
         }
         o_x = x;  o_y = y;
         break;
      }
      case VG_HLINE_TO:
      {
         float x = GET_COORD();
         float t_x = x, t_y;
         if (segment & VG_RELATIVE) {
            x += o_x;
            t_y = 0.0f;
            vg_mat3x3_affine_transform_t(&state->path_user_to_surface, &t_x, &t_y);
         } else {
            t_y = o_y;
            vg_mat3x3_affine_transform(&state->path_user_to_surface, &t_x, &t_y);
         }
         PUT_COORD(t_x);
         PUT_COORD(t_y);
         segment = VG_LINE_TO | (segment & VG_RELATIVE);
         o_x = x;
         break;
      }
      case VG_VLINE_TO:
      {
         float y = GET_COORD();
         float t_x, t_y = y;
         if (segment & VG_RELATIVE) {
            y += o_y;
            t_x = 0.0f;
            vg_mat3x3_affine_transform_t(&state->path_user_to_surface, &t_x, &t_y);
         } else {
            t_x = o_x;
            vg_mat3x3_affine_transform(&state->path_user_to_surface, &t_x, &t_y);
         }
         PUT_COORD(t_x);
         PUT_COORD(t_y);
         segment = VG_LINE_TO | (segment & VG_RELATIVE);
         o_y = y;
         break;
      }
      case VG_SCCWARC_TO:
      case VG_SCWARC_TO:
      case VG_LCCWARC_TO:
      case VG_LCWARC_TO:
      {
         float r_x = GET_COORD();
         float r_y = GET_COORD();
         float angle = GET_COORD();
         float x = GET_COORD();
         float y = GET_COORD();
         VG_MAT3X3_T a;
         float t_x, t_y;

         /* spec section 8.4: negative radii replaced by absolute values, angle taken mod 360 degrees */
         r_x = absf_(r_x);  r_y = absf_(r_y);
         angle = modf_(angle * (PI / 180.0f), 2.0f * PI);

         a = state->path_user_to_surface;
         vg_mat3x3_postmul_rotate(&a, angle);
         vg_mat3x3_postmul_scale(&a, r_x, r_y);

         /*
            a maps from unit circle with unknown center to transformed ellipse
            a can be decomposed into rotate/flip, scale, rotate, translate
            scale and second rotate are new radii and angle
         */

         vg_mat3x3_rsq(&a, &angle, &r_x, &r_y);

         t_x = x; t_y = y;
         if (segment & VG_RELATIVE) {
            x += o_x;  y += o_y;
            vg_mat3x3_affine_transform_t(&state->path_user_to_surface, &t_x, &t_y);
         } else {
            vg_mat3x3_affine_transform(&state->path_user_to_surface, &t_x, &t_y);
         }

         angle *= 180.0f / PI;

         PUT_COORD(r_x);
         PUT_COORD(r_y);
         PUT_COORD(angle);
         PUT_COORD(t_x);
         PUT_COORD(t_y);

         if (vg_mat3x3_det(&state->path_user_to_surface) < 0.0f) {
            /*
               matrix involves a flip
               need to switch direction of arcs
            */

            switch (segment & VG_SEGMENT_MASK) {
            case VG_SCCWARC_TO: segment = VG_SCWARC_TO  | (segment & VG_RELATIVE); break;
            case VG_SCWARC_TO:  segment = VG_SCCWARC_TO | (segment & VG_RELATIVE); break;
            case VG_LCCWARC_TO: segment = VG_LCWARC_TO  | (segment & VG_RELATIVE); break;
            case VG_LCWARC_TO:  segment = VG_LCCWARC_TO | (segment & VG_RELATIVE); break;
            default:            UNREACHABLE();
            }
         }

         o_x = x;  o_y = y;
         break;
      }
      default:
      {
         UNREACHABLE();
      }
      }

      /*
         store the normalised segment type
      */

      *dst_segments = (uint8_t)segment;

      #undef PUT_COORD
      #undef GET_COORD
   }
   mem_unlock(src->coords);
   mem_unlock(dst->coords);
   mem_unlock(src->segments);
   mem_unlock(dst->segments);

   path_changed(state, dst_handle, dst, PATH_ELEMENT_DATA);

   mem_unlock(src_handle);
   mem_unlock(dst_handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgInterpolatePath_impl(
   VGPath dst_vg_handle,
   VGPath begin_vg_handle,
   VGPath end_vg_handle,
   VGfloat t)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T dst_handle, begin_handle, end_handle;
   VG_PATH_T *dst, *begin, *end;
   VGErrorCode error;

   dst_handle = vg_handle_to_mem_handle(dst_vg_handle);
   begin_handle = vg_handle_to_mem_handle(begin_vg_handle);
   end_handle = vg_handle_to_mem_handle(end_vg_handle);
   if (!check_object(state, dst_handle, OBJECT_TYPE_PATH) ||
      !check_object(state, begin_handle, OBJECT_TYPE_PATH) ||
      !check_object(state, end_handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst = (VG_PATH_T *)mem_lock(dst_handle);
   begin = (VG_PATH_T *)mem_lock(begin_handle);
   end = (VG_PATH_T *)mem_lock(end_handle);

   if (!(dst->caps & VG_PATH_CAPABILITY_INTERPOLATE_TO) ||
      !(begin->caps & VG_PATH_CAPABILITY_INTERPOLATE_FROM) ||
      !(end->caps & VG_PATH_CAPABILITY_INTERPOLATE_FROM)) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(end_handle);
      mem_unlock(begin_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   error = vg_path_interpolate(dst_handle, dst, begin_handle, begin, end_handle, end, t);
   if (error == VG_NO_ERROR) {
      path_changed(state, dst_handle, dst, PATH_ELEMENT_DATA);
   } else {
      set_error(state, error);
   }
   mem_unlock(end_handle);
   mem_unlock(begin_handle);
   mem_unlock(dst_handle);
   VG_UNLOCK_SERVER_STATE();
}

VGfloat vgPathLength_impl(
   VGPath vg_handle,
   VGint segments_i, VGint segments_count)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_PATH_T *path;
   float length;

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return -1.0f;
   }

   path = (VG_PATH_T *)mem_lock(handle);

   if (!(path->caps & VG_PATH_CAPABILITY_PATH_LENGTH)) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return -1.0f;
   }

   if ((segments_i < 0) || (segments_count <= 0) ||
      /* will fit in uint32_t (may not fit in int32_t) */
      (((uint32_t)(segments_i + segments_count)) > vg_path_get_segments_size(path))) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return -1.0f;
   }

   if (path->flags & VG_PATH_FLAG_EMPTY) {
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return 0.0f;
   }

   length = -1.0f;
   if (!vg_path_get_length(handle, path, &length, segments_i, segments_count)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
   }
   mem_unlock(handle);
   VG_UNLOCK_SERVER_STATE();
   return length;
}

bool vgPointAlongPath_impl(
   VGPath vg_handle,
   VGint segments_i, VGint segments_count,
   VGfloat distance,
   VGbitfield mask,
   VGfloat *values)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_PATH_T *path;
   bool success;

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return false;
   }

   path = (VG_PATH_T *)mem_lock(handle);

   if (((mask & 0x1) && !(path->caps & VG_PATH_CAPABILITY_POINT_ALONG_PATH)) ||
      ((mask & 0x2) && !(path->caps & VG_PATH_CAPABILITY_TANGENT_ALONG_PATH))) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return false;
   }

   if ((segments_i < 0) || (segments_count <= 0) ||
      /* will fit in uint32_t (may not fit in int32_t) */
      (((uint32_t)(segments_i + segments_count)) > vg_path_get_segments_size(path))) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return false;
   }

   if (path->flags & VG_PATH_FLAG_EMPTY) {
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      if (mask & 0x1) { values[0] = 0.0f; values[1] = 0.0f; } /* position */
      if (mask & 0x2) { values[2] = 1.0f; values[3] = 0.0f; } /* tangent */
      return true;
   }

   success = true;
   if (!vg_path_get_p_t_along(handle, path, (mask & 0x1) ? values : NULL, (mask & 0x2) ? (values + 2) : NULL, segments_i, segments_count, distance)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      success = false;
   }
   mem_unlock(handle);
   VG_UNLOCK_SERVER_STATE();
   return success;
}

bool vgPathBounds_impl(
   VGPath vg_handle,
   VGfloat *values)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_PATH_T *path;

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return false;
   }

   path = (VG_PATH_T *)mem_lock(handle);

   if (!(path->caps & VG_PATH_CAPABILITY_PATH_BOUNDS)) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return false;
   }

   if (!vg_path_read_tight_bounds_immediate(handle, path)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return false;
   }

   /*
      todo:

      the 1.1 spec includes the phrase:
      "The bounding box of a path is defined to contain all points along the
      path, including isolated points created by MOVE_TO segments."

      i'm not entirely sure what this means -- does it mean that a move to
      (10, 10) directly followed by a move to (20, 20) should force (10, 10)
      to be included in the bounding box? the ri doesn't appear to do this
   */

   if (path->bounds[0] > path->bounds[2]) {
      vcos_assert(path->bounds[1] > path->bounds[3]);
      values[0] = 0.0f;
      values[1] = 0.0f;
      values[2] = -1.0f;
      values[3] = -1.0f;
   } else {
      vcos_assert(!nan_gt_(path->bounds[1], path->bounds[3]));
      values[0] = path->bounds[0];
      values[1] = path->bounds[1];
      values[2] = path->bounds[2] - path->bounds[0];
      values[3] = path->bounds[3] - path->bounds[1];
   }

   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
   return true;
}

bool vgPathTransformedBounds_impl(
   VGPath vg_handle,
   VGfloat *values)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_PATH_T *path;

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return false;
   }

   path = (VG_PATH_T *)mem_lock(handle);

   if (!(path->caps & VG_PATH_CAPABILITY_PATH_TRANSFORMED_BOUNDS)) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return false;
   }

   if (!vg_path_read_tight_bounds_immediate(handle, path)) { /* todo: we don't actually need tight bounds here */
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return false;
   }

   if (path->bounds[0] > path->bounds[2]) {
      vcos_assert(path->bounds[1] > path->bounds[3]);
      values[0] = 0.0f;
      values[1] = 0.0f;
      values[2] = -1.0f;
      values[3] = -1.0f;
   } else {
      float transformed_bounds[4];
      vcos_assert(!nan_gt_(path->bounds[1], path->bounds[3]));
      get_transformed_bounds(transformed_bounds, path->bounds, &state->path_user_to_surface);
      values[0] = transformed_bounds[0];
      values[1] = transformed_bounds[1];
      values[2] = transformed_bounds[2] - transformed_bounds[0];
      values[3] = transformed_bounds[3] - transformed_bounds[1];
   }

   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
   return true;
}

void vgDrawPath_impl(
   VGPath vg_handle,
   VGbitfield paint_modes)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_PATH_T *path;
   uint32_t clip_rect[4];
   MEM_HANDLE_T scissor_handle;
   float clip[4], scale_max = 0.0f, stroke_clip[4];
   bool bounds_valid;
   float surface_bounds[4];

   if (!is_paint_modes(paint_modes)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path = (VG_PATH_T *)mem_lock(handle);
   if (path->flags & VG_PATH_FLAG_EMPTY) {
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }
   bounds_valid = vg_path_are_bounds_valid(path);
   if (bounds_valid) {
      get_transformed_bounds(surface_bounds, path->bounds, &state->path_user_to_surface);
   }
   if (!prepare_scissor(state->scissoring ? &state->scissor : NULL, clip_rect, &scissor_handle, clip)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }
   if ((clip_rect[2] == 0) || (clip_rect[3] == 0)) {
      paint_modes = 0;
   }
   if (paint_modes & VG_STROKE_PATH) {
      if (state->stroke.line_width > 0.0f) {
         scale_max = get_scale_max(&state->path_user_to_surface);
         get_stroke_clip(stroke_clip, clip, &state->stroke, scale_max);
         if (bounds_valid && !bounds_intersect(surface_bounds, stroke_clip)) {
            paint_modes = 0;
         }
      } else {
         paint_modes &= ~VG_STROKE_PATH;
      }
   }
   if ((paint_modes & VG_FILL_PATH) && bounds_valid && !bounds_intersect(surface_bounds, clip)) {
      paint_modes &= ~VG_FILL_PATH;
   }
   if (!paint_modes) {
      if (scissor_handle != MEM_INVALID_HANDLE) { mem_unretain(scissor_handle); }
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if (!vg_be_draw_path(state, handle, path, paint_modes, clip_rect, scissor_handle, clip, scale_max, stroke_clip)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
   }
   VG_UNLOCK_SERVER_STATE();
}

/******************************************************************************
api paint
******************************************************************************/

void vgCreatePaint_impl(
   VGHandle vg_handle)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   vcos_assert(is_object_ok(state, handle, OBJECT_TYPE_STEM));

   vg_paint_bprint_from_stem(handle); /* just create a blueprint for now -- we'll create the actual object when we need it */

   VG_UNLOCK_SERVER_STATE();
}

void vgDestroyPaint_impl(
   VGPaint vg_handle)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, (OBJECT_TYPE_T)(OBJECT_TYPE_PAINT_BPRINT | OBJECT_TYPE_PAINT))) { /* allow blueprints to be destroyed */
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   delete_object(state, handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgSetPaint_impl(
   VGPaint vg_handle,
   VGbitfield paint_modes)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   if (handle == MEM_INVALID_HANDLE) {
      handle = default_paint_handle;
   } else if (!check_object(state, handle, OBJECT_TYPE_PAINT)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if (paint_modes & VG_FILL_PATH) {
      mem_acquire(handle);
      mem_release(state->fill_paint);
      state->fill_paint = handle;
      state_changed(state, STATE_ELEMENT_FILL_PAINT);
   }
   if (paint_modes & VG_STROKE_PATH) {
      mem_acquire(handle);
      mem_release(state->stroke_paint);
      state->stroke_paint = handle;
      state_changed(state, STATE_ELEMENT_STROKE_PAINT);
   }

   VG_UNLOCK_SERVER_STATE();
}

void vgPaintPattern_impl(
   VGPaint vg_handle,
   VGImage pattern_vg_handle)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle, pattern_handle;
   VG_PAINT_T *paint;

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PAINT)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   pattern_handle = vg_handle_to_mem_handle(pattern_vg_handle);
   if (pattern_handle != MEM_INVALID_HANDLE) {
      if (!check_object(state, pattern_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
         VG_UNLOCK_SERVER_STATE();
         return;
      }

      mem_acquire(pattern_handle);
   }

   paint = (VG_PAINT_T *)mem_lock(handle);
   if (paint->pattern != MEM_INVALID_HANDLE) {
      mem_release(paint->pattern);
   }
   paint->pattern = pattern_handle;
   paint_changed(state, handle, paint, PAINT_ELEMENT_PATTERN);
   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

/******************************************************************************
api image
******************************************************************************/

void vgCreateImage_impl(
   VGHandle vg_handle,
   VGImageFormat format,
   VGint width, VGint height,
   VGbitfield allowed_quality)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   vcos_assert(is_object_ok(state, handle, OBJECT_TYPE_STEM));

   vg_image_bprint_from_stem(handle, format, allowed_quality, width, height); /* just create a blueprint for now -- we'll create the actual object when we need it */

   VG_UNLOCK_SERVER_STATE();
}

void vgDestroyImage_impl(
   VGImage vg_handle)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE_BPRINT | OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE_BPRINT | OBJECT_TYPE_CHILD_IMAGE))) { /* allow blueprints to be destroyed */
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   delete_object(state, handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgClearImage_impl(
   VGImage vg_handle,
   VGint x, VGint y,
   VGint width, VGint height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   uint32_t child_x, child_y, child_width, child_height;
   VG_IMAGE_T *image;

   if ((width <= 0) || (height <= 0)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   image = vg_image_lock_adam(&child_x, &child_y, &child_width, &child_height, &handle);

   khrn_clip_rect(&x, &y, &width, &height, 0, 0, child_width, child_height);

   vg_bf_clear_region(image->image, child_x + x, child_y + y, width, height, state->clear_rgba);

   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgImageSubData_impl(
   VGImage vg_handle,
   VGint dst_width, VGint dst_height,
   const void *data, VGint data_stride,
   VGImageFormat data_format,
   VGint src_x,
   VGint dst_x, VGint dst_y,
   VGint width, VGint height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   uint32_t child_x, child_y, child_width, child_height;
   VG_IMAGE_T *dst;
   KHRN_IMAGE_WRAP_T src_wrap;
   KHRN_IMAGE_T *dst_image;
   KHRN_IMAGE_WRAP_T dst_wrap;

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst = vg_image_lock_adam(&child_x, &child_y, &child_width, &child_height, &handle);

   if ((child_width != dst_width) ||
      (child_height != dst_height)) {
      /*
         this can happen if we destroy the image and reuse the handle when
         creating another image all before this vgImageSubData command gets to
         the server. an appropriate error is therefore bad handle
      */

      set_error(state, VG_BAD_HANDLE_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   khrn_image_wrap(&src_wrap, get_external_format(data_format), src_x + width, height, data_stride, (void *)data);

   /* src/dst rects clipped on client side */

   dst_image = (KHRN_IMAGE_T *)mem_lock(dst->image);
   khrn_interlock_write_immediate(&dst_image->interlock);
   khrn_image_lock_wrap(dst_image, &dst_wrap);
   khrn_image_wrap_copy_region(&dst_wrap, child_x + dst_x, child_y + dst_y, width, height, &src_wrap, src_x, 0, IMAGE_CONV_VG);
   khrn_image_unlock_wrap(dst_image);
   mem_unlock(dst->image);

   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

bool vgGetImageSubData_impl(
   VGImage vg_handle,
   VGint src_width, VGint src_height,
   void *data, VGint data_stride,
   VGImageFormat data_format,
   VGint dst_x,
   VGint src_x, VGint src_y,
   VGint width, VGint height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   uint32_t child_x, child_y, child_width, child_height;
   VG_IMAGE_T *src;
   KHRN_IMAGE_WRAP_T dst_wrap;
   KHRN_IMAGE_T *src_image;
   KHRN_IMAGE_WRAP_T src_wrap;

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
      VG_UNLOCK_SERVER_STATE();
      return false;
   }

   src = vg_image_lock_adam(&child_x, &child_y, &child_width, &child_height, &handle);

   if ((child_width != src_width) ||
      (child_height != src_height)) {
      /*
         this can happen if we destroy the image and reuse the handle when
         creating another image all before this vgImageSubData command gets to
         the server. an appropriate error is therefore bad handle
      */

      set_error(state, VG_BAD_HANDLE_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return false;
   }

   khrn_image_wrap(&dst_wrap, get_external_format(data_format), dst_x + width, height, data_stride, data);

   /* src/dst rects clipped on client side */

   src_image = (KHRN_IMAGE_T *)mem_lock(src->image);
   khrn_interlock_read_immediate(&src_image->interlock);
   khrn_image_lock_wrap(src_image, &src_wrap);
   khrn_image_wrap_copy_region(&dst_wrap, dst_x, 0, width, height, &src_wrap, child_x + src_x, child_y + src_y, IMAGE_CONV_VG);
   khrn_image_unlock_wrap(src_image);
   mem_unlock(src->image);

   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
   return true;
}

void vgChildImage_impl(
   VGHandle vg_handle,
   VGImage parent_vg_handle,
   VGint parent_width, VGint parent_height,
   VGint x, VGint y,
   VGint width, VGint height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle, parent_handle;
   uint32_t actual_parent_width = 0, actual_parent_height = 0;

   handle = vg_handle_to_mem_handle(vg_handle);
   vcos_assert(is_object_ok(state, handle, OBJECT_TYPE_STEM));

   parent_handle = vg_handle_to_mem_handle(parent_vg_handle);
   if (!check_object(state, parent_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE_BPRINT | OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE_BPRINT | OBJECT_TYPE_CHILD_IMAGE))) {
      delete_object(state, handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   switch (_msb(get_object_type(parent_handle))) {
   case OBJECT_TYPE_IMAGE_BPRINT_BI:
   {
      VG_IMAGE_BPRINT_T *parent = (VG_IMAGE_BPRINT_T *)mem_lock(parent_handle);
      actual_parent_width = parent->width;
      actual_parent_height = parent->height;
      mem_unlock(parent_handle);
      break;
   }
   case OBJECT_TYPE_IMAGE_BI:
   {
      VG_IMAGE_T *parent = (VG_IMAGE_T *)mem_lock(parent_handle);
      actual_parent_width = parent->image_width;
      actual_parent_height = parent->image_height;
      mem_unlock(parent_handle);
      break;
   }
   case OBJECT_TYPE_CHILD_IMAGE_BPRINT_BI:
   case OBJECT_TYPE_CHILD_IMAGE_BI:
   {
      VG_CHILD_IMAGE_T *parent = (VG_CHILD_IMAGE_T *)mem_lock(parent_handle);
      actual_parent_width = parent->width;
      actual_parent_height = parent->height;
      mem_unlock(parent_handle);
      break;
   }
   default:
   {
      UNREACHABLE();
   }
   }

   if ((actual_parent_width != parent_width) ||
      (actual_parent_height != parent_height)) {
      /*
         this can happen if we destroy the parent image and reuse the handle when
         creating another image all before this vgChildImage command gets to the
         server. an appropriate error is therefore bad handle
      */

      set_error(state, VG_BAD_HANDLE_ERROR);
      delete_object(state, handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   vg_child_image_bprint_from_stem(handle, parent_handle, x, y, width, height); /* just create a blueprint for now -- we'll create the actual object when we need it */

   VG_UNLOCK_SERVER_STATE();
}

VGImage vgGetParent_impl(
   VGImage vg_handle)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
      VG_UNLOCK_SERVER_STATE();
      return VG_INVALID_HANDLE;
   }

   if (vg_is_image(handle)) {
      /* no ancestors */
      VG_UNLOCK_SERVER_STATE();
      return vg_handle;
   } else {
      MEM_HANDLE_T ancestor_handle;

      vcos_assert(vg_is_child_image(handle));

      /*
         The vgGetParent function returns the closest valid ancestor (ie, one
         that has not been the target of a vgDestroyImage call) of the given
         image
      */

      ancestor_handle = ((VG_CHILD_IMAGE_T *)mem_lock(handle))->parent;
      mem_unlock(handle);
      for (;;) {
         if (vg_is_image(ancestor_handle)) {
            if (contains_object(state, ancestor_handle)) {
               vg_handle = vg_handle_from_mem_handle(ancestor_handle);
            }
            VG_UNLOCK_SERVER_STATE();
            return vg_handle;
         } else {
            VG_CHILD_IMAGE_T *ancestor;
            MEM_HANDLE_T ancestor_parent_handle;
            vcos_assert(vg_is_child_image(ancestor_handle));
            if (contains_object(state, ancestor_handle)) {
               VG_UNLOCK_SERVER_STATE();
               return vg_handle_from_mem_handle(ancestor_handle);
            }
            ancestor = (VG_CHILD_IMAGE_T *)mem_lock(ancestor_handle);
            ancestor_parent_handle = ancestor->parent;
            mem_unlock(ancestor_handle);
            ancestor_handle = ancestor_parent_handle;
         }
      }
   }

   /* can't get here */
}

void vgCopyImage_impl(
   VGImage dst_vg_handle,
   VGint dst_x, VGint dst_y,
   VGImage src_vg_handle,
   VGint src_x, VGint src_y,
   VGint width, VGint height,
   bool dither)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T dst_handle, src_handle;
   uint32_t dst_child_x, dst_child_y, dst_child_width, dst_child_height;
   uint32_t src_child_x, src_child_y, src_child_width, src_child_height;
   VG_IMAGE_T *dst, *src;

   UNUSED(dither);

   if ((width <= 0) || (height <= 0)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst_handle = vg_handle_to_mem_handle(dst_vg_handle);
   src_handle = vg_handle_to_mem_handle(src_vg_handle);
   if (!check_object(state, dst_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE)) ||
      !check_object(state, src_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst = vg_image_lock_adam(&dst_child_x, &dst_child_y, &dst_child_width, &dst_child_height, &dst_handle);
   src = vg_image_lock_adam(&src_child_x, &src_child_y, &src_child_width, &src_child_height, &src_handle);

   khrn_clip_rect2(
      &dst_x, &dst_y, &src_x, &src_y, &width, &height,
      0, 0, dst_child_width, dst_child_height,
      0, 0, src_child_width, src_child_height);

   vg_bf_copy_region(dst->image, dst_child_x + dst_x, dst_child_y + dst_y, width, height, src->image, src_child_x + src_x, src_child_y + src_y);
   /* todo: dither? */

   mem_unlock(src_handle);
   mem_unlock(dst_handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgDrawImage_impl(
   VGImage vg_handle)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   uint32_t child_x, child_y, child_width, child_height;
   float p[8];
   uint32_t i;
   float surface_bounds[4];
   uint32_t clip_rect[4];
   MEM_HANDLE_T scissor_handle;
   float clip[4];

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   vg_image_lock_adam(&child_x, &child_y, &child_width, &child_height, &handle);
   mem_unlock(handle);

   p[0] = 0.0f;
   p[1] = 0.0f;
   p[2] = (float)child_width;
   p[3] = 0.0f;
   p[4] = (float)child_width;
   p[5] = (float)child_height;
   p[6] = 0.0f;
   p[7] = (float)child_height;

   for (i = 0; i != 4; ++i) {
      VG_MAT3X3_T *a = &state->image_user_to_surface;
      float x = (a->m[0][0] * p[i * 2]) + (a->m[0][1] * p[(i * 2) + 1]) + a->m[0][2];
      float y = (a->m[1][0] * p[i * 2]) + (a->m[1][1] * p[(i * 2) + 1]) + a->m[1][2];
      float d = (a->m[2][0] * p[i * 2]) + (a->m[2][1] * p[(i * 2) + 1]) + a->m[2][2];
      float oo_d;
      if (d < EPS) {
         /*
            spec says d must be positive at each corner or nothing is drawn
         */

         VG_UNLOCK_SERVER_STATE();
         return;
      }
      oo_d = recip_(d);
      p[(i * 2) + 0] = x * oo_d;
      p[(i * 2) + 1] = y * oo_d;
   }

   surface_bounds[0] = _minf(_minf(_minf(p[0], p[2]), p[4]), p[6]);
   surface_bounds[1] = _minf(_minf(_minf(p[1], p[3]), p[5]), p[7]);
   surface_bounds[2] = _maxf(_maxf(_maxf(p[0], p[2]), p[4]), p[6]);
   surface_bounds[3] = _maxf(_maxf(_maxf(p[1], p[3]), p[5]), p[7]);
   if (!prepare_scissor(state->scissoring ? &state->scissor : NULL, clip_rect, &scissor_handle, clip)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }
   if ((clip_rect[2] == 0) || (clip_rect[3] == 0) ||
      !bounds_intersect(surface_bounds, clip)) {
      if (scissor_handle != MEM_INVALID_HANDLE) { mem_unretain(scissor_handle); }
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if (!vg_be_draw_image(state, handle, child_x, child_y, child_width, child_height, p, clip_rect, scissor_handle)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
   }
   VG_UNLOCK_SERVER_STATE();
}

/******************************************************************************
api framebuffer
******************************************************************************/

void vgSetPixels_impl(
   VGint dst_x, VGint dst_y,
   VGImage src_vg_handle,
   VGint src_x, VGint src_y,
   VGint width, VGint height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T src_handle;
   uint32_t src_child_x, src_child_y, src_child_width, src_child_height;
   VG_IMAGE_T *src;

   if ((width <= 0) || (height <= 0)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   src_handle = vg_handle_to_mem_handle(src_vg_handle);
   if (!check_object(state, src_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   src = vg_image_lock_adam(&src_child_x, &src_child_y, &src_child_width, &src_child_height, &src_handle);

   khrn_clip_rect2(
      &dst_x, &dst_y, &src_x, &src_y, &width, &height,
      0, 0, frame_width, frame_height,
      0, 0, src_child_width, src_child_height);

   if (state->scissoring) {
      vg_bf_copy_scissor_regions(frame_handle, dst_x, dst_y, width, height, src->image, src_child_x + src_x, src_child_y + src_y,
         state->scissor.rects, state->scissor.rects_count);
   } else {
      vg_bf_copy_region(frame_handle, dst_x, dst_y, width, height, src->image, src_child_x + src_x, src_child_y + src_y);
   }

   mem_unlock(src_handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgWritePixels_impl(
   const void *data, VGint data_stride,
   VGImageFormat data_format,
   VGint src_x,
   VGint dst_x, VGint dst_y,
   VGint width, VGint height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   KHRN_IMAGE_WRAP_T src_wrap;
   KHRN_IMAGE_T *dst;
   KHRN_IMAGE_WRAP_T dst_wrap;

   khrn_image_wrap(&src_wrap, get_external_format(data_format), src_x + width, height, data_stride, (void *)data);

   /* src/dst rects clipped on client side */

   dst = (KHRN_IMAGE_T *)mem_lock(frame_handle);
   khrn_interlock_write_immediate(&dst->interlock);
   khrn_image_lock_wrap(dst, &dst_wrap);
   if (state->scissoring) {
      khrn_image_wrap_copy_scissor_regions(&dst_wrap, dst_x, dst_y, width, height, &src_wrap, src_x, 0, IMAGE_CONV_VG,
         state->scissor.rects, state->scissor.rects_count);
   } else {
      khrn_image_wrap_copy_region(&dst_wrap, dst_x, dst_y, width, height, &src_wrap, src_x, 0, IMAGE_CONV_VG);
   }
   khrn_image_unlock_wrap(dst);
   mem_unlock(frame_handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgGetPixels_impl(
   VGImage dst_vg_handle,
   VGint dst_x, VGint dst_y,
   VGint src_x, VGint src_y,
   VGint width, VGint height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T dst_handle;
   uint32_t dst_child_x, dst_child_y, dst_child_width, dst_child_height;
   VG_IMAGE_T *dst;

   if ((width <= 0) || (height <= 0)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst_handle = vg_handle_to_mem_handle(dst_vg_handle);
   if (!check_object(state, dst_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst = vg_image_lock_adam(&dst_child_x, &dst_child_y, &dst_child_width, &dst_child_height, &dst_handle);

   khrn_clip_rect2(
      &dst_x, &dst_y, &src_x, &src_y, &width, &height,
      0, 0, dst_child_width, dst_child_height,
      0, 0, frame_width, frame_height);

   vg_bf_copy_region(dst->image, dst_child_x + dst_x, dst_child_y + dst_y, width, height, frame_handle, src_x, src_y);

   mem_unlock(dst_handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgReadPixels_impl(
   void *data, VGint data_stride,
   VGImageFormat data_format,
   VGint dst_x,
   VGint src_x, VGint src_y,
   VGint width, VGint height)
{
   KHRN_IMAGE_WRAP_T dst_wrap;
   KHRN_IMAGE_T *src;
   KHRN_IMAGE_WRAP_T src_wrap;

   khrn_image_wrap(&dst_wrap, get_external_format(data_format), dst_x + width, height, data_stride, data);

   /* src/dst rects clipped on client side */

   src = (KHRN_IMAGE_T *)mem_lock(frame_handle);
   khrn_interlock_read_immediate(&src->interlock);
   khrn_image_lock_wrap(src, &src_wrap);
   khrn_image_wrap_copy_region(&dst_wrap, dst_x, 0, width, height, &src_wrap, src_x, src_y, IMAGE_CONV_VG);
   khrn_image_unlock_wrap(src);
   mem_unlock(frame_handle);
}

void vgCopyPixels_impl(
   VGint dst_x, VGint dst_y,
   VGint src_x, VGint src_y,
   VGint width, VGint height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   if ((width <= 0) || (height <= 0)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   khrn_clip_rect2(
      &dst_x, &dst_y, &src_x, &src_y, &width, &height,
      0, 0, frame_width, frame_height,
      0, 0, frame_width, frame_height);

   if (state->scissoring) {
      vg_bf_copy_scissor_regions(frame_handle, dst_x, dst_y, width, height, frame_handle, src_x, src_y,
         state->scissor.rects, state->scissor.rects_count);
   } else {
      vg_bf_copy_region(frame_handle, dst_x, dst_y, width, height, frame_handle, src_x, src_y);
   }

   VG_UNLOCK_SERVER_STATE();
}

/******************************************************************************
api fonts
******************************************************************************/

void vgCreateFont_impl(
   VGHandle vg_handle,
   VGint glyphs_capacity)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   vcos_assert(is_object_ok(state, handle, OBJECT_TYPE_STEM));

   vg_font_bprint_from_stem(handle, glyphs_capacity); /* just create a blueprint for now -- we'll create the actual object when we need it */

   VG_UNLOCK_SERVER_STATE();
}

void vgDestroyFont_impl(
   VGFont vg_handle)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, (OBJECT_TYPE_T)(OBJECT_TYPE_FONT_BPRINT | OBJECT_TYPE_FONT))) { /* allow blueprints to be destroyed */
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   delete_object(state, handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgSetGlyphToPath_impl(
   VGFont vg_handle,
   VGuint glyph_id,
   VGPath path_vg_handle, bool is_hinted,
   VGfloat glyph_origin_x, VGfloat glyph_origin_y,
   VGfloat escapement_x, VGfloat escapement_y)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle, path_handle;
   VG_FONT_T *font;

   handle = vg_handle_to_mem_handle(vg_handle);
   path_handle = vg_handle_to_mem_handle(path_vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_FONT) ||
      ((path_handle != MEM_INVALID_HANDLE) && !check_object(state, path_handle, OBJECT_TYPE_PATH))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   font = (VG_FONT_T *)mem_lock(handle);
   if (!vg_font_insert(font,
      glyph_id,
      path_handle, !is_hinted,
      glyph_origin_x, glyph_origin_y,
      escapement_x, escapement_y)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
   }
   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgSetGlyphToImage_impl(
   VGFont vg_handle,
   VGuint glyph_id,
   VGImage image_vg_handle,
   VGfloat glyph_origin_x, VGfloat glyph_origin_y,
   VGfloat escapement_x, VGfloat escapement_y)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle, image_handle;
   VG_FONT_T *font;

   handle = vg_handle_to_mem_handle(vg_handle);
   image_handle = vg_handle_to_mem_handle(image_vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_FONT) ||
      ((image_handle != MEM_INVALID_HANDLE) && !check_object(state, image_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE)))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   font = (VG_FONT_T *)mem_lock(handle);
   if (!vg_font_insert(font,
      glyph_id,
      image_handle, false,
      glyph_origin_x, glyph_origin_y,
      escapement_x, escapement_y)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
   }
   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgClearGlyph_impl(
   VGFont vg_handle,
   VGuint glyph_id)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_FONT_T *font;

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_FONT)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   font = (VG_FONT_T *)mem_lock(handle);
   if (!vg_font_delete(font, glyph_id)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
   }
   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

typedef struct {
   uint32_t clip_rect[4];
   MEM_HANDLE_T scissor_handle;
   float scale_max;
   float clip[4];
   float stroke_clip[4];
   float clip_tx[4];
   float stroke_clip_tx[4];

} STATE_DERIVED_T;

static bool state_derive(
   STATE_DERIVED_T *derived,
   VG_SERVER_STATE_T *state,
   VG_MAT3X3_T *t)
{
   VG_MAT3X3_T i;
   if (!prepare_scissor(state->scissoring ? &state->scissor : NULL, derived->clip_rect, &derived->scissor_handle, derived->clip)) {
      return false;
   }
   derived->scale_max = get_scale_max(&state->glyph_user_to_surface);
   get_stroke_clip(derived->stroke_clip, derived->clip, &state->stroke, derived->scale_max);

   /* transform the bounds with the inverse user transformation */
   i = *t;
   if (!vg_mat3x3_try_invert(&i)) {
      return false;
   }
   get_transformed_bounds (derived->clip_tx, derived->clip, &i);
   get_transformed_bounds (derived->stroke_clip_tx, derived->stroke_clip, &i);

   return true;
}

static bool draw_glyph_path(
   VG_SERVER_STATE_T *state, const STATE_DERIVED_T *derived,
   float tr_x, float tr_y,
   MEM_HANDLE_T handle,
   uint32_t paint_modes)
{
   static uint32_t draw_count = 0;

   VG_PATH_T *path;
   if (!paint_modes) {
      return true;
   }
   path = (VG_PATH_T *)mem_lock(handle);
   if (path->flags & VG_PATH_FLAG_EMPTY) {
      mem_unlock(handle);
      return true;
   }

   /* reduce bounds check frequency when we previously drew something */
   if (!(draw_count & 3)) {
      if (vg_path_are_bounds_valid(path)) {
         float path_bounds_offset[4];
         get_offset_bounds(path_bounds_offset, path->bounds, tr_x, tr_y);
         if ((paint_modes & VG_STROKE_PATH) && !bounds_intersect(path_bounds_offset, derived->stroke_clip_tx)) {
            paint_modes = 0;
         }
         if ((paint_modes & VG_FILL_PATH) && !bounds_intersect(path_bounds_offset, derived->clip_tx)) {
            paint_modes &= ~VG_FILL_PATH;
         }
      }
   }
   if (!paint_modes) {
      mem_unlock(handle);
      draw_count = 0;
      return true;
   }
   if (first_glyph) {
      if (!vg_be_draw_glyph_path_init(state, paint_modes, derived->clip_rect, derived->scissor_handle)) {
         mem_unlock(handle);
         return false;
      }
      first_glyph = false;
   }

   draw_count += 1;
   return vg_be_draw_glyph_path(state, handle, path, paint_modes, tr_x, tr_y, derived->clip_rect, derived->scissor_handle, derived->clip, derived->scale_max);
}

static bool draw_glyph_image(
   VG_SERVER_STATE_T *state, const STATE_DERIVED_T *derived,
   float tr_x, float tr_y,
   MEM_HANDLE_T handle)
{
   uint32_t child_x, child_y, child_width, child_height;
   float p[8];
   VG_MAT3X3_T a;
   uint32_t i;
   float surface_bounds[4];

   vg_image_lock_adam(&child_x, &child_y, &child_width, &child_height, &handle);
   mem_unlock(handle);

   p[0] = 0.0f;
   p[1] = 0.0f;
   p[2] = (float)child_width;
   p[3] = 0.0f;
   p[4] = (float)child_width;
   p[5] = (float)child_height;
   p[6] = 0.0f;
   p[7] = (float)child_height;

   a = state->glyph_user_to_surface;
   vg_mat3x3_postmul_translate(&a, tr_x, tr_y);
   for (i = 0; i != 4; ++i) {
      vg_mat3x3_affine_transform(&a, p + (i * 2), p + (i * 2) + 1);
   }

   surface_bounds[0] = _minf(_minf(_minf(p[0], p[2]), p[4]), p[6]);
   surface_bounds[1] = _minf(_minf(_minf(p[1], p[3]), p[5]), p[7]);
   surface_bounds[2] = _maxf(_maxf(_maxf(p[0], p[2]), p[4]), p[6]);
   surface_bounds[3] = _maxf(_maxf(_maxf(p[1], p[3]), p[5]), p[7]);
   if (!bounds_intersect(surface_bounds, derived->clip)) {
      return true;
   }

   first_glyph = true; /* force reload of path shaders for mixed image/path fonts */
   return vg_be_draw_glyph_image(state, handle, child_x, child_y, child_width, child_height, tr_x, tr_y, p, derived->clip_rect, derived->scissor_handle);
}

static bool draw_glyph(
   VG_SERVER_STATE_T *state, const STATE_DERIVED_T *derived,
   VG_GLYPH_T *glyph,
   uint32_t paint_modes)
{
   float tr_x = state->glyph_origin[0] - glyph->origin[0]; /* glyph_origin clean at this point */
   float tr_y = state->glyph_origin[1] - glyph->origin[1];
   state->glyph_origin[0] += glyph->escapement[0];
   state->glyph_origin[1] += glyph->escapement[1];

   vcos_assert((glyph->object == MEM_INVALID_HANDLE) || ((OBJECT_TYPE_PATH | OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE) & get_object_type(glyph->object)));
   return (glyph->object == MEM_INVALID_HANDLE) || (vg_is_path(glyph->object) ?
      draw_glyph_path(state, derived, tr_x, tr_y, glyph->object, paint_modes) :
      draw_glyph_image(state, derived, tr_x, tr_y, glyph->object));
}

void vgDrawGlyph_impl(
   VGFont vg_handle,
   VGuint glyph_id,
   VGbitfield paint_modes,
   bool allow_autohinting)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_FONT_T *font;
   VG_FONT_LOCKED_T font_locked;
   VG_GLYPH_T *glyph;

   UNUSED(allow_autohinting);

   if (paint_modes && !is_paint_modes(paint_modes)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_FONT)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   font = (VG_FONT_T *)mem_lock(handle);
   vg_font_lock(font, &font_locked);

   first_glyph = true;
   glyph = vg_font_locked_lookup(&font_locked, glyph_id);
   if (glyph) {
      STATE_DERIVED_T derived;
      if (state->stroke.line_width <= 0.0f) {
         paint_modes &= ~VG_STROKE_PATH;
      }
      if (state_derive(&derived, state, &state->glyph_user_to_surface)) {
         if ((derived.clip_rect[2] != 0) && (derived.clip_rect[3] != 0)) {
            state->glyph_origin[0] = clean_float(state->glyph_origin[0]);
            state->glyph_origin[1] = clean_float(state->glyph_origin[1]);
            if (!draw_glyph(state, &derived, glyph, paint_modes)) {
               set_error(state, VG_OUT_OF_MEMORY_ERROR);
            }
         }
         if (derived.scissor_handle != MEM_INVALID_HANDLE) { mem_unretain(derived.scissor_handle); }
      } else {
         set_error(state, VG_OUT_OF_MEMORY_ERROR);
      }
   } else {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
   }

   vg_font_unlock(font);
   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgDrawGlyphs_impl(
   VGFont vg_handle,
   VGint glyphs_count, const VGuint *glyph_ids,
   const VGfloat *adjustments_x, const VGfloat *adjustments_y,
   VGbitfield paint_modes,
   bool allow_autohinting)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_FONT_T *font;
   VG_FONT_LOCKED_T font_locked;
   uint32_t i;
   STATE_DERIVED_T derived;

   UNUSED(allow_autohinting);

   if (paint_modes && !is_paint_modes(paint_modes)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_FONT)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   font = (VG_FONT_T *)mem_lock(handle);
   vg_font_lock(font, &font_locked);

   for (i = 0; i != glyphs_count; ++i) {
      if (!vg_font_locked_lookup(&font_locked, glyph_ids[i])) {
         set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
         vg_font_unlock(font);
         mem_unlock(handle);
         VG_UNLOCK_SERVER_STATE();
         return;
      }
   }

   first_glyph = true;
   if (state->stroke.line_width <= 0.0f) {
      paint_modes &= ~VG_STROKE_PATH;
   }
   if (state_derive(&derived, state, &state->glyph_user_to_surface)) {
      if ((derived.clip_rect[2] != 0) && (derived.clip_rect[3] != 0)) {
         if (glyphs_count != 0) {
            state->glyph_origin[0] = clean_float(state->glyph_origin[0]);
            state->glyph_origin[1] = clean_float(state->glyph_origin[1]);
         }
         for (i = 0; i != glyphs_count; ++i) {
            if (!draw_glyph(state, &derived, vg_font_locked_lookup(&font_locked, glyph_ids[i]), paint_modes)) {
               set_error(state, VG_OUT_OF_MEMORY_ERROR);
               break;
            }
            if (adjustments_x) { state->glyph_origin[0] += clean_float(adjustments_x[i]); } /* glyph_origin clean at this point */
            if (adjustments_y) { state->glyph_origin[1] += clean_float(adjustments_y[i]); }
         }
      }
      if (derived.scissor_handle != MEM_INVALID_HANDLE) { mem_unretain(derived.scissor_handle); }
   } else {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
   }

   vg_font_unlock(font);
   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

/******************************************************************************
api filters
******************************************************************************/

static uint32_t get_filter_channel_mask(VG_SERVER_STATE_T *state, KHRN_IMAGE_FORMAT_T format)
{
   uint32_t channel_mask;
   vcos_assert(khrn_image_is_color(format));
   channel_mask = state->filter_channel_mask;
   if (!(format & IMAGE_FORMAT_RGB)) { channel_mask |= VG_RED | VG_GREEN | VG_BLUE; }
   if (!(format & IMAGE_FORMAT_A)) { channel_mask |= VG_ALPHA; }
   return _bitrev(channel_mask, 4);
}

static INLINE KHRN_IMAGE_FORMAT_T get_filter_format(VG_SERVER_STATE_T *state)
{
   return (KHRN_IMAGE_FORMAT_T)((state->filter_format_pre ? IMAGE_FORMAT_PRE : 0) | (state->filter_format_linear ? IMAGE_FORMAT_LIN : 0));
}

static INLINE KHRN_IMAGE_FORMAT_T get_filter_output_format(bool output_pre, bool output_linear)
{
   return (KHRN_IMAGE_FORMAT_T)((output_pre ? IMAGE_FORMAT_PRE : 0) | (output_linear ? IMAGE_FORMAT_LIN : 0));
}

static uint32_t get_filter_tile_fill_rgba(VG_SERVER_STATE_T *state)
{
   uint32_t rgba = state->tile_fill_rgba;
   if (state->filter_format_linear) {
      rgba = khrn_color_rgba_s_to_lin(rgba);
   }
   if (state->filter_format_pre) {
      rgba = khrn_color_rgba_pre(rgba);
   }
   return rgba;
}

static VGImageChannel clean_image_channel(VGImageChannel image_channel, KHRN_IMAGE_FORMAT_T format)
{
   vcos_assert(khrn_image_is_color(format));
   if ((format & IMAGE_FORMAT_L) && !(format & IMAGE_FORMAT_A)) { image_channel = VG_RED; }
   if (!(format & (IMAGE_FORMAT_RGB | IMAGE_FORMAT_L)) && (format & IMAGE_FORMAT_A)) { image_channel = VG_ALPHA; }
   return image_channel;
}

/* convert large shift values to equivalent smaller shift values so we don't
 * have to worry about overflow */
static int32_t nice_shift(int32_t shift,
   uint32_t dst_l, uint32_t src_l, uint32_t kernel_l,
   VGTilingMode tiling_mode)
{
   switch (tiling_mode) {
   case VG_TILE_FILL:
   case VG_TILE_PAD:
   {
      /* the range of virtual source addresses is 0 to dst_l + kernel_l - 2. if
       * shift is <= -src_l, we'll always be off to the right of the source
       * image. if shift is > (dst_l + kernel_l - 2), we'll always be off to the
       * left of the source image */
      return clampi(shift, -(int32_t)src_l, dst_l + kernel_l - 1);
   }
   case VG_TILE_REPEAT:
   case VG_TILE_REFLECT:
   {
      return mod(shift, 2 * src_l);
   }
   default:
   {
      UNREACHABLE();
      return 0;
   }
   }
}

void vgColorMatrix_impl(
   VGImage dst_vg_handle,
   VGImage src_vg_handle,
   const VGfloat *matrix)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T dst_handle, src_handle;
   uint32_t dst_child_x, dst_child_y, dst_child_width, dst_child_height;
   uint32_t src_child_x, src_child_y, src_child_width, src_child_height;
   VG_IMAGE_T *dst, *src;

   dst_handle = vg_handle_to_mem_handle(dst_vg_handle);
   src_handle = vg_handle_to_mem_handle(src_vg_handle);
   if (!check_object(state, dst_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE)) ||
      !check_object(state, src_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst = vg_image_lock_adam(&dst_child_x, &dst_child_y, &dst_child_width, &dst_child_height, &dst_handle);
   src = vg_image_lock_adam(&src_child_x, &src_child_y, &src_child_width, &src_child_height, &src_handle);

   if (vg_image_share_storage(dst, src) && khrn_rects_intersect(
      dst_child_x, dst_child_y, dst_child_width, dst_child_height,
      src_child_x, src_child_y, src_child_width, src_child_height)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      mem_unlock(src_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst_child_width = _min(dst_child_width, src_child_width);
   dst_child_height = _min(dst_child_height, src_child_height);

   vg_bf_color_matrix(
      dst->image, dst_child_x, dst_child_y,
      dst_child_width, dst_child_height,
      src->image, src_child_x, src_child_y,
      get_filter_channel_mask(state, dst->image_format), get_filter_format(state),
      matrix);

   mem_unlock(src_handle);
   mem_unlock(dst_handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgConvolve_impl(
   VGImage dst_vg_handle,
   VGImage src_vg_handle,
   VGint kernel_width, VGint kernel_height,
   VGint shift_x, VGint shift_y,
   VGfloat scale, VGfloat bias,
   VGTilingMode tiling_mode,
   const VGshort *kernel)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T dst_handle, src_handle;
   uint32_t dst_child_x, dst_child_y, dst_child_width, dst_child_height;
   uint32_t src_child_x, src_child_y, src_child_width, src_child_height;
   VG_IMAGE_T *dst, *src;

   if (!is_tiling_mode(tiling_mode)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst_handle = vg_handle_to_mem_handle(dst_vg_handle);
   src_handle = vg_handle_to_mem_handle(src_vg_handle);
   if (!check_object(state, dst_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE)) ||
      !check_object(state, src_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst = vg_image_lock_adam(&dst_child_x, &dst_child_y, &dst_child_width, &dst_child_height, &dst_handle);
   src = vg_image_lock_adam(&src_child_x, &src_child_y, &src_child_width, &src_child_height, &src_handle);

   if (vg_image_share_storage(dst, src) && khrn_rects_intersect(
      dst_child_x, dst_child_y, dst_child_width, dst_child_height,
      src_child_x, src_child_y, src_child_width, src_child_height)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      mem_unlock(src_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst_child_width = _min(dst_child_width, src_child_width);
   dst_child_height = _min(dst_child_height, src_child_height);

   shift_x = nice_shift(shift_x, dst_child_width, src_child_width, kernel_width, tiling_mode);
   shift_y = nice_shift(shift_y, dst_child_height, src_child_height, kernel_height, tiling_mode);

   vg_bf_conv(
      dst->image, dst_child_x, dst_child_y, dst_child_width, dst_child_height,
      src->image, src_child_x, src_child_y, src_child_width, src_child_height,
      get_filter_channel_mask(state, dst->image_format), get_filter_format(state),
      tiling_mode, get_filter_tile_fill_rgba(state),
      shift_x, shift_y,
      kernel, kernel_width, kernel_height,
      scale, bias);

   mem_unlock(src_handle);
   mem_unlock(dst_handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgSeparableConvolve_impl(
   VGImage dst_vg_handle,
   VGImage src_vg_handle,
   VGint kernel_width, VGint kernel_height,
   VGint shift_x, VGint shift_y,
   const VGshort *kernel_x, const VGshort *kernel_y,
   VGfloat scale, VGfloat bias,
   VGTilingMode tiling_mode)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T dst_handle, src_handle;
   uint32_t dst_child_x, dst_child_y, dst_child_width, dst_child_height;
   uint32_t src_child_x, src_child_y, src_child_width, src_child_height;
   VG_IMAGE_T *dst, *src;

   if (!is_tiling_mode(tiling_mode)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst_handle = vg_handle_to_mem_handle(dst_vg_handle);
   src_handle = vg_handle_to_mem_handle(src_vg_handle);
   if (!check_object(state, dst_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE)) ||
      !check_object(state, src_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst = vg_image_lock_adam(&dst_child_x, &dst_child_y, &dst_child_width, &dst_child_height, &dst_handle);
   src = vg_image_lock_adam(&src_child_x, &src_child_y, &src_child_width, &src_child_height, &src_handle);

   if (vg_image_share_storage(dst, src) && khrn_rects_intersect(
      dst_child_x, dst_child_y, dst_child_width, dst_child_height,
      src_child_x, src_child_y, src_child_width, src_child_height)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      mem_unlock(src_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst_child_width = _min(dst_child_width, src_child_width);
   dst_child_height = _min(dst_child_height, src_child_height);

   shift_x = nice_shift(shift_x, dst_child_width, src_child_width, kernel_width, tiling_mode);
   shift_y = nice_shift(shift_y, dst_child_height, src_child_height, kernel_height, tiling_mode);

   vg_bf_sconv(
      dst->image, dst_child_x, dst_child_y, dst_child_width, dst_child_height,
      src->image, src_child_x, src_child_y, src_child_width, src_child_height,
      get_filter_channel_mask(state, dst->image_format), get_filter_format(state),
      tiling_mode, get_filter_tile_fill_rgba(state),
      shift_x, shift_y,
      kernel_x, kernel_width, kernel_y, kernel_height,
      scale, bias,
      VG_GET_SERVER_STATE_HANDLE());

   mem_unlock(src_handle);
   mem_unlock(dst_handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgGaussianBlur_impl(
   VGImage dst_vg_handle,
   VGImage src_vg_handle,
   VGfloat std_dev_x, VGfloat std_dev_y,
   VGTilingMode tiling_mode)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T dst_handle, src_handle;
   uint32_t dst_child_x, dst_child_y, dst_child_width, dst_child_height;
   uint32_t src_child_x, src_child_y, src_child_width, src_child_height;
   VG_IMAGE_T *dst, *src;

   if (is_le_zero(std_dev_x) || is_le_zero(std_dev_y) ||
      (std_dev_x > VG_CONFIG_MAX_GAUSSIAN_STD_DEVIATION) || (std_dev_y > VG_CONFIG_MAX_GAUSSIAN_STD_DEVIATION) ||
      !is_tiling_mode(tiling_mode)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst_handle = vg_handle_to_mem_handle(dst_vg_handle);
   src_handle = vg_handle_to_mem_handle(src_vg_handle);
   if (!check_object(state, dst_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE)) ||
      !check_object(state, src_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst = vg_image_lock_adam(&dst_child_x, &dst_child_y, &dst_child_width, &dst_child_height, &dst_handle);
   src = vg_image_lock_adam(&src_child_x, &src_child_y, &src_child_width, &src_child_height, &src_handle);

   if (vg_image_share_storage(dst, src) && khrn_rects_intersect(
      dst_child_x, dst_child_y, dst_child_width, dst_child_height,
      src_child_x, src_child_y, src_child_width, src_child_height)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      mem_unlock(src_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst_child_width = _min(dst_child_width, src_child_width);
   dst_child_height = _min(dst_child_height, src_child_height);

   vg_bf_gblur(
      dst->image, dst_child_x, dst_child_y, dst_child_width, dst_child_height,
      src->image, src_child_x, src_child_y, src_child_width, src_child_height,
      get_filter_channel_mask(state, dst->image_format), get_filter_format(state),
      tiling_mode, get_filter_tile_fill_rgba(state),
      std_dev_x, std_dev_y,
      VG_GET_SERVER_STATE_HANDLE());

   mem_unlock(src_handle);
   mem_unlock(dst_handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgLookup_impl(
   VGImage dst_vg_handle,
   VGImage src_vg_handle,
   const VGubyte *red_lut,
   const VGubyte *green_lut,
   const VGubyte *blue_lut,
   const VGubyte *alpha_lut,
   bool output_linear,
   bool output_pre)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T dst_handle, src_handle;
   uint32_t dst_child_x, dst_child_y, dst_child_width, dst_child_height;
   uint32_t src_child_x, src_child_y, src_child_width, src_child_height;
   VG_IMAGE_T *dst, *src;

   dst_handle = vg_handle_to_mem_handle(dst_vg_handle);
   src_handle = vg_handle_to_mem_handle(src_vg_handle);
   if (!check_object(state, dst_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE)) ||
      !check_object(state, src_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst = vg_image_lock_adam(&dst_child_x, &dst_child_y, &dst_child_width, &dst_child_height, &dst_handle);
   src = vg_image_lock_adam(&src_child_x, &src_child_y, &src_child_width, &src_child_height, &src_handle);

   if (vg_image_share_storage(dst, src) && khrn_rects_intersect(
      dst_child_x, dst_child_y, dst_child_width, dst_child_height,
      src_child_x, src_child_y, src_child_width, src_child_height)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      mem_unlock(src_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst_child_width = _min(dst_child_width, src_child_width);
   dst_child_height = _min(dst_child_height, src_child_height);

   vg_bf_lookup(
      dst->image, dst_child_x, dst_child_y,
      dst_child_width, dst_child_height,
      src->image, src_child_x, src_child_y,
      get_filter_channel_mask(state, dst->image_format), get_filter_format(state),
      red_lut, green_lut, blue_lut, alpha_lut,
      get_filter_output_format(output_pre, output_linear));

   mem_unlock(src_handle);
   mem_unlock(dst_handle);

   VG_UNLOCK_SERVER_STATE();
}

void vgLookupSingle_impl(
   VGImage dst_vg_handle,
   VGImage src_vg_handle,
   VGImageChannel source_channel,
   bool output_linear,
   bool output_pre,
   const VGuint *lut)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T dst_handle, src_handle;
   uint32_t dst_child_x, dst_child_y, dst_child_width, dst_child_height;
   uint32_t src_child_x, src_child_y, src_child_width, src_child_height;
   VG_IMAGE_T *dst, *src;

   dst_handle = vg_handle_to_mem_handle(dst_vg_handle);
   src_handle = vg_handle_to_mem_handle(src_vg_handle);
   if (!check_object(state, dst_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE)) ||
      !check_object(state, src_handle, (OBJECT_TYPE_T)(OBJECT_TYPE_IMAGE | OBJECT_TYPE_CHILD_IMAGE))) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst = vg_image_lock_adam(&dst_child_x, &dst_child_y, &dst_child_width, &dst_child_height, &dst_handle);
   src = vg_image_lock_adam(&src_child_x, &src_child_y, &src_child_width, &src_child_height, &src_handle);

   source_channel = clean_image_channel(source_channel, src->image_format);

   if ((vg_image_share_storage(dst, src) && khrn_rects_intersect(
      dst_child_x, dst_child_y, dst_child_width, dst_child_height,
      src_child_x, src_child_y, src_child_width, src_child_height)) ||
      !is_image_channel(source_channel)) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      mem_unlock(src_handle);
      mem_unlock(dst_handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   dst_child_width = _min(dst_child_width, src_child_width);
   dst_child_height = _min(dst_child_height, src_child_height);

   vg_bf_lookup_single(
      dst->image, dst_child_x, dst_child_y,
      dst_child_width, dst_child_height,
      src->image, src_child_x, src_child_y,
      get_filter_channel_mask(state, dst->image_format), get_filter_format(state),
      3 - _msb(source_channel), lut,
      get_filter_output_format(output_pre, output_linear));

   mem_unlock(src_handle);
   mem_unlock(dst_handle);

   VG_UNLOCK_SERVER_STATE();
}

/******************************************************************************
api vgu
******************************************************************************/

static bool append(
   VG_PATH_T *path,
   const uint8_t *segments, uint32_t segments_count,
   const float *coords, uint32_t coords_count)
{
   uint32_t path_coords_size;
   void *path_coords;
   float oo_path_scale;

   if (!vg_path_write_immediate(path, VG_PATH_RW_DATA)) {
      return false;
   }

   path_coords_size = coords_count * get_path_datatype_size((VGPathDatatype)path->datatype);

   if (!mem_resize_ex(path->segments, mem_get_size(path->segments) + segments_count, MEM_COMPACT_DISCARD)) {
      return false;
   }
   if (!mem_resize_ex(path->coords, mem_get_size(path->coords) + path_coords_size, MEM_COMPACT_DISCARD)) {
      verify(mem_resize_ex(path->segments, mem_get_size(path->segments) - segments_count, MEM_COMPACT_NONE));
      return false;
   }

   memcpy((uint8_t *)mem_lock(path->segments) + (mem_get_size(path->segments) - segments_count), segments, segments_count);
   mem_unlock(path->segments);

   path_coords = (uint8_t *)mem_lock(path->coords) + (mem_get_size(path->coords) - path_coords_size);
   oo_path_scale = recip_(path->scale);
   for (; coords_count != 0; ++coords, --coords_count) {
      put_coord((VGPathDatatype)path->datatype, oo_path_scale, path->bias, &path_coords, *coords);
   }
   mem_unlock(path->coords);

   return true;
}

void vguLine_impl(
   VGPath vg_handle,
   VGfloat p0_x, VGfloat p0_y,
   VGfloat p1_x, VGfloat p1_y)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_PATH_T *path;
   uint8_t segments[2];
   float coords[4];

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path = (VG_PATH_T *)mem_lock(handle);

   if (!(path->caps & VG_PATH_CAPABILITY_APPEND_TO)) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   segments[0] = VG_MOVE_TO_ABS;
   segments[1] = VG_LINE_TO_ABS;
   coords[0] = p0_x;
   coords[1] = p0_y;
   coords[2] = p1_x;
   coords[3] = p1_y;
   if (!append(path, segments, ARR_COUNT(segments), coords, ARR_COUNT(coords))) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path_changed(state, handle, path, PATH_ELEMENT_DATA);

   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

void vguPolygon_impl(
   VGPath vg_handle,
   const VGfloat *ps, VGint ps_count,
   bool first, bool close)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_PATH_T *path;
   uint32_t segments_count, coords_count, path_coords_size;
   uint8_t *path_segments;
   void *path_coords;
   float oo_path_scale;

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path = (VG_PATH_T *)mem_lock(handle);

   if (!(path->caps & VG_PATH_CAPABILITY_APPEND_TO)) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if (!vg_path_write_immediate(path, VG_PATH_RW_DATA)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   segments_count = ps_count + (close ? 1 : 0);
   coords_count = 2 * ps_count;
   path_coords_size = coords_count * get_path_datatype_size((VGPathDatatype)path->datatype);

   if (!mem_resize_ex(path->segments, mem_get_size(path->segments) + segments_count, MEM_COMPACT_DISCARD)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }
   if (!mem_resize_ex(path->coords, mem_get_size(path->coords) + path_coords_size, MEM_COMPACT_DISCARD)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      verify(mem_resize_ex(path->segments, mem_get_size(path->segments) - segments_count, MEM_COMPACT_NONE));
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path_segments = (uint8_t *)mem_lock(path->segments) + (mem_get_size(path->segments) - segments_count);
   memset(path_segments, VG_LINE_TO_ABS, segments_count);
   if (first) { path_segments[0] = VG_MOVE_TO_ABS; }
   if (close) { path_segments[segments_count - 1] = VG_CLOSE_PATH; }
   mem_unlock(path->segments);

   path_coords = (uint8_t *)mem_lock(path->coords) + (mem_get_size(path->coords) - path_coords_size);
   oo_path_scale = recip_(path->scale);
   for (; coords_count != 0; ++ps, --coords_count) {
      put_coord((VGPathDatatype)path->datatype, oo_path_scale, path->bias, &path_coords, clean_float(*ps));
   }
   mem_unlock(path->coords);

   path_changed(state, handle, path, PATH_ELEMENT_DATA);

   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

void vguRect_impl(
   VGPath vg_handle,
   VGfloat x, VGfloat y,
   VGfloat width, VGfloat height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_PATH_T *path;
   uint8_t segments[5];
   float coords[5];

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path = (VG_PATH_T *)mem_lock(handle);

   if (!(path->caps & VG_PATH_CAPABILITY_APPEND_TO)) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   segments[0] = VG_MOVE_TO_ABS;
   segments[1] = VG_HLINE_TO_REL;
   segments[2] = VG_VLINE_TO_REL;
   segments[3] = VG_HLINE_TO_REL;
   segments[4] = VG_CLOSE_PATH;
   coords[0] = x;
   coords[1] = y;
   coords[2] = width;
   coords[3] = height;
   coords[4] = -width;
   if (!append(path, segments, ARR_COUNT(segments), coords, ARR_COUNT(coords))) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path_changed(state, handle, path, PATH_ELEMENT_DATA);

   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

void vguRoundRect_impl(
   VGPath vg_handle,
   VGfloat x, VGfloat y,
   VGfloat width, VGfloat height,
   VGfloat arc_width, VGfloat arc_height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_PATH_T *path;
   uint8_t segments[10];
   float coords[26];

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path = (VG_PATH_T *)mem_lock(handle);

   if (!(path->caps & VG_PATH_CAPABILITY_APPEND_TO)) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   arc_width = clampf(arc_width, 0.0f, width);
   arc_height = clampf(arc_height, 0.0f, height);

   segments[0] = VG_MOVE_TO_ABS;
   segments[1] = VG_HLINE_TO_REL;
   segments[2] = VG_SCCWARC_TO_REL;
   segments[3] = VG_VLINE_TO_REL;
   segments[4] = VG_SCCWARC_TO_REL;
   segments[5] = VG_HLINE_TO_REL;
   segments[6] = VG_SCCWARC_TO_REL;
   segments[7] = VG_VLINE_TO_REL;
   segments[8] = VG_SCCWARC_TO_REL;
   segments[9] = VG_CLOSE_PATH;
   coords[0] = x + (arc_width * 0.5f);
   coords[1] = y;
   coords[2] = width - arc_width;
   coords[3] = arc_width * 0.5f;
   coords[4] = arc_height * 0.5f;
   coords[5] = 0.0f;
   coords[6] = arc_width * 0.5f;
   coords[7] = arc_height * 0.5f;
   coords[8] = height - arc_height;
   coords[9] = arc_width * 0.5f;
   coords[10] = arc_height * 0.5f;
   coords[11] = 0.0f;
   coords[12] = arc_width * -0.5f;
   coords[13] = arc_height * 0.5f;
   coords[14] = arc_width - width;
   coords[15] = arc_width * 0.5f;
   coords[16] = arc_height * 0.5f;
   coords[17] = 0.0f;
   coords[18] = arc_width * -0.5f;
   coords[19] = arc_height * -0.5f;
   coords[20] = arc_height - height;
   coords[21] = arc_width * 0.5f;
   coords[22] = arc_height * 0.5f;
   coords[23] = 0.0f;
   coords[24] = arc_width * 0.5f;
   coords[25] = arc_height * -0.5f;
   if (!append(path, segments, ARR_COUNT(segments), coords, ARR_COUNT(coords))) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path_changed(state, handle, path, PATH_ELEMENT_DATA);

   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

void vguEllipse_impl(
   VGPath vg_handle,
   VGfloat x, VGfloat y,
   VGfloat width, VGfloat height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_PATH_T *path;
   uint8_t segments[4];
   float coords[12];

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path = (VG_PATH_T *)mem_lock(handle);

   if (!(path->caps & VG_PATH_CAPABILITY_APPEND_TO)) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   segments[0] = VG_MOVE_TO_ABS;
   segments[1] = VG_SCCWARC_TO_REL;
   segments[2] = VG_SCCWARC_TO_REL;
   segments[3] = VG_CLOSE_PATH;
   coords[0] = x + (width * 0.5f);
   coords[1] = y;
   coords[2] = width * 0.5f;
   coords[3] = height * 0.5f;
   coords[4] = 0.0f;
   coords[5] = -width;
   coords[6] = 0.0f;
   coords[7] = width * 0.5f;
   coords[8] = height * 0.5f;
   coords[9] = 0.0f;
   coords[10] = width;
   coords[11] = 0.0f;
   if (!append(path, segments, ARR_COUNT(segments), coords, ARR_COUNT(coords))) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path_changed(state, handle, path, PATH_ELEMENT_DATA);

   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

void vguArc_impl(
   VGPath vg_handle,
   VGfloat x, VGfloat y,
   VGfloat width, VGfloat height,
   VGfloat start_angle, VGfloat angle_extent, VGuint angle_o180,
   VGUArcType arc_type)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle;
   VG_PATH_T *path;
   uint32_t segments_count, coords_count, path_coords_size;
   uint8_t *path_segments;
   void *path_coords;
   float oo_path_scale, d_x, d_y;

   handle = vg_handle_to_mem_handle(vg_handle);
   if (!check_object(state, handle, OBJECT_TYPE_PATH)) {
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path = (VG_PATH_T *)mem_lock(handle);

   if (!(path->caps & VG_PATH_CAPABILITY_APPEND_TO)) {
      set_error(state, VG_PATH_CAPABILITY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   if (!vg_path_write_immediate(path, VG_PATH_RW_DATA)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   segments_count = 2 + angle_o180;
   coords_count = 7 + (angle_o180 * 5);
   switch (arc_type) {
   case VGU_ARC_OPEN:  break;
   case VGU_ARC_CHORD: segments_count += 1; break;
   case VGU_ARC_PIE:   segments_count += 2; coords_count += 2; break;
   default:            UNREACHABLE();
   }
   path_coords_size = coords_count * get_path_datatype_size((VGPathDatatype)path->datatype);

   if (!mem_resize_ex(path->segments, mem_get_size(path->segments) + segments_count, MEM_COMPACT_DISCARD)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }
   if (!mem_resize_ex(path->coords, mem_get_size(path->coords) + path_coords_size, MEM_COMPACT_DISCARD)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      verify(mem_resize_ex(path->segments, mem_get_size(path->segments) - segments_count, MEM_COMPACT_NONE));
      mem_unlock(handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   path_segments = (uint8_t *)mem_lock(path->segments) + (mem_get_size(path->segments) - segments_count);
   path_segments[0] = VG_MOVE_TO_ABS;
   memset(path_segments + 1, (angle_extent < 0.0f) ? VG_SCWARC_TO_ABS : VG_SCCWARC_TO_ABS, segments_count - 1);
   if (arc_type == VGU_ARC_PIE) { path_segments[segments_count - 2] = VG_LINE_TO_ABS; }
   if ((arc_type == VGU_ARC_CHORD) || (arc_type == VGU_ARC_PIE)) { path_segments[segments_count - 1] = VG_CLOSE_PATH; }
   mem_unlock(path->segments);

   path_coords = (uint8_t *)mem_lock(path->coords) + (mem_get_size(path->coords) - path_coords_size);
   oo_path_scale = recip_(path->scale);

   #define PUT_COORD(X) put_coord((VGPathDatatype)path->datatype, oo_path_scale, path->bias, &path_coords, X)

   sin_cos_(&d_y, &d_x, start_angle * (PI / 180.0f));
   d_x *= width * 0.5f;
   d_y *= height * 0.5f;
   PUT_COORD(x + d_x);
   PUT_COORD(y + d_y);
   for (; angle_o180 != 0; --angle_o180) {
      d_x = -d_x;
      d_y = -d_y;
      PUT_COORD(width * 0.5f);
      PUT_COORD(height * 0.5f);
      PUT_COORD(0.0f);
      PUT_COORD(x + d_x);
      PUT_COORD(y + d_y);
   }

   sin_cos_(&d_y, &d_x, (start_angle + angle_extent) * (PI / 180.0f));
   d_x *= width * 0.5f;
   d_y *= height * 0.5f;
   PUT_COORD(width * 0.5f);
   PUT_COORD(height * 0.5f);
   PUT_COORD(0.0f);
   PUT_COORD(x + d_x);
   PUT_COORD(y + d_y);

   if (arc_type == VGU_ARC_PIE) {
      PUT_COORD(x);
      PUT_COORD(y);
   }

   #undef PUT_COORD

   mem_unlock(path->coords);

   path_changed(state, handle, path, PATH_ELEMENT_DATA);

   mem_unlock(handle);

   VG_UNLOCK_SERVER_STATE();
}

/******************************************************************************
VG_KHR_EGL_image
******************************************************************************/

/*
   Khronos documentation:

   vgCreateEGLImageTargetKHR

      vgCreateEGLImageTarget creates an EGLImage target VGImage
      object from the provided EGLImage <image>.  <image> should be
      of type EGLImageKHR, cast into the type VGeglImageKHR.  Assuming
      no errors are generated in this function, the resulting VGImage
      will be an EGLImage target of the specified EGLImage <image>.
      As a side-effect of the referencing operation, all of the pixel
      data in the <buffer> used as the EGLImage source resource (i.e.,
      the <buffer> parameter passed to the CreateImageKHR command that
      returned <image>) will become undefined.

      The resulting VGImage may be used normally by all OpenVG
      operations, including the creation of child images.  All child
      images of VGImages created using vgCreateEGLImageTargetKHR (and
      their children, etc.) are also EGLImage targets, and will remain
      EGLImage targets until destroyed by subsequent calls to
      vgDestroyImage.

      ERRORS

      VG_UNSUPPORTED_IMAGE_FORMAT_ERROR
      - if the OpenVG implementation is not able to create a VGImage
        compatible with the provided VGeglImageKHR for an implementation-
        dependent reason (this could be caused by, but not limited to,
        reasons such as unsupported pixel formats, anti-aliasing quality,
        etc.).

      VG_ILLEGAL_ARGUMENT_ERROR
      - if <image> is not a valid VGeglImageKHR.

*/

#if VG_KHR_EGL_image

#include "middleware/khronos/ext/egl_khr_image.h"

VGImage vgCreateEGLImageTargetKHR_impl(VGeglImageKHR src_egl_handle, VGuint *format_width_height)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   EGL_SERVER_STATE_T *egl_state = EGL_GET_SERVER_STATE();
   MEM_HANDLE_T eglimage_handle, src_handle;
   KHRN_IMAGE_T *src;
   uint32_t src_width, src_height;
   VGImageFormat format;
   MEM_HANDLE_T handle;

   eglimage_handle = khrn_map_lookup(&egl_state->eglimages, (uint32_t)(uintptr_t)src_egl_handle);
   if (eglimage_handle == MEM_INVALID_HANDLE) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return VG_INVALID_HANDLE;
   }

   src_handle = ((EGL_IMAGE_T *)mem_lock(eglimage_handle))->mh_image;
   mem_unlock(eglimage_handle);

   src = (KHRN_IMAGE_T *)mem_lock(src_handle);
   src_width = src->width;
   src_height = src->height;
   format = vg_image_get_external_format(src->format);
   mem_unlock(src_handle);

   if ((src_width == 0) || (src_height == 0) ||
      (src_width > VG_CONFIG_MAX_IMAGE_WIDTH) || (src_height > VG_CONFIG_MAX_IMAGE_HEIGHT) ||
      (format == (VGImageFormat)-1)) {
      set_error(state, VG_UNSUPPORTED_IMAGE_FORMAT_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return VG_INVALID_HANDLE;
   }

   handle = vg_image_alloc_from_image(src_handle);
   if (handle == MEM_INVALID_HANDLE) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      VG_UNLOCK_SERVER_STATE();
      return VG_INVALID_HANDLE;
   }

   if (!insert_object(state, handle)) {
      set_error(state, VG_OUT_OF_MEMORY_ERROR);
      mem_release(handle);
      VG_UNLOCK_SERVER_STATE();
      return VG_INVALID_HANDLE;
   }
   mem_release(handle); /* handle now owned by objects set */

   VG_UNLOCK_SERVER_STATE();
   format_width_height[0] = format;
   format_width_height[1] = src_width;
   format_width_height[2] = src_height;
   return vg_handle_from_mem_handle(handle);
}

#if EGL_BRCM_global_image

void vgCreateImageFromGlobalImage_impl(VGHandle vg_handle, VGuint id_0, VGuint id_1)
{
   VG_SERVER_STATE_T *state = VG_LOCK_SERVER_STATE();
   MEM_HANDLE_T handle, src_handle;

   handle = vg_handle_to_mem_handle(vg_handle);
   vcos_assert(is_object_ok(state, handle, OBJECT_TYPE_STEM));

   src_handle = egl_brcm_global_image_lookup(id_0, id_1);
   if (src_handle == MEM_INVALID_HANDLE) {
      set_error(state, VG_ILLEGAL_ARGUMENT_ERROR);
      delete_object(state, handle);
      VG_UNLOCK_SERVER_STATE();
      return;
   }

   vg_image_from_stem_and_image(handle, src_handle);

   VG_UNLOCK_SERVER_STATE();
}

#endif

#endif
