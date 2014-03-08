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
#include "middleware/khronos/ext/egl_brcm_global_image.h"
#include "middleware/khronos/egl/egl_server.h"
#include "middleware/khronos/common/khrn_image.h"
#include "middleware/khronos/common/khrn_map_64.h"
#include "interface/khronos/include/EGL/egl.h"
#include "interface/khronos/include/EGL/eglext.h"

#if EGL_BRCM_global_image

extern void egl_brcm_global_image_id_get(int32_t *id); /* see egl_brcm_global_image_id.c */

static KHRN_MAP_64_T map;

bool egl_brcm_global_image_init(void)
{
   return khrn_map_64_init(&map, 8);
}

void egl_brcm_global_image_term(void)
{
   khrn_map_64_term(&map);
}

bool egl_brcm_global_image_is_empty(void)
{
   return khrn_map_64_get_count(&map) == 0;
}

MEM_HANDLE_T egl_brcm_global_image_lookup(uint32_t id_0, uint32_t id_1)
{
   return khrn_map_64_lookup(&map, id_0 | ((uint64_t)id_1 << 32));
}

typedef struct {
   EGL_BRCM_GLOBAL_IMAGE_ITERATE_CALLBACK_T callback;
   void *p;
} CALLBACK_P_T;

static void forward_callback(KHRN_MAP_64_T *map, uint64_t key, MEM_HANDLE_T value, void *p)
{
   CALLBACK_P_T *callback_p = (CALLBACK_P_T *)p;

   UNUSED(map);
   UNUSED(key);

   callback_p->callback(value, callback_p->p);
}

void egl_brcm_global_image_iterate(EGL_BRCM_GLOBAL_IMAGE_ITERATE_CALLBACK_T callback, void *p)
{
   CALLBACK_P_T callback_p;
   callback_p.callback = callback;
   callback_p.p = p;
   khrn_map_64_iterate(&map, forward_callback, &callback_p);
}

static KHRN_IMAGE_FORMAT_T get_internal_format(uint32_t pixel_format, uint32_t width, uint32_t height)
{
   KHRN_IMAGE_FORMAT_T internal_format;
   switch (pixel_format & ~EGL_PIXEL_FORMAT_USAGE_MASK_BRCM) {
   case EGL_PIXEL_FORMAT_ARGB_8888_PRE_BRCM: internal_format = (KHRN_IMAGE_FORMAT_T)(ABGR_8888_TF | IMAGE_FORMAT_PRE); break;
   case EGL_PIXEL_FORMAT_ARGB_8888_BRCM:     internal_format = ABGR_8888_TF; break;
   case EGL_PIXEL_FORMAT_XRGB_8888_BRCM:     internal_format = XBGR_8888_TF; break;
   case EGL_PIXEL_FORMAT_RGB_565_BRCM:       internal_format = RGB_565_TF; break;
   case EGL_PIXEL_FORMAT_A_8_BRCM:           internal_format = A_8_TF; break;
   default:                                  UNREACHABLE(); internal_format = (KHRN_IMAGE_FORMAT_T)0; /* check performed on client */
   }
   if (khrn_image_prefer_lt(internal_format, width, height)) {
      internal_format = khrn_image_to_lt_format(internal_format);
   }
   return internal_format;
}

static KHRN_IMAGE_FORMAT_T get_fill_format(uint32_t pixel_format)
{
   switch (pixel_format & ~EGL_PIXEL_FORMAT_USAGE_MASK_BRCM) {
   case EGL_PIXEL_FORMAT_ARGB_8888_PRE_BRCM: return (KHRN_IMAGE_FORMAT_T)(ARGB_8888_RSO | IMAGE_FORMAT_PRE);
   case EGL_PIXEL_FORMAT_ARGB_8888_BRCM:     return ARGB_8888_RSO;
   case EGL_PIXEL_FORMAT_XRGB_8888_BRCM:     return XRGB_8888_RSO;
   case EGL_PIXEL_FORMAT_RGB_565_BRCM:       return RGB_565_RSO;
   case EGL_PIXEL_FORMAT_A_8_BRCM:           return A_8_RSO;
   default:                                  UNREACHABLE(); return (KHRN_IMAGE_FORMAT_T)0;
   }
}

static uint32_t get_external_format(KHRN_IMAGE_FORMAT_T format)
{
   switch (format & ~IMAGE_FORMAT_MEM_LAYOUT_MASK) {
   case (ABGR_8888 | IMAGE_FORMAT_PRE): return EGL_PIXEL_FORMAT_ARGB_8888_PRE_BRCM;
   case ABGR_8888:                      return EGL_PIXEL_FORMAT_ARGB_8888_BRCM;
   case XBGR_8888:                      return EGL_PIXEL_FORMAT_XRGB_8888_BRCM;
   case RGB_565:                        return EGL_PIXEL_FORMAT_RGB_565_BRCM;
   case A_8:                            return EGL_PIXEL_FORMAT_A_8_BRCM;
   default:                             UNREACHABLE(); return 0; /* shouldn't have global images of any other type */
   }
}

