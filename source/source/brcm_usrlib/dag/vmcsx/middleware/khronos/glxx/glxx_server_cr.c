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

#include "middleware/khronos/gl20/gl20_program.h"
#include "middleware/khronos/gl20/gl20_shader.h"
#include "middleware/khronos/glxx/glxx_renderbuffer.h"
#include "middleware/khronos/glxx/glxx_framebuffer.h"

#include <string.h>
#include <math.h>
#include <limits.h>

//jeremyt 30/3/2010 get_boolean_internal moved back from server_cr.c
/*
   int glxx_get_boolean_internal(GLXX_SERVER_STATE_T *state, GLenum pname, GLboolean *params)

   Returns either a boolean state variable or the result of an isEnabled.  A utility 
   function shared by GetBooleanv() GetIntegerv() GetFloatv() GetFixedv() IsEnabled(X)

   Implementation notes:

   stores the value(s) of the given state variable in the memory given by the pointer "params", and returns
   number of such values.  An UNREACHABLE() macro guards against preconditon on pname being unmet by the caller.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context
   pname is one of the native boolean state variables listed below or an IsEnabled thing
   params points to sufficient GLbooleans for the pname (i.e. 1 value except where noted below)

   Postconditions:

   -

   Native Boolean State Variables

   COLOR WRITEMASK GetBooleanv (4 values returned)
   DEPTH WRITEMASK GetBooleanv
   SAMPLE COVERAGE INVERT GetBooleanv
   
   //gl 1.1 specific
   LIGHT MODEL TWO SIDE GetBooleanv
   
   //gl 2.0 specific
   SHADER COMPILER False GetBooleanv

   plus these

   CULL FACE IsEnabled
   POLYGON OFFSET FILL IsEnabled
   SAMPLE ALPHA TO COVERAGE IsEnabled
   SAMPLE COVERAGE IsEnabled
   SCISSOR TEST IsEnabled
   STENCIL TEST IsEnabled
   DEPTH TEST IsEnabled
   BLEND IsEnabled
   DITHER IsEnabled
   
   //gl 1.1 specific
   COLOR LOGIC OP IsEnabled
   NORMALIZE IsEnabled
   RESCALE NORMAL IsEnabled
   CLIP PLANE0 IsEnabled
   FOG IsEnabled
   LIGHTING IsEnabled
   COLOR MATERIAL IsEnabled
   LIGHT0 IsEnabled
   LIGHT1 IsEnabled
   LIGHT2 IsEnabled
   LIGHT3 IsEnabled
   LIGHT4 IsEnabled
   LIGHT5 IsEnabled
   LIGHT6 IsEnabled
   LIGHT7 IsEnabled
   POINT SMOOTH IsEnabled
   POINT SPRITE OES IsEnabled
   LINE SMOOTH IsEnabled
   TEXTURE 2D IsEnabled   
   ALPHA TEST IsEnabled   
   MULTISAMPLE IsEnabled
   SAMPLE ALPHA TO ONE IsEnabled
   
   

   The following state variables are not included because they are client side.  The table here should be 
   cross-referenced when the client side code is reviewed.

   VERTEX ARRAY IsEnabled .
   NORMAL ARRAY IsEnabled .
   COLOR ARRAY IsEnabled .
   TEXTURE COORD ARRAY IsEnabled .
   POINT SIZE ARRAY OES IsEnabled .

*/

