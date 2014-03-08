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
#include "middleware/khronos/glsl/2708/glsl_allocator_4.h"
#include "middleware/khronos/glsl/2708/glsl_qdisasm_4.h"
#include "middleware/khronos/common/khrn_hw.h"

//#define HALF_B_REGFILE

#define MAX_INSTRUCTIONS 15000
#define MAX_UNIFORMS 10000
#define MAX_VARYINGS 32
#define MAX_EXTRA_INSTRUCTIONS 40  /* some spare ones for boilerplate */

#define ALLOW_TEXTURE_LATENCY 16

#ifdef HALF_B_REGFILE
#define BACKTRACK 15
#else
#define BACKTRACK 30
#endif
#define FORWARDTRACK 35

#define MAX_DEPS 64

static uint32_t generate(uint32_t *output);
static uint32_t shader_type;
static bool shader_is_threaded;

/* Shader boilerplate */
#ifdef WORKAROUND_HW2924
static uint32_t simple_hw2924[] = {
   0x959eafff, 0x100248e5, /* mov  r3, rev_flag  ; mov  r5rep, rev_flag */
   0x169e7740, 0x100229e7, /* xor.setf  -, r3, r5 */
   0x00000030, 0xf03809e7, /* brr.anynz  -, r:sep_fwdrev */
   0x159e76c0, 0x10020027, /* mov ra0, r3 */
   0x009e7000, 0x100009e7, /* nop */
   0x009e7000, 0x100009e7, /* nop */
   /* :comb_fwdrev */
   0x009e7000, 0x100009e7, /* nop */
   0x159cffc0, 0x10020b27, /* mov  tlbz, rb15 */
   0x159e7000, 0x10020ba7, /* mov  tlbc, r0 */
   0x009e7000, 0x300009e7, /* nop                                       ; thrend */
   0x009e7000, 0x100009e7, /* nop */
   0x009e7000, 0x500009e7, /* nop                                       ; sbdone */
   /* :sep_fwdrev */
   0x80034036, 0xd00049e5, /* nop                ; mov  r5rep, ra0 >> 4 */
   0x95aa7b76, 0x100248a1, /* mov  r2, r5        ; mov  r1, ms_mask */
   0x169ea5c0, 0x100229e7, /* xor.setf  -, r2, rev_flag */
   0x959c04bf, 0xd002daaa, /* mov  rev_flag, r2  ; mov.ifnz  ms_mask, 0 */
   0x009e7000, 0x100009e7, /* nop */
   0x159cffc0, 0x10020b27, /* mov  tlbz, rb15 */
   0x159e7000, 0x10020ba7, /* mov  tlbc, r0 */
   0x00000000, 0xe0040867, /* mov.ifz  r1, 0 */
   0x969c15c9, 0xd0025aaa, /* xor  rev_flag, r2, 1 ; mov  ms_mask, r1 */
   0x159cffc0, 0x10020b27, /* mov  tlbz, rb15 */
   0x159e7000, 0x10020ba7, /* mov  tlbc, r0 */
   0x009e7000, 0x300009e7, /* nop                                       ; thrend */
   0x009e7000, 0x100009e7, /* nop */
   0x009e7000, 0x500009e7, /* nop                                       ; sbdone */
};

static uint32_t multisample_hw2924[] = {
   0x959eafff, 0x10024025, /* mov  ra0, rev_flag  ; mov  r5rep, rev_flag */
   0x009e7000, 0x100009e7, /* nop */
   0x16027d40, 0x100229e7, /* xor.setf  -, ra0, r5 */
   0x00000028, 0xf03809e7, /* brr.anynz  -, r:sep_fwdrev */
   0x009e7000, 0x100009e7, /* nop */
   0x009e7000, 0x100009e7, /* nop */
   0x009e7000, 0x100009e7, /* nop */
   /* :comb_fwdrev */
   0x159cffc0, 0x10020b27, /* mov  tlbz, rb15 */
   0x159e7000, 0x10020b67, /* mov  tlbm, r0 */
   0x159e7240, 0x30020b67, /* mov  tlbm, r1                             ; thrend */
   0x159e7480, 0x10020b67, /* mov  tlbm, r2 */
   0x159e76c0, 0x50020b67, /* mov  tlbm, r3                             ; sbdone */
   /* :sep_fwdrev */
   0x80034036, 0xd00049e5, /* nop                ; mov  r5rep, ra0 >> 4 */
   0x95aa7b76, 0x100248a1, /* mov  r2, r5        ; mov  r1, ms_mask */
   0x169ea5c0, 0x100229e7, /* xor.setf  -, r2, rev_flag */
   0x959c04bf, 0xd002daaa, /* mov  rev_flag, r2  ; mov.ifnz  ms_mask, 0 */
   0x009e7000, 0x100009e7, /* nop */
   0x159cffc0, 0x10020b27, /* mov  tlbz, rb15 */
   0x159e7000, 0x10020b67, /* mov  tlbm, r0 */
   0x159e7240, 0x10020b67, /* mov  tlbm, r1 */
   0x159e7480, 0x10020b67, /* mov  tlbm, r2 */
   0x159e76c0, 0x10020b67, /* mov  tlbm, r3 */
   0x00000000, 0xe0040867, /* mov.ifz  r1, 0 */
   0x969c15c9, 0xd0025aaa, /* xor  rev_flag, r2, 1 ; mov  ms_mask, r1 */
   0x159cffc0, 0x10020b27, /* mov  tlbz, rb15 */
   0x159e7000, 0x10020b67, /* mov  tlbm, r0 */
   0x159e7240, 0x30020b67, /* mov  tlbm, r1                             ; thrend */
   0x159e7480, 0x10020b67, /* mov  tlbm, r2 */
   0x159e76c0, 0x50020b67, /* mov  tlbm, r3                             ; sbdone */
};
#endif

/*

Op source

This can be:
- the result of another op
- a uniform
- a varying
- prehistoric ops (stuff that's already in the vpm in the vertex shader)

*/

typedef uint32_t CHOICE_T;
/*DECISION (does CHOICE_ACC come first or last)*/
#define CHOICE_ACC 0
#define CHOICE_A 1
#define CHOICE_B 2
#define CHOICE_STANDARD 3
#define MAX_CHOICES 4

#define CHOICE_EITHER 4   /* Not really a choice but used for INPUT_SPECIAL */

typedef uint32_t CHOICE_MASK_T;
#define CHOICE_MASK_ACC (1<<CHOICE_ACC)
#define CHOICE_MASK_A (1<<CHOICE_A)
#define CHOICE_MASK_B (1<<CHOICE_B)
#define CHOICE_MASK_STANDARD (1<<CHOICE_STANDARD)

typedef uint32_t OP_SRC_T;
#define INPUT_SPECIAL(c,r) ((c)<<11 | (r)<<5)
#define INPUT_Z       INPUT_SPECIAL(CHOICE_B, 15)
#define INPUT_W       INPUT_SPECIAL(CHOICE_A, 15)
#define INPUT_UNIFORM INPUT_SPECIAL(CHOICE_EITHER, 32)
#define INPUT_VARYING INPUT_SPECIAL(CHOICE_EITHER, 35)
#define INPUT_X_COORD INPUT_SPECIAL(CHOICE_A, 41)
#define INPUT_Y_COORD INPUT_SPECIAL(CHOICE_B, 41)
#define INPUT_REV_FLAG INPUT_SPECIAL(CHOICE_B, 42)
#define INPUT_VPM     INPUT_SPECIAL(CHOICE_EITHER, 48)
#define INPUT_FIRST_OP_DEST 0x10000000
#define INPUT_LAST_OP_DEST 0x20000000
#define INPUT_FIRST_R4 0x20000000
#define INPUT_LAST_R4 0x30000000
#define INPUT_FIRST_R5 0x30000000
#define INPUT_LAST_R5 0x40000000
static uint32_t get_special_input_choice(OP_SRC_T input)
{
   uint32_t result = input >> 11;
   vcos_assert(result == CHOICE_A || result == CHOICE_B || result == CHOICE_EITHER);
   return result;
}
static uint32_t get_special_input_row(OP_SRC_T input)
{
   return (input >> 5) & 63;
}

typedef struct
{
   OP_SRC_T src;
   uint32_t detail0;
   uint32_t detail1;
   bool retire;
   bool is_new;  /* not already used in instruction */

   /*
      TODO: I don't like having a pointer here, but it's the easiest
      way to make sure input locations get recorded correctly. We need
      to do this so that things can have iodependencies on inputs, and
      the varyings C (r5) thing works.
   */
   int *op;
} OP_SRC_DETAIL_T;

/*
   TODO: I'm still not entirely happy about how I compare inputs for equality.
   We want to allow reuse if:
   - both operands are the same (e.g. mov, squaring)
   - the result of a previous instruction is being used in both halves of this
     instruction.
   We have to be very careful in other cases:
   - if two things are reading from a uniform or varying, the detail will sort them
     out (but we don't record the detail!)
   - if two things are reading from the VPM, the iodependencies should force one of
     them to happen first
   - anything else we need to consider?
*/
static bool src_detail_equal(OP_SRC_DETAIL_T a, OP_SRC_DETAIL_T b)
{
   return a.src == b.src && a.detail0 == b.detail0 && a.detail1 == b.detail1;
}

typedef uint32_t UNPACK_T;
#define UNPACK_NONE  0x00
#define UNPACK_16A   0x01
#define UNPACK_16A_F 0x11
#define UNPACK_16B   0x02
#define UNPACK_16B_F 0x12
#define UNPACK_8R    0x03
#define UNPACK_8A    0x04
#define UNPACK_8A_F  0x14
#define UNPACK_8B    0x05
#define UNPACK_8B_F  0x15
#define UNPACK_8C    0x06
#define UNPACK_8C_F  0x16
#define UNPACK_8D    0x07
#define UNPACK_8D_F  0x17

typedef uint32_t PACK_T;
#define PACK_NONE  0x00
#define PACK_16A   0x21
#define PACK_16A_F 0x11
#define PACK_16B   0x22
#define PACK_16B_F 0x12
#define PACK_8R    0x03
#define PACK_8A    0x04
#define PACK_8B    0x05
#define PACK_8C    0x06
#define PACK_8D    0x07
#define PACK_S     0x08     //Uses overflow/carry flags
#define PACK_16A_S 0x09
#define PACK_16B_S 0x0a
#define PACK_8R_S  0x0b
#define PACK_8A_S  0x0c
#define PACK_8B_S  0x0d
#define PACK_8C_S  0x0e
#define PACK_8D_S  0x0f
#define PACK_8R_C  0x33
#define PACK_8A_C  0x34
#define PACK_8B_C  0x35
#define PACK_8C_C  0x36
#define PACK_8D_C  0x37
#define PACK_TYPE_A   0x00
#define PACK_TYPE_A_F 0x10
#define PACK_TYPE_A_I 0x20
#define PACK_TYPE_MUL 0x30
#define PACK_TYPE_NONE 0x40
static uint32_t get_pack_type(PACK_T pack)
{
   if (pack == PACK_NONE)
      return PACK_TYPE_NONE;
   else
      return pack & 0x30;
}

/*

The Op object

*/

typedef uint32_t OP_FLAVOUR_T;
#define FLAVOUR_UNUSED 0
#define FLAVOUR_FADD 1
#define FLAVOUR_FSUB 2
#define FLAVOUR_FMIN 3
#define FLAVOUR_FMAX 4
#define FLAVOUR_FMINABS 5
#define FLAVOUR_FMAXABS 6
#define FLAVOUR_FTOI 7
#define FLAVOUR_ITOF 8
#define FLAVOUR_ADD 12
#define FLAVOUR_SUB 13
#define FLAVOUR_SHR 14
#define FLAVOUR_ASR 15
#define FLAVOUR_ROR 16
#define FLAVOUR_SHL 17
#define FLAVOUR_MIN 18
#define FLAVOUR_MAX 19
#define FLAVOUR_AND 20
#define FLAVOUR_OR 21
#define FLAVOUR_XOR 22
#define FLAVOUR_NOT 23
#define FLAVOUR_CLZ 24
#define FLAVOUR_FMUL 33
#define FLAVOUR_MUL24 34
#define FLAVOUR_V8MULD 35
#define FLAVOUR_V8MIN 36
#define FLAVOUR_V8MAX 37
#define FLAVOUR_MOV 64
#define FLAVOUR_V8ADDS 65
#define FLAVOUR_V8SUBS 66
#define FLAVOUR_NOP 96

typedef uint32_t FLAVOUR_TYPE_T;
#define FLAVOUR_TYPE_ADD_F 1
#define FLAVOUR_TYPE_FTOI  2
#define FLAVOUR_TYPE_ITOF  3
#define FLAVOUR_TYPE_ADD_I 4
#define FLAVOUR_TYPE_MUL_F 5
#define FLAVOUR_TYPE_MUL_I 6
#define FLAVOUR_TYPE_MOV_I 7

typedef uint32_t OP_COND_T;
#define COND_ALWAYS 0
#define COND_Z_SET 2
#define COND_Z_CLEAR 3
#define COND_N_SET 4
#define COND_N_CLEAR 5
#define COND_C_SET 6
#define COND_C_CLEAR 7

typedef uint32_t OP_T;
#define MOVE_NO_DEST (~1)
#define MOVE_STILL_THERE (~2)
#define MOVE_RETIRED (~3)

typedef uint32_t OP_DEST_T;
#define DEST_A(i) (64*(i))
#define DEST_B(i) (64*(i)+0x1000)
#define DEST_EITHER(i) (64*(i)+0x2000)
#define DEST_ACC(i) (64*(i)+0x3000)
#define DEST_R4 DEST_ACC(4)
#define DEST_UNCERTAIN 0x80000000
#define dest_pack(p) (DEST_UNCERTAIN+(p))
#define dest_is_a(d) (/*(d) >= DEST_A(0) &&*/ (d) < DEST_A(64))
#define dest_is_b(d) ((d) >= DEST_B(0) && (d) < DEST_B(64))
#define dest_is_reg_a(d) (/*(d) >= DEST_A(0) &&*/ (d) < DEST_A(32))
#define dest_is_reg_b(d) ((d) >= DEST_B(0) && (d) < DEST_B(32))
#define dest_is_either(d) ((d) >= DEST_EITHER(0) && (d) < DEST_EITHER(64))
#define dest_is_normal_acc(d) ((d) >= DEST_ACC(0) && (d) < DEST_ACC(4))   // r0-r3 only
#define dest_is_uncertain(d) (get_dest_without_pack(d) == DEST_UNCERTAIN)
#define dest_is_sfu(d) ((d) == DEST_EITHER(52) || (d) == DEST_EITHER(53) || (d) == DEST_EITHER(54) || (d) == DEST_EITHER(55))
#define dest_is_texture_write(d) ((d) >= DEST_EITHER(56) && (d) <= DEST_EITHER(63))
#define dest_is_texture_write_retiring(d) ((d) == DEST_EITHER(56) || (d) == DEST_EITHER(60) || (d) == DEST_A(56))
#define dest_is_texture_direct(d) ((d) == DEST_A(56))       /* TODO: not really DEST_A. We just need some way of distinguishing direct writes with s writes */
static PACK_T get_dest_pack(OP_DEST_T dest)
{
   return dest % 64;
}
static uint32_t get_dest_index(OP_DEST_T dest)
{
   return (dest / 64) & 63;
}
static OP_DEST_T get_dest_without_pack(OP_DEST_T dest)
{
   return dest - get_dest_pack(dest);
}
static void dest_set(OP_DEST_T *dest, OP_DEST_T location)
{
   vcos_assert(dest_is_uncertain(*dest));
   vcos_assert(get_dest_pack(location) == 0);

   *dest = get_dest_pack(*dest) + location;
}

typedef uint32_t INSTR_SIG_T;
#define SIG_UNUSED 1
#define SIG_THRSW 2
#define SIG_THREND 3
#define SIG_SB_WAIT 4
#define SIG_SB_DONE 5
#ifndef WORKAROUND_HW1632
#define SIG_LAST_THRSW 6
#endif
#define SIG_GET_COL 8
#define SIG_GET_COL_THREND 9
#define SIG_TMU0 10
#define SIG_TMU1 11

typedef struct
{
   OP_FLAVOUR_T flavour;
   OP_SRC_DETAIL_T left;
   OP_SRC_DETAIL_T right;
   OP_COND_T cond;

   OP_DEST_T dest;
   uint32_t dest_detail0;
   uint32_t dest_detail1;
   bool set_flags;   //we put this here rather than INSTR_STRUCT_T so we can assert that we don't set the flags on the wrong half

   OP_T move;

   uint32_t schedule_order;  /*DEBUG*/
} OP_STRUCT_T;
static uint32_t schedule_order;/*DEBUG*/

typedef uint32_t INSTR_CONFLICT_T;
//Conflicts
#define CONFLICT_READ_A       0x00000001
#define CONFLICT_READ_B       0x00000002
#define CONFLICT_UNPACK       0x00000004
#define CONFLICT_PACK         0x00000008

#define CONFLICT_UNIFORM      0x00000010
#define CONFLICT_VARYING      0x00000020
#define CONFLICT_VPM_READ     0x00000040
#define CONFLICT_VPM_WRITE    0x00000080
#define CONFLICT_CCHW_TLBR    0x00000100
#define CONFLICT_CCHW_TLBW    0x00000200
#define CONFLICT_READ_R4      0x00000400  /* don't want both inputs to be r4, one with pack and one without */
//Counters
#define CONFLICT_READ_EITHER  0x00000800

#define CONFLICT_MASK_0       0x00000555
#define CONFLICT_MASK_1       0x00001aaa

//Decisions
#define DECISION_IS_REGFILE0  0x00002000
#define DECISION_NOT_REGFILE0 0x00004000
#define DECISION_IS_REGFILE1  0x00008000
#define DECISION_NOT_REGFILE1 0x00010000
#define DECISION_IN_INTEGER0  0x00020000
#define DECISION_IN_FLOAT0    0x00040000
#define DECISION_IN_INTEGER1  0x00080000
#define DECISION_IN_FLOAT1    0x00100000
#define DECISION_OUT_INTEGER0 0x00200000
#define DECISION_OUT_FLOAT0   0x00400000
#define DECISION_OUT_INTEGER1 0x00800000
#define DECISION_OUT_FLOAT1   0x01000000
#define DECISION_ADD0         0x02000000
#define DECISION_ADD1         0x04000000
#define DECISION_WRITE_A0     0x08000000
#define DECISION_WRITE_A1     0x10000000
#define DECISION_PM0          0x20000000
#define DECISION_PM1          0x40000000

#define DECISION_MASK_0       0x2aaaa000
#define DECISION_MASK         (DECISION_MASK_0 | (DECISION_MASK_0 << 1))

#define DECISION_IS_REGFILE(h)  ((h) ? DECISION_IS_REGFILE1 : DECISION_IS_REGFILE0)
#define DECISION_NOT_REGFILE(h) ((h) ? DECISION_NOT_REGFILE1 : DECISION_NOT_REGFILE0)
#define DECISION_IN_INTEGER(h) ((h) ? DECISION_IN_INTEGER1 : DECISION_IN_INTEGER0)
#define DECISION_IN_FLOAT(h) ((h) ? DECISION_IN_FLOAT1 : DECISION_IN_FLOAT0)
#define DECISION_OUT_INTEGER(h) ((h) ? DECISION_OUT_INTEGER1 : DECISION_OUT_INTEGER0)
#define DECISION_OUT_FLOAT(h) ((h) ? DECISION_OUT_FLOAT1 : DECISION_OUT_FLOAT0)
#define DECISION_ADD(h) ((h) ? DECISION_ADD1 : DECISION_ADD0)
#define DECISION_MUL(h) DECISION_ADD(1-(h))
#define DECISION_WRITE_A(h) ((h) ? DECISION_WRITE_A1 : DECISION_WRITE_A0)
#define DECISION_WRITE_B(h) DECISION_WRITE_A(1-(h))

