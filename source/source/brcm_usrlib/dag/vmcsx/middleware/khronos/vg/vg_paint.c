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
#include "middleware/khronos/vg/vg_image.h"
#include "middleware/khronos/vg/vg_paint.h"
#include "middleware/khronos/vg/vg_ramp.h"
#include "middleware/khronos/vg/vg_stem.h"

/* VG_PAINT_BPRINT_T doesn't actually exist (it would be empty) */

void vg_paint_bprint_term(void *p, uint32_t size)
{
   UNUSED(p);
   UNUSED(size);
}

void vg_paint_bprint_from_stem(MEM_HANDLE_T handle)
{
   vcos_assert(vg_is_stem(handle));

   mem_set_desc(handle, "VG_PAINT_BPRINT_T");
   mem_set_term(handle, vg_paint_bprint_term);
}

vcos_static_assert(sizeof(VG_PAINT_T) <= VG_STEM_SIZE);
vcos_static_assert(alignof(VG_PAINT_T) <= VG_STEM_ALIGN);

void vg_paint_term(void *p, uint32_t size)
{
   VG_PAINT_T *paint = (VG_PAINT_T *)p;

   UNUSED(size);

   if (paint->pattern != MEM_INVALID_HANDLE) {
      mem_release(paint->pattern);
   }

   if (paint->ramp != MEM_INVALID_HANDLE) {
      vg_ramp_release(paint->ramp, paint->ramp_i);
   }

   if (paint->ramp_stops != MEM_INVALID_HANDLE) {
      mem_release(paint->ramp_stops);
   }
}

static void init(VG_PAINT_T *paint)
{
   paint->type = VG_PAINT_TYPE_COLOR;

   paint->rgba = 0xff000000;

   paint->linear_gradient[0] = 0.0f;
   paint->linear_gradient[1] = 0.0f;
   paint->linear_gradient[2] = 1.0f;
   paint->linear_gradient[3] = 0.0f;
   paint->radial_gradient[0] = 0.0f;
   paint->radial_gradient[1] = 0.0f;
   paint->radial_gradient[2] = 0.0f;
   paint->radial_gradient[3] = 0.0f;
   paint->radial_gradient[4] = 1.0f;
   vg_paint_set_ramp_spread_mode(paint, VG_COLOR_RAMP_SPREAD_PAD);
   paint->ramp_pre = true;
   paint->ramp_stops = MEM_INVALID_HANDLE;
   paint->ramp = MEM_INVALID_HANDLE;

   paint->pattern_tiling_mode = VG_TILE_FILL;
   paint->pattern = MEM_INVALID_HANDLE;
}

MEM_HANDLE_T vg_paint_alloc(void)
{
   /*
      alloc struct
   */

   MEM_HANDLE_T handle = MEM_ALLOC_STRUCT_EX(VG_PAINT_T, MEM_COMPACT_DISCARD);
   if (handle == MEM_INVALID_HANDLE) {
      return MEM_INVALID_HANDLE;
   }

   /*
      fill in struct
   */

   init((VG_PAINT_T *)mem_lock(handle));
   mem_unlock(handle);

   mem_set_term(handle, vg_paint_term);

   return handle;
}

bool vg_paint_from_bprint(MEM_HANDLE_T handle)
{
   vcos_assert(vg_is_paint_bprint(handle));

   /*
      fill in struct
   */

   init((VG_PAINT_T *)mem_lock(handle));
   mem_unlock(handle);

   mem_set_desc(handle, "VG_PAINT_T");
   mem_set_term(handle, vg_paint_term);

   return true;
}

uint32_t vg_paint_get_ramp_degenerate_rgba(VG_PAINT_T *paint)
{
   /*
      degenerate means function has value 1 everywhere:
      - in pad and reflect spread modes, this maps to the last ramp stop color.
      - in repeat spread mode, this maps to the first ramp stop color.
   */

   return vg_ramp_get_rgba(paint->ramp_stops, vg_paint_get_ramp_spread_mode(paint) != VG_COLOR_RAMP_SPREAD_REPEAT);
}

bool vg_paint_retain_ramp(VG_PAINT_T *paint)
{
   if (paint->ramp == MEM_INVALID_HANDLE) {
      uint32_t ramp_i;
      if (!vg_ramp_alloc(&paint->ramp, &ramp_i)) { /* won't overwrite paint->ramp unless successful */
         return false;
      }
      paint->ramp_i = (uint8_t)ramp_i;
   }
   return vg_ramp_retain(paint->ramp, paint->ramp_i, paint->ramp_pre, paint->ramp_stops);
}
