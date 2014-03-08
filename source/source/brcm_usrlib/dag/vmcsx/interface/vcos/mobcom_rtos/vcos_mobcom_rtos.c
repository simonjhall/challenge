/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  vcos
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - mobcom rtos support
=============================================================================*/
#define VCOS_INLINE_BODIES

#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos_msgqueue.h"
#include "interface/vcos/generic/vcos_generic_named_sem.h"
#include "vcos_platform.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "ostask.h"
#include "dbg.h"
#include "assert.h"
#include "logapi.h"

static int				inited = 0;
static VCOS_THREAD_T	main_thread;

VCOS_STATUS_T vcos_llthread_create(
	VCOS_LLTHREAD_T*			thread,
	const char*					name,
	VCOS_LLTHREAD_ENTRY_FN_T	entry,
	void*						arg,
	void*						stack,
	VCOS_UNSIGNED				stacksz,
	VCOS_UNSIGNED				priority,
	VCOS_UNSIGNED				affinity,
	VCOS_UNSIGNED				timeslice,
	VCOS_UNSIGNED				autostart)
{
//	vcos_assert(priority >= VCOS_THREAD_PRI_MIN);
//	vcos_assert(priority <= VCOS_THREAD_PRI_MAX);
//	vcos_assert((affinity & ~NU_AFFINITY_MASK) == 0);
//	vcos_assert((stacksz & 3) == 0);

    thread->thread = OSTASK_CreateWArg(
						(TEntryWArg_t)entry, 
						(TName_t)name, 
						(TPriority_t)priority, 
						(TStackSize_t)stacksz, 
						(TArgc_t)1, 
						(TArgv_t)arg);

	if (!thread->thread)
		return VCOS_EINVAL; 
	else
		return VCOS_SUCCESS;
}

void vcos_llthread_exit(void)
{
//   STATUS st = NU_Terminate_Task(NU_Current_Task_Pointer());
//   vcos_assert(st==NU_SUCCESS);
}

void vcos_llthread_delete(
	VCOS_LLTHREAD_T*	thread)
{
	OSTASK_Destroy(thread->thread);
}

VCOS_STATUS_T _vcos_llthread_create_attach(VCOS_LLTHREAD_T *thread)
{
//	thread->suspended = 0;
	return VCOS_SUCCESS;
}

void vcos_log(
	const char*	fmt, 
	...)
{
	Log_DebugPrintf(LOGID_V3D, (char*)fmt); 
}

void vcos_logging_init(void)
{
}

void vcos_mob_rtos_report_error(void)
{
//   vcos_log("vcos: error %d", GetLastError());
}

VCOS_STATUS_T vcos_init(void)
{
	VCOS_STATUS_T st = VCOS_SUCCESS;
	
	//	rtos_latch_get(&lock);
	
	if (!inited)
	{
		vcos_logging_init();
		
		st = vcos_tls_init();
		vcos_assert(st == VCOS_SUCCESS);
		
		st = _vcos_named_semaphore_init();
		vcos_assert(st == VCOS_SUCCESS);
		
		//skip this, because it will set current task's tls pointer
		//leave it as NULL, and it will be set later
		//st = _vcos_thread_create_attach(&main_thread, "main");
		//vcos_assert(st == VCOS_SUCCESS);
		
		vcos_msgq_init();
		inited = 1;
	}
	
	//	rtos_latch_put(&lock);
	
	return st;
}


//TODO: find a better solution to handle tls for each thread/task
VCOS_STATUS_T vcos_add_tls()
{
	VCOS_TLS_THREAD_T *tlsdata;
	VCOS_STATUS_T st = VCOS_SUCCESS;
		
#ifdef VCOS_WANT_TLS_EMULATION
	tlsdata = (VCOS_TLS_THREAD_T *)_vcos_tls_thread_ptr_get();
	if (tlsdata)
		return st;

 	tlsdata = (VCOS_TLS_THREAD_T*) malloc(sizeof(VCOS_TLS_THREAD_T));		
    vcos_tls_thread_register(tlsdata);
#endif
      
   return st;
}

VCOS_STATUS_T vcos_remove_tls()
{
	VCOS_TLS_THREAD_T *tlsdata;
	VCOS_STATUS_T st = VCOS_SUCCESS;

#ifdef VCOS_WANT_TLS_EMULATION
	
	tlsdata = (VCOS_TLS_THREAD_T *)_vcos_tls_thread_ptr_get();
	if (!tlsdata)
		return st;

	free(tlsdata);
    _vcos_tls_thread_ptr_set(NULL);
#endif
	
	return st;

}



VCOS_STATUS_T vcos_timer_init(void)
{
	return VCOS_SUCCESS;
}

void vcos_deinit(void)
{
	vcos_assert(inited);
	_vcos_thread_delete(&main_thread);
	vcos_tls_deinit();
	inited = 0;
}

void _vcos_assert(
	const char*	file, 
	int			line, 
	const char*	desc)
{
	vcos_log("assertion failed: %s:%d: %s", file, line, desc);
#if defined(_MSC_VER)								// Visual C define equivalent types
	__asm { int 3 }
#endif //#if defined(_MSC_VER)						// Visual C define equivalent types
	assert(0);
}

int vcos_llthread_running(
	VCOS_LLTHREAD_T*	t) 
{
	vcos_assert(0);   // this function only exists as a nasty hack for the video codecs!
	return 1;
}

int vcos_have_rtos(void)
{
	return 1;
}

int vcos_snprintf(
	char*		buf, 
	size_t		buflen, 
	const char*	fmt,
	...)
{
	int ret;
	va_list ap;
	va_start(ap,fmt);
	ret = vsnprintf(buf, buflen, fmt, ap);
	va_end(ap);
	return ret;
}


