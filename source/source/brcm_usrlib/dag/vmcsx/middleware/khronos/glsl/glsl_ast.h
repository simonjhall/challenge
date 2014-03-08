/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_AST_H
#define GLSL_AST_H

#include "middleware/khronos/glsl/glsl_symbols.h"

#include "middleware/khronos/glsl/glsl_const_types.h"


//
// Forward declarations.
//

#define MAX_SWIZZLE_FIELD_COUNT 4
#define SWIZZLE_FIELD_XYZW_X 0
#define SWIZZLE_FIELD_XYZW_Y 1
#define SWIZZLE_FIELD_XYZW_Z 2
#define SWIZZLE_FIELD_XYZW_W 3
#define SWIZZLE_FIELD_RGBA_R 0
#define SWIZZLE_FIELD_RGBA_G 1
#define SWIZZLE_FIELD_RGBA_B 2
#define SWIZZLE_FIELD_RGBA_A 3
#define SWIZZLE_FIELD_STPQ_S 0
#define SWIZZLE_FIELD_STPQ_T 1
#define SWIZZLE_FIELD_STPQ_P 2
#define SWIZZLE_FIELD_STPQ_Q 3
#define SWIZZLE_SLOT_UNUSED -1
typedef char SWIZZLE_FIELD_FLAGS_T; // Bitmask. Must hold MAX_SWIZZLE_FIELD_COUNT flags

#define SF_NEED_RETURN_GUARD_TO_BLOCK_END		(1 << 0)
#define SF_DONE_RETURN_GUARD_TO_BLOCK_END		(1 << 1)
#define SF_IS_RETURN_GUARD						(1 << 2)
#define ST_IS_RETVAL_RETURN						(1 << 3)
typedef char STATEMENT_FLAGS_T;

//
// Expressions.
//

/*
   _ExprChain

   Expression chain structure, for efficient appending.

   Represents a list {a[0], a[1], ..., a[count-1]} of pointers to Expr.
*/

struct _ExprChain
{
   /*
      first

      Invariants:

      (GL20_COMPILER_EXPRCHAIN_FIRST)
      If count == 0
         first == NULL
      Else
         first is associated with this _ExprChain and index 0.
   */
	ExprChainNode* first;
   /*
      last

      Invariants:

      (GL20_COMPILER_EXPRCHAIN_LAST)
      If count == 0
         last == NULL
      Else
         last is associated with this _ExprChain and index count-1.
   */
	ExprChainNode* last;
   /*
      count

      The number of elements in the list

      Invariants:

      count >= 0
   */
	int count;
   /*
      all_compile_time_values

      All expressions have compile time values

      Invariants:

      (GL20_COMPILER_EXPRCHAIN_ALL_COMPILE_TIME_VALUES)
      If all_compile_time_values is true then a[i]->compile_time_value != NULL for all 0 <= i < count
   */
	bool all_compile_time_values;
   /*
      all_primitive_types

      All expressions have primtive types

      Invariants:

      (GL20_COMPILER_EXPRCHAIN_ALL_PRIMITIVE_TYPES)
      If all_primitive_types is true then a[i]->type->flavour == SYMBOL_PRIMITIVE_TYPE
   */
	bool all_primitive_types; // iff all expressions have primitive types

   bool all_constant_index_expression;
};

/*
   _ExprChainNode

   Associated with an _ExprChain and an index i.
*/

struct _ExprChainNode
{
   /*
      expr

      The Expr object at this list position, a[i]

      Invariants:

      (GL20_COMPILER_EXPRCHAINNODE_EXPR)
      expr is non-NULL.
   */
	Expr* expr;
   /*
      next

      The next element in the list

      Invariants:

      (GL20_COMPILER_EXPRCHAINNODE_NEXT)
      If i == count-1 then
         next == NULL
      Else
         next is associated with the same _ExprChain and index i+1.
   */
	ExprChainNode* next;
};

ExprChain* glsl_expr_chain_create(void);
ExprChain* glsl_expr_chain_append(ExprChain* chain, Expr* expr);
ExprChain* glsl_expr_chain_append_node(ExprChain* chain, ExprChainNode* node);

typedef enum
{
   PRIM_CONS_INVALID,
   PRIM_CONS_SCALAR_FROM_FIRST_COMPONENT,
   PRIM_CONS_VECTOR_FROM_SCALAR,
   PRIM_CONS_MATRIX_FROM_SCALAR,
   PRIM_CONS_MATRIX_FROM_MATRIX,
   PRIM_CONS_VECTOR_OR_MATRIX_FROM_COMPONENT_FLOW
} PrimConsFlavour;

