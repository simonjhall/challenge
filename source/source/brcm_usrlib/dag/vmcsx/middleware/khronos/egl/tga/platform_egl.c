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
#include "helpers/vc_image/vc_image.h"
#include <assert.h>

//#include "vcfw/logging/logging.h"
#include "interface/vchi/os/os.h"

#include <assert.h>

#define tga_mem_size				(800*480*4+32)
static uint8_t						tga_memory[tga_mem_size];

static bool							displaying		= false;
static uint32_t						current_win		= 0;
static EGL_SERVER_RETURN_CALLBACK_T egl_callback	= NULL;

static void vc_image_to_tga_memory(
	VC_IMAGE_T*			image)
{
	uint16_t			width, height;
	uint16_t			x, y;
	KHRN_IMAGE_WRAP_T	src, dst;
	uint8_t*			src_ptr				= (image->mem_handle != MEM_INVALID_HANDLE) ? (uint8_t*)mem_lock(image->mem_handle) : (uint8_t*)image->image_data;
	bool				tformat				= (image->type == VC_IMAGE_TF_RGBA32 || image->type == VC_IMAGE_TF_RGBX32);
	uint8_t				twelve_bytes[12]	= {0,0,2,0,0,0,0,0,0,0,0,0};
	uint32_t			tga_mempos			= 0;
	
	assert(tga_mem_size >= 18 + 4*image->height*image->width);
	
	/* 0-length image id, no colour map, uncompressed true-color image, (0,0) origin. */
	memcpy(tga_memory, twelve_bytes, 12);
	tga_mempos += 12;
	
	/* Width and height, 16-bit little endian */
	*(tga_memory+tga_mempos++) = image->width & 0xFF;
	*(tga_memory+tga_mempos++) = (image->width & 0xFF00) >> 8;
	*(tga_memory+tga_mempos++) = image->height & 0xFF;
	*(tga_memory+tga_mempos++) = (image->height & 0xFF00) >> 8;
	*(tga_memory+tga_mempos++) = '\x20';
	*(tga_memory+tga_mempos++) = '\x20';
	
	// Swap red and blue. Do t-format conversion if necessary.
	khrn_image_wrap(&src, tformat ? ABGR_8888_TF : ABGR_8888_RSO, image->width, image->height, image->pitch, src_ptr);
	khrn_image_wrap(&dst, ARGB_8888_RSO, image->width, image->height, image->pitch, tga_memory + tga_mempos);
	khrn_image_wrap_convert(&dst, &src, IMAGE_CONV_GL);

	if (image->mem_handle != MEM_INVALID_HANDLE) mem_unlock(image->mem_handle);
}


void egl_server_platform_display(
	uint32_t	win, 
	KHRN_IMAGE_T*	khrn_image, 
	uint32_t	cb_arg)
{
    VC_IMAGE_T		vc_image,*image;
	
	assert(win != EGL_PLATFORM_WIN_NONE);
	if (!displaying) 
	{
		displaying = true;
		current_win = win;
	}
	assert(win == current_win);    //Can only display on a single window

	
    khrn_image_fill_vcimage(khrn_image, &vc_image);

	image = &vc_image;
	assert(image->type == VC_IMAGE_TF_RGBA32 || image->type == VC_IMAGE_RGBA32 || image->type == VC_IMAGE_TF_RGBX32);
	vc_image_to_tga_memory(image);
	
	/* May as well call callback immediately */
	egl_callback(cb_arg);
}

void egl_server_platform_display_nothing(
	uint32_t	win, 
	uint32_t	cb_arg)
{
	//TODO: destroy window
	assert(win != EGL_PLATFORM_WIN_NONE);
	assert(!displaying || (win == current_win));
	displaying = false;
}

MEM_HANDLE_T egl_server_platform_create_pixmap_info(
	uint32_t	pixmap)
{
	assert(0);
	return MEM_INVALID_HANDLE;
}

void egl_server_platform_init(
	EGL_SERVER_RETURN_CALLBACK_T return_callback)
{
	egl_callback = return_callback;
}

void egl_server_platform_shutdown(void)
{
}

void egl_server_platform_set_position(uint32_t handle, uint32_t position, uint32_t width, uint32_t height)
{
}

void egl_server_platform_display_nothing_sync(uint32_t win)
{
   //TODO: wait for thread to finish?
   vcos_assert(win != EGL_PLATFORM_WIN_NONE);
// vcos_assert(!displaying || (win == current_win));
   displaying = false;

//   SendMessage(the_hwnd, WM_CLOSE, 0, 0);
}


