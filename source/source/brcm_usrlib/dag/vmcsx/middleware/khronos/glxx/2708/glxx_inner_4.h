/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GLXX_INNER_4_H
#define GLXX_INNER_4_H

#include "middleware/khronos/common/khrn_image.h"
#include "middleware/khronos/common/2708/khrn_prod_4.h"
#include "middleware/khronos/common/2708/khrn_fmem_4.h"
#include "interface/khronos/include/GLES/gl.h"
#include "middleware/khronos/glxx/glxx_server.h"

#define FIXABLE_TYPE_NULL 0
#define FIXABLE_TYPE_ALLOC 1
#define FIXABLE_TYPE_FDATA 2
#define FIXABLE_TYPE_CMEM 3
#define FIXABLE_TYPE_SPECIAL 4
#define FIXABLE_TYPE_IMAGE 5
#define FIXABLE_TYPE_USED 6
#define FIXABLE_TYPE_JUNK 7

typedef struct {
   MEM_HANDLE_T mh_color_image;
   MEM_HANDLE_T mh_aux_image;
   KHRN_IMAGE_FORMAT_T col_format;
   uint32_t width;
   uint32_t height;
   uint32_t pad_width;
   uint32_t pad_height;
   bool ms;
   bool have_depth;
   bool have_stencil;
   bool dither_enable;
} GLXX_HW_FRAMEBUFFER_T;

typedef struct {
   uint16_t flags;
   uint8_t xxx0;
   uint8_t num_varyings;
   uint32_t fshader;
   uint32_t funif;
   uint16_t xxx1;
   uint8_t vattrsel;
   uint8_t vattrsize;
   uint32_t vshader;
   uint32_t vunif;
   uint16_t xxx2;
   uint8_t cattrsel;
   uint8_t cattrsize;
   uint32_t cshader;
   uint32_t cunif;
   struct
   {
      uint32_t base;
      uint8_t sizem1;
      uint8_t stride;
      uint8_t voffset;
      uint8_t coffset;
   } attr[8];
} GLXX_HW_SHADER_RECORD_T;

typedef struct {
   uint32_t color;
   float depth;
   uint8_t stencil;
   bool	colorClear;
   bool depthClear;
   bool stencilClear;
   uint32_t colorMask;
   uint8_t stencilMask;
} GLXX_HW_CLEAR_T;

typedef struct
{
   uint32_t name;
   bool xxx_empty;

   KHRN_FMEM_T *fmem;

   uint32_t num_tiles_x, num_tiles_y;
   GLXX_HW_FRAMEBUFFER_T installed_fb; // = {MEM_INVALID_HANDLE, MEM_INVALID_HANDLE};
   uint32_t hw_frame_count; // = 0;

   bool color_buffer_valid;
   bool aux_buffer_valid;
   bool color_load;
   bool depth_load;
   bool stencil_load;
   bool drawn;
   /*bool color_buffer_clear;
   bool depth_buffer_clear;*/
   uint32_t color_value;
   float depth_value;
   uint8_t stencil_value;
} GLXX_HW_RENDER_STATE_T;

extern bool glxx_hw_render_state_flush(GLXX_HW_RENDER_STATE_T *render_state);

extern bool glxx_hw_start_frame_internal(GLXX_HW_RENDER_STATE_T *rs, GLXX_HW_FRAMEBUFFER_T *fb);
extern void glxx_hw_invalidate_internal(GLXX_HW_RENDER_STATE_T *rs, bool color, bool depth, bool stencil, bool multisample);

extern void glxx_hw_handle_end_of_frame(void);
extern void glxx_hw_handle_flush(void);

extern void glxx_hw_discard_frame(GLXX_HW_RENDER_STATE_T *rs);

extern uint32_t glxx_hw_convert_operation(GLenum operation);
extern uint32_t glxx_hw_convert_test_function(GLenum function);

//for common/2708/hwcommon.c
extern void glxx_2708_hw_init(void);
extern void glxx_2708_hw_term(void);
//

//internal functions/variabless used by gl11/2708/hw.c and gl20/2708/hw.c
extern int glxx_convert_wrap(GLenum wrap);
extern int glxx_convert_filter(GLenum filter);
extern uint32_t glxx_enable_back(GLenum mode);
extern uint32_t glxx_enable_front(GLenum mode);
extern uint32_t glxx_front_facing_is_clockwise(GLenum mode);
extern uint32_t glxx_hw_primitive_mode_to_type(GLenum primitive_mode);

extern bool glxx_hw_texture_fix(MEM_HANDLE_T thandle);

extern uint32_t glxx_hw_acquire_cache_interlock(void);
extern void glxx_hw_release_cache_interlock(uint32_t pool_index);

extern uint32_t glxx_render_state_name(void);

extern bool glxx_lock_fixer_stuff(GLXX_HW_RENDER_STATE_T *rs);
extern void glxx_unlock_fixer_stuff(void);

extern void glxx_hw_stash_state(void);
extern void glxx_hw_unstash_state(void);

extern uint8_t *glxx_big_mem_alloc_cle(int size);
extern uint32_t *glxx_big_mem_alloc_junk(int size, int align);
extern bool glxx_big_mem_insert(uint32_t *location, MEM_HANDLE_T handle, uint32_t offset);
extern bool glxx_big_mem_add_fix(uint8_t **p, MEM_HANDLE_T handle, uint32_t offset);
extern bool glxx_big_mem_add_special(uint8_t **p, uint32_t special_i, uint32_t offset);
extern bool glxx_hw_insert_interlock(MEM_HANDLE_T handle, uint32_t offset);

extern GLXX_HW_RENDER_STATE_T *glxx_install_framebuffer(GLXX_SERVER_STATE_T *state, GLXX_HW_FRAMEBUFFER_T *fb, bool main_buffer);

#endif //GLXX_2708_HW_H
