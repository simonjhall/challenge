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
#include <assert.h>

#include "middleware/khronos/glsl/glsl_trace.h"
#include "middleware/khronos/glsl/glsl_ast.h"
#include "middleware/khronos/glsl/glsl_ast_visitor.h"
#include "middleware/khronos/glsl/glsl_symbols.h"
#include "middleware/khronos/glsl/glsl_errors.h"
#include "middleware/khronos/glsl/glsl_const_operators.h"
#include "middleware/khronos/glsl/glsl_fastmem.h"
#include "middleware/khronos/glsl/glsl_stack.h"

/*
   Potential spec bugs:

   Assignment expressions not mentioned in 5.9


   Irrelevant things in the spec which we don't implement:

   Reserved operators % & | ^ ~ >> <<

   Irrelevant things in the spec which we do implement to some extent:
   do-while loops

   The spec does not seem to specify exactly what "matching" means, in the context of
   the types of uniforms matching between fragment and vertex shaders.
   (glsl_deep_match_nonfunction_types is our interpretation)


   Mysterious "(see Section 4.3.3 “Integral Constant Expressions” )." Maybe means 5.10?

   What is the difference between
   - Subscripted array names.
   - ... array subscript results.
   
*/

/*
   Overall comments:

   Most functions have a precondition that we are "in the middle of compiling a shader".
   Most data is valid for the lifetime of fastmem.
   TODO: document all of these properly
   I won't bother labelling & referring to all the invariants that just say "this thing is valid for
   the lifetime of fastmem"
*/

#include "middleware/khronos/glsl/glsl_ast_cr.c"

//
// Utilities.
//

/*
   void check_returns(SymbolType* return_type, Statement* statement)

   Checks for:
   - void function with value return statement;
   - non-void function with plain return statement;
   - non-void function value return statement type mismatch.

   Khronos documentation:

   Function declarations must specify a return type. This type may be void. If the return type is not void,
   any return statements within the function definition must include a return-expression and the type of the
   expression must match the return type. If the return type of the function is specified as void, any such
   return statements must not include a return-expression.

   Implementation notes:

   Since array return types are not allowed, we do not actually need to call
   glsl_shallow_match_nonfunction_types to check the return type - we can rely on
   primitive and struct types being canonical. But perhaps it's cleaner this way.

   Preconditions:

   return_type points to a valid SymbolType
   return_type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE}
   statement points to a valid Statement

   Postconditions:

   If code contains an invalid return statement, raises whichever of these
   errors are first encountered:

   S0001: Type mismatch
   S0038: Function declared with a return value but return statement has no argument.
   S0039: Function declared void but return statement has an argument.

   Otherwise returns normally.

   Invariants preserved:

   -
*/
static void check_returns(SymbolType* return_type, Statement* statement)
{
	StatementChainNode* node;

   STACK_CHECK();

	switch (statement->flavour)
	{
		case STATEMENT_AST:
         break;

		case STATEMENT_FUNCTION_DEF:
			check_returns(return_type, statement->u.function_def.body);
         break;

		case STATEMENT_COMPOUND:
			for (node = statement->u.compound.statements->first; node; node = node->next)
				check_returns(return_type, node->statement);
         break;

		case STATEMENT_SELECTION:
			check_returns(return_type, statement->u.selection.if_true);

         if (statement->u.selection.if_false)
				check_returns(return_type, statement->u.selection.if_false);
         break;

		case STATEMENT_ITERATOR_FOR:
			check_returns(return_type, statement->u.iterator_for.block);
         break;

		case STATEMENT_ITERATOR_WHILE:
			check_returns(return_type, statement->u.iterator_while.block);
         break;

		case STATEMENT_ITERATOR_DO_WHILE:
			check_returns(return_type, statement->u.iterator_do_while.block);
         break;

		case STATEMENT_DECL_LIST:
		case STATEMENT_VAR_DECL:
		case STATEMENT_EXPR:
		case STATEMENT_CONTINUE:
		case STATEMENT_BREAK:
		case STATEMENT_DISCARD:
		case STATEMENT_NULL:
         break;

		case STATEMENT_RETURN:
			if (&primitiveTypes[PRIM_VOID] != return_type)
			{
				// Declared with return value but return statement has no argument.
				glsl_compile_error(ERROR_SEMANTIC, 38, statement->line_num, NULL);
			}
         break;

		case STATEMENT_RETURN_EXPR:
			if (&primitiveTypes[PRIM_VOID] == return_type)
			{
				// Declared void but return statement has an argument.
				glsl_compile_error(ERROR_SEMANTIC, 39, statement->line_num, NULL);
			}
			if (!glsl_shallow_match_nonfunction_types(statement->u.return_expr.expr->type, return_type))
			{
				// Type mismatch.
				glsl_compile_error(ERROR_SEMANTIC, 1, statement->line_num, NULL);
			}
         break;

		default:
			UNREACHABLE();
			break;
	}
}

/////////////////////
//
// Functions for constructing Expr objects
//
/////////////////////


/*
   Expr* glsl_expr_construct_value_int(const_int v)

   Khronos documentation:

   Expressions in the shading language are built from the following:
   - Constants of type bool, int, float, all vector types, and all matrix types.
   - ...

   Implementation notes:

   -

   Preconditions:

   -

   Postconditions:

   May raise the following errors:

   C0006: Out of memory

   Otherwise returns normally.
   Result is valid for the lifetime of fastmem

   Invariants preserved:

   expr->flavour is a valid ExprFlavour enum constant
   If expr->flavour==EXPR_VALUE then expr->type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE}
   expr->type is valid for the lifetime of fastmem
   expr->type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE,SYMBOL_ARRAY_TYPE}
   If expr->flavour==EXPR_VALUE then expr->compile_time_value != NULL

   (GL20_COMPILER_EXPR_COMPILE_TIME_VALUE)
   If expr->compile_time_value != NULL then:
      compile_time_value is valid for the lifetime of fastmem
      compile_time_value is a compile time instance of &primitiveTypes[PRIM_INT]
         i.e. a const_int
*/

Expr* glsl_expr_construct_value_int(const_int v)
{
	const_int* compile_time_value;
	Expr* expr;

	expr = (Expr *)malloc_fast(sizeof(Expr));
	compile_time_value = (const_int *)malloc_fast(sizeof(const_int));

	expr->line_num = g_LineNumber;
	expr->flavour = EXPR_VALUE;
	expr->type = &primitiveTypes[PRIM_INT];
	*compile_time_value = v;
	expr->compile_time_value = compile_time_value;
   expr->constant_index_expression = true;

	return expr;
}

/*
   Expr* glsl_expr_construct_value_float(const_float v)

   Khronos documentation:

   Expressions in the shading language are built from the following:
   - Constants of type bool, int, float, all vector types, and all matrix types.
   - ...

   Implementation notes:

   -

   Preconditions:

   -

   Postconditions:

   May raise the following errors:

   C0006: Out of memory

   Otherwise returns normally.
   Result is valid for the lifetime of fastmem

   Invariants preserved:

   expr->flavour is a valid ExprFlavour enum constant
   If expr->flavour==EXPR_VALUE then expr->type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE}
   expr->type is valid for the lifetime of fastmem
   expr->type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE,SYMBOL_ARRAY_TYPE}
   If expr->flavour==EXPR_VALUE then expr->compile_time_value != NULL

   (GL20_COMPILER_EXPR_COMPILE_TIME_VALUE)
   If expr->compile_time_value != NULL then:
      compile_time_value is valid for the lifetime of fastmem
      compile_time_value is a compile time instance of &primitiveTypes[PRIM_FLOAT]
         i.e. a const_float
*/

Expr* glsl_expr_construct_value_float(const_float v)
{
	const_float* compile_time_value;
	Expr* expr;

	expr = (Expr *)malloc_fast(sizeof(Expr));
	compile_time_value = (const_float *)malloc_fast(sizeof(const_float));

	expr->line_num = g_LineNumber;
	expr->flavour = EXPR_VALUE;
	expr->type = &primitiveTypes[PRIM_FLOAT];
	*compile_time_value = v;
	expr->compile_time_value = compile_time_value;
   expr->constant_index_expression = true;

	return expr;
}

/*
   Expr* glsl_expr_construct_value_bool(const_bool v)

   Khronos documentation:

   Expressions in the shading language are built from the following:
   - Constants of type bool, int, float, all vector types, and all matrix types.
   - ...

   Implementation notes:

   -

   Preconditions:

   -

   Postconditions:

   May raise the following errors:

   C0006: Out of memory

   Otherwise returns normally.
   Result is valid for the lifetime of fastmem

   Invariants preserved:

   expr->flavour is a valid ExprFlavour enum constant
   If expr->flavour==EXPR_VALUE then expr->type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE}
   expr->type is valid for the lifetime of fastmem
   expr->type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE,SYMBOL_ARRAY_TYPE}
   If expr->flavour==EXPR_VALUE then expr->compile_time_value != NULL

   (GL20_COMPILER_EXPR_COMPILE_TIME_VALUE)
   If expr->compile_time_value != NULL then:
      compile_time_value is valid for the lifetime of fastmem
      compile_time_value is a compile time instance of &primitiveTypes[PRIM_BOOL]
         i.e. a const_bool
*/

Expr* glsl_expr_construct_value_bool(const_bool v)
{
	const_bool* compile_time_value;
	Expr* expr;

	expr = (Expr *)malloc_fast(sizeof(Expr));
	compile_time_value = (const_bool *)malloc_fast(sizeof(const_bool));

	expr->line_num = g_LineNumber;
	expr->flavour = EXPR_VALUE;
	expr->type = &primitiveTypes[PRIM_BOOL];
	*compile_time_value = v;
	expr->compile_time_value = compile_time_value;
   expr->constant_index_expression = true;

	return expr;
}

/*
   Expr* glsl_expr_construct_instance(Symbol* symbol)

   Khronos documentation:

   Expressions in the shading language are built from the following:
   - Variable names of all types, except array names not followed by a subscript.
   - Unsubscripted array names. Unsubscripted arrays may only be used as actual parameters in
function calls or within parentheses. The only operator defined for arrays is the subscript operator
( [ ] ).
   - ...

   Implementation notes:

   -

   Preconditions:

   Symbol == NULL or Symbol is valid for the lifetime of fastmem

   Postconditions:

   May raise the following errors:

   C0006: Out of memory
   L0002: Undefined identifier
   C0001: Identifier is not a variable or parameter

   Otherwise returns normally.
   Result is valid for the lifetime of fastmem

   Invariants preserved:

   expr->flavour is a valid ExprFlavour enum constant
   expr->type is valid for the lifetime of fastmem
   expr->type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE,SYMBOL_ARRAY_TYPE}

   (GL20_COMPILER_EXPR_COMPILE_TIME_VALUE)
   If expr->compile_time_value != NULL then:
      expr->compile_time_value is valid for the lifetime of fastmem
      expr->compile_time_value is a compile time instance of expr->type
   TODO: should we add an invariant matching symbol->type up with our type?
*/

Expr* glsl_expr_construct_instance(Symbol* symbol)
{
	Expr* expr = (Expr *)malloc_fast(sizeof(Expr));
	expr->line_num = g_LineNumber;

   expr->flavour = EXPR_INSTANCE;

	if (!symbol)
	{
		// Unknown identifier.
		glsl_compile_error(ERROR_LEXER_PARSER, 2, g_LineNumber, NULL);
		return NULL;
	}

   switch (symbol->flavour)
   {
      case SYMBOL_VAR_INSTANCE:
			expr->compile_time_value = symbol->u.var_instance.compile_time_value;
         expr->constant_index_expression = symbol->u.var_instance.is_loop_index || symbol->u.var_instance.compile_time_value != NULL;
         break;
      case SYMBOL_PARAM_INSTANCE:
			expr->compile_time_value = NULL;
         expr->constant_index_expression = false;
         break;
      default:
         // Identifier is not var or param.
			glsl_compile_error(ERROR_CUSTOM, 1, g_LineNumber, NULL);
			return NULL;
   }

	expr->u.instance.symbol = symbol;
	expr->type = symbol->type;

	return expr;
}

/*
   bool glsl_expr_is_uniform(Expr *e)

   Returns whether an expression is a uniform, i.e. whether it is suitable for
   non-constant element indexing.

   TODO: should this function be static and renamed to something a bit more
   suggestive of what it's used for?

   Khronos documentation:

   -

   Implementation notes:

   -

   Preconditions:

   e is valid

   Postconditions:

   -

   Invariants preserved:

   -
*/

bool glsl_expr_is_uniform(Expr *e)
{
   STACK_CHECK();

	switch (e->flavour)
	{
      case EXPR_INSTANCE:
         return SYMBOL_VAR_INSTANCE == e->u.instance.symbol->flavour
             && TYPE_QUAL_UNIFORM == e->u.instance.symbol->u.var_instance.type_qual;

      case EXPR_SUBSCRIPT:
         return glsl_expr_is_uniform(e->u.subscript.aggregate);

      case EXPR_FIELD_SELECTOR_STRUCT:
         return glsl_expr_is_uniform(e->u.field_selector_struct.aggregate);

      default:
         return false;
	}
}

