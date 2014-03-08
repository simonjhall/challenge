#define V3D_HACKY_RESET
//#define V3D_DISABLE_L2C
//#define V3D_DISABLE_QPUS 1 /* all but this many qpus disabled */

/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#define LOG_TAG	"V3D_Driver"
#define LOGV printf

#include "intctrl.h"
#include "v3d.h"
#include "helpers/sysman/sysman.h"
#include "helpers/vcsuspend/vcsuspend.h"
#include "interface/vcos/vcos.h"
#include "vcinclude/hardware.h"
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

unsigned int memory_pool_size = 0;

#ifdef ANDROID
#include <cutils/log.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <utils/threads.h>
#include "v3d_linux.h"


static pthread_t cleanup_catch_thread;
static volatile int running_cleanup;

static pthread_t isr_catch_thread;
static volatile int running;
static gl_irq_flags_t irq_flags;
static int fd;
#ifdef BRCM_V3D_OPT
void *regbase = NULL, *memory_pool_base = NULL;	// virtual address
unsigned long memory_pool_addr = 0;		// physical address
unsigned int memory_pool_size = 0;
#else
void *regbase, *memory_pool_base;	// virtual address
unsigned long memory_pool_addr;		// physical address
unsigned int memory_pool_size;
#endif
#endif // ANDROID

#if defined(__HERA_V3D__)

#include "vc_asm_ops.h"
#define _min(x,y) ((x)<(y) ? (x):(y))
#define _max(x,y) ((x)>(y) ? (x):(y))
#include "interface/vcos/vcos.h"

//#ifndef ANDROID
//#include "mobcom_types.h"
//#endif

#define write_reg(x, y)		(*(volatile unsigned int*)(x) = y)
#define read_reg(x)			(*(volatile unsigned int *)(x))

#ifndef ANDROID
#if defined(_HERA_) || defined(_RHEA_)
#define WR_ACCESS			0x00000000
#define MM_RST_BASE			0x3C000f00
#define SOFT_RSTN0			(MM_RST_BASE+0x00000004)
#define V3D_SOFT_RSTN		0x00000020
#elif defined(_ATHENA_)
static void V3D_Disable_Power(void);
static void V3D_Enable_Power(void);
#include "clk_drv.h"
#include "syscfg_drv.h"
#endif
#endif // ANDROID

#endif

/******************************************************************************
private type defs
******************************************************************************/

#define USERS_N 8 /* should be plenty... */
#define ALL_USERS_MASK ((1 << USERS_N) - 1)
vcos_static_assert(USERS_N <= 32); /* free_users_mask is a uint32_t... */

typedef struct {
   V3D_CALLBACK_T callback;
   V3D_MAIN_ISR_T main_isr;
   V3D_USH_ISR_T ush_isr;
} USER_T;

#define REQUESTS_N USERS_N /* at most one request per user */

typedef struct {
   int both; /* is this request in both fifos? */
   USER_T *user;
   void *callback_p;
} REQUEST_T;

typedef enum {
   PART_MAIN,
   PART_USH
} PART_T;

typedef struct {
   VCOS_MUTEX_T mutex;

   VCOS_ISR_HANDLER_T old_isr; /* previous v3d isr */

   /* this handle is valid iff free_users_mask != ALL_USERS_MASK, ie iff we have
    * at least one user */
   SYSMAN_HANDLE_T sysman_handle;

   /* users are allocated by v3d_open and freed by v3d_close */
   USER_T users[USERS_N];
   uint32_t free_users_mask;

   struct {
      /* current owner. the current owner can modify the acquire count without
       * grabbing the mutex */
      USER_T *owner;
      uint32_t acquire_n;

      /* request fifo */
      REQUEST_T requests[REQUESTS_N], *requests_read;
      uint32_t requests_n;
   } parts[2];

   uint32_t user_vpm; /* current user vpm */
   uint32_t final_user_vpm; /* what will user vpm be after request fifos empty? */

   int on; /* just for debugging */

   VCOS_EVENT_T suspend_event;
} V3D_STATE_T;

/******************************************************************************
private function declarations
******************************************************************************/

/* common api */
static int32_t v3d_init(void);
static int32_t v3d_exit(void);
static int32_t v3d_info(const char **driver_name,
                        uint32_t *version_major,
                        uint32_t *version_minor,
                        DRIVER_FLAGS_T *flags);
static int32_t v3d_open(const V3D_PARAMS_T *params,
                        DRIVER_HANDLE_T *handle);
static int32_t v3d_close(DRIVER_HANDLE_T handle);

/* v3d api */
static int32_t v3d_acquire_main(DRIVER_HANDLE_T handle,
                                void *callback_p,
                                uint32_t actual_user_vpm,
                                uint32_t max_user_vpm,
                                int force_reset);
static int32_t v3d_release_main(DRIVER_HANDLE_T handle,
                                uint32_t max_user_vpm);
static int32_t v3d_acquire_ush(DRIVER_HANDLE_T handle,
                               void *callback_p,
                               uint32_t actual_user_vpm,
                               uint32_t min_user_vpm,
                               int force_reset);
static int32_t v3d_release_ush(DRIVER_HANDLE_T handle,
                               uint32_t min_user_vpm);
static int32_t v3d_acquire_main_and_ush(DRIVER_HANDLE_T handle,
                                        void *callback_p,
                                        uint32_t user_vpm);

/* helpers */
#ifndef NDEBUG
static int is_valid_user(USER_T *user);
#endif
static int user_vpm_is_ok(PART_T part, uint32_t user_vpm, uint32_t max_min_user_vpm);
static int acquire(PART_T part, USER_T *user, void *callback_p, uint32_t actual_user_vpm, uint32_t max_min_user_vpm, int force_reset);
static void release(PART_T part, USER_T *user, uint32_t max_min_user_vpm);
static void turn_on(void);
static void turn_off(void);
static REQUEST_T *next_request(PART_T part, REQUEST_T *r);
static int post_request(PART_T part, int both, USER_T *user, void *callback_p);
static void v3d_isr(VCOS_UNSIGNED vector);
static void signal_event(void *event);
static int32_t v3d_do_suspend_resume(uint32_t resume, VCSUSPEND_CALLBACK_ARG_T);

