;;; ===========================================================================
;;; Copyright (c) 2006-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.include "vcinclude/common.inc"

.FORGET
.stabs "asm_file",100,0,0,rgb888_load_initial_block


;;; NAME
;;;    rgb888_load_initial_block
;;;
;;; SYNOPSIS
;;;    void rgb888_load_initial_block(uint8_t *addr, int pitch, int vrf_row);
;;;
;;; FUNCTION
;;;    Load a 16x16 byte block into the VRF row indicated.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_load_initial_block,"text"
.globl rgb888_load_initial_block
rgb888_load_initial_block:
      vld H(0++,0)+r2*, (r0+=r1) REP 16
      b lr
.stabs "rgb888_load_initial_block:F1",36,0,0,rgb888_load_initial_block

;;; NAME
;;;    rgb888_load_blocks
;;;
;;; SYNOPSIS
;;;    void rgb888_load_blocks(uint8_t *addr, int pitch, int vrf_row);
;;;
;;; FUNCTION
;;;    Load 3 16x16 byte blocks into the VRF row indicated.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_load_blocks,"text"
.globl rgb888_load_blocks
rgb888_load_blocks:
      vld H(0++,16)+r2*, (r0+16+=r1) REP 16
      vld H(0++,32)+r2*, (r0+32+=r1) REP 16
      vld H(0++,48)+r2*, (r0+48+=r1) REP 16
      b lr


;;; NAME
;;;    load_block_1bpp
;;;
;;; SYNOPSIS
;;;    void load_block_1bpp(uint8_t *addr, int pitch, int vrf_off, int invert);
;;;
;;; FUNCTION
;;;    Load into VRF row 32 and unpack a 16x16 block of a 1bpp image. invert is 0 or -1
;;;    in which case the relevent bits are NOTted.
;;;
;;; RETURNS
;;;    -

.section .text$load_block_1bpp,"text"
.globl load_block_1bpp
load_block_1bpp:
      vldleft HX(32++,0)+r2, (r0+=r1) REP 16
      veor VX(32,0)+r2, VX(32,0)+r2, r3
      vbitplanes HX(32++,0)+r2, HX(32++,0)+r2 REP 16
      vand HX(32++,0)+r2, HX(32++,0)+r2, 1 REP 16
      b lr

;;; NAME
;;;    rgb888_save_blocks
;;;
;;; SYNOPSIS
;;;    void rgb888_save_blocks(uint8_t *addr, int pitch, int nrows);
;;;
;;; FUNCTION
;;;    Save 3 16 x nrows byte blocks from the top of the VRF.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_save_blocks,"text"
.globl rgb888_save_blocks
rgb888_save_blocks:
      addcmpbeq r2, 0, 16, rgb888_save_blocks_16_rows
      mov r3, 0
      shl r2, 6            ; limit of vrf row address to save
      mov r4, 64           ; vrf address increment by one row
rgb888_save_blocks_n_rows:
      vst H(0,0)+r3*, (r0)
      vst H(0,16)+r3*, (r0+16)
      vst H(0,32)+r3*, (r0+32)
      add r0, r1
      addcmpbne r3, r4, r2, rgb888_save_blocks_n_rows
      b lr     
rgb888_save_blocks_16_rows:
      vst H(0++,0)*, (r0+=r1) REP 16
      vst H(0++,16)*, (r0+16+=r1) REP 16
      vst H(0++,32)*, (r0+32+=r1) REP 16
      b lr



   
;;; NAME
;;;    rgb888_save_final_block
;;;
;;; SYNOPSIS
;;;    void rgb888_save_final_block(uint8_t *addr, int pitch, int nrows);
;;;
;;; FUNCTION
;;;    Save final 16 x nrows byte block from the top of the VRF.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_save_final_block,"text"
.globl rgb888_save_final_block
rgb888_save_final_block:
      addcmpbeq r2, 0, 16, rgb888_save_final_block_16_rows
      mov r3, 0
      shl r2, 6            ; limit of vrf row address to save
      mov r4, 64           ; vrf address increment by one row
rgb888_save_final_block_n_rows:
      vst H(0,0)+r3*, (r0)
      add r0, r1
      addcmpbne r3, r4, r2, rgb888_save_final_block_n_rows
      b lr     
rgb888_save_final_block_16_rows:
      vst H(0++,0)*, (r0+=r1) REP 16
      b lr

;;; NAME
;;;    rgb565_to_rgb888
;;;
;;; SYNOPSIS
;;;    void rgb565_to_rgb888(unsigned char *dest, unsigned char *src, int dest_pitch, int src_pitch)
;;;
;;; FUNCTION
;;;    Convert 16x16 block from 16 to 24bpp.
;;;
;;; RETURNS
;;;    -

.section .text$rgb565_to_rgb888,"text"
.globl rgb565_to_rgb888
rgb565_to_rgb888:
      vld HX(0++,0), (r1+=r3) REP 16
      mov r3, 0
      mov r4, 0
rgb565_to_rgb888_loop:  
.ifdef RGB888
      vshl V(16,2)+r4, VX(0,0)+r3, 3      ; B
      vlsr V(16,1)+r4, VX(0,0)+r3, 5   
      vshl V(16,1)+r4, V(16,1)+r4, 2      ; G
      vand V(16,0)+r4, V(0,16)+r3, 0xf8   ; R
.else
      vshl V(16,0)+r4, VX(0,0)+r3, 3      ; B
      vlsr V(16,1)+r4, VX(0,0)+r3, 5   
      vshl V(16,1)+r4, V(16,1)+r4, 2      ; G
      vand V(16,2)+r4, V(0,16)+r3, 0xf8   ; R
.endif
      add r4, 3
      addcmpblt r3, 1, 16, rgb565_to_rgb888_loop
      vst H(16++,0), (r0+=r2) REP 16
      vst H(16++,16), (r0+16+=r2) REP 16
      vst H(16++,32), (r0+32+=r2) REP 16
      b lr  

;;; NAME
;;;    rgb565_to_rgb888_overlay
;;;
;;; SYNOPSIS
;;;    void rgb565_to_rgb888_overlay(unsigned char *dest, unsigned char *src, int dest_pitch, int src_pitch, int transparent, unsigned short mask, int lines)
;;;
;;; FUNCTION
;;;    Overlay 16x16 block from 16 to 24bpp.
;;;
;;; RETURNS
;;;    -

.section .text$rgb565_to_rgb888_overlay,"text"
.globl rgb565_to_rgb888_overlay
rgb565_to_rgb888_overlay:
      vldleft HX(0++,0), (r1+=r3) REP 16
;      vldright HX(0++,0), (r1+=r3) REP 16
      vld H(16++,0), (r0+=r2) REP 16
      vld H(16++,16), (r0+16+=r2) REP 16
      vld H(16++,32), (r0+32+=r2) REP 16
                vbitplanes -, r5                SETF
                vmov HX(0++,0), r4              IFZ REP 16
      mov r3, 0
      mov r1, 0
.rgb565_to_rgb888_ov_loop:
                veor -, VX(0,0)+r3, r4          SETF
.ifdef RGB888
      vshl V(16,2)+r1, VX(0,0)+r3, 3   IFNZ  ; B
      vlsr V(16,1)+r1, VX(0,0)+r3, 5   IFNZ
      vshl V(16,1)+r1, V(16,1)+r1, 2   IFNZ  ; G
      vand V(16,0)+r1, V(0,16)+r3, 0xf8 IFNZ ; R
.else
      vshl V(16,0)+r1, VX(0,0)+r3, 3   IFNZ  ; B
      vlsr V(16,1)+r1, VX(0,0)+r3, 5   IFNZ
      vshl V(16,1)+r1, V(16,1)+r1, 2   IFNZ  ; G
      vand V(16,2)+r1, V(0,16)+r3, 0xf8 IFNZ ; R
.endif
      add r1, 3
      addcmpblt r3, 1, 16, .rgb565_to_rgb888_ov_loop
                ld  r1, (sp)
                addcmpblt r1, 0, 16, .rgb565ov_skip16
      vst H(16++,0), (r0+=r2) REP 16
      vst H(16++,16), (r0+16+=r2) REP 16
      vst H(16++,32), (r0+32+=r2) REP 16
                b lr
.rgb565ov_skip16:
               mov r3, 0
               btest r1, 3
               beq .rgb565ov_skip8
      vst H(16++,0)+r3, (r0+=r2) REP 8
      vst H(16++,16)+r3, (r0+16+=r2) REP 8
      vst H(16++,32)+r3, (r0+32+=r2) REP 8
                addscale r0, r2 << 3
                add r3, 8*64
.rgb565ov_skip8:
               btest r1, 2
               beq .rgb565ov_skip4
      vst H(16++,0)+r3, (r0+=r2) REP 4
      vst H(16++,16)+r3, (r0+16+=r2) REP 4
      vst H(16++,32)+r3, (r0+32+=r2) REP 4
                addscale r0, r2 << 2
                add r3, 4*64
.rgb565ov_skip4:
               btest r1, 1
               beq .rgb565ov_skip2
      vst H(16++,0)+r3, (r0+=r2) REP 2
      vst H(16++,16)+r3, (r0+16+=r2) REP 2
      vst H(16++,32)+r3, (r0+32+=r2) REP 2
                addscale r0, r2 << 1
                add r3, 2*64
.rgb565ov_skip2:
               btest r1, 0
               beq .rgb565ov_skip1
      vst H(16,0)+r3, (r0)
      vst H(16,16)+r3, (r0+16)
      vst H(16,32)+r3, (r0+32)
.rgb565ov_skip1:
      b lr

