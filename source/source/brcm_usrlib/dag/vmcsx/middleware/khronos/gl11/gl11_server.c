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

#include "middleware/khronos/gl11/gl11_server.h"
#include "middleware/khronos/glxx/glxx_server_internal.h"
#include "interface/khronos/include/GLES/glext.h"

#include "middleware/khronos/glxx/glxx_texture.h"
#include "middleware/khronos/glxx/glxx_buffer.h"
#include "interface/khronos/common/khrn_int_util.h"
#include "middleware/khronos/common/khrn_interlock.h"
#include "middleware/khronos/common/khrn_hw.h"

#include <string.h>
#include <math.h>
#include <limits.h>

#define LOG2E  1.442695f

#ifdef GL11_SERVER_SINGLE
GLXX_SERVER_STATE_T gl11_server_state;
#endif

#include "middleware/khronos/gl11/gl11_server_cr.c"

bool gl11_server_state_init(GLXX_SERVER_STATE_T *state, uint32_t name, uint64_t pid, MEM_HANDLE_T shared)
{
   int i;

   state->type = OPENGL_ES_11;

   //initialise common portions of state
   if(!glxx_server_state_init(state, name, pid, shared))
      return false;
   
   //gl 1.1 specific parts
   state->material.ambient[0] = 0.2f;
   state->material.ambient[1] = 0.2f;
   state->material.ambient[2] = 0.2f;
   state->material.ambient[3] = 1.0f;

   state->material.diffuse[0] = 0.8f;
   state->material.diffuse[1] = 0.8f;
   state->material.diffuse[2] = 0.8f;
   state->material.diffuse[3] = 1.0f;

   state->material.specular[0] = 0.0f;
   state->material.specular[1] = 0.0f;
   state->material.specular[2] = 0.0f;
   state->material.specular[3] = 1.0f;

   state->material.emission[0] = 0.0f;
   state->material.emission[1] = 0.0f;
   state->material.emission[2] = 0.0f;
   state->material.emission[3] = 1.0f;

   state->material.shininess = 0.0f;

   state->lightmodel.ambient[0] = 0.2f;
   state->lightmodel.ambient[1] = 0.2f;
   state->lightmodel.ambient[2] = 0.2f;
   state->lightmodel.ambient[3] = 1.0f;

   state->lightmodel.two_side = false;

   state->shader.two_side = false;

   for (i = 0; i < GL11_CONFIG_MAX_LIGHTS; i++) {
      GL11_LIGHT_T *light = &state->lights[i];

      light->enabled = false;

      light->ambient[0] = 0.0f;
      light->ambient[1] = 0.0f;
      light->ambient[2] = 0.0f;
      light->ambient[3] = 1.0f;

      light->diffuse[0] = i ? 0.0f : 1.0f;
      light->diffuse[1] = i ? 0.0f : 1.0f;
      light->diffuse[2] = i ? 0.0f : 1.0f;
      light->diffuse[3] = 1.0f;

      light->specular[0] = i ? 0.0f : 1.0f;
      light->specular[1] = i ? 0.0f : 1.0f;
      light->specular[2] = i ? 0.0f : 1.0f;
      light->specular[3] = 1.0f;

      light->position[0] = 0.0f;
      light->position[1] = 0.0f;
      light->position[2] = 1.0f;
      light->position[3] = 0.0f;

      light->attenuation.constant = 1.0f;
      light->attenuation.linear = 0.0f;
      light->attenuation.quadratic = 0.0f;

      light->spot.direction[0] = 0.0f;
      light->spot.direction[1] = 0.0f;
      light->spot.direction[2] = -1.0f;
      light->spot.exponent = 0.0f;
      light->spot.cutoff = 180.0f;

      light->position3[0] = 0.0f;
      light->position3[1] = 0.0f;
      light->position3[2] = 1.0f;

      light->cos_cutoff = -1.0f;
   }

   for (i = 0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++) {
      GL11_TEXUNIT_T *texunit = &state->texunits[i];
      GL11_CACHE_TEXUNIT_ABSTRACT_T *texabs = &state->shader.texunits[i];

      texunit->enabled = false;

      texabs->mode = GL_MODULATE;

      texunit->color[0] = 0.0f;
      texunit->color[1] = 0.0f;
      texunit->color[2] = 0.0f;
      texunit->color[3] = 0.0f;

      texunit->rgb.combine = GL_MODULATE;
      texunit->rgb.source[0] = GL_TEXTURE;
      texunit->rgb.source[1] = GL_PREVIOUS;
      texunit->rgb.source[2] = GL_CONSTANT;
      texunit->rgb.operand[0] = GL_SRC_COLOR;
      texunit->rgb.operand[1] = GL_SRC_COLOR;
      texunit->rgb.operand[2] = GL_SRC_ALPHA;
      texunit->rgb.scale = 1.0f;

      texunit->alpha.combine = GL_MODULATE;
      texunit->alpha.source[0] = GL_TEXTURE;
      texunit->alpha.source[1] = GL_PREVIOUS;
      texunit->alpha.source[2] = GL_CONSTANT;
      texunit->alpha.operand[0] = GL_SRC_ALPHA;
      texunit->alpha.operand[1] = GL_SRC_ALPHA;
      texunit->alpha.operand[2] = GL_SRC_ALPHA;
      texunit->alpha.scale = 1.0f;

      texabs->coord_replace = false;

      gl11_matrix_stack_init(&texunit->stack);
   }

   state->fog.mode = GL_EXP;

   state->fog.color[0] = 0.0f;
   state->fog.color[1] = 0.0f;
   state->fog.color[2] = 0.0f;
   state->fog.color[3] = 0.0f;

   state->fog.density = 1.0f;
   state->fog.start = 0.0f;
   state->fog.end = 1.0f;

   state->fog.scale = 1.0f;
   state->fog.coeff_exp = -LOG2E;
   state->fog.coeff_exp2 = -LOG2E;

   state->shader.normalize = false;
   state->shader.rescale_normal = false;
   state->shader.lighting = false;
   state->shader.color_material = false;

   state->caps_fragment.fog = false;
   state->caps_fragment.alpha_test = false;
   state->caps_fragment.point_smooth = false;
   state->caps_fragment.point_sprite = false;
   state->shader.line_smooth = false;

   state->hints_program.fog = GL_DONT_CARE;

   for (i = 0; i < GL11_CONFIG_MAX_PLANES; i++) {
      state->planes[i][0] = 0.0f;
      state->planes[i][1] = 0.0f;
      state->planes[i][2] = 0.0f;
      state->planes[i][3] = 0.0f;
   }
   
   for (i = 0; i < GL11_CONFIG_MAX_PLANES; i++)
      state->caps.clip_plane[i] = false;

   state->shade_model = GL_SMOOTH;

   state->logic_op = GL_COPY;
   
   state->matrix_mode = GL_MODELVIEW;

   gl11_matrix_stack_init(&state->modelview);
   gl11_matrix_stack_init(&state->projection);

   state->alpha_func.func = GL_ALWAYS;
   state->alpha_func.ref = 0.0f;
   
   state->caps.sample_alpha_to_one = GL_FALSE;
   state->caps.color_logic_op = GL_FALSE;
   
   state->hints.perspective_correction = GL_DONT_CARE;
   state->hints.point_smooth = GL_DONT_CARE;
   state->hints.line_smooth = GL_DONT_CARE;

   state->point_params.size_min = 1.0f;
   state->point_params.size_min_clamped = 1.0f;
   state->point_params.size_max = 256.0f;
   state->point_params.fade_threshold = 1.0f;
   state->point_params.distance_attenuation[0] = 1.0f;
   state->point_params.distance_attenuation[1] = 0.0f;
   state->point_params.distance_attenuation[2] = 0.0f;

   state->copy_of_color[0] = 1.0f;
   state->copy_of_color[1] = 1.0f;
   state->copy_of_color[2] = 1.0f;
   state->copy_of_color[3] = 1.0f;

   return true;
}

