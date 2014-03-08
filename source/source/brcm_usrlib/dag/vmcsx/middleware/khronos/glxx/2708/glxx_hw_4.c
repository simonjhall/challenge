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
#include "interface/khronos/common/khrn_int_color.h"
#include "middleware/khronos/common/2708/khrn_prod_4.h"
#include "middleware/khronos/common/2708/khrn_interlock_priv_4.h"
#include "middleware/khronos/common/2708/khrn_render_state_4.h"
#include "middleware/khronos/glxx/2708/glxx_inner_4.h"
#include "middleware/khronos/glxx/2708/glxx_tu_4.h"
#include "middleware/khronos/glxx/2708/glxx_shader_4.h"
#include "middleware/khronos/glxx/2708/glxx_attr_sort_4.h"
#include "middleware/khronos/glxx/glxx_server.h"
#include "middleware/khronos/glxx/glxx_hw.h"
#include "middleware/khronos/glxx/glxx_framebuffer.h"
#include "middleware/khronos/gl20/gl20_program.h"
#include "middleware/khronos/gl20/2708/gl20_support_4.h"
#include "middleware/khronos/glsl/glsl_common.h"
#include "middleware/khronos/glsl/2708/glsl_allocator_4.h"
#include "middleware/khronos/gl11/2708/gl11_support_4.h"
#include "middleware/khronos/gl11/2708/gl11_shader_4.h"
#include "helpers/vc_image/vc_image.h"           // For debugging

#include "vcfw/rtos/rtos.h"


#if defined(__CC_ARM) && !defined(USHRT_MAX)
#define USHRT_MAX       0xffff
#endif

//#define STEREOSCOPIC_FRAMEBUFFER_HACK
//#define WORKAROUND_VND_12

#ifdef DUMP_SHADER
#include "middleware/khronos/glsl/2708/glsl_qdisasm_4.h"
#include <stdio.h>
#endif
// Global variables -------------------------------------------

#ifdef SIMPENROSE_RECORD_OUTPUT
/* if we don't have any uniforms for a particular shader, point here so the
 * recorder won't get confused */
static void *dummy_unifs = NULL;
#endif
static uint32_t *dummy_texture_data = NULL;
#ifdef ANDROID
static MEM_HANDLE_T dummy_texture_mem = MEM_INVALID_HANDLE;
#endif

/*************************************************************
 Static function forwards
 *************************************************************/
#ifdef DUMP_SHADER
static void dump_shader(GLXX_FIXABLE_ADDR_T *fxshader, GLXX_FIXABLE_ADDR_T *fxunif);
#endif

static bool install_uniforms(
   uint32_t *startaddr_location,
   GLXX_SERVER_STATE_T *state,
   GL20_PROGRAM_T *program,
   uint32_t count,
   MEM_HANDLE_T hmap,
   GL20_HW_INDEXED_UNIFORM_T *iu,
   uint32_t * num_vpm_rows,
   GLXX_ATTRIB_T *attrib,
   bool rso_format,
   unsigned int fb_height);

static bool get_shaders(
    GL20_PROGRAM_T *program,
    GLXX_HW_SHADER_RECORD_T *shader_out,
    MEM_HANDLE_T *cunifmap_out,
    MEM_HANDLE_T *vunifmap_out,
    MEM_HANDLE_T *funifmap_out,
    uint32_t *color_varyings_out,
    GLXX_SERVER_STATE_T *state,
    GLXX_ATTRIB_T *attrib,
    uint32_t *mergeable_attribs,
    uint32_t *cattribs_order,
    uint32_t *vattribs_order);
    
static bool do_vcd_setup(
   GLXX_HW_SHADER_RECORD_T *shader_record,
   GLXX_ATTRIB_T *attrib,
   MEM_HANDLE_T *attrib_handles,
   uint32_t cattribs_live,
   uint32_t vattribs_live,
   uint32_t * mergeable_attribs,
   uint32_t * cattribs_order,
   uint32_t * vattribs_order,
   uint32_t * num_vpm_rows_c,
   uint32_t * num_vpm_rows_v,
   uint32_t *attr_count
   );  

static bool glxx_install_tex_param(GLXX_SERVER_STATE_T *state, uint32_t *location, uint32_t u0, uint32_t u1);


static uint32_t convert_primitive_type(GLenum mode);
static uint32_t convert_index_type(GLenum type);

#ifdef __BCM2708A0__                  
static bool backend_uniform_tex_param0( uint32_t u1, 
                  GLXX_SERVER_STATE_T *state,
                  GL20_HW_INDEXED_UNIFORM_T *iu,
                  uint32_t *location);   
                  
static bool backend_uniform_tex_param1( uint32_t u1, 
                  GLXX_SERVER_STATE_T *state,
                  GL20_HW_INDEXED_UNIFORM_T *iu,
                  uint32_t *location);   
#else
static bool backend_uniform_address( uint32_t u1, 
                  GLXX_SERVER_STATE_T *state,
                  GL20_HW_INDEXED_UNIFORM_T *iu,
                  uint32_t *location);   
#endif
/*************************************************************
 Global Functions
 *************************************************************/


void glxx_hw_finish()
{
   khrn_render_state_flush_all(KHRN_RENDER_STATE_TYPE_GLXX);
}

void glxx_hw_finish_context(GLXX_SERVER_STATE_T *state, bool wait)
{
   UNUSED(state);

   //khrn_render_state_flush_all(KHRN_RENDER_STATE_TYPE_GLXX);
   if (wait) {
      khrn_hw_wait();
   }
}

void glxx_hw_invalidate_frame(GLXX_SERVER_STATE_T *state, bool color, bool depth, bool stencil, bool multisample)
{
   GLXX_HW_FRAMEBUFFER_T fb;
   GLXX_HW_RENDER_STATE_T *rs;
   
   rs = glxx_install_framebuffer(state, &fb, true);
   if (!rs)
      return;

   glxx_hw_invalidate_internal(rs, color, depth, stencil, multisample);
}

/*!
 * \brief Terminate
 *
 */

void glxx_hw_term()
{
   gl11_hw_shader_cache_reset();

#ifdef SIMPENROSE_RECORD_OUTPUT
   rtos_priorityfree(dummy_unifs);
   dummy_unifs = NULL;
#endif

   if (dummy_texture_data != NULL)
   {
#ifdef ANDROID
      if (dummy_texture_mem != MEM_INVALID_HANDLE)
      {
         mem_unlock(dummy_texture_mem);
         mem_release(dummy_texture_mem);
         dummy_texture_mem = MEM_INVALID_HANDLE;
      }
#else
      rtos_priorityfree(dummy_texture_data);
#endif
      dummy_texture_data = NULL;
   }
}

static void set_current_render_state(GLXX_SERVER_STATE_T *state, uint32_t rs_name) {
   state->changed_cfg = true;
   state->changed_linewidth = true;
   state->changed_polygon_offset = true;
   state->changed_viewport = true;
   state->old_flat_shading_flags = ~0;

   state->current_render_state = rs_name;
}

GLXX_HW_RENDER_STATE_T *glxx_install_framebuffer(GLXX_SERVER_STATE_T *state, GLXX_HW_FRAMEBUFFER_T *fb, bool main_buffer)
{
   MEM_HANDLE_T hcolor, hdepth, hstencil;
   KHRN_IMAGE_T *color;
   uint32_t i;
   bool multisample;
   GLXX_HW_RENDER_STATE_T *rs;

   if (state->mh_bound_framebuffer != MEM_INVALID_HANDLE && !main_buffer)
   {
      GLXX_FRAMEBUFFER_T *framebuffer = (GLXX_FRAMEBUFFER_T *)mem_lock(state->mh_bound_framebuffer);

      hcolor = glxx_attachment_info_get_image(&framebuffer->attachments.color);
      hdepth = glxx_attachment_info_get_image(&framebuffer->attachments.depth);
      hstencil = glxx_attachment_info_get_image(&framebuffer->attachments.stencil);
      multisample = false;

      fb->mh_aux_image = hdepth != MEM_INVALID_HANDLE ? hdepth : hstencil; /* TODO naughty */
      fb->have_depth = framebuffer->attachments.depth.type != GL_NONE;
      fb->have_stencil = framebuffer->attachments.stencil.type != GL_NONE;
   }
   else
   {
      hcolor = state->mh_draw;
      hdepth = state->mh_depth;
      multisample = state->mh_multi != MEM_INVALID_HANDLE;

      if (multisample)
         fb->mh_aux_image = state->mh_multi; /* TODO naughty */
      else
         fb->mh_aux_image = state->mh_depth; /* TODO naughty */

      fb->have_depth = state->mh_depth != MEM_INVALID_HANDLE;
      fb->have_stencil = fb->have_depth;
   }

   fb->dither_enable = state->caps.dither;

   if(hcolor==MEM_INVALID_HANDLE)
      return NULL;

   color = (KHRN_IMAGE_T *)mem_lock(hcolor);

   fb->mh_color_image = hcolor; /* TODO naughty */

   fb->width = color->width;
   fb->height = color->height;
   fb->col_format = color->format;
#ifdef STEREOSCOPIC_FRAMEBUFFER_HACK
   fb->height /= 2;     /* Stereo writes to every other line, so the top doesn't fit */
#endif
#ifdef GLXX_FORCE_MULTISAMPLE
   fb->ms = true;
#else
   fb->ms = multisample;
#endif
   fb->pad_width = color->stride * 8 / khrn_image_get_bpp(color->format);
#ifdef __BCM2708A0__
   if (fb->ms)
      fb->pad_height = (fb->height + (KHRN_HW_TILE_HEIGHT>>1) - 1) & ~((KHRN_HW_TILE_HEIGHT>>1) - 1);
   else
      fb->pad_height = (fb->height + (KHRN_HW_TILE_HEIGHT - 1)) & ~(KHRN_HW_TILE_HEIGHT - 1);
#else
   fb->pad_height = fb->height;
#endif

#ifdef __BCM2708A0__
   if (fb->pad_width < (fb->ms ? (KHRN_HW_TILE_WIDTH>>1) : KHRN_HW_TILE_WIDTH) || fb->pad_height < (fb->ms ? (KHRN_HW_TILE_HEIGHT>>1) : KHRN_HW_TILE_HEIGHT))
   {
      /* Fail early so we don't fail in start_frame and cause problems */
      mem_unlock(hcolor);
      return NULL;
   }
#endif

   i = khrn_interlock_render_state_i(khrn_interlock_get_writer(&color->interlock));
   /* TODO: usurp KHRN_RENDER_STATE_TYPE_COPY_BUFFER? */
   if (i != (uint32_t)~0 && khrn_render_state_get_type(i) == KHRN_RENDER_STATE_TYPE_GLXX)
   {
      rs = (GLXX_HW_RENDER_STATE_T *)khrn_render_state_get_data(i);
      /* If render state has changed then reissue all the start of frame instructions */
      if (rs->name != state->current_render_state) {
         set_current_render_state(state, rs->name);
      }
   }
   else
   {
      i = khrn_render_state_start(KHRN_RENDER_STATE_TYPE_GLXX);
      rs = (GLXX_HW_RENDER_STATE_T *)khrn_render_state_get_data(i);
      rs->name = i;

#ifdef SIMPENROSE_RECORD_OUTPUT
      {
         record_begin();
         color = (KHRN_IMAGE_T *)mem_lock(hcolor);
         record_set_frame_config(
            color->stride * 8 / khrn_image_get_bpp(color->format),
            (multisample) ? (color->height + 31) & ~31 : (color->height + 63) & ~63,
            khrn_image_get_bpp(color->format), khrn_image_is_tformat(color->format));

         record_map_mem_buffer_section(color->mh_storage, color->offset,  mem_get_size(color->mh_storage)-color->offset, L_FRAMEBUFFER, RECORD_BUFFER_IS_BOTH, mem_get_align(color->mh_storage));
         mem_unlock(hcolor);
      }
#endif

      /* TODO: is this the right place to reset these?          */
      /*       changed_settings now in set_current_render_state */
      set_current_render_state(state, rs->name);

      if (!glxx_hw_start_frame_internal(rs, fb))
      {
         glxx_hw_discard_frame(rs);
         mem_unlock(hcolor);
		 glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
         return NULL;    /* TODO: distinguish between out-of-memory and null clip rectangle */
      }
   }

   mem_unlock(hcolor);
   return rs;
}

/*!
 * \brief Converts the texture wrap setting from the GLenum
 *        representation to the internal one used in the simulator.
 *
 * \param wrap is the GL texture wrap setting.
 */
int glxx_convert_wrap(GLenum wrap)
{
   switch (wrap) {
   case GL_REPEAT:
      return 0;
   case GL_CLAMP_TO_EDGE:
      return 1;
   case GL_MIRRORED_REPEAT:
      return 2;
   default:
      UNREACHABLE();
      return 0;
   }
}


/*!
 * \brief Converts the filter from the GLenum
 *        representation to the internal one used in the simulator.
 *
 * \param filter is the GL mipmap filter.
 */
int glxx_convert_filter(GLenum filter)
{
   switch (filter) {
   case GL_NEAREST:
      return 1;
   case GL_LINEAR:
      return 0;
   case GL_NEAREST_MIPMAP_NEAREST:
      return 2;
   case GL_NEAREST_MIPMAP_LINEAR:
      return 3;
   case GL_LINEAR_MIPMAP_NEAREST:
      return 4;
   case GL_LINEAR_MIPMAP_LINEAR:
      return 5;
   default:
     UNREACHABLE();
      return 0;
   }
}

/*!
 * \brief \a Undocumented
 *
 * TODO: I don't know what this function is doing.
 *
 * \param mode OpenGL mode flag.
 */
