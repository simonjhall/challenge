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
#include "middleware/khronos/gl11/gl11_server.h"
#include "middleware/khronos/glxx/glxx_server.h"
#include "middleware/khronos/glxx/glxx_server_internal.h"
#include "middleware/khronos/glxx/glxx_framebuffer.h"
#include "helpers/bcmcomp/bcmcomp.h"

// #define USE_FAST_PATH
/*
   Khronos Documentation:
   http://www.khronos.org/registry/gles/extensions/OES/OES_draw_texture.txt
   Overview
   
   This extension defines a mechanism for writing pixel
    rectangles from one or more textures to a rectangular
    region of the screen.  This capability is useful for
    fast rendering of background paintings, bitmapped font
    glyphs, and 2D framing elements in games.  This
    extension is primarily intended for use with OpenGL ES.

    The extension relies on a new piece of texture state
    called the texture crop rectangle, which defines a
    rectangular subregion of a texture object.  These
    subregions are used as sources of pixels for the texture
    drawing function.

    Applications use this extension by configuring the
    texture crop rectangle for one or more textures via
    ActiveTexture() and TexParameteriv() with pname equal to
    TEXTURE_CROP_RECT_OES.  They then request a drawing
    operation using DrawTex{sifx}[v]OES().  The effect of
    the latter function is to generate a screen-aligned
    target rectangle, with texture coordinates chosen to map
    the texture crop rectangle(s) linearly to fragments in
    the target rectangle.  The fragments are then processed
    in accordance with the fragment pipeline state.

   Texture Rectangle Drawing

    OpenGL supports drawing sub-regions of a texture to
    rectangular regions of the screen using the texturing
    pipeline.  Source region size and content are determined
    by the texture crop rectangle(s) of the enabled
    texture(s) (see section 3.8.14).

    The functions 

    void DrawTex{sifx}OES(T Xs, T Ys, T Zs, T Ws, T Hs);
    void DrawTex{sifx}vOES(T *coords);

    draw a texture rectangle to the screen.  Xs, Ys, and Zs
    specify the position of the affected screen rectangle.
    Xs and Ys are given directly in window (viewport)
    coordinates.  Zs is mapped to window depth Zw as follows:

                 { n,                 if z <= 0
            Zw = { f,                 if z >= 1
                 { n + z * (f - n),   otherwise

    where <n> and <f> are the near and far values of
    DEPTH_RANGE.  Ws and Hs specify the width and height of
    the affected screen rectangle in pixels.  These values
    may be positive or negative; however, if either (Ws <=
    0) or (Hs <= 0), the INVALID_VALUE error is generated.

    Calling one of the DrawTex functions generates a
    fragment for each pixel that overlaps the screen
    rectangle bounded by (Xs, Ys) and (Xs + Ws), (Ys + Hs).
    For each generated fragment, the depth is given by Zw
    as defined above, and the color by the current color.
    
    If EXT_fog_coord is supported, and FOG_COORDINATE_SOURCE_EXT
    is set to FOG_COORINATE_EXT, then the fragment distance for
    fog purposes is set to CURRENT_FOG_COORDINATE. Otherwise,
    the fragment distance for fog purposes is set to 0.
    
    Texture coordinates for each texture unit are computed
    as follows:

    Let X and Y be the screen x and y coordinates of each
    sample point associated with the fragment.  Let Wt and
    Ht be the width and height in texels of the texture
    currently bound to the texture unit.  (If the texture is
    a mipmap, let Wt and Ht be the dimensions of the level
    specified by TEXTURE_BASE_LEVEL.)  Let Ucr, Vcr, Wcr and
    Hcr be (respectively) the four integers that make up the
    texture crop rectangle parameter for the currently bound
    texture.  The fragment texture coordinates (s, t, r, q)
    are given by

    s = (Ucr + (X - Xs)*(Wcr/Ws)) / Wt
    t = (Vcr + (Y - Ys)*(Hcr/Hs)) / Ht
    r = 0
    q = 1

    In the specific case where X, Y, Xs and Ys are all
    integers, Wcr/Ws and Hcr/Hs are both equal to one, the
    base level is used for the texture read, and fragments
    are sampled at pixel centers, implementations are
    required to ensure that the resulting u, v texture
    indices are also integers.  This results in a one-to-one
    mapping of texels to fragments.

    Note that Wcr and/or Hcr can be negative.  The formulas
    given above for s and t still apply in this case.  The
    result is that if Wcr is negative, the source rectangle
    for DrawTex operations lies to the left of the reference
    point (Ucr, Vcr) rather than to the right of it, and
    appears right-to-left reversed on the screen after a
    call to DrawTex.  Similarly, if Hcr is negative, the
    source rectangle lies below the reference point (Ucr,
    Vcr) rather than above it, and appears upside-down on
    the screen.

    Note also that s, t, r, and q are computed for each
    fragment as part of DrawTex rendering.  This implies
    that the texture matrix is ignored and has no effect on
    the rendered result.


   Implementation Notes:
   
*/
#ifdef USE_FAST_PATH

