/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */


#ifndef DMALLOC_H_
#define DMALLOC_H_

#ifdef __CC_ARM
#include <stdlib.h> // For size_t#
#endif

#if defined(__BCM2708A0__) || defined(__BCM2708B0__)
/* to workaround hw-2827, ALIAS_COHERENT is actually ALIAS_L1_NONALLOCATING. we
 * want a strictly non-allocating alias, in particular for guard word writes, so
 * we use ALIAS_DIRECT for everything instead */
#define DMALLOC_USE_ALIAS_DIRECT
#endif

#ifdef DMALLOC_USE_ALIAS_DIRECT
#define DMALLOC_ALIAS ALIAS_DIRECT
#else
#define DMALLOC_ALIAS ALIAS_COHERENT
#endif

/******************************************************************************
 Global type definitions.
 *****************************************************************************/

typedef struct _segment_header *dmalloc_pool_t;

#define DMALLOC_POOL_ANY     0
#define DMALLOC_POOL_INVALID 0

/******************************************************************************
Global function definitions.
******************************************************************************/

int dmalloc_init(void);

dmalloc_pool_t dmalloc_create_pool(void *start, void *end);
int dmalloc_delete_pool(dmalloc_pool_t pool, int force);

void *dmalloc(dmalloc_pool_t pool, size_t size);
void *drealloc(dmalloc_pool_t pool, void *ptr, size_t size);
void dfree(dmalloc_pool_t pool, void *ptr);

int dmalloc_find(dmalloc_pool_t pool, const void **ptr);
const void *dmalloc_scan(dmalloc_pool_t pool,
   void const *(*check_allocated)(const void *base, int length, void *handle),
   void const *(*check_free)(const void *base, int length, void *handle),
   void *handle);
uint32_t dmalloc_get_free_mem(const dmalloc_pool_t pool);

#endif
