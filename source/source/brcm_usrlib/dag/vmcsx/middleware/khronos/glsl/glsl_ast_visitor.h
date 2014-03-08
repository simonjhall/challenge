/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_AST_VISITOR_H
#define GLSL_AST_VISITOR_H

#include "middleware/khronos/glsl/glsl_ast.h"


// All visitor functions must implement these signatures.
// data will be passed through the accept functions and back to the visitor.
typedef Expr* (*ExprPreVisitor)(Expr* expr, void* data);
typedef Statement* (*StatementPreVisitor)(Statement* statement, void* data);

// Prefix acceptor functions.
// - calls visitor for the current node as ev(statement, data) or sv(statement, data);
// - recurses on the children of the node *that the visitor passes back*, or not if the visitor passed NULL.
//   (this allows the prefix visitor to mutate the node)
// ev and sv can be NULL, in which case they will not be called.
void glsl_expr_accept_prefix(Expr* expr, void* data, ExprPreVisitor eprev);
void glsl_statement_accept_prefix(Statement* statement, void* data, StatementPreVisitor sprev, ExprPreVisitor eprev);

// All visitor functions must implement these signatures.
// data will be passed through the accept functions and back to the visitor.
typedef void (*ExprPostVisitor)(Expr* expr, void* data);
typedef void (*StatementPostVisitor)(Statement* statement, void* data);

// Postfix acceptor functions.
// - recurses on the children of the node;
// - calls visitor for the current node as ev(statement, data) or sv(statement, data).
// ev and sv can be NULL, in which case they will not be called.
void glsl_expr_accept_postfix(Expr* expr, void* data, ExprPostVisitor epostv);
void glsl_statement_accept_postfix(Statement* statement, void* data, StatementPostVisitor spostv, ExprPostVisitor epostv);

// Prefix AND postfix acceptor functions.
// As per the above.
void glsl_expr_accept(Expr* expr, void* data, ExprPreVisitor eprev, ExprPostVisitor epostv);
void glsl_statement_accept(Statement* statement, void* data, StatementPreVisitor sprev, ExprPreVisitor eprev, StatementPostVisitor spostv, ExprPostVisitor epostv);

#endif // AST_VISITOR_H