/*
void glPointSize_impl_11 (GLfloat size)
{
}
*/


////

int glGetTexParameterxv_impl_11 (GLenum target, GLenum pname, GLfixed *params)
{
   GLint temp[4];
   GLuint count = glxx_get_texparameter_internal(target, pname, temp);

   if (count) {
      int i;
      vcos_assert(count == 1 || count == 4);
      for(i=0;i<count;i++)
         params[i] = (GLfixed)temp[i];
   }

   return count;
}

void glTexParameterx_impl_11 (GLenum target, GLenum pname, GLfixed param)
{
   GLint iparam = (GLint)param;         // no scaling for enum to fixed
   glxx_texparameter_internal(target, pname, &iparam);
}


void glTexParameterxv_impl_11 (GLenum target, GLenum pname, const GLfixed *params)
{
   if (params)
   {
      if(pname == GL_TEXTURE_CROP_RECT_OES) {
         GLint iparams[4];
         int i;
         for(i=0;i<4;i++)
            iparams[i] = (GLint)params[i];     // no scaling for enum to fixed
         glxx_texparameter_internal(target, pname, iparams);
      }
      else {
         GLint iparam = (GLint)params[0];     // no scaling for enum to fixed
         glxx_texparameter_internal(target, pname, &iparam);
      }
   }
}





/*
void glMultiTexCoord4f_impl_11 (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
}

void glNormal3f_impl_11 (GLfloat nx, GLfloat ny, GLfloat nz)
{
}
*/
static void point_parameterv_internal(GLenum pname, const GLfloat *params)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   switch (pname) {
   case GL_POINT_SIZE_MIN:
   {
      GLfloat m = clean_float(params[0]);

      if (m >= 0)
      {
         state->point_params.size_min = m;
         state->point_params.size_min_clamped = _maxf(1.0f, state->point_params.size_min);
      }
      else
         glxx_server_state_set_error(state, GL_INVALID_VALUE);

      break;
   }
   case GL_POINT_SIZE_MAX:
   {
      GLfloat m = clean_float(params[0]);

      if (m >= 0)
         state->point_params.size_max = m;
      else
         glxx_server_state_set_error(state, GL_INVALID_VALUE);

      break;
   }
   case GL_POINT_FADE_THRESHOLD_SIZE:
   {
      GLfloat m = clean_float(params[0]);

      if (m >= 0)
         state->point_params.fade_threshold = m;
      else
         glxx_server_state_set_error(state, GL_INVALID_VALUE);

      break;
   }
   case GL_POINT_DISTANCE_ATTENUATION:
   {
      int i;
      for (i = 0; i < 3; i++)
         state->point_params.distance_attenuation[i] = clean_float(params[i]);

      break;
   }
   default:
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
      break;
   }

   GL11_UNLOCK_SERVER_STATE();
}

