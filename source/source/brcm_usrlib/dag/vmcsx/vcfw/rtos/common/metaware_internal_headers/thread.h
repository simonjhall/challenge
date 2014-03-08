/*********************************************************************
(C) Copyright 2002      ARC International (ARC);  Santa Cruz, CA 95060
This program is the unpublished property and trade secret of ARC.   It
is to be  utilized  solely  under  license  from  ARC  and it is to be
maintained on a confidential basis for internal company use only.  The
security  and  protection  of  the program is paramount to maintenance
of the trade secret status.  It is to  be  protected  from  disclosure
to unauthorized parties, both within the Licensee company and outside,
in a manner  not less stringent than  that utilized for Licensee's own
proprietary internal information.  No  copies of  the source or Object
Code are to leave the premises of Licensee's business except in strict
accordance with the license agreement signed by Licensee with ARC.
*********************************************************************/
#ifndef _THREAD_H
#define _THREAD_H

#ifdef __CPLUSPLUS__
extern "C" {
#endif

/* model 0 : no multithreading */
#define _THREAD_MODEL_NONE	0
/* model 1 : mix of thread-local data and locking around file i/o and heap */
#define _THREAD_MODEL_LOCKS	1
/* model 2 : no locking, all global data is thread-local (DOS only) */
#define _THREAD_MODEL_ALL_TLS	2

/* if multi-thread */
#if defined(_REENTRANT)
    #if _MSNT
	#define _THREAD_MODEL _THREAD_MODEL_LOCKS
	#include <stdarg.h>
	#include <excpt.h>
	#include <windef.h>
	#include <winbase.h>

	typedef CRITICAL_SECTION _lock_t;

	#define LOCK(X)		EnterCriticalSection(&X);
	#define UNLOCK(X)	LeaveCriticalSection(&X);
	#define INIT_LOCK(X)	InitializeCriticalSection(&X);
	#define DELETE_LOCK(X)	DeleteCriticalSection(&X);
    #elif _OS2
	#define _THREAD_MODEL _THREAD_MODEL_LOCKS
	#define INCL_NOPMAPI
	#define INCL_DOSSEMAPHORES
	#include <os2.h>

	typedef HMTX _lock_t;

	#define LOCK(X)		DosRequestMutexSem(X, SEM_INDEFINITE_WAIT);
	#define UNLOCK(X)	DosReleaseMutexSem(X);
	#define INIT_LOCK(X)	DosCreateMutexSem(NULL, &X, 0, 0);
	#define DELETE_LOCK(X)	DosCloseMutexSem(X);
    #elif __OS_BARE || _MSDOS || _ARC || _ARM || _MIPS || _VIDEOCORE
	#define _THREAD_MODEL _THREAD_MODEL_LOCKS

	/* We don't know what OS the runtime will be layering on top of.
	   This interface must be generic enough to support most any RTOS.
	   There will be weak routines that do nothing by default which
	   you can easily override with a valid RTOS specific
	   implementation.
	*/
	typedef void *_lock_t;

	#if _ARC
	    extern void * _Preserve_flags _mwget_tls(void);
	#else
	extern void *_mwget_tls(void);
	#endif
	extern void _mwmutex_create(_lock_t*);
	extern void _mwmutex_delete(_lock_t*);
	extern void _mwmutex_lock(_lock_t);
	extern void _mwmutex_unlock(_lock_t);

	#define LOCK(X)		_mwmutex_lock(X);
	#define UNLOCK(X)	_mwmutex_unlock(X);
	#define INIT_LOCK(X)	_mwmutex_create(& (X));
	#define DELETE_LOCK(X)	_mwmutex_delete(& (X));
    #elif __OS_OPEN || _SOL || _ATT4 || _LINUX
	#define _THREAD_MODEL _THREAD_MODEL_LOCKS
	/* Systems supporting POSIX threads. POSIX doesn't support a thread
	   locking a lock it already has locked, but our libraries need to
	   do this sometimes.  Thus, we invent a scheme here that allows us
	   to do this.  We store the thread ID of the thread who has the lock
	   in the lock structure, or an invalid ID if no one has the lock.
	   Thus, if we are going to lock but the stored thread ID is ours,
	   we know not to call the posix lock function. We keep a count
	   of how many lock calls we have received, so when an unlock call
	   happens, we only call the posix unlock function when the count is 0
	*/

    	/* The MetaWare code generators will call _mwget_tls() to implement
	   the __declspec(thread) type qualifier.  You can make any global
	   variable local to a thread via this qualifier.  However, this
	   violates the definition of the language in that there will now
	   be multiple instances of a variable.  Use this feature carefully.
	   It is better to use auto variables wherever possible.
	*/
	extern void *_mwget_tls(void);

	/* We forgo cross-compiling for linux. Their include-files are
	 * too intertwined.
	 */
	#if __OS_OPEN || _LINUX
	    #include <pthread.h>
	#else  /* Solaris */
	    /* Instead of including the actual system headers, we include
	       our own boiled-down header.  This approach allows cross-
	       compilation. */
	    #include <sspc/pthread.h>   /* in library inc/sspc directory */
	#endif

	/* this value must be an invalid pthread_t */
	#define _INVALID_THREAD_ID -1

	typedef struct {
		pthread_mutex_t mutx;
		pthread_t th_id;        // thread ID of thread who has lock
		int count;              // to allow locking when already locked
		} _lock_t;
	/* LOCK should be improved to avoid two calls to pthread_self() */
	/* X is of type _lock_t */
	#define LOCK(X)         ( ((X).th_id == pthread_self()) \
				    ?   ((X).count++) \
				    :   (pthread_mutex_lock(&((X).mutx)),  \
					 (X).th_id = pthread_self(), \
					 (X).count++ \
					) \
				);
	#define UNLOCK(X)       (--((X).count) <= 0 ? \
				    ( (X).count = 0, \
				      (X).th_id = _INVALID_THREAD_ID, \
				      pthread_mutex_unlock(&((X).mutx)) \
				    ) : 0);
	#define INIT_LOCK(X)    (pthread_mutex_init(&((X).mutx), 0), \
				(X).count = 0, (X).th_id = _INVALID_THREAD_ID);
	#define DELETE_LOCK(X)  pthread_mutex_destroy(&((X).mutx));
    #else
	#error Must define _THREAD_MODEL in thread.h
    #endif
#else	/* not multi-thread */
    #define _THREAD_MODEL _THREAD_MODEL_NONE

#endif

#if _THREAD_MODEL == _THREAD_MODEL_LOCKS
    /* Macros that declare a semaphore.
     */
    #define DECLARE_LOCK(X) _lock_t X;
    #define DECLARE_STATIC_LOCK(X) static _lock_t X;
    #define DECLARE_EXTERNAL_LOCK(X) extern _lock_t X;
#else
    /* Semaphore declarations functions are no-ops in single-thread systems.
     */
    #define DECLARE_LOCK(X)
    #define DECLARE_STATIC_LOCK(X)
    #define DECLARE_EXTERNAL_LOCK(X)

    #define LOCK(X)
    #define UNLOCK(X)
    #define INIT_LOCK(X)
    #define DELETE_LOCK(X)
#endif

#ifdef __CPLUSPLUS__
}
#endif

#endif /*_THREAD_H*/
