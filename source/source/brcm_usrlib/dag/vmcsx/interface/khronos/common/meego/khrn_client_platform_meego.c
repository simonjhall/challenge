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
#include "interface/khronos/common/khrn_client_rpc.h"
#include "interface/khronos/common/khrn_int_ids.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "X11/Xlib.h"

static void send_bound_pixmaps(void);
static void dump_hierarchy(Window w, Window thisw, Window look, int level);
static void dump_ancestors(Window w);

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

//see helpers\scalerlib\scalerlib_misc.c
//int32_t scalerlib_convert_vcimage_to_display_element()
//dark blue, 1<<3 in 888
#define CHROMA_KEY_565 0x0001
//

static Display *hacky_display = 0;

static XErrorHandler old_handler = (XErrorHandler) 0 ;
static int application_error_handler(Display *display, XErrorEvent *theEvent)
{
   KHRONOS_CLIENT_LOG(
   		"Ignoring Xlib error: error code %d request code %d\n",
   		theEvent->error_code,
   		theEvent->request_code) ;
   return 0 ;
}


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

static bool process_attached = false;

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
      process_attached = true;
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


static XImage *current_ximage = NULL;

static KHRN_IMAGE_FORMAT_T ximage_to_image_format(int bits_per_pixel, unsigned long red_mask, unsigned long green_mask, unsigned long blue_mask)
{
   if (bits_per_pixel == 16 /*&& red_mask == 0xf800 && green_mask == 0x07e0 && blue_mask == 0x001f*/)
      return RGB_565_RSO;
   //else if (bits_per_pixel == 24 && red_mask == 0xff0000 && green_mask == 0x00ff00 && blue_mask == 0x0000ff)
   //   return RGB_888_RSO;
   else if (bits_per_pixel == 24 && red_mask == 0x0000ff && green_mask == 0x00ff00 && blue_mask == 0xff0000)
      return BGR_888_RSO;
   else if (bits_per_pixel == 32 /*&& red_mask == 0x0000ff && green_mask == 0x00ff00 && blue_mask == 0xff0000*/)
      return ABGR_8888_RSO; //meego uses alpha channel
   else if (bits_per_pixel == 32 && red_mask == 0xff0000 && green_mask == 0x00ff00 && blue_mask == 0x0000ff)
      return ARGB_8888_RSO;
   else
   {
      KHRONOS_CLIENT_LOG("platform_get_pixmap_info unknown image format\n");
      return IMAGE_FORMAT_INVALID;
   }
}

 

bool platform_get_pixmap_info(EGLNativePixmapType pixmap, KHRN_IMAGE_WRAP_T *image)
{
   Window r;
   int x, y;
   unsigned int w, h, b, d;
   KHRN_IMAGE_FORMAT_T format;
   XImage *xi;
   XWindowAttributes attr;
   Status rc;

   KHRONOS_CLIENT_LOG("platform_get_pixmap_info !!!\n");

   if (!XGetGeometry(hacky_display, (Drawable)pixmap, &r, &x, &y, &w, &h, &b, &d))
      return false;

   KHRONOS_CLIENT_LOG("platform_get_pixmap_info %d geometry = %d %d %d %d\n",(int)pixmap,
              x, y, w, h);

   xi = XGetImage(hacky_display, (Drawable)pixmap, 0, 0, w, h, 0xffffffff, ZPixmap);
   if (xi == NULL)
      return false;

   KHRONOS_CLIENT_LOG("platform_get_pixmap_info ximage = %d %d %d 0x%08x %d %x %x %x\n",
              xi->width, xi->height, xi->bytes_per_line, (uint32_t)xi->data,
              xi->bits_per_pixel, (uint32_t)xi->red_mask,
              (uint32_t)xi->green_mask, (uint32_t)xi->blue_mask);

   format = ximage_to_image_format(xi->bits_per_pixel, xi->red_mask, xi->green_mask, xi->blue_mask);
   if (format == IMAGE_FORMAT_INVALID)
   {
      XDestroyImage(xi);
      return false;
   }

   image->format = format;
   image->width = xi->width;
   image->height = xi->height;
   image->stride = xi->bytes_per_line;
   image->aux = NULL;
   image->storage = xi->data;

//hacking to see if this pixmap is actually the offscreen pixmap for the window that is our current surface
   {
      int xw, yw;
      unsigned int ww, hw, bw, dw;
      unsigned long pixel;
      Window rw,win  = (Window)CLIENT_GET_THREAD_STATE()->opengl.draw->win;
      KHRONOS_CLIENT_LOG("current EGL surface win %d ", (int)win);
      if(win!=0)
      {
         /* Install our error handler to override Xlib's termination behavior */
         old_handler = XSetErrorHandler(application_error_handler) ;
         
         XGetGeometry(hacky_display, (Drawable)win, &rw, &xw, &yw, &ww, &hw, &bw, &dw);
         KHRONOS_CLIENT_LOG("%dx%d\n", ww, hw);
         if(ww==w && hw==h)
         {
            //this pixmap is the same size as our current window
            pixel = XGetPixel(xi,w/2,h/2);
            KHRONOS_CLIENT_LOG("Pixmap centre pixel 0x%lx%s\n",pixel,pixel==CHROMA_KEY_565 ? "- chroma key!!" : "");
            if(pixel == CHROMA_KEY_565)//the pixmap is also full of our magic chroma key colour, we want to copy the server side EGL surface.
               image->aux = (void *)CLIENT_GET_THREAD_STATE()->opengl.draw->serverbuffer ;
         }        
         
         (void) XSetErrorHandler(old_handler) ;
      }
   }
//

   current_ximage = xi;
   return true;
}

