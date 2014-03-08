/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/glsl/2708/glsl_qdisasm_4.h"
#include "middleware/khronos/glsl/glsl_common.h"
#include "middleware/khronos/glsl/2708/glsl_allocator_4.h"
#include <stdio.h>

#ifndef SIMPENROSE
#define _snprintf snprintf
#endif

#ifdef ANDROID
#ifndef _snprintf
#define _snprintf snprintf
#endif
#endif

static const char *add_code[32] =
{
   "xnop",
   "fadd",
   "fsub",
   "fmin",
   "fmax",
   "fminabs",
   "fmaxabs",
   "xftoi",
   "xitof",
   "xxx9",
   "xxx10",
   "xxx11",
   "add",
   "sub",
   "shr",
   "asr",
   "ror",
   "shl",
   "min",
   "max",
   "and",
   "or",
   "xor",
   "xnot",
   "xclz",
   "xxx25",
   "xxx26",
   "xxx27",
   "xxx28",
   "xxx29",
   "v8adds",
   "v8subs",
};

static uint32_t mini_add_count[32] =
{
   0,
   3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
   2,
   3, 2, 2, 3, 3, 3, 3, 3, 3, 3,
};

static const char *mini_add_code[32] =
{
   "nop",
   0, 0, 0, 0, 0, 0, "ftoi", "itof", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   "mov",
   0, "not", "clz", 0, 0, 0, 0, 0, 0, 0,
};

static const char *mul_code[8] =
{
   "xnop",
   "fmul",
   "mul24",
   "v8muld",
   "v8min",
   "v8max",
   "v8adds",
   "v8subs",
};

static uint32_t mini_mul_count[8] =
{
   0, 3, 3, 3, 2, 3, 3, 3,
};

static const char *mini_mul_code[8] =
{
   "nop", 0, 0, 0, "mov", 0, 0, 0,
};

static const char *sig_code[16] =
{
   "bkpt",
   "",
   "thrsw",
   "thrend",
   "sbwait",
   "sbdone",
#ifdef __BCM2708A0__
   "int",
#else
   "lthrsw",
#endif
   "loadcv",
   "loadc",
   "ldcend",
   "ldtmu0",
   "ldtmu1",
#ifdef __BCM2708A0__
   "r5rot",
#else
   "loadam",
#endif
   "",          /* small immediate */
   "ldi",
   "bra",
};

static const char *ra_code[64] =
{
    "ra0",  "ra1",  "ra2",  "ra3",  "ra4",  "ra5",  "ra6",  "ra7",
    "ra8",  "ra9", "ra10", "ra11", "ra12", "ra13", "ra14",    "w",
   "ra16", "ra17", "ra18", "ra19", "ra20", "ra21", "ra22", "ra23",
   "ra24", "ra25", "ra26", "ra27", "ra28", "ra29", "ra30", "ra31",
   "unif", "xa33", "xa34", "vary", "xa36", "xa37", "elem_num", "nop",
   "xa40", "x_coord", "ms_mask", "xa43", "xa44", "xa45", "xa46", "xa47",
   "vpm", "vr_busy", "vr_wait", "mutex", "xa52", "xa53", "xa54", "xa55",
   "xa56", "xa57", "xa58", "xa59", "xa60", "xa61", "xa62", "xa63",
};

static const char *rb_code[64] =
{
    "rb0",  "rb1",  "rb2",  "rb3",  "rb4",  "rb5",  "rb6",  "rb7",
    "rb8",  "rb9", "rb10", "rb11", "rb12", "rb13", "rb14",    "z",
   "rb16", "rb17", "rb18", "rb19", "rb20", "rb21", "rb22", "rb23",
   "rb24", "rb25", "rb26", "rb27", "rb28", "rb29", "rb30", "rb31",
   "unif", "xb33", "xb34", "vary", "xb36", "xb37", "qpu_num", "nop",
   "xb40", "y_coord", "rev_flag", "xb43", "xb44", "xb45", "xb46", "xb47",
   "vpm", "vw_busy", "vw_wait", "mutex", "xb52", "xb53", "xb54", "xb55",
   "xb56", "xb57", "xb58", "xb59", "xb60", "xb61", "xb62", "xb63",
};

