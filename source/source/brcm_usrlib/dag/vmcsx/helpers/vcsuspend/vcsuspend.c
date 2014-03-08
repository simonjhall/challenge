/* ============================================================================
Copyright (c) 2010-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "helpers/vcsuspend/vcsuspend.h"

#include "vcfw/drivers/chip/intctrl.h"
#include "vcfw/drivers/chip/otp.h"
#include "vcfw/drivers/chip/power.h"
#include "vcfw/drivers/chip/sdhost.h"
#include "vcfw/drivers/chip/sdram.h"

#ifdef VCLIB_SECURE_HASHES
#include "helpers/vclib/hmac.h"
#include "helpers/vclib/vclib.h"
#endif

#include "vcfw/logging/logging.h"
#include "spinlock.h"

#include "interface/vcos/vcos.h"

#include <string.h>
#include <stdlib.h>

//=============================================================================

#define MAX_CALLBACKS 40

#define VCSUSPEND_FLAGS_HALT            0x01
#define VCSUSPEND_FLAGS_VPU1_IS_AWAKE   0x02

#ifndef HASH_SIZE_IN_BYTES_SHA256
#define HASH_SIZE_IN_BYTES_SHA256   32
#endif

volatile int32_t vcsuspend_spin;

static struct
{
   spinlock_t spinlock;

   RTOS_SECURE_FUNC_HANDLE_T do_suspend_handle;
   RTOS_SECURE_FUNC_HANDLE_T write_cpg_int_vector;
   RTOS_SECURE_FUNC_HANDLE_T get_secure_ram_key;
   RTOS_SECURE_FUNC_HANDLE_T get_secure_ram_settings;
#ifdef __VIDEOCORE4__
   RTOS_SECURE_FUNC_HANDLE_T save_vpu1_state_handle;
#endif

   struct vcsuspend_callback
   {
      VCSUSPEND_RUNLEVEL_T run_level;
      VCSUSPEND_CALLBACK_FUNC_T do_suspend_resume;
      VCSUSPEND_CALLBACK_ARG_T private_data;
   } callbacks[MAX_CALLBACKS];

   int32_t ncallbacks;
   int32_t flags;
   VCSUSPEND_ERR_T last_errcode;
   int32_t last_level_reached_major;
   int32_t last_level_reached_minor;
   int32_t is_sorted;
} _vcsuspend_globals, *vcsuspend_globals=&_vcsuspend_globals;

// Special structure used in vcsuspend_asm_vc?.s and 2nd stage bootloader (main.c)
// Must be kept in sync with the MAGIC_* offsets specified in vcsuspend_asm_vc4.s
#define SCRATCHPAD_RESERVED_BYTES 0x8dc

static union
{
   uint8_t bytes[SCRATCHPAD_RESERVED_BYTES];
   struct
   {
      uint32_t magic;
      uint32_t sp;
      uint32_t pc;
      const uint8_t *secure_data;
      uint32_t secure_data_size;
      uint8_t  signature[HASH_SIZE_IN_BYTES_SHA256];
      uint32_t sp_vpu1;
   } fields;
} vcsuspend_scratchpad;

#pragma Align_to(32, vcsuspend_scratchpad)

//-----------------------------------------------------------------------------

#ifdef VCSUSPEND_WANT_TEST_THREAD
   // Create test thread, if required, from vcsuspend_test.c
   int32_t vcsuspend_test_init(void);
   void vcsuspend_test_stop(void);
#endif

static void vcsuspend_undo_boot_reg_writes(void);
static int32_t vcsuspend_get_reg_index
   (VCSUSPEND_RUNLEVEL_T run_level, VCSUSPEND_CALLBACK_FUNC_T func, VCSUSPEND_CALLBACK_ARG_T private_data);

#ifdef __VIDEOCORE4__
static void vcsuspend_save_vpu1_state(void);
static void vpu1_semaphore_lisr(uint32_t vector_num);
#endif

static void vcsuspend_write_cpg_int_vector
   (uint32_t new_int_vector, uint32_t *old_int_vector);

#ifdef VCLIB_SECURE_HASHES
static void vcsuspend_compute_secure_sig(uint8_t signature[]);
#endif

// Secure functions (in vcsuspend_asm_vc4.s)
void vcsuspend_do_suspend_s(void);
void vcsuspend_write_cpg_int_vector_asm(void);
void vcsuspend_get_secure_ram_key_asm(void);
void vcsuspend_get_secure_ram_settings_asm(void);

#ifdef __VIDEOCORE4__
   void vcsuspend_save_vpu1_state_asm(void);
#endif

//=============================================================================

static int vcsuspend_sort_suspend_order_comparator
   (struct vcsuspend_callback const *a, struct vcsuspend_callback const *b)
{
   VCSUSPEND_RUNLEVEL_T runlevel1, runlevel2;

   runlevel1 = a->run_level;
   runlevel2 = b->run_level;

   return (runlevel1 < runlevel2) ? -1 : (runlevel1 > runlevel2) ? 1 : 0;
}

//-----------------------------------------------------------------------------

// Function pointer type used by qsort
typedef int (*QSORT_T)(const void *elem1, const void *elem2);

static void vcsuspend_sort_suspend_order(void)
{
   qsort(vcsuspend_globals->callbacks,
         vcsuspend_globals->ncallbacks,
         sizeof(struct vcsuspend_callback),
         (QSORT_T) vcsuspend_sort_suspend_order_comparator);
}

//-----------------------------------------------------------------------------

int32_t vcsuspend_initialise(void)
{
   int32_t func_failure = 0, failure = 0;

   // Register secure fnctions
   failure = rtos_secure_function_register(
      vcsuspend_do_suspend_s,
      &vcsuspend_globals->do_suspend_handle);
   vcos_assert(!failure);
   func_failure |= failure;

   failure = rtos_secure_function_register(
      vcsuspend_write_cpg_int_vector_asm,
      &vcsuspend_globals->write_cpg_int_vector);
   vcos_assert(!failure);
   func_failure |= failure;

   failure = rtos_secure_function_register(
      vcsuspend_get_secure_ram_key_asm,
      &vcsuspend_globals->get_secure_ram_key);
   vcos_assert(!failure);
   func_failure |= failure;

   failure = rtos_secure_function_register(
      vcsuspend_get_secure_ram_settings_asm,
      &vcsuspend_globals->get_secure_ram_settings);
   vcos_assert(!failure);
   func_failure |= failure;

#ifdef __VIDEOCORE4__
   // Register VPU1 assembler save state function as secure
   failure = rtos_secure_function_register(
      vcsuspend_save_vpu1_state_asm,
      &vcsuspend_globals->save_vpu1_state_handle);
   vcos_assert(!failure);
   func_failure |= failure;
#endif

#ifdef VCSUSPEND_WANT_TEST_THREAD
   failure = vcsuspend_test_init();
   func_failure |= failure;
#endif

   return func_failure;
}

//-----------------------------------------------------------------------------

void vcsuspend_stop_test_thread(void)
// For apps which detect the suspend button themselves, disable the test thread
{

#ifdef VCSUSPEND_WANT_TEST_THREAD
   vcsuspend_test_stop();
#endif
} // vcsuspend_stop_test_thread()

//-----------------------------------------------------------------------------

uint32_t vcsuspend_is_enabled(void)
// Returns true if vcsuspend is enabled, and false otherwise
{
#ifdef VCSUSPEND_WANT_TEST_THREAD
   return 1;
#else
   return 0;
#endif
} // vcsuspend_is_enabled()

//-----------------------------------------------------------------------------

/* Register a "do suspend/resume" function.
 *
 * This function can be called before vcsuspend_initialise(), as happens for
 * the base drivers and some others, hence the use of spinlock, rather
 * than a regular mutex.
 *
 * If called multiple times with the same parameters, registration will only
 * occur the first time.
 */
