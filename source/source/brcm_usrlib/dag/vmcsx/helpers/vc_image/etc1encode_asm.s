.include "vcinclude/common.inc"

.define R_WEIGHT, 19
.define G_WEIGHT, 37
.define B_WEIGHT, 7
.set TOTAL_WEIGHT, R_WEIGHT+G_WEIGHT+B_WEIGHT

;;; *****************************************************************************
;;; NAME
;;;   etc1_block_score
;;;
;;; SYNOPSIS
;;;   extern unsigned long long etc1_block_score(
;;;      int                  block_index,   /* r0 */
;;;      int                  luma1,         /* r1 */
;;;      int                  luma2,         /* r2 */
;;;      int                  planes);       /* r3 */
;;;
;;; FUNCTION
;;;   blah
;;;
;;; RETURNS
;;;   blah

.function etc1_block_score ; {{{
         vbitplanes -, r3                             SETF

         vdists   H16(47,0), H16(48,0)+r0, r1         IFNZ
         vdists   H16(47,0), H16(48,0)+r0, r2         IFZ

         vdists   H16(0,0),  H16(47,0), TOTAL_WEIGHT * 2
         vdists   H16(0,32), H16(47,0), TOTAL_WEIGHT * 8
         vdists   H16(1,0),  H16(47,0), TOTAL_WEIGHT * 5
         vdists   H16(1,32), H16(47,0), TOTAL_WEIGHT * 17
         vdists   H16(2,0),  H16(47,0), TOTAL_WEIGHT * 9
         vdists   H16(2,32), H16(47,0), TOTAL_WEIGHT * 29
         vdists   H16(3,0),  H16(47,0), TOTAL_WEIGHT * 13
         vdists   H16(3,32), H16(47,0), TOTAL_WEIGHT * 42
         vdists   H16(4,0),  H16(47,0), TOTAL_WEIGHT * 18
         vdists   H16(4,32), H16(47,0), TOTAL_WEIGHT * 60
         vdists   H16(5,0),  H16(47,0), TOTAL_WEIGHT * 24
         vdists   H16(5,32), H16(47,0), TOTAL_WEIGHT * 80
         vdists   H16(6,0),  H16(47,0), TOTAL_WEIGHT * 33
         vdists   H16(6,32), H16(47,0), TOTAL_WEIGHT * 106
         vdists   H16(7,0),  H16(47,0), TOTAL_WEIGHT * 47
         vdists   H16(7,32), H16(47,0), TOTAL_WEIGHT * 183

         vmin     H16(0++,0), H16(0++,0), H16(0++,32) REP 8
         v32mul.ss H32(0++,0), H16(0++,0), H16(0++,0) REP 8

         cmp      r3, 0x00ff
         vbitplanes -, 0x00ff                         SETF

         vadds    V32(0,0++), V32(0,0++), V32(0,4++)  IFNZ REP 4
         vadds    V32(0,8++), V32(0,8++), V32(0,12++) IFNZ REP 4

         vadds    V32(0,0), V32(0,0), V32(0,1)        IFNZ
         vadds    V32(0,2), V32(0,2), V32(0,3)        IFNZ
         vadds    V32(0,8), V32(0,8), V32(0,9)        IFNZ
         vadds    V32(0,10), V32(0,10), V32(0,11)     IFNZ
         bne      .gappy

         vadds    V32(0,0), V32(0,0), V32(0,1)        IFNZ IMIN r1
         vadds    V32(0,8), V32(0,8), V32(0,10)       IFNZ IMIN r2
         vrsubs   -, V32(0,0), 0                      IFNZ MAX r3
         vrsubs   -, V32(0,8), 0                      IFNZ MAX r4
         b        .finish

.gappy:  vadds    V32(0,0), V32(0,0), V32(0,8)        IFNZ IMIN r1
         vadds    V32(0,2), V32(0,2), V32(0,10)       IFNZ IMIN r2
         vrsubs   -, V32(0,0), 0                      IFNZ MAX r3
         vrsubs   -, V32(0,2), 0                      IFNZ MAX r4

.finish: addscale r1, r2 << 4
         adds     r0, r3, r4
         neg      r0
         b        lr

.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   etc1_block_pack
;;;
;;; SYNOPSIS
;;;   extern unsigned long etc1_block_pack(
;;;      int                  block_index,   /* r0 */
;;;      int                  luma1,         /* r1 */
;;;      int                  luma2,         /* r2 */
;;;      int                  thresh1,       /* r3 */
;;;      int                  thresh2,       /* r4 */
;;;      int                  planes);       /* r5 */
;;;
;;; FUNCTION
;;;   blah
;;;
;;; RETURNS
;;;   blah

.function etc1_block_pack ; {{{
         vbitplanes     -, r5                         SETF
         vmov     H16(45,0), r3                       IFNZ
         vmov     H16(45,0), r4                       IFZ
         vmov     H16(46,0), r1                       IFNZ
         vmov     H16(46,0), r2                       IFZ

         mov      r5, 0x10000
         vmov     H32(0,0), 0

         vdists   H16(47,0), H16(48,0)+r0, H16(46,0)

.ifdef __BCM2707A0__
         vsub     -,         H16(48,0)+r0, H16(46,0)  SETF
         vor      H32(0,0),  H32(0,0), r5             IFC
         vrsub    -,         H16(47,0), H16(45,0)     SETF
         vor      H32(0,0),  H32(0,0), 1              IFC
.else
         vsub     -,         H16(48,0)+r0, H16(46,0)  SETF
         vor      H32(0,0),  H32(0,0), 1              IFC
         vrsub    -,         H16(47,0), H16(45,0)     SETF
         vor      H32(0,0),  H32(0,0), r5             IFC
.endif

         v32shl     -, H32(0,0), H(44,0)              USUM r0
         b        lr
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   etc1_load_yuv420
;;;
;;; SYNOPSIS
;;;   extern unsigned long etc1_load_yuv420(
;;;         void                *output,        /* r0 */
;;;         unsigned char const *y_pixels,      /* r1 */
;;;         int                  pitch,         /* r2 */
;;;         int                  rows,          /* r3 */
;;;         int                  columns,       /* r4 */
;;;         unsigned char const *u_pixels,      /* r5 */
;;;         unsigned char const *v_pixels,      /* (sp) */
;;;         int                  uv_pitch);     /* (sp+4) */
;;;   
;;;
;;; FUNCTION
;;;   blah
;;;
;;; RETURNS
;;;   blah

.function etc1_load_yuv420 ; {{{
         stm      r6-r8, (--sp)
         .cfa_push {%r6,%r7,%r8}
         mov      r6, r5
         ld       r7, (sp+12)
         ld       r8, (sp+16)
         vbitplanes -, 0x00FF                         SETF

         rsub     r3, 16
         mov      r5, r0
         rsub     r0, r3, 16
         shl      r3, 6
         vld      H(0++,0)+r3, (r1+=r2)               REP r0

         lsr      r0, 1
         shl      r3, 1

         vld      H(0++,32)+r3, (r6+=r8)              REP r0 IFNZ
         vld      H(0++,48)+r3, (r7+=r8)              REP r0 IFNZ

         vinterl  H(0++,32), H(0++,32), H(0++,48)     REP 8
         vinterl  V(0,32++), V(0,32++), V(0,32++)     REP 16 ; could unroll this, but I'm lazy.

         ldm      r6-r8, (sp++)
         .cfa_pop {%r8,%r7,%r6}

etc1_yuv_loaded:
.globl etc1_yuv_loaded

         mov      r0, r5
         mov      r5, -64
         shl      r4, 2
         addcmpbgt r3, 0, 0, .fvloop
         b        .fvskip
.fvloop: vmov     H32(63,0)+r3, H32(0,0)+r3
         addcmpbgt r3, r5, 0, .fvloop
.fvskip: addcmpblo r4, 0, 16, .fhloop
         b        .fhskip
.fhloop: vmov     V32(0,0)+r4, V32(0,15)+r4
         addcmpblo r4, 1, 16, .fhloop
.fhskip:
         mov      r1, 0
         mov      r2, 0
         mov      r3, 0


;; disabling this seems to work best in testetc1 -- I'm not sure about the
;; conversion paths, but at least it's consistent with _something_.
;.define WANT_JFIF_COLOURSPACE

.ifdef WANT_JFIF_COLOURSPACE
.define YSCALE,    8192  ;;  256 * 32 * 1.0000000
.define bUSCALE,  16531  ;;  256 * 32 * 2.0180000
.define gUSCALE,  -2819  ;; -256 * 32 * 0.3441400
.define rVSCALE,  11485  ;;  256 * 32 * 1.4020000
.define gVSCALE,  -5850  ;; -256 * 32 * 0.7141400

.define YBIAS,        0
.define RBIAS,     -179  ;; -(YSCALE * 0                 + rVSCALE * 128) / 256
.define GBIAS,      135  ;; -(YSCALE * 0 - gUSCALE * 128 - gVSCALE * 128) / 256
.define BBIAS,     -258  ;; -(YSCALE * 0 + bUSCALE * 128                ) / 256
.else
.define YSCALE,    9539  ;;  256 * 32 * 1.1643835
.define bUSCALE,  16525  ;;  256 * 32 * 2.0172321
.define gUSCALE,  -3209  ;; -256 * 32 * 0.3917623
.define rVSCALE,  13075  ;;  256 * 32 * 1.5960267
.define gVSCALE,  -6659  ;; -256 * 32 * 0.8129676

.define YBIAS,  -16
.define RBIAS, -223     ;; -(YSCALE * 16                 + rVSCALE * 128) / 256
.define GBIAS,  135     ;; -(YSCALE * 16 - gUSCALE * 128 - gVSCALE * 128) / 256
.define BBIAS, -277     ;; -(YSCALE * 16 + rUSCALE * 128                ) / 256
.endif

.loop:   vmov     -, YBIAS*63*YSCALE>>13              CLRA  SACC
         vmul     V16(48,0)+r2, V(0,0)+r1, 63*YSCALE>>13  NO_WB UACC
         vmul     V16(48,1)+r2, V(0,1)+r1, 63*YSCALE>>13  NO_WB UACC

         cmp      r2, 6
         add.eq   r2, 8

         v16mulm  -, V(0,0)+r1, YSCALE/2              CLRA  UACC
         v16mulm  -, V(0,1)+r1, YSCALE/2                    UACC
         v16mulm  V16(16,0)+r3, V(0,33)+r1, rVSCALE   NO_WB UACC
         v16mulm  V16(16,2)+r3, V(0,32)+r1, bUSCALE   NO_WB UACC

         v16mulm  -,            V(0,33)+r1, gVSCALE         SACC
         v16mulm  V16(16,1)+r3, V(0,32)+r1, gUSCALE         SACC

         v16asr   V16(16,0)+r3, V16(16,0)+r3, 5      - 1
         v16add   V16(16,0)+r3, V16(16,0)+r3, RBIAS << 1
         v16asr   V16(16,1)+r3, V16(16,1)+r3, 5      - 1
         v16add   V16(16,1)+r3, V16(16,1)+r3, GBIAS << 1
         v16asr   V16(16,2)+r3, V16(16,2)+r3, 5      - 1
         v16add   V16(16,2)+r3, V16(16,2)+r3, BBIAS << 1

.ifndef WANT_JFIF_COLOURSPACE
         vclip    V16(16,0)+r3, V16(16,0)+r3, 510
         vclip    V16(16,1)+r3, V16(16,1)+r3, 510
         vclip    V16(16,2)+r3, V16(16,2)+r3, 510
.endif

         add      r1, 2
         add      r3, 4
         addcmpbne r2, 2, 24, .loop

         b        etc1_loaded
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   etc1_load_yuv422
;;;
;;; SYNOPSIS
;;;   extern unsigned long etc1_load_yuv422(
;;;         void                *output,        /* r0 */
;;;         unsigned char const *y_pixels,      /* r1 */
;;;         int                  pitch,         /* r2 */
;;;         int                  rows,          /* r3 */
;;;         int                  columns,       /* r4 */
;;;         unsigned char const *u_pixels,      /* r5 */
;;;         unsigned char const *v_pixels,      /* (sp) */
;;;         int                  uv_pitch);     /* (sp+4) */
;;;   
;;;
;;; FUNCTION
;;;   blah
;;;
;;; RETURNS
;;;   blah

.function etc1_load_yuv422 ; {{{
         stm      r6-r8, (--sp)
         .cfa_push {%r6,%r7,%r8}
         mov      r6, r5
         ld       r7, (sp+12)
         ld       r8, (sp+16)
         vbitplanes -, 0x00FF                         SETF

         rsub     r3, 16
         mov      r5, r0
         rsub     r0, r3, 16
         shl      r3, 6
         vld      H(0++,0)+r3, (r1+=r2)               REP r0
         vld      H(0++,32)+r3, (r6+=r8)              REP r0 IFNZ
         vld      H(0++,48)+r3, (r7+=r8)              REP r0 IFNZ

         vinterl  H(0++,32), H(0++,32), H(0++,48)     REP 16

         ldm      r6-r8, (sp++)
         .cfa_pop {%r8,%r7,%r6}

         b        etc1_yuv_loaded
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   etc1_load_yuv_uv
;;;
;;; SYNOPSIS
;;;   extern unsigned long etc1_load_yuv_uv(
;;;         void                *output,        /* r0 */
;;;         unsigned char const *y_pixels,      /* r1 */
;;;         int                  pitch,         /* r2 */
;;;         int                  rows,          /* r3 */
;;;         int                  columns,       /* r4 */
;;;         unsigned char const *u_pixels,      /* r5 */
;;;         unsigned char const *v_pixels,      /* (sp) */
;;;         int                  uv_pitch);     /* (sp+4) */
;;;   
;;;
;;; FUNCTION
;;;   blah
;;;
;;; RETURNS
;;;   blah

.function etc1_load_yuv_uv ; {{{
         stm      r6-r7, (--sp)
         .cfa_push {%r6,%r7}
         mov      r6, r5
         ld       r7, (sp+8)

         rsub     r3, 16
         mov      r5, r0
         rsub     r0, r3, 16
         shl      r3, 6
         vld      H(0++,0)+r3, (r1+=r2)               REP r0
         vld      H(0++,32)+r3, (r6+=r7)              REP r0

         vinterl  V(0,32++), V(0,32++), V(0,32++)     REP 16 ; could unroll this, but I'm lazy.
         ldm      r6-r7, (sp++)
         .cfa_pop {%r7,%r6}

         b        etc1_yuv_loaded
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   etc1_load_rgbx5551
;;;
;;; SYNOPSIS
;;;   extern unsigned long etc1_load_rgbx5551(
;;;      void                *output,        /* r0 */
;;;      unsigned short const*pixels,        /* r1 */
;;;      int                  pitch,         /* r2 */
;;;      int                  rows,          /* r3 */
;;;      int                  columns);      /* r4 */
;;;
;;; FUNCTION
;;;   blah
;;;
;;; RETURNS
;;;   blah

.function etc1_load_rgbx5551 ; {{{
         rsub     r3, 16
         mov      r5, r0
         rsub     r0, r3, 16
         shl      r3, 6
         vld      H16(0++,0)+r3, (r1+=r2)             REP r0
         mov      r0, r5
         mov      r5, -64
         addcmpbgt r3, 0, 0, .fvloop
         b        .fvskip
.fvloop: vmov     H16(63,0)+r3, H16(0,0)+r3
         addcmpbgt r3, r5, 0, .fvloop
.fvskip: addcmpblo r4, 0, 16, .fhloop
         b        .fhskip
.fhloop: vmov     V16(0,0)+r4, V16(0,47)+r4
         addcmpblo r4, 1, 16, .fhloop
.fhskip:
         mov      r1, 0
         mov      r2, 0
         mov      r3, 0
.loop:   
         vlsr     V(32,0),    V16(0,0)+r1, 11
         vand     V16(32,1),  V16(0,0)+r1, 0x07C0
         vand     V(32,2),    V16(0,0)+r1, 62

         vlsr     V(32,3),    V16(0,1)+r1, 11
         vand     V16(32,4),  V16(0,1)+r1, 0x07C0
         vand     V(32,5),    V16(0,1)+r1, 62

         vmulm    V(32,0),   V(32,0), 0x840
         vmulm    V(32,1), V16(32,1), 0x021
         vmulm    V(32,2),   V(32,2), 0x420

         vmulm    V(32,3),   V(32,3), 0x840
         vmulm    V(32,4), V16(32,4), 0x021
         vmulm    V(32,5),   V(32,5), 0x420

         vmul     -,            V(32,0), R_WEIGHT     CLRA UACC
         vmul     -,            V(32,1), G_WEIGHT          UACC
         vmul     V16(48,0)+r2, V(32,2), B_WEIGHT          UACC

         vmul     -,            V(32,3), R_WEIGHT     CLRA UACC
         vmul     -,            V(32,4), G_WEIGHT          UACC
         vmul     V16(48,1)+r2, V(32,5), B_WEIGHT          UACC

         cmp      r2, 6
         add.eq   r2, 8

         vadd     V16(16,0)+r3, V(32,0), V(32,3)
         vadd     V16(16,1)+r3, V(32,1), V(32,4)
         vadd     V16(16,2)+r3, V(32,2), V(32,5)

         add      r1, 2
         add      r3, 4
         addcmpbne r2, 2, 24, .loop

         b        etc1_loaded
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   etc1_load_xrgb16
;;;
;;; SYNOPSIS
;;;   extern unsigned long etc1_load_xrgb16(
;;;      void                *output,        /* r0 */
;;;      unsigned short const*pixels,        /* r1 */
;;;      int                  pitch,         /* r2 */
;;;      int                  rows,          /* r3 */
;;;      int                  columns);      /* r4 */
;;;
;;; FUNCTION
;;;   blah
;;;
;;; RETURNS
;;;   blah

.function etc1_load_xrgb16 ; {{{
         rsub     r3, 16
         mov      r5, r0
         rsub     r0, r3, 16
         shl      r3, 6
         vld      H16(0++,0)+r3, (r1+=r2)             REP r0
         mov      r0, r5
         mov      r5, -64
         addcmpbgt r3, 0, 0, .fvloop
         b        .fvskip
.fvloop: vmov     H16(63,0)+r3, H16(0,0)+r3
         addcmpbgt r3, r5, 0, .fvloop
.fvskip: addcmpblo r4, 0, 16, .fhloop
         b        .fhskip
.fhloop: vmov     V16(0,0)+r4, V16(0,47)+r4
         addcmpblo r4, 1, 16, .fhloop
.fhskip:
         mov      r1, 0
         mov      r2, 0
         mov      r3, 0
.loop:   vand     V(32,0), V(0,16)+r1, 15
         vlsr     V(32,1), V(0,0)+r1, 4
         vand     V(32,2), V(0,0)+r1, 15

         vand     V(32,3), V(0,17)+r1, 15
         vlsr     V(32,4), V(0,1)+r1, 4
         vand     V(32,5), V(0,1)+r1, 15

         vmul     -,            V(32,0), R_WEIGHT*17  CLRA UACC
         vmul     -,            V(32,1), G_WEIGHT*17       UACC
         vmul     V16(48,0)+r2, V(32,2), B_WEIGHT*17       UACC

         vmul     -,            V(32,3), R_WEIGHT*17  CLRA UACC
         vmul     -,            V(32,4), G_WEIGHT*17       UACC
         vmul     V16(48,1)+r2, V(32,5), B_WEIGHT*17       UACC

         cmp      r2, 6
         add.eq   r2, 8

         vadd     V16(16,0)+r3, V(32,0), V(32,3)
         vadd     V16(16,1)+r3, V(32,1), V(32,4)
         vadd     V16(16,2)+r3, V(32,2), V(32,5)

         add      r1, 2
         add      r3, 4
         addcmpbne r2, 2, 24, .loop

         vmul     V16(16,0++), V16(16,0++), 0x11      REP 32

         b        etc1_loaded
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   etc1_load_rgbx16
;;;
;;; SYNOPSIS
;;;   extern unsigned long etc1_load_rgbx16(
;;;      void                *output,        /* r0 */
;;;      unsigned short const*pixels,        /* r1 */
;;;      int                  pitch,         /* r2 */
;;;      int                  rows,          /* r3 */
;;;      int                  columns);      /* r4 */
;;;
;;; FUNCTION
;;;   blah
;;;
;;; RETURNS
;;;   blah

.function etc1_load_rgbx16 ; {{{
         rsub     r3, 16
         mov      r5, r0
         rsub     r0, r3, 16
         shl      r3, 6
         vld      H16(0++,0)+r3, (r1+=r2)             REP r0
         mov      r0, r5
         mov      r5, -64
         addcmpbgt r3, 0, 0, .fvloop
         b        .fvskip
.fvloop: vmov     H16(63,0)+r3, H16(0,0)+r3
         addcmpbgt r3, r5, 0, .fvloop
.fvskip: addcmpblo r4, 0, 16, .fhloop
         b        .fhskip
.fhloop: vmov     V16(0,0)+r4, V16(0,47)+r4
         addcmpblo r4, 1, 16, .fhloop
.fhskip:
         mov      r1, 0
         mov      r2, 0
         mov      r3, 0
.loop:   
         vlsr     V(32,0), V(0,16)+r1, 4
         vand     V(32,1), V(0,16)+r1, 15
         vlsr     V(32,2), V(0,0)+r1, 4

         vlsr     V(32,3), V(0,17)+r1, 4
         vand     V(32,4), V(0,17)+r1, 15
         vlsr     V(32,5), V(0,1)+r1, 4

         vmul     -,            V(32,0), R_WEIGHT*17  CLRA UACC
         vmul     -,            V(32,1), G_WEIGHT*17       UACC
         vmul     V16(48,0)+r2, V(32,2), B_WEIGHT*17       UACC

         vmul     -,            V(32,3), R_WEIGHT*17  CLRA UACC
         vmul     -,            V(32,4), G_WEIGHT*17       UACC
         vmul     V16(48,1)+r2, V(32,5), B_WEIGHT*17       UACC

         cmp      r2, 6
         add.eq   r2, 8

         vadd     V16(16,0)+r3, V(32,0), V(32,3)
         vadd     V16(16,1)+r3, V(32,1), V(32,4)
         vadd     V16(16,2)+r3, V(32,2), V(32,5)

         add      r1, 2
         add      r3, 4
         addcmpbne r2, 2, 24, .loop

         vmul     V16(16,0++), V16(16,0++), 0x11      REP 32

         b        etc1_loaded
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   etc1_load_rgb565
;;;
;;; SYNOPSIS
;;;   extern unsigned long etc1_load_rgb565(
;;;      void                *output,        /* r0 */
;;;      unsigned short const*pixels,        /* r1 */
;;;      int                  pitch,         /* r2 */
;;;      int                  rows,          /* r3 */
;;;      int                  columns);      /* r4 */
;;;
;;; FUNCTION
;;;   blah
;;;
;;; RETURNS
;;;   blah

.function etc1_load_rgb565 ; {{{
         rsub     r3, 16
         mov      r5, r0
         rsub     r0, r3, 16
         shl      r3, 6
         vld      H16(0++,0)+r3, (r1+=r2)             REP r0
         mov      r0, r5
         mov      r5, -64
         addcmpbgt r3, 0, 0, .fvloop
         b        .fvskip
.fvloop: vmov     H16(63,0)+r3, H16(0,0)+r3
         addcmpbgt r3, r5, 0, .fvloop
.fvskip: addcmpblo r4, 0, 16, .fhloop
         b        .fhskip
.fhloop: vmov     V16(0,0)+r4, V16(0,47)+r4
         addcmpblo r4, 1, 16, .fhloop
.fhskip:
         mov      r1, 0
         mov      r2, 0
         mov      r3, 0
.loop:   
         vand     V(32,0),      V(0,16)+r1, 0xF8
         vand     V16(32,1),  V16(0,0)+r1, 0x07E0
         vshl     V(32,2),      V(0,0)+r1, 3

         vand     V(32,3),      V(0,17)+r1, 0xF8
         vand     V16(32,4),  V16(0,1)+r1, 0x07E0
         vshl     V(32,5),      V(0,1)+r1, 3

         vmulm    V(32,0),   V(32,0), 0x108
         vmulhd.uu V(32,1),V16(32,1), 0x2080
         vmulm    V(32,2),   V(32,2), 0x108

         vmulm    V(32,3),   V(32,3), 0x108
         vmulhd.uu V(32,4),V16(32,4), 0x2080
         vmulm    V(32,5),   V(32,5), 0x108

         vmul     -,            V(32,0), R_WEIGHT     CLRA UACC
         vmul     -,            V(32,1), G_WEIGHT          UACC
         vmul     V16(48,0)+r2, V(32,2), B_WEIGHT          UACC

         vmul     -,            V(32,3), R_WEIGHT     CLRA UACC
         vmul     -,            V(32,4), G_WEIGHT          UACC
         vmul     V16(48,1)+r2, V(32,5), B_WEIGHT          UACC

         cmp      r2, 6
         add.eq   r2, 8

         vadd     V16(16,0)+r3, V(32,0), V(32,3)
         vadd     V16(16,1)+r3, V(32,1), V(32,4)
         vadd     V16(16,2)+r3, V(32,2), V(32,5)

         add      r1, 2
         add      r3, 4
         addcmpbne r2, 2, 24, .loop

         b        etc1_loaded
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   etc1_load_rgb888
;;;
;;; SYNOPSIS
;;;   extern unsigned long etc1_load_rgb888(
;;;      void                *output,        /* r0 */
;;;      unsigned char const *pixels,        /* r1 */
;;;      int                  pitch,         /* r2 */
;;;      int                  rows,          /* r3 */
;;;      int                  columns);      /* r4 */
;;;
;;; FUNCTION
;;;   blah
;;;
;;; RETURNS
;;;   blah


.function etc1_load_rgb888 ; {{{
         rsub     r3, 16
         mov      r5, r0
         rsub     r0, r3, 16
         shl      r3, 6
         vld      H(0++,0)+r3, (r1+=r2)               REP r0
         vld      H(0++,16)+r3, (r1+16+=r2)           REP r0
         vld      H(0++,32)+r3, (r1+32+=r2)           REP r0
         mov      r0, r5
         mov      r5, -64
         mul      r4, 3
         addcmpbgt r3, 0, 0, .fvloop
         b        .fvskip
.fvloop: vmov     H32(63,0)+r3, H32(0,0)+r3
         addcmpbgt r3, r5, 0, .fvloop
.fvskip: addcmpblo r4, 0, 48, .fhloop
         b        .fhskip
.fhloop: vmov     V(0,0)+r4, V(0,61)+r4
         vmov     V(0,1)+r4, V(0,62)+r4 
         vmov     V(0,2)+r4, V(0,63)+r4
         addcmpblo r4, 3, 48, .fhloop
.fhskip:
         mov      r1, 0
         mov      r2, 0
         mov      r3, 0
.loop:   vmul     -,            V(0,0)+r1, R_WEIGHT   CLRA UACC
         vmul     -,            V(0,1)+r1, G_WEIGHT        UACC
         vmul     V16(48,0)+r2, V(0,2)+r1, B_WEIGHT        UACC

         vmul     -,            V(0,3)+r1, R_WEIGHT   CLRA UACC
         vmul     -,            V(0,4)+r1, G_WEIGHT        UACC
         vmul     V16(48,1)+r2, V(0,5)+r1, B_WEIGHT        UACC

         cmp      r2, 6
         add.eq   r2, 8

         vadd     V16(16,0)+r3, V(0,0)+r1, V(0,3)+r1
         vadd     V16(16,1)+r3, V(0,1)+r1, V(0,4)+r1
         vadd     V16(16,2)+r3, V(0,2)+r1, V(0,5)+r1

         add      r1, 6
         add      r3, 4
         addcmpbne r2, 2, 24, .loop

         b        etc1_loaded
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   etc1_load_rgbx32
;;;
;;; SYNOPSIS
;;;   extern unsigned long etc1_load_rgbx32(
;;;      void                *output,        /* r0 */
;;;      unsigned long const *pixels,        /* r1 */
;;;      int                  pitch,         /* r2 */
;;;      int                  rows,          /* r3 */
;;;      int                  columns);      /* r4 */
;;;
;;; FUNCTION
;;;   blah
;;;
;;; RETURNS
;;;   blah


.function etc1_load_rgbx32 ; {{{
         rsub     r3, 16
         mov      r5, r0
         rsub     r0, r3, 16
         shl      r3, 6
         vld      H(0++,0)+r3, (r1+=r2)               REP r0
         vld      H(0++,16)+r3, (r1+16+=r2)           REP r0
         vld      H(0++,32)+r3, (r1+32+=r2)           REP r0
         vld      H(0++,48)+r3, (r1+48+=r2)           REP r0
         mov      r0, r5
         mov      r5, -64
         shl      r4, 2
         addcmpbgt r3, 0, 0, .fvloop
         b        .fvskip
.fvloop: vmov     H32(63,0)+r3, H32(0,0)+r3
         addcmpbgt r3, r5, 0, .fvloop
.fvskip: mov      r5, 64
         addcmpblo r4, 0, r5, .fhloop
         b        .fhskip
.fhloop: vmov     V(0,0)+r4, V(0,60)+r4
         vmov     V(0,1)+r4, V(0,61)+r4
         vmov     V(0,2)+r4, V(0,62)+r4
         vmov     V(0,3)+r4, V(0,63)+r4
         addcmpblo r4, 4, r5, .fhloop
.fhskip:
         mov      r1, 0
         mov      r2, 0
         mov      r3, 0
.loop:   vmul     -,            V(0,0)+r1, R_WEIGHT   CLRA UACC
         vmul     -,            V(0,1)+r1, G_WEIGHT        UACC
         vmul     V16(48,0)+r2, V(0,2)+r1, B_WEIGHT        UACC

         vmul     -,            V(0,4)+r1, R_WEIGHT   CLRA UACC
         vmul     -,            V(0,5)+r1, G_WEIGHT        UACC
         vmul     V16(48,1)+r2, V(0,6)+r1, B_WEIGHT        UACC

         cmp      r2, 6
         add.eq   r2, 8

         vadd     V16(16,0)+r3, V(0,0)+r1, V(0,4)+r1
         vadd     V16(16,1)+r3, V(0,1)+r1, V(0,5)+r1
         vadd     V16(16,2)+r3, V(0,2)+r1, V(0,6)+r1

         add      r1, 8
         add      r3, 4
         addcmpbne r2, 2, 24, .loop

etc1_loaded:
.global etc1_loaded
         vadd     H32(16,0), H32(16,0), H32(17,0)
         vadd     H32(17,0), H32(18,0), H32(19,0)
         vadd     H32(18,0), H32(20,0), H32(21,0)
         vadd     H32(19,0), H32(22,0), H32(23,0)
         vadd     H32(20,0), H32(24,0), H32(25,0)
         vadd     H32(21,0), H32(26,0), H32(27,0)
         vadd     H32(22,0), H32(28,0), H32(29,0)
         vadd     H32(23,0), H32(30,0), H32(31,0)

etc1_decimated:
.globl etc1_decimated
         vbitplanes -, 0xF0F0                         SETF

         vadd     H32(27,0), H32(16,0),  H32(17,0)
         vadd     H32(26,0), H32(18,0),  H32(19,0)
         vadd     H32(25,0), H32(20,0),  H32(21,0)
         vadd     H32(24,0), H32(22,0),  H32(23,0)

         vadd     H32(31,0), H32(16,4),  H32(16,0)    IFZ
         vadd     H32(31,0), H32(17,12), H32(17,0)    IFNZ
         vadd     H32(30,0), H32(18,4),  H32(18,0)    IFZ
         vadd     H32(30,0), H32(19,12), H32(19,0)    IFNZ
         vadd     H32(29,0), H32(20,4),  H32(20,0)    IFZ
         vadd     H32(29,0), H32(21,12), H32(21,0)    IFNZ
         vadd     H32(28,0), H32(22,4),  H32(22,0)    IFZ
         vadd     H32(28,0), H32(23,12), H32(23,0)    IFNZ

         vmulhn.uu H(16++,0), H16(24++,0), 996        REP 8
         vmulhn.uu H(16++,16), H16(24++,0), 482       REP 8
         vmulhn.uu H(16++,32), H16(24++,32), 996      REP 8
         vmulhn.uu H(16++,48), H16(24++,32), 482      REP 8

         ; TODO: perform the diff-bit test here...

         vmulm    H(16++,0),  H(16++,0),  0x840       REP 4
         vmulm    H(24++,0),  H(20++,0),  0x840       REP 4
         vmul     H(16++,16), H(16++,16), 0x11        REP 4
         vmul     H(24++,16), H(20++,16), 0x11        REP 4
         vmulm    H(20++,0),  H(16++,32), 0x840       REP 4
         vmulm    H(28++,0),  H(20++,32), 0x840       REP 4
         vmul     H(20++,16), H(16++,48), 0x11        REP 4
         vmul     H(28++,16), H(20++,48), 0x11        REP 4

         mov      r1, 32
         vst      H16(16++,0), (r0+=r1)               REP 16

         ; TODO: calculate the quantised luma here...

         ; re-order the luma data now to get some lazy store benefit
         vinterl  V32(32,0), V32(48,0), V32(48,2)
         vinterl  V32(32,1), V32(48,1), V32(48,3)
         vinterl  V32(32,2), V32(48,4), V32(48,6)
         vinterl  V32(32,3), V32(48,5), V32(48,7)
         vinterh  V32(32,4), V32(48,0), V32(48,2)
         vinterh  V32(32,5), V32(48,1), V32(48,3)
         vinterh  V32(32,6), V32(48,4), V32(48,6)
         vinterh  V32(32,7), V32(48,5), V32(48,7)

         vinterl  H16(54,0), V16(32,0), V16(32,1)
         vinterl  H16(55,0), V16(32,2), V16(32,3)
         vinterh  H16(52,0), V16(32,0), V16(32,1)
         vinterh  H16(53,0), V16(32,2), V16(32,3)
         vinterl  H16(50,0), V16(32,4), V16(32,5)
         vinterl  H16(51,0), V16(32,6), V16(32,7)
         vinterh  H16(48,0), V16(32,4), V16(32,5)
         vinterh  H16(49,0), V16(32,6), V16(32,7)

         vinterl  H16(62,0), V16(32,32), V16(32,33)
         vinterl  H16(63,0), V16(32,34), V16(32,35)
         vinterh  H16(60,0), V16(32,32), V16(32,33)
         vinterh  H16(61,0), V16(32,34), V16(32,35)
         vinterl  H16(58,0), V16(32,36), V16(32,37)
         vinterl  H16(59,0), V16(32,38), V16(32,39)
         vinterh  H16(56,0), V16(32,36), V16(32,37)
         vinterh  H16(57,0), V16(32,38), V16(32,39)

         add      r3, pc, pcrel(.shifties)
         vld      H(44,0), (r3)
         b        lr

.align 16
.ifdef __BCM2707A0__
.shifties: .byte 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15
.else
.shifties: .byte 8, 12, 0, 4, 9, 13, 1, 5, 10, 14, 2, 6, 11, 15, 3, 7
.endif
         
.endfn ; }}}
