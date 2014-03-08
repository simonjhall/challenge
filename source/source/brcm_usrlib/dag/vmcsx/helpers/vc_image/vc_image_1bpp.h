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

#ifndef VC_IMAGE_1BPP_H
#define VC_IMAGE_1BPP_H

#include "vc_image.h"

/* Do big to little endian conversion on 1bpp image. */

void vc_image_endian_translate(VC_IMAGE_T *dest, VC_IMAGE_T *src);

/* Transpose an image. */

void vc_image_transpose_1bpp(VC_IMAGE_T *dest, VC_IMAGE_T *src);

/* Vertically flip an image (turn "upside down"). */

void vc_image_vflip_1bpp(VC_IMAGE_T *dest, VC_IMAGE_T *src);

/* Horizontally flip an image ("mirror"). */

void vc_image_hflip_1bpp(VC_IMAGE_T *dest, VC_IMAGE_T *src);

/* Vertically flip an image in place (turn "upside down"). */

void vc_image_vflip_in_place_1bpp(VC_IMAGE_T *dest);

/* Horizontally flip an image in place ("mirror"). */

void vc_image_hflip_in_place_1bpp(VC_IMAGE_T *dest);

/* Copy a rectangle to an image */

void vc_image_copy_1bpp_src_offset(VC_IMAGE_T *dst, int w, int h, VC_IMAGE_T *src, int x_src, int y_src);
void vc_image_copy_1bpp_dst_offset(VC_IMAGE_T *dst, int x_dst, int y_dst, VC_IMAGE_T *src, int w, int h);

/* Fill region. */

void vc_image_fill_1bpp(VC_IMAGE_T *dst, int x, int y, int w, int h, int value);

#endif