struct _Expr
{
   /*
      flavour

      Explains how this expression was constructed syntactically.

      Khronos documentation:

      Expressions in the shading language are built from the following:
      - Constants of type bool, int, float, all vector types, and all matrix types.
      - Constructors of all types.
      - Variable names of all types, except array names not followed by a subscript.
      - Subscripted array names.
      - Unsubscripted array names [...]
      - Function calls that return values.
      - Component field selectors and array subscript results.
      [- Parenthesized expression [...]]
      - The arithmetic binary operators add (+), subtract (-), multiply (*), and divide (/) [...]
      [- The operator remainder (%) is reserved for future use.]
      - The arithmetic unary operators negate (-), post- and pre-increment and decrement (-- and ++) [...]
      - The relational operators greater than (>), less than (<), greater than or equal (>=), and less than or
      equal (<=) [...]
      - The equality operators equal (==), and not equal (!=) [...]
      - The logical binary operators and (&&), or ( | | ), and exclusive or (^^) [...]
      - The logical unary operator not (!). [...]
      - The sequence ( , ) operator [...]
      - The ternary selection operator (?:). [...]
      [- Operators and (&), or ( | ), exclusive or (^), not (~), right-shift (>>), left-shift (<<). These
      operators are reserved for future use.]

      Implementation notes:

      Parenthesized expressions do not appear as all parentheses are absorbed
      when the AST is generated.
      Reserved operators (% & | ^ ~ >> <<) do not appear here either.

      We extend the language syntax to add the following intrinsics:

      $$texture_2d_bias, $$texture_cube_bias
      $$rsqrt, $$rcp, $$log2, $$exp2
      $$ceil, $$floor, $$sign, $$nearest, $$min, $$max

      TODO I'm not convinced about these ones:
      $$texture2DLod, $$texture_cube_lod, $$trunc

      Invariants:

      (GL20_COMPILER_EXPR_FLAVOUR)
      flavour is a valid ExprFlavour enum constant (not including EXPR_FLAVOUR_COUNT)

      Extra documentation (I don't know exactly where to put this):

EXPR_VALUE,
   TODO: is this right?
   Khronos documentation:

   - Constants of type bool, int, float, all vector types, and all matrix types.

   These are just literals, not other constant expressions.

   Invariants:

   type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE}
   compile_time_value != NULL

EXPR_INSTANCE,
   Khronos documentation:

   - Variable names of all types, except array names not followed by a subscript.
   - Unsubscripted array names. Unsubscripted arrays may only be used as actual parameters in
function calls or within parentheses. The only operator defined for arrays is the subscript operator
( [ ] ).

EXPR_SUBSCRIPT,
   TODO: "and array subscript results"?
   Khronos documentation:

   - Subscripted array names.
   - [Component field selectors and] array subscript results.

   5.2 Array Subscripting
   Array elements are accessed using the array subscript operator ( [ ] ). This is the only operator that
   operates on arrays. An example of accessing an array element is
   diffuseColor += lightIntensity[3] * NdotL;

EXPR_FUNCTION_CALL,
   Khronos documentation:

   - Function calls that return values.

   5.3 Function Calls
   If a function returns a value, then a call to that function may be used as an expression, whose type will be
   the type that was used to declare or define the function.
   Function definitions and calling conventions are discussed in Section 6.1 “Function Definitions” .

EXPR_PRIM_CONSTRUCTOR_CALL,
EXPR_TYPE_CONSTRUCTOR_CALL,
   Khronos documentation:

   - Constructors of all types.

   Implementation notes:

   - We separate out primitive types from other ones

   TODO insert bits of 5.4 here?


EXPR_FIELD_SELECTOR_STRUCT,
EXPR_FIELD_SELECTOR_SWIZZLE,
   TODO: "and array subscript results"?
   Khronos documentation:

   - Component field selectors [and array subscript results.]

   Implementation notes:

   - We separate out struct member selection from vector member selection (swizzle)

   TODO insert bits of 5.5, 5.6 and 5.7 here?



EXPR_POST_INC,
EXPR_POST_DEC,
EXPR_PRE_INC,
EXPR_PRE_DEC,
EXPR_ARITH_NEGATE,
   Khronos documentation:

   - The arithmetic unary operators negate (-), post- and pre-increment and decrement (-- and ++)
operate on integer or floating-point values (including vectors and matrices). These result with the
same type they operated on. For post- and pre-increment and decrement, the expression must be
one that could be assigned to (an l-value). Pre-increment and pre-decrement add or subtract 1 or
1.0 to the contents of the expression they operate on, and the value of the pre-increment or predecrement
expression is the resulting value of that modification. Post-increment and postdecrement
expressions add or subtract 1 or 1.0 to the contents of the expression they operate on,
but the resulting expression has the expression’s value before the post-increment or postdecrement
was executed.

EXPR_LOGICAL_NOT,
   Khronos documentation:

   - The logical unary operator not (!). It operates only on a Boolean expression and results in a
Boolean expression. To operate on a vector, use the built-in function not.

EXPR_MUL,
EXPR_DIV,
EXPR_ADD,
EXPR_SUB,
   Khronos documentation:

   - The arithmetic binary operators add (+), subtract (-), multiply (*), and divide (/) operate on
   integer and floating-point typed expressions (including vectors and matrices). The two operands
   must be the same type, or one can be a scalar float and the other a float vector or matrix, or one
   can be a scalar integer and the other an integer vector. Additionally, for multiply (*), one can be a
   vector and the other a matrix with the same dimensional size of the vector. These result in the
   same fundamental type (integer or float) as the expressions they operate on. If one operand is
   scalar and the other is a vector or matrix, the scalar is applied component-wise to the vector or
   matrix, resulting in the same type as the vector or matrix. Dividing by zero does not cause an
   exception but does result in an unspecified value. Multiply (*) applied to two vectors yields a
   component-wise multiply. Multiply (*) applied to two matrices yields a linear algebraic matrix
   multiply, not a component-wise multiply. Multiply of a matrix and a vector yields a linear
   algebraic transform. [...]

   TODO: insert some of 5.11 here? It seems to be saying the exact same thing as the above anyway.

EXPR_LESS_THAN,
EXPR_LESS_THAN_EQUAL,
EXPR_GREATER_THAN,
EXPR_GREATER_THAN_EQUAL,
   Khronos documentation:

   - The relational operators greater than (>), less than (<), greater than or equal (>=), and less than or
equal (<=) operate only on scalar integer and scalar floating-point expressions. The result is
scalar Boolean. The operands’ types must match. [...]

EXPR_EQUAL,
EXPR_NOT_EQUAL,
   Khronos documentation:

   - The equality operators equal (==), and not equal (!=) operate on all types except arrays,
structures containing arrays, sampler types and structures containing sampler types They result in
a scalar Boolean. For vectors, matrices, and structures, all components of the operands must be
equal for the operands to be considered equal.

EXPR_LOGICAL_AND,
EXPR_LOGICAL_XOR,
EXPR_LOGICAL_OR,

   Khronos documentation:

   - The logical binary operators and (&&), or ( | | ), and exclusive or (^^) operate only on two
Boolean expressions and result in a Boolean expression. And (&&) will only evaluate the right
hand operand if the left hand operand evaluated to true. [...]

EXPR_CONDITIONAL,
   Khronos documentation:

   - The ternary selection operator (?:). It operates on three expressions (exp1 ? exp2 : exp3). This
operator evaluates the first expression, which must result in a scalar Boolean. If the result is true,
it selects to evaluate the second expression, otherwise it selects to evaluate the third expression.
Only one of the second and third expressions is evaluated. The second and third expressions must
be the same type, but can be of any type other than an array. The resulting type is the same as the
type of the second and third expressions.

EXPR_ASSIGN,
   Khronos documentation:

   TODO spec bug? Missing from 5.9

   5.8 Assignments
   Assignments of values to variable names are performed using the assignment operator ( = ):
   lvalue-expression = rvalue-expression
   The lvalue-expression evaluates to an l-value. The assignment operator stores the value of the rvalueexpression
   into the l-value and returns an r-value with the type and precision of the lvalue-expression.
   The lvalue-expression and rvalue-expression must have the same type. All desired type-conversions must
   be specified explicitly via a constructor. Variables that are built-in types, entire structures, structure
   fields, l-values with the field selector ( . ) applied to select components or swizzles without repeated
   fields, l-values within parentheses, and l-values dereferenced with the array subscript operator ( [ ] ) are
   all l-values. Other unary, binary and ternary expressions are not l-values. This includes assignments,
   function names, swizzles with repeated fields and constants.
   Array variables are l-values and may be passed to parameters declared as out or inout. However, they
   may not be used as the target of an assignment. Similarly, structures containing arrays may be passed to
   parameters declared as out or inout but may not be used as the target of an assignment.
   Expressions on the left of an assignment are evaluated before expressions on the right of the assignment.
   [...]

   Implementation notes:
   removed - lvalues are side-effect free so we construct x *= y as x = x * y
      EXPR_MUL_ASSIGN,
      EXPR_DIV_ASSIGN,
      EXPR_ADD_ASSIGN,
      EXPR_SUB_ASSIGN,


EXPR_SEQUENCE,
   Khronos documentation:

   - The sequence ( , ) operator that operates on expressions by returning the type and value of the
right-most expression in a comma separated list of expressions. All expressions are evaluated, in
order, from left to right.

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
*/
	ExprFlavour flavour;

