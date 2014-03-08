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
#include "middleware/khronos/vg/vg_image.h"
#include "middleware/khronos/vg/vg_stem.h"
#include "interface/khronos/vg/vg_int_util.h"
#include "middleware/khronos/common/khrn_interlock.h"

/******************************************************************************
helpers
******************************************************************************/

static KHRN_IMAGE_FORMAT_T get_internal_format(VGImageFormat format, uint32_t width, uint32_t height)
{
   KHRN_IMAGE_FORMAT_T internal_format;
   switch (format) {
   case VG_sRGBX_8888:
   case VG_sXRGB_8888:
   case VG_sBGRX_8888:
   case VG_sXBGR_8888:
      internal_format = XBGR_8888_TF;
      break;
   case VG_sRGBA_8888:
   case VG_sARGB_8888:
   case VG_sBGRA_8888:
   case VG_sABGR_8888:
      internal_format = ABGR_8888_TF;
      break;
   case VG_sRGBA_8888_PRE:
   case VG_sARGB_8888_PRE:
   case VG_sBGRA_8888_PRE:
   case VG_sABGR_8888_PRE:
      internal_format = (KHRN_IMAGE_FORMAT_T)(ABGR_8888_TF | IMAGE_FORMAT_PRE);
      break;
   case VG_sRGB_565:
   case VG_sBGR_565:
      internal_format = RGB_565_TF;
      break;
   case VG_sRGBA_5551:
   case VG_sARGB_1555:
   case VG_sBGRA_5551:
   case VG_sABGR_1555:
      internal_format = RGBA_5551_TF;
      break;
   case VG_sRGBA_4444:
   case VG_sARGB_4444:
   case VG_sBGRA_4444:
   case VG_sABGR_4444:
      internal_format = RGBA_4444_TF;
      break;
   case VG_sL_8:
      internal_format = L_8_TF;
      break;
   case VG_lRGBX_8888:
   case VG_lXRGB_8888:
   case VG_lBGRX_8888:
   case VG_lXBGR_8888:
      internal_format = (KHRN_IMAGE_FORMAT_T)(XBGR_8888_TF | IMAGE_FORMAT_LIN);
      break;
   case VG_lRGBA_8888:
   case VG_lARGB_8888:
   case VG_lBGRA_8888:
   case VG_lABGR_8888:
      internal_format = (KHRN_IMAGE_FORMAT_T)(ABGR_8888_TF | IMAGE_FORMAT_LIN);
      break;
   case VG_lRGBA_8888_PRE:
   case VG_lARGB_8888_PRE:
   case VG_lBGRA_8888_PRE:
   case VG_lABGR_8888_PRE:
      internal_format = (KHRN_IMAGE_FORMAT_T)(ABGR_8888_TF | IMAGE_FORMAT_LIN | IMAGE_FORMAT_PRE);
      break;
   case VG_lL_8:
      internal_format = (KHRN_IMAGE_FORMAT_T)(L_8_TF | IMAGE_FORMAT_LIN);
      break;
   case VG_A_8:
      internal_format = (KHRN_IMAGE_FORMAT_T)(A_8_TF | IMAGE_FORMAT_LIN);
      break;
   case VG_BW_1:
      internal_format = (KHRN_IMAGE_FORMAT_T)(L_1_TF | IMAGE_FORMAT_LIN);
      break;
   case VG_A_4:
      internal_format = (KHRN_IMAGE_FORMAT_T)(A_4_TF | IMAGE_FORMAT_LIN);
      break;
   case VG_A_1:
      internal_format = (KHRN_IMAGE_FORMAT_T)(A_1_TF | IMAGE_FORMAT_LIN);
      break;
   default:
      UNREACHABLE(); /* image format check performed on client-side */
      return (KHRN_IMAGE_FORMAT_T)0;
   }
   if (khrn_image_prefer_lt(internal_format, width, height)) {
      internal_format = khrn_image_to_lt_format(internal_format);
   }
   return internal_format;
}

