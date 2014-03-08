/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VG_SET_H
#define VG_SET_H

/*
   todo: this is just a wrapper over map for the moment
*/

#include "middleware/khronos/common/khrn_map.h"

typedef KHRN_MAP_T VG_SET_T;

extern bool vg_set_init(VG_SET_T *set, uint32_t capacity);

extern bool vg_set_insert(VG_SET_T *set, MEM_HANDLE_T key);
extern void vg_set_delete(VG_SET_T *set, MEM_HANDLE_T key);

extern bool vg_set_contains(VG_SET_T *set, MEM_HANDLE_T key);
extern bool vg_set_contains_locked(VG_SET_T *set, MEM_HANDLE_T key, void *storage);

typedef void (*SET_CALLBACK_T)(VG_SET_T *set, MEM_HANDLE_T key, void *);
extern void vg_set_iterate(VG_SET_T *set, SET_CALLBACK_T func, void *);

extern void vg_set_term(VG_SET_T *set);

#endif
