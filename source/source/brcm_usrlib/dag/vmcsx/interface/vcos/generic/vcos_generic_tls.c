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
#include "vcos_generic_tls.h"
#include <stddef.h> /* For NULL */
#include <string.h>

/** Which TLS slots are in use? */
static unsigned long slot_map;

static VCOS_MUTEX_T lock;
static int inited;

VCOS_STATUS_T vcos_tls_init(void)
{
   slot_map = 0;
   vcos_assert(!inited);
   inited = 1;
   return vcos_mutex_create(&lock, "vcostls");
}

void vcos_tls_deinit(void)
{
   vcos_assert(inited);
   inited = 0;
   vcos_mutex_delete(&lock);
}

VCOS_STATUS_T vcos_generic_tls_create(VCOS_TLS_KEY_T *key)
{
   int i;
   VCOS_STATUS_T ret = VCOS_ENOSPC;

   vcos_assert(inited);

   vcos_mutex_lock(&lock);
   for (i=0; i<VCOS_TLS_MAX_SLOTS; i++)
   {
      if ((slot_map & (1<<i)) == 0)
      {
         slot_map |= (1<<i);
         ret = VCOS_SUCCESS;
         *key = i;
         break;
      }
   }
   vcos_mutex_unlock(&lock);
   return ret;
}

void vcos_generic_tls_delete(VCOS_TLS_KEY_T tls)
{
   vcos_mutex_lock(&lock);
   vcos_assert(slot_map & (1<<tls));
   slot_map &= ~(1<<tls);
   vcos_mutex_unlock(&lock);
}


void vcos_tls_thread_register(VCOS_TLS_THREAD_T *tls)
{
   memset(tls, 0, sizeof(*tls));
   _vcos_tls_thread_ptr_set(tls);
}


#ifdef __CC_ARM	
/** Get the given value. No locking required.
  */
void *vcos_tls_get(VCOS_TLS_KEY_T tls) {
   VCOS_TLS_THREAD_T *tlsdata = (VCOS_TLS_THREAD_T *)_vcos_tls_thread_ptr_get();
	if (!tlsdata) //if not allocated
	{
		tlsdata = (VCOS_TLS_THREAD_T*)vcos_malloc(sizeof(VCOS_TLS_THREAD_T),"VCOS_TLS_THREAD_T ");
		vcos_tls_thread_register(tlsdata);
	}
   vcos_assert(tlsdata); /* Fires if this thread has not been registered */
   if (tls<VCOS_TLS_MAX_SLOTS)
   {
      return tlsdata->slots[tls];
   }
   else
   {
      vcos_assert(0);
      return NULL;
   }
}
#endif	


