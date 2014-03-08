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

#include "middleware/khronos/glsl/glsl_header.h"
#include "middleware/khronos/glsl/glsl_const_functions.h"
#include "middleware/khronos/glsl/glsl_const_operators.h"
#include "middleware/khronos/glsl/glsl_mendenhall.h"
#include "middleware/khronos/glsl/glsl_trace.h"
#include "middleware/khronos/glsl/glsl_intern.h"
#include "middleware/khronos/glsl/glsl_fastmem.h"

// NOTICE! Parts of this file are AUTO-GENERATED. See Scripts/const_functions.py.
// To save your sanity, try editing that file first.

SymbolType constantFunctionSignatures[CONST_FUNCTION_SIG_COUNT];
Symbol constantFunctions[CONST_FUNCTION_COUNT];



//
// Constant function declarations.
//

// Angle and Trigonometry Functions
static void radians__const_float__const_float(const_float* result, const_float* degrees);
static void radians__const_vec2__const_vec2(const_vec2* result, const_vec2* degrees);
static void radians__const_vec3__const_vec3(const_vec3* result, const_vec3* degrees);
static void radians__const_vec4__const_vec4(const_vec4* result, const_vec4* degrees);
static void degrees__const_float__const_float(const_float* result, const_float* radians);
static void degrees__const_vec2__const_vec2(const_vec2* result, const_vec2* radians);
static void degrees__const_vec3__const_vec3(const_vec3* result, const_vec3* radians);
static void degrees__const_vec4__const_vec4(const_vec4* result, const_vec4* radians);
static void sin__const_float__const_float(const_float* result, const_float* angle);
static void sin__const_vec2__const_vec2(const_vec2* result, const_vec2* angle);
static void sin__const_vec3__const_vec3(const_vec3* result, const_vec3* angle);
static void sin__const_vec4__const_vec4(const_vec4* result, const_vec4* angle);
static void cos__const_float__const_float(const_float* result, const_float* angle);
static void cos__const_vec2__const_vec2(const_vec2* result, const_vec2* angle);
static void cos__const_vec3__const_vec3(const_vec3* result, const_vec3* angle);
static void cos__const_vec4__const_vec4(const_vec4* result, const_vec4* angle);
static void tan__const_float__const_float(const_float* result, const_float* angle);
static void tan__const_vec2__const_vec2(const_vec2* result, const_vec2* angle);
static void tan__const_vec3__const_vec3(const_vec3* result, const_vec3* angle);
static void tan__const_vec4__const_vec4(const_vec4* result, const_vec4* angle);
static void asin__const_float__const_float(const_float* result, const_float* x);
static void asin__const_vec2__const_vec2(const_vec2* result, const_vec2* x);
static void asin__const_vec3__const_vec3(const_vec3* result, const_vec3* x);
static void asin__const_vec4__const_vec4(const_vec4* result, const_vec4* x);
static void acos__const_float__const_float(const_float* result, const_float* x);
static void acos__const_vec2__const_vec2(const_vec2* result, const_vec2* x);
static void acos__const_vec3__const_vec3(const_vec3* result, const_vec3* x);
static void acos__const_vec4__const_vec4(const_vec4* result, const_vec4* x);
static void atan__const_float__const_float__const_float(const_float* result, const_float* y, const_float* x);
static void atan__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* y, const_vec2* x);
static void atan__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* y, const_vec3* x);
static void atan__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* y, const_vec4* x);
static void atan__const_float__const_float(const_float* result, const_float* y_over_x);
static void atan__const_vec2__const_vec2(const_vec2* result, const_vec2* y_over_x);
static void atan__const_vec3__const_vec3(const_vec3* result, const_vec3* y_over_x);
static void atan__const_vec4__const_vec4(const_vec4* result, const_vec4* y_over_x);

// Exponential Functions
static void pow__const_float__const_float__const_float(const_float* result, const_float* x, const_float* y);
static void pow__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* x, const_vec2* y);
static void pow__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* x, const_vec3* y);
static void pow__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* x, const_vec4* y);
static void exp__const_float__const_float(const_float* result, const_float* x);
static void exp__const_vec2__const_vec2(const_vec2* result, const_vec2* x);
static void exp__const_vec3__const_vec3(const_vec3* result, const_vec3* x);
static void exp__const_vec4__const_vec4(const_vec4* result, const_vec4* x);
static void log__const_float__const_float(const_float* result, const_float* x);
static void log__const_vec2__const_vec2(const_vec2* result, const_vec2* x);
static void log__const_vec3__const_vec3(const_vec3* result, const_vec3* x);
static void log__const_vec4__const_vec4(const_vec4* result, const_vec4* x);
static void exp2__const_float__const_float(const_float* result, const_float* x);
static void exp2__const_vec2__const_vec2(const_vec2* result, const_vec2* x);
static void exp2__const_vec3__const_vec3(const_vec3* result, const_vec3* x);
static void exp2__const_vec4__const_vec4(const_vec4* result, const_vec4* x);
static void log2__const_float__const_float(const_float* result, const_float* x);
static void log2__const_vec2__const_vec2(const_vec2* result, const_vec2* x);
static void log2__const_vec3__const_vec3(const_vec3* result, const_vec3* x);
static void log2__const_vec4__const_vec4(const_vec4* result, const_vec4* x);
static void sqrt__const_float__const_float(const_float* result, const_float* x);
static void sqrt__const_vec2__const_vec2(const_vec2* result, const_vec2* x);
static void sqrt__const_vec3__const_vec3(const_vec3* result, const_vec3* x);
static void sqrt__const_vec4__const_vec4(const_vec4* result, const_vec4* x);
static void inversesqrt__const_float__const_float(const_float* result, const_float* x);
static void inversesqrt__const_vec2__const_vec2(const_vec2* result, const_vec2* x);
static void inversesqrt__const_vec3__const_vec3(const_vec3* result, const_vec3* x);
static void inversesqrt__const_vec4__const_vec4(const_vec4* result, const_vec4* x);

// Common Functions
static void abs__const_float__const_float(const_float* result, const_float* x);
static void abs__const_vec2__const_vec2(const_vec2* result, const_vec2* x);
static void abs__const_vec3__const_vec3(const_vec3* result, const_vec3* x);
static void abs__const_vec4__const_vec4(const_vec4* result, const_vec4* x);
static void sign__const_float__const_float(const_float* result, const_float* x);
static void sign__const_vec2__const_vec2(const_vec2* result, const_vec2* x);
static void sign__const_vec3__const_vec3(const_vec3* result, const_vec3* x);
static void sign__const_vec4__const_vec4(const_vec4* result, const_vec4* x);
static void floor__const_float__const_float(const_float* result, const_float* x);
static void floor__const_vec2__const_vec2(const_vec2* result, const_vec2* x);
static void floor__const_vec3__const_vec3(const_vec3* result, const_vec3* x);
static void floor__const_vec4__const_vec4(const_vec4* result, const_vec4* x);
static void ceil__const_float__const_float(const_float* result, const_float* x);
static void ceil__const_vec2__const_vec2(const_vec2* result, const_vec2* x);
static void ceil__const_vec3__const_vec3(const_vec3* result, const_vec3* x);
static void ceil__const_vec4__const_vec4(const_vec4* result, const_vec4* x);
static void fract__const_float__const_float(const_float* result, const_float* x);
static void fract__const_vec2__const_vec2(const_vec2* result, const_vec2* x);
static void fract__const_vec3__const_vec3(const_vec3* result, const_vec3* x);
static void fract__const_vec4__const_vec4(const_vec4* result, const_vec4* x);
static void mod__const_float__const_float__const_float(const_float* result, const_float* x, const_float* y);
static void mod__const_vec2__const_vec2__const_float(const_vec2* result, const_vec2* x, const_float* y);
static void mod__const_vec3__const_vec3__const_float(const_vec3* result, const_vec3* x, const_float* y);
static void mod__const_vec4__const_vec4__const_float(const_vec4* result, const_vec4* x, const_float* y);
static void mod__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* x, const_vec2* y);
static void mod__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* x, const_vec3* y);
static void mod__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* x, const_vec4* y);
static void min__const_float__const_float__const_float(const_float* result, const_float* x, const_float* y);
static void min__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* x, const_vec2* y);
static void min__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* x, const_vec3* y);
static void min__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* x, const_vec4* y);
static void min__const_vec2__const_vec2__const_float(const_vec2* result, const_vec2* x, const_float* y);
static void min__const_vec3__const_vec3__const_float(const_vec3* result, const_vec3* x, const_float* y);
static void min__const_vec4__const_vec4__const_float(const_vec4* result, const_vec4* x, const_float* y);
static void max__const_float__const_float__const_float(const_float* result, const_float* x, const_float* y);
static void max__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* x, const_vec2* y);
static void max__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* x, const_vec3* y);
static void max__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* x, const_vec4* y);
static void max__const_vec2__const_vec2__const_float(const_vec2* result, const_vec2* x, const_float* y);
static void max__const_vec3__const_vec3__const_float(const_vec3* result, const_vec3* x, const_float* y);
static void max__const_vec4__const_vec4__const_float(const_vec4* result, const_vec4* x, const_float* y);
static void clamp__const_float__const_float__const_float__const_float(const_float* result, const_float* x, const_float* minVal, const_float* maxVal);
static void clamp__const_vec2__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* x, const_vec2* minVal, const_vec2* maxVal);
static void clamp__const_vec3__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* x, const_vec3* minVal, const_vec3* maxVal);
static void clamp__const_vec4__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* x, const_vec4* minVal, const_vec4* maxVal);
static void clamp__const_vec2__const_vec2__const_float__const_float(const_vec2* result, const_vec2* x, const_float* minVal, const_float* maxVal);
static void clamp__const_vec3__const_vec3__const_float__const_float(const_vec3* result, const_vec3* x, const_float* minVal, const_float* maxVal);
static void clamp__const_vec4__const_vec4__const_float__const_float(const_vec4* result, const_vec4* x, const_float* minVal, const_float* maxVal);
static void mix__const_float__const_float__const_float__const_float(const_float* result, const_float* x, const_float* y, const_float* a);
static void mix__const_vec2__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* x, const_vec2* y, const_vec2* a);
static void mix__const_vec3__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* x, const_vec3* y, const_vec3* a);
static void mix__const_vec4__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* x, const_vec4* y, const_vec4* a);
static void mix__const_vec2__const_vec2__const_vec2__const_float(const_vec2* result, const_vec2* x, const_vec2* y, const_float* a);
static void mix__const_vec3__const_vec3__const_vec3__const_float(const_vec3* result, const_vec3* x, const_vec3* y, const_float* a);
static void mix__const_vec4__const_vec4__const_vec4__const_float(const_vec4* result, const_vec4* x, const_vec4* y, const_float* a);
static void step__const_float__const_float__const_float(const_float* result, const_float* edge, const_float* x);
static void step__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* edge, const_vec2* x);
static void step__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* edge, const_vec3* x);
static void step__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* edge, const_vec4* x);
static void step__const_vec2__const_float__const_vec2(const_vec2* result, const_float* edge, const_vec2* x);
static void step__const_vec3__const_float__const_vec3(const_vec3* result, const_float* edge, const_vec3* x);
static void step__const_vec4__const_float__const_vec4(const_vec4* result, const_float* edge, const_vec4* x);
static void smoothstep__const_float__const_float__const_float__const_float(const_float* result, const_float* edge0, const_float* edge1, const_float* x);
static void smoothstep__const_vec2__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* edge0, const_vec2* edge1, const_vec2* x);
static void smoothstep__const_vec3__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* edge0, const_vec3* edge1, const_vec3* x);
static void smoothstep__const_vec4__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* edge0, const_vec4* edge1, const_vec4* x);
static void smoothstep__const_vec2__const_float__const_float__const_vec2(const_vec2* result, const_float* edge0, const_float* edge1, const_vec2* x);
static void smoothstep__const_vec3__const_float__const_float__const_vec3(const_vec3* result, const_float* edge0, const_float* edge1, const_vec3* x);
static void smoothstep__const_vec4__const_float__const_float__const_vec4(const_vec4* result, const_float* edge0, const_float* edge1, const_vec4* x);

// Geometric Functions
static void length__const_float__const_float(const_float* result, const_float* x);
static void length__const_float__const_vec2(const_float* result, const_vec2* x);
static void length__const_float__const_vec3(const_float* result, const_vec3* x);
static void length__const_float__const_vec4(const_float* result, const_vec4* x);
static void distance__const_float__const_float__const_float(const_float* result, const_float* p0, const_float* p1);
static void distance__const_float__const_vec2__const_vec2(const_float* result, const_vec2* p0, const_vec2* p1);
static void distance__const_float__const_vec3__const_vec3(const_float* result, const_vec3* p0, const_vec3* p1);
static void distance__const_float__const_vec4__const_vec4(const_float* result, const_vec4* p0, const_vec4* p1);
static void dot__const_float__const_float__const_float(const_float* result, const_float* x, const_float* y);
static void dot__const_float__const_vec2__const_vec2(const_float* result, const_vec2* x, const_vec2* y);
static void dot__const_float__const_vec3__const_vec3(const_float* result, const_vec3* x, const_vec3* y);
static void dot__const_float__const_vec4__const_vec4(const_float* result, const_vec4* x, const_vec4* y);
static void cross__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* x, const_vec3* y);
static void normalize__const_float__const_float(const_float* result, const_float* x);
static void normalize__const_vec2__const_vec2(const_vec2* result, const_vec2* x);
static void normalize__const_vec3__const_vec3(const_vec3* result, const_vec3* x);
static void normalize__const_vec4__const_vec4(const_vec4* result, const_vec4* x);
static void faceforward__const_float__const_float__const_float__const_float(const_float* result, const_float* N, const_float* I, const_float* Nref);
static void faceforward__const_vec2__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* N, const_vec2* I, const_vec2* Nref);
static void faceforward__const_vec3__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* N, const_vec3* I, const_vec3* Nref);
static void faceforward__const_vec4__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* N, const_vec4* I, const_vec4* Nref);
static void reflect__const_float__const_float__const_float(const_float* result, const_float* I, const_float* N);
static void reflect__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* I, const_vec2* N);
static void reflect__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* I, const_vec3* N);
static void reflect__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* I, const_vec4* N);
static void refract__const_float__const_float__const_float__const_float(const_float* result, const_float* I, const_float* N, const_float* eta);
static void refract__const_vec2__const_vec2__const_vec2__const_float(const_vec2* result, const_vec2* I, const_vec2* N, const_float* eta);
static void refract__const_vec3__const_vec3__const_vec3__const_float(const_vec3* result, const_vec3* I, const_vec3* N, const_float* eta);
static void refract__const_vec4__const_vec4__const_vec4__const_float(const_vec4* result, const_vec4* I, const_vec4* N, const_float* eta);

// Matrix Functions
static void matrixCompMult__const_mat2__const_mat2__const_mat2(const_mat2* result, const_mat2* x, const_mat2* y);
static void matrixCompMult__const_mat3__const_mat3__const_mat3(const_mat3* result, const_mat3* x, const_mat3* y);
static void matrixCompMult__const_mat4__const_mat4__const_mat4(const_mat4* result, const_mat4* x, const_mat4* y);

// Vector Relational Functions
static void lessThan__const_bvec2__const_vec2__const_vec2(const_bvec2* result, const_vec2* x, const_vec2* y);
static void lessThan__const_bvec3__const_vec3__const_vec3(const_bvec3* result, const_vec3* x, const_vec3* y);
static void lessThan__const_bvec4__const_vec4__const_vec4(const_bvec4* result, const_vec4* x, const_vec4* y);
static void lessThan__const_bvec2__const_ivec2__const_ivec2(const_bvec2* result, const_ivec2* x, const_ivec2* y);
static void lessThan__const_bvec3__const_ivec3__const_ivec3(const_bvec3* result, const_ivec3* x, const_ivec3* y);
static void lessThan__const_bvec4__const_ivec4__const_ivec4(const_bvec4* result, const_ivec4* x, const_ivec4* y);
static void lessThanEqual__const_bvec2__const_vec2__const_vec2(const_bvec2* result, const_vec2* x, const_vec2* y);
static void lessThanEqual__const_bvec3__const_vec3__const_vec3(const_bvec3* result, const_vec3* x, const_vec3* y);
static void lessThanEqual__const_bvec4__const_vec4__const_vec4(const_bvec4* result, const_vec4* x, const_vec4* y);
static void lessThanEqual__const_bvec2__const_ivec2__const_ivec2(const_bvec2* result, const_ivec2* x, const_ivec2* y);
static void lessThanEqual__const_bvec3__const_ivec3__const_ivec3(const_bvec3* result, const_ivec3* x, const_ivec3* y);
static void lessThanEqual__const_bvec4__const_ivec4__const_ivec4(const_bvec4* result, const_ivec4* x, const_ivec4* y);
static void greaterThan__const_bvec2__const_vec2__const_vec2(const_bvec2* result, const_vec2* x, const_vec2* y);
static void greaterThan__const_bvec3__const_vec3__const_vec3(const_bvec3* result, const_vec3* x, const_vec3* y);
static void greaterThan__const_bvec4__const_vec4__const_vec4(const_bvec4* result, const_vec4* x, const_vec4* y);
static void greaterThan__const_bvec2__const_ivec2__const_ivec2(const_bvec2* result, const_ivec2* x, const_ivec2* y);
static void greaterThan__const_bvec3__const_ivec3__const_ivec3(const_bvec3* result, const_ivec3* x, const_ivec3* y);
static void greaterThan__const_bvec4__const_ivec4__const_ivec4(const_bvec4* result, const_ivec4* x, const_ivec4* y);
static void greaterThanEqual__const_bvec2__const_vec2__const_vec2(const_bvec2* result, const_vec2* x, const_vec2* y);
static void greaterThanEqual__const_bvec3__const_vec3__const_vec3(const_bvec3* result, const_vec3* x, const_vec3* y);
static void greaterThanEqual__const_bvec4__const_vec4__const_vec4(const_bvec4* result, const_vec4* x, const_vec4* y);
static void greaterThanEqual__const_bvec2__const_ivec2__const_ivec2(const_bvec2* result, const_ivec2* x, const_ivec2* y);
static void greaterThanEqual__const_bvec3__const_ivec3__const_ivec3(const_bvec3* result, const_ivec3* x, const_ivec3* y);
static void greaterThanEqual__const_bvec4__const_ivec4__const_ivec4(const_bvec4* result, const_ivec4* x, const_ivec4* y);
static void equal__const_bvec2__const_vec2__const_vec2(const_bvec2* result, const_vec2* x, const_vec2* y);
static void equal__const_bvec3__const_vec3__const_vec3(const_bvec3* result, const_vec3* x, const_vec3* y);
static void equal__const_bvec4__const_vec4__const_vec4(const_bvec4* result, const_vec4* x, const_vec4* y);
static void equal__const_bvec2__const_ivec2__const_ivec2(const_bvec2* result, const_ivec2* x, const_ivec2* y);
static void equal__const_bvec3__const_ivec3__const_ivec3(const_bvec3* result, const_ivec3* x, const_ivec3* y);
static void equal__const_bvec4__const_ivec4__const_ivec4(const_bvec4* result, const_ivec4* x, const_ivec4* y);
static void equal__const_bvec2__const_bvec2__const_bvec2(const_bvec2* result, const_bvec2* x, const_bvec2* y);
static void equal__const_bvec3__const_bvec3__const_bvec3(const_bvec3* result, const_bvec3* x, const_bvec3* y);
static void equal__const_bvec4__const_bvec4__const_bvec4(const_bvec4* result, const_bvec4* x, const_bvec4* y);
static void notEqual__const_bvec2__const_vec2__const_vec2(const_bvec2* result, const_vec2* x, const_vec2* y);
static void notEqual__const_bvec3__const_vec3__const_vec3(const_bvec3* result, const_vec3* x, const_vec3* y);
static void notEqual__const_bvec4__const_vec4__const_vec4(const_bvec4* result, const_vec4* x, const_vec4* y);
static void notEqual__const_bvec2__const_ivec2__const_ivec2(const_bvec2* result, const_ivec2* x, const_ivec2* y);
static void notEqual__const_bvec3__const_ivec3__const_ivec3(const_bvec3* result, const_ivec3* x, const_ivec3* y);
static void notEqual__const_bvec4__const_ivec4__const_ivec4(const_bvec4* result, const_ivec4* x, const_ivec4* y);
static void notEqual__const_bvec2__const_bvec2__const_bvec2(const_bvec2* result, const_bvec2* x, const_bvec2* y);
static void notEqual__const_bvec3__const_bvec3__const_bvec3(const_bvec3* result, const_bvec3* x, const_bvec3* y);
static void notEqual__const_bvec4__const_bvec4__const_bvec4(const_bvec4* result, const_bvec4* x, const_bvec4* y);
static void any__const_bool__const_bvec2(const_bool* result, const_bvec2* x);
static void any__const_bool__const_bvec3(const_bool* result, const_bvec3* x);
static void any__const_bool__const_bvec4(const_bool* result, const_bvec4* x);
static void all__const_bool__const_bvec2(const_bool* result, const_bvec2* x);
static void all__const_bool__const_bvec3(const_bool* result, const_bvec3* x);
static void all__const_bool__const_bvec4(const_bool* result, const_bvec4* x);
static void not__const_bvec2__const_bvec2(const_bvec2* result, const_bvec2* x);
static void not__const_bvec3__const_bvec3(const_bvec3* result, const_bvec3* x);
static void not__const_bvec4__const_bvec4(const_bvec4* result, const_bvec4* x);


