/*=========================================================
 Copyright © 2008 Broadcom Europe Limited. All rights reserved.

 Project  :  VMCS Host Apps
 Module   :  Platform framework test application
 File     :  $Id: $

 FILE DESCRIPTION:
 Atomic flags tests for vcos test application
 =========================================================*/

#include "interface/vcos/vcos.h"
#include "vcos_test.h"

#define MAX_TESTED 32
static int test()
{
   int passed = 1;
   VCOS_ATOMIC_FLAGS_T f[MAX_TESTED];
   int i;

   for(i=0; i<MAX_TESTED; i++)
      if(vcos_atomic_flags_create(f+i) != VCOS_SUCCESS)
         passed = 0;

   for(i=0; i<MAX_TESTED; i+=2)
      vcos_atomic_flags_delete(f+i);

   for(i=0; i<MAX_TESTED; i+=2)
      if(vcos_atomic_flags_create(f+i) != VCOS_SUCCESS)
         passed = 0;

   if(passed)
   {
      for(i=0; i<MAX_TESTED; i++)
         vcos_atomic_flags_or(f+i, i | (1<<i));

      for(i=0; i<MAX_TESTED; i++)
      {
         if(vcos_atomic_flags_get_and_clear(f+i) != (i | (1<<i)))
            passed = 0;
         if(vcos_atomic_flags_get_and_clear(f+i) != 0)
            passed = 0;
      }

      for(i=0; i<MAX_TESTED; i++)
         vcos_atomic_flags_delete(f+i);
   }

   return passed;
}

void run_atomic_flags_tests(int *run, int *passed)
{
   RUN_TEST(run,passed,test);
}