static const char *wa_code[64] =
{
    "ra0",  "ra1",  "ra2",  "ra3",  "ra4",  "ra5",  "ra6",  "ra7",
    "ra8",  "ra9", "ra10", "ra11", "ra12", "ra13", "ra14",    "w",
   "ra16", "ra17", "ra18", "ra19", "ra20", "ra21", "ra22", "ra23",
   "ra24", "ra25", "ra26", "ra27", "ra28", "ra29", "ra30", "ra31",
#ifdef __BCM2708A0__
   "r0", "r1", "r2", "r3", "xa36", "r5quad", "hdr_mode", "-",
#else
   "r0", "r1", "r2", "r3", "tmurs", "r5quad", "xa38", "-",
#endif
   "unif_addr", "x_coord", "ms_mask", "stencil", "tlbz", "tlbm", "tlbc", "xa47",
   "vpm", "vr_setup", "vr_addr", "mutex", "recip", "recipsqrt", "exp", "log",
   "t0s", "t0t", "t0r", "t0b", "t1s", "t1t", "t1r", "t1b",
};

static const char *wb_code[64] =
{
    "rb0",  "rb1",  "rb2",  "rb3",  "rb4",  "rb5",  "rb6",  "rb7",
    "rb8",  "rb9", "rb10", "rb11", "rb12", "rb13", "rb14",    "w",
   "rb16", "rb17", "rb18", "rb19", "rb20", "rb21", "rb22", "rb23",
   "rb24", "rb25", "rb26", "rb27", "rb28", "rb29", "rb30", "rb31",
#ifdef __BCM2708A0__
   "r0", "r1", "r2", "r3", "xb36", "r5rep", "hdr_mode", "-",
#else
   "r0", "r1", "r2", "r3", "tmurs", "r5rep", "xa38", "-",
#endif
   "unif_addr", "y_coord", "rev_flag", "stencil", "tlbz", "tlbm", "tlbc", "xb47",
   "vpm", "vw_setup", "vw_addr", "mutex", "recip", "recipsqrt", "exp", "log",
   "t0s", "t0t", "t0r", "t0b", "t1s", "t1t", "t1r", "t1b",
};

static const char *sf_code[2] = {"", ".setf"};
static const char *cond_code[8] = {".never", "", ".zs", ".zc", ".ns", ".nc", ".cs", ".cc"};
static const char *cond_br_code[16] = {".allz",".allnz",".anyz",".anynz",".alln",".allnn",".anyn",".anynn",".allc",".allnc",".anyc",".anync","xxx12","xxx13","xxx14",""};

static const char *unpack_pm0_code[8] = {"", ".16a", ".16b", ".8dr", ".8a", ".8b", ".8c", ".8d"};
static const char *pack_pm0_code[16] = {"", ".16a", ".16b", ".8abcd", ".8a", ".8b", ".8c", ".8d",
   ".s", ".16as", ".16bs", ".8abcds", ".8as", ".8bs", ".8cs", ".8ds"};
   
static const char *unpack_pm1_code[8] = {"", ".16a", ".16b", ".8dr", ".8a", ".8b", ".8c", ".8d"};
static const char *pack_pm1_code[16] = {"", "Reserved", "Reserved", ".8abcd", ".8a", ".8b", ".8c", ".8d",
   "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"};   

static const char *small_immed[64] = {
   "0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15",
   "-16","-15","-14","-13","-12","-11","-10","-9","-8","-7","-6","-5","-4","-3","-2","-1",
   "1.0","2.0","4.0","8.0","16.0","32.0","64.0","128.0",
   "1/256","1/128","1/64","1/32","1/16","1/8","1/4","1/2",
   " >> r5"," >> 1"," >> 2"," >> 3"," >> 4"," >> 5"," >> 6"," >> 7"," >> 8"," >> 9"," >> 10"," >> 11"," >> 12"," >> 13"," >> 14"," >> 15"};

void glsl_formatplain_instruction(char *buffer, uint32_t buffer_length, uint32_t word0, uint32_t word1)
{
   _snprintf(buffer,buffer_length,"0x%08x, 0x%08x",word0,word1);
}

