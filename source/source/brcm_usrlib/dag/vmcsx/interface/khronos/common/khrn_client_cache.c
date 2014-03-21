/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "interface/khronos/common/khrn_int_common.h"

#include "interface/khronos/common/khrn_client_cache.h"
#include "interface/khronos/common/khrn_client_platform.h"
#include "interface/khronos/common/khrn_client_rpc.h"

#include "interface/khronos/common/khrn_int_hash.h"
#include "interface/khronos/common/khrn_int_util.h"

#ifdef RPC_DIRECT
#include "interface/khronos/glxx/glxx_int_impl.h"
#endif

#include <assert.h>

#include "cache_cache.h"

#ifdef SIMPENROSE
#include "v3d/verification/tools/2760sim/simpenrose.h"
#endif
//#include "utils/Log.h"

static uint32_t hash(const void *data, int len, int sig)
{
   int hash;

//   if (len > 256)     // TODO: turn this on later
//      len = 256;

   if (!((size_t)data & 3) && !(len & 3))
      hash = khrn_hashword((uint32_t *)data, len >> 2, 0);
   else
      hash = khrn_hashlittle(data, len, 0);

   return (hash & ~0xf) | sig;
}

int khrn_cache_init(KHRN_CACHE_T *cache)
{
   return khrn_map_init(&cache->map, 64);
}

void khrn_cache_term(KHRN_CACHE_T *cache)
{
   khrn_map_term(&cache->map);
}


#ifdef SIMPENROSE_RECORD_OUTPUT
static bool xxx_first = true;
#endif
int khrn_cache_lookup(KHRN_CACHE_T *cache, const void *data, int len, int sig, bool is_opengles_11)
{

	int key = (int)data;
	int loop_counter = 0;
	MEM_HANDLE_T handle = (MEM_HANDLE_T)khrn_map_lookup(&cache->map, key);

	if((handle == MEM_HANDLE_INVALID) || (handle == (MEM_HANDLE_T)(-1)))
		{
		handle = mem_alloc_ex(len,16,(MEM_FLAG_T)(MEM_FLAG_DIRECT | MEM_FLAG_NO_INIT | MEM_FLAG_RESIZEABLE),"vertex attributes",MEM_COMPACT_NONE);
		if(handle != MEM_HANDLE_INVALID)
			{
			void* handle_mem = mem_lock(handle);
			platform_memcpy(handle_mem,data,len);
			mem_unlock(handle);
			khrn_map_insert(&cache->map, key, handle);

#ifdef LRU_FREE
			add_new_entry(&cache->map, key);
#endif

			mem_release(handle);
			}
      }
	else
		{
		void* handle_mem = mem_lock(handle);
		int handle_len = mem_get_size(handle);

		if (handle && handle_len >= len && !memcmp(handle_mem, data, len)) {
			mem_unlock(handle);
#ifdef LRU_FREE
			touch_entry(&cache->map, key);
#endif
			return handle;
			}
		else if(handle && handle_len < len && !memcmp(handle_mem, data, handle_len)) {
			mem_unlock(handle);
			int retval = mem_resize_ex(handle,len,MEM_COMPACT_NONE);
			if(retval != 0)
				{
				handle_mem = mem_lock(handle);
				platform_memcpy(handle_mem,data,len);
#ifdef LRU_FREE
				touch_entry(&cache->map, key);
#endif
				mem_unlock(handle);
				}
			}
		else
			{
			int key = hash(data, len, sig);
			handle = (MEM_HANDLE_T)khrn_map_lookup(&cache->map, key);
			if(!((handle == MEM_HANDLE_INVALID) || (handle == (MEM_HANDLE_T)(-1))))
				{
//				LOGE("hash key had a valid handle .. what to do?");
#ifdef LRU_FREE
					touch_entry(&cache->map, key);
#endif
				}
			else
				{
				handle = mem_alloc_ex(len,16,(MEM_FLAG_T)(MEM_FLAG_DIRECT | MEM_FLAG_NO_INIT | MEM_FLAG_RESIZEABLE),"vertex attributes",MEM_COMPACT_NONE);
				if(handle != MEM_HANDLE_INVALID)
					{
					handle_mem = mem_lock(handle);
					platform_memcpy(handle_mem,data,len);
					mem_unlock(handle);

#ifdef LRU_FREE
					add_new_entry(&cache->map, key);
#endif

					khrn_map_insert(&cache->map, key, handle);
					mem_release(handle);
					}
				}
			}
		}
	return handle;
}

