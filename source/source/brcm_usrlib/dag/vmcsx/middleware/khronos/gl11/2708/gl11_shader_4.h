/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GL11_SHADER_4_H
#define GL11_SHADER_4_H

#include "interface/khronos/glxx/glxx_int_attrib.h"
#include "middleware/khronos/gl11/2708/gl11_shadercache_4.h"
#include "middleware/khronos/glxx/2708/glxx_inner_4.h"

#define UNIF_VIEWPORT 0xc0000030                         /* [scale_x, scale_y, scale_z, offset_z] */
#define UNIF_ATTR_COLOR 0xc0000200
#define UNIF_ATTR_NORMAL 0xc0000204
#define UNIF_ATTR_TEXTURE_COORD(t) (0xc0000208+4*(t))
#define UNIF_ATTR_POINT_SIZE 0xc0000218

extern bool gl11_hw_emit_shaders(GL11_CACHE_KEY_T *v, GLXX_LINK_RESULT_DATA_T *data, uint32_t *color_varyings);


#endif
