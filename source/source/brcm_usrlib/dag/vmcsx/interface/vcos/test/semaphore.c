/*=========================================================
 Copyright © 2008 Broadcom Europe Limited. All rights reserved.

 Project  :  VMCS Host Apps
 Module   :  Platform framework test application
 File     :  $Id: $

 FILE DESCRIPTION:
 Semaphore tests for vcos test application
 =========================================================*/

#include <string.h>  /* for strcpy/cmp, etc. */
#include <stdio.h>
#include <stdlib.h>

#include "interface/vcos/vcos.h"
#include "vcos_test.h"

typedef struct {
   VCOS_SEMAPHORE_T *guard;

   VCOS_THREAD_T thread;
   VCOS_EVENT_FLAGS_T event;
   VCOS_SEMAPHORE_T go;
   VCOS_SEMAPHORE_T done;
   int quit;
   uint32_t wait[2];
   uint32_t post[2];
   int timeout[2];
} GUARD_T;

static void *guard_func(void *param)
{
   GUARD_T *st = (GUARD_T *) param;
   uint32_t set;

   while(1)
   {
      int i;
      vcos_event_flags_get(&st->event, -1, VCOS_OR_CONSUME, 0, &set);

      vcos_semaphore_wait(&st->go);

      if(st->quit)
         break;
      
      for(i=0; i<2; i++)
      {
         if(vcos_event_flags_get(&st->event, 1, VCOS_OR, st->wait[i], &set) != VCOS_SUCCESS)
         {
            int j;
            st->timeout[i] = 1;
            for(j=0; j<st->post[i]; j++)
               vcos_semaphore_post(st->guard);
         }
         else
            st->timeout[i] = 0;
      }

      vcos_semaphore_post(&st->done);
   }

   vcos_thread_exit(NULL);
   return NULL;
}

static int32_t guard_init(GUARD_T *guard)
{
   memset(guard, 0, sizeof(GUARD_T));

   if(vcos_semaphore_create(&guard->go, "go", 0) != VCOS_SUCCESS)
      return -1;

   if(vcos_semaphore_create(&guard->done, "done", 0) != VCOS_SUCCESS)
   {
      vcos_semaphore_delete(&guard->go);
      return -1;
   }

   if(vcos_event_flags_create(&guard->event, "event") != VCOS_SUCCESS)
   {
      vcos_semaphore_delete(&guard->go);
      vcos_semaphore_delete(&guard->done);
      return -1;
   }
   
   if (vcos_thread_create(&guard->thread, "guard", NULL, guard_func, guard) != VCOS_SUCCESS)
   {
      vcos_event_flags_delete(&guard->event);
      vcos_semaphore_delete(&guard->go);
      vcos_semaphore_delete(&guard->done);
      return -1;
   }

   return 0;
}

static void guard_deinit(GUARD_T *guard, int *passed)
{
   guard->quit = 1;
   vcos_semaphore_post(&guard->go);

   vcos_thread_join(&guard->thread,0);

   vcos_semaphore_delete(&guard->go);
   vcos_semaphore_delete(&guard->done);
   vcos_event_flags_delete(&guard->event);
}


#define MAX_TESTED 32
static int create_test()
{
   int passed = 1;
   VCOS_SEMAPHORE_T semas[MAX_TESTED];
   GUARD_T guard;
   int i;

   for(i=0; i<MAX_TESTED; i++)
   {
      if(vcos_semaphore_create(semas+i, "sem", i) != VCOS_SUCCESS)
      {
         vcos_assert(0);
         passed = 0;
      }
   }

   for(i=0; i<MAX_TESTED; i+=2)
      vcos_semaphore_delete(semas+i);

   for(i=0; i<MAX_TESTED; i+=2)
      if(vcos_semaphore_create(semas+i, "sem2", i) != VCOS_SUCCESS)
         passed = 0;

   if(passed)
   {
      if(guard_init(&guard) < 0)
         passed = 0;
      else
      {
         /* see if the values appear to be independent */
         for(i=0; i<MAX_TESTED && passed; i++)
         {
            int j;

            guard.wait[0] = 10;
            guard.post[0] = 1;
            guard.wait[1] = 10;
            guard.post[1] = 100;
            guard.guard = semas+i;

            vcos_semaphore_post(&guard.go);

            /* wait for one more than we set, the first wait by the guard
             * should timeout, and it will post one more time.  We should
             * then wake up here, and set the event, so the second wait
             * shouldn't timeout.
             */

            for(j=0; j<i+1; j++)
               vcos_semaphore_wait(semas+i);

            vcos_event_flags_set(&guard.event, 1, VCOS_OR);

            vcos_semaphore_wait(&guard.done);
            if(!guard.timeout[0] || guard.timeout[1])
               passed = 0;
         }

         guard_deinit(&guard, &passed);
      }

      for(i=0; i<MAX_TESTED; i++)
         vcos_semaphore_delete(semas+i);
   }
   return passed;
}

void run_semaphore_tests(int *run, int *passed)
{
   RUN_TEST(run,passed,create_test);
}