uint32_t glxx_enable_back(GLenum mode)
{
   vcos_assert(mode == GL_FRONT || mode == GL_BACK || mode == GL_FRONT_AND_BACK);

   return mode == GL_FRONT;
}

 /*!
 * \brief \a Undocumented
 *
 * TODO: I don't know what this function is doing.
 *
 * \param mode OpenGL mode flag.
 */
uint32_t glxx_enable_front(GLenum mode)
{
   vcos_assert(mode == GL_FRONT || mode == GL_BACK || mode == GL_FRONT_AND_BACK);

   return mode == GL_BACK;
}


/*!
 * \brief \a Undocumented
 *
 * TODO: I don't know what this function is doing.
 *
 * \param mode OpenGL mode flag.
 */
uint32_t glxx_front_facing_is_clockwise(GLenum mode)
{
   vcos_assert(mode == GL_CW || mode == GL_CCW);

   return mode == GL_CW;
}

uint32_t glxx_hw_primitive_mode_to_type(GLenum primitive_mode)
{
   switch (primitive_mode)
   {
   case GL_POINTS:
      return 0;
   case GL_LINES:
   case GL_LINE_LOOP:
   case GL_LINE_STRIP:
      return 1;
   case GL_TRIANGLES:
   case GL_TRIANGLE_STRIP:
   case GL_TRIANGLE_FAN:
      return 2;
   default:
      UNREACHABLE();
      return 0;
   }
}

static bool do_vcd_setup(
   GLXX_HW_SHADER_RECORD_T *shader_record,
   GLXX_ATTRIB_T *attrib,
   MEM_HANDLE_T *attrib_handles,
   uint32_t cattribs_live,
   uint32_t vattribs_live,
   uint32_t *mergeable_attribs,
   uint32_t *cattribs_order,
   uint32_t *vattribs_order,
   uint32_t *num_vpm_rows_c,
   uint32_t *num_vpm_rows_v,
   uint32_t *attr_count
   )
{
   bool anything = false;
   uint32_t i, j;
   uint32_t n = 0, nv = 0, nc = 0;
   uint32_t last_vattrib = -1;
   uint32_t count_vpm_setup_c = 0;
   uint32_t count_vpm_setup_v = 0;
   uint32_t vpm_offset_v, vpm_offset_c;
   uint32_t total_vpm_offset_v, total_vpm_offset_c;
   uint32_t vattrsel = 0;
   uint32_t cattrsel = 0;

   UNUSED_NDEBUG(cattribs_live);
   UNUSED_NDEBUG(vattribs_live);
   UNUSED(mergeable_attribs);

   vcos_assert(!(cattribs_live & ~vattribs_live));

   vpm_offset_v = 0;             /* vpm_offset counts from the start of one read setup to make sure */
   vpm_offset_c = 0;             /* we never exceed 15 rows at a time.                              */
   total_vpm_offset_v = 0;       /* total_vpm_offset counts from the start of the VPM to ensure     */
   total_vpm_offset_c = 0;       /* attribute data never overlaps once loaded.                      */

   for(i = 0;i<4;i++) {
      num_vpm_rows_c[i] = 0;
      num_vpm_rows_v[i] = 0;
   }

   for (j = 0; j < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; j++) {
      uint32_t jstart = j;
      i = vattribs_order[j];
      
      if (i!=(uint32_t)~0) {
         uint32_t length;
         uint32_t new_num_rows;
         
         last_vattrib = j;
         
         vcos_assert(vattribs_live & 15<<(4*i) && attrib[i].enabled);
         
         length = attrib[i].size * khrn_get_type_size(attrib[i].type);

#ifdef GLXX_WANT_ATTRIBUTE_MERGING
         while(j<GLXX_CONFIG_MAX_VERTEX_ATTRIBS-1 && vattribs_order[j+1]!=(uint32_t)~0 && mergeable_attribs[vattribs_order[j]] == vattribs_order[j+1])
         {
            uint32_t new_length = length + attrib[vattribs_order[j+1]].size * khrn_get_type_size(attrib[vattribs_order[j+1]].type);
            if( ((((new_length + 3) & ~3)+3)/4) > 15) //too big to fit in current vpm_setup
               break;
            //do merge
            j++;
            length = new_length;
            last_vattrib = j;
         }
#endif
         vcos_assert( ((((length + 3) & ~3)+3)/4) <= 15 );//small enough to fit in a vpm_setup

         glxx_big_mem_insert(&shader_record->attr[n].base, attrib_handles[i], (uint32_t)attrib[i].pointer);

         shader_record->attr[n].base = i; /* SW-5891 temporarily store i here so can duplicate shader record if needed */
         shader_record->attr[n].sizem1 = length - 1;
         shader_record->attr[n].stride = attrib[i].stride ? attrib[i].stride : attrib[i].size * khrn_get_type_size(attrib[i].type);

         shader_record->attr[n].voffset = total_vpm_offset_v;
         total_vpm_offset_v += (length + 3) & ~3;
         vpm_offset_v += (length + 3) & ~3;
         vattrsel |= 1<<n;

         new_num_rows = (vpm_offset_v+3)/4;

         //can only load 15 rows at a time
         if(new_num_rows > 15)
         {
            count_vpm_setup_v ++;

            vpm_offset_v = (length + 3) & ~3;
            num_vpm_rows_v[count_vpm_setup_v] = (vpm_offset_v+3)/4;
         }
         else
            num_vpm_rows_v[count_vpm_setup_v] = new_num_rows;

         if (cattribs_order[jstart]!=(uint32_t)~0) /*want all the vattribs from this iteration in cattribs */
         {
            shader_record->attr[n].coffset = total_vpm_offset_c;
            vpm_offset_c += (length + 3) & ~3;
            total_vpm_offset_c += (length + 3) & ~3;

            new_num_rows = (vpm_offset_c+3)/4;

            //can only load 15 rows at a time
            if(new_num_rows > 15)
            {
               count_vpm_setup_c ++;

               vpm_offset_c = (length + 3) & ~3;
               num_vpm_rows_c[count_vpm_setup_c] = (vpm_offset_c+3)/4;
            }
            else
               num_vpm_rows_c[count_vpm_setup_c] = new_num_rows;

            nc++;
            cattrsel |= 1<<n;
         }
         
         nv++;
         n++;

         anything = true;
      }
   }
   
   //now handle any cattribs that didn't match the merged vattribs
   for (j = last_vattrib+1; j < GLXX_CONFIG_MAX_VERTEX_ATTRIBS*2; j++) {
      
      vcos_assert(n<GLXX_CONFIG_MAX_VERTEX_ATTRIBS);
      
      i = cattribs_order[j];
      
      if (i!=(uint32_t)~0)
      {
         uint32_t length;
         uint32_t new_num_rows;
   
         length = attrib[i].size * khrn_get_type_size(attrib[i].type);

#ifdef GLXX_WANT_ATTRIBUTE_MERGING
         while(j<GLXX_CONFIG_MAX_VERTEX_ATTRIBS*2-1 && cattribs_order[j+1]!=(uint32_t)~0  && mergeable_attribs[cattribs_order[j]] == cattribs_order[j+1])
         {
            //do merge
            j++;   
            length += attrib[cattribs_order[j]].size * khrn_get_type_size(attrib[cattribs_order[j]].type);
         }
#endif

         glxx_big_mem_insert(&shader_record->attr[n].base, attrib_handles[i], (uint32_t)attrib[i].pointer);
         
         shader_record->attr[n].base = i; /* SW-5891 temporarily store i here so can duplicate shader record if needed */
         shader_record->attr[n].sizem1 = length - 1;
         shader_record->attr[n].stride = attrib[i].stride ? attrib[i].stride : attrib[i].size * khrn_get_type_size(attrib[i].type);
         
         shader_record->attr[n].voffset = 0xff;
         
         shader_record->attr[n].coffset = total_vpm_offset_c;
         vpm_offset_c += (length + 3) & ~3;
         total_vpm_offset_c += (length + 3) & ~3;
         cattrsel |= 1<<n;

         new_num_rows = (vpm_offset_c+3)/4;

         //can only load 15 rows at a time
         if(new_num_rows > 15)
         {
            count_vpm_setup_c ++;

            vpm_offset_c = (length + 3) & ~3;
            num_vpm_rows_c[count_vpm_setup_c] = (vpm_offset_c+3)/4;
         }
         else
            num_vpm_rows_c[count_vpm_setup_c] = new_num_rows;
         
         nc++;
         n++;
   
         anything = true;
      }
   } 

   if (nc == 0 || nv == 0) {
      if (n == 8) return false;     /* No space. Surely there's a better way */

      shader_record->attr[n].base = khrn_hw_addr(khrn_hw_alias_direct(glxx_big_mem_alloc_junk(4, 4)));
      if (!shader_record->attr[n].base) return false;
      shader_record->attr[n].sizem1 = 0;
      shader_record->attr[n].stride = 0;
         
      if (nc == 0) {
         vcos_assert(vpm_offset_c == 0);
         shader_record->attr[n].coffset = 0;
         cattrsel |= 1<<n;
         num_vpm_rows_c[0] = 1;
      } else shader_record->attr[n].coffset = 0xff;

      if (nv == 0) {
         vcos_assert(vpm_offset_v == 0);
         shader_record->attr[n].voffset = 0;
         vattrsel |= 1<<n;
         num_vpm_rows_v[0] = 1;
      } else shader_record->attr[n].voffset = 0xff;

      n++;
   }

   shader_record->vattrsel = vattrsel;
   shader_record->vattrsize = total_vpm_offset_v;
   shader_record->cattrsel = cattrsel;
   shader_record->cattrsize = total_vpm_offset_c;

   *attr_count = n;

   return true;
}

