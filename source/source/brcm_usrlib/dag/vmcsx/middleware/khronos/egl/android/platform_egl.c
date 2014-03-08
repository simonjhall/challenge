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
#include <ui/android_native_buffer.h>
#include <ui/egl/android_natives.h>
#include <cutils/log.h>

static bool displaying = false;
static uint32_t current_win = 0;
static EGL_SERVER_RETURN_CALLBACK_T egl_callback = NULL;

#ifndef DIRECT_RENDERING
extern gralloc_module_t const* module;
static android_native_window_t* nativeWindow = NULL;
static int bpp = 0;
extern android_native_window_t* get_android_native_window(void);
extern int getBytesPerPixel(int format);

static KHRN_IMAGE_FORMAT_T convert_color_format(int format)
{
	switch (format) {
	case HAL_PIXEL_FORMAT_RGBA_8888: return ABGR_8888_RSO;
	case HAL_PIXEL_FORMAT_BGRA_8888: return ARGB_8888_RSO;
	case HAL_PIXEL_FORMAT_RGBX_8888: return XBGR_8888_RSO;
	case HAL_PIXEL_FORMAT_RGB_565: return RGB_565_RSO;
	default: return 0;
	}
}

static inline KHRN_IMAGE_FORMAT_T check_color_format_khrn_image_vs_native_window(KHRN_IMAGE_T *image, android_native_buffer_t *buffer)
{
	KHRN_IMAGE_FORMAT_T src_format, dst_format;
	dst_format = convert_color_format(buffer->format);
	if (khrn_image_is_color(image->format)) {
		src_format = (image->format & ~(IMAGE_FORMAT_PRE | IMAGE_FORMAT_LIN));
		return (src_format == dst_format) ? 0 : dst_format;
	} else {
		return 0;
	}
}

static void khrn_image_copy(KHRN_IMAGE_WRAP_T *dst, KHRN_IMAGE_WRAP_T *src)
{
	uint32_t dst_stride = dst->stride;
	uint32_t src_stride = src->stride;
	uint32_t op_w_size = (dst->format == RGB_565_RSO) ? dst->width*2 : dst->width*4;
	uint32_t op_h_size = dst->height;
	uint8_t* p_dst_addr = (uint8_t*)dst->storage;
	uint8_t* p_src_addr = (uint8_t*)src->storage;

	assert(dst->format == RGB_565_RSO || dst->format == ARGB_8888_RSO);
	assert(op_h_size);

	while (op_h_size--)
	{
		memcpy(p_dst_addr, p_src_addr, op_w_size);

		p_dst_addr += dst_stride;
		p_src_addr += src_stride;
	}
}

static void vc_image_to_RSO_memory(VC_IMAGE_T *image, android_native_buffer_t *buffer, void *bits)
{
	KHRN_IMAGE_FORMAT_T src_format, dst_format;
	KHRN_IMAGE_WRAP_T src, dst;
	uint8_t* src_ptr;
	int conv_width, conv_height;

	dst_format = convert_color_format(buffer->format);

	if (!dst_format)
		return;

	// Swap red and blue. Do t-format conversion if necessary.
	switch (image->type) {
		case VC_IMAGE_TF_RGBA32:	src_format = ABGR_8888_TF;	break;
		case VC_IMAGE_TF_RGBX32:	src_format = XBGR_8888_TF;	break;
		case VC_IMAGE_TF_RGB565:	src_format = RGB_565_TF;	break;
		case VC_IMAGE_TF_RGBA5551:	src_format = RGBA_5551_TF;	break;
		case VC_IMAGE_TF_RGBA16:	src_format = RGBA_4444_TF;	break;
		case VC_IMAGE_RGBA32:		src_format = ABGR_8888_RSO;	break;
		case VC_IMAGE_RGBX32:		src_format = XBGR_8888_RSO;	break;
		case VC_IMAGE_RGB565:		src_format = RGB_565_RSO;	break;
		default: UNREACHABLE();		src_format = 0;			break;
	}

	if (!src_format)
		return;

	src_ptr = (image->mem_handle != MEM_INVALID_HANDLE) ? (uint8_t*)mem_lock(image->mem_handle) : (uint8_t*)image->image_data;

	conv_width = (image->width <= buffer->width) ? image->width : buffer->width;
	conv_height = (image->height <= buffer->height) ? image->height : buffer->height;

	khrn_image_wrap(&src, src_format, conv_width, conv_height, image->pitch, src_ptr);
	khrn_image_wrap(&dst, dst_format, conv_width, conv_height, buffer->width * bpp, bits);

	if (src.format == dst.format)
		khrn_image_copy(&dst, &src);
	else
		khrn_image_wrap_convert(&dst, &src, IMAGE_CONV_GL);

	if (image->mem_handle != MEM_INVALID_HANDLE)
		mem_unlock(image->mem_handle);
}

