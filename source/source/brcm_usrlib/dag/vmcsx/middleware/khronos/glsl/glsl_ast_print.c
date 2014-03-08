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

#include "middleware/khronos/glsl/glsl_ast_print.h"
#include "middleware/khronos/glsl/glsl_symbols.h"

#ifndef NDEBUG

//
// Utility functions.
//

// Helper function for the various argument-based expressions.
static INLINE void print_expr_chain(FILE* f, ExprChain* args, bool fully_evaluated)
{
	ExprChainNode* arg;
	for (arg = args->first; arg; arg = arg->next)
	{
		glsl_print_expr(f, arg->expr, fully_evaluated);
		if (arg->next) fprintf(f, ", ");
	}
}

static void print_param_qual(FILE* f, ParamQualifier pq)
{
	switch (pq)
	{
		case PARAM_QUAL_IN:
			fprintf(f, "in ");
			break;
		case PARAM_QUAL_OUT:
			fprintf(f, "out ");
			break;
		case PARAM_QUAL_INOUT:
			fprintf(f, "inout ");
			break;
		default:
			UNREACHABLE();
			return;
	}
}

static void print_type_qual(FILE* f, TypeQualifier tq)
{
	switch (tq)
	{
		case TYPE_QUAL_NONE:
			break;
		case TYPE_QUAL_CONST:
			fprintf(f, "const ");
			break;
		case TYPE_QUAL_ATTRIBUTE:
			fprintf(f, "attribute ");
			break;
		case TYPE_QUAL_VARYING:
			fprintf(f, "varying ");
			break;
		case TYPE_QUAL_INVARIANT_VARYING:
			fprintf(f, "invariant varying ");
			break;
		case TYPE_QUAL_UNIFORM:
			fprintf(f, "uniform ");
			break;
		default:
			UNREACHABLE();
			return;
	}
}

// Adds a trailing space unless there are no qualifiers.
static void print_qualifiers(FILE* f, Symbol* symbol)
{
	switch (symbol->flavour)
	{
		case SYMBOL_PARAM_INSTANCE:
			print_type_qual(f, symbol->u.var_instance.type_qual);
			print_param_qual(f, symbol->u.var_instance.param_qual);
			return;
		case SYMBOL_VAR_INSTANCE:
			print_type_qual(f, symbol->u.var_instance.type_qual);
			return;
		default:
			UNREACHABLE();
			return;
	}
}


//
// Value printing.
//

void glsl_print_compile_time_value(FILE* f, SymbolType* type, void* compile_time_value)
{
	if (compile_time_value)
	{
		int i, prim_index;

		switch (type->flavour)
		{
			case SYMBOL_PRIMITIVE_TYPE:
				prim_index = type->u.primitive_type.index;
				switch (primitiveTypeFlags[prim_index] & (PRIM_SCALAR_TYPE | PRIM_VECTOR_TYPE | PRIM_MATRIX_TYPE))
				{
					case PRIM_SCALAR_TYPE:
						if (&primitiveTypes[PRIM_INT] == type)
						{
							fprintf(f, "%d", *(const_int*)compile_time_value);
						}
						else if (&primitiveTypes[PRIM_FLOAT] == type)
						{
							fprintf(f, "%f", *(float*)(const_float*)compile_time_value);
						}
						else if (&primitiveTypes[PRIM_BOOL] == type)
						{
							fprintf(f, *(const_bool*)compile_time_value ? "true" : "false");
						}
						else
						{
							UNREACHABLE();
						}
						return;
					case PRIM_VECTOR_TYPE:
						// fall
					case PRIM_MATRIX_TYPE:
						fprintf(f, "%s(", primitiveTypes[prim_index].name);
						for (i = 0; i < primitiveTypeSubscriptDimensions[prim_index]; i++)
						{
							glsl_print_compile_time_value(f, primitiveTypeSubscriptTypes[prim_index], compile_time_value);
							compile_time_value = (char *)compile_time_value + primitiveTypeSubscriptTypes[prim_index]->size_as_const;
							if (i+1 < primitiveTypeSubscriptDimensions[prim_index]) fprintf(f, ", ");
						}
						fprintf(f, ")");
						return;
					default:
						UNREACHABLE();
						return;
				}

			case SYMBOL_STRUCT_TYPE:
				fprintf(f, "%s(", type->name);
				for (i = 0; i < type->u.struct_type.member_count; i++)
				{
					glsl_print_compile_time_value(f, type->u.struct_type.member_types[i], compile_time_value);
					compile_time_value = (char *)compile_time_value + type->u.struct_type.member_types[i]->size_as_const;
					if (i+1 < type->u.struct_type.member_count) fprintf(f, ", ");
				}
				fprintf(f, ")");
				return;
				
			default:
				// Nothing else can have a compile time value.
				UNREACHABLE();
				return;
		}
	}
}


