/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
//#define OUTPUT_GRAPHVIZ
//#define OUTPUT_GRAPHVIZ_ON_FAILURE
#include "middleware/khronos/glsl/glsl_common.h"

#include "middleware/khronos/glsl/glsl_dataflow.h"
#include "middleware/khronos/glsl/glsl_dataflow_visitor.h"
#include "middleware/khronos/glsl/glsl_backend.h"
#include "middleware/khronos/glsl/glsl_stack.h"
#include "middleware/khronos/common/2708/khrn_prod_4.h"
#include "middleware/khronos/glsl/2708/glsl_allocator_4.h"
#include "middleware/khronos/gl20/gl20_config.h"
#include "middleware/khronos/gl20/2708/gl20_shader_4.h"

#if defined(OUTPUT_GRAPHVIZ) || defined(OUTPUT_GRAPHVIZ_ON_FAILURE)
#include "middleware/khronos/glsl/glsl_dataflow_print.h"
static int graphviz_file_num = 0;
static void graphviz(Dataflow *root)
{
   char filename[20];
   FILE *f;
   sprintf(filename, "/mfs/sd/graph%d.txt", graphviz_file_num++);
   f = fopen(filename, "w");
   vcos_assert(f);
   glsl_print_dataflow_from_root(f, root, BACKEND_ACTUAL_PASS_PRINT);
   fclose(f);
}
#endif

static void get_dependencies(DataflowChain *chain, Dataflow *dataflow);
static void dpostv_calculate_bushiness(Dataflow* dataflow, void* data);
static void visit_recursive(Dataflow *dataflow, bool schedule_if_input);

static uint32_t num_attribute_rows;

static struct
{
   uint32_t tmu_dependencies[TMU_DEP_WORD_COUNT];
   uint32_t write_count;
   Dataflow *write[4];
   Dataflow *read;
   uint32_t stuff_in_input_fifo;
   uint32_t stuff_in_output_fifo;
   uint32_t write_read_dependency;
   uint32_t thrsw_count;
   uint32_t tmu;
} tmu_lookups[MAX_TMU_LOOKUPS];
static uint32_t num_tmu_lookups;
static Dataflow *sbwait_node;
static Dataflow *xxx_varying_nodes[32];
static uint32_t xxx_num_varying_nodes;
static void merge_tmu_dependencies(uint32_t *dep_a, uint32_t *dep_b);
static void flag_tmu_dependency(uint32_t *dep, int dep_num);
static void clear_tmu_dependencies(uint32_t *deps);
static void copy_tmu_dependencies(uint32_t *dep_out, uint32_t *dep_in);

uint32_t glsl_backend_get_schedule_type(Dataflow *dataflow)
{
   switch (dataflow->flavour)
   {
      case DATAFLOW_CONST_BOOL:
      case DATAFLOW_CONST_INT:
      case DATAFLOW_CONST_FLOAT:
      case DATAFLOW_UNIFORM:
      case DATAFLOW_ATTRIBUTE:
      case DATAFLOW_VARYING:
      case DATAFLOW_FRAG_GET_X:
      case DATAFLOW_FRAG_GET_Y:
      case DATAFLOW_FRAG_GET_Z:
      case DATAFLOW_FRAG_GET_W:
      case DATAFLOW_FRAG_GET_FF:
      case DATAFLOW_VARYING_C:
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
#ifndef __BCM2708A0__
      case DATAFLOW_UNIFORM_ADDRESS:
#endif
         return SCHEDULE_TYPE_INPUT;

      case DATAFLOW_TEX_SET_COORD_S:
      case DATAFLOW_TEX_SET_COORD_T:
      case DATAFLOW_TEX_SET_COORD_R:
      case DATAFLOW_TEX_SET_BIAS:
      case DATAFLOW_TEX_SET_LOD:
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
         return SCHEDULE_TYPE_OUTPUT;

      case DATAFLOW_CONDITIONAL:
      case DATAFLOW_INTRINSIC_RSQRT:
      case DATAFLOW_INTRINSIC_RCP:
      case DATAFLOW_INTRINSIC_LOG2:
      case DATAFLOW_INTRINSIC_EXP2:
      case DATAFLOW_PACK_COL_R:
      case DATAFLOW_PACK_COL_G:
      case DATAFLOW_PACK_COL_B:
      case DATAFLOW_PACK_COL_A:
      case DATAFLOW_PACK_16A:
      case DATAFLOW_PACK_16B:
      case DATAFLOW_PACK_FB_R:
      case DATAFLOW_PACK_FB_B:
         return SCHEDULE_TYPE_INPUT | SCHEDULE_TYPE_OUTPUT;

      case DATAFLOW_TEX_GET_CMP_R:    // actually sig
      case DATAFLOW_FRAG_GET_COL:
         return SCHEDULE_TYPE_ALU | SCHEDULE_TYPE_SIG;

      case DATAFLOW_SCOREBOARD_WAIT:
      case DATAFLOW_THREADSWITCH:
         return SCHEDULE_TYPE_SIG;

      case DATAFLOW_CONST_SAMPLER:
         return 0;

      default:
         return SCHEDULE_TYPE_ALU;
   }
}

