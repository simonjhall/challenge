/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  vcos
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - win32 support
=============================================================================*/

#define VCOS_INLINE_BODIES

#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos_msgqueue.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

DWORD vcos_thread_data_tls;
int vcos_thread_priorities[(VCOS_THREAD_PRI_MAX - VCOS_THREAD_PRI_MIN) + 1] = 
{
   THREAD_PRIORITY_IDLE,
   THREAD_PRIORITY_LOWEST,
   THREAD_PRIORITY_BELOW_NORMAL,
   THREAD_PRIORITY_NORMAL,
   THREAD_PRIORITY_ABOVE_NORMAL,
   THREAD_PRIORITY_HIGHEST,
   THREAD_PRIORITY_TIME_CRITICAL
};

static int inited;
static LARGE_INTEGER ticks_per_second;
static HANDLE timer_queue;
static CRITICAL_SECTION lock;

static DWORD WINAPI wrapper(void *arg)
{
   VCOS_LLTHREAD_T *thread = arg;

   // If this fires, it probably means you allocated the thread structure
   // on your stack.
   vcos_assert(thread->magic == _VCOS_WIN32_THREAD_MAGIC);

   TlsSetValue(vcos_thread_data_tls, thread);
   thread->entry(arg);
   return 0;
}

VCOS_STATUS_T vcos_llthread_create(VCOS_LLTHREAD_T *thread,
                                   const char *name,
                                   VCOS_LLTHREAD_ENTRY_FN_T entry,
                                   void *arg,
                                   void *stack,
                                   VCOS_UNSIGNED stacksz,
                                   VCOS_UNSIGNED priority,
                                   VCOS_UNSIGNED affinity,
                                   VCOS_UNSIGNED timeslice,
                                   VCOS_UNSIGNED autostart)
{
   HANDLE h;
   vcos_assert(inited);
   vcos_assert(stack == 0); /* We can't set the stack on this platform */
   vcos_assert((priority >= VCOS_THREAD_PRI_MIN) && (priority <= VCOS_THREAD_PRI_MAX));

   thread->entry = entry;
   thread->arg = arg;
   thread->magic = _VCOS_WIN32_THREAD_MAGIC;

   h = CreateThread(NULL, stacksz, wrapper, thread, CREATE_SUSPENDED, NULL);
   if (!h)
   {
      return VCOS_ENOMEM;
   }
   thread->thread = h;
   if (!SetThreadPriority(h, vcos_thread_priorities[priority - VCOS_THREAD_PRI_MIN]))
   {
      int err = GetLastError();
      vcos_assert(err == 0);
   }

   if (autostart)
   {
      ResumeThread(h);
   }
   return VCOS_SUCCESS;
}

void vcos_llthread_exit(void)
{
   ExitThread(0);
}

void vcos_llthread_delete(VCOS_LLTHREAD_T *thread)
{
   CloseHandle(thread->thread);
}

VCOS_STATUS_T _vcos_llthread_create_attach(VCOS_LLTHREAD_T *thread)
{
   thread->magic = _VCOS_WIN32_THREAD_MAGIC;
   if (!DuplicateHandle(
      GetCurrentProcess(),
      GetCurrentThread(),
      GetCurrentProcess(),
      &thread->thread,
      0, FALSE, DUPLICATE_SAME_ACCESS)) {
      return VCOS_ENOMEM;
   }
   TlsSetValue(vcos_thread_data_tls, thread);
   return VCOS_SUCCESS;
}

VCOS_STATUS_T vcos_win32_map_error(void)
{
   DWORD rc = GetLastError();
   printf("Error: rc = %ld\n", rc);
   switch (rc)
   {
   case ERROR_INVALID_PARAMETER:
      return VCOS_EINVAL;
   default:
      return VCOS_ENOMEM;
   }
}

void vcos_log_impl(const VCOS_LOG_CAT_T *cat, const char *fmt, ...)
{
   va_list ap;
   if (cat->flags.want_prefix)
      printf("%s:", cat->name);
   va_start(ap,fmt);
   vfprintf(stdout, fmt, ap);
   fprintf(stdout,"\n");
   fflush(stdout);
   va_end(ap);
}

void vcos_vlog_impl(const VCOS_LOG_CAT_T *cat, const char *fmt, va_list args)
{
   if (cat->flags.want_prefix)
      printf("%s:", cat->name);
   vfprintf(stdout, fmt, args);
   fprintf(stdout,"\n");
   fflush(stdout);
}

