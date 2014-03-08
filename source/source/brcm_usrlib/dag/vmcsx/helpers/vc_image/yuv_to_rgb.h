/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/**
 * \file
 *
 * This file is not part of the public interface.
 * Objects declared here should not be used externally.
 */

typedef void VC_IMAGE_RGB_BACKEND_T(
   unsigned char       *dest,
   int                  pitch,
   ...);

typedef void VC_IMAGE_YUV_FRONTEND_T(
   unsigned char const *yptr,
   unsigned char const *uptr,
   unsigned char const *vptr,
   int                  src_pitch,
   int                  src_pitch2,
   VC_IMAGE_RGB_BACKEND_T *backend,
   unsigned char       *dest,
   int                  pitch,
   ...);


typedef struct
{
   VC_IMAGE_YUV_FRONTEND_T
   *load32,
   *load16;
   VC_IMAGE_RGB_BACKEND_T
   *store32,
   *store16;
} VC_IMAGE_YUV_TO_RGB_MODE_T;

/******************************************************************************
Assembler functions defined in yuv_to_rgb_asm.s
******************************************************************************/

VC_IMAGE_YUV_FRONTEND_T yuv420_to_rgb_load_32x16_misaligned;
VC_IMAGE_YUV_FRONTEND_T yuv420_to_rgb_load_32x16;

VC_IMAGE_YUV_FRONTEND_T yuv420ci_to_rgb_load_32x16_misaligned;
VC_IMAGE_YUV_FRONTEND_T yuv420ci_to_rgb_load_32x16;

VC_IMAGE_YUV_FRONTEND_T yuv420_to_rgb_load_32x16o_misaligned;
VC_IMAGE_YUV_FRONTEND_T yuv420_to_rgb_load_32x16o;

VC_IMAGE_YUV_FRONTEND_T yuv420ci_to_rgb_load_32x16o_misaligned;
VC_IMAGE_YUV_FRONTEND_T yuv420ci_to_rgb_load_32x16o;

VC_IMAGE_YUV_FRONTEND_T yuv422_to_rgb_load_32x16_misaligned;
VC_IMAGE_YUV_FRONTEND_T yuv422_to_rgb_load_32x16;

VC_IMAGE_YUV_FRONTEND_T yuv_uv_to_rgb_load_32x16_misaligned;
VC_IMAGE_YUV_FRONTEND_T yuv_uv_to_rgb_load_32x16;

VC_IMAGE_YUV_FRONTEND_T yuv422ci_load_32x16_rgb_misaligned;
VC_IMAGE_YUV_FRONTEND_T yuv422ci_load_32x16_rgb;

VC_IMAGE_YUV_FRONTEND_T yuv422pi_load_32x16_rgb_misaligned;
VC_IMAGE_YUV_FRONTEND_T yuv422pi_load_32x16_rgb;

VC_IMAGE_RGB_BACKEND_T yuv_to_rgba32_store_32x16;
VC_IMAGE_RGB_BACKEND_T yuv_to_rgba32_store_16x16;

VC_IMAGE_RGB_BACKEND_T yuv_to_rgba32_store_32xn_misaligned;
VC_IMAGE_RGB_BACKEND_T yuv_to_rgba32_store_mxn_misaligned;

VC_IMAGE_RGB_BACKEND_T yuv_to_tf_rgba32_store_32x16;
VC_IMAGE_RGB_BACKEND_T yuv_to_tf_rgba32_store_16x16;
VC_IMAGE_RGB_BACKEND_T yuv_to_tf_rgba32_linear_store_16x16;
VC_IMAGE_RGB_BACKEND_T yuv_to_tf_rgb565_store_32x16;
VC_IMAGE_RGB_BACKEND_T yuv_to_tf_rgb565_store_32x16_dither;

VC_IMAGE_RGB_BACKEND_T yuv_to_rgba32_store_16x16;
VC_IMAGE_RGB_BACKEND_T yuv_to_rgb888_store_32x16;
VC_IMAGE_RGB_BACKEND_T yuv_to_rgb888_store_16x16;

