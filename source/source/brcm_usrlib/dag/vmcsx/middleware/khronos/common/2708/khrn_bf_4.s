;;; ===========================================================================
; Copyright (c) 2008-2014, Broadcom Corporation
; All rights reserved.
; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.include "middleware/khronos/common/2708/khrn_image_4.inc"

.ifdef __VIDEOCORE4__
   .ifdef __BCM2708A0__
      .define WORKAROUND_HW1297
   .endif
.else
   .define WORKAROUND_HW1297
.endif

.ifndef __VIDECORE4__ ; no uniforms memory attached to vpu on 2708
   .set GR_UNIFORM_BASE, 0x1a00c000 ; todo: fix vcinclude/hardware.inc so it defines this
.endif

; block ld/st functions assume this
.assert (IMAGE_FORMAT_MEM_LAYOUT_SHIFT == 0) && (IMAGE_FORMAT_PIXEL_SIZE_SHIFT == IMAGE_FORMAT_MEM_LAYOUT_BC)
.assert IMAGE_FORMAT_MEM_LAYOUT_BC == 3

;/*****************************************************************************
; helpers
;*****************************************************************************/

.align 32
ppu_index:

.half 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15

; preserves r0, r3
blk_tf_to_rso_16:

   ; r0: vr_data

   v16even  V16(0, 32++) + r0, V16(0, 0++) + r0, V16(0, 4++) + r0 REP 4
   v16even  V16(0, 36++) + r0, V16(0, 8++) + r0, V16(0, 12++) + r0 REP 4
   v16odd  V16(0, 40++) + r0, V16(0, 0++) + r0, V16(0, 4++) + r0 REP 4
   v16odd  V16(0, 44++) + r0, V16(0, 8++) + r0, V16(0, 12++) + r0 REP 4
   add  r1, r0, 12
   add  r2, r0, 3
1:
   v16even  V16(0, 0) + r1, V16(0, 32) + r2, V16(0, 36) + r2
   v16even  V16(0, 1) + r1, V16(0, 40) + r2, V16(0, 44) + r2
   v16odd  V16(0, 2) + r1, V16(0, 32) + r2, V16(0, 36) + r2
   v16odd  V16(0, 3) + r1, V16(0, 40) + r2, V16(0, 44) + r2
   sub  r2, 1
   addcmpbge  r1, -4, r0, 1b
   b  lr

; preserves r0, r3
blk_rso_to_tf_16:

   ; r0: vr_data

   add  r1, r0, 12
   add  r2, r0, 3
1:
   v16interl  V16(0, 32) + r2, V16(0, 0) + r1, V16(0, 2) + r1
   v16interl  V16(0, 40) + r2, V16(0, 1) + r1, V16(0, 3) + r1
   v16interh  V16(0, 36) + r2, V16(0, 0) + r1, V16(0, 2) + r1
   v16interh  V16(0, 44) + r2, V16(0, 1) + r1, V16(0, 3) + r1
   sub  r2, 1
   addcmpbge  r1, -4, r0, 1b
   v16interl  V16(0, 0++) + r0, V16(0, 32++) + r0, V16(0, 40++) + r0 REP 4
   v16interl  V16(0, 8++) + r0, V16(0, 36++) + r0, V16(0, 44++) + r0 REP 4
   v16interh  V16(0, 4++) + r0, V16(0, 32++) + r0, V16(0, 40++) + r0 REP 4
   v16interh  V16(0, 12++) + r0, V16(0, 36++) + r0, V16(0, 44++) + r0 REP 4
   b  lr

; preserves r0, r3
blk_tf_to_rso_32:

   ; r0: vr_data

   .irep i, 1, 2, 3, 6, 7, 11
      v32eor  V32(0, i) + r0, V32(0, i) + r0, V32(0, (i * 4) % 15) + r0
      v32eor  V32(0, (i * 4) % 15) + r0, V32(0, i) + r0, V32(0, (i * 4) % 15) + r0
      v32eor  V32(0, i) + r0, V32(0, i) + r0, V32(0, (i * 4) % 15) + r0
   .endr
   add  r1, r0, 12
1:
   v32odd  -, V32(0, 0) + r1, V32(0, 1) + r1 CLRA SACC
   v32even  V32(0, 0) + r1, V32(0, 0) + r1, V32(0, 1) + r1
   v32mov  V32(0, 1) + r1, 0 SACC
   v32odd  -, V32(0, 2) + r1, V32(0, 3) + r1 CLRA SACC
   v32even  V32(0, 2) + r1, V32(0, 2) + r1, V32(0, 3) + r1
   v32mov  V32(0, 3) + r1, 0 SACC
   v32odd  -, V32(0, 0) + r1, V32(0, 2) + r1 CLRA SACC
   v32even  V32(0, 0) + r1, V32(0, 0) + r1, V32(0, 2) + r1
   v32mov  V32(0, 2) + r1, 0 SACC
   v32odd  -, V32(0, 1) + r1, V32(0, 3) + r1 CLRA SACC
   v32even  V32(0, 1) + r1, V32(0, 1) + r1, V32(0, 3) + r1
   v32mov  V32(0, 3) + r1, 0 SACC
   addcmpbge  r1, -4, r0, 1b
   b  lr

; preserves r0, r3
blk_rso_to_tf_32:

   ; r0: vr_data

   add  r1, r0, 12
1:
   v32interl  -, V32(0, 0) + r1, V32(0, 2) + r1 CLRA SACC
   v32interh  V32(0, 2) + r1, V32(0, 0) + r1, V32(0, 2) + r1
   v32mov  V32(0, 0) + r1, 0 SACC
   v32interl  -, V32(0, 1) + r1, V32(0, 3) + r1 CLRA SACC
   v32interh  V32(0, 3) + r1, V32(0, 1) + r1, V32(0, 3) + r1
   v32mov  V32(0, 1) + r1, 0 SACC
   v32interl  -, V32(0, 0) + r1, V32(0, 1) + r1 CLRA SACC
   v32interh  V32(0, 1) + r1, V32(0, 0) + r1, V32(0, 1) + r1
   v32mov  V32(0, 0) + r1, 0 SACC
   v32interl  -, V32(0, 2) + r1, V32(0, 3) + r1 CLRA SACC
   v32interh  V32(0, 3) + r1, V32(0, 2) + r1, V32(0, 3) + r1
   v32mov  V32(0, 2) + r1, 0 SACC
   addcmpbge  r1, -4, r0, 1b
   .irep i, 1, 2, 3, 6, 7, 11
      v32eor  V32(0, i) + r0, V32(0, i) + r0, V32(0, (i * 4) % 15) + r0
      v32eor  V32(0, (i * 4) % 15) + r0, V32(0, i) + r0, V32(0, (i * 4) % 15) + r0
      v32eor  V32(0, i) + r0, V32(0, i) + r0, V32(0, (i * 4) % 15) + r0
   .endr
   b  lr

;/*****************************************************************************
; khrn_bf_blk_lda
;*****************************************************************************/

khrn_bf_blk_lda::

   ; r0: vr_data
   ; r1: data
   ; r2: stride
   ; r3: format

   bmask  r3, IMAGE_FORMAT_MEM_LAYOUT_BC + IMAGE_FORMAT_PIXEL_SIZE_BC
   switch  r3
1:
   ; IMAGE_FORMAT_1
   .case blk_lda_rso1 - 1b
   .case blk_lda_tf1 - 1b
   .case blk_lda_lt1 - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   ; IMAGE_FORMAT_4
   .case blk_lda_rso4 - 1b
   .case blk_lda_tf4 - 1b
   .case blk_lda_lt4 - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   ; IMAGE_FORMAT_8
   .case blk_lda_rso8 - 1b
   .case blk_lda_tf8 - 1b
   .case blk_lda_lt8 - 1b
   .case blk_lda_rsotf8 - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   ; IMAGE_FORMAT_16
   .case blk_lda_rso16 - 1b
   .case blk_lda_tf16 - 1b
   .case blk_lda_lt16 - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   ; IMAGE_FORMAT_24
   .case blk_lda_rso24 - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   ; IMAGE_FORMAT_32
   .case blk_lda_rso32 - 1b
   .case blk_lda_tf32 - 1b
   .case blk_lda_lt32 - 1b
   .case blk_lda_rsotf32 - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .case blk_lda_invalid - 1b
   .endswitch

blk_lda_invalid:

   bkpt

blk_lda_rso1:

   add  r3, pc, pcrel(ppu_index)
   v16ld  V16(0, 32) + r0, (r3)
   asr  r3, r2, 16
   v32mul.uu  -, V16(0, 32) + r0, r2 CLRA UACC
   v16mul  -, V16(0, 32) + r0, r3 UACCH
   v8lookupml  V8(0, 0) + r0, (r1)
   v8lookupml  V8(0, 16) + r0, (r1 + 1)
.ifdef WORKAROUND_HW1297
   v16mov  V16(0, 0) + r0, V16(0, 0) + r0
.endif
   b  lr

blk_lda_tf1:

   mov  r2, 32 ; tf same as lt but with stride of 32

blk_lda_lt1:

   add  r3, pc, pcrel(ppu_index)
   v16ld  V16(0, 32) + r0, (r3)
   sub  r2, 8
   shl  r2, 2
   v16shl  -, V16(0, 32) + r0, 2 CLRA UACC
   v16bitplanes  -, 0xff00 SETF
   v32mov  -, r2           IFNZ UACC
   v16lookupml  V16(0, 0) + r0, (r1)
.ifdef WORKAROUND_HW1297
   v16mov  V16(0, 0) + r0, V16(0, 0) + r0
.endif
   b  lr

blk_lda_rso4:

   v16bitplanes  -, 0x00ff           SETF
   v8ld  H8(0++, 0) + r0, (r1 += r2) IFNZ REP 16
   b  lr

blk_lda_tf4:

   mov  r2, 32 ; tf same as lt but with stride of 32

blk_lda_lt4:

   v32ld  V32(0, 0) + r0, (r1)
   addscale  r1, r2 << 3
   v32ld  V32(0, 1) + r0, (r1)
   b  lr

blk_lda_rsotf8:

   mov  r2, 16

blk_lda_rso8:

   v8ld  H8(0++, 0) + r0, (r1 += r2) REP 16
   b  lr

blk_lda_tf8:

   mov  r2, 32 ; tf same as lt but with stride of 32

blk_lda_lt8:

   mov  r3, 64
   v32ld  V32(0, 0++) + r0, (r1 += r3) REP 2
   addscale  r1, r2 << 3
   v32ld  V32(0, 2++) + r0, (r1 += r3) REP 2
   b  lr

blk_lda_rso16:

   v16ld  H16(0++, 0) + r0, (r1 += r2) REP 16
   b  lr

blk_lda_tf16:

   mov  r2, 64 ; tf same as lt but with stride of 64

blk_lda_lt16:

   mov  r3, 64
   v32ld  V32(0, 0++) + r0, (r1 += r3) REP 2
   addscale  r1, r2 << 2
   v32ld  V32(0, 2++) + r0, (r1 += r3) REP 2
   addscale  r1, r2 << 2
   v32ld  V32(0, 4++) + r0, (r1 += r3) REP 2
   addscale  r1, r2 << 2
   v32ld  V32(0, 6++) + r0, (r1 += r3) REP 2
   b  lr

blk_lda_rso24:

   v8ld  H8(0++, 0) + r0, (r1 += r2) REP 16
   v8ld  H8(0++, 16) + r0, ((r1 + 16) += r2) REP 16
   v8ld  H8(0++, 32) + r0, ((r1 + 32) += r2) REP 16
   b  lr

blk_lda_rsotf32:

   mov  r2, 64

blk_lda_rso32:

   v32ld  H32(0++, 0) + r0, (r1 += r2) REP 16
   b  lr

blk_lda_tf32:

   mov  r2, 64
   v32ld  V32(0, 0++) + r0, (r1 += r2) REP 16
   b  lr

blk_lda_lt32:

   mov  r3, 64
   v32ld  V32(0, 0++) + r0, (r1 += r3) REP 4
   addscale  r1, r2 << 2
   v32ld  V32(0, 4++) + r0, (r1 += r3) REP 4
   addscale  r1, r2 << 2
   v32ld  V32(0, 8++) + r0, (r1 += r3) REP 4
   addscale  r1, r2 << 2
   v32ld  V32(0, 12++) + r0, (r1 += r3) REP 4
   b  lr

;/*****************************************************************************
; khrn_bf_blk_ldam
;*****************************************************************************/

khrn_bf_blk_ldam::

   ; r0: vr_data
   ; r1: data
   ; r2: stride
   ; r3: mask_x
   ; r4: mask_y
   ; r5: format

   bmask  r5, IMAGE_FORMAT_MEM_LAYOUT_BC + IMAGE_FORMAT_PIXEL_SIZE_BC
   switch  r5
1:
   ; IMAGE_FORMAT_1
   .case blk_ldam_rso1 - 1b
   .case blk_ldam_tf1 - 1b
   .case blk_ldam_lt1 - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   ; IMAGE_FORMAT_4
   .case blk_ldam_rso4 - 1b
   .case blk_ldam_tf4 - 1b
   .case blk_ldam_lt4 - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   ; IMAGE_FORMAT_8
   .case blk_ldam_rso8 - 1b
   .case blk_ldam_tf8 - 1b
   .case blk_ldam_lt8 - 1b
   .case blk_ldam_rsotf8 - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   ; IMAGE_FORMAT_16
   .case blk_ldam_rso16 - 1b
   .case blk_ldam_tf16 - 1b
   .case blk_ldam_lt16 - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   ; IMAGE_FORMAT_24
   .case blk_ldam_rso24 - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   ; IMAGE_FORMAT_32
   .case blk_ldam_rso32 - 1b
   .case blk_ldam_tf32 - 1b
   .case blk_ldam_lt32 - 1b
   .case blk_ldam_rsotf32 - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .case blk_ldam_invalid - 1b
   .endswitch

blk_ldam_invalid:

   bkpt

blk_ldam_rso1:

   add  r5, pc, pcrel(ppu_index)
   v16ld  V16(0, 32) + r0, (r5)
   asr  r5, r2, 16
   v32mul.uu  -, V16(0, 32) + r0, r2 CLRA UACC
   v16mul  -, V16(0, 32) + r0, r5 UACCH
   v16mov  V16(0, 32) + r0, r3
   v16and  -, V16(0, 32) + r0, 0x00ff SETF
   v16bitplanes  -, r4                IFNZ SETF
   v8lookupml  V8(0, 33) + r0, (r1)   IFNZ
   v16and  -, V16(0, 32) + r0, 0xff00   SETF
   v16bitplanes  -, r4                  IFNZ SETF
   v8lookupml  V8(0, 49) + r0, (r1 + 1) IFNZ
   v16bitplanes  -, r4                                    SETF
   v16bic  V16(0, 0) + r0, V16(0, 0) + r0, r3             IFNZ
   v16and  V16(0, 33) + r0, V16(0, 33) + r0, r3           IFNZ ; no need for hw-1297 workaround
   v16or  V16(0, 0) + r0, V16(0, 0) + r0, V16(0, 33) + r0 IFNZ
   b  lr

blk_ldam_tf1:

   mov  r2, 32 ; tf same as lt but with stride of 32

blk_ldam_lt1:

   add  r5, pc, pcrel(ppu_index)
   v16ld  V16(0, 32) + r0, (r5)
   sub  r2, 8
   shl  r2, 2
   v16shl  -, V16(0, 32) + r0, 2 CLRA UACC
   v16bitplanes  -, 0xff00 SETF
   v32mov  -, r2           IFNZ UACC
   v16bitplanes  -, r4                                    SETF
   v16lookupml  V16(0, 32) + r0, (r1)                     IFNZ
   v16bic  V16(0, 0) + r0, V16(0, 0) + r0, r3             IFNZ
   v16and  V16(0, 32) + r0, V16(0, 32) + r0, r3           IFNZ ; no need for hw-1297 workaround
   v16or  V16(0, 0) + r0, V16(0, 0) + r0, V16(0, 32) + r0 IFNZ
   b  lr

