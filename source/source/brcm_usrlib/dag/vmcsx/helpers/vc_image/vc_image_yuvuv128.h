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

#ifndef VC_IMAGE_YUVUV128_H
#define VC_IMAGE_YUVUV128_H

#include "vc_image.h"

/* Convert to YUV420 */
void vc_image_yuvuv128_to_yuv420(VC_IMAGE_T *dest, VC_IMAGE_T *src);

/* Convert from YUV420 */
void vc_image_yuvuv128_from_yuv420(VC_IMAGE_T *dest, VC_IMAGE_T *src);

/* Convert blit from YUVUV to YUV420*/
void vc_image_yuvuv128_to_yuv420_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset);

/* Convert blit from YUVUV to YUV420*/
void vc_image_yuvuv128_from_yuv420_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset);

/* vflip YUV_UV128 image in place */
void vc_image_vflip_in_place_yuvuv128(VC_IMAGE_T *img);

/* hflip YUV_UV128 image in place */
void vc_image_hflip_in_place_yuvuv128(VC_IMAGE_T *img);

/* transpose YUV_UV128 from src to dest */
void vc_image_transpose_yuvuv128(VC_IMAGE_T *dest, VC_IMAGE_T *src);

/* transpose and decimate YUV_UV128 from src to dst */
void vc_image_transpose_and_subsample_yuvuv128(VC_IMAGE_BUF_T *dst, VC_IMAGE_BUF_T *src);

/* resize YUV_UV128 image down by a factor of 2x2 from src to dst */
void vc_image_subsample_yuvuv128(VC_IMAGE_BUF_T *dst, VC_IMAGE_BUF_T *src);

/* horizontally scale in place a 16 high stripe of YUV_UV128 */
void vc_image_resize_stripe_h_yuvuv128(VC_IMAGE_T *dest, int d_width, int s_width);

/* internal assembler functions used to perform horizontal resize of stripes of YUV_UV128 images */
void yuvuv128_h_expand_y_stripe(unsigned char *dest, unsigned int dest_pitch, unsigned char *src, unsigned int src_pitch,
                                unsigned int dest_width, unsigned int step, unsigned int source_offset, unsigned int source_width);
void yuvuv128_h_expand_uv_stripe(unsigned char *dest, unsigned int dest_pitch, unsigned char *src, unsigned int src_pitch,
                                 unsigned int dest_width, unsigned int step, unsigned int source_offset, unsigned int source_width);
void yuvuv128_h_reduce_y_stripe(unsigned char *dest, unsigned int dest_pitch, unsigned char *src, unsigned int src_pitch,
                                unsigned int columns, unsigned int step);
void yuvuv128_h_reduce_uv_stripe(unsigned char *dest, unsigned int dest_pitch, unsigned char *src, unsigned int src_pitch,
                                 unsigned int columns, unsigned int step);
void yuvuv128_h_reduce2_y_stripe(unsigned char *dest, unsigned int dest_pitch, unsigned char *src, unsigned int src_pitch,
                                 unsigned int columns, unsigned int step);
void yuvuv128_h_reduce2_uv_stripe(unsigned char *dest, unsigned int dest_pitch, unsigned char *src, unsigned int src_pitch,
                                  unsigned int columns, unsigned int step);

/* shift a yuvuv128 strip left by offset pixels (offset must be multiple of 2) */
void yuvuv128_move_stripe_left(unsigned char *y, unsigned char *uv, unsigned int nstrips, unsigned int pitch, unsigned int offset);

#endif
