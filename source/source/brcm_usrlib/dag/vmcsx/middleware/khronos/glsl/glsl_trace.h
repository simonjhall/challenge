/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_TRACE_H
#define GLSL_TRACE_H

#include <stdio.h>

#include "middleware/khronos/glsl/glsl_globals.h"
#include "middleware/khronos/glsl/glsl_ast_print.h"
#include "middleware/khronos/glsl/glsl_dataflow_print.h"

#define SLANG_TRACE 0
#define SLANG_TRACE_AST 0
#define SLANG_TRACE_AST_EXPRS_EVALUATED 0
#define SLANG_TRACE_DATAFLOW 1

#if defined(_DEBUG) && SLANG_TRACE

#define TRACE(a) \
   { \
      printf a; \
   }

#define TRACE_CONSTANT(t, v) \
   { \
      printf("constant: "); \
      glsl_print_compile_time_value(stdout, t, v); \
      printf("\n"); \
   }

#define TRACE_PHASE(phase) \
   { \
      printf("\n\n~~~~~ "); \
      printf phase; \
      printf(" ~~~~~\n\n"); \
   }


#else

#define TRACE(a) /**/
#define TRACE_CONSTANT(t, v) /**/
#define TRACE_PHASE(phase) /**/

#endif // defined(_DEBUG) && SLANG_TRACE



#if defined(_DEBUG) && SLANG_TRACE_AST

#ifdef SLANG_TRACE_AST_EXPRS_EVALUATED
#undef SLANG_TRACE_AST_EXPRS_EVALUATED
#define SLANG_TRACE_AST_EXPRS_EVALUATED true
#else
#define SLANG_TRACE_AST_EXPRS_EVALUATED false
#endif

#define TRACE_STATEMENT(s) \
   { \
      glsl_print_statement(stdout, s, SLANG_TRACE_AST_EXPRS_EVALUATED, 0, false); \
   }

#else

#define TRACE_STATEMENT(s) /**/

#endif // defined(_DEBUG) && SLANG_TRACE_AST



#if defined(_DEBUG) && SLANG_TRACE_DATAFLOW

#define TRACE_DATAFLOW_VERT_FILE "Vert.dot"
#define TRACE_DATAFLOW_FRAG_FILE "Frag.dot"
#define TRACE_DATAFLOW_FROM_ROOT(flavour, root, pass) \
   { \
      FILE* f; \
      const char* fn; \
      fn = SHADER_VERTEX == flavour ? TRACE_DATAFLOW_VERT_FILE : TRACE_DATAFLOW_FRAG_FILE; \
      f = fopen(fn, "w"); \
      printf("dumping dataflow graph to <%s>...\n", fn); \
      glsl_print_dataflow_from_root(f, root, pass); \
      fclose(f); \
   }
#define TRACE_DATAFLOW_FROM_ROOTS(flavour, roots, order, pass) \
   { \
      FILE* f; \
      const char* fn; \
      fn = SHADER_VERTEX == flavour ? TRACE_DATAFLOW_VERT_FILE : TRACE_DATAFLOW_FRAG_FILE; \
      f = fopen(fn, "w"); \
      printf("dumping dataflow graph to <%s>...\n", fn); \
      glsl_print_dataflow_from_roots(f, roots, order, pass); \
      fclose(f); \
   }

#else

#define TRACE_DATAFLOW(flavour, root, pass) /**/
#define TRACE_DATAFLOW_FROM_ROOTS(flavour, roots, order, pass) /**/

#endif // defined(_DEBUG) && SLANG_TRACE_DATAFLOW

#endif // TRACE_H