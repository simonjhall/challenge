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

#ifndef VC_IMAGE_RESIZE_BYTES_H
#define VC_IMAGE_RESIZE_BYTES_H

/* vc_image_resize_bytes_vert.s */

void vcir8_rowinterp(unsigned char * dest,
                     const unsigned char * src,
                     int width,
                     int src_pitch,
                     int weight_65536);

void vcir8_rowmix(unsigned char * dest, const unsigned char * src,
                  int width, int src_pitch, int num_rows);

/* vc_image_resize_bytes_hoz.s */

void vcir8_margin(unsigned char * dest, const unsigned char * src,
                  int width, int height, int dest_pitch, int src_pitch);

void vcir8_lerp_16(unsigned char * dest, const unsigned char * src,
                   int dest_width, int pos_65536, int step_65536,
                   int dest_pitch, int src_pitch, int nrows);

void vcir8_lerp_n(unsigned char * dest, const unsigned char * src,
                  int dest_width, int pos_65536, int step_65536,
                  int dest_pitch, int src_pitch, int nrows);

void vcir8_nnbr_16(unsigned char * dest, const unsigned char * src,
                   int dest_width, int pos_65536, int step_65536,
                   int dest_pitch, int src_pitch, int nrows);

void vcir8_nnbr_n(unsigned char * dest, const unsigned char * src,
                  int dest_width, int pos_65536, int step_65536,
                  int dest_pitch, int src_pitch, int nrows);

void vcir8_bavg_16(unsigned char * dest, const unsigned char * src,
                   int dest_width, int pos_65536, int step_65536,
                   int dest_pitch, int src_pitch, int nrows);

void vcir8_bavg_n(unsigned char * dest, const unsigned char * src,
                  int dest_width, int pos_65536, int step_65536,
                  int dest_pitch, int src_pitch, int nrows);

void vcir8_down2_8(unsigned char * dest,
                   const unsigned char * src,
                   int destwidth,
                   int dest_pitch,
                   int src_pitch,
                   int smooth_flag);

/* vc_image_resize_bytes.c */

void vcir8_hzoom(unsigned char * dest, int destw, int h, int destp,
                 const unsigned char * src, int srcw, int srcp, int smooth);

void vcir8_vzoom2(unsigned char * dest, int w, int desth, int dest_pitch,
                  const unsigned char * src, int src_pitch,
                  int pos, int step, int limit,
                  int smooth_flag);

void vc_image_resize_bytes(unsigned char * dest_ptr,
                           int dest_width, int dest_height, int dest_pitch,
                           const unsigned char * src_ptr,
                           int src_width, int src_height, int src_pitch,
                           int smooth_flag);

void vc_image_resize_bytes2(unsigned char * dest_ptr,
                            int dest_width, int dest_height, int dest_pitch,
                            const unsigned char * src_ptr,
                            int src_width, int src_pitch,
                            int pos, int step, int limit,
                            int smooth_flag,
                            void * tmp_buf, int tmp_buf_size);

#endif