/******************************************************************************
private data
******************************************************************************/

static const V3D_DRIVER_T v3d_func_table = {
   /* common api */
   v3d_init,
   v3d_exit,
   v3d_info,
   v3d_open,
   v3d_close,

   /* v3d api */
   v3d_acquire_main,
   v3d_release_main,
   v3d_acquire_ush,
   v3d_release_ush,
   v3d_acquire_main_and_ush
   };

//the automatic registering of the driver - DO NOT REMOVE!
#pragma Data(DATA, ".drivers") //$v3d
static const V3D_DRIVER_T *v3d_func_table_ptr = &v3d_func_table;
#pragma Data()

static V3D_STATE_T v3d_state;

/******************************************************************************
public function defs
******************************************************************************/

const V3D_DRIVER_T *v3d_get_func_table(void)
{
   return v3d_func_table_ptr;
}

/******************************************************************************
private function defs: common api
******************************************************************************/

/* initialise the driver
 * return 0 on success; all other values are failures */
#ifdef ANDROID
static int32_t v3d_init_linux(void)
{
   mem_t mempool;
   char*temp;

   fd = open("/dev/" V3D_DEV_NAME, O_RDWR);
   if (fd < 0)
      return -1;

   regbase = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if (regbase == MAP_FAILED)
      return -1;

   if (ioctl(fd, V3D_IOCTL_GET_MEMPOOL, &mempool) < 0)
      return -1;

#ifndef BRCM_V3D_OPT
   mempool.ptr = mmap(0, mempool.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mempool.addr);
   if (mempool.ptr == MAP_FAILED)
      return -1;

   memory_pool_base = mempool.ptr;
   memory_pool_size = V3D_MEMPOOL_SIZE;
   memory_pool_addr = mempool.addr;
#endif

   return 0;
}

static int32_t v3d_exit_linux(void)
{
   if (regbase != MAP_FAILED)
      munmap(regbase, PAGE_SIZE);

   if (fd > 0)
      close(fd);

   return 0;
}

static void *early_suspend_routine(void *arg)
{
   v3d_do_suspend_resume(0, NULL);
   ioctl(fd, V3D_IOCTL_EARLY_SUSPEND);
   return arg;
}

static int start_early_suspend_thread(void)
{
   pthread_t early_suspend_thread;

   if (pthread_create(&early_suspend_thread, NULL, early_suspend_routine, NULL)) {
      return -1;
   }
   return 0;
}

#define GE_IOC_MAGIC  'g'
#define GE_IOC_RESERVE           _IO(GE_IOC_MAGIC,  6)
#define GE_IOC_UNRESERVE           _IO(GE_IOC_MAGIC,  7)
#define GE_IOC_WAIT           _IO(GE_IOC_MAGIC,  8)


void eglIntDestroyByPid_impl(uint32_t pid_0, uint32_t pid_1);

static void *cleanup_catch_routine(void *arg)
{
	int fd = open("/dev/ge_drv",O_RDWR);
	while(running_cleanup) {
		uint64_t ge_drv_cleanup_data =0;
		int retval = ioctl(fd,GE_IOC_WAIT,&ge_drv_cleanup_data);
		LOGE("v3d cleanup ioctl returned %x %x",(uint32_t)ge_drv_cleanup_data, (uint32_t)(ge_drv_cleanup_data>>32));
		eglIntDestroyByPid_impl((uint32_t)ge_drv_cleanup_data, (uint32_t)(ge_drv_cleanup_data>>32));
		}
	LOGE("cleanup returning");
	return arg;
}
static void *isr_catch_routine(void *arg)
{
   while(running) {
      if(ioctl(fd, V3D_IOCTL_WAIT_IRQ, &irq_flags)) {
         LOGE("V3D_IOCTL_WAIT_IRQ error\n");
         continue;
      }
// DEBUG_V3D needs to be enabled in both kernel and userspace v3d.c file
#ifdef DEBUG_V3D
      LOGE("V3D interrupt 0x%04x, flags = 0x%04x\n", (irq_flags >> 16), (irq_flags & 0xffff));
      irq_flags &= 0xffff;
#endif
      if (irq_flags.v3d_irq_flags || irq_flags.qpu_irq_flags) {
         v3d_isr(INTERRUPT_3D);
      }

     if (0) {//irq_flags.early_suspend == V3D_SUSPEND) {
         start_early_suspend_thread();
      } else if (irq_flags.early_suspend == V3D_RESUME) {
         v3d_do_suspend_resume(1, NULL);
      }
   }
   return arg;
}

static int start_isr_catch_thread(void)
{
   int policy;
   struct sched_param param;

   running = 1;
   if (pthread_create(&isr_catch_thread, NULL, isr_catch_routine, NULL)) {
      running = 0;
      return -1;
   }

   if (!pthread_getschedparam(isr_catch_thread, &policy, &param)) {
      param.sched_priority = ANDROID_PRIORITY_URGENT_DISPLAY;
      pthread_setschedparam(isr_catch_thread, policy, &param);
   }

   running_cleanup =1;
   if (pthread_create(&cleanup_catch_thread, NULL, cleanup_catch_routine, NULL)) {
      running_cleanup = 0;
      return -1;
   }

   if (!pthread_getschedparam(cleanup_catch_thread, &policy, &param)) {
      param.sched_priority = ANDROID_PRIORITY_URGENT_DISPLAY;
      pthread_setschedparam(cleanup_catch_thread, policy, &param);
   }

   return 0;
}

