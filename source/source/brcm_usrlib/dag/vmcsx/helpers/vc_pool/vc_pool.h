/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef _VC_POOL_H_
#define _VC_POOL_H_

#include "vcfw/rtos/common/rtos_common_mem.h"
#include <stdlib.h> // For size_t

typedef enum {
   VC_POOL_FLAGS_NONE,
   VC_POOL_FLAGS_COHERENT = 1 << 0, // all objects use cache coherent aliases
   VC_POOL_FLAGS_DIRECT = 1 << 1,
   VC_POOL_FLAGS_SUBDIVISIBLE = 1 << 2,
   VC_POOL_FLAGS_HINT_PERMALOCK = 1 << 3,
   VC_POOL_FLAGS_UNEQUAL_SUBDIVISION = 1 << 4  //To be set with VC_POOL_FLAGS_SUBDIVISIBLE 
                                                //and allows final allocation to be different 
                                                //from the initially requested size
} VC_POOL_FLAGS_T;

// opaque handles
typedef struct opaque_vc_pool_s VC_POOL_T;
typedef struct opaque_vc_pool_object_s VC_POOL_OBJECT_T;

typedef uint32_t VC_POOL_HANDLE_T;


VC_POOL_T *vc_pool_create( size_t size, uint32_t num, uint32_t align, VC_POOL_FLAGS_T flags, const char *name, uint32_t overhead );
int32_t vc_pool_destroy( VC_POOL_T *pool, uint32_t timeout_usecs );
VC_POOL_OBJECT_T *vc_pool_alloc( VC_POOL_T *pool, size_t size, uint32_t timeout_usecs );
size_t vc_pool_max_object_size( VC_POOL_T *pool);
MEM_HANDLE_T vc_pool_mem_handle( VC_POOL_OBJECT_T *object );
int  vc_pool_offsetof( VC_POOL_OBJECT_T *object );
int  vc_pool_acquire( VC_POOL_OBJECT_T *object );
int  vc_pool_release( VC_POOL_OBJECT_T *object );
void *vc_pool_lock( VC_POOL_OBJECT_T *object );
void vc_pool_unlock( VC_POOL_OBJECT_T *object );
int vc_pool_valid(const VC_POOL_T *pool);
void *vc_pool_overhead(VC_POOL_OBJECT_T *object);
int vc_pool_max_objects( const VC_POOL_T *pool );
size_t vc_pool_pool_size( const VC_POOL_T *pool );

//this allows the setting of a callback signalled when an object is finally released
typedef void (*VC_POOL_CALLBACK_T) (void *cookie);
void vc_pool_set_callback(VC_POOL_T *pool, VC_POOL_CALLBACK_T callback, void* cookie);

// functions to manage handles for remote usage
VC_POOL_HANDLE_T vc_pool_create_handle(VC_POOL_T *pool);
void vc_pool_destroy_handle(VC_POOL_HANDLE_T handle);
void vc_pool_remove_handle(VC_POOL_T *pool);
VC_POOL_T* vc_pool_find_from_handle(VC_POOL_HANDLE_T handle);

#endif // _VC_POOL_H_