/*
   Expr* glsl_expr_construct_subscript(Expr* aggregate, Expr* subscript)

   Khronos documentation:

   Expressions in the shading language are built from the following:
   - Subscripted array names.
   - Component field selectors and array subscript results.
   - ...

   5.2 Array Subscripting
   Array elements are accessed using the array subscript operator ( [ ] ). This is the only operator that
   operates on arrays. An example of accessing an array element is
      diffuseColor += lightIntensity[3] * NdotL;

   Array subscripting syntax can also be applied to vectors to provide numeric indexing. So in
      vec4 pos;
   pos[2] refers to the third element of pos and is equivalent to pos.z. This allows variable indexing into a
   vector, as well as a generic way of accessing components. Any integer expression can be used as the
   subscript. The first component is at index zero. Reading from or writing to a vector using a constant
   integral expression with a value that is negative or greater than or equal to the size of the vector is illegal.
   When indexing with non-constant expressions, behavior is undefined if the index is negative or greater
   than or equal to the size of the vector.

   Limitations for ES 2.0

   5 Array Indexing
   Uniforms (excluding samplers)
   In the vertex shader, support for all forms of array indexing is mandated. In the fragment shader, support
   for indexing is only mandated for constant integral expressions.

   Samplers
   GLSL ES 1.00 supports both arrays of samplers and arrays of structures which contain samplers. In both
   these cases, for ES 2.0, support for indexing with a constant integral expression is mandated but support
   for indexing with other values is not mandated.

   Attributes
   Support for indexing of matrices and vectors with constant integral expressions is mandated.
   Support for indexing of matrices and vectors with other values is not mandated.
   Attribute arrays are disallowed by the specification.

   Varyings
   Support for indexing with a constant integral expression is mandated.
   Support for indexing with other values is not mandated.

   Variables
   Support for indexing with a constant integral expression is mandated.
   Support for indexing with other values is not mandated.

   Constants
   Support for indexing of matrices and vectors with constant integral expressions is mandated.
   Support for indexing of matrices and vectors with other values is not mandated.
   Constant arrays are disallowed by the specification.

   Summary
   The following array indexing functionality must be supported:
                           Vertex Shaders                   Fragment Shaders
   Uniforms                Any integer                      Constant integral expression
   Attributes              Constant integral expression     Not applicable
   (vectors and matrices)
   Varyings                Constant integral expression     Constant integral expression
   Samplers                Constant integral expression     Constant integral expression
   Variables               Constant integral expression     Constant integral expression
   Constants               Constant integral expression     Constant integral expression
   (vectors and matrices)


   Implementation notes:

   -

   Preconditions:

   aggregate is valid for the lifetime of fastmem
   subscript is valid for the lifetime of fastmem

   Postconditions:

   May raise the following errors:

   C0002: Subscript must be integral type
   C0013: Support for indexing array/vector/matrix with a non-constant is only mandated for non-sampler uniforms in the vertex shader
   C0014: Support for indexing array/vector/matrix with a non-constant is not mandated in the fragment shader
   C0006: Out of memory
   S0021: Indexing an array with a negative integral constant expression.
   S0020: Indexing an array with an integral constant expression greater than its declared size.
   C0003: Expression cannot be subscripted

   Otherwise returns normally.
   Result is valid for the lifetime of fastmem

   Invariants preserved:

   (GL20_COMPILER_EXPR_FLAVOUR)
   expr->flavour is a valid ExprFlavour enum constant

   (GL20_COMPILER_EXPR_TYPE)
   expr->type is valid for the lifetime of fastmem
   expr->type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE,SYMBOL_ARRAY_TYPE}

   (GL20_COMPILER_EXPR_COMPILE_TIME_VALUE)
   If expr->compile_time_value != NULL then:
      expr->compile_time_value is valid for the lifetime of fastmem
      expr->compile_time_value is a compile time instance of expr->type

   (GL20_COMPILER_EXPR_SUBSCRIPT)
   If expr->flavour == EXPR_SUBSCRIPT then
      expr->aggregate is valid for the lifetime of fastmem
      TODO: some invariant about expr->aggregate being a valid aggregate?
      expr->subscript is valid for the lifetime of fastmem
      expr->subscript->type == &primitiveTypes[PRIM_INT]
      If expr->subscript has a compile time value, this value does not exceed the range
      of the array/vector/matrix (TODO: formalise this)
*/
Expr* glsl_expr_construct_subscript(Expr* aggregate, Expr* subscript)
{
	Expr* expr;

	if (&primitiveTypes[PRIM_INT] != subscript->type)
	{
		// *subscript is not integral.
		glsl_compile_error(ERROR_CUSTOM, 2, g_LineNumber, NULL);
		return NULL;
	}

	// If the indexing is non-constant, can we get away with not supporting it?
   if (!subscript->constant_index_expression)
	{
		switch (g_ShaderFlavour)
		{
			case SHADER_VERTEX:
				// If it's a non-constant indexing of a uniform then
				// we have to support this in the vertex shader unless it's an array of samplers,
				// or an array of structs containing samplers.

            if (!glsl_expr_is_uniform(aggregate) || glsl_type_contains_samplers(aggregate->type)) {
				   // but we don't have to support this.
				   glsl_compile_error(ERROR_CUSTOM, 13, g_LineNumber, NULL);
				   return NULL;
            }
            break;

			case SHADER_FRAGMENT:
				// We don't need to support any non-constant indexing at all in the fragment shader.
				glsl_compile_error(ERROR_CUSTOM, 14, g_LineNumber, NULL);
				return NULL;
			default:
				UNREACHABLE();
				return NULL;
		}
	}

	expr = (Expr *)malloc_fast(sizeof(Expr));

	expr->line_num = g_LineNumber;
	expr->flavour = EXPR_SUBSCRIPT;
	expr->u.subscript.aggregate = aggregate;
	expr->u.subscript.subscript = subscript;

	if (SYMBOL_PRIMITIVE_TYPE == aggregate->type->flavour)
	{
		int aggregate_index = aggregate->type->u.primitive_type.index;
		expr->type = primitiveTypeSubscriptTypes[aggregate_index];
      
      expr->constant_index_expression = aggregate->constant_index_expression && subscript->constant_index_expression;

		if (expr->type)
		{
			// If the subscript is a constant...
			if (subscript->compile_time_value)
			{
				const_int subscript_value = *(const_int*)subscript->compile_time_value;

				// Bounds check.
				if (subscript_value < 0)
				{
					glsl_compile_error(ERROR_SEMANTIC, 21, g_LineNumber, NULL);
					return NULL;
				}
				else if (primitiveTypeSubscriptDimensions[aggregate_index] <= subscript_value)
				{
					glsl_compile_error(ERROR_SEMANTIC, 20, g_LineNumber, NULL);
					return NULL;
				}

				// If the aggregate is a constant...
				if (aggregate->compile_time_value)
				{
					// Set expression value.
					expr->compile_time_value = (char*)aggregate->compile_time_value + (primitiveTypeSubscriptTypes[aggregate_index]->size_as_const * subscript_value);
				}
				else
				{
					expr->compile_time_value = NULL;
				}
         } else {
				expr->compile_time_value = NULL;
         }

			return expr;
		}
	}
	else if (SYMBOL_ARRAY_TYPE == aggregate->type->flavour)
	{
		expr->type = aggregate->type->u.array_type.member_type;
		// Arrays can't have compile time values.
		expr->compile_time_value = NULL;
      expr->constant_index_expression = false;
		return expr;
	}

	// *aggregate cannot be subscripted.
	glsl_compile_error(ERROR_CUSTOM, 3, g_LineNumber, NULL);
	return NULL;
}

/*
   Expr* glsl_expr_construct_function_call(Symbol* overload_chain, ExprChain* args)

   Khronos documentation:

   5.3 Function Calls
   If a function returns a value, then a call to that function may be used as an expression, whose type will be
   the type that was used to declare or define the function.

   6.1 Function Definitions
   A function is called by using its name followed by a list of arguments in parentheses.
   Arrays are allowed as arguments, but not as the return type. When arrays are declared as formal
   parameters, their size must be included. An array is passed to a function by using the array name without
   any subscripting or brackets, and the size of the array argument passed in must match the size specified in
   the formal parameter declaration.
   Structures are also allowed as arguments. The return type can also be a structure if the structure does not
   contain an array.

   Function names can be overloaded. This allows the same function name to be used for multiple functions,
as long as the argument list types differ. If functions’ names and argument types match, then their return
type and parameter qualifiers must also match. Function signature matching is based on parameter type
only, no qualifiers are used. Overloading is used heavily in the built-in functions. When overloaded
functions (or indeed any functions) are resolved, an exact match for the function's signature is sought.
This includes exact match of array size as well. No promotion or demotion of the return type or input
argument types is done. All expected combination of inputs and outputs must be defined as separate
functions.



   Implementation notes:

   -

   Preconditions:

   overload_chain is null or valid for the lifetime of fastmem
   args is valid for the lifetime of fastmem

   Postconditions:

   May raise the following errors:

   C0006: Out of memory
   L0002: Undefined identifier
   C0009: Identifier is not a function name. Perhaps function is hidden by variable or type name?
	C0016: Argument cannot be used as out or inout parameter
   C0011: No declared overload matches function call arguments

   Otherwise returns normally.
   Result is valid for the lifetime of fastmem

   Invariants preserved:

   (GL20_COMPILER_EXPR_FLAVOUR)
   flavour is a valid StatementFlavour enum constant (not including STATEMENT_FLAVOUR_COUNT)

   (GL20_COMPILER_EXPR_FUNCTION_CALL)
   If expr->flavour == EXPR_FUNCTION_CALL then
      expr->function is valid for the lifetime of fastmem
      expr->function->flavour == GL20_COMPILER_SYMBOL_FUNCTION_INSTANCE_TYPE
      expr->args is valid for the lifetime of fastmem

   (GL20_COMPILER_EXPR_COMPILE_TIME_VALUE)
   If expr->compile_time_value != NULL then:
      expr->compile_time_value is valid for the lifetime of fastmem
      expr->compile_time_value is a compile time instance of expr->type

   Invariants used:
   (GL20_COMPILER_FUNCTIONTYPE_RETURN_TYPE)
   (GL20_COMPILER_SYMBOL_FUNCTION_INSTANCE_CONST_FUNCTION_CODE)
   (GL20_COMPILER_SYMBOL_FUNCTION_INSTANCE_CONST_FUNCTION_BODY)
   (GL20_COMPILER_EXPRCHAIN_ALL_COMPILE_TIME_VALUES)
   (GL20_COMPILER_EXPR_COMPILE_TIME_VALUE)
   (GL20_COMPILER_SYMBOLTYPE_SIZE_AS_CONST)
*/

Expr* glsl_expr_construct_function_call(Symbol* overload_chain, ExprChain* args)
{
	// Note that each function symbol points to the last function symbol
	// with the same name.
	// We create a prototype from the arg types and then traverse the chain
	// until we find the correct overload.

	ConstantFunction const_function_code;

	Expr* expr = (Expr *)malloc_fast(sizeof(Expr));

	expr->line_num = g_LineNumber;
	expr->flavour = EXPR_FUNCTION_CALL;
	expr->u.function_call.args = args;

	// Check that the identifier exists.
	if (!overload_chain)
	{
		// Undefined identifier.
		glsl_compile_error(ERROR_LEXER_PARSER, 2, g_LineNumber, NULL);
		return NULL;
	}
	// Check that the overload_chain is valid.
	if (SYMBOL_FUNCTION_INSTANCE != overload_chain->flavour)
	{
		// Identifier is not a function name.
		glsl_compile_error(ERROR_CUSTOM, 9, g_LineNumber, NULL);
		return NULL;
	}

	// Resolve overloads.
	expr->u.function_call.function = glsl_resolve_overload_using_arguments(overload_chain, args);
	if (!expr->u.function_call.function)
	{
		// No good overload.
		glsl_compile_error(ERROR_CUSTOM, 11, g_LineNumber, NULL);
		return NULL;
	}

	// Set type from return type.
	expr->type = expr->u.function_call.function->type->u.function_type.return_type;

   const_function_code = expr->u.function_call.function->u.function_instance.const_function_code;

	// Work out compile time value if possible.
	expr->compile_time_value = NULL;
   expr->constant_index_expression = const_function_code != NULL && args->all_constant_index_expression;
	if (const_function_code)
	{
      // mark function as seen, requiring us to append the implementation to the shader
#if defined(__CC_ARM)
#ifdef ANDROID
      expr->u.function_call.function->u.function_instance.const_function_body = (const char *)((size_t)expr->u.function_call.function->u.function_instance.const_function_body | 0x40000000);
#else
      expr->u.function_call.function->u.function_instance.const_function_body = (const char *)((size_t)expr->u.function_call.function->u.function_instance.const_function_body | 0x20000000);
#endif
#else
      expr->u.function_call.function->u.function_instance.const_function_body = (const char *)((size_t)expr->u.function_call.function->u.function_instance.const_function_body | 0x80000000);
#endif
   	if (args->all_compile_time_values)
		{
			expr->compile_time_value = malloc_fast(expr->type->size_as_const);

			switch (expr->u.function_call.function->type->u.function_type.param_count)
			{
				case 1:
					((void (*)(void*, void*))const_function_code)
						(
							expr->compile_time_value, // result
							args->first->expr->compile_time_value // argument 1
						);
					break;
				case 2:
					((void (*)(void*, void*, void*))const_function_code)
						(
							expr->compile_time_value, // result
							args->first->expr->compile_time_value, // argument 1
							args->first->next->expr->compile_time_value // argument 2
						);
					break;
				case 3:
					((void (*)(void*, void*, void*, void*))const_function_code)
						(
							expr->compile_time_value, // result
							args->first->expr->compile_time_value, // argument 1
							args->first->next->expr->compile_time_value, // argument 2
							args->first->next->next->expr->compile_time_value // argument 3
						);
					break;
				case 4:
					((void (*)(void*, void*, void*, void*, void*))const_function_code)
						(
							expr->compile_time_value, // result
							args->first->expr->compile_time_value, // argument 1
							args->first->next->expr->compile_time_value, // argument 2
							args->first->next->next->expr->compile_time_value, // argument 3
							args->first->next->next->next->expr->compile_time_value // argument 4
						);
					break;
				default:
					// No built in functions take more than 4 arguments.
					UNREACHABLE();
					return NULL;
			}
		}
	}

	return expr;
}