//
// Expression printing.
//

static void print_expr_value(FILE* f, Expr* expr, bool fully_evaluated)
{
   UNUSED(fully_evaluated);

	glsl_print_compile_time_value(f, expr->type, expr->compile_time_value);
}

static void print_expr_instance(FILE* f, Expr* expr, bool fully_evaluated)
{
   UNUSED(fully_evaluated);

	fprintf(f, expr->u.instance.symbol->name);
}

static void print_expr_subscript(FILE* f, Expr* expr, bool fully_evaluated)
{
	glsl_print_expr(f, expr->u.subscript.aggregate, fully_evaluated);
	fprintf(f, "[");
	glsl_print_expr(f, expr->u.subscript.subscript, fully_evaluated);
	fprintf(f, "]");
}

static void print_expr_function_call(FILE* f, Expr* expr, bool fully_evaluated)
{
	fprintf(f, "%s(", expr->u.function_call.function->name);
	print_expr_chain(f, expr->u.function_call.args, fully_evaluated);
	fprintf(f, ")");
}

static void print_expr_prim_constructor_call(FILE* f, Expr* expr, bool fully_evaluated)
{
	fprintf(f, "%s(", expr->type->name);
	print_expr_chain(f, expr->u.prim_constructor_call.args, fully_evaluated);
	fprintf(f, ")");
}

static void print_expr_type_constructor_call(FILE* f, Expr* expr, bool fully_evaluated)
{
	fprintf(f, "%s(", expr->type->name);
	print_expr_chain(f, expr->u.type_constructor_call.args, fully_evaluated);
	fprintf(f, ")");
}

static void print_expr_field_selector_struct(FILE* f, Expr* expr, bool fully_evaluated)
{
	glsl_print_expr(f, expr->u.field_selector_struct.aggregate, fully_evaluated);
	fprintf(f, ".%s", expr->u.field_selector_struct.field);
}

static void print_expr_field_selector_swizzle(FILE* f, Expr* expr, bool fully_evaluated)
{
	glsl_print_expr(f, expr->u.field_selector_swizzle.aggregate, fully_evaluated);
	fprintf(f, ".%s", expr->u.field_selector_swizzle.field);
}

static void print_expr_unary_op(FILE* f, Expr* expr, bool fully_evaluated)
{
	switch (expr->flavour)
	{
		case EXPR_POST_INC:
			glsl_print_expr(f, expr->u.unary_op.operand, fully_evaluated);
			fprintf(f, "++");
			return;
		case EXPR_POST_DEC:
			glsl_print_expr(f, expr->u.unary_op.operand, fully_evaluated);
			fprintf(f, "--");
			return;

		case EXPR_PRE_INC:
			fprintf(f, "++");
			glsl_print_expr(f, expr->u.unary_op.operand, fully_evaluated);
			return;
		case EXPR_PRE_DEC:
			fprintf(f, "--");
			glsl_print_expr(f, expr->u.unary_op.operand, fully_evaluated);
			return;
		case EXPR_ARITH_NEGATE:
			fprintf(f, "-");
			glsl_print_expr(f, expr->u.unary_op.operand, fully_evaluated);
			return;
		case EXPR_LOGICAL_NOT:
			fprintf(f, "!");
			glsl_print_expr(f, expr->u.unary_op.operand, fully_evaluated);
			return;
		default:
			UNREACHABLE();
			return;
	}
}

