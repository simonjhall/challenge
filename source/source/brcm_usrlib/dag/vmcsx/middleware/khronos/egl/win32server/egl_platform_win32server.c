/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <windows.h>
#include <assert.h>
#include <process.h>
#include "middleware/khronos/egl/egl_platform.h"
#include "helpers/vc_image/vc_image.h"

static bool displaying = false;
static uint32_t current_win;

static HWND the_hwnd;
static HANDLE the_mutex;
static HANDLE display_response_semaphore;
static uint32_t require_display_response;
static HBITMAP the_bitmap;
static HDC the_dc;
static uint32_t the_width;
static uint32_t the_height;

static EGL_SERVER_RETURN_CALLBACK_T egl_callback;

static void win32_update_bitmap(HWND hwnd, void *bits, uint32_t width, uint32_t height, int32_t pitch, bool tformat, bool sbpp);
static HWND win32_create_window(uint32_t width, uint32_t height, LPCWSTR caption);
static LRESULT CALLBACK win32_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void paintwnd(HWND hwnd);
static void window_thread_main(void *arg);

void egl_server_platform_display(uint32_t win, KHRN_IMAGE_T *image, uint32_t cb_arg)
{
   VC_IMAGE_T vc_image;
   void *ptr;
   vcos_assert(win != EGL_PLATFORM_WIN_NONE);
   khrn_image_fill_vcimage(image, &vc_image);
   if (!displaying) {
      displaying = true;
      current_win = win;

      the_mutex = CreateMutex(NULL, FALSE, NULL);
      display_response_semaphore = CreateSemaphore(NULL, 0, 1, NULL);
      require_display_response = 1;   //always require a response to startup
      the_width = vc_image.width;
      the_height = vc_image.height;
      _beginthread(window_thread_main, 0, 0);
      WaitForSingleObject(display_response_semaphore, INFINITE);
      vcos_assert(the_hwnd);
   }
   vcos_assert(win == current_win);    //Can only display on a single window

   WaitForSingleObject(the_mutex, INFINITE);
#ifdef __BCM2708A0__
   vcos_assert(vc_image.type == VC_IMAGE_TF_RGBA32 || vc_image.type == VC_IMAGE_RGBA32 || vc_image.type == VC_IMAGE_TF_RGBX32);
#else
   vcos_assert(vc_image.type == VC_IMAGE_TF_RGBA32 ||
          vc_image.type == VC_IMAGE_RGBA32 ||
          vc_image.type == VC_IMAGE_RGBX32 ||
          vc_image.type == VC_IMAGE_TF_RGBX32 ||
          vc_image.type == VC_IMAGE_TF_RGB565 ||
          vc_image.type == VC_IMAGE_RGB565 ||
          vc_image.type == VC_IMAGE_RGBX32);
#endif
   ptr = (vc_image.mem_handle != MEM_INVALID_HANDLE) ? mem_lock(vc_image.mem_handle) : vc_image.image_data;
   win32_update_bitmap(the_hwnd, ptr,
                       vc_image.width, vc_image.height, vc_image.pitch,
                       (vc_image.type == VC_IMAGE_TF_RGBA32 || vc_image.type == VC_IMAGE_TF_RGBX32 || vc_image.type == VC_IMAGE_TF_RGB565),
                       (vc_image.type == VC_IMAGE_TF_RGB565 || vc_image.type == VC_IMAGE_RGB565));
   if (vc_image.mem_handle != MEM_INVALID_HANDLE) mem_unlock(vc_image.mem_handle);
   InvalidateRgn(the_hwnd, 0, TRUE);
   vcos_assert(!require_display_response);
   require_display_response = 1;
   ReleaseMutex(the_mutex);

   /* Wait for the other thread to respond */
   WaitForSingleObject(display_response_semaphore, INFINITE);
   vcos_assert(!require_display_response);

   /* May as well call callback immediately */
   egl_callback(cb_arg);
}

void egl_server_platform_display_nothing_sync(uint32_t win)
{
   //TODO: wait for thread to finish?
   vcos_assert(win != EGL_PLATFORM_WIN_NONE);
   vcos_assert(!displaying || (win == current_win));
   displaying = false;

   SendMessage(the_hwnd, WM_CLOSE, 0, 0);
}

