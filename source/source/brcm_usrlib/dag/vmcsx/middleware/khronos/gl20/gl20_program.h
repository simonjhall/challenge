/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GL20_PROGRAM_H
#define GL20_PROGRAM_H

#include "interface/khronos/include/GLES2/gl2.h"

#include "middleware/khronos/common/khrn_mem.h"

#include "middleware/khronos/gl20/gl20_config.h"

#ifdef __VIDEOCORE4__
#include "middleware/khronos/gl20/2708/gl20_shader_4.h"
#endif
#include "middleware/khronos/glxx/glxx_server.h"

/*
   The state required per program object consists of:

   N An unsigned integer indicating the program object object name.
   X A boolean holding the delete status, initially FALSE.
   X A boolean holding the status of the last link attempt, initially FALSE.
   X A boolean holding the status of the last validation attempt, initally FALSE.
   X An integer holding the number of attached shader objects.
   X A list of unsigned integers to keep track of the names of the shader objects attached.
   U An array of type char containing the information log, initially empty.
   U An integer holding the length of the information log.
   - An integer holding the number of active uniforms.
   - For each active uniform, three integers, holding its location, size, and type,
     and an array of type char holding its name.
   - An array of words that hold the values of each active uniform.
   - An integer holding the number of active attributes.
   - For each active attbribute, three integers holding its location, size, and type,
     and an array of type char holding its name.
*/


typedef struct {
   uint32_t index;

   MEM_HANDLE_T mh_name;
} GL20_BINDING_T;

extern void gl20_bindings_term(void *v, uint32_t size);

extern void gl20_uniform_info_term(void *v, uint32_t size);

typedef struct {
//   GLint location;
   GLint offset;
   GLenum type;

   MEM_HANDLE_T mh_name;
} GL20_ATTRIB_INFO_T;

extern void gl20_attrib_info_term(void *v, uint32_t size);

#define SIG_PROGRAM 0x097067a8

/*
   We have a conceptual boolean variable conceptual_programs_owned_by_master which
   represents our ability to modify from within the master process.

               gl20_hw_draw_triangles
                ------------------->
           true                     false
                <-------------------
                  khrn_fifo_finish
*/

typedef struct {
   uint32_t sig;
   int32_t refs;
   int32_t name;

   GLboolean deleted;
   GLboolean linked;
   GLboolean validated;
   
   MEM_HANDLE_T mh_vertex;
   MEM_HANDLE_T mh_fragment;

   MEM_HANDLE_T mh_bindings;

#ifdef __VIDEOCORE4__
   GL20_LINK_RESULT_T result;
#else
   int32_t varyings;

   MEM_HANDLE_T mh_vertex_code;
   MEM_HANDLE_T mh_fragment_code;
#endif
   int32_t attribs_live;

   MEM_HANDLE_T mh_sampler_info;

   MEM_HANDLE_T mh_uniform_info;
   MEM_HANDLE_T mh_uniform_data;
   int uniforms_updated;

   MEM_HANDLE_T mh_attrib_info;

   MEM_HANDLE_T mh_info;
#ifdef UNIFORM_LOCATION_IS_16_BIT
   int uniform_index_mask_bits;
#endif
} GL20_PROGRAM_T;

extern void gl20_program_init(GL20_PROGRAM_T *program, int32_t name);
extern void gl20_program_term(void *v, uint32_t size);

extern bool gl20_program_bind_attrib(GL20_PROGRAM_T *program, uint32_t index, const char *name);

extern void gl20_program_acquire(GL20_PROGRAM_T *program);
extern void gl20_program_release(GL20_PROGRAM_T *program);

extern void gl20_program_link(GL20_PROGRAM_T *program);

extern int gl20_is_program(GL20_PROGRAM_T *program);

extern GLboolean gl20_validate_current_program(GLXX_SERVER_STATE_T *state);
extern GLboolean gl20_validate_program(GLXX_SERVER_STATE_T *state, GL20_PROGRAM_T *program);
#endif
