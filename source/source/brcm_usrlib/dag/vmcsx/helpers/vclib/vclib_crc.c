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
#include "interface/vcos/vcos_assert.h"

#ifdef __CC_ARM
#include "vcinclude/vc_asm_ops.h"
#define _Rarely(a) a

unsigned long vclib_crc32_combine( unsigned long* vec_crc, unsigned long* vec_len, const unsigned long* combine_table)
{
	assert(0);
	return 0;
}

unsigned long vclib_crc32_combine_rev(unsigned long* vec_crc, unsigned long* vec_len, const unsigned long* combine_table)
{
	assert(0);
	return 0;
}

void vclib_crc32_update(int rows, const void* base, unsigned long calc_pitch, unsigned long num, unsigned long poly)
{
	assert(0);
}

void vclib_crc32_update_rev( int rows, const void* base, unsigned long calc_pitch, unsigned long num, unsigned long poly)
{
	assert(0);
}

unsigned int  vclib_crc16_core(VCLIB_CRC16_STATE_T const *data, unsigned int initial, void  const *buffer, int length)
{
	assert(0);
	return 0;
}

#endif

/******************************************************************************
Public functions written in this module.
******************************************************************************/

void vclib_crc_init(VCLIB_CRC_STATE_T *state, uint32_t const *poly, int bits);
void vclib_crc_init_rev(VCLIB_CRC_STATE_T *state, uint32_t const *poly, int bits);
void vclib_crc(VCLIB_CRC_STATE_T const *state, uint32_t *crc,
                  void const *buffer, unsigned long length);

extern void vclib_crc32_init(VCLIB_CRC32_STATE_T *state, unsigned long poly);
extern void vclib_crc32_init_rev(VCLIB_CRC32_STATE_T *state, unsigned long poly);
extern unsigned long vclib_crc32(VCLIB_CRC32_STATE_T *state, unsigned long crc,
                                    const void* ptr, unsigned long length);

/******************************************************************************
Private functions in this module.
******************************************************************************/

static unsigned long gf_matrix_times(unsigned long *mat, unsigned long vec);
static void gf_matrix_square(unsigned long* square, unsigned long* mat);
static unsigned long gf_matrix_times_rev(unsigned long *mat, unsigned long vec);
static void gf_matrix_square_rev(unsigned long* square, unsigned long* mat);
extern unsigned int  vclib_crc16_core(
   VCLIB_CRC16_STATE_T const *data,
   unsigned int         initial,
   void          const *buffer,
   int                  length); /* in half-words */


/******************************************************************************
External functions used by this module.
******************************************************************************/

void vclib_crc_core(
   uint32_t    const polytab[32][16],/* r0 */
   uint32_t         *work,          /* r1 */
   uint32_t   const *data,          /* r2 */
   int               length,        /* r3 */
   int               poly_words,    /* r4 */
   int               chomp_words);  /* r5 */

/******************************************************************************
Public functions again
******************************************************************************/

/*******************************************************************************
NAME
   vclib_crc_init

SYNOPSIS
   void vclib_crc_init(VCLIB_CRC32_STATE_T *state, uint32_t const *poly, int bits)

FUNCTION
   Initialise the tables for the CRC calculation using the specified polynomial.

   'bits' may be any multiple of 32 up to and including 512.

RETURNS
   -
*******************************************************************************/

void vclib_crc_init(VCLIB_CRC_STATE_T *state, uint32_t const *poly, int bits)
{
   vclib_crc_init_rev(state, poly, bits);

   _vasm("vld        H32(0++,0), (%r+=%r)          REP 32", state->polytab, 64);
   _vasm("vbitplanes V16(0,0++),  V16(0,0++)       REP 32");
   _vasm("vbitplanes V16(16,0++), V16(16,0++)      REP 32");
   _vasm("vbitrev    V32(0,0++),  V32(0,0++), 0    REP 16"); /* halfwords swapped, */
   _vasm("vbitrev    V32(16,0++), V32(16,0++), 0   REP 16"); /* but fixed below */
   _vasm("vror       V16(0,0++),  V16(0,0++), 8    REP 32");
   _vasm("vror       V16(16,0++), V16(16,0++), 8   REP 32");
   _vasm("vbitplanes V16(0,0++),  V16(0,0++)       REP 32");
   _vasm("vbitplanes V16(16,0++), V16(16,0++)      REP 32");
   _vasm("vbitrev    V32(0,0++),   V32(0,0++), 0   REP 16"); /* halfwords fixed */
   _vasm("vbitrev    V32(16,0++),  V32(16,0++), 0  REP 16");
   _vasm("vror       V16(0 ,0++),  V16(0 ,0++), 8  REP 32");
   _vasm("vror       V16(16,0++),  V16(16,0++), 8  REP 32");
   _vasm("vst H32(0++,0), (%r+=%r)                 REP 32", state->polytab, 64);

   state->dir = 1;
}