static VCOS_EVENT_T draw_event;

static BCM_COMP_RGB_FORMAT convert_format_to_comp(KHRN_IMAGE_FORMAT_T format)
{
   uint32_t comp_format;

   switch(format)
   {
   case ABGR_8888_RSO:
      comp_format = BCM_COMP_COLOR_FORMAT_8888_ABGR;
      break;
   case BGR_888_RSO:/* TODO the inconsistency with the ABGR case above bothers me */
      comp_format = BCM_COMP_COLOR_FORMAT_888_RGB;
      break;

      /* frame buffer formats */
   case RGB_565_RSO:
      comp_format = BCM_COMP_COLOR_FORMAT_565_RGB;
      break;
   case XBGR_8888_RSO:
      comp_format = BCM_COMP_COLOR_FORMAT_8888_ABGR | BCM_COMP_FORMAT_DISABLE_ALPHA;
      break;
   default:
      UNREACHABLE();
   }
   return comp_format;
}

static bool s_init_compositor;

#define DT_MAX_SURFACES 2
static BCM_COMP_RGB_SURFACE_DEF surfaces[DT_MAX_SURFACES];
static MEM_HANDLE_T surface_handles[DT_MAX_SURFACES];

static void *lookup_surface_def(uint32_t id)
{
   if(id<=DT_MAX_SURFACES)
      return &surfaces[id-1];  

   return NULL;
}


static uint32_t create_surface(KHRN_IMAGE_T * image)
{
   uint32_t i,id = ~0;

   for(i=0;i<DT_MAX_SURFACES;i++)
   {
      if(surfaces[i].buffer == 0)
      {
         surfaces[i].format = convert_format_to_comp(image->format);
         surfaces[i].width = image->width;
         surfaces[i].height = image->height;
         surfaces[i].stride = image->stride;
         surfaces[i].buffer = khrn_hw_alias_direct(mem_lock(image->mh_storage));
         //surfaces[i].buffer = mem_lock(image->mh_storage);
         if(surfaces[i].buffer != 0)
         {
            MEM_ASSIGN(surface_handles[i],image->mh_storage);
            id = i+1;
         }
         else
         {
            mem_unlock(image->mh_storage);
         }
         break;
      }
   }
   return id;
}

static void destroy_surface(uint32_t id)
{
   if(id<=DT_MAX_SURFACES)
   {
      id = id-1;
      surfaces[id].buffer = 0;
      mem_unlock(surface_handles[id]);
      MEM_ASSIGN(surface_handles[id],MEM_INVALID_HANDLE);
   }
}


static void add_rect(BCM_COMP_OBJECT *comp, uint32_t sid, int sx,int sy, int sw, int sh, int dx, int dy, int dw, int dh)
{
   /* 16.16 format */
   memset(comp, 0, sizeof(BCM_COMP_OBJECT));
   comp->surface_id = sid;
   comp->config_mask = BCM_COMP_SOURCE_RECT_BIT | BCM_COMP_TARGET_RECT_BIT;
   comp->source_rect.x = sx << 16;
   comp->source_rect.y = sy << 16;
   comp->source_rect.width = sw << 16;
   comp->source_rect.height = sh << 16;
   comp->target_rect.x = dx << 16;
   comp->target_rect.y = dy << 16;
   comp->target_rect.width = dw << 16;
   comp->target_rect.height = dh << 16;
}
/*
static void signal_event(void *p)
{
   vcos_event_signal((VCOS_EVENT_T *)p);
}
*/

