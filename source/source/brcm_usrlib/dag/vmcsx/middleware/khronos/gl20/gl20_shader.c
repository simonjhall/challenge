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

#include "middleware/khronos/gl20/gl20_shader.h"

void gl20_shader_init(GL20_SHADER_T *shader, int32_t name, GLenum type)
{
   vcos_assert(shader);

   vcos_assert(shader->mh_sources_current == MEM_INVALID_HANDLE);
   vcos_assert(shader->mh_sources_compile == MEM_INVALID_HANDLE);
   vcos_assert(shader->mh_info == MEM_INVALID_HANDLE);

   shader->sig = SIG_SHADER;
   shader->refs = 0;
   shader->name = name;

   shader->deleted = GL_FALSE;
   shader->compiled = GL_FALSE;

   shader->type = type;

   MEM_ASSIGN(shader->mh_sources_current, MEM_ZERO_SIZE_HANDLE);
   MEM_ASSIGN(shader->mh_sources_compile, MEM_ZERO_SIZE_HANDLE);
   MEM_ASSIGN(shader->mh_info, MEM_EMPTY_STRING_HANDLE);
}

void gl20_shader_term(void *v, uint32_t size)
{
   GL20_SHADER_T *shader = (GL20_SHADER_T *)v;

   UNUSED(size);

   MEM_ASSIGN(shader->mh_sources_current, MEM_INVALID_HANDLE);
   MEM_ASSIGN(shader->mh_sources_compile, MEM_INVALID_HANDLE);

   MEM_ASSIGN(shader->mh_info, MEM_INVALID_HANDLE);
}

void gl20_shader_sources_term(void *v, uint32_t size)
{
   MEM_HANDLE_T *base = (MEM_HANDLE_T *)v;

   int i, count = size / sizeof(MEM_HANDLE_T);

   vcos_assert(size % sizeof(MEM_HANDLE_T) == 0);

   for (i = 0; i < count; i++)
      MEM_ASSIGN(base[i], MEM_INVALID_HANDLE);
}

void gl20_shader_acquire(GL20_SHADER_T *shader)
{
   vcos_assert(shader);
   vcos_assert(shader->refs >= 0);

   shader->refs++;

   vcos_assert(shader->refs >= 0);
}

void gl20_shader_release(GL20_SHADER_T *shader)
{
   vcos_assert(shader);
   vcos_assert(shader->refs >= 0);

   shader->refs--;

   vcos_assert(shader->refs >= 0);
}

void gl20_shader_compile(GL20_SHADER_T *shader)
{
   /*
      we defer compilation to link-time, but need to snapshot
      the current source
   */

   MEM_ASSIGN(shader->mh_sources_compile, shader->mh_sources_current);

   shader->compiled = GL_TRUE;
}

int gl20_is_shader(GL20_SHADER_T *shader)
{
   return shader->sig == SIG_SHADER;
}
