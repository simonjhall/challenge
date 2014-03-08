%{
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
      TokenType type = 0;
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
%}

%{
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
%}

// Just one if-else shift/reduce conflict, for which the default behaviour is fine.
%expect 1

%no-lines

// The rule that will derive a correct program.
%start translation_unit

// Yacc's value stack type.
%union 
{
	const_int i; const_float f; const char* s;
   struct { const char* name; Symbol* symbol; } lookup;
   SymbolType* type;
   Symbol* symbol;
   CallContext call_context;
   Expr* expr; ExprFlavour expr_flavour;
   Statement* statement;
   StatementChain* statement_chain;
   struct { Statement* a; Statement* b; } statement2;
}

%token<lookup> IDENTIFIER     512
%token<i> PPNUMBERI           513
%token<f> PPNUMBERF           514
%token<s> PPNUMBERU           515

%token<s> UNKNOWN             516

%token HASH                   517
%token WHITESPACE             518
%token NEWLINE                519

%token LEFT_OP                520
%token RIGHT_OP               521
%token INC_OP                 522
%token DEC_OP                 523
%token LE_OP                  524
%token GE_OP                  525
%token EQ_OP                  526
%token NE_OP                  527
%token LOGICAL_AND_OP         528
%token LOGICAL_OR_OP          529
%token LOGICAL_XOR_OP         530
%token MUL_ASSIGN             531
%token DIV_ASSIGN             532
%token ADD_ASSIGN             533
%token MOD_ASSIGN             534
%token LEFT_ASSIGN            535
%token RIGHT_ASSIGN           536
%token AND_ASSIGN             537
%token XOR_ASSIGN             538
%token OR_ASSIGN              539
%token SUB_ASSIGN             540
%token LEFT_PAREN             541
%token RIGHT_PAREN            542
%token LEFT_BRACKET           543
%token RIGHT_BRACKET          544
%token LEFT_BRACE             545
%token RIGHT_BRACE            546
%token DOT                    547
%token COMMA                  548
%token COLON                  549
%token EQUAL                  550
%token SEMICOLON              551
%token BANG                   552
%token DASH                   553
%token TILDE                  554
%token PLUS                   555
%token STAR                   556
%token SLASH                  557
%token PERCENT                558
%token LEFT_ANGLE             559
%token RIGHT_ANGLE            560
%token BITWISE_OR_OP          561
%token BITWISE_XOR_OP         562
%token BITWISE_AND_OP         563
%token QUESTION               564

%token INTRINSIC_TEXTURE_2D_BIAS       565
%token INTRINSIC_TEXTURE_2D_LOD        566
%token INTRINSIC_TEXTURE_CUBE_BIAS     567
%token INTRINSIC_TEXTURE_CUBE_LOD      568
%token INTRINSIC_RSQRT                 569
%token INTRINSIC_RCP                   570
%token INTRINSIC_LOG2                  571
%token INTRINSIC_EXP2                  572
%token INTRINSIC_CEIL                  573
%token INTRINSIC_FLOOR                 574
%token INTRINSIC_SIGN                  575
%token INTRINSIC_TRUNC                 576
%token INTRINSIC_NEAREST               577
%token INTRINSIC_MIN                   578
%token INTRINSIC_MAX                   579

%token DEFINE                 580
%token UNDEF                  581
%token IFDEF                  582
%token IFNDEF                 583
%token ELIF                   584
%token ENDIF                  585
%token ERROR                  586
%token PRAGMA                 587
%token EXTENSION              588
%token VERSION                589
%token LINE                   590

%token ALL                    591
%token REQUIRE                592
%token ENABLE                 593
%token WARN                   594
%token DISABLE                595

%token ATTRIBUTE              596
%token CONST                  597
%token BOOL                   598
%token FLOAT                  599
%token _INT                   600
%token BREAK                  601
%token CONTINUE               602
%token DO                     603
%token ELSE                   604
%token FOR                    605
%token IF                     606
%token DISCARD                607
%token RETURN                 608
%token BVEC2                  609
%token BVEC3                  610
%token BVEC4                  611
%token IVEC2                  612
%token IVEC3                  613
%token IVEC4                  614
%token VEC2                   615
%token VEC3                   616
%token VEC4                   617
%token MAT2                   618
%token MAT3                   619
%token MAT4                   620
%token IN                     621
%token OUT                    622
%token INOUT                  623
%token UNIFORM                624
%token VARYING                625
%token SAMPLER2D              626
%token SAMPLERCUBE            627
%token STRUCT                 628
%token _VOID                  629
%token WHILE                  630
%token INVARIANT              631
%token HIGH_PRECISION         632
%token MEDIUM_PRECISION       633
%token LOW_PRECISION          634
%token PRECISION              635

