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
.stabs "asm_file",100,0,0,vc_yuv_deblock


;******************************************************************************
;NAME
;   vc_yuv_deblock
;
;SYNOPSIS
;   void vc_yuv_deblock(unsigned char *data,int width,int height);
;
;FUNCTION
;   Deblocks an 8bpp image using a simple averaging scheme on 8 by 8 blocks
;
;RETURNS
;   -
;******************************************************************************

.section .text$vc_yuv_deblock,"text"
.globl vc_yuv_deblock
vc_yuv_deblock:
   stm   r6-r8,lr,(--sp)
; First deblock rows
   mov   r6,r0 ;data
   mov   r7,r1 ;width
   mov   r8,r2 ;height
   bl deblock_rows
   
; Next deblock columns
   mov   r0,r6
   mov   r1,r7
   mov   r2,r8
   bl deblock_columns
   
   ldm   r6-r8,pc,(sp++)
.stabs "vc_yuv_deblock:F1",36,0,0,vc_yuv_deblock

.FORGET

deblock_rows2:
   ; Work in multiples of 16 so round down
   mov   r3,r1 ; r3 is the pitch in bytes
   mov   r4,7  
   mul   r4,r3 ; r4 is the offset to the first row to be part of deblocking (7 rows down)
   add   r0,r4 ; r0 points to the first row to be deblocked
   
   asr   r1,4  ;  r1 is number of 16 blocks to deblock across
   
   ; Don't deblock the top and bottom rows
   asr   r2,3  ; r2 is number of 8 blocks down
   sub   r2,1  ; r2 is now the number of rows to deblock
loopdown:   
   mov   r5,0  ; across counter
loopacross:   
   vld   H(0++,0),(r0+=r3) REP 2   ; data from higher block and lower blocks
   
   ; First block has 3 times higher, plus 1 times lower  (add 2 for rounding)
   vmul  -,H(0,0),3 CLRA ACC
   vadd  HX(2,0),H(1,0),2 ACC
   
   vmul  -,H(1,0),3 CLRA ACC
   vadd  HX(3,0),H(0,0),2 ACC
   
   vasr  H(0++,0),HX(2++,0),2 REP 2
   ;vmov  H(0++,0),0 REP 2
   vst  H(0++,0),(r0+=r3) REP 2 
   add   r0,16
   addcmpblt r5,1,r1,loopacross  

   btest r3,1
   beq   blocksof16
   ; Need to patch up the last 8 columns here
   add   r0,8
blocksof16:
   add   r0,r4
   addcmpbgt r2,-1,0,loopdown
   b  lr
.stabs "deblock_rows2:F1",36,0,0,deblock_rows2

.FORGET

; New scheme applies a bigger effect.
; 
; Find discrepancy at join d=b(0)-a(7)
;
; a(4)...a(7) b(0)..b(3)
;
; Then apply a proportion of this to all 3 rows on either side
; d/12 3d/12 5d/12 -5d/12 -3d/12 -d/12
.section .text$deblock_rows,"text"
.globl deblock_rows
deblock_rows:
   ; Work in multiples of 16 so round down
   mov   r3,r1 ; r3 is the pitch in bytes
   mov   r4,4 
   mul   r4,r3 ; r4 is the offset to the first row to be part of deblocking (4 rows down)
   add   r0,r4 ; r0 points to the first row to be deblocked
   
   mov   r4,7
   mul   r4,r3 ; r4 is the remaining offset after we have processed one row
   
   asr   r1,4  ;  r1 is number of 16 blocks to deblock across
   
   ; Don't deblock the top and bottom rows
   asr   r2,3  ; r2 is number of 8 blocks down
   sub   r2,1  ; r2 is now the number of rows to deblock
loopdown:   
   mov   r5,0  ; across counter
