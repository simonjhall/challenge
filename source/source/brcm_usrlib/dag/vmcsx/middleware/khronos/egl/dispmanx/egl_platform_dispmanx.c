#if 0
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
#ifdef DIRECT_RENDERING
#include "interface/khronos/include/EGL/eglext_brcm.h" /* for DIRECT_RENDERING */
#endif
//#include "middleware/dispmanx/dispmanx.h"
//#include "interface/vmcs_host/vc_dispmanx.h"
//#include "interface/vmcs_host/vc_dispmanx_types.h"
#include "helpers/vc_image/vc_image.h"

//#include "vcfw/logging/logging.h"
#include "interface/vchi/os/os.h"

#include "helpers/vclib/vclib.h"

#include <assert.h>
#ifdef ARTS_AUTOMATION
#include <stdlib.h>
#endif

//#define ALWAYS_HDMI
//#define DISPLAY_IS_BROKEN
//#define PRESSING_E_PUTS_IT_ON_HDMI
//#define BLOOM
//#define HDMI_14_3D

#if defined(DIRECT_RENDERING) && !defined(DISPLAY_IS_BROKEN)
/* We're not responsible for putting things on the display in DIRECT_RENDERING systems */
#define DISPLAY_IS_BROKEN
#endif

#ifdef PRESSING_E_PUTS_IT_ON_HDMI
#include "middleware/game/game.h"
#endif

/* KHRN_IMAGE_T raster order is bottom-to-top; VC_IMAGE_T raster order is top-to-bottom... */
#ifndef DISPLAY_IS_BROKEN
static bool flip_it(const VC_IMAGE_T *vc_image)
{
   return (vc_image->type == VC_IMAGE_RGBA32) || (vc_image->type == VC_IMAGE_RGBX32) || (vc_image->type == VC_IMAGE_RGB565);
}
#endif

#ifdef ARTS_AUTOMATION
static int arts_framenum=0;
static int arts_framecount=0;
static int arts_laststc=0, arts_thisstc=0;
static int arts_first = 0;
static int arts_last = 0;
#include "vcinclude/hardware.h"
#include <stdio.h>
void arts_setcapture(int first, int last)
{
   arts_first = first;
   arts_last = last;
}

unsigned int sillyhash(const unsigned char *ptr, int size)
{
   unsigned int hash;
   hash=777;
   while(size)
   {
      size--;
      hash=(hash*97)+(hash>>17);
      hash+=*ptr;
      ptr++;
   }
   return hash;
}

static void arts_display_hook(VC_IMAGE_T *image)
{
   arts_thisstc = STC;
   arts_framenum++;
   if(arts_framenum >= arts_first && arts_framenum < arts_last)
   {
      VC_IMAGE_CRC_T arts_crc;
      unsigned int arts_stc_before_crc;
      unsigned int arts_stc_after_crc;
      unsigned int arts_crc_us;
      unsigned int arts_sillyhash;
      unsigned int arts_stc_before_sillyhash;
      unsigned int arts_stc_after_sillyhash;
      unsigned int arts_sillyhash_us;
      unsigned int arts_interframetime_us;

      image->image_data = mem_lock(image->mem_handle);

      arts_stc_before_crc=STC;
      vclib_obtain_VRF(1);
      arts_crc=vc_image_calc_crc(image, 0, 0, 0, 0);
      vclib_release_VRF();
      arts_stc_after_crc=STC;
      arts_crc_us = arts_stc_after_crc - arts_stc_before_crc;
      arts_stc_before_sillyhash=STC;
      arts_sillyhash=sillyhash(image->image_data, image->size);
      arts_stc_after_sillyhash=STC;
      arts_sillyhash_us = arts_stc_after_sillyhash - arts_stc_before_sillyhash;

      mem_unlock(image->mem_handle);
      image->image_data = NULL;

      arts_interframetime_us = arts_thisstc - arts_laststc;
#if 1
      printf("CRC of image %d was 0x%08x\n", arts_framenum, arts_crc);
      printf("Time to compute CRC: %d.%03dms\n", arts_crc_us/1000, arts_crc_us%1000);
      printf("Silly Hash of image %d was 0x%08x\n", arts_framenum, arts_sillyhash);
      printf("Time to compute Silly Hash: %d.%03dms\n", arts_sillyhash_us/1000, arts_sillyhash_us%1000);
      printf("Time between calls to swap buffers: %d.%03dms\n", arts_interframetime_us/1000, arts_interframetime_us%1000);
#else
      printf("demo %d 0x%08x 0x%08x\n", arts_framenum, arts_crc.y, arts_sillyhash);
#endif

      arts_framecount++;
      if(arts_framecount >= ARTS_AUTOMATION_FRAME_COUNT)
      {
         exit(0);
      }
   }
   arts_laststc = arts_thisstc;
}
#endif

