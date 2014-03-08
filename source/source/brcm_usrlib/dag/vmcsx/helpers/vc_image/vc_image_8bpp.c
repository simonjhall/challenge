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
#include "vcinclude/vcore.h"
#include "vc_image.h"
#include "striperesize.h"
#include "vclib/vclib.h"
#include "interface/vcos/vcos_assert.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vc_image_vflip_in_place_8bpp (VC_IMAGE_T *dest);
extern void vc_image_hflip_in_place_8bpp (VC_IMAGE_T *dest);
extern void vc_image_transpose_8bpp (VC_IMAGE_T *dst, VC_IMAGE_T *src );
extern void vc_image_transpose_tiles_8bpp (VC_IMAGE_T *dst, VC_IMAGE_T *src, int fat);
extern void vc_image_fill_8bpp (VC_IMAGE_T *dst, int x_offset, int y_offset, int width, int height, int value);
extern void vc_image_small_text_8bpp (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                         int *max_x, int *max_y);
extern void vc_image_resize_stripe_h_8bpp (VC_IMAGE_T *dest, int d_width, int s_width);
extern void vc_image_transparent_alpha_blt_8bpp_from_pal8 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int transparent_colour,int alpha);
extern void vc_image_resize_stripe_v_8bpp (VC_IMAGE_T *dest, int d_height, int s_height);

/* Check extern defs match above and loads #defines and typedefs */
#include "vc_image_8bpp.h"

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
extern void hflip_block(unsigned char *start, int width, int height, int pitch);

/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/


/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/

static void byte_set (unsigned char *dst, int dst_pitch, int width, int height, int value);
static void byte_blt (unsigned char *dst, int dst_pitch, unsigned char *src, int src_pitch,
                      int width, int height);

/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/

extern const short smallfontpos[(128-32)];
extern const short smallfontdata[32];  /* Fontdata can actually have more entries than specified here */

/******************************************************************************
Global function definitions.
******************************************************************************/

/******************************************************************************
NAME
   vc_image_vflip_in_place_8bpp

SYNOPSIS
   void vc_image_vflip_in_place_8bpp(VC_IMAGE_T *dest)

FUNCTION
   Vertically flip an image.

RETURNS
   void
******************************************************************************/

void vc_image_vflip_in_place_8bpp (VC_IMAGE_T *dest) {
   int pitch = dest->pitch;
   int height = dest->height;
   unsigned char *start_row = (unsigned char *)dest->image_data;
   unsigned char *end_row = start_row + (height-1)*pitch;
   int nbytes = (dest->width+15)&(~15);

   // Do the Y component. This is always 16-byte aligned.
   yuv420_vflip_in_place_16(start_row, end_row, pitch, nbytes);
}

/******************************************************************************
NAME
   vc_image_hflip_in_place_8bpp

SYNOPSIS
   void vc_image_hflip_in_place_8bpp(VC_IMAGE_T *dest)

FUNCTION
   Horizontally flip an image.

RETURNS
   void
******************************************************************************/

void vc_image_hflip_in_place_8bpp (VC_IMAGE_T *dest) {
   // The Y component can always be done efficiently.
   hflip_block((unsigned char *)dest->image_data, dest->width, dest->height, dest->pitch);
}

void vc_image_transpose_8bpp(VC_IMAGE_T *dst, VC_IMAGE_T *src )
{
   int src_pitch = src->pitch;
   int dst_pitch = dst->pitch;
   int dst_height = dst->height;
   int dst_width = dst->width;

   unsigned char *src_ptr = vc_image_get_y(src);
   unsigned char *dst_ptr = vc_image_get_y(dst);

   // Y.
   yuv420_transpose_Y(dst_ptr, dst_pitch, src_ptr, src_pitch, dst_width, dst_height);
}

/******************************************************************************
Static function definitions.
******************************************************************************/

/******************************************************************************
NAME
   vc_image_transpose_tiles_8bpp

SYNOPSIS
   void vc_image_transpose_tiles_8bpp(VC_IMAGE_T *dest, VC_IMAGE_T *src, int fat);

FUNCTION
   Transpose a series of 16x16 tiles in-place.  dest may be equal to src.

RETURNS
   void
******************************************************************************/

