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

#if !defined(STRIPERESIZE_H_INCLUDED)
#define STRIPERESIZE_H_INCLUDED

/* YUV-family resizes */

void vc_stripe_h_reduce_bytes(
   unsigned char       *dest,
   int                  dpitch,
   unsigned char const *src,
   int                  spitch,
   int                  columns,
   int                  step);

void vc_stripe_h_reduce_bytes2(
   unsigned char       *dest,
   int                  dpitch,
   unsigned char const *src,
   int                  spitch,
   int                  columns,
   int                  step);

void vc_stripe_h_expand_bytes(
   unsigned char       *dest,
   int                  dpitch,
   unsigned char const *src,
   int                  spitch,
   int                  columns,
   int                  step,
   int                  s_width);

void vc_stripe_v_reduce_bytes(
   unsigned char       *dest,
   int                  dpitch,
   unsigned char const *src,
   int                  spitch,
   int                  rows,
   int                  step);

void vc_stripe_v_reduce_bytes2(
   unsigned char       *dest,
   int                  dpitch,
   unsigned char const *src,
   int                  spitch,
   int                  rows,
   int                  step);

void vc_stripe_v_expand_bytes(
   unsigned char       *dest,
   int                  dpitch,
   unsigned char const *src,
   int                  spitch,
   int                  rows,
   int                  step,
   int                  s_height);

/* half-width vertical resizes */

void vc_stripe_v_reduce_uv(
   unsigned char       *dest,
   unsigned char       *dest2,
   int                  dpitch,
   unsigned char const *src,
   unsigned char const *src2,
   int                  spitch,
   int                  rows,
   int                  step);

void vc_stripe_v_reduce_uv2(
   unsigned char       *dest,
   unsigned char       *dest2,
   int                  dpitch,
   unsigned char const *src,
   unsigned char const *src2,
   int                  spitch,
   int                  rows,
   int                  step);

void vc_stripe_v_expand_uv(
   unsigned char       *dest,
   unsigned char       *dest2,
   int                  dpitch,
   unsigned char const *src,
   unsigned char const *src2,
   int                  spitch,
   int                  rows,
   int                  step,
   int                  s_height);

/* Bonus YUV resizes */

void vc_stripe_h_reduce_bytes_unsmoothed(
   unsigned char       *dest,
   int                  dpitch,
   unsigned char const *src,
   int                  spitch,
   int                  columns,
   int                  step);

void vc_stripe_v_reduce_bytes_unsmoothed(
   unsigned char       *dest,
   int                  dpitch,
   unsigned char const *src,
   int                  spitch,
   int                  rows,
   int                  step);


/* RGBA32 resizes */

void vc_stripe_h_reduce_rgba32(
   unsigned long       *dest,
   int                  dpitch,
   unsigned long const *src,
   int                  spitch,
   int                  columns,
   int                  step);

void vc_stripe_h_reduce_rgba322(
   unsigned long       *dest,
   int                  dpitch,
   unsigned long const *src,
   int                  spitch,
   int                  columns,
   int                  step);

void vc_stripe_h_expand_rgba32(
   unsigned long       *dest,
   int                  dpitch,
   unsigned long const *src,
   int                  spitch,
   int                  columns,
   int                  step,
   int                  s_width);

/* vertical RGBA32 resizes are covered by the _bytes (YUV) functions */


/* RGB565 resizes */

void vc_stripe_h_reduce_rgb565(
   unsigned short       *dest,
   int                  dpitch,
   unsigned short const *src,
   int                  spitch,
   int                  columns,
   int                  step);

void vc_stripe_h_reduce_rgb5652(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  columns,
   int                  step);

void vc_stripe_h_expand_rgb565(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  columns,
   int                  step,
   int                  s_width);

void vc_stripe_v_reduce_rgb565(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  rows,
   int                  step);

void vc_stripe_v_reduce_rgb5652(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  rows,
   int                  step);

void vc_stripe_v_expand_rgb565(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  rows,
   int                  step,
   int                  s_height);


/* RGBA565 resizes */

void vc_stripe_h_reduce_rgba565(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  columns,
   int                  step);

void vc_stripe_h_reduce_rgba5652(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  columns,
   int                  step);

void vc_stripe_h_expand_rgba565(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  columns,
   int                  step,
   int                  s_width);

void vc_stripe_v_reduce_rgba565(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  rows,
   int                  step);

void vc_stripe_v_reduce_rgba5652(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  rows,
   int                  step);

void vc_stripe_v_expand_rgba565(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  rows,
   int                  step,
   int                  s_height);


/* RGBA16 resizes */

void vc_stripe_h_reduce_rgba16(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  columns,
   int                  step);

void vc_stripe_h_reduce_rgba162(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  columns,
   int                  step);

void vc_stripe_h_expand_rgba16(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  columns,
   int                  step,
   int                  s_width);

void vc_stripe_v_reduce_rgba16(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  rows,
   int                  step);

void vc_stripe_v_reduce_rgba162(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  rows,
   int                  step);

void vc_stripe_v_expand_rgba16(
   unsigned short      *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  rows,
   int                  step,
   int                  s_height);

void vc_decimate_vert16(void *image, int pitch, int offset, int step);
void vc_decimate_verta16(void *image, int pitch, int offset, int step);
void vc_decimate_vert_rgba565(void *image, int pitch, int offset, int step);

/* vc_decimate_vert can be called for out of place if destination!=source
 * or for in place where destination=source and destination_pitch=source_pitch.
 * valid is the nubmer of valid source lines after offset>>16, 0 indicates don't care.
 * step must be smaller than 64<<16 */
void vc_decimate_vert(void *destination, int destination_pitch, void *source, int source_pitch, int offset, int step, int valid);

void vc_decimate_vert_unsmoothed(void *image, int pitch, void *dummy1, void *dummy2, int offset, int step);
void vc_decimate_vert_cubic(void *image, int pitch, void *dummy1, void *dummy2, int offset, int step);

/* vc_decimate_uv_vert can be called for out of place if pointers are not equal
 * or for in place. Valid indicates how many valid source lines there are,
 * where offset>>16 is the first source line read, 0 indicates don't care.
 * step must be smaller than 64<<16.
 * dest_v may be zero, and if so then all reads from src_v and writes to dest_v
 * are skipped. */
void vc_decimate_uv_vert(void *dest_u, void *dest_v, int dest_pitch, void *src_u, void *src_v, int source_pitch, int offset, int step, int valid);


#endif /* !defined(STRIPERESIZE_H_INCLUDED) */
