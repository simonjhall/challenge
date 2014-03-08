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
#include "middleware/khronos/egl/egl_platform.h"
#include "interface/khronos/include/EGL/egl.h" /* for native window types */
#include "interface/khronos/include/EGL/eglext.h"
#include "middleware/dispmanx/dispmanx.h"
#include "helpers/vc_image/vc_image.h"

#include "vcfw/logging/logging.h"
#include "interface/vchi/os/os.h"

#include "helpers/vclib/vclib.h"

#include <assert.h>

/* KHRN_IMAGE_T raster order is bottom-to-top; VC_IMAGE_T raster order is top-to-bottom... */
static bool flip_it(const VC_IMAGE_T *vc_image)
{
   return (vc_image->type == VC_IMAGE_RGBA32) || (vc_image->type == VC_IMAGE_RGBX32) || (vc_image->type == VC_IMAGE_RGB565);
}

static EGL_SERVER_RETURN_CALLBACK_T egl_callback;
void egl_server_platform_init(EGL_SERVER_RETURN_CALLBACK_T return_callback)
{
   egl_callback = return_callback;
}
void egl_server_platform_shutdown(void)
{
}
static int inited = 0;

static uint32_t display_num = 0;
static DISPMANX_DISPLAY_HANDLE_T display;

#define ELEMENTS_N 10
static struct {
   bool used;
   uint32_t win;
   DISPMANX_ELEMENT_HANDLE_T element;
   bool visible;
   uint32_t x, y, width, height, layer;
   VC_IMAGE_T debug_image;
} elements[ELEMENTS_N] = {0};
vcos_static_assert(DISPMANX_NO_HANDLE == 0);

static int add_win(uint32_t win)
{
   uint32_t i;

   /* Find existing window with this id */
   for (i = 0; i != ELEMENTS_N; ++i)
   {
      if (elements[i].used && elements[i].win == win)
         return i;
   }

   /* Add new window with this id */
   for (i = 0; i != ELEMENTS_N; ++i)
   {
      if (!elements[i].used)
      {
         elements[i].used = true;
         elements[i].element = DISPMANX_NO_HANDLE;
         elements[i].win = win;
         elements[i].x = elements[i].y = 0;
         elements[i].width = elements[i].height = 16;
         return i;
      }
   }
   assert(0); /* increase ELEMENTS_N? */
   return 0;
}

static void remove_win(uint32_t win)
{
   uint32_t i;
   for (i = 0; i != ELEMENTS_N; ++i)
   {
      if (elements[i].used && elements[i].win == win)
      {
         vcos_assert(elements[i].element == DISPMANX_NO_HANDLE);
         elements[i].used = false;
         return;
      }
   }
   /* not finding isn't an error... */
}

static void add_element(DISPMANX_ELEMENT_HANDLE_T element, uint32_t win)
{
   uint32_t i = add_win(win);
   vcos_assert(elements[i].element == DISPMANX_NO_HANDLE);
   elements[i].element = element;
}

static void remove_element(DISPMANX_UPDATE_HANDLE_T update, uint32_t win)
{
   uint32_t i;
   for (i = 0; i != ELEMENTS_N; ++i)
   {
      if (elements[i].used && elements[i].win == win && elements[i].element != DISPMANX_NO_HANDLE)
      {
         dispmanx_element_remove(update, elements[i].element);
         elements[i].element = DISPMANX_NO_HANDLE;
         return;
      }
   }
   /* not finding isn't an error... */
}

static void our_callback(DISPMANX_UPDATE_HANDLE_T u, void * arg)
{
   egl_callback((uint32_t)arg);
}

void egl_server_platform_display(uint32_t win, KHRN_IMAGE_T *image, uint32_t cb_arg)
{
   VC_RECT_T dr;
   DISPMANX_UPDATE_HANDLE_T update;
   uint32_t i;
   VC_IMAGE_T vc_image;

   khrn_image_fill_vcimage(image, &vc_image);

   if (!inited) {
      inited = 1;

      dispmanx_init();
      display = dispmanx_display_open(display_num);
   }

   update = dispmanx_update_start(0);

   i = add_win(win);
   remove_element(update, win);
   if (display && elements[i].visible)
   {
      DISPMANX_ALPHA_T alpha = {DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 255/*128*/, 0};

      dr.x = elements[i].x;
      dr.y = elements[i].y;
      dr.width = elements[i].width;
      dr.height = elements[i].height;
      elements[i].debug_image = vc_image;
	    
      add_element(dispmanx_element_add(update, display, elements[i].layer, &dr, &vc_image, 0, DISPMANX_PROTECTION_NONE, &alpha, 0/*clamp*/,
         flip_it(&vc_image) ? DISPMANX_FLIP_VERT : DISPMANX_NO_ROTATE), win);
   }

   dispmanx_update_submit(update, our_callback, (void *)cb_arg);
}

void egl_server_platform_display_nothing_sync(uint32_t win)
{
   vcos_assert(win != EGL_PLATFORM_WIN_NONE);

   DISPMANX_UPDATE_HANDLE_T update = dispmanx_update_start(0);

   remove_element(update, win);
   dispmanx_update_submit_sync(update);
   //dispmanx_update_submit(update, our_callback, (void *)cb_arg);
   remove_win(win);
}

MEM_HANDLE_T egl_server_platform_create_pixmap_info(uint32_t pixmap)
{
   UNREACHABLE();
   return MEM_INVALID_HANDLE;
}

void egl_server_platform_set_position(uint32_t win, uint32_t position, uint32_t width, uint32_t height)
{
   uint32_t i = add_win(win);
   if (position == ~0)
   {
      elements[i].visible = false;
   }
   else
   {
      elements[i].visible = true;
      elements[i].x = position & 0xfff;
      elements[i].y = (position >> 12) & 0xfff;
      elements[i].width = width;
      elements[i].height = height;
      elements[i].layer = (position >> 24) & 0xff;
   }
}
