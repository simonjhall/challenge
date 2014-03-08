/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "middleware/khronos/glsl/glsl_common.h"

#include <limits.h>
#ifdef NATIVE_FLOATS
#include <math.h>
#endif

#include "middleware/khronos/glsl/2708/glsl_fpu_4.h"

#ifdef WINDOWS
#define int64 __int64
#define uint64 unsigned __int64
#else
#define int64 long long
#define uint64 unsigned long long
#endif

#if defined(WIN32)  || defined(__CC_ARM)
static void float_unpack (uint64 *mantissa, int *exponent, int *sign, unsigned input);
static int float_pack (unsigned *r, uint64 mantissa, int exponent, int sign, int round_info);

static int float_to_int (unsigned *r, unsigned a, unsigned b, int round_to_nearest);
static int float_to_unsigned (unsigned *r, unsigned a, unsigned b, int round_to_nearest);

static int fn_recip (unsigned *r, unsigned f);
static int fn_rsqrt (unsigned *r, unsigned f);
static int fn_log2 (unsigned *r, unsigned f);
static int fn_exp2 (unsigned *r, unsigned f);

static void float_unpack (uint64 *mantissa, int *exponent, int *sign, unsigned input)
{
   *sign = !! (input & (1 << 31));
   *exponent = (input >> 23) & 0xFF;

   if (*exponent == 0xFF) {
      *exponent = INT_MAX;
      *mantissa = (input & ((1 << 23) - 1));
   } else if (*exponent == 0) {
      *exponent = INT_MIN;
      *mantissa = 0;
   } else {
      /* Undo bias.  */
      *exponent -= 127;
      /*  Integerise the mantissa.  */
      *exponent -= 23;
      /* Add in the hidden bit.  */
      *mantissa = (1 << 23) | (input & ((1 << 23) - 1));;
   }

   return;
}

static int float_pack (unsigned *r, uint64 mantissa, int exponent, int sign, int round_info)
{
   const uint64 one = 1;
   int exceptions = 0;
   int highbit;
   int adjust;

   vcos_assert (exponent != INT_MAX);

   if (mantissa == 0) {
      *r = sign << 31;
      return 0;
   }

   vcos_assert (mantissa < (one << 48));
   highbit = 47;
   while ((mantissa & (one << highbit)) == 0)
      highbit--;

   vcos_assert (mantissa < (one << (highbit + 1)));

   adjust = highbit - 23;
   if (adjust < 0) {
      /* Just zero-extend the mantissa.  */
      mantissa <<= -adjust;
   } else if (adjust > 0) {
      uint64 discarded_bits;
      discarded_bits = mantissa & ((one << adjust) - 1);

      if (discarded_bits != 0) {
         if (discarded_bits > (one << (adjust-1))
               || (discarded_bits == (one << (adjust-1))
                   && round_info >= 0)) {
            /* Round away from zero.  */
            mantissa += (one << (adjust-1));
         } /* else always round towards zero.  */
      }

      if ((mantissa & (one << (highbit + 1))) != 0) {
         /* Rounding might have caused us to grow another bit. */
         adjust++;
      }

      mantissa >>= adjust;
   }
   exponent += adjust;
   exponent += 127;
   exponent += 23;

   /* Check that we've got our (soon to be) implicit bit in the correct place.  */
   vcos_assert (mantissa & (1 << 23));

   /* Remove the implicit bit. */
   mantissa &= ((one << 23) - 1);

   if (exponent <= 0) {
      /* Underflow.  Make sure to keep the sign.  */
      *r = (sign << 31);
      return FPE_UF;
   } else if (exponent >= 0xFF) {
      /* Overflow.  */
      *r = (sign << 31) | 0x7f800000;
      return FPE_OF;
   }

   *r = (unsigned)((sign << 31) | (exponent << 23) | mantissa);
   return exceptions;
}

extern int glsl_fpu_mul (unsigned *r, unsigned a, unsigned b)
{
#ifdef NATIVE_FLOATS
   float fa = *(float *)&a;
   float fb = *(float *)&b;

   *(float *)r = fa * fb;
   return 0;
#else
   uint64 m1, m2, m;
   int e1, e2, e;
   int s1, s2, s;

   float_unpack (&m1, &e1, &s1, a);
   float_unpack (&m2, &e2, &s2, b);

   s = s1 ^ s2;

   if ((e1 == INT_MAX && m1 != 0) || (e2 == INT_MAX && m2 != 0)) {
      /* NaN * anything => Invalid Operation.  */
      *r = /* (s << 31) | */ 0x7f800001;
      return FPE_IO;
   }

   if (   (e1 == INT_MAX && m2 == 0 && e2 == INT_MIN)
          || (e2 == INT_MAX && m1 == 0 && e1 == INT_MIN)) {
      /* Infinity times zero => Invalid Operation.  */
      /* Always _positive_ NaN in this case, for some reason.  */
      *r = /* (s << 31) | */ 0x7f800001;
      return FPE_IO;
   }

   if (e1 == INT_MAX || e2 == INT_MAX) {
      /* Infinity times anything else => Infinity.  */
      *r = (s << 31) | 0x7f800000;
#ifdef __BCM2707A0__
      return FPE_OF;
#else
   return 0;
#endif
   }

   if (m1 == 0 || m2 == 0) {
      /* Non-infity times zero => +-0.  */
      *r = (s << 31);
      return 0;
   }

   /* Ordinary case.  */
   e = e1 + e2;
   m = m1 * m2;

   return float_pack (r, m, e, s, 0);
#endif /* ! defined (NATIVE_FLOATS) */
}

