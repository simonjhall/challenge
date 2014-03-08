/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GLXX_SHADER_4_H
#define GLXX_SHADER_4_H

#include "middleware/khronos/glsl/glsl_common.h"
#include "middleware/khronos/glsl/glsl_dataflow.h"
#include "middleware/khronos/glxx/glxx_hw.h"

typedef struct
{
   Dataflow *x;
   Dataflow *y;
   Dataflow *z;
} GLXX_VEC3_T;

typedef struct
{
   Dataflow *x;
   Dataflow *y;
   Dataflow *z;
   Dataflow *w;
} GLXX_VEC4_T;

#define GLXX_VARYING_COLOR 0                /* vec4 */
#define GLXX_VARYING_BACK_COLOR 1           /* vec4 */
#define GLXX_VARYING_EYESPACE 2             /* vec3 */
#define GLXX_VARYING_POINT_SIZE 3           /* [point_size] */
#define GLXX_VARYING_TEX_COORD(i) (4+(i))   /* vec2 or vec3 */
#define GLXX_NUM_VARYINGS 12

#define GLXX_STATE_MASK 0x81000000
/*
   Returns the offset into the server state matrix
   Preconditions:
      state is a valid pointer to a GLXX_SERVER_STATE_T structure
      x is a pointer to a float
      state <= x < state+sizeof(GLXX_SERVER_STATE_T)
   Postconditions:
      GLXX_STATE_MASK <= result < GLXX_STATE_MASK + sizeof(GLXX_SERVER_STATE_T)
*/

#define GLXX_STATE_OFFSET(x) (int)((char *)&(((GLXX_SERVER_STATE_T *)(0))->x)) | GLXX_STATE_MASK
/*
   Returns a pointer to an  offset into the server state matrix
   Preconditions:
      state is a valid pointer to a GLXX_SERVER_STATE_T structure
      GLXX_STATE_MASK <= result < GLXX_STATE_MASK + sizeof(GLXX_SERVER_STATE_T)
   Postconditions:
      result is a pointer to a float
      state <= result < state+sizeof(GLXX_SERVER_STATE_T)
*/
#define GLXX_READ_STATE_OFFSET(state, a) (float *)((char *)state + (int)(a^GLXX_STATE_MASK))

#define GLXX_ATTRIB_MASK 0x82000000
/*
   Returns the offset into the attribs array
   Preconditions:
      attrib is a valid pointer to a GLXX_ATTRIB_T[GLXX_CONFIG_MAX_VERTEX_ATTRIBS] array
      x is a pointer to a float
      attrib <= x < attrib+sizeof(GLXX_ATTRIB_T)*GLXX_CONFIG_MAX_VERTEX_ATTRIBS
   Postconditions:
      GLXX_ATTRIB_MASK <= result < GLXX_ATTRIB_MASK + sizeof(GLXX_ATTRIB_T)*GLXX_CONFIG_MAX_VERTEX_ATTRIBS
*/
#define GLXX_ATTRIB_OFFSET(x) (int)((char *)&(((GLXX_ATTRIB_T *)((x)*sizeof(GLXX_ATTRIB_T)))->value)) | GLXX_ATTRIB_MASK
/*
   Returns a pointer to an  offset into the attribs array
   Preconditions:
      attrib is a valid pointer to a LXX_ATTRIB_T[GLXX_CONFIG_MAX_VERTEX_ATTRIBS] array
      GLXX_ATTRIB_MASK <= result < GLXX_ATTRIB_MASK + sizeof(GLXX_ATTRIB_T)*GLXX_CONFIG_MAX_VERTEX_ATTRIBS
   Postconditions:
      result is a pointer to a float
      attrib <= x < attrib+sizeof(GLXX_ATTRIB_T)*GLXX_CONFIG_MAX_VERTEX_ATTRIBS
*/
#define GLXX_READ_ATTRIB_OFFSET(attrib, a) (float *)((char *)attrib + (int)(a^GLXX_ATTRIB_MASK))


typedef struct {
   GLXX_VEC4_T *vertex;
   Dataflow *point_size;
   GLXX_VEC4_T *varying[GLXX_NUM_VARYINGS];
   Dataflow *force_vpm_read;
} GLXX_VERTEX_CARD_T;

typedef struct {
   GLXX_VEC4_T *varying[GLXX_NUM_VARYINGS];
   Dataflow *root;
} GLXX_FRAGMENT_CARD_T;

