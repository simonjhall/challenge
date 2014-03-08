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
; void vcir8_down2_8(unsigned char * dest,       r0
;                    const unsigned char * src,  r1
;                    int destwidth,              r2
;                    int dest_pitch,             r3
;                    int src_pitch,              r4
;                    int smooth_flag);           r5
;
; Reads 32*16 pixels, produces 16*8 pixels.

.section .text$vcir8_down2_8,"text"
.globl vcir8_down2_8
vcir8_down2_8:
   addcmpbne r2, r0, r0, d28_nonz
   b lr
d28_nonz:
   addcmpbne r5, 0, 0, d28_avg
   mov r5, 16
   shl r4, 1     ; Double src_pitch, so we read every other line!
d28_nn_loop:
   vld V(0,0++), (r1+=r4) REP 8      ; Transpose the data as we read it
   vld V(0,16++), (r1+16+=r4) REP 8 ; Putting the other half in the "B" column
   vadd HX(1,0), HX(2,0), 0 ; These HX accesses operate on both halves at once
   vadd HX(2,0), HX(4,0), 0
   vadd HX(3,0), HX(6,0), 0
   vadd HX(4,0), HX(8,0), 0
   vadd HX(5,0), HX(10,0), 0
   vadd HX(6,0), HX(12,0), 0
   vadd HX(7,0), HX(14,0), 0
   vadd H(8++,0), H(0++,16), 0 REP 8  ; Combine the halves in V(0,0..7)
   addscale r1, r5 << 1
   vst V(0,0++), (r0+=r3) REP 8 ; Transpose again as we write
   addcmpblo r0, r5, r2, d28_nn_loop
   b lr

d28_avg:
   mov r5, 16
d28_avg_loop:
   vld H(0++,0), (r1+=r4) REP 16
   vld H(0++,16), (r1+16+=r4) REP 16
   vadd VX(32,0), V(0,0), V(0,1)
   vadd VX(32,1), V(0,2), V(0,3)
   vadd VX(32,2), V(0,4), V(0,5)
   vadd VX(32,3), V(0,6), V(0,7)
   vadd VX(32,4), V(0,8), V(0,9)
   vadd VX(32,5), V(0,10), V(0,11)
   vadd VX(32,6), V(0,12), V(0,13)
   vadd VX(32,7), V(0,14), V(0,15)
   vadd VX(32,8), V(0,16), V(0,17)
   vadd VX(32,9), V(0,18), V(0,19)
   vadd VX(32,10), V(0,20), V(0,21)
   vadd VX(32,11), V(0,22), V(0,23)
   vadd VX(32,12), V(0,24), V(0,25)
   vadd VX(32,13), V(0,26), V(0,27)
   vadd VX(32,14), V(0,28), V(0,29)
   vadd VX(32,15), V(0,30), V(0,31)
   vadd HX(16,0), HX(32,0), HX(33,0)
   vadd HX(17,0), HX(34,0), HX(35,0)
   vadd HX(18,0), HX(36,0), HX(37,0)
   vadd HX(19,0), HX(38,0), HX(39,0)
   vadd HX(20,0), HX(40,0), HX(41,0)
   vadd HX(21,0), HX(42,0), HX(43,0)
   vadd HX(22,0), HX(44,0), HX(45,0)
   vadd HX(23,0), HX(46,0), HX(47,0)
   vmulhn.ss H(0++,0), HX(16++,0), 16384 REP 8
   addscale r1, r5 << 1
   vst H(0++,0), (r0+=r3) REP 8
   addcmpblo r0, r5, r2, d28_avg_loop
   b lr

.stabs "vcir8_down2_8:F1",36,0,0,vcir8_down2_8

.FORGET
