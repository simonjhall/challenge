/*=========================================================
 Copyright  2008 Broadcom Europe Limited. All rights reserved.

 Project  :  VMCS Host Apps
 Module   :  Platform framework test application
 File     :  $Id: $

 FILE DESCRIPTION:
 Main entrypoint to platform framework test application
 =========================================================*/

#include <string.h>  /* for strcpy/cmp, etc. */
#include <stdio.h>
#include <stdlib.h>
#include "vcos_test.h"
#include "interface/vcos/vcos.h"

extern void run_msgqueue_tests(int *run, int *passed);
extern void run_thread_tests(int *run, int *passed);
extern void run_semaphore_tests(int *run, int *passed);
extern void run_named_semaphore_tests(int *run, int *passed);
extern void run_eventgroup_tests(int *run, int *passed);
extern void run_timer_tests(int *run, int *passed);
extern void run_library_tests(int *run, int *passed);
extern void run_mutex_tests(int *run, int *passed);
extern void run_event_tests(int *run, int *passed);
extern void run_abi_tests(int *run, int *passed);
extern void run_atomic_flags_tests(int *run, int *passed);
extern void run_queue_tests(int *run, int *passed);
extern void run_logging_tests(int *run, int *passed);

/***********************************************************
 * Name: host_app_name
 *
 * Arguments:
 *       void
 *
 * Description: Returns the host app name
 *
 * Returns: char * - host app name
 *
 ***********************************************************/
char * host_app_name( void )
{
   return "Platform Test";
}

/***********************************************************
 * Name: host_app_message_handler
 *
 * Arguments:
 *       const uint16_t msg      - the msg ID
 *       const uint32_t param1   - the msg param 1
 *       const uint32_t param2   - the msg param 2
 *
 * Description: The entry point for the host app
 *
 * Returns: void
 *
 ***********************************************************/

#ifdef __VIDEOCORE__
#include "vcfw/rtos/rtos.h"
#define CHECK_HEAP_CORRUPTION() vc_assert(rtos_find_heap_corruption(0) == NULL)
#else
#define CHECK_HEAP_CORRUPTION() (void)0
#endif

int verbose = 1;
//void a_cpp_function(void);

static void *app(void* cxt)
{
   int run = 0, passed = 0;

//   a_cpp_function();

   VCOS_LOG("starting logging tests");
   run_logging_tests(&run, &passed);

#if VCOS_HAVE_RTOS
   VCOS_LOG("starting msgq tests");
   run_msgqueue_tests(&run, &passed);
   CHECK_HEAP_CORRUPTION();
#endif

   VCOS_LOG("starting library tests");
   run_library_tests(&run, &passed);
   CHECK_HEAP_CORRUPTION();

   VCOS_LOG("starting timer tests");
   run_timer_tests(&run, &passed);
   CHECK_HEAP_CORRUPTION();

#if VCOS_HAVE_QUEUE
   VCOS_LOG("starting queue tests");
   run_queue_tests(&run, &passed);
   CHECK_HEAP_CORRUPTION();
#endif

#if VCOS_HAVE_SEMAPHORE
   VCOS_LOG("starting semaphore tests");
   run_semaphore_tests(&run, &passed);
   CHECK_HEAP_CORRUPTION();
#endif

   VCOS_LOG("starting mutex tests");
   run_mutex_tests(&run, &passed);
   CHECK_HEAP_CORRUPTION();

   VCOS_LOG("starting event tests");
   run_event_tests(&run, &passed);
   CHECK_HEAP_CORRUPTION();

   VCOS_LOG("starting eventgroup tests");
   run_eventgroup_tests(&run, &passed);
   CHECK_HEAP_CORRUPTION();

   VCOS_LOG("starting named semaphore tests");
   run_named_semaphore_tests(&run, &passed);
   CHECK_HEAP_CORRUPTION();

#if VCOS_HAVE_RTOS
   VCOS_LOG("starting thread tests");
   run_thread_tests(&run, &passed);
   CHECK_HEAP_CORRUPTION();
#endif

#ifdef __VIDEOCORE__ // why not #if VCOS_HAVE_ABI?
   run_abi_tests(&run, &passed);
#endif

#if VCOS_HAVE_ATOMIC_FLAGS
   VCOS_LOG("starting atomic flags tests");
   run_atomic_flags_tests(&run, &passed);
   CHECK_HEAP_CORRUPTION();
#endif

#ifndef ARTS
#ifdef __VIDEOCORE__
   _bkpt();
#endif
#endif

   VCOS_LOG("completed tests: %d run, %d passed", run, passed);

#ifdef ARTS
   if ( run != passed )
   {
      _ASM("mov %r29, 0xaffedead");
      _bkpt();
   }
   else
   {
      _ASM("mov %r29, 0xaffe0000");
      _bkpt();
   }
#endif
   return 0;
}

/* Start up the test.
 *
 * Because this code needs to run on both a regular host and on
 * VideoCore, it's more involved than would normally be
 * necessary.
 */

#if defined(VCOS_APPLICATION_INITIALIZE)

#include "vcfw/logging/logging.h"

/* On Nucleus and ThreadX, we can't 'join' any threads
 * from AppInit() because this is not a real task. So we
 * just fire off the first thread and then forget about it.
 */
void VCOS_APPLICATION_INITIALIZE(void *mem)
{
   static VCOS_THREAD_T thread;
   /* Setup logging */
   logging_init();
   logging_level( LOGGING_USER );
   vcos_init();
   vcos_thread_create(&thread, "testapp", NULL, app, NULL);
}

#elif VCOS_HAVE_RTOS

/* On a regular host, all the threads will be terminated
 * as soon as 'main' exits, so wait for the first thread
 * to finish.
 */
int main(int argc, const char **argv)
{
   VCOS_THREAD_T thread;
   vcos_init();
   vcos_timer_init();
   vcos_thread_create(&thread, "testapp", NULL, app, NULL);
   vcos_thread_join(&thread,NULL);
   vcos_deinit();
   return 0;
}

#else

/* On bare metal the closest we can get to launching a thread is starting
 * it on of the other cores.
 */
int main(int argc, const char **argv)
{
   vcos_init();
   vcos_timer_init();
   app(0);
   vcos_deinit();
   return 0;
}

#endif

#if !defined(vcos_demand)
#error vcos_demand not defined
#endif

#if !defined(vcos_assert)
#error vcos_assert not defined
#endif

#if !defined(vcos_verify)
#error vcos_verify not defined
#endif


