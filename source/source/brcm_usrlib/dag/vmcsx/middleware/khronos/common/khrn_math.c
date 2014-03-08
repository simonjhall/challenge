/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "interface/khronos/common/khrn_int_common.h"

#include "interface/khronos/common/khrn_int_math.h"
#include "interface/khronos/common/khrn_int_util.h"

#define P1 1.570796e0f
#define P2 2.663139e0f
#define P3 1.349177e0f
#define P4 1.976203e-1f
#define P5 2.105075e-3f

#define Q1 1.000000e0f
#define Q2 2.332027e0f
#define Q3 1.843527e0f
#define Q4 5.578347e-1f
#define Q5 4.944801e-2f

float acos_(float x)
{
   float abs_x = absf_(x);
   float p = P1 + ((P2 + ((P3 + ((P4 + (P5 * abs_x)) * abs_x)) * abs_x)) * abs_x);
   float q = Q1 + ((Q2 + ((Q3 + ((Q4 + (Q5 * abs_x)) * abs_x)) * abs_x)) * abs_x);
   float acos_abs_x = sqrt_(_maxf(1.0f - (x * x), 0.0f)) * p * recip_(q);
   return (x < 0.0f) ? (PI - acos_abs_x) : acos_abs_x;
}

float mod_one_(float x)
{
   uint32_t m = float_to_bits(x) & 0x7fffff;
   int32_t e = ((float_to_bits(x) >> 23) & 0xff) - 127;
   uint32_t sign = float_to_bits(x) & 0x80000000, n;

   if (e < 0) { return x; }                      /* abs less then 1 (includes zeros/denormals) */
   if (e > 22) { return float_from_bits(sign); } /* no precision less than 1 */

   /*
      clear bits of mantissa corresponding to values >= 1
   */

   m &= ((1 << 23) >> e) - 1;

   /*
      normalise
   */

   if (m == 0) { return float_from_bits(sign); }

   n = 23 - _msb(m);
   e -= n;
   m <<= n;
   m &= ~(1 << 23);

   return float_from_bits(sign | ((e + 127) << 23) | m);
}