;;; NAME
;;;    rgb888_to_rgb565
;;;
;;; SYNOPSIS
;;;    void rgb888_to_rgb555(unsigned char *dest, unsigned char *src, int dest_pitch, int src_pitch)
;;;
;;; FUNCTION
;;;    Convert 16x16 block from 24 to 16bpp.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_to_rgb565,"text"
.globl rgb888_to_rgb565
rgb888_to_rgb565:
      vld H(0++,0), (r1+=r3) REP 16
      vld H(0++,16), (r1+16+=r3) REP 16
      vld H(0++,32), (r1+32+=r3) REP 16
      mov r3, 0
      mov r4, 0
rgb888_to_rgb565_loop:  
.ifdef RGB888
      vlsr -, V(0,2)+r4, 3 CLRA ACC ; B
      vlsr V(16,0)+r3, V(0,1)+r4, 2
      vshl -, V(16,0)+r3, 5 ACC  ; G
      vlsr V(16,0)+r3, V(0,0)+r4, 3
      vshl VX(16,0)+r3, V(16,0)+r3, 11 ACC   ; R
.else
      vlsr -, V(0,0)+r4, 3 CLRA ACC ; B
      vlsr V(16,0)+r3, V(0,1)+r4, 2
      vshl -, V(16,0)+r3, 5 ACC  ; G
      vlsr V(16,0)+r3, V(0,2)+r4, 3
      vshl VX(16,0)+r3, V(16,0)+r3, 11 ACC   ; R
.endif
      add r4, 3
      addcmpblt r3, 1, 16, rgb888_to_rgb565_loop
      vst HX(16++,0), (r0+=r2) REP 16
      b lr  

;;; NAME
;;;    rgb888_to_rgbdelta
;;;
;;; SYNOPSIS
;;;    void rgb888_to_rgbdelta(unsigned char *dest, unsigned char *src, int dest_pitch, int src_pitch, int color_offset)
;;;
;;; FUNCTION
;;;    Convert 16x16 block from 24 to rgb delta.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_to_rgbdelta,"text"
.globl rgb888_to_rgbdelta
rgb888_to_rgbdelta:

      vld H(0++,0), (r1+=r3) REP 16
      vld H(0++,16), (r1+16+=r3) REP 16
      vld H(0++,32), (r1+32+=r3) REP 16
      vld H(0++,48), (r1+48+=r3) REP 16
      
      addcmpbeq r4, 0, 1, rgb888_to_rgbdelta_offset1 
      addcmpbeq r4, 0, 2, rgb888_to_rgbdelta_offset2 

      mov r3, 0
      mov r4, 0
      vmov V(16,0)+r3, V(0,2)+r4                ;R
      vadd VX(16,32), V(0,1)+r4, V(0,4)+r4   ;G
      vlsr V(16,1)+r3, VX(16,32), 1
rgb888_to_rgbdelta_loop_odd:  
      vmov V(16,2)+r3, V(0,3)+r4                ;B
      vadd VX(16,32), V(0,5)+r4, V(0,8)+r4   ;R
      vlsr V(16,3)+r3, VX(16,32), 1
      vmov V(16,4)+r3, V(0,7)+r4                ;G
      vadd VX(16,32), V(0,6)+r4, V(0,9)+r4   ;B
      vlsr V(16,5)+r3, VX(16,32), 1
      vmov V(16,6)+r3, V(0,11)+r4                  ;R
      vadd VX(16,32), V(0,10)+r4, V(0,13)+r4 ;G
      vlsr V(16,7)+r3, VX(16,32), 1
      add r4, 9
      addcmpble r3, 6, 24, rgb888_to_rgbdelta_loop_odd


      mov r3, 0
      mov r4, 0
      vmul VX(16,32), V(16,62)+r4, 64           ;G
      vmul VX(16,33), V(0,1)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,0)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,3)+r4, 64             ;B
      vmul VX(16,33), V(0,0)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,1)+r3, VX(16,32), 8
rgb888_to_rgbdelta_loop_even: 
      vmul VX(16,32), V(0,2)+r4, 64             ;R
      vmul VX(16,33), V(0,5)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,2)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,7)+r4, 64             ;G
      vmul VX(16,33), V(0,4)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,3)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,3)+r4, 64             ;B
      vmul VX(16,33), V(0,6)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,4)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,11)+r4, 64            ;R
      vmul VX(16,33), V(0,8)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,5)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,7)+r4, 64             ;G
      vmul VX(16,33), V(0,10)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,6)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,12)+r4, 64            ;B
      vmul VX(16,33), V(0,9)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,7)+r3, VX(16,32), 8
      add r4, 9
      addcmpble r3, 6, 24, rgb888_to_rgbdelta_loop_even
      b rgb888_to_rgbdelta_end

rgb888_to_rgbdelta_offset1:
      mov r3, 0
      mov r4, 0
      vmov V(16,0)+r3, V(0,0)+r4                ;B
      vadd VX(16,32), V(0,2)+r4, V(0,5)+r4   ;R
      vlsr V(16,1)+r3, VX(16,32), 1
rgb888_to_rgbdelta_loop_odd1: 
      vmov V(16,2)+r3, V(0,4)+r4                ;G
      vadd VX(16,32), V(0,3)+r4, V(0,6)+r4   ;B
      vlsr V(16,3)+r3, VX(16,32), 1
      vmov V(16,4)+r3, V(0,8)+r4                ;R
      vadd VX(16,32), V(0,7)+r4, V(0,10)+r4  ;G
      vlsr V(16,5)+r3, VX(16,32), 1
      vmov V(16,6)+r3, V(0,9)+r4                ;B
      vadd VX(16,32), V(0,11)+r4, V(0,14)+r4 ;R
      vlsr V(16,7)+r3, VX(16,32), 1
      add r4, 9
      addcmpble r3, 6, 24, rgb888_to_rgbdelta_loop_odd1


      vmul VX(16,32), V(16,63), 64              ;R
      vmul VX(16,33), V(0,2), 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,0), VX(16,32), 8
      vmul VX(16,32), V(0,4), 64                ;G
      vmul VX(16,33), V(0,1), 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,1), VX(16,32), 8
      mov r3, 0
      mov r4, 0
rgb888_to_rgbdelta_loop_even1:   
      vmul VX(16,32), V(0,6)+r4, 64             ;B
      vmul VX(16,33), V(0,3)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,2)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,8)+r4, 64             ;R
      vmul VX(16,33), V(0,5)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,3)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,4)+r4, 64             ;G
      vmul VX(16,33), V(0,7)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,4)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,9)+r4, 64            ;B
      vmul VX(16,33), V(0,6)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,5)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,8)+r4, 64             ;R
      vmul VX(16,33), V(0,11)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,6)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,13)+r4, 64            ;G
      vmul VX(16,33), V(0,10)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,7)+r3, VX(16,32), 8
      add r4, 9
      addcmpble r3, 6, 24, rgb888_to_rgbdelta_loop_even1
      b rgb888_to_rgbdelta_end

rgb888_to_rgbdelta_offset2:
      mov r3, 0
      mov r4, 0
      vmov V(16,0)+r3, V(0,1)+r4                ;G
      vadd VX(16,32), V(0,0)+r4, V(0,3)+r4   ;B
      vlsr V(16,1)+r3, VX(16,32), 1
rgb888_to_rgbdelta_loop_odd2: 
      vmov V(16,2)+r3, V(0,5)+r4                ;R
      vadd VX(16,32), V(0,4)+r4, V(0,7)+r4   ;G
      vlsr V(16,3)+r3, VX(16,32), 1
      vmov V(16,4)+r3, V(0,6)+r4                ;B
      vadd VX(16,32), V(0,8)+r4, V(0,11)+r4  ;R
      vlsr V(16,5)+r3, VX(16,32), 1
      vmov V(16,6)+r3, V(0,10)+r4                  ;G
      vadd VX(16,32), V(0,9)+r4, V(0,12)+r4  ;B
      vlsr V(16,7)+r3, VX(16,32), 1
      add r4, 9
      addcmpble r3, 6, 24, rgb888_to_rgbdelta_loop_odd2

      mov r3, 0
      mov r4, 0

      vmul VX(16,32), V(16,61)+r4, 64           ;B
      vmul VX(16,33), V(0,0)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,0)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,5)+r4, 64             ;R
      vmul VX(16,33), V(0,2)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,1)+r3, VX(16,32), 8
rgb888_to_rgbdelta_loop_even2:   
      vmul VX(16,32), V(0,1)+r4, 64             ;G
      vmul VX(16,33), V(0,4)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,2)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,6)+r4, 64             ;B
      vmul VX(16,33), V(0,3)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,3)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,5)+r4, 64             ;R
      vmul VX(16,33), V(0,8)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,4)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,10)+r4, 64           ;G
      vmul VX(16,33), V(0,7)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,5)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,6)+r4, 64             ;B
      vmul VX(16,33), V(0,9)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,6)+r3, VX(16,32), 8
      vmul VX(16,32), V(0,14)+r4, 64            ;R
      vmul VX(16,33), V(0,11)+r4, 192
      vadd VX(16,32), VX(16,32), VX(16,33)
      vlsr V(32,7)+r3, VX(16,32), 8
      add r4, 9
      addcmpble r3, 6, 24, rgb888_to_rgbdelta_loop_even2

rgb888_to_rgbdelta_end:

      vmov HX(17,0), HX(33,0) 
      vmov HX(19,0), HX(35,0) 
      vmov HX(21,0), HX(37,0) 
      vmov HX(23,0), HX(39,0) 
      vmov HX(25,0), HX(41,0) 
      vmov HX(27,0), HX(43,0) 
      vmov HX(29,0), HX(45,0) 
      vmov HX(31,0), HX(47,0) 

      vst H(16++,0), (r0+=r2) REP 16
      vst H(16++,16), (r0+16+=r2) REP 16

      vmov V(16,61), V(0,45)
      vmov V(16,62), V(0,46)
      vmov V(16,63), V(0,47)

      b lr  

