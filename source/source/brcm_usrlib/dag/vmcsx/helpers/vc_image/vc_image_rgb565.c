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
#include "vclib/vclib.h"
#include "vc_image.h"
#include "yuv_to_rgb.h"
#include "striperesize.h"
#include "genconv.h"
#include "interface/vcos/vcos_assert.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vc_image_fill_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height, int value);
extern void vc_image_blt_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                    VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset);
extern void vc_image_transparent_blt_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int transparent_colour);
extern void vc_image_overwrite_blt_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int overwrite_colour);
extern void vc_image_masked_blt_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                           VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, VC_IMAGE_T *mask_image, int mask_x_offset, int mask_y_offset, int invert);
extern void vc_image_masked_fill_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height, int value,
         VC_IMAGE_T *mask_image, int mask_x_offset, int mask_y_offset, int invert);
extern void vc_image_not_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height);
extern void vc_image_fade_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height, int fade);
extern void vc_image_or_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                   VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset);
extern void vc_image_xor_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                    VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset);
extern void vc_image_transpose_rgb565 (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image);
extern void vc_image_transpose_tiles_rgb565 (VC_IMAGE_T *dest, VC_IMAGE_T *src, int hdeci);
extern void vc_image_vflip_rgb565 (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image);
extern void vc_image_hflip_rgb565 (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image);
extern void vc_image_transpose_in_place_rgb565 (VC_IMAGE_T *dest_image);
extern void vc_image_vflip_in_place_rgb565 (VC_IMAGE_T *dest_image);
extern void vc_image_hflip_in_place_rgb565 (VC_IMAGE_T *dest_image);
extern void vc_image_text_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                     int *max_x, int *max_y);
extern void vc_image_small_text_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                           int *max_x, int *max_y);
extern void vc_image_text_32_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                        int *max_x, int *max_y);
extern void vc_image_text_24_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                        int *max_x, int *max_y);
extern void vc_image_text_20_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                        int *max_x, int *max_y);
extern void vc_image_convert_to_48bpp_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *src, int src_x_offset, int src_y_offset);
extern void vc_image_convert_yuv_rgb565 (VC_IMAGE_T *dest,VC_IMAGE_T *src,int mirror,int rotate);
extern void vc_image_yuv2rgb565 (VC_IMAGE_T *dest, VC_IMAGE_T *src, int downsample);
extern void vc_image_convert_yuv422_rgb565 (VC_IMAGE_T *dest, VC_IMAGE_T *src);
extern void vc_image_yuv420_to_rgb565_part (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset);
extern void vc_image_yuv422_to_rgb565_part (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset);
extern void vc_image_8bpp_to_rgb565_part (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T  *src, int src_x_offset,
         int src_y_offset, short *pal);
extern void vc_image_3d32_to_rgb565_part (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T *src, int src_x_offset,
         int src_y_offset, short *pal);
extern void vc_image_3d32b_to_rgb565_part (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T *src, int src_x_offset,
         int src_y_offset);
extern void vc_image_transparent_alpha_blt_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int transparent_colour,int alpha);
extern void vc_image_font_alpha_blt_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int red,int green,int blue,int alpha);
extern void vc_image_transparent_alpha_blt_rgb565_from_pal8 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int transparent_colour,int alpha);
extern void vc_image_resize_stripe_h_rgb565 (VC_IMAGE_T *dest, int d_width, int s_width);
extern void vc_image_resize_stripe_v_rgb565 (VC_IMAGE_T *dest, int d_height, int s_height);

/* Check extern defs match above and loads #defines and typedefs */
#include "vc_image_rgb565.h"

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

extern void vc_rgb565_fill_16_rows(unsigned short *dest, int pitch_bytes, int start_offset, int n_blocks, int n_extra_cols, int value);
extern void vc_rgb565_fill_row(unsigned short *dest, int start_offset, int n_blocks, int n_extra_cols, int value);
extern void vc_rgb565_blt_16_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int width);
extern void vc_rgb565_blt_n_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int width, int nrows);
extern void vc_rgb565_masked_blt_16_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int width,
         unsigned char *mask, int mask_pitch_bytes, int mask_offset, int invert);
extern void vc_rgb565_masked_blt_n_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int width,
                                           int nrows, unsigned char *mask, int mask_pitch_bytes, int mask_offset, int invert);
extern void vc_rgb565_masked_fill_16_rows(unsigned short *dest, unsigned char *mask,
         int dest_pitch_bytes, int src_pitch_bytes, int width, int value, int mask_x_offset, int invert);
extern void vc_rgb565_masked_fill_n_rows(unsigned short *dest, unsigned char *mask,
         int dest_pitch_bytes, int src_pitch_bytes, int width, int nrows, int value, int mask_x_offset, int invert);
extern void vc_rgb565_not_16_rows(unsigned short *dest, int pitch_bytes, int start_offset, int n_blocks, int n_extra_cols);
extern void vc_rgb565_not_row(unsigned short *dest, int start_offset, int n_blocks, int n_extra_cols);
extern void vc_rgb565_fade_16_rows(unsigned short *dest, int pitch_bytes, int start_offset, int n_blocks, int n_extra_cols, int fade);
extern void vc_rgb565_fade_row(unsigned short *dest, int start_offset, int n_blocks, int n_extra_cols, int fade);
extern void vc_rgb565_or_16_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int width);
extern void vc_rgb565_or_n_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int width, int nrows);
extern void vc_rgb565_xor_16_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int width);
extern void vc_rgb565_xor_n_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int width, int nrows);
extern void vc_rgb565_transpose(unsigned short *src, unsigned short *dest, int width, int height, int nblocks);
extern void vc_rgb565_transpose_chunky(unsigned short *dst, int dpitch, unsigned short const *src, int spitch);
extern void vc_rgb565_transpose_chunky0(unsigned short *dst, int dpitch, unsigned short const *src, int spitch);
extern void vc_rgb565_transpose_chunky1(unsigned short *dst, int dpitch, unsigned short const *src, int spitch);
extern void vc_rgb565_vflip(unsigned short *dest, unsigned short *src, int src_pitch_bytes, int height, int nblocks, int dest_pitch_bytes);
extern void vc_rgb565_hflip(unsigned short *dest, unsigned short *src, int src_pitch_bytes, int nblocks, int dest_pitch_bytes);
extern void vc_rgb565_transpose_in_place(unsigned short *dest, unsigned short *src, int pitch_bytes, int swap);
extern void vc_rgb565_vflip_in_place(unsigned short *start, unsigned short *end, int pitch_bytes, int nblocks);
extern void vc_rgb565_hflip_in_place_aligned(unsigned short *start, unsigned short *end, int pitch_bytes);
extern void vc_rgb565_hflip_in_place_unaligned(unsigned short *start, unsigned short *end, int pitch_bytes);
extern void vc_rgb565_transparent_blt_16_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes,
         int src_pitch_bytes, int width, int transparent_colour);
extern void vc_rgb565_transparent_blt_n_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes,
         int src_pitch_bytes, int width, int nrows, int transparent_colour);
extern void vc_rgb565_overwrite_blt_16_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes,
         int src_pitch_bytes, int width, int overwrite_colour);
extern void vc_rgb565_overwrite_blt_n_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes,
         int src_pitch_bytes, int width, int nrows, int overwrite_colour);
extern void vc_rgb565_convert_48bpp(unsigned char *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes);
extern void vc_rgb565_from_yuv_downsampled(unsigned char *Y_ptr, unsigned char *U_ptr, unsigned char *V_ptr,
         int src_pitch, unsigned short *rgb, int dest_pitch);

extern void vc_rgb565_transparent_alpha_blt(unsigned short *dest, unsigned short *src, int dest_pitch_bytes,
         int src_pitch_bytes, int width, int height,int transparent_colour,int alpha);

extern void vc_rgb565_font_alpha_blt(unsigned short *dest, unsigned char *src, int dest_pitch_bytes,
                                        int src_pitch_bytes, int width, int height,int red,int green,int blue,int alpha);

extern void vc_rgb565_transparent_alpha_blt_from_pal8(unsigned short *dest, unsigned char *src, int dest_pitch_bytes,
         int src_pitch_bytes, int width, int height,int transparent_colour,int alpha,void *palette,int shift,int mask);

extern void rgb565_from_rgba32_blocky(
      unsigned short      *dest565,    /* r0 */
      int                  dpitch,     /* r1 */
      unsigned long const *srca32,     /* r2 */
      int                  spitch,     /* r3 */
      int                  width,      /* r4 * in pixels */
      int                  height);    /* r5 * in pixels */


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
   vc_image_fill_rgb565

SYNOPSIS
   void vc_image_fill_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int value)

FUNCTION
   Fill given region of an RGB565 image with solid colour value.

RETURNS
   void
******************************************************************************/