/*
   Expr* glsl_expr_construct_prim_constructor_call(PrimitiveTypeIndex return_index, ExprChain* args)

   Khronos documentation:

   5.4 Constructors
Constructors use the function call syntax, where the function name is a basic-type keyword or structure
name, to make values of the desired type for use in an initializer or an expression. (See Section 9
“Shading Language Grammar” for details.) The parameters are used to initialize the constructed value.
Constructors can be used to request a data type conversion to change from one scalar type to another
scalar type, or to build larger types out of smaller types, or to reduce a larger type to a smaller type.
There is no fixed list of constructor prototypes. Constructors are not built-in functions. Syntactically, all
lexically correct parameter lists are valid. Semantically, the number of parameters must be of sufficient
size and correct type to perform the initialization. Unsubscripted arrays and structures containing arrays
are not allowed as parameters to constructors. Arguments are evaluated left to right. It is an error to
include so many arguments to a constructor that they cannot all be used. Detailed rules follow. The
prototypes actually listed below are merely a subset of examples.
5.4.1 Conversion and Scalar Constructors
Converting between scalar types is done as the following prototypes indicate:
int(bool) // converts a Boolean value to an int
int(float) // converts a float value to an int
float(bool) // converts a Boolean value to a float
float(int) // converts an integer value to a float
bool(float) // converts a float value to a Boolean
bool(int) // converts an integer value to a Boolean
When constructors are used to convert a float to an int, the fractional part of the floating-point value is
dropped.
When a constructor is used to convert an int or a float to bool, 0 and 0.0 are converted to false, and nonzero
values are converted to true. When a constructor is used to convert a bool to an int or float, false is
converted to 0 or 0.0, and true is converted to 1 or 1.0.
Identity constructors, like float(float) are also legal, but of little use.
Scalar constructors with non-scalar parameters can be used to take the first element from a non-scalar.
For example, the constructor float(vec3) will select the first component of the vec3 parameter.

5.4.2 Vector and Matrix Constructors
Constructors can be used to create vectors or matrices from a set of scalars, vectors, or matrices. This
includes the ability to shorten vectors.
If there is a single scalar parameter to a vector constructor, it is used to initialize all components of the
constructed vector to that scalar’s value. If there is a single scalar parameter to a matrix constructor, it is
used to initialize all the components on the matrix’s diagonal, with the remaining components initialized
to 0.0.
If a vector is constructed from multiple scalars, vectors, or matrices, or a mixture of these, the vectors'
components will be constructed in order from the components of the arguments. The arguments will be
consumed left to right, and each argument will have all its components consumed, in order, before any
components from the next argument are consumed. Similarly for constructing a matrix from multiple
scalars or vectors, or a mixture of these. Matrix components will be constructed and consumed in column
major order. In these cases, there must be enough components provided in the arguments to provide an
initializer for every component in the constructed value. It is an error to provide extra arguments beyond
this last used argument.
If a matrix is constructed from a matrix, then each component (column i, row j) in the result that has a
corresponding component (column i, row j) in the argument will be initialized from there. All other
components will be initialized to the identity matrix. If a matrix argument is given to a matrix constructor,
it is an error to have any other arguments.
If the basic type (bool, int, or float) of a parameter to a constructor does not match the basic type of the
object being constructed, the scalar construction rules (above) are used to convert the parameters.
Some useful vector constructors are as follows:
vec3(float) // initializes each component of with the float
vec4(ivec4) // makes a vec4 with component-wise conversion
vec2(float, float) // initializes a vec2 with 2 floats
ivec3(int, int, int) // initializes an ivec3 with 3 ints
bvec4(int, int, float, float) // uses 4 Boolean conversions
vec2(vec3) // drops the third component of a vec3
vec3(vec4) // drops the fourth component of a vec4
vec3(vec2, float) // vec3.x = vec2.x, vec3.y = vec2.y, vec3.z = float
vec3(float, vec2) // vec3.x = float, vec3.y = vec2.x, vec3.z = vec2.y
vec4(vec3, float)
vec4(float, vec3)
vec4(vec2, vec2)
Some examples of these are:
vec4 color = vec4(0.0, 1.0, 0.0, 1.0);
vec4 rgba = vec4(1.0); // sets each component to 1.0
vec3 rgb = vec3(color); // drop the 4th component
To initialize the diagonal of a matrix with all other elements set to zero:
mat2(float)
mat3(float)
mat4(float)
To initialize a matrix by specifying vectors, or by all 4, 9, or 16 floats for mat2, mat3 and mat4
respectively. The floats are assigned to elements in column major order.
mat2(vec2, vec2);
mat3(vec3, vec3, vec3);
mat4(vec4, vec4, vec4, vec4);
mat2(float, float,
float, float);
mat3(float, float, float,
float, float, float,
float, float, float);
mat4(float, float, float, float,
float, float, float, float,
float, float, float, float,
float, float, float, float);
A wide range of other possibilities exist, as long as enough components are present to initialize the matrix.

5.4.3 Structure Constructors
Once a structure is defined, and its type is given a name, a constructor is available with the same name to
construct instances of that structure. For example:
struct light {
float intensity;
vec3 position;
};
light lightVar = light(3.0, vec3(1.0, 2.0, 3.0));
The arguments to the constructor must be in the same order and of the same type as they were declared in
the structure.
Structure constructors can be used as initializers or in expressions.

   Implementation notes:

	Complete list of valid constructors, deduced from spec (1.00 rev 14 p40):
	1. scalar(scalar)							convert
	2. scalar(vector)							convert first component
	3. scalar(matrix)							convert first component
	4. vector(scalar)							convert scalar and fill vector
	5. matrix(scalar)							convert scalar and fill matrix diagonal (rest is zero)
	6. matrix(matrix)							convert component-wise; truncate, or pad with identity
	7. vector(scalars, vectors, matrices, ...)	consume and convert components
												- error if insufficient components
												- error if not all arguments are needed
												- no error if there are some spare components, but all arguments were needed
	8. matrix(scalars, vectors, ...)				consume and convert components
												- error if insufficient components
												- error if not all arguments are needed
												- no error if there are some spare components, but all arguments were needed


   Preconditions:

   0 <= return_index < PRIMITIVE_TYPES_COUNT
   primitiveTypeFlags[return_index] & (PRIM_SCALAR_TYPE|PRIM_VECTOR_TYPE|PRIM_MATRIX_TYPE)
   args is valid for the lifetime of fastmem

   Postconditions:

   May raise the following errors:

   C0006: Out of memory
   S0009: Too few arguments for constructor
   S0007: Wrong arguments for constructor
   S0008: Argument unused in constructor
   S0009: Too few arguments for constructor

   Otherwise returns normally.
   Result is valid for the lifetime of fastmem

   Invariants preserved:

   (GL20_COMPILER_EXPR_TYPE)
   expr->type is valid for the lifetime of fastmem
   expr->type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE,SYMBOL_ARRAY_TYPE}

   (GL20_COMPILER_EXPR_FLAVOUR)
   expr->flavour is a valid ExprFlavour enum constant (not including EXPR_FLAVOUR_COUNT)

   (GL20_PRIM_CONSTRUCTOR_CALL)
   expr->u.prim_constructor_call.args is valid for the lifetime of fastmem

   (GL20_COMPILER_EXPR_COMPILE_TIME_VALUE)
   If compile_time_value != NULL then:
      compile_time_value is valid for the lifetime of fastmem
      compile_time_value is a compile time instance of type

   Invariants used:
   (GL20_COMPILER_PRIMITIVE_TYPES)
   (GL20_COMPILER_PRIMITIVETYPE_INDEX)
   (GL20_COMPILER_PRIMITIVE_TYPE_FLAGS)
   (GL20_COMPILER_EXPRCHAIN_FIRST)
   (GL20_COMPILER_EXPRCHAIN_ALL_PRIMITIVE_TYPES)
   (GL20_COMPILER_EXPRCHAIN_ALL_COMPILE_TIME_VALUES)
   (GL20_COMPILER_PRIMITIVE_TYPE_SUBSCRIPT_DIMENSIONS)
*/

Expr* glsl_expr_construct_prim_constructor_call(PrimitiveTypeIndex return_index, ExprChain* args)
{
	PRIMITIVE_TYPE_FLAGS_T return_flags, arg_flags;
	PrimitiveTypeIndex arg_index;
	ExprChainNode* arg;
	void* dst;
	void* src;
	Expr* expr = (Expr *)malloc_fast(sizeof(Expr));

	expr->line_num = g_LineNumber;
	expr->type = &primitiveTypes[return_index];
	expr->flavour = EXPR_PRIM_CONSTRUCTOR_CALL;
	expr->u.prim_constructor_call.flavour = PRIM_CONS_INVALID;
	expr->u.prim_constructor_call.args = args;
	expr->compile_time_value = NULL;

	return_flags = primitiveTypeFlags[return_index];

	// Quick checks.
	if (!args || args->count <= 0)
	{
		// Too few args.
		glsl_compile_error(ERROR_SEMANTIC, 9, g_LineNumber, NULL);
		return NULL;
	}
	if (!args->all_primitive_types)
	{
		// Wrong args.
		glsl_compile_error(ERROR_SEMANTIC, 7, g_LineNumber, NULL);
		return NULL;
	}

	// If we can decide a value at compile time, allocate space.
	if (args->all_compile_time_values)
	{
		expr->compile_time_value = malloc_fast(primitiveTypes[return_index].size_as_const);
	}

   expr->constant_index_expression = args->all_constant_index_expression;

	// Peek at first argument.
	arg = args->first;
	arg_index = arg->expr->type->u.primitive_type.index;
	arg_flags = primitiveTypeFlags[arg_index];


	// 1. scalar(scalar)							convert
	// 2. scalar(vector)							convert first component
	// 3. scalar(matrix)							convert first component
	if (return_flags & PRIM_SCALAR_TYPE)
	{
		// Ensure that there is only one argument.
		if (args->count == 1)
		{
         expr->u.prim_constructor_call.flavour = PRIM_CONS_SCALAR_FROM_FIRST_COMPONENT;

			dst = args->all_compile_time_values ? expr->compile_time_value : NULL;
			src = args->all_compile_time_values ? arg->expr->compile_time_value : NULL;

			// Check conversion and copy values if possible.
			if (!glsl_single_scalar_type_conversion(arg_index, &src, return_index, &dst))
			{
				// Wrong args.
				glsl_compile_error(ERROR_SEMANTIC, 7, g_LineNumber, NULL);
				return NULL;
			}
		}
		else
		{
			// Too many args.
			glsl_compile_error(ERROR_SEMANTIC, 8, g_LineNumber, NULL);
			return NULL;
		}
	}
	// 4. vector(scalar)							convert scalar and fill vector
	else if ((return_flags & PRIM_VECTOR_TYPE) && args->count == 1 && (arg_flags & PRIM_SCALAR_TYPE))
	{
		int return_dimension, j;

      expr->u.prim_constructor_call.flavour = PRIM_CONS_VECTOR_FROM_SCALAR;

		return_dimension = primitiveTypeSubscriptDimensions[return_index];

		dst = args->all_compile_time_values ? expr->compile_time_value : NULL;

		for (j = 0; j < return_dimension; j++)
		{
			src = args->all_compile_time_values ? arg->expr->compile_time_value : NULL;

			// Convert scalar.
			if (!glsl_single_scalar_type_conversion(arg_index, &src, return_index, &dst))
			{
				// Wrong args.
				glsl_compile_error(ERROR_SEMANTIC, 7, g_LineNumber, NULL);
				return NULL;
			}
		}
	}
	// 5. matrix(scalar)							convert scalar and fill matrix diagonal (rest is zero)
	else if ((return_flags & PRIM_MATRIX_TYPE) && args->count == 1 && (arg_flags & PRIM_SCALAR_TYPE))
	{
		int return_dimension, i, j;

      expr->u.prim_constructor_call.flavour = PRIM_CONS_MATRIX_FROM_SCALAR;

		return_dimension = primitiveTypeSubscriptDimensions[return_index];

		dst = args->all_compile_time_values ? expr->compile_time_value : NULL;

		for (i = 0; i < return_dimension; i++)
		{
			for (j = 0; j < return_dimension; j++)
			{
				if (i == j)
				{
					src = args->all_compile_time_values ? arg->expr->compile_time_value : NULL;

					// Convert scalar.
					if (!glsl_single_scalar_type_conversion(arg_index, &src, return_index, &dst))
					{
						// Wrong args.
						glsl_compile_error(ERROR_SEMANTIC, 7, g_LineNumber, NULL);
						return NULL;
					}
				}
				else
				{
					if (dst) {
					   *(const_float *)dst = CONST_FLOAT_ZERO;
					   dst = (const_float *)dst + 1;
					}
				}
			}
		}
	}
	// 6. matrix(matrix)							convert component-wise; truncate, or pad with identity
	else if ((return_flags & PRIM_MATRIX_TYPE) && args->count == 1 && (arg_flags & PRIM_MATRIX_TYPE))
	{
		int arg_dimension, return_dimension, i, j;

      expr->u.prim_constructor_call.flavour = PRIM_CONS_MATRIX_FROM_MATRIX;

		arg_dimension = primitiveTypeSubscriptDimensions[arg_index];
		return_dimension = primitiveTypeSubscriptDimensions[return_index];

		dst = args->all_compile_time_values ? expr->compile_time_value : NULL;

		for (i = 0; i < return_dimension; i++)
		{
			for (j = 0; j < return_dimension; j++)
			{
				// Set reader to m[i][j] where i selects columns.
				src = args->all_compile_time_values ?
					((const_float*)arg->expr->compile_time_value + (arg_dimension * i + j))
					:
					NULL;

				if (i < arg_dimension && j < arg_dimension)
				{
					// Consume next component.
					if (!glsl_single_scalar_type_conversion(arg_index, &src, return_index, &dst))
					{
						// Wrong args.
						glsl_compile_error(ERROR_SEMANTIC, 7, g_LineNumber, NULL);
						return NULL;
					}
				}
				else
				{
					if (dst) {
					   *(const_float *)dst = (i == j) ? CONST_FLOAT_ONE : CONST_FLOAT_ZERO;
                  dst = (const_float *)dst + 1;
               }
				}
			}
		}
	}
	// 7. vector(scalars, vectors, matrices, ...)	consume and convert components
	//												- error if insufficient components
	//												- error if not all arguments are needed
	//												- no error if there are some spare components, but all arguments were needed
	// 8. matrix(scalars, vectors, ...)				consume and convert components
	//												- error if insufficient components
	//												- error if not all arguments are needed
	//												- no error if there are some spare components, but all arguments were needed
	else if (return_flags & (PRIM_VECTOR_TYPE | PRIM_MATRIX_TYPE))
	{
		int return_components_left, arg_components_left;

      expr->u.prim_constructor_call.flavour = PRIM_CONS_VECTOR_OR_MATRIX_FROM_COMPONENT_FLOW;

      return_components_left = primitiveTypes[return_index].scalar_count;
		dst = args->all_compile_time_values ? expr->compile_time_value : NULL;

		while (arg)
		{
			arg_index = arg->expr->type->u.primitive_type.index;
         arg_components_left = primitiveTypes[arg_index].scalar_count;
			arg_flags = primitiveTypeFlags[arg_index];
			src = args->all_compile_time_values ? arg->expr->compile_time_value : NULL;

			// Ensure that matrix constructors reject matrix arguments.
			if ((return_flags & PRIM_MATRIX_TYPE) && (arg_flags & PRIM_MATRIX_TYPE))
			{
				// Wrong args.
				glsl_compile_error(ERROR_SEMANTIC, 7, g_LineNumber, NULL);
				return NULL;
			}

			for ( ; arg_components_left && return_components_left; arg_components_left--, return_components_left--)
			{
				// Consume next component.
				if (!glsl_single_scalar_type_conversion(arg_index, &src, return_index, &dst))
				{
					// Wrong args.
					glsl_compile_error(ERROR_SEMANTIC, 7, g_LineNumber, NULL);
					return NULL;
				}
			}

			// Now either we've run out of components in this arg, or run out of components to fill.
			// Either way, get the next arg.
			arg = arg->next;

			// If we ran out of components to fill...
			if (0 == return_components_left)
			{
				// We filled all components, but if there's another arg, it was wasted.
				if (arg)
				{
					// Too many args.
					glsl_compile_error(ERROR_SEMANTIC, 8, g_LineNumber, NULL);
					return NULL;
				}
			}
		}

		if (return_components_left)
		{
			// Too few args.
			glsl_compile_error(ERROR_SEMANTIC, 9, g_LineNumber, NULL);
			return NULL;
		}
	}

   vcos_assert(PRIM_CONS_INVALID != expr->u.prim_constructor_call.flavour);

	return expr;
}

