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
#include "middleware/khronos/glxx/glxx_shared.h"

#include "middleware/khronos/glxx/glxx_server.h"
#include "middleware/khronos/glxx/glxx_server_internal.h"
#include "interface/khronos/include/GLES/glext.h"

#include "middleware/khronos/glxx/glxx_texture.h"
#include "middleware/khronos/glxx/glxx_buffer.h"
#include "interface/khronos/common/khrn_int_util.h"
#include "middleware/khronos/common/khrn_interlock.h"
#include "middleware/khronos/common/khrn_hw.h"

#include "interface/khronos/glxx/gl11_int_config.h"
#include "middleware/khronos/gl20/gl20_config.h"

//#include "../gl11/hw.h"
//#include "../gl20/hw.h"
#include "middleware/khronos/glxx/glxx_hw.h"

#include "middleware/khronos/gl20/gl20_program.h"
#include "middleware/khronos/gl20/gl20_shader.h"
#include "middleware/khronos/glxx/glxx_renderbuffer.h"
#include "middleware/khronos/glxx/glxx_framebuffer.h"

#include <string.h>
#include <math.h>
#include <limits.h>

static bool attribs_are_consistent(GLXX_SERVER_STATE_T *state, GLXX_ATTRIB_T *attribs)
{
   int i;
   /*
      If a glBindBuffer call has failed due to an out of memory condition, the client's estimate
      of which attributes have buffers bound can differ from reality. If this happens, we cause
      subsequent glDrawArrays() and glDrawElements() calls to fail with GL_OUT_OF_MEMORY.
   */

   for (i = 0; i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++)
      if (attribs[i].enabled && ((state->bound_buffer.mh_attrib_array[i] == MEM_INVALID_HANDLE) ^ (attribs[i].buffer == 0)))
         return false;

   return true;
}

static MEM_HANDLE_T buffer_handle(MEM_HANDLE_T handle)
{
   if (handle != MEM_INVALID_HANDLE) {
      GLXX_BUFFER_T *buffer = (GLXX_BUFFER_T *)mem_lock(handle);
      MEM_HANDLE_T hresult = glxx_buffer_get_storage_handle(buffer);

      mem_unlock(handle);
      return hresult;
   } else
      return MEM_INVALID_HANDLE;
}

static MEM_HANDLE_T buffer_interlock_offset(MEM_HANDLE_T handle)
{
   if (handle != MEM_INVALID_HANDLE) {
      GLXX_BUFFER_T *buffer = (GLXX_BUFFER_T *)mem_lock(handle);
      uint32_t offset = glxx_buffer_get_interlock_offset(buffer);
      vcos_assert(offset<=sizeof(GLXX_BUFFER_T));
      mem_unlock(handle);
      return offset;
   } else
      return ~0;
}

static bool valid_frame_buffer(GLXX_SERVER_STATE_T *state) 
{
   return state->mh_bound_framebuffer != MEM_INVALID_HANDLE;
}

// We don't need access to the draw, depth and stencil images. The only things
// which care about them are the HAL itself, which does its own thing, and the
// bit depth queries, which only require the image format.
KHRN_IMAGE_FORMAT_T glxx_get_draw_image_format(GLXX_SERVER_STATE_T *state)
{
   KHRN_IMAGE_FORMAT_T result = IMAGE_FORMAT_INVALID;
   if (valid_frame_buffer(state)) {
      GLXX_FRAMEBUFFER_T *framebuffer = (GLXX_FRAMEBUFFER_T *)mem_lock(state->mh_bound_framebuffer);

      MEM_HANDLE_T himage = glxx_attachment_info_get_image(&framebuffer->attachments.color);
      if (himage != MEM_INVALID_HANDLE) {
         KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)mem_lock(himage);   //TODO is this valid?
         result = image->format;

         mem_unlock(himage);
      }
      mem_unlock(state->mh_bound_framebuffer);
   } else {
      KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)mem_lock(state->mh_draw);
      result = image->format;

      mem_unlock(state->mh_draw);
   }
   return result;
}

KHRN_IMAGE_FORMAT_T glxx_get_depth_image_format(GLXX_SERVER_STATE_T *state)
{
   KHRN_IMAGE_FORMAT_T result = IMAGE_FORMAT_INVALID;
   if (valid_frame_buffer(state)) {
      GLXX_FRAMEBUFFER_T *framebuffer = (GLXX_FRAMEBUFFER_T *)mem_lock(state->mh_bound_framebuffer);
      MEM_HANDLE_T rbhandle = framebuffer->attachments.depth.mh_object;

      if (rbhandle != MEM_INVALID_HANDLE) {
         GLXX_RENDERBUFFER_T *renderbuffer = (GLXX_RENDERBUFFER_T *)mem_lock(rbhandle);
         switch (renderbuffer->type) {
         case RB_DEPTH16_T:
            result = DEPTH_16_TF;
            break;
         case RB_DEPTH24_T:
            result = DEPTH_32_TF;
            break;
         default:
         /* default: leave as IMAGE_FORMAT_INVALID */
            break;
         }
         mem_unlock(rbhandle);
      }

      mem_unlock(state->mh_bound_framebuffer);
   } else if (state->mh_depth != MEM_INVALID_HANDLE) {
      KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)mem_lock(state->mh_depth);
      result = image->format;
#ifdef __VIDEOCORE4__
      vcos_assert(result == DEPTH_16_TF || result == DEPTH_32_TF || result == DEPTH_32_TLBD || result == DEPTH_COL_64_TLBD);
#else
      vcos_assert(result == DEPTH_16_TF || result == DEPTH_32_TF);
#endif
      mem_unlock(state->mh_depth);
   } // else leave as IMAGE_FORMAT_INVALID
   return result;
}

static KHRN_IMAGE_FORMAT_T get_stencil_image_format(GLXX_SERVER_STATE_T *state)
{
   KHRN_IMAGE_FORMAT_T result = IMAGE_FORMAT_INVALID;
   if (valid_frame_buffer(state)) {
      GLXX_FRAMEBUFFER_T *framebuffer = (GLXX_FRAMEBUFFER_T *)mem_lock(state->mh_bound_framebuffer);
      MEM_HANDLE_T rbhandle = framebuffer->attachments.stencil.mh_object;

      if (rbhandle != MEM_INVALID_HANDLE) {
         GLXX_RENDERBUFFER_T *renderbuffer = (GLXX_RENDERBUFFER_T *)mem_lock(rbhandle);
         switch (renderbuffer->type) {
         case RB_STENCIL_T:
            result = DEPTH_32_TF;
            break;
         default:
         /* default: leave as IMAGE_FORMAT_INVALID */
            break;
         }
         mem_unlock(rbhandle);
      }

      mem_unlock(state->mh_bound_framebuffer);
      return result;
   } else if (state->mh_depth != MEM_INVALID_HANDLE) {
      KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)mem_lock(state->mh_depth);
      result = image->format;
#ifdef __VIDEOCORE4__
      vcos_assert(result == DEPTH_16_TF || result == DEPTH_32_TF || result == DEPTH_32_TLBD || result == DEPTH_COL_64_TLBD);
#else
      vcos_assert(result == DEPTH_16_TF || result == DEPTH_32_TF);
#endif
      mem_unlock(state->mh_depth);
   } // else leave as IMAGE_FORMAT_INVALID
   return result;
}

/*
   uint32_t get_stencil_size(GLXX_SERVER_STATE_T *state)

   Returns the number of stencil bits per pixel in the stencil buffer, or 0 if there is
   no stencil buffer.

   Preconditions:

   state is a valid GLXX_SERVER_STATE_T object;

   Postconditions:

   result < 32.
*/

uint32_t glxx_get_stencil_size(GLXX_SERVER_STATE_T *state)
{
   uint32_t result = 0;
   KHRN_IMAGE_FORMAT_T format = get_stencil_image_format(state);
   if(format!=IMAGE_FORMAT_INVALID)
      result = khrn_image_get_stencil_size(format);
   return result;
}

/*
   initialises common portions of the GLXX_SERVER_STATE_T state structure
   this function is called by the OpenGL ES version specific functions
   gl11_server_state_init and gl20_server_state_init
*/

bool glxx_server_state_init(GLXX_SERVER_STATE_T *state, uint32_t name, uint64_t pid, MEM_HANDLE_T shared)
{
   int i;
   MEM_HANDLE_T texture, cache;
   MEM_HANDLE_T texture_cube = MEM_INVALID_HANDLE;

   state->name = name;
   state->pid = pid;

   MEM_ASSIGN(state->mh_shared, shared);

   /*
      do texture stuff first so that we have
      the default texture to assign to the texture units
   */

   texture = MEM_ALLOC_STRUCT_EX(GLXX_TEXTURE_T, MEM_COMPACT_DISCARD);         // check, texture term
   if (texture == MEM_INVALID_HANDLE)
      return false;

   mem_set_term(texture, glxx_texture_term);

   glxx_texture_init((GLXX_TEXTURE_T *)mem_lock(texture), 0, false);
   mem_unlock(texture);

   MEM_ASSIGN(state->mh_default_texture_twod, texture);

   mem_release(texture);

   if(!IS_GL_11(state)) {
      /*
         And the cube map texture too
      */
      texture_cube = MEM_ALLOC_STRUCT_EX(GLXX_TEXTURE_T, MEM_COMPACT_DISCARD);
      if (texture_cube == MEM_INVALID_HANDLE)
         return false;
   
      mem_set_term(texture_cube, glxx_texture_term);
   
      glxx_texture_init((GLXX_TEXTURE_T *)mem_lock(texture_cube), 0, true);
      mem_unlock(texture_cube);
   
      MEM_ASSIGN(state->mh_default_texture_cube, texture_cube);
   
      mem_release(texture_cube);
   }
   /*
      now the rest of the structure
   */

   state->active_texture = GL_TEXTURE0;

   state->error = GL_NO_ERROR;

   vcos_assert(state->bound_buffer.mh_array == MEM_INVALID_HANDLE);
   vcos_assert(state->bound_buffer.mh_element_array == MEM_INVALID_HANDLE);
   
   if(IS_GL_11(state)) {
      for (i = 0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++) {
         MEM_ASSIGN(state->bound_texture[i].mh_twod, texture);
      }
   } else {
      for (i = 0; i < GL20_CONFIG_MAX_TEXTURE_UNITS; i++) {
         MEM_ASSIGN(state->bound_texture[i].mh_twod, texture);
         MEM_ASSIGN(state->bound_texture[i].mh_cube, texture_cube);
      }
   }

   state->clear_color[0] = 0.0f;
   state->clear_color[1] = 0.0f;
   state->clear_color[2] = 0.0f;
   state->clear_color[3] = 0.0f;

   state->clear_depth = 1.0f;

   state->clear_stencil = 0;

   state->shader.common.blend.color_mask = 0xffffffff;

   state->cull_mode = GL_BACK;

   state->depth_func = GL_LESS;

   state->depth_mask = GL_TRUE;

   state->caps.cull_face = false;
   state->caps.polygon_offset_fill = false;
   state->caps.sample_coverage = false;
   state->caps.sample_alpha_to_coverage = false;
   state->caps.scissor_test = false;
   state->caps.stencil_test = false;
   state->caps.depth_test = false;
   state->caps.blend = false;
   state->caps.dither = true;
   state->caps.multisample = true;
   
   state->front_face = GL_CCW;

   state->hints.generate_mipmap = GL_DONT_CARE;
   
   state->line_width = 1.0f;
   
   state->polygon_offset.factor = 0.0f;
   state->polygon_offset.units = 0.0f;

   state->sample_coverage.value = 1.0f;
   state->sample_coverage.invert = GL_FALSE;

   state->scissor.x = 0;
   state->scissor.y = 0;
   state->scissor.width = 0;
   state->scissor.height = 0;

   state->stencil_func.front.func = GL_ALWAYS;
   state->stencil_func.front.ref = 0;
   state->stencil_func.front.mask = 0xffffffff;
   state->stencil_mask.front = 0xffffffff;
   
   state->stencil_op.front.fail = GL_KEEP;
   state->stencil_op.front.zfail = GL_KEEP;
   state->stencil_op.front.zpass = GL_KEEP;
   
   state->stencil_func.back.func = GL_ALWAYS;
   state->stencil_func.back.ref = 0;
   state->stencil_func.back.mask = 0xffffffff;
   state->stencil_mask.back = 0xffffffff;
   
   state->stencil_op.back.fail = GL_KEEP;
   state->stencil_op.back.zfail = GL_KEEP;
   state->stencil_op.back.zpass = GL_KEEP;
   
   state->blend_func.src_rgb = GL_ONE;
   state->blend_func.dst_rgb = GL_ZERO;
   state->blend_func.src_alpha = GL_ONE;
   state->blend_func.dst_alpha = GL_ZERO;

   state->blend_color[0] = 0.0f;
   state->blend_color[1] = 0.0f;
   state->blend_color[2] = 0.0f;
   state->blend_color[3] = 0.0f;

   state->blend_equation.rgb = GL_FUNC_ADD;
   state->blend_equation.alpha = GL_FUNC_ADD;
   
   state->viewport.x = 0;
   state->viewport.y = 0;
   state->viewport.width = 0;
   state->viewport.height = 0;
   state->viewport.near = 0.0f;
   state->viewport.far = 1.0f;

   glxx_update_viewport_internal(state);

   vcos_assert(state->mh_draw == MEM_INVALID_HANDLE);
   vcos_assert(state->mh_read == MEM_INVALID_HANDLE);
   vcos_assert(state->mh_depth == MEM_INVALID_HANDLE);
   vcos_assert(state->mh_multi == MEM_INVALID_HANDLE);

   state->changed_misc = IS_GL_11(state);
   state->changed_light = IS_GL_11(state);
   state->changed_texunit = true;
   state->changed_backend = true;
   state->changed_vertex = true;
   state->changed_directly = true;

   state->current_render_state = 0;    /* = No render state */
   state->changed_cfg = true;
   state->changed_linewidth = true;
   state->changed_polygon_offset = true;
   state->changed_viewport = true;
   state->old_flat_shading_flags = ~0;

   state->made_current = GL_FALSE;

#ifdef __VIDEOCORE4__
   //discardable and retained so can treat it like VBOs
   cache = mem_alloc_ex(0, 4, (MEM_FLAG_T)(MEM_FLAG_RESIZEABLE | MEM_FLAG_HINT_GROW | MEM_FLAG_DIRECT | MEM_FLAG_DISCARDABLE | MEM_FLAG_RETAINED),
      "GLXX_SERVER_STATE_T.cache", MEM_COMPACT_DISCARD);
#else
   cache = mem_alloc_ex(0, 4, (MEM_FLAG_T)(MEM_FLAG_RESIZEABLE | MEM_FLAG_HINT_GROW), "GLXX_SERVER_STATE_T.cache", MEM_COMPACT_DISCARD);
#endif
#if 0
   if (!cache)
      return false;
#endif
   MEM_ASSIGN(state->mh_cache, cache);

   mem_release(cache);

   vcos_assert(state->mh_temp_palette == MEM_INVALID_HANDLE);
   
   vcos_assert(state->mh_bound_renderbuffer == MEM_INVALID_HANDLE);
   vcos_assert(state->mh_bound_framebuffer == MEM_INVALID_HANDLE);
   
   return true;
}

void glxx_server_state_flush(GLXX_SERVER_STATE_T *state, bool wait)
{
   glxx_hw_finish_context(state, wait);
}

void glxx_server_state_flush1(GLXX_SERVER_STATE_T *state, bool wait)
{
   glxx_hw_finish();
}

