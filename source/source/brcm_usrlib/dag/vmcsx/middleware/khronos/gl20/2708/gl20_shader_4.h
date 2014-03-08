/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GL20_SHADER_4_H
#define GL20_SHADER_4_H

#include "middleware/khronos/glxx/glxx_server.h"
#include "middleware/khronos/glxx/glxx_hw.h"
#include "middleware/khronos/glxx/2708/glxx_inner_4.h"



typedef struct
{
   GLXX_LINK_RESULT_KEY_T key;
   GLXX_LINK_RESULT_DATA_T data;
   bool used;
} GL20_LINK_RESULT_CACHE_ENTRY_T;

#define GL20_LINK_RESULT_CACHE_SIZE 4
#define GL20_LINK_RESULT_NODE_COUNT 42

typedef struct
{
   MEM_HANDLE_T mh_blob;
   void *nodes[GL20_LINK_RESULT_NODE_COUNT];
   uint32_t vary_count;

   GL20_LINK_RESULT_CACHE_ENTRY_T cache[GL20_LINK_RESULT_CACHE_SIZE];
   uint32_t cache_used;
   uint32_t cache_next;

   uint32_t vattribs_live;    /* TODO duplicated information - also in GL20_PROGRAM_T */
   uint32_t cattribs_live;
} GL20_LINK_RESULT_T;

extern void gl20_link_result_term(void *v, uint32_t size);
extern bool gl20_link_result_get_shaders(
   GL20_LINK_RESULT_T *link_result,
   GLXX_HW_SHADER_RECORD_T *shader_out,
   MEM_HANDLE_T *cunifmap_out,
   MEM_HANDLE_T *vunifmap_out,
   MEM_HANDLE_T *funifmap_out,
   GLXX_SERVER_STATE_T *state,
   GLXX_ATTRIB_T *attrib,
   uint32_t *mergeable_attribs,
   uint32_t * cattribs_order_out,
   uint32_t * vattribs_order_out);
   
extern bool gl20_hw_emit_shaders(GL20_LINK_RESULT_T *link_result, GLXX_LINK_RESULT_KEY_T *key, GLXX_LINK_RESULT_DATA_T *data, void *base);

#endif   