extern int glsl_fpu_muln (unsigned *r, unsigned a, unsigned b)
{
#ifdef NATIVE_FLOATS
   float fa = *(float *)&a;
   float fb = *(float *)&b;

   *(float *)r = -(fa * fb);
   return 0;
#else
   return glsl_fpu_mul (r, a ^ (1 << 31), b);
#endif /* ! defined (NATIVE_FLOATS) */
}

extern int glsl_fpu_add (unsigned *r, unsigned a, unsigned b)
{
#ifdef NATIVE_FLOATS
   float fa = *(float *)&a;
   float fb = *(float *)&b;

   *(float *)r = fa + fb;
   return 0;
#else
   const uint64 one = 1;
   uint64 m1, m2, m;
   int e1, e2, e;
   int s1, s2, s;

   int flags = 0;
   int sticky_bit = 0;

   float_unpack (&m1, &e1, &s1, a);
   float_unpack (&m2, &e2, &s2, b);

   /* Special-case -0.0 + +-0.0 since our fadd is not commutative in
   this particular case.  */
   if (   e1 == INT_MIN && m1 == 0    /* a == +-0 */
          && e2 == INT_MIN && m2 == 0) { /* b == +-0 */
      if (s1 == 0) {
         /* +0.0 + +-0.0  */
         *r = 0;
      } else {
         /* -0.0 + +-0.0  */
         *r = (1 << 31);
      }
      return 0;
   }

   if ((e1 == INT_MAX && m1 != 0) || (e2 == INT_MAX && m2 != 0)) {
      /* NaN + anything => NaN.  */
      *r = 0x7f800001;
      return FPE_IO;
   }

   /* Infinity + infinity?  */
   if (e1 == INT_MAX && e2 == INT_MAX) {
      /* If the signs are the same, return that.  */
      if (s1 == s2) {
         *r = (s1 << 31) | 0x7f800000;
         return FPE_OF;
      } else {
         *r = 0x7f800001;
         return FPE_IO;
      }
   }

   /* Infinity + something else?  */
   if (e1 == INT_MAX) {
      *r = (s1 << 31) | 0x7f800000;
      return FPE_OF;
   }

   if (e2 == INT_MAX) {
      *r = (s2 << 31) | 0x7f800000;
      return FPE_OF;
   }

   if (m1 == 0) {
      e1 = e2;
   }

   if (m2 == 0) {
      e2 = e1;
   }

   /* Ordinary case.  */

   /* The - 2 is to make room for the guard and round bits.  */
   if (e1 > e2)
      e = e1 - 2;
   else
      e = e2 - 2;

   /* Adjust exponent of first operand.  */
   if (e1 < e) {
      int superfluous = e - e1;

      if (superfluous < 64) {
         sticky_bit = !! (m1 & ((one << superfluous) - 1));
         m1 >>= superfluous;
      } else {
         m1 = 0;
      }
   } else {
      /* e1 >= e */
      m1 <<= e1 - e;
   }

   /* Adjust exponent of second operand.  */
   if (e2 < e) {
      int superfluous = e - e2;

      if (superfluous < 64) {
         vcos_assert (sticky_bit == 0); /* At most one operand will be shifted right.  */
         sticky_bit = !! (m2 & ((one << superfluous) - 1));
         m2 >>= superfluous;
      } else {
         m2 = 0;
      }
   } else {
      /* e2 >= e */
      m2 <<= e2 - e;
   }

   if (s1 == s2) {
      m = m1 + m2;
      s = s1;
   } else {
      /* Which operand has the largest absolute value?  Note that in a
      tie, the sign of a wins.  */
      if ((a & ~(1 << 31)) >= (b & ~(1 << 31))) {
         /* a is larger (or equal), so the result is negative iff a is.  */
         s = s1;
         m = m1 - m2;
      } else {
         /* b is larger so the result is negative iff b is.  */
         s = s2;
         m = m2 - m1;
      }
      sticky_bit = -sticky_bit;
   }

   return flags | float_pack (r, m, e, s, sticky_bit);
#endif /* ! defined (NATIVE_FLOATS) */
}

extern int glsl_fpu_sub (unsigned *r, unsigned a, unsigned b)
{
#ifdef NATIVE_FLOATS
   float fa = *(float *)&a;
   float fb = *(float *)&b;

   *(float *)r = fa - fb;
   return 0;
#else
   return glsl_fpu_add (r, a, b ^ (1 << 31));
#endif /* ! defined (NATIVE_FLOATS) */
}