%token ASM                    636
%token CLASS                  637
%token UNION                  638
%token ENUM                   639
%token TYPEDEF                640
%token TEMPLATE               641
%token THIS                   642
%token PACKED                 643
%token GOTO                   644
%token SWITCH                 645
%token DEFAULT                646
%token _INLINE                647
%token NOINLINE               648
%token VOLATILE               649
%token PUBLIC                 650
%token STATIC                 651
%token EXTERN                 652
%token EXTERNAL               653
%token INTERFACE              654
%token FLAT                   655
%token LONG                   656
%token SHORT                  657
%token DOUBLE                 658
%token HALF                   659
%token FIXED                  660
%token _UNSIGNED              661
%token SUPERP                 662
%token INPUT                  663
%token OUTPUT                 664
%token HVEC2                  665
%token HVEC3                  666
%token HVEC4                  667
%token DVEC2                  668
%token DVEC3                  669
%token DVEC4                  670
%token FVEC2                  671
%token FVEC3                  672
%token FVEC4                  673
%token SAMPLER1D              674
%token SAMPLER3D              675
%token SAMPLER1DSHADOW        676
%token SAMPLER2DSHADOW        677
%token SAMPLER2DRECT          678
%token SAMPLER3DRECT          679
%token SAMPLER2DRECTSHADOW    680
%token SIZEOF                 681
%token CAST                   682
%token NAMESPACE              683
%token USING                  684

%token _TRUE                  685
%token _FALSE                 686

%token<lookup> CANDIDATE_TYPE_NAME     687

%type<lookup> variable_identifier
%type<expr> primary_expression
%type<expr> postfix_expression
%type<expr> integer_expression

%type<expr> function_call
%type<call_context> function_call_generic
%type<call_context> function_call_header_no_parameters
%type<call_context> function_call_header_with_parameters
%type<call_context> function_call_header
%type<call_context> function_identifier
%type<call_context> intrinsic_identifier
%type<call_context> constructor_identifier

%type<expr> unary_expression
%type<expr> multiplicative_expression
%type<expr> additive_expression
%type<expr> shift_expression
%type<expr> relational_expression
%type<expr> equality_expression
%type<expr> and_expression
%type<expr> exclusive_or_expression
%type<expr> inclusive_or_expression
%type<expr> logical_and_expression
%type<expr> logical_xor_expression
%type<expr> logical_or_expression
%type<expr> conditional_expression
%type<expr> assignment_expression
%type<expr_flavour> assignment_operator
%type<expr> expression
%type<expr> constant_expression

%type<statement> declaration

%type<symbol> function_prototype

%type<statement_chain> init_declarator_list
%type<statement_chain> init_declarator

%type<expr> initializer

%type<statement> declaration_statement
%type<statement> statement
%type<statement> simple_statement
%type<statement> compound_statement
%type<statement> statement_no_new_scope
%type<statement> compound_statement_no_new_scope
%type<statement_chain> statement_list
%type<statement> expression_statement
%type<statement> selection_statement
%type<statement2> selection_rest_statement
%type<statement> condition

%type<statement> iteration_statement

%type<statement> for_init
%type<expr> for_test
%type<expr> for_loop

%type<statement> jump_statement

%type<statement> translation_unit
%type<statement> external_declaration
%type<statement> function_definition

%type<s> identifier_or_typename

%%

variable_identifier
      : IDENTIFIER { $$ = $1; }
      ;

primary_expression
      : variable_identifier               { $$ = glsl_expr_construct_instance($1.symbol); }
      | PPNUMBERF                         { $$ = glsl_expr_construct_value_float($1); }
      | PPNUMBERI                         { $$ = glsl_expr_construct_value_int($1); }
      | _TRUE                             { $$ = glsl_expr_construct_value_bool(CONST_BOOL_TRUE); }
      | _FALSE                            { $$ = glsl_expr_construct_value_bool(CONST_BOOL_FALSE); }
      | LEFT_PAREN expression RIGHT_PAREN { $$ = $2; }
      ;

postfix_expression
      : primary_expression                                               { $$ = $1; }
      | postfix_expression LEFT_BRACKET integer_expression RIGHT_BRACKET { $$ = glsl_expr_construct_subscript($1, $3); }
      | function_call                                                    { $$ = $1; }
      | postfix_expression DOT identifier_or_typename                    { $$ = glsl_expr_construct_field_selector($1, $3); }
      | postfix_expression INC_OP                                        { $$ = glsl_expr_construct_unary_op_arithmetic(EXPR_POST_INC, $1); }
      | postfix_expression DEC_OP                                        { $$ = glsl_expr_construct_unary_op_arithmetic(EXPR_POST_DEC, $1); }
      ;
      
