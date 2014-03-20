/*
 * cache_cache.cpp
 *
 *  Created on: 19 Mar 2014
 *      Author: simon
 */

#include <stdio.h>
#include <map>

extern "C"
{
#include "interface/khronos/common/khrn_int_common.h"

#include "interface/khronos/common/khrn_client_cache.h"

#include "interface/khronos/common/khrn_int_hash.h"
#include "interface/khronos/common/khrn_int_util.h"
}

#include "cache_cache.h"

static int frame_count = 0;

typedef std::map<int, uint64_t> InnerMap;
typedef std::map<KHRN_MAP_T *, InnerMap *> OuterMap;

static OuterMap s_allKeys;

static InnerMap *s_pOurVertexCache;
static KHRN_MAP_T *s_pTheirVertexCache;

static InnerMap &get_map(KHRN_MAP_T *cache, bool special = false)
{
	if (!special && cache->m_pReverseMap)
		return *(InnerMap *)cache->m_pReverseMap;

	if (special && cache->m_pReverseMap2)
		return *(InnerMap *)cache->m_pReverseMap2;


	assert(((size_t)cache & 1) == 0);

	//get our map
	OuterMap::iterator it = s_allKeys.find((KHRN_MAP_T *)((size_t)cache | (special ? 1 : 0)));
	InnerMap *pMap = 0;

	if (it == s_allKeys.end())
	{
		pMap = new InnerMap;
		s_allKeys[cache] = pMap;
	}
	else
		pMap = it->second;

	if (!special)
		cache->m_pReverseMap = (void *)pMap;
	else
	{
		cache->m_pReverseMap2 = (void *)pMap;
		s_pOurVertexCache = pMap;
		s_pTheirVertexCache = cache;
	}

	return *pMap;
}

void add_new_entry(KHRN_MAP_T *cache, int key)
{
	touch_entry(cache, key);
}

void touch_entry(KHRN_MAP_T *cache, int key)
{
	InnerMap &rMap = get_map(cache, true);
	rMap[key] = frame_count;
}

void increment_frame_count(void)
{
	frame_count++;

//	return;

//	if ((frame_count & 63) != 0)
//		return;


	{
//		printf("cache\t%08x\n", it->first);

		InnerMap::iterator i = s_pOurVertexCache->begin();

//		int deleted = 0;

		while (i != s_pOurVertexCache->end())
		{
			if (frame_count > 100 && (i->second < frame_count - 100))
			{
				khrn_map_delete(s_pTheirVertexCache, (i++)->first);
//				deleted++;
			}
			else
				++i;

//			if (deleted > 20)
//				return;
		}
	}
}
/////////////////////////////////////
extern "C" {

bool khrn_map_init(KHRN_MAP_T *map, uint32_t capacity)
{
	InnerMap &rMap = get_map(map);
	map->m_pReverseMap = &rMap;
	return true;
}

void khrn_map_term(KHRN_MAP_T *map)
{
	map->m_pReverseMap = 0;
	s_allKeys.erase(map);
}

bool khrn_map_insert(KHRN_MAP_T *map, uint32_t key, MEM_HANDLE_T value)
{
	/*if (MEM_HANDLE_T h = khrn_map_lookup(map, key))
	{
		mem_acquire(value);
		mem_release(h);
	}
	else
		mem_acquire(value);

	InnerMap &rMap = get_map(map);

	rMap[key] = (uint64_t)value;
	return true;*/

	mem_acquire(value);

	InnerMap &rMap = get_map(map);
	std::pair<InnerMap::iterator, bool> ret;

	ret = rMap.insert(std::pair<int, uint64_t>(key, (uint64_t)value));

	if (ret.second == false)
	{
		mem_release((MEM_HANDLE_T)ret.first->second);
		rMap.erase(ret.first);
		rMap.insert(ret.first, std::pair<int, uint64_t>(key, (uint64_t)value));
	}

	return true;
/*
	mem_acquire(value);

	InnerMap &rMap = get_map(map);

	rMap[key] = (uint64_t)value;

	return true;*/
}

bool khrn_map_delete(KHRN_MAP_T *map, uint32_t key)
{
	if (MEM_HANDLE_T h = khrn_map_lookup(map, key))
	{
		mem_release(h);

		InnerMap &rMap = get_map(map);
		rMap.erase(key);
		return true;
	}
	else
		return false;
}

MEM_HANDLE_T khrn_map_lookup_locked(KHRN_MAP_T *map, uint32_t key, void *)
{
	InnerMap &rMap = get_map(map);

	InnerMap::iterator it = rMap.find(key);
	if (it == rMap.end())
		return 0;
	else
		return (MEM_HANDLE_T)it->second;
}

MEM_HANDLE_T khrn_map_lookup(KHRN_MAP_T *map, uint32_t key)
{
	return khrn_map_lookup_locked(map, key, 0);
}

void khrn_map_iterate(KHRN_MAP_T *map, KHRN_MAP_CALLBACK_T func, void *p)
{
	InnerMap &rMap = get_map(map);
	InnerMap::iterator it = rMap.begin();

	while (it != rMap.end())
	{
		//it may delete an entry, so let's iterate asap
		(*func)(map, it->first, (MEM_HANDLE_T)(it++)->second, p);
	}
}

uint32_t khrn_map_get_count(KHRN_MAP_T *map)
{
	return get_map(map).size();
}

}