/*******************************************************************************
NAME
   vclib_crc_init_rev

SYNOPSIS
   void vclib_crc_init_rev(VCLIB_CRC32_STATE_T *state, uint32_t const *poly, int bits)

FUNCTION
   Initialise the tables for the CRC calculation using the specified polynomial

   'bits' may be any multiple of 32 up to and including 512.

RETURNS
   -
*******************************************************************************/

void vclib_crc_init_rev(VCLIB_CRC_STATE_T *state, uint32_t const *poly, int bits)
{
   int poly_words;
   int i, k;

   vcos_assert(0 < bits && bits <= 512);
   poly_words = bits + 31 >> 5;

   _vasm("vbitplanes -, %r                   SETF", -1 << poly_words);
   _vasm("vld        H32(32,0), (%r)         IFZ", poly);
   if ((bits & 31) != 0)
   {
      _vasm("vbitplanes -, 1                    CLRA SETF");
      _vasm("vlsr    -, H32(32,15), %r          SACC IFZ", bits & 31);
      _vasm("vshl    H32(32,0), H32(32,0), %r   SACC", 32 - (bits & 31));
   }
   _vasm("vbitplanes H16(32,0),  H16(32,0)");
   _vasm("vbitplanes H16(32,32), H16(32,32)");
   _vasm("vbitrev    H16(32,0),  H16(32,0), %r", poly_words);
   _vasm("vbitrev    H16(32,32), H16(32,32), %r", poly_words);
   _vasm("vbitplanes H16(32,0),  H16(32,0)");
   _vasm("vbitplanes H16(32,32), H16(32,32)");
   _vasm("vbitrev    H32(32,0), H32(32,0), 0");
   poly_words = 1 << (_msb(poly_words - 1) + 1);
   _vasm("vst        H32(32,0)+%r, (%r)", poly_words, state->polytab[31]);

   for (i = (512u / poly_words) - 2; i >= 0; i--)
   {
      uint32_t carry = 0;
      int oi = i + 1;

      for (k = poly_words - 1; k >= 0; k--)
      {
         state->polytab[i & 31][(i >> 5) * poly_words + k] =
                     (state->polytab[oi & 31][(oi >> 5) * poly_words + k] >> 1) | carry;
         carry =     (state->polytab[oi & 31][(oi >> 5) * poly_words + k] << 31) & 0x80000000;
      }
      if (carry)
         for (k = 0; k < poly_words; k++)
            state->polytab[i & 31][(i >> 5) * poly_words + k] ^= state->polytab[31][16 - poly_words + k];
   }
   state->poly_words = poly_words;
   state->poly_bits = bits;
   state->dir = -1;
}


/*******************************************************************************
NAME
   vclib_crc

SYNOPSIS
   void vclib_crc(VCLIB_CRC_STATE_T const *state, uint32_t *crc, void const *buffer, unsigned long length)

FUNCTION
   Calculate the CRC of a block of data using the polynomial given for this
   state buffer when vclib_crc_init*() was called.

   The size of 'crc' must be the same size as the polynomial, and must be
   32-bit aligned.

RETURNS
   -
*******************************************************************************/

