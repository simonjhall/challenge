/*=========================================================
 Copyright © 2008 Broadcom Europe Limited. All rights reserved.

 Project  :  VMCS Host Apps
 Module   :  Platform framework test application
 File     :  $Id: $

 FILE DESCRIPTION:
 Named semaphore tests for vcos test application
 =========================================================*/

#include <string.h>  /* for strcpy/cmp, etc. */
#include <stdio.h>
#include <stdlib.h>

#include "interface/vcos/vcos.h"
#include "vcos_test.h"

/* Can we create them without blowing up ? */
static int create_test()
{
   int i, j;
   for (i=1; i<8; i++)
   {
      VCOS_NAMED_SEMAPHORE_T *sems = vcos_malloc(sizeof(*sems)*i,"sems");
      vcos_assert(sems);

      for (j=0; j<i; j++)
      {
         char name[16];
         VCOS_STATUS_T st;

         vcos_snprintf(name,sizeof(name),"%d",j);
         st = vcos_named_semaphore_create(sems+j, name, 1);
         vcos_assert(st == VCOS_SUCCESS);
         (void)st;
      }

      if (i & 1)
      {
         /* free in reverse order */
         for (j=i-1; j>=0; j--)
         {
            vcos_named_semaphore_wait(sems+j);
            vcos_named_semaphore_delete(sems+j);
         }
      }
      else
      {
         /* free in created order */
         for (j=0; j<i; j++)
         {
            vcos_named_semaphore_wait(sems+j);
            vcos_named_semaphore_delete(sems+j);
         }
      }
      vcos_free(sems);
   }
   return 1;
}

static void *existing_test_worker(void *cxt)
{
   VCOS_NAMED_SEMAPHORE_T *sem = cxt;
   vcos_sleep(1000);
   vcos_named_semaphore_post(sem);
   return NULL;
}

static int open_existing(void)
{
   VCOS_NAMED_SEMAPHORE_T sem0, sem1;
   VCOS_THREAD_T th;

   int ticks;
   VCOS_STATUS_T st;

   st = vcos_named_semaphore_create(&sem0, "foo", 0);
   vcos_assert(st == VCOS_SUCCESS);

   st = vcos_named_semaphore_create(&sem1, "foo", 0);
   vcos_assert(st == VCOS_SUCCESS);

   /* sem0 and sem1 should refer to the same semaphore now - get a helper thread
    * to post one, and we will wait on the other.
    */

   st = vcos_thread_create(&th, "test", NULL, existing_test_worker, &sem0);
   vcos_assert(st == VCOS_SUCCESS);

   ticks = vcos_get_ms();
   vcos_named_semaphore_wait(&sem1);
   ticks = vcos_get_ms() - ticks;

   if (ticks < 900 || ticks > 1100) {
      vcos_log("named semaphore: expected to wait for 1000 ticks, but actually waited for %d", ticks);
      vcos_assert(0);
   }

   vcos_thread_join(&th,NULL);

   vcos_named_semaphore_delete(&sem0);
   vcos_named_semaphore_delete(&sem1);

   return 1;
}

void run_named_semaphore_tests(int *run, int *passed)
{
   RUN_TEST(run,passed,create_test);
   RUN_TEST(run,passed,open_existing);
}



