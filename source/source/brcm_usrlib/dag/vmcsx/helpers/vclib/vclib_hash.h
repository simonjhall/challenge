/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VCLIB_HASH_H
#define VCLIB_HASH_H

#include "helpers/vclib/vclib_crc.h"
#if defined(VCLIB_SECURE_HASHES)
#include "middleware/crypto/crypto.h"

/* backward compatibility... */

typedef CRYPTO_SHA256_STATE_T VCLIB_SHA256_STATE_T;
typedef CRYPTO_SHA256P_STATE_T VCLIB_SHA256P_STATE_T;
typedef CRYPTO_CUBEHASH_STATE_T VCLIB_CUBEHASH_STATE_T;

#define vclib_sha256_init crypto_sha256_init
#define vclib_sha256 crypto_sha256
#define vclib_sha256_end crypto_sha256_end

#define vclib_sha224_init crypto_sha224_init
#define vclib_sha224 crypto_sha224
#define vclib_sha224_end crypto_sha224_end

#define vclib_sha256p_init crypto_sha256p_init
#define vclib_sha256p crypto_sha256p
#define vclib_sha256p_end crypto_sha256p_end

#define vclib_cubehash_init crypto_cubehash_init
#define vclib_cubehash crypto_cubehash
#define vclib_cubehash_end crypto_cubehash_end
#endif 

typedef struct VCLIB_DIGEST_STATE_T VCLIB_DIGEST_STATE_T;

typedef enum
{
   VCLIB_DIGEST_RAW = 0, /**< Get digest without any supporting details */
   VCLIB_DIGEST_ID = 1 << 0, /**< Include digest algorithm identifier when getting digest */
} VCLIB_DIGEST_FLAGS_T;

typedef struct
{
   char const *digest_name; /* should probably support aliases, too */
   int digest_length;
   int digest_ascii_length;
   void const *control;
   void (*digest_init)(VCLIB_DIGEST_STATE_T *state, void const *control);
   void (*digest_accumulate)(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length);
   uint8_t const *(*digest_finalise)(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length);
   void (*digest_get)(VCLIB_DIGEST_STATE_T const *state, uint8_t *output, VCLIB_DIGEST_FLAGS_T flags);
   void (*digest_get_ascii)(VCLIB_DIGEST_STATE_T const *state, char *output, VCLIB_DIGEST_FLAGS_T flags);
   int (*digest_verify)(VCLIB_DIGEST_STATE_T const *state, uint8_t const *digest, VCLIB_DIGEST_FLAGS_T flags);
   int (*digest_verify_ascii)(VCLIB_DIGEST_STATE_T const *state, char const *string, VCLIB_DIGEST_FLAGS_T flags);
} VCLIB_DIGEST_VTABLE_T;

struct VCLIB_DIGEST_STATE_T
{
   union
   {
      struct VCLIB_DIGEST_CRC16_WRAP_T
      {
         VCLIB_CRC16_STATE_T state;
         uint16_t crc;
         uint16_t postcond;
      } crc16;
      struct VCLIB_DIGEST_CRC32_WRAP_T
      {
         VCLIB_CRC32_STATE_T state;
         uint32_t crc;
         uint32_t postcond;
         uint64_t length;
      } crc32;
      struct VCLIB_DIGEST_CRC_WRAP_T
      {
         VCLIB_CRC_STATE_T state;
         uint32_t crc[16];
         uint32_t postcond;
      } crc;
#if defined(VCLIB_SECURE_HASHES)
      CRYPTO_SHA256_STATE_T sha256;
      CRYPTO_SHA256P_STATE_T sha256p;
      CRYPTO_CUBEHASH_STATE_T cubehash;
#endif
   } shared;
   VCLIB_DIGEST_VTABLE_T const *vtable;
   uint8_t const *result;
};

extern const VCLIB_DIGEST_VTABLE_T
   VCLIB_DIGEST_NONE_val,
   VCLIB_DIGEST_CRC16_USB_val,
   VCLIB_DIGEST_CRC16_CCITT_val,
   VCLIB_DIGEST_CRC16_CCITT_FALSE_val,
   VCLIB_DIGEST_CKSUM_val,
   VCLIB_DIGEST_CRC32_IEEE_val,
   VCLIB_DIGEST_CRC32C_val,
   VCLIB_DIGEST_CRC32D_val,
   VCLIB_DIGEST_CRC32K_val,
   VCLIB_DIGEST_CRC32Q_val,
   VCLIB_DIGEST_CRC64_ECMA_val,
   VCLIB_DIGEST_CRC64_ISO_val,
   VCLIB_DIGEST_CRC82_DARC_val,
   VCLIB_DIGEST_CRC256_MOOSE_val,
   VCLIB_DIGEST_CRC128_BRCM_val,
   VCLIB_DIGEST_CRC160_BRCM_val,
   VCLIB_DIGEST_CRC224_BRCM_val,
   VCLIB_DIGEST_CRC256_BRCM_val,
   VCLIB_DIGEST_CRC384_BRCM_val,
   VCLIB_DIGEST_CRC512_BRCM_val,
   VCLIB_DIGEST_MD5_val,
   VCLIB_DIGEST_SHA256_val,
   VCLIB_DIGEST_SHA256P_val,
   VCLIB_DIGEST_CUBEHASH_val;

