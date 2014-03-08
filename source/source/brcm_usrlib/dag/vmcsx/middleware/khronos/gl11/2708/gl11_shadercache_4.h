/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GL11_SHADERCACHE_4_H
#define GL11_SHADERCACHE_4_H

#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/common/khrn_mem.h"
#include "interface/khronos/glxx/gl11_int_config.h"
#include "middleware/khronos/gl11/gl11_matrix.h"
#include "middleware/khronos/gl11/gl11_texunit.h"
#include "middleware/khronos/gl11/gl11_server.h"
#include "middleware/khronos/glxx/2708/glxx_inner_4.h"
#include "interface/khronos/include/GLES/gl.h"
#include "middleware/khronos/glxx/glxx_hw.h"

extern void gl11_hw_shader_cache_reset(void);

typedef struct {
   bool used;
   GL11_CACHE_KEY_T key;
   GLXX_LINK_RESULT_DATA_T data;
   uint32_t color_varyings;
} GL11_CACHE_ENTRY_T;

extern bool gl11_hw_get_shaders(
    GLXX_HW_SHADER_RECORD_T *shader_out,
    MEM_HANDLE_T *cunifmap_out,
    MEM_HANDLE_T *vunifmap_out,
    MEM_HANDLE_T *funifmap_out,
    uint32_t *color_varyings_out,
    GLXX_SERVER_STATE_T *state,
    GLXX_ATTRIB_T *attrib,
    uint32_t *mergeable_attribs,
    uint32_t * cattribs_order_out,
    uint32_t * vattribs_order_out);

#endif
