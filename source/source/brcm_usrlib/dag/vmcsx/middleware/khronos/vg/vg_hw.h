/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VG_HW_H
#define VG_HW_H

#include "middleware/khronos/vg/vg_paint.h"
#include "middleware/khronos/vg/vg_path.h"
#include "middleware/khronos/vg/vg_server.h"

/* global init/term */
extern bool vg_be_init(void);
extern void vg_be_term(void);

/*
   vg_be_buffers_changed is called when either the current framebuffer or the
   current mask buffer changes. this will usually be due to eglMakeCurrent or
   eglSwapBuffers

   vg_be_state_changed is called when the current context (or some element of
   it) changes but the current buffers do not (ie vg_be_buffers_changed should
   assume that the state has also changed)
*/

extern void vg_be_buffers_changed(MEM_HANDLE_T frame_handle, MEM_HANDLE_T mask_handle);

typedef enum {
   VG_BE_STATE_ELEMENT_STATE                   = 1 << 0, /* the state object itself */

   VG_BE_STATE_ELEMENT_GLYPH_ORIGIN            = 1 << 1,
   VG_BE_STATE_ELEMENT_PATH_USER_TO_SURFACE    = 1 << 2,
   VG_BE_STATE_ELEMENT_IMAGE_USER_TO_SURFACE   = 1 << 3,
   VG_BE_STATE_ELEMENT_GLYPH_USER_TO_SURFACE   = 1 << 4,
   VG_BE_STATE_ELEMENT_FILL_PAINT_TO_USER      = 1 << 5,
   VG_BE_STATE_ELEMENT_STROKE_PAINT_TO_USER    = 1 << 6,
   VG_BE_STATE_ELEMENT_FILL_RULE               = 1 << 7,
   VG_BE_STATE_ELEMENT_STROKE_LINE_WIDTH       = 1 << 8,
   VG_BE_STATE_ELEMENT_STROKE_CAP_STYLE        = 1 << 9,
   VG_BE_STATE_ELEMENT_STROKE_JOIN_STYLE       = 1 << 10,
   VG_BE_STATE_ELEMENT_STROKE_MITER_LIMIT      = 1 << 11,
   VG_BE_STATE_ELEMENT_STROKE_DASH_PATTERN     = 1 << 12,
   VG_BE_STATE_ELEMENT_STROKE_DASH_PHASE       = 1 << 13,
   VG_BE_STATE_ELEMENT_STROKE_DASH_PHASE_RESET = 1 << 14,
   VG_BE_STATE_ELEMENT_IMAGE_QUALITY           = 1 << 15,
   VG_BE_STATE_ELEMENT_IMAGE_MODE              = 1 << 16,
   VG_BE_STATE_ELEMENT_SCISSORING              = 1 << 17,
   VG_BE_STATE_ELEMENT_SCISSOR_RECTS           = 1 << 18,
   VG_BE_STATE_ELEMENT_RENDERING_QUALITY       = 1 << 19,
   VG_BE_STATE_ELEMENT_FILL_PAINT              = 1 << 20,
   VG_BE_STATE_ELEMENT_STROKE_PAINT            = 1 << 21,
   VG_BE_STATE_ELEMENT_TILE_FILL_RGBA          = 1 << 22,
   VG_BE_STATE_ELEMENT_CLEAR_RGBA              = 1 << 23,
   VG_BE_STATE_ELEMENT_COLOR_TRANSFORM         = 1 << 24,
   VG_BE_STATE_ELEMENT_COLOR_TRANSFORM_VALUES  = 1 << 25,
   VG_BE_STATE_ELEMENT_BLEND_MODE              = 1 << 26,
   VG_BE_STATE_ELEMENT_MASKING                 = 1 << 27,
   VG_BE_STATE_ELEMENT_FILTER_FORMAT_LINEAR    = 1 << 28,
   VG_BE_STATE_ELEMENT_FILTER_FORMAT_PRE       = 1 << 29,
   VG_BE_STATE_ELEMENT_FILTER_CHANNEL_MASK     = 1 << 30,
   VG_BE_STATE_ELEMENT_PIXEL_LAYOUT            = 1 << 31,

   VG_BE_STATE_ELEMENT_ALL                     = (int)0xffffffff
} VG_BE_STATE_ELEMENT_T;

