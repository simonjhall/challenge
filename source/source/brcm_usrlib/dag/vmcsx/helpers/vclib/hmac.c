/* ============================================================================
Copyright (c) 2010-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <string.h>

#include "interface/vcos/vcos_assert.h"

#include "vclib.h"
#include "vclib_hash.h"
#include "hmac.h"

/******************************************************************************
 Local defines
 *****************************************************************************/

#define PAD_INNER    0x36
#define PAD_OUTER    0x5c

#define BLOCK_SIZE_IN_BYTES_SHA256     64
#define BLOCK_SIZE_IN_BYTES_SHA256P    64
#define BLOCK_SIZE_IN_BYTES_CUBEHASH   32
#define MAX_HASH_BLOCK_BYTES           BLOCK_SIZE_IN_BYTES_SHA256

typedef struct
{
   //void (*init) (void *state);
   //void (*process) (void *state, const void *buffer, int length);
   //uint8_t const * (*end) (void *state, const void *buffer, int length);
   HMAC_HASH_TYPE_T hash_type;
   void *state_inner;
   void *state_outer;
   uint32_t blockbytes;
   uint32_t hash_size_in_bytes;
} HASHIMPL_T;

/******************************************************************************
 Static function defines
 *****************************************************************************/

static void hmac_init
   (const uint8_t *key, uint32_t key_size_in_bytes, uint8_t padding, void *state);
static void hmac_hash_init(void *state);
static void hmac_hash_process(void *state, const void *buffer, int length);
static uint8_t const * hmac_hash_end(void *state, const void *buffer, int length);

static VCLIB_SHA256_STATE_T sha256_state_inner, sha256_state_outer;
static const HASHIMPL_T hash_sha256 =
{
   HMAC_HASH_SHA256,
   &sha256_state_inner,
   &sha256_state_outer,
   BLOCK_SIZE_IN_BYTES_SHA256,
   HASH_SIZE_IN_BYTES_SHA256
};

static VCLIB_SHA256P_STATE_T sha256p_state_inner, sha256p_state_outer;
static const HASHIMPL_T hash_sha256p =
{
   HMAC_HASH_SHA256P,
   &sha256p_state_inner,
   &sha256p_state_outer,
   BLOCK_SIZE_IN_BYTES_SHA256P,
   HASH_SIZE_IN_BYTES_SHA256P
};

static VCLIB_CUBEHASH_STATE_T cubehash_state_inner, cubehash_state_outer;
static const HASHIMPL_T hash_cubehash =
{
   HMAC_HASH_CUBEHASH,
   &cubehash_state_inner,
   &cubehash_state_outer,
   BLOCK_SIZE_IN_BYTES_CUBEHASH,
   HASH_SIZE_IN_BYTES_CUBEHASH
};

static HASHIMPL_T const * const hash_list[HMAC_NUM_OF_HASH_TYPES] =
{
   &hash_sha256,
   &hash_sha256p,
   &hash_cubehash
};

static const HASHIMPL_T *hashimpl;

/******************************************************************************
 Global functions
 *****************************************************************************/

uint32_t hmac_verify_data(
   HMAC_HASH_TYPE_T hash_type,
   const uint8_t *key, uint32_t key_size_in_bytes,
   const uint8_t *data, uint32_t data_size_in_bytes,
   const uint8_t *signature)
// Function returns 0 (success) if the hash value for the hash type, key and
// data specified matches the given signature.
{
   uint32_t failure = -1;
   uint8_t hash[MAX_HASH_SIZE_IN_BYTES];

   failure = hmac_compute_hash
      (hash_type, key, key_size_in_bytes, data, data_size_in_bytes, hash);

   vcos_assert(!failure);

   if(vcos_verify(signature != NULL))
      {failure = memcmp(hash, signature, hashimpl->hash_size_in_bytes);}
   else
      {failure = -1;}

   return failure;
} // hmac_verify_data()

//------------------------------------------------------------------------------

/* The HMAC on key K and message m is defined as:
 *
 * HMAC(K, m) = H((K ^ pad_outer) + H((K ^ pad_inner) + m))
 *
 * where ^ is XOR, + is concatenation, and H() is a hash function.
 *
 * See: http://en.wikipedia.org/wiki/Hmac
 */

