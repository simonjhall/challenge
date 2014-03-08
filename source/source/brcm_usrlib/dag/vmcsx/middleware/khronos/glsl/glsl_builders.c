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
#include <string.h>

#include "middleware/khronos/glsl/glsl_builders.h"
#include "middleware/khronos/glsl/glsl_errors.h"
#include "middleware/khronos/glsl/glsl_globals.h"
#include "middleware/khronos/glsl/glsl_trace.h"
#include "middleware/khronos/glsl/glsl_fastmem.h"

void glsl_reinit_struct_builder(void)
{
	if (g_StructBuilderMembers)
	{
		glsl_symbol_table_clear(g_StructBuilderMembers);
	}
	else
	{
		g_StructBuilderMembers = glsl_symbol_table_new();
	}
	
	g_StructBuilderMemberCount = 0;
	g_StructBuilderSizeAsConst = 0;
	g_StructBuilderScalarCount = 0;
	g_StructBuilderCanBeConst = true;
}

void glsl_reinit_function_builder(void)
{
	if (g_FunctionBuilderParams)
	{
		glsl_symbol_table_clear(g_FunctionBuilderParams);
	}
	else
	{
		g_FunctionBuilderParams = glsl_symbol_table_new();
	}
	
	g_FunctionBuilderParamCount = 0;
	g_FunctionBuilderVoidCount = 0;
}

void glsl_check_namespace_nonfunction(SymbolTable* table, const char* name)
{
	Symbol *sym = glsl_symbol_table_lookup(table, name, false);
	if (sym)
	{
		// Strictly we might want to distinguish between 22,23,24 here.
		glsl_compile_error(ERROR_SEMANTIC, 24, g_LineNumber, NULL);
		return;
	}
}

void glsl_insert_function_definition(Statement* statement)
{
	vcos_assert(SYMBOL_FUNCTION_INSTANCE == g_DefinitionInsertionSymbol->flavour);
	vcos_assert(STATEMENT_FUNCTION_DEF == statement->flavour);

	if (g_DefinitionInsertionSymbol->u.function_instance.function_def)
	{
		// Attempting to define a function which already has a definition!
		glsl_compile_error(ERROR_SEMANTIC, 23, g_LineNumber, NULL);
		return;
	}
	else
	{
		TRACE(("inserting definition for function symbol <%s>\n", g_DefinitionInsertionSymbol->name));
		g_DefinitionInsertionSymbol->u.function_instance.function_def = statement;
      g_DefinitionInsertionSymbol->type = g_DefinitionInsertionSymbolType;
		g_DefinitionInsertionSymbol = NULL;
      g_DefinitionInsertionSymbolType = NULL;
	}
}

void glsl_commit_struct_type(const char* name)
{
	Symbol* symbol = (Symbol *)malloc_fast(sizeof(Symbol));

	g_TypeBuilder->name = name;

   glsl_symbol_construct_type(symbol, g_TypeBuilder);

	glsl_check_namespace_nonfunction(g_SymbolTable, name);
	glsl_symbol_table_insert(g_SymbolTable, symbol);
	TRACE(("added symbol <%s> as new struct type (%d bytes as compile time constant)\n", name, g_TypeBuilder->size_as_const));
}

