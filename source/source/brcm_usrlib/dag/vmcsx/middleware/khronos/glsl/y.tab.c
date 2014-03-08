/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     IDENTIFIER = 512,
     PPNUMBERI = 513,
     PPNUMBERF = 514,
     PPNUMBERU = 515,
     UNKNOWN = 516,
     HASH = 517,
     WHITESPACE = 518,
     NEWLINE = 519,
     LEFT_OP = 520,
     RIGHT_OP = 521,
     INC_OP = 522,
     DEC_OP = 523,
     LE_OP = 524,
     GE_OP = 525,
     EQ_OP = 526,
     NE_OP = 527,
     LOGICAL_AND_OP = 528,
     LOGICAL_OR_OP = 529,
     LOGICAL_XOR_OP = 530,
     MUL_ASSIGN = 531,
     DIV_ASSIGN = 532,
     ADD_ASSIGN = 533,
     MOD_ASSIGN = 534,
     LEFT_ASSIGN = 535,
     RIGHT_ASSIGN = 536,
     AND_ASSIGN = 537,
     XOR_ASSIGN = 538,
     OR_ASSIGN = 539,
     SUB_ASSIGN = 540,
     LEFT_PAREN = 541,
     RIGHT_PAREN = 542,
     LEFT_BRACKET = 543,
     RIGHT_BRACKET = 544,
     LEFT_BRACE = 545,
     RIGHT_BRACE = 546,
     DOT = 547,
     COMMA = 548,
     COLON = 549,
     EQUAL = 550,
     SEMICOLON = 551,
     BANG = 552,
     DASH = 553,
     TILDE = 554,
     PLUS = 555,
     STAR = 556,
     SLASH = 557,
     PERCENT = 558,
     LEFT_ANGLE = 559,
     RIGHT_ANGLE = 560,
     BITWISE_OR_OP = 561,
     BITWISE_XOR_OP = 562,
     BITWISE_AND_OP = 563,
     QUESTION = 564,
     INTRINSIC_TEXTURE_2D_BIAS = 565,
     INTRINSIC_TEXTURE_2D_LOD = 566,
     INTRINSIC_TEXTURE_CUBE_BIAS = 567,
     INTRINSIC_TEXTURE_CUBE_LOD = 568,
     INTRINSIC_RSQRT = 569,
     INTRINSIC_RCP = 570,
     INTRINSIC_LOG2 = 571,
     INTRINSIC_EXP2 = 572,
     INTRINSIC_CEIL = 573,
     INTRINSIC_FLOOR = 574,
     INTRINSIC_SIGN = 575,
     INTRINSIC_TRUNC = 576,
     INTRINSIC_NEAREST = 577,
     INTRINSIC_MIN = 578,
     INTRINSIC_MAX = 579,
     DEFINE = 580,
     UNDEF = 581,
     IFDEF = 582,
     IFNDEF = 583,
     ELIF = 584,
     ENDIF = 585,
     ERROR = 586,
     PRAGMA = 587,
     EXTENSION = 588,
     VERSION = 589,
     LINE = 590,
     ALL = 591,
     REQUIRE = 592,
     ENABLE = 593,
     WARN = 594,
     DISABLE = 595,
     ATTRIBUTE = 596,
     CONST = 597,
     BOOL = 598,
     FLOAT = 599,
     _INT = 600,
     BREAK = 601,
     CONTINUE = 602,
     DO = 603,
     ELSE = 604,
     FOR = 605,
     IF = 606,
     DISCARD = 607,
     RETURN = 608,
     BVEC2 = 609,
     BVEC3 = 610,
     BVEC4 = 611,
     IVEC2 = 612,
     IVEC3 = 613,
     IVEC4 = 614,
     VEC2 = 615,
     VEC3 = 616,
     VEC4 = 617,
     MAT2 = 618,
     MAT3 = 619,
     MAT4 = 620,
     IN = 621,
     OUT = 622,
     INOUT = 623,
     UNIFORM = 624,
     VARYING = 625,
     SAMPLER2D = 626,
     SAMPLERCUBE = 627,
     STRUCT = 628,
     _VOID = 629,
     WHILE = 630,
     INVARIANT = 631,
     HIGH_PRECISION = 632,
     MEDIUM_PRECISION = 633,
     LOW_PRECISION = 634,
     PRECISION = 635,
     ASM = 636,
     CLASS = 637,
     UNION = 638,
     ENUM = 639,
     TYPEDEF = 640,
     TEMPLATE = 641,
     THIS = 642,
     PACKED = 643,
     GOTO = 644,
     SWITCH = 645,
     DEFAULT = 646,
     _INLINE = 647,
     NOINLINE = 648,
     VOLATILE = 649,
     PUBLIC = 650,
     STATIC = 651,
     EXTERN = 652,
     EXTERNAL = 653,
     INTERFACE = 654,
     FLAT = 655,
     LONG = 656,
     SHORT = 657,
     DOUBLE = 658,
     HALF = 659,
     FIXED = 660,
     _UNSIGNED = 661,
     SUPERP = 662,
     INPUT = 663,
     OUTPUT = 664,
     HVEC2 = 665,
     HVEC3 = 666,
     HVEC4 = 667,
     DVEC2 = 668,
     DVEC3 = 669,
     DVEC4 = 670,
     FVEC2 = 671,
     FVEC3 = 672,
     FVEC4 = 673,
     SAMPLER1D = 674,
     SAMPLER3D = 675,
     SAMPLER1DSHADOW = 676,
     SAMPLER2DSHADOW = 677,
     SAMPLER2DRECT = 678,
     SAMPLER3DRECT = 679,
     SAMPLER2DRECTSHADOW = 680,
     SIZEOF = 681,
     CAST = 682,
     NAMESPACE = 683,
     USING = 684,
     _TRUE = 685,
     _FALSE = 686,
     CANDIDATE_TYPE_NAME = 687
   };
#endif




/* Copy the first part of user declarations.  */


   #include "middleware/khronos/glsl/glsl_common.h"

   #include <stdlib.h>
   #include <stdio.h>
   #include <string.h>
   
   #include "middleware/khronos/glsl/glsl_symbols.h"
   #include "middleware/khronos/glsl/glsl_ast.h"
   #include "middleware/khronos/glsl/glsl_dataflow.h"
   #include "middleware/khronos/glsl/glsl_trace.h"
   #include "middleware/khronos/glsl/glsl_errors.h"
   #include "middleware/khronos/glsl/glsl_intern.h"
   #include "middleware/khronos/glsl/glsl_globals.h"
   #include "middleware/khronos/glsl/glsl_builders.h"

   #include "middleware/khronos/glsl/prepro/glsl_prepro_expand.h"
   #include "middleware/khronos/glsl/prepro/glsl_prepro_directive.h"
   
   extern TokenData pptoken;
   
   extern void ppunput(int c);
   extern int pplex(void);
   
   void yyerror(char *);

   static TokenSeq *seq;

   static bool fast;

   int yylex(void)
   {
      TokenType type = (TokenType)0;
      TokenData data;
       
      /*
         fast path bypassing preprocessor
      */
       
      if (fast) {
         bool newline = false;
      
         do {
            type = (TokenType)pplex();
            data = pptoken;
            
            if (type == NEWLINE)
               newline = true;
         } while (type == WHITESPACE || type == NEWLINE);

         if (type == IDENTIFIER) {
            /*
               expand built-in macros
            */         
         
            if (!strcmp(data.s, "__LINE__")) {
               type = PPNUMBERI;
               data.i = g_LineNumber;               
            } else if (!strcmp(data.s, "__FILE__")) {
               type = PPNUMBERI;
               data.i = g_FileNumber;               
            } else if (!strcmp(data.s, "__VERSION__")) {
               type = PPNUMBERI;
               data.i = 100;               
            } else if (!strcmp(data.s, "GL_ES")) {
               type = PPNUMBERI;
               data.i = 1;               
            } else if (!strcmp(data.s, "GL_FRAGMENT_PRECISION_HIGH")) {
               type = PPNUMBERI;
               data.i = 1;               
            }
         }

         if (newline && type == HASH)
            fast = false;
         else
            glsl_directive_disallow_version();
      }

      /*
         slow path via preprocessor
      */
      
      if (!fast) {
         if (!seq)
            seq = glsl_expand(NULL, false);
      
         if (seq) {
            type = seq->token->type;
            data = seq->token->data;
            
            seq = seq->next;
         } else
            return 0;
      }
         
      /*
         detect uses of reserved keywords (anything from the list, or anything containing '__')
      */

      if ((type >= ASM && type <= USING) || (type == IDENTIFIER && strstr(data.s, "__")))
         glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL);

      if (type == UNKNOWN)            
	      glsl_compile_error(ERROR_UNKNOWN, 0, g_LineNumber, "unknown character");

      /*
         detokenize tokens which were meaningful to the preprocessor
      */

      if (type >= DEFINE && type <= DISABLE)
         type = IDENTIFIER;

      /*
         the idea here is to return CANDIDATE_TYPE_NAME if it could be a type,
         and otherwise to return IDENTIFIER
      */            

      switch (type) {
      case IDENTIFIER:
      {
         Symbol *sym = glsl_symbol_table_lookup(g_SymbolTable, data.s, true);

         yylval.lookup.symbol = sym;
         yylval.lookup.name = data.s;

         if (sym)
            switch (sym->flavour) {
            case SYMBOL_TYPE:
               type = CANDIDATE_TYPE_NAME;
               break;
            case SYMBOL_VAR_INSTANCE:
            case SYMBOL_PARAM_INSTANCE:
            case SYMBOL_FUNCTION_INSTANCE:
               break;
            default:
               /*
                  we shouldn't see anything else in the main symbol table
               */                     
               UNREACHABLE();
               break;
            }
         break;
      }
      case PPNUMBERI:
         yylval.i = data.i;
         break;
      case PPNUMBERF:
         yylval.f = data.f;
         break;
      case PPNUMBERU:
         yylval.s = data.s;
         break;
      default:
         break;
      }

      return type;
   }
   
   // This allows yyparse to take an argument of type void* with the given name.
   #define YYPARSE_PARAM top_level_statement
   
   #define YYMALLOC yymalloc
   #define YYFREE yyfree
   
   void *yymalloc(size_t bytes)
   {
      return glsl_fastmem_malloc(bytes, true);
   }
   
   void yyfree(void *ptr)
   {
      UNUSED(ptr);
   }


   void glsl_init_parser(void)
   {
      g_DataflowSources = glsl_dataflow_sources_new();
      g_SymbolTable = glsl_symbol_table_populate(glsl_symbol_table_new());
      g_InGlobalScope = true;

      glsl_sb_invalidate(&g_StringBuilder);
      
      g_StructBuilderMembers = NULL;
      g_FunctionBuilderParams = NULL;

      g_NextAnonParam = 0;
      
      seq = NULL;
      
      fast = true;
   }
   
   static const char *generate_anon_param(void)
   {
      char c[16];
      
      sprintf(c, "$$anon%d\n", g_NextAnonParam++);
      
      return glsl_intern(c, true);
   }


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)

