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
#include <stdio.h>

/* Project includes */
#include "vcinclude/vcore.h"
#include "vclib/vclib.h"
#include "vc_image.h"
#include "yuv_to_rgb.h"
#include "genconv.h"
#include "interface/vcos/vcos_assert.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vc_image_convert_to_24bpp (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                          VC_IMAGE_T *src, int src_x_offset, int src_y_offset);
extern void vc_image_overlay_to_24bpp (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                          VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int transparent_colour);
extern void vc_image_convert_to_16bpp (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                          VC_IMAGE_T *src, int src_x_offset, int src_y_offset);
extern void vc_image_rgb565_to_rgb888_in_place (VC_IMAGE_T *dest);
extern void vc_image_rgb888_to_rgbdelta_in_place (VC_IMAGE_T *dest);
extern void vc_image_rgb565_to_rgb888_part (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *src, int src_x_offset, int src_y_offset);
extern void vc_image_3d32_to_rgb888_part (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *src, int src_x_offset, int src_y_offset,  short const *cmap);
extern void vc_image_3d32b_to_rgb888_part (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *src, int src_x_offset, int src_y_offset);
extern void vc_image_8bpp_to_rgb888_part (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *src, int src_x_offset, int src_y_offset, short *pal);
extern void vc_image_rgb888_to_rgb565_in_place (VC_IMAGE_T *dest);
extern void vc_image_fill_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height, int value);
extern void vc_image_fade_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height, int value);
extern void vc_image_not_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height);
extern void vc_image_blt_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                    VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset);
extern void vc_image_or_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                   VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset);
extern void vc_image_xor_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                    VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset);
extern void vc_image_transparent_blt_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int transparent_col);
extern void vc_image_overwrite_blt_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int overwrite_col);
extern void vc_image_masked_fill_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height, int value,
         VC_IMAGE_T *mask_image, int mask_x_offset, int mask_y_offset, int invert);
extern void vc_image_masked_blt_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                           VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, VC_IMAGE_T *mask_image, int mask_x_offset, int mask_y_offset, int invert);
extern void vc_image_transpose_rgb888 (VC_IMAGE_T *dest, VC_IMAGE_T *src);
extern void vc_image_transpose_in_place_rgb888 (VC_IMAGE_T *dest);
extern void vc_image_vflip_in_place_rgb888 (VC_IMAGE_T *dest);
extern void vc_image_hflip_in_place_rgb888 (VC_IMAGE_T *dest);
extern void vc_image_text_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                     int *max_x, int *max_y);
extern void vc_image_small_text_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                           int *max_x, int *max_y);
extern void vc_image_text_32_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                        int *max_x, int *max_y);
extern void vc_image_text_24_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                        int *max_x, int *max_y);
extern void vc_image_text_20_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                        int *max_x, int *max_y);
extern void vc_image_convert_yuv_rgb888 (VC_IMAGE_T *dest, VC_IMAGE_T *src, int mirror, int rotate);
extern void vc_image_convert_rgba32_rgb888 (VC_IMAGE_T *dest, VC_IMAGE_T *src, int mirror, int rotate);
extern void vc_image_convert_yuv422_rgb888(VC_IMAGE_T *dest, VC_IMAGE_T *src);
extern void vc_image_yuv420_to_rgb888_part (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset);
extern void vc_image_yuv422_to_rgb888_part (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset);
extern void vc_image_yuv2rgb888 (VC_IMAGE_T *dest, VC_IMAGE_T *src, int downsample);
extern void vc_image_convert_yuv422_rgb888 (VC_IMAGE_T *dest, VC_IMAGE_T *src);
extern void vc_image_font_alpha_blt_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int red,int green,int blue,int alpha);

/* Check extern defs match above and loads #defines and typedefs */
#include "vc_image_rgb888.h"

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

// Core functions.
extern void rgb888_load_initial_block(unsigned char *addr, int pitch, int vrf_row);
extern void rgb888_load_blocks(unsigned char *addr, int pitch, int vrf_row);
extern void rgb888_save_blocks(unsigned char *addr, int pitch, int nrows);
extern void rgb888_save_final_block(unsigned char *addr, int pitch, int nrows);
extern void load_block_1bpp(unsigned char *addr3, int pitch3, int circ3, int invert);

// Conversion to/from 16bpp.
extern void rgb888_to_rgb565(unsigned char *dest, unsigned char *src, int dest_pitch, int src_pitch);
extern void rgb565_to_rgb888(unsigned char *dest, unsigned char *src, int dest_pitch, int src_pitch);
extern void rgb565_to_rgb888_overlay(unsigned char *dest, unsigned char *src, int dest_pitch, int src_pitch, int transparent_colour, unsigned short mask, int lines);

// Functions to implement specific operations with 1 bitmap argument.
extern void rgb888_fill(int off1, void *arg, int ncols);
extern void rgb888_not(int off1, void *arg, int ncols);
extern void rgb888_fade(int off1, void *arg, int ncols);

// Functions to implement specific operations with 2 bitmap arguments.
extern void rgb888_blt(int off1, int off2, void *arg, int ncols);
extern void rgb888_transparent_blt(int off1, int off2, void *arg, int ncols);
extern void rgb888_overwrite_blt(int off1, int off2, void *arg, int ncols);
extern void rgb888_xor(int off1, int off2, void *arg, int ncols);
extern void rgb888_or(int off1, int off2, void *arg, int ncols);

// Functions to implement specific operations with 3 bitmap arguments (the last
// one being a 1bpp mask).
extern void rgb888_masked_fill(int off1, int off2, int off3, void *arg, int ncols);
extern void rgb888_masked_blt(int off1, int off2, int off3, void *arg, int ncols);

// Other functions.
extern void rgb888_transpose(int vrf_addr);
extern void rgb888_vflip_in_place(unsigned char *start_row, unsigned char *end_row, int pitch, int nbytes);
extern void rgb888_from_yuv(unsigned char *Y, unsigned char *U, unsigned char *V,
                               int in_pitch, unsigned char *rgb, int out_pitch);
extern void rgb888_from_yuv_downsampled(unsigned char *y_ptr, unsigned char *u_ptr, unsigned char *v_ptr,
                                           int src_pitch, unsigned char *dest_ptr, int dest_pitch);
void rgb888_to_rgbdelta(unsigned char *dest, unsigned char *src, int dest_pitch, int src_pitch, int color_offset);
void rgb888_to_rgbdelta_init(unsigned char *src, int src_pitch, int src_width);
void vc_rgb888_font_alpha_blt(unsigned short *dest, unsigned char *src, int dest_pitch_bytes,
                              int src_pitch_bytes, int width, int height,int red,int green,int blue,int alpha);

// For hflip.
extern void rgb888_hflip_load_right_initial(unsigned char *addr, int pitch);
extern void rgb888_hflip_load_left_blocks(unsigned char *addr, int pitch);
extern void rgb888_hflip_load_right_blocks(unsigned char *addr, int pitch);
extern void rgb888_hflip(int off);
extern void rgb888_hflip_save_right_blocks(unsigned char *addr, int pitch);
extern void rgb888_hflip_save_left_blocks(unsigned char *addr, int pitch);
extern void rgb888_hflip_save_right_final(unsigned char *right, int pitch);
extern void rgb888_hflip_final(int ncols);

/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/

// Type for functions that implement operations with 1 bitmap argument.
typedef void (proc1_fn)(int off1, void *arg, int ncols);

// Type for functions that implement operations with 2 bitmap arguments.
typedef void (proc2_fn)(int off1, int off2, void *arg, int ncols);

// Type for functions that implement operations with 3 bitmap arguments (the last
// one being a 1bpp mask).
typedef void (proc3_fn)(int off1, int off2, int off3, void *arg, int ncols);

/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/

// Wrapper functions for operations that take a single bitmap argument.
static void proc1_rect(VC_IMAGE_T *im1, int im1_x, int im1_y, int w, int h, proc1_fn *fn, void *arg);
static void proc1_stripe(unsigned char *addr1, int pitch1, int w, int nrows, proc1_fn *fn, void *arg);

// Wrapper functions for operations that take 2 bitmap arguments.
static void proc2_rect(VC_IMAGE_T *im1, int im1_x, int im1_y, int w, int h,
                       VC_IMAGE_T *im2, int im2_x, int im2_y, proc2_fn *fn, void *arg);
