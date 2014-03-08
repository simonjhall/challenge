/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :  
Module   :  
File     :  $RCSfile: powerman.c,v $
Revision :  $Revision: #4 $

FILE DESCRIPTION: OS interface test harness for multiple platforms
=============================================================================*/
#include <stdio.h>

// generic header for os abstration layer
#include "interface/vchi/os/os.h"


#define TEST_OK(a, b) {if ((b) != 0) failed(a); else succeeded(a);}
   
#define TERM_EVENT 5

/******************************************************************************
Private typedefs
******************************************************************************/

/******************************************************************************
Static data
******************************************************************************/

static OS_SEMAPHORE_T sem1;
static int flag;

static OS_EVENTGROUP_T eventgroup;
static int current_event;

static OS_EVENTGROUP_T eventgroup2;
static int current_event2;

/******************************************************************************
Static func forwards
******************************************************************************/

/******************************************************************************
Global functions
******************************************************************************/

void failed(char *mess)
{
   os_logging_message("%s : FAILED\n", mess);
}

void succeeded(char *mess)
{
   os_logging_message("%s : ok\n", mess);
}


void AllocatorCheck()
{
   unsigned char *pdata = (unsigned char *)os_malloc(1024, 0, "alloc");
   
   if (!pdata)
      failed("os_malloc");
   else
   {
      succeeded("os_malloc");
      
      int i;
      char fail = 0;
      
      for (i=0;i<1024;i++)
      {
         pdata[i] = i % 255;
         
         if (pdata[i] != i % 255)
         {
            fail = 1; 
         }
      }

      TEST_OK("os_malloc : mem read/write check", fail);
      
      TEST_OK("os_free check", os_free(pdata));
   }
}



void TimerCheck()
{
   int32_t stime, etime, diff;
   
   TEST_OK("Timer Init", os_time_init());
   
   stime = os_get_time_in_usecs();
   
   // One second delay
   os_delay(1000);

   etime = os_get_time_in_usecs();
   
   // Should be at least 1s greater than stime;
   
   diff = etime - stime;
   
   printf("Start %ld, end %ld, diff %d\n", stime, etime, diff);
   
   TEST_OK("Delay approx correct", (diff < 1000000l || diff > 1010000l));

   TEST_OK("Timer term", os_time_term());
}


void ThreadOne(unsigned int a, void *b)
{
   TEST_OK("os_semaphore_obtain", os_semaphore_obtain(&sem1));

   flag = 1;
      
   TEST_OK("os_semaphore_release", os_semaphore_release(&sem1));
}


void ThreadCheck()
{
   VCOS_THREAD_T thread;

   // First create a sync semaphore   
   TEST_OK("os_semaphore_create", os_semaphore_create(&sem1, OS_SEMAPHORE_TYPE_SUSPEND));

   // and grab it   
   TEST_OK("os_semaphore_obtain", os_semaphore_obtain(&sem1));
   
   // Start the thread which shuold stall on the semaphore
   TEST_OK("os_thread_start", os_thread_start( &thread, &ThreadOne, NULL, 1000, "ThreadOne"));

   // Wait for thread to start for 1000 - should stall on semaphore and not change flag
   TEST_OK("os_delay", os_delay(1000));

   // Check global data is unchanged
   TEST_OK("Flag = 0", flag);

   // release semaphore
   TEST_OK("os_semaphore_release", os_semaphore_release(&sem1));
   
   // Wait for thread to continue for 100 - should be enough for it to set the flag

   TEST_OK("os_delay", os_delay(100));

   // Check global data is changed to non-zero
   TEST_OK("Flag = 1", (flag == 0));
   
   // Semaphore is currently released, so check the obtained function is correct.
   TEST_OK("os_semaphore_obtained - 0", os_semaphore_obtained(&sem1))
   
   // now get the sema and test again
   TEST_OK("os_semaphore_obtain", os_semaphore_obtain(&sem1));
   
   TEST_OK("os_semaphore_obtained - 1", (os_semaphore_obtained(&sem1) == 0))
   
   TEST_OK("os_semaphore_release", os_semaphore_release(&sem1));
}