static void get_dependencies(DataflowChain *chain, Dataflow *dataflow)
{
   glsl_dataflow_chain_init(chain);
   switch (dataflow->flavour)
   {
      case DATAFLOW_CONST_BOOL:
      case DATAFLOW_CONST_INT:
      case DATAFLOW_CONST_FLOAT:
      case DATAFLOW_CONST_SAMPLER:
      case DATAFLOW_UNIFORM:
      case DATAFLOW_ATTRIBUTE:
      case DATAFLOW_VARYING:
      case DATAFLOW_FRAG_GET_X:
      case DATAFLOW_FRAG_GET_Y:
      case DATAFLOW_FRAG_GET_Z:
      case DATAFLOW_FRAG_GET_W:
      case DATAFLOW_FRAG_GET_FF:
      case DATAFLOW_VARYING_C:
      case DATAFLOW_FRAG_GET_COL:
#ifdef __BCM2708A0__
      case DATAFLOW_INDEXED_UNIFORM_SAMPLER:
#else
      case DATAFLOW_UNIFORM_ADDRESS:
#endif
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
      case DATAFLOW_BITWISE_NOT:
         glsl_dataflow_chain_append(chain, dataflow->u.unary_op.operand);
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
      case DATAFLOW_LOGICAL_SHR:
      case DATAFLOW_SHIFT_RIGHT:
      case DATAFLOW_BITWISE_AND:
      case DATAFLOW_BITWISE_OR:
      case DATAFLOW_BITWISE_XOR:
      case DATAFLOW_V8MULD:
      case DATAFLOW_V8MIN:
      case DATAFLOW_V8MAX:
      case DATAFLOW_V8ADDS:
      case DATAFLOW_V8SUBS:
      case DATAFLOW_INTEGER_ADD:
         glsl_dataflow_chain_append(chain, dataflow->u.binary_op.left);
         glsl_dataflow_chain_append(chain, dataflow->u.binary_op.right);
         break;

      case DATAFLOW_CONDITIONAL:
         glsl_dataflow_chain_append(chain, dataflow->u.cond_op.cond);
         glsl_dataflow_chain_append(chain, dataflow->u.cond_op.true_value);
         glsl_dataflow_chain_append(chain, dataflow->u.cond_op.false_value);
         break;

      case DATAFLOW_TEX_SET_COORD_S:
      case DATAFLOW_TEX_SET_COORD_T:
      case DATAFLOW_TEX_SET_COORD_R:
      case DATAFLOW_TEX_SET_BIAS:
      case DATAFLOW_TEX_SET_LOD:
#ifndef __BCM2708A0__
      case DATAFLOW_TEX_SET_DIRECT:
#endif
         /* Don't visit sampler node - it isn't interesting to us */;
         glsl_dataflow_chain_append(chain, dataflow->u.texture_lookup_set.param);
         break;
      case DATAFLOW_TEX_GET_CMP_R:
      /*case DATAFLOW_TEX_GET_CMP_G:      -- These ones aren't used in 2708. (GET_CMP_R gets everything)
      case DATAFLOW_TEX_GET_CMP_B:
      case DATAFLOW_TEX_GET_CMP_A:*/
         //glsl_dataflow_chain_append(chain, dataflow->u.texture_lookup_get.sampler);
         break;

      case DATAFLOW_FRAG_SET_R:
      case DATAFLOW_FRAG_SET_G:
      case DATAFLOW_FRAG_SET_B:
      case DATAFLOW_FRAG_SET_A:
         glsl_dataflow_chain_append(chain, dataflow->u.fragment_set.param);
         break;

      case DATAFLOW_FRAG_SUBMIT_STENCIL:
      case DATAFLOW_FRAG_SUBMIT_Z:
      case DATAFLOW_FRAG_SUBMIT_MS:
      case DATAFLOW_FRAG_SUBMIT_ALL:
      case DATAFLOW_FRAG_SUBMIT_R0:
      case DATAFLOW_FRAG_SUBMIT_R1:
      case DATAFLOW_FRAG_SUBMIT_R2:
      case DATAFLOW_FRAG_SUBMIT_R3:
      case DATAFLOW_TMU_SWAP:
         glsl_dataflow_chain_append(chain, dataflow->u.fragment_set.param);
         if (dataflow->u.fragment_set.discard) glsl_dataflow_chain_append(chain, dataflow->u.fragment_set.discard);
         break;

      case DATAFLOW_VERTEX_SET:
      case DATAFLOW_VPM_READ_SETUP:
      case DATAFLOW_VPM_WRITE_SETUP:
         glsl_dataflow_chain_append(chain, dataflow->u.vertex_set.param);
         break;

      case DATAFLOW_PACK_COL_R:
      case DATAFLOW_PACK_COL_G:
      case DATAFLOW_PACK_COL_B:
      case DATAFLOW_PACK_COL_A:
      case DATAFLOW_PACK_16A:
      case DATAFLOW_PACK_16B:
      case DATAFLOW_PACK_FB_R:
      case DATAFLOW_PACK_FB_B:
         glsl_dataflow_chain_append(chain, dataflow->u.pack.operand);
         if (dataflow->u.pack.background != NULL)
            glsl_dataflow_chain_append(chain, dataflow->u.pack.background);
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
         glsl_dataflow_chain_append(chain, dataflow->u.unary_op.operand);
         break;

      case DATAFLOW_SCOREBOARD_WAIT:
      case DATAFLOW_THREADSWITCH:
         break;

      default:
         //TODO: the rest
         UNREACHABLE();
   }
}