static void proc2_stripe(unsigned char *addr1, int pitch1, unsigned char *addr2, int pitch2,
                         int w, int nrows, proc2_fn *fn, void *arg);

// Wrapper functions for operations that take 3 bitmap arguments (the last one
// being a 1bpp mask).
static void proc3_rect(VC_IMAGE_T *im1, int im1_x, int im1_y, int w, int h,
                       VC_IMAGE_T *im2, int im2_x, int im2_y,
                       VC_IMAGE_T *im3, int im3_x, int im3_y, proc3_fn *fn, int invert, void *arg);
static void proc3_stripe(unsigned char *addr1, int pitch1, unsigned char *addr2, int pitch2,
                         unsigned char *addr3, int pitch3, int off3, int w, int nrows, proc3_fn *fn, int invert, void *arg);

static void transpose_1_block(unsigned char *dest, unsigned char *src, int dest_pitch, int src_pitch);
static void transpose_2_blocks(unsigned char *block1, unsigned char *block2, int pitch);

/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/

extern const short fontpos16[(128-32)];
extern const short fontdata16[32];  /* Fontdata can actually have more entries than specified here */

extern const short smallfontpos[(128-32)];
extern const short smallfontdata[32];  /* Fontdata can actually have more entries than specified here */

extern const short fontpos[(128-32)];
extern const long fontdata[32];  /* Fontdata can actually have more entries than specified here */

extern const short fontpos24[(128-32)];
extern const long fontdata24[32];  /* Fontdata can actually have more entries than specified here */

extern const short fontpos20[(128-32)];
extern const short fontdata20[32];  /* Fontdata can actually have more entries than specified here */

/******************************************************************************
Global function definitions.
******************************************************************************/

/******************************************************************************
NAME
   vc_image_convert_to_24bpp

SYNOPSIS
   void vc_image_convert_to_24bpp(VC_IMAGE_T *dest, int x_offset, y_offset, int width, int height,
                                  VC_IMAGE_T *src, int src_x_offset, int src_y_offset)

FUNCTION
   Convert a 16bpp image to 24bpp. Pitch of image must be large.

RETURNS
   void
******************************************************************************/

void vc_image_convert_to_24bpp(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                               VC_IMAGE_T *src, int src_x_offset, int src_y_offset) {
   int h = height;
   int nblocks = ((width+15)&(~15))>>4;
   int n;
   unsigned char *dest_addr = (unsigned char *)dest->image_data + y_offset*dest->pitch + x_offset*3;
   unsigned char *src_addr = (unsigned char *)src->image_data + src_y_offset*src->pitch + src_x_offset*2;

   for (; h > 0; h -= 16, dest_addr += (dest->pitch<<4), src_addr += (src->pitch<<4)) {
      for (n = nblocks-1; n >= 0; n--) {
         unsigned char *dest_block = dest_addr + n*48;
         unsigned char *src_block = src_addr + n*32;
         rgb565_to_rgb888(dest_block, src_block, dest->pitch, src->pitch);
      }
   }
}

/******************************************************************************
NAME
   vc_image_overlay_to_24bpp

SYNOPSIS
   void vc_image_overlay_to_24bpp(VC_IMAGE_T *dest, int x_offset, y_offset, int width, int height,
                                  VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int transparent_colour)

FUNCTION
   Overlay a 16bpp image to 24bpp. Pitch of image must be large.

RETURNS
   void
******************************************************************************/

void vc_image_overlay_to_24bpp(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                               VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int transparent_colour) {
   int h = height;
   int nblocks;
   int n;
   unsigned char *dest_addr = (unsigned char *)dest->image_data + y_offset*dest->pitch + x_offset*3;
   unsigned char *src_addr = (unsigned char *)src->image_data + src_y_offset*src->pitch + src_x_offset*2;
   unsigned short leftmask, rightmask;

   dest_addr -= (x_offset & 15) * 3;
   src_addr -= (x_offset & 15) * 2;
   width += (x_offset & 15);
   leftmask = 0xFFFF << (x_offset & 15);
   rightmask = 0xFFFF >> (~(width - 1) & 15);

   nblocks = ((width+15)&(~15))>>4;
   if (nblocks == 1)
      leftmask &= rightmask;

   for (; h > 0; h -= 16, dest_addr += (dest->pitch<<4), src_addr += (src->pitch<<4))
   {
      int opheight = _min(h, 16);
      int opmask = leftmask;
      unsigned char *dest_block = dest_addr;
      unsigned char *src_block = src_addr;

      for (n = nblocks-1; n >= 0; n--) {
         rgb565_to_rgb888_overlay(dest_block, src_block, dest->pitch, src->pitch, transparent_colour, opmask, opheight);
         opmask = (n == 1) ? rightmask : 0xFFFF;
         dest_block += 48;
         src_block += 32;
      }
   }
}

/******************************************************************************
NAME
   vc_image_convert_to_16bpp

SYNOPSIS
   void vc_image_convert_to_16bpp(VC_IMAGE_T *dest, int x_offset, y_offset, int width, int height,
                                  VC_IMAGE_T *src, int src_x_offset, int src_y_offset)

FUNCTION
   Convert a 24bpp image to 16bpp. Pitch of image must be large.

RETURNS
   void
******************************************************************************/

void vc_image_convert_to_16bpp(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                               VC_IMAGE_T *src, int src_x_offset, int src_y_offset) {
   int h = height;
   int nblocks = ((width+15)&(~15))>>4;
   int n;
   unsigned char *dest_addr = (unsigned char *)dest->image_data + y_offset*dest->pitch + x_offset*2;
   unsigned char *src_addr = (unsigned char *)src->image_data + src_y_offset*src->pitch + src_x_offset*3;

   for (; h > 0; h -= 16, dest_addr += (dest->pitch<<4), src_addr += (src->pitch<<4)) {
      for (n = nblocks-1; n >= 0; n--) {
         unsigned char *dest_block = dest_addr + n*32;
         unsigned char *src_block = src_addr + n*48;
         rgb888_to_rgb565(dest_block, src_block, dest->pitch, src->pitch);
      }
   }
}
/******************************************************************************
NAME
   vc_image_rgb565_to_rgb888_in_place

SYNOPSIS
   void vc_image_rgb565_to_rgb888_in_place(VC_IMAGE_T *dest)

FUNCTION
   Convert a 16bpp image to 24bpp in-place. Pitch of image must be large
   enough for the 24bpp incarnation.

RETURNS
   void
******************************************************************************/

void vc_image_rgb565_to_rgb888_in_place(VC_IMAGE_T *dest) {
   int h = dest->height;
   int pitch = dest->pitch;
   int w = dest->width;
   int nblocks = ((w+15)&(~15))>>4;
   int n;
   unsigned char *addr = dest->image_data;
   assert(pitch >= dest->width*3);

   dest->type = VC_IMAGE_RGB888;

   for (; h > 0; h -= 16, addr += (pitch<<4)) {
      for (n = nblocks-1; n >= 0; n--) {
         unsigned char *dest_block = addr + n*48;
         unsigned char *src_block = addr + n*32;
         rgb565_to_rgb888(dest_block, src_block, pitch, pitch);
      }
   }
}

/******************************************************************************
NAME
   vc_image_rgb888_to_rgbdelta_in_place

SYNOPSIS
   void vc_image_rgb888_to_rgbdelta_in_place(VC_IMAGE_T *dest)

FUNCTION
   Convert a 24bpp image to rgb delta in-place. Pitch is left unchanged.

RETURNS
   void
******************************************************************************/

void vc_image_rgb888_to_rgbdelta_in_place(VC_IMAGE_T *dest) {
   int h = dest->height;
   int pitch = dest->pitch;
   int w = dest->width;
   int nblocks = ((w+15)&(~15))>>4;
   int n;
   unsigned char *addr = dest->image_data;

//   dest->type = VC_IMAGE_8BPP;

   for (; h > 0; h -= 16, addr += (pitch<<4)) {
      rgb888_to_rgbdelta_init(addr, pitch, w);
      for (n = 0; n < nblocks; n++) {
         unsigned char *dest_block = addr + n*32;
         unsigned char *src_block = addr + n*48;
         rgb888_to_rgbdelta(dest_block, src_block, pitch, pitch, n%3);
      }
   }
}