extern int glsl_fpu_rsub (unsigned *r, unsigned a, unsigned b)
{
#ifdef NATIVE_FLOATS
   float fa = *(float *)&a;
   float fb = *(float *)&b;

   *(float *)r = fb - fa;
   return 0;
#else
   return glsl_fpu_add (r, a ^ (1 << 31), b);
#endif /* ! defined (NATIVE_FLOATS) */
}

extern int glsl_fpu_div (unsigned *r, unsigned a, unsigned b)
{
#ifdef NATIVE_FLOATS
   float fa = *(float *)&a;
   float fb = *(float *)&b;

   *(float *)r = fa / fb;
   return 0;
#else
   int s1, s2, s;
   int e1, e2;
   uint64 m1, m2, q, rem;
   int exceptions = 0;

   float_unpack (&m1, &e1, &s1, a);
   float_unpack (&m2, &e2, &s2, b);
   s = s1 ^ s2;

   if (   (e1 == INT_MAX && m1 != 0)
          || (e2 == INT_MAX && m2 != 0)) {
      /* NaNs.  */
      *r = (0xFF << 23) + 1;
      return FPE_IO;
   }

   if (e2 == INT_MAX) {
      if (e1 == INT_MAX) {
         /* Inf/Inf => NaN  */
         *r = (0xFF << 23) + 1;
         return FPE_IO;
      } else {
         /* Return (signed) zero.  */
         *r = s << 31;
#ifdef __BCM2707A0__
         if (e1 != INT_MIN)
            return FPE_UF;
         else
            return 0;
#else
   return 0;
#endif
      }
   } else if (e1 == INT_MAX) {
      /* Inf / non-Inf => Inf */
      *r = (s << 31) | (0xFF << 23);
#ifdef __BCM2707A0__
      if (m2 == 0)
         return FPE_DZ;
      else
         return FPE_OF;
#else
   return 0;
#endif
   }

   if (m2 == 0) {
      /* Divide by zero.  */
      if (m1 != 0) {
         *r = (s << 31) | (0xFF << 23);
         return FPE_DZ;
      } else {
         /* 0/0 => +NaN, not +-Inf.  */
         *r = (0xFF << 23) + 1;
         return FPE_IO;
      }
   } else if (m1 == 0) {
      /* 0 / whatever.  */
      *r = (s << 31);
      return 0;
   }

   /* Common case.  */

   if (m1 < m2) {
      /* This ensures that 1.0 <= m1/m2 < 2.0 always.  */
      m1 <<= 1;
      e1--;
   }

   m1 <<= 23;
   e1 -= 23;

   q = m1 / m2;

   /* Verify that the higest set bit is in the correct place.  */
   vcos_assert (q & (1 << 23));
   vcos_assert (q < (1 << 24));

   rem = m1 % m2;
#ifndef __BCM2707A0__
   if (2*rem >= m2) {
      q++;
   }
#endif

   return exceptions | float_pack (r, q, e1 - e2, s, 0);
#endif /* ! defined (NATIVE_FLOATS) */
}

extern int glsl_fpu_recip (unsigned *r, unsigned b)
{
#ifdef NATIVE_FLOATS
   float fb = *(float *)&b;

   *(float *)r = 1.0 / fb;
   return 0;
#else
   if ((b & ~(1 << 31)) > (0xFF << 23)) {
      *r = (0xFF << 23) + 1;
      return FPE_IO;
   } else {
      return fn_recip (r, b);
   }
#endif /* ! defined (NATIVE_FLOATS) */
}

extern int glsl_fpu_rsqrt (unsigned *r, unsigned b)
{
#ifdef NATIVE_FLOATS
   float fb = *(float *)&b;

   *(float *)r = (float) (1.0 /sqrt (fb));
   return 0;
#else
   if (IS_NAN (b)) {
      *r = (0xFF << 23) + 1;
      return FPE_IO;
   } else {
      return fn_rsqrt (r, b);
   }
#endif /* ! defined (NATIVE_FLOATS) */
}

extern int glsl_fpu_min (unsigned *r, unsigned a, unsigned b)
{
#ifdef NATIVE_FLOATS
   float fa = *(float *)&a;
   float fb = *(float *)&b;

   if (fa > fb)
      *(float *)r = fb;
   else
      *(float *)r = fa;

   return 0;
#else
   unsigned aa = a;
   unsigned bb = b;

   if (IS_NAN (a) || IS_NAN (b)) {
      *r = (0xFF << 23) + 1;
      return FPE_IO;
   }

#ifndef __BCM2707A0__
   /* Flush denormals.  */
   if ((aa & (0xFF << 23)) == 0)
      aa &= (1 << 31);
   if ((bb & (0xFF << 23)) == 0)
      bb &= (1 << 31);
#endif

   if (a & (1 << 31))
      a ^= ~(1 << 31);
   if (b & (1 << 31))
      b ^= ~(1 << 31);

   if (((int) a) > ((int) b))
      *r = bb;
   else
      *r = aa;

   return 0;
#endif /* ! defined (NATIVE_FLOATS) */
}

