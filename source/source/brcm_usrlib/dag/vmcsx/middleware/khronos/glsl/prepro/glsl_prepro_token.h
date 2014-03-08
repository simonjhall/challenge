/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_PREPRO_TOKEN_H
#define GLSL_PREPRO_TOKEN_H

#include "middleware/khronos/glsl/glsl_const_types.h"
#include "middleware/khronos/glsl/glsl_symbols.h"
#include "middleware/khronos/glsl/y.tab.h"

#ifndef NDEBUG
#define TOKEN_DEBUG
#endif

typedef enum yytokentype TokenType;

typedef union _TokenData {
   const char *s;
   const_int i;
   const_float f;
} TokenData;

typedef struct _Token {
   TokenType type;
   TokenData data;
} Token;

extern Token *glsl_token_construct(TokenType type, TokenData data);
extern Token *glsl_token_construct_identifier(char *s);
extern Token *glsl_token_construct_ppnumberi(int i);

extern bool glsl_token_equals(Token *t1, Token *t2);

static INLINE bool is_pp_identifier(Token *t)
{
   return t->type == IDENTIFIER || (t->type >= DEFINE && t->type <= _FALSE);
}

static INLINE bool is_lparen(Token *t)
{
   return t->type == LEFT_PAREN;
}

static INLINE bool is_rparen(Token *t)
{
   return t->type == RIGHT_PAREN;
}

static INLINE bool is_comma(Token *t)
{
   return t->type == COMMA;
}

static INLINE bool is_newline(Token *t)
{
   return t->type == NEWLINE;
}

/*
   sets of tokens implemented as linked lists

   used to implement 'hide sets' for recursion prevention
*/

typedef struct _TokenList {
   Token *token;

   struct _TokenList *next;
} TokenList;

extern TokenList *glsl_tokenlist_construct(Token *token, TokenList *next);
extern TokenList *glsl_tokenlist_intersect(TokenList *hs0, TokenList *hs1);
extern TokenList *glsl_tokenlist_union(TokenList *hs0, TokenList *hs1);

extern bool glsl_tokenlist_equals(TokenList *t1, TokenList *t2);

extern bool glsl_tokenlist_contains(TokenList *hs, Token *t);

extern int glsl_tokenlist_length(TokenList *hs);

/*
   sequences of tokens with associated hide sets implemented
   as linked lists
*/

typedef struct _TokenSeq {
   Token *token;

   struct _TokenList *hide;
   struct _TokenSeq *next;
} TokenSeq;

extern TokenSeq *glsl_tokenseq_construct(Token *token, TokenList *hide, TokenSeq *next);

extern bool glsl_tokenseq_equals(TokenSeq *t1, TokenSeq *t2);

extern TokenSeq *glsl_tokenseq_destructive_reverse(TokenSeq *t, TokenSeq *p);

typedef struct _TokenSeqList {
   TokenSeq *seq;

   struct _TokenSeqList *next;
} TokenSeqList;

extern TokenSeqList *glsl_tokenseqlist_construct(TokenSeq *seq, TokenSeqList *next);

#ifdef TOKEN_DEBUG
extern void glsl_token_malloc_init(void);
extern void glsl_token_malloc_print(void);

extern void glsl_token_dump(Token *token);
extern void glsl_tokenlist_dump(TokenList *list, char *sep);
extern void glsl_tokenseq_dump(TokenSeq *seq, char *sep);
#endif

#endif