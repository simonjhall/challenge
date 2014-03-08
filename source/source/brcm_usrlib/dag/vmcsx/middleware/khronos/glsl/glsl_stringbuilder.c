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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "middleware/khronos/glsl/glsl_fastmem.h"

#include "middleware/khronos/glsl/glsl_stringbuilder.h"


void glsl_sb_invalidate(StringBuilder* sb)
{
	sb->buf = NULL;
	sb->capacity = 0;
}

void glsl_sb_reset(StringBuilder* sb)
{
	sb->len = 0;

	if (sb->buf)
	{
		vcos_assert(sb->capacity >= SB_INIT_CAPACITY);
		sb->buf[0] = '\0';
		return;
	}
   else
   {
	   sb->buf = (char *)malloc_fast(SB_INIT_CAPACITY);
	   sb->capacity = SB_INIT_CAPACITY;
	   sb->buf[0] = '\0';
   }
}

void glsl_sb_append(StringBuilder* sb, const size_t n, const char* format, ...)
{
	size_t c;
	va_list argp;

	if (sb->buf && sb->capacity)
	{
      size_t capacity_req = sb->len + n + 1;

		// Ensure the buffer is big enough.
      // Hopefully, though, this will never be necessary, as SB_INIT_CAPACITY should be big enough.
		if (capacity_req > sb->capacity)
		{
         size_t capacity_new = capacity_req * 2;
         char* buf_new;

         buf_new = (char *)malloc_fast((int32_t)capacity_new);

         memcpy(buf_new, sb->buf, sb->capacity);

         sb->capacity = capacity_new;
         sb->buf = buf_new;
		}

      vcos_assert(capacity_req <= sb->capacity);

		// Print the string.
		va_start(argp, format);
		c = vsprintf(sb->buf + sb->len, format, argp);
		va_end(argp);

		// Work out the new length.
		vcos_assert(c <= n);
		sb->len += c;
	}
}

void glsl_sb_back_up(StringBuilder* sb, const size_t n)
{
	sb->len = (sb->len >= n) ? sb->len - n : 0;
	sb->buf[sb->len] = '\0';
}

char* glsl_sb_copy_out_fast(StringBuilder* sb)
{
	return strdup_fast(sb->buf);
}
