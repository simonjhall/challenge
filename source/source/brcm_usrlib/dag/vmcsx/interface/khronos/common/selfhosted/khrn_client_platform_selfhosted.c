/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "interface/khronos/common/khrn_int_common.h"
#include "interface/khronos/common/khrn_client_platform.h"
#include "interface/khronos/common/khrn_client.h"
#include "interface/khronos/common/khrn_client_rpc.h"
#include "middleware/khronos/egl/egl_server.h"
#include "helpers/vc_image/vc_image.h"
#include "interface/vcos/vcos.h"
#include "middleware/khronos/common/khrn_hw.h"
#include "middleware/khronos/dispatch/khrn_dispatch_rpc.h" /* khdispatch_send_async prototype */
#include "interface/khronos/common/khrn_int_ids.h"

#ifdef KHRONOS_CLIENT_LOGGING
FILE *xxx_vclog = NULL;

void khronos_client_log(const char *fmt, ...)
{
   va_list ap;
   va_start(ap,fmt);
#if 1
   vprintf(fmt, ap);
   fflush(stdout);
#endif
   if(xxx_vclog!=NULL)
   {
      vfprintf(xxx_vclog, fmt, ap);
      fflush(xxx_vclog);
   }
   va_end(ap);
}

#endif

#ifdef BRCM_V3D_OPT
#include <pthread.h>
#endif

#ifdef RPC_LIBRARY
#include "applications/vmcs/khronos/khronos_server.h"
static bool process_attached = false;
#endif

#ifdef RPC_DIRECT_MULTI
void khdispatch_send_async(uint32_t command, uint64_t pid, uint32_t sem)
{
   if (sem != KHRN_NO_SEMAPHORE) {
      int name[3];

      name[0] = (int)pid; name[1] = (int)(pid >> 32); name[2] = (int)sem;

      if (command == ASYNC_COMMAND_DESTROY) {
         /* todo: destroy */
      } else {
         PLATFORM_SEMAPHORE_T s;
         if (khronos_platform_semaphore_create(&s, name, 1) == KHR_SUCCESS) {
            switch (command) {
            case ASYNC_COMMAND_WAIT:
               /* todo: i don't understand what ASYNC_COMMAND_WAIT is for, so this
                * might be completely wrong */
               khronos_platform_semaphore_acquire(&s);
               break;
            case ASYNC_COMMAND_POST:
               khronos_platform_semaphore_release(&s);
               break;
            default:
               UNREACHABLE();
            }
            khronos_platform_semaphore_destroy(&s);
         }
      }
   }
}
#endif

#ifdef CLIENT_THREAD_IS_PROCESS
void *platform_tls_get_process(PLATFORM_TLS_T tls)
{

	void *ret;
   ret = vcos_tls_get(tls);
   if (!ret)
   {
		//if client_process_state is not allocaed and initialized, do it
		CLIENT_PROCESS_STATE_T *process = (CLIENT_PROCESS_STATE_T *)khrn_platform_malloc(sizeof(CLIENT_PROCESS_STATE_T), "CLIENT_PROCESS_STATE_T");
		memset(process,0,sizeof(CLIENT_PROCESS_STATE_T));
		
		if (!process)
			return NULL;
		
		client_process_state_init(process);
		vcos_tls_set(tls, process);
		
      ret = vcos_tls_get(tls);
   }
   return ret;
}
#endif

uint64_t khronos_platform_get_process_id()
{
   return vcos_process_id_current();
}

void *platform_tls_get(PLATFORM_TLS_T tls)
{
   void *ret;

#ifdef RPC_LIBRARY
   if (!process_attached)
   {
      /* TODO: this isn't thread safe */
      client_process_attach();
      process_attached = true;
      tls = client_tls;
   }
#endif

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
#ifndef __CC_ARM	  
      vcos_thread_at_exit(client_thread_detach, NULL);
#endif
      ret = vcos_tls_get(tls);
   }
   return ret;
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
#ifndef __CC_ARM	  
   vcos_thread_deregister_at_exit(client_thread_detach, NULL);
