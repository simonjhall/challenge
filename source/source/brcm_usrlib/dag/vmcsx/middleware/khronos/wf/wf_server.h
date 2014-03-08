/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef WF_SERVER_H
#define WF_SERVER_H

#include "middleware/khronos/common/khrn_map.h"

// There is a single global instance of this

typedef struct
{
   MAP_T contexts;

   uint32_t next_context;
} WF_SERVER_STATE_T;

typedef struct {
   int source;
   int mask;

   int destination_rectangle[4];
   float source_rectangle[4];

   bool flip;

   WFCRotation rotation;
   WFCScaleFilter scale_filter;

   WFCbitfield transparency_types;

   float global_alpha;
} WF_SERVER_LAYER_T;

#define WF_MAX_LAYERS 32

typedef struct
{
   int max;
   int pos;

   WF_SERVER_LAYER_T layers[MAX_WF_LAYERS];
} WF_SERVER_CONTEXT_T;

extern int wfcIntCreateContext_impl(int type, int info);
extern void wfcIntDestroyContext_impl(int handle);
extern void wfcIntCommitBegin_impl(int handle);
extern void wfcIntCommitAdd_impl(int handle, int source, int mask, int drect_0, int drect_1, int drect_2, int drect_3, float srect_0, float srect_1, float srect_2, float srect_3, int flip, int rotation, int scale_filter, int transparency, float global_alpha);
extern void wfcIntCommitEnd_impl(int handle);
extern void wfcActivate_impl(int handle);
extern void wfcDeactivate_impl(int handle);
extern void wfcCompose_impl(int handle);

#endif