void vcos_win32_report_error(void)
{
   vcos_log("vcos: error %d", GetLastError());
}

static VCOS_THREAD_T main_thread;

VCOS_STATUS_T vcos_init(void)
{
   if(!inited)
   {
      BOOL ok;
      VCOS_STATUS_T status;

      ok = QueryPerformanceFrequency(&ticks_per_second);
      vcos_assert(ok);
      if (!ok)
         return VCOS_ENOSYS;

      vcos_thread_data_tls = TlsAlloc();
      if (vcos_thread_data_tls == (DWORD)-1)
         return VCOS_ENOMEM;

      status = _vcos_thread_create_attach(&main_thread, "main");
      if (status != VCOS_SUCCESS) {
         TlsFree(vcos_thread_data_tls);
         return status;
      }

      vcos_msgq_init();

      InitializeCriticalSection(&lock);

      vcos_logging_init();

      inited = 1;
   }
   return VCOS_SUCCESS;
}

VCOS_STATUS_T vcos_timer_init(void)
{
   if (timer_queue != NULL)
   {
      vcos_assert(0);
      return VCOS_EEXIST;
   }

   timer_queue = CreateTimerQueue();
   if (!timer_queue)
      return VCOS_ENOMEM;

   return VCOS_SUCCESS;
}

void vcos_deinit(void)
{
   vcos_assert(inited);
   _vcos_thread_delete(&main_thread);
   TlsFree(vcos_thread_data_tls);
   DeleteTimerQueueEx(timer_queue,INVALID_HANDLE_VALUE);
   inited = 0;
}


void vcos_global_lock(void)
{
   EnterCriticalSection(&lock);
}


void vcos_global_unlock(void)
{
   LeaveCriticalSection(&lock);
}

uint32_t vcos_win32_getmicrosecs(void)
{
   LARGE_INTEGER tick;
   QueryPerformanceCounter(&tick);
   return (uint32_t)((tick.QuadPart*1000*1000)/ticks_per_second.QuadPart);
}

void *vcos_get_thread_data_tls(void)
{
   return TlsGetValue(vcos_thread_data_tls);
}

HANDLE _vcos_timer_queue(void)
{
   return timer_queue;
}

void CALLBACK vcos_win32_timer_wrapper(void *cxt, BOOLEAN wait_or_timer)
{
   VCOS_TIMER_T *timer = cxt;
   vcos_assert(cxt);
   timer->pfn(timer->context);
}

void _vcos_assert(const char *file, int line, const char *fmt, ...)
{
   char desc[512];
   va_list ap;
   va_start(ap,fmt);
   vsprintf(desc, fmt, ap);
   desc[sizeof(desc) - 1] = '\0'; /* The message may be too long, but we're aborting anyway */
   vcos_log("assertion failed: %s:%d: %s", file, line, desc);
   va_end(ap);
#if defined(_MSC_VER)                     // Visual C define equivalent types
   __asm { int 3 }
#endif //#if defined(_MSC_VER)                     // Visual C define equivalent types
   abort();
}

VCOS_STATUS_T
vcos_win32_named_semaphore_create(VCOS_NAMED_SEMAPHORE_T *sem, const char *name, VCOS_UNSIGNED count)
{
   char buf[64];
   HANDLE h;
   int ret = vcos_snprintf(buf, sizeof(buf), "Global\\%s", name);
   if (ret < 0)
   {
      vcos_assert(0);
      return VCOS_ENOSPC;
   }

   h = CreateSemaphoreA(NULL, count, 1<<16, buf);
   if (h != NULL)
   {
      sem->sem = h;
      return VCOS_SUCCESS;
   }
   else
      return vcos_win32_map_error();
}


int vcos_snprintf(char *buf, size_t buflen, const char *fmt, ...)
{
   int ret;
   va_list ap;
   va_start(ap,fmt);
#ifdef __GNUC__
   ret = vsnprintf(buf, buflen, fmt, ap);
#else
   ret = _vsnprintf_s(buf, buflen, _TRUNCATE, fmt, ap);
#endif
   va_end(ap);
   return ret;
}

int vcos_llthread_running(VCOS_LLTHREAD_T *t) {
   vcos_assert(0);   // this function only exists as a nasty hack for the video codecs!
   return 1;
}

int vcos_have_rtos(void)
{
   return 1;
}


