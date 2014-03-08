/*=========================================================
 Copyright © 2008 Broadcom Europe Limited. All rights reserved.

 Project  :  VMCS Host Apps
 Module   :  Platform framework test application
 File     :  $Id: $

 FILE DESCRIPTION:
 Test VCOS function calling ABI
 =========================================================*/

/*
 * Imagine the situation. We have a codec that can only be
 * built from objects by most people. For whatever reason, we
 * choose to build a debug version. This will be filled with
 * calls to vcos_XXX().
 *
 * Now we try to use this in a *release* build - where there
 * aren't any of these functions, as they've all been inlined.
 *
 * This test ensures that it does still work.
 */

extern void vcos_sleep(unsigned int);

void run_abi_tests(int *run, int *passed)
{
   (*run)++;
   vcos_sleep(1); // if this linked in a RELEASE build, then the test passes!
   (*passed)++;
}


