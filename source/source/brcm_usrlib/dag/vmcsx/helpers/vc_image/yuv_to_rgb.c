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
#include "helpers/vc_image/vc_image.h"
#include "yuv_to_rgb.h"
#include "interface/vcos/vcos_assert.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vc_image_yuv_to_rgb_stripe (VC_IMAGE_YUV_TO_RGB_MODE_T const *mode,
                                           unsigned char *dptr, unsigned char const *yptr, unsigned char const *uptr, unsigned char const *vptr,
                                           int dstep, int dpitch, int ypitch, int uvpitch, int width, int arg1);
extern void vc_image_yuv_to_rgb (VC_IMAGE_YUV_TO_RGB_MODE_T const *mode,
                                    unsigned char *dptr, unsigned char const *yptr, unsigned char const *uptr, unsigned char const *vptr,
                                    int dstep, int dpitch, int ypitch, int uvpitch, int uvstripepitch, int width, int height, int arg1);
extern void vc_image_yuv_to_rgb_precise (VC_IMAGE_YUV_TO_RGB_MODE_T const *mode,
         unsigned char *dptr, unsigned char const *yptr, unsigned char const *uptr, unsigned char const *vptr,
         int dstep, int dpitch, int ypitch, int uvpitch, int uvstripepitch, int width, int height, int arg1);

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

static void yuv_to_rgb_stripe (VC_IMAGE_RGB_BACKEND_T *store32, VC_IMAGE_YUV_FRONTEND_T *load32,
                               VC_IMAGE_RGB_BACKEND_T *store16, VC_IMAGE_YUV_FRONTEND_T *load16,
                               unsigned char *dptr, unsigned char const *yptr, unsigned char const *uptr, unsigned char const *vptr,
                               int dstep, int dpitch, int ypitch, int uvpitch, int ccount, int arg1);

static void yuv_to_rgb_stripe3 (VC_IMAGE_RGB_BACKEND_T *store32, VC_IMAGE_YUV_FRONTEND_T *load32,
                                VC_IMAGE_RGB_BACKEND_T *store16, VC_IMAGE_YUV_FRONTEND_T *load16,
                                unsigned char *dptr, unsigned char const *yptr, unsigned char const *uptr, unsigned char const *vptr,
                                int dstep, int dpitch, int ypitch, int uvpitch, int ccount, int arg1, int arg2, int arg3);

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
   vc_image_yuv_to_rgb_stripe

SYNOPSIS
   void vc_image_yuv_to_rgb_stripe(
      VC_IMAGE_YUV_TO_RGB_MODE_T
                    const *mode,
      unsigned char       *dptr,
      unsigned char const *yptr,
      unsigned char const *uptr,
      unsigned char const *vptr,
      int                  dstep,
      int                  dpitch,
      int                  ypitch,
      int                  uvpitch,
      int                  width,
      int                  arg1)

FUNCTION

RETURNS
   void
******************************************************************************/

void vc_image_yuv_to_rgb_stripe(
   VC_IMAGE_YUV_TO_RGB_MODE_T
   const *mode,
   unsigned char       *dptr,
   unsigned char const *yptr,
   unsigned char const *uptr,
   unsigned char const *vptr,
   int                  dstep,
   int                  dpitch,
   int                  ypitch,
   int                  uvpitch,
   int                  width,
   int                  arg1)
{
   vcos_assert(mode != NULL && dptr != NULL && yptr != NULL && uptr != NULL && vptr != NULL);
   vcos_assert(width * dstep <= dpitch * 32);

   width += 15;

   yuv_to_rgb_stripe(
      mode->store32, mode->load32,
      mode->store16, (width & 16) == 0 ? NULL : mode->load16,
      dptr, yptr, uptr, vptr,
      dstep, dpitch, ypitch, uvpitch,
      width >> 5, arg1);
}


/******************************************************************************
NAME
   vc_image_yuv_to_rgb

SYNOPSIS
   void vc_image_yuv_to_rgb(
      VC_IMAGE_YUV_TO_RGB_MODE_T
                    const *mode,
      unsigned char       *dptr,
      unsigned char const *yptr,
      unsigned char const *uptr,
      unsigned char const *vptr,
      int                  dstep,
      int                  dpitch,
      int                  ypitch,
      int                  uvpitch,
      int                  uvstripepitch,
      int                  width,
      int                  height,
      int                  arg1)

FUNCTION

RETURNS
   void
******************************************************************************/