loopacross:   
   vld   H(0++,0),(r0+=r3) REP 8   ; data from higher block and lower blocks
   
   vsub  HX(9,0),H(4,0),H(3,0)   ; HX(9) is the difference
   ;  mulm works in 8.8 format so 256 is equal to 1
   vmulm HX(0,32),HX(9,0),0
   vmulm HX(1,32),HX(9,0),21
   vmulm HX(2,32),HX(9,0),64
   vmulm HX(3,32),HX(9,0),107
   vmulm HX(4,32),HX(9,0),-107
   vmulm HX(5,32),HX(9,0),-64
   vmulm HX(6,32),HX(9,0),-21
   vmulm HX(7,32),HX(9,0),0
   
   vadds  H(0++,0),H(0++,0),HX(0++,32) REP 8

   vst  H(0++,0),(r0+=r3) REP 8 
   add   r0,16
   addcmpblt r5,1,r1,loopacross  

   btest r3,1
   beq   blocksof16
   ; Need to patch up the last 8 columns here
   add   r0,8
blocksof16:
   add   r0,r4
   addcmpbgt r2,-1,0,loopdown
   b  lr
.stabs "deblock_rows:F1",36,0,0,deblock_rows



.FORGET

; New scheme applies a bigger effect.
; 
; Find discrepancy at join d=b(0)-a(7)
;
; a(4)...a(7) b(0)..b(3)
;
; Then apply a proportion of this to all 3 rows on either side
; d/12 3d/12 5d/12 -5d/12 -3d/12 -d/12
.section .text$deblock_columns,"text"
.globl deblock_columns
deblock_columns:
   ; Work in multiples of 16 so round down
   mov   r3,r1 ; r3 is the pitch in bytes
   mov   r4,4
   ; r0 is offset to the first row to be part of deblocking (4 columns across)
   
   mov   r4,15
   mul   r4,r3 ; r4 is the remaining offset after we have processed one row
   
   asr   r1,4  ;  r1 is number of 16 blocks to deblock across
   sub   r1,1  ;  r1 is now the number of blocks across to deblock
   
   ; Don't deblock the top and bottom rows
   asr   r2,4  ; r2 is number of 8 blocks down
   
loopdown:   
   mov   r5,0  ; across counter
loopacross:   
   vld   V(0,0++),(r0+=r3) REP 16   ; data from left block 
   vld   V(16,0++),(r0+16+=r3) REP 16  ; little data from right block
   
   vsub  HX(16,32),H(8,0),H(7,0)   ; HX(9) is the difference
   ;  mulm works in 8.8 format so 256 is equal to 1
   vmulm HX(0,32),HX(16,32),0
   vmulm HX(1,32),HX(16,32),21
   vmulm HX(2,32),HX(16,32),64
   vmulm HX(3,32),HX(16,32),107
   vmulm HX(4,32),HX(16,32),-107
   vmulm HX(5,32),HX(16,32),-64
   vmulm HX(6,32),HX(16,32),-21
   vmulm HX(7,32),HX(16,32),0
   
   vadds  H(4++,0),H(4++,0),HX(0++,32) REP 8
   
   vsub  HX(16,32),H(16,0),H(15,0)   ; HX(9) is the difference
   ;  mulm works in 8.8 format so 256 is equal to 1
   vmulm HX(0,32),HX(16,32),0
   vmulm HX(1,32),HX(16,32),21
   vmulm HX(2,32),HX(16,32),64
   vmulm HX(3,32),HX(16,32),107
   vmulm HX(4,32),HX(16,32),-107
   vmulm HX(5,32),HX(16,32),-64
   vmulm HX(6,32),HX(16,32),-21
   vmulm HX(7,32),HX(16,32),0

   vadds  H(12++,0),H(12++,0),HX(0++,32) REP 8

   vst   V(0,0++),(r0+=r3) REP 16   ; data from left block 
   vst   V(16,0++),(r0+16+=r3) REP 16  ; little data from right block
   add   r0,16
   addcmpblt r5,1,r1,loopacross  

   add   r0,16 ; the last block is not done to avoid overwriting past the edge

   btest r3,1
   beq   blocksof16
   ; Need to patch up the last 8 columns here
   add   r0,8
blocksof16:
   add   r0,r4
   addcmpbgt r2,-1,0,loopdown
   b  lr
.stabs "deblock_columns:F1",36,0,0,deblock_columns


