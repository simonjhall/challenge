// generated 2010-09-16 22:09:41
/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include "middleware/khronos/vg/2708/vg_tess_init_shader_4.h"
#include <assert.h>
#ifdef SIMPENROSE
#include <stddef.h>
#include "v3d/verification/tools/2760sim/simpenrose.h"

unsigned int const *const vg_tess_init_shader_annotations_array[] = {
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
// ::vg_tess_init_shader
/* [0x00000000] */ 0x830200f0, 0xe0020c67, // mov  vr_setup, vdr_setup_0(2, 16, dma_h32(15, 0), 16, 64)
/* [0x00000008] */ 0xdeadbeef, 0xe0020ca7, // mov  vr_addr, a:vg_tess_shader_if + 4
/* [0x00000010] */ 0x80235000, 0xe0020c67, // mov  vr_setup, vdr_setup_0(3, 2, dma_h32(i * 16, 0), 5, 8)
/* [0x00000018] */ 0x90000008, 0xe0020c67, // mov  vr_setup, vdr_setup_1(8)
/* [0x00000020] */ 0xdeadbeef, 0xe0020ca7, // mov  vr_addr, a:vg_tess_shader_if + 4 + 128 + (i * 24)
/* [0x00000028] */ 0x80235100, 0xe0020c67, // mov  vr_setup, vdr_setup_0(3, 2, dma_h32(i * 16, 0), 5, 8)
/* [0x00000030] */ 0x90000008, 0xe0020c67, // mov  vr_setup, vdr_setup_1(8)
/* [0x00000038] */ 0xdeadbeef, 0xe0020ca7, // mov  vr_addr, a:vg_tess_shader_if + 4 + 128 + (i * 24)
/* [0x00000040] */ 0x80235200, 0xe0020c67, // mov  vr_setup, vdr_setup_0(3, 2, dma_h32(i * 16, 0), 5, 8)
/* [0x00000048] */ 0x90000008, 0xe0020c67, // mov  vr_setup, vdr_setup_1(8)
/* [0x00000050] */ 0xdeadbeef, 0xe0020ca7, // mov  vr_addr, a:vg_tess_shader_if + 4 + 128 + (i * 24)
/* [0x00000058] */ 0x80235300, 0xe0020c67, // mov  vr_setup, vdr_setup_0(3, 2, dma_h32(i * 16, 0), 5, 8)
/* [0x00000060] */ 0x90000008, 0xe0020c67, // mov  vr_setup, vdr_setup_1(8)
/* [0x00000068] */ 0xdeadbeef, 0xe0020ca7, // mov  vr_addr, a:vg_tess_shader_if + 4 + 128 + (i * 24)
/* [0x00000070] */ 0x15ca7d80, 0x100009e7, // mov  -, vr_wait
/* [0x00000078] */ 0x00100a01, 0xe0021c67, // mov  vw_setup, vpm_setup(1, 0, h32(1))
/* [0x00000080] */ 0xffffffff, 0xe0020c27, // mov  vpm, -1
/* [0x00000088] */ 0x80814080, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 1, dma_h32(1, 0))
/* [0x00000090] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if
/* [0x00000098] */ 0x159f2fc0, 0x100009e7, // mov  -, vw_wait
/* [0x000000a0] */ 0x00000001, 0xe00209a7, // mov  interrupt, 1
/* [0x000000a8] */ 0x009e7000, 0x300009e7, // thrend
/* [0x000000b0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000000b8] */ 0x009e7000, 0x100009e7, // nop
};

void vg_tess_init_shader_link(void *p_in, unsigned int base
   , unsigned int vg_tess_shader_if
   )
{
   unsigned int *p = (unsigned int *)p_in;
   unsigned int i;
   for (i = 0; i != (VG_TESS_INIT_SHADER_SIZE / 4); ++i) {
      p[i] = array[i];
   }
   assert(p[2] == 0xdeadbeef);
   p[2] = vg_tess_shader_if + 4;
   assert(p[8] == 0xdeadbeef);
   p[8] = vg_tess_shader_if + 132;
   assert(p[14] == 0xdeadbeef);
   p[14] = vg_tess_shader_if + 156;
   assert(p[20] == 0xdeadbeef);
   p[20] = vg_tess_shader_if + 180;
   assert(p[26] == 0xdeadbeef);
   p[26] = vg_tess_shader_if + 204;
   assert(p[36] == 0xdeadbeef);
   p[36] = vg_tess_shader_if + 0;
}