.section .text$rgb888_to_rgbdelta_init,"text"
.globl rgb888_to_rgbdelta_init
rgb888_to_rgbdelta_init:
      vld H(0++,0), (r0+=r1) REP 16
      vmov V(16,61), V(0,0)
      vmov V(16,62), V(0,1)
      vmov V(16,63), V(0,2)

      mul r2, 3
      addcmpbls r1, 0, r2, rgb888_to_rgbdelta_init_end 
      add r0,r2
      vld H(0++,0), (r0+=r1) REP 16
      sub r3, r1, r2
      div.uu r3,r3,3
      sub r3, 3
      vmov V(0,3)+r3, V(0,0)+r3
      vmov V(0,4)+r3, V(0,1)+r3
      vmov V(0,5)+r3, V(0,2)+r3
      vst H(0++,0), (r0+=r1) REP 16
rgb888_to_rgbdelta_init_end:
      b lr  


;;; NAME
;;;    rgb888_fill
;;;
;;; SYNOPSIS
;;;    void rgb888_fill(int off1, void *arg, int ncols)
;;;
;;; FUNCTION
;;;    Do fill on loaded blocks. arg is the fill colour.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_fill,"text"
.globl rgb888_fill
rgb888_fill:
      mov r5, 8
      lsr r3, r1, r5       ; G (& with 255 unnecessary)
      lsr r4, r3, r5       ; R
      ;; r1 is B
      mul r2, 3
      add r5, r0, r2       ; limit value for r0
rgb888_fill_n_cols:
.ifdef RGB888
      vmov V(0,0)+r0*, r4     ; R
      vmov V(0,1)+r0*, r3     ; G
      vmov V(0,2)+r0*, r1     ; B
.else
      vmov V(0,2)+r0*, r4     ; B
      vmov V(0,1)+r0*, r3     ; G
      vmov V(0,0)+r0*, r1     ; R
.endif
      addcmpbne r0, 3, r5, rgb888_fill_n_cols
      b lr     

;;; NAME
;;;    rgb888_fade
;;;
;;; SYNOPSIS
;;;    void rgb888_fade(int off1, void *arg, int ncols)
;;;
;;; FUNCTION
;;;    Do fade on loaded blocks. arg is 8.8 alpha value.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_fade,"text"
.globl rgb888_fade
rgb888_fade:
      mul r2, 3
      add r5, r0, r2       ; limit value for r0
rgb888_fade_n_cols:
      vmulms V(0,0++)+r0*, V(0,0++)+r0*, r1 REP 2  ; R
      vmulms V(0,2)+r0*, V(0,2)+r0*, r1         ; B
      addcmpbne r0, 3, r5, rgb888_fade_n_cols
      b lr     

;;; NAME
;;;    rgb888_not
;;;
;;; SYNOPSIS
;;;    void rgb888_not(int off1, void *arg, int ncols)
;;;
;;; FUNCTION
;;;    Do logical not on loaded blocks. arg is meaningless.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_not,"text"
.globl rgb888_not
rgb888_not:
      mul r2, 3
      add r5, r0, r2       ; limit value for r0
rgb888_not_n_cols:
      veor V(0,0++)+r0*, V(0,0++)+r0*, 0xff REP 2  ; R and G
      veor V(0,2)+r0*, V(0,2)+r0*, 0xff         ; B
      addcmpbne r0, 3, r5, rgb888_not_n_cols
      b lr     

;;; NAME
;;;    rgb888_blt
;;;
;;; SYNOPSIS
;;;    void rgb888_blt(int off1, int off2, void *arg, int ncols)
;;;
;;; FUNCTION
;;;    Do blt on loaded blocks. arg meaningless.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_blt,"text"
.globl rgb888_blt
rgb888_blt:
      mul r3, 3
      add r2, r0, r3       ; limit value for r0
rgb888_blt_n_cols:
      vmov V(0,0++)+r0*, V(16,0++)+r1* REP 2 ; R and G
      vmov V(0,2)+r0*, V(16,2)+r1*        ; B
      add r1, 3
      addcmpbne r0, 3, r2, rgb888_blt_n_cols
      b lr     

;;; NAME
;;;    rgb888_transparent_blt
;;;
;;; SYNOPSIS
;;;    void rgb888_transparent_blt(int off1, int off2, void *arg, int ncols)
;;;
;;; FUNCTION
;;;    Do blt on loaded blocks. arg is the transparent colour.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_transparent_blt,"text"
.globl rgb888_transparent_blt
rgb888_transparent_blt:
      ld r4, (r2)          ; R
      ld r5, (r2+4)        ; G
      ld r2, (r2+8)        ; B
      mul r3, 3
      add r3, r0           ; limit value for r0
rgb888_transparent_blt_n_cols:
      ;; overwrite only when src pixels do not match RGB
      vsub -, V(16,0)+r1*, r4 SETF
      vsub -, V(16,1)+r1*, r5 IFZ SETF
      vsub -, V(16,2)+r1*, r2 IFZ SETF
      vmov V(0,0++)+r0*, V(16,0++)+r1* IFNZ REP 2  ; R and G
      vmov V(0,2)+r0*, V(16,2)+r1* IFNZ         ; B
      add r1, 3
      addcmpbne r0, 3, r3, rgb888_transparent_blt_n_cols
      b lr

;;; NAME
;;;    rgb888_overwrite_blt
;;;
;;; SYNOPSIS
;;;    void rgb888_overwrite_blt(int off1, int off2, void *arg, int ncols)
;;;
;;; FUNCTION
;;;    Do blt on loaded blocks. arg is the colour to be overwritten.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_overwrite_blt,"text"
.globl rgb888_overwrite_blt
rgb888_overwrite_blt:
      ld r4, (r2)          ; R
      ld r5, (r2+4)        ; G
      ld r2, (r2+8)        ; B
      mul r3, 3
      add r3, r0           ; limit value for r0
rgb888_overwrite_blt_n_cols:
      ;; overwrite only when dest pixels match RGB
      vsub -, V(0,0)+r0*, r4 SETF
      vsub -, V(0,1)+r0*, r5 IFZ SETF
      vsub -, V(0,2)+r0*, r2 IFZ SETF
      vmov V(0,0++)+r0*, V(16,0++)+r1* IFZ REP 2   ; R
      vmov V(0,2)+r0*, V(16,2)+r1* IFZ    ; B
      add r1, 3
      addcmpbne r0, 3, r3, rgb888_overwrite_blt_n_cols
      b lr

;;; NAME
;;;    rgb888_or
;;;
;;; SYNOPSIS
;;;    void rgb888_or(int off1, int off2, void *arg, int ncols)
;;;
;;; FUNCTION
;;;    Do or on loaded blocks. arg meaningless.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_or,"text"
.globl rgb888_or
rgb888_or:
      mul r3, 3
      add r2, r0, r3       ; limit value for r0
rgb888_or_n_cols:
      vor V(0,0++)+r0*, V(0,0++)+r0*, V(16,0++)+r1* REP 2   ; R and G
      vor V(0,2)+r0*, V(0,2)+r0*, V(16,2)+r1*            ; B
      add r1, 3
      addcmpbne r0, 3, r2, rgb888_or_n_cols
      b lr     

;;; NAME
;;;    rgb888_xor
;;;
;;; SYNOPSIS
;;;    void rgb888_xor(int off1, int off2, void *arg, int ncols)
;;;
;;; FUNCTION
;;;    Do xor on loaded blocks. arg meaningless.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_xor,"text"
.globl rgb888_xor
rgb888_xor:
      mul r3, 3
      add r2, r0, r3       ; limit value for r0
rgb888_xor_n_cols:
      veor V(0,0++)+r0*, V(0,0++)+r0*, V(16,0++)+r1* REP 2  ; R and G
      veor V(0,2)+r0*, V(0,2)+r0*, V(16,2)+r1*           ; B
      add r1, 3
      addcmpbne r0, 3, r2, rgb888_xor_n_cols
      b lr     

;;; NAME
;;;    rgb888_masked_fill
;;;
;;; SYNOPSIS
;;;    void rgb888_masked_fill(int off1, int off2, int off3, void *arg, int ncols)
;;;
;;; FUNCTION
;;;    Do masked fill on loaded blocks. arg is fill colour. off2 is meaningless.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_masked_fill,"text"
.globl rgb888_masked_fill
rgb888_masked_fill:
      mov r5, 8
      lsr r1, r3, r5       ; G (& with 255 unnecessary)
      lsr r5, r1, r5       ; R
      ;; r3 is B
      mul r4, 3
      add r4, r0           ; limit value for r0
rgb888_masked_fill_n_cols:
      vmov -, VX(32,0)+r2 SETF
      vmov V(0,0)+r0*, r5 IFNZ   ; R
      vmov V(0,1)+r0*, r1 IFNZ   ; G
      vmov V(0,2)+r0*, r3 IFNZ   ; B
      add r2, 1
      addcmpbne r0, 3, r4, rgb888_masked_fill_n_cols
      b lr     

;;; NAME
;;;    rgb888_masked_blt
;;;
;;; SYNOPSIS
;;;    void rgb888_masked_blt(int off1, int off2, int off3, void *arg, int ncols)
;;;
;;; FUNCTION
;;;    Do masked blt on loaded blocks. arg is meaningless.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_masked_blt,"text"
.globl rgb888_masked_blt
rgb888_masked_blt:
      mul r4, 3
      add r4, r0           ; limit value for r0
rgb888_masked_blt_n_cols:
      vmov -, VX(32,0)+r2 SETF
      vmov V(0,0++)+r0*, V(16,0++)+r1* IFNZ REP 2  ; R and G
      vmov V(0,2)+r0*, V(16,2)+r1* IFNZ         ; B
      add r1, 3
      add r2, 1
      addcmpbne r0, 3, r4, rgb888_masked_blt_n_cols
      b lr     