GLXX_TEXTURE_T *glxx_server_state_get_texture(GLXX_SERVER_STATE_T *state, GLenum target, GLboolean use_face, MEM_HANDLE_T *handle)
{
   MEM_HANDLE_T thandle = MEM_INVALID_HANDLE;

   switch (target) {
   case GL_TEXTURE_2D:
   case GL_TEXTURE_EXTERNAL_OES:
      thandle = state->bound_texture[state->active_texture - GL_TEXTURE0].mh_twod;
      break;
   case GL_TEXTURE_CUBE_MAP:
      if (IS_GL_11(state)) {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);//gl 2.0 only
         return NULL;
      }
      else {
         if (!use_face)
            thandle = state->bound_texture[state->active_texture - GL_TEXTURE0].mh_cube;
      }
      break;
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      if (IS_GL_11(state)) {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);//gl 2.0 only
         return NULL;
      }
      else {
         if (use_face)
            thandle = state->bound_texture[state->active_texture - GL_TEXTURE0].mh_cube;
      }
      break;
   }

   if (thandle == MEM_INVALID_HANDLE) {
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
      return NULL;
   }

   if (handle)
      *handle = thandle;

   return (GLXX_TEXTURE_T *)mem_lock(thandle);
}

void glxx_server_state_set_buffers(GLXX_SERVER_STATE_T *state, MEM_HANDLE_T hdraw, MEM_HANDLE_T hread, MEM_HANDLE_T hdepth, MEM_HANDLE_T hmulti)
{
   KHRN_IMAGE_T *draw = (KHRN_IMAGE_T *)mem_lock(hdraw);

#ifndef NDEBUG
   /*/////////////// */
   /* All this stuff only used for assertions */
   KHRN_IMAGE_T *depth = (KHRN_IMAGE_T *)mem_maybe_lock(hdepth);
   KHRN_IMAGE_T *multi = (KHRN_IMAGE_T *)mem_maybe_lock(hmulti);

   vcos_assert(draw->width <= GLXX_CONFIG_MAX_VIEWPORT_SIZE);
   vcos_assert(draw->height <= GLXX_CONFIG_MAX_VIEWPORT_SIZE);

   if (multi) {
      vcos_assert(!depth || ((depth->width >= multi->width) && (depth->height >= multi->height)));
      vcos_assert((multi->width == 2 * draw->width) && (multi->height == 2 * draw->height));
   } else
      vcos_assert(!depth || ((depth->width >= draw->width) && (depth->height >= draw->height)));

   mem_maybe_unlock(hdepth);
   mem_maybe_unlock(hmulti);
   /* */
   /*/////////////// */
#endif

   MEM_ASSIGN(state->mh_draw, hdraw);
   MEM_ASSIGN(state->mh_read, hread);
   MEM_ASSIGN(state->mh_depth, hdepth);
   MEM_ASSIGN(state->mh_multi, hmulti);

   if (!state->made_current) {
      state->scissor.x = 0;
      state->scissor.y = 0;
      state->scissor.width = draw->width;
      state->scissor.height = draw->height;

      state->viewport.x = 0;
      state->viewport.y = 0;
      state->viewport.width = draw->width;
      state->viewport.height = draw->height;

      glxx_update_viewport_internal(state);

#ifdef KHRN_SIMPLE_MULTISAMPLE
      state->scissor.width /= 2;
      state->scissor.height /= 2;
      state->viewport.width /= 2;
      state->viewport.height /= 2;
#endif
      state->made_current = GL_TRUE;
   }

   mem_unlock(hdraw);
}

void glxx_server_state_term(void *v, uint32_t size)
{
   GLXX_SERVER_STATE_T *state = (GLXX_SERVER_STATE_T *)v;

   int i;

   UNUSED_NDEBUG(size);
   vcos_assert(size == sizeof(GLXX_SERVER_STATE_T));

   // We want to make sure that any external buffers modified by this context
   // get updated.
   glxx_server_state_flush(state, true); /* todo: do we need to wait here? */

   if (!IS_GL_11(state)) {
      MEM_ASSIGN(state->mh_program, MEM_INVALID_HANDLE);
   }
   
   MEM_ASSIGN(state->mh_bound_renderbuffer, MEM_INVALID_HANDLE);
   MEM_ASSIGN(state->mh_bound_framebuffer, MEM_INVALID_HANDLE);
  
   MEM_ASSIGN(state->bound_buffer.mh_array, MEM_INVALID_HANDLE);
   MEM_ASSIGN(state->bound_buffer.mh_element_array, MEM_INVALID_HANDLE);

   for (i = 0; i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++)
      MEM_ASSIGN(state->bound_buffer.mh_attrib_array[i], MEM_INVALID_HANDLE);

   for (i = 0; i < GLXX_CONFIG_MAX_TEXTURE_UNITS; i++) {
      MEM_ASSIGN(state->bound_texture[i].mh_twod, MEM_INVALID_HANDLE);
      MEM_ASSIGN(state->bound_texture[i].mh_cube, MEM_INVALID_HANDLE);
   }

   MEM_ASSIGN(state->mh_shared, MEM_INVALID_HANDLE);
   MEM_ASSIGN(state->mh_default_texture_twod, MEM_INVALID_HANDLE);
   MEM_ASSIGN(state->mh_default_texture_cube, MEM_INVALID_HANDLE);

   MEM_ASSIGN(state->mh_draw, MEM_INVALID_HANDLE);
   MEM_ASSIGN(state->mh_read, MEM_INVALID_HANDLE);
   MEM_ASSIGN(state->mh_depth, MEM_INVALID_HANDLE);
   MEM_ASSIGN(state->mh_multi, MEM_INVALID_HANDLE);

   mem_unretain(state->mh_cache);
   MEM_ASSIGN(state->mh_cache, MEM_INVALID_HANDLE);

   MEM_ASSIGN(state->mh_temp_palette, MEM_INVALID_HANDLE);
}

//TODO are there any other operations which need a valid framebuffer and
//therefore need to call this function, that I've forgotten about?
GLenum glxx_check_framebuffer_status (GLXX_SERVER_STATE_T *state, GLenum target)
{
   GLenum result = GL_FRAMEBUFFER_COMPLETE;

   vcos_assert(state);

   if (target == GL_FRAMEBUFFER) {
      if (valid_frame_buffer(state)) {
         GLXX_FRAMEBUFFER_T *framebuffer = (GLXX_FRAMEBUFFER_T *)mem_lock(state->mh_bound_framebuffer);

         result = glxx_framebuffer_check_status(framebuffer);

         mem_unlock(state->mh_bound_framebuffer);
      } else {
         // If there is no bound buffer then we are using the
         // window-system-provided framebuffer. This is always complete.
      }
   } else {
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
      result = 0;
   }

   return result;
}

/*
   glBlendFuncSeparate()

   Sets the RGB and alpha source and destination blend functions to one of

      GL_ZERO
      GL_ONE
      GL_SRC_COLOR
      GL_ONE_MINUS_SRC_COLOR
      GL_DST_COLOR
      GL_ONE_MINUS_DST_COLOR
      GL_SRC_ALPHA
      GL_ONE_MINUS_SRC_ALPHA
      GL_DST_ALPHA
      GL_ONE_MINUS_DST_ALPHA
      GL_CONSTANT_COLOR
      GL_ONE_MINUS_CONSTANT_COLOR
      GL_CONSTANT_ALPHA
      GL_ONE_MINUS_CONSTANT_ALPHA
      GL_SRC_ALPHA_SATURATE*

   Gives GL_INVALID_ENUM error if any function is not one of these.

   *source functions only

   Implementation: Done
   Error Checks: Done
*/

static GLboolean is_blend_func(GLXX_SERVER_STATE_T *state, GLenum func, GLboolean is_src)
{
   //gl 1.1
   return func == GL_ZERO ||
          func == GL_ONE ||
          func == GL_SRC_ALPHA ||
          func == GL_ONE_MINUS_SRC_ALPHA ||
          func == GL_DST_ALPHA ||
          func == GL_ONE_MINUS_DST_ALPHA ||
          (func == GL_SRC_ALPHA_SATURATE && is_src) ||
          (IS_GL_11(state) && (
                (func == GL_SRC_COLOR && !is_src) ||
                (func == GL_ONE_MINUS_SRC_COLOR && !is_src) ||
                (func == GL_DST_COLOR && is_src) ||
                (func == GL_ONE_MINUS_DST_COLOR && is_src)
                )) ||
          (!IS_GL_11(state) && (
             func == GL_SRC_COLOR ||
             func == GL_ONE_MINUS_SRC_COLOR ||
             func == GL_DST_COLOR ||
             func == GL_ONE_MINUS_DST_COLOR ||
             func == GL_CONSTANT_COLOR ||
             func == GL_ONE_MINUS_CONSTANT_COLOR ||
             func == GL_CONSTANT_ALPHA ||
             func == GL_ONE_MINUS_CONSTANT_ALPHA
          ));
}

void glBlendFuncSeparate_impl (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) // S
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (is_blend_func(state, srcRGB, GL_TRUE) && is_blend_func(state, dstRGB, GL_FALSE) &&
       is_blend_func(state, srcAlpha, GL_TRUE) && is_blend_func(state, dstAlpha, GL_FALSE))
   {
      state->changed_backend = true;
      state->blend_func.src_rgb = srcRGB;
      state->blend_func.dst_rgb = dstRGB;
      state->blend_func.src_alpha = srcAlpha;
      state->blend_func.dst_alpha = dstAlpha;
   } else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
   GLXX_UNLOCK_SERVER_STATE();
}

void glClear_impl (GLbitfield mask)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
      glxx_server_state_set_error(state, GL_INVALID_VALUE);
   else if (glxx_check_framebuffer_status(state, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      glxx_server_state_set_error(state, GL_INVALID_FRAMEBUFFER_OPERATION);
   else if (mask != 0 && !glxx_hw_clear(mask & GL_COLOR_BUFFER_BIT ? GL_TRUE : GL_FALSE,
                   mask & GL_DEPTH_BUFFER_BIT ? GL_TRUE : GL_FALSE,
                   mask & GL_STENCIL_BUFFER_BIT ? GL_TRUE : GL_FALSE,
                   state))
      glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);                   

   GLXX_UNLOCK_SERVER_STATE();
}


static KHRN_IMAGE_FORMAT_T set_palette(GLXX_SERVER_STATE_T *state, GLenum internalformat, const void *data)
{
   MEM_HANDLE_T hpalette;
   KHRN_IMAGE_FORMAT_T format;
   uint32_t *palette;
   uint32_t i, palsize;
   const uint8_t *ptr = (const uint8_t *)data;

   switch (internalformat)
   {
      case GL_PALETTE4_RGB8_OES:
         palsize = 16;
         format = XBGR_8888_TF;
         break;
      case GL_PALETTE4_RGBA8_OES:
         palsize = 16;
         format = ABGR_8888_TF;
         break;
      case GL_PALETTE4_R5_G6_B5_OES:
         palsize = 16;
         format = RGB_565_TF;
         break;
      case GL_PALETTE4_RGBA4_OES:
         palsize = 16;
         format = RGBA_4444_TF;
         break;
      case GL_PALETTE4_RGB5_A1_OES:
         palsize = 16;
         format = RGBA_5551_TF;
         break;
      case GL_PALETTE8_RGB8_OES:
         palsize = 256;
         format = XBGR_8888_TF;
         break;
      case GL_PALETTE8_RGBA8_OES:
         palsize = 256;
         format = ABGR_8888_TF;
         break;
      case GL_PALETTE8_R5_G6_B5_OES:
         palsize = 256;
         format = RGB_565_TF;
         break;
      case GL_PALETTE8_RGBA4_OES:
         palsize = 256;
         format = RGBA_4444_TF;
         break;
      case GL_PALETTE8_RGB5_A1_OES:
         palsize = 256;
         format = RGBA_5551_TF;
         break;
      default:
         palsize = 0;
         format = IMAGE_FORMAT_INVALID;
         UNREACHABLE();
   }

   hpalette = mem_alloc_ex(4 * palsize, 4, MEM_FLAG_NONE, "Temporary palette allocation", MEM_COMPACT_DISCARD);

   if (hpalette == MEM_HANDLE_INVALID)
   {
      return IMAGE_FORMAT_INVALID;
   }

   palette = (uint32_t *)mem_lock(hpalette);

   for (i = 0; i < palsize; i++)
   {
      switch (internalformat)
      {
      case GL_PALETTE4_RGB8_OES:
      case GL_PALETTE8_RGB8_OES:
         palette[i] = (uint32_t)ptr[0] | (uint32_t)ptr[1] << 8 | (uint32_t)ptr[2] << 16 | 0xff000000;
         ptr += 3;
         break;
      case GL_PALETTE4_RGBA8_OES:
      case GL_PALETTE8_RGBA8_OES:
         palette[i] = *(uint32_t*)ptr;
         ptr += 4;
         break;
      case GL_PALETTE4_R5_G6_B5_OES:
      case GL_PALETTE4_RGBA4_OES:
      case GL_PALETTE4_RGB5_A1_OES:
      case GL_PALETTE8_R5_G6_B5_OES:
      case GL_PALETTE8_RGBA4_OES:
      case GL_PALETTE8_RGB5_A1_OES:
         palette[i] = *(uint16_t*)ptr;
         ptr += 2;
         break;
      default:
         UNREACHABLE();
      }
   }

   mem_unlock(hpalette);

   MEM_ASSIGN(state->mh_temp_palette, hpalette);
   mem_release(hpalette);

   return format;
}

GLboolean glCompressedTexImage2D_impl (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();
   GLboolean res = GL_FALSE;

   UNUSED(imageSize);

   switch (internalformat)
   {
   case GL_ETC1_RGB8_OES:
   {
      //TODO check imageSize field is correct
      MEM_HANDLE_T thandle;
      GLXX_TEXTURE_T *texture = glxx_server_state_get_texture(state, target, GL_TRUE, &thandle);

      vcos_assert(data == NULL);    /* Data is supplied later in glCompressedTexSubImage2D calls */
      state->changed_texunit = true; /* GLES1.1 - may change has_color/has_alpha */
      if (texture) {
         if (level < 0 || level > LOG2_MAX_TEXTURE_SIZE)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (width < 0 || width > MAX_TEXTURE_SIZE || height < 0 || height > MAX_TEXTURE_SIZE)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (border != 0)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (IS_GL_11(state) && target != GL_TEXTURE_2D && target != GL_TEXTURE_EXTERNAL_OES)
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
         else if (!IS_GL_11(state) && target != GL_TEXTURE_2D && target != GL_TEXTURE_EXTERNAL_OES && width != height)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (!glxx_texture_etc1_blank_image(texture, target, level, width, height))
            glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
         else
            res = GL_TRUE;

         mem_unlock(thandle);
      }
      break;
   }
   case GL_PALETTE4_RGB8_OES:
   case GL_PALETTE4_RGBA8_OES:
   case GL_PALETTE4_R5_G6_B5_OES:
   case GL_PALETTE4_RGBA4_OES:
   case GL_PALETTE4_RGB5_A1_OES:
   case GL_PALETTE8_RGB8_OES:
   case GL_PALETTE8_RGBA8_OES:
   case GL_PALETTE8_R5_G6_B5_OES:
   case GL_PALETTE8_RGBA4_OES:
   case GL_PALETTE8_RGB5_A1_OES:
   {
      //TODO check imageSize field is correct
      MEM_HANDLE_T thandle;
      GLXX_TEXTURE_T *texture = glxx_server_state_get_texture(state, target, GL_TRUE, &thandle);
      if (texture) {
         if (data == NULL)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (level < 0 || level > LOG2_MAX_TEXTURE_SIZE)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (width < 0 || width > MAX_TEXTURE_SIZE || height < 0 || height > MAX_TEXTURE_SIZE)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (1<<level > width && 1<<level > height)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);   //TODO is this a valid check?
         else if (border != 0)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (IS_GL_11(state) && target != GL_TEXTURE_2D && target != GL_TEXTURE_EXTERNAL_OES)
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
         else if (!IS_GL_11(state) && target != GL_TEXTURE_2D && target != GL_TEXTURE_EXTERNAL_OES && width != height)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else
         {
            KHRN_IMAGE_FORMAT_T format;
            state->changed_texunit = true;   /* May change has_color/_alpha */
            format = set_palette(state, internalformat, data);
            if (format == IMAGE_FORMAT_INVALID)
               glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
            else if (!glxx_texture_paletted_blank_image(texture, target, level, width, height, format))
               glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
            else
               res = GL_TRUE;
         }

         mem_unlock(thandle);
      }
      break;
   }
   default:
      // Some format we don't recognise
      glxx_server_state_set_error(state, GL_INVALID_VALUE);
      break;
   }
   GLXX_UNLOCK_SERVER_STATE();
   return res;
}