#ifdef EGL_SERVER_SMALLINT
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
#ifndef DISPLAY_IS_BROKEN
static DISPMANX_DISPLAY_HANDLE_T display;

#define ELEMENTS_N 4
static struct {
   DISPMANX_ELEMENT_HANDLE_T element;
   uint32_t win;
} elements[ELEMENTS_N] = {0};
vcos_static_assert(DISPMANX_NO_HANDLE == 0);

static void add_element(DISPMANX_ELEMENT_HANDLE_T element, uint32_t win)
{
   uint32_t i;
   for (i = 0; i != ELEMENTS_N; ++i) {
      if (elements[i].element == DISPMANX_NO_HANDLE) {
         elements[i].element = element;
         elements[i].win = win;
         return;
      }
   }
   assert(0); /* increase ELEMENTS_N? */
}

static void remove_element(DISPMANX_UPDATE_HANDLE_T update, uint32_t win)
{
   uint32_t i;
   for (i = 0; i != ELEMENTS_N; ++i) {
      if ((elements[i].element != DISPMANX_NO_HANDLE) && (elements[i].win == win)) {
         dispmanx_element_remove(update, elements[i].element);
         elements[i].element = DISPMANX_NO_HANDLE;
#ifndef BLOOM
         return;
#endif
      }
   }
   /* not finding isn't an error... */
}

static void our_callback(DISPMANX_UPDATE_HANDLE_T u, void * arg)
{
   egl_callback((uint32_t)arg);
}
#endif

static uint32_t hdmi_width = 1920;
static uint32_t hdmi_height = 1080;
#ifdef ALWAYS_HDMI
static int frame = 0;
static int32_t get_hdmi_best_mode(HDMI_MODE_T *hdmi_mode, int32_t *hdmi_support, HDMI_RES_GROUP_T *best_group, uint32_t *best_mode) {
   HDMI_RES_T prefer_res;
   uint32_t c[4] = {0};
   uint32_t d[4] = {0}; //the modes in bitmap format
   uint32_t n[4];

   int ret = hdmi_supported_modes(hdmi_mode, &prefer_res, c, d, n);

   //Select the following modes in descending priorities
   static const uint32_t best_cea_modes[] = {
      HDMI_CEA_1080p60,
      HDMI_CEA_1080p50,
      HDMI_CEA_1080i60,
      HDMI_CEA_1080i50,
      HDMI_CEA_1080p30,
      HDMI_CEA_720p60,
      HDMI_CEA_720p50
   };

   static const uint32_t best_dmt_modes[] = {
      HDMI_DMT_1080p_60,
      HDMI_DMT_SWXGAP_60,
      HDMI_DMT_SWXGAP_RB,
      HDMI_DMT_720p_60,
      HDMI_DMT_XGA_60
   };

   *best_group = HDMI_RES_GROUP_INVALID;
   *best_mode = 0;
   if(!ret) {
      int i;
      //There is a TV, choose the best mode, choose CEA preferrably
      if(c[0]||c[1]||c[2]||c[3]) {
         *best_group = HDMI_RES_GROUP_CEA;
         for(i = 0; i < sizeof(best_cea_modes)/sizeof(best_cea_modes[0]); i++) {
            if(c[best_cea_modes[i]/32] & (1 << (best_cea_modes[i]%32))) {
               //Found one
               *best_mode = best_cea_modes[i];
               break;
            }
         }
         if(*best_mode == 0) {
            //VGA must be supported
            *best_mode = HDMI_CEA_VGA;
         }

      } else if(d[0]||d[1]||d[2]||d[3]) {
         *best_group = HDMI_RES_GROUP_DMT;
         for(i = 0; i < sizeof(best_dmt_modes)/sizeof(best_dmt_modes[0]); i++) {
            if(d[best_dmt_modes[i]/32] & (1 << (best_dmt_modes[i]%32))) {
               //Found one
               *best_mode = best_dmt_modes[i];
               break;
            }
         }
         if(*best_mode == 0) {
            //VGA must be supported
            *best_mode = HDMI_DMT_VGA_60;
         }
      } else {
         ret = -1;
         UNREACHABLE(); //VGA is mandatory
      }
   }

   return ret;
}

#endif //ALWAYS_HDMI

