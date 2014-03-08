/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  chip driver
File     :  $RCSfile: $
Revision :  $Revision: $

Types and API for khronos client - VCOS based platform
=============================================================================*/

#ifndef KHRN_CLIENT_PLATFORM_VCOS_VCHIQ_H
#define KHRN_CLIENT_PLATFORM_VCOS_VCHIQ_H

#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos_reentrant_mutex.h"

typedef VCOS_STATUS_T KHR_STATUS_T;
#define KHR_SUCCESS VCOS_SUCCESS

/*
   mutex
*/

typedef VCOS_REENTRANT_MUTEX_T PLATFORM_MUTEX_T;

/* return !VCOS_SUCCESS on failure */
VCOS_STATIC_INLINE
KHR_STATUS_T platform_mutex_create(PLATFORM_MUTEX_T *mutex) {
  return vcos_reentrant_mutex_create(mutex, NULL);
}

VCOS_STATIC_INLINE
void platform_mutex_destroy(PLATFORM_MUTEX_T *mutex) {
   vcos_reentrant_mutex_delete(mutex);
}

VCOS_STATIC_INLINE
void platform_mutex_acquire(PLATFORM_MUTEX_T *mutex) {
   vcos_reentrant_mutex_lock(mutex);
}

VCOS_STATIC_INLINE
void platform_mutex_release(PLATFORM_MUTEX_T *mutex) {
   vcos_reentrant_mutex_unlock(mutex);
}

/*
   named counting semaphore
*/

typedef VCOS_NAMED_SEMAPHORE_T PLATFORM_SEMAPHORE_T;

/* return !VCOS_SUCCESS on failure */

extern
KHR_STATUS_T khronos_platform_semaphore_create(PLATFORM_SEMAPHORE_T *sem, int name[3], int count);

VCOS_STATIC_INLINE
void khronos_platform_semaphore_destroy(PLATFORM_SEMAPHORE_T *sem) {
   vcos_named_semaphore_delete(sem);
}

VCOS_STATIC_INLINE
void khronos_platform_semaphore_acquire(PLATFORM_SEMAPHORE_T *sem) {
   vcos_named_semaphore_wait(sem);
}

VCOS_STATIC_INLINE
void khronos_platform_semaphore_release(PLATFORM_SEMAPHORE_T *sem) {
   vcos_named_semaphore_post(sem);
}

/*
   thread-local storage
*/

typedef VCOS_TLS_KEY_T PLATFORM_TLS_T;

/* return !VCOS_SUCCCESS on failure */
VCOS_STATIC_INLINE
KHR_STATUS_T platform_tls_create(VCOS_TLS_KEY_T *key) {
   return vcos_tls_create(key);
}

#define platform_tls_destroy(tls) vcos_tls_delete(tls)

extern void platform_tls_remove(PLATFORM_TLS_T tls);

/* This has to be per-platform because different platforms do
 * thread attachment differently.
 */
extern void *platform_tls_get(PLATFORM_TLS_T tls);


#define platform_tls_set(tls, v) vcos_tls_set(tls, v)
#define platform_tls_remove(tls) vcos_tls_set(tls,NULL)

#endif

