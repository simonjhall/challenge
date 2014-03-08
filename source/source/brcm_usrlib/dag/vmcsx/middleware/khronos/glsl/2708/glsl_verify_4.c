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
#include "middleware/khronos/glsl/glsl_dataflow.h"
#include "middleware/khronos/glsl/glsl_dataflow_visitor.h"
#include "middleware/khronos/glsl/2708/glsl_allocator_4.h"
#include "middleware/khronos/glsl/2708/glsl_qdisasm_4.h"
#include "middleware/khronos/common/2708/khrn_prod_4.h"

#ifdef BACKEND_VERIFY_SHADERS

#define UNIF_MAX 1024
#define ATTR_MAX 32
#define OUT_MAX 35
#define TEX_MAX 32

#define PASS_VISITED       123456

#define MAX_INSTRUCTIONS 1000
static Dataflow *instr_guide[MAX_INSTRUCTIONS][2];
static uint32_t max_instruction;

typedef struct {
   uint32_t outputs[ATTR_MAX];
   uint32_t inputs[OUT_MAX];
   uint32_t unifs[UNIF_MAX];
   uint32_t unif_scale_x;
   uint32_t unif_scale_y;
   uint32_t unif_scale_z;
   uint32_t unif_offset_z;

   uint32_t num_outputs;
} STATE_T;

static bool truth(uint32_t value, uint32_t mode)
{
   switch (mode)
   {
   case BOOL_REP_BOOL:
      vcos_assert(value == 0 || value == 1);
      return !!value;
   case BOOL_REP_BOOL_N:
      vcos_assert(value == 0 || value == 1);
      return !value;
   case BOOL_REP_NEG:
      return (value & 0x80000000) != 0;
   case BOOL_REP_NEG_N:
      return (value & 0x80000000) == 0;
   case BOOL_REP_ZERO:
      return value == 0;
   case BOOL_REP_ZERO_N:
      return value != 0;
   default:
      UNREACHABLE();
      return false;
   }
}

static void wipe(Dataflow *dataflow)
{
   dataflow_accept_towards_leaves_postfix(dataflow, NULL, NULL, 0 /* wipe label */);
}

