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
#include <stdlib.h>
#include <string.h>

#include <vcinclude/common.h>
#include <vcinclude/vcore.h>
#include <vcinclude/hardware.h>
#include <vclib/vclib.h>

/* Project includes */
#include "vc_image_tformat.h"
#include "yuv_to_rgb.h"
#include "interface/vcos/vcos_assert.h"


unsigned int vc_image_temp_palette[256];





/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

//TODO put them here

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

extern void tformat_subtile_load_32bit(unsigned long const *src32);
extern void tformat_subtile_store_32bit(unsigned long *dst32);
extern void tformat_utiles_store_32bit(int count, unsigned long *dst32);

extern void tformat_linear_load_32bit(unsigned long const *src32, int pitch);
extern unsigned long const * tformat_linear_load_32bit_flip(unsigned long const *src32, int pitch);
extern unsigned long const * tformat_linear_load_rgb565_to_32bit_flip(unsigned long const *src16, int pitch);
extern unsigned long const * tformat_linear_load_32bit_flip_rbswap(unsigned long const *src32, int pitch);
extern unsigned long const * tformat_linear_load_32bit_flip_etc1_workarounds(unsigned long const *src32, int pitch);
extern unsigned long const * tformat_linear_load_32bit_byteswap(unsigned long const *src32, int pitch);
extern unsigned long const * tformat_linear_load_24bit_flip_rgb24_to_rgb565(unsigned long const *src32, int pitch);
extern unsigned long const * tformat_linear_load_32bit_flip_rgba32_to_rgba5551(unsigned long const *src32, int pitch);
extern unsigned long const * tformat_linear_load_24bit_flip(unsigned long const *src24, int pitch);
extern unsigned long const * tformat_linear_load_pal32bit(unsigned long const *src24, int pitch);
extern unsigned long const * tformat_linear_load_pal16bit(unsigned long const *src24, int pitch);
extern unsigned long const * tformat_linear_load_8bit_flip(unsigned long const *src32, int pitch);
extern unsigned long const * tformat_linear_load_8bit_nibbleswap(unsigned long const *src32, int pitch);
extern void tformat_linear_store_32bit(unsigned long *dst32, int pitch);
extern void tformat_linear_store_32bit_flip(unsigned long *dst32, int pitch);
extern void tformat_linear_store_32bit_opaque(unsigned long *dst32, int pitch);
extern void tformat_linear_store_24bit(unsigned char *dst24, int pitch);
extern void tformat_linear_store_16bit(unsigned long *dst16, int pitch);
extern void tformat_linear_store_16bit_flip(unsigned long *dst16, int pitch);
extern unsigned long * tformat_raster_store4(int ycount, unsigned long *dst32, int pitch, int trimleft, int trimright, int trimbottom);
extern unsigned long * tformat_raster_store4_opaque(int ycount, unsigned long *dst32, int pitch, int trimleft, int trimright, int trimbottom);
extern unsigned long * tformat_raster_store_rgb888(int ycount, unsigned long *dst32, int pitch, int trimleft, int trimright, int trimbottom);
extern unsigned long * raster_store_ya(int ycount, unsigned long *dst32, int pitch, int trimleft, int trimright, int trimbottom);
extern unsigned long * raster_store_y(int ycount, unsigned long *dst32, int pitch, int trimleft, int trimright, int trimbottom);
extern unsigned long * raster_store_a(int ycount, unsigned long *dst32, int pitch, int trimleft, int trimright, int trimbottom);
extern unsigned long * tformat_raster_store4_2(int ycount, unsigned long *dst32, int pitch, int trimleft, int trimright, int trimbottom);
extern unsigned long * tformat_raster_store_rgb888_2(int ycount, unsigned long *dst32, int pitch, int trimleft, int trimright, int trimbottom);
extern unsigned long * raster_store_ya2(int ycount, unsigned long *dst32, int pitch, int trimleft, int trimright, int trimbottom);
extern unsigned long * raster_store_y2(int ycount, unsigned long *dst32, int pitch, int trimleft, int trimright, int trimbottom);
extern unsigned long * raster_store_a2(int ycount, unsigned long *dst32, int pitch, int trimleft, int trimright, int trimbottom);
extern void tformat_gather_linear_utiles_32bit(void* ptr,unsigned int stride,unsigned int pixelsleft,unsigned int width);
extern void tformat_gather_linear_utiles_24bit(const void* ptr,unsigned int stride,unsigned int wordsleft,unsigned int wordsperrow);

extern void tformat_linear_store_rgba32_to_rgb565(unsigned short *dst565, int pitch);

extern void tformat_red_blue_swap(void);

extern void tformat_subtiles_store_8bit(unsigned long *dst8_left, unsigned long *dst8_right);
extern void tformat_linear_tile_store_8bit(unsigned long *dst8, int width, int height, int stride);
extern void tformat_linear_tile_store_32bit(unsigned long *dst32, int width, int height, int stride);
extern void tformat_linear_tile_load_32bit(unsigned long const *src32, int stride);
extern void load_tformat_opaque(unsigned long const *src32, int stride);
extern void load_tformat_4444_to_8888(unsigned long const *src32, int stride);
extern void load_tformat_5551_to_8888(unsigned long const *src32, int stride);
extern void load_tformat_565_to_8888(unsigned long const *src32, int stride);

extern void tformat_subtile_transpose_32bit(void *dst32, const void *src32);
extern void tformat_subtile_pair_transpose_16bit(void *dst32a, void *dst32b, const void *src32a, const void *src32b, int enable);

extern void tformat_blit_row(
      void *dest, int   destdir0, int   destdir1,
      const void *srcbl, const void *srcbr, int   srctdir,
      const void *srctl, const void *srctr, int   srcbdir,
      int   offsetx, int   offsety, int   count);

#ifdef __BCM2707A0__
extern void tformat_hw686_swap(void);
#endif
#ifdef WORKAROUND_FTRMK_9
extern void tformat_etc1_vflip(void);  //XXX FTRMK-9
#endif

extern void vc_image_mipmap_blt(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                   VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int miplvl, int cubeface);

extern void *vc_image_get_mipmap(VC_IMAGE_T *image, int miplvl, int cubeface);

extern void tformat_resize_stripe(
      void                *dptr0,      /* r0                 */
      int                  dpitch,     /* r1                 */
      int                  x,          /* r2                 */
      float                xinc,       /* r3                 */
      int                  y,          /* r4                 */
      float                yinc,       /* r5                 */
      int                  tu2offset,  /* (sp)    -> (sp+44) */
      int                  width,      /* (sp+4)  -> (sp+48) */
      void                *dptr1);     /* (sp+8)  -> (sp+52) */

extern void tformat_resize_utile_stripe(
      void                *dptr0,      /* r0                 */
      int                  dpitch,     /* r1                 */
      int                  x,          /* r2                 */
      float                xinc,       /* r3                 */
      int                  y,          /* r4                 */
      float                yinc,       /* r5                 */
      int                  tu2offset,  /* (sp)    -> (sp+44) */
      int                  width,      /* (sp+4)  -> (sp+48) */
      void                *dptr1);     /* (sp+8)  -> (sp+52) */

extern unsigned long * tformat_wide_raster_to_tformat(
      int                  width,      /* r0 */
      unsigned long       *dst32,      /* r1 */
      unsigned long       *src32,      /* r2 */
      int                  dpitch,     /* r3 */
      int                  spitch,     /* r4 */
      int                  swap_rb);   /* r5 */

/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/

typedef unsigned long const * (*LOAD_PROC4_T)(unsigned long const *src32, int pitch);
typedef unsigned long const * (*LOAD_PROC8_T)(unsigned long const *src32, int pitch);
typedef unsigned long * (*STORE_PROC4_T)(int ycount, unsigned long *dst32, int pitch, int trimleft, int trimright, int trimbottom);

typedef void (*LOAD_TFORMAT_PROC_T)(unsigned long const *src32, int stride);

/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/

static void linear_tile_transfer_24bit(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform);
static void *get_mipmap(VC_IMAGE_T *image, int miplvl, int cubeface);
static void my_blt(VC_IMAGE_T *dest,
                   int x_offset, int y_offset, int width,int height,
                   VC_IMAGE_T const *src, int src_x_offset, int src_y_offset,
                   VC_IMAGE_TRANSFORM_T transform);
static void vc_image_configure_mipmap(VC_IMAGE_T *img, VC_IMAGE_T *src, int miplvl, int cubeface);
static void expand_palette(void *destpal, void const *srcpal, int gltype, int size);
static void* malloc_palette(void const *srcpal, int gltype, int size);



// Functions which do the work
static void vc_image_raster_to_tf8(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_PROC8_T load_raster);
static void vc_image_raster_to_tf4(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_PROC4_T load_raster);
static void vc_image_raster_to_ltile8(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_PROC8_T load_raster);
static void vc_image_raster_to_ltile4(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_PROC4_T load_raster);
static void vc_image_tf_to_raster4(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_TFORMAT_PROC_T load_tformat, STORE_PROC4_T store_raster, int xoff, int yoff, int xmax, int ymax);
static void vc_image_ltile_to_raster4(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_TFORMAT_PROC_T load_tformat, STORE_PROC4_T load_raster, int xoff, int yoff, int xmax, int ymax);
static void vc_image_tf_to_tf(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset);

static int tformat_adjusted_width4(VC_IMAGE_T const *img);
static int tformat_adjusted_height4(VC_IMAGE_T const *img);
static int tformat_adjusted_pitch4(VC_IMAGE_T const *img);
static int tformat_adjusted_width8(VC_IMAGE_T const *img);

/******************************************************************************
Global function definitions.
******************************************************************************/

/* Conversions ***********************************************************{{{*/

/******************************************************************************
NAME
   vc_image_tf_rgba32_to_rgba32

SYNOPSIS
   void vc_image_tf_rgba32_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform);

FUNCTION
   Converts a t-format RGBA32 image to raster RGBA32. Also performs a vertical
   flip if requested.
******************************************************************************/

void vc_image_tf_rgba32_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform)
{
   char *destbase;
   int x, y, destpitch;

   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_TF_RGBA32));

   if (!vcos_verify(vc_image_get_mipmap_type(src,0) == VC_IMAGE_MIPMAP_TFORMAT))
      return;

   if (transform)
   {
      destbase = (char *)dest->image_data;
      destpitch = dest->pitch;
   }
   else
   {
      vcos_assert_msg((dest->height & 15) == 0, "risk of illegal access at beginning of image");

      destbase = (char *)dest->image_data + (dest->height - 1) * dest->pitch;
      destpitch = -dest->pitch;
   }

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned long const *srcptr = (void *)((char *)src->image_data + ((y + 32) & ~63) * src->pitch);
      unsigned long *destptr = (void *)(destbase + y * destpitch);
      int rowflip = (y & 32) ? -512 : 0;

      for (x = 0; x < dest->width; x += 32)
      {
         int ofs = (x >> 4) * 512 + ((y >> 4) & 1) * 256;

         tformat_subtile_load_32bit(srcptr + (ofs ^ rowflip));
#if !defined(RGB888)
         tformat_red_blue_swap();
#endif
         tformat_linear_store_32bit_flip(destptr, destpitch);
         destptr += 16;

         if (x + 16 < dest->width)
         {
            ofs += 512;
            tformat_subtile_load_32bit(srcptr + (ofs ^ rowflip ^ 256));
#if !defined(RGB888)
            tformat_red_blue_swap();
#endif
            tformat_linear_store_32bit_flip(destptr, destpitch);
         }
         destptr += 16;
      }
   }
   if ((dest->height & 15) != 0)
      vclib_memmove(dest->image_data, (char *)dest->image_data + dest->pitch * (16 - (dest->height & 15)), dest->pitch * dest->height);
}


/******************************************************************************
NAME
   vc_image_tf_rgbx32_to_rgba32

SYNOPSIS
   void vc_image_tf_rgbx32_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_tf_rgbx32_to_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src)
{
   void *destbase;
   int x, y;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_TF_RGBA32) || is_valid_vc_image_buf(src, VC_IMAGE_TF_RGBX32));

   destbase = (char *)dest->image_data + (dest->height - 1 & ~15) * dest->pitch;

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned long *destptr = (void *)((char *)destbase - y * dest->pitch);
      unsigned long const *srcptr = (void *)((char *)src->image_data + ((y + 32) & ~63) * src->pitch);
      int rowflip = (y & 32) ? -512 : 0;

      for (x = 0; x < dest->width; x += 32)
      {
         int ofs = (x >> 4) * 512 + ((y >> 4) & 1) * 256;

         tformat_subtile_load_32bit(srcptr + (ofs ^ rowflip));
#if !defined(RGB888)
         tformat_red_blue_swap();
#endif
         tformat_linear_store_32bit_opaque(destptr, dest->pitch);
         destptr += 16;

         if (x + 16 < dest->width)
         {
            ofs += 512;
            tformat_subtile_load_32bit(srcptr + (ofs ^ rowflip ^ 256));
#if !defined(RGB888)
            tformat_red_blue_swap();
#endif
            tformat_linear_store_32bit_opaque(destptr, dest->pitch);
         }
         destptr += 16;
      }
   }
   if (!vcos_verify((dest->height & 15) == 0))
   {
      /* TODO: untested code path */
      vclib_memmove(dest->image_data, (char *)dest->image_data + dest->pitch * (16 - (dest->height & 15)), dest->pitch * dest->height);
   }
}


/******************************************************************************
NAME
   vc_image_tf_rgb565_to_rgb565

SYNOPSIS
   void vc_image_tf_rgb565_to_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_tf_rgb565_to_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src)
{
   void *destbase;
   int x, y;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_TF_RGB565));

   /* Note that although the source here has been changed slightly to look like
    * it's working with 16-bit data, the code generated should be almost
    * identical to that in vc_image_tf_rgba32_to_rgba32().
    */

   destbase = (char *)dest->image_data + (dest->height - 1 & ~15) * dest->pitch;

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned short *destptr = (void *)((char *)destbase - y * dest->pitch);
      unsigned short const *srcptr = (void *)((char *)src->image_data + ((y + 32) & ~63) * src->pitch);
      int rowflip = (y & 32) ? -512 : 0;

      for (x = 0; x < dest->width; x += 64)
      {
         int ofs = (x >> 5) * 512 + ((y >> 4) & 1) * 256;

         tformat_subtile_load_32bit((unsigned long *)(srcptr + (ofs ^ rowflip) * 2));
         if (x + 16 < dest->width)
            tformat_linear_store_32bit((unsigned long *)destptr, dest->pitch);
         else
            tformat_linear_store_16bit((unsigned long *)destptr, dest->pitch);
         destptr += 32;

         if (x + 32 < dest->width)
         {
            ofs += 512;
            tformat_subtile_load_32bit((unsigned long *)(srcptr + (ofs ^ rowflip ^ 256) * 2));
            if (x + 48 < dest->width)
               tformat_linear_store_32bit((unsigned long *)destptr, dest->pitch);
            else
               tformat_linear_store_16bit((unsigned long *)destptr, dest->pitch);
         }
         destptr += 32;
      }
   }
   if (!vcos_verify((dest->height & 15) == 0))
   {
      /* TODO: untested code path */
      vclib_memmove(dest->image_data, (char *)dest->image_data + dest->pitch * (16 - (dest->height & 15)), dest->pitch * dest->height);
   }
}