void vc_image_fill_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height, int value) {
   int n_blocks_high = height>>4;
   int n_extra_rows = height&15;
   int n_blocks = ( ((x_offset+width)&(~15)) - ((x_offset+15)&(~15)) )>>4;
   int n_extra_cols = (x_offset+width)&15;
   int start_offset = x_offset&15;
   int pitch_bytes, pitch_pixels;
   unsigned short *dest;
   int i;
   vcos_assert(is_valid_vc_image_buf(dest_image, VC_IMAGE_RGB565) || is_valid_vc_image_buf(dest_image, VC_IMAGE_RGBA565) || is_valid_vc_image_buf(dest_image, VC_IMAGE_RGBA16));
   pitch_bytes = dest_image->pitch; // in bytes
   pitch_pixels = pitch_bytes>>1;
   dest = ((unsigned short *)dest_image->image_data) + y_offset*pitch_pixels + x_offset;

   // First fill the image in 16-deep rows.
   for (i = 0; i < n_blocks_high; i++, dest += (pitch_pixels<<4))
      vc_rgb565_fill_16_rows(dest, pitch_bytes, start_offset, n_blocks, n_extra_cols, value);

   // And do any trailing rows.
   for (i = 0; i < n_extra_rows; i++, dest += pitch_pixels)
      vc_rgb565_fill_row(dest, start_offset, n_blocks, n_extra_cols, value);
}

/******************************************************************************
NAME
   vc_image_blt_rgb565

SYNOPSIS
   void vc_image_blt_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                            VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

FUNCTION
   Blt given region of an RGB565 image to another.

RETURNS
   void
******************************************************************************/

void vc_image_blt_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                          VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset) {
   int n_blocks_high = height>>4;
   int n_extra_rows = height&15;
   int dest_pitch_bytes, src_pitch_bytes;
   int dest_pitch_pixels, src_pitch_pixels;
   unsigned short *dest, *src;
   int i;
   vcos_assert(is_valid_vc_image_buf(dest_image, VC_IMAGE_RGB565) || is_valid_vc_image_buf(dest_image, VC_IMAGE_RGBA565) || is_valid_vc_image_buf(dest_image, VC_IMAGE_RGBA16));
   vcos_assert(is_valid_vc_image_buf(src_image, dest_image->type));
   dest_pitch_bytes = dest_image->pitch;
   src_pitch_bytes = src_image->pitch;
   dest_pitch_pixels = dest_pitch_bytes>>1;
   src_pitch_pixels = src_pitch_bytes>>1;
   dest = ((unsigned short *)dest_image->image_data) + y_offset*dest_pitch_pixels + x_offset;
   src = ((unsigned short *)src_image->image_data) + src_y_offset*src_pitch_pixels + src_x_offset;

   // First fill the image in 16-deep rows.
   for (i = 0; i < n_blocks_high; i++, dest += (dest_pitch_pixels<<4), src += (src_pitch_pixels<<4)) {
      // This function deals with arbitrary alignment. We could provide a more
      // efficient function when the x_offset / src_x_offset are the same...
      vc_rgb565_blt_16_rows(dest, src, dest_pitch_bytes, src_pitch_bytes, width);
   }

   // Do any trailing rows.
   if (n_extra_rows)
      vc_rgb565_blt_n_rows(dest, src, dest_pitch_bytes, src_pitch_bytes, width, n_extra_rows);
}

/******************************************************************************
NAME
   vc_image_transparent_blt_rgb565

SYNOPSIS
   void vc_image_transparent_blt_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                        VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int transparent_colour);

FUNCTION
   Blt given region of an RGB565 image to another with a nominated "transparent" colour.

RETURNS
   void
******************************************************************************/

void vc_image_transparent_blt_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                      VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int transparent_colour) {
   int n_blocks_high = height>>4;
   int n_extra_rows = height&15;
   int dest_pitch_bytes, src_pitch_bytes;
   int dest_pitch_pixels, src_pitch_pixels;
   unsigned short *dest, *src;
   int i;
   vcos_assert(is_valid_vc_image_buf(dest_image, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src_image, VC_IMAGE_RGB565));
   dest_pitch_bytes = dest_image->pitch;
   src_pitch_bytes = src_image->pitch;
   dest_pitch_pixels = dest_pitch_bytes>>1;
   src_pitch_pixels = src_pitch_bytes>>1;
   dest = ((unsigned short *)dest_image->image_data) + y_offset*dest_pitch_pixels + x_offset;
   src = ((unsigned short *)src_image->image_data) + src_y_offset*src_pitch_pixels + src_x_offset;

   // First fill the image in 16-deep rows.
   for (i = 0; i < n_blocks_high; i++, dest += (dest_pitch_pixels<<4), src += (src_pitch_pixels<<4)) {
      // This function deals with arbitrary alignment. We could provide a more
      // efficient function when the x_offset / src_x_offset are the same...
      vc_rgb565_transparent_blt_16_rows(dest, src, dest_pitch_bytes, src_pitch_bytes, width, transparent_colour);
   }

   // Do any trailing rows.
   if (n_extra_rows)
      vc_rgb565_transparent_blt_n_rows(dest, src, dest_pitch_bytes, src_pitch_bytes, width, n_extra_rows, transparent_colour);
}

/******************************************************************************
NAME
   vc_image_overwrite_blt_rgb565

SYNOPSIS
   void vc_image_overwrite_blt_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                        VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int overwrite_colour);

FUNCTION
   Blt given region of an RGB565 image to another, but where only a nominate pixel colour
   in the destination gets overwritten.

RETURNS
   void
******************************************************************************/

void vc_image_overwrite_blt_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                    VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int overwrite_colour) {
   int n_blocks_high = height>>4;
   int n_extra_rows = height&15;
   int dest_pitch_bytes, src_pitch_bytes;
   int dest_pitch_pixels, src_pitch_pixels;
   unsigned short *dest, *src;
   int i;
   vcos_assert(is_valid_vc_image_buf(dest_image, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src_image, VC_IMAGE_RGB565));
   dest_pitch_bytes = dest_image->pitch;
   src_pitch_bytes = src_image->pitch;
   dest_pitch_pixels = dest_pitch_bytes>>1;
   src_pitch_pixels = src_pitch_bytes>>1;
   dest = ((unsigned short *)dest_image->image_data) + y_offset*dest_pitch_pixels + x_offset;
   src = ((unsigned short *)src_image->image_data) + src_y_offset*src_pitch_pixels + src_x_offset;

   // First fill the image in 16-deep rows.
   for (i = 0; i < n_blocks_high; i++, dest += (dest_pitch_pixels<<4), src += (src_pitch_pixels<<4)) {
      // This function deals with arbitrary alignment. We could provide a more
      // efficient function when the x_offset / src_x_offset are the same...
      vc_rgb565_overwrite_blt_16_rows(dest, src, dest_pitch_bytes, src_pitch_bytes, width, overwrite_colour);
   }

   // Do any trailing rows.
   if (n_extra_rows)
      vc_rgb565_overwrite_blt_n_rows(dest, src, dest_pitch_bytes, src_pitch_bytes, width, n_extra_rows, overwrite_colour);
}

/******************************************************************************
NAME
   vc_image_masked_blt_rgb565

SYNOPSIS
   void vc_image_masked_blt_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                  VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset,
                                  VC_IMAGE_T *mask_image, int mask_x_offset, int mask_y_offset);

FUNCTION
   Blt given region of an RGB565 image to another, but where only a nominate pixel colour
   in the destination gets overwritten.

RETURNS
   void
******************************************************************************/

void vc_image_masked_blt_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                 VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset,
                                 VC_IMAGE_T *mask_image, int mask_x_offset, int mask_y_offset, int invert) {
   int n_blocks_high = height>>4;
   int n_extra_rows = height&15;
   int dest_pitch_bytes, src_pitch_bytes, mask_pitch_bytes;
   int dest_pitch_pixels, src_pitch_pixels;
   unsigned short *dest, *src;
   unsigned char *mask;
   int i;
   vcos_assert(is_valid_vc_image_buf(dest_image, VC_IMAGE_RGB565) || is_valid_vc_image_buf(dest_image, VC_IMAGE_RGBA565));
   vcos_assert(is_valid_vc_image_buf(src_image, dest_image->type));
   vcos_assert(is_valid_vc_image_buf(mask_image, VC_IMAGE_1BPP));
   dest_pitch_bytes = dest_image->pitch;
   src_pitch_bytes = src_image->pitch;
   mask_pitch_bytes = mask_image->pitch;
   dest_pitch_pixels = dest_pitch_bytes>>1;
   src_pitch_pixels = src_pitch_bytes>>1;
   dest = ((unsigned short *)dest_image->image_data) + y_offset*dest_pitch_pixels + x_offset;
   src = ((unsigned short *)src_image->image_data) + src_y_offset*src_pitch_pixels + src_x_offset;
   mask = ((unsigned char *)mask_image->image_data) + mask_y_offset*mask_pitch_bytes + ((mask_x_offset&(~15))>>3);
   mask_x_offset &= 15;
   if (invert) invert = -1;

   // First mask-blt the image in 16-deep rows.
   for (i = 0; i < n_blocks_high; i++, dest += (dest_pitch_pixels<<4), src += (src_pitch_pixels<<4), mask += (mask_pitch_bytes<<4)) {
      // This function deals with arbitrary alignment. We could provide a more
      // efficient function when the x_offset / src_x_offset are the same...
      vc_rgb565_masked_blt_16_rows(dest, src, dest_pitch_bytes, src_pitch_bytes, width, mask, mask_pitch_bytes, mask_x_offset, invert);
   }

   // Do any trailing rows.
   if (n_extra_rows)
      vc_rgb565_masked_blt_n_rows(dest, src, dest_pitch_bytes, src_pitch_bytes, width, n_extra_rows, mask,
                                  mask_pitch_bytes, mask_x_offset, invert);
}

