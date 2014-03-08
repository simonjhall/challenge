;;; ===========================================================================
;;; Copyright (c) 2010-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.include "vcinclude/common.inc"

;;; rgb reading functions
;;; source pointer is r0, these functions should read a 16x16 block
;;; and leave R in H(0++,0)REP16, G in H(0++,16)REP16, B in H(0++,32)REP16
;;; they may use the rest of the VRF as scratch
;;; they must update to the next address to read do, 16 pixels to the right
;;; they can only use r9 and r10 as scratch registers
        
.function read_rgb565_block
        v16ld H16(0++,0),  (r0+=r1) REP 16
        
        vand H16(16++,0), H16(0++,0), 0xf81f REP 16  ; red 5 bits + blue 5 bits
        vand H16(16++,32), H16(0++,0), 0x07e0 REP 16 ; green 6 bits
        vmulm H(0++,0), H(16++,16), 0x108 REP 16     ; red
        vmulhd.su H(0++,16), H16(16++,32), 0x2080 REP 16 ; green
        vmulm H(0++,32), H(16++,0), 0x840 REP 16     ; blue
        add r0, 32
        b lr
.endfn

.function read_rgb888_block
        vld H(16++,0), (r0+=r1) REP 16
        vld H(16++,16), (r0+16+=r1) REP 16
        vld H(16++,32), (r0+32+=r1) REP 16

        mov r9, 15
        mov r10, 45
loop_read_rgb888:
        vmov V(0,0)+r9, V(16,0)+r10  ;red
        vmov V(0,16)+r9, V(16,1)+r10 ;green
        vmov V(0,32)+r9, V(16,2)+r10 ;blue
        sub r10, 3
        addcmpbge r9, -1, 0, loop_read_rgb888

        add r0, 48
        b lr
.endfn

.function read_bgr888_block
        vld H(16++,0), (r0+=r1) REP 16
        vld H(16++,16), (r0+16+=r1) REP 16
        vld H(16++,32), (r0+32+=r1) REP 16

        mov r9, 15
        mov r10, 45
loop_read_bgr888:
        vmov V(0,32)+r9, V(16,0)+r10  ;red
        vmov V(0,16)+r9, V(16,1)+r10 ;green
        vmov V(0,0)+r9, V(16,2)+r10 ;blue
        sub r10, 3
        addcmpbge r9, -1, 0, loop_read_bgr888

        add r0, 48
        b lr
.endfn
        
.function read_rgba32_block
        v32ld H32(0++,0), (r0+=r1) REP 16
        add r0, 64
        b lr
.endfn

;;; yuv writing functions
;;; source pointer for y is r2;
;;; source pointer for uv or u is r3
;;; source pointer for v is r7
;;; they must update r2,r3,r7 with the next destination addresses,
;;; one 16x16 block to the right
;;; y pitch is r8 (for yuvuv this means 128)
;;; they can use r9 and r10 for scratch registers
;;; data is stored in YUV444 format with
;;; Y as H(16++,0)REP16, U as H16(32++,0)REP16
;;; V as H16(48++,0)REP16, they may use the rest of the VRF as scratch

.function subsample_vertically
        ;; subsample the u vertically, make block half as tall
        vadd H16(32,0), H16(32,0), H16(33,0)
        vadd H16(33,0), H16(34,0), H16(35,0)
        vadd H16(34,0), H16(36,0), H16(37,0)
        vadd H16(35,0), H16(38,0), H16(39,0)
        vadd H16(36,0), H16(40,0), H16(41,0)
        vadd H16(37,0), H16(42,0), H16(43,0)
        vadd H16(38,0), H16(44,0), H16(45,0)
        vadd H16(39,0), H16(46,0), H16(47,0)
        
        ;; subsample the v vertically, make block half as tall
        vadd H16(48,0), H16(48,0), H16(49,0)
        vadd H16(49,0), H16(50,0), H16(51,0)
        vadd H16(50,0), H16(52,0), H16(53,0)
        vadd H16(51,0), H16(54,0), H16(55,0)
        vadd H16(52,0), H16(56,0), H16(57,0)
        vadd H16(53,0), H16(58,0), H16(59,0)
        vadd H16(54,0), H16(60,0), H16(61,0)
        vadd H16(55,0), H16(62,0), H16(63,0)

        b lr