int glxx_get_boolean_internal(GLXX_SERVER_STATE_T *state, GLenum pname, GLboolean *params)
{
   int result = 0;

   vcos_assert(GL11_CONFIG_MAX_PLANES == 1);
   vcos_assert(GL11_CONFIG_MAX_LIGHTS == 8);

   switch (pname) {
   case GL_LIGHT_MODEL_TWO_SIDE:
      if (IS_GL_11(state)) {
         params[0] = state->lightmodel.two_side;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_SAMPLE_COVERAGE_INVERT:
      params[0] = state->sample_coverage.invert;
      result = 1;
      break;
   case GL_COLOR_WRITEMASK:
      {
         int i;
         for (i = 0; i < 4; i++)
            params[i] = (state->shader.common.blend.color_mask & (0xff << (8*i))) != 0;
         result = 4;
      }
      break;
   case GL_DEPTH_WRITEMASK:
      params[0] = state->depth_mask;
      result = 1;
      break;
   case GL_NORMALIZE:
      if (IS_GL_11(state)) {
         params[0] = state->shader.normalize;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_RESCALE_NORMAL:
      if (IS_GL_11(state)) {
         params[0] = state->shader.rescale_normal;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_CLIP_PLANE0:
      if (IS_GL_11(state)) {
         params[0] = state->caps.clip_plane[pname - GL_CLIP_PLANE0];
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_FOG:
      if (IS_GL_11(state)) {
         params[0] = state->caps_fragment.fog;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_LIGHTING:
      if (IS_GL_11(state)) {
         params[0] = state->shader.lighting;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_COLOR_MATERIAL:
      if (IS_GL_11(state)) {
         params[0] = state->shader.color_material;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
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
      if (IS_GL_11(state)) {
         params[0] = state->lights[pname - GL_LIGHT0].enabled;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_POINT_SMOOTH:
      if (IS_GL_11(state)) {
         params[0] = state->caps_fragment.point_smooth;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_POINT_SPRITE_OES:
      if (IS_GL_11(state)) {
         params[0] = state->caps_fragment.point_sprite;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_LINE_SMOOTH:
      if (IS_GL_11(state)) {
         params[0] = state->caps_fragment.line_smooth;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_SHADER_COMPILER:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }else
      {
         params[0] = GL_TRUE;
         result = 1;
      }
      break;  
   case GL_CULL_FACE:
      params[0] = state->caps.cull_face;
      result = 1;
      break;
   case GL_POLYGON_OFFSET_FILL:
      params[0] = state->caps.polygon_offset_fill;
      result = 1;
      break;
   case GL_MULTISAMPLE:
      if (IS_GL_11(state)) {
         params[0] = state->caps.multisample;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_SAMPLE_ALPHA_TO_COVERAGE:
      params[0] = state->caps.sample_alpha_to_coverage;
      result = 1;
      break;
   case GL_SAMPLE_ALPHA_TO_ONE:
      if (IS_GL_11(state)) {
         params[0] = state->caps.sample_alpha_to_one;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_SAMPLE_COVERAGE:
      params[0] = state->caps.sample_coverage;
      result = 1;
      break;
   case GL_TEXTURE_2D:
   case GL_TEXTURE_EXTERNAL_OES:
      if (IS_GL_11(state)) {
         vcos_assert(state->active_texture - GL_TEXTURE0 < GL11_CONFIG_MAX_TEXTURE_UNITS);
         params[0] = state->texunits[state->active_texture - GL_TEXTURE0].enabled;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_SCISSOR_TEST:
      params[0] = state->caps.scissor_test;
      result = 1;
      break;
   case GL_ALPHA_TEST:
      if (IS_GL_11(state)) {
         params[0] = state->caps_fragment.alpha_test;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_STENCIL_TEST:
      params[0] = state->caps.stencil_test;
      result = 1;
      break;
   case GL_DEPTH_TEST:
      params[0] = state->caps.depth_test;
      result = 1;
      break;
   case GL_BLEND:
      params[0] = state->caps.blend;
      result = 1;
      break;
   case GL_DITHER:
      params[0] = state->caps.dither;
      result = 1;
      break;
   case GL_COLOR_LOGIC_OP:
      if (IS_GL_11(state)) {
         params[0] = state->caps.color_logic_op;
         result = 1;
      }
      else
      {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   default:
      UNREACHABLE();
      result = 0;
      break;
   }

   return result;
}

//jeremyt 30/3/2010 get_float_internal moved back from server_cr.c
/*
   int glxx_get_float_internal(GLXX_SERVER_STATE_T *state, GLenum pname, float *params)

   Returns a float state variable.  A utility 
   function shared by GetBooleanv() GetIntegerv() GetFloatv() GetFixedv()

   Implementation notes:

   stores the value(s) of the given state variable in the memory given by the pointer "params", and returns
   number of such values.  An UNREACHABLE() macro guards against preconditon on pname being unmet by the caller.

   ALPHA TEST REF and DEPTH CLEAR VALUE are stored and set as floats, but retrieved with GetIntegerv.  We treat
   them as floats.  This is a possible deviation from the specification.

   Non-overlapping precondition on gl11_matrix_load is satisfied as params can never point to part of the GL state.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context
   pname is one of the native floating-point state variables listed below
   params points to sufficient float for the pname (i.e. 1 value except where noted below)

   Postconditions:

   -

   Native Floating-point State Variables

   LINE WIDTH GetFloatv
   POLYGON OFFSET FACTOR GetFloatv
   POLYGON OFFSET UNITS GetFloatv
   SAMPLE COVERAGE VALUE GetFloatv
   COLOR CLEAR VALUE GetFloatv (4 values returned)
   DEPTH CLEAR VALUE GetIntegerv                // this is set as a float, so moving to GetFloatv
   DEPTH RANGE GetFloatv (2 values returned)
   
   //gl 1.1 specific
   MODELVIEW MATRIX GetFloatv (16 values returned)
   PROJECTION MATRIX GetFloatv  (16 values returned)
   TEXTURE MATRIX GetFloatv (16 values returned)
   
   FOG COLOR GetFloatv (4 values returned)
   FOG DENSITY GetFloatv
   FOG START GetFloatv
   FOG END GetFloatv
   LIGHT MODEL AMBIENT GetFloatv (4 values returned)
   POINT SIZE MIN GetFloatv
   POINT SIZE MAX GetFloatv
   POINT FADE THRESHOLD SIZE GetFloatv
   POINT DISTANCE ATTENUATION GetFloatv (3 values returned)
   
   ALPHA TEST REF GetIntegerv                  // this is set as a float, so moving to GetFloatv
   
   ALIASED POINT SIZE RANGE GetFloatv (2 values returned)
   SMOOTH POINT SIZE RANGE GetFloatv (2 values returned)
   ALIASED LINE WIDTH RANGE GetFloatv (2 values returned)
   SMOOTH LINE WIDTH RANGE GetFloatv (2 values returned)
   
   //gl 2.0 specific
   BLEND COLOR 0,0,0,0 GetFloatv

   The following state variables are not included because they are client side.  The table here should be 
   cross-referenced when the client side code is reviewed.

   CURRENT_COLOR (4 values returned) .
   CURRENT_TEXTURE_COORDS (4 values returned) .
   CURRENT_NORMAL (3 values returned) .
   POINT_SIZE .
*/
int glxx_get_float_internal(GLXX_SERVER_STATE_T *state, GLenum pname, GLfloat *params)
{
   int result = 0;

   vcos_assert(state);

   switch (pname) {
   case GL_MODELVIEW_MATRIX:
      if (IS_GL_11(state)) {
         gl11_matrix_load(params, state->modelview.body[state->modelview.pos]);
         result = 16;
      }
      else {
         UNREACHABLE();
         result = 0;
      }
      break;
   case GL_PROJECTION_MATRIX:
      if (IS_GL_11(state)) {
         gl11_matrix_load(params, state->projection.body[state->projection.pos]);
         result = 16;
      }
      else {
         UNREACHABLE();
         result = 0;      
      }
      break;
   case GL_TEXTURE_MATRIX:
      if (IS_GL_11(state)) {
         GL11_MATRIX_STACK_T *stack = &state->texunits[state->active_texture - GL_TEXTURE0].stack;
   
         vcos_assert(state->active_texture - GL_TEXTURE0 < GL11_CONFIG_MAX_TEXTURE_UNITS);
   
         gl11_matrix_load(params, stack->body[stack->pos]);
         result = 16;
      }
      else {
         UNREACHABLE();
         result = 0;      
      }   
      break;
   case GL_DEPTH_RANGE:
      params[0] = state->viewport.near;
      params[1] = state->viewport.far;
      result = 2;
      break;
   case GL_FOG_COLOR:
      if (IS_GL_11(state)) {
         int i;
         for (i = 0; i < 4; i++)
            params[i] = state->fog.color[i];
         result = 4;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }
      break;
   case GL_FOG_DENSITY:
      if (IS_GL_11(state)) {
         params[0] = state->fog.density;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;      
      }         
      break;
   case GL_FOG_START:
      if (IS_GL_11(state)) {
         params[0] = state->fog.start;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;      
      }         
      break;
   case GL_FOG_END:
      if (IS_GL_11(state)) {
         params[0] = state->fog.end;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_LIGHT_MODEL_AMBIENT:
      if (IS_GL_11(state)) {
         int i;
         for (i = 0; i < 4; i++)
            params[i] = state->lightmodel.ambient[i];
         result = 4;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_POINT_SIZE_MIN:
      if (IS_GL_11(state)) {
         params[0] = state->point_params.size_min;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;      
      }         
      break;
   case GL_POINT_SIZE_MAX:
      if (IS_GL_11(state)) {
         params[0] = state->point_params.size_max;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;      
      }         
      break;
   case GL_POINT_FADE_THRESHOLD_SIZE:
      if (IS_GL_11(state)) {
         params[0] = state->point_params.fade_threshold;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;      
      }         
      break;
   case GL_POINT_DISTANCE_ATTENUATION:
      if (IS_GL_11(state)) {
         int i;
         for (i = 0; i < 3; i++)
            params[i] = state->point_params.distance_attenuation[i];
         result = 3;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;      
      }      
      break;
   case GL_LINE_WIDTH:
      params[0] = state->line_width;
      result = 1;
      break;
   case GL_POLYGON_OFFSET_FACTOR:
      params[0] = state->polygon_offset.factor;
      result = 1;
      break;
   case GL_POLYGON_OFFSET_UNITS:
      params[0] = state->polygon_offset.units;
      result = 1;
      break;
   case GL_SAMPLE_COVERAGE_VALUE:
      params[0] = state->sample_coverage.value;
      result = 1;
      break;
   case GL_ALPHA_TEST_REF:
      if (IS_GL_11(state)) {
         params[0] = state->alpha_func.ref;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;      
      }         
      break;
   case GL_COLOR_CLEAR_VALUE:
   {
      int i;
      for (i = 0; i < 4; i++)
         params[i] = state->clear_color[i];
      result = 4;
      break;
   }
   case GL_DEPTH_CLEAR_VALUE:
         params[0] = state->clear_depth;
         result = 1;
      break;
   case GL_BLEND_COLOR:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         int i;
         for (i = 0; i < 4; i++)
            params[i] = state->blend_color[i];
         result = 4;
      }         
      break;
   case GL_ALIASED_POINT_SIZE_RANGE:
      if (IS_GL_11(state)) {
         params[0] = GL11_CONFIG_MIN_ALIASED_POINT_SIZE;
         params[1] = GL11_CONFIG_MAX_ALIASED_POINT_SIZE;
         result = 2;
      }
      else {
         params[0] = GL20_CONFIG_MIN_ALIASED_POINT_SIZE;
         params[1] = GL20_CONFIG_MAX_ALIASED_POINT_SIZE;
         result = 2;
      }         
      break;
   case GL_SMOOTH_POINT_SIZE_RANGE:
      if (IS_GL_11(state)) {
         params[0] = GL11_CONFIG_MIN_SMOOTH_POINT_SIZE;
         params[1] = GL11_CONFIG_MAX_SMOOTH_POINT_SIZE;
         result = 2;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_ALIASED_LINE_WIDTH_RANGE:
      if (IS_GL_11(state)) {
         params[0] = GL11_CONFIG_MIN_ALIASED_LINE_WIDTH;
         params[1] = GL11_CONFIG_MAX_ALIASED_LINE_WIDTH;
         result = 2;
      }
      else {
         params[0] = GL20_CONFIG_MIN_ALIASED_LINE_WIDTH;
         params[1] = GL20_CONFIG_MAX_ALIASED_LINE_WIDTH;
         result = 2;
      }         
      break;
   case GL_SMOOTH_LINE_WIDTH_RANGE:
      if (IS_GL_11(state)) {
         params[0] = GL11_CONFIG_MIN_SMOOTH_LINE_WIDTH;
         params[1] = GL11_CONFIG_MAX_SMOOTH_LINE_WIDTH;
         result = 2;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   default:
      UNREACHABLE();
      result = 0;
      break;
   }

   return result;
}

//jeremyt 30/3/2010 get_integer_internal moved back from server_cr.c
/*
   int glxx_get_integer_internal(GLXX_SERVER_STATE_T *state, GLenum pname, int *params)

   Returns an integer state variable.  A utility 
   function shared by GetBooleanv() GetIntegerv() GetFloatv() GetFixedv()

   Implementation notes:

   stores the value(s) of the given state variable in the memory given by the pointer "params", and returns
   number of such values.  An UNREACHABLE() macro guards against preconditon on pname being unmet by the caller.

   ALPHA TEST REF and DEPTH CLEAR VALUE are stored and set as floats, but retrieved with GetIntegerv.  We treat
   them as floats.  This is a possible deviation from the specification.

   GL_SAMPLES is meaningless if GL_SAMPLE_BUFFERS is zero. But we return 1 as this is
   probably the most helpful answer.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context
   pname is one of the native integer state variables listed below (excluding **)
   params points to sufficient storage for the pname (i.e. 1 value except where noted below)

   Postconditions:

   -

   Native Integer State Variables

   VIEWPORT GetIntegerv  (4)
   CULL FACE MODE GetIntegerv 
   FRONT FACE GetIntegerv 
   TEXTURE BINDING 2D GetIntegerv  (n state elements but only return element for current server TU)
   ACTIVE TEXTURE GetIntegerv 
** DEPTH CLEAR VALUE GetIntegerv                // this is set as a float, so moving to GetFloatv
   STENCIL CLEAR VALUE GetIntegerv   
   SCISSOR BOX GetIntegerv  (4)
   STENCIL FUNC GetIntegerv 
   STENCIL VALUE MASK GetIntegerv 
   STENCIL REF GetIntegerv 
   STENCIL FAIL GetIntegerv 
   STENCIL PASS DEPTH FAIL GetIntegerv 
   STENCIL PASS DEPTH PASS GetIntegerv 
   DEPTH FUNC GetIntegerv 
   GENERATE MIPMAP HINT GetIntegerv 
   SUBPIXEL BITS GetIntegerv 
   MAX TEXTURE SIZE GetIntegerv    
   MAX VIEWPORT DIMS GetIntegerv  (2)
   SAMPLE BUFFERS GetIntegerv 
   SAMPLES GetIntegerv 
   COMPRESSED TEXTURE FORMATS GetIntegerv  (10*) we return 11
   NUM COMPRESSED TEXTURE FORMATS GetIntegerv 
   RED BITS GetIntegerv 
   GREEN BITS GetIntegerv 
   BLUE BITS GetIntegerv 
   ALPHA BITS GetIntegerv 
   DEPTH BITS GetIntegerv 
   STENCIL BITS GetIntegerv 
   STENCIL WRITEMASK GetIntegerv    
   ARRAY BUFFER BINDING GetIntegerv .           (state also available on server side)
   ELEMENT ARRAY BUFFER BINDING GetIntegerv .         "

   //gl 1.1 specific

   MODELVIEW MATRIX FLOAT AS INT BITS OES GetIntegerv  (16)
   PROJECTION MATRIX FLOAT AS INT BITS OES GetIntegerv  (16)
   TEXTURE MATRIX FLOAT AS INT BITS OES GetIntegerv  (16)
   MODELVIEW STACK DEPTH GetIntegerv 
   PROJECTION STACK DEPTH GetIntegerv 
   TEXTURE STACK DEPTH GetIntegerv  (n state elements but only return element for current server TU)
   MATRIX MODE GetIntegerv 
   FOG MODE GetIntegerv 
   SHADE MODEL GetIntegerv  
   ALPHA TEST FUNC GetIntegerv  
** ALPHA TEST REF GetIntegerv                   // this is set as a float, so moving to GetFloatv
   BLEND SRC GetIntegerv 
   BLEND DST GetIntegerv 
   LOGIC OP MODE GetIntegerv 
   PERSPECTIVE CORRECTION HINT GetIntegerv 
   POINT SMOOTH HINT GetIntegerv 
   LINE SMOOTH HINT GetIntegerv 
   FOG HINT GetIntegerv 
   MAX LIGHTS GetIntegerv 
   MAX CLIP PLANES GetIntegerv 
   MAX MODELVIEW STACK DEPTH GetIntegerv 
   MAX PROJECTION STACK DEPTH GetIntegerv 
   MAX TEXTURE STACK DEPTH GetIntegerv 
   MAX TEXTURE UNITS GetIntegerv 
   
   //gl 2.0 specific 
   TEXTURE BINDING CUBE MAP 0 GetIntegerv
   STENCIL BACK WRITEMASK 1 s GetIntegerv
   STENCIL BACK FUNC ALWAYS GetIntegerv
   STENCIL BACK VALUE MASK 1 s GetIntegerv
   STENCIL BACK REF 0 GetIntegerv
   STENCIL BACK FAIL KEEP GetIntegerv
   STENCIL BACK PASS DEPTH FAIL KEEP GetIntegerv
   STENCIL BACK PASS DEPTH PASS KEEP GetIntegerv
   BLEND SRC RGB ONE GetIntegerv
   BLEND SRC ALPHA ONE GetIntegerv
   BLEND DST RGB ZERO GetIntegerv
   BLEND DST ALPHA ZERO GetIntegerv
   BLEND EQUATION RGB FUNC ADD GetIntegerv
   BLEND EQUATION ALPHA FUNC ADD GetIntegerv
   MAX CUBE MAP TEXTURE SIZE 16 GetIntegerv
   MAX ELEMENTS INDICES   GetIntegerv
   MAX ELEMENTS VERTICES   GetIntegerv
   SHADER BINARY FORMATS   GetIntegerv
   NUM SHADER BINARY FORMATS 0 GetIntegerv
   MAX VERTEX ATTRIBS 8 GetIntegerv
   MAX VERTEX UNIFORM VECTORS 128 GetIntegerv
   MAX VARYING VECTORS 8 GetIntegerv
   MAX COMBINED TEXTURE IMAGE UNITS 8 GetIntegerv
   MAX VERTEX TEXTURE IMAGE UNITS 0 GetIntegerv
   MAX TEXTURE IMAGE UNITS 8 GetIntegerv
   MAX FRAGMENT UNIFORM VECTORS 16 GetIntegerv
   MAX RENDERBUFFER SIZE 1 GetIntegerv
   IMPLEMENTATION COLOR READ TYPE   GetIntegerv
   IMPLEMENTATION COLOR READ FORMAT   GetIntegerv
   RENDERBUFFER BINDING 0 GetIntegerv
   FRAMEBUFFER BINDING 0 GetIntegerv
   CURRENT PROGRAM 0 GetIntegerv

   The following state variables are not included because they are client side.  The table here should be 
   cross-referenced when the client side code is reviewed.

   CLIENT ACTIVE TEXTURE GetIntegerv .
   VERTEX ARRAY SIZE GetIntegerv .
   VERTEX ARRAY TYPE GetIntegerv .
   VERTEX ARRAY STRIDE GetIntegerv .
   NORMAL ARRAY TYPE GetIntegerv .
   NORMAL ARRAY STRIDE GetIntegerv .
   COLOR ARRAY SIZE GetIntegerv .
   COLOR ARRAY TYPE GetIntegerv .
   COLOR ARRAY STRIDE GetIntegerv .
   TEXTURE COORD ARRAY SIZE GetIntegerv . (n state elements but only return element for current client TU)
   TEXTURE COORD ARRAY TYPE GetIntegerv .          "
   TEXTURE COORD ARRAY STRIDE GetIntegerv .        "
   POINT SIZE ARRAY TYPE OES GetIntegerv .
   POINT SIZE ARRAY STRIDE OES GetIntegerv .
   VERTEX ARRAY BUFFER BINDING GetIntegerv .
   NORMAL ARRAY BUFFER BINDING GetIntegerv .
   COLOR ARRAY BUFFER BINDING GetIntegerv .
   TEXTURE COORD ARRAY BUFFER BINDING GetIntegerv . (n state elements but only return element for current client TU)
   POINT SIZE ARRAY BUFFER BINDING OES GetIntegerv .
   UNPACK ALIGNMENT GetIntegerv .
   PACK ALIGNMENT GetIntegerv .
*/

int glxx_get_integer_internal(GLXX_SERVER_STATE_T *state, GLenum pname, int *params)
{
   int result = 0;

   switch (pname) {
   case GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES:
      if (IS_GL_11(state)) {
         gl11_matrix_load((float *)params, state->modelview.body[state->modelview.pos]);
         result = 16;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }   
      break;
   case GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES:
      if (IS_GL_11(state)) {
         gl11_matrix_load((float *)params, state->projection.body[state->projection.pos]);
         result = 16;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES:
      if (IS_GL_11(state)) {
         GL11_MATRIX_STACK_T *stack = &state->texunits[state->active_texture - GL_TEXTURE0].stack;
   
         vcos_assert(state->active_texture - GL_TEXTURE0 < GL11_CONFIG_MAX_TEXTURE_UNITS);
   
         gl11_matrix_load((float *)params, stack->body[stack->pos]);
         result = 16;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_ARRAY_BUFFER_BINDING:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         if (state->bound_buffer.mh_array != MEM_INVALID_HANDLE) {
            params[0] = ((GLXX_BUFFER_T *)mem_lock(state->bound_buffer.mh_array))->name;
            mem_unlock(state->bound_buffer.mh_array);
         } else
            params[0] = 0;
         result = 1;
      }
      break;
   case GL_ELEMENT_ARRAY_BUFFER_BINDING:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         if (state->bound_buffer.mh_element_array != MEM_INVALID_HANDLE) {
            params[0] = ((GLXX_BUFFER_T *)mem_lock(state->bound_buffer.mh_element_array))->name;
            mem_unlock(state->bound_buffer.mh_element_array);
         } else
            params[0] = 0;
         result = 1;
      }
      break;
   case GL_VIEWPORT:
      params[0] = state->viewport.x;
      params[1] = state->viewport.y;
      params[2] = state->viewport.width;
      params[3] = state->viewport.height;
      result = 4;
      break;
   case GL_MODELVIEW_STACK_DEPTH:
      if (IS_GL_11(state)) {
         params[0] = state->modelview.pos + 1;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_PROJECTION_STACK_DEPTH:
      if (IS_GL_11(state)) {
         params[0] = state->projection.pos + 1;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_TEXTURE_STACK_DEPTH:
      if (IS_GL_11(state)) {
         params[0] = state->texunits[state->active_texture - GL_TEXTURE0].stack.pos + 1;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_MATRIX_MODE:
      if (IS_GL_11(state)) {
         params[0] = state->matrix_mode;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_FOG_MODE:
      if (IS_GL_11(state)) {
         params[0] = state->fog.mode;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_SHADE_MODEL:
      if (IS_GL_11(state)) {
         params[0] = state->shade_model;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_CULL_FACE_MODE:
      params[0] = state->cull_mode;
      result = 1;
      break;
   case GL_FRONT_FACE:
      params[0] = state->front_face;
      result = 1;
      break;
   case GL_TEXTURE_BINDING_2D:
   {
      MEM_HANDLE_T handle = state->bound_texture[state->active_texture - GL_TEXTURE0].mh_twod;

      vcos_assert(handle != MEM_INVALID_HANDLE);

      params[0] = ((GLXX_TEXTURE_T *)mem_lock(handle))->name;
      mem_unlock(handle);

      result = 1;
      break;
   }
   case GL_TEXTURE_BINDING_CUBE_MAP:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         MEM_HANDLE_T handle = state->bound_texture[state->active_texture - GL_TEXTURE0].mh_cube;

         vcos_assert(handle != MEM_INVALID_HANDLE);

         params[0] = ((GLXX_TEXTURE_T *)mem_lock(handle))->name;
         mem_unlock(handle);

         result = 1;
      }         
      break;
   case GL_ACTIVE_TEXTURE:
      params[0] = state->active_texture;
      result = 1;
      break;
   case GL_SCISSOR_BOX:
      params[0] = state->scissor.x;
      params[1] = state->scissor.y;
      params[2] = state->scissor.width;
      params[3] = state->scissor.height;
      result = 4;
      break;
   case GL_ALPHA_TEST_FUNC:
      if (IS_GL_11(state)) {
         params[0] = state->alpha_func.func;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_STENCIL_WRITEMASK:
      params[0] = state->stencil_mask.front;
      result = 1;
      break;
   case GL_STENCIL_BACK_WRITEMASK:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = state->stencil_mask.back;
         result = 1;
      }         
      break;
   case GL_STENCIL_CLEAR_VALUE:
      params[0] = state->clear_stencil;
      result = 1;
      break;
   case GL_STENCIL_FUNC:
      params[0] = state->stencil_func.front.func;
      result = 1;
      break;
   case GL_STENCIL_VALUE_MASK:
      params[0] = state->stencil_func.front.mask;
      result = 1;
      break;
   case GL_STENCIL_REF:
      params[0] = state->stencil_func.front.ref;
      result = 1;
      break;
   case GL_STENCIL_FAIL:
      params[0] = state->stencil_op.front.fail;
      result = 1;
      break;
   case GL_STENCIL_PASS_DEPTH_FAIL:
      params[0] = state->stencil_op.front.zfail;
      result = 1;
      break;
   case GL_STENCIL_PASS_DEPTH_PASS:
      params[0] = state->stencil_op.front.zpass;
      result = 1;
      break;
   case GL_STENCIL_BACK_FUNC:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = state->stencil_func.back.func;
         result = 1;
      }         
      break;
   case GL_STENCIL_BACK_VALUE_MASK:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = state->stencil_func.back.mask;
         result = 1;
      }         
      break;
   case GL_STENCIL_BACK_REF:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = state->stencil_func.back.ref;
         result = 1;
      }         
      break;
   case GL_STENCIL_BACK_FAIL:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = state->stencil_op.back.fail;
         result = 1;
      }         
      break;
   case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = state->stencil_op.back.zfail;
         result = 1;
      }         
      break;
   case GL_STENCIL_BACK_PASS_DEPTH_PASS:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = state->stencil_op.back.zpass;
         result = 1;
      }         
      break;
   case GL_DEPTH_FUNC:
      params[0] = state->depth_func;
      result = 1;
      break;
   case GL_BLEND_SRC:
      if (IS_GL_11(state)) {
         params[0] = state->blend_func.src_rgb;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_BLEND_DST:
      if (IS_GL_11(state)) {
         params[0] = state->blend_func.dst_rgb;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_BLEND_SRC_RGB:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = state->blend_func.src_rgb;
         result = 1;
      }         
      break;
   case GL_BLEND_SRC_ALPHA:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = state->blend_func.src_alpha;
         result = 1;
      }         
      break;
   case GL_BLEND_DST_RGB:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = state->blend_func.dst_rgb;
         result = 1;
      }         
      break;
   case GL_BLEND_DST_ALPHA:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = state->blend_func.dst_alpha;
         result = 1;
      }         
      break;
   case GL_BLEND_EQUATION_RGB:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = state->blend_equation.rgb;
         result = 1;
      }         
      break;
   case GL_BLEND_EQUATION_ALPHA:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = state->blend_equation.alpha;
         result = 1;
      }         
      break;
   case GL_LOGIC_OP_MODE:
      if (IS_GL_11(state)) {
         params[0] = state->logic_op;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_PERSPECTIVE_CORRECTION_HINT:
      if (IS_GL_11(state)) {
         params[0] = state->hints.perspective_correction;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_POINT_SMOOTH_HINT:
      if (IS_GL_11(state)) {
         params[0] = state->hints.point_smooth;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_LINE_SMOOTH_HINT:
      if (IS_GL_11(state)) {
         params[0] = state->hints.line_smooth;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_FOG_HINT:
      if (IS_GL_11(state)) {
         params[0] = state->hints_program.fog;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_GENERATE_MIPMAP_HINT:
      params[0] = state->hints.generate_mipmap;
      result = 1;
      break;
   case GL_MAX_LIGHTS:
      if (IS_GL_11(state)) {
         params[0] = GL11_CONFIG_MAX_LIGHTS;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_MAX_CLIP_PLANES:
      if (IS_GL_11(state)) {
         params[0] = GL11_CONFIG_MAX_PLANES;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_MAX_MODELVIEW_STACK_DEPTH:
   case GL_MAX_PROJECTION_STACK_DEPTH:
   case GL_MAX_TEXTURE_STACK_DEPTH:
      if (IS_GL_11(state)) {
         params[0] = GL11_CONFIG_MAX_STACK_DEPTH;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_SUBPIXEL_BITS:
      if (IS_GL_11(state)) {
         params[0] = GL11_CONFIG_SUBPIXEL_BITS;
         result = 1;
      }
      else {
          if (state->mh_multi == MEM_INVALID_HANDLE)
            params[0] = 1;
         else
            params[0] = 4;
         result = 1;
      }
      break;
   case GL_MAX_TEXTURE_SIZE:
         params[0] = MAX_TEXTURE_SIZE;
         result = 1;
      break;
   case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = MAX_TEXTURE_SIZE;
         result = 1;
         break;
      }         
      break;
   case GL_MAX_VIEWPORT_DIMS:
      params[0] = GLXX_CONFIG_MAX_VIEWPORT_SIZE;
      params[1] = GLXX_CONFIG_MAX_VIEWPORT_SIZE;
      result = 2;
      break;
   case GL_MAX_TEXTURE_UNITS:
      if (IS_GL_11(state)) {
         params[0] = GL11_CONFIG_MAX_TEXTURE_UNITS;
         result = 1;
      }
      else {
         UNREACHABLE();//gl 1.1 only
         result = 0;
      }         
      break;
   case GL_SAMPLE_BUFFERS:
      if (state->mh_multi == MEM_INVALID_HANDLE)
         params[0] = 0;
      else
         params[0] = 1;
      result = 1;
      break;
   case GL_SAMPLES:
      if (IS_GL_11(state)) {
         if (state->mh_multi == MEM_INVALID_HANDLE)
            params[0] = 1;
         else
            params[0] = GL11_CONFIG_SAMPLES;
         result = 1;
      }
      else {
         if (state->mh_multi == MEM_INVALID_HANDLE)
            params[0] = 1;
         else
            params[0] = 4;
         result = 1;
      }
      break;
   case GL_COMPRESSED_TEXTURE_FORMATS:
      params[0] = GL_PALETTE4_RGB8_OES;
      params[1] = GL_PALETTE4_RGBA8_OES;
      params[2] = GL_PALETTE4_R5_G6_B5_OES;
      params[3] = GL_PALETTE4_RGBA4_OES;
      params[4] = GL_PALETTE4_RGB5_A1_OES;
      params[5] = GL_PALETTE8_RGB8_OES;
      params[6] = GL_PALETTE8_RGBA8_OES;
      params[7] = GL_PALETTE8_R5_G6_B5_OES;
      params[8] = GL_PALETTE8_RGBA4_OES;
      params[9] = GL_PALETTE8_RGB5_A1_OES;
      params[10] = GL_ETC1_RGB8_OES;
      result = 11;
      break;
   case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
      params[0] = 11;
      result = 1;
      break;
   case GL_SHADER_BINARY_FORMATS:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         result = 0;
      }         
      break;
   case GL_NUM_SHADER_BINARY_FORMATS:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = 0;
         result = 1;
      }         
      break;
   case GL_MAX_VERTEX_ATTRIBS:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = GLXX_CONFIG_MAX_VERTEX_ATTRIBS;               // 8
         result = 1;
      }         
      break;
   case GL_MAX_VERTEX_UNIFORM_VECTORS:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = GL20_CONFIG_MAX_UNIFORM_VECTORS;              // 128
         result = 1;
      }         
      break;
   case GL_MAX_VARYING_VECTORS:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = GL20_CONFIG_MAX_VARYING_VECTORS;              // 8
         result = 1;
      }         
      break;
   case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = GL20_CONFIG_MAX_TEXTURE_UNITS;                // 8
         result = 1;
      }         
      break;
   case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = 0;
         result = 1;
      }         
      break;
   case GL_MAX_TEXTURE_IMAGE_UNITS:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = GL20_CONFIG_MAX_TEXTURE_UNITS;                // 8
         result = 1;
      }         
      break;
   case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = GL20_CONFIG_MAX_UNIFORM_VECTORS;              // 128
         result = 1;
      }         
      break;
   case GL_MAX_RENDERBUFFER_SIZE:
      {
         params[0] = GLXX_CONFIG_MAX_RENDERBUFFER_SIZE;            // 2048
         result = 1;
      }         
      break;
   case GL_RED_BITS:
      {
         KHRN_IMAGE_FORMAT_T format = glxx_get_draw_image_format(state);
         params[0] = format!=IMAGE_FORMAT_INVALID ? khrn_image_get_red_size(format) : 0;
         result = 1;
         break;
      }
   case GL_GREEN_BITS:
      {
         KHRN_IMAGE_FORMAT_T format = glxx_get_draw_image_format(state);
         params[0] = format!=IMAGE_FORMAT_INVALID ? khrn_image_get_green_size(format) : 0;
         result = 1;
         break;
      }
   case GL_BLUE_BITS:
      {
         KHRN_IMAGE_FORMAT_T format = glxx_get_draw_image_format(state);
         params[0] = format!=IMAGE_FORMAT_INVALID ? khrn_image_get_blue_size(format) : 0;
         result = 1;
         break;
      }
   case GL_ALPHA_BITS:
      {
         KHRN_IMAGE_FORMAT_T format = glxx_get_draw_image_format(state);
         params[0] = format!=IMAGE_FORMAT_INVALID ? khrn_image_get_alpha_size(format) : 0;
         result = 1;
         break;
      }
   case GL_DEPTH_BITS:
      if (state->mh_depth == MEM_INVALID_HANDLE)
         params[0] = 0;
      else {
         KHRN_IMAGE_FORMAT_T format = glxx_get_depth_image_format(state);
         params[0] = format!=IMAGE_FORMAT_INVALID ? khrn_image_get_z_size(format) : 0;
      }
      result = 1;
      break;
   case GL_STENCIL_BITS:
   {
      params[0] = glxx_get_stencil_size(state);
      result = 1;
      break;
   }
   case GL_IMPLEMENTATION_COLOR_READ_TYPE:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = GL_UNSIGNED_BYTE;
         result = 1;
      }         
      break;
   case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         params[0] = GL_RGBA;
         result = 1;
      }         
      break;
   case GL_RENDERBUFFER_BINDING:
      {
         if (state->mh_bound_renderbuffer != MEM_INVALID_HANDLE) {
            params[0] = ((GLXX_RENDERBUFFER_T *)mem_lock(state->mh_bound_renderbuffer))->name;
            mem_unlock(state->mh_bound_renderbuffer);
         } else
            params[0] = 0;
         result = 1;
      }         
      break;
   case GL_FRAMEBUFFER_BINDING:
      {
         if (state->mh_bound_framebuffer != MEM_INVALID_HANDLE) {
            params[0] = ((GLXX_FRAMEBUFFER_T *)mem_lock(state->mh_bound_framebuffer))->name;
            mem_unlock(state->mh_bound_framebuffer);
         } else
            params[0] = 0;
         result = 1;
      }         
      break;
   case GL_CURRENT_PROGRAM:
      if (IS_GL_11(state)) {
         UNREACHABLE();//gl 2.0 only
         result = 0;
      }
      else {
         if (state->mh_program != MEM_INVALID_HANDLE) {
            params[0] = ((GL20_PROGRAM_T *)mem_lock(state->mh_program))->name;
            mem_unlock(state->mh_program);
         } else
            params[0] = 0;
         result = 1;
      }         
      break;
   default:
      UNREACHABLE();
      result = 0;
      break;
   }

   return result;
}

//jeremyt 30/3/2010 update_viewport_internal moved back from server_cr.c
/*
   glxx_update_viewport_internal(GLXX_SERVER_STATE_T *state)

   restore the invariant that state.viewport.internal is consistent with the
   rest of the contents of state.viewport

   Implementation notes:

   A HAL typically requires not the viewport and depthrange values themselves, but
   some trivially-derived values. As this state changes infrequently, we choose to
   cache the derived values.

   The state structure has an invariant that the two sets of values are consistent.
   At entry to this function the invariant may have been broken. This function
   restores it.

   Preconditions:

   state is a valid pointer

   Postconditions:

   Invariant to state.viewport holds
*/

void glxx_update_viewport_internal(GLXX_SERVER_STATE_T *state)
{
   //was 3-8
   state->viewport.internal[0] = (float)state->viewport.width / 2.0f;
   state->viewport.internal[1] = (float)state->viewport.height / 2.0f;
   state->viewport.internal[2] = (float)state->viewport.x + (float)state->viewport.width / 2.0f;
   state->viewport.internal[3] = (float)state->viewport.y + (float)state->viewport.height / 2.0f;

   state->viewport.internal[4] = (state->viewport.far - state->viewport.near) / 2.0f;
   state->viewport.internal[5] = (state->viewport.far + state->viewport.near) / 2.0f;
   
   if (!IS_GL_11(state)) {//gl 2.0 specific
      //was 0-2
      state->viewport.internal[6] = state->viewport.near;
      state->viewport.internal[7] = state->viewport.far;
      state->viewport.internal[8] = state->viewport.far - state->viewport.near;
   }
   //extra entries used by install_uniforms
   state->viewport.internal[9] = 16.0f * state->viewport.internal[0];
   /* don't multiply scale y until installing the uniform as it depends on the attatched FB type */
   state->viewport.internal[10] = state->viewport.internal[1];
   state->viewport.internal[11] = state->viewport.internal[5] * 16777215.0f;
}

//jeremyt 30/3/2010 glxx_server_state_set_error moved back from server_cr.c
/*
   glxx_server_state_set_error()

   set GL server-side error state if none already set

   Implementation notes:

   Preconditions:

   state is a valid pointer
   error is a valid GL error

   Postconditions:
*/

void glxx_server_state_set_error(GLXX_SERVER_STATE_T *state, GLenum error)
{
#ifdef BREAK_ON_ERROR
   BREAKPOINT();
#endif
   if (state->error == GL_NO_ERROR)
      state->error = error;
}

//jeremyt 30/3/2010 glActiveTexture_impl_11 moved back from server_cr.c
/*
   void glActiveTexture_impl_xx (GLenum texture)

   Khronos documentation (GL ES 1.1):

   There is also a corresponding texture matrix stack for each texture unit. To
   change the stack affected by matrix operations, set the active texture unit selector
   by calling

      void ActiveTexture( enum texture );

   The selector also affects calls modifying texture environment state, texture coordinate
   generation state, texture binding state, and queries of all these state values as
   well as current texture coordinates.

   Specifying an invalid texture generates the error INVALID ENUM. Valid values
   of texture are the same as for the MultiTexCoord commands described in section
   2.7.

   Implementation notes:

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context

   Postconditions:   

   If supplied texture is not a valid texture unit and no current error, error becomes INVALID_ENUM

   Invariants preserved:

   state.alpha_func.func and state.alpha_func.ref are valid



*/

void glActiveTexture_impl (GLenum texture)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   vcos_assert(state);

   if (texture >= GL_TEXTURE0 && (
         (IS_GL_11(state) && texture < GL_TEXTURE0 + GL11_CONFIG_MAX_TEXTURE_UNITS) ||
         (IS_GL_20(state) && texture < GL_TEXTURE0 + GL20_CONFIG_MAX_TEXTURE_UNITS)
      ))
      state->active_texture = texture;
   else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GLXX_UNLOCK_SERVER_STATE();
}

//jeremyt 30/3/2010 glBindBuffer_impl_11 moved back from server_cr.c
/*
   void glBindBuffer_impl_xx (GLenum target, GLuint buffer)

   Khronos documentation:

   The name space for buffer objects is the unsigned integers, with zero reserved
   for the GL. A buffer object is created by binding an unused name to
   ARRAY BUFFER. The binding is effected by calling
   void BindBuffer( enum target, uint buffer );
   with target set to ARRAY BUFFER and buffer set to the unused name. The resulting
   buffer object is a new state vector, initialized with a zero-sized memory buffer, and
   comprising the state values listed in Table 2.5.
   BindBuffer may also be used to bind an existing buffer object. If the bind is
   successful no change is made to the state of the newly bound buffer object, and any
   previous binding to target is broken.
   While a buffer object is bound, GL operations on the target to which it is bound
   affect the bound buffer object, and queries of the target to which a buffer object is
   bound return state from the bound object.
   In the initial state the reserved name zero is bound to ARRAY BUFFER. There
   is no buffer object corresponding to the name zero, so client attempts to modify
   or query buffer object state for the target ARRAY BUFFER while zero is bound will
   generate GL errors.

   Blocks of array indices may be stored in buffer objects with the same format options
   that are supported for client-side index arrays. Initially zero is bound to
   ELEMENT ARRAY BUFFER, indicating that DrawElements is to source its indices
   from arrays passed as the indices parameters.
   A buffer object is bound to ELEMENT ARRAY BUFFER by calling BindBuffer
   with target set to ELEMENT ARRAY BUFFER, and buffer set to the name of the buffer
   object. If no corresponding buffer object exists, one is initialized as defined in
   section 2.9.

   Implementation notes:

   -

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context

   Postconditions:   

   If no current error, the following conditions cause error to assume the specified value

      GL_INVALID_ENUM               if target is invalid
      GL_OUT_OF_MEMORY              failed to create buffer object
      

   if more than one condition holds, the first error is generated.

   Invariants preserved:

   All invariants on the shared object buffer map are preserved.
   mh_element_array is either MEM_INVALID_HANDLE or a valid handle to a GLXX_BUFFER_T
   mh_array is either MEM_INVALID_HANDLE or a valid handle to a GLXX_BUFFER_T

*/


void glBindBuffer_impl (GLenum target, GLuint buffer)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   vcos_assert(state);

   if (target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER) {
      MEM_HANDLE_T handle = MEM_INVALID_HANDLE;

      if (buffer) {
         handle = glxx_shared_get_buffer((GLXX_SHARED_T *)mem_lock(state->mh_shared), buffer, true);
         mem_unlock(state->mh_shared);

         if (handle == MEM_INVALID_HANDLE) {
            glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);

            GLXX_UNLOCK_SERVER_STATE();
            return;
         }
      }

      switch (target) {
      case GL_ARRAY_BUFFER:
         MEM_ASSIGN(state->bound_buffer.mh_array, handle);
         break;
      case GL_ELEMENT_ARRAY_BUFFER:
         MEM_ASSIGN(state->bound_buffer.mh_element_array, handle);
		 state->changed = GL_FALSE;
         break;
      //no default case needed as values for target defined by if statement above
      }
   } else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GLXX_UNLOCK_SERVER_STATE();
}

//jeremyt 30/3/2010 glBindTexture_impl_11 moved back from server_cr.c
/*
   void glBindTexture_impl_xx (GLenum target, GLuint texture)

   Khronos documentation:

   In addition to the default texture TEXTURE 2D, named texture objects can be created
   and operated upon. The name space for texture objects is the unsigned integers,
   with zero reserved by the GL.
   A texture object is created by binding an unused name to TEXTURE 2D. The
   binding is effected by calling
   void BindTexture( enum target, uint texture );
   with target set to TEXTURE 2D and texture set to the unused name. The resulting
   texture object is a new state vector, comprising all the state values listed in
   section 3.7.10, set to the same initial values.
   BindTexture may also be used to bind an existing texture object to
   TEXTURE 2D. If the bind is successful no change is made to the state of the bound
   texture object, and any previous binding to target is broken.
   While a texture object is bound, GL operations on the target to which it is
   bound affect the bound object, and queries of the target to which it is bound return
   state from the bound object. If texture mapping is enabled, the state of the bound
   texture object directs the texturing operation.

   Implementation notes:

   -

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context

   Postconditions:   

   If no current error, the following conditions cause error to assume the specified value

      GL_INVALID_ENUM               if target is invalid
      GL_OUT_OF_MEMORY              failed to create texture
      
      TODO: this bit is unreviewed:
      GL_INVALID_OPERATION          wrong dimensionality (i.e. it's a shared cube map texture from GLES2.0 land)
      

   if more than one condition holds, the first error is generated.

   Invariants preserved:

   All invariants on the shared object texture map are preserved.
   mh_bound_texture != MEM_INVALID_HANDLE
   mh_bound_texture is a handle to a valid GLXX_TEXTURE_T object
*/

void glBindTexture_impl (GLenum target, GLuint texture)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   vcos_assert(state);

   if ((target == GL_TEXTURE_2D) || (target == GL_TEXTURE_EXTERNAL_OES) || (!IS_GL_11(state) && target == GL_TEXTURE_CUBE_MAP)) {
      MEM_HANDLE_T handle;
      bool is_cube, has_color, has_alpha, complete;
      int t = state->active_texture - GL_TEXTURE0;

      is_cube = (target == GL_TEXTURE_CUBE_MAP);//always false in gl 1.1

      handle = is_cube ? state->mh_default_texture_cube : state->mh_default_texture_twod;
      complete = false;       /* Default textures are incomplete */

      if (texture) {
         GLenum error;
         handle = glxx_shared_get_or_create_texture((GLXX_SHARED_T *)mem_lock(state->mh_shared), texture, is_cube, &error, &has_color, &has_alpha, &complete);
         mem_unlock(state->mh_shared);

         if (handle == MEM_INVALID_HANDLE) {
            glxx_server_state_set_error(state, error);

            GLXX_UNLOCK_SERVER_STATE();
            return;
         }
      }

      if (is_cube)
         MEM_ASSIGN(state->bound_texture[t].mh_cube, handle);
      else
      {

         GL11_CACHE_TEXUNIT_ABSTRACT_T *texabs = &state->shader.texunits[t];
         MEM_ASSIGN(state->bound_texture[t].mh_twod, handle);
         if (IS_GL_11(state) && state->texunits[t].enabled && texabs->mode != GL_COMBINE)
         {
            state->changed_directly = true;
            texabs->props.active = complete;
            texabs->props.has_color = complete && has_color;
            texabs->props.has_alpha = complete && has_alpha;
         }
      }

   } else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GLXX_UNLOCK_SERVER_STATE();
}

/*
   GLXX_BUFFER_T * glxx_get_bound_buffer(GLXX_SERVER_STATE_T *state, GLenum target, MEM_HANDLE_T *handle)

   Return a locked pointer to the buffer bound to a particular target.

   Implementation notes:

   -

   Preconditions:

   state is a valid pointer
   handle is a valid pointer to an uninit MEM_HANDLE_T
   is being called from a function which _always_ subsequently calls mem_unlock() on the handle if we return non-NULL

   Postconditions:

   If no current error, the following conditions cause error to assume the specified value

      GL_INVALID_ENUM               target invalid
      GL_INVALID_OPERATION          no buffer bound to target

   if more than one condition holds, the first error is generated.

   If no error is generated
      
      returned handle is valid (i.e. not MEM_INVALID_HANDLE)
      returned handle has been locked by this function
      returned pointer is valid until handle is unlocked

   else

      returned pointer is NULL
      no locks have been acquired
*/

GLXX_BUFFER_T *glxx_get_bound_buffer(GLXX_SERVER_STATE_T *state, GLenum target, MEM_HANDLE_T *handle)
{
   MEM_HANDLE_T bhandle;

   vcos_assert(handle);

   switch (target) {
   case GL_ARRAY_BUFFER:
      bhandle = state->bound_buffer.mh_array;
      break;
   case GL_ELEMENT_ARRAY_BUFFER:
      bhandle = state->bound_buffer.mh_element_array;
      break;
   default:
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
      return NULL;
   }

   if (bhandle == MEM_INVALID_HANDLE) {
      glxx_server_state_set_error(state, GL_INVALID_OPERATION);
      return NULL;
   }

   *handle = bhandle;

   return (GLXX_BUFFER_T *)mem_lock(bhandle);
}

//jeremyt 30/3/2010 glBufferData_impl_11, is_usage moved back from server_cr.c
/*
   void glBufferData_impl (GLenum target, GLsizeiptr size, GLenum usage, const GLvoid *data)

   Khronos documentation:

   The data store of a buffer object is created and initialized by calling

      void BufferData( enum target, sizeiptr size, const void *data, enum usage );

   with target set to ARRAY BUFFER, size set to the size of the data store in basic
   machine units, and data pointing to the source data in client memory. If data is
   non-null, then the source data is copied to the buffer object’s data store. If data is
   null, then the contents of the buffer object’s data store are undefined.
   usage is specified as one of two enumerated values, indicating the expected
   application usage pattern of the data store. The values are STATIC DRAW and DYNAMIC DRAW 
   and are provided as a performance hint only. If the GL is unable to create a data store 
   of the requested size, the error OUT OF MEMORY is generated.

   BufferData deletes any existing data store, and sets the values of the buffer
   object's state variables as shown in table 2.6.

   In the initial state the reserved name zero is bound to ARRAY BUFFER. There
   is no buffer object corresponding to the name zero, so client attempts to modify
   or query buffer object state for the target ARRAY BUFFER while zero is bound will
   generate GL errors.

   Implementation notes:

   Note parameter order swap for RPC convenience. glxx_get_bound_buffer() is responsible for generating
   some of the errors (if invalid target or no buffer bound).
   In the dual core driver we are forced to wait for the other core to finish as we currently have no
   way of checking whether it is using this buffer.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   Data is either NULL or a valid pointer to size bytes which does not overlap the GL state

   Postconditions:   

   If no current error, the following conditions cause error to assume the specified value

      GL_INVALID_VALUE              size < 0
      GL_INVALID_ENUM               usage invalid
      GL_INVALID_ENUM               target invalid
      GL_INVALID_OPERATION          no buffer bound to target
      GL_OUT_OF_MEMORY              failed to allocate buffer

   if more than one condition holds, the first error is generated.

   Invariants preserved:

   All buffer objects have valid storage handles.
*/


static bool is_usage(GLXX_SERVER_STATE_T *state, GLenum usage)
{
   return usage == GL_STATIC_DRAW ||
          usage == GL_DYNAMIC_DRAW ||
          (!IS_GL_11(state) && usage == GL_STREAM_DRAW);
}

void glBufferData_impl (GLenum target, GLsizeiptr size, GLenum usage, const GLvoid *data)             // note parameter order swap for RPC convenience
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (size < 0)
      glxx_server_state_set_error(state, GL_INVALID_VALUE);
   else if (!is_usage(state,usage))   
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
   else {
      MEM_HANDLE_T bhandle;

      GLXX_BUFFER_T *buffer = glxx_get_bound_buffer(state, target, &bhandle);

      if (buffer) {
         if (!glxx_buffer_data(buffer, size, data, usage))
            glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);

         mem_unlock(bhandle);
      }
   }

   GLXX_UNLOCK_SERVER_STATE();
}

//jeremyt 30/3/2010 glBufferSubData_impl_11 moved back from server_cr.c
/*
   void glBufferSubData_impl (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data)

   Khronos documentation:

   To modify some or all of the data contained in a buffer object’s data store, the
   client may use the command
   void BufferSubData( enum target, intptr offset,
   sizeiptr size, const void *data );
   with target set to ARRAY BUFFER. offset and size indicate the range of data in the
   buffer object that is to be replaced, in terms of basic machine units. data specifies a
   region of client memory size basic machine units in length, containing the data that
   replace the specified buffer range. An INVALID VALUE error is generated if offset
   or size is less than zero, or if offset + size is greater than the value of BUFFER SIZE.

   Implementation notes:

   glxx_get_bound_buffer() is responsible for generating
   some of the errors (if invalid target or no buffer bound).
   If data is null then we silently do nothing.
   When checking offset+size>BUFFER_SIZE, we first cast everything to uint32_t. We have already
   established that all the integers we are interested in are nonnegative so this cast is valid,
   and it relies on the sum of two nonnegative signed integers not exceeding the valid range of
   an unsigned integer.
   We check the errors in a different order to glBufferData because we need to know the buffer
   size before doing the size check.
   In the dual core driver we are forced to wait for the other core to finish as we currently have no
   way of checking whether it is using this buffer.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context
   Data is either NULL or a valid pointer to size bytes which does not overlap the GL state

   Postconditions:   

   If no current error, the following conditions cause error to assume the specified value

      GL_INVALID_ENUM               target invalid
      GL_INVALID_OPERATION          no buffer bound to target
      GL_INVALID_VALUE              offset < 0
      GL_INVALID_VALUE              size < 0
      GL_INVALID_VALUE              offset + size > BUFFER_SIZE

   if more than one condition holds, the first error is generated.

   Invariants preserved:

   Invariants on size_used_for_max and max for current buffer object
*/

void glBufferSubData_impl (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   MEM_HANDLE_T bhandle;

   GLXX_BUFFER_T *buffer = glxx_get_bound_buffer(state, target, &bhandle);

   if (buffer) {
      /*
         If this is a dual core driver, the other core may be using this
         buffer (we currently have no way of checking). So flush the FIFO
         to make sure.
      */

      if (offset < 0 || size < 0 || (uint32_t)offset + (uint32_t)size > (uint32_t)glxx_buffer_get_size(buffer)) 
         glxx_server_state_set_error(state, GL_INVALID_VALUE);
      else {
         if (data)
            glxx_buffer_subdata(buffer, offset, size, data);
      }

      mem_unlock(bhandle);
   }

   GLXX_UNLOCK_SERVER_STATE();
}

//jeremyt 30/3/2010 glClearColor_impl_11,glClearColorx_impl_11,clear_color_internal moved back from server_cr.c
/*
   glClearColor_impl(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
   glClearColorx_impl_11(GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha)

   Khronos documentation:

   Each of the specified components is clamped to [0, 1] and converted to fixed-point 
   according to the rules of section 2.12.8.

   Implementation notes:

   Conversion to fixed point is deferred to color buffer clear time.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context

   Postconditions:   

   Invariants preserved:

   state.clear_color is valid
*/

void glxx_clear_color_internal(float red, float green, float blue, float alpha)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   state->clear_color[0] = clampf(red, 0.0f, 1.0f);
   state->clear_color[1] = clampf(green, 0.0f, 1.0f);
   state->clear_color[2] = clampf(blue, 0.0f, 1.0f);
   state->clear_color[3] = clampf(alpha, 0.0f, 1.0f);

   GLXX_UNLOCK_SERVER_STATE();
}

void glClearColor_impl (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
   glxx_clear_color_internal(red, green, blue, alpha);
}

//jeremyt 30/3/2010 glClearDepthf_impl_11,glClearDepthx_impl_11,clear_depth_internal moved back from server_cr.c
/*
   glClearDepth_impl(GLclampf depth)
   glClearDepthx_impl_11(GLclampx depth)

   Khronos documentation:

   Takes a value that is clamped to the range [0, 1] and converted to fixed-point according
   to the rules for a window z value given in section 2.10.1, which says:

   zw is taken to be represented in fixed-point with at least as many bits
   as there are in the depth buffer of the framebuffer. We assume that the fixed-point
   representation used represents each value k/(2^m - 1), where k in {0, 1, . . . , 2^m - 1}, 
   as k (e.g. 1.0 is represented in binary as a string of all ones).

   Implementation notes:

   Conversion to fixed point is deferred to depth buffer clear time.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context

   Postconditions:   

   Invariants preserved:

   state.clear_depth is valid
*/

void glxx_clear_depth_internal (float depth)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   state->clear_depth = clampf(depth, 0.0f, 1.0f);

   GLXX_UNLOCK_SERVER_STATE();
}

void glClearDepthf_impl (GLclampf depth)
{
   glxx_clear_depth_internal(depth);
}

//jeremyt 30/3/2010 glClearStencil_impl_11 moved back from server_cr.c
/*
   void glClearStencil_impl (GLint s)

   Khronos documentation:

   Similarly,
   void ClearStencil( int s );
   takes a single integer argument that is the value to which to clear the stencil buffer.
   s is masked to the number of bitplanes in the stencil buffer.

   Implementation notes:

   -

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context

   Postconditions:   

   -

   Invariants preserved:

   -
*/

void glClearStencil_impl (GLint s)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   state->clear_stencil = s & ((1 << glxx_get_stencil_size(state)) - 1);

   GLXX_UNLOCK_SERVER_STATE();
}

