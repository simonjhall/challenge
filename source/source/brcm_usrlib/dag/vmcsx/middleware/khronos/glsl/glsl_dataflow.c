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
#include <assert.h>

#include "middleware/khronos/glsl/glsl_dataflow.h"
#include "middleware/khronos/glsl/glsl_trace.h"
#include "middleware/khronos/glsl/glsl_ast_visitor.h"
#include "middleware/khronos/glsl/glsl_errors.h"
#include "middleware/khronos/glsl/glsl_fastmem.h"
#include "middleware/khronos/glsl/glsl_stack.h"

#include "middleware/khronos/glsl/glsl_const_operators.h"

#include "interface/khronos/include/GLES2/gl2ext.h"

#ifdef __VIDEOCORE4__
#include "middleware/khronos/glsl/glsl_backend.h"

static Dataflow *boolify(Dataflow *dataflow, uint32_t *rep_out);
static void set_bool_rep(Dataflow *dataflow, uint32_t bool_rep);

#endif

// Prototype for the function that brings it all together, so that the individual functions can recurse.
static void expr_calculate_dataflow(Dataflow** scalar_values, Expr* expr);

#ifdef BUILD_FOR_DAVES_TEST
   /* Dave's test verifies that results are bit accurate, including multiplying by constant zeros and
      expecting the sign of such to be preserved.  This means that some optimisations have to
	  be turned off.  They must not be turned off in normal usage as they provide
	  a very valuable gain */
#   define DATAFLOW_DONT_OPTIMISE_MUL_CONST_ZERO
#endif

#ifdef DATAFLOW_LOG
static int dataflow_log_allocs[256];

void glsl_dataflow_log_init()
{
   int i;

   for (i = 0; i < 256; i++)
      dataflow_log_allocs[i] = 0;
}

void glsl_dataflow_log_dump()
{
   int i;

   for (i = 0; i < 256; i++)
      if (dataflow_log_allocs[i])
         printf("%d, %d\n", i, dataflow_log_allocs[i]);
}

#define DATAFLOW_LOG_ALLOC(x) do { dataflow_log_allocs[x] += sizeof(Dataflow); } while(0)
#else
#define DATAFLOW_LOG_ALLOC(x)
#endif

//
// Scalar value stack.
//

#define STACK_MAX_SCALAR_COUNT   128

typedef struct STACK_FRAME {
   Dataflow *scalar_values[STACK_MAX_SCALAR_COUNT];

   struct STACK_FRAME *next;
} STACK_FRAME_T;

static STACK_FRAME_T *free_frames;
static STACK_FRAME_T *used_frames;

void glsl_init_dataflow_stack()
{
   free_frames = NULL;
   used_frames = NULL;
}

static Dataflow **stack_alloc_by_size(int size)
{
   STACK_FRAME_T *frame;

   if (size > STACK_MAX_SCALAR_COUNT) {
      assert(0);     // TODO: compiler error here
   }

   if (free_frames == NULL)
      frame = (STACK_FRAME_T *)malloc_fast(sizeof(STACK_FRAME_T));
   else {
      frame = free_frames;
      free_frames = free_frames->next;
   }

   frame->next = used_frames;
   used_frames = frame;

   return frame->scalar_values;
}

static Dataflow **stack_alloc_by_type(SymbolType *type)
{
   return stack_alloc_by_size(type->scalar_count);
}

static void stack_free(void)
{
   STACK_FRAME_T *frame = used_frames;

   vcos_assert(frame);

   used_frames = frame->next;

   frame->next = free_frames;
   free_frames = frame;
}

//
// Dataflow chain functions.
//

void glsl_dataflow_chain_init(DataflowChain* chain)
{
	chain->first = NULL;
	chain->last = NULL;
	chain->count = 0;
}

DataflowChain* glsl_dataflow_chain_append(DataflowChain* chain, Dataflow* dataflow)
{
	DataflowChainNode* node = (DataflowChainNode *)malloc_fast(sizeof(DataflowChainNode));

	node->dataflow = dataflow;

	node->unlinked = false;
	node->prev = chain->last;
	node->next = NULL;

	if (!chain->first) chain->first = node;
	if (chain->last) chain->last->next = node;
	chain->last = node;

	chain->count++;

	return chain;
}

DataflowChain* glsl_dataflow_chain_remove_node(DataflowChain* chain, DataflowChainNode* node)
{
	// Update nodes.
	if (node->prev) node->prev->next = node->next;
	if (node->next) node->next->prev = node->prev;

	// Update chain.
	chain->count--;
	if (chain->first == node) chain->first = node->next;
	if (chain->last == node) chain->last = node->prev;

   if (chain->count == 0) {
      vcos_assert(!chain->first);
      vcos_assert(!chain->last);
   }

	// Update node.
	node->unlinked = true;

	return chain;
}

DataflowChain* glsl_dataflow_chain_remove(DataflowChain *chain, Dataflow *dataflow)
{
   DataflowChainNode *node;

   for (node = chain->first; node; node = node->next)
      if (node->dataflow == dataflow)
         glsl_dataflow_chain_remove_node(chain, node);

   return chain;
}

DataflowChain* glsl_dataflow_chain_replace(DataflowChain *chain, Dataflow *dataflow_old, Dataflow *dataflow_new)
{
   DataflowChainNode *node;

   for (node = chain->first; node; node = node->next)
      if (node->dataflow == dataflow_old)
         node->dataflow = dataflow_new;

   return chain;
}

DataflowChain *glsl_dataflow_chain_filter(DataflowChain *dst, DataflowChain *src, void *data, DataflowFilter filter)
{
   DataflowChainNode *node;

   for (node = src->first; node; node = node->next) {
      Dataflow *dataflow = node->dataflow;

      if (filter(dataflow, data))
         glsl_dataflow_chain_append(dst, dataflow);
   }

   return dst;
}

bool glsl_dataflow_chain_contains(DataflowChain *chain, Dataflow *dataflow)
{
   DataflowChainNode *node;

   for (node = chain->first; node; node = node->next)
      if (node->dataflow == dataflow)
         return true;

   return false;
}

//
// Utility function to initialise scheduler-specific fields
//

static void init_backend_fields(Dataflow *dataflow, int deps_remaining)
{
   dataflow->delay = 0;
#ifdef __VIDEOCORE4__
   {
      int i;
      for (i=0; i<TMU_DEP_WORD_COUNT; i++) dataflow->tmu_dependencies[i] = 0;
      dataflow->copy = NULL;
   }
#else
   dataflow->etime = 0;
#endif
   dataflow->deps_remaining = deps_remaining;
   dataflow->live = false;

   dataflow->index = -1;
   dataflow->phase = -1;

   dataflow->temp = -1;
   dataflow->slot = -1;
   dataflow->protect = false;

   dataflow->repr = REPR_GENUINE;
}

//
// Dataflow constructors.
//

Dataflow* glsl_dataflow_construct_const_bool(int line_num, const_bool value)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(DATAFLOW_CONST_BOOL);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;

	dataflow->flavour = DATAFLOW_CONST_BOOL;
	dataflow->type_index = PRIM_BOOL;

	glsl_dataflow_clear_chains(dataflow);

	dataflow->u.const_bool.value = value;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 0);

#ifdef __VIDEOCORE4__
   set_bool_rep(dataflow, BOOL_REP_BOOL);
#endif

	return dataflow;
}

Dataflow* glsl_dataflow_construct_const_int(int line_num, const_int value)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(DATAFLOW_CONST_INT);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;

	dataflow->flavour = DATAFLOW_CONST_INT;
	dataflow->type_index = PRIM_INT;
	glsl_dataflow_clear_chains(dataflow);

	dataflow->u.const_int.value = value;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 0);

	return dataflow;
}

Dataflow* glsl_dataflow_construct_const_float(int line_num, const_float value)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(DATAFLOW_CONST_FLOAT);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;

	dataflow->flavour = DATAFLOW_CONST_FLOAT;
	dataflow->type_index = PRIM_FLOAT;
	glsl_dataflow_clear_chains(dataflow);

	dataflow->u.const_float.value = value;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 0);

	return dataflow;
}

Dataflow* glsl_dataflow_construct_const_sampler(int line_num, PrimitiveTypeIndex type_index)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(DATAFLOW_CONST_SAMPLER);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;

	dataflow->flavour = DATAFLOW_CONST_SAMPLER;

   switch (type_index)
   {
      case PRIM_SAMPLER2D:
      case PRIM_SAMPLERCUBE:
         dataflow->type_index = type_index;
         break;

      default:
         UNREACHABLE();
         return NULL;
   }

	glsl_dataflow_clear_chains(dataflow);

   dataflow->u.const_sampler.location = SAMPLER_LOCATION_UNDEFINED;
	dataflow->u.const_sampler.name = SAMPLER_NAME_UNDEFINED;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 0);

	return dataflow;
}

Dataflow* glsl_dataflow_construct_linkable_value(int line_num, DataflowFlavour flavour, PrimitiveTypeIndex type_index)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;

	dataflow->flavour = flavour;
	dataflow->type_index = type_index;
	glsl_dataflow_clear_chains(dataflow);

   dataflow->u.linkable_value.row = LINKABLE_VALUE_ROW_UNDEFINED;
   dataflow->u.linkable_value.name = LINKABLE_VALUE_NAME_UNDEFINED;

#ifdef __VIDEOCORE4__
   if (type_index == PRIM_BOOL)
      set_bool_rep(dataflow, BOOL_REP_BOOL);
#endif

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 0);

	return dataflow;
}

#define COORD_DIM    3
#define COORD_S      0
#define COORD_T      1
#define COORD_R      2

enum
{
   TEXTURE_2D,
   TEXTURE_CUBE
} texture_type;

enum
{
   LOOKUP_BIAS,
   LOOKUP_LOD
} lookup_type;

static TextureUnit last_used_texture_unit;
// Next available sampler slot for each texture unit.
static int next_sampler_index_offsets[TEXTURE_UNIT_COUNT];
// gadget_get_r of the last gadget to use each texture unit, to enforce dependencies.
Dataflow* last_gadget_get_r[TEXTURE_UNIT_COUNT];

Dataflow* glsl_dataflow_construct_linkable_value_offset(int line_num, DataflowFlavour flavour, PrimitiveTypeIndex type_index, Dataflow* linkable_value, Dataflow* offset)
{
#ifdef __VIDEOCORE4__
   Dataflow *pseudo_sampler;
   Dataflow *gadget_set_t;
   Dataflow *gadget_set_s;
   Dataflow *gadget_get_rgba;
   TextureUnit current_texture_unit = TEXTURE_UNIT_A;

   UNUSED_NDEBUG(flavour);
   UNUSED(type_index);

   vcos_assert(DATAFLOW_UNIFORM_OFFSET == flavour); // only uniforms can be accessed dynamically
#ifdef __BCM2708A0__
   vcos_assert(DATAFLOW_INDEXED_UNIFORM_SAMPLER == linkable_value->flavour);

   pseudo_sampler = linkable_value;

   gadget_set_t = glsl_dataflow_construct_texture_lookup_set(
      line_num,
      DATAFLOW_TEX_SET_COORD_T,
      pseudo_sampler,
      glsl_dataflow_construct_const_float(line_num, CONST_FLOAT_ZERO),
      last_gadget_get_r[current_texture_unit],
      NULL,
      NULL);
#else
   UNUSED_NDEBUG(linkable_value);
   vcos_assert(linkable_value == NULL);
   pseudo_sampler = NULL;
   gadget_set_t = NULL;
#endif

   gadget_set_s = glsl_dataflow_construct_texture_lookup_set(
      line_num,
#ifdef __BCM2708A0__
      DATAFLOW_TEX_SET_COORD_S,
#else
      DATAFLOW_TEX_SET_DIRECT,
#endif
      pseudo_sampler,
      offset,
      NULL,
      gadget_set_t,
      NULL);

   gadget_get_rgba = glsl_dataflow_construct_texture_lookup_get(
      line_num,
      DATAFLOW_TEX_GET_CMP_R,
      pseudo_sampler,
      gadget_set_s,
      last_gadget_get_r[current_texture_unit],
      NULL);

   // Finally, save dependencies for next lookup.
   //last_gadget_get_r[current_texture_unit] = gadget_get_rgba;
   //last_used_texture_unit = current_texture_unit;

   return gadget_get_rgba;
#else
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;

   vcos_assert(DATAFLOW_UNIFORM_OFFSET == flavour); // only uniforms can be accessed dynamically
	dataflow->flavour = flavour;
	dataflow->type_index = type_index;
	glsl_dataflow_clear_chains(dataflow);

   vcos_assert(DATAFLOW_UNIFORM == linkable_value->flavour);
   dataflow->u.linkable_value_offset.linkable_value = linkable_value;
	vcos_assert(PRIM_INT == offset->type_index);
   dataflow->u.linkable_value_offset.offset = offset;

	// Add this node to the lists of dependents of the nodes it depends on.
   glsl_dataflow_add_dependent(linkable_value, dataflow);
	glsl_dataflow_add_dependent(offset, dataflow);

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 2);

	return dataflow;
#endif
}

Dataflow* glsl_dataflow_construct_unary_op(int line_num, DataflowFlavour flavour, Dataflow* operand)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;

   if (operand->flavour == DATAFLOW_CONST_FLOAT) {
      // constant folding for floats

      const_int i;
      const_float f;

      switch (flavour) {
      case DATAFLOW_ARITH_NEGATE:
      {
         op_arith_negate__const_float__const_float(&f, &operand->u.const_float.value);

         return glsl_dataflow_construct_const_float(line_num, f);
      }
      case DATAFLOW_INTRINSIC_FLOATTOINT_TRUNC:
      {
         op_floattoint_trunc__const_int__const_float(&i, &operand->u.const_float.value);

         return glsl_dataflow_construct_const_int(line_num, i);
      }
      case DATAFLOW_INTRINSIC_FLOATTOINT_NEAREST:
      {
         op_floattoint_nearest__const_int__const_float(&i, &operand->u.const_float.value);

         return glsl_dataflow_construct_const_int(line_num, i);
      }
      case DATAFLOW_INTRINSIC_FLOATTOBOOL:
      {
         return const_bool_from_float(operand->u.const_float.value) ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_INTRINSIC_RSQRT:
      {
         op_rsqrt__const_float__const_float(&f, &operand->u.const_float.value);

         return glsl_dataflow_construct_const_float(line_num, f);
      }
      case DATAFLOW_INTRINSIC_RCP:
      {
         op_recip__const_float__const_float(&f, &operand->u.const_float.value);

         return glsl_dataflow_construct_const_float(line_num, f);
      }
      case DATAFLOW_INTRINSIC_LOG2:
      {
         op_log2__const_float__const_float(&f, &operand->u.const_float.value);

         return glsl_dataflow_construct_const_float(line_num, f);
      }
      case DATAFLOW_INTRINSIC_EXP2:
      {
         op_exp2__const_float__const_float(&f, &operand->u.const_float.value);

         return glsl_dataflow_construct_const_float(line_num, f);
      }
      case DATAFLOW_INTRINSIC_CEIL:
      {
         op_ceil__const_float__const_float(&f, &operand->u.const_float.value);

         return glsl_dataflow_construct_const_float(line_num, f);
      }
      case DATAFLOW_INTRINSIC_FLOOR:
      {
         op_floor__const_float__const_float(&f, &operand->u.const_float.value);

         return glsl_dataflow_construct_const_float(line_num, f);
      }
      case DATAFLOW_INTRINSIC_SIGN:
      {
         const_float z = CONST_FLOAT_ZERO;

         op_cmp__const_int__const_float__const_float(&i, &operand->u.const_float.value, &z);

         return glsl_dataflow_construct_const_float(line_num, const_float_from_int(i));
      }
      case DATAFLOW_MOV:         // a mov is there for a reason; don't fold it
         break;
      default:
         UNREACHABLE();
         break;
      }
   }

   if (operand->flavour == DATAFLOW_CONST_INT) {
      // constant folding for floats

      const_int i;
      const_float f;

      switch (flavour) {
      case DATAFLOW_ARITH_NEGATE:
      {
         op_arith_negate__const_int__const_int(&i, &operand->u.const_int.value);

         return glsl_dataflow_construct_const_int(line_num, i);
      }
      case DATAFLOW_INTRINSIC_INTTOFLOAT:
      {
         op_inttofloat__const_float__const_int(&f, &operand->u.const_int.value);

         return glsl_dataflow_construct_const_float(line_num, f);
      }
      case DATAFLOW_INTRINSIC_INTTOBOOL:
      {
         return const_bool_from_int(operand->u.const_int.value) ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_INTRINSIC_RCP:     // a recip is there because we're doing integer division
      case DATAFLOW_MOV:               // a mov is there for a reason; don't fold it
         break;
      case DATAFLOW_COERCE_TOFLOAT:
         return operand;
      case DATAFLOW_COERCE_TOINT:
      {
         Dataflow *value = glsl_dataflow_construct_const_float(line_num, operand->u.const_int.value);
         value->type_index = PRIM_INT;
         return value;
      }
#ifdef __VIDEOCORE4__
      case DATAFLOW_BITWISE_NOT:
         break;
#endif
      default:
         UNREACHABLE();
         break;
      }
   }

   if (operand->flavour == DATAFLOW_CONST_BOOL) {
      // constant folding for floats

      const_bool b;

      switch (flavour) {
      case DATAFLOW_LOGICAL_NOT:
      {
         op_logical_not__const_bool__const_bool(&b, &operand->u.const_bool.value);

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_INTRINSIC_BOOLTOINT:
      {
         return operand->u.const_bool.value ? g_IntOne : g_IntZero;
      }
      case DATAFLOW_INTRINSIC_BOOLTOFLOAT:
      {
         return operand->u.const_bool.value ? g_FloatOne : g_FloatZero;
      }
      case DATAFLOW_MOV:         // a mov is there for a reason; don't fold it
         break;
      default:
         UNREACHABLE();
         break;
      }
   }

   if (flavour == DATAFLOW_LOGICAL_NOT) {
      /*
         negation of relational ops
      */

	   switch (operand->flavour)
	   {
#if 0
         case DATAFLOW_EQUAL:
            return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_NOT_EQUAL, operand->u.binary_op.left, operand->u.binary_op.right);
         case DATAFLOW_NOT_EQUAL:
            return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_EQUAL, operand->u.binary_op.left, operand->u.binary_op.right);
         case DATAFLOW_LESS_THAN:
            return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_GREATER_THAN_EQUAL, operand->u.binary_op.left, operand->u.binary_op.right);
         case DATAFLOW_LESS_THAN_EQUAL:
            return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_GREATER_THAN, operand->u.binary_op.left, operand->u.binary_op.right);
         case DATAFLOW_GREATER_THAN:
            return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LESS_THAN_EQUAL, operand->u.binary_op.left, operand->u.binary_op.right);
         case DATAFLOW_GREATER_THAN_EQUAL:
            return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LESS_THAN, operand->u.binary_op.left, operand->u.binary_op.right);
#endif
         case DATAFLOW_LOGICAL_NOT:
            return operand->u.unary_op.operand;
         default:
            break;
	   }
   }

#ifdef __VIDEOCORE4__
   /* No negate instruction */
   if (flavour == DATAFLOW_ARITH_NEGATE)
   {
      return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_SUB, glsl_dataflow_construct_const_float(line_num, CONST_FLOAT_ZERO), operand);
   }
   /* No floor instruction */
   if (flavour == DATAFLOW_INTRINSIC_FLOOR)
   {
      operand = glsl_dataflow_construct_unary_op(line_num, DATAFLOW_INTRINSIC_FLOATTOINT_TRUNC, operand);
      return glsl_dataflow_construct_unary_op(line_num, DATAFLOW_COERCE_TOFLOAT, operand);
   }
#endif

	switch (flavour)
	{
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
      case DATAFLOW_BITWISE_NOT:
#endif
			dataflow->flavour = flavour;
			break;

		default:
			UNREACHABLE();
			return NULL;
	}

	switch (flavour)
	{
      case DATAFLOW_INTRINSIC_FLOATTOBOOL:
      case DATAFLOW_INTRINSIC_INTTOBOOL:
         dataflow->type_index = PRIM_BOOL;
         break;
      case DATAFLOW_INTRINSIC_FLOATTOINT_TRUNC:
      case DATAFLOW_INTRINSIC_FLOATTOINT_NEAREST:
      case DATAFLOW_INTRINSIC_BOOLTOINT:
      case DATAFLOW_COERCE_TOFLOAT:
      case DATAFLOW_COERCE_TOINT:
         dataflow->type_index = PRIM_INT;
         break;
      case DATAFLOW_INTRINSIC_INTTOFLOAT:
      case DATAFLOW_INTRINSIC_BOOLTOFLOAT:
         dataflow->type_index = PRIM_FLOAT;
         break;
      case DATAFLOW_ARITH_NEGATE:
      case DATAFLOW_LOGICAL_NOT:
      case DATAFLOW_INTRINSIC_RSQRT:
      case DATAFLOW_INTRINSIC_RCP:
      case DATAFLOW_INTRINSIC_LOG2:
      case DATAFLOW_INTRINSIC_EXP2:
      case DATAFLOW_INTRINSIC_CEIL:
      case DATAFLOW_INTRINSIC_FLOOR:
      case DATAFLOW_INTRINSIC_SIGN:
      case DATAFLOW_MOV:
			dataflow->type_index = operand->type_index;
			break;
#ifdef __VIDEOCORE4__
      case DATAFLOW_BITWISE_NOT:
         dataflow->type_index = PRIM_INT;
         break;
#endif

		default:
			UNREACHABLE();
			return NULL;
	}

	glsl_dataflow_clear_chains(dataflow);

	// Add this node to the lists of dependents of the nodes it depends on.
	glsl_dataflow_add_dependent(operand, dataflow);

	dataflow->u.unary_op.operand = operand;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 1);

