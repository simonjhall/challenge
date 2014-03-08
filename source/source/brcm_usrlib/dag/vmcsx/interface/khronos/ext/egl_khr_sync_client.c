/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#define EGL_EGLEXT_PROTOTYPES /* we want the prototypes so the compiler will check that the signatures match */
#include "interface/khronos/ext/egl_khr_sync_client.h"
#include "interface/khronos/include/EGL/egl.h"
#include "interface/khronos/include/EGL/eglext.h"
#if defined(V3D_LEAN)
#include "interface/khronos/common/khrn_int_misc_impl.h"
#endif

static EGL_SYNC_T *egl_sync_create(EGLSyncKHR name, EGLint condition, EGLint threshold)
{
   EGL_SYNC_T *sync = (EGL_SYNC_T *)khrn_platform_malloc(sizeof(EGL_SYNC_T), "EGL_SYNC_T");
   uint64_t pid = khronos_platform_get_process_id();
   uint32_t token;

   if (!sync)
      return 0;

   sync->condition = condition;
   sync->threshold = threshold;

   sync->sem[0] = (int)pid;
   sync->sem[1] = (int)(pid >> 32);
   sync->sem[2] = (int)name;

   if (khronos_platform_semaphore_create(&sync->master, sync->sem, 0) != KHR_SUCCESS) {
      khrn_platform_free(sync);
      return 0;
   }

#ifdef __SYMBIAN32__
   token = (uint32_t)sync->master;
#else
   token = (uint32_t)name;
#endif
   sync->serversync = RPC_UINT_RES(RPC_CALL3_RES(eglIntCreateSync_impl,
                                                 EGLINTCREATESYNC_ID,
                                                 RPC_UINT(condition),
                                                 RPC_INT(threshold),
                                                 RPC_UINT(token)));

   if (sync->serversync)
      return sync;
   else {
      khronos_platform_semaphore_destroy(&sync->master);
      khrn_platform_free(sync);
      return 0;
   }
}

/*
   void egl_sync_term(EGL_SYNC_T *sync)

   Implementation notes:

   -

   Preconditions:

   sync is a valid pointer

   Postconditions:

   -

   Invariants preserved:

   -

   Invariants used:

   -
 */

void egl_sync_term(EGL_SYNC_T *sync)
{
   RPC_CALL1(eglIntDestroySync_impl,
             EGLINTDESTROYSYNC_ID,
             RPC_UINT(sync->serversync));

#ifdef __SYMBIAN32__ 
   RPC_FLUSH(); // now videocore requests semaphore deletion
#else
   khronos_platform_semaphore_destroy(&sync->master); 
#endif
}

static EGLBoolean egl_sync_check_attribs(const EGLint *attrib_list, EGLint *condition, EGLint *threshold)
{
   if (!attrib_list)
      return EGL_FALSE;

   *condition = EGL_NONE;
   *threshold = 0;

   while (1) {
      int name = *attrib_list++;
      if (name == EGL_NONE)
         break;
      else {
         /* int value = * */attrib_list++; /* at present no name/value pairs are handled */
         switch (name) {
         default:
            return EGL_FALSE;
         }
      }
   }

   return *condition != EGL_NONE && *threshold >= 0;
}

EGLBoolean egl_sync_get_attrib(EGL_SYNC_T *sync, EGLint attrib, EGLint *value)
{
   switch (attrib) {
   case EGL_SYNC_TYPE_KHR:
      *value = 0;
      return EGL_TRUE;
   case EGL_SYNC_STATUS_KHR:
      *value = 0;
      return EGL_TRUE;
   case EGL_SYNC_CONDITION_KHR:
      *value = sync->condition;
      return EGL_TRUE;
   default:
      return EGL_FALSE;
   }
}

EGLAPI EGLSyncKHR EGLAPIENTRY eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();
   EGLSyncKHR result = EGL_NO_SYNC_KHR;

   UNUSED(type);

   CLIENT_LOCK();

   {
      CLIENT_PROCESS_STATE_T *process = client_egl_get_process_state(thread, dpy, EGL_TRUE);

      EGLint condition;
      EGLint threshold;

      if (process)
      {
         if (1)
            thread->error = EGL_BAD_ATTRIBUTE;
         else if (egl_sync_check_attribs(attrib_list, &condition, &threshold)) {
            EGL_SYNC_T *sync = egl_sync_create((EGLSyncKHR)(size_t)process->next_sync, condition, threshold);

            if (sync) {
               if (khrn_pointer_map_insert(&process->syncs, process->next_sync, sync)) {
                  thread->error = EGL_SUCCESS;
                  result = (EGLSurface)(size_t)process->next_sync++;
               } else {
                  thread->error = EGL_BAD_ALLOC;
                  egl_sync_term(sync);
                  khrn_platform_free(sync);
               }
            } else
               thread->error = EGL_BAD_ALLOC;
         }
      }
   }

   CLIENT_UNLOCK();

   return result;
}