/*
   Expr* glsl_expr_construct_type_constructor_call(SymbolType* type, ExprChain* args)

   Khronos documentation:

   5.4.3 Structure Constructors
   Once a structure is defined, and its type is given a name, a constructor is available with the same name to
   construct instances of that structure. For example:
   struct light {
   float intensity;
   vec3 position;
   };
   light lightVar = light(3.0, vec3(1.0, 2.0, 3.0));
   The arguments to the constructor must be in the same order and of the same type as they were declared in
   the structure.
   Structure constructors can be used as initializers or in expressions.

   Preconditions:

   type is valid for the lifetime of fastmem
   args is valid for the lifetime of fastmem
   type->flavour == SYMBOL_STRUCT_TYPE

   Postconditions:

   May raise the following errors:

   C0006: Out of memory
   S0009: Too few arguments for constructor
   S0008: Argument unused in constructor
   S0007: Wrong arguments for constructor

   Otherwise returns normally.
   Result is valid for the lifetime of fastmem

   Invariants preserved:

   (GL20_COMPILER_EXPR_TYPE)
   expr->type is valid for the lifetime of fastmem
   expr->type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE,SYMBOL_ARRAY_TYPE}

   (GL20_COMPILER_EXPR_FLAVOUR)
   expr->flavour is a valid ExprFlavour enum constant (not including EXPR_FLAVOUR_COUNT)

   (GL20_COMPILER_EXPR_COMPILE_TIME_VALUE)
   If compile_time_value != NULL then:
      compile_time_value is valid for the lifetime of fastmem
      compile_time_value is a compile time instance of type

   Invariants used:
   (GL20_COMPILER_SYMBOLTYPE_SIZE_STRUCTTYPE)
*/

Expr* glsl_expr_construct_type_constructor_call(SymbolType* type, ExprChain* args)
{
	ExprChainNode* arg;
	void* dst;
	int i;

	Expr* expr = (Expr *)malloc_fast(sizeof(Expr));

	vcos_assert(SYMBOL_STRUCT_TYPE == type->flavour);
	expr->line_num = g_LineNumber;
	expr->type = type;
	expr->flavour = EXPR_TYPE_CONSTRUCTOR_CALL;
	expr->u.type_constructor_call.args = args;
	expr->compile_time_value = NULL;
   expr->constant_index_expression = args->all_constant_index_expression;

	// Quick checks.
	if (!args || args->count <= 0)
	{
		// Too few args.
		glsl_compile_error(ERROR_SEMANTIC, 9, g_LineNumber, NULL);
		return NULL;
	}

	// If we can decide a value at compile time, allocate space.
	if (args->all_compile_time_values)
	{
		expr->compile_time_value = malloc_fast(type->size_as_const);
	}

	dst = expr->compile_time_value;

	// Match the types of all arguments, copying values if necessary.
	for (i = 0, arg = args->first; arg; i++, arg = arg->next)
	{
		// Check we have not run out of struct members.
		if (i >= type->u.struct_type.member_count)
		{
			// Too many args.
			glsl_compile_error(ERROR_SEMANTIC, 8, g_LineNumber, NULL);
			return NULL;
		}

		// Match types.
		if (!glsl_shallow_match_nonfunction_types(arg->expr->type, type->u.struct_type.member_types[i]))
		{
			// Wrong args.
			glsl_compile_error(ERROR_SEMANTIC, 7, g_LineNumber, NULL);
			return NULL;
		}

		if (dst)
		{
			// Copy values.
			memcpy(dst, arg->expr->compile_time_value, arg->expr->type->size_as_const);
			dst = (char *)dst + arg->expr->type->size_as_const;
		}
	}

	// Check we've assigned all struct members.
	if (i < type->u.struct_type.member_count)
	{
		// Too few args.
		glsl_compile_error(ERROR_SEMANTIC, 9, g_LineNumber, NULL);
		return NULL;
	}

	return expr;
}

Expr* glsl_expr_construct_field_selector(Expr* aggregate, const char* field)
{
	int i, field_offset, len, component_size_as_const, seen_xyzw, seen_rgba, seen_stpq;
	const char* swizzle_slot;
	PRIMITIVE_TYPE_FLAGS_T primitive_flags;
	Expr* expr = (Expr *)malloc_fast(sizeof(Expr));

	expr->line_num = g_LineNumber;
   expr->constant_index_expression = aggregate->constant_index_expression;

	switch (aggregate->type->flavour)
	{
		case SYMBOL_STRUCT_TYPE:
			expr->flavour = EXPR_FIELD_SELECTOR_STRUCT;
			expr->u.field_selector_struct.aggregate = aggregate;
			expr->u.field_selector_struct.field = field;

			// Work out field number, and value if possible.
			for (i = 0, field_offset = 0; i < aggregate->type->u.struct_type.member_count; i++)
			{
				if (0 == strcmp(aggregate->type->u.struct_type.member_names[i], field))
				{
					expr->u.field_selector_struct.field_no = i;
					expr->type = aggregate->type->u.struct_type.member_types[i];

					if (aggregate->compile_time_value)
					{
						expr->compile_time_value = (char*)aggregate->compile_time_value + field_offset;
					}
					else
					{
						expr->compile_time_value = NULL;
					}

					return expr;
				}

				field_offset += aggregate->type->u.struct_type.member_types[i]->size_as_const;
			}
			break; // fail
		case SYMBOL_PRIMITIVE_TYPE:
			expr->flavour = EXPR_FIELD_SELECTOR_SWIZZLE;
			expr->u.field_selector_swizzle.aggregate = aggregate;
			expr->u.field_selector_swizzle.field = field;

			primitive_flags = primitiveTypeFlags[aggregate->type->u.primitive_type.index];

			if (!(primitive_flags & PRIM_VECTOR_TYPE))
			{
				break; // fail
			}

			// Decode swizzle into swizzle_slots[].
			seen_xyzw = seen_rgba = seen_stpq = 0;
			for (swizzle_slot = field, i = 0; '\0' != *swizzle_slot; swizzle_slot++, i++)
			{
				if (i >= MAX_SWIZZLE_FIELD_COUNT)
				{
					// Too long.
					// Illegal field selector.
					glsl_compile_error(ERROR_SEMANTIC, 26, g_LineNumber, NULL);
					return NULL;
				}
				switch (*swizzle_slot)
				{
					case 'x':
						seen_xyzw++;
						expr->u.field_selector_swizzle.swizzle_slots[i] = SWIZZLE_FIELD_XYZW_X;
						break;
					case 'y':
						seen_xyzw++;
						expr->u.field_selector_swizzle.swizzle_slots[i] = SWIZZLE_FIELD_XYZW_Y;
						break;
					case 'z':
						seen_xyzw++;
						expr->u.field_selector_swizzle.swizzle_slots[i] = SWIZZLE_FIELD_XYZW_Z;
						break;
					case 'w':
						seen_xyzw++;
						expr->u.field_selector_swizzle.swizzle_slots[i] = SWIZZLE_FIELD_XYZW_W;
						break;

					case 'r':
						seen_rgba++;
						expr->u.field_selector_swizzle.swizzle_slots[i] = SWIZZLE_FIELD_RGBA_R;
						break;
					case 'g':
						seen_rgba++;
						expr->u.field_selector_swizzle.swizzle_slots[i] = SWIZZLE_FIELD_RGBA_G;
						break;
					case 'b':
						seen_rgba++;
						expr->u.field_selector_swizzle.swizzle_slots[i] = SWIZZLE_FIELD_RGBA_B;
						break;
					case 'a':
						seen_rgba++;
						expr->u.field_selector_swizzle.swizzle_slots[i] = SWIZZLE_FIELD_RGBA_A;
						break;

					case 's':
						seen_stpq++;
						expr->u.field_selector_swizzle.swizzle_slots[i] = SWIZZLE_FIELD_STPQ_S;
						break;
					case 't':
						seen_stpq++;
						expr->u.field_selector_swizzle.swizzle_slots[i] = SWIZZLE_FIELD_STPQ_T;
						break;
					case 'p':
						seen_stpq++;
						expr->u.field_selector_swizzle.swizzle_slots[i] = SWIZZLE_FIELD_STPQ_P;
						break;
					case 'q':
						seen_stpq++;
						expr->u.field_selector_swizzle.swizzle_slots[i] = SWIZZLE_FIELD_STPQ_Q;
						break;

					default:
						// Illegal field selector.
						glsl_compile_error(ERROR_SEMANTIC, 26, g_LineNumber, NULL);
						return NULL;
				}
            if (expr->u.field_selector_swizzle.swizzle_slots[i] >= primitiveTypes[aggregate->type->u.primitive_type.index].scalar_count)
				{
					// The aggregate does not have the desired member, e.g. 'a' from a vec2.
					// Illegal field selector.
					glsl_compile_error(ERROR_SEMANTIC, 26, g_LineNumber, NULL);
					return NULL;
				}
			}

			// Save length.
			len = i;
			if ((seen_xyzw != len) && (seen_rgba != len) && (seen_stpq != len))
			{
				// Field selectors must be from same set.
				glsl_compile_error(ERROR_SEMANTIC, 25, g_LineNumber, NULL);
				return NULL;
			}

			// Mark unused slots in swizzle_slots[].
			for ( ; i < MAX_SWIZZLE_FIELD_COUNT; i++)
			{
				expr->u.field_selector_swizzle.swizzle_slots[i] = SWIZZLE_SLOT_UNUSED;
			}

			// Work out type of swizzle.
			switch (primitive_flags & (PRIM_BOOL_TYPE | PRIM_INT_TYPE | PRIM_FLOAT_TYPE))
			{
				case PRIM_BOOL_TYPE:
					component_size_as_const = primitiveTypes[PRIM_BOOL].size_as_const;
					switch (len)
					{
						case 1:
							expr->type = &primitiveTypes[PRIM_BOOL];
							break;
						case 2:
							expr->type = &primitiveTypes[PRIM_BVEC2];
							break;
						case 3:
							expr->type = &primitiveTypes[PRIM_BVEC3];
							break;
						case 4:
							expr->type = &primitiveTypes[PRIM_BVEC4];
							break;
						default:
							vcos_assert(4 == MAX_SWIZZLE_FIELD_COUNT);
							return NULL;
					}
					break;
				case PRIM_INT_TYPE:
					component_size_as_const = primitiveTypes[PRIM_INT].size_as_const;
					switch (len)
					{
						case 1:
							expr->type = &primitiveTypes[PRIM_INT];
							break;
						case 2:
							expr->type = &primitiveTypes[PRIM_IVEC2];
							break;
						case 3:
							expr->type = &primitiveTypes[PRIM_IVEC3];
							break;
						case 4:
							expr->type = &primitiveTypes[PRIM_IVEC4];
							break;
						default:
							vcos_assert(4 == MAX_SWIZZLE_FIELD_COUNT);
							return NULL;
					}
					break;
				case PRIM_FLOAT_TYPE:
					component_size_as_const = primitiveTypes[PRIM_FLOAT].size_as_const;
					switch (len)
					{
						case 1:
							expr->type = &primitiveTypes[PRIM_FLOAT];
							break;
						case 2:
							expr->type = &primitiveTypes[PRIM_VEC2];
							break;
						case 3:
							expr->type = &primitiveTypes[PRIM_VEC3];
							break;
						case 4:
							expr->type = &primitiveTypes[PRIM_VEC4];
							break;
						default:
							vcos_assert(4 == MAX_SWIZZLE_FIELD_COUNT);
							return NULL;
					}
					break;
				default:
					// No other types.
					UNREACHABLE();
					return NULL;
			}

			// Work out value, if possible.
			if (aggregate->compile_time_value)
			{
				expr->compile_time_value = malloc_fast(expr->type->size_as_const);

				for (i = 0; i < len; i++)
				{
					void* dst;
					void* src;
					dst = (char*)expr->compile_time_value + (component_size_as_const * i);
					src = (char*)aggregate->compile_time_value + (component_size_as_const * expr->u.field_selector_swizzle.swizzle_slots[i]);
					memcpy(dst, src, component_size_as_const);
				}
			}
			else
			{
				expr->compile_time_value = NULL;
			}

			return expr;
		default:
			break; // fail
	}

	// Illegal field selector.
	glsl_compile_error(ERROR_SEMANTIC, 26, g_LineNumber, NULL);
	return NULL;
}

