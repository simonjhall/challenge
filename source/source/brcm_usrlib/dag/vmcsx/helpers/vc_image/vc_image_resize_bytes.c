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
#include "vc_image_resize_bytes.h"
#include "vclib/vclib.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vcir8_vzoom2 (unsigned char * dest, int w, int desth, int dest_pitch, const unsigned char * src, int src_pitch,
                             int pos, int step, int limit, int smooth_flag);
extern void vcir8_hzoom (unsigned char * dest, int destw, int h, int dest_pitch, const unsigned char * src,
                            int srcw, int src_pitch, int smooth_flag);
extern void vc_image_resize_bytes (unsigned char * dest, int destw, int desth, int dest_pitch, const unsigned char * src,
                                      int srcw, int srch, int src_pitch, int smooth_flag);
extern void vc_image_resize_bytes2 (unsigned char * dest_ptr, int destw, int desth, int dest_pitch, const unsigned char * src_ptr,
                                       int srcw, int src_pitch, int pos, int step, int limit, int smooth_flag, void * bufptr, int tmp_buf_size);

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

typedef void (*ZFUNCP)(unsigned char *, const unsigned char *,int,int,int,int,int, int);

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
   vcir8_vzoom2

SYNOPSIS
   void vcir8_vzoom2(unsigned char * dest,
                     int w, int desth, int dest_pitch,
                     const unsigned char * src, int src_pitch,
                     int pos, int step, int limit,
                     int smooth_flag)

FUNCTION
   Resample image in the vertical direction, by nearest neighbour,
   linear interpolation or block averaging.

RETURNS
   void
******************************************************************************/

void vcir8_vzoom2(unsigned char * dest,
                  int w, int desth, int dest_pitch,
                  const unsigned char * src, int src_pitch,
                  int pos, int step, int limit,
                  int smooth_flag)
{
   int si, si1;
   const unsigned char * sp;

   if (!smooth_flag) {
      /* Nearest neighbour */
      pos += (step-1) >> 1;
      while (--desth >= 0) {
         si = pos >> 16;
         sp = src + si * src_pitch;
         vclib_memcpy(dest, sp, w);
         dest += dest_pitch;
         pos += step;
      }
      return;
   }

   if (step < (1<<17)) {
      /* Linear interpolation */
      pos += step >> 1;
      pos -= (1<<15);
      while (--desth >= 0) {
         si = pos >> 16;
         sp = src + si * src_pitch;
         if (pos <= 0) {
            vclib_memcpy(dest, src, w);
         }
         else if (si >= limit) {
            vclib_memcpy(dest, sp, w);
         }
         else {
            vcir8_rowinterp(dest, sp, w, src_pitch, pos);
         }
         dest += dest_pitch;
         pos += step;
      }
      return;
   }

   /* Block average */
   while (--desth >= 0) {
      si = pos >> 16;
      pos += step;
      si1 = pos >> 16;
      vcir8_rowmix(dest, src + si * src_pitch, w, src_pitch, si1 - si);
      dest += dest_pitch;
   }
}


/******************************************************************************
NAME
   vcir8_hzoom

SYNOPSIS
   void vcir8_hzoom(unsigned char * dest,
                    int destw, int h, int dest_pitch,
                    const unsigned char * src,
                    int srcw, int src_pitch,
                    int smooth_flag)

SPECIAL PRECONDITIONS
   dest_pitch *must* be a multiple of 8 bytes
   src_pitch *must* be a multiple of 8 bytes

FUNCTION
   Resample each scanline in the horizontal direction using blovk average,
   linear interpolation or nearest neighbour.

RETURNS
   void
******************************************************************************/


