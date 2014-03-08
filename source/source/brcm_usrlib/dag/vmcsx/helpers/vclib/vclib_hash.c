/* ============================================================================
Copyright (c) 2009-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <stdio.h> /* just for sprintf() */
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "helpers/vclib/vclib.h"
#include "helpers/vclib/vclib_hash.h"
#include "vcinclude/common.h"
#include "interface/vcos/vcos_assert.h"


static void digest_none_init(VCLIB_DIGEST_STATE_T *state, void const *control);
static void digest_none_accumulate(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length);
static uint8_t const *digest_none_finalise(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length);

static void digest_crc16_init_usb(VCLIB_DIGEST_STATE_T *state, void const *control);
static void digest_crc16_init_ccitt(VCLIB_DIGEST_STATE_T *state, void const *control);
static void digest_crc16_init_ccitt_false(VCLIB_DIGEST_STATE_T *state, void const *control);
static void digest_crc16_accumulate(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length);
static uint8_t const *digest_crc16_end(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length);

static void digest_cksum_init(VCLIB_DIGEST_STATE_T *state, void const *control);
static void digest_cksum_accumulate(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length);
static uint8_t const *digest_cksum_end(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length);

static void digest_crc_init(VCLIB_DIGEST_STATE_T *state, void const *control);
static void digest_crc_accumulate(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length);
static uint8_t const *digest_crc_end(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length);
#if defined(VCLIB_SECURE_HASHES)
static void digest_cubehash_init_16_32_256(VCLIB_DIGEST_STATE_T *state, void const *control);
#endif

static void digest_get_rev(VCLIB_DIGEST_STATE_T const *state, uint8_t *output, VCLIB_DIGEST_FLAGS_T flags);
static void digest_get_ascii_fwd(VCLIB_DIGEST_STATE_T const *state, char *output, VCLIB_DIGEST_FLAGS_T flags);
static void digest_get_ascii_rev(VCLIB_DIGEST_STATE_T const *state, char *output, VCLIB_DIGEST_FLAGS_T flags);
static int digest_verify_rev(VCLIB_DIGEST_STATE_T const *state, uint8_t const *digest, VCLIB_DIGEST_FLAGS_T flags);
static int digest_verify_ascii_fwd(VCLIB_DIGEST_STATE_T const *state, char const *digest, VCLIB_DIGEST_FLAGS_T flags);
static int digest_verify_ascii_rev(VCLIB_DIGEST_STATE_T const *state, char const *digest, VCLIB_DIGEST_FLAGS_T flags);

static void digest_cksum_get(VCLIB_DIGEST_STATE_T const *state, uint8_t *output, VCLIB_DIGEST_FLAGS_T flags);
static void digest_cksum_get_ascii(VCLIB_DIGEST_STATE_T const *state, char *output, VCLIB_DIGEST_FLAGS_T flags);
static int digest_cksum_verify(VCLIB_DIGEST_STATE_T const *state, uint8_t const *digest, VCLIB_DIGEST_FLAGS_T flags);
static int digest_cksum_verify_ascii(VCLIB_DIGEST_STATE_T const *state, char const *digest, VCLIB_DIGEST_FLAGS_T flags);


/* Helpful link: <http://regregex.bbcmicro.net/crc-catalogue.htm> */

static const uint32_t crc32_ieee_control[] = {
   (uint32_t)-32, 0xFFFFFFFF, 0xFFFFFFFF, 0x04c11db7 };

static const uint32_t crc32_c_control[] = {
   (uint32_t)-32, 0xFFFFFFFF, 0xFFFFFFFF, 0x1edc6f41 };

static const uint32_t crc32_d_control[] = {
   (uint32_t)-32, 0xFFFFFFFF, 0xFFFFFFFF, 0xa833982b };

static const uint32_t crc32_k_control[] = { /* TODO: confirm endian */
   (uint32_t)-32, 0xFFFFFFFF, 0xFFFFFFFF, 0x741b8cd7 };