blk_ldam_rso4:

   v16bitplanes  H16(0, 32) + r0, r3
   v16odd  -, H16(0, 32) + r0, 0  SETF
   v16even  -, H16(0, 32) + r0, 0 IFZ SETF
   shl  r0, 16
   or  r0, r4
   ; x mask stuff
   sub  r4, r3, 1
   msb  r5, r3
   eor  r4, r3
   add  r5, 1
   msb  r4
   shl  r5, 17
   or  r3, r4, r5
   ; y mask stuff
   lsr  r4, r0, 16
   sub  r5, r0, 1
   eor  r5, r0
   msb  r5
   addscale  r4, r5 << 6
   mul  r5, r2
   add  r1, r5
   ; can do x masking just with IFNZ?
   btest  r3, 0
   btest.eq  r3, 17
   bne  1f
   ; yes, load
   bmask  r0, 16
   count  r0
   v8ld  H8(0++, 0) + r4, (r1 += r2) IFNZ REP r0
   b  lr
1:
   ; no, save unmaskable columns first
   ror  r0, 16
   lsr  r5, r3, 1
   add  r5, r0
   v16and  V8(0, 32) + r0, V8(0, 0) + r5, 0x0f
   lsr  r5, r3, 18
   add  r5, r0
   v16and  V8(0, 33) + r0, V8(0, 0) + r5, 0xf0
   lsr  r0, 16
   count  r0
   v8ld  H8(0++, 0) + r4, (r1 += r2) IFNZ REP r0
   ; copy masked parts of columns back
   mov  r1, -1
   bmask  r1, r0
   v16bitplanes  -, r1 SETF
   btest  r3, 0
   beq  1f
   lsr  r0, r3, 1
   add  r0, r4
   v16and  V8(0, 0) + r0, V8(0, 0) + r0, 0xf0          IFNZ
   v16or  V8(0, 0) + r0, V8(0, 0) + r0, V8(0, 32) + r4 IFNZ
1:
   btest  r3, 17
   beq  1f
   lsr  r3, 18
   add  r3, r4
   v16and  V8(0, 0) + r3, V8(0, 0) + r3, 0x0f          IFNZ
   v16or  V8(0, 0) + r3, V8(0, 0) + r3, V8(0, 33) + r4 IFNZ
1:
   b  lr

blk_ldam_tf4:

   mov  r2, 32 ; tf same as lt but with stride of 32

blk_ldam_lt4:

   ; x mask
   v16bitplanes  V16(0, 8) + r0, r3
   v16interh  V16(0, 9) + r0, V16(0, 8) + r0, V16(0, 8) + r0
   v16interl  V16(0, 8) + r0, V16(0, 8) + r0, V16(0, 8) + r0
   v16interh  V16(0, 40++) + r0, V16(0, 8++) + r0, V16(0, 8++) + r0 REP 2
   v16interl  V16(0, 8++) + r0, V16(0, 8++) + r0, V16(0, 8++) + r0 REP 2
   v16bitplanes  V16(0, 8++) + r0, V16(0, 8++) + r0 REP 2
   v16bitplanes  V16(0, 40++) + r0, V16(0, 40++) + r0 REP 2
   v32interl  V32(0, 8) + r0, V32(0, 8) + r0, V32(0, 9) + r0 SETF
   ; y mask
   v16bitplanes  V16(0, 9) + r0, r4
   ; load and merge first
   v16interl  -, V16(0, 9) + r0, V16(0, 9) + r0              IFNZ SETF
   v32ld  V32(0, 2) + r0, (r1)                               IFNZ
   v32bic  V32(0, 0) + r0, V32(0, 0) + r0, V32(0, 8) + r0    IFNZ
   v32and  V32(0, 2) + r0, V32(0, 2) + r0, V32(0, 8) + r0    IFNZ
   v32or  V32(0, 0) + r0, V32(0, 0) + r0, V32(0, 2) + r0     IFNZ
   ; advance pointer
   addscale  r1, r2 << 3
   ; load and merge second
   v32mov  -, V32(0, 8) + r0                              SETF
   v16interh  -, V16(0, 9) + r0, V16(0, 9) + r0           IFNZ SETF
   v32ld  V32(0, 2) + r0, (r1)                            IFNZ
   v32bic  V32(0, 1) + r0, V32(0, 1) + r0, V32(0, 8) + r0 IFNZ
   v32and  V32(0, 2) + r0, V32(0, 2) + r0, V32(0, 8) + r0 IFNZ
   v32or  V32(0, 1) + r0, V32(0, 1) + r0, V32(0, 2) + r0  IFNZ
   b  lr

blk_ldam_rsotf8:

   mov  r2, 16

blk_ldam_rso8:

   v16bitplanes  -, r3 SETF
   sub  r3, r4, 1
   eor  r3, r4
   msb  r3
   addscale  r5, r0, r3 << 6
   mul  r3, r2
   count  r0, r4
   add  r1, r3
   v8ld  H8(0++, 0) + r5, (r1 += r2) IFNZ REP r0
   b  lr

blk_ldam_tf8:

   mov  r2, 32 ; tf same as lt but with stride of 32

blk_ldam_lt8:

   sub  r2, 14 ; back to start
   ; create load mask
   v16bitplanes  V16(0, 32) + r0, r3
   v16bitplanes  V16(0, 33) + r0, r4
   v16bitplanes  -, 0x00ff                                   SETF
   v16and  V16(0, 32) + r0, V16(0, 32) + r0, 0x0f0f          IFNZ
   v16and  V16(0, 32) + r0, V16(0, 32) + r0, 0xf0f0          IFZ
   v16or  V16(0, 32) + r0, V16(8, 32) + r0, V16(0, 32) + r0  IFNZ
   v16or  V16(0, 32) + r0, V16(56, 32) + r0, V16(0, 32) + r0 IFZ
   v16add  V8(0, 49) + r0, V8(1, 49) + r0, 0
   v16even  V16(0, 33) + r0, V16(0, 33) + r0, V16(0, 33) + r0
   v16bitplanes  -, 0x0ff0                      SETF
   v16add  V16(0, 33) + r0, V16(60, 33) + r0, 0 IFNZ
   v16bitplanes  V16(0, 33) + r0, V16(0, 33) + r0
   v16and  V16(0, 32) + r0, V16(0, 32) + r0, V16(0, 33) + r0
   ; load
   mov  r3, r0
   add  r4, r0, 7
   add  r5, r0, 16
1:
   v16and  -, V16(0, 32) + r0, 0x1 SETF
   cmp  r3, r4
   v8ld  V8(0, 0) + r3, (r1)       IFNZ
   add.ne  r1, 16
   v16lsr  V16(0, 32) + r0, V16(0, 32) + r0, 1
   add.eq  r1, r2 << 3
   addcmpbne  r3, 1, r5, 1b
   b  lr

blk_ldam_rso16:

   v16bitplanes  -, r3 SETF
   sub  r3, r4, 1
   eor  r3, r4
   msb  r3
   addscale  r5, r0, r3 << 6
   mul  r3, r2
   count  r0, r4
   add  r1, r3
   v16ld  H16(0++, 0) + r5, (r1 += r2) IFNZ REP r0
   b  lr

blk_ldam_tf16:

   mov  r2, 64 ; tf same as lt but with stride of 64

blk_ldam_lt16:

   sub  r2, 24 ; back to start
   ; create load mask
   v16bitplanes  V16(0, 32) + r0, r3
   v16bitplanes  V16(0, 33) + r0, r4
   v16bitplanes  -, 0x00ff                                   SETF
   v16and  V16(0, 32) + r0, V16(0, 32) + r0, 0x3333          IFNZ
   v16and  V16(0, 32) + r0, V16(0, 32) + r0, 0xcccc          IFZ
   v16or  V16(0, 32) + r0, V16(8, 32) + r0, V16(0, 32) + r0  IFNZ
   v16or  V16(0, 32) + r0, V16(56, 32) + r0, V16(0, 32) + r0 IFZ
   v16add  V8(0, 49) + r0, V8(1, 49) + r0, 0
   v16bitplanes  -, 0xaaaa                     SETF
   v16add  V16(0, 33) + r0, V16(1, 33) + r0, 0 IFNZ
   v16bitplanes  -, 0xcccc                      SETF
   v16add  V16(0, 33) + r0, V16(62, 33) + r0, 0 IFNZ
   v16bitplanes  V16(0, 33) + r0, V16(0, 33) + r0
   v16and  V16(0, 32) + r0, V16(0, 32) + r0, V16(0, 33) + r0
   ; load
   mov  r3, r0
   add  r4, r0, 16
1:
   bmask  r5, r3, 2
   v16and  -, V16(0, 32) + r0, 0x1 SETF
   cmp  r5, 3
   v16ld  V16(0, 0) + r3, (r1)     IFNZ
   add.ne  r1, 32
   v16lsr  V16(0, 32) + r0, V16(0, 32) + r0, 1
   add.eq  r1, r2 << 2
   addcmpbne  r3, 1, r4, 1b
   b  lr

blk_ldam_rso24:

   shl  r4, 16
   or  r0, r4
   ; x mask stuff
   sub  r4, r3, 1
   msb  r5, r3
   eor  r4, r3
   add  r5, 1
   msb  r4
   shl  r5, 16
   or  r3, r4, r5
   addscale  r3, r3 << 1
   mov  r5, -1
   bmask  r4, r3, 16
   cmp  r4, 16
   bmask.lt  r5, r4
   not  r5
   lsr  r4, r3, 16
   cmp  r4, 16
   bmask.lt  r5, r4
   v16bitplanes  -, r5 SETF
   mov  r5, -1
   bmask  r4, r3, 16
   sub  r4, 16
   max  r4, 0
   bmask  r5, r4
   not  r5
   lsr  r3, 16
   sub  r3, 16
   max  r3, 0
   cmp  r3, 32
   bmask.lt  r5, r3
   ; y mask stuff
   mov  r4, r0
   lsr  r0, 16
   sub  r3, r0, 1
   eor  r3, r0
   msb  r3
   addscale  r4, r3 << 6
   mul  r3, r2
   count  r0
   add  r1, r3
   ; loads
   lsr  r3, r5, 16
   v8ld  H8(0++, 0) + r4, (r1 += r2) IFNZ REP r0 ; top 16 bits of r4 ignored
   v16bitplanes  -, r5                       SETF
   v8ld  H8(0++, 16) + r4, ((r1 + 16) += r2) IFNZ REP r0
   v16bitplanes  -, r3                       SETF
   v8ld  H8(0++, 32) + r4, ((r1 + 32) += r2) IFNZ REP r0
   b  lr

blk_ldam_rsotf32:

   mov  r2, 64

blk_ldam_rso32:

   v16bitplanes  -, r3 SETF
   sub  r3, r4, 1
   eor  r3, r4
   msb  r3
   addscale  r5, r0, r3 << 6
   mul  r3, r2
   count  r0, r4
   add  r1, r3
   v32ld  H32(0++, 0) + r5, (r1 += r2) IFNZ REP r0
   b  lr

blk_ldam_tf32:

   mov  r2, 64 ; tf same as lt but with stride of 64

blk_ldam_lt32:

   shl  r3, 16
   or  r0, r3
1:
   mov  r3, 0
   .irep i, 0, 1, 2, 3
      btest  r4, i
      bitset.ne  r3, (i * 4)
   .endr
   lsr  r4, 4
   mul  r3, 0xf
   .irep i, 0, 1, 2, 3
      lsr  r5, r0, (16 + (i * 4))
      .if i != 3
         bmask  r5, 4
      .endif
      mul  r5, 0x1111
      and  r5, r3
      v16bitplanes  -, r5                    SETF
      v32ld  V32(0, i) + r0, (r1 + (i * 64)) IFNZ ; top 16 bits of r0 ignored
   .endr
   addscale  r1, r2 << 2
   or  r3, r0, 15
   addcmpble  r0, 4, r3, 1b
   b  lr

;/*****************************************************************************
; khrn_bf_blk_lda_unpack/khrn_bf_blk_ldam_unpack
;*****************************************************************************/

khrn_bf_blk_lda_unpack::

   ; r0: vr_data
   ; r1: tf
   ; r2: format

   bmask  r2, IMAGE_FORMAT_MEM_LAYOUT_BC + IMAGE_FORMAT_PIXEL_SIZE_BC
   switch  r2
1:
   ; IMAGE_FORMAT_1
   .case blk_lda_unpack_rso1 - 1b
   .case blk_lda_unpack_tf1 - 1b
   .case blk_lda_unpack_lt1 - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   ; IMAGE_FORMAT_4
   .case blk_lda_unpack_rso4 - 1b
   .case blk_lda_unpack_tf4 - 1b
   .case blk_lda_unpack_lt4 - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   ; IMAGE_FORMAT_8
   .case blk_lda_unpack_rso8 - 1b
   .case blk_lda_unpack_tf8 - 1b
   .case blk_lda_unpack_lt8 - 1b
   .case blk_lda_unpack_rsotf8 - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   ; IMAGE_FORMAT_16
   .case blk_lda_unpack_rso16 - 1b
   .case blk_lda_unpack_tf16 - 1b
   .case blk_lda_unpack_lt16 - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   ; IMAGE_FORMAT_24
   .case blk_lda_unpack_rso24 - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   ; IMAGE_FORMAT_32
   .case blk_lda_unpack_rso32 - 1b
   .case blk_lda_unpack_tf32 - 1b
   .case blk_lda_unpack_lt32 - 1b
   .case blk_lda_unpack_rsotf32 - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .case blk_lda_unpack_invalid - 1b
   .endswitch

khrn_bf_blk_ldam_unpack::

   ; r0: vr_data
   ; r1: tf
   ; r2: format

   bmask  r2, IMAGE_FORMAT_MEM_LAYOUT_BC + IMAGE_FORMAT_PIXEL_SIZE_BC
   switch  r2
1:
   ; IMAGE_FORMAT_1
   .case blk_ldam_unpack_rso1 - 1b
   .case blk_ldam_unpack_tf1 - 1b
   .case blk_ldam_unpack_lt1 - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   ; IMAGE_FORMAT_4
   .case blk_ldam_unpack_rso4 - 1b
   .case blk_ldam_unpack_tf4 - 1b
   .case blk_ldam_unpack_lt4 - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   ; IMAGE_FORMAT_8
   .case blk_ldam_unpack_rso8 - 1b
   .case blk_ldam_unpack_tf8 - 1b
   .case blk_ldam_unpack_lt8 - 1b
   .case blk_ldam_unpack_rsotf8 - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   ; IMAGE_FORMAT_16
   .case blk_ldam_unpack_rso16 - 1b
   .case blk_ldam_unpack_tf16 - 1b
   .case blk_ldam_unpack_lt16 - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   ; IMAGE_FORMAT_24
   .case blk_ldam_unpack_rso24 - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   ; IMAGE_FORMAT_32
   .case blk_ldam_unpack_rso32 - 1b
   .case blk_ldam_unpack_tf32 - 1b
   .case blk_ldam_unpack_lt32 - 1b
   .case blk_ldam_unpack_rsotf32 - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .case blk_ldam_unpack_invalid - 1b
   .endswitch

blk_lda_unpack_invalid:
blk_ldam_unpack_invalid:

   bkpt

blk_lda_unpack_rso8:
blk_lda_unpack_rsotf8:
blk_lda_unpack_rso16:
blk_ldam_unpack_rso8:
blk_ldam_unpack_rsotf8:
blk_ldam_unpack_rso16:

   cmp  r1, 0
   bne  blk_rso_to_tf_16
   b  lr

blk_lda_unpack_rso32:
blk_lda_unpack_rsotf32:
blk_ldam_unpack_rso32:
blk_ldam_unpack_rsotf32:

   cmp  r1, 0
   bne  blk_rso_to_tf_32
   b  lr

blk_lda_unpack_tf32:
blk_lda_unpack_lt32:
blk_ldam_unpack_tf32:
blk_ldam_unpack_lt32:

   cmp  r1, 0
   beq  blk_tf_to_rso_32
   b  lr

blk_lda_unpack_rso1:
blk_lda_unpack_tf1:
blk_lda_unpack_lt1:
blk_ldam_unpack_rso1:
blk_ldam_unpack_tf1:
blk_ldam_unpack_lt1:

   v16bitplanes  H16(0++, 0) + r0, H16(0++, 0) + r0 REP 16
   cmp  r1, 0
   bne  blk_rso_to_tf_16
   b  lr

blk_lda_unpack_rso4:
blk_ldam_unpack_rso4:

   v16lsr  V8(0, 32++) + r0, V8(0, 0++) + r0, 4 REP 8
   v16interl  H8(0++, 0) + r0, H8(0++, 0) + r0, H8(0++, 32) + r0 REP 16
   cmp  r1, 0
   bne  blk_rso_to_tf_16
   b  lr

