/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_SYMBOLS_H
#define GLSL_SYMBOLS_H

#include <stdlib.h>
#include <string.h>

#include "interface/khronos/include/GLES2/gl2.h"

#include "middleware/khronos/glsl/glsl_fastmem.h"

#include "middleware/khronos/glsl/glsl_ast.h"
#include "middleware/khronos/glsl/glsl_dataflow.h"

#include "middleware/khronos/glsl/glsl_const_types.h"


// Defines the representation of symbols.
// Symbols bind names to either _types_ or _instances_.
// e.g. in "int x", "int" and "x" are symbols,
// with "int" a symbol type and "x" a symbol instance.

// SymbolType represents types exclusively.
// Symbol represents types or instances:
// - for instances it has a pointer to a SymbolType struct defining the underlying type, and some metadata;
// - for types it has a pointer to the SymbolType struct defining the new type, and the metadata is invalid.


// Types.
//

typedef enum
{
	SYMBOL_PRIMITIVE_TYPE, // these are canonical - can do pointer match
	SYMBOL_STRUCT_TYPE, // these are also canonical
	SYMBOL_ARRAY_TYPE, // these are NOT canonical - cannot do pointer match
	SYMBOL_FUNCTION_TYPE // these are also NOT canonical
} SymbolTypeFlavour;

typedef struct
{
   /*
      index

      Identifies which primitive type this is, so that further information
      can be looked up in the primitive* global arrays.

      Invariants:

      (GL20_COMPILER_PRIMITIVETYPE_INDEX)
      0 <= index < PRIMITIVE_TYPES_COUNT
      i.e. index is a valid PrimitiveTypeIndex enum constant (not including PRIMITIVE_TYPE_UNDEFINED)
   */
	PrimitiveTypeIndex index;
} PrimitiveType;

typedef struct
{
   /*
      member_count

      Invariants:
      member_count >= 0
   */
	int member_count;
   /*
      member_names

      Invariants:
      member_names is a valid pointer to member_count elements
      For all 0 <= i < member_count: member_names[i] is a valid pointer to a null-terminated string
   */
	const char** member_names;
   /*
      member_types

      Invariants:
      member_types is a valid pointer to member_count elements
      For all 0 <= i < member_count:
         member_types[i] is a valid pointer to a SymbolType
         member_types[i]->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE,SYMBOL_ARRAY_TYPE}
   */
	SymbolType** member_types;
} StructType;

typedef struct
{
   /*
      member_count

      Implementation notes:

      Used to have the comment "or 0 if member_count_expression has not been evaluated yet"
      It is believed this no longer applies. TODO: check

      Invariants:

      member_count > 0
   */
	int member_count;
   /*
      member_type

      Invariants:

      member_type is valid for the lifetime of fastmem
      member_type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE}
   */
	SymbolType* member_type;
	Expr* member_count_expression;  // for debugging only
} ArrayType;

typedef struct
{
   /*
      return_type

      Invariants:

      (GL20_COMPILER_FUNCTIONTYPE_RETURN_TYPE)
      return_type != NULL
      return_type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE}
   */
	SymbolType* return_type;
	int param_count;
   /*
      params

      Invariants:

      params is a valid pointer to param_count elements
      params[i]->flavour == SYMBOL_PARAM_INSTANCE for 0 <= i < param_count
   */
	Symbol** params;
} FunctionType;

/*
   Definition of a `compile time instance` ptr of a SymbolType t

   ptr must point to a block of size t->size_as_const
      ...which must be greater than zero

   If t is one of the following primitive types then ptr must point the corresponding C type:
   PRIM_BOOL -> const_bool
   PRIM_BVEC2 -> const_bvec2
   PRIM_BVEC3 -> const_bvec3
   PRIM_BVEC4 -> const_bvec4
   PRIM_INT -> const_int
   PRIM_IVEC2 -> const_ivec2
   PRIM_IVEC3 -> const_ivec3
   PRIM_IVEC4 -> const_ivec4
   PRIM_FLOAT -> const_float
   PRIM_VEC2 -> const_vec2
   PRIM_VEC3 -> const_vec3
   PRIM_VEC4 -> const_vec4
   PRIM_MAT2 -> const_mat2
   PRIM_MAT3 -> const_mat3
   PRIM_MAT4 -> const_mat4

   If t is an array type then ptr must point to an array of t->member_count compile
   time instances of t->u.array_type.member_type

   If t is a structure type then ptr must point to compile time instances of each of
   t->u.struct_type.member_types
*/

