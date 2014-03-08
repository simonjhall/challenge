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

#include "interface/khronos/common/khrn_int_color.h"
#include "middleware/khronos/common/khrn_hw.h"
#include "middleware/khronos/common/khrn_image.h"
#include "middleware/khronos/common/khrn_tformat.h"
#include "interface/khronos/common/khrn_int_util.h"
#include "middleware/khronos/common/khrn_bf.h"
#include "middleware/khronos/common/khrn_stats.h"

#include "helpers/vc_image/vc_image.h"
#if !defined(V3D_LEAN)
#include "vcinclude/hardware.h"
#endif

#include <stdlib.h>
#include <string.h>

#define DEFAULT_ALIGN 64 /* todo: what should this be? */

/******************************************************************************
helpers
******************************************************************************/

static uint32_t get_width_ut(KHRN_IMAGE_FORMAT_T format, int32_t stride)
{
   vcos_assert(stride > 0);
   return ((uint32_t)(stride << 3) / khrn_image_get_bpp(format)) >> khrn_image_get_log2_utile_width(format);
}

/******************************************************************************
image handling
******************************************************************************/

bool khrn_image_prefer_lt(KHRN_IMAGE_FORMAT_T format, uint32_t width, uint32_t height)
{
   /* this is not arbitrary -- it matches the rule by which the 2708 hw chooses between lt/tf
    * todo: which means it should probably be platform-specific */
   return (width <= (uint32_t)(4 << khrn_image_get_log2_utile_width(format))) ||
          (height <= (uint32_t)(4 << khrn_image_get_log2_utile_height(format)));
}

uint32_t khrn_image_get_width_ut(const KHRN_IMAGE_T *image)
{
   return get_width_ut(image->format, image->stride);
}

uint32_t khrn_image_get_width_tiles(const KHRN_IMAGE_T *image)
{
   return get_width_ut(image->format, image->stride) >> 3;
}

uint32_t khrn_image_get_aux_width_ut(const KHRN_IMAGE_T *image)
{
   switch (image->format) {
   case DEPTH_16_TF:
   case DEPTH_32_TF:
   {
      uint32_t early_z_width = (image->width + 3) >> 2;
      uint32_t early_z_stride = khrn_image_get_stride(DEPTH_16_TF, early_z_width);
      return get_width_ut(DEPTH_16_TF, early_z_stride);
   }
   default:  /* This function only makes sense for early z auxiliary image */
      UNREACHABLE();
      return 0;
   }
}

uint32_t khrn_image_wrap_get_width_ut(const KHRN_IMAGE_WRAP_T *wrap)
{
   return get_width_ut(wrap->format, wrap->stride);
}

uint32_t khrn_image_wrap_get_width_tiles(const KHRN_IMAGE_WRAP_T *wrap)
{
   return get_width_ut(wrap->format, wrap->stride) >> 3;
}

uint32_t khrn_image_get_align(const KHRN_IMAGE_T *image)
{
   return image->offset ? (uint32_t)_min(mem_get_align(image->mh_storage),
      image->offset & ~(image->offset - 1)) :
      mem_get_align(image->mh_storage);
}

uint32_t khrn_image_get_space(const KHRN_IMAGE_T *image)
{
   return mem_get_size(image->mh_storage) - image->offset;
}

void khrn_image_term(void *v, uint32_t size)
{
   KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)v;

   UNUSED(size);

   khrn_interlock_term(&image->interlock);

   MEM_ASSIGN(image->mh_storage, MEM_INVALID_HANDLE);
   MEM_ASSIGN(image->mh_aux, MEM_INVALID_HANDLE);
}

#define MAX_IMAGES 3
extern unsigned char *images[MAX_IMAGES];
extern unsigned int types[MAX_IMAGES];
extern unsigned int widths[MAX_IMAGES];
extern unsigned int heights[MAX_IMAGES];
extern unsigned int pitches[MAX_IMAGES];
extern unsigned int used_images;

int create_window_surface = 0;

MEM_HANDLE_T khrn_image_create_from_storage(KHRN_IMAGE_FORMAT_T format,
   uint32_t width, uint32_t height, int32_t stride,
   MEM_HANDLE_T aux_handle, MEM_HANDLE_T storage_handle, uint32_t offset,
   KHRN_IMAGE_CREATE_FLAG_T flags)
{
	void *p;

   MEM_HANDLE_T handle;
   KHRN_IMAGE_T *image;

   vcos_assert(format != IMAGE_FORMAT_INVALID);

   /*
      alloc struct
   */

   handle = MEM_ALLOC_STRUCT_EX(KHRN_IMAGE_T, MEM_COMPACT_DISCARD);
   if (handle == MEM_INVALID_HANDLE) {
      return MEM_INVALID_HANDLE;
   }

   /*
      fill in the struct
   */


   image = (KHRN_IMAGE_T *)mem_lock(handle);
   image->format = format;
   image->width = (uint16_t)width;
   image->height = (uint16_t)height;
   image->stride = stride;
   if (aux_handle != MEM_INVALID_HANDLE) {
      mem_acquire(aux_handle);
   }
   image->mh_aux = aux_handle;

   if (storage_handle && create_window_surface)
   {
	   p = mem_lock(storage_handle);

	   assert(used_images < MAX_IMAGES);

	   images[used_images] = p;
	   types[used_images] = format;
	   widths[used_images] = width;
	   heights[used_images] = height;
	   pitches[used_images] = stride;

	   used_images++;

	   printf("creating image %p from memory %p\n", image, p);
	   mem_unlock(storage_handle);
   }

   if (storage_handle != MEM_INVALID_HANDLE) {
      mem_acquire(storage_handle);
   }
   image->mh_storage = storage_handle;
   image->offset = offset;
   image->flags =
      ((flags & IMAGE_CREATE_FLAG_TEXTURE) ? IMAGE_FLAG_TEXTURE : 0) |
      ((flags & IMAGE_CREATE_FLAG_RENDER_TARGET) ? IMAGE_FLAG_RENDER_TARGET : 0) |
      ((flags & IMAGE_CREATE_FLAG_DISPLAY) ? IMAGE_FLAG_DISPLAY : 0);
   khrn_interlock_init(&image->interlock);
   if (flags & IMAGE_CREATE_FLAG_INVALID) {
      khrn_interlock_invalidate(&image->interlock);
   }
#ifdef IMAGE_VC_IMAGE
   khrn_image_fill_vcimage(image, &image->vc_image);
#endif
   mem_unlock(handle);

   /*
      set the terminator
   */

   mem_set_term(handle, khrn_image_term);

   return handle;
}

MEM_HANDLE_T khrn_image_create(KHRN_IMAGE_FORMAT_T format,
   uint32_t width, uint32_t height,
   KHRN_IMAGE_CREATE_FLAG_T flags)
{
   uint32_t padded_width = width, padded_height = height, align = DEFAULT_ALIGN, stagger = 0, storage_size;
   MEM_HANDLE_T aux_handle, storage_handle, handle;

   vcos_assert(format != IMAGE_FORMAT_INVALID);

   khrn_image_platform_fudge(&format, &padded_width, &padded_height, &align, &stagger, flags);

   /*
      alloc palette / early z if needed
   */

   aux_handle = MEM_INVALID_HANDLE;

   if (khrn_image_is_paletted(format)) {
      aux_handle = mem_alloc_ex((1 << khrn_image_get_bpp(format)) * sizeof(uint32_t), alignof(uint32_t), MEM_FLAG_NONE, "KHRN_IMAGE_T.aux (palette)", MEM_COMPACT_DISCARD);      // check, no term
      if (aux_handle == MEM_INVALID_HANDLE) {
         return MEM_INVALID_HANDLE;
      }
   }

   if (khrn_hw_supports_early_z() && (format == DEPTH_16_TF || format == DEPTH_32_TF)) {
      uint32_t early_z_width  = (width  + 3) >> 2;
      uint32_t early_z_height = (height + 3) >> 2;

      uint32_t aux_size = khrn_image_get_size(format, early_z_width, early_z_height);

      if (flags & IMAGE_CREATE_FLAG_PAD_ROTATE)
         aux_size = _max(aux_size, khrn_image_get_size(format, early_z_height, early_z_width));

      aux_handle = mem_alloc_ex(
         aux_size,
         64, /* todo */
         (MEM_FLAG_T)(MEM_FLAG_DIRECT | MEM_FLAG_RESIZEABLE | (((flags & IMAGE_CREATE_FLAG_INIT_MASK) == IMAGE_CREATE_FLAG_ZERO) ? (MEM_FLAG_ZERO | MEM_FLAG_INIT) : MEM_FLAG_NO_INIT)),
         "KHRN_IMAGE_T.aux (early z)",
         MEM_COMPACT_DISCARD);
      if (aux_handle == MEM_INVALID_HANDLE) {
         /* TODO: should we fail, or just silently not create the early z buffer? */
         return MEM_INVALID_HANDLE;
      }
      if ((flags & IMAGE_CREATE_FLAG_INIT_MASK) == IMAGE_CREATE_FLAG_ONE) {
         khrn_memset(mem_lock(aux_handle), -1, aux_size);
         mem_unlock(aux_handle);
      }
   }

   /*
      alloc storage
   */

   if (flags & IMAGE_CREATE_FLAG_NO_STORAGE) {
      storage_handle = MEM_INVALID_HANDLE;
   } else {
      storage_size = khrn_image_get_size(format, padded_width, padded_height);
      if (flags & IMAGE_CREATE_FLAG_PAD_ROTATE)
         storage_size = _max(storage_size, khrn_image_get_size(format, padded_height, padded_width));
      storage_size += stagger;

      storage_handle = mem_alloc_ex(storage_size, align,
         (MEM_FLAG_T)(MEM_FLAG_DIRECT | MEM_FLAG_RESIZEABLE | (((flags & IMAGE_CREATE_FLAG_INIT_MASK) == IMAGE_CREATE_FLAG_ZERO) ? (MEM_FLAG_ZERO | MEM_FLAG_INIT) : MEM_FLAG_NO_INIT)),
         "KHRN_IMAGE_T.storage",
         MEM_COMPACT_DISCARD);     // check, no term
      if (storage_handle == MEM_INVALID_HANDLE) {
         if (aux_handle != MEM_INVALID_HANDLE) {
            mem_release(aux_handle);
         }
         return MEM_INVALID_HANDLE;
      }
      if ((flags & IMAGE_CREATE_FLAG_INIT_MASK) == IMAGE_CREATE_FLAG_ONE) {
         khrn_memset(mem_lock(storage_handle), -1, storage_size);
         mem_unlock(storage_handle);
      }
   }

   /*
      alloc and fill in struct
   */

   handle = khrn_image_create_from_storage(format,
      width, height, khrn_image_get_stride(format, padded_width),
      aux_handle, storage_handle, stagger,
      flags);
   if (aux_handle != MEM_INVALID_HANDLE) {
      mem_release(aux_handle);
   }
   if (storage_handle != MEM_INVALID_HANDLE) {
      mem_release(storage_handle);
   }

   return handle;
}