//jeremyt 30/3/2010 glColorMask_impl_11 moved back from server_cr.c
/*
   void glColorMask_impl (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)

   Khronos documentation:

   Four commands are used to mask the writing of bits to each of the logical framebuffers
   after all per-fragment operations have been performed. The command
   void ColorMask( boolean r, boolean g, boolean b, boolean a );
   controls the writing of R, G, B and A values to the color buffer. r, g, b, and a
   indicate whether R, G, B, or A values, respectively, are written or not (a value of
   TRUE means that the corresponding value is written). In the initial state, all color
   values are enabled for writing.
   The state required for the masking operations is an integer for stencil values
   and a bit for depth values. A set of four bits is also required indicating which color
   components of an RGBA value should be written. In the initial state, the stencil
   mask is all ones, as are the bits controlling depth value and RGBA component
   writing.

   Implementation notes:

   -

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context

   Postconditions:   

   -

   Invariants preserved:

   (GL11_SERVER_STATE_CLEAN_BOOL)
   All "bool" state variables are "clean" - i.e. 0 or 1

*/

void glColorMask_impl (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();
   uint32_t cmask = 0;

   if (red)   cmask |= 0x000000FF;
   if (green) cmask |= 0x0000FF00;
   if (blue)  cmask |= 0x00FF0000;
   if (alpha) cmask |= 0xFF000000;

   state->changed_backend = true;
   state->shader.common.blend.color_mask = cmask;

   GLXX_UNLOCK_SERVER_STATE();
}

