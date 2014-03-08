/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_DATAFLOW_H
#define GLSL_DATAFLOW_H

#include "middleware/khronos/glsl/glsl_symbols.h"
#include "middleware/khronos/glsl/glsl_ast.h"
#include "middleware/khronos/glsl/glsl_globals.h"
#include "middleware/khronos/glsl/glsl_const_types.h"
#include "middleware/khronos/glsl/glsl_map.h"
#include "middleware/khronos/glsl/glsl_fastmem.h"

#define MAX_TMU_LOOKUPS 96
/* We require num < MAX, not <=, so round down on word boundaries */
#define TMU_DEP_WORD_COUNT ( (MAX_TMU_LOOKUPS-1) / 32 + 1)

#ifdef DATAFLOW_LOG
extern void glsl_dataflow_log_init();
extern void glsl_dataflow_log_dump();
#endif

//
// Dataflow graph where all operations act on scalars.
//

#define DATAFLOW_PASS_INIT                0
#define SAMPLER_LOCATION_UNDEFINED        -1
#define SAMPLER_NAME_UNDEFINED            ""
#define LINKABLE_VALUE_ROW_UNDEFINED      -1
#define LINKABLE_VALUE_NAME_UNDEFINED     ""

// Dataflow chain structure, for efficient appending.
struct _DataflowChain
{
	DataflowChainNode* first;
	DataflowChainNode* last;
	int count;
};

struct _DataflowChainNode
{
	Dataflow* dataflow;
	DataflowChainNode* prev;
	DataflowChainNode* next;
	bool unlinked;
};

typedef bool (*DataflowFilter)(Dataflow* dataflow, void* data);

void glsl_dataflow_chain_init(DataflowChain* chain);
DataflowChain* glsl_dataflow_chain_append(DataflowChain* chain, Dataflow* dataflow);
DataflowChain* glsl_dataflow_chain_remove(DataflowChain *chain, Dataflow *dataflow);
DataflowChain* glsl_dataflow_chain_replace(DataflowChain *chain, Dataflow *dataflow_old, Dataflow *dataflow_new);
bool glsl_dataflow_chain_contains(DataflowChain *chain, Dataflow *dataflow);
// If node in chain, removes node from chain.
// Else, behaviour undefined.
DataflowChain* glsl_dataflow_chain_remove_node(DataflowChain* chain, DataflowChainNode* node);
DataflowChain *glsl_dataflow_chain_filter(DataflowChain *dst, DataflowChain *src, void *data, DataflowFilter filter);

