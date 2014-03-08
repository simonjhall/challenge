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
#include <assert.h>

/* Project includes */
#include "vc_image.h"
#include "striperesize.h"
#include "vclib/vclib.h"
#include "interface/vcos/vcos_assert.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vc_image_vflip_in_place_yuv420 (VC_IMAGE_T *dest);
extern void vc_image_hflip_in_place_yuv420 (VC_IMAGE_T *dest);
extern void vc_image_text_yuv (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour,
                                  const char *text,int rotate);
extern void vc_image_blt_yuv (VC_IMAGE_T *dst, int x_offset, int y_offset, int width, int height,
                                 VC_IMAGE_T *src, int src_x_offset, int src_y_offset);
extern void vc_image_transpose_yuv420 (VC_IMAGE_T *dst, VC_IMAGE_T *src);
extern void vc_image_transpose_tiles_yuv420 (VC_IMAGE_T *dst, VC_IMAGE_T *src, int fat);
extern void vc_image_fill_yuv420 (VC_IMAGE_T *dst, int x_offset, int y_offset, int width, int height, int value);
extern void vc_image_resize_stripe_h_yuv420 (VC_IMAGE_T *dest, int d_width, int s_width);
extern void vc_image_resize_stripe_v_yuv420 (VC_IMAGE_T *dest, int d_height, int s_height);
extern void hflip_block (unsigned char *start, int width, int height, int pitch);

/* Check extern defs match above and loads #defines and typedefs */
#include "vc_image_yuv.h"

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

extern void yuv420_vflip_in_place_16(unsigned char *start_row, unsigned char *end_row, int pitch, int nbytes);
extern void yuv420_vflip_in_place_8(unsigned char *start_row, unsigned char *end_row, int pitch, int nbytes);
extern void yuv420_hflip_load_block(unsigned char *addr, int vrf_addr, int pitch);
extern void yuv420_hflip_load_next_block(unsigned char *addr, int vrf_addr, int pitch);
extern void yuv420_hflip(int left_vrf_addr, int right_vrf_addr);
extern void yuv420_hflip_save_block(unsigned char *addr, int vrf_addr, int pitch, int nrows);
extern void yuv420_hflip_save_next_block(unsigned char *addr, int vrf_addr, int pitch, int nrows);
extern int yuv420_hflip_reverse(int start, int end);
extern void yuv420_transpose_Y(unsigned char *dest, int dest_pitch, unsigned char *src, int src_pitch, int dst_width, int dst_height);
extern void yuv420_transpose_chunky_Y(unsigned char *dest, int dest_pitch, unsigned char *src, int src_pitch);
extern void yuv420_transpose_chunky_UV(unsigned char *dest, int dest_pitch, unsigned char *src, int src_pitch);
extern void yuv420_transpose_chunky0_Y(unsigned char *dest, int dest_pitch, unsigned char *src, int src_pitch);
extern void yuv420_transpose_chunky0_UV(unsigned char *dest, int dest_pitch, unsigned char *src, int src_pitch);
extern void yuv420_transpose_chunky1_Y(unsigned char *dest, int dest_pitch, unsigned char *src, int src_pitch);
extern void yuv420_transpose_chunky1_UV(unsigned char *dest, int dest_pitch, unsigned char *src, int src_pitch);

/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/


/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/

static void byte_blt (unsigned char *dst, int dst_pitch, unsigned char *src, int src_pitch,
                      int width, int height);
static void byte_set (unsigned char *dst, int dst_pitch, int width, int height, int value);

/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/

extern const short fontpos16[(128-32)];
extern const short fontdata16[32];  /* Fontdata can actually have more entries than specified here */

/******************************************************************************
Global function definitions.
******************************************************************************/

/******************************************************************************
NAME
   vc_image_vflip_in_place_yuv420

SYNOPSIS
   void vc_image_vflip_in_place_yuv420(VC_IMAGE_T *dest)

FUNCTION
   Vertically flip an image.

RETURNS
   void
******************************************************************************/