   /*
      line_num

      The source line number on which this expression appeared.

      Invariants:

      -
   */
	int line_num;

	/*
      type

      The expression's type.

      Invariants:

      (GL20_COMPILER_EXPR_TYPE)
      type is valid for the lifetime of fastmem
      type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE,SYMBOL_ARRAY_TYPE}
      See flavour for additional flavour-specific restrictions
   */
	SymbolType* type;

   /*
      compile_time_value

	   A pointer to the value of this expression, or NULL if the value is not known at compile time.
	   The value is type->size_as_const bytes.
	   NOTE: For params this will be NULL even if they have the const qualifier,
	   because const params are readonly, but not compile time constants.

      Invariants:

      (GL20_COMPILER_EXPR_COMPILE_TIME_VALUE)
      If compile_time_value != NULL then:
         compile_time_value is valid for the lifetime of fastmem
         compile_time_value is a compile time instance of type

      See flavour for additional flavour-specific restrictions (i.e. when this must be non-NULL)
   */
	void* compile_time_value;

   bool constant_index_expression;

   /*
      union u

      Validity depends on flavour.
      Each branch of this union is annotated with what flavour must be.
   */
	union
	{
		// EXPR_VALUE: no metadata

		// EXPR_INSTANCE
		struct
		{
         /*
            symbol

            The variable symbol this is an instance of.

            Invariants:

            (GL20_COMPILER_EXPR_INSTANCE_SYMBOL_FLAVOUR)
            symbol is valid for the lifetime of fastmem
            symbol->flavour in {SYMBOL_VAR_INSTANCE,SYMBOL_PARAM_INSTANCE}

            (GL20_COMPILER_EXPR_INSTANCE_SYMBOL_TYPE)
            symbol->type is identical to type (i.e. glsl_shallow_match_nonfunction_types would return true)
         */
			Symbol* symbol;
		} instance;