#define DEFAULT_HALF 0

static bool conflict_add(INSTR_CONFLICT_T *conflict_inout, INSTR_CONFLICT_T extra)
{
   INSTR_CONFLICT_T result0, result1, decision;
   INSTR_CONFLICT_T conflict;

   conflict = *conflict_inout;

   result0 = (conflict & CONFLICT_MASK_0) + (extra & CONFLICT_MASK_0);
   result1 = (conflict & CONFLICT_MASK_1) + (extra & CONFLICT_MASK_1);
   decision = (conflict & DECISION_MASK)  | (extra & DECISION_MASK);

   //Fail if any counter overflows
   if ((result0 & ~CONFLICT_MASK_0) || ((result1+CONFLICT_READ_EITHER) & ~CONFLICT_MASK_1))
      return false;

   //Fail if we make any conflicting decisions
   if (decision & ((decision & DECISION_MASK_0) << 1))
      return false;

   *conflict_inout = result0 | result1 | decision;
   return true;
}
//TODO this function is horrible
static INSTR_CONFLICT_T conflict_swap(INSTR_CONFLICT_T conflict, uint32_t half)
{
   INSTR_CONFLICT_T result;

   if (half == DEFAULT_HALF)
      return conflict;

   result = conflict & (CONFLICT_MASK_0 | CONFLICT_MASK_1);
   if (conflict & DECISION_IS_REGFILE0) result |= DECISION_IS_REGFILE1;
   if (conflict & DECISION_NOT_REGFILE0) result |= DECISION_NOT_REGFILE1;
   if (conflict & DECISION_IS_REGFILE1) result |= DECISION_IS_REGFILE0;
   if (conflict & DECISION_NOT_REGFILE1) result |= DECISION_NOT_REGFILE0;
   if (conflict & DECISION_IN_INTEGER0) result |= DECISION_IN_INTEGER1;
   if (conflict & DECISION_IN_FLOAT0) result |= DECISION_IN_FLOAT1;
   if (conflict & DECISION_IN_INTEGER1) result |= DECISION_IN_INTEGER0;
   if (conflict & DECISION_IN_FLOAT1) result |= DECISION_IN_FLOAT0;
   if (conflict & DECISION_OUT_INTEGER0) result |= DECISION_OUT_INTEGER1;
   if (conflict & DECISION_OUT_FLOAT0) result |= DECISION_OUT_FLOAT1;
   if (conflict & DECISION_OUT_INTEGER1) result |= DECISION_OUT_INTEGER0;
   if (conflict & DECISION_OUT_FLOAT1) result |= DECISION_OUT_FLOAT0;
   if (conflict & DECISION_ADD0) result |= DECISION_ADD1;
   if (conflict & DECISION_ADD1) result |= DECISION_ADD0;
   if (conflict & DECISION_WRITE_A0) result |= DECISION_WRITE_A1;
   if (conflict & DECISION_WRITE_A1) result |= DECISION_WRITE_A0;
   if (conflict & DECISION_PM0) result |= DECISION_PM0;
   if (conflict & DECISION_PM1) result |= DECISION_PM1;

   return result;
}

typedef uint32_t TWEAK_T;
#define FORCE_A 1
#define FORBID_MOV_PRE_UNPACK 2
#define FORBID_MOV_POST_UNPACK 4

typedef struct
{
   /*
   Single input:
   - A goes this way (not if same as other input)
   - B goes this way (ditto)
   - R4 goes this way (ditto)
   - unpack goes this way (as only one input can use unpack at a time)
   - hw conflict bits
   Decisions that only conflict with alu/output:
   - pm = 0 or 1
   - alu consumes float

   If input is unpack then it may fail all on its own (e.g. it's trying to read from something
   that isn't regfile A). In this case we need to insert a mov *before* packing.

   On other occasions (reg A conflict, R4 conflict, two unpacks) we need to insert a mov *after*
   packing.

   Are there some times when either will work?


   */
   INSTR_CONFLICT_T conflict;
//   INSTR_DECISION_T decision0;
//   INSTR_DECISION_T decision1;

   INSTR_SIG_T sig;

   OP_T move_r4;
   OP_T move_r5;
} INSTR_STRUCT_T;

static OP_STRUCT_T op_struct[MAX_INSTRUCTIONS][2];
static INSTR_STRUCT_T instr_struct[MAX_INSTRUCTIONS];
static uint32_t last_used_timestamp;

static uint32_t generated_instructions[2*(MAX_INSTRUCTIONS+MAX_EXTRA_INSTRUCTIONS)];
static uint32_t generated_instruction_count;
static bool finished;
static bool failed;

static uint32_t uniform_map[2*MAX_UNIFORMS];
static uint32_t uniform_count;
static uint32_t varying_map[MAX_VARYINGS];
static uint32_t varying_count;

#ifndef WORKAROUND_HW1632
static uint32_t last_thrsw;
#endif

static bool prefer_b;

// Helper functions

static uint32_t get_op_timestamp(OP_T op)
{
   vcos_assert(op / 2 < MAX_INSTRUCTIONS);
   return op / 2;
}

static uint32_t get_op_half(OP_T op)
{
   return op % 2;
}

static OP_STRUCT_T *get_op_struct(OP_T op)
{
   vcos_assert(op < 2 * MAX_INSTRUCTIONS);
   return &op_struct[op / 2][op % 2];
}

static INSTR_STRUCT_T *get_op_instr(OP_T op)
{
   return &instr_struct[get_op_timestamp(op)];
}

static OP_T op_from_timestamp(uint32_t timestamp, uint32_t half)
{
   vcos_assert(timestamp < MAX_INSTRUCTIONS);
   vcos_assert(half <= 1);
   return timestamp * 2 + half;
}

static INSTR_STRUCT_T *get_instr(uint32_t timestamp)
{
   vcos_assert(timestamp < MAX_INSTRUCTIONS);
   return &instr_struct[timestamp];
}

static OP_STRUCT_T *get_instr_op_struct(uint32_t timestamp, uint32_t half)
{
   vcos_assert(timestamp < MAX_INSTRUCTIONS);
   vcos_assert(half <= 1);
   return &op_struct[timestamp][half];
}

static uint32_t get_first_timestamp(void)
{
   if (last_used_timestamp < BACKTRACK)
      return 0;
   else
      return last_used_timestamp - BACKTRACK;
}

static bool input_is_from_op_dest(OP_SRC_T input)
{
   return input >= INPUT_FIRST_OP_DEST && input < INPUT_LAST_OP_DEST;
}

static bool input_is_from_instr_r4(OP_SRC_T input)
{
   return input >= INPUT_FIRST_R4 && input < INPUT_LAST_R4;
}

static bool input_is_from_instr_r5(OP_SRC_T input)
{
   return input >= INPUT_FIRST_R5 && input < INPUT_LAST_R5;
}

static bool input_is_from_instr(OP_SRC_T input)
{
   return input_is_from_op_dest(input) || input_is_from_instr_r4(input) || input_is_from_instr_r5(input);
}


static OP_SRC_T input_from_op(OP_T op, UNPACK_T unpack)
{
   return INPUT_FIRST_OP_DEST + op * 32 + unpack;
}

static OP_SRC_T input_from_instr_r4(uint32_t timestamp, UNPACK_T unpack, bool actually_care)
{
   if (actually_care) vcos_assert(get_instr(timestamp)->move_r4 == (OP_T)MOVE_STILL_THERE);
   return INPUT_FIRST_R4 + timestamp * 32 + unpack;
}

static OP_SRC_T input_from_instr_r5(uint32_t timestamp, UNPACK_T unpack, bool actually_care)
{
   if (actually_care) vcos_assert(get_instr(timestamp)->move_r5 == (OP_T)MOVE_STILL_THERE);
   return INPUT_FIRST_R5 + timestamp * 32 + unpack;
}

static uint32_t get_input_timestamp(OP_SRC_T input)
{
   if (input_is_from_op_dest(input))
      return get_op_timestamp((input - INPUT_FIRST_OP_DEST) / 32);
   else if (input_is_from_instr_r4(input))
      return (input - INPUT_FIRST_R4) / 32;
   else if (input_is_from_instr_r5(input))
      return (input - INPUT_FIRST_R5) / 32;
   else
      UNREACHABLE();

   return ~0;
}

static OP_SRC_DETAIL_T src_detail_from_src(OP_SRC_T src, bool retire)
{
   OP_SRC_DETAIL_T result;

   result.src = src;
   result.detail0 = ~0;
   result.detail1 = ~0;
   result.retire = retire;
   result.is_new = true;
   result.op = NULL;

   return result;
}

static OP_SRC_DETAIL_T src_detail_from_op(OP_T op, bool retire, UNPACK_T unpack)
{
   return src_detail_from_src(input_from_op(op, unpack), retire);
}

static OP_SRC_T input_without_unpack(OP_SRC_T input)
{
   return input & ~31;
}

static OP_SRC_DETAIL_T src_detail_without_unpack(OP_SRC_DETAIL_T input_detail, bool retire)
{
   OP_SRC_DETAIL_T result;

   result.src = input_without_unpack(input_detail.src);
   result.detail0 = input_detail.detail0;
   result.detail1 = input_detail.detail1;
   result.retire = retire;
   result.is_new = true;
   result.op = input_detail.op;

   return result;
}

static OP_SRC_DETAIL_T src_detail_none(void)
{
   OP_SRC_DETAIL_T result;

   result.src = ~0;
   result.detail0 = ~0;
   result.detail1 = ~0;
   result.retire = false;
   result.is_new = true;
   result.op = NULL;

   return result;
}

static OP_SRC_T input_with_unpack(OP_SRC_T input, UNPACK_T unpack)
{
   vcos_assert((input & 31) == 0);
   vcos_assert(unpack <= 31);
   return input | unpack;
}

static OP_SRC_DETAIL_T src_detail_with_unpack(OP_SRC_DETAIL_T input_detail, UNPACK_T unpack)
{
   OP_SRC_DETAIL_T result;

   result.src = input_with_unpack(input_detail.src, unpack);
   result.detail0 = input_detail.detail0;
   result.detail1 = input_detail.detail1;
   result.retire = input_detail.retire;
   result.is_new = true;
   result.op = input_detail.op;

   return result;
}

static UNPACK_T get_input_unpack(OP_SRC_T input)
{
   return input % 32;
}

static OP_T get_input_op(OP_SRC_T input)
{
   vcos_assert(input_is_from_op_dest(input));
   return (input % 0x1000000) / 32;
}

// Functions for handling conflicts

static uint32_t move_after(uint32_t timestamp, OP_SRC_T input)
{
   if (input_is_from_instr(input))
   {
      uint32_t input_timestamp = get_input_timestamp(input);

      /* Allow extra space after TMURS write */
      if (input_is_from_op_dest(input) && get_op_struct(get_input_op(input))->dest == DEST_EITHER(36))
      {
         input_timestamp += 2;
      }

      if (input_timestamp + 1 > timestamp)
         timestamp = input_timestamp + 1;
   }
   return timestamp;
}

// Higher-level checking functions

#define AVAILABLE_FUTURE 0x7fffffff
static bool a_is_available[32];
static bool b_is_available[32];
static bool acc_is_available[6];
static uint32_t a_last_used[32];
static uint32_t b_last_used[32];
static uint32_t acc_last_used[6];
static OP_T acc_node[6];    // Used for returning failure node
static uint32_t flags_available;
static OP_T flags_node;

static int a_scheduler_order[32];
static int b_scheduler_order[32];
static Dataflow *a_scheduler_node[32];
static Dataflow *b_scheduler_node[32];
static int scheduler_order;
static Dataflow *scheduler_current_node;
static int scheduler_attempts;
static bool scheduler_have_node;

static bool fb_rb_swap;

static INSTR_CONFLICT_T get_dest_conflict(OP_DEST_T dest, uint32_t half);
static uint32_t find_dest_acc(OP_T op)
{
   uint32_t i;
   uint32_t timestamp;
   uint32_t half;
   INSTR_STRUCT_T *instr;
   INSTR_CONFLICT_T conflict;

   timestamp = get_op_timestamp(op);
   instr = get_op_instr(op);
   half = get_op_half(op);

   /* Check that we can use acc destination here (i.e. it won't conflict with pack mode) */
   /* TODO: dirty as we're choosing an arbitrary dest here */
   conflict = get_dest_conflict(DEST_ACC(0), get_op_half(op));
   if (conflict_add(&conflict, instr->conflict))
   {
      for (i = 0; i < 4; i++)
      {
         if (acc_is_available[i] && acc_last_used[i] <= timestamp)
            return i;
      }
   }
   return ~0;
}

static uint32_t find_dest_a(OP_T op)
{
   uint32_t i;
   uint32_t timestamp;
   uint32_t half;
   uint32_t result;
   int latest_timestamp;
   INSTR_STRUCT_T *instr;
   INSTR_CONFLICT_T conflict;

   timestamp = get_op_timestamp(op);
   half = get_op_half(op);
   instr = get_op_instr(op);

   result = ~0;
   latest_timestamp = -1;

   /* Check that we can use acc destination here (i.e. the other half isn't using A) */
   /* TODO: dirty as we're choosing an arbitrary dest here */
   conflict = get_dest_conflict(DEST_A(0), get_op_half(op));
   if (conflict_add(&conflict, instr->conflict))
   {
      for (i = 0; i < (uint32_t)(shader_is_threaded ? 16 : 32); i++)
      {
         if (a_is_available[i] && a_last_used[i] <= timestamp && (int)a_last_used[i] > latest_timestamp)
         {
            result = i;
            latest_timestamp = (int)a_last_used[i];
         }
      }
   }
   return result;
}

static uint32_t find_dest_b(OP_T op)
{
   uint32_t i;
   uint32_t timestamp;
   uint32_t half;
   uint32_t result;
   int latest_timestamp;
   INSTR_STRUCT_T *instr;
   INSTR_CONFLICT_T conflict;

   timestamp = get_op_timestamp(op);
   half = get_op_half(op);
   instr = get_op_instr(op);

   result = ~0;
   latest_timestamp = -1;


   /* Check that we can use acc destination here (i.e. it won't conflict with pack mode, and the other half isn't using B) */
   /* TODO: dirty as we're choosing an arbitrary dest here */
   conflict = get_dest_conflict(DEST_B(0), get_op_half(op));
   if (conflict_add(&conflict, instr->conflict))
   {
      for (i = 0; i < (uint32_t)(shader_is_threaded ? 16 : 32); i++)
      {
         if (b_is_available[i] && b_last_used[i] <= timestamp && (int)b_last_used[i] > latest_timestamp)
         {
            result = i;
            latest_timestamp = (int)b_last_used[i];
         }
      }
   }
   return result;
}

static CHOICE_T get_dest_choice(OP_DEST_T dest)
{
   vcos_assert(!dest_is_uncertain(dest));
   if (dest_is_reg_a(dest))
      return CHOICE_A;
   else if (dest_is_reg_b(dest))
      return CHOICE_B;
   else if (dest_is_normal_acc(dest))
      return CHOICE_ACC;
   else
   {
      UNREACHABLE();
      return 0;
   }
}

static INSTR_CONFLICT_T get_input_conflict(OP_SRC_T input, CHOICE_T choice, uint32_t *timestamp_out)
{
   INSTR_CONFLICT_T result = 0;
   bool r4;
   uint32_t timestamp = 0;

   r4 = input_is_from_instr_r4(input);

   if (input_is_from_op_dest(input))
   {
      OP_T op = get_input_op(input);
      OP_DEST_T dest = get_op_struct(op)->dest;

      if (choice == CHOICE_STANDARD)
      {
         vcos_assert(!dest_is_uncertain(dest));
         choice = get_dest_choice(dest);
      }
      else
         vcos_assert(dest_is_uncertain(dest));

      timestamp = get_op_timestamp(op);

      if (choice == CHOICE_ACC)
      {
         timestamp++;
      }
      else if (choice == CHOICE_A)
      {
         result += CONFLICT_READ_A + CONFLICT_READ_EITHER;
         timestamp += 2;
      }
      else if (choice == CHOICE_B)
      {
         result += CONFLICT_READ_B + CONFLICT_READ_EITHER;
         timestamp += 2;
      }
   }
   else if (input_is_from_instr_r4(input))
   {
      INSTR_STRUCT_T *instr;

      result += CONFLICT_READ_R4;
      /* TODO: this is a slightly ugly way to distinguish between sfu (requires 2 nops) and tmu/tlb (no nops) */
      instr = get_instr(get_input_timestamp(input));
      if (instr->sig == SIG_GET_COL || instr->sig == SIG_TMU0 || instr->sig == SIG_TMU1)
         timestamp = get_input_timestamp(input) + 1;
      else
         timestamp = get_input_timestamp(input) + 3;
   }
   else if (input_is_from_instr_r5(input))
   {
      timestamp = get_input_timestamp(input) + 1;
   }
   else
   {
      vcos_assert(choice == CHOICE_STANDARD);

      switch (input_without_unpack(input))
      {
      case INPUT_W:
         result += CONFLICT_READ_A + CONFLICT_READ_EITHER;
         break;
      case INPUT_Z:
         result += CONFLICT_READ_B + CONFLICT_READ_EITHER;
         break;
      case INPUT_UNIFORM:
         result += CONFLICT_READ_EITHER + CONFLICT_UNIFORM;
         break;
      case INPUT_VARYING:
         result += CONFLICT_READ_EITHER + CONFLICT_VARYING;
         break;
      case INPUT_X_COORD:
         result += CONFLICT_READ_A + CONFLICT_READ_EITHER;
         break;
      case INPUT_Y_COORD:
         result += CONFLICT_READ_B + CONFLICT_READ_EITHER;
         break;
      case INPUT_REV_FLAG:
         result += CONFLICT_READ_B + CONFLICT_READ_EITHER;
         break;
      case INPUT_VPM:
         result += CONFLICT_READ_EITHER + CONFLICT_VPM_READ;
         break;
      default:
         //TODO: hw conflicts and timestamp checks for other input nodes
         UNREACHABLE();
      }
   }

   switch (get_input_unpack(input))
   {
   case UNPACK_NONE:
      // Nothing special to do here
      break;
   case UNPACK_16A_F:
   case UNPACK_16B_F:
   case UNPACK_8A_F:
   case UNPACK_8B_F:
   case UNPACK_8C_F:
   case UNPACK_8D_F:
      if (r4)
         result += CONFLICT_UNPACK + DECISION_PM1;
      else if (choice == CHOICE_A)
         result += CONFLICT_UNPACK + DECISION_PM0 + DECISION_IN_FLOAT(DEFAULT_HALF);
      else
         UNREACHABLE();
      break;
   case UNPACK_16A:
   case UNPACK_16B:
   case UNPACK_8A:
   case UNPACK_8B:
   case UNPACK_8C:
   case UNPACK_8D:
      if (choice == CHOICE_A)
         result += CONFLICT_UNPACK + DECISION_PM0 + DECISION_IN_INTEGER(DEFAULT_HALF);
      else
         UNREACHABLE();
      break;
   case UNPACK_8R:
      if (r4)
         result += CONFLICT_UNPACK + DECISION_PM1;
      else if (choice == CHOICE_A)
         result += CONFLICT_UNPACK + DECISION_PM0;
      else
         UNREACHABLE();
      break;
   default:
      UNREACHABLE();
   }

   *timestamp_out = timestamp;
   return result;
}