;;; NAME
;;;    yuv420_vflip_in_place_16
;;;
;;; SYNOPSIS
;;;    void yuv420_vflip_in_place_16(unsigned char *start_row, unsigned char *end_row, int pitch, int nbytes)
;;;
;;; FUNCTION
;;;    Do vertical flip for entire block, assuming 16-byte alignment.
;;;
;;; RETURNS
;;;    -

.section .text$yuv420_vflip_in_place_16,"text"
.globl yuv420_vflip_in_place_16
yuv420_vflip_in_place_16:
      stm r6-r6, lr, (--sp)
      add r5, r2, r3
      sub r2, r2, r3
      mov r6, 16
yuv420_vflip_in_place_16_vloop:
      add r4, r0, r3
yuv420_vflip_in_place_16_hloop:  
      vld H(0,0), (r0)
      vld H(0,16), (r1)
      vst H(0,0), (r1)
      vst H(0,16), (r0)
      add r1, 16
      addcmpbne r0, r6, r4, yuv420_vflip_in_place_16_hloop
      sub r1, r5
      addcmpblt r0, r2, r1, yuv420_vflip_in_place_16_vloop
      ldm r6-r6, pc, (sp++)
.stabs "yuv420_vflip_in_place_16:F1",36,0,0,yuv420_vflip_in_place_16

;;; NAME
;;;    yuv420_vflip_in_place_8
;;;
;;; SYNOPSIS
;;;    void yuv420_vflip_in_place_8(unsigned char *start_row, unsigned char *end_row, int pitch, int nbytes)
;;;
;;; FUNCTION
;;;    Do vertical flip for entire block, assuming 8-byte alignment.
;;;    Big cheat, we do this on the scalar unit.
;;;
;;; RETURNS
;;;    -

.section .text$yuv420_vflip_in_place_8,"text"
.globl yuv420_vflip_in_place_8
yuv420_vflip_in_place_8:
      stm r6-r7, lr, (--sp)
      add r5, r2, r3
      sub r2, r2, r3
yuv420_vflip_in_place_8_vloop:
      add r4, r0, r3
yuv420_vflip_in_place_8_hloop:   
      ld r6, (r0)
      ld r7, (r1)
      st r6, (r1++)
      st r7, (r0++)
      ld r6, (r0)
      ld r7, (r1)
      st r6, (r1++)
      st r7, (r0++)
      addcmpbne r0, 0, r4, yuv420_vflip_in_place_8_hloop
      sub r1, r5
      addcmpblt r0, r2, r1, yuv420_vflip_in_place_8_vloop
      ldm r6-r7, pc, (sp++)
.stabs "yuv420_vflip_in_place_8:F1",36,0,0,yuv420_vflip_in_place_8

;;; NAME
;;;    yuv420_hflip_load_block
;;;
;;; SYNOPSIS
;;;    void yuv420_hflip_load_block(unsigned char *addr, int vrf_addr, int pitch)
;;;
;;; FUNCTION
;;;    Load 16x16 pixel block into VRF registers.
;;;
;;; RETURNS
;;;    -

.section .text$yuv420_hflip_load_block,"text"
.globl yuv420_hflip_load_block
yuv420_hflip_load_block:
.if _VC_VERSION > 2
      bic r0, 15
.endif
      vld H(0++,0)+r1, (r0+=r2) REP 16
      b lr     
.stabs "yuv420_hflip_load_block:F1",36,0,0,yuv420_hflip_load_block

;;; NAME
;;;    yuv420_hflip_load_block
;;;
;;; SYNOPSIS
;;;    void yuv420_hflip_load_next_block(unsigned char *addr, int vrf_addr, int pitch)
;;;
;;; FUNCTION
;;;    Load the next 16x16 pixel block into VRF registers.
;;;
;;; RETURNS
;;;    -

.section .text$yuv420_hflip_load_next_block,"text"
.globl yuv420_hflip_load_next_block
yuv420_hflip_load_next_block:
.if _VC_VERSION > 2
      bic r0, 15
.endif
      vld H(0++,16)+r1, (r0+16+=r2) REP 16
      b lr     
.stabs "yuv420_hflip_load_next_block:F1",36,0,0,yuv420_hflip_load_next_block