#endif
   client_thread_detach(NULL);

	//ZRL: don't do client_process_detach()
	//because we can leave tls and mutex
#ifdef RPC_LIBRARY
   if (!CLIENT_GET_PROCESS_STATE()->connected && process_attached)
   {
      /* TODO: not thread safe */
      client_process_detach();
      process_attached = false;
   }
#endif
}

#ifndef EGL_SERVER_SMALLINT
#ifndef ANDROID
static bool have_default_dwin;
static EGL_DISPMANX_WINDOW_T default_dwin;

static EGL_DISPMANX_WINDOW_T *check_default(EGLNativeWindowType win)
{
   if (win == 0) {
      /*
       * Special identifier indicating the default window. Either use the
       * one we've got or create a new one, which would fill a WVGA screen
       * on display 0
       */

      if (!have_default_dwin) {
         DISPMANX_DISPLAY_HANDLE_T display = vc_dispmanx_display_open( 0 );
         DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start( 0 );
         VC_DISPMANX_ALPHA_T alpha = {DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 255, 0};
         VC_RECT_T dst_rect;
         VC_RECT_T src_rect;

         src_rect.x = 0;
         src_rect.y = 0;
         src_rect.width = 800 << 16;
         src_rect.height = 480 << 16;

         dst_rect.x = 0;
         dst_rect.y = 0;
         dst_rect.width = 800;
         dst_rect.height = 480;

         default_dwin.element = vc_dispmanx_element_add ( update, display,
            0/*layer*/, &dst_rect, 0/*src*/,
            &src_rect, DISPMANX_PROTECTION_NONE, &alpha, 0/*clamp*/, 0/*transform*/);

         default_dwin.width = 800;
         default_dwin.height = 480;

         vc_dispmanx_update_submit_sync( update );

         have_default_dwin = true;
      }
      return &default_dwin;
   } else
      return (EGL_DISPMANX_WINDOW_T*)win;
}

uint32_t platform_get_handle(EGLNativeWindowType win)
{
   EGL_DISPMANX_WINDOW_T *dwin = check_default(win);
   return dwin->element;  /* same handles used on host and videocore sides */
}

void platform_get_dimensions(EGLNativeWindowType win, uint32_t *width, uint32_t *height)
{
   EGL_DISPMANX_WINDOW_T *dwin = check_default(win);

   *width =  dwin->width;
   *height = dwin->height;
}

#endif // ANDROID
#endif //EGL_SERVER_SMALLINT

#ifndef __HERA_V3D__
//sjh lock/unlock
void vc_image_lock( VC_IMAGE_BUF_T *dst, const VC_IMAGE_T * src )
{
   printf("vc_image_lock %s %d\n", __FILE__, __LINE__);
}

void vc_image_unlock( VC_IMAGE_BUF_T *img )
{
   printf("vc_image_unlock %s %d\n", __FILE__, __LINE__);
}

