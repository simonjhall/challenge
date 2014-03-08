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
#include <limits.h>

/* Project includes */
#include "vcinclude/common.h"
#include "interface/vcos/vcos_assert.h"
#include "helpers/vc_image/vc_image.h"

static unsigned int tformat_utile_height(VC_IMAGE_TYPE_T type);
static int calculate_mipmap_type(VC_IMAGE_TYPE_T type, int width, int height, int mipmap_levels, int miplvl);
static int vc_image_helper_get_sdram_page_size( void );

#if defined(_MSC_VER) || defined(__CC_ARM)
int _msb(unsigned int x)
{
   int i, res = -1;
   for (i=0; i<CHAR_BIT*sizeof(x); i++)
      if (x & (1<<i)) res = i;
   return res;
}

int _count(unsigned int x)
{
   int i, res = 0;
   for (i=0; i<CHAR_BIT*sizeof(x); i++)
      if (x & (1<<i)) res++;
   return res;
}

#define max(a, b) ((a)>(b)?(a):(b))
#define rtos_memory_is_valid(b, l) 1
#else
   #include "vcinclude/vcore.h"
   #include "vcfw/rtos/rtos.h"
#endif

const VC_IMAGE_TYPE_INFO_T vc_image_type_info[VC_IMAGE_MAX + 1] =
{
   /* NOTE: We define is_raster_order to be any format where height and width
    * are rounded to multiples of 16, and a whole pixel can be read by the
    * usual (y * pitch + x) * bpp >> 3 calculation. This means that we can test
    * an unknown format to see if we know the pixel-getting equation separately
    * to worrying about what that pixel is.
    */
   /*[VC_IMAGE_MIN]           = */ { 0 },
   /*                                  bits_per_pixel
                                       |  is_rgb
                                       |  |  is_yuv
                                       |  |  |  is_raster_order
                                       |  |  |  |  is_tformat_order
                                       |  |  |  |  |  has_alpha
                                       |  |  |  |  |  |*/
   /*[VC_IMAGE_RGB565]        = */ {  16, 1, 0, 1, 0, 0 },
   /*[VC_IMAGE_1BPP]          = */ {   1, 0, 0, 1, 0, 0 },
   /*[VC_IMAGE_YUV420]        = */ {   8, 0, 1, 0, 0, 0 }, /* XXX: questionable... */
   /*[VC_IMAGE_48BPP]         = */ {  48, 0, 0, 1, 0, 0 },
   /*[VC_IMAGE_RGB888]        = */ {  24, 1, 0, 1, 0, 0 },
   /*[VC_IMAGE_8BPP]          = */ {   8, 0, 0, 1, 0, 0 }, /* XXX: alpha depends on palette */
   /*[VC_IMAGE_4BPP]          = */ {   4, 0, 0, 1, 0, 0 }, /* XXX: alpha depends on palette */
   /*[VC_IMAGE_3D32]          = */ {  32, 0, 0, 0, 0, 0 }, /* XXX: obsolete? */
   /*[VC_IMAGE_3D32B]         = */ {  32, 0, 0, 0, 0, 0 }, /* XXX: obsolete? */
   /*[VC_IMAGE_3D32MAT]       = */ {  32, 0, 0, 0, 0, 0 },
   /*[VC_IMAGE_RGB2X9]        = */ {  32, 1, 0, 0, 0, 0 },
   /*[VC_IMAGE_RGB666]        = */ {  32, 1, 0, 0, 0, 0 },
   /*[VC_IMAGE_PAL4_OBSOLETE] = */ {   4, 0, 0, 1, 0, 0 },
   /*[VC_IMAGE_PAL8_OBSOLETE] = */ {   8, 0, 0, 1, 0, 0 },
   /*[VC_IMAGE_RGBA32]        = */ {  32, 1, 0, 1, 0, 1 },
   /*[VC_IMAGE_YUV422]        = */ {   8, 0, 1, 0, 0, 0 }, /* XXX: questionable... */
   /*[VC_IMAGE_RGBA565]       = */ {  16, 1, 0, 1, 0, 1 }, /* XXX: alpha yuck! */
   /*[VC_IMAGE_RGBA16]        = */ {  16, 1, 0, 1, 0, 1 },
   /*[VC_IMAGE_YUV_UV]        = */ {   8, 0, 1, 0, 0, 0 }, /* XXX: questionable... */
   /*[VC_IMAGE_TF_RGBA32]     = */ {  32, 1, 0, 0, 1, 1 },
   /*[VC_IMAGE_TF_RGBX32]     = */ {  32, 1, 0, 0, 1, 0 },
   /*[VC_IMAGE_TF_FLOAT]      = */ {  32, 0, 0, 0, 1, 0 },
   /*[VC_IMAGE_TF_RGBA16]     = */ {  16, 1, 0, 0, 1, 1 },
   /*[VC_IMAGE_TF_RGBA5551]   = */ {  16, 1, 0, 0, 1, 1 },
   /*[VC_IMAGE_TF_RGB565]     = */ {  16, 1, 0, 0, 1, 0 },
   /*[VC_IMAGE_TF_YA88]       = */ {  16, 0, 0, 0, 1, 1 },
   /*[VC_IMAGE_TF_BYTE]       = */ {   8, 0, 0, 0, 1, 0 },
   /*[VC_IMAGE_TF_PAL8]       = */ {   8, 0, 0, 0, 1, 0 }, /* XXX: alpha depends on palette */
   /*[VC_IMAGE_TF_PAL4]       = */ {   4, 0, 0, 0, 1, 0 }, /* XXX: alpha depends on palette */
   /*[VC_IMAGE_TF_ETC1]       = */ {   4, 0, 0, 0, 1, 0 },
   /*[VC_IMAGE_BGR888]        = */ {  24, 1, 0, 1, 0, 0 },
   /*[VC_IMAGE_BGR888_NP]     = */ {  24, 1, 0, 0, 0, 0 },
   /*[VC_IMAGE_BAYER]         = */ {  -1, 0, 0, 0, 0, 0 }, /* XXX: questionable... */
   /*[VC_IMAGE_CODEC]         = */ {  -1, 0, 0, 0, 0, 0 }, /* XXX: questionable... */
   /*[VC_IMAGE_YUV_UV32]      = */ {   8, 0, 1, 0, 0, 0 }, /* XXX: questionable... */
   /*[VC_IMAGE_TF_Y8]         = */ {   8, 0, 0, 0, 1, 0 },
   /*[VC_IMAGE_TF_A8]         = */ {   8, 0, 0, 0, 1, 1 }, /* XXX: has alpha? is alpha! */
   /*[VC_IMAGE_TF_SHORT]      = */ {  16, 0, 0, 0, 1, 0 },
   /*[VC_IMAGE_TF_1BPP]       = */ {   1, 0, 0, 0, 1, 0 },
   /*[VC_IMAGE_OPENGL]        = */ {  -1, 1, 0, 1, 0, 0 }, /* XXX: questionable... */
   /*[VC_IMAGE_YUV444I]       = */ {  24, 0, 1, 0, 0, 0 }, /* Needs confirming */
   /*[VC_IMAGE_YUV422_PLANAR] = */ {   8, 0, 1, 0, 0, 0 }, /* XXX: questionable... */
   /*[VC_IMAGE_ARGB8888]      = */ {  32, 1, 0, 1, 0, 1 },
   /*[VC_IMAGE_XRGB8888]      = */ {  32, 1, 0, 1, 0, 0 },
   /*[VC_IMAGE_YUV422YUYV]    = */ {   8, 0, 1, 0, 0, 0 }, /* XXX: questionable... */
   /*[VC_IMAGE_YUV422YVYU]    = */ {   8, 0, 1, 0, 0, 0 }, /* XXX: questionable... */
   /*[VC_IMAGE_YUV422UYVY]    = */ {   8, 0, 1, 0, 0, 0 }, /* XXX: questionable... */
   /*[VC_IMAGE_YUV422VYUY]    = */ {   8, 0, 1, 0, 0, 0 }, /* XXX: questionable... */

   /*[VC_IMAGE_RGBX32]        = */ {  32, 1, 0, 1, 0, 0 },


   /*[VC_IMAGE_MAX            = */ { 0 }
};