extern int glsl_fpu_max (unsigned *r, unsigned a, unsigned b)
{
#ifdef NATIVE_FLOATS
   float fa = *(float *)&a;
   float fb = *(float *)&b;

   if (fa < fb)
      *(float *)r = fb;
   else
      *(float *)r = fa;

   return 0;
#else
   unsigned aa = a;
   unsigned bb = b;

   if (IS_NAN (a) || IS_NAN (b)) {
      *r = (0xFF << 23) + 1;
      return FPE_IO;
   }

#ifndef __BCM2707A0__
   /* Flush denormals.  */
   if ((aa & (0xFF << 23)) == 0)
      aa &= (1 << 31);
   if ((bb & (0xFF << 23)) == 0)
      bb &= (1 << 31);
#endif

   if (a & (1 << 31))
      a ^= ~(1 << 31);
   if (b & (1 << 31))
      b ^= ~(1 << 31);

   if (((int) a) < ((int) b))
      *r = bb;
   else
      *r = aa;

   return 0;
#endif /* ! defined (NATIVE_FLOATS) */
}

extern void glsl_fpu_maxabs (unsigned *r, unsigned a, unsigned b)
{
   a &= ~(1 << 31);
   b &= ~(1 << 31);

   glsl_fpu_max (r, a, b);
}

extern int glsl_fpu_ceil (unsigned *r, unsigned b)
{
#ifdef NATIVE_FLOATS
   float fb = *(float *)&b;

   *(float *)r = ceil (fb);
   return 0;
#else
   int s;
   int e;
   uint64 m;

   int fractional_bits;

   float_unpack (&m, &e, &s, b);

   if (e == INT_MIN) {
      *r = (s << 31);
      return 0;
   } else if (e == INT_MAX) {
      if (m == 0) {
         *r = b;
#ifdef __BCM2707A0__
         return FPE_OF;
#else
   return 0;
#endif
      } else {
         *r = (0xFF << 23) + 1;
         return FPE_IO;
      }
   } else if (e >= 0) {
      *r = b;
      return 0;
   }

   fractional_bits = -e;

   if (fractional_bits >= 24) {
      /* All bits are fractional.  */
      if (s)
         *r = (1 << 31);        /* -0.0.  */
      else
         *r = (127 << 23);      /* 1.0.  */
      return 0;
   }

   if (m & ((1 << fractional_bits) - 1)) {
      m &= ~ ((1 << fractional_bits) - 1);

      if (! s) {
         m += (uint64)(1 << fractional_bits);
      }
   } else {
      /* Already an integer.  */
      *r = b;
      return 0;
   }

   return float_pack (r, m, e, s, 0);
#endif /* ! defined (NATIVE_FLOATS) */
}

extern int glsl_fpu_floor (unsigned *r, unsigned b)
{
#ifdef NATIVE_FLOATS
   float fb = *(float *)&b;

   *(float *)r = floor (fb);
   return 0;
#else
   int s;
   int e;
   uint64 m;

   int fractional_bits;

   float_unpack (&m, &e, &s, b);

   if (e == INT_MIN) {
      *r = (s << 31);
      return 0;
   } else if (e == INT_MAX) {
      if (m == 0) {
         *r = b;
#ifdef __BCM2707A0__
         return FPE_OF;
#else
   return 0;
#endif
      } else {
         *r = (0xFF << 23) + 1;
         return FPE_IO;
      }
   } else if (e >= 0) {
      *r = b;
      return 0;
   }

   fractional_bits = -e;

   if (fractional_bits >= 24) {
      /* All bits are fractional.  */
      if (s)
         *r = (1 << 31) | (127 << 23); /* -1.0.  */
      else
         *r = 0;
      return 0;
   }

   if (m & ((1 << fractional_bits) - 1)) {
      m &= ~ ((1 << fractional_bits) - 1);

      if (s) {
         m += (uint64)(1 << fractional_bits);
      }
   } else {
      /* Already an integer.  */
      *r = b;
      return 0;
   }

   return float_pack (r, m, e, s, 0);
#endif /* ! defined (NATIVE_FLOATS) */
}

extern int glsl_fpu_log2 (unsigned *r, unsigned b)
{
#ifdef NATIVE_FLOATS
   float fb = *(float *)&b;

   *(float *)r = log (fb / log (2.0));
   return 0;
#else
   if ((b & ~(1 << 31)) > (0xFF << 23)
#ifndef __BCM2707A0__
         || ((b & (1 << 31)) && (b & (0xFF << 23))) /* Negative and non-zero?  */
#endif
      ) {
      *r = (0xFF << 23) + 1;
      return FPE_IO;
   } else {
      return fn_log2 (r, b);
   }
#endif /* ! defined (NATIVE_FLOATS) */
}

