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
#include "vc_image_resize_bytes.h"
#include "interface/vcos/vcos_assert.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vc_image_resize_yuv(VC_IMAGE_T * dest, int dest_x_offset, int dest_y_offset,
                                   int dest_width, int dest_height, VC_IMAGE_T * src, int src_x_offset, int src_y_offset,
                                   int src_width, int src_height, int smooth_flag);
extern void vc_image_resize_yuv_strip(VC_IMAGE_T * dest, VC_IMAGE_T * src, int src_x_offset,
                                         int src_width, int pos, int step, int smooth_flag, void * tmp_buf, int tmp_buf_size);

/* Check extern defs match above and loads #defines and typedefs */

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/


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
   vc_image_resize_yuv

SYNOPSIS
   void vc_image_resize_yuv(VC_IMAGE_T * dest, VC_IMAGE_T * src,
                            int src_xoffset, int src_yoffset,
                            int src_width, int src_height, int smooth_flag);

FUNCTION
   Copy a portion of src to dest with resize. It is the caller's responsibility
   to ensure that source and destination areas are within their respective
   bitmaps. All widths and heights must be positive. Dimensions should be
   multiples of 2 for accurate alignment of luma and colour components.
   If smooth_flag nonzero, appropriate smoothing will be used.

RETURNS
   void
******************************************************************************/

void vc_image_resize_yuv(VC_IMAGE_T * dest,
                         int dest_x_offset, int dest_y_offset,
                         int dest_width, int dest_height,
                         VC_IMAGE_T * src,
                         int src_x_offset, int src_y_offset,
                         int src_width, int src_height,
                         int smooth_flag)

{
   unsigned char *dp;
   const unsigned char * sp;
   int dpitch = dest->pitch, spitch = src->pitch;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_YUV420) || is_valid_vc_image_buf(dest, VC_IMAGE_YUV422));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV420) || is_valid_vc_image_buf(src, VC_IMAGE_YUV422));

   /* Y component */
   dp = (unsigned char *)dest->image_data;
   dp += dest_y_offset*dpitch + dest_x_offset;
   sp = (unsigned char *)src->image_data;
   sp += src_y_offset*spitch + src_x_offset;

   vc_image_resize_bytes(dp, dest_width, dest_height, dpitch,
                         sp, src_width, src_height, spitch,
                         smooth_flag);

   /* U component */
   dest_width >>= 1;
   dest_x_offset >>= 1;
   dp = (unsigned char *)dest->image_data;
   if (dest->type == VC_IMAGE_YUV422)
      dp += dpitch >> 1;
   else
   {
      dp += dpitch*((dest->height + 15)>>4) << 4;
      dest_height >>= 1;
      dest_y_offset >>= 1;
      dpitch >>= 1;
   }
   dp += dest_y_offset*dpitch + dest_x_offset;

   src_width >>= 1;
   src_x_offset >>= 1;
   sp = (unsigned char *)src->image_data;
   if (src->type == VC_IMAGE_YUV422)
      sp += spitch >> 1;
   else
   {
      sp += spitch*((src->height + 15)>>4) << 4;
      if ((src_height & 1) && (src_y_offset & 1))
         src_y_offset++, src_height--;
      else if ((src_y_offset & 1))
         src_y_offset++, src_height-=2;

      src_y_offset = (src_y_offset)>>1;
      src_height = (src_height)>>1;
      spitch >>= 1;
   }
   sp += src_y_offset*spitch + src_x_offset;

   vc_image_resize_bytes(dp, dest_width, dest_height, dpitch,
                         sp, src_width, src_height, spitch,
                         smooth_flag);

   /* V component */
   if (dest->type == VC_IMAGE_YUV422)
      dp += dpitch >> 2;
   else
      dp += dpitch * ((dest->height + 15)>>4) << 3;

   if (src->type == VC_IMAGE_YUV422)
      sp += spitch >> 2;
   else
      sp += spitch * ((src->height + 15)>>4) << 3;

   vc_image_resize_bytes(dp, dest_width, dest_height, dpitch,
                         sp, src_width, src_height, spitch,
                         smooth_flag);
}

/******************************************************************************
NAME
   vc_image_resize_yuv_strip

SYNOPSIS
   void vc_image_resize_yuv_strip(VC_IMAGE_T * dest, VC_IMAGE_T * src,
                                  int src_x_offset, int src_width,
                                  int pos, int step, int smooth_flag,
                                  void * tmp_buf, int tmp_buf_size)

FUNCTION
   Copy a portion of src to dest with resize. It is the caller's responsibility
   to ensure that source and destination areas are within their respective
   bitmaps. All widths and heights must be positive. Dimensions should be
   multiples of 2 for accurate alignment of luma and colour components.
   If smooth_flag nonzero, appropriate smoothing will be used.

RETURNS
   void
******************************************************************************/

void vc_image_resize_yuv_strip(VC_IMAGE_T * dest, VC_IMAGE_T * src,
                               int src_x_offset, int src_width,
                               int pos, int step, int smooth_flag,
                               void * tmp_buf, int tmp_buf_size)
{
   unsigned char *dp;
   const unsigned char * sp;
   int dpitch = dest->pitch, spitch = src->pitch;
   int limit = src->height - 1;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_YUV420));
   vcos_assert(is_valid_vc_image_buf(src, dest->type));

   /* Y component */

   dp = vc_image_get_y(dest);
   sp = vc_image_get_y(src);
   sp += src_x_offset;

   vc_image_resize_bytes2(dp, dest->width, dest->height, dpitch,
                          sp, src_width, spitch,
                          pos, step, limit, smooth_flag,
                          tmp_buf, tmp_buf_size);

   /* U component */
   dp = vc_image_get_u(dest);
   sp = vc_image_get_u(src);
   sp += (src_x_offset >> 1);
   pos >>= 1;
   limit >>= 1;

   vc_image_resize_bytes2(dp, dest->width >> 1, dest->height >> 1, dpitch >> 1,
                          sp, src_width >> 1, spitch >> 1,
                          pos, step, limit, smooth_flag,
                          tmp_buf, tmp_buf_size);

   /* V component */
   dp = vc_image_get_v(dest);
   sp = vc_image_get_v(src);
   sp += (src_x_offset >> 1);

   vc_image_resize_bytes2(dp, dest->width >> 1, dest->height >> 1, dpitch >> 1,
                          sp, src_width >> 1, spitch >> 1,
                          pos, step, limit, smooth_flag,
                          tmp_buf, tmp_buf_size);
}