void glCompressedTexSubImage2D_impl (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();
   MEM_HANDLE_T thandle;
   GLXX_TEXTURE_T *texture;

   switch(format)
   {
   case GL_ETC1_RGB8_OES:
   {
      texture = glxx_server_state_get_texture(state, target, GL_TRUE, &thandle);
      if (texture) {
         if (level < 0 || level > LOG2_MAX_TEXTURE_SIZE)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (width < 0 || width > MAX_TEXTURE_SIZE || height < 0 || height > MAX_TEXTURE_SIZE)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else
            glxx_texture_etc1_sub_image(texture, target, level, xoffset, yoffset, width, height, data);

         mem_unlock(thandle);
      }
      break;
   }
   case GL_PALETTE4_RGB8_OES:
   case GL_PALETTE4_RGBA8_OES:
   case GL_PALETTE4_R5_G6_B5_OES:
   case GL_PALETTE4_RGBA4_OES:
   case GL_PALETTE4_RGB5_A1_OES:
   case GL_PALETTE8_RGB8_OES:
   case GL_PALETTE8_RGBA8_OES:
   case GL_PALETTE8_R5_G6_B5_OES:
   case GL_PALETTE8_RGBA4_OES:
   case GL_PALETTE8_RGB5_A1_OES:
   {
      vcos_assert(yoffset == 0);  /* We have hacked this so xoffset gives offset into data, yoffset is unused */
      texture = glxx_server_state_get_texture(state, target, GL_TRUE, &thandle);
      if (texture) {
         if (level < 0 || level > LOG2_MAX_TEXTURE_SIZE)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (width < 0 || width > MAX_TEXTURE_SIZE || height < 0 || height > MAX_TEXTURE_SIZE)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (state->mh_temp_palette != MEM_HANDLE_INVALID)
         {
            glxx_texture_paletted_sub_image(texture, target, level, width, height,
               (const uint32_t *)mem_lock(state->mh_temp_palette),
               mem_get_size(state->mh_temp_palette) / 4,
               data, xoffset, imageSize);
            mem_unlock(state->mh_temp_palette);
         }

         mem_unlock(thandle);
      }
      break;
   }
   default:
      UNREACHABLE();
   }
   GLXX_UNLOCK_SERVER_STATE();
}

static int format_is_copy_compatible(GLenum internalformat, KHRN_IMAGE_FORMAT_T srcformat)
{
   switch (srcformat & ~IMAGE_FORMAT_PRE)
   {
   case ABGR_8888_TF:
   case RGBA_4444_TF:
   case RGBA_5551_TF:
#ifdef __VIDEOCORE4__
   case ABGR_8888_RSO:
#endif
      return internalformat == GL_RGBA
          || internalformat == GL_RGB
          || internalformat == GL_LUMINANCE_ALPHA
          || internalformat == GL_LUMINANCE
          || internalformat == GL_ALPHA;
   case XBGR_8888_TF:
   case RGB_565_TF:
#ifdef __VIDEOCORE4__
   case XBGR_8888_RSO:
   case RGB_565_RSO:
#endif
      return internalformat == GL_RGB
          || internalformat == GL_LUMINANCE;
   default:
      // Source image should be a framebuffer. Other image formats aren't
      // used for framebuffers.
      // Unreachable
      UNREACHABLE();
      return 0;
   }
}

static GLboolean is_texture_internal_format(GLenum internalformat)
{
   return internalformat == GL_ALPHA ||
          internalformat == GL_LUMINANCE ||
          internalformat == GL_LUMINANCE_ALPHA ||
          internalformat == GL_RGB ||
          internalformat == GL_RGBA ||
#if GL_EXT_texture_format_BGRA8888
          internalformat == GL_BGRA_EXT || 
#endif
          0;
}

static MEM_HANDLE_T get_read_image(GLXX_SERVER_STATE_T *state)
{
   if (valid_frame_buffer(state)) {
      GLXX_FRAMEBUFFER_T *framebuffer = (GLXX_FRAMEBUFFER_T *)mem_lock(state->mh_bound_framebuffer);

      MEM_HANDLE_T result = glxx_attachment_info_get_image(&framebuffer->attachments.color);

      vcos_assert(result != MEM_INVALID_HANDLE);      //TODO: is this valid?

      mem_unlock(state->mh_bound_framebuffer);
      return result;
   } else {
      return state->mh_read;
   }
}

void glCopyTexImage2D_impl (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
   MEM_HANDLE_T thandle;
   GLXX_TEXTURE_T *texture;
   
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   vcos_assert(state);

   if (glxx_check_framebuffer_status(state, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      glxx_server_state_set_error(state, GL_INVALID_FRAMEBUFFER_OPERATION);
   else
   {
      texture = glxx_server_state_get_texture(state, target, GL_TRUE, &thandle);

      if (texture) {
         if (level < 0 || level > LOG2_MAX_TEXTURE_SIZE)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (width < 0 || width > MAX_TEXTURE_SIZE || height < 0 || height > MAX_TEXTURE_SIZE)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (IS_GL_11(state) && target != GL_TEXTURE_2D && target != GL_TEXTURE_EXTERNAL_OES)
               glxx_server_state_set_error(state, GL_INVALID_ENUM);
         else if (!IS_GL_11(state) && target != GL_TEXTURE_2D && target != GL_TEXTURE_EXTERNAL_OES && width != height) 
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (border != 0 || !is_texture_internal_format(internalformat))
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else
         {
            MEM_HANDLE_T hsrc = get_read_image(state);
            KHRN_IMAGE_T *src = (KHRN_IMAGE_T *)mem_lock(hsrc);
            if (!format_is_copy_compatible(internalformat, src->format))
               glxx_server_state_set_error(state, GL_INVALID_OPERATION);
            else
            {
               khrn_interlock_read_immediate(&src->interlock);

               state->changed_texunit = true; /* GLES1.1 - may change has_color/has_alpha */
#if defined(BCG_FB_LAYOUT) || defined(BCG_FB_LAYOUT_B1)
               if (khrn_image_is_rso(src->format))
               {
                  if (!glxx_texture_copy_image(texture, target, level, width, height, internalformat, src, x, src->height - y - height))
                     glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
               }
               else
               {
#endif /* BCG_FB_LAYOUT || BCG_FB_LAYOUT_B1 */
                  if (!glxx_texture_copy_image(texture, target, level, width, height, internalformat, src, x, y))
                     glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
#if defined(BCG_FB_LAYOUT) || defined(BCG_FB_LAYOUT_B1)
               }
#endif /* BCG_FB_LAYOUT || BCG_FB_LAYOUT_B1 */
            }
            mem_unlock(hsrc);
         }
         mem_unlock(thandle);
      }
   }
   GLXX_UNLOCK_SERVER_STATE();
}

GLboolean glxx_is_texture_target(GLXX_SERVER_STATE_T *state, GLenum target)
{
   return   target == GL_TEXTURE_2D ||
            target == GL_TEXTURE_EXTERNAL_OES ||
            (!IS_GL_11(state) && (
          target == GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
          target == GL_TEXTURE_CUBE_MAP_NEGATIVE_X ||
          target == GL_TEXTURE_CUBE_MAP_POSITIVE_Y ||
          target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y ||
          target == GL_TEXTURE_CUBE_MAP_POSITIVE_Z ||
          target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
          ));
}

void glCopyTexSubImage2D_impl (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   MEM_HANDLE_T thandle;
   GLXX_TEXTURE_T *texture;
   
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   vcos_assert(state);

   if (glxx_check_framebuffer_status(state, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      glxx_server_state_set_error(state, GL_INVALID_FRAMEBUFFER_OPERATION);
   else
   {
      texture = glxx_server_state_get_texture(state, target, GL_TRUE, &thandle);

      if (texture) {
         if (!glxx_is_texture_target(state,target))
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
         else if (width < 0 || width > MAX_TEXTURE_SIZE || height < 0 || height > MAX_TEXTURE_SIZE)
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (!glxx_texture_includes(texture, target, level, xoffset, yoffset))
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (!glxx_texture_includes(texture, target, level, xoffset + width - 1, yoffset + height - 1))
            glxx_server_state_set_error(state, GL_INVALID_VALUE);
         else if (!khrn_image_is_uncomp(glxx_texture_incomplete_get_mipmap_format(texture, target, level)))
            glxx_server_state_set_error(state, GL_INVALID_OPERATION);
         else if (width > 0 && height > 0)
         {
            MEM_HANDLE_T hsrc = get_read_image(state);
            KHRN_IMAGE_T *src = (KHRN_IMAGE_T *)mem_lock(hsrc);
            //TODO: check formats are compatible?

            khrn_interlock_read_immediate(&src->interlock);

            if (x < 0) { xoffset -= x; width += x;  x = 0; }
            if (y < 0) { yoffset -= y; height += y; y = 0; }
            if (width > src->width   || x + width > src->width)   { width = src->width - x; }
            if (height > src->height || y + height > src->height) { height = src->height - y; }

            if (width > 0 && height > 0)
            {
               if (!glxx_texture_copy_sub_image(texture, target, level, xoffset, yoffset, width, height, src, x, y))
                  glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
            }

            mem_unlock(hsrc);
         }
         mem_unlock(thandle);
      }
   }
   GLXX_UNLOCK_SERVER_STATE();
}

/*
   Check if 'func' is a valid depth or stencil test function.
*/

static GLboolean is_func(GLenum func)
{
   return func == GL_NEVER ||
          func == GL_ALWAYS ||
          func == GL_LESS ||
          func == GL_LEQUAL ||
          func == GL_EQUAL ||
          func == GL_GREATER ||
          func == GL_GEQUAL ||
          func == GL_NOTEQUAL;
}

/*
   glDepthFunc()

   Sets the function which determines whether a pixel passes or fails the
   depth test, specifying one of

      GL_NEVER
      GL_ALWAYS
      GL_LESS
      GL_LEQUAL
      GL_EQUAL
      GL_GREATER
      GL_GEQUAL
      GL_NOTEQUAL

   Gives GL_INVALID_ENUM error if the function is not one of these.

   Implementation: Done
   Error Checks: Done
*/

void glDepthFunc_impl (GLenum func) // S
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (is_func(func))
   {
      state->changed_cfg = true;
      state->depth_func = func;
   }
   else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GLXX_UNLOCK_SERVER_STATE();
}

/*
   glDepthMask()

   Sets the write enable for the depth buffer. No errors are generated.

   Implementation: Done
   Error Checks: Done
*/

void glDepthMask_impl (GLboolean flag) // S
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   state->changed_cfg = true;
   state->depth_mask = clean_boolean(flag);

   GLXX_UNLOCK_SERVER_STATE();
}

void glxx_update_color_material(GLXX_SERVER_STATE_T *state)
{
   if (state->shader.color_material) {
      int i;

      for (i = 0; i < 4; i++) {
         state->material.ambient[i] = state->copy_of_color[i];
         state->material.diffuse[i] = state->copy_of_color[i];
      }
   }
}

static void set_enabled(GLenum cap, bool enabled)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   vcos_assert(state);

   vcos_assert(GL11_CONFIG_MAX_PLANES == 1);
   vcos_assert(GL11_CONFIG_MAX_LIGHTS == 8);

   switch (cap) {
   case GL_NORMALIZE:
      if(IS_GL_11(state)) {
         state->changed_light = true;
         state->shader.normalize = enabled;
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }
      break;
   case GL_RESCALE_NORMAL:
      if(IS_GL_11(state)) {
         state->changed_light = true;
         state->shader.rescale_normal = enabled;
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }         
      break;
   case GL_FOG:
      if(IS_GL_11(state)) {
         state->changed_misc = true;
         state->caps_fragment.fog = enabled;
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }         
      break;
   case GL_LIGHTING:
      if(IS_GL_11(state)) {
         state->changed_light = true;
         state->shader.lighting = enabled;
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }         
      break;
   case GL_COLOR_MATERIAL:
      if(IS_GL_11(state)) {
         state->changed_vertex = true;
         state->shader.color_material = enabled;
         glxx_update_color_material(state);
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }         
      break;
   case GL_LIGHT0:
   case GL_LIGHT1:
   case GL_LIGHT2:
   case GL_LIGHT3:
   case GL_LIGHT4:
   case GL_LIGHT5:
   case GL_LIGHT6:
   case GL_LIGHT7:
      if(IS_GL_11(state)) {
         state->changed_light = true;
         state->lights[cap - GL_LIGHT0].enabled = enabled;
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }         
      break;
   case GL_TEXTURE_2D:
   case GL_TEXTURE_EXTERNAL_OES:
      if(IS_GL_11(state)) {
         state->changed_texunit = true;
         state->texunits[state->active_texture - GL_TEXTURE0].enabled = enabled;
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }         
      break;
   case GL_ALPHA_TEST:
      if(IS_GL_11(state)) {
         state->changed_misc = true;
         state->caps_fragment.alpha_test = enabled;
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }         
      break;
   case GL_CLIP_PLANE0:
      if(IS_GL_11(state)) {
         state->changed_misc = true;
         state->caps.clip_plane[cap - GL_CLIP_PLANE0] = enabled;
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }         
      break;
   case GL_POINT_SMOOTH:
      if(IS_GL_11(state)) {
         state->changed_misc = true;
         state->caps_fragment.point_smooth = enabled;
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }         
      break;
   case GL_POINT_SPRITE_OES:
      if(IS_GL_11(state)) {
         state->changed_misc = true;
         state->caps_fragment.point_sprite = enabled;
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }         
      break;
   case GL_LINE_SMOOTH:
      if(IS_GL_11(state)) {
         state->changed_misc = true;
         state->caps_fragment.line_smooth = enabled;
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }         
      break;
   case GL_CULL_FACE:
      state->changed_cfg = true;
      state->caps.cull_face = enabled;
      break;
   case GL_POLYGON_OFFSET_FILL:
      state->changed_cfg = true;
      state->caps.polygon_offset_fill = enabled;
      break;
   case GL_MULTISAMPLE:
      if(IS_GL_11(state)) {
         state->changed_cfg = true;
         state->caps.multisample = enabled;
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }         
      break;
   case GL_SAMPLE_ALPHA_TO_COVERAGE:
      state->changed_backend = true;
      state->caps.sample_alpha_to_coverage = enabled;
      break;
   case GL_SAMPLE_ALPHA_TO_ONE:
      if(IS_GL_11(state)) {
         state->changed_backend = true;
         state->caps.sample_alpha_to_one = enabled;
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }         
      break;
   case GL_SAMPLE_COVERAGE:
      state->changed_backend = true;
      state->caps.sample_coverage = enabled;
      break;
   case GL_SCISSOR_TEST:
      state->changed_viewport = true;
      state->caps.scissor_test = enabled;
      break;
   case GL_STENCIL_TEST:
      state->changed_backend = true;
      state->caps.stencil_test = enabled;
      break;
   case GL_DEPTH_TEST:
      state->changed_cfg = true;
      state->caps.depth_test = enabled;
      break;
   case GL_BLEND:
      state->changed_backend = true;
      state->caps.blend = enabled;
      break;
   case GL_DITHER:
      state->caps.dither = enabled;
      break;
   case GL_COLOR_LOGIC_OP:
      if(IS_GL_11(state)) {
         state->changed_backend = true;
         state->caps.color_logic_op = enabled;
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);      
      }         
      break;
   default:
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
      break;
   }

   GLXX_UNLOCK_SERVER_STATE();
}