void glsl_commit_singleton(SymbolTable* table, SymbolFlavour flavour, const char* name)
{
	Symbol *sym;
	Symbol* symbol;
	
	if (&primitiveTypes[PRIM_VOID] == g_TypeBuilder)
	{
		// Cannot instantiate void type.
		glsl_compile_error(ERROR_CUSTOM, 18, g_LineNumber, NULL);
		return;
	}

   if (!strncmp(name, "gl_", 3)) 
   {
		// Cannot instantiate symbol whose name begins "gl_"
		glsl_compile_error(ERROR_CUSTOM, 32, g_LineNumber, NULL);
		return;
   }

	symbol = (Symbol *)malloc_fast(sizeof(Symbol));

	switch (flavour)
	{
		case SYMBOL_VAR_INSTANCE:
         {
            void* compile_time_value = NULL;

			   glsl_check_namespace_nonfunction(table, name);

			   if (TYPE_QUAL_CONST == g_TypeQual)
			   {
				   if (!g_InitializerExpr)
				   {
					   // Constant variable needs initializer.
					   glsl_compile_error(ERROR_SEMANTIC, 31, g_LineNumber, NULL);
					   return;
				   }
   				
				   // Equality test is good for structs and primitives.
				   if (g_TypeBuilder != g_InitializerExpr->type)
				   {
					   // Type mismatch.
					   glsl_compile_error(ERROR_SEMANTIC, 1, g_LineNumber, NULL);
					   return;
				   }
   				
				   if (!g_InitializerExpr->compile_time_value)
				   {
					   // Initializer must be constant.
					   glsl_compile_error(ERROR_SEMANTIC, 13, g_LineNumber, NULL);
					   return;
				   }
   				
				   compile_time_value = g_InitializerExpr->compile_time_value;
				   TRACE_CONSTANT(g_TypeBuilder, compile_time_value);
			   }

            glsl_symbol_construct_var_instance(symbol, name, g_TypeBuilder, g_TypeQual, compile_time_value);
         }
			break;

		case SYMBOL_PARAM_INSTANCE:
         {
			   glsl_check_namespace_nonfunction(table, name);

			   if (TYPE_QUAL_CONST == g_TypeQual && (PARAM_QUAL_INOUT == g_ParamQual || PARAM_QUAL_OUT == g_ParamQual))
			   {
				   // const type qualifier cannot be used with out or inout parameter qualifier.
				   glsl_compile_error(ERROR_CUSTOM, 10, g_LineNumber, NULL);
				   return;
			   }

            if (TYPE_QUAL_ATTRIBUTE == g_TypeQual || TYPE_QUAL_VARYING == g_TypeQual || TYPE_QUAL_INVARIANT_VARYING == g_TypeQual || TYPE_QUAL_UNIFORM == g_TypeQual) {
				   // const type qualifier cannot be used with out or inout parameter qualifier.
				   glsl_compile_error(ERROR_CUSTOM, 31, g_LineNumber, NULL);
               return;
            }
   			
            glsl_symbol_construct_param_instance(symbol, name, g_TypeBuilder, g_TypeQual, g_ParamQual);
         }
			break;

		case SYMBOL_FUNCTION_INSTANCE:
         {
			   // A simple name space check is not enough - we are allowed:
			   // 1) overloads
			   // 2) multiple declarations
			   // 3) a single definition per declaration
			   // At the moment though we're only committing a declaration, so we can't catch (3).
   			Symbol* next_overload;

			   // The definition AST will be inserted into this symbol unless a symbol with this prototype exists already.
			   g_DefinitionInsertionSymbol = symbol;
            g_DefinitionInsertionSymbolType = g_TypeBuilder;
   			
			   // Special case main().
			   if (0 == strcmp("main", name))
			   {
				   if (&primitiveTypes[PRIM_VOID] != g_TypeBuilder->u.function_type.return_type ||
					   0 != g_TypeBuilder->u.function_type.param_count)
				   {
					   // Signature is not "void main()".
					   glsl_compile_error(ERROR_SEMANTIC, 29, g_LineNumber, NULL);
				   }
			   }
   			
			   sym = glsl_symbol_table_lookup(table, name, true);
			   if (sym)
			   {
				   // May have a name clash.
				   if (SYMBOL_FUNCTION_INSTANCE == sym->flavour)
				   {
					   // Other declarations or definitions exist with this name.
					   // We must check for a prototype match.
					   Symbol* match = glsl_resolve_overload_using_prototype(sym, g_TypeBuilder);
					   if (match)
					   {
						   // There is already a function with our signature in the symbol table, so don't add a new one.
						   TRACE(("declaration of function symbol <%s> matches existing declaration\n", name));
						   g_DefinitionInsertionSymbol = match;
						   g_LastInstance = match;
						   return;
					   }
					   else
					   {
						   // We have a new overload. Chain the other overloads onto ours.
						   TRACE(("declaration of function symbol <%s> is new overload\n", name));
						   next_overload = sym;
					   }
				   }
				   else
				   {
					   // The name is in use by a non-function, so give error.
					   glsl_compile_error(ERROR_SEMANTIC, 24, g_LineNumber, NULL);
					   return;
				   }
			   }
			   else
			   {
				   // No name clash.
				   next_overload = NULL;
			   }
   			
            glsl_symbol_construct_function_instance(symbol, name, g_TypeBuilder, NULL, NULL, next_overload);

			   // At this point we have a fresh declaration to insert into the symbol table.
			   TRACE(("adding symbol <%s> as function\n", name));
         }
			break;
			
		case SYMBOL_STRUCT_MEMBER:
         {
			   glsl_check_namespace_nonfunction(table, name);
   			
			   if (g_StructBuilderCanBeConst)
			   {
				   // If this member cannot be const then neither can the struct.
				   if (g_TypeBuilder->size_as_const == 0)
				   {
					   g_StructBuilderCanBeConst = false;
					   g_StructBuilderSizeAsConst = 0;
				   }
				   else
				   {
					   g_StructBuilderSizeAsConst += g_TypeBuilder->size_as_const;
				   }
			   }

            g_StructBuilderScalarCount += g_TypeBuilder->scalar_count;

            glsl_symbol_construct_struct_member(symbol, name, g_TypeBuilder);
         }
			break;

		default:
			UNREACHABLE();
			break;
	}
	
	glsl_symbol_table_insert(table, symbol);
	g_LastInstance = symbol;
	return;
}