void vclib_crc(VCLIB_CRC_STATE_T const *state, uint32_t *crc, void const *buffer, unsigned long length)
{
   int poly_words, chomp_words, copy_words;
   uint32_t const *data;
   int tail;

   if (length <= 0)
      return;

   _vasm("vld H32(0++,0), (%r+=%r) REP 32", state->polytab, 64);

   poly_words = state->poly_words;
   chomp_words = 16u / poly_words;
   copy_words = state->poly_bits + 31 >> 5;

   _vasm("vbitplanes H32(32,0), %r SETF", ~(-1 << copy_words));
   _vasm("vld H32(32,0), (%r) IFNZ", crc);
   if (state->dir > 0)
   {
      /* TODO: should probably be the same algorithm as used in
       * vclib_crc_init_rev() -- if we go as far as full bit-reversal then the
       * table swizzle in vclib_crc_init() will probably be simplified.
       */
      if ((state->poly_bits & 31) != 0)
      {
         _vasm("vbitplanes -, 1                    CLRA SETF");
         _vasm("vlsr    -, H32(32,15), %r          SACC IFZ", state->poly_bits & 31);
         _vasm("vshl    H32(32,0), H32(32,0), %r   SACC", 32 - (state->poly_bits & 31));
      }
      _vasm("vror H16(33,0), H16(32,32), 8");
      _vasm("vror H16(33,32), H16(32,0), 8");
      _vasm("vbitplanes H16(33,0), H16(33,0)");
      _vasm("vbitplanes H16(33,32), H16(33,32)");
      _vasm("vbitrev H16(33,0), H16(33,0), %r", copy_words);
      _vasm("vbitrev H16(33,32), H16(33,32), %r", copy_words);
      _vasm("vbitplanes H16(32,0), H16(33,0)");
      _vasm("vbitplanes H16(32,32), H16(33,32)");
   }

   tail = _min(-(int)buffer & 3, length);
   if (tail)
   {
      uint32_t head;
      int row = 16 - poly_words;
      int i;
      _vasm("vbitplanes -, 0x7fff         CLRA SETF");
             _vasm("vshl -, H32(32,1), %r SACC IFNZ", 32 - tail * 8);
      head = _vasm("vshl -, H32(32,1), %r IFZ USUM %D", 32 - tail * 8);
      _vasm("vlsr H32(32,0), H32(32,0), %r SACC", tail * 8);
      _vasm("vbitplanes -, %r SETF", -1 << poly_words);
      for (i = 32 - tail * 8; i < 32; i += 8)
         head ^= *(*(uint8_t const **)&buffer)++ << i;
      while ((i = _msb(head)) >= 0)
      {
         _vasm("veor H32(32,0), H32(32,0), H32(0,0)+%r IFZ", i * 64 + row);
         head &= ~(1 << i);
      }
      length -= tail;
   }
   tail = length & 3;
   length >>= 2;
   data = buffer;


   if ((signed long)length > 0)
   {
      vclib_crc_core(state->polytab, NULL, data, length, poly_words, chomp_words);
      data += length;
   }

   if (tail)
   {
      uint32_t head;
      int row = 16 - poly_words;
      int i;
      _vasm("vbitplanes -, 0x7fff         CLRA SETF");
             _vasm("vshl -, H32(32,1), %r SACC IFNZ", 32 - tail * 8);
      head = _vasm("vshl -, H32(32,1), %r IFZ USUM %D", 32 - tail * 8);
      _vasm("vlsr H32(32,0), H32(32,0), %r SACC", tail * 8);
      _vasm("vbitplanes -, %r SETF", -1 << poly_words);
      for (i = 32 - tail * 8; i < 32; i += 8)
         head ^= *(*(uint8_t const **)&data)++ << i;
      while ((i = _msb(head)) >= 0)
      {
         _vasm("veor H32(32,0), H32(32,0), H32(0,0)+%r IFZ", i * 64 + row);
         head &= ~(1 << i);
      }
   }

   if (state->dir > 0)
   {
      _vasm("vror H16(33,0), H16(32,32), 8");
      _vasm("vror H16(33,32), H16(32,0), 8");
      _vasm("vbitplanes H16(33,0), H16(33,0)");
      _vasm("vbitplanes H16(33,32), H16(33,32)");
      _vasm("vbitrev H16(33,0), H16(33,0), %r", copy_words);
      _vasm("vbitrev H16(33,32), H16(33,32), %r", copy_words);
      _vasm("vbitplanes H16(32,0), H16(33,0)");
      _vasm("vbitplanes H16(32,32), H16(33,32)");
      if ((state->poly_bits & 31) != 0)
      {
         _vasm("vbitplanes -, 0x7fff               CLRA SETF");
         _vasm("vshl    -, H32(32,1), %r           SACC IFNZ", state->poly_bits & 31);
         _vasm("vlsr    H32(32,0), H32(32,0), %r   SACC", 32 - (state->poly_bits & 31));
      }
   }
   _vasm("vbitplanes -, %r SETF", -1 << copy_words);
   _vasm("vst H32(32,0), (%r) IFZ", crc);
}