static void khrn_image_to_native_buffer(KHRN_IMAGE_T *image, android_native_buffer_t *buffer, void *bits)
{
	MEM_HANDLE_T mem_handle;
	uint8_t *ptr;
	int size;

	mem_handle = image->mh_storage;
	ptr = (uint8_t*)mem_lock(mem_handle);
	size = mem_get_size(mem_handle);

	// memcpy(bits, ptr, size);
	{
	/* V3D Y axis calibrarion and that of Android is opposite, so copying from top of src to bottom of dest */
	/* This could be a place where we can look for optimization */
		int image_height, image_stride;
		int buffer_height, buffer_stride;
		int copy_height, copy_stride, i;

		image_height = (uint16_t)(khrn_image_is_packed_mask(image->format) ? khrn_image_get_packed_mask_height(image->height) : image->height);
		image_stride = image->stride;
		buffer_height = buffer->height;
		buffer_stride = buffer->width * bpp;
		copy_height = (image_height <= buffer_height) ? image_height : buffer_height;
		copy_stride = (image_stride <= buffer_stride) ? image_stride : buffer_stride;

#ifndef BCG_FB_LAYOUT
		ptr += (image->offset + size - image_stride);
#endif
		for (i=0; i< copy_height; i++) {
			memcpy(bits, ptr, copy_stride);
			bits = (void *)((char *)bits + buffer_stride);
#ifndef BCG_FB_LAYOUT
			ptr -= image_stride;
#else
			ptr += image_stride;
#endif
		}
	}
}
#endif // DIRECT_RENDERING

void egl_server_platform_display(
	uint32_t	win,
	KHRN_IMAGE_T*	image,
	uint32_t	cb_arg)
{
#ifdef DIRECT_RENDERING
	assert(0);
	return;
#else
	assert(win != EGL_PLATFORM_WIN_NONE);

	if (!displaying) {
		displaying = true;
		current_win = win;
	}
	assert(win == current_win);    //Can only display on a single window

	{
	android_native_buffer_t* buffer;
	void* bits;

	if (!nativeWindow)
		nativeWindow = get_android_native_window();

	if (nativeWindow) {
		nativeWindow->dequeueBuffer(nativeWindow, &buffer);
		// comment out the line below when software OpenGL is used for SurfaceFlinger 
		nativeWindow->lockBuffer(nativeWindow, buffer);

		module->lock(module, buffer->handle,
			GRALLOC_USAGE_SW_WRITE_OFTEN,
			0, 0, buffer->width, buffer->height, &bits);

		if (!bpp)
			bpp = getBytesPerPixel(buffer->format);

		if (check_color_format_khrn_image_vs_native_window(image, buffer)) {
			VC_IMAGE_T vc_image;
			khrn_image_fill_vcimage(image, &vc_image);
			vc_image_to_RSO_memory(&vc_image, buffer, bits);
		} else {
			khrn_image_to_native_buffer(image, buffer, bits);
		}

		module->unlock(module, buffer->handle);
		nativeWindow->queueBuffer(nativeWindow, buffer);
	}
	}

	/* May as well call callback immediately */
	if (egl_callback)
		egl_callback(cb_arg);
#endif
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

void egl_server_platform_display_nothing_sync(uint32_t win)
{
	//TODO: wait for thread to finish?
	vcos_assert(win != EGL_PLATFORM_WIN_NONE);
	vcos_assert(!displaying || (win == current_win));
	displaying = false;
}

void egl_server_platform_set_position(uint32_t handle, uint32_t position, uint32_t width, uint32_t height)
{
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
	egl_callback = NULL;
}