int32_t vcsuspend_register_ext
   (VCSUSPEND_RUNLEVEL_T run_level, VCSUSPEND_CALLBACK_FUNC_T func,
      VCSUSPEND_CALLBACK_ARG_T private_data, char *log_info)
{
   int32_t failure = 0; // Succeed by default
   int32_t reg_index;

   spin_lock(&vcsuspend_globals->spinlock);

   if(vcsuspend_globals->ncallbacks >= MAX_CALLBACKS)
   {
      spin_unlock(&vcsuspend_globals->spinlock);
      vcos_assert(0);
      return -1;
   } // if

   // Only register callback if this combination of parameters has not already been stored
   reg_index = vcsuspend_get_reg_index(run_level, func, private_data);
   if(reg_index == -1)
   {
      if(func != NULL)
      {
         vcsuspend_globals->callbacks[vcsuspend_globals->ncallbacks].run_level = run_level;
         vcsuspend_globals->callbacks[vcsuspend_globals->ncallbacks].do_suspend_resume = func;
         vcsuspend_globals->callbacks[vcsuspend_globals->ncallbacks].private_data = private_data;
         vcsuspend_globals->ncallbacks++;
         vcsuspend_globals->is_sorted = 0;
      } // if
      else
      {
         failure = 1;
         vcos_assert(0);
      } // else

   } // if

   spin_unlock(&vcsuspend_globals->spinlock);

   return failure;
} // vcsuspend_register_ext()