bool is_dither_enabled() 
{
	GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();
	return state->caps.dither;
}
void glDisable_impl (GLenum cap) // S
{
   set_enabled(cap, false);
}

static int translate_max_index(MEM_HANDLE_T handle, const GLvoid *v, GLint size, GLenum type, GLsizei stride)
{
   if (handle != MEM_INVALID_HANDLE) {
      GLXX_BUFFER_T *buffer = (GLXX_BUFFER_T *)mem_lock(handle);
      int attrib_size, actual_stride, offset, buffer_size, result;

      vcos_assert(buffer);   // should never be possible to reach here with a deleted or non-existent buffer
      vcos_assert(size >= 1 && size <= 4);

      attrib_size = khrn_get_type_size(type) * size;
      actual_stride = stride ? stride : attrib_size;
      offset = (int)v;
      buffer_size = glxx_buffer_get_size(buffer);

      vcos_assert(actual_stride > 0);
      vcos_assert(attrib_size > 0);

      if (offset < 0 || offset + attrib_size > buffer_size)
         result = 0;    //Not even the first vertex will fit in the buffer
      else {
         result = (buffer_size - offset - attrib_size) / actual_stride + 1;
         vcos_assert(result >= 1);
         vcos_assert(offset + (result-1) * actual_stride + attrib_size <= buffer_size);
         vcos_assert(offset +  result    * actual_stride + attrib_size >  buffer_size);
      }

      mem_unlock(handle);
      return result;
   } else
      return INT_MAX;
}

/*
   Fills in attrib_handles and returns the maximum index which
   is permitted across all attributes. (INT_MAX if we don't care).
*/

static int set_attrib_handles(GLXX_SERVER_STATE_T *state, GLXX_ATTRIB_T *attribs, MEM_HANDLE_T *attrib_handles)
{
   int i;
   int result = INT_MAX;
   uint32_t live = state->batch.cattribs_live | state->batch.vattribs_live;

   for (i = 0; i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++)
      if (attribs[i].enabled && (live & 0xf << (4*i))) {
         result = _min(result,translate_max_index(state->bound_buffer.mh_attrib_array[i], attribs[i].pointer, attribs[i].size, attribs[i].type, attribs[i].stride));
         if (state->bound_buffer.mh_attrib_array[i])
            attrib_handles[i] = buffer_handle(state->bound_buffer.mh_attrib_array[i]);
         else {
		 	attrib_handles[i] = (MEM_HANDLE_T)attribs[i].handle;
			attribs[i].pointer = attribs[i].pointer;
			}
      } else
         attrib_handles[i] = MEM_INVALID_HANDLE;

   return result;
}

static GLboolean is_index_type(GLenum type)
{
   return type == GL_UNSIGNED_BYTE ||
          type == GL_UNSIGNED_SHORT;
}

void glDrawElements_impl (GLenum mode, GLsizei count, GLenum type, const void *indices_pointer, GLuint indices_buffer, GLXX_ATTRIB_T *attribs, int *keys, int keys_count)
{
   MEM_HANDLE_OFFSET_T interlocks[GLXX_CONFIG_MAX_VERTEX_ATTRIBS+1];
   int i, interlock_count;
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();
   
   /* SW-5891 glDrawArrays should handle >65536 vertices, fix is implemented for all but triangle fans and line loops  */  
   if (attribs_are_consistent(state, attribs) && !(type != 0 && ((state->bound_buffer.mh_element_array == MEM_INVALID_HANDLE) ^ (indices_buffer == 0)))) {
      bool indices_ok;
      int max_index;
      MEM_HANDLE_T indices_handle;
      MEM_HANDLE_T attrib_handles[GLXX_CONFIG_MAX_VERTEX_ATTRIBS];

      state->batch.primitive_mode = mode;    /* Needed for getting attr_live. Should mode be checked for validity? */

      if (!glxx_hw_get_attr_live(state, mode, attribs))
      {
         glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
         goto fail_or_done;
      }
      max_index = set_attrib_handles(state, attribs, attrib_handles);
      
      if(type==0) {
         int first = (int)indices_pointer;
         indices_ok = first >= 0 && first < max_index && (first + count -1) <= max_index;
         indices_handle = MEM_INVALID_HANDLE;
      }
      else {
         if (state->bound_buffer.mh_element_array) {
            GLXX_BUFFER_T *buffer;
            int type_size;
            uint32_t buffer_size;
   
            indices_handle = buffer_handle(state->bound_buffer.mh_element_array);
   
            buffer = (GLXX_BUFFER_T *)mem_lock(state->bound_buffer.mh_element_array);
            type_size = khrn_get_type_size(type);
            buffer_size = glxx_buffer_get_size(buffer);
   
            // Need to check two things:
            // - we don't look beyond the index buffer object (the easy bit)
            // - none of the indices go beyond any vertex buffer object, i.e. max_index
            // Note that it is assumed that anything not coming from a buffer object is safe.
            // (The client will have worked out how much data it needs and sent that
            // much across).
   
            indices_ok = (uint32_t)indices_pointer <= buffer_size && (uint32_t)indices_pointer + count * type_size <= buffer_size 
               && max_index > 0;
#ifdef REQUIRE_MAX_INDEX_CHECK
            if (indices_ok && max_index != INT_MAX)
               indices_ok = glxx_buffer_values_are_less_than(buffer, (int)indices_pointer, count, type_size, max_index);
#endif
   
            mem_unlock(state->bound_buffer.mh_element_array);
         } else {
         indices_handle = (MEM_HANDLE_T)indices_pointer;
		 indices_pointer = 0;
   
            if (max_index == INT_MAX) {
               indices_ok = true;
            } else {
               /* No index buffer object, so we have to calculate maximum ourselves */
               /* TODO: the client has probably worked out this information already so */
               /* could send it down */

#ifdef REQUIRE_MAX_INDEX_CHECK
               uint8_t *cache = (uint8_t *)mem_lock(indices_handle);
   
               indices_ok = find_max(count, khrn_get_type_size(type), cache+(int)indices_pointer) < max_index;
   
               mem_unlock(indices_handle);
#else
               //check vertex attribute buffers have space for at least some vertices
               indices_ok = max_index > 0;
#endif
            }
         }
   
      }

// adjust count to match mode so no degenerate primitives are left on the end
// see Open GL ES 1.1, 2.0 spec and HW-2858
      if (type==0 || is_index_type(type))
      {
         switch (mode) {
            case GL_POINTS:
               count = count < 1 ? 0 : count;
               break;
            case GL_LINES:
               count = count < 2 ? 0 : ( count % 2 != 0 ? (count/2)*2 : count );
               break;
            case GL_LINE_LOOP:
            case GL_LINE_STRIP:
               count = count < 2 ? 0 : count;
               break;
            case GL_TRIANGLES:
               count = count < 3 ? 0 : ( count % 3 != 0 ? (count/3)*3 : count );
               break;
            case GL_TRIANGLE_STRIP:
            case GL_TRIANGLE_FAN:
               count = count < 3 ? 0 : count;
               break;
            default:
               glxx_server_state_set_error(state, GL_INVALID_ENUM);
               break;
         }
      }
//
      
      if (!IS_GL_11(state) && !gl20_validate_current_program(state))
         glxx_server_state_set_error(state, GL_INVALID_OPERATION);
      else if (glxx_check_framebuffer_status(state, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
         glxx_server_state_set_error(state, GL_INVALID_FRAMEBUFFER_OPERATION);
      else if (count >= 0 && (!IS_GL_11(state) || attribs[GL11_IX_VERTEX].enabled) && indices_ok) {
         if (type==0 || is_index_type(type))
            switch (mode) {
            case GL_POINTS:
            case GL_LINES:
            case GL_LINE_LOOP:
            case GL_LINE_STRIP:
            case GL_TRIANGLES:
            case GL_TRIANGLE_STRIP:
            case GL_TRIANGLE_FAN:
               interlock_count = 0;
               for (i = 0; i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++)
               {
                  if (attribs[i].enabled && state->bound_buffer.mh_attrib_array[i] != MEM_INVALID_HANDLE)
                  {
                     interlocks[interlock_count].mh_handle = state->bound_buffer.mh_attrib_array[i];
                     interlocks[interlock_count].offset = buffer_interlock_offset(state->bound_buffer.mh_attrib_array[i]);
                     interlock_count++;
                  }
               }
               if (type != 0 && state->bound_buffer.mh_element_array != MEM_INVALID_HANDLE)
               {
                  interlocks[interlock_count].mh_handle = state->bound_buffer.mh_element_array;
                  interlocks[interlock_count].offset = buffer_interlock_offset(state->bound_buffer.mh_element_array);
                  interlock_count++;
               }
               vcos_assert(interlock_count <= GLXX_CONFIG_MAX_VERTEX_ATTRIBS+1);

               if (!glxx_hw_draw_triangles(count, type, (int)indices_pointer, state, attribs, indices_handle, attrib_handles, max_index, interlocks, interlock_count))
                  glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
               break;
            default:
               glxx_server_state_set_error(state, GL_INVALID_ENUM);
               break;
            }
         else
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
      }
      else
         glxx_server_state_set_error(state, GL_INVALID_VALUE);
   } else
      glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);


fail_or_done:
	state->changed = GL_FALSE;
   GLXX_UNLOCK_SERVER_STATE();
}

void glEnable_impl (GLenum cap) // S
{
   set_enabled(cap, true);
}

GLuint glFinish_impl (void)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   glxx_server_state_flush(state, true);

   GLXX_UNLOCK_SERVER_STATE();

   return 0;
}

void glFlush_impl (void)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   glxx_server_state_flush1(state, false);

   GLXX_UNLOCK_SERVER_STATE();
}


static GLboolean is_front_face(GLenum mode)
{
   return mode == GL_CW ||
          mode == GL_CCW;
}

/*
   glFrontFace()

   Sets which winding order is considered to be front facing, specifying
   one of CW (clockwise) or CCW (counterclockwise). Gives GL_INVALID_ENUM
   error if the mode is not one of these.

   Implementation: Done
   Error Checks: Done
*/

void glFrontFace_impl (GLenum mode) // S
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (is_front_face(mode))
   {
      state->changed_cfg = true;
      state->front_face = mode;
   }
   else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GLXX_UNLOCK_SERVER_STATE();
}

void glGenBuffers_impl (GLsizei n, GLuint *buffers)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   GLXX_SHARED_T *shared = (GLXX_SHARED_T *)mem_lock(state->mh_shared);

   int32_t i = 0;

   if (n < 0)
      glxx_server_state_set_error(state, GL_INVALID_VALUE);   // The conformance tests insist...
   else if (buffers)
      while (i < n) {
         if (glxx_shared_get_buffer(shared, shared->next_buffer, false) == MEM_INVALID_HANDLE)
            buffers[i++] = shared->next_buffer;

         shared->next_buffer++;
      }

   mem_unlock(state->mh_shared);

   GLXX_UNLOCK_SERVER_STATE();
}

void glGenTextures_impl (GLsizei n, GLuint *textures)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   GLXX_SHARED_T *shared = (GLXX_SHARED_T *)mem_lock(state->mh_shared);

   int32_t i = 0;

   if (n < 0)
      glxx_server_state_set_error(state, GL_INVALID_VALUE);   // The conformance tests insist...
   else if (textures)
      while (i < n) {
         if (glxx_shared_get_texture(shared, shared->next_texture) == MEM_INVALID_HANDLE)
            textures[i++] = shared->next_texture;

         shared->next_texture++;
      }

   mem_unlock(state->mh_shared);

   GLXX_UNLOCK_SERVER_STATE();
}

/*
   GetError

   Current error code(s) NO ERROR GetError
*/

GLenum glGetError_impl (void)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   GLenum result = state->error;

   state->error = GL_NO_ERROR;

   GLXX_UNLOCK_SERVER_STATE();

   return result;
}

GLboolean glxx_is_boolean (GLXX_SERVER_STATE_T *state, GLenum pname)
{
   switch (pname) {
   //common gl 1.1 and 2.0 booleans
   case GL_SAMPLE_COVERAGE_INVERT:
   case GL_COLOR_WRITEMASK:
   case GL_DEPTH_WRITEMASK:
   case GL_CULL_FACE:
   case GL_POLYGON_OFFSET_FILL:
   case GL_SAMPLE_ALPHA_TO_COVERAGE:
   case GL_SAMPLE_COVERAGE:
   case GL_SCISSOR_TEST:
   case GL_STENCIL_TEST:
   case GL_DEPTH_TEST:
   case GL_BLEND:
   case GL_DITHER:
      return GL_TRUE;
   //gl 1.1 specific booleans
   case GL_LIGHT_MODEL_TWO_SIDE:
   case GL_NORMALIZE:
   case GL_RESCALE_NORMAL:
   case GL_CLIP_PLANE0:
   case GL_FOG:
   case GL_LIGHTING:
   case GL_COLOR_MATERIAL:
   case GL_LIGHT0:
   case GL_LIGHT1:
   case GL_LIGHT2:
   case GL_LIGHT3:
   case GL_LIGHT4:
   case GL_LIGHT5:
   case GL_LIGHT6:
   case GL_LIGHT7:
   case GL_POINT_SMOOTH:
   case GL_POINT_SPRITE_OES:
   case GL_LINE_SMOOTH:
   case GL_MULTISAMPLE:
   case GL_SAMPLE_ALPHA_TO_ONE:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_EXTERNAL_OES:
   case GL_ALPHA_TEST:
   case GL_COLOR_LOGIC_OP:
      return IS_GL_11(state);
   //gl 2.0 specific booleans
   case GL_SHADER_COMPILER:
      return !IS_GL_11(state);
   default:
      return GL_FALSE;
   }
}