Expr* glsl_expr_construct_unary_op_arithmetic(ExprFlavour flavour, Expr* operand)
{
	Expr* expr = (Expr *)malloc_fast(sizeof(Expr));
	expr->line_num = g_LineNumber;
	expr->flavour = flavour;
	expr->u.unary_op.operand = operand;

	// EXPR_POST_INC, EXPR_POST_DEC, EXPR_PRE_INC, EXPR_PRE_DEC cannot be applied to constants.
	expr->compile_time_value = NULL;

	if (SYMBOL_PRIMITIVE_TYPE == operand->type->flavour)
	{
		int prim_index = operand->type->u.primitive_type.index;

		if (primitiveTypeFlags[prim_index] & PRIM_ARITHMETIC_UNARY)
		{
			expr->type = operand->type;

			switch (flavour)
			{
				case EXPR_POST_INC:
				case EXPR_POST_DEC:
				case EXPR_PRE_INC:
				case EXPR_PRE_DEC:
               expr->constant_index_expression = false;
					if (!(glsl_get_lvalue_flags(operand) & LVF_ASSIGNMENT_TARGET))
					{
						// Not an l-value.
						glsl_compile_error(ERROR_CUSTOM, 17, g_LineNumber, NULL);
						return NULL;
					}
					break;

				case EXPR_ARITH_NEGATE:
               expr->constant_index_expression = operand->constant_index_expression;
					// Set value if possible.
					if (operand->compile_time_value)
					{
						int i, n;
						void* src;
						void* dst;

						expr->compile_time_value = malloc_fast(expr->type->size_as_const);

						src = operand->compile_time_value;
						dst = expr->compile_time_value;

                  n = primitiveTypes[prim_index].scalar_count;

						switch (primitiveTypeFlags[prim_index] & (PRIM_INT_TYPE | PRIM_FLOAT_TYPE))
						{
							case PRIM_INT_TYPE:
								for (i = 0; i < n; i++)
								{
									op_arith_negate__const_int__const_int((const_int *)dst, (const_int *)src);
									src = (const_int *)src + 1;
									dst = (const_int *)dst + 1;
								}
								break;
							case PRIM_FLOAT_TYPE:
								for (i = 0; i < n; i++)
								{
									op_arith_negate__const_float__const_float((const_float *)dst, (const_float *)src);
									src = (const_float *)src + 1;
									dst = (const_float *)dst + 1;
								}
								break;
							default:
								UNREACHABLE();
								return NULL;
						}
					}
					break;

				default:
					UNREACHABLE();
					return NULL;
			}

			return expr;
		}
	}

	// Operator not supported.
	glsl_compile_error(ERROR_SEMANTIC, 4, g_LineNumber, NULL);
	return NULL;
}

Expr* glsl_expr_construct_unary_op_logical(ExprFlavour flavour, Expr* operand)
{
	Expr* expr = (Expr *)malloc_fast(sizeof(Expr));
	expr->line_num = g_LineNumber;
	expr->flavour = flavour;
	expr->u.unary_op.operand = operand;

	expr->compile_time_value = NULL;
   expr->constant_index_expression = operand->constant_index_expression;

	if (SYMBOL_PRIMITIVE_TYPE == operand->type->flavour)
	{
		int prim_index = operand->type->u.primitive_type.index;

		if (primitiveTypeFlags[prim_index] & PRIM_LOGICAL_UNARY)
		{
			expr->type = operand->type;

			// Set value if possible.
			if (operand->compile_time_value)
			{
				expr->compile_time_value = malloc_fast(expr->type->size_as_const);

				// This code assumes we do not act on vectors.
				vcos_assert(PRIM_BOOL == prim_index);

				// and assumes that logical not is the only operator in this category.
				vcos_assert(EXPR_LOGICAL_NOT == flavour);

				op_logical_not__const_bool__const_bool((const_bool *)expr->compile_time_value, (const_bool *)operand->compile_time_value);
			}

			return expr;
		}
	}

	// Operator not supported.
	glsl_compile_error(ERROR_SEMANTIC, 4, g_LineNumber, NULL);
	return NULL;
}

// Applies component_op(dst, src_left, src_right) n times.
// dst is incremented by component_size.
// src_left, src_right are incremented iff their locks are not set.
static INLINE void apply_component_wise(
								 void (*component_op)(void*, void*, void*),
								 int n,
								 int component_size,
								 void* dst,
								 void* src_left,
								 bool lock_src_left,
								 void* src_right,
								 bool lock_src_right
								 )
{
	int i;

	for (i = 0; i < n; i++)
	{
		component_op(dst, src_left, src_right);
		dst = (char *)dst + component_size;
		if (!lock_src_left) src_left = (char *)src_left + component_size;
		if (!lock_src_right) src_right = (char *)src_right + component_size;
	}
}

Expr* glsl_expr_construct_binary_op_arithmetic(ExprFlavour flavour, Expr* left, Expr* right)
{
	Expr* expr;

	expr = (Expr *)malloc_fast(sizeof(Expr));
	expr->line_num = g_LineNumber;
	expr->flavour = flavour;
	expr->u.binary_op.left = left;
	expr->u.binary_op.right = right;

	expr->compile_time_value = NULL;
   expr->constant_index_expression = left->constant_index_expression && right->constant_index_expression;

	if (SYMBOL_PRIMITIVE_TYPE == left->type->flavour && SYMBOL_PRIMITIVE_TYPE == right->type->flavour)
	{
		int left_index, right_index;
		PRIMITIVE_TYPE_FLAGS_T left_flags, right_flags;
		left_index = left->type->u.primitive_type.index;
		left_flags = primitiveTypeFlags[left_index];
		right_index = right->type->u.primitive_type.index;
		right_flags = primitiveTypeFlags[right_index];

		if ((left_flags & PRIM_ARITHMETIC_BINARY) && (right_flags & PRIM_ARITHMETIC_BINARY))
		{
			// Find the correct component-wise operation to apply if this is a compile time constant.
			// If the base types of left and right are not equal this will be rejected below.
			void (*component_op)(void*, void*, void*);
			int component_size;

			switch (left_flags & (PRIM_INT_TYPE | PRIM_FLOAT_TYPE))
			{
				case PRIM_INT_TYPE:
					switch (flavour)
					{
						case EXPR_MUL:
							component_op = (void (*)(void*, void*, void*))op_mul__const_int__const_int__const_int;
							break;
						case EXPR_DIV:
							component_op = (void (*)(void*, void*, void*))op_div__const_int__const_int__const_int;
							break;
						case EXPR_ADD:
							component_op = (void (*)(void*, void*, void*))op_add__const_int__const_int__const_int;
							break;
						case EXPR_SUB:
							component_op = (void (*)(void*, void*, void*))op_sub__const_int__const_int__const_int;
							break;
						default:
							UNREACHABLE();
							return NULL;
					}
					component_size = sizeof(const_int);
					break;
				case PRIM_FLOAT_TYPE:
					switch (flavour)
					{
						case EXPR_MUL:
							component_op = (void (*)(void*, void*, void*))op_mul__const_float__const_float__const_float;
							break;
						case EXPR_DIV:
							component_op = (void (*)(void*, void*, void*))op_div__const_float__const_float__const_float;
							break;
						case EXPR_ADD:
							component_op = (void (*)(void*, void*, void*))op_add__const_float__const_float__const_float;
							break;
						case EXPR_SUB:
							component_op = (void (*)(void*, void*, void*))op_sub__const_float__const_float__const_float;
							break;
						default:
							UNREACHABLE();
							return NULL;
					}
					component_size = sizeof(const_float);
					break;
				default:
					UNREACHABLE();
					return NULL;
			}

			// Infer type, and value if possible.
			// From spec (1.00 rev 14 p46), the two operands must be:
			// 1 - the same type (the type being integer scalar/vector, float scalar/vector/matrix),
			// 2 - or one can be a scalar float and the other a float vector or matrix,
			// 3 - or one can be a scalar integer and the other an integer vector,
			// 4 - or, for multiply, one can be a float vector and the other a float matrix with the same dimensional size.
			// All operations are component-wise except EXPR_MUL involving at least one matrix (cases 1 and 4).

			// Case 1.
			if (left->type == right->type)
			{
				expr->type = left->type;

				if (left->compile_time_value && right->compile_time_value)
				{
					expr->compile_time_value = malloc_fast(expr->type->size_as_const);

					if (EXPR_MUL == flavour && (left_flags & PRIM_MATRIX_TYPE))
					{
						// Linear algebraic mat * mat.
						switch (primitiveTypeSubscriptDimensions[left_index])
						{
							case 2:
								op_mul__const_mat2__const_mat2__const_mat2(
									(const_mat2 *)expr->compile_time_value,
									(const_mat2 *)left->compile_time_value,
									(const_mat2 *)right->compile_time_value
									);
								break;
							case 3:
								op_mul__const_mat3__const_mat3__const_mat3(
									(const_mat3 *)expr->compile_time_value,
									(const_mat3 *)left->compile_time_value,
									(const_mat3 *)right->compile_time_value
									);
								break;
							case 4:
								op_mul__const_mat4__const_mat4__const_mat4(
									(const_mat4 *)expr->compile_time_value,
									(const_mat4 *)left->compile_time_value,
									(const_mat4 *)right->compile_time_value
									);
								break;
							default:
								UNREACHABLE();
								return NULL;
						}
					}
					else
					{
						apply_component_wise(
							component_op,
                     primitiveTypes[expr->type->u.primitive_type.index].scalar_count,
							component_size,
							expr->compile_time_value,
							left->compile_time_value,
							false,
							right->compile_time_value,
							false
							);
					}
				}

				return expr;
			}

			// Case 2.
			if ((left_flags & PRIM_FLOAT_TYPE) && (left_flags & PRIM_SCALAR_TYPE)
				&& (right_flags & PRIM_FLOAT_TYPE) && (right_flags & (PRIM_VECTOR_TYPE | PRIM_MATRIX_TYPE)))
			{
				expr->type = right->type; // vector or matrix type

				if (left->compile_time_value && right->compile_time_value)
				{
					expr->compile_time_value = malloc_fast(expr->type->size_as_const);

					apply_component_wise(
						component_op,
                  primitiveTypes[expr->type->u.primitive_type.index].scalar_count,
						component_size,
						expr->compile_time_value,
						left->compile_time_value,
						true,
						right->compile_time_value,
						false
						);
				}

				return expr;
			}
			else if ((right_flags & PRIM_FLOAT_TYPE) && (right_flags & PRIM_SCALAR_TYPE)
				&& (left_flags & PRIM_FLOAT_TYPE) && (left_flags & (PRIM_VECTOR_TYPE | PRIM_MATRIX_TYPE)))
			{
				expr->type = left->type; // vector or matrix type

				if (left->compile_time_value && right->compile_time_value)
				{
					expr->compile_time_value = malloc_fast(expr->type->size_as_const);

					apply_component_wise(
						component_op,
                  primitiveTypes[expr->type->u.primitive_type.index].scalar_count,
						component_size,
						expr->compile_time_value,
						left->compile_time_value,
						false,
						right->compile_time_value,
						true
						);
				}

				return expr;
			}

			// Case 3.
			if ((left_flags & PRIM_INT_TYPE) && (left_flags & PRIM_SCALAR_TYPE)
				&& (right_flags & PRIM_INT_TYPE) && (right_flags & PRIM_VECTOR_TYPE))
			{
				expr->type = right->type; // vector type

				if (left->compile_time_value && right->compile_time_value)
				{
					expr->compile_time_value = malloc_fast(expr->type->size_as_const);

					apply_component_wise(
						component_op,
                  primitiveTypes[expr->type->u.primitive_type.index].scalar_count,
						component_size,
						expr->compile_time_value,
						left->compile_time_value,
						true,
						right->compile_time_value,
						false
						);
				}

				return expr;
			}
			else if ((right_flags & PRIM_INT_TYPE) && (right_flags & PRIM_SCALAR_TYPE)
				&& (left_flags & PRIM_INT_TYPE) && (left_flags & PRIM_VECTOR_TYPE))
			{
				expr->type = left->type; // vector type

				if (left->compile_time_value && right->compile_time_value)
				{
					expr->compile_time_value = malloc_fast(expr->type->size_as_const);

					apply_component_wise(
						component_op,
                  primitiveTypes[expr->type->u.primitive_type.index].scalar_count,
						component_size,
						expr->compile_time_value,
						left->compile_time_value,
						false,
						right->compile_time_value,
						true
						);
				}

				return expr;
			}

			// Case 4.
			if (EXPR_MUL == flavour)
			{
				if ((left_flags & PRIM_FLOAT_TYPE) && (left_flags & PRIM_MATRIX_TYPE)
					&& (right_flags & PRIM_FLOAT_TYPE) && (right_flags & PRIM_VECTOR_TYPE)
					&& primitiveTypeSubscriptDimensions[left_index] == primitiveTypeSubscriptDimensions[left_index])
				{
					expr->type = right->type; // vector type

					if (left->compile_time_value && right->compile_time_value)
					{
						expr->compile_time_value = malloc_fast(expr->type->size_as_const);

						// Linear algebraic mat * vec.
						switch (primitiveTypeSubscriptDimensions[left_index])
						{
							case 2:
								op_mul__const_vec2__const_mat2__const_vec2(
									(const_vec2 *)expr->compile_time_value,
									(const_mat2 *)left->compile_time_value,
									(const_vec2 *)right->compile_time_value
									);
								break;
							case 3:
								op_mul__const_vec3__const_mat3__const_vec3(
									(const_vec3 *)expr->compile_time_value,
									(const_mat3 *)left->compile_time_value,
									(const_vec3 *)right->compile_time_value
									);
								break;
							case 4:
								op_mul__const_vec4__const_mat4__const_vec4(
									(const_vec4 *)expr->compile_time_value,
									(const_mat4 *)left->compile_time_value,
									(const_vec4 *)right->compile_time_value
									);
								break;
							default:
								UNREACHABLE();
								return NULL;
						}
					}

					return expr;
				}
				else if ((right_flags & PRIM_FLOAT_TYPE) && (right_flags & PRIM_MATRIX_TYPE)
					&& (left_flags & PRIM_FLOAT_TYPE) && (left_flags & PRIM_VECTOR_TYPE)
					&& primitiveTypeSubscriptDimensions[left_index] == primitiveTypeSubscriptDimensions[left_index])
				{
					expr->type = left->type; // vector type

					if (left->compile_time_value && right->compile_time_value)
					{
						expr->compile_time_value = malloc_fast(expr->type->size_as_const);

						// Linear algebraic vec * mat.
						switch (primitiveTypeSubscriptDimensions[left_index])
						{
							case 2:
								op_mul__const_vec2__const_vec2__const_mat2(
									(const_vec2 *)expr->compile_time_value,
									(const_vec2 *)left->compile_time_value,
									(const_mat2 *)right->compile_time_value
									);
								break;
							case 3:
								op_mul__const_vec3__const_vec3__const_mat3(
									(const_vec3 *)expr->compile_time_value,
									(const_vec3 *)left->compile_time_value,
									(const_mat3 *)right->compile_time_value
									);
								break;
							case 4:
								op_mul__const_vec4__const_vec4__const_mat4(
									(const_vec4 *)expr->compile_time_value,
									(const_vec4 *)left->compile_time_value,
									(const_mat4 *)right->compile_time_value
									);
								break;
							default:
								UNREACHABLE();
								return NULL;
						}
					}

					return expr;
				}
			}
		}
	}

	// Operator not supported.
	glsl_compile_error(ERROR_SEMANTIC, 4, g_LineNumber, NULL);
	return NULL;
}