struct _SymbolType
{
   /*
      Explains how this type is constructed (whether it is a primitive type or
      whether it is built out of other types as a structure, array or function).

      Khronos documentation:

      -

      Invariants:

      flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE,SYMBOL_ARRAY_TYPE,SYMBOL_FUNCTION_TYPE}
   */
	SymbolTypeFlavour flavour;

	// The name of this type.
	const char* name;

   /*
      The size in bytes of this type as a compile time constant, or 0 if it cannot be a constant.
      Note the set of non-constant types is equal to the set of types that cannot be the target of an assignment.

      Khronos documentation:

      -

      Invariants:

      (GL20_COMPILER_SYMBOLTYPE_SIZE_ASSIGNMENT_TARGET)
      If type can be target of assignment then size_as_const > 0.
      Otherwise size_as_const == 0.

      (GL20_COMPILER_SYMBOLTYPE_SIZE_AS_CONST)
      A compile time instance of this SymbolType has size size_as_const

      (GL20_COMPILER_SYMBOLTYPE_SIZE_STRUCTTYPE)
      If flavour == SYMBOL_STRUCT_TYPE:
         size_as_const = sum {u.struct_type.member_types[i]->size_as_const, for 0 <= i < u.struct_type.member_count}
   */
	int size_as_const;

   // The number of scalar components in this type, or 0 if not a numeric or bool type.
   int scalar_count;

	union
	{
      /*
         primitive_type
         
         Validity: flavour == SYMBOL_PRIMITIVE_TYPE
      */
		PrimitiveType primitive_type;
      /*
         struct_type

         Validity: flavour == SYMBOL_STRUCT_TYPE
      */
		StructType struct_type;
		ArrayType array_type;
      /*
         function_type
         
         Validity: flavour == SYMBOL_FUNCTION_TYPE
      */
		FunctionType function_type;
	} u;
};

//
// Primitive type data.
//

// Flags.
#define PRIM_SCALAR_TYPE			(1 << 0)
#define PRIM_VECTOR_TYPE			(1 << 1)
#define PRIM_MATRIX_TYPE			(1 << 2)
#define PRIM_BOOL_TYPE				(1 << 3)
#define PRIM_INT_TYPE				(1 << 4)
#define PRIM_FLOAT_TYPE				(1 << 5)
#define PRIM_ARITHMETIC_BINARY		(1 << 6)
#define PRIM_ARITHMETIC_UNARY		(1 << 7)
#define PRIM_LOGICAL_BINARY			(1 << 8)
#define PRIM_LOGICAL_UNARY			(1 << 9)
#define PRIM_RELATIONAL				(1 << 10)
#define PRIM_EQUALITY				(1 << 11)
#define PRIM_SAMPLER_TYPE			(1 << 12)
typedef unsigned int PRIMITIVE_TYPE_FLAGS_T;
extern PRIMITIVE_TYPE_FLAGS_T primitiveTypeFlags[PRIMITIVE_TYPES_COUNT];

// Checks that a conversion from the SCALAR type of fromIndex to the SCALAR type of toIndex is valid.
// (e.g. if fromIndex is PRIM_VEC3 and toIndex is PRIM_BOOL, checks that PRIM_FLOAT converts to PRIM_BOOL.)
// Iff value_reader_ptr, *value_reader_ptr, value_writer_ptr, *value_writer_ptr are not NULL,
// additionally does the conversion and advances each pointer appropriately.
bool glsl_single_scalar_type_conversion(PrimitiveTypeIndex from_index, void** value_reader_ptr, PrimitiveTypeIndex to_index, void** value_writer_ptr);



