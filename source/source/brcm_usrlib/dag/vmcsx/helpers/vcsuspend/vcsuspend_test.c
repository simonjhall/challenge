/* ============================================================================
Copyright (c) 2010-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

//#define VCSUSPEND_AUTO  // Automatically suspend periodically
//#define TEST_THREAD_USES_GENCMD

#include <string.h> // memset()

#include "interface/vcos/vcos.h"
#include "vcfw/rtos/rtos.h"
#include "vcfw/drivers/device/buttons.h"
#include "filesystem/filesys.h"
#include "helpers/vcsuspend/vcsuspend.h"

#ifdef VCSUSPEND_AUTO
   #include "vcfw/logging/logging.h"
#endif


#ifdef TEST_THREAD_USES_GENCMD
#include "interface/vmcs_host/vcgencmd.h"
#endif

//=============================================================================

#define VCSUSPEND_TEST_KEY (BUTTON_KEY_HASH | BUTTON_KEY_ENTER)

//-----------------------------------------------------------------------------

static struct
{
   VCOS_THREAD_T test_thread;
   uint32_t halt;
} vcsuspend_test_globals;

#ifdef TEST_THREAD_USES_GENCMD
static char status_result[160];
static char suspend_result[160];
#endif

//-----------------------------------------------------------------------------

static void *vcsuspend_test_thread_entry(void *arg);
static void vcsuspend_test_suspend(void);

//=============================================================================

int vcsuspend_test_init(void)
{
   int failure;

   failure = vcos_thread_create(
      &vcsuspend_test_globals.test_thread,
      "VC Suspend Test",
      NULL,
      vcsuspend_test_thread_entry,
      NULL);
   vcos_assert(!failure);

   return failure;
} // vcsuspend_test_init()

//-----------------------------------------------------------------------------

void vcsuspend_test_stop(void)
{
   vcsuspend_test_globals.halt = 1;
} // vcsuspend_test_stop

//-----------------------------------------------------------------------------

static void *vcsuspend_test_thread_entry(void *arg)
{
   static const BUTTONS_DRIVER_T *buttons_driver = NULL;
   static DRIVER_HANDLE_T buttons_handle;
   static uint32_t last_buttons = 0;
#ifdef VCSUSPEND_AUTO
   uint32_t suspend_counter = 0;
#endif

   volatile static int exit = 0;

   buttons_driver = BUTTONS_DRIVER_GET_FUNC_TABLE();
   vcos_assert( NULL != buttons_driver );
   buttons_driver->open( NULL, &buttons_handle );

   // Ignore button pressed held during startup
   buttons_driver->pressed(buttons_handle, -1, &last_buttons);

   for(;;)
   {
      // If app is handling its own suspend button, stop this thread
      if(vcsuspend_test_globals.halt)
      {
         vcos_thread_exit(NULL);
         return 0;
      }

#ifndef VCSUSPEND_AUTO
      uint32_t current_buttons;
      uint32_t new_buttons;

      buttons_driver->pressed(buttons_handle, -1, &current_buttons);
      new_buttons = current_buttons & ~last_buttons;
      last_buttons = current_buttons;

      if (new_buttons & VCSUSPEND_TEST_KEY)
      {
         /* MUST be registered with filesys before doing vcsuspend...  is that naughty? */
         {
            static int registered = 0;
            if(!registered)
            {
               /* D'oh! */
               filesys_register();
               registered = 1;
            } // if
         } // block

         vcsuspend_test_suspend();

      } // if
      rtos_delay(5000,1);
#else
      rtos_delay(4000000, 1);
      vcsuspend_test_suspend();
      suspend_counter++;
      logging_message(LOGGING_GENERAL, "suspend_counter = %d", suspend_counter);
#endif

      if(exit) break;
   } // for

   return NULL;
} // vcsuspend_test_thread_entry()

//-----------------------------------------------------------------------------

static void vcsuspend_test_suspend(void)
{
#ifdef TEST_THREAD_USES_GENCMD
         /* N.B.  This bit is pretending to be on the "host" side */
         /* None of the rest of this file is host side code, so please don't copy'n'paste
            this stuff blindly. */
         memset(suspend_result, 0, sizeof(suspend_result));
         memset(status_result, 0, sizeof(status_result));
         vc_gencmd(suspend_result, sizeof(suspend_result), "sus_suspend");
         vc_gencmd(status_result, sizeof(status_result), "sus_status");
#else
         vcsuspend_suspend();
#endif
} // vcsuspend_test_suspend()

//-----------------------------------------------------------------------------
