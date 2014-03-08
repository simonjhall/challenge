/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#define VCOS_INLINE_BODIES
#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos_msgqueue.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/param.h>

#ifdef HAVE_PRCTL
#include <sys/prctl.h>
#endif

#ifdef HAVE_CMAKE_CONFIG
#include "cmake_config.h"
#endif

#ifdef HAVE_MTRACE
#include <mcheck.h>
#endif

#ifndef VCOS_DEFAULT_STACK_SIZE
#define VCOS_DEFAULT_STACK_SIZE 4096
#endif

static int vcos_argc;
static const char **vcos_argv;

typedef void (*LEGACY_ENTRY_FN_T)(int, void *);

static VCOS_THREAD_ATTR_T default_attrs = {
   0,
   VCOS_DEFAULT_STACK_SIZE,
};

static VCOS_MUTEX_T lock;
pthread_key_t _vcos_thread_current_key;

/* A VCOS wrapper for the thread which called vcos_init. */
static VCOS_THREAD_T vcos_thread_main;

static void *vcos_thread_entry(void *arg)
{
   int i;
   void *ret;
   VCOS_THREAD_T *thread = (VCOS_THREAD_T *)arg;
   pthread_setspecific(_vcos_thread_current_key, thread);

#ifdef HAVE_PRCTL
   /* String size must be less than 16 for prctl function */
   vcos_assert(sizeof(thread->name) <= 16);

   if (strlen(thread->name))
	   prctl(PR_SET_NAME, (unsigned long) thread->name, 0, 0, 0);
#endif

   if (thread->legacy)
   {
      LEGACY_ENTRY_FN_T fn = (LEGACY_ENTRY_FN_T)thread->entry;
      (*fn)(0, thread->arg);
      ret = 0;
   }
   else
   {
      ret = (*thread->entry)(thread->arg);
   }

   // call termination functions
   for (i=0; thread->at_exit[i].pfn != NULL; i++)
   {
      thread->at_exit[i].pfn(thread->at_exit[i].cxt);
   }

   return ret;
}

VCOS_STATUS_T vcos_thread_create(VCOS_THREAD_T *thread,
                                 const char *name,
                                 VCOS_THREAD_ATTR_T *attrs,
                                 VCOS_THREAD_ENTRY_FN_T entry,
                                 void *arg)
{
   VCOS_STATUS_T st;
   pthread_attr_t pt_attrs;
   VCOS_THREAD_ATTR_T *local_attrs = attrs ? attrs : &default_attrs;

   int rc = pthread_attr_init(&pt_attrs);
   if (rc < 0)
      return VCOS_ENOMEM;

   st = vcos_semaphore_create(&thread->suspend, NULL, 0);
   if (st != VCOS_SUCCESS)
   {
      pthread_attr_destroy(&pt_attrs);
      return st;
   }

   thread->task_timer.pfn = NULL; /* Not initialised */

   pthread_attr_setstacksize(&pt_attrs, local_attrs->ta_stacksz);
#if VCOS_CAN_SET_STACK_ADDR
   if (local_attrs->ta_stackaddr)
   {
      pthread_attr_setstackaddr(&pt_attrs, local_attrs->ta_stackaddr);
   }
#else
   vcos_demand(local_attrs->ta_stackaddr == 0);
#endif

   /* pthread_attr_setpriority(&pt_attrs, local_attrs->ta_priority); */
   
   vcos_assert(local_attrs->ta_stackaddr == 0); /* Not possible */

   thread->entry = entry;
   thread->arg = arg;
   thread->legacy = local_attrs->legacy;

   if (name)
   {
	   strncpy(thread->name, name, sizeof(thread->name)-1);
	   thread->name[sizeof(thread->name)-1] = '\0';
   }
   else
   {
	   thread->name[0] = '\0';
   }

   rc = pthread_create(&thread->thread, &pt_attrs, vcos_thread_entry, thread);

   pthread_attr_destroy(&pt_attrs);

   memset(thread->at_exit, 0, sizeof(thread->at_exit));

   if (rc < 0)
   {
      vcos_semaphore_delete(&thread->suspend);
      return VCOS_ENOMEM;
   }
   else
   {
      return VCOS_SUCCESS;
   }
}

void vcos_thread_join(VCOS_THREAD_T *thread,
                             void **pData)
{
   pthread_join(thread->thread, pData);
   vcos_semaphore_delete(&thread->suspend);

   /* Delete the task timer, if present */
   _vcos_task_timer_cancel();
   if (thread->task_timer.pfn)
   {
      vcos_timer_delete(&thread->task_timer);
      thread->task_timer.pfn = NULL;
   }
}

VCOSPRE_ VCOS_STATUS_T VCOSPOST_ vcos_thread_create_classic(VCOS_THREAD_T *thread,
                                                            const char *name,
                                                            void *(*entry)(void *arg),
                                                            void *arg,
                                                            void *stack,
                                                            VCOS_UNSIGNED stacksz,
                                                            VCOS_UNSIGNED priaff,
                                                            VCOS_UNSIGNED timeslice,
                                                            VCOS_UNSIGNED autostart)
{
   VCOS_THREAD_ATTR_T attrs;
   vcos_thread_attr_init(&attrs);
   vcos_thread_attr_setstacksize(&attrs, stacksz);
   vcos_thread_attr_setpriority(&attrs, priaff & ~_VCOS_AFFINITY_MASK);
   vcos_thread_attr_setaffinity(&attrs, priaff & _VCOS_AFFINITY_MASK);

   if (VCOS_CAN_SET_STACK_ADDR)
   {
      vcos_thread_attr_setstack(&attrs, stack, stacksz);
   }

   return vcos_thread_create(thread, name, &attrs, entry, arg);
}

