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
#include <string.h>

/* Project includes */
#include "vc_image.h"
#include "vcinclude/vcore.h"           // #include <vc/intrinsics.h> - for _brev
#include "interface/vcos/vcos_assert.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vc_image_endian_translate (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image);
extern void vc_image_transpose_1bpp (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image);
extern void vc_image_vflip_1bpp (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image);
extern void vc_image_hflip_1bpp (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image);
extern void vc_image_vflip_in_place_1bpp (VC_IMAGE_T *image);
extern void vc_image_hflip_in_place_1bpp (VC_IMAGE_T *image);
extern void vc_image_copy_1bpp_src_offset (VC_IMAGE_T *dst, int w, int h, VC_IMAGE_T *src, int x_src, int y_src);
extern void vc_image_copy_1bpp_dst_offset (VC_IMAGE_T *dst, int x_dst, int y_dst, VC_IMAGE_T *src, int w, int h);
extern void vc_image_fill_1bpp (VC_IMAGE_T *dst, int x, int y, int w, int h, int value);

/* Check extern defs match above and loads #defines and typedefs */
#include "vc_image_1bpp.h"

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

extern void vc_image_endian_translate_1bpp (unsigned char *dest, unsigned char *src, int src_pitch_bytes, int dest_pitch_bytes);
extern void vc_1bpp_transpose (unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int block_height);
extern void vc_1bpp_vflip (unsigned short *dest, unsigned short *src, int src_pitch_bytes, int height, int nblocks, int dest_pitch_bytes);
extern void vc_1bpp_hflip (unsigned short *dest, unsigned short *src, int src_pitch_bytes, int ncols, int offset, int dest_pitch_bytes);

/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/

// n bits from the LSB/MSB; mask_msb(n) only works for n > 0
#define mask_lsb(n) ((1 << (n)) - 1)
#define mask_msb(n) (~mask_lsb(32 - (n)))

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
   vc_image_endian_translate

SYNOPSIS
   void vc_image_endian_translate(VC_IMAGE_T *dest, VC_IMAGE_T *src)

FUNCTION
   Convert from big to little endian format. src and dest may be the same.

RETURNS
   void
******************************************************************************/

void vc_image_endian_translate (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image) {
   int src_pitch_bytes, src_pitch_bytes_16;
   int dest_pitch_bytes, dest_pitch_bytes_16;
   unsigned char *dest, *src;
   int i;
   vcos_assert(is_valid_vc_image_buf(dest_image, VC_IMAGE_1BPP));
   vcos_assert(is_valid_vc_image_buf(src_image, VC_IMAGE_1BPP));
   dest_image->pitch = src_image->pitch;
   dest_image->width = src_image->width;
   dest_image->height = src_image->height;
   vcos_assert(dest_image->size >= vc_image_required_size(dest_image));

   src_pitch_bytes = src_image->pitch;
   src_pitch_bytes_16 = src_pitch_bytes<<4;
   dest_pitch_bytes = dest_image->pitch;
   dest_pitch_bytes_16 = dest_pitch_bytes<<4;
   dest = (unsigned char *)dest_image->image_data;
   src = (unsigned char *)src_image->image_data;

   for (i = 0; i < src_image->height; i += 16, dest += dest_pitch_bytes_16, src += src_pitch_bytes_16)
      vc_image_endian_translate_1bpp(dest, src, src_pitch_bytes, dest_pitch_bytes);
}

/******************************************************************************
NAME
   vc_image_transpose_1bpp

SYNOPSIS
   void vc_image_transpose_1bpp(VC_IMAGE_T *dest, VC_IMAGE_T *src)

FUNCTION
   Transpose.

RETURNS
   void
******************************************************************************/

void vc_image_transpose_1bpp (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image) {
   int block_height = (src_image->height+15)>>4;
   int src_pitch_bytes = src_image->pitch;
   int dest_pitch_bytes = dest_image->pitch;
   int src_width_shorts = (src_image->width+15)>>4;
   unsigned short *dest = (unsigned short *)dest_image->image_data;
   unsigned short *src = (unsigned short *)src_image->image_data;
   int i;

   for (i = 0; i < src_width_shorts; i++, src++, dest += (dest_pitch_bytes<<3)) {
      // We process a 16-*bit* wide column of src, on each call, turning it into a
      // 16-deep stripe in the destination.
      vc_1bpp_transpose(dest, src, dest_pitch_bytes, src_pitch_bytes, block_height);
   }
}

/******************************************************************************
NAME
   vc_image_vflip_1bpp

SYNOPSIS
   void vc_image_vflip_1bpp(VC_IMAGE_T *dest, VC_IMAGE_T *src)

FUNCTION
   Vertical flip ("upside down").

RETURNS
   void
******************************************************************************/