/******************************************************************************
NAME
   vc_image_tf_rgba32_to_rgb888

SYNOPSIS
   void vc_image_tf_rgba32_to_rgb888(VC_IMAGE_T *dest, VC_IMAGE_T const *src)

FUNCTION
   x
******************************************************************************/

void vc_image_tf_rgba32_to_rgb888(VC_IMAGE_T *dest, VC_IMAGE_T const *src)
{
   void *destbase;
   int x, y;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB888));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_TF_RGBA32) || is_valid_vc_image_buf(src, VC_IMAGE_TF_RGBX32));

   if (!vcos_verify(vc_image_get_mipmap_type((VC_IMAGE_T *)src, 0) == VC_IMAGE_MIPMAP_TFORMAT))
      return;

   vcos_assert_msg((src->height & 15) == 0, "risk of illegal access at beginning of image");

   destbase = (char *)dest->image_data + (dest->height - 1 & ~15) * dest->pitch;

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned char *destptr = (void *)((char *)destbase - y * dest->pitch);
      unsigned long const *srcptr = (void *)((char *)src->image_data + ((y + 32) & ~63) * src->pitch);
      int rowflip = (y & 32) ? -512 : 0;

      for (x = 0; x < dest->width; x += 32)
      {
         int ofs = (x >> 4) * 512 + ((y >> 4) & 1) * 256;

         tformat_subtile_load_32bit(srcptr + (ofs ^ rowflip));
         tformat_linear_store_24bit(destptr, dest->pitch);
         destptr += 48;

         if (x + 16 < dest->width)
         {
            ofs += 512;
            tformat_subtile_load_32bit(srcptr + (ofs ^ rowflip ^ 256));
            tformat_linear_store_24bit(destptr, dest->pitch);
         }
         destptr += 48;
      }
   }
   if (!vcos_verify((dest->height & 15) == 0))
   {
      /* TODO: untested code path */
      vclib_memmove(dest->image_data, (char *)dest->image_data + dest->pitch * (16 - (dest->height & 15)), dest->pitch * dest->height);
   }
}
/******************************************************************************
NAME
   vc_image_rgba32_to_tf_rgba32

SYNOPSIS
   void vc_image_rgba32_to_tf_rgba32(VC_IMAGE_T *dest, int y_off, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, int swap_flags);

FUNCTION
   The function nominally takes RGBA32 input (R in LSB, A in MSB), but can perform swaps to accept other input orderings.
   swap_flags bit 1 rotates the source pixels to move alpha from LSB to MSB
   swap_flags bit 0 swaps red and blue (after any alpha rotation)
******************************************************************************/

void vc_image_rgba32_to_tf_rgba32(VC_IMAGE_T *dest, int y_off, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, int swap_flags)
{
   unsigned long *sptr;
   int spitch;
   int x, y;

   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBA32) || is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBX32));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32));

   if (transform == TRANSFORM_VFLIP)
   {
      sptr = src->image_data;
      spitch = src->pitch;
   }
   else
   {
      vcos_assert((src->height & 15) == 0);
      sptr = (void *)((char *)src->image_data + ((src->height - 1) | 15) * src->pitch);
      spitch = -src->pitch;
   }

   for (y = y_off; y < dest->height; y += 4)
   {
      unsigned long *dptr = (void *)((char *)dest->image_data
                                       + ((y + 32) & ~63) * dest->pitch
                                       + ((y & 32) ? -2048 : 0)
                                       + ((y & 16) ? 1024 : 0)
                                       + (y & 12) * 64);
      int rowpitch = 1024 + ((y & 32) ? -4096 : 0) + ((y & 16) ? 0 : 2048);
      int xstep = (y & 32) ? -4096 : 4096;

      for (x = 0; x < dest->width; x += 128)
      {
         int w = min(dest->width - x, 128);

         tformat_wide_raster_to_tformat(w, dptr, sptr + x, rowpitch, spitch, swap_flags);
         dptr += xstep;
      }
      sptr = (void *)((char *)sptr + 4 * spitch);
   }
}


/******************************************************************************
NAME
   vc_image_rgba32_to_tf_rgba16   FIXME!!!!

SYNOPSIS
   void vc_image_rgba32_to_tf_rgba5551(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform);

FUNCTION
   x

   f00f - red
   0f0f - green
   00ff - blue

   so layout must actually be

   RGBA

******************************************************************************/

uint16_t eben_buffer[512];

inline uint16_t rgba32_to_rgba16(uint32_t pixel)
{
   return (pixel & 0xf0000000) >> 28 | (pixel & 0xf00000) >> 16 | (pixel & 0xf000) >> 4 | (pixel & 0xf0) << 8;
}

void vc_image_rgba32_to_tf_rgba16(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform)
{
   char const *srcbase;
   int x, y, srcpitch;

   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBA16));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32));

   if (vc_image_get_mipmap_type(dest,0) == VC_IMAGE_MIPMAP_LINEAR_TILE)
   {
      //printf("Linear tile not currently supported for rgba16\n");
      return;
   }

   if (transform)
   {
      srcbase = (char *)src->image_data;
      srcpitch = src->pitch;
   }
   else
   {
      vcos_assert_msg((src->height & 15) == 0, "risk of illegal access at beginning of image");

      srcbase = (char *)src->image_data + (src->height - 1) * src->pitch;
      srcpitch = -src->pitch;
   }

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned short *destptr = (void *)((char *)dest->image_data + ((y + 32) & ~63) * dest->pitch);
      unsigned short const *srcptr = (void *)(srcbase + y * srcpitch);
      int rowflip = (y & 32) ? -512 : 0;

      for (x = 0; x < dest->width; x += 64)
      {
         int ofs = (x >> 5) * 512 + ((y >> 4) & 1) * 256;

         for (int i = 0; i < 32; i++)
            for (int j = 0; j < 16; j++) {
               uint32_t pixel = *(uint32_t *)(srcbase + (x + i) * 4 + (y + j) * srcpitch);

               eben_buffer[i + j * 32] = rgba32_to_rgba16(pixel);
            }

//         tformat_linear_load_32bit_flip((unsigned long *)srcptr, srcpitch);
         tformat_linear_load_32bit_flip((unsigned long *)eben_buffer, 64);
         tformat_subtile_store_32bit((unsigned long *)(destptr + (ofs ^ rowflip) * 2));
         srcptr += 32;

         if (x + 32 < dest->width)
         {
            ofs += 512;

            for (int i = 0; i < 32; i++)
               for (int j = 0; j < 16; j++) {
                  uint32_t pixel = *(uint32_t *)(srcbase + (x + i + 32) * 4 + (y + j) * srcpitch);

                  eben_buffer[i + j * 32] = rgba32_to_rgba16(pixel);
               }

//            tformat_linear_load_32bit_flip((unsigned long *)srcptr, srcpitch);
            tformat_linear_load_32bit_flip((unsigned long *)eben_buffer, 64);
            tformat_subtile_store_32bit((unsigned long *)(destptr + (ofs ^ rowflip ^ 256) * 2));
         }
         srcptr += 32;
      }
   }
}

inline uint16_t rgba32_to_rgba5551(uint32_t pixel)
{
   return ((pixel & 0xff000000) ? 0x0001 : 0x0000) | (pixel & 0xf80000) >> 18 | (pixel & 0xf800) >> 5 | (pixel & 0xf8) << 8;
}

void vc_image_rgba32_to_tf_rgba5551(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform)
{
   char const *srcbase;
   int x, y, srcpitch;

   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBA5551));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGBA32)
            || is_valid_vc_image_buf(src, VC_IMAGE_OPENGL) && src->extra.opengl.format_and_type == VC_IMAGE_OPENGL_RGBA32);

   if (!vcos_verify(vc_image_get_mipmap_type(dest,0) == VC_IMAGE_MIPMAP_TFORMAT))
   {
      vclib_memset(dest->image_data, 0, dest->size);
      return;
   }

   if (transform)
   {
      srcbase = (char *)src->image_data;
      srcpitch = src->pitch;
   }
   else
   {
      vcos_assert_msg((src->height & 15) == 0, "risk of illegal access at beginning of image");

      srcbase = (char *)src->image_data + (src->height - 1) * src->pitch;
      srcpitch = -src->pitch;
   }

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned short *destptr = (void *)((char *)dest->image_data + ((y + 32) & ~63) * dest->pitch);
      unsigned short const *srcptr = (void *)(srcbase + y * srcpitch);
      int rowflip = (y & 32) ? -512 : 0;

      for (x = 0; x < dest->width; x += 64)
      {
         int ofs = (x >> 5) * 512 + ((y >> 4) & 1) * 256;

         for (int i = 0; i < 32; i++)
            for (int j = 0; j < 16; j++) {
               uint32_t pixel = *(uint32_t *)(srcbase + (x + i) * 4 + (y + j) * srcpitch);

               eben_buffer[i + j * 32] = rgba32_to_rgba5551(pixel);
            }

//         tformat_linear_load_32bit_flip((unsigned long *)srcptr, srcpitch);
         tformat_linear_load_32bit_flip((unsigned long *)eben_buffer, 64);
         tformat_subtile_store_32bit((unsigned long *)(destptr + (ofs ^ rowflip) * 2));
         srcptr += 32;

         if (x + 32 < dest->width)
         {
            ofs += 512;

            for (int i = 0; i < 32; i++)
               for (int j = 0; j < 16; j++) {
                  uint32_t pixel = *(uint32_t *)(srcbase + (x + i + 32) * 4 + (y + j) * srcpitch);

                  eben_buffer[i + j * 32] = rgba32_to_rgba5551(pixel);
               }

//            tformat_linear_load_32bit_flip((unsigned long *)srcptr, srcpitch);
            tformat_linear_load_32bit_flip((unsigned long *)eben_buffer, 64);
            tformat_subtile_store_32bit((unsigned long *)(destptr + (ofs ^ rowflip ^ 256) * 2));
         }
         srcptr += 32;
      }
   }
}


/******************************************************************************
NAME
   vc_image_rgb888_to_tf_rgb565   FIXME!!!!

SYNOPSIS
   void vc_image_rgb888_to_tf_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform);

FUNCTION
   x
******************************************************************************/

void vc_image_rgb888_to_tf_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform)
{
   char const *srcbase;
   int x, y, srcpitch;

   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGB565));

   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGB888)
            || is_valid_vc_image_buf(src, VC_IMAGE_OPENGL) && src->extra.opengl.format_and_type == VC_IMAGE_OPENGL_RGB24);

   if (!vcos_verify(vc_image_get_mipmap_type(dest,0) == VC_IMAGE_MIPMAP_TFORMAT))
   {
      vclib_memset(dest->image_data, 0, dest->size);
      return;
   }

   if (transform)
   {
      srcbase = (char *)src->image_data;
      srcpitch = src->pitch;
   }
   else
   {
      vcos_assert_msg((src->height & 15) == 0, "risk of illegal access at beginning of image");

      srcbase = (char *)src->image_data + (src->height - 1) * src->pitch;
      srcpitch = -src->pitch;
   }

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned short *destptr = (void *)((char *)dest->image_data + ((y + 32) & ~63) * dest->pitch);
      unsigned short const *srcptr = (void *)(srcbase + y * srcpitch);
      int rowflip = (y & 32) ? -512 : 0;

      for (x = 0; x < dest->width; x += 64)
      {
         int ofs = (x >> 5) * 512 + ((y >> 4) & 1) * 256;

         for (int i = 0; i < 32; i++)
            for (int j = 0; j < 16; j++)
               eben_buffer[i + j * 32] = 0xf81f;

//         tformat_linear_load_32bit_flip((unsigned long *)srcptr, srcpitch);
         tformat_linear_load_32bit_flip((unsigned long *)eben_buffer, 64);
         tformat_subtile_store_32bit((unsigned long *)(destptr + (ofs ^ rowflip) * 2));
         srcptr += 32;

         if (x + 32 < dest->width)
         {
            ofs += 512;

            for (int i = 0; i < 32; i++)
               for (int j = 0; j < 16; j++)
                  eben_buffer[i + j * 32] = 0xf81f;

//            tformat_linear_load_32bit_flip((unsigned long *)srcptr, srcpitch);
            tformat_linear_load_32bit_flip((unsigned long *)eben_buffer, 64);
            tformat_subtile_store_32bit((unsigned long *)(destptr + (ofs ^ rowflip ^ 256) * 2));
         }
         srcptr += 32;
      }
   }
}

#if 0
void vc_image_rgb888_to_tf_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform)
{
   char const *srcbase;
   int x, y, srcpitch;

   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGB888));

   if (!vcos_verify(vc_image_get_mipmap_type(dest,0) == VC_IMAGE_MIPMAP_TFORMAT))
      return;

   if (transform)
   {
      srcbase = (char *)src->image_data;
      srcpitch = src->pitch;
   }
   else
   {
      vcos_assert_msg((src->height & 15) == 0, "risk of illegal access at beginning of image");

      srcbase = (char *)src->image_data + (src->height - 1) * src->pitch;
      srcpitch = -src->pitch;
   }

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned short *destptr = (void *)((char *)dest->image_data + ((y + 32) & ~63) * dest->pitch);
      unsigned short const *srcptr = (void *)(srcbase + y * srcpitch);
      int rowflip = (y & 32) ? -512 : 0;

      for (x = 0; x < dest->width; x += 64)
      {
         int ofs = (x >> 5) * 512 + ((y >> 4) & 1) * 256;

         tformat_linear_load_32bit_flip((unsigned long *)srcptr, srcpitch);
         tformat_linear_store_rgba32_to_rgb565(eben_buffer, 64);

         tformat_linear_load_32bit_flip((unsigned long *)srcptr+16, srcpitch);
         tformat_linear_store_rgba32_to_rgb565(eben_buffer+16, 64);

         tformat_linear_load_32bit((unsigned long *)eben_buffer, 64);

         tformat_subtile_store_32bit((unsigned long *)(destptr + (ofs ^ rowflip) * 2));
         srcptr += 64;

         if (x + 32 < dest->width)
         {
            ofs += 512;
//            tformat_linear_load_32bit_flip((unsigned long *)srcptr, srcpitch);

            tformat_linear_load_32bit_flip((unsigned long *)srcptr, srcpitch);
            tformat_linear_store_rgba32_to_rgb565(eben_buffer, 64);

            tformat_linear_load_32bit_flip((unsigned long *)srcptr+16, srcpitch);
            tformat_linear_store_rgba32_to_rgb565(eben_buffer+16, 64);

            tformat_linear_load_32bit((unsigned long *)eben_buffer, 64);

            tformat_subtile_store_32bit((unsigned long *)(destptr + (ofs ^ rowflip ^ 256) * 2));
         }
         srcptr += 64;
      }
   }
}
#endif