static OP_FLAVOUR_T get_suitable_mov(OP_SRC_T input)
{
   switch (get_input_unpack(input))
   {
   case UNPACK_16A_F:
   case UNPACK_16B_F:
   case UNPACK_8A_F:
   case UNPACK_8B_F:
   case UNPACK_8C_F:
   case UNPACK_8D_F:
      return FLAVOUR_FMIN;
   default:
      return FLAVOUR_MOV;
   }
}

static CHOICE_MASK_T get_input_choice(OP_SRC_DETAIL_T input_detail, INSTR_CONFLICT_T *conflicts, uint32_t *timestamps)
{
   OP_SRC_T input;
   CHOICE_MASK_T choice;
   uint32_t i;
   UNPACK_T unpack;

   if (!input_detail.is_new)
   {
      conflicts[CHOICE_STANDARD] = 0;
      timestamps[CHOICE_STANDARD] = 0;
      return CHOICE_MASK_STANDARD;
   }

   input = input_detail.src;
   unpack = get_input_unpack(input);

   if (input_is_from_op_dest(input))
   {
      OP_T op;
      OP_STRUCT_T *op_struct;

      switch (unpack)
      {
      case UNPACK_NONE:
         choice = CHOICE_MASK_ACC | /*CHOICE_MASK_R4 |*/ CHOICE_MASK_A | CHOICE_MASK_B;
         break;
      case UNPACK_16A_F:
      case UNPACK_16B_F:
      case UNPACK_8A_F:
      case UNPACK_8B_F:
      case UNPACK_8C_F:
      case UNPACK_8D_F:
         choice = /*CHOICE_MASK_R4 |*/ CHOICE_MASK_A;
         break;
      case UNPACK_16A:
      case UNPACK_16B:
      case UNPACK_8A:
      case UNPACK_8B:
      case UNPACK_8C:
      case UNPACK_8D:
         choice = CHOICE_MASK_A;
         break;
      case UNPACK_8R:
         choice = /*CHOICE_MASK_R4 |*/ CHOICE_MASK_A;
         break;
      default:
         UNREACHABLE();
         choice = 0;
      }

      op = get_input_op(input);
      //instr = get_op_instr(op);//UNUSED
      op_struct = get_op_struct(op);

      if (dest_is_uncertain(op_struct->dest))
      {
         // We can't choose R4
         //choice &= ~CHOICE_MASK_R4;

         if (find_dest_acc(op) == (uint32_t)~0 || !input_detail.retire/*DECISION*/)
            choice &= ~CHOICE_MASK_ACC;

         if (find_dest_a(op) == (uint32_t)~0)
            choice &= ~CHOICE_MASK_A;

         if (find_dest_b(op) == (uint32_t)~0)
            choice &= ~CHOICE_MASK_B;
      }
      else
      {
         choice = (choice & (1<<get_dest_choice(op_struct->dest))) ? CHOICE_MASK_STANDARD : 0;
      }
   }
   else
   {
      if (unpack != 0)
      {
         if(input_is_from_instr_r4(input))
         {
            switch (unpack)
            {
            /*case UNPACK_NONE: //UNREACHABLE */
            case UNPACK_16A_F:
            case UNPACK_16B_F:
            case UNPACK_8A_F:
            case UNPACK_8B_F:
            case UNPACK_8C_F:
            case UNPACK_8D_F:
               choice = CHOICE_MASK_STANDARD;
               break;
            case UNPACK_16A:
            case UNPACK_16B:
            case UNPACK_8A:
            case UNPACK_8B:
            case UNPACK_8C:
            case UNPACK_8D:
               choice = 0;
               break;
            case UNPACK_8R:
               choice = CHOICE_MASK_STANDARD;
               break;
            default:
               UNREACHABLE();
               choice = 0;
            }
         }
         else
         {
            choice = 0;  /* can only unpack op dest */
         }
      }
      else if (!input_is_from_instr(input) && !input_detail.retire)
         choice = 0;  /* Can't read from this kind of input if other people are reading from it too */
      else
         choice = CHOICE_MASK_STANDARD;
   }

   for (i = 0; i < MAX_CHOICES; i++)
   {
      if (choice & (1<<i))
         conflicts[i] = get_input_conflict(input, i, &timestamps[i]);
   }

   return choice;
}

static INSTR_CONFLICT_T get_dest_conflict(OP_DEST_T dest, uint32_t half)
{
   uint32_t conflict;
   bool is_regfile = false;
   PACK_T pack;

   pack = get_dest_pack(dest);
   dest = get_dest_without_pack(dest);

   if (dest_is_reg_a(dest))
   {
      conflict = DECISION_WRITE_A(half);
      is_regfile = true;
   }
   else if (dest_is_reg_b(dest))
   {
      conflict = DECISION_WRITE_B(half);
      is_regfile = true;
   }
   else if (dest_is_normal_acc(dest))
      conflict = 0;
   else if (dest == DEST_EITHER(36))  /* tmurs */
      conflict = 0;
   else if (dest == DEST_EITHER(43))  /* stencil */
      conflict = 0;
   else if (dest == DEST_EITHER(44))  /* tlbz */
      conflict = CONFLICT_CCHW_TLBR + CONFLICT_CCHW_TLBW;
   else if (dest == DEST_EITHER(45))  /* tlbm */
      conflict = CONFLICT_CCHW_TLBW;
   else if (dest == DEST_EITHER(46))  /* tlbc */
      conflict = CONFLICT_CCHW_TLBW;
   else if (dest == DEST_EITHER(48))  /* vpm */
      conflict = CONFLICT_VPM_WRITE;
   else if (dest == DEST_A(49))       /* vpm read setup */
#ifdef WORKAROUND_HW2479
      conflict = DECISION_WRITE_A(half) + CONFLICT_VPM_WRITE + CONFLICT_VPM_READ + DECISION_ADD(half);
#else
      conflict = DECISION_WRITE_A(half) + CONFLICT_VPM_WRITE + CONFLICT_VPM_READ;
#endif
   else if (dest == DEST_B(49))       /* vpm write setup */
#ifdef WORKAROUND_HW2479
      conflict = DECISION_WRITE_B(half) + CONFLICT_VPM_WRITE + DECISION_ADD(half);
#else
      conflict = DECISION_WRITE_B(half) + CONFLICT_VPM_WRITE;
#endif
   else if (dest_is_sfu(dest))
      conflict = CONFLICT_CCHW_TLBR + CONFLICT_CCHW_TLBW;
   else if (dest_is_texture_write(dest))
   {
#ifdef WORKAROUND_HW2522
      conflict = CONFLICT_CCHW_TLBR + CONFLICT_CCHW_TLBW + CONFLICT_UNIFORM + CONFLICT_VPM_READ;
#else
      conflict = CONFLICT_CCHW_TLBR + CONFLICT_CCHW_TLBW + CONFLICT_UNIFORM;
#endif
   }
#ifndef __BCM2708A0
   else if (dest_is_texture_direct(dest))
      conflict = CONFLICT_CCHW_TLBR + CONFLICT_CCHW_TLBW;
#endif
   else if (dest == DEST_UNCERTAIN)   /* TODO: should this case be here? */
      conflict = 0;
//   else if (dest == DEST_ACC(0))
//      conflict = 0;
   else
   {
      //TODO
      UNREACHABLE();
      return ~0;
   }

   if (is_regfile)
   {
      verify(conflict_add(&conflict, DECISION_IS_REGFILE(half)));
   }
   else if (dest != DEST_UNCERTAIN)
   {
      verify(conflict_add(&conflict, DECISION_NOT_REGFILE(half)));
   }

   //TODO: handle dest pack conflict
   switch (get_pack_type(pack))
   {
   case PACK_TYPE_A:
      verify(conflict_add(&conflict, CONFLICT_PACK | DECISION_PM0 | DECISION_IS_REGFILE(half) | DECISION_WRITE_A(half)));
      break;
   case PACK_TYPE_A_F:
      verify(conflict_add(&conflict, CONFLICT_PACK | DECISION_PM0 | DECISION_IS_REGFILE(half) | DECISION_WRITE_A(half) | DECISION_OUT_FLOAT(half)));
      break;
   case PACK_TYPE_A_I:
      verify(conflict_add(&conflict, CONFLICT_PACK | DECISION_PM0 | DECISION_IS_REGFILE(half) | DECISION_WRITE_A(half) | DECISION_OUT_INTEGER(half)));
      break;
   case PACK_TYPE_MUL:
      verify(conflict_add(&conflict, CONFLICT_PACK | DECISION_PM1 | DECISION_MUL(half)));
      break;
   case PACK_TYPE_NONE:
      /* Nothing to do */;
      break;
   default:
      UNREACHABLE();
   }
   return conflict;
}

static void commit_alu_choice(OP_T op, CHOICE_T choice)
{
   OP_STRUCT_T *op_struct;
   INSTR_STRUCT_T *instr;
   uint32_t i;
   uint32_t timestamp;
   OP_DEST_T dest;

   vcos_assert(choice != CHOICE_STANDARD);

   instr = get_op_instr(op);
   op_struct = get_op_struct(op);
   timestamp = get_op_timestamp(op);

   vcos_assert(dest_is_uncertain(op_struct->dest));

   switch (choice)
   {
   case CHOICE_ACC:
      i = find_dest_acc(op);
      vcos_assert(i != (uint32_t)~0);
      dest = DEST_ACC(i);

      vcos_assert(acc_is_available[i] && acc_last_used[i] <= timestamp);
      vcos_assert(acc_node[i] == (uint32_t)~0);
      acc_is_available[i] = false;
      acc_last_used[i] = timestamp;
      acc_node[i] = op;

      vcos_assert(op_struct->move == (OP_T)MOVE_NO_DEST);
      op_struct->move = MOVE_STILL_THERE;
      break;
   case CHOICE_A:
      i = find_dest_a(op);
      vcos_assert(i != (uint32_t)~0);
      dest = DEST_A(i);

      vcos_assert(a_is_available[i] && a_last_used[i] <= timestamp);
      a_is_available[i] = false;
      a_last_used[i] = timestamp;
      a_scheduler_order[i] = scheduler_order++;
      a_scheduler_node[i] = scheduler_current_node;

      vcos_assert(op_struct->move == (OP_T)MOVE_NO_DEST);
      op_struct->move = MOVE_STILL_THERE;
      break;
   case CHOICE_B:
      i = find_dest_b(op);
      vcos_assert(i != (uint32_t)~0);
      dest = DEST_B(i);

      vcos_assert(b_is_available[i] && b_last_used[i] <= timestamp);
      b_is_available[i] = false;
      b_last_used[i] = timestamp;
      b_scheduler_order[i] = scheduler_order++;
      b_scheduler_node[i] = scheduler_current_node;

      vcos_assert(op_struct->move == (OP_T)MOVE_NO_DEST);
      op_struct->move = MOVE_STILL_THERE;
      break;
   default:
      UNREACHABLE();
      dest = 0;
   }

   dest_set(&op_struct->dest, dest);
   verify(conflict_add(&instr->conflict, get_dest_conflict(dest, get_op_half(op))));
}

static void commit_input_choice(OP_SRC_DETAIL_T input_detail, CHOICE_T choice, OP_T consumer)
{
   OP_SRC_T input;

   if (!input_detail.is_new) return;

   //if (choice == CHOICE_A || choice == CHOICE_B) prefer_b = !prefer_b;

   input = input_detail.src;
   if (input_is_from_op_dest(input))
   {
      OP_T op = get_input_op(input);
      vcos_assert((get_op_struct(op)->move == (OP_T)MOVE_NO_DEST) || (get_op_struct(op)->move == (OP_T)MOVE_STILL_THERE));
      if (dest_is_uncertain(get_op_struct(op)->dest))
         commit_alu_choice(op, choice);
      else
         vcos_assert(choice == CHOICE_STANDARD);

      //assert(*input_detail.op != ~0);
      vcos_assert(input_detail.op == NULL);
   }
   else
   {
      vcos_assert(choice == CHOICE_STANDARD);
      if (input_is_from_instr(input))
      {
         //assert(*input_detail.op != ~0);
         vcos_assert(input_detail.op == NULL);
      }
      else
      {
         if (input_detail.op)
         {
            vcos_assert(*input_detail.op == ~0);
            vcos_assert(consumer != (OP_T)~0);
            *input_detail.op = consumer;
         }
      }
   }
}

static FLAVOUR_TYPE_T get_flavour_type(OP_FLAVOUR_T flavour)
{
   switch (flavour)
   {
   case FLAVOUR_FADD:
   case FLAVOUR_FSUB:
   case FLAVOUR_FMIN:
   case FLAVOUR_FMAX:
   case FLAVOUR_FMINABS:
   case FLAVOUR_FMAXABS:
      return FLAVOUR_TYPE_ADD_F;
   case FLAVOUR_FTOI:
      return FLAVOUR_TYPE_FTOI;
   case FLAVOUR_ITOF:
      return FLAVOUR_TYPE_ITOF;
   case FLAVOUR_ADD:
   case FLAVOUR_SUB:
   case FLAVOUR_SHR:
   case FLAVOUR_ASR:
   case FLAVOUR_ROR:
   case FLAVOUR_SHL:
   case FLAVOUR_MIN:
   case FLAVOUR_MAX:
   case FLAVOUR_AND:
   case FLAVOUR_OR:
   case FLAVOUR_XOR:
   case FLAVOUR_NOT:
   case FLAVOUR_CLZ:
      return FLAVOUR_TYPE_ADD_I;
   case FLAVOUR_FMUL:
      return FLAVOUR_TYPE_MUL_F;
   case FLAVOUR_MUL24:
   case FLAVOUR_V8MULD:
   case FLAVOUR_V8MIN:
   case FLAVOUR_V8MAX:
      return FLAVOUR_TYPE_MUL_I;
   case FLAVOUR_MOV:
   case FLAVOUR_V8ADDS:
   case FLAVOUR_V8SUBS:
      return FLAVOUR_TYPE_MOV_I;
   default:
      UNREACHABLE();
      return 0;
   }
}

static INSTR_CONFLICT_T get_alu_conflict(OP_FLAVOUR_T flavour, uint32_t half)
{
   switch (get_flavour_type(flavour))
   {
   case FLAVOUR_TYPE_ADD_F: return DECISION_ADD(half) + DECISION_IN_FLOAT(half) + DECISION_OUT_FLOAT(half);
   case FLAVOUR_TYPE_FTOI:  return DECISION_ADD(half) + DECISION_IN_FLOAT(half) + DECISION_OUT_INTEGER(half);
   case FLAVOUR_TYPE_ITOF:  return DECISION_ADD(half) + DECISION_IN_INTEGER(half) + DECISION_OUT_FLOAT(half);
   case FLAVOUR_TYPE_ADD_I: return DECISION_ADD(half) + DECISION_IN_INTEGER(half) + DECISION_OUT_INTEGER(half);
   case FLAVOUR_TYPE_MUL_F: return DECISION_MUL(half) + DECISION_IN_FLOAT(half) + DECISION_OUT_FLOAT(half);
   case FLAVOUR_TYPE_MUL_I: return DECISION_MUL(half) + DECISION_IN_INTEGER(half) + DECISION_OUT_INTEGER(half);
   case FLAVOUR_TYPE_MOV_I: return DECISION_IN_INTEGER(half) + DECISION_OUT_INTEGER(half);
   default:
      UNREACHABLE();
      return 0;
   }
}

static bool flavour_is_add(OP_FLAVOUR_T flavour)
{
   switch (get_flavour_type(flavour))
   {
   case FLAVOUR_TYPE_ADD_F:
   case FLAVOUR_TYPE_FTOI:
   case FLAVOUR_TYPE_ITOF:
   case FLAVOUR_TYPE_ADD_I:
      return true;
   case FLAVOUR_TYPE_MUL_F:
   case FLAVOUR_TYPE_MUL_I:
   case FLAVOUR_TYPE_MOV_I:
      return false;
   default:
      UNREACHABLE();
      return 0;
   }
}

static bool flavour_is_mul(OP_FLAVOUR_T flavour)
{
   switch (get_flavour_type(flavour))
   {
   case FLAVOUR_TYPE_ADD_F:
   case FLAVOUR_TYPE_FTOI:
   case FLAVOUR_TYPE_ITOF:
   case FLAVOUR_TYPE_ADD_I:
   case FLAVOUR_TYPE_MOV_I:
      return false;
   case FLAVOUR_TYPE_MUL_F:
   case FLAVOUR_TYPE_MUL_I:
      return true;
   default:
      UNREACHABLE();
      return 0;
   }
}

static INSTR_CONFLICT_T get_sig_conflict(INSTR_SIG_T sig)
{
   switch (sig)
   {
   case SIG_SB_WAIT:
      return 0;
   case SIG_THRSW:
      return 0;
   case SIG_TMU0:
   case SIG_TMU1:
#ifdef WORKAROUND_HW2522
      return CONFLICT_CCHW_TLBR + CONFLICT_CCHW_TLBW + CONFLICT_VPM_READ;
#else
      return CONFLICT_CCHW_TLBR + CONFLICT_CCHW_TLBW;
#endif
   case SIG_GET_COL:
#ifdef WORKAROUND_HW2488
      return CONFLICT_CCHW_TLBR + CONFLICT_CCHW_TLBW;
#else
      return CONFLICT_CCHW_TLBR;
#endif
   default:
      //TODO
      UNREACHABLE();
      return ~0;
   }
}

static bool check_flags_source(OP_SRC_DETAIL_T flags_src_detail)
{
   uint32_t source_timestamp;
   INSTR_CONFLICT_T conflict;
   uint32_t source_half;
   OP_STRUCT_T *source_struct;
   OP_T flags_source;

   if (flags_src_detail.src == (OP_SRC_T)~0)
      return true;    /* No flags_source so nothing to do */

   if (!input_is_from_op_dest(flags_src_detail.src))
      return false;   /* Can only set flags from op dest */

   flags_source = get_input_op(flags_src_detail.src);

   source_timestamp = get_op_timestamp(flags_source);
   source_half = get_op_half(flags_source);

   if (flags_node == flags_source)
      return true;     // Nothing further to check if the current flags node is the one we want
   if (flags_available > source_timestamp)
      return false;    // Fail if someone else is consuming different flags before we want to produce ours

   source_struct = get_op_struct(flags_source);

   /*
      mov.zc.setf r0,r1,r2 does not do what we want. We'd want it to set flags
      on all elements, but it will only set them if the condition is true.
      So avoid generating this kind of instruction.
   */
   if (source_struct->cond != 0)
      return false;

   if (flavour_is_add(source_struct->flavour))
      return true;     // If we are the add node then we get priority on setting flags

   conflict = get_op_instr(flags_source)->conflict;
   if (!conflict_add(&conflict, DECISION_ADD(source_half)))
      return false;    // Other half has been forced to add node. So we can't set the flags.

   return true;
}