/******************************************************************************
NAME
   vc_image_rgb565_to_rgb888_part

SYNOPSIS
   void vc_image_rgb565_to_rgb888_part(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T          *src,
      int                  src_x_offset,
      int                  src_y_offset);

FUNCTION
   Convert part of an RGB565 image to RGB888.

RETURNS
   void
******************************************************************************/

void vc_image_rgb565_to_rgb888_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset)
{
   int                  dpitch,
   spitch;
   unsigned char       *dptr;
   unsigned short const *sptr;
   int                  optimised_tail = 0;

   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGB565) && is_valid_vc_image_buf(dest, VC_IMAGE_RGB888));

   dpitch = dest->pitch;
   spitch = src->pitch;
   dptr = dest->image_data;
   sptr = src->image_data;

   if ((height & 15) > 2 && height > 16 && (unsigned long)dptr > 0)
      optimised_tail = 16 - (height & 15);


   dptr += x_offset * 3 + y_offset * dpitch;
   sptr = (void *)((char *)sptr + src_y_offset * spitch);
   sptr += src_x_offset;

   if (height >= 16)
      do
      {
         vc_genconv_rgb565_to_rgb888(dptr, dpitch, sptr, spitch, width, 16);
         dptr += dpitch << 4;
         sptr += (spitch << 4)>>1; // measured in shorts
         height -= 16;
      } while (height >= 16);

   if (optimised_tail)
   {
      dptr -= dpitch * optimised_tail;
      sptr -= spitch * optimised_tail;
      vc_genconv_rgb565_to_rgb888(dptr, dpitch, sptr, spitch, width, 16);
   }
   else if (height > 0)
      vc_genconv_rgb565_to_rgb888(dptr, dpitch, sptr, spitch, width, height);
}


/******************************************************************************
NAME
   vc_image_3d32_to_rgb888_part

SYNOPSIS
   void vc_image_3d32_to_rgb888_part(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T          *src,
      int                  src_x_offset,
      int                  src_y_offset,
      hort          const *cmap);

FUNCTION
   Convert part of a V3D32 image to RGB888.

RETURNS
   void
******************************************************************************/

void vc_image_3d32_to_rgb888_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset,
   short         const *cmap)
{
   int                  dpitch,
   spitch;
   unsigned char       *dptr;
   unsigned short const *sptr;
   int                  optimised_tail = 0;

   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_3D32) && is_valid_vc_image_buf(dest, VC_IMAGE_RGB888) && cmap != NULL);

   dpitch = dest->pitch;
   spitch = src->pitch;
   dptr = dest->image_data;
   sptr = src->image_data;

   if ((height & 15) > 2 && height > 16 && (unsigned long)dptr > 0)
      optimised_tail = 16 - (height & 15);


   dptr += x_offset * 3 + y_offset * dpitch;
   sptr = (void *)((char *)sptr + src_y_offset * spitch);
   sptr += src_x_offset + (src_x_offset & ~15);

   if (height >= 16)
      do
      {
         vc_genconv_3d32_to_rgb888(dptr, dpitch, sptr, spitch, width, 16, (unsigned short const *)cmap);
         dptr += dpitch << 4;
         sptr += spitch << 4;
         height -= 16;
      } while (height >= 16);

   if (optimised_tail)
   {
      dptr -= dpitch * optimised_tail;
      sptr -= spitch * optimised_tail;
      vc_genconv_3d32_to_rgb888(dptr, dpitch, sptr, spitch, width, 16, (unsigned short const *)cmap);
   }
   else if (height > 0)
      vc_genconv_3d32_to_rgb888(dptr, dpitch, sptr, spitch, width, height, (unsigned short const *)cmap);
}


/******************************************************************************
NAME
   vc_image_3d32b_to_rgb888_part

SYNOPSIS
   void vc_image_3d32b_to_rgb888_part(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T          *src,
      int                  src_x_offset,
      int                  src_y_offset);

FUNCTION
   Convert part of an V3D32B image to RGB888.

RETURNS
   void
******************************************************************************/

void vc_image_3d32b_to_rgb888_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset)
{
   int                  dpitch,
   spitch;
   unsigned char       *dptr;
   unsigned short const *sptr;
   int                  optimised_tail = 0;

   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_3D32B) && is_valid_vc_image_buf(dest, VC_IMAGE_RGB888));

   dpitch = dest->pitch;
   spitch = src->pitch;
   dptr = dest->image_data;
   sptr = src->image_data;

   if ((height & 15) > 2 && height > 16 && (unsigned long)dptr > 0)
      optimised_tail = 16 - (height & 15);


   dptr += x_offset * 3 + y_offset * dpitch;
   sptr = (void *)((char *)sptr + src_y_offset * spitch);
   sptr += src_x_offset + (src_x_offset & ~15);

   if (height >= 16)
      do
      {
         vc_genconv_3d32b_to_rgb888(dptr, dpitch, sptr, spitch, width, 16);
         dptr += dpitch << 4;
         sptr += spitch << 4;
         height -= 16;
      } while (height >= 16);

   if (optimised_tail)
   {
      dptr -= dpitch * optimised_tail;
      sptr -= spitch * optimised_tail;
      vc_genconv_3d32b_to_rgb888(dptr, dpitch, sptr, spitch, width, 16);
   }
   else if (height > 0)
      vc_genconv_3d32b_to_rgb888(dptr, dpitch, sptr, spitch, width, height);
}

/******************************************************************************
NAME
   vc_image_8bpp_to_rgb888_part

SYNOPSIS
   void vc_image_8bpp_to_rgb888_part(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T          *src,
      int                  src_x_offset,
      int                  src_y_offset);

FUNCTION
   Convert part of an 8bpp image to RGB888.

RETURNS
   void
******************************************************************************/

void vc_image_8bpp_to_rgb888_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset,
   short               *pal)
{
   int                  dpitch,
   spitch;
   unsigned char       *dptr;
   unsigned char const *sptr;
   int                  optimised_tail = 0;

   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_8BPP) && is_valid_vc_image_buf(dest, VC_IMAGE_RGB888));

   dpitch = dest->pitch;
   spitch = src->pitch;
   dptr = dest->image_data;
   sptr = src->image_data;

   if ((height & 15) > 2 && height > 16 && (unsigned long)dptr > 0)
      optimised_tail = 16 - (height & 15);


   dptr += x_offset * 3 + y_offset * dpitch;
   sptr = (void *)((char *)sptr + src_y_offset * spitch);
   sptr += src_x_offset;

   if (height >= 16)
      do
      {
         vc_genconv_8bpp_to_rgb888(dptr, dpitch, sptr, spitch, width, 16, pal);
         dptr += dpitch << 4;
         sptr += spitch << 4;
         height -= 16;
      } while (height >= 16);

   if (optimised_tail)
   {
      dptr -= dpitch * optimised_tail;
      sptr -= spitch * optimised_tail;
      vc_genconv_8bpp_to_rgb888(dptr, dpitch, sptr, spitch, width, 16, pal);
   }
   else if (height > 0)
      vc_genconv_8bpp_to_rgb888(dptr, dpitch, sptr, spitch, width, height, pal);
}


/******************************************************************************
NAME
   vc_image_rgb888_to_rgb565_in_place

SYNOPSIS
   void vc_image_rgb888_to_rgb565_in_place(VC_IMAGE_T *dest)

FUNCTION
   Convert a 24bpp image to 16bpp in-place. Pitch is left unchanged.

RETURNS
   void
******************************************************************************/

void vc_image_rgb888_to_rgb565_in_place(VC_IMAGE_T *dest) {
   int h = dest->height;
   int pitch = dest->pitch;
   int w = dest->width;
   int nblocks = ((w+15)&(~15))>>4;
   int n;
   unsigned char *addr = dest->image_data;

   dest->type = VC_IMAGE_RGB565;

   for (; h > 0; h -= 16, addr += (pitch<<4)) {
      for (n = 0; n < nblocks; n++) {
         unsigned char *dest_block = addr + n*32;
         unsigned char *src_block = addr + n*48;
         rgb888_to_rgb565(dest_block, src_block, pitch, pitch);
      }
   }
}