;;; NAME
;;;    rgb888_transpose
;;;
;;; SYNOPSIS
;;;    void rgb888_transpose(int vrf_addr)
;;;
;;; FUNCTION
;;;    Transpose 16x16 block in VRF at (0,16)+r0 onwards into (0,0).
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_transpose,"text"
.globl rgb888_transpose
rgb888_transpose:
      mov r1, 0
      mov r2, 64
      mov r3, 1024
rgb888_transpose_loop1: 
      vmov H(32,0)+r1, V(0,16)+r0   ; R
      vmov H(32,16)+r1, V(0,17)+r0  ; G
      vmov H(32,32)+r1, V(0,18)+r0  ; B
      add r0, 3
      addcmpbne r1, r2, r3, rgb888_transpose_loop1
      mov r0, 0
      mov r1, 0
rgb888_transpose_loop2: 
      vmov V(0,0)+r0, V(32,0)+r1 ; R
      vmov V(0,1)+r0, V(32,16)+r1   ; G
      vmov V(0,2)+r0, V(32,32)+r1   ; B
      add   r0, 3
      addcmpbne r1, 1, 16, rgb888_transpose_loop2     
      b lr     




   

.section .text$pal8_load_blocks,"text"
         
;;; NAME
;;;    rgb888_vflip_in_place
;;;
;;; SYNOPSIS
;;;    void rgb888_vflip_in_place(unsigned char *start_row, unsigned char *end_row, int pitch, int nbytes)
;;;
;;; FUNCTION
;;;    Do entire vertical flip.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_vflip_in_place,"text"
.globl rgb888_vflip_in_place
rgb888_vflip_in_place:
      stm r6-r7, lr, (--sp)
      mov r5, 48
      add r6, r2, r3
      sub r7, r2, r3
      mov r2, 16
rgb888_vflip_in_place_vloop:  
      add r4, r0, r3    
rgb888_vflip_in_place_hloop:  
      vld H(0++,0), (r0+=r2) REP 2
      vld H(2,0), (r0+32)
      vld H(0++,16), (r1+=r2) REP 2
      vld H(2,16), (r1+32)
      vst H(0++,0), (r1+=r2) REP 2
      vst H(2,0), (r1+32)
      vst H(0++,16), (r0+=r2) REP 2
      vst H(2,16), (r0+32)
      add r1, r5
      addcmpbne r0, r5, r4, rgb888_vflip_in_place_hloop     
      sub r1, r6
      addcmpblt r0, r7, r1, rgb888_vflip_in_place_vloop
      ldm r6-r7, pc, (sp++)

;;; NAME
;;;    rgb888_hflip_load_right_initial
;;;
;;; SYNOPSIS
;;;    void rgb888_hflip_load_right_initial(unsigned char *addr, int pitch)
;;;
;;; FUNCTION
;;;    Load initial right hand block.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_hflip_load_right_initial,"text"
.globl rgb888_hflip_load_right_initial
rgb888_hflip_load_right_initial:
      vld H(16++,48)*, (r0+48+=r1) REP 16
      b lr

;;; NAME
;;;    rgb888_hflip_load_left_blocks
;;;
;;; SYNOPSIS
;;;    void rgb888_hflip_load_left_blocks(unsigned char *addr, int pitch)
;;;
;;; FUNCTION
;;;    Load left hand block.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_hflip_load_left_blocks,"text"
.globl rgb888_hflip_load_left_blocks
rgb888_hflip_load_left_blocks:
      vld H(0++,0), (r0+=r1) REP 16
      vld H(0++,16), (r0+16+=r1) REP 16
      vld H(0++,32), (r0+32+=r1) REP 16
      b lr

;;; NAME
;;;    rgb888_hflip_load_right_blocks
;;;
;;; SYNOPSIS
;;;    void rgb888_hflip_load_right_blocks(unsigned char *addr, int pitch)
;;;
;;; FUNCTION
;;;    Load right hand blocks.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_hflip_load_right_blocks,"text"
.globl rgb888_hflip_load_right_blocks
rgb888_hflip_load_right_blocks:
      vld H(16++,0)*, (r0+=r1) REP 16
      vld H(16++,16)*, (r0+16+=r1) REP 16
      vld H(16++,32)*, (r0+32+=r1) REP 16
      b lr

;;; NAME
;;;    rgb888_hflip
;;;
;;; SYNOPSIS
;;;    void rgb888_hflip(int off)
;;;
;;; FUNCTION
;;;    Hflip loaded blocks.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_hflip,"text"
.globl rgb888_hflip
rgb888_hflip:
      mov r1, 45
rgb888_hflip_loop:
      ;; Hflip from (16,0)*+off into (32,0)
      vmov V(32,0++)+r1, V(16,0++)+r0* REP 2
      vmov V(32,2)+r1, V(16,2)+r0*
      ;; and from (0,0) into (16,0)*+off
      vmov V(16,0++)+r0*, V(0,0++)+r1 REP 2
      vmov V(16,2)+r0*, V(0,2)+r1
      add r0, 3
      addcmpbge r1, -3, 0, rgb888_hflip_loop
      b lr

;;; NAME
;;;    rgb888_hflip_save_right_blocks
;;;
;;; SYNOPSIS
;;;    void rgb888_hflip_save_right_blocks(unsigned char *addr, int pitch)
;;;
;;; FUNCTION
;;;    Save right blocks.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_hflip_save_right_blocks,"text"
.globl rgb888_hflip_save_right_blocks
rgb888_hflip_save_right_blocks:
      mov r2, r0
      bmask r2, 4
      addcmpbne r2, 0, 0, rgb888_hflip_save_right_blocks_unaligned
      ;; Aligned case.
      vst H(16++,0)*, (r0+=r1) REP 16
      vst H(16++,16)*, (r0+16+=r1) REP 16
      vst H(16++,32)*, (r0+32+=r1) REP 16
      b lr
rgb888_hflip_save_right_blocks_unaligned:
      vst H(16++,16)*, (r0+16+=r1) REP 16
      vst H(16++,32)*, (r0+32+=r1) REP 16
      vst H(16++,48)*, (r0+48+=r1) REP 16
      b lr
      
;;; NAME
;;;    rgb888_hflip_save_left_blocks
;;;
;;; SYNOPSIS
;;;    void rgb888_hflip_save_left_blocks(unsigned char *addr, int pitch)
;;;
;;; FUNCTION
;;;    Save left blocks.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_hflip_save_left_blocks,"text"
.globl rgb888_hflip_save_left_blocks
rgb888_hflip_save_left_blocks:
      vst H(32++,0), (r0+=r1) REP 16
      vst H(32++,16), (r0+16+=r1) REP 16
      vst H(32++,32), (r0+32+=r1) REP 16
      b lr


;;; NAME
;;;    rgb888_hflip_save_right_final
;;;
;;; SYNOPSIS
;;;    void rgb888_hflip_save_right_final(unsigned char *addr, int pitch)
;;;
;;; FUNCTION
;;;    Save final portion of right block.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_hflip_save_right_final,"text"
.globl rgb888_hflip_save_right_final
rgb888_hflip_save_right_final:
      ;; add 48 to x coord because a cbadd1 has happended
      ;; add 48 to r0 because 48 has been subtracted from the right addr
      vst H(16++,48)*, (r0+48+=r1) REP 16
      b lr

;;; NAME
;;;    rgb888_hflip_final
;;;
;;; SYNOPSIS
;;;    void rgb888_hflip_final(int ncols)
;;;
;;; FUNCTION
;;;    Hflip final fraction portion of block.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_hflip_final,"text"
.globl rgb888_hflip_final
rgb888_hflip_final:
      mul r0, 3
      mov r1, 0
      sub r0, 3
rgb888_hflip_final_loop1:
      vmov V(32,0++)+r1, V(0,0++)+r0 REP 2
      vmov V(32,2)+r1, V(0,2)+r0
      add r1, 3
      addcmpbge r0, -3, 0, rgb888_hflip_final_loop1
rgb888_hflip_final_loop2:
      vmov V(32,0++)+r1, V(0,0++)+r1 REP 2
      vmov V(32,2)+r1, V(0,2)+r1
      addcmpbne r1, 3, 48, rgb888_hflip_final_loop2
      b lr
.stabs "rgb888_hflip_final:F1",36,0,0,rgb888_hflip_final

;;; NAME
;;;    rgb888_from yuv
;;;
;;; SYNOPSIS
;;;    void rgb888_from_yuv(unsigned char *Y, unsigned char *U, unsigned char *V,
;;;                         int in_pitch, unsigned char *rgb, int out_pitch)
;;;
;;; FUNCTION
;;;    Load 16x16 block of YUV data, convert to RGB888 and write out.
;;;
;;; RETURNS
;;;    -

   
.section .text$rgb888_from_yuv,"text"
.globl rgb888_from_yuv
rgb888_from_yuv:
   vld H(0++,0), (r0+=r3) REP 16
   asr r3, 1
   vldleft H(16++,0), (r1+=r3) REP 8
   vldleft H(24++,0), (r2+=r3) REP 8
   b rgb888_yuv_common


.section .text$rgb888_from_yuv_half,"text"
.globl rgb888_from_yuv_half
rgb888_from_yuv_half:
   shl r3, 1
   vld H(0++,0), (r0+=r3) REP 16
   asr r3, 1
   vldleft H(16++,0), (r1+=r3) REP 8
   vldleft H(24++,0), (r2+=r3) REP 8
   b rgb888_yuv_common

;;; NAME
;;;    rgb888_from yuv_downsampled
;;;
;;; SYNOPSIS
;;;    void rgb888_from_yuv_downsampled(unsigned char *Y, unsigned char *U, unsigned char *V,
;;;                                     int in_pitch, unsigned char *rgb, int out_pitch)
;;;
;;; FUNCTION
;;;    Load 16x16 block of YUV data, convert to RGB888 and write out.
;;;
;;; RETURNS
;;;    -

