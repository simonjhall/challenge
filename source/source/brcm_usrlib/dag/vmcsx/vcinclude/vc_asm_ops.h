/*=============================================================================
Copyright (c) 2003-2006 Broadcom Europe Ltd.
All rights reserved.

Project  :  Library of useful VideoCore functions
Module   :  Header file
File     :  $RCSfile$
Revision :  $Revision: #3 $

FILE DESCRIPTION
Declaration of intrinsic functions uniformly across both VCC and MetaWare.
Also some useful "saturating" scalar arithmetic operators, which one
might otherwise be tempted to write in non-portable inline assembler.
=============================================================================*/

#ifndef VC_ASM_OPS_H
#define VC_ASM_OPS_H

/*
   The following operators, which are intrinsic to the MetaWare compiler,
   are defined here for the VCC compiler, using GCC-style inline assembler.
   Each is equivalent to the corresponding assembler opcode.

   _bmask
   _brev
   _count
   _msb
   _mulhd_ss
   _mulhd_su
   _mulhd_uu
   _ror

   The following new operators are defined here for both compilers.
   Each is equivalent to the corresponding assembler opcode.

   vc_max
   vc_min
   vc_abs
   vc_lsr
   vc_asr

   The following new operators are defined here for both compilers.
   These are saturating operations that are awkward to define using "C".

   vc_add_sat32
   vc_sub_sat32
   vc_abs_sat32      (where vc_abs_sat32(0x80000000) == 0x7fffffff)
   vc_neg_sat32
   vc_shl_sat32      (second operand must be non-negative)
   vc_sat16          (clip to the range -32768..32767)
   vc_sat16pos       (clip at +32767; undefined if below -32768)

   Mixed-precision operations, commonly used in e.g. audio codecs.

   vc_lmult          equivalent to vc_shl_sat32(((short)(a))*(short)(b), 1)
   vc_lmac           equivalent to vc_add_sat32(a, vc_lmult(b, c))
   vc_lmsu           equivalent to vc_sub_sat32(a, vc_lmult(b, c))
*/

#ifndef _VIDEOCORE /* Emulate all in "C" for non-VideoCore platforms */

#include <assert.h>

#define _bmask(a,n) ( (a) & ((1<<((n)&31))-1) )

static __inline int _brev(int val) {
   int i, res = 0;
   for (i = 0; i < 32; ++i)
      if (val & (1<<i)) res |= (1<<(31-i));
   return res;
}

static __inline int _count(int val) {
   int i, res = 0;
   for (i = 0; i < 32; ++i)
      if (val & (1<<i)) ++res;
   return res;
}

static __inline int _msb(int val) {
   int msb=31;
   if (val==0) return -1;
   while ((val&(1<<msb))==0)
      msb--;
   return msb;
}

#ifdef WIN32
typedef __int64 aoint64_t;
#else
typedef long long aoint64_t;
#endif

static __inline int _mulhd_ss(int a, int b) {
   return (int)(((aoint64_t)a * (aoint64_t)b)>>32);
}

static __inline int _mulhd_su(int a, int b) {
   return (int)(((aoint64_t)a * (aoint64_t)(unsigned int)b)>>32);
}

static __inline int _mulhd_uu(int a, int b) {
   return (int)(((aoint64_t)(unsigned int)a * (aoint64_t)(unsigned int)b)>>32);
}

static __inline int _ror(int a, int b) {
   b &= 31;
   return (b) ? ((a >> b) | (a << (32-b))) : a;
}

static __inline int vc_min(int a, int b) {
   int z = a;
   if (b < z) z = b;
   return z;
}
static __inline int vc_max(int a, int b) {
   int z = a;
   if (b > z) z = b;
   return z;
}

static __inline int vc_abs(int a) {
   return (a < 0) ? -a : a;
}

static __inline int vc_add_sat32(int a, int b) {
   int r = a + b;
   if (((r ^ b) & (r ^ a)) < 0) {
      r = 0x7fffffff;
      if (a < 0) ++r;
   }
   return r;
}

static __inline int vc_sub_sat32(int a, int b) {
   int r = a - b;
   if (((a ^ b) & (a ^ r)) < 0) {
      r = 0x7fffffff;
      if (a < 0) ++r;
   }
   return r;
}

static __inline int vc_abs_sat32(int a) {
   int b = vc_abs(a);
   if (b < 0) --b;
   return b;
}

static __inline int vc_neg_sat32(int a) {
   int b = -a;
   if (b == a && b) --b;
   return b;
}

static __inline int vc_shl_sat32(int a, int b)
{
   if (_msb(vc_abs(a)) + b >= 31)
      return (a) ? ((((unsigned int)a) >> 31U) + 0x7fffffff) : 0;
   else
      return a << b;
}