MEM_HANDLE_T khrn_image_create_dup(const KHRN_IMAGE_T *src,
   KHRN_IMAGE_CREATE_FLAG_T flags)
{
   MEM_HANDLE_T handle;
   KHRN_IMAGE_T *image;

   /*
      alloc new image
   */

   handle = khrn_image_create(src->format, src->width, src->height,
      /* todo: preserve rotate padding? */
      (KHRN_IMAGE_CREATE_FLAG_T)(flags |
      ((src->flags & IMAGE_FLAG_TEXTURE) ? IMAGE_CREATE_FLAG_TEXTURE : 0) |
      ((src->flags & IMAGE_FLAG_RENDER_TARGET) ? IMAGE_CREATE_FLAG_RENDER_TARGET : 0) |
      ((src->flags & IMAGE_FLAG_DISPLAY) ? IMAGE_CREATE_FLAG_DISPLAY : 0)));
   if (handle == MEM_INVALID_HANDLE) {
      return MEM_INVALID_HANDLE;
   }

   /*
      copy
   */

   image = (KHRN_IMAGE_T *)mem_lock(handle);
   vcos_assert((src->mh_aux == MEM_INVALID_HANDLE) == (image->mh_aux == MEM_INVALID_HANDLE));
   if (src->mh_aux != MEM_INVALID_HANDLE) {
      vcos_assert(mem_get_size(src->mh_aux) == mem_get_size(image->mh_aux));
      khrn_memcpy(mem_lock(image->mh_aux), mem_lock(src->mh_aux), mem_get_size(src->mh_aux));
      mem_unlock(image->mh_aux);
      mem_unlock(src->mh_aux);
   }
   khrn_image_convert(image, src, IMAGE_CONV_GL);
   mem_unlock(handle);

   return handle;
}

bool khrn_image_resize(KHRN_IMAGE_T *image, uint32_t width, uint32_t height)
{
   KHRN_IMAGE_FORMAT_T format = image->format;
   uint32_t padded_width = width, padded_height = height, align = DEFAULT_ALIGN, stagger = 0;

   khrn_image_platform_fudge(&format, &padded_width, &padded_height, &align, &stagger, (KHRN_IMAGE_CREATE_FLAG_T)(
      ((image->flags & IMAGE_FLAG_TEXTURE) ? IMAGE_CREATE_FLAG_TEXTURE : 0) |
      ((image->flags & IMAGE_FLAG_RENDER_TARGET) ? IMAGE_CREATE_FLAG_RENDER_TARGET : 0) |
      ((image->flags & IMAGE_FLAG_DISPLAY) ? IMAGE_CREATE_FLAG_DISPLAY : 0)));
   vcos_assert(format == image->format);
   vcos_assert(image->mh_storage == MEM_INVALID_HANDLE || align <= khrn_image_get_align(image));

   /*
      this function should only be called if we know an external constraint
      will ensure that our existing backing is sufficiently large
   */

   if (khrn_hw_supports_early_z() && (image->format == DEPTH_16_TF || image->format == DEPTH_32_TF)) {
      uint32_t early_z_width  = (width  + 3) >> 2;
      uint32_t early_z_height = (height + 3) >> 2;

      uint32_t aux_size = khrn_image_get_size(image->format, early_z_width, early_z_height);

      if (mem_get_size(image->mh_aux) < aux_size) {
         if (!mem_resize_ex(image->mh_aux, aux_size, MEM_COMPACT_DISCARD))
            return false;
      }
   }

   if (image->mh_storage != MEM_INVALID_HANDLE && mem_get_size(image->mh_storage) != 0) {
      uint32_t storage_size = khrn_image_get_size(image->format, padded_width, padded_height) + stagger;
      if (mem_get_size(image->mh_storage) < storage_size) {
         if (!mem_resize_ex(image->mh_storage, storage_size, MEM_COMPACT_DISCARD))
            return false;
      }
   }

   image->width = width;
   image->height = height;
   image->stride = khrn_image_get_stride(image->format, padded_width);

#ifdef IMAGE_VC_IMAGE
   khrn_image_fill_vcimage(image, &image->vc_image);
#endif

   return true;
}

void khrn_image_lock_wrap(const KHRN_IMAGE_T *image, KHRN_IMAGE_WRAP_T *wrap)
{
   wrap->format = image->format;
   wrap->width = image->width;
   wrap->height = image->height;
   wrap->stride = image->stride;
   wrap->aux = (image->mh_aux == MEM_INVALID_HANDLE) ? NULL : mem_lock(image->mh_aux);
   wrap->storage = (char*)mem_lock(image->mh_storage) + image->offset;
}

void khrn_image_unlock_wrap(const KHRN_IMAGE_T *image)
{
   mem_unlock(image->mh_storage);
   if (image->mh_aux != MEM_INVALID_HANDLE) {
      mem_unlock(image->mh_aux);
   }
}

/******************************************************************************
blitting etc
******************************************************************************/

static INLINE uint32_t get_bit(const uint8_t *addr, uint32_t bit)
{
   vcos_assert(bit < 8);

   return *addr >> bit & 1;
}

static INLINE uint32_t get_nibble(const uint8_t *addr, uint32_t nibble)
{
   vcos_assert(nibble < 2);

   nibble <<= 2;

   return *addr >> nibble & 0xf;
}

uint32_t khrn_image_wrap_get_pixel(const KHRN_IMAGE_WRAP_T *wrap, uint32_t x, uint32_t y)
{
   uint32_t pixel = 0;

   vcos_assert(khrn_image_is_uncomp(wrap->format));
   vcos_assert(x < wrap->width);
   vcos_assert(y < wrap->height);

   switch (wrap->format & IMAGE_FORMAT_MEM_LAYOUT_MASK) {
   case IMAGE_FORMAT_RSO:
   {
      void *row = (uint8_t *)wrap->storage + (y * wrap->stride);

      switch (wrap->format & IMAGE_FORMAT_PIXEL_SIZE_MASK) {
      case IMAGE_FORMAT_1:  pixel = get_bit((uint8_t *)row + (x >> 3), x & 0x7); break;
      case IMAGE_FORMAT_4:  pixel = get_nibble((uint8_t *)row + (x >> 1), x & 0x1); break;
      case IMAGE_FORMAT_8:  pixel = ((uint8_t *)row)[x]; break;
      case IMAGE_FORMAT_16: vcos_assert(!((uintptr_t)row & 1)); pixel = ((uint16_t *)row)[x]; break;
      case IMAGE_FORMAT_32: vcos_assert(!((uintptr_t)row & 3)); pixel = ((uint32_t *)row)[x]; break;
      case IMAGE_FORMAT_24:
         pixel = ((uint8_t *)row)[3*x+2]<<16 | ((uint8_t *)row)[3*x+1]<<8 | ((uint8_t *)row)[3*x];
         break;
      default:              UNREACHABLE();
      }

      break;
   }
   case IMAGE_FORMAT_TF:
   case IMAGE_FORMAT_LT:
   {
      uint32_t log2_utile_width = khrn_image_get_log2_utile_width(wrap->format);
      uint32_t log2_utile_height = khrn_image_get_log2_utile_height(wrap->format);

      uint32_t ut_w = khrn_image_wrap_get_width_ut(wrap);
      uint32_t ut_x = x >> log2_utile_width;
      uint32_t ut_y = y >> log2_utile_height;

      void *ut_base = (uint8_t *)wrap->storage +
         (khrn_tformat_utile_addr(ut_w, ut_x, ut_y, khrn_image_is_tformat(wrap->format), NULL) * 64);

      x &= (1 << log2_utile_width) - 1;
      y &= (1 << log2_utile_height) - 1;

      switch (wrap->format & IMAGE_FORMAT_PIXEL_SIZE_MASK) {
      case IMAGE_FORMAT_1:  pixel = get_bit((uint8_t *)ut_base + (y * 8) + (x >> 3), x & 0x7); break;
      case IMAGE_FORMAT_4:  pixel = get_nibble((uint8_t *)ut_base + (y * 8) + (x >> 1), x & 0x1); break;
      case IMAGE_FORMAT_8:  pixel = ((uint8_t *)ut_base)[(y * 8) + x]; break;
      case IMAGE_FORMAT_16: pixel = ((uint16_t *)ut_base)[(y * 8) + x]; break;
      case IMAGE_FORMAT_32: pixel = ((uint32_t *)ut_base)[(y * 4) + x]; break;
      default:              UNREACHABLE();
      }

      break;
   }
   default:
   {
      UNREACHABLE();
   }
   }

   return pixel;
}

static uint32_t get_packed_mask_pixel(const KHRN_IMAGE_WRAP_T *wrap, uint32_t x, uint32_t y)
{
   uint32_t channel;
   void *ut_base;

   vcos_assert(khrn_image_is_packed_mask(wrap->format) && (khrn_image_is_tformat(wrap->format) || khrn_image_is_lineartile(wrap->format)));
   vcos_assert(x < wrap->width);
   vcos_assert(y < wrap->height);

   channel = (y / VG_Q_TILE_HEIGHT) & 0x3;
   y = ((y / (VG_Q_TILE_HEIGHT * 4)) * VG_Q_TILE_HEIGHT) + (y & (VG_Q_TILE_HEIGHT - 1));

   ut_base = (uint8_t *)wrap->storage +
      (khrn_tformat_utile_addr(khrn_image_wrap_get_width_ut(wrap), x >> 2, y >> 2, khrn_image_is_tformat(wrap->format), NULL) * 64);
   return ((uint8_t *)ut_base)[((y & 3) * 16) + ((x & 3) * 4) + channel];
}

static INLINE void put_bit(uint8_t *addr, uint32_t bit, uint32_t value)
{
   vcos_assert(bit < 8);
   vcos_assert(value < 2);

   *addr = (uint8_t)((*addr & ~(1 << bit)) | (value << bit));
}

static INLINE void put_nibble(uint8_t *addr, uint32_t nibble, uint32_t value)
{
   vcos_assert(nibble < 2);
   vcos_assert(value < 16);

   nibble <<= 2;

   *addr = (uint8_t)((*addr & ~(0xf << nibble)) | (value << nibble));
}