/******************************************************************************
NAME
   vc_image_masked_fill_rgb565

SYNOPSIS
   void vc_image_masked_fill_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int value,
                                    VC_IMAGE_T *mask, int mask_x_offset, int mask_y_offset);

FUNCTION
   Masked fill of a region of an RGB565 image.

RETURNS
   void
******************************************************************************/

void vc_image_masked_fill_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height, int value,
                                  VC_IMAGE_T *mask_image, int mask_x_offset, int mask_y_offset, int invert) {
   int n_blocks_high = height>>4;
   int n_extra_rows = height&15;
   int dest_pitch_bytes, dest_pitch_pixels, mask_pitch_bytes;
   unsigned short *dest;
   unsigned char *mask;
   int i;
   vcos_assert(is_valid_vc_image_buf(dest_image, VC_IMAGE_RGB565) || is_valid_vc_image_buf(dest_image, VC_IMAGE_RGBA565));
   vcos_assert(is_valid_vc_image_buf(mask_image, VC_IMAGE_1BPP));
   dest_pitch_bytes = dest_image->pitch;
   dest_pitch_pixels = dest_pitch_bytes>>1;
   mask_pitch_bytes = mask_image->pitch;
   dest = ((unsigned short *)dest_image->image_data) + y_offset*dest_pitch_pixels + x_offset;
   // Set mask to the address of the halfword containing the first required pixel.
   mask = ((unsigned char *)mask_image->image_data) + mask_y_offset*mask_pitch_bytes + ((mask_x_offset&(~15))>>3);
   mask_x_offset &= 15;
   if (invert) invert = -1;

   // First fill the image in 16-deep rows.
   for (i = 0; i < n_blocks_high; i++, dest += (dest_pitch_pixels<<4), mask += (mask_pitch_bytes<<4)) {
      // This function deals with arbitrary alignment. We could provide a more
      // efficient function when the x_offset / mask_x_offset are the same...
      vc_rgb565_masked_fill_16_rows(dest, mask, dest_pitch_bytes, mask_pitch_bytes, width, value, mask_x_offset, invert);
   }

   // Do any trailing rows.
   if (n_extra_rows)
      vc_rgb565_masked_fill_n_rows(dest, mask, dest_pitch_bytes, mask_pitch_bytes, width, n_extra_rows, value, mask_x_offset, invert);
}

/******************************************************************************
NAME
   vc_image_not_rgb565

SYNOPSIS
   void vc_image_not_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int value)

FUNCTION
   Not given region of an RGB565 image.

RETURNS
   void
******************************************************************************/

void vc_image_not_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height) {
   int n_blocks_high = height>>4;
   int n_extra_rows = height&15;
   int n_blocks = ( ((x_offset+width)&(~15)) - ((x_offset+15)&(~15)) )>>4;
   int n_extra_cols = (x_offset+width)&15;
   int start_offset = x_offset&15;
   int pitch_bytes, pitch_pixels;
   unsigned short *dest;
   int i;
   vcos_assert(is_valid_vc_image_buf(dest_image, VC_IMAGE_RGB565));
   pitch_bytes = dest_image->pitch;
   pitch_pixels = pitch_bytes>>1;
   dest = ((unsigned short *)dest_image->image_data) + y_offset*pitch_pixels + x_offset;

   // First not the image in 16-deep rows.
   for (i = 0; i < n_blocks_high; i++, dest += (pitch_pixels<<4))
      vc_rgb565_not_16_rows(dest, pitch_bytes, start_offset, n_blocks, n_extra_cols);

   // And do any trailing rows.
   for (i = 0; i < n_extra_rows; i++, dest += pitch_pixels)
      vc_rgb565_not_row(dest, start_offset, n_blocks, n_extra_cols);
}

/******************************************************************************
NAME
   vc_image_fade_rgb565

SYNOPSIS
   void vc_image_fade_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, int value, int fade)

FUNCTION
   Fade the given region of an RGB565 image. fade is an 8.8 value, so values > 256 will brighten the region!

RETURNS
   void
******************************************************************************/

void vc_image_fade_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height, int fade) {
   int n_blocks_high = height>>4;
   int n_extra_rows = height&15;
   int n_blocks = ( ((x_offset+width)&(~15)) - ((x_offset+15)&(~15)) )>>4;
   int n_extra_cols = (x_offset+width)&15;
   int start_offset = x_offset&15;
   int pitch_bytes, pitch_pixels;
   unsigned short *dest;
   int i;
   vcos_assert(is_valid_vc_image_buf(dest_image, VC_IMAGE_RGB565));
   pitch_bytes = dest_image->pitch;
   pitch_pixels = pitch_bytes>>1;
   dest = ((unsigned short *)dest_image->image_data) + y_offset*pitch_pixels + x_offset;

   // First not the image in 16-deep rows.
   for (i = 0; i < n_blocks_high; i++, dest += (pitch_pixels<<4))
      vc_rgb565_fade_16_rows(dest, pitch_bytes, start_offset, n_blocks, n_extra_cols, fade);

   // And do any trailing rows.
   for (i = 0; i < n_extra_rows; i++, dest += pitch_pixels)
      vc_rgb565_fade_row(dest, start_offset, n_blocks, n_extra_cols, fade);
}

/******************************************************************************
NAME
   vc_image_or_rgb565

SYNOPSIS
   void vc_image_or_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                           VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

FUNCTION
   OR two RGB565 images together.

RETURNS
   void
******************************************************************************/

void vc_image_or_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                         VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset) {
   int n_blocks_high = height>>4;
   int n_extra_rows = height&15;
   int dest_pitch_bytes, src_pitch_bytes;
   int dest_pitch_pixels, src_pitch_pixels;
   unsigned short *dest, *src;
   int i;
   vcos_assert(is_valid_vc_image_buf(dest_image, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src_image, VC_IMAGE_RGB565));
   dest_pitch_bytes = dest_image->pitch;
   dest_pitch_pixels = dest_pitch_bytes>>1;
   src_pitch_bytes = src_image->pitch;
   src_pitch_pixels = src_pitch_bytes>>1;
   dest = ((unsigned short *)dest_image->image_data) + y_offset*dest_pitch_pixels + x_offset;
   src = ((unsigned short *)src_image->image_data) + src_y_offset*src_pitch_pixels + src_x_offset;

   // First fill the image in 16-deep rows.
   for (i = 0; i < n_blocks_high; i++, dest += (dest_pitch_pixels<<4), src += (src_pitch_pixels<<4)) {
      // This function deals with arbitrary alignment. We could provide a more
      // efficient function when the x_offset / src_x_offset are the same...
      vc_rgb565_or_16_rows(dest, src, dest_pitch_bytes, src_pitch_bytes, width);
   }

   // Do any trailing rows.
   if (n_extra_rows)
      vc_rgb565_or_n_rows(dest, src, dest_pitch_bytes, src_pitch_bytes, width, n_extra_rows);
}

/******************************************************************************
NAME
   vc_image_xor_rgb565

SYNOPSIS
   void vc_image_xor_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                            VC_IMAGE_T *src, int src_x_offset, int src_y_offset);

FUNCTION
   XOR two RGB565 images together.

RETURNS
   void
******************************************************************************/

void vc_image_xor_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                          VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset) {
   int n_blocks_high = height>>4;
   int n_extra_rows = height&15;
   int dest_pitch_bytes, src_pitch_bytes;
   int dest_pitch_pixels, src_pitch_pixels;
   unsigned short *dest, *src;
   int i;
   vcos_assert(is_valid_vc_image_buf(dest_image, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src_image, VC_IMAGE_RGB565));
   dest_pitch_bytes = dest_image->pitch;
   dest_pitch_pixels = dest_pitch_bytes>>1;
   src_pitch_bytes = src_image->pitch;
   src_pitch_pixels = src_pitch_bytes>>1;
   dest = ((unsigned short *)dest_image->image_data) + y_offset*dest_pitch_pixels + x_offset;
   src = ((unsigned short *)src_image->image_data) + src_y_offset*src_pitch_pixels + src_x_offset;

   // First fill the image in 16-deep rows.
   for (i = 0; i < n_blocks_high; i++, dest += (dest_pitch_pixels<<4), src += (src_pitch_pixels<<4)) {
      // This function deals with arbitrary alignment. We could provide a more
      // efficient function when the x_offset / src_x_offset are the same...
      vc_rgb565_xor_16_rows(dest, src, dest_pitch_bytes, src_pitch_bytes, width);
   }

   // Do any trailing rows.
   if (n_extra_rows)
      vc_rgb565_xor_n_rows(dest, src, dest_pitch_bytes, src_pitch_bytes, width, n_extra_rows);
}