blk_lda_unpack_tf4:
blk_lda_unpack_lt4:
blk_ldam_unpack_tf4:
blk_ldam_unpack_lt4:

   v32odd  V32(0, 8) + r0, V32(0, 0) + r0, V32(0, 1) + r0
   v32even  V32(0, 0) + r0, V32(0, 0) + r0, V32(0, 1) + r0
   .irep i, 1, 2, 3, 4, 5, 6, 7
      v32lsr  V32(0, i) + r0, V32(0, 0) + r0, (i * 4)
   .endr
   .irep i, 1, 2, 3, 4, 5, 6, 7
      v32lsr  V32(0, 8 + i) + r0, V32(0, 8) + r0, (i * 4)
   .endr
   cmp  r1, 0
   bne  blk_rso_to_tf_16
   b  lr

blk_lda_unpack_tf8:
blk_lda_unpack_lt8:

   v32odd  V32(0, 12) + r0, V32(0, 1) + r0, V32(0, 3) + r0
   v32even  V32(0, 8) + r0, V32(0, 1) + r0, V32(0, 3) + r0
   v32odd  V32(0, 4) + r0, V32(0, 0) + r0, V32(0, 2) + r0
   v32even  V32(0, 0) + r0, V32(0, 0) + r0, V32(0, 2) + r0
   add  r2, r0, 12
1:
   v16mov  V8(0, 1) + r2, V8(0, 16) + r2
   v16mov  V8(0, 2) + r2, V8(0, 32) + r2
   v16mov  V8(0, 3) + r2, V8(0, 48) + r2
   addcmpbge  r2, -4, r0, 1b
   cmp  r1, 0
   bne  blk_rso_to_tf_16 ; unpacking to tf could be faster, but it shouldn't happen often
   b  lr

blk_ldam_unpack_tf8:
blk_ldam_unpack_lt8:

   v16mov  V8(0, 32++) + r0, V8(0, 0++) + r0 REP 4
   v16mov  V8(0, 36++) + r0, V8(0, 8++) + r0 REP 4
   v16mov  V8(0, 40++) + r0, V8(0, 4++) + r0 REP 4
   v16mov  V8(0, 44++) + r0, V8(0, 12++) + r0 REP 4
   v16interl  V8(0, 0++) + r0, H8(0++, 32) + r0, H8(8++, 32) + r0 REP 8
   v16interh  V8(0, 8++) + r0, H8(0++, 32) + r0, H8(8++, 32) + r0 REP 8
   cmp  r1, 0
   bne  blk_rso_to_tf_16 ; unpacking to tf could be faster, but it shouldn't happen often
   b  lr

blk_lda_unpack_tf16:
blk_lda_unpack_lt16:

   v32even  V32(0, 8++) + r0, V32(0, 0++) + r0, V32(0, 2++) + r0 REP 2
   v32even  V32(0, 10++) + r0, V32(0, 4++) + r0, V32(0, 6++) + r0 REP 2
   v32odd  V32(0, 12++) + r0, V32(0, 0++) + r0, V32(0, 2++) + r0 REP 2
   v32odd  V32(0, 14++) + r0, V32(0, 4++) + r0, V32(0, 6++) + r0 REP 2
   .irep i, 0, 1
      v32even  V32(0, 0 + (i * 8)) + r0, V32(0, 8 + i) + r0, V32(0, 10 + i) + r0
      v32even  V32(0, 2 + (i * 8)) + r0, V32(0, 12 + i) + r0, V32(0, 14 + i) + r0
      v32odd  V32(0, 4 + (i * 8)) + r0, V32(0, 8 + i) + r0, V32(0, 10 + i) + r0
      v32odd  V32(0, 6 + (i * 8)) + r0, V32(0, 12 + i) + r0, V32(0, 14 + i) + r0
   .endr
   .irep i, 0, 1, 2, 3, 4, 5, 6, 7
      v16mov  V16(0, 1 + (i * 2)) + r0, V16(0, 32 + (i * 2)) + r0
   .endr
   cmp  r1, 0
   bne  blk_rso_to_tf_16 ; unpacking to tf could be faster, but it shouldn't happen often
   b  lr

blk_ldam_unpack_tf16:
blk_ldam_unpack_lt16:

   v16mov  V16(0, 32++) + r0, V16(0, 0++) + r0 REP 2
   v16mov  V16(0, 34++) + r0, V16(0, 4++) + r0 REP 2
   v16mov  V16(0, 36++) + r0, V16(0, 8++) + r0 REP 2
   v16mov  V16(0, 38++) + r0, V16(0, 12++) + r0 REP 2
   v16mov  V16(0, 40++) + r0, V16(0, 2++) + r0 REP 2
   v16mov  V16(0, 42++) + r0, V16(0, 6++) + r0 REP 2
   v16mov  V16(0, 44++) + r0, V16(0, 10++) + r0 REP 2
   v16mov  V16(0, 46++) + r0, V16(0, 14++) + r0 REP 2
   v16interl  V16(0, 0++) + r0, H16(0++, 32) + r0, H16(8++, 32) + r0 REP 8
   v16interh  V16(0, 8++) + r0, H16(0++, 32) + r0, H16(8++, 32) + r0 REP 8
   cmp  r1, 0
   bne  blk_rso_to_tf_16 ; unpacking to tf could be faster, but it shouldn't happen often
   b  lr

blk_lda_unpack_rso24:
blk_ldam_unpack_rso24:

   add  r2, r0, 48
   mov  r3, r0
   mov  r4, r0
1:
   v16mov  V8(0, 48) + r3, V8(0, 1) + r0
   v16mov  V8(0, 0) + r4, V8(0, 0) + r0
   v16mov  V8(0, 1) + r4, V8(0, 2) + r0
   add  r3, 1
   add  r4, 2
   addcmpbne  r0, 3, r2, 1b
   sub  r0, 48
   v16odd  H8(0++, 32) + r0, H8(0++, 0) + r0, H8(0++, 16) + r0 REP 16
   v16even  H8(0++, 0) + r0, H8(0++, 0) + r0, H8(0++, 16) + r0 REP 16
   v16mov  H8(0++, 16) + r0, H8(0++, 48) + r0 REP 16
   cmp  r1, 0
   bne  blk_rso_to_tf_32 ; unpacking to tf could be faster, but it shouldn't happen often
   b  lr

;/*****************************************************************************
; khrn_bf_blk_sta_pack/khrn_bf_blk_stam_pack
;*****************************************************************************/

khrn_bf_blk_sta_pack::

   ; r0: vr_data
   ; r1: tf
   ; r2: format

   bmask  r2, IMAGE_FORMAT_MEM_LAYOUT_BC + IMAGE_FORMAT_PIXEL_SIZE_BC
   switch  r2
1:
   ; IMAGE_FORMAT_1
   .case blk_sta_pack_rso1 - 1b
   .case blk_sta_pack_tf1 - 1b
   .case blk_sta_pack_lt1 - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   ; IMAGE_FORMAT_4
   .case blk_sta_pack_rso4 - 1b
   .case blk_sta_pack_tf4 - 1b
   .case blk_sta_pack_lt4 - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   ; IMAGE_FORMAT_8
   .case blk_sta_pack_rso8 - 1b
   .case blk_sta_pack_tf8 - 1b
   .case blk_sta_pack_lt8 - 1b
   .case blk_sta_pack_rsotf8 - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   ; IMAGE_FORMAT_16
   .case blk_sta_pack_rso16 - 1b
   .case blk_sta_pack_tf16 - 1b
   .case blk_sta_pack_lt16 - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   ; IMAGE_FORMAT_24
   .case blk_sta_pack_rso24 - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   ; IMAGE_FORMAT_32
   .case blk_sta_pack_rso32 - 1b
   .case blk_sta_pack_tf32 - 1b
   .case blk_sta_pack_lt32 - 1b
   .case blk_sta_pack_rsotf32 - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .case blk_sta_pack_invalid - 1b
   .endswitch

khrn_bf_blk_stam_pack::

   ; r0: vr_data
   ; r1: tf
   ; r2: format

   bmask  r2, IMAGE_FORMAT_MEM_LAYOUT_BC + IMAGE_FORMAT_PIXEL_SIZE_BC
   switch  r2
1:
   ; IMAGE_FORMAT_1
   .case blk_stam_pack_rso1 - 1b
   .case blk_stam_pack_tf1 - 1b
   .case blk_stam_pack_lt1 - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   ; IMAGE_FORMAT_4
   .case blk_stam_pack_rso4 - 1b
   .case blk_stam_pack_tf4 - 1b
   .case blk_stam_pack_lt4 - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   ; IMAGE_FORMAT_8
   .case blk_stam_pack_rso8 - 1b
   .case blk_stam_pack_tf8 - 1b
   .case blk_stam_pack_lt8 - 1b
   .case blk_stam_pack_rsotf8 - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   ; IMAGE_FORMAT_16
   .case blk_stam_pack_rso16 - 1b
   .case blk_stam_pack_tf16 - 1b
   .case blk_stam_pack_lt16 - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   ; IMAGE_FORMAT_24
   .case blk_stam_pack_rso24 - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   ; IMAGE_FORMAT_32
   .case blk_stam_pack_rso32 - 1b
   .case blk_stam_pack_tf32 - 1b
   .case blk_stam_pack_lt32 - 1b
   .case blk_stam_pack_rsotf32 - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .case blk_stam_pack_invalid - 1b
   .endswitch

blk_sta_pack_invalid:
blk_stam_pack_invalid:

   bkpt

blk_sta_pack_rso8:
blk_sta_pack_rsotf8:
blk_sta_pack_rso16:
blk_stam_pack_rso8:
blk_stam_pack_rsotf8:
blk_stam_pack_rso16:

   cmp  r1, 0
   bne  blk_tf_to_rso_16
   b  lr

blk_sta_pack_rso32:
blk_sta_pack_rsotf32:
blk_stam_pack_rso32:
blk_stam_pack_rsotf32:

   cmp  r1, 0
   bne  blk_tf_to_rso_32
   b  lr

blk_sta_pack_tf32:
blk_sta_pack_lt32:
blk_stam_pack_tf32:
blk_stam_pack_lt32:

   cmp  r1, 0
   beq  blk_rso_to_tf_32
   b  lr

blk_sta_pack_rso1:
blk_sta_pack_tf1:
blk_sta_pack_lt1:
blk_stam_pack_rso1:
blk_stam_pack_tf1:
blk_stam_pack_lt1:

   cmp  r1, 0
   mov  r3, lr
   add  lr, pc, pcrel(1f)
   bne  blk_tf_to_rso_16 ; preserves r0, r3
1:
   mov  lr, r3
   v16bitplanes  H16(0++, 0) + r0, H16(0++, 0) + r0 REP 16
   b  lr

blk_sta_pack_rso4:
blk_stam_pack_rso4:

   cmp  r1, 0
   mov  r3, lr
   add  lr, pc, pcrel(1f)
   bne  blk_tf_to_rso_16 ; preserves r0, r3
1:
   mov  lr, r3
   v16odd  H8(0++, 32) + r0, H8(0++, 0) + r0, 0 REP 16
   v16even  H8(0++, 0) + r0, H8(0++, 0) + r0, 0 REP 16
   v16shl  V8(0, 32++) + r0, V8(0, 32++) + r0, 4 REP 8
   v16and  V8(0, 0++) + r0, V8(0, 0++) + r0, 0xf REP 8
   v16or  V8(0, 0++) + r0, V8(0, 0++) + r0, V8(0, 32++) + r0 REP 8
   b  lr

blk_sta_pack_tf4:
blk_sta_pack_lt4:
blk_stam_pack_tf4:
blk_stam_pack_lt4:

   cmp  r1, 0
   mov  r3, lr
   add  lr, pc, pcrel(1f)
   bne  blk_tf_to_rso_16 ; preserves r0, r3
1:
   mov  lr, r3
   v16mov  V8(0, 16++) + r0, V8(0, 2++) + r0 REP 2
   v16mov  V8(0, 32++) + r0, V8(0, 4++) + r0 REP 2
   v16mov  V8(0, 48++) + r0, V8(0, 6++) + r0 REP 2
   v16mov  V8(0, 24++) + r0, V8(0, 10++) + r0 REP 2
   v16mov  V8(0, 40++) + r0, V8(0, 12++) + r0 REP 2
   v16mov  V8(0, 56++) + r0, V8(0, 14++) + r0 REP 2
   mov  r1, 0x0f0f0f0f
   v32and  V32(0, 0++) + r0, V32(0, 0++) + r0, r1 REP 2
   v32and  V32(0, 8++) + r0, V32(0, 8++) + r0, r1 REP 2
   v32shl  V32(0, 1) + r0, V32(0, 1) + r0, 4
   v32shl  V32(0, 9) + r0, V32(0, 9) + r0, 4
   v32or  V32(0, 0) + r0, V32(0, 0) + r0, V32(0, 1) + r0
   v32or  V32(0, 8) + r0, V32(0, 8) + r0, V32(0, 9) + r0
   v32interh  V32(0, 1) + r0, V32(0, 0) + r0, V32(0, 8) + r0
   v32interl  V32(0, 0) + r0, V32(0, 0) + r0, V32(0, 8) + r0
   b  lr

blk_sta_pack_tf8:
blk_sta_pack_lt8:

   cmp  r1, 0
   mov  r3, lr
   add  lr, pc, pcrel(1f)
   bne  blk_tf_to_rso_16 ; preserves r0, r3
1:
   mov  lr, r3
   add  r1, r0, 12
1:
   v16mov  V8(0, 16) + r1, V8(0, 1) + r1
   v16mov  V8(0, 32) + r1, V8(0, 2) + r1
   v16mov  V8(0, 48) + r1, V8(0, 3) + r1
   addcmpbge  r1, -4, r0, 1b
   v32interh  V32(0, 3) + r0, V32(0, 8) + r0, V32(0, 12) + r0
   v32interh  V32(0, 2) + r0, V32(0, 0) + r0, V32(0, 4) + r0
   v32interl  V32(0, 1) + r0, V32(0, 8) + r0, V32(0, 12) + r0
   v32interl  V32(0, 0) + r0, V32(0, 0) + r0, V32(0, 4) + r0
   b  lr

blk_stam_pack_tf8:
blk_stam_pack_lt8:

   cmp  r1, 0
   mov  r3, lr
   add  lr, pc, pcrel(1f)
   bne  blk_tf_to_rso_16 ; preserves r0, r3
1:
   mov  lr, r3
   v16even  H8(0++, 32) + r0, V8(0, 0++) + r0, V8(0, 8++) + r0 REP 8
   v16odd  H8(8++, 32) + r0, V8(0, 0++) + r0, V8(0, 8++) + r0 REP 8
   v16mov  V8(0, 0++) + r0, V8(0, 32++) + r0 REP 4
   v16mov  V8(0, 8++) + r0, V8(0, 36++) + r0 REP 4
   v16mov  V8(0, 4++) + r0, V8(0, 40++) + r0 REP 4
   v16mov  V8(0, 12++) + r0, V8(0, 44++) + r0 REP 4
   b  lr

blk_sta_pack_tf16:
blk_sta_pack_lt16:

   cmp  r1, 0
   mov  r3, lr
   add  lr, pc, pcrel(1f)
   bne  blk_tf_to_rso_16 ; preserves r0, r3
1:
   mov  lr, r3
   .irep i, 0, 1, 2, 3, 4, 5, 6, 7
      v16mov  V16(0, 32 + (i * 2)) + r0, V16(0, 1 + (i * 2)) + r0
   .endr
   .irep i, 1, 0
      v32interl  V32(0, 8 + i) + r0, V32(0, 0 + (i * 8)) + r0, V32(0, 4 + (i * 8)) + r0
      v32interl  V32(0, 12 + i) + r0, V32(0, 2 + (i * 8)) + r0, V32(0, 6 + (i * 8)) + r0
      v32interh  V32(0, 10 + i) + r0, V32(0, 0 + (i * 8)) + r0, V32(0, 4 + (i * 8)) + r0
      v32interh  V32(0, 14 + i) + r0, V32(0, 2 + (i * 8)) + r0, V32(0, 6 + (i * 8)) + r0
   .endr
   v32interl  V32(0, 0++) + r0, V32(0, 8++) + r0, V32(0, 12++) + r0 REP 2
   v32interh  V32(0, 2++) + r0, V32(0, 8++) + r0, V32(0, 12++) + r0 REP 2
   v32interl  V32(0, 4++) + r0, V32(0, 10++) + r0, V32(0, 14++) + r0 REP 2
   v32interh  V32(0, 6++) + r0, V32(0, 10++) + r0, V32(0, 14++) + r0 REP 2
   b  lr

