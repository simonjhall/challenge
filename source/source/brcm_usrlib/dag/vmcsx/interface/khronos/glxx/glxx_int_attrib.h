/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GLXX_INT_ATTRIB_H
#define GLXX_INT_ATTRIB_H

#include "interface/khronos/include/GLES/gl.h"
#include "interface/khronos/include/GLES2/gl2.h"

#include "interface/khronos/common/khrn_client_cache.h"
#include <stddef.h>

typedef struct {
   GLboolean enabled;

   GLint size;
   GLenum type;
   GLboolean normalized;
   GLsizei stride;

   const GLvoid *pointer;
   int handle;

   GLuint buffer;

   GLfloat value[4];
} GLXX_ATTRIB_T;

#define SERIALIZE_ATTRIB(b, x, k) RPC_INT((x).size), \
                              RPC_ENUM((x).type), \
                              RPC_BOOLEAN((x).normalized), \
                              RPC_SIZEI((x).stride), \
                              RPC_UINT((b ? (uint32_t)(k + offsetof(CACHE_ENTRY_T, data)) : (uint32_t)(uintptr_t)(x).pointer)), \
                              RPC_UINT((x).buffer)

#define SERIALIZE_ATTRIB_VALUE(x) RPC_FLOAT((x).value[0]), \
                                  RPC_FLOAT((x).value[1]), \
                                  RPC_FLOAT((x).value[2]), \
                                  RPC_FLOAT((x).value[3])

/* GL 1.1 specific For indexing into arrays of handles/pointers */
#define GL11_IX_COLOR 1//0
#define GL11_IX_NORMAL 2//1
#define GL11_IX_VERTEX 0//2
#define GL11_IX_TEXTURE_COORD 3
#define GL11_IX_POINT_SIZE 7
#define GL11_IX_MAX_ATTRIBS 8

#endif