Dataflow *glxx_cfloat(float f);
Dataflow *glxx_cint(uint32_t i);
Dataflow *glxx_cbool(bool b);
GLXX_VEC3_T *glxx_vec3(Dataflow *x, Dataflow *y, Dataflow *z);
GLXX_VEC4_T *glxx_vec4(Dataflow *x, Dataflow *y, Dataflow *z, Dataflow *w);
GLXX_VEC4_T *glxx_rep4(Dataflow *x);
GLXX_VEC3_T *glxx_crep3(float f);
GLXX_VEC4_T *glxx_crep4(float f);
Dataflow *glxx_ustandard(uint32_t i);
Dataflow *glxx_ustandard_int(uint32_t i);
Dataflow *glxx_u(uint32_t i, uint32_t j);
GLXX_VEC3_T *glxx_u3(uint32_t i);
GLXX_VEC3_T *glxx_u3t(uint32_t i);
GLXX_VEC4_T *glxx_u4(uint32_t i);
GLXX_VEC4_T *glxx_u4t(uint32_t i);
Dataflow *glxx_fetch_vpm(Dataflow *prev);
Dataflow *glxx_vertex_set(Dataflow *param, DataflowFlavour flavour);
Dataflow *glxx_vpmw(Dataflow *param, Dataflow *dep);
void glxx_iodep(Dataflow *consumer, Dataflow *supplier);
Dataflow *glxx_vary(uint32_t row, Dataflow *dep);
Dataflow *glxx_vary_non_perspective(uint32_t row, Dataflow *dep);
Dataflow *glxx_get_col(Dataflow *prev);
Dataflow *glxx_fragment_get(DataflowFlavour flavour);
Dataflow *glxx_mul(Dataflow *a, Dataflow *b);
Dataflow *glxx_add(Dataflow *a, Dataflow *b);
Dataflow *glxx_sub(Dataflow *a, Dataflow *b);
Dataflow *glxx_abs(Dataflow *a);
Dataflow *glxx_fmax(Dataflow *a, Dataflow *b);
Dataflow *glxx_fmin(Dataflow *a, Dataflow *b);
Dataflow *glxx_floor(Dataflow *a);
Dataflow *glxx_exp2(Dataflow *a);
Dataflow *glxx_log2(Dataflow *a);
Dataflow *glxx_rsqrt(Dataflow *a);
Dataflow *glxx_recip(Dataflow *a);
Dataflow *glxx_sqrt1(Dataflow *a);
Dataflow *glxx_negate(Dataflow *a);
Dataflow *glxx_clamp(Dataflow *a);
Dataflow *glxx_square(Dataflow *a);
Dataflow *glxx_equal(Dataflow *a, Dataflow *b);
Dataflow *glxx_less(Dataflow *a, Dataflow *b);
Dataflow *glxx_lequal(Dataflow *a, Dataflow *b);
Dataflow *glxx_bitnot(Dataflow *x);
Dataflow *glxx_bitand(Dataflow *a, Dataflow *b);
Dataflow *glxx_bitor(Dataflow *a, Dataflow *b);
Dataflow *glxx_bitxor(Dataflow *a, Dataflow *b);
Dataflow *glxx_logicnot(Dataflow *x);
Dataflow *glxx_logicor(Dataflow *a, Dataflow *b);
Dataflow *glxx_v8muld(Dataflow *a, Dataflow *b);
Dataflow *glxx_v8min(Dataflow *a, Dataflow *b);
Dataflow *glxx_v8adds(Dataflow *a, Dataflow *b);
Dataflow *glxx_v8subs(Dataflow *a, Dataflow *b);
Dataflow *glxx_cond(Dataflow *cond, Dataflow *true_value, Dataflow *false_value);
Dataflow *glxx_sum3(Dataflow *a, Dataflow *b, Dataflow *c);
Dataflow *glxx_sum4(Dataflow *a, Dataflow *b, Dataflow *c, Dataflow *d);
Dataflow *glxx_dot3(GLXX_VEC3_T *a, GLXX_VEC3_T *b);
Dataflow *glxx_dot4(GLXX_VEC4_T *a, GLXX_VEC4_T *b);
GLXX_VEC3_T *glxx_add3(GLXX_VEC3_T *a, GLXX_VEC3_T *b);
GLXX_VEC4_T *glxx_add4(GLXX_VEC4_T *a, GLXX_VEC4_T *b);
GLXX_VEC3_T *glxx_sub3(GLXX_VEC3_T *a, GLXX_VEC3_T *b);
GLXX_VEC4_T *glxx_sub4(GLXX_VEC4_T *a, GLXX_VEC4_T *b);
GLXX_VEC3_T *glxx_mul3(GLXX_VEC3_T *a, GLXX_VEC3_T *b);
GLXX_VEC4_T *glxx_mul4(GLXX_VEC4_T *a, GLXX_VEC4_T *b);
GLXX_VEC3_T *glxx_scale3(Dataflow *a, GLXX_VEC3_T *b);
GLXX_VEC3_T *glxx_normalize3(GLXX_VEC3_T *a);
GLXX_VEC3_T *glxx_negate3(GLXX_VEC3_T *a);
GLXX_VEC4_T *glxx_clamp4(GLXX_VEC4_T *a);
GLXX_VEC3_T *glxx_dehomogenize(GLXX_VEC4_T *a);
GLXX_VEC3_T *glxx_dropw(GLXX_VEC4_T *a);
GLXX_VEC4_T *glxx_nullw(GLXX_VEC3_T *a);
GLXX_VEC4_T *glxx_interp4(GLXX_VEC4_T *a, GLXX_VEC4_T *b, GLXX_VEC4_T *c);
GLXX_VEC4_T *glxx_blend4(GLXX_VEC4_T *a, GLXX_VEC4_T *b, Dataflow *c);
GLXX_VEC3_T *glxx_blend3(GLXX_VEC3_T *a, GLXX_VEC3_T *b, Dataflow *c);
GLXX_VEC4_T *glxx_cond4(Dataflow *c, GLXX_VEC4_T *t, GLXX_VEC4_T *f);
GLXX_VEC4_T *glxx_transform4x4(GLXX_VEC4_T *row0, GLXX_VEC4_T *row1, GLXX_VEC4_T *row2, GLXX_VEC4_T *row3, GLXX_VEC4_T *v);
GLXX_VEC3_T *glxx_transform3x3(GLXX_VEC3_T *row0, GLXX_VEC3_T *row1, GLXX_VEC3_T *row2, GLXX_VEC3_T *v);
Dataflow *glxx_accu_recip(Dataflow *x);
Dataflow *glxx_accu_rsqrt(Dataflow *x);
Dataflow *glxx_fix_exp2(Dataflow *x);
Dataflow *glxx_accu_pow(Dataflow *a, Dataflow *b);
void glxx_iodep3(GLXX_VEC3_T *consumer, GLXX_VEC3_T *producer);
Dataflow *glxx_unpack(Dataflow *x, DataflowFlavour flavour);
Dataflow *glxx_i_to_f(Dataflow *i);
Dataflow *glxx_x_to_f(Dataflow *x);
Dataflow *glxx_h_to_f(Dataflow *h, uint32_t i);
Dataflow *glxx_s_to_f(Dataflow *s, uint32_t i, bool norm);
Dataflow *glxx_us_to_f(Dataflow *s, uint32_t i, bool norm);
Dataflow *glxx_b_to_f(Dataflow *b, uint32_t i, bool norm);
Dataflow *glxx_ub_to_f(Dataflow *ub, uint32_t i, bool norm);
Dataflow *glxx_f_to_i(Dataflow *f);
Dataflow *glxx_perform_blend(GLXX_HW_BLEND_T blend, Dataflow *dst, Dataflow *src, bool render_alpha);
Dataflow *glxx_perform_logic_op(GLenum operation, Dataflow *s, Dataflow *d);
Dataflow *glxx_perform_color_mask(uint32_t mask, Dataflow *s, Dataflow *d);
Dataflow *glxx_backend(GLXX_HW_BLEND_T blend, GLXX_VEC4_T *color, Dataflow *discard, bool use_stencil, bool render_alpha, bool rgb565, bool boilerplate);
Dataflow *glxx_alpha_test(GLenum test, Dataflow *a, Dataflow *b);
bool glxx_schedule(Dataflow *root, uint32_t type, MEM_HANDLE_T *mh_code, MEM_HANDLE_T *mh_uniform_map, bool *threaded, uint32_t *vary_map, uint32_t *vary_count, bool fb_rb_swap);
Dataflow *glxx_vertex_backend(Dataflow *x, Dataflow *y, Dataflow *z, Dataflow *w, Dataflow *point_size, bool write_clip_header, bool write_varyings, Dataflow **vertex_vary, uint32_t *vary_map, uint32_t vary_count, bool rso_format);
Dataflow *glxx_fetch_all_attributes(GLXX_ATTRIB_ABSTRACT_T *abstract,uint32_t *uniform_index, Dataflow **attrib,Dataflow *dep,uint32_t attribs_live, uint32_t *ordering);
#endif