void khrn_image_wrap_put_pixel(KHRN_IMAGE_WRAP_T *wrap, uint32_t x, uint32_t y, uint32_t pixel)
{
   vcos_assert(khrn_image_is_uncomp(wrap->format));
   vcos_assert(x < wrap->width);
   vcos_assert(y < wrap->height);

   switch (wrap->format & IMAGE_FORMAT_MEM_LAYOUT_MASK) {
   case IMAGE_FORMAT_RSO:
   {
      void *row = (uint8_t *)wrap->storage + (y * wrap->stride);

      switch (wrap->format & IMAGE_FORMAT_PIXEL_SIZE_MASK) {
      case IMAGE_FORMAT_1:  put_bit((uint8_t *)row + (x >> 3), x & 0x7, pixel); break;
      case IMAGE_FORMAT_4:  put_nibble((uint8_t *)row + (x >> 1), x & 0x1, pixel); break;
      case IMAGE_FORMAT_8:  ((uint8_t *)row)[x] = (uint8_t)pixel; break;
      case IMAGE_FORMAT_16: vcos_assert(!((uintptr_t)row & 1)); ((uint16_t *)row)[x] = (uint16_t)pixel; break;
      case IMAGE_FORMAT_32: vcos_assert(!((uintptr_t)row & 1)); ((uint32_t *)row)[x] = pixel; break;
      case IMAGE_FORMAT_24:
         ((uint8_t *)row)[3*x+2] = (uint8_t)(pixel>>16 & 0xff);
         ((uint8_t *)row)[3*x+1] = (uint8_t)(pixel>>8 & 0xff);
         ((uint8_t *)row)[3*x+0] = (uint8_t)(pixel & 0xff);
         break;
      default:              UNREACHABLE();
      }

      break;
   }
   case IMAGE_FORMAT_TF:
   case IMAGE_FORMAT_LT:
   {
      uint32_t log2_utile_width = khrn_image_get_log2_utile_width(wrap->format);
      uint32_t log2_utile_height = khrn_image_get_log2_utile_height(wrap->format);

      uint32_t ut_w = khrn_image_wrap_get_width_ut(wrap);
      uint32_t ut_x = x >> log2_utile_width;
      uint32_t ut_y = y >> log2_utile_height;

      void *ut_base = (uint8_t *)wrap->storage +
         (khrn_tformat_utile_addr(ut_w, ut_x, ut_y, khrn_image_is_tformat(wrap->format), NULL) * 64);

      x &= (1 << log2_utile_width) - 1;
      y &= (1 << log2_utile_height) - 1;

      switch (wrap->format & IMAGE_FORMAT_PIXEL_SIZE_MASK) {
      case IMAGE_FORMAT_1:  put_bit((uint8_t *)ut_base + (y * 8) + (x >> 3), x & 0x7, pixel); break;
      case IMAGE_FORMAT_4:  put_nibble((uint8_t *)ut_base + (y * 8) + (x >> 1), x & 0x1, pixel); break;
      case IMAGE_FORMAT_8:  ((uint8_t *)ut_base)[(y * 8) + x] = (uint8_t)pixel; break;
      case IMAGE_FORMAT_16: ((uint16_t *)ut_base)[(y * 8) + x] = (uint16_t)pixel; break;
      case IMAGE_FORMAT_32: ((uint32_t *)ut_base)[(y * 4) + x] = pixel; break;
      default:              UNREACHABLE();
      }

      break;
   }
   default:
   {
      UNREACHABLE();
   }
   }
}

void khrn_image_wrap_put_etc1_block(KHRN_IMAGE_WRAP_T *wrap, uint32_t x, uint32_t y, uint32_t word0, uint32_t word1)
{
   uint32_t l2utw, l2uth, ut_x, ut_y, ut_w;
   void *ut_base;

   vcos_assert(khrn_image_is_etc1(wrap->format) && (khrn_image_is_tformat(wrap->format) || khrn_image_is_lineartile(wrap->format)));
   vcos_assert(4 * x < wrap->width);
   vcos_assert(4 * y < wrap->height);

   l2utw = 1;   //utile is 2 blocks wide
   l2uth = 2;   //utile is 4 blocks high
   ut_x = x >> l2utw;
   ut_y = y >> l2uth;

   ut_w = khrn_image_wrap_get_width_ut(wrap);

   ut_base = (uint8_t *)wrap->storage + khrn_tformat_utile_addr(ut_w, ut_x, ut_y, khrn_image_is_tformat(wrap->format), NULL) * 64;

   x &= (1 << l2utw) - 1;
   y &= (1 << l2uth) - 1;

   vcos_assert(x < 2);
   vcos_assert(y < 4);

   ((uint32_t *)ut_base)[x * 2 + y * 4]     = word0;
   ((uint32_t *)ut_base)[x * 2 + y * 4 + 1] = word1;
}

uint32_t khrn_image_pixel_to_rgba(KHRN_IMAGE_FORMAT_T format, uint32_t pixel, KHRN_IMAGE_CONV_T conv)
{
   uint32_t rgba = 0;

   vcos_assert(khrn_image_is_color(format));

   switch (format & (IMAGE_FORMAT_PIXEL_SIZE_MASK | IMAGE_FORMAT_PIXEL_LAYOUT_MASK)) {
   case (IMAGE_FORMAT_32 | IMAGE_FORMAT_8888):
   {
      vcos_assert((format & IMAGE_FORMAT_RGB) && !(format & IMAGE_FORMAT_L));
      rgba = pixel;
      break;
   }
   case (IMAGE_FORMAT_24 | IMAGE_FORMAT_888):
   {
      vcos_assert((format & IMAGE_FORMAT_RGB) && !(format & IMAGE_FORMAT_L));
      rgba = pixel | 0xff000000;
      break;
   }
   case (IMAGE_FORMAT_16 | IMAGE_FORMAT_4444):
   {
      vcos_assert((format & IMAGE_FORMAT_RGB) && !(format & IMAGE_FORMAT_L));
      rgba =
         (((pixel >> 0) & 0xf) * 0x00000011) |
         (((pixel >> 4) & 0xf) * 0x00001100) |
         (((pixel >> 8) & 0xf) * 0x00110000) |
         (((pixel >> 12) & 0xf) * 0x11000000);
      break;
   }
   case (IMAGE_FORMAT_16 | IMAGE_FORMAT_5551):
   {
      vcos_assert((format & IMAGE_FORMAT_RGB) && !(format & IMAGE_FORMAT_L));
      if (format & IMAGE_FORMAT_XA) {
         format = (KHRN_IMAGE_FORMAT_T)(format & ~IMAGE_FORMAT_XA);
         pixel = (pixel >> 1) | ((pixel & 0x1) << 15);
      }
      rgba =
         ((((pixel & 0x001f) * 0x21) >> (0 + 2)) << 0) |
         ((((pixel & 0x03e0) * 0x21) >> (5 + 2)) << 8) |
         ((((pixel & 0x7c00) * 0x21) >> (10 + 2)) << 16) |
         ((pixel >> 15) * 0xff000000);
      break;
   }
   case (IMAGE_FORMAT_16 | IMAGE_FORMAT_565):
   {
      vcos_assert((format & IMAGE_FORMAT_RGB) && !(format & (IMAGE_FORMAT_L | IMAGE_FORMAT_A | IMAGE_FORMAT_XA)));
      rgba =
         ((((pixel & 0x001f) * 0x21) >> (0 + 2)) << 0) |
         ((((pixel & 0x07e0) * 0x41) >> (5 + 4)) << 8) |
         ((((pixel & 0xf800) * 0x21) >> (11 + 2)) << 16) | (0xFF000000);
      break;
   }
   case (IMAGE_FORMAT_16 | IMAGE_FORMAT_88):
   {
      vcos_assert(!(format & (IMAGE_FORMAT_RGB | IMAGE_FORMAT_XRGBX)) && (format & IMAGE_FORMAT_L));
      if (format & IMAGE_FORMAT_XA) {
         rgba = pixel | ((pixel & 0xff00) * 0x00010100);
      } else {
         rgba = ((pixel & 0xff) * 0x00010101) | ((pixel & 0xff00) << 16);
      }
      break;
   }
   case IMAGE_FORMAT_8:
   case IMAGE_FORMAT_4:
   case IMAGE_FORMAT_1:
   {
      vcos_assert(!(format & (IMAGE_FORMAT_RGB | IMAGE_FORMAT_XRGBX | IMAGE_FORMAT_XA)) && (!!(format & IMAGE_FORMAT_L) ^ !!(format & IMAGE_FORMAT_A)));
      switch (format & IMAGE_FORMAT_PIXEL_SIZE_MASK) {
      case IMAGE_FORMAT_8: rgba = pixel * 0x01010101; break;
      case IMAGE_FORMAT_4: rgba = pixel * 0x11111111; break;
      case IMAGE_FORMAT_1: rgba = pixel * 0xffffffff; break;
      default:             UNREACHABLE();
      }
      break;
   }
   default:
   {
      UNREACHABLE();
   }
   }

   if (format & IMAGE_FORMAT_XA) {
      rgba = (rgba >> 8) | (rgba << 24);
   }

   if (format & IMAGE_FORMAT_XRGBX) {
      rgba =
         (((rgba >> 0) & 0xff) << 16) |
         (((rgba >> 8) & 0xff) << 8) |
         (((rgba >> 16) & 0xff) << 0) |
         (rgba & 0xff000000);
   }

   if (!(format & IMAGE_FORMAT_A)) {
      rgba |= 0xff000000;
   }

   if (!(format & (IMAGE_FORMAT_RGB | IMAGE_FORMAT_L))) {
      /*
         in gl-land alpha-only textures are implicitly black
         in vg-land alpha-only textures are implicitly white (i'll assume this is the odd behaviour)
      */

      if (conv == IMAGE_CONV_VG) {
         rgba |= 0x00ffffff;
      } else {
         rgba &= 0xff000000;
      }
   }

   if ((conv == IMAGE_CONV_VG) && (format & IMAGE_FORMAT_PRE)) {
      rgba = khrn_color_rgba_clamp_to_a(rgba);
   }

   return rgba;
}

static INLINE uint32_t to_1(uint32_t x)
{
   return x >> 7;
}

static INLINE uint32_t to_4(uint32_t x)
{
   return ((x * 0xf) + 0x87) >> 8;
}

static INLINE uint32_t to_5(uint32_t x)
{
   if (x >= 128) { ++x; }
   x += 3 - (x >> 5);
   return x >> 3;
}

static INLINE uint32_t to_6(uint32_t x)
{
   if (x <= 84) { ++x; }
   if (x >= 171) { --x; }
   return x >> 2;
}

/*
   luminance values are taken from the red channel
*/