static void init_const_function_signatures(void)
{
	{
		int i;
		
		for (i = 0; i < CONST_FUNCTION_SIG_COUNT; i++)
		{
			constantFunctionSignatures[i].flavour = SYMBOL_FUNCTION_TYPE;
			constantFunctionSignatures[i].size_as_const = 0;
			constantFunctionSignatures[i].scalar_count = 0;
		}

		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT].name = "(float)";
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT].u.function_type.param_count = 1;
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 1);
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT].u.function_type.params[0] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT].u.function_type.return_type = &primitiveTypes[PRIM_FLOAT];

		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2].name = "(vec2)";
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2].u.function_type.param_count = 1;
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 1);
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2].u.function_type.return_type = &primitiveTypes[PRIM_VEC2];

		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3].name = "(vec3)";
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3].u.function_type.param_count = 1;
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 1);
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3].u.function_type.return_type = &primitiveTypes[PRIM_VEC3];

		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4].name = "(vec4)";
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4].u.function_type.param_count = 1;
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 1);
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4].u.function_type.return_type = &primitiveTypes[PRIM_VEC4];

		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT].name = "(float, float)";
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT].u.function_type.params[0] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT].u.function_type.params[1] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT].u.function_type.return_type = &primitiveTypes[PRIM_FLOAT];

		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2].name = "(vec2, vec2)";
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2].u.function_type.return_type = &primitiveTypes[PRIM_VEC2];

		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3].name = "(vec3, vec3)";
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3].u.function_type.return_type = &primitiveTypes[PRIM_VEC3];

		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4].name = "(vec4, vec4)";
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4].u.function_type.return_type = &primitiveTypes[PRIM_VEC4];

		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT].name = "(vec2, float)";
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT].u.function_type.params[1] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT].u.function_type.return_type = &primitiveTypes[PRIM_VEC2];

		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT].name = "(vec3, float)";
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT].u.function_type.params[1] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT].u.function_type.return_type = &primitiveTypes[PRIM_VEC3];

		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT].name = "(vec4, float)";
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT].u.function_type.params[1] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT].u.function_type.return_type = &primitiveTypes[PRIM_VEC4];

		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT].name = "(float, float, float)";
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT].u.function_type.param_count = 3;
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 3);
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT].u.function_type.params[0] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT].u.function_type.params[1] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT].u.function_type.params[2] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT].u.function_type.return_type = &primitiveTypes[PRIM_FLOAT];

		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2].name = "(vec2, vec2, vec2)";
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2].u.function_type.param_count = 3;
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 3);
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2].u.function_type.params[2] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2].u.function_type.return_type = &primitiveTypes[PRIM_VEC2];

		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3].name = "(vec3, vec3, vec3)";
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3].u.function_type.param_count = 3;
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 3);
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3].u.function_type.params[2] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3].u.function_type.return_type = &primitiveTypes[PRIM_VEC3];

		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4].name = "(vec4, vec4, vec4)";
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4].u.function_type.param_count = 3;
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 3);
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4].u.function_type.params[2] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4].u.function_type.return_type = &primitiveTypes[PRIM_VEC4];

		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT__CONST_FLOAT].name = "(vec2, float, float)";
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT__CONST_FLOAT].u.function_type.param_count = 3;
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT__CONST_FLOAT].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 3);
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT__CONST_FLOAT].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT__CONST_FLOAT].u.function_type.params[1] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT__CONST_FLOAT].u.function_type.params[2] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT__CONST_FLOAT].u.function_type.return_type = &primitiveTypes[PRIM_VEC2];

		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT__CONST_FLOAT].name = "(vec3, float, float)";
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT__CONST_FLOAT].u.function_type.param_count = 3;
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT__CONST_FLOAT].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 3);
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT__CONST_FLOAT].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT__CONST_FLOAT].u.function_type.params[1] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT__CONST_FLOAT].u.function_type.params[2] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT__CONST_FLOAT].u.function_type.return_type = &primitiveTypes[PRIM_VEC3];

		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT__CONST_FLOAT].name = "(vec4, float, float)";
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT__CONST_FLOAT].u.function_type.param_count = 3;
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT__CONST_FLOAT].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 3);
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT__CONST_FLOAT].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT__CONST_FLOAT].u.function_type.params[1] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT__CONST_FLOAT].u.function_type.params[2] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT__CONST_FLOAT].u.function_type.return_type = &primitiveTypes[PRIM_VEC4];

		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_FLOAT].name = "(vec2, vec2, float)";
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_FLOAT].u.function_type.param_count = 3;
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_FLOAT].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 3);
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_FLOAT].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_FLOAT].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_FLOAT].u.function_type.params[2] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_FLOAT].u.function_type.return_type = &primitiveTypes[PRIM_VEC2];

		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_FLOAT].name = "(vec3, vec3, float)";
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_FLOAT].u.function_type.param_count = 3;
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_FLOAT].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 3);
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_FLOAT].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_FLOAT].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_FLOAT].u.function_type.params[2] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_FLOAT].u.function_type.return_type = &primitiveTypes[PRIM_VEC3];

		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_FLOAT].name = "(vec4, vec4, float)";
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_FLOAT].u.function_type.param_count = 3;
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_FLOAT].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 3);
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_FLOAT].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_FLOAT].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_FLOAT].u.function_type.params[2] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_FLOAT].u.function_type.return_type = &primitiveTypes[PRIM_VEC4];

		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_VEC2].name = "(float, vec2)";
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_VEC2].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_VEC2].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_VEC2].u.function_type.params[0] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_VEC2].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_VEC2].u.function_type.return_type = &primitiveTypes[PRIM_VEC2];

		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_VEC3].name = "(float, vec3)";
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_VEC3].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_VEC3].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_VEC3].u.function_type.params[0] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_VEC3].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_VEC3].u.function_type.return_type = &primitiveTypes[PRIM_VEC3];

		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_VEC4].name = "(float, vec4)";
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_VEC4].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_VEC4].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_VEC4].u.function_type.params[0] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_VEC4].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_VEC4].u.function_type.return_type = &primitiveTypes[PRIM_VEC4];

		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_FLOAT__CONST_VEC2].name = "(float, float, vec2)";
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_FLOAT__CONST_VEC2].u.function_type.param_count = 3;
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_FLOAT__CONST_VEC2].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 3);
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_FLOAT__CONST_VEC2].u.function_type.params[0] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_FLOAT__CONST_VEC2].u.function_type.params[1] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_FLOAT__CONST_VEC2].u.function_type.params[2] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_FLOAT__CONST_VEC2].u.function_type.return_type = &primitiveTypes[PRIM_VEC2];

		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_FLOAT__CONST_VEC3].name = "(float, float, vec3)";
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_FLOAT__CONST_VEC3].u.function_type.param_count = 3;
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_FLOAT__CONST_VEC3].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 3);
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_FLOAT__CONST_VEC3].u.function_type.params[0] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_FLOAT__CONST_VEC3].u.function_type.params[1] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_FLOAT__CONST_VEC3].u.function_type.params[2] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_FLOAT__CONST_VEC3].u.function_type.return_type = &primitiveTypes[PRIM_VEC3];

		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_FLOAT__CONST_VEC4].name = "(float, float, vec4)";
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_FLOAT__CONST_VEC4].u.function_type.param_count = 3;
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_FLOAT__CONST_VEC4].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 3);
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_FLOAT__CONST_VEC4].u.function_type.params[0] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_FLOAT__CONST_VEC4].u.function_type.params[1] = &primitiveParamSymbols[PRIM_FLOAT];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_FLOAT__CONST_VEC4].u.function_type.params[2] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_FLOAT__CONST_VEC4].u.function_type.return_type = &primitiveTypes[PRIM_VEC4];

		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC2].name = "(vec2)";
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC2].u.function_type.param_count = 1;
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC2].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 1);
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC2].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC2].u.function_type.return_type = &primitiveTypes[PRIM_FLOAT];

		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC3].name = "(vec3)";
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC3].u.function_type.param_count = 1;
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC3].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 1);
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC3].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC3].u.function_type.return_type = &primitiveTypes[PRIM_FLOAT];

		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC4].name = "(vec4)";
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC4].u.function_type.param_count = 1;
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC4].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 1);
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC4].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC4].u.function_type.return_type = &primitiveTypes[PRIM_FLOAT];

		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC2__CONST_VEC2].name = "(vec2, vec2)";
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC2__CONST_VEC2].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC2__CONST_VEC2].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC2__CONST_VEC2].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC2__CONST_VEC2].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC2__CONST_VEC2].u.function_type.return_type = &primitiveTypes[PRIM_FLOAT];

		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC3__CONST_VEC3].name = "(vec3, vec3)";
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC3__CONST_VEC3].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC3__CONST_VEC3].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC3__CONST_VEC3].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC3__CONST_VEC3].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC3__CONST_VEC3].u.function_type.return_type = &primitiveTypes[PRIM_FLOAT];

		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC4__CONST_VEC4].name = "(vec4, vec4)";
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC4__CONST_VEC4].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC4__CONST_VEC4].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC4__CONST_VEC4].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC4__CONST_VEC4].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC4__CONST_VEC4].u.function_type.return_type = &primitiveTypes[PRIM_FLOAT];

		constantFunctionSignatures[CF_SIG__CONST_MAT2__CONST_MAT2__CONST_MAT2].name = "(mat2, mat2)";
		constantFunctionSignatures[CF_SIG__CONST_MAT2__CONST_MAT2__CONST_MAT2].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_MAT2__CONST_MAT2__CONST_MAT2].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_MAT2__CONST_MAT2__CONST_MAT2].u.function_type.params[0] = &primitiveParamSymbols[PRIM_MAT2];
		constantFunctionSignatures[CF_SIG__CONST_MAT2__CONST_MAT2__CONST_MAT2].u.function_type.params[1] = &primitiveParamSymbols[PRIM_MAT2];
		constantFunctionSignatures[CF_SIG__CONST_MAT2__CONST_MAT2__CONST_MAT2].u.function_type.return_type = &primitiveTypes[PRIM_MAT2];

		constantFunctionSignatures[CF_SIG__CONST_MAT3__CONST_MAT3__CONST_MAT3].name = "(mat3, mat3)";
		constantFunctionSignatures[CF_SIG__CONST_MAT3__CONST_MAT3__CONST_MAT3].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_MAT3__CONST_MAT3__CONST_MAT3].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_MAT3__CONST_MAT3__CONST_MAT3].u.function_type.params[0] = &primitiveParamSymbols[PRIM_MAT3];
		constantFunctionSignatures[CF_SIG__CONST_MAT3__CONST_MAT3__CONST_MAT3].u.function_type.params[1] = &primitiveParamSymbols[PRIM_MAT3];
		constantFunctionSignatures[CF_SIG__CONST_MAT3__CONST_MAT3__CONST_MAT3].u.function_type.return_type = &primitiveTypes[PRIM_MAT3];

		constantFunctionSignatures[CF_SIG__CONST_MAT4__CONST_MAT4__CONST_MAT4].name = "(mat4, mat4)";
		constantFunctionSignatures[CF_SIG__CONST_MAT4__CONST_MAT4__CONST_MAT4].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_MAT4__CONST_MAT4__CONST_MAT4].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_MAT4__CONST_MAT4__CONST_MAT4].u.function_type.params[0] = &primitiveParamSymbols[PRIM_MAT4];
		constantFunctionSignatures[CF_SIG__CONST_MAT4__CONST_MAT4__CONST_MAT4].u.function_type.params[1] = &primitiveParamSymbols[PRIM_MAT4];
		constantFunctionSignatures[CF_SIG__CONST_MAT4__CONST_MAT4__CONST_MAT4].u.function_type.return_type = &primitiveTypes[PRIM_MAT4];

		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_VEC2__CONST_VEC2].name = "(vec2, vec2)";
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_VEC2__CONST_VEC2].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_VEC2__CONST_VEC2].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_VEC2__CONST_VEC2].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_VEC2__CONST_VEC2].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC2];
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_VEC2__CONST_VEC2].u.function_type.return_type = &primitiveTypes[PRIM_BVEC2];

		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_VEC3__CONST_VEC3].name = "(vec3, vec3)";
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_VEC3__CONST_VEC3].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_VEC3__CONST_VEC3].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_VEC3__CONST_VEC3].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_VEC3__CONST_VEC3].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC3];
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_VEC3__CONST_VEC3].u.function_type.return_type = &primitiveTypes[PRIM_BVEC3];

		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_VEC4__CONST_VEC4].name = "(vec4, vec4)";
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_VEC4__CONST_VEC4].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_VEC4__CONST_VEC4].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_VEC4__CONST_VEC4].u.function_type.params[0] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_VEC4__CONST_VEC4].u.function_type.params[1] = &primitiveParamSymbols[PRIM_VEC4];
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_VEC4__CONST_VEC4].u.function_type.return_type = &primitiveTypes[PRIM_BVEC4];

		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2].name = "(ivec2, ivec2)";
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2].u.function_type.params[0] = &primitiveParamSymbols[PRIM_IVEC2];
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2].u.function_type.params[1] = &primitiveParamSymbols[PRIM_IVEC2];
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2].u.function_type.return_type = &primitiveTypes[PRIM_BVEC2];

		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3].name = "(ivec3, ivec3)";
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3].u.function_type.params[0] = &primitiveParamSymbols[PRIM_IVEC3];
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3].u.function_type.params[1] = &primitiveParamSymbols[PRIM_IVEC3];
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3].u.function_type.return_type = &primitiveTypes[PRIM_BVEC3];

		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4].name = "(ivec4, ivec4)";
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4].u.function_type.params[0] = &primitiveParamSymbols[PRIM_IVEC4];
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4].u.function_type.params[1] = &primitiveParamSymbols[PRIM_IVEC4];
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4].u.function_type.return_type = &primitiveTypes[PRIM_BVEC4];

		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_BVEC2__CONST_BVEC2].name = "(bvec2, bvec2)";
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_BVEC2__CONST_BVEC2].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_BVEC2__CONST_BVEC2].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_BVEC2__CONST_BVEC2].u.function_type.params[0] = &primitiveParamSymbols[PRIM_BVEC2];
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_BVEC2__CONST_BVEC2].u.function_type.params[1] = &primitiveParamSymbols[PRIM_BVEC2];
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_BVEC2__CONST_BVEC2].u.function_type.return_type = &primitiveTypes[PRIM_BVEC2];

		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_BVEC3__CONST_BVEC3].name = "(bvec3, bvec3)";
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_BVEC3__CONST_BVEC3].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_BVEC3__CONST_BVEC3].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_BVEC3__CONST_BVEC3].u.function_type.params[0] = &primitiveParamSymbols[PRIM_BVEC3];
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_BVEC3__CONST_BVEC3].u.function_type.params[1] = &primitiveParamSymbols[PRIM_BVEC3];
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_BVEC3__CONST_BVEC3].u.function_type.return_type = &primitiveTypes[PRIM_BVEC3];

		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_BVEC4__CONST_BVEC4].name = "(bvec4, bvec4)";
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_BVEC4__CONST_BVEC4].u.function_type.param_count = 2;
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_BVEC4__CONST_BVEC4].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 2);
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_BVEC4__CONST_BVEC4].u.function_type.params[0] = &primitiveParamSymbols[PRIM_BVEC4];
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_BVEC4__CONST_BVEC4].u.function_type.params[1] = &primitiveParamSymbols[PRIM_BVEC4];
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_BVEC4__CONST_BVEC4].u.function_type.return_type = &primitiveTypes[PRIM_BVEC4];

		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC2].name = "(bvec2)";
		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC2].u.function_type.param_count = 1;
		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC2].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 1);
		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC2].u.function_type.params[0] = &primitiveParamSymbols[PRIM_BVEC2];
		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC2].u.function_type.return_type = &primitiveTypes[PRIM_BOOL];

		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC3].name = "(bvec3)";
		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC3].u.function_type.param_count = 1;
		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC3].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 1);
		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC3].u.function_type.params[0] = &primitiveParamSymbols[PRIM_BVEC3];
		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC3].u.function_type.return_type = &primitiveTypes[PRIM_BOOL];

		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC4].name = "(bvec4)";
		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC4].u.function_type.param_count = 1;
		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC4].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 1);
		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC4].u.function_type.params[0] = &primitiveParamSymbols[PRIM_BVEC4];
		constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC4].u.function_type.return_type = &primitiveTypes[PRIM_BOOL];

		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_BVEC2].name = "(bvec2)";
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_BVEC2].u.function_type.param_count = 1;
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_BVEC2].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 1);
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_BVEC2].u.function_type.params[0] = &primitiveParamSymbols[PRIM_BVEC2];
		constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_BVEC2].u.function_type.return_type = &primitiveTypes[PRIM_BVEC2];

		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_BVEC3].name = "(bvec3)";
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_BVEC3].u.function_type.param_count = 1;
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_BVEC3].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 1);
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_BVEC3].u.function_type.params[0] = &primitiveParamSymbols[PRIM_BVEC3];
		constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_BVEC3].u.function_type.return_type = &primitiveTypes[PRIM_BVEC3];

		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_BVEC4].name = "(bvec4)";
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_BVEC4].u.function_type.param_count = 1;
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_BVEC4].u.function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * 1);
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_BVEC4].u.function_type.params[0] = &primitiveParamSymbols[PRIM_BVEC4];
		constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_BVEC4].u.function_type.return_type = &primitiveTypes[PRIM_BVEC4];
	}
}

