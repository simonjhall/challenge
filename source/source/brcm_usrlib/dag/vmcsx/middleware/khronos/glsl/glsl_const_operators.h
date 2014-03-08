/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_CONST_OPERATORS_H
#define GLSL_CONST_OPERATORS_H

#include "middleware/khronos/glsl/glsl_const_types.h"

#include "middleware/khronos/glsl/2708/glsl_fpu_4.h"

//
// Operators on base types.
//

// EXPR_ARITH_NEGATE
static INLINE void op_arith_negate__const_int__const_int(const_int* result, const_int* operand)
{
	*result = -*operand;
}
static INLINE void op_arith_negate__const_float__const_float(const_float* result, const_float* operand)
{
	*result = *operand ^ (1 << (sizeof(const_float) * 8 - 1));
}

// EXPR_LOGICAL_NOT
static INLINE void op_logical_not__const_bool__const_bool(const_bool* result, const_bool* operand)
{
	*result = !*operand;
}

// EXPR_MUL
static INLINE void op_mul__const_int__const_int__const_int(const_int* result, const_int* left, const_int* right)
{
	*result = (*left) * (*right);
}
static INLINE void op_mul__const_float__const_float__const_float(const_float* result, const_float* left, const_float* right)
{
	glsl_fpu_mul((unsigned *)result, *left, *right);
}

// EXPR_DIV
static INLINE void op_div__const_int__const_int__const_int(const_int* result, const_int* left, const_int* right)
{
	*result = (*left) / (*right);
}
static INLINE void op_div__const_float__const_float__const_float(const_float* result, const_float* left, const_float* right)
{
   unsigned CONST_2_0 = 0x40000000;
	unsigned temp, nr_prod,nr_factor,tempres;
	glsl_fpu_recip(&temp, *right);
	glsl_fpu_mul(&nr_prod, temp, *right);
	glsl_fpu_mul(&tempres, *left,temp);
	glsl_fpu_sub(&nr_factor, CONST_2_0, nr_prod);
	glsl_fpu_mul((unsigned *)result, tempres,nr_factor);
}

// EXPR_ADD
static INLINE void op_add__const_int__const_int__const_int(const_int* result, const_int* left, const_int* right)
{
	*result = (*left) + (*right);
}
static INLINE void op_add__const_float__const_float__const_float(const_float* result, const_float* left, const_float* right)
{
	glsl_fpu_add((unsigned *)result, *left, *right);
}

// EXPR_SUB
static INLINE void op_sub__const_int__const_int__const_int(const_int* result, const_int* left, const_int* right)
{
	*result = (*left) - (*right);
}
static INLINE void op_sub__const_float__const_float__const_float(const_float* result, const_float* left, const_float* right)
{
	glsl_fpu_sub((unsigned *)result, *left, *right);
}

static INLINE void op_cmp__const_int__const_float__const_float(const_int* result, const_float* left, const_float* right)
{
   int a = *left;
   int b = *right;

   if (   (a & ~(1 << 31)) > (0xFF << 23)
            || (b & ~(1 << 31)) > (0xFF << 23)) {
      /* NaNs.  */
      *result = (0xFF << 23) | 1; /* Force N and Z to 0.  */
      return;
   }

   /* Flush denormals to 0.0 on input.  */
   if (((a >> 23) & 0xFF) == 0) {
      a = 0;
   }
   if (((b >> 23) & 0xFF) == 0) {
      b = 0;
   }

   /* Common case.  */
   if (a & (1 << 31))
      a = (a ^ ~(1 << 31)) + 1;
   if (b & (1 << 31))
      b = (b ^ ~(1 << 31)) + 1;

   if (a < b)
      *result = -1;
   else if (a > b)
      *result = 1;
   else
      *result = 0;
}

// EXPR_LESS_THAN
static INLINE void op_less_than__const_bool__const_int__const_int(const_bool* result, const_int* left, const_int* right)
{
	*result = (*left) < (*right);
}
static INLINE void op_less_than__const_bool__const_float__const_float(const_bool* result, const_float* left, const_float* right)
{
   const_int cmpres;

   op_cmp__const_int__const_float__const_float(&cmpres, left, right);

	*result = cmpres < 0;
}

// EXPR_LESS_THAN_EQUAL
static INLINE void op_less_than_equal__const_bool__const_int__const_int(const_bool* result, const_int* left, const_int* right)
{
	*result = (*left) <= (*right);
}
static INLINE void op_less_than_equal__const_bool__const_float__const_float(const_bool* result, const_float* left, const_float* right)
{
   const_int cmpres;

   op_cmp__const_int__const_float__const_float(&cmpres, left, right);

	*result = cmpres <= 0;
}

