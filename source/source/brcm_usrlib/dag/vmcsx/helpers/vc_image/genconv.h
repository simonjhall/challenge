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

#if !defined(GENCONV_H_INCLUDED)
#define GENCONV_H_INCLUDED

void vc_genconv_rgb565_to_rgb888(
   unsigned char       *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  columns,
   int                  lines);

void vc_genconv_3d32_to_rgb888(
   unsigned char       *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  columns,
   int                  lines,
   unsigned short const*cmap);

void vc_genconv_3d32b_to_rgb888(
   unsigned char       *dest,
   int                  dpitch,
   unsigned short const*src,
   int                  spitch,
   int                  columns,
   int                  lines);

void vc_genconv_8bpp_to_rgb888(
   unsigned char       *dest,
   int                  dpitch,
   unsigned char const *src,
   int                  spitch,
   int                  columns,
   int                  lines,
   short               *pal);

void vc_genconv_yuv420_to_rgb888(
   unsigned char       *dest,
   int                  dpitch,
   unsigned char const *src,
   int                  spitch,
   int                  columns,
   int                  lines,
   unsigned char const *src_u,
   unsigned char const *src_v,
   int                  pitch_uv,
   int                  mode);

void vc_genconv_8bpp_to_rgb565(
   unsigned char       *dest,
   int                  dpitch,
   unsigned char const *src,
   int                  spitch,
   int                  columns,
   int                  lines,
   short               *pal);

void vc_genconv_3d32_to_rgb565(
   unsigned char       *dest,
   int                  dpitch,
   unsigned char const *src,
   int                  spitch,
   int                  columns,
   int                  lines,
   short               *cmap);

void vc_genconv_3d32b_to_rgb565(
   unsigned char       *dest,
   int                  dpitch,
   unsigned char const  *src,
   int                  spitch,
   int                  columns,
   int                  lines);

#endif /* !defined(GENCONV_H_INCLUDED) */
