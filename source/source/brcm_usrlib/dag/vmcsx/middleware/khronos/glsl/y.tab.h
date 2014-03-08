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
/* Line 1403 of yacc.c.  */
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



