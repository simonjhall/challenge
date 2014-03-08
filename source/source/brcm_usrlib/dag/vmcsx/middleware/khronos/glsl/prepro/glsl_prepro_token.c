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

#include "middleware/khronos/glsl/prepro/glsl_prepro_token.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

extern int yyfile;
extern int yyline;

#ifdef TOKEN_DEBUG
static struct {
   int token;
   int tokenlist;
   int tokenseq;
   int tokenseqlist;

   int total;
} malloc_info;

void glsl_token_malloc_init()
{
   memset(&malloc_info, 0, sizeof(malloc_info));
}

void glsl_token_malloc_print()
{
   printf("token        = %d\n", malloc_info.token);
   printf("tokenlist    = %d\n", malloc_info.tokenlist);
   printf("tokenseq     = %d\n", malloc_info.tokenseq);
   printf("tokenseqlist = %d\n", malloc_info.tokenseqlist);
   printf("total        = %d\n", malloc_info.total);
}
#endif

static Token *token_alloc(void)   /* clean */
{
#ifdef TOKEN_DEBUG
   malloc_info.token += sizeof(Token);
   malloc_info.total += sizeof(Token);
#endif
   return (Token *)malloc_fast(sizeof(Token));
}

Token *glsl_token_construct(TokenType type, TokenData data)    // clean
{
   Token *token = token_alloc();

   token->type = type;

   if (type == IDENTIFIER || (type >= PPNUMBERI && type <= PPNUMBERU) || (type >= DEFINE && type <= DISABLE))
      token->data = data;
   else
      token->data.s = NULL;

   return token;
}

Token *glsl_token_construct_identifier(char *s)    // clean
{
   TokenData data;

   data.s = s;

   return glsl_token_construct(IDENTIFIER, data);
}

Token *glsl_token_construct_ppnumberi(int i)    // clean
{
   TokenData data;

   data.i = i;

   return glsl_token_construct(PPNUMBERI, data);
}

bool glsl_token_equals(Token *t1, Token *t2)   // clean
{
   if (t1->type != t2->type)
      return false;

   switch (t1->type) {
   case IDENTIFIER:
   case PPNUMBERU:
      return !strcmp(t1->data.s, t2->data.s);
   case PPNUMBERI:
      return t1->data.i == t2->data.i;
   case PPNUMBERF:
      return t1->data.f == t2->data.f;
   default:
      return true;
   }
}

static TokenList *glsl_tokenlist_alloc(void)    /* clean */
{
#ifdef TOKEN_DEBUG
   malloc_info.tokenlist += sizeof(TokenList);
   malloc_info.total += sizeof(TokenList);
#endif
   return (TokenList *)malloc_fast(sizeof(TokenList));
}

TokenList *glsl_tokenlist_construct(Token *token, TokenList *next)  // clean
{
   TokenList *ts = glsl_tokenlist_alloc();

   ts->token = token;
   ts->next = next;

   return ts;
}

TokenList *glsl_tokenlist_intersect(TokenList *hs0, TokenList *hs1)
{
   TokenList *hs = NULL;

   while (hs0) {
      if (glsl_tokenlist_contains(hs1, hs0->token))
         hs = glsl_tokenlist_construct(hs0->token, hs);

      hs0 = hs0->next;
   }
   
   return hs;
}

TokenList *glsl_tokenlist_union(TokenList *hs0, TokenList *hs1)
{
   TokenList *hs = hs1;

   while (hs0) {
      if (!glsl_tokenlist_contains(hs1, hs0->token))
         hs = glsl_tokenlist_construct(hs0->token, hs);

      hs0 = hs0->next;
   }
   
   return hs;
}

bool glsl_tokenlist_equals(TokenList *t1, TokenList *t2)
{
   while (t1) {
      if (!t2 || !glsl_token_equals(t1->token, t2->token))
         return false;

      t1 = t1->next;
      t2 = t2->next;
   }

   return t2 == NULL;
}

bool glsl_tokenlist_contains(TokenList *hs, Token *t)
{
   while (hs) {
      if (glsl_token_equals(hs->token, t))
         return true;

      hs = hs->next;
   }

   return false;
}

int glsl_tokenlist_length(TokenList *hl)
{
   int len = 0;

   while (hl) {
      len++;

      hl = hl->next;
   }

   return len;
}

static TokenSeq *glsl_tokenseq_alloc(void)
{
#ifdef TOKEN_DEBUG
   malloc_info.tokenseq += sizeof(TokenSeq);
   malloc_info.total += sizeof(TokenSeq);
#endif
   return (TokenSeq *)malloc_fast(sizeof(TokenSeq));
}

TokenSeq *glsl_tokenseq_construct(Token *token, TokenList *hide, TokenSeq *next)
{
   TokenSeq *ts = glsl_tokenseq_alloc();;

   ts->token = token;
   ts->hide = hide;
   ts->next = next;

   return ts;
}

