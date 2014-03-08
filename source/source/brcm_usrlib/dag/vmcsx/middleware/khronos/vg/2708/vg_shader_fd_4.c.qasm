//#define VG_SHADER_FD_DUMP_SHADERS
/* using just "dqasm", stdin might not be redirected correctly: http://support.microsoft.com/kb/321788 */
//#define VG_SHADER_FD_DQASM "python /cygdrive/c/path/dqasm.py -c -tb0"

/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

%[ .eval pragma_allow_xor_0(True) %]

#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/vg/2708/vg_shader_4.h"
#include "middleware/khronos/vg/vg_ramp.h" /* for VG_RAMP_WIDTH */
#include "middleware/khronos/common/khrn_fleaky_map.h"
#include "interface/khronos/common/khrn_int_util.h"
#ifdef VG_SHADER_FD_DUMP_SHADERS
   #include <stdio.h>
#endif

/******************************************************************************
generation helpers
******************************************************************************/

#ifdef __HIGHC__
   /*
      use XXX.../XXX syntax for variadic macros
   */

   #define APPEND(P, INSTRS...) \
      do { \
         static const uint32_t instrs_[] = { INSTRS }; \
         uint32_t i_; \
         for (i_ = 0; i_ != ARR_COUNT(instrs_); ++i_) { \
            *((P)++) = instrs_[i_]; \
         } \
      } while (0)

   #define APPEND_I(P, I, INSTRS_N, INSTRS...) \
      do { \
         static const uint32_t instrs_[][(INSTRS_N) * 2] = { INSTRS }; \
         uint32_t i_; \
         for (i_ = 0; i_ != (INSTRS_N) * 2; ++i_) { \
            *((P)++) = instrs_[I][i_]; \
         } \
      } while (0)
#else
   /*
      use c99 .../__VA_ARGS__ syntax for variadic macros
   */

   #define APPEND(P, ...) \
      do { \
         static const uint32_t instrs_[] = { __VA_ARGS__ }; \
         uint32_t i_; \
         for (i_ = 0; i_ != ARR_COUNT(instrs_); ++i_) { \
            *((P)++) = instrs_[i_]; \
         } \
      } while (0)

   #define APPEND_I(P, I, INSTRS_N, ...) \
      do { \
         static const uint32_t instrs_[][(INSTRS_N) * 2] = { __VA_ARGS__ }; \
         uint32_t i_; \
         for (i_ = 0; i_ != (INSTRS_N) * 2; ++i_) { \
            *((P)++) = instrs_[I][i_]; \
         } \
      } while (0)
#endif

static INLINE void xor(uint32_t *p, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
   p[0] ^= a ^ c;
   p[1] ^= b ^ d;
}

static INLINE void swap(uint32_t *p, uint32_t *q)
{
   p[0] ^= q[0]; q[0] ^= p[0]; p[0] ^= q[0];
   p[1] ^= q[1]; q[1] ^= p[1]; p[1] ^= q[1];
}

/******************************************************************************
generation
******************************************************************************/

%[ .set r_pmc, rb15 %]

/* for keeping track of where the color is currently held */
typedef enum {
   WHERE_TU,
   WHERE_RA0,
   WHERE_RA0_OLD
} WHERE_T;

static bool append_multiply(uint32_t **p, bool *need_pmc, VG_SHADER_FD_KEY_T key, WHERE_T where, bool pre)
{
   if (key.paint_type == VG_SHADER_FD_PAINT_TYPE_SOLID) {
      APPEND(*p,                   %[ nop ; nop                   ; ldtmu0 %]);
      APPEND_I(*p, key.image_rgbl, %[ nop ; v8muld  ra0, unif, %0          %| r4.8dr %, r4 %]); /* might add signal here */
      return key.image_pre;
   }
   if (pre || !key.image_pre) {
      switch (where) {
      case WHERE_TU:
      {
         bool paint_rgbl = (key.paint_type != VG_SHADER_FD_PAINT_TYPE_PATTERN) || key.paint_pattern_rgbl;
         APPEND(*p,               %[ nop ; nop          ; ldtmu0 %]);
         APPEND_I(*p, paint_rgbl, %[ nop ; mov  ra0, %0 ; ldtmu0 %| r4.8dr %, r4 %]); /* might change mul write from ra0 */
         break;
      }
      case WHERE_RA0:
      {
         xor(*p - 2, %[ ldtmu0 %], %[ nop %]);
         break;
      }
      case WHERE_RA0_OLD:
      {
         APPEND(*p, %[ ldtmu0 %]);
         break;
      }
      default:
         UNREACHABLE();
      }
      if (pre && !key.image_pre) {
         *need_pmc = true;
         APPEND(*p,                   %[ or  r0, r_pmc, r4.8dr ; nop
                                         nop                   ; v8muld  r0, r0, r4
                                         nop                   ; v8muld  ra0, ra0, r0 %]); /* might add signal here */
      } else if (where == WHERE_RA0_OLD) {
         APPEND_I(*p, key.image_rgbl, %[ nop                   ; v8muld  ra0, ra0, %0 %| r4.8dr %, r4 %]); /* might add signal here */
      } else {
                                      xor(*p - 2, %[ nop ; mov  ra32, 0 %], %[ nop ; mov  ra0, 0 %]); /* change mul write from ra0 --> r0 */
         APPEND_I(*p, key.image_rgbl, %[ nop                   ; v8muld  ra0, r0, %0  %| r4.8dr %, r4 %]); /* might add signal here */
      }
      return pre;
   }
   vcos_assert((where == WHERE_TU) || (where == WHERE_RA0_OLD)); /* ra0 case not needed */
   *need_pmc = true;
   if (where == WHERE_TU) { APPEND(*p, %[ nop                   ; nop                 ; ldtmu0 %]); }
   APPEND_I(*p, where == WHERE_TU,     %[ or  r0, r_pmc, %0.8dr ; nop
                                          nop                   ; v8muld  r0, r0, %0  ; ldtmu0 %| ra0 %, r4 %]);
   APPEND_I(*p, key.image_rgbl,        %[ nop                   ; v8muld  ra0, r0, %0          %| r4.8dr %, r4 %]); /* might add signal here */
   return true;
}
#define MULTIPLY_INSTRS_N_MAX 5

