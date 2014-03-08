/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  vcos
File     :  $RCSfile: $
Revision :  $Revision: #1 $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - Symbian support
=============================================================================*/

#ifdef __KERNEL_MODE__
#include <kernel/kernel.h>
#else
#include <e32std.h>
#endif

// Outside kernel mode, only support vcos_assert at the moment.

#ifdef __KERNEL_MODE__
static void my_sprintf(TDes &aBuf, const char *aFormat, ...)
{
    VA_LIST list;
    VA_START(list, aFormat);
    Kern::AppendFormat(aBuf, aFormat, list);
}
#endif

extern "C" void _vcos_assert(const char *file, const char *func, int line, const char *format, ...)
    {
    TBuf<256> printBuf;
    _LIT(KFailed, "VC: Assertion failed: ");
    printBuf.Append(KFailed);
    VA_LIST list;
    VA_START(list, format);
#ifdef __KERNEL_MODE__
    Kern::AppendFormat(printBuf, format, list);
#else
#ifdef _UNICODE
    TBuf<256> fmt;
    fmt.Copy(TPtrC8((const TUint8 *)format));
#else
    TPtrC fmt((const TUint8 *) format);
#endif
    printBuf.AppendFormatList(fmt, list);
#endif
    VA_END(list);
#ifdef __KERNEL_MODE__
    my_sprintf(printBuf, ", function %s, file %s, line %d", func, file, line);
#else
    printBuf.AppendFormat(_L(", function %s, file %s, line %d"), func, file, line);
#endif

#ifdef __KERNEL_MODE__
    printBuf.Append(TChar(0));
    Kern::Fault(reinterpret_cast<const char *>(printBuf.Ptr()), KErrGeneral);
#else
    User::Panic(printBuf, KErrGeneral);
#endif
    }

extern "C" void vcos_abort(void)
	{
#ifdef __KERNEL_MODE__
	Kern::Fault("vcos_abort", KErrGeneral);
#else
	User::Panic(_L("vcos_abort"), KErrGeneral);
#endif
	}

#ifdef __KERNEL_MODE__

#define VCOS_INLINE_BODIES

#include "interface/vcos/vcos.h"
//#include <string.h>

extern "C"
{

#include "../generic/vcos_joinable_thread_from_plain.c"
#include "../generic/vcos_generic_event_flags.c"

static struct
{
   VCOS_LLTHREAD_T *head;
   VCOS_LLTHREAD_T *tail;
   NFastMutex lock;
} threads;

VCOS_STATUS_T vcos_init(void)
{
   return VCOS_SUCCESS;
}

void vcos_global_lock(void)
{
   NKern::ThreadEnterCS();
}

void vcos_global_unlock(void)
{
   NKern::ThreadLeaveCS();
}

VCOS_STATUS_T vcos_symbian_map_error(TInt ret)
{
   switch (ret)
   {
   case KErrNone:
      return VCOS_SUCCESS;
   case KErrBadDescriptor:
      return VCOS_EINVAL;
   case KErrAccessDenied:
      return VCOS_EACCESS;
   case KErrAlreadyExists:
      return VCOS_EEXIST;
   case KErrNoMemory:
   default:
      return VCOS_ENOMEM;
   }
}

static TInt forward_to_thread_func(TAny *v)
{
   VCOS_LLTHREAD_T *thread = static_cast<VCOS_LLTHREAD_T *>(v);
   thread->entry(thread->arg);
   return KErrNone;
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
   SThreadCreateInfo create;
   TInt success;
   vcos_assert(VCOS_THREAD_PRI_MIN == 0 || priority >= VCOS_THREAD_PRI_MIN);
   vcos_assert(priority <= VCOS_THREAD_PRI_MAX);
   vcos_assert((stacksz & 3) == 0);
   vcos_assert(timeslice < 256);

   NKern::ThreadEnterCS();
   
   thread->entry = entry;
   thread->arg = arg;
   
   // We have to forward this on because thread function required by Symbian
   // requires a different number of parameters to the function being passed in.
   create.iFunction = forward_to_thread_func;
   create.iPtr = thread;
   create.iType = EThreadSupervisor;
   create.iSupervisorStackSize=stacksz;
   create.iInitialThreadPriority=12;  // = Normal, Foreground - I think
   // Don't seem to have strlen?
   TInt name_len=0;
   const char *p = name;
   while (*p++)
       name_len++;
   create.iName.Set((const TUint8*)name, name_len);
   create.iTotalSize = sizeof create;
   
   success = Kern::ThreadCreate(create);
   NKern::ThreadLeaveCS();
   if (success == 0) {
      thread->thread = static_cast<DThread*>(create.iHandle);
      Kern::ThreadResume(*thread->thread);
      NKern::FMWait(&threads.lock);
      VCOS_QUEUE_APPEND_TAIL(&threads, thread);
      NKern::FMSignal(&threads.lock);
   }

   return vcos_symbian_map_error(success);
}

VCOS_STATUS_T _vcos_llthread_create_attach(VCOS_LLTHREAD_T *thread)
{
   thread->thread = &Kern::CurrentThread();
   thread->entry = 0;
   thread->arg = 0;
   return VCOS_SUCCESS;
}

void vcos_llthread_exit(void)
{
   Kern::Exit(KErrNone);    
}

VCOS_LLTHREAD_T *_vcos_llthread_current(void)
{
   DThread &thread = Kern::CurrentThread();
   VCOS_LLTHREAD_T *t;
   NKern::FMWait(&threads.lock);
   for (t = threads.head; t; t = t->next)
      if (t->thread == &thread)
          break;
   NKern::FMSignal(&threads.lock);
   return t;
}

void vcos_llthread_delete(VCOS_LLTHREAD_T *thread)
{
   VCOS_LLTHREAD_T **plist;
   NKern::ThreadEnterCS();
   Kern::SafeClose((DObject*&)thread->thread, NULL);
   
   /* walk the list of threads and remove it
    */
   NKern::FMWait(&threads.lock);
   plist = &threads.head;
   while (*plist != NULL)
   {
      if (*plist == thread)
      {
         int at_end;
         /* found it */
         at_end = ((*plist)->next == NULL);

         /* link past */
         *plist = (*plist)->next;
         if (at_end)
             threads.tail = threads.head;

         break;
      }
      plist = &(*plist)->next;
   }
   NKern::FMSignal(&threads.lock);
   NKern::ThreadLeaveCS();
}

char *strncpy(char *dst, const char *src, size_t n)
{
   char *dp = dst;
   for (; n && *src != '\0'; n--)
      *dp++ = *src++;
   for (; n; n--)
      *dp++ = '\0';
   return dst;
}

int vcos_llthread_running(VCOS_LLTHREAD_T *t) {
   vcos_assert(0);   // this function only exists as a nasty hack for the video codecs!
   return 1;
}

}

#endif // __KERNEL_MODE__

