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
#include <stddef.h>

/* Project includes */
#include "vcinclude/vcore.h"        // #include <vc/intrinsics.h>
#include "vclib/vclib.h"
#include "striperesize.h"
#include "vc_image.h"
#include "interface/vcos/vcos_assert.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vc_image_yuv422_to_rgba565 (VC_IMAGE_T *dest, VC_IMAGE_T const *src);
extern void vc_image_yuv420_to_rgba565 (VC_IMAGE_T *dest, VC_IMAGE_T const *src);
extern void vc_image_rgb565_to_rgba565 (VC_IMAGE_T *dest, VC_IMAGE_T const *src);
extern void vc_image_rgba32_to_rgba565 (VC_IMAGE_T *dest, VC_IMAGE_T const *src);
extern void vc_image_rgba565_to_rgba32 (VC_IMAGE_T *dest, VC_IMAGE_T const *src);
extern void vc_image_rgba32_to_rgba565_part (VC_IMAGE_T *dest, int x_offset, int y_offset,
         int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset);
extern void vc_image_rgba565_to_rgb888_part (VC_IMAGE_T *dest, int x_offset, int y_offset,
         int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset);
extern void vc_image_rgba565_to_rgb565_part (VC_IMAGE_T *dest, int x_offset, int y_offset,
         int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset);
extern void vc_image_overlay_rgba32_to_rgba565 (VC_IMAGE_T *dest, int x_offset, int y_offset,
         int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha);
extern void vc_image_overlay_rgba565_to_rgba32 (VC_IMAGE_T *dest, int x_offset, int y_offset,
         int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha);
extern void vc_image_overlay_rgb565_to_rgba565 (VC_IMAGE_T *dest, int x_offset, int y_offset,
         int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int transparent_colour);
extern void vc_image_overlay_rgba565_to_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset,
         int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha);
extern void vc_image_overlay_rgba565_to_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset,
         int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha);
extern void vc_image_overlay_rgba565_to_rgba565 (VC_IMAGE_T *dest, int x_offset, int y_offset,
         int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, int alpha);
extern void vc_image_quick_overlay_rgba565_to_rgb565 (VC_IMAGE_T *dest, int x_offset, int y_offset,
         int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset,
         VC_IMAGE_T const *block_mask, int block_x_offset, int block_y_offset, int alpha);
extern void vc_image_quick_overlay_rgba565_to_rgb888 (VC_IMAGE_T *dest, int x_offset, int y_offset,
         int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset,
         VC_IMAGE_T const *block_mask, int block_x_offset, int block_y_offset, int alpha);
extern void vc_image_resize_stripe_h_rgba565 (VC_IMAGE_T *dest, int d_width, int s_width);
extern void vc_image_resize_stripe_v_rgba565 (VC_IMAGE_T *dest, int d_height, int s_height);


/* Check extern defs match above and loads #defines and typedefs */
#include "vc_image_rgba565.h"

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

extern void rgba565_from_rgba32_blocky(
      unsigned short      *desta565,   /* r0 */
      int                  dpitch,     /* r1 */
      unsigned long const *srca32,     /* r2 */
      int                  spitch,     /* r3 */
      int                  width,      /* r4 * in pixels */
      int                  height);    /* r5 * in pixels */

extern void rgba565_from_rgba32(
      unsigned short      *desta565,   /* r0 */
      int                  dpitch,     /* r1 */
      unsigned long const *srca32,     /* r2 */
      int                  spitch,     /* r3 */
      int                  width,      /* r4 * in pixels */
      int                  height);    /* r5 * in pixels */

extern void rgba565_to_rgba32_blocky(
      unsigned short      *desta32,    /* r0 */
      int                  dpitch,     /* r1 */
      unsigned long const *srca565,    /* r2 */
      int                  spitch,     /* r3 */
      int                  width,      /* r4 * in pixels */
      int                  height);    /* r5 * in pixels */

extern void rgba565_from_rgb565_blocky(
      unsigned short      *desta565,   /* r0 */
      int                  dpitch,     /* r1 */
      unsigned short const*src565,     /* r2 */
      int                  spitch,     /* r3 */
      int                  width,      /* r4 * in pixels */
      int                  height);    /* r5 * in pixels */

