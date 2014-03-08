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
#include "middleware/khronos/glxx/glxx_server.h"
#include "middleware/khronos/glxx/glxx_server_internal.h"

/*
   Khronos Documentation:
   Overview

    Many applications may need to query the contents and status of the
    current matrix at least for debugging purposes, especially as the
    implementations are allowed to implement matrix machinery either in
    any (possibly proprietary) floating point format, or in a fixed point
    format that has the range and accuracy of at least 16.16 (signed 16 bit
    integer part, unsigned 16 bit fractional part).
    
    This extension is intended to allow application to query the components
    of the matrix and also their status, regardless whether the internal
    representation is in fixed point or floating point.

New Procedures and Functions

    GLbitfield glQueryMatrixxOES( GLfixed mantissa[16],
                                  GLint   exponent[16] )

    mantissa[16] contains the contents of the current matrix in GLfixed
    format.  exponent[16] contains the unbiased exponents applied to the
    matrix components, so that the internal representation of component i
    is close to mantissa[i] * 2^exponent[i].  The function returns a status
    word which is zero if all the components are valid. If
    status & (1<<i) != 0, the component i is invalid (e.g., NaN, Inf).
    The implementations are not required to keep track of overflows.  In
    that case, the invalid bits are never set.

Implementation Notes:
   exponents are fixed at 16
   all components are valid
   we return the same values returned by glGetFixedv(pname, params);
   with pname set to the current matrix mode
*/

void glQueryMatrixxOES_impl_11 ( GLfixed mantissa[16] )
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();
   int count,i;
   GLfloat temp[16];
   GLenum pname = 0;
   
   switch(state->matrix_mode)
   { 
      case GL_TEXTURE:
         pname = GL_TEXTURE_MATRIX;
         break;
      case GL_MODELVIEW:
         pname = GL_MODELVIEW_MATRIX;
         break;
      case GL_PROJECTION:
         pname = GL_PROJECTION_MATRIX;
         break;
      default:
         UNREACHABLE();      
   }
   
   count = glxx_get_float_or_fixed_internal(state, pname, temp);

   vcos_assert(count == 16);

   for (i = 0; i < count; i++)
      mantissa[i] = float_to_fixed(temp[i]);
   
   GLXX_UNLOCK_SERVER_STATE();
}

