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

#include "middleware/khronos/glsl/glsl_globals.h"

// TODO: there are some globals we've left out here, e.g. primitiveType stuff

int g_ShaderSourceCurrentIndex;
int g_ShaderSourceCount;
const char** g_ShaderSourceValues;

/*
   g_ShaderFlavour
   
   Identifies whether we are currently compiling a vertex or fragment shader.
   
   Invariants:
   
   (GL20_COMPILER_SHADERFLAVOUR)
   During a compilation run, g_ShaderFlavour in {SHADER_VERTEX,SHADER_FRAGMENT}
   g_ShaderFlavour constant during a compilation run
*/
ShaderFlavour g_ShaderFlavour;

int g_FileNumber;
int g_LineNumber;

SymbolTable* g_SymbolTable;
DataflowSources* g_DataflowSources;
int g_AttributeScalarValueMask;
int g_AttributeScalarAliasMask;
int g_VaryingScalarValueCount;
Dataflow** g_VertexVaryingScalarValues;

SymbolType* g_TypeBuilder;
TypeQualifier g_TypeQual;
ParamQualifier g_ParamQual;
StringBuilder g_StringBuilder;
Expr* g_InitializerExpr;
Symbol* g_LastInstance;

SymbolTable* g_StructBuilderMembers;
int g_StructBuilderMemberCount;
int g_StructBuilderSizeAsConst;
bool g_StructBuilderCanBeConst;
int g_StructBuilderScalarCount;

SymbolType* g_FunctionBuilderReturnType;
const char* g_FunctionBuilderName;
SymbolTable* g_FunctionBuilderParams;
int g_FunctionBuilderParamCount;
int g_FunctionBuilderVoidCount;
Symbol* g_DefinitionInsertionSymbol;
SymbolType* g_DefinitionInsertionSymbolType;

int g_NextAnonParam;

bool g_InGlobalScope;

bool g_AssignedFragColor;
bool g_AssignedFragData;

jmp_buf g_ErrorHandlerEnv;

Dataflow *g_BoolFalse = NULL;
Dataflow *g_BoolTrue = NULL;
Dataflow* g_IntZero = NULL;
Dataflow* g_IntOne = NULL;
Dataflow* g_FloatZero = NULL;
Dataflow* g_FloatOne = NULL;

Symbol* g_BuiltInVar__gl_Position;
Symbol* g_BuiltInVar__gl_PointSize;

Symbol* g_BuiltInVar__gl_FragCoord;
Symbol* g_BuiltInVar__gl_FrontFacing;
Symbol* g_BuiltInVar__gl_FragColor;
Symbol* g_BuiltInVar__gl_FragData;
Symbol* g_BuiltInVar__gl_PointCoord;

Symbol* g_BuiltInVar__discard;

Symbol* g_BuiltInVar__gl_MaxVertexAttribs;
Symbol* g_BuiltInVar__gl_MaxVertexUniformVectors;
Symbol* g_BuiltInVar__gl_MaxVaryingVectors;
Symbol* g_BuiltInVar__gl_MaxVertexTextureImageUnits;
Symbol* g_BuiltInVar__gl_MaxCombinedTextureImageUnits;
Symbol* g_BuiltInVar__gl_MaxTextureImageUnits;
Symbol* g_BuiltInVar__gl_MaxFragmentUniformVectors;
Symbol* g_BuiltInVar__gl_MaxDrawBuffers;

Symbol* g_BuiltInType__gl_DepthRangeParameters;
Symbol* g_BuiltInVar__gl_DepthRange;