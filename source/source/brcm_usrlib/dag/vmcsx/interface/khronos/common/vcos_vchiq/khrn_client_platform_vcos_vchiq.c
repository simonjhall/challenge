/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  chip driver
File     :  $RCSfile: $
Revision :  $Revision: $

Khronos - VCOS platform support code
=============================================================================*/

#include "interface/khronos/common/khrn_client_platform.h"
#include "interface/khronos/common/khrn_client.h"
#include <stdio.h>
#include <string.h>

#ifdef KHRONOS_CLIENT_LOGGING
FILE *xxx_vclog = NULL;

void khronos_client_log(const char *fmt, ...)
{
   va_list ap;
   va_start(ap,fmt);
   vprintf(fmt, ap);
   fflush(stdout);
   if(xxx_vclog!=NULL)
   {
      vfprintf(xxx_vclog, fmt, ap);
      fflush(xxx_vclog);
   }
   va_end(ap);
}

#endif

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

static int process_attached = 0;

void *platform_tls_get(PLATFORM_TLS_T tls)
{
   void *ret;

   if (!process_attached)
      /* TODO: this isn't thread safe */
   {
#ifdef KHRONOS_CLIENT_LOGGING
      char filename[100];
      sprintf(filename, "/var/log/vclog%d.txt", (uint32_t)vcos_process_id_current());
      xxx_vclog = fopen(filename, "w");
      if (xxx_vclog == NULL)
         printf("Can't open '%s' to write - %s (rc %d)\n",
                filename, strerror(errno), errno);
      else {
         fprintf(xxx_vclog, "Attaching process\n");
         fflush(xxx_vclog);
      }
#endif
      client_process_attach();
      process_attached = 1;
      tls = client_tls;
      
      vc_vchi_khronos_init();
   }

   ret = vcos_tls_get(tls);
   if (!ret)
   {
     /* The problem here is that on VCFW, the first notification we get that a thread
       * exists at all is when it calls an arbitrary EGL function. We need to detect this
       * case and initiliase the per-thread state.
       *
       * On Windows this gets done in DllMain.
       */
      client_thread_attach();
      ret = vcos_tls_get(tls);
   }
   return ret;
}

/* ----------------------------------------------------------------------
 * workaround for broken platforms which don't detect threads exiting
 * -------------------------------------------------------------------- */
void platform_hint_thread_finished()
{
   //We don't need this hint as thread exit should be detected on Windows
}

#ifndef KHRN_PLATFORM_VCOS_NO_MALLOC

/**
   Allocate memory

   @param size Size in bytes of memory block to allocate
   @return pointer to memory block
**/
void *khrn_platform_malloc(size_t size, const char * name)
{
   return vcos_malloc(size, name);
}

/**
   Free memory

   @param v Pointer to  memory area to free
**/
void khrn_platform_free(void *v)
{
   if (v)
   {
      vcos_free(v);
   }
}

#endif

bool platform_get_pixmap_info(EGLNativePixmapType pixmap, KHRN_IMAGE_WRAP_T *image)
{
   return 0;
}

void platform_get_pixmap_server_handle(EGLNativePixmapType pixmap, uint32_t *handle)
{
}

bool platform_match_pixmap_api_support(EGLNativePixmapType pixmap, uint32_t api_support)
{
   return 0;
}

#if EGL_BRCM_global_image && EGL_KHR_image

bool platform_use_global_image_as_egl_image(uint32_t id_0, uint32_t id_1, EGLNativePixmapType pixmap, EGLint *error)
{
   vcos_assert(0);
   return false;
}

void platform_acquire_global_image(uint32_t id_0, uint32_t id_1)
{
   vcos_assert(0);
}

void platform_release_global_image(uint32_t id_0, uint32_t id_1)
{
   vcos_assert(0);
}

void platform_get_global_image_info(uint32_t id_0, uint32_t id_1,
   uint32_t *pixel_format, uint32_t *width, uint32_t *height)
{
   vcos_assert(0);
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

void platform_init_rpc(struct CLIENT_THREAD_STATE *state) 
{
   vcos_assert(1);
}

void platform_term_rpc(struct CLIENT_THREAD_STATE *state) 
{
   vcos_assert(1);
}

void platform_maybe_free_process(void) 
{
   vcos_assert(1);
}

void platform_destroy_winhandle(void *a, uint32_t b) 
{
   vcos_assert(1);
}

void platform_surface_update(uint32_t handle) 
{
   vcos_assert(1);
}

void egl_gce_win_change_image(void) 
{
   vcos_assert(0);
}

void platform_retrieve_pixmap_completed(EGLNativePixmapType pixmap) 
{
   vcos_assert(0);
}

void platform_send_pixmap_completed(EGLNativePixmapType pixmap) 
{
   vcos_assert(0);
}

uint32_t platform_memcmp(const void * aLeft, const void * aRight, size_t aLen) 
{ 
   return memcmp(aLeft, aRight, aLen); 
}

void platform_memcpy(void * aTrg, const void * aSrc, size_t aLength) 
{ 
   memcpy(aTrg, aSrc, aLength); 
}

void khrn_platform_bind_pixmap_to_egl_image(EGLNativePixmapType pixmap, EGLImageKHR egl_image, bool send) { UNREACHABLE(); }
void khrn_platform_unbind_pixmap_from_egl_image(EGLImageKHR egl_image) {  }

#ifdef ANDROID
EGLDisplay khrn_platform_set_display_id(EGLNativeDisplayType display_id)
{
   if (display_id == EGL_DEFAULT_DISPLAY)
      return (EGLDisplay)1;
   else
      return EGL_NO_DISPLAY;
}

uint32_t khrn_platform_get_window_position(EGLNativeWindowType win)
{
   return 0;
}

void khrn_platform_release_pixmap_info(EGLNativePixmapType pixmap, KHRN_IMAGE_WRAP_T *image)
{
   /* Nothing to do */
}
#endif // ANDROID