bool platform_get_pixmap_info(EGLNativePixmapType pixmap, KHRN_IMAGE_WRAP_T *image)
{
   VC_IMAGE_T *vcimage = (VC_IMAGE_T *)pixmap, locked;
   KHRN_IMAGE_FORMAT_T format;
   int stride;
   void *data;

   vc_image_lock(&locked,vcimage);

   switch (locked.type) {
   case VC_IMAGE_RGBA32:
      format = ABGR_8888_RSO;
      break;
   case VC_IMAGE_RGBA16:
      format = RGBA_4444_RSO;
      break;
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGB565:
      format = RGB_565_RSO;
      break;
   case VC_IMAGE_TF_RGBA32:
      format = ABGR_8888_TF;
      break;
   default:
      vcos_verify(0);
      vc_image_unlock(&locked);
      return false;
   }

   switch (locked.type) {
   case VC_IMAGE_RGBA32:
   case VC_IMAGE_RGBA16:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGB565:
      /* IMAGE_WRAP_T raster order is bottom-to-top. VC_IMAGE_T raster order is top-to-bottom */
      stride = -locked.pitch;
      data = (char *)locked.image_data + ((locked.height - 1) * locked.pitch);
      break;
   case VC_IMAGE_TF_RGBA32:
      /* todo: this won't work correctly -- the splitting on send/receive assumes raster order! */
      stride = locked.pitch;
      data = locked.image_data;
      break;
   default:
      UNREACHABLE();
      stride = 0;
      data = NULL;
   }

   khrn_image_wrap(image, format, locked.width, locked.height, stride, data);

   vc_image_unlock(&locked);

   return true;
}
#else
bool platform_get_pixmap_info(EGLNativePixmapType pixmap, KHRN_IMAGE_WRAP_T *image)
{
   VC_IMAGE_T *vcimage = (VC_IMAGE_T *)pixmap;
   KHRN_IMAGE_FORMAT_T format;

   switch (vcimage->type) {
   case VC_IMAGE_RGBA32:
      format = ABGR_8888_RSO;
      break;
   case VC_IMAGE_RGBA16:
      format = RGBA_4444_RSO;
      break;
   case VC_IMAGE_RGBA565:
      format = RGB_565_RSO;
      break;

   case VC_IMAGE_TF_RGBA32:
      format = ABGR_8888_TF;
      break;
   default:
      vcos_verify(0);
      return false;
   }

   khrn_image_wrap(image, format, vcimage->width, vcimage->height, vcimage->pitch, vcimage->image_data);

   return true;
}

#endif
void *khrn_platform_malloc(size_t size, const char *desc)
{
   void* h = malloc(size);
   UNUSED(desc);
   vcos_assert(h);
   return h;
}

void khrn_platform_free(void *v)
{
   if (v) free(v);
}


#if defined(RPC_LIBRARY) || defined(RPC_DIRECT_MULTI)
void platform_get_pixmap_server_handle(EGLNativePixmapType pixmap, uint32_t *handle)
{
#ifdef RPC_LIBRARY
   VC_IMAGE_T *vcimage = (VC_IMAGE_T *)pixmap;

   vcos_assert(vcimage);
   switch (vcimage->type) {
   case VC_IMAGE_TF_RGBA32:
   case VC_IMAGE_TF_RGBX32:
   case VC_IMAGE_TF_RGB565:
   case VC_IMAGE_TF_RGBA5551:
   case VC_IMAGE_TF_RGBA16:
#ifdef __VIDEOCORE4__
   case VC_IMAGE_RGBA32:
   case VC_IMAGE_RGB565:
#endif
      /* TODO: check it's not linear tile? */
      handle[0] = (uint32_t)(uintptr_t)vcimage;
   }
#endif
}

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

void khrn_platform_release_pixmap_info(EGLNativePixmapType pixmap, KHRN_IMAGE_WRAP_T *image) {}

#else
void platform_get_pixmap_server_handle(EGLNativePixmapType pixmap, uint32_t *handle)
{
}
#endif

bool platform_match_pixmap_api_support(EGLNativePixmapType pixmap, uint32_t api_support)
{
   return true;
}

#if EGL_BRCM_global_image && EGL_KHR_image

bool platform_use_global_image_as_egl_image(uint32_t id_0, uint32_t id_1, EGLNativePixmapType pixmap, EGLint *error)
{
   vcos_verify(0);
   return false;
}

void platform_acquire_global_image(uint32_t id_0, uint32_t id_1)
{
   vcos_verify(0);
}

void platform_release_global_image(uint32_t id_0, uint32_t id_1)
{
   vcos_verify(0);
}

void platform_get_global_image_info(uint32_t id_0, uint32_t id_1,
   uint32_t *pixel_format, uint32_t *width, uint32_t *height)
{
   vcos_verify(0);
}

#endif