/* TODO: get rid of fb and attrib parameters */
static void calculate_and_hide(GLXX_SERVER_STATE_T *state, GLXX_HW_FRAMEBUFFER_T *fb, GLXX_ATTRIB_T *attrib)
{
   float *modelview;
   int i;

#if GL_EXT_texture_format_BGRA8888
   /* TODO: we enumerate over textures elsewhere. Feels slightly wasteful. */
   for (i = 0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++)
   {
      MEM_HANDLE_T thandle = MEM_INVALID_HANDLE;

      state->shader.common.texture_rb_swap[i] = false;
      if (IS_GL_11(state))
      {
         if (state->texunits[i].enabled)
            thandle = state->bound_texture[i].mh_twod;
      }
      else
      {
         if (i < state->batch.num_samplers)
         {
            GL20_UNIFORM_INFO_T *ui;
            int index;
            bool is_cube;

            vcos_assert(state->batch.sampler_info != NULL);

            ui = &state->batch.uniform_info[state->batch.sampler_info[i].uniform];
            index = state->batch.uniform_data[ui->offset + state->batch.sampler_info[i].index];
            is_cube = ui->type != GL_SAMPLER_2D;

            vcos_assert(index >= 0 && index < GLXX_CONFIG_MAX_TEXTURE_UNITS);

            thandle = is_cube ? state->bound_texture[index].mh_cube : state->bound_texture[index].mh_twod;
         }
      }
      if (thandle != MEM_INVALID_HANDLE)
      {
         GLXX_TEXTURE_T *texture;

         texture = (GLXX_TEXTURE_T *)mem_lock(thandle);
         if (glxx_texture_check_complete(texture, false) == COMPLETE)
         {
            if (tu_image_format_rb_swap(glxx_texture_get_tformat(texture, false)))
            {
               state->shader.common.texture_rb_swap[i] = true;
            }
         }
         mem_unlock(thandle);
      }
   }
#endif

   state->shader.common.fb_rb_swap = tu_image_format_rb_swap(khrn_image_to_tf_format(fb->col_format));  /* TODO: slight hack - using tu function to determine fb thing */

   /* TODO: this copying shouldn't be necessary */
   for (i = 0; i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++)
   {
      if (attrib[i].enabled)
      {
         state->shader.common.attribs[i].type = attrib[i].type;
         state->shader.common.attribs[i].size = attrib[i].size;
         state->shader.common.attribs[i].norm = attrib[i].normalized;
      }
      else
      {
         state->shader.common.attribs[i].type = 0;
         state->shader.common.attribs[i].size = 0;
         state->shader.common.attribs[i].norm = 0;
      }
   }

   state->shader.common.primitive_type = glxx_hw_primitive_mode_to_type(state->batch.primitive_mode);
   state->shader.common.render_alpha = khrn_image_get_alpha_size(fb->col_format)>0;
   state->shader.common.rgb565 = khrn_image_get_green_size(fb->col_format) == 6;
   state->shader.common.use_stencil = fb->have_stencil && state->caps.stencil_test;
   state->shader.common.blend.ms = fb->ms;
#ifdef WORKAROUND_HW2924
   state->shader.common.workaround_hw2924 = fb->have_stencil && state->caps.stencil_test && (
      state->stencil_func.front.func != state->stencil_func.back.func ||
      state->stencil_func.front.ref != state->stencil_func.back.ref ||
      state->stencil_func.front.mask != state->stencil_func.back.mask ||
      state->stencil_op.front.fail != state->stencil_op.back.fail ||
      state->stencil_op.front.zfail != state->stencil_op.back.zfail ||
      state->stencil_op.front.zpass != state->stencil_op.back.zpass ||
      state->stencil_mask.front != state->stencil_mask.back);
#endif
#ifdef BCG_FB_LAYOUT
   state->shader.common.rso_format = khrn_image_is_rso(fb->col_format);
#else
   state->shader.common.rso_format = false;
#endif
   if (state->changed_backend)
   {
      if (state->caps.blend)
      {
         state->shader.common.blend.equation           = state->blend_equation.rgb;
         state->shader.common.blend.equation_alpha     = state->blend_equation.alpha;
         state->shader.common.blend.src_function       = state->blend_func.src_rgb;
         state->shader.common.blend.src_function_alpha = state->blend_func.src_alpha;
         state->shader.common.blend.dst_function       = state->blend_func.dst_rgb;
         state->shader.common.blend.dst_function_alpha = state->blend_func.dst_alpha;
      }
      else
      {
         state->shader.common.blend.equation = GL_FUNC_ADD;
         state->shader.common.blend.equation_alpha = GL_FUNC_ADD;
         state->shader.common.blend.src_function = GL_ONE;
         state->shader.common.blend.src_function_alpha = GL_ONE;
         state->shader.common.blend.dst_function = GL_ZERO;
         state->shader.common.blend.dst_function_alpha = GL_ZERO;
      }
      if (state->caps.color_logic_op)
         state->shader.common.blend.logic_op = state->logic_op;
      else
         state->shader.common.blend.logic_op = GL_COPY;

      /* TODO: sample_alpha_to_one? */
      state->shader.common.blend.sample_alpha_to_coverage = state->caps.sample_alpha_to_coverage;
      state->shader.common.blend.sample_coverage    = state->caps.sample_coverage;
      state->shader.common.blend.sample_coverage_v  = state->sample_coverage;

      state->changed_backend = false;
   }

   if (IS_GL_11(state))
   {
      /* TODO: This was already done in DrawElements_impl. Better just to copy? */
      gl11_hw_setup_attribs_live(state, &state->shader.cattribs_live, &state->shader.vattribs_live);
     
      /* Has to be copied every time because no changed flags are set when mode changes */
      state->shader.point_smooth = state->caps_fragment.point_smooth && state->batch.primitive_mode == 0;
      state->shader.line_smooth = state->caps_fragment.line_smooth && 
         (state->batch.primitive_mode >= 1 && state->batch.primitive_mode <= 3);

      if (state->changed_misc)
      {
         if (state->caps_fragment.alpha_test)
            state->shader.alpha_func = state->alpha_func.func;
         else
            state->shader.alpha_func = GL_ALWAYS;

         if (state->caps_fragment.fog)
            state->shader.fog_mode = state->fog.mode;
         else
            state->shader.fog_mode = 0;

         /*
            Choose which user clip plane test to use so that complementary planes don't produce gaps or overlap
         */
         if (!state->caps.clip_plane[0])
            state->shader.user_clip_plane = 0;
         else if (state->planes[0][0] > 0 ||
            (state->planes[0][0] == 0 && (state->planes[0][1] > 0 ||
            (state->planes[0][1] == 0 && (state->planes[0][2] > 0 ||
            (state->planes[0][2] == 0 && (state->planes[0][3] >= 0)))))))
            state->shader.user_clip_plane = 1;
         else
            state->shader.user_clip_plane = -1;

         state->changed_misc = false;
      }

      if (state->changed_light)
      {
         for (i = 0; i < GL11_CONFIG_MAX_LIGHTS; i++)
         {
            if (state->shader.lighting && state->lights[i].enabled)
            {
               state->shader.lights[i].enabled = true;
               state->shader.lights[i].position_w_is_0 = state->lights[i].position[3] == 0.0f;
               state->shader.lights[i].spot_cutoff_is_180 = state->lights[i].spot.cutoff == 180.0f;
            }
            else
            {
               state->shader.lights[i].enabled = false;
               state->shader.lights[i].position_w_is_0 = false;
               state->shader.lights[i].spot_cutoff_is_180 = false;
            }
         }
         if (state->shader.lighting) 
            state->shader.two_side = state->lightmodel.two_side;
         else state->shader.two_side = false;

         state->changed_light = false;
      }
         
      //TODO: better handling of matrix stacks
      //optimisation: moved here from install_uniforms to avoid repeated calculation
      modelview = state->modelview.body[state->modelview.pos];
      
      if (state->caps_fragment.fog || state->shader.lighting || state->batch.primitive_mode == 0)
         memcpy(state->current_modelview, modelview, 16*4);
         
      for (i = 0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++) {
         GL11_TEXUNIT_T *texunit = &state->texunits[i];
         if (state->shader.texunits[i].props.active) {
            memcpy(texunit->current_matrix,texunit->stack.body[texunit->stack.pos],16*4);
         }
      }
      gl11_matrix_mult(state->projection_modelview, state->projection.body[state->projection.pos], modelview);
      gl11_matrix_invert_3x3(state->modelview_inv, modelview);
      
      
      if (state->caps.clip_plane[0])
      {
         GLfloat inv[16];
         GLfloat blah[16];

         gl11_matrix_invert_4x4(inv, state->projection.body[state->projection.pos]);
         gl11_matrix_mult(blah, inv, state->projection.body[state->projection.pos]);
         gl11_matrix_mult_row(state->projected_clip_plane, state->planes[0], inv);
         state->projected_clip_plane[0] /= state->viewport.internal[0];    /* xscale */
         state->projected_clip_plane[1] /= state->viewport.internal[1];    /* yscale */
         state->projected_clip_plane[2] /= (16777215.0f * state->viewport.internal[4]);    /* zscale */
      }   
   }
   else
   {
      vcos_assert(!state->changed_misc);
      vcos_assert(!state->changed_light);
      /*
         Can set changed_texunit accidentally because of has_color/has_alpha stuff
         (more specifically, we don't check for IS_ES_11 in glTexImage2D)
      */
   }
}

bool glxx_hw_get_attr_live(GLXX_SERVER_STATE_T *state, GLenum primitive_mode, GLXX_ATTRIB_T *attrib)
{
   /* TODO: This function ignores it's parameter 'mode' */
   if (IS_GL_11(state))
   {
      if (state->changed_texunit)
      {
         if (!gl11_hw_get_texunit_properties(state, &attrib[GL11_IX_TEXTURE_COORD], 0  /*TODO: points*/))
            return false;        // out of memory during blob construction
         state->changed_texunit = false;
      }
      gl11_hw_setup_attribs_live(state,&state->batch.cattribs_live,&state->batch.vattribs_live);
   }
   else
   {
      if (state->mh_program != MEM_INVALID_HANDLE)
      {
         GL20_PROGRAM_T *program = (GL20_PROGRAM_T *)mem_lock(state->mh_program);
         state->batch.cattribs_live = program->result.cattribs_live;
         state->batch.vattribs_live = program->result.vattribs_live;
         mem_unlock(state->mh_program);
      }
   }
   return true;
}

/* insert clip instruction. is_empty is set to true if the region is empty - i.e. nothing to draw. 
   returns false if failed a memory alloc
*/
static bool do_changed_cfg(GLXX_SERVER_STATE_T *state, GLXX_HW_FRAMEBUFFER_T *fb, uint32_t color_varyings, bool * is_empty)
{
   uint32_t flat_shading_flags;
   uint8_t * instr;
   *is_empty = false;
   
   if (state->changed_viewport)
   {
      int x, y, xmax, ymax;
      x = 0;
      y = 0;
      xmax = fb->width;
      ymax = fb->height;
   
      x = _max(x, state->viewport.x);
      y = _max(y, state->viewport.y);
      xmax = _min(xmax, state->viewport.x + state->viewport.width);
      ymax = _min(ymax, state->viewport.y + state->viewport.height);
   
      if (state->caps.scissor_test)
      {
         x = _max(x, state->scissor.x);
         y = _max(y, state->scissor.y);
         xmax = _min(xmax, state->scissor.x + state->scissor.width);
         ymax = _min(ymax, state->scissor.y + state->scissor.height);
      }
   
      if (x >= xmax || y >= ymax)
      {
         *is_empty = true;
         goto done;   /* empty region - nothing to draw */
      }
   
      instr = glxx_big_mem_alloc_cle(32);
      if (!instr) goto fail;
   
      Add_byte(&instr, KHRN_HW_INSTR_STATE_CLIP);     //(9)
      add_short(&instr, x);
#ifdef BCG_FB_LAYOUT
      if (state->shader.common.rso_format)
         add_short(&instr, fb->height - ymax);
      else
#endif /* BCG_FB_LAYOUT */
      add_short(&instr, y);
      add_short(&instr, xmax - x);
      add_short(&instr, ymax - y);
   
      Add_byte(&instr, KHRN_HW_INSTR_STATE_CLIPPER_XY);   //(9)
      add_float(&instr, 8.0f * (float)state->viewport.width);
#ifdef BCG_FB_LAYOUT
      if (state->shader.common.rso_format)
         add_float(&instr, -8.0f * (float)state->viewport.height);
      else
#endif /* BCG_FB_LAYOUT */
      add_float(&instr, 8.0f * (float)state->viewport.height);
   
      Add_byte(&instr, KHRN_HW_INSTR_STATE_VIEWPORT_OFFSET);  //(5)
      add_short(&instr, 8 * (2*state->viewport.x + state->viewport.width));
#ifdef BCG_FB_LAYOUT
      if (state->shader.common.rso_format)
         add_short(&instr, 8 * (2*fb->height - 2*state->viewport.y - state->viewport.height));
      else
#endif /* BCG_FB_LAYOUT */
      add_short(&instr, 8 * (2*state->viewport.y + state->viewport.height));
   
      Add_byte(&instr, KHRN_HW_INSTR_STATE_CLIPPER_Z);    //(9)
      add_float(&instr, 0.5f * (state->viewport.far + state->viewport.near));
      add_float(&instr, 0.5f * (state->viewport.far - state->viewport.near));
      
      state->changed_viewport = false;
   }

   if (state->changed_cfg)
   {
      uint32_t enfwd, enrev, cwise, endo, rasosm, zfunc, enzu, enez;

      enfwd = !state->caps.cull_face || glxx_enable_front(state->cull_mode);
      enrev = !state->caps.cull_face || glxx_enable_back(state->cull_mode);
      cwise = glxx_front_facing_is_clockwise(state->front_face);
#ifdef BCG_FB_LAYOUT
      if (state->shader.common.rso_format)
         cwise = (!cwise) & 0x1;
#endif /* BCG_FB_LAYOUT */
      endo = state->caps.polygon_offset_fill;
      rasosm = state->caps.multisample && fb->ms;
      zfunc = (fb->have_depth && state->caps.depth_test) ? glxx_hw_convert_test_function(state->depth_func) : 7;
      enzu = fb->have_depth && state->depth_mask && state->caps.depth_test;
#ifdef __BCM2708A0__
      enez = 0;
#else
   /* Only safe to do early z if zfunc is less or lequal AND
      There are no side effects due to stencil funcs i.e. zfails */
      enez = (zfunc == 1 || zfunc == 3) &&
                !(state->caps.stencil_test && state->stencil_op.front.zfail != GL_KEEP) &&
                !(state->caps.stencil_test && state->stencil_op.back.zfail  != GL_KEEP);
#endif
   
#ifdef WORKAROUND_HW2905
      {
         GLXX_HW_RENDER_STATE_T *rs = (GLXX_HW_RENDER_STATE_T *)khrn_render_state_get_data(state->current_render_state);
         if (fb->ms && rs->depth_load || rs->stencil_load)
            enez = 0;
      }
#endif
   
      instr = glxx_big_mem_alloc_cle(4);
      if (!instr) goto fail;
   
      Add_byte(&instr, KHRN_HW_INSTR_STATE_CFG);     //(4)
      add_byte(&instr, enfwd | enrev << 1 | cwise << 2 | endo << 3 | rasosm << 6);
      add_byte(&instr, zfunc << 4 | enzu << 7);
      add_byte(&instr, enez | 1<<1);

      state->changed_cfg = false;
   }

   if (state->changed_polygon_offset)
   {
      instr = glxx_big_mem_alloc_cle(5);
      if (!instr) goto fail;

      Add_byte(&instr, KHRN_HW_INSTR_STATE_DEPTH_OFFSET);    //(5)
      add_short(&instr, float_to_bits(state->polygon_offset.factor) >> 16);
      add_short(&instr, float_to_bits(state->polygon_offset.units) >> 16);
      state->changed_polygon_offset = false;
   }

   if (state->changed_linewidth)
   {
      instr = glxx_big_mem_alloc_cle(5);
      if (!instr) goto fail;

      Add_byte(&instr,KHRN_HW_INSTR_STATE_LINE_WIDTH);   //(5)
      add_float(&instr, state->line_width);
      state->changed_linewidth = false;
   }

   flat_shading_flags = (IS_GL_11(state) && (state->shade_model == GL_FLAT)) ? color_varyings : 0;
   if (state->old_flat_shading_flags == ~0 || state->old_flat_shading_flags != flat_shading_flags)
   {
      instr = glxx_big_mem_alloc_cle(5);
      if (!instr) goto fail;

      Add_byte(&instr, KHRN_HW_INSTR_STATE_FLATSHADE);       //(5)
      add_word(&instr, flat_shading_flags);
      state->old_flat_shading_flags = flat_shading_flags;
   }


done:
   return true;
fail:
   return false;      
}

///////////
extern uint32_t xxx_shader;

/*!
 * \brief Processes a batch of primitives and bins them.
 *
 * This function is called for each batch of primitives in the scene. It first performs
 * per-batch tasks like setting up the shaders and uniforms and then runs the binning
 * pipeline, binning the primitives into the according tile lists.
 *
 * \param mode    is the GLenum mode value provided by OpenGL
 * \param count   is the number of
 * \param type    is a GL type indicator.
 * \param indices_offset is ??? (TODO: Document)
 * \param state   is the OpenGL server state.
 * \param attrib  is ??? (TODO: Document)
 * \param indices_handle  is ??? (TODO: Document)
 * \param attrib_handles  is ??? (TODO: Document)
 * \param max_index is ??? (TODO: Document)
 */