void glsl_qdisasm_instruction(char *buffer, uint32_t buffer_length, uint32_t word0, uint32_t word1)
{
   const char *inp_code[8] = {
      "r0","r1","r2","r3","r4","r5",
      0,      //raddr_a
      0,      //raddr_b
   };
   const char **wadd_code = (word1 & 1<<12) ? wb_code : wa_code;      //ws
   const char **wmul_code = (word1 & 1<<12) ? wa_code : wb_code;      //ws
   char shift_buffer[40];
   char add_buffer[40];
   char mul_buffer[40];
   //char junk_buffer[20] = "";
   char add_pack_buffer[8] = "";
   char add_unpack_buffer[8] = "";
   char mul_pack_buffer[8] = "";
   char mul_unpack_buffer[8] = "";
   uint32_t junk;
   uint32_t op_add, op_mul;
   uint32_t add_a, add_b, mul_a, mul_b;
   uint32_t waddr_add, waddr_mul;
   uint32_t cond_add, cond_mul, sf_add, sf_mul;
   uint32_t sig;
   uint32_t unpack, pack, pm;
   uint32_t raddr_a;
   uint32_t raddr_b;

   sig = word1 >> 28 & 15;
   if (sig == 15)
      raddr_a = word1 >> 13 & 63;
   else
      raddr_a = word0 >> 18 & 63;

   raddr_b = word0 >> 12 & 63;

   if (sig == 13 && raddr_b < 48)
   {
      inp_code[6] = ra_code[raddr_a];
      inp_code[7] = small_immed[raddr_b];
   }
   else if (sig == 13)
   {
      _snprintf(shift_buffer, 40, "%s%s%s", ra_code[raddr_a], small_immed[raddr_b], raddr_a < 32 ? "" : "xxx");
      inp_code[6] = shift_buffer;
      inp_code[7] = "xxx";
   }
   else
   {
      inp_code[6] = ra_code[raddr_a];
      inp_code[7] = rb_code[raddr_b];
   }

   junk = word1 & 0x0ff00000;         //TODO: unpack, pm, pack
   if (junk)
   {
      unpack = junk >> 25 & 7;
      pm = junk >> 24 & 1;
      pack = junk >> 20 & 15;
      //_snprintf(junk_buffer, 20, "; JUNK=0x%08x", junk);
      if(pm)
      {
         if(unpack && !pack)
            _snprintf(mul_unpack_buffer, 8, "%s", unpack_pm1_code[unpack]);
         else if(!unpack && pack)
            _snprintf(mul_pack_buffer, 8, "%s", pack_pm1_code[pack]);
         else
         {
            _snprintf(mul_unpack_buffer, 8, "%s", unpack_pm1_code[unpack]);//r4
            _snprintf(mul_pack_buffer, 8, "%s", pack_pm1_code[pack]);
         }
      }else
      {
         //a reg file
         if(unpack && !pack)
            _snprintf(add_unpack_buffer, 8, "%s", unpack_pm0_code[unpack]);
         else if(!unpack && pack)
            _snprintf(add_pack_buffer, 8, "%s", pack_pm0_code[pack]);
         else
         {
            _snprintf(add_unpack_buffer, 8, "%s", unpack_pm0_code[unpack]);
            _snprintf(add_pack_buffer, 8, "%s", pack_pm0_code[pack]);
         }
      }
   }

   op_add = word0 >> 24 & 31;
   op_mul = word0 >> 29 & 7;
   add_a = word0 >> 9 & 7;
   add_b = word0 >> 6 & 7;
   mul_a = word0 >> 3 & 7;
   mul_b = word0 >> 0 & 7;
   waddr_add = word1 >> 6 & 63;
   waddr_mul = word1 >> 0 & 63;

   cond_add = word1 >> 17 & 7;
   cond_mul = word1 >> 14 & 7;
   if (op_add == 0 || cond_add == 0)
   {
      sf_add = 0;
      sf_mul = word1 >> 13 & 1;
   }
   else
   {
      sf_add = word1 >> 13 & 1;
      sf_mul = 0;
   }

   if (sig == 14)
   {
      if (waddr_mul == 39 && cond_mul == 0)
      {
         _snprintf(buffer, buffer_length, "ldi%s%s %s,0x%08x",
            cond_code[cond_add],
            sf_code[sf_add],
            wadd_code[waddr_add],
            word0);
      }
      else
      {
         _snprintf(buffer, buffer_length, "ldi%s%s %s,0x%08x; ldi%s%s %s,0x%08x",
            cond_code[cond_add],
            sf_code[sf_add],
            wadd_code[waddr_add],
            word0,
            cond_code[cond_mul],
            sf_code[sf_mul],
            wadd_code[waddr_mul],
            word0);
      }
   }
   else if (sig == 15)
   {
      uint32_t rel = word1 >> 19 & 1;
      uint32_t reg = word1 >> 18 & 1;
      uint32_t cond_br = word1 >> 20 & 15;
      _snprintf(buffer, buffer_length, "%s%s %s;%s, %s%+d",
         rel ? "brr" : "bra",
         cond_br_code[cond_br],
         wadd_code[waddr_add],
         wmul_code[waddr_mul],
         reg ? ra_code[raddr_a] : "",
         word0);
   }
   else
   {
      if (mini_add_count[op_add] == 0 && waddr_add == 39 && add_a == 0 && add_b == 0 && cond_add == 0)
         _snprintf(add_buffer, 40, "%s", mini_add_code[op_add]);
      else if (mini_add_count[op_add] == 2 && add_a == add_b)
         _snprintf(add_buffer, 40, "%s%s%s  %s%s, %s%s", mini_add_code[op_add], cond_code[cond_add], sf_code[sf_add], wadd_code[waddr_add], add_pack_buffer, inp_code[add_a], add_unpack_buffer);
      else
         _snprintf(add_buffer, 40, "%s%s%s  %s%s, %s, %s %s", add_code[op_add], cond_code[cond_add], sf_code[sf_add], wadd_code[waddr_add], add_pack_buffer, inp_code[add_a], inp_code[add_b], add_unpack_buffer);

      if (mini_mul_count[op_mul] == 0 && waddr_mul == 39 && mul_a == 0 && mul_b == 0 && cond_mul == 0)
         _snprintf(mul_buffer, 40, "%s", mini_mul_code[op_mul]);
      else if (mini_mul_count[op_mul] == 2 && mul_a == mul_b)
         _snprintf(mul_buffer, 40, "%s%s%s  %s%s, %s%s", mini_mul_code[op_mul], cond_code[cond_mul], sf_code[sf_mul], wmul_code[waddr_mul], mul_pack_buffer, inp_code[mul_a], mul_unpack_buffer);
      else
         _snprintf(mul_buffer, 40, "%s%s%s  %s%s, %s, %s %s", mul_code[op_mul], cond_code[cond_mul], sf_code[sf_mul], wmul_code[waddr_mul], mul_pack_buffer, inp_code[mul_a], inp_code[mul_b], mul_unpack_buffer);

      _snprintf(buffer, buffer_length, "%s  ; %s%s %s",
         add_buffer,
         mul_buffer,
         strlen(sig_code[sig])!=0 ? ";   " : "",
         sig_code[sig]);
   }
}

