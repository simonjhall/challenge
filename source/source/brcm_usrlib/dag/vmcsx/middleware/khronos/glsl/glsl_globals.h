/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_GLOBALS_H
#define GLSL_GLOBALS_H

#include <setjmp.h>

#include "middleware/khronos/glsl/glsl_compiler.h"
#include "middleware/khronos/glsl/glsl_symbols.h"
#include "middleware/khronos/glsl/glsl_stringbuilder.h"
#include "middleware/khronos/glsl/glsl_dataflow.h"

//
// Global variable declarations.
//

// Source management.
extern int g_ShaderSourceCurrentIndex;
extern int g_ShaderSourceCount;
extern const char** g_ShaderSourceValues;

// Whether we're compiling a fragment shader, vertex shader, etc.
extern ShaderFlavour g_ShaderFlavour;

// The line number that Lex has reached.
extern int g_FileNumber;
extern int g_LineNumber;
#define LINE_NUMBER_UNDEFINED -1

// The main symbol table, including all built in symbols.
extern SymbolTable* g_SymbolTable;
// The symbols in the symbol table that are sources for the dataflow graph.
extern DataflowSources* g_DataflowSources;
// Bitmask specifying rows of VRF occupied by scalar attributes
extern int g_AttributeScalarValueMask;
// Bitmask specifiying rows of VRF occuped by aliased attributes (and therefore permanently unavailable for reuse)
extern int g_AttributeScalarAliasMask;
// The number of varying scalar values.
extern int g_VaryingScalarValueCount;
// The Dataflow nodes that represent the values written to each scalar in each varying by the end of the vertex shader.
// size is g_VaryingScalarValueCount.
extern Dataflow** g_VertexVaryingScalarValues;

// State for the current type.
extern SymbolType* g_TypeBuilder;
extern TypeQualifier g_TypeQual;
extern ParamQualifier g_ParamQual;

// String builder for creating type names.
extern StringBuilder g_StringBuilder;

// State for the current symbol.
extern Expr* g_InitializerExpr;
extern Symbol* g_LastInstance;

// State for the current struct type.
extern SymbolTable* g_StructBuilderMembers;
extern int g_StructBuilderMemberCount;
extern int g_StructBuilderSizeAsConst;
extern bool g_StructBuilderCanBeConst;
extern int g_StructBuilderScalarCount;

// State for the current function declaration/definition.
extern SymbolType* g_FunctionBuilderReturnType;
extern const char* g_FunctionBuilderName;
extern SymbolTable* g_FunctionBuilderParams;
extern int g_FunctionBuilderParamCount;
extern int g_FunctionBuilderVoidCount;
extern Symbol* g_DefinitionInsertionSymbol;
extern SymbolType* g_DefinitionInsertionSymbolType;

// Index of next $$anon<x> anonymous function parameter.
extern int g_NextAnonParam;

// Whether the parser is currently in global scope.
extern bool g_InGlobalScope;

// Sticky flags telling us whether we assigned to gl_FragColor or gl_FragData[0]
extern bool g_AssignedFragColor;
extern bool g_AssignedFragData;

// Buffer to store environment so that stack can be unwound on error.
extern jmp_buf g_ErrorHandlerEnv;

//
// Built in vars and types.
//

// Floating point and integer zero, boolean true and false
extern Dataflow *g_BoolFalse;
extern Dataflow *g_BoolTrue;
extern Dataflow* g_IntZero;
extern Dataflow* g_IntOne;
extern Dataflow* g_FloatZero;
extern Dataflow* g_FloatOne;

// Vertex shader.
extern Symbol* g_BuiltInVar__gl_Position;
extern Symbol* g_BuiltInVar__gl_PointSize;

// Fragment shader.
extern Symbol* g_BuiltInVar__gl_FragCoord;
extern Symbol* g_BuiltInVar__gl_FrontFacing;
extern Symbol* g_BuiltInVar__gl_FragColor;
extern Symbol* g_BuiltInVar__gl_FragData;
extern Symbol* g_BuiltInVar__gl_PointCoord;

extern Symbol* g_BuiltInVar__discard;

// Constants.
extern Symbol* g_BuiltInVar__gl_MaxVertexAttribs;
extern Symbol* g_BuiltInVar__gl_MaxVertexUniformVectors;
extern Symbol* g_BuiltInVar__gl_MaxVaryingVectors;
extern Symbol* g_BuiltInVar__gl_MaxVertexTextureImageUnits;
extern Symbol* g_BuiltInVar__gl_MaxCombinedTextureImageUnits;
extern Symbol* g_BuiltInVar__gl_MaxTextureImageUnits;
extern Symbol* g_BuiltInVar__gl_MaxFragmentUniformVectors;
extern Symbol* g_BuiltInVar__gl_MaxDrawBuffers;

// Uniforms.
extern Symbol* g_BuiltInType__gl_DepthRangeParameters;
extern Symbol* g_BuiltInVar__gl_DepthRange;

#endif // GLOBALS_H