		// EXPR_SUBSCRIPT
      // (GL20_COMPILER_EXPR_SUBSCRIPT)
		struct
		{
         /*
            aggregate

            The expression `a` in `a[i]`

            Invariants:

            aggregate is valid for the lifetime of fastmem
            TODO: some invariant about aggregate being a valid aggregate?
         */
			Expr* aggregate;
         /*
            subscript

            The expression `i` in `a[i]`

            Invariants:

            subscript is valid for the lifetime of fastmem
            subscript->type == &primitiveTypes[PRIM_INT]
            If subscript has a compile time value, this value does not exceed the range
            of the array/vector/matrix (TODO: formalise this)
         */
			Expr* subscript;
		} subscript;

		// EXPR_FUNCTION_CALL
      // (GL20_COMPILER_EXPR_FUNCTION_CALL)
		struct
		{
         /*
            function

            The symbol `f` in `f(x,y)`

            Invariants:

            function is valid for the lifetime of fastmem
            function->flavour == GL20_COMPILER_SYMBOL_FUNCTION_INSTANCE_TYPE
         */
			Symbol* function;
         /*
            function

            The list `x,y` in `f(x,y)`

            Invariants:

            args is valid for the lifetime of fastmem
         */
			ExprChain* args; // in order, i.e. leftmost first
		} function_call;

		// EXPR_PRIM_CONSTRUCTOR_CALL
      // (GL20_PRIM_CONSTRUCTOR_CALL)
		struct
		{
         /*
            flavour

            Implementation notes:

            We already have the type above.
         */
         PrimConsFlavour flavour;
         /*
            args

            Implementation notes:

            In order, i.e. leftmost first

            Invariants:

            args is valid for the lifetime of fastmem
         */
			ExprChain* args;
		} prim_constructor_call;

