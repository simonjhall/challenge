/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VCLIB_CRC_H
#define VCLIB_CRC_H

typedef struct
{
   uint32_t polytab[32][16];
   uint16_t poly_words;
   uint16_t poly_bits;
   int dir;
} VCLIB_CRC_STATE_T;

typedef struct
{
   unsigned long comb_tab[32][32];
   unsigned long crc_tab[256];
   unsigned long poly;
   signed char shift_dir;

} VCLIB_CRC32_STATE_T;

typedef struct
{
   uint16_t crc_tab[16][16];
   uint16_t poly;
   int shift_dir;
} VCLIB_CRC16_STATE_T;

/* init the combine and crc tables, etc. */

/**
 * \brief Arbitrary-length CRC generator setup (MSB-first variant)
 *
 * Prepare look-up tables for a CRC generator with the specified polynomial.
 * The resulting \a state buffer can be used in calls to #clib_crc() to
 * calculate the checksum of blocks of data using the polynomial specified in
 * this call.
 *
 * The polynomial used here is expressed in an array of 32-bit words, stored in
 * little-endian order.  If the polynomial is not a multiple of 32 bits then it
 * is the most-significant bits that are ignored, so the polynomial should
 * always be an odd number and the first bit of the array should always be set.
 *
 * \b Concurrency:
 * \a state may only be passed to a single instance of \c vclib_crc_init() or
 * \c vclib_crc_init_rev() at a time.
 *
 * \b Resources:
 * The caller must have posession of the VRF during this call.
 * The VRF may be modified by this call.
 *
 * This function should not block.
 *
 * @param   state    A pointer to a state buffer in which to store look-up
 *                   tables associated with the given polynomial.
 * @param   poly     A pointer to a little-endian array (least-significant word
 *                   first) describing the polynomial.  Nothing above bit \a
 *                   bits should be set, and in normal circumstances the first
 *                   word should be an odd value.
 * @param   bits     The number of bits in the polynomial.  This can be any
 *                   value between 1 and 512 inclusive.
 */
void vclib_crc_init(VCLIB_CRC_STATE_T *state, uint32_t const *poly, int bits);

/**
 * \brief Arbitrary-length CRC generator setup (LSB-first variant)
 *
 * Prepare look-up tables for a CRC generator with the specified polynomial.
 * The resulting \a state buffer can be used in calls to #vclib_crc() to
 * calculate the checksum of blocks of data using the polynomial specified in
 * this call.
 *
 * The polynomial used here is expressed in an array of 32-bit words, stored in
 * little-endian order.  If the polynomial is not a multiple of 32 bits then it
 * is the most-significant bits that are ignored, so the polynomial should
 * always be an odd number and the first bit of the array should always be set.
 *
 * \b Concurrency:
 * \a state may only be passed to a single instance of \c vclib_crc_init() or
 * \c vclib_crc_init_rev() at a time.
 *
 * \b Resources:
 * The caller must have posession of the VRF during this call.
 * The VRF may be modified by this call.
 *
 * This function should not block.
 *
 * @param   state    A pointer to a state buffer in which to store look-up
 *                   tables associated with the given polynomial.
 * @param   poly     A pointer to a little-endian array (least-significant word
 *                   first) describing the polynomial.  Nothing above bit \a
 *                   bits should be set, and in normal circumstances the first
 *                   word should be an odd value.
 * @param   bits     The number of bits in the polynomial.  This can be any
 *                   value between 1 and 512 inclusive.
 */
void vclib_crc_init_rev(VCLIB_CRC_STATE_T *state, uint32_t const *poly, int bits);

/**
 * \brief Arbitrary-length CRC generator.
 *
 * Append \a length bytes from \a buffer to the checksum held in \a crc.  The
 * number of words at \a crc that are read and written is the number of bits of
 * the polynomial used to initialise \a state, divided by 32 and rounded up.  If
 * the number of bits is not a multiple of 32 then the most-significant bits of
 * the last word should be zero.
 *
 * \b Concurrency:
 * The \a state buffer is read-only in this function, and may be used by
 * several calls to \c vclib_crc() concurrently.  The \a crc array may be used
 * only in a single call at once.
 *
 * \b Resources:
 * The caller must have posession of the VRF during this call.
 * The VRF may be modified by this call.
 *
 * This function should not block.
 *
 * @param   state    Look-up tables.  Safe for re-entrant use.
 * @param   crc      Input initial state, output new state.  Little-endian.
 * @param   buffer   Data block to accumulate into crc.
 * @param   length   Number of bytes.
 */
void vclib_crc(VCLIB_CRC_STATE_T const *state, /*@inout@*/ uint32_t *crc, void const *buffer, unsigned long length);

extern void vclib_crc32_init(VCLIB_CRC32_STATE_T *state, unsigned long poly);

extern void vclib_crc32_init_rev(VCLIB_CRC32_STATE_T *state, unsigned long poly);

extern void vclib_crc16_init(VCLIB_CRC16_STATE_T *state, unsigned int poly);

extern void vclib_crc16_init_rev(VCLIB_CRC16_STATE_T *state, unsigned int poly);

/* utility function: perform a CRC-32 check on a block of memory */
extern unsigned long vclib_crc32(VCLIB_CRC32_STATE_T *state, unsigned long crc,
                                 const void* ptr, unsigned long length);

extern unsigned int vclib_crc16(VCLIB_CRC16_STATE_T const *state, unsigned int crc,
                                 void const *ptr, unsigned long length);

/* crc a 2d block (with pitch) as though done linearly from lowest address */
extern unsigned long vclib_crc32_2d(VCLIB_CRC32_STATE_T *state, unsigned long crc,
                                    const void* ptr,            unsigned long width,
                                    unsigned long height,       unsigned long pitch);

/* Combine a vector of CRCs (register shifting right - normal implementation) */
extern unsigned long vclib_crc32_combine( unsigned long* vec_crc, unsigned long* vec_len,
                                          const unsigned long* combine_table);

/* as above, but register shifting left */
extern unsigned long vclib_crc32_combine_rev(unsigned long* vec_crc, unsigned long* vec_len,
                                             const unsigned long* combine_table);

extern void vclib_crc32_update(int rows, const void* base,
                                 unsigned long calc_pitch, unsigned long num, unsigned long poly);
                                 /* unsigned long *crc_vec pre-loaded into H32(32,0) */

extern void vclib_crc32_update_rev( int rows, const void* base,
                                    unsigned long calc_pitch, unsigned long num, unsigned long poly);
                                 /* unsigned long *crc_vec pre-loaded into H32(32,0) */

#endif //ifndef VCLIB_CRC_H