//jeremyt 30/3/2010 glCullFace_impl_11, is_cull_face moved back from server_cr.c
/*
   void glCullFace_impl (GLenum mode)

   Khronos documentation:

   The first step of polygon rasterization is to determine if the polygon is back facing
   or front facing. This determination is made by examining the sign of the area computed
   by equation 2.6 of section 2.12.1 (including the possible reversal of this sign
   as indicated by the last call to FrontFace). If this sign is positive, the polygon is
   front facing; otherwise, it is back facing. This determination is used in conjunction
   with the CullFace enable bit and mode value to decide whether or not a particular
   polygon is rasterized. The CullFace mode is set by calling
   void CullFace( enum mode );
   mode is a symbolic constant: one of FRONT, BACK or FRONT AND BACK. Culling
   is enabled or disabled with Enable or Disable using the symbolic constant
   CULL FACE. Front facing polygons are rasterized if either culling is disabled or
   the CullFace mode is BACK while back facing polygons are rasterized only if either
   culling is disabled or the CullFace mode is FRONT. The initial setting of the
   CullFace mode is BACK. Initially, culling is disabled.

   Implementation notes:

   -

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context

   Postconditions:   

   If no current error, the following condition causes error to assume the specified value

      GL_INVALID_ENUM               mode invalid

   Invariants preserved:

      (GL11_SERVER_STATE_CULL_MODE)
      cull_mode in {FRONT, BACK, FRONT_AND_BACK}

*/

