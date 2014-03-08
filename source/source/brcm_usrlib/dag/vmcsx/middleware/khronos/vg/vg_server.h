/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/*
   Applies to all functions

   Precondition:

   Valid EGL server state exists
*/

#ifndef VG_SERVER_H
#define VG_SERVER_H

#include "interface/khronos/vg/vg_int_config.h"
#include "interface/khronos/vg/vg_int.h"
#include "interface/khronos/vg/vg_int_mat3x3.h"
#include "middleware/khronos/vg/vg_set.h"
#include "middleware/khronos/common/khrn_mem.h"
#include "middleware/khronos/common/khrn_stats.h"
#include "middleware/khronos/egl/egl_server.h"
#include "interface/khronos/include/VG/openvg.h"
#include "interface/khronos/include/VG/vgext.h"
#include "interface/khronos/include/VG/vgu.h"

/******************************************************************************
shared state
******************************************************************************/

typedef struct {
   /*
      Invariants:

      (VG_SERVER_SHARED_STATE_OBJECTS) Every object in objects is valid and has
         a type determined by the mem terminator, which is one of:
         - NULL: stem
         - vg_font_bprint_term: VG_FONT_BPRINT_T
         - vg_font_term: VG_FONT_T
         - vg_image_bprint_term: VG_IMAGE_BPRINT_T
         - vg_image_term: VG_IMAGE_T
         - vg_child_image_bprint_term: VG_CHILD_IMAGE_BPRINT_T
         - vg_child_image_term: VG_CHILD_IMAGE_T
         - vg_mask_layer_bprint_term: VG_MASK_LAYER_BPRINT_T
         - vg_mask_layer_term: VG_MASK_LAYER_T
         - vg_paint_bprint_term: VG_PAINT_BPRINT_T
         - vg_paint_term: VG_PAINT_T
         - vg_path_bprint_term: VG_PATH_BPRINT_T
         - vg_path_term: VG_PATH_T
         (The union of these is "type X" for objects)

      (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED_OBJECTS_STORAGE)
   */

   VG_SET_T objects;
} VG_SERVER_SHARED_STATE_T;

extern void vg_server_shared_state_term(void *p, uint32_t);
extern MEM_HANDLE_T vg_server_shared_state_alloc(void);

/******************************************************************************
state
******************************************************************************/

/*
   General invariants:

   (VG_SERVER_STATE_BOOLS_CLEAN) All members with type bool only take on the
      value 0 (false) or 1 (true)
   (VG_SERVER_STATE_ENUMS_CLEAN) All members with a VG enum type (eg VGCapStyle)
      only take on valid enum values
*/

typedef struct {
   /*
      VG_STROKE_LINE_WIDTH. In user space. <= 0 means no stroking

      Invariants:

      (VG_SERVER_STATE_STROKE_LINE_WIDTH_CLEAN) Clean (ie not nan or infinite)
   */

   float line_width;

   /*
      VG_STROKE_CAP_STYLE

      Invariants:

      (VG_SERVER_STATE_ENUMS_CLEAN)
   */

   VGCapStyle cap_style;

   /*
      VG_STROKE_JOIN_STYLE

      Invariants:

      (VG_SERVER_STATE_ENUMS_CLEAN)
   */

   VGJoinStyle join_style;

   /*
      VG_STROKE_MITER_LIMIT

      Invariants:

      (VG_SERVER_STATE_STROKE_MITER_LIMIT_RANGE) In [1, inf) (this implies
         cleanliness)
   */

   float miter_limit;

   /*
      VG_STROKE_DASH_PATTERN. In user space

      Invariants:

      (VG_SERVER_STATE_STROKE_DASH_PATTERN_RANGE) dash_pattern[i] in [0, inf)
         (this implies cleanliness)
      (VG_SERVER_STATE_STROKE_DASH_PATTERN_COUNT_EVEN) dash_pattern_count is
         even
      (VG_SERVER_STATE_STROKE_DASH_PATTERN_COUNT_RANGE) dash_pattern_count <=
         VG_CONFIG_MAX_DASH_COUNT
   */

   float dash_pattern[VG_CONFIG_MAX_DASH_COUNT];
   uint32_t dash_pattern_count;

   /*
      VG_STROKE_DASH_PHASE. In user space

      Invariants:

      (VG_SERVER_STATE_DASH_PHASE_CLEAN) Clean (ie not nan or infinite)
   */

   float dash_phase;

   /*
      VG_STROKE_DASH_PHASE_RESET

      Invariants:

      (VG_SERVER_STATE_BOOLS_CLEAN)
   */

   bool dash_phase_reset;
} VG_SERVER_STATE_STROKE_T;

