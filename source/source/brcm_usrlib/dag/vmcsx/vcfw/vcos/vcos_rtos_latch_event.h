/* ============================================================================
Copyright (c) 2009-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/** @file osal_rtos_latch.h
  *
  * Provide an implementation of VCOS events based on the VideoCore RTOS_LATCH_T.
  */

#ifndef VCOS_RTOS_LATCH_EVENT_H
#define VCOS_RTOS_LATCH_EVENT_H

#include "vcfw/rtos/rtos.h"

typedef RTOS_LATCH_T VCOS_EVENT_T;

#ifdef VCOS_INLINE_BODIES

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_create(VCOS_EVENT_T *event, const char *name)
{
   *event = rtos_latch_locked();
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_event_signal(VCOS_EVENT_T *event)
{
   rtos_latch_put(event);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_wait(VCOS_EVENT_T *event)
{
   rtos_latch_get(event);
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_event_delete(VCOS_EVENT_T *event)
{
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_try(VCOS_EVENT_T *event)
{
   return ((rtos_latch_try(event) == 0) ? VCOS_SUCCESS : VCOS_EAGAIN);
}

#endif /* VCOS_INLINE_BODIES */

/* We want these to be *really* fast! */
#define vcos_event_signal(e) rtos_latch_put(e)
#define vcos_event_wait(e) (rtos_latch_get(e), VCOS_SUCCESS)
#define vcos_event_try(e) ((rtos_latch_try(e) == 0) ? VCOS_SUCCESS : VCOS_EAGAIN)

#endif