integer_expression
      : expression { $$ = $1; }
      ;
      
function_call
      : function_call_generic {
                                 switch($1.flavour)
                                 {
                                    case CALL_CONTEXT_FUNCTION:
                                       $$ = glsl_expr_construct_function_call($1.u.function.symbol, $1.args);
                                       break;
                                    case CALL_CONTEXT_PRIM_CONSTRUCTOR:
                                       $$ = glsl_expr_construct_prim_constructor_call($1.u.prim_constructor.index, $1.args);
                                       break;
                                    case CALL_CONTEXT_TYPE_CONSTRUCTOR:
                                       $$ = glsl_expr_construct_type_constructor_call($1.u.type_constructor.symbol->type, $1.args);
                                       break;
                                    case CALL_CONTEXT_INTRINSIC:
                                       $$ = glsl_expr_construct_intrinsic($1.u.intrinsic.flavour, $1.args);
                                       break;
                                    default:
                                       UNREACHABLE();
                                       break;
                                 }
                              }
      ;
      
function_call_generic
      : function_call_header_with_parameters RIGHT_PAREN { $$ = $1; }
      | function_call_header_no_parameters RIGHT_PAREN   { $$ = $1; }
      ;
      
function_call_header_no_parameters
      : function_call_header _VOID { $$.args = glsl_expr_chain_create(); }
      | function_call_header       { $$.args = glsl_expr_chain_create(); }
      ;
      
function_call_header_with_parameters
      : function_call_header assignment_expression                       { $$.args = glsl_expr_chain_append(glsl_expr_chain_create(), $2); }
      | function_call_header_with_parameters COMMA assignment_expression { $$.args = glsl_expr_chain_append($1.args, $3); }
      ;
      
function_call_header
      : function_identifier LEFT_PAREN   { $$ = $1; }
      ;
      
function_identifier
      : constructor_identifier { $$ = $1; }
      | intrinsic_identifier   { $$ = $1; }
      | IDENTIFIER             { $$.flavour = CALL_CONTEXT_FUNCTION; $$.u.function.symbol = $1.symbol; }
      ;

// These are language extensions.
intrinsic_identifier
      : INTRINSIC_TEXTURE_2D_BIAS   { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_TEXTURE_2D_BIAS; }
      | INTRINSIC_TEXTURE_2D_LOD    { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_TEXTURE_2D_LOD; }
      | INTRINSIC_TEXTURE_CUBE_BIAS { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_TEXTURE_CUBE_BIAS; }
      | INTRINSIC_TEXTURE_CUBE_LOD  { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_TEXTURE_CUBE_LOD; }
      | INTRINSIC_RSQRT             { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_RSQRT; }
      | INTRINSIC_RCP               { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_RCP; }
      | INTRINSIC_LOG2              { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_LOG2; }
      | INTRINSIC_EXP2              { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_EXP2; }
      | INTRINSIC_CEIL              { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_CEIL; }
      | INTRINSIC_FLOOR             { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_FLOOR; }
      | INTRINSIC_SIGN              { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_SIGN; }
      | INTRINSIC_TRUNC             { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_TRUNC; }
      | INTRINSIC_NEAREST           { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_NEAREST; }
      | INTRINSIC_MIN               { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_MIN; }
      | INTRINSIC_MAX               { $$.flavour = CALL_CONTEXT_INTRINSIC; $$.u.intrinsic.flavour = EXPR_INTRINSIC_MAX; }
      ;
      
constructor_identifier
      : FLOAT               { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_FLOAT; }
      | _INT                { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_INT; }
      | BOOL                { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_BOOL; }
      | VEC2                { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_VEC2; }
      | VEC3                { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_VEC3; }
      | VEC4                { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_VEC4; }
      | BVEC2               { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_BVEC2; }
      | BVEC3               { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_BVEC3; }
      | BVEC4               { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_BVEC4; }
      | IVEC2               { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_IVEC2; }
      | IVEC3               { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_IVEC3; }
      | IVEC4               { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_IVEC4; }
      | MAT2                { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_MAT2; }
      | MAT3                { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_MAT3; }
      | MAT4                { $$.flavour = CALL_CONTEXT_PRIM_CONSTRUCTOR; $$.u.prim_constructor.index = PRIM_MAT4; }
      | CANDIDATE_TYPE_NAME { $$.flavour = CALL_CONTEXT_TYPE_CONSTRUCTOR; $$.u.type_constructor.symbol = $1.symbol; }
      ;