extern void rgba32_overlay_to_rgba565(
      unsigned short      *desta565,   /* r0 */
      int                  dpitch,     /* r1 */
      unsigned long const *srca32,     /* r2 */
      int                  spitch,     /* r3 */
      int                  width,      /* r4 * in pixels */
      int                  height,     /* r5 * in pixels */
      int                  ascale);    /* (sp) */

extern void rgba565_overlay_to_rgba32(
      unsigned long       *desta32,    /* r0 */
      int                  dpitch,     /* r1 */
      unsigned short const*srca565,    /* r2 */
      int                  spitch,     /* r3 */
      int                  width,      /* r4 * in pixels */
      int                  height,     /* r5 * in pixels */
      int                  ascale);    /* (sp) */

extern void rgb565_overlay_to_rgba565(
      unsigned short      *desta565,   /* r0 */
      int                  dpitch,     /* r1 */
      unsigned short const*src565,     /* r2 */
      int                  spitch,     /* r3 */
      int                  width,      /* r4 * in pixels */
      int                  height,     /* r5 * in pixels */
      int                  transparent_colour); /* (sp) */

extern void rgba565_to_rgb565(
      unsigned short      *dest565,    /* r0 */
      int                  dpitch,     /* r1 */
      unsigned short const*srca565,    /* r2 */
      int                  spitch,     /* r3 */
      int                  width,      /* r4 * in pixels */
      int                  height);    /* r5 * in pixels */

extern void rgba565_overlay_to_rgb565(
      unsigned short      *dest565,    /* r0 */
      int                  dpitch,     /* r1 */
      unsigned short const*srca565,    /* r2 */
      int                  spitch,     /* r3 */
      int                  width,      /* r4 * in pixels */
      int                  height);    /* r5 * in pixels */

extern void rgba565_overlay_to_rgba565(
      unsigned short      *desta565,   /* r0 */
      int                  dpitch,     /* r1 */
      unsigned short const*srca565,    /* r2 */
      int                  spitch,     /* r3 */
      int                  width,      /* r4 * in pixels */
      int                  height);    /* r5 * in pixels */

extern void rgba565_to_rgb888(
      unsigned char       *dest888,    /* r0 */
      int                  dpitch,     /* r1 */
      unsigned short const*srca565,    /* r2 */
      int                  spitch,     /* r3 */
      int                  width,      /* r4 * in pixels */
      int                  height);    /* r5 * in pixels */