const unsigned int vc_image_rgb_component_order[VC_IMAGE_MAX + 1] =
{
   /* Specify the bit position of the LSB of each component relative to a
    * LITTLE ENDIAN WORD of the appropriate size stored at the same physical
    * address.
    */
   /*[VC_IMAGE_MIN]           = */ 0,
   /*                                                     alpha lsb offset
                                                       blue lsb offset   |
                                                  green lsb offset   |   |
                                                red lsb offset   |   |   |
                                                             |   |   |   |*/
   /*[VC_IMAGE_RGB565]        = */ VC_IMAGE_COMPONENT_ORDER(11,  5,  0,  0),
   /*[VC_IMAGE_1BPP]          = */ 0,
   /*[VC_IMAGE_YUV420]        = */ 0,
   /*[VC_IMAGE_48BPP]         = */ 0,
#if defined(RGB888)
   /*[VC_IMAGE_RGB888]        = */ VC_IMAGE_COMPONENT_ORDER( 0,  8, 16,  0),
#else
   /*[VC_IMAGE_RGB888]        = */ VC_IMAGE_COMPONENT_ORDER(16,  8,  0,  0),
#endif
   /*[VC_IMAGE_8BPP]          = */ 0,
   /*[VC_IMAGE_4BPP]          = */ 0, /* XXX: Don't know */
   /*[VC_IMAGE_3D32]          = */ 0,
   /*[VC_IMAGE_3D32B]         = */ 0,
   /*[VC_IMAGE_3D32MAT]       = */ 0,
   /*[VC_IMAGE_RGB2X9]        = */ 0,
   /*[VC_IMAGE_RGB666]        = */ VC_IMAGE_COMPONENT_ORDER(12,  6,  0, 18),
   /*[VC_IMAGE_PAL4_OBSOLETE] = */ 0,
   /*[VC_IMAGE_PAL8_OBSOLETE] = */ 0,
#if defined(RGB888)
   /*[VC_IMAGE_RGBA32]        = */ VC_IMAGE_COMPONENT_ORDER( 0,  8, 16, 24),
#else
   /*[VC_IMAGE_RGBA32]        = */ VC_IMAGE_COMPONENT_ORDER(16,  8,  0, 24),
#endif
   /*[VC_IMAGE_YUV422]        = */ 0,
   /*[VC_IMAGE_RGBA565]       = */ VC_IMAGE_COMPONENT_ORDER(11,  5,  0,  0),
   /*[VC_IMAGE_RGBA16]        = */ VC_IMAGE_COMPONENT_ORDER(12,  8,  4,  0),
   /*[VC_IMAGE_YUV_UV]        = */ 0,
   /*[VC_IMAGE_TF_RGBA32]     = */ VC_IMAGE_COMPONENT_ORDER( 0,  8, 16, 24),
   /*[VC_IMAGE_TF_RGBX32]     = */ VC_IMAGE_COMPONENT_ORDER( 0,  8, 16, 24),
   /*[VC_IMAGE_TF_FLOAT]      = */ 0,
   /*[VC_IMAGE_TF_RGBA16]     = */ VC_IMAGE_COMPONENT_ORDER(12,  8,  4,  0),
   /*[VC_IMAGE_TF_RGBA5551]   = */ VC_IMAGE_COMPONENT_ORDER(11,  6,  1,  0),
   /*[VC_IMAGE_TF_RGB565]     = */ VC_IMAGE_COMPONENT_ORDER(11,  5,  0,  0),
   /*[VC_IMAGE_TF_YA88]       = */ 0,
   /*[VC_IMAGE_TF_BYTE]       = */ 0,
   /*[VC_IMAGE_TF_PAL8]       = */ 0,
   /*[VC_IMAGE_TF_PAL4]       = */ 0,
   /*[VC_IMAGE_TF_ETC1]       = */ 0,
   /*[VC_IMAGE_BGR888]        = */ 0, /* XXX: Don't know */
   /*[VC_IMAGE_BGR888_NP]     = */ 0, /* XXX: Don't know */
   /*[VC_IMAGE_BAYER]         = */ 0,
   /*[VC_IMAGE_CODEC]         = */ 0,
   /*[VC_IMAGE_YUV_UV32]      = */ 0,
   /*[VC_IMAGE_TF_Y8]         = */ 0,
   /*[VC_IMAGE_TF_A8]         = */ 0,
   /*[VC_IMAGE_TF_SHORT]      = */ 0,
   /*[VC_IMAGE_TF_1BPP]       = */ 0,
   /*[VC_IMAGE_OPENGL]        = */ 0,
   /*[VC_IMAGE_YUV444I]       = */ 0,
   /*[VC_IMAGE_YUV422_PLANAR] = */ 0,
   /*[VC_IMAGE_ARGB8888]      = */ 0, /* XXX: Don't know */
   /*[VC_IMAGE_XRGB8888]      = */ 0, /* XXX: Don't know */
   /*[VC_IMAGE_YUV422YUYV]    = */ 0, /* XXX: Don't know */
   /*[VC_IMAGE_YUV422YVYU]    = */ 0, /* XXX: Don't know */
   /*[VC_IMAGE_YUV422UYVY]    = */ 0, /* XXX: Don't know */
   /*[VC_IMAGE_YUV422VYUY]    = */ 0, /* XXX: Don't know */

#if defined(RGB888)
   /*[VC_IMAGE_RGBX32]        = */ VC_IMAGE_COMPONENT_ORDER( 0,  8, 16, 24),
#else
   /*[VC_IMAGE_RGBX32]        = */ VC_IMAGE_COMPONENT_ORDER(16,  8,  0, 24),
#endif

   /*[VC_IMAGE_MAX            = */ 0
};


/******************************************************************************
Global function definitions.
******************************************************************************/


/******************************************************************************
NAME
   fix_alignment

PARAMS
   long fix_alignment(VC_IMAGE_TYPE_T type, long value)

FUNCTION
   Ensure required alignment

RETURNS
   Required alignment for format
******************************************************************************/
long fix_alignment(VC_IMAGE_TYPE_T type, long value)
{
   switch (type)
   {
case_VC_IMAGE_ANY_TFORMAT:
      /* at least 128 bytes for 3D FBC widths in fat mode */
      /* at least 1024 bytes for HVS 2D SDRAM access trick */
      /* TODO: identify the 4096 byte requirement */
      return value + 4095 & ~4095;

   case VC_IMAGE_YUV_UV:
   case VC_IMAGE_YUV_UV32:
      /* 4096 bytes to suit a short-cut in the hardware video decode block */
      return value + 4095 & ~4095;
   }

   return value + 31 & ~31;
}

