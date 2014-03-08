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

#include "middleware/khronos/glsl/glsl_dataflow_visitor.h"
#include "middleware/khronos/glsl/glsl_dataflow_print.h"
#include "middleware/khronos/glsl/glsl_map.h"
#include "middleware/khronos/glsl/glsl_fastmem.h"
#include "middleware/khronos/glsl/2708/glsl_allocator_4.h"

#ifndef NDEBUG

#include <stdlib.h>

#define GRAPHVIZ_GRAPH_NAME			"Dataflow"
#define GRAPHVIZ_CLUSTER_PREFIX		"line_num"

static void dpostv_gather_by_line_num(Dataflow* dataflow, void* data)
{
   Map *map = (Map *)data;

	// See whether dataflow->line_num is in the map.
	DataflowChain* chain = (DataflowChain *)glsl_map_get(map, (void *)dataflow->line_num, true);

	// If it's not in the map, insert it.
	if (!chain)
	{
		chain = (DataflowChain *)malloc_fast(sizeof(DataflowChain));

		// Init.
		glsl_dataflow_chain_init(chain);

		// Insert.
		glsl_map_put(map, (void *)dataflow->line_num, chain);
	}

	// Add this node to the chain.
	glsl_dataflow_chain_append(chain, dataflow);
}

typedef enum
{
   EDGE_SOLID,
   EDGE_DASHED,
   EDGE_SEQUENCE,

	EDGE_STYLE_COUNT
} EdgeStyle;

static void print_edge(FILE* f, EdgeStyle style, Dataflow* supplier, Dataflow* consumer)
{
   fprintf(f, "\tn%x -> n%x", (size_t)consumer, (size_t)supplier);

   switch (style) {
   case EDGE_SOLID:
      break;
   case EDGE_DASHED:
      fprintf(f, " [style=dashed]");
      break;
   case EDGE_SEQUENCE:
      fprintf(f, " [constraint=false,color=red]");
      break;
   default:
      UNREACHABLE();
      break;
   }

   fprintf(f, ";\n");
}