void ThreadTwo(unsigned int a, void *b)
{
   int32_t event_map = 0;
   int32_t event = 0;
   
   // We wait on an event in the event group, then cross reference agains th eone we were expect. Do multiple times.
   
   while (event!=TERM_EVENT)
   {
      //TEST_OK("os_eventgroup_retrieve", os_eventgroup_retrieve ( &eventgroup, &event_map ) );
      
      os_eventgroup_retrieve ( &eventgroup, &event_map );
      
      // convert event to an index rather than a bit pattern

      // Cheap and cheerful 'lookup'...fast though. 
      switch (event_map)
      {
         case 0 :
         case 1 :
         case 2 : event = event_map -1; break;
         case 4 : event = 2 ; break;
         case 8 : event = 3 ; break;
         case 16 : event = 4; break;
         case 32 : event = 5; break;
         default : event = 0; break;             
      };
      
//      printf("\n  Got pattern %d, changed to %d, expected %d\n", event_map, event, current_event);
         
      TEST_OK("event index correct", !(event == current_event));
   }
   
}

void EventCheck()
{
   VCOS_THREAD_T thread;

   // Create one event group.
   
   TEST_OK("os_eventgroup_create", os_eventgroup_create ( &eventgroup, "group") );

   TEST_OK("os_thread_start", os_thread_start( &thread, &ThreadTwo, NULL, 1000, "ThreadOne"));

   // Wait for thread to start. Should really check a semaphore!
   TEST_OK("os_delay", os_delay(1000));


   for (current_event=1;current_event<=TERM_EVENT;current_event++)
   {
      TEST_OK("os_eventgroup_signal", os_eventgroup_signal ( &eventgroup, current_event) );
      
      TEST_OK("os_delay", os_delay(1000));
   }

   TEST_OK("os_eventgroup_destroy", os_eventgroup_destroy ( &eventgroup ) );
}



void ThreadThree(unsigned int a, void *b)
{
   int32_t event_map = 0;
   int32_t event = 0;
 
   while (event!=TERM_EVENT)
   {
      os_eventgroup_retrieve ( &eventgroup2, &event_map );
      // Cheap and cheerful 'lookup'...fast though. 
      switch (event_map)
      {
         case 0 :
         case 1 :
         case 2 : event = event_map -1; break;
         case 4 : event = 2 ; break;
         case 8 : event = 3 ; break;
         case 16 : event = 4; break;
         case 32 : event = 5; break;
         default : event = 0; break;             
      };
      
//      printf("\n  Got pattern %d, changed to %d, expected %d\n", event_map, event, current_event);
         
      TEST_OK("event index correct", !(event == current_event2));
   }
}

void EventCheck2()
{
   VCOS_THREAD_T thread1, thread2;

   // Create two event group.
   
   TEST_OK("os_eventgroup_create", os_eventgroup_create ( &eventgroup, "group") );
   TEST_OK("os_eventgroup_create", os_eventgroup_create ( &eventgroup2, "group2") );

   TEST_OK("os_thread_start", os_thread_start( &thread1, &ThreadTwo, NULL, 1000, "ThreadOne"));
   TEST_OK("os_thread_start", os_thread_start( &thread2, &ThreadThree, NULL, 1000, "ThreadTwo"));

   // Wait for threads to start. Should really check a semaphore!
   TEST_OK("os_delay", os_delay(1000));


   for (current_event=1;current_event<=TERM_EVENT;current_event++)
   {
      TEST_OK("os_eventgroup_signal", os_eventgroup_signal ( &eventgroup, current_event) );
      
      // for moment same on other thread
      current_event2 = current_event;
      
      TEST_OK("os_eventgroup_signal", os_eventgroup_signal ( &eventgroup2, current_event2) );
      
      TEST_OK("os_delay", os_delay(1000));
   }

   TEST_OK("os_eventgroup_destroy", os_eventgroup_destroy ( &eventgroup ) );
}






main()
{
   printf("Testing OS abstraction layer\n");
   printf("============================\n");
   
   // System init
   TEST_OK("os_init", os_init());
   
   AllocatorCheck();
   
   TimerCheck();
   
   ThreadCheck();       
   
   EventCheck();

   EventCheck2();
}


