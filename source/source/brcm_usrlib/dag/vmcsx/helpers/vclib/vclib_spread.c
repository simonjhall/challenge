/* ============================================================================
Copyright (c) 2009-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "vcinclude/common.h"
#include "helpers/vclib/vclib.h"
#include "vcfw/rtos/rtos.h"


/******************************************************************************
Public functions written in this module.
******************************************************************************/

void vclib_spread(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);
void vclib_spread2(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);
void vclib_unspread(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);
void vclib_unspread2(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);
void vclib_unspread4(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);
void vclib_spread24to32(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);
void vclib_spread24to32hi(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);
void vclib_spread_u8to16hi(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);
void vclib_spread_ulawto16(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);
void vclib_spread_alawto16(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);


/******************************************************************************
Private stuff in this module.
******************************************************************************/

extern void vclib_spread_core(
   int               r0,         /* r0 */
   int               count,      /* r1 */
   void             *dst,        /* r2 */
   int               storepitch, /* r3 */
   void       const *src,        /* r4 */
   int               loadpitch,  /* r5 */
   int               dcount,     /* (sp+0) */
   int               scount);    /* (sp+4) */


extern void vclib_spread2_core(
   int               r0,         /* r0 */
   int               count,      /* r1 */
   void             *dst,        /* r2 */
   int               storepitch, /* r3 */
   void       const *src,        /* r4 */
   int               loadpitch,  /* r5 */
   int               dcount,     /* (sp+0) */
   int               scount);    /* (sp+4) */


extern void vclib_unspread_core(
   int               r0,         /* r0 */
   int               count,      /* r1 */
   void             *dst,        /* r2 */
   int               storepitch, /* r3 */
   void       const *src,        /* r4 */
   int               loadpitch,  /* r5 */
   int               dcount,     /* (sp+0) */
   int               scount);    /* (sp+4) */


extern void vclib_unspread2_core(
   int               r0,         /* r0 */
   int               count,      /* r1 */
   void             *dst,        /* r2 */
   int               storepitch, /* r3 */
   void       const *src,        /* r4 */
   int               loadpitch,  /* r5 */
   int               dcount,     /* (sp+0) */
   int               scount);    /* (sp+4) */


extern void vclib_unspread4_core(
   int               r0,         /* r0 */
   int               count,      /* r1 */
   void             *dst,        /* r2 */
   int               storepitch, /* r3 */
   void       const *src,        /* r4 */
   int               loadpitch,  /* r5 */
   int               dcount,     /* (sp+0) */
   int               scount);    /* (sp+4) */


/*******************************************************************************
Public functions
*******************************************************************************/

/*******************************************************************************
NAME
   vclib_spread

SYNOPSIS
   void vclib_spread(
      void             *dst,
      unsigned int      dpitch,
      void       const *src,
      unsigned int      spitch,
      unsigned int      count);

FUNCTION
   Load \a count blocks of \a spitch bytes from \a src, and store them at
   \a dst at a pitch of \a dpitch.

RETURNS
   -
*******************************************************************************/

void vclib_spread(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count)
{
   int run, blocks;

   if (dpitch == spitch)
   {
      vclib_memcpy(dst, src, dpitch * count);
      return;
   }

   if ((((intptr_t)dst | (intptr_t)src | dpitch | spitch) & 1) == 0)
   {
      vclib_spread2(dst, dpitch >> 1, src, spitch >> 1, count);
      return;
   }

   assert(spitch < dpitch && dpitch <= 64);

   run = 64 / dpitch;
   blocks = count / run;
   count %= run;
   vclib_spread_core(0, blocks, dst, dpitch * run, src, spitch * run, dpitch, spitch);

   blocks *= run;
   dst = (void *)((char *)dst + blocks * dpitch);
   src = (void *)((char *)src + blocks * spitch);
   while ((int)--count >= 0)
   {
      memcpy(dst, src, spitch);
      memset((char *)dst + spitch, 0, dpitch - spitch);
      dst = (char *)dst + dpitch;
      src = (char *)src + spitch;
   }
}