/******************************************************************************
NAME
   vc_image_fill_rgb888

SYNOPSIS
   void vc_image_fill_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int col)

FUNCTION
   Fill given region of an RGB888 image with solid colour value.

RETURNS
   void
******************************************************************************/

void vc_image_fill_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height, int value) {
   proc1_rect(dest_image, x_offset, y_offset, width, height, rgb888_fill, (void *)value);
}

/******************************************************************************
NAME
   vc_image_fade_rgb888

SYNOPSIS
   void vc_image_fade_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int col)

FUNCTION
   Fade given region of an RGB888 image with 8.8 alpha value.

RETURNS
   void
******************************************************************************/

void vc_image_fade_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height, int value) {
   proc1_rect(dest_image, x_offset, y_offset, width, height, rgb888_fade, (void *)value);
}

/******************************************************************************
NAME
   vc_image_not_rgb888

SYNOPSIS
   void vc_image_not_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int col)

FUNCTION
   Not given region of an RGB888 image.

RETURNS
   void
******************************************************************************/

void vc_image_not_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height) {
   proc1_rect(dest_image, x_offset, y_offset, width, height, rgb888_not, NULL);
}

/******************************************************************************
NAME
   vc_image_blt_rgb888

SYNOPSIS
   void vc_image_blt_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                            VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

FUNCTION
   Blt given region of an RGB888 image to another.

RETURNS
   void
******************************************************************************/

void vc_image_blt_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                          VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset) {
   // proc2_rect is not maximally efficient here because we don't need to load
   // the dest bitmap, except at the edges. Still, it will do for now.
   proc2_rect(dest_image, x_offset, y_offset, width, height,
              src_image, src_x_offset, src_y_offset, rgb888_blt, NULL);
}

/******************************************************************************
NAME
   vc_image_or_rgb888

SYNOPSIS
   void vc_image_or_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                           VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

FUNCTION
   Or given region of an RGB888 image onto another.

RETURNS
   void
******************************************************************************/

void vc_image_or_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                         VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset) {
   // proc2_rect is not maximally efficient here because we don't need to load
   // the dest bitmap, except at the edges. Still, it will do for now.
   proc2_rect(dest_image, x_offset, y_offset, width, height,
              src_image, src_x_offset, src_y_offset, rgb888_or, NULL);
}

/******************************************************************************
NAME
   vc_image_xor_rgb888

SYNOPSIS
   void vc_image_xor_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                           VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

FUNCTION
   Xor given region of an RGB888 image onto another.

RETURNS
   void
******************************************************************************/

void vc_image_xor_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                          VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset) {
   // proc2_rect is not maximally efficient here because we don't need to load
   // the dest bitmap, except at the edges. Still, it will do for now.
   proc2_rect(dest_image, x_offset, y_offset, width, height,
              src_image, src_x_offset, src_y_offset, rgb888_xor, NULL);
}

/******************************************************************************
NAME
   vc_image_transparent_blt_rgb888

SYNOPSIS
   void vc_image_transparent_blt_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                        VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int transparent_col);

FUNCTION
   Blt given region of an RGB888 image to another with a nominated transparent colour.

RETURNS
   void
******************************************************************************/

void vc_image_transparent_blt_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                      VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int transparent_col) {
   int RGB[3];
   RGB[0] = transparent_col>>16;
   RGB[1] = (transparent_col>>8)&255;
   RGB[2] = transparent_col&255;
   proc2_rect(dest_image, x_offset, y_offset, width, height,
              src_image, src_x_offset, src_y_offset, rgb888_transparent_blt, RGB);
}

/******************************************************************************
NAME
   vc_image_overwrite_blt_rgb888

SYNOPSIS
   void vc_image_overwrite_blt_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                      VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int overwrite_col);

FUNCTION
   Blt given region of an RGB888 image to another only where dest has given colour.

RETURNS
   void
******************************************************************************/

void vc_image_overwrite_blt_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                    VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int overwrite_col) {
   int RGB[3];
   RGB[0] = overwrite_col>>16;
   RGB[1] = (overwrite_col>>8)&255;
   RGB[2] = overwrite_col&255;
   proc2_rect(dest_image, x_offset, y_offset, width, height,
              src_image, src_x_offset, src_y_offset, rgb888_overwrite_blt, RGB);
}

/******************************************************************************
NAME
   vc_image_masked_fill_rgb888

SYNOPSIS
   void vc_image_masked_fill_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int col, int invert)

FUNCTION
   Fill given region of an RGB888 image with solid colour value where the mask is set (invert==0),
   or where mask is zero (invert non-zero).

RETURNS
   void
******************************************************************************/

void vc_image_masked_fill_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height, int value,
                                  VC_IMAGE_T *mask_image, int mask_x_offset, int mask_y_offset, int invert) {
   if (invert)
      invert = -1;
   proc3_rect(dest_image, x_offset, y_offset, width, height, NULL, 0, 0,
              mask_image, mask_x_offset, mask_y_offset, rgb888_masked_fill, invert, (void *)value);
}

/******************************************************************************
NAME
   vc_image_masked_blt_rgb888

SYNOPSIS
   void vc_image_masked_fill_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                    VC_IMAGE_T *src, int src_x_offset, int src_y_offset,
                                    VC_IMAGE_T *mask, int mask_x_offset, int mask_y_offset, int invert)

FUNCTION
   Blt given region of an RGB888 image from src where the mask is set (invert==0),
   or where mask is zero (invert non-zero).

RETURNS
   void
******************************************************************************/

void vc_image_masked_blt_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                 VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset,
                                 VC_IMAGE_T *mask_image, int mask_x_offset, int mask_y_offset, int invert) {
   if (invert)
      invert = -1;
   proc3_rect(dest_image, x_offset, y_offset, width, height,
              src_image, src_x_offset, src_y_offset,
              mask_image, mask_x_offset, mask_y_offset, rgb888_masked_blt, invert, NULL);
}

/******************************************************************************
NAME
   vc_image_transpose_rgb888

SYNOPSIS
   void vc_image_transpose_rgb888(VC_IMAGE_T *dest, VC_IMAGE_T *src);

FUNCTION
   Not-in-place transpose.

RETURNS
   void
******************************************************************************/

static void transpose_1_block(unsigned char *dest, unsigned char *src, int dest_pitch, int src_pitch) {
   rgb888_load_blocks(src-16, src_pitch, 16*64);
   rgb888_transpose(16*64);
   rgb888_save_blocks(dest, dest_pitch, 16);
}

void vc_image_transpose_rgb888 (VC_IMAGE_T *dest, VC_IMAGE_T *src) {
   int w, h;
   asm("        cbclr\n");
   for (h = 0; h < src->height; h += 16) {
      for (w = 0; w < src->width; w += 16) {
         unsigned char *s = (unsigned char *)src->image_data + h*src->pitch + w*3;
         unsigned char *d = (unsigned char *)dest->image_data + w*dest->pitch + h*3;
         transpose_1_block(d, s, dest->pitch, src->pitch);
      }
   }
}

/******************************************************************************
NAME
   vc_image_transpose_in_place_rgb888

SYNOPSIS
   void vc_image_transpose_in_place_rgb888(VC_IMAGE_T *dest);

FUNCTION
   In-place transpose. Relies on being able to preserve pitch.

RETURNS
   void
******************************************************************************/

static void transpose_2_blocks(unsigned char *block1, unsigned char *block2, int pitch) {
   rgb888_load_blocks(block1-16, pitch, 16*64);
   rgb888_load_blocks(block2-16, pitch, 48*64);
   rgb888_transpose(16*64);
   rgb888_save_blocks(block2, pitch, 16);
   rgb888_transpose(48*64);
   rgb888_save_blocks(block1, pitch, 16);
}

