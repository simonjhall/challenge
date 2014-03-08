/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :  
Module   :  
File     :  $RCSfile: powerman.c,v $
Revision :  $Revision: #4 $

FILE DESCRIPTION: OS interface test harness for multiple platforms

This simply initialises the VCHI stack and makes a simple gencmd call.

=============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interface/vchi/vchi.h"

#include "interface/vmcs_host/vcgencmd.h"
#include "interface/vchi/os/os.h"

#include "vcfw/drivers/chip/mphi.h"

#ifdef __GNUC__
#define COMPILER "gcc"
#define COMPILER_VERSION_MAJ (__GNUC__)
#define COMPILER_VERSION_MIN (__GNUC_MINOR__)
#elif defined(_MSC_VER)
#define COMPILER "cl"
#define COMPILER_VERSION_MAJ (_MSC_VER/100)
#define COMPILER_VERSION_MIN (_MSC_VER%100)
#else
#error
#endif

#define VERSION "4"

typedef struct {
   fourcc_t fourcc;
   uint32_t messages;
   uint32_t last_messages, av_messages;
   uint32_t errors;
} INFO_FOURCC_T;
#define MAX_INFO 256
#define TEST_DURATION (30*1000000)
#define UPDATE_INFO_TIME 1000000
static INFO_FOURCC_T info[MAX_INFO];
static int test_failed = 0;

static OS_SEMAPHORE_T test_sem;
// need physical (i.e. local_mphi_get_func_table) implementation that supports multiple connections for this
#define num_connections 1

extern const MPHI_DRIVER_T *local_mphi_get_func_table(void);
void main_vc(unsigned a, void *b)
{
   int32_t success = -1; //fail by default
   VCHI_INSTANCE_T initialise_instance;
   VCHI_CONNECTION_T *connection[num_connections];
   int i;

   printf("main_vc\n");

   // initialise the vchi layer
   success = vchi_initialise( &initialise_instance );
   assert( success == 0 );

   for (i=0; i<num_connections; i++) {
      connection[i] = vchi_create_connection(single_get_func_table(),
                                             vchi_mphi_message_driver_func_table());
   }   
   // start up our test servers
#if 1
   vc_dc4_server_init (initialise_instance, &connection[0], num_connections);
#else
   vchi_long_server_init(initialise_instance, &connection[0], num_connections);
   vchi_short_server_init(initialise_instance, &connection[0], num_connections);
   // This is the test server initialisation
   vchi_cli_server_init(initialise_instance, &connection[0], num_connections);
   vchi_tstb_server_init(initialise_instance, &connection[0], num_connections);
   vchi_tstd_server_init(initialise_instance, &connection[0], num_connections);
#endif
   // now turn on the control service and initialise the connection
   success = vchi_connect(&connection[0], num_connections, initialise_instance);
   assert( success == 0 );
   vc_dc4_client_init(initialise_instance, &connection[0], num_connections);
}


void main_host(unsigned a, void *b)
{
   int success, iterations;
   VCHI_INSTANCE_T initialise_instance;
   VCHI_CONNECTION_T *connection[1];

   printf("main_host\n");
   
   success = vchi_initialise( &initialise_instance );
   os_assert( success == 0 );

   connection[0] = vchi_create_connection( single_get_func_table(),
                                           vchi_mphi_message_driver_func_table() );
   
#if 1
   host_dc4_server_init(initialise_instance, &connection[0], num_connections);
#else
   host_test_init_long_short   ( initialise_instance, &connection[0], num_connections );
   host_test_init_client_server( initialise_instance, &connection[0], num_connections );
   host_test_init_bulk         ( initialise_instance, &connection[0], num_connections );
   host_test_init_bulkdata     ( initialise_instance, &connection[0], num_connections );
#endif
   // now turn on the control service and initialise the connection
   success = vchi_connect( &connection[0], 1, initialise_instance);
   os_assert( success == 0 );
   host_dc4_client_init(initialise_instance, &connection[0], num_connections);
}

