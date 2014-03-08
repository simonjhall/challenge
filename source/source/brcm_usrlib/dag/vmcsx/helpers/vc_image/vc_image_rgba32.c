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
#include <stdio.h>

/* Project includes */
#include "vcinclude/vcore.h"
#include "vclib/vclib.h"
#include "vc_image.h"
#include "yuv_to_rgb.h"
#include "striperesize.h"
#include "interface/vcos/vcos_assert.h"

#ifdef __CC_ARM
#include <string.h>
#include "vcinclude/vc_asm_ops.h"
#define vclib_check_VRF()			1
#endif

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vc_image_yuv420_to_rgba32 (VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha);
extern void vc_image_yuv422_to_rgba32 (VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha);
extern void vc_image_yuv420_to_rgba32_part (VC_IMAGE_T *dest, int x_offset, int y_offset, int width,
         int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha);
extern void vc_image_yuv422_to_rgba32_part (VC_IMAGE_T *dest, int x_offset, int y_offset, int width,
         int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha);
extern void vc_image_rgb888_to_rgba32 (VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha);
extern void vc_image_rgb565_to_rgba32 (VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha, int transparent_colour);
extern void vc_image_palette8_to_rgba32 (VC_IMAGE_T *dest, VC_IMAGE_T const *src, short const *cmap, int alpha,
         int transparent_colour);
extern void vc_image_palette4_to_rgba32 (VC_IMAGE_T *dest, VC_IMAGE_T const *src, short const *cmap, int alpha,
         int transparent_colour);
extern void vc_image_blt_rgba32 (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                    VC_IMAGE_T const *src, int src_x_offset, int src_y_offset);
extern void vc_image_overlay_rgba32_to_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha);
extern void vc_image_quick_overlay_rgba32_to_rgb565(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, VC_IMAGE_T const *block_mask,
         int block_x_offset, int block_y_offset, int alpha);
extern void vc_image_overlay_rgba32_to_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha);
extern void vc_image_quick_overlay_rgba32_to_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, VC_IMAGE_T const *block_mask,
         int block_x_offset, int block_y_offset, int alpha);
extern void vc_image_overlay_rgba32_to_rgba32 (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha);
extern void vc_image_overlay_rgb565_to_rgba32 (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int transparent_colour, int alpha);
extern void vc_image_mix_rgba32_to_rgba32 (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T const *src, int src_x_offset, int src_y_offset);
extern void vc_image_rgba32_set_alpha_1bpp (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *mask, int mask_x_offset, int mask_y_offset, int low_alpha, int high_alpha);
extern void vc_image_rgba32_set_alpha_8bpp (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *mask, int mask_x_offset, int mask_y_offset);
extern void vc_image_rgba32_adjust_alpha_1bpp (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *mask, int mask_x_offset, int mask_y_offset, int low_alpha_scale, int high_alpha_scale);
extern void vc_image_rgba32_adjust_alpha_8bpp (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_T *mask, int mask_x_offset, int mask_y_offset, int alpha_scale, int alpha_boost);
extern void vc_image_transpose_rgba32 (VC_IMAGE_T *dest, VC_IMAGE_T const *src);
extern void vc_image_transpose_tiles_rgba32 (VC_IMAGE_T *dest, VC_IMAGE_T const *src, int hdeci);
extern void vc_image_vflip_rgba32 (VC_IMAGE_T *dest, VC_IMAGE_T const *src);
extern void vc_image_hflip_rgba32 (VC_IMAGE_T *dest, VC_IMAGE_T const *src);
extern void vc_image_resize_stripe_h_rgba32 (VC_IMAGE_T *dest, int d_width, int s_width);
extern void vc_image_resize_stripe_v_rgba32 (VC_IMAGE_T *dest, int d_height, int s_height);
extern void vc_image_fill_rgba32 (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                     long value);
extern void vc_image_tint_rgba32 (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                     long value);
extern void vc_image_text_rgba32 (VC_IMAGE_T *dest, int x_offset, int y_offset, long fg_colour, long bg_colour, char const *text,
                                     int *max_x, int *max_y);
extern void vc_image_small_text_rgba32 (VC_IMAGE_T *dest, int x_offset, int y_offset, long fg_colour,
                                           long bg_colour, char const *text, int *max_x, int *max_y);
extern void vc_image_text_32_rgba32 (VC_IMAGE_T *dest, int x_offset, int y_offset, long fg_colour,
                                        long bg_colour, char const *text, int *max_x, int *max_y);
extern void vc_image_text_24_rgba32 (VC_IMAGE_T *dest, int x_offset, int y_offset, long fg_colour,
                                        long bg_colour, char const *text, int *max_x, int *max_y);
extern void vc_image_text_20_rgba32 (VC_IMAGE_T *dest, int x_offset, int y_offset, long fg_colour,
                                        long bg_colour, char const *text, int *max_x, int *max_y);

void vc_image_premultiply_rgba32(VC_IMAGE_T *dest);

/* Check extern defs match above and loads #defines and typedefs */
#include "vc_image_rgba32.h"

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/
/* Pointer to conversion function (see below) */
typedef void (*MYCONVFUNC_T)(unsigned long *d, int dp, unsigned char const *s, int sp, int a);

extern void rgba32_from_rgb888(
      unsigned long       *desta32,
      int                  dpitch,
      unsigned char const *src888,
      int                  spitch,
      int                  alpha);

extern void rgba32_from_bgr888(
      unsigned long       *desta32,
      int                  dpitch,
      unsigned char const *src888,
      int                  spitch,
      int                  alpha);

extern void rgba32_from_rgb565(
      unsigned long       *desta32,
      int                  dpitch,
      unsigned short const *src565,
      int                  spitch,
      int                  alpha);

extern void rgba32_from_transparent_rgb565(
      unsigned long       *desta32,
      int                  dpitch,
      unsigned short const*src565,
      int                  spitch,
      int                  alpha,
      int                  transparent_colour);

extern void rgba32_from_transparent_4bpp(
      unsigned long       *desta32,
      int                  dpitch,
      unsigned char const *src4bpp,
      int                  spitch,
      int                  alpha,
      int                  transparent_colour,
      unsigned short const *palette);

extern void rgba32_overlay_to_rgba32(
      unsigned long       *desta32,
      int                  dpitch,
      unsigned long const *srca32,
      int                  spitch,
      unsigned long        opmask,
      int                  ascale);

extern void rgba32_overlay_to_rgba32_n(
      unsigned long       *desta32,
      int                  dpitch,
      unsigned long const *srca32,
      int                  spitch,
      int                  lines,
      unsigned long        opmask,
      int                  ascale);

extern void rgba32_overlay_to_rgb888_blocky( /* the sloppy one that goes fast */
      unsigned char       *dest888,
      int                  dpitch,
      unsigned long const *srca32,
      int                  spitch,
      int                  alpha);

extern void rgba32_overlay_to_rgb888(      /* 16 lines with tidy edges */
      unsigned char       *dest888,
      int                  dpitch,
      unsigned long const *srca32,
      int                  spitch,
      int                  alpha,
      unsigned long long   opmask);

extern void rgba32_overlay_to_rgb888_n(    /* n lines with tidy edges */
      unsigned char       *dest888,
      int                  dpitch,
      unsigned long const *srca32,
      int                  spitch,
      int                  alpha,
      int                  lines,
      unsigned long long   opmask);

extern void rgba32_overlay_to_rgb565_blocky( /* the sloppy one that goes fast */
      unsigned short      *dest565,
      int                  dpitch,
      unsigned long const *srca32,
      int                  spitch,
      int                  alpha);

extern void rgba32_overlay_to_rgb565(      /* 16 lines with tidy edges */
      unsigned short      *dest565,
      int                  dpitch,
      unsigned long const *srca32,
      int                  spitch,
      int                  alpha,
      unsigned short       opmask);

extern void rgba32_overlay_to_rgb565_n(    /* n lines with tidy edges */
      unsigned short       *dest565,
      int                  dpitch,
      unsigned long const *srca32,
      int                  spitch,
      int                  alpha,
      int                  lines,
      unsigned short       opmask);

extern void rgb565_overlay_to_rgba32_n(    /* n lines with tidy edges */
      unsigned long       *desta32,
      int                  dpitch,
      unsigned short const*src565,
      int                  spitch,
      int                  lines,
      unsigned short       opmask,
      int                  transparent_colour,
      int                  alpha);

extern void rgba32_alpha_adjust8(
      unsigned char       *dest,
      int                  dpitch,
      unsigned char const *mask,
      int                  mpitch,
      int                  scale,
      int                  boost);

extern void rgba32_hflip_in_place(
      unsigned long       *desta32,
      int                  dpitch,
      int                  width);

extern void rgba32_transpose_chunky(
      unsigned long       *desta32,
      int                  dpitch,
      unsigned long const *srca32,
      int                  pitch);

extern void rgba32_transpose_chunky0(
      unsigned long       *desta32,
      int                  dpitch,
      unsigned long const *srca32,
      int                  pitch);

extern void rgba32_transpose_chunky1(
      unsigned long       *desta32,
      int                  dpitch,
      unsigned long const *srca32,
      int                  pitch);

extern void rgba32_copy_alpha_from_8bpp(
      unsigned char       *dest,
      int                  dpitch,
      unsigned char const *mask,
      int                  mpitch);

void rgba32_premultiply(
   unsigned long       *dest,       /* r0 */
   int                  dpitch,     /* r1 */
   unsigned long       *src,        /* r2 */
   int                  spitch,     /* r3 */
   int                  alphabits,  /* r4 */
   int                  count);     /* r5 */