static void retire_input(OP_SRC_DETAIL_T input_detail, uint32_t timestamp, OP_T move);
static void commit_flags_source(OP_SRC_DETAIL_T flags_src_detail, OP_T op)
{
   OP_STRUCT_T *source_struct;
   uint32_t source_timestamp, op_timestamp;
   INSTR_CONFLICT_T conflict;
   INSTR_STRUCT_T *instr;
   uint32_t half;
   OP_T flags_source;

   if (flags_src_detail.src == (OP_SRC_T)~0)
      return;      /* Nothing to do */

   vcos_assert(input_is_from_op_dest(flags_src_detail.src));
   flags_source = get_input_op(flags_src_detail.src);

   source_timestamp = get_op_timestamp(flags_source);
   op_timestamp = get_op_timestamp(op);
   half = get_op_half(flags_source);

   vcos_assert(source_timestamp < op_timestamp);

   if (flags_node == flags_source)
   {
      if (flags_available < op_timestamp)
         flags_available = op_timestamp;
      return;     // Nothing further to do
   }

   source_struct = get_op_struct(flags_source);
   vcos_assert(!source_struct->set_flags);
   vcos_assert(flags_available <= source_timestamp);

   source_struct->set_flags = true;
   flags_node = flags_source;
   flags_available = op_timestamp;

   /*
      TODO: retire timestamp could be slightly earlier, i.e. the last time
      the regfile/acc was used, rather than the flags.
   */
   retire_input(flags_src_detail, op_timestamp, MOVE_RETIRED);

   if (flavour_is_add(source_struct->flavour))
      return;     // If we are the add node then we get priority on setting flags
   else if (flavour_is_mul(source_struct->flavour))
   {
      source_struct = get_instr_op_struct(source_timestamp, 1 - half);
      vcos_assert(source_struct->flavour == FLAVOUR_UNUSED);
      source_struct->flavour = FLAVOUR_NOP;    // If node appears in other half it will be forced to add, and pull flags off us
   }
   else
   {
      instr = get_op_instr(flags_source);
      conflict = instr->conflict;
      verify(conflict_add(&conflict, DECISION_ADD(half)));
      instr->conflict = conflict;
   }
}

static void retire_input(OP_SRC_DETAIL_T input_detail, uint32_t timestamp, OP_T move)
{
   bool retire = input_detail.retire;
   vcos_assert((move != (OP_T)MOVE_STILL_THERE) && (move != (OP_T)MOVE_NO_DEST));
   if (input_detail.is_new)
   {
      OP_SRC_T input;

      input = input_detail.src;
      if (input_is_from_op_dest(input))
      {
         OP_T op = get_input_op(input);
         OP_STRUCT_T *op_struct = get_op_struct(op);
         OP_DEST_T dest;
         uint32_t row;

         if (op_struct->move == MOVE_NO_DEST)
		 {
            vcos_assert(move == MOVE_RETIRED);
		 }
         else
         {
            vcos_assert(op_struct->move == (OP_T)MOVE_STILL_THERE);
            dest = op_struct->dest;

            if (dest_is_reg_a(dest))
            {
               row = get_dest_index(dest);
               vcos_assert(!a_is_available[row]);
               if (a_last_used[row] < timestamp)
                  a_last_used[row] = timestamp;
               if (retire)
                  a_is_available[row] = true;
            }
            else if (dest_is_reg_b(dest))
            {
               row = get_dest_index(dest);
               vcos_assert(!b_is_available[row]);
               if (b_last_used[row] < timestamp)
                  b_last_used[row] = timestamp;
               if (retire)
                  b_is_available[row] = true;
            }
            else if (dest_is_normal_acc(dest))
            {
               row = get_dest_index(dest);
               vcos_assert(!acc_is_available[row]);
               vcos_assert(acc_node[row] == op);
               if (acc_last_used[row] < timestamp)
                  acc_last_used[row] = timestamp;
               if (retire)
               {
                  acc_is_available[row] = true;
                  acc_node[row] = ~0;
               }
            }
            else
               UNREACHABLE();
         }

         if (retire)
            op_struct->move = move;
      }
      else if (input_is_from_instr_r4(input))
      {
         uint32_t input_timestamp = get_input_timestamp(input);
         INSTR_STRUCT_T *instr = get_instr(input_timestamp);

         vcos_assert(instr->move_r4 == (OP_T)MOVE_STILL_THERE);

         vcos_assert(!acc_is_available[4]);
         vcos_assert(get_op_timestamp(acc_node[4]) == input_timestamp);
         if (acc_last_used[4] < timestamp)
            acc_last_used[4] = timestamp;
         if (retire)
         {
            acc_is_available[4] = true;
            acc_node[4] = ~0;
            instr->move_r4 = move;
         }
      }
      else if (input_is_from_instr_r5(input))
      {
         uint32_t input_timestamp = get_input_timestamp(input);
         INSTR_STRUCT_T *instr = get_instr(input_timestamp);

         vcos_assert(instr->move_r5 == (OP_T)MOVE_STILL_THERE);

         vcos_assert(!acc_is_available[5]);
         vcos_assert(get_op_timestamp(acc_node[5]) == input_timestamp);
         if (acc_last_used[5] < timestamp)
            acc_last_used[5] = timestamp;
         if (retire)
         {
            acc_is_available[5] = true;
            acc_node[5] = ~0;
            instr->move_r5 = move;
         }
      }
      else
      {
         /* nothing to do */
      }
   }
}

static uint32_t spare_accs(uint32_t timestamp)
{
   uint32_t i, result;
   result = 0;
   for (i = 0; i < 4; i++)
   {
      if (acc_is_available[i] && acc_last_used[i] <= timestamp)
         result++;
   }
   return result;
}

#define ADD_SOMEWHERE_OK 1
#define ADD_SOMEWHERE_LEFT_BEFORE_UNPACK 2
#define ADD_SOMEWHERE_RIGHT_BEFORE_UNPACK 3
#define ADD_SOMEWHERE_LEFT_AFTER_UNPACK 4
#define ADD_SOMEWHERE_RIGHT_AFTER_UNPACK 5
#define ADD_SOMEWHERE_SEPARATE_INPUTS 6
static uint32_t can_be_added_somewhere(
   OP_SRC_DETAIL_T left_detail,
   OP_SRC_DETAIL_T right_detail,
   INSTR_CONFLICT_T extra_conflict,
   CHOICE_MASK_T *left_choices_out,
   CHOICE_MASK_T *right_choices_out,
   INSTR_CONFLICT_T *left_conflict,
   INSTR_CONFLICT_T *right_conflict,
   uint32_t *left_timestamps,
   uint32_t *right_timestamps,
   uint32_t *num_spare_accs)
{
   CHOICE_MASK_T left_choices, right_choices;
   uint32_t i, j, accs;
   bool valid_left, valid_instruction;

   if (right_detail.is_new
      && input_is_from_instr(left_detail.src)
      && input_without_unpack(left_detail.src) == input_without_unpack(right_detail.src))
   {
      /* This is bad: inputs come from the same place but with different unpacking modes. We should move one of them. */
      return ADD_SOMEWHERE_SEPARATE_INPUTS;
   }

   left_choices = get_input_choice(left_detail, left_conflict, left_timestamps);
   right_choices = get_input_choice(right_detail, right_conflict, right_timestamps);

   /*
      If there were no choices returned, it's probably because the unpack mode conflicts
      with the location. Move them now, allowing us to find other conflicts later. TODO:
      this might mean we sometimes move twice when we don't need to.
   */
   if (left_choices == 0)
      return ADD_SOMEWHERE_LEFT_BEFORE_UNPACK;
   if (right_choices == 0)
      return ADD_SOMEWHERE_RIGHT_BEFORE_UNPACK;

   valid_left = false;
   valid_instruction = false;

   *num_spare_accs = 4;
   if (input_is_from_instr(left_detail.src))
   {
      accs = spare_accs(get_input_timestamp(left_detail.src));
      if (accs < *num_spare_accs)
         *num_spare_accs = accs;
   }
   if (input_is_from_instr(right_detail.src))
   {
      accs = spare_accs(get_input_timestamp(right_detail.src));
      if (accs < *num_spare_accs)
         *num_spare_accs = accs;
   }

   for (i = 0; i < MAX_CHOICES; i++)
   if (left_choices & (1<<i))
   for (j = 0; j < MAX_CHOICES; j++)
   if (right_choices & (1<<j))
   {
      INSTR_CONFLICT_T conflict;

      conflict = extra_conflict;

      if (!conflict_add(&conflict, left_conflict[i])) continue;
      valid_left = true;

      if (!conflict_add(&conflict, right_conflict[j])) continue;

      if ((uint32_t)(i == CHOICE_ACC) + (uint32_t)(j == CHOICE_ACC) > *num_spare_accs) continue;
      valid_instruction = true;

      break;
   }

   if (valid_instruction)
   {
      *left_choices_out = left_choices;
      *right_choices_out = right_choices;
      return ADD_SOMEWHERE_OK;
   }
   else if (valid_left)
      return ADD_SOMEWHERE_RIGHT_AFTER_UNPACK;
   else
      return ADD_SOMEWHERE_LEFT_AFTER_UNPACK;
}

static OP_T op_add_alu(OP_FLAVOUR_T flavour, OP_SRC_DETAIL_T left_detail, OP_SRC_DETAIL_T right_detail, OP_SRC_T *deps, uint32_t dep_count, bool allow_mov, INSTR_CONFLICT_T extra_conflict, uint32_t first_timestamp);
static void free_up_acc(uint32_t i)
{
   OP_T old_op, new_op;
   OP_SRC_DETAIL_T src_detail;

   vcos_assert(!acc_is_available[i]);

   vcos_assert(acc_node[i] != (OP_T)~0);
   old_op = acc_node[i];
   if (i < 4)
   {
      src_detail = src_detail_from_op(old_op, true, 0);
      new_op = op_add_alu(FLAVOUR_MOV, src_detail, src_detail, NULL, 0, false, 0, 0);
      if (failed) return;

      vcos_assert(acc_is_available[i] && acc_node[i] == (OP_T)~0);
      vcos_assert(get_op_struct(old_op)->move == new_op);
   }
   else if (i == 4)
   {
      uint32_t old_timestamp = get_op_timestamp(old_op);

      src_detail = src_detail_from_src(input_from_instr_r4(old_timestamp, 0, true), true);
      new_op = op_add_alu(FLAVOUR_MOV, src_detail, src_detail, NULL, 0, false, 0, 0);
      if (failed) return;

      vcos_assert(acc_is_available[i] && acc_node[i] == (OP_T)~0);
      vcos_assert(get_instr(old_timestamp)->move_r4 == (OP_T)new_op);
   }
   else if (i == 5)
   {
      uint32_t old_timestamp = get_op_timestamp(old_op);

      src_detail = src_detail_from_src(input_from_instr_r5(old_timestamp, 0, true), true);
      new_op = op_add_alu(FLAVOUR_MOV, src_detail, src_detail, NULL, 0, false, 0, 0);
      if (failed) return;

      vcos_assert(acc_is_available[i] && acc_node[i] == (OP_T)~0);
      vcos_assert(get_instr(old_timestamp)->move_r5 == new_op);
   }
   else
      UNREACHABLE();
}

// Higher-level functions

static uint32_t generate(uint32_t *output);
static OP_T op_add_alu(OP_FLAVOUR_T flavour, OP_SRC_DETAIL_T left_detail, OP_SRC_DETAIL_T right_detail, OP_SRC_T *deps, uint32_t dep_count, bool allow_mov, INSTR_CONFLICT_T extra_conflict, uint32_t first_timestamp_in)
{
   CHOICE_MASK_T left_choices, right_choices;
   INSTR_CONFLICT_T left_conflict[MAX_CHOICES], right_conflict[MAX_CHOICES], alu_conflict, alu_and_extra;
   uint32_t left_timestamps[MAX_CHOICES], right_timestamps[MAX_CHOICES];
   uint32_t i, j, i0, j0;
   uint32_t timestamp, first_timestamp, last_timestamp;
   bool reads_varying;
   bool is_relocation;
   uint32_t num_spare_accs;

   alu_conflict = get_alu_conflict(flavour, DEFAULT_HALF);

   if (src_detail_equal(right_detail, left_detail))
      right_detail.is_new = false;

   alu_and_extra = alu_conflict;
   if (!conflict_add(&alu_and_extra, extra_conflict))
   {
      /* TODO: not sure how to handle this one */
      UNREACHABLE();
   }

   i = 0;
   while(1)
   {
      uint32_t result;
      OP_T op_mov;
      OP_SRC_DETAIL_T raw;
      UNPACK_T unpack;

      i++;
      if (i > 16)
      {
         /* If we've been around the loop too many times then something has gone wrong */
         /* Probably run out of regfile space. */
         failed = true;
         return 0;
      }

      /*
      Discover whether this can potentially be added somewhere
      or the inputs conflict with each other
      */
      result = can_be_added_somewhere(
         left_detail,
         right_detail,
         alu_and_extra,
         &left_choices,
         &right_choices,
         left_conflict,
         right_conflict,
         left_timestamps,
         right_timestamps,
         &num_spare_accs);

      if (result == ADD_SOMEWHERE_OK)
         break;

      if (!allow_mov)
      {
         /* Probably run out of regfile space. */
         failed = true;
         return 0;
      }

      if (result == ADD_SOMEWHERE_LEFT_BEFORE_UNPACK)
      {
         /* TODO: we may not need all of these deps */
         raw = src_detail_without_unpack(left_detail, true);
         unpack = get_input_unpack(left_detail.src);

         op_mov = op_add_alu(FLAVOUR_MOV, raw, raw, deps, dep_count, false, DECISION_WRITE_A(DEFAULT_HALF), first_timestamp_in);
         if (failed) return 0;
         left_detail = src_detail_from_op(op_mov, left_detail.retire, unpack);
      }
      else if (result == ADD_SOMEWHERE_RIGHT_BEFORE_UNPACK)
      {
         raw = src_detail_without_unpack(right_detail, true);
         unpack = get_input_unpack(right_detail.src);

         op_mov = op_add_alu(FLAVOUR_MOV, raw, raw, deps, dep_count, false, DECISION_WRITE_A(DEFAULT_HALF), first_timestamp_in);
         if (failed) return 0;
         right_detail = src_detail_from_op(op_mov, right_detail.retire, unpack);
      }
      else if (result == ADD_SOMEWHERE_LEFT_AFTER_UNPACK)
      {
         /* Left input conflicts with alu so must be moved */
         op_mov = op_add_alu(get_suitable_mov(left_detail.src), left_detail, left_detail, deps, dep_count, false, 0, first_timestamp_in);
         if (failed) return 0;
         left_detail = src_detail_from_op(op_mov, true, 0);
      }
      else if (result == ADD_SOMEWHERE_RIGHT_AFTER_UNPACK)
      {
         /* Right input conflicts with alu or left input so must be moved */
         op_mov = op_add_alu(get_suitable_mov(right_detail.src), right_detail, right_detail, deps, dep_count, false, 0, first_timestamp_in);
         if (failed) return 0;
         right_detail = src_detail_from_op(op_mov, true, 0);
      }
      else if (result == ADD_SOMEWHERE_SEPARATE_INPUTS)
      {
         /* Inputs come from the same place but with different unpacking modes */
         raw = src_detail_without_unpack(left_detail, false);
         unpack = get_input_unpack(left_detail.src);
         /* TODO: we are using FLAVOUR_OR rather than FLAVOUR_MOV as a hack to prevent is_relocation */
         op_mov = op_add_alu(FLAVOUR_OR, raw, raw, deps, dep_count, false, DECISION_WRITE_A(DEFAULT_HALF), first_timestamp_in);
         if (failed) return 0;
         left_detail = src_detail_from_op(op_mov, left_detail.retire, unpack);
      }
      else
         UNREACHABLE();
   }

   //Determine zone where we might consider adding instructions
   //This starts *after* all of the dependency nodes
   first_timestamp = get_first_timestamp();
   if (first_timestamp_in > first_timestamp)
      first_timestamp = first_timestamp_in;
   first_timestamp = move_after(first_timestamp, left_detail.src);
   first_timestamp = move_after(first_timestamp, right_detail.src);
   for (i = 0; i < dep_count; i++)
      first_timestamp = move_after(first_timestamp, deps[i]);

   /* Special case: varyings must be read after the last thing finished with r5 */
   reads_varying = input_without_unpack(left_detail.src) == INPUT_VARYING || input_without_unpack(right_detail.src) == INPUT_VARYING;
   if (reads_varying)
   {
      if (!acc_is_available[5])
      {
         vcos_assert(allow_mov);
         free_up_acc(5);
         if (failed) return 0;
      }
      if (acc_last_used[5] > first_timestamp)
         first_timestamp = acc_last_used[5];
   }

   last_timestamp = first_timestamp + FORWARDTRACK;
   vcos_assert(last_timestamp <= MAX_INSTRUCTIONS);

   /* See whether this is a relocation instruction, i.e. whether we preserve our input so it can be used later */
   is_relocation = (flavour == FLAVOUR_MOV) && get_input_unpack(left_detail.src) == 0;

   //Find somewhere to add this instruction where it doesn't conflict

   //TODO: sort out this big stack of loops
   for (timestamp = first_timestamp; timestamp < last_timestamp + 100; timestamp++)
   {
      OP_STRUCT_T *op_struct;
      INSTR_STRUCT_T *instr;
      uint32_t half;

      if (timestamp >= last_timestamp)
         UNREACHABLE();

      instr = get_instr(timestamp);

      //Find an empty half-instruction
      half = 0;
      op_struct = get_instr_op_struct(timestamp, half);
      if (op_struct->flavour != FLAVOUR_UNUSED)
      {
         half = 1;
         op_struct = get_instr_op_struct(timestamp, half);
         if (op_struct->flavour != FLAVOUR_UNUSED)
            continue;
      }

      // Find out whether these inputs have already been used in the other half of the instruction
      op_struct = get_instr_op_struct(timestamp, half);

      for (i0 = 0; i0 < MAX_CHOICES; i0++)
      {
         if (prefer_b && (i0 == CHOICE_A || i0 == CHOICE_B))
            i = i0 ^ CHOICE_A ^ CHOICE_B;
         else
            i = i0;

         if (left_choices & (1<<i))
         {
            for (j0 = 0; j0 < MAX_CHOICES; j0++)
            {
               if (prefer_b && (j0 == CHOICE_A || j0 == CHOICE_B))
                  j = j0 ^ CHOICE_A ^ CHOICE_B;
               else
                  j = j0;

               if (right_choices & (1<<j))
               {
                  INSTR_CONFLICT_T conflict, test_conflict;
                  OP_T op_result;

                  op_result = op_from_timestamp(timestamp, half);

                  if ((uint32_t)(i == CHOICE_ACC) + (uint32_t)(j == CHOICE_ACC) > num_spare_accs)
                  {
                     /* Not enough spare accumulators for this combination of choices */
                     continue;
                  }

                  if (left_timestamps[i] > timestamp || right_timestamps[j] > timestamp)
                     continue;

                  conflict = instr->conflict;
                  if (!conflict_add(&conflict, conflict_swap(left_conflict[i], half))) continue;
                  if (!conflict_add(&conflict, conflict_swap(right_conflict[j], half))) continue;
                  if (!conflict_add(&conflict, conflict_swap(alu_conflict, half))) continue;

                  test_conflict = conflict;
                  if (!conflict_add(&test_conflict, conflict_swap(extra_conflict, half)))
                     continue;

                  //Looks like we've succeeded and this is a good place to add the instruction.
                  commit_input_choice(left_detail, i, op_result);
                  commit_input_choice(right_detail, j, op_result);

                  retire_input(left_detail, timestamp, is_relocation ? op_result : (OP_T)MOVE_RETIRED);
                  retire_input(right_detail, timestamp, is_relocation ? op_result : (OP_T)MOVE_RETIRED);

                  if (!right_detail.is_new) right_detail = left_detail;

                  instr->conflict = conflict;
                  op_struct->schedule_order = schedule_order++; /*DEBUG*/
                  op_struct->flavour = flavour;
                  op_struct->left = left_detail;
                  op_struct->right = right_detail;
                  op_struct->dest = DEST_UNCERTAIN;
                  op_struct->move = MOVE_NO_DEST;

                  if (timestamp > last_used_timestamp)
                     last_used_timestamp = timestamp;

                  if (reads_varying)
                  {
                     vcos_assert(instr->move_r5 == (OP_T)MOVE_NO_DEST);
                     instr->move_r5 = MOVE_STILL_THERE;

                     vcos_assert(acc_is_available[5] && acc_last_used[5] <= timestamp);
                     vcos_assert(acc_node[5] == (OP_T)~0);
                     acc_is_available[5] = false;
                     acc_last_used[5] = timestamp;
                     acc_node[5] = op_result;
                  }

                  return op_result;
               }
            }
         }
      }
   }

   //TODO: failed to insert instruction
   UNREACHABLE();
   return 0;
}

