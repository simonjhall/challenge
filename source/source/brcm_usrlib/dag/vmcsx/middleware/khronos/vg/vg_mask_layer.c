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
#include "interface/khronos/vg/vg_int_config.h"
#include "middleware/khronos/vg/vg_mask_layer.h"
#include "middleware/khronos/vg/vg_stem.h"

vcos_static_assert(sizeof(VG_MASK_LAYER_BPRINT_T) <= VG_STEM_SIZE);
vcos_static_assert(alignof(VG_MASK_LAYER_BPRINT_T) <= VG_STEM_ALIGN);

void vg_mask_layer_bprint_term(void *p, uint32_t size)
{
   UNUSED(p);
   UNUSED(size);
}

void vg_mask_layer_bprint_from_stem(
   MEM_HANDLE_T handle,
   int32_t width, int32_t height)
{
   VG_MASK_LAYER_BPRINT_T *mask_layer_bprint;

   vcos_assert(vg_is_stem(handle));
   vcos_assert((width > 0) && (height > 0) &&
      (width <= VG_CONFIG_MAX_IMAGE_WIDTH) && (height <= VG_CONFIG_MAX_IMAGE_HEIGHT) &&
      ((width * height) <= VG_CONFIG_MAX_IMAGE_PIXELS)); /* checks performed on client-side */

   mask_layer_bprint = (VG_MASK_LAYER_BPRINT_T *)mem_lock(handle);
   mask_layer_bprint->width = width;
   mask_layer_bprint->height = height;
   mem_unlock(handle);

   mem_set_desc(handle, "VG_MASK_LAYER_BPRINT_T");
   mem_set_term(handle, vg_mask_layer_bprint_term);
}

vcos_static_assert(sizeof(VG_MASK_LAYER_T) <= VG_STEM_SIZE);
vcos_static_assert(alignof(VG_MASK_LAYER_T) <= VG_STEM_ALIGN);

void vg_mask_layer_term(void *p, uint32_t size)
{
   VG_MASK_LAYER_T *mask_layer = (VG_MASK_LAYER_T *)p;

   UNUSED(size);

   mem_release(mask_layer->image);
}

bool vg_mask_layer_from_bprint(MEM_HANDLE_T handle)
{
   VG_MASK_LAYER_BPRINT_T *mask_layer_bprint;
   int32_t width, height;
   MEM_HANDLE_T image_handle;
   VG_MASK_LAYER_T *mask_layer;

   vcos_assert(vg_is_mask_layer_bprint(handle));

   /*
      get init params from blueprint
   */

   mask_layer_bprint = (VG_MASK_LAYER_BPRINT_T *)mem_lock(handle);
   width = mask_layer_bprint->width;
   height = mask_layer_bprint->height;
   mem_unlock(handle);

   /*
      alloc data
   */

   image_handle = khrn_image_create(vg_mask_layer_get_format(width, height), width, height,
      (KHRN_IMAGE_CREATE_FLAG_T)(IMAGE_CREATE_FLAG_ONE | IMAGE_CREATE_FLAG_TEXTURE));
   if (image_handle == MEM_INVALID_HANDLE) {
      return false;
   }

   /*
      fill in struct
   */

   mask_layer = (VG_MASK_LAYER_T *)mem_lock(handle);
   mask_layer->image = image_handle;
   mask_layer->image_width = width;
   mask_layer->image_height = height;
   mem_unlock(handle);

   mem_set_desc(handle, "VG_MASK_LAYER_T");
   mem_set_term(handle, vg_mask_layer_term);

   return true;
}