static GLboolean is_cull_face(GLenum mode)
{
   return mode == GL_FRONT ||
          mode == GL_BACK ||
          mode == GL_FRONT_AND_BACK;
}

void glCullFace_impl (GLenum mode)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (is_cull_face(mode))
   {
      state->changed_cfg = true;
      state->cull_mode = mode;
   }
   else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GLXX_UNLOCK_SERVER_STATE();
}

//jeremyt 30/3/2010 glDeleteBuffers_impl_11 moved back from server_cr.c
/*
   void glDeleteBuffers_impl (GLsizei n, const GLuint *buffers)

   Khronos documentation:

   Buffer objects are deleted by calling
   void DeleteBuffers( sizei n, const uint *buffers );
   buffers contains n names of buffer objects to be deleted. After a buffer object is
   deleted it has no contents, and its name is again unused. Unused names in buffers
   are silently ignored, as is the value zero.

   If a buffer object is deleted while it is bound, all
   bindings to that object in the current context (i.e. in the thread that called Delete-
   Buffers) are reset to zero.

   Implementation notes:

   We do not establish conceptual_buffers_owned_by_master before manipulating any buffers.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context
   If n > 0 and buffers is not null then buffers is a valid pointer to n elements

   Postconditions:   

   If no current error, the following conditions cause error to assume the specified value

      GL_INVALID_VALUE              n < 0

   Invariants preserved:

   -
*/

