;;; ===========================================================================
;;; Copyright (c) 2006-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.FORGET


; === SPECIAL CASE FOR FACTOR OF 2 DOWNSAMPLING (H&V), ALL ALIGNED ===
;
; void vcir16_down2_8(unsigned short * dest,      r0
;                     const unsigned short * src, r1
;                     int dest_width_bytes,       r2
;                     int dest_pitch,             r3
;                     int src_pitch,              r4
;                     int smooth_flag);           r5
;
; Reads 32*16 pixels (of 2 bytes), produces 16*8 pixels (of 2 bytes).

.section .text$vcir16_down2_8,"text"
.globl vcir16_down2_8
vcir16_down2_8:
   addcmpbne r2, r0, r0, d28_nonz
   b lr
d28_nonz:
   addcmpbne r5,0,0,d28_avg
   mov r5, 32
   shl r4, 1     ; Double src_pitch, so we read every other line!
d28_nn_loop:
   vld VX(0,0++), (r1+=r4) REP 8     ; Transpose the data as we read it
   vld VX(16,0++), (r1+32+=r4) REP 8 ; keeping geometry intact (8 wide 16 high)
   vadd HX(1,0), HX(2,0), 0
   vadd HX(2,0), HX(4,0), 0
   vadd HX(3,0), HX(6,0), 0
   vadd HX(4,0), HX(8,0), 0
   vadd HX(5,0), HX(10,0), 0
   vadd HX(6,0), HX(12,0), 0
   vadd HX(7,0), HX(14,0), 0
   vadd HX(8,0), HX(16,0), 0
   vadd HX(9,0), HX(18,0), 0
   vadd HX(10,0), HX(20,0), 0
   vadd HX(11,0), HX(22,0), 0
   vadd HX(12,0), HX(24,0), 0
   vadd HX(13,0), HX(26,0), 0
   vadd HX(14,0), HX(28,0), 0
   vadd HX(15,0), HX(30,0), 0
   addscale r1, r5 << 1
   vst VX(0,0++), (r0+=r3) REP 8 ;  Transpose again as we write
   addcmpblo r0, r5, r2, d28_nn_loop
   b lr

d28_avg:
   stm r6-r8,lr,(--sp)
   mov r6, r4
   shl r4, 1
   mov r7, 64
   sub r7, r6
   mov r8, r2
d28_avg_loop:
   vld VX(0,0++), (r1+=r4) REP 8
   vld VX(16,0++), (r1+32+=r4) REP 8
   add r1, r6
   vld VX(0,8++), (r1+=r4) REP 8
   vld VX(16,8++), (r1+32+=r4) REP 8
   add r1, r7
   vlsr HX(0++,32), HX(0++,0), 11 REP 32 ; Red components
   vlsr HX(32++,0), HX(0++,0), 5 REP 32
   vand HX(32++,32), HX(0++,0), 31 REP 32 ; Blue components
   vand HX(0++,0), HX(32++,0), 63 REP 32  ; Green components
   mov r5, 32
   mov r2, r5
   bl vcir16_d28_shrink
   mov r5, 0
   mov r2, 40
   bl vcir16_d28_shrink
   mov r5, 2080
   mov r2, r5
   bl vcir16_d28_shrink
   vmulhn.ss HX(0++,32), HX(0++,32), 16384 REP 16
   vmulhn.ss VX(32,32++), VX(32,32++), 16384 REP 8
   vshl VX(0,32++), VX(0,32++), 11 REP 8
   vshl VX(32,0++), VX(0,40++), 5 REP 8
   vor VX(32,0++), VX(0,32++), VX(32,0++) REP 8
   vor VX(0,0++), VX(32,0++), VX(32,32++) REP 8
   vst VX(0,0++), (r0+=r3) REP 8
   mov r5, 32
   addcmpblo r0, r5, r8, d28_avg_loop
   ldm r6-r8,pc,(sp++)
   .stabs "vcir16_down2_8:F1",36,0,0,vcir16_down2_8

vcir16_d28_shrink:
   vadd HX(0,0)+r5, HX(0,0)+r5, HX(1,0)+r5
   vadd HX(1,0)+r5, HX(2,0)+r5, HX(3,0)+r5
   vadd HX(2,0)+r5, HX(4,0)+r5, HX(5,0)+r5
   vadd HX(3,0)+r5, HX(6,0)+r5, HX(7,0)+r5
   vadd HX(4,0)+r5, HX(8,0)+r5, HX(9,0)+r5
   vadd HX(5,0)+r5, HX(10,0)+r5, HX(11,0)+r5
   vadd HX(6,0)+r5, HX(12,0)+r5, HX(13,0)+r5
   vadd HX(7,0)+r5, HX(14,0)+r5, HX(15,0)+r5
   vadd HX(8,0)+r5, HX(16,0)+r5, HX(17,0)+r5
   vadd HX(9,0)+r5, HX(18,0)+r5, HX(19,0)+r5
   vadd HX(10,0)+r5, HX(20,0)+r5, HX(21,0)+r5
   vadd HX(11,0)+r5, HX(22,0)+r5, HX(23,0)+r5
   vadd HX(12,0)+r5, HX(24,0)+r5, HX(25,0)+r5
   vadd HX(13,0)+r5, HX(26,0)+r5, HX(27,0)+r5
   vadd HX(14,0)+r5, HX(28,0)+r5, HX(29,0)+r5
   vadd HX(15,0)+r5, HX(30,0)+r5, HX(31,0)+r5
   vadd VX(0,0++)+r2, VX(0,0++)+r5, VX(0,8++)+r5 REP 8
   b lr
   .stabs "vcir16_d28_shrink:F1",36,0,0,vcir16_d28_shrink
   
.FORGET