void vc_image_vflip_in_place_yuv420 (VC_IMAGE_T *dest) {
   int pitch = dest->pitch;
   int height = dest->height;
   unsigned char *start_row = (unsigned char *)dest->image_data;
   unsigned char *end_row = start_row + (height-1)*pitch;
   int nbytes = (dest->width+15)&(~15);

   // Do the Y component. This is always 16-byte aligned.
   yuv420_vflip_in_place_16(start_row, end_row, pitch, nbytes);

   // Now U and V. If the pitch is a multiple of 32 then we can do U and V with
   // the same efficient function. Otherwise we cheat and use a slower version
   // that copes more easily with the alignment problems.
   nbytes >>= 1;
   pitch >>= 1;
   height >>= 1;
   start_row = vc_image_get_u(dest);
   end_row = start_row + (height-1)*pitch;
   if (pitch&15)
      yuv420_vflip_in_place_8(start_row, end_row, pitch, nbytes);
   else
   {
      nbytes = (nbytes + 15) & ~15;
      yuv420_vflip_in_place_16(start_row, end_row, pitch, nbytes);
   }
   start_row = vc_image_get_v(dest);
   end_row = start_row + (height-1)*pitch;
   if (pitch&15)
      yuv420_vflip_in_place_8(start_row, end_row, pitch, nbytes);
   else
      yuv420_vflip_in_place_16(start_row, end_row, pitch, nbytes);
}

/******************************************************************************
NAME
   vc_image_hflip_in_place_yuv420

SYNOPSIS
   void vc_image_hflip_in_place_yuv420(VC_IMAGE_T *dest)

FUNCTION
   Horizontally flip an image.

RETURNS
   void
******************************************************************************/

void vc_image_hflip_in_place_yuv420 (VC_IMAGE_T *dest) {
   int UV_height, UV_width, UV_pitch;
   // The Y component can always be done efficiently.
   hflip_block((unsigned char *)dest->image_data, dest->width, dest->height, dest->pitch);
   UV_height = dest->height>>1;
   UV_width = dest->width>>1;
   UV_pitch = dest->pitch>>1;
   if ((UV_pitch&15) == 0) {
      hflip_block(vc_image_get_u(dest), UV_width, UV_height, UV_pitch);
      hflip_block(vc_image_get_v(dest), UV_width, UV_height, UV_pitch);
   }
   else {
      // More troublesome. To avoid the problem of each row interfering with the
      // next we shall do the odd and then the even rows separately. This is in fact
      // not drastically worse.
      // U.
      hflip_block(vc_image_get_u(dest), UV_width, (UV_height+1)>>1, UV_pitch<<1);
      hflip_block(vc_image_get_u(dest)+UV_pitch, UV_width, UV_height>>1, UV_pitch<<1);
      // V.
      hflip_block(vc_image_get_v(dest), UV_width, (UV_height+1)>>1, UV_pitch<<1);
      hflip_block(vc_image_get_v(dest)+UV_pitch, UV_width, UV_height>>1, UV_pitch<<1);
   }
}

/******************************************************************************
Static function definitions.
******************************************************************************/