void rgba32_composite(
   unsigned long       *dest,       /* r0 */
   int                  dpitch,     /* r1 */
   unsigned long const *src,        /* r2 */
   int                  spitch,     /* r3 */
   int                  alphabits,  /* r4 */
   int                  width,      /* r5 */
   int                  height,     /* sp */
   int                  alpha);     /* sp+4 */

void rgba32_composite_normalised(
   unsigned long       *dest,       /* r0 */
   int                  dpitch,     /* r1 */
   unsigned long const *src,        /* r2 */
   int                  spitch,     /* r3 */
   int                  alphabits,  /* r4 */
   int                  width,      /* r5 */
   int                  height,     /* sp */
   int                  alpha);     /* sp+4 */

void rgba32_transpose(
   unsigned long *dest,
   const unsigned long *src,
   int dest_pitch, 
   int src_pitch,
   int src_height);

/* borrowed from vc_image_rgb565_asm.s */
extern void vc_rgb565_vflip (unsigned short *dest, unsigned short *src, int src_pitch_bytes, int height, int nblocks, int dest_pitch_bytes);
extern void vc_rgb565_vflip_in_place (unsigned short *start, unsigned short *end, int pitch_bytes, int nblocks);

/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/


/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/

static void vc_image_overlay_rgba32_to_rgba32_historical(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha);

static void vc_image_overlay_rgba32_to_rgba32_superawesome(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha);

static void overlay_16_lines_to_rgb888(
   unsigned char       *dptr,
   int                  dpitch,
   unsigned long const *sptr,
   int                  spitch,
   unsigned long const *mptr,
   int                  mshift,
   int                  width,
   int                  alpha,
   unsigned long long   maskleft,
   unsigned long long   maskright);

static void overlay_n_lines_to_rgb888(
   unsigned char       *dptr,
   int                  dpitch,
   unsigned long const *sptr,
   int                  spitch,
   unsigned long const *mptr,
   int                  mshift,
   int                  width,
   int                  height,
   int                  alpha,
   unsigned long long   maskleft,
   unsigned long long   maskright);

static void overlay_16_lines_to_rgb565(
   unsigned short      *dptr,
   int                  dpitch,
   unsigned long const *sptr,
   int                  spitch,
   unsigned long const *mptr,
   int                  mshift,
   int                  width,
   int                  alpha,
   unsigned short       maskleft,
   unsigned short       maskright);

static void overlay_n_lines_to_rgb565(
   unsigned short      *dptr,
   int                  dpitch,
   unsigned long const *sptr,
   int                  spitch,
   unsigned long const *mptr,
   int                  mshift,
   int                  width,
   int                  height,
   int                  alpha,
   unsigned short       maskleft,
   unsigned short       maskright);

static void overlay_16_lines_from_rgb565(
   unsigned long       *dptr,
   int                  dpitch,
   unsigned short const*sptr,
   int                  spitch,
   int                  width,
   int                  transparent_colour,
   int                  alpha,
   unsigned short       maskleft,
   unsigned short       maskright);

static void overlay_n_lines_from_rgb565(
   unsigned long       *dptr,
   int                  dpitch,
   unsigned short const*sptr,
   int                  spitch,
   int                  width,
   int                  height,
   int                  transparent_colour,
   int                  alpha,
   unsigned short       maskleft,
   unsigned short       maskright);

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

/* Conversions ***********************************************************{{{*/

/******************************************************************************
NAME
   vc_image_yuv420_to_rgba32

SYNOPSIS
   void vc_image_yuv420_to_rgba32(
      VC_IMAGE_T          *dest,
      VC_IMAGE_T    const *src,
      int                  alpha)

FUNCTION
   x
******************************************************************************/

void vc_image_yuv420_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha)
{
   static const VC_IMAGE_YUV_TO_RGB_MODE_T
   mode_misaligned =
   {
      yuv420_to_rgb_load_32x16_misaligned,
      yuv420_to_rgb_load_32x16_misaligned,
      yuv_to_rgba32_store_32x16,
      yuv_to_rgba32_store_16x16
   },
   mode =
   {
      yuv420_to_rgb_load_32x16,
      yuv420_to_rgb_load_32x16,
      yuv_to_rgba32_store_32x16,
      yuv_to_rgba32_store_16x16
   };
   VC_IMAGE_YUV_TO_RGB_MODE_T const *modeptr = &mode;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV420) && (((unsigned long)src->image_data | src->pitch) & 15) == 0);
   vcos_assert(vclib_check_VRF());

   if ((((unsigned long)src->image_data | src->pitch) & 31) != 0)
      modeptr = &mode_misaligned;

   if (src->height <= 16)
      vc_image_yuv_to_rgb_stripe(modeptr,
                                 (unsigned char*)dest->image_data,
                                 vc_image_get_y(src), vc_image_get_u(src), vc_image_get_v(src),
                                 32*4, dest->pitch,
                                 src->pitch, src->pitch >> 1, dest->width, alpha);
   else
      vc_image_yuv_to_rgb(modeptr,
                          (unsigned char*)dest->image_data,
                          vc_image_get_y(src), vc_image_get_u(src), vc_image_get_v(src),
                          32*4, dest->pitch,
                          src->pitch, src->pitch >> 1, src->pitch << (3-1),
                          dest->width, dest->height, alpha);
}


/******************************************************************************
NAME
   vc_image_yuv422_to_rgba32

SYNOPSIS
   void vc_image_yuv422_to_rgba32(
      VC_IMAGE_T          *dest,
      VC_IMAGE_T    const *src,
      int                  alpha)

FUNCTION
   x
******************************************************************************/

void vc_image_yuv422_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha)
{
   static const VC_IMAGE_YUV_TO_RGB_MODE_T
   mode_misaligned =
   {
      yuv422_to_rgb_load_32x16_misaligned,
      yuv422_to_rgb_load_32x16_misaligned,
      yuv_to_rgba32_store_32x16,
      yuv_to_rgba32_store_16x16
   },
   mode =
   {
      yuv422_to_rgb_load_32x16,
      yuv422_to_rgb_load_32x16,
      yuv_to_rgba32_store_32x16,
      yuv_to_rgba32_store_16x16
   };
   VC_IMAGE_YUV_TO_RGB_MODE_T const *modeptr = &mode;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV422) && (((unsigned long)src->image_data | src->pitch) & 31) == 0);
   vcos_assert(vclib_check_VRF());

   if ((((unsigned long)src->image_data | src->pitch) & 31) != 0)
      modeptr = &mode_misaligned;

   if (src->height <= 16)
      vc_image_yuv_to_rgb_stripe(modeptr,
                                 (unsigned char*)dest->image_data,
                                 vc_image_get_y_422(src), vc_image_get_u_422(src), vc_image_get_v_422(src),
                                 32*4, dest->pitch,
                                 src->pitch, src->pitch, dest->width, alpha);
   else
      vc_image_yuv_to_rgb(modeptr,
                          (unsigned char*)dest->image_data,
                          vc_image_get_y_422(src), vc_image_get_u_422(src), vc_image_get_v_422(src),
                          32*4, dest->pitch,
                          src->pitch, src->pitch, src->pitch << 4,
                          dest->width, dest->height, alpha);
}


/******************************************************************************
NAME
   vc_image_yuv420_to_rgba32_part

SYNOPSIS
   void vc_image_yuv420_to_rgba32_part(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T    const *src,
      int                  src_x_offset,
      int                  src_y_offset,
      int                  alpha)

FUNCTION
   x

TODO
   Do the aligned-dest cases as well.
******************************************************************************/

void vc_image_yuv420_to_rgba32_part(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha)
{
   static const VC_IMAGE_YUV_TO_RGB_MODE_T
   mode_misaligned =
   {
      yuv420_to_rgb_load_32x16_misaligned,
      yuv420_to_rgb_load_32x16_misaligned,
      yuv_to_rgba32_store_32xn_misaligned,
      yuv_to_rgba32_store_mxn_misaligned
   },
   mode =
   {
      yuv420_to_rgb_load_32x16,
      yuv420_to_rgb_load_32x16,
      yuv_to_rgba32_store_32xn_misaligned,
      yuv_to_rgba32_store_mxn_misaligned
   };
   VC_IMAGE_YUV_TO_RGB_MODE_T const *modeptr = &mode;
   unsigned char *dptr, *yptr, *uptr, *vptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV420) && (((unsigned long)src->image_data | src->pitch) & 15) == 0);
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

   dptr = (unsigned char*)dest->image_data;
   yptr = vc_image_get_y(src);
   uptr = vc_image_get_u(src);
   vptr = vc_image_get_v(src);

   dptr += y_offset * dest->pitch + x_offset * 4;
   yptr += src_y_offset * src->pitch + src_x_offset;
   src_x_offset >>= 1;
   src_y_offset >>= 1;
   uptr += src_y_offset * (src->pitch >> 1) + src_x_offset;
   vptr += src_y_offset * (src->pitch >> 1) + src_x_offset;

   if ((((unsigned long)yptr | src->pitch) & 31) != 0)
      modeptr = &mode_misaligned;

   vc_image_yuv_to_rgb_precise(modeptr,
                               dptr, yptr, uptr, vptr,
                               32*4, dest->pitch,
                               src->pitch, src->pitch >> 1, src->pitch << (3-1),
                               width, height, alpha);
}


