/* ============================================================================
Copyright (c) 2009-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/** @file
  *
  * Provide an implementation of VCOS mutexes based on the VideoCore RTOS_LATCH_T.
  */

#ifndef VCOS_RTOS_LATCH_H
#define VCOS_RTOS_LATCH_H

#include "vcfw/rtos/rtos.h"

typedef struct VCOS_MUTEX_T
{
   RTOS_LATCH_T latch;
} VCOS_MUTEX_T;

#ifdef VCOS_INLINE_BODIES

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_create(VCOS_MUTEX_T *latch, const char *name) {
   latch->latch = rtos_latch_unlocked();
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL void vcos_mutex_delete(VCOS_MUTEX_T *latch) {
   /* check it really is unlocked */
   /*vcos_assert(*latch == rtos_latch_unlocked()); */

   /* weird magic found in vchi - why?
   rtos_latch_get( &latch->latch );  
   rtos_latch_put( &latch->latch );
   */
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_lock(VCOS_MUTEX_T *latch) {
   rtos_latch_get(&latch->latch);
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_mutex_unlock(VCOS_MUTEX_T *latch) {
   rtos_latch_put(&latch->latch);
}

VCOS_INLINE_IMPL
int vcos_mutex_is_locked(VCOS_MUTEX_T *m) {
   if (m->latch == rtos_latch_unlocked())
      return 0;
   else
      return 1;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_trylock(VCOS_MUTEX_T *m) {
   int rc = rtos_latch_try(&m->latch);
   return rc == 0 ? VCOS_SUCCESS: VCOS_EAGAIN;
}


#endif /* VCOS_INLINE_BODIES */

/* We want these to be *really* fast! */
#define vcos_mutex_lock(l) (rtos_latch_get(&(l)->latch), VCOS_SUCCESS)
#define vcos_mutex_unlock(l) rtos_latch_put(&(l)->latch)
#define vcos_mutex_trylock(l) ((rtos_latch_try(&(l)->latch) == 0) ? VCOS_SUCCESS : VCOS_EAGAIN)

#endif

