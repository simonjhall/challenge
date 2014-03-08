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

#include <stdlib.h>

#include "middleware/khronos/glsl/glsl_dataflow_visitor.h"
#include "middleware/khronos/glsl/glsl_stack.h"

void glsl_dataflow_accept_towards_leaves(Dataflow* dataflow, void* data, DataflowPreVisitor dprev, DataflowPostVisitor dpostv, int pass)
{
   STACK_CHECK();

	if (dataflow && dataflow->pass != pass)
	{
      DataflowChainNode *node;      

      dataflow->pass = pass;

		if (dprev)
      {
         Dataflow* alt_dataflow = dprev(dataflow, data);

         if (alt_dataflow != dataflow)
         {
            glsl_dataflow_accept_towards_leaves(alt_dataflow, data, dprev, dpostv, pass);
            return;
         }
      }

		switch (dataflow->flavour)
		{
			case DATAFLOW_CONST_BOOL:
			case DATAFLOW_CONST_INT:
			case DATAFLOW_CONST_FLOAT:
         case DATAFLOW_CONST_SAMPLER:
				break;

			case DATAFLOW_UNIFORM:
			case DATAFLOW_ATTRIBUTE:
			case DATAFLOW_VARYING:
            break;

         case DATAFLOW_UNIFORM_OFFSET:
            glsl_dataflow_accept_towards_leaves(dataflow->u.linkable_value_offset.linkable_value, data, dprev, dpostv, pass);
            glsl_dataflow_accept_towards_leaves(dataflow->u.linkable_value_offset.offset, data, dprev, dpostv, pass);
				break;
			
			case DATAFLOW_ARITH_NEGATE:
			case DATAFLOW_LOGICAL_NOT:
         case DATAFLOW_INTRINSIC_FLOATTOINT_TRUNC:
         case DATAFLOW_INTRINSIC_FLOATTOINT_NEAREST:
         case DATAFLOW_INTRINSIC_FLOATTOBOOL:
         case DATAFLOW_INTRINSIC_INTTOFLOAT:
         case DATAFLOW_INTRINSIC_INTTOBOOL:
         case DATAFLOW_INTRINSIC_BOOLTOINT:
         case DATAFLOW_INTRINSIC_BOOLTOFLOAT:
			case DATAFLOW_INTRINSIC_RSQRT:
			case DATAFLOW_INTRINSIC_RCP:
			case DATAFLOW_INTRINSIC_LOG2:
			case DATAFLOW_INTRINSIC_EXP2:
         case DATAFLOW_INTRINSIC_CEIL:
         case DATAFLOW_INTRINSIC_FLOOR:
         case DATAFLOW_INTRINSIC_SIGN:
         case DATAFLOW_MOV:
         case DATAFLOW_COERCE_TOFLOAT:
         case DATAFLOW_COERCE_TOINT:
				glsl_dataflow_accept_towards_leaves(dataflow->u.unary_op.operand, data, dprev, dpostv, pass);
				break;

			case DATAFLOW_MUL:
			case DATAFLOW_ADD:
			case DATAFLOW_SUB:
			case DATAFLOW_RSUB:
			case DATAFLOW_LESS_THAN:
			case DATAFLOW_LESS_THAN_EQUAL:
			case DATAFLOW_GREATER_THAN:
			case DATAFLOW_GREATER_THAN_EQUAL:
			case DATAFLOW_EQUAL:
			case DATAFLOW_NOT_EQUAL:
			case DATAFLOW_LOGICAL_AND:
			case DATAFLOW_LOGICAL_XOR:
			case DATAFLOW_LOGICAL_OR:
			case DATAFLOW_INTRINSIC_MIN:
			case DATAFLOW_INTRINSIC_MAX:
				glsl_dataflow_accept_towards_leaves(dataflow->u.binary_op.left, data, dprev, dpostv, pass);
				glsl_dataflow_accept_towards_leaves(dataflow->u.binary_op.right, data, dprev, dpostv, pass);
				break;

			case DATAFLOW_CONDITIONAL:
				glsl_dataflow_accept_towards_leaves(dataflow->u.cond_op.cond, data, dprev, dpostv, pass);
				glsl_dataflow_accept_towards_leaves(dataflow->u.cond_op.true_value, data, dprev, dpostv, pass);
				glsl_dataflow_accept_towards_leaves(dataflow->u.cond_op.false_value, data, dprev, dpostv, pass);
				break;

			case DATAFLOW_TEX_SET_COORD_S:
			case DATAFLOW_TEX_SET_COORD_T:
			case DATAFLOW_TEX_SET_COORD_R:
			case DATAFLOW_TEX_SET_BIAS:
			case DATAFLOW_TEX_SET_LOD:
         {
				glsl_dataflow_accept_towards_leaves(dataflow->u.texture_lookup_set.sampler, data, dprev, dpostv, pass);
				glsl_dataflow_accept_towards_leaves(dataflow->u.texture_lookup_set.param, data, dprev, dpostv, pass);

				break;
         }
			case DATAFLOW_TEX_GET_CMP_R:
			case DATAFLOW_TEX_GET_CMP_G:
			case DATAFLOW_TEX_GET_CMP_B:
			case DATAFLOW_TEX_GET_CMP_A:
         {
				glsl_dataflow_accept_towards_leaves(dataflow->u.texture_lookup_get.sampler, data, dprev, dpostv, pass);

				break;
         }
         case DATAFLOW_THREADSWITCH:
			case DATAFLOW_FRAG_GET_X:
			case DATAFLOW_FRAG_GET_Y:
			case DATAFLOW_FRAG_GET_Z:
			case DATAFLOW_FRAG_GET_W:
			case DATAFLOW_FRAG_GET_PC_X:
			case DATAFLOW_FRAG_GET_PC_Y:
			case DATAFLOW_FRAG_GET_FF:
            break;
			case DATAFLOW_FRAG_SET_R:
			case DATAFLOW_FRAG_SET_G:
			case DATAFLOW_FRAG_SET_B:
			case DATAFLOW_FRAG_SET_A:
         {
				glsl_dataflow_accept_towards_leaves(dataflow->u.fragment_set.param, data, dprev, dpostv, pass);
            if (dataflow->u.fragment_set.discard)
   				glsl_dataflow_accept_towards_leaves(dataflow->u.fragment_set.discard, data, dprev, dpostv, pass);

				break;
         }
#ifdef __VIDEOCORE4__
         case DATAFLOW_VARYING_C:
         case DATAFLOW_FRAG_GET_COL:
            break;
         case DATAFLOW_FRAG_SUBMIT_Z:
         case DATAFLOW_FRAG_SUBMIT_MS:
         case DATAFLOW_FRAG_SUBMIT_ALL:
         case DATAFLOW_FRAG_SUBMIT_STENCIL:
         case DATAFLOW_FRAG_SUBMIT_R0:
         case DATAFLOW_FRAG_SUBMIT_R1:
         case DATAFLOW_FRAG_SUBMIT_R2:
         case DATAFLOW_FRAG_SUBMIT_R3:
         case DATAFLOW_TMU_SWAP:
				glsl_dataflow_accept_towards_leaves(dataflow->u.fragment_set.param, data, dprev, dpostv, pass);
            if (dataflow->u.fragment_set.discard) glsl_dataflow_accept_towards_leaves(dataflow->u.fragment_set.discard, data, dprev, dpostv, pass);
            break;
         case DATAFLOW_VERTEX_SET:
         case DATAFLOW_VPM_READ_SETUP:
         case DATAFLOW_VPM_WRITE_SETUP:
				glsl_dataflow_accept_towards_leaves(dataflow->u.vertex_set.param, data, dprev, dpostv, pass);
            break;
         case DATAFLOW_PACK_COL_R:
         case DATAFLOW_PACK_COL_G:
         case DATAFLOW_PACK_COL_B:
         case DATAFLOW_PACK_COL_A:
         case DATAFLOW_PACK_16A:
         case DATAFLOW_PACK_16B:
         case DATAFLOW_PACK_FB_R:
         case DATAFLOW_PACK_FB_B:
				glsl_dataflow_accept_towards_leaves(dataflow->u.pack.operand, data, dprev, dpostv, pass);
            if (dataflow->u.pack.background != NULL)
				   glsl_dataflow_accept_towards_leaves(dataflow->u.pack.background, data, dprev, dpostv, pass);
            break;
         case DATAFLOW_UNPACK_COL_R:
         case DATAFLOW_UNPACK_COL_G:
         case DATAFLOW_UNPACK_COL_B:
         case DATAFLOW_UNPACK_COL_A:
         case DATAFLOW_UNPACK_16A:
         case DATAFLOW_UNPACK_16A_F:
         case DATAFLOW_UNPACK_16B:
         case DATAFLOW_UNPACK_16B_F:
         case DATAFLOW_UNPACK_8A:
         case DATAFLOW_UNPACK_8B:
         case DATAFLOW_UNPACK_8C:
         case DATAFLOW_UNPACK_8D:
         case DATAFLOW_UNPACK_8R:
         case DATAFLOW_UNPACK_FB_R:
         case DATAFLOW_UNPACK_FB_B:
         case DATAFLOW_BITWISE_NOT:
				glsl_dataflow_accept_towards_leaves(dataflow->u.unary_op.operand, data, dprev, dpostv, pass);
				break;
         case DATAFLOW_UNPACK_PLACEHOLDER_R:
         case DATAFLOW_UNPACK_PLACEHOLDER_B:
            glsl_dataflow_accept_towards_leaves(dataflow->u.binary_op.left, data, dprev, dpostv, pass);
            glsl_dataflow_accept_towards_leaves(dataflow->u.binary_op.right, data, dprev, dpostv, pass);
				break;
         case DATAFLOW_LOGICAL_SHR:
         case DATAFLOW_SHIFT_RIGHT:
         case DATAFLOW_BITWISE_AND:
         case DATAFLOW_BITWISE_OR:
         case DATAFLOW_V8MULD:
         case DATAFLOW_BITWISE_XOR:
         case DATAFLOW_V8MIN:
         case DATAFLOW_V8MAX:
         case DATAFLOW_V8ADDS:
         case DATAFLOW_V8SUBS:
         case DATAFLOW_INTEGER_ADD:
				glsl_dataflow_accept_towards_leaves(dataflow->u.binary_op.left, data, dprev, dpostv, pass);
				glsl_dataflow_accept_towards_leaves(dataflow->u.binary_op.right, data, dprev, dpostv, pass);
				break;
#ifdef __BCM2708A0__
         case DATAFLOW_INDEXED_UNIFORM_SAMPLER:
#else
         case DATAFLOW_UNIFORM_ADDRESS:
#endif
            glsl_dataflow_accept_towards_leaves(dataflow->u.indexed_uniform_sampler.uniform, data, dprev, dpostv, pass);
            break;
         case DATAFLOW_SCOREBOARD_WAIT:
            break;
#ifndef __BCM2708A0__
         case DATAFLOW_TEX_SET_DIRECT:
            vcos_assert(dataflow->u.texture_lookup_set.sampler == NULL);
				glsl_dataflow_accept_towards_leaves(dataflow->u.texture_lookup_set.param, data, dprev, dpostv, pass);
            break;
#endif
#endif
			default:
				UNREACHABLE();
				return;
		}

      for (node = dataflow->iodependencies.first; node; node = node->next)
   		glsl_dataflow_accept_towards_leaves(node->dataflow, data, dprev, dpostv, pass);
		
		if (dpostv) dpostv(dataflow, data);
	}
}