static void print_expr_binary_op(FILE* f, Expr* expr, bool fully_evaluated)
{
	const char* infix;

	switch (expr->flavour)
	{
		case EXPR_MUL:
			infix = "*";
			break;
		case EXPR_DIV:
			infix = "/";
			break;
		case EXPR_ADD:
			infix = "+";
			break;
		case EXPR_SUB:
			infix = "-";
			break;
		case EXPR_LESS_THAN:
			infix = "<";
			break;
		case EXPR_LESS_THAN_EQUAL:
			infix = "<=";
			break;
		case EXPR_GREATER_THAN:
			infix = ">";
			break;
		case EXPR_GREATER_THAN_EQUAL:
			infix = ">=";
			break;
		case EXPR_EQUAL:
			infix = "==";
			break;
		case EXPR_NOT_EQUAL:
			infix = "!=";
			break;
		case EXPR_LOGICAL_AND:
			infix = "&&";
			break;
		case EXPR_LOGICAL_XOR:
			infix = "^^";
			break;
		case EXPR_LOGICAL_OR:
			infix = "||";
			break;
		default:
			UNREACHABLE();
			return;
	}

	glsl_print_expr(f, expr->u.binary_op.left, fully_evaluated);
	fprintf(f, " %s ", infix);
	glsl_print_expr(f, expr->u.binary_op.right, fully_evaluated);
}

static void print_expr_cond_op(FILE* f, Expr* expr, bool fully_evaluated)
{
	glsl_print_expr(f, expr->u.cond_op.cond, fully_evaluated);
	fprintf(f, " ? ");
	glsl_print_expr(f, expr->u.cond_op.if_true, fully_evaluated);
	fprintf(f, " : ");
	glsl_print_expr(f, expr->u.cond_op.if_false, fully_evaluated);
}

static void print_expr_assign_op(FILE* f, Expr* expr, bool fully_evaluated)
{
	glsl_print_expr(f, expr->u.assign_op.lvalue, fully_evaluated);
	fprintf(f, " = ");
	glsl_print_expr(f, expr->u.assign_op.rvalue, fully_evaluated);
}

static void print_expr_sequence(FILE* f, Expr* expr, bool fully_evaluated)
{
	glsl_print_expr(f, expr->u.sequence.all_these, fully_evaluated);
	fprintf(f, ", ");
	glsl_print_expr(f, expr->u.sequence.then_this, fully_evaluated);
}

static void print_expr_intrinsic(FILE* f, Expr* expr, bool fully_evaluated)
{
   const char* name;

   switch (expr->flavour)
   {
      case EXPR_INTRINSIC_TEXTURE_2D_BIAS:
         name = "$$texture_2d_bias";
         break;
      case EXPR_INTRINSIC_TEXTURE_2D_LOD:
         name = "$$texture_2d_lod";
         break;
      case EXPR_INTRINSIC_TEXTURE_CUBE_BIAS:
         name = "$$texture_cube_bias";
         break;
      case EXPR_INTRINSIC_TEXTURE_CUBE_LOD:
         name = "$$texture_cube_lod";
         break;
      case EXPR_INTRINSIC_RSQRT:
         name = "$$rsqrt";
         break;
      case EXPR_INTRINSIC_RCP:
         name = "$$rcp";
         break;
      case EXPR_INTRINSIC_LOG2:
         name = "$$log2";
         break;
      case EXPR_INTRINSIC_EXP2:
         name = "$$exp2";
         break;
      case EXPR_INTRINSIC_CEIL:
         name = "$$ceil";
         break;
      case EXPR_INTRINSIC_FLOOR:
         name = "$$floor";
         break;
      case EXPR_INTRINSIC_SIGN:
         name = "$$sign";
         break;
      case EXPR_INTRINSIC_TRUNC:
         name = "$$trunc";
         break;
      case EXPR_INTRINSIC_NEAREST:
         name = "$$nearest";
         break;
      case EXPR_INTRINSIC_MIN:
         name = "$$min";
         break;
      case EXPR_INTRINSIC_MAX:
         name = "$$max";
         break;
		default:
			UNREACHABLE();
			return;
   }

   fprintf(f, "%s(", name);
   print_expr_chain(f, expr->u.intrinsic.args, fully_evaluated);
	fprintf(f, ")");
}