#ifdef __VIDEOCORE4__
   if (flavour == DATAFLOW_LOGICAL_NOT)
   {
      uint32_t rep = 0;

      switch (glsl_dataflow_get_bool_rep(operand))
      {
      case BOOL_REP_BOOL:   rep = BOOL_REP_BOOL_N; break;
      case BOOL_REP_BOOL_N: rep = BOOL_REP_BOOL;   break;
      case BOOL_REP_NEG:    rep = BOOL_REP_NEG_N;  break;
      case BOOL_REP_NEG_N:  rep = BOOL_REP_NEG;    break;
      case BOOL_REP_ZERO:   rep = BOOL_REP_ZERO_N; break;
      case BOOL_REP_ZERO_N: rep = BOOL_REP_ZERO;   break;
      default:
         UNREACHABLE();
      }

      set_bool_rep(dataflow, rep);
   }
   if (flavour == DATAFLOW_MOV && dataflow->type_index == PRIM_BOOL)
      set_bool_rep(dataflow, glsl_dataflow_get_bool_rep(operand));
#endif

	return dataflow;
}

Dataflow* glsl_dataflow_construct_binary_op(int line_num, DataflowFlavour flavour, Dataflow* left, Dataflow* right)
{
	Dataflow* dataflow;

   vcos_assert(left);
   vcos_assert(right);

   if (left->flavour == DATAFLOW_CONST_FLOAT && right->flavour == DATAFLOW_CONST_FLOAT) {
      // constant folding for floats

      const_float f;
      const_bool b;

      switch (flavour) {
      case DATAFLOW_MUL:
      {
         op_mul__const_float__const_float__const_float(&f, &left->u.const_float.value, &right->u.const_float.value);

         return glsl_dataflow_construct_const_float(line_num, f);
      }
      case DATAFLOW_ADD:
      {
         op_add__const_float__const_float__const_float(&f, &left->u.const_float.value, &right->u.const_float.value);

         return glsl_dataflow_construct_const_float(line_num, f);
      }
      case DATAFLOW_SUB:
      {
         op_sub__const_float__const_float__const_float(&f, &left->u.const_float.value, &right->u.const_float.value);

         return glsl_dataflow_construct_const_float(line_num, f);
      }
      case DATAFLOW_RSUB:
      {
         op_sub__const_float__const_float__const_float(&f, &right->u.const_float.value, &left->u.const_float.value);

         return glsl_dataflow_construct_const_float(line_num, f);
      }
      case DATAFLOW_LESS_THAN:
      {
         op_less_than__const_bool__const_float__const_float(&b, &left->u.const_float.value, &right->u.const_float.value);

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_LESS_THAN_EQUAL:
      {
         op_less_than_equal__const_bool__const_float__const_float(&b, &left->u.const_float.value, &right->u.const_float.value);

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_GREATER_THAN:
      {
         op_greater_than__const_bool__const_float__const_float(&b, &left->u.const_float.value, &right->u.const_float.value);

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_GREATER_THAN_EQUAL:
      {
         op_greater_than_equal__const_bool__const_float__const_float(&b, &left->u.const_float.value, &right->u.const_float.value);

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_EQUAL:
      {
         b = left->u.const_float.value == right->u.const_float.value;

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_NOT_EQUAL:
      {
         b = left->u.const_float.value != right->u.const_float.value;

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_INTRINSIC_MIN:
      {
         op_min__const_float__const_float__const_float(&f, &left->u.const_float.value, &right->u.const_float.value);

         return glsl_dataflow_construct_const_float(line_num, f);
      }
      case DATAFLOW_INTRINSIC_MAX:
      {
         op_max__const_float__const_float__const_float(&f, &left->u.const_float.value, &right->u.const_float.value);

         return glsl_dataflow_construct_const_float(line_num, f);
      }
#ifdef __VIDEOCORE4__
      case DATAFLOW_BITWISE_AND:
      case DATAFLOW_BITWISE_OR:
      case DATAFLOW_BITWISE_XOR:
      case DATAFLOW_V8MULD:
      case DATAFLOW_V8MIN:
      case DATAFLOW_V8MAX:
      case DATAFLOW_V8ADDS:
      case DATAFLOW_V8SUBS:
      case DATAFLOW_INTEGER_ADD:
         /* TODO: ? */
         break;
#endif
      default:
         UNREACHABLE();
         break;
      }
   }

   if (left->flavour == DATAFLOW_CONST_INT && right->flavour == DATAFLOW_CONST_INT) {
      // constant folding for ints

      const_int i;
      const_bool b;

      switch (flavour) {
      case DATAFLOW_MUL:
      {
         op_mul__const_int__const_int__const_int(&i, &left->u.const_int.value, &right->u.const_int.value);

         return glsl_dataflow_construct_const_int(line_num, i);
      }
      case DATAFLOW_ADD:
      {
         op_add__const_int__const_int__const_int(&i, &left->u.const_int.value, &right->u.const_int.value);

         return glsl_dataflow_construct_const_int(line_num, i);
      }
      case DATAFLOW_SUB:
      {
         op_sub__const_int__const_int__const_int(&i, &left->u.const_int.value, &right->u.const_int.value);

         return glsl_dataflow_construct_const_int(line_num, i);
      }
      case DATAFLOW_RSUB:
      {
         op_sub__const_int__const_int__const_int(&i, &right->u.const_int.value, &left->u.const_int.value);

         return glsl_dataflow_construct_const_int(line_num, i);
      }
      case DATAFLOW_LESS_THAN:
      {
         op_less_than__const_bool__const_int__const_int(&b, &left->u.const_int.value, &right->u.const_int.value);

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_LESS_THAN_EQUAL:
      {
         op_less_than_equal__const_bool__const_int__const_int(&b, &left->u.const_int.value, &right->u.const_int.value);

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_GREATER_THAN:
      {
         op_greater_than__const_bool__const_int__const_int(&b, &left->u.const_int.value, &right->u.const_int.value);

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_GREATER_THAN_EQUAL:
      {
         op_greater_than_equal__const_bool__const_int__const_int(&b, &left->u.const_int.value, &right->u.const_int.value);

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_EQUAL:
      {
         b = left->u.const_int.value == right->u.const_int.value;

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_NOT_EQUAL:
      {
         b = left->u.const_int.value != right->u.const_int.value;

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_INTRINSIC_MIN:
      {
         op_min__const_int__const_int__const_int(&i, &left->u.const_int.value, &right->u.const_int.value);

         return glsl_dataflow_construct_const_int(line_num, i);
      }
      case DATAFLOW_INTRINSIC_MAX:
      {
         op_max__const_int__const_int__const_int(&i, &left->u.const_int.value, &right->u.const_int.value);

         return glsl_dataflow_construct_const_int(line_num, i);
      }
#ifdef __VIDEOCORE4__
      case DATAFLOW_BITWISE_AND:
      case DATAFLOW_BITWISE_OR:
      case DATAFLOW_BITWISE_XOR:
      case DATAFLOW_V8MULD:
      case DATAFLOW_V8MIN:
      case DATAFLOW_V8MAX:
      case DATAFLOW_V8ADDS:
      case DATAFLOW_V8SUBS:
      case DATAFLOW_INTEGER_ADD:
         /* TODO: ? */
         break;
#endif
      default:
         UNREACHABLE();
         break;
      }
   }

   if (left->flavour == DATAFLOW_CONST_BOOL && right->flavour == DATAFLOW_CONST_BOOL) {
      // constant folding for bools

      const_bool b;

      switch (flavour) {
      case DATAFLOW_EQUAL:
      {
         b = left->u.const_bool.value == right->u.const_bool.value;

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_NOT_EQUAL:
      {
         b = left->u.const_bool.value != right->u.const_bool.value;

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_LOGICAL_AND:
      {
         b = left->u.const_bool.value & right->u.const_bool.value;

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_LOGICAL_XOR:
      {
         b = left->u.const_bool.value ^ right->u.const_bool.value;

         return b ? g_BoolTrue : g_BoolFalse;
      }
      case DATAFLOW_LOGICAL_OR:
      {
         b = left->u.const_bool.value | right->u.const_bool.value;

         return b ? g_BoolTrue : g_BoolFalse;
      }
      default:
         UNREACHABLE();
         break;
      }
   }

   if (left->flavour == DATAFLOW_UNIFORM && right->flavour == DATAFLOW_UNIFORM) {
      // uniform memory is single ported and needs to be accessed via A port

      return glsl_dataflow_construct_binary_op(line_num, flavour, left, glsl_dataflow_construct_unary_op(line_num, DATAFLOW_MOV, right));
   }

#ifndef __VIDEOCORE4__
   if (left->flavour == DATAFLOW_CONST_FLOAT || left->flavour == DATAFLOW_CONST_INT || left->flavour == DATAFLOW_CONST_BOOL || right->flavour == DATAFLOW_UNIFORM) {
      // operand reversal

	   switch (flavour)
	   {
         case DATAFLOW_MUL:
         case DATAFLOW_ADD:
         case DATAFLOW_LOGICAL_AND:
         case DATAFLOW_LOGICAL_XOR:
         case DATAFLOW_LOGICAL_OR:
         case DATAFLOW_INTRINSIC_MIN:
         case DATAFLOW_INTRINSIC_MAX:
         case DATAFLOW_EQUAL:
         case DATAFLOW_NOT_EQUAL:
            return glsl_dataflow_construct_binary_op(line_num, flavour, right, left);
         case DATAFLOW_SUB:
            return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_RSUB, right, left);
         case DATAFLOW_RSUB:
            return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_SUB, right, left);
         case DATAFLOW_LESS_THAN:
            return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_GREATER_THAN_EQUAL, right, left);
         case DATAFLOW_LESS_THAN_EQUAL:
            return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_GREATER_THAN, right, left);
         case DATAFLOW_GREATER_THAN:
            return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LESS_THAN_EQUAL, right, left);
         case DATAFLOW_GREATER_THAN_EQUAL:
            return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LESS_THAN, right, left);
         default:
			   UNREACHABLE();
            break;
	   }
   }
#endif

#ifndef DATAFLOW_DONT_OPTIMISE_CONST_ZERO
   if (flavour == DATAFLOW_MUL && left->flavour == DATAFLOW_CONST_FLOAT) {
      if (left->u.const_float.value == CONST_FLOAT_ZERO)
         return left;
      if (left->u.const_float.value == CONST_FLOAT_ONE)
         return right;
   }

   if (flavour == DATAFLOW_MUL && right->flavour == DATAFLOW_CONST_FLOAT) {
      if (right->u.const_float.value == CONST_FLOAT_ZERO)
         return right;
      if (right->u.const_float.value == CONST_FLOAT_ONE)
         return left;
   }

   if (flavour == DATAFLOW_ADD && left->flavour == DATAFLOW_CONST_FLOAT) {
      if (left->u.const_float.value == CONST_FLOAT_ZERO)
         return right;
   }

   if (flavour == DATAFLOW_ADD && right->flavour == DATAFLOW_CONST_FLOAT) {
      if (right->u.const_float.value == CONST_FLOAT_ZERO)
         return left;
   }
#endif

   if (flavour == DATAFLOW_LOGICAL_AND && right->flavour == DATAFLOW_CONST_BOOL) {
      if (right->u.const_bool.value)
         return left;
      else
         return right;
   }

   if (flavour == DATAFLOW_LOGICAL_OR && right->flavour == DATAFLOW_CONST_BOOL) {
      if (right->u.const_bool.value)
         return right;
      else
         return left;
   }

#ifdef __VIDEOCORE4__
   if (flavour == DATAFLOW_LOGICAL_AND && left->flavour == DATAFLOW_CONST_BOOL) {
      if (left->u.const_bool.value)
         return right;
      else
         return left;
   }

   if (flavour == DATAFLOW_LOGICAL_OR && left->flavour == DATAFLOW_CONST_BOOL) {
      if (left->u.const_bool.value)
         return left;
      else
         return right;
   }

   if (flavour == DATAFLOW_V8MULD && right->flavour == DATAFLOW_CONST_INT) {
      if (right->u.const_int.value == 0)
         return right;
      else if (right->u.const_int.value == (int)0xffffffff)
         return left;
   }
#endif

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;

#ifdef __VIDEOCORE4__

   /* Boolean representation stuff */

   switch (flavour)
   {
      case DATAFLOW_LOGICAL_AND:
      case DATAFLOW_LOGICAL_XOR:
      case DATAFLOW_LOGICAL_OR:
      {
         uint32_t left_rep, right_rep, rep = 0;
         bool l1, r1;
         bool reverse = false;

         left = boolify(left, &left_rep);
         right = boolify(right, &right_rep);

         vcos_assert((left_rep == BOOL_REP_BOOL || left_rep == BOOL_REP_BOOL_N) && (right_rep == BOOL_REP_BOOL || right_rep == BOOL_REP_BOOL_N));
         l1 = left_rep == BOOL_REP_BOOL;
         r1 = right_rep == BOOL_REP_BOOL;
         if (flavour == DATAFLOW_LOGICAL_AND &&  l1 &&  r1) {
            flavour = DATAFLOW_LOGICAL_AND; rep = BOOL_REP_BOOL;                  // and
         } else if (flavour == DATAFLOW_LOGICAL_AND &&  l1 && !r1) {
            flavour = DATAFLOW_LOGICAL_SHR; rep = BOOL_REP_BOOL;                  // shr
         } else if (flavour == DATAFLOW_LOGICAL_AND && !l1 &&  r1) {
            flavour = DATAFLOW_LOGICAL_SHR; rep = BOOL_REP_BOOL; reverse = true;// shr
         } else if (flavour == DATAFLOW_LOGICAL_AND && !l1 && !r1) {
            flavour = DATAFLOW_LOGICAL_OR;  rep = BOOL_REP_BOOL_N;                // nor
         } else if (flavour == DATAFLOW_LOGICAL_XOR &&  l1 &&  r1) {
            flavour = DATAFLOW_LOGICAL_XOR; rep = BOOL_REP_BOOL;                  // xor
         } else if (flavour == DATAFLOW_LOGICAL_XOR &&  l1 && !r1) {
            flavour = DATAFLOW_LOGICAL_XOR; rep = BOOL_REP_BOOL_N;                // nxor
         } else if (flavour == DATAFLOW_LOGICAL_XOR && !l1 &&  r1) {
            flavour = DATAFLOW_LOGICAL_XOR; rep = BOOL_REP_BOOL_N;                // nxor
         } else if (flavour == DATAFLOW_LOGICAL_XOR && !l1 && !r1) {
            flavour = DATAFLOW_LOGICAL_XOR; rep = BOOL_REP_BOOL;                  // xor
         } else if (flavour == DATAFLOW_LOGICAL_OR  &&  l1 &&  r1) {
            flavour = DATAFLOW_LOGICAL_OR;  rep = BOOL_REP_BOOL;                  // or
         } else if (flavour == DATAFLOW_LOGICAL_OR &&  l1 && !r1) {
            flavour = DATAFLOW_LOGICAL_SHR; rep = BOOL_REP_BOOL_N; reverse = true;// nshr
         } else if (flavour == DATAFLOW_LOGICAL_OR && !l1 &&  r1) {
            flavour = DATAFLOW_LOGICAL_SHR; rep = BOOL_REP_BOOL_N;                // nshr
         } else if (flavour == DATAFLOW_LOGICAL_OR && !l1 && !r1) {
            flavour = DATAFLOW_LOGICAL_AND; rep = BOOL_REP_BOOL_N;                // nand
         } else {
            UNREACHABLE();
         }

         if (reverse)
         {
            Dataflow *temp = left;
            left = right;
            right = temp;
         }

         set_bool_rep(dataflow, rep);
         break;
      }

      case DATAFLOW_LESS_THAN:
         set_bool_rep(dataflow, BOOL_REP_NEG);
         break;
      case DATAFLOW_LESS_THAN_EQUAL:
         set_bool_rep(dataflow, BOOL_REP_NEG_N);
         break;
      case DATAFLOW_GREATER_THAN:
         set_bool_rep(dataflow, BOOL_REP_NEG);
         break;
      case DATAFLOW_GREATER_THAN_EQUAL:
         set_bool_rep(dataflow, BOOL_REP_NEG_N);
         break;
      case DATAFLOW_EQUAL:
         if (left->type_index == PRIM_BOOL && right->type_index == PRIM_BOOL)
            return glsl_dataflow_construct_unary_op(line_num, DATAFLOW_LOGICAL_NOT,
               glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LOGICAL_XOR, left, right));
         set_bool_rep(dataflow, BOOL_REP_ZERO);
         break;
      case DATAFLOW_NOT_EQUAL:
         if (left->type_index == PRIM_BOOL && right->type_index == PRIM_BOOL)
            return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LOGICAL_XOR, left, right);
         set_bool_rep(dataflow, BOOL_REP_ZERO_N);
         break;
      default:
         break;
   }
#endif

	switch (flavour)
	{
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
#endif
			dataflow->flavour = flavour;
			break;

		default:
			UNREACHABLE();
			return NULL;
	}

#ifndef __VIDEOCORE4__
	vcos_assert(left->type_index == right->type_index);
#endif
	switch (flavour)
	{
      case DATAFLOW_MUL:
      case DATAFLOW_ADD:
      case DATAFLOW_SUB:
      case DATAFLOW_RSUB:
      case DATAFLOW_INTRINSIC_MIN:
      case DATAFLOW_INTRINSIC_MAX:
         dataflow->type_index = left->type_index;
         break;
      case DATAFLOW_LESS_THAN:
      case DATAFLOW_LESS_THAN_EQUAL:
      case DATAFLOW_GREATER_THAN:
      case DATAFLOW_GREATER_THAN_EQUAL:
      case DATAFLOW_EQUAL:
      case DATAFLOW_NOT_EQUAL:
      case DATAFLOW_LOGICAL_AND:
      case DATAFLOW_LOGICAL_XOR:
      case DATAFLOW_LOGICAL_OR:
#ifdef __VIDEOCORE4__
      case DATAFLOW_LOGICAL_SHR:
      case DATAFLOW_SHIFT_RIGHT:
#endif
         dataflow->type_index = PRIM_BOOL;
			break;

#ifdef __VIDEOCORE4__
      case DATAFLOW_BITWISE_AND:
      case DATAFLOW_BITWISE_OR:
      case DATAFLOW_BITWISE_XOR:
      case DATAFLOW_V8MULD:
      case DATAFLOW_V8MIN:
      case DATAFLOW_V8MAX:
      case DATAFLOW_V8ADDS:
      case DATAFLOW_V8SUBS:
      case DATAFLOW_INTEGER_ADD:
         dataflow->type_index = PRIM_INT;
         break;
#endif
		default:
			UNREACHABLE();
			return NULL;
	}

	glsl_dataflow_clear_chains(dataflow);

	// Add this node to the lists of dependents of the nodes it depends on.
	glsl_dataflow_add_dependent(left, dataflow);
	glsl_dataflow_add_dependent(right, dataflow);

	dataflow->u.binary_op.left = left;
	dataflow->u.binary_op.right = right;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 2);

	return dataflow;
}

#if 0
static bool is_complement(Dataflow *c1, Dataflow *c2)
{
   switch (c1->flavour) {
   case DATAFLOW_LESS_THAN:
      return c2->flavour == DATAFLOW_GREATER_THAN_EQUAL && c2->u.binary_op.left == c1->u.binary_op.left  && c2->u.binary_op.right == c1->u.binary_op.right ||
             c2->flavour == DATAFLOW_LESS_THAN_EQUAL    && c2->u.binary_op.left == c1->u.binary_op.right && c2->u.binary_op.right == c1->u.binary_op.left;
   case DATAFLOW_LESS_THAN_EQUAL:
      return c2->flavour == DATAFLOW_GREATER_THAN && c2->u.binary_op.left == c1->u.binary_op.left  && c2->u.binary_op.right == c1->u.binary_op.right ||
             c2->flavour == DATAFLOW_LESS_THAN    && c2->u.binary_op.left == c1->u.binary_op.right && c2->u.binary_op.right == c1->u.binary_op.left;
   case DATAFLOW_GREATER_THAN:
      return c2->flavour == DATAFLOW_LESS_THAN_EQUAL    && c2->u.binary_op.left == c1->u.binary_op.left  && c2->u.binary_op.right == c1->u.binary_op.right ||
             c2->flavour == DATAFLOW_GREATER_THAN_EQUAL && c2->u.binary_op.left == c1->u.binary_op.right && c2->u.binary_op.right == c1->u.binary_op.left;
   case DATAFLOW_GREATER_THAN_EQUAL:
      return c2->flavour == DATAFLOW_LESS_THAN    && c2->u.binary_op.left == c1->u.binary_op.left  && c2->u.binary_op.right == c1->u.binary_op.right ||
             c2->flavour == DATAFLOW_GREATER_THAN && c2->u.binary_op.left == c1->u.binary_op.right && c2->u.binary_op.right == c1->u.binary_op.left;
   case DATAFLOW_EQUAL:
      return c2->flavour == DATAFLOW_NOT_EQUAL && (c2->u.binary_op.left == c1->u.binary_op.left  && c2->u.binary_op.right == c1->u.binary_op.right ||
                                                   c2->u.binary_op.left == c1->u.binary_op.right && c2->u.binary_op.right == c1->u.binary_op.left);
   case DATAFLOW_NOT_EQUAL:
      return c2->flavour == DATAFLOW_EQUAL && (c2->u.binary_op.left == c1->u.binary_op.left  && c2->u.binary_op.right == c1->u.binary_op.right ||
                                               c2->u.binary_op.left == c1->u.binary_op.right && c2->u.binary_op.right == c1->u.binary_op.left);
   }

   return c1->flavour == DATAFLOW_LOGICAL_NOT && c1->u.unary_op.operand == c2 ||
          c2->flavour == DATAFLOW_LOGICAL_NOT && c2->u.unary_op.operand == c1;
}
#endif

Dataflow* glsl_dataflow_construct_cond_op(int line_num, Dataflow* cond, Dataflow* true_value, Dataflow* false_value)
{
	Dataflow* dataflow;

   if (cond->flavour == DATAFLOW_CONST_BOOL) {
      if (cond->u.const_bool.value)
         return true_value;
      else
         return false_value;
   }

   if (true_value == false_value)
      return true_value;

   if (true_value->flavour == DATAFLOW_CONST_BOOL && false_value->flavour == DATAFLOW_CONST_BOOL) {
      // TODO: when CSE implemented, remove the following
      if (true_value->u.const_bool.value == false_value->u.const_bool.value)
         return true_value;

      if (true_value->u.const_bool.value) {
         // We're a mov
         vcos_assert(!false_value->u.const_bool.value);

         return cond;
      } else {
         // We're a not
         vcos_assert(false_value->u.const_bool.value);

         return glsl_dataflow_construct_unary_op(line_num, DATAFLOW_LOGICAL_NOT, cond);
      }
   }

#ifdef __VIDEOCORE4__
   if (true_value->type_index == PRIM_BOOL && false_value->type_index == PRIM_BOOL &&
      glsl_dataflow_get_bool_rep(true_value) != glsl_dataflow_get_bool_rep(false_value))
   {
      /*
         Constructing a conditional may not work as inputs have different bool reps.
         So build out of ANDs and ORs instead.
      */
      return glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LOGICAL_OR,
         glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LOGICAL_AND,
            cond,
            true_value),
         glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LOGICAL_AND,
            glsl_dataflow_construct_unary_op(line_num, DATAFLOW_LOGICAL_NOT, cond),
            false_value));
   }
#endif

   if (true_value == cond && false_value->flavour == DATAFLOW_CONST_BOOL && !false_value->u.const_bool.value)
      return cond;
   if (false_value == cond && true_value->flavour == DATAFLOW_CONST_BOOL && true_value->u.const_bool.value)
      return cond;

#if 0
   if (true_value->flavour == DATAFLOW_CONDITIONAL && is_complement(cond, true_value->u.cond_op.cond))
      return glsl_dataflow_construct_cond_op(line_num, cond, true_value->u.cond_op.false_value, false_value);

   if (false_value->flavour == DATAFLOW_CONDITIONAL && is_complement(cond, false_value->u.cond_op.cond))
      return glsl_dataflow_construct_cond_op(line_num, cond, true_value, false_value->u.cond_op.true_value);
#endif
   if (cond->flavour == DATAFLOW_LOGICAL_NOT)
      return glsl_dataflow_construct_cond_op(line_num, cond->u.unary_op.operand, false_value, true_value);

   /*
      TODO: consider replacing this with proper tracking of guarding predicates in back end
   */

   if (true_value->flavour == DATAFLOW_CONDITIONAL && true_value->u.cond_op.cond == cond)
      return glsl_dataflow_construct_cond_op(line_num, cond, true_value->u.cond_op.true_value, false_value);
   if (false_value->flavour == DATAFLOW_CONDITIONAL && false_value->u.cond_op.cond == cond)
      return glsl_dataflow_construct_cond_op(line_num, cond, true_value, false_value->u.cond_op.false_value);

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(DATAFLOW_CONDITIONAL);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;

	dataflow->flavour = DATAFLOW_CONDITIONAL;
	vcos_assert(PRIM_BOOL == cond->type_index);
	vcos_assert(true_value->type_index == false_value->type_index);
	dataflow->type_index = true_value->type_index;
	glsl_dataflow_clear_chains(dataflow);

#ifdef __VIDEOCORE4__
   if (true_value->type_index == PRIM_BOOL)
   {
      set_bool_rep(dataflow, glsl_dataflow_get_bool_rep(true_value));
   }
#endif

	// Add this node to the lists of dependents of the nodes it depends on.
	glsl_dataflow_add_dependent(cond, dataflow);
	glsl_dataflow_add_dependent(true_value, dataflow);
	glsl_dataflow_add_dependent(false_value, dataflow);

	dataflow->u.cond_op.cond = cond;
	dataflow->u.cond_op.true_value = true_value;
	dataflow->u.cond_op.false_value = false_value;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 3);

	return dataflow;
}

Dataflow* glsl_dataflow_construct_texture_lookup_set(int line_num, DataflowFlavour flavour, Dataflow* sampler, Dataflow* param, Dataflow* depends_on0, Dataflow* depends_on1, Dataflow* depends_on2)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;

	switch (flavour)
	{
		case DATAFLOW_TEX_SET_COORD_S:
		case DATAFLOW_TEX_SET_COORD_T:
		case DATAFLOW_TEX_SET_COORD_R:
		case DATAFLOW_TEX_SET_BIAS:
		case DATAFLOW_TEX_SET_LOD:
#if defined(__VIDEOCORE4__) && !defined(__BCM2708A0__)
      case DATAFLOW_TEX_SET_DIRECT:
#endif
			dataflow->flavour = flavour;
			break;

		default:
			UNREACHABLE();
			return NULL;
	}

#if defined(__VIDEOCORE4__) && !defined(__BCM2708A0__)
   vcos_assert((PRIM_FLOAT == param->type_index && flavour != DATAFLOW_TEX_SET_DIRECT) ||
      (PRIM_INT == param->type_index && flavour == DATAFLOW_TEX_SET_DIRECT));
#else
	vcos_assert(PRIM_FLOAT == param->type_index);
   vcos_assert(sampler != NULL);
#endif
	dataflow->type_index = PRIM_VOID;

	glsl_dataflow_clear_chains(dataflow);

	// Add this node to the lists of dependents of the nodes it depends on.
   if (sampler)
	   glsl_dataflow_add_dependent(sampler, dataflow);
	glsl_dataflow_add_dependent(param, dataflow);

   if (depends_on0) {
   	glsl_dataflow_add_iodependent(depends_on0, dataflow);
      glsl_dataflow_add_iodependency(dataflow, depends_on0);
   }
   if (depends_on1) {
   	glsl_dataflow_add_iodependent(depends_on1, dataflow);
      glsl_dataflow_add_iodependency(dataflow, depends_on1);
   }
   if (depends_on2) {
	   glsl_dataflow_add_iodependent(depends_on2, dataflow);
      glsl_dataflow_add_iodependency(dataflow, depends_on2);
   }

	dataflow->u.texture_lookup_set.sampler = sampler;
	dataflow->u.texture_lookup_set.param = param;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 2);

	return dataflow;
}