static VGImageFormat get_external_format(KHRN_IMAGE_FORMAT_T format)
{
   if (!khrn_image_is_tformat(format) && !khrn_image_is_lineartile(format)) {
      return (VGImageFormat)-1;
   }
   switch (format & ~IMAGE_FORMAT_MEM_LAYOUT_MASK) {
   case XBGR_8888:                                         return VG_sXBGR_8888;
   case ABGR_8888:                                         return VG_sABGR_8888;
   case (ABGR_8888 | IMAGE_FORMAT_PRE):                    return VG_sABGR_8888_PRE;
   case RGB_565:                                           return VG_sRGB_565;
   case RGBA_5551:                                         return VG_sRGBA_5551;
   case RGBA_4444:                                         return VG_sRGBA_4444;
   case L_8:                                               return VG_sL_8;
   case (XBGR_8888 | IMAGE_FORMAT_LIN):                    return VG_lXBGR_8888;
   case (ABGR_8888 | IMAGE_FORMAT_LIN):                    return VG_lABGR_8888;
   case (ABGR_8888 | IMAGE_FORMAT_LIN | IMAGE_FORMAT_PRE): return VG_lABGR_8888_PRE;
   case (L_8 | IMAGE_FORMAT_LIN):                          return VG_lL_8;
   case (A_8 | IMAGE_FORMAT_LIN):                          return VG_A_8;
   case (L_1 | IMAGE_FORMAT_LIN):                          return VG_BW_1;
   case (A_4 | IMAGE_FORMAT_LIN):                          return VG_A_4;
   case (A_1 | IMAGE_FORMAT_LIN):                          return VG_A_1;
   /* A_8 isn't exactly equivalent to VG_A_8 (VG_A_8 is "linear"), but we don't
    * have a better match (and we'd rather a slightly imperfect match than no
    * match at all). todo: can we add an extension for non-linear alpha types? */
   case A_8:                                               return VG_A_8;
   default:                                                return (VGImageFormat)-1;
   }
}

/******************************************************************************
image
******************************************************************************/

vcos_static_assert(sizeof(VG_IMAGE_BPRINT_T) <= VG_STEM_SIZE);
vcos_static_assert(alignof(VG_IMAGE_BPRINT_T) <= VG_STEM_ALIGN);

void vg_image_bprint_term(void *p, uint32_t size)
{
   UNUSED(p);
   UNUSED(size);
}

/*
   void vg_image_bprint_from_stem(MEM_HANDLE_T handle, VGImageFormat format,
      uint32_t allowed_quality, int32_t width, int32_t height)

   Converts a stem object into an object of type VG_IMAGE_BPRINT_T with the
   specified properties. Always succeeds

   Preconditions:

   handle is a valid handle to a stem object
   format is a valid VGImageFormat
   allowed_quality is a (non-empty) subset of {VG_IMAGE_QUALITY_NONANTIALIASED,
      VG_IMAGE_QUALITY_FASTER, VG_IMAGE_QUALITY_BETTER}
   width is in (0, VG_CONFIG_MAX_IMAGE_WIDTH]
   height is in (0, VG_CONFIG_MAX_IMAGE_HEIGHT]
   width * height is <= VG_CONFIG_MAX_IMAGE_PIXELS

   Postconditions:

   handle is now a valid handle to an object of type VG_IMAGE_BPRINT_T
*/

void vg_image_bprint_from_stem(
   MEM_HANDLE_T handle,
   VGImageFormat format,
   uint32_t allowed_quality,
   int32_t width, int32_t height)
{
   VG_IMAGE_BPRINT_T *image_bprint;

   vcos_assert(vg_is_stem(handle));
   vcos_assert(is_allowed_quality(allowed_quality) &&
      (width > 0) && (height > 0) &&
      (width <= VG_CONFIG_MAX_IMAGE_WIDTH) && (height <= VG_CONFIG_MAX_IMAGE_HEIGHT) &&
      ((width * height) <= VG_CONFIG_MAX_IMAGE_PIXELS)); /* checks performed on client-side */

   image_bprint = (VG_IMAGE_BPRINT_T *)mem_lock(handle);
   image_bprint->format = format;
   image_bprint->allowed_quality = allowed_quality;
   image_bprint->width = width;
   image_bprint->height = height;
   mem_unlock(handle);

   mem_set_desc(handle, "VG_IMAGE_BPRINT_T");
   mem_set_term(handle, vg_image_bprint_term);
}