;;; NAME
;;;    yuv420_hflip
;;;
;;; SYNOPSIS
;;;    void yuv420_hflip(int left_vrf_addr, int right_vrf_addr)
;;;
;;; FUNCTION
;;;    Horizontally flip and swap a (possibly offset) 16x16 block from the top stripe
;;;    of the VRF with one in the next stripe.
;;;
;;; RETURNS
;;;    -

.section .text$yuv420_hflip,"text"
.globl yuv420_hflip
yuv420_hflip:
      mov r2, r1
      add r1, 16
      sub r0, 1
      ;; The loop is made more awkward because we have to keep adding/subtracting
      ;; 1 to the vrf addresses modulo 63 in case of wraparound.
yuv420_hflip_loop:
      add r0, 1
      sub r1, 1
      bmask r0, 6
      bmask r1, 6
      vmov H(32,0), V(0,0)+r0
      vmov V(0,0)+r0, V(16,0)+r1
      vmov V(16,0)+r1, H(32,0)
      addcmpbne r1, 0, r2, yuv420_hflip_loop
      b lr
.stabs "yuv420_hflip:F1",36,0,0,yuv420_hflip

;;; NAME
;;;    yuv420_hflip_save_block
;;;
;;; SYNOPSIS
;;;    void yuv420_hflip_save_block(unsigned char *addr, int vrf_addr, int pitch, int nrows)
;;;
;;; FUNCTION
;;;    Save nrows of the given vrf block to the given address.
;;;
;;; RETURNS
;;;    -

.section .text$yuv420_hflip_save_block,"text"
.globl yuv420_hflip_save_block
yuv420_hflip_save_block:
.if _VC_VERSION > 2
      bic r0, 15
.endif
      mov r4, 64
      mul r3, r4
      add r3, r1     
yuv420_hflip_save_block_loop:
      vst H(0,0)+r1, (r0)
      add r0, r2
      addcmpblt r1, r4, r3, yuv420_hflip_save_block_loop
      b lr
.stabs "yuv420_hflip_save_block:F1",36,0,0,yuv420_hflip_save_block

;;; NAME
;;;    yuv420_hflip_save_next_block
;;;
;;; SYNOPSIS
;;;    void yuv420_hflip_save_next_block(unsigned char *addr, int vrf_addr, int pitch, int nrows)
;;;
;;; FUNCTION
;;;    Save nrows of the next vrf block to the next block address in memory.
;;;
;;; RETURNS
;;;    -

.section .text$yuv420_hflip_save_next_block,"text"
.globl yuv420_hflip_save_next_block
yuv420_hflip_save_next_block:
.if _VC_VERSION > 2
      bic r0, 15
.endif
      mov r4, 64
      mul r3, r4
      add r3, r1     
yuv420_hflip_save_next_block_loop:
      vst H(0,16)+r1, (r0+16)
      add r0, r2
      addcmpblt r1, r4, r3, yuv420_hflip_save_next_block_loop
      b lr
.stabs "yuv420_hflip_save_next_block:F1",36,0,0,yuv420_hflip_save_next_block

;;; NAME
;;;    yuv420_hflip_reverse
;;;
;;; SYNOPSIS
;;;    void yuv420_hflip_reverse(int start, int end)
;;;
;;; FUNCTION
;;;    Reverse the order of vrf columns from start to end inclusive. There must be
;;;    no wraparound.
;;;
;;; RETURNS
;;;    -

.section .text$yuv420_hflip_reverse,"text"
.globl yuv420_hflip_reverse
yuv420_hflip_reverse:
      vmov H(32,0), V(0,0)+r0
      vmov V(0,0)+r0, V(0,0)+r1
      vmov V(0,0)+r1, H(32,0)
      add r0, 1
      addcmpbgt r1, -1, r0, yuv420_hflip_reverse
      b lr
.stabs "yuv420_hflip_reverse:F1",36,0,0,yuv420_hflip_reverse

;;; NAME
;;;    yuv420_transpose_Y
;;;
;;; SYNOPSIS
;;;    void yuv420_transpose_Y(unsigned char *dest, int dest_pitch, unsigned char *src, int src_pitch, int dst_width, int dst_height)
;;;
;;; FUNCTION
;;;    Transpose a 1 byte per pixel image
;;;
;;; RETURNS
;;;    -