void glDeleteBuffers_impl (GLsizei n, const GLuint *buffers)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   GLXX_SHARED_T *shared = (GLXX_SHARED_T *)mem_lock(state->mh_shared);

   if (n < 0)
      glxx_server_state_set_error(state, GL_INVALID_VALUE);
   else if (buffers) {
      int i;

      /*
         We believe it is not necessary to call khrn_fifo_finish() here because
         the buffer contents are not being modified.
         If the client requires more predictable memory usage, glFinish() should
         be called after deleting objects and before allocating new ones.
      */

      for (i = 0; i < n; i++) {
         if (buffers[i]) {
            MEM_HANDLE_T hbuffer = glxx_shared_get_buffer(shared, buffers[i], false);

            if (hbuffer != MEM_INVALID_HANDLE) {
               int32_t j;
               if (state->bound_buffer.mh_array == hbuffer)
                  MEM_ASSIGN(state->bound_buffer.mh_array, MEM_INVALID_HANDLE);
               if (state->bound_buffer.mh_element_array == hbuffer)
                  MEM_ASSIGN(state->bound_buffer.mh_element_array, MEM_INVALID_HANDLE);
                  
               for (j = 0; j < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; j++)
                  if (state->bound_buffer.mh_attrib_array[j] == hbuffer)
                     MEM_ASSIGN(state->bound_buffer.mh_attrib_array[j], MEM_INVALID_HANDLE);
               
               glxx_shared_delete_buffer(shared, buffers[i]);
            }
         }
      }
   }

   mem_unlock(state->mh_shared);

   GLXX_UNLOCK_SERVER_STATE();
}


