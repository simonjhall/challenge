/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

extern unsigned int memory_pool_size;
#define KHRN_LATE_COLOR_BUFFER_ALLOC

#include "middleware/khronos/glxx/glxx_buffer.h"
#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/common/khrn_hw.h"
#include "middleware/khronos/common/khrn_image.h"
#include "middleware/khronos/common/khrn_interlock.h"
#include "middleware/khronos/common/khrn_llat.h"
#include "middleware/khronos/common/khrn_mem.h"
#if !defined(V3D_LEAN)
#include "middleware/khronos/common/khrn_misc.h"
#endif
#include "middleware/khronos/glxx/glxx_texture.h"
#include "interface/khronos/common/khrn_int_ids.h"
#include "middleware/khronos/dispatch/khrn_dispatch_rpc.h"  /* for khdispatch_send_async */
#include "middleware/khronos/gl11/gl11_server.h"
#include "middleware/khronos/glxx/glxx_hw.h"
#include "middleware/khronos/gl20/gl20_server.h"
#include "middleware/khronos/gl20/gl20_program.h"
#include "middleware/khronos/gl20/gl20_shader.h"
#include "middleware/khronos/glxx/glxx_framebuffer.h"
#include "middleware/khronos/glxx/glxx_renderbuffer.h"
#include "middleware/khronos/vg/vg_server.h"
#include "middleware/khronos/vg/vg_font.h"
#include "middleware/khronos/vg/vg_image.h"
#include "middleware/khronos/vg/vg_mask_layer.h"
#include "middleware/khronos/vg/vg_paint.h"
#include "middleware/khronos/vg/vg_path.h"
#include "middleware/khronos/vg/vg_ramp.h"
#include "middleware/khronos/egl/egl_server.h"
#include "middleware/khronos/egl/egl_platform.h"
#include "middleware/khronos/egl/egl_disp.h"
#include "interface/khronos/include/EGL/egl.h"
#include "interface/khronos/include/EGL/eglext.h"
#ifdef __VIDEOCORE__
#include "helpers/dmalib/dmalib.h"
#endif
#include "helpers/vc_image/vc_image.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#ifdef ANDROID
//#define LOG_NDEBUG 0
#define LOG_TAG	"EGL_SERVER"
#include <cutils/log.h>
#include "vcfw/drivers/chip/v3d.h"
#endif

#define LOGE printf
#define LOGV printf

#include "interface/khronos/egl/egl_int_config.h"

#ifdef SIMPENROSE
#include "v3d/verification/tools/2760sim/simpenrose.h"
#endif

#if EGL_KHR_image
   #include "middleware/khronos/ext/egl_khr_image.h"
#endif

/* must be after EGL/eglext.h */
#if EGL_BRCM_global_image
   #include "middleware/khronos/ext/egl_brcm_global_image.h"
#endif

#ifdef ANDROID
#include <ui/android_native_buffer.h>
#include <ui/egl/android_natives.h>
gralloc_module_t const* module = NULL;
#endif

/*
   void egl_server_surface_term(void *v, uint32_t size)

   Terminator for EGL_SERVER_SURFACE_T

   Preconditions:

   v is a valid pointer to a (possibly uninitialised or partially initialised*) object of type EGL_SERVER_SURFACE_T

   Postconditions:

   Frees any references to resources that are referred to by the object of type EGL_SERVER_SURFACE_T
*/

void egl_server_surface_term(void *v, uint32_t size)
{
   EGL_SERVER_SURFACE_T *surface = (EGL_SERVER_SURFACE_T *)v;

   int i;

   UNUSED_NDEBUG(size);
   vcos_assert(size == sizeof(EGL_SERVER_SURFACE_T));

   if (surface->mh_color[0] != MEM_INVALID_HANDLE) {
      ((KHRN_IMAGE_T *)mem_lock(surface->mh_color[0]))->flags &= ~IMAGE_FLAG_BOUND_CLIENTBUFFER;
      mem_unlock(surface->mh_color[0]);
   }

   for (i = 0; i < EGL_MAX_BUFFERS; i++)
      MEM_ASSIGN(surface->mh_color[i], MEM_INVALID_HANDLE);

   MEM_ASSIGN(surface->mh_depth, MEM_INVALID_HANDLE);
   MEM_ASSIGN(surface->mh_multi, MEM_INVALID_HANDLE);
   MEM_ASSIGN(surface->mh_mask, MEM_INVALID_HANDLE);

   /* todo: why are we responsible for destroying this, but not creating it? i
    * expect there are cases where we forget to destroy it... */
   if ((surface->buffers > 1)&&(surface->sem!=0))
      khdispatch_send_async(ASYNC_COMMAND_DESTROY, surface->pid, surface->sem);

   MEM_ASSIGN(surface->mh_bound_texture, MEM_INVALID_HANDLE);

#ifndef BRCM_V3D_OPT
   if (surface->disp_handle != EGL_DISP_HANDLE_INVALID) {
      egl_disp_free(surface->disp_handle);
   }
#endif
}

bool egl_server_state_initted = false;
EGL_SERVER_STATE_T egl_server_state;
#if !defined(VCMODS_NEWMEM)
static int total_mem_size = 0;
#endif

static uint32_t override_size = 0x2000000;
static uint32_t override_handles_size = 0;
#ifndef ANDROID
#if defined( MM_DATA_INCLUDED )
//#include "osheap.h"
static char* mm_mem_pool_name = "mm_mem_heap";
static Heap_t *mmMemPool;
#endif
#endif

#if !defined(VCMODS_NEWMEM)
static void *mempool_base_alloced, *mempool_handles_base_alloced;
#endif

/* todo: allow this to fail */
void EGLAPIENTRY egl_server_startup_hack()
{
   EGL_SERVER_STATE_T *state;

   if (egl_server_state_initted)
      return;

#ifdef ANDROID
#ifdef RPC_DIRECT
   vcos_init();
   v3d_get_func_table()->init();
#endif
#endif

   {
#if !defined(VCMODS_NEWMEM)
   void *mempool_base = 0; uint32_t mempool_size = 0; void *mempool_handles_base = 0; uint32_t mempool_handles_size = 0;
#if defined(FOR_ARTS)
   mempool_size = 20 * 1024 * 1024 + 65536;
   mempool_handles_size = 4096 * 4; //sizeof(MEM_MANAGER_HANDLE_T)
#elif defined(__CC_ARM) && !defined(SIMPENROSE)
#ifdef ANDROID
   mempool_base = (void *)MEMPOOL_BASE;
   mempool_size = MEMPOOL_SIZE;
   mempool_handles_size = 5 * 4096 * 4; //sizeof(MEM_MANAGER_HANDLE_T)
#else // ANDROID
#ifdef MM_DATA_INCLUDED
   mmMemPool = OSHEAP_GetBytePoolByName(mm_mem_pool_name);
   if (!mmMemPool) assert(0);
#endif    
   if (override_size)
      mempool_size = override_size;
   if (override_handles_size)
      mempool_handles_size = override_handles_size;   
   else
      mempool_handles_size = 5 * 4096 * 4; //sizeof(MEM_MANAGER_HANDLE_T)   
#ifdef MM_DATA_INCLUDED      
   mempool_base_alloced = mempool_base = (void*)OSHEAP_AllocBytePoolMem(mmMemPool, mempool_size);
#endif   
#endif // ANDROID
#elif defined(SIMPENROSE)
   static bool inited_simpenrose = false;
   if (!inited_simpenrose) {
      simpenrose_init_hardware();
      inited_simpenrose = true;
   }
   mempool_base = simpenrose_get_mem_start();
   mempool_size = simpenrose_get_mem_size();
   mempool_handles_size = 32768 * 4; //sizeof(MEM_MANAGER_HANDLE_T)
#else
   mem_get_default_partition(&mempool_base, &mempool_size, &mempool_handles_base, &mempool_handles_size);
#endif
#ifdef ANDROID
   if (override_handles_size) {
      if (override_handles_size <= mempool_handles_size)
         LOGW("%s: No need to override handles size. mempool_handles_size = %d, override_handles_size = %d\n", __FUNCTION__, mempool_handles_size, override_handles_size);
      mempool_handles_size = override_handles_size;
   }
#else
   if (override_size)
      mempool_size = override_size;
   if (override_handles_size)
      mempool_handles_size = override_handles_size;
#endif
#ifdef BRCM_V3D_OPT
	if (mempool_handles_base==NULL)
	   mempool_handles_base_alloced = mempool_handles_base = malloc(mempool_handles_size);
	mem_init(mempool_base, mempool_size, mempool_handles_base, mempool_handles_size);
	total_mem_size = memory_pool_size;
#else
   if (mempool_base==NULL)
      mempool_base_alloced = mempool_base = malloc(mempool_size);
   if (mempool_handles_base==NULL)
      mempool_handles_base_alloced = mempool_handles_base = malloc(mempool_handles_size);

   mem_init(mempool_base, mempool_size, mempool_handles_base, mempool_handles_size);
   total_mem_size = mempool_size;
#endif
#endif /* VCMODS_NEWMEM */
   egl_server_state_initted = true;

   state = EGL_GET_SERVER_STATE();

   memset(state, MEM_HANDLE_INVALID, sizeof(EGL_SERVER_STATE_T));        // do not remove, sets all handles to invalid

   state->glversion = 0;
   state->next_context = 1;
   state->next_surface = 1;
   state->next_eglimage = 1;
#if EGL_KHR_sync
   state->next_sync = 1;
#endif

   state->locked_glcontext = NULL;
   state->locked_vgcontext = NULL;
   state->locked_vgcontext_shared = NULL;
   state->locked_vgcontext_shared_objects_storage = NULL;

   verify(khrn_map_init(&state->glcontexts, 64)); /* todo: handle failure */
   verify(khrn_map_init(&state->vgcontexts, 64)); /* todo: handle failure */
   verify(khrn_map_init(&state->surfaces, 64)); /* todo: handle failure */
   verify(khrn_map_init(&state->eglimages, 64)); /* todo: handle failure */

#if EGL_BRCM_global_image
   verify(egl_brcm_global_image_init()); /* todo: handle failure */
#endif
#if EGL_KHR_sync
   verify(khrn_map_init(&state->syncs, 8)); /* todo: handle failure */
#endif
#if EGL_BRCM_perf_monitor
   state->perf_monitor_refcount = 0;
#endif

   vcos_verify(VCOS_SUCCESS==vcos_init());

#ifndef BRCM_V3D_OPT
   khrn_llat_init(); /* todo: handle failure */
#endif
   khrn_hw_common_init(); /* todo: handle failure */
#ifndef BRCM_V3D_OPT
   egl_server_platform_init(egl_disp_callback);
   egl_disp_init(); /* todo: handle failure */
#endif
   }

#ifdef ANDROID
   if (module == NULL) {
      hw_module_t const* pModule;
      hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &pModule);
      module = (gralloc_module_t const*)(pModule);
   }
#endif
}

void EGLAPIENTRY egl_server_startup_hack_set_size(uint32_t size, uint32_t handles)
{
   override_size = round_up(size, 32);
   override_handles_size = handles * 4; //sizeof(MEM_MANAGER_HANDLE_T)
}

/*
 * Returns true if there are no objects left and so we can shutdown without
 * losing any information
 */
bool egl_server_is_empty()
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   return
#if EGL_BRCM_global_image
      egl_brcm_global_image_is_empty() &&
#endif
      khrn_map_get_count(&state->surfaces) == 0 &&
      khrn_map_get_count(&state->glcontexts) == 0 &&
      khrn_map_get_count(&state->vgcontexts) == 0 &&
      khrn_map_get_count(&state->eglimages) == 0 &&
#if EGL_KHR_sync
      khrn_map_get_count(&state->syncs) == 0 &&
#endif
      state->glcontext == MEM_INVALID_HANDLE &&  //this implies the surfaces are invalid too
      state->vgcontext == MEM_INVALID_HANDLE;
}

/*
 * Normally only called when egl_server_is_empty is true, but if it isn't then
 * we free all our stuff.
 * This shuts down everything set up by egl_server_startup_hack. At present this
 * includes the entire memory system.
 */
