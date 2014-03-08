/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  chip driver
File     :  $RCSfile: $
Revision :  $Revision: $

Types and API for khronos client - linux based platform
=============================================================================*/

#ifndef KHRN_CLIENT_PLATFORM_LINUX_H
#define KHRN_CLIENT_PLATFORM_LINUX_H

#include "interface/vcos/vcos.h"

typedef int KHR_STATUS_T;
#define KHR_SUCCESS  0

typedef pthread_mutex_t PLATFORM_MUTEX_T;

VCOS_STATUS_T platform_mutex_create(PLATFORM_MUTEX_T *mtx);
void platform_mutex_destroy(PLATFORM_MUTEX_T *mutex);
void platform_mutex_acquire(PLATFORM_MUTEX_T *mutex);
void platform_mutex_release(PLATFORM_MUTEX_T *mutex);


typedef sem_t PLATFORM_SEMAPHORE_T;

VCOS_STATUS_T khronos_platform_semaphore_create(PLATFORM_SEMAPHORE_T *sem, int name[3], int count);
void khronos_platform_semaphore_destroy(PLATFORM_SEMAPHORE_T *semaphore);
void khronos_platform_semaphore_acquire(PLATFORM_SEMAPHORE_T *semaphore);
void khronos_platform_semaphore_release(PLATFORM_SEMAPHORE_T *semaphore);


typedef pthread_key_t PLATFORM_TLS_T;

VCOS_STATUS_T platform_tls_create(PLATFORM_TLS_T *tls);
void platform_tls_destroy(PLATFORM_TLS_T tls);
void platform_tls_set(PLATFORM_TLS_T tls, void *v);
void *platform_tls_get(PLATFORM_TLS_T tls);
void platform_tls_remove(PLATFORM_TLS_T tls);

#endif