static void stop_isr_catch_thread(void)
{
   running = 0;
   pthread_kill(isr_catch_thread, SIGKILL);
   running_cleanup = 0;
   pthread_kill(cleanup_catch_thread, SIGKILL);
}

static void soft_reset(void)
{
   ioctl(fd, V3D_IOCTL_SOFT_RESET);
}

static void turn_on_linux(void)
{
   ioctl(fd, V3D_IOCTL_TURN_ON);
}

static void turn_off_linux(void)
{
   ioctl(fd, V3D_IOCTL_TURN_OFF);
}
#endif // ANDROID

static int32_t v3d_init(void)
{
   uint32_t i;


   if (!vcos_verify(vcos_mutex_create(&v3d_state.mutex, "v3d_mutex") == VCOS_SUCCESS)) {
      return -1;
   }

   if (!vcos_verify(vcos_event_create(&v3d_state.suspend_event, "v3d_suspend_event") == VCOS_SUCCESS)) {
      vcos_mutex_delete(&v3d_state.mutex);

      return -1;
   }

#ifdef ANDROID
   if (v3d_init_linux() < 0)
      return -1;
#endif

#ifndef ANDROID
   vcos_register_isr(INTERRUPT_3D, v3d_isr, &v3d_state.old_isr);
#else
   // spawn thread to catch isr
   v3d_state.old_isr = NULL;
#ifndef BRCM_V3D_OPT
   start_isr_catch_thread();
#endif
#endif // ANDROID


   v3d_state.sysman_handle = NULL; /* just to make sure we don't try and use it */

   v3d_state.free_users_mask = ALL_USERS_MASK;

   for (i = 0; i != 2; ++i) {
      v3d_state.parts[i].owner = NULL;
      v3d_state.parts[i].acquire_n = 0;

      v3d_state.parts[i].requests_read = v3d_state.parts[i].requests;
      v3d_state.parts[i].requests_n = 0;
   }

   v3d_state.user_vpm = 0;
   v3d_state.final_user_vpm = 0;

   v3d_state.on = 0;

   return 0;
}

/* exit the driver
 * always succeeds and returns 0 */
static int32_t v3d_exit(void)
{
   uint32_t i;
   VCOS_ISR_HANDLER_T tmp_isr;

   vcos_event_delete(&v3d_state.suspend_event);

   vcos_assert(!v3d_state.on);

   vcos_assert(v3d_state.user_vpm == v3d_state.final_user_vpm);

   for (i = 0; i != 2; ++i) {
      vcos_assert(v3d_state.parts[i].requests_n == 0);

      vcos_assert(v3d_state.parts[i].acquire_n == 0);
      vcos_assert(!v3d_state.parts[i].owner);
   }

   vcos_assert(v3d_state.free_users_mask == ALL_USERS_MASK);

#ifndef ANDROID
   vcos_register_isr(INTERRUPT_3D, v3d_state.old_isr, &tmp_isr);
#else
   // exit isr thread
#ifndef BRCM_V3D_OPT
   stop_isr_catch_thread();
#endif
#endif // ANDROID

#ifdef ANDROID
   v3d_exit_linux();
#endif

   vcos_assert(tmp_isr == v3d_isr);

   vcos_mutex_delete(&v3d_state.mutex);

   return 0;
}

/* get the driver info
 * return 0 on success; all other values are failures */
static int32_t v3d_info(const char **driver_name,
                        uint32_t *version_major,
                        uint32_t *version_minor,
                        DRIVER_FLAGS_T *flags)
{
   if (!driver_name || !version_major || !version_minor || !flags) {
      return -1;
   }

   *driver_name = "v3d";
   *version_major = 0;
   *version_minor = 1;
   *flags = (DRIVER_FLAGS_T)0;

   return 0;
}

//sjh
int _msb(int val)
{
   unsigned int msb=31;
   if (val==0) return -1;
   while ((val&(1<<msb))==0)
      msb--;
   return (int)msb;
}

/* open the driver and obtain a handle
 * return 0 on success; all other values are failures */
static int32_t v3d_open(const V3D_PARAMS_T *params,
                        DRIVER_HANDLE_T *handle)
{
   uint32_t i;

   vcos_mutex_lock(&v3d_state.mutex);

   if (!v3d_state.free_users_mask) {
      vcos_mutex_unlock(&v3d_state.mutex);
      return -1;
   }

   if (v3d_state.free_users_mask == ALL_USERS_MASK) {
#ifndef ANDROID
#ifndef __HERA_V3D__
	   printf("sysman_register_user %s %d\n", __FILE__, __LINE__);
      /*if (sysman_register_user(&v3d_state.sysman_handle) != 0) {
         vcos_mutex_unlock(&v3d_state.mutex);
         return -1;
      }*/

      /* calling multiple times with the same arguments is fine -- every call
       * after the first will be ignored */
      LOGV("vcsuspend_register %s %d\n", __FILE__, __LINE__);
//      vcsuspend_register(VCSUSPEND_RUNLEVEL_V3D, v3d_do_suspend_resume, NULL);
#else
#ifdef _ATHENA_
      V3D_Enable_Power();
#endif
#endif
#endif // ANDROID
   }

   i = _msb(v3d_state.free_users_mask);
   vcos_assert(i != -1); /* or we should have failed above */
   v3d_state.free_users_mask &= ~(1 << i);

   vcos_mutex_unlock(&v3d_state.mutex);

   *handle = (DRIVER_HANDLE_T)(v3d_state.users + i);
   v3d_state.users[i].callback = params->callback;
   v3d_state.users[i].main_isr = params->main_isr;
   v3d_state.users[i].ush_isr = params->ush_isr;

   return 0;
}