/*******************************************************************************
NAME
   vclib_crc32_init

SYNOPSIS
   void vclib_crc32_init(VCLIB_CRC32_STATE_T *state, uint32_t poly)

FUNCTION
   Initialise the tables for the CRC calculation using the specified polynomial

RETURNS
   -
*******************************************************************************/
void vclib_crc32_init(VCLIB_CRC32_STATE_T *state, unsigned long poly)
{
   unsigned long row, even[32], odd[32];
   int n;

   odd[0] = poly;
   row = 1 << 31;
   for (n = 1; n < 32; n++, row >>= 1)
      odd[n] = row;

   gf_matrix_square(even, odd);
   gf_matrix_square(odd, even);

   for (n=0; n < 32; n += 2) {
      gf_matrix_square(even, odd);
      vclib_memcpy(state->comb_tab[n], even, sizeof(even));

      gf_matrix_square(odd, even);
      vclib_memcpy(state->comb_tab[n + 1], odd, sizeof(odd));
   }

   {
      int m;
      unsigned long crc, flip1, flip2, flip3;

      crc = 0x80000000ul;
      state->crc_tab[0] = 0;
      for (n = 1; n < sizeof(state->crc_tab) / sizeof(*state->crc_tab); n <<= 1)
      {
         crc = crc & 0x80000000ul ? (crc << 1) ^ poly : (crc << 1);
         state->crc_tab[n] = crc;
      }
      flip1 = state->crc_tab[1];
      flip2 = state->crc_tab[2];
      flip3 = state->crc_tab[3] = flip1 ^ flip2;

      for (m = 0, n = 4; n < sizeof(state->crc_tab) / sizeof(*state->crc_tab); n += 4, m += 4)
      {
         unsigned long crc0;
         if (_Rarely(_ror(n, _msb(n)) == 1))
         {
            crc = state->crc_tab[n];
            m = 0;
         }

         crc0 = crc ^ state->crc_tab[m];
         state->crc_tab[n + 0] = crc0;
         state->crc_tab[n + 1] = crc0 ^ flip1;
         state->crc_tab[n + 2] = crc0 ^ flip2;
         state->crc_tab[n + 3] = crc0 ^ flip3;
      }
   }

   state->poly = poly;
   state->shift_dir = 1;
}

/*******************************************************************************
NAME
   vclib_crc32_init_rev

SYNOPSIS
   void vclib_crc32_init_rev(VCLIB_CRC32_STATE_T *state, uint32_t poly)

FUNCTION
   Initialise the tables for the CRC calculation using the specified polynomial,
   direction is reversed (ie for little endian CRCs)

RETURNS
   -
*******************************************************************************/
void vclib_crc32_init_rev(VCLIB_CRC32_STATE_T *state, unsigned long poly)
{
   unsigned long row, even[32], odd[32];
   int n;

   poly = _bitrev(poly, 0);
   odd[0] = poly;
   row = 1;
   for (n = 1; n < 32; n++, row <<= 1)
      odd[n] = row;

   gf_matrix_square_rev(even, odd);
   gf_matrix_square_rev(odd, even);

   for (n=0; n < 32; n += 2) {
      gf_matrix_square_rev(even, odd);
      vclib_memcpy(state->comb_tab[n], even, sizeof(even));

      gf_matrix_square_rev(odd, even);
      vclib_memcpy(state->comb_tab[n + 1], odd, sizeof(odd));
   }

   {
      int m;
      unsigned long crc, flip1, flip2, flip3;

      crc = 1;
      for (n = (sizeof(state->crc_tab) / sizeof(*state->crc_tab)) >> 1; n >= 1; n >>= 1)
      {
         crc = crc & 1 ? (crc >> 1) ^ poly : (crc >> 1);
         state->crc_tab[n] = crc;
      }
      state->crc_tab[0] = 0;

      flip1 = state->crc_tab[1];
      flip2 = state->crc_tab[2];
      flip3 = state->crc_tab[3] = flip1 ^ flip2;

      for (m = 0, n = 4; n < sizeof(state->crc_tab) / sizeof(*state->crc_tab); n += 4, m += 4)
      {
         unsigned long crc0;
         if (_Rarely(_ror(n, _msb(n)) == 1))
         {
            crc = state->crc_tab[n];
            m = 0;
         }

         crc0 = crc ^ state->crc_tab[m];
         state->crc_tab[n + 0] = crc0;
         state->crc_tab[n + 1] = crc0 ^ flip1;
         state->crc_tab[n + 2] = crc0 ^ flip2;
         state->crc_tab[n + 3] = crc0 ^ flip3;
      }
   }

   state->poly = poly;
   state->shift_dir = -1;
}