extern void rgba565_overlay_to_rgb888(
      unsigned char       *dest888,    /* r0 */
      int                  dpitch,     /* r1 */
      unsigned short const*srca565,    /* r2 */
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


/******************************************************************************
Global function definitions.
******************************************************************************/

/* Conversions ***********************************************************{{{*/

/******************************************************************************
NAME
   vc_image_yuv422_to_rgba565

SYNOPSIS
   void vc_image_yuv422_to_rgba565(
      VC_IMAGE_T          *dest,
      VC_IMAGE_T    const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_yuv422_to_rgba565(
   VC_IMAGE_T          *dest,
   VC_IMAGE_T    const *src)
{
   vcos_assert(!"Not implemented");
}


/******************************************************************************
NAME
   vc_image_yuv420_to_rgba565

SYNOPSIS
   void vc_image_yuv420_to_rgba565(
      VC_IMAGE_T          *dest,
      VC_IMAGE_T    const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_yuv420_to_rgba565(
   VC_IMAGE_T          *dest,
   VC_IMAGE_T    const *src)
{
   vcos_assert(!"Not implemented");
}


/******************************************************************************
NAME
   vc_image_rgb565_to_rgba565

SYNOPSIS
   void vc_image_rgb565_to_rgba565(
      VC_IMAGE_T          *dest,
      VC_IMAGE_T    const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_rgb565_to_rgba565(
   VC_IMAGE_T          *dest,
   VC_IMAGE_T    const *src)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA565) || is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA565) || is_valid_vc_image_buf(src, VC_IMAGE_RGB565));

   rgba565_from_rgb565_blocky(
      dest->image_data, dest->pitch,
      src->image_data, src->pitch,
      dest->width,
      dest->height);
}


/******************************************************************************
NAME
   vc_image_rgba32_to_rgba565

SYNOPSIS
   void vc_image_rgba32_to_rgba565(
      VC_IMAGE_T          *dest,
      VC_IMAGE_T    const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_rgba32_to_rgba565(
   VC_IMAGE_T          *dest,
   VC_IMAGE_T    const *src)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32));

   rgba565_from_rgba32_blocky(
      dest->image_data, dest->pitch,
      src->image_data, src->pitch,
      dest->width,
      dest->height);
}


/******************************************************************************
NAME
   vc_image_rgba565_to_rgba32

SYNOPSIS
   void vc_image_rgba565_to_rgba32(
      VC_IMAGE_T          *dest,
      VC_IMAGE_T    const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_rgba565_to_rgba32(
   VC_IMAGE_T          *dest,
   VC_IMAGE_T    const *src)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA565));

   rgba565_to_rgba32_blocky(
      dest->image_data, dest->pitch,
      src->image_data, src->pitch,
      dest->width,
      dest->height);
}


/******************************************************************************
NAME
   vc_image_rgba32_to_rgba565_part

SYNOPSIS
   void vc_image_rgba32_to_rgba565_part(
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

void vc_image_rgba32_to_rgba565_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset)
{
   unsigned short      *dptr;
   unsigned long const *sptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32));

   dptr = (void *)((char *)dest->image_data + dest->pitch * y_offset);
   sptr = (void *)((char *)src->image_data + src->pitch * src_y_offset);
   dptr += x_offset;
   sptr += src_x_offset;

   while (height >= 16)
   {
      rgba565_from_rgba32(dptr, dest->pitch, sptr, src->pitch,
                          width, 16);
      dptr = (void *)((char *)dptr + (dest->pitch << 4));
      sptr = (void *)((char *)sptr + (src->pitch << 4));
      height -= 16;
   }

   if (height > 0)
   {
      rgba565_from_rgba32(dptr, dest->pitch, sptr, src->pitch,
                          width, height);
   }
}

/******************************************************************************
NAME
   vc_image_rgba565_to_rgb888_part

SYNOPSIS
   void vc_image_rgba565_to_rgb888_part(
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

void vc_image_rgba565_to_rgb888_part(
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
   unsigned short const*sptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB888));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA565));

   dptr = (void *)((char *)dest->image_data + dest->pitch * y_offset);
   sptr = (void *)((char *)src->image_data + src->pitch * src_y_offset);
   dptr += x_offset * 3;
   sptr += src_x_offset;

   while (height >= 16)
   {
      rgba565_to_rgb888(dptr, dest->pitch, sptr, src->pitch,
                        width, 16);
      dptr = (void *)((char *)dptr + (dest->pitch << 4));
      sptr = (void *)((char *)sptr + (src->pitch << 4));
      height -= 16;
   }

   if (height > 0)
   {
      rgba565_to_rgb888(dptr, dest->pitch, sptr, src->pitch,
                        width, height);
   }
}


/******************************************************************************
NAME
   vc_image_rgba565_to_rgb565_part

SYNOPSIS
   void vc_image_rgba565_to_rgb565_part(
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

void vc_image_rgba565_to_rgb565_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset)
{
   unsigned short      *dptr;
   unsigned short const*sptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA565));

   dptr = (void *)((char *)dest->image_data + dest->pitch * y_offset);
   sptr = (void *)((char *)src->image_data + src->pitch * src_y_offset);
   dptr += x_offset;
   sptr += src_x_offset;

   while (height >= 16)
   {
      rgba565_to_rgb565(dptr, dest->pitch, sptr, src->pitch,
                        width, 16);
      dptr = (void *)((char *)dptr + (dest->pitch << 4));
      sptr = (void *)((char *)sptr + (src->pitch << 4));
      height -= 16;
   }

   if (height > 0)
   {
      rgba565_to_rgb565(dptr, dest->pitch, sptr, src->pitch,
                        width, height);
   }
}

/* End of conversions ****************************************************}}}*/

/* Blending functions ****************************************************{{{*/

/******************************************************************************
NAME
   vc_image_overlay_rgba32_to_rgba565

SYNOPSIS
   void vc_image_overlay_rgba32_to_rgba565(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T    const *src,
      int                  src_x_offset,
      int                  src_y_offset,
      int                  alpha);

FUNCTION
   combine data and output rgba565.
******************************************************************************/

void vc_image_overlay_rgba32_to_rgba565(
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
   unsigned short      *dptr;
   unsigned long const *sptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32));

   alpha += alpha >> 6;
   if (alpha > 0x102)
      alpha = 0x102;

   dptr = (void *)((char *)dest->image_data + dest->pitch * y_offset);
   sptr = (void *)((char *)src->image_data + src->pitch * src_y_offset);
   dptr += x_offset;
   sptr += src_x_offset;

   while (height >= 16)
   {
      rgba32_overlay_to_rgba565(dptr, dest->pitch, sptr, src->pitch,
                                width, 16, alpha);
      dptr = (void *)((char *)dptr + (dest->pitch << 4));
      sptr = (void *)((char *)sptr + (src->pitch << 4));
      height -= 16;
   }

   if (height > 0)
   {
      rgba32_overlay_to_rgba565(dptr, dest->pitch, sptr, src->pitch,
                                width, height, alpha);
   }
}