blk_stam_pack_tf16:
blk_stam_pack_lt16:

   cmp  r1, 0
   mov  r3, lr
   add  lr, pc, pcrel(1f)
   bne  blk_tf_to_rso_16 ; preserves r0, r3
1:
   mov  lr, r3
   v16even  H16(0++, 32) + r0, V16(0, 0++) + r0, V16(0, 8++) + r0 REP 8
   v16odd  H16(8++, 32) + r0, V16(0, 0++) + r0, V16(0, 8++) + r0 REP 8
   v16mov  V16(0, 0++) + r0, V16(0, 32++) + r0 REP 2
   v16mov  V16(0, 4++) + r0, V16(0, 34++) + r0 REP 2
   v16mov  V16(0, 8++) + r0, V16(0, 36++) + r0 REP 2
   v16mov  V16(0, 12++) + r0, V16(0, 38++) + r0 REP 2
   v16mov  V16(0, 2++) + r0, V16(0, 40++) + r0 REP 2
   v16mov  V16(0, 6++) + r0, V16(0, 42++) + r0 REP 2
   v16mov  V16(0, 10++) + r0, V16(0, 44++) + r0 REP 2
   v16mov  V16(0, 14++) + r0, V16(0, 46++) + r0 REP 2
   b  lr

blk_sta_pack_rso24:
blk_stam_pack_rso24:

   cmp  r1, 0
   mov  r3, lr
   add  lr, pc, pcrel(1f)
   bne  blk_tf_to_rso_32 ; preserves r0, r3
1:
   mov  lr, r3
   v16mov  H8(0++, 48) + r0, H8(0++, 16) + r0 REP 16
   v16interh  H8(0++, 16) + r0, H8(0++, 0) + r0, H8(0++, 32) + r0 REP 16
   v16interl  H8(0++, 0) + r0, H8(0++, 0) + r0, H8(0++, 32) + r0 REP 16
   add  r1, r0, 45
   add  r2, r0, 15
   add  r3, r0, 30
1:
   v16mov  V8(0, 2) + r1, V8(0, 1) + r3
   v16mov  V8(0, 0) + r1, V8(0, 0) + r3
   v16mov  V8(0, 1) + r1, V8(0, 48) + r2
   sub  r2, 1
   sub  r3, 2
   addcmpbge  r1, -3, r0, 1b
   b  lr

;/*****************************************************************************
; khrn_bf_blk_sta
;*****************************************************************************/

khrn_bf_blk_sta::

   ; r0: vr_data
   ; r1: data
   ; r2: stride
   ; r3: format

   bmask  r3, IMAGE_FORMAT_MEM_LAYOUT_BC + IMAGE_FORMAT_PIXEL_SIZE_BC
   switch  r3
1:
   ; IMAGE_FORMAT_1
   .case blk_sta_rso1 - 1b
   .case blk_sta_tf1 - 1b
   .case blk_sta_lt1 - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   ; IMAGE_FORMAT_4
   .case blk_sta_rso4 - 1b
   .case blk_sta_tf4 - 1b
   .case blk_sta_lt4 - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   ; IMAGE_FORMAT_8
   .case blk_sta_rso8 - 1b
   .case blk_sta_tf8 - 1b
   .case blk_sta_lt8 - 1b
   .case blk_sta_rsotf8 - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   ; IMAGE_FORMAT_16
   .case blk_sta_rso16 - 1b
   .case blk_sta_tf16 - 1b
   .case blk_sta_lt16 - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   ; IMAGE_FORMAT_24
   .case blk_sta_rso24 - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   ; IMAGE_FORMAT_32
   .case blk_sta_rso32 - 1b
   .case blk_sta_tf32 - 1b
   .case blk_sta_lt32 - 1b
   .case blk_sta_rsotf32 - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .case blk_sta_invalid - 1b
   .endswitch

blk_sta_invalid:

   bkpt

blk_sta_rso1:

   add  r3, pc, pcrel(ppu_index)
   v16ld  V16(0, 32) + r0, (r3)
   asr  r3, r2, 16
   v32mul.uu  -, V16(0, 32) + r0, r2 CLRA UACC
   v16mul  -, V16(0, 32) + r0, r3 UACCH
   v8indexwriteml  V8(0, 0) + r0, (r1)
   v8indexwriteml  V8(0, 16) + r0, (r1 + 1)
.ifdef WORKAROUND_HW1297
   v16mov  V16(0, 0) + r0, V16(0, 0) + r0
.endif
   b  lr

blk_sta_tf1:

   mov  r2, 32 ; tf same as lt but with stride of 32

blk_sta_lt1:

   add  r3, pc, pcrel(ppu_index)
   v16ld  V16(0, 32) + r0, (r3)
   sub  r2, 8
   shl  r2, 2
   v16shl  -, V16(0, 32) + r0, 2 CLRA UACC
   v16bitplanes  -, 0xff00 SETF
   v32mov  -, r2           IFNZ UACC
   v16indexwriteml  V16(0, 0) + r0, (r1)
.ifdef WORKAROUND_HW1297
   v16mov  V16(0, 0) + r0, V16(0, 0) + r0
.endif
   b  lr

blk_sta_rso4:

   v16bitplanes  -, 0x00ff           SETF
   v8st  H8(0++, 0) + r0, (r1 += r2) IFNZ REP 16
   b  lr

blk_sta_tf4:

   mov  r2, 32 ; tf same as lt but with stride of 32

blk_sta_lt4:

   v32st  V32(0, 0) + r0, (r1)
   addscale  r1, r2 << 3
   v32st  V32(0, 1) + r0, (r1)
   b  lr

blk_sta_rsotf8:

   mov  r2, 16

blk_sta_rso8:

   v8st  H8(0++, 0) + r0, (r1 += r2) REP 16
   b  lr

blk_sta_tf8:

   mov  r2, 32 ; tf same as lt but with stride of 32

blk_sta_lt8:

   mov  r3, 64
   v32st  V32(0, 0++) + r0, (r1 += r3) REP 2
   addscale  r1, r2 << 3
   v32st  V32(0, 2++) + r0, (r1 += r3) REP 2
   b  lr

blk_sta_rso16:

   v16st  H16(0++, 0) + r0, (r1 += r2) REP 16
   b  lr

blk_sta_tf16:

   mov  r2, 64 ; tf same as lt but with stride of 64

blk_sta_lt16:

   mov  r3, 64
   v32st  V32(0, 0++) + r0, (r1 += r3) REP 2
   addscale  r1, r2 << 2
   v32st  V32(0, 2++) + r0, (r1 += r3) REP 2
   addscale  r1, r2 << 2
   v32st  V32(0, 4++) + r0, (r1 += r3) REP 2
   addscale  r1, r2 << 2
   v32st  V32(0, 6++) + r0, (r1 += r3) REP 2
   b  lr

blk_sta_rso24:

   v8st  H8(0++, 0) + r0, (r1 += r2) REP 16
   v8st  H8(0++, 16) + r0, ((r1 + 16) += r2) REP 16
   v8st  H8(0++, 32) + r0, ((r1 + 32) += r2) REP 16
   b  lr

blk_sta_rsotf32:

   mov  r2, 64

blk_sta_rso32:

   v32st  H32(0++, 0) + r0, (r1 += r2) REP 16
   b  lr

blk_sta_tf32:

   mov  r2, 64
   v32st  V32(0, 0++) + r0, (r1 += r2) REP 16
   b  lr

blk_sta_lt32:

   mov  r3, 64
   v32st  V32(0, 0++) + r0, (r1 += r3) REP 4
   addscale  r1, r2 << 2
   v32st  V32(0, 4++) + r0, (r1 += r3) REP 4
   addscale  r1, r2 << 2
   v32st  V32(0, 8++) + r0, (r1 += r3) REP 4
   addscale  r1, r2 << 2
   v32st  V32(0, 12++) + r0, (r1 += r3) REP 4
   b  lr

;/*****************************************************************************
; khrn_bf_blk_stam
;*****************************************************************************/

khrn_bf_blk_stam::

   ; r0: vr_data
   ; r1: data
   ; r2: stride
   ; r3: mask_x
   ; r4: mask_y
   ; r5: format

   bmask  r5, IMAGE_FORMAT_MEM_LAYOUT_BC + IMAGE_FORMAT_PIXEL_SIZE_BC
   switch  r5
1:
   ; IMAGE_FORMAT_1
   .case blk_stam_rso1 - 1b
   .case blk_stam_tf1 - 1b
   .case blk_stam_lt1 - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   ; IMAGE_FORMAT_4
   .case blk_stam_rso4 - 1b
   .case blk_stam_tf4 - 1b
   .case blk_stam_lt4 - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   ; IMAGE_FORMAT_8
   .case blk_stam_rso8 - 1b
   .case blk_stam_tf8 - 1b
   .case blk_stam_lt8 - 1b
   .case blk_stam_rsotf8 - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   ; IMAGE_FORMAT_16
   .case blk_stam_rso16 - 1b
   .case blk_stam_tf16 - 1b
   .case blk_stam_lt16 - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   ; IMAGE_FORMAT_24
   .case blk_stam_rso24 - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   ; IMAGE_FORMAT_32
   .case blk_stam_rso32 - 1b
   .case blk_stam_tf32 - 1b
   .case blk_stam_lt32 - 1b
   .case blk_stam_rsotf32 - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .case blk_stam_invalid - 1b
   .endswitch

blk_stam_invalid:

   bkpt

blk_stam_rso1:

   add  r5, pc, pcrel(ppu_index)
   v16ld  V16(0, 32) + r0, (r5)
   asr  r5, r2, 16
   v32mul.uu  -, V16(0, 32) + r0, r2 CLRA UACC
   v16mul  -, V16(0, 32) + r0, r5 UACCH
   v16mov  V16(0, 32) + r0, r3
   v16and  -, V16(0, 32) + r0, 0x00ff SETF
   v16bitplanes  -, r4                IFNZ SETF
   v8lookupml  V8(0, 33) + r0, (r1)   IFNZ
   v16and  -, V16(0, 32) + r0, 0xff00   SETF
   v16bitplanes  -, r4                  IFNZ SETF
   v8lookupml  V8(0, 49) + r0, (r1 + 1) IFNZ
   v16bitplanes  -, r4                                      SETF
   v16and  V16(0, 34) + r0, V16(0, 0) + r0, r3              IFNZ
   v16bic  V16(0, 33) + r0, V16(0, 33) + r0, r3             IFNZ
   v16or  V16(0, 33) + r0, V16(0, 33) + r0, V16(0, 34) + r0 IFNZ
   v16and  -, V16(0, 32) + r0, 0x00ff                       IFNZ SETF
   v8indexwriteml  V8(0, 33) + r0, (r1)                     IFNZ
   v16and  -, V16(0, 32) + r0, 0xff00       SETF
   v16bitplanes  -, r4                      IFNZ SETF
   v8indexwriteml  V8(0, 49) + r0, (r1 + 1) IFNZ
.ifdef WORKAROUND_HW1297
   v16mov  V16(0, 33) + r0, V16(0, 33) + r0
.endif
   b  lr

blk_stam_tf1:

   mov  r2, 32 ; tf same as lt but with stride of 32

blk_stam_lt1:

   add  r5, pc, pcrel(ppu_index)
   v16ld  V16(0, 32) + r0, (r5)
   sub  r2, 8
   shl  r2, 2
   v16shl  -, V16(0, 32) + r0, 2 CLRA UACC
   v16bitplanes  -, 0xff00 SETF
   v32mov  -, r2           IFNZ UACC
   v16bitplanes  -, r4                                      SETF
   v16lookupml  V16(0, 32) + r0, (r1)                       IFNZ
   v16and  V16(0, 33) + r0, V16(0, 0) + r0, r3              IFNZ
   v16bic  V16(0, 32) + r0, V16(0, 32) + r0, r3             IFNZ
   v16or  V16(0, 32) + r0, V16(0, 32) + r0, V16(0, 33) + r0 IFNZ
   v16indexwriteml  V16(0, 32) + r0, (r1)                   IFNZ
.ifdef WORKAROUND_HW1297
   v16mov  V16(0, 32) + r0, V16(0, 32) + r0
.endif
   b  lr

blk_stam_rso4:

   v16bitplanes  H16(0, 32) + r0, r3
   v16bitplanes  -, r4 SETF
   shl  r4, 16
   or  r4, r0
   ; x mask stuff
   sub  r0, r3, 1
   msb  r5, r3
   eor  r0, r3
   add  r5, 1
   msb  r0
   ; can do x masking just with IFNZ?
   btest  r0, 0
   btest.eq  r5, 0
   beq  2f
   ; no
   add  r3, pc, pcrel(ppu_index)
   v16ld  H16(1, 32) + r4, (r3)
   asr  r3, r2, 16
   v32mul.uu  -, H16(1, 32) + r4, r2 CLRA UACC
   v16mul  -, H16(1, 32) + r4, r3 UACCH
   ; copy
   v16mov  H8(0++, 16) + r4, H8(0++, 0) + r4 REP 16
   ; fixup left
   btest  r0, 0
   beq  1f
   lsr  r0, 1
   add  r3, r4, r0
   add  r0, r1
   v8lookupml  H8(1, 32) + r4, (r0)                      IFNZ
   v16and  V8(0, 16) + r3, V8(0, 16) + r3, 0xf0          IFNZ
   v16and  H8(1, 32) + r4, H8(1, 32) + r4, 0x0f          IFNZ ; no need for hw-1297 workaround
   v16or  V8(0, 16) + r3, V8(0, 16) + r3, H8(1, 32) + r4 IFNZ
1:
   ; fixup right
   btest  r5, 0
   beq  1f
   lsr  r5, 1
   add  r0, r4, r5
   add  r5, r1
   v8lookupml  H8(1, 32) + r4, (r5)                      IFNZ
   v16and  V8(0, 16) + r0, V8(0, 16) + r0, 0x0f          IFNZ
   v16and  H8(1, 32) + r4, H8(1, 32) + r4, 0xf0          IFNZ ; no need for hw-1297 workaround
   v16or  V8(0, 16) + r0, V8(0, 16) + r0, H8(1, 32) + r4 IFNZ
1:
   cmp  r1, 0 ; ne
2:
   v16odd  -, H16(0, 32) + r4, 0  SETF
   v16even  -, H16(0, 32) + r4, 0 IFZ SETF
   ; y mask stuff
   lsr  r0, r4, 16
   sub  r3, r0, 1
   eor  r3, r0
   msb  r3
   addscale  r4, r3 << 6
   mul  r3, r2
   count  r0
   add.ne  r4, 16
   add  r1, r3
   ; store
   v8st  H8(0++, 0) + r4, (r1 += r2) IFNZ REP r0
   b  lr

blk_stam_tf4:

   mov  r2, 32 ; tf same as lt but with stride of 32

blk_stam_lt4:

   ; x mask
   v16bitplanes  V16(0, 8) + r0, r3
   v16interh  V16(0, 9) + r0, V16(0, 8) + r0, V16(0, 8) + r0
   v16interl  V16(0, 8) + r0, V16(0, 8) + r0, V16(0, 8) + r0
   v16interh  V16(0, 40++) + r0, V16(0, 8++) + r0, V16(0, 8++) + r0 REP 2
   v16interl  V16(0, 8++) + r0, V16(0, 8++) + r0, V16(0, 8++) + r0 REP 2
   v16bitplanes  V16(0, 8++) + r0, V16(0, 8++) + r0 REP 2
   v16bitplanes  V16(0, 40++) + r0, V16(0, 40++) + r0 REP 2
   v32interl  V32(0, 8) + r0, V32(0, 8) + r0, V32(0, 9) + r0 SETF
   ; y mask
   v16bitplanes  V16(0, 9) + r0, r4
   ; load, merge, and store first
   v16interl  -, V16(0, 9) + r0, V16(0, 9) + r0              IFNZ SETF
   v32ld  V32(0, 2) + r0, (r1)                               IFNZ
   v32and  V32(0, 3) + r0, V32(0, 0) + r0, V32(0, 8) + r0    IFNZ
   v32bic  V32(0, 2) + r0, V32(0, 2) + r0, V32(0, 8) + r0    IFNZ
   v32or  V32(0, 2) + r0, V32(0, 2) + r0, V32(0, 3) + r0     IFNZ
   v32st  V32(0, 2) + r0, (r1)                               IFNZ
   ; advance pointer
   addscale  r1, r2 << 3
   ; load, merge, and store second
   v32mov  -, V32(0, 8) + r0                              SETF
   v16interh  -, V16(0, 9) + r0, V16(0, 9) + r0           IFNZ SETF
   v32ld  V32(0, 2) + r0, (r1)                            IFNZ
   v32and  V32(0, 3) + r0, V32(0, 1) + r0, V32(0, 8) + r0 IFNZ
   v32bic  V32(0, 2) + r0, V32(0, 2) + r0, V32(0, 8) + r0 IFNZ
   v32or  V32(0, 2) + r0, V32(0, 2) + r0, V32(0, 3) + r0  IFNZ
   v32st  V32(0, 2) + r0, (r1)                            IFNZ
   b  lr