GLboolean glxx_is_integer (GLXX_SERVER_STATE_T *state, GLenum pname)
{
   switch (pname) {
   //common gl 1.1 and 2.0 integers
   case GL_VIEWPORT:
   case GL_CULL_FACE_MODE:
   case GL_FRONT_FACE:
   case GL_TEXTURE_BINDING_2D:
   case GL_ACTIVE_TEXTURE:
   case GL_SCISSOR_BOX:
   case GL_STENCIL_FUNC:
   case GL_STENCIL_WRITEMASK:
   case GL_STENCIL_CLEAR_VALUE:
   case GL_STENCIL_VALUE_MASK:
   case GL_STENCIL_REF:
   case GL_STENCIL_FAIL:
   case GL_STENCIL_PASS_DEPTH_FAIL:
   case GL_STENCIL_PASS_DEPTH_PASS:
   case GL_DEPTH_FUNC:
   case GL_GENERATE_MIPMAP_HINT:
   case GL_SUBPIXEL_BITS:
   case GL_MAX_TEXTURE_SIZE:
   case GL_MAX_VIEWPORT_DIMS:
   case GL_SAMPLE_BUFFERS:
   case GL_SAMPLES:
   case GL_COMPRESSED_TEXTURE_FORMATS:
   case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
   case GL_RED_BITS:
   case GL_GREEN_BITS:
   case GL_BLUE_BITS:
   case GL_ALPHA_BITS:
   case GL_DEPTH_BITS:
   case GL_STENCIL_BITS:
   case GL_FRAMEBUFFER_BINDING:
   case GL_RENDERBUFFER_BINDING:
   case GL_MAX_RENDERBUFFER_SIZE:
      return GL_TRUE;
   //gl 1.1 specific integers
   case GL_MODELVIEW_STACK_DEPTH:
   case GL_PROJECTION_STACK_DEPTH:
   case GL_TEXTURE_STACK_DEPTH:
   case GL_MATRIX_MODE:
   case GL_FOG_MODE:
   case GL_SHADE_MODEL:
   case GL_ALPHA_TEST_FUNC:
   case GL_BLEND_SRC:
   case GL_BLEND_DST:
   case GL_LOGIC_OP_MODE:
   case GL_PERSPECTIVE_CORRECTION_HINT:
   case GL_POINT_SMOOTH_HINT:
   case GL_LINE_SMOOTH_HINT:
   case GL_FOG_HINT:
   case GL_MAX_LIGHTS:
   case GL_MAX_CLIP_PLANES:
   case GL_MAX_MODELVIEW_STACK_DEPTH:
   case GL_MAX_PROJECTION_STACK_DEPTH:
   case GL_MAX_TEXTURE_STACK_DEPTH:
   case GL_MAX_TEXTURE_UNITS:
   case GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES:
   case GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES:
   case GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES:
      return IS_GL_11(state);
   //gl 2.0 specific integers
   case GL_ARRAY_BUFFER_BINDING:
   case GL_ELEMENT_ARRAY_BUFFER_BINDING:
   case GL_TEXTURE_BINDING_CUBE_MAP:
   case GL_STENCIL_BACK_WRITEMASK:
   case GL_STENCIL_BACK_FUNC:
   case GL_STENCIL_BACK_VALUE_MASK:
   case GL_STENCIL_BACK_REF:
   case GL_STENCIL_BACK_FAIL:
   case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
   case GL_STENCIL_BACK_PASS_DEPTH_PASS:
   case GL_BLEND_SRC_RGB:
   case GL_BLEND_SRC_ALPHA:
   case GL_BLEND_DST_RGB:
   case GL_BLEND_DST_ALPHA:
   case GL_BLEND_EQUATION_RGB:
   case GL_BLEND_EQUATION_ALPHA:
   case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
   case GL_SHADER_BINARY_FORMATS:
   case GL_NUM_SHADER_BINARY_FORMATS:
   case GL_MAX_VERTEX_ATTRIBS:
   case GL_MAX_VERTEX_UNIFORM_VECTORS:
   case GL_MAX_VARYING_VECTORS:
   case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
   case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
   case GL_MAX_TEXTURE_IMAGE_UNITS:
   case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
   case GL_IMPLEMENTATION_COLOR_READ_TYPE:
   case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
   case GL_CURRENT_PROGRAM:
      return !IS_GL_11(state);
   default:
      return GL_FALSE;
   }
}

static GLboolean is_small_float_glxx (GLXX_SERVER_STATE_T *state, GLenum pname)
{
   switch (pname) {
   //common gl 1.1 and 2.0 floats in range 0-1
   case GL_DEPTH_RANGE:
   case GL_COLOR_CLEAR_VALUE:
   case GL_DEPTH_CLEAR_VALUE:
      return GL_TRUE;
   //gl 1.1 specific floats in range 0-1
   case GL_FOG_COLOR:
   case GL_LIGHT_MODEL_AMBIENT:
   case GL_ALPHA_TEST_REF:
      return IS_GL_11(state);
   //gl 2.0 specific floats in range 0-1
   case GL_BLEND_COLOR:
      return !IS_GL_11(state);
   default:
      return GL_FALSE;
   }
}

GLboolean glxx_is_float (GLXX_SERVER_STATE_T *state, GLenum pname)
{
   switch (pname) {
   //common gl 1.1 and 2.0 floats
   case GL_DEPTH_RANGE:
   case GL_LINE_WIDTH:
   case GL_POLYGON_OFFSET_FACTOR:
   case GL_POLYGON_OFFSET_UNITS:
   case GL_SAMPLE_COVERAGE_VALUE:
   case GL_COLOR_CLEAR_VALUE:
   case GL_DEPTH_CLEAR_VALUE:
   case GL_ALIASED_POINT_SIZE_RANGE:
   case GL_ALIASED_LINE_WIDTH_RANGE:
      return GL_TRUE;
   //gl 1.1 specific floats
   case GL_FOG_COLOR:
   case GL_LIGHT_MODEL_AMBIENT:
   case GL_MODELVIEW_MATRIX:
   case GL_PROJECTION_MATRIX:
   case GL_TEXTURE_MATRIX:
   case GL_FOG_DENSITY:
   case GL_FOG_START:
   case GL_FOG_END:
   case GL_POINT_SIZE_MIN:
   case GL_POINT_SIZE_MAX:
   case GL_POINT_FADE_THRESHOLD_SIZE:
   case GL_POINT_DISTANCE_ATTENUATION:
   case GL_ALPHA_TEST_REF:
   case GL_SMOOTH_POINT_SIZE_RANGE:
   case GL_SMOOTH_LINE_WIDTH_RANGE:
      return IS_GL_11(state);
   //gl 2.0 specific floats
   case GL_BLEND_COLOR:
      return !IS_GL_11(state);
   default:
      return GL_FALSE;
   }
   
}

/*
   glGetBooleanv()

   Gets the value(s) of a specified state variable into an array of
   booleans. Native integer and float variables return GL_FALSE if zero
   and GL_TRUE if non-zero. Gives GL_INVALID_ENUM if the state variable
   does not exist.

   Implementation: Done
   Error Checks: Done
*/

int glGetBooleanv_impl (GLenum pname, GLboolean *params)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();
   
   int result;
   
   if (glxx_is_boolean(state, pname))
      result = glxx_get_boolean_internal(state, pname, params);
   else if (glxx_is_integer(state, pname))
   {
      GLint temp[16];
      GLuint count = glxx_get_integer_internal(state, pname, temp);
      GLuint i;

      vcos_assert(count <= 16);

      for (i = 0; i < count; i++)
         params[i] = temp[i] != 0;

      result = count;
   }
   else if (glxx_is_float(state, pname))
   {
      GLfloat temp[16];
      GLuint count = glxx_get_float_internal(state, pname, temp);
      GLuint i;

      vcos_assert(count <= 16);

      for (i = 0; i < count; i++)
         params[i] = temp[i] != 0.0f;

      result = count;
   }
   else
   {
      GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

      glxx_server_state_set_error(state, GL_INVALID_ENUM);

      GLXX_UNLOCK_SERVER_STATE();

      result = 0;
   }
   
   GLXX_UNLOCK_SERVER_STATE();
   
   return result;
}

/*
   GetBufferParameteriv

   BUFFER SIZE 0 GetBufferParameteriv
   BUFFER USAGE STATIC DRAW GetBufferParameteriv
   BUFFER ACCESS WRITE ONLY GetBufferParameteriv
   BUFFER MAPPED False GetBufferParameteriv
*/

int glGetBufferParameteriv_impl (GLenum target, GLenum pname, GLint *params)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   MEM_HANDLE_T bhandle;
   GLXX_BUFFER_T *buffer;
   int result = 0;

   vcos_assert(state);

   buffer = glxx_get_bound_buffer(state, target, &bhandle);

   if (buffer) {
      switch (pname) {
      case GL_BUFFER_SIZE:
         params[0] = mem_get_size(glxx_buffer_get_storage_handle(buffer));
         result = 1;
         break;
      case GL_BUFFER_USAGE:
         params[0] = buffer->usage;
         result = 1;
         break;
      default:
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
         break;
      }

      mem_unlock(bhandle);
   }

   GLXX_UNLOCK_SERVER_STATE();

   return result;
}

int glGetIntegerv_impl (GLenum pname, GLint *params)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();
   
   int result;
   
   if (glxx_is_boolean(state, pname))
   {
      GLboolean temp[4];
      GLuint count = glxx_get_boolean_internal(state, pname, temp);
      GLuint i;

      vcos_assert(count <= 4);

      for (i = 0; i < count; i++)
         params[i] = temp[i] ? 1 : 0;

      result = count;
   }
   else if (glxx_is_integer(state, pname))
      result = glxx_get_integer_internal(state, pname, params);
   else if (is_small_float_glxx(state, pname))
   {
      GLfloat temp[4];
      GLuint count = glxx_get_float_internal(state, pname, temp);
      GLuint i;

      vcos_assert(count <= 4);

      for (i = 0; i < count; i++) {
         params[i] = (GLint)floor((4294967295.0f * temp[i] - 1.0f) / 2.0f + 0.5f);

         if (params[i] < 0)
            params[i] = 0x7fffffff;
      }

      result = count;
   }
   else if (glxx_is_float(state, pname))
   {
      GLfloat temp[16];
      GLuint count = glxx_get_float_internal(state, pname, temp);
      GLuint i;

      vcos_assert(count <= 16);

      for (i = 0; i < count; i++)
         params[i] = float_to_int(temp[i]);

      result = count;
   }
   else
   {

      glxx_server_state_set_error(state, GL_INVALID_ENUM);

      result = 0;
   }
   
   GLXX_UNLOCK_SERVER_STATE();
   
   return result;
}

//TODO where is glGetString handled? it is not in client/gl.c
/*
const GLubyte * glGetString_impl (GLenum name)
{
   vcos_assert(0);
}
*/

/*
   GetTexParameteriv

   TEXTURE MIN FILTER NEAREST MIPMAP LINEAR GetTexParameteriv
   TEXTURE MAG FILTER LINEAR GetTexParameteriv
   TEXTURE WRAP S REPEAT GetTexParameteriv
   TEXTURE WRAP T REPEAT GetTexParameteriv
   GENERATE MIPMAP GetTexParameter //gl 1.1 specific
*/

int glxx_get_texparameter_internal(GLenum target, GLenum pname, GLint *params)
{
   int result = 0;
   MEM_HANDLE_T thandle;
   GLXX_TEXTURE_T *texture;

   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   vcos_assert(state);

   texture = glxx_server_state_get_texture(state, target, GL_FALSE, &thandle);

   if (texture) {
      switch (pname) {
      case GL_TEXTURE_MIN_FILTER:
         params[0] = texture->min;
         result = 1;
         break;
      case GL_TEXTURE_MAG_FILTER:
         params[0] = texture->mag;
         result = 1;
         break;
      case GL_TEXTURE_WRAP_S:
         params[0] = texture->wrap.s;
         result = 1;
         break;
      case GL_TEXTURE_WRAP_T:
         params[0] = texture->wrap.t;
         result = 1;
         break;
      case GL_GENERATE_MIPMAP:
         if(IS_GL_11(state)) {
            params[0] = texture->generate_mipmap;
            result = 1;
         } else {
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
            result = 0;
         }
         break;
      default:
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
         result = 0;
         break;
      }

      mem_unlock(thandle);
   }

   GLXX_UNLOCK_SERVER_STATE();

   return result;
}

int glGetTexParameterfv_impl (GLenum target, GLenum pname, GLfloat *params)
{
   GLint temp[4];
   GLuint count = glxx_get_texparameter_internal(target, pname, temp);

   if (count) {
      unsigned int i;
      vcos_assert(count == 1 || count == 4);
      for(i=0;i<count;i++)
         params[i] = (GLfloat)temp[i];
   }

   return count;
}

int glGetTexParameteriv_impl (GLenum target, GLenum pname, GLint *params)
{
   return glxx_get_texparameter_internal(target, pname, params);
}

static GLboolean is_hint(GLenum mode)
{
   return mode == GL_FASTEST ||
          mode == GL_NICEST ||
          mode == GL_DONT_CARE;
}

void glHint_impl (GLenum target, GLenum mode)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   vcos_assert(state);

   if (is_hint(mode))
      switch (target) {
      case GL_PERSPECTIVE_CORRECTION_HINT:
         if(IS_GL_11(state)) {
            state->hints.perspective_correction = mode;
         } else {
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
         }
         break;
      case GL_POINT_SMOOTH_HINT:
         if(IS_GL_11(state)) {
            state->hints.point_smooth = mode;
         } else {
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
         }
         break;
      case GL_LINE_SMOOTH_HINT:
         if(IS_GL_11(state)) {
            state->hints.line_smooth = mode;
         } else {
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
         }
         break;
      case GL_FOG_HINT:
         if(IS_GL_11(state)) {
            state->hints_program.fog = mode;
         } else {
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
         }
         break;
      case GL_GENERATE_MIPMAP_HINT:
         state->hints.generate_mipmap = mode;
         break;
      default:
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
         break;
      }
   else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GLXX_UNLOCK_SERVER_STATE();
}




GLboolean glIsBuffer_impl (GLuint buffer)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   GLboolean result = glxx_shared_get_buffer((GLXX_SHARED_T *)mem_lock(state->mh_shared), buffer, false) != MEM_INVALID_HANDLE;
   mem_unlock(state->mh_shared);

   GLXX_UNLOCK_SERVER_STATE();

   return result;
}


GLboolean glIsEnabled_impl (GLenum cap)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   GLboolean result;

   switch (cap) {
   case GL_CULL_FACE:
   case GL_POLYGON_OFFSET_FILL:
   case GL_SAMPLE_ALPHA_TO_COVERAGE:
   case GL_SAMPLE_COVERAGE:   
   case GL_SCISSOR_TEST:   
   case GL_STENCIL_TEST:
   case GL_DEPTH_TEST:
   case GL_BLEND:
   case GL_DITHER:   
   {
      GLuint count = glxx_get_boolean_internal(state, cap, &result);

      UNUSED_NDEBUG(count);
      vcos_assert(count == 1);
      break;
   }
/*
   case GL_VERTEX_ARRAY:
   case GL_NORMAL_ARRAY:
   case GL_COLOR_ARRAY:
   case GL_TEXTURE_COORD_ARRAY:
   case GL_POINT_SIZE_ARRAY_OES:
*/
   case GL_NORMALIZE:
   case GL_RESCALE_NORMAL:
   case GL_CLIP_PLANE0:
   case GL_FOG:
   case GL_LIGHTING:
   case GL_COLOR_MATERIAL:
   case GL_LIGHT0:
   case GL_LIGHT1:
   case GL_LIGHT2:
   case GL_LIGHT3:
   case GL_LIGHT4:
   case GL_LIGHT5:
   case GL_LIGHT6:
   case GL_LIGHT7:
   case GL_POINT_SMOOTH:
   case GL_POINT_SPRITE_OES:
   case GL_LINE_SMOOTH:
   case GL_MULTISAMPLE:
   case GL_SAMPLE_ALPHA_TO_ONE:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_EXTERNAL_OES:
   case GL_ALPHA_TEST:
   case GL_COLOR_LOGIC_OP:
      if(IS_GL_11(state)) {
         GLuint count = glxx_get_boolean_internal(state, cap, &result);
         UNUSED_NDEBUG(count);
         vcos_assert(count == 1);
      } else {
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
         result = GL_FALSE;
      }
      break;
   default:
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
      result = GL_FALSE;
      break;
   }

   GLXX_UNLOCK_SERVER_STATE();

   return result;
}

/*
   glIsTexture()

   Returns TRUE if texture is the name of a texture object. Returns False if texture is zero, or
   is a nonzero value that is not the name of a texture object, or if an error condition occurs.
   A name returned by GenTextures, but not yet bound, is not the name of a texture object.

   Implementation: Done
   Error Checks: Done
*/

GLboolean glIsTexture_impl (GLuint texture)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   GLboolean result = glxx_shared_get_texture((GLXX_SHARED_T *)mem_lock(state->mh_shared), texture) != MEM_INVALID_HANDLE;
   mem_unlock(state->mh_shared);

   GLXX_UNLOCK_SERVER_STATE();

   return result;
}