static uint32_t instr_add_sig(INSTR_SIG_T sig, OP_SRC_T *deps, uint32_t dep_count)
{
   uint32_t timestamp, first_timestamp, last_timestamp;
   uint32_t i;

   first_timestamp = get_first_timestamp();
   for (i = 0; i < dep_count; i++)
   {
      first_timestamp = move_after(first_timestamp, deps[i]);

#ifdef ALLOW_TEXTURE_LATENCY
      /* special hack: give plenty of room between texture write and texture read in non-threaded shaders */
      if (
         !shader_is_threaded &&
         (sig == SIG_TMU0 || sig == SIG_TMU1) &&
         input_is_from_op_dest(deps[i]) &&
         dest_is_texture_write_retiring(get_op_struct(get_input_op(deps[i]))->dest))
      {
         uint32_t dep_timestamp = get_input_timestamp(deps[i]) + ALLOW_TEXTURE_LATENCY;
         if (dep_timestamp > first_timestamp)
            first_timestamp = dep_timestamp;
      }
#endif
   }
   /* special hack: sbwait can't be the first instruction */
   if (sig == SIG_SB_WAIT && first_timestamp == 0) first_timestamp = 1;

   /* thrsw must occur after we have finished with all of our accumulators */
   if (sig == SIG_THRSW)
   {
#ifdef WORKAROUND_HW2384
      vcos_assert(first_timestamp > 0);
      if (
         get_op_struct(op_from_timestamp(first_timestamp-1,0))->dest == DEST_EITHER(56) ||
         get_op_struct(op_from_timestamp(first_timestamp-1,1))->dest == DEST_EITHER(56))
      {
         first_timestamp++;
      }
#endif

      for (i = 0; i <= 5; i++)
      {
         if (!acc_is_available[i])
            free_up_acc(i);

         if (failed) return 0;

         if (acc_last_used[i] + 1 > first_timestamp)
            first_timestamp = acc_last_used[i] + 1;
      }
      if (flags_available + 1 > first_timestamp)
         first_timestamp = flags_available + 1;

      /*
         thrsw timestamp is for last instruction of previous block.
         Things that depend on it can then be placed in the first instruction of new block.
      */
      vcos_assert(first_timestamp > 0);
      first_timestamp--;
   }

   if (sig == SIG_GET_COL || sig == SIG_TMU0 || sig == SIG_TMU1)
   {
      if (!acc_is_available[4])
         free_up_acc(4);

      if (failed) return 0;
      if (acc_last_used[4] > first_timestamp)
         first_timestamp = acc_last_used[4];
   }

   last_timestamp = first_timestamp + FORWARDTRACK;
   vcos_assert(last_timestamp <= MAX_INSTRUCTIONS);

   for (timestamp = first_timestamp; timestamp < last_timestamp; timestamp++)
   {
      INSTR_STRUCT_T *instr;
      INSTR_CONFLICT_T conflict;

      if (sig == SIG_THRSW)
      {
         /*
            timestamp refers to the first instruction in the new thread block.
            The thrsw sig gets added three instructions earlier.
            TODO: make sure two thread switches don't get too close together?
         */
         if (timestamp < 2) continue;
         instr = get_instr(timestamp - 2);
      }
      else
         instr = get_instr(timestamp);

      if (instr->sig != SIG_UNUSED)
         continue;

      // Check conflicts
      conflict = instr->conflict;
      if (!conflict_add(&conflict, get_sig_conflict(sig)))
         continue;

      instr->conflict = conflict;
      instr->sig = sig;

      if (timestamp > last_used_timestamp)
         last_used_timestamp = timestamp;

      if (sig == SIG_GET_COL || sig == SIG_TMU0 || sig == SIG_TMU1)
      {
         vcos_assert(acc_is_available[4] && acc_last_used[4] <= timestamp);
         vcos_assert(acc_node[4] == (OP_T)~0);
         acc_is_available[4] = false;
         acc_last_used[4] = timestamp;
         acc_node[4] = op_from_timestamp(timestamp, 0);   /* TODO: this is badness. We don't know which op it is. */

         vcos_assert(instr->move_r4 == (OP_T)MOVE_NO_DEST);
         instr->move_r4 = MOVE_STILL_THERE;
      }

      if (sig == SIG_THRSW)
      {
         for (i = 0; i <= 5; i++)
         {
            vcos_assert(acc_is_available[i] && acc_last_used[i] < timestamp+1);
            acc_last_used[i] = timestamp+1;
         }
         vcos_assert(flags_available < timestamp+1);
         flags_available = timestamp+1;
         flags_node = ~0;

#ifndef WORKAROUND_HW1632
         vcos_assert((last_thrsw == (OP_T)~0) || (timestamp - 2 > last_thrsw));
         last_thrsw = timestamp - 2;
#endif
      }

      return timestamp;
   }

   //TODO: failed to insert instruction
   UNREACHABLE();
   return 0;
}

// conditionals:
// dest = get_op_struct(target)->dest;

static OP_T op_add_output(OP_SRC_DETAIL_T input_detail, OP_DEST_T dest, uint32_t dest_detail0, uint32_t dest_detail1, OP_SRC_T *deps, uint32_t dep_count, OP_COND_T cond, OP_SRC_DETAIL_T flags_source, bool allow_mov)
{
   INSTR_STRUCT_T *instr = NULL;
   OP_STRUCT_T *op_struct = NULL;
   INSTR_CONFLICT_T conflict;
   bool is_op, ok_dest_conflict = false, ok_only_consumer = false, ok_timestamp = false;
   OP_T op = 0;
   OP_SRC_T input;
   uint32_t i;
   uint32_t first_timestamp;
   UNUSED_NDEBUG(allow_mov);

   input = input_detail.src;

   first_timestamp = 0;

#ifndef KHRN_HW_SINGLE_TEXTURE_UNIT
   //XXX TODO: can't write to TMU in first two instructions due to swapping (HW-2615)
   if (dest_is_texture_write(dest) || dest_is_texture_direct(dest))
      first_timestamp = 2;
   //XXX
#endif

   for (i = 0; i < dep_count; i++)
      first_timestamp = move_after(first_timestamp, deps[i]);

   if (dest_is_sfu(dest))
   {
      if (!acc_is_available[4])
      {
         /* TODO: this is ugly and messy. Move the input if it's the same as the r4 node we're moving. */
         bool input_is_r4 = input_is_from_instr_r4(input);
         vcos_assert(!input_is_r4 || get_input_timestamp(input) == get_op_timestamp(acc_node[4]));

         free_up_acc(4);
         if (failed) return 0;

         if (input_is_r4)
         {
            input_detail = src_detail_from_op(get_instr(get_input_timestamp(input))->move_r4, input_detail.retire, get_input_unpack(input_detail.src));
            input = input_detail.src;
         }
      }

      if (acc_last_used[4] > first_timestamp)
         first_timestamp = acc_last_used[4];
   }

   if (flags_source.src != (OP_SRC_T)~0)
   {
      bool input_is_flags_source =
         input_is_from_op_dest(flags_source.src) &&
         input == input_from_op(get_input_op(flags_source.src), get_input_unpack(input));

      if (!check_flags_source(flags_source))
      {
         OP_SRC_DETAIL_T raw;
         OP_T new_op;

         /* Move flags_source */
         vcos_assert(allow_mov);
         vcos_assert(get_input_unpack(flags_source.src) == 0);
         raw = src_detail_without_unpack(flags_source, true);
         new_op = op_add_alu(FLAVOUR_MOV, raw, raw, NULL, 0, false, DECISION_ADD(DEFAULT_HALF), flags_available);
         if (failed) return 0;
         flags_source = src_detail_from_op(new_op, flags_source.retire, 0);
         vcos_assert(check_flags_source(flags_source));

         /* TODO: this is ugly and messy. Move the input if it's the same as the flags node we've just moved. */
         if (input_is_flags_source)
         {
            input = input_from_op(new_op, get_input_unpack(input));
            input_detail.src = input;
         }
      }

      /* TODO: messiness continues here */
      if (input_is_flags_source)
      {
         input_detail.retire = false;
      }

      first_timestamp = move_after(first_timestamp, flags_source.src);
   }

   if (input_is_from_op_dest(input) && input == input_from_op(get_input_op(input), 0))
   {
      is_op = true;

      op = get_input_op(input);
      instr = get_op_instr(op);
      op_struct = get_op_struct(op);

      vcos_assert(dest != DEST_UNCERTAIN);

      conflict = instr->conflict;
   }
   else
   {
      is_op = false;
   }

   if (is_op)
   {
      ok_dest_conflict = conflict_add(&conflict, get_dest_conflict(dest, get_op_half(op)));

      ok_only_consumer = input_detail.retire && dest_is_uncertain(op_struct->dest);
      ok_timestamp = first_timestamp <= get_op_timestamp(op);
   }

   if (!is_op || !ok_dest_conflict || !ok_only_consumer || !ok_timestamp)
   {
      OP_FLAVOUR_T flavour;
      uint32_t pack_type;

      vcos_assert(allow_mov);

      pack_type = get_pack_type(get_dest_pack(dest));
      if (pack_type == PACK_TYPE_MUL)
         flavour = FLAVOUR_V8MIN;
      else if (pack_type == PACK_TYPE_A_F)
         flavour = FLAVOUR_FMIN;
      else
      {
         vcos_assert(pack_type == PACK_TYPE_A || pack_type == PACK_TYPE_A_I || pack_type == PACK_TYPE_NONE);
         flavour = FLAVOUR_MOV;
      }

      op = op_add_alu(flavour, input_detail, input_detail, deps, dep_count, true, get_dest_conflict(dest, DEFAULT_HALF), first_timestamp);
      if (failed) return 0;
      op_add_output(src_detail_from_op(op, true, 0), dest, dest_detail0, dest_detail1, deps, dep_count, cond, flags_source, false);
      return op;
   }

   vcos_assert(input_detail.detail0 == (uint32_t)~0 && input_detail.detail1 == (uint32_t)~0);
   vcos_assert(op_struct->dest == DEST_UNCERTAIN);

   instr->conflict = conflict;
   op_struct->dest = dest;
   op_struct->dest_detail0 = dest_detail0;
   op_struct->dest_detail1 = dest_detail1;

   if (dest_is_sfu(dest))
   {
      vcos_assert(acc_is_available[4] && acc_last_used[4] <= get_op_timestamp(op));
      vcos_assert(acc_node[4] == (OP_T)~0);
      acc_is_available[4] = false;
      acc_last_used[4] = get_op_timestamp(op);
      acc_node[4] = op;

      vcos_assert(instr->move_r4 == (OP_T)MOVE_NO_DEST);
      instr->move_r4 = MOVE_STILL_THERE;
   }

   op_struct->cond = cond;
   commit_flags_source(flags_source, op);

   return op;
}

static OP_T op_join_output(OP_SRC_DETAIL_T input_detail, OP_SRC_DETAIL_T join_detail, OP_DEST_T pack, OP_SRC_T *deps, uint32_t dep_count, OP_COND_T cond, OP_SRC_DETAIL_T flags_source, bool allow_mov)
{
   OP_STRUCT_T *join_struct = NULL, *result_struct;
   OP_SRC_T new_deps[MAX_DEPS];
   OP_T result;
   bool move = false;
   UNUSED_NDEBUG(allow_mov);

   /* TODO more hackiness */
   if (input_without_unpack(join_detail.src) == input_without_unpack(flags_source.src))
      join_detail.retire = false;

   vcos_assert(dest_is_uncertain(pack));

   if (join_detail.retire && input_is_from_op_dest(join_detail.src))
   {
      OP_T join = get_input_op(join_detail.src);
      join_struct = get_op_struct(join);

      if (get_input_unpack(join_detail.src) != UNPACK_NONE)
         move = true;
      else if (dest_is_uncertain(join_struct->dest))
      {
         INSTR_CONFLICT_T pack_conflict = get_dest_conflict(pack, DEFAULT_HALF);
         /* TODO: this is a bit ugly (and feels like we're sort of duplicating what we do in can_be_added_somewhere) */
         if (find_dest_acc(join) != (OP_T)~0 && conflict_add(&pack_conflict, get_dest_conflict(DEST_ACC(0), DEFAULT_HALF)))
            commit_alu_choice(join, CHOICE_ACC);
         else if (find_dest_a(join) != (OP_T)~0 && conflict_add(&pack_conflict, get_dest_conflict(DEST_A(0), DEFAULT_HALF)))
            commit_alu_choice(join, CHOICE_A);
         else if (find_dest_b(join) != (OP_T)~0 && conflict_add(&pack_conflict, get_dest_conflict(DEST_B(0), DEFAULT_HALF)))
            commit_alu_choice(join, CHOICE_B);
         else
            move = true;
      }
   }
   else
      move = true;

   if (move)
   {
      OP_T op_mov;
      if (!allow_mov) { failed = true; return 0; }
      op_mov = op_add_alu(FLAVOUR_MOV, join_detail, join_detail, NULL, 0, true, 0, 0);
      if (failed) return 0;
      join_detail = src_detail_from_op(op_mov, true, 0);
      return op_join_output(input_detail, join_detail, pack, deps, dep_count, cond, flags_source, false);
   }

   vcos_assert(join_struct->move == (OP_T)MOVE_STILL_THERE);
   join_struct->move = MOVE_RETIRED;

   vcos_assert(dep_count < MAX_DEPS);
   memcpy(new_deps, deps, sizeof(OP_SRC_T) * dep_count);
   new_deps[dep_count] = join_detail.src;

   result = op_add_output(input_detail, get_dest_without_pack(join_struct->dest)+get_dest_pack(pack), ~0, ~0, new_deps, dep_count + 1, cond, flags_source, true);
   if (failed) return 0;
   result_struct = get_op_struct(result);

   vcos_assert(result_struct->move == (OP_T)MOVE_NO_DEST);
   result_struct->move = MOVE_STILL_THERE;
   if (dest_is_normal_acc(join_struct->dest))
   {
      /* accumulator now refers to something different */
      uint32_t row = get_dest_index(join_struct->dest);
      vcos_assert(acc_node[row] == get_input_op(join_detail.src));
      acc_node[row] = result;
   }

   return result;
}

/*

Generator

*/

static uint32_t flavour_to_add_code(OP_FLAVOUR_T flavour)
{
   switch (flavour)
   {
   case FLAVOUR_FADD:
   case FLAVOUR_FSUB:
   case FLAVOUR_FMIN:
   case FLAVOUR_FMAX:
   case FLAVOUR_FMINABS:
   case FLAVOUR_FMAXABS:
   case FLAVOUR_FTOI:
   case FLAVOUR_ITOF:
   case FLAVOUR_ADD:
   case FLAVOUR_SUB:
   case FLAVOUR_SHR:
   case FLAVOUR_ASR:
   case FLAVOUR_ROR:
   case FLAVOUR_SHL:
   case FLAVOUR_MIN:
   case FLAVOUR_MAX:
   case FLAVOUR_AND:
   case FLAVOUR_OR:
   case FLAVOUR_XOR:
   case FLAVOUR_NOT:
   case FLAVOUR_CLZ:
   case FLAVOUR_FMUL:
      return flavour & 31;
   case FLAVOUR_UNUSED:
   case FLAVOUR_NOP:
      return 0;
   case FLAVOUR_MOV:
      return 21;      //or
   case FLAVOUR_V8ADDS:
      return 30;
   case FLAVOUR_V8SUBS:
      return 31;
   default:
      UNREACHABLE();
      return 0;
   }
}

static uint32_t flavour_to_mul_code(OP_FLAVOUR_T flavour)
{
   switch (flavour)
   {
   case FLAVOUR_FMUL:
   case FLAVOUR_MUL24:
   case FLAVOUR_V8MULD:
   case FLAVOUR_V8MIN:
   case FLAVOUR_V8MAX:
      return flavour & 7;
   case FLAVOUR_UNUSED:
   case FLAVOUR_NOP:
      return 0;
   case FLAVOUR_MOV:
      return 4;      //v8min
   case FLAVOUR_V8ADDS:
      return 6;
   case FLAVOUR_V8SUBS:
      return 7;
   default:
      UNREACHABLE();
      return 0;
   }
}

static void input_to_mux_code(
   OP_SRC_DETAIL_T input_detail,
   INSTR_CONFLICT_T conflict,
   uint32_t *mux,
   uint32_t *raddr_a,
   uint32_t *raddr_b,
   uint32_t *pm,
   uint32_t *unp,
   uint32_t *unif0,
   uint32_t *unif1,
   uint32_t *vary)
{
   uint32_t row;
   CHOICE_T choice = 0;
   UNPACK_T unpack;
   OP_SRC_T input;
   uint32_t detail0, detail1;

   input = input_detail.src;
   detail0 = input_detail.detail0;
   detail1 = input_detail.detail1;

   if (input_is_from_op_dest(input))
   {
      OP_T op = get_input_op(input);
      OP_STRUCT_T *op_struct = get_op_struct(op);
      OP_DEST_T dest = op_struct->dest;

      row = get_dest_index(dest);

      if (dest_is_normal_acc(dest))
         choice = CHOICE_ACC;
      else if (dest_is_reg_a(dest))
         choice = CHOICE_A;
      else if (dest_is_reg_b(dest))
         choice = CHOICE_B;
      else
         UNREACHABLE();
   }
   else if (input_is_from_instr_r4(input))
   {
      choice = CHOICE_ACC;
      row = 4;
   }
   else if (input_is_from_instr_r5(input))
   {
      choice = CHOICE_ACC;
      row = 5;
   }
   else
   {
      choice = get_special_input_choice(input);
      row = get_special_input_row(input);
   }
   unpack = get_input_unpack(input);

   if (choice == CHOICE_ACC)
   {
      *mux = row;

      if (unpack != UNPACK_NONE)
      {
         vcos_assert(row == 4);
         vcos_assert(*pm == (uint32_t)~0 || *pm == 1);
         *pm = 1;

         vcos_assert(*unp == (uint32_t)~0 || *unp == (unpack & 7));
         *unp = unpack & 7;
      }
   }
   else if (choice == CHOICE_A)
   {
      vcos_assert(row < 64);
      vcos_assert(conflict & CONFLICT_READ_A);

      vcos_assert(*raddr_a == (uint32_t)~0 || *raddr_a == row);
      *raddr_a = row;
      *mux = 6;

      if (unpack != UNPACK_NONE)
      {
         vcos_assert(*pm == (uint32_t)~0 || *pm == 0);
         *pm = 0;

         vcos_assert(*unp == (uint32_t)~0 || *unp == (unpack & 7));
         *unp = unpack & 7;
      }
   }
   else if (choice == CHOICE_B)
   {
      vcos_assert(row < 64);
      vcos_assert(conflict & CONFLICT_READ_B);
      vcos_assert(unpack == UNPACK_NONE);
      vcos_assert(*raddr_b == (uint32_t)~0 || *raddr_b == row);
      *raddr_b = row;
      *mux = 7;
   }
   else if (choice == CHOICE_EITHER)
   {
      vcos_assert(row < 64);
      vcos_assert(unpack == UNPACK_NONE);
      if (*raddr_a == row)
         choice = CHOICE_A;
      else if (*raddr_b == row)
         choice = CHOICE_B;
      else if (!(conflict & CONFLICT_READ_A) && *raddr_a == (uint32_t)~0)
         choice = CHOICE_A;
      else if (!(conflict & CONFLICT_READ_B) && *raddr_b == (uint32_t)~0)
         choice = CHOICE_B;
      else
         UNREACHABLE();

      if (choice == CHOICE_A)
      {
         *raddr_a = row;
         *mux = 6;
      }
      else
      {
         *raddr_b = row;
         *mux = 7;
      }
   }
   else
      UNREACHABLE();

   /* We carefully avoid acc here */
   if (row == 32)
   {
      vcos_assert((*unif0 == (uint32_t)~0 && *unif1 == (uint32_t)~0) || (*unif0 == detail0 && *unif1 == detail1));
      *unif0 = detail0;
      *unif1 = detail1;
   }
   else if (row == 35)
   {
      vcos_assert(*vary == (uint32_t)~0 || *vary == (uint32_t)detail0);
      vcos_assert(detail1 == (uint32_t)~0);
      *vary = detail0;
   }
   else
      vcos_assert(detail0 == (uint32_t)~0 && detail1 == (uint32_t)~0);
}

