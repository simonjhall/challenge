/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/** FIXME: rename to vcos_mutex_from_sem.c
  */

#include "interface/vcos/vcos_mutex.h"
#include "interface/vcos/vcos_semaphore.h"
#include "interface/vcos/vcos_thread.h"

VCOS_STATUS_T vcos_generic_mutex_create(VCOS_MUTEX_T *latch, const char *name)
{
   VCOS_STATUS_T st = vcos_semaphore_create(&latch->sem, name, 1);
   latch->owner = 0;
   return st;
}

void vcos_generic_mutex_delete(VCOS_MUTEX_T *latch)
{
   vcos_assert(latch->owner == 0);
   vcos_semaphore_delete(&latch->sem);
}

VCOS_STATUS_T vcos_generic_mutex_lock(VCOS_MUTEX_T *latch)
{
   VCOS_STATUS_T st;
   /* Non re-entrant latch - check for attempt to retake. */
   vcos_assert(latch->owner != vcos_thread_current());

   st = vcos_semaphore_wait(&latch->sem);
   if (st == VCOS_SUCCESS)
   {
      vcos_assert(latch->owner == 0);
#ifndef NDEBUG
      latch->owner = vcos_thread_current();
#endif
   }
   return st;
}

void vcos_generic_mutex_unlock(VCOS_MUTEX_T *latch)
{
   vcos_assert(latch->owner == vcos_thread_current());
#ifndef NDEBUG
   latch->owner = 0;
#endif
   vcos_semaphore_post(&latch->sem);
}

