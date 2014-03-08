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

#include "middleware/khronos/glsl/prepro/glsl_prepro_eval.h"
#include "middleware/khronos/glsl/prepro/glsl_prepro_directive.h"

#include "middleware/khronos/glsl/glsl_errors.h"
#include "middleware/khronos/glsl/glsl_stack.h"

#include <string.h>
#include <stdlib.h>

extern int yyfile;
extern int yyline;

/*
   simple recursive-descent parser for preprocessor constant expressions

   0 (highest) parenthetical grouping     ( )                  NA
   1 unary                                defined + - ~ !      Right to Left
   2 multiplicative                       * / %                Left to Right
   3 additive                             + -                  Left to Right
   4 bit-wise shift                       << >>                Left to Right
   5 relational                           < > <= >=            Left to Right
   6 equality                             == !=                Left to Right
   7 bit-wise and                         &                    Left to Right
   8 bit-wise exclusive or                ^                    Left to Right
   9 bit-wise inclusive or                |                    Left to Right
   10 logical and                         &&                   Left to Right
   11 (lowest) logical inclusive or       ||                   Left to Right   
*/

static TokenSeq *seq;

static void check_seq(void)
{
   if (!seq) 
      glsl_compile_error(ERROR_PREPROCESSOR, 1, g_LineNumber, "unexpected end of line in constant expression");
}

static void expect_rparen(void)
{
   check_seq();

   if (seq->token->type == RIGHT_PAREN)
      seq = seq->next;
   else
      glsl_compile_error(ERROR_PREPROCESSOR, 1, g_LineNumber, "mismatched parenthesis in constant expression");
}

static int eval_1(void);
static int eval_2(void);
static int eval_3(void);
static int eval_4(void);
static int eval_5(void);
static int eval_6(void);
static int eval_7(void);
static int eval_8(void);
static int eval_9(void);
static int eval_10(void);
static int eval_11(void);

static int eval_1(void)
{
   check_seq();

   switch (seq->token->type) {
   case IDENTIFIER:
      seq = seq->next;

      return 0;
   case PPNUMBERI:
   {
      int res = (int)seq->token->data.i;

      seq = seq->next;

      return res;
   }
   case PPNUMBERF:
   case PPNUMBERU:
      glsl_compile_error(ERROR_PREPROCESSOR, 1, g_LineNumber, "invalid integer constant");
      return 0;
   case PLUS:
      seq = seq->next;

      return +eval_1();
   case DASH:
      seq = seq->next;

      return -eval_1();
   case TILDE:
      seq = seq->next;

      return ~eval_1();
   case BANG:
      seq = seq->next;

      return !eval_1();
   case LEFT_PAREN:
   {
      int res;

      seq = seq->next;

      STACK_CHECK();

      res = eval_11();

      expect_rparen();

      return res;
   }
   default:
      glsl_compile_error(ERROR_PREPROCESSOR, 1, g_LineNumber, "unexpected token in constant expression");
      return 0;
   }
}

static int eval_2(void)
{
   int res = eval_1();

   while (seq)
      switch (seq->token->type) {
      case STAR:
         seq = seq->next;

         res *= eval_1();
         break;
      case SLASH:
      {
         int rhs;

         seq = seq->next;

         rhs = eval_1();

         if (rhs)
            res /= rhs;
         else 
            glsl_compile_error(ERROR_PREPROCESSOR, 1, g_LineNumber, "divide by zero in constant expression");

         break;
      }
      case PERCENT:
      {
         int rhs;

         seq = seq->next;

         rhs = eval_1();

         if (rhs)
            res %= rhs;
         else 
            glsl_compile_error(ERROR_PREPROCESSOR, 1, g_LineNumber, "divide by zero in constant expression");

         break;
      }
      default:
         return res;
      }

   return res;
}

static int eval_3(void)
{
   int res = eval_2();

   while (seq)
      switch (seq->token->type) {
      case PLUS:
         seq = seq->next;

         res += eval_2();
         break;
      case DASH:
         seq = seq->next;

         res -= eval_2();
         break;
      default:
         return res;
      }

   return res;
}

static int eval_4(void)
{
   int res = eval_3();
   int rhs;

   while (seq)
      switch (seq->token->type) {
      case LEFT_OP:
         seq = seq->next;

         rhs = eval_3();

         if (rhs < 32)
            res <<= rhs;
         else
            res = 0;
         break;
      case RIGHT_OP:
         seq = seq->next;

         rhs = eval_3();

         if (rhs < 32)
            res >>= rhs;
         else
            if (res < 0)
               res = -1;
            else
               res = 0;
         break;
      default:
         return res;
      }

   return res;
}

static int eval_5(void)
{
   int res = eval_4();

   while (seq)
      switch (seq->token->type) {
      case LEFT_ANGLE:
         seq = seq->next;

         res = res < eval_4();
         break;
      case RIGHT_ANGLE:
         seq = seq->next;

         res = res > eval_4();
         break;
      case LE_OP:
         seq = seq->next;

         res = res <= eval_4();
         break;
      case GE_OP:
         seq = seq->next;

         res = res >= eval_4();
         break;
      default:
         return res;
      }

   return res;
}

static int eval_6(void)
{
   int res = eval_5();

   while (seq)
      switch (seq->token->type) {
      case EQ_OP:
         seq = seq->next;

         res = res == eval_5();
         break;
      case NE_OP:
         seq = seq->next;

         res = res != eval_5();
         break;
      default:
         return res;
      }

   return res;
}

static int eval_7(void)
{
   int res = eval_6();

   while (seq)
      switch (seq->token->type) {
      case BITWISE_AND_OP:
         seq = seq->next;

         res &= eval_6();
         break;
      default:
         return res;
      }

   return res;
}

static int eval_8(void)
{
   int res = eval_7();

   while (seq)
      switch (seq->token->type) {
      case BITWISE_XOR_OP:
         seq = seq->next;

         res ^= eval_7();
         break;
      default:
         return res;
      }

   return res;
}

static int eval_9(void)
{
   int res = eval_8();

   while (seq)
      switch (seq->token->type) {
      case BITWISE_OR_OP:
         seq = seq->next;

         res |= eval_8();
         break;
      default:
         return res;
      }

   return res;
}

static int eval_10(void)
{
   int res = eval_9();

   while (seq)
      switch (seq->token->type) {
      case LOGICAL_AND_OP:
      {
         int rhs;

         seq = seq->next;

         rhs = eval_9();

         res = res && rhs;
         break;
      }
      default:
         return res;
      }

   return res;
}

static int eval_11(void)
{
   int res = eval_10();

   while (seq)
      switch (seq->token->type) {
      case LOGICAL_OR_OP:
      {
         int rhs;

         seq = seq->next;

         rhs = eval_10();

         res = res || rhs;
         break;
      }
      default:
         return res;
      }

   return res;
}

void glsl_eval_set_sequence(TokenSeq *_seq)
{
   seq = _seq;
}

bool glsl_eval_has_sequence(void)
{
   return seq != NULL;
}

int glsl_eval_evaluate(void)
{
   return eval_11();
}