// TODO: should we make sure any syncs have come back before destroying the object?

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR _sync)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();
   EGLBoolean result;

   CLIENT_LOCK();

   {
      CLIENT_PROCESS_STATE_T *process = client_egl_get_process_state(thread, dpy, EGL_TRUE);

      if (process) {
         EGL_SYNC_T *sync = (EGL_SYNC_T *)khrn_pointer_map_lookup(&process->syncs, (uint32_t)(size_t)_sync);

         if (sync) {
            thread->error = EGL_SUCCESS;

            khrn_pointer_map_delete(&process->syncs, (uint32_t)(uintptr_t)_sync);

            egl_sync_term(sync);
            khrn_platform_free(sync);
         } else
            thread->error = EGL_BAD_PARAMETER;

         result = (thread->error == EGL_SUCCESS ? EGL_TRUE : EGL_FALSE);
      } else {
         result = EGL_FALSE;
      }
   }

   CLIENT_UNLOCK();

   return result;
}

EGLAPI EGLint EGLAPIENTRY eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR _sync, EGLint flags, EGLTimeKHR timeout)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   UNUSED(timeout);

   CLIENT_LOCK();

   {
      CLIENT_PROCESS_STATE_T *process = client_egl_get_process_state(thread, dpy, EGL_TRUE);

      if (process) {
         EGL_SYNC_T *sync = (EGL_SYNC_T *)khrn_pointer_map_lookup(&process->syncs, (uint32_t)(size_t)_sync);

         if (sync) {
#ifdef __SYMBIAN32__
//At present store semaphore Id in name so no need to call create again (not available in this platform)
            if(1) {
#else
            PLATFORM_SEMAPHORE_T semaphore;
            if( khronos_platform_semaphore_create(&semaphore, sync->sem, 1) == KHR_SUCCESS) {
#endif
               if (flags & EGL_SYNC_FLUSH_COMMANDS_BIT_KHR)
                  RPC_FLUSH();

               CLIENT_UNLOCK();

#ifdef __SYMBIAN32__
               khronos_platform_semaphore_acquire_and_release(&sync->master);
#else
               khronos_platform_semaphore_acquire(&semaphore);
               khronos_platform_semaphore_release(&semaphore);
               khronos_platform_semaphore_destroy(&semaphore);
#endif

               return EGL_CONDITION_SATISFIED_KHR;
            } else
               thread->error = EGL_BAD_ALLOC;         // not strictly allowed by the spec, but indicates that we failed to create a reference to the named semaphore
         } else
            thread->error = EGL_BAD_PARAMETER;
      }
   }

   CLIENT_UNLOCK();

   return EGL_FALSE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSignalSyncKHR(EGLDisplay dpy, EGLSyncKHR _sync, EGLenum mode)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   UNUSED(mode);

   CLIENT_LOCK();

   {
      CLIENT_PROCESS_STATE_T *process = client_egl_get_process_state(thread, dpy, EGL_TRUE);

      if (process) {
         EGL_SYNC_T *sync = (EGL_SYNC_T *)khrn_pointer_map_lookup(&process->syncs, (uint32_t)(size_t)_sync);

         if (sync)
            thread->error = EGL_BAD_MATCH;
         else
            thread->error = EGL_BAD_PARAMETER;
      }
   }

   CLIENT_UNLOCK();

   return EGL_FALSE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetSyncAttribKHR(EGLDisplay dpy, EGLSyncKHR _sync, EGLint attribute, EGLint *value)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();
   EGLBoolean result = EGL_FALSE;

   CLIENT_LOCK();

   {
      CLIENT_PROCESS_STATE_T *process = client_egl_get_process_state(thread, dpy, EGL_TRUE);

      if (process)
      {
         if (value)
         {
            EGL_SYNC_T *sync = (EGL_SYNC_T *)khrn_pointer_map_lookup(&process->syncs, (uint32_t)(size_t)_sync);

            if (sync) {
               if (egl_sync_get_attrib(sync, attribute, value)) {
                  thread->error = EGL_SUCCESS;
                  result = EGL_TRUE;
               } else
                  thread->error = EGL_BAD_ATTRIBUTE;
            } else
               thread->error = EGL_BAD_PARAMETER;
         }
         else
         {
            thread->error = EGL_BAD_PARAMETER;
         }
      }
   }

   CLIENT_UNLOCK();

   return result;
}