/*******************************************************************************
NAME
   vclib_spread2

SYNOPSIS
   void vclib_spread2(
      void             *dst,
      unsigned int      dpitch,
      void       const *src,
      unsigned int      spitch,
      unsigned int      count);

FUNCTION
   Load \a count blocks of \a spitch halfwords from \a src, and store them at
   \a dst at a pitch of \a dpitch.

RETURNS
   -
*******************************************************************************/

void vclib_spread2(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count)
{
   int run, blocks;

   if (dpitch == spitch)
   {
      vclib_memcpy(dst, src, dpitch * count << 1);
      return;
   }
   assert(spitch < dpitch && dpitch <= 64);

   run = 64 / dpitch;
   blocks = count / run;
   count %= run;
   vclib_spread2_core(0, blocks, dst, dpitch * run << 1, src, spitch * run << 1, dpitch, spitch);

   dpitch <<= 1;
   spitch <<= 1;
   blocks *= run;
   dst = (void *)((char *)dst + blocks * dpitch);
   src = (void *)((char *)src + blocks * spitch);
   while ((int)--count >= 0)
   {
      memcpy(dst, src, spitch);
      memset((char *)dst + spitch, 0, dpitch - spitch);
      dst = (char *)dst + dpitch;
      src = (char *)src + spitch;
   }
}


/*******************************************************************************
NAME
   vclib_unspread

SYNOPSIS
   void vclib_unspread(
      void             *dst,
      unsigned int      dpitch,
      void       const *src,
      unsigned int      spitch,
      unsigned int      count);

FUNCTION
   Load \a count blocks of \a spitch bytes from \a src, and store them at
   \a dst at a pitch of \a dpitch.

RETURNS
   -
*******************************************************************************/

void vclib_unspread(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count)
{
   int run, blocks;

   if (dpitch == spitch)
   {
      vclib_memcpy(dst, src, dpitch * count);
      return;
   }

   if ((((intptr_t)dst | (intptr_t)src | dpitch | spitch) & 3) == 0)
   {
      vclib_unspread4(dst, dpitch >> 2, src, spitch >> 2, count);
      return;
   }
   if ((((intptr_t)dst | (intptr_t)src | dpitch | spitch) & 1) == 0)
   {
      vclib_unspread2(dst, dpitch >> 1, src, spitch >> 1, count);
      return;
   }

   assert(dpitch < spitch && spitch <= 64);

   run = 64 / spitch;
   blocks = count / run;
   count %= run;
   vclib_unspread_core(0, blocks, dst, dpitch * run, src, spitch * run, dpitch, spitch);

   blocks *= run;
   dst = (void *)((char *)dst + blocks * dpitch);
   src = (void *)((char *)src + blocks * spitch);
   while ((int)--count >= 0)
   {
      memcpy(dst, src, dpitch);
      dst = (char *)dst + dpitch;
      src = (char *)src + spitch;
   }
}

/*******************************************************************************
NAME
   vclib_unspread2

SYNOPSIS
   void vclib_unspread2(
      void             *dst,
      unsigned int      dpitch,
      void       const *src,
      unsigned int      spitch,
      unsigned int      count);

FUNCTION
   Load \a count blocks of \a spitch halfwords from \a src, and store them at
   \a dst at a pitch of \a dpitch.

RETURNS
   -
*******************************************************************************/

void vclib_unspread2(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count)
{
   int run, blocks;

   if (dpitch == spitch)
   {
      vclib_memcpy(dst, src, dpitch * count << 1);
      return;
   }
   assert(dpitch < spitch && spitch <= 64);

   run = 64 / spitch;
   blocks = count / run;
   count %= run;
   vclib_unspread2_core(0, blocks, dst, dpitch * run << 1, src, spitch * run << 1, dpitch, spitch);

   dpitch <<= 1;
   spitch <<= 1;
   blocks *= run;
   dst = (void *)((char *)dst + blocks * dpitch);
   src = (void *)((char *)src + blocks * spitch);
   while ((int)--count >= 0)
   {
      memcpy(dst, src, dpitch);
      dst = (char *)dst + dpitch;
      src = (char *)src + spitch;
   }
}


