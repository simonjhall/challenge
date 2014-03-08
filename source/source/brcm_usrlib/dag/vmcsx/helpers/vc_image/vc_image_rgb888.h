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

#ifndef VC_IMAGE_RGB888_H
#define VC_IMAGE_RGB888_H

#include "vc_image.h"

/* Convert an RGB565 image to RGB888 in place (must be large enough - and pitch is not altered). */

void vc_image_rgb565_to_rgb888_in_place(VC_IMAGE_T *dest);

/* Convert an RGB888 image to RGB565 in place (must be large enough - and pitch is not altered). */

void vc_image_rgb888_to_rgb565_in_place(VC_IMAGE_T *dest);

/* Convert part of an RGB565 image to RGB888 */
void vc_image_rgb565_to_rgb888_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset);

/* Convert part of a V3D32 image to RGB888 */
void vc_image_3d32_to_rgb888_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset,
   short         const *cmap);

/* Convert part of a V3D32B image to RGB888 */
void vc_image_3d32b_to_rgb888_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset);

/* Convert an RGB888 image to RGB delta in place. */
void vc_image_rgb888_to_rgbdelta_in_place(VC_IMAGE_T *dest);

/* Convert part of an 8bpp image to RGB888 */

void vc_image_8bpp_to_rgb888_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset,
   short               *pal);

/* Fill a region of an RGB888 image with solid colour. */

void vc_image_fill_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int value);

/* Fade a region of an RGB888 image with given alpha value. */

void vc_image_fade_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int alpha);

/* Not a region of an RGB888 image. */

void vc_image_not_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height);

/* Blt a region of one RGB888 image to another. */

void vc_image_blt_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                         VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

/* Or a region of one RGB888 image onto another. */

void vc_image_or_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                        VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

/* Xor a region of one RGB888 image onto another. */

void vc_image_xor_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                         VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

/* Blt a region of one RGB888 image to another with nominated transparent colour in the src. */

void vc_image_transparent_blt_rgb888(VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                     VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int transparent_col);

/* Blt a region of one RGB888 image to another when dest has given colour. */

void vc_image_overwrite_blt_rgb888(VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                   VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int overwrite_col);

/* Masked fill of an RGB888 image using a 1bpp mask. */

void vc_image_masked_fill_rgb888(VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height, int value,
                                 VC_IMAGE_T *mask_image, int mask_x_offset, int mask_y_offset, int invert);

/* Masked blt of one RGB888 image into another using a 1bpp mask. */

void vc_image_masked_blt_rgb888(VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset,
                                VC_IMAGE_T *mask_image, int mask_x_offset, int mask_y_offset, int invert);

/* Not-in-place transpose. */

void vc_image_transpose_rgb888(VC_IMAGE_T *dest, VC_IMAGE_T *src);

/* In-place transpose. Relies on being able to preserve pitch. */

void vc_image_transpose_in_place_rgb888(VC_IMAGE_T *dest);

/* In-place vertical flip. */

void vc_image_vflip_in_place_rgb888(VC_IMAGE_T *dest);

/* In-place horizontal flip. */

void vc_image_hflip_in_place_rgb888(VC_IMAGE_T *dest);

/* Text. */

void vc_image_text_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                          int *max_x, int *max_y);

void vc_image_small_text_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                 int *max_x, int *max_y);

void vc_image_text_32_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                              int *max_x, int *max_y);

void vc_image_text_24_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                              int *max_x, int *max_y);

void vc_image_text_20_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                              int *max_x, int *max_y);


/* Line drawing. */

void vc_image_line_rgb888(VC_IMAGE_T *dest, int x_start, int y_start, int x_end, int y_end, int fg_colour);

/* Convert yuv to rgb888. */

void vc_image_convert_yuv_rgb888(VC_IMAGE_T *dest,VC_IMAGE_T *src,int mirror,int rotate);
void vc_image_convert_rgba32_rgb888 (VC_IMAGE_T *dest, VC_IMAGE_T *src, int mirror, int rotate);
void vc_image_convert_yuv422_rgb888(VC_IMAGE_T *dest, VC_IMAGE_T *src);

void rgb888_from_rgba32(char *dest888, int dpitch, char *srca32, int spitch);

void vc_image_yuv420_to_rgb888_part(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset);
void vc_image_yuv422_to_rgb888_part(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset);
/* Convert yuv to rgb888 with optional 2x downsample in each direction. */

void vc_image_yuv2rgb888(VC_IMAGE_T *dest, VC_IMAGE_T *src, int downsample);

void vc_3d32_to_rgb888(short *lcd_buffer,unsigned char *screen,int num,short *cmap);

void vc_3d32_to_rgb888_anti2(short *lcd_buffer,unsigned char *screen,int num,short *cmap);

void vc_palette8_to_rgb888( unsigned char *lcd_buffer ,unsigned char *screen,int width,int height,short *cmap,int screenwidth);

/* Blt a region of an RGB888 image to another with a nominated transparent colour and alpha blending. */

void vc_image_font_alpha_blt_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                    VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int red, int green, int blue, int alpha);

#endif
