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
#include <stdio.h>
#include <string.h>

#include "middleware/khronos/glsl/glsl_globals.h"
#include "middleware/khronos/glsl/glsl_map.h"
#include "middleware/khronos/glsl/glsl_symbols.h"
#include "middleware/khronos/glsl/glsl_errors.h"
#include "middleware/khronos/glsl/glsl_const_functions.h"
#include "middleware/khronos/glsl/glsl_const_types.h"
#include "middleware/khronos/glsl/glsl_fastmem.h"
#include "middleware/khronos/glsl/glsl_stack.h"
#include "middleware/khronos/glsl/glsl_intern.h"
#ifdef __VIDEOCORE4__
#include "middleware/khronos/glsl/2708/glsl_allocator_4.h"
#endif

// xstr(s) expands first, then stringifies it.
#define xstr(s) str(s)
#define str(s) #s

/*
   primitiveTypes

   SymbolTypes for all primitive types. These are canonical - can do pointer match

   Invariants:

   (GL20_COMPILER_PRIMITIVE_TYPES)
   During a compilation run:
      For all 0 <= i < PRIMITIVE_TYPES_COUNT:
         primitiveTypes[i].flavour == SYMBOL_PRIMITIVE_TYPE
*/
SymbolType primitiveTypes[PRIMITIVE_TYPES_COUNT];
#define GL_FLOAT_VEC2                     0x8B50
#define GL_FLOAT_VEC3                     0x8B51
#define GL_FLOAT_VEC4                     0x8B52
#define GL_INT_VEC2                       0x8B53
#define GL_INT_VEC3                       0x8B54
#define GL_INT_VEC4                       0x8B55
#define GL_BOOL                           0x8B56
#define GL_BOOL_VEC2                      0x8B57
#define GL_BOOL_VEC3                      0x8B58
#define GL_BOOL_VEC4                      0x8B59
#define GL_FLOAT_MAT2                     0x8B5A
#define GL_FLOAT_MAT3                     0x8B5B
#define GL_FLOAT_MAT4                     0x8B5C
#define GL_SAMPLER_2D                     0x8B5E
#define GL_SAMPLER_CUBE                   0x8B60
GLenum primitiveTypesToGLenums[PRIMITIVE_TYPES_COUNT] =
{
   GL_NONE, // PRIM_VOID
   GL_BOOL, // PRIM_BOOL
   GL_BOOL_VEC2, // PRIM_BVEC2
   GL_BOOL_VEC3, // PRIM_BVEC3
   GL_BOOL_VEC4, // PRIM_BVEC4
   GL_INT, // PRIM_INT
   GL_INT_VEC2, // PRIM_IVEC2
   GL_INT_VEC3, // PRIM_IVEC3
   GL_INT_VEC4, // PRIM_IVEC4
   GL_FLOAT, // PRIM_FLOAT
   GL_FLOAT_VEC2, // PRIM_VEC2
   GL_FLOAT_VEC3, // PRIM_VEC3
   GL_FLOAT_VEC4, // PRIM_VEC4
   GL_FLOAT_MAT2, // PRIM_MAT2
   GL_FLOAT_MAT3, // PRIM_MAT3
   GL_FLOAT_MAT4, // PRIM_MAT4
   GL_SAMPLER_2D, // PRIM_SAMPLER2D
   GL_SAMPLER_CUBE, // PRIM_SAMPLERCUBE
};
Symbol primitiveParamSymbols[PRIMITIVE_TYPES_COUNT] = {0};
SymbolType* primitiveTypeSubscriptTypes[PRIMITIVE_TYPES_COUNT] = { NULL };
/*
   primitiveTypeSubscriptDimensions

   For primitive types that support subscript indexing, returns the size. Otherwise
   returns zero.

   Invariants:

   (GL20_COMPILER_PRIMITIVE_TYPE_SUBSCRIPT_DIMENSIONS)
   During a compilation run:
      For all 0 <= i < PRIMITIVE_TYPES_COUNT:
         primitiveScalarTypeIndices[i] exists
*/
int primitiveTypeSubscriptDimensions[PRIMITIVE_TYPES_COUNT] =
{
	0, // PRIM_VOID
	0, // PRIM_BOOL
	2, // PRIM_BVEC2
	3, // PRIM_BVEC3
	4, // PRIM_BVEC4
	0, // PRIM_INT
	2, // PRIM_IVEC2
	3, // PRIM_IVEC3
	4, // PRIM_IVEC4
	0, // PRIM_FLOAT
	2, // PRIM_VEC2
	3, // PRIM_VEC3
	4, // PRIM_VEC4
	2, // PRIM_MAT2
	3, // PRIM_MAT3
	4, // PRIM_MAT4
	0, // PRIM_SAMPLER2D
	0, // PRIM_SAMPLERCUBE
};
PrimitiveTypeIndex primitiveScalarTypeIndices[PRIMITIVE_TYPES_COUNT] =
{
	PRIMITIVE_TYPE_UNDEFINED, // PRIM_VOID
	PRIM_BOOL, // PRIM_BOOL
	PRIM_BOOL, // PRIM_BVEC2
	PRIM_BOOL, // PRIM_BVEC3
	PRIM_BOOL, // PRIM_BVEC4
	PRIM_INT, // PRIM_INT
	PRIM_INT, // PRIM_IVEC2
	PRIM_INT, // PRIM_IVEC3
	PRIM_INT, // PRIM_IVEC4
	PRIM_FLOAT, // PRIM_FLOAT
	PRIM_FLOAT, // PRIM_VEC2
	PRIM_FLOAT, // PRIM_VEC3
	PRIM_FLOAT, // PRIM_VEC4
	PRIM_FLOAT, // PRIM_MAT2
	PRIM_FLOAT, // PRIM_MAT3
	PRIM_FLOAT, // PRIM_MAT4
	PRIM_SAMPLER2D, // PRIM_SAMPLER2D
	PRIM_SAMPLERCUBE, // PRIM_SAMPLERCUBE
};
/*
   primitiveTypeFlags

   Gives some extra information for each primitive type

   Invariants:

   (GL20_COMPILER_PRIMITIVE_TYPE_FLAGS)
   During a compilation run:
      For all 0 <= i < PRIMITIVE_TYPES_COUNT:
         primitiveTypesFlags[i] exists
*/
PRIMITIVE_TYPE_FLAGS_T primitiveTypeFlags[PRIMITIVE_TYPES_COUNT] =
{
	0, // PRIM_VOID
	PRIM_SCALAR_TYPE | PRIM_BOOL_TYPE | PRIM_EQUALITY | PRIM_LOGICAL_BINARY | PRIM_LOGICAL_UNARY, // PRIM_BOOL
	PRIM_VECTOR_TYPE | PRIM_BOOL_TYPE | PRIM_EQUALITY, // PRIM_BVEC2
	PRIM_VECTOR_TYPE | PRIM_BOOL_TYPE | PRIM_EQUALITY, // PRIM_BVEC3
	PRIM_VECTOR_TYPE | PRIM_BOOL_TYPE | PRIM_EQUALITY, // PRIM_BVEC4
	PRIM_SCALAR_TYPE | PRIM_INT_TYPE | PRIM_EQUALITY | PRIM_ARITHMETIC_BINARY | PRIM_ARITHMETIC_UNARY | PRIM_RELATIONAL, // PRIM_INT
	PRIM_VECTOR_TYPE | PRIM_INT_TYPE | PRIM_EQUALITY | PRIM_ARITHMETIC_BINARY | PRIM_ARITHMETIC_UNARY, // PRIM_IVEC2
	PRIM_VECTOR_TYPE | PRIM_INT_TYPE | PRIM_EQUALITY | PRIM_ARITHMETIC_BINARY | PRIM_ARITHMETIC_UNARY, // PRIM_IVEC3
	PRIM_VECTOR_TYPE | PRIM_INT_TYPE | PRIM_EQUALITY | PRIM_ARITHMETIC_BINARY | PRIM_ARITHMETIC_UNARY, // PRIM_IVEC4
	PRIM_SCALAR_TYPE | PRIM_FLOAT_TYPE | PRIM_EQUALITY | PRIM_ARITHMETIC_BINARY | PRIM_ARITHMETIC_UNARY | PRIM_RELATIONAL, // PRIM_FLOAT
	PRIM_VECTOR_TYPE | PRIM_FLOAT_TYPE | PRIM_EQUALITY | PRIM_ARITHMETIC_BINARY | PRIM_ARITHMETIC_UNARY, // PRIM_VEC2
	PRIM_VECTOR_TYPE | PRIM_FLOAT_TYPE | PRIM_EQUALITY | PRIM_ARITHMETIC_BINARY | PRIM_ARITHMETIC_UNARY, // PRIM_VEC3
	PRIM_VECTOR_TYPE | PRIM_FLOAT_TYPE | PRIM_EQUALITY | PRIM_ARITHMETIC_BINARY | PRIM_ARITHMETIC_UNARY, // PRIM_VEC4
	PRIM_MATRIX_TYPE | PRIM_FLOAT_TYPE | PRIM_EQUALITY | PRIM_ARITHMETIC_BINARY | PRIM_ARITHMETIC_UNARY, // PRIM_MAT2
	PRIM_MATRIX_TYPE | PRIM_FLOAT_TYPE | PRIM_EQUALITY | PRIM_ARITHMETIC_BINARY | PRIM_ARITHMETIC_UNARY, // PRIM_MAT3
	PRIM_MATRIX_TYPE | PRIM_FLOAT_TYPE | PRIM_EQUALITY | PRIM_ARITHMETIC_BINARY | PRIM_ARITHMETIC_UNARY, // PRIM_MAT4
	PRIM_SAMPLER_TYPE, // PRIM_SAMPLER2D
	PRIM_SAMPLER_TYPE, // PRIM_SAMPLERCUBE
};

