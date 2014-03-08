/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
/******************************************************************************
state
******************************************************************************/

/*
   void vg_server_state_set_error(VG_SERVER_STATE_T *state, VGErrorCode error)

   Sets the error, if it isn't set already.

   Preconditions:

   state points to a valid VG_SERVER_STATE_T
   error is a valid VGErrorCode

   Postconditions:

   -

   Invariants preserved:

   (VG_SERVER_ERROR)

   Invariants used:

   -
*/

void vg_server_state_set_error(VG_SERVER_STATE_T *state, VGErrorCode error)
{
   vcos_verify(0);

   if (state->error == VG_NO_ERROR) {
      state->error = error;
   }
}

/* this function exists just to save some typing */
static INLINE void set_error(VG_SERVER_STATE_T *state, VGErrorCode error) { vg_server_state_set_error(state, error); }

/******************************************************************************
helpers
******************************************************************************/

/*
   bool insert_object(VG_SERVER_STATE_T *state, MEM_HANDLE_T handle)

   Inserts handle into state->shared_state->objects. A return value of false
   indicates failure

   Preconditions:

   state points to a valid VG_SERVER_STATE_T
   handle refers to a valid VG object (see (VG_SERVER_SHARED_STATE_OBJECTS) for
      a list of valid object types)

   Postconditions:

   If the functions succeeds:
   - true is returned
   - handle is now in state->shared_state->objects
   otherwise:
   - false is returned
   - state->shared_state->objects is unchanged

   Invariants preserved:

   (VG_SERVER_SHARED_STATE_OBJECTS)
   (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED_OBJECTS_STORAGE)

   Invariants used:

   (VG_SERVER_SHARED_STATE_VALID)
*/

static bool insert_object(
   VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle)
{
   bool success;
   VG_SERVER_SHARED_STATE_T *shared_state = VG_LOCK_SERVER_SHARED_STATE(state);
   VG_FORCE_UNLOCK_SERVER_SHARED_STATE_OBJECTS_STORAGE(shared_state);
   success = vg_set_insert(&shared_state->objects, handle);
   VG_UNLOCK_SERVER_SHARED_STATE(state);
   return success;
}

/*
   void delete_object(VG_SERVER_STATE_T *state, MEM_HANDLE_T handle)

   Removes handle from state->shared_state->objects

   Preconditions:

   state points to a valid VG_SERVER_STATE_T
   handle is in state->shared_state->objects

   Postconditions:

   handle is no longer in state->shared_state->objects

   Invariants preserved:

   (VG_SERVER_SHARED_STATE_OBJECTS)
   (EGL_SERVER_STATE_LOCKED_VGCONTEXT_SHARED_OBJECTS_STORAGE)

   Invariants used:

   (VG_SERVER_SHARED_STATE_VALID)
*/

static void delete_object(
   VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle)
{
   VG_SERVER_SHARED_STATE_T *shared_state = VG_LOCK_SERVER_SHARED_STATE(state);
   VG_FORCE_UNLOCK_SERVER_SHARED_STATE_OBJECTS_STORAGE(shared_state);
   vg_set_delete(&shared_state->objects, handle);
   VG_UNLOCK_SERVER_SHARED_STATE(state);
}

/*
   bool contains_object(VG_SERVER_STATE_T *state, MEM_HANDLE_T handle)

   Returns true iff handle is in state->shared_state->objects

   Preconditions:

   state points to a valid VG_SERVER_STATE_T

   Postconditions:

   If true is returned:
      handle is in state->shared_state->objects (and hence it refers to valid VG object)
   Else:
      handle is not in state->shared_state->objects

   Invariants used:

   (VG_SERVER_SHARED_STATE_VALID)
   (VG_SERVER_SHARED_STATE_OBJECTS)
*/

static bool contains_object(
   VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle)
{
   VG_SERVER_SHARED_STATE_T *shared_state = VG_LOCK_SERVER_SHARED_STATE(state);
   bool contains = vg_set_contains_locked(&shared_state->objects, handle,
      VG_LOCK_SERVER_SHARED_STATE_OBJECTS_STORAGE(shared_state));
   VG_UNLOCK_SERVER_SHARED_STATE_OBJECTS_STORAGE(shared_state);
   VG_UNLOCK_SERVER_SHARED_STATE(state);
   return contains;
}