void glPointParameterf_impl_11 (GLenum pname, GLfloat param)
{
   GLfloat params[] = {0.0f, 0.0f, 0.0f};

   params[0] = param;

   point_parameterv_internal(pname, params);
}

void glPointParameterfv_impl_11 (GLenum pname, const GLfloat *params)
{
   point_parameterv_internal(pname, params);
}


static GLboolean is_texture_function(GLenum mode)
{
   return mode == GL_REPLACE ||
          mode == GL_MODULATE ||
          mode == GL_DECAL ||
          mode == GL_BLEND ||
          mode == GL_ADD ||
          mode == GL_COMBINE;
}

static GLboolean is_combine_rgb(GLenum combine)
{
   return combine == GL_REPLACE ||
          combine == GL_MODULATE ||
          combine == GL_ADD ||
          combine == GL_ADD_SIGNED ||
          combine == GL_INTERPOLATE ||
          combine == GL_SUBTRACT ||
          combine == GL_DOT3_RGB ||
          combine == GL_DOT3_RGBA;
}

static GLboolean is_combine_alpha(GLenum combine)
{
   return combine == GL_REPLACE ||
          combine == GL_MODULATE ||
          combine == GL_ADD ||
          combine == GL_ADD_SIGNED ||
          combine == GL_INTERPOLATE ||
          combine == GL_SUBTRACT;
}

static GLboolean is_source(GLenum source)
{
   return source == GL_TEXTURE ||
          source == GL_CONSTANT ||
          source == GL_PRIMARY_COLOR ||
          source == GL_PREVIOUS;
}

static GLboolean is_operand_rgb(GLenum operand)
{
   return operand == GL_SRC_COLOR ||
          operand == GL_ONE_MINUS_SRC_COLOR ||
          operand == GL_SRC_ALPHA ||
          operand == GL_ONE_MINUS_SRC_ALPHA;
}

static GLboolean is_operand_alpha(GLenum operand)
{
   return operand == GL_SRC_ALPHA ||
          operand == GL_ONE_MINUS_SRC_ALPHA;
}

static GLboolean is_scalef(GLfloat scale)
{
   return scale == 1.0f ||
          scale == 2.0f ||
          scale == 4.0f;
}

static void texenvfv_internal(GLenum target, GLenum pname, const GLfloat *params)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   int t = state->active_texture - GL_TEXTURE0;

   switch (target) {
   case GL_POINT_SPRITE_OES:
      switch (pname) {
      case GL_COORD_REPLACE_OES:
         state->changed_texunit = true;
         state->shader.texunits[t].coord_replace = (params[0] != 0.0f);
         break;
      default:
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
         break;
      }
      break;

   case GL_TEXTURE_ENV:
      switch (pname) {
      case GL_TEXTURE_ENV_MODE:
      {
         GLenum mode = (GLenum)params[0];

         if (is_texture_function(mode))
         {
            state->changed_texunit = true;
            state->shader.texunits[t].mode = mode;
         }
         else
            glxx_server_state_set_error(state, GL_INVALID_ENUM);

         break;
      }
      case GL_TEXTURE_ENV_COLOR:
         {
            int i;
            state->changed_texunit = true;
            for (i = 0; i < 4; i++)
               state->texunits[t].color[i] = clampf(params[i], 0.0f, 1.0f);
         }
         break;
      case GL_COMBINE_RGB:
      {
         GLenum combine = (GLenum)params[0];

         if (is_combine_rgb(combine))
         {
            state->changed_texunit = true;
            state->texunits[t].rgb.combine = combine;
         }
         else
            glxx_server_state_set_error(state, GL_INVALID_ENUM);

         break;
      }
      case GL_RGB_SCALE:
      {
         if (is_scalef(params[0]))
         {
            state->changed_texunit = true;
            state->texunits[t].rgb.scale = params[0];
         }
         else
            glxx_server_state_set_error(state, GL_INVALID_VALUE);

         break;
      }
      case GL_SRC0_RGB:
      case GL_SRC1_RGB:
      case GL_SRC2_RGB:
      {
         GLenum source = (GLenum)params[0];

         if (is_source(source))
         {
            state->changed_texunit = true;
            state->texunits[t].rgb.source[pname - GL_SRC0_RGB] = source;
         }
         else
            glxx_server_state_set_error(state, GL_INVALID_ENUM);

         break;
      }
      case GL_OPERAND0_RGB:
      case GL_OPERAND1_RGB:
      case GL_OPERAND2_RGB:
      {
         GLenum operand = (GLenum)params[0];

         if (is_operand_rgb(operand))
         {
            state->changed_texunit = true;
            state->texunits[t].rgb.operand[pname - GL_OPERAND0_RGB] = operand;
         }
         else
            glxx_server_state_set_error(state, GL_INVALID_ENUM);

         break;
      }
      case GL_COMBINE_ALPHA:
      {
         GLenum combine = (GLenum)params[0];

         if (is_combine_alpha(combine))
         {
            state->changed_texunit = true;
            state->texunits[t].alpha.combine = combine;
         }
         else
            glxx_server_state_set_error(state, GL_INVALID_ENUM);

         break;
      }
      case GL_ALPHA_SCALE:
      {
         if (is_scalef(params[0]))
         {
            state->changed_texunit = true;
            state->texunits[t].alpha.scale = params[0];
         }
         else
            glxx_server_state_set_error(state, GL_INVALID_VALUE);

         break;
      }
      case GL_SRC0_ALPHA:
      case GL_SRC1_ALPHA:
      case GL_SRC2_ALPHA:
      {
         GLenum source = (GLenum)params[0];

         if (is_source(source))
         {
            state->changed_texunit = true;
            state->texunits[t].alpha.source[pname - GL_SRC0_ALPHA] = source;
         }
         else
            glxx_server_state_set_error(state, GL_INVALID_ENUM);

         break;
      }
      case GL_OPERAND0_ALPHA:
      case GL_OPERAND1_ALPHA:
      case GL_OPERAND2_ALPHA:
      {
         GLenum operand = (GLenum)params[0];

         if (is_operand_alpha(operand))
         {
            state->changed_texunit = true;
            state->texunits[t].alpha.operand[pname - GL_OPERAND0_ALPHA] = operand;
         }
         else
            glxx_server_state_set_error(state, GL_INVALID_ENUM);

         break;
      }
      default:
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
         break;
      }
      break;

   default:
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
      break;
   }

   GL11_UNLOCK_SERVER_STATE();
}