void glsl_init_primitive_values(void)
{
   g_BoolFalse = glsl_dataflow_construct_const_bool(LINE_NUMBER_UNDEFINED, CONST_BOOL_FALSE);
   g_BoolTrue = glsl_dataflow_construct_const_bool(LINE_NUMBER_UNDEFINED, CONST_BOOL_TRUE);
   g_IntZero = glsl_dataflow_construct_const_int(LINE_NUMBER_UNDEFINED, 0);
   g_IntOne = glsl_dataflow_construct_const_int(LINE_NUMBER_UNDEFINED, 1);
   g_FloatZero = glsl_dataflow_construct_const_float(LINE_NUMBER_UNDEFINED, CONST_FLOAT_ZERO);
   g_FloatOne = glsl_dataflow_construct_const_float(LINE_NUMBER_UNDEFINED, CONST_FLOAT_ONE);
}

// Initializes primitive types.
static void init_primitive_types(void)
{
	int i;

   glsl_init_primitive_values();

	// Constant sizes.

	primitiveTypes[PRIM_VOID].size_as_const = 0; // undefined, cannot be instantiated

	primitiveTypes[PRIM_BOOL].size_as_const = sizeof(const_bool);
	primitiveTypes[PRIM_BVEC2].size_as_const = sizeof(const_bvec2);
	primitiveTypes[PRIM_BVEC3].size_as_const = sizeof(const_bvec3);
	primitiveTypes[PRIM_BVEC4].size_as_const = sizeof(const_bvec4);

	primitiveTypes[PRIM_INT].size_as_const = sizeof(const_int);
	primitiveTypes[PRIM_IVEC2].size_as_const = sizeof(const_ivec2);
	primitiveTypes[PRIM_IVEC3].size_as_const = sizeof(const_ivec3);
	primitiveTypes[PRIM_IVEC4].size_as_const = sizeof(const_ivec4);

	primitiveTypes[PRIM_FLOAT].size_as_const = sizeof(const_float);
	primitiveTypes[PRIM_VEC2].size_as_const = sizeof(const_vec2);
	primitiveTypes[PRIM_VEC3].size_as_const = sizeof(const_vec3);
	primitiveTypes[PRIM_VEC4].size_as_const = sizeof(const_vec4);

	primitiveTypes[PRIM_MAT2].size_as_const = sizeof(const_mat2);
	primitiveTypes[PRIM_MAT3].size_as_const = sizeof(const_mat3);
	primitiveTypes[PRIM_MAT4].size_as_const = sizeof(const_mat4);

	primitiveTypes[PRIM_SAMPLER2D].size_as_const = 0; // undefined, cannot be const
	primitiveTypes[PRIM_SAMPLERCUBE].size_as_const = 0; // undefined, cannot be const


   // Number of scalar components.

	primitiveTypes[PRIM_VOID].scalar_count = 0; // undefined, cannot be instantiated

	primitiveTypes[PRIM_BOOL].scalar_count = 1;
	primitiveTypes[PRIM_BVEC2].scalar_count = 2;
	primitiveTypes[PRIM_BVEC3].scalar_count = 3;
	primitiveTypes[PRIM_BVEC4].scalar_count = 4;

	primitiveTypes[PRIM_INT].scalar_count = 1;
	primitiveTypes[PRIM_IVEC2].scalar_count = 2;
	primitiveTypes[PRIM_IVEC3].scalar_count = 3;
	primitiveTypes[PRIM_IVEC4].scalar_count = 4;

	primitiveTypes[PRIM_FLOAT].scalar_count = 1;
	primitiveTypes[PRIM_VEC2].scalar_count = 2;
	primitiveTypes[PRIM_VEC3].scalar_count = 3;
	primitiveTypes[PRIM_VEC4].scalar_count = 4;

	primitiveTypes[PRIM_MAT2].scalar_count = 4;
	primitiveTypes[PRIM_MAT3].scalar_count = 9;
	primitiveTypes[PRIM_MAT4].scalar_count = 16;

	primitiveTypes[PRIM_SAMPLER2D].scalar_count = 1; // a reference to the sampler
	primitiveTypes[PRIM_SAMPLERCUBE].scalar_count = 1; // a reference to the sampler


	// Names.

	primitiveTypes[PRIM_VOID].name = "void";

	primitiveTypes[PRIM_BOOL].name = "bool";
	primitiveTypes[PRIM_BVEC2].name = "bvec2";
	primitiveTypes[PRIM_BVEC3].name = "bvec3";
	primitiveTypes[PRIM_BVEC4].name = "bvec4";

	primitiveTypes[PRIM_INT].name = "int";
	primitiveTypes[PRIM_IVEC2].name = "ivec2";
	primitiveTypes[PRIM_IVEC3].name = "ivec3";
	primitiveTypes[PRIM_IVEC4].name = "ivec4";

	primitiveTypes[PRIM_FLOAT].name = "float";
	primitiveTypes[PRIM_VEC2].name = "vec2";
	primitiveTypes[PRIM_VEC3].name = "vec3";
	primitiveTypes[PRIM_VEC4].name = "vec4";

	primitiveTypes[PRIM_MAT2].name = "mat2";
	primitiveTypes[PRIM_MAT3].name = "mat3";
	primitiveTypes[PRIM_MAT4].name = "mat4";

	primitiveTypes[PRIM_SAMPLER2D].name = "sampler2D";
	primitiveTypes[PRIM_SAMPLERCUBE].name = "samplerCube";


   // Finish the types and construct parameter instances from them.

	for (i = 0; i < PRIMITIVE_TYPES_COUNT; i++)
	{
		// Set up all SymbolTypes as primitive types.
		primitiveTypes[i].flavour = SYMBOL_PRIMITIVE_TYPE;
		// and set up indices.
		primitiveTypes[i].u.primitive_type.index = (PrimitiveTypeIndex)i;

		// Set up all Symbols as nameless parameter instances of primitive types.
      glsl_symbol_construct_param_instance(&primitiveParamSymbols[i], "", &primitiveTypes[i], TYPE_QUAL_NONE, PARAM_QUAL_IN);
	}


	// Subscript types.

	primitiveTypeSubscriptTypes[PRIM_VOID] = NULL;

	primitiveTypeSubscriptTypes[PRIM_BOOL] = NULL;
	primitiveTypeSubscriptTypes[PRIM_BVEC2] = &primitiveTypes[PRIM_BOOL];
	primitiveTypeSubscriptTypes[PRIM_BVEC3] = &primitiveTypes[PRIM_BOOL];
	primitiveTypeSubscriptTypes[PRIM_BVEC4] = &primitiveTypes[PRIM_BOOL];

	primitiveTypeSubscriptTypes[PRIM_INT] = NULL;
	primitiveTypeSubscriptTypes[PRIM_IVEC2] = &primitiveTypes[PRIM_INT];
	primitiveTypeSubscriptTypes[PRIM_IVEC3] = &primitiveTypes[PRIM_INT];
	primitiveTypeSubscriptTypes[PRIM_IVEC4] = &primitiveTypes[PRIM_INT];

	primitiveTypeSubscriptTypes[PRIM_FLOAT] = NULL;
	primitiveTypeSubscriptTypes[PRIM_VEC2] = &primitiveTypes[PRIM_FLOAT];
	primitiveTypeSubscriptTypes[PRIM_VEC3] = &primitiveTypes[PRIM_FLOAT];
	primitiveTypeSubscriptTypes[PRIM_VEC4] = &primitiveTypes[PRIM_FLOAT];

	primitiveTypeSubscriptTypes[PRIM_MAT2] = &primitiveTypes[PRIM_VEC2];
	primitiveTypeSubscriptTypes[PRIM_MAT3] = &primitiveTypes[PRIM_VEC3];
	primitiveTypeSubscriptTypes[PRIM_MAT4] = &primitiveTypes[PRIM_VEC4];

	primitiveTypeSubscriptTypes[PRIM_SAMPLER2D] = NULL;
	primitiveTypeSubscriptTypes[PRIM_SAMPLERCUBE] = NULL;
}

