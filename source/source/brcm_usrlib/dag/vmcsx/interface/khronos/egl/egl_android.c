/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include <ui/egl/android_natives.h>
#include <ui/android_native_buffer.h>

#ifdef EGL_SERVER_SMALLINT

static INLINE EGLNativeWindowType platform_get_canonical_win(int width, int height)
{
	if ((width == 800) && (height == 480))	return NATIVE_WINDOW_800_480;
	else if ((width == 640) && (height == 480)) return NATIVE_WINDOW_640_480;
	else if ((width == 320) && (height == 240)) return NATIVE_WINDOW_320_240;
	else if ((width == 240) && (height == 400)) return NATIVE_WINDOW_240_400;
	else if ((width == 64) && (height == 64)) return NATIVE_WINDOW_64_64;
	else if ((width == 400) && (height == 480)) return NATIVE_WINDOW_400_480_A;
	else if ((width == 400) && (height == 480)) return NATIVE_WINDOW_400_480_B;
	else if ((width == 512) && (height == 512)) return NATIVE_WINDOW_512_512;
	else if ((width == 360) && (height == 640)) return NATIVE_WINDOW_360_640;
	else if ((width == 640) && (height == 360)) return NATIVE_WINDOW_640_360;
	else if ((width == 1280) && (height == 720)) return NATIVE_WINDOW_1280_720;
	else if ((width == 1920) && (height == 1080)) return NATIVE_WINDOW_1920_1080;
	else if ((width == 480) && (height == 320)) return NATIVE_WINDOW_480_320;
	else if ((width == 1680) && (height == 1050)) return NATIVE_WINDOW_1680_1050;
	else return (NativeWindowType)0;
}

static NativeWindowType convertNativeWindowType(NativeWindowType window)
{
	int width, height;
	android_native_window_t *nativeWindow = (android_native_window_t*)(window);
	nativeWindow->query(nativeWindow, NATIVE_WINDOW_WIDTH, &width);
	nativeWindow->query(nativeWindow, NATIVE_WINDOW_HEIGHT, &height);
	return platform_get_canonical_win(width, height);
}

#else // EGL_SERVER_SMALLINT

uint32_t platform_get_handle(EGLNativeWindowType win)
{
	return (uint32_t)win;
}

void platform_get_dimensions(EGLNativeWindowType win, uint32_t *width, uint32_t *height)
{
#ifndef DIRECT_RENDERING
	android_native_window_t *nativeWindow = (android_native_window_t*)(win);
	nativeWindow->query(nativeWindow, NATIVE_WINDOW_WIDTH, width);
	nativeWindow->query(nativeWindow, NATIVE_WINDOW_HEIGHT, height);
#else
	*width = (uint32_t)win & 0xffff;
	*height = ((uint32_t)win >> 16) & 0xffff;
#endif
}

#endif
