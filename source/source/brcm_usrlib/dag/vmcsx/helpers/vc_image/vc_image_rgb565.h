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

#ifndef VC_IMAGE_RGB565_H
#define VC_IMAGE_RGB565_H

#include "vc_image.h"

/* Fill a region of an RGB565 image with solid colour. */

void vc_image_fill_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int value);

/* Blt a region of one RGB565 image to another. */

void vc_image_blt_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                         VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

/* Blt a region of an RGB565 image to another with a nominated transparent colour. */

void vc_image_transparent_blt_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                     VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int transparent_colour);

/* Blt a region of an RGB565 image to another with a nominated transparent colour and alpha blending. */

void vc_image_transparent_alpha_blt_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
      VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int transparent_colour,int alpha);

/* Blt a region of an RGB565 image to another overwriting only a nominated colour. */

void vc_image_overwrite_blt_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                   VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int overwrite_colour);

/* Masked blt of one RGB565 image to another. */

void vc_image_masked_blt_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                 VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset,
                                 VC_IMAGE_T *mask_image, int mask_x_offset, int mask_y_offset,
                                 int invert);

/* Masked fill a region of an RGB565 image with a solid colour. */

void vc_image_masked_fill_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int value,
                                 VC_IMAGE_T *mask, int mask_x_offset, int mask_y_offset, int invert);

/* Not a region of an RGB565 image. */

void vc_image_not_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height);

/* Fade (or brighten) a region of an RGB565 image. fade is an 8.8 value.*/

void vc_image_fade_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int fade);

/* OR two RGB565 images together. */

void vc_image_or_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                        VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

/* XOR two RGB565 images together. */

void vc_image_xor_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                         VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

/* Transpose an image. */

void vc_image_transpose_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T *src);

/* Transpose tiles within an image. */

void vc_image_transpose_tiles_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T *src, int hdeci);

/* Vertically flip an image (turn "upside down"). */

void vc_image_vflip_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T *src);

/* Horizontally flip an image ("mirror"). */

void vc_image_hflip_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T *src);

/* Transpose an image. */

void vc_image_transpose_in_place_rgb565(VC_IMAGE_T *dest);

/* Vertically flip an image (turn "upside down"). */

void vc_image_vflip_in_place_rgb565(VC_IMAGE_T *dest);

/* Horizontally flip an image ("mirror"). */

void vc_image_hflip_in_place_rgb565(VC_IMAGE_T *dest);

/* Add text string to an image. */

void vc_image_text_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                          int *max_x, int *max_y);

void vc_image_small_text_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                int *max_x, int *max_y);

void vc_image_text_32_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                              int *max_x, int *max_y);

void vc_image_text_24_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                              int *max_x, int *max_y);

void vc_image_text_20_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                              int *max_x, int *max_y);

/* Line drawing. */

void vc_image_line_rgb565(VC_IMAGE_T *dest, int x_start, int y_start, int x_end, int y_end, int fg_colour);

/* Copy and convert image to 48bpp. WARNING: offsets, width and height must be 16-pixel aligned. */

void vc_image_convert_to_48bpp_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                      VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

/* Decompresses 16-bit runlength encoded data into the given buffer (assumes that width and height have been removed) */
void vc_rgb565_runlength(unsigned short *p,unsigned short *out_buffer);

/* Decompresses 8-bit runlength encoded data into the given buffer (assumes that width and height have been removed) */
void vc_runlength8(unsigned short *p,unsigned short *out_buffer);

/* Decompress a 16 by 16 block */
void vc_4by4_decompress(unsigned short *data,unsigned short *out_buffer,int pitch);

/* Convert yuv to rgb565. */

void vc_image_convert_yuv_rgb565(VC_IMAGE_T *dest,VC_IMAGE_T *src,int mirror,int rotate);

void vc_image_convert_yuv422_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T *src);

void vc_image_yuv_uv_to_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

void vc_image_yuv420_to_rgb565_part(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset);
void vc_image_yuv422_to_rgb565_part(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset);

void vc_image_8bpp_to_rgb565_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset,
   short               *pal);

void vc_image_3d32_to_rgb565_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset,
   short               *pal);

void vc_image_3d32b_to_rgb565_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset);

/* Convert yuv to rgb565 with optional 2x downsample in each direction. */

void vc_image_yuv2rgb565(VC_IMAGE_T *dest, VC_IMAGE_T *src, int downsample);

void vc_3d32_to_rgb565(short *lcd_buffer,unsigned char *screen,int num,short *cmap);

void vc_3d32b_to_rgb565(short *lcd_buffer,unsigned char *screen,int num,short *cmap);

void vc_3d32_to_rgb565_anti2(short *lcd_buffer,unsigned char *screen,int num,short *cmap);

void vc_3d32b_to_rgb565_anti2(short *lcd_buffer,unsigned char *screen,int num,short *cmap);

void vc_palette8_to_rgb565( unsigned char *lcd_buffer ,unsigned char *screen,int width,int height,short *cmap,int screenwidth);

void vc_3d32mat_to_rgb565(short *lcd_buffer,unsigned char *screen,int num,short *cmap,int ambient,int specular);

/* Blt a region of an RGB565 image to another with a nominated transparent colour and alpha blending. */

void vc_image_font_alpha_blt_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                    VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int red, int green, int blue, int alpha);


/* Blt a region of an RGB565 image to another with a nominated transparent colour and alpha blending. */
void vc_image_transparent_alpha_blt_rgb565_from_pal8(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
      VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int transparent_colour,int alpha);



void vc_image_resize_stripe_h_rgb565(VC_IMAGE_T *dest, int d_width, int s_width);
void vc_image_resize_stripe_v_rgb565(VC_IMAGE_T *dest, int d_height, int s_height);

void vc_roz_blt_8bpp_to_rgb565(unsigned char *dst_bitmap,unsigned char *src_bitmap,int dst_pitch,int src_pitch,int dst_width,int dst_height,
                               int src_width, int src_height,int incxx,int incxy,int incyx,int incyy,unsigned int cx, unsigned int cy, int transparent_color, short *pal);
void vc_roz_blt_rgb565_to_rgb565(unsigned char *dst_bitmap,unsigned char *src_bitmap,int dst_pitch,int src_pitch,int dst_width,int dst_height,
                                 int src_width, int src_height,int incxx,int incxy,int incyx,int incyy,unsigned int cx, unsigned int cy, int transparent_color);

void vc_image_rgba32_to_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T *src);


#endif
