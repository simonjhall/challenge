/*=============================================================================
Copyright (c) 2005 Broadcom Europe Limited. All rights reserved.

Project  :  Powerman
Module   :  Interface to the power manager
File     :  $RCSfile: powerman.h,v $
Revision :  $Revision$

FILE DESCRIPTION:
Contains the protypes for the OS functions.

These APIs are now deprecated. Please use the equivalent
functions from interface/vcos/vcos.h wherever possible.
=============================================================================*/

#ifndef OS_H_
#define OS_H_

#include "interface/vchi/os/types.h"
#include "interface/vcos/vcos.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
Global typedefs
******************************************************************************/


//Opaque handle for a condition variable
typedef struct opaque_os_cond_t *OS_COND_T;
//Opaque handle for a HISR
typedef struct opaque_os_hisr_t *OS_HISR_T;

/* Semaphores (a mutex) have this extra debugging aid added to ensure
 * that they don't accidentally get copied.
 */
typedef struct OS_SEMAPHORE_T
{
   struct OS_SEMAPHORE_T *self;  /* Check for copying */
   VCOS_SEMAPHORE_T sem;
} OS_SEMAPHORE_T;

typedef VCOS_SEMAPHORE_T OS_COUNT_SEMAPHORE_T;
typedef VCOS_EVENT_FLAGS_T OS_EVENTGROUP_T;

typedef void (*OS_THREAD_FUNC_T)(unsigned, void *);
typedef void (*OS_HISR_FUNC_T)();

typedef enum
{
   OS_SEMAPHORE_TYPE_MIN,

   OS_SEMAPHORE_TYPE_SUSPEND,
   OS_SEMAPHORE_TYPE_BUSY_WAIT,

   OS_SEMAPHORE_TYPE_MAX

} OS_SEMAPHORE_TYPE_T;

#if defined(WIN32) || defined(__GNUC__) || defined __SYMBIAN32__ || defined(__CC_ARM)
static __inline int os_min_generic(int x, int y)
{
   return x < y ? x : y;
}

static __inline int os_max_generic(int x, int y)
{
   return x > y ? x : y;
}

static __inline unsigned int os_count_generic(unsigned int x)
{
   x = (x >> 1 & 0x55555555) + (x & 0x55555555);
   x = (x >> 2 & 0x33333333) + (x & 0x33333333);
   x = (x >> 4 & 0x0f0f0f0f) + (x & 0x0f0f0f0f);
   x = (x >> 8 & 0x00ff00ff) + (x & 0x00ff00ff);
   x = (x >> 16 & 0x0000ffff) + (x & 0x0000ffff);

   return x;
}

static __inline int os_msb_generic(int val)
{
   unsigned int msb=31;
   if (val==0) return -1;
   while ((val&(1<<msb))==0)
      msb--;
   return (int)msb;
}

static __inline int os_bmask_generic(int a, unsigned int n) {
   return (a) & ((int)(1<<((n)&31))-1);
}

static __inline unsigned int os_brev_generic(unsigned int val) {
   unsigned int i, res = 0;
   for (i = 0; i < 32; ++i)
      if ((val & (1<<i))!=0) res |= (1<<(31-i));
   return res;
}

#define OS_MIN(x, y)    os_min_generic(x, y)
#define OS_MAX(x, y)    os_max_generic(x, y)
#define OS_COUNT(x)     os_count_generic(x)
#define OS_MSB(x)       os_msb_generic(x)
#define OS_BREV(x)      os_brev_generic(x)
#define OS_BMASK(x, y)  os_bmask_generic(x, y)
#else
#define OS_MIN(x, y)    _min(x, y)
#define OS_MAX(x, y)    _max(x, y)
#define OS_COUNT(x)     _count(x)
#define OS_MSB(x)       _msb(x)
#define OS_BREV(x)      _brev(x)
#define OS_BMASK(x, y)  _bmask(x, y)
#endif

#if defined _VIDEOCORE || defined WIN32 || defined __SYMBIAN32__
// Define for platforms that want os_assert_failure() called rather than direct use of <assert.h>
#define OS_ASSERT_FAILURE
#endif

#define OS_ALIGN_DEFAULT   0

/******************************************************************************
Global functions
******************************************************************************/

// Routine to init the local OS stack - called from vchi_init
extern int32_t os_init( void );

// Routines called when VCHI receives a DISCONNECT message
extern int32_t os_halt( void );
extern int32_t os_suspend( void );

// Routine to create a semaphore with a maximum count of 1.
//
VCOS_STATIC_INLINE
int32_t os_semaphore_create(OS_SEMAPHORE_T *semaphore,
                            const OS_SEMAPHORE_TYPE_T type) {
   VCOS_STATUS_T st = vcos_semaphore_create(&semaphore->sem,0,1);
   semaphore->self = semaphore;

   /* Note: - as far as I can tell, busywait and suspend mutexes
    * both end up as the same underlying OS type in all OSes -
    * a suspend mutex.
    */
   vcos_assert(type == OS_SEMAPHORE_TYPE_SUSPEND ||
               type == OS_SEMAPHORE_TYPE_BUSY_WAIT);

   return (st == VCOS_SUCCESS?0:-1);
}