void glReadPixels_impl (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint alignment, void *pixels)
{
   MEM_HANDLE_T hsrc;
   KHRN_IMAGE_T *src;
   int fbwidth, fbheight;
   uint32_t a, n, s, k;
   char *cpixels;
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   vcos_assert(state);

   hsrc = get_read_image(state);
   src = (KHRN_IMAGE_T *)mem_lock(hsrc);
   fbwidth = src->width;
   fbheight = src->height;
   vcos_assert(fbwidth != 0 && fbheight != 0);   //TODO I'm not sure I like this assertion

   // Copied from texture.c, get_pixel
   a = alignment;
   n = format == GL_RGBA ? 4 : 3;
   s = 1;

   if (s < a)
      k = (a / s) * ((s * n * width + a - 1) / a);
   else
      k = n * width;

   khrn_interlock_read_immediate(&src->interlock);

   //TODO: sort out this mess!
   cpixels = (char*)pixels;
   if (glxx_check_framebuffer_status(state, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      glxx_server_state_set_error(state, GL_INVALID_FRAMEBUFFER_OPERATION);
   else if ((format == GL_RGBA || format == GL_RGB) && type == GL_UNSIGNED_BYTE)
   {
      KHRN_IMAGE_WRAP_T src_wrap, dst_wrap;
      uint32_t dstx = 0, dsty = 0;

      if (x < 0) { dstx -= x; width += x;  x = 0; }
      if (y < 0) { dsty -= y; height += y; y = 0; }
      if (width > fbwidth   || x + width > fbwidth)   { width = fbwidth - x; }
      if (height > fbheight || y + height > fbheight) { height = fbheight - y; }

      if (width > 0 && height > 0)
      {
         khrn_image_lock_wrap(src, &src_wrap);
         khrn_image_wrap(&dst_wrap, (format == GL_RGBA) ? ABGR_8888_RSO : BGR_888_RSO, width, height, k, pixels);

#if defined(BCG_FB_LAYOUT) || defined(BCG_FB_LAYOUT_B1)
         if (khrn_image_is_rso(src_wrap.format))
         {
            src_wrap.storage = (void *)((uintptr_t)src_wrap.storage + (src_wrap.stride * (src_wrap.height-1)));
            src_wrap.stride = -src_wrap.stride;
         }
#endif /* BCG_FB_LAYOUT || BCG_FB_LAYOUT_B1 */

         khrn_image_wrap_copy_region(
            &dst_wrap, dstx, dsty, width, height,
            &src_wrap, x, y, IMAGE_CONV_GL);

         khrn_image_unlock_wrap(src);
      }
   }
   else
   {
      glxx_server_state_set_error(state, GL_INVALID_VALUE);   //TODO is this the correct error?
   }
   mem_unlock(hsrc);
   GLXX_UNLOCK_SERVER_STATE();
}

void glxx_sample_coverage_internal (GLclampf value, GLboolean invert) // S
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   state->changed_backend = true;
   state->sample_coverage.value = clampf(value, 0.0f, 1.0f);
   state->sample_coverage.invert = clean_boolean(invert);

   GLXX_UNLOCK_SERVER_STATE();
}

void glSampleCoverage_impl (GLclampf value, GLboolean invert)
{
   glxx_sample_coverage_internal(value, invert);
}

void glScissor_impl (GLint x, GLint y, GLsizei width, GLsizei height)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (width >= 0 && height >= 0)
   {
      state->changed_viewport = true;
      state->scissor.x = x;
      state->scissor.y = y;
      state->scissor.width = width;
      state->scissor.height = height;
   } else
      glxx_server_state_set_error(state, GL_INVALID_VALUE);

   GLXX_UNLOCK_SERVER_STATE();
}

static GLboolean is_face(GLenum face)
{
   return face == GL_FRONT ||
          face == GL_BACK ||
          face == GL_FRONT_AND_BACK;
}

void glStencilFuncSeparate_impl (GLenum face, GLenum func, GLint ref, GLuint mask) // S
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (is_face(face) && is_func(func)) {
      if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
         state->stencil_func.front.func = func;
         state->stencil_func.front.ref = ref;
         state->stencil_func.front.mask = mask;
      }

      if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
         state->stencil_func.back.func = func;
         state->stencil_func.back.ref = ref;
         state->stencil_func.back.mask = mask;
      }
   } else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GLXX_UNLOCK_SERVER_STATE();
}

static GLboolean is_op(GLXX_SERVER_STATE_T *state, GLenum op)
{
   return op == GL_KEEP ||
          op == GL_ZERO ||
          op == GL_REPLACE ||
          op == GL_INCR ||
          op == GL_DECR ||
          op == GL_INVERT ||
          (!IS_GL_11(state) && (
          op == GL_INCR_WRAP ||
          op == GL_DECR_WRAP
          ));
}

void glStencilMaskSeparate_impl (GLenum face, GLuint mask) // S
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (is_face(face)) {
      if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
         state->stencil_mask.front = mask;

      if (face == GL_BACK || face == GL_FRONT_AND_BACK)
         state->stencil_mask.back = mask;
   } else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GLXX_UNLOCK_SERVER_STATE();
}

void glStencilOpSeparate_impl (GLenum face, GLenum fail, GLenum zfail, GLenum zpass) // S
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (is_face(face) && is_op(state,fail) && is_op(state,zfail) && is_op(state,zpass)) {
      if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
         state->stencil_op.front.fail = fail;
         state->stencil_op.front.zfail = zfail;
         state->stencil_op.front.zpass = zpass;
      }

      if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
         state->stencil_op.back.fail = fail;
         state->stencil_op.back.zfail = zfail;
         state->stencil_op.back.zpass = zpass;
      }
   } else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GLXX_UNLOCK_SERVER_STATE();
}

static GLboolean texture_format_type_match(GLenum format, GLenum type)
{
   return (format == GL_RGBA && type == GL_UNSIGNED_BYTE) ||
          (format == GL_RGB && type == GL_UNSIGNED_BYTE) ||
          (format == GL_RGBA && type == GL_UNSIGNED_SHORT_4_4_4_4) ||
          (format == GL_RGBA && type == GL_UNSIGNED_SHORT_5_5_5_1) ||
          (format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5) ||
          (format == GL_LUMINANCE_ALPHA && type == GL_UNSIGNED_BYTE) ||
          (format == GL_LUMINANCE && type == GL_UNSIGNED_BYTE) ||
          (format == GL_ALPHA && type == GL_UNSIGNED_BYTE) ||
#if GL_EXT_texture_format_BGRA8888
          (format == GL_BGRA_EXT && type == GL_UNSIGNED_BYTE) ||
#endif
          0 ;
}

GLboolean glTexImage2D_impl (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLint alignment, const GLvoid *pixels)
{
   GLboolean res = GL_FALSE;
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   MEM_HANDLE_T thandle;

   GLXX_TEXTURE_T *texture = glxx_server_state_get_texture(state, target, GL_TRUE, &thandle);

   state->changed_texunit = true; /* GLES1.1 - may change has_color/has_alpha */
   if (texture) {
      if (level < 0 || level > LOG2_MAX_TEXTURE_SIZE)
         glxx_server_state_set_error(state, GL_INVALID_VALUE);
      else if (width < 0 || width > MAX_TEXTURE_SIZE || height < 0 || height > MAX_TEXTURE_SIZE)
         glxx_server_state_set_error(state, GL_INVALID_VALUE);
      else if (IS_GL_11(state) && target != GL_TEXTURE_2D && target != GL_TEXTURE_EXTERNAL_OES)
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
      else if (!IS_GL_11(state) && target != GL_TEXTURE_2D && target != GL_TEXTURE_EXTERNAL_OES && width != height)
         glxx_server_state_set_error(state, GL_INVALID_VALUE);
      else if (border != 0 || !is_texture_internal_format(internalformat))
         glxx_server_state_set_error(state, GL_INVALID_VALUE);
      else if (internalformat != (GLint)format || !texture_format_type_match(format, type))
         glxx_server_state_set_error(state, GL_INVALID_OPERATION);
      else if (!glxx_texture_image(texture, target, level, width, height, format, type, alignment, pixels))
         glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
      else
         res = GL_TRUE;

      mem_unlock(thandle);
   }

   GLXX_UNLOCK_SERVER_STATE();
   return res;
}

static GLboolean is_mag_filter(int i)
{
   return i == GL_NEAREST ||
          i == GL_LINEAR;
}

static GLboolean is_min_filter(int i)
{
   return i == GL_NEAREST ||
          i == GL_LINEAR ||
          i == GL_NEAREST_MIPMAP_NEAREST ||
          i == GL_NEAREST_MIPMAP_LINEAR ||
          i == GL_LINEAR_MIPMAP_NEAREST ||
          i == GL_LINEAR_MIPMAP_LINEAR;
}

static GLboolean is_wrap(GLXX_SERVER_STATE_T *state, int i)
{
   return i == GL_REPEAT ||
          i == GL_CLAMP_TO_EDGE
          || (!IS_GL_11(state) && i == GL_MIRRORED_REPEAT);
}

void glxx_texparameter_internal(GLenum target, GLenum pname, const GLint *i)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   MEM_HANDLE_T thandle;

   GLXX_TEXTURE_T *texture = glxx_server_state_get_texture(state, target, GL_FALSE, &thandle);      // performs valid target check

   if (texture) {
      switch (pname) {
      case GL_TEXTURE_MIN_FILTER:
         if (is_min_filter(*i))
            texture->min = *i;
         else
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
         break;
      case GL_TEXTURE_MAG_FILTER:
         if (is_mag_filter(*i))
            texture->mag = *i;
         else
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
         break;
      case GL_TEXTURE_WRAP_S:
         if (is_wrap(state, *i))
            texture->wrap.s = *i;
         else
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
         break;
      case GL_TEXTURE_WRAP_T:
         if (is_wrap(state, *i))
            texture->wrap.t = *i;
         else
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
         break;
      case GL_GENERATE_MIPMAP:
         if(IS_GL_11(state)) {
            if (*i)
               texture->generate_mipmap = GL_TRUE;
            else
               texture->generate_mipmap = GL_FALSE;
         } else {
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
         }
         break;
      case GL_TEXTURE_CROP_RECT_OES:
         glxx_texture_set_crop_rect(texture,i);
         break;
      default:
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
         break;
      }

      mem_unlock(thandle);
   }

   GLXX_UNLOCK_SERVER_STATE();
}

void glTexParameterf_impl (GLenum target, GLenum pname, GLfloat param)
{
   GLint iparam = (GLint)param;
   glxx_texparameter_internal(target, pname, &iparam);
}

void glTexParameteri_impl (GLenum target, GLenum pname, GLint param)
{
   GLint iparam = (GLint)param;
   glxx_texparameter_internal(target, pname, &iparam);
}

void glTexParameterfv_impl (GLenum target, GLenum pname, const GLfloat *params)
{
   if (params)
   {
      if(pname == GL_TEXTURE_CROP_RECT_OES) {
         GLint iparams[4];
         int i;
         for(i=0;i<4;i++)
            iparams[i] = (GLint)params[i];
         glxx_texparameter_internal(target, pname, iparams);   
      }
      else {
         GLint iparam = (GLint)params[0];
         glxx_texparameter_internal(target, pname, &iparam);
      }
   }
}

void glTexParameteriv_impl (GLenum target, GLenum pname, const GLint *params)
{
   if (params)
      glxx_texparameter_internal(target, pname, params);
}

void glTexSubImage2D_impl (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint alignment, const GLvoid *pixels)
{
   MEM_HANDLE_T thandle;
   GLXX_TEXTURE_T *texture;
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   vcos_assert(state);

   texture = glxx_server_state_get_texture(state, target, GL_TRUE, &thandle);

   if (texture) {
      if (!glxx_is_texture_target(state, target))
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
      else if (width < 0 || width > MAX_TEXTURE_SIZE || height < 0 || height > MAX_TEXTURE_SIZE)
         glxx_server_state_set_error(state, GL_INVALID_VALUE);
      else if (!glxx_texture_includes(texture, target, level, xoffset, yoffset))
         glxx_server_state_set_error(state, GL_INVALID_VALUE);
      else if (!glxx_texture_includes(texture, target, level, xoffset + width - 1, yoffset + height - 1))
         glxx_server_state_set_error(state, GL_INVALID_VALUE);
      else if (!khrn_image_is_uncomp(glxx_texture_incomplete_get_mipmap_format(texture, target, level)))
         glxx_server_state_set_error(state, GL_INVALID_OPERATION);
      else if (width > 0 && height > 0)
      {
         //TODO: check formats are compatible?

         if (!glxx_texture_sub_image(texture, target, level, xoffset, yoffset, width, height, format, type, alignment, pixels))
            glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
      }
      mem_unlock(thandle);
   }
   GLXX_UNLOCK_SERVER_STATE();
}

#ifndef GLXX_NO_VERTEX_CACHE
void glintCacheCreate_impl(GLsizei offset)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   uint8_t * p = ((uint8_t *)mem_lock(state->mh_cache) + offset);

#ifdef __VIDEOCORE4__
   khrn_interlock_init((KHRN_INTERLOCK_T *)p);
#else
   KHRN_CACHE_COUNTERS_T *counters = (KHRN_CACHE_COUNTERS_T *)p;

   counters->acquire = 0;
   counters->release = 0;
#endif

   mem_unlock(state->mh_cache);

   GLXX_UNLOCK_SERVER_STATE();
}

void glintCacheDelete_impl(GLsizei offset)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   uint8_t * p = ((uint8_t *)mem_lock(state->mh_cache) + offset);

#ifdef __VIDEOCORE4__
   KHRN_INTERLOCK_T *interlock = (KHRN_INTERLOCK_T *)p;
   /* wait for the hardware to finish using this cache entry */
   khrn_interlock_write_immediate(interlock);
   khrn_interlock_term(interlock);
#else
   KHRN_CACHE_COUNTERS_T *counters = (KHRN_CACHE_COUNTERS_T *)((uint8_t *)mem_lock(state->mh_cache) + offset);

   while (counters->acquire != counters->release)
      khrn_sync_master_wait();
#endif
   mem_unlock(state->mh_cache);

   GLXX_UNLOCK_SERVER_STATE();
}

void glintCacheData_impl(GLsizei offset, GLsizei length, const GLvoid *data)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   uint8_t *cache = (uint8_t *)mem_lock(state->mh_cache);

   khrn_memcpy(cache + offset, data, length);

   mem_unlock(state->mh_cache);

   GLXX_UNLOCK_SERVER_STATE();
}

GLboolean glintCacheGrow_impl()
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();
   int size, result;

   khrn_hw_common_wait();

   size = mem_get_size(state->mh_cache);
   result = mem_resize_ex(state->mh_cache, size ? size * 2 : 64, MEM_COMPACT_DISCARD);

   GLXX_UNLOCK_SERVER_STATE();

   return result;
}

void glintCacheUse_impl(GLsizei count, GLsizei *offset)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   glxx_hw_cache_use(state, count, offset);

   GLXX_UNLOCK_SERVER_STATE();
}
#endif