/* close a handle to the driver
 * always succeeds and returns 0 */
static int32_t v3d_close(DRIVER_HANDLE_T handle)
{
   USER_T *user = (USER_T *)handle;
   vcos_assert(is_valid_user(user));

   vcos_mutex_lock(&v3d_state.mutex);

   v3d_state.free_users_mask |= 1 << (user - v3d_state.users);
   if (v3d_state.free_users_mask == ALL_USERS_MASK) {
#ifndef ANDROID
#ifndef __HERA_V3D__
      /* todo: vcsuspend_unregister? */

	   printf("sysman_deregister_user %s %d\n", __FILE__, __LINE__);
//      int32_t success = sysman_deregister_user(v3d_state.sysman_handle);
//      vcos_assert(success == 0);
#else
#ifdef _ATHENA_
   V3D_Disable_Power();
#endif
#endif
#endif // ANDROID
      v3d_state.sysman_handle = NULL; /* just to make sure we don't try and use it */
   }

   vcos_mutex_unlock(&v3d_state.mutex);

   return 0;
}

/******************************************************************************
private function defs: v3d api
******************************************************************************/

/* routine to acquire control of the main part of the v3d block (the cle + co)
 * return 0 on success; all other values are failures. if the call fails, the
 * user's callback will be called when the user should try again (they can try
 * again before then, but they won't succeed) */
static int32_t v3d_acquire_main(DRIVER_HANDLE_T handle,
                                void *callback_p,
                                uint32_t actual_user_vpm, /* if we get the opportunity, we'll set the user vpm to this. ignored if forcing a reset */
                                uint32_t max_user_vpm, /* user vpm will be <= this until the corresponding call to release_main */
                                int force_reset)
{
#ifdef BRCM_V3D_OPT
	return 0;
#else
   USER_T *user = (USER_T *)handle;
   vcos_assert(is_valid_user(user));
   return acquire(PART_MAIN, user, callback_p, actual_user_vpm, max_user_vpm, force_reset) ? 0 : -1;
#endif
}

/* routine to release control of the main part of the v3d block (the cle + co)
 * always succeeds and returns 0 */
static int32_t v3d_release_main(DRIVER_HANDLE_T handle,
                                uint32_t max_user_vpm) /* must match the value that was passed in the corresponding call to v3d_acquire_main */
{
#ifdef BRCM_V3D_OPT
	return 0;
#else
   USER_T *user = (USER_T *)handle;
   vcos_assert(is_valid_user(user));
   release(PART_MAIN, user, max_user_vpm);
   return 0;
#endif
}

/* routine to acquire control of the user shader queue
 * return 0 on success; all other values are failures. if the call fails, the
 * user's callback will be called when the user should try again (they can try
 * again before then, but they won't succeed) */
static int32_t v3d_acquire_ush(DRIVER_HANDLE_T handle,
                               void *callback_p,
                               uint32_t actual_user_vpm, /* if we get the opportunity, we'll set the user vpm to this. ignored if forcing a reset */
                               uint32_t min_user_vpm, /* user vpm will be >= this until the corresponding call to release_ush */
                               int force_reset)
{
#ifdef BRCM_V3D_OPT
	return 0;
#else
   USER_T *user = (USER_T *)handle;
   vcos_assert(is_valid_user(user));
   return acquire(PART_USH, user, callback_p, actual_user_vpm, min_user_vpm, force_reset) ? 0 : -1;
#endif
}

/* routine to release control of the user shader queue
 * always succeeds and returns 0 */
static int32_t v3d_release_ush(DRIVER_HANDLE_T handle,
                               uint32_t min_user_vpm) /* must match the value that was passed in the corresponding call to v3d_acquire_ush */
{
#ifdef BRCM_V3D_OPT
	return 0;
#else
   USER_T *user = (USER_T *)handle;
   vcos_assert(is_valid_user(user));
   release(PART_USH, user, min_user_vpm);
   return 0;
#endif
}

/* routine to acquire control of both the main part of the v3d block (the cle +
 * co) and the user shader queue. this is used:
 * - on a0, to workaround hw-2422
 * - on b0, to workaround hw-2959
 * - locally, for suspending */
static int32_t v3d_acquire_main_and_ush(DRIVER_HANDLE_T handle,
                                        void *callback_p,
                                        uint32_t user_vpm)
{
#ifdef BRCM_V3D_OPT
	return 0;
#else
   USER_T *user = (USER_T *)handle;
   vcos_assert(is_valid_user(user));

   vcos_mutex_lock(&v3d_state.mutex);

   /*
      make sure we have an entry in the fifos
   */

   if (post_request(PART_MAIN, 1, user, callback_p)) {
      if (!post_request(PART_USH, 1, user, callback_p)) {
         vcos_assert(0); /* not in the main fifo -- shouldn't be in the ush fifo either */
      }
      v3d_state.final_user_vpm = user_vpm;
   }

   /*
      check if we're good to go
   */

   if (v3d_state.parts[PART_MAIN].owner || (v3d_state.parts[PART_MAIN].requests_read->user != user) ||
      v3d_state.parts[PART_USH].owner || (v3d_state.parts[PART_USH].requests_read->user != user)) {
      vcos_mutex_unlock(&v3d_state.mutex);
      return -1;
   }

   /*
      we are good to go. pop our fifo entries
   */

   v3d_state.parts[PART_MAIN].requests_read = next_request(PART_MAIN, v3d_state.parts[PART_MAIN].requests_read);
   --v3d_state.parts[PART_MAIN].requests_n;
   v3d_state.parts[PART_USH].requests_read = next_request(PART_USH, v3d_state.parts[PART_USH].requests_read);
   --v3d_state.parts[PART_USH].requests_n;
   v3d_state.user_vpm = user_vpm;

   /*
      turn v3d on and mark main/ush as owned by us
   */

   turn_on();
   v3d_state.parts[PART_MAIN].owner = user;
   vcos_assert(v3d_state.parts[PART_MAIN].acquire_n == 0);
   v3d_state.parts[PART_MAIN].acquire_n = 1;
   v3d_state.parts[PART_USH].owner = user;
   vcos_assert(v3d_state.parts[PART_USH].acquire_n == 0);
   v3d_state.parts[PART_USH].acquire_n = 1;

   vcos_mutex_unlock(&v3d_state.mutex);
   return 0;
#endif
}

