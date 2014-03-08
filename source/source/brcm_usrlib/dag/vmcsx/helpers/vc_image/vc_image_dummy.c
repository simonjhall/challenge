/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/* System includes */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

/* Project includes */
#include "vc_image.h"

// vc_image
void	vc_image_line_x(struct VC_IMAGE_T *dest, int x_start, int x_end, int y_start, int gradient, int fg_colour){};
void	vc_image_line_horiz(struct VC_IMAGE_T *dest, int x_start, int y_start, int x_end, int y_end, int fg_colour){};
void	vc_image_line_y(struct VC_IMAGE_T *dest, int x_start, int x_end, int y_start, int gradient, int fg_colour){};
void	vc_image_line_vert(struct VC_IMAGE_T *dest, int x_start, int y_start, int x_end, int y_end, int fg_colour){};
void	vc_image_copy_stripe(unsigned char *dest,int dest_pitch,unsigned char *src,int src_pitch,int width_nblocks){};
void	vc_image_unpack_row(unsigned char *,unsigned char *,int);
void	vc_image_convert_to_16bpp(VC_IMAGE_T *, int, int, int, int, VC_IMAGE_T *, int, int);
void	vc_image_text_rgba32(struct VC_IMAGE_T *,int,int,long,long,char const *,int *,int *);

void	vc_block_rgb2yuv(unsigned short *,int,unsigned char *,unsigned char *,unsigned char *,int);
void	vc_yuv16_hflip(unsigned char *,unsigned char *,unsigned char *,unsigned short *,int,int,int);

void    vc_deflicker_yuv(unsigned char *,int,unsigned char *);
void    vc_deflicker_yuv8(unsigned char *,int,unsigned char *);

void    vc_decimate_horiz32(void *,int,int,int,int);
void    vc_decimate_2_horiz32(void *,int,int);
void    vc_decimate_vert(void *,int,void *,int,int,int,int);

void    vc_rgba32_font_alpha_blt(unsigned long *,unsigned char *,int,int,int,int,int,int,int,int);

void    vc_rgb565_vflip(unsigned short *,unsigned short *,int,int,int,int);
void    vc_rgb565_vflip_in_place(unsigned short *,unsigned short *,int,int);

void    vc_stripe_h_expand_rgba32(unsigned long *,int,unsigned long const *,int,int,int,int);
void    vc_stripe_h_reduce_rgba32(unsigned long *,int,unsigned long const *,int,int,int);
void    vc_stripe_h_reduce_rgba322(unsigned long *,int,unsigned long const *,int,int,int);
void    vc_stripe_v_expand_bytes(unsigned char *,int,unsigned char const *,int,int,int,int);
void    vc_stripe_v_reduce_bytes(unsigned char *,int,unsigned char const *,int,int,int);
void    vc_stripe_v_reduce_bytes2(unsigned char *,int,unsigned char const *,int,int,int);

void    yuv_to_rgba32_store_mxn_misaligned(unsigned char *,int,...);
void    yuv_to_rgba32_store_16x16(unsigned char *,int,...);
void    yuv_to_rgba32_store_32x16(unsigned char *,int,...);
void    yuv_to_rgba32_store_32xn_misaligned(unsigned char *,int,...);

void    yuv422_to_rgb_load_32x16(unsigned char const *,unsigned char const *,unsigned char const *,int,int,void (*)(unsigned char *,int,...),unsigned char *,int,...);
void    yuv422_to_rgb_load_32x16_misaligned(unsigned char const *,unsigned char const *,unsigned char const *,int,int,void (*)(unsigned char *,int,...),unsigned char *,int,...);

void    yuv420_to_rgb_load_32x16(unsigned char const *,unsigned char const *,unsigned char const *,int,int,void (*)(unsigned char *,int,...),unsigned char *,int,...);
void    yuv420_to_rgb_load_32x16_misaligned(unsigned char const *,unsigned char const *,unsigned char const *,int,int,void (*)(unsigned char *,int,...),unsigned char *,int,...);

void    rgba32_from_rgb888(unsigned long *,int,unsigned char const *,int,int);
void    rgba32_from_bgr888(unsigned long *,int,unsigned char const *,int,int);
void    rgba32_from_transparent_rgb565(unsigned long *,int,unsigned short const *,int,int,int);
void    rgba32_from_transparent_4bpp(unsigned long *,int,unsigned char const *,int,int,int,unsigned short const *);
void    rgba32_composite(unsigned long *,int,unsigned long const *,int,int,int,int,int);
void    rgba32_composite_normalised(unsigned long *,int,unsigned long const *,int,int,int,int,int);
void    rgba32_copy_alpha_from_8bpp(unsigned char *,int,unsigned char const *,int);
void    rgba32_alpha_adjust8(unsigned char *,int,unsigned char const *,int,int,int);
void    rgba32_transpose_chunky1(unsigned long *,int,unsigned long const *,int);
void    rgba32_transpose_chunky0(unsigned long *,int,unsigned long const *,int);
void    rgba32_transpose_chunky(unsigned long *,int,unsigned long const *,int);
void    rgba32_hflip_in_place(unsigned long *,int,int);
void    rgba32_premultiply(unsigned long *,int,unsigned long *,int,int,int);

void    rgba32_overlay_to_rgb565_blocky(unsigned short *,int,unsigned long const *,int,int);
void    rgba32_overlay_to_rgb888_blocky(unsigned char *,int,unsigned long const *,int,int);
void    rgba32_overlay_to_rgba32_n(unsigned long *,int,unsigned long const *,int,int,unsigned long,int);
void    rgba32_overlay_to_rgba32(unsigned long *,int,unsigned long const *,int,unsigned long,int);
void    rgba32_overlay_to_rgb888(unsigned char *,int,unsigned long const *,int,int,unsigned __int64);
void    rgba32_overlay_to_rgb888_n(unsigned char *,int,unsigned long const *,int,int,int,unsigned __int64);
void    rgba32_overlay_to_rgb565(unsigned short *,int,unsigned long const *,int,int,unsigned short);
void    rgba32_overlay_to_rgb565_n(unsigned short *,int,unsigned long const *,int,int,int,unsigned short);
void    rgb565_overlay_to_rgba32_n(unsigned long *,int,unsigned short const *,int,int,unsigned short,int,int);