/*******************************************************************************
NAME
   vclib_crc32

SYNOPSIS
   unsigned long vclib_crc32(VCLIB_CRC32_STATE_T *state, unsigned long crc,
                             const void* ptr, unsigned long length)

FUNCTION
   Compute a CRC-32 checksum for the specified block of memory.

RETURNS
   computed crc-32
*******************************************************************************/
unsigned long vclib_crc32(VCLIB_CRC32_STATE_T *state, unsigned long crc,
                          const void* ptr, unsigned long length)
{
   unsigned long vec_mem[64+7], *vec_crc, *vec_len, seg_len, i;
   unsigned char *data;
   unsigned long poly;

   poly = state->poly;

   vec_crc = (unsigned long*)(((unsigned long)vec_mem + 31) & ~31);
   vec_len = vec_crc + 16;

   /* TODO: Do a little scalar work at the start to align the vector loads to
    * improve bus utilisation.  Also consider rounding seg_len so that we can
    * use 32-bit arithmetic. */

   /* divide work amongst 16 PPUs */
   seg_len = length >> 4;

   /* Only use the vector code when the combine() costs don't make us slower */
   if (seg_len > 20) {
      for (i=0; i < 16; ++i)
         vec_len[i] = (15 - i) * seg_len;

      /* initialise all CRCs to zero, except first which is set to 'crc' */
#ifdef __CC_ARM
	  assert(0);
#else
      _vasm("vmov H32(0,0), 0");
      _vasm("vst  H32(0,0), (%r)", vec_crc);

      *vec_crc = crc;

      /* calculate partial crcs */
      if (state->shift_dir >= 0) {
         vclib_crc32_update(16, ptr, seg_len, seg_len, poly);
         _vasm("vst H32(32,0), (%r)", vec_crc);
         crc = vclib_crc32_combine(vec_crc, vec_len, &state->comb_tab[0][0]);
      } else {
         vclib_crc32_update_rev(16, ptr, seg_len, seg_len, poly);
         _vasm("vst H32(32,0), (%r)", vec_crc);
         crc = vclib_crc32_combine_rev(vec_crc, vec_len, &state->comb_tab[0][0]);
      }
#endif
      seg_len <<= 4;

      if (seg_len >= length)
         return crc;
   }
   else
      seg_len = 0;

   data = (unsigned char*)ptr + seg_len;
   length -= seg_len;

   /* crc the last few bytes by hand .. better to do this in scalar code */
   if (state->shift_dir >= 0) {
      while (length--)
         crc = state->crc_tab[crc >> 24 ^ *data++] ^ crc << 8;
   } else {
       while (length--) {
         crc = state->crc_tab[(crc & 255) ^ *data++] ^ crc >> 8;
      }
   }

   return crc;
}