/******************************************************************************
private function defs: helpers
******************************************************************************/

#ifndef NDEBUG
static int is_valid_user(USER_T *user)
{
   uint32_t i = user - v3d_state.users;
   return (i < USERS_N) && (user == (v3d_state.users + i)) &&
      !(v3d_state.free_users_mask & (1 << i));
}
#endif

/*
   the system is first-come-first-serve, ie if your first call to acquire is
   before someone else's, you will get access first. we maintain a fifo of
   requests to do this. although "nested" acquires are allowed, to ensure
   starvation doesn't occur, a "nested" acquire will fail if someone is waiting
   (ie the fifo is not empty) and will end up as a request at the end of the
   fifo

   to make "nested" acquires/releases fast, we handle the common case (noone is
   waiting) without grabbing the mutex. this means that users need to
   synchronise their own calls to acquire/release

   to keep things simple and bound the maximum fifo size, we only allow one
   outstanding request per user (ie if a user calls acquire and it fails, the
   next time they call an acquire it must be the same one and with the same
   parameters) (they're also not allowed to give up and call close if they have
   an outstanding request). so we can tell if an acquire call is a repeat after
   a failure by seeing if the user has a request in the fifo (if they do, it
   must match this acquire, and so we won't post another request)

   as the two parts of v3d are somewhat separate, there are actually two fifos.
   the two parts aren't completely separate though, as the vpm is explicitly
   shared between them -- it is statically partitioned by setting the VPMBASE
   register. the VPMBASE register reserves the specified amount of vpm space for
   user shaders, leaving the rest for the main system

   when posting a request, if we determine that the partition at the time the
   request will be satisfied isn't going to be ok (eg it's a ush request and
   there's not going to be enough vpm space reserved for user shaders), we put
   linked requests into both fifos so we can change the VPMBASE register when
   the time comes. linked requests aren't popped from the fifos until both
   requests are ready. the linked requests serve two purposes:
   - they cause the v3d block to be reset (as when neither part of the v3d block
     is being used, it is turned off). this is necessary as due to hardware
     limitations, the VPMBASE register can only be changed right after a reset
   - they ensure we won't violate any assumptions previously queued requests
     made about the vpm partition

   if we determine that the partition at the time the request will be satisfied
   is going to be ok, we simply put a standalone request in the appropriate fifo

   so how do we know what the partition will be at the time the request will be
   satisfied? we keep track of what VPMBASE will be set to once all the
   outstanding requests have been satisfied (final_user_vpm). this is the same
   for both fifos as we sync the fifos with linked requests before changing
   VPMBASE

   todo: there are some cases where we can't change the vpm partition even
   though it might actually be ok. eg if there's nothing using the hw right now
   and there's only user shader stuff queued up, we always have to wait until
   the user shader stuff is done before we change the user vpm for main stuff
   even if the queued up user shader stuff would be ok with the lower user vpm
*/

static int user_vpm_is_ok(PART_T part, uint32_t user_vpm, uint32_t max_min_user_vpm)
{
   return (part == PART_MAIN) ? (user_vpm <= max_min_user_vpm) : (user_vpm >= max_min_user_vpm);
}

