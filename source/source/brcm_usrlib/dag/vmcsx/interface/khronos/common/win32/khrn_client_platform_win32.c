#include "interface/khronos/common/khrn_int_common.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "interface/khronos/common/khrn_client_platform.h"
#include "interface/khronos/common/khrn_client.h"

#include "interface/khronos/include/EGL/egl.h"
#include "interface/khronos/include/EGL/eglext.h" /* for global image pixel formats */

VCOS_STATUS_T khronos_platform_semaphore_create(PLATFORM_SEMAPHORE_T *sem, int name[3], int count)
{
   char buf[64];
   vcos_snprintf(buf,sizeof(buf),"KhanSemaphore%08x%08x%08x", name[0], name[1], name[2]);
   return vcos_named_semaphore_create(sem, buf, count);
}

uint64_t khronos_platform_get_process_id()
{
   return vcos_process_id_current();
}

void *platform_tls_get(PLATFORM_TLS_T tls)
{
   return vcos_tls_get(tls);
}

void *platform_tls_get_check(PLATFORM_TLS_T tls)
{
   return vcos_tls_get(tls);
}

/* ----------------------------------------------------------------------
 * workaround for broken platforms which don't detect threads exiting
 * -------------------------------------------------------------------- */
void platform_hint_thread_finished()
{
   //We don't need this hint as thread exit should be detected on Windows
}

/*
   want to be able to call from DllMain, so use HeapAlloc/HeapFree from kernel32
   instead of malloc/free from crt (if the crt isn't statically linked, calling
   malloc/free in DllMain can cause bad things to happen)
*/

void *khrn_platform_malloc(size_t size, const char *desc)
{
   return HeapAlloc(GetProcessHeap(), 0, size);
}

void khrn_platform_free(void *v)
{
   if (v) {
      HeapFree(GetProcessHeap(), 0, v);
   }
}

#if EGL_BRCM_global_image

/*
   we support global image pixmaps only. EGLNativePixmapType is a uint32_t *:
   pixmap[0] = global image id 0
   pixmap[1] = global image id 1
   pixmap[2] = width
   pixmap[3] = height
   pixmap[4] = pixel format
*/

static KHRN_IMAGE_FORMAT_T convert_format(uint32_t format)
{
   switch (format & ~EGL_PIXEL_FORMAT_USAGE_MASK_BRCM) {
   case EGL_PIXEL_FORMAT_ARGB_8888_PRE_BRCM: return (KHRN_IMAGE_FORMAT_T)(ABGR_8888 | IMAGE_FORMAT_PRE);
   case EGL_PIXEL_FORMAT_ARGB_8888_BRCM:     return ABGR_8888;
   case EGL_PIXEL_FORMAT_XRGB_8888_BRCM:     return XBGR_8888;
   case EGL_PIXEL_FORMAT_RGB_565_BRCM:       return RGB_565;
   case EGL_PIXEL_FORMAT_A_8_BRCM:           return A_8;
   default:                                  vcos_verify(0); return (KHRN_IMAGE_FORMAT_T)0;
   }
}

bool platform_get_pixmap_info(EGLNativePixmapType pixmap, KHRN_IMAGE_WRAP_T *image)
{
   image->format = convert_format(((uint32_t *)pixmap)[4]);
   image->width = ((uint32_t *)pixmap)[2];
   image->height = ((uint32_t *)pixmap)[3];

   /* can't actually access data */
   image->stride = 0;
   image->aux = 0;
   image->storage = 0;

   return image->format != 0;
}

void platform_get_pixmap_server_handle(EGLNativePixmapType pixmap, uint32_t *handle)
{
   handle[0] = ((uint32_t *)pixmap)[0];
   handle[1] = ((uint32_t *)pixmap)[1];
}

bool platform_match_pixmap_api_support(EGLNativePixmapType pixmap, uint32_t api_support)
{
   return
      (!(((uint32_t *)pixmap)[4] & EGL_PIXEL_FORMAT_RENDER_GL_BRCM) == !(api_support & EGL_OPENGL_BIT)) &&
      (!(((uint32_t *)pixmap)[4] & EGL_PIXEL_FORMAT_RENDER_GLES_BRCM) == !(api_support & EGL_OPENGL_ES_BIT)) &&
      (!(((uint32_t *)pixmap)[4] & EGL_PIXEL_FORMAT_RENDER_GLES2_BRCM) == !(api_support & EGL_OPENGL_ES2_BIT)) &&
      (!(((uint32_t *)pixmap)[4] & EGL_PIXEL_FORMAT_RENDER_VG_BRCM) == !(api_support & EGL_OPENVG_BIT));
}

#if EGL_KHR_image

/* this stuff is just for basic testing of the global image stuff, so i've kept
 * it simple instead of making it fast/robust */

#define GLOBAL_IMAGES_N 32

static uint32_t global_images[GLOBAL_IMAGES_N * 2] = {0};