void egl_server_shutdown()
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   /*
      we have to be careful here about shutting things down in the right order!

      the first thing we do is flush all queued stuff and wait for it to finish
      (we might still have queued stuff as we are quite lazy about flushing it)

      (this doesn't include display stuff: we'll wait for each egl surface
      individually before it is destroyed)
   */

   khrn_hw_common_finish();

   /*
      now we destroy all egl objects
   */

#if EGL_BRCM_global_image
   egl_brcm_global_image_term();
#endif

   khrn_map_term(&state->surfaces);
   khrn_map_term(&state->glcontexts);
   khrn_map_term(&state->vgcontexts);
   khrn_map_term(&state->eglimages);

#if EGL_KHR_sync
   khrn_map_term(&state->syncs);
#endif

   MEM_ASSIGN(state->glcontext, MEM_INVALID_HANDLE);
   MEM_ASSIGN(state->gldrawsurface, MEM_INVALID_HANDLE);
   MEM_ASSIGN(state->glreadsurface, MEM_INVALID_HANDLE);
   MEM_ASSIGN(state->vgcontext, MEM_INVALID_HANDLE);
   MEM_ASSIGN(state->vgsurface, MEM_INVALID_HANDLE);

#if EGL_BRCM_perf_monitor
   MEM_ASSIGN(state->perf_monitor_images[0], MEM_INVALID_HANDLE);
   MEM_ASSIGN(state->perf_monitor_images[1], MEM_INVALID_HANDLE);
#endif

   /*
      vg might be marked for termination...
   */

   vg_maybe_term();

   /*
      no queued stuff and we've waited for everything to finish (including
      display stuff)

      make sure there aren't any spurious notifies flying around
   */

#ifndef BRCM_V3D_OPT
   khrn_llat_wait();
#endif

   /*
      at this point, the system is completely idle and empty

      shutdown everything from top to bottom...
   */

#ifndef BRCM_V3D_OPT
   egl_disp_term();
   egl_server_platform_shutdown();
#endif
   glxx_hw_term();
   khrn_hw_common_term();
#ifndef BRCM_V3D_OPT
   khrn_llat_term();
#endif

#ifdef ANDROID
#ifdef RPC_DIRECT
   v3d_get_func_table()->exit();
   vcos_deinit();
#endif
#endif

   /*
      finally, tear down the heap
   */

#ifndef VCMODS_NEWMEM
#define CHECK_MEMORY_LEAKS
#ifdef CHECK_MEMORY_LEAKS
   /*
    * Memory should be empty at this point. Check for memory leaks by
    * allocating something filling almost the entire memory
    */
   vcos_assert(mem_alloc(total_mem_size-1024, 1, MEM_FLAG_NONE, "Check for leaks") != MEM_INVALID_HANDLE);
#endif
   mem_term();

#if defined(__CC_ARM)
#ifdef ANDROID
    if (mempool_base_alloced) {
      free(mempool_base_alloced);
      mempool_base_alloced = NULL;
    }
#else // ANDROID
#ifdef MM_DATA_INCLUDED
    if (mempool_base_alloced) {
      OSHEAP_FreeBytePoolMem(mmMemPool, mempool_base_alloced);
      mempool_base_alloced = NULL;    
    }    
#endif    
#endif // ANDROID
#endif
 
   if (mempool_base_alloced) {
      free(mempool_base_alloced);
      mempool_base_alloced = NULL;
   }
   if (mempool_handles_base_alloced) {
      free(mempool_handles_base_alloced);
      mempool_handles_base_alloced = NULL;
   }
#endif /* VCMODS_NEWMEM */

   egl_server_state_initted = false;
}

void egl_server_unlock()
{
//TODO: check for thread safe
#if defined(RPC_DIRECT_MULTI)
	if (egl_server_state_initted == false)
		return;
#endif

   GL11_FORCE_UNLOCK_SERVER_STATE();
   GL20_FORCE_UNLOCK_SERVER_STATE();
#ifndef NO_OPENVG
   vg_unlock();
#endif /* NO_OPENVG */
}

/*
   EGL_SURFACE_ID_T create_surface_internal(
      uint32_t win,
      uint32_t buffers,
      uint32_t width,
      uint32_t height,
      KHRN_IMAGE_FORMAT_T colorformat,
      KHRN_IMAGE_FORMAT_T depthstencilformat,
      KHRN_IMAGE_FORMAT_T maskformat,
      uint32_t multisample,
      uint32_t mipmap,
      MEM_HANDLE_T vgimagehandle,
      MEM_HANDLE_T hpixmapimage,
      uint32_t sem)

   Preconditions:
      win is a valid server-side window handle or PLATFORM_WIN_NONE
      1 <= buffers <= EGL_MAX_BUFFERS
      0 < width <= EGL_CONFIG_MAX_WIDTH
      0 < height <= EGL_CONFIG_MAX_HEIGHT
      colorformat is a hardware framebuffer-supported uncompressed color format
      depthstencilformat is a hardware framebuffer-supported depth and/or stencil format, or IMAGE_FORMAT_INVALID
      maskformat is a hardware framebuffer-supported mask format, or IMAGE_FORMAT_INVALID
      multisample in {0,1}
      mipmap in {0,1}
      vgimagehandle is MEM_INVALID_HANDLE or a handle to a VG_IMAGE
      hpixmapimage is MEM_INVALID_HANDLE or a handle to a KHRN_IMAGE_T

      Exactly one of the following is true:
          win && !mipmap && !vgimagehandle && !hpixmapimage
         !win && !mipmap && !vgimagehandle && !hpixmapimage && buffers == 1
         !win &&  mipmap && !vgimagehandle && !hpixmapimage
         !win && !mipmap &&  vgimagehandle && !hpixmapimage && buffers == 1
         !win && !mipmap && !vgimagehandle &&  hpixmapimage && buffers == 1
      where win, vgimagehandle, hpixmapimage are "true" if they are not equal to EGL_PLATFORM_WIN_NONE, MEM_INVALID_HANDLE, MEM_INVALID_HANDLE

   Postconditions:

      Return value is 0 or
*/

static EGL_SURFACE_ID_T create_surface_internal(
   uint32_t win,
   uint32_t buffers,
   uint32_t width,
   uint32_t height,
   KHRN_IMAGE_FORMAT_T colorformat,
   KHRN_IMAGE_FORMAT_T depthstencilformat,
   KHRN_IMAGE_FORMAT_T maskformat,
   KHRN_IMAGE_FORMAT_T multisampleformat,
   uint32_t mipmap,
   MEM_HANDLE_T vgimagehandle,
   MEM_HANDLE_T hpixmapimage,      /*floating KHRN_IMAGE_T*/
   uint32_t sem)
{
   EGL_SERVER_SURFACE_T *surface;
   /* todo: these flags aren't entirely correct... */
   KHRN_IMAGE_CREATE_FLAG_T color_image_create_flags = (KHRN_IMAGE_CREATE_FLAG_T)(
      /* clear, but also mark as invalid */
      IMAGE_CREATE_FLAG_ONE | IMAGE_CREATE_FLAG_INVALID |
      /* if we're a window surface, might rotate/display */
      ((win != EGL_PLATFORM_WIN_NONE) ? (IMAGE_CREATE_FLAG_PAD_ROTATE | IMAGE_CREATE_FLAG_DISPLAY) : 0) |
      /* support rendering to if format is ok (don't force the format) */
      (khrn_image_is_ok_for_render_target(colorformat, false) ? (IMAGE_CREATE_FLAG_RENDER_TARGET |
      /* also support use as texture if not window surface (todo: this is hacky, and i don't think it's right) */
      ((win == EGL_PLATFORM_WIN_NONE) ? IMAGE_CREATE_FLAG_TEXTURE : 0)) : 0));
   KHRN_IMAGE_CREATE_FLAG_T image_create_flags = (KHRN_IMAGE_CREATE_FLAG_T)(
      /* clear, but also mark as invalid */
      IMAGE_CREATE_FLAG_ONE | IMAGE_CREATE_FLAG_INVALID |
      /* if we're a window surface, might rotate */
      ((win != EGL_PLATFORM_WIN_NONE) ? IMAGE_CREATE_FLAG_PAD_ROTATE : 0) |
      /* aux buffers are just there for rendering... */
      IMAGE_CREATE_FLAG_RENDER_TARGET);
   MEM_HANDLE_T hcommon_storage = MEM_INVALID_HANDLE;
   EGL_SURFACE_ID_T result = 0;

   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   MEM_HANDLE_T shandle = MEM_ALLOC_STRUCT_EX(EGL_SERVER_SURFACE_T, MEM_COMPACT_DISCARD);                   // check, egl_server_surface_term
   if (shandle == MEM_INVALID_HANDLE)
      return result;

   mem_set_term(shandle, egl_server_surface_term);

   surface = (EGL_SERVER_SURFACE_T *)mem_lock(shandle);

   /* make it safe to call term... */
   surface->sem = 0;
   surface->disp_handle = EGL_DISP_HANDLE_INVALID;

   if (mipmap) {
      vcos_assert(width > 0 && height > 0);
      vcos_assert(width <= EGL_CONFIG_MAX_WIDTH && height <= EGL_CONFIG_MAX_HEIGHT);
      buffers = _max(_msb(width), _msb(height)) + 1;
   }

   vcos_assert(buffers > 0);
   vcos_assert(buffers <= EGL_MAX_BUFFERS);
   vcos_assert(!mipmap || win == EGL_PLATFORM_WIN_NONE);
   vcos_assert(mipmap || win != EGL_PLATFORM_WIN_NONE || buffers == 1);

   surface->name = state->next_surface;
   surface->mipmap = mipmap;
   surface->win = win;
   surface->buffers = buffers;
   surface->back_buffer_index = 0;
   vcos_assert(surface->mh_bound_texture == MEM_INVALID_HANDLE);
   surface->swap_interval = 1;
   surface->pid = state->pid;
   surface->sem = sem;

   if (hpixmapimage != MEM_INVALID_HANDLE) {
      // Can't combine pixmap wrapping with any of the following features
      vcos_assert(buffers == 1);
      vcos_assert(!mipmap);
      vcos_assert(win == EGL_PLATFORM_WIN_NONE);
      vcos_assert(vgimagehandle == MEM_INVALID_HANDLE);

      /*TODO: accept handle-based images as well as wrapped ones? */

      MEM_ASSIGN(surface->mh_color[0], hpixmapimage);
   } else {
      uint32_t i;
      uint32_t offset = 0;

      if (mipmap)
      {
         uint32_t size = 0;
         for (i = 0; i < buffers; i++)
         {
            uint32_t w,h;
            w = _max(width >> i, 1);
            h = _max(height >> i, 1);
            size += khrn_image_get_size(colorformat, w, h);
         }
         hcommon_storage = mem_alloc_ex(size, 4096, MEM_FLAG_DIRECT, "EGL_SERVER_SURFACE_T mipmapped storage", MEM_COMPACT_DISCARD);
         if (hcommon_storage == MEM_INVALID_HANDLE)
            goto final;
      }


      for (i = 0; i < buffers; i++) {
         if (i == 0 && vgimagehandle != MEM_INVALID_HANDLE) {
            VG_IMAGE_T *vgimage = (VG_IMAGE_T *)mem_lock(vgimagehandle);
            MEM_ASSIGN(surface->mh_color[i], vgimage->image);
            mem_unlock(vgimagehandle);
         } else {
            uint32_t w,h;
            MEM_HANDLE_T himage;
            if (mipmap) {
               w = _max(width >> i, 1);
               h = _max(height >> i, 1);
            } else {
               w = width;
               h = height;
            }

            if (hcommon_storage != MEM_INVALID_HANDLE)
            {
               himage = khrn_image_create_from_storage(
                  colorformat, w, h,
                  khrn_image_get_stride(colorformat, w),
                  MEM_INVALID_HANDLE,
                  hcommon_storage,
                  offset,
                  (KHRN_IMAGE_CREATE_FLAG_T)(IMAGE_CREATE_FLAG_TEXTURE | IMAGE_CREATE_FLAG_RENDER_TARGET)); /* todo: are these flags right? should IMAGE_CREATE_FLAG_INVALID be included? */
               offset += khrn_image_get_size(colorformat, w, h);
            }
            else
            {
#ifdef DIRECT_RENDERING
               /*
               Don't create storage for image. The storage handle will be supplied to us when the surface
               is first made current (and new ones will be supplied on eglSwapBuffers).

               (via eglDirectRenderingPointer_impl)
               */
               color_image_create_flags = (KHRN_IMAGE_CREATE_FLAG_T)(IMAGE_CREATE_FLAG_NO_STORAGE | color_image_create_flags);
#endif
               himage = khrn_image_create(colorformat, w, h, color_image_create_flags);
            }
            if (himage == MEM_INVALID_HANDLE)
               goto final;

            MEM_ASSIGN(surface->mh_color[i], himage);
            mem_release(himage);
         }
      }
   }

   /*
       "The contents of the depth and stencil buffers may not be preserved when rendering
       an OpenGL ES texture to the pbuffer and switching which image of the
       texture is rendered to (e.g., switching from rendering one mipmap level to rendering
       another)."
       So in the mipmapped case, we have just one depth/stencil buffer which is
       big enough for the largest mipmap but which we reuse for all mipmaps.
   */

   if (depthstencilformat != IMAGE_FORMAT_INVALID) {
      MEM_HANDLE_T himage;
      if (multisampleformat != IMAGE_FORMAT_INVALID)
         himage = khrn_image_create(depthstencilformat, 2*width, 2*height,
#ifdef __VIDEOCORE4__
            (KHRN_IMAGE_CREATE_FLAG_T)(image_create_flags | IMAGE_CREATE_FLAG_NO_STORAGE)
#else
            image_create_flags
#endif
            );
      else
         himage = khrn_image_create(depthstencilformat, width, height,
#ifdef __VIDEOCORE4__
            (KHRN_IMAGE_CREATE_FLAG_T)(image_create_flags | IMAGE_CREATE_FLAG_NO_STORAGE)
#else
            image_create_flags
#endif
            );

      if (himage == MEM_INVALID_HANDLE)
         goto final;

      MEM_ASSIGN(surface->mh_depth, himage);
      mem_release(himage);
   } else
      vcos_assert(surface->mh_depth == MEM_INVALID_HANDLE);

   if (multisampleformat != IMAGE_FORMAT_INVALID) {
      MEM_HANDLE_T himage = khrn_image_create(multisampleformat, 2*width, 2*height,
#ifdef __VIDEOCORE4__
         (KHRN_IMAGE_CREATE_FLAG_T)(image_create_flags | IMAGE_CREATE_FLAG_NO_STORAGE)
#else
         image_create_flags
#endif
         );
      if (himage == MEM_INVALID_HANDLE)
         goto final;

      MEM_ASSIGN(surface->mh_multi, himage);
      mem_release(himage);
   } else
      vcos_assert(surface->mh_multi == MEM_INVALID_HANDLE);

   if (maskformat != IMAGE_FORMAT_INVALID) {
      MEM_HANDLE_T himage = khrn_image_create(maskformat, width, height, image_create_flags);
      if (himage == MEM_INVALID_HANDLE)
         goto final;

      MEM_ASSIGN(surface->mh_mask, himage);
      mem_release(himage);
   } else
      vcos_assert(surface->mh_mask == MEM_INVALID_HANDLE);

   if (win != EGL_PLATFORM_WIN_NONE) {
      surface->disp_handle = egl_disp_alloc(state->pid, sem, buffers, surface->mh_color);
      if (surface->disp_handle == EGL_DISP_HANDLE_INVALID) {
#ifndef BRCM_V3D_OPT
         goto final;
#endif
      }
   }

   if (khrn_map_insert(&state->surfaces, state->next_surface, shandle))
      result = state->next_surface++;

   if (result && buffers == 1 && win != EGL_PLATFORM_WIN_NONE) {
      /*
       * Single-buffered window surface. We need to display it now because
       * we won't necessarily get any eglSwapBuffers messages.
       */

      egl_disp_next(surface->disp_handle, surface->mh_color[0], win, 0);
   }
final:
   mem_unlock(shandle);
   mem_release(shandle);
   if (hcommon_storage != MEM_INVALID_HANDLE)
      mem_release(hcommon_storage);

   return result;
}

