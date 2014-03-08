/*=========================================================
 Copyright © 2008 Broadcom Europe Limited. All rights reserved.

 Project  :  VMCS Host Apps
 Module   :  Platform framework test application
 File     :  $Id: $

 FILE DESCRIPTION:
 Thread tests for platform framework test application
 =========================================================*/

#include <string.h>  /* for strcpy/cmp, etc. */
#include <stdio.h>
#include <stdlib.h>

#include "interface/vcos/vcos.h"
#include "vcos_test.h"

static long expected;
static int fail;

static void *entry(void* cxt)
{
   VCOS_UNSIGNED pri0, pri1;
   VCOS_THREAD_T *self = vcos_thread_current();
   if ((long)cxt != expected)
   {
      vcos_assert(0);
      fail = 1;
   }

   pri0 = vcos_thread_get_priority(self);
   vcos_assert(pri0 == expected);

   vcos_thread_set_priority(self, pri0);
   pri1 = vcos_thread_get_priority(self);
   vcos_assert(pri0 == pri1);

#ifdef _VIDEOCORE
   {
      // switch to other core - pri and aff should remain orthogonal
      VCOS_UNSIGNED pri, aff;
      vcos_thread_set_affinity(self, VCOS_AFFINITY_CPU1);
      pri = vcos_thread_get_priority(self);
      vcos_assert(pri == expected);
      aff = vcos_thread_get_affinity(self);
      vcos_assert(aff == VCOS_AFFINITY_CPU1);

      // changing pri on second core should still work
      pri0 = vcos_thread_get_priority(self);
      vcos_assert(pri0 == expected);

      vcos_thread_set_priority(self, pri0);
      pri1 = vcos_thread_get_priority(self);
      vcos_assert(pri0 == pri1);

      vcos_thread_set_affinity(self, VCOS_AFFINITY_CPU0);
      pri = vcos_thread_get_priority(self);
      vcos_assert(pri == expected);
      aff = vcos_thread_get_affinity(self);
      vcos_assert(aff == VCOS_AFFINITY_CPU0);
   }
#endif

   

   return 0;
}

/* Check we can create and destroy some threads.
 */
static int create_test(void)
{
   VCOS_THREAD_T thread;

   /* create some threads in rapid succession to catch problems with
    * tear down
    */
   int i;
   fail = 0;

   for (i=VCOS_THREAD_PRI_MIN; i<=VCOS_THREAD_PRI_MAX; i++)
   {
      VCOS_STATUS_T st;
      VCOS_THREAD_ATTR_T attrs;
      vcos_thread_attr_init(&attrs);
      vcos_thread_attr_setpriority(&attrs, i);
      expected = i;
      st = vcos_thread_create(&thread, "test", &attrs, entry, (void*)i);
      if (st != VCOS_SUCCESS)
      {
         vcos_log("Failed to startup thread %d", i);
         return 0;
      }
      vcos_thread_join(&thread,0);
   }
   return fail ? 0 : 1;
}

static void *entry2(void *cxt)
{
   VCOS_THREAD_T *cur = vcos_thread_current();
   if (cur != cxt)
      fail = 1;
   vcos_sleep(100);
   return 0;
}

/* Does vcos_thread_current() return the right value? */

#define N_THREADS 4

static int current_thread_test(void)
{
   int i;
   VCOS_THREAD_T threads[N_THREADS];
   VCOS_STATUS_T st;
   fail = 0;
   for (i=0; i<N_THREADS; i++)
   {
      st = vcos_thread_create(threads+i, "test", NULL, entry2, threads+i);
      vcos_assert(st == VCOS_SUCCESS);
   }

   for (i=0; i<N_THREADS; i++)
   {
      vcos_thread_join(threads+i, NULL);
   }
   return fail ? 0 : 1;
}

static VCOS_TLS_KEY_T tls_key0;
static VCOS_TLS_KEY_T tls_key1;

static void *tls_entry(void *arg)
{
   int i;
   int me = (int)arg;
   for (i=0; i<32; i++)
   {
      vcos_tls_set(tls_key0, (void*)(me*i) );
      if (vcos_tls_get(tls_key0) != (void*)(me*i)) {
         vcos_assert(0);
         fail = 1;
      }
      vcos_tls_set(tls_key0, (void*)(me*i*2));
      if (vcos_tls_get(tls_key0) != (void*)(me*i*2)) {
         vcos_assert(0);
         fail = 1;
      }
   }

   return 0;
}