// EXPR_GREATER_THAN
static INLINE void op_greater_than__const_bool__const_int__const_int(const_bool* result, const_int* left, const_int* right)
{
	*result = (*left) > (*right);
}
static INLINE void op_greater_than__const_bool__const_float__const_float(const_bool* result, const_float* left, const_float* right)
{
   const_int cmpres;

   op_cmp__const_int__const_float__const_float(&cmpres, left, right);

	*result = cmpres > 0;
}

// EXPR_GREATER_THAN_EQUAL
static INLINE void op_greater_than_equal__const_bool__const_int__const_int(const_bool* result, const_int* left, const_int* right)
{
	*result = (*left) >= (*right);
}
static INLINE void op_greater_than_equal__const_bool__const_float__const_float(const_bool* result, const_float* left, const_float* right)
{
   const_int cmpres;

   op_cmp__const_int__const_float__const_float(&cmpres, left, right);

	*result = cmpres >= 0;
}

// EXPR_LOGICAL_AND
static INLINE void op_logical_and__const_bool__const_bool__const_bool(const_bool* result, const_bool* left, const_bool* right)
{
	*result = (*left) && (*right);
}

// EXPR_LOGICAL_XOR
static INLINE void op_logical_xor__const_bool__const_bool__const_bool(const_bool* result, const_bool* left, const_bool* right)
{
	*result = (*left ? CONST_BOOL_TRUE : CONST_BOOL_FALSE) != (*right ? CONST_BOOL_TRUE : CONST_BOOL_FALSE);
}

// EXPR_LOGICAL_OR
static INLINE void op_logical_or__const_bool__const_bool__const_bool(const_bool* result, const_bool* left, const_bool* right)
{
	*result = (*left) || (*right);
}

// EXPR_INTRINSIC_MIN
static INLINE void op_min__const_int__const_int__const_int(const_int* result, const_int* left, const_int* right)
{
   *result = (*left) < (*right) ? (*left) : (*right);
}
static INLINE void op_min__const_float__const_float__const_float(const_float* result, const_float* left, const_float* right)
{
   glsl_fpu_min((unsigned *)result, *left, *right);
}

// EXPR_INTRINSIC_MAX
static INLINE void op_max__const_int__const_int__const_int(const_int* result, const_int* left, const_int* right)
{
   *result = (*left) > (*right) ? (*left) : (*right);
}
static INLINE void op_max__const_float__const_float__const_float(const_float* result, const_float* left, const_float* right)
{
   glsl_fpu_max((unsigned *)result, *left, *right);
}


//
// Linear algebraic multiplies.
//

static INLINE void op_mul__const_mat2__const_mat2__const_mat2(const_mat2* result, const_mat2* left, const_mat2* right)
{
	int i, j, k;
	const_float mul;
	const int DIM = 2;

	for (i = 0; i < DIM; i++)
	{
		for (j = 0; j < DIM; j++)
		{
			(*result)[i][j] = CONST_FLOAT_ZERO;

			for (k = 0; k < DIM; k++)
			{
				op_mul__const_float__const_float__const_float(&mul, &(*left)[k][j], &(*right)[i][k]);
				op_add__const_float__const_float__const_float(&(*result)[i][j], &(*result)[i][j], &mul);
			}
		}
	}
}
static INLINE void op_mul__const_mat3__const_mat3__const_mat3(const_mat3* result, const_mat3* left, const_mat3* right)
{
	int i, j, k;
	const_float mul;
	const int DIM = 3;

	for (i = 0; i < DIM; i++)
	{
		for (j = 0; j < DIM; j++)
		{
			(*result)[i][j] = CONST_FLOAT_ZERO;

			for (k = 0; k < DIM; k++)
			{
				op_mul__const_float__const_float__const_float(&mul, &(*left)[k][j], &(*right)[i][k]);
				op_add__const_float__const_float__const_float(&(*result)[i][j], &(*result)[i][j], &mul);
			}
		}
	}
}
static INLINE void op_mul__const_mat4__const_mat4__const_mat4(const_mat4* result, const_mat4* left, const_mat4* right)
{
	int i, j, k;
	const_float mul;
	const int DIM = 4;

	for (i = 0; i < DIM; i++)
	{
		for (j = 0; j < DIM; j++)
		{
			(*result)[i][j] = CONST_FLOAT_ZERO;

			for (k = 0; k < DIM; k++)
			{
				op_mul__const_float__const_float__const_float(&mul, &(*left)[k][j], &(*right)[i][k]);
				op_add__const_float__const_float__const_float(&(*result)[i][j], &(*result)[i][j], &mul);
			}
		}
	}
}

