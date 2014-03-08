/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/* Project includes */
#include "vc_image.h"
#include "interface/vcos/vcos_assert.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vc_image_transpose_4bpp (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image);
extern void vc_image_vflip_in_place_4bpp (VC_IMAGE_T *image);
extern void vc_image_hflip_in_place_4bpp (VC_IMAGE_T *image);
extern void vc_image_fill_4bpp (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int value);
extern void vc_image_blt_4bpp (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                  VC_IMAGE_T *src, int src_x_offset, int src_y_offset);
extern void vc_image_4bpp_to_rgb565 (VC_IMAGE_T *dest, int width, int height,
                                        VC_IMAGE_T *src, unsigned short palette[16], int src_x_offset, int src_y_offset);
extern void vc_image_4bpp_to_rgb888 (VC_IMAGE_T *dest, int width, int height,
                                        VC_IMAGE_T *src, unsigned short palette[16], int src_x_offset, int src_y_offset);

/* Check extern defs match above and loads #defines and typedefs */
#include "vc_image_4bpp.h"

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

extern void vc_4bpp_transpose(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int block_height);
extern void vc_4bpp_vflip(unsigned short *dest, unsigned short *src, int src_pitch_bytes, int height, int nblocks, int dest_pitch_bytes);
extern void vc_4bpp_hflip(unsigned short *dest, unsigned short *src, int src_pitch_bytes, int ncols, int offset, int dest_pitch_bytes);
extern void block_4bpp_to_rgb565(unsigned char *dest, unsigned char *src, unsigned short *palette, int dest_pitch, int src_pitch);
extern void block_4bpp_to_rgb888(unsigned char *dest, unsigned char *src, unsigned short *palette, int dest_pitch, int src_pitch);

/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/


/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/


/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/


/******************************************************************************
Global function definitions.
******************************************************************************/

/******************************************************************************
NAME
   vc_image_transpose_4bpp

SYNOPSIS
   void vc_image_transpose_4bpp(VC_IMAGE_T *dest, VC_IMAGE_T *src)

FUNCTION
   Transpose.

RETURNS
   void
******************************************************************************/

void vc_image_transpose_4bpp (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image) {
   int src_pitch_bytes = src_image->pitch;
   int dest_pitch_bytes = dest_image->pitch;
   unsigned char *dest = (unsigned char *)dest_image->image_data;
   unsigned char *src = (unsigned char *)src_image->image_data;
   int src_width_bytes = (src_image->width+1)>>1;
   int x, y;

   //XXX: slow...
   for (y = 0; y < src_image->height; y+=2) {
      unsigned char *dest1 = dest;
      unsigned char *src1 = src;
      for (x = 0; x < src_width_bytes; x++) {
         *dest1 = (*src1&0xf0)|(*(src1+src_pitch_bytes)>>4);
         *(dest1+dest_pitch_bytes) = (*(src1+src_pitch_bytes)<<4)|(*src1&0x0f);
         dest1 += dest_pitch_bytes<<1;
         src1++;
      }
      dest++;
      src += src_pitch_bytes<<1;
   }
}

/******************************************************************************
NAME
   vc_image_vflip_in_place_4bpp

SYNOPSIS
   void vc_image_vflip_in_place_1bpp(VC_IMAGE_T *image)

FUNCTION
   Vertical flip in place ("upside down").

RETURNS
   void
******************************************************************************/

void vc_image_vflip_in_place_4bpp (VC_IMAGE_T *image) {
   int block_pitch = image->pitch >> 2;
   int height = image->height;
   int nwords = (image->width + 7) >> 3;
   unsigned long *img_top = (unsigned long *)image->image_data;
   unsigned long *img_bottom = (unsigned long *)img_top + (height-1) * block_pitch;
   int lines = height>>1;
   int line;

   //XXX: relatively slow...
   for (line = 0; line < lines; line++) {
      int block;
      for (block = 0; block < nwords; block++) {
         unsigned long tmp = img_top[block];
         img_top[block] = img_bottom[block];
         img_bottom[block] = tmp;
      }
      img_top += block_pitch;
      img_bottom -= block_pitch;
   }
}