void vc_image_transpose_in_place_rgb888 (VC_IMAGE_T *dest) {
   int w, h;
   asm("        cbclr\n");
   if (dest->width > dest->height) {
      for (h = 0; h < dest->height; h += 16) {
         for (w = h; w < dest->height; w += 16) {
            unsigned char *block1 = (unsigned char *)dest->image_data + h*dest->pitch + w*3;
            unsigned char *block2 = (unsigned char *)dest->image_data + w*dest->pitch + h*3;
            transpose_2_blocks(block1, block2, dest->pitch);
         }
         for (; w < dest->width; w += 16) {
            unsigned char *block1 = (unsigned char *)dest->image_data + h*dest->pitch + w*3;
            unsigned char *block2 = (unsigned char *)dest->image_data + w*dest->pitch + h*3;
            transpose_1_block(block2, block1, dest->pitch, dest->pitch);
         }
      }
   }
   else {
      for (h = 0; h < dest->width; h += 16) {
         for (w = h; w < dest->width; w += 16) {
            unsigned char *block1 = (unsigned char *)dest->image_data + h*dest->pitch + w*3;
            unsigned char *block2 = (unsigned char *)dest->image_data + w*dest->pitch + h*3;
            transpose_2_blocks(block1, block2, dest->pitch);
         }
      }
      for (; h < dest->height; h += 16) {
         for (w = 0; w < dest->width; w += 16) {
            unsigned char *block1 = (unsigned char *)dest->image_data + h*dest->pitch + w*3;
            unsigned char *block2 = (unsigned char *)dest->image_data + w*dest->pitch + h*3;
            transpose_1_block(block2, block1, dest->pitch, dest->pitch);
         }
      }
   }
}

/******************************************************************************
NAME
   vc_image_vflip_in_place_rgb888

SYNOPSIS
   void vc_image_vflip_in_place_rgb888(VC_IMAGE_T *dest);

FUNCTION
   Not-in-place vertical flip.

RETURNS
   void
******************************************************************************/

void vc_image_vflip_in_place_rgb888 (VC_IMAGE_T *dest) {
   int pitch = dest->pitch;
   unsigned char *start_row = (unsigned char *)dest->image_data;
   unsigned char *end_row = start_row + (dest->height-1)*pitch;
   int nbytes = ((dest->width+15)>>4)*48;
   rgb888_vflip_in_place(start_row, end_row, pitch, nbytes);
}

/******************************************************************************
NAME
   vc_image_hflip_in_place_rgb888

SYNOPSIS
   void vc_image_hflip_in_place_rgb888(VC_IMAGE_T *dest);

FUNCTION
   Not-in-place horizontal flip.

RETURNS
   void
******************************************************************************/

void vc_image_hflip_in_place_rgb888 (VC_IMAGE_T *dest) {
   int pitch = dest->pitch;
   int h;

   for (h = 0; h < dest->height; h += 16) {
      unsigned char *left = (unsigned char *)dest->image_data + pitch*h;
      unsigned char *right = left + (dest->width-16)*3;
      asm("     cbclr\n");
      rgb888_hflip_load_right_initial(right, pitch);

      for (; left <= right; left += 48, right -= 48) {
         // left points to the start of the left hand block, right to the start
         // of the right hand block.

         // Load left block into in (0,0), (0,16) and (0,32).
         rgb888_hflip_load_left_blocks(left, pitch);

         // Load right block into (16,0)* onwards.
         rgb888_hflip_load_right_blocks(right, pitch);

         // Hflip from (16,0)* into (32,0), and from (0,0) into (16,0)*.
         rgb888_hflip((int)right&15);

         // Save blocks from (16,0)* onwards.
         rgb888_hflip_save_right_blocks(right, pitch);

         // Save blocks from (32,0) onwards.
         rgb888_hflip_save_left_blocks(left, pitch);

         asm("  cbadd1\n");
      }

      if (left - right >= 48) {
         // Done?
      }
      else {
         // The final right block still needs saving, if we did anything at all.
         if (dest->width >= 16 && ((int)right&15))
            rgb888_hflip_save_right_final(right, pitch);

         // Load left block into in (0,0), (0,16) and (0,32).
         rgb888_hflip_load_left_blocks(left, pitch);

         // Now need to reverse those from column 0 up to width&15.
         rgb888_hflip_final(dest->width&15);

         // And save them.
         rgb888_hflip_save_left_blocks(left, pitch);
      }
   }
}

/******************************************************************************
NAME
   vc_image_text_rgb888

SYNOPSIS
   void vc_image_text_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, char *text,
                             int *max_x, int *max_y)

FUNCTION
   Write text to an image. Uses a fixed internal font and clips it to the image if
   it overflows. bg_colour may be specified as a colour value, or
   VC_IMAGE_COLOUR_TRANSPARENT to mean the backgroud shows through unchanged all
   around, or VC_IMAGE_COLOUR_FADE to preserve the background, but to fade it slightly.
   Also returns maximum x and y values written to.

RETURNS
   void
******************************************************************************/

void vc_image_text_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                           int *max_x, int *max_y) {
   int pitch = dest->pitch; // in pixels
   int start, end;
   int y;
   int sx = x_offset, sy = y_offset;

   *max_y = min(y_offset+12, dest->height)-1;

   for ( ; text[0] !=0 ; text++) {
      int c=(int)text[0]-32;

      if (c==0) {
         start = 0;
      } else {
         if ((c<0) || (c>=96)) c=1; // Change unknown characters to !
         // Now compute location of the text in memory
         start = fontpos16[c-1];
      }
      end = fontpos16[c];

      if (sx + end - start >= dest->width) {
         end = dest->width + start - sx; }

      for ( ; start < end; start++, sx++) {
         unsigned char *b;
         int data;

         if (sx < 0) continue;

         /* Needs to be displayed at location sx,sy */
         if (fg_colour == VC_IMAGE_COLOUR_TRANSPARENT)
            continue;

         b = (unsigned char *)dest->image_data + sy*pitch + sx*3;
         data = fontdata16[start] << 3;  /* This contains packed binary for the letter */

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
                  b[1] >>= 1;
                  b[2] >>= 1;
               }
               else {
                  b[0] = bg_colour>>16;
                  b[1] = (bg_colour>>8)&0xff;
                  b[2] = bg_colour&0xff;
               }
            }
            else {
               b[0] = fg_colour>>16;
               b[1] = (fg_colour>>8)&0xff;
               b[2] = fg_colour&0xff;
            }
         }
      }
   }
   *max_x = sx;
}

void vc_image_small_text_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
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

         b = (unsigned char *)dest->image_data + sy*pitch + sx*3;
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
                  b[1] >>= 1;
                  b[2] >>= 1;
               }
               else {
                  b[0] = bg_colour>>16;
                  b[1] = (bg_colour>>8)&0xff;
                  b[2] = bg_colour&0xff;
               }
            }
            else {
               b[0] = fg_colour>>16;
               b[1] = (fg_colour>>8)&0xff;
               b[2] = fg_colour&0xff;
            }
         }
      }
   }
   *max_x = sx;
}

void vc_image_text_32_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                              int *max_x, int *max_y) {//probably doesn't work yet.
   int pitch = dest->pitch; // in pixels
   int start, end;
   int y;
   int sx = x_offset, sy = y_offset;

   *max_y = min(y_offset+24, dest->height)-1;

   for ( ; text[0] !=0 ; text++) {
      int c=(int)text[0]-32;

      if (c==0) {
         start = 0;
      } else {
         if ((c<0) || (c>=96)) c=1; // Change unknown characters to !
         // Now compute location of the text in memory
         start = fontpos[c-1];
      }
      end = fontpos[c];

      if (sx + end - start >= dest->width) {
         end = dest->width + start - sx; }

      for ( ; start < end; start++, sx++) {
         unsigned char *b;
         int data;

         if (sx < 0) continue;

         /* Needs to be displayed at location sx,sy */
         if (fg_colour == VC_IMAGE_COLOUR_TRANSPARENT)
            continue;

         b = (unsigned char *)dest->image_data + sy*pitch + sx*3;
         data = fontdata[start] << 3;  /* This contains packed binary for the letter */

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
                  b[1] >>= 1;
                  b[2] >>= 1;
               }
               else {
                  b[0] = bg_colour>>16;
                  b[1] = (bg_colour>>8)&0xff;
                  b[2] = bg_colour&0xff;
               }
            }
            else {
               b[0] = fg_colour>>16;
               b[1] = (fg_colour>>8)&0xff;
               b[2] = fg_colour&0xff;
            }
         }
      }
   }
   *max_x = sx;
}