blk_stam_rsotf8:

   mov  r2, 16

blk_stam_rso8:

   v16bitplanes  -, r3 SETF
   sub  r3, r4, 1
   eor  r3, r4
   msb  r3
   addscale  r5, r0, r3 << 6
   mul  r3, r2
   count  r0, r4
   add  r1, r3
   v8st  H8(0++, 0) + r5, (r1 += r2) IFNZ REP r0
   b  lr

blk_stam_tf8:

   mov  r2, 32 ; tf same as lt but with stride of 32

blk_stam_lt8:

   sub  r2, 14 ; back to start
   ; create store mask
   v16bitplanes  V16(0, 32) + r0, r3
   v16bitplanes  V16(0, 33) + r0, r4
   v16bitplanes  -, 0x00ff                                   SETF
   v16and  V16(0, 32) + r0, V16(0, 32) + r0, 0x0f0f          IFNZ
   v16and  V16(0, 32) + r0, V16(0, 32) + r0, 0xf0f0          IFZ
   v16or  V16(0, 32) + r0, V16(8, 32) + r0, V16(0, 32) + r0  IFNZ
   v16or  V16(0, 32) + r0, V16(56, 32) + r0, V16(0, 32) + r0 IFZ
   v16add  V8(0, 49) + r0, V8(1, 49) + r0, 0
   v16even  V16(0, 33) + r0, V16(0, 33) + r0, V16(0, 33) + r0
   v16bitplanes  -, 0x0ff0                      SETF
   v16add  V16(0, 33) + r0, V16(60, 33) + r0, 0 IFNZ
   v16bitplanes  V16(0, 33) + r0, V16(0, 33) + r0
   v16and  V16(0, 32) + r0, V16(0, 32) + r0, V16(0, 33) + r0
   ; store
   mov  r3, r0
   add  r4, r0, 7
   add  r5, r0, 16
1:
   v16and  -, V16(0, 32) + r0, 0x1 SETF
   cmp  r3, r4
   v8st  V8(0, 0) + r3, (r1)       IFNZ
   add.ne  r1, 16
   v16lsr  V16(0, 32) + r0, V16(0, 32) + r0, 1
   add.eq  r1, r2 << 3
   addcmpbne  r3, 1, r5, 1b
   b  lr

blk_stam_rso16:

   v16bitplanes  -, r3 SETF
   sub  r3, r4, 1
   eor  r3, r4
   msb  r3
   addscale  r5, r0, r3 << 6
   mul  r3, r2
   count  r0, r4
   add  r1, r3
   v16st  H16(0++, 0) + r5, (r1 += r2) IFNZ REP r0
   b  lr

blk_stam_tf16:

   mov  r2, 64 ; tf same as lt but with stride of 64

blk_stam_lt16:

   sub  r2, 24 ; back to start
   ; create store mask
   v16bitplanes  V16(0, 32) + r0, r3
   v16bitplanes  V16(0, 33) + r0, r4
   v16bitplanes  -, 0x00ff                                   SETF
   v16and  V16(0, 32) + r0, V16(0, 32) + r0, 0x3333          IFNZ
   v16and  V16(0, 32) + r0, V16(0, 32) + r0, 0xcccc          IFZ
   v16or  V16(0, 32) + r0, V16(8, 32) + r0, V16(0, 32) + r0  IFNZ
   v16or  V16(0, 32) + r0, V16(56, 32) + r0, V16(0, 32) + r0 IFZ
   v16add  V8(0, 49) + r0, V8(1, 49) + r0, 0
   v16bitplanes  -, 0xaaaa                     SETF
   v16add  V16(0, 33) + r0, V16(1, 33) + r0, 0 IFNZ
   v16bitplanes  -, 0xcccc                      SETF
   v16add  V16(0, 33) + r0, V16(62, 33) + r0, 0 IFNZ
   v16bitplanes  V16(0, 33) + r0, V16(0, 33) + r0
   v16and  V16(0, 32) + r0, V16(0, 32) + r0, V16(0, 33) + r0
   ; store
   mov  r3, r0
   add  r4, r0, 16
1:
   bmask  r5, r3, 2
   v16and  -, V16(0, 32) + r0, 0x1 SETF
   cmp  r5, 3
   v16st  V16(0, 0) + r3, (r1)     IFNZ
   add.ne  r1, 32
   v16lsr  V16(0, 32) + r0, V16(0, 32) + r0, 1
   add.eq  r1, r2 << 2
   addcmpbne  r3, 1, r4, 1b
   b  lr

blk_stam_rso24:

   shl  r4, 16
   or  r0, r4
   ; x mask stuff
   sub  r4, r3, 1
   msb  r5, r3
   eor  r4, r3
   add  r5, 1
   msb  r4
   shl  r5, 16
   or  r3, r4, r5
   addscale  r3, r3 << 1
   mov  r5, -1
   bmask  r4, r3, 16
   cmp  r4, 16
   bmask.lt  r5, r4
   not  r5
   lsr  r4, r3, 16
   cmp  r4, 16
   bmask.lt  r5, r4
   v16bitplanes  -, r5 SETF
   mov  r5, -1
   bmask  r4, r3, 16
   sub  r4, 16
   max  r4, 0
   bmask  r5, r4
   not  r5
   lsr  r3, 16
   sub  r3, 16
   max  r3, 0
   cmp  r3, 32
   bmask.lt  r5, r3
   ; y mask stuff
   mov  r4, r0
   lsr  r0, 16
   sub  r3, r0, 1
   eor  r3, r0
   msb  r3
   addscale  r4, r3 << 6
   mul  r3, r2
   count  r0
   add  r1, r3
   ; stores
   lsr  r3, r5, 16
   v8st  H8(0++, 0) + r4, (r1 += r2) IFNZ REP r0 ; top 16 bits of r4 ignored
   v16bitplanes  -, r5                       SETF
   v8st  H8(0++, 16) + r4, ((r1 + 16) += r2) IFNZ REP r0
   v16bitplanes  -, r3                       SETF
   v8st  H8(0++, 32) + r4, ((r1 + 32) += r2) IFNZ REP r0
   b  lr

blk_stam_rsotf32:

   mov  r2, 64

blk_stam_rso32:

   v16bitplanes  -, r3 SETF
   sub  r3, r4, 1
   eor  r3, r4
   msb  r3
   addscale  r5, r0, r3 << 6
   mul  r3, r2
   count  r0, r4
   add  r1, r3
   v32st  H32(0++, 0) + r5, (r1 += r2) IFNZ REP r0
   b  lr

blk_stam_tf32:

   mov  r2, 64 ; tf same as lt but with stride of 64

blk_stam_lt32:

   shl  r3, 16
   or  r0, r3
1:
   mov  r3, 0
   .irep i, 0, 1, 2, 3
      btest  r4, i
      bitset.ne  r3, (i * 4)
   .endr
   lsr  r4, 4
   mul  r3, 0xf
   .irep i, 0, 1, 2, 3
      lsr  r5, r0, (16 + (i * 4))
      .if i != 3
         bmask  r5, 4
      .endif
      mul  r5, 0x1111
      and  r5, r3
      v16bitplanes  -, r5                    SETF
      v32st  V32(0, i) + r0, (r1 + (i * 64)) IFNZ ; top 16 bits of r0 ignored
   .endr
   addscale  r1, r2 << 2
   or  r3, r0, 15
   addcmpble  r0, 4, r3, 1b
   b  lr

;/*****************************************************************************
; khrn_bf_blk_rotate_x
;*****************************************************************************/

khrn_bf_blk_rotate_x::

   ; r0: vr_data
   ; r1: x

   add  r1, r0
   v32mov  H32(0++, 0) + r0, H32(0++, 0) + r1 REP 16
   b  lr

;/*****************************************************************************
; khrn_bf_blk_rotate_y
;*****************************************************************************/

khrn_bf_blk_rotate_y::

   ; r0: vr_data
   ; r1: y

   mov  r2, -1
   rsub  r3, r1, 16
   bmask  r2, r3
   v16bitplanes  -, r2 SETF
   addscale  r1, r0, r1 << 6
   add  r2, r0, 16
1:
   v32mov  -, V32(0, 0) + r1 CLRA SACC
   v32mov  V32(0, 0) + r0, V32(48, 0) + r1 IFZ
   v32mov  V32(0, 0) + r0, 0               IFNZ SACC
   add  r1, 1
   addcmpbne  r0, 1, r2, 1b
   b  lr

;/*****************************************************************************
; khrn_bf_blk_unpack
;*****************************************************************************/

khrn_bf_blk_unpack::

   ; r0: vr_data
   ; r1: format
   ; r2: conv

   lsr  r3, r1, IMAGE_FORMAT_PIXEL_SIZE_SHIFT
   bmask  r3, IMAGE_FORMAT_PIXEL_SIZE_BC
   switch  r3
1:
   .case blk_unpack_1 - 1b
   .case blk_unpack_4 - 1b
   .case blk_unpack_8 - 1b
   .case blk_unpack_16 - 1b
   .case blk_unpack_24 - 1b
   .case blk_unpack_32 - 1b
   .endswitch

blk_unpack_16:

   lsr  r3, r1, IMAGE_FORMAT_PIXEL_LAYOUT_SHIFT
   bmask  r3, IMAGE_FORMAT_PIXEL_LAYOUT_BC
   switch  r3
1:
   .case blk_unpack_invalid - 1b
   .case blk_unpack_4444 - 1b
   .case blk_unpack_5551 - 1b
   .case blk_unpack_565 - 1b
   .case blk_unpack_88 - 1b
   .endswitch

blk_unpack_invalid:

   bkpt

blk_unpack_1: ; a_1 or l_1

   v32shl  H32(0++, 0) + r0, H32(0++, 0) + r0, 31 REP 16
   v32asr  H32(0++, 0) + r0, H32(0++, 0) + r0, 31 REP 16
   btest  r1, IMAGE_FORMAT_A_BI
   cmp.ne  r2, IMAGE_CONV_VG
   bne  2f
   b  1f

blk_unpack_4: ; a_4 or l_4

   v16and  H16(0++, 0) + r0, H16(0++, 0) + r0, 0xf REP 16
   v16mul  H16(0++, 32) + r0, H16(0++, 0) + r0, 0x1111 REP 16
   btest  r1, IMAGE_FORMAT_A_BI
   cmp.ne  r2, IMAGE_CONV_VG
   bne  2f
   v16mov  H16(0++, 0) + r0, H16(0++, 32) + r0 REP 16
   b  1f

blk_unpack_8: ; a_8 or l_8

   v16mov  H8(0++, 16) + r0, H8(0++, 0) + r0 REP 16
   v16mov  H16(0++, 32) + r0, H16(0++, 0) + r0 REP 16
   btest  r1, IMAGE_FORMAT_A_BI
   cmp.ne  r2, IMAGE_CONV_VG
   bne  2f
1:
   mov  r2, 0x00ffffff
   btest  r1, IMAGE_FORMAT_L_BI
   not.ne  r2
   v32or  H32(0++, 0) + r0, H32(0++, 0) + r0, r2 REP 16
   b  lr
2:
   mov  r1, 0xff000000
   v32and  H32(0++, 0) + r0, H32(0++, 0) + r0, r1 REP 16
   b  lr

blk_unpack_4444: ; rgba_4444, argb_4444, bgra_4444, abgr_4444, rgbx_4444, xrgb_4444, bgrx_4444, or xbgr_4444

   v16bitplanes  H16(0++, 0) + r0, H16(0++, 0) + r0 REP 16
   v16mov  V16(0, 44++) + r0, V16(0, 0++) + r0 REP 4
   v16mov  V16(0, 40++) + r0, V16(0, 0++) + r0 REP 4
   v16mov  V16(0, 36++) + r0, V16(0, 4++) + r0 REP 4
   v16mov  V16(0, 32++) + r0, V16(0, 4++) + r0 REP 4
   v16mov  V16(0, 0++) + r0, V16(0, 12++) + r0 REP 4
   v16mov  V16(0, 4++) + r0, V16(0, 12++) + r0 REP 4
   v16mov  V16(0, 12++) + r0, V16(0, 8++) + r0 REP 4
   v16bitplanes  H16(0++, 0) + r0, H16(0++, 0) + r0 REP 16
   v16bitplanes  H16(0++, 32) + r0, H16(0++, 32) + r0 REP 16
   eor  r1, (IMAGE_FORMAT_XA | IMAGE_FORMAT_XRGBX) ; order flipped so rgba_4444 will be fastest
   b  blk_unpack_32

blk_unpack_5551: ; rgba_5551, argb_1555, bgra_5551, abgr_1555, rgbx_5551, xrgb_1555, bgrx_5551, or xbgr_1555

   btest  r1, IMAGE_FORMAT_XA_BI
   bne  1f
   v16ror  H16(0++, 0) + r0, H16(0++, 0) + r0, 15 REP 16
1:
   v16bitplanes  H16(0++, 0) + r0, H16(0++, 0) + r0 REP 16
   v16mov  V16(0, 40++) + r0, V16(0, 0) + r0 REP 8
   v16mov  V16(0, 36++) + r0, V16(0, 2++) + r0 REP 4
   v16mov  V16(0, 35) + r0, V16(0, 1) + r0
   v16mov  V16(0, 33++) + r0, V16(0, 4++) + r0 REP 2
   v16mov  V16(0, 32) + r0, V16(0, 3) + r0
   v16mov  V16(0, 0++) + r0, V16(0, 13++) + r0 REP 2
   v16mov  V16(0, 2) + r0, V16(0, 15) + r0
   v16mov  V16(0, 3++) + r0, V16(0, 11++) + r0 REP 2
   v16mov  V16(0, 11++) + r0, V16(0, 6++) + r0 REP 4
   v16mov  V16(0, 15) + r0, V16(0, 10) + r0
   v16mov  V16(0, 5++) + r0, V16(0, 0++) + r0 REP 2
   v16mov  V16(0, 7) + r0, V16(0, 2) + r0
   v16bitplanes  H16(0++, 0) + r0, H16(0++, 0) + r0 REP 16
   v16bitplanes  H16(0++, 32) + r0, H16(0++, 32) + r0 REP 16
   bitclear  r1, IMAGE_FORMAT_XA_BI ; handled above
   bitflip  r1, IMAGE_FORMAT_XRGBX_BI ; order flipped so rgba_5551 will be fastest
   b  blk_unpack_32

blk_unpack_565: ; rgb_565 or bgr_565

   v16bitplanes  H16(0++, 0) + r0, H16(0++, 0) + r0 REP 16
   v16mov  V16(0, 40++) + r0, 0xffff REP 8
   v16mov  V16(0, 36++) + r0, V16(0, 1++) + r0 REP 4
   v16mov  V16(0, 35) + r0, V16(0, 0) + r0
   v16mov  V16(0, 33++) + r0, V16(0, 3++) + r0 REP 2
   v16mov  V16(0, 32) + r0, V16(0, 2) + r0
   v16mov  V16(0, 0++) + r0, V16(0, 13++) + r0 REP 2
   v16mov  V16(0, 2) + r0, V16(0, 15) + r0
   v16mov  V16(0, 3++) + r0, V16(0, 11++) + r0 REP 2
   v16mov  V16(0, 12++) + r0, V16(0, 7++) + r0 REP 4
   v16mov  V16(0, 10++) + r0, V16(0, 5++) + r0 REP 2
   v16mov  V16(0, 8++) + r0, V16(0, 14++) + r0 REP 2
   v16mov  V16(0, 5++) + r0, V16(0, 0++) + r0 REP 2
   v16mov  V16(0, 7) + r0, V16(0, 2) + r0
   v16bitplanes  H16(0++, 0) + r0, H16(0++, 0) + r0 REP 16
   v16bitplanes  H16(0++, 32) + r0, H16(0++, 32) + r0 REP 16
   bitset  r1, IMAGE_FORMAT_A_BI ; handled above
   bitflip  r1, IMAGE_FORMAT_XRGBX_BI ; order flipped so rgb_565 will be fastest
   bitclear  r1, IMAGE_FORMAT_PRE_BI ; don't clamp to alpha -- it's 0xff
   b  blk_unpack_32