/******************************************************************************
NAME
   vc_image_bits_per_pixel

SYNOPSIS
   void vc_image_bits_per_pixel(VC_IMAGE_TYPE_T type)

FUNCTION
   Return the number of bits per pixel for a given type.

RETURNS
   -
******************************************************************************/
int vc_image_bits_per_pixel (VC_IMAGE_TYPE_T type) {
   int ret = 0;
   switch (type) {
   case VC_IMAGE_1BPP:
   case VC_IMAGE_TF_1BPP:
      ret = 1;
      break;
   case VC_IMAGE_4BPP:
   case VC_IMAGE_TF_PAL4:
   case VC_IMAGE_TF_ETC1:
      ret = 4;
      break;
   case VC_IMAGE_YUV420: /* questionable... */
   case VC_IMAGE_YUV422: /* questionable... */
   case VC_IMAGE_YUV422PLANAR: /* questionable... */
   case VC_IMAGE_YUV_UV: /* questionable... */
   case VC_IMAGE_YUV_UV32: /* questionable... */
   case VC_IMAGE_8BPP:
   case VC_IMAGE_TF_BYTE:
   case VC_IMAGE_TF_PAL8:
   case VC_IMAGE_TF_Y8:
   case VC_IMAGE_TF_A8:
      ret = 8;
      break;
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
   case VC_IMAGE_TF_RGBA16:
   case VC_IMAGE_TF_RGBA5551:
   case VC_IMAGE_TF_RGB565:
   case VC_IMAGE_TF_YA88:
   case VC_IMAGE_TF_SHORT:
      ret = 16;
      break;
   case VC_IMAGE_RGB888:
      ret = 24;
      break;
   case VC_IMAGE_3D32:
   case VC_IMAGE_3D32B:
   case VC_IMAGE_3D32MAT:
   case VC_IMAGE_RGBA32:
   case VC_IMAGE_RGBX32:
   case VC_IMAGE_ARGB8888:
   case VC_IMAGE_XRGB8888:
   case VC_IMAGE_TF_RGBA32:
   case VC_IMAGE_TF_RGBX32:
   case VC_IMAGE_TF_FLOAT:
      ret = 32;
      break;
   default:
      vcos_assert(!"unsupported format");
   }
   vcos_assert(ret == VC_IMAGE_BITS_PER_PIXEL(type));
   return ret;
}

/******************************************************************************
NAME
   vc_image_set_type

SYNOPSIS
   void vc_image_set_type(VC_IMAGE_T *image)

FUNCTION
   Set the type of a VC_IMAGE_T, noting the correct bits_per_pixel.

RETURNS
   -
******************************************************************************/

void vc_image_set_type (VC_IMAGE_T *image, VC_IMAGE_TYPE_T type) {
   vcos_assert(image);
   image->type = type;
}

/******************************************************************************
NAME
   vc_image_set_image_data

SYNOPSIS
   void vc_image_set_image_data(VC_IMAGE_BUF_T *image, int size, void *image_data)

FUNCTION
   Set the image_data field of a VC_IMAGE_BUF_T, noting the size of the memory block.

RETURNS
   -
******************************************************************************/
void vc_image_set_image_data (VC_IMAGE_BUF_T *image, int size, void *image_data) {
   vcos_assert(image);

   if (VC_IMAGE_IS_YUV(image->type))
      vc_image_set_image_data_yuv(image, size, image_data, NULL, NULL);
   else
   {
      vcos_assert(size == 0 || size >= vc_image_required_size(image));
      image->image_data = image_data;
      image->size = size;
   }

   vcos_assert(image->image_data == NULL || rtos_memory_is_valid(image->image_data, image->size));
}

/******************************************************************************
NAME
   vc_image_set_image_data_yuv

SYNOPSIS
   void vc_image_set_image_data(VC_IMAGE_BUF_T *image, int size, void *image_y, void *image_u, void *image_v)

FUNCTION
   Set the image_data and uv fields of a VC_IMAGE_BUF_T, noting the size of the memory block.
   Setting image_u and image_v to NULL will set them to the default positions of
   directly after the previous component plane.

RETURNS
   -
******************************************************************************/

void vc_image_set_image_data_yuv (VC_IMAGE_BUF_T *image, int size, void *image_y, void *image_u, void *image_v) {
   int offset_u = -1, offset_v = -1;
   int round_height;
   vcos_assert(image);

   round_height = image->height + 15 & ~15;
   switch (image->type)
   {
   case VC_IMAGE_YUV420:
      offset_u = image->pitch * round_height;
      offset_v = offset_u + (image->pitch * round_height >> 2);
      break;

   case VC_IMAGE_YUV422:
      offset_u = image->pitch >> 1;
      offset_v = image->pitch * 3 >> 2;
      break;

   case VC_IMAGE_YUV422YUYV:
      offset_u = -1;
      offset_v = -1;
      break;

   case VC_IMAGE_YUV422PLANAR:
      offset_u = image->pitch * round_height;
      offset_v = offset_u + (image->pitch * round_height >> 1);
      break;

   case VC_IMAGE_YUV_UV:
      offset_u = round_height * VC_IMAGE_YUV_UV_STRIPE_WIDTH;
      offset_v = -1;
      break;
   case VC_IMAGE_YUV_UV32:
      offset_u = round_height * VC_IMAGE_YUV_UV32_STRIPE_WIDTH;
      offset_v = -1;
      break;

   default:
      vcos_assert(!"Image type isn't YUV.");
   }

   vcos_assert(size == 0 || size >= vc_image_required_size(image));
   image->size = size;
   image->image_data = image_y;

   if (image_y != NULL)
   {
      image->extra.uv.u = image_u ? image_u : offset_u == -1 ? NULL : (char *)image_y + fix_alignment((VC_IMAGE_TYPE_T)image->type, offset_u);
      image->extra.uv.v = image_v ? image_v : offset_v == -1 ? NULL : (char *)image_y + fix_alignment((VC_IMAGE_TYPE_T)image->type, offset_v);
   }
   else
   {
      image->extra.uv.u = NULL;
      image->extra.uv.v = NULL;
   }
}


/******************************************************************************
NAME
   vc_image_set_dimensions

SYNOPSIS
   void vc_image_set_dimensions(VC_IMAGE_T *image, int width, int height)

FUNCTION
   Set the dimensions of an image. Remember that the pitch also needs
   setting (vc_image_set_pitch).

RETURNS
   -
******************************************************************************/

void vc_image_set_dimensions (VC_IMAGE_T *image, int width, int height) {
   vcos_assert(image);
   image->width = width;
   image->height = height;
}


/******************************************************************************
NAME
   vc_image_verify

SYNOPSIS
   int vc_image_verify(const VC_IMAGE_T *image)

FUNCTION
   Check that a VC_IMAGE_T is valid

RETURNS
   int : 0 or above is success
******************************************************************************/
int vc_image_verify(const VC_IMAGE_T *image)
{
   int success = -1; //fail by default

   if ( NULL != image)
   {
      if ( (image->type > VC_IMAGE_MIN) && (image->type < VC_IMAGE_MAX) )
      {
         if ( (0 != image->width) && (0 != image->height) && (0 != image->pitch) && (0 != image->size) )
         {
            if ( image->type == VC_IMAGE_YUV420 )
            {
               if ( NULL != image->image_data && NULL != image->extra.uv.u && NULL != image->extra.uv.v)
               {
                  //success!
                  success = 0;
               }
            }
            else if ( NULL != image->image_data)
            {
               //success!
               success = 0;
            }
         }
      }
   }

   return success;
}