static int acquire(PART_T part, USER_T *user, void *callback_p, uint32_t actual_user_vpm, uint32_t max_min_user_vpm, int force_reset)
{
   PART_T other_part = (part == PART_MAIN) ? PART_USH : PART_MAIN;
   vcos_assert(force_reset || user_vpm_is_ok(part, actual_user_vpm, max_min_user_vpm));

   /*
      fast case (no need to acquire the mutex): we already own, user vpm is ok,
      there are no requests, and we don't want to force a reset
   */

#if defined(_ATHENA_) && (CHIP_REVISION==20)
#ifndef ANDROID
	// SW patch for Async APB bridge HW bug
    *((volatile unsigned int *)(0x8950438)) = 0;
    if (*((volatile unsigned int *)(0x8950438)) != 0)
        *((volatile unsigned int *)(0x8950438)) = 0;
#endif
#endif

   if ((v3d_state.parts[part].owner == user) &&
      user_vpm_is_ok(part, v3d_state.user_vpm, max_min_user_vpm) &&
      !v3d_state.parts[part].requests_n && !force_reset) {
      ++v3d_state.parts[part].acquire_n;
      return 1;
   }

   /*
      make sure we have an entry in the fifo(s)
   */

   vcos_mutex_lock(&v3d_state.mutex);
   /* we go in both fifos iff we want to change the user vpm, which we'll want
    * to do either because we need to... */
   {
   int both = !user_vpm_is_ok(part, v3d_state.final_user_vpm, max_min_user_vpm) ||
      /* ...or because we have the opportunity to do so... */
      (!v3d_state.parts[PART_MAIN].owner && !v3d_state.parts[PART_MAIN].requests_n &&
      !v3d_state.parts[PART_USH].owner && !v3d_state.parts[PART_USH].requests_n) ||
      /* ...or because we want to force a reset */
      force_reset;
   /* note that both is based on the assumption that we're going at the end of
    * the fifo(s), and thus isn't valid if we're already in the fifo(s). if
    * we're already in the fifo(s), post_request will ignore both and return 0 */
   if (post_request(part, both, user, callback_p) && both) {
      if (!post_request(other_part, 1, user, callback_p)) {
         vcos_assert(0); /* not in part's fifo -- shouldn't be in the other fifo either */
      }
      v3d_state.final_user_vpm = force_reset ? (
         user_vpm_is_ok(part, v3d_state.final_user_vpm, max_min_user_vpm) ?
         v3d_state.final_user_vpm : max_min_user_vpm) : actual_user_vpm;
   }
   }

   /*
      check if we're good to go
   */

   if (v3d_state.parts[part].owner || (v3d_state.parts[part].requests_read->user != user) ||
      (v3d_state.parts[part].requests_read->both && (
      v3d_state.parts[other_part].owner || (v3d_state.parts[other_part].requests_read->user != user)))) {
      vcos_mutex_unlock(&v3d_state.mutex);
      return 0;
   }

   /*
      we are good to go. pop our fifo entries
   */

   if (v3d_state.parts[part].requests_read->both) {
      v3d_state.parts[other_part].requests_read = next_request(other_part, v3d_state.parts[other_part].requests_read);
      --v3d_state.parts[other_part].requests_n;
      /* we're not going to own other part, next guy might be able to go */
      if (v3d_state.parts[other_part].requests_n && !v3d_state.parts[other_part].requests_read->both) {
         /* although normally we avoid making callbacks while holding
          * mutexes, in this case (even though it might cause some extra task
          * switches) it is necessary as we don't want to call someone's
          * callback after they succeed (they won't be expecting it!) */
         v3d_state.parts[other_part].requests_read->user->callback(v3d_state.parts[other_part].requests_read->callback_p);
      }
      v3d_state.user_vpm = force_reset ? (
         user_vpm_is_ok(part, v3d_state.user_vpm, max_min_user_vpm) ?
         v3d_state.user_vpm : max_min_user_vpm) : actual_user_vpm;
   }
   v3d_state.parts[part].requests_read = next_request(part, v3d_state.parts[part].requests_read);
   --v3d_state.parts[part].requests_n;

   /*
      make sure v3d is on and mark part as owned by us
   */

   vcos_assert(user_vpm_is_ok(part, v3d_state.user_vpm, max_min_user_vpm));
   if (!v3d_state.parts[other_part].owner) {
      turn_on();
   }
   v3d_state.parts[part].owner = user;
   vcos_assert(v3d_state.parts[part].acquire_n == 0);
   v3d_state.parts[part].acquire_n = 1;

   vcos_mutex_unlock(&v3d_state.mutex);
   return 1;
}

static void release(PART_T part, USER_T *user, uint32_t max_min_user_vpm)
{
   PART_T other_part = (part == PART_MAIN) ? PART_USH : PART_MAIN;
   vcos_assert(user_vpm_is_ok(part, v3d_state.user_vpm, max_min_user_vpm));
   if (--v3d_state.parts[part].acquire_n) {
      return;
   }
   vcos_mutex_lock(&v3d_state.mutex);
   vcos_assert(v3d_state.parts[part].owner == user);
   v3d_state.parts[part].owner = NULL;
   if (!v3d_state.parts[other_part].owner) {
      turn_off();
   }
   if (v3d_state.parts[part].requests_n && (!v3d_state.parts[part].requests_read->both || (!v3d_state.parts[other_part].owner &&
      (v3d_state.parts[other_part].requests_read->user == v3d_state.parts[part].requests_read->user)))) {
      v3d_state.parts[part].requests_read->user->callback(v3d_state.parts[part].requests_read->callback_p); /* see comment about callback while holding mutex in acquire */
   }
   vcos_mutex_unlock(&v3d_state.mutex);
}

void vclib_dcache_invalidate_range(void *start_addr, int length)
{
	printf("vclib_dcache_invalidate_range %s %d\n", __FILE__, __LINE__);
}

void _vcos_tls_thread_ptr_set(void *p)
{
	printf("_vcos_tls_thread_ptr_set %s %d\n", __FILE__, __LINE__);
}

void *_vcos_tls_thread_ptr_get(void)
{
	printf("_vcos_tls_thread_ptr_get %s %d\n", __FILE__, __LINE__);
}

#ifdef V3D_HACKY_RESET
#include "helpers/clockman/clockman.h"
static void hacky_reset(void)
{
#if 0
   /* hack: do a reset manually (i just copied this from 2708/power.c. not sure
    * how much is actually necessary) */

#ifndef ANDROID
#ifdef __HERA_V3D__
#if defined(_HERA_) || defined(_RHEA_)
   // reset v3d
   write_reg(MM_RST_BASE, 0x00a5a501);

   write_reg(SOFT_RSTN0, read_reg(SOFT_RSTN0) & ~V3D_SOFT_RSTN);
   while (read_reg(SOFT_RSTN0) & V3D_SOFT_RSTN)
      ;

   write_reg(SOFT_RSTN0, read_reg(SOFT_RSTN0) | V3D_SOFT_RSTN);
   while ((read_reg(SOFT_RSTN0) & 0x20) != V3D_SOFT_RSTN)
      ;
#elif defined(_ATHENA_)
#endif

#else
   // stop the async axi interface
   ASB_V3D_M_CTRL |= ASB_V3D_M_CTRL_CLR_REQ_SET;
   while( (ASB_V3D_M_CTRL & ASB_V3D_M_CTRL_CLR_ACK_SET) == 0 )
      ;
   ASB_V3D_S_CTRL |= ASB_V3D_S_CTRL_CLR_REQ_SET;
   while( (ASB_V3D_S_CTRL & ASB_V3D_S_CTRL_CLR_ACK_SET) == 0 )
      ;

   clockman_setup_clock( CLOCK_OUTPUT_V3D, 0, 0, 0 );

   uint32_t n = 32;
   while (n--) { _vasm("nop"); }

   PM_GRAFX = PM_PASSWORD | (PM_GRAFX & PM_GRAFX_V3DRSTN_CLR);

   n = 32;
   while (n--) { _vasm("nop"); }

   clockman_setup_clock( CLOCK_OUTPUT_V3D, 250*1000*1000, 0, 0 );

   // enable async bridge
   ASB_V3D_S_CTRL &= ASB_V3D_S_CTRL_CLR_REQ_CLR;
   while( ASB_V3D_S_CTRL & ASB_V3D_S_CTRL_CLR_ACK_SET )
      ;
   ASB_V3D_M_CTRL &= ASB_V3D_M_CTRL_CLR_REQ_CLR;
   while( ASB_V3D_M_CTRL & ASB_V3D_M_CTRL_CLR_ACK_SET )
      ;

   n = 32;
   while (n--) { _vasm("nop"); }

   clockman_setup_clock( CLOCK_OUTPUT_V3D, 0, 0, 0 );

   n = 32;
   while (n--) { _vasm("nop"); }

   PM_GRAFX |= PM_PASSWORD | PM_GRAFX_V3DRSTN_SET;

   n = 32;
   while (n--) { _vasm("nop"); }

   clockman_setup_clock( CLOCK_OUTPUT_V3D, 250*1000*1000, 0, 0 );
#endif
#endif // ANDROID
#endif
}
#endif