unary_expression
      : postfix_expression      { $$ = $1; }
      | INC_OP unary_expression { $$ = glsl_expr_construct_unary_op_arithmetic(EXPR_PRE_INC, $2); }
      | DEC_OP unary_expression { $$ = glsl_expr_construct_unary_op_arithmetic(EXPR_PRE_DEC, $2); }
      | PLUS unary_expression   { $$ = $2; }
      | DASH unary_expression   { $$ = glsl_expr_construct_unary_op_arithmetic(EXPR_ARITH_NEGATE, $2); }
      | BANG unary_expression   { $$ = glsl_expr_construct_unary_op_logical(EXPR_LOGICAL_NOT, $2); }
      | TILDE unary_expression  { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); }
      ;
      
// Grammar Note: No traditional style type casts.      
// Grammar Note: No '*' or '&' unary ops. Pointers are not supported.

multiplicative_expression
      : unary_expression                                   { $$ = $1; }
      | multiplicative_expression STAR unary_expression    { $$ = glsl_expr_construct_binary_op_arithmetic(EXPR_MUL, $1, $3); }
      | multiplicative_expression SLASH unary_expression   { $$ = glsl_expr_construct_binary_op_arithmetic(EXPR_DIV, $1, $3); }
      | multiplicative_expression PERCENT unary_expression { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); }
      ;
      
additive_expression
      : multiplicative_expression                          { $$ = $1; }
      | additive_expression PLUS multiplicative_expression { $$ = glsl_expr_construct_binary_op_arithmetic(EXPR_ADD, $1, $3); }
      | additive_expression DASH multiplicative_expression { $$ = glsl_expr_construct_binary_op_arithmetic(EXPR_SUB, $1, $3); }
      ;
      
shift_expression
      : additive_expression                           { $$ = $1; }
      | shift_expression LEFT_OP additive_expression  { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); }
      | shift_expression RIGHT_OP additive_expression { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); }
      ;
      
relational_expression
      : shift_expression                                   { $$ = $1; }
      | relational_expression LEFT_ANGLE shift_expression  { $$ = glsl_expr_construct_binary_op_relational(EXPR_LESS_THAN, $1, $3); }
      | relational_expression RIGHT_ANGLE shift_expression { $$ = glsl_expr_construct_binary_op_relational(EXPR_GREATER_THAN, $1, $3); }
      | relational_expression LE_OP shift_expression       { $$ = glsl_expr_construct_binary_op_relational(EXPR_LESS_THAN_EQUAL, $1, $3); }
      | relational_expression GE_OP shift_expression       { $$ = glsl_expr_construct_binary_op_relational(EXPR_GREATER_THAN_EQUAL, $1, $3); }
      ;
      
equality_expression
      : relational_expression                           { $$ = $1; }
      | equality_expression EQ_OP relational_expression { $$ = glsl_expr_construct_binary_op_equality(EXPR_EQUAL, $1, $3); }
      | equality_expression NE_OP relational_expression { $$ = glsl_expr_construct_binary_op_equality(EXPR_NOT_EQUAL, $1, $3); }
      ;

and_expression
      : equality_expression                               { $$ = $1; }
      | and_expression BITWISE_AND_OP equality_expression { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); }
      ;
      
exclusive_or_expression
      : and_expression                                        { $$ = $1; }
      | exclusive_or_expression BITWISE_XOR_OP and_expression { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); }
      ;
      
inclusive_or_expression
      : exclusive_or_expression                                       { $$ = $1; }
      | inclusive_or_expression BITWISE_OR_OP exclusive_or_expression { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); }
      ;
      
logical_and_expression
      : inclusive_or_expression                                       { $$ = $1; }
      | logical_and_expression LOGICAL_AND_OP inclusive_or_expression { $$ = glsl_expr_construct_binary_op_logical(EXPR_LOGICAL_AND, $1, $3); }
      ;
      
logical_xor_expression
      : logical_and_expression                                       { $$ = $1; }
      | logical_xor_expression LOGICAL_XOR_OP logical_and_expression { $$ = glsl_expr_construct_binary_op_logical(EXPR_LOGICAL_XOR, $1, $3); }
      ;
      
logical_or_expression
      : logical_xor_expression                                     { $$ = $1; }
      | logical_or_expression LOGICAL_OR_OP logical_xor_expression { $$ = glsl_expr_construct_binary_op_logical(EXPR_LOGICAL_OR, $1, $3); }
      ;
      