/******************************************************************************
NAME
   vc_image_overlay_rgba565_to_rgba32

SYNOPSIS
   void vc_image_overlay_rgba565_to_rgba32(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T    const *src,
      int                  src_x_offset,
      int                  src_y_offset,
      int                  alpha);

FUNCTION
   combine data and output rgba32.
******************************************************************************/

void vc_image_overlay_rgba565_to_rgba32(
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
   unsigned short const*sptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA565));

   alpha += alpha >> 7;
   if (alpha > 0x100)
      alpha = 0x100;

   dptr = (void *)((char *)dest->image_data + dest->pitch * y_offset);
   sptr = (void *)((char *)src->image_data + src->pitch * src_y_offset);
   dptr += x_offset;
   sptr += src_x_offset;

   while (height >= 16)
   {
      rgba565_overlay_to_rgba32(dptr, dest->pitch, sptr, src->pitch,
                                width, 16, alpha);
      dptr = (void *)((char *)dptr + (dest->pitch << 4));
      sptr = (void *)((char *)sptr + (src->pitch << 4));
      height -= 16;
   }

   if (height > 0)
   {
      rgba565_overlay_to_rgba32(dptr, dest->pitch, sptr, src->pitch,
                                width, height, alpha);
   }
}

/******************************************************************************
NAME
   vc_image_overlay_rgb565_to_rgba565

SYNOPSIS
   void vc_image_overlay_rgb565_to_rgba565(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T    const *src,
      int                  src_x_offset,
      int                  src_y_offset,
      int                  transparent_colour);

FUNCTION
   combine data and output rgba565.
******************************************************************************/

void vc_image_overlay_rgb565_to_rgba565(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T    const *src,
   int                  src_x_offset,
   int                  src_y_offset,
   int                  transparent_colour)
{
   unsigned short      *dptr;
   unsigned short const *sptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGB565));

   dptr = (void *)((char *)dest->image_data + dest->pitch * y_offset);
   sptr = (void *)((char *)src->image_data + src->pitch * src_y_offset);
   dptr += x_offset;
   sptr += src_x_offset;

   while (height >= 16)
   {
      rgb565_overlay_to_rgba565(dptr, dest->pitch, sptr, src->pitch,
                                width, 16, transparent_colour);
      dptr = (void *)((char *)dptr + (dest->pitch << 4));
      sptr = (void *)((char *)sptr + (src->pitch << 4));
      height -= 16;
   }

   if (height > 0)
   {
      rgb565_overlay_to_rgba565(dptr, dest->pitch, sptr, src->pitch,
                                width, height, transparent_colour);
   }
}


/******************************************************************************
NAME
   vc_image_overlay_rgba565_to_rgb565

SYNOPSIS
   void vc_image_overlay_rgba565_to_rgb565(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T    const *src,
      int                  src_x_offset,
      int                  src_y_offset,
      int                  alpha);

FUNCTION
   combine data and output rgb565.
******************************************************************************/