vcos_static_assert(sizeof(VG_IMAGE_T) <= VG_STEM_SIZE);
vcos_static_assert(alignof(VG_IMAGE_T) <= VG_STEM_ALIGN);

void vg_image_term(void *p, uint32_t size)
{
   VG_IMAGE_T *image = (VG_IMAGE_T *)p;

   UNUSED(size);

   mem_release(image->image);
}

/*
   bool vg_image_from_bprint(MEM_HANDLE_T handle)

   Converts an object of type VG_IMAGE_BPRINT_T to an object of type VG_IMAGE_T.
   A return value of false indicates failure

   Preconditions:

   handle is a valid handle to an object of type VG_IMAGE_BPRINT_T

   Postconditions:

   If the function succeeds:
   - true is returned
   - handle is now a valid handle to an object of type VG_IMAGE_T
   otherwise:
   - false is returned
   - handle is unchanged (ie it is still a valid handle to an object of type
     VG_IMAGE_BPRINT_T)
*/

bool vg_image_from_bprint(MEM_HANDLE_T handle)
{
   VG_IMAGE_BPRINT_T *image_bprint;
   VGImageFormat format;
   uint32_t allowed_quality;
   int32_t width, height;
   KHRN_IMAGE_FORMAT_T internal_format;
   MEM_HANDLE_T image_handle;
   VG_IMAGE_T *image;

   vcos_assert(vg_is_image_bprint(handle));

   /*
      get init params from blueprint
   */

   image_bprint = (VG_IMAGE_BPRINT_T *)mem_lock(handle);
   format = image_bprint->format;
   allowed_quality = image_bprint->allowed_quality;
   width = image_bprint->width;
   height = image_bprint->height;
   mem_unlock(handle);

   /*
      alloc data
   */

   internal_format = get_internal_format(format, width, height);
   image_handle = khrn_image_create(internal_format, width, height,
      (KHRN_IMAGE_CREATE_FLAG_T)(IMAGE_CREATE_FLAG_ZERO | IMAGE_CREATE_FLAG_TEXTURE));
   if (image_handle == MEM_INVALID_HANDLE) {
      return false;
   }

   /*
      fill in struct
   */

   image = (VG_IMAGE_T *)mem_lock(handle);
   image->allowed_quality = allowed_quality;
   image->image = image_handle;
   image->image_width = width;
   image->image_height = height;
   image->image_format = internal_format;
   mem_unlock(handle);

   mem_set_desc(handle, "VG_IMAGE_T");
   mem_set_term(handle, vg_image_term);

   return true;
}

VGImageFormat vg_image_get_external_format(KHRN_IMAGE_FORMAT_T format)
{
   return get_external_format(format);
}

MEM_HANDLE_T vg_image_alloc_from_image(MEM_HANDLE_T src_handle)
{
   VG_IMAGE_T *image;
   KHRN_IMAGE_T *src;

   /*
      alloc struct
   */

   MEM_HANDLE_T handle = MEM_ALLOC_STRUCT_EX(VG_IMAGE_T, MEM_COMPACT_DISCARD);
   if (handle == MEM_INVALID_HANDLE) {
      return MEM_INVALID_HANDLE;
   }

   /*
      fill in struct
   */

   image = (VG_IMAGE_T *)mem_lock(handle);
   src = (KHRN_IMAGE_T *)mem_lock(src_handle);
   image->allowed_quality = VG_IMAGE_QUALITY_NONANTIALIASED | VG_IMAGE_QUALITY_FASTER | VG_IMAGE_QUALITY_BETTER; /* allow all */
   mem_acquire(src_handle);
   image->image = src_handle;
   image->image_width = src->width;
   image->image_height = src->height;
   image->image_format = src->format;
   mem_unlock(src_handle);
   mem_unlock(handle);

   mem_set_term(handle, vg_image_term);

   return handle;
}