static void print_edges(FILE* f, Dataflow* dataflow, bool cross)
{
   DataflowChainNode *node;

   // cross: print only edges that do cross a cluster boundary
   // !cross: print only edges that do not cross a cluster boundary
   switch (dataflow->flavour)
	{
		case DATAFLOW_CONST_BOOL:
		case DATAFLOW_CONST_INT:
		case DATAFLOW_CONST_FLOAT:
      case DATAFLOW_CONST_SAMPLER:
#ifdef __VIDEOCORE4__
#ifdef __BCM2708A0__
      case DATAFLOW_INDEXED_UNIFORM_SAMPLER:
#else
      case DATAFLOW_UNIFORM_ADDRESS:
#endif
#endif
			break;

		case DATAFLOW_UNIFORM:
		case DATAFLOW_ATTRIBUTE:
		case DATAFLOW_VARYING:
         break;

      case DATAFLOW_UNIFORM_OFFSET:
         if (cross == (dataflow->line_num != dataflow->u.linkable_value_offset.linkable_value->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.linkable_value_offset.linkable_value, dataflow);
			}
         if (cross == (dataflow->line_num != dataflow->u.linkable_value_offset.offset->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.linkable_value_offset.offset, dataflow);
			}
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
#ifdef __VIDEOCORE4__
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
#endif
         if (cross == (dataflow->line_num != dataflow->u.unary_op.operand->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.unary_op.operand, dataflow);
			}
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
#ifdef __VIDEOCORE4__
      case DATAFLOW_BITWISE_NOT:
      case DATAFLOW_BITWISE_AND:
      case DATAFLOW_BITWISE_OR:
      case DATAFLOW_BITWISE_XOR:
      case DATAFLOW_V8MULD:
      case DATAFLOW_V8MIN:
      case DATAFLOW_V8MAX:
      case DATAFLOW_V8ADDS:
      case DATAFLOW_V8SUBS:
      case DATAFLOW_INTEGER_ADD:
      case DATAFLOW_SHIFT_RIGHT:
      case DATAFLOW_LOGICAL_SHR:
#endif
         if (cross == (dataflow->line_num != dataflow->u.binary_op.left->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.binary_op.left, dataflow);
			}
         if (cross == (dataflow->line_num != dataflow->u.binary_op.right->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.binary_op.right, dataflow);
			}
			break;

#ifdef __VIDEOCORE4__
      case DATAFLOW_PACK_COL_R:
      case DATAFLOW_PACK_COL_G:
      case DATAFLOW_PACK_COL_B:
      case DATAFLOW_PACK_COL_A:
      case DATAFLOW_PACK_16A:
      case DATAFLOW_PACK_16B:
      case DATAFLOW_PACK_FB_R:
      case DATAFLOW_PACK_FB_B:
         if (cross == (dataflow->line_num != dataflow->u.pack.operand->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.pack.operand, dataflow);
			}
         if (cross == (dataflow->line_num != dataflow->u.pack.background->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.pack.background, dataflow);
			}
			break;
#endif
		case DATAFLOW_CONDITIONAL:
         if (cross == (dataflow->line_num != dataflow->u.cond_op.cond->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.cond_op.cond, dataflow);
			}
         if (cross == (dataflow->line_num != dataflow->u.cond_op.true_value->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.cond_op.true_value, dataflow);
			}
         if (cross == (dataflow->line_num != dataflow->u.cond_op.false_value->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.cond_op.false_value, dataflow);
			}
			break;

		case DATAFLOW_TEX_SET_COORD_S:
		case DATAFLOW_TEX_SET_COORD_T:
		case DATAFLOW_TEX_SET_COORD_R:
      case DATAFLOW_TEX_SET_BIAS:
		case DATAFLOW_TEX_SET_LOD:
#ifdef __VIDEOCORE4__
#ifndef __BCM2708A0__
      case DATAFLOW_TEX_SET_DIRECT:
#endif
#endif
         if (cross == (dataflow->line_num != dataflow->u.texture_lookup_set.sampler->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.texture_lookup_set.sampler, dataflow);
			}
         if (cross == (dataflow->line_num != dataflow->u.texture_lookup_set.param->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.texture_lookup_set.param, dataflow);
			}
			break;

		case DATAFLOW_TEX_GET_CMP_R:
		case DATAFLOW_TEX_GET_CMP_G:
		case DATAFLOW_TEX_GET_CMP_B:
		case DATAFLOW_TEX_GET_CMP_A:
         if (cross == (dataflow->line_num != dataflow->u.texture_lookup_get.sampler->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.texture_lookup_get.sampler, dataflow);
			}
			break;
      
      case DATAFLOW_THREADSWITCH:
      case DATAFLOW_FRAG_GET_X:
      case DATAFLOW_FRAG_GET_Y:
      case DATAFLOW_FRAG_GET_Z:
      case DATAFLOW_FRAG_GET_W:
      case DATAFLOW_FRAG_GET_PC_X:
      case DATAFLOW_FRAG_GET_PC_Y:
      case DATAFLOW_FRAG_GET_FF:
#ifdef __VIDEOCORE4__
      case DATAFLOW_VARYING_C:
      case DATAFLOW_FRAG_GET_COL:
      case DATAFLOW_SCOREBOARD_WAIT:
#endif
         break;

		case DATAFLOW_FRAG_SET_R:
		case DATAFLOW_FRAG_SET_G:
		case DATAFLOW_FRAG_SET_B:
		case DATAFLOW_FRAG_SET_A:
#ifdef __VIDEOCORE4__
      case DATAFLOW_FRAG_SUBMIT_STENCIL:
      case DATAFLOW_FRAG_SUBMIT_Z:
      case DATAFLOW_FRAG_SUBMIT_MS:
      case DATAFLOW_FRAG_SUBMIT_ALL:
      case DATAFLOW_FRAG_SUBMIT_R0:
      case DATAFLOW_FRAG_SUBMIT_R1:
      case DATAFLOW_FRAG_SUBMIT_R2:
      case DATAFLOW_FRAG_SUBMIT_R3:
      case DATAFLOW_TMU_SWAP:
      case DATAFLOW_VERTEX_SET:
      case DATAFLOW_VPM_READ_SETUP:
      case DATAFLOW_VPM_WRITE_SETUP:
#endif
         if (cross == (dataflow->line_num != dataflow->u.fragment_set.param->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.fragment_set.param, dataflow);
			}
         if (dataflow->u.fragment_set.discard && cross == (dataflow->line_num != dataflow->u.fragment_set.discard->line_num))
			{
				print_edge(f, EDGE_SOLID, dataflow->u.fragment_set.discard, dataflow);
			}
			break;

		default:
			UNREACHABLE();
			break;
	}

   for (node = dataflow->iodependencies.first; node; node = node->next)
   {
      if (cross == (dataflow->line_num != node->dataflow->line_num))
		{
         // Dependency: dashed line.
			print_edge(f, EDGE_DASHED, node->dataflow, dataflow);
		}
   }
}