typedef struct {
   int32_t rects[VG_CONFIG_MAX_SCISSOR_RECTS * 4]; /* widths/heights >= 0 */
   uint32_t rects_count; /* multiple of 4 */
   MEM_HANDLE_T scissor;
   int32_t bounds[4];

   /*
      frame width/height at the time scissor was generated
      scissor must be regenerated if the frame width/height changes
   */

   uint32_t width, height;
} VG_SERVER_STATE_SCISSOR_T;

typedef struct {
   /*
      rasterisation
   */

   float glyph_origin[2]; /* clean_float on use */

   VG_MAT3X3_T path_user_to_surface; /* affine */
   VG_MAT3X3_T image_user_to_surface;
   VG_MAT3X3_T glyph_user_to_surface; /* affine */
   VG_MAT3X3_T fill_paint_to_user; /* affine */
   VG_MAT3X3_T stroke_paint_to_user; /* affine */

   VGFillRule fill_rule;
   VG_SERVER_STATE_STROKE_T stroke;
   VGImageQuality image_quality;
   VGImageMode image_mode;

   bool scissoring;
   VG_SERVER_STATE_SCISSOR_T scissor;

   VGRenderingQuality rendering_quality;

   /*
      fill_paint is used for vgDrawImage as well as for regular fills

      Invariants:

      (VG_SERVER_STATE_PAINTS_VALID) fill_paint/stroke_paint always refer to
         valid objects of type VG_PAINT_T
   */

   MEM_HANDLE_T fill_paint;
   MEM_HANDLE_T stroke_paint;

   uint32_t tile_fill_rgba;
   uint32_t clear_rgba;

   bool color_transform;
   float color_transform_values[8]; /* first 4 in [-127, 127], second 4 in [-1, 1] */

   VGBlendMode blend_mode;
   bool masking;

   /*
      image filters
   */

   bool filter_format_linear;
   bool filter_format_pre;
   uint32_t filter_channel_mask;

   /*
      misc
   */

   /*
      error

      Invariants:

      (VG_SERVER_ERROR)
      error is a valid VGErrorCode, as listed in VG/openvg.h
   */

   VGErrorCode error;

   /*
      shared state

      Invariants:

      (VG_SERVER_SHARED_STATE_VALID)
      shared_state is a handle to a valid VG_SERVER_SHARED_STATE_T

      shared_state does not change so we do not need to worry about (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED) or (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED_OBJECTS_STORAGE)
   */

   MEM_HANDLE_T shared_state;

   /*
      id of creating process
   */

   uint64_t pid;
} VG_SERVER_STATE_T;

extern void vg_server_state_term(void *p, uint32_t);
extern MEM_HANDLE_T vg_server_state_alloc(MEM_HANDLE_T shared_state, uint64_t pid);

static INLINE MEM_HANDLE_T vg_get_server_state_handle(void)
{
   return EGL_GET_SERVER_STATE()->vgcontext;
}

static INLINE VG_SERVER_STATE_T *vg_lock_server_state(void)
{
   EGL_SERVER_STATE_T *egl_state = EGL_GET_SERVER_STATE();

   khrn_stats_record_start(KHRN_STATS_VG);

   if (!egl_state->locked_vgcontext) {
      egl_state->locked_vgcontext = mem_lock(egl_state->vgcontext);
   }
   return (VG_SERVER_STATE_T *)egl_state->locked_vgcontext;
}

static INLINE void vg_unlock_server_state(void)
{
#ifndef NDEBUG
   EGL_SERVER_STATE_T *egl_state = EGL_GET_SERVER_STATE();
   vcos_assert(egl_state->locked_vgcontext);
#endif /* NDEBUG */

   khrn_stats_record_end(KHRN_STATS_VG);
}

/*
   VG_SERVER_SHARED_STATE_T *vg_lock_server_shared_state(VG_SERVER_STATE_T *state)

   Returns locked pointer to state->shared_state.

   Implementation notes:

   We keep one of these locked over time (in egl_state).

   Preconditions:

   Is being called from a function which _always_ subsequently calls vg_unlock_server_shared_state(state)
   state is valid pointer

   Postconditions:

   Return value is a valid pointer
   If state == egl_state->locked_vgcontext then egl_state->locked_vgcontext_shared is not null

   Invariants used:
   (EGL_SERVER_STATE_LOCKED_VGCONTEXT)
   (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED)

   Invariants preserved:
   (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED)
*/