void vc_image_text_24_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                              int *max_x, int *max_y) {
   int pitch = dest->pitch; // in pixels
   int start, end;
   int y;
   int sx = x_offset, sy = y_offset;

   *max_y = min(y_offset+24, dest->height)-1;

   for ( ; text[0] !=0 ; text++) {
      int c=(int)text[0]-32;

      if (c==0) {
         start = 0;
      } else {
         if ((c<0) || (c>=96)) c=1; // Change unknown characters to !
         // Now compute location of the text in memory
         start = fontpos24[c-1];
      }
      end = fontpos24[c];

      if (sx + end - start >= dest->width) {
         end = dest->width + start - sx; }

      for ( ; start < end; start++, sx++) {
         unsigned char *b;
         int data;

         if (sx < 0) continue;

         /* Needs to be displayed at location sx,sy */
         if (fg_colour == VC_IMAGE_COLOUR_TRANSPARENT)
            continue;

         b = (unsigned char *)dest->image_data + sy*pitch + sx*3;
         data = fontdata24[start];  /* This contains packed binary for the letter */

         for ( y = sy; y <= *max_y; y++, data <<= 1, b += pitch) {
            if (y < 0) continue;

            if (data&(1<<31)) {
               // one means background
               if (bg_colour == VC_IMAGE_COLOUR_TRANSPARENT) {
                  // do nothing
               }
               else if (bg_colour == VC_IMAGE_COLOUR_FADE) {
                  // wipe out bottom bits and shift down
                  b[0] >>= 1;
                  b[1] >>= 1;
                  b[2] >>= 1;
               }
               else {
                  b[0] = bg_colour>>16;
                  b[1] = (bg_colour>>8)&0xff;
                  b[2] = bg_colour&0xff;
               }
            }
            else {
               b[0] = fg_colour>>16;
               b[1] = (fg_colour>>8)&0xff;
               b[2] = fg_colour&0xff;
            }
         }
      }
   }
   *max_x = sx;
}

void vc_image_text_20_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                              int *max_x, int *max_y) {
   int pitch = dest->pitch; // in pixels
   int start, end;
   int y;
   int sx = x_offset, sy = y_offset;

   *max_y = min(y_offset+16, dest->height)-1;

   for ( ; text[0] !=0 ; text++) {
      int c=(int)text[0]-32;

      if (c==0) {
         start = 0;
      } else {
         if ((c<0) || (c>=96)) c=1; // Change unknown characters to !
         // Now compute location of the text in memory
         start = fontpos20[c-1];
      }
      end = fontpos20[c];

      if (sx + end - start >= dest->width) {
         end = dest->width + start - sx; }

      for ( ; start < end; start++, sx++) {
         unsigned char *b;
         int data;

         if (sx < 0) continue;

         /* Needs to be displayed at location sx,sy */
         if (fg_colour == VC_IMAGE_COLOUR_TRANSPARENT)
            continue;

         b = (unsigned char *)dest->image_data + sy*pitch + sx*3;
         data = 0xffff0000 | fontdata20[start];  /* This contains packed binary for the letter */

         for ( y = sy; y <= *max_y; y++, data <<= 1, b += pitch) {
            if (y < 0) continue;

            if (data&(1<<16)) {
               // one means background
               if (bg_colour == VC_IMAGE_COLOUR_TRANSPARENT) {
                  // do nothing
               }
               else if (bg_colour == VC_IMAGE_COLOUR_FADE) {
                  // wipe out bottom bits and shift down
                  b[0] >>= 1;
                  b[1] >>= 1;
                  b[2] >>= 1;
               }
               else {
                  b[0] = bg_colour>>16;
                  b[1] = (bg_colour>>8)&0xff;
                  b[2] = bg_colour&0xff;
               }
            }
            else {
               b[0] = fg_colour>>16;
               b[1] = (fg_colour>>8)&0xff;
               b[2] = fg_colour&0xff;
            }
         }
      }
   }
   *max_x = sx;
}

/******************************************************************************
NAME
   vc_image_convert_yuv_rgb888

SYNOPSIS
   void vc_image_convert_yuv_rgb888(VC_IMAGE_T *dest, VC_IMAGE_T *src, int mirror, int rotate)


FUNCTION
   Blit from a YUV image to an RGB image with optional mirroring followed
   by optional clockwise rotation. WARNING: mirror and rotate unsupported.

RETURNS
   void
******************************************************************************/

void vc_image_convert_yuv_rgb888(VC_IMAGE_T *dest, VC_IMAGE_T *src, int mirror, int rotate) {
// xxx dc4    assert(!mirror && !rotate);  // not supported (yet?)
   vc_image_yuv2rgb888(dest, src, 0);
}

/******************************************************************************
NAME
   vc_image_convert_rgba32_rgb888

SYNOPSIS
   void vc_image_convert_rgba32_rgb888(VC_IMAGE_T *dest, VC_IMAGE_T *src, int mirror, int rotate)


FUNCTION
   Blit from an RGBA32 image to an RGB888 image with optional mirroring
   followed by optional clockwise rotation. WARNING: mirror and rotate
   unsupported.

RETURNS
   void
******************************************************************************/

void vc_image_convert_rgba32_rgb888 (VC_IMAGE_T *dest, VC_IMAGE_T *src, int mirror, int rotate)
{
   int x, y;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB888));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32));
   assert(mirror == 0 && rotate == 0);

   for (y = 0; y < dest->height; y += 16)
   {
      char *dptr = dest->image_data;
      char *sptr = src->image_data;

      dptr += y * dest->pitch;
      sptr += y * src->pitch;

      for (x = 0; x < dest->width; x += 16)
      {
         rgb888_from_rgba32(dptr, dest->pitch, sptr, src->pitch);

         dptr += 48;
         sptr += 64;
      }
   }
}

/******************************************************************************
NAME
   vc_image_yuv420_to_rgb888_part

SYNOPSIS
   void vc_image_yuv422_to_rgb888_part(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T    const *src,
      int                  src_x_offset,
      int                  src_y_offset)

FUNCTION
   Blit part of a YUV image to an RGB888 image.

RETURNS
   void
******************************************************************************/

void vc_image_yuv420_to_rgb888_part(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset)
{
   unsigned char       *dptr;
   unsigned char const *yptr;
   unsigned char const *uptr;
   unsigned char const *vptr;
   int                  ypitch,
   uvpitch,
   optimised_tail = 0;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB888));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV420));

   ypitch = src->pitch;
   uvpitch = src->pitch >> 1;

   dptr = (void *)((char *)dest->image_data + dest->pitch * y_offset);
   yptr = vc_image_get_y(src) + ypitch * src_y_offset;
   uptr = vc_image_get_u(src) + uvpitch * (src_y_offset >> 1);
   vptr = vc_image_get_v(src) + uvpitch * (src_y_offset >> 1);
   dptr += x_offset * 3;
   yptr += src_x_offset;
   uptr += src_x_offset >> 1;
   vptr += src_x_offset >> 1;

   if ((((unsigned long)dptr | dest->pitch | (unsigned long)yptr | ypitch | width) & 31) == 0
         && (height & 15) == 0)
   {
      static const VC_IMAGE_YUV_TO_RGB_MODE_T
      mode =
      {
         yuv420_to_rgb_load_32x16,
         yuv420_to_rgb_load_32x16,
         yuv_to_rgb888_store_32x16,
         yuv_to_rgb888_store_16x16
      };

      if (dest->height == 16)
         vc_image_yuv_to_rgb_stripe(&mode,
                                    dptr, yptr, uptr, vptr, 32*3, dest->pitch,
                                    ypitch, uvpitch, width, 0);
      else
         vc_image_yuv_to_rgb(&mode,
                             dptr, yptr, uptr, vptr, 32*3, dest->pitch,
                             ypitch, uvpitch, ypitch << (3-1), width, height, 0);
   }
   else
   {
      if ((height & 15) > 2 && (height & 1) == 0 && height > 16 && (unsigned long)dptr > 0)
         optimised_tail = 16 - (height & 15);

      while (height >= 16)
      {
         vc_genconv_yuv420_to_rgb888(dptr, dest->pitch, yptr, ypitch,
                                     width, 16, uptr, vptr, uvpitch, src_y_offset & 1);
         dptr = (void *)((char *)dptr + (dest->pitch << 4));
         yptr = (void *)((char *)yptr + (ypitch << 4));
         uptr = (void *)((char *)uptr + (uvpitch << 3));
         vptr = (void *)((char *)vptr + (uvpitch << 3));
         height -= 16;
      }

      if (optimised_tail)
      {
         dptr = (void *)((char *)dptr - dest->pitch * optimised_tail);
         yptr = (void *)((char *)yptr - ypitch * optimised_tail);

         optimised_tail = (optimised_tail + 1) >> 1;
         uptr = (void *)((char *)uptr - uvpitch * optimised_tail);
         vptr = (void *)((char *)vptr - uvpitch * optimised_tail);

         vc_genconv_yuv420_to_rgb888(dptr, dest->pitch, yptr, ypitch,
                                     width, 16, uptr, vptr, uvpitch, src_y_offset & 1);
      }
      else if (height > 0)
      {
         vc_genconv_yuv420_to_rgb888(dptr, dest->pitch, yptr, ypitch,
                                     width, height, uptr, vptr, uvpitch, src_y_offset & 1);
      }
   }
}