uint32_t khrn_image_rgba_to_pixel(KHRN_IMAGE_FORMAT_T format, uint32_t rgba, KHRN_IMAGE_CONV_T conv)
{
   uint32_t pixel = 0;

   vcos_assert(khrn_image_is_color(format));

   if (format & IMAGE_FORMAT_XRGBX) {
      rgba =
         (((rgba >> 0) & 0xff) << 16) |
         (((rgba >> 8) & 0xff) << 8) |
         (((rgba >> 16) & 0xff) << 0) |
         (rgba & 0xff000000);
   }

   if (format & IMAGE_FORMAT_XA &&
      ((format & (IMAGE_FORMAT_PIXEL_SIZE_MASK | IMAGE_FORMAT_PIXEL_LAYOUT_MASK)) != (IMAGE_FORMAT_16 | IMAGE_FORMAT_5551))) {
      rgba = (rgba << 8) | (rgba >> 24);
   }

   switch (format & (IMAGE_FORMAT_PIXEL_SIZE_MASK | IMAGE_FORMAT_PIXEL_LAYOUT_MASK)) {
   case (IMAGE_FORMAT_32 | IMAGE_FORMAT_8888):
   {
      vcos_assert((format & IMAGE_FORMAT_RGB) && !(format & IMAGE_FORMAT_L));
      pixel = rgba;
      break;
   }
   case (IMAGE_FORMAT_24 | IMAGE_FORMAT_888):
   {
      vcos_assert((format & IMAGE_FORMAT_RGB) && !(format & IMAGE_FORMAT_L));
      pixel = rgba & 0xffffff;
      break;
   }
   case (IMAGE_FORMAT_16 | IMAGE_FORMAT_4444):
   {
      vcos_assert((format & IMAGE_FORMAT_RGB) && !(format & IMAGE_FORMAT_L));
      pixel =
         (to_4((rgba >> 0) & 0xff) << 0) |
         (to_4((rgba >> 8) & 0xff) << 4) |
         (to_4((rgba >> 16) & 0xff) << 8) |
         (to_4((rgba >> 24) & 0xff) << 12);
      break;
   }
   case (IMAGE_FORMAT_16 | IMAGE_FORMAT_5551):
   {
      vcos_assert((format & IMAGE_FORMAT_RGB) && !(format & IMAGE_FORMAT_L));
      pixel =
         (to_5((rgba >> 0) & 0xff) << 0) |
         (to_5((rgba >> 8) & 0xff) << 5) |
         (to_5((rgba >> 16) & 0xff) << 10) |
         (to_1((rgba >> 24) & 0xff) << 15);
      if ((conv == IMAGE_CONV_VG) && (format & IMAGE_FORMAT_PRE) && !(pixel & (1 << 15))) {
         /* keep rgb <= a */
         pixel = 0;
      }
      if (format & IMAGE_FORMAT_XA) {
         pixel = ((pixel & 0x7fff) << 1) | (pixel >> 15);
      }
      break;
   }
   case (IMAGE_FORMAT_16 | IMAGE_FORMAT_565):
   {
      vcos_assert((format & IMAGE_FORMAT_RGB) && !(format & (IMAGE_FORMAT_L | IMAGE_FORMAT_A | IMAGE_FORMAT_XA)));
      pixel =
         (to_5((rgba >> 0) & 0xff) << 0) |
         (to_6((rgba >> 8) & 0xff) << 5) |
         (to_5((rgba >> 16) & 0xff) << 11);
      break;
   }
   case (IMAGE_FORMAT_16 | IMAGE_FORMAT_88):
   {
      vcos_assert(!(format & (IMAGE_FORMAT_RGB | IMAGE_FORMAT_XRGBX)) && (format & IMAGE_FORMAT_L));
      if (format & IMAGE_FORMAT_XA) {
         pixel = rgba & 0xffff;
      } else {
         pixel = (rgba & 0xff) | ((rgba >> 16) & 0xff00);
      }
      break;
   }
   case IMAGE_FORMAT_8:
   case IMAGE_FORMAT_4:
   case IMAGE_FORMAT_1:
   {
      vcos_assert(!(format & (IMAGE_FORMAT_RGB | IMAGE_FORMAT_XRGBX | IMAGE_FORMAT_XA)) && (!!(format & IMAGE_FORMAT_L) ^ !!(format & IMAGE_FORMAT_A)));
      rgba = (format & IMAGE_FORMAT_L) ? (rgba & 0xff) : (rgba >> 24);
      switch (format & IMAGE_FORMAT_PIXEL_SIZE_MASK) {
      case IMAGE_FORMAT_8: pixel = rgba; break;
      case IMAGE_FORMAT_4: pixel = to_4(rgba); break;
      case IMAGE_FORMAT_1: pixel = to_1(rgba); break;
      default:             UNREACHABLE();
      }
      break;
   }
   default:
   {
      UNREACHABLE();
   }
   }

   return pixel;
}

uint32_t khrn_image_rgba_convert_pre_lin(KHRN_IMAGE_FORMAT_T dst_format, KHRN_IMAGE_FORMAT_T src_format, uint32_t rgba)
{
   if ((dst_format ^ src_format) & (IMAGE_FORMAT_PRE | IMAGE_FORMAT_LIN)) {
      if (src_format & IMAGE_FORMAT_PRE) {
         rgba = khrn_color_rgba_unpre(rgba);
      }
      if ((dst_format ^ src_format) & IMAGE_FORMAT_LIN) {
         if (dst_format & IMAGE_FORMAT_LIN) {
            rgba = khrn_color_rgba_s_to_lin(rgba);
         } else {
            rgba = khrn_color_rgba_lin_to_s(rgba);
         }
      }
      if (dst_format & IMAGE_FORMAT_PRE) {
         rgba = khrn_color_rgba_pre(rgba);
      }
   }
   return rgba;
}

uint32_t khrn_image_rgba_convert_l_pre_lin(KHRN_IMAGE_FORMAT_T dst_format, KHRN_IMAGE_FORMAT_T src_format, uint32_t rgba)
{
   if ((dst_format & IMAGE_FORMAT_L) && !(src_format & IMAGE_FORMAT_L)) {
      if (!(src_format & IMAGE_FORMAT_LIN) && (src_format & IMAGE_FORMAT_PRE)) {
         rgba = khrn_color_rgba_unpre(rgba);
         src_format = (KHRN_IMAGE_FORMAT_T)(src_format & ~IMAGE_FORMAT_PRE);
      }
      if ((dst_format | src_format) & IMAGE_FORMAT_LIN) {
         if (!(src_format & IMAGE_FORMAT_LIN)) {
            rgba = khrn_color_rgba_s_to_lin(rgba);
            src_format = (KHRN_IMAGE_FORMAT_T)(src_format | IMAGE_FORMAT_LIN);
         }
         rgba = khrn_color_rgba_to_la_lin(rgba);
      } else {
         rgba = khrn_color_rgba_to_la_s(rgba);
      }
   }
   return khrn_image_rgba_convert_pre_lin(dst_format, src_format, rgba);
}

void khrn_image_wrap_clear_region(
   KHRN_IMAGE_WRAP_T *wrap, uint32_t x, uint32_t y,
   uint32_t width, uint32_t height,
   uint32_t rgba, /* rgba non-lin, unpre */
   KHRN_IMAGE_CONV_T conv)
{
   uint32_t pixel, i, j;

   khrn_stats_record_start(KHRN_STATS_IMAGE);

   vcos_assert(khrn_image_is_color(wrap->format));
   vcos_assert((x + width) <= wrap->width);
   vcos_assert((y + height) <= wrap->height);

   if (khrn_bf_clear_region(wrap, x, y, width, height, rgba, conv))
      goto done;

   khrn_stats_record_event(KHRN_STATS_UNACCELERATED_IMAGE);
   if (conv == IMAGE_CONV_VG) {
      rgba = khrn_image_rgba_convert_l_pre_lin(wrap->format, (KHRN_IMAGE_FORMAT_T)0 /* rgba, non-lin, non-pre */, rgba);
   }
   pixel = khrn_image_rgba_to_pixel(wrap->format, rgba, conv);

   for (j = 0; j != height; ++j) {
      for (i = 0; i != width; ++i) {
         khrn_image_wrap_put_pixel(wrap, x + i, y + j, pixel);
      }
   }
done:
   khrn_stats_record_end(KHRN_STATS_IMAGE);
}

static void copy_region_from_packed_mask_tf(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y)
{
   uint32_t i, j;

   vcos_assert((khrn_image_is_uncomp(dst->format) && (khrn_image_get_bpp(dst->format) == 8)) && (src->format == PACKED_MASK_TF));
   vcos_assert((dst_x + width) <= dst->width);
   vcos_assert((dst_y + height) <= dst->height);
   vcos_assert((src_x + width) <= src->width);
   vcos_assert((src_y + height) <= src->height);

   for (j = 0; j != height; ++j) {
      for (i = 0; i != width; ++i) {
         khrn_image_wrap_put_pixel(dst, dst_x + i, dst_y + j, get_packed_mask_pixel(src, src_x + i, src_y + j));
      }
   }
}

static void copy_pixel(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
   uint32_t pixel;

   vcos_assert(khrn_image_is_uncomp(dst->format) && khrn_image_is_uncomp(src->format));
   vcos_assert(dst_x < dst->width);
   vcos_assert(dst_y < dst->height);
   vcos_assert(src_x < src->width);
   vcos_assert(src_y < src->height);

   pixel = khrn_image_wrap_get_pixel(src, src_x, src_y);

   if (((dst->format ^ src->format) & ~IMAGE_FORMAT_MEM_LAYOUT_MASK) ||
      /* need khrn_image_pixel_to_rgba to clamp rgb to a */
      ((conv == IMAGE_CONV_VG) && khrn_image_is_color(src->format) && (src->format & IMAGE_FORMAT_PRE))) {
      if (khrn_image_is_paletted(src->format) && khrn_image_is_color(dst->format)) {
         /* we can convert from paletted to non-paletted by doing palette lookups */
         uint32_t rgba;

         vcos_assert(src->aux);
         vcos_assert(pixel < 16);

         rgba = ((uint32_t *)src->aux)[pixel];
         /* TODO: would lum/pre/lin conversion make sense here? */
         pixel = khrn_image_rgba_to_pixel(dst->format, rgba, conv);
      } else {
         uint32_t rgba;

         vcos_assert(khrn_image_is_color(dst->format) && khrn_image_is_color(src->format));

         rgba = khrn_image_pixel_to_rgba(src->format, pixel, conv);
         if (conv == IMAGE_CONV_VG) {
            rgba = khrn_image_rgba_convert_l_pre_lin(dst->format, src->format, rgba);
         }
         pixel = khrn_image_rgba_to_pixel(dst->format, rgba, conv);
      }
   }

   khrn_image_wrap_put_pixel(dst, dst_x, dst_y, pixel);
}