typedef enum {
   OBJECT_TYPE_STEM               = 1 << 0,
   OBJECT_TYPE_FONT_BPRINT        = 1 << 1,
   OBJECT_TYPE_IMAGE_BPRINT       = 1 << 2,
   OBJECT_TYPE_CHILD_IMAGE_BPRINT = 1 << 3,
   OBJECT_TYPE_MASK_LAYER_BPRINT  = 1 << 4,
   OBJECT_TYPE_PAINT_BPRINT       = 1 << 5,
   OBJECT_TYPE_PATH_BPRINT        = 1 << 6,
   OBJECT_TYPE_BPRINT             = 0x7e, /* all blueprint types */
   OBJECT_TYPE_FROM_BPRINT_SHIFT  = 6,
   OBJECT_TYPE_FONT               = 1 << 7,
   OBJECT_TYPE_IMAGE              = 1 << 8,
   OBJECT_TYPE_CHILD_IMAGE        = 1 << 9,
   OBJECT_TYPE_MASK_LAYER         = 1 << 10,
   OBJECT_TYPE_PAINT              = 1 << 11,
   OBJECT_TYPE_PATH               = 1 << 12,
   OBJECT_TYPE_ALL                = 0x1f80, /* all types except stem/blueprint */

   OBJECT_TYPE_STEM_BI               = 0,
   OBJECT_TYPE_FONT_BPRINT_BI        = 1,
   OBJECT_TYPE_IMAGE_BPRINT_BI       = 2,
   OBJECT_TYPE_CHILD_IMAGE_BPRINT_BI = 3,
   OBJECT_TYPE_MASK_LAYER_BPRINT_BI  = 4,
   OBJECT_TYPE_PAINT_BPRINT_BI       = 5,
   OBJECT_TYPE_PATH_BPRINT_BI        = 6,
   OBJECT_TYPE_FONT_BI               = 7,
   OBJECT_TYPE_IMAGE_BI              = 8,
   OBJECT_TYPE_CHILD_IMAGE_BI        = 9,
   OBJECT_TYPE_MASK_LAYER_BI         = 10,
   OBJECT_TYPE_PAINT_BI              = 11,
   OBJECT_TYPE_PATH_BI               = 12
} OBJECT_TYPE_T;

/*
   OBJECT_TYPE_T get_object_type(MEM_HANDLE_T handle)

   Returns the type of the object referred to by handle

   Preconditions:

   handle is a valid handle to a VG object (see (VG_SERVER_SHARED_STATE_OBJECTS)
      for a list of valid object types)

   Postconditions:

   returns one of {OBJECT_TYPE_STEM, OBJECT_TYPE_FONT_BPRINT,
      OBJECT_TYPE_IMAGE_BPRINT, OBJECT_TYPE_CHILD_IMAGE_BPRINT,
      OBJECT_TYPE_MASK_LAYER_BPRINT, OBJECT_TYPE_PAINT_BPRINT,
      OBJECT_TYPE_PATH_BPRINT, OBJECT_TYPE_FONT, OBJECT_TYPE_IMAGE,
      OBJECT_TYPE_CHILD_IMAGE, OBJECT_TYPE_MASK_LAYER, OBJECT_TYPE_PAINT,
      OBJECT_TYPE_PATH}
   It's actually the correct type.
*/

static OBJECT_TYPE_T get_object_type(MEM_HANDLE_T handle)
{
   MEM_TERM_T term = mem_get_term(handle);
   if (term == vg_path_term)                    { return OBJECT_TYPE_PATH; }
   else if (term == vg_paint_term)              { return OBJECT_TYPE_PAINT; }
   else if (term == vg_font_term)               { return OBJECT_TYPE_FONT; }
   else if (term == vg_image_term)              { return OBJECT_TYPE_IMAGE; }
   else if (term == vg_child_image_term)        { return OBJECT_TYPE_CHILD_IMAGE; }
   else if (term == vg_mask_layer_term)         { return OBJECT_TYPE_MASK_LAYER; }
   else if (!term)                              { return OBJECT_TYPE_STEM; }
   else if (term == vg_font_bprint_term)        { return OBJECT_TYPE_FONT_BPRINT; }
   else if (term == vg_image_bprint_term)       { return OBJECT_TYPE_IMAGE_BPRINT; }
   else if (term == vg_child_image_bprint_term) { return OBJECT_TYPE_CHILD_IMAGE_BPRINT; }
   else if (term == vg_mask_layer_bprint_term)  { return OBJECT_TYPE_MASK_LAYER_BPRINT; }
   else if (term == vg_paint_bprint_term)       { return OBJECT_TYPE_PAINT_BPRINT; }
   else if (term == vg_path_bprint_term)        { return OBJECT_TYPE_PATH_BPRINT; }
   UNREACHABLE();
   return (OBJECT_TYPE_T)0;
}