/******************************************************************************
NAME
   vc_image_set_pitch

SYNOPSIS
   void vc_image_set_pitch(VC_IMAGE_T *image, int pitch)

FUNCTION
   Set the pitch of an image. The image type field must be set, as must the width.
   0 can be passed as the pitch for the routine to calculate the default smallest
   pitch for the stored width.

RETURNS
   -
******************************************************************************/

int calculate_pitch(VC_IMAGE_TYPE_T type, int width, int height, VC_IMAGE_EXTRA_T *extra)
{
   vcos_assert(width > 0);

// pitch depends on image type and width.
   switch (type) {
   case VC_IMAGE_1BPP:
      return ((width + 255)&(~255))>>3;
   case VC_IMAGE_8BPP:
      return (width+31)&~31;
   case VC_IMAGE_YUV420:
      return (width + 31)&(~31);
   case VC_IMAGE_YUV422:
      return ((width + 31)&(~31))*2;
   case VC_IMAGE_YUV422PLANAR:
      return ((width + 31)&(~31));
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      return ((width + 15)&(~15))<<1;
   case VC_IMAGE_BGR888_NP:
      return width*3;   //BGR with no pitch
   case VC_IMAGE_BGR888:
   case VC_IMAGE_RGB888:
      return ((width + 15)&(~15))*3;
   case VC_IMAGE_RGBA32:
   case VC_IMAGE_RGBX32:
   case VC_IMAGE_ARGB8888:
   case VC_IMAGE_XRGB8888:
      return ((width + 15)&(~15))*4;
   case VC_IMAGE_3D32:
   case VC_IMAGE_3D32MAT:
   case VC_IMAGE_3D32B:
   case VC_IMAGE_RGB2X9:
   case VC_IMAGE_RGB666:
      return ((width + 15)&(~15))*4;
   case VC_IMAGE_48BPP:
      return ((width + 15)&(~15))*6;
   case VC_IMAGE_4BPP:
      return ((width+31)&~31)>>1;
   case VC_IMAGE_YUV_UV:
   {
      int pitch, align;

      /* first calculate required space for a Y stripe, and then pad it to align the UV data */
      pitch = fix_alignment(type, ((height + 15 & ~15) * VC_IMAGE_YUV_UV_STRIPE_WIDTH));
      /* then add the UV stripe space and re-align */
      pitch = fix_alignment(type, pitch * 3 >> 1);

      /* If an SDRAM page pair is large enough to stay suitably aligned ... */
      align = vc_image_helper_get_sdram_page_size() << 1;
      if (align == fix_alignment(type, align))
      {
         /* ... then optimise the pitch to stagger alternate stripes between page pairs */
         pitch = pitch + (align - 1) & ~(align - 1);
         pitch |= align;
      }
      return fix_alignment(type, pitch);
   }
   case VC_IMAGE_YUV_UV32:
   {
      int pitch, align;

      /* first calculate required space for a Y stripe, and then pad it to align the UV data */
      pitch = fix_alignment(type, ((height + 15 & ~15) * VC_IMAGE_YUV_UV32_STRIPE_WIDTH));
      /* then add the UV stripe space and re-align */
      pitch = fix_alignment(type, pitch * 3 >> 1);

      /* If an SDRAM page pair is large enough to stay suitably aligned ... */
      align = vc_image_helper_get_sdram_page_size() << 1;
      if (align == fix_alignment(type, align))
      {
         /* ... then optimise the pitch to stagger alternate stripes between page pairs */
         pitch = pitch + (align - 1) & ~(align - 1);
         pitch |= align;
      }
      return fix_alignment(type, pitch);
   }
case_VC_IMAGE_ANY_TFORMAT:
   {
      int mipmap_levels = extra->tf.mipmap_levels;
      int bits_per_utile_row = 512 / tformat_utile_height(type);
      int unpadded_bits;
#if defined(__BCM2707A0__)
      if (mipmap_levels > 0)
      {
         // Mipmapped. Pad to nearest power of 2
         width = 1 << (_msb(width - 1) + 1);
      }
#endif
      unpadded_bits = width * vc_image_bits_per_pixel(type);
      if (calculate_mipmap_type(type, width, height, extra->tf.mipmap_levels, 0) == VC_IMAGE_MIPMAP_TFORMAT)
      {
         // T-format. Pad to tile.

         //TODO replace ALIGN_UP with something cleverer
         return ALIGN_UP(unpadded_bits, 8*bits_per_utile_row) >> 3;
      }
      else
      {
         // Linear tile format. Pad to microtile.
         return ALIGN_UP(unpadded_bits, bits_per_utile_row) >> 3;
      }
   }
   case VC_IMAGE_BAYER:
   {
      switch (extra->bayer.format)
      {
      case VC_IMAGE_BAYER_RAW6:  //4 pixels per 3 bytes
         return (((((width*3)+3)>>2) + 31)&(~31));
      case VC_IMAGE_BAYER_RAW7:  //8 pixels per 7 bytes
         return (((((width*7)+7)>>3) + 31)&(~31));
      case VC_IMAGE_BAYER_RAW8:  //1 pixel per byte
         return ((width + 31)&(~31));
      case VC_IMAGE_BAYER_RAW10:  //4 pixels per 5 bytes
         return (((((width*5)+3)>>2) + 31)&(~31));
      case VC_IMAGE_BAYER_RAW12:  //2 pixels per 3 bytes
         return (((((width*3)+1)>>1) + 31)&(~31));
      case VC_IMAGE_BAYER_RAW14:  //4 pixels per 7 bytes
         return (((((width*7)+3)>>2) + 31)&(~31));
      case VC_IMAGE_BAYER_RAW16:  //1 pixel per 2 bytes
         return (((width*2) + 31)&(~31));
      case VC_IMAGE_BAYER_RAW10_8:  //10 bits compressed to 1 pixel per bytes
      case VC_IMAGE_BAYER_RAW12_8:  //12 bits compressed to 1 pixels per bytes
      {
         int length=32;  //minimum alignment requirement
//pitch should be a multiple of the block_length
//enforce power of 2 requirement on block length
         while (length<extra->bayer.block_length)
         {
            length *= 2;
         }
         return ((width + (length-1))&(~(length-1)));
      }
      default:
         break;
      }
      break;
   }
   case VC_IMAGE_CODEC:
      return width;  //not relevant for Codec type but set to non-zero value

   case VC_IMAGE_OPENGL:
   {
      switch (extra->opengl.format_and_type)
      {
      case VC_IMAGE_OPENGL_RGBA32:
         return 4 * width;
      case VC_IMAGE_OPENGL_RGB24:
         return 3 * width;
      case VC_IMAGE_OPENGL_RGBA16:
      case VC_IMAGE_OPENGL_RGBA5551:
      case VC_IMAGE_OPENGL_RGB565:
      case VC_IMAGE_OPENGL_YA88:
         return 2 * width;
      case VC_IMAGE_OPENGL_Y8:
      case VC_IMAGE_OPENGL_A8:
      case VC_IMAGE_OPENGL_PALETTE8_RGB24:
      case VC_IMAGE_OPENGL_PALETTE8_RGBA32:
      case VC_IMAGE_OPENGL_PALETTE8_RGB565:
      case VC_IMAGE_OPENGL_PALETTE8_RGBA16:
      case VC_IMAGE_OPENGL_PALETTE8_RGB5551:
         return width;
      case VC_IMAGE_OPENGL_PALETTE4_RGB24:
      case VC_IMAGE_OPENGL_PALETTE4_RGBA32:
      case VC_IMAGE_OPENGL_PALETTE4_RGB565:
      case VC_IMAGE_OPENGL_PALETTE4_RGBA16:
      case VC_IMAGE_OPENGL_PALETTE4_RGB5551:
         //TODO find out if we need to support images with non byte-aligned rows
         vcos_assert((width & 1)==0 || height==1);
         return width>>1;
      case VC_IMAGE_OPENGL_ETC1:
         return (((width-1)|3)+1)<<1;
      default:
         break;
      }
      break;
   }
   
   case VC_IMAGE_YUV422YUYV:  /* interleaved 8 bit samples of Y, U, Y, V */
   case VC_IMAGE_YUV422YVYU:  /* interleaved 8 bit samples of Y, V, Y, U */
   case VC_IMAGE_YUV422UYVY:  /* interleaved 8 bit samples of U, Y, V, Y */
   case VC_IMAGE_YUV422VYUY:  /* interleaved 8 bit samples of V, Y, U, Y */
      return ((width + 31)&(~31))*2;
   
   default:
      break;
   }

   vcos_assert(!"unsupported format");
   return 0;
}

