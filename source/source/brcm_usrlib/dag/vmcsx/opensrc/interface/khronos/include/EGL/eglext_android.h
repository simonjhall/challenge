/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited.
All rights reserved.

Project  :  khronos
Module   :  Header file

FILE DESCRIPTION
structure and constant defines to support Android extensions to EGL
without needing to compile against the full Android tree.

These definitions are all culled from various places in 
The Android Open Source Project which is 
released under the Apache license, Version 2.0
============================================================================*/

/* 
**
** Copyright 2009, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/


#ifndef EGL_EXT_ANDROID_DEFS_H
#define EGL_EXT_ANDROID_DEFS_H


/* from http://android.git.kernel.org/?p=platform/frameworks/base.git;a=blob;f=opengl/include/EGL/eglext.h */
#ifndef EGL_ANDROID_image_native_buffer
#define EGL_ANDROID_image_native_buffer 1
#endif

#ifdef EGL_ANDROID_image_native_buffer

/* Andrdoid definition of buffer_handle_t is some sort of struct (in lib hardware), but for our purposes we'll pretend it is a pointer 
   but this might give us size issues in the structs...
*/
typedef unsigned int buffer_handle_t;

/* from http://android.git.kernel.org/?p=platform/frameworks/base.git;a=blob;f=include/ui/egl/android_natives.h */
typedef struct android_native_base_t
{
   /* a magic value defined by the actual EGL native type */
   int magic;

   /* the sizeof() of the actual EGL native type */
   int version;

   void* reserved[4];

   /* reference-counting interface */
   void (*incRef)(struct android_native_base_t* base);
   void (*decRef)(struct android_native_base_t* base);
} android_native_base_t;
/* */

/* from http://android.git.kernel.org/?p=platform/frameworks/base.git;a=blob;f=include/ui/android_native_buffer.h */
typedef struct android_native_buffer_t
{
    struct android_native_base_t common;

    int width;
    int height;
    int stride;
    int format;
    int usage;
    
    void* reserved[2];

    buffer_handle_t handle;

    void* reserved_proc[8];
} android_native_buffer_t;
/* */

#define EGL_NATIVE_BUFFER_ANDROID       0x3140  /* eglCreateImageKHR target */

/* from http://android.git.kernel.org/?p=platform/hardware/libhardware.git;a=blob;f=include/hardware/hardware.h */
/**
 * pixel format definitions
 */

enum {
    HAL_PIXEL_FORMAT_RGBA_8888          = 1,
    HAL_PIXEL_FORMAT_RGBX_8888          = 2,
    HAL_PIXEL_FORMAT_RGB_888            = 3,
    HAL_PIXEL_FORMAT_RGB_565            = 4,
    HAL_PIXEL_FORMAT_BGRA_8888          = 5,
    HAL_PIXEL_FORMAT_RGBA_5551          = 6,
    HAL_PIXEL_FORMAT_RGBA_4444          = 7,

    /* 0x8 - 0xFF range unavailable */

    /*
     * 0x100 - 0x1FF
     *
     * This range is reserved for pixel formats that are specific to the HAL
     * implementation.  Implementations can use any value in this range to
     * communicate video pixel formats between their HAL modules.  These formats
     * must not have an alpha channel.  Additionally, an EGLimage created from a
     * gralloc buffer of one of these formats must be supported for use with the
     * GL_OES_EGL_image_external OpenGL ES extension.
     */

    /*
     * Android YUV format:
     *
     * This format is exposed outside of the HAL to software
     * decoders and applications.
     * EGLImageKHR must support it in conjunction with the
     * OES_EGL_image_external extension.
     *
     * YV12 is 4:2:0 YCrCb planar format comprised of a WxH Y plane followed
     * by (W/2) x (H/2) Cr and Cb planes.
     *
     * This format assumes
     * - an even width
     * - an even height
     * - a horizontal stride multiple of 16 pixels
     * - a vertical stride equal to the height
     *
     *   y_size = stride * height
     *   c_size = ALIGN(stride/2, 16) * height/2
     *   size = y_size + c_size * 2
     *   cr_offset = y_size
     *   cb_offset = y_size + c_size
     *
     */
	HAL_PIXEL_FORMAT_YCbCr_422_SP       = 0x10, // NV16+    
	HAL_PIXEL_FORMAT_YCrCb_420_SP       = 0x11, // NV21+    
	HAL_PIXEL_FORMAT_YCbCr_422_I        = 0x14, // YUY2+    
	HAL_PIXEL_FORMAT_YCbCr_420_SP       = 0x15,
	HAL_PIXEL_FORMAT_YV12               = 0x16,
	HAL_PIXEL_FORMAT_YCbCr_420_P        = 0x17,

};
/* */



#endif
/* */

#endif