// Attempts to match the given types.
// This is to be used within a single shader.
// Returns false if they cannot be matched.
bool glsl_shallow_match_nonfunction_types(SymbolType* a, SymbolType* b);

// Attempts to match the given types.
// This is to be used between vertex and fragment shaders.
// Returns false if they cannot be matched.
bool glsl_deep_match_nonfunction_types(SymbolType* a, SymbolType* b);

// Finds the function symbol in a Symbol chain that matches the argument types.
// Note array sizes are part of the type and there is no automatic promotion.
// Returns NULL if none found.
Symbol* glsl_resolve_overload_using_arguments(Symbol* head, ExprChain* args);

// Finds the function symbol in a Symbol chain that matches the declaration.
// Overload resolution is, like glsl_resolve_overload_using_arguments() and as per the GLSL rules,
// based purely on argument types. However, parameter qualifiers and return type
// have to match once an overload is found.
// Returns NULL if none found.
Symbol* glsl_resolve_overload_using_prototype(Symbol* head, SymbolType* prototype);

// Returns true iff the type is a sampler type, or is an array containing samplers,
// or is a struct containing (at any depth) one of the above.
bool glsl_type_contains_samplers(SymbolType* t);

// There are type->scalar_count scalars in this type.
// Gets the PrimitiveTypeIndex of scalar n.
PrimitiveTypeIndex glsl_get_scalar_value_type_index(SymbolType* type, int n);


//
// Instances.
//

#define PARAM_QUAL_DEFAULT PARAM_QUAL_IN

#define TYPE_QUAL_DEFAULT TYPE_QUAL_NONE


//
// Symbols.
// i.e. bringing types and instances together.
// Please use the supplied Symbol constructors; it makes refactoring much easier.
//

typedef enum
{
	SYMBOL_TYPE, // named type, as referred to by instances
	SYMBOL_VAR_INSTANCE, // normal variable (instance of primitive/array/struct)
	SYMBOL_PARAM_INSTANCE, // function parameter (instance of primitive/array/struct)
	SYMBOL_FUNCTION_INSTANCE, // function definition (or declaration, to be later upgraded to definition)
	SYMBOL_STRUCT_MEMBER // member of a struct
} SymbolFlavour;

typedef void (*ConstantFunction)(void);

struct _Symbol
{
	const char* name;
   /*
      flavour

      Describes what kind of object this symbol represents.

      Invariants:

      flavour in {SYMBOL_TYPE, SYMBOL_VAR_INSTANCE, SYMBOL_PARAM_INSTANCE, SYMBOL_FUNCTION_INSTANCE, SYMBOL_STRUCT_MEMBER}
   */
	SymbolFlavour flavour;

   /*
      type

      Defined for types, instances and members, though the semantics are slightly different

      TODO: should we add an invariant matching this up with expression type?

      Invariants:

      (GL20_COMPILER_SYMBOL_VAR_OR_PARAM_INSTANCE_TYPE)
      If flavour in {SYMBOL_VAR_INSTANCE, SYMBOL_PARAM_INSTANCE} then
         type is valid for the lifetime of fastmem
         type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE,SYMBOL_ARRAY_TYPE}
      
      (GL20_COMPILER_SYMBOL_FUNCTION_INSTANCE_TYPE)
      If flavour == SYMBOL_FUNCTION_INSTANCE then
         type is valid for the lifetime of fastmem
         type->flavour == SYMBOL_FUNCTION_TYPE
      TODO: fill in the rest of these
   */
	SymbolType* type;