/* now deprecated in favour of vc_lmult() */
static __inline int vc_shl1_sat32pos(int a)
{
   assert(a >= -0x40000000);
   if (a > 0x3fffffff) return 0x7fffffff;
   return a<<1;
}

static __inline short vc_sat16(int a) {
   if (a > 0x7fff) return 0x7fff;
   return (a < -32768) ? -32768 : a;
}

static __inline short vc_sat16pos(int a) {
   assert (a >= -0x8000);
   return (a > 0x7fff) ? 0x7fff : a;
}

static __inline int vc_lmult(short a, short b) {
   int l = a*b*2;
   if (l == (int)0x80000000) --l;
   return l;
}

#define vc_lmac(acc,b,c) vc_add_sat32((acc),vc_lmult((b),(c)))
#define vc_lmsu(acc,b,c) vc_sub_sat32((acc),vc_lmult((b),(c)))

#else

#ifdef __HIGHC__ /* MetaWare tools */

#include <vc/intrinsics.h>

#define vc_min(a,b) (_min((int)(a), (int)(b)))

#define vc_max(a,b) (_max((int)(a), (int)(b)))

#define vc_abs(a) (_abs((int)(a)))

#if defined(_VC_VERSION) && (_VC_VERSION >= 2) /* VideoCore 2 on MetaWare */

#define vc_add_sat32(a,b) _adds((a),(b))

#define vc_sub_sat32(a,b) _subs((a),(b))

#define vc_neg_sat32(a) _subs(0,(a))

#define vc_shl_sat32(a,b) _shls((a),_min((b),31))

/* now deprecated in favour of vc_lmult() */
#define vc_shl1_sat32pos(a) _shls((a),1)

#if _VC_VERSION < 3
extern short vc_sat16(int v);   /* modified _clipsh() that has short result */
# pragma intrinsic(vc_sat16,  "CLIPSH")
#else
extern short vc_sat_n(int v, int nbits);
# pragma intrinsic(vc_sat_n, "CLIPSH_R", "CLIPSH_I")
#endif

#define vc_sat16(a)    vc_sat_n((a), (a))
#define vc_sat16pos(a) vc_sat_n((a), (a))

#if 1 /* Macro version, we hope the casts will behave as expected? */
#define vc_lmult(a,b) _shls(((short)(a))*((short)(b)),1)
#else /* Function version. Clearer but may not get inlined with -g? */
static _Inline int vc_lmult(short a, short b)
{
   return _shls(a*b, 1);
}
#endif

#define vc_lmac(acc,b,c) _adds((acc),vc_lmult((b),(c)))
#define vc_lmsu(acc,b,c) _subs((acc),vc_lmult((b),(c)))

#else /* VideoCore 1 */

int _Asm vc_add_sat32(int ia, int ib) {
   %reg ia; reg ib; lab done;
   .if ia == %r0
   .if ib == %r0
   mov %r1, %r0
   addcmpbvc %r0, %r1, %r1, done
   .else
   addcmpbvc %r0, ib, ib, done
   .endif
   .elseif ib == %r0
   addcmpbvc %r0, ia, ia, done
   .else
   mov %r0, ia
   addcmpbvc %r0, ib, ib, done
   .endif
   asr %r0, 31
   mov %r1, 0x80000000
   add %r0, %r1
done:
}

int _Asm vc_sub_sat32(int ia, int ib) {
   %reg ia; reg ib; lab nosat; lab done;
   addcmpbvc ia, 0, ib, nosat
   .if (ia == %r1 || ia == %r2 || ia == %r3 )
   lsr ia, 31
   mov %r0, 0x7fffffff
   addcmpb %r0, ia, 0, done
   .else
   .if ia != %r0
   mov %r0, ia
   .endif
   lsr %r0, 31
   mov %r1, 0x7fffffff
   addcmpb %r0, %r1, 0, done
   .endif
nosat:
   sub %r0, ia, ib
done:
}

static _Inline int vc_lmult(short a, short b) {
   int prod = a*b;
   prod += prod;
   if (prod == (int)0x80000000) --prod;
   return prod;
}

int _Asm vc_lmac(int acc, short ia, short ib) {
   %reg acc; reg ia; reg ib; lab done1; lab done2;
   .if acc == %r0
   mul %r1, ia, ib
   mov %r2, %r1
   addcmpbvc %r1, %r2, %r2, done1
   add %r1, -1
done1:
   addcmpbvc %r0, %r1, %r1, done2
   .elseif acc == %r1
   mul %r0, ia, ib
   mov %r2, %r0
   addcmpbvc %r0, %r2, %r2, done1
   add %r0, -1
done1:
   addcmpbvc %r0, %r1, %r1, done2
   .else
   mul %r0, ia, ib
   mov %r1, %r0
   addcmpbvc %r0, %r1, %r1, done1
   add %r0, -1
done1:
   addcmpbvc %r0, acc, acc, done2
   .endif
   asr %r0, 31
   mov %r1, 0x80000000
   add %r0, %r1
done2:
}