//-----------------------------------------------------------------------------

int32_t vcsuspend_unregister
   (VCSUSPEND_RUNLEVEL_T run_level, VCSUSPEND_CALLBACK_FUNC_T func,
      VCSUSPEND_CALLBACK_ARG_T private_data)
{
   // Note: at this time (28-Apr-2010), this function is never used
   int32_t i;
   int32_t found;
   int32_t failure = 0; // Succeed by default

   vcos_assert(0); // TODO Remove assert once this has been tested

#if 0 // TODO Remove logging
   logging_message(LOGGING_GENERAL, "vcsuspend_unregister(%d)", run_level);
#endif
   spin_lock(&vcsuspend_globals->spinlock);

   found = 0;
   for(i = 0; i < vcsuspend_globals->ncallbacks; i++)
   {
      if(vcsuspend_globals->callbacks[i].run_level == run_level &&
         vcsuspend_globals->callbacks[i].do_suspend_resume == func &&
         vcsuspend_globals->callbacks[i].private_data == private_data)
      {
         vcos_assert(!found); /* should only be one! */
         found = 1;
         vcsuspend_globals->ncallbacks--;
         vcsuspend_globals->is_sorted = 0;
         // Copy callback from end of list to gap created by removing this one; will be sorted later
         vcsuspend_globals->callbacks[i].run_level = vcsuspend_globals->callbacks[vcsuspend_globals->ncallbacks].run_level;
         vcsuspend_globals->callbacks[i].do_suspend_resume = vcsuspend_globals->callbacks[vcsuspend_globals->ncallbacks].do_suspend_resume;
         vcsuspend_globals->callbacks[i].private_data = vcsuspend_globals->callbacks[vcsuspend_globals->ncallbacks].private_data;
         i--; /* so that we can carry on the search */
      } // if
   } // for

   if(!found)
   {
      vcos_assert(0);
      failure = -1;
   } // if

   spin_unlock(&vcsuspend_globals->spinlock);

   return failure;
}

//-----------------------------------------------------------------------------

#ifndef NDEBUG
/* DEBUG AID - vcsuspend_debug_depth
**
**                               0   1   2   3
**
**   middleware callbacks            Y   Y   Y
**   
**   run domain                      Y   Y   Y
**   
**   drivers                             Y   Y
**
**   run secure mode function                Y
**  
**   power down                              Y (unless HIBER_METHOD_DONT is defined)
**
*/
static volatile enum
{
   DEBUG_DEPTH_NONE,
   DEBUG_DEPTH_DO_CALLBACKS,
   DEBUG_DEPTH_RUN_SECURE_FUNC,
   DEBUG_DEPTH_POWER_DOWN
} vcsuspend_debug_depth = DEBUG_DEPTH_POWER_DOWN;
#endif // NDEBUG

//-----------------------------------------------------------------------------

int32_t vcsuspend_geterror(VCSUSPEND_ERR_T *errcode, int32_t *lastlev_major, int32_t *lastlev_minor)
{
   *errcode = vcsuspend_globals->last_errcode;
   *lastlev_minor = vcsuspend_globals->last_level_reached_minor;
   *lastlev_major = vcsuspend_globals->last_level_reached_major;
   return 0;
}

//-----------------------------------------------------------------------------

