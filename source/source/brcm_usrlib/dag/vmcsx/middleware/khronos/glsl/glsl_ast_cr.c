/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
//
// Utilities.
//

/*
   LVALUE_FLAGS_T glsl_get_lvalue_flags_for_type(SymbolType* type)

   Returns whether the specified type:
   IS_LVALUE
      is a true lvalue and can be used as the target of an assignment
   LVF_OUT_OR_INOUT_PARAM
      can be used as an out or inout parameter but not as the target of an assignment
      (i.e.  arrays or strctures containing arrays)
   IS_NOT_LVALUE
      is not an lvalue, i.e. is read-only.

   Note that even if a type supports lvalues, not all expressions of this type
   will be lvalues.

   Khronos documentation:

   Samplers cannot be treated as lvalues
   and cannot be used as out or inout function parameters.


   Variables that are built-in types, entire structures, structure
   fields, l-values with the field selector ( . ) applied to select components or swizzles without repeated
   fields, l-values within parentheses, and l-values dereferenced with the array subscript operator ( [ ] ) are
   all l-values. Other unary, binary and ternary expressions are not l-values. This includes assignments,
   function names, swizzles with repeated fields and constants.

   Array variables are l-values and may be passed to parameters declared as out or inout. However, they
   may not be used as the target of an assignment. Similarly, structures containing arrays may be passed to
   parameters declared as out or inout but may not be used as the target of an assignment.


   Evaluation of an inout parameter results in an lvalue,
   the value of which is copied to the formal parameter at call time.


   Array variables cannot be constant expressions since constants must be initialized at the point of
declaration and there is no mechanism to initialize arrays.

   Implementation notes:

   The "const" qualifier is a property of the instance not the type. So we do not worry
   about it here.

   Since array variables cannot be constant expressions, we use the "can be a constant expression"
   property to tell us whether something can be an l-value (i.e. arrays, structures containing arrays
   (and samplers) can't be).

   Preconditions:

   type points to a valid SymbolType
   type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE,SYMBOL_ARRAY_TYPE} i.e. not a function type

   Postconditions:

   Always returns normally.
   Result is IS_LVALUE, IS_NOT_LVALUE or LVF_OUT_OR_INOUT_PARAM.

   Invariants preserved:

   -

   Invariants used:
   (GL20_COMPILER_SYMBOLTYPE_SIZE_ASSIGNMENT_TARGET)

 */

static LVALUE_FLAGS_T glsl_get_lvalue_flags_for_type(SymbolType* type)
{
	switch (type->flavour)
	{
		case SYMBOL_PRIMITIVE_TYPE:
			// Primitive variables that are not valid as constant types (i.e. the samplers) are not lvalues.
			return type->size_as_const ? IS_LVALUE : IS_NOT_LVALUE;

		case SYMBOL_STRUCT_TYPE:
			// Struct variables that are not valid as constant types are not true lvalues,
			// but they can be passed to out or inout params.
			return type->size_as_const ? IS_LVALUE : LVF_OUT_OR_INOUT_PARAM;

		case SYMBOL_ARRAY_TYPE:
			// Array variables are not true lvalues,
			// but they can be passed to out or inout params.
			return LVF_OUT_OR_INOUT_PARAM;

		default:
			UNREACHABLE();
			return IS_NOT_LVALUE;
	}
}

/*
   LVALUE_FLAGS_T glsl_get_lvalue_flags(Expr* expr)

   Returns whether the specified expression:
   IS_LVALUE
      is a true lvalue and can be used as the target of an assignment
   LVF_OUT_OR_INOUT_PARAM
      can be used as an out or inout parameter but not as the target of an assignment
      (i.e.  arrays or strctures containing arrays)
   IS_NOT_LVALUE
      is not an lvalue, i.e. is read-only.

   Khronos documentation:

   Samplers cannot be treated as lvalues
   and cannot be used as out or inout function parameters.


   Variables that are built-in types, entire structures, structure
   fields, l-values with the field selector ( . ) applied to select components or swizzles without repeated
   fields, l-values within parentheses, and l-values dereferenced with the array subscript operator ( [ ] ) are
   all l-values. Other unary, binary and ternary expressions are not l-values. This includes assignments,
   function names, swizzles with repeated fields and constants.

   Array variables are l-values and may be passed to parameters declared as out or inout. However, they
   may not be used as the target of an assignment. Similarly, structures containing arrays may be passed to
   parameters declared as out or inout but may not be used as the target of an assignment.


   Evaluation of an inout parameter results in an lvalue,
   the value of which is copied to the formal parameter at call time.

   Implementation notes:

   Arrays with non-constant subscripts are not lvalues because only uniform arrays
   can have non-constant lookups. So we don't need an explicit check for this.

   Preconditions:

   expr points to a valid Expr

   Postconditions:

   Always returns normally.
   Result is IS_LVALUE, IS_NOT_LVALUE or LVF_OUT_OR_INOUT_PARAM.

   Invariants preserved:

   -

   Invariants used:

   (GL20_COMPILER_SYMBOL_VAR_OR_PARAM_INSTANCE_TYPE)
   (GL20_COMPILER_SYMBOL_VAR_INSTANCE_TYPE_QUAL)
   (GL20_COMPILER_SHADERFLAVOUR)
   (GL20_COMPILER_EXPR_TYPE)
   (GL20_COMPILER_IS_NOT_LVALUE_ZERO)
 */