void glsl_dataflow_accept_towards_leaves_prefix(Dataflow* dataflow, void* data, DataflowPreVisitor dprev, int pass)
{
	glsl_dataflow_accept_towards_leaves(dataflow, data, dprev, NULL, pass);
}

void glsl_dataflow_accept_towards_leaves_postfix(Dataflow* dataflow, void* data, DataflowPostVisitor dpostv, int pass)
{
	glsl_dataflow_accept_towards_leaves(dataflow, data, NULL, dpostv, pass);
}


void glsl_dataflow_accept_from_leaves(DataflowChain* pool, void* data, DataflowPreVisitor dprev, DataflowPostVisitor dpostv, int pass)
{
	DataflowChainNode* node;

   STACK_CHECK();

	for (node = pool->first; node; node = node->next)
	{
		Dataflow* dataflow = node->dataflow;

		if (dataflow && dataflow->pass != pass)
		{
         dataflow->pass = pass;

			if (dprev) dataflow = dprev(dataflow, data);
			if (!dataflow) return;

			glsl_dataflow_accept_from_leaves(&dataflow->dependents, data, dprev, dpostv, pass);
			glsl_dataflow_accept_from_leaves(&dataflow->iodependents, data, dprev, dpostv, pass);

			if (dpostv) dpostv(dataflow, data);
		}
	}
}

void glsl_dataflow_accept_from_leaves_prefix(DataflowChain* pool, void* data, DataflowPreVisitor dprev, int pass)
{
	glsl_dataflow_accept_from_leaves(pool, data, dprev, NULL, pass);
}

void glsl_dataflow_accept_from_leaves_postfix(DataflowChain* pool, void* data, DataflowPostVisitor dpostv, int pass)
{
	glsl_dataflow_accept_from_leaves(pool, data, NULL, dpostv, pass);
}