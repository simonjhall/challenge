/*=========================================================
 Copyright © 2008 Broadcom Europe Limited. All rights reserved.

 Project  :  VMCS Host Apps
 Module   :  Platform framework test application
 File     :  $Id: $

 FILE DESCRIPTION:
 Eventgroup tests for vcos test application
 =========================================================*/

#include <string.h>  /* for strcpy/cmp, etc. */
#include <stdio.h>
#include <stdlib.h>

#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos_thread.h"
#include "vcos_test.h"

#define WAITER_PRI   VCOS_THREAD_PRI_LESS(VCOS_THREAD_PRI_NORMAL)
#define WAITER_STACK_SIZE 4096

#define MAX_TESTED 32
static int create_test()
{
   int passed = 1;
   VCOS_EVENT_FLAGS_T events[MAX_TESTED];
   int i;

   for(i=0; i<MAX_TESTED; i++)
      if(vcos_event_flags_create(events+i,"test") != VCOS_SUCCESS)
         passed = 0;

   for(i=0; i<MAX_TESTED; i+=2)
      vcos_event_flags_delete(events+i);

   for(i=0; i<MAX_TESTED; i+=2)
      if(vcos_event_flags_create(events+i,"test") != VCOS_SUCCESS)
         passed = 0;

   if(passed)
   {
      /* see if the bits appear to be independent */
      for(i=0; i<MAX_TESTED; i++)
         vcos_event_flags_set(events+i, i | (1<<i), VCOS_OR);

      for(i=0; i<MAX_TESTED; i++)
      {
         uint32_t set;
         if(vcos_event_flags_get(events+i, -1, VCOS_OR, VCOS_NO_SUSPEND, &set) != VCOS_SUCCESS)
            passed = 0;
         if(set != (i | (1<<i)))
            passed = 0;
      }

      for(i=0; i<MAX_TESTED; i++)
         vcos_event_flags_delete(events+i);
   }

   return passed;
}

static void check_value(VCOS_EVENT_FLAGS_T *event, int expected, int consume, int *passed)
{
   uint32_t set = 0;
   vcos_event_flags_get(event, -1, consume ? VCOS_OR_CONSUME : VCOS_OR, 0, &set);
   if(set != expected && passed)
      *passed = 0;
}

static int set_operations_test()
{
   VCOS_EVENT_FLAGS_T event;
   int passed = 1;

   if(vcos_event_flags_create(&event,"test") != VCOS_SUCCESS)
      passed = 0;
   else
   {
      check_value(&event, 0, 1, &passed);

      vcos_event_flags_set(&event, 1, VCOS_AND);
      check_value(&event, 0, 1, &passed);

      vcos_event_flags_set(&event, 1, VCOS_OR);
      check_value(&event, 1, 0, &passed);

      vcos_event_flags_set(&event, 8, VCOS_OR);
      check_value(&event, 9, 0, &passed);

      vcos_event_flags_set(&event, 96, VCOS_OR);
      check_value(&event, 9|96, 0, &passed);

      vcos_event_flags_set(&event, 65|1024, VCOS_AND);
      check_value(&event, 65, 0, &passed);

      vcos_event_flags_set(&event, -1, VCOS_OR);
      check_value(&event, -1, 1, &passed);

      vcos_event_flags_delete(&event);
   }

   return passed;
}

#ifndef NDEBUG

static const char *op2str(VCOS_OPTION op)
{
   switch (op)
   {
   case VCOS_OR: return "OR";
   case VCOS_AND: return "AND";
   case VCOS_OR_CONSUME: return "OR_CONSUME";
   case VCOS_AND_CONSUME: return "AND_CONSUME";
   default: return "?";
   }
}

#endif

static void try_get_value(int *num, VCOS_EVENT_FLAGS_T *event, uint32_t start, uint32_t bitmask,
                          VCOS_OPTION operation, VCOS_STATUS_T expected, int after, int *passed)
{
   VCOS_STATUS_T rc;
   uint32_t set = 0;
   vcos_event_flags_get(event, -1, VCOS_OR_CONSUME, 0, &set);
   vcos_event_flags_set(event, start, VCOS_OR);

   VCOS_TRACE("%s: mask %x op %s", __FUNCTION__, bitmask, op2str(operation));
   if((rc=vcos_event_flags_get(event, bitmask, operation, 0, &set)) != expected)
   {
      *passed = 0;
   }

   if(expected == VCOS_SUCCESS && set != start)
   {
      *passed = 0;
   }

   check_value(event, after, 1, passed);
}