uint32_t hmac_compute_hash(
   HMAC_HASH_TYPE_T hash_type,
   const uint8_t *key, uint32_t key_size_in_bytes,
   const uint8_t *data, uint32_t data_size_in_bytes,
   uint8_t hash[])
// For specified hash type, key and data, compute the hash, and copy it into
// hash[] (which must be big enough for the requested hash type). Returns 0
// if successful.
{
   const uint8_t *hash_local;
   uint32_t failure = 1;

   hashimpl = hash_list[hash_type];

   //----------------------------------

   vclib_obtain_VRF(1);

   // Process the key
   hmac_init(key, key_size_in_bytes, PAD_INNER, hashimpl->state_inner);

   // Compute the inner hash
   hash_local = hmac_hash_end(hashimpl->state_inner, data, data_size_in_bytes);

   //----------------------------------

   // Initialise the outer hash state
   hmac_init(key, key_size_in_bytes, PAD_OUTER, hashimpl->state_outer);

   // Feed the inner hash result to the outer hash
   hash_local = hmac_hash_end(hashimpl->state_outer, hash_local, hashimpl->hash_size_in_bytes);

   // Copy local copy of hash to array passed down from above
   memcpy(hash, hash_local, hashimpl->hash_size_in_bytes);

   vclib_release_VRF();

   //----------------------------------

   failure = 0;
   return failure;
} // hmac_compute_hash()

/******************************************************************************
 Static functions
 *****************************************************************************/

static void hmac_init
   (const uint8_t *key, uint32_t key_size_in_bytes, uint8_t padding, void *state)
{
   uint8_t key_block[MAX_HASH_BLOCK_BYTES];
   uint32_t i;

   vcos_assert(key_size_in_bytes < hashimpl->blockbytes);

   /* Initialise the hash state */
   hmac_hash_init(state);

   /* Generate the padding */
   for (i = 0; i < key_size_in_bytes; i++)
   {
      key_block[i] = key[i] ^ padding;
   }
   /* Fill the remainder with padding */
   if (key_size_in_bytes < hashimpl->blockbytes)
      memset(&key_block[key_size_in_bytes], padding, hashimpl->blockbytes - key_size_in_bytes);

   /* Hash this first block */
   hmac_hash_process(state, key_block, hashimpl->blockbytes);

} // hmac_init()

//------------------------------------------------------------------------------

static void hmac_hash_init(void *state)
{
   switch(hashimpl->hash_type)
   {
      case HMAC_HASH_SHA256: vclib_sha256_init(state); break;
      case HMAC_HASH_SHA256P: vclib_sha256p_init(state); break;
      case HMAC_HASH_CUBEHASH: vclib_cubehash_init(state,
         16, BLOCK_SIZE_IN_BYTES_CUBEHASH, HASH_SIZE_IN_BYTES_CUBEHASH); break;
      default: vcos_assert(0);
   } // switch
} // hmac_hash_init()

//------------------------------------------------------------------------------

static void hmac_hash_process(void *state, const void *buffer, int length)
{
   switch(hashimpl->hash_type)
   {
      case HMAC_HASH_SHA256: vclib_sha256(state, buffer, length); break;
      case HMAC_HASH_SHA256P: vclib_sha256p(state, buffer, length); break;
      case HMAC_HASH_CUBEHASH: vclib_cubehash(state, buffer, length); break;
      default: vcos_assert(0);
   } // switch
} // hmac_hash_process()

//------------------------------------------------------------------------------

static uint8_t const * hmac_hash_end(void *state, const void *buffer, int length)
{
   switch(hashimpl->hash_type)
   {
      case HMAC_HASH_SHA256: return vclib_sha256_end(state, buffer, length);
      case HMAC_HASH_SHA256P: return vclib_sha256p_end(state, buffer, length);
      case HMAC_HASH_CUBEHASH: return vclib_cubehash_end(state, buffer, length);
      default: vcos_assert(0); return NULL;
   } // switch
} // hmac_hash_end()

/********************************* end of file ********************************/
