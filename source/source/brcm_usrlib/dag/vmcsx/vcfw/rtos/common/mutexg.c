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

/*
 * $Log$
 * Revision 1.8  1999/09/08 16:05:07  marks
 * Extended possible static semaphores at customer request.
 *
 * Revision 1.7  1999/05/03  23:22:45  marks
 * chanded _mutex_* to _mwmutex_* because of collision with Precise MQX
 *
 * Revision 1.6  1999/05/03  23:14:01  marcusm
 * Don't release semaphore unless successful in obtaining it.  We get
 * errors during system initialization which are ok to ignore.
 *
 * Revision 1.5  1999/03/09  23:05:38  marcusm
 * moved __start_kernel out into it's own file
 *
 * Revision 1.4  1999/02/17  20:20:44  marcusm
 * Fixed bug in "unlock".  Need to check for self == 0 otherwise there is
 * a get/put imbalance in the value of the semaphore.
 *
 * Revision 1.3  1999/02/11  17:56:57  marks
 * semaphore name was overflowing max name size
 *
 * Revision 1.2  1998/09/30  18:39:54  marks
 * PowerPC now using TOP OF STACK for thread local storage and errno
 *
 * Revision 1.1  1998/07/13  22:15:56  marks
 * Initial revision
 *
 */

//
// NUCIFACE.C:
//
// MetaWare High C/C++ interface to the Accelerated Technology
// Incorporated, Nucleus PLUS real-time kernel.
//
// Modified from Nucleus NUCIFACE.C to use vcos/threadx latches.
//
//
//
// WARNING/NOTE:
//
//	To initialize the MetaWare C/C++ runtime and to execute
//	C++ global constructors correctly your application MUST
//	call _mw_initialize() just before Application_Initialize()
//
//
//  PUBLIC INTERFACE
//
//	void __start_kernel(void);
//	void _mwmutex_create(_lock_t*);
//	void _mwmutex_delete(_lock_t*);
//	void _mwmutex_lock(_lock_t);
//	void _mwmutex_unlock(_lock_t);
//
//
//  DESCRIPTION
//
//	void __start_kernel(void);
//
//	The MetaWare crtk.s startup file calls the function
//	named __start_kernel() after initializing the small data
//	base registers and the stack pointer.  All other startup
//	functionality must be done here.
//
//	You must completely initialize the Nucleus kernel prior
//	to invoking the C++ constructors so that the constructors
//	can safely invoke any reasonable Nucleus services.
//
// 	NOTE:
// 	To initialize the MetaWare C/C++ runtime and to execute
//	C++ global constructors correctly your application MUST
//	call __init() AFTER initializing your kernel, board,
//	zeroing .bss/.sbss and PRIOR to executing your application.
//	If you wish to be ANSI C runtime conformant you should
//	register __fini() with atexit() and call exit() when your
//	application terminates.
//	void __start_kernel(void);
//
//
//	void _mwmutex_create(_lock_t*);
//	void _mwmutex_delete(_lock_t*);
//	void _mwmutex_lock(_lock_t);
//	void _mwmutex_unlock(_lock_t);
//
//	The MetaWare thread safe C/C++ runtime library will call
//	these semaphore functions to guard global data necessary
//	for implementing stdio, malloc, free and the like.
//
//
//	*NOTE*:
//	In order to get this functionality you must compile and
//	link everything with the -Hthread command line option.
//
//

#include <vcfw/rtos/rtos.h>
#include "interface/vcos/vcos.h"

#undef __PANIC
#ifdef _VIDEOCORE
#   include <assert.h>
#   define __PANIC()	assert(0)
#endif
//
// The MetaWare thread safe runtime will use semaphores and
// thread local variables to maintain global state.  The
// following routines implement interfaces that the thread
// safe C/C++ runtime will call.
//
// When the MetaWare thread safe High C/C++ runtime wants to
// create, destroy, lock or unlock semaphores it will call
// the routines implemented herein.
//

typedef struct _mw_mutex
{
    struct _mw_mutex *next;	// Free list
    int count;			// To allow locking when already locked
    void * th_id;		// thread ID of thread who has lock
    RTOS_LATCH_T latch;		// latch
} *_lock_t;

    //
    // FIXED number of MetaWare runtime semaphores.
    // There's a mutex to guard malloc()/free() and the C++
    // new and delete operators.  We allocate a new sema for
    // each open FILE and C++ iostream.  There's a semaphore
    // for atexit() in the unlikely event that you wish to
    // provide ANSI program termination.