/*******************************************************************************
NAME
   vclib_unspread4

SYNOPSIS
   void vclib_unspread4(
      void             *dst,
      unsigned int      dpitch,
      void       const *src,
      unsigned int      spitch,
      unsigned int      count);

FUNCTION
   Load \a count blocks of \a spitch halfwords from \a src, and store them at
   \a dst at a pitch of \a dpitch.

RETURNS
   -
*******************************************************************************/

void vclib_unspread4(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count)
{
   int run, blocks;

   if (dpitch == spitch)
   {
      vclib_memcpy(dst, src, dpitch * count << 2);
      return;
   }
   assert(dpitch < spitch && spitch <= 64);

   run = 64 / spitch;
   blocks = count / run;
   count %= run;
   vclib_unspread4_core(0, blocks, dst, dpitch * run << 2, src, spitch * run << 2, dpitch, spitch);

   dpitch <<= 2;
   spitch <<= 2;
   blocks *= run;
   dst = (void *)((char *)dst + blocks * dpitch);
   src = (void *)((char *)src + blocks * spitch);
   while ((int)--count >= 0)
   {
      memcpy(dst, src, dpitch);
      dst = (char *)dst + dpitch;
      src = (char *)src + spitch;
   }
}


/*******************************************************************************
NAME
   vclib_spread_24to32

SYNOPSIS
   void vclib_spread_24to32(
      void             *dst,
      unsigned int      dpitch,
      void       const *src,
      unsigned int      spitch,
      unsigned int      count);

FUNCTION
   Read \a spitch 24-bit words from \a src, sign-extend them to 32-bit and then
   pad them with zeroes to \a dpitch length and write to \a dst; repeat this
   \a count times.

RETURNS
   -

TODO
   Write this as a dedicated one-pass function.
*******************************************************************************/

void vclib_spread_24to32(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count)
{
   int dst_size = dpitch * count * sizeof(int32_t);
   int tmp_size = spitch * count * sizeof(int32_t); /* use extended src size here, not original src size */
   /* extend to the far end of the buffer, so that when we spread we don't
    * trample our own input -- even if this is an in-place operation we'll be
    * OK because extend_24to32() works from the far end anyway.
    */
   void *tmp =(void *)((char *)dst + dst_size - tmp_size);

   assert(dst_size >= tmp_size);
   assert(rtos_memory_is_valid(dst, dst_size));

   vclib_extend_24to32(tmp, src, spitch * count);
   if (tmp != dst)
      vclib_spread2(dst, dpitch * sizeof(int32_t) >> 1, tmp, spitch * sizeof(int32_t) >> 1, count);
}


/*******************************************************************************
NAME
   vclib_spread_24to32hi

SYNOPSIS
   void vclib_spread_24to32hi(
      void             *dst,
      unsigned int      dpitch,
      void       const *src,
      unsigned int      spitch,
      unsigned int      count);

FUNCTION
   Read \a spitch 24-bit words from \a src, sign-extend them to 32-bit and then
   pad them with zeroes to \a dpitch length and write to \a dst; repeat this
   \a count times.

RETURNS
   -

TODO
   Write this as a dedicated one-pass function.
*******************************************************************************/

void vclib_spread_24to32hi(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count)
{
   int dst_size = dpitch * sizeof(int32_t) * count;
   int tmp_size = spitch * sizeof(int32_t) * count; /* use extended src size here, not original src size */
   /* extend to the far end of the buffer, so that when we spread we don't
    * trample our own input -- even if this is an in-place operation we'll be
    * OK because extend_24to32hi() works from the far end anyway.
    */
   void *tmp =(void *)((char *)dst + dst_size - tmp_size);

   assert(dst_size >= tmp_size);
   assert(rtos_memory_is_valid(dst, dst_size));

   vclib_extend_24to32hi(tmp, src, spitch * count);
   if (tmp != dst)
      vclib_spread2(dst, dpitch * sizeof(int32_t) >> 1, tmp, spitch * sizeof(int32_t) >> 1, count);
}