typedef enum
{
	DATAFLOW_CONST_BOOL,
	DATAFLOW_CONST_INT,
	DATAFLOW_CONST_FLOAT,
   DATAFLOW_CONST_SAMPLER,
	DATAFLOW_UNIFORM,
	DATAFLOW_ATTRIBUTE,
	DATAFLOW_VARYING,
   DATAFLOW_UNIFORM_OFFSET,
	DATAFLOW_ARITH_NEGATE,
	DATAFLOW_LOGICAL_NOT,
	DATAFLOW_MUL,
	DATAFLOW_ADD,
	DATAFLOW_SUB,
   DATAFLOW_RSUB,
	DATAFLOW_LESS_THAN,
	DATAFLOW_LESS_THAN_EQUAL,
	DATAFLOW_GREATER_THAN,
	DATAFLOW_GREATER_THAN_EQUAL,
	DATAFLOW_EQUAL,
	DATAFLOW_NOT_EQUAL,
	DATAFLOW_LOGICAL_AND,
	DATAFLOW_LOGICAL_XOR,
	DATAFLOW_LOGICAL_OR,
	DATAFLOW_CONDITIONAL,

	// Intrinsic, hardware supported operators.
	DATAFLOW_INTRINSIC_FLOATTOINT_TRUNC,
	DATAFLOW_INTRINSIC_FLOATTOINT_NEAREST,
	DATAFLOW_INTRINSIC_FLOATTOBOOL,
	DATAFLOW_INTRINSIC_INTTOFLOAT,
	DATAFLOW_INTRINSIC_INTTOBOOL,
	DATAFLOW_INTRINSIC_BOOLTOINT,
	DATAFLOW_INTRINSIC_BOOLTOFLOAT,
	DATAFLOW_INTRINSIC_RSQRT,
	DATAFLOW_INTRINSIC_RCP,
	DATAFLOW_INTRINSIC_LOG2,
	DATAFLOW_INTRINSIC_EXP2,
   DATAFLOW_INTRINSIC_CEIL,
   DATAFLOW_INTRINSIC_FLOOR,
   DATAFLOW_INTRINSIC_SIGN,
	DATAFLOW_INTRINSIC_MIN,
	DATAFLOW_INTRINSIC_MAX,

   // Move
   DATAFLOW_MOV,

   // Internal coercion from integer result of floattoint to float version
   DATAFLOW_COERCE_TOFLOAT,
   // Internal coercion from float version to integer operand of linkable value offset
   DATAFLOW_COERCE_TOINT,

   // Thread switch. Only appears in schedule.
   DATAFLOW_THREADSWITCH,

	// Texture lookup operators.
	DATAFLOW_TEX_SET_COORD_S,
	DATAFLOW_TEX_SET_COORD_T,
	DATAFLOW_TEX_SET_COORD_R,
	DATAFLOW_TEX_SET_BIAS,
	DATAFLOW_TEX_SET_LOD,
	DATAFLOW_TEX_GET_CMP_R,
	DATAFLOW_TEX_GET_CMP_G,
	DATAFLOW_TEX_GET_CMP_B,
	DATAFLOW_TEX_GET_CMP_A,

   // Fragment coordinate retrieval.
   DATAFLOW_FRAG_GET_X,
   DATAFLOW_FRAG_GET_Y,
   DATAFLOW_FRAG_GET_Z,
   DATAFLOW_FRAG_GET_W,
   DATAFLOW_FRAG_GET_PC_X,
   DATAFLOW_FRAG_GET_PC_Y,
   DATAFLOW_FRAG_GET_FF,

   // Fragment result submission.
   DATAFLOW_FRAG_SET_R,
   DATAFLOW_FRAG_SET_G,
   DATAFLOW_FRAG_SET_B,
   DATAFLOW_FRAG_SET_A,

#ifdef __VIDEOCORE4__
   // ALU
   DATAFLOW_BITWISE_NOT,
   DATAFLOW_BITWISE_AND,
   DATAFLOW_BITWISE_OR,
   DATAFLOW_BITWISE_XOR,
   DATAFLOW_V8MULD,
   DATAFLOW_V8MIN,
   DATAFLOW_V8MAX,
   DATAFLOW_V8ADDS,
   DATAFLOW_V8SUBS,
   DATAFLOW_INTEGER_ADD,

   // Constant component of varying
   DATAFLOW_VARYING_C,

   // Fragment result submission.
   DATAFLOW_FRAG_GET_COL,
   DATAFLOW_FRAG_SUBMIT_STENCIL,
   DATAFLOW_FRAG_SUBMIT_Z,
   DATAFLOW_FRAG_SUBMIT_MS,
   DATAFLOW_FRAG_SUBMIT_ALL,

   DATAFLOW_FRAG_SUBMIT_R0,
   DATAFLOW_FRAG_SUBMIT_R1,
   DATAFLOW_FRAG_SUBMIT_R2,
   DATAFLOW_FRAG_SUBMIT_R3,

   // TMU configuration
   DATAFLOW_TMU_SWAP,

   // Texture lookup operators.
#ifndef __BCM2708A0__
	DATAFLOW_TEX_SET_DIRECT,
#endif

   // Vertex result submission.
   DATAFLOW_VERTEX_SET,
   DATAFLOW_VPM_READ_SETUP,
   DATAFLOW_VPM_WRITE_SETUP,

   // Packing.
   DATAFLOW_PACK_COL_R,
   DATAFLOW_PACK_COL_G,
   DATAFLOW_PACK_COL_B,
   DATAFLOW_PACK_COL_A,
   DATAFLOW_PACK_16A,
   DATAFLOW_PACK_16B,
   DATAFLOW_PACK_FB_R,
   DATAFLOW_PACK_FB_B,
   DATAFLOW_UNPACK_COL_R,
   DATAFLOW_UNPACK_COL_G,
   DATAFLOW_UNPACK_COL_B,
   DATAFLOW_UNPACK_COL_A,
   DATAFLOW_UNPACK_16A,
   DATAFLOW_UNPACK_16A_F,
   DATAFLOW_UNPACK_16B,
   DATAFLOW_UNPACK_16B_F,
   DATAFLOW_UNPACK_8A,
   DATAFLOW_UNPACK_8B,
   DATAFLOW_UNPACK_8C,
   DATAFLOW_UNPACK_8D,
   DATAFLOW_UNPACK_8R,
   DATAFLOW_UNPACK_FB_R,
   DATAFLOW_UNPACK_FB_B,

   DATAFLOW_UNPACK_PLACEHOLDER_R,
   DATAFLOW_UNPACK_PLACEHOLDER_B,

   // Stuff for boolean expressions
   DATAFLOW_SHIFT_RIGHT,
   DATAFLOW_LOGICAL_SHR,

   // Indexed uniforms
#ifdef __BCM2708A0__
   DATAFLOW_INDEXED_UNIFORM_SAMPLER,
#else
   DATAFLOW_UNIFORM_ADDRESS,
#endif

   // Signalling codes
   DATAFLOW_SCOREBOARD_WAIT,
#endif

	DATAFLOW_FLAVOUR_COUNT
} DataflowFlavour;