Expr* glsl_expr_construct_binary_op_logical(ExprFlavour flavour, Expr* left, Expr* right)
{
	Expr* expr;

	expr = (Expr *)malloc_fast(sizeof(Expr));
	expr->line_num = g_LineNumber;
	expr->flavour = flavour;
	expr->u.binary_op.left = left;
	expr->u.binary_op.right = right;

	expr->compile_time_value = NULL;
   expr->constant_index_expression = left->constant_index_expression && right->constant_index_expression;

	if (left->type == right->type)
	{
		// Only need look at left type as they are identical anyway.
		if (SYMBOL_PRIMITIVE_TYPE == left->type->flavour)
		{
			int left_index = left->type->u.primitive_type.index;

			// Only need look at left type as they are identical anyway.
			if (primitiveTypeFlags[left_index] & PRIM_LOGICAL_BINARY)
			{
				expr->type = &primitiveTypes[PRIM_BOOL];

				if (left->compile_time_value && right->compile_time_value)
				{
					expr->compile_time_value = malloc_fast(expr->type->size_as_const);

					// This code assumes we do not act on vectors.
					vcos_assert(PRIM_BOOL == left_index);

					switch (flavour)
					{
						case EXPR_LOGICAL_AND:
							op_logical_and__const_bool__const_bool__const_bool(
								(const_bool *)expr->compile_time_value,
								(const_bool *)left->compile_time_value,
								(const_bool *)right->compile_time_value);
							break;
						case EXPR_LOGICAL_XOR:
							op_logical_xor__const_bool__const_bool__const_bool(
								(const_bool *)expr->compile_time_value,
								(const_bool *)left->compile_time_value,
								(const_bool *)right->compile_time_value);
							break;
						case EXPR_LOGICAL_OR:
							op_logical_or__const_bool__const_bool__const_bool(
								(const_bool *)expr->compile_time_value,
								(const_bool *)left->compile_time_value,
								(const_bool *)right->compile_time_value);
							break;
						default:
							UNREACHABLE();
							return NULL;
					}
				}

				return expr;
			}
		}
	}

	// Operator not supported.
	glsl_compile_error(ERROR_SEMANTIC, 4, g_LineNumber, NULL);
	return NULL;
}

Expr* glsl_expr_construct_binary_op_relational(ExprFlavour flavour, Expr* left, Expr* right)
{
	Expr* expr;

	expr = (Expr *)malloc_fast(sizeof(Expr));
	expr->line_num = g_LineNumber;
	expr->flavour = flavour;
	expr->u.binary_op.left = left;
	expr->u.binary_op.right = right;

	expr->compile_time_value = NULL;
   expr->constant_index_expression = left->constant_index_expression && right->constant_index_expression;

	if (left->type == right->type)
	{
		// Only need look at left type as they are identical anyway.
		if (SYMBOL_PRIMITIVE_TYPE == left->type->flavour)
		{
			int left_index = left->type->u.primitive_type.index;
			int left_flags = primitiveTypeFlags[left_index];

			// Only need look at left type as they are identical anyway.
			if (left_flags & PRIM_RELATIONAL)
			{
				expr->type = &primitiveTypes[PRIM_BOOL];

				if (left->compile_time_value && right->compile_time_value)
				{
					expr->compile_time_value = malloc_fast(expr->type->size_as_const);

					// This code assumes we do not act on vectors.
					vcos_assert(left_flags & PRIM_SCALAR_TYPE);

					switch (left_flags & (PRIM_INT_TYPE | PRIM_FLOAT_TYPE))
					{
						case PRIM_INT_TYPE:
							switch (flavour)
							{
								case EXPR_LESS_THAN:
									op_less_than__const_bool__const_int__const_int(
										(const_bool *)expr->compile_time_value,
										(const_int *)left->compile_time_value,
										(const_int *)right->compile_time_value
										);
									break;
								case EXPR_LESS_THAN_EQUAL:
									op_less_than_equal__const_bool__const_int__const_int(
										(const_bool *)expr->compile_time_value,
										(const_int *)left->compile_time_value,
										(const_int *)right->compile_time_value
										);
									break;
								case EXPR_GREATER_THAN:
									op_greater_than__const_bool__const_int__const_int(
										(const_bool *)expr->compile_time_value,
										(const_int *)left->compile_time_value,
										(const_int *)right->compile_time_value
										);
									break;
								case EXPR_GREATER_THAN_EQUAL:
									op_greater_than_equal__const_bool__const_int__const_int(
										(const_bool *)expr->compile_time_value,
										(const_int *)left->compile_time_value,
										(const_int *)right->compile_time_value
										);
									break;
								default:
									UNREACHABLE();
									return NULL;
							}
							break;
						case PRIM_FLOAT_TYPE:
							switch (flavour)
							{
								case EXPR_LESS_THAN:
									op_less_than__const_bool__const_float__const_float(
										(const_bool *)expr->compile_time_value,
										(const_float *)left->compile_time_value,
										(const_float *)right->compile_time_value
										);
									break;
								case EXPR_LESS_THAN_EQUAL:
									op_less_than_equal__const_bool__const_float__const_float(
										(const_bool *)expr->compile_time_value,
										(const_float *)left->compile_time_value,
										(const_float *)right->compile_time_value
										);
									break;
								case EXPR_GREATER_THAN:
									op_greater_than__const_bool__const_float__const_float(
										(const_bool *)expr->compile_time_value,
										(const_float *)left->compile_time_value,
										(const_float *)right->compile_time_value
										);
									break;
								case EXPR_GREATER_THAN_EQUAL:
									op_greater_than_equal__const_bool__const_float__const_float(
										(const_bool *)expr->compile_time_value,
										(const_float *)left->compile_time_value,
										(const_float *)right->compile_time_value
										);
									break;
								default:
									UNREACHABLE();
									return NULL;
							}
							break;
						default:
							UNREACHABLE();
							return NULL;
					}

				}

				return expr;
			}
		}
	}

	// Operator not supported.
	glsl_compile_error(ERROR_SEMANTIC, 4, g_LineNumber, NULL);
	return NULL;
}

Expr* glsl_expr_construct_binary_op_equality(ExprFlavour flavour, Expr* left, Expr* right)
{
	Expr* expr;

	expr = (Expr *)malloc_fast(sizeof(Expr));
	expr->line_num = g_LineNumber;
	expr->flavour = flavour;
	expr->u.binary_op.left = left;
	expr->u.binary_op.right = right;

	expr->compile_time_value = NULL;
   expr->constant_index_expression = left->constant_index_expression && right->constant_index_expression;

	if (left->type == right->type)
	{
		int operand_size_as_const = left->type->size_as_const;
		bool op_supported = false;

		// Only need look at left type as they are identical anyway.
		if (SYMBOL_PRIMITIVE_TYPE == left->type->flavour)
		{
			int left_index = left->type->u.primitive_type.index;
			int left_flags = primitiveTypeFlags[left_index];

			// Only need look at left type as they are identical anyway.
			if (left_flags & PRIM_EQUALITY)
			{
				op_supported = true;
			}
		}
		else if (SYMBOL_STRUCT_TYPE == left->type->flavour)
		{
			// Struct types that cannot have const instances also cannot be tested for equality.
			if (operand_size_as_const)
			{
				op_supported = true;
			}
		}

		if (op_supported)
		{
			expr->type = &primitiveTypes[PRIM_BOOL];

			if (left->compile_time_value && right->compile_time_value)
			{
				const_bool equal;

				expr->compile_time_value = malloc_fast(expr->type->size_as_const);

				equal = (0 == memcmp(left->compile_time_value, right->compile_time_value, operand_size_as_const));

				switch (flavour)
				{
					case EXPR_EQUAL:
						*(const_bool*)expr->compile_time_value = equal;
						break;
					case EXPR_NOT_EQUAL:
						*(const_bool*)expr->compile_time_value = !equal;
						break;
					default:
						UNREACHABLE();
						return NULL;
				}
			}

			return expr;
		}
	}

	// Operator not supported.
	glsl_compile_error(ERROR_SEMANTIC, 4, g_LineNumber, NULL);
	return NULL;
}

Expr* glsl_expr_construct_cond_op(Expr* cond, Expr* if_true, Expr* if_false)
{
	Expr* expr = (Expr *)malloc_fast(sizeof(Expr));
	expr->line_num = g_LineNumber;
	expr->flavour = EXPR_CONDITIONAL;
	expr->u.cond_op.cond = cond;
	expr->u.cond_op.if_true = if_true;
	expr->u.cond_op.if_false = if_false;
   expr->constant_index_expression = cond->constant_index_expression && if_true->constant_index_expression && if_false->constant_index_expression;

	if (&primitiveTypes[PRIM_BOOL] == cond->type)
	{
		if (SYMBOL_ARRAY_TYPE == if_true->type->flavour || SYMBOL_ARRAY_TYPE == if_false->type->flavour)
		{
			// if_true and if_false cannot be arrays.
			glsl_compile_error(ERROR_CUSTOM, 4, g_LineNumber, NULL);
			return NULL;
		}
		else if (if_true->type == if_false->type)
		{
			if (cond->compile_time_value)
			{
				if (*(const_bool*)(cond->compile_time_value))
				{
					expr->compile_time_value = if_true->compile_time_value;
				}
				else
				{
					expr->compile_time_value = if_false->compile_time_value;
				}
			}
			else
			{
				expr->compile_time_value = NULL;
			}

			expr->type = if_true->type;
			return expr;
		}
		else
		{
			// if_true and if_false must have same type.
			glsl_compile_error(ERROR_SEMANTIC, 6, g_LineNumber, NULL);
			return NULL;
		}
	}
	else
	{
		// cond must be a (scalar) bool.
		glsl_compile_error(ERROR_SEMANTIC, 5, g_LineNumber, NULL);
		return NULL;
	}
}