#ifdef BLOOM
#include "helpers/vcbloom/vcbloom.h"
static uint32_t bloom_amount = 0x80;
static VCBLOOM_HANDLE bloom_handle = 0;
static void *bloom_tmp = 0;
static VC_IMAGE_T *bloom_result[3] = {0,0,0};
int bloom_i = 0;
#endif

void egl_server_platform_display(uint32_t win, KHRN_IMAGE_T *image, uint32_t cb_arg)
{
#ifdef DISPLAY_IS_BROKEN
   egl_callback(cb_arg);
#else
   uint32_t display_width, display_height, display_zoom;

   VC_RECT_T dr;
   VC_IMAGE_T vc_image;

   khrn_image_fill_vcimage(image, &vc_image);
   if (vc_image.type == VC_IMAGE_RGBX32) vc_image.type = VC_IMAGE_RGBA32;      /* XXX scalerlib can't handle it! */

#ifdef ARTS_AUTOMATION
   arts_display_hook(&vc_image);
#endif

#ifdef BLOOM
   VC_IMAGE_T *bloom_image;
   vc_image.width &= ~63;
   vc_image.height &= ~63;
   bloom_i = (bloom_i + 1) % 3;
#endif

   if (!inited) {
      inited = 1;

      dispmanx_init();
      display = dispmanx_display_open(display_num);

#ifdef BLOOM
      vcos_assert(bloom_handle == 0);
      bloom_result[0] = vc_image_parmalloc(vc_image.type, "Bloom result", vc_image.width / 4, vc_image.height / 4);
      bloom_result[1] = vc_image_parmalloc(vc_image.type, "Bloom result", vc_image.width / 4, vc_image.height / 4);
      bloom_result[2] = vc_image_parmalloc(vc_image.type, "Bloom result", vc_image.width / 4, vc_image.height / 4);
      bloom_handle = vcbloom_genbloom(bloom_result[0]->type, vc_image.type, 25, /*0,1*/7, 29, 1, 4, 1, 0, 0);
      bloom_tmp = malloc(vcbloom_calctmpbuffersize(bloom_handle, bloom_result[0], &vc_image));
      vcos_assert(bloom_handle != 0 && bloom_tmp != 0 && bloom_result[0] != 0 && bloom_result[1] != 0 && bloom_result[2] != 0);
#endif
   }
#ifdef BLOOM
   vc_image_lock(&vc_image, &vc_image);
   vclib_obtain_VRF(1);
   vcbloom_do(bloom_handle, bloom_result[bloom_i], &vc_image, bloom_tmp);
   vclib_release_VRF();
   vc_image_unlock(&vc_image);

   bloom_image = bloom_result[bloom_i];
#endif

#ifdef ALWAYS_HDMI
   frame++;
   //hdmi_hotplug_checker(NULL);
   if (display_num == 0)
   {
#if 0
      dispmanx_display_close(display);
      vcos_assert(display_num == 0);
      display_num = 2;
      HDMI_RES_T hdmi_res = {HDMI_RES_GROUP_DMT, /*HDMI_DMT_SWXGAP_60*//*HDMI_DMT_SWXGAP_RB*/HDMI_DMT_UXGA_60};
      hdmi_hotplug_checker(NULL);
      //_vasm("bkpt");
      hdmi_power_on(HDMI_MODE_DVI/*HDMI*/, hdmi_res, display_num);
      vcos_sleep(1000);
#endif

      HDMI_RES_T best_res = {HDMI_RES_GROUP_DMT, HDMI_DMT_SWXGAP_60/*HDMI_DMT_SWXGAP_RB*/};
      HDMI_MODE_T mode = HDMI_MODE_HDMI;

      if (1/*hdmi_get_mode() != VC_HDMI_UNPLUGGED*/)
      {
         int32_t hdmi_support;

         //int result = hdmi_supported_modes(&mode, &preferred_res, cea_modes, dmt_modes);
         //assert(result == 0);
         get_hdmi_best_mode(&mode, &hdmi_support, &best_res.group, &best_res.mode);
#ifdef HDMI_14_3D
         best_res.group = HDMI_RES_GROUP_CEA;
         best_res.mode = HDMI_CEA_1080p60;
#endif

         dispmanx_display_close(display);

         uint32_t frame_rate, interlaced;
         hdmi_get_screen_size(best_res, &hdmi_width, &hdmi_height, &frame_rate, &interlaced);

         hdmi_power_on(mode, best_res, 2);
         vcos_sleep(100);

         display_num = 2;


         while ((display = dispmanx_display_open(display_num)) == 0)
         {
            vcos_sleep(1000);
            _vasm("bkpt");
         }
#ifdef HDMI_14_3D
         {
            /* Set the info frame to enable 3D using half-height, full-width packed signals */
            //uint8_t info_frame[HDMI_NUM_PACKET_BYTES] = {0x00, 0x03, 0x0c, 0x00, 0x40, 0x60, };
            /* Set the info frame to enable 3D using full-height, half-width packed signals */
            uint8_t info_frame[HDMI_NUM_PACKET_BYTES] = {0x00, 0x03, 0x0c, 0x00, 0x40, 0x80, 0x00, };
            hdmi_set_infoframe(HDMI_INFOFRAME_VS, info_frame, /*5*/ 6);
         }
#endif
      }
   }
   else
   {
      if (0/*hdmi_get_mode() == VC_HDMI_UNPLUGGED*/)
      {
         dispmanx_display_close(display);
         display_num = 0;
         display = dispmanx_display_open(display_num);
      }
   }
#endif

#ifdef PRESSING_E_PUTS_IT_ON_HDMI
   if (game_button_pressed(GAME_KEY_E))
   {
      dispmanx_display_close(display);
      display_num ^= 2;
      display = dispmanx_display_open(display_num);
   }
#endif
#ifdef __VIDEOCORE4__
   if (display_num)
   {
      display_width = hdmi_width;
      display_height = hdmi_height;
      display_zoom = 1;
   }
   else
   {
      display_width = 800;
      display_height = 480;
      display_zoom = 0;
   }
#else
   display_width = display_num ? 1280 : 800;
   display_height = display_num ? 720 : 480;
   display_zoom = display_num ? 1 : 0;
#endif

   DISPMANX_UPDATE_HANDLE_T update = dispmanx_update_start(0);

   remove_element(update, win);
   if (display) {
      uint32_t w,h;
      DISPMANX_ALPHA_T alpha = {DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 255, 0};

      w = vc_image.width;
      h = vc_image.height;

#ifdef KHRN_SIMPLE_MULTISAMPLE
      w /= 2;
      h /= 2;
#endif

      if (display_zoom)
      {
         float zoom0 = (float)display_width / (float)w;
         float zoom1 = (float)display_height / (float)h;

         if (zoom0 < zoom1)
         {
            w = (int)(w * zoom0);
            h = (int)(h * zoom0);
         }
         else
         {
            w = (int)(w * zoom1);
            h = (int)(h * zoom1);
         }
      }

      dr.x = ((display_width - (UNPACK_NATIVE_WINDOW_N(win) * w)) >> 1) + (UNPACK_NATIVE_WINDOW_I(win) * w);
      dr.width = w;

      //VC_RECT_T sr = {0, 0, image->width<<8, image->height<<8};
      if (h > display_height) {
         dr.y               = 0;
         dr.height          = display_height;
      } else {
         dr.y               = display_height - h >> 1;
         dr.height          = h;
      }
#ifdef BLOOM
      add_element(dispmanx_element_add(update, display, 0, &dr, &vc_image, 0, DISPMANX_PROTECTION_NONE, &alpha, 0/*clamp*/,
         flip_it(&vc_image) ? DISPMANX_FLIP_VERT : DISPMANX_NO_ROTATE), win);
      if (bloom_amount)
      {
         alpha.opacity = bloom_amount;
         add_element(dispmanx_element_add(update, display, 0, &dr, bloom_image, 0, DISPMANX_PROTECTION_NONE, &alpha, 0/*clamp*/,
            DISPMANX_NO_ROTATE), win);
      }
#else
      add_element(dispmanx_element_add(update, display, 0, &dr, &vc_image, 0, DISPMANX_PROTECTION_NONE, &alpha, 0/*clamp*/,
         flip_it(&vc_image) ? DISPMANX_FLIP_VERT : DISPMANX_NO_ROTATE), win);
#endif
   }

   dispmanx_update_submit(update, our_callback, (void *)cb_arg);
#endif
}

