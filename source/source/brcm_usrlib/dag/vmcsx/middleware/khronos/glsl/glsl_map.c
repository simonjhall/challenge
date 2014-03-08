/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include "middleware/khronos/glsl/glsl_common.h"

#include <stdlib.h>
#include "middleware/khronos/glsl/glsl_map.h"
#include "middleware/khronos/glsl/glsl_intern.h"
#include "middleware/khronos/glsl/glsl_fastmem.h"

Map* glsl_map_new()
{
	Map* map = (Map *)malloc_fast(sizeof(Map));
	map->head = NULL;
	return map;
}

Map* glsl_map_clear(Map* map)
{
	map->head = NULL;
   return map;
}

bool glsl_map_put(Map* map, const void* k, void* v)
{
	MapNode* new_head = NULL;

	if (!map) return false;
	if (!k) return false;

   //vcos_assert(k == glsl_intern((const char *)k, true));

	new_head = (MapNode *)malloc_fast(sizeof(MapNode));

	new_head->k = k;
	new_head->v = v;
	new_head->next = map->head;
	map->head = new_head;
	return true;
}

void *glsl_map_get(Map* map, const void* k, bool cross_checkpoints)
{
	MapNode* curr = NULL;

	if (!map) return NULL;
	if (!k) return NULL;

   //vcos_assert(k == glsl_intern((const char *)k, true));

	curr = map->head;

	while (curr)
	{
      vcos_assert(((size_t)curr & 3) == 0);

		if (NULL == curr->k && NULL == curr->v)
		{
         if (!cross_checkpoints)
            return NULL;
		}
		else if (k == curr->k)
		{
			return curr->v;
		}

		curr = curr->next;
	}

	return NULL;
}

bool glsl_map_set_checkpoint(Map* map)
{
	MapNode* new_head = NULL;

	if (!map) return false;

	new_head = (MapNode *)malloc_fast(sizeof(MapNode));

	new_head->k = NULL;
	new_head->v = NULL;
	new_head->next = map->head;
	map->head = new_head;
	return true;
}

bool glsl_map_clear_to_checkpoint(Map* map)
{
	MapNode* curr = NULL;

	if (!map) return false;

	curr = map->head;

	while (curr)
	{
		if (NULL == curr->k && NULL == curr->v)
		{
			// We found a checkpoint.
			map->head = curr->next;
			return true;
		}
		else
		{
			curr = curr->next;
		}
	}

	map->head = NULL;
	return false;
}

void* glsl_map_pop(Map* map)
{
	void* ret = NULL;
	MapNode* old_head = NULL;

	if (map->head)
	{
		ret = map->head->v;

		old_head = map->head;
		map->head = map->head->next;
	}
	
	return ret;
}