static int get_operations_test()
{
   VCOS_EVENT_FLAGS_T event;
   int passed = 1;

   if(vcos_event_flags_create(&event,"test") != VCOS_SUCCESS)
      passed = 0;
   else
   {
      int testnum = 0;
      try_get_value(&testnum, &event, 0, 0, VCOS_OR,          VCOS_EAGAIN, 0, &passed);
      try_get_value(&testnum, &event, 0, 0, VCOS_AND,         VCOS_SUCCESS, 0, &passed);
      try_get_value(&testnum, &event, 0, 0, VCOS_OR_CONSUME,  VCOS_EAGAIN, 0, &passed);
      try_get_value(&testnum, &event, 0, 0, VCOS_AND_CONSUME, VCOS_SUCCESS, 0, &passed);

      try_get_value(&testnum, &event, 1, 0, VCOS_OR,          VCOS_EAGAIN, 1, &passed);
      try_get_value(&testnum, &event, 1, 0, VCOS_AND,         VCOS_SUCCESS, 1, &passed);
      try_get_value(&testnum, &event, 1, 0, VCOS_OR_CONSUME,  VCOS_EAGAIN, 1, &passed);
      try_get_value(&testnum, &event, 1, 0, VCOS_AND_CONSUME, VCOS_SUCCESS, 1, &passed);

      try_get_value(&testnum, &event, 7, 7, VCOS_OR,          VCOS_SUCCESS, 7, &passed);
      try_get_value(&testnum, &event, 7, 7, VCOS_OR_CONSUME,  VCOS_SUCCESS, 0, &passed);
      try_get_value(&testnum, &event, 7, 3, VCOS_OR,          VCOS_SUCCESS, 7, &passed);
      try_get_value(&testnum, &event, 7, 3, VCOS_OR_CONSUME,  VCOS_SUCCESS, 4, &passed);
      try_get_value(&testnum, &event, 7, 15, VCOS_OR,         VCOS_SUCCESS, 7, &passed);
      try_get_value(&testnum, &event, 7, 15, VCOS_OR_CONSUME, VCOS_SUCCESS, 0, &passed);
      try_get_value(&testnum, &event, 7, 8, VCOS_OR,          VCOS_EAGAIN, 7, &passed);
      try_get_value(&testnum, &event, 7, 8, VCOS_OR_CONSUME,  VCOS_EAGAIN, 7, &passed);

      try_get_value(&testnum, &event, 7, 7, VCOS_AND,         VCOS_SUCCESS, 7, &passed);
      try_get_value(&testnum, &event, 7, 7, VCOS_AND_CONSUME, VCOS_SUCCESS, 0, &passed);
      try_get_value(&testnum, &event, 7, 3, VCOS_AND,         VCOS_SUCCESS, 7, &passed);
      try_get_value(&testnum, &event, 7, 3, VCOS_AND_CONSUME, VCOS_SUCCESS, 4, &passed);
      try_get_value(&testnum, &event, 7, 15, VCOS_AND,        VCOS_EAGAIN, 7, &passed);
      try_get_value(&testnum, &event, 7, 15, VCOS_AND_CONSUME, VCOS_EAGAIN, 7, &passed);
      try_get_value(&testnum, &event, 7, 8, VCOS_AND,         VCOS_EAGAIN, 7, &passed);
      try_get_value(&testnum, &event, 7, 8, VCOS_AND_CONSUME, VCOS_EAGAIN, 7, &passed);

      vcos_event_flags_delete(&event);
   }

   return passed;
}

#ifdef WIN32
#define SLACK  10
#else
#define SLACK  2
#endif

static int basic_timeout(void)
{
   int delay = 10;
#ifdef VCOS_TIMER_MARGIN_EARLY
   int margin_early = VCOS_TIMER_MARGIN_EARLY;
   int margin_late = VCOS_TIMER_MARGIN_LATE;
#else
   int margin_early = SLACK;
   int margin_late = SLACK;
#endif

   VCOS_EVENT_FLAGS_T flags;
   VCOS_STATUS_T st = vcos_event_flags_create(&flags, "test");
   VCOS_UNSIGNED elapsed_time = vcos_get_ms(), actual;
   st = vcos_event_flags_get(&flags, 1, VCOS_OR, delay, &actual);
   if (st == VCOS_EAGAIN)
   {
      elapsed_time = vcos_get_ms()-elapsed_time;
      if (((int) elapsed_time >= (delay - margin_early)) && ((int) elapsed_time <= (delay + margin_late)))
      {
         vcos_log("Good delay: %d ms", elapsed_time);
         vcos_event_flags_delete(&flags);
         return 1;
      }
      vcos_log("Wrong delay: %d ms, expected %d ms", elapsed_time, delay);
   }

   vcos_event_flags_delete(&flags);

   return 0;
}