/******************************************************************************
NAME
   vc_image_yuv422_to_rgba32_part

SYNOPSIS
   void vc_image_yuv422_to_rgba32_part(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T    const *src,
      int                  src_x_offset,
      int                  src_y_offset,
      int                  alpha)

FUNCTION
   x

TODO
   Do the aligned-dest cases as well.
******************************************************************************/

void vc_image_yuv422_to_rgba32_part(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha)
{
   static const VC_IMAGE_YUV_TO_RGB_MODE_T
   mode_misaligned =
   {
      yuv422_to_rgb_load_32x16_misaligned,
      yuv422_to_rgb_load_32x16_misaligned,
      yuv_to_rgba32_store_32xn_misaligned,
      yuv_to_rgba32_store_mxn_misaligned
   },
   mode =
   {
      yuv422_to_rgb_load_32x16,
      yuv422_to_rgb_load_32x16,
      yuv_to_rgba32_store_32xn_misaligned,
      yuv_to_rgba32_store_mxn_misaligned
   };
   VC_IMAGE_YUV_TO_RGB_MODE_T const *modeptr = &mode;
   unsigned char *dptr, *yptr, *uptr, *vptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV422) && (((unsigned long)src->image_data | src->pitch) & 31) == 0);
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

   dptr = (unsigned char*)dest->image_data;
   yptr = vc_image_get_y_422(src);
   uptr = vc_image_get_u_422(src);
   vptr = vc_image_get_v_422(src);

   dptr += y_offset * dest->pitch + x_offset * 4;
   yptr += src_y_offset * src->pitch + src_x_offset;
   src_x_offset >>= 1;
   uptr += src_y_offset * src->pitch + src_x_offset;
   vptr += src_y_offset * src->pitch + src_x_offset;

   if ((((unsigned long)yptr | src->pitch) & 31) != 0)
      modeptr = &mode_misaligned;

   vc_image_yuv_to_rgb_precise(modeptr,
                               dptr, yptr, uptr, vptr,
                               32*4, dest->pitch,
                               src->pitch, src->pitch, src->pitch << 4,
                               width, height, alpha);
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   x
******************************************************************************/

/* Worker function for the next two functions converting from {rgb|bgr}888 to rgba32 */
static void rgb_to_rgba(VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha, MYCONVFUNC_T f)
{
   unsigned char       *dptr;
   unsigned char const *sptr;
   int                  width,
   height;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert((is_valid_vc_image_buf(src, VC_IMAGE_RGB888)
#ifdef USE_BGR888
            || is_valid_vc_image_buf(src, VC_IMAGE_BGR888)
#endif
          ) && (((unsigned long)src->image_data | src->pitch) & 15) == 0);
   vcos_assert(vclib_check_VRF());

   dptr = (unsigned char*)dest->image_data;
   sptr = (const unsigned char*)src->image_data;
   width = (dest->width + 15) >> 4;
   height = (dest->height + 15) >> 4;

   while (--height >= 0)
   {
      int wcount = width;
      while (--wcount >= 0)
      {
         (f)((unsigned long *)dptr, dest->pitch, sptr, src->pitch, alpha);
         dptr += 64;
         sptr += 48;
      }
      dptr += (dest->pitch << 4) - (width << 6);
      sptr += (src->pitch << 4) - (width * 48);
   }
}

void vc_image_rgb888_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha)
{
   rgb_to_rgba(dest, src, alpha, rgba32_from_rgb888);
}

/******************************************************************************
NAME
   x'

SYNOPSIS
   void x'(y)

FUNCTION
   Convert BGR888 to RGBA32
******************************************************************************/
void vc_image_bgr888_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha)
{
   rgb_to_rgba(dest, src, alpha, rgba32_from_bgr888);
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   x
******************************************************************************/

void vc_image_rgb565_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, int alpha, int transparent_colour)
{
   unsigned char       *dptr;
   unsigned char const *sptr;
   int                  width,
   height;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGB565) && (((unsigned long)src->image_data | src->pitch) & 31) == 0);
   vcos_assert(vclib_check_VRF());

   dptr = (unsigned char*)dest->image_data;
   sptr = (const unsigned char*)src->image_data;
   width = (dest->width + 15) >> 4;
   height = (dest->height + 15) >> 4;

   while (--height >= 0)
   {
      int wcount = width;
      while (--wcount >= 0)
      {
         rgba32_from_transparent_rgb565((unsigned long *)dptr, dest->pitch, (unsigned short const *)sptr, src->pitch, alpha, transparent_colour);
         dptr += 64;
         sptr += 32;
      }
      dptr += (dest->pitch << 4) - (width << 6);
      sptr += (src->pitch << 4) - (width << 5);
   }
}


/******************************************************************************
NAME
   vc_image_rgb565_to_rgba32_part

SYNOPSIS
   void vc_image_rgb565_to_rgba32_part(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T    const *src,
      int                  src_x_offset,
      int                  src_y_offset);

FUNCTION
   x
******************************************************************************/
void vc_image_rgb565_to_rgba32_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha)

{
   unsigned long       *dptr;
   unsigned short const *sptr;
   int pix_width;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32));
   vcos_assert((((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGB565));
   vcos_assert((((unsigned long)src->image_data | src->pitch) & 31) == 0);
   vcos_assert(vclib_check_VRF());

   dptr = (unsigned long *)dest->image_data // 4 bytes per pixel since long
          + (y_offset*dest->pitch >> 2)
          + x_offset;

   sptr = (const unsigned short *)src->image_data  // 2 bytes per pixel
          + (src_y_offset*src->pitch >> 1)
          + src_x_offset;

   while (height > 0)
   {
      pix_width = width;
      while (--pix_width >= 0)
      {
         unsigned long pixelVal = *sptr;
         *dptr = ( ((pixelVal & 0xF800) * 0x21 >> 13)) |
                 ( ((pixelVal & 0x07E0) * 0x41 >>  1) & 0x0000FF00 ) |
                 ( ((pixelVal & 0x001F) * 0x21 << 14) & 0x00FF0000 ) |
                 ( alpha << 24);
         dptr += 1; // 4 bytes per pixel
         sptr += 1; //2 bytes per pixel
      }
      dptr += (dest->pitch >> 2) - width; // 4bytes per pixel
      sptr += (src->pitch >> 1) - width; // 2bytes per pixel
      --height;
   }
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   x
******************************************************************************/

void vc_image_palette8_to_rgba32(
   VC_IMAGE_T          *dest,
   VC_IMAGE_T    const *src,
   short         const *cmap,
   int                  alpha,
   int                  transparent_colour)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_8BPP) && (src->pitch & 15) == 0);
   vcos_assert(vclib_check_VRF());

   vcos_assert(!"not implemented");
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   x
******************************************************************************/

void vc_image_palette4_to_rgba32(
   VC_IMAGE_T          *dest,
   VC_IMAGE_T    const *src,
   short         const *cmap,
   int                  alpha,
   int                  transparent_colour)
{
   unsigned char       *dptr;
   unsigned char const *sptr;
   int                  width,
   height;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_4BPP) && (src->pitch & 7) == 0);
   vcos_assert(vclib_check_VRF());

   dptr = (unsigned char*)dest->image_data;
   sptr = (const unsigned char*)src->image_data;
   width = (dest->width + 15) >> 4;
   height = (dest->height + 15) >> 4;

#if 0
   if (cmap == NULL)
      if (src->type == VC_IMAGE_PAL4)
         cmap = (void *)sptr;
      else
         vcos_assert(!"I forgot where the default palette is.");
   if (src->type == VC_IMAGE_PAL4)
      sptr += 32;
#endif
   while (--height >= 0)
   {
      int wcount = width;
      while (--wcount >= 0)
      {
         rgba32_from_transparent_4bpp((unsigned long *)dptr, dest->pitch, (unsigned char const *)sptr, src->pitch, alpha, transparent_colour, (unsigned short *)cmap);
         dptr += 64;
         sptr += 8;
      }
      dptr += (dest->pitch << 4) - (width << 6);
      sptr += (src->pitch << 4) - (width << 3);
   }
}

/* End of conversions ****************************************************}}}*/

/* Blending functions ****************************************************{{{*/

/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   Move data around without messing with it
******************************************************************************/

void vc_image_blt_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset)
{
   unsigned char       *dptr;
   unsigned char const *sptr;

   if((src == NULL) || (dest == NULL))
   {
     vcos_assert(0);
     return;
   }

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) || is_valid_vc_image_buf(dest, VC_IMAGE_ARGB8888) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32) || is_valid_vc_image_buf(src, VC_IMAGE_ARGB8888) && (((unsigned long)src->image_data | src->pitch) & 31) == 0);
   vcos_assert((((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert((((unsigned long)src->image_data | src->pitch) & 31) == 0);
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height,  0, 0, dest->width, dest->height));
   vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height,  0, 0, src->width, src->height));
   vcos_assert(vclib_check_VRF());

   dptr = (unsigned char *)dest->image_data + (x_offset << 2) + y_offset * dest->pitch;
   sptr = (unsigned char const *)src->image_data + (src_x_offset << 2) + src_y_offset * src->pitch;
   width <<= 2;

   while (--height >= 0)
   {
      vclib_memcpy(dptr, sptr, width);
      dptr += dest->pitch;
      sptr += src->pitch;
   }
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   combine data and output rgb565
******************************************************************************/