static void init_const_functions(void)
{
	{
		// Apologies for the weird order, but it comes out of a Python hashmap in a non-deterministic way.

		const char *intern_clamp = glsl_intern("clamp", false);
		const char *intern_matrixCompMult = glsl_intern("matrixCompMult", false);
		const char *intern_pow = glsl_intern("pow", false);
		const char *intern_greaterThanEqual = glsl_intern("greaterThanEqual", false);
		const char *intern_sign = glsl_intern("sign", false);
		const char *intern_equal = glsl_intern("equal", false);
		const char *intern_faceforward = glsl_intern("faceforward", false);
		const char *intern_tan = glsl_intern("tan", false);
		const char *intern_any = glsl_intern("any", false);
		const char *intern_normalize = glsl_intern("normalize", false);
		const char *intern_asin = glsl_intern("asin", false);
		const char *intern_log = glsl_intern("log", false);
		const char *intern_floor = glsl_intern("floor", false);
		const char *intern_exp2 = glsl_intern("exp2", false);
		const char *intern_lessThan = glsl_intern("lessThan", false);
		const char *intern_cross = glsl_intern("cross", false);
		const char *intern_sqrt = glsl_intern("sqrt", false);
		const char *intern_fract = glsl_intern("fract", false);
		const char *intern_abs = glsl_intern("abs", false);
		const char *intern_degrees = glsl_intern("degrees", false);
		const char *intern_sin = glsl_intern("sin", false);
		const char *intern_all = glsl_intern("all", false);
		const char *intern_log2 = glsl_intern("log2", false);
		const char *intern_atan = glsl_intern("atan", false);
		const char *intern_notEqual = glsl_intern("notEqual", false);
		const char *intern_max = glsl_intern("max", false);
		const char *intern_lessThanEqual = glsl_intern("lessThanEqual", false);
		const char *intern_ceil = glsl_intern("ceil", false);
		const char *intern_reflect = glsl_intern("reflect", false);
		const char *intern_step = glsl_intern("step", false);
		const char *intern_greaterThan = glsl_intern("greaterThan", false);
		const char *intern_not = glsl_intern("not", false);
		const char *intern_inversesqrt = glsl_intern("inversesqrt", false);
		const char *intern_mod = glsl_intern("mod", false);
		const char *intern_distance = glsl_intern("distance", false);
		const char *intern_cos = glsl_intern("cos", false);
		const char *intern_refract = glsl_intern("refract", false);
		const char *intern_min = glsl_intern("min", false);
		const char *intern_radians = glsl_intern("radians", false);
		const char *intern_smoothstep = glsl_intern("smoothstep", false);
		const char *intern_length = glsl_intern("length", false);
		const char *intern_exp = glsl_intern("exp", false);
		const char *intern_acos = glsl_intern("acos", false);
		const char *intern_mix = glsl_intern("mix", false);
		const char *intern_dot = glsl_intern("dot", false);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__CLAMP__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			intern_clamp,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)clamp__const_float__const_float__const_float__const_float,
			clamp__const_float__const_float__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__CLAMP__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			intern_clamp,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)clamp__const_vec2__const_vec2__const_vec2__const_vec2,
			clamp__const_vec2__const_vec2__const_vec2__const_vec2__body,
			&constantFunctions[CF__CLAMP__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__CLAMP__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			intern_clamp,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)clamp__const_vec3__const_vec3__const_vec3__const_vec3,
			clamp__const_vec3__const_vec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__CLAMP__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__CLAMP__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			intern_clamp,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)clamp__const_vec4__const_vec4__const_vec4__const_vec4,
			clamp__const_vec4__const_vec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__CLAMP__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__CLAMP__CONST_VEC2__CONST_VEC2__CONST_FLOAT__CONST_FLOAT],
			intern_clamp,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)clamp__const_vec2__const_vec2__const_float__const_float,
			clamp__const_vec2__const_vec2__const_float__const_float__body,
			&constantFunctions[CF__CLAMP__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__CLAMP__CONST_VEC3__CONST_VEC3__CONST_FLOAT__CONST_FLOAT],
			intern_clamp,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)clamp__const_vec3__const_vec3__const_float__const_float,
			clamp__const_vec3__const_vec3__const_float__const_float__body,
			&constantFunctions[CF__CLAMP__CONST_VEC2__CONST_VEC2__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__CLAMP__CONST_VEC4__CONST_VEC4__CONST_FLOAT__CONST_FLOAT],
			intern_clamp,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)clamp__const_vec4__const_vec4__const_float__const_float,
			clamp__const_vec4__const_vec4__const_float__const_float__body,
			&constantFunctions[CF__CLAMP__CONST_VEC3__CONST_VEC3__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MATRIXCOMPMULT__CONST_MAT2__CONST_MAT2__CONST_MAT2],
			intern_matrixCompMult,
			&constantFunctionSignatures[CF_SIG__CONST_MAT2__CONST_MAT2__CONST_MAT2],
			(ConstantFunction)matrixCompMult__const_mat2__const_mat2__const_mat2,
			matrixCompMult__const_mat2__const_mat2__const_mat2__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MATRIXCOMPMULT__CONST_MAT3__CONST_MAT3__CONST_MAT3],
			intern_matrixCompMult,
			&constantFunctionSignatures[CF_SIG__CONST_MAT3__CONST_MAT3__CONST_MAT3],
			(ConstantFunction)matrixCompMult__const_mat3__const_mat3__const_mat3,
			matrixCompMult__const_mat3__const_mat3__const_mat3__body,
			&constantFunctions[CF__MATRIXCOMPMULT__CONST_MAT2__CONST_MAT2__CONST_MAT2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MATRIXCOMPMULT__CONST_MAT4__CONST_MAT4__CONST_MAT4],
			intern_matrixCompMult,
			&constantFunctionSignatures[CF_SIG__CONST_MAT4__CONST_MAT4__CONST_MAT4],
			(ConstantFunction)matrixCompMult__const_mat4__const_mat4__const_mat4,
			matrixCompMult__const_mat4__const_mat4__const_mat4__body,
			&constantFunctions[CF__MATRIXCOMPMULT__CONST_MAT3__CONST_MAT3__CONST_MAT3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__POW__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			intern_pow,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)pow__const_float__const_float__const_float,
			pow__const_float__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__POW__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			intern_pow,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)pow__const_vec2__const_vec2__const_vec2,
			pow__const_vec2__const_vec2__const_vec2__body,
			&constantFunctions[CF__POW__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__POW__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			intern_pow,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)pow__const_vec3__const_vec3__const_vec3,
			pow__const_vec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__POW__CONST_VEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__POW__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			intern_pow,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)pow__const_vec4__const_vec4__const_vec4,
			pow__const_vec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__POW__CONST_VEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__GREATERTHANEQUAL__CONST_BVEC2__CONST_VEC2__CONST_VEC2],
			intern_greaterThanEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)greaterThanEqual__const_bvec2__const_vec2__const_vec2,
			greaterThanEqual__const_bvec2__const_vec2__const_vec2__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__GREATERTHANEQUAL__CONST_BVEC3__CONST_VEC3__CONST_VEC3],
			intern_greaterThanEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)greaterThanEqual__const_bvec3__const_vec3__const_vec3,
			greaterThanEqual__const_bvec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__GREATERTHANEQUAL__CONST_BVEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__GREATERTHANEQUAL__CONST_BVEC4__CONST_VEC4__CONST_VEC4],
			intern_greaterThanEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)greaterThanEqual__const_bvec4__const_vec4__const_vec4,
			greaterThanEqual__const_bvec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__GREATERTHANEQUAL__CONST_BVEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__GREATERTHANEQUAL__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2],
			intern_greaterThanEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2],
			(ConstantFunction)greaterThanEqual__const_bvec2__const_ivec2__const_ivec2,
			greaterThanEqual__const_bvec2__const_ivec2__const_ivec2__body,
			&constantFunctions[CF__GREATERTHANEQUAL__CONST_BVEC4__CONST_VEC4__CONST_VEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__GREATERTHANEQUAL__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3],
			intern_greaterThanEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3],
			(ConstantFunction)greaterThanEqual__const_bvec3__const_ivec3__const_ivec3,
			greaterThanEqual__const_bvec3__const_ivec3__const_ivec3__body,
			&constantFunctions[CF__GREATERTHANEQUAL__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__GREATERTHANEQUAL__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4],
			intern_greaterThanEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4],
			(ConstantFunction)greaterThanEqual__const_bvec4__const_ivec4__const_ivec4,
			greaterThanEqual__const_bvec4__const_ivec4__const_ivec4__body,
			&constantFunctions[CF__GREATERTHANEQUAL__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SIGN__CONST_FLOAT__CONST_FLOAT],
			intern_sign,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)sign__const_float__const_float,
			sign__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SIGN__CONST_VEC2__CONST_VEC2],
			intern_sign,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)sign__const_vec2__const_vec2,
			sign__const_vec2__const_vec2__body,
			&constantFunctions[CF__SIGN__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SIGN__CONST_VEC3__CONST_VEC3],
			intern_sign,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)sign__const_vec3__const_vec3,
			sign__const_vec3__const_vec3__body,
			&constantFunctions[CF__SIGN__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SIGN__CONST_VEC4__CONST_VEC4],
			intern_sign,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)sign__const_vec4__const_vec4,
			sign__const_vec4__const_vec4__body,
			&constantFunctions[CF__SIGN__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EQUAL__CONST_BVEC2__CONST_VEC2__CONST_VEC2],
			intern_equal,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)equal__const_bvec2__const_vec2__const_vec2,
			equal__const_bvec2__const_vec2__const_vec2__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EQUAL__CONST_BVEC3__CONST_VEC3__CONST_VEC3],
			intern_equal,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)equal__const_bvec3__const_vec3__const_vec3,
			equal__const_bvec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__EQUAL__CONST_BVEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EQUAL__CONST_BVEC4__CONST_VEC4__CONST_VEC4],
			intern_equal,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)equal__const_bvec4__const_vec4__const_vec4,
			equal__const_bvec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__EQUAL__CONST_BVEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EQUAL__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2],
			intern_equal,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2],
			(ConstantFunction)equal__const_bvec2__const_ivec2__const_ivec2,
			equal__const_bvec2__const_ivec2__const_ivec2__body,
			&constantFunctions[CF__EQUAL__CONST_BVEC4__CONST_VEC4__CONST_VEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EQUAL__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3],
			intern_equal,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3],
			(ConstantFunction)equal__const_bvec3__const_ivec3__const_ivec3,
			equal__const_bvec3__const_ivec3__const_ivec3__body,
			&constantFunctions[CF__EQUAL__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EQUAL__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4],
			intern_equal,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4],
			(ConstantFunction)equal__const_bvec4__const_ivec4__const_ivec4,
			equal__const_bvec4__const_ivec4__const_ivec4__body,
			&constantFunctions[CF__EQUAL__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EQUAL__CONST_BVEC2__CONST_BVEC2__CONST_BVEC2],
			intern_equal,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_BVEC2__CONST_BVEC2],
			(ConstantFunction)equal__const_bvec2__const_bvec2__const_bvec2,
			equal__const_bvec2__const_bvec2__const_bvec2__body,
			&constantFunctions[CF__EQUAL__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EQUAL__CONST_BVEC3__CONST_BVEC3__CONST_BVEC3],
			intern_equal,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_BVEC3__CONST_BVEC3],
			(ConstantFunction)equal__const_bvec3__const_bvec3__const_bvec3,
			equal__const_bvec3__const_bvec3__const_bvec3__body,
			&constantFunctions[CF__EQUAL__CONST_BVEC2__CONST_BVEC2__CONST_BVEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EQUAL__CONST_BVEC4__CONST_BVEC4__CONST_BVEC4],
			intern_equal,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_BVEC4__CONST_BVEC4],
			(ConstantFunction)equal__const_bvec4__const_bvec4__const_bvec4,
			equal__const_bvec4__const_bvec4__const_bvec4__body,
			&constantFunctions[CF__EQUAL__CONST_BVEC3__CONST_BVEC3__CONST_BVEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__FACEFORWARD__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			intern_faceforward,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)faceforward__const_float__const_float__const_float__const_float,
			faceforward__const_float__const_float__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__FACEFORWARD__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			intern_faceforward,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)faceforward__const_vec2__const_vec2__const_vec2__const_vec2,
			faceforward__const_vec2__const_vec2__const_vec2__const_vec2__body,
			&constantFunctions[CF__FACEFORWARD__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__FACEFORWARD__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			intern_faceforward,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)faceforward__const_vec3__const_vec3__const_vec3__const_vec3,
			faceforward__const_vec3__const_vec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__FACEFORWARD__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__FACEFORWARD__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			intern_faceforward,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)faceforward__const_vec4__const_vec4__const_vec4__const_vec4,
			faceforward__const_vec4__const_vec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__FACEFORWARD__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__TAN__CONST_FLOAT__CONST_FLOAT],
			intern_tan,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)tan__const_float__const_float,
			tan__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__TAN__CONST_VEC2__CONST_VEC2],
			intern_tan,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)tan__const_vec2__const_vec2,
			tan__const_vec2__const_vec2__body,
			&constantFunctions[CF__TAN__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__TAN__CONST_VEC3__CONST_VEC3],
			intern_tan,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)tan__const_vec3__const_vec3,
			tan__const_vec3__const_vec3__body,
			&constantFunctions[CF__TAN__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__TAN__CONST_VEC4__CONST_VEC4],
			intern_tan,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)tan__const_vec4__const_vec4,
			tan__const_vec4__const_vec4__body,
			&constantFunctions[CF__TAN__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ANY__CONST_BOOL__CONST_BVEC2],
			intern_any,
			&constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC2],
			(ConstantFunction)any__const_bool__const_bvec2,
			any__const_bool__const_bvec2__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ANY__CONST_BOOL__CONST_BVEC3],
			intern_any,
			&constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC3],
			(ConstantFunction)any__const_bool__const_bvec3,
			any__const_bool__const_bvec3__body,
			&constantFunctions[CF__ANY__CONST_BOOL__CONST_BVEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ANY__CONST_BOOL__CONST_BVEC4],
			intern_any,
			&constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC4],
			(ConstantFunction)any__const_bool__const_bvec4,
			any__const_bool__const_bvec4__body,
			&constantFunctions[CF__ANY__CONST_BOOL__CONST_BVEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NORMALIZE__CONST_FLOAT__CONST_FLOAT],
			intern_normalize,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)normalize__const_float__const_float,
			normalize__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NORMALIZE__CONST_VEC2__CONST_VEC2],
			intern_normalize,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)normalize__const_vec2__const_vec2,
			normalize__const_vec2__const_vec2__body,
			&constantFunctions[CF__NORMALIZE__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NORMALIZE__CONST_VEC3__CONST_VEC3],
			intern_normalize,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)normalize__const_vec3__const_vec3,
			normalize__const_vec3__const_vec3__body,
			&constantFunctions[CF__NORMALIZE__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NORMALIZE__CONST_VEC4__CONST_VEC4],
			intern_normalize,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)normalize__const_vec4__const_vec4,
			normalize__const_vec4__const_vec4__body,
			&constantFunctions[CF__NORMALIZE__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ASIN__CONST_FLOAT__CONST_FLOAT],
			intern_asin,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)asin__const_float__const_float,
			asin__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ASIN__CONST_VEC2__CONST_VEC2],
			intern_asin,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)asin__const_vec2__const_vec2,
			asin__const_vec2__const_vec2__body,
			&constantFunctions[CF__ASIN__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ASIN__CONST_VEC3__CONST_VEC3],
			intern_asin,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)asin__const_vec3__const_vec3,
			asin__const_vec3__const_vec3__body,
			&constantFunctions[CF__ASIN__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ASIN__CONST_VEC4__CONST_VEC4],
			intern_asin,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)asin__const_vec4__const_vec4,
			asin__const_vec4__const_vec4__body,
			&constantFunctions[CF__ASIN__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LOG__CONST_FLOAT__CONST_FLOAT],
			intern_log,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)log__const_float__const_float,
			log__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LOG__CONST_VEC2__CONST_VEC2],
			intern_log,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)log__const_vec2__const_vec2,
			log__const_vec2__const_vec2__body,
			&constantFunctions[CF__LOG__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LOG__CONST_VEC3__CONST_VEC3],
			intern_log,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)log__const_vec3__const_vec3,
			log__const_vec3__const_vec3__body,
			&constantFunctions[CF__LOG__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LOG__CONST_VEC4__CONST_VEC4],
			intern_log,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)log__const_vec4__const_vec4,
			log__const_vec4__const_vec4__body,
			&constantFunctions[CF__LOG__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__FLOOR__CONST_FLOAT__CONST_FLOAT],
			intern_floor,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)floor__const_float__const_float,
			floor__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__FLOOR__CONST_VEC2__CONST_VEC2],
			intern_floor,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)floor__const_vec2__const_vec2,
			floor__const_vec2__const_vec2__body,
			&constantFunctions[CF__FLOOR__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__FLOOR__CONST_VEC3__CONST_VEC3],
			intern_floor,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)floor__const_vec3__const_vec3,
			floor__const_vec3__const_vec3__body,
			&constantFunctions[CF__FLOOR__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__FLOOR__CONST_VEC4__CONST_VEC4],
			intern_floor,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)floor__const_vec4__const_vec4,
			floor__const_vec4__const_vec4__body,
			&constantFunctions[CF__FLOOR__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EXP2__CONST_FLOAT__CONST_FLOAT],
			intern_exp2,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)exp2__const_float__const_float,
			exp2__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EXP2__CONST_VEC2__CONST_VEC2],
			intern_exp2,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)exp2__const_vec2__const_vec2,
			exp2__const_vec2__const_vec2__body,
			&constantFunctions[CF__EXP2__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EXP2__CONST_VEC3__CONST_VEC3],
			intern_exp2,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)exp2__const_vec3__const_vec3,
			exp2__const_vec3__const_vec3__body,
			&constantFunctions[CF__EXP2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EXP2__CONST_VEC4__CONST_VEC4],
			intern_exp2,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)exp2__const_vec4__const_vec4,
			exp2__const_vec4__const_vec4__body,
			&constantFunctions[CF__EXP2__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LESSTHAN__CONST_BVEC2__CONST_VEC2__CONST_VEC2],
			intern_lessThan,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)lessThan__const_bvec2__const_vec2__const_vec2,
			lessThan__const_bvec2__const_vec2__const_vec2__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LESSTHAN__CONST_BVEC3__CONST_VEC3__CONST_VEC3],
			intern_lessThan,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)lessThan__const_bvec3__const_vec3__const_vec3,
			lessThan__const_bvec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__LESSTHAN__CONST_BVEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LESSTHAN__CONST_BVEC4__CONST_VEC4__CONST_VEC4],
			intern_lessThan,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)lessThan__const_bvec4__const_vec4__const_vec4,
			lessThan__const_bvec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__LESSTHAN__CONST_BVEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LESSTHAN__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2],
			intern_lessThan,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2],
			(ConstantFunction)lessThan__const_bvec2__const_ivec2__const_ivec2,
			lessThan__const_bvec2__const_ivec2__const_ivec2__body,
			&constantFunctions[CF__LESSTHAN__CONST_BVEC4__CONST_VEC4__CONST_VEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LESSTHAN__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3],
			intern_lessThan,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3],
			(ConstantFunction)lessThan__const_bvec3__const_ivec3__const_ivec3,
			lessThan__const_bvec3__const_ivec3__const_ivec3__body,
			&constantFunctions[CF__LESSTHAN__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LESSTHAN__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4],
			intern_lessThan,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4],
			(ConstantFunction)lessThan__const_bvec4__const_ivec4__const_ivec4,
			lessThan__const_bvec4__const_ivec4__const_ivec4__body,
			&constantFunctions[CF__LESSTHAN__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__CROSS__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			intern_cross,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)cross__const_vec3__const_vec3__const_vec3,
			cross__const_vec3__const_vec3__const_vec3__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SQRT__CONST_FLOAT__CONST_FLOAT],
			intern_sqrt,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)sqrt__const_float__const_float,
			sqrt__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SQRT__CONST_VEC2__CONST_VEC2],
			intern_sqrt,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)sqrt__const_vec2__const_vec2,
			sqrt__const_vec2__const_vec2__body,
			&constantFunctions[CF__SQRT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SQRT__CONST_VEC3__CONST_VEC3],
			intern_sqrt,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)sqrt__const_vec3__const_vec3,
			sqrt__const_vec3__const_vec3__body,
			&constantFunctions[CF__SQRT__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SQRT__CONST_VEC4__CONST_VEC4],
			intern_sqrt,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)sqrt__const_vec4__const_vec4,
			sqrt__const_vec4__const_vec4__body,
			&constantFunctions[CF__SQRT__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__FRACT__CONST_FLOAT__CONST_FLOAT],
			intern_fract,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)fract__const_float__const_float,
			fract__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__FRACT__CONST_VEC2__CONST_VEC2],
			intern_fract,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)fract__const_vec2__const_vec2,
			fract__const_vec2__const_vec2__body,
			&constantFunctions[CF__FRACT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__FRACT__CONST_VEC3__CONST_VEC3],
			intern_fract,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)fract__const_vec3__const_vec3,
			fract__const_vec3__const_vec3__body,
			&constantFunctions[CF__FRACT__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__FRACT__CONST_VEC4__CONST_VEC4],
			intern_fract,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)fract__const_vec4__const_vec4,
			fract__const_vec4__const_vec4__body,
			&constantFunctions[CF__FRACT__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ABS__CONST_FLOAT__CONST_FLOAT],
			intern_abs,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)abs__const_float__const_float,
			abs__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ABS__CONST_VEC2__CONST_VEC2],
			intern_abs,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)abs__const_vec2__const_vec2,
			abs__const_vec2__const_vec2__body,
			&constantFunctions[CF__ABS__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ABS__CONST_VEC3__CONST_VEC3],
			intern_abs,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)abs__const_vec3__const_vec3,
			abs__const_vec3__const_vec3__body,
			&constantFunctions[CF__ABS__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ABS__CONST_VEC4__CONST_VEC4],
			intern_abs,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)abs__const_vec4__const_vec4,
			abs__const_vec4__const_vec4__body,
			&constantFunctions[CF__ABS__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__DEGREES__CONST_FLOAT__CONST_FLOAT],
			intern_degrees,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)degrees__const_float__const_float,
			degrees__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__DEGREES__CONST_VEC2__CONST_VEC2],
			intern_degrees,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)degrees__const_vec2__const_vec2,
			degrees__const_vec2__const_vec2__body,
			&constantFunctions[CF__DEGREES__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__DEGREES__CONST_VEC3__CONST_VEC3],
			intern_degrees,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)degrees__const_vec3__const_vec3,
			degrees__const_vec3__const_vec3__body,
			&constantFunctions[CF__DEGREES__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__DEGREES__CONST_VEC4__CONST_VEC4],
			intern_degrees,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)degrees__const_vec4__const_vec4,
			degrees__const_vec4__const_vec4__body,
			&constantFunctions[CF__DEGREES__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SIN__CONST_FLOAT__CONST_FLOAT],
			intern_sin,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)sin__const_float__const_float,
			sin__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SIN__CONST_VEC2__CONST_VEC2],
			intern_sin,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)sin__const_vec2__const_vec2,
			sin__const_vec2__const_vec2__body,
			&constantFunctions[CF__SIN__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SIN__CONST_VEC3__CONST_VEC3],
			intern_sin,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)sin__const_vec3__const_vec3,
			sin__const_vec3__const_vec3__body,
			&constantFunctions[CF__SIN__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SIN__CONST_VEC4__CONST_VEC4],
			intern_sin,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)sin__const_vec4__const_vec4,
			sin__const_vec4__const_vec4__body,
			&constantFunctions[CF__SIN__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ALL__CONST_BOOL__CONST_BVEC2],
			intern_all,
			&constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC2],
			(ConstantFunction)all__const_bool__const_bvec2,
			all__const_bool__const_bvec2__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ALL__CONST_BOOL__CONST_BVEC3],
			intern_all,
			&constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC3],
			(ConstantFunction)all__const_bool__const_bvec3,
			all__const_bool__const_bvec3__body,
			&constantFunctions[CF__ALL__CONST_BOOL__CONST_BVEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ALL__CONST_BOOL__CONST_BVEC4],
			intern_all,
			&constantFunctionSignatures[CF_SIG__CONST_BOOL__CONST_BVEC4],
			(ConstantFunction)all__const_bool__const_bvec4,
			all__const_bool__const_bvec4__body,
			&constantFunctions[CF__ALL__CONST_BOOL__CONST_BVEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LOG2__CONST_FLOAT__CONST_FLOAT],
			intern_log2,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)log2__const_float__const_float,
			log2__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LOG2__CONST_VEC2__CONST_VEC2],
			intern_log2,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)log2__const_vec2__const_vec2,
			log2__const_vec2__const_vec2__body,
			&constantFunctions[CF__LOG2__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LOG2__CONST_VEC3__CONST_VEC3],
			intern_log2,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)log2__const_vec3__const_vec3,
			log2__const_vec3__const_vec3__body,
			&constantFunctions[CF__LOG2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LOG2__CONST_VEC4__CONST_VEC4],
			intern_log2,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)log2__const_vec4__const_vec4,
			log2__const_vec4__const_vec4__body,
			&constantFunctions[CF__LOG2__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ATAN__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			intern_atan,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)atan__const_float__const_float__const_float,
			atan__const_float__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ATAN__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			intern_atan,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)atan__const_vec2__const_vec2__const_vec2,
			atan__const_vec2__const_vec2__const_vec2__body,
			&constantFunctions[CF__ATAN__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ATAN__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			intern_atan,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)atan__const_vec3__const_vec3__const_vec3,
			atan__const_vec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__ATAN__CONST_VEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ATAN__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			intern_atan,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)atan__const_vec4__const_vec4__const_vec4,
			atan__const_vec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__ATAN__CONST_VEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ATAN__CONST_FLOAT__CONST_FLOAT],
			intern_atan,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)atan__const_float__const_float,
			atan__const_float__const_float__body,
			&constantFunctions[CF__ATAN__CONST_VEC4__CONST_VEC4__CONST_VEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ATAN__CONST_VEC2__CONST_VEC2],
			intern_atan,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)atan__const_vec2__const_vec2,
			atan__const_vec2__const_vec2__body,
			&constantFunctions[CF__ATAN__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ATAN__CONST_VEC3__CONST_VEC3],
			intern_atan,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)atan__const_vec3__const_vec3,
			atan__const_vec3__const_vec3__body,
			&constantFunctions[CF__ATAN__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ATAN__CONST_VEC4__CONST_VEC4],
			intern_atan,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)atan__const_vec4__const_vec4,
			atan__const_vec4__const_vec4__body,
			&constantFunctions[CF__ATAN__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC2__CONST_VEC2__CONST_VEC2],
			intern_notEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)notEqual__const_bvec2__const_vec2__const_vec2,
			notEqual__const_bvec2__const_vec2__const_vec2__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC3__CONST_VEC3__CONST_VEC3],
			intern_notEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)notEqual__const_bvec3__const_vec3__const_vec3,
			notEqual__const_bvec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC4__CONST_VEC4__CONST_VEC4],
			intern_notEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)notEqual__const_bvec4__const_vec4__const_vec4,
			notEqual__const_bvec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2],
			intern_notEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2],
			(ConstantFunction)notEqual__const_bvec2__const_ivec2__const_ivec2,
			notEqual__const_bvec2__const_ivec2__const_ivec2__body,
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC4__CONST_VEC4__CONST_VEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3],
			intern_notEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3],
			(ConstantFunction)notEqual__const_bvec3__const_ivec3__const_ivec3,
			notEqual__const_bvec3__const_ivec3__const_ivec3__body,
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4],
			intern_notEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4],
			(ConstantFunction)notEqual__const_bvec4__const_ivec4__const_ivec4,
			notEqual__const_bvec4__const_ivec4__const_ivec4__body,
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC2__CONST_BVEC2__CONST_BVEC2],
			intern_notEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_BVEC2__CONST_BVEC2],
			(ConstantFunction)notEqual__const_bvec2__const_bvec2__const_bvec2,
			notEqual__const_bvec2__const_bvec2__const_bvec2__body,
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC3__CONST_BVEC3__CONST_BVEC3],
			intern_notEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_BVEC3__CONST_BVEC3],
			(ConstantFunction)notEqual__const_bvec3__const_bvec3__const_bvec3,
			notEqual__const_bvec3__const_bvec3__const_bvec3__body,
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC2__CONST_BVEC2__CONST_BVEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC4__CONST_BVEC4__CONST_BVEC4],
			intern_notEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_BVEC4__CONST_BVEC4],
			(ConstantFunction)notEqual__const_bvec4__const_bvec4__const_bvec4,
			notEqual__const_bvec4__const_bvec4__const_bvec4__body,
			&constantFunctions[CF__NOTEQUAL__CONST_BVEC3__CONST_BVEC3__CONST_BVEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MAX__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			intern_max,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)max__const_float__const_float__const_float,
			max__const_float__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MAX__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			intern_max,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)max__const_vec2__const_vec2__const_vec2,
			max__const_vec2__const_vec2__const_vec2__body,
			&constantFunctions[CF__MAX__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MAX__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			intern_max,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)max__const_vec3__const_vec3__const_vec3,
			max__const_vec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__MAX__CONST_VEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MAX__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			intern_max,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)max__const_vec4__const_vec4__const_vec4,
			max__const_vec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__MAX__CONST_VEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MAX__CONST_VEC2__CONST_VEC2__CONST_FLOAT],
			intern_max,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT],
			(ConstantFunction)max__const_vec2__const_vec2__const_float,
			max__const_vec2__const_vec2__const_float__body,
			&constantFunctions[CF__MAX__CONST_VEC4__CONST_VEC4__CONST_VEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MAX__CONST_VEC3__CONST_VEC3__CONST_FLOAT],
			intern_max,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT],
			(ConstantFunction)max__const_vec3__const_vec3__const_float,
			max__const_vec3__const_vec3__const_float__body,
			&constantFunctions[CF__MAX__CONST_VEC2__CONST_VEC2__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MAX__CONST_VEC4__CONST_VEC4__CONST_FLOAT],
			intern_max,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT],
			(ConstantFunction)max__const_vec4__const_vec4__const_float,
			max__const_vec4__const_vec4__const_float__body,
			&constantFunctions[CF__MAX__CONST_VEC3__CONST_VEC3__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LESSTHANEQUAL__CONST_BVEC2__CONST_VEC2__CONST_VEC2],
			intern_lessThanEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)lessThanEqual__const_bvec2__const_vec2__const_vec2,
			lessThanEqual__const_bvec2__const_vec2__const_vec2__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LESSTHANEQUAL__CONST_BVEC3__CONST_VEC3__CONST_VEC3],
			intern_lessThanEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)lessThanEqual__const_bvec3__const_vec3__const_vec3,
			lessThanEqual__const_bvec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__LESSTHANEQUAL__CONST_BVEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LESSTHANEQUAL__CONST_BVEC4__CONST_VEC4__CONST_VEC4],
			intern_lessThanEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)lessThanEqual__const_bvec4__const_vec4__const_vec4,
			lessThanEqual__const_bvec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__LESSTHANEQUAL__CONST_BVEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LESSTHANEQUAL__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2],
			intern_lessThanEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2],
			(ConstantFunction)lessThanEqual__const_bvec2__const_ivec2__const_ivec2,
			lessThanEqual__const_bvec2__const_ivec2__const_ivec2__body,
			&constantFunctions[CF__LESSTHANEQUAL__CONST_BVEC4__CONST_VEC4__CONST_VEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LESSTHANEQUAL__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3],
			intern_lessThanEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3],
			(ConstantFunction)lessThanEqual__const_bvec3__const_ivec3__const_ivec3,
			lessThanEqual__const_bvec3__const_ivec3__const_ivec3__body,
			&constantFunctions[CF__LESSTHANEQUAL__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LESSTHANEQUAL__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4],
			intern_lessThanEqual,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4],
			(ConstantFunction)lessThanEqual__const_bvec4__const_ivec4__const_ivec4,
			lessThanEqual__const_bvec4__const_ivec4__const_ivec4__body,
			&constantFunctions[CF__LESSTHANEQUAL__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__CEIL__CONST_FLOAT__CONST_FLOAT],
			intern_ceil,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)ceil__const_float__const_float,
			ceil__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__CEIL__CONST_VEC2__CONST_VEC2],
			intern_ceil,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)ceil__const_vec2__const_vec2,
			ceil__const_vec2__const_vec2__body,
			&constantFunctions[CF__CEIL__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__CEIL__CONST_VEC3__CONST_VEC3],
			intern_ceil,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)ceil__const_vec3__const_vec3,
			ceil__const_vec3__const_vec3__body,
			&constantFunctions[CF__CEIL__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__CEIL__CONST_VEC4__CONST_VEC4],
			intern_ceil,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)ceil__const_vec4__const_vec4,
			ceil__const_vec4__const_vec4__body,
			&constantFunctions[CF__CEIL__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__REFLECT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			intern_reflect,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)reflect__const_float__const_float__const_float,
			reflect__const_float__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__REFLECT__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			intern_reflect,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)reflect__const_vec2__const_vec2__const_vec2,
			reflect__const_vec2__const_vec2__const_vec2__body,
			&constantFunctions[CF__REFLECT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__REFLECT__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			intern_reflect,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)reflect__const_vec3__const_vec3__const_vec3,
			reflect__const_vec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__REFLECT__CONST_VEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__REFLECT__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			intern_reflect,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)reflect__const_vec4__const_vec4__const_vec4,
			reflect__const_vec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__REFLECT__CONST_VEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__STEP__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			intern_step,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)step__const_float__const_float__const_float,
			step__const_float__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__STEP__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			intern_step,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)step__const_vec2__const_vec2__const_vec2,
			step__const_vec2__const_vec2__const_vec2__body,
			&constantFunctions[CF__STEP__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__STEP__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			intern_step,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)step__const_vec3__const_vec3__const_vec3,
			step__const_vec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__STEP__CONST_VEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__STEP__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			intern_step,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)step__const_vec4__const_vec4__const_vec4,
			step__const_vec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__STEP__CONST_VEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__STEP__CONST_VEC2__CONST_FLOAT__CONST_VEC2],
			intern_step,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_VEC2],
			(ConstantFunction)step__const_vec2__const_float__const_vec2,
			step__const_vec2__const_float__const_vec2__body,
			&constantFunctions[CF__STEP__CONST_VEC4__CONST_VEC4__CONST_VEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__STEP__CONST_VEC3__CONST_FLOAT__CONST_VEC3],
			intern_step,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_VEC3],
			(ConstantFunction)step__const_vec3__const_float__const_vec3,
			step__const_vec3__const_float__const_vec3__body,
			&constantFunctions[CF__STEP__CONST_VEC2__CONST_FLOAT__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__STEP__CONST_VEC4__CONST_FLOAT__CONST_VEC4],
			intern_step,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_VEC4],
			(ConstantFunction)step__const_vec4__const_float__const_vec4,
			step__const_vec4__const_float__const_vec4__body,
			&constantFunctions[CF__STEP__CONST_VEC3__CONST_FLOAT__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__GREATERTHAN__CONST_BVEC2__CONST_VEC2__CONST_VEC2],
			intern_greaterThan,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)greaterThan__const_bvec2__const_vec2__const_vec2,
			greaterThan__const_bvec2__const_vec2__const_vec2__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__GREATERTHAN__CONST_BVEC3__CONST_VEC3__CONST_VEC3],
			intern_greaterThan,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)greaterThan__const_bvec3__const_vec3__const_vec3,
			greaterThan__const_bvec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__GREATERTHAN__CONST_BVEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__GREATERTHAN__CONST_BVEC4__CONST_VEC4__CONST_VEC4],
			intern_greaterThan,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)greaterThan__const_bvec4__const_vec4__const_vec4,
			greaterThan__const_bvec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__GREATERTHAN__CONST_BVEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__GREATERTHAN__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2],
			intern_greaterThan,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2],
			(ConstantFunction)greaterThan__const_bvec2__const_ivec2__const_ivec2,
			greaterThan__const_bvec2__const_ivec2__const_ivec2__body,
			&constantFunctions[CF__GREATERTHAN__CONST_BVEC4__CONST_VEC4__CONST_VEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__GREATERTHAN__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3],
			intern_greaterThan,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3],
			(ConstantFunction)greaterThan__const_bvec3__const_ivec3__const_ivec3,
			greaterThan__const_bvec3__const_ivec3__const_ivec3__body,
			&constantFunctions[CF__GREATERTHAN__CONST_BVEC2__CONST_IVEC2__CONST_IVEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__GREATERTHAN__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4],
			intern_greaterThan,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_IVEC4__CONST_IVEC4],
			(ConstantFunction)greaterThan__const_bvec4__const_ivec4__const_ivec4,
			greaterThan__const_bvec4__const_ivec4__const_ivec4__body,
			&constantFunctions[CF__GREATERTHAN__CONST_BVEC3__CONST_IVEC3__CONST_IVEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NOT__CONST_BVEC2__CONST_BVEC2],
			intern_not,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC2__CONST_BVEC2],
			(ConstantFunction)not__const_bvec2__const_bvec2,
			not__const_bvec2__const_bvec2__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NOT__CONST_BVEC3__CONST_BVEC3],
			intern_not,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC3__CONST_BVEC3],
			(ConstantFunction)not__const_bvec3__const_bvec3,
			not__const_bvec3__const_bvec3__body,
			&constantFunctions[CF__NOT__CONST_BVEC2__CONST_BVEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__NOT__CONST_BVEC4__CONST_BVEC4],
			intern_not,
			&constantFunctionSignatures[CF_SIG__CONST_BVEC4__CONST_BVEC4],
			(ConstantFunction)not__const_bvec4__const_bvec4,
			not__const_bvec4__const_bvec4__body,
			&constantFunctions[CF__NOT__CONST_BVEC3__CONST_BVEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__INVERSESQRT__CONST_FLOAT__CONST_FLOAT],
			intern_inversesqrt,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)inversesqrt__const_float__const_float,
			inversesqrt__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__INVERSESQRT__CONST_VEC2__CONST_VEC2],
			intern_inversesqrt,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)inversesqrt__const_vec2__const_vec2,
			inversesqrt__const_vec2__const_vec2__body,
			&constantFunctions[CF__INVERSESQRT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__INVERSESQRT__CONST_VEC3__CONST_VEC3],
			intern_inversesqrt,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)inversesqrt__const_vec3__const_vec3,
			inversesqrt__const_vec3__const_vec3__body,
			&constantFunctions[CF__INVERSESQRT__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__INVERSESQRT__CONST_VEC4__CONST_VEC4],
			intern_inversesqrt,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)inversesqrt__const_vec4__const_vec4,
			inversesqrt__const_vec4__const_vec4__body,
			&constantFunctions[CF__INVERSESQRT__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MOD__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			intern_mod,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)mod__const_float__const_float__const_float,
			mod__const_float__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MOD__CONST_VEC2__CONST_VEC2__CONST_FLOAT],
			intern_mod,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT],
			(ConstantFunction)mod__const_vec2__const_vec2__const_float,
			mod__const_vec2__const_vec2__const_float__body,
			&constantFunctions[CF__MOD__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MOD__CONST_VEC3__CONST_VEC3__CONST_FLOAT],
			intern_mod,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT],
			(ConstantFunction)mod__const_vec3__const_vec3__const_float,
			mod__const_vec3__const_vec3__const_float__body,
			&constantFunctions[CF__MOD__CONST_VEC2__CONST_VEC2__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MOD__CONST_VEC4__CONST_VEC4__CONST_FLOAT],
			intern_mod,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT],
			(ConstantFunction)mod__const_vec4__const_vec4__const_float,
			mod__const_vec4__const_vec4__const_float__body,
			&constantFunctions[CF__MOD__CONST_VEC3__CONST_VEC3__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MOD__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			intern_mod,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)mod__const_vec2__const_vec2__const_vec2,
			mod__const_vec2__const_vec2__const_vec2__body,
			&constantFunctions[CF__MOD__CONST_VEC4__CONST_VEC4__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MOD__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			intern_mod,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)mod__const_vec3__const_vec3__const_vec3,
			mod__const_vec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__MOD__CONST_VEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MOD__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			intern_mod,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)mod__const_vec4__const_vec4__const_vec4,
			mod__const_vec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__MOD__CONST_VEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__DISTANCE__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			intern_distance,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)distance__const_float__const_float__const_float,
			distance__const_float__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__DISTANCE__CONST_FLOAT__CONST_VEC2__CONST_VEC2],
			intern_distance,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)distance__const_float__const_vec2__const_vec2,
			distance__const_float__const_vec2__const_vec2__body,
			&constantFunctions[CF__DISTANCE__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__DISTANCE__CONST_FLOAT__CONST_VEC3__CONST_VEC3],
			intern_distance,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)distance__const_float__const_vec3__const_vec3,
			distance__const_float__const_vec3__const_vec3__body,
			&constantFunctions[CF__DISTANCE__CONST_FLOAT__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__DISTANCE__CONST_FLOAT__CONST_VEC4__CONST_VEC4],
			intern_distance,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)distance__const_float__const_vec4__const_vec4,
			distance__const_float__const_vec4__const_vec4__body,
			&constantFunctions[CF__DISTANCE__CONST_FLOAT__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__COS__CONST_FLOAT__CONST_FLOAT],
			intern_cos,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)cos__const_float__const_float,
			cos__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__COS__CONST_VEC2__CONST_VEC2],
			intern_cos,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)cos__const_vec2__const_vec2,
			cos__const_vec2__const_vec2__body,
			&constantFunctions[CF__COS__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__COS__CONST_VEC3__CONST_VEC3],
			intern_cos,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)cos__const_vec3__const_vec3,
			cos__const_vec3__const_vec3__body,
			&constantFunctions[CF__COS__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__COS__CONST_VEC4__CONST_VEC4],
			intern_cos,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)cos__const_vec4__const_vec4,
			cos__const_vec4__const_vec4__body,
			&constantFunctions[CF__COS__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__REFRACT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			intern_refract,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)refract__const_float__const_float__const_float__const_float,
			refract__const_float__const_float__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__REFRACT__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_FLOAT],
			intern_refract,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_FLOAT],
			(ConstantFunction)refract__const_vec2__const_vec2__const_vec2__const_float,
			refract__const_vec2__const_vec2__const_vec2__const_float__body,
			&constantFunctions[CF__REFRACT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__REFRACT__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_FLOAT],
			intern_refract,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_FLOAT],
			(ConstantFunction)refract__const_vec3__const_vec3__const_vec3__const_float,
			refract__const_vec3__const_vec3__const_vec3__const_float__body,
			&constantFunctions[CF__REFRACT__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__REFRACT__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_FLOAT],
			intern_refract,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_FLOAT],
			(ConstantFunction)refract__const_vec4__const_vec4__const_vec4__const_float,
			refract__const_vec4__const_vec4__const_vec4__const_float__body,
			&constantFunctions[CF__REFRACT__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MIN__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			intern_min,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)min__const_float__const_float__const_float,
			min__const_float__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MIN__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			intern_min,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)min__const_vec2__const_vec2__const_vec2,
			min__const_vec2__const_vec2__const_vec2__body,
			&constantFunctions[CF__MIN__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MIN__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			intern_min,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)min__const_vec3__const_vec3__const_vec3,
			min__const_vec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__MIN__CONST_VEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MIN__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			intern_min,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)min__const_vec4__const_vec4__const_vec4,
			min__const_vec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__MIN__CONST_VEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MIN__CONST_VEC2__CONST_VEC2__CONST_FLOAT],
			intern_min,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_FLOAT],
			(ConstantFunction)min__const_vec2__const_vec2__const_float,
			min__const_vec2__const_vec2__const_float__body,
			&constantFunctions[CF__MIN__CONST_VEC4__CONST_VEC4__CONST_VEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MIN__CONST_VEC3__CONST_VEC3__CONST_FLOAT],
			intern_min,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_FLOAT],
			(ConstantFunction)min__const_vec3__const_vec3__const_float,
			min__const_vec3__const_vec3__const_float__body,
			&constantFunctions[CF__MIN__CONST_VEC2__CONST_VEC2__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MIN__CONST_VEC4__CONST_VEC4__CONST_FLOAT],
			intern_min,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_FLOAT],
			(ConstantFunction)min__const_vec4__const_vec4__const_float,
			min__const_vec4__const_vec4__const_float__body,
			&constantFunctions[CF__MIN__CONST_VEC3__CONST_VEC3__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__RADIANS__CONST_FLOAT__CONST_FLOAT],
			intern_radians,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)radians__const_float__const_float,
			radians__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__RADIANS__CONST_VEC2__CONST_VEC2],
			intern_radians,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)radians__const_vec2__const_vec2,
			radians__const_vec2__const_vec2__body,
			&constantFunctions[CF__RADIANS__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__RADIANS__CONST_VEC3__CONST_VEC3],
			intern_radians,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)radians__const_vec3__const_vec3,
			radians__const_vec3__const_vec3__body,
			&constantFunctions[CF__RADIANS__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__RADIANS__CONST_VEC4__CONST_VEC4],
			intern_radians,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)radians__const_vec4__const_vec4,
			radians__const_vec4__const_vec4__body,
			&constantFunctions[CF__RADIANS__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SMOOTHSTEP__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			intern_smoothstep,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)smoothstep__const_float__const_float__const_float__const_float,
			smoothstep__const_float__const_float__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SMOOTHSTEP__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			intern_smoothstep,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)smoothstep__const_vec2__const_vec2__const_vec2__const_vec2,
			smoothstep__const_vec2__const_vec2__const_vec2__const_vec2__body,
			&constantFunctions[CF__SMOOTHSTEP__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SMOOTHSTEP__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			intern_smoothstep,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)smoothstep__const_vec3__const_vec3__const_vec3__const_vec3,
			smoothstep__const_vec3__const_vec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__SMOOTHSTEP__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SMOOTHSTEP__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			intern_smoothstep,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)smoothstep__const_vec4__const_vec4__const_vec4__const_vec4,
			smoothstep__const_vec4__const_vec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__SMOOTHSTEP__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SMOOTHSTEP__CONST_VEC2__CONST_FLOAT__CONST_FLOAT__CONST_VEC2],
			intern_smoothstep,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_FLOAT__CONST_FLOAT__CONST_VEC2],
			(ConstantFunction)smoothstep__const_vec2__const_float__const_float__const_vec2,
			smoothstep__const_vec2__const_float__const_float__const_vec2__body,
			&constantFunctions[CF__SMOOTHSTEP__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SMOOTHSTEP__CONST_VEC3__CONST_FLOAT__CONST_FLOAT__CONST_VEC3],
			intern_smoothstep,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_FLOAT__CONST_FLOAT__CONST_VEC3],
			(ConstantFunction)smoothstep__const_vec3__const_float__const_float__const_vec3,
			smoothstep__const_vec3__const_float__const_float__const_vec3__body,
			&constantFunctions[CF__SMOOTHSTEP__CONST_VEC2__CONST_FLOAT__CONST_FLOAT__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__SMOOTHSTEP__CONST_VEC4__CONST_FLOAT__CONST_FLOAT__CONST_VEC4],
			intern_smoothstep,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_FLOAT__CONST_FLOAT__CONST_VEC4],
			(ConstantFunction)smoothstep__const_vec4__const_float__const_float__const_vec4,
			smoothstep__const_vec4__const_float__const_float__const_vec4__body,
			&constantFunctions[CF__SMOOTHSTEP__CONST_VEC3__CONST_FLOAT__CONST_FLOAT__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LENGTH__CONST_FLOAT__CONST_FLOAT],
			intern_length,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)length__const_float__const_float,
			length__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LENGTH__CONST_FLOAT__CONST_VEC2],
			intern_length,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC2],
			(ConstantFunction)length__const_float__const_vec2,
			length__const_float__const_vec2__body,
			&constantFunctions[CF__LENGTH__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LENGTH__CONST_FLOAT__CONST_VEC3],
			intern_length,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC3],
			(ConstantFunction)length__const_float__const_vec3,
			length__const_float__const_vec3__body,
			&constantFunctions[CF__LENGTH__CONST_FLOAT__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__LENGTH__CONST_FLOAT__CONST_VEC4],
			intern_length,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC4],
			(ConstantFunction)length__const_float__const_vec4,
			length__const_float__const_vec4__body,
			&constantFunctions[CF__LENGTH__CONST_FLOAT__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EXP__CONST_FLOAT__CONST_FLOAT],
			intern_exp,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)exp__const_float__const_float,
			exp__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EXP__CONST_VEC2__CONST_VEC2],
			intern_exp,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)exp__const_vec2__const_vec2,
			exp__const_vec2__const_vec2__body,
			&constantFunctions[CF__EXP__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EXP__CONST_VEC3__CONST_VEC3],
			intern_exp,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)exp__const_vec3__const_vec3,
			exp__const_vec3__const_vec3__body,
			&constantFunctions[CF__EXP__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__EXP__CONST_VEC4__CONST_VEC4],
			intern_exp,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)exp__const_vec4__const_vec4,
			exp__const_vec4__const_vec4__body,
			&constantFunctions[CF__EXP__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ACOS__CONST_FLOAT__CONST_FLOAT],
			intern_acos,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)acos__const_float__const_float,
			acos__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ACOS__CONST_VEC2__CONST_VEC2],
			intern_acos,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)acos__const_vec2__const_vec2,
			acos__const_vec2__const_vec2__body,
			&constantFunctions[CF__ACOS__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ACOS__CONST_VEC3__CONST_VEC3],
			intern_acos,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)acos__const_vec3__const_vec3,
			acos__const_vec3__const_vec3__body,
			&constantFunctions[CF__ACOS__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__ACOS__CONST_VEC4__CONST_VEC4],
			intern_acos,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)acos__const_vec4__const_vec4,
			acos__const_vec4__const_vec4__body,
			&constantFunctions[CF__ACOS__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MIX__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			intern_mix,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)mix__const_float__const_float__const_float__const_float,
			mix__const_float__const_float__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MIX__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			intern_mix,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)mix__const_vec2__const_vec2__const_vec2__const_vec2,
			mix__const_vec2__const_vec2__const_vec2__const_vec2__body,
			&constantFunctions[CF__MIX__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MIX__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			intern_mix,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)mix__const_vec3__const_vec3__const_vec3__const_vec3,
			mix__const_vec3__const_vec3__const_vec3__const_vec3__body,
			&constantFunctions[CF__MIX__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MIX__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			intern_mix,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)mix__const_vec4__const_vec4__const_vec4__const_vec4,
			mix__const_vec4__const_vec4__const_vec4__const_vec4__body,
			&constantFunctions[CF__MIX__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_VEC3]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MIX__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_FLOAT],
			intern_mix,
			&constantFunctionSignatures[CF_SIG__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_FLOAT],
			(ConstantFunction)mix__const_vec2__const_vec2__const_vec2__const_float,
			mix__const_vec2__const_vec2__const_vec2__const_float__body,
			&constantFunctions[CF__MIX__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_VEC4]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MIX__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_FLOAT],
			intern_mix,
			&constantFunctionSignatures[CF_SIG__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_FLOAT],
			(ConstantFunction)mix__const_vec3__const_vec3__const_vec3__const_float,
			mix__const_vec3__const_vec3__const_vec3__const_float__body,
			&constantFunctions[CF__MIX__CONST_VEC2__CONST_VEC2__CONST_VEC2__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__MIX__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_FLOAT],
			intern_mix,
			&constantFunctionSignatures[CF_SIG__CONST_VEC4__CONST_VEC4__CONST_VEC4__CONST_FLOAT],
			(ConstantFunction)mix__const_vec4__const_vec4__const_vec4__const_float,
			mix__const_vec4__const_vec4__const_vec4__const_float__body,
			&constantFunctions[CF__MIX__CONST_VEC3__CONST_VEC3__CONST_VEC3__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__DOT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			intern_dot,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT],
			(ConstantFunction)dot__const_float__const_float__const_float,
			dot__const_float__const_float__const_float__body,
			NULL);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__DOT__CONST_FLOAT__CONST_VEC2__CONST_VEC2],
			intern_dot,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC2__CONST_VEC2],
			(ConstantFunction)dot__const_float__const_vec2__const_vec2,
			dot__const_float__const_vec2__const_vec2__body,
			&constantFunctions[CF__DOT__CONST_FLOAT__CONST_FLOAT__CONST_FLOAT]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__DOT__CONST_FLOAT__CONST_VEC3__CONST_VEC3],
			intern_dot,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC3__CONST_VEC3],
			(ConstantFunction)dot__const_float__const_vec3__const_vec3,
			dot__const_float__const_vec3__const_vec3__body,
			&constantFunctions[CF__DOT__CONST_FLOAT__CONST_VEC2__CONST_VEC2]);

		glsl_symbol_construct_function_instance(
			&constantFunctions[CF__DOT__CONST_FLOAT__CONST_VEC4__CONST_VEC4],
			intern_dot,
			&constantFunctionSignatures[CF_SIG__CONST_FLOAT__CONST_VEC4__CONST_VEC4],
			(ConstantFunction)dot__const_float__const_vec4__const_vec4,
			dot__const_float__const_vec4__const_vec4__body,
			&constantFunctions[CF__DOT__CONST_FLOAT__CONST_VEC3__CONST_VEC3]);
	}
}

