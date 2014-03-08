/* ============================================================================
Copyright (c) 2010-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <stdio.h>
#include <stdlib.h>

#include "vc_image.h"
#include "vcinclude/common.h"

// these functions read an rgb block of 16x16 pixels into
// red:H(0++,0) green:H(0++,16) blue:H(0++,32) 
// they leave r0 as the value of the next block to read
extern void read_rgb565_block();
extern void read_rgb888_block();
extern void read_bgr888_block();
extern void read_rgba32_block();

// these functions write a yuv block of 16x16 pixels from
// YUV422 in Y:H(16++,0)REP16, 2U:H16(32++,0)REP8, 2V:H(48++,0)REP8
// dest y is r2, dest u/uv is r3, if yuv420/yuv422planar then dest v is r7, pitch is r8
extern void write_yuvuv_block();
extern void write_yuv420_block();
extern void write_yuv422planar_block();

// this function converts a strip from rgb to yuvuv/yuv420
// for yuv420, size ((left+15)&~15)x16 pixels
// for yuvuv, size min(128, ((left+15)&~15))x16 pixels
extern void yuv_strip_from_rgb(unsigned char *src, unsigned int src_pitch,
                               unsigned char *dst_y, unsigned char *dst_uv,
                               void (*read_rgb_block)(),
                               void (*write_yuv_block)(),
                               unsigned int left,
                               unsigned char *dst_v, unsigned int dst_pitch);

/******************************************************************************
NAME
   vc_image_yuv_from_rgb

SYNOPSIS
   void vc_image_yuv_from_rgb(VC_IMAGE_T *dest, VC_IMAGE_T *src)

FUNCTION
   Convert rgb565/rgb888/rgba32 to yuvuv/yuv420/yuv422planar format (can't use in place)

RETURNS
   void
******************************************************************************/
void vc_image_rgb_to_yuv(VC_IMAGE_T *dest, VC_IMAGE_T *src)
{
   int i, j;
   unsigned char *inrgbptr, *outyptr, *outuvptr, *outvptr;
   int nstrips, rgbinc, yinc, uvinc;
   void (*read_rgb_block)();
   void (*write_yuv_block)();
   int left = src->width, lmax;

   switch(src->type) {
   case VC_IMAGE_RGB565: read_rgb_block = read_rgb565_block; rgbinc = 256; break;
   case VC_IMAGE_RGB888: read_rgb_block = read_rgb888_block; rgbinc = 384; break;
   case VC_IMAGE_BGR888: read_rgb_block = read_bgr888_block; rgbinc = 384; break;
   case VC_IMAGE_RGBA32: read_rgb_block = read_rgba32_block; rgbinc = 512; break;
   default: return;
   }

   switch(dest->type) {
   case VC_IMAGE_YUV_UV:
      write_yuv_block = write_yuvuv_block;    
      nstrips = ((src->width)+VC_IMAGE_YUV_UV_STRIPE_WIDTH-1)>>VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2;
      yinc = 128;
      uvinc = 64;
      lmax = 128;
      break;
   case VC_IMAGE_YUV420:
      write_yuv_block = write_yuv420_block;
      nstrips = 1;
      yinc = dest->pitch;
      uvinc = dest->pitch>>2;
      lmax = dest->width;
      break;
   case VC_IMAGE_YUV422PLANAR:
      write_yuv_block = write_yuv422planar_block;
      nstrips = 1;
      yinc = dest->pitch;
      uvinc = dest->pitch>>1;
      lmax = dest->width;
      break;
   default: return;
   }


   inrgbptr = src->image_data;
   outyptr = dest->image_data;
   outuvptr = dest->extra.uv.u;
   outvptr = dest->extra.uv.v;

   for(i=0; i<nstrips; i++)
   {
      unsigned char *in = inrgbptr;
      unsigned char *outy = outyptr;
      unsigned char *outuv = outuvptr;
      unsigned char *outv = outvptr;

      for(j=0; j<dest->height; j+=16)
      {
         yuv_strip_from_rgb(in, src->pitch, outy, outuv, read_rgb_block, write_yuv_block, _min(lmax, left), outv, yinc);

         in += src->pitch<<4;
         outy += yinc<<4;
         outuv += uvinc<<4;
         outv += uvinc<<4;
      }

      inrgbptr += rgbinc;
      outyptr += dest->pitch;
      outuvptr += dest->pitch;
      left -= 128;
   }
}