void glTexEnvf_impl_11 (GLenum target, GLenum pname, GLfloat param)
{
   GLfloat params[] = {0.0f, 0.0f, 0.0f, 0.0f};

   params[0] = param;

   texenvfv_internal(target, pname, params);
}

void glTexEnvfv_impl_11 (GLenum target, GLenum pname, const GLfloat *params)
{
   texenvfv_internal(target, pname, params);
}






/*
void glDisableClientState_impl_11 (GLenum array)
{
}
*/



/*
void glEnableClientState_impl_11 (GLenum array)
{
}
*/

/*
   glLightf_impl_11 (GLenum light, GLenum pname, GLfloat param)
   glLightfv_impl_11 (GLenum light, GLenum pname, const GLfloat *params)
   glLightx_impl_11 (GLenum light, GLenum pname, GLfixed param)
   glLightxv_impl_11 (GLenum light, GLenum pname, const GLfixed *params)

   Khronos documentation:

   Lighting parameters are divided into three categories: material parameters, light
   source parameters, and lighting model parameters (see Table 2.8). Sets of lighting
   parameters are specified with
            :  :  :  :
      void Light{xf}( enum light, enum pname, T param );
      void Light{xf}v( enum light, enum pname, T params );

   In the vector versions of the commands, params is a pointer to a group
   of values to which to set the indicated parameter. The number of values pointed to
   depends on the parameter being set. In the non-vector versions, param is a value
   to which to set a single-valued parameter. (If param corresponds to a multi-valued
   parameter, the error INVALID ENUM results.) In the case of Light, light is a symbolic
   constant of the form LIGHTi, indicating that light i is to have the specified parameter
   set. The constants obey LIGHTi = LIGHT0 + i.

   The current model-view matrix is applied to the position parameter indicated
   with Light for a particular light source when that position is specified. These
   transformed values are the values used in the lighting equation.

   The spotlight direction is transformed when it is specified using only the upper
   leftmost 3x3 portion of the model-view matrix

   Lighting Parameters

      AMBIENT 4
      DIFFUSE 4
      SPECULAR 4
      POSITION 4
      SPOT DIRECTION 3
      SPOT EXPONENT 1
      SPOT CUTOFF 1
      CONSTANT ATTENUATION 1
      LINEAR ATTENUATION 1
      QUADRATIC ATTENUATION 1

   Implementation notes:

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   For vector variants, params be a valid pointer to 4 elements

   Postconditions:

   If pname is not a valid light parameter or is a vector-valued light parameter
   and a scalar function has been used or light is invalid, and no current error,
   error becomes GL_INVALID_ENUM

   Invariants preserved:
*/