#define VCLIB_DIGEST_NULL        (VCLIB_DIGEST_VTABLE_T const *)NULL
#define VCLIB_DIGEST_NONE        &VCLIB_DIGEST_NONE_val
#define VCLIB_DIGEST_CRC16_USB   &VCLIB_DIGEST_CRC16_USB_val
#define VCLIB_DIGEST_CRC16_CCITT &VCLIB_DIGEST_CRC16_CCITT_val
#define VCLIB_DIGEST_CRC16_CCITT_FALSE &VCLIB_DIGEST_CRC16_CCITT_FALSE_val
#define VCLIB_DIGEST_CRC32_IEEE  &VCLIB_DIGEST_CRC32_IEEE_val
#define VCLIB_DIGEST_CKSUM       &VCLIB_DIGEST_CKSUM_val
#define VCLIB_DIGEST_CRC32C      &VCLIB_DIGEST_CRC32C_val
#define VCLIB_DIGEST_CRC32D      &VCLIB_DIGEST_CRC32D_val
#define VCLIB_DIGEST_CRC32K      &VCLIB_DIGEST_CRC32K_val
#define VCLIB_DIGEST_CRC32Q      &VCLIB_DIGEST_CRC32Q_val
#define VCLIB_DIGEST_CRC64_ECMA  &VCLIB_DIGEST_CRC64_ECMA_val
#define VCLIB_DIGEST_CRC64_ISO   &VCLIB_DIGEST_CRC64_ISO_val
#define VCLIB_DIGEST_CRC82_DARC  &VCLIB_DIGEST_CRC82_DARC_val
#define VCLIB_DIGEST_CRC256_MOOSE &VCLIB_DIGEST_CRC256_MOOSE_val
#define VCLIB_DIGEST_CRC128_BRCM &VCLIB_DIGEST_CRC128_BRCM_val
#define VCLIB_DIGEST_CRC160_BRCM &VCLIB_DIGEST_CRC160_BRCM_val
#define VCLIB_DIGEST_CRC224_BRCM &VCLIB_DIGEST_CRC224_BRCM_val
#define VCLIB_DIGEST_CRC256_BRCM &VCLIB_DIGEST_CRC256_BRCM_val
#define VCLIB_DIGEST_CRC384_BRCM &VCLIB_DIGEST_CRC384_BRCM_val
#define VCLIB_DIGEST_CRC512_BRCM &VCLIB_DIGEST_CRC512_BRCM_val
#define VCLIB_DIGEST_MD5         &VCLIB_DIGEST_MD5_val
#define VCLIB_DIGEST_SHA256      &VCLIB_DIGEST_SHA256_val
#define VCLIB_DIGEST_SHA256P     &VCLIB_DIGEST_SHA256P_val
#define VCLIB_DIGEST_CUBEHASH    &VCLIB_DIGEST_CUBEHASH_val


/**
 * \brief A mock enum type used for algorithm selection in #vclib_digest_init().
 */
typedef VCLIB_DIGEST_VTABLE_T const *VCLIB_DIGEST_T;

/**
 * \brief Identify a digest algorithm by its ascii name.
 *
 * Given a pointer to a string beginning with the name of a digest algorithm,
 * return a #VCLIB_DIGEST_T for use in #vclib_digest_init().  The string may be
 * terminated by a variety of non-alphabetic characters (the list is not well
 * defined at this point) such that the function is fit for use in strings in
 * \c openssl form as well as strings returned from #vclib_digest_get_ascii().
 *
 * @param   string   Pointer to a string beginning with the name of a known
 *                   digest algorithm.
 *
 * @returns          The digest algorithm, if known, or \c VCLIB_DIGEST_NULL if
 *                   no algorithm could be identified.
 */
extern VCLIB_DIGEST_T vclib_digest_identify(char const *string);

/**
 * \brief Initialise a #VCLIB_DIGEST_STATE_T buffer for use by #vclib_digest().
 *
 * \b Resources:
 * The caller must have posession of the VRF during this call.
 * The VRF may be modified by this call.
 *
 * This function should not block.
 *
 * @param   state    The state buffer maintaining the accumulated state of the
 *                   block(s) of data for which the digest is to be calculated.
 * @param   algorithm The digest algorithm that will be calculated on the data
 *                   passed to #vclib_digest() when used with this \a state
 *                   buffer.
 *
 * @returns          0 on success, or -1 if the algorithm is unknown.
 */
extern int vclib_digest_init(VCLIB_DIGEST_STATE_T *state, VCLIB_DIGEST_T algorithm);