void khrn_platform_release_pixmap_info(EGLNativePixmapType pixmap, KHRN_IMAGE_WRAP_T *image)
{
   XDestroyImage(current_ximage);
   current_ximage = NULL;
}

void platform_get_pixmap_server_handle(EGLNativePixmapType pixmap, uint32_t *handle)
{
   KHRONOS_CLIENT_LOG("platform_get_pixmap_server_handle !!!\n");
   /* Don't do anything */
}

bool platform_match_pixmap_api_support(EGLNativePixmapType pixmap, uint32_t api_support)
{
   KHRONOS_CLIENT_LOG("platform_match_pixmap_api_support !!!\n");
   return 0;
}

#if EGL_BRCM_global_image && EGL_KHR_image

bool platform_use_global_image_as_egl_image(uint32_t id_0, uint32_t id_1, EGLNativePixmapType pixmap, EGLint *error)
{
   assert(0);
   return false;
}

void platform_acquire_global_image(uint32_t id_0, uint32_t id_1)
{
   assert(0);
}

void platform_release_global_image(uint32_t id_0, uint32_t id_1)
{
   assert(0);
}

void platform_get_global_image_info(uint32_t id_0, uint32_t id_1,
   uint32_t *pixel_format, uint32_t *width, uint32_t *height)
{
   assert(0);
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
   assert(1);
}

void platform_term_rpc(struct CLIENT_THREAD_STATE *state) 
{
   assert(1);
}

void platform_maybe_free_process(void) 
{
   assert(1);
}

void platform_destroy_winhandle(void *a, uint32_t b) 
{
   assert(1);
}

void platform_surface_update(uint32_t handle) 
{
   /*
   XXX This seems as good a place as any to do the client side pixmap hack.
   (called from eglSwapBuffers)
   */
   send_bound_pixmaps();
}

void egl_gce_win_change_image(void) 
{
   assert(0);
}

void platform_retrieve_pixmap_completed(EGLNativePixmapType pixmap) 
{
   assert(0);
}

void platform_send_pixmap_completed(EGLNativePixmapType pixmap) 
{
   assert(0);
}

uint32_t platform_memcmp(const void * aLeft, const void * aRight, size_t aLen) 
{ 
   return memcmp(aLeft, aRight, aLen); 
}

void platform_memcpy(void * aTrg, const void * aSrc, size_t aLength) 
{ 
   memcpy(aTrg, aSrc, aLength); 
}



uint32_t platform_get_handle(EGLNativeWindowType win)
{
   return (uint32_t)win;
}

void platform_get_dimensions(EGLNativeWindowType win, uint32_t *width, uint32_t *height)
{
   Window w = (Window) win;
   XWindowAttributes attr;
   GC gc;
   Status rc = XGetWindowAttributes(hacky_display, w, &attr);

   // check rc is OK and if it is (vcos_assert(rc == 0);?????)
   *width = attr.width;
   *height = attr.height;

	 /* Hackily assume if this function is called then they want to fill with GL stuff. So fill window with chromakey. */
   KHRONOS_CLIENT_LOG("Calling XCreateGC %d\n",(int)w);

	 gc = XCreateGC(hacky_display, w, 0, NULL);
	 XSetForeground(hacky_display, gc, CHROMA_KEY_565);

   KHRONOS_CLIENT_LOG("Calling XFillRectangle %d %dx%d\n",(int)w,attr.width, attr.height);

	 XFillRectangle(hacky_display, w, gc, 0, 0, attr.width, attr.height);

   KHRONOS_CLIENT_LOG("Calling XFreeGC\n");

	 XFreeGC(hacky_display, gc);

   KHRONOS_CLIENT_LOG("Done platform_get_dimensions\n");
    //debugging
    dump_hierarchy(attr.root, w, 0, 0);
}

EGLDisplay khrn_platform_set_display_id(EGLNativeDisplayType display_id)
{
   if(hacky_display==0)
   {
	   hacky_display = (Display *)display_id;
	   return (EGLDisplay)1;
	}
	else
	   return EGL_NO_DISPLAY;
}