/******************************************************************************
NAME
   vc_image_rgb888_to_tf_etc1

SYNOPSIS
   void vc_image_rgb888_to_tf_etc1(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform);

FUNCTION
   x
******************************************************************************/

extern void etc1_encode(VC_IMAGE_T *out, VC_IMAGE_T const *in);
void vc_image_rgb888_to_tf_etc1(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform)
{
   vcos_assert(transform == 0);
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_ETC1));
   vcos_assert(is_valid_vc_image_buf(src, src->type) && (src->type == VC_IMAGE_OPENGL
                        || src->type == VC_IMAGE_YUV420 || src->type == VC_IMAGE_YUV422 || src->type == VC_IMAGE_YUV422PLANAR || src->type == VC_IMAGE_YUV_UV || src->type == VC_IMAGE_YUV_UV32
                        || src->type == VC_IMAGE_RGBA16 || src->type == VC_IMAGE_RGB565 || src->type == VC_IMAGE_RGB888 || src->type == VC_IMAGE_RGBA32));

   etc1_encode(dest, src);
}



/******************************************************************************
NAME
   vc_image_rgb888_to_ogl_etc1

SYNOPSIS
   void vc_image_rgb888_to_ogl_etc1(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform);

FUNCTION
   x
******************************************************************************/

void vc_image_rgb888_to_ogl_etc1(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform)
{
   vcos_assert(transform == 0);
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_OPENGL) && dest->extra.opengl.format_and_type == VC_IMAGE_OPENGL_ETC1);
   vcos_assert(is_valid_vc_image_buf(src, src->type) && (src->type == VC_IMAGE_OPENGL
                        || src->type == VC_IMAGE_YUV420 || src->type == VC_IMAGE_YUV422 || src->type == VC_IMAGE_YUV422PLANAR || src->type == VC_IMAGE_YUV_UV || src->type == VC_IMAGE_YUV_UV32
                        || src->type == VC_IMAGE_RGBA16 || src->type == VC_IMAGE_RGB565 || src->type == VC_IMAGE_RGB888 || src->type == VC_IMAGE_RGBA32));

   etc1_encode(dest, src);
}



/******************************************************************************
NAME
   vc_image_rgb565_to_tf_rgb565

SYNOPSIS
   void vc_image_rgb565_to_tf_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_rgb565_to_tf_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform)
{
   unsigned long *sptr;
   int spitch;
   int x, y;

   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGB565));

   if (transform == TRANSFORM_VFLIP)
   {
      sptr = src->image_data;
      spitch = src->pitch;
   }
   else
   {
      vcos_assert((src->height & 15) == 0);
      sptr = (void *)((char *)src->image_data + ((src->height - 1) | 15) * src->pitch);
      spitch = -src->pitch;
   }

   for (y = 0; y < dest->height; y += 4)
   {
      unsigned long *dptr = (void *)((char *)dest->image_data
                                       + ((y + 32) & ~63) * dest->pitch
                                       + ((y & 32) ? -2048 : 0)
                                       + ((y & 16) ? 1024 : 0)
                                       + (y & 12) * 64);
      int rowpitch = 1024 + ((y & 32) ? -4096 : 0) + ((y & 16) ? 0 : 2048);
      int xstep = (y & 32) ? -4096 : 4096;

      for (x = 0; x < dest->width; x += 256)
      {
         int w = min(dest->width - x, 256);

         tformat_wide_raster_to_tformat(w >> 1, dptr, (void *)((unsigned short *)sptr + x), rowpitch, spitch, 0);
         dptr += xstep;
      }
      sptr = (void *)((char *)sptr + 4 * spitch);
   }
}


/******************************************************************************
NAME
   vc_image_rgb565_to_tf_rgba32

SYNOPSIS
   void vc_image_rgb565_to_tf_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_rgb565_to_tf_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform)
{
   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBA32) || is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBX32));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGB565));

   vc_image_raster_to_tf4(dest, src, transform, tformat_linear_load_rgb565_to_32bit_flip);
}

/******************************************************************************
NAME
   vc_image_tf_rgba32_to_rgb565

SYNOPSIS
   void vc_image_tf_rgba32_to_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_tf_rgba32_to_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src)
{
   int x, y;
   void *destbase;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_TF_RGBA32) || is_valid_vc_image_buf(src, VC_IMAGE_TF_RGBX32));
   vcos_assert((dest->height & 15) == 0);

   destbase = (char *)dest->image_data + (dest->height - 1 & ~15) * dest->pitch;

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned short *destptr = (void *)((char *)destbase - y * dest->pitch);
      unsigned long const *srcptr = (void *)((char *)src->image_data + ((y + 32) & ~63) * src->pitch);
      int rowflip = (y & 32) ? -512 : 0;

      for (x = 0; x < dest->width; x += 32)
      {
         int ofs = (x >> 4) * 512 + ((y >> 4) & 1) * 256;

         tformat_subtile_load_32bit(srcptr + (ofs ^ rowflip));
         tformat_linear_store_rgba32_to_rgb565(destptr, dest->pitch);
         destptr += 16;

         if (x + 16 < dest->width)
         {
            ofs += 512;
            tformat_subtile_load_32bit(srcptr + (ofs ^ rowflip ^ 256));
            tformat_linear_store_rgba32_to_rgb565(destptr, dest->pitch);
         }
         destptr += 16;
      }
   }
   if (!vcos_verify((dest->height & 15) == 0))
   {
      /* TODO: untested code path */
      vclib_memmove(dest->image_data, (char *)dest->image_data + dest->pitch * (16 - (dest->height & 15)), dest->pitch * dest->height);
   }
}

/******************************************************************************
NAME
   vc_image_rgb888_to_tf_rgbx32

SYNOPSIS
   void vc_image_rgb888_to_tf_rgbx32(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_rgb888_to_tf_rgbx32(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform)
{
   char const *srcbase;
   int x, y, srcpitch;

   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBA32) || is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBX32));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_RGB888));

   if (vc_image_get_mipmap_type(dest,0) == VC_IMAGE_MIPMAP_LINEAR_TILE)
   {
      //VC_IMAGE_T *hack = vc_image_parmalloc(VC_IMAGE_RGBA32, "XXX HACK XXX", src->width, src->height);
      //vc_image_convert(hack, (VC_IMAGE_T*)src, 0, 0);
      linear_tile_transfer_24bit(dest, src, transform);
      //vc_image_free(hack);
      return;
   }

   if (transform)
   {
      srcbase = (char const *)src->image_data;
      srcpitch = src->pitch;
   }
   else
   {
      vcos_assert_msg((src->height & 15) == 0, "risk of illegal access at beginning of image");

      srcbase = (char const *)src->image_data + (src->height - 1) * src->pitch;
      srcpitch = -src->pitch;
   }

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned long *destptr = (void *)((char *)dest->image_data + ((y + 32) & ~63) * dest->pitch);
      unsigned long const *srcptr = (void *)(srcbase + y * srcpitch);
      int rowflip = (y & 32) ? -512 : 0;

      for (x = 0; x < dest->width; x += 32)
      {
         int ofs = (x >> 4) * 512 + ((y >> 4) & 1) * 256;

         tformat_linear_load_24bit_flip(srcptr, srcpitch);
         tformat_subtile_store_32bit(destptr + (ofs ^ rowflip));
         srcptr += 12;

         if (x + 16 < dest->width)
         {
            ofs += 512;
            tformat_linear_load_24bit_flip(srcptr, srcpitch);
            tformat_subtile_store_32bit(destptr + (ofs ^ rowflip ^ 256));
         }
         srcptr += 12;
      }
   }
}


/******************************************************************************
NAME
   vc_image_yuv420_to_tf_rgba32_linear_part

SYNOPSIS
   void vc_image_yuv420_to_tf_rgba32_linear_part(VC_IMAGE_T *dest,
         int x_off, int y_off, int width, int height,
         VC_IMAGE_T const *src, int src_x_off, int src_y_off)

FUNCTION
   x
store square part of subtile in linear tile form
width and height must equal and <= 16
currently offsets must be 0
******************************************************************************/

void vc_image_yuv420_to_tf_rgba32_linear_part(VC_IMAGE_T *dest, int x_off, int y_off, int width, int height, VC_IMAGE_T const *src, int src_x_off, int src_y_off)
{
   unsigned char const *srcbase_y;
   unsigned char const *srcbase_u;
   unsigned char const *srcbase_v;
   int srcpitch_y, srcpitch_uv;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBA32) || is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBX32));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV420));
   vcos_assert((!x_off) && (!y_off));
   vcos_assert(width == height);
   vcos_assert(width <= 16);
   vcos_assert((unsigned)(width) <= dest->width);
   vcos_assert((unsigned)(height) <= dest->height);
   vcos_assert(width <= src->width);
   vcos_assert(height <= src->height);

   if (!vcos_verify(vc_image_get_mipmap_type(dest,0) == VC_IMAGE_MIPMAP_LINEAR_TILE))
      return;

   srcpitch_y = -src->pitch;
   srcpitch_uv = srcpitch_y >> 1;
   srcbase_y = vc_image_get_y(src) - (src_y_off + height - 1) * srcpitch_y + src_x_off;
   srcbase_u = vc_image_get_u(src) - (((src_y_off + height) >> 1) - 1) * srcpitch_uv + (src_x_off >> 1);
   srcbase_v = vc_image_get_v(src) - (((src_y_off + height) >> 1) - 1) * srcpitch_uv + (src_x_off >> 1);

   {
      void *destptr = (void *)dest->image_data;
      unsigned char const *srcptr_y = srcbase_y;
      unsigned char const *srcptr_u = srcbase_u;
      unsigned char const *srcptr_v = srcbase_v;

      yuv420_to_rgb_load_32x16(srcptr_y, srcptr_u, srcptr_v, srcpitch_y, srcpitch_uv,
                               yuv_to_tf_rgba32_linear_store_16x16,
                               destptr, width, 0xff);
   }
}

/******************************************************************************
NAME
   vc_image_yuv420_to_tf_rgba32_part

SYNOPSIS
   void vc_image_yuv420_to_tf_rgba32_part(VC_IMAGE_T *dest,
         int x_off, int y_off, int width, int height,
         VC_IMAGE_T const *src, int src_x_off, int src_y_off)

FUNCTION
   x
******************************************************************************/

void vc_image_yuv420_to_tf_rgba32_part(VC_IMAGE_T *dest, int x_off, int y_off, int width, int height, VC_IMAGE_T const *src, int src_x_off, int src_y_off)
{
   unsigned char const *srcbase_y;
   unsigned char const *srcbase_u;
   unsigned char const *srcbase_v;
   int x, y, srcpitch_y, srcpitch_uv;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBA32) || is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBX32));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV420));
   vcos_assert((x_off & 31) == 0 && (y_off & 15) == 0);
   vcos_assert((width & 15) == 0 || x_off + width >= dest->width);
   vcos_assert((height & 15) == 0 || y_off + height >= dest->height);
   vcos_assert(is_within_rectangle(x_off, y_off, width, height,  0, 0, dest->width+15&~15, dest->height+15&~15));
   vcos_assert(is_within_rectangle(src_x_off, src_y_off, width, height,  0, 0, src->width, src->height));

   if (vc_image_get_mipmap_type(dest, 0) == VC_IMAGE_MIPMAP_LINEAR_TILE)
   {
      if ((width==height)&&(width<=16))
      {
         vc_image_yuv420_to_tf_rgba32_linear_part(dest, x_off, y_off, width, height, src, src_x_off, src_y_off);
      } else {
         vcos_assert(!"linear tile not yet supported unless square and <= subtile.");
      }
      return;
   }

   vcos_assert_msg((src->height & 15) == 0, "risk of illegal access at beginning of image (this comment isn't clearly thought through)");

   srcpitch_y = -src->pitch;
   srcpitch_uv = srcpitch_y >> 1;
   srcbase_y = vc_image_get_y(src) - (src_y_off + height - 1) * srcpitch_y + src_x_off;
   srcbase_u = vc_image_get_u(src) - (((src_y_off + height) >> 1) - 1) * srcpitch_uv + (src_x_off >> 1);
   srcbase_v = vc_image_get_v(src) - (((src_y_off + height) >> 1) - 1) * srcpitch_uv + (src_x_off >> 1);

   width += x_off;
   height += y_off;
   for (y = y_off; y < height; y += 16)
   {
      unsigned long *destptr = (void *)((char *)dest->image_data + ((y + 32) & ~63) * dest->pitch);
      unsigned char const *srcptr_y = srcbase_y;
      unsigned char const *srcptr_u = srcbase_u;
      unsigned char const *srcptr_v = srcbase_v;

      int rowflip = (y & 32) ? -512 : 0;

      for (x = x_off; x < width; x += 32)
      {
         int ofs = (x >> 4) * 512 + ((y >> 4) & 1) * 256;
         void *dptr1, *dptr2;

         dptr1 = destptr + (ofs ^ rowflip);
         dptr2 = destptr + ((ofs + 512) ^ rowflip ^ 256);

         yuv420_to_rgb_load_32x16(srcptr_y, srcptr_u, srcptr_v, srcpitch_y, srcpitch_uv,
                                  (x + 16 < width) ? yuv_to_tf_rgba32_store_32x16
                                  : yuv_to_tf_rgba32_store_16x16,
                                  dptr1, (char *)dptr2 - (char *)dptr1, 0xff);
         srcptr_y += 32;
         srcptr_u += 16;
         srcptr_v += 16;
      }
      srcbase_y += 16 * srcpitch_y;
      srcbase_u += 8 * srcpitch_uv;
      srcbase_v += 8 * srcpitch_uv;
   }
}