bool glxx_hw_draw_triangles(
   GLsizei count,
   GLenum type,
   uint32_t indices_offset,
   GLXX_SERVER_STATE_T *state,
   GLXX_ATTRIB_T *attrib,
   MEM_HANDLE_T indices_handle,
   MEM_HANDLE_T *attrib_handles,
   uint32_t max_index,
   MEM_HANDLE_OFFSET_T *interlocks,
   uint32_t interlock_count)
{
   uint32_t i, j;
   GLXX_HW_FRAMEBUFFER_T fb;
   MEM_HANDLE_T cunif_map, vunif_map, funif_map;
   uint32_t cattribs_live;
   uint32_t vattribs_live;

   uint32_t cunif_count;
   uint32_t vunif_count;
   uint32_t funif_count;
   uint32_t mergeable_attribs[GLXX_CONFIG_MAX_VERTEX_ATTRIBS];
   uint32_t vattribs_order[GLXX_CONFIG_MAX_VERTEX_ATTRIBS*2];
   uint32_t cattribs_order[GLXX_CONFIG_MAX_VERTEX_ATTRIBS*2];
   uint32_t num_vpm_rows_c[4];
   uint32_t num_vpm_rows_v[4];
   GLXX_HW_RENDER_STATE_T *rs;
   GLXX_HW_SHADER_RECORD_T *shader_record;
   uint32_t attr_count;
   uint8_t *instr;

   //GL 1.1 specific
   uint32_t color_varyings;

   //gl 2.0 specific
   GL20_PROGRAM_T *program = NULL;
   GL20_HW_INDEXED_UNIFORM_T iu;
   bool locked_program_items = false;

   if(IS_GL_11(state)) {
      if (count == 0)
         return true;
   } else {
       program = (GL20_PROGRAM_T *)mem_lock(state->mh_program);
       if(!program->linked)
          goto done;
   }

   rs = glxx_install_framebuffer(state, &fb, false);
   if (!rs)
      goto done;

   if(!glxx_lock_fixer_stuff(rs))
      goto fail2;

   if(!IS_GL_11(state)) {
      state->batch.sampler_info = (GL20_SAMPLER_INFO_T *)mem_lock(program->mh_sampler_info);
      state->batch.uniform_info = (GL20_UNIFORM_INFO_T *)mem_lock(program->mh_uniform_info);
      state->batch.uniform_data = (uint32_t *)mem_lock(program->mh_uniform_data);
      state->batch.num_samplers = mem_get_size(program->mh_sampler_info) / sizeof(GL20_SAMPLER_INFO_T);
      locked_program_items = true;
   }

   calculate_and_hide(state, &fb, attrib);

   for (i = 0; i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++) mergeable_attribs[i] = (uint32_t)~0;

   for (i = 0; i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++)
   {
      if (attrib_handles[i] != MEM_INVALID_HANDLE) {
         //look for potentially mergable attributes (packed in same buffer)
         for (j = 0; j < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; j++)
         {
            if(mergeable_attribs[i] == (uint32_t)~0 //not already found a potential merge 
               && attrib_handles[i] == attrib_handles[j]) {
               int size = attrib[i].size*khrn_get_type_size(attrib[i].type);
               if((uint32_t)attrib[i].pointer + size == (uint32_t)attrib[j].pointer
                  && attrib[i].stride == attrib[j].stride
                  && (size & 3) == 0)
               {
                  //can merge such that j follows i
                  mergeable_attribs[i] = j;
               }
            }
         }
      }
   }

#ifdef DRAW_TEX_LOGGING
   printf("state_name: %d rs: %d draw_triangles: count %d type %d\n",
      state->name,rs->name,count,type);
   printf("--------------------\n");
#endif

if ((state->shader_record_addr != NULL) &&(state->pres_mode == state->batch.primitive_mode) &&(type !=0) ) {

		  instr = glxx_big_mem_alloc_cle(20);
		  if (!instr) goto fail;
		  
		  // Emit a GLDRAWELEMENTS instruction
		  Add_byte(&instr, KHRN_HW_INSTR_GLDRAWELEMENTS);	//(14)
		  add_byte(&instr,
			 convert_primitive_type(state->batch.primitive_mode) |	 //Primitive mode
			 convert_index_type(type) << 4);  //Index type
	
		  add_word(&instr, count);		  //Length (number of indices)
		  if (!glxx_big_mem_add_fix(&instr, indices_handle, indices_offset))
			 goto fail;
#ifdef SIMPENROSE_RECORD_OUTPUT
		  record_map_mem_buffer(indices_handle, L_INDICES, RECORD_BUFFER_IS_BOTH, mem_get_align(indices_handle));
#endif
		  add_word(&instr, _min(max_index, 0xffff));	//Maximum index (primitives using a greater index will cause error)
	
	Add_byte(&instr, KHRN_HW_INSTR_NOP);		//(1) TODO: is this necessary?
	Add_byte(&instr, KHRN_HW_INSTR_NOP);		//(1) TODO: is this necessary?
	Add_byte(&instr, KHRN_HW_INSTR_NOP);		//(1) TODO: is this necessary?
	Add_byte(&instr, KHRN_HW_INSTR_NOP);		//(1) TODO: is this necessary?
	Add_byte(&instr, KHRN_HW_INSTR_NOP);		//(1) TODO: is this necessary?
	Add_byte(&instr, KHRN_HW_INSTR_NOP);		//(1) TODO: is this necessary?
	   goto skip_install;
}
   /* TODO: only allocate space for as many vertex attributes as we need */
   /* TODO: extended vertex stride? */
   shader_record = (GLXX_HW_SHADER_RECORD_T *)glxx_big_mem_alloc_junk(100, 16);
   if (!shader_record) goto fail;

   /* create or retrieve shaders from cache and setup attribs_live */

   cattribs_live = state->batch.cattribs_live;
   vattribs_live = state->batch.vattribs_live;
   if(!vcos_verify(get_shaders(
         program,
         shader_record,
         &cunif_map, &vunif_map, &funif_map,
         &color_varyings,
         state,
         attrib,
         mergeable_attribs,
         cattribs_order,
         vattribs_order)))
   {
      glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
      goto fail;
   }

   /* Create VCD configuration */

   do_vcd_setup(shader_record, attrib, attrib_handles, cattribs_live, vattribs_live, mergeable_attribs, cattribs_order, vattribs_order,
      num_vpm_rows_c, num_vpm_rows_v, &attr_count);
   vcos_assert(attr_count >= 1 && attr_count <= 8);

   if(!IS_GL_11(state)) {
#ifdef XXX_OFFLINE
      if (shader.use_offline)
      {
         /* Allocate space for shaded vertices */
         //GLXX_FIXABLE_BUF_T buf;
         vcd_setup.shaded_vertex_size = 28 + 4 * shader.num_varyings;
         vcd_setup.shaded_vertex_count = (max_index + 15) & ~15;
         //if (!glxx_fixable_alloc(&buf, &vcd_setup.shaded_vertices, vcd_setup.shaded_vertex_size * vcd_setup.shaded_vertex_count, 4, L_VERTEX_DATA))
         //    goto fail;
         //glxx_fixable_close(&buf);
         if(!glxx_alloc_junk_mem(&vcd_setup.shaded_vertices,vcd_setup.shaded_vertex_size * vcd_setup.shaded_vertex_count,4))
            goto fail;
#ifdef SIMPENROSE_RECORD_OUTPUT
         record_map_mem_buffer_section(vcd_setup.shaded_vertices.handle, vcd_setup.shaded_vertices.offset, vcd_setup.shaded_vertex_size * vcd_setup.shaded_vertex_count, L_VERTEX_DATA, RECORD_BUFFER_IS_BOTH, 4);
#endif
      }
      else
         vcd_setup.shaded_vertices = glxx_fixable_null();
#endif // XXX_OFFLINE

      gl20_hw_iu_init(&iu);
   }

   /* Install uniforms */
   cunif_count = mem_get_size(cunif_map)/8;
   vunif_count = mem_get_size(vunif_map)/8;
   funif_count = mem_get_size(funif_map)/8;

#ifdef XXX_OFFLINE
   if (!shader.use_offline)
#endif
   {
      if(!install_uniforms(
         &shader_record->cunif,
         state,
         program,
         cunif_count,
         cunif_map,
         &iu,
         num_vpm_rows_c,
         attrib,/*TODO for GL 2.0 NULL is passed instead of attrib - does this matter? */
         state->shader.common.rso_format,
         fb.height
         ))
      {
         goto fail;
      }
   }
   if(!install_uniforms(
      &shader_record->vunif,
      state,
      program,
      vunif_count,
      vunif_map,
      &iu,
      num_vpm_rows_v,
      attrib,
      state->shader.common.rso_format,
      fb.height
      ))
   {
      goto fail;
   }
   if(!install_uniforms(
      &shader_record->funif,
      state,
      program,
      funif_count,
      funif_map,
      &iu,
      0,
      attrib,
      state->shader.common.rso_format,
      fb.height
      ))
   {
      goto fail;
   }

   
   if(!IS_GL_11(state)) {
      mem_unlock(program->mh_sampler_info);
      mem_unlock(program->mh_uniform_info);
      mem_unlock(program->mh_uniform_data);
      locked_program_items = false;
   }

   if(!IS_GL_11(state)) {
      gl20_hw_iu_close(&iu);
   }

#ifdef DUMP_SHADER
   dump_shader(&shader.cshader, &shader.cunif);
#endif
   
   /* emit any necessary config change instructions */
   {
      bool is_empty,ok;
      ok = do_changed_cfg(state,&fb,color_varyings,&is_empty);
      if(!ok) goto fail;
      if(is_empty) 
      {
         glxx_unlock_fixer_stuff();
         goto done;   /* empty region - nothing to draw */         
      }
   }   

#ifdef SIMPENROSE_RECORD_OUTPUT
   for (i=0; i<GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++) {
      /* TODO: This doesn't record the dummy attrib that can be created. Problematic? */
      if (attrib_handles[i] != MEM_HANDLE_INVALID)
         record_map_mem_buffer(attrib_handles[i], L_ATTRIBUTES, RECORD_BUFFER_IS_BOTH, 1);
   }
   record_map_buffer(khrn_hw_addr(shader_record), 44 + 8*(attr_count-1), L_GL_SHADER_RECORD, RECORD_BUFFER_IS_BOTH, 16);
#endif

   if (type == 0)
   {
      uint32_t offset = 0;
      while(count>0)
      {         
         uint32_t step;
         uint32_t batch_count;
         uint32_t batch_indices_offset = indices_offset;
        
         /* SW-5891 hardware can only do 65536 vertices at a time */
         if(indices_offset>0 && indices_offset+count>USHRT_MAX)
         {
            /* current shader record is no use, (indices_offset+batch_count)*stride > 16bit, so batch_count = 0 */
            /* set up correct attrib pointer in next shade record */
            batch_indices_offset = batch_count = step = 0;
         }
         else if(count>USHRT_MAX)
         {
            /* count * stride > 16 bit so do no more than ushrt_max in this batch */
            /* for lines, triangle strips etc. will need 1 or more of the previous vertices so step < ushrt_max */
            /* TODO for line loops and triangle fans would have to copy vertex data so can insert first vertex at beginning of new chunk */
            switch(state->batch.primitive_mode)
            {
               case GL_POINTS:
                  step = batch_count = USHRT_MAX;
                  break;
               case GL_LINES:
                  step = batch_count = USHRT_MAX - (USHRT_MAX % 2);
                  break;
               case GL_LINE_LOOP:
                  goto fail;/* not implemented as require expensive alloc and copy */
               case GL_LINE_STRIP:
                  batch_count = USHRT_MAX;
                  step = USHRT_MAX - 1;
                  break;
               case GL_TRIANGLES:
                  step = batch_count = USHRT_MAX - (USHRT_MAX % 3);
                  break;
               case GL_TRIANGLE_STRIP:
                  batch_count = USHRT_MAX;
                  step = USHRT_MAX - 2;
                  break;
               case GL_TRIANGLE_FAN:
                  goto fail;/* not implemented as require expensive alloc and copy */
               default:
                  UNREACHABLE();
                  return 0;
            }
         }
         else
         {
            step = batch_count = count;
         }
        
         instr = glxx_big_mem_alloc_cle(20);
         if (!instr) goto fail;
         
         Add_byte(&instr, KHRN_HW_INSTR_GL_SHADER);     //(5)
         add_pointer(&instr, (uint8_t *)shader_record + (attr_count & 7));
         
		 state->shader_record_addr = 0;
		 state->pres_mode = 0;
         // Emit a GLDRAWARRAYS instruction
         Add_byte(&instr, KHRN_HW_INSTR_GLDRAWARRAYS);
         add_byte(&instr, convert_primitive_type(state->batch.primitive_mode));  //Primitive mode
         add_word(&instr, batch_count);       //Length (number of vertices)
         add_word(&instr, batch_indices_offset);            //Index of first vertex
   
         Add_byte(&instr, KHRN_HW_INSTR_NOP);         //Pad to the same length as KHRN_HW_INSTR_GLDRAWELEMENTS to make it easier for ourselves
         Add_byte(&instr, KHRN_HW_INSTR_NOP);
         Add_byte(&instr, KHRN_HW_INSTR_NOP);
         Add_byte(&instr, KHRN_HW_INSTR_NOP);
         
         Add_byte(&instr, KHRN_HW_INSTR_NOP);        //(1) TODO: is this necessary?

         vcos_assert(step <= (uint32_t)count);
         
         count = count - step;

         if(count > 0)
         {
            GLXX_HW_SHADER_RECORD_T *new_shader_record;
            /* some vertices were not drawn */
            /* create a new shader record with offset pointers for another GLDRAWARRAYS instruction */
            new_shader_record = (GLXX_HW_SHADER_RECORD_T *)glxx_big_mem_alloc_junk(100, 16);
            if (!new_shader_record) goto fail;
            memcpy(new_shader_record,shader_record,sizeof(GLXX_HW_SHADER_RECORD_T));
            shader_record = new_shader_record;
            /* get_shaders() put copies of the handles we need in shader_record->fshader etc. */
            glxx_big_mem_insert(&shader_record->fshader,shader_record->fshader,0);
            glxx_big_mem_insert(&shader_record->vshader,shader_record->vshader,0);
#ifdef XXX_OFFLINE
         if (shader_record->cshader!=MEM_INVALID_HANDLE)
#endif
            glxx_big_mem_insert(&shader_record->cshader,shader_record->cshader,0);


            offset += step + indices_offset;
            indices_offset = 0;
            /* advance attribute pointers */
            for(j=0; j < attr_count; j++)
            {
               i = shader_record->attr[j].base;
               vcos_assert(i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS);/* stored i here in do_vcd_setup */
               glxx_big_mem_insert(&shader_record->attr[j].base, attrib_handles[i], (uint32_t)attrib[i].pointer + offset*shader_record->attr[j].stride);
               vcos_assert(i == shader_record->attr[j].base);/* check big_mem_insert didn't change this */
            }
         }
         else
         {
            count = 0;
         }
      }
   }
   else
   {
      instr = glxx_big_mem_alloc_cle(20);
      if (!instr) goto fail;
      
      Add_byte(&instr, KHRN_HW_INSTR_GL_SHADER);     //(5)
      add_pointer(&instr, (uint8_t *)shader_record + (attr_count & 7));
	  state->shader_record_addr = (uint32_t *)((uint8_t *)shader_record + (attr_count & 7));
	  state->pres_mode = state->batch.primitive_mode;

      // Emit a GLDRAWELEMENTS instruction
      Add_byte(&instr, KHRN_HW_INSTR_GLDRAWELEMENTS);   //(14)
      add_byte(&instr,
         convert_primitive_type(state->batch.primitive_mode) |   //Primitive mode
         convert_index_type(type) << 4);  //Index type

      add_word(&instr, count);        //Length (number of indices)
      if (!glxx_big_mem_add_fix(&instr, indices_handle, indices_offset))
         goto fail;
#ifdef SIMPENROSE_RECORD_OUTPUT
      record_map_mem_buffer(indices_handle, L_INDICES, RECORD_BUFFER_IS_BOTH, mem_get_align(indices_handle));
#endif
      add_word(&instr, _min(max_index, 0xffff));    //Maximum index (primitives using a greater index will cause error)

/*#ifdef SIMPENROSE_RECORD_BINNING
      vcos_assert(type == GL_UNSIGNED_SHORT);
      record_map_buffer(indices, 2 * count, L_INDICES, RECORD_BUFFER_IS_BOTH, 2);
#endif*/
   Add_byte(&instr, KHRN_HW_INSTR_NOP);        //(1) TODO: is this necessary?
   }


#ifdef SIMPENROSE_WRITE_LOG
   // Increment batch count
   log_batchcount++;
#endif
skip_install:
   /* Mark the state as drawn so that clearing works properly */
   rs->drawn = true;
   rs->xxx_empty = false;

   //now add references to vertex attribute cache interlocks to fixer
   for (i = 0; i < interlock_count; i++)
   {
      KHRN_INTERLOCK_T *lock = (KHRN_INTERLOCK_T *)((char *)mem_lock(interlocks[i].mh_handle) + interlocks[i].offset);
      khrn_interlock_read(lock, khrn_interlock_user(rs->name));
      mem_unlock(interlocks[i].mh_handle);
      glxx_hw_insert_interlock(interlocks[i].mh_handle, interlocks[i].offset);
      //fixer should then mark all the interlocks as in use and when the next flush occurs transfer them to the hardware
      //client is then able to wait until hardware has finished before deleting a cache entry
   }

   glxx_unlock_fixer_stuff();

done:
   
   if(!IS_GL_11(state)) {
      mem_unlock(state->mh_program);
   }
   
   return true;

fail:
   glxx_unlock_fixer_stuff();

fail2:
   if(!IS_GL_11(state)) {
      mem_unlock(state->mh_program);
      if(locked_program_items)
      {
         mem_unlock(program->mh_sampler_info);
         mem_unlock(program->mh_uniform_info);
         mem_unlock(program->mh_uniform_data);
         locked_program_items = false;
      }
   }
   
   glxx_hw_discard_frame(rs);

   return false;
}

