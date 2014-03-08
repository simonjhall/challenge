/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_PREPRO_MACRO_H
#define GLSL_PREPRO_MACRO_H

#include "middleware/khronos/glsl/prepro/glsl_prepro_token.h"

#ifdef _DEBUG
#define MACRO_DEBUG        // compile dump routines
#endif

typedef enum {
   MACRO_UNDEF,
   MACRO_OBJECT,
   MACRO_FUNCTION,
   MACRO_LINE,
   MACRO_FILE
} MacroType;

typedef struct _Macro {
   MacroType type;

   Token *name;
   TokenList *args;
   TokenSeq *body;
} Macro;

extern Macro *glsl_macro_construct_undef(Token *name);
extern Macro *glsl_macro_construct_object(Token *name, TokenSeq *body);
extern Macro *glsl_macro_construct_function(Token *name, TokenList *args, TokenSeq *body);
extern Macro *glsl_macro_construct_line(void);
extern Macro *glsl_macro_construct_file(void);

extern bool glsl_macro_equals(Macro *m1, Macro *m2);

typedef struct _MacroList {
   Macro *macro;

   struct _MacroList *next;
} MacroList;

extern MacroList *glsl_macrolist_construct(Macro *macro, MacroList *next);
extern MacroList *glsl_macrolist_construct_initial(void);

extern Macro *glsl_macrolist_find(MacroList *list, Token *name);

extern bool glsl_is_defined(MacroList *list, Token *name);

extern TokenSeq *glsl_remove_defined(TokenSeq *seq);

#ifdef MACRO_DEBUG
extern void glsl_macro_dump(Macro *macro);
extern void glsl_macrolist_dump(MacroList *list);
#endif

#endif