//jeremyt 30/3/2010 glDeleteTextures_impl_11 moved back from server_cr.c
/*
   void glDeleteTextures_impl (GLsizei n, const GLuint *textures)

   Khronos documentation:

   Texture objects are deleted by calling
   void DeleteTextures( sizei n, uint *textures );
   textures contains n names of texture objects to be deleted. After a texture object
   is deleted, it has no contents, and its name is again unused. If a texture that is
   currently bound to the target TEXTURE 2D is deleted, it is as though BindTexture
   had been executed with the same target and texture zero. Unused names in textures
   are silently ignored, as is the value zero.

   If a texture object is deleted, it is as if all texture units which are bound to that
   texture object are rebound to texture object zero.

   //gl 2.0 specific
   If a texture object is deleted while its image is attached to one or more attachment
   points in the currently bound framebuffer, then it is as if FramebufferTexture2D
   had been called, with a texture of 0, for each attachment point to which
   this image was attached in the currently bound framebuffer. In other words, this
   texture image is first detached from all attachment points in the currently bound
   framebuffer. Note that the texture image is specifically not detached from any
   other framebuffer objects

   (eglBindTexImage)
   Note that the color buffer is bound to a texture object. If the texture object is
   shared between contexts, then the color buffer is also shared. If a texture object is
   deleted before eglReleaseTexImage is called, then the color buffer is released and
   the surface is made available for reading and writing.


   Implementation notes:

   In the above paragraph, we take it to refer to the actual deletion of a texture, rather
   than when glDeleteTextures is called (there may still be live references to it if the
   texture object is shared). If we want the EGL binding to be broken on a glDeleteTextures
   call, call glxx_texture_release_teximage on htexture.
   
   Deletes a specified list of texture objects. If a texture is bound to the
   TEXTURE_2D or TEXTURE_CUBE_MAP target of any texture unit, the binding is
   reset. No errors are specified in the spec, but we check if buffers is NULL
   and fail silently if so.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   If n > 0 and textures is not null then textures is a valid pointer to n elements

   Postconditions:   

   If no current error, the following conditions cause error to assume the specified value

      GL_INVALID_VALUE              n < 0

   Invariants preserved:

   -
*/