void vc_image_overlay_rgba565_to_rgb565(
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
   unsigned short      *dptr;
   unsigned short const *sptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA565));

   dptr = (void *)((char *)dest->image_data + dest->pitch * y_offset);
   sptr = (void *)((char *)src->image_data + src->pitch * src_y_offset);
   dptr += x_offset;
   sptr += src_x_offset;

   while (height >= 16)
   {
      rgba565_overlay_to_rgb565(dptr, dest->pitch, sptr, src->pitch,
                                width, 16);
      dptr = (void *)((char *)dptr + (dest->pitch << 4));
      sptr = (void *)((char *)sptr + (src->pitch << 4));
      height -= 16;
   }

   if (height > 0)
   {
      rgba565_overlay_to_rgb565(dptr, dest->pitch, sptr, src->pitch,
                                width, height);
   }
}


/******************************************************************************
NAME
   vc_image_overlay_rgba565_to_rgb888

SYNOPSIS
   void vc_image_overlay_rgba565_to_rgb888(
      VC_IMAGE_T          *dest,
      int                  x_offset,
      int                  y_offset,
      int                  width,
      int                  height,
      VC_IMAGE_T    const *src,
      int                  src_x_offset,
      int                  src_y_offset,
      int                  alpha);

FUNCTION
   combine data and output rgb888.
******************************************************************************/

void vc_image_overlay_rgba565_to_rgb888(
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
   unsigned short const *sptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB888));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA565));

   dptr = (void *)((char *)dest->image_data + dest->pitch * y_offset);
   sptr = (void *)((char *)src->image_data + src->pitch * src_y_offset);
   dptr += x_offset * 3;
   sptr += src_x_offset;

   while (height >= 16)
   {
      rgba565_overlay_to_rgb888(dptr, dest->pitch, sptr, src->pitch,
                                width, 16);
      dptr = (void *)((char *)dptr + (dest->pitch << 4));
      sptr = (void *)((char *)sptr + (src->pitch << 4));
      height -= 16;
   }

   if (height > 0)
   {
      rgba565_overlay_to_rgb888(dptr, dest->pitch, sptr, src->pitch,
                                width, height);
   }
}


/******************************************************************************
NAME
   vc_image_overlay_rgba565_to_rgba565

SYNOPSIS
   void vc_image_overlay_rgba565_to_rgba565(
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
   combine data and output rgba565.  src appears over the top of dest.  odd
   alignments will be allowed.
******************************************************************************/

void vc_image_overlay_rgba565_to_rgba565(
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
   unsigned short      *dptr;
   unsigned short const *sptr;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA565));

   dptr = (void *)((char *)dest->image_data + dest->pitch * y_offset);
   sptr = (void *)((char *)src->image_data + src->pitch * src_y_offset);
   dptr += x_offset;
   sptr += src_x_offset;

   while (height >= 16)
   {
      rgba565_overlay_to_rgba565(dptr, dest->pitch, sptr, src->pitch,
                                 width, 16);
      dptr = (void *)((char *)dptr + (dest->pitch << 4));
      sptr = (void *)((char *)sptr + (src->pitch << 4));
      height -= 16;
   }

   if (height > 0)
   {
      rgba565_overlay_to_rgba565(dptr, dest->pitch, sptr, src->pitch,
                                 width, height);
   }
}


/******************************************************************************
NAME
   vc_image_quick_overlay_rgba565_to_rgb565

SYNOPSIS
   void vc_image_quick_overlay_rgba565_to_rgb565(
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
      int                  alpha);

FUNCTION
   combine data and output rgb565, using only the 16x16 blocks indicated in
   block_mask
******************************************************************************/

void vc_image_quick_overlay_rgba565_to_rgb565(
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
   vcos_assert(!"Not implemented");
}


/******************************************************************************
NAME
   vc_image_quick_overlay_rgba565_to_rgb888

SYNOPSIS
   void vc_image_quick_overlay_rgba565_to_rgb888(
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
      int                  alpha);

FUNCTION
   combine data and output rgb888, using only the 16x16 blocks indicated in
   block_mask
******************************************************************************/