void hflip_block (unsigned char *start, int width, int height, int pitch) {
   int h;
   for (h = 0; h < height; h += 16) {
      int nrows = h+16 < height ? 16 : height-h;
      int block_reg;
      unsigned char *block;
      unsigned char *left = start + pitch*h;
      unsigned char *right = left + width-16;

      if (width >= 48) {
         int left_reg = 0, right_reg = 32;
         int save_right = ((int)right&15) != 0;

         yuv420_hflip_load_block(left, left_reg, pitch);
         yuv420_hflip_load_next_block(right, right_reg+(16<<6), pitch);

         // We loop working outwards from the left edge and inwards from the right,
         // scrolling our way across the image swapping 16x16 blocks until we reach
         // the stage that the image blocks we're processing start to overlap.
         for (; right-left >= 32; left += 16, right -= 16) {
            // left points to the start of the left hand block, right to the start
            // of the right hand block.

            yuv420_hflip_load_next_block(left, left_reg, pitch);
            yuv420_hflip_load_block(right, right_reg+(16<<6), pitch);

            yuv420_hflip(left_reg+(((int)left)&15), right_reg+(((int)right)&15));

            yuv420_hflip_save_block(left, left_reg, pitch, nrows);
            if (save_right) // take care not to save memory that might not be "ours"
               yuv420_hflip_save_next_block(right, right_reg+(16<<6), pitch, nrows);
            else
               save_right = 1;

            left_reg = (left_reg+16)&63;
            right_reg = (right_reg-16)&63;
         }

         yuv420_hflip_save_block(left, left_reg, pitch, nrows);
         yuv420_hflip_save_next_block(right, right_reg+(16<<6), pitch, nrows);
      }

      // Now we must reverse the pixels between left and right+16. Processing this
      // middle stripe of the image separately here is less efficient, but it at least
      // gives people of fighting chance of understanding what's going on!
      right += 15;
      for (block = left, block_reg = 0; ((int)block&~15) <= ((int)right&~15); block += 16, block_reg += 16)
         yuv420_hflip_load_block(block, block_reg, pitch);
      // Now reverse from left to right inclusive.
      yuv420_hflip_reverse((int)left&15, (int)right-((int)left&~15));
      // Now write those blocks out again.
      for (block = left, block_reg = 0; ((int)block&~15) <= ((int)right&~15); block += 16, block_reg += 16)
         yuv420_hflip_save_block(block, block_reg, pitch, nrows);
   }
}


/******************************************************************************
NAME
   vc_image_text_yuv

SYNOPSIS
   void vc_image_text_yuv(VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, char *text,int rotate)

FUNCTION
   Write text to an image. Uses a fixed internal font and clips it to the image if
   it overflows. bg_colour may be specified as a colour value, or
   VC_IMAGE_COLOUR_TRANSPARENT to mean the backgroud shows through unchanged all
   around, or VC_IMAGE_COLOUR_FADE to preserve the background, but to fade it slightly.
   Currently does not change the chrominance components

RETURNS
   void
******************************************************************************/

void vc_image_text_yuv (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,int rotate)
{
   unsigned char *buffer = dest->image_data;
   int pitch = dest->pitch; // in pixels
   int start, end;
   int y;
   int sx = x_offset, sy = y_offset;
   int fx = dest->width, fy = dest->height;


   for ( ; text[0] !=0 ; text++) {
      int c=(int)text[0];

      if ((c<32) || (c>=128)) c=33; // Change unknown characters to !
      c-=32;

      // Now compute location of the text in memory
      if (c > 0)
         start = fontpos16[c-1];
      else
         start = 0;

      end = fontpos16[c];

      for ( ; start < end; start++, sx++) {
         int data;
         int end_y;
         if (sx >= fx)
            break;

         data=fontdata16[start];  /* This contains packed binary for the letter */
         /* Needs to be displayed at location sx,sy */
         end_y = sy+16 < fy ? sy+16 : fy;
         for ( y = sy; y < end_y; y++) {
            int pos = 15-(y-sy);

            if (data&(1<<pos)) {
               // one means background
               if (bg_colour == VC_IMAGE_COLOUR_TRANSPARENT) {
                  // do nothing
               }
               else if (bg_colour == VC_IMAGE_COLOUR_FADE) {
                  unsigned char *b = buffer + y*pitch + sx;
                  // wipe out bottom bits and shift down
                  b[0]>>=1;
               }
               else
                  buffer[y*pitch + sx]= bg_colour;
            }
            else {
               buffer[y*pitch + sx]= fg_colour;
            }
         }
      }
   }
}


/******************************************************************************
NAME
   vc_image_blt_yuv

SYNOPSIS
   void vc_image_blt_yuv(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                         VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

FUNCTION
   Copy YUV data around...
   Currently restricted in various ways...

RETURNS
   void
******************************************************************************/