static void print_node(FILE* f, Dataflow* dataflow)
{
   // Print node reference and open label string.
   fprintf(f, "\tn%x [label=\"", (size_t)dataflow);

   // Print contents of label string.
   switch (dataflow->flavour)
	{
		case DATAFLOW_CONST_BOOL:
         fprintf(f, dataflow->u.const_bool.value ? "true" : "false");
         break;

		case DATAFLOW_CONST_INT:
         fprintf(f, "%d", dataflow->u.const_int.value);
         break;

		case DATAFLOW_CONST_FLOAT:
			fprintf(f, "%f", *(float*)&dataflow->u.const_float.value);
			break;

      case DATAFLOW_CONST_SAMPLER:
         fprintf(f, "sampler[%d] from <%s>", dataflow->u.const_sampler.location, dataflow->u.const_sampler.name);
         break;

		case DATAFLOW_UNIFORM:
         fprintf(f, "uniform[%d] from <%s>", dataflow->u.linkable_value.row, dataflow->u.linkable_value.name);
         break;
		case DATAFLOW_ATTRIBUTE:
         fprintf(f, "attribute[%d] from <%s>", dataflow->u.linkable_value.row, dataflow->u.linkable_value.name);
         break;
		case DATAFLOW_VARYING:
         fprintf(f, "varying[%d] from <%s>", dataflow->u.linkable_value.row, dataflow->u.linkable_value.name);
         break;

      case DATAFLOW_UNIFORM_OFFSET:
         fprintf(f, "uniform offset");
         break;
      
		case DATAFLOW_ARITH_NEGATE:
         fprintf(f, "-");
         break;
		case DATAFLOW_LOGICAL_NOT:
         fprintf(f, "!");
         break;
      case DATAFLOW_INTRINSIC_FLOATTOINT_TRUNC:
         fprintf(f, "trunc");
         break;
      case DATAFLOW_INTRINSIC_FLOATTOINT_NEAREST:
         fprintf(f, "nearest");
         break;
      case DATAFLOW_INTRINSIC_FLOATTOBOOL:
         fprintf(f, "ftob");
         break;
      case DATAFLOW_INTRINSIC_INTTOFLOAT:
         fprintf(f, "itof");
         break;
      case DATAFLOW_INTRINSIC_INTTOBOOL:
         fprintf(f, "itob");
         break;
      case DATAFLOW_INTRINSIC_BOOLTOINT:
         fprintf(f, "btoi");
         break;
      case DATAFLOW_INTRINSIC_BOOLTOFLOAT:
         fprintf(f, "btof");
         break;
		case DATAFLOW_INTRINSIC_RSQRT:
         fprintf(f, "rsqrt");
         break;
      case DATAFLOW_INTRINSIC_CEIL:
         fprintf(f, "ceil");
         break;
      case DATAFLOW_INTRINSIC_FLOOR:
         fprintf(f, "floor");
         break;
      case DATAFLOW_INTRINSIC_SIGN:
         fprintf(f, "sign");
         break;
		case DATAFLOW_INTRINSIC_RCP:
         fprintf(f, "rcp");
         break;
		case DATAFLOW_INTRINSIC_LOG2:
         fprintf(f, "log2");
         break;
		case DATAFLOW_INTRINSIC_EXP2:
         fprintf(f, "exp2");
         break;

      case DATAFLOW_MOV:
         fprintf(f, "mov");
         break;

      case DATAFLOW_COERCE_TOFLOAT:
         fprintf(f, "tof");
         break;
      case DATAFLOW_COERCE_TOINT:
         fprintf(f, "toi");
         break;

		case DATAFLOW_MUL:
         fprintf(f, "*");
         break;
		case DATAFLOW_ADD:
         fprintf(f, "+");
         break;
		case DATAFLOW_SUB:
		case DATAFLOW_RSUB:
         fprintf(f, "-");
         break;
		case DATAFLOW_LESS_THAN:
         fprintf(f, "<");
         break;
		case DATAFLOW_LESS_THAN_EQUAL:
         fprintf(f, "<=");
         break;
		case DATAFLOW_GREATER_THAN:
         fprintf(f, ">");
         break;
		case DATAFLOW_GREATER_THAN_EQUAL:
         fprintf(f, ">=");
         break;
		case DATAFLOW_EQUAL:
         fprintf(f, "==");
         break;
		case DATAFLOW_NOT_EQUAL:
         fprintf(f, "!=");
         break;
		case DATAFLOW_LOGICAL_AND:
         fprintf(f, "&&");
         break;
		case DATAFLOW_LOGICAL_XOR:
         fprintf(f, "^^");
         break;
		case DATAFLOW_LOGICAL_OR:
         fprintf(f, "||");
         break;
		case DATAFLOW_INTRINSIC_MIN:
         fprintf(f, "min");
         break;
		case DATAFLOW_INTRINSIC_MAX:
         fprintf(f, "max");
         break;

		case DATAFLOW_CONDITIONAL:
         fprintf(f, "cond");
         break;

      case DATAFLOW_THREADSWITCH:
         fprintf(f, "thrsw");
         break;

		case DATAFLOW_TEX_SET_COORD_S:
         fprintf(f, "set coord s");
         break;
		case DATAFLOW_TEX_SET_COORD_T:
         fprintf(f, "set coord t");
         break;
		case DATAFLOW_TEX_SET_COORD_R:
         fprintf(f, "set coord r");
         break;
		case DATAFLOW_TEX_SET_BIAS:
         fprintf(f, "set bias");
         break;
		case DATAFLOW_TEX_SET_LOD:
         fprintf(f, "set lod");
         break;

		case DATAFLOW_TEX_GET_CMP_R:
         fprintf(f, "get cmp r");
         break;
		case DATAFLOW_TEX_GET_CMP_G:
         fprintf(f, "get cmp g");
         break;
		case DATAFLOW_TEX_GET_CMP_B:
         fprintf(f, "get cmp b");
         break;
		case DATAFLOW_TEX_GET_CMP_A:
         fprintf(f, "get cmp a");
         break;

		case DATAFLOW_FRAG_GET_X:
         fprintf(f, "get frag x");
         break;
		case DATAFLOW_FRAG_GET_Y:
         fprintf(f, "get frag y");
         break;
		case DATAFLOW_FRAG_GET_Z:
         fprintf(f, "get frag z");
         break;
		case DATAFLOW_FRAG_GET_W:
         fprintf(f, "get frag w");
         break;
		case DATAFLOW_FRAG_GET_PC_X:
         fprintf(f, "get frag pc x");
         break;
		case DATAFLOW_FRAG_GET_PC_Y:
         fprintf(f, "get frag pc y");
         break;
		case DATAFLOW_FRAG_GET_FF:
         fprintf(f, "get frag ff");
         break;

		case DATAFLOW_FRAG_SET_R:
         fprintf(f, "set frag r");
         break;
		case DATAFLOW_FRAG_SET_G:
         fprintf(f, "set frag g");
         break;
		case DATAFLOW_FRAG_SET_B:
         fprintf(f, "set frag b");
         break;
		case DATAFLOW_FRAG_SET_A:
         fprintf(f, "set frag a");
         break;

#ifdef __VIDEOCORE4__
   case DATAFLOW_BITWISE_NOT:
      fprintf(f, "~");
      break;
   case DATAFLOW_BITWISE_AND:
      fprintf(f, "&");
      break;
   case DATAFLOW_BITWISE_OR:
      fprintf(f, "|");
      break;
   case DATAFLOW_BITWISE_XOR:
      fprintf(f, "^");
      break;
   case DATAFLOW_V8MULD:
      fprintf(f, "v8 *");
      break;
   case DATAFLOW_V8MIN:
      fprintf(f, "v8 min");
      break;
   case DATAFLOW_V8MAX:
      fprintf(f, "v8 max");
      break;
   case DATAFLOW_V8ADDS:
      fprintf(f, "v8 +");
      break;
   case DATAFLOW_V8SUBS:
      fprintf(f, "v8 -");
      break;
   case DATAFLOW_INTEGER_ADD:
      fprintf(f, "int +");
      break;
   case DATAFLOW_VARYING_C:
      fprintf(f, "vary c");
      break;
   case DATAFLOW_FRAG_GET_COL:
      fprintf(f, "get frag col");
      break;
   case DATAFLOW_FRAG_SUBMIT_STENCIL:
      fprintf(f, "set frag stencil");
      break;
   case DATAFLOW_FRAG_SUBMIT_Z:
      fprintf(f, "set frag z");
      break;
   case DATAFLOW_FRAG_SUBMIT_MS:
      fprintf(f, "set frag ms");
      break;
   case DATAFLOW_FRAG_SUBMIT_ALL:
      fprintf(f, "set frag all");
      break;
   case DATAFLOW_FRAG_SUBMIT_R0:
      fprintf(f, "set r0");
      break;
   case DATAFLOW_FRAG_SUBMIT_R1:
      fprintf(f, "set r1");
      break;
   case DATAFLOW_FRAG_SUBMIT_R2:
      fprintf(f, "set r2");
      break;
   case DATAFLOW_FRAG_SUBMIT_R3:
      fprintf(f, "set r3");
      break;
   case DATAFLOW_TMU_SWAP:
      fprintf(f, "set tmu swap");
      break;
#ifndef __BCM2708A0__
   case DATAFLOW_TEX_SET_DIRECT:
      fprintf(f, "set direct lookup");
      break;
#endif
   case DATAFLOW_VERTEX_SET:
      fprintf(f, "set vertex data");
      break;
   case DATAFLOW_VPM_READ_SETUP:
      fprintf(f, "set vpm read setup");
      break;
   case DATAFLOW_VPM_WRITE_SETUP:
      fprintf(f, "set vpm write setup");
      break;
   case DATAFLOW_PACK_COL_R:
      fprintf(f, "pack col r");
      break;
   case DATAFLOW_PACK_COL_G:
      fprintf(f, "pack col g");
      break;
   case DATAFLOW_PACK_COL_B:
      fprintf(f, "pack col b");
      break;
   case DATAFLOW_PACK_COL_A:
      fprintf(f, "pack col a");
      break;
   case DATAFLOW_PACK_16A:
      fprintf(f, "pack 16a");
      break;
   case DATAFLOW_PACK_16B:
      fprintf(f, "pack 16b");
      break;
   case DATAFLOW_PACK_FB_R:
      fprintf(f, "pack col (r)");
      break;
   case DATAFLOW_PACK_FB_B:
      fprintf(f, "pack col (b)");
      break;
   case DATAFLOW_UNPACK_COL_R:
      fprintf(f, "unpack col r");
      break;
   case DATAFLOW_UNPACK_COL_G:
      fprintf(f, "unpack col g");
      break;
   case DATAFLOW_UNPACK_COL_B:
      fprintf(f, "unpack col b");
      break;
   case DATAFLOW_UNPACK_COL_A:
      fprintf(f, "unpack col a");
      break;
   case DATAFLOW_UNPACK_16A:
      fprintf(f, "unpack 16a");
      break;
   case DATAFLOW_UNPACK_16A_F:
      fprintf(f, "unpack 16a float");
      break;
   case DATAFLOW_UNPACK_16B:
      fprintf(f, "unpack 16b");
      break;
   case DATAFLOW_UNPACK_16B_F:
      fprintf(f, "unpack 16b float");
      break;
   case DATAFLOW_UNPACK_8A:
      fprintf(f, "unpack 8a");
      break;
   case DATAFLOW_UNPACK_8B:
      fprintf(f, "unpack 8b");
      break;
   case DATAFLOW_UNPACK_8C:
      fprintf(f, "unpack 8c");
      break;
   case DATAFLOW_UNPACK_8D:
      fprintf(f, "unpack 8d");
      break;
   case DATAFLOW_UNPACK_8R:
      fprintf(f, "unpack 8r");
      break;
   case DATAFLOW_UNPACK_FB_R:
      fprintf(f, "unpack col (r)");
      break;
   case DATAFLOW_UNPACK_FB_B:
      fprintf(f, "unpack col (g)");
      break;
   case DATAFLOW_SHIFT_RIGHT:
      fprintf(f, ">>");
      break;
   case DATAFLOW_LOGICAL_SHR:
      fprintf(f, "logical >>");
      break;
#ifdef __BCM2708A0__
   case DATAFLOW_INDEXED_UNIFORM_SAMPLER:
      fprintf(f, "sampler from indexed uniform");
      break;
#else
   case DATAFLOW_UNIFORM_ADDRESS:
      fprintf(f, "uniform address");
      break;
#endif
   case DATAFLOW_SCOREBOARD_WAIT:
      fprintf(f, "sbwait");
      break;
#endif

		default:
			UNREACHABLE();
			return;
	}

   // Append type.
   {
      const char* type = NULL;

      switch (dataflow->type_index)
      {
         case PRIM_BOOL:
         case PRIM_INT:
         case PRIM_FLOAT:
         case PRIM_VOID:
         case PRIM_SAMPLER2D:
         case PRIM_SAMPLERCUBE:
            type = primitiveTypes[dataflow->type_index].name;
            break;

         default:
            UNREACHABLE();
            return;
      }

      fprintf(f, " : %s", type);
   }

   if (dataflow->slot == -1)
      fprintf(f, " -> ?");
   else
      if (dataflow->slot & DATAFLOW_SLOT_VRF)
         fprintf(f, " -> v%d", dataflow->slot & DATAFLOW_SLOT_MASK);
      else
         fprintf(f, " -> s%d", dataflow->slot);

   fprintf(f, " (%d)\"", dataflow->delay);

   switch (dataflow->phase)
   {
   case BACKEND_PASS_AWAITING_SCHEDULE:
      fprintf(f, " style=\"filled\" fillcolor=\"#ffff80\"");
      break;
   case BACKEND_PASS_SCHEDULED:
      fprintf(f, " style=\"filled\" fillcolor=\"#ff8080\"");
      break;
   case BACKEND_PASS_DIVING:
      fprintf(f, " style=\"filled\" fillcolor=\"#8080ff\"");
      break;
   }

   // Close label string.
   fprintf(f, "];\n");
}