#define DATAFLOW_SLOT_SPM  0x00000000
#define DATAFLOW_SLOT_VRF  0x80000000
#define DATAFLOW_SLOT_MASK 0x7fffffff

typedef enum
{
   REPR_GENUINE,

   REPR_FCMP_LESS_THAN,
   REPR_FCMP_LESS_THAN_EQUAL,
   REPR_FCMP_GREATER_THAN,
   REPR_FCMP_GREATER_THAN_EQUAL,
   REPR_FCMP_EQUAL,
   REPR_FCMP_NOT_EQUAL
} Repr;

struct _Dataflow
{
	// The flavour of this dataflow node.
	DataflowFlavour flavour;

	// The source line number from which it was generated, or LINE_NUMBER_UNDEFINED.
	int line_num;

   // The most recent visitor pass to traverse this node.
   // Initialized to DATAFLOW_PASS_INIT.
   int pass;

	// Its type, which must be a primitive scalar.
   // - bool, int or float; or
   // - one of the sampler types; or
   // - void, for texture lookup set nodes.
	PrimitiveTypeIndex type_index;

	// A linked list of dataflow nodes that depend on this node.
	DataflowChain dependents;

	union
	{
		// DATAFLOW_CONST_BOOL
		struct
		{
			const_bool value;
		} const_bool;

		// DATAFLOW_CONST_FLOAT
		struct
		{
			const_float value;
		} const_float;

      // DATAFLOW_CONST_SAMPLER
		struct
		{
         // location set to SAMPLER_LOCATION_UNDEFINED before samplers have been resolved at link time.
         const_int location;
         // name set to SAMPLER_NAME_UNDEFINED similarly.
         const char* name;
      } const_sampler;

		// DATAFLOW_UNIFORM, DATAFLOW_ATTRIBUTE, DATAFLOW_VARYING
		struct
		{
         // row set to LINKABLE_VALUE_ROW_UNDEFINED before linkable values have been resolved at link time.
			const_int row;
         // name set to LINKABLE_VALUE_NAME_UNDEFINED similarly.
         const char* name;
		} linkable_value;

		// DATAFLOW_CONST_INT
		struct
		{
			const_int value;
		} const_int;

		// DATAFLOW_UNIFORM_OFFSET
		struct
		{
			Dataflow* linkable_value; // the first element in the array
			Dataflow* offset; // the index to access
		} linkable_value_offset;

		// DATAFLOW_ARITH_NEGATE, DATAFLOW_LOGICAL_NOT
      // DATAFLOW_INTRINSIC_FLOATTOINT_TRUNC, DATAFLOW_INTRINSIC_FLOATTOINT_NEAREST
      // DATAFLOW_INTRINSIC_FLOATTOBOOL
      // DATAFLOW_INTRINSIC_INTTOFLOAT, DATAFLOW_INTRINSIC_INTTOBOOL
      // DATAFLOW_INTRINSIC_BOOLTOINT, DATAFLOW_INTRINSIC_BOOLTOFLOAT
		// DATAFLOW_INTRINSIC_RSQRT, DATAFLOW_INTRINSIC_RCP, DATAFLOW_INTRINSIC_LOG2, DATAFLOW_INTRINSIC_EXP2
      // DATAFLOW_INTRINSIC_CEIL, DATAFLOW_INTRINSIC_FLOOR, DATAFLOW_INTRINSIC_SIGN
      // DATAFLOW_MOV, DATAFLOW_COERCE_TOFLOAT, DATAFLOW_COERCE_TOINT
		struct
		{
			Dataflow* operand;
		} unary_op;