/*************************************************************
 Static Functions
 *************************************************************/

#ifdef DUMP_SHADER
static void dump_shader(GLXX_FIXABLE_ADDR_T *fxshader, GLXX_FIXABLE_ADDR_T *fxunif)
{
   void *start, *end, *unif;
   char buffer[100];
   uint32_t *pos;
   uint32_t state = 0;

   start = (char *)mem_lock(fxshader->handle) + fxshader->offset;
   end = (char *)start + mem_get_size(fxshader->handle);
   unif = (char *)mem_lock(fxunif->handle) + fxunif->offset;

   pos = (uint32_t *)start;
   while (pos != end)
   {
      unif = glsl_qdisasm_with_uniform(&state, buffer, sizeof(buffer), pos[0], pos[1], unif, false);
      printf("%s\n", buffer);
      pos += 2;
   }
   printf("\n");

   mem_unlock(fxshader->handle);
   mem_unlock(fxunif->handle);
}
#endif

static bool get_shaders(
    GL20_PROGRAM_T *program,
    GLXX_HW_SHADER_RECORD_T *shader_out,
    MEM_HANDLE_T *cunifmap_out,
    MEM_HANDLE_T *vunifmap_out,
    MEM_HANDLE_T *funifmap_out,
    uint32_t *color_varyings_out,
    GLXX_SERVER_STATE_T *state,
    GLXX_ATTRIB_T *attrib,
    uint32_t *mergeable_attribs,
    uint32_t *cattribs_order_out,
    uint32_t *vattribs_order_out)
{
   if(IS_GL_11(state)) {
      return gl11_hw_get_shaders(
         shader_out,
         cunifmap_out,
         vunifmap_out,
         funifmap_out,
         color_varyings_out,
         state,
         attrib,
         mergeable_attribs,
         cattribs_order_out,
         vattribs_order_out);

   }else {
      bool ok = false;
      xxx_shader = program->name;

      ok = gl20_link_result_get_shaders(
         &program->result,
         shader_out,
         cunifmap_out,
         vunifmap_out,
         funifmap_out,
         state,
         attrib,
         mergeable_attribs,
         cattribs_order_out,
         vattribs_order_out);
      return ok;
   }
}

void glxx_hw_cache_use(GLXX_SERVER_STATE_T *state, int count, int *keys)
{
   //GLXX_HW_MSG_CACHE_RELEASE_T *msg = glxx_hw_get_next_cache_msg();

   //msg->mh_cache = state->mh_cache;
   //msg->count = count;

   //uint8_t *cache = (uint8_t *)mem_lock(state->mh_cache);

   //for (int i = 0; i < count; i++) {
   //   ((KHRN_CACHE_COUNTERS_T *)(cache + keys[i]))->acquire++;

   //   msg->offset[i] = keys[i];
   //}

   //mem_unlock(state->mh_cache);
   //mem_unlock(msg->handle);
   UNUSED(state);
   UNUSED(count);
   UNUSED(keys);
}