// Routine to destroy a mutex
VCOS_STATIC_INLINE
int32_t os_semaphore_destroy( OS_SEMAPHORE_T *semaphore ) {
   vcos_assert(semaphore == semaphore->self);
   vcos_semaphore_delete(&semaphore->sem);
   return 0;
}

// Routine to obtain a mutex
VCOS_STATIC_INLINE
int32_t os_semaphore_obtain( OS_SEMAPHORE_T *semaphore ) {
   vcos_assert(semaphore == semaphore->self);
   vcos_semaphore_wait(&semaphore->sem);
   return 0;
}

// Routine to release a semaphore
VCOS_STATIC_INLINE
int32_t os_semaphore_release( OS_SEMAPHORE_T *semaphore ) {
   vcos_assert(semaphore == semaphore->self);
   vcos_semaphore_post(&semaphore->sem);
   return 0;
}

// Routine to check if a semaphore is obtained
VCOS_STATIC_INLINE
int32_t os_semaphore_obtained( OS_SEMAPHORE_T *semaphore ) {
   VCOS_STATUS_T st = vcos_semaphore_trywait(&semaphore->sem);
   if (st == VCOS_SUCCESS) {
      vcos_semaphore_post(&semaphore->sem);
      return 0;
   }
   else
      return 1;
}

// Routine to create a counting semaphore
VCOS_STATIC_INLINE
int32_t os_count_semaphore_create( OS_COUNT_SEMAPHORE_T *semaphore,
                                   const int32_t count,
                                   const OS_SEMAPHORE_TYPE_T type ) {
   VCOS_STATUS_T status = vcos_semaphore_create(semaphore, "", count);
   /* LGD - as far as I can tell, busywait and suspend mutexes
    * both end up as the same underlying OS type in all OSes -
    * a suspend mutex.
    */
   vcos_assert(type == OS_SEMAPHORE_TYPE_SUSPEND ||
               type == OS_SEMAPHORE_TYPE_BUSY_WAIT);
   return (status == VCOS_SUCCESS?0:-1);

}


// Routine to destroy a counting semaphore
VCOS_STATIC_INLINE
int32_t os_count_semaphore_destroy( OS_COUNT_SEMAPHORE_T *semaphore ) {
   vcos_semaphore_delete(semaphore);
   return 0;
}

// Routine to obtain a counting semaphore
VCOS_STATIC_INLINE
int32_t os_count_semaphore_obtain( OS_COUNT_SEMAPHORE_T *semaphore,
                                   const int32_t block ) {
   if (block) {
      vcos_semaphore_wait(semaphore);
      return 0;
   }
   else {
      VCOS_STATUS_T st = vcos_semaphore_trywait(semaphore);
      return st == VCOS_SUCCESS ? 0:-1;
   }
}


// Routine to release a counting semaphore
VCOS_STATIC_INLINE
int32_t os_count_semaphore_release( OS_COUNT_SEMAPHORE_T *semaphore ) {
   vcos_semaphore_post(semaphore);
   return 0;
}

// Routine to create an event group
VCOS_STATIC_INLINE
int32_t os_eventgroup_create ( OS_EVENTGROUP_T *eventgroup, const char *name ) {
   VCOS_STATUS_T st = vcos_event_flags_create(eventgroup, name);
   return st == VCOS_SUCCESS ? 0 : -1;
}

// Routine to destroy an event group
VCOS_STATIC_INLINE
int32_t os_eventgroup_destroy ( OS_EVENTGROUP_T *eventgroup ) {
   vcos_event_flags_delete(eventgroup);
   return 0;
}

// Routine to signal a single element of an event group
VCOS_STATIC_INLINE
int32_t os_eventgroup_signal ( OS_EVENTGROUP_T *eventgroup, int32_t index ) {
   vcos_assert(index >=0 && index <= 31);
   vcos_event_flags_set(eventgroup, (1<<index), VCOS_OR);
   return 0;
}

// Routine to wait for one or more elements of an event group to trigger
VCOS_STATIC_INLINE
int32_t os_eventgroup_retrieve ( OS_EVENTGROUP_T *eventgroup, uint32_t *events ) {
   vcos_event_flags_get(eventgroup, (VCOS_UNSIGNED)-1, VCOS_OR_CONSUME, VCOS_SUSPEND, (VCOS_UNSIGNED*)events);
   return 0;
}

// Routine to obtain a global semaphore.
// Note: this is for thread safety from initialisation code. Use your own locks outside of initialisation.
int32_t os_semaphore_obtain_global( void );
// Routine to release a global semaphore.
int32_t os_semaphore_release_global( void );

