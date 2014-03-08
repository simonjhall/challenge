/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GLXX_SHARED_H
#define GLXX_SHARED_H
#ifdef __cplusplus
extern "C" {
#endif
#include "middleware/khronos/common/khrn_map.h"
#include "interface/khronos/include/GLES/gl.h"

typedef struct {
   uint32_t next_pobject;
   uint32_t next_texture;
   uint32_t next_buffer;
   uint32_t next_renderbuffer;
   uint32_t next_framebuffer;

   KHRN_MAP_T pobjects;

   /*
      Map of texture identifier to texture object

      Khronos state variable names:

      -

      Invariant:

      textures is a valid map and the elements are valid TEXTURE_Ts
   */

   KHRN_MAP_T textures;

   /*
      Map of buffer identifier to buffer object

      Khronos state variable names:

      -

      Invariant:

      buffers is a valid map and the elements are valid BUFFER_Ts
   */

   KHRN_MAP_T buffers;
   KHRN_MAP_T renderbuffers;
   KHRN_MAP_T framebuffers;
} GLXX_SHARED_T;

extern bool glxx_shared_init(GLXX_SHARED_T *shared);
extern void glxx_shared_term(void *v, uint32_t size);

extern uint32_t glxx_shared_create_program(GLXX_SHARED_T *shared);
extern uint32_t glxx_shared_create_shader(GLXX_SHARED_T *shared, uint32_t type);

extern MEM_HANDLE_T glxx_shared_get_pobject(GLXX_SHARED_T *shared, uint32_t pobject);
extern MEM_HANDLE_T glxx_shared_get_buffer(GLXX_SHARED_T *shared, uint32_t buffer, bool create);
extern MEM_HANDLE_T glxx_shared_get_texture(GLXX_SHARED_T *shared, uint32_t texture);
extern MEM_HANDLE_T glxx_shared_get_or_create_texture(GLXX_SHARED_T *shared, uint32_t texture, bool is_cube, GLenum *error, bool *has_color, bool *has_alpha, bool *complete);
extern MEM_HANDLE_T glxx_shared_get_renderbuffer(GLXX_SHARED_T *shared, uint32_t renderbuffer, bool create);
extern MEM_HANDLE_T glxx_shared_get_framebuffer(GLXX_SHARED_T *shared, uint32_t framebuffer, bool create);

extern void glxx_shared_delete_pobject(GLXX_SHARED_T *shared, uint32_t pobject);
extern void glxx_shared_delete_buffer(GLXX_SHARED_T *shared, uint32_t buffer);
extern void glxx_shared_delete_texture(GLXX_SHARED_T *shared, uint32_t texture);
extern void glxx_shared_delete_renderbuffer(GLXX_SHARED_T *shared, uint32_t renderbuffer);
extern void glxx_shared_delete_framebuffer(GLXX_SHARED_T *shared, uint32_t framebuffer);
#ifdef __cplusplus
 }
#endif
#endif
