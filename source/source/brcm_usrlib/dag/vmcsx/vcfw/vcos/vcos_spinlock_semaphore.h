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
  * Provide an implementation of VCOS mutexes based on spinlocks.
  *
  * Originally this used the spinlock implementation from the VCFW drivers, however
  * it turns out that it is not possible to do this in a way that does not result
  * in deadlock. This version uses the OS's own spinlocks instead to avoid this.
  */

#ifndef VCOS_SPINLOCK_SEM_H
#define VCOS_SPINLOCK_SEM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VCOS_SEMAPHORE_T
{
   unsigned count;
   struct VCOS_SEMAPHORE_WAITER_T *head;
   struct VCOS_SEMAPHORE_WAITER_T *tail;
} VCOS_SEMAPHORE_T;

typedef struct VCOS_SEMAPHORE_WAITER_T
{
   struct VCOS_SEMAPHORE_WAITER_T *next;
   VCOS_LLTHREAD_T *thread;
} VCOS_SEMAPHORE_WAITER_T;

VCOS_STATUS_T vcos_spinlock_sem_create(VCOS_SEMAPHORE_T *sem, const char *name, VCOS_UNSIGNED count);
void vcos_spinlock_sem_delete(VCOS_SEMAPHORE_T *sem);
void vcos_spinlock_sem_wait(VCOS_SEMAPHORE_T *sem);
VCOS_STATUS_T vcos_spinlock_sem_trywait(VCOS_SEMAPHORE_T *sem);
void vcos_spinlock_sem_post(VCOS_SEMAPHORE_T *sem);
VCOS_STATUS_T vcos_spinlock_init(void);

#ifdef VCOS_INLINE_BODIES

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_create(VCOS_SEMAPHORE_T *sem, const char *name, VCOS_UNSIGNED count) {
   return vcos_spinlock_sem_create(sem, name, count);
}

VCOS_INLINE_IMPL void vcos_semaphore_delete(VCOS_SEMAPHORE_T *sem) {
   vcos_spinlock_sem_delete(sem);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_wait(VCOS_SEMAPHORE_T *sem) {
   vcos_spinlock_sem_wait(sem);
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_trywait(VCOS_SEMAPHORE_T *sem) {
   return vcos_spinlock_sem_trywait(sem);
}
VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_post(VCOS_SEMAPHORE_T *sem) {
   vcos_spinlock_sem_post(sem);
   return VCOS_SUCCESS;
}

#endif /* VCOS_INLINE_BODIES */

#ifdef __cplusplus
}
#endif

#endif