static void dpostv_calculate_bushiness(Dataflow* dataflow, void* data)
{
   DataflowChainNode *node;
   DataflowChain deps;
   int value = 0;
   int minb, maxb;
   uint32_t schedule_type;
   uint32_t count;
   uint32_t tmu_dependencies[TMU_DEP_WORD_COUNT];

   UNUSED(data);
   dataflow->phase = BACKEND_PASS_AWAITING_SCHEDULE;

   minb = 0x7fffffff;
   maxb = -0x7fffffff;
   count = 0;

   schedule_type = glsl_backend_get_schedule_type(dataflow);

   clear_tmu_dependencies(tmu_dependencies);
   get_dependencies(&deps, dataflow);
   for (node = deps.first; node; node = node->next)
   {
      value = node->dataflow->delay;
      if (value < minb) minb = value;
      if (value > maxb) maxb = value;
      count++;
      merge_tmu_dependencies(tmu_dependencies, node->dataflow->tmu_dependencies);
   }

   for (node = dataflow->iodependencies.first; node; node = node->next)
      merge_tmu_dependencies(tmu_dependencies, node->dataflow->tmu_dependencies);

   if (dataflow->flavour == DATAFLOW_TEX_GET_CMP_R)
   {
      Dataflow *write_s;
      uint32_t write_count;

      vcos_assert(num_tmu_lookups < MAX_TMU_LOOKUPS);

      copy_tmu_dependencies(tmu_lookups[num_tmu_lookups].tmu_dependencies, tmu_dependencies);
      write_s = NULL;
      for (node = dataflow->iodependencies.first; node; node = node->next)
      {
#ifdef __BCM2708A0__
         if (node->dataflow->flavour == DATAFLOW_TEX_SET_COORD_S)
#else
         if (node->dataflow->flavour == DATAFLOW_TEX_SET_COORD_S || node->dataflow->flavour == DATAFLOW_TEX_SET_DIRECT)
#endif
         {
            write_s = node->dataflow;
            break;
         }
      }
      vcos_assert(write_s != NULL);
      tmu_lookups[num_tmu_lookups].read = dataflow;

      tmu_lookups[num_tmu_lookups].write[0] = write_s;
      write_count = 1;
      for (node = write_s->iodependencies.first; node; node = node->next)
      {
         if (node->dataflow->flavour == DATAFLOW_TEX_SET_COORD_T || node->dataflow->flavour == DATAFLOW_TEX_SET_COORD_R || node->dataflow->flavour == DATAFLOW_TEX_SET_BIAS)
         {
            vcos_assert(write_count < 4);
            tmu_lookups[num_tmu_lookups].write[write_count] = node->dataflow;
            write_count++;
         }
      }
      tmu_lookups[num_tmu_lookups].write_count = write_count;

      flag_tmu_dependency(tmu_dependencies, num_tmu_lookups);
      num_tmu_lookups++;
   }
   copy_tmu_dependencies(dataflow->tmu_dependencies, tmu_dependencies);

   if (dataflow->flavour == DATAFLOW_SCOREBOARD_WAIT)
   {
      vcos_assert(sbwait_node == NULL);
      sbwait_node = dataflow;
   }

   if (dataflow->flavour == DATAFLOW_ADD && dataflow->u.binary_op.right->flavour == DATAFLOW_VARYING_C)
   {
      vcos_assert(xxx_num_varying_nodes < 32);
      xxx_varying_nodes[xxx_num_varying_nodes] = dataflow;
      xxx_num_varying_nodes++;
   }

   if (minb > maxb)
      value = 0;          /* Input list was empty */
   else if (minb == maxb && count > 1)
      value = maxb + 1;   /* All inputs were the same */
   else
      value = maxb;

   dataflow->slot = ~0;

   if (dataflow->flavour == DATAFLOW_ATTRIBUTE)
   {
      num_attribute_rows = _max(num_attribute_rows, dataflow->u.linkable_value.row + 1);
   }

   dataflow->delay = value;
}