static void dest_to_waddr(
   OP_DEST_T dest,
   uint32_t dest_detail0,
   uint32_t dest_detail1,
   uint32_t *waddr,
   uint32_t *pm,
   uint32_t *pack,
   uint32_t *unif0,
   uint32_t *unif1,
   uint32_t *current_tex_param)
{
   if (dest_is_normal_acc(dest))
   {
      vcos_assert(get_dest_index(dest) < 4);
      *waddr = 32 + get_dest_index(dest);
   }
   else if (dest_is_a(dest) || dest_is_b(dest) || dest_is_either(dest))
   {
      *waddr = get_dest_index(dest);
   }
   else if (dest == DEST_UNCERTAIN)
   {
      *waddr = 39;   /* nop */
   }
   else
      UNREACHABLE();

   if (get_dest_pack(dest) != PACK_NONE)
   {
      uint32_t our_pack = get_dest_pack(dest) & 0x0f;
      uint32_t our_pm = (get_dest_pack(dest) & 0x30) == 0x30;

      vcos_assert(*pack == (uint32_t)~0);
      *pack = our_pack;

      vcos_assert(*pm == (uint32_t)~0 || *pm == our_pm);
      *pm = our_pm;
   }

   if (dest_is_texture_write(dest))
   {
      uint32_t tmu = dest >= DEST_EITHER(60);
      vcos_assert(*unif0 == (uint32_t)~0 && *unif1 == (uint32_t)~0);
      *unif0 = dest_detail0 + current_tex_param[tmu];
      *unif1 = dest_detail1;

      if (current_tex_param[tmu] != BACKEND_UNIFORM_TEX_NOT_USED)
         current_tex_param[tmu]++;

      if (dest_is_texture_write_retiring(dest))
         current_tex_param[tmu] = BACKEND_UNIFORM_TEX_PARAM0;
   }
}

static uint32_t sig_to_code(INSTR_SIG_T sig)
{
   return sig;
}

static uint32_t cond_to_code(OP_COND_T cond)
{
   if (cond == COND_ALWAYS)
      return 1;
   else
      return cond;
}

bool xxx_dummy_shader = false; /* DEBUG */
static uint32_t generate(uint32_t *output)
{
   uint32_t i;
   uint32_t current_tex_param[2] = {BACKEND_UNIFORM_TEX_PARAM0,BACKEND_UNIFORM_TEX_PARAM0};
   uint32_t *umap = uniform_map;
   uint32_t *output_start = output;
   uint32_t xxx_anop_count = 0, xxx_mnop_count = 0, xxx_amov_count = 0, xxx_mmov_count = 0;

   /* DEBUG */
   if (shader_type & GLSL_BACKEND_TYPE_FRAGMENT && xxx_dummy_shader)
   {
      *(output++) = 0x00010101; *(output++) = 0xe0020867; /* mov r1, 0x10101 */
      *(output++) = 0x000000ff; *(output++) = 0xe00208a7; /* mov r2, 0xff */
      *(output++) = 0x009e7000; *(output++) = 0x400009e7; /* nop; sbwait */
      *(output++) = 0x159cffc0; *(output++) = 0x10020b27; /* mov tlbz, z; */

      *(output++) = 0x009e7000; *(output++) = 0x800009e7; /* nop; loadc */
      *(output++) = 0x0c9c19c0; *(output++) = 0xd0020827; /* add r0, r4, 1 */
      *(output++) = 0x809e7001; *(output++) = 0x100049e0; /* nop; v8min r0, r0, r1 */
      *(output++) = 0x409e7002; *(output++) = 0x100049ed; /* nop; mul24 tlbm, r0, r2 */

      *(output++) = 0x009e7000; *(output++) = 0x800009e7; /* nop; loadc */
      *(output++) = 0x0c9c19c0; *(output++) = 0xd0020827; /* add r0, r4, 1 */
      *(output++) = 0x809e7001; *(output++) = 0x100049e0; /* nop; v8min r0, r0, r1 */
      *(output++) = 0x409e7002; *(output++) = 0x100049ed; /* nop; mul24 tlbm, r0, r2 */

      *(output++) = 0x009e7000; *(output++) = 0x800009e7; /* nop; loadc */
      *(output++) = 0x0c9c19c0; *(output++) = 0xd0020827; /* add r0, r4, 1 */
      *(output++) = 0x809e7001; *(output++) = 0x100049e0; /* nop; v8min r0, r0, r1 */
      *(output++) = 0x409e7002; *(output++) = 0x100049ed; /* nop; mul24 tlbm, r0, r2 */

      *(output++) = 0x009e7000; *(output++) = 0x800009e7; /* nop; loadc */
      *(output++) = 0x0c9c19c0; *(output++) = 0xd0020827; /* add r0, r4, 1 */
      *(output++) = 0x809e7001; *(output++) = 0x100049e0; /* nop; v8min r0, r0, r1 */
      *(output++) = 0x409e7002; *(output++) = 0x100049ed; /* nop; mul24 tlbm, r0, r2 */

      *(output++) = 0x009e7000; *(output++) = 0x500009e7; /* nop; sbdone */
      *(output++) = 0x009e7000; *(output++) = 0x300009e7; /* nop; thrend */
      *(output++) = 0x009e7000; *(output++) = 0x100009e7; /* nop */
      *(output++) = 0x009e7000; *(output++) = 0x100009e7; /* nop */
      varying_count = 0;
      uniform_count = (umap - uniform_map) / 2;
      return (output - output_start) / 2;
   }

   if (shader_type & GLSL_BACKEND_TYPE_OFFLINE_VERTEX)
   {

      *(output++) = 0x15827d80; *(output++) = 0x100207e7; /* mov ra31, unif */
      *(umap++) = BACKEND_UNIFORM; *(umap++) = BACKEND_UNIFORM_VDR_ADDR_START;

      *(output++) = 0x15827d80; *(output++) = 0x100217e7; /* mov rb31, unif */
      *(umap++) = BACKEND_UNIFORM; *(umap++) = BACKEND_UNIFORM_VDW_ADDR_START;

      /* start of loop */

      *(output++) = 0x15827d80; *(output++) = 0x10020c67; /*:label mov vr_setup, unif */
      *(umap++) = BACKEND_UNIFORM; *(umap++) = BACKEND_UNIFORM_VDR_SETUP0;

      *(output++) = 0x15827d80; *(output++) = 0x10020c67; /* mov vr_setup, unif */
      *(umap++) = BACKEND_UNIFORM; *(umap++) = BACKEND_UNIFORM_VDR_SETUP1;

      *(output++) = 0x157e7d80; *(output++) = 0x10020ca7; /* mov vr_addr, ra31 */

      *(output++) = 0x15ca7d80; *(output++) = 0x100009e7; /* mov -, vr_wait */
   }

#ifndef WORKAROUND_HW2924
   if (shader_type & GLSL_BACKEND_TYPE_FRAGMENT)
#else
   if ((shader_type & GLSL_BACKEND_TYPE_FRAGMENT) && !(shader_type & GLSL_BACKEND_TYPE_SIMPLE_HW2924) && !(shader_type & GLSL_BACKEND_TYPE_MULTISAMPLE_HW2924))
#endif
   {
      /* Add thrend and sbdone */
      uint32_t thrend_timestamp, sbdone_timestamp;
      INSTR_STRUCT_T *instr;

#ifdef WORKAROUND_HW1637
      /* This is easy - scoreboard bugs force last four instructions to be sbdone, thrend, nop, nop */
      sbdone_timestamp = last_used_timestamp + 1;
      thrend_timestamp = last_used_timestamp + 2;
#else
      uint32_t i, j;

      /* assess suitability */
      i = last_used_timestamp - 3;
      while (1)
      {
         bool bad = false;
         i++;

         vcos_assert(i <= last_used_timestamp + 1);

         if (get_instr(i)->sig != SIG_UNUSED && get_instr(i)->sig != SIG_GET_COL)
            continue;

         if (get_instr(i+2)->sig != SIG_UNUSED)
            continue;

#ifdef WORKAROUND_HW2806
         for (j = 0; j <= 4; j++)
         {
            if (i + j >= 1)
            {
               OP_STRUCT_T *op0, *op1;
               OP_DEST_T dest0, dest1;

               /* Avoid writing to TLB z in last 4 instructions */
               op0 = get_op_struct(op_from_timestamp(i+j-1, 0));
               op1 = get_op_struct(op_from_timestamp(i+j-1, 1));
               dest0 = (op0->flavour != FLAVOUR_UNUSED) ? get_dest_without_pack(op0->dest) : (OP_FLAVOUR_T)~0;
               dest1 = (op1->flavour != FLAVOUR_UNUSED) ? get_dest_without_pack(op1->dest) : (OP_FLAVOUR_T)~0;
               if (dest0 == DEST_EITHER(44) || dest1 == DEST_EITHER(44))
                  bad = true;
            }
         }
#endif

         for (j = 0; j <= 2; j++)
         {
            OP_STRUCT_T *op0, *op1;
            OP_DEST_T dest0, dest1;

            op0 = get_op_struct(op_from_timestamp(i+j, 0));
            op1 = get_op_struct(op_from_timestamp(i+j, 1));
            dest0 = (op0->flavour != FLAVOUR_UNUSED) ? get_dest_without_pack(op0->dest) : (OP_FLAVOUR_T)~0;
            dest1 = (op1->flavour != FLAVOUR_UNUSED) ? get_dest_without_pack(op1->dest) : (OP_FLAVOUR_T)~0;

            if (j == 0)
            {
               /* thrend instruction can't write to A or B regfile */
               if (dest_is_reg_a(dest0) || dest_is_reg_b(dest0) || dest_is_reg_a(dest1) || dest_is_reg_b(dest1))
                  bad = true;
            }
            else
            {
               /* instructions after thrend can't write to ra14 or rb14 */
               if (dest0 == DEST_A(14) || dest0 == DEST_B(14) || dest1 == DEST_A(14) || dest1 == DEST_B(14))
                  bad = true;
            }

            /* Can't read uniforms or varyings in last 3 instructions */

            if (op0->left.src == INPUT_UNIFORM || op0->right.src == INPUT_UNIFORM || op1->left.src == INPUT_UNIFORM || op1->right.src == INPUT_UNIFORM)
               bad = true;

            if (op0->left.src == INPUT_VARYING || op0->right.src == INPUT_VARYING || op1->left.src == INPUT_VARYING || op1->right.src == INPUT_VARYING)
               bad = true;
         }

         if (bad)
            continue;

         /* Nothing wrong with this location, so break */
         break;
      }
      thrend_timestamp = i;
      sbdone_timestamp = i + 2;
#endif

      vcos_assert(thrend_timestamp >= last_used_timestamp - 2);
      vcos_assert(sbdone_timestamp >= last_used_timestamp && sbdone_timestamp <= thrend_timestamp + 2);
      last_used_timestamp = thrend_timestamp + 2;

      instr = get_instr(thrend_timestamp);
      if (instr->sig == SIG_GET_COL)
         instr->sig = SIG_GET_COL_THREND;
      else
      {
         vcos_assert(instr->sig == SIG_UNUSED);
         instr->sig = SIG_THREND;
      }

      instr = get_instr(sbdone_timestamp);
      vcos_assert(instr->sig == SIG_UNUSED);
      instr->sig = SIG_SB_DONE;
   }

   for (i = 0; i <= last_used_timestamp; i++)
   {
      INSTR_STRUCT_T *instr;
      OP_STRUCT_T *op0, *op1;
      uint32_t raddr_a, raddr_b;
      uint32_t add_a, add_b, mul_a, mul_b;
      uint32_t pm, pack, unpack;
      uint32_t waddr_add, waddr_mul;
      uint32_t op_add, op_mul;
      uint32_t cond_add, cond_mul;
      uint32_t sig, ws, sf;
      uint32_t unif0, unif1, vary;

      instr = get_instr(i);
      op0 = get_instr_op_struct(i, 0);
      op1 = get_instr_op_struct(i, 1);

      ws = !!(instr->conflict & DECISION_WRITE_A1);
      if (instr->conflict & DECISION_ADD1)
      {
         OP_STRUCT_T *temp = op0;
         op0 = op1;
         op1 = temp;
         ws = 1 - ws;

         /* DEBUG (also a hack - now correspond to add/mul pipeline operation rather than the structure they're part of)*/
         op0->schedule_order ^= op1->schedule_order;
         op1->schedule_order ^= op0->schedule_order;
         op0->schedule_order ^= op1->schedule_order;
      }

      raddr_a = ~0;
      raddr_b = ~0;
      pm = ~0;
      pack = ~0;
      unpack = ~0;
      add_a = add_b = mul_a = mul_b = 0;
      waddr_add = waddr_mul = 39;
      cond_add = cond_mul = op_add = op_mul = 0;
      unif0 = unif1 = vary = ~0;

      if (op0->flavour != FLAVOUR_UNUSED && op0->flavour != FLAVOUR_NOP)
      {
         input_to_mux_code(op0->left,  instr->conflict, &add_a, &raddr_a, &raddr_b, &pm, &unpack, &unif0, &unif1, &vary);
         input_to_mux_code(op0->right, instr->conflict, &add_b, &raddr_a, &raddr_b, &pm, &unpack, &unif0, &unif1, &vary);
         dest_to_waddr(op0->dest, op0->dest_detail0, op0->dest_detail1, &waddr_add, &pm, &pack, &unif0, &unif1, current_tex_param);
         cond_add = cond_to_code(op0->cond);
         op_add = flavour_to_add_code(op0->flavour);
      }
      if (op1->flavour != FLAVOUR_UNUSED && op1->flavour != FLAVOUR_NOP)
      {
         input_to_mux_code(op1->left,  instr->conflict, &mul_a, &raddr_a, &raddr_b, &pm, &unpack, &unif0, &unif1, &vary);
         input_to_mux_code(op1->right, instr->conflict, &mul_b, &raddr_a, &raddr_b, &pm, &unpack, &unif0, &unif1, &vary);
         dest_to_waddr(op1->dest, op1->dest_detail0, op1->dest_detail1, &waddr_mul, &pm, &pack, &unif0, &unif1, current_tex_param);
         cond_mul = cond_to_code(op1->cond);
         op_mul = flavour_to_mul_code(op1->flavour);
      }

      /* TODO: not quite accurate - might be genuine OR or V8MIN */
      if (op_add == 0) xxx_anop_count++;
      if (op_mul == 0) xxx_mnop_count++;
      if (op_add == 21) xxx_amov_count++;
      if (op_mul == 4) xxx_mmov_count++;

      if (unpack == (uint32_t)~0) unpack = 0;
      if (raddr_a == (uint32_t)~0) raddr_a = 39;
      if (raddr_b == (uint32_t)~0) raddr_b = 39;
      if (pack == (uint32_t)~0) pack = 0;
      if (pm == (uint32_t)~0) pm = 0;

#ifndef WORKAROUND_HW1632
      if (i == last_thrsw)
      {
         vcos_assert(instr->sig == SIG_THRSW);
         sig = sig_to_code(SIG_LAST_THRSW);
      }
      else
         sig = sig_to_code(instr->sig);
#else
      sig = sig_to_code(instr->sig);
#endif

      sf = 0;
      if (op0->set_flags)
         sf = 1;
      if (op1->set_flags)
      {
         vcos_assert(op0->flavour == FLAVOUR_UNUSED);
         sf = 1;
      }

      vcos_assert(mul_b <= 7 && mul_a <= 7 && add_b <= 7 && add_a <= 7 && raddr_b <= 63 && raddr_a <= 63 && op_add <= 31 && op_mul <= 7);
      vcos_assert(waddr_mul <= 63 && waddr_add <= 63 && ws <= 1 && sf <= 1 && cond_mul <= 7 && cond_add <= 7 && pack <= 15 && pm <= 1 && unpack <= 7 && sig <= 15);

      *(output++) = mul_b | mul_a << 3 | add_b << 6 | add_a << 9 | raddr_b << 12 | raddr_a << 18 | op_add << 24 | op_mul << 29;
      *(output++) = waddr_mul | waddr_add << 6 | ws << 12 | sf << 13 | cond_mul << 14 | cond_add << 17 | pack << 20 | pm << 24 | unpack << 25 | sig << 28;

      if (unif0 != (uint32_t)~0 || unif1 != (uint32_t)~0)
      {
         vcos_assert((umap - uniform_map) / 2 < MAX_UNIFORMS);
         *(umap++) = unif0;
         *(umap++) = unif1;
      }
      if (vary != (uint32_t)~0)
      {
         if (vary >= 32)   /* Don't write out point coord varyings (0 and 1) */
         {
            vcos_assert(varying_count < MAX_UNIFORMS);
            varying_map[varying_count] = vary - 32;
            varying_count++;
         }
      }
   }

   /* DEBUG */
   /*if (shader_type & GLSL_BACKEND_TYPE_FRAGMENT)
   {
      extern uint32_t xxx_shader;
      uint32_t total = (output - output_start)/2;
      printf("shader%4d -- add:%3d mul:%3d amov:%3d mmov:%3d anop:%3d mnop:%3d instr:%3d\n",
         xxx_shader,
         total-xxx_amov_count-xxx_anop_count,
         total-xxx_mmov_count-xxx_mnop_count,
         xxx_amov_count, xxx_mmov_count, xxx_anop_count, xxx_mnop_count,
         total);
   }*/

   if (shader_type & GLSL_BACKEND_TYPE_OFFLINE_VERTEX)
   {
      uint32_t jump;

      *(output++) = 0x0d7e0dc0; *(output++) = 0x100229e7; /* sub.setf -, ra31, unif */
      *(umap++) = BACKEND_UNIFORM; *(umap++) = BACKEND_UNIFORM_VDR_ADDR_END;

      *(output++) = 0x957e0dbf; *(output++) = 0x10024831; /* mov r0, ra31; mov vw_setup, unif */
      *(umap++) = BACKEND_UNIFORM; *(umap++) = BACKEND_UNIFORM_VDW_SETUP0;

      /*
         jump back over everything so far
            except the first two instructions
            plus an extra four instructions (the branch instruction itself plus 3 delay slots)
      */
      jump = (uint32_t)output_start - (uint32_t)output - 2*8;
      *(output++) = jump; *(output++) = 0xf06809e7; /* brr.anyn -,r:label */

      *(output++) = 0x9581fff6; *(output++) = 0x10024871; /* mov r1, rb31; mov vw_setup, unif */
      *(umap++) = BACKEND_UNIFORM; *(umap++) = BACKEND_UNIFORM_VDW_SETUP1;

      *(output++) = 0x8c81f1bf; *(output++) = 0x100247f2; /* add ra31, r0, unif; mov vw_addr, rb31 */
      *(umap++) = BACKEND_UNIFORM; *(umap++) = BACKEND_UNIFORM_VDR_ADDR_INCR;

      *(output++) = 0x8c8323bf; *(output++) = 0x100217e7; /* add rb31, r1, unif; mov -, vw_wait */
      *(umap++) = BACKEND_UNIFORM; *(umap++) = BACKEND_UNIFORM_VDW_ADDR_INCR;

      /* end of loop */

      *(output++) = 0x15827d80; *(output++) = 0x100207e7; /* mov ra31, unif */
      *(umap++) = BACKEND_UNIFORM; *(umap++) = BACKEND_UNIFORM_NEXT_USER_SHADER;

      *(output++) = 0x009e7000; *(output++) = 0x100009e7; /* nop */
      *(output++) = 0x00000000; *(output++) = 0xf0f7e9e7; /* bra -, ra31 */

      *(output++) = 0x15827d80; *(output++) = 0x10020a27; /* mov unif_addr, unif */
      *(umap++) = BACKEND_UNIFORM; *(umap++) = BACKEND_UNIFORM_NEXT_USER_UNIF;

      *(output++) = 0x009e7000; *(output++) = 0x100009e7; /* nop */
      *(output++) = 0x009e7000; *(output++) = 0x100009e7; /* nop */
   }

#ifdef WORKAROUND_HW2924
   if (shader_type & GLSL_BACKEND_TYPE_SIMPLE_HW2924)
   {
      memcpy(output, simple_hw2924, sizeof(simple_hw2924));
      output += sizeof(simple_hw2924) / sizeof(uint32_t);
   }
   if (shader_type & GLSL_BACKEND_TYPE_MULTISAMPLE_HW2924)
   {
      memcpy(output, multisample_hw2924, sizeof(multisample_hw2924));
      output += sizeof(multisample_hw2924) / sizeof(uint32_t);
   }
#endif

   vcos_assert(current_tex_param[0] == BACKEND_UNIFORM_TEX_PARAM0 && current_tex_param[1] == BACKEND_UNIFORM_TEX_PARAM0);
   uniform_count = (umap - uniform_map) / 2;

   return (output - output_start) / 2;
}