/******************************************************************************
NAME
   vc_image_yuv420_to_tf_rgba32

SYNOPSIS
   void vc_image_yuv420_to_tf_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_yuv420_to_tf_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src)
{
   vc_image_yuv420_to_tf_rgba32_part(dest, 0, 0, dest->width, dest->height, src, 0, 0);
}


/******************************************************************************
NAME
   vc_image_yuv420_to_tf_rgba32

SYNOPSIS
   void vc_image_yuv420_to_tf_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_yuv422_to_tf_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src)
{
   unsigned char const *srcbase_y;
   unsigned char const *srcbase_u;
   unsigned char const *srcbase_v;
   int x, y, srcpitch;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBA32) || is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBX32));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV422));

   if (!vcos_verify(vc_image_get_mipmap_type(dest,0) == VC_IMAGE_MIPMAP_TFORMAT))
      return;

   vcos_assert_msg((src->height & 15) == 0, "risk of illegal access at beginning of image");

   srcpitch = -src->pitch;
   srcbase_y = vc_image_get_y(src) - (src->height - 1) * srcpitch;
   srcbase_u = vc_image_get_u(src) - (src->height - 1) * srcpitch;
   srcbase_v = vc_image_get_v(src) - (src->height - 1) * srcpitch;

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned long *destptr = (void *)((char *)dest->image_data + ((y + 32) & ~63) * dest->pitch);
      unsigned char const *srcptr_y = (void *)(srcbase_y + y * srcpitch);
      unsigned char const *srcptr_u = (void *)(srcbase_u + y * srcpitch);
      unsigned char const *srcptr_v = (void *)(srcbase_v + y * srcpitch);

      int rowflip = (y & 32) ? -512 : 0;

      for (x = 0; x < dest->width; x += 32)
      {
         int ofs = (x >> 4) * 512 + ((y >> 4) & 1) * 256;
         void *dptr1, *dptr2;

         dptr1 = destptr + (ofs ^ rowflip);
         dptr2 = destptr + ((ofs + 512) ^ rowflip ^ 256);

         yuv422_to_rgb_load_32x16(srcptr_y, srcptr_u, srcptr_v, srcpitch, srcpitch,
                                  (x + 16 < dest->width) ? yuv_to_tf_rgba32_store_32x16
                                  : yuv_to_tf_rgba32_store_16x16,
                                  dptr1, (char *)dptr2 - (char *)dptr1, 0xff);
         srcptr_y += 32;
         srcptr_u += 16;
         srcptr_v += 16;
      }
   }
}

/******************************************************************************
NAME
   vc_image_yuv_uv_to_tf_rgba32

SYNOPSIS
   void vc_image_yuv_uv_to_tf_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

FUNCTION
   x
******************************************************************************/

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

void vc_image_yuv_uv_to_tf_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src)
{
   int x, y;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBA32) || is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGBX32));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV_UV));

   if (!vcos_verify(vc_image_get_mipmap_type(dest,0) == VC_IMAGE_MIPMAP_TFORMAT))
      return;

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned long *destptr = (void *)((char *)dest->image_data + ((y + 32) & ~63) * dest->pitch);

      int rowflip = (y & 32) ? -512 : 0;

      for (x = 0; x < dest->width; x += 32)
      {
         int ofs = (x >> 4) * 512 + ((y >> 4) & 1) * 256;
         void *dptr1 = destptr + (ofs ^ rowflip);
         void *dptr2 = destptr + ((ofs + 512) ^ rowflip ^ 256);
         void *srcptr_y  = get_yuv_uv_y(x, dest->height-1-y, vc_image_get_y(src), src->pitch);
         void *srcptr_uv = get_yuv_uv_uv(x, dest->height-1-y, vc_image_get_u(src), src->pitch);

         yuv_uv_to_rgb_load_32x16(srcptr_y, srcptr_uv, srcptr_uv, -VC_IMAGE_YUV_UV_STRIPE_WIDTH, -VC_IMAGE_YUV_UV_STRIPE_WIDTH,
                                  (x + 16 < dest->width) ? yuv_to_tf_rgba32_store_32x16
                                  : yuv_to_tf_rgba32_store_16x16,
                                  dptr1, (char *)dptr2 - (char *)dptr1, 0xff, dest->height-y);
      }
   }
}

/******************************************************************************
NAME
   vc_image_yuv_uv_to_tf_rgb565

SYNOPSIS
   void vc_image_yuv_uv_to_tf_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_yuv_uv_to_tf_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src)
{
   int x, y;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV_UV));

   if (!vcos_verify(vc_image_get_mipmap_type(dest,0) == VC_IMAGE_MIPMAP_TFORMAT))
      return;

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned short *destptr = (void *)((char *)dest->image_data + ((y + 32) & ~63) * dest->pitch);

      int rowflip = (y & 32) ? -1024 : 0;

      for (x = 0; x < dest->width; x += 32)
      {
         int colflip = (x & 32) ? 512 : 0;
         int ofs = (x >> 5) * 1024 + ((y >> 4) & 1) * 512;
         void *dptr1 = destptr + (ofs ^ rowflip ^ colflip);
         void *srcptr_y  = get_yuv_uv_y(x, dest->height-1-y, vc_image_get_y(src), src->pitch);
         void *srcptr_uv = get_yuv_uv_uv(x, dest->height-1-y, vc_image_get_u(src), src->pitch);

         yuv_uv_to_rgb_load_32x16(srcptr_y, srcptr_uv, srcptr_uv, -VC_IMAGE_YUV_UV_STRIPE_WIDTH, -VC_IMAGE_YUV_UV_STRIPE_WIDTH,
                                  yuv_to_tf_rgb565_store_32x16_default, dptr1, 0, 0, dest->height-y);
      }
   }
}

/******************************************************************************
NAME
   vc_image_yuv420_to_tf_rgb565

SYNOPSIS
   void vc_image_yuv420_to_tf_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_yuv420_to_tf_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src)
{
   int x, y;
   int srcpitch_y, srcpitch_uv;
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV420));

   if (!vcos_verify(vc_image_get_mipmap_type(dest,0) == VC_IMAGE_MIPMAP_TFORMAT))
      return;

   vcos_assert_msg((src->height & 15) == 0, "risk of illegal access at beginning of image");

   srcpitch_y = -src->pitch;
   srcpitch_uv = srcpitch_y >> 1;

   for (y = 0; y < dest->height; y += 16)
   {
      unsigned short *destptr = (void *)((char *)dest->image_data + ((y + 32) & ~63) * dest->pitch);
      unsigned char const *srcptr_y = vc_image_get_y(src) + (src->height - 1) * src->pitch
                                                          + y * srcpitch_y;
      unsigned char const *srcptr_u = vc_image_get_u(src) + ((src->height >> 1) - 1) * (src->pitch >> 1)
                                                          + (y >> 1) * srcpitch_uv;
      unsigned char const *srcptr_v = vc_image_get_v(src) + ((src->height >> 1) - 1) * (src->pitch >> 1)
                                                          + (y >> 1) * srcpitch_uv;

      int rowflip = (y & 32) ? -1024 : 0;

      for (x = 0; x < dest->width; x += 32)
      {
         int colflip = (x & 32) ? 512 : 0;
         int ofs = (x >> 5) * 1024 + ((y >> 4) & 1) * 512;
         void *dptr1 = destptr + (ofs ^ rowflip ^ colflip);

         yuv420_to_rgb_load_32x16(srcptr_y, srcptr_u, srcptr_v, srcpitch_y, srcpitch_uv,
                                  yuv_to_tf_rgb565_store_32x16_default, dptr1, 0);
         srcptr_y += 32;
         srcptr_u += 16;
         srcptr_v += 16;
      }
   }
}


/******************************************************************************
NAME
   vc_image_yuv420_to_tf_rgba32

SYNOPSIS
   void vc_image_yuv420_to_tf_rgba32(VC_IMAGE_T *dest, VC_IMAGE_T const *src);

FUNCTION
   x
******************************************************************************/

void vc_image_yuv422_to_tf_rgb565(VC_IMAGE_T *dest, VC_IMAGE_T const *src)
{
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_TF_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV422));

   vcos_assert(!"not implemented");
}


/* End of conversions ****************************************************}}}*/

/******************************************************************************
NAME
   vc_image_XXX_mipmap_blt_XXX

SYNOPSIS
   void vc_image_XXX_mipmap_blt_XXX(
      VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
      VC_IMAGE_T *src, int src_x_offset, int src_y_offset,
      int miplvl, int cubeface, VC_IMAGE_TRANSFORM_T transform);

FUNCTION
   Performs the same function as vc_image_convert_blt, except that you may
   choose which mipmap level of the destination image you wish to write to
******************************************************************************/

void vc_image_XXX_mipmap_blt_XXX(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                 VC_IMAGE_T *src, int src_x_offset, int src_y_offset, int miplvl, int cubeface, VC_IMAGE_TRANSFORM_T transform)
{
   if (miplvl == 0 && cubeface == 0)
   {
      // The easy case
      my_blt(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset, transform);
   }
   else
   {
      VC_IMAGE_T helper_image;

      vc_image_configure_mipmap(&helper_image, dest, miplvl, cubeface);
      vcos_assert(width == helper_image.width && height == helper_image.height);
      vcos_assert((intptr_t)helper_image.image_data >= (intptr_t)dest->image_data);
      vcos_assert((intptr_t)helper_image.image_data + helper_image.size <= (intptr_t)dest->image_data + dest->size);
      my_blt(&helper_image, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset, transform);
   }
}

/******************************************************************************
NAME
   vc_image_configure_mipmap

SYNOPSIS
   void vc_image_configure_mipmap(VC_IMAGE_T *img, VC_IMAGE_T *src, int miplvl, int cubeface)

FUNCTION
   Configure img to point to one of the mipmaps of src.
   The image_data pointers will therefore overlap, so be careful. (img is
   intended to be used only temporarily).
******************************************************************************/

void vc_image_configure_mipmap(VC_IMAGE_T *img, VC_IMAGE_T *src, int miplvl, int cubeface)
{
   vcos_assert(is_valid_vc_image_buf(src, src->type) && VC_IMAGE_IS_TFORMAT(src->type));

   vcos_assert(miplvl >= 0 && miplvl <= src->extra.tf.mipmap_levels);
   vcos_assert(cubeface >= 0 && cubeface <= 5);
   // only allow cube face selection if image is cubemapped
   vcos_assert(src->extra.tf.cube_map == 1 || cubeface == 0);

   int linear_tile = vc_image_get_mipmap_type(src, miplvl);
   int hw = src->width >> miplvl;
   int hh = src->height >> miplvl;
   int hp = vc_image_get_mipmap_pitch(src, miplvl);
   int hs = hp * vc_image_get_mipmap_padded_height(src, miplvl);
   void* hd = get_mipmap(src, miplvl, cubeface);
   if (hw < 1) hw = 1;
   if (hh < 1) hh = 1;

   img->type = src->type;
   img->width = hw;
   img->height = hh;
   img->pitch = hp;
   img->size = hs;
   img->image_data = hd;
   img->extra.tf.cube_map = 0;
   img->extra.tf.mipmap_levels = linear_tile ? -1 : 0;
   img->extra.tf.palette = src->extra.tf.palette;

   /* The img is fully contained by the src image */
   vcos_assert((int)img->image_data+img->size<=(int)src->image_data+src->size);
   vcos_assert((int)img->image_data>=(int)src->image_data);
}

/******************************************************************************
NAME
   vc_image_rebuild_mipmaps

SYNOPSIS
   void vc_image_rebuild_mipmaps(VC_IMAGE_T *dst, int filter)

FUNCTION
   Rebuild mipmaps in a t-format image

TODO
   Make this not crappy -- right now it's better than nothing but not much more
******************************************************************************/


/* lifted from vmcs/apps/transitions/bend3.c: */
static void v3d_vc_image_to_tu(int sampler, VC_IMAGE_T *image, int mipmaps_available)
{
   static volatile int mipmapmode = 5;  /* LINEAR_MIPMAP_LINEAR minification */
   static volatile float lod_bias = 0.0;
   unsigned long cfg;
   volatile unsigned long *tu_base;

   if (mipmaps_available == -1)
      mipmaps_available = 0, mipmapmode = 0;

   vcos_assert(is_valid_vc_image_buf(image, image->type));
   vcos_assert((unsigned)sampler < 8);
   vcos_assert(mipmaps_available <= max(0, image->extra.tf.mipmap_levels));

   tu_base = (volatile unsigned long *)(GR_TU_BASE0 + (GR_TU_BASE1 - GR_TU_BASE0) * sampler);

   switch (image->type)
   {
   case VC_IMAGE_TF_RGBA32:   cfg =  0 << 2;    break;
   case VC_IMAGE_TF_RGBX32:   cfg =  1 << 2;    break;
   case VC_IMAGE_TF_RGBA16:   cfg =  2 << 2;    break;
   case VC_IMAGE_TF_RGBA5551: cfg =  3 << 2;    break;
   case VC_IMAGE_TF_RGB565:   cfg =  4 << 2;    break;
   case VC_IMAGE_TF_YA88:     cfg =  7 << 2;    break;
   case VC_IMAGE_TF_BYTE:     cfg = 10 << 2;    break;
   case VC_IMAGE_TF_PAL4:     cfg =  9 << 2;    break;
   case VC_IMAGE_TF_ETC1:     cfg =  8 << 2;    break;
   case VC_IMAGE_TF_Y8:       cfg =  5 << 2;    break;
   case VC_IMAGE_TF_A8:       cfg =  6 << 2;    break;
   case VC_IMAGE_TF_SHORT:    cfg = 11 << 2;    break;
   case VC_IMAGE_TF_1BPP:     cfg = 12 << 2;    break;
   default:
      vcos_assert(!"Unsupported source image format.");
      return;
   }

   cfg |= (1 << 28) /* clamp t */
          |  (1 << 24) /* clamp s */
          |  (0 << 16) /* linear magnification */
          |  (1 << 8); /* packed RGBA32 presentation */

   if (mipmaps_available > 0)
      cfg |= (mipmapmode << 20)
             |  (0 << 6)  /* T-format */
             |  (mipmaps_available << 12);
   else if (mipmaps_available < 0)
      cfg |= (0 << 20) /* LINEAR minification */
             |  (1 << 6); /* LT-format */
   else
      cfg |= (0 << 20); /* LINEAR minification */

   if (_count(image->width) != 1 || _count(image->height) != 1)
   {
#if defined(__BCM2707A0__)
      if (image->extra.tf.mipmap_levels > 0)
         cfg |= (2 << 18); /* non-power-of-two mipmapped texture */
      else
#endif
         cfg |= (1 << 18); /* non-power-of-two texture no mipmaps */
   }

   if (image->extra.tf.cube_map != 0)
      cfg |= 1;

   tu_base[&GRTCFG0 - &GRTCS0] = cfg;
   tu_base[&GRTDIM0 - &GRTCS0] = ((_msb(image->height - 1) + 1) << 28)
                                 | (image->height << 16)
                                 | ((_msb(image->width - 1) + 1) << 12)
                                 | (image->width << 0);
   tu_base[&GRTBCOL0 - &GRTCS0] = 0xFF00FF00;
   if (image->type == VC_IMAGE_TF_PAL4)
   {
      int i;
      volatile int x = tu_base[&GRTPTBA0 - &GRTCS0];
      /* TODO: to be consistent we should probably load RGB565 data and convert it to RGBA32. */
      for (i = 0; i < 16; i++)
         tu_base[&GRTPTBA0 - &GRTCS0] = ((unsigned long const *)image->extra.tf.palette)[i];
   }
#if defined(__BCM2707A0__)
#if !defined(GRTCPX0)
#define GRTCPX0                  HW_REGISTER_RW(GR_TU_BASE0 + 0x14)
#define GRTCDS0                  HW_REGISTER_RW(GR_TU_BASE0 + 0x18)
#endif
   tu_base[&GRTCPX0 - &GRTCS0] = (uintptr_t)image->image_data;
   tu_base[&GRTCDS0 - &GRTCS0] = image->extra.tf.cube_stride_utiles << 6;
   {
      int bigwidth, bigheight;
      float lod;

      bigwidth = 1 << (_msb(image->width - 1) + 1);
      bigheight = 1 << (_msb(image->height - 1) + 1);

      lod = _flog2(min((float)image->width / bigwidth, (float)image->height / bigheight));
      lod += lod_bias;
      tu_base[&GRTLBIAS0 - &GRTCS0] = (int)(lod * 256.0f) & 0x3fff;
   }
#else
   {
      int level;
      unsigned long volatile *mpm = (void *)(((sampler & 4) == 0) ? GRTMPM0_BASE : GRTMPM1_BASE);

      mpm += 12 * (sampler & 3);
      level = 0;
      do
      {
         unsigned long address = (unsigned long)image->image_data + vc_image_get_mipmap_offset(image, level);
         *mpm++ = address | (vc_image_get_mipmap_type(image, level) == VC_IMAGE_MIPMAP_TFORMAT ? 0x80000000 : 0);
         level++;
      } while (level <= mipmaps_available);
   }
   {
      int bigwidth, bigheight;
      float lodx, lody;

      bigwidth = 1 << (_msb(image->width - 1) + 1);
      bigheight = 1 << (_msb(image->height - 1) + 1);

      lodx = _flog2((float)image->width / bigwidth);
      lodx += lod_bias;
      lody = _flog2((float)image->height / bigheight);
      lody += lod_bias;
      tu_base[&GRTLBIAS0 - &GRTCS0] = ((int)(lodx * 256.0f) & 0x3fff)
                                      | ((int)(lody * 256.0f) & 0x3fff) << 16;
   }
#endif
}