int main(void)
{
   int32_t success;
   VCOS_THREAD_T thread_vc, thread_host;
   const MPHI_DRIVER_T *mphi_driver;
   const int STACK=16*1024;
      
   //setvbuf(stdout, NULL, _IOLBF, 256);
   
   printf("Testing MPHI driver layer\n");
   printf("=========================\n"); 
   
   printf("Version: %s\n", VERSION);
   printf("Compiler %s (%d.%d)\n", COMPILER, COMPILER_VERSION_MAJ, COMPILER_VERSION_MIN);

   //printf("Press any key to start system\n");
   //getchar();
   
   // System init
   os_init();
   os_time_init();

   //mphi_driver = mphi_get_func_table();
   //mphi_driver->init();

   success = os_semaphore_create( &test_sem, OS_SEMAPHORE_TYPE_SUSPEND );
   assert(success==0);

   success = os_thread_start( &thread_host, main_host, NULL, STACK, "main_host" );
   assert(success==0);

   success = os_thread_start( &thread_vc, main_vc, NULL, STACK, "main_vc" );
   assert(success==0);
   
   while (1)
   {
      uint32_t t = os_get_time_in_usecs();
      static uint32_t last_t;
      if (last_t == 0) last_t = t;
      if (t - last_t > TEST_DURATION) {
         break;
         //os_print_stats();
         last_t = t;
      }
      os_sleep(500);
   }
   printf(test_failed ? "Failed\n":"Passed\n");
   return test_failed;
}

void vchi_test_display_init(void)
{
   printf("vchi_test_display_init\n");
}

static int find_fourcc(fourcc_t fourcc)
{
   int i;
   for (i=0; i<MAX_INFO; i++) {
      if (info[i].fourcc == fourcc)
         return i;
   }
   for (i=0; i<MAX_INFO; i++) {
      if (info[i].fourcc == 0) {
         info[i].fourcc = fourcc;
         return i;
      }
   }
   assert(0);
   return 0;
}

void update_display_info( fourcc_t service_id,int errors, int client_or_server )
{
   static uint32_t last_info_time;
   uint32_t now;
   int index;
   os_semaphore_obtain(&test_sem);
   now = os_get_time_in_usecs();
   if (!last_info_time) last_info_time = now;
   index = find_fourcc(service_id);
   info[index].errors = errors;
   info[index].messages++;

   if (now - last_info_time > UPDATE_INFO_TIME) {
      int i;
      last_info_time = now;
      for (i=0; i<MAX_INFO; i++) {
         if (info[i].fourcc == 0) continue;
         if (info[i].av_messages == 0) info[i].av_messages = info[i].messages;
         else info[i].av_messages = (15*info[i].av_messages + info[i].messages-info[i].last_messages)>>4;
         // failed if numbers stop increasing
         if (info[i].last_messages == info[i].messages)
            test_failed = 1;
         info[i].last_messages = info[i].messages;
         printf("%c%c%c%c:%d (%d) %d\n", (info[i].fourcc>>24)&0xff, (info[i].fourcc>>16)&0xff, (info[i].fourcc>>8)&0xff, (info[i].fourcc>>0)&0xff, info[i].messages, info[i].errors, info[i].av_messages);
      }
      if (i < 8)
         test_failed = 1;
   }
   os_semaphore_release(&test_sem);
}
void update_display_speed( int type, double speed )
{
   printf("update_display_speed(%d,%f)\n", type, speed);
}
void _dummy_function(void)
{
   assert(0);
}

uint32_t mem_get_size(VCHI_MEM_HANDLE_T handle)
{
   return 0;
}

void *mem_lock(VCHI_MEM_HANDLE_T handle)
{
   return NULL;
}

extern void mem_unlock(VCHI_MEM_HANDLE_T handle)
{
}