		// EXPR_TYPE_CONSTRUCTOR_CALL
		struct
		{
         /*
            args

            Implementation notes

            We already have the type above.
            In order, i.e. leftmost first
         */
			ExprChain* args;
		} type_constructor_call;

		// EXPR_FIELD_SELECTOR_STRUCT
		struct
		{
         /*
            aggregate

            The expression `a` in `a.f`

            Invariants:

            aggregate is valid for the lifetime of fastmem
         */
			Expr* aggregate;
			const char* field;
			int field_no;
		} field_selector_struct;

		// EXPR_FIELD_SELECTOR_SWIZZLE
		struct
		{
         /*
            aggregate

            The expression `a` in `a.xxz`

            Invariants:

            aggregate is valid for the lifetime of fastmem
         */
			Expr* aggregate;
			const char* field;
         /*
            swizzle_slots

            Each slot contains the field number (0 <= n < MAX_SWIZZLE_FIELD_COUNT) that it is selecting,
            or SWIZZLE_SLOT_UNUSED.
            e.g. .xxz would be represented as { 0, 0, 2, SWIZZLE_SLOT_UNUSED }.

            Invariants:

            For all 0 <= i < MAX_SWIZZLE_FIELD_COUNT:
               swizzle_slots[i] == SWIZZLE_SLOT_UNUSED or 0 <= swizzle_slots[i] < MAX_SWIZZLE_FIELD_COUNT

            For all 0 <= i < j < MAX_SWIZZLE_FIELD_COUNT:
               If swizzle_slots[i] == SWIZZLE_SLOT_UNUSED then swizzle_slots[j] == SWIZZLE_SLOT_UNUSED

            swizzle_slots[0] != SWIZZLE_SLOT_UNUSED
         */

			signed char swizzle_slots[MAX_SWIZZLE_FIELD_COUNT];
		} field_selector_swizzle;

		// EXPR_POST_INC, EXPR_POST_DEC, EXPR_PRE_INC, EXPR_PRE_DEC, EXPR_ARITH_NEGATE, EXPR_LOGICAL_NOT
		struct
		{
			Expr* operand;
		} unary_op;

		// EXPR_MUL, EXPR_DIV, EXPR_ADD, EXPR_SUB
		// EXPR_LESS_THAN, EXPR_LESS_THAN_EQUAL, EXPR_GREATER_THAN, EXPR_GREATER_THAN_EQUAL
		// EXPR_EQUAL, EXPR_NOT_EQUAL
		// EXPR_LOGICAL_AND, EXPR_LOGICAL_XOR, EXPR_LOGICAL_OR
		struct
		{
			Expr* left;
			Expr* right;
		} binary_op;

		// EXPR_CONDITIONAL
		struct
		{
			Expr* cond;
			Expr* if_true;
			Expr* if_false;
		} cond_op;

		// EXPR_ASSIGN, EXPR_MUL_ASSIGN, EXPR_DIV_ASSIGN, EXPR_ADD_ASSIGN, EXPR_SUB_ASSIGN
		struct
		{
			Expr* lvalue;
			Expr* rvalue;
		} assign_op;

		// EXPR_SEQUENCE
		struct
		{
			Expr* all_these;
			Expr* then_this;
		} sequence;

		// EXPR_INTRINSIC_TEXTURE_2D_BIAS, EXPR_INTRINSIC_TEXTURE_2D_LOD
		// EXPR_INTRINSIC_TEXTURE_CUBE_BIAS, EXPR_INTRINSIC_TEXTURE_CUBE_LOD
		// EXPR_INTRINSIC_RSQRT, EXPR_INTRINSIC_RCP, EXPR_INTRINSIC_LOG2, EXPR_INTRINSIC_EXP2
      // EXPR_INTRINSIC_CEIL, EXPR_INTRINSIC_FLOOR, EXPR_INTRINSIC_SIGN
      // EXPR_INTRINSIC_TRUNC, EXPR_INTRINSIC_NEAREST
		// EXPR_INTRINSIC_MIN, EXPR_INTRINSIC_MAX
		struct
		{
			// Note we already have the type above.
			ExprChain* args; // in order, i.e. leftmost first
		} intrinsic;
	} u;
};