extern int glsl_fpu_exp2 (unsigned *r, unsigned b)
{
#ifdef NATIVE_FLOATS
   float fb = *(float *)&b;

   *(float *)r = exp (fb * log (2.0));
   return 0;
#else
   if (IS_NAN (b)) {
      *r = (0xFF << 23) + 1;
      return FPE_IO;
   } else {
      return fn_exp2 (r, b);
   }
#endif /* ! defined (NATIVE_FLOATS) */
}
#endif
static int float_to_int (unsigned *r, unsigned a, unsigned b, int round_to_nearest)
{
#ifdef NATIVE_FLOATS
   float fa = *(float *)&a;

   if (round_to_nearest) {
      if (fa > 0)
         fa += 0.5f;
      else
         fa -= 0.5f;
   }

   *r = (int) (fa * pow (2.0f, (float) (int) b));
   return 0;
#else
   int s = !! (a & (1 << 31));
   int e = ((a >> 23) & 0xFF) - 127;
   int m = (1 << 23) | (a & ((1 << 23) - 1));
   int shift;
   int sb;

#ifndef __BCM2707A0__
   if (IS_NAN(a)) {
      /* Turn into 0 or INT_MIN.  */
      *r = (a & (1 << 31));
      return FPE_IO;
   }
#endif

   sb = b;
   if (sb & (1 << 5)) {
      sb |= ~((1 << 6) - 1);
   } else {
      sb &= ((1 << 6) - 1);
   }

   shift = e - 23 + sb;

   if (
#ifdef __BCM2707A0__
      1
#else
   round_to_nearest
#endif
   ) {
      if (shift > (30 - 23)) {
         /* Saturate at INT_MAX or INT_MIN.  */
         if (s) {
            *r = INT_MIN;
         } else {
            *r = INT_MAX;
         }
#ifndef __BCM2707A0__
         return FPE_OF;
#else
   return 0;
#endif
      }
   }

   if (shift > 31) {
      if (e == 128)
         *r = (s ? INT_MIN : INT_MAX);
      else
         *r = 0;
      return FPE_OF;
   } else if (shift > 0) {
      m <<= shift;
   } else if (shift < -31) {
      *r = 0;
#ifdef __BCM2707A0__
      return 0;
#else
   return (e == -127 ? 0 : FPE_UF);
#endif
   } else if (shift < 0) {
      if (round_to_nearest && 0 <= -shift-1 && -shift-1 <= 23) {
         /* Round to nearest (biased).  */
         m += (1 << (-shift-1));
      }

      if (-shift <= 31) {
         m >>= -shift;
      } else {
         m = 0;
      }
   }

   if (s) {
      m = -m;
   }

   *r = m;

#ifndef __BCM2707A0__
   if (s && m == INT_MIN)
      return 0;                 /* A natural INT_MIN.  */

   if (shift > (30 - 23))
      return FPE_OF;

   if (m == 0 && e != -127)
      return FPE_UF;
#endif
   return 0;
#endif  /* ! defined (NATIVE_FLOATS) */
}
#if defined(WIN32) || defined(__CC_ARM)
extern int glsl_fpu_floattoint (unsigned *r, unsigned a, unsigned b)
{
   return float_to_int (r, a, b, 0);
}

extern void glsl_fpu_floattointz (unsigned *r, unsigned a, unsigned b)
{
   float_to_int (r, a, b, 0);
}
#endif
extern void glsl_fpu_floattointn (unsigned *r, unsigned a, unsigned b)
{
   float_to_int (r, a, b, 1);
}
#if defined(WIN32) || defined(__CC_ARM)
static int float_to_unsigned (unsigned *r, unsigned a, unsigned b, int round_to_nearest)
{
#ifdef NATIVE_FLOATS
   float fa = *(float *)&a;

   if (round_to_nearest) {
      if (fa > 0)
         fa += 0.5f;
      else
         fa -= 0.5f;
   }

   *r = (unsigned) (fa * pow (2.0f, (float) (int) b));
   return 0;
#else
   int s = (a & (1 << 31));
   int e = ((a >> 23) & 0xFF) - 127;
   int m = (1 << 23) | (a & ((1 << 23) - 1));
   int shift;
   int sb;

#ifndef __BCM2707A0__
   if (IS_NAN(a)) {
      *r = 0;
      return FPE_IO;
   }
#endif

   sb = b;
   if (sb & (1 << 5)) {
      sb |= ~((1 << 6) - 1);
   } else {
      sb &= ((1 << 6) - 1);
   }

   shift = e - 23 + sb;

   if (round_to_nearest
#ifdef __BCM2707A0__
         || 1                     /* Everything saturates on A0.  */
#endif
      ) {

      /* All negative numbers saturate at 0.  */
      if (((int) a) <= 0) {
         *r = 0;
         return 0;
      }

      if (shift > (31 - 23)) {
         /* Saturate at UINT_MAX.  */
         *r = UINT_MAX;
         return 0;
      }
   }

   if (e == 128) {
      if (s) {
         *r = 0;
         return FPE_IO;
      } else {
         *r = ~0;
         return FPE_OF;
      }
   }

   if (shift > 31) {
#ifdef __BCM2707A0__
      m = 0;
#else
   *r = 0;
   return (s ? FPE_IO : FPE_OF);
#endif
   } else if (shift > 0) {
      m <<= shift;
   } else if (shift < 0) {
      if (round_to_nearest && 0 <= -shift-1 && -shift-1 <= 23) {
         /* Round to nearest (biased).  */
         m += 1 << (-shift-1);
#ifndef __BCM2707A0__
      } else if (s && ! round_to_nearest) {
         /* Round to -infinity.  */
         if (-shift < 31)
            m += (1 << -shift) - 1;
         else {
            *r = (e == -127 ? 0 : ~0);
            return FPE_IO;
         }
#endif
      }

      if (-shift <= 31) {
         m >>= -shift;
      } else {
         m = 0;
      }
   }

   if (s)
      m = -m;

   *r = m;
#ifdef __BCM2707A0__
   return 0;
#else
   if (s)
      return FPE_IO;
   else if (shift > (31 - 23))
      return FPE_OF;
   else
      return 0;
#endif
#endif  /* ! defined (NATIVE_FLOATS) */
}