void vc_image_transpose_tiles_8bpp(VC_IMAGE_T *dst, VC_IMAGE_T *src, int fat)
{
   void (*yfunc)(unsigned char *dest, int dest_pitch, unsigned char *src, int src_pitch) = yuv420_transpose_chunky_Y;
   unsigned char *src_ptr = vc_image_get_y(src);
   unsigned char *dst_ptr = vc_image_get_y(dst);
   int dst_pitch, dst_height, dst_width, src_pitch = src->pitch;
   int i, chunksize = 256, x_offset;

   if (fat & 2)
   {
      if (fat == 2)
      {
         yfunc = yuv420_transpose_chunky0_Y;
      }
      else
      {
         yfunc = yuv420_transpose_chunky1_Y;
      }
      chunksize >>= 1;
      vcos_assert(src->width <= 32);
   }
   else
      vcos_assert(src->width <= 16);

   dst_pitch = (src_pitch * chunksize) >> 4;
   dst_width = dst_pitch & -chunksize;
   dst_height = (src->height + dst_width - 1) / dst_width << 4;

   vcos_assert(dst_ptr != src_ptr || dst_height * dst_pitch <= src->height * src_pitch);

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

   dst->width = dst_width;
   dst->pitch = dst_pitch;
   dst->height = dst_height;
}

/******************************************************************************
NAME
   vc_image_fill_8bpp

SYNOPSIS
   void vc_image_fill_8bpp(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int value)

FUNCTION
   Fill given region of an 8bpp image with solid colour value.

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

void vc_image_fill_8bpp (VC_IMAGE_T *dst, int x_offset, int y_offset, int width, int height, int value) {
   int dst_pitch = dst->pitch;
   int dst_offset = y_offset*dst_pitch + x_offset;
   unsigned char *dst_ptr;
   dst_ptr = vc_image_get_y(dst) + dst_offset;
   byte_set(dst_ptr, dst_pitch, width, height, value);
}


void vc_image_small_text_8bpp (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                               int *max_x, int *max_y) {
   int pitch = dest->pitch; // in pixels
   int start, end;
   int y;
   int sx = x_offset, sy = y_offset;

   *max_y = min(y_offset+8, dest->height)-1;

   for ( ; text[0] !=0 ; text++) {
      int c=(int)text[0]-32;

      if (c==0) {
         start = 0;
      } else {
         if ((c<0) || (c>=96)) c=1; // Change unknown characters to !
         // Now compute location of the text in memory
         start = smallfontpos[c-1];
      }
      end = smallfontpos[c];

      if (sx + end - start >= dest->width) {
         end = dest->width + start - sx; }

      for ( ; start < end; start++, sx++) {
         unsigned char *b;
         int data;

         if (sx < 0) continue;

         /* Needs to be displayed at location sx,sy */
         if (fg_colour == VC_IMAGE_COLOUR_TRANSPARENT)
            continue;

         b = (unsigned char *)dest->image_data + sy*pitch + sx;
         data = smallfontdata[start];  /* This contains packed binary for the letter */

         for ( y = sy; y <= *max_y; y++, data <<= 1, b += pitch) {
            if (y < 0) continue;

            if (data&(1<<15)) {
               // one means background
               if (bg_colour == VC_IMAGE_COLOUR_TRANSPARENT) {
                  // do nothing
               }
               else if (bg_colour == VC_IMAGE_COLOUR_FADE) {
                  // wipe out bottom bits and shift down
                  b[0] >>= 1;
               }
               else {
                  b[0] = bg_colour;
               }
            }
            else {
               b[0] = fg_colour;
            }
         }
      }
   }
   *max_x = sx;
}


/******************************************************************************
NAME
   vc_image_resize_stripe_h_8bpp

SYNOPSIS
   void vc_image_resize_stripe_h_8bpp(VC_IMAGE_T *dest, int d_width, int s_width)

FUNCTION
   Resize a 8BPP stripe horizontally.
******************************************************************************/

void vc_image_resize_stripe_h_8bpp(VC_IMAGE_T *dest, int d_width, int s_width)
{
   int step;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_8BPP));
   vcos_assert(d_width > 0 && s_width > 0);
   vcos_assert(dest->height == 16);
   vcos_assert((dest->pitch & 31) == 0);
   vcos_assert(dest->width >= s_width);
   vcos_assert(dest->width >= d_width);

   if (d_width <= 0)
      return;

   while (d_width << 2 < s_width)
   {
      s_width >>= 2;
      vc_stripe_h_reduce_bytes2(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, s_width, 0x40000);
   }

   step = ((s_width << 16) + (d_width >> 1)) / d_width;

   if (step != 0x10000)
   {
      switch (_msb((step - 1) | 0x1000))
      {
      case 17: /* 2.0 < step <= 4.0 */
         vc_stripe_h_reduce_bytes2(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, d_width, step);
         break;
      case 16: /* 1.0 < step <= 2.0 */
         vc_stripe_h_reduce_bytes_unsmoothed(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, d_width, step);
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

            vc_stripe_h_expand_bytes(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest) + off, dest->pitch, d_width, step, s_width);
         }