// Checks whether this expression is an lvalue.
typedef char LVALUE_FLAGS_T;
#define IS_NOT_LVALUE				0   /* (GL20_COMPILER_IS_NOT_LVALUE_ZERO) We rely on this being zero for the ?: operator to work */
#define LVF_OUT_OR_INOUT_PARAM		(1 << 0)
#define LVF_ASSIGNMENT_TARGET		(1 << 1)
#define IS_LVALUE					(LVF_OUT_OR_INOUT_PARAM | LVF_ASSIGNMENT_TARGET)

LVALUE_FLAGS_T glsl_get_lvalue_flags(Expr* expr);

// On failure, these functions call glsl_compile_error() and return NULL.
Expr* glsl_expr_construct_value_int(const_int v);
Expr* glsl_expr_construct_value_float(const_float v);
Expr* glsl_expr_construct_value_bool(const_bool v);
Expr* glsl_expr_construct_instance(Symbol* symbol);
Expr* glsl_expr_construct_subscript(Expr* aggregate, Expr* subscript);
Expr* glsl_expr_construct_function_call(Symbol* overload_chain, ExprChain* args);
Expr* glsl_expr_construct_prim_constructor_call(PrimitiveTypeIndex return_index, ExprChain* args);
Expr* glsl_expr_construct_type_constructor_call(SymbolType* type, ExprChain* args);
Expr* glsl_expr_construct_field_selector(Expr* aggregate, const char* field);
Expr* glsl_expr_construct_unary_op_arithmetic(ExprFlavour flavour, Expr* operand);
Expr* glsl_expr_construct_unary_op_logical(ExprFlavour flavour, Expr* operand);
Expr* glsl_expr_construct_binary_op_arithmetic(ExprFlavour flavour, Expr* left, Expr* right);
Expr* glsl_expr_construct_binary_op_logical(ExprFlavour flavour, Expr* left, Expr* right);
Expr* glsl_expr_construct_binary_op_relational(ExprFlavour flavour, Expr* left, Expr* right);
Expr* glsl_expr_construct_binary_op_equality(ExprFlavour flavour, Expr* left, Expr* right);
Expr* glsl_expr_construct_cond_op(Expr* cond, Expr* if_true, Expr* if_false);
Expr* glsl_expr_construct_assign_op(Expr* lvalue, Expr* rvalue);
Expr* glsl_expr_construct_sequence(Expr* all_these, Expr* then_this);
Expr* glsl_expr_construct_intrinsic(ExprFlavour flavour, ExprChain* args);


//
// Statements.
//

// Statement chain structure, for efficient appending.
struct _StatementChain
{
   /*
      first

      The first node of this linked list or null if the list is empty.

      Invariants:

      If count == 0 then first == NULL.
      Else
         first is valid for the lifetime of fastmem
         first->prev == NULL
   */
	StatementChainNode* first;
   /*
      last

      The last node of this linked list or null if the list is empty.

      Invariants:

      If count == 0 then last == NULL.
      Else
         last is valid for the lifetime of fastmem
         last->next == NULL
   */
	StatementChainNode* last;
   /*
      count

      The number of items in this list.

      Invariants:

      count >= 0
      If count > 0 then starting from first and following next (count-1) times
      yields last.
   */
	int count;
};

struct _StatementChainNode
{
   /*
      statement

      Invariants:

      statement is valid for the lifetime of fastmem
   */
	Statement* statement;
   /*
      prev

      Invariants:

      If prev != NULL then
         prev is valid for the lifetime of fastmem
         prev->next == this
   */
	StatementChainNode* prev;
   /*
      next

      Invariants:

      If next != NULL then
         next is valid for the lifetime of fastmem
         next->prev == this
   */
	StatementChainNode* next;

   /* TODO: what is this for? */
	bool unlinked;
};

StatementChain* glsl_statement_chain_create(void);
StatementChain* glsl_statement_chain_append(StatementChain* chain, Statement* statement);

