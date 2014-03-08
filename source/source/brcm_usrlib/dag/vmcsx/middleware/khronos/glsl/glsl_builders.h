/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_BUILDERS_H
#define GLSL_BUILDERS_H

#include "middleware/khronos/glsl/glsl_symbols.h"

//
// Symbol builders.
//

// Readies the struct builder for the next struct.
void glsl_reinit_struct_builder(void);
// Readies the function builder for the next function.
void glsl_reinit_function_builder(void);

// Checks that the given table can accept a new non-function symbol with the given name.
void glsl_check_namespace_nonfunction(SymbolTable* table, const char* name);

// Inserts the given STATEMENT_FUNCTION_DEF into the last function symbol we added to the main symbol table.
void glsl_insert_function_definition(Statement* statement);

// Adds the struct type *g_TypeBuilder to the main symbol table.
void glsl_commit_struct_type(const char* name);

// Adds a singleton (e.g. "x") of type *g_TypeBuilder to a symbol table.
void glsl_commit_singleton(SymbolTable* table, SymbolFlavour flavour, const char* name);
// Specializes glsl_commit_singleton() for variable instances.
void glsl_commit_singleton_variable_instance(const char* name, Expr* initializer);
// Specializes glsl_commit_singleton() for struct members.
void glsl_commit_singleton_struct_member(const char* name);
// Specializes glsl_commit_singleton() for function params.
void glsl_commit_singleton_function_param(const char* name);
// Specializes glsl_commit_singleton() for function instances.
// At this point we're only committing a declaration,
// because even if we're parsing a definition, we've only seen the prototype so far.
void glsl_commit_singleton_function_declaration(const char* name);

// Adds an array (e.g. "x[1]") of type *g_TypeBuilder to a symbol table.
void glsl_commit_array(SymbolTable* table, SymbolFlavour flavour, const char* name, Expr* size);
// Specializes glsl_commit_array() for variable instances.
void glsl_commit_array_instance(const char* name, Expr* size);
// Specializes glsl_commit_array() for struct members.
void glsl_commit_array_struct_member(const char* name, Expr* size);
// Specializes glsl_commit_array() for function params.
void glsl_commit_array_function_param(const char* name, Expr* size);

// Aggregates the struct symbol table into a new SymbolType at *g_TypeBuilder, and then clears it for the next struct.
void glsl_build_struct_type(void);
// Aggregates the function symbol table into a new SymbolType at *g_TypeBuilder, and then clears it for the next function.
void glsl_build_function_type(void);

// Instantiates all function params in the main symbol table.
void glsl_instantiate_function_params(SymbolType* fun);

// Controls scoping in the main symbol table.
void glsl_enter_scope(void);
void glsl_exit_scope(void);


#endif // BUILDERS_H