static void visit_recursive(Dataflow *dataflow, bool schedule_if_input)
{
   STACK_CHECK();

   if (dataflow->phase != BACKEND_PASS_SCHEDULED && (schedule_if_input || dataflow->phase != BACKEND_PASS_SEEN_INPUT) && !glsl_allocator_failed())
   {
      DataflowPriorityQueue queue;
      DataflowChain deps;
      DataflowChainNode *node;
      Dataflow *parent;
      uint32_t schedule_type;

      if (dataflow->phase == BACKEND_PASS_AWAITING_SCHEDULE)
         dataflow->phase = BACKEND_PASS_DIVING;

      schedule_type = glsl_backend_get_schedule_type(dataflow);

      if ((schedule_type & SCHEDULE_TYPE_ALU))
      {
         Dataflow *suggestion = glsl_allocator_get_next_scheduler_node();
         if (suggestion)
         {
            /*
            Interrupt normal scheduling sequence with something the allocator wants us to schedule (i.e.
            a dependent of one of the things filling up its regfile)
            */
            visit_recursive(suggestion, true);
            glsl_allocator_finish_scheduler_node(suggestion);
         }
      }

      glsl_dataflow_priority_queue_init(&queue, 256);
      for (node = dataflow->iodependencies.first; node; node = node->next)
      {
         glsl_dataflow_priority_queue_push(&queue, node->dataflow);
      }

      /*
         Add other dependencies.

         For DATAFLOW_CONDITIONAL nodes, the dependencies must be visited in the
         right order. TODO: how can we handle this more cleanly? It should be
         possible with iodependencies, but this means we "lose track" of the
         output node.
      */
      if (dataflow->flavour == DATAFLOW_CONDITIONAL)
      {
         vcos_assert(dataflow->u.cond_op.true_value != dataflow->u.cond_op.false_value);

         if (dataflow->u.cond_op.true_value == dataflow->u.cond_op.cond)
            dataflow->u.cond_op.false_value->delay = -1;  /* Make sure this one gets visited last */
         else
            dataflow->u.cond_op.true_value->delay = -1;   /* Make sure this one gets visited last */
      }

      get_dependencies(&deps, dataflow);
      for (node = deps.first; node; node = node->next)
      {
         glsl_dataflow_priority_queue_push(&queue, node->dataflow);
      }

      while ((parent = glsl_dataflow_priority_queue_pop(&queue)) != NULL)
      {
         visit_recursive(parent, !glsl_dataflow_chain_contains(&deps, parent));
      }

      if (glsl_allocator_failed()) return;

      vcos_assert(dataflow->slot == ~0);
      if (schedule_type & SCHEDULE_TYPE_ALU || schedule_type & SCHEDULE_TYPE_OUTPUT || schedule_type & SCHEDULE_TYPE_SIG)
      {
         dataflow->phase = BACKEND_PASS_SCHEDULED;
         glsl_allocator_add_node(dataflow);
      }
      else if (schedule_if_input)
      {
         dataflow->phase = BACKEND_PASS_SCHEDULED;
         verify(glsl_allocator_add_input_dependency(dataflow));
      }
      else
      {
         /*
            Don't mark input as scheduled if it hasn't actually been scheduled yet.
            Otherwise we can get problems if one inputs depends on the other (e.g. adding two attributes).
         */
         dataflow->phase = BACKEND_PASS_SEEN_INPUT;
      }

      /* Mark all inputs as scheduled, to stop them getting scheduled again */
      for (node = deps.first; node; node = node->next)
      {
         if (node->dataflow->phase == BACKEND_PASS_SEEN_INPUT)
            node->dataflow->phase = BACKEND_PASS_SCHEDULED;
      }
   }
}