static void byte_blt(unsigned char *dst, int dst_pitch,
                     unsigned char *src, int src_pitch,
                     int width, int height)
{
   if (src_pitch == width && dst_pitch == width) {
      vclib_memcpy(dst, src, width*height);
   } else {
      int i;
      for (i = 0; i < height; i++) {
         vclib_memcpy(dst, src, width);
         dst += dst_pitch;
         src += src_pitch;
      }
   }
}

void vc_image_blt_yuv(VC_IMAGE_T *dst, int x_offset, int y_offset, int width, int height,
                      VC_IMAGE_T *src, int src_x_offset, int src_y_offset)
{
   int src_pitch = src->pitch;
   int dst_pitch = dst->pitch;
   int src_offset = src_y_offset*src_pitch + src_x_offset;
   int dst_offset = y_offset*dst_pitch + x_offset;
   unsigned char *src_ptr;
   unsigned char *dst_ptr;

   vcos_assert(is_valid_vc_image_buf(dst, dst->type));
   vcos_assert(is_valid_vc_image_buf(src, dst->type));

   assert((src_x_offset & 1) == 0 && ((src_y_offset & 1) == 0 || src->type == VC_IMAGE_YUV422PLANAR));
   assert((x_offset & 1) == 0 && ((y_offset & 1) == 0 || dst->type == VC_IMAGE_YUV422PLANAR));

   // if we're copying the whole width of an image, we're better off
   // copying pitch pixels across, it's likely to be more aligned than width
   if (x_offset == 0 && width == dst->width &&
       src_x_offset == 0 && width == src->width)
      width = _min(dst->pitch, src->pitch);

// Y
   src_ptr = vc_image_get_y(src) + src_offset;
   dst_ptr = vc_image_get_y(dst) + dst_offset;

   byte_blt(dst_ptr, dst_pitch, src_ptr, src_pitch, width, height);

// Shrink by halving each dimension
   width  = (width + 1) >> 1;
   src_pitch >>= 1;
   dst_pitch >>= 1;

   if(src->type == VC_IMAGE_YUV420)
   {
      height = (height + 1) >> 1;

      src_offset = (src_y_offset>>1)*src_pitch + (src_x_offset>>1);
      dst_offset = (y_offset>>1)*dst_pitch + (x_offset>>1);
   }
   else
   {
      src_offset = src_y_offset*src_pitch + (src_x_offset>>1);
      dst_offset = y_offset*dst_pitch + (x_offset>>1);
   }

//U
   src_ptr = vc_image_get_u(src) + src_offset;
   dst_ptr = vc_image_get_u(dst) + dst_offset;

   byte_blt(dst_ptr, dst_pitch, src_ptr, src_pitch, width, height);

// V
   src_ptr = vc_image_get_v(src) + src_offset;
   dst_ptr = vc_image_get_v(dst) + dst_offset;

   byte_blt(dst_ptr, dst_pitch, src_ptr, src_pitch, width, height);
}

/******************************************************************************
NAME
   vc_image_transpose_yuv420

SYNOPSIS
   void vc_image_transpose_yuv420(VC_IMAGE_T *dest, VC_IMAGE_T *src);

FUNCTION
   Copying transpose of YUV image.

RETURNS
   void
******************************************************************************/

void vc_image_transpose_yuv420(VC_IMAGE_T *dst, VC_IMAGE_T *src) {
   int src_pitch = src->pitch;
   int dst_pitch = dst->pitch;
   int dst_height = dst->height;
   int dst_width = dst->width;

   unsigned char *src_ptr = vc_image_get_y(src);
   unsigned char *dst_ptr = vc_image_get_y(dst);

   // Y.
   yuv420_transpose_Y(dst_ptr, dst_pitch, src_ptr, src_pitch, dst_width, dst_height);

   // U and V are half width/pitch/height.
   dst_height >>= 1;
   dst_width >>= 1;
   src_pitch >>= 1;
   dst_pitch >>= 1;

   // U.
   src_ptr = vc_image_get_u(src);
   dst_ptr = vc_image_get_u(dst);
   yuv420_transpose_Y(dst_ptr, dst_pitch, src_ptr, src_pitch, dst_width, dst_height);

   // V.
   src_ptr = vc_image_get_v(src);
   dst_ptr = vc_image_get_v(dst);
   yuv420_transpose_Y(dst_ptr, dst_pitch, src_ptr, src_pitch, dst_width, dst_height);
}