static void vc_image_rebuild_mipmaps_v3d(VC_IMAGE_T *dst)
{
   int width, height;
   int level;
   int kill_3d = 0;

   vcos_assert((_vasm("version %D") & 0x10000) != 0);
   vcos_assert(is_valid_vc_image_buf(dst, VC_IMAGE_TF_RGBA32) || is_valid_vc_image_buf(dst, VC_IMAGE_TF_RGBX32));
   /* TODO: assert that we have the appropriate 3d locks */

   width = dst->width;
   height = dst->height;

   if ((GROCFG & 1) == 0)
      kill_3d = 1;

   GROCFG = 1;
#if defined(__BCM2707A0__)
   GRTCS0 = 3;
   GRTCS1 = 3;
#else
   GRTCS0 = 1;
   GRTCS1 = 1;
#endif
   {
      int ticker = 250000000;
      while ((GRTCS0 != 0 || GRTCS1 != 0) && --ticker > 0)
         _nop();
   }

   for (level = 1; level <= dst->extra.tf.mipmap_levels; level++)
   {
      float xstep, ystep;

      width >>= 1;
      height >>= 1;

      v3d_vc_image_to_tu(0, dst, level - 1);
      v3d_vc_image_to_tu(4, dst, level - 1);

      xstep = 1.0f / (float)width;
      ystep = 1.0f / (float)height;

      if (vc_image_get_mipmap_type(dst, level) == VC_IMAGE_MIPMAP_TFORMAT)
      {
         int half_offset, half_width;
         int y;

         half_offset = (width >> 1) & ~31;
         half_width = (width - half_offset) + 15 & ~15;

         for (y = 0; y < height; y += 16)
         {
            void *pptr0, *pptr1;
            int pitch;

            pptr0 = vc_image_pixel_addr_gl(dst, 0, y, level);
            pptr1 = vc_image_pixel_addr_gl(dst, half_offset, y, level);
            pitch = (long)vc_image_pixel_addr_gl(dst, 16, y, level) - (long)pptr0;

            tformat_resize_stripe(pptr0, pitch,
                                  0, xstep,
                                  y, ystep,
                                  half_offset, half_width, pptr1);
         }
      }
      else
      {
         int half_offset, half_width;
         int y;

#if 0
         vclib_memset4((char *)dst->image_data + vc_image_get_mipmap_offset(dst, level),
                       0xFF808080,
                       (vc_image_get_mipmap_pitch(dst, level) * (height + 3 & ~3)) >> 2);
#endif

         half_offset = (width >> 1) & ~7;
         half_width = (width - half_offset) + 3 & ~3;

         for (y = 0; y < height; y += 4)
         {
            void *pptr0, *pptr1;

            pptr0 = vc_image_pixel_addr_gl(dst, 0, y, level);
            pptr1 = vc_image_pixel_addr_gl(dst, half_offset, y, level);

            tformat_resize_utile_stripe(pptr0, 64,
                                  0, xstep,
                                  y, ystep,
                                  half_offset, half_width, pptr1);
         }
      }
   }

   if (kill_3d)
      GROCFG = 0;
}


void vc_image_rebuild_mipmaps(VC_IMAGE_T *image, int filter)
{
   int level = 0, logutw = 0, loguth = 0;
   void (*fp)(int, void*, const void*, const void*) = NULL;

   extern void vc_image_tfsub2ut_pal8(int, void*, const void*, const void*);
   extern void vc_image_tfsub2ut_y8(int, void*, const void*, const void*);
   extern void vc_image_tfsub2ut_4444(int, void*, const void*, const void*);
   extern void vc_image_tfsub2ut_565(int, void*, const void*, const void*);
   extern void vc_image_tfsub2ut_5551(int, void*, const void*, const void*);
   extern void vc_image_tfsub2ut_32(int, void*, const void*, const void*);

   vcos_assert(is_valid_vc_image_buf(image, image->type) && VC_IMAGE_IS_TFORMAT(image->type));
   if (filter == 1)
   {
      vc_image_rebuild_mipmaps_v3d(image);
      return;
   }

   switch (image->type) {
   case VC_IMAGE_TF_PAL8:
   case VC_IMAGE_TF_BYTE:
      fp = vc_image_tfsub2ut_pal8; logutw = 3;  loguth = 3;
      break;
   case VC_IMAGE_TF_Y8:
   case VC_IMAGE_TF_A8:
      fp = vc_image_tfsub2ut_y8;   logutw = 3;  loguth = 3;
      break;
   case VC_IMAGE_TF_RGBA16:
      fp = vc_image_tfsub2ut_4444; logutw = 3;  loguth = 2;
      break;
   case VC_IMAGE_TF_RGB565:
      fp = vc_image_tfsub2ut_565;  logutw = 3;  loguth = 2;
      break;
   case VC_IMAGE_TF_RGBA5551:
      fp = vc_image_tfsub2ut_5551; logutw = 3;  loguth = 2;
      break;
   case VC_IMAGE_TF_RGBA32:
   case VC_IMAGE_TF_RGBX32:
      fp = vc_image_tfsub2ut_32;   logutw = 2;  loguth = 2;
      break;
   default:
      vcos_assert(!"unsupported image type");
      return;
   }

   for(level = 0; level < image->extra.tf.mipmap_levels; ++level) {
      /* Downsample from level to level+1 */
      unsigned i, j, w, h, lin0, lin1, pitch0, pitch1;
      unsigned char * ptr0;
      unsigned char * ptr1;
      ptr0   = ((unsigned char *)(image->image_data)) + vc_image_get_mipmap_offset(image, level);
      ptr1   = ((unsigned char *)(image->image_data)) + vc_image_get_mipmap_offset(image, level+1);
      lin0   = vc_image_get_mipmap_type(image, level)   == VC_IMAGE_MIPMAP_LINEAR_TILE;
      lin1   = vc_image_get_mipmap_type(image, level+1) == VC_IMAGE_MIPMAP_LINEAR_TILE;
      pitch0 = vc_image_get_mipmap_pitch(image, level)   << loguth; // bytes per vertical uTile
      pitch1 = vc_image_get_mipmap_pitch(image, level+1) << loguth; // bytes per vertical uTile

      // If the source image has width==1 or height==1, duplicate a row or column.
      // This allows us to use factor-of-2 decimation even when actual factor is unity.
      // XXX by the time we get to this stage, assume that image occupies only one uTile.
      if ((image->height >> level) <= 1) {
         memcpy(ptr0 + (64>>loguth), ptr0, (64>>loguth));
      }
      if ((image->width >> level) <= 1) {
         for(i = 0; i < 64; i += (64>>loguth)) {
            memcpy(ptr0 + i + (64>>(loguth+logutw)), ptr0 + i, 64>>(loguth+logutw));
         }
      }

      // Destination image size, rounded up to whole number of uTiles.
      w  = _max(1, image->width >> (level+1));
      h  = _max(1, image->height >> (level+1));
      w = (w+(1<<logutw)-1) >> logutw;
      h = (h+(1<<loguth)-1) >> loguth;

      /* XXX We assume that it's a factor of 2 at every level. At some levels the
         factor is slightly greater than 2, and and we'll lose the odd row/column. */

      for(i = 0; i < h; ++i)
      {
         unsigned char * dst;
         const unsigned char * sr0;
         const unsigned char * sr1;

         // Get pointers to the start of the ith row of uTiles (or i*2 in src)
         if (lin0) {
            sr0 = ptr0 + (i*2)*pitch0;
            sr1 = sr0 + pitch0;
         }
         else {
            sr0 = ptr0 + (((i*2)+8)&~15)*pitch0;
            if (i&4) sr0 -= 2048;
            sr0 += (i&3) * 512;
            sr1 = sr0 + 256;
         }
         if (lin1) {
            dst = ptr1 + i*pitch1;
         }
         else {
            dst = ptr1 + ((i+8)&~15)*pitch1;
            if (i&8) dst -= 2048;
            dst += (i&7) * 256;
         }

         for(j = 0; j < w; j+=2)
         {
            // Process 2 uTiles using assembler routine
            fp(w-j, dst, sr0, sr1);

            // Advance T-Format pointers
            if (lin0) {
               sr0 += 256;
               sr1 = sr0 + pitch0;
            }
            else {
               sr0 = sr0 - 1024*((i&6)^(j&2)) + 3072;
               sr1 = sr0 + 256;
            }
            if (!lin1 && (j&2)) {
               dst = dst - 512*((i&12)^(j&4)) + 2816;
            }
            dst += 128;
         }
      }
   }
}


/******************************************************************************
NAME
   vc_image_tf_hardware_to_tf_rgba32

SYNOPSIS
   void vc_image_tf_hardware_to_tf_rgba32(VC_IMAGE_T *dst, VC_IMAGE_T const *src)

FUNCTION
   Convert anything a texture unit can handle into t-format RGBA32.

******************************************************************************/

void vc_image_tf_hardware_to_tf_rgba32(VC_IMAGE_T *dst, VC_IMAGE_T *src)
{
   int width, height;
   int level = 0;
   int kill_3d = 0;

   vcos_assert((_vasm("version %D") & 0x10000) != 0);
   vcos_assert(is_valid_vc_image_buf(dst, VC_IMAGE_TF_RGBA32) || is_valid_vc_image_buf(dst, VC_IMAGE_TF_RGBX32));
   vcos_assert(is_valid_vc_image_buf(src, src->type));
   /* TODO: assert that we have the appropriate 3d locks */

   width = dst->width;
   height = dst->height;
   vcos_assert(width == src->width && height == src->height);

   if ((GROCFG & 1) == 0)
      kill_3d = 1;

   GROCFG = 1;
#if defined(__BCM2707A0__)
   GRTCS0 = 3;
   GRTCS1 = 3;
#else
   GRTCS0 = 1;
   GRTCS1 = 1;
#endif
   {
      int ticker = 250000000;
      while ((GRTCS0 != 0 || GRTCS1 != 0) && --ticker > 0)
         _nop();
   }

   {
      int half_offset, half_width;
      float xstep, ystep;
      int y;

      v3d_vc_image_to_tu(0, src, -1);
      v3d_vc_image_to_tu(4, src, -1);

      half_offset = (width >> 1) & ~31;
      half_width = (width - half_offset) + 15 & ~15;

      xstep = 1.0f / (float)width;
      ystep = 1.0f / (float)height;

      for (y = 0; y < height; y += 16)
      {
         void *pptr0, *pptr1;
         int pitch;

         pptr0 = vc_image_pixel_addr_gl(dst, 0, y, level);
         pptr1 = vc_image_pixel_addr_gl(dst, half_offset, y, level);
         pitch = (long)vc_image_pixel_addr_gl(dst, 16, y, level) - (long)pptr0;

         tformat_resize_stripe(pptr0, pitch,
                               0, xstep,
                               y, ystep,
                               half_offset, half_width, pptr1);
      }
   }

   if (kill_3d)
      GROCFG = 0;
}



/******************************************************************************
Static function definitions.
******************************************************************************/