.function yuv420_transpose_Y ; {{{
   ;; r0 = remaining rows to write
   ;; r1 = dest_pitch
   ;; r2 = src
   ;; r3 = src_pitch
   ;; r4 = width of dest remaining (or height of src)
   ;; r5 = height of dest remaining (or width of src)
   ;; r6 = dest
   ;; r7 = src inc at end of column
   ;; r8 = dst inc at end of column
   ;; r9 = -1
   ;; r10 = temp
   ;; r11 = copy of dest height
   stm r6-r11, lr, (--sp)
   .cfa_push {lr,r6,r7,r8,r9,r10,r11}

   mov r6, r0
   mov r9, -1
   mov r11, r5

   ;; r7 = src inc at end of column
   add r10, r5, 15
   lsr r10, 4 ; number of tiles in dst image row (or src image column)

   shl r7, r10, 4
   rsub r7, 0
   addscale r7, r3<<4

   ;; r8 = dst inc at end of column
   shl r8, r10, 4
   mul r8, r1
   rsub r8, r8, 16
   
yuv420_transpose_Y_down:
   mov r5, r11
   min r10, r4, 16  ; remaining columns to write
   shl r10, r9, r10
   vbitplanes -, r10 SETF

yuv420_transpose_Y_across:
   min r0, r5, 16  ; remaining rows to write

   vld H(0++,0), (r2+=r3) REP 16
   add r2, 16
   vst V(0,0++), (r6+=r1) IFZ REP r0
   addscale r6, r1 << 4

   sub r5, 16
   addcmpbgt r5, 0, 0, yuv420_transpose_Y_across

   add r2, r7
   add r6, r8

   sub r4, 16
   addcmpbgt r4, 0, 0, yuv420_transpose_Y_down

   ldm r6-r11, pc, (sp++)
   .cfa_pop {r11,r10,r9,r8,r7,r6,pc}
.endfn ; }}}


;;; NAME
;;;    yuv420_transpose_tiles_Y
;;;
;;; SYNOPSIS
;;;    void yuv420_transpose_Y(unsigned char *dest, int dest_pitch, unsigned char *src, int src_pitch, int width_bytes)
;;;
;;; FUNCTION
;;;    Transpose 16-wide column of src into 16-high stripe of dest.
;;;
;;; RETURNS
;;;    -

.section .text$yuv420_transpose_chunky_Y,"text"
.globl yuv420_transpose_chunky_Y
         ;; Transpose stuff, where the source pitch is known to be 16 and the
         ;; dest pitch is known to be 256
yuv420_transpose_chunky_Y:
         stm      r6, (--sp)
         ;; r0 = dest
         ;; r1 = dest_pitch (probably has to be 256)
         ;; r2 = src
         ;; r3 = src_pitch
         addscale r4, r2, r3 << 6
         addscale r5, r2, r3 << 7
         addscale r6, r5, r3 << 6

         vld      V(0,0++), (r2+=r3)         REP 64
         vld      V(16,0++), (r4+=r3)        REP 64
         vld      V(32,0++), (r5+=r3)        REP 64
         vld      V(48,0++), (r6+=r3)        REP 64

         ldm      r6, (sp++)

         mov      r4, 0
         mov      r2, 4
.tcy_loop:
         vst      H(0++,0)+r4, (r0+=r1)      REP 16
         vst      H(0++,16)+r4, (r0+16+=r1)  REP 16
         vst      H(0++,32)+r4, (r0+32+=r1)  REP 16
         vst      H(0++,48)+r4, (r0+48+=r1)  REP 16
         add      r0, 64
         add      r4, 64*16
         addcmpbne r2, -1, 0, .tcy_loop

         b lr
         .stabs "yuv420_transpose_chunky_Y:F1",36,0,0,yuv420_transpose_chunky_Y

.section .text$yuv420_transpose_chunky_UV,"text"
.globl yuv420_transpose_chunky_UV
         ;; Transpose stuff, where the source pitch is known to be 16 and the
         ;; dest pitch is known to be 256
