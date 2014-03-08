/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/common/khrn_interlock.h"

void khrn_interlock_init(KHRN_INTERLOCK_T *interlock)
{
   interlock->disp_image_handle = EGL_DISP_IMAGE_HANDLE_INVALID;
   interlock->users = KHRN_INTERLOCK_USER_NONE;
   khrn_interlock_extra_init(interlock);
}

void khrn_interlock_term(KHRN_INTERLOCK_T *interlock)
{
   interlock->users &= ~KHRN_INTERLOCK_USER_INVALID;
   vcos_assert(!interlock->users);
   khrn_interlock_extra_term(interlock);
}

bool khrn_interlock_read(KHRN_INTERLOCK_T *interlock, KHRN_INTERLOCK_USER_T user) /* user allowed to be KHRN_INTERLOCK_USER_NONE */
{
   vcos_assert(_count(user) <= 1);
   if (!(interlock->users & user)) {
      if (interlock->users & KHRN_INTERLOCK_USER_WRITING) {
         vcos_assert(_count(interlock->users) == 2); /* exactly 1 writer (plus writing bit) */
         khrn_interlock_flush((KHRN_INTERLOCK_USER_T)(interlock->users & ~KHRN_INTERLOCK_USER_WRITING));
         vcos_assert(!interlock->users);
      }
      interlock->users = (KHRN_INTERLOCK_USER_T)(interlock->users | user);
      return true;
   }
   return false;
}

bool khrn_interlock_write(KHRN_INTERLOCK_T *interlock, KHRN_INTERLOCK_USER_T user) /* user allowed to be KHRN_INTERLOCK_USER_NONE */
{
   interlock->users &= ~KHRN_INTERLOCK_USER_INVALID;
   vcos_assert(_count(user) <= 1);
   if (!user || (~interlock->users & (user | KHRN_INTERLOCK_USER_WRITING))) {
      for (;;) {
         KHRN_INTERLOCK_USER_T other_users, other_user;
         other_users = (KHRN_INTERLOCK_USER_T)(interlock->users & ~user & KHRN_INTERLOCK_USER_WRITING);
         if (!other_users) { break; }
         other_users = (KHRN_INTERLOCK_USER_T)(interlock->users & ~user & ~KHRN_INTERLOCK_USER_WRITING);
         other_user = (KHRN_INTERLOCK_USER_T)(1 << _msb(other_users));
         khrn_interlock_flush(other_user);
         vcos_assert(!(interlock->users & other_user));
      }
      if (user) {
         interlock->users = (KHRN_INTERLOCK_USER_T)(interlock->users | user | KHRN_INTERLOCK_USER_WRITING);
      }
      return true;
   }
   return false;
}

KHRN_INTERLOCK_USER_T khrn_interlock_get_writer(KHRN_INTERLOCK_T *interlock)
{
   vcos_assert(!(interlock->users & KHRN_INTERLOCK_USER_WRITING) || (_count(interlock->users) == 2));
   return (interlock->users & KHRN_INTERLOCK_USER_WRITING) ?
      (KHRN_INTERLOCK_USER_T)(interlock->users & ~KHRN_INTERLOCK_USER_WRITING) :
      KHRN_INTERLOCK_USER_NONE;
}

bool khrn_interlock_release(KHRN_INTERLOCK_T *interlock, KHRN_INTERLOCK_USER_T user)
{
   vcos_assert(_count(user) == 1);
   vcos_assert(!(interlock->users & KHRN_INTERLOCK_USER_WRITING) || (interlock->users == (user | KHRN_INTERLOCK_USER_WRITING)));
   if (interlock->users & user) {
      interlock->users = (KHRN_INTERLOCK_USER_T)(interlock->users & ~user & ~KHRN_INTERLOCK_USER_WRITING);
      return true;
   }
   return false;
}

bool khrn_interlock_write_would_block(KHRN_INTERLOCK_T *interlock)
{
   KHRN_INTERLOCK_USER_T other_users;
   other_users = (KHRN_INTERLOCK_USER_T)(interlock->users & ~KHRN_INTERLOCK_USER_WRITING & ~KHRN_INTERLOCK_USER_INVALID);
   return !!(other_users);
}

void khrn_interlock_invalidate(KHRN_INTERLOCK_T *interlock)
{
   khrn_interlock_write (interlock, KHRN_INTERLOCK_USER_NONE);
   interlock->users |= KHRN_INTERLOCK_USER_INVALID;
}

bool khrn_interlock_is_invalid(KHRN_INTERLOCK_T *interlock)
{
   return ((interlock->users & KHRN_INTERLOCK_USER_INVALID) != 0);
}

