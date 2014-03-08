/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_AST_PRINT_H
#define GLSL_AST_PRINT_H

// Debug only.
#ifndef NDEBUG

#include <stdio.h>
#include "middleware/khronos/glsl/glsl_ast.h"


// Pretty print a compile time constant.
void glsl_print_compile_time_value(FILE* f, SymbolType* type, void* compile_time_value);

// Pretty prints the Expr as source code to the given file.
// No newlines are added.
// If fully_evaluated is set, will replace expressions by their values.
void glsl_print_expr(FILE* f, Expr* expr, bool fully_evaluated);

// Pretty prints the Statement as source code to the given file.
// The string will be prepended by indent_depth tabs.
// Newlines may be added in the string but it will not start or end with a newline.
// If fully_evaluated is set, will replace expressions by their values.
void glsl_print_statement(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon);


#endif // _DEBUG
#endif // AST_PRINT_H