void glsl_commit_singleton_variable_instance(const char* name, Expr* initializer)
{
	g_InitializerExpr = initializer;
	glsl_commit_singleton(g_SymbolTable, SYMBOL_VAR_INSTANCE, name);
	TRACE(("added symbol <%s> as single instance\n", name));
}

void glsl_commit_singleton_struct_member(const char* name)
{
	glsl_commit_singleton(g_StructBuilderMembers, SYMBOL_STRUCT_MEMBER, name);
	TRACE(("added symbol <%s> as struct singleton member %d\n", name, g_StructBuilderMemberCount));
	g_StructBuilderMemberCount++;
}

void glsl_commit_singleton_function_param(const char* name)
{
	// Ignore nameless parameters with void type, for now at least.
	if (&primitiveTypes[PRIM_VOID] == g_TypeBuilder && !strncmp(name, "$$anon", 6))
	{
		g_FunctionBuilderVoidCount++;
		return;
	}

	glsl_commit_singleton(g_FunctionBuilderParams, SYMBOL_PARAM_INSTANCE, name);
	TRACE(("added symbol <%s> as function singleton parameter %d\n", name, g_FunctionBuilderParamCount));
	g_FunctionBuilderParamCount++;
}

void glsl_commit_singleton_function_declaration(const char* name)
{
	glsl_commit_singleton(g_SymbolTable, SYMBOL_FUNCTION_INSTANCE, name);
}

void glsl_commit_array(SymbolTable* table, SymbolFlavour flavour, const char* name, Expr* size)
{
	Symbol* symbol;
	SymbolType* type;
	SymbolType* member_type = g_TypeBuilder;
	
	if (&primitiveTypes[PRIM_VOID] == member_type)
	{
		// Cannot instantiate void type.
		glsl_compile_error(ERROR_CUSTOM, 18, g_LineNumber, NULL);
		return;
	}

	type = (SymbolType *)malloc_fast(sizeof(SymbolType));
   symbol = (Symbol *)malloc_fast(sizeof(Symbol));
	

   // Construct array type.

	type->flavour = SYMBOL_ARRAY_TYPE;
	
	if (&primitiveTypes[PRIM_INT] != size->type || !size->compile_time_value)
	{
		// Array size must be integral constant expression.
		glsl_compile_error(ERROR_SEMANTIC, 15, g_LineNumber, NULL);
		return;
	}
	type->u.array_type.member_count_expression = size;
	type->u.array_type.member_count = *(int*)size->compile_time_value;
	
	if (type->u.array_type.member_count <= 0)
	{
		// Array size must be greater than zero.
		glsl_compile_error(ERROR_SEMANTIC, 17, g_LineNumber, NULL);
		return;
	}
	
	type->u.array_type.member_type = member_type;
	
   TRACE(("<%d> members in array <%s>\n", type->u.array_type.member_count, name));
	type->size_as_const = 0;
   type->scalar_count = type->u.array_type.member_count * type->u.array_type.member_type->scalar_count;

	// Create the array type name.
	glsl_sb_reset(&g_StringBuilder);
	glsl_sb_append(&g_StringBuilder,
		strlen(member_type->name) + 2 + (sizeof(type->u.array_type.member_count)*3), // 3 digits per byte
		"%s[%d]", member_type->name, type->u.array_type.member_count);
	type->name = glsl_sb_copy_out_fast(&g_StringBuilder);
	

   // Construct array symbol.

   switch (flavour)
	{
		case SYMBOL_VAR_INSTANCE:
			{
			   if (TYPE_QUAL_CONST == g_TypeQual)
			   {
				   // Array cannot be const.
				   glsl_compile_error(ERROR_CUSTOM, 5, g_LineNumber, NULL);
				   return;
			   }
			   
            glsl_symbol_construct_var_instance(symbol, name, type, g_TypeQual, NULL);
         }
			break;

		case SYMBOL_PARAM_INSTANCE:
         {
			   if (TYPE_QUAL_CONST == g_TypeQual && (PARAM_QUAL_INOUT == g_ParamQual || PARAM_QUAL_OUT == g_ParamQual))
			   {
				   // const type qualifier cannot be used with out or inout parameter qualifier.
				   glsl_compile_error(ERROR_CUSTOM, 10, g_LineNumber, NULL);
				   return;
			   }
			   
            glsl_symbol_construct_param_instance(symbol, name, type, g_TypeQual, g_ParamQual);
         }
			break;

		case SYMBOL_STRUCT_MEMBER:
         {
			   // Arrays cannot be const and so neither can be the struct.
			   g_StructBuilderCanBeConst = false;
			   g_StructBuilderSizeAsConst = 0;

            // But they can contain scalars.
            g_StructBuilderScalarCount += type->scalar_count;

            glsl_symbol_construct_struct_member(symbol, name, type);
         }
			break;

		default:
			// Anything else is invalid for arrays!
			UNREACHABLE();
			break;
	}

	glsl_check_namespace_nonfunction(table, name);
	glsl_symbol_table_insert(table, symbol);
	g_LastInstance = symbol;
	return;
}