/******************************************************************************
NAME
   vc_image_transpose_tiles_yuv420

SYNOPSIS
   void vc_image_transpose_tiles_yuv420(VC_IMAGE_T *dest, VC_IMAGE_T *src, int fat);

FUNCTION
   Transpose a series of 16x16 tiles in-place.  dest may be equal to src.

RETURNS
   void
******************************************************************************/

void vc_image_transpose_tiles_yuv420(VC_IMAGE_T *dst, VC_IMAGE_T *src, int fat)
{
   void (*yfunc)(unsigned char *dest, int dest_pitch, unsigned char *src, int src_pitch) = yuv420_transpose_chunky_Y;
   void (*uvfunc)(unsigned char *dest, int dest_pitch, unsigned char *src, int src_pitch) = yuv420_transpose_chunky_UV;
   unsigned char *src_ptr = vc_image_get_y(src);
   unsigned char *dst_ptr = vc_image_get_y(dst);
   int dst_pitch, dst_height, dst_width, src_pitch = src->pitch;
   int i, chunksize = 256, x_offset;

   if (fat & 2)
   {
      if (fat == 2)
      {
         yfunc = yuv420_transpose_chunky0_Y;
         uvfunc = yuv420_transpose_chunky0_UV;
      }
      else
      {
         yfunc = yuv420_transpose_chunky1_Y;
         uvfunc = yuv420_transpose_chunky1_UV;
      }
      chunksize >>= 1;
      assert(src->width <= 32);
   }
   else
      assert(src->width <= 16);

   dst_pitch = (src_pitch * chunksize) >> 4;
   dst_width = dst_pitch & -chunksize;
   dst_height = (src->height + dst_width - 1) / dst_width << 4;

   assert(dst_ptr != src_ptr || dst_height * dst_pitch <= src->height * src_pitch);

   x_offset = 0;

   for (i = src->height; i > 0; i -= chunksize)
   {
      yfunc(dst_ptr + x_offset, dst_pitch, src_ptr, src_pitch);

      if ((x_offset += chunksize) >= dst_width)
      {
         x_offset = 0;
         dst_ptr += dst_pitch << 4;
      }
      src_ptr += src_pitch * chunksize;
   }

   src_pitch >>= 1;
   dst_pitch >>= 1;
   dst_width >>= 1;
   chunksize >>= 1;

   src_ptr = vc_image_get_u(src);
   dst_ptr = (unsigned char *)dst->image_data + (dst_pitch << 1) * dst_height;
   x_offset = 0;
   for (i = src->height >> 1; i > 0; i -= chunksize)
   {
      uvfunc(dst_ptr + x_offset, dst_pitch, src_ptr, src_pitch);
      if ((x_offset += chunksize) >= dst_width)
      {
         x_offset = 0;
         dst_ptr += dst_pitch * 8;
      }
      src_ptr += src_pitch * chunksize;
   }

   src_ptr = vc_image_get_v(src);
   dst_ptr = (unsigned char *)dst->image_data + (dst_pitch * (5 << 1) >> 2) * dst_height;
   x_offset = 0;
   for (i = src->height >> 1; i > 0; i -= chunksize)
   {
      uvfunc(dst_ptr + x_offset, dst_pitch, src_ptr, src_pitch);
      if ((x_offset += chunksize) >= dst_width)
      {
         x_offset = 0;
         dst_ptr += dst_pitch * 8;
      }
      src_ptr += src_pitch * chunksize;
   }

   dst->width = dst_width << 1;
   dst->pitch = dst_pitch << 1;
   dst->height = dst_height;
   vc_image_set_image_data_yuv (dst, dst->size, dst->image_data, 0, 0);
}

