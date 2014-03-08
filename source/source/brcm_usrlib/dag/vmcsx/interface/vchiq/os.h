#ifndef VCHIQ_OS_H
#define VCHIQ_OS_H

// These APIs are now deprecated. Please use the equivalent
// functions from interface/vcos/vcos.h wherever possible.

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NDEBUG
#define CHECK(x) {int _check = (x); assert(_check);}
#else
#define CHECK(x) (x)
#endif

#include "interface/vcos/vcos.h"

typedef VCOS_EVENT_T OS_EVENT_T;
typedef VCOS_MUTEX_T OS_MUTEX_T;

VCOS_STATIC_INLINE
void os_mutex_create(OS_MUTEX_T *mutex) {
   VCOS_STATUS_T st = vcos_mutex_create(mutex,"");
   vcos_assert(st == VCOS_SUCCESS);
}

VCOS_STATIC_INLINE
void os_mutex_acquire(OS_MUTEX_T *mutex) {
   vcos_mutex_lock(mutex);
}

VCOS_STATIC_INLINE
void os_mutex_release(OS_MUTEX_T *mutex) {
   vcos_mutex_unlock(mutex);
}

VCOS_STATIC_INLINE
void os_mutex_destroy(OS_MUTEX_T *mutex) {
   vcos_mutex_delete(mutex);
}

extern
void os_thread_begin(VCOS_THREAD_T *thread, void (*func)(void*), int stack, void *data, const char *name);

VCOS_STATIC_INLINE
void os_event_create(OS_EVENT_T *event) {
   VCOS_STATUS_T st;
   st = vcos_event_create(event,0);
   assert(st==VCOS_SUCCESS);
}

VCOS_STATIC_INLINE
void os_event_signal(OS_EVENT_T *event) {
   vcos_event_signal(event);
}

VCOS_STATIC_INLINE
void os_event_wait(OS_EVENT_T *event) {
   vcos_event_wait(event);
}

VCOS_STATIC_INLINE
void os_event_destroy(OS_EVENT_T *event) {
   vcos_event_delete(event);
}

// Routine to init the local OS stack - called from vchi_init
extern int32_t os_init( void );

// Routine to obtain a global semaphore.
// Note: this is for thread safety from initialisation code. Use your own locks outside of initialisation.
int32_t os_semaphore_obtain_global( void );

// Routine to release a global semaphore.
int32_t os_semaphore_release_global( void );

#ifdef __cplusplus
}
#endif
#endif