void glsl_print_expr(FILE* f, Expr* expr, bool fully_evaluated)
{
	// Don't print out the whole expression if we are allowed to compress it.
	if (fully_evaluated && expr->compile_time_value)
	{
		glsl_print_compile_time_value(f, expr->type, expr->compile_time_value);
		return;
	}

	switch (expr->flavour)
	{
		case EXPR_VALUE:
			print_expr_value(f, expr, fully_evaluated);
			return;

		case EXPR_INSTANCE:
			print_expr_instance(f, expr, fully_evaluated);
			return;

		case EXPR_SUBSCRIPT:
			print_expr_subscript(f, expr, fully_evaluated);
			return;

		case EXPR_FUNCTION_CALL:
			print_expr_function_call(f, expr, fully_evaluated);
			return;

		case EXPR_PRIM_CONSTRUCTOR_CALL:
			print_expr_prim_constructor_call(f, expr, fully_evaluated);
			return;

		case EXPR_TYPE_CONSTRUCTOR_CALL:
			print_expr_type_constructor_call(f, expr, fully_evaluated);
			return;

		case EXPR_FIELD_SELECTOR_STRUCT:
			print_expr_field_selector_struct(f, expr, fully_evaluated);
			return;

		case EXPR_FIELD_SELECTOR_SWIZZLE:
			print_expr_field_selector_swizzle(f, expr, fully_evaluated);
			return;

		case EXPR_POST_INC:
		case EXPR_POST_DEC:
		case EXPR_PRE_INC:
		case EXPR_PRE_DEC:
		case EXPR_ARITH_NEGATE:
		case EXPR_LOGICAL_NOT:
			// The postfix operators don't really need the brackets, but never mind.
			fprintf(f, "(");
			print_expr_unary_op(f, expr, fully_evaluated);
			fprintf(f, ")");
			return;

		case EXPR_MUL:
		case EXPR_DIV:
		case EXPR_ADD:
		case EXPR_SUB:
		case EXPR_LESS_THAN:
		case EXPR_LESS_THAN_EQUAL:
		case EXPR_GREATER_THAN:
		case EXPR_GREATER_THAN_EQUAL:
		case EXPR_EQUAL:
		case EXPR_NOT_EQUAL:
		case EXPR_LOGICAL_AND:
		case EXPR_LOGICAL_XOR:
		case EXPR_LOGICAL_OR:
			fprintf(f, "(");
			print_expr_binary_op(f, expr, fully_evaluated);
			fprintf(f, ")");
			return;

		case EXPR_CONDITIONAL:
			fprintf(f, "(");
			print_expr_cond_op(f, expr, fully_evaluated);
			fprintf(f, ")");
			return;

		case EXPR_ASSIGN:
			fprintf(f, "(");
			print_expr_assign_op(f, expr, fully_evaluated);
			fprintf(f, ")");
			return;

		case EXPR_SEQUENCE:
			fprintf(f, "(");
			print_expr_sequence(f, expr, fully_evaluated);
			fprintf(f, ")");
			return;

      case EXPR_INTRINSIC_TEXTURE_2D_BIAS:
      case EXPR_INTRINSIC_TEXTURE_2D_LOD:
      case EXPR_INTRINSIC_TEXTURE_CUBE_BIAS:
      case EXPR_INTRINSIC_TEXTURE_CUBE_LOD:
      case EXPR_INTRINSIC_RSQRT:
      case EXPR_INTRINSIC_RCP:
      case EXPR_INTRINSIC_LOG2:
      case EXPR_INTRINSIC_EXP2:
      case EXPR_INTRINSIC_CEIL:
      case EXPR_INTRINSIC_FLOOR:
      case EXPR_INTRINSIC_SIGN:
      case EXPR_INTRINSIC_TRUNC:
      case EXPR_INTRINSIC_NEAREST:
      case EXPR_INTRINSIC_MIN:
      case EXPR_INTRINSIC_MAX:
			print_expr_intrinsic(f, expr, fully_evaluated);
         return;

		default:
			UNREACHABLE();
			return;
	}
}


//
// Statement printing.
//

static void print_statement_ast(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon)
{
	StatementChainNode* decl;
	for (decl = statement->u.ast.decls->first; decl; decl = decl->next)
	{
		glsl_print_statement(f, decl->statement, fully_evaluated, indent_depth, suppress_semicolon);
		if (decl->next) fprintf(f, "\n");
	}
}

static void print_statement_decl_list(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon)
{
	StatementChainNode* decl;
	for (decl = statement->u.decl_list.decls->first; decl; decl = decl->next)
	{
		glsl_print_statement(f, decl->statement, fully_evaluated, indent_depth, suppress_semicolon);
		if (decl->next) fprintf(f, "\n");
	}
}