/******************************************************************************
NAME
   vc_image_fill_yuv420

SYNOPSIS
   void vc_image_fill_yuv420(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int value)

FUNCTION
   Fill given region of an YUV420 image with solid colour value.
   value = (Yval<<0) | (Uval<<8) | (Vval<<16)

RETURNS
   void
******************************************************************************/

static void byte_set(unsigned char *dst, int dst_pitch,
                     int width, int height, int value)
{
   if (dst_pitch == width) {
      vclib_memset(dst, value, width*height);
   } else {
      int i;
      for (i = 0; i < height; i++) {
         vclib_memset(dst, value, width);
         dst += dst_pitch;
      }
   }
}

void vc_image_fill_yuv420 (VC_IMAGE_T *dst, int x_offset, int y_offset, int width, int height, int value) {
   int dst_pitch = dst->pitch;
   int dst_offset = y_offset*dst_pitch + x_offset;
   unsigned char *dst_ptr;

   assert((x_offset & 1) == 0 && (y_offset & 1) == 0);
// Y
   dst_ptr = vc_image_get_y(dst) + dst_offset;
   byte_set(dst_ptr, dst_pitch, width, height, value);

// Shrink by halving each dimension
   width  = (width + 1) >> 1;
   height = (height + 1) >> 1;
   dst_pitch >>= 1;
   dst_offset = (y_offset>>1)*dst_pitch + (x_offset>>1);

// U
   dst_ptr = vc_image_get_u(dst) + dst_offset;
   value >>= 8;
   byte_set(dst_ptr, dst_pitch, width, height, value);

// V
   dst_ptr = vc_image_get_v(dst) + dst_offset;
   value >>= 8;
   byte_set(dst_ptr, dst_pitch, width, height, value);
}

void vc_image_fill_yuv422 (VC_IMAGE_T *dst, int x_offset, int y_offset, int width, int height, int value) {
   int dst_pitch = dst->pitch;
   int dst_offset = y_offset*dst_pitch + x_offset;
   unsigned char *dst_ptr;

   assert((x_offset & 1) == 0 && (y_offset & 1) == 0);
// Y
   dst_ptr = vc_image_get_y_422(dst) + dst_offset;
   byte_set(dst_ptr, dst_pitch, width, height, value);

// Shrink by halving each dimension
   width  = (width + 1) >> 1;
   //height = (height + 1) >> 1;
   if(dst->type == VC_IMAGE_YUV422PLANAR)
      dst_pitch >>= 1;
   dst_offset = (y_offset>>0)*dst_pitch + (x_offset>>1);

// U
   dst_ptr = ((dst->type == VC_IMAGE_YUV422PLANAR) ? vc_image_get_u_422planar(dst) : vc_image_get_u_422(dst)) + dst_offset;
   value >>= 8;
   byte_set(dst_ptr, dst_pitch, width, height, value);

// V
   dst_ptr = ((dst->type == VC_IMAGE_YUV422PLANAR) ? vc_image_get_v_422planar(dst) : vc_image_get_v_422(dst)) + dst_offset;
   value >>= 8;
   byte_set(dst_ptr, dst_pitch, width, height, value);
}

/******************************************************************************
NAME
   vc_image_resize_stripe_h_yuv420

SYNOPSIS
   void vc_image_resize_stripe_h_yuv420(VC_IMAGE_T *dest, int d_width, int s_width)

FUNCTION
   Resize a YUV stripe horizontally.
******************************************************************************/