/******************************************************************************
NAME
   vc_image_transpose_rgb565

SYNOPSIS
   void vc_image_transpose_rgb565(VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image)

FUNCTION
   Transpose.

RETURNS
   void
******************************************************************************/

void vc_image_transpose_rgb565 (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image) {
   int src_pitch_bytes = src_image->pitch;
   int src_pitch_pixels = src_pitch_bytes>>1;
   int dest_pitch_bytes = dest_image->pitch;
   int block_width = (src_image->width+15)>>4;
   int height = src_image->height;
   unsigned short *src = (unsigned short *)src_image->image_data;
   unsigned short *dest = (unsigned short *)dest_image->image_data;
   int i;

   for (i = 0; i < height; i += 16, src += (src_pitch_pixels<<4), dest += 16) {
      vc_rgb565_transpose(src, dest, src_pitch_bytes, dest_pitch_bytes, block_width);
   }
}

/******************************************************************************
NAME
   vc_image_transpose_tiles_rgb565

SYNOPSIS
   void vc_image_transpose_tiles_rgb565(VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image)

FUNCTION
   Transpose.

RETURNS
   void
******************************************************************************/

void vc_image_transpose_tiles_rgb565 (VC_IMAGE_T *dest, VC_IMAGE_T *src, int hdeci)
{
   void (*func)(unsigned short *dst, int dpitch, unsigned short const *src, int hpitch) = vc_rgb565_transpose_chunky;
   unsigned short *dptr;
   unsigned short const *sptr;
   int dst_pitch, dst_width, dst_height;
   int chunksize, x_offset, i;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB565) || is_valid_vc_image_buf(dest, VC_IMAGE_RGBA565));
   vcos_assert(is_valid_vc_image_buf(src, dest->type));
   vcos_assert(vclib_check_VRF());

   chunksize = 128;

   if (hdeci & 2)
   {
      if (hdeci == 2)
         func = vc_rgb565_transpose_chunky0;
      else
         func = vc_rgb565_transpose_chunky1;

      hdeci &= ~3;
      chunksize >>= 1;
      vcos_assert(src->width == 32);
   }
   else
      vcos_assert(src->width == 16);
   vcos_assert(hdeci == 0);

   dst_pitch = (src->pitch * chunksize) >> 4;
   dst_width = (dst_pitch >> 1) & -chunksize;
   dst_height = (src->height + dst_width - 1) / dst_width << 4;

   vcos_assert(dest->size >= dst_pitch * dst_height);

   dptr = dest->image_data;
   sptr = src->image_data;
   x_offset = 0;

   for (i = src->height; i > 0; i -= chunksize)
   {
      func(dptr + x_offset, dst_pitch, sptr, src->pitch);
      if ((x_offset += chunksize) >= dst_width)
      {
         x_offset = 0;
         dptr = (void *)((char *)dptr + (dst_pitch << 4));
      }
      sptr = (void *)((char *)sptr + src->pitch * chunksize);
   }

   dest->width = dst_width;
   dest->pitch = dst_pitch;
   dest->height = dst_height;
}

/******************************************************************************
NAME
   vc_image_vflip_rgb565

SYNOPSIS
   void vc_image_vflip_rgb565(VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image)

FUNCTION
   Vertically flip an image.

RETURNS
   void
******************************************************************************/

void vc_image_vflip_rgb565 (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image) {
   int src_pitch_bytes = src_image->pitch;
   int dest_pitch_bytes = dest_image->pitch;
   int height = src_image->height;
   int nblocks = (src_image->width+15)>>4;
   unsigned short *dest = (unsigned short *)dest_image->image_data;
   unsigned short *src = (unsigned short *)src_image->image_data + (height-1) * (src_pitch_bytes>>1);

   vc_rgb565_vflip(dest, src, src_pitch_bytes, height, nblocks, dest_pitch_bytes);
}

/******************************************************************************
NAME
   vc_image_hflip_rgb565

SYNOPSIS
   void vc_image_hflip_rgb565(VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image)

FUNCTION
   Horizontally flip an image.

RETURNS
   void
******************************************************************************/

void vc_image_hflip_rgb565 (VC_IMAGE_T *dest_image, VC_IMAGE_T *src_image) {
   int src_pitch_bytes = src_image->pitch;
   int src_pitch_pixels16 = src_pitch_bytes<<3;
   int dest_pitch_bytes = dest_image->pitch;
   int dest_pitch_pixels16 = dest_pitch_bytes<<3;
   int height = src_image->height;
   int nblocks = (src_image->width+15)>>4;
   int i;
   unsigned short *dest = (unsigned short *)dest_image->image_data;
   unsigned short *src = (unsigned short *)src_image->image_data + src_image->width;

   for (i = 0; i < height; i += 16, dest += dest_pitch_pixels16, src += src_pitch_pixels16) {
      vc_rgb565_hflip(dest, src, src_pitch_bytes, nblocks, dest_pitch_bytes);
   }
}

/******************************************************************************
NAME
   vc_image_transpose_in_place_rgb565

SYNOPSIS
   void vc_image_transpose_in_place_rgb565(VC_IMAGE_T *dest_image)

FUNCTION
   Transpose.

RETURNS
   void
******************************************************************************/

void vc_image_transpose_in_place_rgb565 (VC_IMAGE_T *dest_image) {
   // To work in place we have to preserve the image pitch.
   int hblocks = (dest_image->height+15)>>4;
   int wblocks = (dest_image->width+15)>>4;
   int pitch_bytes = dest_image->pitch;
   int pitch_pixels = pitch_bytes>>1;
   int pitch_pixels16 = pitch_pixels<<4;
   int h, w;
   unsigned short *src, *dest;

   vcos_assert(dest_image->pitch >= (dest_image->height<<1));

   if (wblocks >= hblocks) {
      for (h = 0; h < hblocks; h++) {
         src = (unsigned short *)dest_image->image_data + h*pitch_pixels16 + (h<<4);
         dest = src;
         vc_rgb565_transpose_in_place(dest, src, pitch_bytes, 0);
         src += 16;
         dest += pitch_pixels16;
         for (w = h+1; w < hblocks; w++, src += 16, dest += pitch_pixels16)
            vc_rgb565_transpose_in_place(dest, src, pitch_bytes, 1);
         for (; w < wblocks; w++, src += 16, dest += pitch_pixels16)
            vc_rgb565_transpose_in_place(dest, src, pitch_bytes, 0);
      }
   }
   else {
      for (w = 0; w < wblocks; w++) {
         src = (unsigned short *)dest_image->image_data + w*pitch_pixels16 + (w<<4);
         dest = src;
         vc_rgb565_transpose_in_place(dest, src, pitch_bytes, 0);
         src += pitch_pixels16;
         dest += 16;
         for (h = w+1; h < wblocks; h++, src += pitch_pixels16, dest += 16)
            vc_rgb565_transpose_in_place(dest, src, pitch_bytes, 1);
         for (; h < hblocks; h++, src += pitch_pixels16, dest += 16)
            vc_rgb565_transpose_in_place(dest, src, pitch_bytes, 0);
      }
   }
}

/******************************************************************************
NAME
   vc_image_vflip_in_place_rgb565

SYNOPSIS
   void vc_image_vflip_in_place_rgb565(VC_IMAGE_T *dest_image)

FUNCTION
   Vertically flip an image.

RETURNS
   void
******************************************************************************/

void vc_image_vflip_in_place_rgb565 (VC_IMAGE_T *dest_image) {
   int pitch_bytes = dest_image->pitch;
   int height = dest_image->height;
   int nblocks = (dest_image->width+15)>>4;
   unsigned short *start = (unsigned short *)dest_image->image_data;
   unsigned short *end = (unsigned short *)dest_image->image_data + (height-1) * (pitch_bytes>>1);

   vc_rgb565_vflip_in_place(start, end, pitch_bytes, nblocks);
}

/******************************************************************************
NAME
   vc_image_hflip_in_place_rgb565

SYNOPSIS
   void vc_image_hflip_in_place_rgb565(VC_IMAGE_T *dest_image)

FUNCTION
   Horizontally flip an image.

RETURNS
   void
******************************************************************************/