Expr* glsl_expr_construct_assign_op(Expr* lvalue, Expr* rvalue)
{
	Expr* expr = (Expr *)malloc_fast(sizeof(Expr));
	expr->line_num = g_LineNumber;
	expr->flavour = EXPR_ASSIGN;
	expr->u.assign_op.lvalue = lvalue;
	expr->u.assign_op.rvalue = rvalue;
	expr->type = rvalue->type;

	// Assignments can never have a compile_time_value as the lvalue cannot be constant,
	// and thus they cannot form part of constant expressions.
	expr->compile_time_value = NULL;
   expr->constant_index_expression = false;

	if (!(glsl_get_lvalue_flags(lvalue) & LVF_ASSIGNMENT_TARGET))
	{
		// Not an l-value.
		glsl_compile_error(ERROR_SEMANTIC, 27, g_LineNumber, NULL);
		return NULL;
	}

	if (!glsl_shallow_match_nonfunction_types(lvalue->type, rvalue->type))
	{
		// Type mismatch.
		glsl_compile_error(ERROR_SEMANTIC, 1, g_LineNumber, NULL);
		return NULL;
	}

	return expr;
}

Expr* glsl_expr_construct_sequence(Expr* all_these, Expr* then_this)
{
	Expr* expr = (Expr *)malloc_fast(sizeof(Expr));
	expr->line_num = g_LineNumber;
	expr->flavour = EXPR_SEQUENCE;
	expr->u.sequence.all_these = all_these;
	expr->u.sequence.then_this = then_this;
	expr->type = then_this->type;

   /*
      Slightly fishy - we define a sequence to be a compile time
      constant iff both operands are compile time constants. This
      is consistent with the spec, and means that lvalues have
      no side effects.
   */

	if (all_these->compile_time_value && then_this->compile_time_value)
   	expr->compile_time_value = then_this->compile_time_value;
   else
   	expr->compile_time_value = NULL;

   expr->constant_index_expression = all_these->constant_index_expression && then_this->constant_index_expression;

	return expr;
}

Expr* glsl_expr_construct_intrinsic(ExprFlavour flavour, ExprChain* args)
{
   bool bad_arg_types = false, bad_arg_count = false;
	Expr* expr = (Expr *)malloc_fast(sizeof(Expr));

	expr->line_num = g_LineNumber;
	expr->flavour = flavour;
	expr->compile_time_value = NULL;
   expr->constant_index_expression = false;

   expr->u.intrinsic.args = args;

	// Check args and set expression type.
   switch (flavour)
   {
      case EXPR_INTRINSIC_TEXTURE_2D_BIAS:
      case EXPR_INTRINSIC_TEXTURE_2D_LOD:
         {
            if (3 == args->count)
            {
               if (&primitiveTypes[PRIM_SAMPLER2D] != args->first->expr->type)
               {
                  bad_arg_types = true;
               }
               if (&primitiveTypes[PRIM_VEC2] != args->first->next->expr->type)
               {
                  bad_arg_types = true;
               }
               if (&primitiveTypes[PRIM_FLOAT] != args->first->next->next->expr->type)
               {
                  bad_arg_types = true;
               }
            }
            else
            {
               bad_arg_count = true;
            }

            expr->type = &primitiveTypes[PRIM_VEC4];
         }
         break;

      case EXPR_INTRINSIC_TEXTURE_CUBE_BIAS:
      case EXPR_INTRINSIC_TEXTURE_CUBE_LOD:
         {
            if (3 == args->count)
            {
               if (&primitiveTypes[PRIM_SAMPLERCUBE] != args->first->expr->type)
               {
                  bad_arg_types = true;
               }
               if (&primitiveTypes[PRIM_VEC3] != args->first->next->expr->type)
               {
                  bad_arg_types = true;
               }
               if (&primitiveTypes[PRIM_FLOAT] != args->first->next->next->expr->type)
               {
                  bad_arg_types = true;
               }
            }
            else
            {
               bad_arg_count = true;
            }

            expr->type = &primitiveTypes[PRIM_VEC4];
         }
         break;

      case EXPR_INTRINSIC_RSQRT:
      case EXPR_INTRINSIC_RCP:
      case EXPR_INTRINSIC_LOG2:
      case EXPR_INTRINSIC_EXP2:
      case EXPR_INTRINSIC_CEIL:
      case EXPR_INTRINSIC_FLOOR:
      case EXPR_INTRINSIC_SIGN:
      case EXPR_INTRINSIC_TRUNC:
      case EXPR_INTRINSIC_NEAREST:
         {
            if (1 == args->count)
            {
               if (&primitiveTypes[PRIM_FLOAT] != args->first->expr->type)
               {
                  bad_arg_types = true;
               }
            }
            else
            {
               bad_arg_count = true;
            }

            expr->type = &primitiveTypes[PRIM_FLOAT];
         }
         break;

      case EXPR_INTRINSIC_MIN:
      case EXPR_INTRINSIC_MAX:
         {
            if (2 == args->count)
            {
               if (&primitiveTypes[PRIM_FLOAT] != args->first->expr->type)
               {
                  bad_arg_types = true;
               }
               if (&primitiveTypes[PRIM_FLOAT] != args->first->next->expr->type)
               {
                  bad_arg_types = true;
               }
            }
            else
            {
               bad_arg_count = true;
            }

            expr->type = &primitiveTypes[PRIM_FLOAT];
         }
         break;

      default:
         UNREACHABLE();
         return NULL;
   }

   if (bad_arg_count)
   {
      // Bad number of arguments to intrinsic.
      glsl_compile_error(ERROR_CUSTOM, 24, g_LineNumber, NULL);
      return NULL;
   }
   if (bad_arg_types)
   {
      // Bad type of arguments to intrinsic.
      glsl_compile_error(ERROR_CUSTOM, 25, g_LineNumber, NULL);
      return NULL;
   }

   return expr;
}


Statement* glsl_statement_construct(StatementFlavour flavour)
{
	Statement* statement = (Statement *)malloc_fast(sizeof(Statement));
	statement->line_num = g_LineNumber;
	statement->flags = 0;
	statement->flavour = flavour;
	return statement;
}

Statement* glsl_statement_construct_ast(StatementChain* decls)
{
	Statement* statement = (Statement *)malloc_fast(sizeof(Statement));
	statement->line_num = g_LineNumber;
	statement->flags = 0;
	statement->flavour = STATEMENT_AST;
	statement->u.ast.decls = decls;
	return statement;
}

Statement* glsl_statement_construct_decl_list(StatementChain* decls)
{
	Statement* statement = (Statement *)malloc_fast(sizeof(Statement));
	statement->line_num = g_LineNumber;
	statement->flags = 0;
	statement->flavour = STATEMENT_DECL_LIST;
	statement->u.decl_list.decls = decls;
	return statement;
}

Statement* glsl_statement_construct_function_def(Symbol* header, Statement* body)
{
	Statement* statement = (Statement *)malloc_fast(sizeof(Statement));
	statement->line_num = g_LineNumber;
	statement->flags = 0;
	statement->flavour = STATEMENT_FUNCTION_DEF;
	statement->u.function_def.header = header;
	statement->u.function_def.body = body;
   statement->u.function_def.active = false;

	check_returns(header->type->u.function_type.return_type, body);

	return statement;
}

Statement* glsl_statement_construct_var_decl(Symbol* var, Expr* initializer)
{
	Statement* statement = (Statement *)malloc_fast(sizeof(Statement));
	statement->line_num = g_LineNumber;
	statement->flags = 0;

	// Ensure that samplers can only be uniforms.
	// (Note they can also be function parameters, but that isn't dealt with here.)
	if (SYMBOL_PRIMITIVE_TYPE == var->type->flavour
		&& (primitiveTypeFlags[var->type->u.primitive_type.index] & PRIM_SAMPLER_TYPE))
	{
		if (TYPE_QUAL_UNIFORM != var->u.var_instance.type_qual)
		{
			// Sampler type can only be used for uniform or function parameter.
         // BUT, suppress this error if we're not in user code,
         // because we need to declare local sampler variables for function inlining.
         if (LINE_NUMBER_UNDEFINED != g_LineNumber)
         {
			   glsl_compile_error(ERROR_CUSTOM, 22, g_LineNumber, NULL);
			   return NULL;
         }
		}
	}

	// Check that the type qualifiers are valid here.
	switch (var->u.var_instance.type_qual)
	{
		case TYPE_QUAL_NONE:
		case TYPE_QUAL_CONST:
			break;

		case TYPE_QUAL_ATTRIBUTE:
			if (SHADER_VERTEX != g_ShaderFlavour)
			{
				// Declaring an attribute outside of a vertex shader.
				glsl_compile_error(ERROR_SEMANTIC, 44, g_LineNumber, NULL);
				return NULL;
			}
			if (SYMBOL_PRIMITIVE_TYPE != var->type->flavour
				|| !(primitiveTypeFlags[var->type->u.primitive_type.index] & PRIM_FLOAT_TYPE))
			{
				// Only float types allowed as attributes. Cannot be arrays or structures.
				// Illegal data type for attribute.
				glsl_compile_error(ERROR_SEMANTIC, 49, g_LineNumber, NULL);
				return NULL;
			}
			if (initializer)
			{
				// Initializer for attribute.
				glsl_compile_error(ERROR_SEMANTIC, 50, g_LineNumber, NULL);
				return NULL;
			}
			if (!g_InGlobalScope)
			{
				// Declaring an attribute inside a function.
				glsl_compile_error(ERROR_SEMANTIC, 45, g_LineNumber, NULL);
				return NULL;
			}
			break;

		case TYPE_QUAL_VARYING:
		case TYPE_QUAL_INVARIANT_VARYING:
			if ((
				// Not a float type.
				SYMBOL_PRIMITIVE_TYPE != var->type->flavour
				|| !(primitiveTypeFlags[var->type->u.primitive_type.index] & PRIM_FLOAT_TYPE)
				) && (
				// Not an array of float type.
				SYMBOL_ARRAY_TYPE != var->type->flavour
				|| SYMBOL_PRIMITIVE_TYPE != var->type->u.array_type.member_type->flavour
				|| !(primitiveTypeFlags[var->type->u.array_type.member_type->u.primitive_type.index] & PRIM_FLOAT_TYPE)
				))
			{
				// Only float types or arrays of float types allowed as varyings. Cannot be structures.
				// Illegal data type for varying.
				glsl_compile_error(ERROR_SEMANTIC, 48, g_LineNumber, NULL);
				return NULL;
			}
			if (initializer)
			{
				// Initializer for varying.
				glsl_compile_error(ERROR_SEMANTIC, 51, g_LineNumber, NULL);
				return NULL;
			}
			if (!g_InGlobalScope)
			{
				// Declaring a varying inside a function.
				glsl_compile_error(ERROR_SEMANTIC, 47, g_LineNumber, NULL);
				return NULL;
			}
			break;

		case TYPE_QUAL_UNIFORM:
			if (initializer)
			{
				// Initializer for uniform.
				glsl_compile_error(ERROR_SEMANTIC, 52, g_LineNumber, NULL);
				return NULL;
			}
			if (!g_InGlobalScope)
			{
				// Declaring a uniform inside a function.
				glsl_compile_error(ERROR_SEMANTIC, 46, g_LineNumber, NULL);
				return NULL;
			}
			break;

		default:
			UNREACHABLE();
			return NULL;
	}

	// Check types.
	if (initializer && !glsl_shallow_match_nonfunction_types(var->type, initializer->type))
	{
		// Type mismatch.
		glsl_compile_error(ERROR_SEMANTIC, 1, g_LineNumber, NULL);
		return NULL;
	}

	statement->flavour = STATEMENT_VAR_DECL;
	statement->u.var_decl.var = var;
	statement->u.var_decl.initializer = initializer;
	return statement;
}

Statement* glsl_statement_construct_compound(StatementChain* statements)
{
	Statement* statement = (Statement *)malloc_fast(sizeof(Statement));
	statement->line_num = g_LineNumber;
	statement->flags = 0;
	statement->flavour = STATEMENT_COMPOUND;
	statement->u.compound.statements = statements;
	return statement;
}

Statement* glsl_statement_construct_expr(Expr* expr)
{
	Statement* statement = (Statement *)malloc_fast(sizeof(Statement));
	statement->line_num = g_LineNumber;
	statement->flags = 0;
	statement->flavour = STATEMENT_EXPR;
	statement->u.expr.expr = expr;
	return statement;
}

static INLINE Statement* promote_to_statement_compound(Statement* statement)
{
	if (!statement)
	{
		return NULL;
	}
	else if (STATEMENT_COMPOUND == statement->flavour)
	{
		return statement;
	}
	else
	{
		return glsl_statement_construct_compound(glsl_statement_chain_append(glsl_statement_chain_create(), statement));
	}
}

Statement* glsl_statement_construct_selection(Expr* cond, Statement* if_true, Statement* if_false)
{
	if (&primitiveTypes[PRIM_BOOL] == cond->type)
	{
	   Statement* statement = (Statement *)malloc_fast(sizeof(Statement));
	   statement->line_num = g_LineNumber;
	   statement->flags = 0;
	   statement->flavour = STATEMENT_SELECTION;
	   statement->u.selection.cond = cond;
	   statement->u.selection.if_true = promote_to_statement_compound(if_true);
	   statement->u.selection.if_false = promote_to_statement_compound(if_false);

	   return statement;
	}
	else
	{
		// cond must be a (scalar) bool.
		glsl_compile_error(ERROR_SEMANTIC, 3, g_LineNumber, NULL);
		return NULL;
	}
}

