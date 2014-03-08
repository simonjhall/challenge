/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef KHRN_DISPATCH_H
#define KHRN_DISPATCH_H

#include <stdlib.h>

#ifdef KHRN_USE_VCHIQ
#include "interface/vchiq_arm/vchiq_vc.h"

extern int khronos_dispatch( void *_message, int _length, VCHIQ_SERVICE_HANDLE_T khrn_handle, VCHIQ_SERVICE_HANDLE_T khan_handle, VCHIU_QUEUE_T *queue, VCOS_EVENT_T *event );

typedef int KHRONOS_DISPATCH_FUNC( void *_message, int _length, VCHIQ_SERVICE_HANDLE_T khrn_handle, VCHIQ_SERVICE_HANDLE_T khan_handle, VCHIU_QUEUE_T *queue, VCOS_EVENT_T *event );
#else
#include "interface/vchi/vchi.h"

extern int khronos_dispatch( void *_message, int _length, VCHI_SERVICE_HANDLE_T _khrn_handle, VCHI_SERVICE_HANDLE_T _khan_handle );

typedef int KHRONOS_DISPATCH_FUNC( void *_message, int _length, VCHI_SERVICE_HANDLE_T _khrn_handle, VCHI_SERVICE_HANDLE_T _khan_handle );
#endif

extern bool khdispatch_within_workspace( const void *ptr, size_t length );

#endif /* SERVER_DISPATCH_H */