Dataflow* glsl_dataflow_construct_fragment_get(int line_num, DataflowFlavour flavour)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;

	switch (flavour)
	{
#ifndef __VIDEOCORE4__
		case DATAFLOW_FRAG_GET_X:
		case DATAFLOW_FRAG_GET_Y:
#endif
		case DATAFLOW_FRAG_GET_Z:
		case DATAFLOW_FRAG_GET_W:
		case DATAFLOW_FRAG_GET_PC_X:
		case DATAFLOW_FRAG_GET_PC_Y:
#ifdef __VIDEOCORE4__
      case DATAFLOW_VARYING_C:
#endif
			dataflow->flavour = flavour;

      	dataflow->type_index = PRIM_FLOAT;
			break;

      case DATAFLOW_FRAG_GET_FF:
			dataflow->flavour = flavour;

      	dataflow->type_index = PRIM_BOOL;
#ifdef __VIDEOCORE4__
         set_bool_rep(dataflow, BOOL_REP_BOOL_N);
#endif
         break;

#ifdef __VIDEOCORE4__
		case DATAFLOW_FRAG_GET_X:
		case DATAFLOW_FRAG_GET_Y:
      case DATAFLOW_FRAG_GET_COL:
         dataflow->flavour = flavour;
         dataflow->type_index = PRIM_INT;
         break;
#endif

		default:
			UNREACHABLE();
			return NULL;
	}

	glsl_dataflow_clear_chains(dataflow);

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 0);

	return dataflow;
}

Dataflow* glsl_dataflow_construct_fragment_set(int line_num, DataflowFlavour flavour, Dataflow* param, Dataflow *discard)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;

	switch (flavour)
	{
		case DATAFLOW_FRAG_SET_R:
		case DATAFLOW_FRAG_SET_G:
		case DATAFLOW_FRAG_SET_B:
		case DATAFLOW_FRAG_SET_A:
			dataflow->flavour = flavour;
			break;

		default:
			UNREACHABLE();
			return NULL;
	}

	vcos_assert(PRIM_FLOAT == param->type_index);
   vcos_assert(PRIM_BOOL == discard->type_index);
	dataflow->type_index = PRIM_VOID;

	glsl_dataflow_clear_chains(dataflow);

	// Add this node to the lists of dependents of the nodes it depends on.
	glsl_dataflow_add_dependent(param, dataflow);
   glsl_dataflow_add_dependent(discard, dataflow);

	dataflow->u.fragment_set.param = param;
   dataflow->u.fragment_set.discard = discard;
   dataflow->u.fragment_set.redundant = false;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 2);

	return dataflow;
}

Dataflow* glsl_dataflow_construct_texture_lookup_get(int line_num, DataflowFlavour flavour, Dataflow* sampler, Dataflow* depends_on0, Dataflow* depends_on1, Dataflow* depends_on2)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;

	switch (flavour)
	{
		case DATAFLOW_TEX_GET_CMP_R:
		case DATAFLOW_TEX_GET_CMP_G:
		case DATAFLOW_TEX_GET_CMP_B:
		case DATAFLOW_TEX_GET_CMP_A:
			dataflow->flavour = flavour;
			break;

		default:
			UNREACHABLE();
			return NULL;
	}

	dataflow->type_index = PRIM_FLOAT;

	glsl_dataflow_clear_chains(dataflow);

#if !defined(__VIDEOCORE4__) || defined(__BCM2708A0__)
   vcos_assert(sampler != NULL);
#endif

	// Add this node to the lists of dependents of the nodes it depends on.
   if (sampler)
	   glsl_dataflow_add_dependent(sampler, dataflow);

   if (depends_on0) {
   	glsl_dataflow_add_iodependent(depends_on0, dataflow);
      glsl_dataflow_add_iodependency(dataflow, depends_on0);
   }
   if (depends_on1) {
   	glsl_dataflow_add_iodependent(depends_on1, dataflow);
      glsl_dataflow_add_iodependency(dataflow, depends_on1);
   }
   if (depends_on2) {
	   glsl_dataflow_add_iodependent(depends_on2, dataflow);
      glsl_dataflow_add_iodependency(dataflow, depends_on2);
   }

	dataflow->u.texture_lookup_get.sampler = sampler;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 1);

	return dataflow;
}

#ifdef __VIDEOCORE4__
Dataflow* glsl_dataflow_construct_vertex_set(int line_num, DataflowFlavour flavour, Dataflow* param, Dataflow* prev)
{
	Dataflow* dataflow;

   vcos_assert(flavour == DATAFLOW_VERTEX_SET || flavour == DATAFLOW_VPM_READ_SETUP || flavour == DATAFLOW_VPM_WRITE_SETUP);

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;
   dataflow->flavour = flavour;

   vcos_assert(PRIM_FLOAT == param->type_index || PRIM_INT == param->type_index);    /* x,y is integer, everything else is float */
   //vcos_assert(NULL == prev || PRIM_VOID == prev->type_index);//why was this here?
	dataflow->type_index = PRIM_VOID;

	glsl_dataflow_clear_chains(dataflow);

	// Add this node to the lists of dependents of the nodes it depends on.
	glsl_dataflow_add_dependent(param, dataflow);
   if (prev)
   {
      glsl_dataflow_add_iodependent(prev, dataflow);
      glsl_dataflow_add_iodependency(dataflow, prev);
   }

   dataflow->u.vertex_set.param = param;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 1);

	return dataflow;
}

Dataflow* glsl_dataflow_construct_fragment_submit(int line_num, DataflowFlavour flavour, Dataflow* param, Dataflow *prev, Dataflow *discard)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;

	switch (flavour)
	{
      case DATAFLOW_FRAG_SUBMIT_STENCIL:
      case DATAFLOW_FRAG_SUBMIT_Z:
      case DATAFLOW_FRAG_SUBMIT_MS:
      case DATAFLOW_FRAG_SUBMIT_ALL:
      case DATAFLOW_FRAG_SUBMIT_R0:
      case DATAFLOW_FRAG_SUBMIT_R1:
      case DATAFLOW_FRAG_SUBMIT_R2:
      case DATAFLOW_FRAG_SUBMIT_R3:
      case DATAFLOW_TMU_SWAP:
			dataflow->flavour = flavour;
			break;

		default:
			UNREACHABLE();
			return NULL;
	}

   if (flavour == DATAFLOW_FRAG_SUBMIT_Z)
   {
	   vcos_assert(PRIM_FLOAT == param->type_index);
   }
   else
   {
	   vcos_assert(PRIM_INT == param->type_index);
   }
	dataflow->type_index = PRIM_VOID;

	glsl_dataflow_clear_chains(dataflow);

	// Add this node to the lists of dependents of the nodes it depends on.
	glsl_dataflow_add_dependent(param, dataflow);
   if (prev)
   {
#ifdef __VIDEOCORE4__
      glsl_dataflow_add_iodependent(prev, dataflow);
      glsl_dataflow_add_iodependency(dataflow, prev);
#else
      glsl_dataflow_add_iodependent(prev, param);
      glsl_dataflow_add_iodependency(param, prev);
#endif
   }

	dataflow->u.fragment_set.param = param;
   if (discard == NULL || (discard->flavour == DATAFLOW_CONST_BOOL && discard->u.const_bool.value == CONST_BOOL_FALSE))
      dataflow->u.fragment_set.discard = NULL;
   else
   {
      dataflow->u.fragment_set.discard = discard;
      glsl_dataflow_add_dependent(discard, dataflow);
   }

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 2);

	return dataflow;
}

Dataflow *glsl_dataflow_construct_pack(int line_num, DataflowFlavour flavour, Dataflow *operand, Dataflow *background)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;
   dataflow->flavour = flavour;

   if (DATAFLOW_PACK_COL_R == flavour || DATAFLOW_PACK_COL_G == flavour || DATAFLOW_PACK_COL_B == flavour || DATAFLOW_PACK_COL_A == flavour || DATAFLOW_PACK_FB_R == flavour || DATAFLOW_PACK_FB_B == flavour)
   {
	   vcos_assert(PRIM_FLOAT == operand->type_index); 
   }
   else if (DATAFLOW_PACK_16A == flavour || DATAFLOW_PACK_16B == flavour)
   {
      vcos_assert(PRIM_INT == operand->type_index);
   }
   else
      UNREACHABLE();

   vcos_assert(NULL == background || PRIM_INT == background->type_index);
	dataflow->type_index = PRIM_INT;

	glsl_dataflow_clear_chains(dataflow);

	// Add this node to the lists of dependents of the nodes it depends on.
   glsl_dataflow_add_dependent(operand, dataflow);
   if (NULL != background)
      glsl_dataflow_add_dependent(background, dataflow);

   dataflow->u.pack.operand = operand;
   dataflow->u.pack.background = background;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 1);

	return dataflow;
}

Dataflow* glsl_dataflow_construct_pack_col(int line_num, Dataflow* red, Dataflow *green, Dataflow *blue, Dataflow *alpha)
{
   Dataflow *dataflow = NULL;

   if( red->flavour  == DATAFLOW_UNPACK_COL_R &&
      green->flavour == DATAFLOW_UNPACK_COL_G && 
      blue->flavour == DATAFLOW_UNPACK_COL_B &&
      alpha->flavour == DATAFLOW_UNPACK_COL_A &&
      red->u.unary_op.operand == green->u.unary_op.operand &&
      red->u.unary_op.operand == blue->u.unary_op.operand &&
      red->u.unary_op.operand == alpha->u.unary_op.operand) 
   {
      //all the inputs to this pack come in the same order from a single unpack
      //so we can collapse the pack/unpack and just return the parent
      dataflow = red->u.unary_op.operand;
      dataflow->type_index = PRIM_INT;
   }
   else
   {
      //add iodependencies to force ordering
//      glsl_dataflow_add_iodependency(green, red);
//      glsl_dataflow_add_iodependent(red, green);   
//   
//      glsl_dataflow_add_iodependency(blue, green);
//      glsl_dataflow_add_iodependent(green, blue); 
//      
//      glsl_dataflow_add_iodependency(alpha, blue);
//      glsl_dataflow_add_iodependent(blue, alpha); 
      
      dataflow = glsl_dataflow_construct_pack(line_num, DATAFLOW_PACK_FB_R, red, dataflow);
      dataflow = glsl_dataflow_construct_pack(line_num, DATAFLOW_PACK_COL_G, green, dataflow);
      dataflow = glsl_dataflow_construct_pack(line_num, DATAFLOW_PACK_FB_B, blue, dataflow);
      dataflow = glsl_dataflow_construct_pack(line_num, DATAFLOW_PACK_COL_A, alpha, dataflow);
   }
   return dataflow;
}

Dataflow* glsl_dataflow_construct_pack_col_rgb(int line_num, Dataflow* red, Dataflow *green, Dataflow *blue)
{
   Dataflow *dataflow = NULL;
   
   if( red->flavour  == DATAFLOW_UNPACK_COL_R &&
      green->flavour == DATAFLOW_UNPACK_COL_G && 
      blue->flavour == DATAFLOW_UNPACK_COL_B &&
      red->u.unary_op.operand == green->u.unary_op.operand &&
      red->u.unary_op.operand == blue->u.unary_op.operand ) 
   {
      //all the inputs to this pack come in the same order from a single unpack
      //so we can collapse the pack/unpack and just return the parent
      dataflow = red->u.unary_op.operand;
      dataflow->type_index = PRIM_INT;
   }
   else
   {
      //add iodependencies to force ordering
//      glsl_dataflow_add_iodependency(green, red);
//      glsl_dataflow_add_iodependent(red, green);   
//   
//      glsl_dataflow_add_iodependency(blue, green);
//      glsl_dataflow_add_iodependent(green, blue);   
   
      dataflow = glsl_dataflow_construct_pack(line_num, DATAFLOW_PACK_FB_R, red, dataflow);
      dataflow = glsl_dataflow_construct_pack(line_num, DATAFLOW_PACK_COL_G, green, dataflow);
      dataflow = glsl_dataflow_construct_pack(line_num, DATAFLOW_PACK_FB_B, blue, dataflow);
   }

   return dataflow;
}

Dataflow* glsl_dataflow_construct_pack_int16(int line_num, Dataflow* a, Dataflow *b)
{
   Dataflow *dataflow = NULL;

   dataflow = glsl_dataflow_construct_pack(line_num, DATAFLOW_PACK_16A, a, dataflow);
   dataflow = glsl_dataflow_construct_pack(line_num, DATAFLOW_PACK_16B, b, dataflow);

   return dataflow;
}

Dataflow* glsl_dataflow_construct_varying_tree(int line_num, Dataflow* varying)
{
   Dataflow *w;
   Dataflow *c;
   Dataflow *dataflow;

   vcos_assert(varying->flavour == DATAFLOW_VARYING);

   w = glsl_dataflow_construct_fragment_get(line_num, DATAFLOW_FRAG_GET_W);
   c = glsl_dataflow_construct_fragment_get(line_num, DATAFLOW_VARYING_C);

   dataflow = glsl_dataflow_construct_binary_op(line_num, DATAFLOW_MUL, varying, w);
   dataflow = glsl_dataflow_construct_binary_op(line_num, DATAFLOW_ADD, dataflow, c);

   // We don't need this as an actual iodependency
   // Rather, it will let us find the VARYING_C node from the VARYING node.
	glsl_dataflow_add_iodependent(varying, c);
   glsl_dataflow_add_iodependency(c, varying);

   return dataflow;
}

Dataflow* glsl_dataflow_construct_varying_non_perspective_tree(int line_num, Dataflow* varying)
{
   Dataflow *w;
   Dataflow *c;
   Dataflow *dataflow;

   vcos_assert(varying->flavour == DATAFLOW_VARYING);

   w = glsl_dataflow_construct_const_float(line_num, float_to_bits(1.0f));
   c = glsl_dataflow_construct_fragment_get(line_num, DATAFLOW_VARYING_C);

   dataflow = glsl_dataflow_construct_binary_op(line_num, DATAFLOW_MUL, varying, w);
   dataflow = glsl_dataflow_construct_binary_op(line_num, DATAFLOW_ADD, dataflow, c);

   // We don't need this as an actual iodependency
   // Rather, it will let us find the VARYING_C node from the VARYING node.
	glsl_dataflow_add_iodependent(varying, c);
   glsl_dataflow_add_iodependency(c, varying);

   return dataflow;
}

Dataflow* glsl_dataflow_construct_unpack(int line_num, DataflowFlavour flavour, Dataflow* param)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;
   vcos_assert(
      flavour == DATAFLOW_UNPACK_COL_R ||
      flavour == DATAFLOW_UNPACK_COL_G ||
      flavour == DATAFLOW_UNPACK_COL_B ||
      flavour == DATAFLOW_UNPACK_COL_A ||
      flavour == DATAFLOW_UNPACK_16A ||
      flavour == DATAFLOW_UNPACK_16A_F ||
      flavour == DATAFLOW_UNPACK_16B ||
      flavour == DATAFLOW_UNPACK_16B_F ||
      flavour == DATAFLOW_UNPACK_8A ||
      flavour == DATAFLOW_UNPACK_8B ||
      flavour == DATAFLOW_UNPACK_8C ||
      flavour == DATAFLOW_UNPACK_8D ||
      flavour == DATAFLOW_UNPACK_8R ||
      flavour == DATAFLOW_UNPACK_FB_R ||
      flavour == DATAFLOW_UNPACK_FB_B);
   dataflow->flavour = flavour;

   //vcos_assert(PRIM_INT == param->type_index);      TODO: texture fetch says it's float when it's really int
	dataflow->type_index = PRIM_FLOAT;

	glsl_dataflow_clear_chains(dataflow);

	// Add this node to the lists of dependents of the nodes it depends on.
	glsl_dataflow_add_dependent(param, dataflow);

   dataflow->u.unary_op.operand = param;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 1);

	return dataflow;
}

Dataflow* glsl_dataflow_construct_unpack_placeholder(int line_num, DataflowFlavour flavour, Dataflow* param, Dataflow *sampler)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;
   vcos_assert(
      flavour == DATAFLOW_UNPACK_PLACEHOLDER_R ||
      flavour == DATAFLOW_UNPACK_PLACEHOLDER_B);
   dataflow->flavour = flavour;

   //vcos_assert(PRIM_INT == param->type_index);      TODO: texture fetch says it's float when it's really int
	dataflow->type_index = PRIM_FLOAT;

	glsl_dataflow_clear_chains(dataflow);

	// Add this node to the lists of dependents of the nodes it depends on.
	glsl_dataflow_add_dependent(param, dataflow);

   dataflow->u.binary_op.left = param;
   dataflow->u.binary_op.right = sampler;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 1);

	return dataflow;
}

