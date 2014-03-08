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
#include "helpers/vc_image/vc_image_tmpbuf.h"
#include "helpers/vc_image/vc_image.h"
#include "vclib/vclib.h"
#include "interface/vcos/vcos_assert.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern VC_IMAGE_BUF_T *vc_image_alloc_tmp_image (VC_IMAGE_TYPE_T type, int w, int h);
extern void vc_image_free_tmp_image (VC_IMAGE_BUF_T *image);

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

#define NUM_TMP_IMAGES 1

/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/


/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/

static VC_IMAGE_T tmp_images[NUM_TMP_IMAGES];
static void *vc_image_tmpbuf;

/******************************************************************************
Global function definitions.
******************************************************************************/

VC_IMAGE_BUF_T *vc_image_alloc_tmp_image(VC_IMAGE_TYPE_T type, int w, int h)
{
   int i, tbsh = (vclib_get_tmp_buf_size() / NUM_TMP_IMAGES) & ~15;

   vc_image_tmpbuf = vclib_obtain_tmp_buf(0); // get tmpbuf lock

   for (i = NUM_TMP_IMAGES-1; i >= 0; --i)
   {
      if (tmp_images[i].image_data == 0)
      {
         VC_IMAGE_T *image = &tmp_images[i];

         vc_image_configure(image, type, w, h,
                            VC_IMAGE_PROP_DATA, (char *)vc_image_tmpbuf + i*tbsh,
                            VC_IMAGE_PROP_SIZE, tbsh);

         return image;
      }
   }

   (void)vcos_verify(!"Failed to find free tmp image");

   return 0;
}

void vc_image_free_tmp_image(VC_IMAGE_BUF_T *image)
{
   int i;

   vcos_assert(is_valid_vc_image_buf(image, image->type));

   for (i = 0; i < NUM_TMP_IMAGES; ++i) {
      if (image == &tmp_images[i]) {
         image->image_data = 0;
         vclib_release_tmp_buf(vc_image_tmpbuf); // release tmpbuf lock
         return;
      }
   }

   (void)vcos_verify(!"Argument isn't a tmp image");
}