// map must map a line number to a DataflowChain, containing all Dataflow nodes for that line.
// Outputs graphviz dot representation of these nodes,
// clustering by line number.
static void print_dataflow_from_map(FILE* f, Map* map)
{
	DataflowChain* chain;
	DataflowChainNode* node;

   // For all line numbers...
	while ((chain = (DataflowChain *)glsl_map_pop(map)) != NULL)
	{
      int line_num;

      vcos_assert(chain->first);
      line_num = chain->first->dataflow->line_num;

      // Open cluster if we have a valid line number to group by.
      if (LINE_NUMBER_UNDEFINED != line_num)
      {
//         fprintf(f, "\tsubgraph cluster_" GRAPHVIZ_CLUSTER_PREFIX "_%d\n", line_num);
         fprintf(f, "\t{\n");
         fprintf(f, "\tlabel=\"line %d\";\n", line_num);
		   fprintf(f, "\tstyle=filled;\n");
		   fprintf(f, "\tfillcolor=\"0 0 .9\";\n");
      }
      
      // For all nodes...
		for (node = chain->first; node; node = node->next)
		{
         // Declare node.
         print_node(f, node->dataflow);

         // Add all the edges that don't cross a cluster boundary.
         print_edges(f, node->dataflow, false);
		}

      // Close cluster.
      if (LINE_NUMBER_UNDEFINED != line_num)
      {
         fprintf(f, "\t}\n");
      }

      // For all nodes...
		for (node = chain->first; node; node = node->next)
		{
         // Add remaining edges.
         print_edges(f, node->dataflow, true);
		}
	}
}