void
vcir8_hzoom(unsigned char * dest,
            int destw, int h, int dest_pitch,
            const unsigned char * src,
            int srcw, int src_pitch,
            int smooth_flag)
{
   const ZFUNCP fp16[3] = { vcir8_nnbr_16, vcir8_lerp_16, vcir8_bavg_16 };
   const ZFUNCP fpn[3] = { vcir8_nnbr_n, vcir8_lerp_n, vcir8_bavg_n };

   int step = (srcw * (1<<16)) / destw;
   int pos0 = (step-1) >> 1;

   if (smooth_flag) {
      if (step > (1<<17)) {
         /* If scaling down by more than 2, use block average */
         pos0 = 0;
         smooth_flag = 2;
      }
      else {
         /* Otherwise use linear interpolation */
         pos0 = (step-65536) >> 1;
         if (destw > srcw) {
            // NickH said it was right to remove the -1 here. Ask him about it.
            int mgn = (destw+srcw/*-1*/)/(2*srcw);
            /* Leftmost and rightmost "mgn" of pixels cannot be interpolated,
               they would need to see pixels "off the edge" of the given image.  */
            vcir8_margin(dest, src, mgn, h, dest_pitch, src_pitch);
            vcir8_margin(dest+destw-mgn, src+srcw-1, mgn, h, dest_pitch, src_pitch);
            dest += mgn;
            pos0 += mgn*step;
            destw -= mgn*2;
            smooth_flag = 1;
         }
      }
   }

   if (((dest_pitch | src_pitch) & 8) == 0) {
      /* Pitch is a multiple of 16. Good. */
      while (h >= 16) {
         fp16[smooth_flag](dest, src, destw, pos0, step, dest_pitch, src_pitch, 16);
         dest += dest_pitch << 4;
         src += src_pitch << 4;
         h -= 16;
      }
      if (h >= 8) {
         fpn[smooth_flag](dest, src, destw, pos0, step, dest_pitch, src_pitch, 8);
         dest += dest_pitch << 3;
         src += src_pitch << 3;
         h -= 8;
      }
      if (h > 0) {
         fpn[smooth_flag](dest, src, destw, pos0, step, dest_pitch, src_pitch, h);
      }
   }
   else {
      /* Pitch a multiple of 8 (we require this for YUV420). Interlace to get 16. */
      while (h >= 32) {
         fp16[smooth_flag](dest, src, destw, pos0, step, dest_pitch*2, src_pitch*2, 16);
         fp16[smooth_flag](dest+dest_pitch, src+src_pitch,
                           destw, pos0, step, dest_pitch*2, src_pitch*2, 16);
         dest += dest_pitch << 5;
         src += src_pitch << 5;
         h -= 32;
      }
      if (h >= 16) {
         fpn[smooth_flag](dest, src, destw, pos0, step, dest_pitch*2, src_pitch*2, 8);
         fpn[smooth_flag](dest+dest_pitch, src+src_pitch,
                          destw, pos0, step, dest_pitch*2, src_pitch*2, 8);
         dest += dest_pitch << 4;
         src += src_pitch << 4;
         h -= 16;
      }
      if (h > 0) {
         fpn[smooth_flag](dest, src, destw, pos0, step, dest_pitch*2, src_pitch*2,
                          (h+1)>>1);
         if (h > 1) {
            fpn[smooth_flag](dest+dest_pitch, src+src_pitch,
                             destw, pos0, step, dest_pitch*2, src_pitch*2, h>>1);
         }
      }
   }
}



/*** EXPORTED FUNCTION ***/

/******************************************************************************
NAME
   vc_image_resize_bytes

SYNOPSIS
   void vc_image_resize_bytes(unsigned char * dest_ptr,
                              int destw, int desth, int dest_pitch,
                              const unsigned char * src_ptr,
                              int srcw, int srch, int src_pitch,
                              int smooth_flag)

SPECIAL PRECONDITIONS
   All image dimensions must be positive
   dest_pitch *must* be a multiple of 8 bytes
   src_pitch *must* be a multiple of 8 bytes

FUNCTION
   Copies an 8-bit image from src to dest, and resizes it arbitrarily.
   If smooth_flag is zero, nearest neighbour algorithm will be used.
   If smooth_flag is nonzero, appropriate smoothing will be used.

RETURNS
   void
******************************************************************************/