void vc_image_yuv_to_rgb(
   VC_IMAGE_YUV_TO_RGB_MODE_T
   const *mode,
   unsigned char       *dptr,
   unsigned char const *yptr,
   unsigned char const *uptr,
   unsigned char const *vptr,
   int                  dstep,
   int                  dpitch,
   int                  ypitch,
   int                  uvpitch,
   int                  uvstripepitch,
   int                  width,
   int                  height,
   int                  arg1)
{
   VC_IMAGE_YUV_FRONTEND_T  *load16;
   int                  lcount;

   vcos_assert(mode != NULL && dptr != NULL && yptr != NULL && uptr != NULL && vptr != NULL);
   vcos_assert(width * dstep <= dpitch * 32);

   width += 15;

   if ((width & 16) == 0)
      load16 = NULL;
   else
   {
      vcos_assert(mode->load16 != NULL && mode->store16 != NULL);
      load16 = mode->load16;
   }

   width >>= 5;

   vcos_assert(height >= 16);
   vcos_assert(!(height & 1));

   lcount = height >> 4;
   while (--lcount >= 0)
   {
      yuv_to_rgb_stripe(
         mode->store32, mode->load32,
         mode->store16, load16,
         dptr, yptr, uptr, vptr,
         dstep, dpitch, ypitch, uvpitch,
         width, arg1);

      dptr += dpitch << 4;
      yptr += ypitch << 4;
      uptr += uvstripepitch;
      vptr += uvstripepitch;

   }

   /* finish the end of the image if it is not
    * a multiple of 16; move pointers up to point
    * to the last 16 lines (even those already converted)
    * and convert them. */
   lcount = 16 - (height & 15);
   if(lcount < 16)
   {
      dptr -= lcount * dpitch;
      yptr -= lcount * ypitch;
      uptr -= (lcount * uvstripepitch) >> 4;
      vptr -= (lcount * uvstripepitch) >> 4;
      yuv_to_rgb_stripe(
         mode->store32, mode->load32,
         mode->store16, load16,
         dptr, yptr, uptr, vptr,
         dstep, dpitch, ypitch, uvpitch,
         width, arg1);
   }
}


/******************************************************************************
NAME
   vc_image_yuv_to_rgb_precise

SYNOPSIS
   void vc_image_yuv_to_rgb_precise(
      VC_IMAGE_YUV_TO_RGB_MODE_T
                    const *mode,
      unsigned char       *dptr,
      unsigned char const *yptr,
      unsigned char const *uptr,
      unsigned char const *vptr,
      int                  dstep,
      int                  dpitch,
      int                  ypitch,
      int                  uvpitch,
      int                  uvstripepitch,
      int                  width,
      int                  height,
      int                  arg1)

FUNCTION

RETURNS
   void
******************************************************************************/

void vc_image_yuv_to_rgb_precise(
   VC_IMAGE_YUV_TO_RGB_MODE_T
   const *mode,
   unsigned char       *dptr,
   unsigned char const *yptr,
   unsigned char const *uptr,
   unsigned char const *vptr,
   int                  dstep,
   int                  dpitch,
   int                  ypitch,
   int                  uvpitch,
   int                  uvstripepitch,
   int                  width,
   int                  height,
   int                  arg1)
{
   VC_IMAGE_YUV_FRONTEND_T  *load16;
   int                  wcount,
   lcount;

   vcos_assert(mode != NULL && dptr != NULL && yptr != NULL && uptr != NULL && vptr != NULL);
   vcos_assert(width * dstep <= dpitch * 32);

   if ((width & 31) == 0)
      load16 = NULL;
   else
   {
      vcos_assert(mode->load16 != NULL && mode->store16 != NULL);
      load16 = mode->load16;
   }

   wcount = width >> 5;
   width &= 31;
   lcount = height >> 4;
   height &= 15;
   while (--lcount >= 0)
   {
      yuv_to_rgb_stripe3(
         mode->store32, mode->load32,
         mode->store16, load16,
         dptr, yptr, uptr, vptr,
         dstep, dpitch, ypitch, uvpitch,
         wcount, arg1, 16, width);

      dptr += dpitch << 4;
      yptr += ypitch << 4;
      uptr += uvstripepitch;
      vptr += uvstripepitch;
   }
   if (height > 0)
   {
      yuv_to_rgb_stripe3(
         mode->store32, mode->load32,
         mode->store16, load16,
         dptr, yptr, uptr, vptr,
         dstep, dpitch, ypitch, uvpitch,
         wcount, arg1, height, width);
   }
}