static int euclidean_gcd(int a, int b)
{
   if (b > a) {
      int t = a;
      a = b;
      b = t;
   }

   while (b != 0) {
      int m = a % b;
      a = b;
      b = m;
   }

   return a;
}

/*
   int eglIntCreateSurface_impl(
   uint32_t win,
   uint32_t buffers,
   uint32_t width,
   uint32_t height,
   KHRN_IMAGE_FORMAT_T colorformat,
   KHRN_IMAGE_FORMAT_T depthstencilformat,
   KHRN_IMAGE_FORMAT_T maskformat,
   uint32_t largest_pbuffer,
   uint32_t mipmap,
   uint32_t sem,
   uint32_t *results)

   Implementation notes:

   If largest_pbuffer is true, we try creating surfaces of various different
   sizes (with the correct aspect ratio) until the allocation succeeds.
   Otherwise we just attempt to create one of the correct size (create_surface_internal
   does the actual work).

   Preconditions:
      win is a valid server-side window handle or PLATFORM_WIN_NONE
      1 <= buffers <= EGL_MAX_BUFFERS
      0 < width, 0 < height
      If !largest_pbuffer then width <= EGL_CONFIG_MAX_WIDTH, height <= EGL_CONFIG_MAX_HEIGHT
      colorformat is a hardware framebuffer-supported uncompressed color format
      depthstencilformat is a hardware framebuffer-supported depth and/or stencil format, or IMAGE_FORMAT_INVALID
      maskformat is a hardware framebuffer-supported mask format, or IMAGE_FORMAT_INVALID
      multisampleformat is a hardware framebuffer-supported multisample color format or IMAGE_FORMAT_INVALID
      largest_pbuffer in {0,1}
      mipmap in {0,1}
      results is a valid pointer to three elements

   Postconditions:
      Return value is 3 (i.e. the number of items returned)
      On success:
         results[0] is the width of the surface
         results[1] is the height of the surface
         results[2] is the server-side surface handle and is nonzero
      On failure:
         results[2] == 0
*/

int eglIntCreateSurface_impl(
   uint32_t win,
   uint32_t buffers,
   uint32_t width,
   uint32_t height,
   KHRN_IMAGE_FORMAT_T colorformat,
   KHRN_IMAGE_FORMAT_T depthstencilformat,
   KHRN_IMAGE_FORMAT_T maskformat,
   KHRN_IMAGE_FORMAT_T multisampleformat,
   uint32_t largest_pbuffer,
   uint32_t mipmap,
   uint32_t sem,
   uint32_t *results)
{
   uint32_t max;
   uint32_t i;
   EGL_SURFACE_ID_T surface;

   if (largest_pbuffer && width && height) {
      uint32_t gcd = euclidean_gcd(width, height);

      width /= gcd;
      height /= gcd;

      max = _min(_min(EGL_CONFIG_MAX_WIDTH / width, EGL_CONFIG_MAX_HEIGHT / height), gcd);
   } else
      max = 1;

   surface = 0;

   for (i = max; i > 0 && !surface; i--) {
      surface = create_surface_internal(win, buffers, width*i, height*i, colorformat, depthstencilformat, maskformat, multisampleformat, mipmap, MEM_INVALID_HANDLE, MEM_INVALID_HANDLE, sem);

      results[0] = width*i;
      results[1] = height*i;
   }

   results[2] = surface;

   return 3;
}

#ifndef NO_OPENVG

/*
   return true if vgimage_format is compatible with config_format, ie the pixel
   format (ignoring premultiplied/linear flags) is the same
*/

static bool compatible_formats(KHRN_IMAGE_FORMAT_T config_format, KHRN_IMAGE_FORMAT_T vgimage_format)
{
   return (config_format & ~(IMAGE_FORMAT_MEM_LAYOUT_MASK | IMAGE_FORMAT_PRE | IMAGE_FORMAT_LIN)) ==
      (vgimage_format & ~(IMAGE_FORMAT_MEM_LAYOUT_MASK | IMAGE_FORMAT_PRE | IMAGE_FORMAT_LIN));
}

/*
   results
   0  EGL_SURFACE_ID_T  Handle to new surface (0 on failure)
   1  EGLInt            Error code
   2  EGLInt            Width
   3  EGLInt            Height
   4  KHRN_IMAGE_FORMAT_T    Format (may differ from the requested one in PRE or LIN)

   We always return 5 so that the RPC knows to transmit 5 return values.
*/

int eglIntCreatePbufferFromVGImage_impl(
   VGImage vg_handle,
   KHRN_IMAGE_FORMAT_T colorformat,
   KHRN_IMAGE_FORMAT_T depthstencilformat,
   KHRN_IMAGE_FORMAT_T maskformat,
   KHRN_IMAGE_FORMAT_T multisampleformat,
   uint32_t mipmap,
   uint32_t *results)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   results[0] = 0;

   if (state->vgcontext != MEM_INVALID_HANDLE) {
      VG_SERVER_STATE_T *server = VG_LOCK_SERVER_STATE();
      MEM_HANDLE_T imagehandle = vg_get_image(server, vg_handle, (EGLint *)(results + 1) /* error code */);

      if (imagehandle != MEM_INVALID_HANDLE) {
         if (vg_is_image(imagehandle)) {
            /* at this point, if the pixel format of the image is ok for
             * rendering to, the whole image is ok for rendering to and has the
             * IMAGE_FLAG_RENDER_TARGET flag set. so if the compatible_formats
             * call below succeeds, we know the image is ok to use as the color
             * buffer of a surface (which is the whole point of this function) */

            VG_IMAGE_T *vgimage = (VG_IMAGE_T *)mem_lock(imagehandle);
            if (vg_image_leak(vgimage)) {
               KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)mem_lock(vgimage->image);
               uint32_t width = vgimage->image_width;
               uint32_t height = vgimage->image_height;
               uint32_t buffers = mipmap ? _msb(_max(width, height)) + 1 : 1;
               KHRN_IMAGE_FORMAT_T actualformat = vgimage->image_format;

               if (!compatible_formats(colorformat, actualformat)) {
                  results[1] = EGL_BAD_MATCH;
               } else if (image->flags & (IMAGE_FLAG_BOUND_CLIENTBUFFER|IMAGE_FLAG_BOUND_EGLIMAGE)) {
                  /*
                   * TODO: we are forbidden from binding to a pbuffer when it's
                   * already bound to a different pbuffer.
                   * But what about binding to a pbuffer when it's already an EGLImage
                   * sibling?
                   */
                  results[1] = EGL_BAD_ACCESS;
               } else {
                  results[0] = create_surface_internal(
                     EGL_PLATFORM_WIN_NONE,
                     buffers,
                     width,
                     height,
                     colorformat,
                     depthstencilformat,
                     maskformat,
                     multisampleformat,
                     mipmap,
                     imagehandle,
                     MEM_INVALID_HANDLE,
                     0);

                  if (results[0]) {
                     image->flags |= IMAGE_FLAG_BOUND_CLIENTBUFFER;
                     results[2] = width;
                     results[3] = height;
                     results[4] = actualformat;
                  }
                  else
                     results[1] = EGL_BAD_ALLOC;
               }

               mem_unlock(vgimage->image);
            } else
               results[1] = EGL_BAD_ALLOC;
            mem_unlock(imagehandle);
         } else {
            /* child images cannot be render targets */
            results[1] = EGL_BAD_MATCH;
         }
      }

      VG_UNLOCK_SERVER_STATE();
   } else {
      results[1] = EGL_BAD_PARAMETER;       //TODO: what error to return if we don't have an OpenVG context at all?
   }

   return 5;
}

#endif /* NO_OPENVG */