/*******************************************************************************
NAME
   vclib_crc32_2d

SYNOPSIS
   unsigned long vclib_crc32_2d(VCLIB_CRC32_STATE_T *state, unsigned long crc,
                                const void* ptr, unsigned long width,
                                unsigned long height, unsigned long pitch)

FUNCTION
   Generate a CRC32 for a 2d block of memory, consisting of height lines each
   seperated by pitch bytes, of which the CRC is for the first width bytes of
   each line. The state should have been initialised and the CRC computation
   is started with the specified 'crc' value.

RETURNS
   unsigned long
*******************************************************************************/
unsigned long vclib_crc32_2d(VCLIB_CRC32_STATE_T *state, unsigned long crc,
                             const void* ptr,            unsigned long width,
                             unsigned long height,       unsigned long pitch)
{
   unsigned long vec_mem[64+7], *vec_crc, *vec_len;
   unsigned long nlines, mlines, nvalid, j, cpitch, poly, lcrc;
   const unsigned char *data;
   int i;

   /* get pointers for control data */
   vec_crc = (unsigned long*)(((unsigned long)vec_mem + 31) & ~31);
   vec_len = vec_crc + 16;

   /* determine lines to loop through & cut-off point at which to grab the last CRC */
   nlines  = (height + 15) >> 4;
   nvalid  = (height + nlines - 1) / nlines;
   mlines  = height - (nvalid - 1) * nlines;
   cpitch  = nlines * pitch;
   data    = (const unsigned char *)ptr;

   /* fill out the vector lengths as well */
   if(nvalid > 1) {
      for(i = 15; i >= (nvalid-1); --i)
         vec_len[i] = 0;
      for(i = nvalid-2, j = mlines*width; i >= 0; --i, j += nlines*width)
         vec_len[i] = j;          
   } else {
      memset(vec_len, 0, sizeof(vec_len));
   }

   poly = state->poly;

#ifdef __CC_ARM
	  assert(0);
#else
   /* init the crcs */
   _vasm("vbitplanes H32(32,0), 1 SETF \n\t"
         "vmov       H32(32,0), %r IFNZ", crc);
   /* update CRCs with all of the data */
   if(state->shift_dir >= 0) {
      for(i = 0; i < mlines; ++i, data += pitch)
         vclib_crc32_update(nvalid, data, cpitch, width, poly);
      lcrc = _vasm("vbitplanes -, %r SETF \n\t"
                   "vmov -, H32(32,0) IFNZ USUM %D", 1 << nvalid - 1);
      for(; i < nlines; ++i, data += pitch)
         vclib_crc32_update(nvalid - 1, data, cpitch, width, poly);
      _vasm("vbitplanes -, %r SETF", 1 << nvalid - 1);
      _vasm("vmov H32(32,0), %r IFNZ", lcrc);
      _vasm("vbitplanes -, %r SETF", -1 << nvalid);
      _vasm("vmov H32(32,0), 0 IFNZ");
      _vasm("vst H32(32,0), (%r)", vec_crc);
      crc = vclib_crc32_combine(vec_crc, vec_len, &state->comb_tab[0][0]);
   } else {
      for(i = 0; i < mlines; ++i, data += pitch)
         vclib_crc32_update_rev(nvalid, data, cpitch, width, poly);
      lcrc = _vasm("vbitplanes -, %r SETF \n\t"
                   "vmov -, H32(32,0) IFNZ USUM %D", 1 << nvalid - 1);
      for(; i < nlines; ++i, data += pitch)
         vclib_crc32_update_rev(nvalid - 1, data, cpitch, width, poly);
      _vasm("vbitplanes -, %r SETF", 1 << nvalid - 1);
      _vasm("vmov H32(32,0), %r IFNZ", lcrc);
      _vasm("vbitplanes -, %r SETF", -1 << nvalid);
      _vasm("vmov H32(32,0), 0 IFNZ");
      _vasm("vst H32(32,0), (%r)", vec_crc);
      crc = vclib_crc32_combine_rev(vec_crc, vec_len, &state->comb_tab[0][0]);
   }
#endif
   return crc;   
}


/*******************************************************************************
NAME
   vclib_crc16_init

SYNOPSIS
   void vclib_crc16_init(VCLIB_CRC16_STATE_T *state, unsigned int poly)

FUNCTION
   Initialise the tables for the CRC calculation using the specified polynomial

RETURNS
   -
*******************************************************************************/

void vclib_crc16_init(VCLIB_CRC16_STATE_T *state, unsigned int poly)
{
   vclib_crc16_init_rev(state, poly);
   _vasm("vld H16(0++,0), (%r+=%r) REP 16", state->crc_tab, 32);
   _vasm("vbitrev H16(0++,0), H16(0++,0), 16 REP 16");
   _vasm("vbitplanes V16(0,0++), V16(0,0++) REP 16");
   _vasm("vbitrev V16(0,0++), V16(0,0++), 16 REP 16");
   _vasm("vbitplanes V16(0,0++), V16(0,0++) REP 16");
   _vasm("vror H16(48++,0), H16(8++,0), 8 REP 8");
   _vasm("vror H16(56++,0), H16(0++,0), 8 REP 8");
   _vasm("vst H16(48++,0), (%r+=%r) REP 16", state->crc_tab, 32);
   state->shift_dir = 1;
}


