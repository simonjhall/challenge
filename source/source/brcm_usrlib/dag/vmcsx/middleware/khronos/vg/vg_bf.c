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
#include "middleware/khronos/vg/vg_bf.h"
#include "interface/khronos/common/khrn_int_math.h"

uint32_t vg_bf_gblur_kernel_gen(int16_t *kernel, float std_dev)
{
   float tolerated_sum, oo_std_dev, exp_scale;
   uint32_t count;
   float sum, scale;
   uint32_t i;

   if (std_dev < EPS) {
      kernel[0] = 0x7fff;
      return 1;
   }

   /*
      G(x, s) = exp(-x^2 / 2s^2) / (sqrt(2pi) * s)
   */

   tolerated_sum = sqrt_(2.0f * PI) * std_dev * VG_BF_GBLUR_KERNEL_TOLERANCE;
   oo_std_dev = recip_(std_dev);
   exp_scale = -0.5f * oo_std_dev * oo_std_dev;

   count = 1;
   sum = 1.0f;
   do {
      float x;

      /*
         tolerated_sum is based on integral rather than sum of samples (which is
         what sum actually is)
         this works well enough for large std dev, but for very small std dev,
         tolerated_sum is too small
         to workaround this, we always do at least one iteration of the loop
         even if sum < tolerated_sum to begin with
      */

      count += 2;
      x = (float)(count >> 1);
      sum += 2.0f * exp_(x * x * exp_scale); /* times 2 for left and right */
   } while ((count != VG_BF_GBLUR_KERNEL_COUNT_MAX) && (sum < tolerated_sum));

   scale = recip_(sum);

   kernel[count >> 1] = (int16_t)_min(float_to_int_shift(scale, 15), 0x7fff);
   for (i = 0; i != (count >> 1); ++i) {
      float x = (float)((count >> 1) - i);
      kernel[i] = kernel[count - (i + 1)] =
         (int16_t)float_to_int_shift(exp_(x * x * exp_scale) * scale, 15);
   }

   return count;
}