struct _Statement
{
	/*
      flavour
      The flavour of statement.

      Khronos documentation:

      The fundamental building blocks of the OpenGL ES Shading Language are:
      - statements and declarations
      - function definitions
      - selection (if-else)
      - iteration (for, while, and do-while)
      - jumps (discard, return, break, and continue)

      Invariants:

      (GL20_COMPILER_EXPR_FLAVOUR)
      flavour is a valid StatementFlavour enum constant (not including STATEMENT_FLAVOUR_COUNT)

      Khronos documentation:

      TODO
      -

      Extra documentation (I don't know exactly where to put this):
STATEMENT_AST,
   Khronos documentation:
   The overall structure of a compilation unit is as follows
      translation-unit:
      global-declaration
      translation-unit global-declaration
      global-declaration:
      function-definition
      declaration
   That is, a compilation unit is a sequence of declarations and function bodies.
STATEMENT_DECL_LIST,
   TODO I don't quite understand this one

STATEMENT_FUNCTION_DEF,
   Khronos documentation:
   Function bodies are defined as
      function-definition:
      function-prototype { statement-list }
      statement-list:
      statement
      statement-list statement
      statement:
      compound-statement
      simple-statement
   TODO: some stuff from 6.1?

STATEMENT_VAR_DECL,
   Khronos documentation:

   simple-statement:
      declaration-statement
      ...

   All variables and functions must be declared before being used. Variable and function names are
   identifiers.
   There are no default types. All variable and function declarations must have a declared type, and
   optionally qualifiers. A variable is declared by specifying its type followed by one or more names
   separated by commas. In many cases, a variable can be initialized as part of its declaration by using the
   assignment operator (=).

STATEMENT_COMPOUND,
   Khronos documentation:

   statement:
   compound-statement
   simple-statement
   Curly braces are used to group sequences of statements into compound statements.
   compound-statement:
   { statement-list }

STATEMENT_EXPR,
   Khronos documentation:

   simple-statement:
   expression-statement
   ...

STATEMENT_SELECTION,
   Khronos documentation:

   simple-statement:
   selection-statement
   ...

STATEMENT_ITERATOR_FOR,
STATEMENT_ITERATOR_WHILE,
STATEMENT_ITERATOR_DO_WHILE,
   Khronos documentation:

   simple-statement:
   iteration-statement
   ...

   For, while, and do loops are allowed as follows:
   for (for-init-statement; condition(opt); expression)
      statement-no-new-scope

   while (condition)
      statement-no-new-scope

   do
      statement
   while (expression)

STATEMENT_CONTINUE,
STATEMENT_BREAK,
STATEMENT_DISCARD,
STATEMENT_RETURN,
STATEMENT_RETURN_EXPR,
   Khronos documentation:

   simple-statement:
   jump-statement
   ...

   There is no “goto” nor other non-structured flow of control.
   The continue jump is used only in loops. It skips the remainder of the body of the inner most loop of
   which it is inside. For while and do-while loops, this jump is to the next evaluation of the loop
   condition-expression from which the loop continues as previously defined. For for loops, the jump is to
   the loop-expression, followed by the condition-expression.
   The break jump can also be used only in loops. It is simply an immediate exit of the inner-most loop
   containing the break. No further execution of condition-expression or loop-expression is done.
   The discard keyword is only allowed within fragment shaders. It can be used within a fragment shader to
   abandon the operation on the current fragment. This keyword causes the fragment to be discarded and no
   updates to any buffers will occur. It would typically be used within a conditional statement, for example:
   if (intensity < 0.0)
   discard;
   A fragment shader may test a fragment’s alpha value and discard the fragment based on that test.
   However, it should be noted that coverage testing occurs after the fragment shader runs, and the coverage
   test can change the alpha value.
   The return jump causes immediate exit of the current function. If it has expression then that is the return
   value for the function.

STATEMENT_NULL,
   TODO any khronos documentation?

   */
   StatementFlavour flavour;

	/*
      line_num

      The source line number on which this statement appeared.

      Invariants:

      -
   */
	int line_num;

	// Flags.
	STATEMENT_FLAGS_T flags;

   /*
      union u

      Validity depends on flavour.
      Each branch of this union is annotated with what flavour must be.
   */
	union
	{
		// STATEMENT_AST
		// - The root of the AST, containing STATEMENT_FUNCTION_DEF and STATEMENT_DECL_LIST.
		// - However, after loop transforms, when STATEMENT_DECL_LIST are no longer necessary,
		//   compiler-generated variables will go straight into STATEMENT_VAR_DECL instead.
		struct
		{
			StatementChain* decls;
		} ast;

		// STATEMENT_DECL_LIST
		// - Contains a list of STATEMENT_VAR_DECL of the same type.
		// - Needed in loop statements where more than one STATEMENT_VAR_DECL is possible.
		struct
		{
			StatementChain* decls; // possibly containing 0 declarations
		} decl_list;