static uint32_t visit(Dataflow *dataflow, STATE_T *state)
{
   if (dataflow->pass != PASS_VISITED)
   {
      DataflowChainNode *node;
      uint32_t value;

      dataflow->pass = PASS_VISITED;

      for (node = dataflow->iodependencies.first; node; node = node->next)
      {
         visit(node->dataflow, state);
      }

      if (dataflow->temp != ~0 && dataflow->temp >= 4)
      {
         uint32_t timestamp = dataflow->temp >> 2;
         vcos_assert(timestamp < MAX_INSTRUCTIONS);

         if (timestamp > max_instruction)
            max_instruction = timestamp;

         if (instr_guide[timestamp][0] == NULL)
            instr_guide[timestamp][0] = dataflow;
         else
         {
            vcos_assert(instr_guide[timestamp][1] == NULL);
            instr_guide[timestamp][1] = dataflow;
         }
      }

      switch (dataflow->flavour)
      {
      case DATAFLOW_CONST_BOOL:
      case DATAFLOW_CONST_FLOAT:
         value = dataflow->u.const_int.value;
         break;
      case DATAFLOW_CONST_INT:
         *(float *)&value = (float)dataflow->u.const_int.value;
         break;
      //case DATAFLOW_CONST_SAMPLER:
      case DATAFLOW_UNIFORM:
         switch (dataflow->u.linkable_value.row)
         {
         case BACKEND_UNIFORM_SCALE_X:
            value = state->unif_scale_x; break;
         case BACKEND_UNIFORM_SCALE_Y:
            value = state->unif_scale_y; break;
         case BACKEND_UNIFORM_SCALE_Z:
            value = state->unif_scale_z; break;
         case BACKEND_UNIFORM_OFFSET_Z:
            value = state->unif_offset_z; break;
         default:
            vcos_assert((uint32_t)dataflow->u.linkable_value.row < UNIF_MAX);
            value = state->unifs[dataflow->u.linkable_value.row];
         }
         break;
      case DATAFLOW_ATTRIBUTE:
         vcos_assert((uint32_t)dataflow->u.linkable_value.row < ATTR_MAX);
         value = state->inputs[dataflow->u.linkable_value.row];
         break;
      //case DATAFLOW_VARYING:
      //case DATAFLOW_UNIFORM_OFFSET:
      //case DATAFLOW_ARITH_NEGATE:
      case DATAFLOW_LOGICAL_NOT:
         value = visit(dataflow->u.unary_op.operand, state);
         break;
      case DATAFLOW_MUL:
         value = qpu_float_mul(visit(dataflow->u.binary_op.left, state), visit(dataflow->u.binary_op.right, state));
         break;
      case DATAFLOW_ADD:
         value = qpu_float_add(visit(dataflow->u.binary_op.left, state), visit(dataflow->u.binary_op.right, state));
         break;
      case DATAFLOW_SUB:
      case DATAFLOW_LESS_THAN:
      case DATAFLOW_GREATER_THAN_EQUAL:
         value = qpu_float_sub(visit(dataflow->u.binary_op.left, state), visit(dataflow->u.binary_op.right, state));
         break;
      case DATAFLOW_RSUB:
      case DATAFLOW_LESS_THAN_EQUAL:
      case DATAFLOW_GREATER_THAN:
         value = qpu_float_sub(visit(dataflow->u.binary_op.right, state), visit(dataflow->u.binary_op.left, state));
         break;
      case DATAFLOW_EQUAL:
      case DATAFLOW_NOT_EQUAL:
      case DATAFLOW_LOGICAL_XOR:
         value = visit(dataflow->u.binary_op.left, state) ^ visit(dataflow->u.binary_op.right, state);
         break;
      case DATAFLOW_LOGICAL_AND:
         value = visit(dataflow->u.binary_op.left, state) & visit(dataflow->u.binary_op.right, state);
         break;
      case DATAFLOW_LOGICAL_OR:
         value = visit(dataflow->u.binary_op.left, state) | visit(dataflow->u.binary_op.right, state);
         break;
      case DATAFLOW_CONDITIONAL:
      {
         uint32_t t = visit(dataflow->u.cond_op.true_value, state);
         uint32_t f = visit(dataflow->u.cond_op.false_value, state);

         value = truth(visit(dataflow->u.cond_op.cond, state), dataflow_get_bool_rep(dataflow->u.cond_op.cond)) ? t : f;
         break;
      }
      case DATAFLOW_INTRINSIC_FLOATTOINT_TRUNC:
      case DATAFLOW_INTRINSIC_FLOATTOINT_NEAREST:
         value = qpu_float_ftoi(visit(dataflow->u.unary_op.operand, state));
         break;
      //case DATAFLOW_INTRINSIC_FLOATTOBOOL:
      case DATAFLOW_INTRINSIC_INTTOFLOAT:
         value = visit(dataflow->u.unary_op.operand, state);
         break;
      //case DATAFLOW_INTRINSIC_INTTOBOOL:
      //case DATAFLOW_INTRINSIC_BOOLTOINT:
      //case DATAFLOW_INTRINSIC_BOOLTOFLOAT:
      case DATAFLOW_INTRINSIC_RSQRT:
         value = sfu_recipsqrt(visit(dataflow->u.unary_op.operand, state));
         break;
      case DATAFLOW_INTRINSIC_RCP:
         value = sfu_recip(visit(dataflow->u.unary_op.operand, state));
         break;
      case DATAFLOW_INTRINSIC_LOG2:
         value = sfu_log(visit(dataflow->u.unary_op.operand, state));
         break;
      case DATAFLOW_INTRINSIC_EXP2:
         value = sfu_exp(visit(dataflow->u.unary_op.operand, state));
         break;
      //case DATAFLOW_INTRINSIC_CEIL:
      //case DATAFLOW_INTRINSIC_FLOOR:
      //case DATAFLOW_INTRINSIC_SIGN:
      case DATAFLOW_INTRINSIC_MIN:
      {
         uint32_t carry;
         value = qpu_float_min(visit(dataflow->u.binary_op.left, state), visit(dataflow->u.binary_op.right, state), &carry);
         break;
      }
      case DATAFLOW_INTRINSIC_MAX:
         uint32_t carry;
         value = qpu_float_max(visit(dataflow->u.binary_op.left, state), visit(dataflow->u.binary_op.right, state), &carry);
         break;
      case DATAFLOW_MOV:
         value = visit(dataflow->u.unary_op.operand, state);
         break;
      case DATAFLOW_COERCE_TOFLOAT:
         value = qpu_float_itof(visit(dataflow->u.unary_op.operand, state));
         break;
      //case DATAFLOW_COERCE_TOINT:
      //case DATAFLOW_THREADSWITCH:
      case DATAFLOW_TEX_SET_COORD_S:
      case DATAFLOW_TEX_SET_COORD_T:
      case DATAFLOW_TEX_SET_COORD_R:
      case DATAFLOW_TEX_SET_BIAS:
      case DATAFLOW_TEX_SET_LOD:
         //Don't visit input. DATAFLOW_TEX_GET_CMP_R will do this for us.
         value = 0;
         break;
      case DATAFLOW_TEX_GET_CMP_R:
      {
         uint32_t value_s = 0;
         uint32_t value_t = 0;
         Dataflow *sampler;
         for (node = dataflow->iodependencies.first; node; node = node->next)
         {
            if (node->dataflow->flavour == DATAFLOW_TEX_SET_COORD_S)
               value_s = visit(node->dataflow->u.texture_lookup_set.param, state);
            else if (node->dataflow->flavour == DATAFLOW_TEX_SET_COORD_T)
               value_t = visit(node->dataflow->u.texture_lookup_set.param, state);
         }

         sampler = dataflow->u.texture_lookup_get.sampler;
         if (sampler->flavour == DATAFLOW_INDEXED_UNIFORM_SAMPLER)
         {
            uint32_t unif_base = sampler->u.indexed_uniform_sampler.uniform->u.linkable_value.row;
            uint32_t unif_size = sampler->u.indexed_uniform_sampler.size;
            int index;

            vcos_assert(unif_base < UNIF_MAX && unif_size < UNIF_MAX && unif_base + unif_size < UNIF_MAX);
            index = (int)(*(float*)&value_s * unif_size);
            if (index < 0) index = 0;
            if (index >= (int)unif_size) index = unif_size - 1;
            value = state->unifs[unif_base + index];
         }
         else
         {
            vcos_assert(0);    //TODO
         }
         break;
      }
      //case DATAFLOW_TEX_GET_CMP_G:
      //case DATAFLOW_TEX_GET_CMP_B:
      //case DATAFLOW_TEX_GET_CMP_A:
      //case DATAFLOW_FRAG_GET_X:
      //case DATAFLOW_FRAG_GET_Y:
      //case DATAFLOW_FRAG_GET_Z:
      //case DATAFLOW_FRAG_GET_W:
      //case DATAFLOW_FRAG_GET_PC_X:
      //case DATAFLOW_FRAG_GET_PC_Y:
      //case DATAFLOW_FRAG_GET_FF:
      //case DATAFLOW_FRAG_SET_R:
      //case DATAFLOW_FRAG_SET_G:
      //case DATAFLOW_FRAG_SET_B:
      //case DATAFLOW_FRAG_SET_A:
      //case DATAFLOW_VARYING_C:
      //case DATAFLOW_FRAG_SUBMIT_Z:
      //case DATAFLOW_FRAG_SUBMIT_MS:
      //case DATAFLOW_FRAG_SUBMIT_ALL:
      case DATAFLOW_VERTEX_SET:
         vcos_assert(state->num_outputs < OUT_MAX);
         state->outputs[state->num_outputs] = visit(dataflow->u.vertex_set.param, state);
         state->num_outputs++;
         value = 0;
         break;
      //case DATAFLOW_PACK_COL:
      case DATAFLOW_PACK_INT16:
         value = visit(dataflow->u.pack.b, state) << 16 | (visit(dataflow->u.pack.a, state) & 0xffff);
         break;
      //case DATAFLOW_UNPACK_COL_R:
      //case DATAFLOW_UNPACK_COL_G:
      //case DATAFLOW_UNPACK_COL_B:
      //case DATAFLOW_UNPACK_COL_A:
      case DATAFLOW_SHIFT_RIGHT:
      case DATAFLOW_LOGICAL_SHR:
         value = visit(dataflow->u.binary_op.left, state) >> visit(dataflow->u.binary_op.right, state);
         break;
      //case DATAFLOW_INDEXED_UNIFORM_SAMPLER:
      default:
         UNREACHABLE();
         value = 0;
      }

      dataflow->slot = value;
   }

   return dataflow->slot;
}