static INLINE const_int* allocate_const_int(const_int i)
{
   const_int* p = (const_int *)malloc_fast(sizeof(const_int));
   *p = i;
   return p;
}

#ifdef __VIDEOCORE4__
static const_float cf(float f)
{
   return *(const_float *)&f;
}
#endif

static void init_built_in_vars_and_struct_types(void)
{
   init_primitive_types();
   g_InGlobalScope = true;

   switch (g_ShaderFlavour)
   {
      case SHADER_VERTEX:
         {
            // Vertex shader.

            g_BuiltInVar__gl_Position = (Symbol *)malloc_fast(sizeof(Symbol));
	         glsl_symbol_construct_var_instance(
		         g_BuiltInVar__gl_Position,
		         glsl_intern("gl_Position", false),
               &primitiveTypes[PRIM_VEC4],
		         TYPE_QUAL_NONE,
		         NULL);

            g_BuiltInVar__gl_PointSize = (Symbol *)malloc_fast(sizeof(Symbol));
	         glsl_symbol_construct_var_instance(
		         g_BuiltInVar__gl_PointSize,
		         glsl_intern("gl_PointSize", false),
               &primitiveTypes[PRIM_FLOAT],
		         TYPE_QUAL_NONE,
		         NULL);
         }
         break;

      case SHADER_FRAGMENT:
         {
            // Fragment shader.

            g_BuiltInVar__gl_FragCoord = (Symbol *)malloc_fast(sizeof(Symbol));
	         glsl_symbol_construct_var_instance(
		         g_BuiltInVar__gl_FragCoord,
		         glsl_intern("gl_FragCoord", false),
		         &primitiveTypes[PRIM_VEC4],
		         TYPE_QUAL_NONE,
		         NULL);

#ifdef __VIDEOCORE4__
            {
               Dataflow *df;
               /* TODO: this is hacky */
               df = glsl_dataflow_construct_fragment_get(LINE_NUMBER_UNDEFINED, DATAFLOW_FRAG_GET_X);
               df = glsl_dataflow_construct_unary_op(LINE_NUMBER_UNDEFINED, DATAFLOW_COERCE_TOFLOAT, df);
               df->type_index = PRIM_FLOAT;
               g_BuiltInVar__gl_FragCoord->u.var_instance.scalar_values[0] = df;

#ifdef BCG_FB_LAYOUT
               {
                  Dataflow * df_height, * df_comp, * df_left;
                  df_height = glsl_dataflow_construct_linkable_value(LINE_NUMBER_UNDEFINED, DATAFLOW_UNIFORM, PRIM_FLOAT);
                  df_height->u.linkable_value.row = BACKEND_UNIFORM_FBHEIGHT;
                  df_comp = glsl_dataflow_construct_linkable_value(LINE_NUMBER_UNDEFINED, DATAFLOW_UNIFORM, PRIM_BOOL);
                  df_comp->u.linkable_value.row = BACKEND_UNIFORM_RSO_FORMAT;
                  df = glsl_dataflow_construct_fragment_get(LINE_NUMBER_UNDEFINED, DATAFLOW_FRAG_GET_Y);
                  df = glsl_dataflow_construct_unary_op(LINE_NUMBER_UNDEFINED, DATAFLOW_COERCE_TOFLOAT, df);
                  df->type_index = PRIM_FLOAT;
                  df_left = glsl_dataflow_construct_binary_op(LINE_NUMBER_UNDEFINED, DATAFLOW_SUB, df_height, df);
                  df_comp = glsl_dataflow_construct_cond_op(LINE_NUMBER_UNDEFINED, df_comp, df_left, df);
                  g_BuiltInVar__gl_FragCoord->u.var_instance.scalar_values[1] = df_comp;
               }
#else
               df = glsl_dataflow_construct_fragment_get(LINE_NUMBER_UNDEFINED, DATAFLOW_FRAG_GET_Y);
               df = glsl_dataflow_construct_unary_op(LINE_NUMBER_UNDEFINED, DATAFLOW_COERCE_TOFLOAT, df);
               df->type_index = PRIM_FLOAT;
               g_BuiltInVar__gl_FragCoord->u.var_instance.scalar_values[1] = df;
#endif /* BCG_FB_LAYOUT */

               df = glsl_dataflow_construct_fragment_get(LINE_NUMBER_UNDEFINED, DATAFLOW_FRAG_GET_Z);
               df = glsl_dataflow_construct_unary_op(LINE_NUMBER_UNDEFINED, DATAFLOW_COERCE_TOFLOAT, df);
               df->type_index = PRIM_FLOAT;
               df = glsl_dataflow_construct_binary_op(LINE_NUMBER_UNDEFINED, DATAFLOW_MUL,
                  df,
                  glsl_dataflow_construct_const_float(LINE_NUMBER_UNDEFINED, cf(1.0f/16777215.0f)));
               g_BuiltInVar__gl_FragCoord->u.var_instance.scalar_values[2] = df;
             
               df = glsl_dataflow_construct_fragment_get(LINE_NUMBER_UNDEFINED, DATAFLOW_FRAG_GET_W);
               df = glsl_dataflow_construct_unary_op(LINE_NUMBER_UNDEFINED, DATAFLOW_INTRINSIC_RCP, df);
               g_BuiltInVar__gl_FragCoord->u.var_instance.scalar_values[3] = df;
            }
#else
            g_BuiltInVar__gl_FragCoord->u.var_instance.scalar_values[0] = glsl_dataflow_construct_fragment_get(LINE_NUMBER_UNDEFINED, DATAFLOW_FRAG_GET_X);
            g_BuiltInVar__gl_FragCoord->u.var_instance.scalar_values[1] = glsl_dataflow_construct_fragment_get(LINE_NUMBER_UNDEFINED, DATAFLOW_FRAG_GET_Y);
            g_BuiltInVar__gl_FragCoord->u.var_instance.scalar_values[2] = glsl_dataflow_construct_fragment_get(LINE_NUMBER_UNDEFINED, DATAFLOW_FRAG_GET_Z);
            g_BuiltInVar__gl_FragCoord->u.var_instance.scalar_values[3] = glsl_dataflow_construct_fragment_get(LINE_NUMBER_UNDEFINED, DATAFLOW_FRAG_GET_W);
#endif

            g_BuiltInVar__gl_FrontFacing = (Symbol *)malloc_fast(sizeof(Symbol));
	         glsl_symbol_construct_var_instance(
		         g_BuiltInVar__gl_FrontFacing,
		         glsl_intern("gl_FrontFacing", false),
		         &primitiveTypes[PRIM_BOOL],
		         TYPE_QUAL_NONE,
		         NULL);

            g_BuiltInVar__gl_FrontFacing->u.var_instance.scalar_values[0] = glsl_dataflow_construct_fragment_get(LINE_NUMBER_UNDEFINED, DATAFLOW_FRAG_GET_FF);

            g_BuiltInVar__gl_FragColor = (Symbol *)malloc_fast(sizeof(Symbol));
	         glsl_symbol_construct_var_instance(
		         g_BuiltInVar__gl_FragColor,
		         glsl_intern("gl_FragColor", false),
		         &primitiveTypes[PRIM_VEC4],
		         TYPE_QUAL_NONE,
		         NULL);

            {
               SymbolType* type = (SymbolType *)malloc_fast(sizeof(SymbolType));
               type->flavour = SYMBOL_ARRAY_TYPE;
               type->name = "vec4[" xstr(GL_MAXDRAWBUFFERS) "]";
               type->size_as_const = GL_MAXDRAWBUFFERS * primitiveTypes[PRIM_VEC4].size_as_const;
               type->scalar_count = GL_MAXDRAWBUFFERS * primitiveTypes[PRIM_VEC4].scalar_count;
               type->u.array_type.member_count = GL_MAXDRAWBUFFERS;
               type->u.array_type.member_count_expression = glsl_expr_construct_value_int(GL_MAXDRAWBUFFERS);
               type->u.array_type.member_count_expression->line_num = LINE_NUMBER_UNDEFINED;
               type->u.array_type.member_type = &primitiveTypes[PRIM_VEC4];

               g_BuiltInVar__gl_FragData = (Symbol *)malloc_fast(sizeof(Symbol));
		         glsl_symbol_construct_var_instance(
			         g_BuiltInVar__gl_FragData,
			         glsl_intern("gl_FragData", false),
			         type,
			         TYPE_QUAL_NONE,
			         NULL);
            }

            g_BuiltInVar__gl_PointCoord = (Symbol *)malloc_fast(sizeof(Symbol));
	         glsl_symbol_construct_var_instance(
		         g_BuiltInVar__gl_PointCoord,
		         glsl_intern("gl_PointCoord", false),
		         &primitiveTypes[PRIM_VEC2],
		         TYPE_QUAL_NONE,
		         NULL);

#ifdef __VIDEOCORE4__
            g_BuiltInVar__gl_PointCoord->u.var_instance.scalar_values[0] = glsl_dataflow_construct_linkable_value(LINE_NUMBER_UNDEFINED, DATAFLOW_VARYING, PRIM_FLOAT);
            g_BuiltInVar__gl_PointCoord->u.var_instance.scalar_values[0]->u.linkable_value.row = 0;
#ifdef BCG_FB_LAYOUT
            {
               Dataflow * df_left, * df_right, * df_comp;
               df_right = glsl_dataflow_construct_linkable_value(LINE_NUMBER_UNDEFINED, DATAFLOW_VARYING, PRIM_FLOAT);
               df_right->u.linkable_value.row = 1;
               df_comp = glsl_dataflow_construct_linkable_value(LINE_NUMBER_UNDEFINED, DATAFLOW_UNIFORM, PRIM_BOOL);
               df_comp->u.linkable_value.row = BACKEND_UNIFORM_RSO_FORMAT;
               df_left = glsl_dataflow_construct_binary_op(LINE_NUMBER_UNDEFINED, DATAFLOW_SUB,
                                                           glsl_dataflow_construct_const_float(LINE_NUMBER_UNDEFINED, cf(1.0f)),
                                                           df_right);
               df_comp = glsl_dataflow_construct_cond_op(LINE_NUMBER_UNDEFINED, df_comp, df_left, df_right);
               g_BuiltInVar__gl_PointCoord->u.var_instance.scalar_values[1] = df_comp;
            }
#else
            g_BuiltInVar__gl_PointCoord->u.var_instance.scalar_values[1] = glsl_dataflow_construct_linkable_value(LINE_NUMBER_UNDEFINED, DATAFLOW_VARYING, PRIM_FLOAT);
            g_BuiltInVar__gl_PointCoord->u.var_instance.scalar_values[1]->u.linkable_value.row = 1;
#endif /* BCG_FB_LAYOUT */
#else
            g_BuiltInVar__gl_PointCoord->u.var_instance.scalar_values[0] = glsl_dataflow_construct_fragment_get(LINE_NUMBER_UNDEFINED, DATAFLOW_FRAG_GET_PC_X);
            g_BuiltInVar__gl_PointCoord->u.var_instance.scalar_values[1] = glsl_dataflow_construct_fragment_get(LINE_NUMBER_UNDEFINED, DATAFLOW_FRAG_GET_PC_Y);
#endif

            g_BuiltInVar__discard = (Symbol *)malloc_fast(sizeof(Symbol));
	         glsl_symbol_construct_var_instance(
		         g_BuiltInVar__discard,
		         glsl_intern("$$discard", false),
		         &primitiveTypes[PRIM_BOOL],
		         TYPE_QUAL_NONE,
		         NULL);
         }
         break;

      default:
         UNREACHABLE();
         return;
   }


   // Constants.

   g_BuiltInVar__gl_MaxVertexAttribs = (Symbol *)malloc_fast(sizeof(Symbol));
	glsl_symbol_construct_var_instance(
		g_BuiltInVar__gl_MaxVertexAttribs,
		glsl_intern("gl_MaxVertexAttribs", false),
		&primitiveTypes[PRIM_INT],
		TYPE_QUAL_CONST,
		allocate_const_int(GL_MAXVERTEXATTRIBS));

   g_BuiltInVar__gl_MaxVertexUniformVectors = (Symbol *)malloc_fast(sizeof(Symbol));
	glsl_symbol_construct_var_instance(
		g_BuiltInVar__gl_MaxVertexUniformVectors,
		glsl_intern("gl_MaxVertexUniformVectors", false),
		&primitiveTypes[PRIM_INT],
		TYPE_QUAL_CONST,
		allocate_const_int(GL_MAXVERTEXUNIFORMVECTORS));

   g_BuiltInVar__gl_MaxVaryingVectors = (Symbol *)malloc_fast(sizeof(Symbol));
	glsl_symbol_construct_var_instance(
		g_BuiltInVar__gl_MaxVaryingVectors,
		glsl_intern("gl_MaxVaryingVectors", false),
		&primitiveTypes[PRIM_INT],
		TYPE_QUAL_CONST,
		allocate_const_int(GL_MAXVARYINGVECTORS));

   g_BuiltInVar__gl_MaxVertexTextureImageUnits = (Symbol *)malloc_fast(sizeof(Symbol));
	glsl_symbol_construct_var_instance(
		g_BuiltInVar__gl_MaxVertexTextureImageUnits,
		glsl_intern("gl_MaxVertexTextureImageUnits", false),
		&primitiveTypes[PRIM_INT],
		TYPE_QUAL_CONST,
		allocate_const_int(GL_MAXVERTEXTEXTUREIMAGEUNITS));

   g_BuiltInVar__gl_MaxCombinedTextureImageUnits = (Symbol *)malloc_fast(sizeof(Symbol));
	glsl_symbol_construct_var_instance(
		g_BuiltInVar__gl_MaxCombinedTextureImageUnits,
		glsl_intern("gl_MaxCombinedTextureImageUnits", false),
		&primitiveTypes[PRIM_INT],
		TYPE_QUAL_CONST,
		allocate_const_int(GL_MAXCOMBINEDTEXTUREIMAGEUNITS));

   g_BuiltInVar__gl_MaxTextureImageUnits = (Symbol *)malloc_fast(sizeof(Symbol));
	glsl_symbol_construct_var_instance(
		g_BuiltInVar__gl_MaxTextureImageUnits,
		glsl_intern("gl_MaxTextureImageUnits", false),
		&primitiveTypes[PRIM_INT],
		TYPE_QUAL_CONST,
		allocate_const_int(GL_MAXTEXTUREIMAGEUNITS));

   g_BuiltInVar__gl_MaxFragmentUniformVectors = (Symbol *)malloc_fast(sizeof(Symbol));
	glsl_symbol_construct_var_instance(
		g_BuiltInVar__gl_MaxFragmentUniformVectors,
		glsl_intern("gl_MaxFragmentUniformVectors", false),
		&primitiveTypes[PRIM_INT],
		TYPE_QUAL_CONST,
		allocate_const_int(GL_MAXFRAGMENTUNIFORMVECTORS));

   g_BuiltInVar__gl_MaxDrawBuffers = (Symbol *)malloc_fast(sizeof(Symbol));
	glsl_symbol_construct_var_instance(
		g_BuiltInVar__gl_MaxDrawBuffers,
		glsl_intern("gl_MaxDrawBuffers", false),
		&primitiveTypes[PRIM_INT],
		TYPE_QUAL_CONST,
		allocate_const_int(GL_MAXDRAWBUFFERS));


   // Uniforms.
   {
      const int member_count = 3;
      SymbolType* type;

      type = (SymbolType *)malloc_fast(sizeof(SymbolType));

      type->flavour = SYMBOL_STRUCT_TYPE;
      type->name = glsl_intern("gl_DepthRangeParameters", false);
      type->size_as_const = member_count * primitiveTypes[PRIM_FLOAT].size_as_const;
      type->scalar_count = member_count * primitiveTypes[PRIM_FLOAT].scalar_count;
      type->u.struct_type.member_count = member_count;
      type->u.struct_type.member_names = (const char **)malloc_fast(sizeof(const char*) * member_count);
      type->u.struct_type.member_names[0] = "near";
      type->u.struct_type.member_names[1] = "far";
      type->u.struct_type.member_names[2] = "diff";
      type->u.struct_type.member_types = (SymbolType **)malloc_fast(sizeof(SymbolType*) * member_count);
      type->u.struct_type.member_types[0] = &primitiveTypes[PRIM_FLOAT];
      type->u.struct_type.member_types[1] = &primitiveTypes[PRIM_FLOAT];
      type->u.struct_type.member_types[2] = &primitiveTypes[PRIM_FLOAT];

      g_BuiltInType__gl_DepthRangeParameters = (Symbol *)malloc_fast(sizeof(Symbol));
      glsl_symbol_construct_type(g_BuiltInType__gl_DepthRangeParameters, type);
   }

   g_BuiltInVar__gl_DepthRange = (Symbol *)malloc_fast(sizeof(Symbol));
	glsl_symbol_construct_var_instance(
		g_BuiltInVar__gl_DepthRange,
		glsl_intern("gl_DepthRange", false),
      g_BuiltInType__gl_DepthRangeParameters->type,
		TYPE_QUAL_UNIFORM,
		NULL);

   g_BuiltInVar__gl_DepthRange->u.var_instance.scalar_values[0] = glsl_dataflow_construct_linkable_value(LINE_NUMBER_UNDEFINED, DATAFLOW_UNIFORM, PRIM_FLOAT);
   g_BuiltInVar__gl_DepthRange->u.var_instance.scalar_values[0]->u.linkable_value.name = "gl_DepthRange.near";
#ifdef __VIDEOCORE4__
   g_BuiltInVar__gl_DepthRange->u.var_instance.scalar_values[0]->u.linkable_value.row = BACKEND_UNIFORM_DEPTHRANGE_NEAR;
#else
   g_BuiltInVar__gl_DepthRange->u.var_instance.scalar_values[0]->u.linkable_value.row = SLANG_GL_DEPTHRANGE_NEAR_ROW;
#endif

   g_BuiltInVar__gl_DepthRange->u.var_instance.scalar_values[1] = glsl_dataflow_construct_linkable_value(LINE_NUMBER_UNDEFINED, DATAFLOW_UNIFORM, PRIM_FLOAT);
   g_BuiltInVar__gl_DepthRange->u.var_instance.scalar_values[1]->u.linkable_value.name = "gl_DepthRange.far";
#ifdef __VIDEOCORE4__
   g_BuiltInVar__gl_DepthRange->u.var_instance.scalar_values[1]->u.linkable_value.row = BACKEND_UNIFORM_DEPTHRANGE_FAR;
#else
   g_BuiltInVar__gl_DepthRange->u.var_instance.scalar_values[1]->u.linkable_value.row = SLANG_GL_DEPTHRANGE_FAR_ROW;
#endif

   g_BuiltInVar__gl_DepthRange->u.var_instance.scalar_values[2] = glsl_dataflow_construct_linkable_value(LINE_NUMBER_UNDEFINED, DATAFLOW_UNIFORM, PRIM_FLOAT);
   g_BuiltInVar__gl_DepthRange->u.var_instance.scalar_values[2]->u.linkable_value.name = "gl_DepthRange.diff";
#ifdef __VIDEOCORE4__
   g_BuiltInVar__gl_DepthRange->u.var_instance.scalar_values[2]->u.linkable_value.row = BACKEND_UNIFORM_DEPTHRANGE_DIFF;
#else
   g_BuiltInVar__gl_DepthRange->u.var_instance.scalar_values[2]->u.linkable_value.row = SLANG_GL_DEPTHRANGE_DIFF_ROW;
#endif
}