void egl_server_platform_display_nothing_sync(uint32_t win)
{
#ifndef DISPLAY_IS_BROKEN
   vcos_assert(win != EGL_PLATFORM_WIN_NONE);

   DISPMANX_UPDATE_HANDLE_T update = dispmanx_update_start(0);

   remove_element(update, win);
   dispmanx_update_submit_sync(update);
#endif
}

MEM_HANDLE_T egl_server_platform_create_pixmap_info(uint32_t pixmap)
{
   UNREACHABLE();
   return MEM_INVALID_HANDLE;
}

#if EGL_BRCM_perf_monitor
/*
   Show or hide the performance monitor surface.
*/

static VC_IMAGE_T perf_monitor_vc_image;
static DISPMANX_ELEMENT_HANDLE_T perf_monitor_element = 0;

void egl_server_platform_perf_monitor_show(struct VC_IMAGE_T *image)
{
   memcpy(&perf_monitor_vc_image, image, sizeof(VC_IMAGE_T));

   if (!inited) {
      inited = 1;

      dispmanx_init();

      display = dispmanx_display_open(display_num);
   }

   DISPMANX_UPDATE_HANDLE_T update = dispmanx_update_start(0);

   if (perf_monitor_element != DISPMANX_NO_HANDLE) {
      dispmanx_element_remove(update, perf_monitor_element);
      perf_monitor_element = DISPMANX_NO_HANDLE;
   }
   if (display) {
      VC_RECT_T dr = {0, 0, 800, 480};
      DISPMANX_ALPHA_T alpha = {DISPMANX_FLAGS_ALPHA_FROM_SOURCE, 128, 0};

      perf_monitor_element = dispmanx_element_add(update, display, 1, &dr, &perf_monitor_vc_image, NULL, DISPMANX_PROTECTION_NONE, &alpha, 0/*clamp*/, VC_IMAGE_ROT0);
   }

   dispmanx_update_submit_sync(update);
}