void glsl_populate_with_const_functions(SymbolTable* symbol_table)
{
	int i;

	init_const_function_signatures();
	init_const_functions();
	
	for (i = 0; i < CONST_FUNCTION_COUNT; i++)
	{
		glsl_symbol_table_insert(symbol_table, &constantFunctions[i]);
	}
}


//
// Constant function definitions.
//

static void radians__const_float__const_float(const_float* result, const_float* degrees)
{
   /*
      const float pi_on_180 = 0.01745329252;
      return pi_on_180 * degrees;
   */
   
   float pi_on_180 = (float)atof("0.01745329252");

   op_mul__const_float__const_float__const_float(result, (const_float*)&pi_on_180, degrees);
}
static void radians__const_vec2__const_vec2(const_vec2* result, const_vec2* degrees)
{
   int i;
   for (i = 0; i < 2; i++)
   	radians__const_float__const_float(&(*result)[i], &(*degrees)[i]);
}
static void radians__const_vec3__const_vec3(const_vec3* result, const_vec3* degrees)
{
   int i;
   for (i = 0; i < 3; i++)
   	radians__const_float__const_float(&(*result)[i], &(*degrees)[i]);
}
static void radians__const_vec4__const_vec4(const_vec4* result, const_vec4* degrees)
{
   int i;
   for (i = 0; i < 4; i++)
   	radians__const_float__const_float(&(*result)[i], &(*degrees)[i]);
}
static void degrees__const_float__const_float(const_float* result, const_float* radians)
{
   /*
      const float _180_on_pi = 57.29577951;
      return _180_on_pi * radians;
   */ 
   
   float _180_on_pi = (float)atof("57.29577951");

   op_mul__const_float__const_float__const_float(result, (const_float*)&_180_on_pi, radians);
}
static void degrees__const_vec2__const_vec2(const_vec2* result, const_vec2* radians)
{
   int i;
   for (i = 0; i < 2; i++)
   	degrees__const_float__const_float(&(*result)[i], &(*radians)[i]);
}
static void degrees__const_vec3__const_vec3(const_vec3* result, const_vec3* radians)
{
   int i;
   for (i = 0; i < 3; i++)
   	degrees__const_float__const_float(&(*result)[i], &(*radians)[i]);
}
static void degrees__const_vec4__const_vec4(const_vec4* result, const_vec4* radians)
{
   int i;
   for (i = 0; i < 4; i++)
   	degrees__const_float__const_float(&(*result)[i], &(*radians)[i]);
}
static void sin__const_float__const_float(const_float* result, const_float* angle)
{
	uint32_t tempcos;
	glsl_mendenhall_sincospair(*angle, result, &tempcos);
}
static void sin__const_vec2__const_vec2(const_vec2* result, const_vec2* angle)
{
   int i;
   for (i = 0; i < 2; i++)
   	sin__const_float__const_float(&(*result)[i], &(*angle)[i]);
}
static void sin__const_vec3__const_vec3(const_vec3* result, const_vec3* angle)
{
   int i;
   for (i = 0; i < 3; i++)
   	sin__const_float__const_float(&(*result)[i], &(*angle)[i]);
}
static void sin__const_vec4__const_vec4(const_vec4* result, const_vec4* angle)
{
   int i;
   for (i = 0; i < 4; i++)
   	sin__const_float__const_float(&(*result)[i], &(*angle)[i]);
}
static void cos__const_float__const_float(const_float* result, const_float* angle)
{
	uint32_t tempsin;
	glsl_mendenhall_sincospair(*angle, &tempsin, result);
}
static void cos__const_vec2__const_vec2(const_vec2* result, const_vec2* angle)
{
   int i;
   for (i = 0; i < 2; i++)
   	cos__const_float__const_float(&(*result)[i], &(*angle)[i]);
}
static void cos__const_vec3__const_vec3(const_vec3* result, const_vec3* angle)
{
   int i;
   for (i = 0; i < 3; i++)
   	cos__const_float__const_float(&(*result)[i], &(*angle)[i]);
}
static void cos__const_vec4__const_vec4(const_vec4* result, const_vec4* angle)
{
   int i;
   for (i = 0; i < 4; i++)
   	cos__const_float__const_float(&(*result)[i], &(*angle)[i]);
}
static void tan__const_float__const_float(const_float* result, const_float* angle)
{
	const_float tempsin,tempcos,secant;
	glsl_mendenhall_sincospair(*angle, (uint32_t *)&tempsin, (uint32_t *)&tempcos);

   op_recip__const_float__const_float(&secant, &tempcos);
   op_mul__const_float__const_float__const_float(result, &tempsin, &secant);
}
static void tan__const_vec2__const_vec2(const_vec2* result, const_vec2* angle)
{
   int i;
   for (i = 0; i < 2; i++)
   	tan__const_float__const_float(&(*result)[i], &(*angle)[i]);
}
static void tan__const_vec3__const_vec3(const_vec3* result, const_vec3* angle)
{
   int i;
   for (i = 0; i < 3; i++)
   	tan__const_float__const_float(&(*result)[i], &(*angle)[i]);
}
static void tan__const_vec4__const_vec4(const_vec4* result, const_vec4* angle)
{
   int i;
   for (i = 0; i < 4; i++)
   	tan__const_float__const_float(&(*result)[i], &(*angle)[i]);
}
static void asin__const_float__const_float(const_float* result, const_float* x)
{
   const_float one = CONST_FLOAT_ONE;
   const_float t1, t2, t3;

   op_mul__const_float__const_float__const_float(&t1, x, x);
   op_sub__const_float__const_float__const_float(&t2, &one, &t1);
   sqrt__const_float__const_float(&t3, &t2);
   atan__const_float__const_float__const_float(result, x, &t3);
}
static void asin__const_vec2__const_vec2(const_vec2* result, const_vec2* x)
{
   int i;
   for (i = 0; i < 2; i++)
   	asin__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void asin__const_vec3__const_vec3(const_vec3* result, const_vec3* x)
{
   int i;
   for (i = 0; i < 3; i++)
   	asin__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void asin__const_vec4__const_vec4(const_vec4* result, const_vec4* x)
{
   int i;
   for (i = 0; i < 4; i++)
   	asin__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void acos__const_float__const_float(const_float* result, const_float* x)
{
   const_float one = CONST_FLOAT_ONE;
   const_float t1, t2, t3;

   op_mul__const_float__const_float__const_float(&t1, x, x);
   op_sub__const_float__const_float__const_float(&t2, &one, &t1);
   sqrt__const_float__const_float(&t3, &t2);
   atan__const_float__const_float__const_float(result, &t3, x);
}
static void acos__const_vec2__const_vec2(const_vec2* result, const_vec2* x)
{
   int i;
   for (i = 0; i < 2; i++)
   	acos__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void acos__const_vec3__const_vec3(const_vec3* result, const_vec3* x)
{
   int i;
   for (i = 0; i < 3; i++)
   	acos__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void acos__const_vec4__const_vec4(const_vec4* result, const_vec4* x)
{
   int i;
   for (i = 0; i < 4; i++)
   	acos__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void atan__const_float__const_float__const_float(const_float* result, const_float* _y, const_float* _x)
{
   int i;

   float pi_on_4096 = (float)atof("7.6699039e-4");

   const_float x = *_x;
   const_float y = *_y;

   const_float zero = CONST_FLOAT_ZERO;
   const_float one = CONST_FLOAT_ONE;
   const_float two = CONST_FLOAT_TWO;

   const_float t1, t2, t3;
   const_float invhyp, num;

   const_int i1;

   // float invhyp = inversesqrt(x*x + y*y);

   op_mul__const_float__const_float__const_float(&t1, &x, &x);
   op_mul__const_float__const_float__const_float(&t2, &y, &y);
   op_add__const_float__const_float__const_float(&t3, &t1, &t2);
   op_rsqrt__const_float__const_float(&invhyp, &t3);

   op_cmp__const_int__const_float__const_float(&i1, &y, &zero);
   
   if (i1 >= 0) {
      // num = 0.0;

      num = CONST_FLOAT_ZERO;
   } else {
      // num = -1.0;
      // y = - y;
      // x = - x;

      num = CONST_FLOAT_MINUS_ONE;
      op_arith_negate__const_float__const_float(&y, &y);
      op_arith_negate__const_float__const_float(&x, &x);
   }

   // x = x * invhyp;
   // y = y * invhyp;

   op_mul__const_float__const_float__const_float(&x, &x, &invhyp);
   op_mul__const_float__const_float__const_float(&y, &y, &invhyp);

   for (i = 0; i < 12; i++) {
      const_float t5, t6, t7;
      const_float newx, newy;

      const_int i2;

      // float newx = x*x - y*y;
      // float newy = 2.0*y*x;

      op_mul__const_float__const_float__const_float(&t5, &x, &x);
      op_mul__const_float__const_float__const_float(&t6, &y, &y);
      op_sub__const_float__const_float__const_float(&newx, &t5, &t6);

      op_mul__const_float__const_float__const_float(&t7, &two, &y);
      op_mul__const_float__const_float__const_float(&newy, &t7, &x);

      // num = num * 2.0;

      op_mul__const_float__const_float__const_float(&num, &num, &two);

      op_cmp__const_int__const_float__const_float(&i2, &x, &zero);

      if (i2 > 0) {
         // x = newx;
         // y = newy;

         x = newx;
         y = newy;
      } else {
         // num = num + 1.0;
         // x = -newx;
         // y = -newy;

         op_add__const_float__const_float__const_float(&num, &num, &one);
         op_arith_negate__const_float__const_float(&x, &newx);
         op_arith_negate__const_float__const_float(&y, &newy);
      }
   }

   // const float pi_on_4096 = 7.6699039e-4;
   // return num * pi_on_4096;

   op_mul__const_float__const_float__const_float(result, &num, (const_float*)&pi_on_4096);
}
static void atan__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* y, const_vec2* x)
{
   int i;
   for (i = 0; i < 2; i++)
   	atan__const_float__const_float__const_float(&(*result)[i], &(*y)[i], &(*x)[i]);
}
static void atan__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* y, const_vec3* x)
{
   int i;
   for (i = 0; i < 3; i++)
   	atan__const_float__const_float__const_float(&(*result)[i], &(*y)[i], &(*x)[i]);
}
static void atan__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* y, const_vec4* x)
{
   int i;
   for (i = 0; i < 4; i++)
   	atan__const_float__const_float__const_float(&(*result)[i], &(*y)[i], &(*x)[i]);
}
static void atan__const_float__const_float(const_float* result, const_float* y_over_x)
{
   const_float one = CONST_FLOAT_ONE;

   atan__const_float__const_float__const_float(result, y_over_x, &one);
}
static void atan__const_vec2__const_vec2(const_vec2* result, const_vec2* y_over_x)
{
   int i;
   for (i = 0; i < 2; i++)
   	atan__const_float__const_float(&(*result)[i], &(*y_over_x)[i]);
}
static void atan__const_vec3__const_vec3(const_vec3* result, const_vec3* y_over_x)
{
   int i;
   for (i = 0; i < 3; i++)
   	atan__const_float__const_float(&(*result)[i], &(*y_over_x)[i]);
}
static void atan__const_vec4__const_vec4(const_vec4* result, const_vec4* y_over_x)
{
   int i;
   for (i = 0; i < 4; i++)
   	atan__const_float__const_float(&(*result)[i], &(*y_over_x)[i]);
}
static void pow__const_float__const_float__const_float(const_float* result, const_float* x, const_float* y)
{
	unsigned log2x,temp;
	glsl_fpu_log2(&log2x,*x);
	glsl_fpu_mul(&temp,log2x,*y);
	glsl_fpu_exp2((unsigned *)result,temp);
}
static void pow__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* x, const_vec2* y)
{
   int i;
   for (i = 0; i < 2; i++)
   	pow__const_float__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void pow__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* x, const_vec3* y)
{
   int i;
   for (i = 0; i < 3; i++)
   	pow__const_float__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void pow__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* x, const_vec4* y)
{
   int i;
   for (i = 0; i < 4; i++)
   	pow__const_float__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void exp__const_float__const_float(const_float* result, const_float* x)
{
   float e = (float)atof("2.718281828");

   pow__const_float__const_float__const_float(result, (const_float*)&e, x);
}
static void exp__const_vec2__const_vec2(const_vec2* result, const_vec2* x)
{
   int i;
   for (i = 0; i < 2; i++)
   	exp__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void exp__const_vec3__const_vec3(const_vec3* result, const_vec3* x)
{
   int i;
   for (i = 0; i < 3; i++)
   	exp__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void exp__const_vec4__const_vec4(const_vec4* result, const_vec4* x)
{
   int i;
   for (i = 0; i < 4; i++)
   	exp__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void log__const_float__const_float(const_float* result, const_float* x)
{
	float rlog2e=(float)atof("0.6931471805");
   const_float log2x;

   log2__const_float__const_float(&log2x, x);
	glsl_fpu_mul((unsigned *)result,(unsigned)log2x,*(unsigned *)&rlog2e);
}
static void log__const_vec2__const_vec2(const_vec2* result, const_vec2* x)
{
   int i;
   for (i = 0; i < 2; i++)
   	log__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void log__const_vec3__const_vec3(const_vec3* result, const_vec3* x)
{
   int i;
   for (i = 0; i < 3; i++)
   	log__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void log__const_vec4__const_vec4(const_vec4* result, const_vec4* x)
{
   int i;
   for (i = 0; i < 4; i++)
   	log__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void exp2__const_float__const_float(const_float* result, const_float* x)
{
	glsl_fpu_exp2((unsigned *)result,*x);
}
static void exp2__const_vec2__const_vec2(const_vec2* result, const_vec2* x)
{
   int i;
   for (i = 0; i < 2; i++)
   	exp2__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void exp2__const_vec3__const_vec3(const_vec3* result, const_vec3* x)
{
   int i;
   for (i = 0; i < 3; i++)
   	exp2__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void exp2__const_vec4__const_vec4(const_vec4* result, const_vec4* x)
{
   int i;
   for (i = 0; i < 4; i++)
   	exp2__const_float__const_float(&(*result)[i], &(*x)[i]);
}
static void log2__const_float__const_float(const_float* result, const_float* x)
{
	glsl_fpu_log2((unsigned *)result,*x);
}
static void log2__const_vec2__const_vec2(const_vec2* result, const_vec2* x)
{
	log2__const_float__const_float(&(*result)[0], &(*x)[0]);
	log2__const_float__const_float(&(*result)[1], &(*x)[1]);
}
static void log2__const_vec3__const_vec3(const_vec3* result, const_vec3* x)
{
	log2__const_float__const_float(&(*result)[0], &(*x)[0]);
	log2__const_float__const_float(&(*result)[1], &(*x)[1]);
	log2__const_float__const_float(&(*result)[2], &(*x)[2]);
}
static void log2__const_vec4__const_vec4(const_vec4* result, const_vec4* x)
{
	log2__const_float__const_float(&(*result)[0], &(*x)[0]);
	log2__const_float__const_float(&(*result)[1], &(*x)[1]);
	log2__const_float__const_float(&(*result)[2], &(*x)[2]);
	log2__const_float__const_float(&(*result)[3], &(*x)[3]);
}
static void sqrt__const_float__const_float(const_float* result, const_float* x)
{
   const_float t1;

   op_rsqrt__const_float__const_float(&t1, x);
   op_recip__const_float__const_float(result, &t1);
}
static void sqrt__const_vec2__const_vec2(const_vec2* result, const_vec2* x)
{
	sqrt__const_float__const_float(&(*result)[0], &(*x)[0]);
	sqrt__const_float__const_float(&(*result)[1], &(*x)[1]);
}
static void sqrt__const_vec3__const_vec3(const_vec3* result, const_vec3* x)
{
	sqrt__const_float__const_float(&(*result)[0], &(*x)[0]);
	sqrt__const_float__const_float(&(*result)[1], &(*x)[1]);
	sqrt__const_float__const_float(&(*result)[2], &(*x)[2]);
}
static void sqrt__const_vec4__const_vec4(const_vec4* result, const_vec4* x)
{
	sqrt__const_float__const_float(&(*result)[0], &(*x)[0]);
	sqrt__const_float__const_float(&(*result)[1], &(*x)[1]);
	sqrt__const_float__const_float(&(*result)[2], &(*x)[2]);
	sqrt__const_float__const_float(&(*result)[3], &(*x)[3]);
}
static void inversesqrt__const_float__const_float(const_float* result, const_float* x)
{
   op_rsqrt__const_float__const_float(result, x);
}
static void inversesqrt__const_vec2__const_vec2(const_vec2* result, const_vec2* x)
{
	inversesqrt__const_float__const_float(&(*result)[0], &(*x)[0]);
	inversesqrt__const_float__const_float(&(*result)[1], &(*x)[1]);
}
static void inversesqrt__const_vec3__const_vec3(const_vec3* result, const_vec3* x)
{
	inversesqrt__const_float__const_float(&(*result)[0], &(*x)[0]);
	inversesqrt__const_float__const_float(&(*result)[1], &(*x)[1]);
	inversesqrt__const_float__const_float(&(*result)[2], &(*x)[2]);
}
static void inversesqrt__const_vec4__const_vec4(const_vec4* result, const_vec4* x)
{
	inversesqrt__const_float__const_float(&(*result)[0], &(*x)[0]);
	inversesqrt__const_float__const_float(&(*result)[1], &(*x)[1]);
	inversesqrt__const_float__const_float(&(*result)[2], &(*x)[2]);
	inversesqrt__const_float__const_float(&(*result)[3], &(*x)[3]);
}

static void abs__const_float__const_float(const_float* result, const_float* x)
{
   const_float z = CONST_FLOAT_ZERO;
   const_int i;

   op_cmp__const_int__const_float__const_float(&i, x, &z);

   if (i >= 0)
      *result = *x;
   else
      op_arith_negate__const_float__const_float(result, x);
}
static void abs__const_vec2__const_vec2(const_vec2* result, const_vec2* x)
{
	abs__const_float__const_float(&(*result)[0], &(*x)[0]);
	abs__const_float__const_float(&(*result)[1], &(*x)[1]);
}
static void abs__const_vec3__const_vec3(const_vec3* result, const_vec3* x)
{
	abs__const_float__const_float(&(*result)[0], &(*x)[0]);
	abs__const_float__const_float(&(*result)[1], &(*x)[1]);
	abs__const_float__const_float(&(*result)[2], &(*x)[2]);
}
static void abs__const_vec4__const_vec4(const_vec4* result, const_vec4* x)
{
	abs__const_float__const_float(&(*result)[0], &(*x)[0]);
	abs__const_float__const_float(&(*result)[1], &(*x)[1]);
	abs__const_float__const_float(&(*result)[2], &(*x)[2]);
	abs__const_float__const_float(&(*result)[3], &(*x)[3]);
}
static void sign__const_float__const_float(const_float* result, const_float* x)
{
   const_float z = CONST_FLOAT_ZERO;
   const_int i;

   op_cmp__const_int__const_float__const_float(&i, x, &z);
   op_inttofloat__const_float__const_int(result, &i);
}
static void sign__const_vec2__const_vec2(const_vec2* result, const_vec2* x)
{
	sign__const_float__const_float(&(*result)[0], &(*x)[0]);
	sign__const_float__const_float(&(*result)[1], &(*x)[1]);
}
static void sign__const_vec3__const_vec3(const_vec3* result, const_vec3* x)
{
	sign__const_float__const_float(&(*result)[0], &(*x)[0]);
	sign__const_float__const_float(&(*result)[1], &(*x)[1]);
	sign__const_float__const_float(&(*result)[2], &(*x)[2]);
}
static void sign__const_vec4__const_vec4(const_vec4* result, const_vec4* x)
{
	sign__const_float__const_float(&(*result)[0], &(*x)[0]);
	sign__const_float__const_float(&(*result)[1], &(*x)[1]);
	sign__const_float__const_float(&(*result)[2], &(*x)[2]);
	sign__const_float__const_float(&(*result)[3], &(*x)[3]);
}
static void floor__const_float__const_float(const_float* result, const_float* x)
{
   op_floor__const_float__const_float(result, x);
}
static void floor__const_vec2__const_vec2(const_vec2* result, const_vec2* x)
{
	floor__const_float__const_float(&(*result)[0], &(*x)[0]);
	floor__const_float__const_float(&(*result)[1], &(*x)[1]);
}
static void floor__const_vec3__const_vec3(const_vec3* result, const_vec3* x)
{
	floor__const_float__const_float(&(*result)[0], &(*x)[0]);
	floor__const_float__const_float(&(*result)[1], &(*x)[1]);
	floor__const_float__const_float(&(*result)[2], &(*x)[2]);
}
static void floor__const_vec4__const_vec4(const_vec4* result, const_vec4* x)
{
	floor__const_float__const_float(&(*result)[0], &(*x)[0]);
	floor__const_float__const_float(&(*result)[1], &(*x)[1]);
	floor__const_float__const_float(&(*result)[2], &(*x)[2]);
	floor__const_float__const_float(&(*result)[3], &(*x)[3]);
}
static void ceil__const_float__const_float(const_float* result, const_float* x)
{
   op_ceil__const_float__const_float(result, x);
}
static void ceil__const_vec2__const_vec2(const_vec2* result, const_vec2* x)
{
	ceil__const_float__const_float(&(*result)[0], &(*x)[0]);
	ceil__const_float__const_float(&(*result)[1], &(*x)[1]);
}
static void ceil__const_vec3__const_vec3(const_vec3* result, const_vec3* x)
{
	ceil__const_float__const_float(&(*result)[0], &(*x)[0]);
	ceil__const_float__const_float(&(*result)[1], &(*x)[1]);
	ceil__const_float__const_float(&(*result)[2], &(*x)[2]);
}
static void ceil__const_vec4__const_vec4(const_vec4* result, const_vec4* x)
{
	ceil__const_float__const_float(&(*result)[0], &(*x)[0]);
	ceil__const_float__const_float(&(*result)[1], &(*x)[1]);
	ceil__const_float__const_float(&(*result)[2], &(*x)[2]);
	ceil__const_float__const_float(&(*result)[3], &(*x)[3]);
}
static void fract__const_float__const_float(const_float* result, const_float* x)
{
   const_float t1;

   op_floor__const_float__const_float(&t1, x);
   op_sub__const_float__const_float__const_float(result, x, &t1);
}
static void fract__const_vec2__const_vec2(const_vec2* result, const_vec2* x)
{
	fract__const_float__const_float(&(*result)[0], &(*x)[0]);
	fract__const_float__const_float(&(*result)[1], &(*x)[1]);
}
static void fract__const_vec3__const_vec3(const_vec3* result, const_vec3* x)
{
	fract__const_float__const_float(&(*result)[0], &(*x)[0]);
	fract__const_float__const_float(&(*result)[1], &(*x)[1]);
	fract__const_float__const_float(&(*result)[2], &(*x)[2]);
}
static void fract__const_vec4__const_vec4(const_vec4* result, const_vec4* x)
{
	fract__const_float__const_float(&(*result)[0], &(*x)[0]);
	fract__const_float__const_float(&(*result)[1], &(*x)[1]);
	fract__const_float__const_float(&(*result)[2], &(*x)[2]);
	fract__const_float__const_float(&(*result)[3], &(*x)[3]);
}
static void mod__const_float__const_float__const_float(const_float* result, const_float* x, const_float* y)
{
   const_float two = CONST_FLOAT_TWO;

   const_float r0, r1;
   const_float t1, t2, t3, t4, t5;

   op_recip__const_float__const_float(&r0, y);
   op_mul__const_float__const_float__const_float(&t1, y, &r0);
   op_sub__const_float__const_float__const_float(&t2, &two, &t1);
   op_mul__const_float__const_float__const_float(&r1, &r0, &t2);
   op_mul__const_float__const_float__const_float(&t3, x, &r1);
   op_floor__const_float__const_float(&t4, &t3);
   op_mul__const_float__const_float__const_float(&t5, y, &t4);
   op_sub__const_float__const_float__const_float(result, x, &t5);
}
static void mod__const_vec2__const_vec2__const_float(const_vec2* result, const_vec2* x, const_float* y)
{
	mod__const_float__const_float__const_float(&(*result)[0], &(*x)[0], y);
	mod__const_float__const_float__const_float(&(*result)[1], &(*x)[1], y);
}
static void mod__const_vec3__const_vec3__const_float(const_vec3* result, const_vec3* x, const_float* y)
{
	mod__const_float__const_float__const_float(&(*result)[0], &(*x)[0], y);
	mod__const_float__const_float__const_float(&(*result)[1], &(*x)[1], y);
	mod__const_float__const_float__const_float(&(*result)[2], &(*x)[2], y);
}
static void mod__const_vec4__const_vec4__const_float(const_vec4* result, const_vec4* x, const_float* y)
{
	mod__const_float__const_float__const_float(&(*result)[0], &(*x)[0], y);
	mod__const_float__const_float__const_float(&(*result)[1], &(*x)[1], y);
	mod__const_float__const_float__const_float(&(*result)[2], &(*x)[2], y);
	mod__const_float__const_float__const_float(&(*result)[3], &(*x)[3], y);
}
static void mod__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* x, const_vec2* y)
{
	mod__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0]);
	mod__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1]);
}
static void mod__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* x, const_vec3* y)
{
	mod__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0]);
	mod__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1]);
	mod__const_float__const_float__const_float(&(*result)[2], &(*x)[2], &(*y)[2]);
}
static void mod__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* x, const_vec4* y)
{
	mod__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0]);
	mod__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1]);
	mod__const_float__const_float__const_float(&(*result)[2], &(*x)[2], &(*y)[2]);
	mod__const_float__const_float__const_float(&(*result)[3], &(*x)[3], &(*y)[3]);
}
static void min__const_float__const_float__const_float(const_float* result, const_float* x, const_float* y)
{
   op_min__const_float__const_float__const_float(result, x, y);
}
static void min__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* x, const_vec2* y)
{
	min__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0]);
	min__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1]);
}
static void min__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* x, const_vec3* y)
{
	min__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0]);
	min__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1]);
	min__const_float__const_float__const_float(&(*result)[2], &(*x)[2], &(*y)[2]);
}
static void min__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* x, const_vec4* y)
{
	min__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0]);
	min__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1]);
	min__const_float__const_float__const_float(&(*result)[2], &(*x)[2], &(*y)[2]);
	min__const_float__const_float__const_float(&(*result)[3], &(*x)[3], &(*y)[3]);
}
static void min__const_vec2__const_vec2__const_float(const_vec2* result, const_vec2* x, const_float* y)
{
	min__const_float__const_float__const_float(&(*result)[0], &(*x)[0], y);
	min__const_float__const_float__const_float(&(*result)[1], &(*x)[1], y);
}
static void min__const_vec3__const_vec3__const_float(const_vec3* result, const_vec3* x, const_float* y)
{
	min__const_float__const_float__const_float(&(*result)[0], &(*x)[0], y);
	min__const_float__const_float__const_float(&(*result)[1], &(*x)[1], y);
	min__const_float__const_float__const_float(&(*result)[2], &(*x)[2], y);
}
static void min__const_vec4__const_vec4__const_float(const_vec4* result, const_vec4* x, const_float* y)
{
	min__const_float__const_float__const_float(&(*result)[0], &(*x)[0], y);
	min__const_float__const_float__const_float(&(*result)[1], &(*x)[1], y);
	min__const_float__const_float__const_float(&(*result)[2], &(*x)[2], y);
	min__const_float__const_float__const_float(&(*result)[3], &(*x)[3], y);
}
static void max__const_float__const_float__const_float(const_float* result, const_float* x, const_float* y)
{
   op_max__const_float__const_float__const_float(result, x, y);
}
static void max__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* x, const_vec2* y)
{
	max__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0]);
	max__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1]);
}
static void max__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* x, const_vec3* y)
{
	max__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0]);
	max__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1]);
	max__const_float__const_float__const_float(&(*result)[2], &(*x)[2], &(*y)[2]);
}
static void max__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* x, const_vec4* y)
{
	max__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0]);
	max__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1]);
	max__const_float__const_float__const_float(&(*result)[2], &(*x)[2], &(*y)[2]);
	max__const_float__const_float__const_float(&(*result)[3], &(*x)[3], &(*y)[3]);
}
static void max__const_vec2__const_vec2__const_float(const_vec2* result, const_vec2* x, const_float* y)
{
	max__const_float__const_float__const_float(&(*result)[0], &(*x)[0], y);
	max__const_float__const_float__const_float(&(*result)[1], &(*x)[1], y);
}
static void max__const_vec3__const_vec3__const_float(const_vec3* result, const_vec3* x, const_float* y)
{
	max__const_float__const_float__const_float(&(*result)[0], &(*x)[0], y);
	max__const_float__const_float__const_float(&(*result)[1], &(*x)[1], y);
	max__const_float__const_float__const_float(&(*result)[2], &(*x)[2], y);
}
static void max__const_vec4__const_vec4__const_float(const_vec4* result, const_vec4* x, const_float* y)
{
	max__const_float__const_float__const_float(&(*result)[0], &(*x)[0], y);
	max__const_float__const_float__const_float(&(*result)[1], &(*x)[1], y);
	max__const_float__const_float__const_float(&(*result)[2], &(*x)[2], y);
	max__const_float__const_float__const_float(&(*result)[3], &(*x)[3], y);
}
static void clamp__const_float__const_float__const_float__const_float(const_float* result, const_float* x, const_float* minVal, const_float* maxVal)
{
   const_float t1;

   op_max__const_float__const_float__const_float(&t1, x, minVal);
   op_min__const_float__const_float__const_float(result, &t1, maxVal);
}
static void clamp__const_vec2__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* x, const_vec2* minVal, const_vec2* maxVal)
{
	clamp__const_float__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*minVal)[0], &(*maxVal)[0]);
	clamp__const_float__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*minVal)[1], &(*maxVal)[1]);
}
static void clamp__const_vec3__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* x, const_vec3* minVal, const_vec3* maxVal)
{
	clamp__const_float__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*minVal)[0], &(*maxVal)[0]);
	clamp__const_float__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*minVal)[1], &(*maxVal)[1]);
	clamp__const_float__const_float__const_float__const_float(&(*result)[2], &(*x)[2], &(*minVal)[2], &(*maxVal)[2]);
}
static void clamp__const_vec4__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* x, const_vec4* minVal, const_vec4* maxVal)
{
	clamp__const_float__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*minVal)[0], &(*maxVal)[0]);
	clamp__const_float__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*minVal)[1], &(*maxVal)[1]);
	clamp__const_float__const_float__const_float__const_float(&(*result)[2], &(*x)[2], &(*minVal)[2], &(*maxVal)[2]);
	clamp__const_float__const_float__const_float__const_float(&(*result)[3], &(*x)[3], &(*minVal)[3], &(*maxVal)[3]);
}
static void clamp__const_vec2__const_vec2__const_float__const_float(const_vec2* result, const_vec2* x, const_float* minVal, const_float* maxVal)
{
	clamp__const_float__const_float__const_float__const_float(&(*result)[0], &(*x)[0], minVal, maxVal);
	clamp__const_float__const_float__const_float__const_float(&(*result)[1], &(*x)[1], minVal, maxVal);
}
static void clamp__const_vec3__const_vec3__const_float__const_float(const_vec3* result, const_vec3* x, const_float* minVal, const_float* maxVal)
{
	clamp__const_float__const_float__const_float__const_float(&(*result)[0], &(*x)[0], minVal, maxVal);
	clamp__const_float__const_float__const_float__const_float(&(*result)[1], &(*x)[1], minVal, maxVal);
	clamp__const_float__const_float__const_float__const_float(&(*result)[2], &(*x)[2], minVal, maxVal);
}
static void clamp__const_vec4__const_vec4__const_float__const_float(const_vec4* result, const_vec4* x, const_float* minVal, const_float* maxVal)
{
	clamp__const_float__const_float__const_float__const_float(&(*result)[0], &(*x)[0], minVal, maxVal);
	clamp__const_float__const_float__const_float__const_float(&(*result)[1], &(*x)[1], minVal, maxVal);
	clamp__const_float__const_float__const_float__const_float(&(*result)[2], &(*x)[2], minVal, maxVal);
	clamp__const_float__const_float__const_float__const_float(&(*result)[3], &(*x)[3], minVal, maxVal);
}
static void mix__const_float__const_float__const_float__const_float(const_float* result, const_float* x, const_float* y, const_float* a)
{
   // return x * (1.0 - a) + y * a;

   const_float o = CONST_FLOAT_ONE;
   const_float t1, t2, t3;

   op_sub__const_float__const_float__const_float(&t1, &o, a);
   op_mul__const_float__const_float__const_float(&t2, x, &t1);
   op_mul__const_float__const_float__const_float(&t3, y, a);
   op_add__const_float__const_float__const_float(result, &t2, &t3);
}
static void mix__const_vec2__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* x, const_vec2* y, const_vec2* a)
{
	mix__const_float__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0], &(*a)[0]);
	mix__const_float__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1], &(*a)[1]);
}
static void mix__const_vec3__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* x, const_vec3* y, const_vec3* a)
{
	mix__const_float__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0], &(*a)[0]);
	mix__const_float__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1], &(*a)[1]);
	mix__const_float__const_float__const_float__const_float(&(*result)[2], &(*x)[2], &(*y)[2], &(*a)[2]);
}
static void mix__const_vec4__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* x, const_vec4* y, const_vec4* a)
{
	mix__const_float__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0], &(*a)[0]);
	mix__const_float__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1], &(*a)[1]);
	mix__const_float__const_float__const_float__const_float(&(*result)[2], &(*x)[2], &(*y)[2], &(*a)[2]);
	mix__const_float__const_float__const_float__const_float(&(*result)[3], &(*x)[3], &(*y)[3], &(*a)[3]);
}
static void mix__const_vec2__const_vec2__const_vec2__const_float(const_vec2* result, const_vec2* x, const_vec2* y, const_float* a)
{
	mix__const_float__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0], a);
	mix__const_float__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1], a);
}
static void mix__const_vec3__const_vec3__const_vec3__const_float(const_vec3* result, const_vec3* x, const_vec3* y, const_float* a)
{
	mix__const_float__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0], a);
	mix__const_float__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1], a);
	mix__const_float__const_float__const_float__const_float(&(*result)[2], &(*x)[2], &(*y)[2], a);
}
static void mix__const_vec4__const_vec4__const_vec4__const_float(const_vec4* result, const_vec4* x, const_vec4* y, const_float* a)
{
	mix__const_float__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &(*y)[0], a);
	mix__const_float__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &(*y)[1], a);
	mix__const_float__const_float__const_float__const_float(&(*result)[2], &(*x)[2], &(*y)[2], a);
	mix__const_float__const_float__const_float__const_float(&(*result)[3], &(*x)[3], &(*y)[3], a);
}
static void step__const_float__const_float__const_float(const_float* result, const_float* edge, const_float* x)
{
   const_int i;

   op_cmp__const_int__const_float__const_float(&i, x, edge);

   if (i < 0)
      *result = CONST_FLOAT_ZERO;
   else
      *result = CONST_FLOAT_ONE;
}
static void step__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* edge, const_vec2* x)
{
	step__const_float__const_float__const_float(&(*result)[0], &(*edge)[0], &(*x)[0]);
	step__const_float__const_float__const_float(&(*result)[1], &(*edge)[1], &(*x)[1]);
}
static void step__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* edge, const_vec3* x)
{
	step__const_float__const_float__const_float(&(*result)[0], &(*edge)[0], &(*x)[0]);
	step__const_float__const_float__const_float(&(*result)[1], &(*edge)[1], &(*x)[1]);
	step__const_float__const_float__const_float(&(*result)[2], &(*edge)[2], &(*x)[2]);
}
static void step__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* edge, const_vec4* x)
{
	step__const_float__const_float__const_float(&(*result)[0], &(*edge)[0], &(*x)[0]);
	step__const_float__const_float__const_float(&(*result)[1], &(*edge)[1], &(*x)[1]);
	step__const_float__const_float__const_float(&(*result)[2], &(*edge)[2], &(*x)[2]);
	step__const_float__const_float__const_float(&(*result)[3], &(*edge)[3], &(*x)[3]);
}
static void step__const_vec2__const_float__const_vec2(const_vec2* result, const_float* edge, const_vec2* x)
{
	step__const_float__const_float__const_float(&(*result)[0], edge, &(*x)[0]);
	step__const_float__const_float__const_float(&(*result)[1], edge, &(*x)[1]);
}
static void step__const_vec3__const_float__const_vec3(const_vec3* result, const_float* edge, const_vec3* x)
{
	step__const_float__const_float__const_float(&(*result)[0], edge, &(*x)[0]);
	step__const_float__const_float__const_float(&(*result)[1], edge, &(*x)[1]);
	step__const_float__const_float__const_float(&(*result)[2], edge, &(*x)[2]);
}
static void step__const_vec4__const_float__const_vec4(const_vec4* result, const_float* edge, const_vec4* x)
{
	step__const_float__const_float__const_float(&(*result)[0], edge, &(*x)[0]);
	step__const_float__const_float__const_float(&(*result)[1], edge, &(*x)[1]);
	step__const_float__const_float__const_float(&(*result)[2], edge, &(*x)[2]);
	step__const_float__const_float__const_float(&(*result)[3], edge, &(*x)[3]);
}
static void smoothstep__const_float__const_float__const_float__const_float(const_float* result, const_float* edge0, const_float* edge1, const_float* x)
{
   /*
      float t;
      t = clamp ((x - edge0) / (edge1 - edge0), 0.0, 1.0);
      return t * t * (3.0 - 2.0 * t);
   */

   const_float zero = CONST_FLOAT_ZERO;
   const_float one = CONST_FLOAT_ONE;
   const_float two = const_float_from_int(2);
   const_float three = const_float_from_int(3);

   const_float t1, t2, t3, t4, t5, t6, t7, t8;

   op_sub__const_float__const_float__const_float(&t1, x, edge0);
   op_sub__const_float__const_float__const_float(&t2, edge1, edge0);
   op_recip__const_float__const_float(&t3, &t2);
   op_mul__const_float__const_float__const_float(&t4, &t1, &t3);

   clamp__const_float__const_float__const_float__const_float(&t5, &t4, &zero, &one);

   op_mul__const_float__const_float__const_float(&t6, &two, &t5);
   op_sub__const_float__const_float__const_float(&t7, &three, &t6);
   op_mul__const_float__const_float__const_float(&t8, &t7, &t5);
   op_mul__const_float__const_float__const_float(result, &t8, &t5);
}
static void smoothstep__const_vec2__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* edge0, const_vec2* edge1, const_vec2* x)
{
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[0], &(*edge0)[0], &(*edge1)[0], &(*x)[0]);
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[1], &(*edge0)[1], &(*edge1)[1], &(*x)[1]);
}
static void smoothstep__const_vec3__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* edge0, const_vec3* edge1, const_vec3* x)
{
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[0], &(*edge0)[0], &(*edge1)[0], &(*x)[0]);
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[1], &(*edge0)[1], &(*edge1)[1], &(*x)[1]);
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[2], &(*edge0)[2], &(*edge1)[2], &(*x)[2]);
}
static void smoothstep__const_vec4__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* edge0, const_vec4* edge1, const_vec4* x)
{
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[0], &(*edge0)[0], &(*edge1)[0], &(*x)[0]);
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[1], &(*edge0)[1], &(*edge1)[1], &(*x)[1]);
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[2], &(*edge0)[2], &(*edge1)[2], &(*x)[2]);
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[3], &(*edge0)[3], &(*edge1)[3], &(*x)[3]);
}
static void smoothstep__const_vec2__const_float__const_float__const_vec2(const_vec2* result, const_float* edge0, const_float* edge1, const_vec2* x)
{
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[0], edge0, edge1, &(*x)[0]);
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[1], edge0, edge1, &(*x)[1]);
}
static void smoothstep__const_vec3__const_float__const_float__const_vec3(const_vec3* result, const_float* edge0, const_float* edge1, const_vec3* x)
{
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[0], edge0, edge1, &(*x)[0]);
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[1], edge0, edge1, &(*x)[1]);
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[2], edge0, edge1, &(*x)[2]);
}
static void smoothstep__const_vec4__const_float__const_float__const_vec4(const_vec4* result, const_float* edge0, const_float* edge1, const_vec4* x)
{
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[0], edge0, edge1, &(*x)[0]);
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[1], edge0, edge1, &(*x)[1]);
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[2], edge0, edge1, &(*x)[2]);
	smoothstep__const_float__const_float__const_float__const_float(&(*result)[3], edge0, edge1, &(*x)[3]);
}