conditional_expression
      : logical_or_expression                                                 { $$ = $1; }
      | logical_or_expression QUESTION expression COLON assignment_expression { $$ = glsl_expr_construct_cond_op($1, $3, $5); }
      ;
      
assignment_expression
      : conditional_expression                                     { $$ = $1; }
      | unary_expression EQUAL assignment_expression               { $$ = glsl_expr_construct_assign_op($1, $3); }
      | unary_expression assignment_operator assignment_expression { $$ = glsl_expr_construct_assign_op($1, glsl_expr_construct_binary_op_arithmetic($2, $1, $3)); }
      ;
      
assignment_operator
      : MUL_ASSIGN   { $$ = EXPR_MUL; }
      | DIV_ASSIGN   { $$ = EXPR_DIV; }
      | ADD_ASSIGN   { $$ = EXPR_ADD; }
      | SUB_ASSIGN   { $$ = EXPR_SUB; }
      | MOD_ASSIGN   { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); }
      | LEFT_ASSIGN  { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); }
      | RIGHT_ASSIGN { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); }
      | AND_ASSIGN   { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); }
      | XOR_ASSIGN   { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); }
      | OR_ASSIGN    { /* RESERVED */ glsl_compile_error(ERROR_LEXER_PARSER, 3, g_LineNumber, NULL); }
      ;
      
expression
      : assignment_expression                  { $$ = $1; }
      | expression COMMA assignment_expression { $$ = glsl_expr_construct_sequence($1, $3); }
      ;
      
constant_expression
      : conditional_expression { $$ = $1; }
      ;
      
declaration
      : function_prototype SEMICOLON                                   { 
                                                                          // Don't store these in the AST.
                                                                          $$ = glsl_statement_construct_decl_list(glsl_statement_chain_create());
                                                                       }
      | init_declarator_list SEMICOLON                                 {
                                                                          $$ = glsl_statement_construct_decl_list($1);
                                                                       }
      | PRECISION precision_qualifier type_specifier_no_prec SEMICOLON {
                                                                          // "Precision not supported" warning already issued.
                                                                          // Don't store these in the AST.
                                                                          $$ = glsl_statement_construct_decl_list(glsl_statement_chain_create());
                                                                       }
      ;
      
function_prototype
      : function_declarator RIGHT_PAREN { glsl_build_function_type(); glsl_commit_singleton_function_declaration(g_FunctionBuilderName); $$ = g_LastInstance; }
      ;

// We cannot find (void) functions here due to shift/reduce conflicts with nameless parameters in function declarations.
// Instead we will convert (void) to the canonical form () in glsl_build_function_type().

function_declarator
      : function_header                 { /* Nothing to do here. */ }
      | function_header_with_parameters { /* Nothing to do here. */ }
      ;
      
function_header_with_parameters
      : function_header parameter_declaration                       { /* Nothing to do here. */ }
      | function_header_with_parameters COMMA parameter_declaration { /* Nothing to do here. */ }
      ;
      
function_header
      : fully_specified_type identifier_or_typename LEFT_PAREN                                                { g_FunctionBuilderReturnType = g_TypeBuilder; g_FunctionBuilderName = $2; glsl_reinit_function_builder(); }
      | fully_specified_type LEFT_BRACKET constant_expression RIGHT_BRACKET identifier_or_typename LEFT_PAREN { glsl_compile_error(ERROR_SEMANTIC, 41, g_LineNumber, NULL); } 
      ;
      
parameter_declaration
      : type_and_parameter_qualifier type_specifier identifier_or_typename                                                { glsl_commit_singleton_function_param($3); }
      | type_and_parameter_qualifier type_specifier identifier_or_typename LEFT_BRACKET constant_expression RIGHT_BRACKET { glsl_commit_array_function_param($3, $5); }
      | type_and_parameter_qualifier type_specifier                                                                       { glsl_commit_singleton_function_param(generate_anon_param()); }
      | type_and_parameter_qualifier type_specifier LEFT_BRACKET constant_expression RIGHT_BRACKET                        { glsl_commit_array_function_param(generate_anon_param(), $4); }
      ;

type_and_parameter_qualifier
      : type_qualifier parameter_qualifier { /* Nothing to do here. */ }
      | parameter_qualifier                { g_TypeQual = TYPE_QUAL_DEFAULT; }
      ;
      