// Macros for calls to suspend/resume callbacks
#define SUSPEND   0
#define RESUME    1

int32_t vcsuspend_go(uint32_t do_halt)
// Carry out the suspend (or halt) operation
{
   int32_t failure = -1;
   int32_t i;
   uint32_t old_cpg_int_addr = 0;

   // Note: the vcsuspend mutex can't be locked here, as when suspending some
   // modules, others are opened and registered with vcsuspend.

   vcsuspend_globals->last_errcode = 0;
   vcsuspend_globals->flags = 0;

   if(do_halt)
      {vcsuspend_globals->flags |= VCSUSPEND_FLAGS_HALT;}

#ifndef NDEBUG
   if(vcsuspend_debug_depth == DEBUG_DEPTH_NONE)
   {
      /* most shallow debug mode... don't suspend at all */
      vcsuspend_globals->last_errcode = VCSUSPEND_ERR_DEBUGDEPTH_REACHED;
      vcsuspend_globals->last_level_reached_minor = 0;
      vcsuspend_globals->last_level_reached_major = 0;
      return -1;
   } // if
#endif

   // Sort callbacks by run level, if they're not already sorted
   if(!vcsuspend_globals->is_sorted)
   {
      vcsuspend_sort_suspend_order();
      vcsuspend_globals->is_sorted = 1;
   } // if

   /* Shut down middleware and drivers */
   for(i = vcsuspend_globals->ncallbacks - 1; i >= 0; i--)
   {
      failure = vcsuspend_globals->callbacks[i].do_suspend_resume
         (SUSPEND, vcsuspend_globals->callbacks[i].private_data);
      if(failure)
      {
         /* oops.  Vetoed. */
         vcsuspend_globals->last_errcode = VCSUSPEND_ERR_VETOED_BY_MIDDLEWARE;
         vcsuspend_globals->last_level_reached_minor = i;
         vcsuspend_globals->last_level_reached_major = 1;

         // Resume suspended modules
         for( ; i < vcsuspend_globals->ncallbacks ; i++)
         {
            int32_t resume_failure;
            resume_failure = vcsuspend_globals->callbacks[i].do_suspend_resume
               (RESUME, vcsuspend_globals->callbacks[i].private_data);
            vcos_assert(!resume_failure);
         } // for
         return failure;
      } // if
   } // for

#ifndef NDEBUG
   if(vcsuspend_debug_depth <= DEBUG_DEPTH_DO_CALLBACKS)
   {
      /* Debug depth means shutdown middleware and drivers only */
      vcsuspend_globals->last_errcode = VCSUSPEND_ERR_DEBUGDEPTH_REACHED;
      vcsuspend_globals->last_level_reached_minor = vcsuspend_globals->ncallbacks;
      vcsuspend_globals->last_level_reached_major = 1;

      // Resume all drivers, and return
      for(i = 0; i < vcsuspend_globals->ncallbacks; i++)
      {
         failure = vcsuspend_globals->callbacks[i].do_suspend_resume
            (RESUME, vcsuspend_globals->callbacks[i].private_data);
         vcos_assert(!failure);
      } // for
      return -1;
   } // if
#endif

   if(!do_halt)
   {
#ifdef __VIDEOCORE4__
      // If VPU1 is awake, then save its state, and put it to sleep
      vcsuspend_save_vpu1_state();
#endif
      // CPG interrupt vector is used to store address of scratchpad, for use by
      // 2nd stage bootloader. So, write this address, and store old one. This
      // must be done BEFORE computing the signature, as the vector is (usually)
      // in the secure area
      vcsuspend_write_cpg_int_vector
         ((uint32_t) vcsuspend_scratchpad.bytes, &old_cpg_int_addr);

#ifdef VCLIB_SECURE_HASHES
      // Compute signature on secure area, and store in scratchpad
      vcsuspend_compute_secure_sig(vcsuspend_scratchpad.fields.signature);
#endif
   } // if

#ifndef NDEBUG
   if(vcsuspend_debug_depth == DEBUG_DEPTH_POWER_DOWN)
#endif
   {
      // Go to sleep...
      rtos_secure_function_call
         (vcsuspend_globals->do_suspend_handle,
            (int32_t) vcsuspend_scratchpad.bytes,
            sizeof(vcsuspend_scratchpad),
            vcsuspend_globals->flags, 0, 0);
      // ...and now we've awoken
   } // if

   // Reset registers which may have been changed by the bootloader, and which
   // should be reset by the app, even if they're not otherwise used by it.
   vcsuspend_undo_boot_reg_writes();

   // Restore the CPG interrupt
   vcsuspend_write_cpg_int_vector(old_cpg_int_addr, NULL);

   /* Resume middleware and drivers */
   for(i = 0; i < vcsuspend_globals->ncallbacks; i++)
   {
      failure = vcsuspend_globals->callbacks[i].do_suspend_resume
         (RESUME, vcsuspend_globals->callbacks[i].private_data);
      vcos_assert(!failure);
   } // for

   return 0;

} // vcsuspend_go()