/******************************************************************************
NAME
   vc_image_yuv422_to_rgb888_part

SYNOPSIS
   void vc_image_yuv422_to_rgb888_part(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T    const *src,
      int                  src_x_offset,
      int                  src_y_offset)

FUNCTION
   Blit part of a YUV image to an RGB888 image.

RETURNS
   void
******************************************************************************/

void vc_image_yuv422_to_rgb888_part(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset)
{
   static const VC_IMAGE_YUV_TO_RGB_MODE_T
   mode_misaligned =
   {
      yuv422_to_rgb_load_32x16_misaligned,
      yuv422_to_rgb_load_32x16_misaligned,
      yuv_to_rgb888_store_32xn_misaligned,
      yuv_to_rgb888_store_mxn_misaligned
   },
   mode =
   {
      yuv422_to_rgb_load_32x16,
      yuv422_to_rgb_load_32x16,
      yuv_to_rgb888_store_32xn_misaligned,
      yuv_to_rgb888_store_mxn_misaligned
   };
   VC_IMAGE_YUV_TO_RGB_MODE_T const *modeptr = &mode;
   unsigned char *dptr, *yptr, *uptr, *vptr;

   assert(dest != 0 && dest->type == VC_IMAGE_RGB888);
   assert(src != 0 && src->type == VC_IMAGE_YUV422);
   assert(x_offset >= 0 && x_offset + width <= dest->width && src_x_offset >= 0 && src_x_offset + width <= src->width);
   assert(y_offset >= 0 && y_offset + height <= dest->height && src_y_offset >= 0 && src_y_offset + height <= src->height);

   assert(vclib_check_VRF());

#if 1
   width -= x_offset & 1;
   src_x_offset += x_offset & 1;
   x_offset += x_offset & 1;
   width &= ~1;
   src_x_offset &= ~1;
#else
   assert(((x_offset | src_x_offset | width) & 1) == 0);
#endif

   dptr = dest->image_data;
   yptr = vc_image_get_y_422(src);
   uptr = vc_image_get_u_422(src);
   vptr = vc_image_get_v_422(src);

   dptr += y_offset * dest->pitch + x_offset * 3;
   yptr += src_y_offset * src->pitch + src_x_offset;
   src_x_offset >>= 1;
   uptr += src_y_offset * src->pitch + src_x_offset;
   vptr += src_y_offset * src->pitch + src_x_offset;

   if ((((unsigned long)yptr | src->pitch) & 31) != 0)
      modeptr = &mode_misaligned;

   vc_image_yuv_to_rgb_precise(modeptr,
                               dptr, yptr, uptr, vptr,
                               32*3, dest->pitch,
                               src->pitch, src->pitch, src->pitch << 4,
                               width, height, 0);
}





/******************************************************************************
NAME
   vc_image_yuv2rgb888

SYNOPSIS
   void vc_image_yuv2rgb888(VC_IMAGE_T *dest, VC_IMAGE_T *src, int downsample)


FUNCTION
   Convert from YUV to RGB888 with optional downsample.

RETURNS
   void
******************************************************************************/

void vc_image_yuv2rgb888(VC_IMAGE_T *dest, VC_IMAGE_T *src, int downsample) {
#if _VC_VERSION < 2
   unsigned char *Y, *U, *V;
   int src_w, src_h;
   int dest_w, dest_h;
   int dest_pitch = dest->pitch, src_pitch = src->pitch;
   int src_inc = downsample ? 32 : 16;
   Y = vc_image_get_y(src);
   U = vc_image_get_u(src);
   V = vc_image_get_v(src);


   for (src_h = 0, dest_h = 0; src_h < src->height; src_h += src_inc, dest_h += 16) {
      for (src_w = 0, dest_w = 0; src_w < src->width; src_w += src_inc, dest_w += 16) {
         unsigned char *y_ptr = Y + src_h*src_pitch + src_w;
         int uv_off = ((src_h*src_pitch)>>2) + (src_w>>1);
         unsigned char *u_ptr = U + uv_off;
         unsigned char *v_ptr = V + uv_off;
         unsigned char *dest_ptr = (unsigned char *)dest->image_data + dest_h*dest_pitch + dest_w*3;
         if (downsample)
            rgb888_from_yuv_downsampled(y_ptr, u_ptr, v_ptr, src_pitch, dest_ptr, dest_pitch);
         else
            rgb888_from_yuv(y_ptr, u_ptr, v_ptr, src_pitch, dest_ptr, dest_pitch);
      }
   }
#else
   static const VC_IMAGE_YUV_TO_RGB_MODE_T
   mode_misaligned =
   {
      yuv420_to_rgb_load_32x16_misaligned,
      yuv420_to_rgb_load_32x16_misaligned,
      yuv_to_rgb888_store_32x16,
      yuv_to_rgb888_store_16x16
   },
   mode =
   {
      yuv420_to_rgb_load_32x16,
      yuv420_to_rgb_load_32x16,
      yuv_to_rgb888_store_32x16,
      yuv_to_rgb888_store_16x16
   };
   VC_IMAGE_YUV_TO_RGB_MODE_T const *modeptr = &mode;

   assert(downsample == 0);

   if ((((unsigned long)src->image_data | src->pitch) & 31) != 0)
      modeptr = &mode_misaligned;

   if (dest->height <= 16)
      vc_image_yuv_to_rgb_stripe(modeptr,
                                 dest->image_data,
                                 vc_image_get_y(src), vc_image_get_u(src), vc_image_get_v(src),
                                 32*3, dest->pitch,
                                 src->pitch, src->pitch >> 1, dest->width, 0);
   else
      vc_image_yuv_to_rgb(modeptr,
                          dest->image_data,
                          vc_image_get_y(src), vc_image_get_u(src), vc_image_get_v(src),
                          32*3, dest->pitch,
                          src->pitch, src->pitch >> 1, src->pitch << (3-1),
                          dest->width, dest->height, 0);
#endif
}

/******************************************************************************
NAME
   vc_image_convert_yuv422_rgb888

SYNOPSIS
   void vc_image_convert_yuv422_rgb888(VC_IMAGE_T *dest, VC_IMAGE_T *src)


FUNCTION
   Blit from a YUV image to an RGB image.

RETURNS
   void
******************************************************************************/

void vc_image_convert_yuv422_rgb888(VC_IMAGE_T *dest, VC_IMAGE_T *src) {
   static const VC_IMAGE_YUV_TO_RGB_MODE_T
   mode =
   {
      yuv422_to_rgb_load_32x16,
      yuv422_to_rgb_load_32x16,
      yuv_to_rgb888_store_32x16,
      yuv_to_rgb888_store_16x16
   };

   assert((((unsigned long)src->image_data | src->pitch) & 31) == 0);

   if (src->height <= 16)
      vc_image_yuv_to_rgb_stripe(&mode,
                                 dest->image_data,
                                 vc_image_get_y_422(src), vc_image_get_u_422(src), vc_image_get_v_422(src),
                                 32*3, dest->pitch,
                                 src->pitch, src->pitch, dest->width, 0);
   else
      vc_image_yuv_to_rgb(&mode,
                          dest->image_data,
                          vc_image_get_y_422(src), vc_image_get_u_422(src), vc_image_get_v_422(src),
                          32*3, dest->pitch,
                          src->pitch, src->pitch, src->pitch << (4-1),
                          dest->width, dest->height, 0);
}