yuv420_transpose_chunky_UV:
         ;; r0 = dest
         ;; r1 = dest_pitch (probably has to be 128)
         ;; r2 = src
         ;; r3 = src_pitch
         addscale r4, r2, r3 << 6

         vldleft  V(0,0++), (r2+=r3)         REP 64
         vldleft  V(16,0++), (r4+=r3)        REP 64

         vst      H(0++,0), (r0+=r1)         REP 8
         vst      H(0++,16), (r0+16+=r1)     REP 8
         vst      H(0++,32), (r0+32+=r1)     REP 8
         vst      H(0++,48), (r0+48+=r1)     REP 8
         vst      H(16++,0), (r0+64+=r1)     REP 8
         vst      H(16++,16), (r0+80+=r1)    REP 8
         vst      H(16++,32), (r0+96+=r1)    REP 8
         vst      H(16++,48), (r0+112+=r1)   REP 8

         b lr
         .stabs "yuv420_transpose_chunky_UV:F1",36,0,0,yuv420_transpose_chunky_UV

.section .text$yuv420_transpose_chunky0_Y,"text"
.globl yuv420_transpose_chunky0_Y
         ;; Transpose stuff, where the source pitch is known to be 16 and the
         ;; dest pitch is known to be 128
yuv420_transpose_chunky0_Y:
         stm      r6, (--sp)
         ;; r0 = dest
         ;; r1 = dest_pitch (probably has to be 128)
         ;; r2 = src
         ;; r3 = src_pitch
         addscale r4, r2, r3 << 5
         addscale r5, r2, r3 << 6
         addscale r6, r5, r3 << 5

         vld      VX(0,0++), (r2+=r3)        REP 32
         vld      VX(16,0++), (r4+=r3)       REP 32
         vld      VX(32,0++), (r5+=r3)       REP 32
         vld      VX(48,0++), (r6+=r3)       REP 32

         ldm      r6, (sp++)

         vst      H(0++,0), (r0+=r1)         REP 16
         vst      H(0++,32), (r0+16+=r1)     REP 16
         vst      H(16++,0), (r0+32+=r1)     REP 16
         vst      H(16++,32), (r0+48+=r1)    REP 16
         vst      H(32++,0), (r0+64+=r1)     REP 16
         vst      H(32++,32), (r0+80+=r1)    REP 16
         vst      H(48++,0), (r0+96+=r1)     REP 16
         vst      H(48++,32), (r0+112+=r1)   REP 16

         b lr
         .stabs "yuv420_transpose_chunky0_Y:F1",36,0,0,yuv420_transpose_chunky0_Y

.section .text$yuv420_transpose_chunky0_UV,"text"
.globl yuv420_transpose_chunky0_UV
         ;; Transpose stuff, where the source pitch is known to be 16 and the
         ;; dest pitch is known to be 256
yuv420_transpose_chunky0_UV:
         ;; r0 = dest
         ;; r1 = dest_pitch (probably has to be 64)
         ;; r2 = src
         ;; r3 = src_pitch
         addscale r4, r2, r3 << 5

         vldleft  VX(0,0++), (r2+=r3)        REP 32
         vldleft  VX(16,0++), (r4+=r3)       REP 32

         vst      H(0++,0), (r0+=r1)         REP 8
         vst      H(0++,32), (r0+16+=r1)     REP 8
         vst      H(16++,0), (r0+32+=r1)     REP 8
         vst      H(16++,32), (r0+48+=r1)    REP 8

         b lr
         .stabs "yuv420_transpose_chunky0_UV:F1",36,0,0,yuv420_transpose_chunky0_UV


.section .text$yuv420_transpose_chunky1_Y,"text"
.globl yuv420_transpose_chunky1_Y
         ;; Transpose stuff, where the source pitch is known to be 16 and the
         ;; dest pitch is known to be 128
