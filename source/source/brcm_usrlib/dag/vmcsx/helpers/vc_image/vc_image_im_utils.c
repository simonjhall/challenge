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
#include <stdlib.h>

/* Project includes */
#include "vcinclude/vcore.h"
#include "vclib/vclib.h"
#include "helpers/vc_image/vc_image.h"
#include "interface/vcos/vcos_assert.h"

#ifdef __CC_ARM
#define asm(x) 
#endif

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void im_split_components (VC_IMAGE_T *dest, VC_IMAGE_T *src,
                                    int width, int height, int pitch);
extern void im_merge_components_adjacent (VC_IMAGE_T *dest, VC_IMAGE_T *src,
         int width, int height, int pitch, int badjacent);
extern void im_merge_components (VC_IMAGE_T *dest, VC_IMAGE_T *src,
                                    int width, int height, int pitch );

/* Check extern defs match above and loads #defines and typedefs */

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

extern void im_load_1_block(unsigned char *addr, int pitch, int vrf_row);
extern void im_load_2_blocks(unsigned char *addr, int pitch, int vrf_row);
extern void im_load_3_blocks(unsigned char *addr, int pitch, int vrf_row);
extern void im_load_4_blocks(unsigned char *addr, int pitch, int vrf_row);
extern void im_split_planar(int n);
extern void im_save_1_block(unsigned char *addr, int pitch, int nrows);
extern void im_save_2_blocks(unsigned char *addr, unsigned char *addr2, int pitch, int nrows);
extern void im_save_3_blocks(unsigned char *addr, unsigned char *addr2, int pitch, int nrows);
extern void im_save_4_blocks(unsigned char *addr, unsigned char *addr2, int pitch, int nrows);
extern void im_load_2_blocks2(unsigned char *addr, unsigned char *addr2, int pitch, int vrf_row);
extern void im_load_3_blocks2(unsigned char *addr, unsigned char *addr2, int pitch, int vrf_row);
extern void im_load_4_blocks2(unsigned char *addr, unsigned char *addr2, int pitch, int vrf_row);
extern void im_merge_planar(int n);
extern void im_save_2_blocks2(unsigned char *addr, int pitch, int nrows);
extern void im_save_3_blocks2(unsigned char *addr, int pitch, int nrows);
extern void im_save_4_blocks2(unsigned char *addr, int pitch, int nrows);

/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/

typedef void (proc_fn)(unsigned char *a, int pitch, int nrows);  /* Saving or loading function */
typedef void (proc_fn2)(unsigned char *a, unsigned char *b, int pitch, int nrows); /* Saving or loading function */
typedef void (proc_fns)(int n); /* Merge / split function */

/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/

static void proc_stripe (unsigned char *dest_addr, unsigned char *src_addr, unsigned char *src_addr2,
                         int width, int ndest_pitch, int nsrc_pitch, int nrows, int ncomponents, proc_fn *fn, proc_fn2 *fn2, proc_fns *fns);
static void proc_stripe2 (unsigned char *dest_addr, unsigned char *dest_addr2, unsigned char *src_addr,
                          int width, int ndest_pitch, int nsrc_pitch, int nrows, int ncomponents, proc_fn *fn, proc_fn2 *fn2, proc_fns *fns);

/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/


/******************************************************************************
Global function definitions.
******************************************************************************/

/* Two addresses from source, 1 address from destination */
static void proc_stripe(unsigned char *dest_addr, unsigned char *src_addr, unsigned char *src_addr2,
                        int width, int ndest_pitch, int nsrc_pitch,
                        int nrows, int ncomponents,
                        proc_fn *fn, proc_fn2 *fn2, proc_fns *fns) {
   int i = ncomponents << 4;
   asm("        cbclr\n");
   for (; width >= 16; width -= 16) {
      /* Read from source */
      (*fn2)(src_addr, src_addr2, nsrc_pitch , 0);
      /* Merge */
      (*fns)(ncomponents);
      /* Write it back out */
      (*fn)(dest_addr, ndest_pitch, nrows);
      src_addr += 16;
      src_addr2 += 16;
      dest_addr += i;
   }
   if (width) {
      (*fn2)(src_addr, src_addr2, nsrc_pitch , 0);
      (*fns)(ncomponents);
      (*fn)(dest_addr, ndest_pitch, nrows);
   }

}

/* One address from source, 2 addresses from destination */
static void proc_stripe2(unsigned char *dest_addr, unsigned char *dest_addr2, unsigned char *src_addr,
                         int width, int ndest_pitch, int nsrc_pitch,
                         int nrows, int ncomponents,
                         proc_fn *fn, proc_fn2 *fn2, proc_fns *fns) {
   int i = ncomponents << 4;
   asm("        cbclr\n");
   for (; width >= 16; width -= 16) {
      /* Read from source */
      (*fn)(src_addr, nsrc_pitch , 0);
      /* Split */
      (*fns)(ncomponents);
      /* Write it back out */
      (*fn2)(dest_addr, dest_addr2, ndest_pitch, nrows);
      src_addr += i;
      dest_addr += 16;
      dest_addr2 += 16;
   }

   if (width) {
      (*fn)(src_addr, nsrc_pitch , 0);
      (*fns)(ncomponents);
      (*fn2)(dest_addr, dest_addr2, ndest_pitch, nrows);
   }


}