typedef struct {
   VCOS_THREAD_T thread;
   VCOS_EVENT_FLAGS_T *event;
   VCOS_SEMAPHORE_T go;
   VCOS_SEMAPHORE_T done;
   int id;
   int quit;
   int sleep_first;
   uint32_t bitmask;
   VCOS_EVENTGROUP_OPERATION_T operation;
   uint32_t suspend;
   VCOS_STATUS_T result;
   uint32_t retrieved_bits;
} WAITER_T;

static void *waiter_func(void *param)
{
   WAITER_T *st = (WAITER_T *) param;

   while(1)
   {
      vcos_semaphore_wait(&st->go);

      if(st->quit)
         break;

      if(st->sleep_first)
         vcos_sleep(st->sleep_first);

      st->result = vcos_event_flags_get(st->event, st->bitmask, st->operation, st->suspend, &st->retrieved_bits);

      vcos_semaphore_post(&st->done);
   }

   return 0;
}

static int32_t waiter_init(WAITER_T *waiter, VCOS_EVENT_FLAGS_T *event, int id)
{
   memset(waiter, 0, sizeof(WAITER_T));
   waiter->id = id;

   if(vcos_semaphore_create(&waiter->go, "go", 0) != VCOS_SUCCESS)
      return -1;

   if(vcos_semaphore_create(&waiter->done, "done", 0) != VCOS_SUCCESS)
   {
      vcos_semaphore_delete(&waiter->go);
      return -1;
   }

   waiter->event = event;

   if (vcos_thread_create(&waiter->thread, "waiter", NULL, waiter_func, waiter) != VCOS_SUCCESS)
   {
      vcos_semaphore_delete(&waiter->go);
      vcos_semaphore_delete(&waiter->done);
      return -1;
   }

   return 0;
}

static void waiter_deinit(WAITER_T *waiter, int *passed)
{
   waiter->quit = 1;
   vcos_semaphore_post(&waiter->go);

   vcos_thread_join(&waiter->thread,NULL);

   vcos_semaphore_delete(&waiter->go);
   vcos_semaphore_delete(&waiter->done);
}

static void run_waiters(WAITER_T *waiter, int num, int *sleep, uint32_t *bitmask, VCOS_EVENTGROUP_OPERATION_T *operation,
                        int32_t *suspend, int set_sleep, uint32_t set_bitmask, VCOS_STATUS_T *expected, uint32_t after, int *passed)
{
   int i;
   uint32_t set;
   VCOS_STATUS_T st;

   for(i=0; i<num; i++)
   {
      waiter[i].sleep_first = sleep ? sleep[i] : 0;
      waiter[i].bitmask = bitmask[i];
      waiter[i].operation = operation[i];
      waiter[i].suspend = suspend[i];
      vcos_semaphore_post(&waiter[i].go);
   }

   if(set_sleep)
      vcos_sleep(set_sleep);
   vcos_log("ORring in 0x%x", set_bitmask);
   vcos_event_flags_set(waiter[0].event, set_bitmask, VCOS_OR);

   for(i=0; i<num; i++)
   {
      vcos_semaphore_wait(&waiter[i].done);
      if(waiter[i].result != expected[i] || (waiter[i].result == VCOS_SUCCESS && waiter[i].retrieved_bits != set_bitmask))
         *passed = 0;
   }

   st = vcos_event_flags_get(waiter[0].event, -1, VCOS_OR_CONSUME, 0, &set);
   if (after == 0 && st != VCOS_EAGAIN)
   {
      // we were expecting to retrieve nothing this time, but instead we got something!
      *passed = 0;
       vcos_log("run_waiters: got unexpected events back afterwards");
   }
   else if (after && (st != VCOS_SUCCESS) && (set != after))
   {
       // we were expecting to retrieve something, but instead got nothing, or not the right thing!
       vcos_log("run_waiters: got wrong events back afterwards");
      *passed = 0;
   }
}

static void run_waiter(WAITER_T *waiter, int sleep, uint32_t bitmask, VCOS_EVENTGROUP_OPERATION_T operation,
                       int32_t suspend, int set_sleep, uint32_t set_bitmask, VCOS_STATUS_T expected, uint32_t after, int *passed)
{
   run_waiters(waiter, 1, &sleep, &bitmask, &operation, &suspend, set_sleep, set_bitmask, &expected, after, passed);
}