LVALUE_FLAGS_T glsl_get_lvalue_flags(Expr* expr)
{
   STACK_CHECK();

	switch (expr->flavour)
	{
		case EXPR_VALUE:
		case EXPR_FUNCTION_CALL:
		case EXPR_PRIM_CONSTRUCTOR_CALL:
		case EXPR_TYPE_CONSTRUCTOR_CALL:
		case EXPR_POST_INC:
		case EXPR_POST_DEC:
		case EXPR_PRE_INC:
		case EXPR_PRE_DEC:
		case EXPR_ARITH_NEGATE:
		case EXPR_LOGICAL_NOT:
		case EXPR_MUL:
		case EXPR_DIV:
		case EXPR_ADD:
		case EXPR_SUB:
		case EXPR_LESS_THAN:
		case EXPR_LESS_THAN_EQUAL:
		case EXPR_GREATER_THAN:
		case EXPR_GREATER_THAN_EQUAL:
		case EXPR_EQUAL:
		case EXPR_NOT_EQUAL:
		case EXPR_LOGICAL_AND:
		case EXPR_LOGICAL_XOR:
		case EXPR_LOGICAL_OR:
		case EXPR_CONDITIONAL:
		case EXPR_ASSIGN:
		case EXPR_SEQUENCE:
      case EXPR_INTRINSIC_TEXTURE_2D_BIAS:
      case EXPR_INTRINSIC_TEXTURE_2D_LOD:
      case EXPR_INTRINSIC_TEXTURE_CUBE_BIAS:
      case EXPR_INTRINSIC_TEXTURE_CUBE_LOD:
      case EXPR_INTRINSIC_RSQRT:
      case EXPR_INTRINSIC_RCP:
      case EXPR_INTRINSIC_LOG2:
      case EXPR_INTRINSIC_EXP2:
      case EXPR_INTRINSIC_CEIL:
      case EXPR_INTRINSIC_FLOOR:
      case EXPR_INTRINSIC_SIGN:
      case EXPR_INTRINSIC_TRUNC:
      case EXPR_INTRINSIC_NEAREST:
      case EXPR_INTRINSIC_MIN:
      case EXPR_INTRINSIC_MAX:
			return IS_NOT_LVALUE;

		case EXPR_INSTANCE:
         {
            TypeQualifier type_qual;

            switch (expr->u.instance.symbol->flavour)
            {
               case SYMBOL_VAR_INSTANCE:
               case SYMBOL_PARAM_INSTANCE:
                  type_qual = expr->u.instance.symbol->u.var_instance.type_qual;
                  break;
               default:
                  UNREACHABLE();
                  return IS_NOT_LVALUE;
            }

			   switch (type_qual)
			   {
				   case TYPE_QUAL_CONST:
				   case TYPE_QUAL_ATTRIBUTE:
				   case TYPE_QUAL_UNIFORM:
					   // Not writeable, so not lvalues.
					   return IS_NOT_LVALUE;

				   case TYPE_QUAL_VARYING:
				   case TYPE_QUAL_INVARIANT_VARYING:
					   // Not writeable in a fragment shader, so not lvalues.
                  if (g_ShaderFlavour == SHADER_FRAGMENT)
                     return IS_NOT_LVALUE;
                  else
   					   return glsl_get_lvalue_flags_for_type(expr->type);

				   case TYPE_QUAL_NONE:
					   // Return flags based on the variable type.
					   return glsl_get_lvalue_flags_for_type(expr->type);

				   default:
					   UNREACHABLE();
					   return IS_NOT_LVALUE;
			   }
         }

		case EXPR_SUBSCRIPT:
			return glsl_get_lvalue_flags(expr->u.subscript.aggregate) ? glsl_get_lvalue_flags_for_type(expr->type) : IS_NOT_LVALUE;

		case EXPR_FIELD_SELECTOR_STRUCT:
			return glsl_get_lvalue_flags(expr->u.field_selector_struct.aggregate) ? glsl_get_lvalue_flags_for_type(expr->type) : IS_NOT_LVALUE;

		case EXPR_FIELD_SELECTOR_SWIZZLE:
			{
				// Must check for repeated swizzle fields.
				int i;
				SWIZZLE_FIELD_FLAGS_T swizzle_fields_in_use = 0;

				for (i = 0; i < MAX_SWIZZLE_FIELD_COUNT; i++)
				{
					if (SWIZZLE_SLOT_UNUSED != expr->u.field_selector_swizzle.swizzle_slots[i])
					{
						SWIZZLE_FIELD_FLAGS_T swizzle_field_flag = 1 << expr->u.field_selector_swizzle.swizzle_slots[i];
						if (swizzle_fields_in_use & swizzle_field_flag)
						{
							return IS_NOT_LVALUE;
						}
						swizzle_fields_in_use |= swizzle_field_flag;
					}
				}

				return glsl_get_lvalue_flags(expr->u.field_selector_swizzle.aggregate) ? glsl_get_lvalue_flags_for_type(expr->type) : IS_NOT_LVALUE;
			}

		default:
			UNREACHABLE();
			return IS_NOT_LVALUE;
	}
}