void glDeleteTextures_impl (GLsizei n, const GLuint *textures)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   GLXX_SHARED_T *shared = (GLXX_SHARED_T *)mem_lock(state->mh_shared);

   if (n < 0)
      glxx_server_state_set_error(state, GL_INVALID_VALUE);   // The conformance tests insist...
   else if (textures) {
      int i;
      for (i = 0; i < n; i++) {
         if (textures[i]) {
            MEM_HANDLE_T htexture = glxx_shared_get_texture(shared, textures[i]);

            if (htexture != MEM_INVALID_HANDLE) {
               int j;
               
               if(IS_GL_11(state)) {
                  for (j = 0; j < GL11_CONFIG_MAX_TEXTURE_UNITS; j++) {
                     if (state->bound_texture[j].mh_twod == htexture)
                        MEM_ASSIGN(state->bound_texture[j].mh_twod, state->mh_default_texture_twod);
                  }
               } else {
                  for (j = 0; j < GL20_CONFIG_MAX_TEXTURE_UNITS; j++) {
                     if (state->bound_texture[j].mh_twod == htexture)
                        MEM_ASSIGN(state->bound_texture[j].mh_twod, state->mh_default_texture_twod);
                     if (state->bound_texture[j].mh_cube == htexture)
                        MEM_ASSIGN(state->bound_texture[j].mh_cube, state->mh_default_texture_cube);
                  }
               }

               glxx_shared_delete_texture(shared, textures[i]);
            }
         }
      }
   }

   mem_unlock(state->mh_shared);

   GLXX_UNLOCK_SERVER_STATE();
}

//jeremyt 30/3/2010 glDepthRangef_impl_11,glDepthRangex_impl_11,depth_range_internal moved back from server_cr.c
/*
   glDepthRangef_impl (GLclampf zNear, GLclampf zFar)
   glDepthRangex_impl_11 (GLclampx zNear, GLclampx zFar)

   Khronos documentation:

   Each of n and f are clamped to lie within [0, 1], as are all arguments of type clampf
   or clampx. zw is taken to be represented in fixed-point with at least as many bits
   as there are in the depth buffer of the framebuffer. We assume that the fixed-point
   representation used represents each value k/(2^m - 1), where k in {0, 1, . . . , 2^m - 1}, 
   as k (e.g. 1.0 is represented in binary as a string of all ones).

   Implementation notes:

   We update the near and far elements of state.viewport, potentially
   violating the invariant and call glxx_update_viewport_internal() to restore it.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context

   Postconditions:   

   -

   Invariants preserved:

   0.0 <= state.viewport.near <= 1.0
   0.0 <= state.viewport.far  <= 1.0

   state.viewport.internal is consistent with other elements according to glxx_update_viewport_internal() docs
   elements of state.viewport.internal are valid
*/

void glxx_depth_range_internal(float zNear, float zFar)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   state->viewport.near = clampf(zNear, 0.0f, 1.0f);
   state->viewport.far = clampf(zFar, 0.0f, 1.0f);

   glxx_update_viewport_internal(state);

   GLXX_UNLOCK_SERVER_STATE();
}

void glDepthRangef_impl (GLclampf zNear, GLclampf zFar)
{
   glxx_depth_range_internal(zNear, zFar);
}

//jeremyt 30/3/2010 glGetFloatv_impl_11, get_float_or_fixed_internal moved back from server_cr.c
/*
   glGetFloatv_impl (GLenum pname, GLfloat *params)
   glGetFixedv_impl_11 (GLenum pname, GLfixed *params)

   Khronos documentation:

   State variables that can be obtained using any of GetBooleanv,
   GetIntegerv, GetFixedv, or GetFloatv are listed with just one of these commands
   – the one that is most appropriate given the type of the data to be returned. These
   state variables cannot be obtained using IsEnabled. However, state variables for
   which IsEnabled is listed as the query command can also be obtained using Get-
   Booleanv, GetIntegerv, GetFixedv, and GetFloatv. State variables for which any
   other command is listed as the query command can be obtained only by using that
   command.

   If GetFloatv is called, a boolean value is interpreted as either 1.0 or 0.0, and
   an integer or fixed-point value is coerced to floating-point.

   Implementation notes:

   These functions use glxx_get_float_or_fixed_internal() to first convert the boolean, 
   float or integer state variable into floating point form. There is an internal
   assumption (checked by assertions which may not fire due to data corruption) that
   no state variable has more than 16 scalar elements.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context
   params is a valid pointer to sufficient elements for the state variable specified

   Postconditions:   

   If pname is not a valid clipping plane and no current error, error becomes INVALID_ENUM

   Invariants preserved:

   -
*/

int glxx_get_float_or_fixed_internal(GLXX_SERVER_STATE_T *state, GLenum pname, float *params)
{
   if (glxx_is_boolean(state, pname))
   {
      GLboolean temp[16];
      int count = glxx_get_boolean_internal(state, pname, temp);
      int i;

      vcos_assert(count <= 16);

      for (i = 0; i < count; i++)
         params[i] = temp[i] ? 1.0f : 0.0f;

      return count;
   }
   else if (glxx_is_integer(state, pname))
   {
      int temp[16];
      int count = glxx_get_integer_internal(state, pname, temp);
      int i;

      vcos_assert(count <= 16);

      for (i = 0; i < count; i++)
         params[i] = (float)temp[i];

      return count;
   }
   else if (glxx_is_float(state, pname))
      return glxx_get_float_internal(state, pname, params);
   else
   {
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

      return 0;
   }
}

int glGetFloatv_impl (GLenum pname, GLfloat *params)
{
   int result;
   
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   result = glxx_get_float_or_fixed_internal(state, pname, params);
   
   GLXX_UNLOCK_SERVER_STATE();
   
   return result;
}

//jeremyt 30/3/2010 glLineWidth_impl_11, glLineWidthx_impl_11, line_width_internal moved back from server_cr.c
/*
   void glLineWidth_impl (GLfloat width)
   void glLineWidthx_impl_11 (GLfixed width)

   Khronos documentation:

   Line width may be set by calling

      void LineWidth( float width );
      void LineWidthx( fixed width );

   with an appropriate positive width, controls the width of rasterized line segments.
   The default width is 1.0. Values less than or equal to 0.0 generate the error
   INVALID VALUE. 

   Implementation notes:

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context

   Postconditions:   

   If !(width > 0) and no current error, error becomes GL_INVALID_ENUM

   Invariants preserved:

   state.line_width > 0.0
*/

void glxx_line_width_internal(float width)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (width > 0.0f)
   {
      state->changed_linewidth = true;
      state->line_width = width;
   }
   else
      glxx_server_state_set_error(state, GL_INVALID_VALUE);

   GLXX_UNLOCK_SERVER_STATE();
}

void glLineWidth_impl (GLfloat width) // S
{
   glxx_line_width_internal(width);
}

//jeremyt 30/3/2010 glPolygonOffset_impl_11, polygon_offset_internal moved back from server_cr.c
/*
   void glPolygonOffset_impl (GLfloat factor, GLfloat units)
   void glPolygonOffsetx_impl_11 (GLfixed factor, GLfixed units)

   Khronos documentation:

   The depth values of all fragments generated by the rasterization of a polygon may
   be offset by a single value that is computed for that polygon. The function that
   determines this value is specified by calling

      void PolygonOffset( float factor, float units );
      void PolygonOffsetx( fixed factor, fixed units );

   factor scales the maximum depth slope of the polygon, and units scales an implementation
   dependent constant that relates to the usable resolution of the depth
   buffer. The resulting values are summed to produce the polygon offset value. Both
   factor and units may be either positive or negative.

   Implementation notes:

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 or 2.0 context

   Postconditions:   

   Invariants preserved:
*/

void glxx_polygon_offset_internal(GLfloat factor, GLfloat units)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   state->changed_polygon_offset = true;
   state->polygon_offset.factor = factor;
   state->polygon_offset.units = units;

   GLXX_UNLOCK_SERVER_STATE();
}

void glPolygonOffset_impl (GLfloat factor, GLfloat units)
{
   glxx_polygon_offset_internal(factor, units);
}

//jeremyt 30/3/2010 glViewport_impl_11 moved back from server_cr.c
/*
   glViewport_impl_11 (GLint x, GLint y, GLsizei width, GLsizei height)

   Khronos documentation:

   where x and y give the x and y window coordinates of the viewport’s lower left
   corner and w and h give the viewport’s width and height, respectively. The viewport
   parameters shown in the above equations are found from these values as 
   
   ox = x + w/2
   oy = y + h/2 
   px = w 
   py = h

   Viewport width and height are clamped to implementation-dependent maximums
   when specified. The maximum width and height may be found by issuing
   an appropriate Get command (see Chapter 6). The maximum viewport dimensions
   must be greater than or equal to the visible dimensions of the display being
   rendered to. INVALID VALUE is generated if either w or h is negative.

   Implementation notes:

   We check that width and height are non-negative, update the x, y, width and height
   elements of state.viewport, potentially violating the invariant and call 
   glxx_update_viewport_internal() to restore it.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context

   Postconditions:   

   -

   Invariants preserved:

   state.viewport.width  >= 0
   state.viewport.height >= 0

   state.viewport.internal is consistent with other elements according to glxx_update_viewport_internal() docs
   elements of state.viewport.internal are valid
*/

void glViewport_impl (GLint x, GLint y, GLsizei width, GLsizei height)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();

   if (width >= 0 && height >= 0)
   {
      state->changed_viewport = true;
      state->viewport.x = x;
      state->viewport.y = y;
      state->viewport.width = clampi(width, 0, GLXX_CONFIG_MAX_VIEWPORT_SIZE);
      state->viewport.height = clampi(height, 0, GLXX_CONFIG_MAX_VIEWPORT_SIZE);

      glxx_update_viewport_internal(state);
   } else
      glxx_server_state_set_error(state, GL_INVALID_VALUE);

   GLXX_UNLOCK_SERVER_STATE();
}
