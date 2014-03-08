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

#include "middleware/khronos/glxx/2708/glxx_inner_4.h"

#include "middleware/khronos/common/2708/khrn_prod_4.h"
#include "middleware/khronos/common/2708/khrn_interlock_priv_4.h"
#include "middleware/khronos/common/2708/khrn_render_state_4.h"
#include "middleware/khronos/glxx/2708/glxx_inner_4.h"
#include "middleware/khronos/glxx/glxx_hw.h"

#include "middleware/khronos/gl11/2708/gl11_shader_4.h"
#include "middleware/khronos/gl11/2708/gl11_shadercache_4.h"
#include "middleware/khronos/gl11/2708/gl11_support_4.h"
#include "middleware/khronos/glxx/2708/glxx_tu_4.h"
#include "middleware/khronos/gl11/gl11_server.h"
#include "middleware/khronos/gl11/gl11_matrix.h"
#include "middleware/khronos/glxx/glxx_texture.h"
#include "helpers/vc_image/vc_image.h"           // For debugging

#include "middleware/khronos/glsl/glsl_common.h"
#include "middleware/khronos/glsl/glsl_dataflow.h"
#include "middleware/khronos/glsl/2708/glsl_allocator_4.h"


static int get_argument_count(GLenum combine)
{
   switch (combine) {
   case GL_REPLACE:
      return 1;
   case GL_MODULATE:
      return 2;
   case GL_ADD:
      return 2;
   case GL_ADD_SIGNED:
      return 2;
   case GL_INTERPOLATE:
      return 3;
   case GL_SUBTRACT:
      return 2;
   case GL_DOT3_RGB:
      return 2;
   case GL_DOT3_RGBA:
      return 2;
   default:
      UNREACHABLE();
      return 0;
   }
}

/*!
 * \brief Works out the texture unit properties and returns the number of varyings.
 *
 * This will evaluate the texture unit parameters for a given texture (associated with
 * a batch of triangles), set up some state and return the number of varyings to be
 *  expected.
 *
 * \param state is the OpenGL server state.
 * \param texture_coord is a set of texture coordinates.
 * \param point_sprites is a set of point sprites, if these are being used.
 */
bool gl11_hw_get_texunit_properties(GLXX_SERVER_STATE_T *state, GLXX_ATTRIB_T *texture_coord, uint32_t point_sprites)
{
   bool out_of_memory = false;

   int i, j;

   for (i = 0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++) {
      GL11_TEXUNIT_T *texunit = &state->texunits[i];
      GL11_CACHE_TEXUNIT_ABSTRACT_T *texabs = &state->shader.texunits[i];

      MEM_HANDLE_T thandle = state->bound_texture[i].mh_twod;

      memset(&texabs->props, 0, sizeof(texabs->props));
      memset(&texabs->rgb, 0, sizeof(texabs->rgb));
      memset(&texabs->alpha, 0, sizeof(texabs->alpha));

      if (texunit->enabled && thandle != MEM_INVALID_HANDLE) {
         GLXX_TEXTURE_T *texture = (GLXX_TEXTURE_T *)mem_lock(thandle);
         GLXX_TEXTURE_COMPLETENESS_T completeness = glxx_texture_check_complete(texture, false);
         KHRN_IMAGE_FORMAT_T format = glxx_texture_get_tformat(texture, false);

         switch (completeness) {
         case COMPLETE:
         {
            if (texabs->mode == GL_COMBINE)
            {
               int argc = _max(get_argument_count(texunit->alpha.combine), get_argument_count(texunit->rgb.combine));

               texabs->props.active = true;
               for (j = 0; j < argc; j++)
               {
                  texabs->alpha.source[j] = texunit->alpha.source[j];
                  texabs->alpha.operand[j] = texunit->alpha.operand[j];
                  texabs->rgb.source[j] = texunit->rgb.source[j];
                  texabs->rgb.operand[j] = texunit->rgb.operand[j];
               }

               texabs->alpha.combine = texunit->alpha.combine;
               texabs->rgb.combine = texunit->rgb.combine;
               texabs->alpha.scale = texunit->alpha.scale;
               texabs->rgb.scale = texunit->rgb.scale;
            }
            else
            {
               glxx_texture_has_color_alpha(texture, &texabs->props.has_color, &texabs->props.has_alpha, &texabs->props.active);
               vcos_assert(texabs->props.active);
            }

            if (point_sprites && texabs->coord_replace)
            {
               texabs->props.complex = 0;
            }
            else
            {
               texabs->props.complex = texture_coord[i].size == 4 || gl11_matrix_is_projective(texunit->stack.body[texunit->stack.pos]);
            }
            break;
         }
         case INCOMPLETE:
            texabs->props.active = false;
            break;
         case OUT_OF_MEMORY:
            texabs->props.active = false;
            out_of_memory = true;
            break;
         }

         mem_unlock(thandle);
      } else {
         texabs->props.active = false;
      }
   }

   return !out_of_memory;
}

void gl11_hw_setup_attribs_live(GLXX_SERVER_STATE_T *state,uint32_t *cattribs_live, uint32_t *vattribs_live)
{
   int i;

   *vattribs_live = 15<<(4*GL11_IX_VERTEX);

   if (!state->shader.lighting || state->shader.color_material) {
      *vattribs_live |= 15<<(4*GL11_IX_COLOR);
   }
   if (state->shader.lighting) {
      *vattribs_live |= 15<<(4*GL11_IX_NORMAL);
   }
   for (i = 0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++)
   {
      if (state->shader.texunits[i].props.active)
         *vattribs_live |= 15<<(4*(GL11_IX_TEXTURE_COORD+i));
   }
   if(state->batch.primitive_mode == GL_POINTS) {
      *vattribs_live |= 15<<(4*GL11_IX_POINT_SIZE);
   }
   *cattribs_live = *vattribs_live;
}