static void print_statement_function_def(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon)
{
	int i;

	Symbol* header = statement->u.function_def.header;
	Statement* body = statement->u.function_def.body;

	for (i = 0; i < indent_depth; i++) fprintf(f, "\t");

	fprintf(f, "%s %s(", header->type->u.function_type.return_type->name, header->name);

	for (i = 0; i < header->type->u.function_type.param_count; i++)
	{
		print_qualifiers(f, header->type->u.function_type.params[i]);
		fprintf(f, "%s %s", header->type->u.function_type.params[i]->type->name, header->type->u.function_type.params[i]->name);
		if (i < header->type->u.function_type.param_count-1) fprintf(f, ", ");
	}

	fprintf(f, ")\n");

	glsl_print_statement(f, body, fully_evaluated, indent_depth, suppress_semicolon);
}

static void print_statement_var_def(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon)
{
	int i;

	Symbol* var = statement->u.var_decl.var;
	Expr* initializer = statement->u.var_decl.initializer;

	for (i = 0; i < indent_depth; i++) fprintf(f, "\t");

	print_qualifiers(f, var);
	fprintf(f, "%s %s", var->type->name, var->name);

	if (initializer)
	{
		fprintf(f, " = ");
		glsl_print_expr(f, initializer, fully_evaluated);
	}

	if (!suppress_semicolon) fprintf(f, ";");
}

static void print_statement_compound(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon)
{
	int i;
	StatementChainNode* s;

	for (i = 0; i < indent_depth; i++) fprintf(f, "\t");
	fprintf(f, "{\n");

	for (s = statement->u.compound.statements->first; s; s = s->next)
	{
		glsl_print_statement(f, s->statement, fully_evaluated, indent_depth + 1, suppress_semicolon);
		fprintf(f, "\n");
	}
	
	for (i = 0; i < indent_depth; i++) fprintf(f, "\t");
	fprintf(f, "}");

#if 0
	// Dumps count.
	fprintf(f, " [count:%d]", statement->compound.statements->count);
#endif
}

static void print_statement_expr(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon)
{
	int i;
	for (i = 0; i < indent_depth; i++) fprintf(f, "\t");
	glsl_print_expr(f, statement->u.expr.expr, fully_evaluated);
	if (!suppress_semicolon) fprintf(f, ";");
}

static void print_statement_selection(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon)
{
	int i;

	for (i = 0; i < indent_depth; i++) fprintf(f, "\t");
	fprintf(f, "if (");
	glsl_print_expr(f, statement->u.selection.cond, fully_evaluated);
	fprintf(f, ")");

	if (STATEMENT_COMPOUND == statement->u.selection.if_true->flavour)
	{
		fprintf(f, "\n");
		glsl_print_statement(f, statement->u.selection.if_true, fully_evaluated, indent_depth, suppress_semicolon);
	}
	else
	{
		fprintf(f, " ");
		glsl_print_statement(f, statement->u.selection.if_true, fully_evaluated, 0, suppress_semicolon);
	}

	if (statement->u.selection.if_false)
	{
		fprintf(f, "\n");
		for (i = 0; i < indent_depth; i++) fprintf(f, "\t");
		fprintf(f, "else");
		
		if (STATEMENT_COMPOUND == statement->u.selection.if_true->flavour)
		{
			fprintf(f, "\n");
			glsl_print_statement(f, statement->u.selection.if_false, fully_evaluated, indent_depth, suppress_semicolon);
		}
		else
		{
			fprintf(f, " ");
			glsl_print_statement(f, statement->u.selection.if_false, fully_evaluated, 0, suppress_semicolon);
		}
	}
}

static void print_statement_iterator_for(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon)
{
	int i;

	for (i = 0; i < indent_depth; i++) fprintf(f, "\t");
	fprintf(f, "for (");
	glsl_print_statement(f, statement->u.iterator_for.init, fully_evaluated, 0, true);
	fprintf(f, "; ");
	glsl_print_expr(f, statement->u.iterator_for.test, fully_evaluated);
	fprintf(f, "; ");
	glsl_print_expr(f, statement->u.iterator_for.loop, fully_evaluated);
	fprintf(f, ")\n");
	
	glsl_print_statement(f, statement->u.iterator_for.block, fully_evaluated, indent_depth, suppress_semicolon);
}

static void print_statement_iterator_while(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon)
{
	int i;

	for (i = 0; i < indent_depth; i++) fprintf(f, "\t");
	fprintf(f, "while (");
	glsl_print_statement(f, statement->u.iterator_while.cond_or_decl, fully_evaluated, 0, true);
	fprintf(f, ")\n");
	
	glsl_print_statement(f, statement->u.iterator_while.block, fully_evaluated, indent_depth, suppress_semicolon);
}