void glsl_print_dataflow_from_root(FILE* f, Dataflow* root, int pass)
{
   Map* map;
   
   // Initialize map with key comparator.
   map = glsl_map_new();
 
   // Gather nodes into map.
   glsl_dataflow_accept_towards_leaves(root, map, NULL, dpostv_gather_by_line_num, pass);

	// Print opening.
	fprintf(f, "digraph " GRAPHVIZ_GRAPH_NAME "\n");
   fprintf(f, "{\n");

   // Print map.
   print_dataflow_from_map(f, map);

	// Print closing.
	fprintf(f, "}\n");
}

void glsl_print_dataflow_from_roots(FILE* f, DataflowChain* roots, DataflowChain *order, int pass)
{
   Map* map;
   DataflowChainNode *node;
   
   // Initialize map with key comparator.
   map = glsl_map_new();

   // Gather nodes into map.
   for (node = roots->first; node; node = node->next)
   {
      Dataflow* root = node->dataflow;
      glsl_dataflow_accept_towards_leaves(root, map, NULL, dpostv_gather_by_line_num, pass);
   }

	// Print opening.
	fprintf(f, "digraph " GRAPHVIZ_GRAPH_NAME "\n");
   fprintf(f, "{\n");

   // Print map.
   print_dataflow_from_map(f, map);

   // Print ordering edges (typically from scheduler)
   if (order)
      for (node = order->first; node; node = node->next) 
         if (node->next) 
            print_edge(f, EDGE_SEQUENCE, node->dataflow, node->next->dataflow);

	// Print closing.
	fprintf(f, "}\n");
}
#else
/*
   keep Metaware happy by providing an exported symbol
*/

void glsl_print_dataflow_from_roots(FILE* f, DataflowChain* roots, DataflowChain *order, int pass)
{
   UNUSED(f);
   UNUSED(roots);
   UNUSED(order);
   UNUSED(pass);
}
#endif // _DEBUG