void vc_image_set_pitch (VC_IMAGE_T *image, int pitch)
{
   vcos_assert(image);
   if (pitch == 0) {
      /* TODO: figure out how to handle mipmap pitches */
      pitch = calculate_pitch((VC_IMAGE_TYPE_T)image->type, image->width, image->height, &image->extra);
   } else {
      switch (image->type) {
      case VC_IMAGE_1BPP:
         vcos_assert((pitch&3)==0);
         break;
      case VC_IMAGE_RGB565:
      case VC_IMAGE_RGBA565:
      case VC_IMAGE_RGBA16:
      case VC_IMAGE_3D32:
      case VC_IMAGE_3D32B:
      case VC_IMAGE_3D32MAT:
      case VC_IMAGE_YUV422:
         //      case VC_IMAGE_RGB888:
         vcos_assert((pitch&31) == 0);
         break;
      case VC_IMAGE_YUV422YUYV:  /* interleaved 8 bit samples of Y, U, Y, V */
      case VC_IMAGE_YUV422YVYU:  /* interleaved 8 bit samples of Y, V, Y, U */
      case VC_IMAGE_YUV422UYVY:  /* interleaved 8 bit samples of U, Y, V, Y */
      case VC_IMAGE_YUV422VYUY:  /* interleaved 8 bit samples of V, Y, U, Y */
         vcos_assert((pitch&31) == 0);
         break;
   
      case VC_IMAGE_YUV420:
      case VC_IMAGE_8BPP:
      case VC_IMAGE_YUV422PLANAR:
         vcos_assert((pitch&15) == 0);
         break;
      case VC_IMAGE_48BPP:
         vcos_assert((pitch%96) == 0); // a bit slow, but this is very rarely called
         break;
      case VC_IMAGE_BGR888:
      case VC_IMAGE_RGB888:
         vcos_assert((pitch%48) == 0); // a bit slow, but this is very rarely called
         break;
      case VC_IMAGE_RGBA32:
      case VC_IMAGE_RGBX32:
      case VC_IMAGE_ARGB8888:
      case VC_IMAGE_XRGB8888:
         vcos_assert((pitch&63) == 0);
         break;
      case VC_IMAGE_4BPP:
         vcos_assert((pitch&15) == 0);
         break;
      case VC_IMAGE_YUV_UV:
      case VC_IMAGE_YUV_UV32:
         {
            int align;

            align = vc_image_helper_get_sdram_page_size() << 1;
            align = max(align, 4096);
            vcos_assert((pitch & (align * 2 - 1)) == align);
            break;
         }
      case_VC_IMAGE_ANY_TFORMAT:
         // XXX this is wrong.
         //vcos_assert((pitch & 127) == 0);
         if (image->extra.tf.mipmap_levels > 0)
            vcos_assert(_count(pitch) == 1);
         break;
      case VC_IMAGE_BAYER:
         {
            switch (image->extra.bayer.format)
               {
               case VC_IMAGE_BAYER_RAW6:  //4 pixels per 3 bytes
               case VC_IMAGE_BAYER_RAW7:  //8 pixels per 7 bytes
               case VC_IMAGE_BAYER_RAW8:  //1 pixel per byte
               case VC_IMAGE_BAYER_RAW10:  //4 pixels per 5 bytes
               case VC_IMAGE_BAYER_RAW12:  //2 pixels per 3 bytes
               case VC_IMAGE_BAYER_RAW14:  //4 pixels per 7 bytes
               case VC_IMAGE_BAYER_RAW16:  //1 pixel per 2 bytes
                  vcos_assert((pitch&15) == 0);
                  break;
               case VC_IMAGE_BAYER_RAW10_8:  //10 bits compressed to 1 pixel per bytes
               case VC_IMAGE_BAYER_RAW12_8:  //12 bits compressed to 1 pixels per bytes
                  {
                     int length=16;  //minimum alignment requirement
                     //pitch should be a multiple of the block_length
                     //enforce power of 2 requirement on block length
                     while (length<image->extra.bayer.block_length)
                        {
                           length *= 2;
                        }
                     vcos_assert((pitch&(length-1)) == 0);
                     break;
                  }
               }
            break;
         }

      case VC_IMAGE_CODEC:
         break; //no constraint for Codec type

      case VC_IMAGE_OPENGL:
         {
            switch (image->extra.opengl.format_and_type)
               {
               case VC_IMAGE_OPENGL_ETC1:
                  vcos_assert((pitch & 7) == 0);
                  /* Fall through... */
               case VC_IMAGE_OPENGL_RGBA32:
                  vcos_assert((pitch & 3) == 0);
                  /* Fall through... */
               case VC_IMAGE_OPENGL_RGBA16:
               case VC_IMAGE_OPENGL_RGBA5551:
               case VC_IMAGE_OPENGL_RGB565:
               case VC_IMAGE_OPENGL_YA88:
                  vcos_assert((pitch & 1) == 0);
                  /* Fall through... */
               case VC_IMAGE_OPENGL_RGB24:
               case VC_IMAGE_OPENGL_Y8:
               case VC_IMAGE_OPENGL_A8:
                  break;   // no constraint for 24bit or 8bit OpenGL textures
               default:
                  vcos_assert(!"unsupported format");
                  break;
               }
            break;
         }
      default:
         vcos_assert(!"unsupported format");
      }
   }

   image->pitch = pitch;

   if (VC_IMAGE_IS_TFORMAT(image->type) && image->extra.tf.cube_map)
      {
         //In the t-format case, also set cube map stride

         /* Work out where the next mipmap would start. This gives us the size of
          * all thep previous ones.
          */
         int size = vc_image_get_mipmap_offset(image, image->extra.tf.mipmap_levels + 1);
         //Round up to 4kb. XXX this isn't *quite* what we want (e.g. linear tile
         //case)
#ifdef __BCM2707A0__
         image->extra.tf.cube_stride_utiles = ALIGN_UP(size,0x1000)>>6;
#endif
      }
}