void vc_image_overlay_rgba32_to_rgb565(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha)
{
   vc_image_quick_overlay_rgba32_to_rgb565(
      dest, x_offset, y_offset, width, height,
      src, src_x_offset, src_y_offset,
      NULL, 0, 0, alpha);
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   combine data and output rgb565, using only the 16x16 blocks indicated in
   block_mask
******************************************************************************/

void vc_image_quick_overlay_rgba32_to_rgb565(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   VC_IMAGE_T    const *block_mask,
   int                  block_x_offset,
   int                  block_y_offset,
   int                  alpha)
{
   unsigned char       *dptr;
   unsigned char const *sptr;
   unsigned long const *mptr;
   int                  mptr_pitch;
   int                  mshift;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB565) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32) && (((unsigned long)src->image_data | src->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(block_mask, VC_IMAGE_1BPP));
   vcos_assert(width > 0 && height > 0);
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height,  0, 0, dest->width+15&~15, dest->height+15&~15));
   vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height,  0, 0, src->width+15&~15, src->height+15&~15));
   vcos_assert( (dest->type == VC_IMAGE_RGB565) && ((((unsigned long)dest->image_data | dest->pitch) & 31) == 0) );
   vcos_assert( ((src->type == VC_IMAGE_RGBA32) || (src->type == VC_IMAGE_ARGB8888)) && ((((unsigned long)src->image_data | src->pitch) & 31) == 0) );
   vcos_assert(vclib_check_VRF());

   if (block_mask != NULL)
   {
      mptr = (const unsigned long *)((char *)block_mask->image_data + block_y_offset * block_mask->pitch);
      mshift = block_x_offset & 31;
      mptr += block_x_offset >> 5;
      mptr_pitch = block_mask->pitch;
   }
   else
   {
      static const unsigned long allones = 0xFFFFFFFFul;
      mptr = &allones;
      mptr_pitch = 0;
      mshift = 32;
   }

   if (((x_offset | src_x_offset | width | height) & 15) == 0)
   {
      dptr = (unsigned char *)dest->image_data + (x_offset << 1) + y_offset * dest->pitch;
      sptr = (unsigned char const *)src->image_data + (src_x_offset << 2) + src_y_offset * src->pitch;
      width = (width + 15) >> 4;
      height = (height + 15) >> 4;

      while (--height >= 0)
      {
         int wcount = width;
         unsigned long const *tmpmptr = mptr;
         unsigned long tmpmshift = mshift;
         unsigned long blocks = *tmpmptr++;

         while (--wcount >= 0)
         {
            if (blocks & 1 << tmpmshift)
               rgba32_overlay_to_rgb565_blocky((unsigned short *)dptr, dest->pitch, (const unsigned long *)sptr, src->pitch, alpha);
            dptr += 32;
            sptr += 64;
            if (++tmpmshift == 32)
            {
               blocks = *tmpmptr++;
               tmpmshift = 0;
            }
         }
         dptr += (dest->pitch << 4) - (width << 5);
         sptr += (src->pitch << 4) - (width << 6);
         mptr = (const unsigned long *)((char *)mptr + mptr_pitch);
      }
      return;
   }
   else
   {
      unsigned short       maskleft,
      maskright;

      sptr = (unsigned char const *)src->image_data + ((src_x_offset & ~7) << 2) + src_y_offset * src->pitch;
      src_x_offset &= 7;
      width += src_x_offset;
      x_offset -= src_x_offset;

      dptr = (unsigned char *)dest->image_data + (x_offset << 1) + y_offset * dest->pitch;

      maskleft = 0xFFFFu << src_x_offset;
      width--;
      maskright = 0xFFFFu >> (~width & 15);
      width >>= 4;

      if (width == 0)
         maskleft &= maskright;

      while (height >= 16)
      {
         int wcount = width;

         overlay_16_lines_to_rgb565((unsigned short *)dptr, dest->pitch, (const unsigned long *)sptr, src->pitch, mptr, mshift, width, alpha, maskleft, maskright);

         dptr += dest->pitch << 4;
         sptr += src->pitch << 4;
         mptr = (const unsigned long *)((char *)mptr + mptr_pitch);
         height -= 16;
      }

      if (height > 0)
         overlay_n_lines_to_rgb565((unsigned short *)dptr, dest->pitch, (const unsigned long *)sptr, src->pitch, mptr, mshift, width, height, alpha, maskleft, maskright);
   }
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   combine data and output rgb888
******************************************************************************/

void vc_image_overlay_rgba32_to_rgb888(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha)
{
   vc_image_quick_overlay_rgba32_to_rgb888(
      dest, x_offset, y_offset, width, height,
      src, src_x_offset, src_y_offset,
      NULL, 0, 0, alpha);
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   combine data and output rgb888, using only the 16x16 blocks indicated in
   block_mask
******************************************************************************/

void vc_image_quick_overlay_rgba32_to_rgb888(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   VC_IMAGE_T    const *block_mask,
   int                  block_x_offset,
   int                  block_y_offset,
   int                  alpha)
{
   unsigned char       *dptr;
   unsigned char const *sptr;
   unsigned long const *mptr;
   int                  mptr_pitch;
   int                  mshift;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB888) && (((unsigned long)dest->image_data | dest->pitch) & 15) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32) && (((unsigned long)src->image_data | src->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(block_mask, VC_IMAGE_1BPP));
   vcos_assert(width > 0 && height > 0);
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height,  0, 0, dest->width+15&~15, dest->height+15&~15));
   vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height,  0, 0, src->width+15&~15, src->height+15&~15));
   vcos_assert(vclib_check_VRF());

   if (block_mask != NULL)
   {
      mptr = (const unsigned long *)((char *)block_mask->image_data + block_y_offset * block_mask->pitch);
      mshift = block_x_offset & 31;
      mptr += block_x_offset >> 5;
      mptr_pitch = block_mask->pitch;
   }
   else
   {
      static const unsigned long allones = 0xFFFFFFFFul;
      mptr = &allones;
      mptr_pitch = 0;
      mshift = 32;
   }

   if (((x_offset | src_x_offset | width | height) & 15) == 0)
   {
      dptr = (unsigned char *)dest->image_data + (x_offset * 3) + y_offset * dest->pitch;
      sptr = (unsigned char const *)src->image_data + (src_x_offset << 2) + src_y_offset * src->pitch;
      width >>= 4;
      height >>= 4;

      while (--height >= 0)
      {
         int wcount = width;
         unsigned long const *tmpmptr = mptr;
         unsigned long tmpmshift = mshift;
         unsigned long blocks = *tmpmptr++;

         while (--wcount >= 0)
         {
            if (blocks & 1 << tmpmshift)
               rgba32_overlay_to_rgb888_blocky(dptr, dest->pitch, (unsigned long const *)sptr, src->pitch, alpha);
            dptr += 48;
            sptr += 64;
            if (++tmpmshift == 32)
            {
               blocks = *tmpmptr++;
               tmpmshift = 0;
            }
         }
         dptr += (dest->pitch << 4) - (width * 48);
         sptr += (src->pitch << 4) - (width << 6);
         mptr = (const unsigned long *)((char *)mptr + mptr_pitch);
      }
   }
   else
   {
      unsigned long long   maskleft,
      maskright;

      sptr = (unsigned char const *)src->image_data + ((src_x_offset & ~7) << 2) + src_y_offset * src->pitch;
      src_x_offset &= 7;
      width += src_x_offset;
      x_offset -= src_x_offset;

      dptr = (unsigned char *)dest->image_data + (x_offset * 3) + y_offset * dest->pitch;

      maskleft = 0xFFFFFFFFFFFFull << (src_x_offset * 3);
      width--;
      maskright = 0xFFFFFFFFFFFFull >> ((~width & 15) * 3);
      width >>= 4;

      if (width == 0)
         maskleft &= maskright;

      while (height >= 16)
      {
         int wcount = width;

         overlay_16_lines_to_rgb888(dptr, dest->pitch, (unsigned long const *)sptr, src->pitch, mptr, mshift, width, alpha, maskleft, maskright);

         dptr += dest->pitch << 4;
         sptr += src->pitch << 4;
         mptr = (const unsigned long *)((char *)mptr + mptr_pitch);
         height -= 16;
      }

      if (height > 0)
         overlay_n_lines_to_rgb888(dptr, dest->pitch, (unsigned long const *)sptr, src->pitch, mptr, mshift, width, height, alpha, maskleft, maskright);
   }
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   combine data and output rgba32.  src appears over the top of dest

TODO
   Consider an optimised _blocky variant -- although it's so slow it may be
   pointless.
******************************************************************************/

void vc_image_overlay_rgba32_to_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32) && (((unsigned long)src->image_data | src->pitch) & 31) == 0);
   vcos_assert(src->extra.rgba.component_order == dest->extra.rgba.component_order);
   vcos_assert(width > 0 && height > 0);
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height,  0, 0, dest->width+15&~15, dest->height+15&~15));
   vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height,  0, 0, src->width+15&~15, src->height+15&~15));
   vcos_assert(vclib_check_VRF());


   if (dest->extra.rgba.normalised_alpha)
   {
#if 0
      vcos_assert(!"normalised alpha isn't cool.");
      vc_image_premultiply_rgba32(dest);
#else
      vc_image_overlay_rgba32_to_rgba32_historical(
         dest, x_offset, y_offset, width, height,
         src, src_x_offset, src_y_offset, alpha);
      return;
#endif
   }

   vc_image_overlay_rgba32_to_rgba32_superawesome(
      dest, x_offset, y_offset, width, height,
      src, src_x_offset, src_y_offset, alpha);
}