/******************************************************************************
Static functions definitions.
******************************************************************************/

void proc1_rect (VC_IMAGE_T *im1, int im1_x, int im1_y, int w, int h, proc1_fn *fn, void *arg) {
   int pitch1 = im1->pitch;
   unsigned char *addr1 = (unsigned char *)im1->image_data + pitch1*im1_y + im1_x*3;

   for (; h >= 16; h -= 16, addr1 += (pitch1<<4))
      proc1_stripe(addr1, pitch1, w, 16, fn, arg);

   if (h)
      proc1_stripe(addr1, pitch1, w, h, fn, arg);
}

void proc1_stripe (unsigned char *addr1, int pitch1, int w, int nrows, proc1_fn *fn, void *arg) {
   int off1 = ((int)addr1)&15;
   int i;

   asm("        cbclr\n");
   rgb888_load_initial_block(addr1, pitch1, 0);

   for (; w >= 16; w -= 16) {
      rgb888_load_blocks(addr1, pitch1, 0);
      (*fn)(off1, arg, 16);
      rgb888_save_blocks(addr1, pitch1, nrows);
      addr1 += 48;
      asm("     cbadd3\n");
   }

   if (w) {
      rgb888_load_blocks(addr1, pitch1, 0);
      (*fn)(off1, arg, w);
   }

   for (i = off1+w*3; i > 0; i -= 16) {
      rgb888_save_final_block(addr1, pitch1, nrows);
      addr1 += 16;
      asm("     cbadd1\n");
   }
}

void proc2_rect (VC_IMAGE_T *im1, int im1_x, int im1_y, int w, int h, VC_IMAGE_T *im2, int im2_x, int im2_y, proc2_fn *fn, void *arg) {
   int pitch1 = im1->pitch, pitch2 = im2->pitch;
   unsigned char *addr1 = (unsigned char *)im1->image_data + pitch1*im1_y + im1_x*3;
   unsigned char *addr2 = (unsigned char *)im2->image_data + pitch2*im2_y + im2_x*3;

   for (; h >= 16; h -= 16, addr1 += (pitch1<<4), addr2 += (pitch2<<4))
      proc2_stripe(addr1, pitch1, addr2, pitch2, w, 16, fn, arg);

   if (h)
      proc2_stripe(addr1, pitch1, addr2, pitch2, w, h, fn, arg);
}

void proc2_stripe (unsigned char *addr1, int pitch1, unsigned char *addr2, int pitch2, int w, int nrows, proc2_fn *fn, void *arg) {
   int off1 = ((int)addr1)&15, off2 = ((int)addr2)&15;
   int i;

   asm("        cbclr\n");
   rgb888_load_initial_block(addr1, pitch1, 0);
   rgb888_load_initial_block(addr2, pitch2, 64*16);

   for (; w >= 16; w -= 16) {
      rgb888_load_blocks(addr1, pitch1, 0);
      rgb888_load_blocks(addr2, pitch2, 64*16);
      (*fn)(off1, off2, arg, 16);
      rgb888_save_blocks(addr1, pitch1, nrows);
      addr1 += 48;
      addr2 += 48;
      asm("     cbadd3\n");
   }

   if (w) {
      rgb888_load_blocks(addr1, pitch1, 0);
      rgb888_load_blocks(addr2, pitch2, 64*16);
      (*fn)(off1, off2, arg, w);
   }

   for (i = off1+w*3; i > 0; i -= 16) {
      rgb888_save_final_block(addr1, pitch1, nrows);
      addr1 += 16;
      asm("     cbadd1\n");
   }
}

void proc3_rect (VC_IMAGE_T *im1, int im1_x, int im1_y, int w, int h,
                 VC_IMAGE_T *im2, int im2_x, int im2_y,
                 VC_IMAGE_T *im3, int im3_x, int im3_y, proc3_fn *fn, int invert, void *arg) {
   unsigned char *addr1, *addr2, *addr3;
   int pitch1, pitch2, pitch3;
   int off3;
   pitch1 = im1->pitch;
   pitch2 = im2 ? im2->pitch : 0;
   pitch3 = im3->pitch;
   addr1 = (unsigned char *)im1->image_data + pitch1*im1_y + im1_x*3;
   addr2 = im2 ? (unsigned char *)im2->image_data + pitch2*im2_y + im2_x*3 : NULL;
   addr3 = (unsigned char *)im3->image_data + pitch3*im3_y + (im3_x>>4)*2;
   off3 = im3_x&15;

   for (; h >= 16; h -= 16, addr1 += (pitch1<<4), addr2 += (pitch2<<4), addr3 += (pitch3<<4))
      proc3_stripe(addr1, pitch1, addr2, pitch2, addr3, pitch3, off3, w, 16, fn, invert, arg);

   if (h)
      proc3_stripe(addr1, pitch1, addr2, pitch2, addr3, pitch3, off3, w, h, fn, invert, arg);
}

void proc3_stripe (unsigned char *addr1, int pitch1, unsigned char *addr2, int pitch2,
                   unsigned char *addr3, int pitch3, int off3,
                   int w, int nrows, proc3_fn *fn, int invert, void *arg) {
   int off1 = ((int)addr1)&15, off2 = ((int)addr2)&15;
   int circ3 = 0;
   int i;

   asm("        cbclr\n");
   rgb888_load_initial_block(addr1, pitch1, 0);
   if (addr2)
      rgb888_load_initial_block(addr2, pitch2, 64*16);
   load_block_1bpp(addr3, pitch3, circ3, invert);

   for (; w >= 16; w -= 16) {
      rgb888_load_blocks(addr1, pitch1, 0);
      if (addr2)
         rgb888_load_blocks(addr2, pitch2, 64*16);
      load_block_1bpp(addr3+2, pitch3, circ3^32, invert);
      (*fn)(off1, off2, off3 | circ3, arg, 16);
      rgb888_save_blocks(addr1, pitch1, nrows);
      addr1 += 48;
      if (addr2) addr2 += 48;
      addr3 += 2;
      circ3 ^= 32;
      asm("     cbadd3\n");
   }

   if (w) {
      rgb888_load_blocks(addr1, pitch1, 0);
      if (addr2)
         rgb888_load_blocks(addr2, pitch2, 64*16);
      load_block_1bpp(addr3+2, pitch3, circ3^32, invert);
      (*fn)(off1, off2, off3 | circ3, arg, w);
   }

   for (i = off1+w*3; i > 0; i -= 16) {
      rgb888_save_final_block(addr1, pitch1, nrows);
      addr1 += 16;
      asm("     cbadd1\n");
   }
}

/******************************************************************************
NAME
   vc_image_font_alpha_blt_rgb888

SYNOPSIS
   void vc_image_font_alpha_blt_rgb888(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                        VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int red,int green,int blue,int alpha);

FUNCTION
   Blt a special 4.4 font image generated from a PNG file.
   Only works with a 888 destination image.
   Can choose a colour via red,green,blue arguments.  Each component should be between 0 and 255.
   The alpha value present in the font image is scaled by the alpha argument given to the routine
   Alpha should be between 0 and 256.

RETURNS
   void
******************************************************************************/

void vc_image_font_alpha_blt_rgb888 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                     VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int red,int green,int blue,int alpha)
{
   int dest_pitch_bytes, src_pitch_bytes;
   unsigned char *dest;
   unsigned char *src;

   dest_pitch_bytes = dest_image->pitch;
   src_pitch_bytes = src_image->pitch;
   dest = ((unsigned char *)dest_image->image_data) + y_offset*dest_pitch_bytes + x_offset*3;
   src = ((unsigned char *)src_image->image_data) + src_y_offset*src_pitch_bytes + src_x_offset;

   assert(width<=256); /* Current limitation of the blit function */

   vc_rgb888_font_alpha_blt((void *)dest, src, dest_pitch_bytes, src_pitch_bytes, width, height, red,green,blue,alpha);

}