int glintFindMax_impl(GLsizei count, GLenum type, const void *indices_pointer)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   int result = -1;

   if (state->bound_buffer.mh_element_array != MEM_INVALID_HANDLE) {
      MEM_HANDLE_T indices_handle = buffer_handle(state->bound_buffer.mh_element_array);
      uint32_t indices_size;
      uint32_t indices_offset = (uint32_t)indices_pointer;

      indices_pointer = (char *)mem_lock(indices_handle) + indices_offset;

      if (indices_offset < mem_get_size(indices_handle))
         indices_size = mem_get_size(indices_handle) - (uint32_t)indices_pointer;
      else
         indices_size = 0;

      switch (type) {
      case GL_UNSIGNED_BYTE:
         count = _min(count, indices_size);
         result = find_max(count, 1, indices_pointer);
         break;
      case GL_UNSIGNED_SHORT:
         count = _min(count, indices_size / 2);
         result = find_max(count, 2, indices_pointer);
         break;
      default:
         UNREACHABLE();
         break;
      }

      mem_unlock(indices_handle);
   }

   GLXX_UNLOCK_SERVER_STATE();

   return result;
}

void glDiscardFramebufferEXT_impl(GLenum target, GLsizei numAttachments, const GLenum *attachments)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (target != GL_FRAMEBUFFER)
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
   else if (numAttachments < 0 || attachments == NULL)
      glxx_server_state_set_error(state, GL_INVALID_VALUE);
   else
   {
      GLsizei i;
      bool    color = false;
      bool    ds = false;
      bool    err = false;

      if (valid_frame_buffer(state)) 
      {
         for (i = 0; i < numAttachments; i++)
         {
            switch (attachments[i])
            {
            case GL_COLOR_ATTACHMENT0:
               color = true;
               break;
            case GL_DEPTH_ATTACHMENT:
               ds = true;
               break;
            case GL_STENCIL_ATTACHMENT:
               ds = true;
               break;
            default:
               glxx_server_state_set_error(state, GL_INVALID_ENUM);
               err = true;
               break;
            }
         }
      }
      else
      {
         for (i = 0; i < numAttachments; i++)
         {
            switch (attachments[i])
            {
            case GL_COLOR_EXT:
               color = true;
               break;
            case GL_DEPTH_EXT:
               ds = true;
               break;
            case GL_STENCIL_EXT:
               ds = true;
               break;
            default:
               glxx_server_state_set_error(state, GL_INVALID_ENUM);
               err = true;
               break;
            }
         }
      }
      
      if (!err)
         glxx_hw_invalidate_frame(state, color, ds, ds, color);
   }

   GLXX_UNLOCK_SERVER_STATE();
}

/* OES_framebuffer_object for ES 1.1 and core in ES 2.0 */

/* check 1.1 constants match their 2.0 equivalents */
vcos_static_assert(GL_FRAMEBUFFER == GL_FRAMEBUFFER_OES);
vcos_static_assert(GL_INVALID_FRAMEBUFFER_OPERATION == GL_INVALID_FRAMEBUFFER_OPERATION_OES);
vcos_static_assert(GL_FRAMEBUFFER_BINDING == GL_FRAMEBUFFER_BINDING_OES);
vcos_static_assert(GL_RENDERBUFFER_BINDING == GL_RENDERBUFFER_BINDING_OES);
vcos_static_assert(GL_MAX_RENDERBUFFER_SIZE == GL_MAX_RENDERBUFFER_SIZE_OES);
vcos_static_assert(GL_COLOR_ATTACHMENT0 == GL_COLOR_ATTACHMENT0_OES);
vcos_static_assert(GL_DEPTH_ATTACHMENT == GL_DEPTH_ATTACHMENT_OES);
vcos_static_assert(GL_STENCIL_ATTACHMENT == GL_STENCIL_ATTACHMENT_OES);
vcos_static_assert(GL_RENDERBUFFER == GL_RENDERBUFFER_OES);
vcos_static_assert(GL_DEPTH_COMPONENT16 == GL_DEPTH_COMPONENT16_OES);
vcos_static_assert(GL_RGBA4 == GL_RGBA4_OES);
vcos_static_assert(GL_RGB5_A1 == GL_RGB5_A1_OES);
vcos_static_assert(GL_RGB565 == GL_RGB565_OES);
vcos_static_assert(GL_STENCIL_INDEX8 == GL_STENCIL_INDEX8_OES);
vcos_static_assert(GL_RENDERBUFFER_WIDTH == GL_RENDERBUFFER_WIDTH_OES);
vcos_static_assert(GL_RENDERBUFFER_HEIGHT == GL_RENDERBUFFER_HEIGHT_OES);
vcos_static_assert(GL_RENDERBUFFER_INTERNAL_FORMAT == GL_RENDERBUFFER_INTERNAL_FORMAT_OES);
vcos_static_assert(GL_RENDERBUFFER_RED_SIZE == GL_RENDERBUFFER_RED_SIZE_OES);
vcos_static_assert(GL_RENDERBUFFER_GREEN_SIZE == GL_RENDERBUFFER_GREEN_SIZE_OES);
vcos_static_assert(GL_RENDERBUFFER_BLUE_SIZE == GL_RENDERBUFFER_BLUE_SIZE_OES);
vcos_static_assert(GL_RENDERBUFFER_ALPHA_SIZE == GL_RENDERBUFFER_ALPHA_SIZE_OES);
vcos_static_assert(GL_RENDERBUFFER_DEPTH_SIZE == GL_RENDERBUFFER_DEPTH_SIZE_OES);
vcos_static_assert(GL_RENDERBUFFER_STENCIL_SIZE == GL_RENDERBUFFER_STENCIL_SIZE_OES);
vcos_static_assert(GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES);
vcos_static_assert(GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES);
vcos_static_assert(GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL == GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_OES);
vcos_static_assert(GL_NONE == GL_NONE_OES);
vcos_static_assert(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES);
vcos_static_assert(GL_FRAMEBUFFER_UNSUPPORTED == GL_FRAMEBUFFER_UNSUPPORTED_OES);
vcos_static_assert(GL_FRAMEBUFFER_UNSUPPORTED == GL_FRAMEBUFFER_UNSUPPORTED_OES);
vcos_static_assert(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES);
vcos_static_assert(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES);


static bool is_valid_attachment(GLenum attachment)
{
   return attachment == GL_COLOR_ATTACHMENT0 ||
      attachment == GL_DEPTH_ATTACHMENT ||
      attachment == GL_STENCIL_ATTACHMENT;
}

/*
   glIsRenderbuffer()

   [Not in spec, behaviour defined to be analogous to texture]

   Returns TRUE if renderbuffer is the name of a renderbuffer object. Returns FALSE if renderbuffer
   is zero, or is a nonzero value that is not the name of a renderbuffer object, or if an error
   condition occurs. A name returned by GenRenderbuffers, but not yet bound, is not the name of
   a renderbuffer object.

   Implementation: Done
   Error Checks: Done
*/

GLboolean glIsRenderbuffer_impl (GLuint renderbuffer)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   GLboolean result = glxx_shared_get_renderbuffer((GLXX_SHARED_T *)mem_lock(state->mh_shared), renderbuffer, false) != MEM_INVALID_HANDLE;
   mem_unlock(state->mh_shared);

   GLXX_UNLOCK_SERVER_STATE();

   return result;
}

/*
   glBindRenderbuffer()

   Called with target set to RENDERBUFFER and renderbuffer set to the unused name. If renderbuffer is not zero,
   then the resulting renderbuffer object is a new state vector, initialized with a zero-sized memory buffer, and
   comprising the state values listed in table 6.32. Any previous binding to target is broken.

   BindRenderbuffer may also be used to bind an existing renderbuffer object. If the bind is successful, no
   change is made to the state of the newly bound renderbuffer object, and any previous binding to target is
   broken.

   Implementation: Done
   Error Checks: Done
*/


void glBindRenderbuffer_impl (GLenum target, GLuint renderbuffer)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (target == GL_RENDERBUFFER) {
      MEM_HANDLE_T handle = MEM_INVALID_HANDLE;

      if (renderbuffer) {
         handle = glxx_shared_get_renderbuffer((GLXX_SHARED_T *)mem_lock(state->mh_shared), renderbuffer, true);
         mem_unlock(state->mh_shared);

         if (handle == MEM_INVALID_HANDLE) {
            glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);

            GLXX_UNLOCK_SERVER_STATE();
            return;
         }
      }

      MEM_ASSIGN(state->mh_bound_renderbuffer, handle);
   } else
      glxx_server_state_set_error(state, GL_INVALID_OPERATION);

   GLXX_UNLOCK_SERVER_STATE();
}

static void detach_renderbuffer_callback(KHRN_MAP_T *map, uint32_t key, MEM_HANDLE_T value, void *data)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();
   GLXX_FRAMEBUFFER_T *framebuffer = (GLXX_FRAMEBUFFER_T *)mem_lock(value);
   UNUSED(state);
   UNUSED(map);
   UNUSED(key);

   if (framebuffer->attachments.color.mh_object == (MEM_HANDLE_T)(size_t)data)
   {
      framebuffer->attachments.color.type = GL_NONE;
      framebuffer->attachments.color.target = 0;
      framebuffer->attachments.color.level = 0;
      MEM_ASSIGN(framebuffer->attachments.color.mh_object, MEM_INVALID_HANDLE);
   }
   if (framebuffer->attachments.depth.mh_object == (MEM_HANDLE_T)(size_t)data)
   {
      framebuffer->attachments.depth.type = GL_NONE;
      framebuffer->attachments.depth.target = 0;
      framebuffer->attachments.depth.level = 0;
      MEM_ASSIGN(framebuffer->attachments.depth.mh_object, MEM_INVALID_HANDLE);
   }
   if (framebuffer->attachments.stencil.mh_object == (MEM_HANDLE_T)(size_t)data)
   {
      framebuffer->attachments.stencil.type = GL_NONE;
      framebuffer->attachments.stencil.target = 0;
      framebuffer->attachments.stencil.level = 0;
      MEM_ASSIGN(framebuffer->attachments.stencil.mh_object, MEM_INVALID_HANDLE);
   }

   mem_unlock(value);

   GLXX_UNLOCK_SERVER_STATE();
}

void glDeleteRenderbuffers_impl (GLsizei n, const GLuint *renderbuffers)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   GLXX_SHARED_T *shared = (GLXX_SHARED_T *)mem_lock(state->mh_shared);

   if (n < 0)
      glxx_server_state_set_error(state, GL_INVALID_VALUE);   // The conformance tests insist...
   else if (renderbuffers)
   {
      int32_t i;
      for (i = 0; i < n; i++)
         if (renderbuffers[i]) {
            MEM_HANDLE_T handle = glxx_shared_get_renderbuffer(shared, renderbuffers[i], false);

            if (handle != MEM_INVALID_HANDLE) {
               if (state->mh_bound_renderbuffer == handle)
                  MEM_ASSIGN(state->mh_bound_renderbuffer, MEM_INVALID_HANDLE);

               khrn_map_iterate(&shared->framebuffers, detach_renderbuffer_callback, (void *)handle);

               glxx_shared_delete_renderbuffer(shared, renderbuffers[i]);
            }
         }
   }

   mem_unlock(state->mh_shared);

   GLXX_UNLOCK_SERVER_STATE();
}

void glGenRenderbuffers_impl (GLsizei n, GLuint *renderbuffers)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   GLXX_SHARED_T *shared = (GLXX_SHARED_T *)mem_lock(state->mh_shared);

   int32_t i = 0;

   if (n < 0)
      glxx_server_state_set_error(state, GL_INVALID_VALUE);   // The conformance tests insist...
   else if (renderbuffers)
      while (i < n) {
         if (glxx_shared_get_renderbuffer(shared, shared->next_renderbuffer, false) == MEM_INVALID_HANDLE)
            renderbuffers[i++] = shared->next_renderbuffer;

         shared->next_renderbuffer++;
      }

   mem_unlock(state->mh_shared);

   GLXX_UNLOCK_SERVER_STATE();
}

static GLboolean is_internal_format(GLXX_SERVER_STATE_T *state,GLenum format)
{
   return format == GL_RGBA4 ||
          format == GL_RGB5_A1 ||
          format == GL_RGB565 ||
          format == GL_RGB8_OES ||
          format == GL_RGBA8_OES ||
          format == GL_DEPTH_COMPONENT16 ||
          format == GL_DEPTH_COMPONENT24_OES ||
          format == GL_STENCIL_INDEX8 ||
          format == GL_DEPTH_COMPONENT32_OES
/* TODO confirm whether these values are permissable (defined in 2.0 headers but not in core spec as allowed formats)
             format == GL_DEPTH_COMPONENT ||
             format == GL_RGB ||
             format == GL_RGBA ||
             format == GL_STENCIL_INDEX
*/            
          ;
}

void glRenderbufferStorage_impl (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (target != GL_RENDERBUFFER)
      glxx_server_state_set_error(state, GL_INVALID_OPERATION);
   else if (state->mh_bound_renderbuffer == MEM_INVALID_HANDLE)
      glxx_server_state_set_error(state, GL_INVALID_OPERATION);
   else if (!is_internal_format(state,internalformat))
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
   else if (width < 0 || width > GLXX_CONFIG_MAX_RENDERBUFFER_SIZE || height < 0 || height > GLXX_CONFIG_MAX_RENDERBUFFER_SIZE)
      glxx_server_state_set_error(state, GL_INVALID_VALUE);
   else {
      if (!glxx_renderbuffer_storage((GLXX_RENDERBUFFER_T *)mem_lock(state->mh_bound_renderbuffer), internalformat, width, height))
         glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);

      mem_unlock(state->mh_bound_renderbuffer);
   }

   GLXX_UNLOCK_SERVER_STATE();
}

/*
   GetRenderBufferParameteriv

   RENDERBUFFER WIDTH 0 GetRenderbufferParameteriv
   RENDERBUFFER HEIGHT 0 GetRenderbufferParameteriv
   RENDERBUFFER INTERNAL FORMAT RGBA GetRenderbufferParameteriv
   RENDERBUFFER RED SIZE 0 GetRenderbufferParameteriv
   RENDERBUFFER GREEN SIZE 0 GetRenderbufferParameteriv
   RENDERBUFFER BLUE SIZE 0 GetRenderbufferParameteriv
   RENDERBUFFER ALPHA SIZE 0 GetRenderbufferParameteriv
   RENDERBUFFER DEPTH SIZE 0 GetRenderbufferParameteriv
   RENDERBUFFER STENCIL SIZE 0 GetRenderbufferParameteriv

   here we assume that the allowable internal renderbuffer
   formats are

      GL_DEPTH_COMPONENT
      GL_RGB
      GL_RGBA
      GL_RGBA4
      GL_RGB5_A1
      GL_RGB565
      GL_DEPTH_COMPONENT16
      GL_STENCIL_INDEX
      GL_STENCIL_INDEX8
*/