static void lightv_internal(GLenum l, GLenum pname, const GLfloat *params)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   GL11_LIGHT_T *light = get_light(state, l);

   if (light)
      switch (pname) {
      case GL_AMBIENT:
      {
         int i;
         for (i = 0; i < 4; i++)
            light->ambient[i] = clean_float(params[i]);
         break;
      }
      case GL_DIFFUSE:
      {
         int i;
         for (i = 0; i < 4; i++)
            light->diffuse[i] = clean_float(params[i]);
         break;
      }
      case GL_SPECULAR:
      {
         int i;
         for (i = 0; i < 4; i++)
            light->specular[i] = clean_float(params[i]);
         break;
      }
      case GL_POSITION:
      {
         GLfloat clean[4];

         int i;

         for(i = 0; i < 4; i++)
            clean[i] = clean_float(params[i]);

         gl11_matrix_mult_col(light->position, state->modelview.body[state->modelview.pos], clean);

         /*
            compute 3-vector position of the light for use by the HAL
         */

         state->changed_light = true;  /* Shader changes if position[3]==0 */
         if (light->position[3] != 0.0f)
            for (i = 0; i < 3; i++)
               light->position3[i] = light->position[i] / light->position[3];

         break;
      }
      case GL_SPOT_DIRECTION:
      {
         int i;
         GLfloat clean[4];

         for(i = 0; i < 3; i++)
            clean[i] = clean_float(params[i]);

         clean[3] = 0.0f;

         gl11_matrix_mult_col(light->spot.direction, state->modelview.body[state->modelview.pos], clean);
         break;
      }
      case GL_SPOT_EXPONENT:
         light->spot.exponent = clean_float(params[0]);
         break;
      case GL_SPOT_CUTOFF:
         state->changed_light = true;  /* Shader changes if cutoff==180 */
         light->spot.cutoff = clean_float(params[0]);

         light->cos_cutoff = (GLfloat)cos(2.0f * PI * light->spot.cutoff / 360.0f);        // derived value
         break;
      case GL_CONSTANT_ATTENUATION:
         light->attenuation.constant = clean_float(params[0]);
         break;
      case GL_LINEAR_ATTENUATION:
         light->attenuation.linear = clean_float(params[0]);
         break;
      case GL_QUADRATIC_ATTENUATION:
         light->attenuation.quadratic = clean_float(params[0]);
         break;
      default:
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
         break;
      }

   GL11_UNLOCK_SERVER_STATE();
}

void glLightf_impl_11 (GLenum light, GLenum pname, GLfloat param)
{
   GLfloat params[] = {0.0f, 0.0f, 0.0f, 0.0f};

   params[0] = param;

   lightv_internal(light, pname, params);
}

void glLightfv_impl_11 (GLenum light, GLenum pname, const GLfloat *params)
{
   lightv_internal(light, pname, params);
}

void glLightx_impl_11 (GLenum light, GLenum pname, GLfixed param)
{
   GLfloat params[] = {0.0f, 0.0f, 0.0f, 0.0f};

   params[0] = fixed_to_float(param);

   lightv_internal(light, pname, params);
}

void glLightxv_impl_11 (GLenum light, GLenum pname, const GLfixed *params)
{
   int i;
   GLfloat temp[4];

   for (i = 0; i < 4; i++)
      temp[i] = fixed_to_float(params[i]);

   lightv_internal(light, pname, temp);
}


static GLboolean is_logic_op(GLenum op)
{
   return op == GL_CLEAR ||
          op == GL_AND ||
          op == GL_AND_REVERSE ||
          op == GL_COPY ||
          op == GL_AND_INVERTED ||
          op == GL_NOOP ||
          op == GL_XOR ||
          op == GL_OR ||
          op == GL_NOR ||
          op == GL_EQUIV ||
          op == GL_INVERT ||
          op == GL_OR_REVERSE ||
          op == GL_COPY_INVERTED ||
          op == GL_OR_INVERTED ||
          op == GL_NAND ||
          op == GL_SET;
}

void glLogicOp_impl_11 (GLenum opcode)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   if (is_logic_op(opcode))
   {
      state->changed_backend = true;
      state->logic_op = opcode;
   }
   else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GL11_UNLOCK_SERVER_STATE();
}

static GLboolean is_matrix_mode(GLenum mode)
{
   return mode == GL_TEXTURE ||
          mode == GL_MODELVIEW ||
          mode == GL_PROJECTION;
}

void glMatrixMode_impl_11 (GLenum mode)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   if (is_matrix_mode(mode))
      state->matrix_mode = mode;
   else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GL11_UNLOCK_SERVER_STATE();
}

/*
   void glMaterialf_impl_11 (GLenum face, GLenum pname, GLfloat param)
   void glMaterialfv_impl_11 (GLenum face, GLenum pname, const GLfloat *params)
   void glMaterialx_impl_11 (GLenum face, GLenum pname, GLfixed param)
   void glMaterialxv_impl_11 (GLenum face, GLenum pname, const GLfixed *params)

   Khronos documentation:

   Lighting parameters are divided into three categories: material parameters, light
   source parameters, and lighting model parameters (see Table 2.8). Sets of lighting
   parameters are specified with
            :  :  :  :
      void Material{xf}( enum face, enum pname, T param );
      void Material{xf}v( enum face, enum pname, T params );

   Lighting Model Parameters

      AMBIENT 4
      DIFFUSE 4
      AMBIENT AND DIFFUSE 4
      SPECULAR 4
      EMISSION 4
      SHININESS 1

   Implementation notes:

   The range of light, lightmodel and material color parameters is -INF to INF (i.e. unclamped)

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   For vector variants, params be a valid pointer to 4 elements

   Postconditions:

   If pname is not a valid material parameter and no current error, error becomes GL_INVALID_ENUM

   Invariants preserved:
*/