static void test_shader(const void *original_shader, uint32_t num_instr, const uint32_t *unif_map, uint32_t num_unif, STATE_T *state, uint32_t num_outputs)
{
   MEM_HANDLE_T hshader, hunif, hout;
   MEM_HANDLE_T htexs[TEX_MAX];
   uint32_t *shader;
   uint32_t *sptr;
   uint32_t *unifs;
   uint32_t *out;
   uint32_t i;
   uint32_t tex_count = 0;

   memset(htexs, MEM_HANDLE_INVALID, sizeof(htexs));

   num_instr -= 3;      /* Ignore the thrend/nop/nop at the end */

   hunif = mem_alloc(4 * num_unif, 4, MEM_FLAG_NONE, "unifs");
   hshader = mem_alloc(8 * (ATTR_MAX + 1 + num_instr + 4), 4, MEM_FLAG_NONE, "shader");
   hout = mem_alloc(4 * num_outputs, 4, MEM_FLAG_NONE, "out");
   vcos_assert(hunif != MEM_HANDLE_INVALID && hshader != MEM_HANDLE_INVALID && hout != MEM_HANDLE_INVALID);

   shader = (uint32_t *)mem_lock(hshader);
   unifs = (uint32_t *)mem_lock(hunif);
   out = (uint32_t *)mem_lock(hout);

   /*
      Build shader
      This consists of:
      - moving inputs into ra slots
      - setting up VPM writes
      - the shader we were given (without final thrend)
      - using VDW to copy VPM to memory (and thrend)
   */

   // -- moving inputs into ra slots --
   sptr = shader;
   for (i = 0; i < ATTR_MAX; i++)
   {
      //sig=14, cond_add=1, waddr_add=i, waddr_mul=39            ldi ra(i), x
      sptr[1] = 14 << 28 | 1 << 17 | i << 6 | 39 << 0;
      sptr[0] = state->inputs[i];
      sptr += 2;
   }

   // -- setting up VPM writes --
   //sig=14, cond_add=1, ws=1, waddr_add=49, waddr_mul=39        ldi vpmw_setup, x
   sptr[1] = 14 << 28 | 1 << 17 | 1 << 12 | 49 << 6 | 39 << 0;
   sptr[0] = 0x00001a00;
   sptr += 2;

   // -- the shader we were given (without final thrend) --
   memcpy(sptr, original_shader, 8 * num_instr);
   sptr += 2 * num_instr;

   // -- using VDW to copy VPM to memory --
   //sig=14, cond_add=1, ws=1, waddr_add=49, waddr_mul=39        ldi vdw_setup, x
   sptr[1] = 14 << 28 | 1 << 17 | 1 << 12 | 49 << 6 | 39 << 0;
   //id=2, units=num_outputs, depth=1, horiz=1, vpm_base=0, offsize=0
   sptr[0] = 2 << 30 | num_outputs << 23 | 1 << 16 | 1 << 14 | 0 << 3 | 0 << 0;

   //sig=3, waddr_add=39, waddr_mul=39                     -- thrend --
   sptr[3] = 3 << 28 | 39 << 6 | 39;
   //raddr_a=39, raddr_b=39
   sptr[2] = 39 << 18 | 39 << 12;

   //sig=14, cond_add=1, ws=1, waddr_add=49, waddr_mul=39        ldi vdw_setup, x
   sptr[5] = 14 << 28 | 1 << 17 | 1 << 12 | 49 << 6 | 39 << 0;
   //id=3, stride=0
   sptr[4] = 3 << 30 | 0 << 0;

   //sig=14, cond_add=1, ws=1, waddr_add=50, waddr_mul=39        ldi vdw_addr, x
   sptr[7] = 14 << 28 | 1 << 17 | 1 << 12 | 50 << 6 | 39 << 0;
   sptr[6] = khrn_hw_addr(out);

   /*
      Set up uniforms
   */
   for (i = 0; i < num_unif; i++)
   {
      uint32_t u0 = unif_map[2*i];
      uint32_t u1 = unif_map[2*i+1];
      uint32_t value;

      switch (u0)
      {
      case BACKEND_UNIFORM:
         value = state->unifs[u1];
         break;
      case BACKEND_UNIFORM_LITERAL:
         value = u1;
         break;
      case BACKEND_UNIFORM_SCALE_X:
         value = state->unif_scale_x;
         break;
      case BACKEND_UNIFORM_SCALE_Y:
         value = state->unif_scale_y;
         break;
      case BACKEND_UNIFORM_SCALE_Z:
         value = state->unif_scale_z;
         break;
      case BACKEND_UNIFORM_OFFSET_Z:
         value = state->unif_offset_z;
         break;
      case BACKEND_UNIFORM_TEX_UNIF|BACKEND_UNIFORM_TEX_PARAM0:
      {
         uint32_t index = u1 & 0xffff;
         uint32_t size = u1 >> 16;
         MEM_HANDLE_T htex;
         uint32_t i;
         uint32_t *tex;

         htex = mem_alloc_ex(64 * ((size + 3) / 4), 4096, MEM_FLAG_DIRECT, "Temporary texture allocation (for indexed uniforms)", MEM_COMPACT_DISCARD);
         vcos_assert(htex != MEM_HANDLE_INVALID);       //TODO
         vcos_assert(tex_count < TEX_MAX);
         MEM_ASSIGN(htexs[tex_count], htex);
         mem_release(htex);
         tex_count++;
         tex = (uint32_t *)mem_lock(htex);

         for (i = 0; i < size; i++)
         {
            vcos_assert(index + i < UNIF_MAX);
            tex[(i & ~3) << 2 | (i & 3)] = state->unifs[index + i];
         }

         value =
            0 << 0 |        // no mipmaps
            0 << 4 |        // type = RGBA_8888
            khrn_hw_addr(tex);
         break;
      }
      case BACKEND_UNIFORM_TEX_UNIF|BACKEND_UNIFORM_TEX_PARAM1:
      {
         uint32_t size = u1 >> 16;
         value =
            1 << 0 |        // wrap_s
            1 << 2 |        // wrap_t
            1 << 4 |        // min=nearest
            1 << 7 |        // mag=nearest
            size << 8 |     // width
            0 << 19 |       // etcflipy
            1 << 20;        // height
         break;
      }
      default:
         UNREACHABLE();
      }

      unifs[i] = value;
   }

   simpenrose_do_user_shader(khrn_hw_addr(shader), khrn_hw_addr(unifs), 0);
   memcpy(state->outputs, out, 4 * num_outputs);

   mem_unlock(hshader);
   mem_unlock(hunif);
   mem_unlock(hout);
   mem_release(hshader);
   mem_release(hunif);
   mem_release(hout);
   for (i = 0; i < tex_count; i++)
   {
      mem_unlock(htexs[i]);
      MEM_ASSIGN(htexs[i], MEM_HANDLE_INVALID);
   }
}