void platform_client_lock(void)
{
#ifdef CLIENT_THREAD_IS_PROCESS
	PLATFORM_MUTEX_T *local_mutex = (PLATFORM_MUTEX_T*)vcos_tls_get(client_tls_mutex);
	platform_mutex_acquire(local_mutex);
#else
	//KHRONOS_CLIENT_LOG("platform_client_lock start %x",vcos_thread_current());
	platform_mutex_acquire(&client_mutex);
	//KHRONOS_CLIENT_LOG("platform_client_lock end  %x",vcos_thread_current());
#endif
}

void platform_client_release(void)
{
#ifdef CLIENT_THREAD_IS_PROCESS
	PLATFORM_MUTEX_T *local_mutex = (PLATFORM_MUTEX_T*)vcos_tls_get(client_tls_mutex);
	platform_mutex_release(local_mutex);	
#else
	//KHRONOS_CLIENT_LOG("platform_client_release start %x",vcos_thread_current());
	platform_mutex_release(&client_mutex);
	//KHRONOS_CLIENT_LOG("platform_client_release end %x",vcos_thread_current());
#endif
}

void platform_init_rpc(struct CLIENT_THREAD_STATE *state) { }
void platform_term_rpc(struct CLIENT_THREAD_STATE *state) { }
void platform_maybe_free_process(void) { }
void platform_destroy_winhandle(void *a, uint32_t b) { }

#if defined(RPC_DIRECT_MULTI)
void platform_retrieve_pixmap_completed(EGLNativePixmapType pixmap) {}
void platform_send_pixmap_completed(EGLNativePixmapType pixmap) {}
#else
void platform_retrieve_pixmap_completed(EGLNativePixmapType pixmap) { UNREACHABLE(); }
void platform_send_pixmap_completed(EGLNativePixmapType pixmap) { UNREACHABLE(); }
#endif
uint32_t platform_memcmp(const void * aLeft, const void * aRight, size_t aLen) { return memcmp(aLeft, aRight, aLen); }
void platform_memcpy(void * aTrg, const void * aSrc, size_t aLength) { memcpy(aTrg, aSrc, aLength); }

#define NUM_PIXMAP_BINDINGS 32
static struct
{
   bool used;
   bool send;
   EGLNativePixmapType pixmap;
#ifdef PLATFORM_PIXMAP_HANDLE_SAVE
   KHRN_IMAGE_WRAP_T local_pixmap;
#endif	
   EGLImageKHR egl_image;
} pixmap_binding[NUM_PIXMAP_BINDINGS];

static void set_egl_image_color_data(EGLImageKHR egl_image, KHRN_IMAGE_WRAP_T *image)
{
   int line_size = (image->stride < 0) ? -image->stride : image->stride;
   int lines = KHDISPATCH_WORKSPACE_SIZE / line_size;
   int offset = 0;
   int height = image->height;

   if (khrn_image_is_tformat(image->format))
      lines &= ~63;

   assert(lines > 0);

   while (height > 0) {
      int batch = _min(lines, height);
      uint32_t len = batch * line_size;

      int adjusted_offset = (image->stride < 0) ? (offset + (batch - 1)) : offset;

      RPC_CALL7_IN_BULK(eglImageSetColorData_impl,
         EGLIMAGESETCOLORDATA_ID,
         RPC_EGLID(egl_image),
         RPC_UINT(image->format),
         RPC_UINT(image->width),
         RPC_INT(batch),
         RPC_UINT(image->stride),
         RPC_INT(offset),
         (const char *)image->storage + adjusted_offset * image->stride,
         len);

      offset += batch;
      height -= batch;
   }
}

static void send_bound_pixmap(int i)
{
   KHRN_IMAGE_WRAP_T image;

   KHRONOS_CLIENT_LOG("send_bound_pixmap %d %d\n", i, (int)pixmap_binding[i].egl_image);

   vcos_assert(i >= 0 && i < NUM_PIXMAP_BINDINGS);
   vcos_assert(pixmap_binding[i].used);
   
   platform_get_pixmap_info(pixmap_binding[i].pixmap, &image);
   set_egl_image_color_data(pixmap_binding[i].egl_image, &image);
   khrn_platform_release_pixmap_info(pixmap_binding[i].pixmap, &image);
}