static INLINE VG_SERVER_SHARED_STATE_T *vg_lock_server_shared_state(VG_SERVER_STATE_T *state)
{
   EGL_SERVER_STATE_T *egl_state = EGL_GET_SERVER_STATE();
   if (state == egl_state->locked_vgcontext) {
      if (!egl_state->locked_vgcontext_shared) {
         egl_state->locked_vgcontext_shared = mem_lock(state->shared_state);
      }
      return (VG_SERVER_SHARED_STATE_T *)egl_state->locked_vgcontext_shared;
   } else {
      return (VG_SERVER_SHARED_STATE_T *)mem_lock(state->shared_state);
   }
}

/*
   void vg_unlock_server_shared_state(VG_SERVER_STATE_T *state)

   Releases locked pointer returned by vg_lock_server_shared_state. This may not
   actually result in the server shared state being unlocked.

   Implementation notes:

   Unlock won't be performed if EGL owns the shared state lock.

   Preconditions:

   Is being called from a function which has _always_ previously called vg_lock_server_shared_state(state)
   and nothing has changed egl_state->locked_vgcontext or egl_state->locked_vgcontext_shared in the meantime.

   Postconditions:

   -

   Invariants used:
   (EGL_SERVER_STATE_LOCKED_VGCONTEXT)
   (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED)
*/

static INLINE void vg_unlock_server_shared_state(VG_SERVER_STATE_T *state)
{
   EGL_SERVER_STATE_T *egl_state = EGL_GET_SERVER_STATE();
   if (state == egl_state->locked_vgcontext) {
      vcos_assert(egl_state->locked_vgcontext_shared);
   } else {
      mem_unlock(state->shared_state);
   }
}

/*
   void *vg_lock_server_shared_state_objects_storage(VG_SERVER_SHARED_STATE_T *shared_state)

   Returns locked pointer to state->shared_state->objects->storage.

   Implementation notes:

   We keep one of these locked over time (in egl_state).

   Preconditions:

   Is being called from a function which _always_ subsequently calls vg_unlock_server_shared_state_objects_storage(shared_state)
   shared_state is valid pointer

   Postconditions:

   Return value is a locked pointer to shared_state->objects->storage
   If shared_state == egl_state->locked_vgcontext_shared then egl_state->locked_vgcontext_shared_objects_storage is not null

   Invariants used:
   (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED)
   (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED_OBJECTS_STORAGE)

   Invariants preserved:
   (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED_OBJECTS_STORAGE)
*/

static INLINE void *vg_lock_server_shared_state_objects_storage(VG_SERVER_SHARED_STATE_T *shared_state)
{
   EGL_SERVER_STATE_T *egl_state = EGL_GET_SERVER_STATE();
   if (shared_state == egl_state->locked_vgcontext_shared) {
      if (!egl_state->locked_vgcontext_shared_objects_storage) {
         egl_state->locked_vgcontext_shared_objects_storage = mem_lock(shared_state->objects.storage);
      }
      return egl_state->locked_vgcontext_shared_objects_storage;
   } else {
      return mem_lock(shared_state->objects.storage);
   }
}

/*
   void vg_unlock_server_shared_state_objects_storage(VG_SERVER_SHARED_STATE_T *shared_state)

   Releases locked pointer returned by vg_lock_server_shared_state_objects_storage. This may not
   actually result in the storage being unlocked.

   Implementation notes:

   Unlock won't be performed if EGL owns the storage lock.

   Preconditions:

   Is being called from a function which has _always_ previously called vg_lock_server_shared_state_objects_storage(shared_state)
   and nothing has changed egl_state->locked_vgcontext_shared or egl_state->locked_vgcontext_shared_objects_storage in the meantime.

   Postconditions:

   -

   Invariants used:
   (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED)
   (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED_OBJECTS_STORAGE)
*/

static INLINE void vg_unlock_server_shared_state_objects_storage(VG_SERVER_SHARED_STATE_T *shared_state)
{
   EGL_SERVER_STATE_T *egl_state = EGL_GET_SERVER_STATE();
   if (shared_state == egl_state->locked_vgcontext_shared) {
      vcos_assert(egl_state->locked_vgcontext_shared_objects_storage);
   } else {
      mem_unlock(shared_state->objects.storage);
   }
}