/******************************************************************************
Static functions definitions.
******************************************************************************/

/******************************************************************************
NAME
   yuv_to_rgb_stripe

SYNOPSIS
   void yuv_to_rgb_stripe(
      VC_IMAGE_RGB_BACKEND_T  *dfunc32,
      VC_IMAGE_YUV_FRONTEND_T  *sfunc32,
      VC_IMAGE_RGB_BACKEND_T  *dfunc16,
      VC_IMAGE_YUV_FRONTEND_T  *sfunc16,
      unsigned char       *dptr,
      unsigned char const *yptr,
      unsigned char const *uptr,
      unsigned char const *vptr,
      int                  dstep,
      int                  dpitch,
      int                  ypitch,
      int                  uvpitch,
      int                  ccount,
      int                  arg1)

FUNCTION

RETURNS
   void
******************************************************************************/

static void yuv_to_rgb_stripe(
   VC_IMAGE_RGB_BACKEND_T  *store32,
   VC_IMAGE_YUV_FRONTEND_T  *load32,
   VC_IMAGE_RGB_BACKEND_T  *store16,
   VC_IMAGE_YUV_FRONTEND_T  *load16,
   unsigned char       *dptr,
   unsigned char const *yptr,
   unsigned char const *uptr,
   unsigned char const *vptr,
   int                  dstep,
   int                  dpitch,
   int                  ypitch,
   int                  uvpitch,
   int                  ccount,
   int                  arg1)
{
   while (--ccount >= 0)
   {
      load32(yptr, uptr, vptr, ypitch, uvpitch,
             store32, dptr, dpitch, arg1);
      dptr += dstep;
      yptr += 32;
      uptr += 16;
      vptr += 16;
   }
   if (load16 != NULL)
      load16(yptr, uptr, vptr, ypitch, uvpitch,
             store16, dptr, dpitch, arg1);
}

/******************************************************************************
NAME
   yuv_to_rgb_stripe3

SYNOPSIS
   void yuv_to_rgb_stripe3(
      VC_IMAGE_RGB_BACKEND_T  *dfunc32,
      VC_IMAGE_YUV_FRONTEND_T  *sfunc32,
      VC_IMAGE_RGB_BACKEND_T  *dfunc16,
      VC_IMAGE_YUV_FRONTEND_T  *sfunc16,
      unsigned char       *dptr,
      unsigned char const *yptr,
      unsigned char const *uptr,
      unsigned char const *vptr,
      int                  dstep,
      int                  dpitch,
      int                  ypitch,
      int                  uvpitch,
      int                  ccount,
      int                  arg1,
      int                  arg2,
      int                  arg3)

FUNCTION

RETURNS
   void
******************************************************************************/

static void yuv_to_rgb_stripe3(
   VC_IMAGE_RGB_BACKEND_T  *store32,
   VC_IMAGE_YUV_FRONTEND_T  *load32,
   VC_IMAGE_RGB_BACKEND_T  *store16,
   VC_IMAGE_YUV_FRONTEND_T  *load16,
   unsigned char       *dptr,
   unsigned char const *yptr,
   unsigned char const *uptr,
   unsigned char const *vptr,
   int                  dstep,
   int                  dpitch,
   int                  ypitch,
   int                  uvpitch,
   int                  ccount,
   int                  arg1,
   int                  arg2,
   int                  arg3)
{
   while (--ccount >= 0)
   {
      load32(yptr, uptr, vptr, ypitch, uvpitch,
             store32, dptr, dpitch, arg1, arg2, arg3);
      dptr += dstep;
      yptr += 32;
      uptr += 16;
      vptr += 16;
   }
   if (load16 != NULL)
      load16(yptr, uptr, vptr, ypitch, uvpitch,
             store16, dptr, dpitch, arg1, arg2, arg3);
}