static int tls_test(void)
{
   VCOS_STATUS_T st = vcos_tls_create(&tls_key0);
   int i;
   VCOS_THREAD_T threads[8];

   st = vcos_tls_create(&tls_key1);
   fail = 0;
   vcos_assert(st == VCOS_SUCCESS);

   for (i=0; i<8; i++)
   {
      st = vcos_thread_create(threads+i, "thread", NULL, tls_entry, (void*)i);
      vcos_assert(st == VCOS_SUCCESS);
   }

   for (i=0; i<8; i++)
      vcos_thread_join(threads+i,NULL);

   vcos_tls_delete(tls_key0);
   vcos_tls_delete(tls_key1);

   return fail ? 0 : 1;
}

#ifdef VCOS_NO_PREEMPT
// run two threads at the same priority; one of them turns off preemption,
// and the other one should stop running, and then resume.

static void *preemption_worker(void *);

static int preemption(void)
{
   VCOS_STATUS_T st;
   VCOS_THREAD_T t0, t1;
   VCOS_THREAD_ATTR_T attrs;
   vcos_thread_attr_init(&attrs);
   vcos_thread_attr_settimeslice(&attrs,1);

   st = vcos_thread_create(&t0, "t0", &attrs, preemption_worker, (void*)0);
   vcos_assert(st == VCOS_SUCCESS);

   st = vcos_thread_create(&t1, "t1", &attrs, preemption_worker, (void*)1);
   vcos_assert(st == VCOS_SUCCESS);

   int t0_ticks, t1_ticks;

   vcos_thread_join(&t0, (void**)&t0_ticks);
   vcos_thread_join(&t1, (void**)&t1_ticks);

   vcos_assert(t0_ticks <= t1_ticks/2);
   int failed = 0;
   if (t0_ticks > t1_ticks/2)
   {
      failed = 1; // we didn't stop it
   }
    
   if (t0_ticks < t1_ticks/8)
   {
      failed = 1; // it didn't run at all hardly!
   }

   return failed ? 0 : 1;
}

static void *preemption_worker(void *p)
{
   int start = vcos_get_ms();
   int changed_preemption = 0;
   int count = 0;
   VCOS_UNSIGNED pe = 0;
   while (vcos_get_ms() - start < 100)
   {
      if (p && (vcos_get_ms() - start >= 30) &&
          !changed_preemption)
      {
         pe = vcos_change_preemption(VCOS_NO_PREEMPT);
         changed_preemption = 1;
      }
      count++;
   }
   if (changed_preemption)
      vcos_change_preemption(pe);

   return (void*)count;
}
#endif


#ifdef VCOS_HAVE_THREAD_REAPER
static void *quick_reapee(void *cxt)
{
   vcos_log("quick repee returning %d", (int)cxt);
   return NULL;
}

static void reap_cb(void *cxt)
{
   VCOS_SEMAPHORE_T *sem = cxt;
   vcos_semaphore_post(sem);
}

#define MAX_REAP 4
int reaper(void)
{
   VCOS_SEMAPHORE_T sem;
   VCOS_THREAD_T *t = vcos_malloc(sizeof(*t)*MAX_REAP,"threads");
   int i;
   vcos_semaphore_create(&sem,"",0);
   vcos_assert(t);

   vcos_log("Starting reaper tests");

   for (i=1; i<MAX_REAP; i++)
   {
      int j;
      vcos_log("*** %d reapees", i);
      for (j=0; j<i; j++)
      {
         vcos_thread_create(t+j, "reapee", NULL, quick_reapee, (void*)j);
      }

      for (j=0; j<i; j++)
      {
         vcos_log("reap %d of %d", j, i);
         vcos_thread_reap(t+j,reap_cb,&sem);
         // check our callback actually gets called - normal code
         // would not do anything as daft as this.
         vcos_semaphore_wait(&sem);
         // if we don't get here, the callback was not invoked!
         vcos_log("terminated %d of %d", j, i);
      }
   }
   vcos_semaphore_delete(&sem);
   vcos_free(t);

   return 1;
}
#endif

void run_thread_tests(int *run, int *passed)
{
#ifdef VCOS_HAVE_THREAD_REAPER
   RUN_TEST(run,passed,reaper);
#endif
   RUN_TEST(run,passed,create_test);
   RUN_TEST(run,passed,current_thread_test);
   RUN_TEST(run,passed,tls_test);
#ifdef VCOS_NO_PREEMPT
   RUN_TEST(run,passed,preemption);
#endif
}


