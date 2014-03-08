/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include "vc_image.h"
#include "vc_image_bgr888.h"
#include "helpers/vclib/vclib.h"
#include "interface/vcos/vcos_assert.h"
/* ============================================================================
 NAME: rgbswap
 SYNOPSIS: void rgbswap(uint8_t* dest, uint8_t* src, uint32_t width, uint32_t height, VC_IMAGE_TYPE_T src_format, VC_IMAGE_TYPE_T dest_format)
 FUNCTION: swap the R & B components of an image, the image pitch is calculated depending on whether the image has pitch or not
 ARGUMENT: destination, source, width, height and image format
 RETURN: -

 REMARK: only support 24-bit colour at the moment, destination must be in internal memory
 ============================================================================== */

extern void vc_rgbswaprgb(uint8_t* dest, uint8_t* src, uint32_t width, uint32_t height, uint32_t source_pitch, uint32_t dest_pitch);

void rgbswap(unsigned char* dest, unsigned char* src, unsigned int width, unsigned int height, VC_IMAGE_TYPE_T src_format, VC_IMAGE_TYPE_T dest_format) {
   int src_pitch = (src_format == VC_IMAGE_BGR888_NP)? width*3 : ((width+15)&~15)*3;
   int dest_pitch = (dest_format == VC_IMAGE_BGR888_NP)? width*3 : ((width+15)&~15)*3;

   if ( (src_format == VC_IMAGE_RGB888 && (dest_format == VC_IMAGE_BGR888 || dest_format == VC_IMAGE_BGR888_NP)) ||
         (dest_format == VC_IMAGE_RGB888 && (src_format == VC_IMAGE_BGR888 || src_format == VC_IMAGE_BGR888_NP)) ) {
      vc_rgbswaprgb(dest, src, width, height, src_pitch, dest_pitch);
   } else {
      vcos_assert(!"format not supported.");
   }
}

/* ============================================================================
 NAME: rgb_stripe_swap
 SYNOPSIS: void rgb_stripe_swap(uint8_t* dest, uint8_t* src, uint32_t width, uint32_t height, VC_IMAGE_TYPE_T src_format, VC_IMAGE_TYPE_T dest_format)
 FUNCTION: swap the R & B components of an image, the image pitch is calculated depending on whether the image has pitch or not
 ARGUMENT: destination, source, width, height and image format
 RETURN: -

 REMARK: only supports 24-bit colour at the moment, also handles destination in
         external memory, in which case a stripe buffer will be allocated
 ============================================================================== */
void rgb_stripe_swap(uint8_t* dest, uint8_t* src, uint32_t width, uint32_t height, VC_IMAGE_TYPE_T src_format, VC_IMAGE_TYPE_T dest_format) {

   int height_to_do = height;
   int src_pitch = (src_format == VC_IMAGE_BGR888_NP)? width*3 : ((width+15)&~15)*3;
   int dest_pitch = (dest_format == VC_IMAGE_BGR888_NP)? width*3 : ((width+15)&~15)*3;
   if ( (src_format == VC_IMAGE_RGB888 && (dest_format == VC_IMAGE_BGR888 || dest_format == VC_IMAGE_BGR888_NP)) ||
         (dest_format == VC_IMAGE_RGB888 && (src_format == VC_IMAGE_BGR888 || src_format == VC_IMAGE_BGR888_NP)) ) {
      //if the destination is not in internal memory we have to create a temporary stripe
      uint8_t *mydest = dest;
      if ((int32_t) mydest < 0) {
         mydest = vclib_prioritymalloc(dest_pitch << 4, VCLIB_ALIGN_256BIT, VCLIB_PRIORITY_INTERNAL, "rgb swap");
         if (vcos_verify(mydest != NULL))
         {
            for (; height_to_do > 0; height_to_do -= 16, src += (src_pitch << 4), dest += (dest_pitch << 4)) {
               int rows = (height_to_do > 16)? 16 : height_to_do;
               vc_rgbswaprgb(mydest, src, width, rows, src_pitch, dest_pitch);
               vclib_memcpy(dest, mydest, rows * dest_pitch);
            }
            vclib_priorityfree(mydest);
         }
      } else {
         // Destination is in internal memory, just call our assembly directly
         vc_rgbswaprgb(dest, src, width, height, src_pitch, dest_pitch);
      }

   } else {
      vcos_assert(!"format not supported.");
   }

}
