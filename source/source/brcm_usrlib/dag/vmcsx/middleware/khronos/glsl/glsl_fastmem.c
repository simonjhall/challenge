/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include "middleware/khronos/glsl/glsl_common.h"

#include "middleware/khronos/glsl/glsl_fastmem.h"
#include "middleware/khronos/common/khrn_stats.h"

#include <stdlib.h>

Hunk *fastmem_hunk;

void glsl_fastmem_init(void)
{
   khrn_stats_record_start(KHRN_STATS_COMPILE);
   fastmem_hunk = NULL;
}

void glsl_fastmem_term(void)
{
   Hunk *hunk, *next;

   khrn_stats_record_end(KHRN_STATS_COMPILE);
   for (hunk = fastmem_hunk; hunk; hunk = next) {
#ifndef FASTMEM_USE_MALLOC
      MEM_HANDLE_T handle = hunk->handle;
#endif

      next = hunk->next;
#ifdef FASTMEM_USE_MALLOC
      free(hunk);
#else
      mem_unlock(handle);
      mem_release(handle);
#endif
   }

   fastmem_hunk = NULL;
}

Hunk *glsl_fastmem_alloc_hunk(Hunk *next)
{
   MEM_HANDLE_T handle = MEM_INVALID_HANDLE;
   Hunk *hunk;

   /*
      check free list for available hunks; allocate one from the global heap if none found
   */

#ifdef FASTMEM_USE_MALLOC
   hunk = malloc(sizeof(Hunk));
   memset(hunk, 0, sizeof(Hunk));
#else
   handle = MEM_ALLOC_STRUCT_EX(Hunk, MEM_COMPACT_DISCARD);       // check, no term
   if (handle == MEM_INVALID_HANDLE)
      return NULL;

   hunk = (Hunk *)mem_lock(handle);
#endif

   /*
      initialize hunk
   */

   hunk->used = 0;
   hunk->next = next;
   hunk->handle = handle;

   return hunk;
}
