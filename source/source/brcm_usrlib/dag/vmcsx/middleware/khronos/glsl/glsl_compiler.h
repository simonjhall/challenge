/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_COMPILER_H
#define GLSL_COMPILER_H

#include <stdlib.h>
#include <stdio.h>

#include "middleware/khronos/common/khrn_mem.h"

#include "interface/khronos/include/GLES2/gl2.h"

//
// Uniforms.
//

// Note that uniform variables can appear in the source as:
// - float/int/bool scalars/vectors/matrices
// - samplers
// - arrays of these
// - structs of these
//
// Samplers are conceptually different to the other types and so they are handled separately (below).

// base <= row < limit
#define SLANG_UNIFORMS_BASE_ROW        0
#define SLANG_UNIFORMS_LIMIT_ROW       544         /* Must be at least 513 to run glBenchmark 2 */
#define GL_MAXVERTEXUNIFORMVECTORS 	   ((SLANG_UNIFORMS_LIMIT_ROW - SLANG_UNIFORMS_BASE_ROW) / 4)
#define GL_MAXFRAGMENTUNIFORMVECTORS 	GL_MAXVERTEXUNIFORMVECTORS

#define SLANG_GL_DEPTHRANGE_NEAR_ROW   1021
#define SLANG_GL_DEPTHRANGE_FAR_ROW    1022
#define SLANG_GL_DEPTHRANGE_DIFF_ROW   1023

//
// Attributes.
//

// Note that attribute variables can appear in the source as:
// - float scalars/vectors/matrices

#define SLANG_MAX_NUM_ATTRIBUTES       64
#define GL_MAXVERTEXATTRIBS            8

//
// Varyings.
//

// Note that varying variables can appear in the source as:
// - float scalars/vectors/matrices
// - arrays of these

// Varyings start at SLANG_VARYINGS_BASE_ROW and occupy slang_getNumVaryings() contiguous rows.
// base <= row < limit
#define SLANG_VARYINGS_BASE_ROW     32
#define SLANG_VARYINGS_LIMIT_ROW    64
#define GL_MAXVARYINGVECTORS 			((SLANG_VARYINGS_LIMIT_ROW - SLANG_VARYINGS_BASE_ROW) / 4)

//
// Samplers.
// Note that vertex and fragment samplers are handled separately.
//

#define SLANG_MAX_NUM_VSAMPLERS           0
#define SLANG_MAX_NUM_FSAMPLERS           8
#define GL_MAXVERTEXTEXTUREIMAGEUNITS 	   SLANG_MAX_NUM_VSAMPLERS
#define GL_MAXCOMBINEDTEXTUREIMAGEUNITS   (SLANG_MAX_NUM_VSAMPLERS + SLANG_MAX_NUM_FSAMPLERS)
#define GL_MAXTEXTUREIMAGEUNITS 		      SLANG_MAX_NUM_FSAMPLERS
#define GL_MAXDRAWBUFFERS 				      1

//
// Structs and enums.
//

typedef struct slang_vshader_s
{
   int sourcec;
   const char** sourcev; // these are *copies* of the input and so can be safely mutated
} slang_vshader;

typedef struct slang_fshader_s
{
   int sourcec;
   const char** sourcev; // these are *copies* of the input and so can be safely mutated
} slang_fshader;

typedef struct slang_binding_s
{
   int index;
   const char *name;
} slang_binding;

typedef struct slang_sampler_s
{
   GLint uniform;
   GLint array_index;
} slang_sampler;

typedef struct slang_uniform_s
{
	GLenum type;
   union {
   	const char* name;
      MEM_HANDLE_T handle;
   } u;
	unsigned int row;
	unsigned int array_length;
} slang_uniform;

typedef struct slang_attribute_s
{
	GLenum type;
   union {
	   const char* name;
      MEM_HANDLE_T handle;
   } u;
	unsigned int row;
} slang_attribute;

typedef struct slang_program_s
{
   slang_vshader vshader;
   slang_fshader fshader;

   unsigned int num_bindings;
   slang_binding *bindings;

   unsigned int num_samplers;
   slang_sampler samplers[SLANG_MAX_NUM_FSAMPLERS];

   unsigned int num_uniforms;
   slang_uniform uniforms[SLANG_UNIFORMS_LIMIT_ROW - SLANG_UNIFORMS_BASE_ROW];

   unsigned int num_attributes;
   slang_attribute attributes[SLANG_MAX_NUM_ATTRIBUTES];

   unsigned int num_scalar_varyings;
   unsigned int num_scalar_uniforms;

   unsigned int live_attributes;
#ifndef __VIDEOCORE4__
   short* code_vshader;
   short* code_fshader;

   unsigned int size_vshader;
   unsigned int size_fshader;
#endif

#ifdef __VIDEOCORE4__
   void *result;  /* GL20_LINK_RESULT_T */
#endif
} slang_program;

typedef enum
{
	SHADER_VERTEX,
	SHADER_FRAGMENT,
	SHADER_FLAVOUR_UNDEFINED
} ShaderFlavour;

//
// Functions.
//

#define MAX_ERROR_LENGTH 256

extern char error_buffer[MAX_ERROR_LENGTH];

// The top level compilation function.
// Expects that program has valid vshader and fshader,
// but all other program metadata will be set by this function.
extern bool glsl_compile_and_link(slang_program* program);

// Causes the compiler to exit early.
void glsl_compiler_exit(void);

#endif // COMPILER_H