static bool in_scissor_rect(
   uint32_t x, uint32_t y,
   const int32_t *scissor_rects, uint32_t scissor_rects_count)
{
   uint32_t i;
   for (i = 0; i != scissor_rects_count; i += 4) {
      int32_t rect_x      = scissor_rects[i];
      int32_t rect_y      = scissor_rects[i + 1];
      int32_t rect_width  = scissor_rects[i + 2];
      int32_t rect_height = scissor_rects[i + 3];
      if (((int32_t)x >= rect_x) && ((int32_t)x < _adds(rect_x, rect_width)) &&
         ((int32_t)y >= rect_y) && ((int32_t)y < _adds(rect_y, rect_height))) {
         return true;
      }
   }
   return false;
}

static void copy_scissor_regions(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv,
   const int32_t *scissor_rects, uint32_t scissor_rects_count)
{
   int32_t begin_j, end_j, dir_j;
   int32_t begin_i, end_i, dir_i;
   int32_t i, j;

   vcos_assert(khrn_image_is_uncomp(dst->format) && khrn_image_is_uncomp(src->format));
   vcos_assert((dst_x + width) <= dst->width);
   vcos_assert((dst_y + height) <= dst->height);
   vcos_assert((src_x + width) <= src->width);
   vcos_assert((src_y + height) <= src->height);

   /*
      choose blitting direction to handle overlapping src/dst
   */

   if (dst_y < src_y) {
      begin_j = 0;
      end_j = height;
      dir_j = 1;
   } else {
      begin_j = height - 1;
      end_j = -1;
      dir_j = -1;
   }

   if (dst_x < src_x) {
      begin_i = 0;
      end_i = width;
      dir_i = 1;
   } else {
      begin_i = width - 1;
      end_i = -1;
      dir_i = -1;
   }

   for (j = begin_j; j != end_j; j += dir_j) {
      for (i = begin_i; i != end_i; i += dir_i) {
         if (!scissor_rects || in_scissor_rect(dst_x + i, dst_y + j, scissor_rects, scissor_rects_count)) {
            copy_pixel(
               dst, dst_x + i, dst_y + j,
               src, src_x + i, src_y + j,
               conv);
         }
      }
   }
}

/* TFORMAT ACCELERATION ROUTINES
 *
 * Copying from linear to TFORMAT follows the following algorithm:
 *
 * For each 1k TILE
 *    calculate tile address
 *    calculate linear address
 *
 *    for each sub-pixel copy from linear to Tformat
 *
 * Address in the TFORMAT image is calculated using lookup tables or-inlined
 *
 * TFORMAT address calculated as:
 *
 * +--------------------------------------------------------------------------------+
 * | BLOCK ADDRESSS | MAP[Y5:Y4:X4] | Y3 | Y2 | X3 | X2 | Y1 | Y0 | X1 | X0 | 0 | 0 |
 * +--------------------------------------------------------------------------------+
 *
 * The low order bits of the address are calculated from the subX and subY position
 * using the s_ySwizzle[] and s_xSwizzle[] tables (a bit twiddling solution is also
 * possible).
 *
 * MAP converts an X, Y tile address to the tile ordering within TFORMAT blocks according
 *     to whether it is an odd (Y5=1) or even (Y5 = 0) block row.  This function is implemented
 *     in s_tileOrder[].  MAP is encoded in a single word in the routines below to avoid extra
 *     memory accesses.
 *
 * BLOCK ADDRESS is calculated from the block X and Y position, noting that the X position
 *     is inverted if Y5 = 1 (odd block rows)
 */

/* This converts (Y5:Y4:X4) bits to the bits required to index the correct Tformat block */
/*
static const unsigned int s_tileOrder[] =
{
      0x000, 0xc00, 0x400, 0x800,
      0x800, 0x400, 0xc00, 0x000
};
*/

/* This converts lower four bits of x-address into the corresponding bits of the Tformat address
 * It is equivalelent to XSWIZZLE()
 */
static const unsigned int s_xSwizzle[] =
{
      0x00, 0x04, 0x08, 0x0c,
      0x40, 0x44, 0x48, 0x4c,
      0x80, 0x84, 0x88, 0x8c,
      0xc0, 0xc4, 0xc8, 0xcc
};

#define XSWIZZLE(X)  ((((X) & 0x3) << 2) | (((X) & 0xc) << 4))

/* This converts lower four bits of y-address into the corresponding bits of the Tformat address
 * It is equivalent to YSWIZZLE()
 */
static const unsigned int s_ySwizzle[] =
{
      0x000, 0x010, 0x020, 0x030,
      0x100, 0x110, 0x120, 0x130,
      0x200, 0x210, 0x220, 0x230,
      0x300, 0x310, 0x320, 0x330
};

#define YSWIZZLE(Y)  ((((Y) & 0x3) << 4) | (((Y) & 0xc) << 6))

/* khrn_copy_rgba8888_to_tf32
 *
 * Copies an array in RGBA 32-bit linear format to the equivalent 32-bit Tformat array
 */
static void khrn_copy_rgba8888_to_tf32(
   unsigned char *dst,                                 /* address of Tformat image                                             */
   unsigned int  dstWidth,                             /* width of destination in texels                                       */
   unsigned int  dstStartX, unsigned int dstStartY,    /* bottom left of destination region                                    */
   unsigned int  width,     unsigned int height,       /* top right of destination region                                      */
   unsigned char *src,                                 /* address of linear image                                              */
   unsigned int  srcStartX, unsigned int srcStartY,    /* bottom left of source region                                         */
   unsigned int  srcStride)                            /* stride of source image in bytes (i.e. line length including padding) */
{
   static const unsigned int   BLOCK_SIZE  = 4096;
   static const unsigned int   BLOCK_SHIFT = 5;
   static const unsigned int   BLOCK_BITS  = 0x1f;
   static const unsigned int   TILE_WIDTH  = 16;
   static const unsigned int   TEXEL_SIZE  = 4;

   unsigned int   blockY, blockX, subY, subX;

   unsigned int   dstBlockWidth  = ((dstWidth + BLOCK_BITS) & ~BLOCK_BITS) >> BLOCK_SHIFT;  /* Width in 4k tiles */
   unsigned int   dstBlockStride = dstBlockWidth * BLOCK_SIZE;

   unsigned int   dstEndX    = dstStartX + width - 1;
   unsigned int   dstEndY    = dstStartY + height - 1;

   unsigned int   startBlockX = dstStartX >> BLOCK_SHIFT;
   unsigned int   startBlockY = dstStartY >> BLOCK_SHIFT;
   unsigned int   endBlockX   = dstEndX   >> BLOCK_SHIFT;
   unsigned int   endBlockY   = dstEndY   >> BLOCK_SHIFT;

   int   offX = srcStartX - dstStartX;
   int   offY = srcStartY - dstStartY;

   /* For each 4k tile vertically */
   for (blockY = startBlockY; blockY <= endBlockY; ++blockY)
   {
      unsigned int   startSubY = blockY == startBlockY ? (dstStartY & BLOCK_BITS) : 0;
      unsigned int   endSubY   = blockY == endBlockY   ? (dstEndY   & BLOCK_BITS) : BLOCK_BITS;

      unsigned int   odd = blockY & 1;
      unsigned int   mask = odd ? 0x0c48 : 0x84c0;

      /* For each 4k tile horizontally */
      for (blockX = startBlockX; blockX <= endBlockX; ++blockX)
      {
         unsigned int   startSubX = blockX == startBlockX ? (dstStartX & BLOCK_BITS) : 0;
         unsigned int   endSubX   = blockX == endBlockX   ? (dstEndX   & BLOCK_BITS) : BLOCK_BITS;
         bool           fast      = startSubX == 0 && endSubX == BLOCK_BITS;

         unsigned int   blockXR   = odd ? dstBlockWidth - blockX - 1 : blockX;

         /* Address of 32 x 32 block */
         unsigned char  *blockAddr  = dst + blockY * dstBlockStride + blockXR * BLOCK_SIZE;
         unsigned int   Y = (blockY << BLOCK_SHIFT) + offY;
         unsigned int   X = (blockX << BLOCK_SHIFT) + startSubX + offX;

         /* For each row in the sub tile */
         for (subY = startSubY; subY <= endSubY; ++subY)
         {
            unsigned char  *src_texel = src + (Y + subY) * srcStride + X * TEXEL_SIZE;

            unsigned int   offset     = mask >> ((subY & 0x10) >> 1);
            unsigned char  *rowAddr   = blockAddr + YSWIZZLE(subY);
            unsigned char  *rowAddr0  = rowAddr + ((offset & 0x0f) << 8);
            unsigned char  *rowAddr1  = rowAddr + ((offset & 0xf0) << 4);

            if (fast)
            {
               *(unsigned int *)(rowAddr0 + 0x00) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0x04) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0x08) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0x0c) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0x40) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0x44) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0x48) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0x4c) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0x80) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0x84) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0x88) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0x8c) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0xc0) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0xc4) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0xc8) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr0 + 0xcc) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;

               *(unsigned int *)(rowAddr1 + 0x00) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0x04) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0x08) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0x0c) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0x40) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0x44) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0x48) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0x4c) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0x80) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0x84) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0x88) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0x8c) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0xc0) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0xc4) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0xc8) = *(unsigned int *)src_texel; src_texel += TEXEL_SIZE;
               *(unsigned int *)(rowAddr1 + 0xcc) = *(unsigned int *)src_texel;
            }
            else
            {
               /* Slower path for edges of image */
               unsigned int   endSubX1 = endSubX < (TILE_WIDTH - 1) ? endSubX : TILE_WIDTH - 1;

               for (subX = startSubX; subX <= endSubX1; ++subX)
               {
                  unsigned int *dst_texel =  (unsigned int *)(rowAddr0 + s_xSwizzle[subX]);
                  *dst_texel = *(unsigned int *)src_texel;
                  src_texel += TEXEL_SIZE;
               }

               for (subX = startSubX > TILE_WIDTH ? startSubX : TILE_WIDTH; subX <= endSubX; ++subX)
               {
                  unsigned int *dst_texel =  (unsigned int *)(rowAddr1 + s_xSwizzle[subX & 0xf]);
                  *dst_texel = *(unsigned int *)src_texel;
                  src_texel += TEXEL_SIZE;
               }
            }
         }
      }
   }
}

/* khrn_copy_rgb565_to_tf16
 *
 * Copies an array in RGB565 linear format to the equivalent 16-bit Tformat array
 */
