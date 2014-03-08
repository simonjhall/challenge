/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GL11_TEXUNIT_H
#define GL11_TEXUNIT_H

#include "interface/khronos/include/GLES/gl.h"

/*
   The state required for the current texture environment, for each texture unit,
   consists of

      X a six-valued integer indicating the texture function
      X an eight-valued integer indicating the RGB combiner function
      X a six-valued integer indicating the ALPHA combiner function
      X six four-valued integers indicating the combiner RGB and ALPHA source arguments
      X three four-valued integers indicating the combiner RGB operands
      X three two-valued integers indicating the combiner ALPHA operands,
      X four floating-point environment color values
      X two three-valued floating-point scale factors
*/

typedef struct {
   GLenum combine;
   GLenum source[3];
   GLenum operand[3];

   GLfloat scale;
} GL11_COMBINER_T;

typedef struct {
   bool active;
   bool complex;
   bool has_color;
   bool has_alpha;

#ifndef __VIDEOCORE4__
   uint32_t varying;
#endif
} GL11_TEXUNIT_PROPS_T;

typedef struct {
   bool enabled;

   GLfloat color[4];

   GL11_COMBINER_T rgb;
   GL11_COMBINER_T alpha;

   /*
      Texture matrix stack for this unit

      Khronos name: TEXTURE_MATRIX
   */

   GL11_MATRIX_STACK_T stack;

   /*
      jeremyt 1/4/2010
      mh_bound_texture moved to state.bound_texture[i].twod
   */

   //cache of current matrix for install uniforms
   float current_matrix[16];
} GL11_TEXUNIT_T;

#endif
