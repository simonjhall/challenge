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

#ifndef VC_IMAGE_RGBA565_H
#define VC_IMAGE_RGBA565_H

#include "vc_image.h"

/* Conversions */

void vc_image_yuv422_to_rgba565(VC_IMAGE_T *dest, VC_IMAGE_T const *src);
void vc_image_yuv420_to_rgba565(VC_IMAGE_T *dest, VC_IMAGE_T const *src);
void vc_image_rgb565_to_rgba565(VC_IMAGE_T *dest, VC_IMAGE_T const *src);
void vc_image_rgba32_to_rgba565(VC_IMAGE_T *dest, VC_IMAGE_T const *src);
void vc_image_rgba565_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

void vc_image_rgba32_to_rgba565_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset);

void vc_image_rgba565_to_rgba32_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset);

void vc_image_rgba565_to_rgb888_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset);

void vc_image_rgba565_to_rgb565_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset);

/* Copies, translations, merges */

/* combine data and output rgba565. */
void vc_image_overlay_rgba32_to_rgba565(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha);

/* combine data and output rgba565. */
void vc_image_overlay_rgba565_to_rgba565(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha);

/* combine data and output rgba565. */
void vc_image_overlay_rgb565_to_rgba565(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  transparent_colour);

/* combine data and output rgb565. */
void vc_image_overlay_rgba565_to_rgb565(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha);

/* combine data and output rgb888. */
void vc_image_overlay_rgba565_to_rgb888(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha);

/* combine data and output rgba32. */
void vc_image_overlay_rgba565_to_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha);

/* combine data and output rgb565, using only the 16x16 blocks indicated in
 * block_mask */
void vc_image_quick_overlay_rgba565_to_rgb565(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   VC_IMAGE_T    const *block_mask,
   int                  block_x_offset,
   int                  block_y_offset,
   int                  alpha);

/* combine data and output rgb888, using only the 16x16 blocks indicated in
 * block_mask */
void vc_image_quick_overlay_rgba565_to_rgb888(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   VC_IMAGE_T    const *block_mask,
   int                  block_x_offset,
   int                  block_y_offset,
   int                  alpha);

void vc_image_resize_stripe_h_rgba565(VC_IMAGE_T *dest, int d_width, int s_width);
void vc_image_resize_stripe_v_rgba565(VC_IMAGE_T *dest, int d_height, int s_height);

#endif

