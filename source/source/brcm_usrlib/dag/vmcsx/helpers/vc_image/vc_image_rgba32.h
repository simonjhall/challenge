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

#ifndef VC_IMAGE_RGBA32_H
#define VC_IMAGE_RGBA32_H

#include "vc_image.h"

/* Conversions */

void vc_image_yuv422_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha);
void vc_image_yuv420_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha);
void vc_image_yuv422_to_rgba32_part(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha);
void vc_image_yuv420_to_rgba32_part(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha);
void vc_image_rgb565_to_rgba32_part(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha);
void vc_image_rgb888_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha);
void vc_image_bgr888_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha);
void vc_image_rgb565_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha, int transparent_colour);
void vc_image_palette8_to_rgba32(
   VC_IMAGE_T          *dest,
   VC_IMAGE_T    const *src,
   short         const *cmap,
   int                  alpha,
   int                  transparent_colour);
void vc_image_palette4_to_rgba32(
   VC_IMAGE_T          *dest,
   VC_IMAGE_T    const *src,
   short         const *cmap,
   int                  alpha,
   int                  transparent_colour);

/* Copies, translations, merges */

/* move data around without messing with it.  odd alignemnts will be allowed. */
void vc_image_blt_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset);

/* combine data and output rgb565.  horizontal alignment and width must be a
 * multiple of 16. */
void vc_image_overlay_rgba32_to_rgb565(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha);

/* combine data and output rgb888.  horizontal alignment and width must be a
 * multiple of 16. */
void vc_image_overlay_rgba32_to_rgb888(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha);

/* combine data and output rgba32.  src appears over the top of dest.  odd
 * alignments will be allowed. */
void vc_image_overlay_rgba32_to_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha);

/* combine data and output rgba32.  src appears over the top of dest.  odd
 * alignments will be allowed. */
void vc_image_overlay_rgb565_to_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  transparent_colour,
   int                  alpha);

/* combine data and output rgba32.  src and dest are mixed in proportion to
 * their alpha values */
void vc_image_mix_rgba32_to_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset);

/* combine data and output rgb565, using only the 16x16 blocks indicated in
 * block_mask */
void vc_image_quick_overlay_rgba32_to_rgb565(
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
void vc_image_quick_overlay_rgba32_to_rgb888(
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

/* Force values into the alpha channel.
 *
 * low_alpha is the alpha value for a 0 bit, high_alpha is the alpha value for
 * a 1 bit. */
void vc_image_rgba32_set_alpha_1bpp(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *mask,
   int                  mask_x_offset,
   int                  mask_y_offset,
   int                  low_alpha,
   int                  high_alpha);

/* Force values into the alpha channel using an 8bpp mask.
 *
 * alpha_scale gives the FSD value of the mask (either positive or negative),
 * and alpha_boost gives the offset added afterwards.  A simple inversion of
 * the mask would have a scale of -256, and a boost of 255. */
void vc_image_rgba32_set_alpha_8bpp(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *mask,
   int                  mask_x_offset,
   int                  mask_y_offset);

/* Adjust the alpha channel with a 1bpp mask.
 *
 * low_alpha is the alpha value for a 0 bit, high_alpha is the alpha value for
 * a 1 bit. */
void vc_image_rgba32_adjust_alpha_1bpp(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *mask,
   int                  mask_x_offset,
   int                  mask_y_offset,
   int                  low_alpha_scale,
   int                  high_alpha_scale);

/* Adjust the alpha channel with an 8bpp mask.
 *
 * alpha_scale gives the FSD value of the mask (either positive or negative),
 * and alpha_boost gives the offset added afterwards.  A simple inversion of
 * the mask would have a scale of -256, and a boost of 255. */
void vc_image_rgba32_adjust_alpha_8bpp(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *mask,
   int                  mask_x_offset,
   int                  mask_y_offset,
   int                  alpha_scale,
   int                  alpha_boost);

/* transpose (in-place illegal) */
void vc_image_transpose_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

/* transpose tiles (in-place OK) */
void vc_image_transpose_tiles_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, int hdeci);

/* vertical flip (in-place OK) */
void vc_image_vflip_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

/* horizontal flip (in-place OK) */
void vc_image_hflip_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

/* resize */
void vc_image_resize_stripe_h_rgba32(VC_IMAGE_T *dest, int d_width, int s_width);
void vc_image_resize_stripe_v_rgba32(VC_IMAGE_T *dest, int d_height, int s_height);

/* force region in dest to be the specified value */
void vc_image_fill_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   long                 value);

/* mix the specified value into a region in dest */
void vc_image_tint_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   long                 value);

/* draw a line (may have anti-aliasing and/or alpha-blended edges, so don't
 * assume it'll be easy to delete) */
void vc_image_line_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_start,
   int                  y_start,
   int                  x_end,
   int                  y_end,
   long                 fg_colour);

/* Text */

void vc_image_text_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   long                 fg_colour,
   long                 bg_colour,
   char          const *text,
   int                 *max_x,
   int                 *max_y);

void vc_image_small_text_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   long                 fg_colour,
   long                 bg_colour,
   char          const *text,
   int                 *max_x,
   int                 *max_y);

void vc_image_text_32_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   long                 fg_colour,
   long                 bg_colour,
   char          const *text,
   int                 *max_x,
   int                 *max_y);

void vc_image_text_24_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   long                 fg_colour,
   long                 bg_colour,
   char          const *text,
   int                 *max_x,
   int                 *max_y);

void vc_image_text_20_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   long                 fg_colour,
   long                 bg_colour,
   char          const *text,
   int                 *max_x,
   int                 *max_y);

void vc_image_font_alpha_blt_rgba32(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                    VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int red, int green, int blue, int alpha);

void vc_image_premultiply_rgba32(VC_IMAGE_T *dest);
#endif