static void populate_with_vars_and_struct_types(SymbolTable* symbol_table)
{
	init_built_in_vars_and_struct_types();

	switch (g_ShaderFlavour)
   {
      case SHADER_VERTEX:
			// Vertex shader.
			glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_Position);
			glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_PointSize);
         break;

      case SHADER_FRAGMENT:
			// Fragment shader.
			glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_FragCoord);
			glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_FrontFacing);
			glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_FragColor);
			glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_FragData);
			glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_PointCoord);
         glsl_symbol_table_insert(symbol_table, g_BuiltInVar__discard);
         break;

      default:
         UNREACHABLE();
         return;
   }

	// Constants.
	glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_MaxVertexAttribs);
	glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_MaxVertexUniformVectors);
	glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_MaxVaryingVectors);
	glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_MaxVertexTextureImageUnits);
	glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_MaxCombinedTextureImageUnits);
	glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_MaxTextureImageUnits);
	glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_MaxFragmentUniformVectors);
	glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_MaxDrawBuffers);

	// Uniforms.
	glsl_symbol_table_insert(symbol_table, g_BuiltInType__gl_DepthRangeParameters);
	glsl_symbol_table_insert(symbol_table, g_BuiltInVar__gl_DepthRange);
}

