### ============================================================================
### Copyright (c) 2008-2014, Broadcom Corporation
### All rights reserved.
### Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
### 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
### 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
### 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
### THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
### ============================================================================
### ===========================================================================
###
### FILE DESCRIPTION
### Restores persistent VPM stuff used by fill/stroke user shaders.
###
### vg_tess_init_shader_4.c/h are generated from vg_tess_init_4.qasm by:
### qasm -mml_c:middleware/khronos/vg/2708/vg_tess_init_shader_4,VG_TESS_INIT_SHADER_4,vg_tess_init_shader,annots -tb0 vg_tess_init_4.qasm >vg_tess_init_shader_4.c
### and:
### qasm -mml_h:middleware/khronos/vg/2708/vg_tess_init_shader_4,VG_TESS_INIT_SHADER_4,vg_tess_init_shader,annots -tb0 vg_tess_init_4.qasm >vg_tess_init_shader_4.h
### So don't edit vg_tess_init_shader_4.c/h directly.
### ===========================================================================

.include "vg_util_4.qinc"

::vg_tess_init_shader

   # 2 common rows at 15 and 31
   mov  vr_setup, vdr_setup_0(2, 16, dma_h32(15, 0), 16, 64)
   mov  vr_addr, a:vg_tess_shader_if + 4

   # 12 * 8 bytes at 0, 5, 10, 16, 21, 26, 32, 37, 42, 48, 53, 58
   .rep i, 4
      mov  vr_setup, vdr_setup_0(3, 2, dma_h32(i * 16, 0), 5, 8)
      mov  vr_setup, vdr_setup_1(8)
      mov  vr_addr, a:vg_tess_shader_if + 4 + 128 + (i * 24)
   .endr

   mov  -, vr_wait

   # write -1 to indicate we're done
   mov  vw_setup, vpm_setup(1, 0, h32(1))
   mov  vpm, -1
   mov  vw_setup, vdw_setup_0(1, 1, dma_h32(1, 0))
   mov  vw_addr, a:vg_tess_shader_if
   mov  -, vw_wait

   # prod host and end
   mov  interrupt, 1
   thrend
   nop
   nop