#else
         vc_stripe_h_expand_bytes(vc_image_get_y(dest) + d_width, dest->pitch, vc_image_get_y(dest) + s_width, dest->pitch, d_width, step);
#endif
         break;

      default:
         vcos_assert(!"can't cope with scale factor");
      }
   }
}

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

void vc_image_transparent_alpha_blt_8bpp_from_pal8 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
      VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int transparent_colour,int alpha)
{
   // add 512 to skip the palette
   unsigned char *src = (unsigned char *) src_image->image_data + src_image->pitch * src_y_offset + src_x_offset;
   unsigned char *dst = (unsigned char *)dest_image->image_data + dest_image->pitch * y_offset + x_offset;
   vcos_assert(alpha == 256);
   byte_blt(dst, dest_image->pitch, src, src_image->pitch, width, height);
}
/******************************************************************************
NAME
   vc_image_resize_stripe_v_8bpp

SYNOPSIS
   void vc_image_resize_stripe_v_8bpp(VC_IMAGE_T *dest, int d_height, int s_height)

FUNCTION
   Resize a 8BPP stripe vertically.
******************************************************************************/

void vc_image_resize_stripe_v_8bpp(VC_IMAGE_T *dest, int d_height, int s_height)
{
   int step;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_8BPP));
   vcos_assert(d_height > 0 && s_height > 0);
   vcos_assert(dest->width == 16);
//   vcos_assert((dest->pitch & 31) == 0);
   vcos_assert(dest->height >= s_height);
   vcos_assert(dest->height >= d_height);

   if (d_height <= 0)
      return;

   while (d_height << 2 < s_height)
   {
      s_height >>= 2;
      vc_stripe_v_reduce_bytes2(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, s_height, 0x40000);
   }

   step = ((s_height << 16) + (d_height >> 1)) / d_height;

   if (step != 0x10000)
   {
      switch (_msb((step - 1) | 0x1000))
      {
      case 17: /* 2.0 < step <= 4.0 */
         vc_stripe_v_reduce_bytes2(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, d_height, step);
         break;
      case 16: /* 1.0 < step <= 2.0 */
         vc_stripe_v_reduce_bytes_unsmoothed(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, d_height, step);
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

            vc_stripe_v_expand_bytes(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest) + off, dest->pitch, d_height, step, s_height);
         }
#else
         vc_stripe_v_expand_bytes(vc_image_get_y(dest) + dest->pitch * d_height, dest->pitch, vc_image_get_y(dest) + dest->pitch * s_height, dest->pitch, d_height, step);
#endif
         break;
      default:
         vcos_assert(!"can't cope with scale factor");
      }
   }
}

extern void vc_8bpp_font_alpha_blt();

/******************************************************************************
NAME
   vc_image_font_alpha_blt_8bpp

SYNOPSIS
   void vc_image_font_alpha_blt_8bpp(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                     VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int colour, int alpha, int antialias_flag);

FUNCTION
   Blt a special 4.4 font image. Antialiasing works on Y component of YUV
   or on greyscale destination; there is no antialiasing on palettized image.
   Alpha should be between 0 and 256.

RETURNS
   void
******************************************************************************/

void vc_image_font_alpha_blt_8bpp(VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                  VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int colour, int alpha, int antialias_flag)
{
   int dest_pitch, src_pitch;
   unsigned char *dest;
   unsigned char *src;

   dest_pitch = dest_image->pitch;
   src_pitch = src_image->pitch;
   dest = ((unsigned char *)dest_image->image_data) + y_offset*dest_pitch + x_offset;
   src = ((unsigned char *)src_image->image_data) + src_y_offset*src_pitch + src_x_offset;

   vcos_assert(width<=256); /* Current limitation of the blit function */

   vc_8bpp_font_alpha_blt(dest, src, dest_pitch, src_pitch, width, height, colour & 255, alpha, antialias_flag);
}
