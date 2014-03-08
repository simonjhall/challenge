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

#include <stdio.h>
#include <stdarg.h>
#include "middleware/khronos/glsl/glsl_globals.h"
#include "middleware/khronos/glsl/glsl_errors.h"

#ifdef __CC_ARM
#ifndef ANDROID
//#include "dbg.h"
#endif
#endif

#define BAD_ERROR_CODE "Invalid error code"

char* ErrorsCustom[] =
{
	//00000
	BAD_ERROR_CODE,
	//00001
	"Identifier is not a variable or parameter",
	//00002
	"Subscript must be integral type",
	//00003
	"Expression cannot be subscripted",
	//00004
	"2nd and 3rd parameters of ?: cannot be array",
	//00005
	"Array cannot be const",
	//00006
	"Out of memory",
	//00007
	"Out of temporaries",
	//00008
	"Too many texture accesses",
	//00009
	"Identifier is not a function name. Perhaps function is hidden by variable or type name?",
	//00010
	"const type qualifier cannot be used with out or inout parameter qualifier",
	//00011
	"No declared overload matches function call arguments",
	//00012
	"Discard statement can only be used in fragment shader",
	//00013
	"Support for indexing array/vector/matrix with a non-constant is only mandated for non-sampler uniforms in the vertex shader",
	//00014
	"Support for indexing array/vector/matrix with a non-constant is not mandated in the fragment shader",
	//00015
	"Unreachable code detected",
	//00016
	"Argument cannot be used as out or inout parameter",
	//00017
	"Unary increment/decrement operators act only on lvalues",
	//00018
	"Cannot instantiate void type",
	//00019
	"void type can only be used in empty formal parameter list",
	//00020
	"Recursion is not supported",
	//00021
	"Function call without function definition",
	//00022
	"Sampler type can only be used for uniform or function parameter",
	//00023
	"Nested swizzles not supported",
   //00024
   "Bad number of arguments to intrinsic",
   //00025
   "Bad type of arguments to intrinsic",
   //00026
   "Texture lookups in vertex shader not supported",
   //00027
	"Too many samplers",
   //00028
   "Incorrect #version",
   //00029
   "Use of break or continue outside loop",
   //0030
   "Missing function parameter name in function definition",
   //0031
   "Declaring an attribute, varying or uniform as a function parameter",
   //0032
   "Declaring a symbol beginning with gl_",
	//0033
	"Assignment to both gl_FragColor and gl_FragData[0]",
};

char* ErrorsPreprocessor[] =
{
	//P0000
	BAD_ERROR_CODE,
	//P0001
	"Preprocessor syntax error",
	//P0002
	"#error",
	//P0003
	"#extension if a required extension extension_name is not supported, or if all is specified",
	//P0004
	"High Precision not supported",
	//P0005
	"#version must be the 1st directive/statement in a program",
	//P0006
	"#line has wrong parameters",
};

char* ErrorsLexerParser[] =
{
	//L0000
	BAD_ERROR_CODE,
	//L0001
	"Syntax error",
	//L0002
	"Undefined identifier",
	//L0003
	"Use of reserved keywords",
};

char* ErrorsSemantic[] =
{
	//S0000
	BAD_ERROR_CODE,
	//S0001
	"Type mismatch in expression",
	//S0002
	"Array parameter must be an integer",
	//S0003
	"if parameter must be a boolean",
	//S0004
	"Operator not supported for operand types",
	//S0005
	"?: parameter must be a boolean",
	//S0006
	"2nd and 3rd parameters of ?: must have the same type",
	//S0007
	"Wrong arguments for constructor",
	//S0008
	"Argument unused in constructor",
	//S0009
	"Too few arguments for constructor",
	//S0010
	BAD_ERROR_CODE,
	//S0011
	"Arguments in wrong order for structure constructor",
	//S0012
	"Expression must be a constant expression",
	//S0013
	"Initializer for constant variable must be a constant expression",
	//S0014
	BAD_ERROR_CODE,
	//S0015
	"Expression must be an integral constant expression",
	//S0016
	BAD_ERROR_CODE,
	//S0017
	"Array size must be greater than zero",
	//S0018
	BAD_ERROR_CODE,
	//S0019
	BAD_ERROR_CODE,
	//S0020
	"Indexing with an integral constant expression greater than declared size",
	//S0021
	"Indexing with a negative integral constant expression",
	//S0022
	"Redefinition of variable in same scope",
	//S0023
	"Redefinition of function",
	//S0024
	"Redefinition of name in same scope (e.g. declaring a function with the same name as a struct)",
	//S0025
	"Field selectors must be from the same set (cannot mix xyzw with rgba)",
	//S0026
	"Illegal field selector (e.g. using .z with a vec2)",
	//S0027
	"Target of assignment is not an l-value",
	//S0028
	"Precision used with type other than integer, floating point or sampler type",
	//S0029
	"Declaring a main function with the wrong signature or return type",
	//S0030
	BAD_ERROR_CODE,
	//S0031
	"const variable does not have initializer",
	//S0032
	"Use of float or int without a precision qualifier where the default precision is not defined",
	//S0033
	"Expression that does not have an intrinsic precision where the default precision is not defined",
	//S0034
	"Variable cannot be declared invariant",
	//S0035
	"All uses of invariant must be at the global scope",
	//S0036
	BAD_ERROR_CODE,
	//S0037
	"L-value contains duplicate components (e.g. v.xx = q;)",
	//S0038
	"Function declared with a return value but return statement has no argument",
	//S0039
	"Function declared void but return statement has an argument",
	//S0040
	"Function declared with a return value but not all paths return a value",
	//S0041
	"Function return type is an array",
	//S0042
	"Return type of function definition must match return type of function declaration",
	//S0043
	"Parameter qualifiers of function definition must match parameter qualifiers of function declaration",
	//S0044
	"Declaring an attribute outside of a vertex shader",
	//S0045
	"Declaring an attribute inside a function",
	//S0046
	"Declaring a uniform inside a function",
	//S0047
	"Declaring a varying inside a function",
	//S0048
	"Illegal data type for varying",
	//S0049
	"Illegal data type for attribute (can only use float, vec2, vec3, vec4, mat2, mat3, and mat4)",
	//S0050
	"Initializer for attribute",
	//S0051
	"Initializer for varying",
	//S0052
	"Initializer for uniform",
};

