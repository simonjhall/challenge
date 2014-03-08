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

   ; R = 1.164*(Y-16)                 + 1.596*(V-128)
   ; G = 1.164*(Y-16) - 0.391*(U-128) - 0.813*(V-128)
   ; B = 1.164*(Y-16) + 2.018*(U-128)

   ; so

   ; R = 1.164*Y           + 1.596*V  - (1.164*16             + 1.596*128)
   ; G = 1.164*Y - 0.391*U - 0.813*V  - (1.164*16 - 0.391*128 - 0.813*128)
   ; B = 1.164*Y + 2.018*U            - (1.164*16 + 2.018*128            )

.define WANT_JFIF_COLOURSPACE
.ifdef WANT_JFIF_COLOURSPACE
; full range yuv->rgb for jfif
; R = Y + 1.402 (Cr-128)
; G = Y - 0.34414 (Cb-128) - 0.71414 (Cr-128)
; B = Y + 1.772 (Cb-128)

.define YSCALE,    8192  ;;  256 * 32 * 1.0000000
.define bUSCALE,  16531  ;;  256 * 32 * 2.0180000
.define gUSCALE,  -2819  ;; -256 * 32 * 0.3441400
.define rVSCALE,  11485  ;;  256 * 32 * 1.4020000
.define gVSCALE,  -5850  ;; -256 * 32 * 0.7141400

.define RBIAS,     -179  ;; -(YSCALE * 0                 + rVSCALE * 128) / 256
.define GBIAS,      135  ;; -(YSCALE * 0 - gUSCALE * 128 - gVSCALE * 128) / 256
.define BBIAS,     -258  ;; -(YSCALE * 0 + bUSCALE * 128                ) / 256
.else
.define YSCALE,    9539  ;;  256 * 32 * 1.1643835
.define bUSCALE,  16525  ;;  256 * 32 * 2.0172321
.define gUSCALE,  -3209  ;; -256 * 32 * 0.3917623
.define rVSCALE,  13075  ;;  256 * 32 * 1.5960267
.define gVSCALE,  -6659  ;; -256 * 32 * 0.8129676

.define RBIAS, -223     ;; -(YSCALE * 16                 + rVSCALE * 128) / 256
.define GBIAS,  135     ;; -(YSCALE * 16 - gUSCALE * 128 - gVSCALE * 128) / 256
.define BBIAS, -277     ;; -(YSCALE * 16 + rUSCALE * 128                ) / 256
.endif
   ; R = (YSCALE*Y             + rVSCALE*V) / 256 + RBIAS
   ; G = (YSCALE*Y - gUSCALE*U - gVSCALE*V) / 256 + GBIAS
   ; B = (YSCALE*Y + bUSCALE*U            ) / 256 + BBIAS

.if 1
.define RMASK, 0xF8
.define GMASK, 0xFC
.define BMASK, 0xF8
.else
.define RMASK, 0xE0
.define GMASK, 0xE0
.define BMASK, 0xE0
.endif

.ifndef RGB888
.warn "We don't support blue-first RGB888 anymore."
.endif


;;; *****************************************************************************

; TODO: replace these with the ones from vcinclude/common.inc
.macro function, name
.define funcname, name
.pushsect .text
.section .text$\&funcname,"text"
.globl funcname
funcname:
.cfa_bf funcname
.endm

.macro endfunc
.cfa_ef
.size funcname,.-funcname
.popsect
.undef funcname
.endm