extern int glsl_fpu_floattouint (unsigned *r, unsigned a, unsigned b)
{
   return float_to_unsigned (r, a, b, 0);
}

extern int glsl_fpu_floattouintq (unsigned *r, unsigned a, unsigned b)
{
   /* No exceptions.  */
   float_to_unsigned (r, a, b, 0);
   return 0;
}

extern void glsl_fpu_floattouintz (unsigned *r, unsigned a, unsigned b)
{
   float_to_unsigned (r, a, b, 0);
}

extern void glsl_fpu_floattouintn (unsigned *r, unsigned a, unsigned b)
{
   float_to_unsigned (r, a, b, 1);
}

extern int glsl_fpu_inttofloat (unsigned *r, unsigned a, unsigned b)
{
#ifdef NATIVE_FLOATS
   *(float *)r = ((int) a) / pow (2.0f, (double) (int) b);
   return 0;
#else
   int sb = b;
   int s;

   if (sb & (1 << 5)) {
      sb |= ~((1 << 6) - 1);
   } else {
      sb &= ((1 << 6) - 1);
   }

   if (((int) a) < 0) {
      a = (unsigned)-(int)a;
      s = 1;
   } else {
      s = 0;
   }

   float_pack (r, a, -sb, s, 0);
   return 0;
#endif /* ! defined (NATIVE_FLOATS) */
}

extern int glsl_fpu_uinttofloat (unsigned *r, unsigned a, unsigned b)
{
#ifdef NATIVE_FLOATS
   *(float *)r = ((unsigned int) a) / pow (2.0f, (double) (int) b);
   return 0;
#else
   int sb = b;

   if (sb & (1 << 5)) {
      sb |= ~((1 << 6) - 1);
   } else {
      sb &= ((1 << 6) - 1);
   }

   float_pack (r, a, -sb, 0, 0);
   return 0;
#endif /* ! defined (NATIVE_FLOATS) */
}

/* LUT-based stuff.  A more-or-less straight port from Gary's verilog.  */

static unsigned square[64] = {
   0x00, 0x04, 0x08, 0x0b, 0x0f, 0x12, 0x16, 0x19, 0x1c, 0x1f, 0x22, 0x24, 0x27, 0x29, 0x2c, 0x2e,
   0x30, 0x32, 0x34, 0x35, 0x37, 0x38, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3e, 0x3f, 0x3f, 0x3f, 0x3f,
   0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3e, 0x3e, 0x3d, 0x3c, 0x3b, 0x3a, 0x38, 0x37, 0x35, 0x34, 0x32,
   0x30, 0x2e, 0x2c, 0x29, 0x27, 0x24, 0x22, 0x1f, 0x1c, 0x19, 0x16, 0x12, 0x0f, 0x0b, 0x08, 0x04,
};

static unsigned c0_recip[16] = {
   0x7ffc, 0x70ee, 0x638c, 0x5792, 0x4ccb, 0x430b, 0x3a2e, 0x3215, 0x2aaa, 0x23d6, 0x1d8a, 0x17b4, 0x1249, 0x0d3e, 0x0889, 0x0421,
};

static unsigned c1_recip[16] = {
   0xf0b, 0xd60, 0xbf8, 0xac5, 0x9bf, 0x8dc, 0x818, 0x76a, 0x6d3, 0x64c, 0x5d6, 0x56b, 0x50b, 0x4b5, 0x468, 0x421,
};

static unsigned c2_recip[16] = {
   0x1d, 0x18, 0x15, 0x12, 0x0f, 0x0d, 0x0c, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x06, 0x05, 0x05, 0x04,
};