		// DATAFLOW_MUL, DATAFLOW_ADD, DATAFLOW_SUB, DATAFLOW_RSUB,
		// DATAFLOW_LESS_THAN, DATAFLOW_LESS_THAN_EQUAL, DATAFLOW_GREATER_THAN, DATAFLOW_GREATER_THAN_EQUAL
		// DATAFLOW_EQUAL, DATAFLOW_NOT_EQUAL
		// DATAFLOW_LOGICAL_AND, DATAFLOW_LOGICAL_XOR, DATAFLOW_LOGICAL_OR
		// DATAFLOW_INTRINSIC_MIN, DATAFLOW_INTRINSIC_MAX
		struct
		{
			Dataflow* left;
			Dataflow* right;
		} binary_op;

		// DATAFLOW_CONDITIONAL
		struct
		{
			Dataflow* cond;
			Dataflow* true_value;
			Dataflow* false_value;
		} cond_op;

		// DATAFLOW_TEX_SET_COORD_S, DATAFLOW_TEX_SET_COORD_T, DATAFLOW_TEX_SET_COORD_R
		// DATAFLOW_TEX_SET_LOD DATAFLOW_TEX_SET_BIAS
		struct
		{
			Dataflow* sampler;
			Dataflow* param;
      } texture_lookup_set;

		// DATAFLOW_TEX_GET_CMP_R, DATAFLOW_TEX_GET_CMP_G, DATAFLOW_TEX_GET_CMP_B, DATAFLOW_TEX_GET_CMP_A
		struct
		{
			Dataflow* sampler;
		} texture_lookup_get;

      // DATAFLOW_FRAG_SET_R, DATAFLOW_FRAG_SET_G, DATAFLOW_FRAG_SET_B, DATAFLOW_FRAG_SET_A
		struct
		{
			Dataflow* param;
         Dataflow* discard;

         bool redundant;
      } fragment_set;

#ifdef __VIDEOCORE4__
      // DATAFLOW_VERTEX_SET, DATAFLOW_VPM_READ_SETUP, DATAFLOW_VPM_WRITE_SETUP
      struct
      {
         Dataflow* param;
      } vertex_set;

      // DATAFLOW_PACK_COL_R, DATAFLOW_PACK_COL_G, DATAFLOW_PACK_COL_B, DATAFLOW_PACK_COL_A, 
      // DATAFLOW_PACK_16A, DATAFLOW_PACK_16B
      // DATAFLOW_PACK_FB_R, DATAFLOW_PACK_FB_B
      struct
      {
         Dataflow *operand;
         Dataflow *background;
      } pack;

      // DATAFLOW_INDEXED_UNIFORM_SAMPLER (2708A0), DATAFLOW_UNIFORM_ADDRESS (2708B0)
      struct
      {
         Dataflow* uniform;
         uint32_t size;
      } indexed_uniform_sampler;

      // DATAFLOW_UNPACK_COL_R, DATAFLOW_UNPACK_COL_G, DATAFLOW_UNPACK_COL_B, DATAFLOW_UNPACK_COL_A
      // DATAFLOW_UNPACK_16A, DATAFLOW_UNPACK_16A_F, DATAFLOW_UNPACK_16B, DATAFLOW_UNPACK_16B_F
      // DATAFLOW_UNPACK_8A, DATAFLOW_UNPACK_8B, DATAFLOW_UNPACK_8C, DATAFLOW_UNPACK_8D
      // DATAFLOW_UNPACK_8R
      // DATAFLOW_UNPACK_FB_R, DATAFLOW_UNPACK_FB_B
      // DATAFLOW_BITWISE_NOT
      // use unary_op

      // DATAFLOW_BITWISE_AND, DATAFLOW_BITWISE_OR, DATAFLOW_BITWISE_XOR,
      // DATAFLOW_V8MULD, DATAFLOW_V8MIN, DATAFLOW_V8MAX, DATAFLOW_V8ADDS, DATAFLOW_V8SUBS,
      // DATAFLOW_INTEGER_ADD, DATAFLOW_SHIFT_RIGHT, DATAFLOW_LOGICAL_SHR,
      // DATAFLOW_UNPACK_PLACEHOLDER_R, DATAFLOW_UNPACK_PLACEHOLDER_B,
      // use binary_op

      /* DATAFLOW_FRAG_SUBMIT_STENCIL, DATAFLOW_FRAG_SUBMIT_Z, DATAFLOW_FRAG_SUBMIT_MS, DATAFLOW_FRAG_SUBMIT_ALL */
      /* DATAFLOW_FRAG_SUBMIT_R0, DATAFLOW_FRAG_SUBMIT_R1, DATAFLOW_FRAG_SUBMIT_R2, DATAFLOW_FRAG_SUBMIT_R3: */
      /* DATAFLOW_TMU_SWAP, */
      /* use fragment_set */

#endif
	} u;

