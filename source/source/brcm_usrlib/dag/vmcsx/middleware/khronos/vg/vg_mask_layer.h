/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VG_MASK_LAYER_H
#define VG_MASK_LAYER_H

#include "middleware/khronos/common/khrn_image.h"
#include "middleware/khronos/common/khrn_mem.h"
#include "interface/khronos/include/VG/openvg.h"

typedef struct {
   int32_t width;
   int32_t height;
} VG_MASK_LAYER_BPRINT_T;

extern void vg_mask_layer_bprint_from_stem(
   MEM_HANDLE_T handle,
   int32_t width, int32_t height);

extern void vg_mask_layer_bprint_term(void *, uint32_t);

static INLINE bool vg_is_mask_layer_bprint(MEM_HANDLE_T handle)
{
   return mem_get_term(handle) == vg_mask_layer_bprint_term;
}

static INLINE KHRN_IMAGE_FORMAT_T vg_mask_layer_get_format(
   uint32_t width, uint32_t height)
{
   return khrn_image_prefer_lt(A_8_TF, width, height) ? A_8_LT : A_8_TF;
}

typedef struct {
   MEM_HANDLE_T image; /* KHRN_IMAGE_T */
   uint32_t image_width;
   uint32_t image_height;
} VG_MASK_LAYER_T;

extern bool vg_mask_layer_from_bprint(MEM_HANDLE_T handle);

extern void vg_mask_layer_term(void *, uint32_t);

static INLINE bool vg_is_mask_layer(MEM_HANDLE_T handle)
{
   return mem_get_term(handle) == vg_mask_layer_term;
}

#endif