parameter_qualifier
      : /* empty */ { g_ParamQual = PARAM_QUAL_DEFAULT; }
      | IN          { g_ParamQual = PARAM_QUAL_IN; }
      | OUT         { g_ParamQual = PARAM_QUAL_OUT; }
      | INOUT       { g_ParamQual = PARAM_QUAL_INOUT; }
      ;
      
// This rule should return a StatementChain* of STATEMENT_VAR_DECL.
init_declarator_list
      : init_declarator                                                                                  {
                                                                                                            $$ = $1;
                                                                                                         }
      | init_declarator_list COMMA identifier_or_typename                                                {
                                                                                                            glsl_commit_singleton_variable_instance($3, NULL);
                                                                                                            $$ = glsl_statement_chain_append($1, glsl_statement_construct_var_decl(g_LastInstance, NULL));
                                                                                                         }
      | init_declarator_list COMMA identifier_or_typename LEFT_BRACKET constant_expression RIGHT_BRACKET {
                                                                                                            glsl_commit_array_instance($3, $5);
                                                                                                            $$ = glsl_statement_chain_append($1, glsl_statement_construct_var_decl(g_LastInstance, NULL));
                                                                                                         }
      | init_declarator_list COMMA identifier_or_typename EQUAL initializer                              {
                                                                                                            glsl_commit_singleton_variable_instance($3, $5);
                                                                                                            $$ = glsl_statement_chain_append($1, glsl_statement_construct_var_decl(g_LastInstance, $5));
                                                                                                         }
      ;
      
// This rule should return a StatementChain* of STATEMENT_VAR_DECL to init_declarator_list, possibly to insert more definitions.
init_declarator
      : fully_specified_type                                                                       {
                                                                                                      // This is to match struct declarations, but unfortunately it also admits rubbish like "int , x".
                                                                                                      $$ = glsl_statement_chain_create();
                                                                                                   }
      | fully_specified_type identifier_or_typename                                                {
                                                                                                      glsl_commit_singleton_variable_instance($2, NULL);
                                                                                                      $$ = glsl_statement_chain_append(glsl_statement_chain_create(), glsl_statement_construct_var_decl(g_LastInstance, NULL));
                                                                                                   }
      | fully_specified_type identifier_or_typename LEFT_BRACKET constant_expression RIGHT_BRACKET {
                                                                                                      glsl_commit_array_instance($2, $4);
                                                                                                      $$ = glsl_statement_chain_append(glsl_statement_chain_create(), glsl_statement_construct_var_decl(g_LastInstance, NULL));
                                                                                                   }
      | fully_specified_type identifier_or_typename EQUAL initializer                              {
                                                                                                      glsl_commit_singleton_variable_instance($2, $4);
                                                                                                      $$ = glsl_statement_chain_append(glsl_statement_chain_create(), glsl_statement_construct_var_decl(g_LastInstance, $4));
                                                                                                   }
      | INVARIANT identifier_or_typename                                                           {
                                                                                                      $$ = glsl_statement_chain_create();
                                                                                                   }
      ;
      
// Grammar Note: No 'enum', or 'typedef'.

fully_specified_type
      : type_specifier                { g_TypeQual = TYPE_QUAL_DEFAULT; }
      | type_qualifier type_specifier
      ;
      
type_qualifier
      : CONST             { g_TypeQual = TYPE_QUAL_CONST; }
      | ATTRIBUTE         { g_TypeQual = TYPE_QUAL_ATTRIBUTE; }
      | VARYING           { g_TypeQual = TYPE_QUAL_VARYING; }
      | INVARIANT VARYING { g_TypeQual = TYPE_QUAL_INVARIANT_VARYING; }
      | UNIFORM           { g_TypeQual = TYPE_QUAL_UNIFORM; }
      ;

type_specifier
      : type_specifier_no_prec
      | precision_qualifier type_specifier_no_prec
      ;
 
