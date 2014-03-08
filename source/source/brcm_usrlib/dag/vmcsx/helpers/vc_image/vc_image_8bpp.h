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

#ifndef VC_IMAGE_8BPP_H
#define VC_IMAGE_8BPP_H

#include "vc_image.h"

/* Vertically flip an image (turn "upside down"). */

void vc_image_vflip_in_place_8bpp(VC_IMAGE_T *dest);

/* Horizontally flip an image ("mirror"). */

void vc_image_hflip_in_place_8bpp(VC_IMAGE_T *dest);

void vc_image_transpose_tiles_8bpp(VC_IMAGE_T *dst, VC_IMAGE_T *src, int fat);

void vc_image_transpose_8bpp(VC_IMAGE_T *dst, VC_IMAGE_T *src);

void vc_image_fill_8bpp(VC_IMAGE_T *dst, int x_offset, int y_offset, int width, int height, int value);

void vc_image_line_8bpp(VC_IMAGE_T *dest, int x_start, int y_start, int x_end, int y_end, int fg_colour);

void vc_image_small_text_8bpp (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                               int *max_x, int *max_y);

void vc_image_transparent_alpha_blt_8bpp_from_pal8 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
      VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int transparent_colour,int alpha);

/* Decompresses 8-bit runlength encoded data into the given buffer (assumes that width and height have been removed) */
void vc_runlength8(unsigned short *p,unsigned short *out_buffer);

void vc_image_resize_stripe_h_8bpp(VC_IMAGE_T *dest, int d_width, int s_width);
void vc_image_resize_stripe_v_8bpp(VC_IMAGE_T *dest, int d_height, int s_height);

void vc_roz_blt_8bpp_to_8bpp(unsigned char *dst_bitmap,unsigned char *src_bitmap,int dst_pitch,int src_pitch,int dst_width,int dst_height,
                             int src_width, int src_height,int incxx,int incxy,int incyx,int incyy,unsigned int cx, unsigned int cy, int transparent_color);

/* Font alpha blit function, shared between 8BPP, YUV420 and YUV422 types */
void vc_image_font_alpha_blt_8bpp(VC_IMAGE_T *dest, int dest_x_offset, int dest_y_offset, int width, int height,
                                  VC_IMAGE_T *src, int src_x_offset, int src_y_offset,
                                  int colour, int alpha, int antialias_flag);

#endif