#if VCOS_HAVE_RTOS
// Routine to create and start a thread or task
extern int32_t os_thread_start( VCOS_THREAD_T *thread, OS_THREAD_FUNC_T func, void *arg, uint32_t stack_size, const char *name );

// Routine to set thread priority. priority should be < 16; lower numbers mean higher priority
VCOS_STATIC_INLINE
int32_t os_thread_set_priority( VCOS_THREAD_T *thread, uint32_t priority ) {
   vcos_thread_set_priority(thread, priority);
   return 0;
}

// Routine to determine whether a given thread is currently executing
VCOS_STATIC_INLINE
int32_t os_thread_is_running( VCOS_THREAD_T *thread ) {
   return vcos_thread_current() == thread;
}
#endif

/*
 * These are an approximation of a POSIX pthread_cond_t implementation. However,
 * because of the difficulty in implementing this on some platforms, there
 * are the following restrictions:
 *
 * - The same semaphore must always be used for a given OS_COND_T, and this
 *   is specified on creation (as well as on waiting).
 * - The signal and broadcast functions may need to obtain the semaphore during
 *   processing - the caller must indicate whether the semaphore is already
 *   obtained.
 * - When signal is called, the thread woken may either be the first
 *   thread that waited, or it may be determined by the scheduling policy.
 *
 */

// Routine to create a condition variable
extern int32_t os_cond_create( OS_COND_T *waitq, OS_SEMAPHORE_T *semaphore );

// Routine to destroy a condition variable
extern int32_t os_cond_destroy( OS_COND_T *waitq );

// Routine to wake one waiter on a condition variable
extern int32_t os_cond_signal( OS_COND_T *waitq, bool_t sem_claimed );

// Routine to wake all waiters on a condition variable
extern int32_t os_cond_broadcast( OS_COND_T *waitq, bool_t sem_claimed );

// Routine to wait for a signal on a condition variable
extern int32_t os_cond_wait( OS_COND_T *waitq, OS_SEMAPHORE_T *semaphore );

// Routine to create a HISR
extern int32_t os_hisr_create ( OS_HISR_T *hisr, OS_HISR_FUNC_T func, const char *name );

// Routine to activate a HISR
extern int32_t os_hisr_activate ( OS_HISR_T *hisr );

// Routine to allocate / free memory
VCOS_STATIC_INLINE
void *os_malloc( const uint32_t size, const uint32_t align, const char *name ) {
   return vcos_malloc_aligned(size, align, name);
}

VCOS_STATIC_INLINE
int32_t os_free( void *ptr ) {
   vcos_free(ptr);
   return 0;
}

// Routine to put the current thread to sleep for x ms
VCOS_STATIC_INLINE
int32_t os_sleep( const uint32_t time_in_ms ) {
   vcos_sleep(time_in_ms);
   return 0;
}

// Routine to put the current thread to sleep for x ms
VCOS_STATIC_INLINE
int32_t os_delay( const uint32_t time_in_ms ) {
   vcos_sleep(time_in_ms);
   return 0;
}

// Routine to get current time in microseconds
VCOS_STATIC_INLINE
uint32_t os_get_time_in_usecs( void ) {
   return vcos_getmicrosecs();
}

// Routine to initialize OS microsecond timer
extern int32_t os_time_init( void );

// Routine to terminate OS microsecond timer
extern int32_t os_time_term( void );

// Routine to write message to log
extern void os_logging_message( const char *format, ... );

// Macro to check assertions - fails if expression==0. Use instead of <assert.h>
// void os_assert( expression );
#ifdef OS_ASSERT_FAILURE
   #ifdef NDEBUG
      #define os_assert(cond) ((void)0)
   #else
      #ifdef _VIDEOCORE
         #define os_assert(cond) (_Usually(cond) ? (void)0 : (_bkpt(),os_assert_failure(#cond, __FILE__, __func__, __LINE__)))
      #elif defined WIN32
         #define os_assert(cond) ((cond) ? (void)0 : os_assert_failure(#cond, __FILE__, __FUNCTION__, __LINE__))
      #elif __STDC_VERSION >= 199901L
         #define os_assert(cond) ((cond) ? (void)0 : os_assert_failure(#cond, __FILE__, __func__, __LINE__))
      #else
         #define os_assert(cond) ((cond) ? (void)0 : os_assert_failure(#cond, __FILE__, 0, __LINE__))
      #endif
   #endif

   // Routine called when an os_assert has failed
   extern void os_assert_failure( const char *msg, const char *file, const char *func, int line );
#else
   #include <assert.h>
   #define os_assert(cond) assert(cond)
#endif

#ifdef __cplusplus
}
#endif

#endif /* OS_H_ */

/****************************** End of file **********************************/