static bool install_uniform(uint32_t *ptr, uint32_t u0, uint32_t u1,
   GLXX_SERVER_STATE_T *state,
   GL20_HW_INDEXED_UNIFORM_T *iu,
   uint32_t * num_vpm_rows,
   GLXX_ATTRIB_T *attrib,
   bool rso_format,
   unsigned int fb_height)
{
#ifdef SIMPENROSE_RECORD_OUTPUT
   LABEL_T label = L_UNIFORMS;
#endif

   switch (u0)
   {
   case BACKEND_UNIFORM:
   {
      if(u1 & (GLXX_ATTRIB_MASK - 0x80000000)) //u1 is an offset into the server state struct
      {
         float *x  = GLXX_READ_ATTRIB_OFFSET(attrib, u1);
         *ptr = *(uint32_t *)x;
      }
      else// gl 1.1 and 2.0 specials
      {
         uint32_t u = u1 & ~3;
         uint32_t j = u1 % 4;
         //uint32_t k;
         switch (u)
         {
         //common 1.1 and 2.0 cases
         case BACKEND_UNIFORM_VPM_READ_SETUP:
            {
               uint8_t data_start;
               vcos_assert(j <= 3 && num_vpm_rows[j] <= 15);
               data_start = ( j>0 ? num_vpm_rows[j-1] : 0) & 0x3F;
#ifdef XXX_OFFLINE
               if(IS_GL_11(state) || vcd_setup->shaded_vertices.handle == MEM_INVALID_HANDLE)
                  *ptr = 0x00001a00 | (num_vpm_rows[j] & 15) << 20;
               else
                  *ptr = 0x00001200 | (num_vpm_rows[j] & 15) << 20;   /* do vertical reads in user shader mode */
#else
               *ptr = 0x00001a00 | ((num_vpm_rows[j] & 15) << 20) | data_start;
#endif
            }
            break;
         case BACKEND_UNIFORM_VPM_WRITE_SETUP:
#ifdef XXX_OFFLINE
            if(IS_GL_11(state) || vcd_setup->shaded_vertices.handle == MEM_INVALID_HANDLE)
               *ptr = 0x00001a00;
            else
               *ptr = 0x00001a10;
#else
            *ptr = 0x00001a00;
#endif
            break;
         case BACKEND_UNIFORM_STENCIL_FRONT:
            *ptr =
               ((state->stencil_func.front.mask > 0xff) ? 0xff : state->stencil_func.front.mask) |
               ((state->stencil_func.front.ref > 0xff) ? 0xff : state->stencil_func.front.ref) << 8 |
               glxx_hw_convert_test_function(state->stencil_func.front.func) << 16 |
               glxx_hw_convert_operation(state->stencil_op.front.fail) << 19 |
               glxx_hw_convert_operation(state->stencil_op.front.zpass) << 22 |
               glxx_hw_convert_operation(state->stencil_op.front.zfail) << 25 |
               1 << 30;
            break;
         case BACKEND_UNIFORM_STENCIL_BACK:
            *ptr =
               ((state->stencil_func.back.mask > 0xff) ? 0xff : state->stencil_func.back.mask) |
               ((state->stencil_func.back.ref > 0xff) ? 0xff : state->stencil_func.back.ref) << 8 |
               glxx_hw_convert_test_function(state->stencil_func.back.func) << 16 |
               glxx_hw_convert_operation(state->stencil_op.back.fail) << 19 |
               glxx_hw_convert_operation(state->stencil_op.back.zpass) << 22 |
               glxx_hw_convert_operation(state->stencil_op.back.zfail) << 25 |
               2 << 30;
            break;
         case BACKEND_UNIFORM_STENCIL_MASK:
            *ptr =
               (state->stencil_mask.front & 0xff) |
               (state->stencil_mask.back & 0xff) << 8 |
               0 << 30;
            break;
         //2.0 specific cases
         case BACKEND_UNIFORM_BLEND_COLOR:
            vcos_assert(!IS_GL_11(state));
            *ptr = color_floats_to_rgba(state->blend_color[0], state->blend_color[1], state->blend_color[2], state->blend_color[3]);
            break;
#ifdef XXX_OFFLINE
         case BACKEND_UNIFORM_NEXT_USER_SHADER:
            {
               KHRN_FIXER_TYPE_T fixer_type = 0;
               GLXX_FIXABLE_ADDR_T addr = glxx_hw_get_next_user_shader();

               if(addr.handle == MEM_INVALID_HANDLE)
                  goto fail;

               vcos_assert(!IS_GL_11(state));

               //glxx_add_addr(buf, &addr);

               vcos_assert(addr.fixable_type == FIXABLE_TYPE_JUNK || addr.fixable_type == FIXABLE_TYPE_ALLOC);

               if(addr.fixable_type == FIXABLE_TYPE_JUNK)
                  fixer_type = 0;
               else if (addr.fixable_type == FIXABLE_TYPE_ALLOC)
                  fixer_type = (KHRN_FIXER_TYPE_T)(KHRN_FIXER_TYPE_NO_HISTORY|KHRN_FIXER_TYPE_REF);

               if(!glxx_fixer_locked_add_handle(buf, addr.handle, fixer_type))
                  goto fail;
               word = addr.offset;
            }
            break;
         case BACKEND_UNIFORM_NEXT_USER_UNIF:
            {
               KHRN_FIXER_TYPE_T fixer_type = 0;
               GLXX_FIXABLE_ADDR_T addr = glxx_hw_get_next_user_unif();
               vcos_assert(!IS_GL_11(state));
               //glxx_add_valid_addr(buf, &addr);   /* Use "add_valid" to guard against null */

               if (addr.fixable_type == FIXABLE_TYPE_NULL)
               {
                  word = 0;
               }
               else
               {
                  vcos_assert(addr.fixable_type == FIXABLE_TYPE_JUNK || addr.fixable_type == FIXABLE_TYPE_ALLOC);

                  if(addr.fixable_type == FIXABLE_TYPE_JUNK)
                     fixer_type = 0;
                  else if (addr.fixable_type == FIXABLE_TYPE_ALLOC)
                     fixer_type = (KHRN_FIXER_TYPE_T)(KHRN_FIXER_TYPE_NO_HISTORY|KHRN_FIXER_TYPE_REF);
                  word = addr.offset;
                  if(!glxx_fixer_locked_add_handle(buf, addr.handle, fixer_type))
                     goto fail;
               }
            }
            break;
         case BACKEND_UNIFORM_VDR_SETUP0:
            vcos_assert(!IS_GL_11(state));
            vcos_assert(vcd_setup->attr_count == 1);
            vcos_assert(vcd_setup->sizem1[0]+1 < 64);

            word =
               0 << 0 |                               /* x */
               0 << 4 |                               /* y */
               0 << 11 |                              /* horizontal */
               1 << 12 |                              /* vpitch */
               0 << 16 |                              /* nrows (0=16) */
               ((vcd_setup->sizem1[0]+1)/4) << 20 | /* rowlen */
               0 << 24 |                              /* mpitch */
               0 << 28 |                              /* size is 32 bits, 0 offset */
               1 << 31;                              /* id */
            break;
         case BACKEND_UNIFORM_VDR_SETUP1:
            vcos_assert(!IS_GL_11(state));
            vcos_assert(vcd_setup->attr_count == 1);
            vcos_assert(vcd_setup->stride[0] < 8192);
            word =
               vcd_setup->stride[0] |    /* stride */
               9 << 28;                 /* id */
            break;
         case BACKEND_UNIFORM_VDR_ADDR_START:
            {
               //GLXX_FIXABLE_ADDR_T addr = glxx_fixable_copy(&vcd_setup->base[0]);

               GLXX_FIXABLE_ADDR_T addr = vcd_setup->base[0];

               vcos_assert(!IS_GL_11(state));
               vcos_assert(vcd_setup->attr_count == 1);

               vcos_assert(addr.fixable_type == FIXABLE_TYPE_NULL || addr.fixable_type == FIXABLE_TYPE_ALLOC);

               //glxx_add_addr(buf, &addr);
               if (addr.fixable_type == FIXABLE_TYPE_NULL)
               {
                  word = 0;
               }
               else
               {
                  KHRN_FIXER_TYPE_T fixer_type;
                  fixer_type = (KHRN_FIXER_TYPE_T)(KHRN_FIXER_TYPE_NO_HISTORY|KHRN_FIXER_TYPE_REF);

                  word  = addr.offset;
                  if(!glxx_fixer_locked_add_handle(buf, addr.handle, fixer_type))
                     goto fail;
               }
            }
            break;
         case BACKEND_UNIFORM_VDR_ADDR_INCR:
            vcos_assert(!IS_GL_11(state));
            vcos_assert(vcd_setup->attr_count == 1);
            word = 16 * vcd_setup->stride[0];
            break;
         case BACKEND_UNIFORM_VDR_ADDR_END:
            {
               //GLXX_FIXABLE_ADDR_T addr = glxx_fixable_copy(&vcd_setup->base[0]);
               GLXX_FIXABLE_ADDR_T addr = vcd_setup->base[0];
               vcos_assert(!IS_GL_11(state));
               vcos_assert(vcd_setup->attr_count == 1);
               vcos_assert(vcd_setup->shaded_vertex_count > 0);
               vcos_assert(!(vcd_setup->shaded_vertex_count & 15));
               //glxx_add_addr_plus(buf, &addr, (vcd_setup->shaded_vertex_count - 16) * vcd_setup->stride[0]);

               vcos_assert(addr.fixable_type == FIXABLE_TYPE_NULL || addr.fixable_type == FIXABLE_TYPE_ALLOC);

               if (addr.fixable_type == FIXABLE_TYPE_NULL)
               {
                  word = 0;
               }
               else
               {
                  KHRN_FIXER_TYPE_T fixer_type;
                  fixer_type = (KHRN_FIXER_TYPE_T)(KHRN_FIXER_TYPE_NO_HISTORY|KHRN_FIXER_TYPE_REF);

                  word  = addr.offset + (vcd_setup->shaded_vertex_count - 16) * vcd_setup->stride[0];
                  if(!glxx_fixer_locked_add_handle(buf, addr.handle, fixer_type))
                     goto fail;
               }
            }
            break;
         case BACKEND_UNIFORM_VDW_SETUP0:
            vcos_assert(!IS_GL_11(state));
            vcos_assert(vcd_setup->shaded_vertex_size < 512);
            word =
               0 << 0 |                                     /* size is 32 bits, 0 offset */
               0 << 3 |                                     /* x */
               16 << 7 |                                    /* y */
               0 << 14 |                                    /* vertical */
               (vcd_setup->shaded_vertex_size/4) << 16 |    /* size in bytes */
               16 << 23 |                                   /* number of units */
               2 << 30;                                    /* id */
            break;
         case BACKEND_UNIFORM_VDW_SETUP1:
            vcos_assert(!IS_GL_11(state));
            word =
               0 |            /* distance from last byte to first byte of next block */
               0 << 16 |      /* block mode */
               3 << 30;      /* id */
            break;
         case BACKEND_UNIFORM_VDW_ADDR_START:
            {
               //GLXX_FIXABLE_ADDR_T addr = glxx_fixable_copy(&vcd_setup->shaded_vertices);
               //vcos_assert(!IS_GL_11(state));
               //glxx_add_addr(buf, &addr);
               GLXX_FIXABLE_ADDR_T *addr = &vcd_setup->shaded_vertices;

               vcos_assert(!IS_GL_11(state));

               vcos_assert(addr->fixable_type == FIXABLE_TYPE_NULL || addr->fixable_type == FIXABLE_TYPE_ALLOC);

               //glxx_add_addr(buf, &addr);
               if (addr->fixable_type == FIXABLE_TYPE_NULL)
               {
                  word = 0;
               }
               else
               {
                  KHRN_FIXER_TYPE_T fixer_type;
                  fixer_type = (KHRN_FIXER_TYPE_T)(KHRN_FIXER_TYPE_NO_HISTORY|KHRN_FIXER_TYPE_REF);

                  word  = addr->offset;
                  if(!glxx_fixer_locked_add_handle(buf, addr->handle, fixer_type))
                     goto fail;
               }
            }
            break;
         case BACKEND_UNIFORM_VDW_ADDR_INCR:
            vcos_assert(!IS_GL_11(state));
            word = 16 * vcd_setup->shaded_vertex_size;
            break;
#endif //XXX_OFFLINE
         case BACKEND_UNIFORM_SCALE_Y:
            {
               float tmp;
#ifdef BCG_FB_LAYOUT
               tmp = ((rso_format) ? -16.0f : 16.0f) * state->viewport.internal[10];
#else
               tmp = 16.0f * state->viewport.internal[10];
#endif /* BCG_FB_LAYOUT */
               *ptr = float_to_bits(tmp);
            }
            break;
#ifdef BCG_FB_LAYOUT
         case BACKEND_UNIFORM_FBHEIGHT:
         {
            *ptr = float_to_bits((float)fb_height);
            break;
         }
         case BACKEND_UNIFORM_RSO_FORMAT:
            *ptr = rso_format;
            break;
#endif /* BCG_FB_LAYOUT */
         case BACKEND_UNIFORM_DEPTHRANGE_NEAR:
            vcos_assert(!IS_GL_11(state));
            *ptr = *(uint32_t *)&state->viewport.internal[6];
            break;
         case BACKEND_UNIFORM_DEPTHRANGE_FAR:
            vcos_assert(!IS_GL_11(state));
            *ptr = *(uint32_t *)&state->viewport.internal[7];
            break;
         case BACKEND_UNIFORM_DEPTHRANGE_DIFF:
            vcos_assert(!IS_GL_11(state));
            *ptr = *(uint32_t *)&state->viewport.internal[8];
            break;
         default:
            UNREACHABLE();
            goto fail;
         }
      }
      break;
   }
   case BACKEND_UNIFORM_TEX_CUBE_STRIDE:
      if(IS_GL_11(state)) {
         *ptr = 0;
      } else {
         if(!glxx_install_tex_param(state, ptr, u0, u1))
            goto fail;
      }
      break;
   case BACKEND_UNIFORM_TEX_NOT_USED:
      *ptr = 0;
      break;
   case BACKEND_UNIFORM_TEX_PARAM0:
   case BACKEND_UNIFORM_TEX_PARAM1:
#ifdef SIMPENROSE_RECORD_OUTPUT
      if (u0 == BACKEND_UNIFORM_TEX_PARAM0) label = L_TEX_PARAM0;
#endif
      if(!glxx_install_tex_param(state, ptr, u0, u1))
         goto fail;
      break;
#ifdef __BCM2708A0__
   case BACKEND_UNIFORM_TEX_UNIF|BACKEND_UNIFORM_TEX_PARAM0:
   {
//      count_platform_specific++;
      if(!backend_uniform_tex_param0(u1,state,iu, ptr))
         return false;
      break;
   }
   case BACKEND_UNIFORM_TEX_UNIF|BACKEND_UNIFORM_TEX_PARAM1:
   {
//      count_platform_specific++;
      if(!backend_uniform_tex_param1(u1,state,iu, ptr))
         return false;
      
      break;
   }
#endif
#ifndef __BCM2708A0__
   case BACKEND_UNIFORM_ADDRESS:
   {
//      count_platform_specific ++;
      if(!backend_uniform_address(u1, state, iu, ptr))
         goto fail;
#ifdef SIMPENROSE_RECORD_OUTPUT
      label = L_ADDRESS;
#endif
      
      break;
   }
#endif
   default:
      UNREACHABLE();
      goto fail;
   }

#ifdef SIMPENROSE_RECORD_OUTPUT
   record_map_buffer(khrn_hw_addr(ptr), 4, label, RECORD_BUFFER_IS_DATA, 4);
#endif

   return true;
fail:
   return false;
}

static bool install_uniforms(
   uint32_t *startaddr_location,
   GLXX_SERVER_STATE_T *state,
   GL20_PROGRAM_T *program,
   uint32_t count,
   MEM_HANDLE_T hmap,
   GL20_HW_INDEXED_UNIFORM_T *iu,
   uint32_t * num_vpm_rows,
   GLXX_ATTRIB_T *attrib,
   bool rso_format,
   unsigned int fb_height)
{
   //debugging 
   //uint32_t count_platform_specific = 0;
   uint32_t i;
   uint32_t *ptr;
   uint32_t *map;
   //gl 2.0 specific
   uint32_t max = 0;
   uint32_t texture_swizzle = 0;

#ifdef SIMPENROSE_RECORD_OUTPUT
   if (count == 0) {
      if (dummy_unifs == NULL) {
         dummy_unifs = rtos_malloc_priority(4, 4, RTOS_PRIORITY_DIRECT, "Dummy uniforms");
         if (!dummy_unifs) {
            return false;
         }
      }
      record_map_buffer(khrn_hw_addr(dummy_unifs), 4, L_UNIFORMS, RECORD_BUFFER_IS_BOTH, 4);
      startaddr_location = dummy_unifs;
      return true;
   }
#endif

   ptr = glxx_big_mem_alloc_junk(4 * count, 4);
   if (!ptr) return false;
   *startaddr_location = khrn_hw_addr(khrn_hw_alias_direct(ptr)); //PTR
#ifdef MEGA_DEBUG
   printf("uniform installed %p=%08x\n", startaddr_location, khrn_hw_addr(khrn_hw_alias_direct(ptr)));
#endif

#ifdef SIMPENROSE_RECORD_OUTPUT
   record_map_buffer(khrn_hw_addr(ptr), 4*count, L_UNIFORMS, (count==0) ? RECORD_BUFFER_IS_BOTH : RECORD_BUFFER_IS_CLIF, 4);
#endif

   map = (uint32_t *)mem_lock(hmap);

   if(!IS_GL_11(state)) {
      max = mem_get_size(program->mh_uniform_data);
   }

   for (i = 0; i < count; i++)
   {
      uint32_t u0 = map[2*i];
      uint32_t u1 = map[2*i+1];

      if (u0 == BACKEND_UNIFORM && u1 < 0x80000000)
      {
         //gl 2.0 uniforms
         *ptr = state->batch.uniform_data[u1];
#ifdef SIMPENROSE_RECORD_OUTPUT
         record_map_buffer(khrn_hw_addr(ptr), 4, L_UNIFORMS, RECORD_BUFFER_IS_DATA, 4);
#endif
      }
      else if (u0 == BACKEND_UNIFORM && (u1 & (GLXX_STATE_MASK - 0x80000000)))
      {
          //u1 is an offset into the server state struct
         *ptr = *(uint32_t *)GLXX_READ_STATE_OFFSET(state, u1);
#ifdef SIMPENROSE_RECORD_OUTPUT
         record_map_buffer(khrn_hw_addr(ptr), 4, L_UNIFORMS, RECORD_BUFFER_IS_DATA, 4);
#endif
      }
      else if (u0 == BACKEND_UNIFORM_LITERAL)
      {
         *ptr = u1;
#ifdef SIMPENROSE_RECORD_OUTPUT
         record_map_buffer(khrn_hw_addr(ptr), 4, L_UNIFORMS, RECORD_BUFFER_IS_DATA, 4);
#endif
      }
      else
      {
         if (!install_uniform(ptr, u0, u1, state, iu, num_vpm_rows, attrib, rso_format, fb_height))
            goto fail;
      }

      ptr++;
   }
   mem_unlock(hmap);
   return true;
fail:
   mem_unlock(hmap);
   return false;
}