int glGetRenderbufferParameteriv_impl (GLenum target, GLenum pname, GLint* params)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   int result;

   if (target == GL_RENDERBUFFER) {
      if (state->mh_bound_renderbuffer != MEM_INVALID_HANDLE) {
         GLXX_RENDERBUFFER_T *renderbuffer = (GLXX_RENDERBUFFER_T *)mem_lock(state->mh_bound_renderbuffer);
         KHRN_IMAGE_T *storage = (KHRN_IMAGE_T *)mem_maybe_lock(renderbuffer->mh_storage);

         switch (pname) {
         case GL_RENDERBUFFER_WIDTH:
            params[0] = storage ? storage->width : 0;
            result = 1;
            break;
         case GL_RENDERBUFFER_HEIGHT:
            params[0] = storage ? storage->height : 0;
            result = 1;
            break;
         case GL_RENDERBUFFER_INTERNAL_FORMAT:
            if (!storage)
               params[0] = 0;
            else switch (storage->format) {
            case ABGR_8888_TF:
               params[0] = GL_RGBA8_OES;/* TODO confirm this is the correct constant to return  */
               break;
            case XBGR_8888_TF:
               params[0] = GL_RGB8_OES;/* TODO confirm this is the correct constant to return  */
               break;
            case RGBA_4444_TF:
               params[0] = GL_RGBA4;
               break;
            case RGBA_5551_TF:
               params[0] = GL_RGB5_A1;
               break;
            case RGB_565_TF:
               params[0] = GL_RGB565;
               break;
            case DEPTH_32_TF:
               params[0] = GL_DEPTH_COMPONENT24_OES;/* TODO confirm this is the correct constant to return  */
               break;
            case DEPTH_16_TF:
               params[0] = GL_DEPTH_COMPONENT16;
               break;
            default:
               UNREACHABLE();
               break;
            }
            result = 1;
            break;
         case GL_RENDERBUFFER_RED_SIZE:
            params[0] = storage ? khrn_image_get_red_size(storage->format) : 0;
            result = 1;
            break;
         case GL_RENDERBUFFER_GREEN_SIZE:
            params[0] = storage ? khrn_image_get_green_size(storage->format) : 0;
            result = 1;
            break;
         case GL_RENDERBUFFER_BLUE_SIZE:
            params[0] = storage ? khrn_image_get_blue_size(storage->format) : 0;
            result = 1;
            break;
         case GL_RENDERBUFFER_ALPHA_SIZE:
            params[0] = storage ? khrn_image_get_alpha_size(storage->format) : 0;
            result = 1;
            break;
         case GL_RENDERBUFFER_DEPTH_SIZE:
            params[0] = storage ? khrn_image_get_z_size(storage->format) : 0;
            result = 1;
            break;
         case GL_RENDERBUFFER_STENCIL_SIZE:
            params[0] = storage ? khrn_image_get_stencil_size(storage->format) : 0;
            result = 1;
            break;
         default:
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
            result = 0;
            break;
         }

         mem_maybe_unlock(renderbuffer->mh_storage);
         mem_unlock(state->mh_bound_renderbuffer);
      } else {
         glxx_server_state_set_error(state, GL_INVALID_OPERATION);
         result = 0;
      }
   } else {
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
      result = 0;
   }

   GLXX_UNLOCK_SERVER_STATE();

   return result;
}

GLboolean glIsFramebuffer_impl (GLuint framebuffer)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   GLboolean result = glxx_shared_get_framebuffer((GLXX_SHARED_T *)mem_lock(state->mh_shared), framebuffer, false) != MEM_INVALID_HANDLE;
   mem_unlock(state->mh_shared);

   GLXX_UNLOCK_SERVER_STATE();

   return result;
}

void glBindFramebuffer_impl (GLenum target, GLuint framebuffer)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (target == GL_FRAMEBUFFER) {
      MEM_HANDLE_T handle = MEM_INVALID_HANDLE;

      if (framebuffer) {
         handle = glxx_shared_get_framebuffer((GLXX_SHARED_T *)mem_lock(state->mh_shared), framebuffer, true);
         mem_unlock(state->mh_shared);

         if (handle == MEM_INVALID_HANDLE) {
            glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);

            GLXX_UNLOCK_SERVER_STATE();
            return;
         }
      }

      MEM_ASSIGN(state->mh_bound_framebuffer, handle);
   } else
      glxx_server_state_set_error(state, GL_INVALID_OPERATION);

   GLXX_UNLOCK_SERVER_STATE();
}

void glDeleteFramebuffers_impl (GLsizei n, const GLuint *framebuffers)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   GLXX_SHARED_T *shared = (GLXX_SHARED_T *)mem_lock(state->mh_shared);

   if (n < 0)
      glxx_server_state_set_error(state, GL_INVALID_VALUE);   // The conformance tests insist...
   else if (framebuffers) {
      int i;
      for (i = 0; i < n; i++)
         if (framebuffers[i]) {
            MEM_HANDLE_T handle = glxx_shared_get_framebuffer(shared, framebuffers[i], false);

            if (handle != MEM_INVALID_HANDLE) {
               if (state->mh_bound_framebuffer == handle)
                  MEM_ASSIGN(state->mh_bound_framebuffer, MEM_INVALID_HANDLE);

               glxx_shared_delete_framebuffer(shared, framebuffers[i]);
            }
         }
   }

   mem_unlock(state->mh_shared);

   GLXX_UNLOCK_SERVER_STATE();
}

void glGenFramebuffers_impl (GLsizei n, GLuint *framebuffers)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   GLXX_SHARED_T *shared = (GLXX_SHARED_T *)mem_lock(state->mh_shared);

   int32_t i = 0;

   if (n < 0)
      glxx_server_state_set_error(state, GL_INVALID_VALUE);   // The conformance tests insist...
   else if (framebuffers)
      while (i < n) {
         if (glxx_shared_get_framebuffer(shared, shared->next_framebuffer, false) == MEM_INVALID_HANDLE)
            framebuffers[i++] = shared->next_framebuffer;

         shared->next_framebuffer++;
      }

   mem_unlock(state->mh_shared);

   GLXX_UNLOCK_SERVER_STATE();
}

GLenum glCheckFramebufferStatus_impl (GLenum target)
{
   GLenum result;
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   result = glxx_check_framebuffer_status(state, target);

   GLXX_UNLOCK_SERVER_STATE();
   return result;
}

void glFramebufferTexture2D_impl (GLenum target, GLenum a, GLenum textarget, GLuint texture, GLint level)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (target == GL_FRAMEBUFFER && is_valid_attachment(a)) {
      if (state->mh_bound_framebuffer != MEM_INVALID_HANDLE) {
         GLXX_ATTACHMENT_INFO_T *attachment;
         GLXX_FRAMEBUFFER_T *framebuffer = (GLXX_FRAMEBUFFER_T *)mem_lock(state->mh_bound_framebuffer);

         vcos_assert(framebuffer);

         switch (a) {
         case GL_COLOR_ATTACHMENT0:
            attachment = &framebuffer->attachments.color;
            break;
         case GL_DEPTH_ATTACHMENT:
            attachment = &framebuffer->attachments.depth;
            break;
         case GL_STENCIL_ATTACHMENT:
            attachment = &framebuffer->attachments.stencil;
            break;
         default:
            attachment = NULL;
           UNREACHABLE();
            break;
         }

         if (texture) {
            if (level != 0)
               glxx_server_state_set_error(state, GL_INVALID_VALUE);
            else {
               if (!glxx_is_texture_target(state, textarget))
                  glxx_server_state_set_error(state, GL_INVALID_ENUM);
               else if (IS_GL_11(state) && GL_TEXTURE_2D != textarget && GL_TEXTURE_EXTERNAL_OES != textarget)
                  glxx_server_state_set_error(state, GL_INVALID_ENUM);                  
               else {
                  MEM_HANDLE_T handle = glxx_shared_get_texture((GLXX_SHARED_T *)mem_lock(state->mh_shared), texture);
                  mem_unlock(state->mh_shared);

                  if (handle == MEM_INVALID_HANDLE)
                     glxx_server_state_set_error(state, GL_INVALID_OPERATION);
                  else {
                     if (((GLXX_TEXTURE_T *)mem_lock(handle))->is_cube != (textarget != GL_TEXTURE_2D && textarget != GL_TEXTURE_EXTERNAL_OES))
                        glxx_server_state_set_error(state, GL_INVALID_OPERATION);
                     else {
                        attachment->type = GL_TEXTURE;
                        attachment->target = textarget;
                        attachment->level = level;

                        MEM_ASSIGN(attachment->mh_object, handle);
                     }
                     mem_unlock(handle);
                  }
               }
            }
         } else {
            attachment->type = GL_NONE;
            attachment->target = 0;
            attachment->level = 0;

            MEM_ASSIGN(attachment->mh_object, MEM_INVALID_HANDLE);
         }

         mem_unlock(state->mh_bound_framebuffer);
      } else
         glxx_server_state_set_error(state, GL_INVALID_OPERATION);
   } else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GLXX_UNLOCK_SERVER_STATE();
}

void glFramebufferRenderbuffer_impl (GLenum target, GLenum a, GLenum renderbuffertarget, GLuint renderbuffer)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   vcos_assert(state);

   if (target == GL_FRAMEBUFFER && is_valid_attachment(a)) {
      if (state->mh_bound_framebuffer != MEM_INVALID_HANDLE) {
         GLXX_ATTACHMENT_INFO_T *attachment;
         GLXX_FRAMEBUFFER_T *framebuffer = (GLXX_FRAMEBUFFER_T *)mem_lock(state->mh_bound_framebuffer);

         vcos_assert(framebuffer);

         switch (a) {
         case GL_COLOR_ATTACHMENT0:
            attachment = &framebuffer->attachments.color;
            break;
         case GL_DEPTH_ATTACHMENT:
            attachment = &framebuffer->attachments.depth;
            break;
         case GL_STENCIL_ATTACHMENT:
            attachment = &framebuffer->attachments.stencil;
            break;
         default:
            attachment = NULL;
           UNREACHABLE();
            break;
         }

         vcos_assert(attachment);

         if (renderbuffer) {
            if (renderbuffertarget == GL_RENDERBUFFER) {
               MEM_HANDLE_T handle = glxx_shared_get_renderbuffer((GLXX_SHARED_T *)mem_lock(state->mh_shared), renderbuffer, false);
               mem_unlock(state->mh_shared);

               if (handle == MEM_INVALID_HANDLE)
                  glxx_server_state_set_error(state, GL_INVALID_OPERATION);
               else {
                  attachment->type = GL_RENDERBUFFER;
                  attachment->target = 0;
                  attachment->level = 0;

                  MEM_ASSIGN(attachment->mh_object, handle);
               }
            } else
               glxx_server_state_set_error(state, GL_INVALID_ENUM);
         } else {
            attachment->type = GL_NONE;
            attachment->target = 0;
            attachment->level = 0;

            MEM_ASSIGN(attachment->mh_object, MEM_INVALID_HANDLE);
         }

         mem_unlock(state->mh_bound_framebuffer);
      } else
         glxx_server_state_set_error(state, GL_INVALID_OPERATION);
   } else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GLXX_UNLOCK_SERVER_STATE();
}

/*
   GetFramebufferParameteriv

   FRAMEBUFFER OBJECT TYPE NONE GetFramebufferParameteriv
   FRAMEBUFFER OBJECT NAME 0 GetFramebufferParameteriv
   FRAMEBUFFER TEXTURE LEVEL 0 GetFramebufferParameteriv
   FRAMEBUFFER TEXTURE CUBE MAP FACE +ve X face GetFramebufferParameteriv
*/

int glGetFramebufferAttachmentParameteriv_impl (GLenum target, GLenum attachment, GLenum pname, GLint *params)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   int result;

   if (target == GL_FRAMEBUFFER && is_valid_attachment(attachment)) {
      if (state->mh_bound_framebuffer != MEM_INVALID_HANDLE) {
         GLXX_FRAMEBUFFER_T *framebuffer = (GLXX_FRAMEBUFFER_T *)mem_lock(state->mh_bound_framebuffer);

         GLXX_ATTACHMENT_INFO_T *attachmentinfo;

         switch (attachment) {
         case GL_COLOR_ATTACHMENT0:
            attachmentinfo = &framebuffer->attachments.color;
            break;
         case GL_DEPTH_ATTACHMENT:
            attachmentinfo = &framebuffer->attachments.depth;
            break;
         case GL_STENCIL_ATTACHMENT:
            attachmentinfo = &framebuffer->attachments.stencil;
            break;
         default:
            attachmentinfo = 0;
         }

         if (attachmentinfo) {
            switch (attachmentinfo->type) {
            case GL_RENDERBUFFER:
               switch (pname) {
               case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
                  params[0] = GL_RENDERBUFFER;
                  result = 1;
                  break;
               case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
               {
                  GLXX_RENDERBUFFER_T *renderbuffer = (GLXX_RENDERBUFFER_T *)mem_lock(attachmentinfo->mh_object);
                  params[0] = renderbuffer->name;
                  result = 1;
                  mem_unlock(attachmentinfo->mh_object);
                  break;
               }
               case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
               case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
               default:
                  glxx_server_state_set_error(state, GL_INVALID_ENUM);
                  result = 0;
               }
               break;
            case GL_TEXTURE:
               switch (pname) {
               case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
                  params[0] = GL_RENDERBUFFER;
                  result = 1;
                  break;
               case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
               {
                  GLXX_TEXTURE_T *texture = (GLXX_TEXTURE_T *)mem_lock(attachmentinfo->mh_object);
                  params[0] = texture->name;
                  result = 1;
                  mem_unlock(attachmentinfo->mh_object);
                  break;
               }
               case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
                  params[0] = attachmentinfo->level;
                  result = 1;
                  break;
               case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
                  if(IS_GL_11(state)) {
                     glxx_server_state_set_error(state, GL_INVALID_ENUM);
                     result = 0;
                  }
                  else {
                  //TODO: is this right? We return integers rather than enums
                     switch (attachmentinfo->target) {
                     case GL_TEXTURE_2D:
					 case GL_TEXTURE_EXTERNAL_OES:
                        params[0] = 0;
                        break;
                     case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
                        params[0] = 0;
                        break;
                     case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
                        params[0] = 1;
                        break;
                     case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
                        params[0] = 2;
                        break;
                     case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
                        params[0] = 3;
                        break;
                     case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
                        params[0] = 4;
                        break;
                     case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                        params[0] = 5;
                        break;
                     default:
                        //Unreachable
                       UNREACHABLE();
                     }
                     result = 1;
                  }
                  break;
               default:
                  glxx_server_state_set_error(state, GL_INVALID_ENUM);
                  result = 0;
               }
               break;
            case GL_NONE:
               switch (pname) {
               case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
                  params[0] = GL_NONE;
                  result = 1;
                  break;
               case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
               case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
               case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
               default:
                  glxx_server_state_set_error(state, GL_INVALID_ENUM);
                  result = 0;
               }
               break;
            default:
               // Unreachable
               result = 0;
              UNREACHABLE();
            }
         } else {
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
            result = 0;
         }

         mem_unlock(state->mh_bound_framebuffer);
      } else {
         glxx_server_state_set_error(state, GL_INVALID_OPERATION);
         result = 0;
      }
   } else {
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
      result = 0;
   }

   GLXX_UNLOCK_SERVER_STATE();

   return result;
}

void glGenerateMipmap_impl (GLenum target)
{
   GLXX_TEXTURE_T *texture;
   MEM_HANDLE_T thandle;
   bool invalid_operation;
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   vcos_assert(state);

   texture = glxx_server_state_get_texture(state, target, GL_FALSE, &thandle);

   if (texture) {
      switch (target) {
      case GL_TEXTURE_2D:
	  case GL_TEXTURE_EXTERNAL_OES:
         if (!glxx_texture_generate_mipmap(texture, TEXTURE_BUFFER_TWOD, &invalid_operation))
            glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
         else if (invalid_operation)
            glxx_server_state_set_error(state, GL_INVALID_OPERATION);
         break;
      case GL_TEXTURE_CUBE_MAP:
         if(IS_GL_11(state)) {
            glxx_server_state_set_error(state, GL_INVALID_ENUM);
         }
         else if (glxx_texture_is_cube_complete(texture)) {
            int i;
            for (i = TEXTURE_BUFFER_POSITIVE_X; i <= TEXTURE_BUFFER_NEGATIVE_Z; i++) {
               if (!glxx_texture_generate_mipmap(texture, i, &invalid_operation))
                  glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
               else if (invalid_operation)
                  glxx_server_state_set_error(state, GL_INVALID_OPERATION);
            }
         } else
            glxx_server_state_set_error(state, GL_INVALID_OPERATION);
         break;
      default:
        UNREACHABLE();
         break;
      }

      mem_unlock(thandle);
   }

   GLXX_UNLOCK_SERVER_STATE();
}

void glxx_set_state_change ()
{
	GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();
	GLXX_UNLOCK_SERVER_STATE();
}