/*******************************************************************************
NAME
   vclib_spread_u8to16hi

SYNOPSIS
   void vclib_spread_u8to16hi(
      void             *dst,
      unsigned int      dpitch,
      void       const *src,
      unsigned int      spitch,
      unsigned int      count);

FUNCTION
   Read \a spitch 8-bit bytes from \a src, extend them to 16-bit and then pad
   them with zeroes to \a dpitch length and write to \a dst; repeat this \a
   count times.  Operates from the far end of the buffer so \a dst and \a src
   may be the same value.

RETURNS
   -
*******************************************************************************/

void vclib_spread_u8to16hi(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count)
{
   int dst_size = dpitch * sizeof(int16_t) * count;
   int tmp_size = spitch * sizeof(int16_t) * count; /* use extended src size here, not original src size */
   /* extend to the far end of the buffer, so that when we spread we don't
    * trample our own input -- even if this is an in-place operation we'll be
    * OK because extend_24to32hi() works from the far end anyway.
    */
   void *tmp =(void *)((char *)dst + dst_size - tmp_size);

   assert(dst_size >= tmp_size);
   assert(rtos_memory_is_valid(dst, dst_size));

   vclib_extend_u8to16hi(tmp, src, spitch * count);
   if (tmp != dst)
      vclib_spread2(dst, dpitch, tmp, spitch, count);
}


/*******************************************************************************
NAME
   vclib_spread_ulawto16

SYNOPSIS
   void vclib_spread_ulawto16(
      void             *dst,
      unsigned int      dpitch,
      void       const *src,
      unsigned int      spitch,
      unsigned int      count);

FUNCTION
   Read \a spitch 8-bit bytes from \a src, extend them to 16-bit and then pad
   them with zeroes to \a dpitch length and write to \a dst; repeat this \a
   count times.  Operates from the far end of the buffer so \a dst and \a src
   may be the same value.

RETURNS
   -
*******************************************************************************/

void vclib_spread_ulawto16(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count)
{
   int dst_size = dpitch * sizeof(int16_t) * count;
   int tmp_size = spitch * sizeof(int16_t) * count; /* use extended src size here, not original src size */
   /* extend to the far end of the buffer, so that when we spread we don't
    * trample our own input -- even if this is an in-place operation we'll be
    * OK because extend_24to32hi() works from the far end anyway.
    */
   void *tmp =(void *)((char *)dst + dst_size - tmp_size);

   assert(dst_size >= tmp_size);
   assert(rtos_memory_is_valid(dst, dst_size));

   vclib_extend_ulawto16(tmp, src, spitch * count);
   if (tmp != dst)
      vclib_spread2(dst, dpitch, tmp, spitch, count);
}


/*******************************************************************************
NAME
   vclib_spread_alawto16

SYNOPSIS
   void vclib_spread_alawto16(
      void             *dst,
      unsigned int      dpitch,
      void       const *src,
      unsigned int      spitch,
      unsigned int      count);

FUNCTION
   Read \a spitch 8-bit bytes from \a src, extend them to 16-bit and then pad
   them with zeroes to \a dpitch length and write to \a dst; repeat this \a
   count times.  Operates from the far end of the buffer so \a dst and \a src
   may be the same value.

RETURNS
   -
*******************************************************************************/

void vclib_spread_alawto16(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count)
{
   int dst_size = dpitch * sizeof(int16_t) * count;
   int tmp_size = spitch * sizeof(int16_t) * count; /* use extended src size here, not original src size */
   /* extend to the far end of the buffer, so that when we spread we don't
    * trample our own input -- even if this is an in-place operation we'll be
    * OK because extend_24to32hi() works from the far end anyway.
    */
   void *tmp =(void *)((char *)dst + dst_size - tmp_size);

   assert(dst_size >= tmp_size);
   assert(rtos_memory_is_valid(dst, dst_size));

   vclib_extend_alawto16(tmp, src, spitch * count);
   if (tmp != dst)
      vclib_spread2(dst, dpitch, tmp, spitch, count);
}