.endfn

.function subsample_horizontally
        ;; subsample u horizontally, make block half as wide
        vadd V16(0,0), V16(32,0), V16(32,1)
        vadd V16(0,1), V16(32,2), V16(32,3)
        vadd V16(0,2), V16(32,4), V16(32,5)
        vadd V16(0,3), V16(32,6), V16(32,7)
        vadd V16(0,4), V16(32,8), V16(32,9)
        vadd V16(0,5), V16(32,10), V16(32,11)
        vadd V16(0,6), V16(32,12), V16(32,13)
        vadd V16(0,7), V16(32,14), V16(32,15)

        ;; subsample v horizontally, make block half as wide
        vadd V16(16,0), V16(48,0), V16(48,1)
        vadd V16(16,1), V16(48,2), V16(48,3)
        vadd V16(16,2), V16(48,4), V16(48,5)
        vadd V16(16,3), V16(48,6), V16(48,7)
        vadd V16(16,4), V16(48,8), V16(48,9)
        vadd V16(16,5), V16(48,10), V16(48,11)
        vadd V16(16,6), V16(48,12), V16(48,13)
        vadd V16(16,7), V16(48,14), V16(48,15)

        b lr
.endfn

.function write_yuvuv_block        
        ;; write out the y
        vst H(16++,0), r2+=r8 REP 16

        mov r10, lr
        bl subsample_vertically

        ;; subsample horizontally, make block half as wide, interleaving uv data
        vadd V16(0,0), V16(32,0), V16(32,1)
        vadd V16(0,2), V16(32,2), V16(32,3)
        vadd V16(0,4), V16(32,4), V16(32,5)
        vadd V16(0,6), V16(32,6), V16(32,7)
        vadd V16(0,8), V16(32,8), V16(32,9)
        vadd V16(0,10), V16(32,10), V16(32,11)
        vadd V16(0,12), V16(32,12), V16(32,13)
        vadd V16(0,14), V16(32,14), V16(32,15)
        
        vadd V16(0,1), V16(48,0), V16(48,1)
        vadd V16(0,3), V16(48,2), V16(48,3)
        vadd V16(0,5), V16(48,4), V16(48,5)
        vadd V16(0,7), V16(48,6), V16(48,7)
        vadd V16(0,9), V16(48,8), V16(48,9)
        vadd V16(0,11), V16(48,10), V16(48,11)
        vadd V16(0,13), V16(48,12), V16(48,13)
        vadd V16(0,15), V16(48,14), V16(48,15)

        vasr H16(0++,0), H16(0++,0), 2 REP 8

        ;; write out the uv
        vst H(0++,0), r3+=r8 REP 8

        add r2, 16
        add r3, 16

        b r10
.endfn

.function write_yuv420_block
        vbitplanes -, 0xff SETF

        ;; write out the y
        vst H(16++,0), r2+=r8 REP 16

        mov r10, lr
        bl subsample_vertically
        bl subsample_horizontally
        
        lsr r9, r8, 1           ;u/v pitch
        
        vasr H16(0++,0), H16(0++,0), 2 REP 8
        vst H(0++,0), (r3+=r9) REP 8 IFNZ

        vasr H16(16++,0), H16(16++,0), 2 REP 8
        vst H(16++,0), (r7+=r9) REP 8 IFNZ

        add r2, 16
        add r3, 8
        add r7, 8

        b r10
.endfn