int _Asm vc_lmsu(int acc, short ia, short ib) {
   %reg acc; reg ia; reg ib; lab done1; lab nosat; lab done2;
   .if acc == %r0
   mul %r1, ia, ib
   mov %r2, %r1
   addcmpbvc %r1, %r2, %r2, done1
   add %r1, -1
done1:
   addcmpbvc %r0, 0, %r1, nosat
   lsr %r0, 31
   mov %r1, 0x7fffffff
   addcmpb %r0, %r1, 0, done2
nosat:
   sub %r0, %r1
   .elseif ( acc == %r1 || acc == %r3 )
   mul %r0, ia, ib
   mov %r2, %r0
   addcmpbvc %r0, %r2, %r2, done1
   add %r0, -1
done1:
   addcmpbvc acc, 0, %r0, nosat
   lsr acc, 31
   mov %r0, 0x7fffffff
   addcmpb %r0, acc, 0, done2
nosat:
   rsub %r0, acc
   .else
   mul %r0, ia, ib
   mov %r1, %r0
   addcmpbvc %r0, %r1, %r1, done1
   add %r0, -1
done1:
   addcmpbvc acc, 0, %r0, nosat
   mov %r0, acc
   mov %r1, 0x7fffffff
   lsr %r0, 31
   addcmpb %r0, %r1, 0, done2
nosat:
   rsub %r0, acc
   .endif
done2:
}

#define vc_neg_sat32(a) (-(vc_max((a), -0x7fffffff)))

static _Inline int vc_shl_sat32(int a, int b)
{
   if (_msb(_abs(a)) + b >= 31)
      return (a) ? ((((unsigned int)a) >> 31U) + 0x7fffffff) : 0;
   else
      return a << b;
}

/* now deprecated in favour of vc_lmult() */
static _Inline int vc_shl1_sat32pos(int a)
{
   if (a > 0x3fffffff) return 0x7fffffff;
   return a<<1;
}

#define vc_sat16(a) (vc_max(-0x8000, vc_min((a), 0x7fff)))

#define vc_sat16pos(a) (vc_min((a),0x7fff))

#endif /* core */

static _Inline int vc_abs_sat32(int a) {
   a = _abs(a);
   if (a < 0) --a;  /* 0x80000000 => 0x7fffffff */
   return a;
}

#else /* not metaware... */

#ifndef __GNUC__
#error "Unsupported compiler"
#endif

#define _bmask(a,b) ({int _valuez; asm("bmask %0, %1, %2" : "=r" (_valuez) : "r" ((int)(a)), "r" ((int)(b))); _valuez; })

#define _brev(a) ({int _valueb; asm("brev %0, %1" : "=r" (_valueb) : "r" ((int)(a))); _valueb; })

#define _count(a) ({int _valuec; asm("count %0, %1" : "=r" (_valuec) : "r" ((int)(a))); _valuec; })

#define _msb(a) ({int _valued; asm("msb %0, %1" : "=r" (_valued) : "r" ((int)(a))); _valued; })

#define _mulhd_ss(a,b) ({int _valuee; asm("mulhd.ss %0, %1, %2" : "=r" (_valuee) :"r"((int)(a)), "r"((int)(b))); _valuee;})

#define _mulhd_su(a,b) ({int _valuef; asm("mulhd.su %0, %1, %2" : "=r" (_valuef) :"r"((int)(a)), "r"((int)(b))); _valuef;})

#define _mulhd_uu(a,b) ({int _valueg; asm("mulhd.uu %0, %1, %2" : "=r" (_valueg) :"r"((int)(a)), "r"((int)(b))); _valueg;})

#define _ror(a,b) ({int _valueh; asm("ror %0, %1, %2" : "=r" (_valueh) :"r"((int)(a)), "r"((int)(b))); _valueh;})

#define vc_abs(a) ({int _valuek; asm("abs %0, %1" : "=r" (_valuek) : "r" ((int)(a))); _valuek; })

#define vc_min(a,b) ({int _value1; asm("min %0, %1, %2" : "=r" (_value1) :"r"((int)(a)), "r"((int)(b))); _value1;})

#define vc_max(a,b) ({int _value2; asm("max %0, %1, %2" : "=r" (_value2) :"r"((int)(a)), "r"((int)(b))); _value2;})