type_specifier_no_prec
      : _VOID               { g_TypeBuilder = &primitiveTypes[PRIM_VOID]; }
      | FLOAT               { g_TypeBuilder = &primitiveTypes[PRIM_FLOAT]; }
      | _INT                { g_TypeBuilder = &primitiveTypes[PRIM_INT]; }
      | BOOL                { g_TypeBuilder = &primitiveTypes[PRIM_BOOL]; }
      | VEC2                { g_TypeBuilder = &primitiveTypes[PRIM_VEC2]; }
      | VEC3                { g_TypeBuilder = &primitiveTypes[PRIM_VEC3]; }
      | VEC4                { g_TypeBuilder = &primitiveTypes[PRIM_VEC4]; }
      | BVEC2               { g_TypeBuilder = &primitiveTypes[PRIM_BVEC2]; }
      | BVEC3               { g_TypeBuilder = &primitiveTypes[PRIM_BVEC3]; }
      | BVEC4               { g_TypeBuilder = &primitiveTypes[PRIM_BVEC4]; }
      | IVEC2               { g_TypeBuilder = &primitiveTypes[PRIM_IVEC2]; }
      | IVEC3               { g_TypeBuilder = &primitiveTypes[PRIM_IVEC3]; }
      | IVEC4               { g_TypeBuilder = &primitiveTypes[PRIM_IVEC4]; }
      | MAT2                { g_TypeBuilder = &primitiveTypes[PRIM_MAT2]; }
      | MAT3                { g_TypeBuilder = &primitiveTypes[PRIM_MAT3]; }
      | MAT4                { g_TypeBuilder = &primitiveTypes[PRIM_MAT4]; }
      | SAMPLER2D           { g_TypeBuilder = &primitiveTypes[PRIM_SAMPLER2D]; }
      | SAMPLERCUBE         { g_TypeBuilder = &primitiveTypes[PRIM_SAMPLERCUBE]; }
      | struct_specifier    { /* g_TypeBuilder already set. */ }
      | CANDIDATE_TYPE_NAME { g_TypeBuilder = $1.symbol->type; }
      ;
      
// Ignore - everything is high precision.
precision_qualifier
      : HIGH_PRECISION   {  }
      | MEDIUM_PRECISION {  }
      | LOW_PRECISION    {  }
      ;
      
// Note the struct grammar has been changed from the spec to match in the same way as the normal declarations.
// This allows the type to be known before identifiers are introduced.
struct_specifier
      : STRUCT { glsl_reinit_struct_builder(); } identifier_or_typename LEFT_BRACE struct_declaration_list RIGHT_BRACE { glsl_build_struct_type(); glsl_commit_struct_type($3); }
      | STRUCT { glsl_reinit_struct_builder(); } LEFT_BRACE struct_declaration_list RIGHT_BRACE                        { glsl_build_struct_type(); }
      ;
      
struct_declaration_list
      : struct_declaration
      | struct_declaration_list struct_declaration
      ;
      
struct_declaration
      : struct_declarator_list SEMICOLON
      ;

struct_declarator_list
      : struct_declarator
      | struct_declarator_list COMMA identifier_or_typename                                                { glsl_commit_singleton_struct_member($3); }
      | struct_declarator_list COMMA identifier_or_typename LEFT_BRACKET constant_expression RIGHT_BRACKET { glsl_commit_array_struct_member($3, $5); }
      ;
      
struct_declarator
      : type_specifier identifier_or_typename                                                { glsl_commit_singleton_struct_member($2); }
      | type_specifier identifier_or_typename LEFT_BRACKET constant_expression RIGHT_BRACKET { glsl_commit_array_struct_member($2, $4); }
      ;
      
initializer
      : assignment_expression { $$ = $1; }
      ;
      
declaration_statement
      : declaration { $$ = $1; }
      ;
      
statement
      : compound_statement { $$ = $1; }
      | simple_statement   { $$ = $1; }
      ;
      
// Grammar Note: No labeled statements; 'goto' is not supported.

simple_statement
      : declaration_statement { $$ = $1; }
      | expression_statement  { $$ = $1; }
      | selection_statement   { $$ = $1; }
      | iteration_statement   { $$ = $1; }
      | jump_statement        { $$ = $1; }
      ;
      
compound_statement
      : LEFT_BRACE RIGHT_BRACE                                   { $$ = glsl_statement_construct_compound(glsl_statement_chain_create()); }
      | LEFT_BRACE { glsl_enter_scope(); } statement_list RIGHT_BRACE { glsl_exit_scope(); $$ = glsl_statement_construct_compound($3); }
      ;
      
statement_no_new_scope
      : compound_statement_no_new_scope { $$ = $1; }
      | simple_statement                { $$ = $1; }
      ;
      
compound_statement_no_new_scope
      : LEFT_BRACE RIGHT_BRACE                { $$ = glsl_statement_construct_compound(glsl_statement_chain_create()); }
      | LEFT_BRACE statement_list RIGHT_BRACE { $$ = glsl_statement_construct_compound($2); }
      ;
      
statement_list
      : statement                { $$ = glsl_statement_chain_append(glsl_statement_chain_create(), $1); }
      | statement_list statement { $$ = glsl_statement_chain_append($1, $2); }
      ;
      