static void vc_image_overlay_rgba32_to_rgba32_historical(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha)
{
   unsigned char       *dptr;
   unsigned char const *sptr;
   unsigned long        maskleft,
   maskright;

   vcos_assert(VC_IMAGE_ALPHA_OFFSET(src->extra.rgba.component_order) == 24);

   alpha += alpha >> 6;
   dptr = (unsigned char *)dest->image_data + ((x_offset & ~7) << 2) + y_offset * dest->pitch;
   x_offset &= 7;
   width += x_offset;
   src_x_offset -= x_offset;

   sptr = (unsigned char *)src->image_data + (src_x_offset << 2) + src_y_offset * src->pitch;

   maskleft = 0xFFFFFFFFul << (x_offset << 1);

   width--;
   maskright = 0xFFFFFFFFul >> ((~width & 15) << 1);
   width >>= 4;

   if (width == 0)
      maskleft &= maskright;

   while (height >= 16)
   {
      int wcount = width;

      rgba32_overlay_to_rgba32((unsigned long *)dptr, dest->pitch, (const unsigned long *)sptr, src->pitch, maskleft, alpha);
      dptr += 64; sptr += 64;
      if (--wcount >= 0)
      {
         while (--wcount >= 0)
         {
            rgba32_overlay_to_rgba32((unsigned long *)dptr, dest->pitch, (const unsigned long *)sptr, src->pitch, 0xFFFFFFFFul, alpha);
            dptr += 64; sptr += 64;
         }
         rgba32_overlay_to_rgba32((unsigned long *)dptr, dest->pitch, (const unsigned long *)sptr, src->pitch, maskright, alpha);
         dptr += 64; sptr += 64;
      }

      dptr += (dest->pitch << 4) - ((width + 1) << 6);
      sptr += (src->pitch << 4) - ((width + 1) << 6);
      height -= 16;
   }

   if (height > 0)
   {
      rgba32_overlay_to_rgba32_n((unsigned long *)dptr, dest->pitch, (const unsigned long *)sptr, src->pitch, height, maskleft, alpha);
      dptr += 64; sptr += 64;
      if (--width >= 0)
      {
         while (--width >= 0)
         {
            rgba32_overlay_to_rgba32_n((unsigned long *)dptr, dest->pitch, (const unsigned long *)sptr, src->pitch, height, 0xFFFFFFFFul, alpha);
            dptr += 64; sptr += 64;
         }
         rgba32_overlay_to_rgba32_n((unsigned long *)dptr, dest->pitch, (const unsigned long *)sptr, src->pitch, height, maskright, alpha);
      }
   }
}


static void vc_image_overlay_rgba32_to_rgba32_superawesome(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  alpha)
{
   unsigned long       *dptr;
   unsigned long const *sptr;
   int                  alphabits;

   alpha = min(max(0, alpha), 255);

   alphabits = VC_IMAGE_ALPHA_OFFSET(src->extra.rgba.component_order);

   dptr = (unsigned long *)((intptr_t)dest->image_data + x_offset * 4 + y_offset * dest->pitch);
   sptr = (const unsigned long *)((intptr_t)src->image_data + src_x_offset * 4 + src_y_offset * src->pitch);

   while (height > 0)
   {
      int stripeheight = min(height, 8);

      if (src->extra.rgba.normalised_alpha)
         rgba32_composite_normalised(dptr, dest->pitch, sptr, src->pitch, alphabits, width, stripeheight, alpha);
      else
         rgba32_composite(dptr, dest->pitch, sptr, src->pitch, alphabits, width, stripeheight, alpha);

      dptr = (unsigned long *)((intptr_t)dptr + dest->pitch * 8);
      sptr = (const unsigned long *)((intptr_t)sptr + src->pitch * 8);
      height -= 8;
   }
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   combine data and output rgba32.  src appears over the top of dest
******************************************************************************/

void vc_image_overlay_rgb565_to_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  transparent_colour,
   int                  alpha)
{
   unsigned char       *dptr;
   unsigned char const *sptr;
   unsigned short       maskleft,
   maskright;


   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGB565) && (((unsigned long)src->image_data | src->pitch) & 31) == 0);
   vcos_assert(width > 0 && height > 0);
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height,  0, 0, dest->width+15&~15, dest->height+15&~15));
   vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height,  0, 0, src->width+15&~15, src->height+15&~15));
   vcos_assert(vclib_check_VRF());

   dptr = (unsigned char *)dest->image_data + ((x_offset & ~7) << 2) + y_offset * dest->pitch;
   x_offset &= 7;
   width += x_offset;
   src_x_offset -= x_offset;

   sptr = (unsigned char const *)src->image_data + (src_x_offset << 1) + src_y_offset * src->pitch;

   maskleft = 0xFFFFu << x_offset;
   width--;
   maskright = 0xFFFFu >> (~width & 15);
   width >>= 4;

   if (width == 0)
      maskleft &= maskright;

   while (height >= 16)
   {
      int wcount = width;

      overlay_16_lines_from_rgb565((unsigned long *)dptr, dest->pitch, (unsigned short const *)sptr, src->pitch, width, transparent_colour, alpha, maskleft, maskright);

      dptr += dest->pitch << 4;
      sptr += src->pitch << 4;
      height -= 16;
   }

   if (height > 0)
      overlay_n_lines_from_rgb565((unsigned long *)dptr, dest->pitch, (unsigned short const *)sptr, src->pitch, width, height, transparent_colour, alpha, maskleft, maskright);
}

/* End of blending functions *********************************************}}}*/

/* Drawing primitives ****************************************************{{{*/

/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   combine data and output rgba32.  src and dest are mixed in proportion to
   their alpha values
******************************************************************************/

void vc_image_mix_rgba32_to_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32) && (((unsigned long)src->image_data | src->pitch) & 31) == 0);
   vcos_assert(vclib_check_VRF());

   vcos_assert(!"not implemented");
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   Force values into the alpha channel.

   low_alpha is the alpha value for a 0 bit, high_alpha is the alpha value
   for a 1 bit.
******************************************************************************/

void vc_image_rgba32_set_alpha_1bpp(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *mask,
   int                  mask_x_offset,
   int                  mask_y_offset,
   int                  low_alpha,
   int                  high_alpha)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(mask, VC_IMAGE_1BPP));
   vcos_assert(((x_offset | mask_x_offset) & 31) == 0);
   vcos_assert(vclib_check_VRF());

   vcos_assert(!"not implemented");
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   Force values into the alpha channel using an 8bpp source.

   alpha_scale gives the FSD value of the mask (either positive or negative),
   and alpha_boost gives the offset added afterwards.  A simple inversion of
   the mask would have a scale of -256, and a boost of 255.
******************************************************************************/

void vc_image_rgba32_set_alpha_8bpp(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *mask,
   int                  mask_x_offset,
   int                  mask_y_offset)
{
   unsigned char       *dptr;
   unsigned char const *mptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(mask, VC_IMAGE_8BPP));
   vcos_assert(((x_offset | mask_x_offset) & 15) == 0);
   vcos_assert(((width | height) & 15) == 0);
   vcos_assert(vclib_check_VRF());

   dptr = (unsigned char *)dest->image_data + (x_offset << 2) + y_offset * dest->pitch;
   mptr = (unsigned char *)mask->image_data + mask_x_offset + mask_y_offset * mask->pitch;

   /* does 16x16 cells */
   height >>= 4;
   width >>= 4;
   while (--height >= 0)
   {
      int wcount = width;
      while (--wcount >= 0)
      {
         /* Note: Mask is NOT 4.4. Assumes it's an 8-bit all alpha mask */
         rgba32_copy_alpha_from_8bpp(dptr, dest->pitch, mptr, mask->pitch);
         dptr += 64;
         mptr += 16;
      }
      dptr += (dest->pitch << 4) - (width << 6);
      mptr += (mask->pitch - width) << 4;
   }
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   Adjust the alpha channel with a 1bpp mask.

   low_alpha is the alpha value for a 0 bit, high_alpha is the alpha value
   for a 1 bit.
******************************************************************************/

void vc_image_rgba32_adjust_alpha_1bpp(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *mask,
   int                  mask_x_offset,
   int                  mask_y_offset,
   int                  low_alpha_scale,
   int                  high_alpha_scale)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(mask, VC_IMAGE_1BPP));
   vcos_assert(((x_offset | mask_x_offset) & 31) == 0);
   vcos_assert(vclib_check_VRF());

   vcos_assert(!"not implemented");
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   Adjust the alpha channel with an 8bpp mask.

   alpha_scale gives the FSD value of the mask (either positive or negative),
   and alpha_boost gives the offset added afterwards.  A simple inversion of
   the mask would have a scale of -256, and a boost of 255.
   Adjust the alpha channel with a 8bpp mask.
******************************************************************************/

void vc_image_rgba32_adjust_alpha_8bpp(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *mask,
   int                  mask_x_offset,
   int                  mask_y_offset,
   int                  alpha_scale,
   int                  alpha_boost)
{
   unsigned char       *dptr;
   unsigned char const *mptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(mask, VC_IMAGE_8BPP));
   vcos_assert(((x_offset | mask_x_offset) & 15) == 0);
   vcos_assert(((width | height) & 15) == 0);
   vcos_assert(vclib_check_VRF());

   dptr = (unsigned char *)dest->image_data + (x_offset << 2) + y_offset * dest->pitch;
   mptr = (unsigned char *)mask->image_data + mask_x_offset + mask_y_offset * mask->pitch;

   /* adjust alpha_scale so that 256 means 256 after it's been multiplied by 255 */
   alpha_scale += alpha_scale >> 7;

   height >>= 4;
   width >>= 4;
   while (--height >= 0)
   {
      int wcount = width;
      while (--wcount >= 0)
      {
         rgba32_alpha_adjust8(dptr, dest->pitch, mptr, mask->pitch, alpha_scale, alpha_boost);
         dptr += 64;
         mptr += 16;
      }
      dptr += (dest->pitch << 4) - (width << 6);
      mptr += (mask->pitch - width) << 4;
   }
}