static void length__const_float__const_float(const_float* result, const_float* x)
{
   // return abs(x);

   abs__const_float__const_float(result, x);
}
static void length__const_float__const_vec2(const_float* result, const_vec2* x)
{
   // return $$rcp($$rsqrt(dot(x, x)));

   const_float t1, t2;

   dot__const_float__const_vec2__const_vec2(&t1, x, x);

   op_rsqrt__const_float__const_float(&t2, &t1);
   op_recip__const_float__const_float(result, &t2);
}
static void length__const_float__const_vec3(const_float* result, const_vec3* x)
{
   // return $$rcp($$rsqrt(dot(x, x)));

   const_float t1, t2;

   dot__const_float__const_vec3__const_vec3(&t1, x, x);

   op_rsqrt__const_float__const_float(&t2, &t1);
   op_recip__const_float__const_float(result, &t2);
}
static void length__const_float__const_vec4(const_float* result, const_vec4* x)
{
   // return $$rcp($$rsqrt(dot(x, x)));

   const_float t1, t2;

   dot__const_float__const_vec4__const_vec4(&t1, x, x);

   op_rsqrt__const_float__const_float(&t2, &t1);
   op_recip__const_float__const_float(result, &t2);
}
static void distance__const_float__const_float__const_float(const_float* result, const_float* p0, const_float* p1)
{
   // return length(p0 - p1);

   const_float t1;

   op_sub__const_float__const_float__const_float(&t1, p0, p1);

   length__const_float__const_float(result, &t1);
}
static void distance__const_float__const_vec2__const_vec2(const_float* result, const_vec2* p0, const_vec2* p1)
{
   // return length(p0 - p1);

   const_vec2 t1;

   op_sub__const_float__const_float__const_float(&t1[0], &(*p0)[0], &(*p1)[0]);
   op_sub__const_float__const_float__const_float(&t1[1], &(*p0)[1], &(*p1)[1]);

   length__const_float__const_vec2(result, &t1);
}
static void distance__const_float__const_vec3__const_vec3(const_float* result, const_vec3* p0, const_vec3* p1)
{
   // return length(p0 - p1);

   const_vec3 t1;

   op_sub__const_float__const_float__const_float(&t1[0], &(*p0)[0], &(*p1)[0]);
   op_sub__const_float__const_float__const_float(&t1[1], &(*p0)[1], &(*p1)[1]);
   op_sub__const_float__const_float__const_float(&t1[2], &(*p0)[2], &(*p1)[2]);

   length__const_float__const_vec3(result, &t1);
}
static void distance__const_float__const_vec4__const_vec4(const_float* result, const_vec4* p0, const_vec4* p1)
{
   // return length(p0 - p1);

   const_vec4 t1;

   op_sub__const_float__const_float__const_float(&t1[0], &(*p0)[0], &(*p1)[0]);
   op_sub__const_float__const_float__const_float(&t1[1], &(*p0)[1], &(*p1)[1]);
   op_sub__const_float__const_float__const_float(&t1[2], &(*p0)[2], &(*p1)[2]);
   op_sub__const_float__const_float__const_float(&t1[3], &(*p0)[3], &(*p1)[3]);

   length__const_float__const_vec4(result, &t1);
}
static void dot__const_float__const_float__const_float(const_float* result, const_float* x, const_float* y)
{
   // return x*y;

   op_mul__const_float__const_float__const_float(result, x, y);
}
static void dot__const_float__const_vec2__const_vec2(const_float* result, const_vec2* x, const_vec2* y)
{
   // return x[0]*y[0] + x[1]*y[1];

   const_float t1, t2;

   op_mul__const_float__const_float__const_float(&t1, &(*x)[0], &(*x)[0]);
   op_mul__const_float__const_float__const_float(&t2, &(*x)[1], &(*x)[1]);
   op_add__const_float__const_float__const_float(result, &t1, &t2);
}
static void dot__const_float__const_vec3__const_vec3(const_float* result, const_vec3* x, const_vec3* y)
{
   // return x[0]*y[0] + x[1]*y[1] + x[2]*y[2];

   const_float t1, t2, t3, t4;

   op_mul__const_float__const_float__const_float(&t1, &(*x)[0], &(*x)[0]);
   op_mul__const_float__const_float__const_float(&t2, &(*x)[1], &(*x)[1]);
   op_mul__const_float__const_float__const_float(&t3, &(*x)[2], &(*x)[2]);
   op_add__const_float__const_float__const_float(&t4, &t1, &t2);
   op_add__const_float__const_float__const_float(result, &t4, &t3);
}
static void dot__const_float__const_vec4__const_vec4(const_float* result, const_vec4* x, const_vec4* y)
{
   // return x[0]*y[0] + x[1]*y[1] + x[2]*y[2] + x[3]*y[3];

   const_float t1, t2, t3, t4, t5, t6;

   op_mul__const_float__const_float__const_float(&t1, &(*x)[0], &(*x)[0]);
   op_mul__const_float__const_float__const_float(&t2, &(*x)[1], &(*x)[1]);
   op_mul__const_float__const_float__const_float(&t3, &(*x)[2], &(*x)[2]);
   op_mul__const_float__const_float__const_float(&t4, &(*x)[3], &(*x)[3]);
   op_add__const_float__const_float__const_float(&t5, &t1, &t2);
   op_add__const_float__const_float__const_float(&t6, &t5, &t3);
   op_add__const_float__const_float__const_float(result, &t6, &t4);
}
static void cross__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* x, const_vec3* y)
{
   // return vec3(x[1]*y[2] - x[2]*y[1], x[2]*y[0] - x[0]*y[2], x[0]*y[1] - x[1]*y[0]);

   const_float t1, t2;

   op_mul__const_float__const_float__const_float(&t1, &(*x)[1], &(*y)[2]);
   op_mul__const_float__const_float__const_float(&t2, &(*x)[2], &(*y)[1]);
   op_sub__const_float__const_float__const_float(&(*result)[0], &t1, &t2);

   op_mul__const_float__const_float__const_float(&t1, &(*x)[2], &(*y)[0]);
   op_mul__const_float__const_float__const_float(&t2, &(*x)[0], &(*y)[2]);
   op_sub__const_float__const_float__const_float(&(*result)[1], &t1, &t2);

   op_mul__const_float__const_float__const_float(&t1, &(*x)[0], &(*y)[1]);
   op_mul__const_float__const_float__const_float(&t2, &(*x)[1], &(*y)[0]);
   op_sub__const_float__const_float__const_float(&(*result)[2], &t1, &t2);
}
static void normalize__const_float__const_float(const_float* result, const_float* x)
{
   // return sign(x);

   sign__const_float__const_float(result, x);
}
static void normalize__const_vec2__const_vec2(const_vec2* result, const_vec2* x)
{
   // return x * $$rsqrt(dot(x, x));

   const_float t1, t2;

   dot__const_float__const_vec2__const_vec2(&t1, x, x);

   op_rsqrt__const_float__const_float(&t2, &t1);
   op_mul__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &t2);
   op_mul__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &t2);
}
static void normalize__const_vec3__const_vec3(const_vec3* result, const_vec3* x)
{
   // return x * $$rsqrt(dot(x, x));

   const_float t1, t2;

   dot__const_float__const_vec3__const_vec3(&t1, x, x);

   op_rsqrt__const_float__const_float(&t2, &t1);
   op_mul__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &t2);
   op_mul__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &t2);
   op_mul__const_float__const_float__const_float(&(*result)[2], &(*x)[2], &t2);
}
static void normalize__const_vec4__const_vec4(const_vec4* result, const_vec4* x)
{
   // return x * $$rsqrt(dot(x, x));

   const_float t1, t2;

   dot__const_float__const_vec4__const_vec4(&t1, x, x);

   op_rsqrt__const_float__const_float(&t2, &t1);
   op_mul__const_float__const_float__const_float(&(*result)[0], &(*x)[0], &t2);
   op_mul__const_float__const_float__const_float(&(*result)[1], &(*x)[1], &t2);
   op_mul__const_float__const_float__const_float(&(*result)[2], &(*x)[2], &t2);
   op_mul__const_float__const_float__const_float(&(*result)[3], &(*x)[3], &t2);
}
static void faceforward__const_float__const_float__const_float__const_float(const_float* result, const_float* N, const_float* I, const_float* Nref)
{
   /*
      if (dot(Nref, I) < 0.0)
         return N;
      else
         return -N;
   */

   const_float z = CONST_FLOAT_ZERO;
   const_float t1;

   const_int i;

   dot__const_float__const_float__const_float(&t1, Nref, I);

   op_cmp__const_int__const_float__const_float(&i, &t1, &z);

   if (i < 0)
      *result = *N;
   else
      op_arith_negate__const_float__const_float(result, N);
}
static void faceforward__const_vec2__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* N, const_vec2* I, const_vec2* Nref)
{
   /*
      if (dot(Nref, I) < 0.0)
         return N;
      else
         return -N;
   */

   const_float z = CONST_FLOAT_ZERO;
   const_float t1;

   const_int i;

   dot__const_float__const_vec2__const_vec2(&t1, Nref, I);

   op_cmp__const_int__const_float__const_float(&i, &t1, &z);

   if (i < 0) {
      (*result)[0] = (*N)[0];
      (*result)[1] = (*N)[1];
   } else {
      op_arith_negate__const_float__const_float(&(*result)[0], &(*N)[0]);
      op_arith_negate__const_float__const_float(&(*result)[1], &(*N)[1]);
   }
}
static void faceforward__const_vec3__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* N, const_vec3* I, const_vec3* Nref)
{
   /*
      if (dot(Nref, I) < 0.0)
         return N;
      else
         return -N;
   */

   const_float z = CONST_FLOAT_ZERO;
   const_float t1;

   const_int i;

   dot__const_float__const_vec3__const_vec3(&t1, Nref, I);

   op_cmp__const_int__const_float__const_float(&i, &t1, &z);

   if (i < 0) {
      (*result)[0] = (*N)[0];
      (*result)[1] = (*N)[1];
      (*result)[2] = (*N)[2];
   } else {
      op_arith_negate__const_float__const_float(&(*result)[0], &(*N)[0]);
      op_arith_negate__const_float__const_float(&(*result)[1], &(*N)[1]);
      op_arith_negate__const_float__const_float(&(*result)[2], &(*N)[2]);
   }
}
static void faceforward__const_vec4__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* N, const_vec4* I, const_vec4* Nref)
{
   /*
      if (dot(Nref, I) < 0.0)
         return N;
      else
         return -N;
   */

   const_float z = CONST_FLOAT_ZERO;
   const_float t1;

   const_int i;

   dot__const_float__const_vec4__const_vec4(&t1, Nref, I);

   op_cmp__const_int__const_float__const_float(&i, &t1, &z);

   if (i < 0) {
      (*result)[0] = (*N)[0];
      (*result)[1] = (*N)[1];
      (*result)[2] = (*N)[2];
      (*result)[3] = (*N)[3];
   } else {
      op_arith_negate__const_float__const_float(&(*result)[0], &(*N)[0]);
      op_arith_negate__const_float__const_float(&(*result)[1], &(*N)[1]);
      op_arith_negate__const_float__const_float(&(*result)[2], &(*N)[2]);
      op_arith_negate__const_float__const_float(&(*result)[3], &(*N)[3]);
   }
}
static void reflect__const_float__const_float__const_float(const_float* result, const_float* I, const_float* N)
{
   // return I - 2.0 * dot(N, I) * N;

   const_float two = const_float_from_int(2);
   const_float t1, t2, t3;

   dot__const_float__const_float__const_float(&t1, N, I);

   op_mul__const_float__const_float__const_float(&t2, &t1, &two);

   op_mul__const_float__const_float__const_float(&t3, &t2, N);
   op_sub__const_float__const_float__const_float(result, I, &t3);
}
static void reflect__const_vec2__const_vec2__const_vec2(const_vec2* result, const_vec2* I, const_vec2* N)
{
   // return I - 2.0 * dot(N, I) * N;

   const_float two = const_float_from_int(2);
   const_float t1, t2, t3;

   dot__const_float__const_vec2__const_vec2(&t1, N, I);

   op_mul__const_float__const_float__const_float(&t2, &t1, &two);

   op_mul__const_float__const_float__const_float(&t3, &t2, &(*N)[0]);
   op_sub__const_float__const_float__const_float(&(*result)[0], &(*I)[0], &t3);

   op_mul__const_float__const_float__const_float(&t3, &t2, &(*N)[1]);
   op_sub__const_float__const_float__const_float(&(*result)[1], &(*I)[1], &t3);
}
static void reflect__const_vec3__const_vec3__const_vec3(const_vec3* result, const_vec3* I, const_vec3* N)
{
   // return I - 2.0 * dot(N, I) * N;

   const_float two = const_float_from_int(2);
   const_float t1, t2, t3;

   dot__const_float__const_vec3__const_vec3(&t1, N, I);

   op_mul__const_float__const_float__const_float(&t2, &t1, &two);

   op_mul__const_float__const_float__const_float(&t3, &t2, &(*N)[0]);
   op_sub__const_float__const_float__const_float(&(*result)[0], &(*I)[0], &t3);

   op_mul__const_float__const_float__const_float(&t3, &t2, &(*N)[1]);
   op_sub__const_float__const_float__const_float(&(*result)[1], &(*I)[1], &t3);

   op_mul__const_float__const_float__const_float(&t3, &t2, &(*N)[2]);
   op_sub__const_float__const_float__const_float(&(*result)[2], &(*I)[2], &t3);
}
static void reflect__const_vec4__const_vec4__const_vec4(const_vec4* result, const_vec4* I, const_vec4* N)
{
   // return I - 2.0 * dot(N, I) * N;

   const_float two = const_float_from_int(2);
   const_float t1, t2, t3;

   dot__const_float__const_vec4__const_vec4(&t1, N, I);

   op_mul__const_float__const_float__const_float(&t2, &t1, &two);

   op_mul__const_float__const_float__const_float(&t3, &t2, &(*N)[0]);
   op_sub__const_float__const_float__const_float(&(*result)[0], &(*I)[0], &t3);

   op_mul__const_float__const_float__const_float(&t3, &t2, &(*N)[1]);
   op_sub__const_float__const_float__const_float(&(*result)[1], &(*I)[1], &t3);

   op_mul__const_float__const_float__const_float(&t3, &t2, &(*N)[2]);
   op_sub__const_float__const_float__const_float(&(*result)[2], &(*I)[2], &t3);

   op_mul__const_float__const_float__const_float(&t3, &t2, &(*N)[3]);
   op_sub__const_float__const_float__const_float(&(*result)[3], &(*I)[3], &t3);
}