expression_statement
      : SEMICOLON            { $$ = glsl_statement_construct(STATEMENT_NULL); }
      | expression SEMICOLON { $$ = glsl_statement_construct_expr($1); }
      ;
      
selection_statement
      : IF LEFT_PAREN expression RIGHT_PAREN selection_rest_statement { $$ = glsl_statement_construct_selection($3, $5.a, $5.b); }
      ;

selection_rest_statement
      : statement ELSE statement { $$.a = $1; $$.b = $3; }
      | statement                { $$.a = $1; $$.b = NULL; }
      ;
      
// Grammar Note: No 'switch'. Switch statements not supported.

condition
      : expression                                                    { $$ = glsl_statement_construct_expr($1); }
      | fully_specified_type identifier_or_typename EQUAL initializer { glsl_commit_singleton_variable_instance($2, $4); $$ = glsl_statement_construct_var_decl(g_LastInstance, $4); }
      ;
      
iteration_statement
      : WHILE LEFT_PAREN { glsl_enter_scope(); } condition RIGHT_PAREN statement_no_new_scope                                    { glsl_exit_scope(); $$ = glsl_statement_construct_iterator_while($4, $6); }
      | DO statement WHILE LEFT_PAREN expression RIGHT_PAREN SEMICOLON                                                      { $$ = glsl_statement_construct_iterator_do_while($2, $5); }
      | FOR LEFT_PAREN { glsl_enter_scope(); } for_init SEMICOLON for_test SEMICOLON for_loop RIGHT_PAREN statement_no_new_scope { glsl_exit_scope(); $$ = glsl_statement_construct_iterator_for($4, $6, $8, $10); }
      ;

induction_type_specifier
      : induction_type_specifier_no_prec
      | precision_qualifier induction_type_specifier_no_prec
      ;

induction_type_specifier_no_prec
      : FLOAT { g_TypeBuilder = &primitiveTypes[PRIM_FLOAT]; }
      | _INT  { g_TypeBuilder = &primitiveTypes[PRIM_INT]; }
      ;
 
for_init
      : induction_type_specifier identifier_or_typename EQUAL initializer {
                                                                             g_TypeQual = TYPE_QUAL_LOOP_INDEX;

                                                                             glsl_commit_singleton_variable_instance($2, $4);
                                                                             $$ = glsl_statement_construct_var_decl(g_LastInstance, $4);
                                                                          }
      ;

for_test
      : expression
      ;
      
for_loop
      : expression
      ;  

jump_statement
      : CONTINUE SEMICOLON          { $$ = glsl_statement_construct(STATEMENT_CONTINUE); }
      | BREAK SEMICOLON             { $$ = glsl_statement_construct(STATEMENT_BREAK); }
      | RETURN SEMICOLON            { $$ = glsl_statement_construct(STATEMENT_RETURN); }
      | RETURN expression SEMICOLON { $$ = glsl_statement_construct_return_expr($2); }
      | DISCARD SEMICOLON           { if (SHADER_FRAGMENT != g_ShaderFlavour) glsl_compile_error(ERROR_CUSTOM, 12, g_LineNumber, NULL); $$ = glsl_statement_construct(STATEMENT_DISCARD); }
      ;
      
// Grammar Note: No 'goto'. Gotos are not supported.

translation_unit
      : external_declaration                  {
                                                 StatementChain* chain = glsl_statement_chain_create();
                                                 glsl_statement_chain_append(chain, $1);
                                                 $$ = glsl_statement_construct_ast(chain);
                                                 *(Statement**)top_level_statement = $$; // Save for calling function.
                                              }
      | translation_unit external_declaration {
                                                 StatementChain* chain = $1->u.ast.decls;
                                                 glsl_statement_chain_append(chain, $2);
                                                 $$ = $1;
                                                 *(Statement**)top_level_statement = $$; // Save for calling function.
                                              }
      ;
      
external_declaration
      : function_definition { $$ = $1; }
      | declaration         { $$ = $1; }
      ;
      
function_definition
      : function_prototype { glsl_enter_scope(); g_InGlobalScope = false; glsl_instantiate_function_params(g_TypeBuilder); } compound_statement_no_new_scope { glsl_exit_scope(); g_InGlobalScope = true; $$ = glsl_statement_construct_function_def($1, $3); glsl_insert_function_definition($$); }
      ;

// This is a bit of a hack, but it accounts for over-eager Lex matching as CANDIDATE_TYPE_NAME instead of IDENTIFIER.
identifier_or_typename
      : IDENTIFIER    { $$ = $1.name; }
      | CANDIDATE_TYPE_NAME { $$ = $1.name; }
      ;
