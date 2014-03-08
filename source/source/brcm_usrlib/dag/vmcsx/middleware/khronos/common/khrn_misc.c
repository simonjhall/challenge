/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "middleware/khronos/common/khrn_misc.h"
#include "middleware/khronos/gl11/gl11_server.h"
#include "middleware/khronos/gl20/gl20_server.h"
#include "middleware/khronos/vg/vg_server.h"
#include "middleware/khronos/egl/egl_server.h"

#if !defined(RPC_DIRECT) || defined(RPC_DIRECT_MULTI)
static const KHRONOS_FUNC_TABLE_T func_table = {
#if defined(RPC_DIRECT_MULTI)
	NULL,
#else
   khronos_dispatch,
#endif   
   khrn_sync_master_wait,
   khrn_specify_event,
   khrn_do_suspend_resume,
   
#define KHRN_IMPL_STRUCT_INIT
#include "interface/khronos/glxx/gl11_int_impl.h"
#include "interface/khronos/glxx/gl20_int_impl.h"
#include "interface/khronos/glxx/glxx_int_impl.h"
#include "interface/khronos/vg/vg_int_impl.h"
#include "interface/khronos/egl/egl_int_impl.h"
#include "interface/khronos/common/khrn_int_misc_impl.h"
#undef KHRN_IMPL_STRUCT_INIT
};

const KHRONOS_FUNC_TABLE_T *khronos_get_func_table(void)
{
   return &func_table;
}
#endif

void khrn_misc_startup_impl(void)
{
   egl_server_startup_hack();
}

int khrn_misc_try_unload_impl(void)
{
//TODO: check for thread safe
#if defined(RPC_DIRECT_MULTI) && !defined(BRCM_V3D_OPT)
	if (egl_server_state_initted==0)
		return 0;	
#endif	
   if (egl_server_is_empty()) {
      egl_server_shutdown();
      egl_server_state_initted = 0;
   }
   return 0;
}

void khrn_misc_fifo_finish_impl(void) /* todo: misleading name? */
{
   khrn_hw_common_finish();
}

void khrn_misc_rpc_flush_impl(void)
{
//TODO: check for thread safe
#if defined(RPC_DIRECT_MULTI)
	if (egl_server_state_initted==0)
		return;	
#endif	
   /* states must be unlocked before eglMakeCurrent */
   egl_server_unlock();
}