/******************************************************************************
NAME
   vc_image_transpose_rgba32

PARAMETERS
   VC_IMAGE_T *dest  destination for transposed version of source image
   VC_IMAGE_T const *src  source image to be transposed

RETURNS
   void

FUNCTION
   Full image transpose (in-place illegal)
   Takes 16x16 tiles and transposes each to (transposed) destination directly
******************************************************************************/
void vc_image_transpose_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src)
{
   int i, j;
   const unsigned char * sptr;
   unsigned char* dptr;
   int src_pitch;
   int dst_pitch;

   if((src == NULL) || (dest == NULL) || (src == dest) ||
           (src->image_data == NULL) || (dest->image_data == NULL))
   {  // block potential badness
      vcos_assert(0);
      return;
   }

   src_pitch = src->pitch;
   dst_pitch = dest->pitch;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && ((((unsigned long)dest->image_data | dest->pitch) & 31) == 0) );
   vcos_assert(dest->pitch >= (((dest->width + 15) & ~15) << 2));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32) && !(((unsigned long)src->image_data | src->pitch) & 3));
   vcos_assert(src->pitch >= (((src->width + 15) & ~15) << 2));
   vcos_assert(vclib_check_VRF());

   for(i = 0; i < dest->height; i += 16)
   {
      sptr = ((const unsigned char *)(src->image_data)) + (i*4);
      dptr = ((unsigned char *)(dest->image_data)) + (i*dst_pitch);
      for(j = 0; j < dest->width; j += 16)
      {
#ifndef __CC_ARM
         _vasm("v32ld  H32(0++,0), (%r+=%r) REP 16", sptr, src_pitch);
         _vasm("v32st  V32(0,0++), (%r+=%r) REP 16", dptr, dst_pitch);
         _vasm("v32add V32(0,15), V32(0,15), 0");
#else
		 assert(0);
#endif
         sptr += src_pitch*16;
         dptr += 64;
      }
   }
}


/******************************************************************************
NAME
   vc_image_transpose_tiles_rgba32

PARAMETERS
   VC_IMAGE_T *dest        destination for transposed version of source image
   VC_IMAGE_T const *src   source image to be transposed
   int hdeci               decimation parameter

RETURNS
   void

FUNCTION
   transpose tiles (in-place OK)
******************************************************************************/

void vc_image_transpose_tiles_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, int hdeci)
{
   void (*func)(unsigned long *desta32, int dpitch, unsigned long const *srca32, int spitch) = rgba32_transpose_chunky;
   unsigned long *dptr;
   unsigned long const *sptr;
   int dst_pitch, dst_width, dst_height;
   int chunksize, x_offset, i;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32) && (((unsigned long)src->image_data | src->pitch) & 31) == 0);
   vcos_assert((dest->type == VC_IMAGE_RGBA32) || (dest->type == VC_IMAGE_ARGB8888) || (dest->type == VC_IMAGE_RGBX32) || (dest->type == VC_IMAGE_XRGB8888));
   vcos_assert((src->type == VC_IMAGE_RGBA32) || (src->type == VC_IMAGE_ARGB8888) || (src->type == VC_IMAGE_RGBX32) || (src->type == VC_IMAGE_XRGB8888));
   vcos_assert(vclib_check_VRF());

   chunksize = 64;

   if (hdeci & 2)
   {
      if (hdeci == 2)
         func = rgba32_transpose_chunky0;
      else
         func = rgba32_transpose_chunky1;

      hdeci &= ~3;
      chunksize >>= 1;
      vcos_assert(src->width == 32);
   }
   else
      vcos_assert(src->width == 16);
   vcos_assert(hdeci == 0);

   dst_pitch = (src->pitch * chunksize) >> 4;
   dst_width = (dst_pitch >> 2) & -chunksize;
   dst_height = (src->height + dst_width - 1) / dst_width << 4;

   vcos_assert(dest->size >= dst_pitch * dst_height);

   dptr = (unsigned long *)dest->image_data;
   sptr = (const unsigned long *)src->image_data;
   x_offset = 0;

   for (i = src->height; i > 0; i -= chunksize)
   {
      func(dptr + x_offset, dst_pitch, sptr, src->pitch);
      if ((x_offset += chunksize) >= dst_width)
      {
         x_offset = 0;
         dptr = (unsigned long *)((char *)dptr + (dst_pitch << 4));
      }
      sptr = (const unsigned long *)((char *)sptr + src->pitch * chunksize);
   }

   dest->width = dst_width;
   dest->pitch = dst_pitch;
   dest->height = dst_height;
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   vertical flip (in-place OK)
******************************************************************************/