void glsl_commit_array_instance(const char* name, Expr* size)
{
	glsl_commit_array(g_SymbolTable, SYMBOL_VAR_INSTANCE, name, size);
	TRACE(("added symbol <%s> as array instance\n", name));
}

void glsl_commit_array_struct_member(const char* name, Expr* size)
{
	glsl_commit_array(g_StructBuilderMembers, SYMBOL_STRUCT_MEMBER, name, size);
	TRACE(("added symbol <%s> as struct array member %d\n", name, g_StructBuilderMemberCount));
	g_StructBuilderMemberCount++;
}

void glsl_commit_array_function_param(const char* name, Expr* size)
{
	glsl_commit_array(g_FunctionBuilderParams, SYMBOL_PARAM_INSTANCE, name, size);
	TRACE(("added symbol <%s> as function array param %d\n", name, g_FunctionBuilderParamCount));
	g_FunctionBuilderParamCount++;
}

void glsl_build_struct_type(void)
{
	const char** structMemberNames;
	SymbolType** structMemberTypes;
	Symbol* symbol;
	int i = g_StructBuilderMemberCount - 1;
	
	g_TypeBuilder = (SymbolType *)malloc_fast(sizeof(SymbolType));
	structMemberNames = (const char **)malloc_fast(sizeof(char*) * g_StructBuilderMemberCount);
	structMemberTypes = (SymbolType **)malloc_fast(sizeof(SymbolType*) * g_StructBuilderMemberCount);
	
//	while (symbol = glsl_symbol_table_pop(g_StructBuilderMembers))
	for (symbol = glsl_symbol_table_pop(g_StructBuilderMembers); symbol; symbol = glsl_symbol_table_pop(g_StructBuilderMembers))
	{
		vcos_assert(SYMBOL_STRUCT_MEMBER == symbol->flavour);
		structMemberNames[i] = symbol->name;
		structMemberTypes[i] = symbol->type;
		i--; // build backwards to preserve source order
	}
	
	g_TypeBuilder->flavour = SYMBOL_STRUCT_TYPE;
	g_TypeBuilder->size_as_const = g_StructBuilderSizeAsConst;
	g_TypeBuilder->scalar_count = g_StructBuilderScalarCount;
	g_TypeBuilder->u.struct_type.member_count = g_StructBuilderMemberCount;
	g_TypeBuilder->u.struct_type.member_names = structMemberNames;
	g_TypeBuilder->u.struct_type.member_types = structMemberTypes;
}