//-----------------------------------------------------------------------------

static void vcsuspend_undo_boot_reg_writes(void)
{
   /* Some registers will have been prodded by the boot sequence.  We want the driver startup to
      work from chip-reset state.  So, we reset anything that might have been touched during boot here. */

   /* This is important so that we get no surprises during debug.  It is essential that the register values
      during driver startup are as they are in after power on reset even if we didn't suspend for any reason. */

   // Ensure that SD card is reset and switched off, for the benefit of those apps
   // which don't use it.
   sdhost_get_func_table()->sdhost_reset_regs();

#ifdef __VIDEOCORE4__

   // TODO OTP clock?

   // TODO Watchdog?


#elif defined __VIDEOCORE3__
   /* OTP clock */
   CM_OTP = CM_OTP & ~(1<<CM_OTP_ENAB_LSB) | 0xa5a50000;
   while(CM_OTP & (1<<CM_OTP_RUNNING_LSB));
   CM_OTP = 0 | 0xa5a50000;

   /* And the watchdog was used too */
   RSTCS=0xa5a5ff00;
   RSTWD=0xa5a50000;
#endif
}

//-----------------------------------------------------------------------------

static int32_t vcsuspend_get_reg_index
   (VCSUSPEND_RUNLEVEL_T run_level, VCSUSPEND_CALLBACK_FUNC_T func,
      VCSUSPEND_CALLBACK_ARG_T private_data)
// Return the registration index of the specified callback function, or -1 if it's not registered
{
   int32_t index = -1;

   for(index = 0; index < vcsuspend_globals->ncallbacks; index++)
   {
      if(vcsuspend_globals->callbacks[index].run_level == run_level &&
         vcsuspend_globals->callbacks[index].do_suspend_resume == func &&
         vcsuspend_globals->callbacks[index].private_data == private_data)
      {
         return index;
      } // if
   } // for

   return -1;
} // vcsuspend_get_reg_index()

//-----------------------------------------------------------------------------

#ifdef __VIDEOCORE4__

#define MS_STATUS_SEMA_1_SET     (1 << 1)

static void vcsuspend_save_vpu1_state(void)
{
   uint32_t failure = 0;
   uint32_t dummy = 0;
   RTOS_LISR_T old_vpu1_semaphore_lisr;

   // If VPU1 is awake, set flag and continue; otherwise, exit
   if(power_get_func_table()->vpu1_is_awake != NULL)
   {
      if(power_get_func_table()->vpu1_is_awake(NULL))
      {
         // Save VPU1 state
         vcsuspend_globals->flags |= VCSUSPEND_FLAGS_VPU1_IS_AWAKE;
      } // if
      else {return;}
   } // if
   else {return;}

   // Register VPU1 semaphore interrupt, and enable
   failure = rtos_register_lisr(INTERRUPT_MULTICORESYNC1, vpu1_semaphore_lisr, &old_vpu1_semaphore_lisr);
   vcos_assert(!failure);
   intctrl_get_func_table()->set_imask_per_core(NULL, 1, INTERRUPT_MULTICORESYNC1, 1);

   // Clear semaphore 0 by writing to register (should already be clear at this point)
   MS_SEMA_0 = 0;
   // Read semaphore 1 to set it
   dummy = MS_SEMA_1;
   vcos_assert(dummy == 0);

   // Enable VPU1 semaphore interrupt 0; should trigger immediately
   MS_IREQ_1 |= 0x1;

   // Wait for semaphore 1 to be cleared by interrupt
   while(MS_STATUS & MS_STATUS_SEMA_1_SET) {}

   // Disable semaphore interrupt 0
   intctrl_get_func_table()->set_imask_per_core(NULL, 1, INTERRUPT_MULTICORESYNC1, 0);

   // Restore previous VPU1 semaphore interrupt
   failure = rtos_register_lisr(INTERRUPT_MULTICORESYNC1, old_vpu1_semaphore_lisr, NULL);

   vcos_assert(!failure);
} // vcsuspend_save_vpu1_state()

