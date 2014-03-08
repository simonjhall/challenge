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
#include "vc_image.h"
#include "vclib/vclib.h"
#include "interface/vcos/vcos_assert.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vcir16_vzoom (unsigned char * dest, int w_bytes, int desth, int dest_pitch_bytes,
                             const unsigned char * src, int srch, int src_pitch_bytes, int smooth_flag);
extern void vcir16_hzoom (unsigned char * dest,  int destw, int h, int dest_pitch,
                             const unsigned char * src, int srcw, int src_pitch, int smooth_flag);
extern void vc_image_resize_rgb565_2 (unsigned char * dest, int destw, int desth, int dest_pitch,
                                         const unsigned char * src, int srcw, int srch, int src_pitch, int smooth_flag);
extern void vc_image_resize_rgb565 (VC_IMAGE_T * dest, int dest_x_offset, int dest_y_offset,
                                       int dest_width, int dest_height, VC_IMAGE_T * src, int src_x_offset, int src_y_offset,
                                       int src_width, int src_height, int smooth_flag);

/* Check extern defs match above and loads #defines and typedefs */

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

extern void vcir16_rowinterp (unsigned char *, const unsigned char *, int, int, int);
extern void vcir16_rowmix (unsigned char *, const unsigned char *, int, int, int);
extern void vcir16_nnbr_16 (unsigned char *, const unsigned char *, int, int, int, int, int);
extern void vcir16_nnbr_1 (unsigned char *, const unsigned char *, int, int, int);
extern void vcir16_down2_8 (unsigned char *, const unsigned char *, int, int, int, int);

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

/*** PRIVATE FUNCTIONS ***/

/******************************************************************************
NAME
   vcir16_vzoom

SYNOPSIS
   void vcir16_vzoom(unsigned char * dest,
                     int w_bytes, int desth, int dest_pitch,
                     const unsigned char * src,
                     int srch, int src_pitch,
                     int smooth_flag)

FUNCTION
   Decimate image in the vertical direction, by nearest neighbour.

RETURNS
   void
******************************************************************************/


/*static */void vcir16_vzoom(unsigned char * dest,
                             int w_bytes, int desth, int dest_pitch_bytes,
                             const unsigned char * src,
                             int srch, int src_pitch_bytes,
                             int smooth_flag)
{
   int pos, step, si, nrows;
   const unsigned char * sp;

   step = (srch * (1<<16)) / desth;

   if (!smooth_flag) {
      /* Nearest neighbour */
      pos = (step-1) >> 1;
      while (--desth >= 0) {
         si = pos >> 16;
         sp = src + si * src_pitch_bytes;
         vclib_memcpy(dest, sp, w_bytes);
         dest += dest_pitch_bytes;
         pos += step;
      }
      return;
   }

   if (step <= (1<<17)) {
      /* Linear interpolation */
      pos = ((srch - desth) * (1<<15)) / desth;
      --srch;
      while (--desth >= 0) {
         si = pos >> 16;
         sp = src + si * src_pitch_bytes;
         if (pos <= 0) {
            vclib_memcpy(dest, src, w_bytes);
         }
         else if (si >= srch) {
            vclib_memcpy(dest, sp, w_bytes);
         }
         else {
            vcir16_rowinterp(dest, sp, w_bytes, src_pitch_bytes, pos);
         }
         dest += dest_pitch_bytes;
         pos += step;
      }
      return;
   }

   /* Block average */
   pos = 0;
   while (--desth >= 0) {
      si = pos >> 16;
      pos += step;
      nrows = pos >> 16;
      nrows -= si;
      vcir16_rowmix(dest, src, w_bytes, src_pitch_bytes, nrows);
      dest += dest_pitch_bytes;
      src += nrows * src_pitch_bytes;
   }
}



/******************************************************************************
NAME
   vcir16_hzoom

SYNOPSIS
   void vcir16_hzoom(unsigned short * dest,
                     int destw, int h, int dest_pitch_bytes,
                     const unsigned short * src,
                     int srcw, int src_pitch_bytes,
                     int smooth_flag)

SPECIAL PRECONDITIONS
   dest_pitch *must* be a multiple of 32 bytes
   src_pitch *must* be a multiple of 32 bytes

FUNCTION
   Resample each scanline in the horizontal direction using block average,
   linear interpolation or nearest neighbour.

RETURNS
   void
******************************************************************************/


//typedef void (*ZFUNCP)(unsigned char *, const unsigned char *,int,int,int,int,int);