static void khrn_copy_rgb565_to_tf16(
   unsigned char *dst,                                 /* address of Tformat image                                             */
   unsigned int  dstWidth,                             /* width of destination in texels                                       */
   unsigned int  dstStartX, unsigned int dstStartY,    /* bottom left of destination region                                    */
   unsigned int  width,     unsigned int height,       /* top right of destination region                                      */
   unsigned char *src,                                 /* address of linear image                                              */
   unsigned int  srcStartX, unsigned int srcStartY,    /* bottom left of source region                                         */
   unsigned int  srcStride)                            /* stride of source image in bytes (i.e. line length including padding) */
{
   static const unsigned int   BLOCK_SIZE   = 4096;
   static const unsigned int   BLOCK_XSHIFT = 6;
   static const unsigned int   BLOCK_XBITS  = 0x3f;
   static const unsigned int   BLOCK_YSHIFT = 5;
   static const unsigned int   BLOCK_YBITS  = 0x1f;
   static const unsigned int   TILE_WIDTH   = 32;
   static const unsigned int   TEXEL_SIZE   = 2;

   unsigned int   blockY, blockX, subY, subX;

   unsigned int   dstBlockWidth  = ((dstWidth + BLOCK_XBITS) & ~BLOCK_XBITS) >> BLOCK_XSHIFT;  /* Width in 4k tiles */
   unsigned int   dstBlockStride = dstBlockWidth * BLOCK_SIZE;

   unsigned int   dstEndX    = dstStartX + width  - 1;
   unsigned int   dstEndY    = dstStartY + height - 1;

   unsigned int   startBlockX = dstStartX >> BLOCK_XSHIFT;
   unsigned int   startBlockY = dstStartY >> BLOCK_YSHIFT;
   unsigned int   endBlockX   = dstEndX   >> BLOCK_XSHIFT;
   unsigned int   endBlockY   = dstEndY   >> BLOCK_YSHIFT;

   int   offX = srcStartX - dstStartX;
   int   offY = srcStartY - dstStartY;

   /* For each 4k tile vertically */
   for (blockY = startBlockY; blockY <= endBlockY; ++blockY)
   {
      unsigned int   startSubY = blockY == startBlockY ? (dstStartY & BLOCK_YBITS) : 0;
      unsigned int   endSubY   = blockY == endBlockY   ? (dstEndY   & BLOCK_YBITS) : BLOCK_YBITS;

      unsigned int   odd = blockY & 1;
      unsigned int   mask = odd ? 0x0c48 : 0x84c0;

      /* For each 4k tile horizontally */
      for (blockX = startBlockX; blockX <= endBlockX; ++blockX)
      {
         unsigned int   startSubX = blockX == startBlockX ? (dstStartX & BLOCK_XBITS) : 0;
         unsigned int   endSubX   = blockX == endBlockX   ? (dstEndX   & BLOCK_XBITS) : BLOCK_XBITS;
         bool           fast      = startSubX == 0 && endSubX == BLOCK_XBITS;

         unsigned int   blockXR   = odd ? dstBlockWidth - blockX - 1 : blockX;

         /* Address of 32 x 32 block */
         unsigned char  *blockAddr  = dst + blockY * dstBlockStride + blockXR * BLOCK_SIZE;
         unsigned int   Y = (blockY << BLOCK_YSHIFT) + offY;
         unsigned int   X = (blockX << BLOCK_XSHIFT) + startSubX + offX;

         /* For each row in the sub tile */
         for (subY = startSubY; subY <= endSubY; ++subY)
         {
            unsigned char  *src_texel = src + (Y + subY) * srcStride + X * TEXEL_SIZE;

            unsigned int   offset     = mask >> ((subY & 0x10) >> 1);
            unsigned char  *rowAddr   = blockAddr + YSWIZZLE(subY);
            unsigned char  *rowAddr0  = rowAddr + ((offset & 0x0f) << 8);
            unsigned char  *rowAddr1  = rowAddr + ((offset & 0xf0) << 4);
            
            if (fast)
            {
               /* Fast path for centre of image */
               /* Copy as integers for speed    */
			   /* Changed to Short Copy this was generating alignment fault when src_stride is odd*/
               *(unsigned short *)(rowAddr0 + 0x00) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
			   *(unsigned short *)(rowAddr0 + 0x02) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x04) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x06) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x08) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x0a) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x0c) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x0e) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
			   *(unsigned short *)(rowAddr0 + 0x40) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x42) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x44) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x46) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
			   *(unsigned short *)(rowAddr0 + 0x48) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
			   *(unsigned short *)(rowAddr0 + 0x4a) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
 			   *(unsigned short *)(rowAddr0 + 0x4c) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
 			   *(unsigned short *)(rowAddr0 + 0x4e) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
			   *(unsigned short *)(rowAddr0 + 0x80) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x82) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x84) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x86) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
			   *(unsigned short *)(rowAddr0 + 0x88) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x8a) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x8c) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0x8e) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0xc0) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0xc2) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0xc4) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0xc6) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0xc8) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0xca) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0xcc) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr0 + 0xce) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);

               *(unsigned short *)(rowAddr1 + 0x00) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
			   *(unsigned short *)(rowAddr1 + 0x02) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x04) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x06) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x08) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x0a) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x0c) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x0e) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
			   *(unsigned short *)(rowAddr1 + 0x40) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x42) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x44) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x46) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
			   *(unsigned short *)(rowAddr1 + 0x48) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
			   *(unsigned short *)(rowAddr1 + 0x4a) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
 			   *(unsigned short *)(rowAddr1 + 0x4c) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
 			   *(unsigned short *)(rowAddr1 + 0x4e) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
			   *(unsigned short *)(rowAddr1 + 0x80) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x82) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x84) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x86) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
			   *(unsigned short *)(rowAddr1 + 0x88) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x8a) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x8c) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0x8e) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0xc0) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0xc2) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0xc4) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0xc6) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0xc8) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0xca) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0xcc) = *(unsigned short *)src_texel; src_texel += sizeof(unsigned short);
               *(unsigned short *)(rowAddr1 + 0xce) = *(unsigned short *)src_texel;
            }
            else
            {
               /* Slower path for edges of image */
               unsigned int   endSubX1 = endSubX < (TILE_WIDTH - 1) ? endSubX : TILE_WIDTH - 1;

               for (subX = startSubX; subX <= endSubX1; ++subX)
               {
                  unsigned short *dst_texel =  (unsigned short *)(rowAddr0 + s_xSwizzle[subX >> 1]);
                  *(dst_texel + (subX & 1)) = *(unsigned short *)src_texel;
                  src_texel += TEXEL_SIZE;
               }

               for (subX = startSubX > TILE_WIDTH ? startSubX : TILE_WIDTH; subX <= endSubX; ++subX)
               {
                  unsigned short *dst_texel =  (unsigned short *)(rowAddr1 + s_xSwizzle[(subX >> 1) & 0xf]);
                  *(dst_texel + (subX & 1)) = *(unsigned short *)src_texel;
                  src_texel += TEXEL_SIZE;
               }
            }
         }
      }
   }
}

/* khrn_copy_rgb888_to_tf32
 *
 * Copies an array in RGB 24-bit linear format to the equivalent 32-bit Tformat array
 */