static void my_blt(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset, VC_IMAGE_TRANSFORM_T transform)
{
   int just_copy = 0;
   int transform2 = 0;
   if (dest->width == width && dest->height == height
         && src->width == width && src->height == height
         && x_offset == 0 && y_offset == 0 && src_x_offset == 0 && src_y_offset == 0)
   {
      just_copy = 1;
   }
   if (src->type == VC_IMAGE_OPENGL && just_copy)
   {
      int dt = dest->type;
      int st = src->extra.opengl.format_and_type;

      if (transform == 0)
         transform2 = TRANSFORM_VFLIP;
      else if (transform == TRANSFORM_VFLIP)
         transform2 = 0;
      else
      {
         vcos_assert(!"unsupported transform");
         return;
      }

      if (dt == VC_IMAGE_TF_RGB565 && st == VC_IMAGE_OPENGL_RGB24 && just_copy)
      {
         if (transform == 0)
            transform2 = TRANSFORM_VFLIP;
         else if (transform == TRANSFORM_VFLIP)
            transform2 = 0;
         else
         {
            vcos_assert(!"unsupported transform");
            return;
         }

         vc_image_raster_to_tf4(dest, src, transform2, tformat_linear_load_24bit_flip_rgb24_to_rgb565);
         return;
      }
      if (dt == VC_IMAGE_TF_RGBA5551 && st == VC_IMAGE_OPENGL_RGBA32 && just_copy)
      {
         if (transform == 0)
            transform2 = TRANSFORM_VFLIP;
         else if (transform == TRANSFORM_VFLIP)
            transform2 = 0;
         else
         {
            vcos_assert(!"unsupported transform");
            return;
         }

         vc_image_raster_to_tf4(dest, src, transform2, tformat_linear_load_32bit_flip_rgba32_to_rgba5551);
         return;
      }

      if (dt==VC_IMAGE_TF_RGBX32 && st==VC_IMAGE_OPENGL_RGB24)
      {
         vc_image_raster_to_tf4(dest, src, transform2, tformat_linear_load_24bit_flip);
         return;
      }
      else if (
         ((dt==VC_IMAGE_TF_RGBA32||dt==VC_IMAGE_TF_RGBX32) && st==VC_IMAGE_OPENGL_RGBA32)
         || (dt==VC_IMAGE_TF_RGBA16 && st==VC_IMAGE_OPENGL_RGBA16)
         || (dt==VC_IMAGE_TF_RGBA5551 && st==VC_IMAGE_OPENGL_RGBA5551)
         || (dt==VC_IMAGE_TF_RGB565 && st==VC_IMAGE_OPENGL_RGB565))
      {
         vc_image_raster_to_tf4(dest, src, transform2, tformat_linear_load_32bit_flip);
         return;
      }
      else if (dt==VC_IMAGE_TF_YA88 && st==VC_IMAGE_OPENGL_YA88)
      {
         //Workaround HW-1334
         vc_image_raster_to_tf4(dest, src, transform2, tformat_linear_load_32bit_byteswap);
         return;
      }
      else if (
         (dt==VC_IMAGE_TF_ETC1 && st==VC_IMAGE_OPENGL_ETC1))
      {
         vc_image_raster_to_tf4(dest, src, transform2, tformat_linear_load_32bit_flip_etc1_workarounds);
         return;
      }
      else if (
         (dt==VC_IMAGE_TF_Y8 && st==VC_IMAGE_OPENGL_Y8)
         ||(dt==VC_IMAGE_TF_A8 && st==VC_IMAGE_OPENGL_A8))
      {
         vc_image_raster_to_tf8(dest, src, transform2, tformat_linear_load_8bit_flip);
         return;
      }
      else if (dt==VC_IMAGE_TF_PAL4 && (
                  st==VC_IMAGE_OPENGL_PALETTE4_RGB24
                  || st==VC_IMAGE_OPENGL_PALETTE4_RGBA32
                  || st==VC_IMAGE_OPENGL_PALETTE4_RGB565
                  || st==VC_IMAGE_OPENGL_PALETTE4_RGBA16
                  || st==VC_IMAGE_OPENGL_PALETTE4_RGB5551))
      {
         if(dest->extra.tf.palette)
         {
            free(dest->extra.tf.palette);
         }
         dest->extra.tf.palette = malloc_palette(src->extra.opengl.palette, st, 16);
         //TODO: make sure palette doesn't memory-leak
         vc_image_raster_to_tf8(dest, src, transform2, tformat_linear_load_8bit_nibbleswap);
         return;
      }
      else if (
         (dt==VC_IMAGE_TF_RGBX32 && st==VC_IMAGE_OPENGL_PALETTE8_RGB24)
         || ((dt==VC_IMAGE_TF_RGBA32||dt==VC_IMAGE_TF_RGBX32) && st==VC_IMAGE_OPENGL_PALETTE8_RGBA32)
         || (dt==VC_IMAGE_TF_RGB565 && st==VC_IMAGE_OPENGL_PALETTE8_RGB565)
         || (dt==VC_IMAGE_TF_RGBA16 && st==VC_IMAGE_OPENGL_PALETTE8_RGBA16)
         || (dt==VC_IMAGE_TF_RGBA5551 && st==VC_IMAGE_OPENGL_PALETTE8_RGB5551))
      {
         LOAD_PROC4_T load_raster = tformat_linear_load_pal32bit;
         switch (st)
         {
         case VC_IMAGE_OPENGL_PALETTE8_RGB24:
            load_raster = tformat_linear_load_pal32bit;
            expand_palette(vc_image_temp_palette, src->extra.opengl.palette, st, 256);
            break;
         case VC_IMAGE_OPENGL_PALETTE8_RGBA32:
            load_raster = tformat_linear_load_pal32bit;
            memcpy(vc_image_temp_palette, src->extra.opengl.palette, 4 * 256);
            break;
         case VC_IMAGE_OPENGL_PALETTE8_RGB565:
         case VC_IMAGE_OPENGL_PALETTE8_RGBA16:
         case VC_IMAGE_OPENGL_PALETTE8_RGB5551:
            load_raster = tformat_linear_load_pal16bit;
            memcpy(vc_image_temp_palette, src->extra.opengl.palette, 2 * 256);
            break;
         default:
            vcos_assert(!"Unsupported source image type.");
            return;
         }
         vc_image_raster_to_tf4(dest, src, transform2, load_raster);
         return;
      }
   }
   if (dest->type == VC_IMAGE_OPENGL && x_offset == 0 && y_offset == 0)
   {
      int dt = dest->extra.opengl.format_and_type;
      int st = src->type;
      if (transform == 0)
         transform2 = TRANSFORM_VFLIP;
      else if (transform == TRANSFORM_VFLIP)
         transform2 = 0;
      else
      {
         vcos_assert(!"unsupported transform");
         return;
      }

      vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height,  0, 0, src->width, src->height));
      if (
         (st==VC_IMAGE_TF_RGBA32   && dt==VC_IMAGE_OPENGL_RGBA32)
      || (st==VC_IMAGE_TF_RGBA16   && dt==VC_IMAGE_OPENGL_RGBA16)
      || (st==VC_IMAGE_TF_RGBA5551 && dt==VC_IMAGE_OPENGL_RGBA5551)
      || (st==VC_IMAGE_TF_RGB565   && dt==VC_IMAGE_OPENGL_RGB565))
      {
         int sixteenbit = (st==VC_IMAGE_TF_RGBA16 || st==VC_IMAGE_TF_RGBA5551 || st==VC_IMAGE_TF_RGB565);
         if (sixteenbit)
         {
            vcos_assert((src_x_offset & 1) == 0);
            vcos_assert((width & 1) == 0);
         }
         int shift = sixteenbit ? 1 : 2;

         vc_image_tf_to_raster4(dest, src, transform2,
            tformat_linear_tile_load_32bit, tformat_raster_store4,
            src_x_offset << shift,
            src_y_offset,
            (src_x_offset + width) << shift,
            src_y_offset + height);
         return;
      }
      else if ((
         st==VC_IMAGE_TF_RGBA32
      || st==VC_IMAGE_TF_RGBX32
      || st==VC_IMAGE_TF_RGBA16
      || st==VC_IMAGE_TF_RGBA5551
      || st==VC_IMAGE_TF_RGB565
      ) && (
         dt==VC_IMAGE_OPENGL_RGBA32
      || dt==VC_IMAGE_OPENGL_RGB24
      || dt==VC_IMAGE_OPENGL_YA88
      || dt==VC_IMAGE_OPENGL_Y8
      || dt==VC_IMAGE_OPENGL_A8))
      {
         LOAD_TFORMAT_PROC_T loadt;
         STORE_PROC4_T storer;
         int sixteenbit = 0;
         switch (st)
         {
            default:
               vcos_assert(!"unsupported source image type");
               /* Fall through to avoid compiler warning uninitialised variable */
            case VC_IMAGE_TF_RGBA32:
               loadt = tformat_linear_tile_load_32bit;
               break;
            case VC_IMAGE_TF_RGBX32:
               loadt = load_tformat_opaque;
               break;
            case VC_IMAGE_TF_RGBA16:
               loadt = load_tformat_4444_to_8888;
               sixteenbit = 1;
               break;
            case VC_IMAGE_TF_RGBA5551:
               loadt = load_tformat_5551_to_8888;
               sixteenbit = 1;
               break;
            case VC_IMAGE_TF_RGB565:
               loadt = load_tformat_565_to_8888;
               sixteenbit = 1;
               break;
         }
         switch (dt)
         {
            default:
               vcos_assert(!"unsupported dest image type");
               /* Fall through to avoid compiler warning uninitialised variable */
            case VC_IMAGE_OPENGL_RGBA32:
               storer = sixteenbit ? tformat_raster_store4_2 : tformat_raster_store4;
               break;
            case VC_IMAGE_OPENGL_RGB24:
               storer = sixteenbit ? tformat_raster_store_rgb888_2 : tformat_raster_store_rgb888;
               break;
            case VC_IMAGE_OPENGL_YA88:
               storer = sixteenbit ? raster_store_ya2 : raster_store_ya;
               break;
            case VC_IMAGE_OPENGL_Y8:
               storer = sixteenbit ? raster_store_y2 : raster_store_y;
               break;
            case VC_IMAGE_OPENGL_A8:
               storer = sixteenbit ? raster_store_a2 : raster_store_a;
               break;
         }
         int shift = sixteenbit ? 1 : 2;
         vc_image_tf_to_raster4(dest, src, transform2, loadt, storer,
            src_x_offset << shift,
            src_y_offset,
            (src_x_offset + width) << shift,
            src_y_offset + height);
         return;
      }
   }
   if (src->type == VC_IMAGE_TF_RGBA32 && dest->type == VC_IMAGE_TF_RGBA32)
   {
      vcos_assert(transform == 0);
      vc_image_tf_to_tf(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
      return;
   }

   /*if (transform == VC_IMAGE_ROT0)
   {
      vc_image_convert_blt(dest, x_offset, y_offset, width, height, (VC_IMAGE_T*)src, src_x_offset, src_y_offset);
      return;
   }*/
   vcos_assert(!"Can't do this transformation");
}

/******************************************************************************
NAME
   linear_tile_transfer

SYNOPSIS
   ?

FUNCTION
   Loads 16 microtiles of image data at a time into the VRF
   Then write them out in linear tile format.

   3333333444444444444
   2222222222333333333
   1111111111111222222
   0000000000000000111
******************************************************************************/

static void linear_tile_transfer_24bit(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform)
{
   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   /*if ((src->width & 3)!=0 || (src->height & 3)!=0)
   {
      vclib_memset4(dest->image_data, 0xffff00ff, 16);
      //printf("Could not convert.\n");
      return;
   }*/
   vcos_assert(src->pitch == src->width * 3);
   int w = (src->width + 3) / 4;
   int h = (src->height + 3) / 4;
   int utiles = w * h;
   unsigned int i;
   unsigned long* destptr = dest->image_data;
   const char* srcbase;
   int srcpitch;

   if (transform)
   {
      srcbase = (char*)src->image_data;
      srcpitch = src->pitch;
   }
   else
   {
      srcbase = (char*)src->image_data + (src->height-1) * src->pitch;
      srcpitch = -src->pitch;
   }

   for (i = 0; i < utiles; i+=16)
   {
      unsigned int y = i / w;
      unsigned int x = i % w;
      vcos_assert(i == y * w + x);
      const char* ptr = srcbase + srcpitch * (4 * y) + 12 * x;
      tformat_gather_linear_utiles_24bit(ptr, srcpitch, 3 * (w - x), (dest->width*3+3)/4);
#if !defined(RGB888)
      tformat_red_blue_swap();
#endif
      tformat_utiles_store_32bit(min(16, utiles-i), destptr);
      destptr += 256;
   }
}

/******************************************************************************
NAME
   vc_image_raster_to_ltile4

SYNOPSIS
   vc_image_raster_to_ltile4(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_PROC4_T load_raster)

FUNCTION
   General function for converting raster format image data into linear tile
   image data. This deals with the t-formats where 4 raster lines correspond to
   1 microtile (16bit, 32bit and ETC1 formats).
******************************************************************************/

static void vc_image_raster_to_ltile4(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_PROC4_T load_raster)
{
   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   int x, y;
   int w = ((tformat_adjusted_width4(dest)-1)|3)+1;
   int h = ((tformat_adjusted_height4(dest)-1)|3)+1;
   int pit = tformat_adjusted_pitch4(dest);
   const char* srcbase;
   int srcpitch;

   if (transform)
   {
      srcbase = (char*)src->image_data;
      srcpitch = src->pitch;
   }
   else
   {
      if (src->extra.opengl.format_and_type == VC_IMAGE_OPENGL_ETC1)
      {
         //XXX hack to get flipping ETC1 images correct
         srcbase = (char *)src->image_data + (src->height/4 - 1) * src->pitch;
      }
      else
      {
         srcbase = (char *)src->image_data + (src->height - 1) * src->pitch;
      }
      srcpitch = -src->pitch;
   }

   for (y = 0; y < h; y += 16)
   {
      unsigned long *destptr = (void *)((char *)dest->image_data + y * pit);
      unsigned long const *srcptr = (void *)(srcbase + y * srcpitch);

      for (x = 0; x < w; x += 16)
      {
         srcptr = load_raster(srcptr, srcpitch);
         tformat_linear_tile_store_32bit(destptr, min(w-x,16), min(h-y,16), pit);

         destptr += 64;
      }
   }
}

/******************************************************************************
NAME
   vc_image_raster_to_ltile8

SYNOPSIS
   vc_image_raster_to_ltile8(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_PROC4_T load_raster)

FUNCTION
   Converts from raster format to 1, 4, or 8-bit t-format. This does not
   include ETC1.
******************************************************************************/

static void vc_image_raster_to_ltile8(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_PROC8_T load_raster)
{
   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   int x, y;
   int w = ((tformat_adjusted_width8(dest)-1)|7)+1;
   int h = ((dest->height-1)|7)+1;
   int pit = dest->pitch;
   const char* srcbase;
   int srcpitch;

   if (transform)
   {
      srcbase = (char*)src->image_data;
      srcpitch = src->pitch;
   }
   else
   {
      srcbase = (char*)src->image_data + (src->height-1) * src->pitch;
      srcpitch = -src->pitch;
   }

   for (y = 0; y < h; y += 32)
   {
      unsigned long *destptr = (void *)((char *)dest->image_data + y * pit);
      unsigned long const *srcptr = (void *)(srcbase + y * srcpitch);

      for (x = 0; x < w; x += 64)
      {
         srcptr = load_raster(srcptr, srcpitch);
         tformat_linear_tile_store_8bit(destptr, min(w-x,64), min(h-y,32), pit);

         destptr += 128;
      }
   }
}

/******************************************************************************
NAME
   get_mipmap

SYNOPSIS
   void *get_mipmap(VC_IMAGE_T *image, int miplvl, int cubeface);

FUNCTION
   Asserts that the image is t-format and the given mipmap level is valid.
   Returns a pointer to the start of image data for that mipmap.
******************************************************************************/
static void *get_mipmap(VC_IMAGE_T *image, int miplvl, int cubeface)
{
   int mipmap_levels = image->extra.tf.mipmap_levels;
   vcos_assert(is_valid_vc_image_buf(image, image->type) && VC_IMAGE_IS_TFORMAT(image->type));
   vcos_assert(miplvl >= 0);
   vcos_assert(mipmap_levels != -1 || miplvl == 0);
   vcos_assert(mipmap_levels == -1 || miplvl <= mipmap_levels);
   vcos_assert(cubeface >= 0 && cubeface <= 5);
   // only allow cube face selection if image is cubemapped
   vcos_assert(image->extra.tf.cube_map == 1 || cubeface == 0);

#ifdef __BCM2707A0__
   return (char*)image->image_data
          + vc_image_get_mipmap_offset(image, miplvl)
          + cubeface * (image->extra.tf.cube_stride_utiles << 6);
#else
   int mipmapsize = vc_image_get_mipmap_pitch(image, miplvl) * vc_image_get_mipmap_padded_height(image, miplvl);
   return (char*)image->image_data
          + vc_image_get_mipmap_offset(image, miplvl)
          + cubeface * mipmapsize;
#endif
}
