static uint32_t *vg_shader_fd_gen(uint32_t *p, VG_SHADER_FD_KEY_T key)
{
   bool paint_rgbl = (key.paint_type != VG_SHADER_FD_PAINT_TYPE_PATTERN) || key.paint_pattern_rgbl;
   uint32_t i, extra_thrsws;
   bool need_pmc, have_pmc;
   bool so_solid = (key.draw_type == VG_SHADER_FD_DRAW_TYPE_PAINT) && (key.paint_type == VG_SHADER_FD_PAINT_TYPE_SOLID);
   uint32_t *scissor_branch = NULL;
   WHERE_T where = WHERE_TU;
   bool pre = false;
   bool stencil = key.draw_type == VG_SHADER_FD_DRAW_TYPE_STENCIL;
   bool need_stencil_a = vg_shader_fd_need_stencil_a((VG_SHADER_FD_BLEND_T)key.blend);
   enum {
      SRC_R0,
      SRC_RA0
   } src = SRC_R0;
   enum {
      STENCIL_A_R1,
      STENCIL_A_RA0_8DR
   } stencil_a = STENCIL_A_R1;
   bool pre_before_blend = !key.pre && !key.no_alpha;
   bool unpre_after_blend = !key.pre && (!key.no_alpha || (key.blend == VG_SHADER_FD_BLEND_SRC) || (key.blend == VG_SHADER_FD_BLEND_DST_IN));
   bool darken = key.blend == VG_SHADER_FD_BLEND_DARKEN;
   bool tlbc_in_mul = false;

   vcos_assert((key.draw_type == VG_SHADER_FD_DRAW_TYPE_IMAGE) || paint_rgbl || key.paint_pattern_pre);
   vcos_assert((key.draw_type == VG_SHADER_FD_DRAW_TYPE_PAINT) || key.image_rgbl || key.image_pre);

   /*
      overall structure of shader is:

      extra thrsws (todo: on b0, these won't be necessary)
      pmc
      scissor fetch (possible early exit)                           [thrsw]
      (non-solid) paint fetch                                       [thrsw]
      image fetch                                                   [thrsw]
      draw-type multiply
      color transform
      colorspace conversion                                         [thrsw]
      draw-type stencil (also affects blend)
      blend prepare (loading/preing colors and sbwait)
      blend
      scissor and non-pre fb handling
      thrend
      scissor early exit (possibly branch here after scissor fetch)
      thrend
   */

   /*
      same number of thrsws in every shader: put extra thrsws here to make up
      for having fewer later
      todo: remove this stuff for b0
   */

   extra_thrsws = vg_shader_fd_get_extra_thrsws(key);
   for (i = 0; i != extra_thrsws; ++i) {
      APPEND(p, %[ nop ; nop ; thrsw
                   nop ; nop
                   nop ; nop         %]);
   }
   #define EXTRA_THRSWS_INSTRS_N_MAX 1 /* just need an extra one for the fifo-overflow-avoiding thrsw */

   /*
      if we need the magic constant 0xff000000 anywhere in the shader, we load
      it here

      when we use the magic constant, we set need_pmc to true

      at the end of the function, we check that need_pmc == have_pmc, ie we
      loaded it iff we needed it
   */

   need_pmc = false;
   have_pmc = pre_before_blend || unpre_after_blend ||
      ((key.blend == VG_SHADER_FD_BLEND_DST_IN) && !key.pre && key.no_alpha);
   if (!have_pmc) {
      bool pre = (key.draw_type == VG_SHADER_FD_DRAW_TYPE_IMAGE) ? key.image_pre :
         (((key.paint_type == VG_SHADER_FD_PAINT_TYPE_SOLID) && ((key.draw_type == VG_SHADER_FD_DRAW_TYPE_PAINT) || key.image_pre)) ||
         ((key.paint_type == VG_SHADER_FD_PAINT_TYPE_PATTERN) && key.paint_pattern_pre));
      have_pmc = !pre && !key.color_transform && !key.colorspace_convert;
      if (key.draw_type == VG_SHADER_FD_DRAW_TYPE_MULTIPLY) { have_pmc = have_pmc || (pre != key.image_pre); }
      if (stencil) { have_pmc = have_pmc || !key.image_pre; }
   }
   if (have_pmc) {
      APPEND(p, %[ mov  r_pmc, 0xff000000 %]);
   }
   #define PMC_INSTRS_N_MAX 1

   /*
      load scissor and end shader early if possible
   */

   if (key.scissor) {
      %[ .set r_scissor, ra15 %]
      APPEND(p,                  %[ itof  r0, x_coord                ; mov  ra0, f(0.5)
                                    itof  r1, y_coord                ; mov  r2, unif
                                    fadd  r1, r1, ra0                ; mov  r3, unif
                                    fadd  r0, r0, ra0                ; fmul  t0t, r1, r3 ; thrsw
                                    nop                              ; fmul  t0s, r0, r2
                                    nop                              ; nop                        %]); /* workaround for hw-2384. todo: remove on b0 */
      if (!so_solid) { APPEND(p, %[ nop                              ; nop               ; ldtmu0
                                    and.setf  r_scissor, ms_mask, r4 ; nop
                                    brr.allz  -, 0                                                %]);
         scissor_branch = p - 2;
      } /* else: do ldtmu0 and setf just before blend */
   }
   #define SCISSOR_FETCH_INSTRS_N_MAX 9

   /*
      we might have just branched, so we shouldn't put a thrsw/thrend/etc in the
      next 3 instructions

      if scissoring, pixel valid flags are already set
   */

   /*
      (non-solid) paint fetch and image fetch
      (solid paint color passed in as uniform when required)
   */

   if (key.draw_type != VG_SHADER_FD_DRAW_TYPE_IMAGE) {
      switch (key.paint_type) {
      case VG_SHADER_FD_PAINT_TYPE_SOLID:
      {
         /* we read in the color as a uniform when it's needed */
         break;
      }
      case VG_SHADER_FD_PAINT_TYPE_GRAD_LINEAR:
      {
         /*
            dx = x1 - x0, dy = y1 - y0
            g(x, y) = ((dx * (x - x0)) + (dy * (y - y0))) / (dx^2 + dy^2)

            rotating/translating/scaling the surface-to-paint matrix so
            (x0, y0) = (0, 0) and (x1, y1) = (1, 0):

            dx = 1, dy = 0
            g(x, y) = x
         */

         APPEND(p,                %[ itof  r0, x_coord   ; mov  r2, unif
                                     itof  r1, y_coord   ; fmul  r0, unif, r0
                                     fadd  r0, r2, r0    ; fmul  r1, unif, r1
                                     # potential scissor branch happens here
                                     mov  r2, unif       ; nop                %]);
         APPEND_I(p, key.scissor, %[ mov%0  t0t, r2      ; nop                     # might add thrsw here
                                     fadd%0  t0s, r0, r1 ; nop                %| %, .ifnz %]);
         APPEND(p,                %[ nop                 ; nop                %]); /* workaround for hw-2384. todo: remove on b0 */
         #define PAINT_GRAD_LINEAR_FETCH_INSTRS_N_MAX 7
         break;
      }
      case VG_SHADER_FD_PAINT_TYPE_GRAD_RADIAL:
      {
         /*
            fx' = fx - cx, fy' = fy - cy, dx = x - fx, dy = y - fy
            g(x, y) = ((dx * fx') + (dy * fy') + sqrt((r^2 * (dx^2 + dy^2)) - ((dx * fy') - (dy * fx'))^2)) / (r^2 - (fx'^2 + fy'^2))

            rotating/translating/scaling the surface-to-paint matrix so
            (fx, fy) = (0, 0), cy = 0, and r = 1:

            fx' = -cx, fy' = 0, dx = x, dy = y
            g(x, y) = (sqrt((x^2 + y^2) - (y * cx)^2) - (x * cx)) / (1 - cx^2)
            u = 1 / (1 - cx^2)
            v = -u * cx
            g(x, y) = (v * x) + sqrt((u^2 * x^2) + (u * y^2))
         */

         vcos_static_assert(VG_RAMP_WIDTH == 256); /* offset of 1/512 is hardcoded into qpu asm below (see ramp.c) */
         APPEND(p,                %[ itof  r0, x_coord       ; mov  r3, unif
                                     itof  r1, y_coord       ; fmul  r2, unif, r0
                                     fadd  r2, r3, r2        ; fmul  r3, unif, r1
                                     # potential scissor branch happens here
                                     fadd  r2, r2, r3        ; fmul  r0, unif, r0   # r2: x
                                     fadd  r0, unif, r0      ; fmul  r3, r2, r2     # r3: x^2
                                     nop                     ; fmul  r1, unif, r1
                                     fadd  r0, r0, r1        ; fmul  r1, unif, r3   # r0: y, r1: u^2 * x^2
                                     nop                     ; fmul  r0, r0, r0     # r0: y^2
                                     mov  r3, f(1.0 / 256.0) ; fmul  r0, unif, r0   # r3: 1/256, r0: u * y^2
                                     fadd  rsqrt, r0, r1     ; fmul  r3, r3, f(0.5) # r4: rsqrt((u * y^2) + (u^2 * x^2)), r3: 1/512
                                     fadd  r0, r0, r1        ; fmul  r1, unif, r2   # r0: (u * y^2) + (u^2 * x^2), r1: v * x
                                     fadd  r1, r1, r3        ; mov  r2, unif        # r1: (v * x) + 1/512                           %]);
         APPEND_I(p, key.scissor, %[ mov%0  t0t, r2          ; fmul  r0, r0, r4                                                          # might add thrsw here
                                     fadd%0  t0s, r0, r1     ; nop                                                                  %| %, .ifnz %]);
         APPEND(p,                %[ nop                     ; nop                                                                  %]); /* workaround for hw-2384. todo: remove on b0 */
         #define PAINT_GRAD_RADIAL_FETCH_INSTRS_N_MAX 15
         break;
      }
      case VG_SHADER_FD_PAINT_TYPE_PATTERN:
      {
         APPEND(p,                                                               %[ itof  r0, x_coord   ; mov  r3, unif
                                                                                    itof  r1, y_coord   ; fmul  r2, unif, r0
                                                                                    fadd  r2, r3, r2    ; fmul  r3, unif, r1
                                                                                    # potential scissor branch happens here
                                                                                    fadd  r2, r2, r3    ; fmul  r0, unif, r0
                                                                                    nop                 ; fmul  r1, unif, r1
                                                                                    fadd  r0, unif, r0  ; nop                %]);
         if (key.paint_pattern_tile_fill) { APPEND(p,                            %[ mov  r3, unif       ; nop                %]); }
         if (key.paint_pattern_child) { APPEND(p,                                %[ mov  t0b, 0         ; nop                %]); }
         if (key.paint_pattern_tile_fill || key.paint_pattern_child) { APPEND(p, %[ mov  t0r, r3        ; nop                %]); }
         APPEND_I(p, key.scissor,                                                %[ fadd%0  t0t, r0, r1 ; nop                     # might add thrsw here
                                                                                    mov%0  t0s, r2      ; nop                %| %, .ifnz %]);
         APPEND(p,                                                               %[ nop                 ; nop                %]); /* workaround for hw-2384. todo: remove on b0 */
         pre = key.paint_pattern_pre;
         #define PAINT_PATTERN_FETCH_INSTRS_N_MAX 12
         break;
      }
      default:
      {
         UNREACHABLE();
      }
      }
   } else {
      pre = key.image_pre;
   }
   #define PAINT_FETCH_INSTRS_N_MAX _max( \
      PAINT_GRAD_LINEAR_FETCH_INSTRS_N_MAX, _max( \
      PAINT_GRAD_RADIAL_FETCH_INSTRS_N_MAX, \
      PAINT_PATTERN_FETCH_INSTRS_N_MAX))

   if (key.draw_type != VG_SHADER_FD_DRAW_TYPE_PAINT) {
      if (/* have both paint and image */
         (key.draw_type != VG_SHADER_FD_DRAW_TYPE_IMAGE) &&
         /* and paint tu lookup needs more than 2 input fifo entries */
         (((key.paint_type == VG_SHADER_FD_PAINT_TYPE_PATTERN) &&
         (key.paint_pattern_tile_fill || key.paint_pattern_child)) ||
         /* or paint needs tu lookup and image tu lookup needs more than 2 input
          * fifo entries */
         ((key.paint_type != VG_SHADER_FD_PAINT_TYPE_SOLID) &&
         key.image_child))) {
         /*
            there's not going to be enough room in the tu fifo for the image
            lookup
         */

         vcos_assert(!key.image_projective); /* no need to handle this in projective case below */
         xor(p - 6, %[ thrsw %], %[ nop %]);
         where = WHERE_RA0_OLD;
      }
      APPEND(p,                                  %[ itof  r0, x_coord      ; mov  r3, unif
                                                    itof  r1, y_coord      ; fmul  r2, unif, r0
                                                    fadd  r2, r3, r2       ; fmul  r3, unif, r1           %]);
                                                    /* potential scissor branch happens here */
      if (key.image_projective) {
         APPEND(p,                               %[ fadd  recip, r2, r3    ; mov  r3, unif
                                                    nop                    ; fmul  r2, unif, r0
                                                    fadd  r2, r3, r2       ; fmul  r3, unif, r1
                                                    fadd  r2, r2, r3       ; fmul  r0, unif, r0
                                                    nop                    ; fmul  r1, unif, r1
                                                    fadd  r0, unif, r0     ; nop                          %]);
         if (key.image_child) { APPEND(p,        %[ mov  t0b, 0            ; nop
                                                    mov  t0r, 0            ; nop                          %]); }
         APPEND_I(p, key.scissor,                %[ fadd  r0, r0, r1       ; fmul%0  t0t, r2, r4 ; thrsw
                                                    nop                    ; fmul%0  t0s, r0, r4          %| %, .ifnz %]);
      } else {
         APPEND(p,                               %[ nop                    ; fmul  r0, unif, r0           %]);
         if (where == WHERE_RA0_OLD) { APPEND(p, %[ mov.setf  -, r_scissor ; fmul  r1, unif, r1  ; ldtmu0 %]); /* we did a thrsw after the paint fetch, we need to reinstate the flags */
            APPEND_I(p, paint_rgbl,              %[ fadd  r0, unif, r0     ; mov  ra0, %0                 %| r4.8dr %, r4 %]);
         } else { APPEND(p,                      %[ nop                    ; fmul  r1, unif, r1
                                                    fadd  r0, unif, r0     ; nop                          %]); }
         if (key.image_child) { APPEND(p,        %[ mov  t0b, 0            ; nop
                                                    mov  t0r, 0            ; nop                          %]); }
         APPEND_I(p, key.scissor,                %[ fadd%0  t0t, r2, r3    ; nop                 ; thrsw
                                                    fadd%0  t0s, r0, r1    ; nop                          %| %, .ifnz %]);
      }
      APPEND(p,                                  %[ nop                    ; nop                          %]); /* workaround for hw-2384. todo: remove on b0 */
   } else if (key.paint_type != VG_SHADER_FD_PAINT_TYPE_SOLID) {
      xor(p - 6, %[ thrsw %], %[ nop %]); /* put thrsw in paint fetch */
   }
   #define IMAGE_FETCH_INSTRS_N_MAX 14

   if (key.draw_type == VG_SHADER_FD_DRAW_TYPE_MULTIPLY) {
      pre = append_multiply(&p, &need_pmc, key, where, pre);
      where = WHERE_RA0;
   }
   /* MULTIPLY_INSTRS_N_MAX defined after append_multiply */

   if (key.color_transform) {
      /*
         input is either in tu fifo or in ra0
         pre indicates whether or not it is premultiplied

         we always output premultiplied. we could cut some instructions in the
         case where we have a colorspace conversion by keeping the color
         non-pre, but we don't really care about the colorspace conversion case
         (hopefully all applications will use a non-linear colorspace for
         everything)
      */

      if (pre) {
         switch (where) {
         case WHERE_TU: {
            bool rgbl = (key.draw_type == VG_SHADER_FD_DRAW_TYPE_IMAGE) ? key.image_rgbl : paint_rgbl;
            APPEND(p,                    %[ nop                          ; nop                      ; ldtmu0 %]);
            APPEND_I(p, rgbl,            %[ fadd  r2, f(128.0), f(128.0) ; mov  ra0, %0                      %| r4.8dr %, r4 %]);
            APPEND(p,                    %[ mov  recip, r4.8dc           ; fmul  r3, r4.8dc, unif            %]); break; }
         case WHERE_RA0:
         case WHERE_RA0_OLD: { APPEND(p, %[ mov  r2, f(256.0)
                                            mov  recip, ra0.8dc          ; fmul  r3, ra0.8dc, unif           %]); break; }
         default: UNREACHABLE(); }
         APPEND(p,                       %[ mov  r1, 0                   ; fmul  ra1, ra0.8ac, unif
                                            not  r0, r1                  ; fmul  ra2, ra0.8bc, unif
                                            fsub.setf  -, r2, r4         ; fmul  r2, ra0.8cc, unif
                                            fadd  r3, r3, unif           ; fmul.ifnn  r1, ra1, r4
                                            fadd  ra0, r1, unif          ; fmul.ifnn  r1, ra2, r4
                                            fadd  ra1, r1, unif          ; fmul.ifnn  r1, r2, r4
                                            fadd  r2, r1, unif           ; mov  r1.8abcdc, r3
                                            nop                          ; mov  r0.8ac, ra0
                                            nop                          ; mov  r0.8bc, ra1
                                            nop                          ; mov  r0.8cc, r2
                                            nop                          ; v8muld  ra0, r0, r1               %]); /* might add signal here. might change mul write from ra0 */
      } else {
         switch (where) {
         case WHERE_TU: { APPEND(p,      %[ nop                  ; nop                      ; ldtmu0
                                            mov.setf  -, r4.8dc  ; fmul  r3, r4.8dc, unif
                                            mov  r0, -1          ; fmul  ra1, r4.8ac, unif
                                            mov.ifz  ra1, f(0.0) ; fmul  r1, r4.8bc, unif
                                            mov.ifz  r1, f(0.0)  ; fmul  r2, r4.8cc, unif            %]); break; }
         case WHERE_RA0:
         case WHERE_RA0_OLD: { APPEND(p, %[ mov  r0, -1          ; nop
                                            mov.setf  -, ra0.8dc ; fmul  r3, ra0.8dc, unif
                                            nop                  ; fmul  ra1, ra0.8ac, unif
                                            mov.ifz  ra1, f(0.0) ; fmul  r1, ra0.8bc, unif
                                            mov.ifz  r1, f(0.0)  ; fmul  r2, ra0.8cc, unif           %]); break; }
         default: UNREACHABLE(); }
         APPEND(p,                       %[ fadd  r3, r3, unif   ; mov.ifz  r2, f(0.0)
                                            fadd  r3, ra1, unif  ; mov  ra0.8abcdc, r3
                                            fadd  r3, r1, unif   ; mov  r0.8ac, r3
                                            fadd  r3, r2, unif   ; mov  r0.8bc, r3
                                            nop                  ; mov  r0.8cc, r3
                                            nop                  ; v8muld  ra0, r0, ra0              %]); /* might add signal here. might change mul write from ra0 */
      }
      where = WHERE_RA0;
      pre = true;
   }
   #define COLOR_TRANSFORM_INSTRS_N_MAX 14

   if (key.colorspace_convert) {
      /*
         input is either in tu fifo or in ra0
         pre indicates whether or not it is premultiplied

         always output premultiplied

         todo: on b0, use tu general memory lookups
      */

      if (pre) {
         switch (where) {
         case WHERE_TU: {
            bool rgbl = (key.draw_type == VG_SHADER_FD_DRAW_TYPE_IMAGE) ? key.image_rgbl : paint_rgbl;
            APPEND(p,                                  %[ nop                          ; nop                        ; ldtmu0 %]);
            APPEND_I(p, rgbl,                          %[ mov  ra0, %0                 ; nop                                 %| r4.8dr %, r4 %]);
            APPEND(p,                                  %[ mov  recip, r4.8dc           ; nop                                 %]);
            break; }
         case WHERE_RA0:     APPEND(p,                 %[ nop                          ; nop                                 %]); /* fall through */
         case WHERE_RA0_OLD: APPEND(p,                 %[ fadd  recip, ra0.8dc, f(0.0) ; nop                                 %]); break;
         default: UNREACHABLE(); }
         APPEND(p,                                     %[ mov  t0t, f(0.0)             ; nop
                                                          fadd  r1, ra0.8ac, f(0.0)    ; mov  r2, f(0.0)                     %]);
         if (stencil) { APPEND(p,                      %[ mov  r0, r4                  ; nop                        ; ldtmu0 %]);
            APPEND_I(p, key.image_rgbl,                %[ mov  ra2, %0                 ; fmul  t0s, r1, r0                   %| r4.8dr %, r4 %]);
         } else { APPEND(p,                            %[ mov  r0, r4                  ; fmul  t0s, r1, r4                   %]); }
         APPEND(p,                                     %[ fadd  ra1, ra0.8dc, r2       ; mov  t0t, f(0.0)           ; thrsw
                                                          mov  rb0, r0                 ; fmul  t0s, ra0.8bc, r0
                                                          nop                          ; nop                                      # workaround for hw-2384. todo: remove on b0
                                                          nop                          ; fmul  r0, ra0.8cc, rb0     ; ldtmu0
                                                          nop                          ; fmul  ra0.8ac, r4.8ac, ra1 ; ldtmu0
                                                          mov  t0t, f(0.0)             ; fmul  ra0.8bc, r4.8ac, ra1 ; thrsw
                                                          mov  t0s, r0                 ; nop
                                                          nop                          ; nop                                 %]); /* workaround for hw-2384. todo: remove on b0 */
      } else { switch (where) {
         case WHERE_TU: { APPEND(p,                    %[ mov  r0, f(0.0)              ; nop                        ; ldtmu0
                                                          mov  t0t, f(0.0)             ; mov  ra0, r4                        %]); break; }
         case WHERE_RA0:
         case WHERE_RA0_OLD: { APPEND(p,               %[ mov  t0t, f(0.0)             ; mov  r0, f(0.0)                     %]); break; }
         default: UNREACHABLE(); }
         if (stencil) { APPEND_I(p, where == WHERE_TU, %[ fadd  r1, %0.8ac, r0         ; nop                        ; ldtmu0 %| ra0 %, r4 %]);
            APPEND_I(p, key.image_rgbl,                %[ mov  ra2, %0                 ; mov  t0s, r1                        %| r4.8dr %, r4 %]);
         } else { APPEND_I(p, where == WHERE_TU,       %[ fadd  t0s, %0.8ac, r0        ; nop                                 %| ra0 %, r4 %]); }
         APPEND(p,                                     %[ fadd  ra1, ra0.8dc, r0       ; mov  t0t, f(0.0)           ; thrsw
                                                          fadd  t0s, ra0.8bc, r0       ; mov  rb0, r0
                                                          nop                          ; nop                                      # workaround for hw-2384. todo: remove on b0
                                                          fadd  r0, ra0.8cc, rb0       ; nop                        ; ldtmu0
                                                          nop                          ; fmul  ra0.8ac, r4.8ac, ra1 ; ldtmu0
                                                          mov  t0t, f(0.0)             ; fmul  ra0.8bc, r4.8ac, ra1 ; thrsw
                                                          mov  t0s, r0                 ; nop
                                                          nop                          ; nop                                 %]); /* workaround for hw-2384. todo: remove on b0 */
      }
      APPEND(p,                                        %[ nop                          ; nop                        ; ldtmu0
                                                          nop                          ; fmul  ra0.8cc, r4.8ac, ra1          %]); /* might add signal here */

      /*
         we have to explicitly handle the stencil case: we were forced to load
         the image early so we could use the tu

         we only have to handle a few cases though:
         - need / don't need stencil a
         - image pre/non-pre
      */

      if (stencil) {
         if (key.image_pre) { APPEND(p,  %[ mov  r0, ra2           ; nop                     %]);
         } else { need_pmc = true;
            APPEND(p,                    %[ or  r0, r_pmc, ra2.8dr ; nop
                                            nop                    ; v8muld  r0, r0, ra2     %]);
         }
         if (need_stencil_a) { APPEND(p, %[ mov  ra1, ra0          ; nop
                                            nop                    ; v8muld  ra0, ra0, r0         # might add signal here
                                            nop                    ; v8muld  r1, ra1.8dr, r0 %]); /* might add signal here */
            where = WHERE_RA0_OLD;
         } else { APPEND(p,              %[ nop                    ; v8muld  ra0, ra0, r0    %]); /* might add signal here */
            where = WHERE_RA0;
         }
      } else {
         where = WHERE_RA0;
      }
      pre = true;
   } else if (stencil) {
      if (need_stencil_a) {
         if (pre || (key.paint_type == VG_SHADER_FD_PAINT_TYPE_SOLID)) {
            if (key.paint_type == VG_SHADER_FD_PAINT_TYPE_SOLID) {
               APPEND(p,                                     %[ mov  r0, unif         ; mov  ra1, unif          ; ldtmu0 %]);
            } else { switch (where) {
               case WHERE_TU: { APPEND(p,                    %[ nop                   ; nop                     ; ldtmu0 %]);
                  APPEND_I(p, paint_rgbl,                    %[ mov  r0, %0           ; mov ra1, %0             ; ldtmu0 %| r4.8dr %, r4 %]);
                  break; }
               case WHERE_RA0: {                             xor(p - 2, %[ nop ; mov  ra32, 0 %], %[ nop ; mov  ra0, 0 %]); /* change mul write from ra0 --> r0 */
                  APPEND(p,                                  %[ mov  ra1, r0          ; nop                     ; ldtmu0 %]);
                  break; }
               case WHERE_RA0_OLD: { APPEND(p,               %[ mov  r0, ra0          ; mov ra1, ra0            ; ldtmu0 %]); break; }
               default: UNREACHABLE(); } }
            if (key.image_pre) { APPEND_I(p, key.image_rgbl, %[ mov  r0, %0           ; v8muld  ra0, r0, %0              %| r4.8dr %, r4 %]); /* might add signal here */
               APPEND(p,                                     %[ nop                   ; v8muld  r1, ra1.8dr, r0          %]); /* might add signal here */
            } else { /* r0 is loaded with the paint color, even though we don't really need it */
               need_pmc = true;
               APPEND(p,                                     %[ or  r0, r_pmc, r4.8dr ; nop
                                                                nop                   ; v8muld  r0, r0, r4
                                                                nop                   ; v8muld  ra0, ra1, r0                  # might add signal here
                                                                nop                   ; v8muld  r1, ra1.8dr, r0          %]); /* might add signal here */
            }
         } else {
            vcos_assert((where == WHERE_TU) || (where == WHERE_RA0_OLD)); /* ra0 case not needed */
            need_pmc = true;
            if (where == WHERE_TU) { APPEND(p, %[ nop                   ; nop                 ; ldtmu0 %]); }
            APPEND_I(p, where == WHERE_TU,     %[ or  r0, r_pmc, %0.8dr ; mov  r1, %0.8dr
                                                  nop                   ; v8muld  r0, r0, %0  ; ldtmu0 %| ra0 %, r4 %]);
            if (key.image_pre) {
               APPEND_I(p, key.image_rgbl,     %[ nop                   ; v8muld  ra0, r0, %0                               # might add signal here
                                                  nop                   ; v8muld  r1, r1, %0           %| r4.8dr %, r4 %]); /* might add signal here */
            } else {
               APPEND(p,                       %[ or  r2, r_pmc, r4.8dr ; nop
                                                  nop                   ; v8muld  r2, r2, r4
                                                  nop                   ; v8muld  ra0, r0, r2               # might add signal here
                                                  nop                   ; v8muld  r1, r1, r2           %]); /* might add signal here */
            }
         }
         where = WHERE_RA0_OLD;
         pre = true;
      } else {
         pre = append_multiply(&p, &need_pmc, key, where, pre);
         where = WHERE_RA0;
      }
   }
   #define COLORSPACE_CONVERT_AND_STENCIL_INSTRS_N_MAX _max(22, MULTIPLY_INSTRS_N_MAX)

   /*
      input is either unif, in tu fifo, or in ra0
      pre indicates whether or not it is premultiplied (except in the unif case,
      where the input is always premultiplied)

      in preparation for blending, we load/pre and note where src and stencil
      alpha can be found

      we also perform sbwait (todo: might not be necessary on b0? in which case
      we can probably save a cycle in some cases... eg by changing prev ra0
      writes to r0, not loading src if not necessary, etc)
      the last instruction output by this section never has a signal (and we
      always output at least one instruction), so we can insert a signal "for
      free" later (most likely either a loadc or a loadcv)
   */

   if (so_solid) {
      if (key.scissor) {
         APPEND(p, %[ ldtmu0 %]);
      } else if (!have_pmc) {
         /* can't have sbwait on the first instruction of a shader */
         /* todo: on b0 too? */
         APPEND(p, %[ nop %]);
      }
      if (need_stencil_a) {
         APPEND(p, %[ mov  r0, unif ; mov  ra0, unif ; sbwait %]);
         if (key.scissor) {
            if (!unpre_after_blend) {
               APPEND(p, %[ and.setf  -, ms_mask, r4 ; nop %]);
            } else {
               APPEND(p, %[ and  r_scissor, ms_mask, r4 ; nop %]);
            }
         } else {
            APPEND(p, %[ nop %]);
         }
      } else {
         if (key.scissor) {
            if (!unpre_after_blend) {
               APPEND(p, %[ and.setf  -, ms_mask, r4 ; nop ; sbwait %]);
            } else {
               APPEND(p, %[ and  r_scissor, ms_mask, r4 ; nop ; sbwait %]);
            }
         } else {
            APPEND(p, %[ sbwait %]);
         }
         APPEND(p, %[ mov  r0, unif ; nop %]);
      }
      stencil_a = STENCIL_A_RA0_8DR;
   } else {
      switch (where) {
      case WHERE_TU:
      {
         APPEND(p, %[ ldtmu0 %]);
         if (pre) {
            bool rgbl = (key.draw_type == VG_SHADER_FD_DRAW_TYPE_IMAGE) ? key.image_rgbl : paint_rgbl;
            APPEND_I(p, need_stencil_a, %[ %0 ; nop ; sbwait %| nop %, mov  r1, r4.8dr %]);
            APPEND_I(p, rgbl, %[ mov  r0, %0 ; nop %| r4.8dr %, r4 %]);
         } else {
            need_pmc = true;
            APPEND_I(p, need_stencil_a, %[ or  r0, r_pmc, r4.8dr ; %0 ; sbwait %| nop %, mov  r1, r4.8dr %]);
            APPEND(p, %[ nop ; v8muld  r0, r0, r4 %]);
         }
         break;
      }
      case WHERE_RA0:
      {
         if (pre) {
            xor(p - 2, %[ sbwait %], %[ nop %]);
            APPEND(p, %[ nop %]);
            src = SRC_RA0;
         } else {
            need_pmc = true;
            APPEND(p, %[ nop                    ; nop                 ; sbwait
                         or  r0, r_pmc, ra0.8dr ; nop
                         nop                    ; v8muld  r0, r0, ra0          %]);
         }
         stencil_a = STENCIL_A_RA0_8DR;
         break;
      }
      case WHERE_RA0_OLD:
      {
         vcos_assert(pre && need_stencil_a);
         xor(p - 4, %[ sbwait %], %[ nop %]);
         src = SRC_RA0;
         break;
      }
      default:
      {
         UNREACHABLE();
      }
      }
   }
   #define BLEND_SRC_PREP_INSTRS_N_MAX 3

   /*
      if the framebuffer isn't premultiplied, we need to
      premultiply/unpremultiply before/after blending

      this doesn't need to be fast, as we shouldn't be rendering to non-pre
      framebuffers

      todo: we don't need to do this if the blend doesn't actually need dst
      (eg src with !key.coverage)
   */

   if (pre_before_blend) {
      %[ .set r_dst, rb0 %]
      %[ .set r_a_dst, rb1 %]
      xor(p - 2, %[ loadc %], %[ nop %]);
      need_pmc = true;
      APPEND(p, %[ or  r2, r_pmc, r4.8dr ; mov  r_a_dst, r4.8dr
                   nop                   ; v8muld  r_dst, r2, r4
                   nop                   ; nop                   %]);
   }
   #define BLEND_DST_PREP_INSTRS_N_MAX 3

   /*
      blending...
      todo: could we cut some cycles in the key.masking cases?
      todo: we could probably cut some cycles in the !key.coverage cases...
   */

   switch ((key.no_alpha && ((key.blend == VG_SHADER_FD_BLEND_DARKEN) || (key.blend == VG_SHADER_FD_BLEND_LIGHTEN))) ?
      VG_SHADER_FD_BLEND_SRC_OVER : key.blend) {
   case VG_SHADER_FD_BLEND_SRC:
   {
      /*
         dst' = src

         with coverage:
         dst' = (src * cvg) + (dst * (1 - cvg))
      */

      if (key.coverage || key.masking) {
         if (!key.coverage) {                                    xor(p - 2, %[ loadam %], %[ nop %]);
         } else {                                                xor(p - 2, %[ loadcv %], %[ nop %]);
            if (key.masking) { APPEND(p,                         %[ mov  r1, r4          ; nop                ; loadam
                                                                    nop                  ; v8muld  r1, r1, r4          %]); } }
         APPEND_I(p, src | ((key.coverage && key.masking) << 1), %[ not  r1, %1          ; v8muld  r0, %0, %1 ; loadc  %| r0%/r4 %, ra0%/r4 %, r0%/r1 %, ra0%/r1 %]); /* r0: src * cvg, r1: 1 - cvg */
         APPEND_I(p, !pre_before_blend,                          %[ nop                  ; v8muld  r1, r1, %0          %| r_dst %, r4 %]);                            /* r1: dst * (1 - cvg) */
         APPEND(p,                                               %[ v8adds  tlbc, r0, r1 ; nop                         %]);
         if (pre_before_blend) {                                 xor(p - 6, %[ nop %], %[ loadc %]); }
      } else {
         APPEND_I(p, src,                                        %[ mov  tlbc, %0        ; nop                         %| r0 %, ra0 %]);
      }
      #define BLEND_SRC_INSTRS_N_MAX 5
      break;
   }
   case VG_SHADER_FD_BLEND_SRC_OVER:
   {
      /*
         dst' = src + (dst * (1 - c_a))

         with coverage:
         dst' = ((src + (dst * (1 - c_a))) * cvg) + (dst * (1 - cvg))
         dst' = (src * cvg) + (dst * (1 - c_a) * cvg) + (dst * (1 - cvg))
         dst' = (src * cvg) + (dst * (((1 - c_a) * cvg) + (1 - cvg)))
         dst' = (src * cvg) + (dst * (1 - (c_a * cvg)))

         we actually do darken/lighten blend here too when there is no alpha:
         dst' = min/max(src + (dst * (1 - c_a)), dst)
      */

      if (key.coverage || key.masking) {
         if (!key.coverage) {                                          xor(p - 2, %[ loadam %], %[ nop %]);
         } else {                                                      xor(p - 2, %[ loadcv %], %[ nop %]);
            if (key.masking) { APPEND(p,                               %[ mov  r2, r4          ; nop                ; loadam
                                                                          nop                  ; v8muld  r2, r2, r4          %]); } }
         APPEND_I(p, stencil_a | ((key.coverage && key.masking) << 1), %[ nop                  ; v8muld  r1, %0, %1          %| r1%/r4 %, ra0.8dr%/r4 %, r1%/r2 %, ra0.8dr%/r2 %]); /* r1: c_a * cvg */
         APPEND_I(p, src | ((key.coverage && key.masking) << 1),       %[ not  r1, r1          ; v8muld  r0, %0, %1 ; loadc  %| r0%/r4 %, ra0%/r4 %, r0%/r2 %, ra0%/r2 %]);         /* r0: src * cvg, r1: 1 - (c_a * cvg) */
         src = SRC_R0;
      } else { APPEND_I(p, stencil_a,                                  %[ not  r1, %0          ; nop                ; loadc  %| r1 %, ra0.8dr %]); }                                /* r1: 1 - c_a */
      if (pre_before_blend) {                                          xor(p - 2, %[ nop %], %[ loadc %]); }
      APPEND_I(p, !pre_before_blend,                                   %[ nop                  ; v8muld  r1, r1, %0          %| r_dst %, r4 %]);
      if (key.blend == VG_SHADER_FD_BLEND_SRC_OVER) { APPEND_I(p, src, %[ v8adds  tlbc, r1, %0 ; nop                         %| r0 %, ra0 %]);
      } else { vcos_assert(key.no_alpha && ((key.blend == VG_SHADER_FD_BLEND_DARKEN) || (key.blend == VG_SHADER_FD_BLEND_LIGHTEN)));
         APPEND_I(p, src,                                              %[ v8adds  r0, r1, %0   ; nop                         %| r0 %, ra0 %]);
         APPEND_I(p, darken | (!pre_before_blend << 1),                %[ nop                  ; %0  tlbc, r0, %1            %| v8max%/r_dst %, v8min%/r_dst %, v8max%/r4 %, v8min%/r4 %]);
         tlbc_in_mul = true;
      }
      #define BLEND_SRC_OVER_INSTRS_N_MAX 7
      break;
   }
   case VG_SHADER_FD_BLEND_DST_OVER:
   {
      /*
         dst' = (src * (1 - a_dst)) + dst

         with coverage:
         dst' = (((src * (1 - a_dst)) + dst) * cvg) + (dst * (1 - cvg))
         dst' = (src * (1 - a_dst) * cvg) + (dst * cvg) + (dst * (1 - cvg))
         dst' = (src * (1 - a_dst) * cvg) + dst

         no alpha:
         dst' = dst
         (which is a nop unless rendering to a non-premultiplied buffer)
      */

      if (key.no_alpha) {
         if (!pre_before_blend) {                                   xor(p - 2, %[ loadc %], %[ nop %]); }
         APPEND_I(p, !pre_before_blend,                             %[ mov  tlbc, %0        ; nop                         %| r_dst %, r4 %]);
      } else {
         if (key.coverage || key.masking) {
            if (!key.coverage) {                                    xor(p - 2, %[ loadam %], %[ nop %]);
            } else {                                                xor(p - 2, %[ loadcv %], %[ nop %]);
               if (key.masking) { APPEND(p,                         %[ mov  r1, r4          ; nop                ; loadam
                                                                       nop                  ; v8muld  r1, r1, r4          %]);
                  APPEND_I(p, !pre_before_blend | (src << 1),       %[ not  r1, %0          ; v8muld  r0, %1, r1          %| r_a_dst%/r0 %, r4.8dr%/r0 %, r_a_dst%/ra0 %, r4.8dr%/ra0 %]); } } /* r0: src * cvg, r1: 1 - a_dst */
            if (!(key.coverage && key.masking)) { APPEND_I(p, src,  %[ nop                  ; v8muld  r0, %0, r4          %| r0 %, ra0 %]); }                                                  /* r0: src * cvg */
            src = SRC_R0; }
         if (!(key.coverage && key.masking)) { APPEND_I(p, !pre_before_blend, %[ not  r1, %0          ; nop                         %| r_a_dst %, r4.8dr %]); }                                          /* r1: 1 - a_dst */
         APPEND_I(p, src,                                           %[ nop                  ; v8muld  r0, %0, r1          %| r0 %, ra0 %]);
         APPEND_I(p, !pre_before_blend,                             %[ v8adds  tlbc, r0, %0 ; nop                         %| r_dst %, r4 %]);
         if (!pre_before_blend) {                                   xor(p - 8, %[ loadc %], %[ nop %]); }
      }
      #define BLEND_DST_OVER_INSTRS_N_MAX 5
      break;
   }
   case VG_SHADER_FD_BLEND_SRC_IN:
   {
      /*
         dst' = src * a_dst

         with coverage:
         dst' = (src * a_dst * cvg) + (dst * (1 - cvg))

         no alpha:
         becomes the same as SRC blend. this gets converted in vg_hw_4.c, so we
         shouldn't get here!
      */

      vcos_assert(!key.no_alpha);

      if (key.coverage || key.masking) {
         if (!key.coverage) {                                    xor(p - 2, %[ loadam %], %[ nop %]);
         } else {                                                xor(p - 2, %[ loadcv %], %[ nop %]);
            if (key.masking) { APPEND(p,                         %[ mov  r1, r4          ; nop                  ; loadam
                                                                    nop                  ; v8muld  r1, r1, r4            %]); } }
         APPEND_I(p, src | ((key.coverage && key.masking) << 1), %[ not  r1, %1          ; v8muld  r0, %0, %1   ; loadc  %| r0%/r4 %, ra0%/r4 %, r0%/r1 %, ra0%/r1 %]); /* r0: src * cvg, r1: 1 - cvg */
         APPEND_I(p, !pre_before_blend,                          %[ nop                  ; v8muld  r0, r0, %0                                                           # r0: src * a_dst * cvg
                                                                    nop                  ; v8muld  r1, r1, %1            %| r_a_dst%/r_dst %, r4.8dr%/r4 %]);           /* r1: dst * (1 - cvg) */
         APPEND(p,                                               %[ v8adds  tlbc, r0, r1 ; nop                           %]);
         if (pre_before_blend) {                                 xor(p - 8, %[ nop %], %[ loadc %]); }
      } else {
         if (!pre_before_blend) {                                xor(p - 2, %[ loadc %], %[ nop %]); }
         APPEND_I(p, src | (!pre_before_blend << 1),             %[ nop                  ; v8muld  tlbc, %0, %1          %| r0%/r_a_dst %, ra0%/r_a_dst %, r0%/r4.8dr %, ra0%/r4.8dr %]);
         tlbc_in_mul = true;
      }
      #define BLEND_SRC_IN_INSTRS_N_MAX 6
      break;
   }
   case VG_SHADER_FD_BLEND_DST_IN:
   {
      /*
         dst' = dst * c_a

         with coverage:
         dst' = (dst * c_a * cvg) + (dst * (1 - cvg))
         dst' = dst * ((c_a * cvg) + (1 - cvg))
      */

      if (key.coverage || key.masking) {
         if (!key.coverage) {                                          xor(p - 2, %[ loadam %], %[ nop %]);
         } else {                                                      xor(p - 2, %[ loadcv %], %[ nop %]);
            if (key.masking) { APPEND(p,                               %[ mov  r2, r4        ; nop                  ; loadam
                                                                          nop                ; v8muld  r2, r2, r4            %]); } }
         APPEND_I(p, stencil_a | ((key.coverage && key.masking) << 1), %[ not  r1, %1        ; v8muld  r0, %0, %1            %| r1%/r4 %, ra0.8dr%/r4 %, r1%/r2 %, ra0.8dr%/r2 %]); /* r0: c_a * cvg, r1: 1 - cvg */
         APPEND(p,                                                     %[ v8adds  r0, r0, r1 ; nop                  ; loadc  %]);                                                   /* r0: (c_a * cvg) + (1 - cvg) */
         if (!key.pre && key.no_alpha) {
            need_pmc = true;
            vcos_assert(!pre_before_blend);
            APPEND(p,                                                  %[ or  r1, r4, r_pmc  ; nop
                                                                          nop                ; v8muld  tlbc, r0, r1          %]);
         } else {
            APPEND_I(p, !pre_before_blend,                             %[ nop                ; v8muld  tlbc, r0, %0          %| r_dst %, r4 %]);
            if (pre_before_blend) {                                    xor(p - 4, %[ nop %], %[ loadc %]); }
         }
      } else {
         if (!pre_before_blend) {                                      xor(p - 2, %[ loadc %], %[ nop %]); }
         if (!key.pre && key.no_alpha) {
            need_pmc = true;
            vcos_assert(!pre_before_blend);
            APPEND(p,                                                  %[ or  r0, r4, r_pmc  ; nop                           %]);
            APPEND_I(p, stencil_a,                                     %[ nop                ; v8muld  tlbc, %0, r0          %| r1 %, ra0.8dr %]);
         } else {
            APPEND_I(p, stencil_a | (!pre_before_blend << 1),          %[ nop                ; v8muld  tlbc, %0, %1          %| r1%/r_dst %, ra0.8dr%/r_dst %, r1%/r4 %, ra0.8dr%/r4 %]);
         }
      }
      tlbc_in_mul = true;
      #define BLEND_DST_IN_INSTRS_N_MAX 5
      break;
   }
   case VG_SHADER_FD_BLEND_MULTIPLY:
   {
      /*
         dst' = (src * (1 - a_dst)) + (dst * (1 - c_a)) + (src * dst)
         dst' = (src * ((1 - a_dst) + dst)) + (dst * (1 - c_a))

         with coverage:
         dst' = (((src * (1 - a_dst)) + (dst * (1 - c_a)) + (src * dst)) * cvg) + (dst * (1 - cvg))
         dst' = (src * (1 - a_dst) * cvg) + (dst * (1 - c_a) * cvg) + (src * dst * cvg) + (dst * (1 - cvg))
         dst' = (src * cvg * ((1 - a_dst) + dst)) + (dst * (1 - c_a) * cvg) + (dst * (1 - cvg))
         dst' = (src * cvg * ((1 - a_dst) + dst)) + (dst * (((1 - c_a) * cvg) + (1 - cvg)))
         dst' = (src * cvg * ((1 - a_dst) + dst)) + (dst * (1 - (c_a * cvg)))
         dst' = (src * cvg * ((1 - a_dst) + dst)) + (dst - (dst * c_a * cvg))

         no alpha:
         dst' = (src * dst) + (dst * (1 - c_a))
         dst' = dst * (src + (1 - c_a))

         no alpha, with coverage:
         dst' = (src * cvg * dst) + (dst - (dst * c_a * cvg))
         dst' = dst * ((src * cvg) + (1 - (c_a * cvg)))
      */

      if (key.coverage || key.masking) {
         if (!key.coverage) {                                         xor(p - 2, %[ loadam %], %[ nop %]);
         } else {                                                     xor(p - 2, %[ loadcv %], %[ nop %]);
            if (key.masking) { APPEND(p,                              %[ mov  r2, r4          ; nop                  ; loadam
                                                                         nop                  ; v8muld  r2, r2, r4            %]);
               APPEND_I(p, stencil_a,                                 %[ nop                  ; v8muld  r1, %0, r2   ; loadc  %| r1 %, ra0.8dr %]); } }                                        /* r1: c_a * cvg, r2: cvg */
         if (!(key.coverage && key.masking)) { APPEND_I(p, stencil_a, %[ mov  r2, r4          ; v8muld  r1, %0, r4   ; loadc  %| r1 %, ra0.8dr %]); }                                          /* r1: c_a * cvg, r2: cvg */
         if (pre_before_blend) {                                      xor(p - 2, %[ nop %], %[ loadc %]); } /* todo: loadc could be moved forward in no_alpha case */
         if (key.no_alpha) { APPEND_I(p, src,                         %[ not  r1, r1          ; v8muld  r0, %0, r2            %| r0 %, ra0 %]);                                                /* r1: 1 - (c_a * cvg), r0: src * cvg */
            APPEND(p,                                                 %[ v8adds  r0, r0, r1   ; nop                           %]);                                                             /* r0: (src * cvg) + (1 - (c_a * cvg)) */
            APPEND_I(p, !pre_before_blend,                            %[ nop                  ; v8muld  tlbc, %0, r0          %| r_dst %, r4 %]);
            tlbc_in_mul = true;
         } else { APPEND_I(p, !pre_before_blend | (src << 1),         %[ not  r2, %0          ; v8muld  r0, %1, r2            %| r_a_dst%/r0 %, r4.8dr%/r0 %, r_a_dst%/ra0 %, r4.8dr%/ra0 %]); /* r0: src * cvg, r2: 1 - a_dst */
            APPEND_I(p, !pre_before_blend,                            %[ v8adds  r2, r2, %0   ; v8muld  r1, r1, %0                                                                             # r1: dst * c_a * cvg, r2: (1 - a_dst) + dst
                                                                         v8subs  r1, %0, r1   ; v8muld  r0, r0, r2            %| r_dst %, r4 %]);                                              /* r0: src * cvg * ((1 - a_dst) + dst), r1: dst - (dst * c_a * cvg) */
            APPEND(p,                                                 %[ v8adds  tlbc, r0, r1 ; nop                           %]); }
      } else {
         if (key.no_alpha) { APPEND_I(p, stencil_a,                   %[ not  r1, %0          ; nop                           %| r1 %, ra0.8dr %]);                                            /* r1: 1 - c_a */
            APPEND_I(p, src,                                          %[ v8adds  r0, %0, r1   ; nop                  ; loadc  %| r0 %, ra0 %]);                                                /* r0: src + (1 - c_a) */
            APPEND_I(p, !pre_before_blend,                            %[ nop                  ; v8muld  tlbc, %0, r0          %| r_dst %, r4 %]);
            if (pre_before_blend) {                                   xor(p - 4, %[ nop %], %[ loadc %]); }
            tlbc_in_mul = true;
         } else {
            if (!pre_before_blend) {                                  xor(p - 2, %[ loadc %], %[ nop %]); }
            APPEND_I(p, !pre_before_blend,                            %[ not  r2, %0          ; nop                           %| r_a_dst %, r4.8dr %]);                                        /* r2: 1 - a_dst */
            APPEND_I(p, stencil_a | (!pre_before_blend << 1),         %[ v8adds  r2, r2, %1   ; v8muld  r1, %0, %1            %| r1%/r_dst %, ra0.8dr%/r_dst %, r1%/r4 %, ra0.8dr%/r4 %]);     /* r2: (1 - a_dst) + dst, r1: dst * c_a */
            APPEND_I(p, !pre_before_blend | (src << 1),               %[ v8subs  r1, %0, r1   ; v8muld  r0, %1, r2            %| r_dst%/r0 %, r4%/r0 %, r_dst%/ra0 %, r4%/ra0 %]);             /* r0: src * ((1 - a_dst) + dst), r1: dst * (1 - c_a) */
            APPEND(p,                                                 %[ v8adds  tlbc, r0, r1 ; nop                           %]);
         }
      }
      #define BLEND_MULTIPLY_INSTRS_N_MAX 7
      break;
   }
   case VG_SHADER_FD_BLEND_SCREEN:
   {
      /*
         dst' = src + (dst - (src * dst))

         with coverage:
         dst' = ((src + (dst - (src * dst))) * cvg) + (dst * (1 - cvg))
         dst' = (src * cvg) + (dst * (1 - src) * cvg) + (dst * (1 - cvg))
         dst' = (src * cvg) + (dst * (((1 - src) * cvg) + (1 - cvg)))
         dst' = (src * cvg) + (dst * (1 - (src * cvg)))
      */

      if (key.coverage || key.masking) {
         if (!key.coverage) {                                    xor(p - 2, %[ loadam %], %[ nop %]);
         } else {                                                xor(p - 2, %[ loadcv %], %[ nop %]);
            if (key.masking) { APPEND(p,                         %[ mov  r1, r4          ; nop                ; loadam
                                                                    nop                  ; v8muld  r1, r1, r4          %]); } }
         APPEND_I(p, src | ((key.coverage && key.masking) << 1), %[ nop                  ; v8muld  r0, %0, %1          %| r0%/r4 %, ra0%/r4 %, r0%/r1 %, ra0%/r1 %]);
         src = SRC_R0; }
      APPEND_I(p, src,                                           %[ not  r1, %0          ; nop                ; loadc  %| r0 %, ra0 %]);
      APPEND_I(p, !pre_before_blend,                             %[ nop                  ; v8muld  r1, r1, %0          %| r_dst %, r4 %]);
      APPEND_I(p, src,                                           %[ v8adds  tlbc, r1, %0 ; nop                         %| r0 %, ra0 %]);
      if (pre_before_blend) {                                    xor(p - 6, %[ nop %], %[ loadc %]); }
      #define BLEND_SCREEN_INSTRS_N_MAX 6
      break;
   }
   case VG_SHADER_FD_BLEND_DARKEN:
   case VG_SHADER_FD_BLEND_LIGHTEN:
   {
      /*
         dst' = min/max(src + (dst * (1 - c_a)),
                        dst + (src * (1 - a_dst)))

         with coverage:
         dst' = (min/max(src + (dst * (1 - c_a)),
                         dst + (src * (1 - a_dst))) * cvg) + (dst * (1 - cvg))
         dst' = min/max(((src + (dst * (1 - c_a))) * cvg) + (dst * (1 - cvg)),   (src over)
                        ((dst + (src * (1 - a_dst))) * cvg) + (dst * (1 - cvg))) (dst over)
         dst' = min/max((src * cvg) + (dst * (1 - (c_a * cvg))),
                        (src * (1 - a_dst) * cvg) + dst)

         no alpha:
         becomes almost the same as src over blend (just min'd/max'd with dst);
         we translate in the switch above
      */

      vcos_assert(!key.no_alpha);

      if (key.coverage || key.masking) {
         if (!key.coverage) {                                         xor(p - 2, %[ loadam %], %[ nop %]);
         } else {                                                     xor(p - 2, %[ loadcv %], %[ nop %]);
            if (key.masking) { APPEND(p,                              %[ mov  r2, r4        ; nop                ; loadam
                                                                         nop                ; v8muld  r2, r2, r4          %]);
               APPEND_I(p, stencil_a,                                 %[ nop                ; v8muld  r1, %0, r2 ; loadc  %| r1 %, ra0.8dr %]); } }                                        /* r1: c_a * cvg, r2: cvg */
         if (!(key.coverage && key.masking)) { APPEND_I(p, stencil_a, %[ mov  r2, r4        ; v8muld  r1, %0, r4 ; loadc  %| r1 %, ra0.8dr %]); }                                          /* r1: c_a * cvg, r2: cvg */
         APPEND_I(p, !pre_before_blend | (src << 1),                  %[ not  r2, %0        ; v8muld  r0, %1, r2          %| r_a_dst%/r0 %, r4.8dr%/r0 %, r_a_dst%/ra0 %, r4.8dr%/ra0 %]); /* r0: src * cvg, r2: 1 - a_dst */
         APPEND(p,                                                    %[ not  r1, r1        ; v8muld  r2, r0, r2          %]);                                                             /* r1: 1 - (c_a * cvg), r2: src * (1 - a_dst) * cvg */
         APPEND_I(p, !pre_before_blend,                               %[ v8adds  r2, r2, %0 ; v8muld  r1, r1, %0          %| r_dst %, r4 %]);                                              /* r1: dst * (1 - (c_a * cvg)), r2: (src * (1 - a_dst) * cvg) + dst */
         APPEND(p,                                                    %[ v8adds  r0, r0, r1 ; nop                         %]);                                                             /* r0: (src * cvg) + (dst * (1 - (c_a * cvg))) */
         APPEND_I(p, darken,                                          %[ nop                ; %0  tlbc, r0, r2            %| v8max %, v8min %]);
         if (pre_before_blend) {                                      xor(p - 12, %[ nop %], %[ loadc %]); }
      } else {
         if (!pre_before_blend) {                                     xor(p - 2, %[ loadc %], %[ nop %]); }
         APPEND_I(p, src | (!pre_before_blend << 1),                  %[ mov  r2, %0        ; v8muld  r0, %0, %1          %| r0%/r_a_dst %, ra0%/r_a_dst %, r0%/r4.8dr %, ra0%/r4.8dr %]);
         APPEND_I(p, stencil_a | (!pre_before_blend << 1),            %[ v8subs  r0, r2, r0 ; v8muld  r1, %0, %1          %| r1%/r_dst %, ra0.8dr%/r_dst %, r1%/r4 %, ra0.8dr%/r4 %]);     /* r0: src * (1 - a_dst), r1: dst * c_a */
         APPEND_I(p, !pre_before_blend,                               %[ v8subs  r1, %0, r1 ; v8adds  r0, %0, r0          %| r_dst %, r4 %]);                                              /* r0: dst + (src * (1 - a_dst)), r1: dst * (1 - c_a) */
         APPEND(p,                                                    %[ nop                ; v8adds  r1, r2, r1          %]);                                                             /* r1: src + (dst * (1 - c_a)) */
         APPEND_I(p, darken,                                          %[ nop                ; %0  tlbc, r0, r1            %| v8max %, v8min %]);
      }
      tlbc_in_mul = true;
      #define BLEND_DARKEN_LIGHTEN_INSTRS_N_MAX 8
      break;
   }
   case VG_SHADER_FD_BLEND_ADDITIVE:
   {
      /*
         dst' = src + dst

         with coverage:
         dst' = ((src + dst) * cvg) + (dst * (1 - cvg))

         todo: (src * cvg) + dst is almost identical (and obviously simpler), is
         it acceptable?
      */

      if (!pre_before_blend) {                    xor(p - 2, %[ loadc %], %[ nop %]); }
      if (key.coverage || key.masking) {
         APPEND_I(p, src | (!pre_before_blend << 1), %[ v8adds  r0, %0, %1   ; mov  r1, %1        ; loadcv %| r0%/r_dst %, ra0%/r_dst %, r0%/r4 %, ra0%/r4 %]); /* r0: src + dst, r1: dst */
         if (!key.coverage) {                     xor(p - 2, %[ loadam %], %[ loadcv %]);
         } else if (key.masking) { APPEND(p,      %[ mov  r2, r4          ; nop                ; loadam
                                                     nop                  ; v8muld  r2, r2, r4          %]); }
         APPEND_I(p, key.coverage && key.masking, %[ not  r2, %0          ; v8muld  r0, r0, %0          %| r4 %, r2 %]);                                     /* r0: (src + dst) * cvg, r2: 1 - cvg */
         APPEND(p,                                %[ nop                  ; v8muld  r1, r1, r2                                                               # r1: dst * (1 - cvg)
                                                     v8adds  tlbc, r0, r1 ; nop                         %]);
      } else {
         APPEND_I(p, src | (!pre_before_blend << 1), %[ v8adds  tlbc, %0, %1 ; nop                         %| r0%/r_dst %, ra0%/r_dst %, r0%/r4 %, ra0%/r4 %]);
      }
      #define BLEND_ADDITIVE_INSTRS_N_MAX 6
      break;
   }
   default:
   {
      UNREACHABLE();
   }
   }
   #define BLEND_INSTRS_N_MAX _max( \
      BLEND_SRC_INSTRS_N_MAX, _max( \
      BLEND_SRC_OVER_INSTRS_N_MAX, _max( \
      BLEND_DST_OVER_INSTRS_N_MAX, _max( \
      BLEND_SRC_IN_INSTRS_N_MAX, _max( \
      BLEND_DST_IN_INSTRS_N_MAX, _max( \
      BLEND_MULTIPLY_INSTRS_N_MAX, _max( \
      BLEND_SCREEN_INSTRS_N_MAX, _max( \
      BLEND_DARKEN_LIGHTEN_INSTRS_N_MAX, \
      BLEND_ADDITIVE_INSTRS_N_MAX))))))))

   %[ .unset r_a_dst %]
   %[ .unset r_dst %]

   /*
      if the framebuffer isn't premultiplied, we need to unpremultiply after
      blending

      also handle scissor here... (we need to handle it differently if we're
      unpreing)
   */

   if (unpre_after_blend) {
      if (tlbc_in_mul) {
         xor(p - 2, %[ nop ; mov  ra0, 0 %], %[ nop ; mov  tlbc, 0 %]);
      } else {
         xor(p - 2, %[ mov  ra0, 0 ; nop %], %[ mov  tlbc, 0 ; nop %]);
      }
      need_pmc = true;
      APPEND(p,    %[ mov  r2, f(256.0)
                      fadd  recip, ra0.8dc, f(0.0) ; nop
                      and  r0, ra0, r_pmc          ; nop
                      nop                          ; fmul  r1, ra0.8ac, f(1.0)
                      fsub.setf  -, r2, r4         ; fmul  r2, ra0.8bc, r4
                      nop                          ; fmul  r3, ra0.8cc, r4
                      nop                          ; fmul.ifnn  r0.8ac, r1, r4
                      nop                          ; mov.ifnn  r0.8bc, r2      %]);
      if (key.scissor) {
         APPEND(p, %[ mov.setf  -, r_scissor       ; mov.ifnn  r0.8cc, r3
                      mov.ifnz  tlbc, r0           ; nop                       %]);
      } else {
         APPEND(p, %[ nop                          ; mov.ifnn  r0.8cc, r3
                      mov  tlbc, r0                ; nop                       %]); }
   } else if (key.scissor) {
      if (!so_solid) { /* setf already done for solid paint */
         APPEND(p, %[ mov.setf  -, r_scissor ; nop %]);
         swap(p - 2, p - 4);
      }
      if (tlbc_in_mul) {
         xor(p - 2, %[ nop ; mov.ifnz  ra0, 0 %], %[ nop ; mov  ra0, 0 %]);
      } else {
         xor(p - 2, %[ mov.ifnz  ra0, 0 ; nop %], %[ mov  ra0, 0 ; nop %]);
      }
   }
   #define SCISSOR_SETF_AND_UNPRE_INSTRS_N_MAX 10

   %[ .unset r_scissor %]

   /*
      thrend...
      todo: on b0, put thrend in blend. implicit sbdone?
   */

   APPEND(p, %[ nop ; nop ; sbdone
                nop ; nop ; thrend
                nop ; nop
                nop ; nop          %]);
   #define THREND_INSTRS_N_MAX 4

   /*
      scissor early out
   */

   if (key.scissor && !so_solid) {
      *scissor_branch = (p - (scissor_branch + 8)) * sizeof(uint32_t);
      /* todo: no need for these thrsws on b0 (we'll need one "final" thrsw though?) */
      /* need 5 total. already done extra_thrsws + 1 */
      for (i = 0; i != (4 - extra_thrsws); ++i) {
         APPEND(p,                  %[ nop ; nop ; thrsw
                                       nop ; nop
                                       nop ; nop          %]); }
      APPEND(p,                     %[ nop ; nop ; sbwait %]);
      if (key.coverage) { APPEND(p, %[ nop ; nop ; loadcv %]); } /* must load coverage to clear */
      APPEND(p,                     %[ nop ; nop ; sbdone
                                       nop ; nop ; thrend
                                       nop ; nop
                                       nop ; nop          %]);
   }
   #define SCISSOR_EARLY_INSTRS_N_MAX 18

   /*
      check we set r_pmc iff necessary
   */

   vcos_assert(need_pmc == have_pmc);
   %[ .unset r_pmc %]

   return p;
}
#define INSTRS_N_MAX ( \
   EXTRA_THRSWS_INSTRS_N_MAX + \
   PMC_INSTRS_N_MAX + \
   SCISSOR_FETCH_INSTRS_N_MAX + \
   PAINT_FETCH_INSTRS_N_MAX + \
   IMAGE_FETCH_INSTRS_N_MAX + \
   MULTIPLY_INSTRS_N_MAX + \
   COLOR_TRANSFORM_INSTRS_N_MAX + \
   COLORSPACE_CONVERT_AND_STENCIL_INSTRS_N_MAX + \
   BLEND_SRC_PREP_INSTRS_N_MAX + \
   BLEND_DST_PREP_INSTRS_N_MAX + \
   BLEND_INSTRS_N_MAX + \
   SCISSOR_SETF_AND_UNPRE_INSTRS_N_MAX + \
   THREND_INSTRS_N_MAX + \
   SCISSOR_EARLY_INSTRS_N_MAX)

