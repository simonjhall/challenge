/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef KHRN_CLIENT_CACHE_H
#define KHRN_CLIENT_CACHE_H

//#define WORKAROUND_HW2551     // define to pad header structure to 32 bytes

#include "middleware/khronos/common/khrn_map.h"


typedef struct {

   KHRN_MAP_T map;
} KHRN_CACHE_T;

#define CACHE_LOG2_BLOCK_SIZE    6
#define CACHE_MAX_DEPTH          16

#define CACHE_SIG_ATTRIB_0    0
#define CACHE_SIG_ATTRIB_1    1
#define CACHE_SIG_ATTRIB_2    2
#define CACHE_SIG_ATTRIB_3    3
#define CACHE_SIG_ATTRIB_4    4
#define CACHE_SIG_ATTRIB_5    5
#define CACHE_SIG_ATTRIB_6    6
#define CACHE_SIG_ATTRIB_7    7

#define CACHE_SIG_INDEX       8

extern int khrn_cache_init(KHRN_CACHE_T *cache);
extern void khrn_cache_term(KHRN_CACHE_T *cache);

extern int khrn_cache_lookup(KHRN_CACHE_T *cache, const void *data, int len, int sig, bool is_opengles_11);

#endif