static void iodep(Dataflow *consumer, Dataflow *supplier)
{
   if (consumer != NULL && supplier != NULL)
   {
      vcos_assert(consumer != supplier);
      glsl_dataflow_add_iodependent(supplier, consumer);
      glsl_dataflow_add_iodependency(consumer, supplier);
   }
}

static void merge_tmu_dependencies(uint32_t *dep_a, uint32_t *dep_b) {
   int i;
   for (i=0; i<TMU_DEP_WORD_COUNT; i++) dep_a[i] |= dep_b[i];
}

static void copy_tmu_dependencies(uint32_t *dep_out, uint32_t *dep_in) {
   int i;
   for (i=0; i<TMU_DEP_WORD_COUNT; i++) dep_out[i] = dep_in[i];
}

static void flag_tmu_dependency(uint32_t *dep, int dep_num) {
   vcos_assert((dep_num / 32) < TMU_DEP_WORD_COUNT);
   dep[dep_num/32] |= (1 << (dep_num & 31));
}

static bool tmu_dependency_flagged(uint32_t *dep, int dep_num) {
   vcos_assert(dep_num/32 < TMU_DEP_WORD_COUNT);
   return ( (dep[dep_num/32] & (1<<(dep_num&31))) != 0 );
}

static bool tmu_all_n_flagged(uint32_t *dep, int num_deps) {
   while (num_deps >= 32) {
      if (dep[0] != ~0) return 0;
      num_deps -= 32;
      dep++;
   }
   return dep[0] == ( ((1<<num_deps)-1) | (1<<num_deps));
}

static bool all_tmu_dependencies_flagged(uint32_t *flagged_in, uint32_t *deps) {
   int i;
   for (i=0; i<TMU_DEP_WORD_COUNT; i++) if (deps[i] & ~flagged_in[i]) return 0;
   return 1;
}

static bool some_tmu_dependencies_flagged(uint32_t *flagged_in, uint32_t *deps) {
   int i;
   for (i=0; i<TMU_DEP_WORD_COUNT; i++) if (flagged_in[i] & deps[i]) return 1;
   return 0;
}

static void clear_tmu_dependencies(uint32_t *deps) {
   int i;
   for (i=0; i<TMU_DEP_WORD_COUNT; i++) deps[i] = 0;
}

