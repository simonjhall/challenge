include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
					
	COMMON_VPATH	+=	generic 
	COMMON_VPATH	+=	mobcom_rtos

	SRC32S			+=	vcos_mobcom_rtos.c
	SRC32S			+=	vcos_joinable_thread_from_plain.c
	SRC32S			+=	vcos_generic_reentrant_mtx.c 
	SRC32S			+=	vcos_generic_named_sem.c
	SRC32S			+=	vcos_generic_tls.c
	SRC32S			+=	vcos_latch_from_sem.c
#	SRC32S			+=	vcos_spinlock_semaphore.c
	SRC32S			+=	vcos_msgqueue.c
	SRC32S			+=	vcos_abort.c
#	SRC32S			+=	vcos_threadx_quickslow_mutex.c
#	SRC32S			+=	vcos_logcat.c
	SRC32S			+=	vcos_mem_from_malloc.c

#
# From Threadx platform
#
#vcos_threadx.c 
#vcos_joinable_thread_from_plain.c
#vcos_generic_reentrant_mtx.c 
#vcos_generic_named_sem.c
#vcos_generic_tls.c
#vcos_spinlock_semaphore.c
#vcos_msgqueue.c 
#vcos_abort.c 
#vcos_dlfcn.c
#vcos_threadx_quickslow_mutex.c
#vcos_logcat.c