yuv420_transpose_chunky1_Y:
         stm      r6, (--sp)
         ;; r0 = dest
         ;; r1 = dest_pitch (probably has to be 128)
         ;; r2 = src
         ;; r3 = src_pitch
         addscale r4, r2, r3 << 5
         addscale r5, r2, r3 << 6
         addscale r6, r5, r3 << 5

         vld      VX(0,0++), (r2+=r3)        REP 32
         vld      VX(16,0++), (r4+=r3)       REP 32
         vld      VX(32,0++), (r5+=r3)       REP 32
         vld      VX(48,0++), (r6+=r3)       REP 32

         ldm      r6, (sp++)

         vst      H(0++,16), (r0+=r1)        REP 16
         vst      H(0++,48), (r0+16+=r1)     REP 16
         vst      H(16++,16), (r0+32+=r1)    REP 16
         vst      H(16++,48), (r0+48+=r1)    REP 16
         vst      H(32++,16), (r0+64+=r1)    REP 16
         vst      H(32++,48), (r0+80+=r1)    REP 16
         vst      H(48++,16), (r0+96+=r1)    REP 16
         vst      H(48++,48), (r0+112+=r1)   REP 16

         b lr
         .stabs "yuv420_transpose_chunky1_Y:F1",36,0,0,yuv420_transpose_chunky1_Y

.section .text$yuv420_transpose_chunky1_UV,"text"
.globl yuv420_transpose_chunky1_UV
         ;; Transpose stuff, where the source pitch is known to be 16 and the
         ;; dest pitch is known to be 256
yuv420_transpose_chunky1_UV:
         ;; r0 = dest
         ;; r1 = dest_pitch (probably has to be 64)
         ;; r2 = src
         ;; r3 = src_pitch

         addscale r4, r2, r3 << 5

         vldleft  VX(0,0++), (r2+=r3)        REP 32
         vldleft  VX(16,0++), (r4+=r3)       REP 32

         vst      H(0++,16), (r0+=r1)        REP 8
         vst      H(0++,48), (r0+16+=r1)     REP 8
         vst      H(16++,16), (r0+32+=r1)    REP 8
         vst      H(16++,48), (r0+48+=r1)    REP 8

         b lr
         .stabs "yuv420_transpose_chunky1_UV:F1",36,0,0,yuv420_transpose_chunky1_UV


;******************************************************************************
;NAME
;   vc_deflicker_yuv
;
;SYNOPSIS
;   void vc_deflicker_yuv(unsigned char *data, int pitch, unsigned char *context);
;
;FUNCTION
;   Deflicker a 16x16 YUV block using a simple low pass filter
;   [1/4 1/2 1/4] vertically. Uses context as a two line buffer between calls.
;
;RETURNS
;   -
;******************************************************************************

.section .text$vc_deflicker_yuv,"text"
.globl vc_deflicker_yuv
vc_deflicker_yuv:
   .cfa_bf vc_deflicker_yuv
   add r3, r0, r1
   btest r1, 4
   beq aligned

   mov r4, 16
unalignedloop:
   vld   H(2++, 0), (r0+=r1) REP 16
   vld   H(0++,0), (r2+=r1) REP 2
   vst   H(16++, 0), (r2+=r1) REP 2

   vadd  HX(0++, 32), H(0++, 0), H(1++, 0) REP 16
   vadd  HX(0++, 32), HX(0++, 32), H(1++, 0) REP 16
   vadd  HX(0++, 32), HX(0++, 32), H(2++, 0) REP 16
   vlsr  H(0++, 0), HX(0++, 32), 2 REP 16
   vst   H(0++,0), (r0+=r1) REP 16
   add r2, r4
   addcmpblt r0, r4, r3, unalignedloop
   b lr
aligned:
   mov r4, 32
alignedloop:
   vld   HX(2++, 0), (r0+=r1) REP 16
   vld   HX(0++,0), (r2+=r1) REP 2
   vst   HX(16++, 0), (r2+=r1) REP 2

   vadd  HX(0++, 32), H(0++, 0), H(1++, 0) REP 16
   vadd  HX(0++, 32), HX(0++, 32), H(1++, 0) REP 16
   vadd  HX(0++, 32), HX(0++, 32), H(2++, 0) REP 16
   vlsr  H(0++, 0), HX(0++, 32), 2 REP 16

   vadd  HX(0++, 32), H(0++, 16), H(1++, 16) REP 16
   vadd  HX(0++, 32), HX(0++, 32), H(1++, 16) REP 16
   vadd  HX(0++, 32), HX(0++, 32), H(2++, 16) REP 16
   vlsr  H(0++, 16), HX(0++, 32), 2 REP 16

   vst   HX(0++,0), (r0+=r1) REP 16
   add r2, r4
   addcmpblt r0, r4, r3, alignedloop
   b lr
   .cfa_ef