static void sb_append_qualifiers(StringBuilder* sb, TypeQualifier tq, ParamQualifier pq)
{
	const char* tqs;
	const char* pqs;

	switch (tq)
	{
		case TYPE_QUAL_NONE:
			tqs = "";
			break;
		case TYPE_QUAL_CONST:
			tqs = "const ";
			break;
		case TYPE_QUAL_ATTRIBUTE:
			tqs = "attribute ";
			break;
		case TYPE_QUAL_VARYING:
			tqs = "varying ";
			break;
		case TYPE_QUAL_INVARIANT_VARYING:
			tqs = "invariant varying ";
			break;
		case TYPE_QUAL_UNIFORM:
			tqs = "uniform ";
			break;
		default:
			UNREACHABLE();
			return;
	}

	switch (pq)
	{
		case PARAM_QUAL_IN:
			pqs = "in ";
			break;
		case PARAM_QUAL_OUT:
			pqs = "out ";
			break;
		case PARAM_QUAL_INOUT:
			pqs = "inout ";
			break;
		default:
			UNREACHABLE();
			return;
	}

	glsl_sb_append(sb, strlen(tqs) + strlen(pqs), "%s%s", tqs, pqs);
}

void glsl_build_function_type(void)
{
	Symbol** functionParams = NULL;
	Symbol* symbol = NULL;
	int i = g_FunctionBuilderParamCount - 1;
	
	g_TypeBuilder = (SymbolType *)malloc_fast(sizeof(SymbolType));
	
	if (g_FunctionBuilderVoidCount > 1 || (g_FunctionBuilderVoidCount == 1 && g_FunctionBuilderParamCount > 0))
	{
		// void type can only be used in empty formal parameter list.
		glsl_compile_error(ERROR_CUSTOM, 19, g_LineNumber, NULL);
		return;
	}

	if (g_FunctionBuilderParamCount > 0)
	{	
		functionParams = (Symbol **)malloc_fast(sizeof(Symbol*) * g_FunctionBuilderParamCount);
		
		// Populate the parameter array.
//		while (symbol = glsl_symbol_table_pop(g_FunctionBuilderParams))
		for (symbol = glsl_symbol_table_pop(g_FunctionBuilderParams); symbol; symbol = glsl_symbol_table_pop(g_FunctionBuilderParams))
		{
			vcos_assert(SYMBOL_PARAM_INSTANCE == symbol->flavour);
			functionParams[i] = symbol;
			i--; // build backwards to preserve source order
		}

		// Create the type name (working forwards!).
		glsl_sb_reset(&g_StringBuilder);
		glsl_sb_append(&g_StringBuilder, 1, "(");
		for (i = 0; i < g_FunctionBuilderParamCount; i++)
		{
			sb_append_qualifiers(&g_StringBuilder, functionParams[i]->u.var_instance.type_qual, functionParams[i]->u.var_instance.param_qual);
			glsl_sb_append(&g_StringBuilder, strlen(functionParams[i]->type->name) + 2, "%s, ", functionParams[i]->type->name);
		}
		glsl_sb_back_up(&g_StringBuilder, 2); // remove last ", "
		glsl_sb_append(&g_StringBuilder, 1, ")");
		g_TypeBuilder->name = glsl_sb_copy_out_fast(&g_StringBuilder);
	}
	else
	{
		g_TypeBuilder->name = "()";
	}
	
	g_TypeBuilder->flavour = SYMBOL_FUNCTION_TYPE;
	g_TypeBuilder->size_as_const = 0;
	g_TypeBuilder->scalar_count = 0;
	g_TypeBuilder->u.function_type.return_type = g_FunctionBuilderReturnType;
	g_TypeBuilder->u.function_type.param_count = g_FunctionBuilderParamCount;
	g_TypeBuilder->u.function_type.params = functionParams;
}

void glsl_instantiate_function_params(SymbolType* fun)
{
	int i;
	
	vcos_assert(SYMBOL_FUNCTION_TYPE == fun->flavour);
	
	for (i = 0; i < fun->u.function_type.param_count; i++)
	{
      if (!strncmp(fun->u.function_type.params[i]->name, "$$anon", 6))
         glsl_compile_error(ERROR_CUSTOM, 30, g_LineNumber, NULL);

		// No need to check namespace as we have a clean scope for the function.
		glsl_symbol_table_insert(g_SymbolTable, fun->u.function_type.params[i]);
	}
}

void glsl_enter_scope(void)
{
	TRACE(("entering scope\n"));
	glsl_symbol_table_enter_scope(g_SymbolTable);
}

void glsl_exit_scope(void)
{
	TRACE(("exiting scope\n"));
	glsl_symbol_table_exit_scope(g_SymbolTable);
}