/*
   bool object_from_bprint(MEM_HANDLE_T handle, OBJECT_TYPE_T type)

   Converts an object from a blueprint type to the corresponding normal type. A
   return value of false indicates failure

   Preconditions:

   handle is a valid handle to an object with type in {OBJECT_TYPE_FONT_BPRINT,
      OBJECT_TYPE_IMAGE_BPRINT, OBJECT_TYPE_CHILD_IMAGE_BPRINT,
      OBJECT_TYPE_MASK_LAYER_BPRINT, OBJECT_TYPE_PAINT_BPRINT,
      OBJECT_TYPE_PATH_BPRINT}
   type is the type of handle

   Postconditions:

   If the function succeeds:
   - true is returned
   - handle is now a valid handle to an object with type <the normal type
     corresponding to the original blueprint type>
   otherwise:
   - we failed due to out-of-memory
   - false is returned
   - handle is unchanged
*/

static bool object_from_bprint(MEM_HANDLE_T handle, OBJECT_TYPE_T type)
{
   switch (_msb(type)) {
   case OBJECT_TYPE_FONT_BPRINT_BI:        return vg_font_from_bprint(handle);
   case OBJECT_TYPE_IMAGE_BPRINT_BI:       return vg_image_from_bprint(handle);
   case OBJECT_TYPE_CHILD_IMAGE_BPRINT_BI: return vg_child_image_from_bprint(handle);
   case OBJECT_TYPE_MASK_LAYER_BPRINT_BI:  return vg_mask_layer_from_bprint(handle);
   case OBJECT_TYPE_PAINT_BPRINT_BI:       return vg_paint_from_bprint(handle);
   case OBJECT_TYPE_PATH_BPRINT_BI:        return vg_path_from_bprint(handle);
   default:                                UNREACHABLE(); return false;
   }
}

/*
   bool check_object(VG_SERVER_STATE_T *state, MEM_HANDLE_T handle,
      OBJECT_TYPE_T allowed_types)

   Returns true iff handle is in state->shared_state->objects (which implies
   that it refers to a valid VG object) and its type is in allowed_types (or it
   can be converted to a type in allowed_types -- check_object will do the
   conversion)

   Preconditions:

   state points to a valid VG_SERVER_STATE_T
   allowed_types is a subset of {OBJECT_TYPE_STEM, OBJECT_TYPE_FONT_BPRINT,
      OBJECT_TYPE_IMAGE_BPRINT, OBJECT_TYPE_CHILD_IMAGE_BPRINT,
      OBJECT_TYPE_MASK_LAYER_BPRINT, OBJECT_TYPE_PAINT_BPRINT,
      OBJECT_TYPE_PATH_BPRINT, OBJECT_TYPE_FONT, OBJECT_TYPE_IMAGE,
      OBJECT_TYPE_CHILD_IMAGE, OBJECT_TYPE_MASK_LAYER, OBJECT_TYPE_PAINT,
      OBJECT_TYPE_PATH}

   Postconditions:

   If true is returned, handle is now a valid handle to an object with type in
      allowed_types (and is in state->shared_state->objects). Otherwise, no
      changes have been made to handle

   If no current error, the following conditions cause error to assume the specified value

      VG_BAD_HANDLE_ERROR           handle is not a valid object for this context or the type isn't and cannot be converted to a type in allowed_types
      VG_OUT_OF_MEMORY_ERROR        out of memory

   if more than one condition holds, the first error is generated.
   false is returned on error.
*/

static bool check_object(
   VG_SERVER_STATE_T *state,
   MEM_HANDLE_T handle,
   OBJECT_TYPE_T allowed_types)
{
   OBJECT_TYPE_T type;
   if (!contains_object(state, handle)) {
      set_error(state, VG_BAD_HANDLE_ERROR);
      return false;
   }
   type = get_object_type(handle);
   if (!(allowed_types & type)) {
      if (allowed_types & ((type & OBJECT_TYPE_BPRINT) << OBJECT_TYPE_FROM_BPRINT_SHIFT)) {
         /*
            this type would be allowed, but it's a blueprint
            try to convert to a real object
         */

         if (!object_from_bprint(handle, type)) {
            set_error(state, VG_OUT_OF_MEMORY_ERROR);
            return false;
         }
      } else {
         set_error(state, VG_BAD_HANDLE_ERROR);
         return false;
      }
   }
   return true;
}