#ifdef __BCM2708A0__
Dataflow* glsl_dataflow_construct_indexed_uniform_sampler(int line_num, Dataflow* uniform, uint32_t size)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(DATAFLOW_INDEXED_UNIFORM_SAMPLER);

   vcos_assert(DATAFLOW_UNIFORM == uniform->flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;
   dataflow->flavour = DATAFLOW_INDEXED_UNIFORM_SAMPLER;

	dataflow->type_index = PRIM_VOID;

	glsl_dataflow_clear_chains(dataflow);

	// Add this node to the lists of dependents of the nodes it depends on.
	glsl_dataflow_add_dependent(uniform, dataflow);

   dataflow->u.indexed_uniform_sampler.uniform = uniform;
   dataflow->u.indexed_uniform_sampler.size = size;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 1);

	return dataflow;
}
#else
Dataflow* glsl_dataflow_construct_uniform_address(int line_num, Dataflow* uniform, uint32_t size)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(DATAFLOW_UNIFORM_ADDRESS);

   vcos_assert(DATAFLOW_UNIFORM == uniform->flavour);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;
   dataflow->flavour = DATAFLOW_UNIFORM_ADDRESS;

   dataflow->type_index = PRIM_INT;

	glsl_dataflow_clear_chains(dataflow);

	// Add this node to the lists of dependents of the nodes it depends on.
	glsl_dataflow_add_dependent(uniform, dataflow);

   dataflow->u.indexed_uniform_sampler.uniform = uniform;
   dataflow->u.indexed_uniform_sampler.size = size;

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 1);

	return dataflow;
}
#endif

Dataflow* glsl_dataflow_construct_scoreboard_wait(int line_num, Dataflow* operand)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(DATAFLOW_SCOREBOARD_WAIT);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;
   dataflow->flavour = DATAFLOW_SCOREBOARD_WAIT;
   dataflow->type_index = PRIM_VOID;

	glsl_dataflow_clear_chains(dataflow);

	// Add this node to the lists of dependents of the nodes it depends on.

   if (operand != NULL)
   {
      glsl_dataflow_add_iodependent(operand, dataflow);
      glsl_dataflow_add_iodependency(dataflow, operand);
   }

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 0);

	return dataflow;
}

/*
   Convert boolean value into a form which can be handled by AND/OR/XOR.

   First strip off any "not" node (this doesn't correspond to any actual instruction, it
   just flips our sense of which value is true).

   If it is in BOOL_REP_BOOL form then leave it as it is.

   If it is in BOOL_REP_NEG form then shift right by 31 (to be left with msb)

   If it is in BOOL_REP_ZERO form then construct a conditional node, so that 1 gets moved
   over the top of the value if it is nonzero.
*/

static Dataflow *boolify(Dataflow *dataflow, uint32_t *rep_out)
{
   uint32_t rep;
   uint32_t line_num = dataflow->line_num;

   vcos_assert(PRIM_BOOL == dataflow->type_index);

   rep = glsl_dataflow_get_bool_rep(dataflow);

   if (DATAFLOW_LOGICAL_NOT == dataflow->flavour)
   {
      dataflow = dataflow->u.unary_op.operand;
   }

   switch (rep)
   {
   case BOOL_REP_BOOL:
   case BOOL_REP_BOOL_N:
      *rep_out = rep;
      return dataflow;

   case BOOL_REP_NEG:
   case BOOL_REP_NEG_N:
   {
      Dataflow *const31;

      /* TODO: it's annoying that we have to do it this way (to stop ourselves accidentally converting 31 back to float) */
      const31 = glsl_dataflow_construct_const_float(line_num, 31);
      const31->type_index = PRIM_INT;

      dataflow = glsl_dataflow_construct_binary_op(
         line_num,
         DATAFLOW_SHIFT_RIGHT,
         dataflow,
         const31);
      rep = (rep == BOOL_REP_NEG) ? BOOL_REP_BOOL : BOOL_REP_BOOL_N;
      set_bool_rep(dataflow, rep);
      *rep_out = rep;
      return dataflow;
   }
   case BOOL_REP_ZERO:
   case BOOL_REP_ZERO_N:
      if (glsl_dataflow_get_bool_rep(dataflow) == BOOL_REP_ZERO)
      {
         /* TODO: the hackiness is to prevent glsl_dataflow_construct_cond_op from flattening */
         Dataflow *cbool = glsl_dataflow_construct_const_float(line_num, 1);
         cbool->type_index = PRIM_BOOL;
         set_bool_rep(cbool, BOOL_REP_ZERO);
         dataflow = glsl_dataflow_construct_cond_op(
            line_num,
            dataflow,
            dataflow,
            cbool);
      }
      else
      {
         Dataflow *cbool = glsl_dataflow_construct_const_float(line_num, 1);
         cbool->type_index = PRIM_BOOL;
         set_bool_rep(cbool, BOOL_REP_ZERO_N);
         dataflow = glsl_dataflow_construct_cond_op(
            line_num,
            dataflow,
            cbool,
            dataflow);
      }

      rep = (rep == BOOL_REP_ZERO) ? BOOL_REP_BOOL_N : BOOL_REP_BOOL;
      set_bool_rep(dataflow, rep);
      *rep_out = rep;
      return dataflow;

   default:
      UNREACHABLE();
      return NULL;
   }
}

uint32_t glsl_dataflow_get_bool_rep(Dataflow *dataflow)
{
   vcos_assert(dataflow->etime != ~0);
   return dataflow->etime;
}

static void set_bool_rep(Dataflow *dataflow, uint32_t bool_rep)
{
   dataflow->etime = bool_rep;
}


#endif   //__VIDEOCORE4__

//
// Dataflow type conversion.
//

Dataflow* glsl_dataflow_convert_type(Dataflow* input, PrimitiveTypeIndex type_index)
{
   switch (input->type_index)
   {
      case PRIM_BOOL:
         switch (type_index)
         {
            case PRIM_BOOL:
               return input;

            case PRIM_INT:
#ifdef __VIDEOCORE4__
               return glsl_dataflow_construct_cond_op(input->line_num,
                  input,
                  glsl_dataflow_construct_const_int(input->line_num, 1),
                  glsl_dataflow_construct_const_int(input->line_num, 0));
#else
               return glsl_dataflow_construct_unary_op(input->line_num, DATAFLOW_INTRINSIC_BOOLTOINT, input);
#endif

            case PRIM_FLOAT:
#ifdef __VIDEOCORE4__
               return glsl_dataflow_construct_cond_op(input->line_num,
                  input,
                  glsl_dataflow_construct_const_float(input->line_num, CONST_FLOAT_ONE),
                  glsl_dataflow_construct_const_float(input->line_num, CONST_FLOAT_ZERO));
#else
               return glsl_dataflow_construct_unary_op(input->line_num, DATAFLOW_INTRINSIC_BOOLTOFLOAT, input);
#endif

            default:
               UNREACHABLE();
               return NULL;
         }

      case PRIM_INT:
         switch (type_index)
         {
            case PRIM_BOOL:
               return glsl_dataflow_construct_unary_op(input->line_num, DATAFLOW_INTRINSIC_INTTOBOOL, input);

            case PRIM_INT:
               return input;

            case PRIM_FLOAT:
               return glsl_dataflow_construct_unary_op(input->line_num, DATAFLOW_INTRINSIC_INTTOFLOAT, input);

            default:
               UNREACHABLE();
               return NULL;
         }

      case PRIM_FLOAT:
         switch (type_index)
         {
            case PRIM_BOOL:
#ifdef __VIDEOCORE4__
               return glsl_dataflow_construct_binary_op(input->line_num, DATAFLOW_NOT_EQUAL, input, glsl_dataflow_construct_const_float(input->line_num, CONST_FLOAT_ZERO));
#else
               return glsl_dataflow_construct_unary_op(input->line_num, DATAFLOW_INTRINSIC_FLOATTOBOOL, input);
#endif

            case PRIM_INT:
               // "When constructors are used to convert a float to an int,
               // the fractional part of the floating-point value is dropped."
               // -- I take this to mean truncation.
               return glsl_dataflow_construct_unary_op(input->line_num, DATAFLOW_COERCE_TOFLOAT, glsl_dataflow_construct_unary_op(input->line_num, DATAFLOW_INTRINSIC_FLOATTOINT_TRUNC, input));

            case PRIM_FLOAT:
               return input;

            default:
               UNREACHABLE();
               return NULL;
         }

      default:
         UNREACHABLE();
         return NULL;
   }
}


//
// Dataflow calculation helper functions.
//

// Populates and array of Dataflow** with count members starting at offset.
// This represents the lvalue of the expr and should be updated if this expr is used as an lvalue.

static void expr_evaluate_lvalue(Expr* expr, Dataflow ***result, int offset, int count)
{
   STACK_CHECK();

	switch (expr->flavour)
	{
		case EXPR_VALUE:
		case EXPR_FUNCTION_CALL:
		case EXPR_PRIM_CONSTRUCTOR_CALL:
		case EXPR_TYPE_CONSTRUCTOR_CALL:
		case EXPR_POST_INC:
		case EXPR_POST_DEC:
		case EXPR_PRE_INC:
		case EXPR_PRE_DEC:
		case EXPR_ARITH_NEGATE:
		case EXPR_LOGICAL_NOT:
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
		case EXPR_CONDITIONAL:
		case EXPR_ASSIGN:
		case EXPR_SEQUENCE:
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
         // These are not lvalues.
			UNREACHABLE();
         return;

		case EXPR_INSTANCE:
         {
            int i;

            switch (expr->u.instance.symbol->flavour) {
            case SYMBOL_VAR_INSTANCE:
            case SYMBOL_PARAM_INSTANCE:
               if (!strcmp(expr->u.instance.symbol->name, "gl_FragColor"))
                  g_AssignedFragColor = true;
               if (!strcmp(expr->u.instance.symbol->name, "gl_FragData"))
                  g_AssignedFragData = true;

               for (i = 0; i < count; i++)
                  result[i] = expr->u.instance.symbol->u.var_instance.scalar_values + offset + i;
               break;

            default:
               UNREACHABLE();
               break;
            }

            break;
         }

		case EXPR_SUBSCRIPT:
         {
            const_int index;
            int member_scalar_count;

            // All subscripts used as lvalues must be indexed by constants.
            vcos_assert(expr->u.subscript.subscript->constant_index_expression);
            if (expr->u.subscript.subscript->compile_time_value)
            {
               index = *(const_int*)expr->u.subscript.subscript->compile_time_value;
            }
            else
            {
               Dataflow *dataflow;
               vcos_assert(expr->u.subscript.subscript->type->scalar_count == 1);
               expr_calculate_dataflow(&dataflow, expr->u.subscript.subscript);
               vcos_assert(dataflow->flavour == DATAFLOW_CONST_INT);
               index = dataflow->u.const_int.value;
            }

            switch (expr->u.subscript.aggregate->type->flavour)
            {
               case SYMBOL_PRIMITIVE_TYPE:
                  member_scalar_count = primitiveTypeSubscriptTypes[expr->u.subscript.aggregate->type->u.primitive_type.index]->scalar_count;
                  break;

               case SYMBOL_ARRAY_TYPE:
                  member_scalar_count = expr->u.subscript.aggregate->type->u.array_type.member_type->scalar_count;
                  break;

               default:
                  member_scalar_count = 0;
                  UNREACHABLE();
                  break;
            }

            // Perform bounds check.

            if (index * member_scalar_count >= expr->u.subscript.aggregate->type->scalar_count)
            {
               // Indexing with an integral constant expression greater than declared size.
               glsl_compile_error(ERROR_SEMANTIC, 20, expr->line_num, NULL);
               return;
            }

            expr_evaluate_lvalue(expr->u.subscript.aggregate, result, offset + index * member_scalar_count, count);

            break;
         }

		case EXPR_FIELD_SELECTOR_STRUCT:
         {
            int i;
            int field_no = expr->u.field_selector_struct.field_no;
            int scalar_count_offset;

            vcos_assert(SYMBOL_STRUCT_TYPE == expr->u.field_selector_struct.aggregate->type->flavour);

            for (i = 0, scalar_count_offset = 0; i < field_no; i++)
            {
               scalar_count_offset += expr->u.field_selector_struct.aggregate->type->u.struct_type.member_types[i]->scalar_count;
            }

            expr_evaluate_lvalue(expr->u.subscript.aggregate, result, offset + scalar_count_offset, count);

            break;
         }

		case EXPR_FIELD_SELECTOR_SWIZZLE:
         {
            Dataflow **aggregate_scalar_values[4];
            int i;

            vcos_assert(expr->u.field_selector_swizzle.aggregate->type->scalar_count <= 4);

            expr_evaluate_lvalue(expr->u.field_selector_swizzle.aggregate, aggregate_scalar_values, 0, expr->u.field_selector_swizzle.aggregate->type->scalar_count);

            for (i = 0; i < count; i++) {
               vcos_assert(expr->u.field_selector_swizzle.swizzle_slots[i + offset] != SWIZZLE_SLOT_UNUSED);

               result[i] = aggregate_scalar_values[expr->u.field_selector_swizzle.swizzle_slots[i + offset]];
            }

            break;
         }

		default:
         // Nothing else.
			UNREACHABLE();
         break;
	}
}

// Writes dataflow graph pointers for compile_time_value to scalar_values.
// scalar_values is array of Dataflow* with expr->type->scalar_count members.
static void expr_calculate_dataflow_compile_time_value(Dataflow** scalar_values, int line_num, void* compile_time_value, SymbolType* type)
{
   STACK_CHECK();

   switch (type->flavour)
   {
      case SYMBOL_PRIMITIVE_TYPE:
         {
            int i;
            switch (primitiveTypeFlags[type->u.primitive_type.index] & (PRIM_BOOL_TYPE | PRIM_INT_TYPE | PRIM_FLOAT_TYPE))
            {
               case PRIM_BOOL_TYPE:
                  for (i = 0; i < type->scalar_count; i++)
                  {
                     scalar_values[i] = ((const_bool*)compile_time_value)[i] ? g_BoolTrue : g_BoolFalse;
                  }
                  return;

               case PRIM_INT_TYPE:
                  for (i = 0; i < type->scalar_count; i++)
                  {
                     scalar_values[i] = glsl_dataflow_construct_const_int(line_num, ((const_int*)compile_time_value)[i]);
                  }
                  return;

               case PRIM_FLOAT_TYPE:
                  for (i = 0; i < type->scalar_count; i++)
                  {
                     scalar_values[i] = glsl_dataflow_construct_const_float(line_num, ((const_float*)compile_time_value)[i]);
                  }
                  return;

               default:
                  UNREACHABLE();
                  return;
            }
         }

      case SYMBOL_STRUCT_TYPE:
         {
            int i;

            for (i = 0; i < type->u.struct_type.member_count; i++)
            {
               SymbolType* member_type = type->u.struct_type.member_types[i];

               expr_calculate_dataflow_compile_time_value(scalar_values, line_num, compile_time_value, member_type);
               compile_time_value = (char *)compile_time_value + member_type->size_as_const;
               scalar_values += member_type->scalar_count;
            }
         }
         return;

      case SYMBOL_ARRAY_TYPE:
         // These can't have compile time values.
         UNREACHABLE();
         return;

      default:
         UNREACHABLE();
         return;
   }
}



//
// Dataflow calculation for different expression flavours.
//

typedef enum {
   GUARD_LOOP,
   GUARD_FUNCTION
} GuardFlavour;

typedef struct _Guard {
   GuardFlavour flavour;

   union {
      struct {
         Dataflow *break_guard;
         Dataflow *continue_guard;
      } loop;

      struct {
         Dataflow **scalar_values;
         Dataflow *return_guard;
      } function;
   } u;

   Dataflow *if_guard;

   struct _Guard *next;
} Guard;

static Guard *guard_construct_loop(Guard *next)
{
   Guard *guard = (Guard *)malloc_fast(sizeof(Guard));

   guard->flavour = GUARD_LOOP;
   guard->u.loop.break_guard = g_BoolTrue;
   guard->u.loop.continue_guard = g_BoolTrue;
   guard->if_guard = g_BoolTrue;
   guard->next = next;

   return guard;
}

static Guard *guard_construct_function(Dataflow **scalar_values, Guard *next)
{
   Guard *guard = (Guard *)malloc_fast(sizeof(Guard));

   guard->flavour = GUARD_FUNCTION;
   guard->u.function.scalar_values = scalar_values;
   guard->u.function.return_guard = g_BoolTrue;
   guard->if_guard = g_BoolTrue;
   guard->next = next;

   return guard;
}

static Guard *g_CurrentGuard;

static Dataflow *guard_scalar_assign(int line_num, Dataflow *new_value, Dataflow *old_value, bool is_local_variable)
{
   Dataflow *cond = g_BoolTrue;
   Guard *guard;

   for (guard = g_CurrentGuard; guard; guard = guard->next) {
      switch (guard->flavour) {
      case GUARD_LOOP:
         cond = glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LOGICAL_AND, cond, guard->u.loop.break_guard);
         cond = glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LOGICAL_AND, cond, guard->u.loop.continue_guard);
         break;
      case GUARD_FUNCTION:
         cond = glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LOGICAL_AND, cond, guard->u.function.return_guard);
         break;
      default:
         UNREACHABLE();
         break;
      }

      cond = glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LOGICAL_AND, cond, guard->if_guard);

      if (guard->flavour == GUARD_FUNCTION && is_local_variable)
         break;
   }

   return glsl_dataflow_construct_cond_op(line_num, cond, new_value, old_value);
}

static void update_return_guard(int line_num)
{
   /*
      we've not returned if we hadn't returned and either the current statement is inactive or we've broken or continued

      r = r . (¬i + ¬c + ¬b)
        = r . ¬(i . c . b)

      we must consider the if, continue and break guards out to the level of the function we are returning from
   */

   Dataflow *cond = g_BoolTrue;
   Guard *guard;

   for (guard = g_CurrentGuard; guard->flavour == GUARD_LOOP; guard = guard->next) {
      cond = glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LOGICAL_AND, cond, guard->u.loop.break_guard);
      cond = glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LOGICAL_AND, cond, guard->u.loop.continue_guard);
      cond = glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LOGICAL_AND, cond, guard->if_guard);
   }

   cond = glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LOGICAL_AND, cond, guard->if_guard);
   cond = glsl_dataflow_construct_unary_op(line_num, DATAFLOW_LOGICAL_NOT, cond);

   guard->u.function.return_guard = glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LOGICAL_AND, cond, guard->u.function.return_guard);
}

static void update_scalar_values(int line_num, Dataflow **scalar_values, int count)
{
   Guard *guard = g_CurrentGuard;
   int i;

   while (guard->flavour == GUARD_LOOP)
      guard = guard->next;

   for (i = 0; i < count; i++)
      guard->u.function.scalar_values[i] = glsl_dataflow_construct_cond_op(line_num, guard->u.function.return_guard, scalar_values[i], guard->u.function.scalar_values[i]);
}

// Locates all expressions top-down, and calculates dataflow graphs for them.
static Statement* sprev_calculate_dataflow(Statement* statement, void* data)
{
   bool traverse_function_def = (bool)data;

   switch (statement->flavour)
   {
	   case STATEMENT_AST:
	   case STATEMENT_COMPOUND:
	   case STATEMENT_DECL_LIST:
         return statement; // no expressions at this level, but recurse down

	   case STATEMENT_FUNCTION_DEF:
         if (traverse_function_def || !strcmp(statement->u.function_def.header->name, "main"))
            return statement;
         else
            return NULL;

	   case STATEMENT_VAR_DECL:
         {
            vcos_assert(SYMBOL_VAR_INSTANCE == statement->u.var_decl.var->flavour);

            // If there's an initializer, and the variable is non-const,
            // save the dataflow graphs (for each scalar value in the initializer) in the variable.
            if (statement->u.var_decl.initializer && TYPE_QUAL_CONST != statement->u.var_decl.var->u.var_instance.type_qual)
            {
               expr_calculate_dataflow(statement->u.var_decl.var->u.var_instance.scalar_values, statement->u.var_decl.initializer);
            }
         }
         return NULL; // no need to recurse

	   case STATEMENT_EXPR:
         {
            // Throw away the dataflow graphs at the top level.
            Dataflow** scalar_values = stack_alloc_by_type(statement->u.expr.expr->type);

            expr_calculate_dataflow(scalar_values, statement->u.expr.expr);

            stack_free();
         }
         return NULL; // no need to recurse

	   case STATEMENT_SELECTION:
         {
            Dataflow *cond_scalar_value;
            expr_calculate_dataflow(&cond_scalar_value, statement->u.selection.cond);

            {
               Dataflow *if_guard = g_CurrentGuard->if_guard;

               g_CurrentGuard->if_guard = glsl_dataflow_construct_binary_op(statement->line_num, DATAFLOW_LOGICAL_AND, cond_scalar_value, if_guard);

               glsl_statement_accept_prefix(statement->u.selection.if_true, NULL, sprev_calculate_dataflow, NULL);

               if (statement->u.selection.if_false) {
                  g_CurrentGuard->if_guard = glsl_dataflow_construct_binary_op(statement->line_num, DATAFLOW_LOGICAL_AND, glsl_dataflow_construct_unary_op(statement->line_num, DATAFLOW_LOGICAL_NOT, cond_scalar_value), if_guard);

                  glsl_statement_accept_prefix(statement->u.selection.if_false, NULL, sprev_calculate_dataflow, NULL);
               }

               g_CurrentGuard->if_guard = if_guard;
            }
         }
         return NULL; // already handled recursion manually

	   case STATEMENT_ITERATOR_FOR:
      {
         Dataflow *scalar_value;
         int i;

         glsl_statement_accept_prefix(statement->u.iterator_for.init, NULL, sprev_calculate_dataflow, NULL);

         g_CurrentGuard = guard_construct_loop(g_CurrentGuard);

         for (i = 0; i < statement->u.iterator_for.iterations; i++) {
            glsl_statement_accept_prefix(statement->u.iterator_for.block, NULL, sprev_calculate_dataflow, NULL);

            g_CurrentGuard->u.loop.continue_guard = g_BoolTrue;

            expr_calculate_dataflow(&scalar_value, statement->u.iterator_for.loop);
         }

         g_CurrentGuard = g_CurrentGuard->next;

         return NULL;
      }

	   case STATEMENT_ITERATOR_WHILE:
	   case STATEMENT_ITERATOR_DO_WHILE:
         // These should never appear!
         UNREACHABLE();
         return NULL;

	   case STATEMENT_CONTINUE:
         if (g_CurrentGuard->flavour != GUARD_LOOP)
            glsl_compile_error(ERROR_CUSTOM, 29, statement->line_num, NULL);

         /*
            we've not continued if we hadn't continued and the current statement is inactive

            c = c . ¬i
         */

         g_CurrentGuard->u.loop.continue_guard = glsl_dataflow_construct_binary_op(statement->line_num, DATAFLOW_LOGICAL_AND, g_CurrentGuard->u.loop.continue_guard, glsl_dataflow_construct_unary_op(statement->line_num, DATAFLOW_LOGICAL_NOT, g_CurrentGuard->if_guard));

         return NULL;

	   case STATEMENT_BREAK:
         if (g_CurrentGuard->flavour != GUARD_LOOP)
            glsl_compile_error(ERROR_CUSTOM, 29, statement->line_num, NULL);

         /*
            we've not broken if we hadn't broken and either the current statement is inactive or we've continued

            b = b . (¬i + ¬c)
              = b . ¬(i . c)
         */

         g_CurrentGuard->u.loop.break_guard = glsl_dataflow_construct_binary_op(statement->line_num, DATAFLOW_LOGICAL_AND, g_CurrentGuard->u.loop.break_guard, glsl_dataflow_construct_unary_op(statement->line_num, DATAFLOW_LOGICAL_NOT, glsl_dataflow_construct_binary_op(statement->line_num, DATAFLOW_LOGICAL_AND, g_CurrentGuard->if_guard, g_CurrentGuard->u.loop.continue_guard)));

         return NULL;

	   case STATEMENT_DISCARD:
         g_BuiltInVar__discard->u.var_instance.scalar_values[0] = guard_scalar_assign(statement->line_num, g_BoolTrue, g_BuiltInVar__discard->u.var_instance.scalar_values[0], false);
         return NULL;

	   case STATEMENT_RETURN:
         update_return_guard(statement->line_num);
         return NULL;

	   case STATEMENT_RETURN_EXPR:
      {
         Dataflow** scalar_values = stack_alloc_by_type(statement->u.return_expr.expr->type);

         expr_calculate_dataflow(scalar_values, statement->u.return_expr.expr);

         update_scalar_values(statement->line_num, scalar_values, statement->u.return_expr.expr->type->scalar_count);
         update_return_guard(statement->line_num);

         stack_free();
         return NULL;
      }

	   case STATEMENT_NULL:
         return NULL; // no need to recurse

	   default:
		   UNREACHABLE();
		   return NULL;
   }
}