char* ErrorsLinker[] =
{
	//L0000
	BAD_ERROR_CODE,
	//L0001
	"Global variables must have the same type (including the same names for structure and field names) and precision",
	//L0002
	BAD_ERROR_CODE,
	//L0003
	BAD_ERROR_CODE,
	//L0004
	"Too many attribute values",
	//L0005
	"Too many uniform values",
	//L0006
	"Too many varyings",
	//L0007
	"Fragment shader uses a varying that has not been declared in the vertex shader",
	//L0008
	"Type mismatch between varyings",
	//L0009
	"Missing main function for shader",
};

char* ErrorsOptimizer[] =
{
	//L0000
	BAD_ERROR_CODE,
	//L0001
	"Support for while loops is not mandated. Use a for loop instead",
	//L0002
	"Support for do-while loops is not mandated. Use a for loop instead",
	//L0003
	"Support for for loops is restricted",
	//L0004
	"Infinite loop detected"
};

char* Warnings[] =
{
	//W0000
	BAD_ERROR_CODE,
	//W0001
	"Unsupported extension"
};

char* ErrorTypeStrings[] =
{
	"UNKNOWN", // ERROR_UNKNOWN
	"CUSTOM", // ERROR_CUSTOM
	"PREPROCESSOR", // ERROR_PREPROCESSOR
	"LEX/PARSE", // ERROR_LEXER_PARSER
	"SEMANTIC", // ERROR_SEMANTIC
	"LINK", // ERROR_LINKER
	"OPTIMIZER", // ERROR_OPTIMIZER
   "WARN" // WARNING
};

char** ErrorStrings[] =
{
	NULL, // ERROR_UNKNOWN
	ErrorsCustom, // ERROR_CUSTOM
	ErrorsPreprocessor, // ERROR_PREPROCESSOR
	ErrorsLexerParser, // ERROR_LEXER_PARSER
	ErrorsSemantic, // ERROR_SEMANTIC
	ErrorsLinker, // ERROR_LINKER
	ErrorsOptimizer, // ERROR_OPTIMIZER
   Warnings // WARNING
};

/*
   glsl_compile_error

   Prints out a suitable message to the console. Exits compilation if it is an error (rather than a warning).

   Preconditions:
   Compilation is in progress
   (clarification, ...) form a valid printf sequence
   ASSERT_ON_ERROR not defined

   Postconditions:
   Returns normally iff e == WARNING

   Invariants preserved:
   -
*/

void glsl_compile_error(ErrorType e, int code, int line_num, const char* clarification, ...)
{
	va_list argp;
	int len;

   const char *kind = e == WARNING ? "WARNING" : "ERROR";
#ifdef WIN32
	if (!ErrorStrings[e])
	{
      printf("%s:%s-%d (line %d) ", kind, ErrorTypeStrings[e], code, line_num);
	}
	else
	{
		printf("%s:%s-%d (line %d) %s ", kind, ErrorTypeStrings[e], code, line_num, ErrorStrings[e][code]);
	}

	if (clarification)
	{
		printf(": ");

		va_start(argp, clarification);
		vprintf(clarification, argp);
		va_end(argp);

		printf("\n");
	}
	else
	{
		printf("\n");
	}
#else
	if (!ErrorStrings[e])
	{
      snprintf(error_buffer, MAX_ERROR_LENGTH, "%s:%s-%d (line %d)", kind, ErrorTypeStrings[e], code, line_num);
	}
	else
	{
		snprintf(error_buffer, MAX_ERROR_LENGTH, "%s:%s-%d (line %d) %s", kind, ErrorTypeStrings[e], code, line_num, ErrorStrings[e][code]);
	}

    len = strlen(error_buffer);

	if (clarification && len < MAX_ERROR_LENGTH - 3)
	{
      strcat(error_buffer, " : ");
      len += 3;

		va_start(argp, clarification);
		vsnprintf(error_buffer + len, MAX_ERROR_LENGTH - len, clarification, argp);
		va_end(argp);
	}

#ifdef __CC_ARM
#define ASSERT_ON_ERROR
#ifndef ANDROID
	dprintf(34, error_buffer);
#endif
#endif

#endif
   if (e != WARNING) {
//#define ASSERT_ON_ERROR
#ifdef ASSERT_ON_ERROR
      assert(0);
#endif
	   glsl_compiler_exit();
   }
}