static void *alloc_data = 0;
static void win32_update_bitmap(HWND hwnd, void *bits, uint32_t width, uint32_t height, int32_t pitch, bool tformat, bool sbpp)
{
   HDC hdcScreen;
   KHRN_IMAGE_WRAP_T src, dst;
   BITMAPINFO bmi = {
      {
         sizeof(BITMAPINFOHEADER),     //biSize
         sbpp ? (pitch >> 1) : (pitch >> 2),                   //biWidth
#if defined(BCG_FB_LAYOUT) || defined(BCG_FB_LAYOUT_B1)
         -(int)height,                       //biHeight
#else
         height,                       //biHeight
#endif /* BCG_FB_LAYOUT */
         1,                            //biPlanes
         32,                           //biBitCount
         BI_RGB,                       //BiCompression
         0,                            //biSizeImage
         2048,                         //biXPelsPerMeter
         2048,                         //biYPelsPerMeter
         0,                            //biClrUsed
         0,                            //biClrImportant
      }
   };
   int result;

   vcos_assert(!(pitch & 0x3));

   hdcScreen = GetDC(hwnd);

   if (!alloc_data)
   {
      alloc_data = malloc(4*2048*2048);
      assert(alloc_data);
   }
   vcos_assert(width * height <= 2048 * 2048);

   if (the_dc == 0)
   {
      the_dc = CreateCompatibleDC(hdcScreen);
      the_bitmap = CreateCompatibleBitmap(hdcScreen, width, height);
   }

   if (sbpp)
   {
      /* Don't swap red and blue. Do t-format conversion if necessary. */
      khrn_image_wrap(&src, tformat ? RGB_565_TF : RGB_565_RSO, width, height, pitch, bits);
      khrn_image_wrap(&dst, ARGB_8888_RSO, width, height, pitch * 2, alloc_data);
   }
   else
   {
      /* Swap red and blue. Do t-format conversion if necessary. */
      khrn_image_wrap(&src, tformat ? ABGR_8888_TF : ABGR_8888_RSO, width, height, pitch, bits);
      khrn_image_wrap(&dst, ARGB_8888_RSO, width, height, pitch, alloc_data);
   }

   khrn_image_wrap_convert(&dst, &src, IMAGE_CONV_GL);

   result = SetDIBits(the_dc, the_bitmap, 0, height, alloc_data, &bmi, DIB_RGB_COLORS);
   assert(result);

   the_width = width;
   the_height = height;
}

static HWND win32_create_window(uint32_t width, uint32_t height, LPCWSTR caption)
{
   HWND hw;
   DWORD foo, foo0;
   RECT wndRect = { 0, 0, width, height };

   WCHAR className[] = L"myglutclass";

   WNDCLASS wcsetup = {
      CS_HREDRAW|CS_VREDRAW,       //style
      win32_wndproc,               //lpfnWndProc
      0,                           //cbClsExtra
      0,                           //cbWndExtra
      GetModuleHandle(NULL),       //hInstance
      0,                           //hIcon
      0,                           //hCursor
      (HBRUSH)(COLOR_WINDOW+1),    //hbrBackground
      NULL,                        //lpszMenuName
      className,                   //lpszClassName
   };
   // remove maximize option as this will cause a resize
   DWORD style = WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MINIMIZEBOX;
   ATOM wc = RegisterClass(&wcsetup);
   foo0 = GetLastError();
   // adjust the window to the size it needs to be to accomodate the width and height
   AdjustWindowRect(&wndRect, style, false);
   hw = CreateWindow(
      className,                    //lpClassName
      caption,                      //lpWindowName
      style,                        //dwStyle
      CW_USEDEFAULT,                //x
      0,                            //y
      wndRect.right - wndRect.left, //nWidth
      wndRect.bottom - wndRect.top, //nHeight
      NULL,                         //hWndParent
      NULL,                         //hMenu
      GetModuleHandle(NULL),        //hInstance
      NULL);                        //lParam
   foo = GetLastError();
   ShowWindow(hw, SW_SHOW);
   return hw;
}