   DataflowChain iodependents;
   DataflowChain iodependencies;

	// scheduler data

   int delay;
   int etime;
   int deps_remaining;

   bool live;

   int index;
   int phase;
#ifdef __VIDEOCORE4__
   uint32_t tmu_dependencies[TMU_DEP_WORD_COUNT];
   Dataflow *copy;
#endif

   // allocator data

   int temp;
   int slot;

   bool protect;

   // generator data

   Repr repr;
};

// Clears the list of dependents, iodependents and iodependencies
static INLINE void glsl_dataflow_clear_chains(Dataflow* dataflow)
{
	glsl_dataflow_chain_init(&dataflow->dependents);
	glsl_dataflow_chain_init(&dataflow->iodependents);
	glsl_dataflow_chain_init(&dataflow->iodependencies);
}

// Adds consumer to the list of dependents in the supplier.
static INLINE void glsl_dataflow_add_dependent(Dataflow* supplier, Dataflow* consumer)
{
	glsl_dataflow_chain_append(&supplier->dependents, consumer);
}

// Adds consumer to the list of io dependents in the supplier.
static INLINE void glsl_dataflow_add_iodependent(Dataflow* supplier, Dataflow* consumer)
{
	glsl_dataflow_chain_append(&supplier->iodependents, consumer);
}

// Adds consumer to the list of io dependencies in the consumer.
static INLINE void glsl_dataflow_add_iodependency(Dataflow* consumer, Dataflow* supplier)
{
	glsl_dataflow_chain_append(&consumer->iodependencies, supplier);
}

// On failure, these functions return NULL.
Dataflow* glsl_dataflow_construct_const_bool(int line_num, const_bool value);
Dataflow* glsl_dataflow_construct_const_int(int line_num, const_int value);
Dataflow* glsl_dataflow_construct_const_float(int line_num, const_float value);
Dataflow* glsl_dataflow_construct_const_sampler(int line_num, PrimitiveTypeIndex type_index);
Dataflow* glsl_dataflow_construct_linkable_value(int line_num, DataflowFlavour flavour, PrimitiveTypeIndex type_index);
Dataflow* glsl_dataflow_construct_linkable_value_offset(int line_num, DataflowFlavour flavour, PrimitiveTypeIndex type_index, Dataflow* linkable_value, Dataflow* offset);
Dataflow* glsl_dataflow_construct_unary_op(int line_num, DataflowFlavour flavour, Dataflow* operand);
Dataflow* glsl_dataflow_construct_binary_op(int line_num, DataflowFlavour flavour, Dataflow* left, Dataflow* right);
Dataflow* glsl_dataflow_construct_cond_op(int line_num, Dataflow* cond, Dataflow* true_value, Dataflow* false_value);
Dataflow* glsl_dataflow_construct_texture_lookup_set(int line_num, DataflowFlavour flavour, Dataflow* sampler, Dataflow* param, Dataflow* depends_on0, Dataflow* depends_on1, Dataflow* depends_on2);
Dataflow* glsl_dataflow_construct_texture_lookup_get(int line_num, DataflowFlavour flavour, Dataflow* sampler, Dataflow* depends_on0, Dataflow* depends_on1, Dataflow* depends_on2);
Dataflow* glsl_dataflow_construct_fragment_get(int line_num, DataflowFlavour flavour);
Dataflow* glsl_dataflow_construct_fragment_set(int line_num, DataflowFlavour flavour, Dataflow* param, Dataflow *discard);
Dataflow* glsl_dataflow_construct_threadswitch(int line_num, Dataflow* operand);
#ifdef __VIDEOCORE4__
Dataflow* glsl_dataflow_construct_vertex_set(int line_num, DataflowFlavour flavour, Dataflow* param, Dataflow* prev);
Dataflow* glsl_dataflow_construct_fragment_submit(int line_num, DataflowFlavour flavour, Dataflow* param, Dataflow* prev, Dataflow *discard);
Dataflow *glsl_dataflow_construct_pack(int line_num, DataflowFlavour flavour, Dataflow *operand, Dataflow *background);
Dataflow* glsl_dataflow_construct_pack_col(int line_num, Dataflow* red, Dataflow *green, Dataflow *blue, Dataflow *alpha);
Dataflow* glsl_dataflow_construct_pack_col_rgb(int line_num, Dataflow* red, Dataflow *green, Dataflow *blue);
Dataflow* glsl_dataflow_construct_pack_int16(int line_num, Dataflow* a, Dataflow *b);
Dataflow* glsl_dataflow_construct_varying_tree(int line_num, Dataflow* varying);
Dataflow* glsl_dataflow_construct_varying_non_perspective_tree(int line_num, Dataflow* varying);
Dataflow* glsl_dataflow_construct_unpack(int line_num, DataflowFlavour flavour, Dataflow* param);
Dataflow* glsl_dataflow_construct_unpack_placeholder(int line_num, DataflowFlavour flavour, Dataflow* param, Dataflow *sampler);
Dataflow* glsl_dataflow_construct_indexed_uniform_sampler(int line_num, Dataflow* uniform, uint32_t size);
Dataflow* glsl_dataflow_construct_scoreboard_wait(int line_num, Dataflow* operand);



