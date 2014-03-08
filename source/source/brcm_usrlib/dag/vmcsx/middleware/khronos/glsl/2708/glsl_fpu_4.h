/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GLSL_FPU_4_H
#define GLSL_FPU_4_H

#if defined(WIN32) || defined(__CC_ARM)
extern int glsl_fpu_add (unsigned *r, unsigned a, unsigned b);
extern int glsl_fpu_sub (unsigned *r, unsigned a, unsigned b);
extern int glsl_fpu_rsub (unsigned *r, unsigned a, unsigned b);
extern int glsl_fpu_mul (unsigned *r, unsigned a, unsigned b);
extern int glsl_fpu_muln (unsigned *r, unsigned a, unsigned b);
extern int glsl_fpu_min (unsigned *r, unsigned a, unsigned b);
extern int glsl_fpu_max (unsigned *r, unsigned a, unsigned b);
extern int glsl_fpu_floor (unsigned *r, unsigned b);
extern int glsl_fpu_ceil (unsigned *r, unsigned b);
extern int glsl_fpu_log2 (unsigned *r, unsigned b);
extern int glsl_fpu_exp2 (unsigned *r, unsigned b);
extern int glsl_fpu_recip (unsigned *r, unsigned b);
extern int glsl_fpu_rsqrt (unsigned *r, unsigned b);

extern void glsl_fpu_maxabs (unsigned *r, unsigned a, unsigned b);
extern void glsl_fpu_floattouintz (unsigned *r, unsigned a, unsigned b);
extern void glsl_fpu_floattouintn (unsigned *r, unsigned a, unsigned b);
extern void glsl_fpu_floattointz (unsigned *r, unsigned a, unsigned b);
extern void glsl_fpu_floattointn (unsigned *r, unsigned a, unsigned b);
extern int glsl_fpu_inttofloat (unsigned *r, unsigned a, unsigned b);
extern int glsl_fpu_uinttofloat (unsigned *r, unsigned a, unsigned b);
extern int glsl_fpu_floattouintq (unsigned *r, unsigned a, unsigned b);
#else
#include <vc/intrinsics.h>

static inline int glsl_fpu_add (unsigned *r, unsigned a, unsigned b)
{
   float fa = *(float *)&a;
   float fb = *(float *)&b;

   *(float *)r = fa + fb;
   return 0;
}

static inline int glsl_fpu_sub (unsigned *r, unsigned a, unsigned b)
{
   float fa = *(float *)&a;
   float fb = *(float *)&b;

   *(float *)r = fa - fb;
   return 0;
}

static inline int glsl_fpu_mul (unsigned *r, unsigned a, unsigned b)
{
   float fa = *(float *)&a;
   float fb = *(float *)&b;

   *(float *)r = fa * fb;
   return 0;
}

static inline int glsl_fpu_min (unsigned *r, unsigned a, unsigned b)
{
   float fa = *(float *)&a;
   float fb = *(float *)&b;

   *(float *)r = fa < fb ? fa : fb;
   return 0;
}

static inline int glsl_fpu_max (unsigned *r, unsigned a, unsigned b)
{
   float fa = *(float *)&a;
   float fb = *(float *)&b;

   *(float *)r = fa > fb ? fa : fb;
   return 0;
}

static inline int glsl_fpu_log2 (unsigned *r, unsigned a)
{
   float fa = *(float *)&a;

   *(float *)r = _flog2(fa);
   return 0;
}

static inline int glsl_fpu_exp2 (unsigned *r, unsigned a)
{
   float fa = *(float *)&a;

   *(float *)r = _fexp2(fa);
   return 0;
}

static inline int glsl_fpu_recip (unsigned *r, unsigned a)
{
   float fa = *(float *)&a;

   *(float *)r = _frecip(fa);
   return 0;
}

static inline int glsl_fpu_rsqrt (unsigned *r, unsigned a)
{
   float fa = *(float *)&a;

   *(float *)r = _frsqrt(fa);
   return 0;
}

static inline void glsl_fpu_floattointz (unsigned *r, unsigned a, unsigned b)
{
   float fa = *(float *)&a;

   *(float *)r = _floattoint(fa, 0);      // scalar float to int rounds to zero
}

extern void glsl_fpu_floattointn (unsigned *r, unsigned a, unsigned b);         // no scalar equivalent so use emulation

static inline int glsl_fpu_inttofloat (unsigned *r, unsigned a, unsigned b)
{
   *(float *)r = _inttofloat(a, b);

   return 0;
}

static inline int glsl_fpu_ceil (unsigned *r, unsigned a)
{
   float fa = *(float *)&a;

   *(float *)r = _fceil(fa);

   return 0;
}

static inline int glsl_fpu_floor (unsigned *r, unsigned a)
{
   float fa = *(float *)&a;

   *(float *)r = _ffloor(fa);

   return 0;
}
#endif

/* These are flags in the p0 register.  They are set whenever the
   corresponding exceptions occur.  */
#define FPE_IO 0x01             /* Invalid Operation.  */
#define FPE_DZ 0x02             /* Divide by Zero.  */
#define FPE_OF 0x04             /* Overflow.  */
#define FPE_UF 0x08             /* Underflow.  */
/* Inexact exceptions are excessively buggy in A0 sillicon, and have
   been dropped from B0 onwards. So we don't try to support them in
   the sim at all.  */
/* #define FPE_I  0x10 */       /* Inexact.  */

/* Check for a NaN.  */
#define IS_NAN(f) ((((unsigned) f) & ~(1U << 31)) > (0xFF << 23))

#endif