static void materialv_internal (GLenum face, GLenum pname, const GLfloat *params)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   if (face == GL_FRONT_AND_BACK) {
      switch (pname) {
      case GL_AMBIENT:
         if (!state->shader.color_material) {
            int i;
            for (i = 0; i < 4; i++)
               state->material.ambient[i] = clean_float(params[i]);
         }
         break;
      case GL_DIFFUSE:
      {
         if (!state->shader.color_material) {
            int i;
            for (i = 0; i < 4; i++)
               state->material.diffuse[i] = clean_float(params[i]);
         }
         break;
      }
      case GL_AMBIENT_AND_DIFFUSE:
      {
         if (!state->shader.color_material) {
            int i;
            for (i = 0; i < 4; i++) {
               GLfloat f = clean_float(params[i]);

               state->material.ambient[i] = f;
               state->material.diffuse[i] = f;
            }
         }
         break;
      }
      case GL_SPECULAR:
      {
         int i;
         for (i = 0; i < 4; i++)
            state->material.specular[i] = clean_float(params[i]);
         break;
      }
      case GL_EMISSION:
      {
         int i;
         for (i = 0; i < 4; i++)
            state->material.emission[i] = clean_float(params[i]);
         break;
      }
      case GL_SHININESS:
         state->material.shininess = clean_float(params[0]);
         break;
      default:
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
         break;
      }
   } else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GL11_UNLOCK_SERVER_STATE();
}

void glMaterialf_impl_11 (GLenum face, GLenum pname, GLfloat param)
{
   GLfloat params[] = {0.0f, 0.0f, 0.0f, 0.0f};

   params[0] = param;

   materialv_internal(face, pname, params);
}

void glMaterialfv_impl_11 (GLenum face, GLenum pname, const GLfloat *params)
{
   materialv_internal(face, pname, params);
}

void glMaterialx_impl_11 (GLenum face, GLenum pname, GLfixed param)
{
   GLfloat params[] = {0.0f, 0.0f, 0.0f, 0.0f};

   params[0] = fixed_to_float(param);

   materialv_internal(face, pname, params);
}

void glMaterialxv_impl_11 (GLenum face, GLenum pname, const GLfixed *params)
{
   int i;
   GLfloat temp[4];

   for (i = 0; i < 4; i++)
      temp[i] = fixed_to_float(params[i]);

   materialv_internal(face, pname, temp);
}

/*
void glMultiTexCoord4x_impl_11 (GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q)
{
}

void glNormal3x_impl_11 (GLfixed nx, GLfixed ny, GLfixed nz)
{
}
*/

void glPointParameterx_impl_11 (GLenum pname, GLfixed param)
{
   GLfloat params[] = {0.0f, 0.0f, 0.0f};

   params[0] = fixed_to_float(param);

   point_parameterv_internal(pname, params);
}

void glPointParameterxv_impl_11 (GLenum pname, const GLfixed *params)
{
   int i;
   GLfloat temp[3];

   for (i = 0; i < 3; i++)
      temp[i] = fixed_to_float(params[i]);

   point_parameterv_internal(pname, temp);
}
/*
void glPointSizex_impl_11 (GLfixed size)
{
}
*/


void glSampleCoveragex_impl_11 (GLclampx value, GLboolean invert)
{
   glxx_sample_coverage_internal(fixed_to_float(value), invert);
}


static GLboolean is_shade_model(GLenum model)
{
   return model == GL_SMOOTH ||
          model == GL_FLAT;
}

void glShadeModel_impl_11 (GLenum model)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   if (is_shade_model(model))
      state->shade_model = model;
   else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GL11_UNLOCK_SERVER_STATE();
}

static GLboolean texenv_requires_scaling(GLenum pname)
{
   return pname == GL_TEXTURE_ENV_COLOR ||
          pname == GL_RGB_SCALE ||
          pname == GL_ALPHA_SCALE;
}

void glTexEnvi_impl_11 (GLenum target, GLenum pname, GLint param)
{
   float params[] = {0.0f, 0.0f, 0.0f, 0.0f};

   params[0] = (float)param;

   texenvfv_internal(target, pname, params);
}

void glTexEnvx_impl_11 (GLenum target, GLenum pname, GLfixed param)
{
   float params[] = {0.0f, 0.0f, 0.0f, 0.0f};

   params[0] = texenv_requires_scaling(pname) ? fixed_to_float(param) : (float)param;

   texenvfv_internal(target, pname, params);
}

/*
TODO: slightly annoying that we convert enum parameters to float and back.
But at least it avoids duplicated code.
*/
void glTexEnviv_impl_11 (GLenum target, GLenum pname, const GLint *params)
{
   int i;
   float temp[4];

   for (i = 0; i < 4; i++)
      temp[i] = (float)params[i];

   texenvfv_internal(target, pname, temp);
}

void glTexEnvxv_impl_11 (GLenum target, GLenum pname, const GLfixed *params)
{
   int i;
   float temp[4];

   for (i = 0; i < 4; i++)
      if (texenv_requires_scaling(pname))
         temp[i] = fixed_to_float(params[i]);
      else
         temp[i] = (float)params[i];

   texenvfv_internal(target, pname, temp);
}