static void send_bound_pixmaps(void)
{
   int i;
   for (i = 0; i < NUM_PIXMAP_BINDINGS; i++)
   {
      if (pixmap_binding[i].used && pixmap_binding[i].send)
      {
         send_bound_pixmap(i);
      }
   }
}

void platform_surface_update(uint32_t handle)
{
   /*
   XXX This seems as good a place as any to do the client side pixmap hack.
   (called from eglSwapBuffers)
   */
   send_bound_pixmaps();
}

void khrn_platform_bind_pixmap_to_egl_image(EGLNativePixmapType pixmap, EGLImageKHR egl_image, bool send)
{
   int i;
   for (i = 0; i < NUM_PIXMAP_BINDINGS; i++)
   {
      if (!pixmap_binding[i].used)
      {

         KHRONOS_CLIENT_LOG("khrn_platform_bind_pixmap_to_egl_image %d\n", i);

         pixmap_binding[i].used = true;
#ifdef PLATFORM_PIXMAP_HANDLE_SAVE
         pixmap_binding[i].pixmap = &(pixmap_binding[i].local_pixmap);
         memcpy(pixmap_binding[i].pixmap,pixmap,sizeof(KHRN_IMAGE_WRAP_T));
#else
         pixmap_binding[i].pixmap = pixmap;
#endif
         pixmap_binding[i].egl_image = egl_image;
         pixmap_binding[i].send = send;
         if(send)
            send_bound_pixmap(i);

         //pixmap_binding[i].send = 0; 
		 return;
      }
   }
   vcos_assert(0);  /* Not enough NUM_PIXMAP_BINDINGS? */
}

void khrn_platform_unbind_pixmap_from_egl_image(EGLImageKHR egl_image)
{
   int i;
   for (i = 0; i < NUM_PIXMAP_BINDINGS; i++)
   {
      if (pixmap_binding[i].used && pixmap_binding[i].egl_image == egl_image)
      {
         pixmap_binding[i].used = false;
      }
   }
}


#ifdef RPC_DIRECT_MULTI
static VCOS_SEMAPHORE_T rpc_direct_multi_lock;
bool rpc_direct_multi_init(void)
{
	int success = -1;
	success = vcos_semaphore_create(&rpc_direct_multi_lock,"rpc_direct_multi_lock",1);
	vcos_assert(success == 0);

	return true;
}
void rpc_term(void)
{

	vcos_semaphore_delete(&rpc_direct_multi_lock);

}

int client_library_get_connection(void)
{
   return 0;
}
#ifdef BRCM_V3D_OPT
static uint64_t thread_id = NULL;
#else
static VCOS_THREAD_T *thread_id = NULL;
#endif
const KHRONOS_FUNC_TABLE_T *khronos_server_lock_func_table(int connection)
{
   KHRONOS_FUNC_TABLE_T *khronos_func_table;	
#ifdef BRCM_V3D_OPT
	uint64_t current_thread_id;
#else
	VCOS_THREAD_T *current_thread_id;
#endif
	int success = -1;
	
	success = vcos_semaphore_wait(&rpc_direct_multi_lock);
	vcos_assert(success==0);

   khronos_func_table = (KHRONOS_FUNC_TABLE_T*)khronos_get_func_table();
#ifdef BRCM_V3D_OPT
	current_thread_id = pthread_self();//gettid();
#else
	current_thread_id = (VCOS_THREAD_T *)vcos_thread_current();
#endif
	if (thread_id == NULL)
		thread_id = current_thread_id;
	else if (current_thread_id != thread_id)
	{  
		thread_id = current_thread_id;
		khronos_func_table->khrn_misc_rpc_flush_impl();
		client_library_send_make_current(khronos_func_table);
	}

	return (KHRONOS_FUNC_TABLE_T*)khronos_get_func_table();

}

void khronos_server_unlock_func_table(void)
{
	int success = -1;
	success = vcos_semaphore_post(&rpc_direct_multi_lock);
	
	vcos_assert(success==0);
}
#endif