static INLINE void op_mul__const_vec2__const_vec2__const_mat2(const_vec2* result, const_vec2* left, const_mat2* right)
{
	int i, k;
	const_float mul;
	const int DIM = 2;

	for (i = 0; i < DIM; i++)
	{
		(*result)[i] = CONST_FLOAT_ZERO;

		for (k = 0; k < DIM; k++)
		{
			op_mul__const_float__const_float__const_float(&mul, &(*left)[k], &(*right)[i][k]);
			op_add__const_float__const_float__const_float(&(*result)[i], &(*result)[i], &mul);
		}
	}
}
static INLINE void op_mul__const_vec3__const_vec3__const_mat3(const_vec3* result, const_vec3* left, const_mat3* right)
{
	int i, k;
	const_float mul;
	const int DIM = 3;

	for (i = 0; i < DIM; i++)
	{
		(*result)[i] = CONST_FLOAT_ZERO;

		for (k = 0; k < DIM; k++)
		{
			op_mul__const_float__const_float__const_float(&mul, &(*left)[k], &(*right)[i][k]);
			op_add__const_float__const_float__const_float(&(*result)[i], &(*result)[i], &mul);
		}
	}
}
static INLINE void op_mul__const_vec4__const_vec4__const_mat4(const_vec4* result, const_vec4* left, const_mat4* right)
{
	int i, k;
	const_float mul;
	const int DIM = 4;

	for (i = 0; i < DIM; i++)
	{
		(*result)[i] = CONST_FLOAT_ZERO;

		for (k = 0; k < DIM; k++)
		{
			op_mul__const_float__const_float__const_float(&mul, &(*left)[k], &(*right)[i][k]);
			op_add__const_float__const_float__const_float(&(*result)[i], &(*result)[i], &mul);
		}
	}
}

static INLINE void op_mul__const_vec2__const_mat2__const_vec2(const_vec2* result, const_mat2* left, const_vec2* right)
{
	int j, k;
	const_float mul;
	const int DIM = 2;

	for (j = 0; j < DIM; j++)
	{
		(*result)[j] = CONST_FLOAT_ZERO;

		for (k = 0; k < DIM; k++)
		{
			op_mul__const_float__const_float__const_float(&mul, &(*left)[k][j], &(*right)[k]);
			op_add__const_float__const_float__const_float(&(*result)[j], &(*result)[j], &mul);
		}
	}
}
static INLINE void op_mul__const_vec3__const_mat3__const_vec3(const_vec3* result, const_mat3* left, const_vec3* right)
{
	int j, k;
	const_float mul;
	const int DIM = 3;

	for (j = 0; j < DIM; j++)
	{
		(*result)[j] = CONST_FLOAT_ZERO;

		for (k = 0; k < DIM; k++)
		{
			op_mul__const_float__const_float__const_float(&mul, &(*left)[k][j], &(*right)[k]);
			op_add__const_float__const_float__const_float(&(*result)[j], &(*result)[j], &mul);
		}
	}
}
static INLINE void op_mul__const_vec4__const_mat4__const_vec4(const_vec4* result, const_mat4* left, const_vec4* right)
{
	int j, k;
	const_float mul;
	const int DIM = 4;

	for (j = 0; j < DIM; j++)
	{
		(*result)[j] = CONST_FLOAT_ZERO;

		for (k = 0; k < DIM; k++)
		{
			op_mul__const_float__const_float__const_float(&mul, &(*left)[k][j], &(*right)[k]);
			op_add__const_float__const_float__const_float(&(*result)[j], &(*result)[j], &mul);
		}
	}
}

static INLINE void op_rsqrt__const_float__const_float(const_float* result, const_float* operand)
{
	glsl_fpu_rsqrt((unsigned *)result, *operand);
}

static INLINE void op_recip__const_float__const_float(const_float* result, const_float* operand)
{
	glsl_fpu_recip((unsigned *)result, *operand);
}

static INLINE void op_log2__const_float__const_float(const_float* result, const_float* operand)
{
	glsl_fpu_log2((unsigned *)result, *operand);
}

static INLINE void op_exp2__const_float__const_float(const_float* result, const_float* operand)
{
	glsl_fpu_exp2((unsigned *)result, *operand);
}

static INLINE void op_ceil__const_float__const_float(const_float* result, const_float* operand)
{
	glsl_fpu_ceil((unsigned *)result, *operand);
}

static INLINE void op_floor__const_float__const_float(const_float* result, const_float* operand)
{
	glsl_fpu_floor((unsigned *)result, *operand);
}

static INLINE void op_floattoint_trunc__const_int__const_float(const_int* result, const_float* operand)
{
	glsl_fpu_floattointz((unsigned *)result, *operand, 0);
}

static INLINE void op_floattoint_nearest__const_int__const_float(const_int* result, const_float* operand)
{
	glsl_fpu_floattointn((unsigned *)result, *operand, 0);
}

static INLINE void op_inttofloat__const_float__const_int(const_float *result, const_int* operand)
{
	glsl_fpu_inttofloat((unsigned *)result, *operand, 0);
}

#endif // CONST_OPERATORS_H