/*
   EGL_SURFACE_ID_T eglIntCreateWrappedSurface_impl(
      uint32_t handle_0, uint32_t handle_1,
      KHRN_IMAGE_FORMAT_T depthstencilformat,
      KHRN_IMAGE_FORMAT_T maskformat,
      uint32_t multisample)

   Implementation notes:

   TODO: multisample is being ignored

   Preconditions:
      [handle_0, handle_1] is a valid server pixmap handle
      depthstencilformat is a hardware framebuffer-supported depth and/or stencil format, or IMAGE_FORMAT_INVALID
      maskformat is a hardware framebuffer-supported mask format, or IMAGE_FORMAT_INVALID
*/

EGL_SURFACE_ID_T eglIntCreateWrappedSurface_impl(
   uint32_t handle_0, uint32_t handle_1,
   KHRN_IMAGE_FORMAT_T depthstencilformat,
   KHRN_IMAGE_FORMAT_T maskformat,
   KHRN_IMAGE_FORMAT_T multisample)
{
   MEM_HANDLE_T himage;
#if EGL_BRCM_global_image
   if (handle_1 != (uint32_t)-1) {
      himage = egl_brcm_global_image_lookup(handle_0, handle_1);
   } else
#else
   vcos_assert(handle_1 == -1);
#endif
   {
      himage = egl_server_platform_create_pixmap_info(handle_0);
   }

   if (himage != MEM_INVALID_HANDLE) {
      KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)mem_lock(himage);
      EGL_SURFACE_ID_T result = create_surface_internal(
         EGL_PLATFORM_WIN_NONE,
         1,
         image->width,
         image->height,
         image->format,
         depthstencilformat,
         maskformat,
         multisample,
         0,
         MEM_INVALID_HANDLE,
         himage,
         0);
      mem_unlock(himage);
#if EGL_BRCM_global_image
      if (handle_1 == (uint32_t)-1)
#endif
      {
      mem_release(himage);
      }
      return result;
   } else {
      return 0;
   }
}

static MEM_HANDLE_T create_shared_context(void)
{
   MEM_HANDLE_T handle = MEM_ALLOC_STRUCT_EX(GLXX_SHARED_T, MEM_COMPACT_DISCARD);                                // check, glxx_shared_term

   if (handle != MEM_INVALID_HANDLE) {
      mem_set_term(handle, glxx_shared_term);

      if (glxx_shared_init((GLXX_SHARED_T *)mem_lock(handle)))
         mem_unlock(handle);
      else {
         mem_unlock(handle);
         mem_release(handle);
         handle = MEM_INVALID_HANDLE;
      }
   }

   return handle;
}

static MEM_HANDLE_T get_shared_context(EGL_SERVER_STATE_T *state, EGL_GL_CONTEXT_ID_T share_id, EGL_CONTEXT_TYPE_T share_type)
{
   MEM_HANDLE_T result = MEM_INVALID_HANDLE;

   switch (share_type) {
   case OPENGL_ES_11:
   {
      GLXX_SERVER_STATE_T *server;
      MEM_HANDLE_T handle = khrn_map_lookup(&state->glcontexts, share_id);
      vcos_assert(handle != MEM_INVALID_HANDLE);
      server = (GLXX_SERVER_STATE_T *)mem_lock(handle);
      result = server->mh_shared;
      mem_unlock(handle);
      break;
   }
   case OPENGL_ES_20:
   {
      GLXX_SERVER_STATE_T *server;
      MEM_HANDLE_T handle = khrn_map_lookup(&state->glcontexts, share_id);
      vcos_assert(handle != MEM_INVALID_HANDLE);
      server = (GLXX_SERVER_STATE_T *)mem_lock(handle);
      result = server->mh_shared;
      mem_unlock(handle);
      break;
   }
   case OPENVG:
   {
      VG_SERVER_STATE_T *server;
      MEM_HANDLE_T handle = khrn_map_lookup(&state->vgcontexts, share_id);
      vcos_assert(handle != MEM_INVALID_HANDLE);
      server = (VG_SERVER_STATE_T *)mem_lock(handle);
      result = server->shared_state;
      mem_unlock(handle);
      break;
   }
   default:
      UNREACHABLE();
   }
   return result;
}

// Create server states. To actually use these, call eglIntMakeCurrent.
EGL_GL_CONTEXT_ID_T eglIntCreateGLES11_impl(EGL_GL_CONTEXT_ID_T share_id, EGL_CONTEXT_TYPE_T share_type)
{
   GLXX_SERVER_STATE_T *server;
   EGL_GL_CONTEXT_ID_T result = 0;

   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   /*
      create and initialize shared state structure
      or obtain one from the specified context
   */

   MEM_HANDLE_T shandle, handle;
   if (share_id) {
      vcos_assert(share_type == OPENGL_ES_11 || share_type == OPENGL_ES_20);
      shandle = get_shared_context(state, share_id, share_type);
      mem_acquire(shandle);
   } else
      shandle = create_shared_context();

   if (shandle == MEM_INVALID_HANDLE)
      return 0;

   /*
      create and initialize state structure
   */

   handle = MEM_ALLOC_STRUCT_EX(GLXX_SERVER_STATE_T, MEM_COMPACT_DISCARD);                     // check, glxx_server_state_term
   if (handle == MEM_INVALID_HANDLE) {
      mem_release(shandle);
      return 0;
   }

   mem_set_term(handle, glxx_server_state_term);

   server = (GLXX_SERVER_STATE_T *)mem_lock(handle);

   if (!gl11_server_state_init(server, state->next_context, state->pid, shandle)) {
      mem_unlock(handle);
      mem_release(handle);
      mem_release(shandle);
      return 0;
   }

   if (khrn_map_insert(&state->glcontexts, state->next_context, handle))
      result = state->next_context++;

   mem_unlock(handle);
   mem_release(handle);
   mem_release(shandle);

   return result;
}

EGL_GL_CONTEXT_ID_T eglIntCreateGLES20_impl(EGL_GL_CONTEXT_ID_T share, EGL_CONTEXT_TYPE_T share_type)
{
   EGL_GL_CONTEXT_ID_T result = 0;

   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   /*
      create and initialize shared state structure
      or obtain one from the specified context
   */

   MEM_HANDLE_T shandle, handle;
   if (share) {
      vcos_assert(share_type == OPENGL_ES_11 || share_type == OPENGL_ES_20);
      shandle = get_shared_context(state, share, share_type);
      mem_acquire(shandle);
   } else
      shandle = create_shared_context();

   if (shandle == MEM_INVALID_HANDLE)
      return 0;

   /*
      create and initialize state structure
   */

   handle = MEM_ALLOC_STRUCT_EX(GLXX_SERVER_STATE_T, MEM_COMPACT_DISCARD);                     // check, glxx_server_state_term
   if (handle == MEM_INVALID_HANDLE) {
      mem_release(shandle);
      return 0;
   }

   mem_set_term(handle, glxx_server_state_term);

   if (!gl20_server_state_init((GLXX_SERVER_STATE_T *)mem_lock(handle), state->next_context, state->pid, shandle)) {
      mem_unlock(handle);
      mem_release(handle);
      mem_release(shandle);
      return 0;
   }

   if (khrn_map_insert(&state->glcontexts, state->next_context, handle))
      result = state->next_context++;

   mem_unlock(handle);
   mem_release(handle);
   mem_release(shandle);

   return result;
}

#ifndef NO_OPENVG

EGL_VG_CONTEXT_ID_T eglIntCreateVG_impl(EGL_VG_CONTEXT_ID_T share, EGL_CONTEXT_TYPE_T share_type)
{
   EGL_GL_CONTEXT_ID_T result = 0;

   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   /*
      create and initialize shared state structure
      or obtain one from the specified context
   */

   MEM_HANDLE_T shared_state_handle, handle;
   if (share) {
      vcos_assert(share_type == OPENVG);
      shared_state_handle = get_shared_context(state, share, share_type);
      mem_acquire(shared_state_handle);
   } else
      shared_state_handle = vg_server_shared_state_alloc();

   if (shared_state_handle == MEM_INVALID_HANDLE)
      return 0;

   handle = vg_server_state_alloc(shared_state_handle, state->pid);
   mem_release(shared_state_handle);

   if (handle == MEM_INVALID_HANDLE)
      return 0;

   if (khrn_map_insert(&state->vgcontexts, state->next_context, handle))
      result = state->next_context++;

   mem_release(handle);

   return result;
}

#endif /* NO_OPENVG */

// Disassociates surface or context objects from their handles. The objects
// themselves still exist as long as there is a reference to them. In
// particular, if you delete part of a triple buffer group, the remaining color
// buffers plus the ancillary buffers all survive.


int eglIntDestroySurface_impl(EGL_SURFACE_ID_T s)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   vcos_log("KHRN eglIntDestroySurface_impl %d", s);

   khrn_map_delete(&state->surfaces, s);

   return 0;
}

void eglIntDestroyGL_impl(EGL_GL_CONTEXT_ID_T c)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   vcos_log("KHRN eglIntDestroyGL_impl %d", c);
   khrn_map_delete(&state->glcontexts, c);
}

void eglIntDestroyVG_impl(EGL_VG_CONTEXT_ID_T c)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   khrn_map_delete(&state->vgcontexts, c);
}

void destroy_surface_callback(KHRN_MAP_T *map, uint32_t key, MEM_HANDLE_T value, void *pid)
{
   bool todestroy;
   UNUSED(map);

   todestroy = ((EGL_SERVER_SURFACE_T *)mem_lock(value))->pid == *(uint64_t *)pid;
   mem_unlock(value);

   vcos_log("KHRN destroy_surface_callback %d%s", key, todestroy ? " D" : "");
   if (todestroy) {
      EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
      khrn_map_delete(&state->surfaces, key);
   }
}

void destroy_glcontext_callback(KHRN_MAP_T *map, uint32_t key, MEM_HANDLE_T value, void *pid)
{
   MEM_TERM_T term = mem_get_term(value);

   bool todestroy = false;

   UNUSED(map);

   if (term == glxx_server_state_term) {
      todestroy = ((GLXX_SERVER_STATE_T *)mem_lock(value))->pid == *(uint64_t *)pid;
      mem_unlock(value);
   } else
      UNREACHABLE();

   vcos_log("KHRN destroy_glcontext_callback %d%s", key, todestroy ? " D" : "");
   if (todestroy) {
      EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
      khrn_map_delete(&state->glcontexts, key);
   }
}

void destroy_vgcontext_callback(KHRN_MAP_T *map, uint32_t key, MEM_HANDLE_T value, void *pid)
{
   bool todestroy;
   UNUSED(map);

   todestroy = ((VG_SERVER_STATE_T *)mem_lock(value))->pid == *(uint64_t *)pid;
   mem_unlock(value);

   if (todestroy) {
      EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
      khrn_map_delete(&state->vgcontexts, key);
   }
}

#if EGL_KHR_image
void destroy_eglimage_callback(KHRN_MAP_T *map, uint32_t key, MEM_HANDLE_T value, void *pid)
{
   bool todestroy;
   UNUSED(map);

   todestroy = ((EGL_IMAGE_T *)mem_lock(value))->pid == *(uint64_t *)pid;
   mem_unlock(value);

   if (todestroy) {
      EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
      khrn_map_delete(&state->eglimages, key);
   }
}
#endif

void eglIntDestroyByPid_impl(uint32_t pid_0, uint32_t pid_1)
{
   if(egl_server_state_initted)
   {
      EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
	  LOGE("pid mem_get_free_space before =  %x",mem_get_free_space());

      uint64_t pid = pid_0 | ((uint64_t)pid_1 << 32);
      vcos_log("KHRN eglIntDestroyByPid_impl %d %d", pid_0, pid_1);

      eglIntCheckCurrent_impl(pid_0, pid_1);

      khrn_map_iterate(&state->surfaces, destroy_surface_callback, &pid);
      khrn_map_iterate(&state->glcontexts, destroy_glcontext_callback, &pid);
      khrn_map_iterate(&state->vgcontexts, destroy_vgcontext_callback, &pid);
#if EGL_KHR_image
      khrn_map_iterate(&state->eglimages, destroy_eglimage_callback, &pid);
#endif
	  LOGE("pid mem_get_free_space =  %x",mem_get_free_space());
      /* todo: syncs */
   }
}

