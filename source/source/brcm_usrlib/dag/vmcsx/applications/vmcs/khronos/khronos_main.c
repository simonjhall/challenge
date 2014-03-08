/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include "interface/vchi/os/os.h"
#include "interface/vchi/vchi.h"
#include "interface/vcos/vcos_init.h"
#include "interface/vchi/connections/connection.h"
#include "interface/vchi/message_drivers/message.h"
#include "vcfw/drivers/chip/v3d.h"
#ifdef ANDROID
#include <cutils/log.h>
#endif

extern void vchi_khrn_server_init(VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T ** connections, uint32_t num_connections);
extern void vc_vchi_khronos_init(VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections);

/******************************************************************************
Defines
******************************************************************************/

/******************************************************************************
Static data
******************************************************************************/

/******************************************************************************
Static functions.
******************************************************************************/
#ifndef RPC_DIRECT
static void  vchi_init_host(unsigned a, void *b)
{
	VCHI_INSTANCE_T		initialise_instance;
	VCHI_CONNECTION_T*	connection[1];
	int32_t				success = -1; //fail by default

	success = vchi_initialise(&initialise_instance);
	assert( success == 0 );

	connection[0] = vchi_create_connection(single_get_func_table(), vchi_mphi_message_driver_func_table());

	// now turn on the control service and initialise the connection
	success = vchi_connect(&connection[0], 1, initialise_instance);

	vc_vchi_khronos_init(initialise_instance, &connection[0], 1);
}

static void  vchi_init_vc(unsigned a, void *b)
{
	VCHI_INSTANCE_T		initialise_instance;
	VCHI_CONNECTION_T*	connection[1];
	int32_t				success = -1; //fail by default

	success = vchi_initialise(&initialise_instance);
	assert( success == 0 );

	connection[0] = vchi_create_connection(single_get_func_table(), vchi_mphi_message_driver_func_table());

	vchi_khrn_server_init(initialise_instance, &connection[0], 1);

	#ifdef WANT_VCHI_TESTS
	// vchi test servers
	#if 1
	vc_dc4_server_init(initialise_instance, &connection[0], 1);
	#else
//	vchi_short_server_init(initialise_instance, &connection[0], 1);
	vchi_long_server_init(initialise_instance, &connection[0], 1);
	vchi_cli_server_init(initialise_instance, &connection[0], 1);
	vchi_svr_server_init(initialise_instance, &connection[0], 1);
	vchi_tstb_server_init(initialise_instance, &connection[0], 1);
	vchi_tstd_server_init(initialise_instance, &connection[0], 1);
	#endif
	#endif

	// now turn on the control service and initialise the connection
	success = vchi_connect(&connection[0], 1, initialise_instance);
	assert( success == 0 );

	#ifdef WANT_VCHI_TESTS
	vc_dc4_client_init(initialise_instance, &connection[0], 1);
	#endif
} 
#endif

static int	v3d_init = 1;
/******************************************************************************
 * public functions.
******************************************************************************/
#if defined(ANDROID) && !defined(BRCM_V3D_OPT)
void khronos_init()
{
	int32_t success;
	VCOS_THREAD_T thread_vc, thread_host;
	const int STACK = 16*1024;

#ifdef RPC_DIRECT
	vcos_init();
	v3d_get_func_table()->init();
#else
	if (os_init())
		LOGE("khronos_init: os_init() failed\n");

	v3d_get_func_table()->init();

//	success = os_thread_start(&thread_host, vchi_init_host, NULL, STACK, "vchi_init_host");
//	assert(success==0);

	success = os_thread_start(&thread_vc, vchi_init_vc, NULL, STACK, "vchi_init_vc");
	assert(success==0);

	vchi_init_host(0, NULL);
#endif // RPC_DIRECT
}

#else // ANDROID

//i moved it from  khrn_client_platform_direct.c
#ifdef RPC_DIRECT_MULTI

#include "interface/khronos/common/khrn_client.h"

static VCOS_EVENT_T ev;
static VCOS_SEMAPHORE_T egl_server_multi_lock;
extern int egl_server_state_initted;
void egl_server_init()
{

	vcos_semaphore_wait(&egl_server_multi_lock);

	if (egl_server_state_initted == 0)
	{	
		//i moved it from platform_tls_get() in khrn_client_platform_direct.c
		egl_server_startup_hack(); //egl_server_state_initted will be set
	}
#ifdef BRCM_V3D_OPT
	else
		egl_server_state_initted++;
#endif
	vcos_semaphore_post(&egl_server_multi_lock);

}

void khronos_exit()
{
   CLIENT_PROCESS_STATE_T *process;
	
	vcos_semaphore_wait(&egl_server_multi_lock);

#ifdef BRCM_V3D_OPT
	if(egl_server_state_initted == 0)
	{
		vcos_semaphore_post(&egl_server_multi_lock);
		return;
	}
	egl_server_state_initted--;
	if(egl_server_state_initted != 0)
	{
		vcos_semaphore_post(&egl_server_multi_lock);
		return;
	}
#endif

#ifndef BRCM_V3D_OPT
	#ifndef SIMPENROSE
		vcos_event_delete(&ev);
	#endif
#endif
	process = client_egl_get_process_state(NULL, (EGLDisplay)1, EGL_FALSE);
	if (process)
		client_process_state_term(process);
	else
		vcos_assert(0);

#ifndef BRCM_V3D_OPT
	client_process_detach();
#endif
	khrn_misc_try_unload_impl();	
	
	vcos_semaphore_post(&egl_server_multi_lock);
}

#endif


void khronos_init()
{
	int32_t			success;
	VCOS_THREAD_T	thread_vc, thread_host;
	const int		STACK		= 16*1024;

	if (v3d_init)
	{
		assert(v3d_get_func_table()->init() == 0);
		v3d_init = 0;

#ifdef RPC_DIRECT
		vcos_init();

	#ifdef RPC_DIRECT_MULTI
		success = vcos_semaphore_create(&egl_server_multi_lock,"egl_server_multi_lock",1);
		vcos_assert(success==0);

		#ifndef SIMPENROSE
			vcos_event_create(&ev, "Khronos Master Event");
			khrn_specify_event(&ev);
		#endif

#ifndef BRCM_V3D_OPT
		egl_server_init();
#endif
		client_process_attach();
	#endif
#else
		assert(os_init() == 0);
	
		success = os_thread_start(&thread_host, vchi_init_host, NULL, STACK, "vchi_init_host");
		assert(success==0);
	
		success = os_thread_start(&thread_vc, vchi_init_vc, NULL, STACK, "vchi_init_vc");
		assert(success==0);
#endif
	}

	return;  //only once

}
#endif // ANDROID

void patch_async_APB_bridge_HW_bug(void)
{
	if (!v3d_init)
	{
		// SW patch for Async APB bridge HW bug
		*((volatile unsigned int *)(0x8950438)) = 0;
		if (*((volatile unsigned int *)(0x8950438)) != 0) 
			*((volatile unsigned int *)(0x8950438)) = 0;
	}
}