static bool try_fast_path(GLXX_SERVER_STATE_T *state, GLfloat Xs, GLfloat Ys, GLfloat Zw, GLfloat Ws, GLfloat Hs)
{
   uint32_t i, active_index = ~0;
   GL11_TEXUNIT_T * texunit = NULL;
   MEM_HANDLE_T thandle = MEM_INVALID_HANDLE;
   GL11_CACHE_TEXUNIT_ABSTRACT_T *texabs = NULL;
   GLXX_TEXTURE_T *texture = NULL;

   /* Backend tests not supported */
   if (state->caps.depth_test ||
      state->caps.stencil_test ||
      state->caps_fragment.alpha_test ||
      /* extras */
      state->caps.color_logic_op || 
 //     state->caps.dither ||
      state->caps.sample_alpha_to_coverage ||
      state->caps.sample_coverage ||
      state->caps.sample_alpha_to_one || 
      state->caps.polygon_offset_fill ||
      state->caps_fragment.fog
      )
   {
      goto use_slow;
   }

   /* Exactly one texture unit must be enabled with a bound raster order texture */
   
   for (i = 0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++)
   {
      texunit = &state->texunits[i];

      thandle = state->bound_texture[i].mh_twod;

      if (texunit->enabled && thandle != MEM_INVALID_HANDLE) {
         GLXX_TEXTURE_COMPLETENESS_T completeness;
         KHRN_IMAGE_FORMAT_T format;
         texture = (GLXX_TEXTURE_T *)mem_lock(thandle);
         completeness = glxx_texture_check_complete(texture, false);
         format = texture->format;

         /* Mipmap texture filters are not permitted
            scaling is not permitted 
            */
         if(khrn_image_is_rso(texture->format) &&
            completeness==COMPLETE &&
            ((texture->min == GL_NEAREST) || (texture->min == GL_LINEAR)) &&
            abs(texture->crop_rect.Wcr) == Ws &&
            abs(texture->crop_rect.Hcr) == Hs
            )
         {
            mem_unlock(thandle);
            /* Track the candidate texture unit to use */
            active_index = i;
            break;
         }

         mem_unlock(thandle);
      }
   }

   if (active_index == ~0) {
      goto use_slow;
   }

   texunit = &state->texunits[active_index];
   thandle = state->bound_texture[active_index].mh_twod;
   texabs = &state->shader.texunits[active_index];

   /* Active texture unit must be REPLACE */
   /* MODULATE is okay as long as the active draw colour is [1, 1, 1, 1] */
   /* COMBINE mode is okay too as long as both RGB and alpha are set to REPLACE */
   if (!(
         (texabs->mode == GL_REPLACE) ||

#if 0
         ((texabs->mode == GL_MODULATE) &&
          (state->copy_of_color[0] == 1.0f) &&
          (state->copy_of_color[1] == 1.0f) &&
          (state->copy_of_color[2] == 1.0f) &&
          (state->copy_of_color[3] == 1.0f)) ||
#endif

         ((texabs->mode == GL_COMBINE)               &&
          (texabs->rgb.combine      == GL_REPLACE)   &&
          (texabs->rgb.source[0]    == GL_TEXTURE)   &&
          (texabs->rgb.operand[0]   == GL_SRC_COLOR) &&
          (texabs->alpha.combine    == GL_REPLACE)   &&
          (((texabs->alpha.source[0]  == GL_TEXTURE)   &&
            (texabs->alpha.operand[0] == GL_SRC_COLOR)) ||
           ((texabs->alpha.source[0]  == GL_CONSTANT)   &&
            (texabs->alpha.operand[0] == GL_ONE_MINUS_SRC_ALPHA))))

         ))
   {
      goto use_slow;
   }

   /* do fast path */
   {
      KHRN_IMAGE_T *src_image, *dst_image;
      MEM_HANDLE_T hsrc_im, hdst_im;
      uint32_t src_surf_id, dst_surf_id;
      BCM_COMP_RECT target_scissor;
      BCM_COMP_OBJECT comp[4];
      int su,sv,sw,sh;
      int dx, dy, dw, dh;
      dx = (int)Xs, dy = (int)Ys, dw = (int)Ws, dh = (int)Hs;

      if(!s_init_compositor)
      {       
         if(BCM_COMP_STATUS_OK == bcmcompInit())
         {
            vcos_verify(vcos_event_create(&draw_event, "OES_draw_texture_draw_event") == VCOS_SUCCESS);
            s_init_compositor = true;
         }
         else
         {
            vcos_assert(0);
            goto use_slow;
         }
      }

      texture = (GLXX_TEXTURE_T *)mem_lock(thandle);
      hsrc_im = texture->mh_mipmaps[0][0];

      if (state->mh_bound_framebuffer != MEM_INVALID_HANDLE)
      {
         GLXX_FRAMEBUFFER_T *framebuffer = (GLXX_FRAMEBUFFER_T *)mem_lock(state->mh_bound_framebuffer);
         hdst_im = glxx_attachment_info_get_image(&framebuffer->attachments.color);
         mem_unlock(state->mh_bound_framebuffer);
      }
      else
      {
         hdst_im = state->mh_draw;
      }

      src_image = (KHRN_IMAGE_T *)mem_lock(hsrc_im);
      dst_image = (KHRN_IMAGE_T *)mem_lock(hdst_im);

      vcos_assert(src_image->mh_storage!=MEM_INVALID_HANDLE && src_image->offset == 0);
      vcos_assert(dst_image->mh_storage!=MEM_INVALID_HANDLE && dst_image->offset == 0);

      src_surf_id = create_surface(src_image);
      dst_surf_id = create_surface(dst_image);

      vcos_assert(src_surf_id != ~0);
      vcos_assert(dst_surf_id != ~0);

      {
         int x, y, xmax, ymax;
         x = 0;
         y = 0;
         xmax = dst_image->width;
         ymax = dst_image->height;
      
         x = _max(x, state->viewport.x);
         y = _max(y, state->viewport.y);
         xmax = _min(xmax, state->viewport.x + state->viewport.width);
         ymax = _min(ymax, state->viewport.y + state->viewport.height);
      
         if (state->caps.scissor_test)
         {
            //glScissor fix for bcmcompDraw
            state->scissor.y = dst_image->height - (state->scissor.y + state->scissor.height);
            x = _max(x, state->scissor.x);
            y = _max(y, state->scissor.y);
            xmax = _min(xmax, state->scissor.x + state->scissor.width);
            ymax = _min(ymax, state->scissor.y + state->scissor.height);
         }
         target_scissor.x = x;
         target_scissor.y = y;
         target_scissor.width = xmax-x;
         target_scissor.height = ymax-y;
      }

      su = texture->crop_rect.Ucr;
      sv = texture->height - texture->crop_rect.Vcr - texture->crop_rect.Hcr; /*comp api has inverted (wrt GL) y axis */
      sw = texture->crop_rect.Wcr;
      sh = texture->crop_rect.Hcr;

      dy = dst_image->height - dy - dh;

      if(sw < 0) /* horizontal reflection */
      {
         /*TODO set up horiz relf. flag*/
         su = su + sw;
         sw = -sw; 
      }

      if(sh < 0) /* vertical reflection */
      {
         sv = sv + sh;
         sh = -sh; 
      }

      if(texture->wrap.s == GL_REPEAT)
      {
         su = su % texture->width;
      }

      if(texture->wrap.t == GL_REPEAT)
      {
         sv = sv % texture->height;
      }

      vcos_assert(dw == sw && dh == sh);

      khrn_interlock_write_immediate(&dst_image->interlock);
      khrn_interlock_read_immediate(&src_image->interlock);
#if 1
      if(texture->wrap.s == GL_REPEAT && su+sw>texture->width)
      {
         if(texture->wrap.t == GL_REPEAT && sv+sh>texture->height)
         {
            /* four draws a top left, top right, bottom left, bottom right */
            add_rect(&comp[0], src_surf_id, su, sv, texture->width - su, texture->height - sv, dx, dy, texture->width - su, texture->height - sv);
            add_rect(&comp[1], src_surf_id, 0, sv, su + sw - texture->width, texture->height - sv, dx + texture->width - su, dy, su + sw - texture->width, texture->height - sv);
            add_rect(&comp[2], src_surf_id, su, 0, texture->width - su, sh + sv - texture->height, dx, dy + texture->height - sv, texture->width - su, sh + sv - texture->height);
            add_rect(&comp[3], src_surf_id, 0, 0, su + sw - texture->width, sh + sv - texture->height, dx + texture->width - su, dy + texture->height - sv, su + sw - texture->width, sh + sv - texture->height);
            comp[0].next=&comp[1], comp[1].next=&comp[2],comp[2].next=&comp[3],comp[3].next=0;
         }
         else{
            /* two draws a left and right */
            add_rect(&comp[0], src_surf_id, su, sv, texture->width - su, sh, dx, dy, texture->width - su, dh);
            add_rect(&comp[1], src_surf_id, 0, sv, su + sw - texture->width, sh, dx + texture->width - su, dy, su + sw - texture->width, dh);
            comp[0].next=&comp[1], comp[1].next=0;
         }
      }
      else
      {
         if(texture->wrap.t == GL_REPEAT && sv+sh>texture->height)
         {
            /* two draws a top and a bottom */
            add_rect(&comp[0], src_surf_id, su, sv, sw, texture->height - sv, dx, dy, dw, texture->height - sv);
            add_rect(&comp[1], src_surf_id, su, 0, sw, sh + sv - texture->height, dx, dy + texture->height - sv, dw, sh + sv - texture->height);
            comp[0].next=&comp[1], comp[1].next=0;
         }
         else{
            /* one */
            add_rect(&comp[0], src_surf_id, su, sv, sw, sh, dx, dy, dw, dh);
            comp[0].next=0;
         }
      }
#else
      add_rect(&comp[0], src_surf_id, su, sv, sw, sh, dx, dy, dw, dh);
      comp[0].next=0;
#endif

      /* bcmcompDrawAsync( dst_surf_id, */
      bcmcompDraw( dst_surf_id,
                0,
                &target_scissor,
                0,
                comp,
                0,
                lookup_surface_def);
                /*,signal_event, &draw_event); */

      destroy_surface(dst_surf_id);
      destroy_surface(src_surf_id);
      mem_unlock(hdst_im);
      mem_unlock(hsrc_im);
      mem_unlock(thandle);

   }

   return true;

use_slow:
   return false;/* try slow path  */
}
#endif