void vg_image_from_stem_and_image(MEM_HANDLE_T handle, MEM_HANDLE_T src_handle)
{
   VG_IMAGE_T *image;
   KHRN_IMAGE_T *src;

   vcos_assert(vg_is_stem(handle));

   image = (VG_IMAGE_T *)mem_lock(handle);
   src = (KHRN_IMAGE_T *)mem_lock(src_handle);
   image->allowed_quality = VG_IMAGE_QUALITY_NONANTIALIASED | VG_IMAGE_QUALITY_FASTER | VG_IMAGE_QUALITY_BETTER; /* allow all */
   mem_acquire(src_handle);
   image->image = src_handle;
   image->image_width = src->width;
   image->image_height = src->height;
   image->image_format = src->format;
   mem_unlock(src_handle);
   mem_unlock(handle);

   mem_set_desc(handle, "VG_IMAGE_T");
   mem_set_term(handle, vg_image_term);
}

VG_IMAGE_T *vg_image_lock_adam(
   uint32_t *x_out, uint32_t *y_out, uint32_t *width_out, uint32_t *height_out,
   MEM_HANDLE_T *handle_io)
{
   uint32_t x = 0, y = 0, width = (uint32_t)-1, height = 0;
   MEM_HANDLE_T handle = *handle_io;
   VG_IMAGE_T *image;

   while (!vg_is_image(handle)) {
      VG_CHILD_IMAGE_T *child_image;
      MEM_HANDLE_T parent_handle;
      vcos_assert(vg_is_child_image(handle));
      child_image = (VG_CHILD_IMAGE_T *)mem_lock(handle);
      x += child_image->x;
      y += child_image->y;
      if (width == -1) {
         width = child_image->width;
         height = child_image->height;
         vcos_assert(width != -1);
      }
      parent_handle = child_image->parent;
      mem_unlock(handle);
      handle = parent_handle;
   }
   image = (VG_IMAGE_T *)mem_lock(handle);
   if (width == -1) {
      width = image->image_width;
      height = image->image_height;
   }

   *x_out = x;
   *y_out = y;
   if (width_out) { *width_out = width; }
   if (height_out) { *height_out = height; }
   *handle_io = handle;
   return image;
}

bool vg_image_leak(VG_IMAGE_T *image)
{
   if (khrn_image_is_ok_for_render_target(image->image_format, true)) {
      KHRN_IMAGE_T *image_image = (KHRN_IMAGE_T *)mem_lock(image->image);
      if (!khrn_image_can_use_as_render_target(image_image)) {
         MEM_HANDLE_T handle;
         khrn_interlock_read_immediate(&image_image->interlock); /* just need to make sure there are no outstanding writes */
         handle = khrn_image_create_dup(image_image, IMAGE_CREATE_FLAG_RENDER_TARGET);
         mem_unlock(image->image);
         if (handle == MEM_INVALID_HANDLE) {
            return false;
         }
         mem_release(image->image);
         image->image = handle;
         image->image_format = ((KHRN_IMAGE_T *)mem_lock(handle))->format;
         mem_unlock(handle);
      } else {
         mem_unlock(image->image);
      }
   }
   return true;
}

/******************************************************************************
child image
******************************************************************************/

vcos_static_assert(sizeof(VG_CHILD_IMAGE_T) <= VG_STEM_SIZE);
vcos_static_assert(alignof(VG_CHILD_IMAGE_T) <= VG_STEM_ALIGN);

