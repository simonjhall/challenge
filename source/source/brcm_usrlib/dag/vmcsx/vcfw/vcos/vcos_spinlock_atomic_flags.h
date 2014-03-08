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
  * Provide an implementation of VCOS atomic flags based on spinlocks.
  */

#ifndef VCOS_SPINLOCK_ATOMIC_FLAGS_H
#define VCOS_SPINLOCK_ATOMIC_FLAGS_H

typedef uint32_t VCOS_ATOMIC_FLAGS_T;

#ifdef VCOS_INLINE_BODIES

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_atomic_flags_create(VCOS_ATOMIC_FLAGS_T *atomic_flags)
{
   *atomic_flags = 0;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_atomic_flags_or(VCOS_ATOMIC_FLAGS_T *atomic_flags, uint32_t flags)
{
   _VCOS_OS_LOCK_STATE;
   _VCOS_OS_DISABLE();
   *atomic_flags |= flags;
   _VCOS_OS_RESTORE();
}

VCOS_INLINE_IMPL
uint32_t vcos_atomic_flags_get_and_clear(VCOS_ATOMIC_FLAGS_T *atomic_flags)
{
   _VCOS_OS_LOCK_STATE;
   uint32_t flags;
   _VCOS_OS_DISABLE();
   flags = *atomic_flags;
   *atomic_flags = 0;
   _VCOS_OS_RESTORE();
   return flags;
}

VCOS_INLINE_IMPL
void vcos_atomic_flags_delete(VCOS_ATOMIC_FLAGS_T *atomic_flags)
{
}

#endif /* VCOS_INLINE_BODIES */

#endif