blk_unpack_88: ; la_88 or al_88

   btest  r1, IMAGE_FORMAT_XA_BI
   beq  2f
   cmp  r2, IMAGE_CONV_VG
   bne  1f ; todo: can this branch be merged with the next one?
   btest  r1, IMAGE_FORMAT_PRE_BI
   beq  1f
   v16min  H8(0++, 16) + r0, H8(0++, 16) + r0, H8(0++, 0) + r0 REP 16
1:
   v16mov  H8(0++, 48) + r0, H8(0++, 0) + r0 REP 16
   v16mov  H8(0++, 0) + r0, H8(0++, 16) + r0 REP 16
   v16mov  H8(0++, 32) + r0, H8(0++, 16) + r0 REP 16
   b  lr
2:
   cmp  r2, IMAGE_CONV_VG
   bne  1f ; todo: can this branch be merged with the next one?
   btest  r1, IMAGE_FORMAT_PRE_BI
   beq  1f
   v16min  H8(0++, 0) + r0, H8(0++, 0) + r0, H8(0++, 16) + r0 REP 16
1:
   v16mov  H8(0++, 48) + r0, H8(0++, 16) + r0 REP 16
   v16mov  H8(0++, 16) + r0, H8(0++, 0) + r0 REP 16
   v16mov  H8(0++, 32) + r0, H8(0++, 0) + r0 REP 16
   b  lr

blk_unpack_24: ; rgb_888 or bgr_888
blk_unpack_32: ; rgba_8888, argb_8888, bgra_8888, abgr_8888, rgbx_8888, xrgb_8888, bgrx_8888, or xbgr_8888

   btest  r1, IMAGE_FORMAT_XA_BI
   beq  1f
   v32ror  H32(0++, 0) + r0, H32(0++, 0) + r0, 8 REP 16
1:
   btest  r1, IMAGE_FORMAT_XRGBX_BI
   beq  1f
   v16eor  H8(0++, 0) + r0, H8(0++, 0) + r0, H8(0++, 32) + r0 REP 16
   v16eor  H8(0++, 32) + r0, H8(0++, 0) + r0, H8(0++, 32) + r0 REP 16
   v16eor  H8(0++, 0) + r0, H8(0++, 0) + r0, H8(0++, 32) + r0 REP 16
1:
   btest  r1, IMAGE_FORMAT_A_BI
   beq  2f
   cmp  r2, IMAGE_CONV_VG
   bne  1f ; todo: can this branch be merged with the next one?
   btest  r1, IMAGE_FORMAT_PRE_BI
   beq  1f
   v16min  H8(0++, 0) + r0, H8(0++, 0) + r0, H8(0++, 48) + r0 REP 16
   v16min  H8(0++, 16) + r0, H8(0++, 16) + r0, H8(0++, 48) + r0 REP 16
   v16min  H8(0++, 32) + r0, H8(0++, 32) + r0, H8(0++, 48) + r0 REP 16
1:
   b  lr
2:
   v16mov  H8(0++, 48) + r0, 0xff REP 16
   b  lr

;/*****************************************************************************
; khrn_bf_blk_pack
;*****************************************************************************/

; quantisation done using:
; 4-bit = ((8-bit * 0x0f) + 0x87) >> 8
; 5-bit = ((8-bit * 0x1f) + 0x8f) >> 8
; 6-bit = ((8-bit * 0x3f) + 0x9f) >> 8

khrn_bf_blk_pack::

   ; r0: vr_data
   ; r1: format
   ; r2: conv

   lsr  r3, r1, IMAGE_FORMAT_PIXEL_SIZE_SHIFT
   bmask  r3, IMAGE_FORMAT_PIXEL_SIZE_BC
   switch  r3
1:
   .case blk_pack_1 - 1b
   .case blk_pack_4 - 1b
   .case blk_pack_8 - 1b
   .case blk_pack_16 - 1b
   .case blk_pack_24 - 1b
   .case blk_pack_32 - 1b
   .endswitch

blk_pack_16:

   lsr  r3, r1, IMAGE_FORMAT_PIXEL_LAYOUT_SHIFT
   bmask  r3, IMAGE_FORMAT_PIXEL_LAYOUT_BC
   switch  r3
1:
   .case blk_pack_invalid - 1b
   .case blk_pack_4444 - 1b
   .case blk_pack_5551 - 1b
   .case blk_pack_565 - 1b
   .case blk_pack_88 - 1b
   .endswitch

blk_pack_invalid:

   bkpt

blk_pack_1: ; a_1 or l_1

   btest  r1, IMAGE_FORMAT_L_BI
   mov.eq  r1, 31
   mov.ne  r1, 7 ; l from r
1:
   v32lsr  H32(0++, 0) + r0, H32(0++, 0) + r0, r1 REP 16
   b  lr

blk_pack_8: ; a_8 or l_8

   btest  r1, IMAGE_FORMAT_L_BI
   mov  r1, 24
   beq  1b
   b  lr ; l from r (which is already in the right place)

blk_pack_4: ; a_4 or l_4

   btest  r1, IMAGE_FORMAT_L_BI
   add  r2, r0, 48
   mov.ne  r2, r0 ; l from r
   add  r3, r2, 16
1:
   v32mul.uu  -, V8(0, 0) + r2, 0xf00 CLRA UACC
   v16mov  -, 0x8700 UACC
   v16mov  V8(0, 0) + r0, 0 UACCH
   add  r0, 1
   addcmpbne  r2, 1, r3, 1b
   b  lr

blk_pack_4444: ; rgba_4444, argb_4444, bgra_4444, abgr_4444, rgbx_4444, xrgb_4444, bgrx_4444, or xbgr_4444

   add  r2, r0, 15
1:
   .irep i, 0, 16, 32, 48
      v32mul.uu  -, V8(0, i) + r2, 0xf000 CLRA UACC
      v16mov  -, 0x7000 UACC
      v16mov  V8(0, i) + r2, 0x8 UACCH
   .endr
   addcmpbge  r2, -1, r0, 1b
   eor  r1, (IMAGE_FORMAT_XA | IMAGE_FORMAT_XRGBX) ; order flipped so rgba_4444 will be fastest
   mov  r2, lr
   bl  blk_pack_32
   mov  lr, r2
   v16bitplanes  H16(0++, 0) + r0, H16(0++, 0) + r0 REP 16
   v16bitplanes  H16(0++, 32) + r0, H16(0++, 32) + r0 REP 16
   v16mov  V16(0, 8++) + r0, V16(0, 12++) + r0 REP 4
   v16mov  V16(0, 12++) + r0, V16(0, 4++) + r0 REP 4
   v16mov  V16(0, 4++) + r0, V16(0, 36++) + r0 REP 4
   v16mov  V16(0, 0++) + r0, V16(0, 44++) + r0 REP 4
   v16bitplanes  H16(0++, 0) + r0, H16(0++, 0) + r0 REP 16
   b  lr

blk_pack_5551: ; rgba_5551, argb_1555, bgra_5551, abgr_1555, rgbx_5551, xrgb_1555, bgrx_5551, or xbgr_1555

   add  r3, r0, 15
1:
   .irep i, 0, 16, 32
      v32mul.uu  -, V8(0, i) + r3, 0xf800 CLRA UACC
      v16mov  -, 0x7800 UACC
      v16mov  V8(0, i) + r3, 0x4 UACCH
   .endr
   addcmpbge  r3, -1, r0, 1b
   mov  r3, r1
   bitclear  r1, IMAGE_FORMAT_XA_BI ; handled below
   bitflip  r1, IMAGE_FORMAT_XRGBX_BI ; order flipped so rgba_5551 will be fastest
   mov  r4, lr
   bl  blk_pack_32
   mov  lr, r4
   v16bitplanes  H16(0++, 0) + r0, H16(0++, 0) + r0 REP 16
   v16bitplanes  H16(0++, 32) + r0, H16(0++, 32) + r0 REP 16
   v16mov  V16(0, 9++) + r0, V16(0, 14++) + r0 REP 2
   v16mov  V16(0, 8) + r0, V16(0, 13) + r0
   v16mov  V16(0, 14++) + r0, V16(0, 6++) + r0 REP 2
   v16mov  V16(0, 13) + r0, V16(0, 5) + r0
   v16mov  V16(0, 6++) + r0, V16(0, 11++) + r0 REP 2
   v16mov  V16(0, 11++) + r0, V16(0, 3++) + r0 REP 2
   v16mov  V16(0, 2++) + r0, V16(0, 36++) + r0 REP 4
   v16mov  V16(0, 1) + r0, V16(0, 35) + r0
   v16mov  V16(0, 0) + r0, V16(0, 47) + r0
   cmp  r2, IMAGE_CONV_VG
   bne  1f ; todo: can this branch be merged with the next one?
   btest  r1, IMAGE_FORMAT_PRE_BI
   beq  1f
   v16and  V16(0, 0++) + r0, V16(0, 0++) + r0, V16(0, 0) + r0 REP 16
1:
   v16bitplanes  H16(0++, 0) + r0, H16(0++, 0) + r0 REP 16
   btest  r3, IMAGE_FORMAT_XA_BI
   bne  1f
   v16ror  H16(0++, 0) + r0, H16(0++, 0) + r0, 1 REP 16
1:
   b  lr

blk_pack_565: ; rgb_565 or bgr_565

   add  r2, r0, 15
1:
   .irep i, 0, 32
      v32mul.uu  -, V8(0, i) + r2, 0xf800 CLRA UACC
      v16mov  -, 0x7800 UACC
      v16mov  V8(0, i) + r2, 0x4 UACCH
   .endr
   v32mul.uu  -, V8(0, 16) + r2, 0xfc00 CLRA UACC
   v16mov  -, 0x7c00 UACC
   v16mov  V8(0, 16) + r2, 0x2 UACCH
   addcmpbge  r2, -1, r0, 1b
   bitflip  r1, IMAGE_FORMAT_XRGBX_BI ; order flipped so rgb_565 will be fastest
   mov  r2, lr
   bl  blk_pack_32
   mov  lr, r2
   v16bitplanes  H16(0++, 0) + r0, H16(0++, 0) + r0 REP 16
   v16bitplanes  H16(0++, 32) + r0, H16(0++, 32) + r0 REP 16
   v16mov  V16(0, 8++) + r0, V16(0, 13++) + r0 REP 2
   v16mov  V16(0, 13++) + r0, V16(0, 5++) + r0 REP 2
   v16mov  V16(0, 5++) + r0, V16(0, 10++) + r0 REP 2
   v16mov  V16(0, 10) + r0, V16(0, 15) + r0
   v16mov  V16(0, 15) + r0, V16(0, 7) + r0
   v16mov  V16(0, 7) + r0, V16(0, 12) + r0
   v16mov  V16(0, 11++) + r0, V16(0, 3++) + r0 REP 2
   v16mov  V16(0, 1++) + r0, V16(0, 36++) + r0 REP 4
   v16mov  V16(0, 0) + r0, V16(0, 35) + r0
   v16bitplanes  H16(0++, 0) + r0, H16(0++, 0) + r0 REP 16
   b  lr

blk_pack_88: ; la_88 or al_88

   btest  r1, IMAGE_FORMAT_XA_BI
   beq  1f
   v32ror  H32(0++, 0) + r0, H32(0++, 0) + r0, 24 REP 16
   b  lr
1:
   v16mov  H8(0++, 16) + r0, H8(0++, 48) + r0 REP 16
   b  lr

; preserves r0, r1, r2, r3
blk_pack_24: ; rgb_888 or bgr_888
blk_pack_32: ; rgba_8888, argb_8888, bgra_8888, abgr_8888, rgbx_8888, xrgb_8888, bgrx_8888, or xbgr_8888

   btest  r1, IMAGE_FORMAT_XRGBX_BI
   beq  1f
   v16eor  H8(0++, 0) + r0, H8(0++, 0) + r0, H8(0++, 32) + r0 REP 16
   v16eor  H8(0++, 32) + r0, H8(0++, 0) + r0, H8(0++, 32) + r0 REP 16
   v16eor  H8(0++, 0) + r0, H8(0++, 0) + r0, H8(0++, 32) + r0 REP 16
1:
   btest  r1, IMAGE_FORMAT_XA_BI
   beq  1f
   v32ror  H32(0++, 0) + r0, H32(0++, 0) + r0, 24 REP 16
1:
   b  lr

;/*****************************************************************************
; khrn_bf_blk_pre
;*****************************************************************************/

khrn_bf_blk_pre::

   ; r0: vr_data

   add  r1, r0, 16
1:
   .irep i, 0, 16, 32
      v16mul  -, V8(0, i) + r0, V8(0, 48) + r0 CLRA UACC
      v16mulm  -, V8(0, i) + r0, V8(0, 48) + r0 UACC
      v16mov  -, 0x80 UACC
      v32getacc  V8(0, i) + r0, 8
   .endr
   addcmpbne  r0, 1, r1, 1b
   b  lr

;/*****************************************************************************
; khrn_bf_blk_unpre
;*****************************************************************************/

khrn_bf_blk_unpre::

   ; r0: vr_data
   ; r1: vr_temp

.ifndef __VIDEOCORE4__ ; no vector fp at all on 2708
   version  r2
   btest  r2, 16
   bne  2f
.endif
   add  r2, pc, pcrel(COLOR_RECIP)
   mov  r3, 0x8000
   add  r4, r0, 16
1:
   v16mov  -, V8(0, 48) + r0 CLRA UACC
   v32lookupml  H32(0, 0) + r1, (r2) ; no need for hw-1297 workaround
   .irep i, 0, 1, 2
      v32mul.uu  -, V8(0, i * 16) + r0, H16(0, 0) + r1 CLRA UACC
      v16mul  -, V8(0, i * 16) + r0, H16(0, 32) + r1 UACCH
      v32mov  H32(1 + i, 0) + r1, r3 UACC
   .endr
   .irep i, 0, 1, 2
      v16min  V8(0, i * 16) + r0, H16(1 + i, 32) + r1, 0xff
   .endr
   addcmpbne  r0, 1, r4, 1b
   b  lr
.ifndef __VIDEOCORE4__ ; no vector fp at all on 2708
2:
   mov  r2, OO_255
   mov  r3, 0x8000
   add  r4, r0, 16
1:
   vinttofloat  H32(0, 0) + r1, V8(0, 48) + r0, 0
   vfmul  H32(0, 0) + r1, H32(0, 0) + r1, r2
   v16mov  -, V8(0, 48) + r0                        SETF
   vfrecip  H32(0, 0) + r1, H32(0, 0) + r1          IFNZ
   v32mov  H32(0, 0) + r1, 0                        IFZ
   vfloattointn  H32(0, 0) + r1, H32(0, 0) + r1, 16 IFNZ
   .irep i, 0, 1, 2
      v32mul.uu  -, V8(0, i * 16) + r0, H16(0, 0) + r1 CLRA UACC
      v16mul  -, V8(0, i * 16) + r0, H16(0, 32) + r1 UACCH
      v32mov  H32(1 + i, 0) + r1, r3 UACC
   .endr
   .irep i, 0, 1, 2
      v16min  V8(0, i * 16) + r0, H16(1 + i, 32) + r1, 0xff
   .endr
   addcmpbne  r0, 1, r4, 1b
   b  lr
.endif

;/*****************************************************************************
; khrn_bf_blk_clamp_to_a
;*****************************************************************************/

khrn_bf_blk_clamp_to_a::

   ; r0: vr_data

   v16min  H8(0++, 0) + r0, H8(0++, 0) + r0, H8(0++, 48) + r0 REP 16
   v16min  H8(0++, 16) + r0, H8(0++, 16) + r0, H8(0++, 48) + r0 REP 16
   v16min  H8(0++, 32) + r0, H8(0++, 32) + r0, H8(0++, 48) + r0 REP 16
   b  lr