const INTCTRL_DRIVER_T *intctrl_get_func_table( void )
{
	printf("intctrl_get_func_table %s %d\n", __FILE__, __LINE__);
}

static void turn_on(void)
{
#ifdef ANDROID
   turn_on_linux();
   soft_reset();
#endif
   vcos_assert(!v3d_state.on);
   v3d_state.on = 1;

   /* we assume that the v3d block will be reset when we ask sysman to turn it
    * on: VPMBASE writes are potentially unsafe in a non-reset state (see
    * hw-2791)... */

#ifndef __HERA_V3D__
   /* if this assert fires, someone else is keeping v3d on. all users of v3d
    * should use this driver and not be talking to sysman directly... */
   printf("sysman_is_enabled assert check %s %d\n", __FILE__, __LINE__);
//   vcos_assert(!sysman_is_enabled(SYSMAN_BLOCK_V3D));
#endif

   /* the v3d block should be being held in reset at this point. if it isn't,
    * it's probably because sysman is disabled (ie everything is kept on all the
    * time). we need the reset in order to be able to safely change VPMBASE */
#ifdef V3D_HACKY_RESET
#ifndef __HERA_V3D__
   if (PM_GRAFX & PM_GRAFX_V3DRSTN_SET)
#endif
   {
      hacky_reset();
   }
#else
   vcos_assert(!(PM_GRAFX & PM_GRAFX_V3DRSTN_SET));
#endif

#ifdef __HERA_V3D__
#ifndef ANDROID
   vcos_set_interrupt_mask(INTERRUPT_3D, 1);
#endif
#else
   printf("sysman_set_user_request %s %d\n", __FILE__, __LINE__);
//   sysman_set_user_request(v3d_state.sysman_handle, SYSMAN_BLOCK_V3D, 1, SYSMAN_WAIT_BLOCKING);
   intctrl_get_func_table()->set_imask_per_core(0, 0, INTERRUPT_3D, 1); /* enable 3d interrupts on vpu0 */
#endif

   vcos_assert(v3d_state.user_vpm <= 16);
   v3d_write(VPMBASE, v3d_state.user_vpm);
#ifdef __BCM2708A0__
   v3d_write(DBSCFG, 0x3f); /* make sure we won't be connected to a client when we turn debug on (on b0, this is the reset value) */
#endif
   v3d_write(DBCFG, 1); /* need debug enabled to halt/run/see interrupts/etc */

#ifdef V3D_DISABLE_L2C
   v3d_write(L2CACTL, 1 << 1);
#endif
#ifdef V3D_DISABLE_QPUS
   v3d_write(SQRSV0, 0xffffffff ^
      ((1 << (((V3D_DISABLE_QPUS >= 8) ? 8 : V3D_DISABLE_QPUS) << 2)) - 1));
   v3d_write(SQRSV1, 0xffffffff ^
      ((1 << (((V3D_DISABLE_QPUS <= 8) ? 0 : (V3D_DISABLE_QPUS - 8)) << 2)) - 1));
#endif
}

static void turn_off(void)
{
   vcos_assert(v3d_state.on);
   v3d_state.on = 0;

   vcos_assert(!(v3d_read(PCS) & ((1 << 0) | (1 << 2)))); /* main hw isn't busy */
   vcos_assert(!(v3d_read(SRQCS) & 0x7f)); /* no user programs queued up */
   /* no user programs running (this really just checks requests count against
    * completed count) */
   assert(!(((v3d_read(SRQCS) >> 8) ^ (v3d_read(SRQCS) >> 16)) & 0xff));
#ifdef __HERA_V3D__
#ifndef ANDROID
   vcos_set_interrupt_mask(INTERRUPT_3D, 0);
#endif
#else
   intctrl_get_func_table()->set_imask_per_core(0, 0, INTERRUPT_3D, 0); /* disable 3d interrupts on vpu0 */
   printf("sysman_set_user_request %s %d\n", __FILE__, __LINE__);
//   sysman_set_user_request(v3d_state.sysman_handle, SYSMAN_BLOCK_V3D, 0, SYSMAN_WAIT_BLOCKING);
#endif
#ifdef ANDROID
   turn_off_linux();
#endif
}

static REQUEST_T *next_request(PART_T part, REQUEST_T *r)
{
   return (r == (v3d_state.parts[part].requests + (REQUESTS_N - 1))) ? v3d_state.parts[part].requests : (r + 1);
}