/*static */void
vcir16_hzoom(unsigned char * dest,
             int destw, int h, int dest_pitch,
             const unsigned char * src,
             int srcw, int src_pitch,
             int smooth_flag)
{
   //const ZFUNCP fp16[2] = { vcir16_nnbr_16, vcir16_nnbr_16 };

   int step = (srcw * (1<<16)) / destw;
   int pos0 = (step-1) >> 1;

#if 0
#error None of the "smooth" stuff works yet
   if (smooth_flag && 0) {
      if (step > (1<<17)) {
         /* If scaling down by more than 2, use block avg */
         while (--h >= 0) {
            vcir16_blockavg(dest, src, destw, step);
            dest += dest_pitch;
            src += src_pitch;
         }
         return;
      }

      /* Otherwise use linear interpolation */
      pos0 = ((srcw - destw) * (1<<15)) / destw;
      if (destw > srcw) {
         int mgn = (destw+srcw-1)/(2*srcw);
         /* The leftmost and rightmost "mgn" of pixels cannot be interpolated,
            they would need to see pixels "off the edge" of the supplied image.
            [If we are cropping, this is valid. Unfortunately we may not be.]    */
         vcir16_margin(dest, src, mgn, h, dest_pitch, src_pitch);
         vcir16_margin(dest+destw-mgn, src+srcw-1, mgn, h, dest_pitch, src_pitch);
         dest += mgn;
         pos0 += mgn*step;
         destw -= mgn*2;
      }

      smooth_flag = 1;
   }
#endif

   if ((h & 15) >= 4 && h > 16) {
      vcir16_nnbr_16(dest, src, destw, pos0, step, dest_pitch, src_pitch);
      dest += dest_pitch * (h & 15);
      src += src_pitch * (h & 15);
      h &= ~15;
   }
   while (h >= 16) {
      vcir16_nnbr_16(dest, src, destw, pos0, step, dest_pitch, src_pitch);
      dest += dest_pitch << 4;
      src += src_pitch << 4;
      h -= 16;
   }

   /* Clean up the remaining lines one at a time */
   if (h <= 0) return;
   if (smooth_flag) {
      do {
         vcir16_nnbr_1(dest, src, destw, pos0, step);
         dest += dest_pitch;
         src += src_pitch;
      } while (--h > 0);
   }
   else {
      do {
         vcir16_nnbr_1(dest, src, destw, pos0, step);
         dest += dest_pitch;
         src += src_pitch;
      } while (--h > 0);
   }
}





/*** EXPORTED FUNCTION ***/

/******************************************************************************
NAME
   vc_image_resize_rgb565

SYNOPSIS
   void vc_image_resize_rgb565(VC_IMAGE_T * dest,
                               int dest_x_offset, dest_y_offset,
                               int dest_width, int dest_height,
                               VC_IMAGE_T * src,
                               int src_x_offset, int src_y_offset,
                               int src_width, int src_height,
                               int smooth_flag)
FUNCTION
   Copies a 16-bit image from src to dest, and resizes it arbitrarily.
   If smooth_flag is zero, nearest neighbour algorithm will be used.
   If smooth_flag is nonzero, appropriate smoothing will be used.
   XXX uses horizontal nearest neighbour in all cases

RETURNS
   void
******************************************************************************/


/* Lower-level interface. Note the use of unsigned char rather than short */

void vc_image_resize_rgb565_2(unsigned char * dest,
                              int destw, int desth, int dest_pitch,
                              const unsigned char * src,
                              int srcw, int srch, int src_pitch,
                              int smooth_flag)
{
   if (desth <=0 || srch <= 0 || destw <= 0 || srcw <= 0) return;

   if (desth >= srch) {
      /* Expanding vertically. Can therefore do vertical resize in situ! */
      unsigned char * ptr = dest + dest_pitch * (desth - srch);
      vcir16_hzoom(ptr, destw, srch, dest_pitch, src, srcw, src_pitch, smooth_flag);
      vcir16_vzoom(dest, destw, desth, dest_pitch, ptr, srch, dest_pitch, smooth_flag);
   }

   else {
      /* Shrinking vertically. Since the horizontal functions are often slower,
      it makes sense to do vertical first. Do it in stripes of up to 64 rows
      */

      int i, j, step, pos, buf_pitch, buf_size;
      unsigned char * bufptr = vclib_obtain_tmp_buf(0);
      buf_size = vclib_get_tmp_buf_size();

      /* Special case for shrinking by a factor of exactly 2, all aligned */
      if (desth*2 == srch && destw*2 == srcw &&
            ((destw|(srch*2)|((int)dest)|((int)src)) & 31) == 0) {
         while (desth > 0) {
            vcir16_down2_8(dest, src, destw, dest_pitch, src_pitch, smooth_flag);
            dest += dest_pitch << 3;
            src += src_pitch << 4;
            desth -= 8;
         }
         return;
      }

      buf_pitch = (srcw + 31) & ~31;
      j = 64;
      if (buf_pitch * j > buf_size) {
         j = buf_size / buf_pitch;
         if (j > 16) j &= ~15;
         vcos_assert(j > 0);
      }
      pos = 0;
      step = j * ((srch * (1<<16)) / desth);

      for (i = 0; i < desth; i += j) {
         int s0, src_stripe_h;
         s0 = pos >> 16;
         if (i + j >= desth) {
            j = desth - i;
            src_stripe_h = srch - s0;
         }
         else {
            pos += step;
            src_stripe_h = (pos >> 16) - s0;
         }
         vcir16_vzoom(bufptr, srcw, j, buf_pitch, src, src_stripe_h, src_pitch,
                      smooth_flag);
         vcir16_hzoom(dest, destw, j, dest_pitch, bufptr, srcw, buf_pitch,
                      smooth_flag);

         dest += j * dest_pitch;
         src += src_stripe_h * src_pitch;
      }
      vclib_release_tmp_buf(bufptr);
   }
}

void vc_image_resize_rgb565(VC_IMAGE_T * dest,
                            int dest_x_offset, int dest_y_offset,
                            int dest_width, int dest_height,
                            VC_IMAGE_T * src,
                            int src_x_offset, int src_y_offset,
                            int src_width, int src_height,
                            int smooth_flag)
{
   unsigned char * dp;
   unsigned char * sp;
   dp = dest->image_data;
   dp += dest_x_offset*2 + dest_y_offset * dest->pitch;
   sp = src->image_data;
   sp += src_x_offset*2 + src_y_offset * src->pitch;
   vc_image_resize_rgb565_2(dp, dest_width*2, dest_height, dest->pitch,
                            sp, src_width*2, src_height, src->pitch,
                            smooth_flag);
}
