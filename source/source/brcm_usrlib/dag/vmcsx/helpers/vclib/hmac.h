/* ============================================================================
Copyright (c) 2010-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef HMAC_H
#define HMAC_H

#include "vcinclude/common.h" //Header file for basic type definitions etc...

/******************************************************************************
 Global defines
 *****************************************************************************/

#define HASH_SIZE_IN_BYTES_SHA256   32
#define HASH_SIZE_IN_BYTES_SHA256P  32
#define HASH_SIZE_IN_BYTES_CUBEHASH 32
#define MAX_HASH_SIZE_IN_BYTES      HASH_SIZE_IN_BYTES_CUBEHASH

typedef enum
{
   HMAC_HASH_SHA256,
   HMAC_HASH_SHA256P,
   HMAC_HASH_CUBEHASH,

   HMAC_NUM_OF_HASH_TYPES
} HMAC_HASH_TYPE_T;

#define HMAC_HASH_TYPE_FIRST  HMAC_HASH_SHA256
#define HMAC_HASH_TYPE_LAST   HMAC_HASH_CUBEHASH

/******************************************************************************
 Global Funcs
 *****************************************************************************/

uint32_t hmac_verify_data(
   HMAC_HASH_TYPE_T hash_type,
   const uint8_t *key, uint32_t key_size_in_bytes,
   const uint8_t *data, uint32_t data_size_in_bytes,
   const uint8_t *signature);

uint32_t hmac_compute_hash(
   HMAC_HASH_TYPE_T hash_type,
   const uint8_t *key, uint32_t key_size_in_bytes,
   const uint8_t *data, uint32_t data_size_in_bytes,
   uint8_t hash[]);

#endif /* HMAC_H */

/***************************** End of file ********************************/
