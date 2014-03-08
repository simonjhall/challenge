/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef KHRN_INTERLOCK_PRIV_4_H
#define KHRN_INTERLOCK_PRIV_4_H

#include "middleware/khronos/common/khrn_interlock.h"

typedef enum {
   KHRN_INTERLOCK_FIFO_NONE, /* just for khrn_interlock_get_write_fifo */

   /* todo: would KHRN_INTERLOCK_FIFO_HW_BIN/KHRN_INTERLOCK_FIFO_HW_BIN_RENDER be useful? */
   KHRN_INTERLOCK_FIFO_HW_RENDER,
   KHRN_INTERLOCK_FIFO_WORKER
} KHRN_INTERLOCK_FIFO_T;

extern void khrn_interlock_transfer(KHRN_INTERLOCK_T *interlock, KHRN_INTERLOCK_USER_T user, KHRN_INTERLOCK_FIFO_T fifo);

extern KHRN_INTERLOCK_FIFO_T khrn_interlock_get_write_fifo(KHRN_INTERLOCK_T *interlock);

#endif