/******************************************************************************
NAME
   vc_image_required_size

SYNOPSIS
   int vc_image_required_size(VC_IMAGE_T *image)

FUNCTION
   Return the space required for the given image, in bytes.

RETURNS
   int
******************************************************************************/

int vc_image_required_size (VC_IMAGE_T *image)
{
   int height;
   int pitch;
   int mipmap_levels = 0;
   int size = 0;

   height = ((image->height + 15) & ~15);
   switch (image->type)
   {
   case VC_IMAGE_YUV420:
      /* Add space for U and V blocks, which amount to half the size of the Y block */
      height = height * 3 >> 1;
      break;
   case VC_IMAGE_YUV422YUYV:
   case VC_IMAGE_YUV422PLANAR:
      /* Add space for U and V blocks, which amount to the size of the Y block */
      height <<= 1;
      break;
   case VC_IMAGE_YUV_UV:
      /* YUV_UV pitch is the area of a stripe, so we need to know how many of
       * those stripes will be used for the image. */
      height = (image->width + VC_IMAGE_YUV_UV_STRIPE_WIDTH - 1) / (unsigned)VC_IMAGE_YUV_UV_STRIPE_WIDTH;
      break;
   case VC_IMAGE_YUV_UV32:
      /* YUV_UV pitch is the area of a stripe, so we need to know how many of
       * those stripes will be used for the image. */
      height = (image->width + VC_IMAGE_YUV_UV32_STRIPE_WIDTH - 1) / (unsigned)VC_IMAGE_YUV_UV32_STRIPE_WIDTH;
      break;
   case VC_IMAGE_BGR888_NP:
      /* For no-pitch BGR, we don't round up the height to multiple of 16 */
      height = image->height;
      break;
case_VC_IMAGE_ANY_TFORMAT:
      vcos_assert(image->pitch != 0);
      /* Work out where the next mipmap would start. This gives us the size of
       * all thep previous ones.
       */
      size = vc_image_get_mipmap_offset(image, max(0, image->extra.tf.mipmap_levels) + 1);
#ifdef __BCM2707A0__
      /* If cube mapping is enabled, the cube map stride tells us the size of
       * the first five sides of the cube.
       */
      if (image->extra.tf.cube_map)
      {
         /* Theoretically cube stride can be 0, meaning all faces of the cube
          * are the same. But don't allow it for now.
          */
         vcos_assert(image->extra.tf.cube_stride_utiles != 0);
         return 5*64*image->extra.tf.cube_stride_utiles + size;
      }
      else
         return size;
#else
      return size;
#endif
   case VC_IMAGE_BAYER:
//nothing to do, just use height directly
      break;
   case VC_IMAGE_CODEC:
      return image->extra.codec.maxsize;
   case VC_IMAGE_OPENGL:
      //nothing to do
      break;
   }

   //set the pitch
   if (image->pitch == 0)
      vc_image_set_pitch(image, 0);

   pitch = image->pitch;

   size = pitch * height;

   return size;
}

int vc_image_palette_size (VC_IMAGE_T *image) {
   if (image->type == VC_IMAGE_4BPP)
      return 16*2;
   else if (image->type == VC_IMAGE_8BPP)
      return 256*2;
   return 0;
}

/******************************************************************************
NAME
   vc_image_get_[uv]

SYNOPSIS
   unsigned char *vc_image_get_[uv](VC_IMAGE_BUF_T *image)

FUNCTION
   Return the u or v pointer for a YUV420 or YUV422PLANAR image.
   Change back to a #define when code is fixed so the asserts are not needed.

RETURNS
   unsigned char *
******************************************************************************/

unsigned char *vc_image_get_u(const VC_IMAGE_BUF_T *image)
{
   vcos_assert(image->extra.uv.u);
   return (unsigned char *)image->extra.uv.u;
}

unsigned char *vc_image_get_v(const VC_IMAGE_BUF_T *image)
{
   vcos_assert(image->extra.uv.v);
   return (unsigned char *)image->extra.uv.v;
}

/******************************************************************************
NAME
   vc_image_pixel_addr

SYNOPSIS
   void *vc_image_pixel_addr

FUNCTION
   Return a pointer to the memory containing the pixel at the given coordinate.

RETURNS
   void *
******************************************************************************/

void *vc_image_pixel_addr(VC_IMAGE_BUF_T *image, int x, int y)
{
   int pitch = image->pitch;
   VC_IMAGE_TYPE_T type = (VC_IMAGE_TYPE_T)image->type;
   char *base = (char *)image->image_data;
   int nudge = 0;

   if (type == VC_IMAGE_PAL8_OBSOLETE)
      nudge = 256 * 2;

   if (type == VC_IMAGE_PAL4_OBSOLETE)
      nudge = 16 * 2;

   switch (type)
   {
   case VC_IMAGE_48BPP:
      x *= 6;
      goto byteraster;

   case VC_IMAGE_3D32: /* 16bpp plus 16 gaps every 16 pixels */
   case VC_IMAGE_3D32B:
   case VC_IMAGE_3D32MAT:
      x = (x & ~15) << 2 | (x & 15) << 1;
      goto byteraster;

   case VC_IMAGE_RGB2X9: /* 32bpp */
   case VC_IMAGE_RGB666:
   case VC_IMAGE_RGBA32:
   case VC_IMAGE_RGBX32:
   case VC_IMAGE_ARGB8888:
   case VC_IMAGE_XRGB8888:
      x *= 4;
      goto byteraster;

   case VC_IMAGE_RGB888: /* 24bpp */
   case VC_IMAGE_BGR888:
   case VC_IMAGE_BGR888_NP:
      x *= 3;
      goto byteraster;

   case VC_IMAGE_RGB565: /* 16bpp */
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      x *= 2;
      goto byteraster;

   case VC_IMAGE_4BPP:
   case VC_IMAGE_PAL4_OBSOLETE:
      {
         int halfheight = image->height >> 1;
         if (y >= halfheight)
            y -= halfheight;
      }
      goto byteraster;

   case VC_IMAGE_1BPP:
      x >>= 3;
      /*@fallthrough@*/
   case VC_IMAGE_8BPP:
   case VC_IMAGE_PAL8_OBSOLETE:
   case VC_IMAGE_YUV420:
   case VC_IMAGE_YUV422:
   case VC_IMAGE_YUV422PLANAR:
   byteraster:
      return base + x + pitch * y + nudge;

   case VC_IMAGE_YUV_UV:
      return base + (x & (VC_IMAGE_YUV_UV_STRIPE_WIDTH - 1))
                  + (x >> VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2) * pitch
                  + y * VC_IMAGE_YUV_UV_STRIPE_WIDTH;

   case VC_IMAGE_YUV_UV32:
      return base + (x & (31)) + (x >> 5) * pitch + y * 32;

   case VC_IMAGE_TF_ETC1:
   case VC_IMAGE_TF_RGBA32:
   case VC_IMAGE_TF_RGBX32:
   case VC_IMAGE_TF_FLOAT:
   case VC_IMAGE_TF_RGBA16:
   case VC_IMAGE_TF_RGBA5551:
   case VC_IMAGE_TF_RGB565:
   case VC_IMAGE_TF_YA88:
   case VC_IMAGE_TF_SHORT:
   case VC_IMAGE_TF_Y8:
   case VC_IMAGE_TF_A8:
   case VC_IMAGE_TF_BYTE:
   case VC_IMAGE_TF_PAL8:
   case VC_IMAGE_TF_1BPP:
   case VC_IMAGE_TF_PAL4:
      return vc_image_pixel_addr_mm(image, x, y, 0);

   case VC_IMAGE_OPENGL:
      y = image->height - 1 - y;
      switch (image->extra.opengl.format_and_type)
      {
      case VC_IMAGE_OPENGL_ETC1:
         x >>= 2;
         y >>= 2;
         x *= 8;
         goto byteraster;

      case VC_IMAGE_OPENGL_RGBA32:
         x *= 4;
         goto byteraster;

      case VC_IMAGE_OPENGL_RGB24:
         x *= 3;
         goto byteraster;

      case VC_IMAGE_OPENGL_RGBA16:
      case VC_IMAGE_OPENGL_RGBA5551:
      case VC_IMAGE_OPENGL_RGB565:
      case VC_IMAGE_OPENGL_YA88:
         x *= 2;
         goto byteraster;

      case VC_IMAGE_OPENGL_PALETTE4_RGB24:
      case VC_IMAGE_OPENGL_PALETTE4_RGBA32:
      case VC_IMAGE_OPENGL_PALETTE4_RGB565:
      case VC_IMAGE_OPENGL_PALETTE4_RGBA16:
      case VC_IMAGE_OPENGL_PALETTE4_RGB5551:
         x >>= 1;
         /*@fallthrough@*/
      case VC_IMAGE_OPENGL_PALETTE8_RGB24:
      case VC_IMAGE_OPENGL_PALETTE8_RGBA32:
      case VC_IMAGE_OPENGL_PALETTE8_RGB565:
      case VC_IMAGE_OPENGL_PALETTE8_RGBA16:
      case VC_IMAGE_OPENGL_PALETTE8_RGB5551:
      case VC_IMAGE_OPENGL_Y8:
      case VC_IMAGE_OPENGL_A8:
         goto byteraster;
      }

   case VC_IMAGE_BAYER:
   case VC_IMAGE_CODEC:
   case VC_IMAGE_YUV444I:
      /* TODO */
      break;
   }
   vcos_assert(!"Unknown or unsupported image type.");
   return NULL;
}