void vc_image_vflip_1bpp (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image) {
   int src_pitch_bytes = src_image->pitch;
   int dest_pitch_bytes = dest_image->pitch;
   int height = src_image->height;
   int nblocks = (src_image->width+255)>>8;
   unsigned short *dest = (unsigned short *)dest_image->image_data;
   unsigned short *src = (unsigned short *)src_image->image_data + (height-1) * (src_pitch_bytes>>1);

   vc_1bpp_vflip(dest, src, src_pitch_bytes, height, nblocks, dest_pitch_bytes);
}

/******************************************************************************
NAME
   vc_image_hflip_1bpp

SYNOPSIS
   void vc_image_hflip_1bpp(VC_IMAGE_T *dest, VC_IMAGE_T *src)

FUNCTION
   Horizontal flip ("mirror").

RETURNS
   void
******************************************************************************/

void vc_image_hflip_1bpp (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image) {
   int height = src_image->height;
   int src_pitch_bytes = src_image->pitch;
   int src_pitch_shorts_16 = src_pitch_bytes<<3;
   int dest_pitch_bytes = dest_image->pitch;
   int dest_pitch_shorts_16 = dest_pitch_bytes<<3;
   int offset = src_image->width&15;
   int ncols = (src_image->width+15)>>4;
   unsigned short *src = (unsigned short *)src_image->image_data + (src_image->width>>4);
   unsigned short *dest = (unsigned short *)dest_image->image_data;
   int i;

   for (i = 0; i < height; i += 16, dest += dest_pitch_shorts_16, src += src_pitch_shorts_16) {
      vc_1bpp_hflip(dest, src, src_pitch_bytes, ncols, offset, dest_pitch_bytes);
   }
}

/******************************************************************************
NAME
   vc_image_vflip_in_place_1bpp

SYNOPSIS
   void vc_image_vflip_in_place_1bpp(VC_IMAGE_T *image)

FUNCTION
   Vertical flip in place ("upside down").

RETURNS
   void
******************************************************************************/

void vc_image_vflip_in_place_1bpp (VC_IMAGE_T *image) {
   int block_pitch = image->pitch >> 1;
   int height = image->height;
   int nwords = (image->width + 15)>>4;
   unsigned short *img_top = (unsigned short *)image->image_data;
   unsigned short *img_bottom = (unsigned short *)img_top + (height-1) * block_pitch;
   int lines = height>>1;
   int line;

//XXX: Very slow...
   for (line = 0; line < lines; line++) {
      int block;
      for (block = 0; block < nwords; block++) {
         unsigned short tmp = img_top[block];
         img_top[block] = img_bottom[block];
         img_bottom[block] = tmp;
      }
      img_top += block_pitch;
      img_bottom -= block_pitch;
   }
}

/******************************************************************************
NAME
   vc_image_hflip_in_place_1bpp

SYNOPSIS
   void vc_image_hflip_in_place_1bpp(VC_IMAGE_T *image)

FUNCTION
   Horizontal flip in place ("mirror").

RETURNS
   void
******************************************************************************/

void vc_image_hflip_in_place_1bpp (VC_IMAGE_T *image) {
   int block_pitch = image->pitch >> 1;
   int nwords = (image->width + 15) >> 4;
   int nwords2 = (nwords + 1) >> 1;
   unsigned short *img = (unsigned short *)image->image_data;
   int lines = image->height;
   int shift = 16 - (image->width & 15);
   int line;

//XXX: Very very slow...
   for (line = 0; line < lines; line++) {
      int block;

// First, shift the pixels right by the appropriate amount
      if (shift != 16) {
         unsigned int prev = 0;
         for (block = 0; block < nwords; block++) {
            unsigned int pix = img[block];
            pix = (pix << shift) | prev;
            img[block] = pix;
            prev = pix >> 16;
         }
      }

// Then swap the words, bit-reversing on the way...
      for (block = 0; block < nwords2; block++) {
         int b2 = nwords - block - 1;

         int tmp1 = img[block];
         int tmp2 = img[b2];
         tmp2 = _brev(tmp2);
         img[block] = tmp2 >> 16;
         tmp1 = _brev(tmp1);
         img[b2] = tmp1 >> 16;
      }

      img += block_pitch;
   }

}

/******************************************************************************
NAME
   vc_image_copy_1bpp_src_offset

SYNOPSIS
   void vc_image_copy_1bpp_src_offset(VC_IMAGE_T *dst, int w, int h, VC_IMAGE_T *src, int x_src, int y_src);

FUNCTION
   Blt a rectangle of pixels at the specified (x, y) offset to a destination image.

RETURNS
   void
******************************************************************************/

