/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GL20_SHADER_H
#define GL20_SHADER_H

#include "interface/khronos/include/GLES2/gl2.h"

#include "middleware/khronos/common/khrn_mem.h"

/*
   The state required per shader object consists of:

   - An unsigned integer specifying the shader object name.
   X An integer holding the value of SHADER TYPE.
   X A boolean holding the delete status, initially FALSE.
   X A boolean holding the status of the last compile, initially FALSE.
   U An array of type char containing the information log, initially empty.
   U An integer holding the length of the information log.
   X An array of type char containing the concatenated shader string, initially empty.
   X An integer holding the length of the concatenated shader string.
*/

#define SIG_SHADER 0x0054ade7

typedef struct {
   uint32_t sig;
   int32_t refs;
   int32_t name;

   GLboolean deleted;
   GLboolean compiled;

   GLenum type;

   MEM_HANDLE_T mh_sources_current;
   MEM_HANDLE_T mh_sources_compile;

   MEM_HANDLE_T mh_info;
} GL20_SHADER_T;

extern void gl20_shader_init(GL20_SHADER_T *shader, int32_t name, GLenum type);
extern void gl20_shader_term(void *v, uint32_t size);

extern void gl20_shader_sources_term(void *v, uint32_t size);

extern void gl20_shader_acquire(GL20_SHADER_T *shader);
extern void gl20_shader_release(GL20_SHADER_T *shader);

extern void gl20_shader_compile(GL20_SHADER_T *shader);

extern int gl20_is_shader(GL20_SHADER_T *shader);

#endif