static void print_statement_iterator_do_while(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon)
{
	int i;

	for (i = 0; i < indent_depth; i++) fprintf(f, "\t");
	fprintf(f, "do\n");

	glsl_print_statement(f, statement->u.iterator_do_while.block, fully_evaluated, indent_depth, suppress_semicolon);

	for (i = 0; i < indent_depth; i++) fprintf(f, "\t");
	fprintf(f, "while (");
	glsl_print_expr(f, statement->u.iterator_do_while.cond, fully_evaluated);
	fprintf(f, ")");

	if (!suppress_semicolon) fprintf(f, ";");
}

static void print_statement_jump(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon)
{
	int i;

	for (i = 0; i < indent_depth; i++) fprintf(f, "\t");

	switch (statement->flavour)
	{
		case STATEMENT_CONTINUE:
			fprintf(f, "continue");
			break;
		case STATEMENT_BREAK:
			fprintf(f, "break");
			break;
		case STATEMENT_DISCARD:
			fprintf(f, "discard");
			break;
		case STATEMENT_RETURN:
			fprintf(f, "return");
			break;
		case STATEMENT_RETURN_EXPR:
			fprintf(f, "return ");
			glsl_print_expr(f, statement->u.return_expr.expr, fully_evaluated);
			break;
		default:
			UNREACHABLE();
			return;
	}

	if (!suppress_semicolon) fprintf(f, ";");
}

static void print_statement_null(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon)
{
	int i;

   UNUSED(statement);
   UNUSED(fully_evaluated);

	for (i = 0; i < indent_depth; i++) fprintf(f, "\t");

	if (!suppress_semicolon) fprintf(f, ";");
}

void glsl_print_statement(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon)
{
	switch (statement->flavour)
	{
		case STATEMENT_AST:
			print_statement_ast(f, statement, fully_evaluated, indent_depth, suppress_semicolon);
			break;

		case STATEMENT_DECL_LIST:
			print_statement_decl_list(f, statement, fully_evaluated, indent_depth, suppress_semicolon);
			break;

		case STATEMENT_FUNCTION_DEF:
			print_statement_function_def(f, statement, fully_evaluated, indent_depth, suppress_semicolon);
			break;

		case STATEMENT_VAR_DECL:
			print_statement_var_def(f, statement, fully_evaluated, indent_depth, suppress_semicolon);
			break;

		case STATEMENT_COMPOUND:
			print_statement_compound(f, statement, fully_evaluated, indent_depth, suppress_semicolon);
			break;

		case STATEMENT_EXPR:
			print_statement_expr(f, statement, fully_evaluated, indent_depth, suppress_semicolon);
			break;

		case STATEMENT_SELECTION:
			print_statement_selection(f, statement, fully_evaluated, indent_depth, suppress_semicolon);
			break;

		case STATEMENT_ITERATOR_FOR:
			print_statement_iterator_for(f, statement, fully_evaluated, indent_depth, suppress_semicolon);
			break;

		case STATEMENT_ITERATOR_WHILE:
			print_statement_iterator_while(f, statement, fully_evaluated, indent_depth, suppress_semicolon);
			break;

		case STATEMENT_ITERATOR_DO_WHILE:
			print_statement_iterator_do_while(f, statement, fully_evaluated, indent_depth, suppress_semicolon);
			break;

		case STATEMENT_CONTINUE:
		case STATEMENT_BREAK:
		case STATEMENT_DISCARD:
		case STATEMENT_RETURN:
		case STATEMENT_RETURN_EXPR:
			print_statement_jump(f, statement, fully_evaluated, indent_depth, suppress_semicolon);
			break;

		case STATEMENT_NULL:
			print_statement_null(f, statement, fully_evaluated, indent_depth, suppress_semicolon);
			break;

		default:
			UNREACHABLE();
			break;
	}

	// Add statement dump commands here if you need them.
#if 0
	// Dumps flags.
	fprintf(f, " [%1d]", statement->flags);
#endif
}

#else
/*
   keep Metaware happy by providing an exported symbol
*/

void glsl_print_statement(FILE* f, Statement* statement, bool fully_evaluated, int indent_depth, bool suppress_semicolon)
{
   UNUSED(f);
   UNUSED(statement);
   UNUSED(fully_evaluated);
   UNUSED(indent_depth);
   UNUSED(suppress_semicolon);

   UNREACHABLE();
}
#endif // _DEBUG