.section .text$rgb888_from_yuv_downsampled,"text"
.globl rgb888_from_yuv_downsampled
rgb888_from_yuv_downsampled:
      stm r6-r7, lr, (--sp)
      ;; we will have to keep r4 and r5
      mov r6, r4
      mov r7, r5
      ;; Now load and downsample. The args r0-r3 are correct for this call.
      bl vc_image_load_yuv_downsampled
      ;; Restore r4 and r5, and carry on as above.
      mov r4, r6
      mov r5, r7
      bl rgb888_yuv_common
      ldm r6-r7, pc, (sp++)
   
   
rgb888_yuv_common:
   ;; Code common to the various flavours of YUV->RGB transformation.
   ;; Intended for internal use only; passes arguments and results
   ;; in the VRF.
   ;;
   ;; Inputs: 16*16 of Y in H(0++,0)
   ;;           8*8 of U in H(16++,0)
   ;;           8*8 of V in H(24++,0)
   ;;
   ;; Outputs: 16*16 of RGB565 in HX(0++,32)  [green bits set only]
   ;;          16*16 of RGB565 in HX(16++,32) [red and blue bits only]
   ;;
   ;; Uses the entire top half of the VRF as temporary storage.
   ;; Uses registers r0,r1,r2. Preserves registers r3 and above.
   
   ; Scale up the U and V components in the X direction only
   ; and subtract 128, placing them from HX(16,32) onwards.
   vsub VX(16,32++), V(16,0), 128 REP 2
   vsub VX(16,34++), V(16,1), 128 REP 2
   vsub VX(16,36++), V(16,2), 128 REP 2
   vsub VX(16,38++), V(16,3), 128 REP 2
   vsub VX(16,40++), V(16,4), 128 REP 2
   vsub VX(16,42++), V(16,5), 128 REP 2
   vsub VX(16,44++), V(16,6), 128 REP 2
   vsub VX(16,46++), V(16,7), 128 REP 2

   ; Apply scaling to Y and store 16-bit at HX(0..15,32).
   vsub HX(16++,0), H(0++,0), 16 REP 16
   vmulm HX(0++,32), HX(16++,0), 298 REP 16  ; (Y-16) * 1.164

   ; We are now ready to complete the conversion YCbCr -> RGB
   ; for CCIR601 inputs (limited excursion: includes scaling up)
   ; R = 1.164*(Y-16)                 + 1.596*(V-128) 
   ; G = 1.164*(Y-16) - 0.391*(U-128) - 0.781*(V-128)
   ; B = 1.164*(Y-16) + 2.018*(U-128)

   ; (U,V)->G contributions
   vmulm HX(0++,0), HX(16++,32), -100 REP 8
   vmulm HX(8++,0), HX(24++,32), -200 REP 8
   vadds HX(0++,0), HX(0++,0), HX(8++,0) REP 8

   ; U->B and V->R contributions
   vmulm HX(16++,0), HX(16++,32), 408 REP 8
   vmulm HX(24++,0), HX(24++,32), 516 REP 8

   mov r0, 896     ; 64 * 14
   mov r1, 448     ; 64 * 7
   mov r2, -64
rgb888_from_yuv_loop1:
   vadds H(16++,48)+r0, HX(0++,32)+r0, HX(24,0)+r1 REP 2   ; red
   vadds H(16++,32)+r0, HX(0++,32)+r0, HX(16,0)+r1 REP 2   ; blue
   vadds H(0++,32)+r0, HX(0++,32)+r0, HX(0,0)+r1 REP 2     ; green
   addscale r0, r2 << 1
   addcmpbge r1, r2, 0, rgb888_from_yuv_loop1
 
   mov r0, 0
   mov r1, 0
rgb888_from_yuv_loop2:
.ifdef RGB888
   vmov V(48,0)+r0, V(16,48)+r1  ; red
   vmov V(48,1)+r0, V(0,32)+r1   ; green
   vmov V(48,2)+r0, V(16,32)+r1  ; blue
.else
   vmov V(48,2)+r0, V(16,48)+r1  ; red
   vmov V(48,1)+r0, V(0,32)+r1   ; green
   vmov V(48,0)+r0, V(16,32)+r1  ; blue
.endif
   add r0, 3
   addcmpbne r1, 1, 16, rgb888_from_yuv_loop2

   ;; Now write out and we're done.
   vst H(48++,0), (r4+=r5) REP 16
   vst H(48++,16), (r4+16+=r5) REP 16
   vst H(48++,32), (r4+32+=r5) REP 16
   b lr
.stabs "rgb888_from_yuv:F1",36,0,0,rgb888_from_yuv


;extern void vc_3d32_to_rgb888(unsigned char *lcd_buffer  r0
;             ,unsigned char *screen     r1
;             ,int num          r2
;             ,short *cmap)                r3
;
; Converts num pixels from 32bpp 3d format to a 24bpp direct mapped
; colour image (based on a 16 bit palette)
.section .text$vc_3d32_to_rgb888,"text"
.globl vc_3d32_to_rgb888
vc_3d32_to_rgb888:
       stm r6-r7,lr,(--sp)
       mov      r5,0
       asr      r2,8 ; blocks of 256 pixels
       mov r6,64
       mov r7,1024
copyloop:

       mov  r4,0  ; counter within a block
blockloop:  
       vld      HX(10,0),(r1)
       add      r1,64       
       vmov     -,H(10,0) CLRA ACC      
       vlookupm HX(0,0),(r3)
; Now split into colour components and apply light level     
       vlsr  HX(3,0),HX(0,0),5    ; 6 bits of green
       vshl  HX(3,0),HX(3,0),2 
       vlsr  HX(2,0),HX(0,0),11   ; 5 bits of red
       vshl  HX(2,0),HX(2,0),3   
       vshl  HX(4,0),HX(0,0),3    ; blue

       vmulm H(16,0)+r4,H(2,0),H(10,16) ; red
       vmulm H(16,16)+r4,H(3,0),H(10,16) ; green
       vmulm H(16,32)+r4,H(4,0),H(10,16) ; blue 
      
       addcmpblt  r4,r6,r7,blockloop
   

.ifdef RGB888
       vmov V(0,0),V(16,0) ; 
       vmov V(0,1),V(16,16)
         vmov V(0,2),V(16,32)
       vmov V(0,3),V(16,1) ; 
       vmov V(0,4),V(16,17)
         vmov V(0,5),V(16,33)
       vmov V(0,6),V(16,2) ; 
       vmov V(0,7),V(16,18)
         vmov V(0,8),V(16,34)
       vmov V(0,9),V(16,3) ; 
       vmov V(0,10),V(16,19)
         vmov V(0,11),V(16,35)
       vmov V(0,12),V(16,4)   ; 
       vmov V(0,13),V(16,20)
         vmov V(0,14),V(16,36)
       vmov V(0,15),V(16,5)   ; 
       vmov V(0,16),V(16,21)
         vmov V(0,17),V(16,37)
       vmov V(0,18),V(16,6)   ; 
       vmov V(0,19),V(16,22)
         vmov V(0,20),V(16,38)
       vmov V(0,21),V(16,7)   ; 
       vmov V(0,22),V(16,23)
         vmov V(0,23),V(16,39)   
       vmov V(0,24),V(16,8)   ; 
       vmov V(0,25),V(16,24)
         vmov V(0,26),V(16,40)
       vmov V(0,27),V(16,9)   ; 
       vmov V(0,28),V(16,25)
         vmov V(0,29),V(16,41)
       vmov V(0,30),V(16,10)  ; 
       vmov V(0,31),V(16,26)
         vmov V(0,32),V(16,42)
       vmov V(0,33),V(16,11)  ; 
       vmov V(0,34),V(16,27)
         vmov V(0,35),V(16,43)
       vmov V(0,36),V(16,12)  ; 
       vmov V(0,37),V(16,28)
         vmov V(0,38),V(16,44)
       vmov V(0,39),V(16,13)  ; 
       vmov V(0,40),V(16,29)
         vmov V(0,41),V(16,45)
       vmov V(0,42),V(16,14)  ; 
       vmov V(0,43),V(16,30)
         vmov V(0,44),V(16,46)
       vmov V(0,45),V(16,15)  ; 
       vmov V(0,46),V(16,31)
         vmov V(0,47),V(16,47)   
.else
       vmov V(0,2),V(16,0) ; 
       vmov V(0,1),V(16,16)
         vmov V(0,0),V(16,32)
       vmov V(0,5),V(16,1) ; 
       vmov V(0,4),V(16,17)
         vmov V(0,3),V(16,33)
       vmov V(0,8),V(16,2) ; 
       vmov V(0,7),V(16,18)
         vmov V(0,6),V(16,34)
       vmov V(0,11),V(16,3)   ; 
       vmov V(0,10),V(16,19)
         vmov V(0,9),V(16,35)
       vmov V(0,14),V(16,4)   ; 
       vmov V(0,13),V(16,20)
         vmov V(0,12),V(16,36)
       vmov V(0,17),V(16,5)   ; 
       vmov V(0,16),V(16,21)
         vmov V(0,15),V(16,37)
       vmov V(0,20),V(16,6)   ; 
       vmov V(0,19),V(16,22)
         vmov V(0,18),V(16,38)
       vmov V(0,23),V(16,7)   ; 
       vmov V(0,22),V(16,23)
         vmov V(0,21),V(16,39)   
       vmov V(0,26),V(16,8)   ; 
       vmov V(0,25),V(16,24)
         vmov V(0,24),V(16,40)
       vmov V(0,29),V(16,9)   ; 
       vmov V(0,28),V(16,25)
         vmov V(0,27),V(16,41)
       vmov V(0,32),V(16,10)  ; 
       vmov V(0,31),V(16,26)
         vmov V(0,30),V(16,42)
       vmov V(0,35),V(16,11)  ; 
       vmov V(0,34),V(16,27)
         vmov V(0,33),V(16,43)
       vmov V(0,38),V(16,12)  ; 
       vmov V(0,37),V(16,28)
         vmov V(0,36),V(16,44)
       vmov V(0,41),V(16,13)  ; 
       vmov V(0,40),V(16,29)
         vmov V(0,39),V(16,45)
       vmov V(0,44),V(16,14)  ; 
       vmov V(0,43),V(16,30)
         vmov V(0,42),V(16,46)
       vmov V(0,47),V(16,15)  ; 
       vmov V(0,46),V(16,31)
         vmov V(0,45),V(16,47)   