SymbolTable* glsl_symbol_table_new(void)
{
	Map* table;

	table = glsl_map_new();
	if (!table) return NULL;

   // Init primitive types.
	// init_primitive_types();

   return table;
}

SymbolTable* glsl_symbol_table_clear(SymbolTable* table)
{
   return glsl_map_clear(table);
}

SymbolTable* glsl_symbol_table_populate(SymbolTable* table)
{
   // Put const functions in the outermost scope.
	glsl_populate_with_const_functions(table);

	// Create the global scope.
	glsl_symbol_table_enter_scope(table);

   // Put built in vars in the global scope.
   populate_with_vars_and_struct_types(table);

	return table;
}

bool glsl_symbol_table_enter_scope(SymbolTable* table)
{
	return glsl_map_set_checkpoint(table);
}

bool glsl_symbol_table_exit_scope(SymbolTable* table)
{
	return glsl_map_clear_to_checkpoint(table);
}

Symbol *glsl_symbol_table_lookup(SymbolTable* table, const char* name, bool cross_checkpoints)
{
   return (Symbol *)glsl_map_get(table, name, cross_checkpoints);
}

bool glsl_symbol_table_insert(SymbolTable* table, Symbol* symbol)
{
	return glsl_map_put(table, symbol->name, symbol);
}

Symbol* glsl_symbol_table_pop(SymbolTable* table)
{
	return (Symbol*)glsl_map_pop(table);
}

Symbol* glsl_symbol_table_peek(SymbolTable* table)
{
	return (Symbol*)(table->head->v);
}

/*
   bool glsl_shallow_match_nonfunction_types(SymbolType* a, SymbolType* b)

   Returns whether two types are identical. This will not match identically defined
   structure types between the fragment and vertex shaders.

   Khronos documentation:

   -

   Implementation notes:

   Relies on the fact that primitive and structure types are canonical and can
   be compared with pointer equality, and that arrays can only be built from
   primitive and structure types (i.e. no multidimensional arrays or arrays of
   functions).
   However structure types are not canonical between the fragment and vertex
   shader, so this function is not appropriate for determining whether fragment
   and vertex shaders will link together.

   Preconditions:

   a points to a valid SymbolType
   b points to a valid SymbolType
   a->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE,SYMBOL_ARRAY_TYPE}
   b->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE,SYMBOL_ARRAY_TYPE}

   Postconditions:

   -

   Invariants preserved:

   -
*/

