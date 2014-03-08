/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <string.h>
#include <assert.h>
#include "helpers/vclib/vclib.h"


/******************************************************************************
Public functions written in this module.
******************************************************************************/

void vclib_srand(VCLIB_RAND_STATE_T *state, uint32_t seed);
void vclib_srand_buffer(VCLIB_RAND_STATE_T *state, void const *buffer, int length);
uint32_t vclib_rand32(VCLIB_RAND_STATE_T *state);
void vclib_memrand(VCLIB_RAND_STATE_T *state, void *dest, int length);
void vclib_memrand_stateless(void *dest, int length, uint32_t seed);

/******************************************************************************
Private stuff in this module.
******************************************************************************/

extern void vclib_mt19937_update(unsigned long const *bits);
extern int vclib_mt19937_temper_block(int lines, void *dest, unsigned long const *bits);
extern void vclib_mt19937_update_and_temper(void *dest, int lines, unsigned long *bits);
extern void vclib_mt19937_update_and_temper_stateless(void *dest, int lines, unsigned long const *seed);

#define VCLIB_RAND_STATE_MAGIC 0x2453498bUL
#define RAND_BITS_LEN ((19937 + 31) >> 5)

/*******************************************************************************
NAME
   vclib_srand

SYNOPSIS
   void vclib_srand(VCLIB_RAND_STATE_T *state, uint32_t seed)

FUNCTION
   Initialise and seed the random number generator state from a 32-bit value.

RETURNS
   -
*******************************************************************************/

void vclib_srand(VCLIB_RAND_STATE_T *state, uint32_t seed)
{
   int i;

   assert(state != NULL);

   for (i = 0; i < RAND_BITS_LEN; i++)
   {
      state->bits[i] = seed;

      seed ^= (seed >> 30);
      seed = seed * 0x6c078965 + 1 + i;
   }
   state->index = i;
   state->magic = VCLIB_RAND_STATE_MAGIC;
}


/*******************************************************************************
NAME
   vclib_srand_buffer

SYNOPSIS
   void vclib_srand_buffer(VCLIB_RAND_STATE_T *state, void const *buffer, int length)

FUNCTION
   Initialise and seed the random number generator state from an array of bytes.

RETURNS
   -
*******************************************************************************/

void vclib_srand_buffer(VCLIB_RAND_STATE_T *state, void const *buffer, int length)
{
   uint32_t const *ptr_stop = (void *)((char *)buffer + length);
   uint32_t const *ptr;
   uint32_t seed;
   int index, swiz_offset = 0, count;

   assert(state != NULL);
   assert(buffer != NULL);

   /* TODO: enable support for misaligned pointers.
    */
   assert(((intptr_t)buffer & 3) == 0 && (length & 3) == 0);


   vclib_srand(state, 19650218);

   count = _max(RAND_BITS_LEN, (length + 3) >> 2);
   index = 1;
   seed = state->bits[0];
   ptr = ptr_stop;
   while (--count >= 0)
   {
      uint32_t swizzle;

      if (ptr >= ptr_stop)
      {
         ptr = buffer;
         swiz_offset = 0;
      }
      swizzle = *ptr++ + swiz_offset++;

      seed ^= seed >> 30;
      seed = ((seed * 1664525) ^ state->bits[index]) + swizzle;
      state->bits[index] = seed;
      if (++index >= RAND_BITS_LEN)
      {
         state->bits[0] = seed;
         index = 1;
      }
   }

   count = RAND_BITS_LEN - 1;
   while (--count >= 0)
   {
      uint32_t swizzle = -index;

      seed ^= seed >> 30;
      seed = ((seed * 1566083941) ^ state->bits[index]) + swizzle;
      state->bits[index] = seed;
      if (++index >= RAND_BITS_LEN)
      {
         state->bits[0] = seed;
         index = 1;
      }
   }

   /* ensure non-zero starting state */
   state->bits[0] = 0x80000000UL;
}


/*******************************************************************************
NAME
   vclib_rand32

SYNOPSIS
   uint32_t vclib_rand32(VCLIB_RAND_STATE_T *state)

FUNCTION
   Read a 32 bit random value from the pseudo-random sequence.

RETURNS
   The next 32 bits of the pseudo-random sequence.
*******************************************************************************/

uint32_t vclib_rand32(VCLIB_RAND_STATE_T *state)
{
   unsigned index;
   uint32_t result;

   assert(state != NULL && state->magic == VCLIB_RAND_STATE_MAGIC && state->index <= RAND_BITS_LEN);

   index = state->index;
   if (index >= RAND_BITS_LEN)
   {
      vclib_mt19937_update(state->bits);
      index = 0;
   }

   result = state->bits[index++];
   result ^= result >> 11;
   result ^= result << 7 & 0x9d2c5680UL;
   result ^= result << 15 & 0xefc60000UL;
   result ^= result >> 18;
   state->index = index;

   return result;
}


/*******************************************************************************
NAME
   vclib_memrand

SYNOPSIS
   void vclib_memrand(VCLIB_RAND_STATE_T *state, void *dest, int length)

FUNCTION
   Fill memory with a pseudo-random sequence.

RETURNS
   -
*******************************************************************************/

void vclib_memrand(VCLIB_RAND_STATE_T *state, void *dest, int length)
{
   uint32_t *ptr32 = dest;
   int count = length >> 2;

   assert(state != NULL && state->magic == VCLIB_RAND_STATE_MAGIC && state->index <= RAND_BITS_LEN);

   /* TODO: enable support for misaligned pointers.
    */
   assert(((intptr_t)dest & 3) == 0 && (length & 3) == 0);

   while ((state->index & 15) != 0 && count > 0)
   {
      *ptr32++ = vclib_rand32(state);
      count--;
   }

   if (count >= 16)
   {
      int lines;

      /* Finalise the current iteration */
      if (state->index < RAND_BITS_LEN)
      {
         lines = vclib_mt19937_temper_block(_min(RAND_BITS_LEN - state->index, count) >> 4, ptr32, state->bits + state->index);
         ptr32 += lines << 4;
         count -= lines << 4;
         state->index += lines << 4;
      }

      /* Iterate, temper, and write large blocks for as long as possible */
      if (count >= 16)
      {
         lines = count >> 4;

         assert(state->index == RAND_BITS_LEN);
         vclib_mt19937_update_and_temper(ptr32, lines, state->bits);

         ptr32 += lines << 4;
         count -= lines << 4;
         state->index = (lines << 4) % (unsigned)RAND_BITS_LEN;
      }
   }

   while (count > 0)
   {
      *ptr32++ = vclib_rand32(state);
      count--;
   }
}

/*******************************************************************************
NAME
   vclib_memrand_stateless

SYNOPSIS
   void vclib_memrand_stateless(void *dest, int length, uint32_t seed)

FUNCTION
   Fill memory with a pseudo-random sequence using a 32-bit seed rather than a
   state buffer.

   The seed is not guaranteed to produce the same sequence as that used
   vclib_srand().

RETURNS
   -
*******************************************************************************/

void vclib_memrand_stateless(void *dest, int length, uint32_t seed)
{
   unsigned long tmp[16];
   int i;

   /* TODO: enable support for misaligned pointers.
    */
   assert(((intptr_t)dest & 3) == 0 && (length & 63) == 0);

   for (i = 0; i < 16; i++)
   {
      tmp[i] = seed;

      seed ^= (seed >> 30);
      seed = seed * 0x6c078965 + 1 + i;
   }

   vclib_mt19937_update_and_temper_stateless(dest, length >> 6, tmp);
}

