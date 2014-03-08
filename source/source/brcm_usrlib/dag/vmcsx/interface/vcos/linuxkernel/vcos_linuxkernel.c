/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  vcos
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - pthreads types
=============================================================================*/

#define  VCOS_INLINE_BODIES
#include "interface/vcos/vcos.h"

#ifndef VCOS_DEFAULT_STACK_SIZE
#define VCOS_DEFAULT_STACK_SIZE 4096
#endif

static VCOS_THREAD_ATTR_T default_attrs = {
   0,
   VCOS_DEFAULT_STACK_SIZE,
};

DECLARE_MUTEX(lock);

VCOS_STATUS_T vcos_thread_create(VCOS_THREAD_T *thread,
                                 const char *name,
                                 VCOS_THREAD_ATTR_T *attrs,
                                 VCOS_THREAD_ENTRY_FN_T entry,
                                 void *arg)
{
   *thread = kthread_run((int (*)(void *))entry, arg, name);
   vcos_assert(*thread != NULL);
   return VCOS_SUCCESS;
}

void vcos_thread_join(VCOS_THREAD_T *thread,
                             void **pData)
{
   void *ret = (void *)kthread_stop(*thread);
   if (pData)
      *pData = ret;
}

uint32_t vcos_getmicrosecs( void )
{
   struct timeval tv;
   do_gettimeofday(&tv);
   return (tv.tv_sec*1000000) + tv.tv_usec;
}

void vcos_timer_init(void)
{
}

void vcos_log_impl(const VCOS_LOG_CAT_T *cat, const char *fmt, ...)
{
   va_list ap;
   if (cat->flags.want_prefix)
      printk("%s:", cat->name);

   va_start(ap,fmt);
   vprintk(fmt, ap);
   va_end(ap);
   printk("\n");
}

void vcos_vlog_impl(const VCOS_LOG_CAT_T *cat, const char *fmt, va_list args)
{
   if (cat->flags.want_prefix)
      printk("%s:", cat->name);
   vprintk(fmt, args);
   printk("\n");
}

VCOS_STATUS_T vcos_init(void)
{
   return VCOS_SUCCESS;
}

void vcos_deinit(void)
{
}

void vcos_global_lock(void)
{
   down(&lock);
}

void vcos_global_unlock(void)
{
   up(&lock);
}

void vcos_thread_exit(void *arg)
{
   /* This isn't possible */
   vcos_assert(0);
}


void vcos_thread_attr_init(VCOS_THREAD_ATTR_T *attrs)
{
   *attrs = default_attrs;
}

void _vcos_task_timer_set(void (*pfn)(void*), void *cxt, VCOS_UNSIGNED ms)
{
   vcos_assert(0); /* no timers on event groups yet */
}

void _vcos_task_timer_cancel(void)
{
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