.endif       

       mov r4,48
       vst H(0++,0),(r0+=r4) REP 16
       vst H(0++,16),(r0+16+=r4) REP 16
       vst H(0++,32),(r0+32+=r4) REP 16
    
       add      r0,768
       addcmpblt r5,1,r2,copyloop
       ldm r6-r7,pc,(sp++)

.stabs "Colourmap lookup:F19",36,0,1,vc_3d32_to_rgb888

.FORGET


;extern void vc_3d32_to_rgb888_anti2(short *lcd_buffer             r0
;                      ,unsigned char *screen                r1
;                      ,int num                                 r2
;                      ,short *cmap)                   r3
; Combines the values from two pixels when computing the result
.section .text$vc_3d32_to_rgb888_anti2,"text"
.globl vc_3d32_to_rgb888_anti2
vc_3d32_to_rgb888_anti2:
       stm r6-r8,lr,(--sp)
       mov      r5,0

       mov     r8,r2 ; copy of r2 to be used for the last store pass
       bmask   r8,8 ; number of stores in last stage
       asr      r2,8 ; blocks of 256 pixels
       mov r6,64
       mov r7,1024 ; load 16*16*2 blocks of data
copyloop:
       mov r4,0   ; counter within a block

; First read values into H(16,0):H(16,16):H(16,32)
blockloop:  
       vld      HX(10,0),(r1)
       add      r1,64       
       vmov     -,H(10,0) CLRA ACC      
       vlookupm HX(0,0),(r3)
; Now split into colour components and apply light level     
       vlsr  HX(3,0),HX(0,0),5    ; 6 bits of green
       vshl  HX(3,0),HX(3,0),2 
       vlsr  HX(2,0),HX(0,0),11   ; 5 bits of red
       vshl  HX(2,0),HX(2,0),3   
       vshl  HX(4,0),HX(0,0),3    ; blue
       
       vlsr H(10,16),H(10,16),1  ; divide by 2 to allow for later combination

       vmulm H(16,0)+r4,H(2,0),H(10,16) ; red
       vmulm H(16,16)+r4,H(3,0),H(10,16) ; green
       vmulm H(16,32)+r4,H(4,0),H(10,16) ; blue 
       
       vld      HX(10,0),(r1)
       add      r1,64       
       vmov     -,H(10,0) CLRA ACC      
       vlookupm HX(0,0),(r3)
; Now split into colour components and apply light level     
       vlsr  HX(3,0),HX(0,0),5    ; 6 bits of green
       vshl  HX(3,0),HX(3,0),2 
       vlsr  HX(2,0),HX(0,0),11   ; 5 bits of red
       vshl  HX(2,0),HX(2,0),3   
       vshl  HX(4,0),HX(0,0),3    ; blue
       
       vlsr H(10,16),H(10,16),1  ; divide by 2 to allow for later combination

       vmulm H(32,0)+r4,H(2,0),H(10,16) ; red
       vmulm H(32,16)+r4,H(3,0),H(10,16) ; green
       vmulm H(32,32)+r4,H(4,0),H(10,16) ; blue 
      
       addcmpblt  r4,r6,r7,blockloop
      
; First compute all red values
.ifdef RGB888
       vadd V(0,0 ),V(16,0 ),V(16,1 )
       vadd V(0,3 ),V(16,2 ),V(16,3 )
       vadd V(0,6 ),V(16,4 ),V(16,5 )
       vadd V(0,9 ),V(16,6 ),V(16,7 )
       vadd V(0,12),V(16,8 ),V(16,9 )
       vadd V(0,15),V(16,10),V(16,11)
       vadd V(0,18),V(16,12),V(16,13)
       vadd V(0,21),V(16,14),V(16,15)
       vadd V(0,24),V(32,0 ),V(32,1 )
       vadd V(0,27),V(32,2 ),V(32,3 )
       vadd V(0,30),V(32,4 ),V(32,5 )
       vadd V(0,33),V(32,6 ),V(32,7 )
       vadd V(0,36),V(32,8 ),V(32,9 )
       vadd V(0,39),V(32,10),V(32,11)
       vadd V(0,42),V(32,12),V(32,13)
       vadd V(0,45),V(32,14),V(32,15)
.else
       vadd V(0,2 ),V(16,0 ),V(16,1 )
       vadd V(0,5 ),V(16,2 ),V(16,3 )
       vadd V(0,8 ),V(16,4 ),V(16,5 )
       vadd V(0,11 ),V(16,6 ),V(16,7 )
       vadd V(0,14),V(16,8 ),V(16,9 )
       vadd V(0,17),V(16,10),V(16,11)
       vadd V(0,20),V(16,12),V(16,13)
       vadd V(0,23),V(16,14),V(16,15)
       vadd V(0,26),V(32,0 ),V(32,1 )
       vadd V(0,29),V(32,2 ),V(32,3 )
       vadd V(0,32),V(32,4 ),V(32,5 )
       vadd V(0,35),V(32,6 ),V(32,7 )
       vadd V(0,38),V(32,8 ),V(32,9 )
       vadd V(0,41),V(32,10),V(32,11)
       vadd V(0,44),V(32,12),V(32,13)
       vadd V(0,47),V(32,14),V(32,15)
.endif
; Now for green
       vadd V(0,1 ),V(16,16),V(16,17)
       vadd V(0,4 ),V(16,18),V(16,19)
       vadd V(0,7 ),V(16,20),V(16,21)
       vadd V(0,10),V(16,22),V(16,23)
       vadd V(0,13),V(16,24),V(16,25)
       vadd V(0,16),V(16,26),V(16,27)
       vadd V(0,19),V(16,28),V(16,29)
       vadd V(0,22),V(16,30),V(16,31)
       vadd V(0,25),V(32,16),V(32,17)
       vadd V(0,28),V(32,18),V(32,19)
       vadd V(0,31),V(32,20),V(32,21)
       vadd V(0,34),V(32,22),V(32,23)
       vadd V(0,37),V(32,24),V(32,25)
       vadd V(0,40),V(32,26),V(32,27)
       vadd V(0,43),V(32,28),V(32,29)
       vadd V(0,46),V(32,30),V(32,31)
; Now for blue     
.ifdef RGB888
       vadd V(0,2 ),V(16,32),V(16,33)
       vadd V(0,5 ),V(16,34),V(16,35)
       vadd V(0,8 ),V(16,36),V(16,37)
       vadd V(0,11),V(16,38),V(16,39)
       vadd V(0,14),V(16,40),V(16,41)
       vadd V(0,17),V(16,42),V(16,43)
       vadd V(0,20),V(16,44),V(16,45)
       vadd V(0,23),V(16,46),V(16,47)
       vadd V(0,26),V(32,32),V(32,32)
       vadd V(0,29),V(32,34),V(32,34)
       vadd V(0,32),V(32,36),V(32,36)
       vadd V(0,35),V(32,38),V(32,38)
       vadd V(0,38),V(32,40),V(32,40)
       vadd V(0,41),V(32,42),V(32,42)
       vadd V(0,44),V(32,44),V(32,44)
       vadd V(0,47),V(32,46),V(32,46)
.else
       vadd V(0,0 ),V(16,32),V(16,33)
       vadd V(0,3 ),V(16,34),V(16,35)
       vadd V(0,6 ),V(16,36),V(16,37)
       vadd V(0,9),V(16,38),V(16,39)
       vadd V(0,12),V(16,40),V(16,41)
       vadd V(0,15),V(16,42),V(16,43)
       vadd V(0,18),V(16,44),V(16,45)
       vadd V(0,21),V(16,46),V(16,47)
       vadd V(0,24),V(32,32),V(32,32)
       vadd V(0,27),V(32,34),V(32,34)
       vadd V(0,30),V(32,36),V(32,36)
       vadd V(0,33),V(32,38),V(32,38)
       vadd V(0,36),V(32,40),V(32,40)
       vadd V(0,39),V(32,42),V(32,42)
       vadd V(0,42),V(32,44),V(32,44)
       vadd V(0,45),V(32,46),V(32,46)
.endif
       mov r4,48
       vst H(0++,0),(r0+=r4) REP 16
       vst H(0++,16),(r0+16+=r4) REP 16
       vst H(0++,32),(r0+32+=r4) REP 16
    
       add      r0,768
       addcmpblt r5,1,r2,copyloop

; xxx dc4 hack
       cmp r8,16
       blt skip_out
       mov r4,0   ; counter within a block

; First read values into H(16,0):H(16,16):H(16,32)
blockloop2: 
       vld      HX(10,0),(r1)
       add      r1,64       
       vmov     -,H(10,0) CLRA ACC      
       vlookupm HX(0,0),(r3)