#define BOOL_REP_BOOL   1    /* 0  = False, 1   = True,  Other = Invalid */
#define BOOL_REP_BOOL_N 2    /* 0  = True,  1   = False, Other = Invalid */
#define BOOL_REP_NEG    3    /* <0 = True,  >=0 = False */
#define BOOL_REP_NEG_N  4    /* <0 = False, >=0 = True */
#define BOOL_REP_ZERO   5    /* 0  = True,  !=0 = False */
#define BOOL_REP_ZERO_N 6    /* 0  = False, !=0 = True */
uint32_t glsl_dataflow_get_bool_rep(Dataflow *dataflow);

#endif

// Returns a dataflow node that converts input to the given type.
// If input is already of the required type, trivially returns input.
Dataflow* glsl_dataflow_convert_type(Dataflow* input, PrimitiveTypeIndex type_index);

typedef enum
{
   TEXTURE_UNIT_A,
   TEXTURE_UNIT_B,
   TEXTURE_UNIT_COUNT
} TextureUnit;

extern Dataflow* last_gadget_get_r[TEXTURE_UNIT_COUNT];

extern void glsl_init_texture_lookups(void);
extern void glsl_init_dataflow_stack(void);

//
// AST to dataflow conversion.
// Calculates the Dataflow graphs for all the numeric and boolean variables in ast.
//

void glsl_calculate_dataflow(Statement* ast);


//
// Dataflow source (linkable value and sampler) metadata.
//

// DataflowSources for the vertex and fragment shaders will be compared
// to decide how to allocate the linkable values and samplers.
// Once packing has been decided, the allocator must record its choices in
// the Dataflow nodes of each source's initial_scalar_values.

struct _DataflowSource
{
	Symbol* source;
	Dataflow** initial_scalar_values;
   bool active;
   bool handled;
};

struct _DataflowSources
{
   // Note that at this point, uniforms still include samplers.
	Map* uniforms; // Map<const char*, DataflowSource>
	Map* attributes; // Map<const char*, DataflowSource>
	Map* varyings; // Map<const char*, DataflowSource>
};

static INLINE DataflowSources* glsl_dataflow_sources_new(void)
{
   DataflowSources* ds = (DataflowSources *)malloc_fast(sizeof(DataflowSources));

   ds->uniforms = glsl_map_new();
   ds->attributes = glsl_map_new();
   ds->varyings = glsl_map_new();

   return ds;
}

typedef struct {
   int size;
   int used;

   Dataflow **nodes;
} DataflowPriorityQueue;

extern void glsl_dataflow_priority_queue_init(DataflowPriorityQueue* queue, int size);
extern void glsl_dataflow_priority_queue_heapify(DataflowPriorityQueue* queue);

extern void glsl_dataflow_priority_queue_push(DataflowPriorityQueue* queue, Dataflow *node);
extern Dataflow *glsl_dataflow_priority_queue_pop(DataflowPriorityQueue* queue);


#ifdef __VIDEOCORE4__

extern MEM_HANDLE_T glsl_dataflow_copy_to_relocatable(uint32_t count, Dataflow **dataflow_out, Dataflow **dataflow_in, void *in_offset);
extern void glsl_dataflow_copy(uint32_t count, Dataflow **dataflow_out, Dataflow **dataflow_in, void *in_offset, Dataflow **inputs, uint32_t num_inputs, DataflowFlavour input_flavour, bool *texture_rb_swap);
#endif

#endif // DATAFLOW_H