static void calc_k(const_float *k, const_float *d, const_float *eta)
{
   const_float one = CONST_FLOAT_ONE;

   const_float t1, t2, t3;

   op_mul__const_float__const_float__const_float(&t1, &one, d);
   op_mul__const_float__const_float__const_float(&t2, eta, &t1);
   op_mul__const_float__const_float__const_float(&t3, eta, &t2);
   op_sub__const_float__const_float__const_float(k, &one, &t3);
}

static void refract__const_float__const_float__const_float__const_float(const_float* result, const_float* I, const_float* N, const_float* eta)
{
   /*
      float d = dot(N, I);
      float k = 1.0 - eta * eta * (1.0 - d * d);
      if (k < 0.0)
         return float(0.0);
      else
         return eta * I - (eta * d + sqrt(k)) * N;
   */

   const_float zero = CONST_FLOAT_ZERO;
   const_float d, k;

   const_int i;

   dot__const_float__const_float__const_float(&d, N, I);

   calc_k(&k, &d, eta);

   op_cmp__const_int__const_float__const_float(&i, &k, &zero);

   if (i < 0) {
      *result = CONST_FLOAT_ZERO;
   } else {
      const_float t1, t2, t3, t4, t5;

      sqrt__const_float__const_float(&t1, &k);

      op_mul__const_float__const_float__const_float(&t2, eta, &d);
      op_add__const_float__const_float__const_float(&t3, &t1, &t2);

      op_mul__const_float__const_float__const_float(&t4, eta, I);
      op_mul__const_float__const_float__const_float(&t5, &t3, N);
      op_sub__const_float__const_float__const_float(result, &t4, &t5);
   }
}
static void refract__const_vec2__const_vec2__const_vec2__const_float(const_vec2* result, const_vec2* I, const_vec2* N, const_float* eta)
{
   /*
      float d = dot(N, I);
      float k = 1.0 - eta * eta * (1.0 - d * d);
      if (k < 0.0)
         return float(0.0);
      else
         return eta * I - (eta * d + sqrt(k)) * N;
   */

   const_float zero = CONST_FLOAT_ZERO;
   const_float d, k;

   const_int i;

   dot__const_float__const_vec2__const_vec2(&d, N, I);

   calc_k(&k, &d, eta);

   op_cmp__const_int__const_float__const_float(&i, &k, &zero);

   if (i < 0) {
      (*result)[0] = CONST_FLOAT_ZERO;
      (*result)[1] = CONST_FLOAT_ZERO;
   } else {
      const_float t1, t2, t3, t4, t5;

      sqrt__const_float__const_float(&t1, &k);

      op_mul__const_float__const_float__const_float(&t2, eta, &d);
      op_add__const_float__const_float__const_float(&t3, &t1, &t2);

      op_mul__const_float__const_float__const_float(&t4, eta, &(*I)[0]);
      op_mul__const_float__const_float__const_float(&t5, &t3, &(*N)[0]);
      op_sub__const_float__const_float__const_float(&(*result)[0], &t4, &t5);

      op_mul__const_float__const_float__const_float(&t4, eta, &(*I)[1]);
      op_mul__const_float__const_float__const_float(&t5, &t3, &(*N)[1]);
      op_sub__const_float__const_float__const_float(&(*result)[1], &t4, &t5);
   }
}
static void refract__const_vec3__const_vec3__const_vec3__const_float(const_vec3* result, const_vec3* I, const_vec3* N, const_float* eta)
{
   /*
      float d = dot(N, I);
      float k = 1.0 - eta * eta * (1.0 - d * d);
      if (k < 0.0)
         return float(0.0);
      else
         return eta * I - (eta * d + sqrt(k)) * N;
   */

   const_float zero = CONST_FLOAT_ZERO;
   const_float d, k;

   const_int i;

   dot__const_float__const_vec3__const_vec3(&d, N, I);

   calc_k(&k, &d, eta);

   op_cmp__const_int__const_float__const_float(&i, &k, &zero);

   if (i < 0) {
      (*result)[0] = CONST_FLOAT_ZERO;
      (*result)[1] = CONST_FLOAT_ZERO;
      (*result)[2] = CONST_FLOAT_ZERO;
   } else {
      const_float t1, t2, t3, t4, t5;

      sqrt__const_float__const_float(&t1, &k);

      op_mul__const_float__const_float__const_float(&t2, eta, &d);
      op_add__const_float__const_float__const_float(&t3, &t1, &t2);

      op_mul__const_float__const_float__const_float(&t4, eta, &(*I)[0]);
      op_mul__const_float__const_float__const_float(&t5, &t3, &(*N)[0]);
      op_sub__const_float__const_float__const_float(&(*result)[0], &t4, &t5);

      op_mul__const_float__const_float__const_float(&t4, eta, &(*I)[1]);
      op_mul__const_float__const_float__const_float(&t5, &t3, &(*N)[1]);
      op_sub__const_float__const_float__const_float(&(*result)[1], &t4, &t5);

      op_mul__const_float__const_float__const_float(&t4, eta, &(*I)[2]);
      op_mul__const_float__const_float__const_float(&t5, &t3, &(*N)[2]);
      op_sub__const_float__const_float__const_float(&(*result)[2], &t4, &t5);
   }
}
static void refract__const_vec4__const_vec4__const_vec4__const_float(const_vec4* result, const_vec4* I, const_vec4* N, const_float* eta)
{
   /*
      float d = dot(N, I);
      float k = 1.0 - eta * eta * (1.0 - d * d);
      if (k < 0.0)
         return float(0.0);
      else
         return eta * I - (eta * d + sqrt(k)) * N;
   */

   const_float zero = CONST_FLOAT_ZERO;
   const_float d, k;

   const_int i;

   dot__const_float__const_vec4__const_vec4(&d, N, I);

   calc_k(&k, &d, eta);

   op_cmp__const_int__const_float__const_float(&i, &k, &zero);

   if (i < 0) {
      (*result)[0] = CONST_FLOAT_ZERO;
      (*result)[1] = CONST_FLOAT_ZERO;
      (*result)[2] = CONST_FLOAT_ZERO;
      (*result)[3] = CONST_FLOAT_ZERO;
   } else {
      const_float t1, t2, t3, t4, t5;

      sqrt__const_float__const_float(&t1, &k);

      op_mul__const_float__const_float__const_float(&t2, eta, &d);
      op_add__const_float__const_float__const_float(&t3, &t1, &t2);

      op_mul__const_float__const_float__const_float(&t4, eta, &(*I)[0]);
      op_mul__const_float__const_float__const_float(&t5, &t3, &(*N)[0]);
      op_sub__const_float__const_float__const_float(&(*result)[0], &t4, &t5);

      op_mul__const_float__const_float__const_float(&t4, eta, &(*I)[1]);
      op_mul__const_float__const_float__const_float(&t5, &t3, &(*N)[1]);
      op_sub__const_float__const_float__const_float(&(*result)[1], &t4, &t5);

      op_mul__const_float__const_float__const_float(&t4, eta, &(*I)[2]);
      op_mul__const_float__const_float__const_float(&t5, &t3, &(*N)[2]);
      op_sub__const_float__const_float__const_float(&(*result)[2], &t4, &t5);

      op_mul__const_float__const_float__const_float(&t4, eta, &(*I)[3]);
      op_mul__const_float__const_float__const_float(&t5, &t3, &(*N)[3]);
      op_sub__const_float__const_float__const_float(&(*result)[3], &t4, &t5);
   }
}

