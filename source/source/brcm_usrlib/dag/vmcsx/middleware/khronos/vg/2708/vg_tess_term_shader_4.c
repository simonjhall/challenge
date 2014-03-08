// generated 2010-08-20 21:24:29
/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include "middleware/khronos/vg/2708/vg_tess_term_shader_4.h"
#include <assert.h>
#ifdef SIMPENROSE
#include <stddef.h>
#include "v3d/verification/tools/2760sim/simpenrose.h"

unsigned int const *const vg_tess_term_shader_annotations_array[] = {
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
};
#endif

static unsigned int const array[] = {
// ::vg_tess_term_shader
/* [0x00000000] */ 0x80904780, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 16, dma_h32(15, 0))
/* [0x00000008] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if + 4
/* [0x00000010] */ 0x80904f80, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 16, dma_h32(31, 0))
/* [0x00000018] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if + 4 + 64
/* [0x00000020] */ 0x80824000, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 2, dma_h32(((i / 3) * 16) + ((i % 3) * 5), 0))
/* [0x00000028] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if + 4 + 128 + (i * 8)
/* [0x00000030] */ 0x80824280, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 2, dma_h32(((i / 3) * 16) + ((i % 3) * 5), 0))
/* [0x00000038] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if + 4 + 128 + (i * 8)
/* [0x00000040] */ 0x80824500, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 2, dma_h32(((i / 3) * 16) + ((i % 3) * 5), 0))
/* [0x00000048] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if + 4 + 128 + (i * 8)
/* [0x00000050] */ 0x80824800, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 2, dma_h32(((i / 3) * 16) + ((i % 3) * 5), 0))
/* [0x00000058] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if + 4 + 128 + (i * 8)
/* [0x00000060] */ 0x80824a80, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 2, dma_h32(((i / 3) * 16) + ((i % 3) * 5), 0))
/* [0x00000068] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if + 4 + 128 + (i * 8)
/* [0x00000070] */ 0x80824d00, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 2, dma_h32(((i / 3) * 16) + ((i % 3) * 5), 0))
/* [0x00000078] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if + 4 + 128 + (i * 8)
/* [0x00000080] */ 0x80825000, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 2, dma_h32(((i / 3) * 16) + ((i % 3) * 5), 0))
/* [0x00000088] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if + 4 + 128 + (i * 8)
/* [0x00000090] */ 0x80825280, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 2, dma_h32(((i / 3) * 16) + ((i % 3) * 5), 0))
/* [0x00000098] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if + 4 + 128 + (i * 8)
/* [0x000000a0] */ 0x80825500, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 2, dma_h32(((i / 3) * 16) + ((i % 3) * 5), 0))
/* [0x000000a8] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if + 4 + 128 + (i * 8)
/* [0x000000b0] */ 0x80825800, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 2, dma_h32(((i / 3) * 16) + ((i % 3) * 5), 0))
/* [0x000000b8] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if + 4 + 128 + (i * 8)
/* [0x000000c0] */ 0x80825a80, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 2, dma_h32(((i / 3) * 16) + ((i % 3) * 5), 0))
/* [0x000000c8] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if + 4 + 128 + (i * 8)
/* [0x000000d0] */ 0x80825d00, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 2, dma_h32(((i / 3) * 16) + ((i % 3) * 5), 0))
/* [0x000000d8] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if + 4 + 128 + (i * 8)
/* [0x000000e0] */ 0x00100a01, 0xe0021c67, // mov  vw_setup, vpm_setup(1, 0, h32(1))
/* [0x000000e8] */ 0xfffffffe, 0xe0020c27, // mov  vpm, -2
/* [0x000000f0] */ 0x80814080, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 1, dma_h32(1, 0))
/* [0x000000f8] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if
/* [0x00000100] */ 0x159f2fc0, 0x100009e7, // mov  -, vw_wait
/* [0x00000108] */ 0x00000001, 0xe00209a7, // mov  interrupt, 1
/* [0x00000110] */ 0x009e7000, 0x300009e7, // thrend
/* [0x00000118] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000120] */ 0x009e7000, 0x100009e7, // nop
};

void vg_tess_term_shader_link(void *p_in, unsigned int base
   , unsigned int vg_tess_shader_if
   )
{
   unsigned int *p = (unsigned int *)p_in;
   unsigned int i;
   for (i = 0; i != (VG_TESS_TERM_SHADER_SIZE / 4); ++i) {
      p[i] = array[i];
   }
   assert(p[2] == 0xdeadbeef);
   p[2] = vg_tess_shader_if + 4;
   assert(p[6] == 0xdeadbeef);
   p[6] = vg_tess_shader_if + 68;
   assert(p[10] == 0xdeadbeef);
   p[10] = vg_tess_shader_if + 132;
   assert(p[14] == 0xdeadbeef);
   p[14] = vg_tess_shader_if + 140;
   assert(p[18] == 0xdeadbeef);
   p[18] = vg_tess_shader_if + 148;
   assert(p[22] == 0xdeadbeef);
   p[22] = vg_tess_shader_if + 156;
   assert(p[26] == 0xdeadbeef);
   p[26] = vg_tess_shader_if + 164;
   assert(p[30] == 0xdeadbeef);
   p[30] = vg_tess_shader_if + 172;
   assert(p[34] == 0xdeadbeef);
   p[34] = vg_tess_shader_if + 180;
   assert(p[38] == 0xdeadbeef);
   p[38] = vg_tess_shader_if + 188;
   assert(p[42] == 0xdeadbeef);
   p[42] = vg_tess_shader_if + 196;
   assert(p[46] == 0xdeadbeef);
   p[46] = vg_tess_shader_if + 204;
   assert(p[50] == 0xdeadbeef);
   p[50] = vg_tess_shader_if + 212;
   assert(p[54] == 0xdeadbeef);
   p[54] = vg_tess_shader_if + 220;
   assert(p[62] == 0xdeadbeef);
   p[62] = vg_tess_shader_if + 0;
}