void egl_server_platform_perf_monitor_hide()
{
   DISPMANX_UPDATE_HANDLE_T update = dispmanx_update_start(0);

   if (perf_monitor_element != DISPMANX_NO_HANDLE) {
      dispmanx_element_remove(update, perf_monitor_element);
      perf_monitor_element = DISPMANX_NO_HANDLE;
   }

   dispmanx_update_submit_sync(update);
}
#endif

#else  //EGL_SERVER_SMALLINT

static EGL_SERVER_RETURN_CALLBACK_T egl_callback;

#ifndef DISPLAY_IS_BROKEN
static void our_callback(DISPMANX_UPDATE_HANDLE_T u, void * arg)
{
   egl_callback((uint32_t)arg);
}
#endif

void egl_server_platform_display(uint32_t win, KHRN_IMAGE_T *image, uint32_t cb_arg)
{
#ifdef DISPLAY_IS_BROKEN
   egl_callback(cb_arg);
#else
   DISPMANX_UPDATE_HANDLE_T update = dispmanx_update_start(0);
   VC_IMAGE_T vc_image;

   khrn_image_fill_vcimage(image, &vc_image);
   if (vc_image.type == VC_IMAGE_RGBX32) vc_image.type = VC_IMAGE_RGBA32;      /* XXX scalerlib can't handle it! */

#ifdef ARTS_AUTOMATION
   arts_display_hook(&vc_image);
#endif

   dispmanx_element_change_source(update, (DISPMANX_ELEMENT_HANDLE_T)win, &vc_image);

   if (flip_it(&vc_image))
      dispmanx_element_change_transform(update, (DISPMANX_ELEMENT_HANDLE_T)win, DISPMANX_FLIP_VERT);

   dispmanx_update_submit(update, our_callback, (void *)cb_arg);
#endif
}

void egl_server_platform_display_nothing_sync(uint32_t win)
{
#ifndef DISPLAY_IS_BROKEN
   DISPMANX_UPDATE_HANDLE_T update = dispmanx_update_start(0);

   dispmanx_element_change_source(update, (DISPMANX_ELEMENT_HANDLE_T)win, 0);
   dispmanx_update_submit_sync(update);
#endif
}

MEM_HANDLE_T egl_server_platform_create_pixmap_info(uint32_t pixmap)
{
   VC_IMAGE_T *vcimage = (VC_IMAGE_T *)pixmap;
   KHRN_IMAGE_FORMAT_T format = 0;
   MEM_HANDLE_T data_handle;

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
   MEM_HANDLE_T handle = khrn_image_create_from_storage(format,
      vcimage->width, vcimage->height, vcimage->pitch,
      MEM_INVALID_HANDLE, data_handle, 0,
      (KHRN_IMAGE_CREATE_FLAG_T)(IMAGE_CREATE_FLAG_TEXTURE | IMAGE_CREATE_FLAG_RENDER_TARGET)); /* todo: are these flags right? */
   mem_release(data_handle);
   return handle;
}

void egl_server_platform_init(EGL_SERVER_RETURN_CALLBACK_T return_callback)
{
   egl_callback = return_callback;
}

void egl_server_platform_shutdown(void)
{
}

#endif  //EGL_SERVER_SMALLINT

void egl_server_platform_set_position(uint32_t win, uint32_t position, uint32_t width, uint32_t height)
{
}


#endif