/******************************************************************************
NAME
   vc_image_hflip_in_place_4bpp

SYNOPSIS
   void vc_image_hflip_in_place_4bpp(VC_IMAGE_T *image)

FUNCTION
   Horizontal flip in place ("mirror").

RETURNS
   void
******************************************************************************/

void vc_image_hflip_in_place_4bpp (VC_IMAGE_T *image) {
   int nbytes = image->width>>1;
   unsigned char *ptr = (unsigned char *)image->image_data;
   int line;
   //XXX also relatively slow...
   if ((image->width&1) == 0) {
      // Easier aligned case.
      for (line = 0; line < image->height; line++) {
         unsigned char *start = ptr;
         unsigned char *end = ptr + nbytes - 1;
         while (end > start) {
            unsigned char tmps = *start;
            *(start++) = (*end>>4u)|(*end<<4u);
            *(end--) = (tmps>>4u)|(tmps<<4u);
         }
         if (end == start)
            *start = (*start>>4u)|(*start<<4u);
         ptr += image->pitch;
      }
   }
   else {
      unsigned char prev;
      for (line = 0; line < image->height; line++) {
         unsigned char *start = ptr;
         unsigned char *end = ptr + nbytes - 1;
         prev = 0;
         while (end >= start) {
            unsigned char tmp = *start;
            prev |= (tmp&0xf0);
            *(start++) = (*(end+1)&0xf0)|(*end&0x0f);
            *(end+1) = prev;
            prev = tmp&0x0f;
            end--;
         }
         if (end+1 == start)
            *start = (*start&0xf0)|prev;
         ptr += image->pitch;
      }
   }
}

/******************************************************************************
NAME
   vc_image_fill_4bpp

SYNOPSIS
   void vc_image_fill_4bpp(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int value)

FUNCTION
   Fill given region of image with solid colour value.

RETURNS
   void
******************************************************************************/

void vc_image_fill_4bpp (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int value) {
   int y;
   unsigned char *ptr = (unsigned char *)dest->image_data + y_offset*dest->pitch;
   int value_2bytes;
   int value16;
   value &= 15;
   value16 = value<<4;
   value_2bytes = value16|value;
   if (x_offset&1)
      width--;  // accounts for initial unaligned pixel

   for (y = 0; y < height; y++) {
      unsigned char *ptr1 = ptr+(x_offset>>1);
      int nleft = width;
      // First do any unaligned first value.
      if (x_offset&1)
         *(ptr1++) = ((*ptr1)&0xf0)|value;
      // Now do whole bytes. Could easily extend this to work in 32-bit words without resorting to vector code.
      for (; nleft > 1; nleft -= 2)
         *(ptr1++) = value_2bytes;
      // Finish with any trailing pixel.
      if (nleft)
         *ptr1 = ((*ptr1)&0x0f)|value16;
      ptr += dest->pitch;
   }
}

/******************************************************************************
NAME
   vc_image_blt_4bpp

SYNOPSIS
   int vc_image_blt_4bpp(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                         VC_IMAGE_T *src, int src_x_offset, int src_y_offset)

FUNCTION
   Blt from src bitmap to destination.

RETURNS
   void
******************************************************************************/

