/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_ERRORS_H
#define GLSL_ERRORS_H

#include "middleware/khronos/glsl/glsl_compiler.h"

extern int lineNumber;

typedef enum
{
	ERROR_UNKNOWN,
	ERROR_CUSTOM,
	ERROR_PREPROCESSOR,
	ERROR_LEXER_PARSER,
	ERROR_SEMANTIC,
	ERROR_LINKER,
	ERROR_OPTIMIZER,
   WARNING
} ErrorType;

// Fetches a standard error string based on type and code, but if clarification is supplied, prints that too.
void glsl_compile_error(ErrorType e, int code, int line_num, const char* clarification, ...);

extern char* ErrorsCustom[];
extern char* ErrorsPreprocessor[];
extern char* ErrorsLexerParser[];
extern char* ErrorsSemantic[];
extern char* ErrorsLinker[];
extern char* ErrorsOptimizer[];
extern char* Warnings[];

extern char* ErrorTypeStrings[];
extern char** ErrorStrings[];

#endif // ERRORS_H