static OP_T find_node(Dataflow *dataflow)
{
   vcos_assert(dataflow->slot != ~0);
   return dataflow->slot;
}

static bool dataflow_is_unpack(Dataflow *dataflow)
{
   switch (dataflow->flavour)
   {
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
         return true;
      default:
         return false;
   }
}

static OP_SRC_DETAIL_T find_src(Dataflow *dataflow, bool track, bool actually_care)
{
   DataflowChainNode *node, *subnode;
   OP_SRC_DETAIL_T result;
   OP_SRC_T src;
   uint32_t detail0, detail1;
   int *op;
   bool retire;

   detail0 = ~0;
   detail1 = ~0;
   op = NULL;

   /*
      Mark input node for retirement if it has no dependents which are yet to be scheduled.
      (This doesn't include the current node, which has already been marked as scheduled).
   */
   retire = true;
   for (node = dataflow->dependents.first; node; node = node->next)
   {
      if (node->dataflow->phase == BACKEND_PASS_AWAITING_SCHEDULE || node->dataflow->phase == BACKEND_PASS_DIVING)
         retire = false;

      if (dataflow_is_unpack(node->dataflow))
      {
         /* TODO: double unpack?? */
         for (subnode = node->dataflow->dependents.first; subnode; subnode = subnode->next)
         {
            if (subnode->dataflow->phase == BACKEND_PASS_AWAITING_SCHEDULE || subnode->dataflow->phase == BACKEND_PASS_DIVING)
               retire = false;
         }
      }
   }

   switch (dataflow->flavour)
   {
      case DATAFLOW_CONST_BOOL:
         detail0 = BACKEND_UNIFORM_LITERAL;
         detail1 = dataflow->u.const_bool.value;
         src = INPUT_UNIFORM;
         break;
      case DATAFLOW_CONST_INT:
         detail0 = BACKEND_UNIFORM_LITERAL;
         detail1 = const_float_from_int(dataflow->u.const_int.value);
         src = INPUT_UNIFORM;
         break;
      case DATAFLOW_CONST_FLOAT:
         detail0 = BACKEND_UNIFORM_LITERAL;
         detail1 = dataflow->u.const_float.value;
         src = INPUT_UNIFORM;
         break;
      case DATAFLOW_UNIFORM:
         detail0 = BACKEND_UNIFORM;
         detail1 = dataflow->u.linkable_value.row;
         src = INPUT_UNIFORM;
         break;
      case DATAFLOW_ATTRIBUTE:
         src = INPUT_VPM;
         break;
      case DATAFLOW_VARYING:
         /* TODO: get rid of this +32 thing */
         vcos_assert((dataflow->u.linkable_value.row <= 1) || (dataflow->u.linkable_value.row >= 32 && dataflow->u.linkable_value.row < 64));
         detail0 = dataflow->u.linkable_value.row;
         src = INPUT_VARYING;
         break;
      case DATAFLOW_INTRINSIC_RSQRT:
      case DATAFLOW_INTRINSIC_RCP:
      case DATAFLOW_INTRINSIC_LOG2:
      case DATAFLOW_INTRINSIC_EXP2:
         src = input_from_instr_r4(get_op_timestamp(find_node(dataflow)), 0, !track && actually_care);
         break;
      case DATAFLOW_FRAG_GET_X:
         src = INPUT_X_COORD;
         break;
      case DATAFLOW_FRAG_GET_Y:
         src = INPUT_Y_COORD;
         break;
      case DATAFLOW_FRAG_GET_Z:
         src = INPUT_Z;
         break;
      case DATAFLOW_FRAG_GET_W:
         src = INPUT_W;
         break;
      case DATAFLOW_FRAG_GET_FF:
         src = INPUT_REV_FLAG;
         break;
      case DATAFLOW_TEX_GET_CMP_R:
         src = input_from_instr_r4(get_op_timestamp(find_node(dataflow)), 0, !track && actually_care);
         break;
      case DATAFLOW_VARYING_C:
         vcos_assert(dataflow->iodependencies.first && !dataflow->iodependencies.first->next);
         src = input_from_instr_r5(get_op_timestamp(find_node(dataflow->iodependencies.first->dataflow)), 0, !track && actually_care);
         break;
      case DATAFLOW_FRAG_GET_COL:
         src = input_from_instr_r4(get_op_timestamp(find_node(dataflow)), 0, !track && actually_care);
         break;
      case DATAFLOW_UNPACK_COL_R:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), UNPACK_8A_F);   /* TODO: what if there's already an unpack?? */
      case DATAFLOW_UNPACK_COL_G:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), UNPACK_8B_F);
      case DATAFLOW_UNPACK_COL_B:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), UNPACK_8C_F);
      case DATAFLOW_UNPACK_COL_A:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), UNPACK_8D_F);
      case DATAFLOW_UNPACK_16A:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), UNPACK_16A);
      case DATAFLOW_UNPACK_16A_F:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), UNPACK_16A_F);
      case DATAFLOW_UNPACK_16B:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), UNPACK_16B);
      case DATAFLOW_UNPACK_16B_F:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), UNPACK_16B_F);
      case DATAFLOW_UNPACK_8A:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), UNPACK_8A);
      case DATAFLOW_UNPACK_8B:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), UNPACK_8B);
      case DATAFLOW_UNPACK_8C:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), UNPACK_8C);
      case DATAFLOW_UNPACK_8D:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), UNPACK_8D);
      case DATAFLOW_UNPACK_8R:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), UNPACK_8R);
      case DATAFLOW_UNPACK_FB_R:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), fb_rb_swap ? UNPACK_8C_F : UNPACK_8A_F);
      case DATAFLOW_UNPACK_FB_B:
         return src_detail_with_unpack(find_src(dataflow->u.unary_op.operand, track, actually_care), fb_rb_swap ? UNPACK_8A_F : UNPACK_8C_F);
#ifndef __BCM2708A0__
      case DATAFLOW_UNIFORM_ADDRESS:
         detail0 = BACKEND_UNIFORM_ADDRESS;
         detail1 = dataflow->u.indexed_uniform_sampler.size << 16 |
            dataflow->u.indexed_uniform_sampler.uniform->u.linkable_value.row;
         src = INPUT_UNIFORM;
         break;
#endif
      default:
         src = input_from_op(find_node(dataflow), 0);
   }

   if (!input_is_from_instr(src))
   {
      if (dataflow->slot != ~0)
      {
#ifndef NDEBUG
         OP_STRUCT_T *op_struct = get_op_struct(dataflow->slot);
         if (actually_care)
         {
            vcos_assert(op_struct->flavour == FLAVOUR_MOV && get_input_unpack(op_struct->left.src) == UNPACK_NONE);
         }
#endif
         src = input_from_op(dataflow->slot, 0);
         detail0 = detail1 = ~0;
      }
      else
      {
         op = &dataflow->slot;
      }
   }

   if (src == INPUT_UNIFORM)
   {
      /* DECISION: what happens if we read from the same uniform several times? */
      retire = true;
      op = NULL;
   }


   if (track)
   {
      /* Track relocated nodes */
      while (1)
      {
         OP_T move;

         if (input_is_from_op_dest(src))
            move = get_op_struct(get_input_op(src))->move;
         else if (input_is_from_instr_r4(src))
            move = get_instr(get_input_timestamp(src))->move_r4;
         else if (input_is_from_instr_r5(src))
            move = get_instr(get_input_timestamp(src))->move_r5;
         else
            break;

         op = NULL;
         vcos_assert(move != (OP_T)MOVE_RETIRED);
         if (move == (OP_T)MOVE_NO_DEST) break;
         if (move == (OP_T)MOVE_STILL_THERE) break;
         //if (get_op_timestamp(move) >= get_op_timestamp(consumer)) break;
         src = input_from_op(move, get_input_unpack(src));
         detail0 = detail1 = (uint32_t)~0;
      }
   }

   result.src = src;
   result.detail0 = detail0;
   result.detail1 = detail1;
   result.retire = retire;
   result.is_new = true;
   result.op = op;
   return result;
}

/*

API

*/

void glsl_allocator_init(uint32_t type, bool is_threaded, uint32_t num_attribute_rows, bool fb_rb_swap_in)
{
   uint32_t i;

   UNUSED(num_attribute_rows);

   schedule_order = 0;/*DEBUG*/

   khrn_memset(op_struct, 0, sizeof(op_struct));
   khrn_memset(instr_struct, 0, sizeof(instr_struct));

   for (i = 0; i < MAX_INSTRUCTIONS; i++)
   {
      instr_struct[i].sig = SIG_UNUSED;
      instr_struct[i].move_r4 = MOVE_NO_DEST;
      instr_struct[i].move_r5 = MOVE_NO_DEST;
   }

   last_used_timestamp = 0;
   uniform_count = 0;
   varying_count = 0;

   for (i = 0; i <= 5; i++)
      acc_is_available[i] = true;
   for (i = 0; i < 32; i++)
      a_is_available[i] = b_is_available[i] = true;

#ifdef HALF_B_REGFILE
   for (i = 0; i < 8; i++)
      b_is_available[i] = b_is_available[i + 16] = false;
#endif

   memset(acc_node, ~0, sizeof(acc_node));
   memset(a_last_used, 0, sizeof(a_last_used));
   memset(b_last_used, 0, sizeof(b_last_used));
   memset(acc_last_used, 0, sizeof(acc_last_used));
   flags_available = 0;
   flags_node = ~0;

   if (type & GLSL_BACKEND_TYPE_FRAGMENT)
   {
      a_is_available[15] = false;
      b_is_available[15] = false;

      a_scheduler_node[15] = NULL;
      b_scheduler_node[15] = NULL;
   }
   else if (type & GLSL_BACKEND_TYPE_OFFLINE_VERTEX)
   {
      a_is_available[31] = false;  /* used for read pointer */
      b_is_available[31] = false;  /* used for write pointer */

      a_scheduler_node[31] = NULL;
      b_scheduler_node[31] = NULL;
   }

   vcos_assert(!is_threaded || type & GLSL_BACKEND_TYPE_FRAGMENT);
   shader_type = type;
   shader_is_threaded = is_threaded;

#ifndef WORKAROUND_HW1632
   last_thrsw = ~0;
#endif

   prefer_b = false;

   finished = false;
   failed = false;

   scheduler_order = 0;
   scheduler_attempts = 0;
   scheduler_have_node = false;

   fb_rb_swap = fb_rb_swap_in;
}

static void get_sampler_details(Dataflow *sampler, uint32_t *dest_detail0, uint32_t *dest_detail1)
{
   switch (sampler->flavour)
   {
   case DATAFLOW_CONST_SAMPLER:
      *dest_detail0 = 0;
      *dest_detail1 = sampler->u.const_sampler.location & ~0x80000000;
      break;
#ifdef __BCM2708A0__
   case DATAFLOW_INDEXED_UNIFORM_SAMPLER:
      *dest_detail0 = BACKEND_UNIFORM_TEX_UNIF;
      *dest_detail1 =
         (sampler->u.indexed_uniform_sampler.size & ~0x80000000) << 16 |
         sampler->u.indexed_uniform_sampler.uniform->u.linkable_value.row;
      break;
#endif
   default:
      UNREACHABLE();
   }
}

static uint32_t bool_rep_to_cond(uint32_t bool_rep, bool flip)
{
   switch (bool_rep)
   {
   case BOOL_REP_ZERO_N:
   case BOOL_REP_BOOL:
      return flip ? 2 : 3;            // Z clear
   case BOOL_REP_ZERO:
   case BOOL_REP_BOOL_N:
      return flip ? 3 : 2;            // Z set
   case BOOL_REP_NEG:
      return flip ? 5 : 4;            // N set
   case BOOL_REP_NEG_N:
      return flip ? 4 : 5;            // N clear
   default:
      UNREACHABLE();
      return 0;
   }
}

static uint32_t get_textunit(Dataflow *sampler)
{
   if (sampler == NULL)
      return 0;     /* TODO: indexed lookups should be spread over both units */
   else if (sampler->flavour == DATAFLOW_CONST_SAMPLER)
      return (sampler->u.const_sampler.location & 0x80000000) ? 1 : 0;
   else
   {
#ifdef __BCM2708A0__
      vcos_assert(sampler->flavour == DATAFLOW_INDEXED_UNIFORM_SAMPLER);
#else
      UNREACHABLE();
#endif
      return (sampler->u.indexed_uniform_sampler.size & 0x80000000) ? 1 : 0;
   }
}

static void add_iodependencies(Dataflow *dataflow, OP_SRC_T *deps, uint32_t *dep_count)
{
   DataflowChainNode *node;
   for (node = dataflow->iodependencies.first; node; node = node->next)
   {
      vcos_assert(*dep_count < MAX_DEPS);
      deps[*dep_count] = find_src(node->dataflow, false, false).src;
      (*dep_count)++;
   }
   switch (dataflow->flavour)
   {
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
         add_iodependencies(dataflow->u.unary_op.operand, deps, dep_count);
         break;
      default:
         break;
   }
}