vcos_static_assert(GL11_CONFIG_MAX_TEXTURE_UNITS <= GL20_CONFIG_MAX_TEXTURE_UNITS);
static bool glxx_install_tex_param(GLXX_SERVER_STATE_T *state, uint32_t *location, uint32_t u0, uint32_t u1)
{
   uint32_t index;
   bool is_cube;
   MEM_HANDLE_T thandle;
   GLXX_TEXTURE_T *texture;
   uint32_t texture_swizzle = 0;
   bool is_blob = false;
   MEM_HANDLE_T handle = MEM_INVALID_HANDLE;

   *location = 0;

   if (IS_GL_11(state))
   {
      index = u1;
      vcos_assert(index < GL11_CONFIG_MAX_TEXTURE_UNITS);
      vcos_assert(state->shader.texunits[index].props.active);
      is_cube = false;
   }
   else
   {
      GL20_UNIFORM_INFO_T *ui = &state->batch.uniform_info[state->batch.sampler_info[u1].uniform];
      index = state->batch.uniform_data[ui->offset + state->batch.sampler_info[u1].index];
      is_cube = ui->type != GL_SAMPLER_2D;
   }

   texture = NULL;
   thandle = MEM_INVALID_HANDLE;
   if (index < GL20_CONFIG_MAX_TEXTURE_UNITS)
   {
      thandle = is_cube ? state->bound_texture[index].mh_cube : state->bound_texture[index].mh_twod;
      texture = (GLXX_TEXTURE_T *)mem_lock(thandle);

      switch (glxx_texture_check_complete(texture, false))
      {
      //case COMPLETE: do nothing
      case INCOMPLETE:
         texture = NULL;
         break;
      case OUT_OF_MEMORY:
         goto fail;
      }
   }
   if (texture == NULL)
   {
      /* Set up dummy texture instead */
      int i;
      vcos_assert(!IS_GL_11(state));

      if (dummy_texture_data == NULL)
      {
#ifdef ANDROID
         if (dummy_texture_mem == MEM_INVALID_HANDLE)
         {
            dummy_texture_mem = mem_alloc_ex(64, 4096, MEM_FLAG_NORMAL, "Dummy texture data", MEM_COMPACT_DISCARD);
            if (dummy_texture_mem == MEM_INVALID_HANDLE)
               goto fail;
         }
         dummy_texture_data = (uint32_t *)mem_lock(dummy_texture_mem);
#else
         dummy_texture_data = rtos_malloc_priority(64, 4096, RTOS_PRIORITY_DIRECT, "Dummy texture data");
#endif
         if (dummy_texture_data == NULL)
            goto fail;
         for (i = 0; i < 16; i++)
            dummy_texture_data[i] = 0xff000000;
      }

      switch (u0)
      {
      case BACKEND_UNIFORM_TEX_PARAM0:
         *location = khrn_hw_addr(dummy_texture_data);
         break;
      case BACKEND_UNIFORM_TEX_PARAM1:
         *location = 4 << 8 | 4 << 20;
         break;
      default:
         *location = 0;
      }
   }
   else switch (u0)
   {
      case BACKEND_UNIFORM_TEX_PARAM0:
      {
         uint32_t mipmap_count = 0;
         uint32_t type = 0;
         uint32_t offset;

         vcos_assert(!texture->is_cube || !IS_GL_11(state));

         mipmap_count = glxx_texture_get_mipmap_count(texture);
         if(texture->force_disable_mipmap)
			{
			mipmap_count = 1;
			}
#ifdef RSO_ARGB8888_TEXTURE
         if(texture->preferRSO == 1)
            type = 16;
         else
#endif
         type = tu_image_format_to_type(glxx_texture_get_tformat(texture, false));

         if (texture->binding_type != TEXTURE_STATE_COMPLETE_UNBOUND || (!is_cube && texture->framebuffer_sharing))
         {
            KHRN_IMAGE_T *image;
            
            image = (KHRN_IMAGE_T *)mem_lock(texture->mh_mipmaps[0][0]);
            handle = image->mh_storage;
            offset = image->offset;
            mem_unlock(texture->mh_mipmaps[0][0]);
#ifdef SIMPENROSE_RECORD_OUTPUT
            vcos_assert(0);  //TODO?
#endif
         }
         else
         {
            handle = glxx_texture_get_storage_handle(texture);
            offset = glxx_texture_get_mipmap_offset(texture, is_cube ? TEXTURE_BUFFER_POSITIVE_X : TEXTURE_BUFFER_TWOD, 0, false);
            is_blob = true;
#ifdef SIMPENROSE_RECORD_OUTPUT
            record_map_mem_buffer_section(handle, offset, mem_get_size(handle)-offset, L_TEXTURE_DATA, RECORD_BUFFER_IS_BOTH, mem_get_align(handle));
#endif
         }

         vcos_assert(handle != MEM_INVALID_HANDLE);
         vcos_assert(mipmap_count >= 1 && mipmap_count <= LOG2_MAX_TEXTURE_SIZE + 1);

         texture_swizzle = (texture_swizzle + 1) & 3;
#ifdef RSO_ARGB8888_TEXTURE
         type = type &0xF;
#endif
         offset += (mipmap_count - 1) | (type & 15) << 4 | is_cube << 9 | 0/*TODO: texture_swizzle*/ << 10;
         if(!glxx_big_mem_insert(location, handle, offset))
            goto fail;

         if (!glxx_hw_texture_fix(thandle))
            goto fail;
         break;
      }
      case BACKEND_UNIFORM_TEX_PARAM1:
      {
#ifdef RSO_ARGB8888_TEXTURE
         uint32_t type = 0;
         if(texture->preferRSO == 1)
            type = 16;
         else
            type = tu_image_format_to_type(glxx_texture_get_tformat(texture, false));

         type = type &0x10;
#else
         uint32_t type = tu_image_format_to_type(glxx_texture_get_tformat(texture, false));
#endif

         *location =
            ((type >> 4) & 1) << 31 |
            glxx_convert_wrap(texture->wrap.s) << 0 |
            glxx_convert_wrap(texture->wrap.t) << 2 |
            glxx_convert_filter(texture->min) << 4 |
            glxx_convert_filter(texture->mag) << 7 |
            (texture->width &0x7FF)<< 8 |
#ifdef WORKAROUND_VND_12
            0 << 19 |     /* ETCFLIPY */
#else
            1 << 19 |     /* ETCFLIPY */
#endif
            (texture->height &0x7FF)<< 20;
         break;
      }
      case BACKEND_UNIFORM_TEX_CUBE_STRIDE:
      {
         vcos_assert(!IS_GL_11(state));

         if (!is_cube || !texture->is_cube)
         {
            //XXX TODO find out why are we getting !texture->is_cube
            *location = 0;
         }
         else
         {
            uint32_t stride = glxx_texture_get_cube_stride(texture);
            vcos_assert(!(stride & 0xfff));

            *location = stride | 1 << 30;
         }
         break;
      }
      default:
         UNREACHABLE();
         return false;
   }
   if (thandle != MEM_INVALID_HANDLE)
      mem_unlock(thandle);

   return true;
fail:
   if(is_blob)
      mem_unretain(handle);
   if (thandle != MEM_INVALID_HANDLE)
      mem_unlock(thandle);
   return false;
}

#ifdef __BCM2708A0__
static bool backend_uniform_tex_param0( uint32_t u1, 
                  GLXX_SERVER_STATE_T *state,
                  GL20_HW_INDEXED_UNIFORM_T *iu,
                  uint32_t *location)
{
   uint32_t index = u1 & 0xffff;
   uint32_t size = u1 >> 16;
   uint32_t *texa = NULL;
   uint32_t i;
   uint32_t *data;

   UNUSED_NDEBUG(state);
   UNUSED_NDEBUG(max);
   data = state->batch.uniform_data;

   vcos_assert(!IS_GL_11(state));

   for (i = 0; i < iu->count; i++)
   {
      if (iu->index[i] == index && iu->size[i] == size)
      {
         texa = iu->addr[i];
         break;
      }
   }

   if (i == iu->count)
   {
      texa = glxx_big_mem_alloc_junk(64 * ((size + 3) / 4),4096);
      if (!texa)
      {
         *location = 0;
         return false;
      }

#ifdef SIMPENROSE_RECORD_OUTPUT
      record_map_buffer(khrn_hw_addr(texa), 4 * size, L_TEXTURE_DATA, RECORD_BUFFER_IS_BOTH, 4);
#endif

      for (i = 0; i < size; i++)
      {
         texa[(i & ~3) << 2 | (i & 3)] = data[index + i];
      }

      if (iu->count < IU_MAX)
      {
         iu->index[iu->count] = index;
         iu->size[iu->count] = size;
         iu->addr[iu->count] = texa;
         iu->count++;
      }
   }

#ifdef SIMPENROSE_RECORD_OUTPUT
   label = L_TEX_PARAM0;
#endif
   //glxx_add_addr_plus(buf,
   //   &texa,
   //   0 << 0 |        // no mipmaps
   //   0 << 4);        // type = RGBA_8888

   *location = khrn_hw_addr(khrn_hw_alias_direct(texa)) +
      0 << 0 |        // no mipmaps
      0 << 4;        // type = RGBA_8888;
   
   return true;
}

static bool backend_uniform_tex_param1( uint32_t u1, 
                  GLXX_SERVER_STATE_T *state,
                  GL20_HW_INDEXED_UNIFORM_T *iu,
                  uint32_t *location)
{
   uint32_t size = u1 >> 16;
   UNUSED_NDEBUG(state);
   UNUSED(iu);
   UNUSED(data);
   UNUSED(buf);

#ifdef SIMPENROSE_RECORD_OUTPUT
   label = L_TEX_PARAM1;
#endif
   vcos_assert(!IS_GL_11(state));
   *location =
      1 << 0 |        // wrap_s
      1 << 2 |        // wrap_t
      1 << 4 |        // min=nearest
      1 << 7 |        // mag=nearest
      size << 8 |     // width
      0 << 19 |       // etcflipy
      1 << 20;       // height
   
   return true;
}
#else
static bool backend_uniform_address( uint32_t u1, 
                  GLXX_SERVER_STATE_T *state,
                  GL20_HW_INDEXED_UNIFORM_T *iu,
                  uint32_t *location)
{
   uint32_t index = u1 & 0xffff;
   uint32_t size = u1 >> 16;
   uint32_t i;
   uint32_t *texa = NULL;
   UNUSED_NDEBUG(state);

   vcos_assert(!IS_GL_11(state));
   for (i = 0; i < iu->count; i++)
   {
      if (iu->index[i] == index && iu->size[i] == size)
      {
         texa = iu->addr[i];
         break;
      }
   }

   if (i == iu->count)
   {
      texa = glxx_big_mem_alloc_junk(4 * size, 4);
      if(!texa)
      {
         *location = 0;
         return false;
      }
#ifdef SIMPENROSE_RECORD_OUTPUT
      record_map_buffer(khrn_hw_addr(texa), 4 * size, L_TEXTURE_DATA, RECORD_BUFFER_IS_BOTH, 4);
#endif
      memcpy(texa, state->batch.uniform_data + index, 4 * size);

      if (iu->count < IU_MAX)
      {
         iu->index[iu->count] = index;
         iu->size[iu->count] = size;
         iu->addr[iu->count] = texa;
         iu->count++;
      }
   }

   *location = khrn_hw_addr(khrn_hw_alias_direct(texa)); //PTR
   return true;
}
#endif

/*!
 * \brief Converts the primitive type from its GLenum representation
 *        to the representation used in the simulator (unsigned int).
 *
 * Also asserts that the mode is within the valid/implemented range.
 *
 * \param mode is the GL primitive type.
 */
static uint32_t convert_primitive_type(GLenum mode)
{
   vcos_assert(mode <= GL_TRIANGLE_FAN);

   return (uint32_t)mode;
}

/*!
 * \brief Converts the index type from the GLenum
 *        representation to the internal one used in the simulator.
 *
 * \param type is the GL index type specifier.
 */
static uint32_t convert_index_type(GLenum type)
{
   switch (type) {
   case GL_UNSIGNED_BYTE:
      return 0; // VCM_SRCTYPE_8BIT;
   case GL_UNSIGNED_SHORT:
      return 1; // VCM_SRCTYPE_16BIT;
   default:
      UNREACHABLE(); // unsupported index type
      return 0;
   }
}
#ifdef DRAW_TEX_LOGGING
static void draw_tex_log(GLXX_SERVER_STATE_T *state, GLXX_HW_FRAMEBUFFER_T *fb, GLXX_HW_RENDER_STATE_T *rs, GLfloat Xs, GLfloat Ys, GLfloat Zw, GLfloat Ws, GLfloat Hs)
{
   GL11_CACHE_KEY_T * entry = &state->shader;
   int i;

   printf("state_name: %d rs: %d drawTex: Xs %f Ys %f Zw %f Ws %f Hs %f\n",
      state->name, rs->name, Xs, Ys, Zw, Ws, Hs);

   for (i = 0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++) {
      if(entry->texunits[i].props.active)
      {
         GLXX_TEXTURE_T * texture;
         printf("texunit:%d mode:0x%x coord_replace:0x%x\n",i,entry->texunits[i].mode,entry->texunits[i].coord_replace);
         printf("   active:%d complex:%d has_color:%d has_alpha:%d\n",entry->texunits[i].props.active,entry->texunits[i].props.complex,
            entry->texunits[i].props.has_color, entry->texunits[i].props.has_alpha);
         printf("   rgb: combine:0x%x scale:%f source:0x%x 0x%x 0x%x operand:0x%x 0x%x 0x%x\n",entry->texunits[i].rgb.combine,entry->texunits[i].rgb.scale,
            entry->texunits[i].rgb.source[0],entry->texunits[i].rgb.source[1],entry->texunits[i].rgb.source[2],
            entry->texunits[i].rgb.operand[0],entry->texunits[i].rgb.operand[1],entry->texunits[i].rgb.operand[2]);
         printf("   alpha: combine:0x%x scale:%f source:0x%x 0x%x 0x%x operand:0x%x 0x%x 0x%x\n",entry->texunits[i].alpha.combine,entry->texunits[i].alpha.scale,
            entry->texunits[i].alpha.source[0],entry->texunits[i].alpha.source[1],entry->texunits[i].alpha.source[2],
            entry->texunits[i].alpha.operand[0],entry->texunits[i].alpha.operand[1],entry->texunits[i].alpha.operand[2]);
         texture = mem_lock(state->bound_texture[i].mh_twod);
         printf("   handle: %d crop_rect: Ucr %d Vcr %d Wcr %d Hcr %d W %d H %d\n",
            state->bound_texture[i].mh_twod,
            texture->crop_rect.Ucr,texture->crop_rect.Vcr,texture->crop_rect.Wcr,texture->crop_rect.Hcr,texture->width,texture->height);
         printf("   wrap: s 0x%x t 0x%x mag: 0x%x min: 0x%x\n",texture->wrap.s,texture->wrap.t,texture->mag,texture->min);
         mem_unlock(state->bound_texture[i].mh_twod);
      }
   }

   printf("viewport: x %d y %d w %d h %d\n",state->viewport.x,state->viewport.y,state->viewport.width,state->viewport.height);
   printf("scissor_test: %d\n",state->caps.scissor_test);
   if(state->caps.scissor_test)
      printf("  scissor: x %d y %d w %d h %d\n",state->scissor.x,state->scissor.y,state->scissor.width,state->scissor.height);
   printf("blend: %d\n",state->caps.blend);
   if(state->caps.blend)
   {
      printf("  color_mask 0x%x eqn 0x%x eqn_alpha 0x%x\n",
         entry->common.blend.color_mask,entry->common.blend.equation,entry->common.blend.equation_alpha);
      printf("  src_func 0x%x src_func_a 0x%x dst_func 0x%x dst_func_a 0x%x\n",
         entry->common.blend.src_function,entry->common.blend.src_function_alpha,entry->common.blend.dst_function,entry->common.blend.dst_function_alpha);
      printf("  blend_color: (%f %f %f %f)\n",
         state->blend_color[0],state->blend_color[1],state->blend_color[2],state->blend_color[3]);
   }
   printf("stencil_test: %d depth_test: %d\n",state->caps.stencil_test,state->caps.depth_test);
   printf("logic_op: %d func: 0x%x\n",state->caps.color_logic_op,entry->common.blend.logic_op);
   printf("dither: %d multisample: %d\n",state->caps.dither,state->caps.multisample);
   printf("sample_a2c: %d sample_c: %d sample_a21: %d\n",
      state->caps.sample_alpha_to_coverage,state->caps.sample_coverage,state->caps.sample_alpha_to_one);
   printf("polygon_offset: %d alpha_test: %d fog: %d\n",
      state->caps.polygon_offset_fill,state->caps_fragment.alpha_test,state->caps_fragment.fog);
   printf("color: (%f %f %f %f)\n",
         state->copy_of_color[0],state->copy_of_color[1],state->copy_of_color[2],state->copy_of_color[3]);
   
   printf("cattribs_live:0x%x vattribs_live:0x%x\n",entry->cattribs_live,entry->vattribs_live);

   /* TODO frame buffer format */
   printf("framebuffer: format 0x%x has_depth %d has_stencil %d ms %d w %d h %d\n",
      fb->col_format, fb->have_depth, fb->have_stencil, fb->ms, fb->width, fb->height);

   printf("--------------------\n");
}
#endif