static bool found_mutation;

#define MAX_ITERATIONS 1024

static void epostv_find_mutations(Expr* expr, Symbol *var)
{
	vcos_assert(&primitiveTypes[PRIM_INT] == var->type || &primitiveTypes[PRIM_FLOAT] == var->type);

	switch (expr->flavour)
	{
		case EXPR_POST_INC:
		case EXPR_POST_DEC:
		case EXPR_PRE_INC:
		case EXPR_PRE_DEC:
			if (EXPR_INSTANCE == expr->u.unary_op.operand->flavour)
            if (expr->u.unary_op.operand->u.instance.symbol == var)
               found_mutation = true;
			break;
		case EXPR_ASSIGN:
			if (EXPR_INSTANCE == expr->u.assign_op.lvalue->flavour)
            if (expr->u.assign_op.lvalue->u.instance.symbol == var)
               found_mutation = true;
			break;
      case EXPR_FUNCTION_CALL:
      {
         int i;

         Symbol *function = expr->u.function_call.function;

         ExprChainNode *node;

         node = expr->u.function_call.args->first;

         for (i = 0; i < function->type->u.function_type.param_count; i++, node = node->next) {
            Symbol *formal = function->type->u.function_type.params[i];
            Expr *actual = node->expr;

            if (formal->u.var_instance.param_qual == PARAM_QUAL_OUT || formal->u.var_instance.param_qual == PARAM_QUAL_INOUT)
      			if (EXPR_INSTANCE == actual->flavour)
                  if (expr->u.instance.symbol == var)
                     found_mutation = true;
         }

         break;
      }
      default:
         break;
	}
}

static int calc_iterations(Statement* init, Expr* test, Expr* loop, Statement* block)
{
   int i;

   union {
      const_int i;
      const_float f;
   } loop_init;
   union {
      const_int i;
      const_float f;
   } loop_bound;
   union {
      const_int i;
      const_float f;
   } loop_stride;

   /*
      find the loop index variable and check that it is initialized to a
      constant value
   */

   Symbol* loop_index = init->u.var_decl.var;

   bool is_int = loop_index->type->u.primitive_type.index == PRIM_INT;

	if (init->u.var_decl.initializer->compile_time_value)
      if (is_int)
   		loop_init.i = *(const_int*)init->u.var_decl.initializer->compile_time_value;
      else
	   	loop_init.f = *(const_float*)init->u.var_decl.initializer->compile_time_value;
   else
		glsl_compile_error(ERROR_OPTIMIZER, 3, init->line_num, "loop index must be initialized to a constant expression");

   /*
      find the condition expression, and check that it applies one of the
      six relational operators
   */

	switch (test->flavour)
	{
		case EXPR_GREATER_THAN:
		case EXPR_GREATER_THAN_EQUAL:
		case EXPR_LESS_THAN:
		case EXPR_LESS_THAN_EQUAL:
		case EXPR_EQUAL:
		case EXPR_NOT_EQUAL:
			break;
		default:
			glsl_compile_error(ERROR_OPTIMIZER, 3, test->line_num, "condition expression must be binary operation returning boolean");
         break;
	}

   /*
      check that the relational operator is applied to the loop index and a constant
   */

	if (EXPR_INSTANCE != test->u.binary_op.left->flavour || test->u.binary_op.left->u.instance.symbol != loop_index)
		glsl_compile_error(ERROR_OPTIMIZER, 3, test->line_num, "condition expression must act directly on loop index");

	if (!test->u.binary_op.right->compile_time_value)
		glsl_compile_error(ERROR_OPTIMIZER, 3, test->line_num, "right side of condition expression must be constant");

   if (is_int)
	   loop_bound.i = *(const_int *)test->u.binary_op.right->compile_time_value;
   else
      loop_bound.f = *(const_float *)test->u.binary_op.right->compile_time_value;

   /*
	   check that loop_index is not assigned to in the body
   */

   found_mutation = false;

	glsl_statement_accept_postfix(block, loop_index, NULL, (ExprPostVisitor)epostv_find_mutations);

	if (found_mutation)
		glsl_compile_error(ERROR_OPTIMIZER, 3, g_LineNumber, "cannot assign to loop index in loop body");

   /*
      check that the loop index is mutated exactly once using one of
      the ++, --, += or -= operators and find the loop stride
   */

	switch (loop->flavour) {
		case EXPR_POST_INC:
      case EXPR_PRE_INC:
         if (loop->u.unary_op.operand->flavour != EXPR_INSTANCE || loop->u.unary_op.operand->u.instance.symbol != loop_index)
      		glsl_compile_error(ERROR_OPTIMIZER, 3, loop->line_num, "loop expression must act directly on loop index");

         if (is_int)
            loop_stride.i = (const_int)1;
         else
            loop_stride.f = CONST_FLOAT_ONE;
			break;
		case EXPR_POST_DEC:
      case EXPR_PRE_DEC:
         if (loop->u.unary_op.operand->flavour != EXPR_INSTANCE || loop->u.unary_op.operand->u.instance.symbol != loop_index)
      		glsl_compile_error(ERROR_OPTIMIZER, 3, loop->line_num, "loop expression must act directly on loop index");

         if (is_int)
            loop_stride.i = (const_int)-1;
         else
            loop_stride.f = CONST_FLOAT_MINUS_ONE;
			break;
		case EXPR_ASSIGN:
      {
         Expr *rhs = loop->u.assign_op.rvalue;

         if (EXPR_ADD != rhs->flavour && EXPR_SUB != rhs->flavour)
      		glsl_compile_error(ERROR_OPTIMIZER, 3, loop->line_num, "loop expression must be: ++, --, += or -=");

         if (EXPR_INSTANCE != rhs->u.binary_op.left->flavour || rhs->u.binary_op.left->u.instance.symbol != loop_index)
      		glsl_compile_error(ERROR_OPTIMIZER, 3, loop->line_num, "loop expression must act directly on loop index");

			if (!rhs->u.binary_op.right->compile_time_value)
				glsl_compile_error(ERROR_OPTIMIZER, 3, loop->line_num, "right side of loop expression must be constant");

         if (is_int) {
            loop_stride.i = *(const_int *)rhs->u.binary_op.right->compile_time_value;

            if (EXPR_SUB == rhs->flavour)
               op_arith_negate__const_int__const_int(&loop_stride.i, &loop_stride.i);
         } else {
            loop_stride.f = *(const_float *)rhs->u.binary_op.right->compile_time_value;

            if (EXPR_SUB == rhs->flavour)
               op_arith_negate__const_float__const_float(&loop_stride.f, &loop_stride.f);
         }

			break;
      }
		default:
      	glsl_compile_error(ERROR_OPTIMIZER, 3, loop->line_num, "loop expression must be: ++, --, += or -=");
         break;
	}

   if (is_int) {
      for (i = 0; i < MAX_ITERATIONS; i++) {
         const_bool b = false;

	      switch (test->flavour) {
		      case EXPR_GREATER_THAN:
               op_greater_than__const_bool__const_int__const_int(&b, &loop_init.i, &loop_bound.i);
			      break;
		      case EXPR_GREATER_THAN_EQUAL:
               op_greater_than_equal__const_bool__const_int__const_int(&b, &loop_init.i, &loop_bound.i);
			      break;
		      case EXPR_LESS_THAN:
               op_less_than__const_bool__const_int__const_int(&b, &loop_init.i, &loop_bound.i);
			      break;
		      case EXPR_LESS_THAN_EQUAL:
               op_less_than_equal__const_bool__const_int__const_int(&b, &loop_init.i, &loop_bound.i);
			      break;
		      case EXPR_EQUAL:
               b = loop_init.i == loop_bound.i;
			      break;
		      case EXPR_NOT_EQUAL:
               b = loop_init.i != loop_bound.i;
			      break;
		      default:
               UNREACHABLE();
               break;
	      }

         if (!b)
            return i;

         op_add__const_int__const_int__const_int(&loop_init.i, &loop_init.i, &loop_stride.i);
      }
   } else {
      for (i = 0; i < MAX_ITERATIONS; i++) {
         const_bool b;

	      switch (test->flavour) {
		      case EXPR_GREATER_THAN:
               op_greater_than__const_bool__const_float__const_float(&b, &loop_init.f, &loop_bound.f);
			      break;
		      case EXPR_GREATER_THAN_EQUAL:
               op_greater_than_equal__const_bool__const_float__const_float(&b, &loop_init.f, &loop_bound.f);
			      break;
		      case EXPR_LESS_THAN:
               op_less_than__const_bool__const_float__const_float(&b, &loop_init.f, &loop_bound.f);
			      break;
		      case EXPR_LESS_THAN_EQUAL:
               op_less_than_equal__const_bool__const_float__const_float(&b, &loop_init.f, &loop_bound.f);
			      break;
		      case EXPR_EQUAL:
               b = loop_init.f == loop_bound.f;
			      break;
		      case EXPR_NOT_EQUAL:
               b = loop_init.f != loop_bound.f;
			      break;
		      default:
               UNREACHABLE();
               b = true; /* stop the compiler complaining */
               break;
	      }

         if (!b)
            return i;

         op_add__const_float__const_float__const_float(&loop_init.f, &loop_init.f, &loop_stride.f);
      }
   }

   glsl_compile_error(ERROR_OPTIMIZER, 3, init->line_num, "too many iterations");
   return -1;
}

Statement* glsl_statement_construct_iterator_for(Statement* init, Expr* test, Expr* loop, Statement* block)
{
	Statement* statement = (Statement *)malloc_fast(sizeof(Statement));
	statement->line_num = g_LineNumber;
	statement->flags = 0;
	statement->flavour = STATEMENT_ITERATOR_FOR;
	statement->u.iterator_for.init = init;
	statement->u.iterator_for.test = test;
	statement->u.iterator_for.loop = loop;
	statement->u.iterator_for.block = promote_to_statement_compound(block);

   statement->u.iterator_for.iterations = calc_iterations(init, test, loop, block);

	return statement;
}

Statement* glsl_statement_construct_iterator_while(Statement* cond_or_decl, Statement* block)
{
	Statement* statement = (Statement *)malloc_fast(sizeof(Statement));
	statement->line_num = g_LineNumber;
	statement->flags = 0;
	statement->flavour = STATEMENT_ITERATOR_WHILE;
	statement->u.iterator_while.cond_or_decl = cond_or_decl;
	statement->u.iterator_while.block = promote_to_statement_compound(block);

	glsl_compile_error(ERROR_OPTIMIZER, 1, g_LineNumber, NULL);

   assert(0); // need to add type check for cond_or_decl

	return statement;
}

Statement* glsl_statement_construct_iterator_do_while(Statement* block, Expr* cond)
{
	Statement* statement = (Statement *)malloc_fast(sizeof(Statement));
	statement->line_num = g_LineNumber;
	statement->flags = 0;
	statement->flavour = STATEMENT_ITERATOR_DO_WHILE;
	statement->u.iterator_do_while.block = promote_to_statement_compound(block);
	statement->u.iterator_do_while.cond = cond;

	glsl_compile_error(ERROR_OPTIMIZER, 2, g_LineNumber, NULL);

   assert(0); // need to add type check for cond_or_decl

	return statement;
}

Statement* glsl_statement_construct_return_expr(Expr* expr)
{
	Statement* statement = (Statement *)malloc_fast(sizeof(Statement));
	statement->line_num = g_LineNumber;
	statement->flags = 0;
	statement->flavour = STATEMENT_RETURN_EXPR;
	statement->u.return_expr.expr = expr;

	return statement;
}



//
// Chain constructors.
//

ExprChain* glsl_expr_chain_create(void)
{
	ExprChain* chain = (ExprChain *)malloc_fast(sizeof(ExprChain));

	chain->count = 0;
	chain->all_compile_time_values = true; // trivially so!
	chain->all_primitive_types = true; // trivially so!
   chain->all_constant_index_expression = true; // trivially so!

	chain->first = NULL;
	chain->last = NULL;

	return chain;
}

ExprChain* glsl_expr_chain_append_node(ExprChain* chain, ExprChainNode* node)
{
	Expr* expr = node->expr;

	node->next = NULL;

	if (!chain->first) chain->first = node;
	if (chain->last) chain->last->next = node;
	chain->last = node;

	chain->count++;
	chain->all_compile_time_values = chain->all_compile_time_values && expr->compile_time_value;
	chain->all_primitive_types = chain->all_primitive_types && (SYMBOL_PRIMITIVE_TYPE == expr->type->flavour);
   chain->all_constant_index_expression = chain->all_constant_index_expression && expr->constant_index_expression;

	return chain;
}

ExprChain* glsl_expr_chain_append(ExprChain* chain, Expr* expr)
{
	ExprChainNode* node = (ExprChainNode *)malloc_fast(sizeof(ExprChainNode));

	node->expr = expr;

	return glsl_expr_chain_append_node(chain, node);
}

StatementChain* glsl_statement_chain_create(void)
{
	StatementChain* chain = (StatementChain *)malloc_fast(sizeof(StatementChain));

	chain->count = 0;

	chain->first = NULL;
	chain->last = NULL;

	return chain;
}

StatementChain* glsl_statement_chain_append(StatementChain* chain, Statement* statement)
{
	StatementChainNode* node = (StatementChainNode *)malloc_fast(sizeof(StatementChainNode));

	node->statement = statement;

	node->unlinked = false;
	node->prev = chain->last;
	node->next = NULL;

	if (!chain->first) chain->first = node;
	if (chain->last) chain->last->next = node;
	chain->last = node;

	chain->count++;

	return chain;
}
