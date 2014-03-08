/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_STRINGBUILDER_H
#define GLSL_STRINGBUILDER_H

#include "middleware/khronos/glsl/glsl_fastmem.h"

typedef struct
{
	char* buf;
	size_t len; // note that len does not include the NULL byte, so the NULL byte will be stored at buf[len].
	size_t capacity; // this of course does include the NULL byte.
} StringBuilder;

#ifndef SB_INIT_CAPACITY
#define SB_INIT_CAPACITY 128
#endif

// Invalidate the internal pointer
void glsl_sb_invalidate(StringBuilder* sb);

// If buf is NULL, allocates SB_INIT_CAPACITY bytes of space.
// Otherwise just sets length to zero and adds NULL terminator.
void glsl_sb_reset(StringBuilder* sb);

// The string builder will ensure that it has enough space to write n more chars, so ensure this is accurate!
// n does *not* include the NULL terminator.
void glsl_sb_append(StringBuilder* sb, const size_t n, const char* format, ...);

// Backs up n characters (but stopping at 0).
void glsl_sb_back_up(StringBuilder* sb, const size_t n);

// Copies the current string out of the buffer (using strdup_fast()).
// Does not free builder memory or reset its length.
char* glsl_sb_copy_out_fast(StringBuilder* sb);

#endif // STRINGBUILDER_H