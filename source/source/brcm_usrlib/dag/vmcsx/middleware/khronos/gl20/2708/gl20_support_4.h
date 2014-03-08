/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GL20_SUPPORT_4_H
#define GL20_SUPPORT_4_H

#include "middleware/khronos/glxx/2708/glxx_inner_4.h"


typedef struct
{
#define IU_MAX 3
   uint32_t index[IU_MAX];
   uint32_t size[IU_MAX];
   uint32_t *addr[IU_MAX];
   uint32_t count;
} GL20_HW_INDEXED_UNIFORM_T;

 
//extern uint32_t gl20_install_tex_param(GLXX_SERVER_STATE_T *state, GL20_PROGRAM_T *program, uint32_t *data, GLXX_FIXABLE_BUF_T *buf, uint32_t u0, uint32_t u1);   

extern void gl20_hw_iu_init(GL20_HW_INDEXED_UNIFORM_T *iu);
extern void gl20_hw_iu_close(GL20_HW_INDEXED_UNIFORM_T *iu);

#endif