static void reset_random(uint32_t seed)
{
   srand(seed);
}

static uint32_t random_float()
{
   float result = (float)rand() * 4.0f / (float)RAND_MAX;
   return *(uint32_t *)&result;
}

void glsl_backend_verify_vertex_shader(uint32_t seed, Dataflow *dataflow, const void *shader, uint32_t num_instr, const uint32_t *unif_map, uint32_t num_unif, uint32_t num_outputs)
{
   STATE_T df_state;
   STATE_T sh_state;
   uint32_t i;

   reset_random(seed);

   for (i = 0; i < ATTR_MAX; i++)
   {
      df_state.inputs[i] = sh_state.inputs[i] = random_float();
   }
   for (i = 0; i < UNIF_MAX; i++)
   {
      df_state.unifs[i] = sh_state.unifs[i] = random_float();
   }
   df_state.unif_scale_x = sh_state.unif_scale_x = random_float();
   df_state.unif_scale_y = sh_state.unif_scale_y = random_float();
   df_state.unif_scale_z = sh_state.unif_scale_z = random_float();
   df_state.unif_offset_z = sh_state.unif_offset_z = random_float();
   df_state.num_outputs = 0;

   memset(instr_guide, 0, sizeof(instr_guide));
   max_instruction = 0;
   visit(dataflow, &df_state);
   vcos_assert(df_state.num_outputs == num_outputs);
   test_shader(shader, num_instr, unif_map, num_unif, &sh_state, num_outputs);

   for (i = 0; i < num_outputs; i++)
   {
      if (df_state.outputs[i] != sh_state.outputs[i])
      {
         for (i = 0; i <= max_instruction; i++)
         {
            char buffer[50];
            Dataflow *d0, *d1;
            d0 = instr_guide[i][0];
            d1 = instr_guide[i][1];
            if (d0)
            {
               if (d0->flavour == DATAFLOW_ADD || d0->flavour == DATAFLOW_MUL)
               {
                  sprintf(buffer, "%08x & %08x = ", d0->u.binary_op.left->slot, d0->u.binary_op.right->slot);
                  debug_print(buffer);
               }
               sprintf(buffer, "%08x ", d0->slot);
               debug_print(buffer);
            }
            if (d1)
            {
               sprintf(buffer, "%08x", d1->slot);
               debug_print(buffer);
            }
            backend_qdisasm_instruction(buffer, 50, ((uint32_t*)shader)[2*i], ((uint32_t*)shader)[2*i+1]);
            debug_print(buffer);
            debug_print("\n");
         }
         test_shader(shader, num_instr, unif_map, num_unif, &sh_state, num_outputs);
      }
      vcos_assert(df_state.outputs[i] == sh_state.outputs[i]);
   }
   wipe(dataflow);
}

#endif //BACKEND_VERIFY_SHADERS