void vc_image_resize_stripe_h_yuv420(VC_IMAGE_T *dest, int d_width, int s_width)
{
   int step;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_YUV420));
   assert(d_width > 0 && s_width > 0);
   assert(dest->height == 16);
   assert((dest->pitch & 31) == 0);
   assert(dest->width >= s_width);
   assert(dest->width >= d_width);

   if (d_width <= 0)
      return;

   /* When reducing the image an even destination size is necessary to stop the
    * subsampled UV from trying to read more data than exists.  The consequence
    * of this workaround is that some of the left/bottom edge of the image is
    * cut off.  Alternatives are to assert() that the sizes must be even, or to
    * spend the extra processing time duplicating the final column of U and V
    * data so that the source image appears complete when rounded up.
    */
   if (d_width < s_width && (d_width & 1) != 0)
      d_width++;

   while (d_width << 2 < s_width)
   {
      s_width >>= 2;
      vc_stripe_h_reduce_bytes2(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, s_width, 0x40000);
      vc_stripe_h_reduce_bytes2(vc_image_get_u(dest), dest->pitch >> 1, vc_image_get_u(dest), dest->pitch >> 1, (s_width + 1) >> 1, 0x40000);
   }

   step = ((s_width << 16) + (d_width >> 1)) / d_width;

   if (step != 0x10000)
   {
      switch (_msb((step - 1) | 0x1000))
      {
      case 17: /* 2.0 < step <= 4.0 */
         vc_stripe_h_reduce_bytes2(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, d_width, step);
         vc_stripe_h_reduce_bytes2(vc_image_get_u(dest), dest->pitch >> 1, vc_image_get_u(dest), dest->pitch >> 1, (d_width + 1) >> 1, step);
         break;
      case 16: /* 1.0 < step <= 2.0 */
         vc_stripe_h_reduce_bytes(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, d_width, step);
         vc_stripe_h_reduce_bytes(vc_image_get_u(dest), dest->pitch >> 1, vc_image_get_u(dest), dest->pitch >> 1, (d_width + 1) >> 1, step);
         break;
      case 12: /* 0.0625 < step <= 0.125 */
      case 13: /* 0.125 < step <= 0.25 */
      case 14: /* 0.25 < step <= 0.5 */
      case 15: /* 0.5 < step <= 1.0 */
#if 1
         {
            int off;

            off = (dest->pitch - (s_width+1&~1)) & ~31;

            vclib_memmove(vc_image_get_y(dest) + off, vc_image_get_y(dest), dest->pitch * 16 - off);
            vclib_memmove(vc_image_get_u(dest) + (off >> 1), vc_image_get_u(dest), dest->pitch * 8 - (off >> 1));

            vc_stripe_h_expand_bytes(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest) + off, dest->pitch, d_width, step, s_width);
            vc_stripe_h_expand_bytes(vc_image_get_u(dest), dest->pitch >> 1, vc_image_get_u(dest) + (off >> 1), dest->pitch >> 1, (d_width + 1) >> 1, step, (s_width + 1) >> 1);
         }
#else
         vc_stripe_h_expand_bytes(vc_image_get_y(dest) + d_width, dest->pitch, vc_image_get_y(dest) + s_width, dest->pitch, d_width, step);
         d_width = (d_width + 1) >> 1;
         s_width = (s_width + 1) >> 1;
         vc_stripe_h_expand_bytes(vc_image_get_u(dest) + d_width, dest->pitch >> 1, vc_image_get_u(dest) + s_width, dest->pitch >> 1, d_width, step);
#endif
         break;

      default:
         assert(0); /* can't cope with scale factor */
      }
   }
}


/******************************************************************************
NAME
   vc_image_resize_stripe_v_yuv420

SYNOPSIS
   void vc_image_resize_stripe_v_yuv420(VC_IMAGE_T *dest, int d_height, int s_height)

FUNCTION
   Resize a YUV stripe vertically.
******************************************************************************/

void vc_image_resize_stripe_v_yuv420(VC_IMAGE_T *dest, int d_height, int s_height)
{
   int step;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_YUV420));
   assert(d_height > 0 && s_height > 0);
   assert(dest->width == 16);