void eglCreateGlobalImageBRCM_impl(EGLint width, EGLint height, EGLint pixel_format, EGLint *id)
{
   MEM_HANDLE_T handle;

   /*
      create the image
   */

   handle = khrn_image_create(get_internal_format(pixel_format, width, height), width, height,
      (KHRN_IMAGE_CREATE_FLAG_T)(IMAGE_CREATE_FLAG_ONE |
      ((pixel_format & EGL_PIXEL_FORMAT_RENDER_MASK_BRCM) ? IMAGE_CREATE_FLAG_RENDER_TARGET : 0) |
      ((pixel_format & EGL_PIXEL_FORMAT_TEXTURE_MASK_BRCM) ? IMAGE_CREATE_FLAG_TEXTURE : 0)));
   if (handle == MEM_INVALID_HANDLE) {
      id[0] = 0; id[1] = 0;
      return;
   }
   /* IMAGE_FLAG_BOUND_EGLIMAGE won't prevent us from creating a pixmap surface
    * or an egl image from the global image (egl image creation for global
    * images is dealt with specially), but it will prevent further unpermitted
    * operations, eg eglCreateImageKHR(vgCreateEGLImageTargetKHR(egl_image)) or
    * eglCreatePbufferFromClientBuffer(vgCreateEGLImageTargetKHR(egl_image)) */
   ((KHRN_IMAGE_T *)mem_lock(handle))->flags |= IMAGE_FLAG_BOUND_EGLIMAGE;
   mem_unlock(handle);

   /*
      grab the next available id and map it to the image
   */

   egl_brcm_global_image_id_get(id);
   if (!khrn_map_64_insert(&map, id[0] | ((uint64_t)id[1] << 32), handle)) {
      id[0] = 0; id[1] = 0;
   }
   mem_release(handle);
}

void eglFillGlobalImageBRCM_impl(EGLint id_0, EGLint id_1, EGLint y, EGLint height, const void *data, EGLint data_stride, EGLint data_pixel_format)
{
   KHRN_IMAGE_T *image;
   KHRN_IMAGE_WRAP_T dst, src;

   /*
      find the image
   */

   MEM_HANDLE_T handle = khrn_map_64_lookup(&map, id_0 | ((uint64_t)id_1 << 32));
   if (handle == MEM_INVALID_HANDLE) {
      UNREACHABLE(); /* we shouldn't get any invalid ids here... */
      return;
   }

   /*
      do the fillin'
   */

   image = (KHRN_IMAGE_T *)mem_lock(handle);
   khrn_image_lock_wrap(image, &dst);
   khrn_image_wrap(&src, get_fill_format(data_pixel_format), image->width, height, data_stride, (void *)data); /* casting away const, but not going to modify */
   khrn_image_wrap_copy_region(&dst, 0, y, image->width, height, &src, 0, 0, IMAGE_CONV_GL);
   khrn_image_unlock_wrap(image);
   mem_unlock(handle);
}

void eglCreateCopyGlobalImageBRCM_impl(EGLint src_id_0, EGLint src_id_1, EGLint *id)
{
   MEM_HANDLE_T src_handle, handle;

   /*
      find the source image
   */

   src_handle = khrn_map_64_lookup(&map, src_id_0 | ((uint64_t)src_id_1 << 32));
   if (src_handle == MEM_INVALID_HANDLE) {
      id[0] = -1; id[1] = -1;
      return;
   }

   /*
      create a copy
   */

   handle = khrn_image_create_dup((KHRN_IMAGE_T *)mem_lock(src_handle), IMAGE_CREATE_FLAG_NONE);
   mem_unlock(src_handle);
   if (handle == MEM_INVALID_HANDLE) {
      id[0] = 0; id[1] = 0;
      return;
   }
   ((KHRN_IMAGE_T *)mem_lock(handle))->flags |= IMAGE_FLAG_BOUND_EGLIMAGE; /* see eglCreateGlobalImageBRCM_impl */
   mem_unlock(handle);

   /*
      grab the next available id and map it to the copy
   */

   egl_brcm_global_image_id_get(id);
   if (!khrn_map_64_insert(&map, id[0] | ((uint64_t)id[1] << 32), handle)) {
      id[0] = 0; id[1] = 0;
   }
   mem_release(handle);
}

bool eglDestroyGlobalImageBRCM_impl(EGLint id_0, EGLint id_1)
{
   return khrn_map_64_delete(&map, id_0 | ((uint64_t)id_1 << 32));
}

bool eglQueryGlobalImageBRCM_impl(EGLint id_0, EGLint id_1, EGLint *width_height_pixel_format)
{
   MEM_HANDLE_T handle;
   KHRN_IMAGE_T *image;

   handle = khrn_map_64_lookup(&map, id_0 | ((uint64_t)id_1 << 32));
   if (handle == MEM_INVALID_HANDLE) {
      return false;
   }

   image = (KHRN_IMAGE_T *)mem_lock(handle);
   width_height_pixel_format[0] = image->width;
   width_height_pixel_format[1] = image->height;
   width_height_pixel_format[2] = get_external_format(image->format);
   mem_unlock(handle);
   return true;
}

#endif