static const uint32_t crc32_q_control[] = {
   (uint32_t)32, 0, 0, 0x814141ab };

static const uint32_t crc64_ecma_control[] = {
   (uint32_t)64, 0, 0,
   0xA9EA3693, 0x42F0E1EB };

static const uint32_t crc64_iso_control[] = {
   (uint32_t)-64, 0, 0,
   0x0000001b, 0x00000000 };

static const uint32_t crc82_darc_control[] = {
   (uint32_t)-82, 0, 0,
   0x01440411, 0x01110114, 0x0308C };

static const uint32_t crc256_moose_control[] = {
   (uint32_t)-256, 0xFFFFFFFF, 0,
   0x7C224741, 0x5C1C04C6, 0x05451D04, 0xC5895785,
   0x0C599533, 0x1C0A04A3, 0x5541392C, 0xE4BF15C5 };

static const uint32_t crc128_brcm_control[] = {
   (uint32_t)-128, 0xFFFFFFFF, 0xFFFFFFFF,
   0xca26c33f, 0x6f695167, 0xd1dd5128, 0x92f88c86 };

static const uint32_t crc160_brcm_control[] = {
   (uint32_t)-160, 0xFFFFFFFF, 0xFFFFFFFF,
   0xcd9ed1e3, 0x271c8f57, 0x25115fd3, 0x02a9342f,
   0xdee24a86 };

static const uint32_t crc224_brcm_control[] = {
   (uint32_t)-224, 0xFFFFFFFF, 0xFFFFFFFF,
   0xbd44bcff, 0xcfbf72aa, 0xdb27d013, 0x7fdd9c52,
   0xfee71ce9, 0x66d0e9c4, 0x828c58df };

static const uint32_t crc256_brcm_control[] = {
   (uint32_t)-256, 0xFFFFFFFF, 0xFFFFFFFF,
   0xbb98691f, 0xe3084ee5, 0x297f0222, 0xe63e2a3c,
   0x64295e4e, 0x009066c5, 0xa3ea7e3a, 0x9a567623 };

static const uint32_t crc384_brcm_control[] = {
   (uint32_t)-384, 0xFFFFFFFF, 0xFFFFFFFF,
   0xa7c94aa5, 0x3d9044d2, 0xa147a2ff, 0xf806ec40,
   0xbde6ae7b, 0xf80c2970, 0xb505e44e, 0x39d5005e,
   0x6d7bc684, 0x027e8b1b, 0x1337a53a, 0x314fc574 };

static const uint32_t crc512_brcm_control[] = {
   (uint32_t)-512, 0xFFFFFFFF, 0xFFFFFFFF,
   0x61650735, 0x982d1157, 0xd95764fe, 0x97eca674,
   0xaa2ca03b, 0x11a107e5, 0x191cdc66, 0xbc9ff50c,
   0x64571e59, 0x1cbbb718, 0xa6c63e25, 0x92720535,
   0xf99144f0, 0x60407259, 0x3bf1f811, 0xa836f163 };