int glGetTexEnvxv_impl_11 (GLenum env, GLenum pname, GLfixed *params)
{
   int i;
   float temp[4];

   int count = get_texenv_float_or_fixed_internal(env, pname, temp);

   vcos_assert(count <= 4);

   for (i = 0; i < count; i++)
      if (texenv_requires_scaling(pname))
         params[i] = float_to_fixed(temp[i]);
      else
         params[i] = (GLfixed)temp[i];

   return count;
}



//jeremyt 30/3/2010 glColorPointer_impl_11 moved back from server_cr.c
/*
   void glColorPointer_impl_11 ()
   
   Khronos documentation:

   Blocks of vertex array data may be stored in buffer objects with the same format
   and layout options supported for client-side vertex arrays.

   The client state associated with each vertex array type includes a buffer object
   binding point. The commands that specify the locations and organizations of vertex
   arrays copy the buffer object name that is bound to ARRAY BUFFER to the binding
   point corresponding to the vertex array of the type being specified. For example,
   the NormalPointer command copies the value of ARRAY BUFFER BINDING (the
   queriable name of the buffer binding corresponding to the target ARRAY BUFFER)
   to the client state variable NORMAL ARRAY BUFFER BINDING.

   Implementation notes:

   We could snapshot the index of the currently bound buffer object at ColorPointer time
   on the client side, and then pass this down with every DrawElements or DrawArrays call.
   However it is perfectly legal to bind a buffer as the backing for the vertex color array,
   delete the buffer FROM ANOTHER EGL CONTEXT SHARING WITH THIS ONE, and then expect to
   continue using the buffer.

   For this reason, we snapshot the handle on the server side instead.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context

   Postconditions:   

   -

   Invariants preserved:

   (GL11_SERVER_STATE_BOUND_COLOR_ARRAY) mh_color_array is either MEM_INVALID_HANDLE or a valid handle to a GLXX_BUFFER_T
*/

void glColorPointer_impl_11 ()
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   MEM_ASSIGN(state->bound_buffer.mh_attrib_array[GL11_IX_COLOR], state->bound_buffer.mh_array);
   
   GL11_UNLOCK_SERVER_STATE();
}

//jeremyt 30/3/2010 glNormalPointer_impl_11 moved back from server_cr.c
/*
   void glNormalPointer_impl_11 ()
   
   Khronos documentation:

   Blocks of vertex array data may be stored in buffer objects with the same format
   and layout options supported for client-side vertex arrays.

   The client state associated with each vertex array type includes a buffer object
   binding point. The commands that specify the locations and organizations of vertex
   arrays copy the buffer object name that is bound to ARRAY BUFFER to the binding
   point corresponding to the vertex array of the type being specified. For example,
   the NormalPointer command copies the value of ARRAY BUFFER BINDING (the
   queriable name of the buffer binding corresponding to the target ARRAY BUFFER)
   to the client state variable NORMAL ARRAY BUFFER BINDING.

   Implementation notes:

   We could snapshot the index of the currently bound buffer object at NormalPointer time
   on the client side, and then pass this down with every DrawElements or DrawArrays call.
   However it is perfectly legal to bind a buffer as the backing for the vertex normal array,
   delete the buffer FROM ANOTHER EGL CONTEXT SHARING WITH THIS ONE, and then expect to
   continue using the buffer.

   For this reason, we snapshot the handle on the server side instead.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context

   Postconditions:   

   -

   Invariants preserved:

   (GL11_SERVER_STATE_BOUND_NORMAL_ARRAY) mh_normal_array is either MEM_INVALID_HANDLE or a valid handle to a GLXX_BUFFER_T
*/

void glNormalPointer_impl_11 ()
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   MEM_ASSIGN(state->bound_buffer.mh_attrib_array[GL11_IX_NORMAL], state->bound_buffer.mh_array);

   GL11_UNLOCK_SERVER_STATE();
}

//jeremyt 30/3/2010 glVertexPointer_impl_11 moved back from server_cr.c
/*
   void glVertexPointer_impl_11 ()
   
   Khronos documentation:

   Blocks of vertex array data may be stored in buffer objects with the same format
   and layout options supported for client-side vertex arrays.

   The client state associated with each vertex array type includes a buffer object
   binding point. The commands that specify the locations and organizations of vertex
   arrays copy the buffer object name that is bound to ARRAY BUFFER to the binding
   point corresponding to the vertex array of the type being specified. For example,
   the NormalPointer command copies the value of ARRAY BUFFER BINDING (the
   queriable name of the buffer binding corresponding to the target ARRAY BUFFER)
   to the client state variable NORMAL ARRAY BUFFER BINDING.

   Implementation notes:

   We could snapshot the index of the currently bound buffer object at VertexPointer time
   on the client side, and then pass this down with every DrawElements or DrawArrays call.
   However it is perfectly legal to bind a buffer as the backing for the vertex array,
   delete the buffer FROM ANOTHER EGL CONTEXT SHARING WITH THIS ONE, and then expect to
   continue using the buffer.

   For this reason, we snapshot the handle on the server side instead.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context

   Postconditions:   

   -

   Invariants preserved:

   (GL11_SERVER_STATE_BOUND_VERTEX_ARRAY) mh_vertex_array is either MEM_INVALID_HANDLE or a valid handle to a GLXX_BUFFER_T
*/