/**
 * \brief Accumulate data into \a state for digest calculation.
 *
 * \b Resources:
 * The caller must have posession of the VRF during this call.
 * The VRF may be modified by this call.
 *
 * This function should not block.
 *
 * @param   state    The state buffer maintaining the accumulated state of the
 *                   block(s) of data for which the digest is to be calculated.
 * @param   buffer   The data to be appended to the digested message.
 * @param   length   The number of bytes at \a buffer to consume.
 */
extern void vclib_digest(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length);

/**
 * \brief Finalise the digest calculation.
 *
 * Calculate the final message digest for the message that has been fed into
 * \a state.  After this call #vclib_digest() and \c vclib_digest_end() may not
 * be called with the same \a state buffer again.  Only functions to extract or
 * test the digest may be used.
 *
 * If \a length is non-zero then the data at \a buffer is accumulated into
 * the digest before finalisation.  This is provided as an optimisation for
 * short messages so that a separate call to #vclib_digest() isn't required to
 * capture overhang data before message padding for finalisation in block-based
 * algorithms.
 *
 * \b Resources:
 * The caller must have posession of the VRF during this call.
 * The VRF may be modified by this call.
 *
 * This function should not block.
 *
 * @param   state    The state buffer maintaining the accumulated state of the
 *                   block(s) of data for which the digest is to be calculated.
 * @param   buffer   The data to be appended to the digested message.
 * @param   length   The number of bytes at \a buffer to consume.
 */
extern void vclib_digest_end(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length);

/**
 * \brief Get binary length of digest.
 *
 * Return the number of bytes required to store the digest that has been
 * computed in \a state.
 */
extern int vclib_digest_length(VCLIB_DIGEST_STATE_T const *state, VCLIB_DIGEST_FLAGS_T flags);

/**
 * \brief Get ASCII length of digest.
 *
 * Return the number of chars required to store the ASCII representation of the
 * digest that has been computed in \a state, including the terminating NUL.
 *
 * This function may only be called after a call to #vclib_digest_end() on the
 * same \a state.  Some digests may result in different-length strings
 * depending on the specific message processed.
 */
extern int vclib_digest_ascii_length(VCLIB_DIGEST_STATE_T const *state, VCLIB_DIGEST_FLAGS_T flags);

/**
 * \brief Copy a message digest to another buffer.
 *
 * If the digest is a single scalar value (eg., a CRC remainder) and the
 * algorithm does not specify otherwise, then the digest will be stored in
 * big-endian format.  If the length is not a round number of bytes and the
 * algorithm does not specify otherwise, then the padding bits will be in the
 * most-significant byte (the first byte of the array).
 *
 * This function may only be called after a call to #vclib_digest_end() on the
 * same \a state.
 */
extern void vclib_digest_get(VCLIB_DIGEST_STATE_T const *state, uint8_t *output, VCLIB_DIGEST_FLAGS_T flags);

/**
 * \brief Convert digest to formatted string.
 *
 * Convert a message digest into a formatted string containing the name of the
 * algorithm and the digest in the standard ASCII form associated with that
 * digest algorithm (or a hex-dump in the general case).
 *
 * If the format is a plain hex-dump and the digest length is not a round number
 * of nybbles and the algorithm does not specify otherwise, then the padding bits
 * will be in the most-significant nybble, at the beginning of the string.
 *
 * This function may only be called after a call to #vclib_digest_end() on the
 * same \a state.
 *
 * \b Resources:
 * The caller must have posession of the VRF during this call.
 * The VRF may be modified by this call.
 *
 * This function should not block.
 */
extern void vclib_digest_get_ascii(VCLIB_DIGEST_STATE_T const *state, char *output, VCLIB_DIGEST_FLAGS_T flags);

/**
 * \brief Test computed digest against against binary dump.
 *
 * This function may only be called after a call to #vclib_digest_end() on the
 * same \a state.
 *
 * \b Resources:
 * The caller must have posession of the VRF during this call.
 * The VRF may be modified by this call.
 *
 * This function should not block.
 *
 * @returns          zero if the digests match, non-zero otherwise.
 */
extern int vclib_digest_verify(VCLIB_DIGEST_STATE_T const *state, uint8_t const *digest, VCLIB_DIGEST_FLAGS_T flags);

/**
 * \brief Test computed digest against against formatted string.
 *
 * The string should be in the same format as that produced by
 * #vclib_digest_get_ascii().
 *
 * This function may only be called after a call to #vclib_digest_end() on the
 * same \a state.
 *
 * \b Resources:
 * The caller must have posession of the VRF during this call.
 * The VRF may be modified by this call.
 *
 * This function should not block.
 *
 * @returns          zero if the digests match, non-zero otherwise.
 */
extern int vclib_digest_verify_ascii(VCLIB_DIGEST_STATE_T const *state, char const *string, VCLIB_DIGEST_FLAGS_T flags);


/* Provide a prototype for vclib_memcpy if we're not being included from the main vclib.h. */
#ifndef VCLIB_H
# ifndef _MSC_VER
extern void vclib_memcpy( void *to, const void *from, int numbytes );
# else
#  define vclib_memcpy     memcpy
# endif
#endif

#endif //ifndef VCLIB_HASH_H