uint32_t *glsl_qdisasm_with_uniform(uint32_t *state, char *buffer, uint32_t buffer_length, uint32_t word0, uint32_t word1, uint32_t *uniform, bool is_uniform_map)
{
   bool has_unif;
   uint32_t raddr_a, raddr_b, waddr_add, waddr_mul, sig;
   char instr_buffer[200];

   sig = word1 >> 28 & 15;
   if (sig == 15)
      raddr_a = word1 >> 13 & 63;
   else
      raddr_a = word0 >> 18 & 63;
   raddr_b = word0 >> 12 & 63;
   waddr_add = word1 >> 6 & 63;
   waddr_mul = word1 >> 0 & 63;

   /* Check for uniform read or texture write */
   has_unif = false;

   if (sig != 14)
      has_unif |= (raddr_a == 32);

   if (sig != 13 && sig != 14 && sig != 15)
      has_unif |= (raddr_b == 32);

   if (sig != 15)
   {
      if (waddr_add == 56 || waddr_mul == 56)
      {
         has_unif |= !!(*state & 1);
         *state &= ~1;
      }
      if (waddr_add >= 57 && waddr_add < 60 || waddr_mul >= 57 && waddr_mul < 60)
      {
         has_unif = true;
         *state |= 1;
      }

      if (waddr_add == 60 || waddr_mul == 60)
      {
         has_unif |= !!(*state & 2);
         *state &= ~2;
      }
      if (waddr_add >= 61 && waddr_add < 64 || waddr_mul >= 61 && waddr_mul < 64)
      {
         has_unif = true;
         *state |= 2;
      }
   }

   if (has_unif)
   {
      glsl_qdisasm_instruction(instr_buffer, sizeof(instr_buffer), word0, word1);
      if (is_uniform_map)
      {
         switch (uniform[0])
         {
         case BACKEND_UNIFORM:
            _snprintf(buffer, buffer_length, "%s    [unif %x]", instr_buffer, uniform[1]);
            break;
         case BACKEND_UNIFORM_LITERAL:
            _snprintf(buffer, buffer_length, "%s    [literal 0x%08x]", instr_buffer, uniform[1]);
            break;
         case BACKEND_UNIFORM_SCALE_X:
            _snprintf(buffer, buffer_length, "%s    [unif scale_x]", instr_buffer);
            break;
         case BACKEND_UNIFORM_SCALE_Y:
            _snprintf(buffer, buffer_length, "%s    [unif scale_y]", instr_buffer);
            break;
         case BACKEND_UNIFORM_SCALE_Z:
            _snprintf(buffer, buffer_length, "%s    [unif scale_z]", instr_buffer);
            break;
         case BACKEND_UNIFORM_OFFSET_Z:
            _snprintf(buffer, buffer_length, "%s    [unif offset_z]", instr_buffer);
            break;
         case BACKEND_UNIFORM_VPM_READ_SETUP:
            _snprintf(buffer, buffer_length, "%s    [unif vpm_read_setup]", instr_buffer);
            break;
         case BACKEND_UNIFORM_VPM_WRITE_SETUP:
            _snprintf(buffer, buffer_length, "%s    [unif vpm_write_setup]", instr_buffer);
            break;
         default:
            _snprintf(buffer, buffer_length, "%s    [??? 0x%08x 0x%08x]", instr_buffer, uniform[0], uniform[1]);
            break;
         }
         uniform += 2;
      }
      else
      {
         _snprintf(buffer, buffer_length, "%s    [unif 0x%08x]", instr_buffer, uniform[0]);
         uniform++;
      }
   }
   else
      glsl_qdisasm_instruction(buffer, buffer_length, word0, word1);

   return uniform;
}

