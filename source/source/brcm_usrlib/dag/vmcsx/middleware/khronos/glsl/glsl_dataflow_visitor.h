/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_DATAFLOW_VISITOR_H
#define GLSL_DATAFLOW_VISITOR_H

#include "middleware/khronos/glsl/glsl_dataflow.h"


// Compare with ast_visitor.h for usage instructions.

typedef Dataflow* (*DataflowPreVisitor)(Dataflow* dataflow, void* data);
typedef void (*DataflowPostVisitor)(Dataflow* dataflow, void* data);

// These functions visit all nodes that "dataflow" depends on.
void glsl_dataflow_accept_towards_leaves_prefix(Dataflow* dataflow, void* data, DataflowPreVisitor dprev, int pass);
void glsl_dataflow_accept_towards_leaves_postfix(Dataflow* dataflow, void* data, DataflowPostVisitor dpostv, int pass);
void glsl_dataflow_accept_towards_leaves(Dataflow* dataflow, void* data, DataflowPreVisitor dprev, DataflowPostVisitor dpostv, int pass);

// These functions visit all nodes dependent on the nodes in pool.
void glsl_dataflow_accept_from_leaves_prefix(DataflowChain* pool, void* data, DataflowPreVisitor dprev, int pass);
void glsl_dataflow_accept_from_leaves_postfix(DataflowChain* pool, void* data, DataflowPostVisitor dpostv, int pass);
void glsl_dataflow_accept_from_leaves(DataflowChain* pool, void* data, DataflowPreVisitor dprev, DataflowPostVisitor dpostv, int pass);


#endif // DATAFLOW_VISITOR_H