bool platform_use_global_image_as_egl_image(uint32_t id_0, uint32_t id_1, EGLNativePixmapType pixmap, EGLint *error)
{
   /* should also report EGL_BAD_PARAMETER if the flags passed to
    * eglCreateGlobalImageBRCM didn't include EGL_PIXEL_FORMAT_VG_IMAGE_BRCM,
    * EGL_PIXEL_FORMAT_GLES_TEXTURE_BRCM, or EGL_PIXEL_FORMAT_GLES2_TEXTURE_BRCM */

   uint32_t i;
   for (i = 0; i != (GLOBAL_IMAGES_N * 2); i += 2) {
      if ((global_images[i] == id_0) && (global_images[i + 1] == id_1)) {
         *error = EGL_BAD_ACCESS;
         return false;
      }
   }
   return true;
}

void platform_acquire_global_image(uint32_t id_0, uint32_t id_1)
{
   uint32_t i;
   for (i = 0; i != (GLOBAL_IMAGES_N * 2); i += 2) {
      if (!global_images[i] && !global_images[i + 1]) {
         global_images[i] = id_0;
         global_images[i + 1] = id_1;
         return;
      }
   }
   vcos_verify(0); /* increase GLOBAL_IMAGES_N... */
}

void platform_release_global_image(uint32_t id_0, uint32_t id_1)
{
   uint32_t i;
   for (i = 0; i != (GLOBAL_IMAGES_N * 2); i += 2) {
      if ((global_images[i] == id_0) && (global_images[i + 1] == id_1)) {
         global_images[i] = 0;
         global_images[i + 1] = 0;
         return;
      }
   }
   vcos_verify(0); /* not found... */
}

void platform_get_global_image_info(uint32_t id_0, uint32_t id_1,
   uint32_t *pixel_format, uint32_t *width, uint32_t *height)
{
   EGLint id[2] = {id_0, id_1};
   EGLint width_height_pixel_format[3];
   verify(eglQueryGlobalImageBRCM(id, width_height_pixel_format));
   width_height_pixel_format[2] |=
      /* this isn't right (the flags should be those passed in to
       * eglCreateGlobalImageBRCM), but this stuff is just for basic testing, so
       * it doesn't really matter */
      EGL_PIXEL_FORMAT_RENDER_GLES_BRCM | EGL_PIXEL_FORMAT_RENDER_GLES2_BRCM |
      EGL_PIXEL_FORMAT_RENDER_VG_BRCM | EGL_PIXEL_FORMAT_VG_IMAGE_BRCM |
      EGL_PIXEL_FORMAT_GLES_TEXTURE_BRCM | EGL_PIXEL_FORMAT_GLES2_TEXTURE_BRCM;
   if (pixel_format) { *pixel_format = width_height_pixel_format[2]; }
   if (width) { *width = width_height_pixel_format[0]; }
   if (height) { *height = width_height_pixel_format[1]; }
}

#endif

#else

bool platform_get_pixmap_info(EGLNativePixmapType pixmap, IMAGE_WRAP_T *image)
{
   return false;
}

void platform_get_pixmap_server_handle(EGLNativePixmapType pixmap, uint32_t *handle)
{
}

bool platform_match_pixmap_api_support(EGLNativePixmapType pixmap, uint32_t api_support)
{
   return true;
}

#endif

void platform_client_lock(void)
{
   platform_mutex_acquire(&client_mutex);
}

void platform_client_release(void)
{
   platform_mutex_release(&client_mutex);
}

void platform_init_rpc(struct CLIENT_THREAD_STATE *state) { }
void platform_term_rpc(struct CLIENT_THREAD_STATE *state) { }
void platform_maybe_free_process(void) { }
void platform_destroy_winhandle(void *a, uint32_t b) { }

void platform_surface_update(uint32_t handle) { }
void egl_gce_win_change_image(void) { UNREACHABLE(); }


void platform_retrieve_pixmap_completed(EGLNativePixmapType pixmap) { UNREACHABLE(); }
void platform_send_pixmap_completed(EGLNativePixmapType pixmap) { UNREACHABLE(); }
uint32_t platform_memcmp(const void * aLeft, const void * aRight, size_t aLen) { return memcmp(aLeft, aRight, aLen); }
void platform_memcpy(void * aTrg, const void * aSrc, size_t aLength) { memcpy(aTrg, aSrc, aLength); }

EGLDisplay khrn_platform_set_display_id(EGLNativeDisplayType display_id)
{
	if (display_id == EGL_DEFAULT_DISPLAY)
      return (EGLDisplay)1;
   else
      return EGL_NO_DISPLAY;
}

static int xxx_position = 0;
uint32_t khrn_platform_get_window_position(EGLNativeWindowType win)
{
   return xxx_position;
}

void khrn_platform_release_pixmap_info(EGLNativePixmapType pixmap, KHRN_IMAGE_WRAP_T *image){}
void khrn_platform_bind_pixmap_to_egl_image(EGLNativePixmapType pixmap, EGLImageKHR egl_image, bool send) { UNREACHABLE(); }
void khrn_platform_unbind_pixmap_from_egl_image(EGLImageKHR egl_image) {  }
