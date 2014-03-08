/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_COMMON_H
#define GLSL_COMMON_H

#include "interface/khronos/common/khrn_int_common.h"

// Enumerations.

typedef enum _ParamQualifier
{
	PARAM_QUAL_IN,
	PARAM_QUAL_OUT,
	PARAM_QUAL_INOUT
} ParamQualifier;

typedef enum _TypeQualifier
{
	TYPE_QUAL_NONE,
	TYPE_QUAL_CONST,
	TYPE_QUAL_ATTRIBUTE,
	TYPE_QUAL_VARYING,
	TYPE_QUAL_INVARIANT_VARYING,
	TYPE_QUAL_UNIFORM,
   TYPE_QUAL_LOOP_INDEX,
} TypeQualifier;

typedef enum _ExprFlavour
{
	EXPR_VALUE,
	EXPR_INSTANCE,
	EXPR_SUBSCRIPT,
	EXPR_FUNCTION_CALL,
	EXPR_PRIM_CONSTRUCTOR_CALL,
	EXPR_TYPE_CONSTRUCTOR_CALL,
	EXPR_FIELD_SELECTOR_STRUCT,
	EXPR_FIELD_SELECTOR_SWIZZLE,
	EXPR_POST_INC,
	EXPR_POST_DEC,
	EXPR_PRE_INC,
	EXPR_PRE_DEC,
	EXPR_ARITH_NEGATE,
	EXPR_LOGICAL_NOT,
	EXPR_MUL,
	EXPR_DIV,
	EXPR_ADD,
	EXPR_SUB,
	EXPR_LESS_THAN,
	EXPR_LESS_THAN_EQUAL,
	EXPR_GREATER_THAN,
	EXPR_GREATER_THAN_EQUAL,
	EXPR_EQUAL,
	EXPR_NOT_EQUAL,
	EXPR_LOGICAL_AND,
	EXPR_LOGICAL_XOR,
	EXPR_LOGICAL_OR,
	EXPR_CONDITIONAL,
	EXPR_ASSIGN,
/*
   removed - lvalues are side-effect free so we construct x *= y as x = x * y

	EXPR_MUL_ASSIGN,
	EXPR_DIV_ASSIGN,
	EXPR_ADD_ASSIGN,
	EXPR_SUB_ASSIGN,
*/
	EXPR_SEQUENCE,

   // Language extensions.
   EXPR_INTRINSIC_TEXTURE_2D_BIAS, // (sampler2D sampler, vec2 coord, float bias)
   EXPR_INTRINSIC_TEXTURE_2D_LOD, // (sampler2D sampler, vec2 coord, float lod)
   EXPR_INTRINSIC_TEXTURE_CUBE_BIAS, // (samplerCube sampler, vec3 coord, float bias)
   EXPR_INTRINSIC_TEXTURE_CUBE_LOD, // (samplerCube sampler, vec3 coord, float lod)
   EXPR_INTRINSIC_RSQRT, // (float a)
   EXPR_INTRINSIC_RCP, // (float a)
   EXPR_INTRINSIC_LOG2, // (float a)
   EXPR_INTRINSIC_EXP2, // (float a)
   EXPR_INTRINSIC_CEIL, // (float a)
   EXPR_INTRINSIC_FLOOR, // (float a)
   EXPR_INTRINSIC_SIGN, // (float a)
   EXPR_INTRINSIC_TRUNC, // (float a)
   EXPR_INTRINSIC_NEAREST, // (float a)
   EXPR_INTRINSIC_MIN, // (float a, float b)
   EXPR_INTRINSIC_MAX, // (float a, float b)

	EXPR_FLAVOUR_COUNT
} ExprFlavour;

typedef enum _StatementFlavour
{
	STATEMENT_AST,
	STATEMENT_DECL_LIST,
	STATEMENT_FUNCTION_DEF,
	STATEMENT_VAR_DECL,
	STATEMENT_COMPOUND,
	STATEMENT_EXPR,
	STATEMENT_SELECTION,
	STATEMENT_ITERATOR_FOR,
	STATEMENT_ITERATOR_WHILE,
	STATEMENT_ITERATOR_DO_WHILE,
	STATEMENT_CONTINUE,
	STATEMENT_BREAK,
	STATEMENT_DISCARD,
	STATEMENT_RETURN,
	STATEMENT_RETURN_EXPR,
	STATEMENT_NULL,
	STATEMENT_FLAVOUR_COUNT
} StatementFlavour;

typedef enum _PrimitiveTypeIndex
{
	PRIM_VOID,
	PRIM_BOOL, PRIM_BVEC2, PRIM_BVEC3, PRIM_BVEC4,
	PRIM_INT, PRIM_IVEC2, PRIM_IVEC3, PRIM_IVEC4,
	PRIM_FLOAT, PRIM_VEC2, PRIM_VEC3, PRIM_VEC4,
	PRIM_MAT2, PRIM_MAT3, PRIM_MAT4,
	PRIM_SAMPLER2D, PRIM_SAMPLERCUBE,
	PRIMITIVE_TYPES_COUNT,
   PRIMITIVE_TYPE_UNDEFINED // this is after PRIMITIVE_TYPES_COUNT; memory is not allocated for it, and it is not iterated over
} PrimitiveTypeIndex;

//
// Forward declarations for symbols.h, ast.h, dataflow.h, const_types.h.
//

// Typedefs.

struct _Map;
typedef struct _Map Map;

typedef Map SymbolTable;

struct _Symbol;
typedef struct _Symbol Symbol;
struct _SymbolType;
typedef struct _SymbolType SymbolType;

struct _Expr;
typedef struct _Expr Expr;
struct _Statement;
typedef struct _Statement Statement;

struct _CallContext;
typedef struct _CallContext CallContext;

struct _ExprChain;
typedef struct _ExprChain ExprChain;
struct _ExprChainNode;
typedef struct _ExprChainNode ExprChainNode;

struct _StatementChain;
typedef struct _StatementChain StatementChain;
struct _StatementChainNode;
typedef struct _StatementChainNode StatementChainNode;

struct _Dataflow;
typedef struct _Dataflow Dataflow;
struct _DataflowChain;
typedef struct _DataflowChain DataflowChain;
struct _DataflowChainNode;
typedef struct _DataflowChainNode DataflowChainNode;
struct _DataflowSource;
typedef struct _DataflowSource DataflowSource;
struct _DataflowSources;
typedef struct _DataflowSources DataflowSources;


// Functions.

void glsl_dataflow_init_linkable_values(Symbol* var);


#endif // COMMON_H