extern void vg_be_state_changed(VG_BE_STATE_ELEMENT_T elements);

/*
   drawing functions. it can be assumed that when these are called, we have a
   current state and framebuffer (and in the case of vg_be_mask and
   vg_be_render_to_mask, a current mask buffer)
*/

extern bool vg_be_mask(
   MEM_HANDLE_T mask_handle,
   VGMaskOperation operation,
   uint32_t dst_x, uint32_t dst_y, uint32_t width, uint32_t height, uint32_t src_x, uint32_t src_y);

extern bool vg_be_render_to_mask(VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle, VG_PATH_T *path, /* should unlock handle if path is non-NULL */
   uint32_t paint_modes, VGMaskOperation operation,
   const uint32_t *clip_rect, MEM_HANDLE_T scissor_handle, /* should unretain scissor_handle if not MEM_INVALID_HANDLE */
   const float *clip, float scale_max, const float *stroke_clip);

extern bool vg_be_clear(VG_SERVER_STATE_T *state,
   uint32_t x, uint32_t y, uint32_t width, uint32_t height);

extern bool vg_be_draw_path(VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle, VG_PATH_T *path, /* should unlock handle */
   uint32_t paint_modes,
   const uint32_t *clip_rect, MEM_HANDLE_T scissor_handle, /* should unretain scissor_handle if not MEM_INVALID_HANDLE */
   const float *clip, float scale_max, const float *stroke_clip);

extern bool vg_be_draw_image(VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
   const float *p, /* already transformed (by state->image_user_to_surface) */
   const uint32_t *clip_rect, MEM_HANDLE_T scissor_handle); /* should unretain scissor_handle if not MEM_INVALID_HANDLE */

extern bool vg_be_draw_glyph_path(VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle, VG_PATH_T *path, /* should unlock handle */
   uint32_t paint_modes,
   float tr_x, float tr_y,
   const uint32_t *clip_rect, MEM_HANDLE_T scissor_handle,
   const float *clip, float scale_max);
   
extern bool vg_be_draw_glyph_path_init(VG_SERVER_STATE_T *state,
   uint32_t paint_modes,
   const uint32_t *clip_rect, MEM_HANDLE_T scissor_handle);
   
extern bool vg_be_draw_glyph_image(VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
   float tr_x, float tr_y,
   const float *p, /* already transformed (by state->glyph_user_to_surface) and translated (by tr_x/tr_y) */
   const uint32_t *clip_rect, MEM_HANDLE_T scissor_handle);

/*
   vg_be_flush should flush all outstanding vg rendering (the vgFlush spec says
   we should flush all outstanding rendering for the current context, but that
   doesn't make much sense in our architecture)

   vg_be_finish should have the same effect as vg_be_flush, but should wait for
   all the relevant rendering to complete before returning
*/

extern void vg_be_flush(void);
extern void vg_be_finish(void);

/*
   unlock any handles we're keeping "unnecessarily" locked
   this might be called when the backend isn't initialised
*/

extern void vg_be_unlock(void);

/******************************************************************************
non-platform-specific helpers
******************************************************************************/

extern bool vg_be_prepare_scissor(VG_SERVER_STATE_SCISSOR_T *scissor, uint32_t *rect, MEM_HANDLE_T *handle, uint32_t width, uint32_t height); /* if handle != MEM_INVALID_HANDLE on return, it has been retained */
extern bool vg_be_prepare_paint_grad_linear(VG_PAINT_T *paint, VG_MAT3X3_T *surface_to_paint);
extern bool vg_be_prepare_paint_grad_radial(VG_PAINT_T *paint, VG_MAT3X3_T *surface_to_paint, float *u, float *v);
extern MEM_HANDLE_T vg_be_prepare_paint_pattern(VG_PAINT_T *paint, VG_MAT3X3_T *surface_to_paint,
   KHRN_IMAGE_FORMAT_T *format, uint32_t *width_height, bool *bilinear, uint32_t *child_rect, uint32_t *tile_fill_rgba);
extern MEM_HANDLE_T vg_be_prepare_image(MEM_HANDLE_T handle,
   KHRN_IMAGE_FORMAT_T *format, uint32_t *width_height, bool *bilinear);

#endif
