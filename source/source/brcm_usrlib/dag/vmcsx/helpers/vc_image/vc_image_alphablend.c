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
#include "interface/vcos/vcos_assert.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vc_image_make_alpha_gradient (VC_IMAGE_T *pimage, int start_alpha, int end_alpha);
extern void vc_image_alphablend (VC_IMAGE_T *dest, VC_IMAGE_T *src, VC_IMAGE_T *mask,
                                    int dest_xoffset, int dest_yoffset, int src_xoffset, int src_yoffset, int width, int height);

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

static void vc_image_alphablend_yuv (VC_IMAGE_T *dest, VC_IMAGE_T *src, VC_IMAGE_T *mask,
                                     int dest_xoffset, int dest_yoffset, int src_xoffset, int src_yoffset, int width, int height);

/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/


/******************************************************************************
Global function definitions.
******************************************************************************/

/*******************************************************************
NAME: vc_image_make_alpha_gradient

ARGUMENTS: pointer to vc_image_holding the mask,
           start alpha value, end alpha value

FUNCTION: make a horizontal alpha gradient (i.e. alpha values changes
          only in horizontal direction) which covers the entire
          VC_IMAGE_T struct passed in. Only YUV420 format is supported, and
          two masks are generated, one in Y component and one for U
          (the mask for V will be the same as U anyway).

RETURN:  -
********************************************************************/
void vc_image_make_alpha_gradient(VC_IMAGE_T *pimage,
                                  int start_alpha, int end_alpha) {

   extern void make_alpha_gradient2(unsigned char* buffer, int start_alpha, int end_alpha,
                                       int width, int height, int pitch, unsigned char *buffer2);
   assert(start_alpha >= 0 && start_alpha <= 255);
   assert(end_alpha >= 0 && end_alpha <= 255);

   if (pimage->type == VC_IMAGE_YUV420) {
      unsigned char *y = vc_image_get_y(pimage);
      unsigned char *uv = vc_image_get_u(pimage);
      make_alpha_gradient2(y, start_alpha, end_alpha,
                           pimage->width, pimage->height,
                           pimage->pitch, uv);
   } else {
      assert(0); /* Only support YUV420 */
   }

}

/*******************************************************************
NAME: vc_image_alphablend_yuv

ARGUMENTS: pointer to background, pointer to foreground,
           pointer to alpha mask, background x/y offset,
           foreground x/y offset, width, height

FUNCTION: Does alpha blending of YUV 420 images

RETURN:  -
********************************************************************/
static void vc_image_alphablend_yuv(VC_IMAGE_T *dest, VC_IMAGE_T *src, VC_IMAGE_T *mask,
                                    int dest_xoffset, int dest_yoffset,
                                    int src_xoffset, int src_yoffset,
                                    int width, int height) {
   unsigned char *ymask = vc_image_get_y(mask);
   unsigned char *uvmask = vc_image_get_u(mask);
   unsigned char *ydest = vc_image_get_y(dest) + dest_yoffset * dest->pitch + dest_xoffset;
   unsigned char *udest = vc_image_get_u(dest) + ((dest_yoffset * dest->pitch) >> 2) + (dest_xoffset >> 1);
   unsigned char *vdest = vc_image_get_v(dest) + ((dest_yoffset * dest->pitch) >> 2) + (dest_xoffset >> 1);
   unsigned char *ysrc = vc_image_get_y(src) + src_yoffset * src->pitch + src_xoffset;
   unsigned char *usrc = vc_image_get_u(src) + ((src_yoffset * src->pitch) >> 2) + (src_xoffset >> 1);
   unsigned char *vsrc = vc_image_get_v(src) + ((src_yoffset * src->pitch) >> 2) + (src_xoffset >> 1);

   extern void vc_image_alpha_blend_byte( unsigned char *dest, unsigned char *src,
                                             int width, int height,
                                             int dest_pitch, int source_pitch,
                                             unsigned char *alpha_mask, int mask_pitch );

   extern void vc_image_alpha_blend_byte2( unsigned char *dest, unsigned char *src,
                                              int width, int height,
                                              int dest_pitch, int source_pitch,
                                              unsigned char *alpha_mask, int mask_pitch );

   /* The whole blend region must lie within the boundaries of all images */
   assert(dest_xoffset + width <= dest->width && dest_yoffset + height <= dest->height);
   assert(src_xoffset + width <= src->width && src_yoffset + height <= src->height);
   assert(width <= mask->width && height <= mask->height);

   vc_image_alpha_blend_byte( ydest, ysrc,
                              width, height,
                              dest->pitch, src->pitch,
                              ymask, mask->pitch );

   vc_image_alpha_blend_byte2( udest, usrc,
                               width >> 1, height >> 1,
                               dest->pitch >> 1, src->pitch >> 1,
                               uvmask, mask->pitch >> 1);

   vc_image_alpha_blend_byte2( vdest, vsrc,
                               width >> 1, height >> 1,
                               dest->pitch >> 1, src->pitch >> 1,
                               uvmask, mask->pitch >> 1);

}


/*******************************************************************
NAME: vc_image_alphablend

ARGUMENTS: pointer to background, pointer to foreground,
           pointer to alpha mask, background x/y offset,
           foreground x/y offset, width, height

FUNCTION: Does alpha blending (only for YUV images at the moment),
          dest is overwritten while src is unchanged

RETURN:  -
********************************************************************/

void vc_image_alphablend(VC_IMAGE_T *dest, VC_IMAGE_T *src, VC_IMAGE_T *mask,
                         int dest_xoffset, int dest_yoffset,
                         int src_xoffset, int src_yoffset,
                         int width, int height) {

   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, dest->type));
   switch (dest->type) {
   case VC_IMAGE_YUV420:
      vc_image_alphablend_yuv(dest, src, mask,
                              dest_xoffset, dest_yoffset,
                              src_xoffset, src_yoffset,
                              width, height);
      break;

   default:
      assert(0); /* Only support YUV420 at the mometn */
   }

}