static void matrixCompMult__const_mat2__const_mat2__const_mat2(const_mat2* result, const_mat2* x, const_mat2* y)
{
   int i, j;

   for (i = 0; i < 2; i++)
      for (j = 0; j < 2; j++)
         op_mul__const_float__const_float__const_float(&(*result)[i][j], &(*x)[i][j], &(*y)[i][j]);
}
static void matrixCompMult__const_mat3__const_mat3__const_mat3(const_mat3* result, const_mat3* x, const_mat3* y)
{
   int i, j;

   for (i = 0; i < 3; i++)
      for (j = 0; j < 3; j++)
         op_mul__const_float__const_float__const_float(&(*result)[i][j], &(*x)[i][j], &(*y)[i][j]);
}
static void matrixCompMult__const_mat4__const_mat4__const_mat4(const_mat4* result, const_mat4* x, const_mat4* y)
{
   int i, j;

   for (i = 0; i < 4; i++)
      for (j = 0; j < 4; j++)
         op_mul__const_float__const_float__const_float(&(*result)[i][j], &(*x)[i][j], &(*y)[i][j]);
}

static void lessThan__const_bool__const_float__const_float(const_bool* result, const_float* x, const_float* y)
{
   const_int i;

   op_cmp__const_int__const_float__const_float(&i, x, y);

   if (i < 0)
      *result = CONST_BOOL_TRUE;
   else
      *result = CONST_BOOL_FALSE;
}
static void lessThan__const_bvec2__const_vec2__const_vec2(const_bvec2* result, const_vec2* x, const_vec2* y)
{
   int i;

   for (i = 0; i < 2; i++)
      lessThan__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void lessThan__const_bvec3__const_vec3__const_vec3(const_bvec3* result, const_vec3* x, const_vec3* y)
{
   int i;

   for (i = 0; i < 3; i++)
      lessThan__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void lessThan__const_bvec4__const_vec4__const_vec4(const_bvec4* result, const_vec4* x, const_vec4* y)
{
   int i;

   for (i = 0; i < 4; i++)
      lessThan__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void lessThan__const_bool__const_int__const_int(const_bool* result, const_int* x, const_int* y)
{
   if (*x < *y)
      *result = CONST_BOOL_TRUE;
   else
      *result = CONST_BOOL_FALSE;
}
static void lessThan__const_bvec2__const_ivec2__const_ivec2(const_bvec2* result, const_ivec2* x, const_ivec2* y)
{
   int i;

   for (i = 0; i < 2; i++)
      lessThan__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void lessThan__const_bvec3__const_ivec3__const_ivec3(const_bvec3* result, const_ivec3* x, const_ivec3* y)
{
   int i;

   for (i = 0; i < 3; i++)
      lessThan__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void lessThan__const_bvec4__const_ivec4__const_ivec4(const_bvec4* result, const_ivec4* x, const_ivec4* y)
{
   int i;

   for (i = 0; i < 4; i++)
      lessThan__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void lessThanEqual__const_bool__const_float__const_float(const_bool* result, const_float* x, const_float* y)
{
   const_int i;

   op_cmp__const_int__const_float__const_float(&i, x, y);

   if (i <= 0)
      *result = CONST_BOOL_TRUE;
   else
      *result = CONST_BOOL_FALSE;
}
static void lessThanEqual__const_bvec2__const_vec2__const_vec2(const_bvec2* result, const_vec2* x, const_vec2* y)
{
   int i;

   for (i = 0; i < 2; i++)
      lessThanEqual__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void lessThanEqual__const_bvec3__const_vec3__const_vec3(const_bvec3* result, const_vec3* x, const_vec3* y)
{
   int i;

   for (i = 0; i < 3; i++)
      lessThanEqual__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void lessThanEqual__const_bvec4__const_vec4__const_vec4(const_bvec4* result, const_vec4* x, const_vec4* y)
{
   int i;

   for (i = 0; i < 4; i++)
      lessThanEqual__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void lessThanEqual__const_bool__const_int__const_int(const_bool* result, const_int* x, const_int* y)
{
   if (*x <= *y)
      *result = CONST_BOOL_TRUE;
   else
      *result = CONST_BOOL_FALSE;
}
static void lessThanEqual__const_bvec2__const_ivec2__const_ivec2(const_bvec2* result, const_ivec2* x, const_ivec2* y)
{
   int i;

   for (i = 0; i < 2; i++)
      lessThanEqual__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void lessThanEqual__const_bvec3__const_ivec3__const_ivec3(const_bvec3* result, const_ivec3* x, const_ivec3* y)
{
   int i;

   for (i = 0; i < 3; i++)
      lessThanEqual__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void lessThanEqual__const_bvec4__const_ivec4__const_ivec4(const_bvec4* result, const_ivec4* x, const_ivec4* y)
{
   int i;

   for (i = 0; i < 4; i++)
      lessThanEqual__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void greaterThan__const_bool__const_float__const_float(const_bool* result, const_float* x, const_float* y)
{
   const_int i;

   op_cmp__const_int__const_float__const_float(&i, x, y);

   if (i > 0)
      *result = CONST_BOOL_TRUE;
   else
      *result = CONST_BOOL_FALSE;
}
static void greaterThan__const_bvec2__const_vec2__const_vec2(const_bvec2* result, const_vec2* x, const_vec2* y)
{
   int i;

   for (i = 0; i < 2; i++)
      greaterThan__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void greaterThan__const_bvec3__const_vec3__const_vec3(const_bvec3* result, const_vec3* x, const_vec3* y)
{
   int i;

   for (i = 0; i < 3; i++)
      greaterThan__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void greaterThan__const_bvec4__const_vec4__const_vec4(const_bvec4* result, const_vec4* x, const_vec4* y)
{
   int i;

   for (i = 0; i < 4; i++)
      greaterThan__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void greaterThan__const_bool__const_int__const_int(const_bool* result, const_int* x, const_int* y)
{
   if (*x > *y)
      *result = CONST_BOOL_TRUE;
   else
      *result = CONST_BOOL_FALSE;
}
static void greaterThan__const_bvec2__const_ivec2__const_ivec2(const_bvec2* result, const_ivec2* x, const_ivec2* y)
{
   int i;

   for (i = 0; i < 2; i++)
      greaterThan__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void greaterThan__const_bvec3__const_ivec3__const_ivec3(const_bvec3* result, const_ivec3* x, const_ivec3* y)
{
   int i;

   for (i = 0; i < 3; i++)
      greaterThan__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void greaterThan__const_bvec4__const_ivec4__const_ivec4(const_bvec4* result, const_ivec4* x, const_ivec4* y)
{
   int i;

   for (i = 0; i < 4; i++)
      greaterThan__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void greaterThanEqual__const_bool__const_float__const_float(const_bool* result, const_float* x, const_float* y)
{
   const_int i;

   op_cmp__const_int__const_float__const_float(&i, x, y);

   if (i >= 0)
      *result = CONST_BOOL_TRUE;
   else
      *result = CONST_BOOL_FALSE;
}
static void greaterThanEqual__const_bvec2__const_vec2__const_vec2(const_bvec2* result, const_vec2* x, const_vec2* y)
{
   int i;

   for (i = 0; i < 2; i++)
      greaterThanEqual__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void greaterThanEqual__const_bvec3__const_vec3__const_vec3(const_bvec3* result, const_vec3* x, const_vec3* y)
{
   int i;

   for (i = 0; i < 3; i++)
      greaterThanEqual__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void greaterThanEqual__const_bvec4__const_vec4__const_vec4(const_bvec4* result, const_vec4* x, const_vec4* y)
{
   int i;

   for (i = 0; i < 4; i++)
      greaterThanEqual__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void greaterThanEqual__const_bool__const_int__const_int(const_bool* result, const_int* x, const_int* y)
{
   if (*x >= *y)
      *result = CONST_BOOL_TRUE;
   else
      *result = CONST_BOOL_FALSE;
}
static void greaterThanEqual__const_bvec2__const_ivec2__const_ivec2(const_bvec2* result, const_ivec2* x, const_ivec2* y)
{
   int i;

   for (i = 0; i < 2; i++)
      greaterThanEqual__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void greaterThanEqual__const_bvec3__const_ivec3__const_ivec3(const_bvec3* result, const_ivec3* x, const_ivec3* y)
{
   int i;

   for (i = 0; i < 3; i++)
      greaterThanEqual__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void greaterThanEqual__const_bvec4__const_ivec4__const_ivec4(const_bvec4* result, const_ivec4* x, const_ivec4* y)
{
   int i;

   for (i = 0; i < 4; i++)
      greaterThanEqual__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void equal__const_bool__const_float__const_float(const_bool* result, const_float* x, const_float* y)
{
   const_int i;

   op_cmp__const_int__const_float__const_float(&i, x, y);

   if (i == 0)
      *result = CONST_BOOL_TRUE;
   else
      *result = CONST_BOOL_FALSE;
}
static void equal__const_bvec2__const_vec2__const_vec2(const_bvec2* result, const_vec2* x, const_vec2* y)
{
   int i;

   for (i = 0; i < 2; i++)
      equal__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void equal__const_bvec3__const_vec3__const_vec3(const_bvec3* result, const_vec3* x, const_vec3* y)
{
   int i;

   for (i = 0; i < 3; i++)
      equal__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void equal__const_bvec4__const_vec4__const_vec4(const_bvec4* result, const_vec4* x, const_vec4* y)
{
   int i;

   for (i = 0; i < 4; i++)
      equal__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void equal__const_bool__const_int__const_int(const_bool* result, const_int* x, const_int* y)
{
   if (*x == *y)
      *result = CONST_BOOL_TRUE;
   else
      *result = CONST_BOOL_FALSE;
}
static void equal__const_bvec2__const_ivec2__const_ivec2(const_bvec2* result, const_ivec2* x, const_ivec2* y)
{
   int i;

   for (i = 0; i < 2; i++)
      equal__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void equal__const_bvec3__const_ivec3__const_ivec3(const_bvec3* result, const_ivec3* x, const_ivec3* y)
{
   int i;

   for (i = 0; i < 3; i++)
      equal__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void equal__const_bvec4__const_ivec4__const_ivec4(const_bvec4* result, const_ivec4* x, const_ivec4* y)
{
   int i;

   for (i = 0; i < 4; i++)
      equal__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void equal__const_bool__const_bool__const_bool(const_bool* result, const_bool* x, const_bool* y)
{
   if (*x == *y)
      *result = CONST_BOOL_TRUE;
   else
      *result = CONST_BOOL_FALSE;
}
static void equal__const_bvec2__const_bvec2__const_bvec2(const_bvec2* result, const_bvec2* x, const_bvec2* y)
{
   int i;

   for (i = 0; i < 2; i++)
      equal__const_bool__const_bool__const_bool(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void equal__const_bvec3__const_bvec3__const_bvec3(const_bvec3* result, const_bvec3* x, const_bvec3* y)
{
   int i;

   for (i = 0; i < 3; i++)
      equal__const_bool__const_bool__const_bool(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void equal__const_bvec4__const_bvec4__const_bvec4(const_bvec4* result, const_bvec4* x, const_bvec4* y)
{
   int i;

   for (i = 0; i < 4; i++)
      equal__const_bool__const_bool__const_bool(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void notEqual__const_bool__const_float__const_float(const_bool* result, const_float* x, const_float* y)
{
   const_int i;

   op_cmp__const_int__const_float__const_float(&i, x, y);

   if (i != 0)
      *result = CONST_BOOL_TRUE;
   else
      *result = CONST_BOOL_FALSE;
}
static void notEqual__const_bvec2__const_vec2__const_vec2(const_bvec2* result, const_vec2* x, const_vec2* y)
{
   int i;

   for (i = 0; i < 2; i++)
      notEqual__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void notEqual__const_bvec3__const_vec3__const_vec3(const_bvec3* result, const_vec3* x, const_vec3* y)
{
   int i;

   for (i = 0; i < 3; i++)
      notEqual__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void notEqual__const_bvec4__const_vec4__const_vec4(const_bvec4* result, const_vec4* x, const_vec4* y)
{
   int i;

   for (i = 0; i < 4; i++)
      notEqual__const_bool__const_float__const_float(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void notEqual__const_bool__const_int__const_int(const_bool* result, const_int* x, const_int* y)
{
   if (*x != *y)
      *result = CONST_BOOL_TRUE;
   else
      *result = CONST_BOOL_FALSE;
}
static void notEqual__const_bvec2__const_ivec2__const_ivec2(const_bvec2* result, const_ivec2* x, const_ivec2* y)
{
   int i;

   for (i = 0; i < 2; i++)
      notEqual__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void notEqual__const_bvec3__const_ivec3__const_ivec3(const_bvec3* result, const_ivec3* x, const_ivec3* y)
{
   int i;

   for (i = 0; i < 3; i++)
      notEqual__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void notEqual__const_bvec4__const_ivec4__const_ivec4(const_bvec4* result, const_ivec4* x, const_ivec4* y)
{
   int i;

   for (i = 0; i < 4; i++)
      notEqual__const_bool__const_int__const_int(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void notEqual__const_bool__const_bool__const_bool(const_bool* result, const_bool* x, const_bool* y)
{
   if (*x != *y)
      *result = CONST_BOOL_TRUE;
   else
      *result = CONST_BOOL_FALSE;
}
static void notEqual__const_bvec2__const_bvec2__const_bvec2(const_bvec2* result, const_bvec2* x, const_bvec2* y)
{
   int i;

   for (i = 0; i < 2; i++)
      notEqual__const_bool__const_bool__const_bool(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void notEqual__const_bvec3__const_bvec3__const_bvec3(const_bvec3* result, const_bvec3* x, const_bvec3* y)
{
   int i;

   for (i = 0; i < 3; i++)
      notEqual__const_bool__const_bool__const_bool(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void notEqual__const_bvec4__const_bvec4__const_bvec4(const_bvec4* result, const_bvec4* x, const_bvec4* y)
{
   int i;

   for (i = 0; i < 4; i++)
      notEqual__const_bool__const_bool__const_bool(&(*result)[i], &(*x)[i], &(*y)[i]);
}
static void any__const_bool__const_bvec2(const_bool* result, const_bvec2* x)
{
   *result = (*x)[0] | (*x)[1];
}
static void any__const_bool__const_bvec3(const_bool* result, const_bvec3* x)
{
   *result = (*x)[0] | (*x)[1] | (*x)[2];
}
static void any__const_bool__const_bvec4(const_bool* result, const_bvec4* x)
{
   *result = (*x)[0] | (*x)[1] | (*x)[2] | (*x)[3];
}
static void all__const_bool__const_bvec2(const_bool* result, const_bvec2* x)
{
   *result = (*x)[0] & (*x)[1];
}
static void all__const_bool__const_bvec3(const_bool* result, const_bvec3* x)
{
   *result = (*x)[0] & (*x)[1] & (*x)[2];
}
static void all__const_bool__const_bvec4(const_bool* result, const_bvec4* x)
{
   *result = (*x)[0] & (*x)[1] & (*x)[2] & (*x)[3];
}
static void not__const_bool__const_bool(const_bool* result, const_bool* x)
{
   *result = !(*x);
}
static void not__const_bvec2__const_bvec2(const_bvec2* result, const_bvec2* x)
{
   int i;

   for (i = 0; i < 2; i++)
      not__const_bool__const_bool(&(*result)[i], &(*x)[i]);
}
static void not__const_bvec3__const_bvec3(const_bvec3* result, const_bvec3* x)
{
   int i;

   for (i = 0; i < 3; i++)
      not__const_bool__const_bool(&(*result)[i], &(*x)[i]);
}
static void not__const_bvec4__const_bvec4(const_bvec4* result, const_bvec4* x)
{
   int i;

   for (i = 0; i < 4; i++)
      not__const_bool__const_bool(&(*result)[i], &(*x)[i]);
}