static void detach_buffers_from_gl(uint32_t glversion, MEM_HANDLE_T handle)
{
   if (handle != MEM_INVALID_HANDLE) {
      switch (glversion) {
      case EGL_SERVER_GL11:
      {
         GLXX_SERVER_STATE_T *state = (GLXX_SERVER_STATE_T *)mem_lock(handle);

         MEM_ASSIGN(state->mh_draw, MEM_INVALID_HANDLE);
         MEM_ASSIGN(state->mh_read, MEM_INVALID_HANDLE);
         MEM_ASSIGN(state->mh_depth, MEM_INVALID_HANDLE);
         MEM_ASSIGN(state->mh_multi, MEM_INVALID_HANDLE);

         mem_unlock(handle);
         break;
      }
      case EGL_SERVER_GL20:
      {
         GLXX_SERVER_STATE_T *state = (GLXX_SERVER_STATE_T *)mem_lock(handle);

         MEM_ASSIGN(state->mh_draw, MEM_INVALID_HANDLE);
         MEM_ASSIGN(state->mh_read, MEM_INVALID_HANDLE);
         MEM_ASSIGN(state->mh_depth, MEM_INVALID_HANDLE);
         MEM_ASSIGN(state->mh_multi, MEM_INVALID_HANDLE);

         mem_unlock(handle);
         break;
      }
      default:
         UNREACHABLE();
      }
   }
}

static void attach_buffers_to_gl(uint32_t glversion, MEM_HANDLE_T handle, MEM_HANDLE_T draw, MEM_HANDLE_T read, MEM_HANDLE_T depth, MEM_HANDLE_T multi)
{
   if (handle != MEM_INVALID_HANDLE) {
      switch (glversion) {
      case EGL_SERVER_GL11:
      case EGL_SERVER_GL20:
      {
         GLXX_SERVER_STATE_T *state = (GLXX_SERVER_STATE_T *)mem_lock(handle);

         glxx_server_state_set_buffers(state, draw, read, depth, multi);

         mem_unlock(handle);
         break;
      }
      default:
         UNREACHABLE();
      }
   }
}

static void update_gl_buffers(EGL_SERVER_STATE_T *state)
{
   if (state->glcontext != MEM_INVALID_HANDLE) {
      EGL_SERVER_SURFACE_T *dsurface = (EGL_SERVER_SURFACE_T *)mem_lock(state->gldrawsurface);
      EGL_SERVER_SURFACE_T *rsurface = (EGL_SERVER_SURFACE_T *)mem_lock(state->glreadsurface);

      attach_buffers_to_gl(
         state->glversion,
         state->glcontext,
         dsurface->mh_color[dsurface->back_buffer_index],
         rsurface->mh_color[rsurface->back_buffer_index],
         dsurface->mh_depth,
         dsurface->mh_multi);

      mem_unlock(state->gldrawsurface);
      mem_unlock(state->glreadsurface);
   }
}

#ifndef NO_OPENVG

static void update_vg_buffers(EGL_SERVER_STATE_T *state)
{
   if (state->vgsurface == MEM_INVALID_HANDLE) {
      vg_buffers_changed(MEM_INVALID_HANDLE, MEM_INVALID_HANDLE);
   } else {
      EGL_SERVER_SURFACE_T *surface = (EGL_SERVER_SURFACE_T *)mem_lock(state->vgsurface);
      vg_buffers_changed(surface->mh_color[surface->back_buffer_index], surface->mh_mask);
      mem_unlock(state->vgsurface);
   }
}

#endif /* NO_OPENVG */

// Selects the given process id for all operations. Most resource creation is
//  associated with the currently selected process id
// Selects the given context, draw and read surfaces for GL operations.
// Selects the given context and surface for VG operations.
// Any of the surfaces may be identical to each other.
// Contexts will not be flushed. (eglMakeCurrent must flush but this is handled
// by the client. It is not necessary to flush when switching threads
// If any of the surfaces have been resized then the color and ancillary buffers
//  are freed and recreated in the new size.
void eglIntMakeCurrent_impl(uint32_t pid_0, uint32_t pid_1, uint32_t glversion, EGL_GL_CONTEXT_ID_T gc, EGL_SURFACE_ID_T gdraw, EGL_SURFACE_ID_T gread, EGL_VG_CONTEXT_ID_T vc, EGL_SURFACE_ID_T vsurf)
{
#ifndef NO_OPENVG
   MEM_HANDLE_T chandle, shandle;
   bool surface_changed, context_changed;
#endif /* NO_OPENVG */
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   /* uint32_t thread_id = rtos_get_thread_id(); */
#ifdef NO_OPENVG
   UNUSED(vc);
   UNUSED(vsurf);
#endif

#ifdef WANT_ARM_LINUX
   khrn_misc_set_connection_pid(pid_0, pid_1);
#endif
   state->pid = ((uint64_t)pid_1 << 32) | pid_0;

   vcos_log("KHRN eglIntMakeCurrent_impl %d %d %d %d %p", pid_0, pid_1, gc, gdraw, vcos_thread_current());

   //GL
   if (gc != EGL_SERVER_NO_GL_CONTEXT) {
      MEM_HANDLE_T chandle = khrn_map_lookup(&state->glcontexts, gc);
      MEM_HANDLE_T dhandle = khrn_map_lookup(&state->surfaces, gdraw);
      MEM_HANDLE_T rhandle = khrn_map_lookup(&state->surfaces, gread);

      vcos_assert(chandle != MEM_INVALID_HANDLE && dhandle != MEM_INVALID_HANDLE && rhandle != MEM_INVALID_HANDLE);

      if (chandle != state->glcontext || dhandle != state->gldrawsurface || rhandle != state->glreadsurface)
      {
         detach_buffers_from_gl(state->glversion, state->glcontext);

         vcos_assert(glversion == EGL_SERVER_GL11 || glversion == EGL_SERVER_GL20);

         state->glversion = glversion;
         MEM_ASSIGN(state->glcontext, chandle);
         MEM_ASSIGN(state->gldrawsurface, dhandle);
         MEM_ASSIGN(state->glreadsurface, rhandle);

         update_gl_buffers(state);
      }

   } else {
      vcos_assert(gdraw == EGL_SERVER_NO_SURFACE);
      vcos_assert(gread == EGL_SERVER_NO_SURFACE);
      if (state->glcontext != MEM_INVALID_HANDLE) {
         detach_buffers_from_gl(state->glversion, state->glcontext);
         state->glversion = 0;
         MEM_ASSIGN(state->glcontext, MEM_INVALID_HANDLE);
         MEM_ASSIGN(state->gldrawsurface, MEM_INVALID_HANDLE);
         MEM_ASSIGN(state->glreadsurface, MEM_INVALID_HANDLE);
      }
   }

#ifndef NO_OPENVG
   /*VG */
   chandle = (vc == EGL_SERVER_NO_VG_CONTEXT) ? MEM_INVALID_HANDLE : khrn_map_lookup(&state->vgcontexts, vc);
   shandle = (vsurf == EGL_SERVER_NO_SURFACE) ? MEM_INVALID_HANDLE : khrn_map_lookup(&state->surfaces, vsurf);

   vcos_assert((chandle == MEM_INVALID_HANDLE) == (shandle == MEM_INVALID_HANDLE));

   surface_changed = state->vgsurface != shandle;
   context_changed = state->vgcontext != chandle;
   MEM_ASSIGN(state->vgcontext, chandle);
   MEM_ASSIGN(state->vgsurface, shandle);

   if (surface_changed) {
      update_vg_buffers(state);
   } else if (context_changed) {
      vg_state_changed();
   }
#endif
}

//Check if anything for this pid is current and detach it to allow destroybypid to work
void eglIntCheckCurrent_impl(uint32_t pid_0, uint32_t pid_1)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   uint64_t pid = ((uint64_t)pid_1 << 32) | pid_0;

   bool pidmatch=0;
   if(state->glcontext!=MEM_INVALID_HANDLE) {
      GLXX_SERVER_STATE_T *glstate = (GLXX_SERVER_STATE_T *)mem_lock(state->glcontext);
      if(glstate->pid==pid) pidmatch = true;
      mem_unlock(state->glcontext);
   }
   if(state->gldrawsurface!=EGL_SERVER_NO_SURFACE) {
      EGL_SERVER_SURFACE_T *dsurface = (EGL_SERVER_SURFACE_T *)mem_lock(state->gldrawsurface);
      if(dsurface->pid==pid) pidmatch = true;
      mem_unlock(state->gldrawsurface);
   }
   if(state->glreadsurface!=EGL_SERVER_NO_SURFACE) {
      EGL_SERVER_SURFACE_T *dsurface = (EGL_SERVER_SURFACE_T *)mem_lock(state->glreadsurface);
      if(dsurface->pid==pid) pidmatch = true;
      mem_unlock(state->glreadsurface);
   }
   if(state->vgcontext!=MEM_INVALID_HANDLE) {
      VG_SERVER_STATE_T *vgstate = (VG_SERVER_STATE_T *)mem_lock(state->vgcontext);
      if(vgstate->pid==pid) pidmatch = true;
      mem_unlock(state->vgcontext);
   }
   if(state->vgsurface!=EGL_SERVER_NO_SURFACE) {
      EGL_SERVER_SURFACE_T *dsurface = (EGL_SERVER_SURFACE_T *)mem_lock(state->vgsurface);
      if(dsurface->pid==pid) pidmatch = true;
      mem_unlock(state->vgsurface);
   }

//send NULL pid in this case
   if(pidmatch) eglIntMakeCurrent_impl(0, 0, 0, EGL_SERVER_NO_GL_CONTEXT, EGL_SERVER_NO_SURFACE, EGL_SERVER_NO_SURFACE, EGL_SERVER_NO_VG_CONTEXT, EGL_SERVER_NO_SURFACE);

}

static void flush_gl(uint32_t glversion, MEM_HANDLE_T handle, bool wait)
{
   if (handle != MEM_INVALID_HANDLE) {
      switch (glversion) {
      case EGL_SERVER_GL11:
      case EGL_SERVER_GL20:
      {
         GLXX_SERVER_STATE_T *state = (GLXX_SERVER_STATE_T *)mem_lock(handle);

         glxx_server_state_flush(state, wait);

         mem_unlock(handle);
         break;
      }
      default:
         UNREACHABLE();
      }
   }
}

// Flushes one or both context, and waits for the flushes to complete before returning.
// Equivalent to:
// if (flushgl) glFinish();
// if (flushvg) vgFinish();
// (todo: actually, we now also wait for the display stuff to finish. should
// gl/vgFinish also do this?)
int eglIntFlushAndWait_impl(uint32_t flushgl, uint32_t flushvg)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
#ifdef NO_OPENVG
   UNUSED(flushvg);
#endif

   if (flushgl)
      flush_gl(state->glversion, state->glcontext, true);
#ifndef NO_OPENVG
   if (flushvg && (state->vgcontext != MEM_INVALID_HANDLE)) {
      vgFinish_impl();
   }
#endif /* NO_OPENVG */

   /* todo: just wait for the current surface? */
   egl_disp_finish_all();

   return 0;
}

void eglIntFlush_impl(uint32_t flushgl, uint32_t flushvg)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
#ifdef NO_OPENVG
   UNUSED(flushvg);
#endif

   if (flushgl)
      flush_gl(state->glversion, state->glcontext, false);
#ifndef NO_OPENVG
   if (flushvg && (state->vgcontext != MEM_INVALID_HANDLE)) {
      vgFlush_impl();
   }
#endif /* NO_OPENVG */
}