void vc_image_resize_bytes(unsigned char * dest,
                           int destw, int desth, int dest_pitch,
                           const unsigned char * src,
                           int srcw, int srch, int src_pitch,
                           int smooth_flag)
{
   if (desth <=0 || srch <= 0 || destw <= 0 || srcw <= 0) return;

   if (destw == srcw && desth == srch) {
      /* Same size. */
      if (dest == src) {
         assert(dest_pitch == src_pitch);
         return;
      }
      if (dest_pitch == destw && src_pitch == srcw) {
         vclib_memcpy(dest, src, destw*desth);
      }
      else {
         do {
            vclib_memcpy(dest, src, destw);
            dest += dest_pitch;
            src += src_pitch;
         } while (--desth > 0);
      }
   }

   else if (desth >= srch) {
      /* Expanding vertically. Can do vertical resize in situ! */
      unsigned char * ptr = dest + dest_pitch * (desth - srch);
      vcir8_hzoom(ptr, destw, srch, dest_pitch, src, srcw, src_pitch, smooth_flag);
      if (desth != srch) {
         vcir8_vzoom2(dest, destw, desth, dest_pitch, ptr, dest_pitch,
                      0, (srch<<16)/desth, srch-1, smooth_flag);
      }
   }

   else if (destw >= srcw) {
      /* Expanding horizontally. Can do horizontal resize in situ! */
      unsigned char * ptr = dest + (destw - srcw);
      vcir8_vzoom2(ptr, srcw, desth, dest_pitch, src, src_pitch,
                   0, (srch<<16)/desth, srch-1, smooth_flag);
      if (destw != srcw) {
         vcir8_hzoom(dest, destw, desth, dest_pitch,
                     ptr, srcw, dest_pitch, smooth_flag);
      }
   }

   else {
      /* Shrinking both. Since the horizontal functions are often slower,
         we should do vertical first. Do it in stripes of up to 64 rows. */

      int i, j, step, pos, buf_pitch, buf_size;
      unsigned char * bufptr;

      /* Special case for shrinking by a factor of exactly 2, all aligned */
      if (desth*2 == srch && destw*2 == srcw &&
            ((destw|srch|dest_pitch|src_pitch|((int)dest)|((int)src)) & 15) == 0) {
         while (desth > 0) {
            vcir8_down2_8(dest, src, destw, dest_pitch, src_pitch, smooth_flag);
            dest += dest_pitch << 3;
            src += src_pitch << 4;
            desth -= 8;
         }
         return;
      }

#if 0
      /* XXX DESTRUCTIVE IN SITU. For Dom. Hope nobody else minds. */
      if (dest>=src && src < dest+dest_pitch*desth && dest_pitch==src_pitch) {
         vcir8_vzoom2(dest, srcw, desth, dest_pitch, dest, dest_pitch,
                      0, (srch<<16)/desth, srch-1, smooth_flag);
         vcir8_hzoom(dest, destw, desth, dest_pitch,
                     dest, srcw,  dest_pitch, smooth_flag);
         return;
      }
      assert(0); // not good for display use
#endif

      /* We have the VRF lock, so use the "shared" temporary buffer */
      bufptr = vclib_obtain_tmp_buf(0);
      buf_size = vclib_get_tmp_buf_size();
      buf_pitch = (srcw+15) & ~15;

      j = 64;
      if (buf_pitch * j > buf_size) {
         j = 32;
         if (buf_pitch * j > buf_size) {
            j = buf_size / buf_pitch;
            if (j > 16) j = 16;
            assert(j > 0);
         }
      }
      pos = 0;
      step = (srch << 16) / desth;

      for (i = 0; i < desth; i += j) {
         if (i + j > desth) {
            j = desth - i;
         }
         vcir8_vzoom2(bufptr, srcw, j, buf_pitch,
                      src, src_pitch, pos, step, srch-1, smooth_flag);
         vcir8_hzoom(dest, destw, j, dest_pitch, bufptr, srcw, buf_pitch,
                     smooth_flag);
         pos += j * step;
         dest += j * dest_pitch;
      }
      vclib_release_tmp_buf(bufptr);
   }
}

/******************************************************************************
NAME
   vc_image_resize_bytes2

SYNOPSIS
   void vc_image_resize_bytes(unsigned char * dest_ptr,
                              int destw, int desth, int dest_pitch,
                              const unsigned char * src_ptr,
                              int srcw, int src_pitch,
                              int pos, int step, int limit,
                              int smooth_flag,
                              void * tmp_buf, int tmp_buf_size)

SPECIAL PRECONDITIONS
   All image dimensions must be positive
   dest_pitch *must* be a multiple of 8 bytes
   src_pitch *must* be a multiple of 8 bytes

FUNCTION
   Copies an 8-bit image from src to dest, and resizes it arbitrarily.
   If smooth_flag is zero, nearest neighbour algorithm will be used.
   If smooth_flag is nonzero, appropriate smoothing will be used.

RETURNS
   void
******************************************************************************/

void vc_image_resize_bytes2(unsigned char * dest_ptr,
                            int destw, int desth, int dest_pitch,
                            const unsigned char * src_ptr,
                            int srcw, int src_pitch,
                            int pos, int step, int limit,
                            int smooth_flag,
                            void * bufptr, int tmp_buf_size)
{
   int i, j, buf_pitch;

   if (desth <=0 || destw <= 0 || srcw <= 0) return;

   buf_pitch = (srcw + 15) & ~15;
   j = desth;
   if (j > 64) j = 64;
   if (buf_pitch * j > tmp_buf_size) {
      j = 32;
      if (buf_pitch * j > tmp_buf_size) {
         j = tmp_buf_size / buf_pitch;
         if (j > 16) j = 16;
         assert(j > 0);
      }
   }

   for (i = 0; i < desth; i += j) {
      if (i + j > desth) {
         j = desth - i;
      }
      vcir8_vzoom2(bufptr, srcw, j, buf_pitch,
                   src_ptr, src_pitch, pos, step, limit, smooth_flag);
      vcir8_hzoom(dest_ptr, destw, j, dest_pitch, bufptr, srcw, buf_pitch,
                  smooth_flag);
      pos += j * step;
      dest_ptr += j * dest_pitch;
   }
}