static unsigned c0_rsqrt[16] = {
   0x7ffb, 0x7158, 0x64f6, 0x5a4f, 0x5104, 0x48d2, 0x4184, 0x3af4, 0x3501, 0x2aa8, 0x21e7, 0x1a5f, 0x13cc, 0x0e00, 0x08d5, 0x0433,
};

static unsigned c1_rsqrt[16] = {
   0xe9e, 0xc5f, 0xaa5, 0x949, 0x831, 0x74d, 0x68f, 0x5ef, 0xa56, 0x8bf, 0x787, 0x692, 0x5cb, 0x52a, 0x4a2, 0x433,
};

static unsigned c2_rsqrt[16] = {
   0x29, 0x1f, 0x18, 0x13, 0x10, 0x0d, 0x0b, 0x09, 0x1d, 0x16, 0x11, 0x0e, 0x0b, 0x09, 0x08, 0x07,
};

static unsigned c0_log2[16] = {
   0x7fff, 0x74cd, 0x6a3f, 0x6043, 0x56cb, 0x4dc9, 0x4531, 0x3cfc, 0x3520, 0x2d96, 0x2658, 0x1f60, 0x18a9, 0x122e, 0x0beb, 0x05dd,
};

static unsigned c1_log2[16] = {
   0xb31, 0xa8d, 0x9fb, 0x978, 0x902, 0x898, 0x835, 0x7dc, 0x78a, 0x73e, 0x6f8, 0x6b7, 0x67b, 0x643, 0x60e, 0x5dd,
};

static unsigned c2_log2[16] = {
   0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x06, 0x05, 0x05, 0x05, 0x04, 0x04, 0x04, 0x03, 0x03, 0x03,
};

static unsigned c0_exp2[16] = {
   0x0000, 0x05ab, 0x0b96, 0x11c4, 0x1838, 0x1ef5, 0x25ff, 0x2d58, 0x3505, 0x3d09, 0x4567, 0x4e25, 0x5745, 0x60cd, 0x6ac1, 0x7524,
};

static unsigned c1_exp2[16] = {
   0x5ab, 0x5eb, 0x62e, 0x674, 0x6bd, 0x70a, 0x759, 0x7ad, 0x804, 0x85e, 0x8be, 0x920, 0x988, 0x9f4, 0xa64, 0xadc,
};

static unsigned c2_exp2[16] = {
   0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x08,
};

static unsigned fn_interp (unsigned c0_table[], unsigned c1_table[], unsigned c2_table[], unsigned m)
{
   unsigned in_14_11 = (m >> 11) & 0xF;
   unsigned in_10_0  = m & ((1 << 11) - 1);
   unsigned in_10_5  = (m >> 5) & ((1 << 6) - 1);

   unsigned c0 = c0_table[in_14_11];
   unsigned c1 = c1_table[in_14_11];
   unsigned c2 = c2_table[in_14_11];

   unsigned c1x = c1 * in_10_0;
   unsigned xx = square [in_10_5];
   unsigned c2xx = c2*xx;

   unsigned c1x_a = (c1x >> 11) & ((1 << 12) - 1);
   unsigned c2xx_a = (c2xx >> 5) & ((1 << 7) - 1);

   unsigned out_ms;

   if (c0_table == c0_exp2)
      c1x_a = (~ c1x_a) & ((1 << 15) - 1);

   vcos_assert (m < (1 << 16));

   out_ms = c0 - c1x_a - c2xx_a;
   out_ms &= ((1 << 15) - 1);

   return out_ms << 8;
}

static int fn_recip (unsigned *r, unsigned f)
{
   unsigned s = f & (1 << 31);
   unsigned e = (f >> 23) & 0xFF;

   if (e == 0) {
      *r = s | (0xFF << 23);
      return FPE_DZ;
   } else if (e >= 253) {
      *r = s;
#ifdef __BCM2707A0__
      if (e == 0xFF)
         return FPE_UF | FPE_OF;
      else
         return FPE_UF;
#else
      if (e == 0xFF)
         return 0;
      else
         return FPE_UF;
#endif
   }

   e = 0xFF & (~ (e + 0x2));

   /* Result depends on bits 22-8 of the mantissa.  */
   *r = s | (e << 23) | fn_interp (c0_recip, c1_recip, c2_recip, (f >> 8) & ((1 << 15) - 1));
   return 0;
}

static int fn_rsqrt (unsigned *r, unsigned f)
{
   unsigned s = f & (1 << 31);
   unsigned char e = (f >> 23) & 0xFF;
   int e0 = !! (f & (1 << 23));

   /* Catch all negative numbers, except -0.0 and negative denormals.  */
   if (s && (f & ~((1 << 23) - 1)) != (unsigned)(1 << 31)) {
      *r = (0xFF << 23) + 1;
      return FPE_IO;
   }

   if (e == 255) {
      *r = 0;
#ifdef __BCM2707A0__
      return FPE_UF;
#else
      return 0;
#endif
   } else if (e == 0) {
      *r = (0xFF << 23);
      return FPE_DZ;
   }

   vcos_assert (s == 0);

   e = ~e;
   e -= 127;

   /* ASR */
   e >>= 1;
   if (e & (1 << 6))
      e |= (1 << 7);

   e += 126;

   *r = (e << 23) | fn_interp (c0_rsqrt,
                               c1_rsqrt,
                               c2_rsqrt,
                               ((! e0) << 14) | ((f >> 9) & ((1 << 14) - 1)));
   return 0;
}