void glVertexPointer_impl_11 ()
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   MEM_ASSIGN(state->bound_buffer.mh_attrib_array[GL11_IX_VERTEX], state->bound_buffer.mh_array);

   GL11_UNLOCK_SERVER_STATE();
}

//jeremyt 30/3/2010 glTexCoordPointer_impl_11 moved back from server_cr.c
/*
   void glTexCoordPointer_impl_11 (GLenum unit)
   
   Khronos documentation:

   Blocks of vertex array data may be stored in buffer objects with the same format
   and layout options supported for client-side vertex arrays.

   The client state associated with each vertex array type includes a buffer object
   binding point. The commands that specify the locations and organizations of vertex
   arrays copy the buffer object name that is bound to ARRAY BUFFER to the binding
   point corresponding to the vertex array of the type being specified. For example,
   the NormalPointer command copies the value of ARRAY BUFFER BINDING (the
   queriable name of the buffer binding corresponding to the target ARRAY BUFFER)
   to the client state variable NORMAL ARRAY BUFFER BINDING.

   Implementation notes:

   We could snapshot the index of the currently bound buffer object at TexCoordPointer time
   on the client side, and then pass this down with every DrawElements or DrawArrays call.
   However it is perfectly legal to bind a buffer as the backing for the vertex texture coord array,
   delete the buffer FROM ANOTHER EGL CONTEXT SHARING WITH THIS ONE, and then expect to
   continue using the buffer.

   For this reason, we snapshot the handle on the server side instead.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   0 <= unit-GL_TEXTURE0 < GL11_CONFIG_MAX_TEXTURE_UNITS 

   Postconditions:   

   -

   Invariants preserved:

   (GL11_SERVER_STATE_BOUND_TEXTURE_COORD_ARRAY) each element of mh_texture_coord_array is either MEM_INVALID_HANDLE or a valid handle to a GLXX_BUFFER_T
*/

void glTexCoordPointer_impl_11 (GLenum unit)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   vcos_assert(unit >= GL_TEXTURE0);
   vcos_assert(unit < GL_TEXTURE0 + GL11_CONFIG_MAX_TEXTURE_UNITS);

   MEM_ASSIGN(state->bound_buffer.mh_attrib_array[unit - GL_TEXTURE0 + GL11_IX_TEXTURE_COORD], state->bound_buffer.mh_array);

   GL11_UNLOCK_SERVER_STATE();
}

//jeremyt 30/3/2010 glPointSizePointerOES_impl_11 moved back from server_cr.c
/*
   void glPointSizePointerOES_impl_11 ()
   
   Khronos documentation:

   Blocks of vertex array data may be stored in buffer objects with the same format
   and layout options supported for client-side vertex arrays.

   The client state associated with each vertex array type includes a buffer object
   binding point. The commands that specify the locations and organizations of vertex
   arrays copy the buffer object name that is bound to ARRAY BUFFER to the binding
   point corresponding to the vertex array of the type being specified. For example,
   the NormalPointer command copies the value of ARRAY BUFFER BINDING (the
   queriable name of the buffer binding corresponding to the target ARRAY BUFFER)
   to the client state variable NORMAL ARRAY BUFFER BINDING.

   Implementation notes:

   We could snapshot the index of the currently bound buffer object at PointSizePointerOES time
   on the client side, and then pass this down with every DrawElements or DrawArrays call.
   However it is perfectly legal to bind a buffer as the backing for the vertex point size array,
   delete the buffer FROM ANOTHER EGL CONTEXT SHARING WITH THIS ONE, and then expect to
   continue using the buffer.

   For this reason, we snapshot the handle on the server side instead.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context

   Postconditions:   

   -

   Invariants preserved:

   (GL11_SERVER_STATE_BOUND_POINT_SIZE_ARRAY) mh_point_size_array is either MEM_INVALID_HANDLE or a valid handle to a GLXX_BUFFER_T
*/

void glPointSizePointerOES_impl_11 ()
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   MEM_ASSIGN(state->bound_buffer.mh_attrib_array[GL11_IX_POINT_SIZE], state->bound_buffer.mh_array);

   GL11_UNLOCK_SERVER_STATE();
}

/*
   "It is possible to attach the ambient and diffuse material properties to the current
color, so that they continuously track its component values.
Color material tracking is enabled and disabled by calling Enable or Disable
with the symbolic value COLOR MATERIAL. When enabled, both the ambient (acm)
and diffuse (dcm) properties of both the front and back material are immediately
set to the value of the current color, and will track changes to the current color
resulting from either the Color commands or drawing vertex arrays with the color
array enabled.
The replacements made to material properties are permanent; the replaced values
remain until changed by either sending a new color or by setting a new material
value when COLOR MATERIAL is not currently enabled, to override that particular
value."

   TODO: it is irritating that we have to do it this way
*/

void glintColor_impl_11(float red, float green, float blue, float alpha)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   state->copy_of_color[0] = red;
   state->copy_of_color[1] = green;
   state->copy_of_color[2] = blue;
   state->copy_of_color[3] = alpha;

   glxx_update_color_material(state);

   GL11_UNLOCK_SERVER_STATE();
}