;;; yuv422 is subsampled horizontally but not vertically
;;; so the u/v planes are at half width/pitch but the same height as y
.function write_yuv422planar_block
        vbitplanes -, 0xff SETF

        ;; write out the y
        vst H(16++,0), r2+=r8 REP 16

        mov r10, lr
        bl subsample_horizontally

        lsr r9, r8, 1           ;u/v pitch
        
        vasr H16(0++,0), H16(0++,0), 1 REP 16
        vst H(0++,0), (r3+=r9) REP 16 IFNZ

        vasr H16(16++,0), H16(16++,0), 1 REP 16
        vst H(16++,0), (r7+=r9) REP 16 IFNZ

        add r2, 16
        add r3, 8
        add r7, 8

        b r10
.endfn
        
;;; // this function converts a strip from rgb to yuvuv/yuv420
;;; if yuvuv the strip will be a maximum of 128 pixels wide and be a strip of the yuvuv image
;;; if yuv420, the strip will be full image width
;;; 
;;; extern void yuv_strip_from_rgb(unsigned char *src, unsigned int src_pitch,
;;;                                unsigned char *dst_y, unsigned char *dst_uv,
;;;                                void (*read_rgb_block)(),
;;;                                void (*write_yuv)(),
;;;                                unsigned int left,
;;;                                unsigned char *dst_v, unsigned int dst_pitch)

.function yuv_strip_from_rgb
        ;; r0 src
        ;; r1 src pitch
        ;; r2 dest y
        ;; r3 dest uv/u
        ;; r4 src loader fn
        ;; r5 dest writer fn
        ;; r6 pixels left, block counter
        ;; r7 dest v (unused if yuvuv)
        ;; r8 dest y pitch (128 for yuvuv)
        ;; r9 [used by rgb loader if rgb888, yuv420/yuv422planar writer]
        ;; r10 [used by rgb loader if rgb888, yuv writers]
        stm r6-r10, lr, (--sp)
        .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10}

        ld r6, (sp+24)           ;left
        ld r7, (sp+28)           ;dst_v
        ld r8, (sp+32)           ;dst_pitch
        
        add r6, 15
        lsr r6, r6, 4           ;blocks to process in strip

loop_rgb_yuv:
        bl r4                   ;load rgb
        
        ;; Make the Y component
        vmulhn.ss H16(16++,0), H(0++,0), 11913 REP 16   ; R contribution
        vmulhn.su H16(16++,32), H(0++,16), 40108 REP 16
        vadd H16(16++,0), H16(16++,0), H16(16++,32) REP 16 ; add G contribution
        vmulhn.ss H16(16++,32), H(0++,32), 4042 REP 16
        vadd H16(16++,0), H16(16++,0), H16(16++,32) REP 16 ; add B contribution
        vadds H(16++,0), H16(16++,0), 16 REP 16          ; final offset of 16        

        ;; Make the U component.
        vmulhn.ss H16(32++,0), H(0++,0), -6594 REP 16   ; R contribution
        vmulhn.ss H16(32++,32), H(0++,16), -22134 REP 16
        vadd H16(32++,0), H16(32++,0), H16(32++,32) REP 16 ; add G contribution
        vmulhn.ss H16(32++,32), H(0++,32), 28672 REP 16
        vadd H16(32++,0), H16(32++,0), H16(32++,32) REP 16 ; add B contribution
        vadds H16(32++,0), H16(32++,0), 128 REP 16         ; final offset of 128
        
        ;; Make the V component.
        vmulhn.ss H16(48++,0), H(0++,0), 28672 REP 16    ; R contribution
        vmulhn.ss H16(48++,32), H(0++,16), -26033 REP 16
        vadd H16(48++,0), H16(48++,0), H16(48++,32) REP 16 ; add G contribution
        vmulhn.ss H16(48++,32), H(0++,32), -2638 REP 16
        vadd H16(48++,0), H16(48++,0), H16(48++,32) REP 16 ; add B contribution
        vadds H16(48++,0), H16(48++,0), 128 REP 16         ; final offset of 128

        bl r5                   ;write yuv
 
        addcmpbgt r6, -1, 0, loop_rgb_yuv
        
        .cfa_pop {%r10,%r9,%r8,%r7,%r6,%pc}
        ldm r6-r10, pc, (sp++)
.endfn
