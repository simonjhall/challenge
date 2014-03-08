/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "interface/vcos/vcos.h"
#include <stdlib.h>

int vcos_verify_bkpts = 0;

int vcos_verify_bkpts_enabled(void)
{
#ifdef __CC_ARM
   return 0;
#endif   
  
   return vcos_verify_bkpts;
}

int vcos_verify_bkpts_enable(int enable)
{
   int old = vcos_verify_bkpts;
#ifdef __CC_ARM
   return 0;
#endif   

   vcos_verify_bkpts = enable;
   return old;
}

/**
  * Call the fatal error handler.
  */
void vcos_abort(void)
{
#ifdef __CC_ARM
    assert(0);
    return;
#endif

#ifdef __VIDEOCORE__
   _bkpt();
#endif

#if defined(VCOS_HAVE_BACKTRACE) && !defined(NDEBUG)
   vcos_backtrace_self();
#endif

   /* Insert chosen fatal error handler here */
   abort();
}