void vc_image_copy_1bpp_src_offset(VC_IMAGE_T *dst, int w, int h, VC_IMAGE_T *src, int x_src, int y_src)/*{{{*/
{
   unsigned short *s, *d;
   int s_pitch = src->pitch >> 1;
   int d_pitch = dst->pitch >> 1;

   int offset, blocks, remain;
   int y,n;

   s = (unsigned short *)src->image_data + (y_src * s_pitch) + (x_src >> 4);
   d = (unsigned short *)dst->image_data;

   offset = x_src & 0x0f;
   blocks = w >> 4;
   remain = w & 0x0f;

   // TODO: not terribly fast; potential for the PPUs to do 16 rows at a time
   for (y = 0; y < h; y++, s += s_pitch, d += d_pitch)
   {
      unsigned short prev = s[0] >> offset;
      for (n = 0; n < blocks; n++)
      {
         d[n] = prev | ((s[n+1] << (16 - offset)) & 0xffff);
         prev = s[n+1] >> offset;
      }

      d[blocks] = ( (prev | (s[blocks + 1] << (16 - offset))) & mask_lsb(remain) )
                  | ( d[blocks] & ~mask_lsb(remain) );
   }
}/*}}}*/

/******************************************************************************
NAME
   vc_image_copy_1bpp_dst_offset

SYNOPSIS
   void vc_image_copy_1bpp_dst_offset(VC_IMAGE_T *dst, int x_dst, int y_dst, VC_IMAGE_T *src, int w, int h);

FUNCTION
   Blt the (0,0)-(w,h) rectangle of src into dst at (x_dst,y_dst).

RETURNS
   void
******************************************************************************/

void vc_image_copy_1bpp_dst_offset(VC_IMAGE_T *dst, int x_dst, int y_dst, VC_IMAGE_T *src, int w, int h)/*{{{*/
{
   unsigned short *s, *d;
   int s_pitch = src->pitch >> 1;
   int d_pitch = dst->pitch >> 1;

   int offset, blocks, remain;
   int y,n;

   s = (unsigned short *)src->image_data;
   d = (unsigned short *)dst->image_data + (y_dst * d_pitch) + (x_dst >> 4);

   offset = x_dst & 0x0f;
   blocks = w >> 4;
   remain = w & 0x0f;

   // TODO: not terribly fast; potential for the PPUs to do 16 rows at a time
   for (y = 0; y < h; y++, s += s_pitch, d += d_pitch)
   {
      unsigned int tail;
      unsigned short prev = d[0] & mask_lsb(offset);
      for (n = 0; n < blocks; n++)
      {
         d[n] = prev | ((s[n] << offset) & 0xffff);
         prev = s[n] >> (16 - offset);
      }

      tail = prev | ((s[blocks] & mask_lsb(remain)) << offset)
             | ( (d[blocks] | (d[blocks + 1] << 16)) & mask_msb(32 - offset - remain) );
      d[blocks] = tail & 0xffff;
      d[blocks + 1] = tail >> 16;
   }
}/*}}}*/

/******************************************************************************
NAME
   vc_image_fill_1bpp

SYNOPSIS
   void vc_image_fill_1bpp(VC_IMAGE_T *dst, int x, int y, int w, int h, int value);

FUNCTION
   Fill given region of image with solid colour value.

RETURNS
   void
******************************************************************************/

void vc_image_fill_1bpp(VC_IMAGE_T *dst, int x, int y, int w, int h, int value)/*{{{*/
{
   unsigned short *d;
   int d_pitch = dst->pitch >> 1;
   int offset, blocks, remain;

   d = (unsigned short *)dst->image_data + (y * d_pitch) + (x >> 4);


   offset = x & 0x0f;            // find the number of bits falling into the first word
   remain = (x + w) & 0x0f;      // find out the remaining bits in the last word
   blocks = (w-16+offset-remain) >> 4;

   if (value) // fill
   {
      int y;
      if ( blocks > 0 )
      {
         int offset_mask = ~mask_lsb(offset);
         int remain_mask =  mask_lsb(remain);
         for (y = 0; y < h; y++, d += d_pitch)
         {
            d[0] |= offset_mask;
            memset(d + 1, 0xff, blocks * 2);
            d[blocks + 1] |= remain_mask;
         }
      }
      else
      {
         int mask = ~mask_lsb(offset) & mask_lsb(remain);
         for (y = 0; y < h; y++, d+= d_pitch)
         {
            *d |= mask;
         }
      }
   }
   else // clear
   {
      int y;
      if ( blocks > 0 )
      {
         int offset_mask =  mask_lsb(offset);
         int remain_mask = ~mask_lsb(remain);
         for (y = 0; y < h; y++, d += d_pitch)
         {
            d[0] &= offset_mask;
            memset(d + 1, 0x00, blocks * 2);
            d[blocks + 1] &= remain_mask;
         }
      }
      else
      {
         int mask = ~(~mask_lsb(offset) & mask_lsb(remain));
         for (y = 0; y < h; y++, d += d_pitch)
         {
            *d &= mask;
         }
      }
   }
}/*}}}*/