bool glsl_shallow_match_nonfunction_types(SymbolType* a, SymbolType* b)
{
	if (a == b)
	{
		// Certain match for SYMBOL_PRIMITIVE_TYPE or SYMBOL_STRUCT_TYPE.
		return true;
	} else if (SYMBOL_ARRAY_TYPE == a->flavour && SYMBOL_ARRAY_TYPE == b->flavour)
	{
		// Array types are not canonical, so do a manual match.
      vcos_assert(a->u.array_type.member_count && b->u.array_type.member_count);
		if (a->u.array_type.member_count == b->u.array_type.member_count)
		{
			if (a->u.array_type.member_type == b->u.array_type.member_type)
			{
				// Match for SYMBOL_ARRAY_TYPE. (Note, arrays cannot be multidimensional or nested.)
				return true;
			}
		}
	}

	return false;
}

bool glsl_deep_match_nonfunction_types(SymbolType* a, SymbolType* b)
{
   STACK_CHECK();

   if (a->flavour != b->flavour)
   {
      return false;
   }

   switch (a->flavour)
   {
      case SYMBOL_PRIMITIVE_TYPE:
         {
            if (a->u.primitive_type.index != b->u.primitive_type.index)
            {
               return false;
            }
         }
         break;

      case SYMBOL_STRUCT_TYPE:
         {
            int i;

            if (a->u.struct_type.member_count != b->u.struct_type.member_count)
            {
               return false;
            }

            for (i = 0; i < a->u.struct_type.member_count; i++)
            {
               if (strcmp(a->u.struct_type.member_names[i], b->u.struct_type.member_names[i]) != 0)
               {
                  return false;
               }

               if (!glsl_deep_match_nonfunction_types(a->u.struct_type.member_types[i], b->u.struct_type.member_types[i]))
               {
                  return false;
               }
            }
         }
         break;

      case SYMBOL_ARRAY_TYPE:
         {
            if (a->u.array_type.member_count != b->u.array_type.member_count)
            {
               return false;
            }

            if (!glsl_deep_match_nonfunction_types(a->u.array_type.member_type, b->u.array_type.member_type))
            {
               return false;
            }
         }
         break;

      default:
         UNREACHABLE();
         return false;
   }

   return true;
}

/*
   bool glsl_single_scalar_type_conversion(PrimitiveTypeIndex from_index, void** value_reader_ptr, PrimitiveTypeIndex to_index, void** value_writer_ptr)

   -

   Khronos documentation:

   -

   Implementation notes:

   Define:
      from_size =
         sizeof(const_bool)      if from_flags & PRIM_BOOL_TYPE
         sizeof(const_int)       if from_flags & PRIM_INT_TYPE
         sizeof(const_float)     if from_flags & PRIM_FLOAT_TYPE
      to_size =
         sizeof(const_bool)      if to_flags & PRIM_BOOL_TYPE
         sizeof(const_int)       if to_flags & PRIM_INT_TYPE
         sizeof(const_float)     if to_flags & PRIM_FLOAT_TYPE

   Preconditions:

   0 <= to_index < PRIMITIVE_TYPES_COUNT
   0 <= from_index < PRIMITIVE_TYPES_COUNT
   Either:
      value_read_ptr is a pointer to NULL
   Or:
      value_read_ptr is a pointer to a pointer to an object of size from_size

   Either:
      value_write_ptr is a pointer to NULL
   Or:
      value_write_ptr is a pointer to a pointer to an object of size to_size


   Postconditions:

   Either:
      *value_read_ptr has been advanced by from_size
      *value_write_ptr has been advanced by to_size
      return value is true
   Or:
      *value_read_ptr is left unchanged
      *value_write_ptr is left unchanged
      return value is false

   Invariants preserved:

   -

   Invariants used:

   (GL20_COMPILER_PRIMITIVE_TYPE_FLAGS)
*/

bool glsl_single_scalar_type_conversion(PrimitiveTypeIndex from_index, void** value_reader_ptr, PrimitiveTypeIndex to_index, void** value_writer_ptr)
{
	PRIMITIVE_TYPE_FLAGS_T from_flags, to_flags;
	bool convert;

	from_flags = primitiveTypeFlags[from_index];
	to_flags = primitiveTypeFlags[to_index];
	convert = value_writer_ptr && *value_writer_ptr && value_reader_ptr && *value_reader_ptr;

	switch (from_flags & (PRIM_BOOL_TYPE | PRIM_INT_TYPE | PRIM_FLOAT_TYPE))
	{
		case PRIM_BOOL_TYPE:
			switch (to_flags & (PRIM_BOOL_TYPE | PRIM_INT_TYPE | PRIM_FLOAT_TYPE))
			{
				case PRIM_BOOL_TYPE:
					if (convert) {
					   *(const_bool*)*value_writer_ptr = const_bool_from_bool(*(const_bool*)*value_reader_ptr);
					   *value_reader_ptr = (const_bool*)*value_reader_ptr + 1;
					   *value_writer_ptr = (const_bool*)*value_writer_ptr + 1;
					}
					break;
				case PRIM_INT_TYPE:
					if (convert) {
					   *(const_int*)*value_writer_ptr = const_int_from_bool(*(const_bool*)*value_reader_ptr);
					   *value_reader_ptr = (const_bool*)*value_reader_ptr + 1;
					   *value_writer_ptr = (const_int*)*value_writer_ptr + 1;
					}
					break;
				case PRIM_FLOAT_TYPE:
					if (convert) {
					   *(const_float*)*value_writer_ptr = const_float_from_bool(*(const_bool*)*value_reader_ptr);
					   *value_reader_ptr = (const_bool*)*value_reader_ptr + 1;
					   *value_writer_ptr = (const_float*)*value_writer_ptr + 1;
					}
					break;
				default:
					return false;
			}
			break;
		case PRIM_INT_TYPE:
			switch (to_flags & (PRIM_BOOL_TYPE | PRIM_INT_TYPE | PRIM_FLOAT_TYPE))
			{
				case PRIM_BOOL_TYPE:
					if (convert) {
					   *(const_bool*)*value_writer_ptr = const_bool_from_int(*(const_int*)*value_reader_ptr);
					   *value_reader_ptr = (const_int*)*value_reader_ptr + 1;
					   *value_writer_ptr = (const_bool*)*value_writer_ptr + 1;
					}
					break;
				case PRIM_INT_TYPE:
					if (convert) {
					   *(const_int*)*value_writer_ptr = const_int_from_int(*(const_int*)*value_reader_ptr);
					   *value_reader_ptr = (const_int*)*value_reader_ptr + 1;
					   *value_writer_ptr = (const_int*)*value_writer_ptr + 1;
					}
					break;
				case PRIM_FLOAT_TYPE:
					if (convert) {
					   *(const_float*)*value_writer_ptr = const_float_from_int(*(const_int*)*value_reader_ptr);
					   *value_reader_ptr = (const_int*)*value_reader_ptr + 1;
					   *value_writer_ptr = (const_float*)*value_writer_ptr + 1;
					}
					break;
				default:
					return false;
			}
			break;
		case PRIM_FLOAT_TYPE:
			switch (to_flags & (PRIM_BOOL_TYPE | PRIM_INT_TYPE | PRIM_FLOAT_TYPE))
			{
				case PRIM_BOOL_TYPE:
					if (convert) {
					   *(const_bool*)*value_writer_ptr = const_bool_from_float(*(const_float*)*value_reader_ptr);
					   *value_reader_ptr = (const_float*)*value_reader_ptr + 1;
					   *value_writer_ptr = (const_bool*)*value_writer_ptr + 1;
					}
					break;
				case PRIM_INT_TYPE:
					if (convert) {
					   *(const_int*)*value_writer_ptr = const_int_from_float(*(const_float*)*value_reader_ptr);
					   *value_reader_ptr = (const_float*)*value_reader_ptr + 1;
					   *value_writer_ptr = (const_int*)*value_writer_ptr + 1;
					}
					break;
				case PRIM_FLOAT_TYPE:
					if (convert) {
					   *(const_float*)*value_writer_ptr = const_float_from_float(*(const_float*)*value_reader_ptr);
					   *value_reader_ptr = (const_float*)*value_reader_ptr + 1;
					   *value_writer_ptr = (const_float*)*value_writer_ptr + 1;
					}
					break;
				default:
					return false;
			}
			break;
		default:
			return false;
	}

	return true;
}
/*
   Symbol* glsl_resolve_overload_using_arguments(Symbol* head, ExprChain* args)

   Khronos documentation:

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

   If head != NULL:
      head is valid for the lifetime of fastmem
      head->flavour == SYMBOL_FUNCTION_INSTANCE
   args is valid for the lifetime of fastmem
   head and args belong to the same shader

   Postconditions:

   May raise the following errors:

	C0016: Argument cannot be used as out or inout parameter

   Otherwise returns normally.

   If result != NULL
      Result is valid for the lifetime of fastmem
      result->flavour == SYMBOL_FUNCTION_INSTANCE
      result->type->u.function_type.param_count == args->count
      result->type->u.function_type.params[i]->type == args->a[i]->type for 0 <= i < args->count

   Invariants preserved:

   -

   Invariants used:

   (GL20_COMPILER_SYMBOL_FUNCTION_DEF_NEXT)
   (GL20_COMPILER_SYMBOL_FUNCTION_INSTANCE_TYPE)
   (GL20_COMPILER_EXPRCHAINNODE_EXPR)
   (GL20_COMPILER_EXPRCHAINNODE_NEXT)
   (GL20_COMPILER_SYMBOL_VAR_OR_PARAM_INSTANCE_TYPE)
*/