void glsl_allocator_add_node(Dataflow *dataflow)
{
   DataflowChainNode *node;
   DataflowFlavour flavour = dataflow->flavour;
   uint32_t op = 0;
   bool uop = false;
   bool binop = false;
   bool reverse = false;
   bool output = false;
   bool sig = false;
   bool pack = false;
   bool cond_flip = false;
   OP_SRC_T deps[MAX_DEPS];
   uint32_t dep_count;
   OP_T result;
   Dataflow *left = NULL, *right = NULL, *source = NULL;
   Dataflow *cond_node = NULL;
   INSTR_SIG_T sig_code = 0;
   uint32_t dest_detail0 = ~0;
   uint32_t dest_detail1 = ~0;

   if (failed) return;

   scheduler_current_node = dataflow;

   dep_count = 0;
   for (node = dataflow->iodependencies.first; node; node = node->next)
   {
      vcos_assert(dep_count < MAX_DEPS);
      deps[dep_count] = find_src(node->dataflow, false, false).src;
      dep_count++;
   }

   switch (flavour)
   {
      case DATAFLOW_INTRINSIC_SIGN:
         uop = true; op = FLAVOUR_MOV; break;// XXX XXX XXX
      case DATAFLOW_CONST_BOOL:
      case DATAFLOW_CONST_INT:
      case DATAFLOW_CONST_FLOAT:
      case DATAFLOW_CONST_SAMPLER:
      case DATAFLOW_UNIFORM:
      case DATAFLOW_ATTRIBUTE:
      case DATAFLOW_VARYING:
      case DATAFLOW_TEX_GET_CMP_G:
      case DATAFLOW_TEX_GET_CMP_B:
      case DATAFLOW_TEX_GET_CMP_A:
      case DATAFLOW_FRAG_GET_X:
      case DATAFLOW_FRAG_GET_Y:
      case DATAFLOW_FRAG_GET_Z:
      case DATAFLOW_FRAG_GET_W:
      case DATAFLOW_FRAG_GET_PC_X:
      case DATAFLOW_FRAG_GET_PC_Y:
      case DATAFLOW_FRAG_GET_FF:
#ifndef __BCM2708A0__
      case DATAFLOW_UNIFORM_ADDRESS:
#endif
         /*
            This function only makes sense for:
            - Nodes corresponding to arithmetic operators.
            - Output nodes (pixel colour, vertex output)
            - Special output+input nodes (SFU, pack)
            Not to be used on:
            - Input nodes (literals, uniforms, varyings, texture results, attributes)
            - Devices such as thread switch
         */
         UNREACHABLE();
         break;

      // TODO: add things to node_has_float_output as they get added here

      //case DATAFLOW_UNIFORM_OFFSET:
      //case DATAFLOW_ARITH_NEGATE:
      case DATAFLOW_MUL:
         binop = true; op = FLAVOUR_FMUL; break;
      case DATAFLOW_ADD:
         binop = true; op = FLAVOUR_FADD; break;
      case DATAFLOW_SUB:
         binop = true; op = FLAVOUR_FSUB; break;
      case DATAFLOW_RSUB:
         binop = true; op = FLAVOUR_FSUB; reverse = true; break;

      //case DATAFLOW_LOGICAL_NOT:

      case DATAFLOW_LESS_THAN:
         binop = true; op = FLAVOUR_FSUB; break;
      case DATAFLOW_LESS_THAN_EQUAL:
         binop = true; op = FLAVOUR_FSUB; reverse = true; break;
      case DATAFLOW_GREATER_THAN:
         binop = true; op = FLAVOUR_FSUB; reverse = true; break;
      case DATAFLOW_GREATER_THAN_EQUAL:
         binop = true; op = FLAVOUR_FSUB; break;
      case DATAFLOW_EQUAL:
         binop = true; op = FLAVOUR_XOR; break;
      case DATAFLOW_NOT_EQUAL:
         binop = true; op = FLAVOUR_XOR; break;

      case DATAFLOW_LOGICAL_AND:
         binop = true; op = FLAVOUR_AND; break;
      case DATAFLOW_LOGICAL_XOR:
         binop = true; op = FLAVOUR_XOR; break;
      case DATAFLOW_LOGICAL_OR:
         binop = true; op = FLAVOUR_OR; break;
      case DATAFLOW_LOGICAL_SHR:
         binop = true; op = FLAVOUR_V8SUBS; break;
      case DATAFLOW_SHIFT_RIGHT:
         binop = true; op = FLAVOUR_SHR; break;

      case DATAFLOW_CONDITIONAL:
      {
         OP_SRC_DETAIL_T src_detail = find_src(dataflow->u.cond_op.true_value, true, true);
         OP_SRC_DETAIL_T background = find_src(dataflow->u.cond_op.false_value, true, true);
         OP_SRC_DETAIL_T flags_src = find_src(dataflow->u.cond_op.cond, true, true);
         OP_COND_T cond = bool_rep_to_cond(glsl_dataflow_get_bool_rep(dataflow->u.cond_op.cond), false);

         result = op_join_output(src_detail, background, dest_pack(PACK_NONE), deps, dep_count, cond, flags_src, true);
         vcos_assert(dataflow->slot == ~0);
         dataflow->slot = result;
         return;
      }

      case DATAFLOW_INTRINSIC_FLOATTOINT_TRUNC:
      case DATAFLOW_INTRINSIC_FLOATTOINT_NEAREST:     //TODO: these can't both be right
      case DATAFLOW_COERCE_TOINT:                     //TODO: sort out itof/ftoi mess
         uop = true; op = FLAVOUR_FTOI; break;
      //case DATAFLOW_INTRINSIC_FLOATTOBOOL:
      case DATAFLOW_COERCE_TOFLOAT:
         uop = true; op = FLAVOUR_ITOF; break;
      //case DATAFLOW_INTRINSIC_INTTOBOOL:
      //case DATAFLOW_INTRINSIC_BOOLTOINT:
      //case DATAFLOW_INTRINSIC_BOOLTOFLOAT:
      case DATAFLOW_INTRINSIC_RSQRT:
         output = true; op = DEST_EITHER(53); source = dataflow->u.unary_op.operand; break;
      case DATAFLOW_INTRINSIC_RCP:
         output = true; op = DEST_EITHER(52); source = dataflow->u.unary_op.operand; break;
      case DATAFLOW_INTRINSIC_LOG2:
         output = true; op = DEST_EITHER(55); source = dataflow->u.unary_op.operand; break;
      case DATAFLOW_INTRINSIC_EXP2:
         output = true; op = DEST_EITHER(54); source = dataflow->u.unary_op.operand; break;
      //case DATAFLOW_INTRINSIC_CEIL:
      //case DATAFLOW_INTRINSIC_FLOOR:
      //case DATAFLOW_INTRINSIC_SIGN:
      case DATAFLOW_INTRINSIC_MIN:
         binop = true; op = FLAVOUR_FMIN; break;
      case DATAFLOW_INTRINSIC_MAX:
         binop = true; op = FLAVOUR_FMAX; break;
      case DATAFLOW_INTRINSIC_INTTOFLOAT:
      case DATAFLOW_MOV:
      case DATAFLOW_LOGICAL_NOT:      /* TODO: we shouldn't need this one */
         uop = true; op = FLAVOUR_MOV; break;
      //case DATAFLOW_COERCE_TOFLOAT:
      //case DATAFLOW_COERCE_TOINT:
      case DATAFLOW_THREADSWITCH:
         sig = true; sig_code = SIG_THRSW; break;
      case DATAFLOW_TEX_SET_COORD_S:
         output = true; source = dataflow->u.texture_lookup_set.param;
         op = DEST_EITHER(56 + 4 * get_textunit(dataflow->u.texture_lookup_set.sampler));
         get_sampler_details(dataflow->u.texture_lookup_set.sampler, &dest_detail0, &dest_detail1);
         break;
      case DATAFLOW_TEX_SET_COORD_T:
         output = true; source = dataflow->u.texture_lookup_set.param;
         op = DEST_EITHER(57 + 4 * get_textunit(dataflow->u.texture_lookup_set.sampler));
         get_sampler_details(dataflow->u.texture_lookup_set.sampler, &dest_detail0, &dest_detail1);
         break;
      case DATAFLOW_TEX_SET_COORD_R:
         output = true; source = dataflow->u.texture_lookup_set.param;
         op = DEST_EITHER(58 + 4 * get_textunit(dataflow->u.texture_lookup_set.sampler));
         get_sampler_details(dataflow->u.texture_lookup_set.sampler, &dest_detail0, &dest_detail1);
         break;
      case DATAFLOW_TEX_SET_BIAS:
         output = true; source = dataflow->u.texture_lookup_set.param;
         op = DEST_EITHER(59 + 4 * get_textunit(dataflow->u.texture_lookup_set.sampler));
         get_sampler_details(dataflow->u.texture_lookup_set.sampler, &dest_detail0, &dest_detail1);
         break;
      //case DATAFLOW_TEX_SET_LOD:
#ifndef __BCM2708A0__
      case DATAFLOW_TEX_SET_DIRECT:
         output = true; source = dataflow->u.texture_lookup_set.param;
         op = DEST_A(56 + 4 * get_textunit(dataflow->u.texture_lookup_set.sampler));    /* TODO: not really DEST_A */
         break;
#endif
      case DATAFLOW_TEX_GET_CMP_R:
         sig = true;
         sig_code = get_textunit(dataflow->u.texture_lookup_get.sampler) ? SIG_TMU1 : SIG_TMU0;
         break;
      case DATAFLOW_FRAG_GET_COL:
         sig = true; sig_code = SIG_GET_COL; break;
      case DATAFLOW_FRAG_SUBMIT_STENCIL:
         output = true; op = DEST_EITHER(43); source = dataflow->u.fragment_set.param;
         vcos_assert(!dataflow->u.fragment_set.discard);
         break;
      case DATAFLOW_FRAG_SUBMIT_Z:
         output = true; op = DEST_EITHER(44); source = dataflow->u.fragment_set.param;
         cond_node = dataflow->u.fragment_set.discard;
         cond_flip = true;
         break;
      case DATAFLOW_FRAG_SUBMIT_MS:
         output = true; op = DEST_EITHER(45); source = dataflow->u.fragment_set.param;
         cond_node = dataflow->u.fragment_set.discard;
         cond_flip = true;
         break;
      case DATAFLOW_FRAG_SUBMIT_ALL:
         output = true; op = DEST_EITHER(46); source = dataflow->u.fragment_set.param;
         cond_node = dataflow->u.fragment_set.discard;
         cond_flip = true;
         break;
      case DATAFLOW_FRAG_SUBMIT_R0:
         output = true; op = DEST_ACC(0); source = dataflow->u.fragment_set.param; cond_node = NULL;
         break;
      case DATAFLOW_FRAG_SUBMIT_R1:
         output = true; op = DEST_ACC(1); source = dataflow->u.fragment_set.param; cond_node = NULL;
         break;
      case DATAFLOW_FRAG_SUBMIT_R2:
         output = true; op = DEST_ACC(2); source = dataflow->u.fragment_set.param; cond_node = NULL;
         break;
      case DATAFLOW_FRAG_SUBMIT_R3:
         output = true; op = DEST_ACC(3); source = dataflow->u.fragment_set.param; cond_node = NULL;
         break;
      case DATAFLOW_TMU_SWAP:
         output = true; op = DEST_EITHER(36); source = dataflow->u.fragment_set.param;
         vcos_assert(!dataflow->u.fragment_set.discard);
         break;
      case DATAFLOW_VERTEX_SET:
         output = true; op = DEST_EITHER(48); source = dataflow->u.vertex_set.param; break;
      case DATAFLOW_VPM_READ_SETUP:
         output = true; op = DEST_A(49); source = dataflow->u.vertex_set.param; break;
      case DATAFLOW_VPM_WRITE_SETUP:
         output = true; op = DEST_B(49); source = dataflow->u.vertex_set.param; break;
      case DATAFLOW_PACK_COL_R:
         pack = true; op = dest_pack(PACK_8A_C); break;
      case DATAFLOW_PACK_COL_G:
         pack = true; op = dest_pack(PACK_8B_C); break;
      case DATAFLOW_PACK_COL_B:
         pack = true; op = dest_pack(PACK_8C_C); break;
      case DATAFLOW_PACK_COL_A:
         pack = true; op = dest_pack(PACK_8D_C); break;
      case DATAFLOW_PACK_16A:
         pack = true; op = dest_pack(PACK_16A); break;
      case DATAFLOW_PACK_16B:
         pack = true; op = dest_pack(PACK_16B); break;
      case DATAFLOW_PACK_FB_R:
         pack = true; op = dest_pack(fb_rb_swap ? PACK_8C_C : PACK_8A_C); break;
      case DATAFLOW_PACK_FB_B:
         pack = true; op = dest_pack(fb_rb_swap ? PACK_8A_C : PACK_8C_C); break;
      case DATAFLOW_SCOREBOARD_WAIT:
         sig = true; sig_code = SIG_SB_WAIT;
         break;
      case DATAFLOW_BITWISE_NOT:
         uop = true; op = FLAVOUR_NOT; break;
      case DATAFLOW_BITWISE_AND:
         binop = true; op = FLAVOUR_AND; break;
      case DATAFLOW_BITWISE_OR:
         binop = true; op = FLAVOUR_OR; break;
      case DATAFLOW_BITWISE_XOR:
         binop = true; op = FLAVOUR_XOR; break;
      case DATAFLOW_V8MULD:
         binop = true; op = FLAVOUR_V8MULD; break;
      case DATAFLOW_V8MIN:
         binop = true; op = FLAVOUR_V8MIN; break;
      case DATAFLOW_V8MAX:
         binop = true; op = FLAVOUR_V8MAX; break;
      case DATAFLOW_V8ADDS:
         binop = true; op = FLAVOUR_V8ADDS; break;
      case DATAFLOW_V8SUBS:
         binop = true; op = FLAVOUR_V8SUBS; break;
      case DATAFLOW_INTEGER_ADD:
         binop = true; op = FLAVOUR_ADD; break;
      default:
         //TODO: other sorts of node
          UNREACHABLE();
   }

   if (pack)
   {
      OP_SRC_DETAIL_T src_detail = find_src(dataflow->u.pack.operand, true, true);
      if (dataflow->u.pack.background)
      {
         OP_SRC_DETAIL_T background = find_src(dataflow->u.pack.background, true, true);
         result = op_join_output(src_detail, background, op, deps, dep_count, COND_ALWAYS, src_detail_none(), true);
      }
      else
      {
         result = op_add_output(src_detail, op, ~0, ~0, deps, dep_count, COND_ALWAYS, src_detail_none(), true);
      }

      vcos_assert(dataflow->slot == ~0);
      dataflow->slot = result;

      return;
   }

   if (output)
   {
      OP_SRC_DETAIL_T flags_src;
      OP_COND_T cond;

      if (cond_node)
      {
         flags_src = find_src(cond_node, true, true);
         cond = bool_rep_to_cond(glsl_dataflow_get_bool_rep(cond_node), cond_flip);
      }
      else
      {
         flags_src = src_detail_none();
         cond = COND_ALWAYS;
      }

      /* Need to add dependencies of inputs too. */
      add_iodependencies(source, deps, &dep_count);

      result = op_add_output(find_src(source, true, true), op, dest_detail0, dest_detail1, deps, dep_count, cond, flags_src, true);

      vcos_assert(dataflow->slot == ~0);
      dataflow->slot = result;

      return;
   }

   if (sig)
   {
      result = instr_add_sig(sig_code, deps, dep_count);

      vcos_assert(dataflow->slot == ~0);
      dataflow->slot = op_from_timestamp(result, 0);    /* TODO: bad */

      return;
   }

   if (uop)
   {
      left = right = dataflow->u.unary_op.operand;
   }
   if (binop && !reverse)
   {
      left = dataflow->u.binary_op.left;
      right = dataflow->u.binary_op.right;
   }
   if (binop && reverse)
   {
      left = dataflow->u.binary_op.right;
      right = dataflow->u.binary_op.left;
   }

   /* Need to add dependencies of inputs too. */
   add_iodependencies(left, deps, &dep_count);
   add_iodependencies(right, deps, &dep_count);

   result = op_add_alu(op, find_src(left, true, true), find_src(right, true, true), deps, dep_count, true, 0, 0);

   vcos_assert(dataflow->slot == ~0);
   dataflow->slot = result;

   return;
}

/*
   I don't particularly like this. Issue a mov from an input node if somebody
   else depends on that input node (and it hasn't already been scheduled).
*/
bool glsl_allocator_add_input_dependency(Dataflow *dataflow)
{
   OP_SRC_T deps[MAX_DEPS];
   uint32_t dep_count;
   OP_SRC_DETAIL_T src_detail, raw;
   DataflowChainNode *node;

   if (failed) return true;

   dep_count = 0;
   for (node = dataflow->iodependencies.first; node; node = node->next)
   {
      vcos_assert(dep_count < MAX_DEPS);
      deps[dep_count] = find_src(node->dataflow, false, false).src;
      dep_count++;
   }

   src_detail = find_src(dataflow, false, true);
   raw = src_detail_without_unpack(src_detail, true);
   op_add_alu(FLAVOUR_MOV, raw, raw, deps, dep_count, false, 0, 0);
   return true;
}

bool glsl_allocator_finish()
{
   if (failed)
   {
      finished = true;
      return false;
   }

   if (shader_type & GLSL_BACKEND_TYPE_VERTEX_OR_COORD)
   {
      last_used_timestamp++;
      get_instr(last_used_timestamp)->sig = SIG_THREND;
      last_used_timestamp += 2;
   }

   generated_instruction_count = generate(generated_instructions);
   vcos_assert(generated_instruction_count <= MAX_INSTRUCTIONS);
   vcos_assert(!finished);
   finished = true;

   /*if (shader_type & GLSL_BACKEND_TYPE_FRAGMENT) glsl_allocator_dump(); */
   /*if (shader_type & GLSL_BACKEND_TYPE_VERTEX_OR_COORD) glsl_allocator_dump(); */
   /*if (shader_type & GLSL_BACKEND_TYPE_OFFLINE_VERTEX) glsl_qdisasm_py_dump(generated_instruction_count, generated_instructions); */
   /*if (shader_type & GLSL_BACKEND_TYPE_VERTEX_OR_COORD)glsl_qdisasm_py_dump(generated_instruction_count,generated_instructions); */
   /*if (shader_type & GLSL_BACKEND_TYPE_FRAGMENT)glsl_qdisasm_py_dump(generated_instruction_count,generated_instructions); */
   return true;
}

void glsl_allocator_dump()
{
   vcos_assert(finished);

   /*DEBUG*/
   {
      uint32_t i;
      uint32_t *uniform = uniform_map;
      uint32_t state = 0;
      for (i = 0; i < generated_instruction_count; i++)
      {
         char buffer[256], buffer2[256];
         INSTR_STRUCT_T *instr;
         uniform = glsl_qdisasm_with_uniform(&state, buffer, sizeof(buffer), generated_instructions[2*i], generated_instructions[2*i+1], uniform, true);
         instr = get_instr(i);
         sprintf(buffer2, "[%2d,%2d] %s\n", op_struct[i][0].schedule_order, op_struct[i][1].schedule_order, buffer);
         debug_print(buffer2);
      }
      debug_print("\n");
   }
//   glsl_qdisasm_dump(generated_instructions, uniform_map, generated_instruction_count, true);
   /*DEBUG*/

}

uint32_t *glsl_allocator_get_shader_pointer()
{
   vcos_assert(finished);
   return generated_instructions;
}

uint32_t glsl_allocator_get_shader_size()
{
   vcos_assert(finished);
   return 8 * generated_instruction_count;
}

uint32_t *glsl_allocator_get_unif_pointer()
{
   vcos_assert(finished);
   return uniform_map;
}

uint32_t glsl_allocator_get_unif_size()
{
   vcos_assert(finished);
   return 8 * uniform_count;
}

uint32_t *glsl_allocator_get_varying_pointer()
{
   vcos_assert(finished);
   return varying_map;
}

uint32_t glsl_allocator_get_varying_count()
{
   vcos_assert(finished);
   return varying_count;
}

bool glsl_allocator_failed(void)
{
   return failed;
}

/*
   Get one of the dependents on something which has been left on our stack. Hopefully this will
   help clear the stack down.
*/
Dataflow *glsl_allocator_get_next_scheduler_node(void)
{
   int best_order = 0x7ffffff; //INT_MAX;
   int i;
   Dataflow *best_node = NULL;
   DataflowChainNode *ch;
   bool found;
   int a_count = 0, b_count = 0;

   /* Don't allow recursive sidetracking */
   if (scheduler_have_node) return NULL;

   for (i = 0; i < 32; i++)
   {
      if (a_is_available[i]) a_count++;
      if (b_is_available[i]) b_count++;
   }

   /* Don't bother unless it's really tight. */
   if (a_count > 5 && b_count > 5 ) return NULL;

   /* Do this stuff more often as it gets tigher. */
   /*scheduler_attempts++;
   if (scheduler_attempts < _min(a_count, b_count)/2) return NULL;
   scheduler_attempts = 0;*/

   for (i = 0; i < 32; i++)
   {
      if (!a_is_available[i] && a_scheduler_node[i] && a_scheduler_order[i] < best_order)
      {
         found = false;
         for (ch = a_scheduler_node[i]->dependents.first; ch; ch = ch->next)
         {
            /*
            Not BACKEND_PASS_DIVING. We're going to schedule those at some point anyway, and doing them in the
            wrong order will confuse the scheduler.
            */
            if (ch->dataflow->phase == BACKEND_PASS_AWAITING_SCHEDULE)
            {
               best_order = i;
               best_node = ch->dataflow;
               found = true;
               break;
            }
         }
         if (!found)
         {
            /*
            No dependents left to schedule. Remove this from the scheduler's interest.
            (Why is it still on the stack though?)
            */
            //vcos_assert(0);
            a_scheduler_node[i] = NULL;
         }
      }
      if (!b_is_available[i] && b_scheduler_node[i] && b_scheduler_order[i] < best_order)
      {
         found = false;
         for (ch = b_scheduler_node[i]->dependents.first; ch; ch = ch->next)
         {
            if (ch->dataflow->phase == BACKEND_PASS_AWAITING_SCHEDULE)
            {
               best_order = i;
               best_node = ch->dataflow;
               found = true;
               break;
            }
         }
         if (!found)
         {
            //vcos_assert(0);
            b_scheduler_node[i] = NULL;
         }
      }
   }

   scheduler_have_node = best_node != NULL;
   return best_node;
}

void glsl_allocator_finish_scheduler_node(Dataflow *suggestion)
{
   vcos_assert(scheduler_have_node);
   scheduler_have_node = false;
}
