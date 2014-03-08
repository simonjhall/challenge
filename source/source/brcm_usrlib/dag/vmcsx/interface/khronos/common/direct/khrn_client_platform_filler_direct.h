/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  chip driver
File     :  $RCSfile: $
Revision :  $Revision: $

Types and API for khronos client - direct based platform
=============================================================================*/

#ifndef KHRN_CLIENT_PLATFORM_DIRECT_H
#define KHRN_CLIENT_PLATFORM_DIRECT_H

#include "interface/vcos/vcos.h"

typedef int KHR_STATUS_T;
#define KHR_SUCCESS  0
/*
   mutex
*/

typedef struct PLATFORM_MUTEX_T
{
   /* nothing needed here */
#if defined(_MSC_VER) || defined(__CC_ARM)
   char dummy;    /* empty structures are not allowed */
#endif
} PLATFORM_MUTEX_T;


VCOS_STATIC_INLINE
VCOS_STATUS_T platform_mutex_create(PLATFORM_MUTEX_T *mutex) {
   UNUSED(mutex);
   // Nothing to do
   return VCOS_SUCCESS;
}

VCOS_STATIC_INLINE
void platform_mutex_destroy(PLATFORM_MUTEX_T *mutex)
{
   UNUSED(mutex);
   /* Nothing to do */
}

VCOS_STATIC_INLINE
void platform_mutex_acquire(PLATFORM_MUTEX_T *mutex)
{
   UNUSED(mutex);
   /* Nothing to do */
}

VCOS_STATIC_INLINE
void platform_mutex_release(PLATFORM_MUTEX_T *mutex)
{
   UNUSED(mutex);
   /* Nothing to do */
}

/*
   named counting semaphore
*/

typedef VCOS_NAMED_SEMAPHORE_T PLATFORM_SEMAPHORE_T;

/*
   VCOS_STATUS_T khronos_platform_semaphore_create(PLATFORM_SEMAPHORE_T *sem, int name[3], int count);

   Preconditions:
      sem is a valid pointer to an uninitialised variable
      name is a valid pointer to three elements

   Postconditions:
      If return value is KHR_SUCCESS then sem contains a valid PLATFORM_SEMAPHORE_T
*/

extern VCOS_STATUS_T khronos_platform_semaphore_create(PLATFORM_SEMAPHORE_T *sem, int name[3], int count);
extern void khronos_platform_semaphore_destroy(PLATFORM_SEMAPHORE_T *sem);
extern void khronos_platform_semaphore_acquire(PLATFORM_SEMAPHORE_T *sem);
extern void khronos_platform_semaphore_release(PLATFORM_SEMAPHORE_T *sem);

/*
   thread-local storage
*/

typedef void *PLATFORM_TLS_T;

extern VCOS_STATUS_T platform_tls_create(PLATFORM_TLS_T *tls);
extern void platform_tls_destroy(PLATFORM_TLS_T tls);
extern void platform_tls_set(PLATFORM_TLS_T tls, void *v);
extern void *platform_tls_get(PLATFORM_TLS_T tls);
extern void* platform_tls_get_check(PLATFORM_TLS_T tls);
extern void platform_tls_remove(PLATFORM_TLS_T tls);

#endif