/*
   Add iodependencies between nodes to ensure they are scheduled without overflowing
   input or output fifo. Add thread switches where appropriate.

   TODO: create a B0 version (this has HW-1632 workaround and only one TMU)
*/
static bool fix_texture_dependencies(bool thrsw)
{
   uint32_t done[TMU_DEP_WORD_COUNT] = {0,};
   uint32_t input_fifo_size = thrsw ? 4 : 8;
   uint32_t output_fifo_size = thrsw ? 2 : 4;
   uint32_t stuff_in_input_fifo[2] = {0,0}, stuff_in_output_fifo[2] = {0,0};
   uint32_t block_stuff_input[2];
   uint32_t block_stuff_output[2];
   uint32_t block[TMU_DEP_WORD_COUNT] = {0,};
   uint32_t thrsw_count = 0;
   uint32_t tmu;
   Dataflow *switch_nodes[MAX_TMU_LOOKUPS];
#ifdef KHRN_HW_SINGLE_TEXTURE_UNIT
   uint32_t num_tmus = 1;
#else
   uint32_t num_tmus = 2;
   Dataflow *tmu_swap = NULL;
#endif

#ifdef __BCM2708A0__
   if (thrsw) return false;
#endif

   block_stuff_input[0] = input_fifo_size;
   block_stuff_input[1] = input_fifo_size;
   block_stuff_output[0] = output_fifo_size;
   block_stuff_output[1] = output_fifo_size;

   /* Add sentinel */
   vcos_assert(num_tmu_lookups < MAX_TMU_LOOKUPS);
   tmu_lookups[num_tmu_lookups].read = NULL;
   tmu_lookups[num_tmu_lookups].write[0] = NULL;
   tmu_lookups[num_tmu_lookups].stuff_in_input_fifo = 0;
   tmu_lookups[num_tmu_lookups].stuff_in_output_fifo = 0;
   flag_tmu_dependency(done, num_tmu_lookups);

   /*
      1. Write depends on the previous write
      2. If writing would overflow the input or output fifo then write depends on the last read which clears the fifo down to size
      3. Read depends on the previous read
      4. thrsw depends on the last write in the thread switch block
      5. thrsw depends on the last read in the thread switch block
      6. If read is the first in a thread switch block then it depends on the thrsw

      We must start a new thread switch block if we have filled the fifos or we have a dependency on
      the result of one of the other lookups in the block.
   */

   tmu = 0;
   while (!tmu_all_n_flagged(done, num_tmu_lookups))
   {
      uint32_t best, best_j, i, j, write_count;
      Dataflow *sampler_i;

      /*
         Look for something which has not been scheduled, but all of its dependencies have been scheduled.
         TODO: find the "best" one, not just an arbitrary one.
      */
      for (i = 0; i < num_tmu_lookups; i++)
      {
         if (!tmu_dependency_flagged(done, i) && all_tmu_dependencies_flagged(done, tmu_lookups[i].tmu_dependencies))
            break;
      }
      vcos_assert(i < num_tmu_lookups);

      /*
         See if something else is using the same texture sampler. If so, force this to the same
         texture unit.
      */
      sampler_i = tmu_lookups[i].read->u.texture_lookup_get.sampler;
      for (j = 0; j < num_tmu_lookups; j++)
      {
         if (tmu_dependency_flagged(done, j))
         {
            Dataflow *sampler_j = tmu_lookups[j].read->u.texture_lookup_get.sampler;
            if (sampler_j == sampler_i)
            {
               tmu = tmu_lookups[j].tmu;
               break;
            }
         }
      }

      flag_tmu_dependency(done, i);

      tmu_lookups[i].tmu = tmu;

#ifndef KHRN_HW_SINGLE_TEXTURE_UNIT
      if (tmu == 1)
      {
         if (tmu_swap == NULL)
         {
            tmu_swap = glsl_dataflow_construct_fragment_submit(-1, DATAFLOW_TMU_SWAP,
               glsl_dataflow_construct_const_int(-1, 1), NULL, NULL);
         }
      }
#endif

      /*
         Calculate how much stuff has been put into the FIFOs after this texture lookup
      */
      write_count = tmu_lookups[i].write_count;
      stuff_in_input_fifo[tmu] += write_count;
      stuff_in_output_fifo[tmu]++;
      tmu_lookups[i].stuff_in_input_fifo = stuff_in_input_fifo[tmu];
      tmu_lookups[i].stuff_in_output_fifo = stuff_in_output_fifo[tmu];
      
      /*
         Find the earliest texture lookup which, when read back, leaves enough room in
         the FIFOs.
      */
      if (input_fifo_size >= stuff_in_input_fifo[tmu] &&
         output_fifo_size >= stuff_in_output_fifo[tmu]) {
         tmu_lookups[i].write_read_dependency = num_tmu_lookups;
      } else {
         best = ~0;
         best_j = ~0;
         for (j = 0; j <= num_tmu_lookups; j++)
         {
            if (
               tmu_dependency_flagged(done, j) &&
               tmu_lookups[j].tmu == tmu &&
               tmu_lookups[j].stuff_in_input_fifo + input_fifo_size >= stuff_in_input_fifo[tmu] &&
               tmu_lookups[j].stuff_in_output_fifo + output_fifo_size >= stuff_in_output_fifo[tmu] &&
               tmu_lookups[j].stuff_in_output_fifo < best)
            {
               best = tmu_lookups[j].stuff_in_output_fifo;
               best_j = j;
            }
         }
         vcos_assert(best_j != (uint32_t)~0);
         tmu_lookups[i].write_read_dependency = best_j;
      }

      if (thrsw)
      {
         if (
            block_stuff_input[tmu] + write_count > input_fifo_size ||
            block_stuff_output[tmu] + 1 > output_fifo_size ||
            some_tmu_dependencies_flagged(tmu_lookups[i].tmu_dependencies, block))
         {
            block_stuff_input[0] = block_stuff_input[1] = 0;
            block_stuff_output[0] = block_stuff_output[1] = 0;
            clear_tmu_dependencies(block);
            thrsw_count++;
         }

         tmu_lookups[i].thrsw_count = thrsw_count;

         block_stuff_input[tmu] += write_count;
         block_stuff_output[tmu]++;
         flag_tmu_dependency(block, i);;
      }

#ifndef KHRN_HW_SINGLE_TEXTURE_UNIT
      tmu ^= 1;
#endif
   }
   vcos_assert(stuff_in_output_fifo[0] + stuff_in_output_fifo[1] == num_tmu_lookups);

#ifdef WORKAROUND_HW1632
   /* HW-1632 workaround */
   if (thrsw && thrsw_count != 1)
      return false;
#else
   if (thrsw && thrsw_count == 0)
      return false;
#endif

   /* Create thrsw nodes */
   {
      uint32_t i;
      switch_nodes[0] = NULL;

      for (i = 1; i <= thrsw_count; i++)
      {
         switch_nodes[i] = glsl_dataflow_construct_threadswitch(-1, NULL);
         iodep(switch_nodes[i], switch_nodes[i-1]);
      }

#ifdef WORKAROUND_HW2487
      if (thrsw_count)
      {
         /* XXX thread switch depends on varyings? */
         for (i = 0; i < xxx_num_varying_nodes; i++)
            iodep(switch_nodes[1], xxx_varying_nodes[i]);
      }
#endif
   }

   /* Add dependencies now we know that we'll succeed */
   for (tmu = 0; tmu < num_tmus; tmu++)
   {
      uint32_t last = num_tmu_lookups;  /* sentinel */
      uint32_t i,j,k;
      uint32_t tblock;

      for (i = 1; i <= stuff_in_output_fifo[tmu]; i++)
      {
         /* Use stuff_in_output_fifo to index the elements into the correct sequence */
         for (j = 0; j < num_tmu_lookups; j++)
         {
            if (tmu_lookups[j].tmu == tmu && tmu_lookups[j].stuff_in_output_fifo == i)
               break;
         }
         vcos_assert(j != num_tmu_lookups);

         /* Let allocator know which TMU this is assigned to */
         if (tmu == 1)
         {
            Dataflow *sampler = tmu_lookups[j].read->u.texture_lookup_get.sampler;
            if (sampler->flavour == DATAFLOW_CONST_SAMPLER)
               sampler->u.const_sampler.location |= 0x80000000;
            else
            {
#ifdef __BCM2708A0__
               vcos_assert(sampler->flavour == DATAFLOW_INDEXED_UNIFORM_SAMPLER);
#else
               UNREACHABLE();
#endif
               sampler->u.indexed_uniform_sampler.size |= 0x80000000;
            }
            for (k = 0; k < tmu_lookups[j].write_count; k++)
            {
               vcos_assert(tmu_lookups[j].write[k]->u.texture_lookup_set.sampler == sampler);
            }
         }

         /* 1. Write depends on the previous write */
         /* 2. If writing would overflow the input or output fifo then write depends on the last read which clears the fifo down to size */
         for (k = 0; k < tmu_lookups[j].write_count; k++)
         {
            iodep(tmu_lookups[j].write[k], tmu_lookups[last].write[0]);
            iodep(tmu_lookups[j].write[k], tmu_lookups[tmu_lookups[j].write_read_dependency].read);

#ifndef KHRN_HW_SINGLE_TEXTURE_UNIT
            if (tmu_lookups[last].read == NULL)
               iodep(tmu_lookups[j].write[k], tmu_swap);
#endif
         }

         /* 3. Read depends on the previous read */
         iodep(tmu_lookups[j].read, tmu_lookups[last].read);

         /*
            4. thrsw depends on the last write in the thread switch block
         */
         tblock = tmu_lookups[j].thrsw_count;
         if (tblock <= thrsw_count)
            iodep(switch_nodes[tblock], tmu_lookups[j].write[0]);

         /*
            5. thrsw depends on the last read in the thread switch block
         */
         if (tblock + 1 <= thrsw_count)
            iodep(switch_nodes[tblock+1], tmu_lookups[j].read);

         /*
            6. If read is the first in a thread switch block then it depends on the thrsw
         */
         if (tblock <= thrsw_count)
            iodep(tmu_lookups[j].read, switch_nodes[tblock]);

         last = j;
      }

      /*
         Force sbwait after last texture lookup
      */
      iodep(sbwait_node, tmu_lookups[last].read);
   }

   return true;
}