//   assert((dest->pitch & 31) == 0);
   assert(dest->height >= s_height);
   assert(dest->height >= d_height);

   if (d_height <= 0)
      return;

   /* When reducing the image an even destination size is necessary to stop the
    * subsampled UV from trying to read more data than exists.  The consequence
    * of this workaround is that some of the left/bottom edge of the image is
    * cut off.  Alternatives are to assert() that the sizes must be even, or to
    * spend the extra processing time duplicating the final column of U and V
    * data so that the source image appears complete when rounded up.
    */
   if (d_height < s_height && (d_height & 1) != 0)
      d_height++;

   while (d_height << 2 < s_height)
   {
      s_height >>= 2;
      vc_stripe_v_reduce_bytes2(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, s_height, 0x40000);
      vc_stripe_v_reduce_uv2(vc_image_get_u(dest), vc_image_get_v(dest), dest->pitch >> 1, vc_image_get_u(dest), vc_image_get_v(dest), dest->pitch >> 1, (s_height + 1) >> 1, 0x40000);
   }

   step = ((s_height << 16) + (d_height >> 1)) / d_height;

   if (step != 0x10000)
   {
      switch (_msb((step - 1) | 0x1000))
      {
      case 17: /* 2.0 < step <= 4.0 */
         vc_stripe_v_reduce_bytes2(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, d_height, step);
         vc_stripe_v_reduce_uv2(vc_image_get_u(dest), vc_image_get_v(dest), dest->pitch >> 1, vc_image_get_u(dest), vc_image_get_v(dest), dest->pitch >> 1, (d_height + 1) >> 1, step);
         break;
      case 16: /* 1.0 < step <= 2.0 */
         vc_stripe_v_reduce_bytes(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, d_height, step);
         vc_stripe_v_reduce_uv(vc_image_get_u(dest), vc_image_get_v(dest), dest->pitch >> 1, vc_image_get_u(dest), vc_image_get_v(dest), dest->pitch >> 1, (d_height + 1) >> 1, step);
         break;
      case 12: /* 0.0625 < step <= 0.125 */
      case 13: /* 0.125 < step <= 0.25 */
      case 14: /* 0.25 < step <= 0.5 */
      case 15: /* 0.5 < step <= 1.0 */
#if 1
         {
            int off;

            off = ((dest->height + 15 & ~15) - (s_height+1&~1) - 2) * dest->pitch;

            vclib_memmove(vc_image_get_y(dest) + off, vc_image_get_y(dest), dest->pitch * s_height);
            vclib_memmove(vc_image_get_u(dest) + (off >> 2), vc_image_get_u(dest), (dest->pitch >> 1) * (s_height+1 >> 1));
            vclib_memmove(vc_image_get_v(dest) + (off >> 2), vc_image_get_v(dest), (dest->pitch >> 1) * (s_height+1 >> 1));

            vc_stripe_v_expand_bytes(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest) + off, dest->pitch, d_height, step, s_height);
            vc_stripe_v_expand_uv(vc_image_get_u(dest), vc_image_get_v(dest), dest->pitch >> 1, vc_image_get_u(dest) + (off >> 2), vc_image_get_v(dest) + (off >> 2), dest->pitch >> 1, (d_height + 1) >> 1, step, (s_height + 1) >> 1);
         }
#else
         vc_stripe_v_expand_bytes(vc_image_get_y(dest) + dest->pitch * d_height, dest->pitch, vc_image_get_y(dest) + dest->pitch * s_height, dest->pitch, d_height, step);
         vc_stripe_v_increase_uv(vc_image_get_u(dest) + dest->pitch * ((d_height + 1) >> 1), dest->pitch >> 1, vc_image_get_u(dest) + dest->pitch * ((s_height + 1) >> 1), dest->pitch >> 1, (d_height + 1) >> 1, step);
#endif
         break;
      default:
         assert(0); /* can't cope with scale factor */
      }
   }
}