void glsl_calculate_dataflow(Statement* ast)
{
   TRACE_PHASE(("calculating dataflow graphs"));

   g_CurrentGuard = guard_construct_function(NULL, NULL);

   glsl_statement_accept_prefix(ast, (void *)false, sprev_calculate_dataflow, NULL);
}

static INLINE void expr_calculate_dataflow_instance(Dataflow** scalar_values, Expr* expr)
{
   // Copy all dataflow pointers from the variable.
   switch (expr->u.instance.symbol->flavour) {
   case SYMBOL_VAR_INSTANCE:
   case SYMBOL_PARAM_INSTANCE:
      memcpy(scalar_values, expr->u.instance.symbol->u.var_instance.scalar_values, expr->type->scalar_count * sizeof(Dataflow*));
      break;
   default:
      UNREACHABLE();
      break;
   }
}

static INLINE Dataflow *build_uniform_offset(Expr *expr, Dataflow **linkable_value, int n)
{
   STACK_CHECK();

   switch (expr->flavour) {
   case EXPR_INSTANCE:
   {
      int i;

      // Assert that only uniforms can have non-constant indexing.
      vcos_assert(SYMBOL_VAR_INSTANCE == expr->u.instance.symbol->flavour &&
             TYPE_QUAL_UNIFORM == expr->u.instance.symbol->u.var_instance.type_qual);

      for (i = 0; i < n; i++)
         linkable_value[i] = expr->u.instance.symbol->u.var_instance.scalar_values[i];

      return g_IntZero;
   }
   case EXPR_SUBSCRIPT:
   {
      Dataflow* aggregate_scalar_value;
      Dataflow* subscript_scalar_value;

      // Recurse on aggregate.
      aggregate_scalar_value = build_uniform_offset(expr->u.subscript.aggregate, linkable_value, n);

      // Recurse on subscript.
      expr_calculate_dataflow(&subscript_scalar_value, expr->u.subscript.subscript);
      vcos_assert(PRIM_INT == subscript_scalar_value->type_index);

      subscript_scalar_value = glsl_dataflow_construct_binary_op(expr->line_num, DATAFLOW_MUL, subscript_scalar_value, glsl_dataflow_construct_const_int(expr->line_num, (const_int)expr->type->scalar_count));

      return glsl_dataflow_construct_binary_op(expr->line_num, DATAFLOW_ADD, subscript_scalar_value, aggregate_scalar_value);
   }
   case EXPR_FIELD_SELECTOR_STRUCT:
   {
      Dataflow* aggregate_scalar_value;
      int i, field_offset = 0;

      // Recurse on aggregate.
      aggregate_scalar_value = build_uniform_offset(expr->u.field_selector_struct.aggregate, linkable_value, n);

      for (i = 0; i < expr->u.field_selector_struct.field_no; i++)
      {
         field_offset += expr->u.field_selector_struct.aggregate->type->u.struct_type.member_types[i]->scalar_count;
      }

      return glsl_dataflow_construct_binary_op(expr->line_num, DATAFLOW_ADD, glsl_dataflow_construct_const_int(expr->line_num, field_offset), aggregate_scalar_value);
   }

   default:
      UNREACHABLE();
      return NULL;
   }
}

#ifdef __BCM2708A0__
static const_float cf(float f)
{
   return *(const_float *)&f;
}
#endif

static int num_tmu_lookups;     /* Number of tmu lookups used for uniform arrays */

static INLINE void expr_calculate_dataflow_subscript(Dataflow** scalar_values, Expr* expr)
{
   Dataflow** aggregate_scalar_values;

   if (expr->u.subscript.subscript->constant_index_expression)
   {
      const_int expr_index;
      int scalar_index;
      
      if (expr->u.subscript.subscript->compile_time_value)
      {
         expr_index = *(const_int*)expr->u.subscript.subscript->compile_time_value;
      }
      else
      {
         Dataflow *dataflow;
         vcos_assert(expr->u.subscript.subscript->type->scalar_count == 1);
         expr_calculate_dataflow(&dataflow, expr->u.subscript.subscript);
         vcos_assert(dataflow->flavour == DATAFLOW_CONST_INT);
         expr_index = dataflow->u.const_int.value;
      }
      
      scalar_index = expr_index * expr->type->scalar_count;

      // Recurse on aggregate.
      if (EXPR_INSTANCE == expr->u.subscript.aggregate->flavour)
      {
         // It's a variable instance, so we can optimize out the recursion (which would just be a memcpy).
         // However, we do this less for speed, and more because it allows us to deal with much larger arrays.
         // (Instead of fitting the whole array in STACK_MAX_SCALAR_COUNT, we need fit just a single member.)
         Symbol* symbol = expr->u.subscript.aggregate->u.instance.symbol;
         aggregate_scalar_values = symbol->u.var_instance.scalar_values;
      }
      else
      {
         aggregate_scalar_values = stack_alloc_by_type(expr->u.subscript.aggregate->type);

         expr_calculate_dataflow(aggregate_scalar_values, expr->u.subscript.aggregate);
      }

      if (scalar_index >= expr->u.subscript.aggregate->type->scalar_count)
      {
         // Indexing with an integral constant expression greater than declared size.
         glsl_compile_error(ERROR_SEMANTIC, 20, expr->line_num, NULL);
         return;
      }

      memcpy(scalar_values, aggregate_scalar_values + scalar_index, expr->type->scalar_count * sizeof(Dataflow*));

      if (EXPR_INSTANCE != expr->u.subscript.aggregate->flavour)
         stack_free();
   }
   else
   {
#ifdef __VIDEOCORE4__
      uint32_t size;
#endif
      int i;
      Dataflow* subscript_scalar_value;

      aggregate_scalar_values = stack_alloc_by_type(expr->type);

      subscript_scalar_value = build_uniform_offset(expr, aggregate_scalar_values, expr->type->scalar_count);

#ifdef __VIDEOCORE4__
      {
         Expr *thing = expr;
         while (thing->flavour == EXPR_SUBSCRIPT)
         {
            thing = thing->u.subscript.aggregate;
         }
         size = thing->type->scalar_count;
      }

#ifdef __BCM2708A0__
      //2708A0
      subscript_scalar_value = glsl_dataflow_construct_unary_op(expr->line_num, DATAFLOW_INTRINSIC_INTTOFLOAT, subscript_scalar_value);

      for (i = 0; i < expr->type->scalar_count; i++)
      {
         Dataflow *value;

         value = glsl_dataflow_construct_binary_op(
            expr->line_num,
            DATAFLOW_ADD,
            subscript_scalar_value,
            glsl_dataflow_construct_const_float(expr->line_num, cf((float)i + 0.5f)));

         value = glsl_dataflow_construct_binary_op(
            expr->line_num,
            DATAFLOW_MUL,
            value,
            glsl_dataflow_construct_const_float(expr->line_num, cf((float)(1.0 / (double)size))));
         scalar_values[i] = glsl_dataflow_construct_linkable_value_offset(
            expr->line_num,
            DATAFLOW_UNIFORM_OFFSET,
            glsl_get_scalar_value_type_index(expr->type, i),
            glsl_dataflow_construct_indexed_uniform_sampler(expr->line_num, aggregate_scalar_values[0], size),
            value);
      }
#else
      //2708B0
      {
         Dataflow *base = glsl_dataflow_construct_uniform_address(expr->line_num, aggregate_scalar_values[0], size);

         subscript_scalar_value = glsl_dataflow_construct_binary_op(expr->line_num, DATAFLOW_INTRINSIC_MAX, subscript_scalar_value, glsl_dataflow_construct_const_int(expr->line_num, 0));
         subscript_scalar_value = glsl_dataflow_construct_binary_op(expr->line_num, DATAFLOW_INTRINSIC_MIN, subscript_scalar_value, glsl_dataflow_construct_const_int(expr->line_num, size - 1));
         subscript_scalar_value = glsl_dataflow_construct_binary_op(expr->line_num, DATAFLOW_MUL, subscript_scalar_value, glsl_dataflow_construct_const_int(expr->line_num, 4));
         subscript_scalar_value = glsl_dataflow_construct_unary_op(expr->line_num, DATAFLOW_COERCE_TOINT, subscript_scalar_value);
         subscript_scalar_value = glsl_dataflow_construct_binary_op(expr->line_num, DATAFLOW_INTEGER_ADD, base, subscript_scalar_value);

         num_tmu_lookups += expr->type->scalar_count;
         if (num_tmu_lookups >= MAX_TMU_LOOKUPS) {
            glsl_compile_error(ERROR_CUSTOM, 8, expr->line_num, "Caused by uniform array access");
         }

         for (i = 0; i < expr->type->scalar_count; i++)
         {
            Dataflow *value;

            /* TODO: it's annoying that we have to do it this way (to stop ourselves accidentally converting 31 back to float) */
            value = glsl_dataflow_construct_const_float(expr->line_num, 4 * i);
            value->type_index = PRIM_INT;

            value = glsl_dataflow_construct_binary_op(expr->line_num, DATAFLOW_INTEGER_ADD, subscript_scalar_value, value);
            scalar_values[i] = glsl_dataflow_construct_linkable_value_offset(
               expr->line_num,
               DATAFLOW_UNIFORM_OFFSET,
               glsl_get_scalar_value_type_index(expr->type, i),
               NULL,
               value);
         }
      }
#endif
#else
      //2707
      subscript_scalar_value = glsl_dataflow_construct_unary_op(expr->line_num, DATAFLOW_COERCE_TOINT, subscript_scalar_value);

      for (i = 0; i < expr->type->scalar_count; i++)
      {
         scalar_values[i] = glsl_dataflow_construct_linkable_value_offset(
            expr->line_num,
            DATAFLOW_UNIFORM_OFFSET,
            glsl_get_scalar_value_type_index(expr->type, i),
            aggregate_scalar_values[0 + i],
            subscript_scalar_value);
      }
#endif

      stack_free();
   }
}

static INLINE void expr_calculate_dataflow_sequence(Dataflow** scalar_values, Expr* expr)
{
   Dataflow** all_these_scalar_values = stack_alloc_by_type(expr->u.sequence.all_these->type);

   expr_calculate_dataflow(all_these_scalar_values, expr->u.sequence.all_these);
   expr_calculate_dataflow(scalar_values, expr->u.sequence.then_this);

   stack_free();
}

static INLINE void expr_calculate_dataflow_prim_constructor_call(Dataflow** scalar_values, Expr* expr)
{
   int d;
   ExprChain* args = expr->u.prim_constructor_call.args;
   PrimitiveTypeIndex scalar_type_index;

   vcos_assert(SYMBOL_PRIMITIVE_TYPE == expr->type->flavour);
   scalar_type_index = primitiveScalarTypeIndices[expr->type->u.primitive_type.index];
   d = primitiveTypeSubscriptDimensions[expr->type->u.primitive_type.index];

   switch(expr->u.prim_constructor_call.flavour)
   {
      case PRIM_CONS_SCALAR_FROM_FIRST_COMPONENT:
         {
            Dataflow** arg_scalar_values = stack_alloc_by_type(args->first->expr->type);

            // Recurse on first arg.
            expr_calculate_dataflow(arg_scalar_values, args->first->expr);

            // Convert first component of first arg.
            scalar_values[0] = glsl_dataflow_convert_type(arg_scalar_values[0], scalar_type_index);

            stack_free();
         }
         return;

      case PRIM_CONS_VECTOR_FROM_SCALAR:
         {
            int i;
            Dataflow** arg_scalar_values = stack_alloc_by_type(args->first->expr->type);
            Dataflow* converted_scalar_value;

            // Recurse on first arg.
            expr_calculate_dataflow(arg_scalar_values, args->first->expr);

            // Convert first component of first arg.
            converted_scalar_value = glsl_dataflow_convert_type(arg_scalar_values[0], scalar_type_index);

            // Fill.
            for (i = 0; i < d; i++)
            {
               scalar_values[i] = converted_scalar_value;
            }

            stack_free();
         }
         return;

      case PRIM_CONS_MATRIX_FROM_SCALAR:
         {
            int i, j, offset;
            Dataflow** arg_scalar_values = stack_alloc_by_type(args->first->expr->type);
            Dataflow* converted_scalar_value;

            // Recurse on first arg.
            expr_calculate_dataflow(arg_scalar_values, args->first->expr);

            // Convert first component of first arg.
            converted_scalar_value = glsl_dataflow_convert_type(arg_scalar_values[0], scalar_type_index);
            // Create zero value.

            // Fill.
            offset = 0;
            for (i = 0; i < d; i++)
            {
               for (j = 0; j < d; j++)
               {
                  scalar_values[offset++] = (i == j) ? converted_scalar_value : g_FloatZero;
               }
            }

            stack_free();
         }
         return;

      case PRIM_CONS_MATRIX_FROM_MATRIX:
         {
            int i, j, arg_d, offset;
            Dataflow** arg_scalar_values = stack_alloc_by_type(args->first->expr->type);

            // Recurse on first arg.
            expr_calculate_dataflow(arg_scalar_values, args->first->expr);

            // Find first arg dimension.
            vcos_assert(SYMBOL_PRIMITIVE_TYPE == args->first->expr->type->flavour);
            arg_d = primitiveTypeSubscriptDimensions[args->first->expr->type->u.primitive_type.index];

            // Fill.
            offset = 0;
            for (i = 0; i < d; i++)
            {
               for (j = 0; j < d; j++)
               {
                  scalar_values[offset++] = (i < arg_d && j < arg_d) ? arg_scalar_values[arg_d * i + j]
                     : ((i == j) ? g_FloatOne : g_FloatZero);
               }
            }

            stack_free();
         }
         return;

      case PRIM_CONS_VECTOR_OR_MATRIX_FROM_COMPONENT_FLOW:
         {
            int offset, arg_offset;
            ExprChainNode* arg;

            offset = 0;
            for (arg = args->first; arg; arg = arg->next)
            {
               Dataflow** arg_scalar_values = stack_alloc_by_type(arg->expr->type);

               arg_offset = 0;

               // Recurse on arg.
               expr_calculate_dataflow(arg_scalar_values, arg->expr);

               while (arg_offset < arg->expr->type->scalar_count && offset < expr->type->scalar_count)
               {
                  scalar_values[offset++] = glsl_dataflow_convert_type(arg_scalar_values[arg_offset++], scalar_type_index);
               }

               stack_free();
            }
         }
         return;

      case PRIM_CONS_INVALID:
      default:
         UNREACHABLE();
         return;
   }
}

static INLINE void expr_calculate_dataflow_type_constructor_call(Dataflow** scalar_values, Expr* expr)
{
   ExprChainNode* arg;

   for (arg = expr->u.type_constructor_call.args->first; arg; arg = arg->next)
   {
      Dataflow** arg_scalar_values = stack_alloc_by_type(arg->expr->type);

      // Recurse on arg.
      expr_calculate_dataflow(arg_scalar_values, arg->expr);

      // Copy out result.
      memcpy(scalar_values, arg_scalar_values, arg->expr->type->scalar_count * sizeof(Dataflow*));
      scalar_values += arg->expr->type->scalar_count;

      stack_free();
   }
}

static INLINE void expr_calculate_dataflow_field_selector_swizzle(Dataflow** scalar_values, Expr* expr)
{
   int i;
   Dataflow** aggregate_scalar_values = stack_alloc_by_type(expr->u.field_selector_swizzle.aggregate->type);

   // Recurse on aggregate.
   expr_calculate_dataflow(aggregate_scalar_values, expr->u.field_selector_swizzle.aggregate);

   for (i = 0; i < MAX_SWIZZLE_FIELD_COUNT && expr->u.field_selector_swizzle.swizzle_slots[i] != SWIZZLE_SLOT_UNUSED; i++)
   {
      vcos_assert(expr->u.field_selector_swizzle.swizzle_slots[i] < expr->u.field_selector_swizzle.aggregate->type->scalar_count);

      scalar_values[i] = aggregate_scalar_values[expr->u.field_selector_swizzle.swizzle_slots[i]];
   }

   stack_free();
}

static INLINE void expr_calculate_dataflow_field_selector_struct(Dataflow** scalar_values, Expr* expr)
{
   int i;
   Dataflow** aggregate_scalar_values = stack_alloc_by_type(expr->u.field_selector_struct.aggregate->type);
   Dataflow** field = aggregate_scalar_values;

   // Recurse on aggregate.
   expr_calculate_dataflow(aggregate_scalar_values, expr->u.field_selector_struct.aggregate);

   for (i = 0; i < expr->u.field_selector_struct.field_no; i++)
   {
      field += expr->u.field_selector_struct.aggregate->type->u.struct_type.member_types[i]->scalar_count;
   }

   // Copy out field.
   memcpy(scalar_values, field, expr->type->scalar_count * sizeof(Dataflow*));

   stack_free();
}

// Not inlined as it's common.
static void expr_calculate_dataflow_unary_op_common(Dataflow** scalar_values, Expr* expr)
{
   Expr* operand;
	int i;
   Dataflow** operand_scalar_values;

   DataflowFlavour dataflow_flavour;

   // Work out operand.
   switch (expr->flavour)
   {
      // Logical.
      // Arithemetic.
      case EXPR_LOGICAL_NOT:
      case EXPR_ARITH_NEGATE:
         operand = expr->u.unary_op.operand;
         break;

      // Intrinsic.
      case EXPR_INTRINSIC_RSQRT:
      case EXPR_INTRINSIC_RCP:
      case EXPR_INTRINSIC_LOG2:
      case EXPR_INTRINSIC_EXP2:
      case EXPR_INTRINSIC_CEIL:
      case EXPR_INTRINSIC_FLOOR:
      case EXPR_INTRINSIC_SIGN:
      case EXPR_INTRINSIC_TRUNC:
      case EXPR_INTRINSIC_NEAREST:
         operand = expr->u.intrinsic.args->first->expr;
         break;

      default:
         UNREACHABLE();
         return;
   }

   operand_scalar_values = stack_alloc_by_type(operand->type);

   // Recurse.
   expr_calculate_dataflow(operand_scalar_values, operand);

   // Work out dataflow flavour.
   switch (expr->flavour)
   {
      // Logical.
      case EXPR_LOGICAL_NOT:
         dataflow_flavour = DATAFLOW_LOGICAL_NOT;
         break;

      // Arithemetic.
      case EXPR_ARITH_NEGATE:
         dataflow_flavour = DATAFLOW_ARITH_NEGATE;
         break;

      // Intrinsic.
      case EXPR_INTRINSIC_RSQRT:
         dataflow_flavour = DATAFLOW_INTRINSIC_RSQRT;
         break;
      case EXPR_INTRINSIC_RCP:
         dataflow_flavour = DATAFLOW_INTRINSIC_RCP;
         break;
      case EXPR_INTRINSIC_LOG2:
         dataflow_flavour = DATAFLOW_INTRINSIC_LOG2;
         break;
      case EXPR_INTRINSIC_EXP2:
         dataflow_flavour = DATAFLOW_INTRINSIC_EXP2;
         break;
      case EXPR_INTRINSIC_CEIL:
         dataflow_flavour = DATAFLOW_INTRINSIC_CEIL;
         break;
      case EXPR_INTRINSIC_FLOOR:
         dataflow_flavour = DATAFLOW_INTRINSIC_FLOOR;
         break;
      case EXPR_INTRINSIC_SIGN:
         dataflow_flavour = DATAFLOW_INTRINSIC_SIGN;
         break;
      case EXPR_INTRINSIC_TRUNC:
         dataflow_flavour = DATAFLOW_INTRINSIC_FLOATTOINT_TRUNC;
         break;
      case EXPR_INTRINSIC_NEAREST:
         dataflow_flavour = DATAFLOW_INTRINSIC_FLOATTOINT_NEAREST;
         break;

      default:
         UNREACHABLE();
         return;
   }

   // Create nodes.
   for (i = 0; i < expr->type->scalar_count; i++)
   {
      scalar_values[i] = glsl_dataflow_construct_unary_op(
         expr->line_num,
         dataflow_flavour,
         operand_scalar_values[i]);

      if (dataflow_flavour == DATAFLOW_INTRINSIC_FLOATTOINT_TRUNC || dataflow_flavour == DATAFLOW_INTRINSIC_FLOATTOINT_NEAREST)
         scalar_values[i] = glsl_dataflow_construct_unary_op(
            expr->line_num,
            DATAFLOW_COERCE_TOFLOAT,
            scalar_values[i]);
   }

   stack_free();
}