/***********************************************************************
   Exported functions
 ***********************************************************************/

/*****************************************************************
NAME: im_split_components

SYNOPSIS: void im_split_components(VC_IMAGE_T *dest, VC_IMAGE_T *src,
                                  int width, int height, int pitch)

FUNCTION: Convert a interleaved image to a planar one

ARGUMENTS: destination image, source image, width / height of
          source image to convert, pitch of source image

RETURN: -
******************************************************************/

void im_split_components(VC_IMAGE_T *dest, VC_IMAGE_T *src,
                         int width, int height, int pitch) {

   int nnew_pitch;
   int ncomponents;
   proc_fn *fn = NULL;
   proc_fn2 *fn2 = NULL;
   proc_fns *fns = im_split_planar;
   unsigned char *psrc_data = (unsigned char *)src->image_data;
   unsigned char *pdest_data = (unsigned char *)dest->image_data;
   unsigned char *pdest_data2;
   int height16 = (height + 15) & ~15;

   switch (src->type) {
   case VC_IMAGE_RGB888:  /* We have 3 components */
      nnew_pitch = pitch / 3;
      pdest_data2 = pdest_data + height16 * nnew_pitch;
      ncomponents = 3;
      fn = im_load_3_blocks;
      fn2 = im_save_3_blocks;
      break;
   default:
      vcos_assert(!"unsupported format");
      return;                /* can't continue as we don't know what to do */
   }

   if (fn && fn2) {
      for (; height >= 16; height -= 16) {
         proc_stripe2(pdest_data, pdest_data2, psrc_data,
                      width, nnew_pitch, pitch, 16, ncomponents,
                      fn, fn2, fns);
         pdest_data += 16*nnew_pitch;
         pdest_data2 += 16*nnew_pitch;
         psrc_data += 16*pitch;
      }

      /* In case the height is not a multiple of 16 and we
         have some left over */
      if (height)
         proc_stripe2(pdest_data, pdest_data2, psrc_data,
                      width, nnew_pitch, pitch, height, ncomponents,
                      fn, fn2, fns);
      dest->pitch = nnew_pitch;
   }

}

/*****************************************************************
NAME: im_merge_components_adjacent

SYNOPSIS: void im_merge_components_adjacent(VC_IMAGE_T *dest, VC_IMAGE_T *src,
                                            int width, int height, int pitch, int badjacent)

FUNCTION: Convert a planar image to a interleaved one, set badjacent
          to 0 if the components are aligned to multiple of 16 rows in memory
          (inverse of im_split_components), otherwise set it to 1


the inverse
          of im_split_components

ARGUMENTS: destination image, source image, width / height of
           source image to convert, pitch of source image

RETURN: -
******************************************************************/

void im_merge_components_adjacent(VC_IMAGE_T *dest, VC_IMAGE_T *src,
                                  int width, int height, int pitch,
                                  int badjacent) {

   int nnew_pitch;
   int ncomponents;
   proc_fn *fn = NULL;
   proc_fn2 *fn2 = NULL;
   proc_fns *fns = im_merge_planar;
   unsigned char *psrc_data = (unsigned char *)src->image_data;
   unsigned char *pdest_data = (unsigned char *)dest->image_data;
   unsigned char *psrc_data2;
   int height16 = (height + 15) & ~15;

   switch (src->type) {
   case VC_IMAGE_RGB888:  /* We have 3 components */
      nnew_pitch = pitch * 3;
      psrc_data2 = (badjacent)? psrc_data + height * pitch :
                   psrc_data + height16 * pitch;
      ncomponents = 3;
      fn = im_save_3_blocks2;
      fn2 = im_load_3_blocks2;
      break;
   default:
      vcos_assert(!"unsupported format");
      return;                /* can't continue as we don't know what to do */
   }

   if (fn && fn2) {
      for (; height >= 16; height -= 16) {
         proc_stripe(pdest_data, psrc_data, psrc_data2,
                     width, nnew_pitch, pitch, 16, ncomponents,
                     fn, fn2, fns);
         psrc_data += 16*pitch;
         psrc_data2 += 16*pitch;
         pdest_data += 16*nnew_pitch;

      }

      /* In case the height is not a multiple of 16 and we
         have some left over */
      if (height)
         proc_stripe(pdest_data, psrc_data, psrc_data2,
                     width, nnew_pitch, pitch, height, ncomponents,
                     fn, fn2, fns);
      dest->pitch = nnew_pitch;
   }

}

void im_merge_components(VC_IMAGE_T *dest, VC_IMAGE_T *src,
                         int width, int height, int pitch ) {

   im_merge_components_adjacent(dest, src,
                                width, height, pitch, 0);
}