void vc_image_blt_4bpp (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                        VC_IMAGE_T *src, int src_x_offset, int src_y_offset) {
   unsigned char *dest_ptr = (unsigned char *)dest->image_data + y_offset*dest->pitch;
   unsigned char *src_ptr = (unsigned char *)src->image_data + src_y_offset*src->pitch;
   int y;
   if (x_offset&1)
      width--;  // accounts for initial unaligned pixel
   if ((x_offset&1) == (src_x_offset&1)) {
      // Alignment the same.
      for (y = 0; y < height; y++) {
         unsigned char *dest_ptr1 = dest_ptr + (x_offset>>1);
         unsigned char *src_ptr1 = src_ptr + (src_x_offset>>1);
         int nleft = width;
         // Take care of an initially unaligned pixel.
         if (x_offset&1)
            *(dest_ptr1++) = ((*dest_ptr1)&0xf0)|((*src_ptr1++)&0x0f);
         // Now do whole bytes. Could be extended easily enough for 32-bit words without resorting to vector code.
         for (; nleft > 1; nleft -= 2)
            *(dest_ptr1++) = *(src_ptr1++);
         // Finish any trailing pixel.
         if (nleft)
            *dest_ptr1 = ((*dest_ptr1&0x0f)|((*src_ptr1)&0xf0));
         dest_ptr += dest->pitch;
         src_ptr += src->pitch;
      }
   }
   else {
      // Alignment different.
      for (y = 0; y < height; y++) {
         unsigned char *dest_ptr1 = dest_ptr + (x_offset>>1);
         unsigned char *src_ptr1 = src_ptr + (src_x_offset>>1);
         int nleft = width;
         // Take care of initial unaligned dest pixel.
         if (x_offset&1)
            *(dest_ptr1++) = ((*dest_ptr1)&0xf0)|((*src_ptr1)>>4u);
         src_ptr1++;
         for (; nleft > 1; nleft -= 2)
            (*dest_ptr1++) = ((*(src_ptr1-1))<<4u)|((*src_ptr1++)>>4u);
         // And any trailing pixel.
         if (nleft)
            *dest_ptr1 = ((*(src_ptr1-1))<<4u)|((*dest_ptr1)&0x0f);
         dest_ptr += dest->pitch;
         src_ptr += src->pitch;
      }
   }
}

/******************************************************************************
NAME
   vc_image_4bpp_to_rgb565

SYNOPSIS
   void vc_image_4bpp_to_rgb565(VC_IMAGE_T *dest, int width, int height,
                                VC_IMAGE_T *src, int16_t palette[16], int src_x_offset, int src_y_offset)

FUNCTION
   Convert 4bpp to rgb565. Converts aligned blocks only.

RETURNS
   void
******************************************************************************/

void vc_image_4bpp_to_rgb565(VC_IMAGE_T *dest, int width, int height,
                             VC_IMAGE_T *src, unsigned short palette[16], int src_x_offset, int src_y_offset) {
   unsigned char *dest_addr = (unsigned char *)dest->image_data;
   unsigned char *src_addr = (unsigned char *)src->image_data + src_y_offset*src->pitch + (src_x_offset>>1);
   int h, x;
   vcos_assert((src_x_offset&15) == 0);

   for (h = 0; h < height; h += 16) {
      unsigned char *dest_addr1 = dest_addr;
      unsigned char *src_addr1 = src_addr;
      for (x = 0; x < width; x += 16) {
         block_4bpp_to_rgb565(dest_addr1, src_addr1, palette, dest->pitch, src->pitch);
         dest_addr1 += 32;  // bytes
         src_addr1 += 8;
      }
      dest_addr += dest->pitch<<4;
      src_addr += src->pitch<<4;
   }
}

/******************************************************************************
NAME
   vc_image_4bpp_to_rgb888

SYNOPSIS
   void vc_image_4bpp_to_rgb565(VC_IMAGE_T *dest, int width, int height,
                                VC_IMAGE_T *src, int16_t palette[16], int src_x_offset, int src_y_offset)

FUNCTION
   Convert 4bpp to rgb565. Converts aligned blocks only.

RETURNS
   void
******************************************************************************/

void vc_image_4bpp_to_rgb888(VC_IMAGE_T *dest, int width, int height,
                             VC_IMAGE_T *src, unsigned short palette[16], int src_x_offset, int src_y_offset) {
   unsigned char *dest_addr = (unsigned char *)dest->image_data;
   unsigned char *src_addr = (unsigned char *)src->image_data + src_y_offset*src->pitch + (src_x_offset>>1);
   int h, x;
   vcos_assert((src_x_offset&15) == 0);

   for (h = 0; h < height; h += 16) {
      unsigned char *dest_addr1 = dest_addr;
      unsigned char *src_addr1 = src_addr;
      for (x = 0; x < width; x += 16) {
         block_4bpp_to_rgb888(dest_addr1, src_addr1, palette, dest->pitch, src->pitch);
         dest_addr1 += 48;  // bytes
         src_addr1 += 8;
      }
      dest_addr += dest->pitch<<4;
      src_addr += src->pitch<<4;
   }
}