void *vc_image_pixel_addr_mm(VC_IMAGE_BUF_T *image, int x, int y, int miplevel)
{
   int pitch = image->pitch;
   VC_IMAGE_TYPE_T type = (VC_IMAGE_TYPE_T)image->type;
   int height = image->height;
   char *base = (char*)image->image_data;
   int mipmap_type = vc_image_get_mipmap_type(image, miplevel);

   if (miplevel > 0)
   {
      base += vc_image_get_mipmap_offset(image, miplevel);
      pitch = vc_image_get_mipmap_pitch(image, miplevel);
      height >>= miplevel;
      height = max(1, height);

      /* ASSUME that the x and y coordinates are pre-scaled to the dimensions
       * of the selected mipmap level */
   }

   y = height - 1 - y;

   switch (type)
   {
   case VC_IMAGE_TF_ETC1:
      pitch <<= 2;
      x >>= 2;
      y >>= 2;
      x <<= 1;
      /*@fallthrough@*/
   case VC_IMAGE_TF_RGBA32:
   case VC_IMAGE_TF_RGBX32:
   case VC_IMAGE_TF_FLOAT:
      x <<= 1;
      /*@fallthrough@*/
   case VC_IMAGE_TF_RGBA16:
   case VC_IMAGE_TF_RGBA5551:
   case VC_IMAGE_TF_RGB565:
   case VC_IMAGE_TF_YA88:
   case VC_IMAGE_TF_SHORT:
      {
         int offset_row, offset_col, offset_min, offset;

         offset_min = (y & 3) * 8
                    + (x & 7) * 1;

         if (mipmap_type == VC_IMAGE_MIPMAP_LINEAR_TILE)
         {
            offset_row = (y & ~3) * (pitch >> 1);
            offset_col = (x >> 3) * 32;
         }
         else
         {
            offset_row = ((y + 32) & ~63) * (pitch >> 1);
            offset_col = (x >> 5) * 1024
                       + ((y >> 4) & 1) * 512;
            offset_col ^= (y & 32) ? -1024 : 0;
            offset_col ^= (x << 4) & 512;
            offset_col += ((y >> 2) & 3) * 128;
            offset_col += ((x >> 3) & 3) * 32;
         }

         offset = offset_row + offset_col + offset_min;

         return (short *)base + offset;
      }

   case VC_IMAGE_TF_1BPP:
      x >>= 2;
      /*@fallthrough@*/
   case VC_IMAGE_TF_PAL4:
      x >>= 1;
      /*@fallthrough@*/
   case VC_IMAGE_TF_Y8:
   case VC_IMAGE_TF_A8:
   case VC_IMAGE_TF_BYTE:
   case VC_IMAGE_TF_PAL8:
      {
         int offset_row, offset_col, offset_min, offset;

         offset_min = (y & 7) * 8
                    + (x & 7) * 1;

         if (mipmap_type == VC_IMAGE_MIPMAP_LINEAR_TILE)
         {
            offset_row = (y & ~7) * pitch;
            offset_col = (x >> 3) * 64;
         }
         else
         {
            offset_row = ((y + 64) & ~127) * pitch;
            offset_col = (x >> 5) * 2048
                       + ((y >> 5) & 1) * 1024;
            offset_col ^= (y & 64) ? -2048 : 0;
            offset_col ^= (x << 5) & 1024;
            offset_col += ((y >> 3) & 3) * 256;
            offset_col += ((x >> 3) & 3) * 64;
         }
         offset = offset_row + offset_col + offset_min;

         return base + offset;
      }
   }
   vcos_assert(!"Unknown or inappropriate image type for mipmap pointer lookups.");
   return NULL;
}

static void *vc_image_pixel_addr_uv(VC_IMAGE_BUF_T *image, int x, int y, char *base)
{
   int pitch = image->pitch;
   VC_IMAGE_TYPE_T type = (VC_IMAGE_TYPE_T)image->type;

   switch (type)
   {
   case VC_IMAGE_YUV420:
      return base + (x >> 1) + (pitch >> 1) * (y >> 1);

   case VC_IMAGE_YUV422:
      return base + (x >> 1) + pitch * y;

   case VC_IMAGE_YUV422PLANAR:
      return base + (x >> 1) + (pitch >> 1) * y;

   case VC_IMAGE_YUV_UV:
      return base + (x & ~1 & (VC_IMAGE_YUV_UV_STRIPE_WIDTH - 1))
                  + (x >> VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2) * pitch
                  + (y >> 1) * VC_IMAGE_YUV_UV_STRIPE_WIDTH;

   case VC_IMAGE_YUV_UV32:
      return base + (x & ~1 & 31) + (x >> 5) * pitch + (y >> 1) * 32;
   }
   vcos_assert(!"Unknown or inappropriate image type for U/V pointer lookups.");
   return NULL;
}

void *vc_image_pixel_addr_u(VC_IMAGE_BUF_T *image, int x, int y)
{
   return vc_image_pixel_addr_uv(image, x, y, (char*)image->extra.uv.u);
}

void *vc_image_pixel_addr_v(VC_IMAGE_BUF_T *image, int x, int y)
{
   char *vptr = (char *)image->extra.uv.v;
   return vc_image_pixel_addr_uv(image, x, y, vptr ? vptr : (char *)image->extra.uv.u + 1);
}