/******************************************************************************
NAME
   vc_image_raster_to_tf8

SYNOPSIS
   void vc_image_raster_to_tf8(VC_IMAGE_T *dest, VC_IMAGE_T const *src, LOAD_PROC8_T load_raster);

FUNCTION
   Converts from 1, 4, or 8-bit raster format to t-format (assuming compatible
   bit depth on source and destination).
******************************************************************************/

static void vc_image_raster_to_tf8(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_PROC8_T load_raster)
{
   char const *srcbase;
   int x, y, srcpitch;
   int w = tformat_adjusted_width8(dest);
   int h = dest->height;
   int pit = dest->pitch;

   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, dest->type) && is_valid_vc_image_buf(src, src->type));

   if (vc_image_get_mipmap_type(dest,0) == VC_IMAGE_MIPMAP_LINEAR_TILE)
   {
      vc_image_raster_to_ltile8(dest, src, transform, load_raster);
      return;
   }


   if (transform)
   {
      srcbase = (char *)src->image_data;
      srcpitch = src->pitch;
   }
   else
   {
      vcos_assert_msg((src->height & 31) == 0, "risk of illegal access at beginning of image");

      srcbase = (char *)src->image_data + (src->height - 1) * src->pitch;
      srcpitch = -src->pitch;
   }

   for (y = 0; y < h; y += 32)
   {
      unsigned long *destptr = (void *)((char *)dest->image_data + ((y + 64) & ~127) * pit);
      unsigned long const *srcptr = (void *)(srcbase + y * srcpitch);
      int rowflip = (y & 64) ? -512 : 0;

      for (x = 0; x < w; x += 64)
      {
         int ofs = (x >> 5) * 512 + ((y >> 5) & 1) * 256;

         srcptr = load_raster(srcptr, srcpitch);
         if ((x + 16 < dest->width))
         {
            tformat_subtiles_store_8bit(destptr + (ofs ^ rowflip), destptr + ((ofs+512) ^ rowflip ^ 256));
         }
         else
         {
            tformat_subtiles_store_8bit(destptr + (ofs ^ rowflip), 0);
         }
      }
   }
}

/******************************************************************************
NAME
   vc_image_raster_to_tf4

SYNOPSIS
   vc_image_raster_to_tf4(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_PROC4_T load_raster)

FUNCTION
   General function for converting raster format image data into t-format image
   data. This deals with the t-formats where 4 raster lines correspond to 1
   microtile (16bit, 32bit and ETC1 formats).

   The raster loader procedure is usually tformat_linear_load_32bit_flip, but a
   different one can be supplied if any conversion needs doing.
******************************************************************************/

static void vc_image_raster_to_tf4(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_PROC4_T load_raster)
{
   char const *srcbase;
   int x, y, srcpitch;
   int w = tformat_adjusted_width4(dest);
   int h = tformat_adjusted_height4(dest);
   int pit = tformat_adjusted_pitch4(dest);

   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, dest->type) && is_valid_vc_image_buf(src, src->type));

   if (vc_image_get_mipmap_type(dest,0) == VC_IMAGE_MIPMAP_LINEAR_TILE)
   {
      vc_image_raster_to_ltile4(dest, src, transform, load_raster);
      return;
   }

   if (transform)
   {
      srcbase = (char *)src->image_data;
      srcpitch = src->pitch;
   }
   else
   {
      vcos_assert_msg((src->height & 15) == 0, "risk of illegal access at beginning of image");

      if (src->extra.opengl.format_and_type == VC_IMAGE_OPENGL_ETC1)
      {
         //XXX hack to get flipping ETC1 images correct
         srcbase = (char *)src->image_data + (src->height/4 - 1) * src->pitch;
      }
      else
      {
         srcbase = (char *)src->image_data + (src->height - 1) * src->pitch;
      }
      srcpitch = -src->pitch;
   }

   for (y = 0; y < h; y += 16)
   {
      unsigned long *destptr = (void *)((char *)dest->image_data + ((y + 32) & ~63) * pit);
      unsigned long const *srcptr = (void *)(srcbase + y * srcpitch);
      int rowflip = (y & 32) ? -512 : 0;

      for (x = 0; x < w; x += 32)
      {
         int ofs = (x >> 4) * 512 + ((y >> 4) & 1) * 256;

         srcptr = load_raster(srcptr, srcpitch);
         tformat_subtile_store_32bit(destptr + (ofs ^ rowflip));

         if (x + 16 < w)
         {
            ofs += 512;
            srcptr = load_raster(srcptr, srcpitch);
            tformat_subtile_store_32bit(destptr + (ofs ^ rowflip ^ 256));
         }
      }
   }
}

static int tformat_adjusted_width4(VC_IMAGE_T const *img)
{
   switch (img->type)
   {
   case VC_IMAGE_TF_RGBA32:
   case VC_IMAGE_TF_RGBX32:
      return img->width;
   case VC_IMAGE_TF_RGB565:
   case VC_IMAGE_TF_RGBA16:
   case VC_IMAGE_TF_RGBA5551:
   case VC_IMAGE_TF_YA88:
   case VC_IMAGE_TF_SHORT:
      return (img->width+1) >> 1;
   case VC_IMAGE_TF_ETC1:
      return (img->width+1) >> 1;
   default:
      vcos_assert(!"unsupported image type");
      return 0;
   }
}

static int tformat_adjusted_height4(VC_IMAGE_T const *img)
{
   switch (img->type)
   {
   case VC_IMAGE_TF_RGBA32:
   case VC_IMAGE_TF_RGBX32:
   case VC_IMAGE_TF_RGB565:
   case VC_IMAGE_TF_RGBA16:
   case VC_IMAGE_TF_RGBA5551:
   case VC_IMAGE_TF_YA88:
   case VC_IMAGE_TF_SHORT:
      return img->height;
   case VC_IMAGE_TF_ETC1:
      return (img->height+3) >> 2;
   default:
      vcos_assert(!"unsupported image type");
      return 0;
   }
}

static int tformat_adjusted_pitch4(VC_IMAGE_T const *img)
{
   switch (img->type)
   {
   case VC_IMAGE_TF_RGBA32:
   case VC_IMAGE_TF_RGBX32:
   case VC_IMAGE_TF_RGB565:
   case VC_IMAGE_TF_RGBA16:
   case VC_IMAGE_TF_RGBA5551:
   case VC_IMAGE_TF_YA88:
   case VC_IMAGE_TF_SHORT:
      return img->pitch;
   case VC_IMAGE_TF_ETC1:
      return img->pitch << 2;
   default:
      vcos_assert(!"unsupported image type");
      return 0;
   }
}

static int tformat_adjusted_width8(VC_IMAGE_T const *img)
{
   switch (img->type)
   {
   case VC_IMAGE_TF_Y8:
   case VC_IMAGE_TF_A8:
   case VC_IMAGE_TF_BYTE:
      return img->width;
   case VC_IMAGE_TF_PAL4:
      return (img->width+1) >> 1;
   case VC_IMAGE_TF_1BPP:
      return (img->width+7) >> 3;
   default:
      vcos_assert(!"unsupported image type");
      return 0;
   }
}

/******************************************************************************
NAME
   vc_image_tf_to_raster4

SYNOPSIS
   void vc_image_tf_to_raster4(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_TFORMAT_PROC_T load_tformat, STORE_PROC4_T store_raster, int xoff, int yoff);

FUNCTION
   Converts t-format data to raster format. The supplied storer function is
   responsible for doing partial writes around the image boundary if we are not
   16-pixel aligned. It may also do colour conversion if this is needed.

   The supplied loader function is responsible for loading t-format data and
   rearranging it into t-format. This may also do some colour conversion, which
   in case we need both a front and back end for colour conversion.
******************************************************************************/

static void vc_image_tf_to_raster4(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_TFORMAT_PROC_T load_tformat, STORE_PROC4_T store_raster, int xoff, int yoff, int xmax, int ymax)
{
   char *destbase;
   int x, y, destpitch;
   int pit = tformat_adjusted_pitch4(src);

   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));

   if (vc_image_get_mipmap_type(src,0) == VC_IMAGE_MIPMAP_LINEAR_TILE)
   {
      vc_image_ltile_to_raster4(dest, src, transform, load_tformat, store_raster, xoff, yoff, xmax, ymax);
      return;
   }

   if (transform)
   {
      destbase = (char *)dest->image_data;
      destpitch = dest->pitch;
   }
   else
   {
      destbase = (char *)dest->image_data + (dest->height - 1) * dest->pitch;
      destpitch = -dest->pitch;
   }

   char *destrowptr = destbase;
   for (y = yoff & ~15; y < ymax; y += 16)
   {
      unsigned long const *srcptr = (void *)((char *)src->image_data + ((y + 32) & ~63) * pit);
      unsigned long *destptr = (unsigned long*)destrowptr;
      int rowflip = (y & 32) ? -512 : 0;
      int ystart = yoff <= y ? 0 : yoff - y;
      int ycount = min(16 - ystart, ymax - y - ystart);

      vcos_assert(ystart >= 0 && ystart <= 15 && ycount >= 0 && ystart + ycount <= 16);

      for (x = xoff & ~127; x < xmax; x += 128)
      {
         int ofs = (x >> 6) * 512 + ((y >> 4) & 1) * 256;

         if (x > xoff - 64)
         {
            load_tformat(srcptr + (ofs ^ rowflip), 64);
            destptr = store_raster(ycount, destptr, destpitch, xoff - x, x + 64 - xmax, ystart);
         }

         if (x + 64 < xmax)
         {
            ofs += 512;
            load_tformat(srcptr + (ofs ^ rowflip ^ 256), 64);
            destptr = store_raster(ycount, destptr, destpitch, xoff - x - 64, x + 128 - xmax, ystart);
         }
      }

      destrowptr += ycount * destpitch;
   }
}
/************************************************************************
NAME
   vc_image_ltile_to_raster4

SYNOPSIS
   vc_image_ltile_to_raster4(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_TFORMAT_PROC_T load_tformat, STORE_PROC4_T load_raster)

FUNCTION
   General function for converting linear tile image data into raster format.
   This deals with the t-formats where 4 raster lines correspond to
   1 microtile (16bit, 32bit and ETC1 formats).
******************************************************************************/

static void vc_image_ltile_to_raster4(VC_IMAGE_T *dest, VC_IMAGE_T const *src, VC_IMAGE_TRANSFORM_T transform, LOAD_TFORMAT_PROC_T load_tformat, STORE_PROC4_T store_raster, int xoff, int yoff, int xmax, int ymax)
{
   int x, y, destpitch;
   int pit = tformat_adjusted_pitch4(src);
   char *destbase;

   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, dest->type) && is_valid_vc_image_buf(src, src->type));

   if (transform)
   {
      destbase = (char *)dest->image_data;
      destpitch = dest->pitch;
   }
   else
   {
      destbase = (char *)dest->image_data + (dest->height - 1) * dest->pitch;
      destpitch = -dest->pitch;
   }

   char *destrowptr = destbase;
   for (y = yoff & ~15; y < ymax; y += 16)
   {
      unsigned long const *srcptr = (void *)((char *)src->image_data + y * pit);
      unsigned long *destptr = (unsigned long*)destrowptr;

      int ystart = yoff <= y ? 0 : yoff - y;
      int ycount = min(16 - ystart, ymax - y - ystart);

      vcos_assert(ystart >= 0 && ystart <= 15 && ycount >= 0 && ystart + ycount <= 16);

      for (x = xoff & ~63; x < xmax; x += 64)
      {
         load_tformat(srcptr + x, pit);
         destptr = store_raster(ycount, destptr, destpitch, xoff - x, x + 64 - xmax, ystart);
      }

      destrowptr += ycount * destpitch;
   }
}

static void expand_palette(void *destpal, void const *srcpal, int gltype, int size)
{
   switch (gltype)
   {
   case VC_IMAGE_OPENGL_PALETTE4_RGB24:
   case VC_IMAGE_OPENGL_PALETTE4_RGBA32:
   case VC_IMAGE_OPENGL_PALETTE4_RGB565:
   case VC_IMAGE_OPENGL_PALETTE4_RGBA16:
   case VC_IMAGE_OPENGL_PALETTE4_RGB5551:
      vcos_assert(size == 16);
      break;
   case VC_IMAGE_OPENGL_PALETTE8_RGB24:
   case VC_IMAGE_OPENGL_PALETTE8_RGBA32:
   case VC_IMAGE_OPENGL_PALETTE8_RGB565:
   case VC_IMAGE_OPENGL_PALETTE8_RGBA16:
   case VC_IMAGE_OPENGL_PALETTE8_RGB5551:
      vcos_assert(size == 256);
      break;
   default:
      vcos_assert(!"Image is not a paletted type");
   }

   unsigned char const *src = (unsigned char const*)srcpal;
   unsigned short const *src2 = (unsigned short const*)srcpal;
   unsigned char *dst       = (unsigned char*)destpal;
   int i;
   //unsigned char c;
   unsigned int c2;

   switch (gltype)
   {
   case VC_IMAGE_OPENGL_PALETTE4_RGB24:
   case VC_IMAGE_OPENGL_PALETTE8_RGB24:
      for (i = 0; i < size; i++)
      {
         *(dst++) = *(src++);   //r
         *(dst++) = *(src++);   //g
         *(dst++) = *(src++);   //b
         *(dst++) = 255;        //a
      }
      break;
   case VC_IMAGE_OPENGL_PALETTE4_RGBA32:
   case VC_IMAGE_OPENGL_PALETTE8_RGBA32:
      memcpy(destpal, srcpal, 4*size);
      break;
   case VC_IMAGE_OPENGL_PALETTE4_RGB565:
   case VC_IMAGE_OPENGL_PALETTE8_RGB565:
      for (i = 0; i < size; i++)
      {
         c2 = *(src2++);
         *(dst++) = ( (c2 >> 11)       *255)/31; //r
         *(dst++) = (((c2 >> 5) & 0x3f)*255)/63; //g
         *(dst++) = ( (c2 & 0x1f)      *255)/31; //b
         *(dst++) = 255;              //a
      }
      break;
   case VC_IMAGE_OPENGL_PALETTE4_RGBA16:
   case VC_IMAGE_OPENGL_PALETTE8_RGBA16:
      for (i = 0; i < size; i++)
      {
         c2 = *(src2++);
         *(dst++) = 17 * (c2 >> 12);       //r
         *(dst++) = 17 * ((c2>>8) & 0xf);  //g
         *(dst++) = 17 * ((c2>>4) & 0xf);  //b
         *(dst++) = 17 * (c2 & 0xf);       //a
      }
      break;
   case VC_IMAGE_OPENGL_PALETTE4_RGB5551:
   case VC_IMAGE_OPENGL_PALETTE8_RGB5551:
      for (i = 0; i < size; i++)
      {
         c2 = *(src2++);
         *(dst++) = (((c2 >> 11)& 0x1f)*255)/31; //r
         *(dst++) = (((c2 >> 6) & 0x1f)*255)/31; //g
         *(dst++) = (((c2 >> 1) & 0x1f)*255)/31; //b
         *(dst++) = (c2 & 1) * 255;              //a
      }
      break;
   default:
      vcos_assert(!"Image is not a paletted type");
   }
}