; Now split into colour components and apply light level     
       vlsr  HX(3,0),HX(0,0),5    ; 6 bits of green
       vshl  HX(3,0),HX(3,0),2 
       vlsr  HX(2,0),HX(0,0),11   ; 5 bits of red
       vshl  HX(2,0),HX(2,0),3   
       vshl  HX(4,0),HX(0,0),3    ; blue
       
       vlsr H(10,16),H(10,16),1  ; divide by 2 to allow for later combination

       vmulm H(16,0)+r4,H(2,0),H(10,16) ; red
       vmulm H(16,16)+r4,H(3,0),H(10,16) ; green
       vmulm H(16,32)+r4,H(4,0),H(10,16) ; blue 
       
       vld      HX(10,0),(r1)
       add      r1,64       
       vmov     -,H(10,0) CLRA ACC      
       vlookupm HX(0,0),(r3)
; Now split into colour components and apply light level     
       vlsr  HX(3,0),HX(0,0),5    ; 6 bits of green
       vshl  HX(3,0),HX(3,0),2 
       vlsr  HX(2,0),HX(0,0),11   ; 5 bits of red
       vshl  HX(2,0),HX(2,0),3   
       vshl  HX(4,0),HX(0,0),3    ; blue
       
       vlsr H(10,16),H(10,16),1  ; divide by 2 to allow for later combination

       vmulm H(32,0)+r4,H(2,0),H(10,16) ; red
       vmulm H(32,16)+r4,H(3,0),H(10,16) ; green
       vmulm H(32,32)+r4,H(4,0),H(10,16) ; blue 
      
       addcmpblt  r4,r6,r7,blockloop2
      
; First compute all red values
.ifdef RGB888
       vadd V(0,0 ),V(16,0 ),V(16,1 )
       vadd V(0,3 ),V(16,2 ),V(16,3 )
       vadd V(0,6 ),V(16,4 ),V(16,5 )
       vadd V(0,9 ),V(16,6 ),V(16,7 )
       vadd V(0,12),V(16,8 ),V(16,9 )
       vadd V(0,15),V(16,10),V(16,11)
       vadd V(0,18),V(16,12),V(16,13)
       vadd V(0,21),V(16,14),V(16,15)
       vadd V(0,24),V(32,0 ),V(32,1 )
       vadd V(0,27),V(32,2 ),V(32,3 )
       vadd V(0,30),V(32,4 ),V(32,5 )
       vadd V(0,33),V(32,6 ),V(32,7 )
       vadd V(0,36),V(32,8 ),V(32,9 )
       vadd V(0,39),V(32,10),V(32,11)
       vadd V(0,42),V(32,12),V(32,13)
       vadd V(0,45),V(32,14),V(32,15)
.else
       vadd V(0,2 ),V(16,0 ),V(16,1 )
       vadd V(0,5 ),V(16,2 ),V(16,3 )
       vadd V(0,8 ),V(16,4 ),V(16,5 )
       vadd V(0,11 ),V(16,6 ),V(16,7 )
       vadd V(0,14),V(16,8 ),V(16,9 )
       vadd V(0,17),V(16,10),V(16,11)
       vadd V(0,20),V(16,12),V(16,13)
       vadd V(0,23),V(16,14),V(16,15)
       vadd V(0,26),V(32,0 ),V(32,1 )
       vadd V(0,29),V(32,2 ),V(32,3 )
       vadd V(0,32),V(32,4 ),V(32,5 )
       vadd V(0,35),V(32,6 ),V(32,7 )
       vadd V(0,38),V(32,8 ),V(32,9 )
       vadd V(0,41),V(32,10),V(32,11)
       vadd V(0,44),V(32,12),V(32,13)
       vadd V(0,47),V(32,14),V(32,15)
.endif
; Now for green
       vadd V(0,1 ),V(16,16),V(16,17)
       vadd V(0,4 ),V(16,18),V(16,19)
       vadd V(0,7 ),V(16,20),V(16,21)
       vadd V(0,10),V(16,22),V(16,23)
       vadd V(0,13),V(16,24),V(16,25)
       vadd V(0,16),V(16,26),V(16,27)
       vadd V(0,19),V(16,28),V(16,29)
       vadd V(0,22),V(16,30),V(16,31)
       vadd V(0,25),V(32,16),V(32,17)
       vadd V(0,28),V(32,18),V(32,19)
       vadd V(0,31),V(32,20),V(32,21)
       vadd V(0,34),V(32,22),V(32,23)
       vadd V(0,37),V(32,24),V(32,25)
       vadd V(0,40),V(32,26),V(32,27)
       vadd V(0,43),V(32,28),V(32,29)
       vadd V(0,46),V(32,30),V(32,31)
; Now for blue     
.ifdef RGB888
       vadd V(0,2 ),V(16,32),V(16,33)
       vadd V(0,5 ),V(16,34),V(16,35)
       vadd V(0,8 ),V(16,36),V(16,37)
       vadd V(0,11),V(16,38),V(16,39)
       vadd V(0,14),V(16,40),V(16,41)
       vadd V(0,17),V(16,42),V(16,43)
       vadd V(0,20),V(16,44),V(16,45)
       vadd V(0,23),V(16,46),V(16,47)
       vadd V(0,26),V(32,32),V(32,32)
       vadd V(0,29),V(32,34),V(32,34)
       vadd V(0,32),V(32,36),V(32,36)
       vadd V(0,35),V(32,38),V(32,38)
       vadd V(0,38),V(32,40),V(32,40)
       vadd V(0,41),V(32,42),V(32,42)
       vadd V(0,44),V(32,44),V(32,44)
       vadd V(0,47),V(32,46),V(32,46)
.else
       vadd V(0,0 ),V(16,32),V(16,33)
       vadd V(0,3 ),V(16,34),V(16,35)
       vadd V(0,6 ),V(16,36),V(16,37)
       vadd V(0,9),V(16,38),V(16,39)
       vadd V(0,12),V(16,40),V(16,41)
       vadd V(0,15),V(16,42),V(16,43)
       vadd V(0,18),V(16,44),V(16,45)
       vadd V(0,21),V(16,46),V(16,47)
       vadd V(0,24),V(32,32),V(32,32)
       vadd V(0,27),V(32,34),V(32,34)
       vadd V(0,30),V(32,36),V(32,36)
       vadd V(0,33),V(32,38),V(32,38)
       vadd V(0,36),V(32,40),V(32,40)
       vadd V(0,39),V(32,42),V(32,42)
       vadd V(0,42),V(32,44),V(32,44)
       vadd V(0,45),V(32,46),V(32,46)
.endif
; xxx dc4 hack

; now need to last r8 pixels
      asr r8, 4 ; write in sixteens
      mov r3, 0
writelastblock:
       vst H(0,0)+r3,(r0)
       vst H(0,16)+r3,(r0+16)
       vst H(0,32)+r3,(r0+32)
       add r0,48
       add r3, 64
       addcmpbgt r8, -1, 0, writelastblock

skip_out:
       ldm r6-r8,pc,(sp++)

.stabs "vc_3d32_to_rgb888_anti2:F19",36,0,1,vc_3d32_to_rgb888_anti2
.FORGET

;extern void vc_palette8torgb888(unsigned char *lcd_buffer     r0
;            ,unsigned char *screen r1
;            ,int dest_pitch        r2
;            ,int width             r3
;            ,short *cmap           r4
;            ,int src_pitch)        r5
; 
; Uses 16 bit lookup and converts back to 24 bit colour
; Applies conversion to a 16 deep strip
           
.section .text$vc_palett8_to_rgb888,"text"
.globl vc_palette8_to_rgb888
vc_palette8_to_rgb888:
   stm   r6-r8,lr,(--sp)
   mov   r8,r3 ; !!!!SPL was hard coded to 176, caused PEND to break!!!!
   asr   r8,4  ; Numbers of 16 blocks across to process
   mov   r6,r2
rowloop:
   vldleft   H(0++,0),(r1+=r5) REP 16
;   vldright   H(0++,0),(r1+=r5) REP 16
   mov   r3,0
   mov   r7,16
downloop:   
   vmov  HX(0,0)+r3,H(0,0)+r3 CLRA ACC
   vlookupm HX(16,0)+r3,(r4)
   add   r3,64
   addcmpbgt r7,-1,0,downloop  
   
.ifdef RGB888
; Now convert to 48 bit colour
; Blue in H(0++,0)>>3
; Red in H(16++,0)>>3
; Green in H(48++,0)<<2
   vmov  HX(0++,0),HX(16++,0) REP 16 ; !!!! Workaround vlookupm bug for r3==0
   vasr  H(16++,0),HX(0++,0),11  REP 16 ; Red
   vasr  H(48++,0),HX(0++,0),5  REP 16  ; Green
.else
; Now convert to 48 bit colour
; Blue in H(16++,0)>>3
; Red in H(0++,0)>>3
; Green in H(48++,0)<<2
   vasr  H(48++,0),HX(16++,0),5  REP 16  ; Green
   vasr  H(0++,0),HX(16++,0),11  REP 16 ; Red
