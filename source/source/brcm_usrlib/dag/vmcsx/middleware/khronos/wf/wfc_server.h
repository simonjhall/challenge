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

#include "interface/khronos/include/WF/wfc.h"
#include "interface/khronos/wf/wfc_int.h"

uint32_t wfcIntCreateContext_impl
   (WFCContext context, uint32_t context_type, uint32_t screen_or_stream_num);
void wfcIntDestroyContext_impl(WFCContext ctx);
bool wfcIntCommitBegin_impl(WFCContext context, bool wait, WFCRotation rotation,
   WFCfloat background_red, WFCfloat background_green, WFCfloat background_blue,
   WFCfloat background_alpha);
void wfcIntCommitAdd_impl(WFCContext context, WFCElement element, WFC_ELEMENT_ATTRIB_T *element_attr,
   WFCNativeStreamType source_stream, WFCNativeStreamType mask_stream);
void wfcIntCommitEnd_impl(WFCContext ctx);
void wfcActivate_impl(WFCContext ctx);
void wfcDeactivate_impl(WFCContext ctx);
void wfcCompose_impl(WFCContext context);

#endif