/******************************************************************************
interface
******************************************************************************/

static KHRN_FLEAKY_MAP_T fleaky_map;
static KHRN_FLEAKY_MAP_ENTRY_T fleaky_map_entries[64];

void vg_shader_init(void)
{
   khrn_fleaky_map_init(&fleaky_map, fleaky_map_entries, ARR_COUNT(fleaky_map_entries));
}

void vg_shader_term(void)
{
   khrn_fleaky_map_term(&fleaky_map);
}

uint32_t vg_shader_fd_get_extra_thrsws(VG_SHADER_FD_KEY_T key) /* todo: no need for this on b0 */
{
   uint32_t extra_thrsws = 0;

   /* thrsw for scissor */
   if (!key.scissor) { ++extra_thrsws; }

   /* thrsw between paint and image fetches (to avoid fifo overflow) */
   if (/* either only have paint or image */
      (key.draw_type == VG_SHADER_FD_DRAW_TYPE_PAINT) ||
      (key.draw_type == VG_SHADER_FD_DRAW_TYPE_IMAGE) ||
      /* or paint tu lookup doesn't need more than 2 input fifo entries */
      (((key.paint_type != VG_SHADER_FD_PAINT_TYPE_PATTERN) ||
      (!key.paint_pattern_tile_fill && !key.paint_pattern_child)) &&
      /* and no paint tu lookup or image tu lookup doesn't need more than 2
       * input fifo entries */
      ((key.paint_type == VG_SHADER_FD_PAINT_TYPE_SOLID) ||
      !key.image_child))) { ++extra_thrsws; }

   /* thrsw after paint/image fetches */
   if ((key.draw_type == VG_SHADER_FD_DRAW_TYPE_PAINT) &&
      (key.paint_type == VG_SHADER_FD_PAINT_TYPE_SOLID)) { ++extra_thrsws; }

   /* thrsws for colorspace conversion */
   if (!key.colorspace_convert) { extra_thrsws += 2; }

   return extra_thrsws;
}

