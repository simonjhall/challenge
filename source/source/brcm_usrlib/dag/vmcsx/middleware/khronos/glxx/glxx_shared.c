/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "middleware/khronos/glxx/glxx_buffer.h"
#include "middleware/khronos/glxx/glxx_texture.h"
#include "middleware/khronos/glxx/glxx_shared.h"
#include "middleware/khronos/gl20/gl20_program.h"
#include "middleware/khronos/gl20/gl20_shader.h"
#include "middleware/khronos/glxx/glxx_renderbuffer.h"
#include "middleware/khronos/glxx/glxx_framebuffer.h"

/* Import code-reviewed functions */
#include "middleware/khronos/glxx/glxx_shared_cr.c"

bool glxx_shared_init(GLXX_SHARED_T *shared)
{
   shared->next_pobject = 1;
   shared->next_texture = 1;
   shared->next_buffer = 1;
   shared->next_renderbuffer = 1;
   shared->next_framebuffer = 1;

   if (!khrn_map_init(&shared->pobjects, 256))
      return false;
   if (!khrn_map_init(&shared->textures, 256))
      return false;
   if (!khrn_map_init(&shared->buffers, 256))
      return false;
   if (!khrn_map_init(&shared->renderbuffers, 256))
      return false;
   if (!khrn_map_init(&shared->framebuffers, 256))
      return false;

   return true;
}

void glxx_shared_term(void *v, uint32_t size)
{
   GLXX_SHARED_T *shared = (GLXX_SHARED_T *)v;

   UNUSED(size);

   khrn_map_term(&shared->pobjects);
   khrn_map_term(&shared->textures);
   khrn_map_term(&shared->buffers);
   khrn_map_term(&shared->renderbuffers);
   khrn_map_term(&shared->framebuffers);
}

uint32_t glxx_shared_create_program(GLXX_SHARED_T *shared)
{
   uint32_t result = 0;

   MEM_HANDLE_T handle = MEM_ALLOC_STRUCT_EX(GL20_PROGRAM_T, MEM_COMPACT_DISCARD);     // check, gl20_program_term

   if (handle != MEM_INVALID_HANDLE) {
      mem_set_term(handle, gl20_program_term);

      gl20_program_init((GL20_PROGRAM_T *)mem_lock(handle), shared->next_pobject);
      mem_unlock(handle);

      if (khrn_map_insert(&shared->pobjects, shared->next_pobject, handle))
         result = shared->next_pobject++;

      mem_release(handle);
   }

   return result;
}

uint32_t glxx_shared_create_shader(GLXX_SHARED_T *shared, uint32_t type)
{
   uint32_t result = 0;

   MEM_HANDLE_T handle = MEM_ALLOC_STRUCT_EX(GL20_SHADER_T, MEM_COMPACT_DISCARD);   // check, gl20_shader_term

   if (handle != MEM_INVALID_HANDLE) {
      mem_set_term(handle, gl20_shader_term);

      gl20_shader_init((GL20_SHADER_T *)mem_lock(handle), shared->next_pobject, type);
      mem_unlock(handle);

      if (khrn_map_insert(&shared->pobjects, shared->next_pobject, handle))
         result = shared->next_pobject++;

      mem_release(handle);
   }

   return result;
}

MEM_HANDLE_T glxx_shared_get_pobject(GLXX_SHARED_T *shared, uint32_t pobject)
{
   return khrn_map_lookup(&shared->pobjects, pobject);
}


MEM_HANDLE_T glxx_shared_get_renderbuffer(GLXX_SHARED_T *shared, uint32_t renderbuffer, bool create)
{
   MEM_HANDLE_T handle = khrn_map_lookup(&shared->renderbuffers, renderbuffer);

   if (create && handle == MEM_INVALID_HANDLE) {
      handle = MEM_ALLOC_STRUCT_EX(GLXX_RENDERBUFFER_T, MEM_COMPACT_DISCARD);       // check, glxx_renderbuffer_term

      if (handle != MEM_INVALID_HANDLE) {
         mem_set_term(handle, glxx_renderbuffer_term);

         glxx_renderbuffer_init((GLXX_RENDERBUFFER_T *)mem_lock(handle), renderbuffer);
         mem_unlock(handle);

         if (khrn_map_insert(&shared->renderbuffers, renderbuffer, handle))
            mem_release(handle);
         else {
            mem_release(handle);
            handle = MEM_INVALID_HANDLE;
         }
      }
   }

   return handle;
}