.endif
; Now move into correct positions
   vshl  V(32,0),V(16,0),3    ; Red
   vshl  V(32,3),V(16,1),3
   vshl  V(32,6),V(16,2),3
   vshl  V(32,9),V(16,3),3
   vshl  V(32,12),V(16,4),3
   vshl  V(32,15),V(16,5),3
   vshl  V(32,18),V(16,6),3
   vshl  V(32,21),V(16,7),3
   vshl  V(32,24),V(16,8),3
   vshl  V(32,27),V(16,9),3
   vshl  V(32,30),V(16,10),3
   vshl  V(32,33),V(16,11),3
   vshl  V(32,36),V(16,12),3
   vshl  V(32,39),V(16,13),3
   vshl  V(32,42),V(16,14),3
   vshl  V(32,45),V(16,15),3
   
   vshl  V(32,1),V(48,0),2    ; Green
   vshl  V(32,4),V(48,1),2
   vshl  V(32,7),V(48,2),2
   vshl  V(32,10),V(48,3),2
   vshl  V(32,13),V(48,4),2
   vshl  V(32,16),V(48,5),2
   vshl  V(32,19),V(48,6),2
   vshl  V(32,22),V(48,7),2
   vshl  V(32,25),V(48,8),2
   vshl  V(32,28),V(48,9),2
   vshl  V(32,31),V(48,10),2
   vshl  V(32,34),V(48,11),2
   vshl  V(32,37),V(48,12),2
   vshl  V(32,40),V(48,13),2
   vshl  V(32,43),V(48,14),2
   vshl  V(32,46),V(48,15),2
   
   vshl  V(32,2),V(0,0),3     ; Blue  
   vshl  V(32,5),V(0,1),3
   vshl  V(32,8),V(0,2),3
   vshl  V(32,11),V(0,3),3
   vshl  V(32,14),V(0,4),3
   vshl  V(32,17),V(0,5),3
   vshl  V(32,20),V(0,6),3
   vshl  V(32,23),V(0,7),3
   vshl  V(32,26),V(0,8),3
   vshl  V(32,29),V(0,9),3
   vshl  V(32,32),V(0,10),3
   vshl  V(32,35),V(0,11),3
   vshl  V(32,38),V(0,12),3
   vshl  V(32,41),V(0,13),3
   vshl  V(32,44),V(0,14),3
   vshl  V(32,47),V(0,15),3
   
   vst   H(32++,0),(r0+=r6) REP 16
   vst   H(32++,16),(r0+16+=r6) REP 16
   vst   H(32++,32),(r0+32+=r6) REP 16
   add   r1,16
   add   r0,48
   addcmpbgt   r8,-1,0,rowloop
   ldm   r6-r8,pc,(sp++)

.stabs "vc_palette8_to_rgb888:F19",36,0,1,vc_palette8_to_rgb888

.FORGET


;;;
.section .text$rgb8bpp_load_block,"text"
.globl rgb8bpp_load_block
rgb8bpp_load_block:
      vld H(0++,16)+r2*, (r0+=r1) REP 16
      b lr
   
;;; 
.section .text$rgb8bpp_transpose,"text"
.globl rgb8bpp_transpose
rgb8bpp_transpose:
      mov r1, 0
      mov r2, 64
      mov r3, 1024
rgb8bpp_transpose_loop1:   
      vmov H(32,0)+r1, V(0,16)+r0   ; R
      add r0, 1
      addcmpbne r1, r2, r3, rgb8bpp_transpose_loop1
      mov r0, 0
      mov r1, 0
rgb8bpp_transpose_loop2:   
      vmov V(0,0)+r0, V(32,0)+r1 ; R
      add   r0, 1
      addcmpbne r1, 1, 16, rgb8bpp_transpose_loop2    
      b lr
;;; 
.section .text$rgb8bpp_save_blocks,"text"
.globl rgb8bpp_save_blocks
rgb8bpp_save_blocks:
      addcmpbeq r2, 0, 16, rgb8bpp_save_blocks_16_rows
      mov r3, 0
      shl r2, 6            ; limit of vrf row address to save
      mov r4, 64           ; vrf address increment by one row
rgb8bpp_save_blocks_n_rows:
      vst H(0,0)+r3*, (r0)
      add r0, r1
      addcmpbne r3, r4, r2, rgb8bpp_save_blocks_n_rows
      b lr     
rgb8bpp_save_blocks_16_rows:
      vst H(0++,0)*, (r0+=r1) REP 16
      b lr

;;;    vc_rgb888_font_alpha_blt
;;;
;;; SYNOPSIS
;;;    void vc_rgb888_font_alpha_blt(unsigned short *dest (r0), unsigned short *src (r1), int dest_pitch_bytes (r2),
;;;                                          int src_pitch_bytes (r3), int width (r4), int height(r5), int red,int green,int blue,int alpha)
;;;
;;; FUNCTION
;;;    Transparent blt of nrows rows of dest and src arrays.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb888_font_alpha_blt,"text"
.globl vc_rgb888_font_alpha_blt
vc_rgb888_font_alpha_blt:
      stm r6-r9, lr, (--sp)

.ifdef RGB888
      ld r6, (sp+20)       ; red
      vmov HX(60,0), r6
      ld r6, (sp+24)       ; green
      vmov HX(61,0), r6
      ld r6, (sp+28)       ; blue
      vmov HX(62,0), r6
.else
      ld r6, (sp+28)       ; blue (hereinafter called "red", the lowest byte)
      vmov HX(60,0), r6
      ld r6, (sp+24)       ; green
      vmov HX(61,0), r6
      ld r6, (sp+20)       ; red (hereinafter called "blue")
      vmov HX(62,0), r6
.endif
      ld r6, (sp+32)       ; alpha value
      vmov HX(63,0), r6
      
      ; We want to generate an alpha mask in HX(0,0) (allow for up to 256 rows in the image)
      ; Blit works already unaligned, so no need to extend.
      vmov HX(0++,0),HX(63,0) REP 16   ; Fill up the block with the alpha value
      add r7,r4,-1   ; width rounded down
      asr r7,4       ; r7 now contains the number of blocks across to apply (-1), 0 for a single block
      ; Now we want to compute the space at the end
      neg r4,r4  
      bmask r4,4     ; r4 is now 0 for aligned blocks, up to 15 for addresses just hitting the next block
      mov r6,0xffff
      lsr r6,r4
      vbitplanes -,r6 SETF ; Flags are Z for blocks past the end
      mov r4,r7
      shl r7,4       ; r7 is now the number of bytes added per row
      shl r4,6       ; r4 is now the VRF offset to the last row
      vmov HX(0,0)+r4,0 IFZ   ; Clear the alpha mask for the end of the row
      
      sub r3,r7   ; Correct pitches to take into account addition per row
      mul r7, 3   ; destination is 24 bit colour
      sub r2, r7
      sub r2,48
      sub r3,16
      
      ; We now have r6 spare, and can process all the rows
      ; Use r6 to index through the rows, with an upper limit of r4
      ; Use r8 as a loop increment
      mov r8,64
      mov r9,r4 ; I need r4 for pitch
      mov r4,3  ; pitch
      vbitplanes -, 7 SETF
loopdown:
      mov r6,0
loopacross:
      ; apologies - this is *very* inefficient
      vldleft  V(16, 0++), (r0+=r4) REP 16 IFNZ
;      vldright V(16, 0++), (r0+=r4) REP 16 IFNZ
      vmov HX(18, 32),H(16, 0)  ; R
      vmov HX(19, 32),H(17, 0)  ; G
      vmov HX(20, 32),H(18, 0)  ; B 

      vldleft  H(17,0),(r1)   ; Source
;      vldright H(17,0),(r1)   ; in 4.4 (alpha/gray) format
      
      ; Compute actual alpha value via a mulm
      vand HX(18,0),HX(17,0),0xf0   ; mask of grayscale value
      vmulm HX(22,0),HX(18,0),HX(0,0)+r6   ; alpha value
      vrsub HX(23,0),HX(22,0),256         ; 256-alpha value
      
      ; Scale source gray scale values in order to determine original colour
      ; Start with 8 bit gray, and 8 bit colours, multiply to get 8 bit of colour, 
      vshl HX(17,0),H(17,0),4 ; extract gray values into full 8bit precision
      vmulm HX(18,0),HX(60,0),H(17,0)
      vmulm HX(19,0),HX(61,0),H(17,0)
      vmulm HX(20,0),HX(62,0),H(17,0)
      
      ; Apply alpha blend
      vmulm -,HX(18,0),HX(22,0) CLRA ACC    
      vmulm HX(18,32),H(18,32),HX(23,0) ACC    ; red
      vmulm -,HX(19,0),HX(22,0) CLRA ACC    
      vmulm HX(19,32),H(19,32),HX(23,0) ACC    ; green
      vmulm -,HX(20,0),HX(22,0) CLRA ACC    
      vmulm HX(20,32),H(20,32),HX(23,0) ACC    ; blue
      
      vmov H(16, 0), HX(18, 32)  ; R
      vmov H(17, 0), HX(19, 32)  ; G
      vmov H(18, 0), HX(20, 32)  ; B 
      vstleft  V(16, 0++), (r0+=r4) REP 16 IFNZ
;      vstright V(16, 0++), (r0+=r4) REP 16 IFNZ
      add r0,48
      add r1,16
            
      addcmpble r6,r8,r9,loopacross    
      add r0,r2
      add r1,r3
      addcmpbgt r5,-1,0,loopdown
      
      ldm r6-r9, pc, (sp++)
.stabs "vc_rgb888_font_alpha_blt:F1",36,0,0,vc_rgb888_font_alpha_blt

.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgb888_from_rgba32
;;;
;;; SYNOPSIS
;;;   void rgb888_from_rgba32(
;;;      unsigned long       *dest888,    r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *srca32,     r2
;;;      int                  spitch);    r3
;;;
;;; FUNCTION
;;;   Convert an rgba32 16x16 cell to rgb888

.function rgb888_from_rgba32 ; {{{
      vld      HX(0++,0), (r2+=r3)           REP 16
      vld      HX(0++,32), (r2+32+=r3)       REP 16

      mov      r2, 0
      mov      r3, 0
.loop:
      vmov     V(48,0)+r2, V(0,0)+r3
      vmov     V(48,2)+r2, V(0,1)+r3
      vmov     V(48,1)+r2, V(0,16)+r3

      vmov     V(48,24+0)+r2, V(0,32)+r3
      vmov     V(48,24+2)+r2, V(0,33)+r3
      vmov     V(48,24+1)+r2, V(0,48)+r3

      add      r2, 3
      addcmpbne r3, 2, 16, .loop

      vst      H(48++,0), (r0+=r1)          REP 16
      vst      H(48++,16), (r0+16+=r1)      REP 16
      vst      H(48++,32), (r0+32+=r1)      REP 16
      b        lr
.endfn ; }}}
.FORGET