static INLINE void expr_calculate_dataflow_binary_op_arithmetic(Dataflow** scalar_values, Expr* expr)
{
	Expr* left = expr->u.binary_op.left;
   Expr* right = expr->u.binary_op.right;

   ExprFlavour flavour = expr->flavour;
   DataflowFlavour dataflow_flavour;

   Dataflow** left_scalar_values = stack_alloc_by_type(left->type);
   Dataflow** right_scalar_values = stack_alloc_by_type(right->type);

   // Recurse.
   expr_calculate_dataflow(left_scalar_values, left);
   expr_calculate_dataflow(right_scalar_values, right);

   // Work out dataflow flavour for the component-wise operations.
   switch (flavour)
   {
      case EXPR_MUL:
         dataflow_flavour = DATAFLOW_MUL;
         break;
      case EXPR_ADD:
         dataflow_flavour = DATAFLOW_ADD;
         break;
      case EXPR_SUB:
         dataflow_flavour = DATAFLOW_SUB;
         break;
      default:
         UNREACHABLE();
         return;
   }

   // The following control flow is based on that in ast.c and should be kept in sync with it.
   {
      int left_index, right_index;
      PRIMITIVE_TYPE_FLAGS_T left_flags, right_flags;
      left_index = left->type->u.primitive_type.index;
      left_flags = primitiveTypeFlags[left_index];
      right_index = right->type->u.primitive_type.index;
      right_flags = primitiveTypeFlags[right_index];

      // Infer type, and value if possible.
      // From spec (1.00 rev 14 p46), the two operands must be:
      // 1 - the same type (the type being integer scalar/vector, float scalar/vector/matrix),
      // 2 - or one can be a scalar float and the other a float vector or matrix,
      // 3 - or one can be a scalar integer and the other an integer vector,
      // 4 - or, for multiply, one can be a float vector and the other a float matrix with the same dimensional size.
      // All operations are component-wise except EXPR_MUL involving at least one matrix (cases 1 and 4).

      // Case 1.
      if (left->type == right->type)
      {
	      if (EXPR_MUL == flavour && (left_flags & PRIM_MATRIX_TYPE))
	      {
		      // Linear algebraic mat * mat.
            int i, j, k, d = primitiveTypeSubscriptDimensions[expr->type->u.primitive_type.index];

            for (i = 0; i < d; i++)
            {
               for (j = 0; j < d; j++)
               {
                  Dataflow* acc;

                  k = 0;

                  // Multiply the first pair.
                  acc = glsl_dataflow_construct_binary_op(
                     expr->line_num,
                     DATAFLOW_MUL,
                     left_scalar_values[k * d + j],
                     right_scalar_values[i * d + k]);

                  // Fold in the rest.
                  for (k = 1; k < d; k++)
                  {
                     Dataflow* mul;

                     mul = glsl_dataflow_construct_binary_op(
                        expr->line_num,
                        DATAFLOW_MUL,
                        left_scalar_values[k * d + j],
                        right_scalar_values[i * d + k]);

                     acc = glsl_dataflow_construct_binary_op(
                        expr->line_num,
                        DATAFLOW_ADD,
                        acc,
                        mul);
                  }

                  scalar_values[i * d + j] = acc;
               }
            }

            stack_free();
            stack_free();
            return;
	      }
	      else
	      {
		      // Component-wise on same scalar type, same scalar count.
            int i;

            for (i = 0; i < expr->type->scalar_count; i++)
            {
               vcos_assert(left_scalar_values[i]);
               vcos_assert(right_scalar_values[i]);

               scalar_values[i] = glsl_dataflow_construct_binary_op(
                  expr->line_num,
                  dataflow_flavour,
                  left_scalar_values[i],
                  right_scalar_values[i]);
            }

            stack_free();
            stack_free();
            return;
	      }
      }

      // Case 2.
      if ((left_flags & PRIM_FLOAT_TYPE) && (left_flags & PRIM_SCALAR_TYPE)
	      && (right_flags & PRIM_FLOAT_TYPE) && (right_flags & (PRIM_VECTOR_TYPE | PRIM_MATRIX_TYPE)))
      {
         // Component-wise on same scalar type, different scalar counts.
         int i;

         for (i = 0; i < expr->type->scalar_count; i++)
         {
            scalar_values[i] = glsl_dataflow_construct_binary_op(
               expr->line_num,
               dataflow_flavour,
               left_scalar_values[0],
               right_scalar_values[i]);
         }

         stack_free();
         stack_free();
         return;
      }
      else if ((right_flags & PRIM_FLOAT_TYPE) && (right_flags & PRIM_SCALAR_TYPE)
	      && (left_flags & PRIM_FLOAT_TYPE) && (left_flags & (PRIM_VECTOR_TYPE | PRIM_MATRIX_TYPE)))
      {
         // Component-wise on same scalar type, different scalar counts.
         int i;

         for (i = 0; i < expr->type->scalar_count; i++)
         {
            scalar_values[i] = glsl_dataflow_construct_binary_op(
               expr->line_num,
               dataflow_flavour,
               left_scalar_values[i],
               right_scalar_values[0]);
         }

         stack_free();
         stack_free();
         return;
      }

      // Case 3.
      if ((left_flags & PRIM_INT_TYPE) && (left_flags & PRIM_SCALAR_TYPE)
	      && (right_flags & PRIM_INT_TYPE) && (right_flags & PRIM_VECTOR_TYPE))
      {
         // Component-wise on same scalar type, different scalar counts.
         int i;

         for (i = 0; i < expr->type->scalar_count; i++)
         {
            scalar_values[i] = glsl_dataflow_construct_binary_op(
               expr->line_num,
               dataflow_flavour,
               left_scalar_values[0],
               right_scalar_values[i]);
         }

         stack_free();
         stack_free();
         return;
      }
      else if ((right_flags & PRIM_INT_TYPE) && (right_flags & PRIM_SCALAR_TYPE)
	      && (left_flags & PRIM_INT_TYPE) && (left_flags & PRIM_VECTOR_TYPE))
      {
         // Component-wise on same scalar type, different scalar counts.
         int i;

         for (i = 0; i < expr->type->scalar_count; i++)
         {
            scalar_values[i] = glsl_dataflow_construct_binary_op(
               expr->line_num,
               dataflow_flavour,
               left_scalar_values[i],
               right_scalar_values[0]);
         }

         stack_free();
         stack_free();
         return;
      }

      // Case 4.
      if (EXPR_MUL == flavour)
      {
         if ((left_flags & PRIM_FLOAT_TYPE) && (left_flags & PRIM_MATRIX_TYPE)
		      && (right_flags & PRIM_FLOAT_TYPE) && (right_flags & PRIM_VECTOR_TYPE)
		      && primitiveTypeSubscriptDimensions[left_index] == primitiveTypeSubscriptDimensions[left_index])
	      {
		      /* Linear algebraic mat * vec. */
            int j, k, d = primitiveTypeSubscriptDimensions[expr->type->u.primitive_type.index];
            Dataflow** vec_scalar_values = right_scalar_values;
            Dataflow** mat_scalar_values = left_scalar_values;

            for (j = 0; j < d; j++)
            {
               Dataflow* acc;

               k = 0;

               // Multiply the first pair.
               acc = glsl_dataflow_construct_binary_op(
                  expr->line_num,
                  DATAFLOW_MUL,
                  mat_scalar_values[k * d + j],
                  vec_scalar_values[k]); // [i, k] i.e. [0, k]

               // Fold in the rest.
               for (k = 1; k < d; k++)
               {
                  Dataflow* mul;

                  mul = glsl_dataflow_construct_binary_op(
                     expr->line_num,
                     DATAFLOW_MUL,
                     mat_scalar_values[k * d + j],
                     vec_scalar_values[k]); // [i, k] i.e. [0, k]

                  acc = glsl_dataflow_construct_binary_op(
                     expr->line_num,
                     DATAFLOW_ADD,
                     acc,
                     mul);
               }

               scalar_values[j] = acc; // [i, j] i.e. [0, j]
            }

            stack_free();
            stack_free();
		      return;
	      }
	      else if ((right_flags & PRIM_FLOAT_TYPE) && (right_flags & PRIM_MATRIX_TYPE)
		      && (left_flags & PRIM_FLOAT_TYPE) && (left_flags & PRIM_VECTOR_TYPE)
		      && primitiveTypeSubscriptDimensions[left_index] == primitiveTypeSubscriptDimensions[left_index])
	      {
		      /* Linear algebraic vec * mat. */
            int i, k, d = primitiveTypeSubscriptDimensions[expr->type->u.primitive_type.index];
            Dataflow** vec_scalar_values = left_scalar_values;
            Dataflow** mat_scalar_values = right_scalar_values;

            for (i = 0; i < d; i++)
            {
               Dataflow* acc;

               k = 0;

               // Multiply the first pair.
               acc = glsl_dataflow_construct_binary_op(
                  expr->line_num,
                  DATAFLOW_MUL,
                  vec_scalar_values[k], // [k, j] i.e. [k, 0]
                  mat_scalar_values[i * d + k]);

               // Fold in the rest.
               for (k = 1; k < d; k++)
               {
                  Dataflow* mul;

                  mul = glsl_dataflow_construct_binary_op(
                     expr->line_num,
                     DATAFLOW_MUL,
                     vec_scalar_values[k], // [k, j] i.e. [k, 0]
                     mat_scalar_values[i * d + k]);

                  acc = glsl_dataflow_construct_binary_op(
                     expr->line_num,
                     DATAFLOW_ADD,
                     acc,
                     mul);
               }

               scalar_values[i] = acc; // [i, j] i.e. [i, 0]
            }

            stack_free();
            stack_free();
		      return;
	      }
      }
   }

   // All valid instances of expr should have been handled above.
   UNREACHABLE();
}

static Dataflow* construct_integer_rounder(int line_num, Dataflow* operand)
{
#ifdef __VIDEOCORE4__
   Dataflow *guess0, *guess1;
   guess0 = glsl_dataflow_construct_unary_op(line_num, DATAFLOW_COERCE_TOFLOAT, glsl_dataflow_construct_unary_op(line_num, DATAFLOW_COERCE_TOINT, operand));
   guess1 = glsl_dataflow_construct_binary_op(line_num, DATAFLOW_ADD, guess0, glsl_dataflow_construct_const_int(line_num, 1));
   return glsl_dataflow_construct_cond_op(line_num,
      glsl_dataflow_construct_binary_op(line_num, DATAFLOW_LESS_THAN,
         glsl_dataflow_construct_binary_op(line_num, DATAFLOW_SUB, operand, guess0),
         glsl_dataflow_construct_binary_op(line_num, DATAFLOW_SUB, guess1, operand)
      ),
      guess0,
      guess1);
#else
   return glsl_dataflow_construct_unary_op(line_num, DATAFLOW_COERCE_TOFLOAT, glsl_dataflow_construct_unary_op(line_num, DATAFLOW_COERCE_TOINT, operand));
#endif
}

static INLINE void expr_calculate_dataflow_binary_op_divide(Dataflow** scalar_values, Expr* expr)
{
	Expr* left = expr->u.binary_op.left;
   Expr* right = expr->u.binary_op.right;

   Dataflow** left_scalar_values = stack_alloc_by_type(left->type);
   Dataflow** right_scalar_values = stack_alloc_by_type(right->type);

   // Recurse.
   expr_calculate_dataflow(left_scalar_values, left);
   expr_calculate_dataflow(right_scalar_values, right);

   // The following control flow is based on that in ast.c and should be kept in sync with it.
   {
      int left_index, right_index;
      PRIMITIVE_TYPE_FLAGS_T left_flags, right_flags;
      left_index = left->type->u.primitive_type.index;
      left_flags = primitiveTypeFlags[left_index];
      right_index = right->type->u.primitive_type.index;
      right_flags = primitiveTypeFlags[right_index];

      // Infer type, and value if possible.
      // From spec (1.00 rev 14 p46), the two operands must be:
      // 1 - the same type (the type being integer scalar/vector, float scalar/vector/matrix),
      // 2 - or one can be a scalar float and the other a float vector or matrix,
      // 3 - or one can be a scalar integer and the other an integer vector,
      // 4 - or, for multiply, one can be a float vector and the other a float matrix with the same dimensional size.
      // All operations are component-wise except EXPR_MUL involving at least one matrix (cases 1 and 4).

      // Case 1.
      if (left->type == right->type)
      {
	      {
		      // Component-wise on same scalar type, same scalar count.
            int i;

            for (i = 0; i < expr->type->scalar_count; i++)
            {
               scalar_values[i] = glsl_dataflow_construct_binary_op(
                  expr->line_num,
                  DATAFLOW_MUL,
                  left_scalar_values[i],
                  glsl_dataflow_construct_unary_op(expr->line_num,
                                              DATAFLOW_INTRINSIC_RCP,
                                              right_scalar_values[i]));

               if (left_flags & PRIM_INT_TYPE)
                  scalar_values[i] = construct_integer_rounder(expr->line_num, scalar_values[i]);
            }

            stack_free();
            stack_free();
            return;
	      }
      }

      // Case 2.
      if ((left_flags & PRIM_FLOAT_TYPE) && (left_flags & PRIM_SCALAR_TYPE)
	      && (right_flags & PRIM_FLOAT_TYPE) && (right_flags & (PRIM_VECTOR_TYPE | PRIM_MATRIX_TYPE)))
      {
         // Component-wise on same scalar type, different scalar counts.
         int i;

         for (i = 0; i < expr->type->scalar_count; i++)
         {
            scalar_values[i] = glsl_dataflow_construct_binary_op(
               expr->line_num,
               DATAFLOW_MUL,
               left_scalar_values[0],
               glsl_dataflow_construct_unary_op(expr->line_num,
                                           DATAFLOW_INTRINSIC_RCP,
                                           right_scalar_values[i]));
         }

         stack_free();
         stack_free();
         return;
      }
      else if ((right_flags & PRIM_FLOAT_TYPE) && (right_flags & PRIM_SCALAR_TYPE)
	      && (left_flags & PRIM_FLOAT_TYPE) && (left_flags & (PRIM_VECTOR_TYPE | PRIM_MATRIX_TYPE)))
      {
         // Component-wise on same scalar type, different scalar counts.
         int i;

         for (i = 0; i < expr->type->scalar_count; i++)
         {
            scalar_values[i] = glsl_dataflow_construct_binary_op(
               expr->line_num,
               DATAFLOW_MUL,
               left_scalar_values[i],
               glsl_dataflow_construct_unary_op(expr->line_num,
                                           DATAFLOW_INTRINSIC_RCP,
                                           right_scalar_values[0]));
         }

         stack_free();
         stack_free();
         return;
      }

      // Case 3.
      if ((left_flags & PRIM_INT_TYPE) && (left_flags & PRIM_SCALAR_TYPE)
	      && (right_flags & PRIM_INT_TYPE) && (right_flags & PRIM_VECTOR_TYPE))
      {
         // Component-wise on same scalar type, different scalar counts.
         int i;

         for (i = 0; i < expr->type->scalar_count; i++)
         {
            scalar_values[i] = glsl_dataflow_construct_binary_op(
               expr->line_num,
               DATAFLOW_MUL,
               left_scalar_values[0],
               glsl_dataflow_construct_unary_op(expr->line_num,
                                           DATAFLOW_INTRINSIC_RCP,
                                           right_scalar_values[i]));

            scalar_values[i] = construct_integer_rounder(expr->line_num, scalar_values[i]);
         }

         stack_free();
         stack_free();
         return;
      }
      else if ((right_flags & PRIM_INT_TYPE) && (right_flags & PRIM_SCALAR_TYPE)
	      && (left_flags & PRIM_INT_TYPE) && (left_flags & PRIM_VECTOR_TYPE))
      {
         // Component-wise on same scalar type, different scalar counts.
         int i;

         for (i = 0; i < expr->type->scalar_count; i++)
         {
            scalar_values[i] = glsl_dataflow_construct_binary_op(
               expr->line_num,
               DATAFLOW_MUL,
               left_scalar_values[i],
               glsl_dataflow_construct_unary_op(expr->line_num,
                                           DATAFLOW_INTRINSIC_RCP,
                                           right_scalar_values[0]));

            scalar_values[i] = construct_integer_rounder(expr->line_num, scalar_values[i]);
         }

         stack_free();
         stack_free();
         return;
      }
   }

   // All valid instances of expr should have been handled above.
   UNREACHABLE();
}

// Not inlined as it's common.
static void expr_calculate_dataflow_binary_op_common(Dataflow** scalar_values, Expr* expr)
{
	Expr* left;
   Expr* right;

   Dataflow* left_scalar_value;
   Dataflow* right_scalar_value;

   DataflowFlavour dataflow_flavour;

   // Work out left and right operands.
   switch (expr->flavour)
   {
      // Logical.
      // Relational.
      case EXPR_LOGICAL_XOR:
      case EXPR_LESS_THAN:
      case EXPR_LESS_THAN_EQUAL:
      case EXPR_GREATER_THAN:
      case EXPR_GREATER_THAN_EQUAL:
         left = expr->u.binary_op.left;
         right = expr->u.binary_op.right;
         break;

      // Intrinsic.
      case EXPR_INTRINSIC_MIN:
      case EXPR_INTRINSIC_MAX:
         left = expr->u.intrinsic.args->first->expr;
         right = expr->u.intrinsic.args->first->next->expr;
         break;

      default:
         UNREACHABLE();
         return;
   }

   // This code assumes we do not act on vectors.
   vcos_assert(1 == left->type->scalar_count);
   vcos_assert(1 == right->type->scalar_count);

   // Recurse.
   expr_calculate_dataflow(&left_scalar_value, left);
   expr_calculate_dataflow(&right_scalar_value, right);

   // Work out dataflow flavour.
   switch (expr->flavour)
   {
      // Logical.
      case EXPR_LOGICAL_XOR:
         dataflow_flavour = DATAFLOW_LOGICAL_XOR;
         break;

      // Relational.
      case EXPR_LESS_THAN:
         dataflow_flavour = DATAFLOW_LESS_THAN;
         break;
      case EXPR_LESS_THAN_EQUAL:
         dataflow_flavour = DATAFLOW_LESS_THAN_EQUAL;
         break;
      case EXPR_GREATER_THAN:
         dataflow_flavour = DATAFLOW_GREATER_THAN;
         break;
      case EXPR_GREATER_THAN_EQUAL:
         dataflow_flavour = DATAFLOW_GREATER_THAN_EQUAL;
         break;

      // Intrinsic.
      case EXPR_INTRINSIC_MIN:
         dataflow_flavour = DATAFLOW_INTRINSIC_MIN;
         break;
      case EXPR_INTRINSIC_MAX:
         dataflow_flavour = DATAFLOW_INTRINSIC_MAX;
         break;

      default:
         UNREACHABLE();
         return;
   }

   // Create node.
   scalar_values[0] = glsl_dataflow_construct_binary_op(
      expr->line_num,
      dataflow_flavour,
      left_scalar_value,
      right_scalar_value);
}

static void expr_calculate_dataflow_binary_op_short_circuit(Dataflow** scalar_values, Expr* expr)
{
	Expr* left;
   Expr* right;

   Dataflow* left_scalar_value;
   Dataflow* right_scalar_value;

   DataflowFlavour dataflow_flavour;

   left = expr->u.binary_op.left;
   right = expr->u.binary_op.right;

   // This code assumes we do not act on vectors.
   vcos_assert(1 == left->type->scalar_count);
   vcos_assert(1 == right->type->scalar_count);

   // Recurse.
   expr_calculate_dataflow(&left_scalar_value, left);

   // Work out dataflow flavour.
   switch (expr->flavour)
   {
      // Logical.
      case EXPR_LOGICAL_AND:
      {
         Dataflow *if_guard = g_CurrentGuard->if_guard;

         g_CurrentGuard->if_guard = glsl_dataflow_construct_binary_op(expr->line_num, DATAFLOW_LOGICAL_AND, left_scalar_value, if_guard);

         expr_calculate_dataflow(&right_scalar_value, right);

         g_CurrentGuard->if_guard = if_guard;

         dataflow_flavour = DATAFLOW_LOGICAL_AND;
         break;
      }
      case EXPR_LOGICAL_OR:
      {
         Dataflow *if_guard = g_CurrentGuard->if_guard;

         g_CurrentGuard->if_guard = glsl_dataflow_construct_binary_op(expr->line_num, DATAFLOW_LOGICAL_AND, glsl_dataflow_construct_unary_op(expr->line_num, DATAFLOW_LOGICAL_NOT, left_scalar_value), if_guard);

         expr_calculate_dataflow(&right_scalar_value, right);

         g_CurrentGuard->if_guard = if_guard;

         dataflow_flavour = DATAFLOW_LOGICAL_OR;
         break;
      }
      default:
         UNREACHABLE();
         return;
   }

   // Create node.
   scalar_values[0] = glsl_dataflow_construct_binary_op(
      expr->line_num,
      dataflow_flavour,
      left_scalar_value,
      right_scalar_value);
}