void eglIntSwapBuffers_impl(EGL_SURFACE_ID_T s, uint32_t width, uint32_t height, uint32_t handle, uint32_t preserve, uint32_t position)
{
#ifdef DIRECT_RENDERING
   /* Wrapper layer shouldn't call eglSwapBuffers */
   UNREACHABLE();
#else
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   MEM_HANDLE_T shandle = khrn_map_lookup(&state->surfaces, s);
   EGL_SERVER_SURFACE_T *surface = (EGL_SERVER_SURFACE_T *)mem_lock(shandle);
   MEM_HANDLE_T ihandle;
   KHRN_IMAGE_T *image;

   khrn_stats_record_event(KHRN_STATS_SWAP_BUFFERS);

   egl_server_platform_set_position(handle, position, width, height);

   vcos_assert(surface->win != EGL_PLATFORM_WIN_NONE);   //TODO: is having this EGL_PLATFORM_WIN_NONE thing messy?
   vcos_assert(surface->buffers >= 1);

   if (surface->buffers > 1) {                           /* Auxilliary buffers are never preserved */
      if (state->glcontext != MEM_INVALID_HANDLE) {
         GLXX_SERVER_STATE_T *glstate = (GLXX_SERVER_STATE_T *)mem_lock(state->glcontext);
         glxx_hw_invalidate_frame(glstate, false, true, true, true);
         mem_unlock(state->glcontext);
      }
   }

   egl_disp_next(surface->disp_handle, surface->mh_color[surface->back_buffer_index],
      surface->win, (surface->buffers == 1) ? 0 : surface->swap_interval);

   surface->win = handle;
   surface->back_buffer_index = (surface->back_buffer_index + 1) % surface->buffers;
   /* (non-single-buffered surfaces) don't wait for the next buffer to come off
    * the display -- the interlock system will handle this */

   /*
      at this point we may need to resize the new back buffer and any attendant
      auxiliary buffers
   */

   ihandle = surface->mh_color[surface->back_buffer_index];
   image = (KHRN_IMAGE_T *)mem_lock(ihandle);

   if (image->width != width || image->height != height) {
      /* todo: for single buffered surfaces do we want to do the resize before
       * calling egl_disp_next? as it stands, egl_disp_next will use the old
       * size...
       * todo: khrn_image_resize can fail
       * todo: khrn_image_resize may try to mem_resize the buffer. this isn't
       * safe in the swap interval 0 case -- the buffer may be on the display
       * (and thus locked). also, khrn_interlock_write_immediate isn't strictly
       * enough? it only waits for reads and writes to complete, not for the
       * buffer to get unlocked? */

      void *data;

      khrn_interlock_write_immediate(&image->interlock);
      khrn_image_resize(image, width, height);

      if (image->mh_storage != MEM_INVALID_HANDLE)
      {
         //Fill resized buffer with 0s
         /* todo: why? */
         data=mem_lock(image->mh_storage);
#ifdef __VIDEOCORE__
         dma_memset(data,0,mem_get_size(image->mh_storage));
#else
         khrn_memset(data,0,mem_get_size(image->mh_storage));
#endif
         mem_unlock(image->mh_storage);
      }

      if (surface->mh_depth) {
         int scale = surface->mh_multi ? 2 : 1;

         KHRN_IMAGE_T *depth = (KHRN_IMAGE_T *)mem_lock(surface->mh_depth);
         khrn_interlock_write_immediate(&depth->interlock);
         khrn_image_resize(depth, width*scale, height*scale);
         mem_unlock(surface->mh_depth);
      }

      if (surface->mh_multi) {
         KHRN_IMAGE_T *multi = (KHRN_IMAGE_T *)mem_lock(surface->mh_multi);
         khrn_interlock_write_immediate(&multi->interlock);
         khrn_image_resize(multi, width*2, height*2);
         mem_unlock(surface->mh_multi);
      }

      if ((surface->buffers > 1) && preserve) {
         MEM_HANDLE_T prev_handle =
            surface->mh_color[mod(surface->back_buffer_index - 1, surface->buffers)];
         KHRN_IMAGE_T *prev = (KHRN_IMAGE_T *)mem_lock(prev_handle);

         /* todo: we could avoid this by pushing the copy through the worker
          * fifo */
         khrn_interlock_read_immediate(&prev->interlock);

         khrn_image_copy_region(image, 0, 0,
            _min(width, prev->width), _min(height, prev->height),
            prev, 0, 0,
            IMAGE_CONV_GL);

         mem_unlock(prev_handle);
      }
   } else if ((surface->buffers > 1) && preserve) {
      khrn_delayed_copy_buffer(
         surface->mh_color[surface->back_buffer_index],
         surface->mh_color[mod(surface->back_buffer_index - 1, surface->buffers)]);
   }

   /* The contents of the buffers becomes undefined so mark the interlocks to prevent loads */
   if (surface->buffers > 1) {
      if (!preserve) khrn_interlock_invalidate(&image->interlock);
      if (surface->mh_multi) {
         KHRN_IMAGE_T *multi = (KHRN_IMAGE_T *)mem_lock(surface->mh_multi);
         khrn_interlock_invalidate(&multi->interlock);
         mem_unlock(surface->mh_multi);
      }
      if (surface->mh_depth) {
         KHRN_IMAGE_T *depth = (KHRN_IMAGE_T *)mem_lock(surface->mh_depth);
         khrn_interlock_invalidate(&depth->interlock);
         mem_unlock(surface->mh_depth);
      }
   }

   mem_unlock(ihandle);
   mem_unlock(shandle);

   if (surface->buffers > 1) {
      // Buffers have swapped around, so need to make sure we're rendering into
      // the right ones.
      update_gl_buffers(state);
#ifndef NO_OPENVG
      if (state->vgsurface == shandle) { update_vg_buffers(state); }
#endif /* NO_OPENVG */
   }
#endif // DIRECT_RENDERING
}

void eglIntSelectMipmap_impl(EGL_SURFACE_ID_T s, int level)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   MEM_HANDLE_T shandle = khrn_map_lookup(&state->surfaces, s);
   EGL_SERVER_SURFACE_T *surface = (EGL_SERVER_SURFACE_T *)mem_lock(shandle);

   vcos_assert(surface->mipmap);

   // If the value of this attribute is outside the range of supported
   // mipmap levels, the closest valid mipmap level is selected for rendering.
   if (level < 0)
      level = 0;
   else if ((uint32_t)level >= surface->buffers)
      level = surface->buffers - 1;

   surface->back_buffer_index = level;

   mem_unlock(shandle);

   // Buffers have swapped around, so need to make sure we're rendering into
   // the right ones.
   update_gl_buffers(state);
#ifndef NO_OPENVG
   if (state->vgsurface == shandle) { update_vg_buffers(state); }
#endif /* NO_OPENVG */
}

void eglIntGetColorData_impl(EGL_SURFACE_ID_T s, KHRN_IMAGE_FORMAT_T format, uint32_t width, uint32_t height, int32_t stride, uint32_t y_offset, void *data)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   MEM_HANDLE_T shandle = khrn_map_lookup(&state->surfaces, s);
   EGL_SERVER_SURFACE_T *surface = (EGL_SERVER_SURFACE_T *)mem_lock(shandle);
   KHRN_IMAGE_T *src;
   KHRN_IMAGE_WRAP_T dst_wrap, src_wrap;

   if (stride < 0) {
      data = (uint8_t *)data - (stride * (height - 1));
   }
   khrn_image_wrap(&dst_wrap, format, width, height, stride, data);

   //TODO: we currently expect the client to flush before calling this
   //I've added a khrn_interlock_read_immediate below. Can we remove the flush on the client?

   // TODO will this handle all necessary conversions correctly?
   // Will it handle images of different sizes?
   src = (KHRN_IMAGE_T *)mem_lock(surface->mh_color[surface->back_buffer_index]);
   khrn_interlock_read_immediate(&src->interlock);
   khrn_image_lock_wrap(src, &src_wrap);
   khrn_image_wrap_copy_region(
      &dst_wrap, 0, 0,
      width, height,
      &src_wrap, 0, y_offset,
      IMAGE_CONV_GL);
   khrn_image_unlock_wrap(src);
   mem_unlock(surface->mh_color[surface->back_buffer_index]);

   mem_unlock(shandle);
}

void eglIntSetColorData_impl(EGL_SURFACE_ID_T s, KHRN_IMAGE_FORMAT_T format, uint32_t width, uint32_t height, int32_t stride, uint32_t y_offset, const void *data)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   MEM_HANDLE_T shandle = khrn_map_lookup(&state->surfaces, s);
   EGL_SERVER_SURFACE_T *surface = (EGL_SERVER_SURFACE_T *)mem_lock(shandle);
   KHRN_IMAGE_T *dst;
   KHRN_IMAGE_WRAP_T dst_wrap, src_wrap;

   if (stride < 0) {
      data = (const uint8_t *)data - (stride * (height - 1));
   }
   khrn_image_wrap(&src_wrap, format, width, height, stride, (void *)data); /* casting away constness here, but we won't actually modify */

   // TODO will this handle all necessary conversions correctly?
   // Will it handle images of different sizes?
   dst = (KHRN_IMAGE_T *)mem_lock(surface->mh_color[surface->back_buffer_index]);
   khrn_interlock_write_immediate(&dst->interlock);
   khrn_image_lock_wrap(dst, &dst_wrap);
   khrn_image_wrap_copy_region(
      &dst_wrap, 0, y_offset,
      width, height,
      &src_wrap, 0, 0,
      IMAGE_CONV_GL);
   khrn_image_unlock_wrap(dst);
   mem_unlock(surface->mh_color[surface->back_buffer_index]);

   mem_unlock(shandle);
}

static MEM_HANDLE_T get_active_gl_texture_2d(EGL_SERVER_STATE_T *state)
{
   MEM_HANDLE_T result = MEM_INVALID_HANDLE;
   if (state->glcontext != MEM_INVALID_HANDLE) {
      switch (state->glversion) {
      case EGL_SERVER_GL11:
      case EGL_SERVER_GL20:
      {
         GLXX_SERVER_STATE_T *glstate = (GLXX_SERVER_STATE_T *)mem_lock(state->glcontext);
         result = glstate->bound_texture[glstate->active_texture - GL_TEXTURE0].mh_twod;
         mem_unlock(state->glcontext);
         break;
      }
      default:
         UNREACHABLE();
      }
   }

   return result;
}

bool eglIntBindTexImage_impl(EGL_SURFACE_ID_T s)
{
   bool result;
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   MEM_HANDLE_T shandle = khrn_map_lookup(&state->surfaces, s);
   EGL_SERVER_SURFACE_T *surface = (EGL_SERVER_SURFACE_T *)mem_lock(shandle);
   MEM_HANDLE_T thandle = get_active_gl_texture_2d(state);
   KHRN_IMAGE_T *image0 = (KHRN_IMAGE_T *)mem_lock(surface->mh_color[0]);

   /* We permit IMAGE_FLAG_BOUND_CLIENTBUFFER */
   vcos_assert(!(image0->flags & IMAGE_FLAG_BOUND_EGLIMAGE));
   if (!(image0->flags & IMAGE_FLAG_BOUND_TEXIMAGE)) {
      if (thandle != MEM_INVALID_HANDLE) {
         GLXX_TEXTURE_T *texture = (GLXX_TEXTURE_T *)mem_lock(thandle);
         vcos_assert(!texture->is_cube);
         glxx_texture_bind_images(texture, surface->buffers, surface->mh_color, TEXTURE_STATE_BOUND_TEXIMAGE, shandle, surface->back_buffer_index);
         mem_unlock(thandle);

         MEM_ASSIGN(surface->mh_bound_texture, thandle);
      }
      result = true;
   } else {
      result = false;
   }

   mem_unlock(surface->mh_color[0]);
   mem_unlock(shandle);

   return result;
}

void eglIntReleaseTexImage_impl(EGL_SURFACE_ID_T s)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   MEM_HANDLE_T shandle = khrn_map_lookup(&state->surfaces, s);
   EGL_SERVER_SURFACE_T *surface = (EGL_SERVER_SURFACE_T *)mem_lock(shandle);

   if (surface->mh_bound_texture != MEM_INVALID_HANDLE)
   {
      MEM_HANDLE_T thandle = surface->mh_bound_texture;
      GLXX_TEXTURE_T *texture = (GLXX_TEXTURE_T *)mem_lock(thandle);
      glxx_texture_release_teximage(texture);   // this will remove IMAGE_FLAG_BOUND_TEXIMAGE from all images
      mem_unlock(thandle);

      MEM_ASSIGN(surface->mh_bound_texture, MEM_INVALID_HANDLE);
   }

   mem_unlock(shandle);
}