void *vc_image_pixel_addr_gl(VC_IMAGE_BUF_T *image, int x, int y, int miplevel)
{
   int height = image->height;
   if (VC_IMAGE_IS_TFORMAT(image->type))
   {
      height >>= miplevel;
      height = max(height, 1);
      return vc_image_pixel_addr_mm(image, x, height - 1 - y, miplevel);
   }

   return vc_image_pixel_addr(image, x, height - 1 - y);
}


/******************************************************************************
NAME
   vc_image_reshape

SYNOPSIS
   int vc_image_reshape(VC_IMAGE_T *image)

FUNCTION
   Reset the type and size of an image and recalculate the pitch

RETURNS
   Non-zero if the image is too small for the given type and size
******************************************************************************/

int vc_image_reshape(VC_IMAGE_T *image, VC_IMAGE_TYPE_T type, int width, int height)
{
   vcos_assert(image);
   image->type = type;
   image->width = width;
   image->height = height;
   vc_image_set_pitch(image, 0);

   // check if the image fits before setting the data pointer.
   // some functions use to set the image data assert if the
   // size is not sufficient, which we don't want.
   if (image->size < vc_image_required_size(image))
      return 1;

   vc_image_set_image_data(image, image->size, image->image_data);
   return 0;
}

int vc_image_get_mipmap_offset(VC_IMAGE_T *image, int miplvl)
{
   int result = 0;
   int i;
   int mipmap_levels = image->extra.tf.mipmap_levels;
#ifdef __BCM2707A0__
   int cube_faces = 1;
#else
   int cube_faces = image->extra.tf.cube_map ? 6 : 1;
#endif

   if (mipmap_levels == -1)
   {
      mipmap_levels = 0;
   }

   vcos_assert(VC_IMAGE_IS_TFORMAT(image->type));
   // Allowed to ask for offset of "next" miplvl as well, in order to calculate
   // allocated size.
   vcos_assert(miplvl >= 0 && miplvl <= mipmap_levels + 1);
   vcos_assert(image->width >= 1 && image->height >= 1);

   for (i = 0; i < miplvl; i++)
   {
      result += cube_faces * vc_image_get_mipmap_pitch(image, i) * vc_image_get_mipmap_padded_height(image, i);
   }
   return result;
}

unsigned int vc_image_get_mipmap_pitch(VC_IMAGE_T *image, int miplvl)
{
   int result, mipmap_levels = image->extra.tf.mipmap_levels;
   // Expect t-format image and valid mipmap level
   vcos_assert(VC_IMAGE_IS_TFORMAT(image->type));
   vcos_assert(miplvl >= 0);
   vcos_assert(mipmap_levels != -1 || miplvl == 0);
   vcos_assert(mipmap_levels == -1 || miplvl <= mipmap_levels);

   assert(image->width * vc_image_bits_per_pixel((VC_IMAGE_TYPE_T)image->type) <= image->pitch * 8);
#ifdef __BCM2707A0__
   if (mipmap_levels > 0)
   {
      // If we have mipmaps, expect pitch to be a power of 2
      vcos_assert((image->pitch & (image->pitch - 1)) == 0);
      // Expect this to be as close as possible to the width
      vcos_assert(image->width * vc_image_bits_per_pixel(image->type) > image->pitch * 4
             || image->pitch == 64 / tformat_utile_height(image->type));
   }
#endif
   if (miplvl <= 0)
      result = image->pitch;
   else
      result = calculate_pitch((VC_IMAGE_TYPE_T)image->type, max(image->width >> miplvl, 1), max(image->height >> miplvl, 1), &image->extra);

   return result;
}

unsigned int vc_image_get_mipmap_padded_height(VC_IMAGE_T *image, int miplvl)
{
   int mipmap_levels = image->extra.tf.mipmap_levels;
   int utile_height = tformat_utile_height((VC_IMAGE_TYPE_T)image->type);
   int height = image->height;
   vcos_assert(VC_IMAGE_IS_TFORMAT(image->type));
#ifdef __BCM2707A0__
   if (mipmap_levels > 0)
   {
      //Pad to a power of 2
      height = 1 << (_msb(height - 1) + 1);
   }
#endif
   if (vc_image_get_mipmap_type(image, miplvl) == VC_IMAGE_MIPMAP_TFORMAT)
   {
      //Pad to a tile multiple

      //TODO replace ALIGN_UP with something cleverer
      return ALIGN_UP(max(1,height >> miplvl), 8*utile_height);
   }
   else
   {
      //Pad to a microtile multiple
      return ALIGN_UP(max(1,height >> miplvl), utile_height);
   }
}

static int calculate_mipmap_type(VC_IMAGE_TYPE_T type, int width, int height, int mipmap_levels, int miplvl)
{
   vcos_assert(VC_IMAGE_IS_TFORMAT(type));
   if (mipmap_levels == 0)
   {
      vcos_assert(miplvl == 0);
      return VC_IMAGE_MIPMAP_TFORMAT;
   }
   else if (mipmap_levels == -1)
   {
      vcos_assert(miplvl == 0);
      return VC_IMAGE_MIPMAP_LINEAR_TILE;
   }
   else
   {
      int utileh = tformat_utile_height(type);
      int utilew = 512 / vc_image_bits_per_pixel(type) / utileh;
      vcos_assert(miplvl >= 0 && miplvl <= mipmap_levels);
      if (width > (4 * utilew) << miplvl
            && height > (4 * utileh) << miplvl)
      {
         return VC_IMAGE_MIPMAP_TFORMAT;
      }
      else
      {
         return VC_IMAGE_MIPMAP_LINEAR_TILE;
      }
   }
}

int vc_image_get_mipmap_type(VC_IMAGE_T const *image, int miplvl)
{
   return calculate_mipmap_type(
             (VC_IMAGE_TYPE_T)image->type, image->width, image->height,
             image->extra.tf.mipmap_levels, miplvl);
}

/******************************************************************************
Static function definitions.
******************************************************************************/

static unsigned int tformat_utile_height(VC_IMAGE_TYPE_T type)
{
   switch (type)
   {
   case VC_IMAGE_TF_RGBA32:
   case VC_IMAGE_TF_RGBX32:
   case VC_IMAGE_TF_FLOAT:
   case VC_IMAGE_TF_RGBA16:
   case VC_IMAGE_TF_RGBA5551:
   case VC_IMAGE_TF_RGB565:
   case VC_IMAGE_TF_YA88:
   case VC_IMAGE_TF_SHORT:
      return 4;
   case VC_IMAGE_TF_BYTE:
   case VC_IMAGE_TF_PAL8:
   case VC_IMAGE_TF_PAL4:
   case VC_IMAGE_TF_Y8:
   case VC_IMAGE_TF_A8:
   case VC_IMAGE_TF_1BPP:
      return 8;
   case VC_IMAGE_TF_ETC1:
      return 16;
   default:
      vcos_assert(!"Unknown or unsupported image type.");
      return 0;
   }
}

#if  defined(_MSC_VER) || defined(__CC_ARM)
   static int vc_image_helper_get_sdram_page_size(void)
   {
      return 4*1024;
   }
#else
   #include "vcfw/drivers/chip/sdram.h"

   static int vc_image_helper_get_sdram_page_size(void)
   {
      int page_size = 0;
      SDRAM_DRIVER_INFO_T sdram_info;

      if(vcos_verify(sdram_get_func_table()->get_info(&sdram_info) == 0))
         page_size = sdram_info.page_size;

      return page_size;
   }
#endif

