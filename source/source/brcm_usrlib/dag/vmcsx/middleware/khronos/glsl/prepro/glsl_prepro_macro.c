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

#include "middleware/khronos/glsl/prepro/glsl_prepro_macro.h"
#include "middleware/khronos/glsl/prepro/glsl_prepro_directive.h"

#include "middleware/khronos/glsl/glsl_errors.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef MACRO_DEBUG
static struct {
   int macro;
   int macrolist;

   int total;
} malloc_info;

void macro_malloc_init()
{
   memset(&malloc_info, 0, sizeof(malloc_info));
}

void macro_malloc_print()
{
   printf("macro        = %d\n", malloc_info.macro);
   printf("macrolist    = %d\n", malloc_info.macrolist);
   printf("total        = %d\n", malloc_info.total);
}
#endif

static Macro *macro_alloc(void)
{
#ifdef MACRO_DEBUG
   malloc_info.macro += sizeof(Macro);
   malloc_info.total += sizeof(Macro);
#endif
   return (Macro *)malloc_fast(sizeof(Macro));
}

Macro *glsl_macro_construct_undef(Token *name)
{
   Macro *macro = macro_alloc();
   
   macro->type = MACRO_UNDEF;
   macro->name = name;

   return macro;
}

Macro *glsl_macro_construct_object(Token *name, TokenSeq *body)
{
   Macro *macro = macro_alloc();
   
   macro->type = MACRO_OBJECT;
   macro->name = name;
   macro->args = NULL;
   macro->body = body;

   return macro;
}

Macro *glsl_macro_construct_function(Token *name, TokenList *args, TokenSeq *body)
{
   Macro *macro = macro_alloc();
   
   macro->type = MACRO_FUNCTION;
   macro->name = name;
   macro->args = args;
   macro->body = body;

   return macro;
}

static Macro *glsl_macro_construct_builtin(MacroType type, char *s)
{
   Macro *macro = macro_alloc();

   TokenData data;

   data.s = s;

   macro->type = type;
   macro->name = glsl_token_construct(IDENTIFIER, data);
   macro->args = NULL;
   macro->body = NULL;

   return macro;
}

Macro *glsl_macro_construct_line()
{
   return glsl_macro_construct_builtin(MACRO_LINE, "__LINE__");
}

Macro *glsl_macro_construct_file()
{
   return glsl_macro_construct_builtin(MACRO_LINE, "__FILE__");
}

bool glsl_macro_equals(Macro *m1, Macro *m2)
{
   vcos_assert(m1);
   vcos_assert(m2);

   return m1->type == m2->type &&
          glsl_token_equals(m1->name, m2->name) &&
          glsl_tokenlist_equals(m1->args, m2->args) &&
          glsl_tokenseq_equals(m1->body, m2->body);
}

static MacroList *macrolist_alloc(void)
{
#ifdef MACRO_DEBUG
   malloc_info.macrolist += sizeof(MacroList);
   malloc_info.total += sizeof(MacroList);
#endif
   return (MacroList *)malloc_fast(sizeof(MacroList));
}

MacroList *glsl_macrolist_construct(Macro *macro, MacroList *next)
{
   MacroList *list = macrolist_alloc();

   list->macro = macro;
   list->next = next;

   return list;
}

MacroList *glsl_macrolist_construct_initial()
{
   MacroList *list = NULL;
      
   list = glsl_macrolist_construct(glsl_macro_construct_line(), list);
   list = glsl_macrolist_construct(glsl_macro_construct_file(), list);
   list = glsl_macrolist_construct(glsl_macro_construct_object(glsl_token_construct_identifier("__VERSION__"), glsl_tokenseq_construct(glsl_token_construct_ppnumberi(100), NULL, NULL)), list);
   list = glsl_macrolist_construct(glsl_macro_construct_object(glsl_token_construct_identifier("GL_ES"), glsl_tokenseq_construct(glsl_token_construct_ppnumberi(1), NULL, NULL)), list);
   list = glsl_macrolist_construct(glsl_macro_construct_object(glsl_token_construct_identifier("GL_FRAGMENT_PRECISION_HIGH"), glsl_tokenseq_construct(glsl_token_construct_ppnumberi(1), NULL, NULL)), list);

   return list;
}

Macro *glsl_macrolist_find(MacroList *list, Token *name)
{
   while (list) {
      if (glsl_token_equals(list->macro->name, name))
      {
         if (list->macro->type == MACRO_UNDEF)
            return NULL;
         else
            return list->macro;
      }

      list = list->next;
   }

   return NULL;
}

bool glsl_is_defined(MacroList *list, Token *name)
{
   return glsl_macrolist_find(list, name) != NULL;
}

TokenSeq *glsl_remove_defined(TokenSeq *seq)
{
   TokenSeq *res = NULL;

   while (seq) {
      if (seq->token->type == IDENTIFIER && !strcmp(seq->token->data.s, "defined")) {
         bool has_lparen;

         seq = seq->next;

         if (seq && seq->token->type == LEFT_PAREN) {
            seq = seq->next;
            has_lparen = true;
         } else
            has_lparen = false;

         if (seq && is_pp_identifier(seq->token)) {
            res = glsl_tokenseq_construct(glsl_token_construct_ppnumberi(glsl_is_defined(directive_macros, seq->token) ? 1 : 0), NULL, res);
            seq = seq->next;
         } else 
            glsl_compile_error(ERROR_PREPROCESSOR, 1, g_LineNumber, "expected identifier in constant expression");

         if (has_lparen)
         {
            if (seq && seq->token->type == RIGHT_PAREN)
               seq = seq->next;
            else 
               glsl_compile_error(ERROR_PREPROCESSOR, 1, g_LineNumber, "mismatched parenthesis in constant expression");
         }
      } else {
         res = glsl_tokenseq_construct(seq->token, NULL, res);
         seq = seq->next;
      }
   }

   return glsl_tokenseq_destructive_reverse(res, NULL);
}

#ifdef MACRO_DEBUG
void glsl_macro_dump(Macro *macro)
{
   glsl_token_dump(macro->name);

   if (macro->type == MACRO_FUNCTION) {
      printf("(");

      glsl_tokenlist_dump(macro->args, ", ");

      printf(")");
   }

   printf(" -> ");

   glsl_tokenseq_dump(macro->body, " ");

   printf("\n");
}

void glsl_macrolist_dump(MacroList *list) 
{
   while (list) {
      glsl_macro_dump(list->macro);

      list++;
   }
}
#endif