const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_NONE_val =
{
   "NONE", 0, 0, NULL,
   digest_none_init, digest_none_accumulate, digest_none_finalise,
   digest_get_rev, digest_get_ascii_rev, digest_verify_rev, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC16_USB_val =
{
   "CRC-16-USB", 16, 0, NULL,
   digest_crc16_init_usb, digest_crc16_accumulate, digest_crc16_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC16_CCITT_val =
{
   "CRC-16-CCITT", 16, 0, NULL,
   digest_crc16_init_ccitt, digest_crc16_accumulate, digest_crc16_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC16_CCITT_FALSE_val =
{
   "CRC-16-CCITT-FALSE", 16, 0, NULL,
   digest_crc16_init_ccitt_false, digest_crc16_accumulate, digest_crc16_end,
   digest_get_rev, digest_get_ascii_rev, digest_verify_rev, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CKSUM_val =
{
   "CKSUM", 96, 10+1+20 /* strlen("4294967296 18446744073709551616") */,
   NULL,
   digest_cksum_init, digest_cksum_accumulate, digest_cksum_end,
   digest_cksum_get, digest_cksum_get_ascii,
   digest_cksum_verify, digest_cksum_verify_ascii
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC32_IEEE_val =
{
   "CRC-32-IEEE", 32, 0, crc32_ieee_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC32C_val =
{
   "CRC-32C", 32, 0, crc32_c_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC32D_val =
{
   "CRC-32D", 32, 0, crc32_d_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC32K_val =
{
   "unconfirmed:CRC-32K", 32, 0, crc32_k_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC32Q_val =
{
   "CRC-32Q", 32, 0, crc32_q_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   digest_get_rev, digest_get_ascii_rev, digest_verify_rev, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC64_ECMA_val =
{
   "CRC-64-ECMA", 64, 0, crc64_ecma_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   digest_get_rev, digest_get_ascii_rev, digest_verify_rev, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC64_ISO_val =
{
   "CRC-64-ISO", 64, 0, crc64_iso_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC82_DARC_val =
{
   "CRC-82-DARC", 82, 0, crc82_darc_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC256_MOOSE_val =
{
   "CRC-256-MOOSE", 256, 0, crc256_moose_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC128_BRCM_val =
{
   "CRC-128-BRCM", 128, 0, crc128_brcm_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC160_BRCM_val =
{
   "CRC-160-BRCM", 160, 0, crc160_brcm_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC224_BRCM_val =
{
   "CRC-224-BRCM", 224, 0, crc224_brcm_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC256_BRCM_val =
{
   "CRC-256-BRCM", 256, 0, crc256_brcm_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC384_BRCM_val =
{
   "CRC-384-BRCM", 384, 0, crc384_brcm_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CRC512_BRCM_val =
{
   "CRC-512-BRCM", 512, 0, crc512_brcm_control,
   digest_crc_init, digest_crc_accumulate, digest_crc_end,
   NULL, digest_get_ascii_rev, NULL, digest_verify_ascii_rev
};

#if defined(VCLIB_SECURE_HASHES)
#if 0 /* not implemented */
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_MD5_val =
{
   "MD5", 128, 0, NULL,
   (void (*)(VCLIB_DIGEST_STATE_T *, void const *))crypto_md5_init,
   (void (*)(VCLIB_DIGEST_STATE_T *, void const *, int))crypto_md5,
   (uint8_t const *(*)(VCLIB_DIGEST_STATE_T *,void const *, int))crypto_md5_end
};
#endif
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_SHA256_val =
{
   "SHA256", 256, 0, NULL,
   (void (*)(VCLIB_DIGEST_STATE_T *, void const *))crypto_sha256_init,
   (void (*)(VCLIB_DIGEST_STATE_T *, void const *, int))crypto_sha256,
   (uint8_t const *(*)(VCLIB_DIGEST_STATE_T *,void const *, int))crypto_sha256_end
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_SHA256P_val =
{
   "SHA256P", 256, 0, NULL,
   (void (*)(VCLIB_DIGEST_STATE_T *, void const *))crypto_sha256p_init,
   (void (*)(VCLIB_DIGEST_STATE_T *, void const *, int))crypto_sha256p,
   (uint8_t const *(*)(VCLIB_DIGEST_STATE_T *,void const *, int))crypto_sha256p_end
};
const VCLIB_DIGEST_VTABLE_T VCLIB_DIGEST_CUBEHASH_val =
{
   "CUBEHASH-16/32-256", 256, 0, NULL,
   digest_cubehash_init_16_32_256,
   (void (*)(VCLIB_DIGEST_STATE_T *, void const *, int))crypto_cubehash,
   (uint8_t const *(*)(VCLIB_DIGEST_STATE_T *,void const *, int))crypto_cubehash_end
};
#endif

/******************************************************************************
Public functions written in this module.
******************************************************************************/


/******************************************************************************
Private stuff in this module.
******************************************************************************/


/*******************************************************************************
Public functions
*******************************************************************************/


/*******************************************************************************
NAME
   vclib_digest_identify

SYNOPSIS
   VCLIB_DIGEST_VTABLE_T const *vclib_digest_identify(char const *string)

FUNCTION
   bleh

RETURNS
   -
*******************************************************************************/

VCLIB_DIGEST_VTABLE_T const *vclib_digest_identify(char const *string)
{
   static const VCLIB_DIGEST_VTABLE_T *known_digest[] =
   {
      VCLIB_DIGEST_NONE,
      VCLIB_DIGEST_CRC16_USB,
      VCLIB_DIGEST_CRC16_CCITT,
      VCLIB_DIGEST_CRC16_CCITT_FALSE,
      VCLIB_DIGEST_CRC32_IEEE,
      VCLIB_DIGEST_CRC32C,
      VCLIB_DIGEST_CRC32D,
      VCLIB_DIGEST_CRC32K,
      VCLIB_DIGEST_CRC32Q,
      VCLIB_DIGEST_CRC64_ECMA,
      VCLIB_DIGEST_CRC64_ISO,
      VCLIB_DIGEST_CRC82_DARC,
      VCLIB_DIGEST_CRC256_MOOSE,
#if defined(VCLIB_SECURE_HASHES)
#if 0 /* not implemented */
      VCLIB_DIGEST_MD5,
#endif
      VCLIB_DIGEST_SHA256,
      VCLIB_DIGEST_SHA256P,
      VCLIB_DIGEST_CUBEHASH
#endif
   };
   int i;

   for (i = 0; i < sizeof(known_digest) / sizeof(*known_digest); i++)
   {
      int len = strlen(known_digest[i]->digest_name);
      if (strncmp(string, known_digest[i]->digest_name, len) == 0 && strchr(" !|(){}[]:;#", string[len]) != NULL)
         return known_digest[i];
   }
   return NULL;
}


int vclib_digest_init(VCLIB_DIGEST_STATE_T *state, VCLIB_DIGEST_T algorithm)
{
   vcos_assert(state != NULL);
   state->vtable = algorithm != VCLIB_DIGEST_NULL ? algorithm : VCLIB_DIGEST_NONE;
   state->result = NULL;
   algorithm->digest_init(state, state->vtable->control);
   return -(algorithm == VCLIB_DIGEST_NULL);
}


void vclib_digest(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length)
{
   vcos_assert(state != NULL && state->result == NULL);
   state->vtable->digest_accumulate(state, buffer, length);
}


void vclib_digest_end(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length)
{
   vcos_assert(state != NULL && state->result == NULL);
   state->result = state->vtable->digest_finalise(state, buffer, length);
   vcos_assert(state->result != NULL);
}


int vclib_digest_length(VCLIB_DIGEST_STATE_T const *state, VCLIB_DIGEST_FLAGS_T flags)
{
   vcos_demand(flags == VCLIB_DIGEST_RAW);
   return state->vtable->digest_length + 7 >> 3;
}

int vclib_digest_ascii_length(VCLIB_DIGEST_STATE_T const *state, VCLIB_DIGEST_FLAGS_T flags)
{
   int length;
   vcos_assert(state != NULL && state->result != NULL);

   if (state->vtable->digest_ascii_length != 0)
      length = state->vtable->digest_ascii_length;
   else
      length = state->vtable->digest_length + 3 >> 2;

   if (flags & VCLIB_DIGEST_ID)
      length += strlen(state->vtable->digest_name) + 1;

   return length + 1;
}

void vclib_digest_get(VCLIB_DIGEST_STATE_T const *state, uint8_t *output, VCLIB_DIGEST_FLAGS_T flags)
{
   vcos_assert(state != NULL && state->result != NULL);
   vcos_demand(flags == VCLIB_DIGEST_RAW);

   if (state->vtable->digest_get != NULL)
      state->vtable->digest_get(state, output, flags);
   else
      vclib_memcpy(output, state->result, state->vtable->digest_length + 7 >> 3);
}

void vclib_digest_get_ascii(VCLIB_DIGEST_STATE_T const *state, char *output, VCLIB_DIGEST_FLAGS_T flags)
{
   vcos_assert(state != NULL && state->result != NULL);

   if (flags & VCLIB_DIGEST_ID)
   {
      strcpy(output, state->vtable->digest_name);
      output = strchr(output, '\0');
      *output++ = '!';
      flags &= ~VCLIB_DIGEST_ID;
   }

   if (state->vtable->digest_get_ascii != NULL)
      state->vtable->digest_get_ascii(state, output, flags);
   else
      digest_get_ascii_fwd(state, output, flags);
}

int vclib_digest_verify(VCLIB_DIGEST_STATE_T const *state, uint8_t const *digest, VCLIB_DIGEST_FLAGS_T flags)
{
   vcos_assert(state != NULL && state->result != NULL);
   vcos_demand(flags == VCLIB_DIGEST_RAW);

   if (state->vtable->digest_verify != NULL)
      return state->vtable->digest_verify(state, digest, flags);
   return vclib_memcmp(state->result, digest, state->vtable->digest_length + 7 >> 3);
}

int vclib_digest_verify_ascii(VCLIB_DIGEST_STATE_T const *state, char const *string, VCLIB_DIGEST_FLAGS_T flags)
{
   int len;

   vcos_assert(state != NULL && state->result != NULL);

   len = strlen(state->vtable->digest_name);
   if (strncmp(string, state->vtable->digest_name, len) != 0)
   {
      /* wrong algorithm or malformed (unrecognised) digest string */
      return -1;
   }
   string += len;
   if (*string++ != '!')
   {
      /* wrong algorithm or malformed (unrecognised) digest string */
      return -1;
   }

   flags &= ~VCLIB_DIGEST_ID;

   if (state->vtable->digest_verify_ascii != NULL)
      return state->vtable->digest_verify_ascii(state, string, flags);
   return digest_verify_ascii_fwd(state, string, flags);
}


/*******************************************************************************
Private functions
*******************************************************************************/

static void digest_none_init(VCLIB_DIGEST_STATE_T *state, void const *control)
{
   (void)state;
}

static void digest_none_accumulate(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length)
{
   (void)state;
   (void)buffer;
   (void)length;
}

static uint8_t const *digest_none_finalise(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length)
{
   (void)buffer;
   (void)length;

   return (void const *)state;
}

static void digest_crc16_init_usb(VCLIB_DIGEST_STATE_T *state, void const *control)
{
   state->shared.crc16.crc = (uint16_t)-1;
   state->shared.crc16.postcond = (uint16_t)-1;
   vclib_crc16_init_rev(&state->shared.crc16.state, 0x8005);
}

static void digest_crc16_init_ccitt(VCLIB_DIGEST_STATE_T *state, void const *control)
{
   state->shared.crc16.crc = 0;
   state->shared.crc16.postcond = 0;
   vclib_crc16_init_rev(&state->shared.crc16.state, 0x1021);
}
static void digest_crc16_init_ccitt_false(VCLIB_DIGEST_STATE_T *state, void const *control)
{
   state->shared.crc16.crc = (uint16_t)-1;
   state->shared.crc16.postcond = 0;
   vclib_crc16_init(&state->shared.crc16.state, 0x1021);
}

static void digest_crc16_accumulate(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length)
{
   state->shared.crc16.crc = vclib_crc16(&state->shared.crc16.state, state->shared.crc16.crc, buffer, length);
}

static uint8_t const *digest_crc16_end(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length)
{
   unsigned int crc = state->shared.crc16.crc;

   if (length)
      crc = vclib_crc16(&state->shared.crc16.state, crc, buffer, length);

   state->shared.crc16.crc = crc ^ state->shared.crc16.postcond;
   return (void *)&state->shared.crc16.crc;
}

static void digest_cksum_init(VCLIB_DIGEST_STATE_T *state, void const *control)
{
   state->shared.crc32.crc = 0;
   state->shared.crc32.postcond = (uint32_t)-1;
   state->shared.crc32.length = 0;
   vclib_crc32_init(&state->shared.crc32.state, 0x04c11db7);
}

static void digest_cksum_accumulate(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length)
{
   state->shared.crc32.length += length;
   state->shared.crc32.crc = vclib_crc32(&state->shared.crc32.state, state->shared.crc32.crc, buffer, length);
}

static uint8_t const *digest_cksum_end(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length)
{
   uint32_t crc = state->shared.crc32.crc;

   if (length)
   {
      state->shared.crc32.length += length;
      crc = vclib_crc32(&state->shared.crc32.state, crc, buffer, length);
   }

   for (length = 0; state->shared.crc32.length >> length * 8 > 0; length++)
      continue;
   /* cheeky little-endian assumption: */
   crc = vclib_crc32(&state->shared.crc32.state, crc, &state->shared.crc32.length, length);

   state->shared.crc32.crc = crc ^ state->shared.crc32.postcond;

   return (void *)state;
}

static void digest_crc_init(VCLIB_DIGEST_STATE_T *state, void const *control)
{
   uint32_t const *words = control;
   _vasm("vmov H32(0,0), %r", words[1]);
   _vasm("vst H32(0,0), (%r)", state->shared.crc.crc);
   state->shared.crc.postcond = words[2];
   if ((int32_t)words[0] < 0)
      vclib_crc_init_rev(&state->shared.crc.state, words + 3, -words[0]);
   else
      vclib_crc_init(&state->shared.crc.state, words + 3, words[0]);
}

static void digest_crc_accumulate(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length)
{
   vclib_crc(&state->shared.crc.state, state->shared.crc.crc, buffer, length);
}

static uint8_t const *digest_crc_end(VCLIB_DIGEST_STATE_T *state, void const *buffer, int length)
{
   if (length)
      vclib_crc(&state->shared.crc.state, state->shared.crc.crc, buffer, length);
   _vasm("vld H32(32,0), (%r)", state->shared.crc.crc);
   _vasm("veor H32(32,0), H32(32,0), %r", state->shared.crc.postcond);
   _vasm("vst H32(32,0), (%r)", state->shared.crc.crc);

   return (void *)state->shared.crc.crc;
}

#if defined(VCLIB_SECURE_HASHES)
static void digest_cubehash_init_16_32_256(VCLIB_DIGEST_STATE_T *state, void const *control)
{
   crypto_cubehash_init(&state->shared.cubehash, 16, 32, 256);
}
#endif


static void digest_get_rev(VCLIB_DIGEST_STATE_T const *state, uint8_t *output, VCLIB_DIGEST_FLAGS_T flags)
{
#if 0 /* needs generalisation */
   int tail;
   _vasm("vld H32(32,0), (%r)", state->result);
   tail = -(state->vtable->digest_length + 7 >> 3) & 3;
#if 0
   if (tail)
   {
      _vasm("vbitplanes -, 0x7fff         CLRA SETF");
      _vasm("vlsr       -, H32(32,1), %r  SACC IFNZ", 32 - tail * 8);
      _vasm("vshl H32(32,0), H32(32,0), %r SACC", tail * 8);
   }
#endif
   _vasm("vbitplanes H16(32,0),  H16(32,0)");
   _vasm("vbitplanes H16(32,32), H16(32,32)");
   _vasm("vbitrev    H16(32,0),  H16(32,0),  %r", state->vtable->digest_length + 31 >> 5);
   _vasm("vbitrev    H16(32,32), H16(32,32), %r", state->vtable->digest_length + 31 >> 5);
   _vasm("vbitplanes H16(32,0),  H16(32,0)");
   _vasm("vbitplanes H16(32,32), H16(32,32)");
   _vasm("v16ror     -,          H16(32,0),  8 CLRA SACCH");
   _vasm("v16ror     H32(32,0),  H16(32,32), 8      UACC");
   /* set output mask, etc. */
   _vasm("vst        H32(32,0), (%r)", output);
#else
   uint8_t const *ptr = state->result;
   int i = state->vtable->digest_length + 7 >> 3;

   while (--i >= 0)
      *output++ = ptr[i];
#endif
}


static void digest_get_ascii_fwd(VCLIB_DIGEST_STATE_T const *state, char *output, VCLIB_DIGEST_FLAGS_T flags)
{
   static const char *hexdigits = "0123456789abcdef";
   int i = state->vtable->digest_length + 3 >> 2;
   uint8_t const *ptr = state->result;

   if (i & 1)
      *output++ = hexdigits[*ptr++ & 15];
   i >>= 1;
   while (--i >= 0)
   {
      int byte = *ptr++;
      *output++ = hexdigits[byte >> 4];
      *output++ = hexdigits[byte & 15];
   }
   output[0] = '\0';
}


static void digest_get_ascii_rev(VCLIB_DIGEST_STATE_T const *state, char *output, VCLIB_DIGEST_FLAGS_T flags)
{
   static const char *hexdigits = "0123456789abcdef";
   int i = state->vtable->digest_length + 3 >> 2;
   uint8_t const *ptr = state->result;

   if (i & 1)
      *output++ = hexdigits[ptr[i >> 1] & 15];
   i >>= 1;
   while (--i >= 0)
   {
      int byte = ptr[i];
      *output++ = hexdigits[byte >> 4];
      *output++ = hexdigits[byte & 15];
   }
   output[0] = '\0';
}

static int digest_verify_rev(VCLIB_DIGEST_STATE_T const *state, uint8_t const *digest, VCLIB_DIGEST_FLAGS_T flags)
{
   uint8_t const *ptr = state->result;
   int i = state->vtable->digest_length + 7 >> 3;

   while (--i >= 0)
      if (*digest++ != ptr[i])
         return 1;
   return 0;
}

static int digest_verify_ascii_fwd(VCLIB_DIGEST_STATE_T const *state, char const *digest, VCLIB_DIGEST_FLAGS_T flags)
{
   int i = state->vtable->digest_length + 3 >> 2;
   uint8_t const *ptr = state->result;

   if (i & 1)
   {
      int c = *digest++;
      if ('a' <= c && c <= 'f') c += 'A' - 'a';
      if ('A' <= c && c <= 'F') c += '0' + 10 - 'A';
      c -= '0';
      if (c > 15u) return -1;
      if (*ptr++ != c)
         return 1;
   }
   i >>= 1;
   while (--i >= 0)
   {
      int c, d;
      c = *digest++;
      if ('a' <= c && c <= 'f') c += 'A' - 'a';
      if ('A' <= c && c <= 'F') c += '0' - 'A' + 10;
      c -= '0';
      if (c > 15u) return -1;
      d = *digest++;
      if ('a' <= d && d <= 'f') d += 'A' - 'a';
      if ('A' <= d && d <= 'F') d += '0' - 'A' + 10;
      d -= '0';
      if (d > 15u) return -1;

      if (*ptr++ != (c * 16 + d))
         return 1;
   }

   {
      int c = *digest;
      /* if there are more hex digits left in the string then we're obviously
       * on the wrong track -- we'll tolerate other characters (generally
       * whitespace or punctuation) so we don't run into too much trouble with
       * overly pedantic tests.
       */
      if ('a' <= c && 'f') return -1;
      if ('A' <= c && 'F') return -1;
      if ('0' <= c && '9') return -1;
   }
   return 0;
}


static int digest_verify_ascii_rev(VCLIB_DIGEST_STATE_T const *state, char const *digest, VCLIB_DIGEST_FLAGS_T flags)
{
   int i = state->vtable->digest_length + 3 >> 2;
   uint8_t const *ptr = state->result;

   if (i & 1)
   {
      int c = *digest++;
      if ('a' <= c && c <= 'f') c += 'A' - 'a';
      if ('A' <= c && c <= 'F') c += '0' + 10 - 'A';
      c -= '0';
      if (c > 15u) return -1;
      if (ptr[i >> 1] != c)
         return 1;
   }
   i >>= 1;
   while (--i >= 0)
   {
      int c, d;
      c = *digest++;
      if ('a' <= c && c <= 'f') c += 'A' - 'a';
      if ('A' <= c && c <= 'F') c += '0' - 'A' + 10;
      c -= '0';
      if (c > 15u) return -1;
      d = *digest++;
      if ('a' <= d && d <= 'f') d += 'A' - 'a';
      if ('A' <= d && d <= 'F') d += '0' - 'A' + 10;
      d -= '0';
      if (d > 15u) return -1;

      if (ptr[i] != (c * 16 + d))
         return 1;
   }

   {
      int c = *digest;
      /* if there are more hex digits left in the string then we're obviously
       * on the wrong track -- we'll tolerate other characters (generally
       * whitespace or punctuation) so we don't run into too much trouble with
       * overly pedantic tests.
       */
      if ('a' <= c && 'f') return -1;
      if ('A' <= c && 'F') return -1;
      if ('0' <= c && '9') return -1;
   }
   return 0;
}


static void digest_cksum_get(VCLIB_DIGEST_STATE_T const *state, uint8_t *output, VCLIB_DIGEST_FLAGS_T flags)
{
   uint32_t tmp;
   
   tmp = state->shared.crc32.crc;
   output[0] = 255 & tmp >> 24;
   output[1] = 255 & tmp >> 16;
   output[2] = 255 & tmp >> 8;
   output[3] = 255 & tmp >> 0;
   tmp = (uint32_t)(state->shared.crc32.length >> 32);
   output[4] = 255 & tmp >> 24;
   output[5] = 255 & tmp >> 16;
   output[6] = 255 & tmp >> 8;
   output[7] = 255 & tmp >> 0;
   tmp = (uint32_t)state->shared.crc32.length;
   output[8] =  255 & tmp >> 24;
   output[9] =  255 & tmp >> 16;
   output[10] = 255 & tmp >> 8;
   output[11] = 255 & tmp >> 0;
}

static void digest_cksum_get_ascii(VCLIB_DIGEST_STATE_T const *state, char *output, VCLIB_DIGEST_FLAGS_T flags)
{
   int verify;
   verify = sprintf(output, "%u %llu", state->shared.crc32.crc, state->shared.crc32.length);
   vcos_assert(verify < state->vtable->digest_ascii_length);
}

static int digest_cksum_verify(VCLIB_DIGEST_STATE_T const *state, uint8_t const *digest, VCLIB_DIGEST_FLAGS_T flags)
{
   uint32_t tmp;
   
   tmp = digest[0] << 24
       | digest[1] << 16
       | digest[2] << 8
       | digest[3] << 0;
   if (tmp != state->shared.crc32.crc)
      return 1;
   tmp = digest[4] << 24
       | digest[5] << 16
       | digest[6] << 8
       | digest[7] << 0;
   if (tmp != (uint32_t)(state->shared.crc32.length >> 32))
      return 1;
   tmp = digest[8]  << 24
       | digest[9]  << 16
       | digest[10] << 8
       | digest[11] << 0;
   if (tmp != (uint32_t)state->shared.crc32.length)
      return 1;
   return 0;
}

static int digest_cksum_verify_ascii(VCLIB_DIGEST_STATE_T const *state, char const *digest, VCLIB_DIGEST_FLAGS_T flags)
{
   char *lptr;
   uint32_t crc;
   uint64_t length;

   crc = strtoul(digest, &lptr, 10);
   length = strtoull(lptr, &lptr, 10);
   if (lptr == digest)
      return -1;
   if (crc != state->shared.crc32.crc || length != state->shared.crc32.length)
      return 1;
   return 0;
}
