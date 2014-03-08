/*=========================================================
 Copyright © 2008 Broadcom Europe Limited. All rights reserved.

 Project  :  VMCS Host Apps
 Module   :  Platform framework test application
 File     :  $Id: $

 FILE DESCRIPTION:
 Mutex tests for vcos test application
 =========================================================*/

#include <string.h>  /* for strcpy/cmp, etc. */
#include <stdio.h>
#include <stdlib.h>

#include "interface/vcos/vcos.h"
#include "vcos_test.h"

static void *lock_helper(void *p)
{
   VCOS_MUTEX_T *m = p;
   int locked = vcos_mutex_is_locked(m);

   return (void*)locked;
}

static int mutex_is_locked(VCOS_MUTEX_T *mutex)
{
   VCOS_THREAD_T t;
   int locked;
   void *ptr = &locked;
   vcos_thread_create(&t, "mtxlck", NULL, lock_helper, mutex);
   vcos_thread_join(&t,ptr);
   return locked;
}

static int basic_mutex_test(void)
{
   VCOS_MUTEX_T m;
   VCOS_STATUS_T st;
   st = vcos_mutex_create(&m,"test");
   vcos_assert(st == VCOS_SUCCESS);

   if (mutex_is_locked(&m))
   {
      vcos_log("Mutex is created locked");
      vcos_assert(0);
      vcos_mutex_delete(&m);
      return 0;
   }

   vcos_mutex_lock(&m);

   if (!mutex_is_locked(&m))
   {
      vcos_log("Mutex not locked after locking");
      vcos_assert(0);
      vcos_mutex_delete(&m);
      return 0;
   }

   vcos_mutex_unlock(&m);

   if (mutex_is_locked(&m))
   {
      vcos_assert(0);
      vcos_log("Could not unlock mutex");
      vcos_mutex_delete(&m);
      return 0;
   }

   // so, mutex is unlocked now. check trying works
   if (vcos_mutex_trylock(&m) != VCOS_SUCCESS)
   {
      vcos_assert(0);
      vcos_log("Failed to try to lock mutex");
      return 0;
   }
   if (!mutex_is_locked(&m))
   {
      vcos_assert(0);
      vcos_log("Trying to lock did not lock");
      return 0;
   }
   vcos_mutex_unlock(&m);

   vcos_mutex_delete(&m);

   return 1;
}

static int basic_reentrant_mutex_test(void)
{
   VCOS_REENTRANT_MUTEX_T m;
   VCOS_STATUS_T st;
   st = vcos_reentrant_mutex_create(&m,"test");
   vcos_assert(st == VCOS_SUCCESS);

   vcos_reentrant_mutex_lock(&m);
   vcos_reentrant_mutex_lock(&m);
   vcos_reentrant_mutex_lock(&m);

   vcos_reentrant_mutex_unlock(&m);
   vcos_reentrant_mutex_unlock(&m);
   vcos_reentrant_mutex_unlock(&m);

   vcos_reentrant_mutex_delete(&m);

   return 1;
}

void run_mutex_tests(int *run, int *passed)
{
   RUN_TEST(run,passed,basic_mutex_test);
   RUN_TEST(run,passed,basic_reentrant_mutex_test);
}