#if _REENTRANT
static struct _mw_mutex svec[52] =
{
    {&svec[1]},	 {&svec[2]},  {&svec[3]},  {&svec[4]},
    {&svec[5]},	 {&svec[6]},  {&svec[7]},  {&svec[8]},
    {&svec[9]},	 {&svec[10]}, {&svec[11]}, {&svec[12]},
    {&svec[13]}, {&svec[14]}, {&svec[15]}, {&svec[16]},
    {&svec[17]}, {&svec[18]}, {&svec[19]}, {&svec[20]},
    {&svec[21]}, {&svec[22]}, {&svec[23]}, {&svec[24]},
    {&svec[25]}, {&svec[26]}, {&svec[27]}, {&svec[28]},
    {&svec[29]}, {&svec[30]}, {&svec[31]}, {&svec[32]},
    {&svec[33]}, {&svec[34]}, {&svec[35]}, {&svec[36]},
    {&svec[37]}, {&svec[38]}, {&svec[39]}, {&svec[40]},
    {&svec[41]}, {&svec[42]}, {&svec[43]}, {&svec[44]},
    {&svec[45]}, {&svec[46]}, {&svec[47]}, {&svec[48]},
    {&svec[49]}, {&svec[50]}, {&svec[51]}, {0}
};

static _lock_t free_list = &svec[0];
static RTOS_LATCH_T free_mutex;
static char free_mutex_initialized = 0;

static void fatal_allocation_error()
{
#ifdef _VIDEOCORE
    assert(0);
#endif
}

static _lock_t alloc_sema()
{
    _lock_t sem;
    if (! free_mutex_initialized)
    {
	free_mutex_initialized = 1;
	free_mutex = rtos_latch_unlocked();
	}
    rtos_latch_get( &free_mutex );
    if (free_list != 0) {
	sem = free_list;
	free_list = sem->next;
	}
    else{
	fatal_allocation_error();
	sem = 0;
	}
    rtos_latch_put( &free_mutex );
    return sem;
}

static _Inline void free_sema(_lock_t sem)
{
   rtos_latch_get( &free_mutex );
   sem->next = free_list;
   free_list = sem;
   rtos_latch_put( &free_mutex );
}
#endif

    // Create a mutex semaphore
void _mwmutex_create(_lock_t *mutex_ptr)
{
#if _REENTRANT
   _lock_t sem = alloc_sema();
   if (sem == 0) __PANIC();
   sem->latch = rtos_latch_unlocked();
   sem->count = 0;
   sem->th_id = 0;
   sem->next = 0;
   *mutex_ptr = sem;
#endif
}

    // Destroy the mutex semaphore
void _mwmutex_delete(_lock_t *mutex_ptr)
{
#if _REENTRANT
    if (mutex_ptr != 0 && *mutex_ptr != 0)
    {
	_lock_t sem = *mutex_ptr;
	sem->count = 0;
	sem->th_id = 0;
	// JV: TODO Should we assert latch has nothing waiting on it here ??
	sem->latch = rtos_latch_unlocked();
	free_sema(sem);
    }
    *mutex_ptr = 0;
#endif
}

    // Wait on the mutex semaphore if we don't already own it.
void _mwmutex_lock(_lock_t sem)
{
#if _REENTRANT
   if (sem != 0)
   {
      void *self = vcos_thread_current();
      if (sem->th_id == self)
         sem->count++;
      else
      {
	 rtos_latch_get( &sem->latch );
	 sem->th_id = self;
	 sem->count = 1;
      }
   }
#endif
}

    // Signal the mutex semaphore or decrement the wait count
    // if we called _mwmutex_lock more than once.
void _mwmutex_unlock(_lock_t sem)
{
#if _REENTRANT
   if (sem != 0)
   {
      // void *self = vcos_thread_current();
      // JV: TODO Shoud we assert((sem->th_id == self) ??

      if( --sem->count <= 0)
      {
	 sem->count = 0;
	 sem->th_id = 0;
	 rtos_latch_put( &sem->latch );
      }
   }
#endif
}