uint32_t vcos_getmicrosecs( void )
{
   struct timeval tv;
   uint32_t tm = 0;;

   if (!gettimeofday(&tv, NULL))
   {
      tm =  (tv.tv_sec*1000000) + tv.tv_usec;
   }

   return tm;
}

VCOS_STATUS_T vcos_timer_init(void)
{
   return VCOS_SUCCESS;
}

void vcos_log_impl(const VCOS_LOG_CAT_T *cat, const char *fmt, ...)
{
   va_list ap;
   if (cat->flags.want_prefix)
      fprintf(stdout, "%s:", cat->name);
   va_start(ap,fmt);
   vfprintf(stdout, fmt, ap);
   fprintf(stdout, "\n");
   va_end(ap);
}

void vcos_vlog_impl(const VCOS_LOG_CAT_T *cat, const char *fmt, va_list args)
{
   if (cat->flags.want_prefix)
      fprintf(stdout, "%s:", cat->name);
   vfprintf(stdout, fmt, args);
   fprintf(stdout, "\n");
}

VCOS_STATUS_T vcos_init(void)
{
   VCOS_STATUS_T st = VCOS_SUCCESS;

#ifdef HAVE_MTRACE
   // enable glibc memory debugging, if the environment
   // variable MALLOC_TRACE names a valid file.
   mtrace();
#endif

   vcos_mutex_create(&lock, "global_lock");

   vcos_demand(pthread_key_create(&_vcos_thread_current_key, NULL) == 0);

   /* Initialise a VCOS wrapper for the thread which called vcos_init. */
   vcos_semaphore_create(&vcos_thread_main.suspend, NULL, 0);
   vcos_thread_main.thread = pthread_self();
   pthread_setspecific(_vcos_thread_current_key, &vcos_thread_main);

   st = vcos_msgq_init();
   vcos_assert(st == VCOS_SUCCESS);

   vcos_logging_init();

   return st;
}

void vcos_deinit(void)
{
}

void vcos_global_lock(void)
{
   vcos_mutex_lock(&lock);
}

void vcos_global_unlock(void)
{
   vcos_mutex_unlock(&lock);
}

void vcos_thread_exit(void *arg)
{
   pthread_exit(arg);
}


void vcos_thread_attr_init(VCOS_THREAD_ATTR_T *attrs)
{
   *attrs = default_attrs;
}

VCOS_STATUS_T vcos_pthreads_map_errno(void)
{
   switch (errno)
   {
   case ENOMEM:
      return VCOS_ENOMEM;
   case ENXIO:
      return VCOS_ENXIO;
   case EAGAIN:
      return VCOS_EAGAIN;
   case ENOSPC:
      return VCOS_ENOSPC;
   default:
      return VCOS_EINVAL;
   }
}

void _vcos_task_timer_set(void (*pfn)(void*), void *cxt, VCOS_UNSIGNED ms)
{
   VCOS_THREAD_T *thread = vcos_thread_current();
   VCOS_STATUS_T status;
   status = vcos_timer_create(&thread->task_timer, "vcos_task_timer", pfn, cxt);
   vcos_assert(status == VCOS_SUCCESS);
   vcos_timer_set(&thread->task_timer, ms);
}

void _vcos_task_timer_cancel(void)
{
   VCOS_THREAD_T *thread = vcos_thread_current();
   if (thread->task_timer.pfn)
   {
      thread->task_timer.pfn = NULL;
      vcos_timer_delete(&thread->task_timer);
   }
}

int vcos_snprintf(char *buf, size_t buflen, const char *fmt, ...)
{
   int ret;
   va_list ap;
   va_start(ap,fmt);
   ret = vsnprintf(buf, buflen, fmt, ap);
   va_end(ap);
   return ret;
}

int vcos_llthread_running(VCOS_THREAD_T *t) {
   vcos_assert(0);   // this function only exists as a nasty hack for the video codecs!
   return 1;
}

int vcos_have_rtos(void)
{
   return 1;
}

const char * vcos_thread_get_name(const VCOS_THREAD_T *thread)
{
   return thread->name;
}

void vcos_pthreads_logging_assert(const char *file, const char *func, unsigned int line, const char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   fprintf(stderr, "assertion failure:%s:%d:%s():",
           file, line, func);
   vfprintf(stderr, fmt, ap);
   va_end(ap);
   fprintf(stderr, "\n");
#ifdef VCOS_HAVE_BACKTRACE
   vcos_backtrace_self();
#endif
}

extern VCOS_STATUS_T vcos_thread_at_exit(void (*pfn)(void*), void *cxt)
{
   int i;
   VCOS_THREAD_T *self = vcos_thread_current();
   if (!self)
   {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
   for (i=0; i<VCOS_MAX_EXIT_HANDLERS; i++)
   {
      if (self->at_exit[i].pfn == NULL)
      {
         self->at_exit[i].pfn = pfn;
         self->at_exit[i].cxt = cxt;
         return VCOS_SUCCESS;
      }
   }
   return VCOS_ENOSPC;
}

void vcos_set_args(int argc, const char **argv)
{
   vcos_argc = argc;
   vcos_argv = argv;
}

int vcos_get_argc(void)
{
   return vcos_argc;
}

const char ** vcos_get_argv(void)
{
   return vcos_argv;
}

// we can't inline this, because HZ comes from sys/param.h which
// dumps all sorts of junk into the global namespace, notable MIN and
// MAX.
uint32_t _vcos_get_ticks_per_second(void)
{
   return HZ;
}