void eglIntSwapInterval_impl(EGL_SURFACE_ID_T s, uint32_t swap_interval)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   MEM_HANDLE_T shandle = khrn_map_lookup(&state->surfaces, s);
   EGL_SERVER_SURFACE_T *surface = (EGL_SERVER_SURFACE_T *)mem_lock(shandle);

   surface->swap_interval = swap_interval;
   mem_unlock(shandle);
}

typedef int (*USAGE_WALK_T)(void *v, int);

typedef struct {
   MEM_TERM_T term;
   USAGE_WALK_T walk;
} USAGE_RESOLVE_T;

static int get_mem_usage(MEM_HANDLE_T handle);

static int image_walk(KHRN_IMAGE_T *image, int size)
{
   int usage = 0;
   UNUSED(size);

   usage += get_mem_usage(image->mh_storage);
   usage += get_mem_usage(image->mh_aux);

   return usage;
}

static int texture_walk(GLXX_TEXTURE_T *texture, int size)
{
   int usage = 0;
   int i, j;
   UNUSED(size);

   for (i = 0; i < 7; i++)
      for (j = 0; j <= LOG2_MAX_TEXTURE_SIZE; j++)
         usage += get_mem_usage(texture->mh_mipmaps[i][j]);

   for(i = 0; i < GLXX_TEXTURE_POOL_SIZE; i++)
      usage += get_mem_usage(texture->blob_pool[i].mh_storage);
   usage += get_mem_usage(texture->mh_depaletted_blob);

   return usage;
}

static int buffer_walk(GLXX_BUFFER_T *buffer, int size)
{
   int usage = 0;
   int i;
   UNUSED(size);

   for(i = 0; i < GLXX_BUFFER_POOL_SIZE; i++)
      usage += get_mem_usage(buffer->pool[i].mh_storage);

   return usage;
}

static void map_iterator(KHRN_MAP_T *map, uint32_t key, MEM_HANDLE_T value, void *v)
{
   UNUSED(map);
   UNUSED(key);

   *(int *)v += get_mem_usage(value);
}

static int shared_walk(GLXX_SHARED_T *shared, int size)
{
   int usage = 0;
   UNUSED(size);

   khrn_map_iterate(&shared->pobjects, map_iterator, &usage);
   khrn_map_iterate(&shared->textures, map_iterator, &usage);
   khrn_map_iterate(&shared->buffers, map_iterator, &usage);
   khrn_map_iterate(&shared->renderbuffers, map_iterator, &usage);
   khrn_map_iterate(&shared->framebuffers, map_iterator, &usage);

   return usage;
}

static int egl_server_surface_walk(EGL_SERVER_SURFACE_T *surface, int size)
{
   int usage = 0;
   int i;
   UNUSED(size);

   for (i = 0; i < EGL_MAX_BUFFERS; i++)
      usage += get_mem_usage(surface->mh_color[i]);

   usage += get_mem_usage(surface->mh_depth);
   usage += get_mem_usage(surface->mh_multi);
   usage += get_mem_usage(surface->mh_mask);

   usage += get_mem_usage(surface->mh_bound_texture);

   return usage;
}

static int gl11_server_state_walk(GLXX_SERVER_STATE_T *state, int size)
{
   int usage = 0;
   int i;
   UNUSED(size);

   usage += get_mem_usage(state->bound_buffer.mh_array);
   usage += get_mem_usage(state->bound_buffer.mh_element_array);

   for (i = 0; i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++)
      usage += get_mem_usage(state->bound_buffer.mh_attrib_array[i]);

   for (i = 0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++)
      usage += get_mem_usage(state->bound_texture[i].mh_twod);

   usage += get_mem_usage(state->mh_shared);
   usage += get_mem_usage(state->mh_default_texture_twod);

   usage += get_mem_usage(state->mh_draw);
   usage += get_mem_usage(state->mh_read);
   usage += get_mem_usage(state->mh_depth);
   usage += get_mem_usage(state->mh_multi);

   usage += get_mem_usage(state->mh_cache);

   usage += get_mem_usage(state->mh_temp_palette);

   return usage;
}

static int gl20_server_state_walk(GLXX_SERVER_STATE_T *state, int size)
{
   int usage = 0;
   int i;
   UNUSED(size);

   usage += get_mem_usage(state->mh_program);

   usage += get_mem_usage(state->bound_buffer.mh_array);
   usage += get_mem_usage(state->bound_buffer.mh_element_array);

   for (i = 0; i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++)
      usage += get_mem_usage(state->bound_buffer.mh_attrib_array[i]);

   for (i = 0; i < GL20_CONFIG_MAX_TEXTURE_UNITS; i++) {
      usage += get_mem_usage(state->bound_texture[i].mh_twod);
      usage += get_mem_usage(state->bound_texture[i].mh_cube);
   }

   usage += get_mem_usage(state->mh_bound_renderbuffer);
   usage += get_mem_usage(state->mh_bound_framebuffer);

   usage += get_mem_usage(state->mh_shared);
   usage += get_mem_usage(state->mh_default_texture_twod);
   usage += get_mem_usage(state->mh_default_texture_cube);

   usage += get_mem_usage(state->mh_draw);
   usage += get_mem_usage(state->mh_read);
   usage += get_mem_usage(state->mh_depth);
   usage += get_mem_usage(state->mh_multi);

   usage += get_mem_usage(state->mh_cache);

   usage += get_mem_usage(state->mh_temp_palette);

   return usage;
}

static int gl20_bindings_walk(GL20_BINDING_T *base, int size)
{
   int usage = 0;
   int i, count = size / sizeof(GL20_BINDING_T);

   vcos_assert(size % sizeof(GL20_BINDING_T) == 0);

   for (i = 0; i < count; i++)
      usage += get_mem_usage(base[i].mh_name);

   return usage;
}

static int gl20_uniform_info_walk(GL20_UNIFORM_INFO_T *base, int size)
{
   int usage = 0;
   int i, count = size / sizeof(GL20_UNIFORM_INFO_T);

   vcos_assert(size % sizeof(GL20_UNIFORM_INFO_T) == 0);

   for (i = 0; i < count; i++)
      usage += get_mem_usage(base[i].mh_name);

   return usage;
}

static int gl20_attrib_info_walk(GL20_ATTRIB_INFO_T *base, int size)
{
   int usage = 0;
   int i, count = size / sizeof(GL20_ATTRIB_INFO_T);

   vcos_assert(size % sizeof(GL20_ATTRIB_INFO_T) == 0);

   for (i = 0; i < count; i++)
      usage += get_mem_usage(base[i].mh_name);

   return usage;
}

static int gl20_shader_sources_walk(MEM_HANDLE_T *base, int size)
{
   int usage = 0;
   int i, count = size / sizeof(MEM_HANDLE_T);

   vcos_assert(size % sizeof(MEM_HANDLE_T) == 0);

   for (i = 0; i < count; i++)
      usage += get_mem_usage(base[i]);

   return usage;
}

static int gl20_program_walk(GL20_PROGRAM_T *program, int size)
{
   int usage = 0;
#ifdef __VIDEOCORE4__
   int i;
#endif
   UNUSED(size);

   usage += get_mem_usage(program->mh_vertex);
   usage += get_mem_usage(program->mh_fragment);

   usage += get_mem_usage(program->mh_bindings);

#ifdef __VIDEOCORE4__
   usage += get_mem_usage(program->result.mh_blob);
   for (i = 0; i < GL20_LINK_RESULT_CACHE_SIZE; i++)
   {
      usage += get_mem_usage(program->result.cache[i].data.mh_vcode);
      usage += get_mem_usage(program->result.cache[i].data.mh_ccode);
      usage += get_mem_usage(program->result.cache[i].data.mh_fcode);
      usage += get_mem_usage(program->result.cache[i].data.mh_vuniform_map);
      usage += get_mem_usage(program->result.cache[i].data.mh_cuniform_map);
      usage += get_mem_usage(program->result.cache[i].data.mh_funiform_map);
   }
#else
   usage += get_mem_usage(program->mh_vertex_code);
   usage += get_mem_usage(program->mh_fragment_code);
#endif

   usage += get_mem_usage(program->mh_sampler_info);

   usage += get_mem_usage(program->mh_uniform_info);
   usage += get_mem_usage(program->mh_uniform_data);

   usage += get_mem_usage(program->mh_attrib_info);

   usage += get_mem_usage(program->mh_info);

   return usage;
}

static int gl20_shader_walk(GL20_SHADER_T *shader, int size)
{
   int usage = 0;
   UNUSED(size);

   usage += get_mem_usage(shader->mh_sources_current);
   usage += get_mem_usage(shader->mh_sources_compile);

   usage += get_mem_usage(shader->mh_info);

   return usage;
}

static int glxx_renderbuffer_walk(GLXX_RENDERBUFFER_T *renderbuffer, int size)
{
   int usage = 0;
   UNUSED(size);

   usage += get_mem_usage(renderbuffer->mh_storage);

   return usage;
}

static int glxx_framebuffer_walk(GLXX_FRAMEBUFFER_T *framebuffer, int size)
{
   int usage = 0;
   UNUSED(size);

   usage += get_mem_usage(framebuffer->attachments.color.mh_object);
   usage += get_mem_usage(framebuffer->attachments.depth.mh_object);
   usage += get_mem_usage(framebuffer->attachments.stencil.mh_object);

   return usage;
}

#ifndef NO_OPENVG

static int vg_font_walk(VG_FONT_T *font, int size)
{
   int usage = 0;
   int i;

   VG_FONT_LOCKED_T font_locked;

   vg_font_lock(font, &font_locked);
   for (i = 0; i != (font_locked.capacity * 2); ++i)
      if (font_locked.entries[i] != SLOT_NONE)
         usage += get_mem_usage(font_locked.slots[font_locked.entries[i]].object);
   vg_font_unlock(font);

   usage += get_mem_usage(font->entries);
   usage += get_mem_usage(font->slots);

   return usage;
}

static int vg_image_walk(VG_IMAGE_T *image, int size)
{
   int usage = 0;

   usage += get_mem_usage(image->image);

   return usage;
}

static int vg_child_image_walk(VG_CHILD_IMAGE_T *child_image, int size)
{
   int usage = 0;

   usage += get_mem_usage(child_image->parent);

   return usage;
}

static int vg_mask_layer_walk(VG_MASK_LAYER_T *mask_layer, int size)
{
   int usage = 0;

   usage += get_mem_usage(mask_layer->image);

   return usage;
}

static int vg_paint_walk(VG_PAINT_T *paint, int size)
{
   int usage = 0;

   usage += get_mem_usage(paint->pattern);

   if (paint->ramp != MEM_INVALID_HANDLE) {
      VG_RAMP_T *ramp = (VG_RAMP_T *)mem_lock(paint->ramp);
      usage += get_mem_usage(ramp->data);
      mem_unlock(paint->ramp);
   }

   usage += get_mem_usage(paint->ramp_stops);

   return usage;
}

static int vg_path_walk(VG_PATH_T *path, int size)
{
   int usage = 0;

   /*
      TODO: these elements are in the platform-specific extras structure
   */
#ifdef __VIDEOCORE4__
   //TODO
#else
   usage += get_mem_usage(path->extra.segment_lengths);
   usage += get_mem_usage(path->extra.tess);
#endif

   usage += get_mem_usage(path->coords);
   usage += get_mem_usage(path->segments);

   return usage;
}

static void set_iterator(VG_SET_T *map, MEM_HANDLE_T handle, void *p)
{
   *(int *)p += get_mem_usage(handle);
}