void vg_child_image_bprint_term(void *p, uint32_t size) /* also works for instantiated-type VG_CHILD_IMAGE_Ts */
{
   VG_CHILD_IMAGE_T *child_image = (VG_CHILD_IMAGE_T *)p;
   MEM_HANDLE_T handle, parent_handle;

   UNUSED(size);

   /*
      the following should hopefully be equivalent to
      mem_release(child_image->parent) but without recursion
   */

   handle = child_image->parent;
   while (!mem_try_release(handle)) {
      MEM_TERM_T term = mem_get_term(handle);
      if ((term == vg_image_term) || (term == vg_image_bprint_term)) {
         mem_release(handle);
         break;
      }
      vcos_assert((term == vg_child_image_term) || (term == vg_child_image_bprint_term));
      parent_handle = ((VG_CHILD_IMAGE_T *)mem_lock(handle))->parent;
      mem_unlock(handle);
      mem_set_term(handle, NULL);
      mem_release(handle);
      handle = parent_handle;
   }
}

void vg_child_image_bprint_from_stem(
   MEM_HANDLE_T handle,
   MEM_HANDLE_T parent_handle,
   int32_t x, int32_t y,
   int32_t width, int32_t height)
{
   VG_CHILD_IMAGE_T *child_image_bprint;

   vcos_assert(vg_is_stem(handle));
   vcos_assert((x >= 0) && (y >= 0) && (width > 0) && (height > 0)); /* checks performed on client-side */

   child_image_bprint = (VG_CHILD_IMAGE_T *)mem_lock(handle);
   mem_acquire(parent_handle);
   child_image_bprint->parent = parent_handle;
   child_image_bprint->x = (uint16_t)x;
   child_image_bprint->y = (uint16_t)y;
   child_image_bprint->width = (uint16_t)width;
   child_image_bprint->height = (uint16_t)height;
   mem_unlock(handle);

   mem_set_desc(handle, "VG_CHILD_IMAGE_T (blueprint)");
   mem_set_term(handle, vg_child_image_bprint_term);
}

void vg_child_image_term(void *p, uint32_t size)
{
   vg_child_image_bprint_term(p, size); /* will do the right thing */
}

bool vg_child_image_from_bprint(MEM_HANDLE_T handle)
{
   MEM_HANDLE_T ancestor_handle;

   vcos_assert(vg_is_child_image_bprint(handle));

   /*
      work down through ancestors until we reach an instantiated image, or we
      reach the root

      if the root is a blueprint, try to instantiate it
   */

   ancestor_handle = ((VG_CHILD_IMAGE_T *)mem_lock(handle))->parent;
   mem_unlock(handle);
   for (;;) {
      MEM_TERM_T ancestor_term = mem_get_term(ancestor_handle);
      MEM_HANDLE_T ancestor_parent_handle;
      if ((ancestor_term == vg_image_term) || (ancestor_term == vg_child_image_term)) {
         break; /* reached an instantiated image */
      }
      if (ancestor_term == vg_image_bprint_term) {
         /* reached the root and it isn't instantiated. try to instantiate it */
         if (!vg_image_from_bprint(ancestor_handle)) {
            return false;
         }
         break;
      }
      vcos_assert(ancestor_term == vg_child_image_bprint_term);
      ancestor_parent_handle = ((VG_CHILD_IMAGE_T *)mem_lock(ancestor_handle))->parent;
      mem_unlock(ancestor_handle);
      ancestor_handle = ancestor_parent_handle;
   }

   /*
      the root is now an instantiated image

      mark all child images on the path from us to the root as being
      instantiated (this includes us)
   */

   for (;;) {
      MEM_TERM_T term = mem_get_term(handle);
      MEM_HANDLE_T parent_handle;
      if ((term == vg_image_term) || (term == vg_child_image_term)) {
         break;
      }
      vcos_assert(term == vg_child_image_bprint_term);
      mem_set_desc(handle, "VG_CHILD_IMAGE_T (instantiated)");
      mem_set_term(handle, vg_child_image_term);
      parent_handle = ((VG_CHILD_IMAGE_T *)mem_lock(handle))->parent;
      mem_unlock(handle);
      handle = parent_handle;
   }

   return true;
}