		// STATEMENT_FUNCTION_DEF
		struct
		{
			Symbol* header;
         /*
            body

            Invariants:

            body is valid for the lifetime of fastmem
            body->flavour == STATEMENT_COMPOUND
         */
			Statement* body; // as STATEMENT_COMPOUND

         bool active;
		} function_def;

		// STATEMENT_VAR_DECL
		// - When contructed during parsing, these will always be inside STATEMENT_DECL_LIST.
		// - However, after loop transforms, when STATEMENT_DECL_LIST are no longer necessary,
		//   a later transformation will bring them out to statement context level.
		//   (i.e. splicing the list of STATEMENT_VAR_DECL into the place of the STATEMENT_DECL_LIST)
		struct
		{
			// Note we already have the type above.
			Symbol* var;
			Expr* initializer; // or NULL if no initializer
		} var_decl;

		// STATEMENT_COMPOUND
		struct
		{
         /*
            statements

            Invariants:

            statements is valid for the lifetime of fastmem
         */
			StatementChain* statements; // possibly containing 0 statements
		} compound;

		// STATEMENT_EXPR
		struct
		{
			Expr* expr;
		} expr;

		// STATEMENT_SELECTION
		struct
		{
			Expr* cond;
         /*
            if_true

            The body of the `then` branch of the if statement.

            Invariants:

            if_true is valid for the lifetime of fastmem
            if_true->flavour == STATEMENT_COMPOUND
         */
			Statement* if_true; // *always* constructed as a STATEMENT_COMPOUND
         /*
            if_false

            The body of the `else` branch of the if statement or NULL if there
            is no else branch.

            Invariants:

            If if_false != NULL then
               if_false is valid for the lifetime of fastmem
               if_false->flavour == STATEMENT_COMPOUND
         */
			Statement* if_false; // *always* constructed as a STATEMENT_COMPOUND, or NULL if none
		} selection;

		// STATEMENT_ITERATOR_FOR
		struct
		{
			Statement* init;
			Expr* test;
			Expr* loop;
         /*
            block

            Invariants:

            block is valid for the lifetime of fastmem
            block->flavour == STATEMENT_COMPOUND
         */
			Statement* block; // *always* constructed as a STATEMENT_COMPOUND

         int iterations;
		} iterator_for;

		// STATEMENT_ITERATOR_WHILE
		struct
		{
			Statement* cond_or_decl;
         /*
            block

            Invariants:

            block is valid for the lifetime of fastmem
            block->flavour == STATEMENT_COMPOUND
         */
			Statement* block; // *always* constructed as a STATEMENT_COMPOUND
		} iterator_while;

		// STATEMENT_ITERATOR_DO_WHILE
		struct
		{
         /*
            block

            Invariants:

            block is valid for the lifetime of fastmem
            block->flavour == STATEMENT_COMPOUND
         */
			Statement* block; // *always* constructed as a STATEMENT_COMPOUND
			Expr* cond;
		} iterator_do_while;

		// STATEMENT_CONTINUE, STATEMENT_BREAK, STATEMENT_DISCARD, STATEMENT_RETURN: no metadata

		// STATEMENT_RETURN_EXPR
		struct
		{
         /*
            expr

            Invariants:

            expr is valid for the lifetime of fastmem
            expr->type->flavour in {SYMBOL_PRIMITIVE_TYPE,SYMBOL_STRUCT_TYPE}
         */
			Expr* expr;
		} return_expr;
	} u;
};


// On failure, these functions call glsl_compile_error() and return NULL.
Statement* glsl_statement_construct(StatementFlavour flavour);
Statement* glsl_statement_construct_ast(StatementChain* decls);
Statement* glsl_statement_construct_decl_list(StatementChain* decls);
Statement* glsl_statement_construct_function_def(Symbol* header, Statement* body);
Statement* glsl_statement_construct_var_decl(Symbol* var, Expr* initializer);
Statement* glsl_statement_construct_compound(StatementChain* statements);
Statement* glsl_statement_construct_expr(Expr* expr);
Statement* glsl_statement_construct_selection(Expr* cond, Statement* if_true, Statement* if_false);
Statement* glsl_statement_construct_iterator_for(Statement* init, Expr* test, Expr* loop, Statement* block);
Statement* glsl_statement_construct_iterator_while(Statement* cond_or_decl, Statement* block);
Statement* glsl_statement_construct_iterator_do_while(Statement* block, Expr* cond);
Statement* glsl_statement_construct_return_expr(Expr* expr);


#endif // AST_H