void vc_image_quick_overlay_rgba565_to_rgb888(
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
   vcos_assert(!"Not implemented");
}

/* End of blending functions *********************************************}}}*/

/* Resizing functions ****************************************************{{{*/

/******************************************************************************
NAME
   vc_image_resize_stripe_h_rgba565

SYNOPSIS
   void vc_image_resize_stripe_h_rgba565(
      VC_IMAGE_T          *dest,
      int                  d_width,
      int                  s_width);

FUNCTION
   stuff
******************************************************************************/

void vc_image_resize_stripe_h_rgba565(VC_IMAGE_T *dest, int d_width, int s_width)
{
   int step;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA565));
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
      vc_stripe_h_reduce_rgba5652(dest->image_data, dest->pitch, dest->image_data, dest->pitch, s_width, 0x40000);
   }

   step = ((s_width << 16) + (d_width >> 1)) / d_width;

   if (step != 0x10000)
   {
      switch (_msb((step - 1) | 0x1000))
      {
      case 17: /* 2.0 < step <= 4.0 */
         vc_stripe_h_reduce_rgba5652(dest->image_data, dest->pitch, dest->image_data, dest->pitch, d_width, step);
         break;
      case 16: /* 1.0 < step <= 2.0 */
         vc_stripe_h_reduce_rgba565(dest->image_data, dest->pitch, dest->image_data, dest->pitch, d_width, step);
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

            vc_stripe_h_expand_rgba565(dest->image_data, dest->pitch, (void *)((char *)dest->image_data + off), dest->pitch, d_width, step, s_width);
         }
#else
         vc_stripe_h_expand_rgba565((unsigned short *)dest->image_data + d_width, dest->pitch, (unsigned short *)dest->image_data + s_width, dest->pitch, d_width, step);
#endif
         break;

      default:
         vcos_assert(!"can't cope with scale factor");
      }
   }
}


/******************************************************************************
NAME
   vc_image_resize_stripe_v_rgba565

SYNOPSIS
   void vc_image_resize_stripe_v_rgba565(
      VC_IMAGE_T          *dest,
      int                  d_height,
      int                  s_height);

FUNCTION
   stuff
******************************************************************************/

void vc_image_resize_stripe_v_rgba565(VC_IMAGE_T *dest, int d_height, int s_height)
{
   int step;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA565));
   vcos_assert(d_height > 0 && s_height > 0);
   vcos_assert(dest->width == 16);
   vcos_assert(dest->height >= s_height);
   vcos_assert(dest->height >= d_height);

   if (d_height <= 0)
      return;

   while (d_height << 2 < s_height)
   {
      s_height >>= 2;
      vc_stripe_v_reduce_rgba5652(dest->image_data, dest->pitch, dest->image_data, dest->pitch, s_height, 0x40000);
   }

   step = ((s_height << 16) + (d_height >> 1)) / d_height;

   if (step != 0x10000)
   {
      switch (_msb((step - 1) | 0x1000))
      {
      case 17: /* 2.0 < step <= 4.0 */
         vc_stripe_v_reduce_rgba5652(dest->image_data, dest->pitch, dest->image_data, dest->pitch, d_height, step);
         break;
      case 16: /* 1.0 < step <= 2.0 */
         vc_stripe_v_reduce_rgba565(dest->image_data, dest->pitch, dest->image_data, dest->pitch, d_height, step);
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

            vc_stripe_v_expand_rgba565(dest->image_data, dest->pitch, (void *)((char *)dest->image_data + off), dest->pitch, d_height, step, s_height);
         }
#else
         vc_stripe_v_expand_rgba565((void *)((char *)dest->image_data + dest->pitch * d_height), dest->pitch, (void *)((char *)dest->image_data + dest->pitch * s_height), dest->pitch, d_height, step);
#endif
         break;
      default:
         vcos_assert(!"can't cope with scale factor");
      }
   }
}


/* End of resizing functions *********************************************}}}*/

/******************************************************************************
Static function definitions.
******************************************************************************/