;******************************************************************************
;NAME
;   vc_deflicker_yuv8
;
;SYNOPSIS
;   void vc_deflicker_yuv8(unsigned char *data, int pitch, unsigned char *context);
;
;FUNCTION
;   Deflicker a 8x16 YUV block using a simple low pass filter
;   [1/4 1/2 1/4] vertically. Uses context as a two line buffer between calls.
;
;RETURNS
;   -
;******************************************************************************

.section .text$vc_deflicker_yuv8,"text"
.globl vc_deflicker_yuv8
vc_deflicker_yuv8:
   .cfa_bf vc_deflicker_yuv8
   add r3, r0, r1                   ; Store the end address of the stripe in R3
   btest r1, 4                      ; Check whether the pitch is a multiple of 32
   beq aligned8                     ; Jump to the section for aligned processing if pitch is multiple of 32

   mov r4, 16                       ; 
unalignedloop8:
   vld    H(2++,  0), (r0+=r1) REP 8
   vld    H(0++,  0), (r2+=r1) REP 2
   vst    H(8++,  0), (r2+=r1) REP 2

   vadd  HX(0++, 32),  H(0++,  0), H(1++,  0) REP 8
   vadd  HX(0++, 32), HX(0++, 32), H(1++,  0) REP 8
   vadd  HX(0++, 32), HX(0++, 32), H(2++,  0) REP 8
   vlsr   H(0++,  0), HX(0++, 32), 2 REP 8 
   vst    H(0++,  0), (r0+=r1) REP 8
   add r2, r4
   addcmpblt r0, r4, r3, unalignedloop8
   b lr                                                  ; Return

aligned8:
   mov r4, 32                                            ; For widths that are a multiple of 32, we can use both halves of the HX registers, so we work with 32 bytes at a time
alignedloop8:
   vld   HX(2++,  0), (r0+=r1) REP 8                     ; Load 32x8 image piece into VRF at 0,2  - Load Data from Buffer, into VRF, placing it at x=0,y=2, Add pitch to buffer position, increment VRF line number, repeat 8 times
   vld   HX(0++,  0), (r2+=r1) REP 2                     ; Load 2 lines from previous piece into VRF at 0,0  - Load Data from Context Buffer, into VRF, placing it at the origin, Add pitch to context buffer position, increment VRF line number, repeat 2 times
   vst   HX(8++,  0), (r2+=r1) REP 2                     ; Save last 2 lines from image in VRF into context buffer for next stripe - Store Data into Context Buffer, from VRF, taking from 0,8  , Add pitch to context buffer position, increment VRF line number, repeat 2 times

   vadd  HX(0++, 32),  H(0++,  0), H(1++,  0) REP 8      ; Add adjacent lines together, starting with lines 0&1 for 8 pairs, putting the result at 32,0
   vadd  HX(0++, 32), HX(0++, 32), H(1++,  0) REP 8      ; Add second line of previous pairs again
   vadd  HX(0++, 32), HX(0++, 32), H(2++,  0) REP 8      ; Add line below previous pairs to totals
   vlsr   H(0++,  0), HX(0++, 32), 2 REP 8               ; Divide the totals by 4 and move back to original place

   ; Repeat previous block but for second set of 16 pixels
   vadd  HX(0++, 32),  H(0++, 16), H(1++, 16) REP 8
   vadd  HX(0++, 32), HX(0++, 32), H(1++, 16) REP 8
   vadd  HX(0++, 32), HX(0++, 32), H(2++, 16) REP 8
   vlsr   H(0++, 16), HX(0++, 32), 2 REP 8

   vst   HX(0++,  0), (r0+=r1) REP 8                     ; Copy the result into memory
   add r2, r4                                            ; Move the Context Buffer pointer to next block
   addcmpblt r0, r4, r3, alignedloop8                    ; Move the main buffer pointer to next block, and loop if it is less than the end of the buffer
   b lr                                                  ; Return
   .cfa_ef