void vc_image_vflip_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src)
{
   int src_pitch_bytes;
   int dest_pitch_bytes;
   int height;
   int nblocks;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(vclib_check_VRF());

   src_pitch_bytes = src->pitch;
   dest_pitch_bytes = dest->pitch;
   height = src->height;
   nblocks = (src->width+7)>>3; /* 8 RGBA32 pixels fit where 16 RGB565 pixels might have been */

   if (dest == src)
   {
      unsigned short *start = (unsigned short *)dest->image_data;
      unsigned short *end = (unsigned short *)dest->image_data + (height-1) * (dest_pitch_bytes>>1);

      vc_rgb565_vflip_in_place(start, end, dest_pitch_bytes, nblocks);
   }
   else
   {
      unsigned short *dptr = (unsigned short *)dest->image_data;
      unsigned short *sptr = (unsigned short *)src->image_data + (height-1) * (src_pitch_bytes>>1);

      vc_rgb565_vflip(dptr, sptr, src_pitch_bytes, height, nblocks, dest_pitch_bytes);
   }
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   horizontal flip (in-place OK)
******************************************************************************/

void vc_image_hflip_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src)
{
   int                  height,
   width,
   dpitch,
   spitch;
   unsigned long       *dptr,
   *sptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(vclib_check_VRF());

   width = dest->width;
   height = dest->height;
   dpitch = dest->pitch;
   spitch = src->pitch;
   dptr = (unsigned long *)dest->image_data;
   sptr = (unsigned long *)src->image_data;

   vcos_assert(dptr == sptr);

   while (height > 0)
   {
      rgba32_hflip_in_place(dptr, dpitch, width);

      dptr = (unsigned long *)((char *)dptr + (dpitch << 4));
      sptr = (unsigned long *)((char *)sptr + (spitch << 4));
      height -= 1 << 4;
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

void vc_image_resize_stripe_h_rgba32(VC_IMAGE_T *dest, int d_width, int s_width)
{
   int step;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32));
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
      vc_stripe_h_reduce_rgba322((unsigned long *)dest->image_data, dest->pitch, (const unsigned long *)dest->image_data, dest->pitch, s_width, 0x40000);
   }

   step = ((s_width << 16) + (d_width >> 1)) / d_width;

   if (step != 0x10000)
   {
      switch (_msb((step - 1) | 0x1000))
      {
      case 17: /* 2.0 < step <= 4.0 */
         vc_stripe_h_reduce_rgba322((unsigned long *)dest->image_data, dest->pitch, (const unsigned long *)dest->image_data, dest->pitch, d_width, step);
         break;
      case 16: /* 1.0 < step <= 2.0 */
         vc_stripe_h_reduce_rgba32((unsigned long *)dest->image_data, dest->pitch, (const unsigned long *)dest->image_data, dest->pitch, d_width, step);
         break;
      case 12: /* 0.0625 < step <= 0.125 */
      case 13: /* 0.125 < step <= 0.25 */
      case 14: /* 0.25 < step <= 0.5 */
      case 15: /* 0.5 < step <= 1.0 */
#if 1
         {
            int off;

            off = (dest->pitch - (s_width<<2)) & ~31;

            vclib_memmove((char *)dest->image_data + off, dest->image_data, dest->pitch * 16 - off);

            vc_stripe_h_expand_rgba32((unsigned long *)dest->image_data, dest->pitch, (const unsigned long *)((char *)dest->image_data + off), dest->pitch, d_width, step, s_width);
         }
#else
         vc_stripe_h_expand_rgba32((unsigned long *)dest->image_data + d_width, dest->pitch, (unsigned long *)dest->image_data + s_width, dest->pitch, d_width, step);
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

void vc_image_resize_stripe_v_rgba32(VC_IMAGE_T *dest, int d_height, int s_height)
{
   int step;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32));
   vcos_assert(d_height > 0 && s_height > 0);
   vcos_assert(dest->width == 16);
   vcos_assert(dest->height >= s_height);
   vcos_assert(dest->height >= d_height);

   if (d_height <= 0)
      return;

   while (d_height << 2 < s_height)
   {
      s_height >>= 2;
      vc_stripe_v_reduce_bytes2((unsigned char *)((char *)dest->image_data), dest->pitch, (const unsigned char *)((char *)dest->image_data), dest->pitch, s_height, 0x40000);
      vc_stripe_v_reduce_bytes2((unsigned char *)((char *)dest->image_data + 16), dest->pitch, (const unsigned char *)((char *)dest->image_data + 16), dest->pitch, s_height, 0x40000);
      vc_stripe_v_reduce_bytes2((unsigned char *)((char *)dest->image_data + 32), dest->pitch, (const unsigned char *)((char *)dest->image_data + 32), dest->pitch, s_height, 0x40000);
      vc_stripe_v_reduce_bytes2((unsigned char *)((char *)dest->image_data + 48), dest->pitch, (const unsigned char *)((char *)dest->image_data + 48), dest->pitch, s_height, 0x40000);
   }

   step = ((s_height << 16) + (d_height >> 1)) / d_height;

   if (step != 0x10000)
   {
      switch (_msb((step - 1) | 0x1000))
      {
      case 17: /* 2.0 < step <= 4.0 */
         vc_stripe_v_reduce_bytes2((unsigned char *)((char *)dest->image_data), dest->pitch, (const unsigned char *)((char *)dest->image_data), dest->pitch, d_height, step);
         vc_stripe_v_reduce_bytes2((unsigned char *)((char *)dest->image_data + 16), dest->pitch, (const unsigned char *)((char *)dest->image_data + 16), dest->pitch, d_height, step);
         vc_stripe_v_reduce_bytes2((unsigned char *)((char *)dest->image_data + 32), dest->pitch, (const unsigned char *)((char *)dest->image_data + 32), dest->pitch, d_height, step);
         vc_stripe_v_reduce_bytes2((unsigned char *)((char *)dest->image_data + 48), dest->pitch, (const unsigned char *)((char *)dest->image_data + 48), dest->pitch, d_height, step);
         break;
      case 16: /* 1.0 < step <= 2.0 */
         vc_stripe_v_reduce_bytes((unsigned char *)((char *)dest->image_data), dest->pitch, (const unsigned char *)((char *)dest->image_data), dest->pitch, d_height, step);
         vc_stripe_v_reduce_bytes((unsigned char *)((char *)dest->image_data + 16), dest->pitch, (const unsigned char *)((char *)dest->image_data + 16), dest->pitch, d_height, step);
         vc_stripe_v_reduce_bytes((unsigned char *)((char *)dest->image_data + 32), dest->pitch, (const unsigned char *)((char *)dest->image_data + 32), dest->pitch, d_height, step);
         vc_stripe_v_reduce_bytes((unsigned char *)((char *)dest->image_data + 48), dest->pitch, (const unsigned char *)((char *)dest->image_data + 48), dest->pitch, d_height, step);
         break;
      case 12: /* 0.0625 < step <= 0.125 */
      case 13: /* 0.125 < step <= 0.25 */
      case 14: /* 0.25 < step <= 0.5 */
      case 15: /* 0.5 < step <= 1.0 */
#if 1
         {
            int off;
            s_height = s_height+1&~1;
            d_height = d_height+1&~1;

            off = ((dest->height + 15 & ~15) - s_height) * dest->pitch;

            vclib_memmove((void *)((char *)dest->image_data + off), dest->image_data, dest->pitch * s_height);

            vc_stripe_v_expand_bytes((unsigned char *)((char *)dest->image_data), dest->pitch, (const unsigned char*)((char *)dest->image_data + off), dest->pitch, d_height, step, s_height);
            vc_stripe_v_expand_bytes((unsigned char *)((char *)dest->image_data + 16), dest->pitch, (const unsigned char*)((char *)dest->image_data + 16 + off), dest->pitch, d_height, step, s_height);
            vc_stripe_v_expand_bytes((unsigned char *)((char *)dest->image_data + 32), dest->pitch, (const unsigned char*)((char *)dest->image_data + 32 + off), dest->pitch, d_height, step, s_height);
            vc_stripe_v_expand_bytes((unsigned char *)((char *)dest->image_data + 48), dest->pitch, (const unsigned char*)((char *)dest->image_data + 48 + off), dest->pitch, d_height, step, s_height);
         }
#else
         vc_stripe_v_expand_bytes((unsigned char *)((char *)dest->image_data + dest->pitch * d_height), dest->pitch, (void *)((char *)dest->image_data + dest->pitch * s_height), dest->pitch, d_height, step);
         vc_stripe_v_expand_bytes((unsigned char *)((char *)dest->image_data + 16 + dest->pitch * d_height), dest->pitch, (const unsigned char*)((char *)dest->image_data + 16 + dest->pitch * s_height), dest->pitch, d_height, step);
         vc_stripe_v_expand_bytes((unsigned char *)((char *)dest->image_data + 32 + dest->pitch * d_height), dest->pitch, (const unsigned char*)((char *)dest->image_data + 32 + dest->pitch * s_height), dest->pitch, d_height, step);
         vc_stripe_v_expand_bytes((unsigned char *)((char *)dest->image_data + 48 + dest->pitch * d_height), dest->pitch, (const unsigned char*)((char *)dest->image_data + 48 + dest->pitch * s_height), dest->pitch, d_height, step);
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
   force region in dest to be the specified value
******************************************************************************/

void vc_image_fill_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   long                 value)
{
   unsigned char *dptr;

   vcos_assert((is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) || is_valid_vc_image_buf(dest, VC_IMAGE_ARGB8888) || is_valid_vc_image_buf(dest, VC_IMAGE_XRGB8888)) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height, 0, 0, dest->width, dest->height));
   vcos_assert(vclib_check_VRF());

   dptr = (unsigned char *)dest->image_data + (x_offset << 2) + y_offset * dest->pitch;

   if (width >= dest->width)
      vclib_memset4(dptr, value, height * dest->pitch >> 2);
   else
   {
      while (--height >= 0)
      {
         vclib_memset4(dptr, value, width);
         dptr += dest->pitch;
      }
   }
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   mix the specified value into a region in dest
******************************************************************************/

void vc_image_tint_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   long                 value)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);
   vcos_assert(vclib_check_VRF());

   vcos_assert(!"not implemented");
}

/* End of drawing primitives *********************************************}}}*/

/* Text functions ********************************************************{{{*/

/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   x
******************************************************************************/

void vc_image_text_rgba32 (VC_IMAGE_T *dest, int x_offset, int y_offset, long fg_colour, long bg_colour, char const *text,
                           int *max_x, int *max_y)
{
   int start, end;
   int y;
   int sx = x_offset, sy = y_offset;

   if ( (NULL != dest) && (VC_IMAGE_RGBA32 == dest->type) && ((((unsigned long)dest->image_data | dest->pitch) & 31) == 0) )
   {
      int pitch = dest->pitch; // in pixels

      *max_y = min(y_offset+12, dest->height)-1;

      for ( ; text[0] !=0 ; text++)
      {
         int c=(int)text[0]-32;

         if (c==0)
         {
            start = 0;
         }
         else
         {
            if ((c<0) || (c>=96)) c=1; // Change unknown characters to !
            // Now compute location of the text in memory
            start = fontpos16[c-1];
         }
         end = fontpos16[c];

         if (sx + end - start >= dest->width) {
            end = dest->width + start - sx; }

         for ( ; start < end; start++, sx++)
         {
            unsigned char *b;
            int data;

            if (sx < 0) continue;

            /* Needs to be displayed at location sx,sy */
            if (fg_colour == VC_IMAGE_COLOUR_TRANSPARENT)
               continue;

            b = (unsigned char *)dest->image_data + sy*pitch + sx*4;
            data = fontdata16[start] << 3;  /* This contains packed binary for the letter */

            for ( y = sy; y <= *max_y; y++, data <<= 1, b += pitch)
            {
               if (y < 0) continue;

               if (data&(1<<15))
               {
                  // one means background
                  if (bg_colour == VC_IMAGE_COLOUR_TRANSPARENT)
                  {
                     // do nothing
                  }
                  else if (bg_colour == VC_IMAGE_COLOUR_FADE)
                  {
                     // wipe out bottom bits and shift down
                     b[0] >>= 1;
                     b[1] >>= 1;
                     b[2] >>= 1;
                     b[3] = 0xFF; //TODO
                  }
                  else
                  {
                     b[0] = bg_colour&0xff;
                     b[1] = (bg_colour>>8)&0xff;
                     b[2] = bg_colour>>16;
                     b[3] = 0xFF; //TODO
                  }
               }
               else
               {
                  b[0] = fg_colour&0xff;
                  b[1] = (fg_colour>>8)&0xff;
                  b[2] = fg_colour>>16;
                  b[3] = 0xFF; //TODO
               }
            }
         }
      }

      *max_x = sx;
   }
   else
      vcos_assert(!"paramter error!");
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   x
******************************************************************************/

void vc_image_small_text_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   long                 fg_colour,
   long                 bg_colour,
   char          const *text,
   int                 *max_x,
   int                 *max_y)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);

   vcos_assert(!"not implemented");
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   x
******************************************************************************/

void vc_image_text_32_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   long                 fg_colour,
   long                 bg_colour,
   char          const *text,
   int                 *max_x,
   int                 *max_y)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);

   vcos_assert(!"not implemented");
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   x
******************************************************************************/

void vc_image_text_24_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   long                 fg_colour,
   long                 bg_colour,
   char          const *text,
   int                 *max_x,
   int                 *max_y)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);

   vcos_assert(!"not implemented");
}


/******************************************************************************
NAME
   x

SYNOPSIS
   void x(y)

FUNCTION
   x
******************************************************************************/