Symbol* glsl_resolve_overload_using_arguments(Symbol* head, ExprChain* args)
{
	ExprChainNode* arg;
	SymbolType* existing_prototype;
	Symbol* existing_prototype_param;
	int i, param_count;

	param_count = args->count;

	for ( ; head; head = head->u.function_instance.next_overload)
	{
      /*
         Loop invariants:

         head->flavour == SYMBOL_FUNCTION_INSTANCE
      */
		arg = args->first;

		existing_prototype = head->type;

		vcos_assert(SYMBOL_FUNCTION_INSTANCE == head->flavour);
		vcos_assert(SYMBOL_FUNCTION_TYPE == existing_prototype->flavour);

		// First check that the number of params is the same.
		if (existing_prototype->u.function_type.param_count != param_count)
		{
			continue; // next prototype
		}
		else
		{
			// Next match the param types, one by one.
			bool params_match = true;

			for (i = 0; i < param_count; i++, arg = arg->next)
			{
            /*
               Loop invariants:

               param_count-i is the number of applications of ->next needed to get from arg to NULL
            */
				existing_prototype_param = existing_prototype->u.function_type.params[i];

				if (PARAM_QUAL_INOUT == existing_prototype_param->u.var_instance.param_qual
					|| PARAM_QUAL_OUT == existing_prototype_param->u.var_instance.param_qual)
				{
					// Check that the argument can be passed to an out/inout parameter.
					if (!(glsl_get_lvalue_flags(arg->expr) & LVF_OUT_OR_INOUT_PARAM))
					{
						glsl_compile_error(ERROR_CUSTOM, 16, arg->expr->line_num, NULL);
					}
				}


				if (glsl_shallow_match_nonfunction_types(arg->expr->type, existing_prototype_param->type))
				{
					continue; // next pair
				}
				else
				{
					params_match = false;
					break; // stop matching pairs
				}
			}

			if (!params_match)
			{
				continue; // next prototype
			}
		}

		// Success!
		return head;
	}

	return NULL;
}

Symbol* glsl_resolve_overload_using_prototype(Symbol* head, SymbolType* prototype)
{
	SymbolType* existing_prototype;
	Symbol* existing_prototype_param;
	Symbol* prototype_param;
	int i, param_count;

	vcos_assert(SYMBOL_FUNCTION_TYPE == prototype->flavour);

	param_count = prototype->u.function_type.param_count;

	for ( ; head; head = head->u.function_instance.next_overload)
	{
		existing_prototype = head->type;

		vcos_assert(SYMBOL_FUNCTION_INSTANCE == head->flavour);
		vcos_assert(SYMBOL_FUNCTION_TYPE == existing_prototype->flavour);

		// First check that the number of params is the same.
		if (existing_prototype->u.function_type.param_count != param_count)
		{
			continue; // next prototype
		}
		else
		{
			// Next match the param types, one by one.
			bool params_match = true;

			for (i = 0; i < param_count; i++)
			{
				existing_prototype_param = existing_prototype->u.function_type.params[i];
				prototype_param = prototype->u.function_type.params[i];

				if (glsl_shallow_match_nonfunction_types(prototype_param->type, existing_prototype_param->type))
				{
					continue; // next pair
				}
				else
				{
					params_match = false;
					break; // stop matching pairs
				}
			}

			if (!params_match)
			{
				continue; // next prototype
			}
		}

		// Under the overloading rules we've found a match, and CANNOT consider any further overloads.
		// However, there are some further tests to pass.

		// The return types must match.
		if (existing_prototype->u.function_type.return_type != prototype->u.function_type.return_type)
		{
			// Equality of pointers is good for SYMBOL_PRIMITIVE_TYPE or SYMBOL_STRUCT_TYPE.
			// No need to check further for SYMBOL_ARRAY_TYPE as functions cannot return arrays.
			glsl_compile_error(ERROR_SEMANTIC, 42, g_LineNumber, NULL);
		}

		// Parameter qualifiers must be consistent between declaration and definition.
		for (i = 0; i < param_count; i++)
		{
			existing_prototype_param = existing_prototype->u.function_type.params[i];
			prototype_param = prototype->u.function_type.params[i];

			if (prototype_param->u.var_instance.param_qual != existing_prototype_param->u.var_instance.param_qual)
			{
				glsl_compile_error(ERROR_SEMANTIC, 43, g_LineNumber, NULL);
			}
		}

		// Success!
		return head;
	}

	return NULL;
}

bool glsl_type_contains_samplers(SymbolType* t)
{
   STACK_CHECK();

	switch (t->flavour)
	{
		case SYMBOL_PRIMITIVE_TYPE:
			return !!(primitiveTypeFlags[t->u.primitive_type.index] & PRIM_SAMPLER_TYPE);

		case SYMBOL_STRUCT_TYPE:
			{
				bool contains_samplers = false;
				int i, n = t->u.struct_type.member_count;

				for (i = 0; i < n; i++)
				{
					// Short-circuit if possible.
					contains_samplers = contains_samplers || glsl_type_contains_samplers(t->u.struct_type.member_types[i]);
				}

				return contains_samplers;
			}

		case SYMBOL_ARRAY_TYPE:
			return glsl_type_contains_samplers(t->u.array_type.member_type);

		case SYMBOL_FUNCTION_TYPE:
			// fall
		default:
			UNREACHABLE();
			return false;
	}
}

PrimitiveTypeIndex glsl_get_scalar_value_type_index(SymbolType* type, int n)
{
   int i;

   STACK_CHECK();

   vcos_assert(0 <= n && n < type->scalar_count);

   switch (type->flavour)
	{
		case SYMBOL_PRIMITIVE_TYPE:
         {
            vcos_assert(PRIMITIVE_TYPE_UNDEFINED != primitiveScalarTypeIndices[type->u.primitive_type.index]);
            return primitiveScalarTypeIndices[type->u.primitive_type.index];
         }

		case SYMBOL_STRUCT_TYPE:
			{
            for (i = 0; i < type->u.struct_type.member_count; i++)
            {
               int cmp = n - type->u.struct_type.member_types[i]->scalar_count;
               if (cmp < 0) break;
               n = cmp;
            }

            return glsl_get_scalar_value_type_index(type->u.struct_type.member_types[i], n);
			}

		case SYMBOL_ARRAY_TYPE:
         {
            return glsl_get_scalar_value_type_index(type->u.array_type.member_type, n % type->u.array_type.member_type->scalar_count);
         }

		default:
			UNREACHABLE();
			return PRIM_VOID;
	}
}

