/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GLXX_BUFFER_H
#define GLXX_BUFFER_H

#include "interface/khronos/include/GLES/gl.h"

#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/common/khrn_mem.h"
#include "middleware/khronos/common/khrn_interlock.h"

// Some platforms may not need this
#if !defined(__VIDEOCORE4__) || defined(__BCM2708A0__)
#define REQUIRE_MAX_INDEX_CHECK
#endif
/*
   Invariant:

   Before performing _any_ operation on a GLXX_BUFFER_T, the underlying memory
   block will have had glxx_buffer_term() installed as its terminator



   We have a conceptual boolean variable conceptual_buffers_owned_by_master which
   represents our ability to modify buffer contents from within the master process.
   (It is not necessary in order to modify the buffer object itself or to alter the
   "shared" object by adding/removing buffers).

               glxx_hw_draw_triangles
                ------------------->
           true                     false
                <-------------------
                  khrn_fifo_finish
*/

/*
   To reduce flushes to the hardware, we keep a small pool of buffers.
   If an attempt is made to change the contents of the buffer while
   there is still a pending render from that buffer, one of the other 
   members of the pool is used instead.
*/
typedef struct {
   /*
      storage associated with the buffer

      Invariants:

      mh_storage != MEM_INVALID_HANDLE
      mh_storage is MEM_ZERO_SIZE_HANDLE or we have the only reference to it
   */

   MEM_HANDLE_T mh_storage;


   KHRN_INTERLOCK_T interlock;

} GLXX_BUFFER_INNER_T;

#define GLXX_BUFFER_POOL_SIZE 16

typedef struct {
   /*
      name of the buffer

      Implementation Note:

      This is here solely to let us answer glGet<blah>() queries. Note that due
      to the shared context semantics it's not necessarily a no-op to get the name
      of the bound buffer and then call glBindBuffer() with that name

      Invariant:

      name != 0
   */

   uint32_t name;

   /*
      usage of the buffer

      Invariants:

      usage in {STATIC_DRAW, DYNAMIC_DRAW}
   */

   GLenum usage;

   int current_item;
   GLXX_BUFFER_INNER_T pool[GLXX_BUFFER_POOL_SIZE];
 

#ifdef REQUIRE_MAX_INDEX_CHECK
   /*
      indicates which element type has been assumed when calculating the stored maximum

         0        maximum not yet calculated
         1        calculated for GL_UNSIGNED_BYTE
         2        calculated for GL_UNSIGNED_SHORT

      Invariants:

      size_used_for_max in {0, 1, 2}
   */

   uint32_t size_used_for_max;

   /*
      The maximum element in this buffer as indicated by size_used_for_max.

      Invariants:

      if size_used_for_max==1 then max==maximum(storage treated as a sequence of unsigned bytes)
      if size_used_for_max==2 then max==maximum(storage treated as a sequence of unsigned shorts)

      Implementation notes:

      The maximum of an empty set is considered to be INT_MIN
      If size is odd and buffer is treated as unsigned shorts, size is effectively rounded down to
      nearest even number.
   */
   int max;
#endif
} GLXX_BUFFER_T;

extern void glxx_buffer_init(GLXX_BUFFER_T *buffer, uint32_t name);
extern void glxx_buffer_term(void *v, uint32_t size);

extern bool glxx_buffer_data(GLXX_BUFFER_T *buffer, int32_t size, const void *data, GLenum usage);
extern void glxx_buffer_subdata(GLXX_BUFFER_T *buffer, int32_t offset, int32_t size, const void *data);

extern int glxx_buffer_get_size(GLXX_BUFFER_T *buffer);

#ifdef REQUIRE_MAX_INDEX_CHECK
extern bool glxx_buffer_values_are_less_than(GLXX_BUFFER_T *buffer, int offset, int count, int size, int value);
#endif

extern MEM_HANDLE_T glxx_buffer_get_storage_handle(GLXX_BUFFER_T *buffer);
extern uint32_t glxx_buffer_get_interlock_offset(GLXX_BUFFER_T *buffer);

#endif