static void khrn_copy_rgb888_to_tf32(
   unsigned char *dst,                                 /* address of Tformat image                                               */
   unsigned int  dstWidth,                             /* width of destination in texels                                         */
   unsigned int  dstStartX, unsigned int dstStartY,    /* bottom left of destination region                                      */
   unsigned int  width,     unsigned int height,       /* top right of destination region                                        */
   unsigned char *src,                                 /* address of linear image                                                */
   unsigned int  srcStartX, unsigned int srcStartY,    /* bottom left of source region                                           */
   unsigned int  srcStride)                            /* stride of source image in bytes (i.e. line length including padding)   */
{
   static const unsigned int   BLOCK_SIZE  = 4096;
   static const unsigned int   BLOCK_SHIFT = 5;
   static const unsigned int   BLOCK_BITS  = 0x1f;
   static const unsigned int   TILE_WIDTH  = 16;
   static const unsigned int   TEXEL_SIZE  = 3;
   static const unsigned int   ALPHA       = 0xff000000;

   unsigned int   blockY, blockX, subY, subX;

   unsigned int   dstBlockWidth  = ((dstWidth + BLOCK_BITS) & ~BLOCK_BITS) >> BLOCK_SHIFT;  /* Width in 4k tiles */
   unsigned int   dstBlockStride = dstBlockWidth * BLOCK_SIZE;

   unsigned int   dstEndX    = dstStartX + width - 1;
   unsigned int   dstEndY    = dstStartY + height - 1;

   unsigned int   startBlockX = dstStartX >> BLOCK_SHIFT;
   unsigned int   startBlockY = dstStartY >> BLOCK_SHIFT;
   unsigned int   endBlockX   = dstEndX   >> BLOCK_SHIFT;
   unsigned int   endBlockY   = dstEndY   >> BLOCK_SHIFT;

   int   offX = srcStartX - dstStartX;
   int   offY = srcStartY - dstStartY;

   bool  aligned = (offX == 0) && (((unsigned int)src & 0x3) == 0) && ((srcStride & 0x3) == 0);

   /* For each 4k block vertically */
   for (blockY = startBlockY; blockY <= endBlockY; ++blockY)
   {
      unsigned int startSubY = blockY == startBlockY ? (dstStartY & BLOCK_BITS) : 0;
      unsigned int endSubY   = blockY == endBlockY   ? (dstEndY   & BLOCK_BITS) : BLOCK_BITS;

      unsigned int   odd  = blockY & 1;
      unsigned int   mask = odd ? 0x0c48 : 0x84c0;

      /* For each 4k block horizontally */
      for (blockX = startBlockX; blockX <= endBlockX; ++blockX)
      {
         unsigned int   startSubX = blockX == startBlockX ? (dstStartX & BLOCK_BITS) : 0;
         unsigned int   endSubX   = blockX == endBlockX   ? (dstEndX   & BLOCK_BITS) : BLOCK_BITS;
         bool           fast      = aligned && startSubX == 0 && endSubX == BLOCK_BITS;

         unsigned int   blockXR   = odd ? dstBlockWidth - blockX - 1 : blockX;

         /* Address of 32 x 32 block */
         unsigned char  *blockAddr  = dst + blockY * dstBlockStride + blockXR * BLOCK_SIZE;
         unsigned int   Y = (blockY << BLOCK_SHIFT) + offY;
         unsigned int   X = (blockX << BLOCK_SHIFT) + startSubX + offX;

         /* For each row in the sub tile */
         for (subY = startSubY; subY <= endSubY; ++subY)
         {
            unsigned char  *src_texel = src + (Y + subY) * srcStride + X * TEXEL_SIZE;

            unsigned int   offset     = mask >> ((subY & 0x10) >> 1);
            unsigned char  *rowAddr   = blockAddr + YSWIZZLE(subY);
            unsigned char  *rowAddr0  = rowAddr + ((offset & 0x0f) << 8);
            unsigned char  *rowAddr1  = rowAddr + ((offset & 0xf0) << 4);

            if (fast)
            {
               /* Fast path for middle of image */
               unsigned int   intA, intB;

               /* FIRST batch of 16 texels */
               /* Four pixels (three reads from source) */
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr0 + 0x00) = intA | ALPHA;

               intB = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr0 + 0x04) = intA >> 24 | intB << 8 | ALPHA;
               
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr0 + 0x08) = intB >> 16 | intA << 16 | ALPHA;

               *(unsigned int *)(rowAddr0 + 0x0c) = intA >> 8 | ALPHA;

               /* Four pixels (three reads from source) */
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr0 + 0x40) = intA | ALPHA;

               intB = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr0 + 0x44) = intA >> 24 | intB << 8 | ALPHA;
               
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr0 + 0x48) = intB >> 16 | intA << 16 | ALPHA;

               *(unsigned int *)(rowAddr0 + 0x4c) = intA >> 8 | ALPHA;

               /* Four pixels (three reads from source) */
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr0 + 0x80) = intA | ALPHA;

               intB = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr0 + 0x84) = intA >> 24 | intB << 8 | ALPHA;
               
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr0 + 0x88) = intB >> 16 | intA << 16 | ALPHA;

               *(unsigned int *)(rowAddr0 + 0x8c) = intA >> 8 | ALPHA;

               /* Four pixels (three reads from source) */
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr0 + 0xc0) = intA | ALPHA;

               intB = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr0 + 0xc4) = intA >> 24 | intB << 8 | ALPHA;
               
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr0 + 0xc8) = intB >> 16 | intA << 16 | ALPHA;

               *(unsigned int *)(rowAddr0 + 0xcc) = intA >> 8 | ALPHA;

               /* SECOND batch of 16 texels */
               /* Four pixels (three reads from source) */
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr1 + 0x00) = intA | ALPHA;

               intB = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr1 + 0x04) = intA >> 24 | intB << 8 | ALPHA;
               
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr1 + 0x08) = intB >> 16 | intA << 16 | ALPHA;

               *(unsigned int *)(rowAddr1 + 0x0c) = intA >> 8 | ALPHA;

               /* Four pixels (three reads from source) */
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr1 + 0x40) = intA | ALPHA;

               intB = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr1 + 0x44) = intA >> 24 | intB << 8 | ALPHA;
               
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr1 + 0x48) = intB >> 16 | intA << 16 | ALPHA;

               *(unsigned int *)(rowAddr1 + 0x4c) = intA >> 8 | ALPHA;

               /* Four pixels (three reads from source) */
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr1 + 0x80) = intA | ALPHA;

               intB = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr1 + 0x84) = intA >> 24 | intB << 8 | ALPHA;
               
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr1 + 0x88) = intB >> 16 | intA << 16 | ALPHA;

               *(unsigned int *)(rowAddr1 + 0x8c) = intA >> 8 | ALPHA;

               /* Four pixels (three reads from source) */
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr1 + 0xc0) = intA | ALPHA;

               intB = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr1 + 0xc4) = intA >> 24 | intB << 8 | ALPHA;
               
               intA = *(unsigned int *)src_texel; src_texel += 4;
               *(unsigned int *)(rowAddr1 + 0xc8) = intB >> 16 | intA << 16 | ALPHA;

               *(unsigned int *)(rowAddr1 + 0xcc) = intA >> 8 | ALPHA;
            }
            else
            {
               /* Slower path for edges of image */
               unsigned int   endSubX1 = endSubX < (TILE_WIDTH - 1) ? endSubX : TILE_WIDTH - 1;

               for (subX = startSubX; subX <= endSubX1; ++subX)
               {
                  unsigned int r = *src_texel++;
                  unsigned int g = *src_texel++;
                  unsigned int b = *src_texel++;

                  *(unsigned int *)(rowAddr0 + s_xSwizzle[subX]) = r | g << 8 | b << 16 | ALPHA;
               }

               for (subX = startSubX > TILE_WIDTH ? startSubX : TILE_WIDTH; subX <= endSubX; ++subX)
               {
                  unsigned int r = *src_texel++;
                  unsigned int g = *src_texel++;
                  unsigned int b = *src_texel++;

                  *(unsigned int *)(rowAddr1 + s_xSwizzle[subX & 0xf]) = r | g << 8 | b << 16 | ALPHA;
               }
            }
         }
      }
   }
}

#undef XSWIZZLE
#undef YSWIZZLE

void khrn_image_wrap_copy_region(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
   khrn_stats_record_start(KHRN_STATS_IMAGE);
   if ((dst->format == src->format) &&
      (dst->width == width) && (dst->height == height) &&
      (src->width == width) && (src->height == height) &&
      (dst->stride == (int32_t)khrn_image_get_stride(dst->format, width)) && /* only if the images are tightly packed... */
      (dst->stride == src->stride)) {
      vcos_assert((dst_x == 0) && (dst_y == 0) && (src_x == 0) && (src_y == 0));
      khrn_memcpy(dst->storage, src->storage, khrn_image_get_size(dst->format, width, height));
      goto done;
   }

   vcos_assert((dst_x + width) <= dst->width);
   vcos_assert((dst_y + height) <= dst->height);
   vcos_assert((src_x + width) <= src->width);
   vcos_assert((src_y + height) <= src->height);

#ifdef RSO_ARGB8888_TEXTURE
  if(src->format == ABGR_8888_RSO )
  {
		int i=0;
		unsigned char* inStorage = src->storage;
		unsigned char* outStorage = dst->storage;
		int inStride = src->stride;
		int outStride = dst->stride;
		outStorage += dst_y*outStride;
		inStorage += src_y*inStride;
		outStorage += dst_x;
		inStorage += src_x;
		for(i=0;i<height;i++)
		{
			memcpy(outStorage,inStorage,width*4);
			outStorage += dst->stride;
			inStorage += src->stride;
		}
		goto done;

  }
#endif

   if (khrn_image_is_etc1(dst->format) && khrn_image_is_etc1(src->format))
   {
      /*
         ETC1 repacking is an identical operation to RGBA32 repacking, where 4x4 ETC1 pixels are
         mapped to 2x1 ABGR pixels (64 bits in either case).
         Obviously we can't do the copy if the src/dst regions are not aligned to ETC1 blocks.
      */
      KHRN_IMAGE_WRAP_T rgba_dst, rgba_src;

      vcos_assert(!(dst_x & 3) && !(dst_y & 3) && !(src_x & 3) && !(src_y && 3));
      vcos_assert(!(width & 3) || dst_x + width == dst->width);
      vcos_assert(!(height & 3) || dst_y + height == dst->height);

      khrn_image_wrap(&rgba_dst,
         (KHRN_IMAGE_FORMAT_T)(ABGR_8888 | (dst->format & IMAGE_FORMAT_MEM_LAYOUT_MASK)),
         2*((dst->width+3)/4), (dst->height+3)/4,
         4*dst->stride, dst->storage);
      khrn_image_wrap(&rgba_src,
         (KHRN_IMAGE_FORMAT_T)(ABGR_8888 | (src->format & IMAGE_FORMAT_MEM_LAYOUT_MASK)),
         2*((src->width+3)/4), (src->height+3)/4,
         4*src->stride, src->storage);
      khrn_image_wrap_copy_region(
         &rgba_dst, dst_x/4, dst_y/4, (width+3)/4, (height+3)/4,
         &rgba_src, src_x/4, src_y/4, IMAGE_CONV_GL);
      goto done;
   }

   vcos_assert(khrn_image_is_uncomp(dst->format) && (khrn_image_is_uncomp(src->format) || (src->format == PACKED_MASK_TF)));

   if (khrn_bf_copy_region(dst, dst_x, dst_y, width, height, src, src_x, src_y, conv))
      goto done;

   /* Accelerated special cases */
   if (src->format == ABGR_8888 && dst->format == ABGR_8888_TF)
   {
      khrn_copy_rgba8888_to_tf32(dst->storage, dst->width, dst_x, dst_y, width, height, src->storage, src_x, src_y, src->stride);
      goto done;
   }
   
   if (src->format == BGR_888 && dst->format == XBGR_8888_TF)
   {
      khrn_copy_rgb888_to_tf32(dst->storage, dst->width, dst_x, dst_y, width, height, src->storage, src_x, src_y, src->stride);
      goto done;
   }
   
   if (src->format == RGB_565 && dst->format == RGB_565_TF)
   {
      khrn_copy_rgb565_to_tf16(dst->storage, dst->width, dst_x, dst_y, width, height, src->storage, src_x, src_y, src->stride);
      goto done;
   }

   khrn_stats_record_event(KHRN_STATS_UNACCELERATED_IMAGE);

   if (src->format == PACKED_MASK_TF) {
      copy_region_from_packed_mask_tf(dst, dst_x, dst_y, width, height, src, src_x, src_y);
      goto done;
   }

   copy_scissor_regions(dst, dst_x, dst_y, width, height, src, src_x, src_y, conv, NULL, 0);
done:
   khrn_stats_record_end(KHRN_STATS_IMAGE);
}

void khrn_image_wrap_copy_scissor_regions(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv,
   const int32_t *scissor_rects, uint32_t scissor_rects_count)
{
   if (scissor_rects_count == 4) {
      int32_t rect_x      = scissor_rects[0];
      int32_t rect_y      = scissor_rects[1];
      int32_t rect_width  = scissor_rects[2];
      int32_t rect_height = scissor_rects[3];
      khrn_clip_rect(
         &rect_x, &rect_y, &rect_width, &rect_height,
         dst_x, dst_y, width, height);
      khrn_image_wrap_copy_region(
         dst, rect_x, rect_y,
         rect_width, rect_height,
         src, src_x + (rect_x - dst_x), src_y + (rect_y - dst_y),
         conv);
      return;
   }

   khrn_stats_record_start(KHRN_STATS_IMAGE);

   vcos_assert(khrn_image_is_uncomp(dst->format) && khrn_image_is_uncomp(src->format));
   vcos_assert((dst_x + width) <= dst->width);
   vcos_assert((dst_y + height) <= dst->height);
   vcos_assert((src_x + width) <= src->width);
   vcos_assert((src_y + height) <= src->height);

   if (khrn_bf_copy_scissor_regions(dst, dst_x, dst_y, width, height, src, src_x, src_y, conv, scissor_rects, scissor_rects_count))
      goto done;

   khrn_stats_record_event(KHRN_STATS_UNACCELERATED_IMAGE);
   copy_scissor_regions(dst, dst_x, dst_y, width, height, src, src_x, src_y, conv, scissor_rects, scissor_rects_count);
done:
   khrn_stats_record_end(KHRN_STATS_IMAGE);
}