void glsl_symbol_construct_var_instance(Symbol* result, const char* name, SymbolType* type, TypeQualifier type_qual, void*compile_time_value)
{
   // Set common symbol parameters.
   result->flavour = SYMBOL_VAR_INSTANCE;
   result->name = name;
   result->type = type;

   if (type_qual == TYPE_QUAL_LOOP_INDEX)
   {
      type_qual = TYPE_QUAL_NONE;
      result->u.var_instance.is_loop_index = true;
   }
   else
   {
      result->u.var_instance.is_loop_index = false;
   }

   // Set var instance symbol parameters.
   result->u.var_instance.type_qual = type_qual;
   result->u.var_instance.compile_time_value = compile_time_value;
   result->u.var_instance.is_local = !g_InGlobalScope;

   // Set scalar dataflow nodes if this is an lvalue.
   if (type->scalar_count && type_qual != TYPE_QUAL_CONST)
   {
      int i;

      // Allocate space for every scalar in this variable.
      result->u.var_instance.scalar_values = (Dataflow **)malloc_fast(sizeof(Dataflow*) * type->scalar_count);

      // Set every scalar:
      // - for samplers: construct sampler node with index SAMPLER_LOCATION_UNDEFINED;
      // - for uniform/varying/attribute: construct linkable value node with index LINKABLE_VALUE_ROW_UNDEFINED;
      // - for non-uniform/varying/attribute bool/int/float: set to one of the uninitialized variables.
      switch (type_qual)
      {
         case TYPE_QUAL_NONE:
            {
               for (i = 0; i < type->scalar_count; i++)
               {
                  PrimitiveTypeIndex type_index = glsl_get_scalar_value_type_index(type, i);

                  switch (type_index)
                  {
                     case PRIM_BOOL:
                        vcos_assert(g_BoolFalse);
                        result->u.var_instance.scalar_values[i] = g_BoolFalse;
                        break;

                     case PRIM_INT:
                        vcos_assert(g_IntZero);
                        result->u.var_instance.scalar_values[i] = g_IntZero;
                        break;

                     case PRIM_FLOAT:
                        vcos_assert(g_FloatZero);
                        result->u.var_instance.scalar_values[i] = g_FloatZero;
                        break;

                     case PRIM_SAMPLER2D:
                     case PRIM_SAMPLERCUBE:
                        // Although the user cannot make TYPE_QUAL_NONE (i.e. non-uniform) sampler var instances,
                        // we need this ability for function inlining.
                        result->u.var_instance.scalar_values[i] = glsl_dataflow_construct_const_sampler(LINE_NUMBER_UNDEFINED, type_index);
                        break;

                     default:
                        UNREACHABLE();
                        return;
                  }
               }
            }
            return;

         case TYPE_QUAL_CONST:
            UNREACHABLE(); // we shouldn't follow this branch for TYPE_QUAL_CONST
            return;

         case TYPE_QUAL_ATTRIBUTE:
         case TYPE_QUAL_VARYING:
         case TYPE_QUAL_INVARIANT_VARYING:
         case TYPE_QUAL_UNIFORM:
            {
               DataflowFlavour dataflow_flavour;
               DataflowSource* dataflow_source;

               switch (type_qual)
               {
                  case TYPE_QUAL_ATTRIBUTE:
                     dataflow_flavour = DATAFLOW_ATTRIBUTE;
                     break;

                  case TYPE_QUAL_VARYING:
                  case TYPE_QUAL_INVARIANT_VARYING:
                     dataflow_flavour = DATAFLOW_VARYING;
                     break;

                  case TYPE_QUAL_UNIFORM:
                     dataflow_flavour = DATAFLOW_UNIFORM;
                     break;

                  default:
                     UNREACHABLE();
                     return;
               }

               // Save metadata to dataflow_source.
               dataflow_source = (DataflowSource *)malloc_fast(sizeof(DataflowSource));
               dataflow_source->source = result;
               dataflow_source->initial_scalar_values = (Dataflow **)malloc_fast(sizeof(Dataflow*) * type->scalar_count);

               for (i = 0; i < type->scalar_count; i++)
               {
                  Dataflow *dataflow;
                  PrimitiveTypeIndex type_index = glsl_get_scalar_value_type_index(type, i);

                  switch (type_index)
                  {
                     case PRIM_BOOL:
                     case PRIM_INT:
                     case PRIM_FLOAT:
                        dataflow = glsl_dataflow_construct_linkable_value(LINE_NUMBER_UNDEFINED, dataflow_flavour, type_index);
                        break;

                     case PRIM_SAMPLER2D:
                     case PRIM_SAMPLERCUBE:
                        dataflow = glsl_dataflow_construct_const_sampler(LINE_NUMBER_UNDEFINED, type_index);
                        break;

                     default:
                        UNREACHABLE();
                        return;
                  }

                  dataflow_source->initial_scalar_values[i] = dataflow;

                  if (dataflow_flavour == DATAFLOW_VARYING) {
                     if (g_ShaderFlavour == SHADER_VERTEX) {
                        switch (type_index) {
                        case PRIM_BOOL:
                           vcos_assert(g_BoolFalse);
                           dataflow = g_BoolFalse;
                           break;
                        case PRIM_INT:
                           vcos_assert(g_IntZero);
                           dataflow = g_IntZero;
                           break;
                        case PRIM_FLOAT:
                           vcos_assert(g_FloatZero);
                           dataflow = g_FloatZero;
                           break;
                        default:
                           UNREACHABLE();        // can't have varying samplers
                           break;
                        }
                     } else {
#ifdef __VIDEOCORE4__
                        //dataflow = glsl_dataflow_construct_varying_tree(LINE_NUMBER_UNDEFINED, dataflow);
#endif
                     }
                  }
                  result->u.var_instance.scalar_values[i] = dataflow;
               }

               dataflow_source->active = false;
               dataflow_source->handled = false;

               // Put it in g_DataflowSources.
               switch (type_qual)
               {
                  case TYPE_QUAL_ATTRIBUTE:
                     glsl_map_put(g_DataflowSources->attributes, name, dataflow_source);
                     break;

                  case TYPE_QUAL_VARYING:
                  case TYPE_QUAL_INVARIANT_VARYING:
                     glsl_map_put(g_DataflowSources->varyings, name, dataflow_source);
                     break;

                  case TYPE_QUAL_UNIFORM:
                     glsl_map_put(g_DataflowSources->uniforms, name, dataflow_source);
                     break;

                  default:
                     UNREACHABLE();
                     return;
               }
            }
            return;

         default:
            UNREACHABLE();
            return;
      }
   }
   else
   {
      result->u.var_instance.scalar_values = NULL;
      return;
   }
}

void glsl_symbol_construct_param_instance(Symbol* result, const char* name, SymbolType* type, TypeQualifier type_qual, ParamQualifier param_qual)
{
   int i;

   // Set common symbol parameters.
   result->flavour = SYMBOL_PARAM_INSTANCE;
   result->name = name;
   result->type = type;

   // Set param instance symbol parameters.
   result->u.var_instance.type_qual = type_qual;
   result->u.var_instance.param_qual = param_qual;

   // Allocate space for every scalar in this variable.
   result->u.var_instance.scalar_values = (Dataflow **)malloc_fast(sizeof(Dataflow*) * type->scalar_count);

   // Set every scalar:
   // - for samplers: construct sampler node with index SAMPLER_LOCATION_UNDEFINED;
   // - for non-uniform/varying/attribute bool/int/float: set to one of the uninitialized variables.

   for (i = 0; i < type->scalar_count; i++)
   {
      PrimitiveTypeIndex type_index = glsl_get_scalar_value_type_index(type, i);

      switch (type_index)
      {
         case PRIM_BOOL:
            vcos_assert(g_BoolFalse);
            result->u.var_instance.scalar_values[i] = g_BoolFalse;
            break;

         case PRIM_INT:
            vcos_assert(g_IntZero);
            result->u.var_instance.scalar_values[i] = g_IntZero;
            break;

         case PRIM_FLOAT:
            vcos_assert(g_FloatZero);
            result->u.var_instance.scalar_values[i] = g_FloatZero;
            break;

         case PRIM_SAMPLER2D:
         case PRIM_SAMPLERCUBE:
            // Although the user cannot make TYPE_QUAL_NONE (i.e. non-uniform) sampler var instances,
            // we need this ability for function inlining.
            result->u.var_instance.scalar_values[i] = glsl_dataflow_construct_const_sampler(LINE_NUMBER_UNDEFINED, type_index);
            break;

         default:
            UNREACHABLE();
            return;
      }
   }
}