static void* malloc_palette(void const *srcpal, int gltype, int size)
{
   void *result = malloc(4 * size);
   expand_palette(result, srcpal, gltype, size);
   return result;
}

static char* tformat_ptr(VC_IMAGE_T *img, int x, int y, int *dir0, int *dir1)
{
   int d0=0, d1=0;
   char *ptr = NULL;
   switch (y & 48)
   {
   case 0:
      ptr = (char*)img->image_data + y * img->pitch;
      d0 = 3072;
      d1 = 1024;
      break;
   case 16:
      ptr = (char*)img->image_data + (y - 16) * img->pitch + 1024;
      d0 = 1024;
      d1 = 3072;
      break;
   case 32:
      ptr = (char*)img->image_data + (y + 32) * img->pitch - 2048;
      d0 = -1024;
      d1 = -3072;
      break;
   case 48:
      ptr = (char*)img->image_data + (y + 16) * img->pitch - 1024;
      d0 = -3072;
      d1 = -1024;
      break;
   default:
      vcos_assert(!"nonsense y offset");
      return NULL;
   }
   ptr += (d0 + d1) * (x >> 5);
   if (x & 16)
   {
      ptr += d0;
      d0 ^= d1;
      d1 ^= d0;
      d0 ^= d1;
   }
   *dir0 = d0;
   *dir1 = d1;
   return ptr;
}

static void vc_image_tf_to_tf(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height, VC_IMAGE_T const *src, int src_x_offset, int src_y_offset)
{
   int y;
   int ymin = ((y_offset-1)|15)+1;
   int ymax = (y_offset+height)&~15;
   int xmin = ((x_offset-1)|15)+1;
   int xmax = (x_offset+width)&~15;

   //XXX currently can't deal with nonaligned destination rectangle
   vcos_assert((x_offset&15) == 0 && (width&15) == 0);
   vcos_assert((y_offset&15) == 0 && (height&15) == 0);

   for (y = ymin; y < ymax; y+=16)
   {
      int destdir0, destdir1, bdir0, bdir1, tdir0, tdir1;
      char *destptr = tformat_ptr(dest, xmin, y, &destdir0, &destdir1);
      int x2 = (xmin - x_offset + src_x_offset);
      int y2 = (y - y_offset + src_y_offset);
      const char *bptr = tformat_ptr((VC_IMAGE_T*)src, x2 & ~15, y2 & ~15, &bdir0, &bdir1);
      const char *tptr = tformat_ptr((VC_IMAGE_T*)src, x2 & ~15, (y2 & ~15)+16, &tdir0, &tdir1);

      tformat_blit_row(
         destptr, destdir0, destdir1,
         bptr, bptr + bdir0, bdir0 + bdir1,
         tptr, tptr + tdir0, tdir0 + tdir1,
         x2 & 15, y2 & 15, (xmax-xmin)>>4);
   }
}


/******************************************************************************
NAME
   vc_image_rgba32_to_tf_rgba32_part

SYNOPSIS
   void vc_image_rgba32_to_tf_rgba32_part(VC_IMAGE_T *dest,
         int x_off, int y_off, int width, int height,
         VC_IMAGE_T const *src, int src_x_off, int src_y_off)

FUNCTION
   x
******************************************************************************/

void vc_image_rgba32_to_tf_rgba32_part(VC_IMAGE_T *dest, int x_off, int y_off, int width, int height, VC_IMAGE_T const *src, int src_x_off, int src_y_off)
{
   char const *srcbase;
   int x, y, srcpitch;
   int w, h, pit = tformat_adjusted_pitch4(dest);
   LOAD_PROC4_T load_raster = tformat_linear_load_32bit_flip;
   VC_IMAGE_TRANSFORM_T transform = TRANSFORM_VFLIP; // normal case
   VC_IMAGE_T temp = *dest;
   vcos_assert(x_off == 0 && src_x_off == 0); // not supported
   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, dest->type) && is_valid_vc_image_buf(src, src->type));
   temp.width = width;
   temp.height = height;
   w = tformat_adjusted_width4(&temp);
   h = tformat_adjusted_height4(&temp);

   if (vc_image_get_mipmap_type(dest,0) == VC_IMAGE_MIPMAP_LINEAR_TILE)
   {
      vc_image_raster_to_ltile4(dest, src, transform, load_raster);
      return;
   }

   if (transform)
   {
      srcbase = (char *)src->image_data;
      srcpitch = src->pitch;
   }
   else
   {
      vcos_assert_msg((src->height & 15) == 0, "risk of illegal access at beginning of image");

      if (src->extra.opengl.format_and_type == VC_IMAGE_OPENGL_ETC1)
      {
         //XXX hack to get flipping ETC1 images correct
         srcbase = (char *)src->image_data + (src->height/4 - 1) * src->pitch;
      }
      else
      {
         srcbase = (char *)src->image_data + (src->height - 1) * src->pitch;
      }
      srcpitch = -src->pitch;
   }

   for (y = 0; y < h; y += 16)
   {
      unsigned long *destptr = (void *)((char *)dest->image_data + (((y + y_off) + 32) & ~63) * pit);
      unsigned long const *srcptr = (void *)(srcbase + (y + src_y_off) * srcpitch);
      int rowflip = ((y + y_off)& 32) ? -512 : 0;

      for (x = 0; x < w; x += 32)
      {
         int ofs = (x >> 4) * 512 + (((y + y_off) >> 4) & 1) * 256;

         srcptr = load_raster(srcptr, srcpitch);
         tformat_subtile_store_32bit(destptr + (ofs ^ rowflip));

         if (x + 16 < w)
         {
            ofs += 512;
            srcptr = load_raster(srcptr, srcpitch);
            tformat_subtile_store_32bit(destptr + (ofs ^ rowflip ^ 256));
         }
      }
   }
}

/******************************************************************************
NAME
   vc_image_tf_rgba32_to_rgba32_part

SYNOPSIS
   void vc_image_tf_rgb565_to_rgb565_part(VC_IMAGE_T *dest,
         int x_off, int y_off, int width, int height,
         VC_IMAGE_T const *src, int src_x_off, int src_y_off)

FUNCTION
   x
******************************************************************************/
void vc_image_tf_rgba32_to_rgba32_part(VC_IMAGE_T *dest, int x_off, int y_off, int width, int height, VC_IMAGE_T const *src, int src_x_off, int src_y_off)
{
   char *destbase;
   int x, y, destpitch;
   VC_IMAGE_TRANSFORM_T transform = TRANSFORM_VFLIP; // normal case

   vcos_assert(x_off == 0 && src_x_off == 0); // not supported
   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGBA32));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_TF_RGBA32));

   if (!vcos_verify(vc_image_get_mipmap_type(src,0) == VC_IMAGE_MIPMAP_TFORMAT))
      return;

   if (transform)
   {
      destbase = (char *)dest->image_data;
      destpitch = dest->pitch;
   }
   else
   {
      vcos_assert_msg((dest->height & 15) == 0, "risk of illegal access at beginning of image");

      destbase = (char *)dest->image_data + (dest->height - 1) * dest->pitch;
      destpitch = -dest->pitch;
   }

   for (y = 0; y < height; y += 16)
   {
      unsigned long const *srcptr = (void *)((char *)src->image_data + (((y + src_y_off) + 32) & ~63) * src->pitch);
      unsigned long *destptr = (void *)(destbase + (y + y_off) * destpitch);
      int rowflip = ((y + src_y_off) & 32) ? -512 : 0;

      for (x = 0; x < width; x += 32)
      {
         int ofs = (x >> 4) * 512 + (((y + src_y_off) >> 4) & 1) * 256;

         tformat_subtile_load_32bit(srcptr + (ofs ^ rowflip));
#if !defined(RGB888)
         tformat_red_blue_swap();
#endif
         tformat_linear_store_32bit_flip(destptr, destpitch);
         destptr += 16;

         if (x + 16 < dest->width)
         {
            ofs += 512;
            tformat_subtile_load_32bit(srcptr + (ofs ^ rowflip ^ 256));
#if !defined(RGB888)
            tformat_red_blue_swap();
#endif
            tformat_linear_store_32bit_flip(destptr, destpitch);
         }
         destptr += 16;
      }
   }
   if ((height & 15) != 0)
      vclib_memmove(dest->image_data, (char *)dest->image_data + dest->pitch * (16 - (height & 15)), dest->pitch * height);
}

void vc_image_tf_rgb565_to_rgb565_part(VC_IMAGE_T *dest, int x_off, int y_off, int width, int height, VC_IMAGE_T const *src, int src_x_off, int src_y_off)
{
   char *destbase;
   int x, y, destpitch;
   VC_IMAGE_TRANSFORM_T transform = TRANSFORM_VFLIP; // normal case

   vcos_assert(x_off == 0 && src_x_off == 0); // not supported
   vcos_assert(transform == 0 || transform == TRANSFORM_VFLIP);
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_RGB565));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_TF_RGB565));

   if (!vcos_verify(vc_image_get_mipmap_type(src,0) == VC_IMAGE_MIPMAP_TFORMAT))
      return;

   if (transform)
   {
      destbase = (char *)dest->image_data;
      destpitch = dest->pitch;
   }
   else
   {
      vcos_assert_msg((dest->height & 15) == 0, "risk of illegal access at beginning of image");

      destbase = (char *)dest->image_data + (dest->height - 1) * dest->pitch;
      destpitch = -dest->pitch;
   }

   for (y = 0; y < height; y += 16)
   {
      unsigned short const *srcptr = (void *)((char *)src->image_data + (((y + src_y_off) + 32) & ~63) * src->pitch);
      unsigned short *destptr = (void *)(destbase + (y + y_off) * destpitch);
      int rowflip = ((y + src_y_off) & 32) ? -512 : 0;

      for (x = 0; x < width; x += 64)
      {
         int ofs = (x >> 5) * 512 + (((y + src_y_off) >> 4) & 1) * 256;

         tformat_subtile_load_32bit((unsigned long *)(srcptr + (ofs ^ rowflip) * 2));
         if (x + 16 < width)
            tformat_linear_store_32bit_flip((unsigned long *)destptr, destpitch);
         else
            tformat_linear_store_16bit_flip((unsigned long *)destptr, destpitch);
         destptr += 32;

         if (x + 32 < width)
         {
            ofs += 512;
            tformat_subtile_load_32bit((unsigned long *)(srcptr + (ofs ^ rowflip ^ 256) * 2));
            if (x + 48 < width)
               tformat_linear_store_32bit_flip((unsigned long *)destptr, destpitch);
            else
               tformat_linear_store_16bit_flip((unsigned long *)destptr, destpitch);
         }
         destptr += 32;
      }
   }
   if (!vcos_verify((height & 15) == 0))
   {
      /* TODO: untested code path */
      vclib_memmove(dest->image_data, (char *)dest->image_data + dest->pitch * (16 - (dest->height & 15)), dest->pitch * dest->height);
   }
}

void vc_image_rgb565_to_tf_rgb565_part(VC_IMAGE_T *dest, int x_off, int y_off, int width, int height, VC_IMAGE_T const *src, int src_x_off, int src_y_off)
{
   vc_image_rgba32_to_tf_rgba32_part(dest, x_off, y_off, width, height, src, src_x_off, src_y_off);
}

static unsigned inline int tformat_utile_addr
   (int ut_w, int ut_x, int ut_y)
{
   unsigned int wt, xt, yt, xst, yst;
   unsigned int tile_addr, sub_tile_addr;
   int line_odd = 0;

   // first check that ut_w and ut_h conform to
   // exact multiples of tiles
   if(ut_w&0x7)
      vcos_assert(!"ut_w not padded to integer # tiles!");

   // find width and height of image in tiles
   wt = ut_w>>3;

   // find address of tile given ut_x,ut_y
   // remembering tiles are 8x8 u-tiles,
   // also remembering scan direction changes
   // for odd and even lines
   xt = ut_x>>3; yt = ut_y>>3;
   if(yt&0x1) {
      xt = wt-xt-1; // if odd line, reverse direction
      line_odd = 1;
   }
   tile_addr = yt*wt + xt;

   // now find address of sub-tile within tile (2 bits)
   // remembering scan order is not raster scan, and
   // changes direction given odd/even tile-scan-line.
   xst = (ut_x>>2)&0x1;
   yst = (ut_y>>2)&0x1;
   switch(xst+(yst<<1)){
    case 0:  sub_tile_addr = line_odd ? 2 : 0; break;
    case 1:  sub_tile_addr = line_odd ? 1 : 3; break;
    case 2:  sub_tile_addr = line_odd ? 3 : 1; break;
    default: sub_tile_addr = line_odd ? 0 : 2; break;
   }

   return (sub_tile_addr<<10) + (tile_addr<<12);
}

void vc_image_transpose_tf_rgba32(VC_IMAGE_T *dst, VC_IMAGE_T const *src)
{
   int x, y;

   for (y = 0; y < src->height; y += 16)
      for (x = 0; x < src->width; x += 16) {
         int src_offset = tformat_utile_addr(src->pitch >> 4, x >> 2, y >> 2);
         int dst_offset = tformat_utile_addr(dst->pitch >> 4, y >> 2, x >> 2);

         tformat_subtile_transpose_32bit((uint8_t *)dst->image_data + dst_offset, (uint8_t *)src->image_data + src_offset);
      }
}

void vc_image_transpose_tf_rgb565(VC_IMAGE_T *dst, VC_IMAGE_T const *src)
{
   int x, y;

   for (y = 0; y < src->height; y += 32) {
      for (x = 0; x < src->width; x += 32) {
         int src_offset_a = tformat_utile_addr(src->pitch >> 4, x >> 3, y >> 2);
         int src_offset_b = tformat_utile_addr(src->pitch >> 4, x >> 3, (y >> 2) + 4);

         int dst_offset_a = tformat_utile_addr(dst->pitch >> 4, y >> 3, x >> 2);
         int dst_offset_b = tformat_utile_addr(dst->pitch >> 4, y >> 3, (x >> 2) + 4);

         tformat_subtile_pair_transpose_16bit((uint8_t *)dst->image_data + dst_offset_a, (uint8_t *)dst->image_data + dst_offset_b,
                                              (uint8_t *)src->image_data + src_offset_a, (uint8_t *)src->image_data + src_offset_b,
                                              x < src->width - 16);
      }
   }
}