/*
   void vg_force_unlock_server_shared_state_objects_storage(VG_SERVER_SHARED_STATE_T *shared_state)

   If EGL owns a lock on shared_state->objects->storage then release it.

   Implementation notes:

   -

   Preconditions:

   shared_state is a valid pointer

   Postconditions:

   egl_state->locked_vgcontext_shared_objects_storage != shared_state->objects->storage

   Invariants used:
   (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED)

   Invariants preserved:
   (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED_OBJECTS_STORAGE)
*/

static INLINE void vg_force_unlock_server_shared_state_objects_storage(VG_SERVER_SHARED_STATE_T *shared_state)
{
   EGL_SERVER_STATE_T *egl_state = EGL_GET_SERVER_STATE();
   if ((shared_state == egl_state->locked_vgcontext_shared) && egl_state->locked_vgcontext_shared_objects_storage) {
      mem_unlock(shared_state->objects.storage);
      egl_state->locked_vgcontext_shared_objects_storage = NULL;
   }
}

static INLINE void vg_force_unlock_server_shared_state(VG_SERVER_STATE_T *state)
{
   EGL_SERVER_STATE_T *egl_state = EGL_GET_SERVER_STATE();
   if ((state == egl_state->locked_vgcontext) && egl_state->locked_vgcontext_shared) {
      vg_force_unlock_server_shared_state_objects_storage((VG_SERVER_SHARED_STATE_T *)egl_state->locked_vgcontext_shared);
      mem_unlock(state->shared_state);
      egl_state->locked_vgcontext_shared = NULL;
   }
}

static INLINE void vg_force_unlock_server_state(void)
{
   EGL_SERVER_STATE_T *egl_state = EGL_GET_SERVER_STATE();
   if (egl_state->locked_vgcontext) {
      vg_force_unlock_server_shared_state((VG_SERVER_STATE_T *)egl_state->locked_vgcontext);
      mem_unlock(egl_state->vgcontext);
      egl_state->locked_vgcontext = NULL;
   }
}

#define VG_GET_SERVER_STATE_HANDLE() vg_get_server_state_handle()
#define VG_LOCK_SERVER_STATE() vg_lock_server_state()
#define VG_UNLOCK_SERVER_STATE() vg_unlock_server_state()
#define VG_LOCK_SERVER_SHARED_STATE(STATE) vg_lock_server_shared_state(STATE)
#define VG_UNLOCK_SERVER_SHARED_STATE(STATE) vg_unlock_server_shared_state(STATE)
#define VG_LOCK_SERVER_SHARED_STATE_OBJECTS_STORAGE(SHARED_STATE) vg_lock_server_shared_state_objects_storage(SHARED_STATE)
#define VG_UNLOCK_SERVER_SHARED_STATE_OBJECTS_STORAGE(SHARED_STATE) vg_unlock_server_shared_state_objects_storage(SHARED_STATE)
#define VG_FORCE_UNLOCK_SERVER_SHARED_STATE_OBJECTS_STORAGE(SHARED_STATE) vg_force_unlock_server_shared_state_objects_storage(SHARED_STATE)
#define VG_FORCE_UNLOCK_SERVER_SHARED_STATE(STATE) vg_force_unlock_server_shared_state(STATE)
#define VG_FORCE_UNLOCK_SERVER_STATE() vg_force_unlock_server_state()

extern void vg_server_state_set_error(VG_SERVER_STATE_T *state, VGErrorCode error);

/******************************************************************************
internal interface
******************************************************************************/

extern MEM_HANDLE_T vg_get_image(VG_SERVER_STATE_T *state, VGImage vg_handle, EGLint *error);

/*
   vg_buffers_changed is called when either the current framebuffer or the
   current mask buffer changes. this will usually be due to eglMakeCurrent or
   eglSwapBuffers

   vg_state_changed is called when the current context changes but the current
   buffers do not (ie vg_buffers_changed should assume that the state has also
   changed)
*/

extern void vg_buffers_changed(MEM_HANDLE_T frame_handle, MEM_HANDLE_T mask_handle);
extern void vg_state_changed(void);

/*
   the frame/mask buffer handles are saved when vg_buffers_changed is called...
   these functions return them
*/

extern MEM_HANDLE_T vg_get_frame(void);
extern MEM_HANDLE_T vg_get_mask(void);

/*
   unlock any handles we're keeping "unnecessarily" locked
*/

extern void vg_unlock(void);

extern void vg_maybe_term(void);

/******************************************************************************
server-side functions
******************************************************************************/

#include "interface/khronos/vg/vg_int_impl.h"

#endif