void vc_image_hflip_in_place_rgb565 (VC_IMAGE_T *dest_image) {
   int pitch_bytes = dest_image->pitch;
   int pitch_pixels = pitch_bytes>>1;
   int pitch_pixels16 = pitch_pixels<<4;
   int width = dest_image->width;
   int height = dest_image->height;
   unsigned short *start = (unsigned short *)dest_image->image_data;
   unsigned short *end = (unsigned short *)dest_image->image_data + width - 16;
   int h;

   if ((width&15) == 0) {
      // Aligned case. Easy!
      for (h = 0; h < height; h += 16, start += pitch_pixels16, end += pitch_pixels16)
         vc_rgb565_hflip_in_place_aligned(start, end, pitch_bytes);
   }
   else {
      // Unaligned case.
      for (h = 0; h < height; h += 16, start += pitch_pixels16, end += pitch_pixels16)
         vc_rgb565_hflip_in_place_unaligned(start, end, pitch_bytes);
   }
}

/******************************************************************************
NAME
   vc_image_text_rgb565

SYNOPSIS
   void vc_image_text_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, char *text,
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

void vc_image_text_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                           int *max_x, int *max_y) {
   int pitch = dest->pitch>>1; // in pixels
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
         unsigned short *b;
         int data;
         if (sx < 0) continue;

         /* Needs to be displayed at location sx,sy */
         if (fg_colour == VC_IMAGE_COLOUR_TRANSPARENT)
            continue;

         b = (unsigned short *)dest->image_data + sy*pitch + sx;
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
                  b[0]=(b[0]&(0xffff-(1+(1<<5)+(1<<11))) )>>1;
               }
               else
                  b[0] = bg_colour;
            }
            else {
               b[0] = fg_colour;
            }
         }
      }
   }
   *max_x = sx;
}

void vc_image_small_text_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                 int *max_x, int *max_y) {
   int pitch = dest->pitch>>1; // in pixels
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
         unsigned short *b;
         int data;

         if (sx < 0) continue;

         /* Needs to be displayed at location sx,sy */
         if (fg_colour == VC_IMAGE_COLOUR_TRANSPARENT)
            continue;

         b = (unsigned short *)dest->image_data + sy*pitch + sx;
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
                  b[0]=(b[0]&(0xffff-(1+(1<<5)+(1<<11))) )>>1;
               }
               else
                  b[0] = bg_colour;
            }
            else {
               b[0] = fg_colour;
            }
         }
      }
   }
   *max_x = sx;
}

void vc_image_text_32_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                              int *max_x, int *max_y) {
   int pitch = dest->pitch>>1; // in pixels
   int start, end;
   int y;
   int sx = x_offset, sy = y_offset;

   *max_y = min(y_offset+32, dest->height)-1;

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
         unsigned short *b;
         int data;
         if (sx < 0) continue;

         /* Needs to be displayed at location sx,sy */
         if (fg_colour == VC_IMAGE_COLOUR_TRANSPARENT)
            continue;

         b = (unsigned short *)dest->image_data + sy*pitch + sx;
         data = fontdata[start];  /* This contains packed binary for the letter */

         for ( y = sy; y <= *max_y; y++, data <<= 1, b += pitch) {
            if (y < 0) continue;

            if (data&(1<<31)) {
               // one means background
               if (bg_colour == VC_IMAGE_COLOUR_TRANSPARENT) {
                  // do nothing
               }
               else if (bg_colour == VC_IMAGE_COLOUR_FADE) {
                  // wipe out bottom bits and shift down
                  b[0]=(b[0]&(0xffff-(1+(1<<5)+(1<<11))) )>>1;
               }
               else
                  b[0] = bg_colour;
            }
            else {
               b[0] = fg_colour;
            }
         }
      }
   }
   *max_x = sx;
}

void vc_image_text_24_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                              int *max_x, int *max_y) {
   int pitch = dest->pitch>>1; // in pixels
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
         unsigned short *b;
         int data;
         if (sx < 0) continue;

         /* Needs to be displayed at location sx,sy */
         if (fg_colour == VC_IMAGE_COLOUR_TRANSPARENT)
            continue;

         b = (unsigned short *)dest->image_data + sy*pitch + sx;
         data = fontdata24[start];  /* This contains packed binary for the letter */

         for ( y = sy; y <= *max_y; y++, data <<= 1, b += pitch) {
            if (y < 0) continue;

            if (data&(1<<15)) {
               // one means background
               if (bg_colour == VC_IMAGE_COLOUR_TRANSPARENT) {
                  // do nothing
               }
               else if (bg_colour == VC_IMAGE_COLOUR_FADE) {
                  // wipe out bottom bits and shift down
                  b[0]=(b[0]&(0xffff-(1+(1<<5)+(1<<11))) )>>1;
               }
               else
                  b[0] = bg_colour;
            }
            else {
               b[0] = fg_colour;
            }
         }
      }
   }
   *max_x = sx;
}

void vc_image_text_20_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                              int *max_x, int *max_y) {
   int pitch = dest->pitch>>1; // in pixels
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
         unsigned short *b;
         int data;
         if (sx < 0) continue;

         /* Needs to be displayed at location sx,sy */
         if (fg_colour == VC_IMAGE_COLOUR_TRANSPARENT)
            continue;

         b = (unsigned short *)dest->image_data + sy*pitch + sx;
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
                  b[0]=(b[0]&(0xffff-(1+(1<<5)+(1<<11))) )>>1;
               }
               else
                  b[0] = bg_colour;
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
   vc_image_convert_to_48bpp_rgb565

SYNOPSIS
   void vc_image_convert_to_48bpp_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                         VC_IMAGE_T *src, int src_x_offset, int src_y_offset)

FUNCTION
   Convert region of one bitmap to 48bpp and copy into a 48bpp destination.
   WARNING: all offsets, width and height MUST be 16-pixel aligned.

RETURNS
   void
******************************************************************************/

void vc_image_convert_to_48bpp_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                       VC_IMAGE_T *src, int src_x_offset, int src_y_offset) {
   int src_pitch_bytes = src->pitch;
   int src_pitch_shorts = src_pitch_bytes>>1;
   int dest_pitch_bytes = dest->pitch;
   unsigned short *src_ptr = (unsigned short *)src->image_data + y_offset * src_pitch_shorts + src_x_offset;
   unsigned char *dest_ptr = (unsigned char *)dest->image_data + y_offset * dest_pitch_bytes + x_offset*6;
   int w, h;

   for (h = 0; h < height; h += 16, src_ptr += src_pitch_shorts<<4, dest_ptr += dest_pitch_bytes<<4) {
      unsigned short *s = src_ptr;
      unsigned char *d = dest_ptr;
      for (w = 0; w < width; w += 16, s += 16, d += 96) {
         vc_rgb565_convert_48bpp(d, s, dest_pitch_bytes, src_pitch_bytes);
      }
   }
}

/******************************************************************************
NAME
   vc_image_convert_yuv_rgb565

SYNOPSIS
   void vc_image_convert_yuv_rgb565(VC_IMAGE_T *dest,VC_IMAGE_T *src,int mirror,int rotate)


FUNCTION
   Blit from a YUV image to an RGB image with optional mirroring followed
   by optional clockwise rotation

RETURNS
   void
******************************************************************************/

void vc_image_convert_yuv_rgb565(VC_IMAGE_T *dest,VC_IMAGE_T *src,int mirror,int rotate)
{
   unsigned char * Y, * U, * V;
   unsigned short * buffer;
   int width=(src->width+15)&(~15);
   int height=src->height;
   int cols=dest->pitch>>1;
   int lines=dest->height;

   Y = vc_image_get_y(src);
   U = vc_image_get_u(src);
   V = vc_image_get_v(src);
   buffer = dest->image_data;

   if (rotate)
   {
      if (mirror)
      {
         //vc_image_convert_yuv2rgb_rotflip(Y, U, V, width, height,
                                       //buffer, cols, lines);
         vcos_assert(!"unsupported mode");
      }
      else
      {
         //vc_image_convert_yuv2rgb_rotated(Y, width, height,
                                       //buffer, cols, lines);
         vcos_assert(!"unsupported mode");
      }
   }
   else
   {
      if (mirror)
      {
         vc_image_convert_yuv2rgb(Y, U, V, width, height,
                               buffer, cols, lines);
      }
      else
         vc_image_yuv2rgb565(dest, src, 0);
   }

}

/******************************************************************************
NAME
   vc_image_yuv2rgb565

SYNOPSIS
   void vc_image_yuv2rgb565(VC_IMAGE_T *dest, VC_IMAGE_T *src, int downsample)


FUNCTION
   Convert from YUV to RGB565 with optional 2x downsample in each direction.

RETURNS
   void
******************************************************************************/