bool vg_shader_fd_need_stencil_a(VG_SHADER_FD_BLEND_T blend)
{
   return (blend == VG_SHADER_FD_BLEND_SRC_OVER) ||
          (blend == VG_SHADER_FD_BLEND_DST_IN) ||
          (blend == VG_SHADER_FD_BLEND_MULTIPLY) ||
          (blend == VG_SHADER_FD_BLEND_DARKEN) ||
          (blend == VG_SHADER_FD_BLEND_LIGHTEN);
}

MEM_HANDLE_T vg_shader_fd_get(VG_SHADER_FD_KEY_T key)
{
   MEM_HANDLE_T handle = khrn_fleaky_map_lookup(&fleaky_map, *(uint32_t *)&key);
   if (handle == MEM_INVALID_HANDLE) {
      uint32_t *p;
      uint32_t size;
#ifdef VG_SHADER_FD_DUMP_SHADERS
#ifdef VG_SHADER_FD_DQASM
      FILE *f;
#endif
      uint32_t i;
#endif

      /*
         alloc (assume worst-case size)
      */

      handle = mem_alloc_ex(INSTRS_N_MAX * 2 * sizeof(uint32_t), 8, (MEM_FLAG_T)(MEM_FLAG_DIRECT | MEM_FLAG_NO_INIT | MEM_FLAG_RESIZEABLE), "vg_shader_fd", MEM_COMPACT_DISCARD);
      if (handle == MEM_INVALID_HANDLE) {
         return MEM_INVALID_HANDLE;
      }

      /*
         generate shader
      */

      p = (uint32_t *)mem_lock(handle);
      size = (vg_shader_fd_gen(p, key) - p) * sizeof(uint32_t);
      vcos_assert(size <= (INSTRS_N_MAX * 2 * sizeof(uint32_t)));
#ifdef VG_SHADER_FD_DUMP_SHADERS
      printf("shader (handle: 0x%08x, key: 0x%08x) {\n", handle, *(uint32_t *)&key);
#ifdef VG_SHADER_FD_DQASM
      f = _popen(VG_SHADER_FD_DQASM, "wt");
      if (vcos_verify(f)) {
#endif
         for (i = 0; i != (size / sizeof(uint32_t)); i += 2) {
#ifdef VG_SHADER_FD_DQASM
            fprintf(f,
#else
            printf(
#endif
               "0x%08x_%08x\n", p[i + 1], p[i]);
         }
#ifdef VG_SHADER_FD_DQASM
         _pclose(f);
      }
#endif
      printf("}\n");
#endif
      mem_unlock(handle);

      /*
         shrink down to actual size
      */

      verify(mem_resize_ex(handle, size, MEM_COMPACT_NONE));

      /*
         put in map
      */

      khrn_fleaky_map_insert(&fleaky_map, *(uint32_t *)&key, handle);
      mem_release(handle); /* map now owns handle */
   }
   return handle;
}

/******************************************************************************
clear shader
******************************************************************************/

const uint32_t VG_SHADER_FD_CLEAR_EXTRA_THRSWS = 5; /* todo: remove for b0 */

/* todo: on b0, cut this down... */
const uint32_t VG_SHADER_FD_CLEAR[] = { %[
   nop             ; nop ; thrsw
   nop             ; nop
   nop             ; nop
   nop             ; nop ; thrsw
   nop             ; nop
   nop             ; nop
   nop             ; nop ; thrsw
   nop             ; nop
   nop             ; nop
   nop             ; nop ; thrsw
   nop             ; nop
   nop             ; nop
   nop             ; nop ; thrsw
   nop             ; nop
   nop             ; nop
   nop             ; nop          # sbwait can't be first instr
   mov  tlbc, unif ; nop ; sbwait # of shader (todo: on b0 too?)
   nop             ; nop ; sbdone
   nop             ; nop ; thrend
   nop             ; nop
   nop             ; nop                                         %] };

const uint32_t VG_SHADER_FD_CLEAR_SIZE = sizeof(VG_SHADER_FD_CLEAR);
