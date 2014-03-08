/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
/*
   void callback_destroy_context(KHRN_POINTER_MAP_T *map, uint32_t key, void *value, void *data)

   Implementation notes:

   Passed as a callback to khrn_pointer_map_iterate.

   Preconditions:

   Thread owns EGL lock

   map is the CLIENT_PROCESS_STATE_T.contexts
   value is a pointer to a valid EGL_CONTEXT_T
   key is a key in map
   map[key] == value

   Postconditions:

   Does not alter map
   value is a dead pointer (i.e. either a pointer to a freed thing or something we don't hold a reference to)

   Invariants preserved:

   (EGL_CONTEXT_IS_DESTROYED)

   Invariants used:

   (CLIENT_PROCESS_STATE_CONTEXTS)
   (EGL_CONTEXT_IS_CURRENT)
*/

static void callback_destroy_context(KHRN_POINTER_MAP_T *map, uint32_t key, void *value, void *data)
{
   EGL_CONTEXT_T *context = (EGL_CONTEXT_T *)value;

   UNUSED(map);
   UNUSED(data);
   UNUSED_NDEBUG(key);

   vcos_assert( context != NULL );
   vcos_assert((uintptr_t)key == (uintptr_t)context->name);

   vcos_assert(!context->is_destroyed);

   context->is_destroyed = true;
   egl_context_maybe_free(context);
}

/*
   void callback_destroy_surface(KHRN_POINTER_MAP_T *map, uint32_t key, void *value, void *data)

   Implementation notes:

   Passed as a callback to khrn_pointer_map_iterate.

   Preconditions:

   Thread owns EGL lock

   map is the CLIENT_PROCESS_STATE_T.surfaces
   value is a pointer to a valid EGL_SURFACE_T
   key is a key in map
   map[key] == value

   Postconditions:

   Does not alter map
   value is a dead pointer (i.e. either a pointer to a freed thing or something we don't hold a reference to)

   Invariants preserved:

   (EGL_SURFACE_IS_DESTROYED)

   Invariants used:

   (CLIENT_PROCESS_STATE_SURFACES)
   (EGL_SURFACE_BINDING_COUNT)
*/

static void callback_destroy_surface(KHRN_POINTER_MAP_T *map, uint32_t key, void *value, void *data)
{
   EGL_SURFACE_T *surface = (EGL_SURFACE_T *)value;

   UNUSED(map);
   UNUSED(data);
   UNUSED_NDEBUG(key);

   vcos_assert( surface != NULL );
   vcos_assert((uintptr_t)key == (uintptr_t)surface->name);

   surface->is_destroyed = true;
   egl_surface_maybe_free(surface);
}

/*
   void callback_destroy_sync(KHRN_POINTER_MAP_T *map, uint32_t key, void *value, void *data)

   Implementation notes:

   Passed as a callback to khrn_pointer_map_iterate.

   Preconditions:

   Thread owns EGL lock

   value is a pointer to a valid EGL_SYNC_T

   Postconditions:

   Does not alter map
   value is a dead pointer (i.e. a pointer to a freed thing)

   Invariants preserved:

   -

   Invariants used:

   -
*/

#if EGL_KHR_sync
static void callback_destroy_sync(KHRN_POINTER_MAP_T *map, uint32_t key, void *value, void *data)
{
   EGL_SYNC_T *sync = (EGL_SYNC_T *)value;

   UNUSED(map);
   UNUSED(data);
   UNUSED(key);

   vcos_assert( sync != NULL );

   egl_sync_term(sync);
   khrn_platform_free(sync);
}
#endif

/*
   CLIENT_PROCESS_STATE_T *client_egl_get_process_state(CLIENT_THREAD_STATE_T *thread, EGLDisplay dpy, EGLBoolean check_inited)

   Returns the process-global CLIENT_PROCESS_STATE_T object. If check_inited is true, also insists that the process state
   is inited.

   Implementation notes:

   -

   Preconditions:

   thread is a valid pointer
   Thread owns EGL lock

   Postconditions:

   The following conditions cause error to assume the specified value

      EGL_BAD_DISPLAY               An EGLDisplay argument does not name a valid EGLDisplay
      EGL_NOT_INITIALIZED           check_inited is true and EGL is not initialized for the specified display.

   if more than one condition holds, the first error is generated.

   If error, NULL is returned. Otherwise a pointer is returned which is valid for the lifetime of the process.

   Invariants preserved:

   -

   Invariants used:

   -
*/

CLIENT_PROCESS_STATE_T *client_egl_get_process_state(CLIENT_THREAD_STATE_T *thread, EGLDisplay dpy, EGLBoolean check_inited)
{
   if ((size_t)dpy == 1) {
      CLIENT_PROCESS_STATE_T *process = CLIENT_GET_PROCESS_STATE();

      if (check_inited && !process->inited) {
         thread->error = EGL_NOT_INITIALIZED;
         return NULL;
      } else
         return process;
   } else {
      thread->error = EGL_BAD_DISPLAY;
      return NULL;
   }
}