bool glsl_backend_schedule(Dataflow *root, uint32_t type, bool *allow_thread, bool fb_rb_swap)
{
   bool isvtx;
   bool result;
   num_attribute_rows = 0;
   num_tmu_lookups = 0;
   sbwait_node = NULL;

   xxx_num_varying_nodes = 0;
   
   isvtx = (type & GLSL_BACKEND_TYPE_VERTEX_OR_COORD || type & GLSL_BACKEND_TYPE_OFFLINE_VERTEX);

   /*
      Compute the "bushiness" at each node, starting from the leaves.
      - For a leaf node, the bushiness is 0.
      - For a branch node where all inputs have the same bushiness, we add 1 to this value
      - Otherwise we take the maximum of the inputs.

      The idea is that this is an estimate of the register space required to calculate that
      node's value. It is not entirely accurate because:
      - The dual ALU, the stall between regfile write/read and other issues mean that in
        reality we have more than a simple register stack
      - The register stack model is only appropriate if the graph is a tree, which it isn't
        necessarily. Specifically, we assume we have "finished" with a value the first time
        it is read, when in fact it might be stored away for later, taking up an extra register.

      This estimate is used to decide which branch to schedule first (the bushier one).
   */

   glsl_dataflow_accept_towards_leaves_postfix(root, NULL, dpostv_calculate_bushiness, BACKEND_ACTUAL_PASS_SCHEDULE);

   /*
      Sort out texture iodependencies
   */
   if (isvtx)
   {
      if (!fix_texture_dependencies(false))
         return false;
   }
   else
   {
      if (!fix_texture_dependencies(*allow_thread))
      {
         *allow_thread = false;
         if (!fix_texture_dependencies(*allow_thread))
            return false;
      }
   }

   /*
      Add nodes to the schedule using the following algorithm:
      - Take a list of nodes
      - Process it in order of bushiness (greatest first)
      - Schedule each of these nodes' input lists, then add the node itself to the schedule
      - Stop when we reach a leaf node (input list is empty) or a node we have already marked as scheduled
   */
   glsl_allocator_init(type, !isvtx && *allow_thread, num_attribute_rows, fb_rb_swap);
#ifdef OUTPUT_GRAPHVIZ
   graphviz(root);
#endif

   visit_recursive(root, true);
   result = glsl_allocator_finish();
#ifdef OUTPUT_GRAPHVIZ_ON_FAILURE
   if (!result) graphviz(root);
#endif
   return result;
}