void vc_image_yuv2rgb565 (VC_IMAGE_T *dest, VC_IMAGE_T *src, int downsample) {
   unsigned char *Y, *U, *V;
   unsigned short *rgb;
   int width = (src->width+15)&(~15);
   int height = src->height;

   Y = vc_image_get_y(src);
   U = vc_image_get_u(src);
   V = vc_image_get_v(src);
   rgb = (unsigned short *)dest->image_data;

   if (downsample) {
      int w, h;
      for (h = 0; h < height; h += 32) {
         unsigned char *Y_ptr = Y, *U_ptr = U, *V_ptr = V;
         unsigned short *rgb_ptr = rgb;
         for (w = 0; w < width; w += 32) {
            vc_rgb565_from_yuv_downsampled(Y_ptr, U_ptr, V_ptr, src->pitch, rgb_ptr, dest->pitch);
            Y_ptr += 32;
            U_ptr += 16;
            V_ptr += 16;
            rgb_ptr += 16;
         }
         Y += 32*src->pitch;
         U += 8*src->pitch;
         V += 8*src->pitch;
         rgb += 8*dest->pitch; // remember pitch is in bytes
      }
   }
   else {
#if (_VC_VERSION < 2)
      vc_image_convert_yuv2rgb_identity(Y, U, V, width, height, src->pitch,
                                     rgb, dest->pitch>>1, height);
#else
      static const VC_IMAGE_YUV_TO_RGB_MODE_T
      mode_misaligned =
      {
         yuv420_to_rgb_load_32x16_misaligned,
         yuv420_to_rgb_load_32x16_misaligned,
         yuv_to_rgb565_store_32x16_default,
         yuv_to_rgb565_store_16x16_default
      },
      mode =
      {
         yuv420_to_rgb_load_32x16,
         yuv420_to_rgb_load_32x16,
         yuv_to_rgb565_store_32x16_default,
         yuv_to_rgb565_store_16x16_default
      };
      VC_IMAGE_YUV_TO_RGB_MODE_T const *modeptr = &mode;

      width = (dest->width+15)&(~15);
      height = dest->height;
      if (width > src->width)
         width = src->width+15&~15;
      if (height > src->height)
         height = src->height;

      if ((((unsigned long)Y | src->pitch) & 31) != 0)
         modeptr = &mode_misaligned;

      if (height <= 16)
         vc_image_yuv_to_rgb_stripe(modeptr,
                                    (void *)rgb, Y, U, V,
                                    32*2, dest->pitch,
                                    src->pitch, src->pitch >> 1, width, 0);
      else
         vc_image_yuv_to_rgb(modeptr,
                             (void *)rgb, Y, U, V,
                             32*2, dest->pitch,
                             src->pitch, src->pitch >> 1, src->pitch << (3-1),
                             width, height, 0);
#endif
   }
}


/******************************************************************************
NAME
   vc_image_convert_yuv422_rgb565

SYNOPSIS
   void vc_image_convert_yuv422_rgb888(VC_IMAGE_T *dest, VC_IMAGE_T *src)

FUNCTION
   Blit from a YUV image to an RGB image.

RETURNS
   void
******************************************************************************/

void vc_image_convert_yuv422_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T *src) {
   static const VC_IMAGE_YUV_TO_RGB_MODE_T
   mode =
   {
      yuv422_to_rgb_load_32x16,
      yuv422_to_rgb_load_32x16,
      yuv_to_rgb565_store_32x16_default,
      yuv_to_rgb565_store_16x16_default
   };

   vcos_assert((((unsigned long)src->image_data | src->pitch) & 31) == 0);

   if (src->height <= 16)
      vc_image_yuv_to_rgb_stripe(&mode,
                                 dest->image_data,
                                 vc_image_get_y_422(src), vc_image_get_u_422(src), vc_image_get_v_422(src),
                                 32*2, dest->pitch,
                                 src->pitch, src->pitch, dest->width, 0);
   else
      vc_image_yuv_to_rgb(&mode,
                          dest->image_data,
                          vc_image_get_y_422(src), vc_image_get_u_422(src), vc_image_get_v_422(src),
                          32*2, dest->pitch,
                          src->pitch, src->pitch, src->pitch << (4-1),
                          dest->width, dest->height, 0);
}


/******************************************************************************
NAME
   vc_image_yuv420_to_rgb565_part

SYNOPSIS
   void vc_image_yuv422_to_rgb565_part(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T    const *src,
      int                  src_x_offset,
      int                  src_y_offset)

FUNCTION
   Blit part of a YUV image to an RGB565 image.

RETURNS
   void
******************************************************************************/

void vc_image_yuv420_to_rgb565_part(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset)
{
   static const VC_IMAGE_YUV_TO_RGB_MODE_T
   mode_misaligned =
   {
      yuv420_to_rgb_load_32x16_misaligned,
      yuv420_to_rgb_load_32x16_misaligned,
      yuv_to_rgb565_store_32xn_misaligned_default,
      yuv_to_rgb565_store_mxn_misaligned_default
   },
   mode =
   {
      yuv420_to_rgb_load_32x16,
      yuv420_to_rgb_load_32x16,
      yuv_to_rgb565_store_32xn_misaligned_default,
      yuv_to_rgb565_store_mxn_misaligned_default
   };
   VC_IMAGE_YUV_TO_RGB_MODE_T const *modeptr = &mode;
   unsigned char *dptr, *yptr, *uptr, *vptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV420));
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height,  0, 0, dest->width, dest->height));
   vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height,  0, 0, src->width, src->height));
   vcos_assert(vclib_check_VRF());

#if 1
   width -= src_x_offset & 1;
   x_offset += src_x_offset & 1;
   src_x_offset += src_x_offset & 1;
   height -= src_y_offset & 1;
   y_offset += src_y_offset & 1;
   src_y_offset += src_y_offset & 1;
#else
   vcos_assert(((src_x_offset | width) & 1) == 0);
   vcos_assert(((src_y_offset | height) & 1) == 0);
#endif

   dptr = dest->image_data;
   yptr = vc_image_get_y(src);
   uptr = vc_image_get_u(src);
   vptr = vc_image_get_v(src);

   dptr += y_offset * dest->pitch + x_offset * 2;
   yptr += src_y_offset * src->pitch + src_x_offset;
   src_x_offset >>= 1;
   src_y_offset >>= 1;
   uptr += src_y_offset * (src->pitch >> 1) + src_x_offset;
   vptr += src_y_offset * (src->pitch >> 1) + src_x_offset;

   if ((((unsigned long)yptr | src->pitch) & 31) != 0)
      modeptr = &mode_misaligned;

   vc_image_yuv_to_rgb_precise(modeptr,
                               dptr, yptr, uptr, vptr,
                               32*2, dest->pitch,
                               src->pitch, src->pitch >> 1, src->pitch << (3-1),
                               width, height, 0);
}


/******************************************************************************
NAME
   vc_image_yuv422_to_rgb565_part

SYNOPSIS
   void vc_image_yuv422_to_rgb565_part(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T    const *src,
      int                  src_x_offset,
      int                  src_y_offset)

FUNCTION
   Blit part of a YUV image to an RGB565 image.

RETURNS
   void
******************************************************************************/

void vc_image_yuv422_to_rgb565_part(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset)
{
   static const VC_IMAGE_YUV_TO_RGB_MODE_T
   mode_misaligned =
   {
      yuv422_to_rgb_load_32x16_misaligned,
      yuv422_to_rgb_load_32x16_misaligned,
      yuv_to_rgb565_store_32xn_misaligned_default,
      yuv_to_rgb565_store_mxn_misaligned_default
   },
   mode =
   {
      yuv422_to_rgb_load_32x16,
      yuv422_to_rgb_load_32x16,
      yuv_to_rgb565_store_32xn_misaligned_default,
      yuv_to_rgb565_store_mxn_misaligned_default
   };
   VC_IMAGE_YUV_TO_RGB_MODE_T const *modeptr = &mode;
   unsigned char *dptr, *yptr, *uptr, *vptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV422));
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height,  0, 0, dest->width, dest->height));
   vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height,  0, 0, src->width, src->height));

   vcos_assert(vclib_check_VRF());

#if 1
   width -= src_x_offset & 1;
   x_offset += src_x_offset & 1;
   src_x_offset += src_x_offset & 1;
#else
   vcos_assert(((src_x_offset | width) & 1) == 0);
#endif

   dptr = dest->image_data;
   yptr = vc_image_get_y_422(src);
   uptr = vc_image_get_u_422(src);
   vptr = vc_image_get_v_422(src);

   dptr += y_offset * dest->pitch + x_offset * 2;
   yptr += src_y_offset * src->pitch + src_x_offset;
   src_x_offset >>= 1;
   uptr += src_y_offset * src->pitch + src_x_offset;
   vptr += src_y_offset * src->pitch + src_x_offset;

   if ((((unsigned long)yptr | src->pitch) & 31) != 0)
      modeptr = &mode_misaligned;

   vc_image_yuv_to_rgb_precise(modeptr,
                               dptr, yptr, uptr, vptr,
                               32*2, dest->pitch,
                               src->pitch, src->pitch, src->pitch << 4,
                               width, height, 0);
}