static int fn_log2 (unsigned *r, unsigned f)
{
   unsigned s = f & (1 << 31);
   unsigned e = (f >> 23) & 0xFF;
   unsigned fraction;
   unsigned tmp_e;
   unsigned denorm;

   if (e == 255) {
      *r = (0xFF << 23);
#ifdef __BCM2707A0__
      return FPE_OF;
#else
      return 0;
#endif
   } else if (e == 0) {
      *r = (1 << 31) | (0xFF << 23);
#ifdef __BCM2707A0__
      return FPE_OF;
#else
      return 0;
#endif
   }

   fraction = ~ fn_interp (c0_log2, c1_log2, c2_log2, (f >> 8) & ((1 << 15) - 1));;
   fraction &= ((1 << 23) - 1);

   tmp_e = e + 1;
   tmp_e &= ((1 << 7) - 1);

   //denorm = {tmp_exp, fraction[22:8], 1'b0};
   denorm = (tmp_e << 16) | (fraction >> 7);
   denorm &= ~1;

   if (e >= 127) {
      s = 0;
   } else {
      s = (1 << 31);
      denorm = ~ denorm;
      denorm &= ((1 << 23) - 1);
   }

   if (denorm == 0) {
      *r = 0;
#ifdef __BCM2707A0__
      return 0;
#else
      /* We have underflowed iff the input was not _exactly_ +1.0 or
      -1.0.  */
      if ((f & ~(1 << 31)) != 0x3f800000)
         return FPE_UF;
      else
         return 0;
#endif
   }

   vcos_assert (denorm < (1 << 23));

   e = 133;
   while (! (denorm & (1 << 22))) {
      e--;
      denorm <<= 1;
#if 0 /* This was dropped in the hardware to reuse the existing
      shifter.  Ditto for the shift immediately after the loop.
      Gary's verilog labels it as non-essential anyway.  */
      denorm |= !! (denorm & 2);
#endif
   }

      /* Loose hidden bit.  */
      denorm <<= 1;
#if 0
      denorm |= !! (denorm & 2);
#endif
      denorm &= ~(1 << 23);

      *r = s | (e << 23) | denorm;
      return 0;
   }

      static int fn_exp2 (unsigned *r, unsigned f)
      {
      unsigned s;
      unsigned e;
      unsigned tmpshift;
      unsigned overflow;
      unsigned tmpden;
      unsigned denorm;

      /* All denormals are silently changed to +0.0 on input.  This is so
      that e.g. +-0.0 will return the same value.  */
      if (((f >> 23) & 0xFF) == 0)
      f = 0;

#ifndef __BCM2707A0__
      /* Infinity?  */
      if ((f & (0xFF << 23)) == (0xFF << 23)) {
      if (f & (1 << 31))
      *r = 0;
      else
      *r = (0xFF << 23);
      return 0;
   }
#endif

      s = f & (1 << 31);
      e = (f >> 23) & 0xFF;

      if (e & (1 << 7)) {
      tmpshift = (~e) & ((1 << 3) - 1);
   } else {
      tmpshift = (~e) & ((1 << 4) - 1);
   }
      tmpden = (1 << 23) | (f & ((1 << 23) - 1));
      tmpden >>= tmpshift;

      overflow = 0;
      if (e & (1 << 7)) {
      denorm = tmpden & ((1 << 22) - 1);
      if (((e >> 3) & ((1 << 4) - 1))
      || ((tmpden >> 22) & 0x3)) {
      overflow = 1;
   }
   } else {
      if ((e & 0x70) == 0x70) {
      denorm = tmpden >> 8;
      denorm &= ((1 << 16) - 1);
   } else {
      denorm = 0;
   }
   }

      if (s) {
      denorm = (~denorm) & ((1 << 22) - 1);
   }

      f = fn_interp (c0_exp2, c1_exp2, c2_exp2, denorm & ((1 << 15) - 1));

      if (s) {
      unsigned d;
      d = (denorm >> 15);
      d &= ((1 << 7) - 1);

      if (overflow || d < 2) {
      *r = 0;
      return FPE_UF;
   } else {
      f |= (d - 1) << 23;
   }
   } else {
      if (overflow) {
      *r = (0xFF << 23);
      return FPE_OF;
   } else {
      unsigned d;
      d = (denorm >> 15);
      d &= ((1 << 7) - 1);

      d += 127;
      d &= ((1 << 8) - 1);

      f |= (d << 23);
   }
   }

      *r = f;
      return 0;
   }
#endif