;;; *****************************************************************************
;;; NAME
;;;   yuv420ci_to_rgb_load_32x16
;;;   yuv420ci_to_rgb_load_32x16_misaligned
;;;   yuv420_to_rgb_load_32x16
;;;   yuv420_to_rgb_load_32x16_misaligned
;;;
;;; SYNOPSIS
;;;   void yuv420_to_rgb_load_32x16(
;;;      unsigned char const *yptr,          r0
;;;      unsigned char const *uptr,          r1
;;;      unsigned char const *vptr,          r2
;;;      int                  src_pitch,     r3
;;;      int                  src_pitch2,    r4
;;;      void               (*commit)(unsigned char *dest, int pitch, ...),  r5
;;;      unsigned char       *dest,          (sp)
;;;      int                  pitch,         (sp+4)
;;;      ...                                 (sp+8)
;;;
;;; FUNCTION
;;;   Load 32x16 block of YUV data, do a partial conversion to RGB, and link a
;;;   function (eg. yuv_to_rgb888_store_32x16) to finalise and store the result.
;;;
;;; RETURNS
;;;    -

function yuv420ci_to_rgb_load_32x16 ; {{{
.globl yuv420ci_to_rgb_load_32x16_misaligned
yuv420ci_to_rgb_load_32x16_misaligned: ; legacy alias
      vld      HX(0++,0), (r0+=r3) REP 16             ;; Y (split into odds and evens)
      vld      HX(0++,32), (r1+=r4) REP 8             ;; UV (interleaved)
      b        yuv420_to_rgb_load_32x16_loaded
endfunc ; }}}

function yuvuv420_to_rgb_load_32x16 ; {{{
      add r1,r2   ; Bodge the 32 increment in uv pointer by summing u and v pointers
      vld      HX(0++,0), (r0+=r3) REP 16             ;; Y (split into odds and evens)
      vld      HX(0++,32), (r1+=r4) REP 8             ;; UV
      sub r1,r2   ; Undo bodge
      b        yuv420_to_rgb_load_32x16_loaded
endfunc ; }}}

function yuv420_to_rgb_load_32x16 ; {{{
.globl yuv420_to_rgb_load_32x16_misaligned
yuv420_to_rgb_load_32x16_misaligned: ; legacy alias
      vld      HX(0++,0), (r0+=r3) REP 16             ;; Y (split into odds and evens)
      vld      H(0++,32), (r1+=r4) REP 8              ;; U
      vld      H(0++,48), (r2+=r4) REP 8              ;; V

yuv420_to_rgb_load_32x16_loaded:
      vmulm    HX(48++,0),  H(0++,32), bUSCALE REP 8  ;; bU = U * 2.018
      vmulm    HX(56++,0),  H(0++,48), rVSCALE REP 8  ;; rV = V * 1.596
      vmulm    HX(48++,32), H(0++,32), gUSCALE REP 8  ;; gU = U * 0.391
      vmulm    HX(56++,32), H(0++,48), gVSCALE REP 8  ;; gV = V * 0.813

      vadd     HX(56++,32), HX(48++,32), HX(56++,32) REP 8 ;; gUV = gU + gV

      vmulm    HX(0++,32), H(0++,16), YSCALE REP 16   ;; Y *= 1.164
      vmulm    HX(0++,0),  H(0++,0),  YSCALE REP 16   ;; Y *= 1.164

      mov      r0, 0

.oddeven:
      ;; reds
      vadd     HX(32++,0)*, HX(0++,0)*,  HX(56,0) REP 2
      vadd     HX(34++,0)*, HX(2++,0)*,  HX(57,0) REP 2
      vadd     HX(36++,0)*, HX(4++,0)*,  HX(58,0) REP 2
      vadd     HX(38++,0)*, HX(6++,0)*,  HX(59,0) REP 2
      vadd     HX(40++,0)*, HX(8++,0)*,  HX(60,0) REP 2
      vadd     HX(42++,0)*, HX(10++,0)*, HX(61,0) REP 2
      vadd     HX(44++,0)*, HX(12++,0)*, HX(62,0) REP 2
      vadd     HX(46++,0)*, HX(14++,0)*, HX(63,0) REP 2
      ;; blues
      vadd     HX(16++,0)*, HX(0++,0)*,  HX(48,0) REP 2
      vadd     HX(18++,0)*, HX(2++,0)*,  HX(49,0) REP 2
      vadd     HX(20++,0)*, HX(4++,0)*,  HX(50,0) REP 2
      vadd     HX(22++,0)*, HX(6++,0)*,  HX(51,0) REP 2
      vadd     HX(24++,0)*, HX(8++,0)*,  HX(52,0) REP 2
      vadd     HX(26++,0)*, HX(10++,0)*, HX(53,0) REP 2
      vadd     HX(28++,0)*, HX(12++,0)*, HX(54,0) REP 2
      vadd     HX(30++,0)*, HX(14++,0)*, HX(55,0) REP 2
      ;; greens
      vadd     HX(0++,0)*,  HX(0++,0)*,  HX(56,32) REP 2
      vadd     HX(2++,0)*,  HX(2++,0)*,  HX(57,32) REP 2
      vadd     HX(4++,0)*,  HX(4++,0)*,  HX(58,32) REP 2
      vadd     HX(6++,0)*,  HX(6++,0)*,  HX(59,32) REP 2
      vadd     HX(8++,0)*,  HX(8++,0)*,  HX(60,32) REP 2
      vadd     HX(10++,0)*, HX(10++,0)*, HX(61,32) REP 2
      vadd     HX(12++,0)*, HX(12++,0)*, HX(62,32) REP 2
      vadd     HX(14++,0)*, HX(14++,0)*, HX(63,32) REP 2
      cbadd2
      addcmpblt r0, 1, 2, .oddeven

      vasr     HX(0++,0), HX(0++,0), 5       REP 16
      vasr     HX(16++,0), HX(16++,0), 5     REP 32
      vasr     HX(0++,32), HX(0++,32), 5     REP 16
      vasr     HX(16++,32), HX(16++,32), 5   REP 32

      ld       r0, (sp)
      ld       r1, (sp+4)
      b        r5
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuv420ci_to_rgb_load_32x16o
;;;   yuv420ci_to_rgb_load_32x16o_misaligned
;;;   yuv420_to_rgb_load_32x16o
;;;   yuv420_to_rgb_load_32x16o_misaligned
;;;
;;; SYNOPSIS
;;;   void yuv420_to_rgb_load_32x16o(
;;;      unsigned char const *yptr,          r0
;;;      unsigned char const *uptr,          r1
;;;      unsigned char const *vptr,          r2
;;;      int                  src_pitch,     r3
;;;      int                  src_pitch2,    r4
;;;      void               (*commit)(unsigned char *dest, int pitch, ...),  r5
;;;      unsigned char       *dest,          (sp)
;;;      int                  pitch,         (sp+4)
;;;      ...                                 (sp+8)
;;;
;;; FUNCTION
;;;   Load 32x16o block of YUV data, do a partial conversion to RGB, and link a
;;;   function (eg. yuv_to_rgb888_store_32x16o) to finalise and store the result.
;;;
;;; RETURNS
;;;    -

function yuv420ci_to_rgb_load_32x16o ; {{{
.globl yuv420ci_to_rgb_load_32x16o_misaligned
yuv420ci_to_rgb_load_32x16o_misaligned: ; legacy alias
      vld      HX(0++,0), (r0+=r3) REP 16             ;; Y (split into odds and evens)
      vld      HX(0++,32), (r1+=r4) REP 8             ;; UV (interleaved)
      addscale r1, r4 * 8
      vld      HX(8,32), (r1)
      b        yuv420_to_rgb_load_32x16o_loaded
endfunc ; }}}

function yuv420_to_rgb_load_32x16o ; {{{
.globl yuv420_to_rgb_load_32x16o_misaligned
yuv420_to_rgb_load_32x16o_misaligned: ; legacy alias
      vld      HX(0++,0), (r0+=r3) REP 16             ;; Y (split into odds and evens)
      vld      H(0++,32), (r1+=r4) REP 8              ;; U
      vld      H(0++,48), (r2+=r4) REP 8              ;; V
      addscale r1, r4 * 8
      addscale r2, r4 * 8
      vld      H(8,32), (r1)
      vld      H(8,48), (r2)

yuv420_to_rgb_load_32x16o_loaded:
      vmulm    HX(48++,0),  H(0++,32), bUSCALE REP 8  ;; bU = U * 2.018
      vmulm    HX(56++,0),  H(0++,48), rVSCALE REP 8  ;; rV = V * 1.596
      vmulm    HX(48++,32), H(0++,32), gUSCALE REP 8  ;; gU = U * 0.391
      vmulm    HX(56++,32), H(0++,48), gVSCALE REP 8  ;; gV = V * 0.813

      vadd     HX(56++,32), HX(48++,32), HX(56++,32) REP 8 ;; gUV = gU + gV

      vmulm    HX(48,32),   H(8  ,32), bUSCALE        ;; bU = U * 2.018
      vmulm    HX(49,32),   H(8  ,48), rVSCALE        ;; rV = V * 1.596
      vmulm    -        ,   H(8  ,32), gUSCALE CLRA ACC ;; gU = U * 0.391
      vmulm    HX(50,32),   H(8  ,48), gVSCALE ACC    ;; gUV = V * 0.813 + gU

      vmulm    HX(0++,32), H(0++,16), YSCALE REP 16   ;; Y *= 1.164
      vmulm    HX(0++,0),  H(0++,0),  YSCALE REP 16   ;; Y *= 1.164

      mov      r0, 0
.oddeven:
      ;; reds
      vadd     HX(32  ,0)*, HX(0  ,0)*,  HX(56,0)
      vadd     HX(33++,0)*, HX(1++,0)*,  HX(57,0) REP 2
      vadd     HX(35++,0)*, HX(3++,0)*,  HX(58,0) REP 2
      vadd     HX(37++,0)*, HX(5++,0)*,  HX(59,0) REP 2
      vadd     HX(39++,0)*, HX(7++,0)*,  HX(60,0) REP 2
      vadd     HX(41++,0)*, HX(9++,0)*,  HX(61,0) REP 2
      vadd     HX(43++,0)*, HX(11++,0)*, HX(62,0) REP 2
      vadd     HX(45++,0)*, HX(13++,0)*, HX(63,0) REP 2
      vadd     HX(47  ,0)*, HX(15  ,0)*, HX(49,32)
      ;; blues
      vadd     HX(16  ,0)*, HX(0  ,0)*,  HX(48,0)
      vadd     HX(17++,0)*, HX(1++,0)*,  HX(49,0) REP 2
      vadd     HX(19++,0)*, HX(3++,0)*,  HX(50,0) REP 2
      vadd     HX(21++,0)*, HX(5++,0)*,  HX(51,0) REP 2
      vadd     HX(23++,0)*, HX(7++,0)*,  HX(52,0) REP 2
      vadd     HX(25++,0)*, HX(9++,0)*,  HX(53,0) REP 2
      vadd     HX(27++,0)*, HX(11++,0)*, HX(54,0) REP 2
      vadd     HX(29++,0)*, HX(13++,0)*, HX(55,0) REP 2
      vadd     HX(31  ,0)*, HX(15  ,0)*, HX(48,32)
      ;; greens
      vadd     HX(0  ,0)*,  HX(0  ,0)*,  HX(56,32)
      vadd     HX(1++,0)*,  HX(1++,0)*,  HX(57,32) REP 2
      vadd     HX(3++,0)*,  HX(3++,0)*,  HX(58,32) REP 2
      vadd     HX(5++,0)*,  HX(5++,0)*,  HX(59,32) REP 2
      vadd     HX(7++,0)*,  HX(7++,0)*,  HX(60,32) REP 2
      vadd     HX(9++,0)*,  HX(9++,0)*,  HX(61,32) REP 2
      vadd     HX(11++,0)*, HX(11++,0)*, HX(62,32) REP 2
      vadd     HX(13++,0)*, HX(13++,0)*, HX(63,32) REP 2
      vadd     HX(15  ,0)*, HX(15  ,0)*, HX(50,32)
      cbadd2
      addcmpblt r0, 1, 2, .oddeven

      vasr     HX(0++,0), HX(0++,0), 5       REP 16
      vasr     HX(16++,0), HX(16++,0), 5     REP 32
      vasr     HX(0++,32), HX(0++,32), 5     REP 16
      vasr     HX(16++,32), HX(16++,32), 5   REP 32

      ld       r0, (sp)
      ld       r1, (sp+4)
      b        r5
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuv422_to_rgb_load_32x16
;;;   yuv422_to_rgb_load_32x16_misaligned
;;;   yuv422ci_to_rgb_load_32x16
;;;   yuv422ci_to_rgb_load_32x16_misaligned
;;;
;;; SYNOPSIS
;;;   void yuv422_to_rgb_load_32x16(
;;;      unsigned char const *yptr,          r0
;;;      unsigned char const *uptr,          r1
;;;      unsigned char const *vptr,          r2
;;;      int                  src_pitch,     r3
;;;      int                  src_pitch2,    r4
;;;      void               (*commit)(unsigned char *dest, int pitch, ...),  r5
;;;      unsigned char       *dest,          (sp)
;;;      int                  pitch,         (sp+4)
;;;      ...                                 (sp+8)
;;;
;;; FUNCTION
;;;   Load 32x16 block of YUV 422 data, do a partial conversion to RGB, and link
;;;   a function (eg. yuv422_store_32x16_rgb888) to finalise and store the result.
;;;
;;; RETURNS
;;;    -
;;;
;;; TODO
;;;   test

function yuv422ci_to_rgb_load_32x16 ; {{{
.globl yuv422ci_to_rgb_load_32x16_misaligned
yuv422ci_to_rgb_load_32x16_misaligned: ; legacy alias
      vld      HX(0++,0), (r0+=r3) REP 16             ;; Y (split into odds and evens)
      vld      HX(0++,32), (r1+=r4) REP 16            ;; UV (interleaved)
      b        yuv422_to_rgb_load_32x16_loaded
endfunc ; }}}

function yuv422_to_rgb_load_32x16 ; {{{
.globl yuv422_to_rgb_load_32x16_misaligned
yuv422_to_rgb_load_32x16_misaligned: ; legacy alias

      vld      HX(0++,0), (r0+=r3) REP 16             ;; Y (split into odds and evens)
      vld      H(0++,32), (r1+=r4) REP 16             ;; U
      vld      H(0++,48), (r2+=r4) REP 16             ;; V

yuv422_to_rgb_load_32x16_loaded:
      vmulm    HX(16++,32), H(0++,32), bUSCALE REP 16 ;; bU = U * 2.018
      vmulm    HX(32++,32), H(0++,48), rVSCALE REP 16 ;; rV = V * 1.596
      vmulm    HX(48++,0),  H(0++,32), gUSCALE REP 16 ;; gU = U * 0.391
      vmulm    HX(48++,32), H(0++,48), gVSCALE REP 16 ;; gV = V * 0.813

      vadd     HX(48++,32), HX(48++,32), HX(48++,0) REP 16 ;; gUV = gU + gV

      vmulm    HX(0++,32), H(0++,16), YSCALE REP 16   ;; Y *= 1.164
      vmulm    HX(0++,0),  H(0++,0),  YSCALE REP 16   ;; Y *= 1.164

      ;; even reds
      vadd     HX(32++,0),  HX(0++,0),  HX(32++,32) REP 16
      ;; even blues
      vadd     HX(16++,0),  HX(0++,0),  HX(16++,32) REP 16
      ;; even greens
      vadd     HX(0++,0),   HX(0++,0),  HX(48++,32) REP 16

      ;; odd reds
      vadd     HX(32++,32), HX(0++,32), HX(32++,32) REP 16
      ;; odd blues
      vadd     HX(16++,32), HX(0++,32), HX(16++,32) REP 16
      ;; odd greens
      vadd     HX(0++,32),  HX(0++,32), HX(48++,32) REP 16

      vasr     HX(0++,0), HX(0++,0), 5       REP 16
      vasr     HX(16++,0), HX(16++,0), 5     REP 32
      vasr     HX(0++,32), HX(0++,32), 5     REP 16
      vasr     HX(16++,32), HX(16++,32), 5   REP 32

      ld       r0, (sp)
      ld       r1, (sp+4)
      b        r5
endfunc ; }}}
.FORGET


;;; this function takes an extra argument to describe how many valid source lines there are
;;; and will load the maximum of this and 16 for the Y, and half this for the UV.
function yuv_uv_to_rgb_load_32x16 ; {{{
      mov      r3, r0
      ld       r0, (sp+12)
      min      r0, 16
      vld      HX(0++,0), (r3+=r4) REP r0             ;; Y (split into odds and evens)
      add      r0, 1
      asr      r0, 1
      vld      HX(0++,32), (r1+=r4) REP r0            ;; U&V

yuv_uv_to_rgb_load_32x16_loaded:
      vmulm    HX(48++,0),  H(0++,32), bUSCALE REP 8  ;; bU = U * 2.018
      vmulm    HX(56++,0),  H(0++,48), rVSCALE REP 8  ;; rV = V * 1.596
      vmulm    HX(48++,32), H(0++,32), gUSCALE REP 8  ;; gU = U * 0.391
      vmulm    HX(56++,32), H(0++,48), gVSCALE REP 8  ;; gV = V * 0.813

      vadd     HX(56++,32), HX(48++,32), HX(56++,32) REP 8 ;; gUV = gU + gV

      vmulm    HX(0++,32), H(0++,16), YSCALE REP 16   ;; Y *= 1.164
      vmulm    HX(0++,0),  H(0++,0),  YSCALE REP 16   ;; Y *= 1.164

      mov      r0, 0

.oddeven:
      ;; reds
      vadd     HX(32++,0)*, HX(0++,0)*,  HX(56,0) REP 2
      vadd     HX(34++,0)*, HX(2++,0)*,  HX(57,0) REP 2
      vadd     HX(36++,0)*, HX(4++,0)*,  HX(58,0) REP 2
      vadd     HX(38++,0)*, HX(6++,0)*,  HX(59,0) REP 2
      vadd     HX(40++,0)*, HX(8++,0)*,  HX(60,0) REP 2
      vadd     HX(42++,0)*, HX(10++,0)*, HX(61,0) REP 2
      vadd     HX(44++,0)*, HX(12++,0)*, HX(62,0) REP 2
      vadd     HX(46++,0)*, HX(14++,0)*, HX(63,0) REP 2
      ;; blues
      vadd     HX(16++,0)*, HX(0++,0)*,  HX(48,0) REP 2
      vadd     HX(18++,0)*, HX(2++,0)*,  HX(49,0) REP 2
      vadd     HX(20++,0)*, HX(4++,0)*,  HX(50,0) REP 2
      vadd     HX(22++,0)*, HX(6++,0)*,  HX(51,0) REP 2
      vadd     HX(24++,0)*, HX(8++,0)*,  HX(52,0) REP 2
      vadd     HX(26++,0)*, HX(10++,0)*, HX(53,0) REP 2
      vadd     HX(28++,0)*, HX(12++,0)*, HX(54,0) REP 2
      vadd     HX(30++,0)*, HX(14++,0)*, HX(55,0) REP 2
      ;; greens
      vadd     HX(0++,0)*,  HX(0++,0)*,  HX(56,32) REP 2
      vadd     HX(2++,0)*,  HX(2++,0)*,  HX(57,32) REP 2
      vadd     HX(4++,0)*,  HX(4++,0)*,  HX(58,32) REP 2
      vadd     HX(6++,0)*,  HX(6++,0)*,  HX(59,32) REP 2
      vadd     HX(8++,0)*,  HX(8++,0)*,  HX(60,32) REP 2
      vadd     HX(10++,0)*, HX(10++,0)*, HX(61,32) REP 2
      vadd     HX(12++,0)*, HX(12++,0)*, HX(62,32) REP 2
      vadd     HX(14++,0)*, HX(14++,0)*, HX(63,32) REP 2
      cbadd2
      addcmpblt r0, 1, 2, .oddeven

      vasr     HX(0++,0), HX(0++,0), 5       REP 16
      vasr     HX(16++,0), HX(16++,0), 5     REP 32
      vasr     HX(0++,32), HX(0++,32), 5     REP 16
      vasr     HX(16++,32), HX(16++,32), 5   REP 32

      ld       r0, (sp)
      ld       r1, (sp+4)
      b        r5
endfunc ; }}}
.FORGET



;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgba32_store_32x16
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgba32_store_32x16(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...)                             (sp+8) (alpha)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 32x16 block of partially converted YUV data from
;;;   yuv420_load_32x16_rgb().
;;;
;;; RETURNS
;;;    -

function yuv_to_rgba32_store_32xn_misaligned ; {{{
      ld       r2, (sp+8)                             ;; alpha
      mov      r4, r0
      ld       r0, (sp+12)                            ;; lines
      b        yuv_to_rgba32_store_32xn_entry
endfunc ; }}}

function yuv_to_rgba32_store_32x16 ; {{{
      ld       r2, (sp+8)                             ;; alpha
      mov      r4, r0
      mov      r0, 16                                 ;; lines
yuv_to_rgba32_store_32xn_entry:
      mov      r3, 0                                  ;; input offset
      vmov     V(48,48++), r2             REP 16

.leftright16:
      mov      r2, 0                                  ;; output offset
.pairstep:
      vadds    V(48,0)+r2,  VX(32,0)+r3, RBIAS        ;; even red
      vadds    V(48,16)+r2, VX(0,0)+r3,  GBIAS        ;; even green
      vadds    V(48,32)+r2, VX(16,0)+r3, BBIAS        ;; even blue

      vadds    V(48,1)+r2,  VX(32,32)+r3, RBIAS       ;; odd red
      vadds    V(48,17)+r2, VX(0,32)+r3,  GBIAS       ;; odd green
      vadds    V(48,33)+r2, VX(16,32)+r3, BBIAS       ;; odd blue
      add      r3, 1
      addcmpbne r2, 2, 16, .pairstep       ;; bne means that we set up the next prediction correctly even when we fall through

      vst      H32(48++,0), (r4+=r1)      REP r0
      add      r4, 64
      addcmpbne r3, 0, 16, .leftright16
      b        lr
endfunc ; }}}
.FORGET



;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgba32_store_16x16
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgba32_store_16x16(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...)                             (sp+8) (alpha)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 16x16 block of partially converted YUV data.
;;;
;;; RETURNS
;;;    -
;;;
;;; TODO
;;;   Combine with 32x16 function because they're identical except for the loop

function yuv_to_rgba32_store_16x16 ; {{{
      ld       r5, (sp+8)                             ;; alpha
      mov      r3, 0                                  ;; input offset
      mov      r2, 0                                  ;; output offset
.quadstep:
      vadds    V(48,0)+r2,  VX(32,0)+r3, RBIAS        ;; left8 even red
      vadds    V(48,16)+r2, VX(0,0)+r3,  GBIAS        ;; left8 even green
      vadds    V(48,1)+r2,  VX(16,0)+r3, BBIAS        ;; left8 even blue
      vmov     V(48,17)+r2, r5                        ;; alpha

      vadds    V(48,2)+r2,  VX(32,32)+r3, RBIAS       ;; left8 odd red
      vadds    V(48,18)+r2, VX(0,32)+r3,  GBIAS       ;; left8 odd green
      vadds    V(48,3)+r2,  VX(16,32)+r3, BBIAS       ;; left8 odd blue
      vmov     V(48,19)+r2, r5                        ;; alpha

      vadds    V(48,32+0)+r2,  VX(32,4)+r3, RBIAS     ;; right8 even red
      vadds    V(48,32+16)+r2, VX(0,4)+r3,  GBIAS     ;; right8 even green
      vadds    V(48,32+1)+r2,  VX(16,4)+r3, BBIAS     ;; right8 even blue
      vmov     V(48,32+17)+r2, r5                     ;; alpha

      vadds    V(48,32+2)+r2,  VX(32,36)+r3, RBIAS    ;; right8 odd red
      vadds    V(48,32+18)+r2, VX(0,36)+r3,  GBIAS    ;; right8 odd green
      vadds    V(48,32+3)+r2,  VX(16,36)+r3, BBIAS    ;; right8 odd blue
      vmov     V(48,32+19)+r2, r5                     ;; alpha

      add      r3, 1
      addcmpbne r2, 4, 16, .quadstep       ;; bne means that we set up the next prediction correctly even when we fall through

      vst      HX(48++,0), (r0+=r1) REP 16
      vst      HX(48++,32), (r0+32+=r1) REP 16
      b        lr
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgba32_store_mxn_misaligned
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgba32_store_mxn_misaligned(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...,
;;;      int                  alpha,      (sp+8)  (ignored)
;;;      int                  lines,      (sp+12)
;;;      int                  columns);   (sp+16)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a (columns)x(lines) block of partially converted
;;;   YUV data.
;;;
;;; RETURNS
;;;    -

function yuv_to_rgba32_store_mxn_misaligned ; {{{
      ld       r4, (sp+12)                            ;; line limit
      addcmpbeq r4, 0, 0, .donothing
      cmp      r4, 16                        ; set Z for .all16 branch later on
      ld       r5, (sp+16)                            ;; column limit
      stm      r6-r7, (--sp)
      .cfa_push {%r6,%r7}
      add      r6, pc, pcrel(.onetwothree)
      mov      r7, 64
      shl      r4, 6

      mov      r3, 0                                  ;; input offset
.leftright16:
      vld      V(48,47), (r6)
      vsub     VX(48,47), V(48,47), r5             SETF     ; C = i < r5
      mov      r2, 0                                  ;; output offset
      sub      r5, 16
      vadd     VX(48,47), VX(48,47), 8
      vand     -, VX(48,47), 0x8000                SETF     ; N = i + 8 < r5

.quadstep:
      vadds    V(48,0)+r2,  VX(32,0)+r3, RBIAS        ;; left8 even red
      vadds    V(48,16)+r2, VX(0,0)+r3,  GBIAS        ;; left8 even green
      vadds    V(48,1)+r2,  VX(16,0)+r3, BBIAS        ;; left8 even blue
      vmov     V(48,17)+r2, 255                       ;; alpha

      vadds    V(48,2)+r2,  VX(32,32)+r3, RBIAS       ;; left8 odd red
      vadds    V(48,18)+r2, VX(0,32)+r3,  GBIAS       ;; left8 odd green
      vadds    V(48,3)+r2,  VX(16,32)+r3, BBIAS       ;; left8 odd blue
      vmov     V(48,19)+r2, 255                       ;; alpha

      vadds    V(48,32+0)+r2,  VX(32,4)+r3, RBIAS     ;; right8 even red
      vadds    V(48,32+16)+r2, VX(0,4)+r3,  GBIAS     ;; right8 even green
      vadds    V(48,32+1)+r2,  VX(16,4)+r3, BBIAS     ;; right8 even blue
      vmov     V(48,32+17)+r2, 255                    ;; alpha

      vadds    V(48,32+2)+r2,  VX(32,36)+r3, RBIAS    ;; right8 odd red
      vadds    V(48,32+18)+r2, VX(0,36)+r3,  GBIAS    ;; right8 odd green
      vadds    V(48,32+3)+r2,  VX(16,36)+r3, BBIAS    ;; right8 odd blue
      vmov     V(48,32+19)+r2, 255                    ;; alpha
      add      r3, 1
      addcmpbne r2, 4, 16, .quadstep       ;; bne means that we set up the next prediction correctly even when we fall through

      ; flag set at entry to function
      beq      .all16

      mov      r2, 0
      mov      r6, r0
.saveloop:
      vst      HX(48,0)+r2, (r6)                   IFC
      vst      HX(48,32)+r2, (r6+32)               IFN
      add      r6, r1
      addcmpbne r2, r7, r4, .saveloop
      add      r6, pc, pcrel(.onetwothree)
      b        .onward

.all16:
      vst      HX(48++,0), (r0+=r1) REP 16         IFC
      vst      HX(48++,32), (r0+32+=r1) REP 16     IFN
.onward:
      addcmpble r5, 0, 0, .quitnow
      add      r0, 64
      addcmpbne r3, 4, 16, .leftright16
.quitnow:
      ldm      r6-r7, (sp++)
      .cfa_pop {%r7,%r6}
.donothing:
      b        lr

.align 16
.onetwothree:   .byte 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7
endfunc ; }}}
.FORGET


.if _VC_VERSION >= 3
;;; *****************************************************************************
;;; NAME
;;;   yuv_to_tf_rgba32_store_32x16
;;;
;;; SYNOPSIS
;;;   void yuv_to_tf_rgba32_store_32x16(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...)                             (sp+8) (alpha)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 32x16 block of partially converted YUV data from
;;;   yuv420_load_32x16_rgb().
;;;
;;; RETURNS
;;;    -

function yuv_to_tf_rgba32_store_32x16 ; {{{
      vadds    V8(16,0++),  V16(16,0++),  BBIAS REP 16 ;; even blue
      ld       r5, (sp+8)                             ;; alpha
      vadds    V8(16,16++), V16(16,32++), BBIAS REP 16 ;; odd blue
      mov      r3, 0                                  ;; input offset
      vadds    V8(16,32++), V16(32,0++),  RBIAS REP 16 ;; even red
      mov      r4, 0
      vadds    V8(16,48++), V16(32,32++), RBIAS REP 16 ;; odd red
      nop
      vmov     V(32,48), r5                           ;; alpha
      vmov     V(32,49), r5                           ;; alpha
      vmov     V(32,50), r5                           ;; alpha
      vmov     V(32,51), r5                           ;; alpha

.oloop:
      mov      r2, 0                                  ;; output offset
      nop
.quadstep:
      vmov     V(32,0),  V(16,32)+r3                  ;; even red
      vadds    V(32,16), VX(0,0)+r3,  GBIAS           ;; even green
      vmov     V(32,32), V(16,0)+r3                   ;; even blue

      vmov     V(32,1),  V(16,48)+r3                  ;; odd red
      vadds    V(32,17), VX(0,32)+r3, GBIAS           ;; odd green
      vmov     V(32,33), V(16,16)+r3                  ;; odd blue

      vmov     V(32,2),  V(16,33)+r3                  ;; even red
      vadds    V(32,18), VX(0,1)+r3,  GBIAS           ;; even green
      vmov     V(32,34), V(16,1)+r3                   ;; even blue

      vmov     V(32,3),  V(16,49)+r3                  ;; odd red
      vadds    V(32,19), VX(0,33)+r3, GBIAS           ;; odd green
      vmov     V(32,35), V(16,17)+r3                  ;; odd blue

      add      r3, 2

      v32interl   V32(32,4), V32(32,0), V32(32,2)
      v32interl   V32(32,5), V32(32,1), V32(32,3)
      v32interh   V32(32,6), V32(32,0), V32(32,2)
      v32interh   V32(32,7), V32(32,1), V32(32,3)

      v32interl   V32(48,0)+r2, V32(32,4), V32(32,5)
      v32interh   V32(48,4)+r2, V32(32,4), V32(32,5)
      v32interl   V32(48,8)+r2, V32(32,6), V32(32,7)
      v32interh   V32(48,12)+r2, V32(32,6), V32(32,7)

      addcmpbne r2, 1, 4, .quadstep       ;; bne means that we set up the next prediction correctly even when we fall through

      cmp      r4, 64
      mov      r4, 64
      vst      V32(48,0++), (r0+=r4) REP 16
      add      r0, r1
      bne      .oloop
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuv_to_tf_rgba32_store_16x16
;;;
;;; SYNOPSIS
;;;   void yuv_to_tf_rgba32_store_16x16(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...)                             (sp+8) (alpha)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 32x16 block of partially converted YUV data from
;;;   yuv420_load_32x16_rgb().
;;;
;;; RETURNS
;;;    -

function yuv_to_tf_rgba32_store_16x16 ; {{{
      vadds    V8(16,0++),  V16(16,0++),  BBIAS REP 16 ;; even blue
      ld       r5, (sp+8)                             ;; alpha
      vadds    V8(16,16++), V16(16,32++), BBIAS REP 16 ;; odd blue
      mov      r3, 0                                  ;; input offset
      vadds    V8(16,32++), V16(32,0++),  RBIAS REP 16 ;; even red
      mov      r4, 0
      vadds    V8(16,48++), V16(32,32++), RBIAS REP 16 ;; odd red
      nop

      mov      r2, 0                                  ;; output offset
      nop
.quadstep:
      vmov     V(32,32), V(16,0)+r3                   ;; even blue
      vadds    V(32,16), VX(0,0)+r3,  GBIAS           ;; even green
      vmov     V(32,0),  V(16,32)+r3                  ;; even red
      vmov     V(32,48), r5                           ;; alpha

      vmov     V(32,33), V(16,16)+r3                  ;; odd blue
      vadds    V(32,17), VX(0,32)+r3, GBIAS           ;; odd green
      vmov     V(32,1),  V(16,48)+r3                  ;; odd red
      vmov     V(32,49), r5                           ;; alpha

      vmov     V(32,34), V(16,1)+r3                   ;; even blue
      vadds    V(32,18), VX(0,1)+r3,  GBIAS           ;; even green
      vmov     V(32,2),  V(16,33)+r3                  ;; even red
      vmov     V(32,50), r5                           ;; alpha

      vmov     V(32,35), V(16,17)+r3                  ;; odd blue
      vadds    V(32,19), VX(0,33)+r3, GBIAS           ;; odd green
      vmov     V(32,3),  V(16,49)+r3                  ;; odd red
      vmov     V(32,51), r5                           ;; alpha

      add      r3, 2

      v32interl   V32(32,4), V32(32,0), V32(32,2)
      v32interl   V32(32,5), V32(32,1), V32(32,3)
      v32interh   V32(32,6), V32(32,0), V32(32,2)
      v32interh   V32(32,7), V32(32,1), V32(32,3)

      v32interl   V32(48,0)+r2, V32(32,4), V32(32,5)
      v32interh   V32(48,4)+r2, V32(32,4), V32(32,5)
      v32interl   V32(48,8)+r2, V32(32,6), V32(32,7)
      v32interh   V32(48,12)+r2, V32(32,6), V32(32,7)

      addcmpbne r2, 1, 4, .quadstep       ;; bne means that we set up the next prediction correctly even when we fall through

      mov      r4, 64
      vst      V32(48,0++), (r0+=r4) REP 16
      add      r0, r1
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuv_to_tf_rgba32_linear_store_16x16
;;;
;;; SYNOPSIS
;;;   void yuv_to_tf_rgba32_store_16x16(
;;;      unsigned char       *dest,         r0
;;;      int                  length        r1  (must be power of 2)
;;;      ...)                             (sp+8) (alpha)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 32x16 block of partially converted YUV data from
;;;   yuv420_load_32x16_rgb().
;;;   Processes full subtile but only stores square subregion restricted to
;;;   power of 2 length
;;;
;;; RETURNS
;;;    -

function yuv_to_tf_rgba32_linear_store_16x16 ; {{{
      vadds    V8(16,0++),  V16(16,0++),  BBIAS REP 16 ;; even blue
      ld       r5, (sp+8)                             ;; alpha
      vadds    V8(16,16++), V16(16,32++), BBIAS REP 16 ;; odd blue
      mov      r3, 0                                  ;; input offset
      vadds    V8(16,32++), V16(32,0++),  RBIAS REP 16 ;; even red
      mov      r4, 0
      vadds    V8(16,48++), V16(32,32++), RBIAS REP 16 ;; odd red
      nop

      mov      r2, 0                                  ;; output offset
      nop
.quadstep:
      vmov     V(32,32), V(16,0)+r3                   ;; even blue
      vadds    V(32,16), VX(0,0)+r3,  GBIAS           ;; even green
      vmov     V(32,0),  V(16,32)+r3                  ;; even red
      vmov     V(32,48), r5                           ;; alpha

      vmov     V(32,33), V(16,16)+r3                  ;; odd blue
      vadds    V(32,17), VX(0,32)+r3, GBIAS           ;; odd green
      vmov     V(32,1),  V(16,48)+r3                  ;; odd red
      vmov     V(32,49), r5                           ;; alpha

      vmov     V(32,34), V(16,1)+r3                   ;; even blue
      vadds    V(32,18), VX(0,1)+r3,  GBIAS           ;; even green
      vmov     V(32,2),  V(16,33)+r3                  ;; even red
      vmov     V(32,50), r5                           ;; alpha

      vmov     V(32,35), V(16,17)+r3                  ;; odd blue
      vadds    V(32,19), VX(0,33)+r3, GBIAS           ;; odd green
      vmov     V(32,3),  V(16,49)+r3                  ;; odd red
      vmov     V(32,51), r5                           ;; alpha

      add      r3, 2

      v32interl   V32(32,4), V32(32,0), V32(32,2)
      v32interl   V32(32,5), V32(32,1), V32(32,3)
      v32interh   V32(32,6), V32(32,0), V32(32,2)
      v32interh   V32(32,7), V32(32,1), V32(32,3)

      v32interl   V32(48,0)+r2, V32(32,4), V32(32,5)
      v32interh   V32(48,4)+r2, V32(32,4), V32(32,5)
      v32interl   V32(48,8)+r2, V32(32,6), V32(32,7)
      v32interh   V32(48,12)+r2, V32(32,6), V32(32,7)

      addcmpbne r2, 1, 4, .quadstep       ;; bne means that we set up the next prediction correctly even when we fall through

      mov      r4, 64
      addcmpbne r1,0,16,1f
;;store whole subtile (16x16)      
      vst      V32(48,0++), (r0+=r4) REP 16  
      b storedone
1:    addcmpbne r1,0,8,2f
;;store quarter subtile (8x8)      
      vst      V32(48,0++), (r0+=r4) REP 2
      add      r0, r4<<1
      vst      V32(48,4++), (r0+=r4) REP 2
      b storedone
2:    addcmpbne r1,0,4,3f
;;store microtile (4x4)      
      vst      V32(48,0), (r0)
      b storedone
3:    addcmpbne r1,0,2,4f
;;store quarter microtile (2x2)
      vmov     H32(49,0), H32(48,0)
      vbitplanes -, 0xc SETF
      vmov     H32(48,0), H32(49,0) IFNZ
      vbitplanes -, 0xf SETF
      vst      V32(48,0), (r0)
      b storedone
;;store single pixel (1x1)
4:    vbitplanes -, 0x1 SETF
      vst      V32(48,0), (r0) IFNZ
storedone:
      b        lr
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   yuv_to_tf_rgb565_store_32x16
;;;
;;; SYNOPSIS
;;;   void yuv_to_tf_rgb565_store_32x16(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...)                             (sp+8) (alpha)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 32x16 block of partially converted YUV data from
;;;   yuv420_load_32x16_rgb().
;;;
;;; RETURNS
;;;    -

function yuv_to_tf_rgb565_store_32x16 ; {{{
      vadds    V(0,0++),  VX(0,0++),    GBIAS   REP 16   ;; even green
      vadds    V(0,32++),  VX(0,32++),  GBIAS   REP 16   ;; odd green
      vadds    V(16,0++),  VX(16,0++),  BBIAS   REP 16  ;; even blue
      vadds    V(16,32++),  VX(16,32++), BBIAS  REP 16  ;; odd blue
      vadds    V(32,0++),  VX(32,0++),  RBIAS   REP 16  ;; even red
      vadds    V(32,32++),  VX(32,32++), RBIAS  REP 16  ;; odd red
      vand     V(0,0++),  V(0,0++),     GMASK   REP 16   ;; even green mask
      vand     V(0,32++),  V(0,32++),   GMASK   REP 16   ;; odd green mask
.if BMASK < 0xF8
      vand     V(16,0++),  V(16,0++),  BMASK    REP 16   ;; even blue mask
      vand     V(16,32++),  V(16,32++), BMASK   REP 16   ;; odd blue mask
.endif
      vand     V(16,16++), V(32,0++),  RMASK    REP 16   ;; even red mask
      vand     V(16,48++), V(32,32++), RMASK    REP 16   ;; odd red mask
      vshl     VX(32,0++), V(0,0++), 5-2        REP 16   ;; even green centre
      vshl     VX(32,32++), V(0,32++), 5-2      REP 16   ;; odd green centre
      vlsr     V(16,0++), V(16,0++), 3          REP 16   ;; even blue shift
      vlsr     V(16,32++), V(16,32++), 3        REP 16   ;; odd blue shift

      vor      VX(0,0++), VX(16,0++), VX(32,0++) REP 32

      b        tformat_subtile_store_32bit
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   yuv_to_tf_rgb565_store_32x16_dither
;;;
;;; SYNOPSIS
;;;   void yuv_to_tf_rgb565_store_32x16_dither(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...)                             (sp+8) (alpha)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 32x16 block of partially converted YUV data from
;;;   yuv420_load_32x16_rgb().
;;;
;;; RETURNS
;;;    -

function yuv_to_tf_rgb565_store_32x16_dither ; {{{
      add      r4, pc, pcrel27(rgb565_dither_state)
      mov      r2, 0
      
      vld      HX(48,0), (r4)
      vld      HX(49,0), (r4+32)
      vld      HX(50,0), (r4+64)

      mov      r3, 4000001039
      mulhd.uu r3, r0 ; very ineffective random number generator

      vbitplanes HX(51,0), r3
      vsub     HX(48,0), HX(48,0), HX(51,0)
      vsub     HX(49,0), HX(49,0), HX(51,0)
      vsub     HX(50,0), HX(50,0), HX(51,0)

.loop:
      vadds    V(0,0)+r2, VX(0,0)+r2, HX(48,0) CLRA ACC   ;; even green
      vand     V(0,0)+r2,  V(0,0)+r2, GMASK           ;; even green mask
      vrsub    HX(48,0), V(0,0)+r2, GBIAS       ACC

      vadds    V(16,0)+r2, VX(16,0)+r2, HX(50,0) CLRA ACC ;; even blue
      vand     V(16,0)+r2,  V(16,0)+r2, BMASK         ;; even blue mask
      vrsub    HX(50,0), V(16,0)+r2, BBIAS      ACC

      vadds    V(32,0)+r2, VX(32,0)+r2, HX(49,0) CLRA ACC ;; even red
      vand     V(16,16)+r2, V(32,0)+r2, RMASK         ;; even red mask
      vrsub    HX(49,0), V(16,16)+r2, RBIAS     ACC

      vadds    V(0,32)+r2, VX(0,32)+r2, HX(48,0) CLRA ACC ;; odd green
      vand     V(0,32)+r2,  V(0,32)+r2, GMASK         ;; odd green mask
      vrsub    HX(48,0), V(0,32)+r2, GBIAS      ACC

      vadds    V(16,32)+r2, VX(16,32)+r2, HX(50,0) CLRA ACC ;; odd blue
      vand     V(16,32)+r2,  V(16,32)+r2, BMASK       ;; odd blue mask
      vrsub    HX(50,0), V(16,32)+r2, BBIAS     ACC

      vadds    V(32,32)+r2, VX(32,32)+r2, HX(49,0) CLRA ACC ;; odd red
      vand     V(16,48)+r2,  V(32,32)+r2, RMASK       ;; odd red mask
      vrsub    HX(49,0), V(16,48)+r2, RBIAS     ACC

      addcmpbne r2, 1, 16, .loop

      vst      HX(48,0), (r4)
      vst      HX(49,0), (r4+32)
      vst      HX(50,0), (r4+64)

      vshl     VX(32,0++),  V(0,0++), 5-2       REP 16   ;; even green centre
      vshl     VX(32,32++),  V(0,32++), 5-2     REP 16   ;; odd green centre
      vlsr     V(16,0++),  V(16,0++), 3         REP 16   ;; even blue shift
      vlsr     V(16,32++),  V(16,32++), 3       REP 16   ;; odd blue shift
      vor      VX(0,0++),  VX(16,0++),  VX(32,0++)  REP 32

      b        tformat_subtile_store_32bit
endfunc ; }}}
.FORGET

.endif

;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgb888_store_32x16
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgb888_store_32x16(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...)                             unused
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 32x16 block of partially converted YUV data.
;;;
;;; RETURNS
;;;    -

function yuv_to_rgb888_store_32x16 ; {{{
      mov      r2, 0                                  ;; output offset
      mov      r3, 0                                  ;; input offset
.quadstep1:
      vadds    V(48,0)+r2,  VX(32,0)+r3, RBIAS        ;; left10 even red
      vadds    V(48,16)+r2, VX(0,0)+r3,  GBIAS        ;; left10 even green
      vadds    V(48,1)+r2,  VX(16,0)+r3, BBIAS        ;; left10 even blue

      vadds    V(48,17)+r2, VX(32,32)+r3, RBIAS       ;; left10 odd red
      vadds    V(48,2)+r2,  VX(0,32)+r3,  GBIAS       ;; left10 odd green
      vadds    V(48,18)+r2, VX(16,32)+r3, BBIAS       ;; left10 odd blue

      vadds    V(48,32+16)+r2, VX(32,37)+r3, RBIAS    ;; mid10 odd red
      vadds    V(48,32+1)+r2,  VX(0,37)+r3,  GBIAS    ;; mid10 odd green
      vadds    V(48,32+17)+r2, VX(16,37)+r3, BBIAS    ;; mid10 odd blue

      vadds    V(48,32+2)+r2,  VX(32,6)+r3, RBIAS     ;; mid10 even red
      vadds    V(48,32+18)+r2, VX(0,6)+r3,  GBIAS     ;; mid10 even green
      vadds    V(48,32+3)+r2,  VX(16,6)+r3, BBIAS     ;; mid10 even blue

      add      r3, 1
      addcmpbne r2, 3, 15, .quadstep1

      ;; fix-ups
      vadds    V(48,15), VX(32,5), RBIAS              ;; pixel10 even red
      vadds    V(48,31), VX(0,5),  GBIAS              ;; pixel10 even green
      vadds    V(48,32), VX(16,5), BBIAS              ;; pixel10 even blue

      vadds    V(48,63), VX(32,42), RBIAS             ;; pixel21 odd red

      vst      HX(48++,0), (r0+=r1) REP 16
      vst      HX(48++,32), (r0+32+=r1) REP 16

      ;; more fix-ups
      vadds    V(48,0),  VX(0,42),  GBIAS             ;; pixel21 odd green
      vadds    V(48,16), VX(16,42), BBIAS             ;; pixel21 odd blue

      mov      r2, 0                                  ;; output offset
      mov      r3, 0                                  ;; input offset
.quadstep2:
      vadds    V(48,1)+r2,  VX(32,11)+r3, RBIAS       ;; right10 even red
      vadds    V(48,17)+r2, VX(0,11)+r3,  GBIAS       ;; right10 even green
      vadds    V(48,2)+r2,  VX(16,11)+r3, BBIAS       ;; right10 even blue

      vadds    V(48,18)+r2, VX(32,43)+r3, RBIAS       ;; right10 odd red
      vadds    V(48,3)+r2,  VX(0,43)+r3,  GBIAS       ;; right10 odd green
      vadds    V(48,19)+r2, VX(16,43)+r3, BBIAS       ;; right10 odd blue

      add      r3, 1
      addcmpbne r2, 3, 15, .quadstep2

      vst      HX(48++,0), (r0+64+=r1) REP 16
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgb888_store_16x16
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgb888_store_16x16(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...)                             unused
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 16x16 block of partially converted YUV data.
;;;
;;; RETURNS
;;;   -

function yuv_to_rgb888_store_16x16 ; {{{
      mov      r2, 0                                  ;; output offset
      mov      r3, 0                                  ;; input offset
.quadstep1:
      vadds    V(48,0)+r2,  VX(32,0)+r3, RBIAS        ;; left10 even red
      vadds    V(48,16)+r2, VX(0,0)+r3,  GBIAS        ;; left10 even green
      vadds    V(48,1)+r2,  VX(16,0)+r3, BBIAS        ;; left10 even blue

      vadds    V(48,17)+r2, VX(32,32)+r3, RBIAS       ;; left10 odd red
      vadds    V(48,2)+r2,  VX(0,32)+r3,  GBIAS       ;; left10 odd green
      vadds    V(48,18)+r2, VX(16,32)+r3, BBIAS       ;; left10 odd blue

      add      r3, 1
      addcmpbne r2, 3, 15, .quadstep1

      vadds    V(48,15), VX(32,5), RBIAS              ;; pixel10 even red
      vadds    V(48,31), VX(0,5),  GBIAS              ;; pixel10 even green
      vadds    V(48,32), VX(16,5), BBIAS              ;; pixel10 even blue

      vadds    V(48,32+1), VX(32,32+5), RBIAS         ;; pixel11 odd red
      vadds    V(48,32+2), VX(0,32+5),  GBIAS         ;; pixel11 odd green
      vadds    V(48,32+3), VX(16,32+5), BBIAS         ;; pixel11 odd blue

      vadds    V(48,32+4), VX(32,6), RBIAS            ;; pixel12 even red
      vadds    V(48,32+5), VX(0,6),  GBIAS            ;; pixel12 even green
      vadds    V(48,32+6), VX(16,6), BBIAS            ;; pixel12 even blue

      vadds    V(48,32+7), VX(32,32+6), RBIAS         ;; pixel13 odd red
      vadds    V(48,32+8), VX(0,32+6),  GBIAS         ;; pixel13 odd green
      vadds    V(48,32+9), VX(16,32+6), BBIAS         ;; pixel13 odd blue

      vadds    V(48,32+10), VX(32,7), RBIAS           ;; pixel14 even red
      vadds    V(48,32+11), VX(0,7),  GBIAS           ;; pixel14 even green
      vadds    V(48,32+12), VX(16,7), BBIAS           ;; pixel14 even blue

      vadds    V(48,32+13), VX(32,32+7), RBIAS        ;; pixel15 odd red
      vadds    V(48,32+14), VX(0,32+7),  GBIAS        ;; pixel15 odd green
      vadds    V(48,32+15), VX(16,32+7), BBIAS        ;; pixel15 odd blue

      vst      HX(48++,0), (r0+=r1) REP 16
      vst      H(48++,32), (r0+32+=r1) REP 16
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgb888_store_32xn_misaligned
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgb888_store_32xn_misaligend(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...,
;;;      int                  arg1,       (sp+8)
;;;      int                  lines);     (sp+12)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 32x(lines) block of partially converted YUV data.
;;;
;;; RETURNS
;;;    -

function yuv_to_rgb888_store_32xn_misaligned ; {{{
      mov      r4, r0
      ld       r0, (sp+12)                            ; lines
      mov      r2, 0                                  ;; output offset
      mov      r3, 0                                  ;; input offset
.quadstep1:
      vadds    V(48,0)+r2,  VX(32,0)+r3, RBIAS        ;; left10 even red
      vadds    V(48,16)+r2, VX(0,0)+r3,  GBIAS        ;; left10 even green
      vadds    V(48,1)+r2,  VX(16,0)+r3, BBIAS        ;; left10 even blue

      vadds    V(48,17)+r2, VX(32,32)+r3, RBIAS       ;; left10 odd red
      vadds    V(48,2)+r2,  VX(0,32)+r3,  GBIAS       ;; left10 odd green
      vadds    V(48,18)+r2, VX(16,32)+r3, BBIAS       ;; left10 odd blue

      vadds    V(48,32+16)+r2, VX(32,37)+r3, RBIAS    ;; mid10 odd red
      vadds    V(48,32+1)+r2,  VX(0,37)+r3,  GBIAS    ;; mid10 odd green
      vadds    V(48,32+17)+r2, VX(16,37)+r3, BBIAS    ;; mid10 odd blue

      vadds    V(48,32+2)+r2,  VX(32,6)+r3, RBIAS     ;; mid10 even red
      vadds    V(48,32+18)+r2, VX(0,6)+r3,  GBIAS     ;; mid10 even green
      vadds    V(48,32+3)+r2,  VX(16,6)+r3, BBIAS     ;; mid10 even blue

      add      r3, 1
      addcmpbne r2, 3, 15, .quadstep1

      ;; fix-ups
      vadds    V(48,32), VX(32,5), RBIAS              ;; pixel10 even red
      vadds    V(48,31), VX(0,5),  GBIAS              ;; pixel10 even green
      vadds    V(48,15), VX(16,5), BBIAS              ;; pixel10 even blue

      vadds    V(48,63), VX(32,42), RBIAS             ;; pixel21 odd red

      vst      HX(48++,0), (r4+=r1)       REP r0
      vst      HX(48++,32), (r4+32+=r1)   REP r0

      ;; more fix-ups
      vadds    V(48,0),  VX(0,42),  GBIAS             ;; pixel21 odd green
      vadds    V(48,16), VX(16,42), BBIAS             ;; pixel21 odd blue

      mov      r2, 0                                  ;; output offset
      mov      r3, 0                                  ;; input offset
.quadstep2:
      vadds    V(48,1)+r2,  VX(32,11)+r3, RBIAS       ;; right10 even red
      vadds    V(48,17)+r2, VX(0,11)+r3,  GBIAS       ;; right10 even green
      vadds    V(48,2)+r2,  VX(16,11)+r3, BBIAS       ;; right10 even blue

      vadds    V(48,18)+r2, VX(32,43)+r3, RBIAS       ;; right10 odd red
      vadds    V(48,3)+r2,  VX(0,43)+r3,  GBIAS       ;; right10 odd green
      vadds    V(48,19)+r2, VX(16,43)+r3, BBIAS       ;; right10 odd blue

      add      r3, 1
      addcmpbne r2, 3, 15, .quadstep2

      vst      HX(48++,0), (r4+64+=r1)    REP r0
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgb888_store_mxn_misaligned
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgb888_store_mxn_misaligend(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...,
;;;      int                  arg1,       (sp+8)
;;;      int                  lines,      (sp+12)
;;;      int                  columns);   (sp+16)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a (columns)x(lines) block of partially converted
;;;   YUV data.
;;;
;;; RETURNS
;;;    -

function yuv_to_rgb888_store_mxn_misaligned ; {{{
      mov      r2, 0                                  ;; output offset
      mov      r3, 0                                  ;; input offset
.quadstep1:
      vadds    V(48,0)+r2,  VX(32,0)+r3, RBIAS        ;; left10 even red
      vadds    V(48,16)+r2, VX(0,0)+r3,  GBIAS        ;; left10 even green
      vadds    V(48,1)+r2,  VX(16,0)+r3, BBIAS        ;; left10 even blue

      vadds    V(48,17)+r2, VX(32,32)+r3, RBIAS       ;; left10 odd red
      vadds    V(48,2)+r2,  VX(0,32)+r3,  GBIAS       ;; left10 odd green
      vadds    V(48,18)+r2, VX(16,32)+r3, BBIAS       ;; left10 odd blue

      vadds    V(48,32+16)+r2, VX(32,37)+r3, RBIAS    ;; mid10 odd red
      vadds    V(48,32+1)+r2,  VX(0,37)+r3,  GBIAS    ;; mid10 odd green
      vadds    V(48,32+17)+r2, VX(16,37)+r3, BBIAS    ;; mid10 odd blue

      vadds    V(48,32+2)+r2,  VX(32,6)+r3, RBIAS     ;; mid10 even red
      vadds    V(48,32+18)+r2, VX(0,6)+r3,  GBIAS     ;; mid10 even green
      vadds    V(48,32+3)+r2,  VX(16,6)+r3, BBIAS     ;; mid10 even blue

      add      r3, 1
      addcmpbne r2, 3, 15, .quadstep1

      ;; fix-ups
      vadds    V(48,15), VX(32,5), RBIAS              ;; pixel10 even red
      vadds    V(48,31), VX(0,5),  GBIAS              ;; pixel10 even green
      vadds    V(48,32), VX(16,5), BBIAS              ;; pixel10 even blue

      vadds    V(48,63), VX(32,42), RBIAS             ;; pixel21 odd red

      add      r5, pc, pcrel(.twofour)
      ld       r3, (sp+16)
      vld      VX(0,0), (r5)
      vsub     -, V(0,0), r3                    SETF        ; C = i < r3
      vsub     VX(0,0), V(0,16), r3
      vand     -, VX(0,0), 0x8000               SETF        ; N = i < r3

      mov      r2, 0
      mov      r5, r0
      ld       r4, (sp+12)
      mov      r3, 64
      shl      r4, 6
.storeloop1:
      vst      HX(48,0)+r2, (r5)                IFC
      vst      HX(48,32)+r2, (r5+32)            IFN
      add      r5, r1
      addcmpbne r2, r3, r4, .storeloop1

      ;; more fix-ups
      vadds    V(48,0),  VX(0,42),  GBIAS             ;; pixel21 odd green
      vadds    V(48,16), VX(16,42), BBIAS             ;; pixel21 odd blue

      mov      r2, 0                                  ;; output offset
      mov      r3, 0                                  ;; input offset
.quadstep2:
      vadds    V(48,1)+r2,  VX(32,11)+r3, RBIAS       ;; right10 even red
      vadds    V(48,17)+r2, VX(0,11)+r3,  GBIAS       ;; right10 even green
      vadds    V(48,2)+r2,  VX(16,11)+r3, BBIAS       ;; right10 even blue

      vadds    V(48,18)+r2, VX(32,43)+r3, RBIAS       ;; right10 odd red
      vadds    V(48,3)+r2,  VX(0,43)+r3,  GBIAS       ;; right10 odd green
      vadds    V(48,19)+r2, VX(16,43)+r3, BBIAS       ;; right10 odd blue

      add      r3, 1
      addcmpbne r2, 3, 15, .quadstep2

      add      r5, pc, pcrel(.six)
      ld       r3, (sp+16)
      vld      V(0,0), (r5)
      vsub     -, V(0,0), r3              SETF

      mov      r2, 0
      mov      r3, 64
.storeloop2:
      vst      HX(48,0)+r2, (r0+64)       IFN
      add      r0, r1
      addcmpbne r2, r3, r4, .storeloop2
      b        lr

.align 32
.twofour:.byte  0, 10,  0, 10,  0, 12,  2, 12,  2, 12,  2, 14,  4, 14,  4, 14,  4, 16,  6, 16,  6, 16,  6, 18,  8, 18,  8, 18,  8, 20, 10, 20
.six:    .byte 20, 22, 22, 22, 24, 24, 24, 26, 26, 26, 28, 28, 28, 30, 30, 30
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgb565_store_32x16_dither
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgb565_store_32x16_dither(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...)                             (sp+8) (alpha)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 32x16 block of partially converted YUV data.
;;;
;;; RETURNS
;;;    -
;;;
;;; TODO
;;;   optimise

.section .data$rgb565_dither_state,"data"
.align 32
rgb565_dither_state: .half GBIAS, GBIAS, GBIAS, GBIAS, GBIAS, GBIAS, GBIAS, GBIAS, GBIAS, GBIAS, GBIAS, GBIAS, GBIAS, GBIAS, GBIAS, GBIAS
                     .half RBIAS, RBIAS, RBIAS, RBIAS, RBIAS, RBIAS, RBIAS, RBIAS, RBIAS, RBIAS, RBIAS, RBIAS, RBIAS, RBIAS, RBIAS, RBIAS
                     .half BBIAS, BBIAS, BBIAS, BBIAS, BBIAS, BBIAS, BBIAS, BBIAS, BBIAS, BBIAS, BBIAS, BBIAS, BBIAS, BBIAS, BBIAS, BBIAS
.globl rgb565_dither_state

function yuv_to_rgb565_store_32x16_dither ; {{{
      add      r4, pc, pcrel27(rgb565_dither_state)
      mov      r2, 0
      
      vld      HX(48,0), (r4)
      vld      HX(49,0), (r4+32)
      vld      HX(50,0), (r4+64)

      mov      r3, 4000001039
      mulhd.uu r3, r0 ; very ineffective random number generator

      vbitplanes HX(51,0), r3
      vsub     HX(48,0), HX(48,0), HX(51,0)
      vsub     HX(49,0), HX(49,0), HX(51,0)
      vsub     HX(50,0), HX(50,0), HX(51,0)

.loop:
      vadds    V(0,0)+r2, VX(0,0)+r2, HX(48,0) CLRA ACC   ;; even green
      vand     V(0,0)+r2,  V(0,0)+r2, GMASK           ;; even green mask
      vrsub    HX(48,0), V(0,0)+r2, GBIAS       ACC

      vadds    V(16,0)+r2, VX(16,0)+r2, HX(50,0) CLRA ACC ;; even blue
      vand     V(16,0)+r2,  V(16,0)+r2, BMASK         ;; even blue mask
      vrsub    HX(50,0), V(16,0)+r2, BBIAS      ACC

      vadds    V(32,0)+r2, VX(32,0)+r2, HX(49,0) CLRA ACC ;; even red
      vand     V(16,16)+r2, V(32,0)+r2, RMASK         ;; even red mask
      vrsub    HX(49,0), V(16,16)+r2, RBIAS     ACC

      vadds    V(0,32)+r2, VX(0,32)+r2, HX(48,0) CLRA ACC ;; odd green
      vand     V(0,32)+r2,  V(0,32)+r2, GMASK         ;; odd green mask
      vrsub    HX(48,0), V(0,32)+r2, GBIAS      ACC

      vadds    V(16,32)+r2, VX(16,32)+r2, HX(50,0) CLRA ACC ;; odd blue
      vand     V(16,32)+r2,  V(16,32)+r2, BMASK       ;; odd blue mask
      vrsub    HX(50,0), V(16,32)+r2, BBIAS     ACC

      vadds    V(32,32)+r2, VX(32,32)+r2, HX(49,0) CLRA ACC ;; odd red
      vand     V(16,48)+r2,  V(32,32)+r2, RMASK       ;; odd red mask
      vrsub    HX(49,0), V(16,48)+r2, RBIAS     ACC

      addcmpbne r2, 1, 16, .loop

      vst      HX(48,0), (r4)
      vst      HX(49,0), (r4+32)
      vst      HX(50,0), (r4+64)

      vshl     VX(0,0++),  V(0,0++), 5-2     REP 16   ;; even green centre
      vshl     VX(0,32++), V(0,32++), 5-2    REP 16   ;; odd green centre

      vlsr     V(16,0++),  V(16,0++), 3      REP 16   ;; even blue shift
      vlsr     V(16,32++), V(16,32++), 3     REP 16   ;; odd blue shift

      vor      VX(48,0++),  VX(16,0++),  VX(0,0++)  REP 16
      vor      VX(48,32++), VX(16,32++), VX(0,32++) REP 16

      VST_H32_REP   <(48++,0)>, r0, r1, 16

      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgb565_store_16x16_dither
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgb565_store_16x16_dither(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...)                             (sp+8) (alpha)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 16x16 block of partially converted YUV data.
;;;
;;; RETURNS
;;;    -
;;;
;;; TODO
;;;   optimise

function yuv_to_rgb565_store_16x16_dither ; {{{
      add      r4, pc, pcrel27(rgb565_dither_state)
      mov      r2, 0
      
      vld      HX(48,0), (r4)
      vld      HX(49,0), (r4+32)
      vld      HX(50,0), (r4+64)

      mov      r3, 4000001039
      mulhd.uu r3, r0

      vbitplanes HX(51,0), r3
      vadd     HX(48,0), HX(48,0), HX(51,0)
      vadd     HX(49,0), HX(49,0), HX(51,0)
      vadd     HX(50,0), HX(50,0), HX(51,0)

.loop:
      vadds    V(0,0)+r2, VX(0,0)+r2, HX(48,0) CLRA ACC   ;; even green
      vand     V(0,0)+r2,  V(0,0)+r2, GMASK           ;; even green mask
      vrsub    HX(48,0), V(0,0)+r2, GBIAS          ACC

      vadds    V(16,0)+r2, VX(16,0)+r2, HX(50,0) CLRA ACC ;; even blue
      vand     V(16,0)+r2,  V(16,0)+r2, BMASK         ;; even blue mask
      vrsub    HX(50,0), V(16,0)+r2, BBIAS         ACC

      vadds    V(32,0)+r2, VX(32,0)+r2, HX(49,0) CLRA ACC ;; even red
      vand     V(16,16)+r2, V(32,0)+r2, RMASK         ;; even red mask
      vrsub    HX(49,0), V(16,16)+r2, RBIAS        ACC

      vadds    V(0,32)+r2, VX(0,32)+r2, HX(48,0) CLRA ACC ;; odd green
      vand     V(0,32)+r2,  V(0,32)+r2, GMASK         ;; odd green mask
      vrsub    HX(48,0), V(0,32)+r2, GBIAS         ACC

      vadds    V(16,32)+r2, VX(16,32)+r2, HX(50,0) CLRA ACC ;; odd blue
      vand     V(16,32)+r2,  V(16,32)+r2, BMASK       ;; odd blue mask
      vrsub    HX(50,0), V(16,32)+r2, BBIAS        ACC

      vadds    V(32,32)+r2, VX(32,32)+r2, HX(49,0) CLRA ACC ;; odd red
      vand     V(16,48)+r2,  V(32,32)+r2, RMASK       ;; odd red mask
      vrsub    HX(49,0), V(16,48)+r2, RBIAS        ACC

      addcmpbne r2, 1, 8, .loop

      vst      HX(48,0), (r4)
      vst      HX(49,0), (r4+32)
      vst      HX(50,0), (r4+64)

      vshl     VX(0,0++),  V(0,0++), 5-2     REP 8    ;; even green centre
      vshl     VX(0,32++), V(0,32++), 5-2    REP 8    ;; odd green centre

      vlsr     V(16,0++),  V(16,0++), 3      REP 8    ;; even blue shift
      vlsr     V(16,32++), V(16,32++), 3     REP 8    ;; odd blue shift

      vor      VX(48,0++),  VX(16,0++),  VX(0,0++)  REP 8
      vor      VX(48,32++), VX(16,32++), VX(0,32++) REP 8

.if _VC_VERSION > 2
      v16interl HX(48++,0), HX(48++,0), HX(48++,32) REP 16
      vst       HX(48++,0), (r0+=r1) REP 16
.else
      vst32l   HX(48++,0), (r0+=r1) REP 16
.endif
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgb565_store_32x16
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgb565_store_32x16(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...)                             (sp+8) (alpha)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 32x16 block of partially converted YUV data.
;;;
;;; RETURNS
;;;    -
;;;
;;; TODO
;;;   optimise

function yuv_to_rgb565_store_32x16 ; {{{
      vadds    V(0,0++),  VX(0,0++),  GBIAS  REP 16   ;; even green
      vadds    V(0,32++), VX(0,32++), GBIAS  REP 16   ;; odd green

      vadds    V(16,0++),  VX(16,0++),  BBIAS REP 16  ;; even blue
      vadds    V(16,32++), VX(16,32++), BBIAS REP 16  ;; odd blue

      vadds    V(32,0++),  VX(32,0++),  RBIAS REP 16  ;; even red
      vadds    V(32,32++), VX(32,32++), RBIAS REP 16  ;; odd red

      vand     V(0,0++),  V(0,0++),  GMASK   REP 16   ;; even green mask
      vand     V(0,32++), V(0,32++), GMASK   REP 16   ;; odd green mask

.if BMASK < 0xF8
      vand     V(16,0++),  V(16,0++),  BMASK REP 16   ;; even blue mask
      vand     V(16,32++), V(16,32++), BMASK REP 16   ;; odd blue mask
.endif

      vand     V(16,16++), V(32,0++),  RMASK REP 16   ;; even red mask
      vand     V(16,48++), V(32,32++), RMASK REP 16   ;; odd red mask

      vshl     VX(0,0++), V(0,0++), 5-2      REP 16   ;; even green centre
      vshl     VX(0,32++), V(0,32++), 5-2    REP 16   ;; odd green centre

      vlsr     V(16,0++), V(16,0++), 3       REP 16   ;; even blue shift
      vlsr     V(16,32++), V(16,32++), 3     REP 16   ;; odd blue shift

      vor      VX(48,0++), VX(16,0++), VX(0,0++) REP 16
      vor      VX(48,32++), VX(16,32++), VX(0,32++) REP 16

      VST_H32_REP <(48++,0)>, r0, r1, 16
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgb565_store_16x16
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgb565_store_16x16(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...)                             (sp+8) (alpha)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 16x16 block of partially converted YUV data.
;;;
;;; RETURNS
;;;   -
;;;
;;; TODO
;;;   optimise

function yuv_to_rgb565_store_16x16 ; {{{
      vadds    V(0,0++),  VX(0,0++),  GBIAS  REP 8    ;; even green
      vadds    V(0,32++), VX(0,32++), GBIAS  REP 8    ;; odd green

      vadds    V(16,0++),  VX(16,0++),  BBIAS REP 8   ;; even blue
      vadds    V(16,32++), VX(16,32++), BBIAS REP 8   ;; odd blue

      vadds    V(32,0++),  VX(32,0++),  RBIAS REP 8   ;; even red
      vadds    V(32,32++), VX(32,32++), RBIAS REP 8   ;; odd red

      vand     V(0,0++),  V(0,0++),  GMASK   REP 8    ;; even green mask
      vand     V(0,32++), V(0,32++), GMASK   REP 8    ;; odd green mask

.if BMASK < 0xF8
      vand     V(16,0++),  V(16,0++),  BMASK REP 8    ;; even blue mask
      vand     V(16,32++), V(16,32++), BMASK REP 8    ;; odd blue mask
.endif

      vand     V(16,16++), V(32,0++),  RMASK REP 8    ;; even red mask
      vand     V(16,48++), V(32,32++), RMASK REP 8    ;; odd red mask

      vshl     VX(0,0++), V(0,0++), 5-2      REP 8    ;; even green centre
      vshl     VX(0,32++), V(0,32++), 5-2    REP 8    ;; odd green centre

      vlsr     V(16,0++), V(16,0++), 3       REP 8    ;; even blue shift
      vlsr     V(16,32++), V(16,32++), 3     REP 8    ;; odd blue shift

      vor      VX(48,0++), VX(16,0++), VX(0,0++) REP 8 
      vor      VX(48,32++), VX(16,32++), VX(0,32++) REP 8 

.if _VC_VERSION > 2
      v16interl HX(48++,0), HX(48++,0), HX(48++,32) REP 16
      vst       HX(48++,0), (r0+=r1) REP 16
.else
      vst32l   HX(48++,0), (r0+=r1) REP 16
.endif
      b        lr
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgb565_store_32xn_misaligned_dither
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgb565_store_32xn_misaligned_dither(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...,
;;;      int                  arg1,       (sp+8) (ignored)
;;;      int                  lines);     (sp+12)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 32x(lines) block of partially converted YUV data.
;;;
;;; RETURNS
;;;    -
;;;
;;; TODO
;;;   optimise

function yuv_to_rgb565_store_32xn_misaligned_dither ; {{{
      add      r4, pc, pcrel27(rgb565_dither_state)
      mov      r2, 0
      
      vld      HX(48,0), (r4)
      vld      HX(49,0), (r4+32)
      vld      HX(50,0), (r4+64)

      mov      r3, 4000001039
      mulhd.uu r3, r0 ; very ineffective random number generator

      vbitplanes HX(51,0), r3
      vsub     HX(48,0), HX(48,0), HX(51,0)
      vsub     HX(49,0), HX(49,0), HX(51,0)
      vsub     HX(50,0), HX(50,0), HX(51,0)

.loop:
      vadds    V(0,0)+r2, VX(0,0)+r2, HX(48,0) CLRA ACC   ;; even green
      vand     V(0,0)+r2,  V(0,0)+r2, GMASK           ;; even green mask
      vrsub    HX(48,0), V(0,0)+r2, GBIAS       ACC

      vadds    V(16,0)+r2, VX(16,0)+r2, HX(50,0) CLRA ACC ;; even blue
      vand     V(16,0)+r2,  V(16,0)+r2, BMASK         ;; even blue mask
      vrsub    HX(50,0), V(16,0)+r2, BBIAS      ACC

      vadds    V(32,0)+r2, VX(32,0)+r2, HX(49,0) CLRA ACC ;; even red
      vand     V(16,16)+r2, V(32,0)+r2, RMASK         ;; even red mask
      vrsub    HX(49,0), V(16,16)+r2, RBIAS     ACC

      vadds    V(0,32)+r2, VX(0,32)+r2, HX(48,0) CLRA ACC ;; odd green
      vand     V(0,32)+r2,  V(0,32)+r2, GMASK         ;; odd green mask
      vrsub    HX(48,0), V(0,32)+r2, GBIAS      ACC

      vadds    V(16,32)+r2, VX(16,32)+r2, HX(50,0) CLRA ACC ;; odd blue
      vand     V(16,32)+r2,  V(16,32)+r2, BMASK       ;; odd blue mask
      vrsub    HX(50,0), V(16,32)+r2, BBIAS     ACC

      vadds    V(32,32)+r2, VX(32,32)+r2, HX(49,0) CLRA ACC ;; odd red
      vand     V(16,48)+r2,  V(32,32)+r2, RMASK       ;; odd red mask
      vrsub    HX(49,0), V(16,48)+r2, RBIAS     ACC

      addcmpbne r2, 1, 16, .loop

      vst      HX(48,0), (r4)
      vst      HX(49,0), (r4+32)
      vst      HX(50,0), (r4+64)

      vshl     VX(0,0++),  V(0,0++), 5-2     REP 16   ;; even green centre
      vshl     VX(0,32++), V(0,32++), 5-2    REP 16   ;; odd green centre

      vlsr     V(16,0++),  V(16,0++), 3      REP 16   ;; even blue shift
      vlsr     V(16,32++), V(16,32++), 3     REP 16   ;; odd blue shift

      mov      r2, 0
      mov      r3, 0
.iloop:
      vor      VX(48,0)+r2, VX(16,0)+r3, VX(0,0)+r3
      vor      VX(48,1)+r2, VX(16,32)+r3, VX(0,32)+r3

      add      r2, 2
      addcmpbne r3, 1, 16, .iloop

      mov      r2, 0
      mov      r3, 64
      ld       r4, (sp+12)
      shl      r4, 6
.storeloop:
      vst      HX(48,0)+r2, (r0)
      vst      HX(48,32)+r2, (r0+32)
      add      r0, r1
      addcmpbne r2, r3, r4, .storeloop

      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgb565_store_mxn_misaligned_dither
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgb565_store_mxn_misaligned_dither(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...,
;;;      int                  arg1,       (sp+8) (ignored)
;;;      int                  lines,      (sp+12)
;;;      int                  columns);   (sp+16)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a (columns)x(lines) block of partially converted YUV data.
;;;
;;; RETURNS
;;;    -
;;;
;;; TODO
;;;   optimise

function yuv_to_rgb565_store_mxn_misaligned_dither ; {{{
      add      r4, pc, pcrel27(rgb565_dither_state)
      mov      r2, 0
      
      vld      HX(48,0), (r4)
      vld      HX(49,0), (r4+32)
      vld      HX(50,0), (r4+64)

      mov      r3, 4000001039
      mulhd.uu r3, r0

      vbitplanes HX(51,0), r3
      vadd     HX(48,0), HX(48,0), HX(51,0)
      vadd     HX(49,0), HX(49,0), HX(51,0)
      vadd     HX(50,0), HX(50,0), HX(51,0)

.loop:
      vadds    V(0,0)+r2, VX(0,0)+r2, HX(48,0) CLRA ACC   ;; even green
      vand     V(0,0)+r2,  V(0,0)+r2, GMASK           ;; even green mask
      vrsub    HX(48,0), V(0,0)+r2, GBIAS          ACC

      vadds    V(16,0)+r2, VX(16,0)+r2, HX(50,0) CLRA ACC ;; even blue
      vand     V(16,0)+r2,  V(16,0)+r2, BMASK         ;; even blue mask
      vrsub    HX(50,0), V(16,0)+r2, BBIAS         ACC

      vadds    V(32,0)+r2, VX(32,0)+r2, HX(49,0) CLRA ACC ;; even red
      vand     V(16,16)+r2, V(32,0)+r2, RMASK         ;; even red mask
      vrsub    HX(49,0), V(16,16)+r2, RBIAS        ACC

      vadds    V(0,32)+r2, VX(0,32)+r2, HX(48,0) CLRA ACC ;; odd green
      vand     V(0,32)+r2,  V(0,32)+r2, GMASK         ;; odd green mask
      vrsub    HX(48,0), V(0,32)+r2, GBIAS         ACC

      vadds    V(16,32)+r2, VX(16,32)+r2, HX(50,0) CLRA ACC ;; odd blue
      vand     V(16,32)+r2,  V(16,32)+r2, BMASK       ;; odd blue mask
      vrsub    HX(50,0), V(16,32)+r2, BBIAS        ACC

      vadds    V(32,32)+r2, VX(32,32)+r2, HX(49,0) CLRA ACC ;; odd red
      vand     V(16,48)+r2,  V(32,32)+r2, RMASK       ;; odd red mask
      vrsub    HX(49,0), V(16,48)+r2, RBIAS        ACC

      addcmpbne r2, 1, 16, .loop

      vst      HX(48,0), (r4)
      vst      HX(49,0), (r4+32)
      vst      HX(50,0), (r4+64)

      vshl     VX(0,0++),  V(0,0++), 5-2     REP 16   ;; even green centre
      vshl     VX(0,32++), V(0,32++), 5-2    REP 16   ;; odd green centre

      vlsr     V(16,0++),  V(16,0++), 3      REP 16   ;; even blue shift
      vlsr     V(16,32++), V(16,32++), 3     REP 16   ;; odd blue shift

      mov      r2, 0
      mov      r3, 0
.iloop:
      vor      VX(48,0)+r2, VX(16,0)+r3, VX(0,0)+r3
      vor      VX(48,1)+r2, VX(16,32)+r3, VX(0,32)+r3

      add      r2, 2
      addcmpbne r3, 1, 16, .iloop

      add      r2, pc, pcrel(.onetwothree)
      ld       r3, (sp+16)                          ;; columns
      vld      H(0,0), (r2)
      vsub     HX(0,0), H(0,0), r3                 SETF     ; C = i < r5
      vadd     HX(0,0), HX(0,0), 16
      vand     -, HX(0,0), 0x8000                  SETF     ; N = i + 8 < r5

      mov      r2, 0
      mov      r3, 64
      ld       r4, (sp+12)
      shl      r4, 6
.storeloop:
      vst      HX(48,0)+r2, (r0)                   IFC
      vst      HX(48,32)+r2, (r0+32)               IFN
      add      r0, r1
      addcmpbne r2, r3, r4, .storeloop

      b        lr

.align 16
.onetwothree: .byte 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgb565_store_32xn_misaligned
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgb565_store_32xn_misaligned(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...,
;;;      int                  arg1,       (sp+8) (ignored)
;;;      int                  lines);     (sp+12)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a 32x(lines) block of partially converted YUV data.
;;;
;;; RETURNS
;;;    -
;;;
;;; TODO
;;;   optimise

function yuv_to_rgb565_store_32xn_misaligned ; {{{
      vadds    V(0,0++),  VX(0,0++),  GBIAS  REP 16   ;; even green
      vadds    V(0,32++), VX(0,32++), GBIAS  REP 16   ;; odd green

      vadds    V(16,0++),  VX(16,0++),  BBIAS REP 16  ;; even blue
      vadds    V(16,32++), VX(16,32++), BBIAS REP 16  ;; odd blue

      vadds    V(32,0++),  VX(32,0++),  RBIAS REP 16  ;; even red
      vadds    V(32,32++), VX(32,32++), RBIAS REP 16  ;; odd red

      vand     V(0,0++),  V(0,0++),  GMASK   REP 16   ;; even green mask
      vand     V(0,32++), V(0,32++), GMASK   REP 16   ;; odd green mask

.if BMASK < 0xF8
      vand     V(16,0++),  V(16,0++),  BMASK REP 16   ;; even blue mask
      vand     V(16,32++), V(16,32++), BMASK REP 16   ;; odd blue mask
.endif

      vand     V(16,16++), V(32,0++),  RMASK REP 16   ;; even red mask
      vand     V(16,48++), V(32,32++), RMASK REP 16   ;; odd red mask

      vshl     VX(0,0++), V(0,0++), 5-2      REP 16   ;; even green centre
      vshl     VX(0,32++), V(0,32++), 5-2    REP 16   ;; odd green centre

      vlsr     V(16,0++), V(16,0++), 3       REP 16   ;; even blue shift
      vlsr     V(16,32++), V(16,32++), 3     REP 16   ;; odd blue shift

      mov      r2, 0
      mov      r3, 0
.iloop:
      vor      VX(48,0)+r2, VX(16,0)+r3, VX(0,0)+r3
      vor      VX(48,1)+r2, VX(16,32)+r3, VX(0,32)+r3

      add      r2, 2
      addcmpbne r3, 1, 16, .iloop

      mov      r2, 0
      mov      r3, 64
      ld       r4, (sp+12)
      shl      r4, 6
.storeloop:
      vst      HX(48,0)+r2, (r0)
      vst      HX(48,32)+r2, (r0+32)
      add      r0, r1
      addcmpbne r2, r3, r4, .storeloop

      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuv_to_rgb565_store_mxn_misaligned
;;;
;;; SYNOPSIS
;;;   void yuv_to_rgb565_store_mxn_misaligned(
;;;      unsigned char       *dest,       r0
;;;      int                  pitch,      r1
;;;      ...,
;;;      int                  arg1,       (sp+8) (ignored)
;;;      int                  lines,      (sp+12)
;;;      int                  columns);   (sp+16)
;;;
;;; FUNCTION
;;;   Adjust, clip, and store a (columns)x(lines) block of partially converted YUV data.
;;;
;;; RETURNS
;;;    -
;;;
;;; TODO
;;;   optimise

function yuv_to_rgb565_store_mxn_misaligned ; {{{
      vadds    V(0,0++),  VX(0,0++),  GBIAS  REP 16   ;; even green
      vadds    V(0,32++), VX(0,32++), GBIAS  REP 16   ;; odd green

      vadds    V(16,0++),  VX(16,0++),  BBIAS REP 16  ;; even blue
      vadds    V(16,32++), VX(16,32++), BBIAS REP 16  ;; odd blue

      vadds    V(32,0++),  VX(32,0++),  RBIAS REP 16  ;; even red
      vadds    V(32,32++), VX(32,32++), RBIAS REP 16  ;; odd red

      vand     V(0,0++),  V(0,0++),  GMASK   REP 16   ;; even green mask
      vand     V(0,32++), V(0,32++), GMASK   REP 16   ;; odd green mask

.if BMASK < 0xF8
      vand     V(16,0++),  V(16,0++),  BMASK REP 16   ;; even blue mask
      vand     V(16,32++), V(16,32++), BMASK REP 16   ;; odd blue mask
.endif

      vand     V(16,16++), V(32,0++),  RMASK REP 16   ;; even red mask
      vand     V(16,48++), V(32,32++), RMASK REP 16   ;; odd red mask

      vshl     VX(0,0++), V(0,0++), 5-2      REP 16   ;; even green centre
      vshl     VX(0,32++), V(0,32++), 5-2    REP 16   ;; odd green centre

      vlsr     V(16,0++), V(16,0++), 3       REP 16   ;; even blue shift
      vlsr     V(16,32++), V(16,32++), 3     REP 16   ;; odd blue shift

      mov      r2, 0
      mov      r3, 0
.iloop:
      vor      VX(48,0)+r2, VX(16,0)+r3, VX(0,0)+r3
      vor      VX(48,1)+r2, VX(16,32)+r3, VX(0,32)+r3

      add      r2, 2
      addcmpbne r3, 1, 16, .iloop

      add      r2, pc, pcrel(.onetwothree)
      ld      r3, (sp+16)                          ;; columns
      vld      H(0,0), (r2)
      vsub     HX(0,0), H(0,0), r3                 SETF     ; C = i < r5
      vadd     HX(0,0), HX(0,0), 16
      vand     -, HX(0,0), 0x8000                  SETF     ; N = i + 8 < r5

      mov      r2, 0
      mov      r3, 64
      ld       r4, (sp+12)
      shl      r4, 6
.storeloop:
      vst      HX(48,0)+r2, (r0)                   IFC
      vst      HX(48,32)+r2, (r0+32)               IFN
      add      r0, r1
      addcmpbne r2, r3, r4, .storeloop

      b        lr

.align 16
.onetwothree: .byte 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
endfunc ; }}}
.FORGET