/******************************************************************************
NAME
   vc_image_8bpp_to_rgb565_part

SYNOPSIS
   void vc_image_8bpp_to_rgb565_part(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T          *src,
      int                  src_x_offset,
      int                  src_y_offset);

FUNCTION
   Convert part of an 8bpp image to RGB565.

RETURNS
   void
******************************************************************************/

void vc_image_8bpp_to_rgb565_part(
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

   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_8BPP) && is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));

   dpitch = dest->pitch;
   spitch = src->pitch;
   dptr = dest->image_data;
   sptr = src->image_data;

   if ((height & 15) > 2 && height > 16 && (unsigned long)dptr > 0)
      optimised_tail = 16 - (height & 15);


   dptr += x_offset * 2 + y_offset * dpitch;
   sptr = (void *)((char *)sptr + src_y_offset * spitch);
   sptr += src_x_offset;

   if (height >= 16)
      do
      {
         vc_genconv_8bpp_to_rgb565(dptr, dpitch, sptr, spitch, width, 16, pal);
         dptr += dpitch << 4;
         sptr += spitch << 4;
         height -= 16;
      } while (height >= 16);

   if (optimised_tail)
   {
      dptr -= dpitch * optimised_tail;
      sptr -= spitch * optimised_tail;
      vc_genconv_8bpp_to_rgb565(dptr, dpitch, sptr, spitch, width, 16, pal);
   }
   else if (height > 0)
      vc_genconv_8bpp_to_rgb565(dptr, dpitch, sptr, spitch, width, height, pal);
}

/******************************************************************************
NAME
   vc_image_3d32_to_rgb565_part

SYNOPSIS
   void vc_image_3d32_to_rgb565_part(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T          *src,
      int                  src_x_offset,
      int                  src_y_offset);

FUNCTION
   Convert part of an 3d32 image to RGB565.

RETURNS
   void
******************************************************************************/

void vc_image_3d32_to_rgb565_part(
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

   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_3D32) && is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));

   dpitch = dest->pitch;
   spitch = src->pitch;
   dptr = dest->image_data;
   sptr = src->image_data;

   if ((height & 15) > 2 && height > 16 && (unsigned long)dptr > 0)
      optimised_tail = 16 - (height & 15);


   dptr += x_offset * 2 + y_offset * dpitch;
   sptr = (void *)((char *)sptr + src_y_offset * spitch);
   sptr += src_x_offset * sizeof(unsigned int);

   if (height >= 16)
      do
      {
         vc_genconv_3d32_to_rgb565(dptr, dpitch, sptr, spitch, width, 16, pal);
         dptr += dpitch << 4;
         sptr += spitch << 4;
         height -= 16;
      } while (height >= 16);

   if (optimised_tail)
   {
      dptr -= dpitch * optimised_tail;
      sptr -= spitch * optimised_tail;
      vc_genconv_3d32_to_rgb565(dptr, dpitch, sptr, spitch, width, 16, pal);
   }
   else if (height > 0)
      vc_genconv_3d32_to_rgb565(dptr, dpitch, sptr, spitch, width, height, pal);
}

/******************************************************************************
NAME
   vc_image_3d32b_to_rgb565_part

SYNOPSIS
   void vc_image_3d32b_to_rgb565_part(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T          *src,
      int                  src_x_offset,
      int                  src_y_offset);

FUNCTION
   Convert part of an 3d32b image to RGB565.

RETURNS
   void
******************************************************************************/

void vc_image_3d32b_to_rgb565_part(
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
   unsigned char const *sptr;
   int                  optimised_tail = 0;

   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_3D32) && is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));

   dpitch = dest->pitch;
   spitch = src->pitch;
   dptr = dest->image_data;
   sptr = src->image_data;

   if ((height & 15) > 2 && height > 16 && (unsigned long)dptr > 0)
      optimised_tail = 16 - (height & 15);


   dptr += x_offset * 2 + y_offset * dpitch;
   sptr = (void *)((char *)sptr + src_y_offset * spitch);
   sptr += src_x_offset * sizeof(unsigned int);

   if (height >= 16)
      do
      {
         vc_genconv_3d32b_to_rgb565(dptr, dpitch, sptr, spitch, width, 16);
         dptr += dpitch << 4;
         sptr += spitch << 4;
         height -= 16;
      } while (height >= 16);

   if (optimised_tail)
   {
      dptr -= dpitch * optimised_tail;
      sptr -= spitch * optimised_tail;
      vc_genconv_3d32b_to_rgb565(dptr, dpitch, sptr, spitch, width, 16);
   }
   else if (height > 0)
      vc_genconv_3d32b_to_rgb565(dptr, dpitch, sptr, spitch, width, height);
}


/******************************************************************************
NAME
   vc_image_transparent_alpha_blt_rgb565

SYNOPSIS
   void vc_image_transparent_alpha_blt_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                        VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int transparent_colour,int alpha);

FUNCTION
   Blt given region of an RGB565 image to another with a nominated "transparent" colour.
   Transparency values between 0 and 256 are allowed

RETURNS
   void
******************************************************************************/

void vc_image_transparent_alpha_blt_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
      VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int transparent_colour,int alpha) {
   int dest_pitch_bytes, src_pitch_bytes;
   int dest_pitch_pixels, src_pitch_pixels;
   unsigned short *dest, *src;

   vcos_assert(is_valid_vc_image_buf(dest_image, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src_image, VC_IMAGE_RGB565));
   dest_pitch_bytes = dest_image->pitch;
   src_pitch_bytes = src_image->pitch;
   dest_pitch_pixels = dest_pitch_bytes>>1;
   src_pitch_pixels = src_pitch_bytes>>1;
   dest = ((unsigned short *)dest_image->image_data) + y_offset*dest_pitch_pixels + x_offset;
   src = ((unsigned short *)src_image->image_data) + src_y_offset*src_pitch_pixels + src_x_offset;

   vcos_assert(width<=256); /* Current limitation of the blit function */

   vc_rgb565_transparent_alpha_blt(dest, src, dest_pitch_bytes, src_pitch_bytes, width, height, transparent_colour,alpha);

}


/******************************************************************************
NAME
   vc_image_font_alpha_blt_rgb565

SYNOPSIS
   void vc_image_font_alpha_blt_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                        VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int red,int green,int blue,int alpha);

FUNCTION
   Blt a special 4.4 font image generated from a PNG file.
   Only works with a 565 destination image.
   Can choose a colour via red,green,blue arguments.  Each component should be between 0 and 255.
   The alpha value present in the font image is scaled by the alpha argument given to the routine
   Alpha should be between 0 and 256.

RETURNS
   void
******************************************************************************/

void vc_image_font_alpha_blt_rgb565 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                     VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int red,int green,int blue,int alpha)
{
   int dest_pitch_bytes, src_pitch_bytes;
   int dest_pitch_pixels, src_pitch_pixels;
   unsigned short *dest;
   unsigned char *src;

   dest_pitch_bytes = dest_image->pitch;
   src_pitch_bytes = src_image->pitch;
   dest_pitch_pixels = dest_pitch_bytes>>1;
   src_pitch_pixels = src_pitch_bytes;
   dest = ((unsigned short *)dest_image->image_data) + y_offset*dest_pitch_pixels + x_offset;
   src = ((unsigned char *)src_image->image_data) + src_y_offset*src_pitch_pixels + src_x_offset;

   vcos_assert(width<=256); /* Current limitation of the blit function */

   vc_rgb565_font_alpha_blt(dest, src, dest_pitch_bytes, src_pitch_bytes, width, height, red,green,blue,alpha);

}



/******************************************************************************
NAME
   vc_image_transparent_alpha_blt_rgb565_from_pal8

SYNOPSIS
   void vc_image_transparent_alpha_blt_rgb565_from_pal8(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                        VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int transparent_colour,int alpha);

FUNCTION
   Blt given region of an RGB565 image to another with a nominated "transparent" colour.
   Transparency values between 0 and 256 are allowed

RETURNS
   void
******************************************************************************/