bool glxx_hw_draw_tex(GLXX_SERVER_STATE_T *state, float Xs, float Ys, float Zw, float Ws, float Hs)
{
   uint32_t i;
   GLXX_HW_FRAMEBUFFER_T fb;
   MEM_HANDLE_T cunif_map, vunif_map, funif_map;
   uint32_t cattribs_live;
   uint32_t vattribs_live;

   uint32_t cunif_count;
   uint32_t vunif_count;
   uint32_t funif_count;
   uint32_t mergeable_attribs[GLXX_CONFIG_MAX_VERTEX_ATTRIBS];
   uint32_t vattribs_order[GLXX_CONFIG_MAX_VERTEX_ATTRIBS*2];
   uint32_t cattribs_order[GLXX_CONFIG_MAX_VERTEX_ATTRIBS*2];
   uint32_t num_vpm_rows_c[4];
   uint32_t num_vpm_rows_v[4];
   GLXX_HW_RENDER_STATE_T *rs;
   GLXX_HW_SHADER_RECORD_T *shader_record;
   uint32_t attr_count = 1;
   uint32_t stride = 3*4;/* sizeof(float)*(3 vertices) */
   uint8_t *instr;
   uint32_t *attrib_data;
   float *p;
   float x,y,dw,dh;
   float s[GL11_CONFIG_MAX_TEXTURE_UNITS],t[GL11_CONFIG_MAX_TEXTURE_UNITS],sw[GL11_CONFIG_MAX_TEXTURE_UNITS],sh[GL11_CONFIG_MAX_TEXTURE_UNITS];
   int merge_index;

   uint32_t color_varyings;
   
   GLXX_ATTRIB_T attrib[8];
   
   //unused gl 2.0 specific
   GL20_PROGRAM_T *program = NULL;
   GL20_HW_INDEXED_UNIFORM_T iu;
   //

   vcos_assert(IS_GL_11(state));

   rs = glxx_install_framebuffer(state, &fb, false);
   if (!rs)
      goto done;

   if(!glxx_lock_fixer_stuff(rs))
      goto fail2;

   for (i = 0; i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++)
   {
      attrib[i].enabled = 0;
      mergeable_attribs[i] = (uint32_t)~0;
   }   

   for(i = 0;i<4;i++) {
      num_vpm_rows_c[i] = 0;
      num_vpm_rows_v[i] = 0;
   }

   glxx_hw_get_attr_live(state, 0, attrib);
     
   attrib[GL11_IX_VERTEX].enabled = 1;
   attrib[GL11_IX_VERTEX].size = 3;
   attrib[GL11_IX_VERTEX].type = GL_FLOAT;
   attrib[GL11_IX_VERTEX].pointer = 0;
   attrib[GL11_IX_COLOR].value[0] = state->copy_of_color[0];
   attrib[GL11_IX_COLOR].value[1] = state->copy_of_color[1];
   attrib[GL11_IX_COLOR].value[2] = state->copy_of_color[2];
   attrib[GL11_IX_COLOR].value[3] = state->copy_of_color[3];
   merge_index = GL11_IX_VERTEX;
   for(i=0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++) {
      if(state->shader.texunits[i].props.active) {
         GLXX_TEXTURE_T * texture;
         attrib[GL11_IX_TEXTURE_COORD+i].enabled = 1;
         attrib[GL11_IX_TEXTURE_COORD+i].size = 2;
         attrib[GL11_IX_TEXTURE_COORD+i].type = GL_FLOAT;
         attrib[GL11_IX_TEXTURE_COORD+i].pointer = (void *)stride;
         stride += 2*4;/* sizeof(float)*(2 texcoords) */

         texture = mem_lock(state->bound_texture[i].mh_twod);
         s[i] = ((float)texture->crop_rect.Ucr)/texture->width;
         t[i] = ((float)texture->crop_rect.Vcr)/texture->height;
         sw[i] = ((float)texture->crop_rect.Wcr)/texture->width;
         sh[i] = ((float)texture->crop_rect.Hcr)/texture->height;
         mem_unlock(state->bound_texture[i].mh_twod);

         mergeable_attribs[merge_index] = GL11_IX_TEXTURE_COORD+i;
         merge_index = GL11_IX_TEXTURE_COORD+i;
      }
   }

   state->batch.primitive_mode = GL_TRIANGLE_FAN;
   calculate_and_hide(state, &fb, attrib);
   
   state->shader.drawtex = true;

#ifdef DRAW_TEX_LOGGING
   draw_tex_log(state, &fb, rs, Xs, Ys, Zw, Ws, Hs);
#endif
   
   attrib_data = glxx_big_mem_alloc_junk(stride*4, 4);
   if(!attrib_data)
      goto fail;
      
   /* calculate vertices and tex coords for quad */
   p = (float *)attrib_data;
   
   x = 2.0f*Xs/state->viewport.width - 1.0f;
   y = 2.0f*Ys/state->viewport.height - 1.0f;
   dw = 2.0f*Ws/state->viewport.width;
   dh = 2.0f*Hs/state->viewport.height;
   
   *p++ = x;
   *p++ = y;
   *p++ = Zw;
   for(i=0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++) {
      if(state->shader.texunits[i].props.active) {
         *p++ = s[i];
         *p++ = t[i];
      }
   }

   *p++ = x + dw;
   *p++ = y;
   *p++ = Zw;
   for(i=0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++) {
      if(state->shader.texunits[i].props.active) {
         *p++ = s[i] + sw[i];
         *p++ = t[i];
      }
   }

   *p++ = x + dw;
   *p++ = y + dh;
   *p++ = Zw;
   for(i=0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++) {
      if(state->shader.texunits[i].props.active) {
         *p++ = s[i] + sw[i];
         *p++ = t[i] + sh[i];
      }
   }

   *p++ = x;
   *p++ = y + dh;
   *p++ = Zw;
   for(i=0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++) {
      if(state->shader.texunits[i].props.active) {
         *p++ = s[i];
         *p++ = t[i] + sh[i];
      }
   }
   
   vcos_assert((uint8_t *)p == (uint8_t *)attrib_data + stride*4);
   
   /* TODO: only allocate space for as many vertex attributes as we need */
   /* TODO: extended vertex stride? */
   shader_record = (GLXX_HW_SHADER_RECORD_T *)glxx_big_mem_alloc_junk(100, 16);
   if (!shader_record) goto fail;

   /* create or retrieve shaders from cache and setup attribs_live */

   cattribs_live = state->batch.cattribs_live;
   vattribs_live = state->batch.vattribs_live;
   if(!vcos_verify(get_shaders(
         program,
         shader_record,
         &cunif_map, &vunif_map, &funif_map,
         &color_varyings,
         state,
         attrib,
         mergeable_attribs,
         cattribs_order,
         vattribs_order)))
   {
      glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
      goto fail;
   }

   /* Create VCD configuration */

   shader_record->attr[0].base = khrn_hw_addr(khrn_hw_alias_direct(attrib_data));
   shader_record->attr[0].sizem1 = stride-1;
   shader_record->attr[0].stride = stride;
   shader_record->attr[0].coffset = 0;
   shader_record->cattrsel = 1;
   num_vpm_rows_c[0] = (((stride + 3) & ~3)+3)/4;
   shader_record->attr[0].voffset = 0;
   shader_record->vattrsel = 1;
   num_vpm_rows_v[0] = (((stride + 3) & ~3)+3)/4;
   
   shader_record->vattrsize = (stride + 3) & ~3;
   shader_record->cattrsize = (stride + 3) & ~3;

   /* Install uniforms */
   cunif_count = mem_get_size(cunif_map)/8;
   vunif_count = mem_get_size(vunif_map)/8;
   funif_count = mem_get_size(funif_map)/8;

#ifdef XXX_OFFLINE
   if (!shader.use_offline)
#endif
   {
      if(!install_uniforms(
         &shader_record->cunif,
         state,
         program,
         cunif_count,
         cunif_map,
         &iu,
         num_vpm_rows_c,
         attrib,/*TODO for GL 2.0 NULL is passed instead of attrib - does this matter? */
         state->shader.common.rso_format,
         fb.height
         ))
      {
         goto fail;
      }
   }
   if(!install_uniforms(
      &shader_record->vunif,
      state,
      program,
      vunif_count,
      vunif_map,
      &iu,
      num_vpm_rows_v,
      attrib,
      state->shader.common.rso_format,
      fb.height
      ))
   {
      goto fail;
   }
   if(!install_uniforms(
      &shader_record->funif,
      state,
      program,
      funif_count,
      funif_map,
      &iu,
      0,
      attrib,
      state->shader.common.rso_format,
      fb.height
      ))
   {
      goto fail;
   }
   

#ifdef DUMP_SHADER
   dump_shader(&shader.cshader, &shader.cunif);
#endif
   
   /* emit any necessary config change instructions */
   {
      bool is_empty,ok;
      ok = do_changed_cfg(state, &fb, color_varyings, &is_empty);
      if(!ok) goto fail;
      if(is_empty) 
      {
         glxx_unlock_fixer_stuff();
         goto done;   /* empty region - nothing to draw */         
      }
   }   

#ifdef SIMPENROSE_RECORD_OUTPUT
   record_map_buffer(khrn_hw_addr(shader_record), 44 + 8*(attr_count-1), L_GL_SHADER_RECORD, RECORD_BUFFER_IS_BOTH, 16);
#endif
    
   instr = glxx_big_mem_alloc_cle(20);
   if (!instr) goto fail;
   
   Add_byte(&instr, KHRN_HW_INSTR_GL_SHADER);     //(5)
   add_pointer(&instr, (uint8_t *)shader_record + (attr_count & 7));
   
   // Emit a GLDRAWARRAYS instruction
   Add_byte(&instr, KHRN_HW_INSTR_GLDRAWARRAYS);
   add_byte(&instr, convert_primitive_type(GL_TRIANGLE_FAN));  //Primitive mode
   add_word(&instr, 4);       //Length (number of vertices)
   add_word(&instr, 0);            //Index of first vertex

   Add_byte(&instr, KHRN_HW_INSTR_NOP);         //Pad to the same length as KHRN_HW_INSTR_GLDRAWELEMENTS to make it easier for ourselves
   Add_byte(&instr, KHRN_HW_INSTR_NOP);
   Add_byte(&instr, KHRN_HW_INSTR_NOP);
   Add_byte(&instr, KHRN_HW_INSTR_NOP);
   
   Add_byte(&instr, KHRN_HW_INSTR_NOP);        //(1) TODO: is this necessary?


   Add_byte(&instr, KHRN_HW_INSTR_NOP);        //(1) TODO: is this necessary?

#ifdef SIMPENROSE_WRITE_LOG
   // Increment batch count
   log_batchcount++;
#endif

   /* Mark the state as drawn so that clearing works properly */
   rs->drawn = true;
   rs->xxx_empty = false;

   glxx_unlock_fixer_stuff();

done:
   
   state->shader.drawtex = false;
   
   return true;

fail:
   glxx_unlock_fixer_stuff();

fail2:
   state->shader.drawtex = false;
  
   glxx_hw_discard_frame(rs);

   return false;
}