	union // defined only if flavour is SYMBOL_VAR_INSTANCE, SYMBOL_PARAM_INSTANCE or SYMBOL_FUNCTION_INSTANCE
	{
      /*
         var_instance

         Validity:

         flavour in {SYMBOL_VAR_INSTANCE,SYMBOL_PARAM_INSTANCE}
      */
		struct
		{
         /*
            type_qual

            Invariants:

            (GL20_COMPILER_SYMBOL_VAR_INSTANCE_TYPE_QUAL)
            type_qual in {TYPE_QUAL_NONE,TYPE_QUAL_CONST,TYPE_QUAL_ATTRIBUTE,TYPE_QUAL_VARYING,TYPE_QUAL_INVARIANT_VARYING,TYPE_QUAL_UNIFORM}
         */
			TypeQualifier type_qual;
         /*
            param_qual

            Invariants:

            If flavour == SYMBOL_PARAM_INSTANCE then param_qual in {PARAM_QUAL_IN, PARAM_QUAL_OUT, PARAM_QUAL_INOUT}
         */
			ParamQualifier param_qual;

         bool is_local;

         /*
            compile_time_value

            type->size_as_const bytes.
            NULL iff type_qual is not TYPE_QUAL_CONST.

            Invariants:
            If type_qual == TYPE_QUAL_CONST then:
               compile_time_value is valid for the lifetime of fastmem
               compile_time_value is a compile time instance of type
            Else:
               compile_time_value == NULL
         */
			void* compile_time_value;

         // Array of pointers to the Dataflow node that stores the value of each scalar.
         // Array has type->scalar_count members.
         // NULL iff type->scalar_count == 0 or type_qual is TYPE_QUAL_CONST.
         Dataflow** scalar_values;

         bool is_loop_index;
		} var_instance;

      /*
         function_instance

         Validity:

         flavour == SYMBOL_FUNCTION_INSTANCE
      */
		struct
		{
         /*
            const_function_code

            Implementation notes:

            As code pointer, or NULL for non-const functions

            If we are expr->u.function_instance then define
               type = expr->type->u.function_type
               param_count = type.param_count

            Invariants:

            (GL20_COMPILER_SYMBOL_FUNCTION_INSTANCE_CONST_FUNCTION_CODE)
            If const_function_code != NULL:
               param_count in {1,2,3,4}
               const_function_code is a valid function pointer.
               The function prototype is
                  void const_function_code(void *result, const void *arg0, const void *arg1, ..., const void *arg{param_count-1})
               The precondition is
                  arg{i} is a valid pointer to a compile time instance of type.params[i]->type
                  result is a valid pointer to type.return_type->size_as_const bytes
               The postcondition is
                  result is a valid pointer to a compile time instance of type.return_type
         */
			ConstantFunction const_function_code;

         /*
            const_function_body

            Implementation notes:

            pointer to function source for builtin functions; msb set when we see an invocation of the function

            Invariants:

            (GL20_COMPILER_SYMBOL_FUNCTION_INSTANCE_CONST_FUNCTION_BODY)
            If const_function_code != NULL:
               (const_function_body & 0x7fffffff) is a valid pointer to a null-terminated string
         */
         const char *const_function_body;

			Statement* function_def; // as STATEMENT_FUNCTION_DEF, or NULL for declarations

         /*
            next_overload

            Implementation notes:

            The overload list relies on the transitivity of function visibility.
			   That is, if I can see a function f(int) and it can see f(float), I can also see f(float).

            NULL if none exists.

            Invariants:

            (GL20_COMPILER_SYMBOL_FUNCTION_DEF_NEXT)
            If next_overload != NULL then next_overload->flavour == SYMBOL_FUNCTION_INSTANCE
         */
			Symbol* next_overload;
		} function_instance;
	} u;
};


//
// Symbol constructors.
//

static INLINE void glsl_symbol_construct_type(Symbol* result, SymbolType* type)
{
   result->flavour = SYMBOL_TYPE;
   result->name = type->name;
   result->type = type;
}

// These are pretty long, so they are not inlined.
extern void glsl_symbol_construct_var_instance(Symbol* result, const char* name, SymbolType* type, TypeQualifier type_qual, void*compile_time_value);
extern void glsl_symbol_construct_param_instance(Symbol* result, const char* name, SymbolType* type, TypeQualifier type_qual, ParamQualifier param_qual);