MEM_HANDLE_T glxx_shared_get_framebuffer(GLXX_SHARED_T *shared, uint32_t framebuffer, bool create)
{
   MEM_HANDLE_T handle = khrn_map_lookup(&shared->framebuffers, framebuffer);

   if (create && handle == MEM_INVALID_HANDLE) {
      handle = MEM_ALLOC_STRUCT_EX(GLXX_FRAMEBUFFER_T, MEM_COMPACT_DISCARD);        // check, glxx_framebuffer_term

      if (handle != MEM_INVALID_HANDLE) {
         mem_set_term(handle, glxx_framebuffer_term);

         glxx_framebuffer_init((GLXX_FRAMEBUFFER_T *)mem_lock(handle), framebuffer);
         mem_unlock(handle);

         if (khrn_map_insert(&shared->framebuffers, framebuffer, handle))
            mem_release(handle);
         else {
            mem_release(handle);
            handle = MEM_INVALID_HANDLE;
         }
      }
   }

   return handle;
}

void glxx_shared_delete_pobject(GLXX_SHARED_T *shared, uint32_t pobject)
{
   khrn_map_delete(&shared->pobjects, pobject);
}

void glxx_shared_delete_renderbuffer(GLXX_SHARED_T *shared, uint32_t renderbuffer)
{
   khrn_map_delete(&shared->renderbuffers, renderbuffer);
}

void glxx_shared_delete_framebuffer(GLXX_SHARED_T *shared, uint32_t framebuffer)
{
   khrn_map_delete(&shared->framebuffers, framebuffer);
}

/*
   MEM_HANDLE_T glxx_shared_get_or_create_texture(GLXX_SHARED_T *shared, uint32_t texture, bool is_cube, GLenum *error)

   Obtains a handle to a texture object, given a shared state object,
   creating a new one should it not already exist.
   
   The is_cube parameter controls what sort of texture is created. It also
   generates an error if there is an existing texture with that name of the
   wrong type.

   Returns either a valid handle, or MEM_INVALID_HANDLE.  MEM_INVALID_HANDLE
   indicates an error. These errors are distinguished by the error parameter:
      GL_OUT_OF_MEMORY      Texture object does not exist, and insufficient memory to create a new one
      GL_INVALID_OPERATION  Texture object does exist but is of the wrong type

   Khronos documentation:

   -

   Implementation notes:

   -

   Preconditions:

   shared is a valid pointer to a shared state object
   texture != 0
   error is a valid pointer

   Postconditions:   

   returns either a valid handle to a GLXX_TEXTURE_T object or MEM_INVALID_HANDLE
   If result == MEM_INVALID_HANDLE then
      *error in {GL_OUT_OF_MEMORY, GL_INVALID_OPERATION}
   If result != MEM_INVALID_HANDLE then
      result.is_cube == is_cube

   Invariants preserved:

   shared.textures is a valid map and the elements are valid TEXTURE_Ts 

*/

MEM_HANDLE_T glxx_shared_get_or_create_texture(GLXX_SHARED_T *shared, uint32_t texture, bool is_cube, GLenum *error, bool *has_color, bool *has_alpha, bool *complete)
{
   MEM_HANDLE_T handle = khrn_map_lookup(&shared->textures, texture);

   vcos_assert(texture);
   if (handle == MEM_INVALID_HANDLE) {
      handle = MEM_ALLOC_STRUCT_EX(GLXX_TEXTURE_T, MEM_COMPACT_DISCARD);                 // check, glxx_texture_term

      if (handle != MEM_INVALID_HANDLE) {
         mem_set_term(handle, glxx_texture_term);

         glxx_texture_init((GLXX_TEXTURE_T *)mem_lock(handle), texture, is_cube);
         mem_unlock(handle);

         if (khrn_map_insert(&shared->textures, texture, handle))
            mem_release(handle);
         else {
            mem_release(handle);
            handle = MEM_INVALID_HANDLE;
         }
      }

      *complete = false;

      if (handle == MEM_INVALID_HANDLE)
         *error = GL_OUT_OF_MEMORY;
   } else {
      GLXX_TEXTURE_T *texture = (GLXX_TEXTURE_T *)mem_lock(handle);
      bool fail = texture->is_cube != is_cube;
      glxx_texture_has_color_alpha(texture, has_color, has_alpha, complete);
      mem_unlock(handle);
      
      if (fail) {
         *error = GL_INVALID_OPERATION;
         handle = MEM_INVALID_HANDLE;
      }
   }

   return handle;
}
