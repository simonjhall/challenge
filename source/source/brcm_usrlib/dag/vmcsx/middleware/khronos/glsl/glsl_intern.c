/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/glsl/glsl_fastmem.h"

struct {
   int size;
   int used;

   const char **table;
} state;

void glsl_init_intern(int size)
{
   state.size = size;
   state.used = 0;

   state.table = (const char **)malloc_fast(size * sizeof(const char *));
   memset(state.table, 0, size * sizeof(const char *));
}

static unsigned long hash(const char *str)
{
   unsigned long hash = 5381;
   int c;

   while ((c = *str++) != 0)
      hash = hash *33 ^ c;

   return hash;
}

const char *glsl_intern(const char *s, bool dup)
{
   const char **t;

   if (state.used >= state.size >> 1) {
      int i, size = state.size;
      const char **table = state.table;

      glsl_init_intern(state.size << 1);

      for (i = 0; i < size; i++)
         if (table[i])
            glsl_intern(table[i], false);
   }

   for (t = state.table + (hash(s) & (state.size - 1)); *t; t = (t == state.table + state.size - 1) ? state.table : t + 1)
      if (!strcmp(s, *t))
         return *t;

   state.used++;

   return *t = dup ? glsl_fastmem_strdup(s) : s;
}