static uint32_t cattribs_live;
static void dpostv_find_live_attribs(Dataflow* dataflow, void* data)
{
   UNUSED(data);

   if (dataflow->flavour == DATAFLOW_ATTRIBUTE)
   {
      cattribs_live |= 1<<dataflow->u.linkable_value.row;
   }
}

bool glsl_backend_create_shaders(
   slang_program *program,
   Dataflow *vertex_x,
   Dataflow *vertex_y,
   Dataflow *vertex_z,
   Dataflow *vertex_w,
   Dataflow *vertex_point_size,
   Dataflow **vertex_vary,
   uint32_t vary_count,
   Dataflow *frag_r,
   Dataflow *frag_g,
   Dataflow *frag_b,
   Dataflow *frag_a,
   Dataflow *frag_discard)
{
   GL20_LINK_RESULT_T *result;
   uint32_t i;
   Dataflow *nodes[GL20_LINK_RESULT_NODE_COUNT];
   MEM_HANDLE_T hblob;

   result = program->result;

   /* Calculate which attributes are live for coordinate shaders */
   cattribs_live = 0;
   glsl_dataflow_accept_towards_leaves_postfix(vertex_x, NULL, dpostv_find_live_attribs, BACKEND_PASS_FIND_ATRIBS);
   glsl_dataflow_accept_towards_leaves_postfix(vertex_y, NULL, dpostv_find_live_attribs, BACKEND_PASS_FIND_ATRIBS);
   glsl_dataflow_accept_towards_leaves_postfix(vertex_z, NULL, dpostv_find_live_attribs, BACKEND_PASS_FIND_ATRIBS);
   glsl_dataflow_accept_towards_leaves_postfix(vertex_w, NULL, dpostv_find_live_attribs, BACKEND_PASS_FIND_ATRIBS);
   if (vertex_point_size) glsl_dataflow_accept_towards_leaves_postfix(vertex_point_size, NULL, dpostv_find_live_attribs, BACKEND_PASS_FIND_ATRIBS);
   result->cattribs_live = cattribs_live;
   result->vattribs_live = program->live_attributes;
   vcos_assert(!(result->cattribs_live & ~result->vattribs_live));

   nodes[0] = frag_r;
   nodes[1] = frag_g;
   nodes[2] = frag_b;
   nodes[3] = frag_a;
   nodes[4] = frag_discard;
   nodes[5] = vertex_x;
   nodes[6] = vertex_y;
   nodes[7] = vertex_z;
   nodes[8] = vertex_w;
   nodes[9] = vertex_point_size;
   for (i = 0; i < vary_count; i++)
      nodes[10+i] = vertex_vary[i];
   hblob = glsl_dataflow_copy_to_relocatable(10+vary_count, (Dataflow **)result->nodes, nodes, 0);
   result->vary_count = vary_count;
   result->mh_blob = hblob;     /* TODO naughty */

   return result->mh_blob != MEM_INVALID_HANDLE;
}