MEM_HANDLE_T egl_server_platform_create_pixmap_info(uint32_t pixmap)
{
   VC_IMAGE_T *vcimage = (VC_IMAGE_T *)pixmap;
   KHRN_IMAGE_FORMAT_T format = 0;
   MEM_HANDLE_T data_handle;
   MEM_HANDLE_T handle;

   vcos_assert(pixmap);

   switch (vcimage->type) {
   case VC_IMAGE_TF_RGBA32:
      format = ABGR_8888_TF;
      break;
   case VC_IMAGE_TF_RGBX32:
      format = XBGR_8888_TF;
      break;
   case VC_IMAGE_TF_RGB565:
      format = RGB_565_TF;
      break;
   case VC_IMAGE_TF_RGBA5551:
      format = RGBA_5551_TF;
      break;
   case VC_IMAGE_TF_RGBA16:
      format = RGBA_4444_TF;
      break;
#ifdef __VIDEOCORE4__
   case VC_IMAGE_RGBA32:
      format = ABGR_8888_RSO;
      break;
   case VC_IMAGE_RGB565:
      format = RGB_565_RSO;
      break;
#endif
   default:
      UNREACHABLE();
   }

   if (vcimage->mem_handle != MEM_INVALID_HANDLE)
   {
      data_handle = vcimage->mem_handle;
      mem_acquire(data_handle);
   }
   else
      data_handle = mem_wrap(vcimage->image_data, vcimage->size, 64, MEM_FLAG_NONE, "egl_server_platform_create_pixmap_info");
   if (data_handle == MEM_INVALID_HANDLE) {
      return MEM_INVALID_HANDLE;
   }
   handle = khrn_image_create_from_storage(format,
      vcimage->width, vcimage->height, vcimage->pitch,
      MEM_INVALID_HANDLE, data_handle, 0,
      (KHRN_IMAGE_CREATE_FLAG_T)(IMAGE_CREATE_FLAG_TEXTURE | IMAGE_CREATE_FLAG_RENDER_TARGET)); /* todo: are these flags right? */
   mem_release(data_handle);
   return handle;
}

void egl_server_platform_init(EGL_SERVER_RETURN_CALLBACK_T return_callback)
{
   egl_callback = return_callback;
   the_hwnd = NULL;
   the_bitmap = NULL;
   the_dc = NULL;
}

void egl_server_platform_shutdown(void)
{
}

void egl_server_platform_set_position(uint32_t handle, uint32_t position, uint32_t width, uint32_t height)
{
    UNUSED(handle);
    UNUSED(position);
    UNUSED(width);
    UNUSED(height);
}



/*** Window thread ***/



static void window_thread_main(void *arg)
{
   MSG msg;

   the_hwnd = win32_create_window(the_width, the_height, L"EGL Window");

   vcos_assert(require_display_response);   //always require a response to startup
   require_display_response = 0;
   ReleaseSemaphore(display_response_semaphore, 1, NULL);
   while(GetMessage( &msg, NULL, 0, 0 ))
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }
}

static void paintwnd(HWND hwnd)
{
   PAINTSTRUCT ps;

   BeginPaint(hwnd, &ps);

   vcos_assert(the_hwnd == hwnd);

   WaitForSingleObject(the_mutex, INFINITE);
   if (the_dc && the_bitmap)
   {
      HGDIOBJ prev = SelectObject(the_dc, the_bitmap);
      BOOL result = BitBlt(GetDC(hwnd), 0, 0, the_width, the_height, the_dc, 0, 0, SRCCOPY);
      SelectObject(the_dc, prev);
   }

   if (require_display_response)
   {
      require_display_response = 0;
      ReleaseSemaphore(display_response_semaphore, 1, NULL);
   }

   ReleaseMutex(the_mutex);
   EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK win32_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg)
   {
   /*case WM_CREATE:
   case WM_SIZE:
   case WM_DESTROY:
      return 0;*/
   case WM_SIZING:
      {
         // cause the resize messages to be ignored
         RECT rt;
         UINT bx, by;
         /* stop the resize */
         LPRECT new_sz = (LPRECT)lParam;

         GetWindowRect(hwnd, &rt);
         by = (rt.bottom - rt.top);
         bx = (rt.right - rt.left);

         new_sz->right = new_sz->left + bx;
         new_sz->bottom = new_sz->top + by;
      }
      return 0;
   case WM_ERASEBKGND:
      // intercept this message to not clear the window on redraw
      return 1;
   case WM_DESTROY:
      exit(0);
   case WM_PAINT:
      paintwnd(hwnd);
      return 0;
   default:
      return DefWindowProc(hwnd, uMsg, wParam, lParam);
   }
   //return -1;
}