static int vg_server_shared_state_walk(VG_SERVER_SHARED_STATE_T *shared_state, int size)
{
   int usage = 0;

   vg_set_iterate(&shared_state->objects, set_iterator, &usage);

   return usage;
}

static int vg_server_state_walk(VG_SERVER_STATE_T *state, int size)
{
   int usage = 0;

   usage += get_mem_usage(state->shared_state);

   usage += get_mem_usage(state->stroke_paint);
   usage += get_mem_usage(state->fill_paint);

   usage += get_mem_usage(state->scissor.scissor);

   return usage;
}

#endif /* NO_OPENVG */

USAGE_RESOLVE_T resolves[] = {
   {khrn_image_term, (USAGE_WALK_T)image_walk},

   {glxx_texture_term, (USAGE_WALK_T)texture_walk},
   {glxx_buffer_term, (USAGE_WALK_T)buffer_walk},
   {glxx_shared_term, (USAGE_WALK_T)shared_walk},

   {egl_server_surface_term, (USAGE_WALK_T)egl_server_surface_walk},

   {glxx_server_state_term, (USAGE_WALK_T)gl11_server_state_walk},

   {glxx_server_state_term, (USAGE_WALK_T)gl20_server_state_walk},
   {gl20_bindings_term, (USAGE_WALK_T)gl20_bindings_walk},
   {gl20_uniform_info_term, (USAGE_WALK_T)gl20_uniform_info_walk},
   {gl20_attrib_info_term, (USAGE_WALK_T)gl20_attrib_info_walk},
   {gl20_shader_sources_term, (USAGE_WALK_T)gl20_shader_sources_walk},
   {gl20_program_term, (USAGE_WALK_T)gl20_program_walk},
   {gl20_shader_term, (USAGE_WALK_T)gl20_shader_walk},
   {glxx_renderbuffer_term, (USAGE_WALK_T)glxx_renderbuffer_walk},
   {glxx_framebuffer_term, (USAGE_WALK_T)glxx_framebuffer_walk},

#ifndef NO_OPENVG
   {vg_font_bprint_term, NULL},
   {vg_font_term, (USAGE_WALK_T)vg_font_walk},
   {vg_image_bprint_term, NULL},
   {vg_image_term, (USAGE_WALK_T)vg_image_walk},
   {vg_child_image_bprint_term, (USAGE_WALK_T)vg_child_image_walk},
   {vg_child_image_term, (USAGE_WALK_T)vg_child_image_walk},
   {vg_mask_layer_bprint_term, NULL},
   {vg_mask_layer_term, (USAGE_WALK_T)vg_mask_layer_walk},
   {vg_paint_bprint_term, NULL},
   {vg_paint_term, (USAGE_WALK_T)vg_paint_walk},
   {vg_path_bprint_term, NULL},
   {vg_path_term, (USAGE_WALK_T)vg_path_walk},
   {vg_server_shared_state_term, (USAGE_WALK_T)vg_server_shared_state_walk},
   {vg_server_state_term, (USAGE_WALK_T)vg_server_state_walk},
#endif /* NO_OPENVG */

   {NULL, NULL}
};

static int flag;

static int get_mem_usage(MEM_HANDLE_T handle)
{
   int usage = 0;

   if (handle && (mem_get_flags(handle) & MEM_FLAG_USER) != (MEM_FLAG_T)flag) {
      MEM_TERM_T term = mem_get_term(handle);
      int size = mem_get_size(handle);

      usage = size;

      mem_set_user_flag(handle, flag);

      if (term)
      {
         USAGE_RESOLVE_T *resolve;
         for (resolve = resolves; resolve->term; resolve++) {
            if (resolve->term == term) {
               if(resolve->walk) {
                  usage += resolve->walk(mem_lock(handle), size);
                  mem_unlock(handle);
               }
               break;
            }
         }
      }
   }

   return usage;
}

#if EGL_BRCM_global_image
static void global_image_callback(MEM_HANDLE_T handle, void *dummy)
{
   UNUSED(dummy);
   get_mem_usage(handle);
}
#endif

typedef struct {
   uint64_t pid;
   uint32_t private_usage;
   uint32_t shared_usage;
} USAGE_RECORD_T;

typedef struct {
   USAGE_RECORD_T *data;
   uint32_t count;
} USAGE_USERDATA_T;

static USAGE_RECORD_T *find_record(USAGE_USERDATA_T *user, uint64_t pid)
{
   uint32_t i;
   for (i = 0; i < user->count; i++) {
      if (!user->data[i].pid || user->data[i].pid == pid) {
         user->data[i].pid = pid;

         return &user->data[i];
      }
   }

   return NULL;
}

static void glcontext_callback(KHRN_MAP_T *map, uint32_t key, MEM_HANDLE_T value, void *user)
{
   MEM_TERM_T term = mem_get_term(value);

   int usage = get_mem_usage(value);

   UNUSED(map);
   UNUSED(key);

   if (flag == MEM_FLAG_USER) {
      USAGE_RECORD_T *record = NULL;

      if (term == glxx_server_state_term) {
         record = find_record((USAGE_USERDATA_T *)user, ((GLXX_SERVER_STATE_T *)mem_lock(value))->pid);
         mem_unlock(value);
      } else
         UNREACHABLE();

      if (record)
         record->private_usage += usage;
   }
}

static void vgcontext_callback(KHRN_MAP_T *map, uint32_t key, MEM_HANDLE_T value, void *user)
{
   VG_SERVER_STATE_T *state = (VG_SERVER_STATE_T *)mem_lock(value);

   int usage = get_mem_usage(value);

   UNUSED(map);
   UNUSED(key);

   if (flag == MEM_FLAG_USER) {
      USAGE_RECORD_T *record = find_record((USAGE_USERDATA_T *)user, state->pid);

      if (record)
         record->private_usage += usage;
   }

   mem_unlock(value);
}

static void surface_callback(KHRN_MAP_T *map, uint32_t key, MEM_HANDLE_T value, void *user)
{
   EGL_SERVER_SURFACE_T *surface = (EGL_SERVER_SURFACE_T *)mem_lock(value);

   int usage = get_mem_usage(value);

   UNUSED(map);
   UNUSED(key);

   if (flag == MEM_FLAG_USER) {
      USAGE_RECORD_T *record = find_record((USAGE_USERDATA_T *)user, surface->pid);

      if (record)
         record->private_usage += usage;
   }

   mem_unlock(value);
}

void eglIntGetProcessMemUsage_impl(uint32_t size, uint32_t *result)
{
   USAGE_USERDATA_T user;
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   user.data = (USAGE_RECORD_T *)result;
   user.count = (size * sizeof(uint32_t)) / sizeof(USAGE_RECORD_T);
   memset(result, 0, size * sizeof(uint32_t));

   /*
      walk our data structures, setting the user flag as we go
   */

   flag = MEM_FLAG_USER;

#if EGL_BRCM_global_image
   /* mark all global images first so they won't be counted */
   egl_brcm_global_image_iterate(global_image_callback, NULL);
#endif

   khrn_map_iterate(&state->glcontexts, glcontext_callback, &user);
   khrn_map_iterate(&state->vgcontexts, vgcontext_callback, &user);
   khrn_map_iterate(&state->surfaces, surface_callback, &user);

   /*
      clear the flags back down
   */

   flag = MEM_FLAG_NONE;

#if EGL_BRCM_global_image
   egl_brcm_global_image_iterate(global_image_callback, NULL);
#endif

   khrn_map_iterate(&state->glcontexts, glcontext_callback, NULL);
   khrn_map_iterate(&state->vgcontexts, vgcontext_callback, NULL);
   khrn_map_iterate(&state->surfaces, surface_callback, NULL);
}

void eglIntGetGlobalMemUsage_impl(uint32_t *result)
{
   result[0] = 0x02000000;
   result[1] = 0x02000000 - mem_get_free_space();
}

EGL_SYNC_ID_T eglIntCreateSync_impl(uint32_t condition, int32_t threshold, uint32_t sem)
{
   EGL_SERVER_SYNC_T *sync;

   EGL_SYNC_ID_T result = 0;

   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   MEM_HANDLE_T shandle = MEM_ALLOC_STRUCT_EX(EGL_SERVER_SYNC_T, MEM_COMPACT_DISCARD);                   // check
   if (shandle == MEM_INVALID_HANDLE)
      return result;

   sync = (EGL_SERVER_SYNC_T *)mem_lock(shandle);

   sync->condition = condition;
   sync->threshold = threshold;
   sync->pid = state->pid;
   sync->sem = sem;
   sync->state = false;

   if (khrn_map_insert(&state->syncs, state->next_sync, shandle))
      result = state->next_sync++;

   mem_unlock(shandle);
   mem_release(shandle);

   return result;
}

void eglIntDestroySync_impl(EGL_SYNC_ID_T s)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   MEM_HANDLE_T handle = khrn_map_lookup(&state->syncs, s);
   EGL_SERVER_SYNC_T *sync = (EGL_SERVER_SYNC_T *)mem_lock(handle);
   khdispatch_send_async(ASYNC_COMMAND_DESTROY, sync->pid, sync->sem);  //tell host to delete semaphore
   mem_unlock(handle);

   khrn_map_delete(&state->syncs, s);
}

static void egl_khr_sync_callback(KHRN_MAP_T *map, uint32_t key, MEM_HANDLE_T value, void *used)
{
   EGL_SERVER_SYNC_T *sync = (EGL_SERVER_SYNC_T *)mem_lock(value);

   bool new_state = sync->state;

   UNUSED(map);
   UNUSED(key);
   UNUSED(used);

   if (new_state && !sync->state)
      khdispatch_send_async(ASYNC_COMMAND_POST, sync->pid, sync->sem);
   if (!new_state && sync->state)
      khdispatch_send_async(ASYNC_COMMAND_WAIT, sync->pid, sync->sem);

   sync->state = new_state;

   mem_unlock(value);
}

void egl_khr_sync_update()
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   if (state->syncs.entries) {
      uint32_t used = 0x02000000 - mem_get_free_space();

      khrn_map_iterate(&state->syncs, egl_khr_sync_callback, (void *)used);
   }
}

#ifdef DIRECT_RENDERING
void eglDirectRenderingPointer_impl(
   EGL_SURFACE_ID_T surf,
#ifdef BRCM_V3D_OPT
	uint32_t aux,
#endif
   uint32_t buffer,
   KHRN_IMAGE_FORMAT_T buffer_format,
   uint32_t buffer_width,
   uint32_t buffer_height,
   uint32_t buffer_stride)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   MEM_HANDLE_T shandle;
   MEM_HANDLE_T ihandle;
   MEM_HANDLE_T whandle;
   EGL_SERVER_SURFACE_T *surface;
   KHRN_IMAGE_T *image;

   shandle = khrn_map_lookup(&state->surfaces, surf);
   vcos_assert(shandle != MEM_INVALID_HANDLE);

   surface = (EGL_SERVER_SURFACE_T *)mem_lock(shandle);

//   surface->back_buffer_index = (surface->back_buffer_index + 1) % surface->buffers;

   ihandle = surface->mh_color[surface->back_buffer_index];
   image = (KHRN_IMAGE_T *)mem_lock(ihandle);

   LOGV("eglDirectRenderingPointer_impl - mem = 0x%08X", buffer );

#ifdef BRCM_V3D_OPT
	whandle = mem_wrap( aux, buffer, buffer_height * buffer_stride, 1, 0, "EGL_IMAGE_WRAP_BRCM");
#else
   whandle = mem_wrap((void *)khrn_hw_unaddr(buffer), buffer_height * buffer_stride, 1, 0, "EGL_IMAGE_WRAP_BRCM");
#endif

   /* Client should already have waited for rendering to stop on this buffer, so write_immediate shouldn't block */
   MEM_ASSIGN(image->mh_storage, whandle);
   mem_release(whandle);   

   image->format = buffer_format;
   image->width = buffer_width;
   image->height = buffer_height;
   image->stride = buffer_stride;
   khrn_interlock_write_immediate(&image->interlock);

   mem_unlock(ihandle);
   mem_unlock(shandle);

}
#endif