/*******************************************************************************
NAME
   vclib_crc16_init_rev

SYNOPSIS
   void vclib_crc16_init_rev(VCLIB_CRC16_STATE_T *state, unsigned int poly)

FUNCTION
   Initialise the tables for the CRC calculation using the specified polynomial

RETURNS
   -
*******************************************************************************/

void vclib_crc16_init_rev(VCLIB_CRC16_STATE_T *state, unsigned int poly)
{
   uint16_t crc = 0x0001;
   int i, j;

   poly = _bitrev(poly, 16);
   for (i = 15; i >= 0; i--)
      for (j = 15; j >= 0; j--)
      {
         crc = crc >> 1 ^ (crc & 0x0001 ? poly : 0);
         state->crc_tab[j][i] = crc;
      }
   state->shift_dir = -1;
}


/*******************************************************************************
NAME
   vclib_crc16

SYNOPSIS
   unsigned int vclib_crc16(VCLIB_CRC16_STATE_T *state, unsigned int crc,
                             const void* ptr, unsigned long length)

FUNCTION
   Compute a CRC-16 checksum for the specified block of memory.

RETURNS
   computed crc-16
*******************************************************************************/

static inline unsigned int vclib_crc16_byte(VCLIB_CRC16_STATE_T const *state, unsigned int crc, uint8_t byte)
{
   uint32_t a, b, c;
   uint32_t toggle;
   a = state->crc_tab[15][15];
   toggle = 0;
   b = state->crc_tab[14][15];
   crc ^= byte;
   c = state->crc_tab[13][15];
   if (crc & 0x80) toggle ^= a;
   a = state->crc_tab[12][15];
   if (crc & 0x40) toggle ^= b;
   b = state->crc_tab[11][15];
   if (crc & 0x20) toggle ^= c;
   c = state->crc_tab[10][15];
   if (crc & 0x10) toggle ^= a;
   a = state->crc_tab[ 9][15];
   if (crc & 0x08) toggle ^= b;
   b = state->crc_tab[ 8][15];
   if (crc & 0x04) toggle ^= c;
   if (crc & 0x02) toggle ^= a;
   if (crc & 0x01) toggle ^= b;

   return (crc >> 8 ^ toggle) & 0xffff;
}

unsigned int vclib_crc16(VCLIB_CRC16_STATE_T const *state, unsigned int crc, void const *ptr, unsigned long length)
{
   if (length <= 0)
      return crc;

   if (state->shift_dir > 0)
      crc = crc >> 8 | (crc & 255) << 8;

   if (_Rarely(((intptr_t)ptr & 1) != 0))
   {
      crc = vclib_crc16_byte(state, crc, *(*(uint8_t const **)&ptr)++);
      length--;
   }
   crc = vclib_crc16_core(state, crc, ptr, length >> 1);
   if (_Rarely((length & 1) != 0))
      crc = vclib_crc16_byte(state, crc, ((uint8_t const *)ptr)[length - 1]);

   if (state->shift_dir > 0)
      crc = crc >> 8 | (crc & 255) << 8;

   return crc;
}


/*******************************************************************************
Private functions
*******************************************************************************/

static unsigned long gf_matrix_times(unsigned long *mat, unsigned long vec)
{
   unsigned long sum;

   sum = 0;
   while (vec) {
      if (vec & 0x80000000)
         sum ^= *mat;
      vec <<= 1;
      mat++;
   }
   return sum;
}

static void gf_matrix_square(unsigned long* square, unsigned long* mat)
{
   int n;
   for (n = 0; n < 32; n++)
      square[n] = gf_matrix_times(mat, mat[n]);
}

static unsigned long gf_matrix_times_rev(unsigned long *mat, unsigned long vec)
{
   unsigned long sum;

   sum = 0;
   while (vec) {
      if (vec & 1)
         sum ^= *mat;
      vec >>= 1;
      mat++;
   }
   return sum;
}

static void gf_matrix_square_rev(unsigned long* square, unsigned long* mat)
{
   int n;
   for (n = 0; n < 32; n++)
      square[n] = gf_matrix_times_rev(mat, mat[n]);
}