;/*****************************************************************************
; khrn_bf_blk_s_to_lin/khrn_bf_blk_lin_to_s
;*****************************************************************************/

khrn_bf_blk_s_to_lin::

   ; r0: vr_data

   add  r1, pc, pcrel(COLOR_S_TO_LIN)
   b  1f

khrn_bf_blk_lin_to_s::

   ; r0: vr_data

   add  r1, pc, pcrel(COLOR_LIN_TO_S)
1:
   add  r2, r0, 16
1:
   .irep i, 0, 16, 32
      v16mov  -, V8(0, i) + r0 CLRA UACC
      v8lookupml  V8(0, i) + r0, (r1)
.ifdef WORKAROUND_HW1297
      v16mov  V8(0, i) + r0, V8(0, i) + r0
.endif
   .endr
   addcmpbne  r0, 1, r2, 1b
   b  lr

;/*****************************************************************************
; khrn_bf_blk_to_la_lin
;*****************************************************************************/

khrn_bf_blk_to_la_lin::

   ; r0: vr_data

   add  r1, r0, 16
1:
   v32mul.uu  -, V8(0, 0) + r0, 13933 CLRA UACC
   v32mul.uu  -, V8(0, 16) + r0, 46871 UACC
   v32mul.uu  -, V8(0, 32) + r0, 4732 UACC
   v16mov  -, 0x8000 UACC
   v16mov  V8(0, 0) + r0, 0 UACCH
   addcmpbne  r0, 1, r1, 1b
   b  lr

;/*****************************************************************************
; khrn_bf_blk_to_la_s
;*****************************************************************************/

khrn_bf_blk_to_la_s::

   ; r0: vr_data
   ; r1: vr_temp

   add  r2, pc, pcrel(COLOR_S_TO_LIN16)
   add  r3, pc, pcrel(COLOR_LIN16_TO_S)
   add  r4, r0, 16
1:
   .irep i, 0, 1, 2
      v16mov  -, V8(0, i * 16) + r0 CLRA UACC
      v16lookupml  H16(i, 0) + r1, (r2)
.ifdef WORKAROUND_HW1297
      v16mov  H16(i, 0) + r1, H16(i, 0) + r1
.endif
   .endr
   v32mul.uu  -, H16(0, 0) + r1, 13933 CLRA UACC
   v32mul.uu  -, H16(1, 0) + r1, 46871 UACC
   v32mul.uu  -, H16(2, 0) + r1, 4732 UACC
   v16mov  -, 0x8000 UACC
   v32mov  H32(0, 0) + r1, 0x8000 SACCH
   v32sub  H32(0, 0) + r1, H32(0, 0) + r1, 0x8000
   ; 0: l
   v32msb  H32(1, 0) + r1, H32(0, 0) + r1, 0
   v32max  H32(1, 0) + r1, H32(1, 0) + r1, 8
   v32min  H32(1, 0) + r1, H32(1, 0) + r1, 12
   ; 1: msb = clampi(_msb(l), 8, 12)
   v32sub  H32(2, 0) + r1, H32(1, 0) + r1, 5
   v32mov  H32(3, 0) + r1, 1
   v32shl  H32(2, 0) + r1, H32(3, 0) + r1, H32(2, 0) + r1
   v32add  H32(0, 0) + r1, H32(0, 0) + r1, H32(2, 0) + r1
   ; 0: l + (1 << (msb - 5))
   v32sub  H32(2, 0) + r1, H32(1, 0) + r1, 4
   v32lsr  H32(0, 0) + r1, H32(0, 0) + r1, H32(2, 0) + r1
   ; 0: (l + (1 << (msb - 5))) >> (msb - 4)
   v32sub  H32(1, 0) + r1, H32(1, 0) + r1, 8
   v32shl  H32(1, 0) + r1, H32(1, 0) + r1, 4
   v32add  H32(0, 0) + r1, H32(0, 0) + r1, H32(1, 0) + r1
   ; 0: ((msb - 8) << 4) + ((l + (1 << (msb - 5))) >> (msb - 4))
   v32min  -, H32(0, 0) + r1, 319 CLRA UACC
   v8lookupml  V8(0, 0) + r0, (r3)
.ifdef WORKAROUND_HW1297
   v16mov  V8(0, 0) + r0, V8(0, 0) + r0
.endif
   addcmpbne  r0, 1, r4, 1b
   b  lr

;/*****************************************************************************
; khrn_bf_blk_color_matrix
;*****************************************************************************/

khrn_bf_blk_color_matrix::

   ; r0: vr_data
   ; r1: vr_temp
   ; r2: vr_matrix
   ; V16(0, i) + vr_matrix: matrix[i]
   ; V32(1, i) + vr_matrix: matrix[16 + i]
   ; H16(2, 0) + vr_matrix: -shift

   add  r3, r0, 16
1:
   .irep i, 0, 1, 2, 3
      v32mul.su  -, V16(0, 0 + i) + r2, V8(0, 0) + r0 CLRA SACC
      v32mul.su  -, V16(0, 4 + i) + r2, V8(0, 16) + r0 SACC
      v32mul.su  -, V16(0, 8 + i) + r2, V8(0, 32) + r0 SACC
      v32mul.su  -, V16(0, 12 + i) + r2, V8(0, 48) + r0 SACC
      v32add  H32(i, 0) + r1, V32(1, i) + r2, 0 SACC
   .endr
   .irep i, 0, 1, 2, 3
      v32signasls  V8(0, i * 16) + r0, H32(i, 0) + r1, H16(2, 0) + r2
   .endr
   addcmpbne  r0, 1, r3, 1b
   b  lr

;/*****************************************************************************
; khrn_bf_blk_sconv_8_8
;*****************************************************************************/

khrn_bf_blk_sconv_8_8::

   ; r0: vr_dst
   ; r1: rows_n
   ; r2: vr_src
   ; r3: vr_temp
   ; r4: vr_kernel
   ; V16(0, 0) + vr_kernel: kernel[32]
   ; V16(0, 1) + vr_kernel: -shift
   ; V32(0, 2) + vr_kernel: bias
   ; V16(1, i) + vr_kernel: kernel[i]
   ; V16(1, 32 + i) + vr_kernel: kernel[16 + i]
   ; r5: kernel_size

   eor  r0, r5
   eor  r5, r0
   eor  r0, r5
   sub  r5, (1 << 6)
   addscale  r1, r5, r1 << 6
   addcmpble  r0, 0, 16, 6f
   sub  r0, 16
   addcmpble  r0, 0, 16, 4f
   b  2f