static void try_slow_path(GLXX_SERVER_STATE_T *state, GLfloat Xs, GLfloat Ys, GLfloat Zw, GLfloat Ws, GLfloat Hs)
{
   uint32_t i;
   GL11_TEXUNIT_T * texunit = NULL;
   MEM_HANDLE_T thandle = MEM_INVALID_HANDLE;
   GLXX_TEXTURE_T *texture = NULL;
   bool tex_ok = false;

   /* check textures are acceptable by V3D HW */
   /* all enabled textures are tformat or linear tile or 32bit, pot, 4k aligned raster */
   
   for (i = 0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++)
   {
      texunit = &state->texunits[i];

      thandle = state->bound_texture[i].mh_twod;

      if (texunit->enabled && thandle != MEM_INVALID_HANDLE) {
         KHRN_IMAGE_FORMAT_T format;
         texture = (GLXX_TEXTURE_T *)mem_lock(thandle);
         format = texture->format;

         if(ABGR_8888_RSO == texture->format &&
            texture->width == texture->height &&
            texture->width == 1 << _msb(texture->width) &&
            COMPLETE == glxx_texture_check_complete(texture, false))/* pot */
         {
            MEM_HANDLE_T himage = texture->mh_mipmaps[0][0];
            if(himage!=MEM_INVALID_HANDLE)
            {
               KHRN_IMAGE_T * image = (KHRN_IMAGE_T *)mem_lock(himage);
               MEM_HANDLE_T hstorage = image->mh_storage;
               if(hstorage!=MEM_INVALID_HANDLE)
               {
                  uint32_t pixels = (uint32_t)mem_lock(hstorage);
                  if(!(pixels & 0xfff))/* 4k aligned */
                  {
                     tex_ok = true;
                  }
                  mem_unlock(hstorage);
               }
               mem_unlock(himage);
            }
         }
         else if(khrn_image_is_tformat(texture->format) || khrn_image_is_lineartile(texture->format))
         {
            tex_ok = true;
         }

         mem_unlock(thandle);
         if(!tex_ok) /*found an enabled but not supported texture */
         {
            if(texture->format == ABGR_8888_RSO || texture->format == BGR_888_RSO)
            {
               /* call texture_image with null pixel pointer - should convert mipmaps back into tformat blob */
               tex_ok = glxx_texture_image(texture, GL_TEXTURE_2D, 0, texture->width, texture->height, 
                  texture->format == ABGR_8888_RSO ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, 4, 0);
            }
         }
            
      }
   }
// Application can call DrawTex for drawing a texture or color. Even if no active texture found, let the openGL draw a Color
//   if(tex_ok)
   {
      glxx_hw_draw_tex(state, Xs, Ys, Zw, Ws, Hs);
   }
}

void glDrawTexfOES_impl_11 (GLfloat Xs, GLfloat Ys, GLfloat Zs, GLfloat Ws, GLfloat Hs)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   vcos_assert(state);

   if(Ws <=0.0f || Hs <= 0.0f)
      glxx_server_state_set_error(state, GL_INVALID_VALUE);
   else
   {
      GLfloat Zw;
      GLclampf n, f;
      n = state->viewport.near;
      f = state->viewport.far;
      Zw = Zs <=0 ? n : Zs>=1 ? f : n + Zs * (f - n);

#ifdef USE_FAST_PATH
      if(!try_fast_path(state, Xs, Ys, Zw, Ws, Hs))
#endif
         try_slow_path(state, Xs, Ys, Zw, Ws, Hs);
   }   

   GL11_UNLOCK_SERVER_STATE();
}