bool glsl_tokenseq_equals(TokenSeq *t1, TokenSeq *t2)
{
   while (t1) {
      if (!t2 || !glsl_token_equals(t1->token, t2->token))
         return false;

      t1 = t1->next;
      t2 = t2->next;
   }

   return t2 == NULL;
}

TokenSeq *glsl_tokenseq_destructive_reverse(TokenSeq *t, TokenSeq *p)
{
   while (t) {
      TokenSeq *n = t->next;
      t->next = p;
      p = t;
      t = n;
   }

   return p;
}

static TokenSeqList *tokenseqlist_alloc(void)
{
#ifdef TOKEN_DEBUG
   malloc_info.tokenseqlist += sizeof(TokenSeqList);
   malloc_info.total += sizeof(TokenSeqList);
#endif
   return (TokenSeqList *)malloc_fast(sizeof(TokenSeqList));
}

TokenSeqList *glsl_tokenseqlist_construct(TokenSeq *seq, TokenSeqList *next)
{
   TokenSeqList *tss = tokenseqlist_alloc();

   tss->seq = seq;
   tss->next = next;

   return tss;
}

#ifdef TOKEN_DEBUG
static const char *token_names[] = {
   "#",
   "<ws>",
   "<nl>",

   "<<",
   ">>",
   "++",
   "--",
   "<=",
   ">=",
   "==",
   "!=",
   "&&",
   "||",
   "^^",
   "*=",
   "/=",
   "+=",
   "%%=",
   "<<=",
   ">>=",
   "&=",
   "^=",
   "|=",
   "-=",
   "(",
   ")",
   "[",
   "]",
   "{",
   "}",
   ".",
   ",",
   ":",
   "=",
   ";",
   "!",
   "-",
   "~",
   "+",
   "*",
   "/",
   "%%",
   "<",
   ">",
   "|",
   "^",
   "&",
   "?",

   "$$texture_2d_bias",  
   "$$texture_2d_lod",   
   "$$texture_cube_bias",
   "$$texture_cube_lod", 
   "$$rsqrt",            
   "$$rcp",              
   "$$log2",             
   "$$exp2",    
   "$$ceil",
   "$$floor",
   "$$sign",
   "$$trunc",            
   "$$nearest",          
   "$$min",              
   "$$max",              

   "define",
   "undef",
   "ifdef",
   "ifndef",
   "elif",
   "endif",
   "error",
   "pragma",
   "extension",
   "version",
   "line",

   "all",
   "require",
   "enable",
   "warn",
   "disable",

   "attribute",
   "const",
   "bool",
   "float",
   "int",
   "break",
   "continue",
   "do",
   "else",
   "for",
   "if",
   "discard",
   "return",
   "bvec2",
   "bvec3",
   "bvec4",
   "ivec2",
   "ivec3",
   "ivec4",
   "vec2",
   "vec3",
   "vec4",
   "mat2",
   "mat3",
   "mat4",
   "in",
   "out",
   "inout",
   "uniform",
   "varying",
   "sampler2D",
   "samplerCube",
   "struct",
   "void",
   "while",
   "invariant",
   "highp",
   "mediump",
   "lowp",
   "precision",

   "asm",
   "class",
   "union",
   "enum",
   "typedef",
   "template",
   "this",
   "packed",
   "goto",
   "switch",
   "default",
   "inline",
   "noinline",
   "volatile",
   "public",
   "static",
   "extern",
   "external",
   "interface",
   "flat",
   "long",
   "short",
   "double",
   "half",
   "fixed",
   "unsigned",
   "superp",
   "input",
   "output",
   "hvec2",
   "hvec3",
   "hvec4",
   "dvec2",
   "dvec3",
   "dvec4",
   "fvec2",
   "fvec3",
   "fvec4",
   "sampler1D",
   "sampler3D",
   "sampler1DShadow",
   "sampler2DShadow",
   "sampler2DRect",
   "sampler3DRect",
   "sampler2DRectShadow",
   "sizeof",
   "cast",
   "namespace",
   "using",

   "true",
   "false"
};

void glsl_token_dump(Token *token)
{
   switch (token->type) {
   case IDENTIFIER:
   case PPNUMBERU:
   case UNKNOWN:
      printf(token->data.s);
      break;
   case PPNUMBERI:
      printf("%d", token->data.i);
      break;
   case PPNUMBERF:
      printf("%f", *(float *)&token->data.f);
      break;
   default:
      vcos_assert(token->type >= HASH && token->type <= _FALSE);
      vcos_assert(!strcmp(token_names[_FALSE - HASH], "false"));

      printf(token_names[token->type - HASH]);
      break;
   }
}

void glsl_tokenlist_dump(TokenList *list, char *sep)
{
   while (list) {
      glsl_token_dump(list->token);

      if (list->next)
         printf("%s", sep);

      list = list->next;
   }
}

void glsl_tokenseq_dump(TokenSeq *seq, char *sep)
{
   while (seq) {
      glsl_token_dump(seq->token);

      if (seq->next)
         printf("%s", sep);

      seq = seq->next;
   }
}
#endif