static int single_waiter_test()
{
   VCOS_EVENT_FLAGS_T event;
   WAITER_T waiter;
   int passed = 1;

   vcos_log("%s", __FUNCTION__);

   if(vcos_event_flags_create(&event,"test") != VCOS_SUCCESS)
      passed = 0;

   if(passed && waiter_init(&waiter, &event, 1) < 0)
   {
      vcos_event_flags_delete(&event);
      passed = 0;
   }

   if(passed)
   {
      run_waiter(&waiter, 0, 1, VCOS_OR_CONSUME, VCOS_EVENT_FLAGS_SUSPEND, 10, 1, VCOS_SUCCESS, 0, &passed);
      run_waiter(&waiter, 10, 1, VCOS_OR_CONSUME, VCOS_EVENT_FLAGS_SUSPEND, 0, 1, VCOS_SUCCESS, 0, &passed);
      run_waiter(&waiter, 0, 1, VCOS_OR_CONSUME, VCOS_EVENT_FLAGS_SUSPEND, 10, 3, VCOS_SUCCESS, 2, &passed);
      run_waiter(&waiter, 0, 1, VCOS_OR_CONSUME, 20, 10, 2, VCOS_EAGAIN, 2, &passed);
      run_waiter(&waiter, 10, 1, VCOS_OR_CONSUME, 10, 0, 2, VCOS_EAGAIN, 2, &passed);

      waiter_deinit(&waiter, &passed);
      vcos_event_flags_delete(&event);
   }

   return passed;
}

typedef struct {
   uint32_t bitmask[3];
   VCOS_EVENTGROUP_OPERATION_T operation[3];
   int32_t suspend[3];
   int set_sleep;
   uint32_t set_bitmask;
   VCOS_STATUS_T expected[3];
   uint32_t after;
} MULTIPLE_WAITER_SPEC_T;

#define EG_OR VCOS_OR
#define EG_ORC VCOS_OR_CONSUME
#define EG_AND VCOS_AND
#define EG_ANDC VCOS_AND_CONSUME

#define EG_S VCOS_EVENT_FLAGS_SUSPEND
#define EG_NS VCOS_EVENT_FLAGS_NO_SUSPEND

static MULTIPLE_WAITER_SPEC_T specs[] = {
   {{1, 1, 1},     {EG_OR, EG_OR, EG_OR},       {EG_S, EG_S, EG_S},    10, 1,  {0, 0, VCOS_SUCCESS},    1},
   {{1, 1, 1},     {EG_ORC, EG_OR, EG_ORC},     {EG_S, EG_S, EG_S},    10, 1,  {0, 0, VCOS_SUCCESS},    0},
   {{1, 3, 4},     {EG_OR, EG_ANDC, EG_OR},     {20,   20,   20},      10, 7,  {0, 0, VCOS_SUCCESS},    4},
   {{1, 3, 4},     {EG_OR, EG_ANDC, EG_ORC},    {20,   20,   20},      10, 7,  {0, 0, VCOS_SUCCESS},    0},
   {{1, 3, 4},     {EG_ORC, EG_AND, EG_OR},     {20,   20,   20},      10, 3,  {0, 0, VCOS_EAGAIN},   2},
   {{1, 3, 4},     {EG_ORC, EG_ANDC, EG_OR},    {20,   20,   20},      10, 3,  {0, 0, VCOS_EAGAIN},   0}
};

static int multiple_waiters_test()
{
   WAITER_T waiter[3];
   VCOS_EVENT_FLAGS_T event;
   int i;
   int passed = 1;

   vcos_log("%s", __FUNCTION__);

   if(vcos_event_flags_create(&event,"test") != VCOS_SUCCESS)
      passed = 0;

   i=0;
   while(passed && (i < 3) && waiter_init(waiter+(i), &event, i) == 0)
   {
      i++;
   }

   if(i < 3) // tidy up if we failed to create all the waiter threads
   {
      while(--i >= 0)
      {
         waiter_deinit(waiter+i, &passed);
      }
      passed = 0;
   }

   if(passed)
   {
      for(i=0; i<sizeof(specs) / sizeof(specs[0]); i++)
      {
         vcos_log("running multiple test spec %d", i);
         run_waiters(waiter, 3, NULL, specs[i].bitmask, specs[i].operation, specs[i].suspend,
                     specs[i].set_sleep, specs[i].set_bitmask, specs[i].expected, specs[i].after, &passed);
      }

      for(i=0; i<3; i++)
      {
         waiter_deinit(waiter+i, &passed);
      }

      vcos_event_flags_delete(&event);
   }

   return passed;
}

void run_eventgroup_tests(int *run, int *passed)
{
   RUN_TEST(run,passed,create_test);
   RUN_TEST(run,passed,basic_timeout);
   RUN_TEST(run,passed,set_operations_test);
   RUN_TEST(run,passed,get_operations_test);
   RUN_TEST(run,passed,single_waiter_test);
   if (VCOS_HAVE_RTOS)
   {
      RUN_TEST(run,passed,multiple_waiters_test);
   }
}

