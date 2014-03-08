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

#ifndef VC_IMAGE_YUV_H
#define VC_IMAGE_YUV_H

#include "vc_image.h"

/* Vertically flip an image (turn "upside down"). */

void vc_image_vflip_in_place_yuv420(VC_IMAGE_T *dest);

/* Horizontally flip an image ("mirror"). */

void vc_image_hflip_in_place_yuv420(VC_IMAGE_T *dest);

/* Deblock a YUV image. */

void vc_yuv_deblock(unsigned char *data,int width,int height);

/* Draw text onto a yuv image */
void vc_image_text_yuv (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,int rotate);

/* YUV blt */

void vc_image_blt_yuv(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                      VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

/* YUV copying transpose */

void vc_image_transpose_yuv420(VC_IMAGE_T *dest, VC_IMAGE_T *src);
void vc_image_transpose_tiles_yuv420(VC_IMAGE_T *dest, VC_IMAGE_T *src, int fat);

void vc_image_fill_yuv420(VC_IMAGE_T *dst, int x_offset, int y_offset, int width, int height, int value);
void vc_image_fill_yuv422(VC_IMAGE_T *dst, int x_offset, int y_offset, int width, int height, int value);

void vc_image_resize_stripe_h_yuv420(VC_IMAGE_T *dest, int d_width, int s_width);
void vc_image_resize_stripe_v_yuv420(VC_IMAGE_T *dest, int d_height, int s_height);

#endif
