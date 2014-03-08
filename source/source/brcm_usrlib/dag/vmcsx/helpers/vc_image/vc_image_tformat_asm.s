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

;;; *****************************************************************************

.include "vcinclude/common.inc"


;;; *****************************************************************************
;;; NAME
;;;   tformat_subtile_load_32bit
;;;
;;; SYNOPSIS
;;;   void tformat_subtile_load_32bit(unsigned long const *src32 /* r0 */);
;;;
;;; FUNCTION
;;;   Load a t-format subtile and re-order it into traditional linear order
;;;   results are in H32(0++,0)

.function tformat_subtile_load_32bit ; {{{
      mov      r1, 64
      vld      V32(48,0++), (r0+=r1)               REP 16

      mov      r0, 0
      mov      r1, -64
.loop:
      add      r1, 64
      v32interl   H32(16,0), H32(48,0)+r0, H32(50,0)+r0
      v32interl   H32(17,0), H32(49,0)+r0, H32(51,0)+r0
      v32interh   H32(18,0), H32(48,0)+r0, H32(50,0)+r0
      v32interh   H32(19,0), H32(49,0)+r0, H32(51,0)+r0
      add      r0, 64 * 4
      v32interl   H32(0,0)+r1, H32(16,0), H32(17,0)
      cmp      r0, 64 * 16
      v32interh   H32(4,0)+r1, H32(16,0), H32(17,0)
      v32interl   H32(8,0)+r1, H32(18,0), H32(19,0)
      v32interh   H32(12,0)+r1, H32(18,0), H32(19,0)
      bne      .loop
      b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   tformat_subtile_store_32bit
;;;
;;; SYNOPSIS
;;;   void tformat_subtile_store_32bit(unsigned long *dst32 /* r0 */);
;;;
;;; FUNCTION
;;;   Take a traditional linear-order 16x16 block and reorder it and store it
;;;   as a t-format subtile.  The source data is taken from H32(0++,0)

.function tformat_subtile_store_32bit ; {{{

      mov      r2, 0
      mov      r3, -1
.loop:
      add      r3, 1
      v32interl   V32(16,0), V32(0,0)+r2, V32(0,2)+r2
      v32interl   V32(16,1), V32(0,1)+r2, V32(0,3)+r2
      v32interh   V32(16,2), V32(0,0)+r2, V32(0,2)+r2
      v32interh   V32(16,3), V32(0,1)+r2, V32(0,3)+r2
      add      r2, 4
      v32interl   V32(48,0)+r3, V32(16,0), V32(16,1)
      cmp      r2, 16
      v32interh   V32(48,4)+r3, V32(16,0), V32(16,1)
      v32interl   V32(48,8)+r3, V32(16,2), V32(16,3)
      v32interh   V32(48,12)+r3, V32(16,2), V32(16,3)
      bne      .loop

      mov      r1, 64
      vst      V32(48,0++), (r0+=r1)               REP 16
      b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   tformat_utiles_store_32bit
;;;
;;; SYNOPSIS
;;;   void tformat_utiles_store_32bit(int count, unsigned long *dst32);
;;;
;;; FUNCTION
;;;   Same as tformat_subtile_store_32bit, but you can limit the number of
;;;   microtiles written. Useful for linear-tile format.

.function tformat_utiles_store_32bit ; {{{

      mov      r2, 0
      mov      r3, -1
.loop:
      add      r3, 1
      v32interl   V32(16,0), V32(0,0)+r2, V32(0,2)+r2
      v32interl   V32(16,1), V32(0,1)+r2, V32(0,3)+r2
      v32interh   V32(16,2), V32(0,0)+r2, V32(0,2)+r2
      v32interh   V32(16,3), V32(0,1)+r2, V32(0,3)+r2
      add      r2, 4
      v32interl   V32(48,0)+r3, V32(16,0), V32(16,1)
      cmp      r2, 16
      v32interh   V32(48,4)+r3, V32(16,0), V32(16,1)
      v32interl   V32(48,8)+r3, V32(16,2), V32(16,3)
      v32interh   V32(48,12)+r3, V32(16,2), V32(16,3)
      bne      .loop

      mov      r2, 64
      vst      V32(48,0++), (r1+=r2)               REP r0
      b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_load_32bit
;;;
;;; SYNOPSIS
;;;   void tformat_linear_load_32bit(
;;;      unsigned long const *src32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Load 32 bits to H32(0++,0) in traditional linear order.

.function tformat_linear_load_32bit ; {{{
      addscale r0, r1 << 4
      rsub     r1, 0
      add      r0, r1
      vld      H32(0++,0), (r0+=r1)                REP 16
      b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_load_32bit_flip
;;;
;;; SYNOPSIS
;;;   unsigned long const * tformat_linear_load_32bit_flip(
;;;      unsigned long const *src32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Load 32 bits to H32(0++,0) in traditional linear order.
;;;   And flips.

.function tformat_linear_load_32bit_flip ; {{{
      vld      H32(0++,0), (r0+=r1)                REP 16
      add r0,  64
      b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_load_rgb565_to_32bit_flip
;;;
;;; SYNOPSIS
;;;   unsigned long const * tformat_linear_load_rgb565_to_32bit_flip(
;;;      unsigned long const *src32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Load 32 bits to H32(0++,0) in traditional linear order.
;;;   And flips.

.function tformat_linear_load_rgb565_to_32bit_flip ; {{{
      vld      H16(0++,0), (r0+=r1)                REP 16
      add r0,  32
      vand     HX(16++,32), HX(0++,0), 0x07E0 REP 16
      vand     HX(16++,0), HX(0++,0), 0xF81F  REP 16

      mov      r2, 0
.loop:
      vmulm    V(0,0)+r2, V(16,16)+r2, 0x108 ; red
      vmulhd.su V(0,16)+r2, VX(16,32)+r2, 0x2080 ; green
      vmulm    V(0,32)+r2, V(16,0)+r2, 0x840 ; blue
      vmov     V(0,48)+r2, 255              ; alpha
      addcmpbne r2, 1, 16, .loop
      b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_load_32bit_flip_rbswap
;;;
;;; SYNOPSIS
;;;   unsigned long const * tformat_linear_load_32bit_flip_rbswap(
;;;      unsigned long const *src32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   tformat_linear_load_32bit_flip, followed by a swap of red and blue

.function tformat_linear_load_32bit_flip_rbswap ; {{{
      vld      H32(0++,0), (r0+=r1)                REP 16
      veor     H(0++,0), H(0++,0), H(0++,32)    REP 16
      veor     H(0++,32), H(0++,32), H(0++,0)   REP 16
      veor     H(0++,0), H(0++,0), H(0++,32)    REP 16
      add r0,  64
      b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_load_32bit_flip_etc1_workarounds
;;;
;;; SYNOPSIS
;;;   unsigned long const * tformat_linear_load_32bit_flip_etc1_workarounds(
;;;      unsigned long const *src32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   tformat_linear_load_32bit_flip but with workaround for FTRMK-9 and HW-686
;;;   if appropriate.

.function tformat_linear_load_32bit_flip_etc1_workarounds ; {{{
      vld         H32(0++,0), (r0+=r1)                REP 16
      add         r0, 64

      stm         r0, lr, (--sp)

.ifdef WORKAROUND_FTRMK_9
      bl tformat_etc1_vflip
.endif
.ifdef __BCM2707A0__
      bl tformat_hw686_swap
.endif

      ldm         r0, pc, (sp++)
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_load_32bit_byteswap
;;;
;;; SYNOPSIS
;;;   unsigned long const * tformat_linear_load_32bit_byteswap(
;;;      unsigned long const *src32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Like tformat_linear_load_32bit_flip but with a workaround for HW-1334.
;;;   This is used for luminance+alpha textures.

.function tformat_linear_load_32bit_byteswap ; {{{
      v32ld    H32(0++,0), (r0+=r1)                REP 16
      mov r1, 0xff00ff00
      v32and   H32(16++,0), H32(0++,0), r1         REP 16
      mov r2, 0x00ff00ff
      v32and   H32(0++,0),  H32(0++,0), r2         REP 16
      v32lsr   H32(16++,0), H32(16++,0), 8         REP 16
      v32shl   H32(0++,0),  H32(0++,0), 8          REP 16
      v32or    H32(0++,0),  H32(0++,0), H32(16++,0) REP 16
      add r0,  64
      b        lr
.endfn ; }}}
.FORGET



;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_load_24bit_flip_rgb24_to_rgb565
;;;
;;; SYNOPSIS
;;;   unsigned long const * tformat_linear_load_24bit_flip_rgb24_to_rgb565(
;;;      unsigned long const *src32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Load two batches of RGBA32 data, converting to one batch of RGB565 and
;;;   leaving in H32(0++,0).

.function tformat_linear_load_24bit_flip_rgb24_to_rgb565 ; {{{

      stm r6, lr, (--sp)
      mov r6, lr
      bl       tformat_linear_load_24bit_flip
      v32mov   H32(32++,0), H32(0++,0)   REP 16
      bl       tformat_linear_load_24bit_flip
      v32mov   H32(48++,0), H32(0++,0)   REP 16
      mov lr, r6

      v16even  H16(0++,0),   H16(32++,32), H16(48++,32)   REP 16     ; Blue
      v16odd   H16(0++,32),  H16(32++,32), H16(48++,32)   REP 16     ; Blue
      v16even  H16(16++,0),  H16(32++,0),  H16(48++,0)    REP 16     ; Red/green
      v16odd   H16(16++,32), H16(32++,0),  H16(48++,0)    REP 16     ; Red/green

      mov r1, 0x07e007e0
      v32lsr   H32(32++,0), H32(16++,0), 5   REP 16   ; Get green in right place
      mov r2, 0xf800f800
      v32and   H32(32++,0), H32(32++,0), r1  REP 16   ; Remove everything except green
      v32shl   H32(16++,0), H32(16++,0), 8   REP 16   ; Get red in right place
      v32and   H32(16++,0), H32(16++,0), r2  REP 16   ; Remove everything except red
      mov r1, 0x001f001f
      v32lsr   H32(0++,0),  H32(0++,0),  3   REP 16   ; Get blue in right place
      v32and   H32(0++,0),  H32(0++,0),  r1  REP 16   ; Remove everything except blue
      v32or    H32(0++,0),  H32(0++,0), H32(32++,0) REP 16  ; Collect green
      v32or    H32(0++,0),  H32(0++,0), H32(16++,0) REP 16  ; Collect red
      ldm r6, pc, (sp++)

      b        lr
.endfn ; }}}
.FORGET



;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_load_32bit_flip_rgba32_to_rgba5551
;;;
;;; SYNOPSIS
;;;   unsigned long const * tformat_linear_load_32bit_flip_rgba32_to_rgba5551(
;;;      unsigned long const *src32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Load two batches of RGBA32 data, converting to one batch of RGB5551 and
;;;   leaving in H32(0++,0).

.function tformat_linear_load_32bit_flip_rgba32_to_rgba5551 ; {{{
      vld      H32(32++,0), (r0+=r1)                REP 16
      add      r0, 64
      vld      H32(48++,0), (r0+=r1)                REP 16
      add      r0, 64
      v16even  H16(0++,0),   H16(32++,32), H16(48++,32)   REP 16     ; Blue/alpha
      v16odd   H16(0++,32),  H16(32++,32), H16(48++,32)   REP 16     ; Blue/alpha
      v16even  H16(16++,0),  H16(32++,0),  H16(48++,0)    REP 16     ; Red/green
      v16odd   H16(16++,32), H16(32++,0),  H16(48++,0)    REP 16     ; Red/green

      mov r1, 0x07c007c0
      v32lsr   H32(32++,0), H32(16++,0), 5   REP 16   ; Get green in right place
      mov r2, 0xf800f800
      v32and   H32(32++,0), H32(32++,0), r1  REP 16   ; Remove everything except green
      v32shl   H32(16++,0), H32(16++,0), 8   REP 16   ; Get red in right place
      v32and   H32(16++,0), H32(16++,0), r2  REP 16   ; Remove everything except red
      v32or    H32(16++,0), H32(16++,0), H32(32++,0) REP 16  ; Collect red and green together
      mov r1, 0x00010001
      v32lsr   H32(32++,0), H32(0++,0),  15  REP 16   ; Get alpha in right place
      v32and   H32(32++,0), H32(32++,0), r1  REP 16   ; Remove everything except alpha
      mov r2, 0x003e003e
      v32lsr   H32(0++,0),  H32(0++,0),  2   REP 16   ; Get blue in right place
      v32and   H32(0++,0),  H32(0++,0),  r2  REP 16   ; Remove everything except blue
      v32or    H32(0++,0),  H32(0++,0), H32(16++,0) REP 16  ; Collect red/green
      v32or    H32(0++,0),  H32(0++,0), H32(32++,0) REP 16  ; Collect alpha

      b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_load_pal32bit
;;;
;;; SYNOPSIS
;;;   unsigned long const * tformat_linear_load_pal32bit(
;;;      unsigned long const *src32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Loads 16x16 pixels, stored as bytes. These are looked up in the 32-bit
;;;   256-entry palette, vc_image_temp_palette. The resulting 32-bit colour
;;;   values are left in H32(0++,0).

.function tformat_linear_load_pal32bit ; {{{
      add         r2, pc, pcrel(vc_image_temp_palette)
      vld         H8(16++,0), (r0+=r1)                REP 16
      mov         r1, 0
      mov         r4, 0x40
      mov         r3, 0x400
loop:
      vmov        -, H8(16,0)+r1    CLRA UACC
      v32lookupml H32(0,0)+r1, r2
.pragma offwarn
      v32mov -,   H32(0,0)+r1       ; Workaround HW-1297
.pragma popwarn
      addcmpblt   r1,r4,r3,loop

      add r0, 16
      b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_load_pal16bit
;;;
;;; SYNOPSIS
;;;   unsigned long const * tformat_linear_load_pal16bit(
;;;      unsigned long const *src32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Loads 32x16 pixels, stored as bytes. These are looked up in the 16-bit
;;;   256-entry palette, vc_image_temp_palette. The resulting 16-bit colour
;;;   values are left in H32(0++,0).

.function tformat_linear_load_pal16bit ; {{{
      add         r2, pc, pcrel(vc_image_temp_palette)
      vld         H8(16++,0), (r0+=r1)                REP 16
      vld         H8(16++,32), (r0+16+=r1)            REP 16
      mov         r1, 0
      mov         r4, 0x40
      mov         r3, 0x400
loop:
      vmov        -, H8(16, 0)+r1   CLRA UACC
      v16lookupml H16(32, 0), r2
.pragma offwarn
      v16mov -,   H16(32, 0)       ; Workaround HW-1297
.pragma popwarn
      vmov        -, H8(16,32)+r1   CLRA UACC
      v16lookupml H16(33, 0), r2

      v16even    H16(0, 0)+r1, H16(32,0), H16(33,0)
      v16odd     H16(0,32)+r1, H16(32,0), H16(33,0)

      addcmpblt   r1,r4,r3,loop

      add r0, 32
      b        lr
.endfn ; }}}
.FORGET





;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_store_32bit
;;;
;;; SYNOPSIS
;;;   void tformat_linear_store_32bit(
;;;      unsigned long       *dst32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Store 32 bits from H32(0++,0) in traditional linear order.

.function tformat_linear_store_32bit ; {{{
         addscale r0, r1 << 4
         rsub     r1, 0
         add      r0, r1
         vst      H32(0++,0), (r0+=r1)                REP 16
         b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_store_32bit_flip
;;;
;;; SYNOPSIS
;;;   void tformat_linear_store_32bit_flip(
;;;      unsigned long       *dst32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Store 32 bits from H32(0++,0) in traditional linear order.
;;;   And flips.

.function tformat_linear_store_32bit_flip ; {{{
         vst      H32(0++,0), (r0+=r1)                REP 16
         b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_raster_store4
;;;
;;; SYNOPSIS
;;;   unsigned long * tformat_raster_store4(
;;;      int                  ycount,
;;;      unsigned long       *dst32,
;;;      int                  pitch,
;;;      int                  trimleft,
;;;      int                  trimright,
;;;      int                  ystart);
;;;
;;; FUNCTION
;;;   Store 32 bits from H32(0++,0) in traditional linear order. The trim
;;;   values, if they are positive, tell us how many bytes should be discarded
;;;   ystart tells us where to start reading from (we always start writing to
;;;   dst32). ycount tells us how many rows to write.
;;;   on each of the four sides of this microtile.

.function tformat_raster_store4 ; {{{
         asr      r3, 2
         asr      r4, 2
         max      r3, 0
         max      r4, 0
         rsub     r4, 16
         sub      r4, r3          ; Number of pixels across
         max      r4, 0
         v32mov   H32(63,0), -1
         v32shl   H32(63,0), H32(63,0), r4
         vbitplanes -, H32(63,0) SETF

         shl      r5, 6
         add      r5, r3

         vst      H32(0++,0)+r5, (r1+=r2)     IFZ REP r0

         addscale r0, r1, r4<<2
         b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_raster_store4_opaque
;;;
;;; SYNOPSIS
;;;   unsigned long * tformat_raster_store4_opaque(
;;;      int                  ycount,
;;;      unsigned long       *dst32,
;;;      int                  pitch,
;;;      int                  trimleft,
;;;      int                  trimright,
;;;      int                  ystart);
;;;
;;; FUNCTION
;;;   Like tformat_raster_store_rgb4, but fills the alpha channel with 0xff.
;;;   Used for converting RGBX to RGBA.

.function tformat_raster_store4_opaque ; {{{
   stm r6, lr, (--sp)
   mov r6, 0xff000000
   v32or H32(0++,0), H32(0++,0), r6 REP 16
   bl tformat_raster_store4
   ldm r6, pc, (sp++)
.endfn ; }}}

;;; *****************************************************************************
;;; NAME
;;;   tformat_raster_store_rgb888
;;;
;;; SYNOPSIS
;;;   unsigned long * tformat_raster_store_rgb888(
;;;      int                  ycount,
;;;      unsigned long       *dst32,
;;;      int                  pitch,
;;;      int                  trimleft,
;;;      int                  trimright,
;;;      int                  ystart);
;;;
;;; FUNCTION
;;;   Store H32(0++,0) in 24-bit raster order. Alpha values are discarded.
;;;   As with tformat_raster_store4, the edges can be trimmed so we avoid
;;;   writing more than we are supposed to.

.function tformat_raster_store_rgb888 ; {{{
   stm r6-r7,lr,(--sp)

   mov      r6, 0
   mov      r7, 0
convert_loop:
   vmov    V8(16,0)+r6, V8(0,0)+r7
   vmov    V8(16,1)+r6, V8(0,16)+r7
   vmov    V8(16,2)+r6, V8(0,32)+r7
   add      r6, 3
   addcmpblt r7, 1, 16, convert_loop

   asr      r3, 2
   asr      r4, 2
   max      r3, 0
   max      r4, 0
   rsub     r4, 16
   sub      r4, r3          ; Number of pixels across
   max      r4, 0

   mul      r3, 3           ; Starting column in VRF
   shl      r5, 6
   add      r5, r3          ; Starting row/column in VRF

   mul      r4, 3           ; Number of bytes to write
write_loop:
   min      r6, r4, 16      ; Number of bytes to write in one go
   mov      r7, -1
   bmask    r7, r6
   vbitplanes -, r7         SETF

.ifdef __BCM2707A0__
   ; We are affected by HW-681
   ; Write to cache-coherent alias instead
   mov      r3, 0x2fffffff
   and      r3, r1
   vst      H8(16++,0)+r5, (r3+=r2)     IFNZ REP r0
.else
   vst      H8(16++,0)+r5, (r1+=r2)     IFNZ REP r0
.endif

   add      r5, r6          ; Next VRF position: advanced by this many bytes
   add      r1, r6          ; Next write location: advanced by this many bytes
   neg      r6
   addcmpbgt r4, r6, 0, write_loop   ; Number of bytes left to write

   mov      r0, r1          ; Return where the write pointer got to
   ldm r6-r7,pc,(sp++)
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_store_32bit_opaque
;;;
;;; SYNOPSIS
;;;   void tformat_linear_store_32bit(
;;;      unsigned long       *dst32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Store 32 bits from H32(0++,0) in traditional linear order.

.function tformat_linear_store_32bit_opaque ; {{{
         addscale r0, r1 << 4
         rsub     r1, 0
         add      r0, r1
         vmov     H(0++,48), 0xff                     REP 16
         vst      H32(0++,0), (r0+=r1)                REP 16
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_store_16bit
;;;
;;; SYNOPSIS
;;;   void tformat_linear_store_16bit(
;;;      unsigned long       *dst16,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Store 16 bits from H32(0++,0) in traditional linear order.

.function tformat_linear_store_16bit ; {{{
         addscale r0, r1 << 4
         rsub     r1, 0
         vbitplanes -, 0x00FF                         SETF
         add      r0, r1
         vst      H32(0++,0), (r0+=r1)                REP 16 IFNZ
         b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_store_16bit_flip
;;;
;;; SYNOPSIS
;;;   void tformat_linear_store_16bit_flip(
;;;      unsigned long       *dst16,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Store 16 bits from H32(0++,0) in traditional linear order.

.function tformat_linear_store_16bit_flip ; {{{
         vbitplanes -, 0x00FF                         SETF
         vst      H32(0++,0), (r0+=r1)                REP 16 IFNZ
         b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_store_24bit
;;;
;;; SYNOPSIS
;;;   void tformat_linear_store_24bit(
;;;      unsigned char       *dst24,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Store 32 bits from H32(0++,0) in traditional linear order.

.function tformat_linear_store_24bit ; {{{
.ifdef RGB888
.ldefine BYTE0, 0
.ldefine BYTE1, 16
.ldefine BYTE2, 32
.else
.ldefine BYTE0, 32
.ldefine BYTE1, 16
.ldefine BYTE2, 0
.endif
         addscale r0, r1 << 4
         rsub     r1, 0
         vbitplanes -, 0x0FFF                         SETF
         add      r0, r1

         mov      r2, 0
         mov      r3, 0
.loop:
         vmov     V(48, 0)+r2, V(0,BYTE0+0)+r3
         vmov     V(48,16)+r2, V(0,BYTE1+0)+r3
         vmov     V(48,32)+r2, V(0,BYTE2+0)+r3

         vmov     V(48,48)+r2, V(0,BYTE0+1)+r3
         vmov     V(48, 1)+r2, V(0,BYTE1+1)+r3
         vmov     V(48,17)+r2, V(0,BYTE2+1)+r3

         vmov     V(48,33)+r2, V(0,BYTE0+2)+r3
         vmov     V(48,49)+r2, V(0,BYTE1+2)+r3
         vmov     V(48, 2)+r2, V(0,BYTE2+2)+r3

         vmov     V(48,18)+r2, V(0,BYTE0+3)+r3
         vmov     V(48,34)+r2, V(0,BYTE1+3)+r3
         vmov     V(48,50)+r2, V(0,BYTE2+3)+r3

         add      r2, 3
         addcmpbne r3, 4, 16, .loop

         vst      H32(48++,0), (r0+=r1)               REP 16 IFNZ
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_store_rgba32_to_rgb565
;;;
;;; SYNOPSIS
;;;   void tformat_linear_store_rgba32_to_rgb565(
;;;      unsigned short      *dst565,  /* r0 */
;;;      int                  pitch);
;;;
;;; FUNCTION
;;;   Take RGBA32 data from H32(0++,0), convert it to RGB565, and store it in
;;;   traditional linear order.

.function tformat_linear_store_rgba32_to_rgb565 ; {{{
         addscale r0, r1 << 4
         rsub     r1, 0
         mov      r2, 0x00F8FCF8
         add      r0, r1
         v32and   H32(0++,0), H32(0++,0), r2       REP 16
         v32lsr   H32(0++,0), H32(0++,0), 3        REP 16   ; shift all down
         vshl     H(0++,16), H(0++,16), 3          REP 16   ; except green
         vror     HX(0++,0), HX(0++,0), 5          REP 16   ; push red above green
         vor      HX(0++,0), HX(0++,0), HX(0++,32) REP 16   ; or blue in below
         vst      HX(0++,0), (r0+=r1)              REP 16
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   tformat_red_blue_swap
;;;
;;; SYNOPSIS
;;;   void tformat_red_blue_swap(void);
;;;
;;; FUNCTION
;;;   Swap the red and blue components of an RGBA32 block at H32(0++,0).

.function tformat_red_blue_swap ; {{{
         veor     H(0++,0), H(0++,0), H(0++,32)    REP 16
         veor     H(0++,32), H(0++,32), H(0++,0)   REP 16
         veor     H(0++,0), H(0++,0), H(0++,32)    REP 16
         b        lr
.endfn ; }}}
.FORGET





;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_load_24bit_flip
;;;
;;; SYNOPSIS
;;;   void tformat_linear_load_24bit_flip(
;;;      unsigned long const *src24,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Load 24 bits to H32(0++,0) in traditional linear order.
;;;   Flips unless pitch is negative.

.function tformat_linear_load_24bit_flip ; {{{
.ifdef RGB888
.ldefine BYTE0, 0
.ldefine BYTE1, 16
.ldefine BYTE2, 32
.else
.ldefine BYTE0, 32
.ldefine BYTE1, 16
.ldefine BYTE2, 0
.endif
.ifdef __BCM2707A0__
      ; vld version of HW-681?
      mov      r5, 0x2fffffff
      and      r5, r0
      vld      H(16++,0), (r5+=r1)            REP 16
      vld      H(16++,16), (r5+16+=r1)        REP 16
      vld      H(16++,32), (r5+32+=r1)        REP 16
.else
      ; Copied and tweaked from vc_image_rgba32_asm.s:rgba32_from_rgb888
      ; ...although I don't think I can have quite understood it.
      vld      H(16++,0), (r0+=r1)            REP 16
      vld      H(16++,16), (r0+16+=r1)        REP 16
      vld      H(16++,32), (r0+32+=r1)        REP 16
.endif

      mov      r2, 0
      mov      r3, 0
      mov      r4, 0xff   ; Pseudo alpha
.loop:
      vmov     V(0,BYTE0)+r2, V(16,0)+r3
      vmov     V(0,BYTE1)+r2, V(16,1)+r3
      vmov     V(0,BYTE2)+r2, V(16,2)+r3
      vmov     V(0,48)+r2, r4

      add      r3, 3
      addcmpbne r2, 1, 16, .loop

      add r0,  48
      b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_gather_linear_utiles_32bit
;;;
;;; SYNOPSIS
;;;   void tformat_gather_linear_utiles_32bit(void* ptr,unsigned int stride,unsigned int wordsleft,unsigned int width)
;;;
;;; FUNCTION
;;;
;;; r0 - ptr     "where start of VRF row would be in memory"
;;; r1 - stride (negative)
;;; r2 - number of pixels left to consume on this row
;;; r3 - width
;;; r4 - bitmask "which items to load next"
;;; r5 - VRF row we're writing to (also acts as loop counter)
;;; r6     [ ** temporary ** ]
;;; r7 - bitmask "which items on this row have already been loaded"

.function tformat_gather_linear_utiles_32bit ; {{{
      stm r6-r7,lr,(--sp)
      mov r7, 0       ; Nothing loaded yet
      mov r5, 0       ; Writing to top of VRF

.loop:
      not r4, r7      ; Don't load if it's been loaded already
      min r6,r2,16    ; Don't load if we've run out of row (and load maximum of 16 elements anyway)
      bmask r4, r6
      vbitplanes -,r4 SETF

      vld     H32(0++,0)+r5, (r0+=r1)    IFNZ REP 4

      or r7, r4       ; Account for pixels we've just loaded
      cmp r7, 0xffff
      bne .load_more_stuff_on_this_row


      ; We've written all the way to the end of this VRF row
      add r0, 64
      sub r2, 16

      mov r7, 0       ; Nothing loaded into next row yet


      add r5, 0x100   ; Increment counter by 4 rows
      cmp r5, 0x400   ; Have we finished?
      bge .finish

      b .loop


.load_more_stuff_on_this_row:
; We haven't managed to fill this row of the VRF
; So we'll have to move onto the next row of the source image
      sub      r0, r3<<2    ; Back to the beginning of this row... or slightly before it
      addscale r0, r1<<2    ; Advance up the image
      add      r2, r3       ; We are now placed before the start of this row.
                            ; So the number of words left to consume on this row is
                            ; more than the width of the image.

      b .loop


.finish:
      ldm r6-r7,pc,(sp++)
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   slow_vld4
;;;
;;; FUNCTION
;;;   vbitplanes -,r4 SETF
;;;   vld H32(16++,0)+r5, (r0+=r1)   IFNZ REP 4
;;;
;;;   ... but allows loads which are not word-aligned
;;;
;;; r2: counter
;;; r3: aligned address
;;; r4: temporary

.function slow_vld4
   stm r0-r5,lr,(--sp)

   ;shl r2,r4,1
   ;or  r2,r4
   ;vbitplanes -,r2 SETF
   vbitplanes -,r4 SETF

   mov r2,0
.loop:
   bic r3,r0,3
   vld H32(0,0), r3   ;IFNZ
   sub r4,r0,r3
   shl r4,4
   vmov H8(16,0)+r5, H8(0,0)+r4   IFNZ
   add r4,16
   cmp r4,64
   mov.eq r4,1
   vmov H8(16,16)+r5, H8(0,0)+r4  IFNZ
   add r4,16
   cmp r4,64
   mov.eq r4,1
   vmov H8(16,32)+r5, H8(0,0)+r4  IFNZ
   add r4,16
   cmp r4,64
   mov.eq r4,1
   vmov H8(16,48)+r5, H8(0,0)+r4  IFNZ


   add r0,r1
   add r5,0x40
   addcmpbne r2, 1, 4, .loop

   ldm r0-r5,pc,(sp++)
.endfn

;;; *****************************************************************************
;;; NAME
;;;   tformat_gather_linear_utiles_24bit
;;;
;;; SYNOPSIS
;;;   void tformat_gather_linear_utiles_24bit(const void* ptr,unsigned int stride,unsigned int wordsleft,unsigned int wordsperrow)
;;;
;;; FUNCTION
;;;
;;; r0 - ptr     "where start of VRF row would be in memory"
;;; r1 - stride
;;; r2 - number of words left to consume on this row
;;; r3 - number of words per row
;;; r4 - bitmask "which items to load next"
;;; r5 - VRF row we're writing to (also acts as loop counter)
;;; r6     [ ** temporary ** ]
;;; r7 - bitmask "which items on this row have already been loaded"
;;; r8 - set to nonzero if slow vld is required

.function tformat_gather_linear_utiles_24bit ; {{{
      stm r6-r8,lr,(--sp)
      mov r7, 0       ; Nothing loaded yet
      mov r5, 0       ; Writing to top of VRF
      bmask r8, r1, 2

.loop:
      not r4, r7      ; Don't load if it's been loaded already
      min r6,r2,12    ; Don't load if we've run out of row (and load maximum of 12 words anyway)
      bmask r4, r6

      cmp r8,0
      beq .standard_vld

      bl      slow_vld4
      b       .endvld

.standard_vld:
      vbitplanes -,r4 SETF
      vld     H32(16++,0)+r5, (r0+=r1)    IFNZ REP 4
.endvld:

      or r7, r4       ; Account for pixels we've just loaded
      cmp r7, 0xfff
      bne .load_more_stuff_on_this_row


      ; We've written all the way to the end of this VRF row
      add r0, 48      ; Advance pointer
      sub r2, 12      ; Advance words consumed

      mov r7, 0       ; Nothing loaded into next row yet


      add r5, 0x100   ; Increment counter by 4 rows
      cmp r5, 0x400   ; Have we finished?
      bge .finish

      b .loop

.load_more_stuff_on_this_row:
; We haven't managed to fill this row of the VRF
; So we'll have to move onto the next row of the source image
      sub      r0, r3<<2    ; Back to the beginning of this row... or slightly before it
      addscale r0, r1<<2    ; Advance up the image
      add      r2, r3       ; We are now placed before the start of this row.
                            ; So the number of words left to consume on this row is
                            ; more than the width of the image.

      b .loop

.finish:
; I can't be bothered writing a clever 24bit->32bit converter right now

      mov r0,0
      mov r1,0
.convertloop:
      vmov V8(0, 0)+r0, V8(16,0)+r1
      vmov V8(0,16)+r0, V8(16,16)+r1
      vmov V8(0,32)+r0, V8(16,32)+r1

      vmov V8(0, 1)+r0, V8(16,48)+r1
      vmov V8(0,17)+r0, V8(16,1)+r1
      vmov V8(0,33)+r0, V8(16,17)+r1

      vmov V8(0, 2)+r0, V8(16,33)+r1
      vmov V8(0,18)+r0, V8(16,49)+r1
      vmov V8(0,34)+r0, V8(16,2)+r1

      vmov V8(0, 3)+r0, V8(16,18)+r1
      vmov V8(0,19)+r0, V8(16,34)+r1
      vmov V8(0,35)+r0, V8(16,50)+r1

      add r1,3
      addcmpbne r0,4,16,.convertloop

      ldm r6-r8,pc,(sp++)
.endfn ; }}}
.FORGET



















;;; *****************************************************************************
;;; NAME
;;;   tformat_subtiles_store_8bit
;;;
;;; SYNOPSIS
;;;   void tformat_subtiles_store_8bit(char *dst8_left, char *dst8_right);
;;;
;;; FUNCTION
;;;   Take two linear order 32x32 blocks of bytes (packed into words in the
;;;   VRF) and store them in t-format.

.function tformat_subtiles_store_8bit ; {{{
      mov         r2, 0
      mov         r4, 64

.start:
      mov         r3, 0
.loop:
      v32interl   V32(32,0)+r3, V32(0,0)+r2, V32(0,1)+r2
      v32interh   V32(32,4)+r3, V32(0,0)+r2, V32(0,1)+r2
      v32interl   V32(32,8)+r3, V32(16,0)+r2, V32(16,1)+r2
      v32interh   V32(32,12)+r3, V32(16,0)+r2, V32(16,1)+r2

      add      r3, 1
      add      r2, 2
      cmp      r3, 4
      bne      .loop

      v32st    V32(32,0++), (r0+=r4) REP 16

      cmp      r1, 0   ; Should we write the right hand subtile too?
      beq      .finish

      mov      r0, r1
      mov      r1, 0   ; Remind us to finish next time
      b        .start

.finish:
      b lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_load_8bit_flip
;;;
;;; SYNOPSIS
;;;   unsigned long const * tformat_linear_load_8bit_flip(
;;;      unsigned long const *src32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Load 2 subtiles worth of 8bit data from a raster order image. This data
;;;   is effectively flipped (or at least it will be when stored as t-format).

.function tformat_linear_load_8bit_flip ; {{{
      vld      H32(0++,0), (r0+=r1)                REP 32
      add r0,  64
      b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_load_8bit_nibbleswap
;;;
;;; SYNOPSIS
;;;   unsigned long const * tformat_linear_load_8bit_flip(
;;;      unsigned long const *src32,   /* r0 */
;;;      int                  pitch);  /* r1 */
;;;
;;; FUNCTION
;;;   Like tformat_linear_load_8bit_flip but with a workaround for HW-1324.
;;;   This is used for 4bpp paletted textures where the nibbles are stored the
;;;   opposite way around in the OpenGL source image and T-format destination
;;;   image.

.function tformat_linear_load_8bit_nibbleswap ; {{{
      v32ld    H32(0++,0), (r0+=r1)                REP 32
      mov r1, 0xf0f0f0f0
      v32and   H32(32++,0), H32(0++,0), r1         REP 32
      mov r2, 0x0f0f0f0f
      v32and   H32(0++,0),  H32(0++,0), r2         REP 32
      v32lsr   H32(32++,0), H32(32++,0), 4         REP 32
      v32shl   H32(0++,0),  H32(0++,0), 4          REP 32
      v32or    H32(0++,0),  H32(0++,0), H32(32++,0) REP 32
      add r0,  64
      b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   tformat_hw686_swap
;;;
;;; SYNOPSIS
;;;   void tformat_hw686_swap()
;;;
;;; FUNCTION
;;;   Swaps the endianness of the 64bit items stored in the top 16 rows of the
;;;   VRF. Used to work around bug HW-686.
;;;   Note that it is not possible to do this correctly in all cases.

.ifdef __BCM2707A0__

.function tformat_hw686_swap ; {{{

; First, swap alternate words
   vbitplanes -,0x5555 SETF
   mov r0, 1
   vmov H32(16++,0)+r0, H32(0++,0)    IFNZ REP 16
   vmov H32(16++,0),    H32(0++,0)+r0 IFNZ REP 16

; Now swap the byte ordering within each word
   vmov H8(0++, 0), H8(16++,48) REP 16
   vmov H8(0++,16), H8(16++,32) REP 16
   vmov H8(0++,32), H8(16++,16) REP 16
   vmov H8(0++,48), H8(16++, 0) REP 16

   b lr
.endfn ; }}}

.endif

;;; *****************************************************************************
;;; NAME
;;;   tformat_etc1_vflip
;;;
;;; SYNOPSIS
;;;   void tformat_etc1_vflip()
;;;
;;; FUNCTION
;;;   Flips individual ETC1 tiles
;;;   Not the most efficient sequence of instructions, but left so that I can
;;;   see what I'm doing.

.function tformat_etc1_vflip ; {{{
; Firstly, vflip the pixel index values
; This means reversing each group of 4 bits in alternate words
   v32mov H32(32++,0), 0 REP 16
   vbitplanes -,0xaaaa SETF
   vbitrev H8(16++,0), H8(0++,0), 8       IFNZ REP 16
   vbitrev H8(16++,16), H8(0++,16), 8     IFNZ REP 16
   vbitrev H8(16++,32), H8(0++,32), 8     IFNZ REP 16
   vbitrev H8(16++,48), H8(0++,48), 8     IFNZ REP 16
   mov r0,0x0f0f0f0f
   v32and H32(0++,0), H32(16++,0), r0      IFNZ REP 16
   v32shl H32(0++,0), H32(0++,0),  4       IFNZ REP 16
   v32lsr H32(16++,0), H32(16++,0),  4     IFNZ REP 16
   v32and H32(16++,0), H32(16++,0), r0     IFNZ REP 16
   v32or  H32(0++,0), H32(0++,0), H32(16++,0) IFNZ REP 16

; Now vflip the base colours
; If flipbit=0 (side by side) there is nothing to do here.
; Otherwise, if diffbit=0 then we swap the nibbles of each colour component
; If diffbit=1, we need to add the difference to the base colour, then negate
; the difference.
   mov r0,0
.loop:

; Look for words with flipbit=1, diffbit=0
   vbitplanes -, 0x5555 SETF
   mov r1,0x01000000
   v32and -, H32(0,0)+r0, r1  IFNZ SETF
   mov r1,0x02000000
   v32mov H32(16,0), r1
   v32bic -, H32(16,0), H32(0,0)+r0  IFNZ SETF

   mov r1,0x000f0f0f
   v32and H32(16,0),   H32(0,0)+r0, r1     IFNZ
   mov r1,0x00f0f0f0
   v32and H32(17,0),   H32(0,0)+r0, r1     IFNZ
   mov r1,0xff000000
   v32shl H32(16,0),   H32(16,0), 4        IFNZ
   v32lsr H32(17,0),   H32(17,0), 4        IFNZ

   v32and H32(0,0)+r0, H32(0,0)+r0, r1     IFNZ
   v32or  H32(0,0)+r0, H32(0,0)+r0, H32(16,0) IFNZ
   v32or  H32(0,0)+r0, H32(0,0)+r0, H32(17,0) IFNZ

; Look for words with flipbit=1, diffbit=1
   vbitplanes -, 0x5555 SETF
   mov r1,0x01000000
   v32and -, H32(0,0)+r0, r1  IFNZ SETF
   mov r1,0x02000000
   v32and -, H32(0,0)+r0, r1  IFNZ SETF

   mov r1, 0x00030303
   v32and H32(16,0),   H32(0,0)+r0, r1     IFNZ
   v32shl H32(16,0),   H32(16,0),   3      IFNZ
   mov r1, 0x00040404
   v32and H32(17,0),   H32(0,0)+r0, r1       IFNZ
   v32shl H32(17,0),   H32(17,0),   3      IFNZ

   v32add H32(0,0)+r0, H32(0,0)+r0, H32(16,0) IFNZ
   v32sub H32(0,0)+r0, H32(0,0)+r0, H32(17,0) IFNZ

   mov r1, 0x00070707

   v32and H32(16,0), H32(0,0)+r0, r1       IFNZ
   v32bic H32(0,0)+r0, H32(0,0)+r0, r1     IFNZ
   mov r2, 0x00010101
   v32eor H32(16,0), H32(16,0), r1         IFNZ
   v32add H32(16,0), H32(16,0), r2         IFNZ
   v32and H32(16,0), H32(16,0), r1         IFNZ
   v32or  H32(0,0)+r0, H32(0,0)+r0, H32(16,0) IFNZ

   ;v32eor H32(0,0)+r0, H32(0,0)+r0, r1      IFNZ   ; XXX off by one

   add r0,0x40
   cmp r0,0x400
   blt .loop


; Finished
   b lr
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   vc_image_putpixels_tf_rgba32
;;;
;;; SYNOPSIS
;;;   void vc_image_putpixels_tf_rgba32(
;;;      unsigned long       *dest,          /* r0 */
;;;      int                  width,         /* r1 */
;;;      int                  height,        /* r2 */
;;;      int                  pitch,         /* r3 */
;;;      void          const *coordinates,   /* r4 */
;;;      int                  length)        /* r5 */
;;;
;;; FUNCTION
;;;   does stuff.

.function vc_image_putpixels_tf_rgba32 ; {{{
         stm         r6-r9, (--sp)
         .cfa_push   {%r6,%r7,%r8,%r9}

         asr         r3, 2
         mov         r6, -16

.loop:   min         r7, r5, 16
         vmov        H16(0,0), -1
         v32shl      H16(0,0), H16(0,0), r7
         vbitplanes  H16(0,0), H16(0,0)            SETF
         vld         H32(3,0), (r4)
         vld         H32(4,0), (r4+64)

         veven       H32(1,0), H32(3,0), H32(4,0)
         vodd        H32(2,0), H32(3,0), H32(4,0)

         vadd        H16(4,0), H16(1,32), 32
         vand        H16(4,0), H16(4,0), -64
         v32mul.uu   -, H16(4,0), r3               CLRA UACC

         vlsr        H32(4,0), H16(1,0), 4
         vshl        H32(4,0), H32(4,0), 9
         vand        -, H16(1,32), 16              SETF
         vadd        H32(4,0), H32(4,0), 256       IFNZ
         vand        -, H16(1,32), 32              SETF
         veor        H32(4,0), H32(4,0), -512      IFNZ
         vand        -, H16(1,0), 16               SETF
         veor        H32(4,0), H32(4,0), 256       IFNZ

         vmov        -, H32(4,0)                   SACC

         vand        H16(4,0), H16(1,32), 12
         vshl        -, H16(4,0), 4                UACC
         vand        H16(4,0), H16(1,0), 12
         vshl        -, H16(4,0), 2                UACC
         vand        H16(4,0), H16(1,32), 3
         vshl        -, H16(4,0), 2                UACC
         vand        -, H16(1,0), 3                UACC

         vrsub       -, H16(1,0), r1 - 1           SETF
         vrsub       -, H16(1,32), r2 - 1          IFNC SETF
         vsubc       -, H16(0,0), 0                SETF

         vindexwritem H32(2,0), (r0)               UACC IFZ

         add         r4, 128
         addcmpbgt   r5, r6, 0, .loop

         ldm         r6-r9, (sp++)
         .cfa_pop    {%r9,%r8,%r7,%r6}

         b lr
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_tile_store_32bit
;;;
;;; SYNOPSIS
;;;   void tformat_linear_tile_store_32bit(
;;;      unsigned long *dst32,
;;;      int width,
;;;      int height,
;;;      int stride
;;;   );
;;;
;;; FUNCTION
;;;   Take a traditional linear-order 16x16 block and reorder it and store it
;;;   as linear microtiles. The stride parameter is measured in pixels
;;;   (assuming 32bit textures).
;;;   The width and height are needed to make sure we don't write too much.
;;;   These are also measured in pixels.

.function tformat_linear_tile_store_32bit ; {{{

      mov      r4, 0
      mov      r5, -1
.loop:
      add      r5, 1
      v32interl   V32(16,0), V32(0,0)+r4, V32(0,2)+r4
      v32interl   V32(16,1), V32(0,1)+r4, V32(0,3)+r4
      v32interh   V32(16,2), V32(0,0)+r4, V32(0,2)+r4
      v32interh   V32(16,3), V32(0,1)+r4, V32(0,3)+r4
      add      r4, 4
      v32interl   V32(48,0)+r5, V32(16,0), V32(16,1)
      cmp      r4, 16
      v32interh   V32(48,4)+r5, V32(16,0), V32(16,1)
      v32interl   V32(48,8)+r5, V32(16,2), V32(16,3)
      v32interh   V32(48,12)+r5, V32(16,2), V32(16,3)
      bne      .loop

      mov r5, r0
      mov r0, r1
      mov r1, r5

      lsr r0, 2
      mul r4, r2, r3
      add r4, r1
      mov r5, 64
      shl r3, 2
      vst      V32(48,0++),  (r1+=r5)          REP r0
      addcmpbeq r1, r3, r4, done
      vst      V32(48,4++),  (r1+=r5)          REP r0
      addcmpbeq r1, r3, r4, done
      vst      V32(48,8++),  (r1+=r5)          REP r0
      addcmpbeq r1, r3, r4, done
      vst      V32(48,12++), (r1+=r5)          REP r0
      addcmpbeq r1, r3, r4, done

      bkpt   ; We should have finished by now
done:
      b        lr
.endfn ; }}}

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_tile_store_8bit
;;;
;;; SYNOPSIS
;;;   void tformat_linear_tile_store_8bit(
;;;      unsigned long *dst8,
;;;      int width,
;;;      int height,
;;;      int stride
;;;   );
;;;
;;; FUNCTION
;;;   Take a traditional linear-order 32x32 block and reorder it and store it
;;;   as linear microtiles. The stride parameter is measured in pixels
;;;   (assuming 8bit textures).
;;;   The width and height are needed to make sure we don't write too much.
;;;   These are also measured in pixels.

.function tformat_linear_tile_store_8bit ; {{{
      stm r6-r9, lr, (--sp)

      mov         r4, 0
      mov         r6, r0     ; Must use r0 for REP
      mov         r7, 64

      mul r8, r2, r3
      add r8, r6
      shl r3, 3

.start:
      mov         r5, 0
.loop:
      v32interl   V32(32,0)+r5, V32(0,0)+r4, V32(0,1)+r4
      v32interh   V32(32,4)+r5, V32(0,0)+r4, V32(0,1)+r4
      v32interl   V32(32,8)+r5, V32(16,0)+r4, V32(16,1)+r4
      v32interh   V32(32,12)+r5, V32(16,0)+r4, V32(16,1)+r4

      add      r5, 1
      add      r4, 2
      cmp      r5, 4
      bne      .loop

      lsr r0, r1, 3
      min r0, 4

      mov r9, r6
      v32st    V32(32,0++), (r6+=r7)          REP r0
      addcmpbeq r6, r3, r8, done
      v32st    V32(32,4++), (r6+=r7)          REP r0
      addcmpbeq r6, r3, r8, done
      v32st    V32(32,8++), (r6+=r7)          REP r0
      addcmpbeq r6, r3, r8, done
      v32st    V32(32,12++),(r6+=r7)          REP r0
      addcmpbeq r6, r3, r8, done

      bkpt   ; We should have finished by now
done:
      add r6, r9, 256
      add r8, 256
      ; Check if width is more than 32, and if so convert the other subtile
      sub      r1, 32
      cmp      r1, 0
      bgt      .start

      ldm r6-r9, pc, (sp++)
.endfn ; }}}

;;; *****************************************************************************
;;; NAME
;;;   tformat_linear_tile_load_32bit
;;;
;;; SYNOPSIS
;;;   void tformat_linear_tile_load_32bit(unsigned long const *src32, int stride);
;;;
;;; FUNCTION
;;;   Load a linaer tile subtile and re-order it into traditional linear order
;;;   results are in H32(0++,0)
;;;   This code is almost identical to the t-format case, but a stride is
;;;   provided.
;;;   XXX for very small images, this will load more than it has to.
;;;   The stride parameter is measured in pixels (assuming 32bit textures).

.function tformat_linear_tile_load_32bit ; {{{
      mov      r2, 64
      vld      V32(48,0++), (r0+=r2)               REP 4
      addscale r0, r1<<2
      vld      V32(48,4++), (r0+=r2)               REP 4
      addscale r0, r1<<2
      vld      V32(48,8++), (r0+=r2)               REP 4
      addscale r0, r1<<2
      vld      V32(48,12++), (r0+=r2)               REP 4

      mov      r0, 0
      mov      r1, -64
.loop:
      add      r1, 64
      v32interl   H32(16,0), H32(48,0)+r0, H32(50,0)+r0
      v32interl   H32(17,0), H32(49,0)+r0, H32(51,0)+r0
      v32interh   H32(18,0), H32(48,0)+r0, H32(50,0)+r0
      v32interh   H32(19,0), H32(49,0)+r0, H32(51,0)+r0
      add      r0, 64 * 4
      v32interl   H32(0,0)+r1, H32(16,0), H32(17,0)
      cmp      r0, 64 * 16
      v32interh   H32(4,0)+r1, H32(16,0), H32(17,0)
      v32interl   H32(8,0)+r1, H32(18,0), H32(19,0)
      v32interh   H32(12,0)+r1, H32(18,0), H32(19,0)
      bne      .loop
      b        lr
.endfn ; }}}





;;; *****************************************************************************
;;; NAME
;;;   tformat_blit_row
;;;
;;; SYNOPSIS
;;;   void tformat_blit_row(
;;;      void *dest,
;;;      int   destdir0,
;;;      int   destdir1,
;;;      const void *srcbl,
;;;      const void *srcbr,
;;;      int   srctdir,
;;;      const void *srctl,
;;;      const void *srctr,
;;;      int   srcbdir,
;;;      int   offsetx,
;;;      int   offsety,
;;;      int   count);
;;;
;;; FUNCTION
;;;   Load a linaer tile subtile and re-order it into traditional linear order
;;;   results are in H32(0++,0)
;;;   This code is almost identical to the t-format case, but a stride is
;;;   provided.
;;;   XXX for very small images, this will load more than it has to.
;;;   The stride parameter is measured in pixels (assuming 32bit textures).

.function tformat_blit_row
   .define r_x_offset, r6   ;0-3
   .define r_y_offset, r7
   .define r_bl,       r8   ;Read address for bottom-left subtile
   .define r_br,       r9
   .define r_tl,       r10
   .define r_tr,       r11
   .define r_right,    r12
   .define r_count,    r13
   .define r_store,    r14
   .define r_tdir,     r15   ;4096 if this row of tiles is left-to-right, otherwise -4096
   .define r_bdir,     r16
   .define r_sdir0,    r17   ;Displacement from this subtile to the next
   .define r_sdir1,    r18   ;Displacement from the next subtile to the one after
   .define r_ldif,     r19
   .define r_rdif,     r20

   stm r6-r23,lr,(--sp)

   mov   r_store,    r0
   mov   r_sdir0,    r1
   mov   r_sdir1,    r2
   mov   r_bl,       r3
   mov   r_br,       r4
   mov   r_bdir,     r5
   ld    r_tl,       (sp+76+0)
   ld    r_tr,       (sp+76+4)
   ld    r_tdir,     (sp+76+8)
   ld    r_x_offset, (sp+76+12)
   ld    r_y_offset, (sp+76+16)
   ld    r_count,    (sp+76+20)

; Calculate the permutation

   and r0, r_x_offset, 3
   add r1, pc, pcrel(v0123)
   v32ld  H32(0,0), r1
   v32add H32(0,0), H32(0,0), r0
   v32sub -, H32(0,0), 4              SETF
   v32add H32(0,0), H32(0,0), 0x40-4  IFNN  ; Advance one row if we've fallen off the right

   and r0, r_y_offset, 3
   shl r0, 2
   add r1, pc, pcrel(v048c)
   v32ld  H32(1,0), r1
   v32add H32(1,0), H32(1,0), r0
   v32sub -, H32(1,0), 16              SETF
   v32add H32(1,0), H32(1,0), 0x140-16 IFNN  ; Advance five rows if we've fallen off the top

   v32add H32(0,0), H32(0,0), H32(1,0)

; Store permutation on the stack
   sub sp, 64
   mov r0, sp
   v32st H32(0,0), r0

; Now only interested in x/y offset measured in whole microtiles
   lsr r_x_offset, 2
   lsr r_y_offset, 2

; Add this offset to the read pointers
   shl r0, r_y_offset, 2
   or  r0, r_x_offset
   shl r0, 6
   add r_bl, r0
   add r_br, r0
   add r_tl, r0
   add r_tr, r0

blit_subtile_row:
   sub    r_ldif, r_tl, r_bl
   sub    r_ldif, 1024
   sub    r_rdif, r_tr, r_br
   sub    r_rdif, 1024

   mov    r1, r_bl
   sub    r2, r_br, 256
   mov    r4, 4
   mov    r5, 0
load_5x5_loop:
   cmp    r_y_offset, r4
   add.eq r1, r_ldif
   add.eq r2, r_rdif

   mov    r0, r1

   v32ld  H32(0,0)+r5, (r0)
   cmp    r_x_offset, 3
   mov.eq r0, r2
   v32ld  H32(1,0)+r5, (r0+64)
   cmp    r_x_offset, 2
   mov.eq r0, r2
   v32ld  H32(2,0)+r5, (r0+128)
   cmp    r_x_offset, 1
   mov.eq r0, r2
   v32ld  H32(3,0)+r5, (r0+192)
   cmp    r_x_offset, 0
   mov.eq r0, r2
   v32ld  H32(4,0)+r5, (r0+256)

   add    r1, 256
   add    r2, 256
   add    r5, 0x140
   addcmpbge r4,-1,0,load_5x5_loop

   ld r0,(sp)
   v32mov V32(32,0),  V32(0,0)+r0
   ld r1,(sp+4)
   v32mov V32(48,0),  V32(16,0)+r0
   ld r2,(sp+8)
   v32mov V32(32,1),  V32(0,0)+r1
   ld r3,(sp+12)
   v32mov V32(48,1),  V32(16,0)+r1
   ld r0,(sp+16)
   v32mov V32(32,2),  V32(0,0)+r2
   ld r1,(sp+20)
   v32mov V32(48,2),  V32(16,0)+r2
   ld r2,(sp+24)
   v32mov V32(32,3),  V32(0,0)+r3
   v32mov V32(48,3),  V32(16,0)+r3
   ld r3,(sp+28)
   v32mov V32(32,4),  V32(0,0)+r0
   v32mov V32(48,4),  V32(16,0)+r0
   ld r0,(sp+32)
   v32mov V32(32,5),  V32(0,0)+r1
   v32mov V32(48,5),  V32(16,0)+r1
   ld r1,(sp+36)
   v32mov V32(32,6),  V32(0,0)+r2
   v32mov V32(48,6),  V32(16,0)+r2
   ld r2,(sp+40)
   v32mov V32(32,7),  V32(0,0)+r3
   v32mov V32(48,7),  V32(16,0)+r3
   ld r3,(sp+44)
   v32mov V32(32,8),  V32(0,0)+r0
   v32mov V32(48,8),  V32(16,0)+r0
   ld r0,(sp+48)
   v32mov V32(32,9),  V32(0,0)+r1
   v32mov V32(48,9),  V32(16,0)+r1
   ld r1,(sp+52)
   v32mov V32(32,10), V32(0,0)+r2
   v32mov V32(48,10), V32(16,0)+r2
   ld r2,(sp+56)
   v32mov V32(32,11), V32(0,0)+r3
   v32mov V32(48,11), V32(16,0)+r3
   ld r3,(sp+60)
   v32mov V32(32,12), V32(0,0)+r0
   v32mov V32(48,12), V32(16,0)+r0
   v32mov V32(32,13), V32(0,0)+r1
   v32mov V32(48,13), V32(16,0)+r1
   v32mov V32(32,14), V32(0,0)+r2
   v32mov V32(48,14), V32(16,0)+r2
   v32mov V32(32,15), V32(0,0)+r3
   v32mov V32(48,15), V32(16,0)+r3

   mov r1,64
   mov r0, r_store
   v32st  H32(32++,0),   (r0+=r1) REP 4
   add r0, 256
   v32st  H32(37++,0),   (r0+=r1) REP 4
   add r0, 256
   v32st  H32(42++,0),   (r0+=r1) REP 4
   add r0, 256
   v32st  H32(47++,0),   (r0+=r1) REP 4

   add    r0,   r_bl, r_bdir
   mov    r_bl, r_br
   mov    r_br, r0
   add    r0,   r_tl, r_tdir
   mov    r_tl, r_tr
   mov    r_tr, r0

   add    r_store, r_sdir0
   mov    r0,      r_sdir0
   mov    r_sdir0, r_sdir1
   mov    r_sdir1, r0

   addcmpbgt r_count, -1, 0, blit_subtile_row

; Remember to pop permumation off the stack
   add sp, 64
   ldm r6-r23,pc,(sp++)
.align 4
v0123:
   .word 0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3
v048c:
   .word 0,0,0,0,4,4,4,4,8,8,8,8,12,12,12,12
.endfn



;;; Used by the load_tformat functions

;d currently has to be 0 or 32
.macro LOAD_TFORMAT, d
      mov      r2, 64
      vld      V32(48,0++), (r0+=r2)               REP 4
      addscale r0, r1<<2
      vld      V32(48,4++), (r0+=r2)               REP 4
      addscale r0, r1<<2
      vld      V32(48,8++), (r0+=r2)               REP 4
      addscale r0, r1<<2
      vld      V32(48,12++), (r0+=r2)               REP 4

      mov      r0, 0
      mov      r1, -64
.loop:
      add      r1, 64
      v32interl   H32(16,0), H32(48,0)+r0, H32(50,0)+r0
      v32interl   H32(17,0), H32(49,0)+r0, H32(51,0)+r0
      v32interh   H32(18,0), H32(48,0)+r0, H32(50,0)+r0
      v32interh   H32(19,0), H32(49,0)+r0, H32(51,0)+r0
      add      r0, 64 * 4
      v32interl   H32(d+0,0)+r1, H32(16,0), H32(17,0)
      cmp      r0, 64 * 16
      v32interh   H32(d+4,0)+r1, H32(16,0), H32(17,0)
      v32interl   H32(d+8,0)+r1, H32(18,0), H32(19,0)
      v32interh   H32(d+12,0)+r1, H32(18,0), H32(19,0)
      bne      .loop
.endm


;;; *****************************************************************************
;;; NAME
;;;   load_tformat_opaque
;;;
;;; SYNOPSIS
;;;   void load_tformat_opaque(unsigned long const *src32, int stride);
;;;
;;; FUNCTION
;;;   Load RGBX32 tformat/linear tile. Set the alpha component to 255.

.function load_tformat_opaque ; {{{
	LOAD_TFORMAT(0)
	vmov H8(0++,48), 0xff   REP 16
	b lr
.endfn ; }}}



;;; *****************************************************************************
;;; NAME
;;;   load_tformat_4444_to_8888
;;;
;;; SYNOPSIS
;;;   void load_tformat_4444_to_8888(unsigned long const *src32, int stride);
;;;
;;; FUNCTION
;;;   Load RGBA16 tformat/linear tile and converts to RGBA8888.
;;;   Stored in H32(0++,0) and H32(32++,0)

.function load_tformat_4444_to_8888 ; {{{
   LOAD_TFORMAT(0)

   v16interl H16(16++,0), H16(0++,0), H16(0++,32) REP 16
   v16interh H16(48++,0), H16(0++,0), H16(0++,32) REP 16

   v16lsr    H8(0++,0),   H8(16++,16), 4          REP 16  ;red
   v16and    H8(0++,16),  H8(16++,16), 0xf        REP 16  ;green
   v16lsr    H8(0++,32),  H8(16++,0),  4          REP 16  ;blue
   v16and    H8(0++,48),  H8(16++,0),  0xf        REP 16  ;alpha
   v16mul    H16(0++,0),  H16(0++,0),  17         REP 16
   v16mul    H16(0++,32), H16(0++,32), 17         REP 16

   v16lsr    H8(32++,0),   H8(48++,16), 4          REP 16  ;red
   v16and    H8(32++,16),  H8(48++,16), 0xf        REP 16  ;green
   v16lsr    H8(32++,32),  H8(48++,0),  4          REP 16  ;blue
   v16and    H8(32++,48),  H8(48++,0),  0xf        REP 16  ;alpha
   v16mul    H16(32++,0),  H16(32++,0), 17         REP 16
   v16mul    H16(32++,32), H16(32++,32),17         REP 16
   b lr
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   load_tformat_5551_to_8888
;;;
;;; SYNOPSIS
;;;   void load_tformat_5551_to_8888(unsigned long const *src32, int stride);
;;;
;;; FUNCTION
;;;   Load RGBA5551 tformat/linear tile and converts to RGBA8888.
;;;   Stored in H32(0++,0) and H32(32++,0)

.function load_tformat_5551_to_8888 ; {{{
   LOAD_TFORMAT(0)

   v16interl H16(16++,0), H16(0++,0), H16(0++,32) REP 16
   v16interh H16(48++,0), H16(0++,0), H16(0++,32) REP 16

   v16and      H16(0++,32), H16(16++,0), 0xf800     REP 16  ;red
   v16mulhd.uu H8 (0++,0),  H16(0++,32), 0x108      REP 16
   v16and      H16(0++,32), H16(16++,0), 0x07c0     REP 16  ;green
   v16mulhd.uu H8 (0++,16), H16(0++,32), 0x2100     REP 16
   v16shl      H16(0++,32), H16(16++,0), 10         REP 16  ;blue
   v16and      H16(0++,32), H16(0++,32), 0xf800     REP 16
   v16mulhd.uu H8 (0++,32), H16(0++,32), 0x108      REP 16
   v16and      H8 (0++,48), H16(16++,0), 1          REP 16  ;alpha
   v16rsub     H8 (0++,48), H8 (0++,48), 0          REP 16

   v16and      H16(32++,32), H16(48++,0),  0xf800   REP 16  ;red
   v16mulhd.uu H8 (32++,0),  H16(32++,32), 0x108    REP 16
   v16and      H16(32++,32), H16(48++,0),  0x07c0   REP 16  ;green
   v16mulhd.uu H8 (32++,16), H16(32++,32), 0x2100   REP 16
   v16shl      H16(32++,32), H16(48++,0),  10       REP 16  ;blue
   v16and      H16(32++,32), H16(32++,32), 0xf800   REP 16
   v16mulhd.uu H8 (32++,32), H16(32++,32), 0x108    REP 16
   v16and      H8 (32++,48), H16(48++,0),  1        REP 16  ;alpha
   v16rsub     H8 (32++,48), H8 (32++,48), 0        REP 16
   b lr
.endfn ; }}}

;;; *****************************************************************************
;;; NAME
;;;   load_tformat_565_to_8888
;;;
;;; SYNOPSIS
;;;   void load_tformat_565_to_8888(unsigned long const *src32, int stride);
;;;
;;; FUNCTION
;;;   Load RGB565 tformat/linear tile and converts to RGBA8888.
;;;   Stored in H32(0++,0) and H32(32++,0)

.function load_tformat_565_to_8888 ; {{{
   LOAD_TFORMAT(0)

   v16interl H16(16++,0), H16(0++,0), H16(0++,32) REP 16
   v16interh H16(48++,0), H16(0++,0), H16(0++,32) REP 16


   v16and      H16(0++,32), H16(16++,0), 0xf800     REP 16  ;red
   v16mulhd.uu H8 (0++,0),  H16(0++,32), 0x108      REP 16
   v16and      H16(0++,32), H16(16++,0), 0x07e0     REP 16  ;green
   v16mulhd.uu H8 (0++,16), H16(0++,32), 0x2080     REP 16
   v16shl      H16(0++,32), H16(16++,0), 11         REP 16  ;blue
   v16mulhd.uu H8 (0++,32), H16(0++,32), 0x108      REP 16
   v16mov      H8 (0++,48), 0xff                    REP 16  ;alpha


   v16and      H16(32++,32), H16(48++,0),  0xf800   REP 16  ;red
   v16mulhd.uu H8 (32++,0),  H16(32++,32), 0x108    REP 16
   v16and      H16(32++,32), H16(48++,0),  0x07e0   REP 16  ;green
   v16mulhd.uu H8 (32++,16), H16(32++,32), 0x2080   REP 16
   v16shl      H16(32++,32), H16(48++,0),  11       REP 16  ;blue
   v16mulhd.uu H8 (32++,32), H16(32++,32), 0x108    REP 16
   v16mov      H8 (32++,48), 0xff                   REP 16  ;alpha
   b lr
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   raster_store_ya
;;;
;;; SYNOPSIS
;;;   unsigned long * raster_store_ya(
;;;      int                  ycount,
;;;      unsigned long       *dst32,
;;;      int                  pitch,
;;;      int                  trimleft,
;;;      int                  trimright,
;;;      int                  ystart);
;;;
;;; FUNCTION
;;;   Extract just the luminance (i.e. red) and alpha components and store them
;;;   in raster order, with the usual trim values.

.function raster_store_ya ; {{{
         asr      r3, 2
         asr      r4, 2
         max      r3, 0
         max      r4, 0
         rsub     r4, 16
         sub      r4, r3          ; Number of pixels across
         v32mov   H32(63,0), -1
         v32shl   H32(63,0), H32(63,0), r4
         vbitplanes -, H32(63,0) SETF

         shl      r5, 6
         add      r5, r3

         vmov     H8(16++,0)+r5,  H8(0++, 0)+r5    IFZ REP r0      ;Red
         vmov     H8(16++,16)+r5, H8(0++, 48)+r5   IFZ REP r0      ;Alpha
         v16st    H16(16++,0)+r5, (r1+=r2)     IFZ REP r0

         addscale r0, r1, r4<<1
         b        lr
.endfn ; }}}

;;; *****************************************************************************
;;; NAME
;;;   raster_store_y
;;;
;;; SYNOPSIS
;;;   unsigned long * raster_store_y(
;;;      int                  ycount,
;;;      unsigned long       *dst32,
;;;      int                  pitch,
;;;      int                  trimleft,
;;;      int                  trimright,
;;;      int                  ystart);
;;;
;;; FUNCTION
;;;   Extract just the luminance (i.e. red) component and store it
;;;   in raster order, with the usual trim values.

.function raster_store_y ; {{{
         asr      r3, 2
         asr      r4, 2
         max      r3, 0
         max      r4, 0
         rsub     r4, 16
         sub      r4, r3          ; Number of pixels across
         max      r4, 0
         v32mov   H32(63,0), -1
         v32shl   H32(63,0), H32(63,0), r4
         vbitplanes -, H32(63,0) SETF

         shl      r5, 6
         add      r5, r3

         vmov    H8(16++,0)+r5,  H8(0++, 0)+r5   IFZ REP r0      ;Red
         v8st    H8(16++,0)+r5, (r1+=r2)     IFZ REP r0

         add      r0, r1, r4
         b        lr
.endfn ; }}}

;;; *****************************************************************************
;;; NAME
;;;   raster_store_a
;;;
;;; SYNOPSIS
;;;   unsigned long * raster_store_a(
;;;      int                  ycount,
;;;      unsigned long       *dst32,
;;;      int                  pitch,
;;;      int                  trimleft,
;;;      int                  trimright,
;;;      int                  ystart);
;;;
;;; FUNCTION
;;;   Extract just the alpha component and store it
;;;   in raster order, with the usual trim values.

.function raster_store_a ; {{{
         asr      r3, 2
         asr      r4, 2
         max      r3, 0
         max      r4, 0
         rsub     r4, 16
         sub      r4, r3          ; Number of pixels across
         max      r4, 0
         v32mov   H32(63,0), -1
         v32shl   H32(63,0), H32(63,0), r4
         vbitplanes -, H32(63,0) SETF

         shl      r5, 6
         add      r5, r3

         vmov     H8(16++,0)+r5, H8(0++, 48)+r5 IFZ REP r0      ;Alpha
         v8st    H8(16++,0)+r5, (r1+=r2)     IFZ REP r0

         add      r0, r1, r4
         b        lr
.endfn ; }}}

.function raster_store_a2 ; {{{
   stm r6, lr, (--sp)
   shl r3, 1
   shl r4, 1
   stm r0-r5, (--sp)
   sub r4, 64
   bl raster_store_a
   mov r6, r0
   ldm r0-r5, (sp++)
   sub r3, 64
   mov r1, r6
   v32mov H32(0++,0), H32(32++,0) REP 16
   bl raster_store_a
   ldm r6, pc, (sp++)
.endfn ; }}}

.function raster_store_y2 ; {{{
   stm r6, lr, (--sp)
   shl r3, 1
   shl r4, 1
   stm r0-r5, (--sp)
   sub r4, 64
   bl raster_store_y
   mov r6, r0
   ldm r0-r5, (sp++)
   sub r3, 64
   mov r1, r6
   v32mov H32(0++,0), H32(32++,0) REP 16
   bl raster_store_y
   ldm r6, pc, (sp++)
.endfn ; }}}

.function raster_store_ya2 ; {{{
   stm r6, lr, (--sp)
   shl r3, 1
   shl r4, 1
   stm r0-r5, (--sp)
   sub r4, 64
   bl raster_store_ya
   mov r6, r0
   ldm r0-r5, (sp++)
   sub r3, 64
   mov r1, r6
   bl raster_store_ya
   ldm r6, pc, (sp++)
.endfn ; }}}

.function tformat_raster_store4_2 ; {{{
   stm r6, lr, (--sp)
   shl r3, 1
   shl r4, 1
   stm r0-r5, (--sp)
   sub r4, 64
   bl tformat_raster_store4
   mov r6, r0
   ldm r0-r5, (sp++)
   sub r3, 64
   mov r1, r6
   v32mov H32(0++,0), H32(32++,0) REP 16
   bl tformat_raster_store4
   ldm r6, pc, (sp++)
.endfn ; }}}

.function tformat_raster_store_rgb888_2 ; {{{
   stm r6, lr, (--sp)
   shl r3, 1
   shl r4, 1
   stm r0-r5, (--sp)
   sub r4, 64
   bl tformat_raster_store_rgb888
   mov r6, r0
   ldm r0-r5, (sp++)
   sub r3, 64
   mov r1, r6
   v32mov H32(0++,0), H32(32++,0) REP 16
   bl tformat_raster_store_rgb888
   ldm r6, pc, (sp++)
.endfn ; }}}




;;; *****************************************************************************
;;; NAME
;;;   tformat_wide_raster_to_tformat
;;;
;;; SYNOPSIS
;;;   unsigned long * tformat_wide_raster_to_tformat(
;;;      int                  width,            /* r0 */
;;;      unsigned long       *dst32,            /* r1 */
;;;      unsigned long       *src32,            /* r2 */
;;;      int                  dpitch,           /* r3 */
;;;      int                  spitch,           /* r4 */
;;;      int                  swap_flags);      /* r5 */
;;;
;;; FUNCTION
;;;   Load a <width>x4 pixels of raster data and store out to tformat.
;;;   This function attempts to be friendly on SDRAM bank aliasing.

.function tformat_wide_raster_to_tformat ; {{{
         st       r6, (--sp)
         .cfa_push {%r6}

         add      r0, 15
         lsr      r0, 4
         mov      r6, 64

         vld      V32(0,0++), (r2+=r6)                   REP r0
         addscale r2, r4 * 2
         vld      V32(16,0++), (r2+=r6)                  REP r0
         sub      r2, r4
         vld      V32(0,8++), (r2+=r6)                   REP r0
         addscale r2, r4 * 2
         vld      V32(16,8++), (r2+=r6)                  REP r0
         add      r2, r4

         btest    r5, 2
         beq      2f
         ; rotate to move alpha from LSB to MSB
         vror     V32(0,0++), V32(0,0++), 8              REP r0
         vror     V32(0,8++), V32(0,8++), 8              REP r0
         vror     V32(16,0++), V32(16,0++), 8            REP r0
         vror     V32(16,8++), V32(16,8++), 8            REP r0
         
2:
         btest    r5, 1
         beq      1f
         ; swap r+b
         veor     H8(0++,0), H8(0++,0), H8(0++,32)       REP 32
         veor     H8(0++,32), H8(0++,0), H8(0++,32)      REP 32
         veor     H8(0++,0), H8(0++,0), H8(0++,32)       REP 32
1:

         mov      r2, 0
         mov      r4, 0
.loop0:  veven    V32(32,0)+r2, V32(0,0)+r4,  V32(0,1)+r4
         vodd     V32(48,0)+r2, V32(0,0)+r4,  V32(0,1)+r4
         veven    V32(32,1)+r2, V32(0,2)+r4,  V32(0,3)+r4
         vodd     V32(48,1)+r2, V32(0,2)+r4,  V32(0,3)+r4

         veven    V32(32,8)+r2, V32(16,0)+r4,  V32(16,1)+r4
         vodd     V32(48,8)+r2, V32(16,0)+r4,  V32(16,1)+r4
         veven    V32(32,9)+r2, V32(16,2)+r4,  V32(16,3)+r4
         vodd     V32(48,9)+r2, V32(16,2)+r4,  V32(16,3)+r4
         add      r4, 4
         addcmpbne r2, 2, 8, .loop0

         mov      r2, 0
.loop1:  veven    V32(0,0)+r2,  V32(32,0)+r2, V32(32,1)+r2
         veven    V32(0,1)+r2,  V32(48,0)+r2, V32(48,1)+r2
         vodd     V32(0,2)+r2,  V32(32,0)+r2, V32(32,1)+r2
         vodd     V32(0,3)+r2,  V32(48,0)+r2, V32(48,1)+r2

         veven    V32(16,0)+r2, V32(32,2)+r2, V32(32,3)+r2
         veven    V32(16,1)+r2, V32(48,2)+r2, V32(48,3)+r2
         vodd     V32(16,2)+r2, V32(32,2)+r2, V32(32,3)+r2
         vodd     V32(16,3)+r2, V32(48,2)+r2, V32(48,3)+r2
         addcmpbne r2, 4, 16, .loop1

         mov      r2, 0
.stloop: vst      H32(0++,0)+r2, (r1+=r6)                REP 4
         add      r2, 64 * 4
         add      r1, r3
         eor      r3, 2048
         addcmpbne r0, -1, 0, .stloop
         ld       r6, (sp++)
         .cfa_pop {%r6}
         b        lr
.endfn ; }}}

;;; *****************************************************************************
;;; NAME
;;;   tformat_subtile_transpose_32bit
;;;
;;; SYNOPSIS
;;;   void tformat_subtile_transpose(unsigned long *dst32 /* r0 */, unsigned long const *src32 /* r1 */);
;;;
;;; FUNCTION
;;;   Load a t-format subtile, transpose and store

.function tformat_subtile_transpose_32bit ; {{{
      mov      r2, 64
      vld      H32(0++,0), (r1+=r2)               REP 16

      ; transpose each microtile

      v32mov   V32(16,0),  V32(0,0)
      v32mov   V32(16,1),  V32(0,4)
      v32mov   V32(16,2),  V32(0,8)
      v32mov   V32(16,3),  V32(0,12)

      v32mov   V32(16,4),  V32(0,1)
      v32mov   V32(16,5),  V32(0,5)
      v32mov   V32(16,6),  V32(0,9)
      v32mov   V32(16,7),  V32(0,13)

      v32mov   V32(16,8),  V32(0,2)
      v32mov   V32(16,9),  V32(0,6)
      v32mov   V32(16,10), V32(0,10)
      v32mov   V32(16,11), V32(0,14)

      v32mov   V32(16,12), V32(0,3)
      v32mov   V32(16,13), V32(0,7)
      v32mov   V32(16,14), V32(0,11)
      v32mov   V32(16,15), V32(0,15)

      ; transpose the microtiles within the tile

      v32mov   H32(0,0),   H32(16,0)
      v32mov   H32(1,0),   H32(20,0)
      v32mov   H32(2,0),   H32(24,0)
      v32mov   H32(3,0),   H32(28,0)

      v32mov   H32(4,0),   H32(17,0)
      v32mov   H32(5,0),   H32(21,0)
      v32mov   H32(6,0),   H32(25,0)
      v32mov   H32(7,0),   H32(29,0)

      v32mov   H32(8,0),   H32(18,0)
      v32mov   H32(9,0),   H32(22,0)
      v32mov   H32(10,0),  H32(26,0)
      v32mov   H32(11,0),  H32(30,0)

      v32mov   H32(12,0),  H32(19,0)
      v32mov   H32(13,0),  H32(23,0)
      v32mov   H32(14,0),  H32(27,0)
      v32mov   H32(15,0),  H32(31,0)

      vst      H32(0++,0), (r0+=r2)               REP 16

      b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_subtile_pair_transpose_16bit
;;;
;;; SYNOPSIS
;;;   void tformat_subtile_transpose(unsigned long *dst32a /* r0 */, unsigned long *dst32b /* r1 */, unsigned long const *src32a /* r2 */, unsigned long const *src32b /* r3 */);
;;;
;;; FUNCTION
;;;   Load a vertically-stacked t-format subtile pair, transpose and store

.function tformat_subtile_pair_transpose_16bit ; {{{
      mov      r5, 64

      vld      H32(0++,0),  (r2+=r5)               REP 4
      add      r2,r2,256
      vld      H32(16++,0), (r2+=r5)               REP 4
      add      r2,r2,256
      vld      H32(4++,0),  (r2+=r5)               REP 4
      add      r2,r2,256
      vld      H32(20++,0), (r2+=r5)               REP 4

      vld      H32(8++,0),  (r3+=r5)               REP 4
      add      r3,r3,256
      vld      H32(24++,0), (r3+=r5)               REP 4
      add      r3,r3,256
      vld      H32(12++,0), (r3+=r5)               REP 4
      add      r3,r3,256
      vld      H32(28++,0), (r3+=r5)               REP 4

      ; transpose each microtile

      v16mov   V16(32, 0),  V16(0, 0)
      v16mov   V16(32, 32), V16(0, 4)
      v16mov   V16(32, 1),  V16(0, 8)
      v16mov   V16(32, 33), V16(0, 12)
      v16mov   V16(32, 2),  V16(16, 0)
      v16mov   V16(32, 34), V16(16, 4)
      v16mov   V16(32, 3),  V16(16, 8)
      v16mov   V16(32, 35), V16(16, 12)

      v16mov   V16(32, 4), V16(0, 32)
      v16mov   V16(32, 36), V16(0, 36)
      v16mov   V16(32, 5), V16(0, 40)
      v16mov   V16(32, 37), V16(0, 44)
      v16mov   V16(32, 6), V16(16, 32)
      v16mov   V16(32, 38), V16(16, 36)
      v16mov   V16(32, 7), V16(16, 40)
      v16mov   V16(32, 39), V16(16, 44)

      v16mov   V16(32, 8),  V16(0, 1)
      v16mov   V16(32, 40), V16(0, 5)
      v16mov   V16(32, 9),  V16(0, 9)
      v16mov   V16(32, 41), V16(0, 13)
      v16mov   V16(32, 10), V16(16, 1)
      v16mov   V16(32, 42), V16(16, 5)
      v16mov   V16(32, 11), V16(16, 9)
      v16mov   V16(32, 43), V16(16, 13)

      v16mov   V16(32, 12), V16(0, 33)
      v16mov   V16(32, 44), V16(0, 37)
      v16mov   V16(32, 13), V16(0, 41)
      v16mov   V16(32, 45), V16(0, 45)
      v16mov   V16(32, 14), V16(16, 33)
      v16mov   V16(32, 46), V16(16, 37)
      v16mov   V16(32, 15), V16(16, 41)
      v16mov   V16(32, 47), V16(16, 45)

      v16mov   V16(48, 0),  V16(0, 2)
      v16mov   V16(48, 32), V16(0, 6)
      v16mov   V16(48, 1),  V16(0, 10)
      v16mov   V16(48, 33), V16(0, 14)
      v16mov   V16(48, 2),  V16(16, 2)
      v16mov   V16(48, 34), V16(16, 6)
      v16mov   V16(48, 3),  V16(16, 10)
      v16mov   V16(48, 35), V16(16, 14)

      v16mov   V16(48, 4),  V16(0, 34)
      v16mov   V16(48, 36), V16(0, 38)
      v16mov   V16(48, 5),  V16(0, 42)
      v16mov   V16(48, 37), V16(0, 46)
      v16mov   V16(48, 6),  V16(16, 34)
      v16mov   V16(48, 38), V16(16, 38)
      v16mov   V16(48, 7),  V16(16, 42)
      v16mov   V16(48, 39), V16(16, 46)

      v16mov   V16(48, 8),  V16(0, 3)
      v16mov   V16(48, 40), V16(0, 7)
      v16mov   V16(48, 9),  V16(0, 11)
      v16mov   V16(48, 41), V16(0, 15)
      v16mov   V16(48, 10), V16(16, 3)
      v16mov   V16(48, 42), V16(16, 7)
      v16mov   V16(48, 11), V16(16, 11)
      v16mov   V16(48, 43), V16(16, 15)

      v16mov   V16(48, 12), V16(0, 35)
      v16mov   V16(48, 44), V16(0, 39)
      v16mov   V16(48, 13), V16(0, 43)
      v16mov   V16(48, 45), V16(0, 47)
      v16mov   V16(48, 14), V16(16, 35)
      v16mov   V16(48, 46), V16(16, 39)
      v16mov   V16(48, 15), V16(16, 43)
      v16mov   V16(48, 47), V16(16, 47)

      ; transpose the microtiles within the tile

      v32mov   H32(0, 0),   H32(32, 0)
      v32mov   H32(16, 0),  H32(48, 0)
      v32mov   H32(1, 0),   H32(36, 0)
      v32mov   H32(17, 0),  H32(52, 0)
      v32mov   H32(2, 0),   H32(40, 0)
      v32mov   H32(18, 0),  H32(56, 0)
      v32mov   H32(3, 0),   H32(44, 0)
      v32mov   H32(19, 0),  H32(60, 0)

      v32mov   H32(4, 0),   H32(33, 0)
      v32mov   H32(20, 0),  H32(49, 0)
      v32mov   H32(5, 0),   H32(37, 0)
      v32mov   H32(21, 0),  H32(53, 0)
      v32mov   H32(6, 0),   H32(41, 0)
      v32mov   H32(22, 0),  H32(57, 0)
      v32mov   H32(7, 0),   H32(45, 0)
      v32mov   H32(23, 0),  H32(61, 0)

      v32mov   H32(8, 0),   H32(34, 0)
      v32mov   H32(24, 0),  H32(50, 0)
      v32mov   H32(9, 0),   H32(38, 0)
      v32mov   H32(25, 0),  H32(54, 0)
      v32mov   H32(10, 0),  H32(42, 0)
      v32mov   H32(26, 0),  H32(58, 0)
      v32mov   H32(11, 0),  H32(46, 0)
      v32mov   H32(27, 0),  H32(62, 0)

      v32mov   H32(12, 0),  H32(35, 0)
      v32mov   H32(28, 0),  H32(51, 0)
      v32mov   H32(13, 0),  H32(39, 0)
      v32mov   H32(29, 0),  H32(55, 0)
      v32mov   H32(14, 0),  H32(43, 0)
      v32mov   H32(30, 0),  H32(59, 0)
      v32mov   H32(15, 0),  H32(47, 0)
      v32mov   H32(31, 0),  H32(63, 0)

      vst      H32(0++,0),  (r0+=r5)               REP 4
      add      r0,r0,256
      vst      H32(16++,0), (r0+=r5)               REP 4
      add      r0,r0,256
      vst      H32(4++,0),  (r0+=r5)               REP 4
      add      r0,r0,256
      vst      H32(20++,0), (r0+=r5)               REP 4

      cmp      r4, 0
      beq      .out

      vst      H32(8++,0),  (r1+=r5)               REP 4
      add      r1,r1,256
      vst      H32(24++,0), (r1+=r5)               REP 4
      add      r1,r1,256
      vst      H32(12++,0), (r1+=r5)               REP 4
      add      r1,r1,256
      vst      H32(28++,0), (r1+=r5)               REP 4

.out:
      b        lr
.endfn ; }}}
.FORGET
