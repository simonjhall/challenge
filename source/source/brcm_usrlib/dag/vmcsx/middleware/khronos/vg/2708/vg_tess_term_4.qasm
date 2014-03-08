### ============================================================================
### Copyright (c) 2008-2014, Broadcom Corporation
### All rights reserved.
### Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
### 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
### 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
### 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
### THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
### ============================================================================

.include "vg_util_4.qinc"

::vg_tess_term_shader

   # 2 common rows at 15 and 31
   mov  vw_setup, vdw_setup_0(1, 16, dma_h32(15, 0))
   mov  vw_addr, a:vg_tess_shader_if + 4
   mov  vw_setup, vdw_setup_0(1, 16, dma_h32(31, 0))
   mov  vw_addr, a:vg_tess_shader_if + 4 + 64

   # 12 * 8 bytes at 0, 5, 10, 16, 21, 26, 32, 37, 42, 48, 53, 58
   .rep i, 12
      mov  vw_setup, vdw_setup_0(1, 2, dma_h32(((i / 3) * 16) + ((i % 3) * 5), 0))
      mov  vw_addr, a:vg_tess_shader_if + 4 + 128 + (i * 8)
   .endr

   # write -2 to indicate we're done
   mov  vw_setup, vpm_setup(1, 0, h32(1))
   mov  vpm, -2
   mov  vw_setup, vdw_setup_0(1, 1, dma_h32(1, 0))
   mov  vw_addr, a:vg_tess_shader_if
   mov  -, vw_wait

   # prod host and end
   mov  interrupt, 1
   thrend
   nop
   nop
