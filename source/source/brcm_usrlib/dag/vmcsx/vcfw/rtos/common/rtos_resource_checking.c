/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "vcfw/rtos/rtos.h"

#ifdef RESOURCE_FAILURE_TESTING

uint32_t total_checks_done = 0;
uint32_t fail_on_check = 0; // Set by the debugger
uint32_t is_check_failing = 0;

static RTOS_LATCH_T check_fail_latch = rtos_latch_unlocked();
static uint32_t are_checking = 0;

void start_resource_failure_testing()
{
   are_checking = 1;
}


uint32_t should_check_fail()
{
   rtos_latch_get(&check_fail_latch);
   if (are_checking)
   {
      total_checks_done++;
      if (total_checks_done == fail_on_check)
      {
         is_check_failing = 1;
         _bkpt();
         is_check_failing = 0;
         rtos_latch_put(&check_fail_latch);
         return 1;
      }
   }
   rtos_latch_put(&check_fail_latch);
   return 0;
}

#endif