static int post_request(PART_T part, int both, USER_T *user, void *callback_p)
{
   REQUEST_T *r = v3d_state.parts[part].requests_read;
   uint32_t n = v3d_state.parts[part].requests_n;
   while (n--) {
      if (r->user == user) {
         vcos_assert(r->callback_p == callback_p);
         return 0;
      }
      r = next_request(part, r);
   }
   /* it shouldn't be possible to overflow the fifo -- we can have at most one
    * outstanding request per user, and the fifo is sized based on the maximum
    * number of users */
   vcos_assert(v3d_state.parts[part].requests_n != REQUESTS_N);
   ++v3d_state.parts[part].requests_n;
   r->both = both;
   r->user = user;
   r->callback_p = callback_p;
   return 1;
}

static void v3d_isr(VCOS_UNSIGNED vector)
{
   uint32_t flags;

   vcos_assert(vector == INTERRUPT_3D);

#ifdef ANDROID
   flags = irq_flags.v3d_irq_flags;
#else
   flags = v3d_read(INTCTL);
   flags &= v3d_read(INTENA);
   flags &= v3d_read(INTCTL);
#endif
   if (flags) {
      /* todo: in theory, need a barrier here */
#ifdef ANDROID
      if (v3d_state.parts[PART_MAIN].owner)
#endif
      v3d_state.parts[PART_MAIN].owner->main_isr(flags);
   }

#ifdef ANDROID
   flags = irq_flags.qpu_irq_flags;
#else
   flags = v3d_read(DBQITC);
#endif
   if (flags) {
      /* todo: in theory, need a barrier here */
#ifdef ANDROID
      if (v3d_state.parts[PART_USH].owner)
#endif
      v3d_state.parts[PART_USH].owner->ush_isr(flags);
   }

   if (v3d_state.old_isr) {
      v3d_state.old_isr(vector);
   }
}

/******************************************************************************
suspend/resume
******************************************************************************/

static void signal_event(void *event)
{
   vcos_event_signal((VCOS_EVENT_T *)event);
}

static int32_t v3d_do_suspend_resume(uint32_t resume, VCSUSPEND_CALLBACK_ARG_T x)
{
   static DRIVER_HANDLE_T handle;

   if (!resume) {
      /* suspend */

      /* register */
      V3D_PARAMS_T params = {signal_event, NULL, NULL};
      if (!vcos_verify(v3d_open(&params, &handle) == 0)) {
         /* v3d_open should only fail if there are too many users, which
          * shouldn't ever happen */
         return -1;
      }

      /* acquire v3d */
      while (v3d_acquire_main_and_ush(handle, &v3d_state.suspend_event, 0) != 0) {
         vcos_event_wait(&v3d_state.suspend_event);
      }

      /* now we own v3d, turn it off */
      turn_off();
   } else {
      /* resume */

      /* turn v3d back on */
      turn_on();

      /* release v3d */
      v3d_release_main(handle, 0);
      v3d_release_ush(handle, 0);

      /* deregister */
      v3d_close(handle);
   }

   return 0;
}


#if defined(_ATHENA_)
#ifdef ANDROID
#define USE_IOCTL
#ifndef USE_IOCTL
uint32_t v3d_read_reg(uint32_t addr)
{
    uint32_t reg_val;
    reg_val = *(volatile unsigned int*)addr;
    *(volatile unsigned long *)V3D_SCRATCH = 0;
    if ((*(volatile unsigned long *)V3D_SCRATCH) != 0)
       *(volatile unsigned long *)V3D_SCRATCH = 0;
    return reg_val;
}
#else // USE_IOCTL
uint32_t v3d_read_reg(uint32_t addr)
{
    uint32_t value = addr - V3D_BASE;
    if (ioctl(fd, V3D_IOCTL_READ_REG, &value) < 0)
       return 0;
    else
       return value;
}
#endif // USE_IOCTL
#else // ANDROID
extern UInt32 ARM_DisableIRQFIQ(void);
extern void ARM_RecoverIRQFIQ(UInt32);
uint32_t  v3d_read_reg(uint32_t addr)
{
    uint32_t irqfiq_mask, reg_val;

    irqfiq_mask = ARM_DisableIRQFIQ();
    reg_val = *(volatile unsigned int*)addr;
    *(volatile unsigned long *)V3D_SCRATCH = 0;
    if ((*(volatile unsigned long *)V3D_SCRATCH) != 0)
       *(volatile unsigned long *)V3D_SCRATCH = 0;
    ARM_RecoverIRQFIQ(irqfiq_mask);
    return reg_val;
}

static void* v3dClkHandle = NULL;
static void V3D_Disable_Power(void)
{
    vcos_assert(!v3dClkHandle);
    SYSCFGDRV_Set_Periph_AHB_Clk_Item(SYSCFG_PERIPH_AHB_CLK_DPE, SYSCFG_PERIPH_AHB_CLK_OFF);
    if (!v3dClkHandle)
    {
        v3dClkHandle = CLKDRV_Open();

        if (v3dClkHandle)
        {
            CLKDRV_Set_V3D_Power_Mode(v3dClkHandle, 0);
            CLKDRV_Close(v3dClkHandle);
            v3dClkHandle = NULL;
        }
    }
}

static void V3D_Enable_Power(void)
{
    vcos_assert(!v3dClkHandle);
    SYSCFGDRV_Set_Periph_AHB_Clk_Item(SYSCFG_PERIPH_AHB_CLK_DPE, SYSCFG_PERIPH_AHB_CLK_ON);
    if (!v3dClkHandle)
    {
        v3dClkHandle = CLKDRV_Open();

        if (v3dClkHandle)
        {
            CLKDRV_Set_V3D_Power_Mode(v3dClkHandle, 1);
            CLKDRV_Close(v3dClkHandle);
            v3dClkHandle = NULL;
        }
    }
}

#endif // ANDROID
#endif
