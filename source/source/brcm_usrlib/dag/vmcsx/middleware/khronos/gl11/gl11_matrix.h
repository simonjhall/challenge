/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GL11_MATRIX_H
#define GL11_MATRIX_H

#include "interface/khronos/include/GLES/gl.h"
#include "interface/khronos/glxx/gl11_int_config.h"

typedef struct {
   GLfloat body[GL11_CONFIG_MAX_STACK_DEPTH][16];

   /*
      Current top of stack
      
      i.e. body[n] is on the stack iff n <= pos

      Invariant: 
      
      0 <= pos < GL11_CONFIG_MAX_STACK_DEPTH
   */

   int32_t pos;
} GL11_MATRIX_STACK_T;

extern void gl11_matrix_load(GLfloat *d, const GLfloat *a);
extern void gl11_matrix_mult(GLfloat *d, const GLfloat *a, const GLfloat *b);

/*
   multiply matrix by row vector

   (x' y' z' w') = (x y z w) B
*/

extern void gl11_matrix_mult_row(GLfloat *d, const GLfloat *a, const GLfloat *b);
extern void gl11_matrix_mult_col(GLfloat *d, const GLfloat *a, const GLfloat *b);

/*
   gl11_matrix_invert_4x4(GLfloat *d, const GLfloat *a)

   invert a non-singular 4x4 matrix
*/

extern void gl11_matrix_invert_4x4(GLfloat *d, const GLfloat *a);

extern void gl11_matrix_invert_3x3(GLfloat *d, const GLfloat *a);

extern void gl11_matrix_transpose(GLfloat *d, const GLfloat *a);

extern GLboolean gl11_matrix_is_projective(GLfloat *a);

extern void gl11_matrix_stack_init(GL11_MATRIX_STACK_T *stack);

#endif
