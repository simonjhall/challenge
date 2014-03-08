/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_MAP_H
#define GLSL_MAP_H

struct _MapNode;
typedef struct _MapNode MapNode;

struct _MapNode
{
	const void* k;
	void* v;
	MapNode* next;
};

struct _Map
{
	MapNode* head;
};

// Creates new map.
Map* glsl_map_new(void);

// Clears all nodes from the map.
Map* glsl_map_clear(Map* map);

// Puts (k,v) into the map.
// Does *not* guarantee that k is unique in the map.
// User cannot put NULL keys as they are used for checkpoints.
bool glsl_map_put(Map* map, const void* k, void* v);

// Finds the latest occurrence of k in the map, and then gets the v associated with it.
// User cannot get NULL keys as they are used for checkpoints.
void *glsl_map_get(Map* map, const void* k, bool cross_checkpoints);

// Sets a checkpoint.
bool glsl_map_set_checkpoint(Map* map);

// Clears everything up to the last checkpoint.
bool glsl_map_clear_to_checkpoint(Map* map);

// Pops the latest value to be added.
void* glsl_map_pop(Map* map);

#endif // MAP_H