static INLINE void glsl_symbol_construct_function_instance(Symbol* result, const char* name, SymbolType* type, ConstantFunction const_function_code, const char *const_function_body, Symbol* next_overload)
{
   result->flavour = SYMBOL_FUNCTION_INSTANCE;
   result->name = name;
   result->type = type;

   result->u.function_instance.const_function_code = const_function_code;
   result->u.function_instance.const_function_body = const_function_body;
   result->u.function_instance.function_def = NULL;
   result->u.function_instance.next_overload = next_overload;
}

static INLINE void glsl_symbol_construct_struct_member(Symbol* result, const char* name, SymbolType* type)
{
   result->flavour = SYMBOL_STRUCT_MEMBER;
   result->name = name;
   result->type = type;
}


//
// Function call metadata.
//

typedef enum
{
   CALL_CONTEXT_FUNCTION,
   CALL_CONTEXT_PRIM_CONSTRUCTOR,
   CALL_CONTEXT_TYPE_CONSTRUCTOR,
   CALL_CONTEXT_INTRINSIC
} CallContextFlavour;

struct _CallContext
{
	CallContextFlavour flavour;

	ExprChain* args;

	union
	{
		struct
		{
			Symbol* symbol;
		} function;

		struct
		{
			PrimitiveTypeIndex index;
		} prim_constructor;

		struct
		{
			Symbol* symbol;
		} type_constructor;

      struct
      {
         ExprFlavour flavour;
      } intrinsic;
	} u;
};


//
// Symbol table operations.
//

// NOTE:
// The symbol table should only be accessed through this interface.
// The implementation may assume that SymbolTable is immutable.

// TODO: there's some stuff in here not directly related to symbols...
void glsl_init_primitive_values(void);

// Creates a new symbol table.
SymbolTable* glsl_symbol_table_new(void);

// Clears all symbols from the table.
SymbolTable* glsl_symbol_table_clear(SymbolTable* table);

// Populates table with built in symbols.
SymbolTable* glsl_symbol_table_populate(SymbolTable* table);

// Enters a new scope.
bool glsl_symbol_table_enter_scope(SymbolTable* table);

// Exits a scope, clearing everything encountered in this scope so far.
bool glsl_symbol_table_exit_scope(SymbolTable* table);

// Looks up a symbol by name in the table.
Symbol *glsl_symbol_table_lookup(SymbolTable* table, const char* name, bool cross_checkpoints);

// Inserts a symbol into the table.
// Fails if the name is NULL.
bool glsl_symbol_table_insert(SymbolTable* table, Symbol* symbol);

// Pops the last element to be added to the table, removing it.
Symbol* glsl_symbol_table_pop(SymbolTable* table);

// Peeks at the last element to be added to the table, without removing it.
Symbol* glsl_symbol_table_peek(SymbolTable* table);



//
// Primitive type data.
//

// SymbolTypes for all primitive types. These are canonical - can do pointer match
extern SymbolType primitiveTypes[PRIMITIVE_TYPES_COUNT];
// GLenum corresponding to each primitive type.
extern GLenum primitiveTypesToGLenums[PRIMITIVE_TYPES_COUNT];
// Symbols for all primitive types (as SYMBOL_PARAM_INSTANCE with empty string names),
// to permit sharing in constant function prototypes.
extern Symbol primitiveParamSymbols[PRIMITIVE_TYPES_COUNT];
// The types returned by the subscript operator, or NULL if that operator is not defined.
extern SymbolType* primitiveTypeSubscriptTypes[PRIMITIVE_TYPES_COUNT];
// The dimension of the type as available to the subscript operator, or 0 if that operator is not defined.
extern int primitiveTypeSubscriptDimensions[PRIMITIVE_TYPES_COUNT];
// The type indices of scalar components, e.g. for PRIM_BVEC3, PRIM_BOOL; for PRIM_FLOAT, PRIM_FLOAT.
extern PrimitiveTypeIndex primitiveScalarTypeIndices[PRIMITIVE_TYPES_COUNT];

#endif // SYMBOLS_H