void glsl_qdisasm_dump(const void *shader, const void *uniforms, uint32_t count, bool is_uniform_map)
{
   uint32_t i;
   uint32_t *instr, *unif;
   char buffer[100];
   uint32_t state = 0;

   instr = (uint32_t *)shader;
   unif = (uint32_t *)uniforms;

   for (i = 0; i < count; i++)
   {
      if (uniforms)
         unif = glsl_qdisasm_with_uniform(&state, buffer, sizeof(buffer), instr[2 * i], instr[2 * i + 1], unif, is_uniform_map);
      else
         glsl_qdisasm_instruction(buffer, sizeof(buffer), instr[2 * i], instr[2 * i + 1]);
      debug_print(buffer);
      debug_print("\n");
   }
   debug_print("\n");
}


#ifndef ANDROID
//for msvc win32
#ifdef SIMPENROSE
#define GLSL_SHADER_FD_DQASM "c:\\Python27\\python ..\\..\\..\\..\\..\\v3d\\verification\\tools\\dqasm.py -tb0"
#endif
#define GLSL_SHADER_FD_OUTPUT "shaders_asm\\shader"
//under cygwin -c gives colours
//use the external python disassembler
static int shader_num = 0;
void glsl_qdisasm_py_dump(uint32_t count,uint32_t * words)
{
#ifdef GLSL_SHADER_FD_DQASM
   FILE * f;
   uint32_t i;
   char cmd[256];
   sprintf(cmd,"%s > %s%03d.asm",GLSL_SHADER_FD_DQASM,GLSL_SHADER_FD_OUTPUT,shader_num++);
   f = _popen(cmd, "wt");
   if (vcos_verify(f)) {
      for (i = 0; i < count; i ++) {
            fprintf(f,
               "0x%08x_%08x\n", words[i*2 + 1], words[i*2]);
         
      }
      _pclose(f);
   }
#endif
}
#endif // ANDROID