void vc_image_transparent_alpha_blt_rgb565_from_pal8 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
      VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int transparent_colour,int alpha) {
   int dest_pitch_bytes, src_pitch_bytes;
   int dest_pitch_pixels, src_pitch_pixels;
   unsigned short *dest;
   unsigned char *src;

   vcos_assert(is_valid_vc_image_buf(dest_image, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src_image, VC_IMAGE_4BPP) || is_valid_vc_image_buf(src_image, VC_IMAGE_8BPP));
   dest_pitch_bytes = dest_image->pitch;
   src_pitch_bytes = src_image->pitch;
   dest_pitch_pixels = dest_pitch_bytes>>1;
   src_pitch_pixels = src_pitch_bytes;

   vcos_assert(width<=256); /* Current limitation of the blit function */

   /* Now we need to determine whether we have 4 bpp or 8bpp */
   if (src_image->type == VC_IMAGE_8BPP)
   {
      /* 8bpp */
      dest = ((unsigned short *)dest_image->image_data) + y_offset*dest_pitch_pixels + x_offset;
      src = ((unsigned char *)src_image->image_data) + src_y_offset*src_pitch_pixels + src_x_offset;

      vc_rgb565_transparent_alpha_blt_from_pal8(dest, src, dest_pitch_bytes, src_pitch_bytes, width, height, transparent_colour, alpha, vc_image_get_palette(src_image), 0, 255);
      /* 0 and 255 correspond to a=(a>>0)&255, used for selecting 4 bit or 8 bit palettes */
   }
   else
   {
      /* 4bpp */
      int midh=(src_image->height+1)>>1; /* Level at which second half of image begins, first half in bottom 4 bits, second in top 4 bits */
      /* Calculate the last y value for us to do*/
      int top_y=min(midh,height+src_y_offset);
      int top_height=top_y-src_y_offset;

      if (top_height>0)
      {
         /* Need to do a blit to the lower portion */
         dest = ((unsigned short *)dest_image->image_data) + y_offset*dest_pitch_pixels + x_offset;
         src = ((unsigned char *)src_image->image_data) + src_y_offset*src_pitch_pixels + src_x_offset;

         vc_rgb565_transparent_alpha_blt_from_pal8(dest, src, dest_pitch_bytes, src_pitch_bytes, width, top_height, transparent_colour,alpha,vc_image_get_palette(src_image),0,15);

         src_y_offset+=top_height;
         y_offset+=top_height;
         height-=top_height;
      }

      if (height>0)
      {
         /* Need to blit in the bottom portion */
         dest = ((unsigned short *)dest_image->image_data) + y_offset*dest_pitch_pixels + x_offset;
         src = ((unsigned char *)src_image->image_data) + (src_y_offset-midh)*src_pitch_pixels + src_x_offset;

         vc_rgb565_transparent_alpha_blt_from_pal8(dest, src, dest_pitch_bytes, src_pitch_bytes, width, height, transparent_colour,alpha,vc_image_get_palette(src_image),4,15);
      }
   }
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   resize
******************************************************************************/

void vc_image_resize_stripe_h_rgb565(VC_IMAGE_T *dest, int d_width, int s_width)
{
   int step;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));
   vcos_assert(d_width > 0 && s_width > 0);
   vcos_assert(dest->height == 16);
   vcos_assert((dest->pitch & 31) == 0);
   vcos_assert(dest->width >= s_width);
   vcos_assert(dest->width >= d_width);
   vcos_assert(vclib_check_VRF());

   if (d_width <= 0)
      return;

   while (d_width << 2 < s_width)
   {
      s_width >>= 2;
      vc_stripe_h_reduce_rgb5652(dest->image_data, dest->pitch, dest->image_data, dest->pitch, s_width, 0x40000);
   }

   step = ((s_width << 16) + (d_width >> 1)) / d_width;

   if (step != 0x10000)
   {
      switch (_msb((step - 1) | 0x1000))
      {
      case 17: /* 2.0 < step <= 4.0 */
         vc_stripe_h_reduce_rgb5652(dest->image_data, dest->pitch, dest->image_data, dest->pitch, d_width, step);
         break;
      case 16: /* 1.0 < step <= 2.0 */
         vc_stripe_h_reduce_rgb565(dest->image_data, dest->pitch, dest->image_data, dest->pitch, d_width, step);
         break;
      case 12: /* 0.0625 < step <= 0.125 */
      case 13: /* 0.125 < step <= 0.25 */
      case 14: /* 0.25 < step <= 0.5 */
      case 15: /* 0.5 < step <= 1.0 */
#if 1
         {
            int off;

            off = (dest->pitch - (s_width<<1)) & ~31;

            vclib_memmove((char *)dest->image_data + off, dest->image_data, dest->pitch * 16 - off);

            vc_stripe_h_expand_rgb565(dest->image_data, dest->pitch, (void *)((char *)dest->image_data + off), dest->pitch, d_width, step, s_width);
         }
#else
         vc_stripe_h_expand_rgb565((unsigned short *)dest->image_data + d_width, dest->pitch, (unsigned short *)dest->image_data + s_width, dest->pitch, d_width, step);
#endif
         break;

      default:
         vcos_assert(!"can't cope with scale factor");
      }
   }
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   resize
******************************************************************************/

void vc_image_resize_stripe_v_rgb565(VC_IMAGE_T *dest, int d_height, int s_height)
{
   int step;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));
   vcos_assert(d_height > 0 && s_height > 0);
   vcos_assert(dest->width == 16);
   vcos_assert(dest->height >= s_height);
   vcos_assert(dest->height >= d_height);

   if (d_height <= 0)
      return;

   while (d_height << 2 < s_height)
   {
      s_height >>= 2;
      vc_stripe_v_reduce_rgb5652(dest->image_data, dest->pitch, dest->image_data, dest->pitch, s_height, 0x40000);
   }

   step = ((s_height << 16) + (d_height >> 1)) / d_height;

   if (step != 0x10000)
   {
      switch (_msb((step - 1) | 0x1000))
      {
      case 17: /* 2.0 < step <= 4.0 */
         vc_stripe_v_reduce_rgb5652(dest->image_data, dest->pitch, dest->image_data, dest->pitch, d_height, step);
         break;
      case 16: /* 1.0 < step <= 2.0 */
         vc_stripe_v_reduce_rgb565(dest->image_data, dest->pitch, dest->image_data, dest->pitch, d_height, step);
         break;
      case 12: /* 0.0625 < step <= 0.125 */
      case 13: /* 0.125 < step <= 0.25 */
      case 14: /* 0.25 < step <= 0.5 */
      case 15: /* 0.5 < step <= 1.0 */
#if 1
         {
            int off;

            off = ((dest->height + 15 & ~15) - s_height) * dest->pitch;

            vclib_memmove((void *)((char *)dest->image_data + off), dest->image_data, dest->pitch * s_height);

            vc_stripe_v_expand_rgb565(dest->image_data, dest->pitch, (void *)((char *)dest->image_data + off), dest->pitch, d_height, step, s_height);
         }
#else
         vc_stripe_v_expand_rgb565((void *)((char *)dest->image_data + dest->pitch * d_height), dest->pitch, (void *)((char *)dest->image_data + dest->pitch * s_height), dest->pitch, d_height, step);
#endif
         break;
      default:
         vcos_assert(!"can't cope with scale factor");
      }
   }
}


/******************************************************************************
NAME
   vc_image_rgba32_to_rgb565

SYNOPSIS
   void vc_image_rgba32_to_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T *src)

FUNCTION
   convert
******************************************************************************/

void vc_image_rgba32_to_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T *src)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32));

   rgb565_from_rgba32_blocky(
      dest->image_data, dest->pitch,
      src->image_data, src->pitch,
      dest->width,
      dest->height);
}

//get a pixel from a SAND VIDEO image
static inline void *get_yuv_uv_y(int x, int y, void *imagebuf, int imagepitch)
{
   int strip = x>>VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2;
   int strip_offset = strip*imagepitch;
   int xoffset = x & (VC_IMAGE_YUV_UV_STRIPE_WIDTH-1);

   return &(strip_offset+(unsigned char *)imagebuf)[y*VC_IMAGE_YUV_UV_STRIPE_WIDTH+xoffset];
}
static inline void *get_yuv_uv_uv(int x, int y, void *imagebuf, int imagepitch)
{
   int strip = x>>VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2;
   int strip_offset = strip*imagepitch;
   int xoffset = x & (VC_IMAGE_YUV_UV_STRIPE_WIDTH-1);

   return &(strip_offset+(unsigned char *)imagebuf)[(y>>1)*VC_IMAGE_YUV_UV_STRIPE_WIDTH+(xoffset&(~0x1))];
}

/******************************************************************************
NAME
   vc_image_yuv_uv_to_rgb565

SYNOPSIS
   void vc_image_yuv_uv_to_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src)

FUNCTION
   Convert SAND YUVUV to RGB565
******************************************************************************/
void vc_image_yuv_uv_to_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src)
{
   int x, y;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV_UV));

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned short *destptr = (void *)((char *)dest->image_data + y * dest->pitch);

      for (x = 0; x < dest->width; x += 32)
      {
         void *dptr1 = destptr + x;
         void *srcptr_y  = get_yuv_uv_y(x, y, vc_image_get_y(src), src->pitch);
         void *srcptr_uv = get_yuv_uv_uv(x, y, vc_image_get_u(src), src->pitch);

         yuv_uv_to_rgb_load_32x16(srcptr_y, srcptr_uv, srcptr_uv, VC_IMAGE_YUV_UV_STRIPE_WIDTH, VC_IMAGE_YUV_UV_STRIPE_WIDTH,
                                  (x + 16 < dest->width) ? yuv_to_rgb565_store_32x16
                                                         : yuv_to_rgb565_store_16x16,
                                  dptr1, dest->pitch, 0xff, 16);
      }
   }
}