//-----------------------------------------------------------------------------

static void vpu1_semaphore_lisr(uint32_t vector_num)
// LISR for VPU1 semaphore interrupt (referred to in the code as INTERRUPT_MULTICORESYNC1)
{
   MS_IREQ_1 &= ~0x1;   // Disable VPU1 semaphore interrupt 0

   // Call assembler function to save VPU1 state
   rtos_secure_function_call
      (vcsuspend_globals->save_vpu1_state_handle,
         (uint32_t) vcsuspend_scratchpad.bytes, 0, 0, 0, 0);

} // vpu1_semaphore_lisr()

#endif // __VIDEOCORE4__
//-----------------------------------------------------------------------------

static void vcsuspend_write_cpg_int_vector
   (uint32_t new_int_vector, uint32_t *old_int_vector)
// Write a new address to the CPG interrupt vector; the old contents is
// (optionally) sent back.
// WARNING: interrupt vectors are not normally modified like this. Use with care.
{
   new_int_vector = (uint32_t) ALIAS_DIRECT(new_int_vector);

   rtos_secure_function_call
      (vcsuspend_globals->write_cpg_int_vector,
         new_int_vector, (uint32_t) old_int_vector, 0, 0, 0);
} // vcsuspend_write_cpg_int_vector()

//-----------------------------------------------------------------------------
#ifdef VCLIB_SECURE_HASHES

#define KEY_SIZE_IN_BYTES     (OTP_SUSPEND_SECURE_RAM_KEY_SIZE_IN_ROWS * 4)

static void vcsuspend_compute_secure_sig(uint8_t signature[])
// Compute signature (hash) on secure areas of RAM, and store in array. This
// has to be performed as late as possible, as some of the suspend functions
// modify the interrupt vector table, which is in secure RAM.
{
   // TODO Store key somewhere secure once we have a secure SHA256 implementation
   union
   {
      uint8_t u8[KEY_SIZE_IN_BYTES];
      uint32_t u32[KEY_SIZE_IN_BYTES / 4];
   } key;
   uint32_t secure_start_reg, secure_start_addr, secure_end;

   // Get key from OTP
   rtos_secure_function_call
      (vcsuspend_globals->get_secure_ram_key, (uint32_t) key.u32, 0, 0, 0, 0);

   // Get secure RAM settings
   rtos_secure_function_call
      (vcsuspend_globals->get_secure_ram_settings,
         (uint32_t) &secure_start_reg, (uint32_t) &secure_end, 0, 0, 0);
   secure_start_addr = secure_start_reg & SD_SECSRT0_EN_CLR;
   vcsuspend_scratchpad.fields.secure_data = ALIAS_DIRECT(secure_start_addr);
   vcsuspend_scratchpad.fields.secure_data_size = (secure_end - secure_start_addr) + 1;

   // HMAC uses the vector core, which is locked by its
   // suspend function, so we have to unlock it before proceeding
   vclib_release_VRF();

   // Turn off security
   // TODO would be nice to have secure version of SHA256 so we don't have to
   //    disable security.
   sdram_get_func_table()->enable_security(NULL, 0);

   // Compute signature
   hmac_compute_hash(HMAC_HASH_SHA256, key.u8, KEY_SIZE_IN_BYTES,
      vcsuspend_scratchpad.fields.secure_data,
      vcsuspend_scratchpad.fields.secure_data_size, signature);

   // Restore security
   sdram_get_func_table()->enable_security(NULL, secure_start_reg);

   // Lock vector core again
   vclib_obtain_VRF(1);

} // vcsuspend_compute_secure_sig()
#endif
//-----------------------------------------------------------------------------

