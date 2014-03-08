/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/* System includes */
#include <assert.h>
#include "helpers/vclib/vclib.h"
#include "vcfw/rtos/rtos.h"


/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/

#ifndef VCLIB_TMP_BUF_SIZE
#define VCLIB_TMP_BUF_SIZE (32*1024)
#endif


/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
******************************************************************************/

#define TMP_BUF_START (*(volatile uint32_t *)vclib_static_tmp_buf)
#define TMP_BUF_MAGIC 0xdadcdcdd

static uint32_t vclib_static_tmp_buf[VCLIB_TMP_BUF_SIZE / sizeof(uint32_t)] = { TMP_BUF_MAGIC, 0 };
#pragma align_to(32, vclib_static_tmp_buf);

static int vclib_static_tmp_buf_count = 0;
static RTOS_LATCH_T vclib_tmp_buf_lock = rtos_latch_unlocked(); // Lock for whole list


/******************************************************************************
Global function definitions.
******************************************************************************/

void *vclib_obtain_tmp_buf(int size)
{
   if (size == 0)
      size = VCLIB_TMP_BUF_SIZE;
   assert(size <= VCLIB_TMP_BUF_SIZE);
   rtos_latch_get(&vclib_tmp_buf_lock);
   assert(TMP_BUF_START == TMP_BUF_MAGIC);
   assert(vclib_static_tmp_buf_count == 0);
   vclib_static_tmp_buf_count++;
   return vclib_static_tmp_buf;
}

void vclib_release_tmp_buf(void *tmpbuf)
{
   assert(vclib_static_tmp_buf_count == 1);
   assert(tmpbuf == vclib_static_tmp_buf);
   --vclib_static_tmp_buf_count;
   TMP_BUF_START = TMP_BUF_MAGIC;
   rtos_latch_put(&vclib_tmp_buf_lock);
}

void *vclib_get_tmp_buf(void)
{
   assert(0);
   return vclib_static_tmp_buf; // xxx deprecated
}

int vclib_get_tmp_buf_size(void)  // Get the size of the shared buffer
{
   return VCLIB_TMP_BUF_SIZE;
}