static INLINE void expr_calculate_dataflow_binary_op_equality(Dataflow** scalar_values, Expr* expr)
{
	Expr* left = expr->u.binary_op.left;
   Expr* right = expr->u.binary_op.right;

   Dataflow** left_scalar_values = stack_alloc_by_type(left->type);
   Dataflow** right_scalar_values = stack_alloc_by_type(right->type);

   DataflowFlavour dataflow_flavour;

   Dataflow* acc;
   int i;

   // Recurse.
   expr_calculate_dataflow(left_scalar_values, left);
   expr_calculate_dataflow(right_scalar_values, right);

   // Work out dataflow flavour.
   switch (expr->flavour)
   {
      case EXPR_EQUAL:
         dataflow_flavour = DATAFLOW_EQUAL;
         break;
      case EXPR_NOT_EQUAL:
         dataflow_flavour = DATAFLOW_NOT_EQUAL;
         break;
      default:
         UNREACHABLE();
         return;
   }

   // Compare first two scalars.
   acc = glsl_dataflow_construct_binary_op(
      expr->line_num,
      dataflow_flavour,
      left_scalar_values[0],
      right_scalar_values[0]);

   // Fold in the rest.
   vcos_assert(left->type->scalar_count == right->type->scalar_count);
   for (i = 1; i < left->type->scalar_count; i++)
   {
      Dataflow* cmp = glsl_dataflow_construct_binary_op(
         expr->line_num,
         dataflow_flavour,
         left_scalar_values[i],
         right_scalar_values[i]);

      acc = glsl_dataflow_construct_binary_op(
         expr->line_num,
         DATAFLOW_LOGICAL_AND,
         acc,
         cmp);
   }

   scalar_values[0] = acc;

   stack_free();
   stack_free();
}

static INLINE void expr_calculate_dataflow_cond_op(Dataflow** scalar_values, Expr* expr)
{
   int i;
   Dataflow* cond_scalar_value;
   Dataflow** true_scalar_values = stack_alloc_by_type(expr->u.cond_op.if_true->type);
   Dataflow** false_scalar_values = stack_alloc_by_type(expr->u.cond_op.if_false->type);

   // Recurse on operands.
   expr_calculate_dataflow(&cond_scalar_value, expr->u.cond_op.cond);

   {
      Dataflow *if_guard = g_CurrentGuard->if_guard;

      g_CurrentGuard->if_guard = glsl_dataflow_construct_binary_op(expr->line_num, DATAFLOW_LOGICAL_AND, cond_scalar_value, if_guard);
      expr_calculate_dataflow(true_scalar_values, expr->u.cond_op.if_true);
      g_CurrentGuard->if_guard = glsl_dataflow_construct_binary_op(expr->line_num, DATAFLOW_LOGICAL_AND, glsl_dataflow_construct_unary_op(expr->line_num, DATAFLOW_LOGICAL_NOT, cond_scalar_value), if_guard);
      expr_calculate_dataflow(false_scalar_values, expr->u.cond_op.if_false);

      g_CurrentGuard->if_guard = if_guard;
   }

   for (i = 0; i < expr->type->scalar_count; i++)
   {
      scalar_values[i] = glsl_dataflow_construct_cond_op(expr->line_num, cond_scalar_value, true_scalar_values[i], false_scalar_values[i]);
   }

   stack_free();
   stack_free();
}

static bool is_lvalue_local(Expr *lvalue)
{
   if (lvalue->flavour == EXPR_INSTANCE)
   {
      Symbol *symbol = lvalue->u.instance.symbol;
      if (symbol->flavour == SYMBOL_VAR_INSTANCE)
         return symbol->u.var_instance.is_local;
   }
   return false;
}

static INLINE void expr_calculate_dataflow_assign_op(Dataflow** scalar_values, Expr* expr)
{
   Dataflow*** lvalue_scalar_values = (Dataflow ***)malloc_fast(expr->type->scalar_count * sizeof(Dataflow**));
   int i;

   // Recurse on rvalue and write value of this expression to scalar_values.
   expr_calculate_dataflow(scalar_values, expr->u.assign_op.rvalue);

   // Evaluate lvalue.
   expr_evaluate_lvalue(expr->u.assign_op.lvalue, lvalue_scalar_values, 0, expr->type->scalar_count);

   // Copy to rvalue to lvalue.
   for (i = 0; i < expr->type->scalar_count; i++)
      *(lvalue_scalar_values[i]) = guard_scalar_assign(expr->line_num, scalar_values[i], *(lvalue_scalar_values[i]), is_lvalue_local(expr->u.assign_op.lvalue));
}

static INLINE void expr_calculate_dataflow_affix(Dataflow** scalar_values, Expr* expr)
{
   int i;

   Dataflow** operand_scalar_values = stack_alloc_by_type(expr->u.unary_op.operand->type);
   Dataflow*** lvalue_scalar_values = (Dataflow ***)malloc_fast(expr->type->scalar_count * sizeof(Dataflow**));  // TODO: stack allocate this

   bool is_pre = false;
   bool is_inc = false;

   // Manufacture a suitable representation of the constant 1.

   Dataflow* one = NULL;

   vcos_assert(expr->type->flavour == SYMBOL_PRIMITIVE_TYPE);

	switch (primitiveTypeFlags[expr->type->u.primitive_type.index] & (PRIM_INT_TYPE | PRIM_FLOAT_TYPE))
	{
		case PRIM_INT_TYPE:
         one = g_IntOne;
			break;
      case PRIM_FLOAT_TYPE:
         one = g_FloatOne;
			break;
		default:
			UNREACHABLE();
         break;
	}

   // detect whether we're pre or post, increment or decrement

   switch (expr->flavour) {
	case EXPR_POST_INC:
      is_pre = false;
      is_inc = true;
      break;
	case EXPR_POST_DEC:
      is_pre = false;
      is_inc = false;
      break;
	case EXPR_PRE_INC:
      is_pre = true;
      is_inc = true;
      break;
	case EXPR_PRE_DEC:
      is_pre = true;
      is_inc = false;
      break;
   default:
      UNREACHABLE();
      break;
   }

   // Evaluate lvalue.
   expr_evaluate_lvalue(expr->u.unary_op.operand, lvalue_scalar_values, 0, expr->type->scalar_count);

   // Evaluate rvalue.
   expr_calculate_dataflow(operand_scalar_values, expr->u.unary_op.operand);

   for (i = 0; i < expr->type->scalar_count; i++)
   {
      Dataflow *new_value = glsl_dataflow_construct_binary_op(
         expr->line_num,
         is_inc ? DATAFLOW_ADD : DATAFLOW_SUB,
         operand_scalar_values[i],
         one);

      *(lvalue_scalar_values[i]) = guard_scalar_assign(expr->line_num, new_value, *(lvalue_scalar_values[i]), is_lvalue_local(expr->u.unary_op.operand));

      scalar_values[i] = is_pre ? new_value : operand_scalar_values[i];
   }

   stack_free();
}

static INLINE void expr_calculate_dataflow_function_call(Dataflow** scalar_values, Expr* expr)
{
   int size;
   int i;

   Symbol *function = expr->u.function_call.function;
	Statement* function_def = function->u.function_instance.function_def;

   ExprChainNode *node;

   Dataflow **temps;

   if (function_def == NULL)
	{
		// Function call without function definition.
      glsl_compile_error(ERROR_CUSTOM, 21, expr->line_num, "maybe <%s> is a built-in function, and needs implementing?", function->name);
		return;
	}

   if (function_def->u.function_def.active)
   {
		// Function call without function definition.
      glsl_compile_error(ERROR_CUSTOM, 20, expr->line_num, "recursive call to function <%s>", function->name);
		return;
   }

   // Marshall input arguments.
   //
   // Note we do this first into a temporary buffer and then copy the temporaries into the formal parameter storage
   // so that we work correctly for nested invocations like min(a, min(b, c))

   node = expr->u.function_call.args->first;
   size = 0;
   for (i = 0; i < function->type->u.function_type.param_count; i++, node = node->next) {
      Symbol *formal = function->type->u.function_type.params[i];

      if (formal->u.var_instance.param_qual == PARAM_QUAL_IN || formal->u.var_instance.param_qual == PARAM_QUAL_INOUT) {
         size += formal->type->scalar_count;
      }
   }

   temps = stack_alloc_by_size(size);

   node = expr->u.function_call.args->first;
   size = 0;
   for (i = 0; i < function->type->u.function_type.param_count; i++, node = node->next) {
      Symbol *formal = function->type->u.function_type.params[i];

      if (formal->u.var_instance.param_qual == PARAM_QUAL_IN || formal->u.var_instance.param_qual == PARAM_QUAL_INOUT) {
         expr_calculate_dataflow(temps + size, node->expr);
         size += formal->type->scalar_count;
      }
   }

   node = expr->u.function_call.args->first;
   size = 0;
   for (i = 0; i < function->type->u.function_type.param_count; i++, node = node->next) {
      Symbol *formal = function->type->u.function_type.params[i];

      if (formal->u.var_instance.param_qual == PARAM_QUAL_IN || formal->u.var_instance.param_qual == PARAM_QUAL_INOUT) {
         memcpy(formal->u.var_instance.scalar_values, temps + size, formal->type->scalar_count * sizeof(Dataflow*));
         size += formal->type->scalar_count;
      }
   }

   stack_free();

   // Set return scalar values to be uninitialized.

   for (i = 0; i < expr->type->scalar_count; i++)
   {
      PrimitiveTypeIndex type_index = glsl_get_scalar_value_type_index(expr->type, i);

      switch (type_index)
      {
         case PRIM_BOOL:
            scalar_values[i] = g_BoolFalse;
            break;

         case PRIM_INT:
            scalar_values[i] = g_IntZero;
            break;

         case PRIM_FLOAT:
            scalar_values[i] = g_FloatZero;
            break;

         default:
            UNREACHABLE();
            return;
      }
   }

   // Evaluate function body.

   g_CurrentGuard = guard_construct_function(scalar_values, g_CurrentGuard);

   function_def->u.function_def.active = true;
   glsl_statement_accept_prefix(function_def, (void *)true, sprev_calculate_dataflow, NULL);
   function_def->u.function_def.active = false;

   g_CurrentGuard = g_CurrentGuard->next;

   // Marshall output arguments.

   node = expr->u.function_call.args->first;

   for (i = 0; i < function->type->u.function_type.param_count; i++, node = node->next) {
      Symbol *formal = function->type->u.function_type.params[i];
      Expr *actual = node->expr;

      if (formal->u.var_instance.param_qual == PARAM_QUAL_OUT || formal->u.var_instance.param_qual == PARAM_QUAL_INOUT) {
         Dataflow*** lvalue_scalar_values = (Dataflow ***)malloc_fast(formal->type->scalar_count * sizeof(Dataflow**));
         int i;

         // Evaluate lvalue.

         expr_evaluate_lvalue(actual, lvalue_scalar_values, 0, formal->type->scalar_count);

         // Copy to rvalue to lvalue.

         for (i = 0; i < formal->type->scalar_count; i++)
            *(lvalue_scalar_values[i]) = guard_scalar_assign(expr->line_num, formal->u.var_instance.scalar_values[i], *(lvalue_scalar_values[i]), is_lvalue_local(actual));
      }
   }

   vcos_assert(!node);
}

void glsl_init_texture_lookups()
{
   int i;

   num_tmu_lookups = 0;

   last_used_texture_unit = TEXTURE_UNIT_B;

   for (i = 0; i < TEXTURE_UNIT_COUNT; i++) {
      last_gadget_get_r[i] = NULL;

      next_sampler_index_offsets[i] = 0;
   }
}

Dataflow* glsl_dataflow_construct_threadswitch(int line_num, Dataflow* operand)
{
	Dataflow* dataflow;

	dataflow = (Dataflow *)malloc_fast(sizeof(Dataflow));
   DATAFLOW_LOG_ALLOC(DATAFLOW_THREADSWITCH);

	dataflow->line_num = line_num;
	dataflow->pass = DATAFLOW_PASS_INIT;
   dataflow->flavour = DATAFLOW_THREADSWITCH;
   dataflow->type_index = PRIM_VOID;

	glsl_dataflow_clear_chains(dataflow);

	// Add this node to the lists of dependents of the nodes it depends on.

   if (operand != NULL)
   {
      glsl_dataflow_add_iodependent(operand, dataflow);
      glsl_dataflow_add_iodependency(dataflow, operand);
   }

   // Set up the scheduler-specific fields

   init_backend_fields(dataflow, 0);

	return dataflow;
}

static INLINE void expr_calculate_dataflow_texture_lookup(Dataflow** scalar_values, Expr* expr)
{
   switch (g_ShaderFlavour)
   {
      case SHADER_VERTEX:
         {
            // Texture lookups in vertex shader not supported.
            glsl_compile_error(ERROR_CUSTOM, 26, g_LineNumber, NULL);
            return;
         }

      case SHADER_FRAGMENT:
         {
            Expr* sampler_expr;
            Dataflow* sampler_scalar_value;
            Expr* coord_expr;
            Dataflow* coord_scalar_values[COORD_DIM];
            Expr* bias_or_lod_expr;
            Dataflow* bias_or_lod_scalar_value;

            // Current texture unit.
            TextureUnit current_texture_unit;

            Dataflow* gadget_set_s;
            Dataflow* gadget_set_t;
            Dataflow* gadget_set_r;
            Dataflow* gadget_set_bias_or_lod;
#ifdef __VIDEOCORE4__
            Dataflow* gadget_get_rgba;
#else
            Dataflow* gadget_thread_switch;
#endif
            Dataflow* gadget_get_r;
            Dataflow* gadget_get_g;
            Dataflow* gadget_get_b;
            Dataflow* gadget_get_a;


            // Assert that we're returning a vec4 (i.e. rgba).
            vcos_assert(&primitiveTypes[PRIM_VEC4] == expr->type);

            // Gather args.
            sampler_expr = expr->u.intrinsic.args->first->expr;
            coord_expr = expr->u.intrinsic.args->first->next->expr;
            bias_or_lod_expr = expr->u.intrinsic.args->first->next->next->expr;
            switch (expr->flavour)
            {
               case EXPR_INTRINSIC_TEXTURE_2D_BIAS:
                  vcos_assert(&primitiveTypes[PRIM_SAMPLER2D] == sampler_expr->type);
                  vcos_assert(&primitiveTypes[PRIM_VEC2] == coord_expr->type);
                  vcos_assert(&primitiveTypes[PRIM_FLOAT] == bias_or_lod_expr->type);
                  texture_type = TEXTURE_2D;
                  lookup_type = LOOKUP_BIAS;
                  break;

               case EXPR_INTRINSIC_TEXTURE_2D_LOD:
                  vcos_assert(&primitiveTypes[PRIM_SAMPLER2D] == sampler_expr->type);
                  vcos_assert(&primitiveTypes[PRIM_VEC2] == coord_expr->type);
                  vcos_assert(&primitiveTypes[PRIM_FLOAT] == bias_or_lod_expr->type);
                  texture_type = TEXTURE_2D;
                  lookup_type = LOOKUP_LOD;
                  break;

               case EXPR_INTRINSIC_TEXTURE_CUBE_BIAS:
                  vcos_assert(&primitiveTypes[PRIM_SAMPLERCUBE] == sampler_expr->type);
                  vcos_assert(&primitiveTypes[PRIM_VEC3] == coord_expr->type);
                  vcos_assert(&primitiveTypes[PRIM_FLOAT] == bias_or_lod_expr->type);
                  texture_type = TEXTURE_CUBE;
                  lookup_type = LOOKUP_BIAS;
                  break;

               case EXPR_INTRINSIC_TEXTURE_CUBE_LOD:
                  vcos_assert(&primitiveTypes[PRIM_SAMPLERCUBE] == sampler_expr->type);
                  vcos_assert(&primitiveTypes[PRIM_VEC3] == coord_expr->type);
                  vcos_assert(&primitiveTypes[PRIM_FLOAT] == bias_or_lod_expr->type);
                  texture_type = TEXTURE_CUBE;
                  lookup_type = LOOKUP_LOD;
                  break;

               default:
                  UNREACHABLE();
                  return;
            }

            // Recurse on args.
            expr_calculate_dataflow(&sampler_scalar_value, sampler_expr);
            expr_calculate_dataflow(coord_scalar_values, coord_expr);
            expr_calculate_dataflow(&bias_or_lod_scalar_value, bias_or_lod_expr);

            current_texture_unit = (TextureUnit)((last_used_texture_unit + 1) % TEXTURE_UNIT_COUNT);

            // Allocate this sampler to a texture unit, if it hasn't been done already.
            if (SAMPLER_LOCATION_UNDEFINED == sampler_scalar_value->u.const_sampler.location)
            {

               if (next_sampler_index_offsets[current_texture_unit] >= SLANG_MAX_NUM_FSAMPLERS)
               {
                  // Too many samplers.
                  glsl_compile_error(ERROR_CUSTOM, 27, sampler_scalar_value->line_num, NULL);
                  return;
               }

               // Allocate.
               sampler_scalar_value->u.const_sampler.location = current_texture_unit + next_sampler_index_offsets[current_texture_unit];

               // Mark this sampler index offset as taken.
               next_sampler_index_offsets[current_texture_unit]++;
            }

            num_tmu_lookups++;
            if (num_tmu_lookups >= MAX_TMU_LOOKUPS) {
               /* TODO: This doesn't seem to generate a useful line number */
               glsl_compile_error(ERROR_CUSTOM, 8, sampler_scalar_value->line_num, NULL);
            }

            // Create gadget.
            gadget_set_t = glsl_dataflow_construct_texture_lookup_set(
               expr->line_num,
               DATAFLOW_TEX_SET_COORD_T,
               sampler_scalar_value,
               coord_scalar_values[COORD_T],
               last_gadget_get_r[current_texture_unit],
               NULL,
               NULL);
            switch (texture_type)
            {
               case TEXTURE_2D:
                  gadget_set_r = NULL;
                  break;

               case TEXTURE_CUBE:
                  gadget_set_r = glsl_dataflow_construct_texture_lookup_set(
                     expr->line_num,
                     DATAFLOW_TEX_SET_COORD_R,
                     sampler_scalar_value,
                     coord_scalar_values[COORD_R],
                     last_gadget_get_r[current_texture_unit],
                     NULL,
                     NULL);
                  break;

               default:
                  UNREACHABLE();
                  return;
            }
            switch (lookup_type)
            {
               case LOOKUP_BIAS:
                  if (DATAFLOW_CONST_FLOAT == bias_or_lod_scalar_value->flavour
                     && CONST_FLOAT_ZERO == bias_or_lod_scalar_value->u.const_float.value)
                  {
                     gadget_set_bias_or_lod = NULL;
                  }
                  else
                  {
                     gadget_set_bias_or_lod = glsl_dataflow_construct_texture_lookup_set(
                        expr->line_num,
                        DATAFLOW_TEX_SET_BIAS,
                        sampler_scalar_value,
                        bias_or_lod_scalar_value,
                        last_gadget_get_r[current_texture_unit],
                        NULL,
                        NULL);
                  }
                  break;

               case LOOKUP_LOD:
                  gadget_set_bias_or_lod = glsl_dataflow_construct_texture_lookup_set(
                     expr->line_num,
                     DATAFLOW_TEX_SET_LOD,
                     sampler_scalar_value,
                     bias_or_lod_scalar_value,
                     last_gadget_get_r[current_texture_unit],
                     NULL,
                     NULL);
                  break;

               default:
                  UNREACHABLE();
                  return;
            }
            gadget_set_s = glsl_dataflow_construct_texture_lookup_set(
               expr->line_num,
               DATAFLOW_TEX_SET_COORD_S,
               sampler_scalar_value,
               coord_scalar_values[COORD_S],
               gadget_set_bias_or_lod,
               gadget_set_t,
               gadget_set_r);
#ifdef __VIDEOCORE4__
            //TODO: thread switch
            gadget_get_rgba = glsl_dataflow_construct_texture_lookup_get(
               expr->line_num,
               DATAFLOW_TEX_GET_CMP_R,
               sampler_scalar_value,
               gadget_set_s,
               NULL,
               NULL);

            gadget_get_r = glsl_dataflow_construct_unpack_placeholder(
               expr->line_num,
               DATAFLOW_UNPACK_PLACEHOLDER_R,
               gadget_get_rgba,
               sampler_scalar_value);
            gadget_get_g = glsl_dataflow_construct_unpack(
               expr->line_num,
               DATAFLOW_UNPACK_COL_G,
               gadget_get_rgba);
            gadget_get_b = glsl_dataflow_construct_unpack_placeholder(
               expr->line_num,
               DATAFLOW_UNPACK_PLACEHOLDER_B,
               gadget_get_rgba,
               sampler_scalar_value);
            gadget_get_a = glsl_dataflow_construct_unpack(
               expr->line_num,
               DATAFLOW_UNPACK_COL_A,
               gadget_get_rgba);

            // Set return values.
            scalar_values[0] = gadget_get_r;
            scalar_values[1] = gadget_get_g;
            scalar_values[2] = gadget_get_b;
            scalar_values[3] = gadget_get_a;

            // Finally, save dependencies for next lookup.
            //last_gadget_get_r[current_texture_unit] = gadget_get_rgba;
            //last_used_texture_unit = current_texture_unit;
#else
            gadget_thread_switch = glsl_dataflow_construct_threadswitch(
               expr->line_num,
               gadget_set_s);
            gadget_get_g = glsl_dataflow_construct_texture_lookup_get(
               expr->line_num,
               DATAFLOW_TEX_GET_CMP_G,
               sampler_scalar_value,
               gadget_thread_switch,
               NULL,
               NULL);
            gadget_get_b = glsl_dataflow_construct_texture_lookup_get(
               expr->line_num,
               DATAFLOW_TEX_GET_CMP_B,
               sampler_scalar_value,
               gadget_thread_switch,
               NULL,
               NULL);
            gadget_get_a = glsl_dataflow_construct_texture_lookup_get(
               expr->line_num,
               DATAFLOW_TEX_GET_CMP_A,
               sampler_scalar_value,
               gadget_thread_switch,
               NULL,
               NULL);
            gadget_get_r = glsl_dataflow_construct_texture_lookup_get(
               expr->line_num,
               DATAFLOW_TEX_GET_CMP_R,
               sampler_scalar_value,
               gadget_get_g,
               gadget_get_b,
               gadget_get_a);

            // Set return values.
            scalar_values[0] = gadget_get_r;
            scalar_values[1] = gadget_get_g;
            scalar_values[2] = gadget_get_b;
            scalar_values[3] = gadget_get_a;

            // Finally, save dependencies for next lookup.
            last_gadget_get_r[current_texture_unit] = gadget_get_r;
            last_used_texture_unit = current_texture_unit;
#endif

            return;
         }

      default:
         UNREACHABLE();
         return;
   }
}


