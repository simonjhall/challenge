/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <string.h>
#include <assert.h>
#include "vcfw/rtos/rtos.h"

extern void *memset_asm(void *s, int _c, size_t n);

void *memset(void *s, int _c, size_t n)
{
   // if this assert goes off, let dc4 know. It suggests an inefficient uncached access which can interfere with other blocks
   assert(!RTOS_IS_ALIAS_DIRECT(s) || n <= 1024*1024);
   return memset_asm(s, _c, n);
}

// Some functions like localtime use getenv which does a slow hostlink call when built with hostlink enable.
// Override MW getenv call to avoid this.
char *getenv(const char *p)
{
   return NULL;
} 


#if 0
#define UNALIGNED(X) ((long)X & (sizeof (long) - 1))
#define LBLOCKSIZE (sizeof (long))
#define DETECTNULL(X) (((X) - 0x01010101) & ~(X) & 0x80808080)
#define DETECTCHAR(X,MASK) (DETECTNULL(X ^ MASK))
#define TOO_SMALL(LEN) ((LEN) < LBLOCKSIZE)

void *memchr(const void *src_void, int c, size_t length)
{
   const unsigned char *src = (const unsigned char *) src_void;
   unsigned char d = c;
   unsigned long *asrc;
   unsigned long  mask;
   int i;

   while (UNALIGNED (src))
   {
      if (!length--)
         return NULL;
      if (*src == d)
         return (void *)src;
      src++;
   }

   if (!TOO_SMALL (length))
   {
      /* If we get this far, we know that length is large and src is
         word-aligned. */
      /* The fast code reads the source one word at a time and only
         performs the bytewise search on word-sized segments if they
         contain the search character, which is detected by XORing
         the word-sized segment with a word-sized block of the search
         character and then detecting for the presence of NUL in the
         result.  */
      asrc = (unsigned long *) src;
      mask = d << 8 | d;
      mask = mask << 16 | mask;
      for (i = 32; i < LBLOCKSIZE * 8; i <<= 1)
         mask = (mask << i) | mask;

      while (length >= LBLOCKSIZE)
      {
         if (DETECTCHAR (*asrc, mask))
            break;
         length -= LBLOCKSIZE;
         asrc++;
      }

      /* If there are fewer than LBLOCKSIZE characters left,
         then we resort to the bytewise loop.  */

      src = (unsigned char *) asrc;
   }

   while (length--)
   {
      if (*src == d)
         return (void *)src;
      src++;
   }

  return NULL;
}
#endif