static void dump_hierarchy(Window w, Window thisw, Window look, int level)
{
#ifdef KHRONOS_CLIENT_LOGGING
   Window root_dummy, parent_dummy, *children;
   unsigned int i, nchildren;
   XWindowAttributes attr;
   
   XGetWindowAttributes(hacky_display, w, &attr);
   XQueryTree(hacky_display, w, &root_dummy, &parent_dummy, &children, &nchildren);
      
   for (i = 0; i < level; i++)
   {
         KHRONOS_CLIENT_LOG(" ");
   }
   KHRONOS_CLIENT_LOG( "%d %d%s%s\n",
              attr.map_state, (int)w,
              (w==look)?" <-- LOOK FOR ME!":((w==thisw)?" <-- THIS WINDOW":""),
              children?"":" no children");
               
   if (children)
   {
      for (i = 0; i < nchildren; i++)
      {
         dump_hierarchy(children[i], thisw, look, level + 1);
      }
      XFree(children);
   }
#endif
}

static void dump_ancestors(Window w)
{
#ifdef KHRONOS_CLIENT_LOGGING   
   Window root_dummy, *children;
   unsigned int i, nchildren;

   Window grandparent,parent = w, child = 0;
   unsigned int rlayer = ~0;
   bool bidirectional;
   KHRONOS_CLIENT_LOG("walking back up heirarchy\n");
   while(parent)
   {
      bidirectional = false;
      if(!XQueryTree(hacky_display, parent, &root_dummy, &grandparent, &children, &nchildren))
         break;
      if (children)
      {
         for (i = 0; i < nchildren; i++)
         {
            if (children[i] == child)
            {
               bidirectional = true;
               rlayer = i;
            }
         }
         XFree(children);
      }
      KHRONOS_CLIENT_LOG("%s%s%d", bidirectional ? "<" : "", (child>0) ? "->" : "", (int)parent);

      child = parent;
      parent = grandparent;
      
   }
   KHRONOS_CLIENT_LOG("->end\n");

#endif   
}


uint32_t khrn_platform_get_window_position(EGLNativeWindowType win)
{
   Window w = (Window) win;
   Window dummy;
   XWindowAttributes attr;
   Window look_for_me, root_dummy, root_dummy2, parent_dummy, *children;
   int x, y;
   unsigned int layer, i, nchildren;

   //the assumption is that windows are at the 2nd level i.e. in the below
   //root_dummy/attr.root -> look_for_me -> w
   KHRONOS_CLIENT_LOG("Start khrn_platform_get_window_position\n");

   XGetWindowAttributes(hacky_display, w, &attr);
   
   KHRONOS_CLIENT_LOG("XGetWindowAttributes\n");

   if (attr.map_state == IsViewable)
   {
      XTranslateCoordinates(hacky_display, w, attr.root, 0, 0, &x, &y, &dummy);

      KHRONOS_CLIENT_LOG("XTranslateCoordinates\n");

      XQueryTree(hacky_display, w, &root_dummy, &look_for_me, &children, &nchildren);
      if (children) XFree(children);
      XQueryTree(hacky_display, attr.root, &root_dummy2, &parent_dummy, &children, &nchildren);

      KHRONOS_CLIENT_LOG("XQueryTree\n");

      layer = ~0;
      
      KHRONOS_CLIENT_LOG("Dumping hierarchy %d %d (%d)\n", (int)w, (int)look_for_me, (int)root_dummy);
      dump_hierarchy(attr.root, w, look_for_me, 0);

      if (children)
      {
         for (i = 0; i < nchildren; i++)
         {
            if (children[i] == look_for_me)
               layer = i;
         }
         XFree(children);
      }

      KHRONOS_CLIENT_LOG("XFree\n");
      
      if (layer == ~0)
      {
         KHRONOS_CLIENT_LOG("EGL window isn't child of root\n", i);
         
         //to try and find out where this window has gone, let us walk back up the heirarchy
         dump_ancestors(w);
         return ~0;
      } 
      else
      {
         KHRONOS_CLIENT_LOG("End khrn_platform_get_window_position - visible\n");
         return x | y << 12 | layer << 24;
      }
   }
   else
   {
      KHRONOS_CLIENT_LOG("End khrn_platform_get_window_position - invisible\n");
      
      return ~0;      /* Window is invisible */
   }
}

#define NUM_PIXMAP_BINDINGS 16
static struct
{
   bool used;
   bool send;
   EGLNativePixmapType pixmap;
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

void khrn_platform_bind_pixmap_to_egl_image(EGLNativePixmapType pixmap, EGLImageKHR egl_image, bool send)
{
   int i;
   for (i = 0; i < NUM_PIXMAP_BINDINGS; i++)
   {
      if (!pixmap_binding[i].used)
      {

         KHRONOS_CLIENT_LOG("khrn_platform_bind_pixmap_to_egl_image %d\n", i);

         pixmap_binding[i].used = true;
         pixmap_binding[i].pixmap = pixmap;
         pixmap_binding[i].egl_image = egl_image;
         pixmap_binding[i].send = send;
         if(send)
            send_bound_pixmap(i);
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