void vc_image_text_20_rgba32(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   long                 fg_colour,
   long                 bg_colour,
   char          const *text,
   int                 *max_x,
   int                 *max_y)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) && (((unsigned long)dest->image_data | dest->pitch) & 31) == 0);

   vcos_assert(!"not implemented");
}

/* End of text functions *************************************************}}}*/


/******************************************************************************
NAME
   vc_image_premultiply_rgba32

SYNOPSIS
   void vc_image_premultiply_rgba32(VC_IMAGE_T *dest)

FUNCTION
   Convert an image from normalised to premultiplied alpha

RETURNS
   void
******************************************************************************/

void vc_image_premultiply_rgba32(VC_IMAGE_T *dest)
{
   unsigned long *dptr;
   int pitch, alphabits, count, hcount;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32) || is_valid_vc_image_buf(dest, VC_IMAGE_ARGB8888));
   vcos_assert(dest->extra.rgba.normalised_alpha != 0);

   dptr = (unsigned long *)dest->image_data;
   pitch = dest->pitch;
   alphabits = VC_IMAGE_ALPHA_OFFSET(dest->extra.rgba.component_order);

   count = dest->height + 15 >> 4;
   hcount = dest->width + 15 >> 4;

   while (--count >= 0)
   {
      rgba32_premultiply(dptr, pitch, dptr, pitch, alphabits, hcount);

      dptr = (unsigned long *)((char *)dptr + dest->pitch * 16);
   }

   dest->extra.rgba.normalised_alpha = 0;
}



/******************************************************************************
Static function definitions.
******************************************************************************/

static void overlay_16_lines_to_rgb888(
   unsigned char       *dptr,
   int                  dpitch,
   unsigned long const *sptr,
   int                  spitch,
   unsigned long const *mptr,
   int                  mshift,
   int                  width,
   int                  alpha,
   unsigned long long   maskleft,
   unsigned long long   maskright)
{
   unsigned long        blockmask;

   blockmask = *mptr++;

   if (blockmask & 1 << mshift)
      rgba32_overlay_to_rgb888(dptr, dpitch, sptr, spitch, alpha, maskleft);
   dptr += 16 * 3;
   sptr += 16;
   if (++mshift == 32)
   {
      blockmask = *mptr++;
      mshift = 0;
   }
   if (--width >= 0)
   {
      while (--width >= 0)
      {
         if (blockmask & 1 << mshift)
            rgba32_overlay_to_rgb888(dptr, dpitch, sptr, spitch, alpha, 0xFFFFFFFFFFFFull);
         dptr += 16 * 3;
         sptr += 16;
         if (++mshift == 32)
         {
            blockmask = *mptr++;
            mshift = 0;
         }
      }
      if (blockmask & 1 << mshift)
         rgba32_overlay_to_rgb888(dptr, dpitch, sptr, spitch, alpha, maskright);
   }
}


static void overlay_n_lines_to_rgb888(
   unsigned char       *dptr,
   int                  dpitch,
   unsigned long const *sptr,
   int                  spitch,
   unsigned long const *mptr,
   int                  mshift,
   int                  width,
   int                  height,
   int                  alpha,
   unsigned long long   maskleft,
   unsigned long long   maskright)
{
   unsigned long        blockmask;

   blockmask = *mptr++;

   if (blockmask & 1 << mshift)
      rgba32_overlay_to_rgb888_n(dptr, dpitch, sptr, spitch, alpha, height, maskleft);
   dptr += 16 * 3;
   sptr += 16;
   if (++mshift == 32)
   {
      blockmask = *mptr++;
      mshift = 0;
   }
   if (--width >= 0)
   {
      while (--width >= 0)
      {
         if (blockmask & 1 << mshift)
            rgba32_overlay_to_rgb888_n(dptr, dpitch, sptr, spitch, alpha, height, 0xFFFFFFFFFFFFull);
         dptr += 16 * 3;
         sptr += 16;
         if (++mshift == 32)
         {
            blockmask = *mptr++;
            mshift = 0;
         }
      }
      if (blockmask & 1 << mshift)
         rgba32_overlay_to_rgb888_n(dptr, dpitch, sptr, spitch, alpha, height, maskright);
   }
}


static void overlay_16_lines_to_rgb565(
   unsigned short      *dptr,
   int                  dpitch,
   unsigned long const *sptr,
   int                  spitch,
   unsigned long const *mptr,
   int                  mshift,
   int                  width,
   int                  alpha,
   unsigned short       maskleft,
   unsigned short       maskright)
{
   unsigned long        blockmask;

   blockmask = *mptr++;

   if (blockmask & 1 << mshift)
      rgba32_overlay_to_rgb565(dptr, dpitch, sptr, spitch, alpha, maskleft);
   dptr += 16;
   sptr += 16;
   if (++mshift == 32)
   {
      blockmask = *mptr++;
      mshift = 0;
   }
   if (--width >= 0)
   {
      while (--width >= 0)
      {
         if (blockmask & 1 << mshift)
            rgba32_overlay_to_rgb565(dptr, dpitch, sptr, spitch, alpha, 0xFFFFu);
         dptr += 16;
         sptr += 16;
         if (++mshift == 32)
         {
            blockmask = *mptr++;
            mshift = 0;
         }
      }
      if (blockmask & 1 << mshift)
         rgba32_overlay_to_rgb565(dptr, dpitch, sptr, spitch, alpha, maskright);
   }
}


static void overlay_n_lines_to_rgb565(
   unsigned short      *dptr,
   int                  dpitch,
   unsigned long const *sptr,
   int                  spitch,
   unsigned long const *mptr,
   int                  mshift,
   int                  width,
   int                  height,
   int                  alpha,
   unsigned short       maskleft,
   unsigned short       maskright)
{
   unsigned long        blockmask;

   blockmask = *mptr++;

   if (blockmask & 1 << mshift)
      rgba32_overlay_to_rgb565_n(dptr, dpitch, sptr, spitch, alpha, height, maskleft);
   dptr += 16;
   sptr += 16;
   if (++mshift == 32)
   {
      blockmask = *mptr++;
      mshift = 0;
   }
   if (--width >= 0)
   {
      while (--width >= 0)
      {
         if (blockmask & 1 << mshift)
            rgba32_overlay_to_rgb565_n(dptr, dpitch, sptr, spitch, alpha, height, 0xFFFFu);
         dptr += 16;
         sptr += 16;
         if (++mshift == 32)
         {
            blockmask = *mptr++;
            mshift = 0;
         }
      }
      if (blockmask & 1 << mshift)
         rgba32_overlay_to_rgb565_n(dptr, dpitch, sptr, spitch, alpha, height, maskright);
   }
}


static void overlay_16_lines_from_rgb565(
   unsigned long       *dptr,
   int                  dpitch,
   unsigned short const*sptr,
   int                  spitch,
   int                  width,
   int                  transparent_colour,
   int                  alpha,
   unsigned short       maskleft,
   unsigned short       maskright)
{
   overlay_n_lines_from_rgb565(
      dptr, dpitch, sptr, spitch,
      width, 16,
      transparent_colour, alpha, maskleft, maskright);
}


static void overlay_n_lines_from_rgb565(
   unsigned long       *dptr,
   int                  dpitch,
   unsigned short const*sptr,
   int                  spitch,
   int                  width,
   int                  height,
   int                  transparent_colour,
   int                  alpha,
   unsigned short       maskleft,
   unsigned short       maskright)
{
   vcos_assert((unsigned long)transparent_colour <= 0xFFFF);
   rgb565_overlay_to_rgba32_n(dptr, dpitch, sptr, spitch, height, maskleft, transparent_colour, alpha);
   dptr += 16;
   sptr += 16;
   if (--width >= 0)
   {
      while (--width >= 0)
      {
         rgb565_overlay_to_rgba32_n(dptr, dpitch, sptr, spitch, height, 0xFFFFu, transparent_colour, alpha);
         dptr += 16;
         sptr += 16;
      }
      rgb565_overlay_to_rgba32_n(dptr, dpitch, sptr, spitch, height, maskright, transparent_colour, alpha);
   }
}

#if _VC_VERSION >= 3
/******************************************************************************
NAME
   vc_image_font_alpha_blt_rgba32

SYNOPSIS
   void vc_image_font_alpha_blt_rgba32(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
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

extern void vc_rgba32_font_alpha_blt(unsigned long *dest, unsigned char *src, int dest_pitch_bytes,
                                        int src_pitch_bytes, int width, int height,int red,int green,int blue,int alpha);

void vc_image_font_alpha_blt_rgba32 (VC_IMAGE_T *dest_image, int x_offset, int y_offset, int width, int height,
                                     VC_IMAGE_T *src_image, int src_x_offset, int src_y_offset, int red,int green,int blue,int alpha)
{
   int dest_pitch_bytes, src_pitch_bytes;
   int dest_pitch_pixels, src_pitch_pixels;
   unsigned long *dest;
   unsigned char *src;

   dest_pitch_bytes = dest_image->pitch;
   src_pitch_bytes = src_image->pitch;
   dest_pitch_pixels = dest_pitch_bytes>>2;
   src_pitch_pixels = src_pitch_bytes;
   dest = ((unsigned long *)dest_image->image_data) + y_offset*dest_pitch_pixels + x_offset;
   src = ((unsigned char *)src_image->image_data) + src_y_offset*src_pitch_pixels + src_x_offset;

   vc_rgba32_font_alpha_blt(dest, src, dest_pitch_bytes, src_pitch_bytes, width, height, red,green,blue,alpha);

}
#endif

