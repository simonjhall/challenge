/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef KHRN_WORKER_4_H
#define KHRN_WORKER_4_H

#include "interface/khronos/common/khrn_int_util.h"

extern bool khrn_worker_init(void);
extern void khrn_worker_term(void);

typedef enum {
   KHRN_WORKER_CALLBACK_RESULT_DO_IT_WAIT,
   KHRN_WORKER_CALLBACK_RESULT_DO_IT_OK,
   KHRN_WORKER_CALLBACK_RESULT_DO_IT_OK_ADVANCE_EXIT_POS,
   KHRN_WORKER_CALLBACK_RESULT_CLEANUP
} KHRN_WORKER_CALLBACK_RESULT_T;

typedef enum {
   KHRN_WORKER_CALLBACK_REASON_DO_IT,
   KHRN_WORKER_CALLBACK_REASON_CLEANUP
} KHRN_WORKER_CALLBACK_REASON_T;

typedef KHRN_WORKER_CALLBACK_RESULT_T (*KHRN_WORKER_CALLBACK_T)(
   KHRN_WORKER_CALLBACK_REASON_T reason,
   void *message, uint32_t size);

extern void *khrn_worker_post_begin(KHRN_WORKER_CALLBACK_T callback, uint32_t size); /* message will be 4-byte aligned */
extern void khrn_worker_post_end(void);

extern void khrn_worker_cleanup(void); /* called from khrn_sync_master_wait */

extern void khrn_worker_wait(void);

extern void khrn_worker_notify(void);

/*
   fifo pos stuff for interlocking with other fifos (eg hw)

   khrn_worker_get_enter_pos should only be called from the master task and
   should always return the right value

   khrn_worker_get_exit_pos can be called from any task, but might return the
   wrong value. the value will always be <= the right value however
*/

extern uint64_t khrn_worker_enter_pos;

static INLINE void khrn_worker_advance_enter_pos(void)
{
   ++khrn_worker_enter_pos;
}

static INLINE uint64_t khrn_worker_get_enter_pos(void)
{
   return khrn_worker_enter_pos;
}

extern uint32_t khrn_worker_exit_pos_0, khrn_worker_exit_pos_1;

static INLINE void khrn_worker_advance_exit_pos(void)
{
   uint64_t next_exit_pos = (((uint64_t)khrn_worker_exit_pos_1 << 32) | khrn_worker_exit_pos_0) + 1;
   khrn_barrier();
   khrn_worker_exit_pos_0 = (uint32_t)next_exit_pos;
   khrn_barrier();
   khrn_worker_exit_pos_1 = (uint32_t)(next_exit_pos >> 32);
}

static INLINE uint64_t khrn_worker_get_exit_pos(void)
{
   uint32_t exit_pos_1 = khrn_worker_exit_pos_1, exit_pos_0;
   khrn_barrier();
   exit_pos_0 = khrn_worker_exit_pos_0;
   khrn_barrier();
   return ((uint64_t)exit_pos_1 << 32) | exit_pos_0;
}

#endif