VC_IMAGE_RGB_BACKEND_T yuv_to_rgb888_store_32xn_misaligned;
VC_IMAGE_RGB_BACKEND_T yuv_to_rgb888_store_mxn_misaligned;

VC_IMAGE_RGB_BACKEND_T yuv_to_rgb565_store_32x16_dither;
VC_IMAGE_RGB_BACKEND_T yuv_to_rgb565_store_16x16_dither;

VC_IMAGE_RGB_BACKEND_T yuv_to_rgb565_store_32x16;
VC_IMAGE_RGB_BACKEND_T yuv_to_rgb565_store_16x16;

VC_IMAGE_RGB_BACKEND_T yuv_to_rgb565_store_32xn_misaligned_dither;
VC_IMAGE_RGB_BACKEND_T yuv_to_rgb565_store_mxn_misaligned_dither;

VC_IMAGE_RGB_BACKEND_T yuv_to_rgb565_store_32xn_misaligned;
VC_IMAGE_RGB_BACKEND_T yuv_to_rgb565_store_mxn_misaligned;

VC_IMAGE_YUV_FRONTEND_T yuvuv420_to_rgb_load_32x16;

#if defined(FAST_YUVTORGB565)
#define yuv_to_rgb565_store_32x16_default yuv_to_rgb565_store_32x16
#define yuv_to_rgb565_store_16x16_default yuv_to_rgb565_store_16x16
#define yuv_to_rgb565_store_32xn_misaligned_default yuv_to_rgb565_store_32xn_misaligned
#define yuv_to_rgb565_store_mxn_misaligned_default yuv_to_rgb565_store_mxn_misaligned
#define yuv_to_tf_rgb565_store_32x16_default yuv_to_tf_rgb565_store_32x16
#else
#define yuv_to_rgb565_store_32x16_default yuv_to_rgb565_store_32x16_dither
#define yuv_to_rgb565_store_16x16_default yuv_to_rgb565_store_16x16_dither
#define yuv_to_rgb565_store_32xn_misaligned_default yuv_to_rgb565_store_32xn_misaligned_dither
#define yuv_to_rgb565_store_mxn_misaligned_default yuv_to_rgb565_store_mxn_misaligned_dither
#define yuv_to_tf_rgb565_store_32x16_default yuv_to_tf_rgb565_store_32x16_dither
#endif

/******************************************************************************
Exported functions.
******************************************************************************/

void vc_image_yuv_to_rgb_stripe(
   VC_IMAGE_YUV_TO_RGB_MODE_T
   const *mode,
   unsigned char       *dptr,
   unsigned char const *yptr,
   unsigned char const *uptr,
   unsigned char const *vptr,
   int                  dstep,
   int                  dpitch,
   int                  ypitch,
   int                  uvpitch,
   int                  width,
   int                  arg1);

void vc_image_yuv_to_rgb_stripe_precise(
   VC_IMAGE_YUV_TO_RGB_MODE_T
   const *mode,
   unsigned char       *dptr,
   unsigned char const *yptr,
   unsigned char const *uptr,
   unsigned char const *vptr,
   int                  dstep,
   int                  dpitch,
   int                  ypitch,
   int                  uvpitch,
   int                  width,
   int                  arg1);

void vc_image_yuv_to_rgb(
   VC_IMAGE_YUV_TO_RGB_MODE_T
   const *mode,
   unsigned char       *dptr,
   unsigned char const *yptr,
   unsigned char const *uptr,
   unsigned char const *vptr,
   int                  dstep,
   int                  dpitch,
   int                  ypitch,
   int                  uvpitch,
   int                  uvstripepitch,
   int                  width,
   int                  height,
   int                  arg1);


void vc_image_yuv_to_rgb_precise(
   VC_IMAGE_YUV_TO_RGB_MODE_T
   const *mode,
   unsigned char       *dptr,
   unsigned char const *yptr,
   unsigned char const *uptr,
   unsigned char const *vptr,
   int                  dstep,
   int                  dpitch,
   int                  ypitch,
   int                  uvpitch,
   int                  uvstripepitch,
   int                  width,
   int                  height,
   int                  arg1);


