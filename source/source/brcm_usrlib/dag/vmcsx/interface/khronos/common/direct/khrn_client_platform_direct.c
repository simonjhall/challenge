#include <stdlib.h>
#include <assert.h>

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
#include <stdio.h>
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
   printk("\n");
}

#endif

static void *single_tls = 0;

static VCOS_EVENT_T ev;
static int ev_init = 0;
// Client is single-threaded on direct platform so no mutex stuff necessary

// TODO: find somewhere better to put this function?

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

VCOS_STATUS_T khronos_platform_semaphore_create(PLATFORM_SEMAPHORE_T *sem, int name[3], int count)
{
   char buf[64];
   // should be KhanSemaphore but we are limited to 31 characters
   vcos_snprintf(buf,sizeof(buf),"KhanSem%08x%08x%08x", name[0], name[1], name[2]);
   return vcos_named_semaphore_create(sem, buf, count);
}

void khronos_platform_semaphore_destroy(PLATFORM_SEMAPHORE_T *sem)
{
   vcos_named_semaphore_delete(sem);
}

void khronos_platform_semaphore_acquire(PLATFORM_SEMAPHORE_T *sem)
{
   while (vcos_named_semaphore_trywait(sem) == VCOS_EAGAIN) {
      khrn_sync_master_wait();
   }
}

void khronos_platform_semaphore_release(PLATFORM_SEMAPHORE_T *sem)
{
   vcos_named_semaphore_post(sem);
   khrn_sync_notify_master();
}

VCOS_STATUS_T platform_tls_create(PLATFORM_TLS_T *tls)
{
   *tls = (PLATFORM_TLS_T)1;
   return VCOS_SUCCESS;
}

void platform_tls_destroy(PLATFORM_TLS_T tls)
{
   UNUSED(tls);
   /* Nothing to do */
}

void platform_tls_set(PLATFORM_TLS_T tls, void *v)
{
   UNUSED(tls);
   single_tls = v;
}

void *platform_tls_get(PLATFORM_TLS_T tls)
{
   UNUSED(tls);
   if (!egl_server_state_initted) {
#ifdef KHRONOS_CLIENT_LOGGING
      xxx_vclog = fopen("vclog.txt", "w");
      if (xxx_vclog == NULL)
         perror("Can't open 'vclog.txt' to write");
      else {
         fprintf(xxx_vclog, "Attaching process\n");
         fflush(xxx_vclog);
      }
#endif
#ifndef SIMPENROSE
      if (ev_init == 0) { ev_init=1;
      vcos_event_create(&ev, "Khronos Master Event");
      }
      khrn_specify_event(&ev);
#endif
      egl_server_startup_hack();
      client_process_attach();
      if (!single_tls)
         client_thread_attach();
   }
   return single_tls;
}

void * platform_tls_get_check(PLATFORM_TLS_T tls)
{
   UNUSED(tls);
   return (egl_server_state_initted) ? single_tls : NULL;
}

void platform_tls_remove(PLATFORM_TLS_T tls)
{
   UNUSED(tls);
   single_tls = 0;
}

/* ----------------------------------------------------------------------
 * workaround for broken platforms which don't detect threads exiting
 * -------------------------------------------------------------------- */
void platform_hint_thread_finished()
{
   vcos_assert(!egl_server_state_initted);
   egl_server_state_initted = 1;
   client_thread_detach(NULL);
   egl_server_state_initted = 0;
}

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

#ifndef EGL_SERVER_SMALLINT
#ifndef ANDROID
#include "middleware/dispmanx/dispmanx.h"

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
         DISPMANX_DISPLAY_HANDLE_T display = dispmanx_display_open( 0 );
         DISPMANX_UPDATE_HANDLE_T update = dispmanx_update_start( 0 );
         DISPMANX_ALPHA_T alpha = {DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 255, 0};
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

         default_dwin.element = dispmanx_element_add ( update, display,
            0/*layer*/, &dst_rect, 0/*src*/,
            &src_rect, 0 /* protection*/, &alpha, 0/*clamp*/, 0/*transform*/);

         default_dwin.width = 800;
         default_dwin.height = 480;

         dispmanx_update_submit_sync( update );

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

void platform_get_pixmap_server_handle(EGLNativePixmapType pixmap, uint32_t *handle)
{
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
}

bool platform_match_pixmap_api_support(EGLNativePixmapType pixmap, uint32_t api_support)
{
   UNUSED(pixmap);
   UNUSED(api_support);

   return true;
}

uint64_t khronos_platform_get_process_id(void)
{
   return 0;
}

#if EGL_BRCM_global_image && EGL_KHR_image

bool platform_use_global_image_as_egl_image(uint32_t id_0, uint32_t id_1, EGLNativePixmapType pixmap, EGLint *error)
{
   UNUSED(id_0);
   UNUSED(id_1);
   UNUSED(pixmap);
   UNUSED(error);

   vcos_verify(0);
   return false;
}

void platform_acquire_global_image(uint32_t id_0, uint32_t id_1)
{
   UNUSED(id_0);
   UNUSED(id_1);

   vcos_verify(0);
}

void platform_release_global_image(uint32_t id_0, uint32_t id_1)
{
   UNUSED(id_0);
   UNUSED(id_1);

   vcos_verify(0);
}

void platform_get_global_image_info(uint32_t id_0, uint32_t id_1,
   uint32_t *pixel_format, uint32_t *width, uint32_t *height)
{
   UNUSED(id_0);
   UNUSED(id_1);
   UNUSED(pixel_format);
   UNUSED(width);
   UNUSED(height);

   vcos_verify(0);
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

void platform_init_rpc(struct CLIENT_THREAD_STATE *state) { UNUSED(state); }
void platform_term_rpc(struct CLIENT_THREAD_STATE *state) { UNUSED(state); }
void platform_maybe_free_process(void) { }
void platform_destroy_winhandle(void *a, uint32_t b) { UNUSED(a); UNUSED(b); }

void egl_gce_win_change_image(void) { UNREACHABLE(); }


void platform_retrieve_pixmap_completed(EGLNativePixmapType pixmap) {}
void platform_send_pixmap_completed(EGLNativePixmapType pixmap) {}
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

void khrn_platform_release_pixmap_info(EGLNativePixmapType pixmap, KHRN_IMAGE_WRAP_T *image)
{
   UNUSED(pixmap);
   UNUSED(image);
   /* Nothing to do */
}

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
   UNUSED(pixmap);
   UNUSED(egl_image);
   UNUSED(send);
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
   UNUSED(egl_image);
   for (i = 0; i < NUM_PIXMAP_BINDINGS; i++)
   {
      if (pixmap_binding[i].used && pixmap_binding[i].egl_image == egl_image)
      {
         pixmap_binding[i].used = false;
      }
   }
}