void khrn_image_wrap_convert(KHRN_IMAGE_WRAP_T *dst, const KHRN_IMAGE_WRAP_T *src, KHRN_IMAGE_CONV_T conv)
{
   vcos_assert((dst->width == src->width) && (dst->height == src->height));
   khrn_image_wrap_copy_region(dst, 0, 0, dst->width, dst->height, src, 0, 0, conv);
}

// This function takes two 32-bit depth/stencil images, assumed to be the same
// size, and copies the stencil channel of one onto the stencil channel of
// the other.
// TODO: make this fast! This is used when renderbuffers are recombined, so
// unlike glTexImage functions it could be considered to be on the fast path.
void khrn_image_wrap_copy_stencil_channel(KHRN_IMAGE_WRAP_T *dst, const KHRN_IMAGE_WRAP_T *src)
{
   uint32_t x, y;

   vcos_assert((dst->format == DEPTH_32_TF) && (src->format == DEPTH_32_TF));
   vcos_assert((dst->width == src->width) && (dst->height == src->height));

   for (y = 0; y != dst->height; ++y) {
      for (x = 0; x != dst->width; ++x) {
         uint32_t pixel0 = khrn_image_wrap_get_pixel(dst, x, y);
         uint32_t pixel1 = khrn_image_wrap_get_pixel(src, x, y);
         khrn_image_wrap_put_pixel(dst, x, y, (pixel0 & 0x00ffffff) | (pixel1 & 0xff000000));
      }
   }
}

static uint32_t blend(KHRN_IMAGE_FORMAT_T format, uint32_t x, uint32_t y)
{
   vcos_assert(khrn_image_is_color(format));

   switch (format & (IMAGE_FORMAT_PIXEL_SIZE_MASK | IMAGE_FORMAT_PIXEL_LAYOUT_MASK)) {
   case (IMAGE_FORMAT_32 | IMAGE_FORMAT_8888):
   {
      uint32_t a = ((x >> 24) + (y >> 24)) >> 1;
      uint32_t b = ((x >> 16 & 0xff) + (y >> 16 & 0xff)) >> 1;
      uint32_t g = ((x >> 8 & 0xff) + (y >> 8 & 0xff)) >> 1;
      uint32_t r = ((x & 0xff) + (y & 0xff)) >> 1;

      return a << 24 | b << 16 | g << 8 | r;
   }
   /* Miss out BGR888: currently no need to blend this type */
   case (IMAGE_FORMAT_16 | IMAGE_FORMAT_4444):
   {
      uint32_t r, g, b, a;

      vcos_assert(x < 65536);
      vcos_assert(y < 65536);

      r = ((x >> 12) + (y >> 12)) >> 1;
      g = ((x >> 8 & 0xf) + (y >> 8 & 0xf)) >> 1;
      b = ((x >> 4 & 0xf) + (y >> 4 & 0xf)) >> 1;
      a = ((x & 0xf) + (y & 0xf)) >> 1;

      return r << 12 | g << 8 | b << 4 | a;
   }
   case (IMAGE_FORMAT_16 | IMAGE_FORMAT_5551):
   {
      uint32_t r, g, b, a;

      vcos_assert(x < 65536);
      vcos_assert(y < 65536);

      vcos_assert(format & IMAGE_FORMAT_XA);

      r = ((x >> 11) + (y >> 11)) >> 1;
      g = ((x >> 6 & 0x1f) + (y >> 6 & 0x1f)) >> 1;
      b = ((x >> 1 & 0x1f) + (y >> 1 & 0x1f)) >> 1;
      a = ((x & 0x1) + (y & 0x1)) >> 1;

      return r << 11 | g << 6 | b << 1 | a;
   }
   case (IMAGE_FORMAT_16 | IMAGE_FORMAT_565):
   {
      uint32_t r, g, b;

      vcos_assert(x < 65536);
      vcos_assert(y < 65536);

      r = ((x >> 11) + (y >> 11)) >> 1;
      g = ((x >> 5 & 0x3f) + (y >> 5 & 0x3f)) >> 1;
      b = ((x & 0x1f) + (y & 0x1f)) >> 1;

      return r << 11 | g << 5 | b;
   }
   case (IMAGE_FORMAT_16 | IMAGE_FORMAT_88):
   {
      uint32_t a, l;

      vcos_assert(x < 65536);
      vcos_assert(y < 65536);

      a = ((x >> 8) + (y >> 8)) >> 1;
      l = ((x & 0xff) + (y & 0xff)) >> 1;

      return a << 8 | l;
   }
   case IMAGE_FORMAT_8:
   case IMAGE_FORMAT_4:
   case IMAGE_FORMAT_1:
   {
      return (x + y) >> 1;
   }
   default:
   {
      UNREACHABLE();
      return 0;
   }
   }
}

void khrn_image_wrap_subsample(KHRN_IMAGE_WRAP_T *dst, const KHRN_IMAGE_WRAP_T *src)
{
   uint32_t x, y;

   khrn_stats_record_start(KHRN_STATS_IMAGE);

   vcos_assert(!((dst->format ^ src->format) & ~IMAGE_FORMAT_MEM_LAYOUT_MASK & ~IMAGE_FORMAT_PRE));
   vcos_assert(khrn_image_is_color(src->format));
   vcos_assert(dst->width == _max(src->width >> 1, 1));
   vcos_assert(dst->height == _max(src->height >> 1, 1));

   if (khrn_bf_subsample(dst, src))
      goto done;

   khrn_stats_record_event(KHRN_STATS_UNACCELERATED_IMAGE);
   for (y = 0; y != dst->height; ++y) {
      for (x = 0; x != dst->width; ++x) {
         uint32_t sx0 = x << 1;
         uint32_t sy0 = y << 1;
         uint32_t sx1 = _min(sx0 + 1, (uint32_t)src->width - 1);
         uint32_t sy1 = _min(sy0 + 1, (uint32_t)src->height - 1);

         uint32_t p0 = khrn_image_wrap_get_pixel(src, sx0, sy0);
         uint32_t p1 = khrn_image_wrap_get_pixel(src, sx1, sy0);
         uint32_t p2 = khrn_image_wrap_get_pixel(src, sx0, sy1);
         uint32_t p3 = khrn_image_wrap_get_pixel(src, sx1, sy1);

         uint32_t p = blend(src->format, blend(src->format, p0, p1), blend(src->format, p2, p3));

         khrn_image_wrap_put_pixel(dst, x, y, p);
      }
   }
done:
   khrn_stats_record_end(KHRN_STATS_IMAGE);
}

void khrn_image_clear_region(
   KHRN_IMAGE_T *image, uint32_t x, uint32_t y,
   uint32_t width, uint32_t height,
   uint32_t rgba, /* non-lin, unpre */
   KHRN_IMAGE_CONV_T conv)
{
   KHRN_IMAGE_WRAP_T wrap;
   khrn_image_lock_wrap(image, &wrap);
   khrn_image_wrap_clear_region(&wrap, x, y, width, height, rgba, conv);
   khrn_image_unlock_wrap(image);
}

void khrn_image_copy_region(
   KHRN_IMAGE_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
   KHRN_IMAGE_WRAP_T dst_wrap, src_wrap;
   khrn_image_lock_wrap(dst, &dst_wrap);
   khrn_image_lock_wrap(src, &src_wrap);
   khrn_image_wrap_copy_region(&dst_wrap, dst_x, dst_y, width, height, &src_wrap, src_x, src_y, conv);
   khrn_image_unlock_wrap(src);
   khrn_image_unlock_wrap(dst);
}

void khrn_image_convert(KHRN_IMAGE_T *dst, const KHRN_IMAGE_T *src, KHRN_IMAGE_CONV_T conv)
{
   vcos_assert((dst->width == src->width) && (dst->height == src->height));
   khrn_image_copy_region(dst, 0, 0, dst->width, dst->height, src, 0, 0, conv);
}

void khrn_image_copy_stencil_channel(KHRN_IMAGE_T *dst, const KHRN_IMAGE_T *src)
{
   KHRN_IMAGE_WRAP_T dst_wrap, src_wrap;
   khrn_image_lock_wrap(dst, &dst_wrap);
   khrn_image_lock_wrap(src, &src_wrap);
   khrn_image_wrap_copy_stencil_channel(&dst_wrap, &src_wrap);
   khrn_image_unlock_wrap(src);
   khrn_image_unlock_wrap(dst);
}

void khrn_image_subsample(KHRN_IMAGE_T *dst, const KHRN_IMAGE_T *src)
{
   KHRN_IMAGE_WRAP_T dst_wrap, src_wrap;
   khrn_image_lock_wrap(dst, &dst_wrap);
   khrn_image_lock_wrap(src, &src_wrap);
   khrn_image_wrap_subsample(&dst_wrap, &src_wrap);
   khrn_image_unlock_wrap(src);
   khrn_image_unlock_wrap(dst);
}

bool khrn_image_alloc_storage(KHRN_IMAGE_T *image, const char *description)
{
   uint32_t storage_size;
   MEM_HANDLE_T handle;

   if (image->mh_storage == MEM_INVALID_HANDLE)
   {
   	  KHRN_IMAGE_FORMAT_T format = image->format;
	    uint32_t padded_width = image->width, padded_height = image->height, align = DEFAULT_ALIGN, stagger = 0;

	    khrn_image_platform_fudge(&format, &padded_width, &padded_height, &align, &stagger, (KHRN_IMAGE_CREATE_FLAG_T)(
	       ((image->flags & IMAGE_FLAG_TEXTURE) ? IMAGE_CREATE_FLAG_TEXTURE : 0) |
	       ((image->flags & IMAGE_FLAG_RENDER_TARGET) ? IMAGE_CREATE_FLAG_RENDER_TARGET : 0) |
	       ((image->flags & IMAGE_FLAG_DISPLAY) ? IMAGE_CREATE_FLAG_DISPLAY : 0)));

      storage_size = khrn_image_get_size(image->format, padded_width, padded_height);

      /* TODO: I'm ignoring stagger/rotate here (see khrn_image_create calculation of storage_size) */
      handle = mem_alloc_ex(storage_size, 4096,
         (MEM_FLAG_T)(MEM_FLAG_DIRECT | MEM_FLAG_NO_INIT | MEM_FLAG_RESIZEABLE),
         description,
         MEM_COMPACT_DISCARD);

      if (handle == MEM_INVALID_HANDLE)
         return false;

      MEM_ASSIGN(image->mh_storage, handle);
      mem_release(handle);

      /* Fill image with known colour rather than snow */
      if (khrn_image_is_color(image->format) && image->format != COL_32_TLBD)
      	khrn_image_clear_region(image, 0, 0, image->width, image->height, 0xff0080ff, IMAGE_CONV_GL);
   }
   return true;
}