typedef union YYSTYPE {
	const_int i; const_float f; const char* s;
   struct { const char* name; Symbol* symbol; } lookup;
   SymbolType* type;
   Symbol* symbol;
   CallContext call_context;
   Expr* expr; ExprFlavour expr_flavour;
   Statement* statement;
   StatementChain* statement_chain;
   struct { Statement* a; Statement* b; } statement2;
} YYSTYPE;
/* Line 191 of yacc.c.  */

# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */


#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T) && (defined (__STDC__) || defined (__cplusplus))
# include <stddef.h> /* INFRINGES ON USER NAME SPACE */
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if defined (__STDC__) || defined (__cplusplus)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     define YYINCLUDED_STDLIB_H
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2005 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM ((YYSIZE_T) -1)
#  endif
#  ifdef __cplusplus
extern "C" {
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if (! defined (malloc) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if (! defined (free) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifdef __cplusplus
}
#  endif
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  71
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1473

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  179
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  79
/* YYNRULES -- Number of rules. */
#define YYNRULES  235
/* YYNRULES -- Number of states. */
#define YYNSTATES  374

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   688

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,   158,   159,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,     2
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    19,
      21,    26,    28,    32,    35,    38,    40,    42,    45,    48,
      51,    53,    56,    60,    63,    65,    67,    69,    71,    73,
      75,    77,    79,    81,    83,    85,    87,    89,    91,    93,
      95,    97,    99,   101,   103,   105,   107,   109,   111,   113,
     115,   117,   119,   121,   123,   125,   127,   129,   131,   133,
     136,   139,   142,   145,   148,   151,   153,   157,   161,   165,
     167,   171,   175,   177,   181,   185,   187,   191,   195,   199,
     203,   205,   209,   213,   215,   219,   221,   225,   227,   231,
     233,   237,   239,   243,   245,   249,   251,   257,   259,   263,
     267,   269,   271,   273,   275,   277,   279,   281,   283,   285,
     287,   289,   293,   295,   298,   301,   306,   309,   311,   313,
     316,   320,   324,   331,   335,   342,   345,   351,   354,   356,
     357,   359,   361,   363,   365,   369,   376,   382,   384,   387,
     393,   398,   401,   403,   406,   408,   410,   412,   415,   417,
     419,   422,   424,   426,   428,   430,   432,   434,   436,   438,
     440,   442,   444,   446,   448,   450,   452,   454,   456,   458,
     460,   462,   464,   466,   468,   469,   476,   477,   483,   485,
     488,   491,   493,   497,   504,   507,   513,   515,   517,   519,
     521,   523,   525,   527,   529,   531,   534,   535,   540,   542,
     544,   547,   551,   553,   556,   558,   561,   567,   571,   573,
     575,   580,   581,   588,   596,   597,   608,   610,   613,   615,
     617,   622,   624,   626,   629,   632,   635,   639,   642,   644,
     647,   649,   651,   652,   656,   658
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     253,     0,    -1,     3,    -1,   180,    -1,     5,    -1,     4,
      -1,   176,    -1,   177,    -1,    32,   207,    33,    -1,   181,
      -1,   182,    34,   183,    35,    -1,   184,    -1,   182,    38,
     257,    -1,   182,    13,    -1,   182,    14,    -1,   207,    -1,
     185,    -1,   187,    33,    -1,   186,    33,    -1,   188,   120,
      -1,   188,    -1,   188,   205,    -1,   187,    39,   205,    -1,
     189,    32,    -1,   191,    -1,   190,    -1,     3,    -1,    56,
      -1,    57,    -1,    58,    -1,    59,    -1,    60,    -1,    61,
      -1,    62,    -1,    63,    -1,    64,    -1,    65,    -1,    66,
      -1,    67,    -1,    68,    -1,    69,    -1,    70,    -1,    90,
      -1,    91,    -1,    89,    -1,   106,    -1,   107,    -1,   108,
      -1,   100,    -1,   101,    -1,   102,    -1,   103,    -1,   104,
      -1,   105,    -1,   109,    -1,   110,    -1,   111,    -1,   178,
      -1,   182,    -1,    13,   192,    -1,    14,   192,    -1,    46,
     192,    -1,    44,   192,    -1,    43,   192,    -1,    45,   192,
      -1,   192,    -1,   193,    47,   192,    -1,   193,    48,   192,
      -1,   193,    49,   192,    -1,   193,    -1,   194,    46,   193,
      -1,   194,    44,   193,    -1,   194,    -1,   195,    11,   194,
      -1,   195,    12,   194,    -1,   195,    -1,   196,    50,   195,
      -1,   196,    51,   195,    -1,   196,    15,   195,    -1,   196,
      16,   195,    -1,   196,    -1,   197,    17,   196,    -1,   197,
      18,   196,    -1,   197,    -1,   198,    54,   197,    -1,   198,
      -1,   199,    53,   198,    -1,   199,    -1,   200,    52,   199,
      -1,   200,    -1,   201,    19,   200,    -1,   201,    -1,   202,
      21,   201,    -1,   202,    -1,   203,    20,   202,    -1,   203,
      -1,   203,    55,   207,    40,   205,    -1,   204,    -1,   192,
      41,   205,    -1,   192,   206,   205,    -1,    22,    -1,    23,
      -1,    24,    -1,    31,    -1,    25,    -1,    26,    -1,    27,
      -1,    28,    -1,    29,    -1,    30,    -1,   205,    -1,   207,
      39,   205,    -1,   204,    -1,   210,    42,    -1,   217,    42,
      -1,   126,   223,   222,    42,    -1,   211,    33,    -1,   213,
      -1,   212,    -1,   213,   214,    -1,   212,    39,   214,    -1,
     219,   257,    32,    -1,   219,    34,   208,    35,   257,    32,
      -1,   215,   221,   257,    -1,   215,   221,   257,    34,   208,
      35,    -1,   215,   221,    -1,   215,   221,    34,   208,    35,
      -1,   220,   216,    -1,   216,    -1,    -1,   112,    -1,   113,
      -1,   114,    -1,   218,    -1,   217,    39,   257,    -1,   217,
      39,   257,    34,   208,    35,    -1,   217,    39,   257,    41,
     231,    -1,   219,    -1,   219,   257,    -1,   219,   257,    34,
     208,    35,    -1,   219,   257,    41,   231,    -1,   122,   257,
      -1,   221,    -1,   220,   221,    -1,    88,    -1,    87,    -1,
     116,    -1,   122,   116,    -1,   115,    -1,   222,    -1,   223,
     222,    -1,   120,    -1,    90,    -1,    91,    -1,    89,    -1,
     106,    -1,   107,    -1,   108,    -1,   100,    -1,   101,    -1,
     102,    -1,   103,    -1,   104,    -1,   105,    -1,   109,    -1,
     110,    -1,   111,    -1,   117,    -1,   118,    -1,   224,    -1,
     178,    -1,   123,    -1,   124,    -1,   125,    -1,    -1,   119,
     225,   257,    36,   227,    37,    -1,    -1,   119,   226,    36,
     227,    37,    -1,   228,    -1,   227,   228,    -1,   229,    42,
      -1,   230,    -1,   229,    39,   257,    -1,   229,    39,   257,
      34,   208,    35,    -1,   221,   257,    -1,   221,   257,    34,
     208,    35,    -1,   205,    -1,   209,    -1,   235,    -1,   234,
      -1,   232,    -1,   240,    -1,   241,    -1,   244,    -1,   252,
      -1,    36,    37,    -1,    -1,    36,   236,   239,    37,    -1,
     238,    -1,   234,    -1,    36,    37,    -1,    36,   239,    37,
      -1,   233,    -1,   239,   233,    -1,    42,    -1,   207,    42,
      -1,    97,    32,   207,    33,   242,    -1,   233,    95,   233,
      -1,   233,    -1,   207,    -1,   219,   257,    41,   231,    -1,
      -1,   121,    32,   245,   243,    33,   237,    -1,    94,   233,
     121,    32,   207,    33,    42,    -1,    -1,    96,    32,   246,
     249,    42,   250,    42,   251,    33,   237,    -1,   248,    -1,
     223,   248,    -1,    90,    -1,    91,    -1,   247,   257,    41,
     231,    -1,   207,    -1,   207,    -1,    93,    42,    -1,    92,
      42,    -1,    99,    42,    -1,    99,   207,    42,    -1,    98,
      42,    -1,   254,    -1,   253,   254,    -1,   255,    -1,   209,
      -1,    -1,   210,   256,   238,    -1,     3,    -1,   178,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   484,   484,   488,   489,   490,   491,   492,   493,   497,
     498,   499,   500,   501,   502,   506,   510,   533,   534,   538,
     539,   543,   544,   548,   552,   553,   554,   559,   560,   561,
     562,   563,   564,   565,   566,   567,   568,   569,   570,   571,
     572,   573,   577,   578,   579,   580,   581,   582,   583,   584,
     585,   586,   587,   588,   589,   590,   591,   592,   596,   597,
     598,   599,   600,   601,   602,   609,   610,   611,   612,   616,
     617,   618,   622,   623,   624,   628,   629,   630,   631,   632,
     636,   637,   638,   642,   643,   647,   648,   652,   653,   657,
     658,   662,   663,   667,   668,   672,   673,   677,   678,   679,
     683,   684,   685,   686,   687,   688,   689,   690,   691,   692,
     696,   697,   701,   705,   709,   712,   720,   727,   728,   732,
     733,   737,   738,   742,   743,   744,   745,   749,   750,   754,
     755,   756,   757,   762,   765,   769,   773,   781,   785,   789,
     793,   797,   805,   806,   810,   811,   812,   813,   814,   818,
     819,   823,   824,   825,   826,   827,   828,   829,   830,   831,
     832,   833,   834,   835,   836,   837,   838,   839,   840,   841,
     842,   847,   848,   849,   855,   855,   856,   856,   860,   861,
     865,   869,   870,   871,   875,   876,   880,   884,   888,   889,
     895,   896,   897,   898,   899,   903,   904,   904,   908,   909,
     913,   914,   918,   919,   923,   924,   928,   932,   933,   939,
     940,   944,   944,   945,   946,   946,   950,   951,   955,   956,
     960,   969,   973,   977,   978,   979,   980,   981,   987,   993,
    1002,  1003,  1007,  1007,  1012,  1013
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "IDENTIFIER", "PPNUMBERI", "PPNUMBERF",
  "PPNUMBERU", "UNKNOWN", "HASH", "WHITESPACE", "NEWLINE", "LEFT_OP",
  "RIGHT_OP", "INC_OP", "DEC_OP", "LE_OP", "GE_OP", "EQ_OP", "NE_OP",
  "LOGICAL_AND_OP", "LOGICAL_OR_OP", "LOGICAL_XOR_OP", "MUL_ASSIGN",
  "DIV_ASSIGN", "ADD_ASSIGN", "MOD_ASSIGN", "LEFT_ASSIGN", "RIGHT_ASSIGN",
  "AND_ASSIGN", "XOR_ASSIGN", "OR_ASSIGN", "SUB_ASSIGN", "LEFT_PAREN",
  "RIGHT_PAREN", "LEFT_BRACKET", "RIGHT_BRACKET", "LEFT_BRACE",
  "RIGHT_BRACE", "DOT", "COMMA", "COLON", "EQUAL", "SEMICOLON", "BANG",
  "DASH", "TILDE", "PLUS", "STAR", "SLASH", "PERCENT", "LEFT_ANGLE",
  "RIGHT_ANGLE", "BITWISE_OR_OP", "BITWISE_XOR_OP", "BITWISE_AND_OP",
  "QUESTION", "INTRINSIC_TEXTURE_2D_BIAS", "INTRINSIC_TEXTURE_2D_LOD",
  "INTRINSIC_TEXTURE_CUBE_BIAS", "INTRINSIC_TEXTURE_CUBE_LOD",
  "INTRINSIC_RSQRT", "INTRINSIC_RCP", "INTRINSIC_LOG2", "INTRINSIC_EXP2",
  "INTRINSIC_CEIL", "INTRINSIC_FLOOR", "INTRINSIC_SIGN", "INTRINSIC_TRUNC",
  "INTRINSIC_NEAREST", "INTRINSIC_MIN", "INTRINSIC_MAX", "DEFINE", "UNDEF",
  "IFDEF", "IFNDEF", "ELIF", "ENDIF", "ERROR", "PRAGMA", "EXTENSION",
  "VERSION", "LINE", "ALL", "REQUIRE", "ENABLE", "WARN", "DISABLE",
  "ATTRIBUTE", "CONST", "BOOL", "FLOAT", "_INT", "BREAK", "CONTINUE", "DO",
  "ELSE", "FOR", "IF", "DISCARD", "RETURN", "BVEC2", "BVEC3", "BVEC4",
  "IVEC2", "IVEC3", "IVEC4", "VEC2", "VEC3", "VEC4", "MAT2", "MAT3",
  "MAT4", "IN", "OUT", "INOUT", "UNIFORM", "VARYING", "SAMPLER2D", 
  "SAMPLERCUBE", "STRUCT", "_VOID", "WHILE", "INVARIANT", "HIGH_PRECISION",
  "MEDIUM_PRECISION", "LOW_PRECISION", "PRECISION", "ASM", "CLASS",
  "UNION", "ENUM", "TYPEDEF", "TEMPLATE", "THIS", "PACKED", "GOTO",
  "SWITCH", "DEFAULT", "_INLINE", "NOINLINE", "VOLATILE", "PUBLIC",
  "STATIC", "EXTERN", "EXTERNAL", "INTERFACE", "FLAT", "LONG", "SHORT",
  "DOUBLE", "HALF", "FIXED", "_UNSIGNED", "SUPERP", "INPUT", "OUTPUT",
  "HVEC2", "HVEC3", "HVEC4", "DVEC2", "DVEC3", "DVEC4", "FVEC2", "FVEC3",
  "FVEC4", "SAMPLER1D", "SAMPLER3D", "SAMPLER1DSHADOW", "SAMPLER2DSHADOW",
  "SAMPLER2DRECT", "SAMPLER3DRECT", "SAMPLER2DRECTSHADOW", "SIZEOF",
  "CAST", "NAMESPACE", "USING", "_TRUE", "_FALSE", "CANDIDATE_TYPE_NAME",
  "$accept", "variable_identifier", "primary_expression",
  "postfix_expression", "integer_expression", "function_call",
  "function_call_generic", "function_call_header_no_parameters",
  "function_call_header_with_parameters", "function_call_header",
  "function_identifier", "intrinsic_identifier", "constructor_identifier",
  "unary_expression", "multiplicative_expression", "additive_expression",
  "shift_expression", "relational_expression", "equality_expression",
  "and_expression", "exclusive_or_expression", "inclusive_or_expression",
  "logical_and_expression", "logical_xor_expression",
  "logical_or_expression", "conditional_expression",
  "assignment_expression", "assignment_operator", "expression",
  "constant_expression", "declaration", "function_prototype",
  "function_declarator", "function_header_with_parameters",
  "function_header", "parameter_declaration",
  "type_and_parameter_qualifier", "parameter_qualifier",
  "init_declarator_list", "init_declarator", "fully_specified_type",
  "type_qualifier", "type_specifier", "type_specifier_no_prec",
  "precision_qualifier", "struct_specifier", "@1", "@2",
  "struct_declaration_list", "struct_declaration",
  "struct_declarator_list", "struct_declarator", "initializer",
  "declaration_statement", "statement", "simple_statement",
  "compound_statement", "@3", "statement_no_new_scope",
  "compound_statement_no_new_scope", "statement_list",
  "expression_statement", "selection_statement",
  "selection_rest_statement", "condition", "iteration_statement", "@4",
  "@5", "induction_type_specifier", "induction_type_specifier_no_prec",
  "for_init", "for_test", "for_loop", "jump_statement", "translation_unit",
  "external_declaration", "function_definition", "@6",
  "identifier_or_typename",  0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   688,   512,   513,   514,   515,   516,   517,   518,
     519,   520,   521,   522,   523,   524,   525,   526,   527,   528,
     529,   530,   531,   532,   533,   534,   535,   536,   537,   538,
     539,   540,   541,   542,   543,   544,   545,   546,   547,   548,
     549,   550,   551,   552,   553,   554,   555,   556,   557,   558,
     559,   560,   561,   562,   563,   564,   565,   566,   567,   568,
     569,   570,   571,   572,   573,   574,   575,   576,   577,   578,
     579,   580,   581,   582,   583,   584,   585,   586,   587,   588,
     589,   590,   591,   592,   593,   594,   595,   596,   597,   598,
     599,   600,   601,   602,   603,   604,   605,   606,   607,   608,
     609,   610,   611,   612,   613,   614,   615,   616,   617,   618,
     619,   620,   621,   622,   623,   624,   625,   626,   627,   628,
     629,   630,   631,   632,   633,   634,   635,   636,   637,   638,
     639,   640,   641,   642,   643,   644,   645,   646,   647,   648,
     649,   650,   651,   652,   653,   654,   655,   656,   657,   658,
     659,   660,   661,   662,   663,   664,   665,   666,   667,   668,
     669,   670,   671,   672,   673,   674,   675,   676,   677,   678,
     679,   680,   681,   682,   683,   684,   685,   686,   687
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned short int yyr1[] =
{
       0,   179,   180,   181,   181,   181,   181,   181,   181,   182,
     182,   182,   182,   182,   182,   183,   184,   185,   185,   186,
     186,   187,   187,   188,   189,   189,   189,   190,   190,   190,
     190,   190,   190,   190,   190,   190,   190,   190,   190,   190,
     190,   190,   191,   191,   191,   191,   191,   191,   191,   191,
     191,   191,   191,   191,   191,   191,   191,   191,   192,   192,
     192,   192,   192,   192,   192,   193,   193,   193,   193,   194,
     194,   194,   195,   195,   195,   196,   196,   196,   196,   196,
     197,   197,   197,   198,   198,   199,   199,   200,   200,   201,
     201,   202,   202,   203,   203,   204,   204,   205,   205,   205,
     206,   206,   206,   206,   206,   206,   206,   206,   206,   206,
     207,   207,   208,   209,   209,   209,   210,   211,   211,   212,
     212,   213,   213,   214,   214,   214,   214,   215,   215,   216,
     216,   216,   216,   217,   217,   217,   217,   218,   218,   218,
     218,   218,   219,   219,   220,   220,   220,   220,   220,   221,
     221,   222,   222,   222,   222,   222,   222,   222,   222,   222,
     222,   222,   222,   222,   222,   222,   222,   222,   222,   222,
     222,   223,   223,   223,   225,   224,   226,   224,   227,   227,
     228,   229,   229,   229,   230,   230,   231,   232,   233,   233,
     234,   234,   234,   234,   234,   235,   236,   235,   237,   237,
     238,   238,   239,   239,   240,   240,   241,   242,   242,   243,
     243,   245,   244,   244,   246,   244,   247,   247,   248,   248,
     249,   250,   251,   252,   252,   252,   252,   252,   253,   253,
     254,   254,   256,   255,   257,   257
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     3,     1,
       4,     1,     3,     2,     2,     1,     1,     2,     2,     2,
       1,     2,     3,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     2,     2,     2,     2,     1,     3,     3,     3,     1,
       3,     3,     1,     3,     3,     1,     3,     3,     3,     3,
       1,     3,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     1,     5,     1,     3,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     1,     2,     2,     4,     2,     1,     1,     2,
       3,     3,     6,     3,     6,     2,     5,     2,     1,     0,
       1,     1,     1,     1,     3,     6,     5,     1,     2,     5,
       4,     2,     1,     2,     1,     1,     1,     2,     1,     1,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     0,     6,     0,     5,     1,     2,
       2,     1,     3,     6,     2,     5,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     0,     4,     1,     1,
       2,     3,     1,     2,     1,     2,     5,     3,     1,     1,
       4,     0,     6,     7,     0,    10,     1,     2,     1,     1,
       4,     1,     1,     2,     2,     2,     3,     2,     1,     2,
       1,     1,     0,     3,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,   145,   144,   154,   152,   153,   158,   159,   160,   161,
     162,   163,   155,   156,   157,   164,   165,   166,   148,   146,
     167,   168,   174,   151,     0,   171,   172,   173,     0,   170,
     231,   232,     0,   118,   129,     0,   133,   137,     0,   142,
     149,     0,   169,     0,   228,   230,     0,     0,   234,   147,
     235,   141,     0,   113,     0,   116,   129,   130,   131,   132,
       0,   119,     0,   128,   129,     0,   114,     0,   138,   143,
     150,     1,   229,     0,     0,     0,     0,   233,   120,   125,
     127,   134,     2,     5,     4,     0,     0,     0,     0,     0,
       0,     0,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    44,    42,    43,
      48,    49,    50,    51,    52,    53,    45,    46,    47,    54,
      55,    56,     6,     7,    57,     3,     9,    58,    11,    16,
       0,     0,    20,     0,    25,    24,    65,    69,    72,    75,
      80,    83,    85,    87,    89,    91,    93,    95,   112,     0,
     121,     0,     0,     0,     0,     0,   178,     0,   181,   115,
     196,   200,   204,   154,   152,   153,     0,     0,     0,     0,
       0,     0,     0,   158,   159,   160,   161,   162,   163,   155,
     156,   157,   164,   165,   166,     0,   170,    65,    97,   110,
       0,   187,     0,   190,   202,   189,   188,     0,   191,   192,
     193,   194,     0,   123,     0,     0,    59,    60,     0,    63,
      62,    64,    61,    13,    14,     0,     0,    18,    17,     0,
      19,    21,    23,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   186,   140,     0,   184,   177,
     179,     0,   180,   195,     0,   224,   223,     0,   214,     0,
     227,   225,     0,   211,   100,   101,   102,   104,   105,   106,
     107,   108,   109,   103,     0,     0,     0,   205,   201,   203,
       0,     0,     0,   136,     8,     0,    15,    12,    22,    66,
      67,    68,    71,    70,    73,    74,    78,    79,    76,    77,
      81,    82,    84,    86,    88,    90,    92,    94,     0,     0,
     139,   175,     0,   182,     0,     0,     0,     0,   226,     0,
      98,    99,   111,   126,     0,   135,    10,     0,   122,     0,
       0,   197,     0,   218,   219,     0,     0,   216,     0,     0,
     209,     0,     0,   124,    96,   185,     0,     0,   217,     0,
       0,   208,   206,     0,     0,   183,     0,     0,   221,     0,
       0,     0,   199,   212,   198,   213,   220,     0,   207,   210,
     222,     0,     0,   215
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,   125,   126,   127,   285,   128,   129,   130,   131,   132,
     133,   134,   135,   187,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   188,   189,   275,   190,   149,
     191,   192,    32,    33,    34,    61,    62,    63,    35,    36,
      37,    38,    39,    40,    41,    42,    46,    47,   155,   156,
     157,   158,   246,   193,   194,   195,   196,   254,   363,   364,
     197,   198,   199,   352,   342,   200,   319,   316,   336,   337,
     338,   359,   371,   201,    43,    44,    45,    54,    51
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -343
static const short int yypact[] =
{
    1257,  -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,
    -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,
    -343,  -343,   -17,  -343,    29,  -343,  -343,  -343,    -9,  -343,
    -343,    -4,   -18,     1,    20,   -19,  -343,     2,   -34,  -343,
    -343,  1295,  -343,   205,  -343,  -343,    31,    27,  -343,  -343,
    -343,  -343,  1295,  -343,    42,  -343,    39,  -343,  -343,  -343,
      13,  -343,   -34,  -343,    52,    31,  -343,  1232,    70,  -343,
    -343,  -343,  -343,    46,   -34,    56,   329,  -343,  -343,    30,
    -343,   -20,    99,  -343,  -343,  1232,  1232,  1232,  1232,  1232,
    1232,  1232,  -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,
    -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,
    -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,
    -343,  -343,  -343,  -343,  -343,  -343,  -343,    67,  -343,  -343,
     105,    -2,  1072,   111,  -343,  -343,  -343,   120,   -22,   135,
      45,   155,   102,   117,   106,   159,   192,   -10,  -343,   182,
    -343,  1232,  1232,   -34,    31,   866,  -343,    23,  -343,  -343,
     187,  -343,  -343,   193,   194,   195,   177,   186,   701,   197,
     198,   190,  1152,   201,   204,   206,   208,   209,   210,   211,
     212,   213,   214,   216,   229,   230,   231,   228,  -343,  -343,
      86,  -343,    -4,  -343,  -343,  -343,  -343,   453,  -343,  -343,
    -343,  -343,  1232,   203,  1232,  1232,  -343,  -343,    64,  -343,
    -343,  -343,  -343,  -343,  -343,  1232,    31,  -343,  -343,  1232,
    -343,  -343,  -343,  1232,  1232,  1232,  1232,  1232,  1232,  1232,
    1232,  1232,  1232,  1232,  1232,  1232,  1232,  1232,  1232,  1232,
    1232,  1232,  1232,    31,   225,  -343,  -343,   989,   232,  -343,
    -343,    31,  -343,  -343,   701,  -343,  -343,   143,  -343,  1232,
    -343,  -343,    98,  -343,  -343,  -343,  -343,  -343,  -343,  -343,
    -343,  -343,  -343,  -343,  1232,  1232,  1232,  -343,  -343,  -343,
     233,  1232,   236,  -343,  -343,   237,   234,  -343,  -343,  -343,
    -343,  -343,   120,   120,   -22,   -22,   135,   135,   135,   135,
      45,    45,   155,   102,   117,   106,   159,   192,   137,   242,
    -343,  -343,  1232,   241,   577,   244,   -73,    73,  -343,   949,
    -343,  -343,  -343,  -343,   243,  -343,  -343,  1232,  -343,   245,
    1232,  -343,  1232,  -343,  -343,   113,    31,  -343,   235,   701,
     234,    31,   246,  -343,  -343,  -343,   248,    80,  -343,   240,
    1232,   189,  -343,   249,   825,  -343,   255,  1232,   234,   256,
     701,  1232,  -343,  -343,  -343,  -343,  -343,  1232,  -343,  -343,
     234,   252,   825,  -343
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,
    -343,  -343,  -343,   -42,   -16,   -13,  -109,   -12,    50,    54,
      49,    61,    62,    60,  -343,   -63,  -126,  -343,   -85,  -110,
      11,    16,  -343,  -343,  -343,   247,  -343,   253,  -343,  -343,
      -1,   -21,   -35,    58,   -27,  -343,  -343,  -343,   166,  -147,
    -343,  -343,  -198,  -343,  -139,  -342,  -343,  -343,   -46,   281,
      82,  -343,  -343,  -343,  -343,  -343,  -343,  -343,  -343,     3,
    -343,  -343,  -343,  -343,  -343,   294,  -343,  -343,   -37
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -177
static const short int yytable[] =
{
      68,    52,   208,    69,   148,    48,   221,   283,   250,    73,
     241,    30,   362,    64,   204,    55,    31,   333,   334,  -176,
      65,   205,   226,    66,   227,   136,   245,    79,    81,   257,
     362,   218,    48,    48,    48,    64,    67,   219,    53,   154,
      56,   244,   203,   206,   207,   242,   209,   210,   211,   212,
      25,    26,    27,  -117,    30,     3,     4,     5,   279,    31,
     230,   231,   251,    74,   202,   252,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    76,   245,
     213,   214,   153,    20,    21,    22,    23,   262,   148,    25,
      26,    27,   280,   288,   282,   232,   233,   284,   159,    70,
     250,   215,   150,   276,   151,   216,   339,     1,     2,   136,
      75,   152,   276,   356,    25,    26,    27,   248,   154,   276,
     154,   296,   297,   298,   299,   276,     1,     2,   277,    49,
     286,   -26,    57,    58,    59,    18,    19,   276,   217,   148,
     318,   148,    60,   222,    29,    49,   228,   229,   320,   321,
     322,    57,    58,    59,    18,    19,   236,   308,   238,   366,
     136,    60,   136,   369,    57,    58,    59,   223,   224,   225,
     237,   324,   234,   235,   317,   279,   276,   327,   239,   287,
      50,   289,   290,   291,   136,   136,   136,   136,   136,   136,
     136,   136,   136,   136,   136,   136,   136,   136,   136,   136,
     351,   344,   329,   333,   334,    71,   309,    50,    50,    50,
     292,   293,   154,   240,   313,   294,   295,   243,   148,   255,
     346,   368,   300,   301,   253,   -44,   -42,   -43,   256,   258,
     259,   245,   260,   -48,   340,   245,   -49,   281,   -50,   136,
     -51,   -52,   -53,   -45,   -46,   -47,   -54,   347,   -55,   148,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     310,   -56,   263,   -57,   315,   358,   312,   148,   323,   274,
     136,   325,   326,   276,   328,   330,   332,   350,   343,   354,
     345,   357,   370,   355,   360,   372,   302,   304,   136,   335,
     361,   303,     1,     2,     3,     4,     5,   365,   367,   349,
     305,   307,   306,    78,   353,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    80,   341,   247,
      18,    19,    20,    21,    22,    23,   373,    24,    25,    26,
      27,    28,    82,    83,    84,    77,   314,    72,   348,     0,
       0,     0,    85,    86,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    87,     0,     0,     0,   160,   161,     0,     0,     0,
       0,   162,    88,    89,    90,    91,     0,     0,     0,     0,
       0,     0,     0,    29,     0,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     1,     2,   163,   164,
     165,   166,   167,   168,     0,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,   182,   183,
     184,     0,     0,     0,    18,    19,    20,    21,    22,    23,
     185,    24,    25,    26,    27,    28,    82,    83,    84,     0,
       0,     0,     0,     0,     0,     0,    85,    86,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    87,     0,     0,     0,   160,
     278,     0,     0,     0,     0,   162,    88,    89,    90,    91,
       0,     0,     0,     0,     0,   122,   123,   186,     0,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       1,     2,   163,   164,   165,   166,   167,   168,     0,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,     0,     0,     0,    18,    19,
      20,    21,    22,    23,   185,    24,    25,    26,    27,    28,
      82,    83,    84,     0,     0,     0,     0,     0,     0,     0,
      85,    86,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    87,
       0,     0,     0,   160,   331,     0,     0,     0,     0,   162,
      88,    89,    90,    91,     0,     0,     0,     0,     0,   122,
     123,   186,     0,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     1,     2,   163,   164,   165,   166,
     167,   168,     0,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,     0,
       0,     0,    18,    19,    20,    21,    22,    23,   185,    24,
      25,    26,    27,    28,    82,    83,    84,     0,     0,     0,
       0,     0,     0,     0,    85,    86,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    87,     0,     0,     0,   160,     0,     0,
       0,     0,     0,   162,    88,    89,    90,    91,     0,     0,
       0,     0,     0,   122,   123,   186,     0,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     1,     2,
     163,   164,   165,   166,   167,   168,     0,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   180,   181,
     182,   183,   184,     0,     0,     0,    18,    19,    20,    21,
      22,    23,   185,    24,    25,    26,    27,    28,    82,    83,
      84,     0,     0,     0,     0,     0,     0,     0,    85,    86,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    87,     0,     0,
       0,    76,     0,     0,     0,     0,     0,   162,    88,    89,
      90,    91,     0,     0,     0,     0,     0,   122,   123,   186,
       0,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,     0,     0,     0,     0,
       0,     0,     0,   249,     0,     0,     0,     0,     0,     0,
       0,     0,     1,     2,   163,   164,   165,   166,   167,   168,
       0,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,     0,     0,     0,
      18,    19,    20,    21,    22,    23,   185,    24,    25,    26,
      27,    28,    82,    83,    84,     3,     4,     5,     0,     0,
       0,     0,    85,    86,     0,     0,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,     0,     0,
       0,    87,     0,    20,    21,    22,    23,     0,     0,    25,
      26,    27,    88,    89,    90,    91,     0,     0,     0,     0,
       0,   122,   123,   186,     0,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
       0,     0,     0,     0,     0,     0,   311,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     1,     2,   163,   164,
     165,     0,     0,     0,    29,     0,     0,     0,     0,   173,
     174,   175,   176,   177,   178,   179,   180,   181,   182,   183,
     184,     0,     0,     0,    18,    19,    20,    21,    22,    23,
       0,    60,    25,    26,    27,    82,    83,    84,     3,     4,
       5,     0,     0,     0,     0,    85,    86,     0,     0,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,     0,     0,     0,    87,     0,    20,    21,    22,    23,
       0,     0,    25,    26,    27,    88,    89,    90,    91,     0,
       0,     0,     0,     0,     0,   122,   123,   186,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    82,    83,    84,     0,     0,
       0,   107,   108,   109,     0,    85,    86,    29,     0,     0,
       0,     0,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,    87,     0,     0,     0,     0,     0,
       0,     0,   220,     0,   261,    88,    89,    90,    91,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    82,    83,    84,     0,     0,
       0,   107,   108,   109,     0,    85,    86,     0,   122,   123,
     124,     0,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,    87,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    88,    89,    90,    91,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,   108,   109,     0,     0,     0,     0,   122,   123,
     124,     0,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,     1,     2,     3,     4,     5,     0,
       0,     0,     0,     0,     0,     0,     0,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,     0,
       0,     0,    18,    19,    20,    21,    22,    23,     0,    24,
      25,    26,    27,    28,     3,     4,     5,     0,     0,     0,
       0,     0,     0,     0,     0,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,     0,   122,   123,
     124,     0,    20,    21,    22,    23,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    29,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    29
};

static const short int yycheck[] =
{
      37,    28,    87,    38,    67,     3,   132,   205,   155,    46,
      20,     0,   354,    34,    34,    33,     0,    90,    91,    36,
      39,    41,    44,    42,    46,    67,   152,    62,    65,   168,
     372,    33,     3,     3,     3,    56,    34,    39,    42,    74,
      39,   151,    79,    85,    86,    55,    88,    89,    90,    91,
     123,   124,   125,    33,    43,    89,    90,    91,   197,    43,
      15,    16,    39,    36,    34,    42,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,    36,   205,
      13,    14,    36,   117,   118,   119,   120,   172,   151,   123,
     124,   125,   202,   219,   204,    50,    51,    33,    42,    41,
     247,    34,    32,    39,    34,    38,    33,    87,    88,   151,
      52,    41,    39,    33,   123,   124,   125,   154,   153,    39,
     155,   230,   231,   232,   233,    39,    87,    88,    42,   116,
     215,    32,   112,   113,   114,   115,   116,    39,    33,   202,
      42,   204,   122,    32,   178,   116,    11,    12,   274,   275,
     276,   112,   113,   114,   115,   116,    54,   242,    52,   357,
     202,   122,   204,   361,   112,   113,   114,    47,    48,    49,
      53,   281,    17,    18,   259,   314,    39,    40,    19,   216,
     178,   223,   224,   225,   226,   227,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     339,   327,   312,    90,    91,     0,   243,   178,   178,   178,
     226,   227,   247,    21,   251,   228,   229,    35,   281,    42,
     330,   360,   234,   235,    37,    32,    32,    32,    42,    32,
      32,   357,    42,    32,   319,   361,    32,    34,    32,   281,
      32,    32,    32,    32,    32,    32,    32,   332,    32,   312,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      35,    32,    32,    32,   121,   350,    34,   330,    35,    41,
     312,    35,    35,    39,    32,    34,    32,    42,    35,    33,
      35,    41,   367,    35,    95,    33,   236,   238,   330,   316,
      41,   237,    87,    88,    89,    90,    91,    42,    42,   336,
     239,   241,   240,    56,   341,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,    64,   319,   153,
     115,   116,   117,   118,   119,   120,   372,   122,   123,   124,
     125,   126,     3,     4,     5,    54,   254,    43,   335,    -1,
      -1,    -1,    13,    14,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    32,    -1,    -1,    -1,    36,    37,    -1,    -1,    -1,
      -1,    42,    43,    44,    45,    46,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   178,    -1,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    87,    88,    89,    90,
      91,    92,    93,    94,    -1,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,    -1,    -1,    -1,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,     3,     4,     5,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    13,    14,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,    36,
      37,    -1,    -1,    -1,    -1,    42,    43,    44,    45,    46,
      -1,    -1,    -1,    -1,    -1,   176,   177,   178,    -1,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      87,    88,    89,    90,    91,    92,    93,    94,    -1,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,    -1,    -1,    -1,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
       3,     4,     5,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      13,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      -1,    -1,    -1,    36,    37,    -1,    -1,    -1,    -1,    42,
      43,    44,    45,    46,    -1,    -1,    -1,    -1,    -1,   176,
     177,   178,    -1,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    87,    88,    89,    90,    91,    92,
      93,    94,    -1,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,    -1,
      -1,    -1,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,     3,     4,     5,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    13,    14,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    32,    -1,    -1,    -1,    36,    -1,    -1,
      -1,    -1,    -1,    42,    43,    44,    45,    46,    -1,    -1,
      -1,    -1,    -1,   176,   177,   178,    -1,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,    88,
      89,    90,    91,    92,    93,    94,    -1,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,    -1,    -1,    -1,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,     3,     4,
       5,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    13,    14,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,    -1,    -1,
      -1,    36,    -1,    -1,    -1,    -1,    -1,    42,    43,    44,
      45,    46,    -1,    -1,    -1,    -1,    -1,   176,   177,   178,
      -1,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    37,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    87,    88,    89,    90,    91,    92,    93,    94,
      -1,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,    -1,    -1,    -1,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,     3,     4,     5,    89,    90,    91,    -1,    -1,
      -1,    -1,    13,    14,    -1,    -1,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,    -1,    -1,
      -1,    32,    -1,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,    43,    44,    45,    46,    -1,    -1,    -1,    -1,
      -1,   176,   177,   178,    -1,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      -1,    -1,    -1,    -1,    -1,    -1,    37,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    87,    88,    89,    90,
      91,    -1,    -1,    -1,   178,    -1,    -1,    -1,    -1,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,    -1,    -1,    -1,   115,   116,   117,   118,   119,   120,
      -1,   122,   123,   124,   125,     3,     4,     5,    89,    90,
      91,    -1,    -1,    -1,    -1,    13,    14,    -1,    -1,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,    -1,    -1,    -1,    32,    -1,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,    43,    44,    45,    46,    -1,
      -1,    -1,    -1,    -1,    -1,   176,   177,   178,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,    -1,    -1,
      -1,    89,    90,    91,    -1,    13,    14,   178,    -1,    -1,
      -1,    -1,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,    32,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   120,    -1,    42,    43,    44,    45,    46,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,    -1,    -1,
      -1,    89,    90,    91,    -1,    13,    14,    -1,   176,   177,
     178,    -1,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,    32,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    43,    44,    45,    46,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    89,    90,    91,    -1,    -1,    -1,    -1,   176,   177,
     178,    -1,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,    87,    88,    89,    90,    91,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,    -1,
      -1,    -1,   115,   116,   117,   118,   119,   120,    -1,   122,
     123,   124,   125,   126,    89,    90,    91,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,    -1,   176,   177,
     178,    -1,   117,   118,   119,   120,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   178,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   178
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned short int yystos[] =
{
       0,    87,    88,    89,    90,    91,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   115,   116,
     117,   118,   119,   120,   122,   123,   124,   125,   126,   178,
     209,   210,   211,   212,   213,   217,   218,   219,   220,   221,
     222,   223,   224,   253,   254,   255,   225,   226,     3,   116,
     178,   257,   223,    42,   256,    33,    39,   112,   113,   114,
     122,   214,   215,   216,   220,    39,    42,    34,   257,   221,
     222,     0,   254,   257,    36,   222,    36,   238,   214,   221,
     216,   257,     3,     4,     5,    13,    14,    32,    43,    44,
      45,    46,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    89,    90,    91,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   176,   177,   178,   180,   181,   182,   184,   185,
     186,   187,   188,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,   201,   202,   203,   204,   208,
      32,    34,    41,    36,   221,   227,   228,   229,   230,    42,
      36,    37,    42,    89,    90,    91,    92,    93,    94,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   121,   178,   192,   204,   205,
     207,   209,   210,   232,   233,   234,   235,   239,   240,   241,
     244,   252,    34,   257,    34,    41,   192,   192,   207,   192,
     192,   192,   192,    13,    14,    34,    38,    33,    33,    39,
     120,   205,    32,    47,    48,    49,    44,    46,    11,    12,
      15,    16,    50,    51,    17,    18,    54,    53,    52,    19,
      21,    20,    55,    35,   208,   205,   231,   227,   257,    37,
     228,    39,    42,    37,   236,    42,    42,   233,    32,    32,
      42,    42,   207,    32,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    41,   206,    39,    42,    37,   233,
     208,    34,   208,   231,    33,   183,   207,   257,   205,   192,
     192,   192,   193,   193,   194,   194,   195,   195,   195,   195,
     196,   196,   197,   198,   199,   200,   201,   202,   207,   257,
      35,    37,    34,   257,   239,   121,   246,   207,    42,   245,
     205,   205,   205,    35,   208,    35,    35,    40,    32,   208,
      34,    37,    32,    90,    91,   223,   247,   248,   249,    33,
     207,   219,   243,    35,   205,    35,   208,   207,   248,   257,
      42,   233,   242,   257,    33,    35,    33,    41,   207,   250,
      95,    41,   234,   237,   238,    42,   231,    42,   233,   231,
     207,   251,    33,   237
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
              (Loc).first_line, (Loc).first_column,	\
              (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr,					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      size_t yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

#endif /* YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()
    ;
#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to look-ahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

    { (yyval.lookup) = (yyvsp[0].lookup); ;}
    break;

  case 3:

    { (yyval.expr) = glsl_expr_construct_instance((yyvsp[0].lookup).symbol); ;}
    break;

  case 4:

    { (yyval.expr) = glsl_expr_construct_value_float((yyvsp[0].f)); ;}
    break;

  case 5:

    { (yyval.expr) = glsl_expr_construct_value_int((yyvsp[0].i)); ;}
    break;

  case 6:

    { (yyval.expr) = glsl_expr_construct_value_bool(CONST_BOOL_TRUE); ;}
    break;

  case 7:

    { (yyval.expr) = glsl_expr_construct_value_bool(CONST_BOOL_FALSE); ;}
    break;

  case 8:

    { (yyval.expr) = (yyvsp[-1].expr); ;}
    break;

  case 9:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 10:

    { (yyval.expr) = glsl_expr_construct_subscript((yyvsp[-3].expr), (yyvsp[-1].expr)); ;}
    break;

  case 11:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 12:

    { (yyval.expr) = glsl_expr_construct_field_selector((yyvsp[-2].expr), (yyvsp[0].s)); ;}
    break;

  case 13:

    { (yyval.expr) = glsl_expr_construct_unary_op_arithmetic(EXPR_POST_INC, (yyvsp[-1].expr)); ;}
    break;

  case 14:

    { (yyval.expr) = glsl_expr_construct_unary_op_arithmetic(EXPR_POST_DEC, (yyvsp[-1].expr)); ;}
    break;

  case 15:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 16:

    {
                                 switch((yyvsp[0].call_context).flavour)
                                 {
                                    case CALL_CONTEXT_FUNCTION:
                                       (yyval.expr) = glsl_expr_construct_function_call((yyvsp[0].call_context).u.function.symbol, (yyvsp[0].call_context).args);
                                       break;
                                    case CALL_CONTEXT_PRIM_CONSTRUCTOR:
                                       (yyval.expr) = glsl_expr_construct_prim_constructor_call((yyvsp[0].call_context).u.prim_constructor.index, (yyvsp[0].call_context).args);
                                       break;
                                    case CALL_CONTEXT_TYPE_CONSTRUCTOR:
                                       (yyval.expr) = glsl_expr_construct_type_constructor_call((yyvsp[0].call_context).u.type_constructor.symbol->type, (yyvsp[0].call_context).args);
                                       break;
                                    case CALL_CONTEXT_INTRINSIC:
                                       (yyval.expr) = glsl_expr_construct_intrinsic((yyvsp[0].call_context).u.intrinsic.flavour, (yyvsp[0].call_context).args);
                                       break;
                                    default:
                                       UNREACHABLE();
                                       break;
                                 }
                              ;}
    break;

  case 17:

    { (yyval.call_context) = (yyvsp[-1].call_context); ;}
    break;

  case 18:

    { (yyval.call_context) = (yyvsp[-1].call_context); ;}
    break;

  case 19:

    { (yyval.call_context).args = glsl_expr_chain_create(); ;}
    break;

  case 20:

    { (yyval.call_context).args = glsl_expr_chain_create(); ;}
    break;

  case 21:

    { (yyval.call_context).args = glsl_expr_chain_append(glsl_expr_chain_create(), (yyvsp[0].expr)); ;}
    break;

  case 22:

    { (yyval.call_context).args = glsl_expr_chain_append((yyvsp[-2].call_context).args, (yyvsp[0].expr)); ;}
    break;

  case 23:

    { (yyval.call_context) = (yyvsp[-1].call_context); ;}
    break;

  case 24:

    { (yyval.call_context) = (yyvsp[0].call_context); ;}
    break;

  case 25:

    { (yyval.call_context) = (yyvsp[0].call_context); ;}
    break;

  case 26:

    { (yyval.call_context).flavour = CALL_CONTEXT_FUNCTION; (yyval.call_context).u.function.symbol = (yyvsp[0].lookup).symbol; ;}
    break;

  case 27:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_TEXTURE_2D_BIAS; ;}
    break;

  case 28:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_TEXTURE_2D_LOD; ;}
    break;

  case 29:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_TEXTURE_CUBE_BIAS; ;}
    break;

  case 30:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_TEXTURE_CUBE_LOD; ;}
    break;

  case 31:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_RSQRT; ;}
    break;

  case 32:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_RCP; ;}
    break;

  case 33:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_LOG2; ;}
    break;

  case 34:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_EXP2; ;}
    break;

  case 35:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_CEIL; ;}
    break;

  case 36:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_FLOOR; ;}
    break;

  case 37:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_SIGN; ;}
    break;

  case 38:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_TRUNC; ;}
    break;

  case 39:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_NEAREST; ;}
    break;

  case 40:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_MIN; ;}
    break;

  case 41:

    { (yyval.call_context).flavour = CALL_CONTEXT_INTRINSIC; (yyval.call_context).u.intrinsic.flavour = EXPR_INTRINSIC_MAX; ;}
    break;

  case 42:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_FLOAT; ;}
    break;

  case 43:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_INT; ;}
    break;

  case 44:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_BOOL; ;}
    break;

  case 45:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_VEC2; ;}
    break;

  case 46:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_VEC3; ;}
    break;

  case 47:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_VEC4; ;}
    break;

  case 48:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_BVEC2; ;}
    break;

  case 49:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_BVEC3; ;}
    break;

  case 50:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_BVEC4; ;}
    break;

  case 51:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_IVEC2; ;}
    break;

  case 52:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_IVEC3; ;}
    break;

  case 53:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_IVEC4; ;}
    break;

  case 54:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_MAT2; ;}
    break;

  case 55:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_MAT3; ;}
    break;

  case 56:

    { (yyval.call_context).flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; (yyval.call_context).u.prim_constructor.index = PRIM_MAT4; ;}
    break;

  case 57:

    { (yyval.call_context).flavour = CALL_CONTEXT_TYPE_CONSTRUCTOR; (yyval.call_context).u.type_constructor.symbol = (yyvsp[0].lookup).symbol; ;}
    break;

  case 58:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 59:

    { (yyval.expr) = glsl_expr_construct_unary_op_arithmetic(EXPR_PRE_INC, (yyvsp[0].expr)); ;}
    break;

  case 60:

    { (yyval.expr) = glsl_expr_construct_unary_op_arithmetic(EXPR_PRE_DEC, (yyvsp[0].expr)); ;}
    break;

  case 61:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 62:

    { (yyval.expr) = glsl_expr_construct_unary_op_arithmetic(EXPR_ARITH_NEGATE, (yyvsp[0].expr)); ;}
    break;

  case 63:

    { (yyval.expr) = glsl_expr_construct_unary_op_logical(EXPR_LOGICAL_NOT, (yyvsp[0].expr)); ;}
    break;

  case 64:

    { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); ;}
    break;

  case 65:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 66:

    { (yyval.expr) = glsl_expr_construct_binary_op_arithmetic(EXPR_MUL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 67:

    { (yyval.expr) = glsl_expr_construct_binary_op_arithmetic(EXPR_DIV, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 68:

    { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); ;}
    break;

  case 69:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 70:

    { (yyval.expr) = glsl_expr_construct_binary_op_arithmetic(EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 71:

    { (yyval.expr) = glsl_expr_construct_binary_op_arithmetic(EXPR_SUB, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 72:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 73:

    { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); ;}
    break;

  case 74:

    { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); ;}
    break;

  case 75:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 76:

    { (yyval.expr) = glsl_expr_construct_binary_op_relational(EXPR_LESS_THAN, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 77:

    { (yyval.expr) = glsl_expr_construct_binary_op_relational(EXPR_GREATER_THAN, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 78:

    { (yyval.expr) = glsl_expr_construct_binary_op_relational(EXPR_LESS_THAN_EQUAL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 79:

    { (yyval.expr) = glsl_expr_construct_binary_op_relational(EXPR_GREATER_THAN_EQUAL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 80:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 81:

    { (yyval.expr) = glsl_expr_construct_binary_op_equality(EXPR_EQUAL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 82:

    { (yyval.expr) = glsl_expr_construct_binary_op_equality(EXPR_NOT_EQUAL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 83:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 84:

    { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); ;}
    break;

  case 85:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 86:

    { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); ;}
    break;

  case 87:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 88:

    { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); ;}
    break;

  case 89:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 90:

    { (yyval.expr) = glsl_expr_construct_binary_op_logical(EXPR_LOGICAL_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 91:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 92:

    { (yyval.expr) = glsl_expr_construct_binary_op_logical(EXPR_LOGICAL_XOR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 93:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 94:

    { (yyval.expr) = glsl_expr_construct_binary_op_logical(EXPR_LOGICAL_OR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 95:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 96:

    { (yyval.expr) = glsl_expr_construct_cond_op((yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 97:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 98:

    { (yyval.expr) = glsl_expr_construct_assign_op((yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 99:

    { (yyval.expr) = glsl_expr_construct_assign_op((yyvsp[-2].expr), glsl_expr_construct_binary_op_arithmetic((yyvsp[-1].expr_flavour), (yyvsp[-2].expr), (yyvsp[0].expr))); ;}
    break;

  case 100:

    { (yyval.expr_flavour) = EXPR_MUL; ;}
    break;

  case 101:

    { (yyval.expr_flavour) = EXPR_DIV; ;}
    break;

  case 102:

    { (yyval.expr_flavour) = EXPR_ADD; ;}
    break;

  case 103:

    { (yyval.expr_flavour) = EXPR_SUB; ;}
    break;

  case 104:

    { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); ;}
    break;

  case 105:

    { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); ;}
    break;

  case 106:

    { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); ;}
    break;

  case 107:

    { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); ;}
    break;

  case 108:

    { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); ;}
    break;

  case 109:

    { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); ;}
    break;

  case 110:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 111:

    { (yyval.expr) = glsl_expr_construct_sequence((yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 112:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 113:

    { 
                                                                          // Don't store these in the AST.
                                                                          (yyval.statement) = glsl_statement_construct_decl_list(glsl_statement_chain_create());
                                                                       ;}
    break;

  case 114:

    {
                                                                          (yyval.statement) = glsl_statement_construct_decl_list((yyvsp[-1].statement_chain));
                                                                       ;}
    break;

  case 115:

    {
                                                                          // "Precision not supported" warning already issued.
                                                                          // Don't store these in the AST.
                                                                          (yyval.statement) = glsl_statement_construct_decl_list(glsl_statement_chain_create());
                                                                       ;}
    break;

  case 116:

    { glsl_build_function_type(); glsl_commit_singleton_function_declaration(g_FunctionBuilderName); (yyval.symbol) = g_LastInstance; ;}
    break;

  case 117:

    { /* Nothing to do here. */ ;}
    break;

  case 118:

    { /* Nothing to do here. */ ;}
    break;

  case 119:

    { /* Nothing to do here. */ ;}
    break;

  case 120:

    { /* Nothing to do here. */ ;}
    break;

  case 121:

    { g_FunctionBuilderReturnType = g_TypeBuilder; g_FunctionBuilderName = (yyvsp[-1].s); glsl_reinit_function_builder(); ;}
    break;

  case 122:

    { glsl_compile_error(ERROR_SEMANTIC, 41, g_LineNumber, NULL); ;}
    break;

  case 123:

    { glsl_commit_singleton_function_param((yyvsp[0].s)); ;}
    break;

  case 124:

    { glsl_commit_array_function_param((yyvsp[-3].s), (yyvsp[-1].expr)); ;}
    break;

  case 125:

    { glsl_commit_singleton_function_param(generate_anon_param()); ;}
    break;

  case 126:

    { glsl_commit_array_function_param(generate_anon_param(), (yyvsp[-1].expr)); ;}
    break;

  case 127:

    { /* Nothing to do here. */ ;}
    break;

  case 128:

    { g_TypeQual = TYPE_QUAL_DEFAULT; ;}
    break;

  case 129:

    { g_ParamQual = PARAM_QUAL_DEFAULT; ;}
    break;

  case 130:

    { g_ParamQual = PARAM_QUAL_IN; ;}
    break;

  case 131:

    { g_ParamQual = PARAM_QUAL_OUT; ;}
    break;

  case 132:

    { g_ParamQual = PARAM_QUAL_INOUT; ;}
    break;

  case 133:

    {
                                                                                                            (yyval.statement_chain) = (yyvsp[0].statement_chain);
                                                                                                         ;}
    break;

  case 134:

    {
                                                                                                            glsl_commit_singleton_variable_instance((yyvsp[0].s), NULL);
                                                                                                            (yyval.statement_chain) = glsl_statement_chain_append((yyvsp[-2].statement_chain), glsl_statement_construct_var_decl(g_LastInstance, NULL));
                                                                                                         ;}
    break;

  case 135:

    {
                                                                                                            glsl_commit_array_instance((yyvsp[-3].s), (yyvsp[-1].expr));
                                                                                                            (yyval.statement_chain) = glsl_statement_chain_append((yyvsp[-5].statement_chain), glsl_statement_construct_var_decl(g_LastInstance, NULL));
                                                                                                         ;}
    break;

  case 136:

    {
                                                                                                            glsl_commit_singleton_variable_instance((yyvsp[-2].s), (yyvsp[0].expr));
                                                                                                            (yyval.statement_chain) = glsl_statement_chain_append((yyvsp[-4].statement_chain), glsl_statement_construct_var_decl(g_LastInstance, (yyvsp[0].expr)));
                                                                                                         ;}
    break;

  case 137:

    {
                                                                                                      // This is to match struct declarations, but unfortunately it also admits rubbish like "int , x".
                                                                                                      (yyval.statement_chain) = glsl_statement_chain_create();
                                                                                                   ;}
    break;

  case 138:

    {
                                                                                                      glsl_commit_singleton_variable_instance((yyvsp[0].s), NULL);
                                                                                                      (yyval.statement_chain) = glsl_statement_chain_append(glsl_statement_chain_create(), glsl_statement_construct_var_decl(g_LastInstance, NULL));
                                                                                                   ;}
    break;

  case 139:

    {
                                                                                                      glsl_commit_array_instance((yyvsp[-3].s), (yyvsp[-1].expr));
                                                                                                      (yyval.statement_chain) = glsl_statement_chain_append(glsl_statement_chain_create(), glsl_statement_construct_var_decl(g_LastInstance, NULL));
                                                                                                   ;}
    break;

  case 140:

    {
                                                                                                      glsl_commit_singleton_variable_instance((yyvsp[-2].s), (yyvsp[0].expr));
                                                                                                      (yyval.statement_chain) = glsl_statement_chain_append(glsl_statement_chain_create(), glsl_statement_construct_var_decl(g_LastInstance, (yyvsp[0].expr)));
                                                                                                   ;}
    break;

  case 141:

    {
                                                                                                      (yyval.statement_chain) = glsl_statement_chain_create();
                                                                                                   ;}
    break;

  case 142:

    { g_TypeQual = TYPE_QUAL_DEFAULT; ;}
    break;

  case 144:

    { g_TypeQual = TYPE_QUAL_CONST; ;}
    break;

  case 145:

    { g_TypeQual = TYPE_QUAL_ATTRIBUTE; ;}
    break;

  case 146:

    { g_TypeQual = TYPE_QUAL_VARYING; ;}
    break;

  case 147:

    { g_TypeQual = TYPE_QUAL_INVARIANT_VARYING; ;}
    break;

  case 148:

    { g_TypeQual = TYPE_QUAL_UNIFORM; ;}
    break;

  case 151:

    { g_TypeBuilder = &primitiveTypes[PRIM_VOID]; ;}
    break;

  case 152:

    { g_TypeBuilder = &primitiveTypes[PRIM_FLOAT]; ;}
    break;

  case 153:

    { g_TypeBuilder = &primitiveTypes[PRIM_INT]; ;}
    break;

  case 154:

    { g_TypeBuilder = &primitiveTypes[PRIM_BOOL]; ;}
    break;

  case 155:

    { g_TypeBuilder = &primitiveTypes[PRIM_VEC2]; ;}
    break;

  case 156:

    { g_TypeBuilder = &primitiveTypes[PRIM_VEC3]; ;}
    break;

  case 157:

    { g_TypeBuilder = &primitiveTypes[PRIM_VEC4]; ;}
    break;

  case 158:

    { g_TypeBuilder = &primitiveTypes[PRIM_BVEC2]; ;}
    break;

  case 159:

    { g_TypeBuilder = &primitiveTypes[PRIM_BVEC3]; ;}
    break;

  case 160:

    { g_TypeBuilder = &primitiveTypes[PRIM_BVEC4]; ;}
    break;

  case 161:

    { g_TypeBuilder = &primitiveTypes[PRIM_IVEC2]; ;}
    break;

  case 162:

    { g_TypeBuilder = &primitiveTypes[PRIM_IVEC3]; ;}
    break;

  case 163:

    { g_TypeBuilder = &primitiveTypes[PRIM_IVEC4]; ;}
    break;

  case 164:

    { g_TypeBuilder = &primitiveTypes[PRIM_MAT2]; ;}
    break;

  case 165:

    { g_TypeBuilder = &primitiveTypes[PRIM_MAT3]; ;}
    break;

  case 166:

    { g_TypeBuilder = &primitiveTypes[PRIM_MAT4]; ;}
    break;

  case 167:

    { g_TypeBuilder = &primitiveTypes[PRIM_SAMPLER2D]; ;}
    break;

  case 168:

    { g_TypeBuilder = &primitiveTypes[PRIM_SAMPLERCUBE]; ;}
    break;

  case 169:

    { /* g_TypeBuilder already set. */ ;}
    break;

  case 170:

    { g_TypeBuilder = (yyvsp[0].lookup).symbol->type; ;}
    break;

  case 171:

    {  ;}
    break;

  case 172:

    {  ;}
    break;

  case 173:

    {  ;}
    break;

  case 174:

    { glsl_reinit_struct_builder(); ;}
    break;

  case 175:

    { glsl_build_struct_type(); glsl_commit_struct_type((yyvsp[-3].s)); ;}
    break;

  case 176:

    { glsl_reinit_struct_builder(); ;}
    break;

  case 177:

    { glsl_build_struct_type(); ;}
    break;

  case 182:

    { glsl_commit_singleton_struct_member((yyvsp[0].s)); ;}
    break;

  case 183:

    { glsl_commit_array_struct_member((yyvsp[-3].s), (yyvsp[-1].expr)); ;}
    break;

  case 184:

    { glsl_commit_singleton_struct_member((yyvsp[0].s)); ;}
    break;

  case 185:

    { glsl_commit_array_struct_member((yyvsp[-3].s), (yyvsp[-1].expr)); ;}
    break;

  case 186:

    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 187:

    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 188:

    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 189:

    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 190:

    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 191:

    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 192:

    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 193:

    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 194:

    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 195:

    { (yyval.statement) = glsl_statement_construct_compound(glsl_statement_chain_create()); ;}
    break;

  case 196:

    { glsl_enter_scope(); ;}
    break;

  case 197:

    { glsl_exit_scope(); (yyval.statement) = glsl_statement_construct_compound((yyvsp[-1].statement_chain)); ;}
    break;

  case 198:

    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 199:

    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 200:

    { (yyval.statement) = glsl_statement_construct_compound(glsl_statement_chain_create()); ;}
    break;

  case 201:

    { (yyval.statement) = glsl_statement_construct_compound((yyvsp[-1].statement_chain)); ;}
    break;

  case 202:

    { (yyval.statement_chain) = glsl_statement_chain_append(glsl_statement_chain_create(), (yyvsp[0].statement)); ;}
    break;

  case 203:

    { (yyval.statement_chain) = glsl_statement_chain_append((yyvsp[-1].statement_chain), (yyvsp[0].statement)); ;}
    break;

  case 204:

    { (yyval.statement) = glsl_statement_construct(STATEMENT_NULL); ;}
    break;

  case 205:

    { (yyval.statement) = glsl_statement_construct_expr((yyvsp[-1].expr)); ;}
    break;

  case 206:

    { (yyval.statement) = glsl_statement_construct_selection((yyvsp[-2].expr), (yyvsp[0].statement2).a, (yyvsp[0].statement2).b); ;}
    break;

  case 207:

    { (yyval.statement2).a = (yyvsp[-2].statement); (yyval.statement2).b = (yyvsp[0].statement); ;}
    break;

  case 208:

    { (yyval.statement2).a = (yyvsp[0].statement); (yyval.statement2).b = NULL; ;}
    break;

  case 209:

    { (yyval.statement) = glsl_statement_construct_expr((yyvsp[0].expr)); ;}
    break;

  case 210:

    { glsl_commit_singleton_variable_instance((yyvsp[-2].s), (yyvsp[0].expr)); (yyval.statement) = glsl_statement_construct_var_decl(g_LastInstance, (yyvsp[0].expr)); ;}
    break;

  case 211:

    { glsl_enter_scope(); ;}
    break;

  case 212:

    { glsl_exit_scope(); (yyval.statement) = glsl_statement_construct_iterator_while((yyvsp[-2].statement), (yyvsp[0].statement)); ;}
    break;

  case 213:

    { (yyval.statement) = glsl_statement_construct_iterator_do_while((yyvsp[-5].statement), (yyvsp[-2].expr)); ;}
    break;

  case 214:

    { glsl_enter_scope(); ;}
    break;

  case 215:

    { glsl_exit_scope(); (yyval.statement) = glsl_statement_construct_iterator_for((yyvsp[-6].statement), (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); ;}
    break;

  case 218:

    { g_TypeBuilder = &primitiveTypes[PRIM_FLOAT]; ;}
    break;

  case 219:

    { g_TypeBuilder = &primitiveTypes[PRIM_INT]; ;}
    break;

  case 220:

    {
                                                                             g_TypeQual = TYPE_QUAL_LOOP_INDEX;

                                                                             glsl_commit_singleton_variable_instance((yyvsp[-2].s), (yyvsp[0].expr));
                                                                             (yyval.statement) = glsl_statement_construct_var_decl(g_LastInstance, (yyvsp[0].expr));
                                                                          ;}
    break;

  case 223:

    { (yyval.statement) = glsl_statement_construct(STATEMENT_CONTINUE); ;}
    break;

  case 224:

    { (yyval.statement) = glsl_statement_construct(STATEMENT_BREAK); ;}
    break;

  case 225:

    { (yyval.statement) = glsl_statement_construct(STATEMENT_RETURN); ;}
    break;

  case 226:

    { (yyval.statement) = glsl_statement_construct_return_expr((yyvsp[-1].expr)); ;}
    break;

  case 227:

    { if (SHADER_FRAGMENT != g_ShaderFlavour) glsl_compile_error(ERROR_CUSTOM, 12, g_LineNumber, NULL); (yyval.statement) = glsl_statement_construct(STATEMENT_DISCARD); ;}
    break;

  case 228:

    {
                                                 StatementChain* chain = glsl_statement_chain_create();
                                                 glsl_statement_chain_append(chain, (yyvsp[0].statement));
                                                 (yyval.statement) = glsl_statement_construct_ast(chain);
                                                 *(Statement**)top_level_statement = (yyval.statement); // Save for calling function.
                                              ;}
    break;

  case 229:

    {
                                                 StatementChain* chain = (yyvsp[-1].statement)->u.ast.decls;
                                                 glsl_statement_chain_append(chain, (yyvsp[0].statement));
                                                 (yyval.statement) = (yyvsp[-1].statement);
                                                 *(Statement**)top_level_statement = (yyval.statement); // Save for calling function.
                                              ;}
    break;

  case 230:

    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 231:

    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 232:

    { glsl_enter_scope(); g_InGlobalScope = false; glsl_instantiate_function_params(g_TypeBuilder); ;}
    break;

  case 233:

    { glsl_exit_scope(); g_InGlobalScope = true; (yyval.statement) = glsl_statement_construct_function_def((yyvsp[-2].symbol), (yyvsp[0].statement)); glsl_insert_function_definition((yyval.statement)); ;}
    break;

  case 234:

    { (yyval.s) = (yyvsp[0].lookup).name; ;}
    break;

  case 235:

    { (yyval.s) = (yyvsp[0].lookup).name; ;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */


  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  int yytype = YYTRANSLATE (yychar);
	  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
	  YYSIZE_T yysize = yysize0;
	  YYSIZE_T yysize1;
	  int yysize_overflow = 0;
	  char *yymsg = 0;
#	  define YYERROR_VERBOSE_ARGS_MAXIMUM 5
	  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	  int yyx;

#if 0
	  /* This is so xgettext sees the translatable formats that are
	     constructed on the fly.  */
	  YY_("syntax error, unexpected %s");
	  YY_("syntax error, unexpected %s, expecting %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
#endif
	  char *yyfmt;
	  char const *yyf;
	  static char const yyunexpected[] = "syntax error, unexpected %s";
	  static char const yyexpecting[] = ", expecting %s";
	  static char const yyor[] = " or %s";
	  char yyformat[sizeof yyunexpected
			+ sizeof yyexpecting - 1
			+ ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
			   * (sizeof yyor - 1))];
	  char const *yyprefix = yyexpecting;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 1;

	  yyarg[0] = yytname[yytype];
	  yyfmt = yystpcpy (yyformat, yyunexpected);

	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
		  {
		    yycount = 1;
		    yysize = yysize0;
		    yyformat[sizeof yyunexpected - 1] = '\0';
		    break;
		  }
		yyarg[yycount++] = yytname[yyx];
		yysize1 = yysize + yytnamerr (0, yytname[yyx]);
		yysize_overflow |= yysize1 < yysize;
		yysize = yysize1;
		yyfmt = yystpcpy (yyfmt, yyprefix);
		yyprefix = yyor;
	      }

	  yyf = YY_(yyformat);
	  yysize1 = yysize + yystrlen (yyf);
	  yysize_overflow |= yysize1 < yysize;
	  yysize = yysize1;

	  if (!yysize_overflow && yysize <= YYSTACK_ALLOC_MAXIMUM)
	    yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg)
	    {
	      /* Avoid sprintf, as that infringes on the user's name space.
		 Don't have undefined behavior even if the translation
		 produced a string with the wrong number of "%s"s.  */
	      char *yyp = yymsg;
	      int yyi = 0;
	      while ((*yyp = *yyf))
		{
		  if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		    {
		      yyp += yytnamerr (yyp, yyarg[yyi++]);
		      yyf += 2;
		    }
		  else
		    {
		      yyp++;
		      yyf++;
		    }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    {
	      yyerror (YY_("syntax error"));
	      goto yyexhaustedlab;
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (YY_("syntax error"));
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
        }
      else
	{
	  yydestruct ("Error: discarding", yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (0)
     goto yyerrorlab;

yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}