//
// Dataflow calculation.
//

// Writes dataflow graph pointers for expr to scalar_values.
// scalar_values is array of Dataflow* with expr->type->scalar_count members.
static void expr_calculate_dataflow(Dataflow** scalar_values, Expr* expr)
{
   STACK_CHECK();

   vcos_assert(scalar_values);
   vcos_assert(expr->type->scalar_count <= STACK_MAX_SCALAR_COUNT);

   if (expr->compile_time_value)
   {
      // Constant expression.
      expr_calculate_dataflow_compile_time_value(scalar_values, expr->line_num, expr->compile_time_value, expr->type);
      return;
   }
   else
   {
      // Non-constant expression.
      switch (expr->flavour)
      {
	      case EXPR_VALUE:
            // There should not be any of these for which expr->compile_time_value is NULL.
            UNREACHABLE();
            return;

	      case EXPR_FUNCTION_CALL:
            expr_calculate_dataflow_function_call(scalar_values, expr);
		      return;

	      case EXPR_FIELD_SELECTOR_SWIZZLE:
            expr_calculate_dataflow_field_selector_swizzle(scalar_values, expr);
            return;

	      case EXPR_POST_INC:
	      case EXPR_POST_DEC:
	      case EXPR_PRE_INC:
	      case EXPR_PRE_DEC:
            expr_calculate_dataflow_affix(scalar_values, expr);
            return;

	      case EXPR_ASSIGN:
            expr_calculate_dataflow_assign_op(scalar_values, expr);
		      return;

	      case EXPR_SEQUENCE:
            expr_calculate_dataflow_sequence(scalar_values, expr);
		      return;

	      case EXPR_INSTANCE:
            expr_calculate_dataflow_instance(scalar_values, expr);
            return;

	      case EXPR_SUBSCRIPT:
            expr_calculate_dataflow_subscript(scalar_values, expr);
            return;

	      case EXPR_PRIM_CONSTRUCTOR_CALL:
            expr_calculate_dataflow_prim_constructor_call(scalar_values, expr);
            return;

	      case EXPR_TYPE_CONSTRUCTOR_CALL:
            expr_calculate_dataflow_type_constructor_call(scalar_values, expr);
            return;

	      case EXPR_FIELD_SELECTOR_STRUCT:
            expr_calculate_dataflow_field_selector_struct(scalar_values, expr);
            return;

	      case EXPR_ARITH_NEGATE:
         case EXPR_LOGICAL_NOT:
            expr_calculate_dataflow_unary_op_common(scalar_values, expr);
            return;

	      case EXPR_MUL:
	      case EXPR_ADD:
	      case EXPR_SUB:
            expr_calculate_dataflow_binary_op_arithmetic(scalar_values, expr);
            return;

	      case EXPR_DIV:
            expr_calculate_dataflow_binary_op_divide(scalar_values, expr);
            return;

	      case EXPR_LESS_THAN:
	      case EXPR_LESS_THAN_EQUAL:
	      case EXPR_GREATER_THAN:
	      case EXPR_GREATER_THAN_EQUAL:
            expr_calculate_dataflow_binary_op_common(scalar_values, expr);
            return;

	      case EXPR_EQUAL:
	      case EXPR_NOT_EQUAL:
            expr_calculate_dataflow_binary_op_equality(scalar_values, expr);
            return;

	      case EXPR_LOGICAL_AND:
	      case EXPR_LOGICAL_OR:
            expr_calculate_dataflow_binary_op_short_circuit(scalar_values, expr);
            return;

	      case EXPR_LOGICAL_XOR:
            expr_calculate_dataflow_binary_op_common(scalar_values, expr);
            return;

	      case EXPR_CONDITIONAL:
            expr_calculate_dataflow_cond_op(scalar_values, expr);
            return;

         case EXPR_INTRINSIC_TEXTURE_2D_BIAS:
         case EXPR_INTRINSIC_TEXTURE_2D_LOD:
         case EXPR_INTRINSIC_TEXTURE_CUBE_BIAS:
         case EXPR_INTRINSIC_TEXTURE_CUBE_LOD:
            expr_calculate_dataflow_texture_lookup(scalar_values, expr);
            return;

         case EXPR_INTRINSIC_RSQRT:
         case EXPR_INTRINSIC_RCP:
         case EXPR_INTRINSIC_LOG2:
         case EXPR_INTRINSIC_EXP2:
         case EXPR_INTRINSIC_CEIL:
         case EXPR_INTRINSIC_FLOOR:
         case EXPR_INTRINSIC_SIGN:
         case EXPR_INTRINSIC_TRUNC:
         case EXPR_INTRINSIC_NEAREST:
            expr_calculate_dataflow_unary_op_common(scalar_values, expr);
            return;

         case EXPR_INTRINSIC_MIN:
         case EXPR_INTRINSIC_MAX:
            expr_calculate_dataflow_binary_op_common(scalar_values, expr);
            return;

	      default:
		      UNREACHABLE();
		      return;
      }
   }
}

void glsl_dataflow_priority_queue_init(DataflowPriorityQueue* queue, int size)
{
   queue->size = size;
   queue->used = 0;

   queue->nodes = (Dataflow **)malloc_fast(size * sizeof(Dataflow *));
}

static INLINE bool compare(const Dataflow *d0, const Dataflow *d1)
{
   return d1->delay > d0->delay || (d1->delay == d0->delay && d1->etime < d0->etime);
}

#define COMPARE(d0, d1) ((d1)->delay > (d0)->delay || (((d1)->delay == (d0)->delay) && ((d1)->etime < (d0)->etime)))

static void siftDown(DataflowPriorityQueue *queue, int start, int end)
{
   int root = start;

   while (root * 2 + 1 <= end) {
      int child = (root << 1) + 1;

      if (child + 1 <= end && COMPARE(queue->nodes[child], queue->nodes[child + 1]))
          child++;

      if (COMPARE(queue->nodes[root], queue->nodes[child])) {
         Dataflow *temp = queue->nodes[root];
         queue->nodes[root] = queue->nodes[child];
         queue->nodes[child] = temp;

         root = child;
      } else
         return;
   }
}

static void siftUp(DataflowPriorityQueue *queue, int root)
{
   while (root > 0) {
      int parent = (root - 1) >> 1;

      if (compare(queue->nodes[parent], queue->nodes[root])) {
         Dataflow *temp = queue->nodes[root];
         queue->nodes[root] = queue->nodes[parent];
         queue->nodes[parent] = temp;

         root = parent;
      } else
         return;
   }
}

void glsl_dataflow_priority_queue_heapify(DataflowPriorityQueue* queue)
{
   int start = (queue->used - 2) >> 1;

   while (start >= 0) {
      siftDown(queue, start, queue->used - 1);
      start--;
   }
}

void glsl_dataflow_priority_queue_push(DataflowPriorityQueue* queue, Dataflow *node)
{
   vcos_assert(queue->used < queue->size);

   queue->nodes[queue->used] = node;

   siftUp(queue, queue->used++);
}

Dataflow *glsl_dataflow_priority_queue_pop(DataflowPriorityQueue* queue)
{
   if (queue->used == 0)
      return NULL;
   else {
      Dataflow *result = queue->nodes[0];

      queue->nodes[0] = queue->nodes[--queue->used];

      siftDown(queue, 0, queue->used - 1);

      return result;
   }
}

#ifdef __VIDEOCORE4__
#include "middleware/khronos/common/khrn_mem.h"

typedef struct
{
   Dataflow *dependent;
   Dataflow **inputs;
   uint32_t num_inputs;
   MEM_HANDLE_T mh_out_blob;
   uint32_t alloc_amount;
   char *in_offset;
   bool revert;
   DataflowFlavour input_flavour;
#if GL_EXT_texture_format_BGRA8888
   bool *texture_rb_swap;
#endif
} GLSL_COPY_CONTEXT_T;

static void *stuff_alloc(GLSL_COPY_CONTEXT_T *stuff, uint32_t size)
{
   if (stuff->mh_out_blob == MEM_INVALID_HANDLE)
   {
      return malloc_fast(size);
   }
   else
   {
      void *result;
      if (stuff->alloc_amount + size > mem_get_size(stuff->mh_out_blob))
      {
         verify(mem_resize(stuff->mh_out_blob, mem_get_size(stuff->mh_out_blob) + 1024));   /* TODO: out of memory */
      }
      vcos_assert(stuff->alloc_amount + size <= mem_get_size(stuff->mh_out_blob));
      result = (void *)stuff->alloc_amount;
      stuff->alloc_amount += size;
      return result;
   }
}

static void *stuff_out_lock(GLSL_COPY_CONTEXT_T *stuff, void *handle)
{
   if (stuff->mh_out_blob == MEM_INVALID_HANDLE)
      return handle;
   else
      return ((char *)mem_lock(stuff->mh_out_blob)) + (size_t)handle;
}

static void stuff_out_unlock(GLSL_COPY_CONTEXT_T *stuff, void *handle)
{
   UNUSED(handle);

   if (stuff->mh_out_blob != MEM_INVALID_HANDLE)
      mem_unlock(stuff->mh_out_blob);
}

static void *stuff_in_translate(GLSL_COPY_CONTEXT_T *stuff, void *ptr)
{
   return ptr ? (stuff->in_offset + (size_t)ptr) : 0;
}

static void stuff_chain_append(GLSL_COPY_CONTEXT_T *stuff, DataflowChain *chain, Dataflow *dataflow)
{
	void* handle = stuff_alloc(stuff, sizeof(DataflowChainNode));
   DataflowChainNode *node = (DataflowChainNode *)stuff_out_lock(stuff, handle);

	node->dataflow = dataflow;
	node->unlinked = false;
	node->prev = chain->last;
	node->next = NULL;

   stuff_out_unlock(stuff, handle);

	if (!chain->first) chain->first = handle;
	if (chain->last)
   {
      ((DataflowChainNode *)stuff_out_lock(stuff, chain->last))->next = handle;
      stuff_out_unlock(stuff, chain->last);
   }
	chain->last = handle;
	chain->count++;
}

static Dataflow *copy(GLSL_COPY_CONTEXT_T *stuff, Dataflow *dataflow_in)
{
   Dataflow *result;
   Dataflow *dataflow;

   STACK_CHECK();

   dataflow = (Dataflow *)stuff_in_translate(stuff, dataflow_in);

   vcos_assert(dataflow);

   if (dataflow->flavour == stuff->input_flavour)
   {
      vcos_assert(dataflow->flavour == DATAFLOW_ATTRIBUTE || dataflow->flavour == DATAFLOW_VARYING);
      vcos_assert((uint32_t)dataflow->u.linkable_value.row < stuff->num_inputs);
      result = stuff->inputs[dataflow->u.linkable_value.row];
      vcos_assert(result);
   }
   else if (dataflow->copy && !stuff->revert)
   {
      vcos_assert(dataflow->copy != (Dataflow *)~0);
      result = dataflow->copy;
   }
   else if (!dataflow->copy && stuff->revert)
      return NULL;
   else
   {
      Dataflow df;
      DataflowChainNode *node;
      Dataflow *olddep;

      if (!stuff->revert) dataflow->copy = (Dataflow *)~0;

      df.flavour = dataflow->flavour;
	   df.line_num = dataflow->line_num;
	   df.pass = DATAFLOW_PASS_INIT;
	   df.type_index = dataflow->type_index;
      df.etime = dataflow->etime;     /* TODO: this is actually bool rep */
      glsl_dataflow_clear_chains(&df);

      result = (Dataflow *)stuff_alloc(stuff, sizeof(Dataflow));

      olddep = stuff->dependent;
      stuff->dependent = result;

      switch (dataflow->flavour)
      {
      case DATAFLOW_CONST_BOOL:
         df.u.const_bool.value = dataflow->u.const_bool.value;
         break;
      case DATAFLOW_CONST_FLOAT:
         df.u.const_float.value = dataflow->u.const_float.value;
         break;
      case DATAFLOW_CONST_SAMPLER:
         df.u.const_sampler.location = dataflow->u.const_sampler.location;
         df.u.const_sampler.name = NULL;
         break;
      case DATAFLOW_UNIFORM:
      case DATAFLOW_ATTRIBUTE:
      case DATAFLOW_VARYING:
         df.u.linkable_value.row = dataflow->u.linkable_value.row;
         df.u.linkable_value.name = NULL;
         break;
      case DATAFLOW_CONST_INT:
         df.u.const_int.value = dataflow->u.const_int.value;
         break;
      case DATAFLOW_UNIFORM_OFFSET:
         df.u.linkable_value_offset.linkable_value = copy(stuff, dataflow->u.linkable_value_offset.linkable_value);
         df.u.linkable_value_offset.offset = copy(stuff, dataflow->u.linkable_value_offset.offset);
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
         df.u.unary_op.operand = copy(stuff, dataflow->u.unary_op.operand);
         break;

      case DATAFLOW_UNPACK_PLACEHOLDER_R:
      case DATAFLOW_UNPACK_PLACEHOLDER_B:
         if (stuff->texture_rb_swap == NULL || stuff->revert)
         {
            df.u.binary_op.left = copy(stuff, dataflow->u.binary_op.left);
            df.u.binary_op.right = copy(stuff, dataflow->u.binary_op.right);
         }
         else
         {
            bool swap;
            Dataflow *sampler = copy(stuff, dataflow->u.binary_op.right);
            int loc = sampler->u.const_sampler.location;

            vcos_assert(loc >= 0 && loc < 8);

            swap = stuff->texture_rb_swap[loc];
            if ((dataflow->flavour == DATAFLOW_UNPACK_PLACEHOLDER_R) ^ swap)
               df.flavour = DATAFLOW_UNPACK_COL_R;
            else
               df.flavour = DATAFLOW_UNPACK_COL_B;
            df.u.unary_op.operand = copy(stuff, dataflow->u.binary_op.left);
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
         df.u.binary_op.left = copy(stuff, dataflow->u.binary_op.left);
         df.u.binary_op.right = copy(stuff, dataflow->u.binary_op.right);
         break;

      case DATAFLOW_CONDITIONAL:
         df.u.cond_op.cond = copy(stuff, dataflow->u.cond_op.cond);
         df.u.cond_op.true_value = copy(stuff, dataflow->u.cond_op.true_value);
         df.u.cond_op.false_value = copy(stuff, dataflow->u.cond_op.false_value);
         break;

      case DATAFLOW_TEX_SET_COORD_S:
      case DATAFLOW_TEX_SET_COORD_T:
      case DATAFLOW_TEX_SET_COORD_R:
      case DATAFLOW_TEX_SET_LOD:
      case DATAFLOW_TEX_SET_BIAS:
#ifndef __BCM2708A0__
      case DATAFLOW_TEX_SET_DIRECT:
#endif
         if (dataflow->u.texture_lookup_set.sampler)
            df.u.texture_lookup_set.sampler = copy(stuff, dataflow->u.texture_lookup_set.sampler);
         else
            df.u.texture_lookup_set.sampler = NULL;
         df.u.texture_lookup_set.param = copy(stuff, dataflow->u.texture_lookup_set.param);
         break;

      case DATAFLOW_TEX_GET_CMP_R:
      case DATAFLOW_TEX_GET_CMP_G:
      case DATAFLOW_TEX_GET_CMP_B:
      case DATAFLOW_TEX_GET_CMP_A:
         if (dataflow->u.texture_lookup_get.sampler)
            df.u.texture_lookup_get.sampler = copy(stuff, dataflow->u.texture_lookup_get.sampler);
         else
            df.u.texture_lookup_get.sampler = NULL;
         break;

      case DATAFLOW_FRAG_SET_R:
      case DATAFLOW_FRAG_SET_G:
      case DATAFLOW_FRAG_SET_B:
      case DATAFLOW_FRAG_SET_A:
      case DATAFLOW_FRAG_SUBMIT_STENCIL:
      case DATAFLOW_FRAG_SUBMIT_Z:
      case DATAFLOW_FRAG_SUBMIT_MS:
      case DATAFLOW_FRAG_SUBMIT_ALL:
      case DATAFLOW_FRAG_SUBMIT_R0:
      case DATAFLOW_FRAG_SUBMIT_R1:
      case DATAFLOW_FRAG_SUBMIT_R2:
      case DATAFLOW_FRAG_SUBMIT_R3:
      case DATAFLOW_TMU_SWAP:
         df.u.fragment_set.param = copy(stuff, dataflow->u.fragment_set.param);
         df.u.fragment_set.discard = dataflow->u.fragment_set.discard ? copy(stuff, dataflow->u.fragment_set.discard) : NULL;
         df.u.fragment_set.redundant = dataflow->u.fragment_set.redundant;
         break;

      case DATAFLOW_VERTEX_SET:
      case DATAFLOW_VPM_READ_SETUP:
      case DATAFLOW_VPM_WRITE_SETUP:
         df.u.vertex_set.param = copy(stuff, dataflow->u.vertex_set.param);
         break;

      case DATAFLOW_PACK_COL_R:
      case DATAFLOW_PACK_COL_G:
      case DATAFLOW_PACK_COL_B:
      case DATAFLOW_PACK_COL_A:
      case DATAFLOW_PACK_16A:
      case DATAFLOW_PACK_16B:
      case DATAFLOW_PACK_FB_R:
      case DATAFLOW_PACK_FB_B:
         df.u.pack.operand = copy(stuff, dataflow->u.pack.operand);
         df.u.pack.background = copy(stuff, dataflow->u.pack.background);
         break;

#ifdef __BCM2708A0__
      case DATAFLOW_INDEXED_UNIFORM_SAMPLER:
#else
      case DATAFLOW_UNIFORM_ADDRESS:
#endif
         df.u.indexed_uniform_sampler.uniform = copy(stuff, dataflow->u.indexed_uniform_sampler.uniform);
         df.u.indexed_uniform_sampler.size = dataflow->u.indexed_uniform_sampler.size;
         break;

      case DATAFLOW_THREADSWITCH:
      case DATAFLOW_FRAG_GET_X:
	   case DATAFLOW_FRAG_GET_Y:
	   case DATAFLOW_FRAG_GET_Z:
	   case DATAFLOW_FRAG_GET_W:
	   case DATAFLOW_FRAG_GET_PC_X:
	   case DATAFLOW_FRAG_GET_PC_Y:
      case DATAFLOW_FRAG_GET_FF:
      case DATAFLOW_VARYING_C:
      case DATAFLOW_FRAG_GET_COL:
      case DATAFLOW_SCOREBOARD_WAIT:
         /* No extra fields */
         break;

      default:
         UNREACHABLE();
      }

      stuff->dependent = NULL;   /* Do this before visiting iodependencies. Don't want to add anything as a dependent of iodependencies. */

      node = (DataflowChainNode *)stuff_in_translate(stuff, dataflow->iodependencies.first);
      while (node)
      {
         Dataflow *out_iodep = copy(stuff, node->dataflow);

         if (!stuff->revert)
         {
            stuff_chain_append(stuff, &df.iodependencies, out_iodep);

            if (stuff->mh_out_blob == MEM_INVALID_HANDLE)
            {
               glsl_dataflow_chain_append(&out_iodep->iodependents, result);
            }
         }

         node = (DataflowChainNode *)stuff_in_translate(stuff, node->next);
      }

      if (stuff->revert)
      {
         vcos_assert(dataflow->copy != NULL);
         dataflow->copy = NULL;
         return NULL;
      }

      vcos_assert(dataflow->copy == (Dataflow *)~0);
      dataflow->copy = result;

      init_backend_fields(&df, 0);
      memcpy(stuff_out_lock(stuff, result), &df, sizeof(Dataflow));
      stuff_out_unlock(stuff, result);

      stuff->dependent = olddep;
   }

   if (stuff->mh_out_blob == MEM_INVALID_HANDLE && stuff->dependent)
   {
      glsl_dataflow_chain_append(&result->dependents, stuff->dependent);
   }

   return result;
}

MEM_HANDLE_T glsl_dataflow_copy_to_relocatable(uint32_t count, Dataflow **dataflow_out, Dataflow **dataflow_in, void *in_offset)
{
   GLSL_COPY_CONTEXT_T stuff;
   uint32_t i;

   stuff.dependent = NULL;
   stuff.inputs = NULL;
   stuff.num_inputs = 0;
   stuff.input_flavour = DATAFLOW_FLAVOUR_COUNT;
   stuff.mh_out_blob = mem_alloc_ex(1024, 4, MEM_FLAG_RESIZEABLE|MEM_FLAG_HINT_GROW, "GLSL_COPY_CONTEXT_T.mh_blob", MEM_COMPACT_DISCARD);

   if (stuff.mh_out_blob == MEM_INVALID_HANDLE)
      return MEM_INVALID_HANDLE;

   stuff.alloc_amount = 4;    /* waste some memory to avoid NULL being returned */
   stuff.in_offset = in_offset;
   stuff.revert = false;
   stuff.texture_rb_swap = NULL;

   for (i = 0; i < count; i++)
      dataflow_out[i] = copy(&stuff, dataflow_in[i]);

   stuff.revert = true;
   for (i = 0; i < count; i++)
      copy(&stuff, dataflow_in[i]);

   return stuff.mh_out_blob;
}

void glsl_dataflow_copy(uint32_t count, Dataflow **dataflow_out, Dataflow **dataflow_in, void *in_offset, Dataflow **inputs, uint32_t num_inputs, DataflowFlavour input_flavour, bool *texture_rb_swap)
{
   GLSL_COPY_CONTEXT_T stuff;
   uint32_t i;

   stuff.dependent = NULL;
   stuff.inputs = inputs;
   stuff.num_inputs = num_inputs;
   stuff.input_flavour = input_flavour;
   stuff.mh_out_blob = MEM_INVALID_HANDLE;
   stuff.alloc_amount = 0;
   stuff.in_offset = in_offset;
   stuff.revert = false;
   stuff.texture_rb_swap = texture_rb_swap;

   for (i = 0; i < count; i++)
   {
      if (dataflow_in[i] != NULL)
         dataflow_out[i] = copy(&stuff, dataflow_in[i]);
      else
         dataflow_out[i] = NULL;
   }

   stuff.revert = true;
   for (i = 0; i < count; i++)
      copy(&stuff, dataflow_in[i]);
}

#endif