1:
   v32signasls  H8(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
2:
   v32mov  -, V32(0, 2) + r4 CLRA SACC
   v32mul.su  -, V16(0, 0) + r4, H8(32, 0) + r2 SACC
   v32mul.su  -, V16(1, 32++) + r4, H8(16++, 0) + r2 SACC REP 16
   v32mul.su  -, V16(1, 0++) + r4, H8(0++, 0) + r2 SACC REP 8
   v32mul.su  -, V16(1, 8++) + r4, H8(8++, 0) + r2 SACC REP 4
   v32mul.su  -, V16(1, 12++) + r4, H8(12++, 0) + r2 SACC REP 2
   v32mul.su  -, V16(1, 14) + r4, H8(14, 0) + r2 SACC
   v32mul.su  H32(0, 0) + r3, V16(1, 15) + r4, H8(15, 0) + r2 SACC
   add  r2, (1 << 6)
   add  r5, (1 << 6)
   addcmpbne  r5, 0, r1, 1b
   v32signasls  H8(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
   b  lr
3:
   v32signasls  H8(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
4:
   v32mov  -, V32(0, 2) + r4 CLRA SACC
   v32mul.su  -, V16(1, 32++) + r4, H8(16++, 0) + r2 SACC REP r0
   v32mul.su  -, V16(1, 0++) + r4, H8(0++, 0) + r2 SACC REP 8
   v32mul.su  -, V16(1, 8++) + r4, H8(8++, 0) + r2 SACC REP 4
   v32mul.su  -, V16(1, 12++) + r4, H8(12++, 0) + r2 SACC REP 2
   v32mul.su  -, V16(1, 14) + r4, H8(14, 0) + r2 SACC
   v32mul.su  H32(0, 0) + r3, V16(1, 15) + r4, H8(15, 0) + r2 SACC
   add  r2, (1 << 6)
   add  r5, (1 << 6)
   addcmpbne  r5, 0, r1, 3b
   v32signasls  H8(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
   b  lr
5:
   v32mov  H32(0, 0) + r3, 0 SACC
   v32signasls  H8(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
6:
   v32mov  -, V32(0, 2) + r4 CLRA SACC
   v32mul.su  -, V16(1, 0++) + r4, H8(0++, 0) + r2 SACC REP r0
   add  r2, (1 << 6)
   add  r5, (1 << 6)
   addcmpbne  r5, 0, r1, 5b
   v32mov  H32(0, 0) + r3, 0 SACC
   v32signasls  H8(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
   b  lr

;/*****************************************************************************
; khrn_bf_blk_sconv_8_16
;*****************************************************************************/

khrn_bf_blk_sconv_8_16::

   ; r0: vr_dst
   ; r1: rows_n
   ; r2: vr_src
   ; r3: vr_temp
   ; r4: vr_kernel
   ; V16(0, 0) + vr_kernel: kernel[32]
   ; V16(0, 1) + vr_kernel: -shift
   ; V32(0, 2) + vr_kernel: bias
   ; V16(1, i) + vr_kernel: kernel[i]
   ; V16(1, 32 + i) + vr_kernel: kernel[16 + i]
   ; r5: kernel_size

   eor  r0, r5
   eor  r5, r0
   eor  r0, r5
   sub  r5, (1 << 6)
   addscale  r1, r5, r1 << 6
   addcmpble  r0, 0, 16, 6f
   sub  r0, 16
   addcmpble  r0, 0, 16, 4f
   b  2f
1:
   v32signasls  H16(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
2:
   v32mov  -, V32(0, 2) + r4 CLRA SACC
   v32mul.su  -, V16(0, 0) + r4, H8(32, 0) + r2 SACC
   v32mul.su  -, V16(1, 32++) + r4, H8(16++, 0) + r2 SACC REP 16
   v32mul.su  -, V16(1, 0++) + r4, H8(0++, 0) + r2 SACC REP 8
   v32mul.su  -, V16(1, 8++) + r4, H8(8++, 0) + r2 SACC REP 4
   v32mul.su  -, V16(1, 12++) + r4, H8(12++, 0) + r2 SACC REP 2
   v32mul.su  -, V16(1, 14) + r4, H8(14, 0) + r2 SACC
   v32mul.su  H32(0, 0) + r3, V16(1, 15) + r4, H8(15, 0) + r2 SACC
   add  r2, (1 << 6)
   add  r5, (1 << 6)
   addcmpbne  r5, 0, r1, 1b
   v32signasls  H16(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
   b  lr
3:
   v32signasls  H16(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
4:
   v32mov  -, V32(0, 2) + r4 CLRA SACC
   v32mul.su  -, V16(1, 32++) + r4, H8(16++, 0) + r2 SACC REP r0
   v32mul.su  -, V16(1, 0++) + r4, H8(0++, 0) + r2 SACC REP 8
   v32mul.su  -, V16(1, 8++) + r4, H8(8++, 0) + r2 SACC REP 4
   v32mul.su  -, V16(1, 12++) + r4, H8(12++, 0) + r2 SACC REP 2
   v32mul.su  -, V16(1, 14) + r4, H8(14, 0) + r2 SACC
   v32mul.su  H32(0, 0) + r3, V16(1, 15) + r4, H8(15, 0) + r2 SACC
   add  r2, (1 << 6)
   add  r5, (1 << 6)
   addcmpbne  r5, 0, r1, 3b
   v32signasls  H16(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
   b  lr
5:
   v32mov  H32(0, 0) + r3, 0 SACC
   v32signasls  H16(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
6:
   v32mov  -, V32(0, 2) + r4 CLRA SACC
   v32mul.su  -, V16(1, 0++) + r4, H8(0++, 0) + r2 SACC REP r0
   add  r2, (1 << 6)
   add  r5, (1 << 6)
   addcmpbne  r5, 0, r1, 5b
   v32mov  H32(0, 0) + r3, 0 SACC
   v32signasls  H16(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
   b  lr

;/*****************************************************************************
; khrn_bf_blk_sconv_8_32
;*****************************************************************************/

khrn_bf_blk_sconv_8_32::

   ; r0: vr_dst
   ; r1: rows_n
   ; r2: vr_src
   ; r3: vr_kernel
   ; V16(0, 0) + vr_kernel: kernel[32]
   ; V16(1, i) + vr_kernel: kernel[i]
   ; V16(1, 32 + i) + vr_kernel: kernel[16 + i]
   ; r4: kernel_size

   mov  r5, r0
   mov  r0, r4
   addscale  r1, r5, r1 << 6
   addcmpble  r0, 0, 16, 3f
   sub  r0, 16
   addcmpble  r0, 0, 16, 2f
1:
   v32mul.su  -, V16(0, 0) + r3, H8(32, 0) + r2 CLRA SACC
   v32mul.su  -, V16(1, 32++) + r3, H8(16++, 0) + r2 SACC REP 16
   v32mul.su  -, V16(1, 0++) + r3, H8(0++, 0) + r2 SACC REP 8
   v32mul.su  -, V16(1, 8++) + r3, H8(8++, 0) + r2 SACC REP 4
   v32mul.su  -, V16(1, 12++) + r3, H8(12++, 0) + r2 SACC REP 2
   v32mul.su  -, V16(1, 14) + r3, H8(14, 0) + r2 SACC
   v32mul.su  H32(0, 0) + r5, V16(1, 15) + r3, H8(15, 0) + r2 SACC
   add  r2, (1 << 6)
   add  r5, (1 << 6)
   addcmpbne  r5, 0, r1, 1b
   b  lr
2:
   v32mul.su  -, V16(1, 32++) + r3, H8(16++, 0) + r2 CLRA SACC REP r0
   v32mul.su  -, V16(1, 0++) + r3, H8(0++, 0) + r2 SACC REP 8
   v32mul.su  -, V16(1, 8++) + r3, H8(8++, 0) + r2 SACC REP 4
   v32mul.su  -, V16(1, 12++) + r3, H8(12++, 0) + r2 SACC REP 2
   v32mul.su  -, V16(1, 14) + r3, H8(14, 0) + r2 SACC
   v32mul.su  H32(0, 0) + r5, V16(1, 15) + r3, H8(15, 0) + r2 SACC
   add  r2, (1 << 6)
   add  r5, (1 << 6)
   addcmpbne  r5, 0, r1, 2b
   b  lr
3:
   sub  r1, (1 << 6)
   sub  r5, (1 << 6)
   b  5f
4:
   v32mov  H32(0, 0) + r5, 0 SACC
5:
   v32mul.su  -, V16(1, 0++) + r3, H8(0++, 0) + r2 CLRA SACC REP r0
   add  r2, (1 << 6)
   add  r5, (1 << 6)
   addcmpbne  r5, 0, r1, 4b
   v32mov  H32(0, 0) + r5, 0 SACC
   b  lr

;/*****************************************************************************
; khrn_bf_blk_sconv_16_8
;*****************************************************************************/

khrn_bf_blk_sconv_16_8::

   ; r0: vr_dst
   ; r1: rows_n
   ; r2: vr_src
   ; r3: vr_temp
   ; r4: vr_kernel
   ; V16(0, 0) + vr_kernel: kernel[32]
   ; V16(0, 1) + vr_kernel: _max(-shift, 0)
   ; V32(0, 2) + vr_kernel: bias >> 16
   ; V16(0, 3) + vr_kernel: _max(shift, 0)
   ; V16(0, 4) + vr_kernel: bias & 0xffff
   ; V16(1, i) + vr_kernel: kernel[i]
   ; V16(1, 32 + i) + vr_kernel: kernel[16 + i]
   ; r5: kernel_size

   eor  r0, r5
   eor  r5, r0
   eor  r0, r5
   sub  r5, (1 << 6)
   addscale  r1, r5, r1 << 6
   addcmpble  r0, 0, 16, 6f
   sub  r0, 16
   addcmpble  r0, 0, 16, 4f
   b  2f
1:
   v32getaccs32  H32(0, 0) + r3, V16(0, 3) + r4
   v32signasls  H8(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
2:
   v32mov  -, V32(0, 2) + r4 CLRA SACCH
   v16mov  -, V16(0, 4) + r4 UACC
   v32mul.ss  -, V16(0, 0) + r4, H16(32, 0) + r2 SACC
   v32mul.ss  -, V16(1, 32++) + r4, H16(16++, 0) + r2 SACC REP 16
   v32mul.ss  -, V16(1, 0++) + r4, H16(0++, 0) + r2 SACC REP 16
   add  r2, (1 << 6)
   add  r5, (1 << 6)
   addcmpbne  r5, 0, r1, 1b
   v32getaccs32  H32(0, 0) + r3, V16(0, 3) + r4
   v32signasls  H8(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
   b  lr
3:
   v32getaccs32  H32(0, 0) + r3, V16(0, 3) + r4
   v32signasls  H8(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
4:
   v32mov  -, V32(0, 2) + r4 CLRA SACCH
   v16mov  -, V16(0, 4) + r4 UACC
   v32mul.ss  -, V16(1, 32++) + r4, H16(16++, 0) + r2 SACC REP r0
   v32mul.ss  -, V16(1, 0++) + r4, H16(0++, 0) + r2 SACC REP 16
   add  r2, (1 << 6)
   add  r5, (1 << 6)
   addcmpbne  r5, 0, r1, 3b
   v32getaccs32  H32(0, 0) + r3, V16(0, 3) + r4
   v32signasls  H8(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
   b  lr
5:
   v32getaccs32  H32(0, 0) + r3, V16(0, 3) + r4
   v32signasls  H8(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
6:
   v32mov  -, V32(0, 2) + r4 CLRA SACCH
   v16mov  -, V16(0, 4) + r4 UACC
   v32mul.ss  -, V16(1, 0++) + r4, H16(0++, 0) + r2 SACC REP r0
   add  r2, (1 << 6)
   add  r5, (1 << 6)
   addcmpbne  r5, 0, r1, 5b
   v32getaccs32  H32(0, 0) + r3, V16(0, 3) + r4
   v32signasls  H8(0, 0) + r5, H32(0, 0) + r3, V16(0, 1) + r4
   b  lr

;/*****************************************************************************
; khrn_bf_blk_conv
;*****************************************************************************/

; explicitly uses H32(63, 0) as a temp row, which is a bit ugly
khrn_bf_blk_conv::

   ; r0: vr_dst
   ; r1: vr_src
   ; r2: vr_kernel
   ; H16(y, x) + vr_kernel: kernel[x, y]
   ; H16(0, 32) + vr_kernel: -shift
   ; V32(0, 15) + vr_kernel: bias
   ; r3: kernel_width
   ; r4: kernel_height

   sub  r5, r0, (1 << 6)
   shl  r0, r3, 16
   or  r0, r4
   add  r4, r5, (16 << 6)
   b  2f
1:
   v32mov  H32(63, 0), 0 SACC
   v32signasls  H8(0, 0) + r5, H32(63, 0), H16(0, 32) + r2
2:
   v32mov  -, V32(0, 15) + r2 CLRA SACC
   add  r3, r1
3:
   v32mul.su  -, H16(0++, 0) + r2, H8(0++, 0) + r1 SACC REP r0
   add  r2, 1
   addcmpbne  r1, 1, r3, 3b
   lsr  r3, r0, 16
   sub  r1, r3
   sub  r2, r3
   add  r1, (1 << 6)
   add  r5, (1 << 6)
   addcmpbne  r5, 0, r4, 1b
   v32mov  H32(63, 0), 0 SACC
   v32signasls  H8(0, 0) + r5, H32(63, 0), H16(0, 32) + r2
   b  lr

;/*****************************************************************************
; khrn_bf_blk_st_ppulm
;*****************************************************************************/

khrn_bf_blk_st_ppulm::

   ; r0: vr_data
   ; r1: offset

   .irep i, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
      v32memwrite  -, H32(i, 0) + r0, r1
      add  r1, 1
   .endr
   b  lr

;/*****************************************************************************
; khrn_bf_blk_ld_ppulm
;*****************************************************************************/

khrn_bf_blk_ld_ppulm::

   ; r0: vr_data
   ; r1: offset

   .irep i, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
      v32memread  H32(i, 0) + r0, -, r1
      add  r1, 1
   .endr
   b  lr

;/*****************************************************************************
; khrn_bf_um_stall
;*****************************************************************************/

.ifndef __VIDEOCORE4__ ; no uniforms memory attached to vpu on 2708

khrn_bf_um_stall::

   mov  r0, GR_UNIFORM_BASE
   ld  r0, (r0)
   mov  r0, r0
   b  lr

.endif


;/*****************************************************************************
; khrn_bf_blk_subsample_tf_32
; r0: VRF offset of input block (16 rows, i.e. 16 microtiles)
; r1: VRF offset of output block (row row blank blank row row)
;  48-53 is temporary workspace

; r2: r1+2  (or just 2)
; r4: loop counter
; r5: mask
;*****************************************************************************/

khrn_bf_blk_subsample_tf_32::

v16bitplanes -,0x3333 SETF       ;1100110011001100
mov r4, 0

1:
add r2, r1, 2
v32even  H32(48,0),   H32(0,0)+r0, H32(4,0)+r0
v32odd   H32(49,0),   H32(0,0)+r0, H32(4,0)+r0
v32even  H32(50,0),   H32(1,0)+r0, H32(5,0)+r0
v32odd   H32(51,0),   H32(1,0)+r0, H32(5,0)+r0

v32add   H32(52,0),   H32(48,0),   H32(49,0)
v32add   H32(53,0),   H32(50,0),   H32(51,0)
v32add   H32(0,0)+r1, H32(52,2),   H32(52,0)    IFNZ
v32add   H32(0,0)+r2, H32(53,2),   H32(53,0)    IFNZ
v32lsr   H32(0,0)+r1, H32(0,0)+r1, 2

mov r5, 0xff030300
v32and   H32(48++,0), H32(48++,0), r5           REP 4
v32lsr   H32(48++,0), H32(48++,0), 2            REP 4
v32add   H32(52,0),   H32(48,0),   H32(49,0)
v32add   H32(53,0),   H32(50,0),   H32(51,0)
v32add   H32(48,0),   H32(52,2),   H32(52,0)    IFNZ
mov r2, 2
v32add   H32(48,0)+r2,H32(53,2),   H32(53,0)    IFNZ

mov r5, 0xc0c0c0c0
v32and   H32(48,0),   H32(48,0),   r5
mov r5, 0xc0000000
v32eor   H32(48,0),   H32(48,0),   r5
mov r5, 0x40000000
v32add   H32(48,0),   H32(48,0),   r5
v32sub   H32(0,0)+r1, H32(0,0)+r1, H32(48,0)

add r4, 1
cmp r4, 2
add    r0, 0x80
add    r1, 0x40
add.eq r0, 0x100
add.eq r1, 0x80
addcmpblt r4,0,4,1b

b lr




;/*****************************************************************************
; khrn_bf_row_subsample_tf_565
; r0: dst address
; r1: src address
; r2: number of input subtiles
; r3: row number (controls initial phases)
; -------
; r0: temporary
; r3: VRF offset of input block (16 rows, i.e. 16 microtiles)
; r1: VRF offset of output block (row row blank blank row row)
;  48-55 is temporary workspace

; r2: r1+2  (or just 2)
; r4: loop counter
; r5: mask
; r6: src address
; r7: dst address
; r8: number of input subtiles remaining
; r9: phase (position in dst subtile)
; r10: phase (dst address): 3 <-> 1 or -1 <-> -3
; r11: number of subtiles processed
;*****************************************************************************/

khrn_bf_row_subsample_tf_565::
stm r6-r11,lr,(--sp)
.macro ROW_SUBSAMPLE_START
mov r7, r0
mov r6, r1
mov r8, r2
mov r1, 0x800
mov r11, 0

mov r9, 0
mov r10, 3
btest r3, 0
add.nz r9, 2     ; Odd rows fill subtiles in opposite order
add.nz r1, 0x280

eor.nz r10, -4
btest r3, 1
eor.nz r10, -4   ; Rows 1,2 (mod 4) fill dst in opposite order. Start from -1 instead of 3.

mov r3, 0

mov r2, 64
mov r0, 16
v32ld H32(0++,0)+r3, (r6 += r2) REP r0
eor r3, 0x400
add r6, 1024

;;;;;;;;;;;;;;;
; Main loop
;;;;;;;;;;;;;;;

2:

mov r2, 64
mov r0, 16
v32ld H32(0++,0)+r3, (r6 += r2) REP r0
eor r3, 0x400
add r6, 1024

;;;;;;;;;;;
; Begin subsample code
;;;;;;;;;;;

v16bitplanes -,0x3333 SETF       ;1100110011001100
mov r4, 0

1:
add r2, r1, 2
v16even  H16(48, 0),   H16(0, 0)+r3, H16(4, 0)+r3
v16odd   H16(48,32),   H16(0, 0)+r3, H16(4, 0)+r3
v16even  H16(49, 0),   H16(0,32)+r3, H16(4,32)+r3
v16odd   H16(49,32),   H16(0,32)+r3, H16(4,32)+r3
v16even  H16(50, 0),   H16(1, 0)+r3, H16(5, 0)+r3
v16odd   H16(50,32),   H16(1, 0)+r3, H16(5, 0)+r3
v16even  H16(51, 0),   H16(1,32)+r3, H16(5,32)+r3
v16odd   H16(51,32),   H16(1,32)+r3, H16(5,32)+r3
.endm
ROW_SUBSAMPLE_START

mov r5, 0x07e0f81f       ; 00000111111000001111100000011111
v32and   H32(52++,0), H32(48++,0), r5           REP 4
v32add   H32(52,0),   H32(52,0),   H32(53,0)
v32add   H32(53,0),   H32(54,0),   H32(55,0)
v32add   H32(0,0)+r1, H32(52,2),   H32(52,0)    IFNZ
v32add   H32(0,0)+r2, H32(53,2),   H32(53,0)    IFNZ
v32lsr   H32(0,0)+r1, H32(0,0)+r1, 2
v32and   H32(0,0)+r1, H32(0,0)+r1, r5

mov r5, 0xf81f07e0       ; 11111000000111110000011111100000
v32and   H32(48++,0), H32(48++,0), r5           REP 4
v32lsr   H32(48++,0), H32(48++,0), 2            REP 4
v32add   H32(52,0),   H32(48,0),   H32(49,0)
v32add   H32(53,0),   H32(50,0),   H32(51,0)
v32add   H32(48,0),   H32(52,2),   H32(52,0)    IFNZ
mov r2, 2
v32add   H32(48,0)+r2,H32(53,2),   H32(53,0)    IFNZ
v32and   H32(48,0),   H32(48,0),   r5

v32or    H32(0,0)+r1, H32(0,0)+r1, H32(48,0)

.macro ROW_SUBSAMPLE_END
add r4, 1
cmp r4, 2
add    r3, 0x80
add    r1, 0x40
add.eq r3, 0x100
add.eq r1, 0x80
addcmpblt r4,0,4,1b

sub r3, 0x300       ; Restore r3 back to what it was
sub r1, 0x180       ; Restore r1 back to what it was

;;;;;;;;;;;
; End subsample code
;;;;;;;;;;;

btest r9, 0
mov r0, 0x80
add.z r0, 0x180
btest r9, 1
add.z r1, r0
sub.nz r1, r0

addcmpbne r11, 1, 4, 1f
mov r2, 64
mov r0, 16
v32st H32(32++,0), (r7 += r2) REP r0
shl r0, r10, 10
add r7, r0
eor r10, 2        ; 3<->1 or -1<->-3
mov r11, 0
1:

add r9, 1

addcmpbne r8, -1, 0, 2b
.endm
ROW_SUBSAMPLE_END


ldm r6-r11,pc,(sp++)


;/*****************************************************************************
; khrn_bf_row_subsample_tf_4444
; Like khrn_bf_row_subsample_tf_565
;*****************************************************************************/

khrn_bf_row_subsample_tf_4444::
stm r6-r11,lr,(--sp)
ROW_SUBSAMPLE_START

mov r5, 0x0f0f0f0f       ; 00001111000011110000111100001111
v32and   H32(52++,0), H32(48++,0), r5           REP 4
v32add   H32(52,0),   H32(52,0),   H32(53,0)
v32add   H32(53,0),   H32(54,0),   H32(55,0)
v32add   H32(0,0)+r1, H32(52,2),   H32(52,0)    IFNZ
v32add   H32(0,0)+r2, H32(53,2),   H32(53,0)    IFNZ
v32lsr   H32(0,0)+r1, H32(0,0)+r1, 2
v32and   H32(0,0)+r1, H32(0,0)+r1, r5

mov r5, 0xf0f0f0f0       ; 11110000111100001111000011110000
v32and   H32(48++,0), H32(48++,0), r5           REP 4
v32lsr   H32(48++,0), H32(48++,0), 2            REP 4
v32add   H32(52,0),   H32(48,0),   H32(49,0)
v32add   H32(53,0),   H32(50,0),   H32(51,0)
v32add   H32(48,0),   H32(52,2),   H32(52,0)    IFNZ
mov r2, 2
v32add   H32(48,0)+r2,H32(53,2),   H32(53,0)    IFNZ
v32and   H32(48,0),   H32(48,0),   r5

v32or    H32(0,0)+r1, H32(0,0)+r1, H32(48,0)

ROW_SUBSAMPLE_END
ldm r6-r11,pc,(sp++)

;/*****************************************************************************
; khrn_bf_row_subsample_tf_5551
; Like khrn_bf_row_subsample_tf_565, although care must be taken with the alpha channel
;*****************************************************************************/

khrn_bf_row_subsample_tf_5551::
stm r6-r11,lr,(--sp)
ROW_SUBSAMPLE_START

mov r5, 0x07c0f83e       ; 00000111110000001111100000111110
v32and   H32(52++,0), H32(48++,0), r5           REP 4
v32add   H32(52,0),   H32(52,0),   H32(53,0)
v32add   H32(53,0),   H32(54,0),   H32(55,0)
v32add   H32(0,0)+r1, H32(52,2),   H32(52,0)    IFNZ
v32add   H32(0,0)+r2, H32(53,2),   H32(53,0)    IFNZ
v32lsr   H32(0,0)+r1, H32(0,0)+r1, 2
v32and   H32(0,0)+r1, H32(0,0)+r1, r5

mov r5, 0xf83e07c0       ; 11111000001111100000011111000000
v32and   H32(52++,0), H32(48++,0), r5           REP 4
v32lsr   H32(52++,0), H32(52++,0), 2            REP 4
v32add   H32(52,0),   H32(52,0),   H32(53,0)
v32add   H32(53,0),   H32(54,0),   H32(55,0)
v32add   H32(52,0),   H32(52,2),   H32(52,0)    IFNZ
mov r2, 2
v32add   H32(52,0)+r2,H32(53,2),   H32(53,0)    IFNZ
v32and   H32(52,0),   H32(52,0),   r5

v32or    H32(0,0)+r1, H32(0,0)+r1, H32(52,0)

mov r5, 0x00010001       ; 00000000000000010000000000000001
v32and   H32(48++,0), H32(48++,0), r5           REP 4
v32add   H32(52,0),   H32(48,0),   H32(49,0)
v32add   H32(53,0),   H32(50,0),   H32(51,0)
v32add   H32(48,0),   H32(52,2),   H32(52,0)    IFNZ
mov r2, 2
v32add   H32(48,0)+r2,H32(53,2),   H32(53,0)    IFNZ
mov r2, 0x00020002
v32add   H32(48,0),   H32(48,0),   r2

v32lsr   H32(48,0),   H32(48,0), 2
v32and   H32(48,0),   H32(48,0),   r5
v32or    H32(0,0)+r1, H32(0,0)+r1, H32(48,0)

ROW_SUBSAMPLE_END
ldm r6-r11,pc,(sp++)



; r0 Width in microtiles
; r1 Height in microtiles
; r2 Address
; r3 Stride (offset of row 1 from address)
; r4 VRF offset
khrn_bf_blk_subsample_ld_microtiles::
mov r5, 64
1:
v32ld H32(0++,0)+r4, (r2 += r5) REP r0
add r2, r3
add r4, 0x100
addcmpbgt r1, -1, 0, 1b
b lr

khrn_bf_blk_subsample_st_microtiles::
mov r5, 64
1:
v32st H32(0++,0)+r4, (r2 += r5) REP r0
add r2, r3
add r4, 0x100
addcmpbgt r1, -1, 0, 1b
b lr