#ifdef __VIDEOCORE2__

#define vc_add_sat32(a,b) ({int _valuep; asm("adds %0, %1, %2" : "=r"(_valuep) : "r"((int)(a)), "r"((int)(b))); _valuep; })

#define vc_sub_sat32(a,b) ({int _valueq; asm("subs %0, %1, %2" : "=r"(_valueq) : "r"((int)(a)), "r"((int)(b))); _valueq; })

#define vc_abs_sat32(a) ({int _valueqq; asm("abs %0,%1\n\taddcmpbge %0,0,0,LBL%=\n\tadd %0,-1\nLBL%=:\n\t" : "=r"(_valueqq) : "r"(a)); _valueqq; })

#define vc_neg_sat32(a) ({int _valuer; asm("mov %0, 0\n\tsubs %0, %0, %1" : "=&r"(_valuer) : "r"((int)(a))); _valuer; })

static __inline int vc_shl_sat32(int a, int b)
{
asm("min %0, %0, 31\n\tshls %0, %1, %0" : "+&r"(b) : "r"(a));
   return b;
}

/* now deprecated in favour of vc_lmult() */
static __inline int vc_shl1_sat32pos(int a)
{
   int result;
asm("shls %0, %1, 1" : "=r"(result) : "r"(a));
   return result;
}

#define vc_sat16(a) ({int _valueclipsh; asm("clipsh %0, %1" : "=r"(_valueclipsh) : "r"((int)(a))); (short)_valueclipsh; })

#define vc_sat16pos(a) vc_sat16(a)

static __inline int vc_lmult(short a, short b)
{
   int res, l = a*b;
asm("shls %0, %1, 1" : "=r"(res) : "r"(l));
   return res;
}

#else /* not __VIDEOCORE2__ */

static __inline int vc_add_sat32(int a, int b)
{
asm("addcmpbvc %0,%1,%1,LBL%=\n\tcmp %0,0\n\tmov %0,0x80000000\n\tbpl LBL%=\n\tadd %0,-1\nLBL%=:" : "+&r"(a) : "r"(b));
   return a;
}

static __inline int vc_sub_sat32(int a, int b)
{
   int _value;
asm("addcmpbvc %1,0,%2,LBLA%=\n\tcmp %1,%2\n\tmov %0,0x80000000\n\tblt LBLB%=\n\taddcmpb %0,-1,0,LBLB%=\nLBLA%=:\n\tsub %0,%1,%2\nLBLB%=: " : "=r"(_value) : "r"(a), "r"(b));
   return _value;
}

static __inline int vc_abs_sat32(int a)
{
   int _value;
asm("abs %0,%1\n\taddcmpbge %0,0,0,LBL%=\n\tadd %0,-1\nLBL%=:\n\t" : "=r"(_value) : "r"(a));
   return _value;
}

static __inline int vc_neg_sat32(int a)
{
   int _value;
asm("neg %0,%1\n\taddcmpbne %0,0,%1,LBL%=\n\taddcmpbeq %0,0,0,LBL%=\n\tadd %0,-1\nLBL%=: " : "=&r"(_value) : "r"(a));
   return _value;
}

static __inline int vc_shl_sat32(int a, int b)
{
   if (_msb(vc_abs(a)) + b >= 31)
      return (a) ? ((((unsigned int)a) >> 31U) + 0x7fffffff) : 0;
   else
      return a << b;
}

static __inline int vc_shl1_sat32pos(int a)
{
   int _value;
asm("mov %0,%1\n\taddcmpbvc %0,%1,%1,LBL%=\n\tmov %0,0x7fffffff\nLBL%=:" : "=&r"(_value) : "r"(a));
   return _value;
}

#define vc_sat16(a) (vc_max(-0x8000, vc_min((a), 0x7fff)))

#define vc_sat16pos(a) (vc_min((a), 0x7fff))

static __inline int vc_lmult(short a, short b) {
   int _value, _tmp;
   _tmp = a * b;
asm("mov %0,%1\n\taddcmpbvc %0,%1,%1,LBL%=\n\tadd %0,-1\nLBL%=:" : "=&r"(_value) : "r"(_tmp));
   return _value;
}

#endif /* core */

#define vc_lmac(acc,b,c) vc_add_sat32((acc),vc_lmult((b),(c)))
#define vc_lmsu(acc,b,c) vc_sub_sat32((acc),vc_lmult((b),(c)))

#endif /* toolchain */

#endif /* platform */

#define vc_lsr(a,b) (((unsigned int)(a)) >> ((unsigned int)(b)))

#define vc_asr(a,b) (((int)(a)) >> ((int)(b)))

#endif
