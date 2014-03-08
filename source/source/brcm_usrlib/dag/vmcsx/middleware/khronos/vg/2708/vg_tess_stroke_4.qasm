### ============================================================================
### Copyright (c) 2008-2014, Broadcom Corporation
### All rights reserved.
### Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
### 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
### 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
### 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
### THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
### ============================================================================

.set prev_allow_xor_0, pragma_allow_xor_0(True)
.set prev_dont_warn_when_mul_rot_inp_r5, pragma_dont_warn_when_mul_rot_inp_r5(True)

.include "vg_geomd_4.qinc"
.include "vg_tess_4.qinc"
.include "vg_util_4.qinc"

.set LEVEL_MAX, 8 # limit number of cubic segments to 256
.set TRI_LIST_HEAD, 0x042a0101
.set FLT_MAX, 0x7f7fffff
.set PI, 3.1415926535897932384626433832795
.set EPS, 1.0e-10

# ---

#.set debug, True
.set debug, False

.if debug

   .macro mk, num
      .if num >= 0 and num <= 1
         .error ("mk %d disallowed" % num)
      .endif
      mov -, (0xf1f10000)+num
   .endm

   .macro mkp, format_str, register_str, element
      .ifnset element
         .set element, 0
      .endif
      .ifnset register_str
         .set register_str, "r0"
      .endif
      mov -, 0xf1f10000
      .rep i, len(format_str)
         mov -, ord(format_str[i])
      .endr
      mov -, 0x0
      .rep i, len(register_str)
         mov -, ord(register_str[i])
      .endr
      mov -, 0x0
      .assert element < 16
      mov -, element
   .endm

   .macro fail, num
      .ifnset num
         .set num, 0x00
      .endif
      .assert not (num&~0xff)
      mkp "fail"
      mk 0xf100 | num
      nop
      bkpt
      nop
      nop
      thrend
      nop
      nop
   .endm

.else

   .macro mk, num
   .endm

   .macro mkp, format_str, register_str, element
   .endm

   .macro fail, num
   .endm
.endif

.macro unreachable
   fail 0xed
.endm

# ---

# register file a usage
# usage of ra_even() is to workaround hw-2780
# todo: some of the scalary registers could probably be merged (including ra_subd_consts)
.set remaining_ras, ras()
.set ra_scalar, ra_even()
   .set rsi_clerp_q_n, 0
   .set rsi_q_lr, 1
   .set rsi_chunk_lr, 2 # where chunk_normal|cubic jumps to when it's done. Will be load|load_interp.
   .set rsi_tess_i, 3
   .set rsi_chunks_a, 4
   .set rsi_chunks_b, 5
   .set rsi_chunks_m_size, 6
   .set rsi_t, 7
   .set rsi_chunks_vdr_setup_0, 8
   .set rsi_chunks_vpm_setup, 9
   .set rsi_out_vpm_setup, 10
   .set rsi_out_vdw_setup_0, 12
   .set rsi_alloc_p, 13
   .set rsi_alloc_size, 14
   # this corresponds to stroke_dash_clerp() in the c, but instead of an
   # if (stroke->dashing) every time, we just set this to either
   # dash_clerp_dashing or dash_clerp_not_dashing at the start
   .set rsi_dash_clerp_lbl, 15
   .set rsi_m_cos_theta, 11
.set ra_scalar2, ra()
   .set rs2i_x, 0
   .set rs2i_dash_phase, 1
   .set rs2i_dash_phase_i, 2
   .set CLOSE_X_DEGENERATE, 0x7f800002 # unusual nans
   .set CLOSE_X_NOT_IN_DASH, 0x7f800003
   .set rs2i_close_x, 3
   .set rs2i_flush_cap_q_lbl, 4
   .set rs2i_flush_join_q_lbl, 5
   .set rs2i_y, 7
   .set rs2i_close_y, 8
   .set rs2i_close_tx, 9
   .set rs2i_close_ty, 10
   .set rs2i_is_cubic, 11
   .set rs2i_clip_lr, 12
   .set rs2i_dash_clerp_lr, 13
   .set rs2i_tx, 14
   .set rs2i_ty, 15
.set ra_scalar3, ra() # this maps directly to IO_CHUNK_STROKE_I2_T
   .set rs3i_line_width, 0
   .set rs3i_clip_outer, 1
   .set rs3i_miter_limit, 5
   .set rs3i_oo_theta, 6
   .set rs3i_cos_theta, 7
   .set rs3i_cos_theta_inter, 8
   .set rs3i_sin_theta, 9
   .set rs3i_oo_flatness_sq, 10
   .set rs3i_width_hack_clerp, 11
   .set rs3i_width_hack_round, 12
   .set rs3i_bitfield, 13
   .set rs3i_dash_oo_scale, 14
   .set rs3i_initial_dash_phase, 15
.set ra_user_to_surface_clip_inner, ra()
.set ra_chunk, ra_even()
.set ra_cubics, [ra_even() for i in range(LEVEL_MAX)] # could get rid of ra_cubics[0]?
.set ra_dash_pattern, ra() # io chunk 3
.set ra_subd_flags1, ra()
.set ra_subd_flags2, ra()
.set ra_subd_consts, ra()
.set ra_clerp_q_x0, ra()
.set ra_clerp_q_y0, ra()
.set ra_clerp_q_x1, ra()
.set ra_clerp_q_y1, ra()
.set CLERP_Q_TX0_NORMAL, 0x7f800002 # unusual nan
.set ra_clerp_q_tx0, ra()
.set ra_clerp_q_ty0, ra()
.set ra_cap_q_x, ra()
.set ra_cap_q_y, ra()
.set ra_cap_q_tx, ra()
.set ra_cap_q_ty, ra()
.set ra_dc_locals, ra() # dash clerp locals
   .set rsi_dc_locals_x, 0
   .set rsi_dc_locals_prev_phase, 1
   .set rsi_dc_locals_sod, 2
   .set rsi_dc_locals_next_x, 3
   .set rsi_dc_locals_dx, 4
   .set rsi_dc_locals_y, 7
   .set rsi_dc_locals_next_y, 10
   .set rsi_dc_locals_dy, 11
   .set rsi_dc_locals_tx, 14
   .set rsi_dc_locals_ty, 15

# chunk_normal registers shared with chunk_cubic:
.set ra_chunk_normal_locals, ra_cubics[0] # [0..6]:x0..x6, [7..13]:y0..y6, [14]:tx, [15]:ty
   .set rsi_chunk_normal_locals_x0, 0
   .set rsi_chunk_normal_locals_y0, 7
   .set rsi_chunk_normal_locals_tx, 14
   .set rsi_chunk_normal_locals_ty, 15
   #TODO-OPT - put tx,ty at 0,1 instead, to take advantage of non-accum rotations.

.set ra_temp, ra_even()
.set ra_temp2, ra()
.set ra_temp3, ra()
.set ra_temp4, ra()

# register file b usage
.set remaining_rbs, rbs()
.set rb_scalar, rb()
   .set rbsi_cap_q_n, 0
   .set rbsi_join_q_n, 1
   .set rbsi_m_cos_theta_inter, 11
.set rb_cubics, [rb() for i in range(LEVEL_MAX)] # don't really need rb_cubics[LEVEL_MAX - 1]
.set rb_list_tail, rb()
.set rb_temp1, rb()
.set rb_temp, rb_temp1
.set rb_temp2, rb()
.set rb_temp3, rb()
.set rb_temp4, rb()
.set rb_temp5, rb()
.set rb_temp6, rb()
.set rb_temp7, rb()
.set rb_temp8, rb()
.set rb_temp9, rb()
.set rb_temp10, rb()
.set rb_temp11, rb()
.set rb_temp12, rb()
.set rb_clerp_q_tx1, rb()
.set rb_clerp_q_ty1, rb()
.set rb_join_q_x, rb()
.set rb_join_q_y, rb()
.set rb_join_q_tx0, rb()
.set rb_join_q_ty0, rb()
.set rb_join_q_tx1, rb()
.set rb_join_q_ty1, rb()
.set rb_inactive_elems, rb() # negative means active
.set rb_eps, rb()

# vpm usage: same as in tess_fill.qasm


::vg_tess_stroke_shader

.ifset VG_TESS_RTL_SIM
   # we've permitted the assembler to convert movs of 0 to xors of r0. if r0
   # contains x, this won't work. so put something (anything) in r0
   mov  r0, 0xdeadbeef

   # using registers with x elements can cause issues. registers we do
   # conditional writes to might be used with some unwritten elements. clear
   # them down now to make sure those unwritten elements won't be xs
   mov  ra_scalar2, 0     ; mov  rb_clerp_q_tx1, 0
   mov  ra_clerp_q_x0, 0  ; mov  rb_clerp_q_ty1, 0
   mov  ra_clerp_q_y0, 0  ; mov  rb_join_q_x, 0
   mov  ra_clerp_q_x1, 0  ; mov  rb_join_q_y, 0
   mov  ra_clerp_q_y1, 0  ; mov  rb_join_q_tx0, 0
   mov  ra_clerp_q_tx0, 0 ; mov  rb_join_q_ty0, 0
   mov  ra_clerp_q_ty0, 0 ; mov  rb_join_q_tx1, 0
   mov  ra_cap_q_x, 0     ; mov  rb_join_q_ty1, 0
   mov  ra_cap_q_y, 0     ; nop
   mov  ra_cap_q_tx, 0    ; nop
   mov  ra_cap_q_ty, 0    ; nop
.endif

.ifset VG_TESS_SEMA
   sacq  -, 0
   srel  -, 0
.endif

   # this setup stuff is only run once per path, so i've tried to keep it simple
   # and straight-forward instead of making it fast

   # figure out where our vpm space is
   mov  r0, qpu_num ; mov  ra_scalar, 0  @geomd_open
   and  r1, r0, 3   ; mov  r2, 3
   shr  r0, r0, 2   ; v8adds  r2, r2, 2
   shl  r1, r1, 4   ; mul24  r0, r0, r2
   add  r0, r0, r1  ; mov  rb_scalar, 0

   # r0: vpm y

   # set chunks_vpm_setup
   mov  r1, vpm_setup(1, 1, h32(0))
   sub.setf  -, elem_num, rsi_chunks_vpm_setup
   add.ifz  ra_scalar, r1, r0

   # get alloc p/size (might be unusable, but we'll decide that later)
   add  vr_setup, r1, r0
   add  r3, r1, r0 ; mov  r1, vpm
   .assert rsi_alloc_size == (rsi_alloc_p + 1)
   mov.setf  -, elem_range(rsi_alloc_p, 2)
   nop ; mov.ifnz  ra_scalar, r1 >> rsi_alloc_p

   # set chunks_vdr_setup_0
   mov  r1, vdr_setup_0(1, 16, dma_h32(0, 0), 0, 8)
   shl  r2, r0, 4
   sub.setf  -, elem_num, rsi_chunks_vdr_setup_0
   add.ifz  ra_scalar, r1, r2

   # load io chunk (into chunks space in vpm)
   add  vr_setup, r1, r2 ## vdr read IO_CHUNK_I_T into VPM rd-row-0
   mov  vr_addr, unif

   # set out_vpm_setup
   mov  r1, vpm_setup(1, 1, h32(2)) # notice stride of 1 row.
   sub.setf  -, elem_num, rsi_out_vpm_setup
   add.ifz  ra_scalar, r1, r0

   # set out_vdw_setup_0
   mov  r1, vdw_setup_0(1, 16, dma_h32(2, 0))
   shl  r2, r0, 7
   sub.setf  -, elem_num, rsi_out_vdw_setup_0
   add.ifz  ra_scalar, r1, r2

   # get io chunk from vpm
   mov  -, vr_wait
   mov  vr_setup, r3
   mov  r0, vpm  @geomd_i(geomd_i_clip_inner) @geomd_region_set_a(11)

   # IO_CHUNK_I_T (r0) looks like this:
   # [0] tess_i
   # [1] chunks_a
   # [2] chunks_b
   # [3] chunks_size
   # [4] t
   # [5] user_to_surface[0]
   # [11] clip[0] -- clip_inner for strokes
   # [15] flipbit

   #TODO verify this is all correct

   # load IO_CHUNK_STROKE_I2_T
   nop
   mov r2, ra_scalar
   nop
   nop ; mov r5rep, r2 << rsi_chunks_vdr_setup_0
   mov vr_setup, r5 ; mov r1, r5
   mov vr_addr, unif
   mov  -, vr_wait
   mov  vr_setup, r3
   mov ra_scalar3, vpm ; mov r2, vpm  @geomd_i(geomd_i_clip_outer) @geomd_region_set_a(rs3i_clip_outer)

   mov r5rep, f(-1.0)
   sub.setf -, elem_num, rsi_m_cos_theta
   nop ; fmul.ifz (ra_scalar >> rs3i_cos_theta) << rsi_m_cos_theta, r2, r5
   sub.setf -, elem_num, rbsi_m_cos_theta_inter
   nop ; fmul.ifz (rb_scalar >> rs3i_cos_theta_inter) << rbsi_m_cos_theta_inter, r2, r5
   nop ; mov r5rep, r2 << rs3i_bitfield
   shr r5rep, r5, 10
   and r5rep, r5, 0xf
   sub.setf -, elem_num, rs2i_dash_phase_i
   mov.ifz ra_scalar2, r5     # stroke->dash_phase_i = stroke->i2->dash_phase_i
                              mkp "init: i2->dash_phase_i: %d\n", "r5"
   nop ; mov r5rep, r2 << rs3i_initial_dash_phase
                              mkp "init: i2->dash_phase: %f\n", "r5"
   sub.setf -, elem_num, rs2i_dash_phase
   mov.ifz ra_scalar2, r5     # stroke->dash_phase = stroke->i2->dash_phase

   # change this later if it turns out we're dashing
   sub.setf -, elem_num, rsi_dash_clerp_lbl
   mov.ifz ra_scalar, a:dash_clerp_not_dashing

   # choice of join
   nop ; mov r5rep, r2 << rs3i_bitfield
   and r5rep, r5, 0xc # join is bits 2..3
   sub.setf -, r5, 4 # result is -4 for MITER, 0 for ROUND, 4 for BEVEL.
   mov.ifnz r5rep, a:flush_join_q_bevel
   mov.ifn r5rep, a:flush_join_q_miter
   mov.ifz r5rep, a:flush_join_q_round
   sub.setf -, elem_num, rs2i_flush_join_q_lbl
   mov.ifz ra_scalar2, r5

   # choice of cap
   nop ; mov r5rep, r2 << rs3i_bitfield
   and r5rep, r5, 0x3 # cap is bits 0..1
   sub.setf -, r5, 1 # result is -1 for BUTT, 0 for ROUND, 1 for SQUARE.
   mov.ifnz r5rep, a:flush_cap_q_square
   mov.ifn r5rep, a:flush_cap_q_butt
   mov.ifz r5rep, a:flush_cap_q_round
   sub.setf -, elem_num, rs2i_flush_cap_q_lbl
   mov.ifz ra_scalar2, r5

   # load IO_CHUNK_STROKE_I3_T if dashing was true.
   nop ; mov r5rep, r2 << rs3i_bitfield
   mov r2, 0x10 # dashing is bit 4
   and.setf -, r5, r2
   brr.allz -, r:1f           # branch taken if !dashing
      nop
      nop
      nop
                              mkp "dashing is on\n"
   mov vr_setup, r1
   mov vr_addr, unif
   mov  -, vr_wait
   mov  vr_setup, r3
   mov ra_dash_pattern, vpm
   sub.setf -, elem_num, rsi_dash_clerp_lbl
   mov.ifz ra_scalar, a:dash_clerp_dashing
   mov  unif_addr_rel, -1
:1
   mov  unif_addr_rel, -2 # unif stream now at first unif again: this is where the IO_CHUNK_O will be written.

   mk 0x0100
   # load initial (non-io) chunks
   # setup stuff that depends on whether or not we're doing interpolation
   mov.setf  -, r0 ; mov  r1, ra_scalar
   sub.ifz.setf  -, elem_num, 2 # chunks_b==0 if no interpolation
   nop ; mov  r1, r1 << rsi_chunks_vdr_setup_0
   sub  r1, r1, -16 ; mov  vr_setup, r1
   brr.anyz  -, r:1f
   # branch delay slots {
   nop ; mov  vr_addr, r0 << 1
   sub.setf  -, elem_num, rsi_chunk_lr
   mov.ifz  ra_scalar, a:load
   # }
   # we're interpolating...
   mov.ifz  ra_scalar, a:load_interp
   sub.setf  -, elem_num, rsi_chunks_vpm_setup ; mov  vr_setup, r1
   nop ; mov  vr_addr, r0 << 2
   mov  r1, 1 << 20
   add.ifz  ra_scalar, ra_scalar, r1
:1

   # tess_i/chunks_a/chunks_b/chunks_size
   .assert rsi_chunks_a == (rsi_tess_i + 1)
   .assert rsi_chunks_b == (rsi_chunks_a + 1)
   .assert rsi_chunks_m_size == (rsi_chunks_b + 1)
   .assert rsi_t == (rsi_chunks_m_size + 1)
   mov.setf  -, elem_range(rsi_tess_i, 5)
   nop ; mov.ifnz  ra_scalar, r0 >> rsi_tess_i
   sub.setf  -, elem_num, rsi_chunks_m_size
   sub.ifz  ra_scalar, 0, ra_scalar

   # user_to_surface/clip_inner
   nop ; mov  ra_user_to_surface_clip_inner, r0 << 5

   ## no bbox stuff

   ## magic numbers
   mov  ra_subd_flags1, 2 * [-2, -2, -1, -1, 0, 0, 1, 1]
   mov  ra_subd_flags2, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -1, 1, 1, 1, 1]
   .assert rs3i_line_width == 0
   mov  r1, ra_scalar3 ; fmul  r5rep, ra_scalar3, f(0.5)
   sub.setf  -, elem_num, 11 ; fmul  r5rep, r5, r5
   mov  ra_subd_consts, r5 ; mov  r5rep, r1 << rs3i_oo_flatness_sq
   fsub.ifnz  ra_subd_consts, f(0.0), r5
   # ra_subd_consts: garbage, -1/flatness^2, garbage[5], -1/flatness^2, garbage[3], (line_width/2)^2, garbage[4]
   mov rb_eps, f(EPS)

   # handle flipbit and associate qpu with tess index
   nop ; mov  r5rep, r0 << 15
   empty_vpm_fifo # workaround for hw-2497
   mov  -, mutex ; mov  r1, r5
   mov  r5rep, r0
   # r1: flipbit (all elems)
   # r5: tess_i (all elems)
   mov  vr_setup, vpm_setup(2, 16, h32(15))
   and  r0, vpm, 1           ; mov  r2, vpm
   sub.setf  -, elem_num, r5 ; mov  r3, vpm
   # r0: current flipbits
   # r2: first common vpm row
   # r3: second common vpm row
   xor.ifz.setf  -, r0, r1
   brr.anyz  -, r:1f
   # branch delay slots {
   sub.setf  -, qpu_num, elem_num ; mov  r0, r3 # save second vpm row for later
   sub.setf  -, elem_num, r5      ; mov.ifz  r3, r5 # associate qpu with tess index
   mov  vw_setup, vpm_setup(1, 0, h32(31))
   # }
   # flipbit doesn't match...
   sub.setf  -, qpu_num, elem_num ; mov.ifz  r2, r1 # correct flipbit + clear tess alloc stuff
   mov  r0, -1                    ; mov.ifz  r3, -1 # overwrite saved row so we won't use saved alloc stuff
   sub.setf  -, r3, r5            ; mov.ifz  r3, r5 # reassociate qpu with tess index (unassociated in previous instruction so it wouldn't get flagged)
   mov  vw_setup, vpm_setup(2, 16, h32(15))
   mk 0x0101
   mov  vpm, r2                   ; mov.ifz  r3, -1 # mark all saved alloc stuff for this tess index as invalid
:1
   mov  vpm, r3 # second common vpm row
   empty_vpm_fifo # make sure vpm writes are done before we release the mutex
   mov  mutex, 0

   # if qpu wasn't associated with this tess index before, can't use saved alloc stuff
   sub  r0, r0, r5 ; mov  r1, 0
   sub  r5rep, r1, qpu_num
   mov.setf  -, elems(rsi_alloc_size)
   nop ; mov  r5rep, r0 >> r5
   mov.ifnz.setf  -, r5
   mov.ifnz  ra_scalar, 0

   # initial value of list_tail: 0x0 0x0 0xbfff0000 0x00000012
   mov  rb_list_tail, 0x0
   sub.setf -, elem_num, 2
   mov.ifz rb_list_tail, 0xbfff0000
   sub.setf -, elem_num, 3
   mov.ifz  rb_list_tail, 18 # return
   mk 0x0102
   .if debug
      sub.setf -, elem_num, 4
      mov.ifnn rb_list_tail, 0xfefefefe
   .endif

   #TODO review this: we need at least 212 bytes of space to start at rsi_alloc_p:
   # - 16 for the first list's tail
   # - 192 (16*3*4) for at least 1 batch of tris in the first list
   # - 4 for the first list's head: 0x042a0101
   # if we don't have 212 bytes, we'll have to alloc
   mov.setf  -, neg_elems(elems(rsi_alloc_size))
   mov  r0, 212
   sub.ifn.setf  -, ra_scalar, r0 ; nop # alloc_size -= 212
   mov  ra_temp, a:1f # alloc will return here
   brr.anyn  -, r:alloc_first # branch taken if new alloc_size<0
      nop
      nop
      nop

   # if here, we have at least 212 bytes at rsi_alloc_p. write out the tail of the first list
   add  r0, ra_scalar, -16  ; mov  r1, ra_scalar # alloc_p and alloc_size decremented 16 bytes.
   mov  r2, -(12 << 16) # only 4 elements
   add  r1, r1, r2         ; mov  vw_setup, r1 << rsi_out_vpm_setup
   nop                     ; mov  vpm, rb_list_tail
   nop                     ; mov  vw_setup, r1 << rsi_out_vdw_setup_0
   mov.setf  -, elems(rsi_alloc_p, rsi_alloc_size)
   mov.ifnz  ra_scalar, r0 ; mov  vw_addr, r0 << rsi_alloc_p
:1

   # setup list tail for all the future lists: 0x0 0x0 0xbfff0000 0x10010101
   # alloc must write the branch dest into the word after the 0x10010101
   sub.setf  -, elem_num, 3
   mov.ifz  rb_list_tail, 0x10010101 # branch

   sub.setf  -, elem_num, rsi_chunks_b
   mov.ifz.setf  -, ra_scalar
   brr.anyz  -, r:load # branch taken if chunks_b==0
      nop
      nop
      nop

:load_interp

   mov  r0, ra_scalar        ; nop
   mov  -, vr_wait           ; mov  r1, rsi_chunks_m_size
   sub.setf  -, elem_num, r1 ; mov  vr_setup, r0 << rsi_chunks_vpm_setup
   mov  r1, VG_TESS_CHUNK_NORMAL_SIZE
   add.ifz.setf  -, r0, r1   ; mov  r5rep, r0 << rsi_t
   # anyz: last chunk
   fsub  r3, f(1.0), r5      ; mov  r2, r5
   # r2: t, r3: 1 - t
   mov  ra_chunk, vpm        ; fmul  r3, r3, vpm
   brr.anyz  -, r:chunk_normal
   # branch delay slots {
   mov  r5rep, ra_chunk.8a   ; fmul  r2, r2, vpm
   fadd  r2, r2, r3          ; mov.ifz  ra_scalar, 0
   mov.setf  -, r5           ; mov  r3, r0 << rsi_chunks_vdr_setup_0
   # }
   mov.ifz  r1, VG_TESS_CHUNK_CUBIC_SIZE
   add  r0, r0, r1           ; mov  vr_setup, r3
   mov  ra_temp, a:chunk_normal
   nop                       ; mov  vr_addr, r0 << rsi_chunks_a
   mov.ifz  ra_temp, a:chunk_cubic
   sub  vr_setup, r3, -16    ; nop
   bra  -, ra_temp
   # branch delay slots {
   nop                       ; mov  vr_addr, r0 << rsi_chunks_b
   mov.setf  -, elems(rsi_chunks_a, rsi_chunks_b, rsi_chunks_m_size)
   mov.ifnz  ra_scalar, r0   ; nop
   # }

:load
   mk 0x0200

   mov  r0, ra_scalar        ; nop
   mov  -, vr_wait           ; mov  r1, rsi_chunks_m_size
   sub.setf  -, elem_num, r1 ; mov  vr_setup, r0 << rsi_chunks_vpm_setup
   mov  r1, VG_TESS_CHUNK_NORMAL_SIZE
   add.ifz.setf  -, r0, r1   ; nop
   # anyz: last chunk
   brr.anyz  -, r:chunk_normal
   # branch delay slots {
   mov.ifz  ra_scalar, 0     ; v8adds  r3, 12, 12
   mov  r5rep, vpm           ; mov  r2, vpm
   shl.setf  -, r5, r3       ; mov  ra_chunk, r2
   # }
   mov.ifz  r1, VG_TESS_CHUNK_CUBIC_SIZE
   add  r0, r0, r1           ; mov  vr_setup, r0 << rsi_chunks_vdr_setup_0
   brr.allz  -, r:chunk_cubic
   # branch delay slots {
   nop                       ; mov  vr_addr, r0 << rsi_chunks_a
   mov.setf  -, elems(rsi_chunks_a, rsi_chunks_m_size)
   mov.ifnz  ra_scalar, r0   ; nop
   # }
   # fall through to chunk_normal...

# Corresponds to tess.c:stroke_chunk_normal()
#
# Preconditions:
#
# ra_chunk.8a[15]: subpath_first
# ra_chunk.8b[15]: subpath_last
# ra_chunk.8c[15]: close
# ra_chunk.8d[15]: explicit_n
# r2: garbage*1, x0, ..., x6, y0, ..., y6 (not transformed by user->surface yet)
#
:chunk_normal

   mkp "chunk_normal: explicit_n.close.subpath_last.subpath_first:0x%08x\n", "ra_chunk", 15
   mk 0x4000

   # move vertices into place
   nop ; mov ra_chunk_normal_locals, r2 << 1

   # Replicate ra_chunk[15] across ra_chunk for convenience
   mov r0, ra_chunk
   nop
   nop ; mov r5rep, r0 << 15
   mov ra_chunk, r5
   #TODO make aliases for ra_chunk.8a etc (assembler doesn't support atm)

   sub.setf -, elem_num, rs2i_is_cubic
   mov.ifz ra_scalar2, 0                                 # `stroke->is_cubic = false`

   # c local `close` stored in ra_chunk.8c

   # chunk->subpath_first?
   mov.setf -, ra_chunk.8a # subpath first
   brr.allz -, r:chunk_normal_loop_preamble # branch taken if !chunk->subpath_first
      nop
      nop
      nop
   mkp "chunk_normal: subpath_first\n"

   .assert rs2i_x == rsi_chunk_normal_locals_x0 and rs2i_y == rsi_chunk_normal_locals_y0
   mov.setf -, elems(rs2i_x, rs2i_y)
   mov.ifnz ra_scalar2, ra_chunk_normal_locals           # `stroke->x/y = chunk->x/ys[0]`
   sub.setf -, elem_num, rs2i_tx
   mov.ifz ra_scalar2, f(1.0)                            # `stroke->tx = 1.0f`
   sub.setf -, elem_num, rs2i_ty
   mov.ifz ra_scalar2, f(0.0)                            # `stroke->ty = 0.0f`
   sub.setf -, elem_num, rs2i_close_x
   mov.ifz ra_scalar2, CLOSE_X_DEGENERATE                # `stroke->close_x = CLOSE_X_DEGENERATE`

   mov r0, ra_scalar3
   mov r1, (1<<14)                                       # dash_phase_reset is at bit 14.
   nop ; mov r5rep, r0 << rs3i_bitfield
   and.setf -, r1, r5
   brr.anyz -, r:1f                                      # branch taken if !stroke->i2->dash_phase_reset
      nop
      nop
      nop
   shr r5rep, r5, 10
   and r5rep, r5, 0xf
   sub.setf -, elem_num, rs2i_dash_phase_i
   mov.ifz ra_scalar2, r5                                # `stroke->dash_phase_i = stroke->i2->dash_phase_i`
   nop ; mov r5rep, r0 << rs3i_initial_dash_phase
   sub.setf -, elem_num, rs2i_dash_phase
   mov.ifz ra_scalar2, r5                                # `stroke->dash_phase = stroke->i2->dash_phase`
:1

   brr -, r:chunk_normal_loop_next_iteration             # takes care of `i = (chunk->subpath_first ? 1 : 0)`
      nop
      nop
      nop


:chunk_normal_loop_preamble

   mov.setf -, ra_chunk.8d # explicit n
   brr.allz -, r:chunk_normal_loop_end
      nop
      nop
      nop

:chunk_normal_loop_begin
   # start of the for-loop
   # note that the subpath_first case was taken care of above.
   # x/y means x[0]/y[0] in comments below, which is actually x[i]/y[i] because the array is shifted down each iteration.

   .assert rsi_chunk_normal_locals_x0 == rs2i_x          # these assumptions are used in several places below.
   .assert rsi_chunk_normal_locals_y0 == rs2i_y

   mov r0, ra_scalar2
   fsub.setf r0, ra_chunk_normal_locals, r0              # r0[x] = tx = x-stroke->x, r0[y] = ty = y-stroke->y
   mov.ifnz.setf -, elems(rsi_chunk_normal_locals_x0, rsi_chunk_normal_locals_y0)
   brr.anynz -, r:1f
      sub.setf -, ra_chunk.8b, 1 # subpath last - 1
      sub.ifz.setf -, ra_chunk.8d, 1 # explicit n - 1
      sub.ifz.setf -, elem_num, rs2i_close_x
   mov r5rep, CLOSE_X_DEGENERATE
   sub.ifz.setf -, ra_scalar2, r5 # close_x - CLOSE_X_DEGENERATE
   brr.allnz -, r:chunk_normal_loop_next_iteration # continue
      nop
      nop
      nop
   mk 0x401d
   mov ra_chunk.8c, 0 # close = false
:1

   nop ; fmul r1, r0, r0                                    # r1[x] = tx*tx, r1[y] = ty*ty
   nop ; mov r2, r0 >> (rsi_chunk_normal_locals_tx - rsi_chunk_normal_locals_x0) # r2[tx] = tx
   sub.setf -, rsi_chunk_normal_locals_ty, elem_num
   nop ; mov.ifz r2, r0 >> (rsi_chunk_normal_locals_ty - rsi_chunk_normal_locals_y0) # r2[ty] = ty
   mov r5rep, r1<<rsi_chunk_normal_locals_x0                # r5 = tx*tx
   mov r1, r5  ; mov r5rep, r1<<rsi_chunk_normal_locals_y0  # r1[all elem] = tx*tx ; r5 = ty*ty
   fadd r1, r1, r5                                          # r1[all elem] = (tx*tx)+(ty*ty) # hyp_sq

   mk 0x401e
   fsub.setf -, rb_eps, r1                                  # N set on all flags if hyp_sq > EPS
   # one newton-raphson iteration
   mov rsqrt, r1                                            # r4 = oo_hyp (estimate) in 3 cycles
   mov r5rep, f(1.5)
   nop ; fmul r1, r1, f(0.5)                                # r1 = hyp_sq*0.5f
   nop ; fmul r1, r1, r4
   nop ; fmul r1, r1, r4                                    # r1 = (hyp_sq*0.5f) * est * est
   fsub r1, r5, r1
   nop ; fmul r1, r4, r1                                    # r1 = oo_hyp (more accurate)

   mov.ifnn r0, ra_scalar2                                  # if !(hyp_sq > EPS): r0[tx/ty] = stroke->tx/ty
   nop
   nop ; fmul.ifn r0, r2, r1                                # if (hyp_sq > EPS): r0[tx] = r2[tx] * oo_hyp, similar for ty
   mov.setf -, elems(rsi_chunk_normal_locals_tx, rsi_chunk_normal_locals_ty)
   mov.ifnz ra_chunk_normal_locals, r0

   nop ; mov r5rep, ra_scalar2<<rs2i_close_x
   mov r0, CLOSE_X_DEGENERATE
   sub.setf -, r5, r0
   bra.allz -, a:2f     # branch taken if floats_identical(stroke->close_x, CLOSE_X_DEGENERATE)
      nop ; mov r5rep, ra_scalar2<<rs2i_dash_phase_i
      and.setf -, r5, 0x1
      nop
   bra.allnz -, a:3f    # branch taken if (stroke->dash_phase_i & 0x1)
      # add join, with tx/y1=locals tx/y
      mov r1, ra_chunk_normal_locals
      sub.setf -, elem_num, rsi_q_lr
      mov.ifz ra_scalar, a:3f
   brr -, r:add_join
      nop
      nop
      nop
:2

   mkp "chunk_normal: floats_identical...\n"
   nop ; mov r5rep, ra_scalar2 << rs2i_dash_phase_i
   and.setf -, r5, 0x1  # allnz if (dash_phase_i & 0x1), otherwise allz.
   brr.allz -, r:1f
      sub.setf -, elem_num, rs2i_close_x
      nop
      nop
   brr -, r:3f
      mov.ifz ra_scalar2, CLOSE_X_NOT_IN_DASH                        # `stroke->close_x = CLOSE_X_NOT_IN_DASH`
      nop
      nop
:1
   mk 0x4020
   mov r0, ra_scalar2 ; mov r5rep, ra_scalar2<<rs2i_x
   sub.setf -, elem_num, rs2i_close_y ; mov.ifz ra_scalar2, r5       # ; `stroke->close_x = stroke->x`
   mov r0, ra_chunk_normal_locals ; mov r5rep, r0<<rs2i_y
   sub.setf -, elem_num, rs2i_close_tx ; mov.ifz ra_scalar2, r5      # ; `stroke->close_y = stroke->y`
   nop ; mov r5rep, r0<<rsi_chunk_normal_locals_tx
   sub.setf -, elem_num, rs2i_close_ty ; mov.ifz ra_scalar2, r5      # ; `stroke->close_ty = tx`
   nop ; mov r5rep, r0<<rsi_chunk_normal_locals_ty
   nop ; mov.ifz ra_scalar2, r5                                      # ; `stroke->close_ty = ty`
:3

   # call dash_clerp_*
   nop
   mov r1, ra_chunk_normal_locals                           # r1[0] = x, r1[7] = y, r1[14] = tx, r1[15] = ty
   sub.setf -, elem_num, rs2i_dash_clerp_lr
   mov.ifz ra_scalar2, a:1f
   .assert rsi_dash_clerp_lbl==15
   bra -, ra_scalar
      nop
      nop
      nop
:1

:chunk_normal_loop_next_iteration

   mov r0, ra_chunk_normal_locals
   nop
   nop ; mov ra_chunk_normal_locals, r0 << 1 # move vertices down for the next iteration. tx/ty now garbage.
   sub.setf ra_chunk.8d, ra_chunk.8d, 1 # --explicit_n

   brr.allnz -, r:chunk_normal_loop_begin    # branch taken if new value of explicit_n != 0
      nop
      nop
      nop

:chunk_normal_loop_end

   mkp "chunk_normal: loop ended\n"
   mk 0x4030

   # `if (chunk->subpath_last) {...`
   mov.setf -, ra_chunk.8b # subpath last
   brr.allz -, r:chunk_normal_end
      nop
      nop
      nop
   mk 0x4031
   mov.setf -, ra_chunk.8c # close
   brr.allz -, r:1f  # branch taken if !close
      nop
      nop
      nop
   sub.setf -, elem_num, rs2i_close_x
   mov r0, CLOSE_X_NOT_IN_DASH
   sub.ifz.setf -, ra_scalar2, r0
   brr.anyz -, r:1f  # branch taken if floats_identical(stroke->close_x, CLOSE_X_NOT_IN_DASH)
      nop
      nop
      nop
   sub.setf -, elem_num, rs2i_dash_phase_i
   and.ifz.setf -, ra_scalar2, 0x1
   brr.allnz -, r:1f # branch taken if (stroke->dash_phase_i & 0x1)
      nop
      nop
      nop

   mk 0x4032
   # call add_join, with tx/y1=stroke->close_tx/y
   .assert rs2i_close_tx+5==14
   .assert rs2i_close_ty+5==15
   mov r0, ra_scalar2
   nop
   nop ; mov r1, r0>>5

   sub.setf -, elem_num, rsi_q_lr
   mov.ifz ra_scalar, a:chunk_normal_end
   brr -, r:add_join
      nop
      nop
      nop

:1 # ` } else { `

   mk 0x4033

   sub.setf -, elem_num, rs2i_close_x
   mov r0, CLOSE_X_NOT_IN_DASH
   sub.ifz.setf -, ra_scalar2, r0
   brr.anyz -, r:1f # branch taken if floats_identical(stroke->close_x, CLOSE_X_NOT_IN_DASH)
      nop
      nop
      nop

   # call add_cap, with x/y=stroke->close_x/y, tx/y=-stroke->close_tx/y
   mk 0x4034

   mov r1, ra_scalar2
   sub.setf -, elem_num, 7
   nop ; mov r0, r1<<rs2i_close_x
   nop ; mov.ifz r0, r1<<(rs2i_close_y-7)
   sub.setf -, elem_num, 14
   nop ; mov.ifnn r0, r1<<(rs2i_close_ty-15)
   nop ; mov.ifz r0, r1<<(rs2i_close_tx-14)
   fsub.ifnn r0, f(0.0), r0

   sub.setf -, elem_num, rsi_q_lr
   mov.ifz ra_scalar, a:1f
   brr -, r:add_cap
      nop
      nop
      nop
:1

   sub.setf -, elem_num, rs2i_dash_phase_i
   and.ifz.setf -, ra_scalar2, 0x1
   brr.allnz -, r:chunk_normal_end # branch taken if (stroke->dash_phase_i & 0x1)
      nop
      nop
      nop

   # call add_cap, with x/y=stroke->x/y, tx/y=stroke->tx/y
   mk 0x4035

   mov r0, ra_scalar2

   sub.setf -, elem_num, rsi_q_lr
   mov.ifz ra_scalar, a:chunk_normal_end
   brr -, r:add_cap
      nop
      nop
      nop

:chunk_normal_end

   # if we're not done, go to chunk_lr
   mov.setf  -, ra_scalar ; mov  r5rep, ra_scalar << rsi_chunk_lr
   sub.ifz.setf  -, elem_num, rsi_chunks_m_size ; mov  ra_temp, r5
   nop                    ; nop
   bra.allnz  -, ra_temp
      nop
      nop
      nop

   #TODO stuff from stroke_end()

   mkp "end.\n"
   mk 0xf000

   # flush queues

   sub.setf -, elem_num, rsi_q_lr
   mov.ifz ra_scalar, a:1f
   sub.setf  -, elem_num, rsi_clerp_q_n
   mov.ifz.setf  -, ra_scalar
   brr.allnz -, r:flush_clerp_q # branch taken if clerp_q_n!=0
      nop
      nop
      nop
:1

   sub.setf -, elem_num, rsi_q_lr
   mov.ifz ra_scalar, a:1f
   sub.setf  -, elem_num, rbsi_join_q_n
   mov.ifz.setf  -, rb_scalar
   mov r0, ra_scalar2
   nop
   nop ; mov ra_temp, (r0 << rs2i_flush_join_q_lbl) >> 15
   nop
   bra.allnz -, ra_temp # branch taken if join_q_n!=0
      nop
      nop
      nop
:1

   sub.setf -, elem_num, rsi_q_lr
   mov.ifz ra_scalar, a:1f
   sub.setf  -, elem_num, rbsi_cap_q_n
   mov.ifz.setf  -, rb_scalar
   mov r0, ra_scalar2
   nop
   nop ; mov ra_temp, (r0 << rs2i_flush_cap_q_lbl) >> 15
   nop
   bra.allnz -, ra_temp # branch taken if cap_q_n!=0
      nop
      nop
      nop
:1

   mkp "end: writing leading head\n"
   mk 0xf001

   # we need at least 64 bytes:
   # - 4 for the current list's head
   # - 60 for the rep
   mov.setf  -, neg_elems(elems(rsi_alloc_size))
   mov  r0, 64
   sub.ifn.setf  -, ra_scalar, r0 ; nop
   brr.anyn  -, r:alloc
   # branch delay slots {
   mov  ra_temp, a:1f # alloc will return here
   nop
   nop
   # }

   # we have at least 64 bytes at rsi_alloc_p. write out the head of the current
   # list
   mov  r0, ra_scalar
   mov  r1, -(15 << 16) # only 1 element
   add  r1, r0, r1 ; mov  vw_setup, r0 << rsi_out_vpm_setup
   sub  r0, r0, 4
   mov  -, vw_wait
   mov  vpm, TRI_LIST_HEAD
   nop ; mov  vw_setup, r1 << rsi_out_vdw_setup_0
   nop ; mov  vw_addr, r0 << rsi_alloc_p
   mov.setf  -, elems(rsi_alloc_p, rsi_alloc_size)
   add.ifnz  ra_scalar, ra_scalar, -16
:1

   # fill in the rest of the IO_CHUNK_O_T (r0) and write it out
   add.setf  -, elem_num, -1 # only elem 0 has N set, only elem 1 has Z set.
   mov  r0, -1 ; mov  r2, ra_scalar
   mov  r1, 48
   nop ; mov  r5rep, r2 << rsi_alloc_p
   sub.ifz  r0, r5, r1 # r0[1] := alloc_p - 48
   add.ifn  r0, r5, 12 # r0[0] := alloc_p + 12 (after the list tail we just alloced)
   # r0[0]: branch
   # r0[1]: rep (60 bytes of 4-byte aligned memory)
   # r0[2-14]: don't care
   # r0[15]: -1
   add  r2, r2, 1 # r2's out_vpm_setup is 1 row down
   mov  r3, 1 << 7 # 1 row down
   # no need to vw_wait here -- just waited for head write
   add  r2, ra_scalar, r3 ; mov  vw_setup, r2 << rsi_out_vpm_setup # r2's out_vdw_setup_0 is now 1 row down.
   nop                    ; mov  vpm, r0
   mov  r0, ra_scalar     ; mov  vw_setup, r2 << rsi_out_vdw_setup_0
   sub  r1, ra_scalar, r1 ; mov  vw_addr, unif

   # save adjusted (alloced another 48 bytes for rep) alloc p/size for next shader
   nop ; mov  vw_setup, r0 << rsi_chunks_vpm_setup
   .assert rsi_alloc_size == (rsi_alloc_p + 1)
   nop ; mov  vpm, r1 << rsi_alloc_p

   # wait for writes, prod host, then end
   empty_vpm_fifo
   mov  -, vw_wait
   mkp "end: thrending\n"
.ifnset VG_TESS_NO_INT
   mov  interrupt, 1
.endif
   thrend
   nop
   nop

:chunk_cubic
   mk 0x4100

   # ra_chunk.8b[0]: subpath_first
   # ra_chunk.8c[0]: subpath_last
   # r2: garbage, x0, y0, x1, y1, x2, y2, x3, y3

   mov  r5rep, ra_chunk
   mov  ra_chunk, r5 ; mov  rb_cubics[0], r2 << 1

   # stroke->is_cubic = true;
   sub.setf  -, elem_num, rs2i_is_cubic
   mov.ifz  ra_scalar2, 1

   # if (chunk->subpath_first) {
   mov.setf  -, ra_chunk.8b # subpath first
   brr.allz  -, r:1f
   # branch delay slots {
   nop
   nop
   nop
   # }

   # stroke->x = chunk->p[0];
   # stroke->y = chunk->p[1];
   # stroke->tx = 1.0f;
   # stroke->ty = 0.0f;
   sub.setf  -, elem_num, rs2i_x
   nop ; mov.ifz  ra_scalar2, (r2 << 1) >> rs2i_x
   sub.setf  -, elem_num, rs2i_y
   nop ; mov.ifz  ra_scalar2, (r2 << 2) >> rs2i_y
   sub.setf  -, elem_num, rs2i_tx
   mov.ifz  ra_scalar2, f(1.0)
   sub.setf  -, elem_num, rs2i_ty
   mov.ifz  ra_scalar2, f(0.0)

   # stroke->close_x = CLOSE_X_DEGENERATE;
   sub.setf  -, elem_num, rs2i_close_x
   mov.ifz  ra_scalar2, CLOSE_X_DEGENERATE

   # if (stroke->i2->dash_phase_reset) {
   mov  r0, ra_scalar3
   mov  r1, (1 << 14) # dash_phase_reset
   nop ; mov  r5rep, r0 << rs3i_bitfield
   and.setf  -, r5, r1
   brr.allz  -, r:1f
   # branch delay slots {
   nop
   nop
   nop
   # }

   # stroke->dash_phase = stroke->i2->dash_phase;
   sub.setf  -, elem_num, rs2i_dash_phase
   nop ; mov.ifz  ra_scalar2, (r0 << rs3i_initial_dash_phase) >> rs2i_dash_phase

   # stroke->dash_phase_i = stroke->i2->dash_phase_i;
   sub.setf  -, elem_num, rs2i_dash_phase_i
   shr  r5rep, r5, 10
   and.ifz  ra_scalar2, r5, 0xf

   # } /* if (stroke->i2->dash_phase_reset) */
   # } /* if (chunk->subpath_first) */
:1

   # if (!chunk->subpath_last &&
   #    (chunk->p[0] == chunk->p[2]) && (chunk->p[2] == chunk->p[4]) && (chunk->p[4] == chunk->p[6]) &&
   #    (chunk->p[1] == chunk->p[3]) && (chunk->p[3] == chunk->p[5]) && (chunk->p[5] == chunk->p[7])) {
   #    /* skip degenerate segments so the initial tangent will be that of the
   #     * first non-degenerate segment, rather than (1, 0). don't skip the last
   #     * segment though, we need to do all the subpath_last stuff */
   #    return;
   # }
   nop                          ; mov  r0, r2 << 2
   fsub.setf  -, r2, r0         ; mov  r5rep, ra_scalar << rsi_chunk_lr
   mov.ifz.setf  -, ra_chunk.8c ; mov  ra_temp, r5
   mov.ifnz.setf  -, [0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0]
   bra.allz  -, ra_temp
   # branch delay slots {
   nop
   nop
   nop
   # }

   # if (!get_cubic_t(t0, chunk->p, true)) {
   #    t0[0] = stroke->tx;
   #    t0[1] = stroke->ty;
   # }
   # if (!get_cubic_t(t1, chunk->p, false)) {
   #    t1[0] = t0[0];
   #    t1[1] = t0[1];
   # }
   nop                       ; mov  r2, r2 << 7
   # r2: x3, y3, garbage[8], x0, y0, x1, y1, x2, y2
   mov.setf  -, [1, 1, 0, 0, 0, 0, 0, 0, -1, -1, 1, 1, 1, 1, 1, 1]
   mov  r0, r2               ; mov.ifn  r2, r2 << 2         @mul_used(8, 9)
   # r0: x3, y3, garbage[8], x0, y0, x1, y1, x2, y2
   # r2: x3, y3, garbage[6], x0, y0, x0, y0, x1, y1, x2, y2
   nop                       ; mov.ifn  r0, r0 >> 8         @mul_used(8, 9)
   # r0: x3, y3, garbage[6], x3, y3, x0, y0, x1, y1, x2, y2
   nop                       ; mov.ifz  r2, r2 << 4
   # r2: x3, y3, garbage[2], x0, y0, x0, y0, x0, y0, x0, y0, x1, y1, x2, y2
   nop                       ; mov.ifz  r0, r0 << 2
   # r0: x3, y3, garbage[4], x3, y3, x3, y3, x0, y0, x1, y1, x2, y2
   mov.ifnz  ra_temp, r0     ; mov.ifz  r2, r2 << 2
   # r2: x3, y3, x0, y0, x0, y0, x0, y0, x0, y0, x0, y0, x1, y1, x2, y2
   # ra_temp: x3, y3, garbage[6], x3, y3, x0, y0, x1, y1, x2, y2
   nop                       ; mov.ifz  ra_temp, r0 << 2
   # ra_temp: x3, y3, garbage[2], x3, y3, x3, y3, x3, y3, x0, y0, x1, y1, x2, y2
   mov  r0, ra_scalar2       ; mov  r2, r2 << 8
   # r2: x0, y0, x0, y0, x1, y1, x2, y2, x3, y3, x0, y0, x0, y0, x0, y0
   .pushnwarn # non-accum rotation warning: we want the weird semantics here!
   nop                       ; mov  r1, ra_temp << 2
   .popnwarn
   # r1: garbage[2], x3, y3, x3, y3, x3, y3, x0, y0, x3, y3, x2, y2, x1, y1
   .assert rs2i_ty == (rs2i_tx + 1)
   fsub.ifnn  r1, r1, r2     ; mov.ifn  r1, (r0 << rs2i_tx) >> 8
   # r1: garbage[2], (3 - 0)[2], (3 - 1)[2], (3 - 2)[2], prev_t[2], (3 - 0)[2], (2 - 0)[2], (1 - 0)[2]
   mov.setf  -, [-1, -1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0]
   nop                       ; fmul  r0, r1, r1
   # r0: garbage[2], (3 - 0)^2[2], (3 - 1)^2[2], (3 - 2)^2[2], prev_t[2], (3 - 0)^2[2], (2 - 0)^2[2], (1 - 0)^2[2]
   nop                       ; fmul  r2 >> 1, r1, r1
   # r2: garbage, (3 - 0)^2[2], (3 - 1)^2[2], (3 - 2)^2[2], prev_t[2], (3 - 0)^2[2], (2 - 0)^2[2], (1 - 0)^2[2], garbage
   fadd.ifnz  r0, r0, r2     ; mov  r2, r0 >> 1
   # r0: garbage[2], |3 - 0|^2, garbage, |3 - 1|^2, garbage, |3 - 2|^2, garbage, |prev_t|^2, garbage, |3 - 0|^2, garbage, |2 - 0|^2, garbage, |1 - 0|^2, garbage
   # r2: garbage[3], (3 - 0)^2[2], (3 - 1)^2[2], (3 - 2)^2[2], prev_t[2], (3 - 0)^2[2], (2 - 0)^2[2], (1 - 0)^2[2]
   fadd.ifz  r0, r0, r2      ; mov.ifn  r1, r1 << 8
   # r1: prev_t[2], (3 - 0)[2], (3 - 1)[2], (3 - 2)[2], prev_t[2], (3 - 0)[2], (2 - 0)[2], (1 - 0)[2]
   # r0: garbage[2], |3 - 0|^2[2], |3 - 1|^2[2], |3 - 2|^2[2], |prev_t|^2[2], |3 - 0|^2[2], |2 - 0|^2[2], |1 - 0|^2[2]
   mov  rsqrt, r0            ; nop
   # r4: garbage[2], 1/|3 - 0|[2], 1/|3 - 1|[2], 1/|3 - 2|[2], 1/|prev_t|[2], 1/|3 - 0|[2], 1/|2 - 0|[2], 1/|1 - 0|[2]
   fsub.setf  -, r0, rb_eps  ; nop
   nop                       ; mov.ifn  r1, r1 >> 2
   mov  r0, 14               ; fmul.ifnn  r1, r1, r4
   nop                       ; mov.ifn  r1, r1 >> 2         @mul_used(4, 5, 6, 7, 12, 13, 14, 15)
   sub.setf  -, elem_num, r0 ; mov.ifn  r1, r1 >> 2         @mul_used(6, 7, 14, 15)
   # r1: garbage[6], t1[2], garbage[6], t0[2]
   nop                       ; mov.ifnn  ra_chunk, r1 >> 8  @mul_used(14, 15)
   nop                       ; mov  ra_cubics[0], r1 << 4

   # if (floats_identical(stroke->close_x, CLOSE_X_DEGENERATE)) {
   sub.setf  -, elem_num, rs2i_close_x
   mov  r0, CLOSE_X_DEGENERATE
   sub.ifz.setf  -, ra_scalar2, r0 ; mov  r0, ra_scalar2
   brr.allnz  -, r:1f
   # branch delay slots {
   sub.setf  -, elem_num, rs2i_dash_phase_i
   and.ifz.setf  -, r0, 0x1
   nop
   # }
   # if (stroke->dash_phase_i & 0x1) {
   #    stroke->close_x = CLOSE_X_NOT_IN_DASH;
   # } else {
   #    stroke->close_x = stroke->x; /* chunk->p[0/1] would also be fine */
   #    stroke->close_y = stroke->y;
   #    stroke->close_tx = t0[0];
   #    stroke->close_ty = t0[1];
   # }
   brr.allnz  -, r:2f
   # branch delay slots {
   sub.setf  -, elem_num, rs2i_close_x
   mov.ifz  ra_scalar2, CLOSE_X_NOT_IN_DASH
   nop
   # }
   nop ; mov.ifz  ra_scalar2, (r0 << rs2i_x) >> rs2i_close_x
   sub.setf  -, elem_num, rs2i_close_y
   brr  -, r:2f
   # branch delay slots {
   nop ; mov.ifz  ra_scalar2, (r0 << rs2i_y) >> rs2i_close_y
   mov.setf  -, elems(rs2i_close_tx, rs2i_close_ty)
   .assert rs2i_close_ty == (rs2i_close_tx + 1)
   nop ; mov.ifnz  ra_scalar2, (r1 << 14) >> rs2i_close_tx
   # }
:1
   # } else { /* if (floats_identical(stroke->close_x, CLOSE_X_DEGENERATE)) */
   # r1: garbage[6], t1[2], garbage[6], t0[2]
   brr.anyz  ra_temp, r:add_join
   # branch delay slots {
   sub.setf  -, elem_num, rsi_q_lr
   mov.ifz  ra_scalar, ra_temp
   nop
   # }
   # } /* if (floats_identical(stroke->close_x, CLOSE_X_DEGENERATE)) */
:2

   # stroke->tx = t0[0];
   # stroke->ty = t0[1];
   mov  r0, ra_cubics[0]
   mov.setf  -, elems(rs2i_tx, rs2i_ty)
   .assert rs2i_ty == (rs2i_tx + 1)
   nop ; mov.ifnz  ra_scalar2, (r0 << 10) >> rs2i_tx

   sub.setf  -, elem_num, 8

   # a = -3p0 + 9p1 - 9p2 + 3p3
   # b = 6p0 - 12p1 + 6p2
   # c = -3p0 + 3p1

   #  2   3   0   0  -1  -4   0
   #  p0  p1  p2  p3  p0  p1  p2
   #  +   +   +   +   +   +   +
   #  0  -1   1   2   1   2   1
   #  p3  p0  p1  p2  p3  p0  p1
   #  +   +   +   +   +   +   +
   #  2   1  -1  -4  -3   0  -1
   #  p2  p3  p0  p1  p2  p3  p0
   #  +   +   +   +   +   +   +
   # -4  -3   0   2   3   2   0
   #  p1  p2  p3  p0  p1  p2  p3
   #  =   =   =   =   =   =   =
   #  b/3 a/3 c/3 b/3 a/3 b/3 c/3

   mov  r1, [0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0]
   mov  r2, [2, 2, 3, 3, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 0]
   xor  r1, r1, r2       ; mov  r0, rb_cubics[0]
   # r0: x0, y0, x1, y1, x2, y2, x3, y3
   # r1: 2, 3, 0, 0, -1, -4, 0 (doubled up)
   mov  r2, [1, 1, -1, -1, -2, -2, -1, -1, -2, -2, -1, -1, 0, 0, 0, 0]
   # r2: 0, 1, -1, -2, -1, -2, -1 (doubled up, << 2)
   itof  ra_temp, r1     ; mov.ifnn  r0, r0 << 8
   # r0: x0, y0, x1, y1, x2, y2, x3, y3 (twice)
   itof  r1, r2          ; mov  rb_cubics[0], r0
   mov  r2, [-1, -1, -2, -2, -2, -2, 0, 0, -1, -1, 0, 0, 1, 1, 1, 1]
   mov  r3, [0, 0, -2, -2, -1, -1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0]
   add  r2, r2, r3       ; fmul  r1 << 2, r1, r0
   # r2: 2, 1, -1, -4, -3, 0, -1 (doubled up, << 4)
   itof  r2, r2          ; nop
   mov  ra_temp2, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -2, -2, 0, 0]
   mov  r3, [2, 2, 3, 3, 2, 2, 0, 0, 0, 0, 3, 3, 3, 3, 0, 0]
   xor  r3, r3, ra_temp2 ; fmul  r2 << 4, r2, r0
   # r3: -4, -3, 0, 2, 3, 2, 0 (doubled up, << 6)
   itof  r3, r3          ; nop
   fsub  r1, r2, r1      ; fmul  r2, ra_temp, r0
   fadd  r1, r1, r2      ; fmul  r3 << 6, r3, r0
   fadd  r1, r1, r3      ; nop
   # r1: bx/3, by/3, ax/3, ay/3, cx/3, cy/3, bx/3, by/3, ax/3, ay/3, bx/3, by/3, cx/3, cy/3, garbage[2]

   nop                   ; mov  ra_temp, r1 >> 1  @mul_used(2, 4, 6)
   nop                   ; mov  r2, r1 >> 3
   nop                   ; fmul  ra_temp, r1, ra_temp
   # ra_temp: garbage[2], (ax * by)/9, garbage, (cx * ay)/9, garbage, (bx * cy)/9, garbage[9]
   mov  r3, [0, 0, 1, 0, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0]
   itof  r3, r3          ; fmul  r2 >> 1, r1, r2
   # r2: garbage[2], (ay * bx)/9, garbage, (cy * ax)/9, garbage, (by * cx)/9, garbage[9]
   fsub  r2, r2, ra_temp ; mov  r5rep, f(4.0)
   # r2: garbage[2], ((ay * bx) - (ax * by))/9, garbage, ((cy * ax) - (cx * ay))/9, garbage, ((by * cx) - (bx * cy))/9, garbage[9]
   mov  r3, f(1.0)       ; fmul.ifn  r1, r2, r3
   # r1: garbage[2], ((ay * bx) - (ax * by))/9, garbage, 2 * ((cx * ay) - (cy * ax))/9, garbage, ((by * cx) - (bx * cy))/9, garbage, ax/3, ay/3, bx/3, by/3, cx/3, cy/3, garbage[2]
   # r3: 1
   # r5: 4

   # http://en.wikipedia.org/wiki/Quadratic_equation#Floating_point_implementation
   #
   # at^2 + bt + c = 0
   # u = -1/2 * (b + (sign(b) * sqrt(b^2 - 4ac)))
   # t0 = c / u
   # t1 = u / a

   # r1: a, garbage, b, garbage, c (sort of)
   mov.setf  recip, r1 ; fmul  r0, r1, r1
   # flags[2]: n if b < 0
   # r0: garbage[2], b^2
   # r4: 1/a
   mov  r5rep, f(0.0)  ; fmul  r2 >> 4, r1, r5
   # r2: 4c
   fsub  r5rep, r5, r3 ; fmul  r2 << 2, r1, r2  @mul_unused(0, 1)
   # r2: garbage[2], 4ac
   # r5: -1
   fsub  rsqrt, r0, r2 ; fmul  r3, r4, f(0.5)
   # r3: 1/2a
   # r4: garbage[2], 1/sqrt(b^2 - 4ac)
   fsub  r0, r0, r2    ; mov  r3, r3 >> 2       @mul_unused(0, 1)
   # r0: garbage[2], b^2 - 4ac
   # r3: garbage[2], 1/2a
   mov.setf  -, r0     ; fmul.ifnn  r0, r5, r0
   # flags[2]: n if (b^2 - 4ac) < 0
   # r0: garbage[2], -sign(b) * (b^2 - 4ac)
   nop                 ; fmul  r0, r0, r4
   # r0: garbage[2], -sign(b) * sqrt(b^2 - 4ac)
   fsub  recip, r0, r1 ; mov  r2, r1 << 2
   # r2: garbage[2], c
   # r4: garbage[2], 1/(-b - (sign(b) * sqrt(b^2 - 4ac)))
   fsub  r0, r0, r1    ; fmul  r2, r2, f(2.0)
   # r0: garbage[2], -b - (sign(b) * sqrt(b^2 - 4ac))
   # r2: garbage[2], 2c
   mov.ifn  r3, r5     ; fmul.ifnn  r3, r0, r3
   mov.ifn  r2, r5     ; fmul.ifnn  r2, r2, r4

   # r2: garbage[4], inflection t0, garbage[5], x zv t0, y zv t0, garbage[4]
   # r3: garbage[4], inflection t1, garbage[5], x zv t1, y zv t1, garbage[4]
   # r5: -1

   fadd  r0, r2, r3                  ; fmul  r5rep, r5, f(0.5)
   # r0: garbage[10], (x zv t0) + (x zv t1), (y zv t0) + (y zv t1)
   # r5: -0.5
   fsub.setf  -, r3, f(1.0)          ; mov  r1, f(0.0)
   # flags: n if t1 < 1
   # r1: 0
   fsub.ifn.setf  r3, r1, r3         ; fmul  r0, r0, r5
   # flags: n if (t1 > 0) and (t1 < 1)
   # r0: garbage[10], -((x zv t0) + (x zv t1))/2, -((y zv t0) + (y zv t1))/2
   # r3: garbage[4], -t1 (>= 0 if bad)
   fsub.setf  -, r2, f(1.0)          ; mov.ifnn  r0, f(1.0)
   # flags: n if t0 < 1
   # r0: garbage[10], -((x zv t0) + (x zv t1))/2 (1 if bad x zv t1), -((y zv t0) + (y zv t1))/2 (1 if bad y zv t1)
   fsub.ifn.setf  r2, r1, r2         ; mov  r1, ra_cubics[0] << 1  @mul_used(14)
   # flags: n if (t0 > 0) and (t0 < 1)
   # r1: garbage[10], initial tang y
   # r2: garbage[4], -t0 (>= 0 if bad)
   mov.ifnn  r2, r3                  ; mov.ifnn  r3, r2 # swap t0/t1 if t0 bad
   fmaxabs.setf  -, ra_cubics[0], r1 ; mov.ifnn  r0, f(1.0)
   # flags[10]: c if |initial tang x| > |initial tang y|
   # r0: garbage[10], -((x zv t0) + (x zv t1))/2 (>= 0 if bad), -((y zv t0) + (y zv t1))/2 (>= 0 if bad)
   nop                               ; mov.ifnc  r0, r0 << 1       @mul_used(14)
   # r0: garbage[10], -t_zv (>= 0 if bad)
   nop                               ; mov.setf  r5rep, r2 << 4
   mov  r1, ra_cubics[0]             ; mov.ifnn  r5rep, r0 << 10

   # r1: garbage[2], t1[2], garbage[6], t0[2], garbage[4]
   # r3: garbage[4], -t1 (>= 0 if bad)
   # r5: -t0 (>= 0 if bad)

   # save final p/t in ra_chunk so we can add final clerp in chunk_cubic_tail
   # (final t actually already in)
   # if t0 bad, do the whole cubic at once
   mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
   mov  r0, rb_cubics[0] ; mov.ifz  ra_chunk, rb_cubics[0]
   nop                   ; nop
   mov.setf  -, r5       ; mov.ifn  ra_chunk, r0 << 6
   brr.allnn  -, r:cubic_subd_0
   # branch delay slots {
   mov.setf  -, [1, 1, 1, 1, -1, -1, -1, -1, 0, 0, 1, 1, 1, 1, 0, 0]
   fsub.ifn  r1, r3, r5  ; mov.ifz  r1, r1 >> 12
   # r1: garbage[4], t0 - t1, garbage[5], tx0, ty0, garbage[2], tx1, ty1
   mov  ra_cubics[0], a:chunk_cubic_tail
   # }

   # t0 good. split cubic at t0...

   # p0' = (1 - t)p0 + (t)p1
   # p1' = (1 - t)p1 + (t)p2
   # p2' = (1 - t)p2 + (t)p3
   # p0'' = (1 - t)p0' + (t)p1'
   # p1'' = (1 - t)p1' + (t)p2'
   # p0''' = (1 - t)p0'' + (t)p1''
   #
   # q0 = p0
   # q1 = p0'
   # q2 = p0''
   # q3 = p0'''
   #
   # r0 = p0'''
   # r1 = p1''
   # r2 = p2'
   # r3 = p3

   fadd  recip, f(1.0), r5         ; mov  ra_cubics[0], r1
:chunk_cubic_split # we branch back here to do the t1 split of the remaining half
   # flags:
   # - - - - - - - - z z - - - - z z
   # - - - - n n n n - - - - - - - -
   # r0: p0, p1, p2, p3 (twice)
   # r4: 1/(1 - t)
   # r5: -t
   # ra_cubics[0]: garbage[4], t0 - t1, garbage[5], tx0, ty0, garbage[2], tx1, ty1
   # rb_cubics[0]: p0, p1, p2, p3 (twice)
   fadd  r1, f(1.0), r5            ; fmul  r2, r5, r0
   # r1: 1 - t
   # r2: garbage, (-t)p1, (-t)p2, (-t)p3 (twice)
   nop                             ; fmul  r3 << 2, r1, r0           @mul_unused(0, 1, 8, 9)
   # r3: garbage, (1 - t)p0, (1 - t)p1, (1 - t)p2 (twice)
   fsub  r2, r3, r2                ; fmul.ifn  ra_cubics[0], ra_cubics[0], r4
   # r2: garbage, p0', p1', p2' (twice)
   # ra_cubics[0]: garbage[4], (t0 - t1)/(1 - t0), garbage[5], tx0, ty0, garbage[2], tx1, ty1
   nop                             ; fmul  ra_temp, r5, r2
   # ra_temp: garbage[2], (-t)p1', (-t)p2' (twice)
   mov  ra_temp2, r2               ; fmul  r3 << 2, r1, r2
   # r3: garbage[2], (1 - t)p0', (1 - t)p1' (twice)
   # ra_temp2: garbage, p0', p1', p2' (twice)
   fsub  r2, r3, ra_temp           ; mov  r3, r2 << 4
   # r2: garbage[2], p0'', p1'' (twice)
   # r3: garbage[5], p2'
   mov  rb_temp1, r2               ; fmul  ra_temp, r5, r2
   # ra_temp: garbage[3], (-t)p1'' (twice)
   # rb_temp1: garbage[2], p0'', p1'' (twice)
   mov.ifnn  r2, ra_temp2          ; fmul  r1 << 2, r1, r2
   # r1: garbage[3], (1 - t)p0'' (twice)
   # r2: garbage[2], p0'', p1'', garbage, p0'
   fsub  ra_temp, r1, ra_temp      ; mov.ifn  r3, r2 << 2            @mul_used(4, 5)
   # r3: garbage[2], p1'', garbage[2], p2'
   # ra_temp: garbage[3], p0''' (twice)
   fsub.ifnz  r1, r3, r2           ; mov.ifz  r1, ra_cubics[0] << 2  @mul_used(8, 9)
   # r1: garbage[2], (p1'' - p0''), garbage, t0, (p2' - p0')
   and.setf  -, elem_num, 1        ; fmul  r2, r1, r1
   # r2: garbage[4], (p1''x - p0''x)^2, garbage[3], t0x^2, garbage, (p2'x - p0'x)^2
   nop                             ; fmul  r3 >> 1, r1, r1
   # r3: garbage[4], (p1''y - p0''y)^2, garbage[3], t0y^2, garbage, (p2'y - p0'y)^2
   fadd  r2, r2, r3                ; nop
   # r2: garbage[4], |p1'' - p0''|^2, garbage[3], |t0|^2, garbage, |p2' - p0'|^2
   nop                             ; mov.ifnz  r2, r2 >> 1           @mul_unused(0)
   # r2: garbage[4], |p1'' - p0''|^2[2], garbage[2], |t0|^2[2], |p2' - p0'|^2[2]
   mov  rsqrt, r2                  ; mov  r5rep, -1
   # r4: garbage[4], 1/|p1'' - p0''|[2], garbage[2], 1/|t0|[2], 1/|p2' - p0'|[2]
   fsub.setf  -, r2, rb_eps        ; nop
   nop                             ; mov.ifn  r1, r1 << 4
   mov  r2, rb_temp1               ; fmul.ifnn  r1, r1, r4
   max.setf  -, r5, ra_subd_flags1 ; mov.ifn  r1, r1 >> 6            @mul_used(10, 11)
   mov  r3, ra_temp                ; mov  r1, r1 >> 4                @mul_used(14, 15)

   # flags:
   # - - - - z z - - (twice)
   # n n n n - - - - (twice)
   # c c - - - - - - (twice)
   # r0: x0, y0, x1, y1, x2, y2, x3, y3 (twice)
   # r1: garbage[14], tx, ty
   # r2: garbage[2], p0'', p1'' (twice)
   # r3: garbage[3], p0''' (twice)
   # ra_temp2: garbage, p0', p1', p2' (twice)

   mov.ifnc  r0, ra_temp2    ; mov.ifz  rb_cubics[0], ra_temp2 << 2  @mul_used(4, 5, 12, 13)
   brr  ra_cubics[1], r:cubic_subd_1
   # branch delay slots {
   mov.ifnn  r0, r3          ; mov.ifn  rb_cubics[0], r2 << 4
   mov.ifz  r0, r2           ; mov.ifc  rb_cubics[0], r3 << 6
   # r0: p0, p0', p0'', p0''' (twice)
   # rb_cubics[0]: p0''', p1'', p2', p3 (twice)
   # ra_cubics[0]: garbage[4], (t0 - t1)/(1 - t0), garbage[5], tx0, ty0, garbage[2], tx1, ty1
   mov.ifn  r1, ra_cubics[0] ; mov.ifn  ra_cubics[0], r1 << 4
   # r1: garbage[10], tx0, ty0, garbage[2], tx, ty
   # ra_cubics[0]: garbage[4], (t0 - t1)/(1 - t0), garbage[5], tx, ty, garbage[2], tx1, ty1
   # }

   # ra_cubics[0]: garbage[4], (t0 - t1)/(1 - t0), garbage[5], tx, ty, garbage[2], tx1, ty1
   # rb_cubics[0]: p0''', p1'', p2', p3 (twice)

   mov  r0, rb_cubics[0]        ; mov  r1, ra_cubics[0]
   mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
   mov.ifn  r1, r0              ; mov.ifnn  r1, r1 >> 4
   bra  ra_temp, (ra_scalar << rsi_dash_clerp_lbl) >> 15
   # branch delay slots {
   nop                          ; mov.ifz  r1, r0 >> 6
   # r1: p0'''x, garbage[6], p0'''y, garbage[6], tx, ty
   sub.setf  -, elem_num, rs2i_dash_clerp_lr ; nop
   mov.ifz  ra_scalar2, ra_temp ; nop
   # }

   # ra_cubics[0]: garbage[4], (t0 - t1)/(1 - t0), garbage[5], tx, ty, garbage[2], tx1, ty1
   # rb_cubics[0]: p0''', p1'', p2', p3 (twice)

   # if (t1 - t0)/(1 - t0) bad, do remaining half of the cubic at once
   mov.setf  -, [0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
   mov.ifn.setf  r1, ra_cubics[0] ; mov  r0, rb_cubics[0]
   brr.allnn  -, r:cubic_subd_1
   # branch delay slots {
   mov  r1, ra_cubics[0]          ; mov  r5rep, r1 << 4
   mov  ra_cubics[1], a:chunk_cubic_tail
   mov.setf  -, [1, 1, 1, 1, -1, -1, -1, -1, 0, 0, 1, 1, 1, 1, 0, 0]
   # }

   # (t1 - t0)/(1 - t0) good. split at...
   # but instead of duplicating a bunch of code, just branch back
   brr  -, r:chunk_cubic_split
   # branch delay slots {
   # need to make sure all registers etc have the right values...
   # flags: ok (just set)
   # r0: ok (just fetched from rb_cubics[0])
   # r4: need to set
   # r5: -t, ok (just fetched from ra_cubics[0][4])
   # ra_cubics[0]: almost ok, need to set [4] to 0
   # rb_cubics[0]: ok
   mov  recip, f(1.0) ; mov.ifn  ra_cubics[0], 0 # as long as r4 isn't nan or inf, multiplying by 0 will give 0
   nop                ; nop
   nop                ; nop
   # }

:chunk_cubic_tail

   # ra_chunk: x3, subpath_first/subpath_last, garbage[5], y3, garbage[6], t1x, t1y

   bra  ra_temp, (ra_scalar << rsi_dash_clerp_lbl) >> 15
   # branch delay slots {
   mov  r1, ra_chunk            ; nop
   sub.setf  -, elem_num, rs2i_dash_clerp_lr ; nop
   mov.ifz  ra_scalar2, ra_temp ; nop
   # }

   nop                          ; mov  r5rep, ra_scalar << rsi_chunk_lr
   sub.setf  -, elem_num, 1     ; mov  ra_temp, r5
   mov.ifz.setf  -, ra_chunk.8c ; nop
   bra.anyz  -, ra_temp
   # branch delay slots {
   sub.setf  -, elem_num, rsi_q_lr ; nop
   mov.ifz  ra_scalar, a:1f
   sub.setf  -, elem_num, rs2i_close_x ; nop
   # }

   # if (!floats_identical(stroke->close_x, CLOSE_X_NOT_IN_DASH)) {
   #    stroke_cap(stroke, stroke->close_x, stroke->close_y, -stroke->close_tx, -stroke->close_ty);
   # }
   mov  r0, CLOSE_X_NOT_IN_DASH
   sub.ifz.setf  -, ra_scalar2, r0 ; mov  r1, ra_scalar2
   .assert rs2i_close_ty == (rs2i_close_tx + 1)
   mov  r2, f(0.0)                 ; mov  r0, (r1 << rs2i_close_tx) >> 14  @mul_used(14, 15)
   brr.allnz  -, r:add_cap
   # branch delay slots {
   mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
   fsub.ifnn  r0, r2, r0           ; mov.ifn  r0, r1 << rs2i_close_x
   nop                             ; mov.ifz  r0, (r1 << rs2i_close_y) >> 7
   # }
:1

   # if (!(stroke->dash_phase_i & 0x1)) {
   #    /* chunk->p[6/7], t1[0/1] would be fine in place of stroke->x/y, stroke->tx/ty */
   #    stroke_cap(stroke, stroke->x, stroke->y, stroke->tx, stroke->ty);
   # }
   sub.setf  -, elem_num, rs2i_dash_phase_i ; nop
   and.ifz.setf  -, ra_scalar2, 0x1 ; nop
   brr.anyz  ra_temp, r:add_cap
   # branch delay slots {
   sub.setf  -, elem_num, rsi_q_lr ; nop
   mov.ifz  ra_scalar, ra_temp ; nop
   .assert rs2i_x == 0
   .assert rs2i_y == 7
   .assert rs2i_tx == 14
   .assert rs2i_ty == 15
   mov  r0, ra_scalar2 ; nop
   # }

   nop ; mov  r5rep, ra_scalar << rsi_chunk_lr
   mov  ra_temp, r5
   nop
   bra  -, ra_temp
   # branch delay slots {
   nop
   nop
   nop
   # }

.rep i, LEVEL_MAX
.if i == 0
:cubic_subd_0
.elif i == 1
:cubic_subd_1
.endif

   # we do three things here:
   # - split the cubic in half
   # - find the tangent at the split point (normalise((p3 - p0) + (p2 - p1)),
   #   falling back to t0 if that's unreliable)
   # - decide if we're flat enough. this involves two separate tests:
   #   - the tangents are close enough
   #   - the perpendicular distance of the control points from the line is <=
   #     flatness (this is almost the same as the test for fills)
   # see vg_tess_4.c for more details

   # a = p3 - p0
   # b = p1 - p0
   # c = p2 - p3
   # d = |a|
   # e = p2 - p1

   # r0: x0, y0, x1, y1, x2, y2, x3, y3 (twice)
   # r1: garbage[10], tx0, ty0, garbage[2], tx1, ty1
   mov.setf  -, ra_subd_flags2   ; mov  r2, r0 >> 2
   # flags:
   # z z z z z z z z z z - - - - - -
   # - - - - - - - - - - - n - - - -
   # r2: x3, y3, x0, y0, x1, y1, x2, y2 (twice)
   fsub.ifz  r1, r2, r0          ; mov  rb_cubics[i], r0
   # r1: ax, ay, -bx, -by, -ex, -ey, cx, cy, ax, ay, tx0, ty0, garbage[2], tx1, ty1
   fadd  r0, r0, r2              ; fmul  ra_temp3, r1, r1
   # r0: garbage[2], x0 + x1, y0 + y1, x1 + x2, y1 + y2, x2 + x3, y2 + y3 (twice)
   # ra_temp3: ax^2, ay^2
   mov  ra_temp, r0              ; mov  r2, r1 << 4
   # r2: -ex, -ey, garbage[8], tx1, ty1, garbage[4]
   # ra_temp: garbage[2], x0 + x1, y0 + y1, x1 + x2, y1 + y2, x2 + x3, y2 + y3 (twice)
   fsub  ra_temp4, r1, r2        ; mov  r3, r0 >> 2
   # r3: garbage[4], x0 + x1, y0 + y1, x1 + x2, y1 + y2 (twice)
   # ra_temp4: ax + ex, ay + ey
   fadd  ra_temp2, r0, r3        ; mov.ifz  r2, r1 << 1
   # r2: garbage, -bx, garbage[5], ax, garbage[2], tx1, ty1, garbage[4]
   # ra_temp2: garbage[4], x0 + 2x1 + x2, y0 + 2y1 + y2, x1 + 2x2 + x3, y1 + 2y2 + y3 (twice)
   fsub.ifnz  r2, f(0.0), r2     ; fmul  r0, r1, r2
   # r0: garbage, ay * -bx, garbage[5], ax * cy, garbage[3], ty0 * ty1, garbage[4]
   # r2: garbage[10], -tx1, -ty1, garbage[4]
   .assert rsi_m_cos_theta == 11
   mov  r3, ra_scalar            ; mov.ifz  r2, r1 << 3
   # r2: -by, garbage[5], ay, garbage[3], -tx1, -ty1, garbage[4]
   # r3: garbage[11], -cos theta
   nop                           ; mov  r5rep, ra_temp3 << 1
   # r5: ay^2
   fadd  r5rep, r5, ra_temp3     ; fmul  r2 << 1, r1, r2
   # r2: garbage, ax * -by, garbage[5], ay * cx, garbage[3], tx0 * -tx1, garbage[4]
   # r5: ax^2 + ay^2 (= d^2)
   fsub  r0, r0, r2              ; fmul.ifz  r1, ra_temp4, ra_temp4
   # r0: garbage, (ax * by) - (ay * bx), garbage[5], (ax * cy) - (ay * cx), garbage[3], (tx0 * tx1) + (ty0 * ty1), garbage[4]
   # r1: (ax + ex)^2, garbage[9], tx0, ty0, garbage[2], tx1, ty1
   mov.ifnn  r3, r5              ; fmul.ifz  rb_temp >> 1, ra_temp4, ra_temp4  @mul_used(0)
   # r3: d^2[11], -cos theta, d^2[4]
   # rb_temp: (ay + ey)^2
   fsub  rb_temp2, ra_subd_consts, r5 ; fmul  r2, r0, r0
   # r2: garbage, ((ax * by) - (ay * bx))^2, garbage[5], ((ax * cy) - (ay * cx))^2, garbage[3], ((tx0 * tx1) + (ty0 * ty1))^2, garbage[4]
   # rb_temp2[11]: (line_width/2)^2 - d^2
   fadd  r5rep, r1, rb_temp      ; fmul  ra_temp3, r2, ra_subd_consts
   # r5: (ax + ex)^2 + (ay + ey)^2
   # ra_temp3: garbage, -((ax * by) - (ay * bx))^2/flatness^2, garbage[5], -((ax * cy) - (ay * cx))^2/flatness^2, garbage[3], ((tx0 * tx1) + (ty0 * ty1))^2(line_width/2)^2, garbage[4]
   mov  rsqrt, r5                ; mov  r2, ra_temp2 >> 2                      @mul_used(6, 7, 14, 15)
   # r2: garbage[6], x0 + 2x1 + x2, y0 + 2y1 + y2 (twice)
   # r4: 1/sqrt((ax + ex)^2 + (ay + ey)^2)
   fsub.ifn.setf  -, ra_temp3, rb_temp2 ; mov.ifn  ra_temp3, r0
   # flags[11]: n if inter. all other flags !n
   # ra_temp3: garbage, -((ax * by) - (ay * bx))^2/flatness^2, garbage[5], -((ax * cy) - (ay * cx))^2/flatness^2, garbage[3], (tx0 * tx1) + (ty0 * ty1), garbage[4]
   .assert rbsi_m_cos_theta_inter == 11
   fadd  r0, r2, ra_temp2        ; mov.ifn  r3, rb_scalar
   # r0: garbage[6], x0 + 3x1 + 3x2 + x3, y0 + 3y1 + 3y2 + y3 (twice)
   # r3: d^2[11], -cos theta (inter), d^2[4]
   fadd.setf  -, r3, ra_temp3    ; mov  r3, r4
   mov.ifn.setf  -, [0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0]
   bra.allnn  -, ra_cubics[i] # if already flat enough, just return
   # branch delay slots {
   fsub.setf  -, r5, rb_eps      ; mov  r2, ra_temp4
   # flags: n if tangent bad
   # r2: ax + ex, ay + ey
   mov.ifn  r2, r1               ; fmul.ifnn  r2 << 10, r2, r3                 @mul_used(10, 11)
   # r2: garbage[10], tx, ty

.if i == (LEVEL_MAX - 1)
   # don't subdivide any further. just add clerp to middle if not flat enough
   mov  r2, ra_cubics[i] ; mov  r1, r2 >> 4  @mul_used(14, 15)
   # }
   sub.setf  -, elem_num, rs2i_dash_clerp_lr ; nop
   nop                   ; mov.ifz  ra_scalar2, (r2 << 15) >> rs2i_dash_clerp_lr
   bra  -, (ra_scalar << rsi_dash_clerp_lbl) >> 15
   # branch delay slots {
   mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
   mov  r5rep, f(0.125)  ; fmul.ifz  r1, r0, f(0.125)
   nop                   ; fmul.ifn  r1 >> 6, r0, r5
   # }
.else
   # finish split then subdivide first half
   max.setf  -, -1, ra_subd_flags1 ; nop
   # }
   # flags:
   # - - - - z z - - (twice)
   # n n n n - - - - (twice)
   # c c - - - - - - (twice)
   # rb_cubics[i]: x0, y0, x1, y1, x2, y2, x3, y3 (twice)
   # ra_temp: garbage[2], x0 + x1, y0 + y1, x1 + x2, y1 + y2, x2 + x3, y2 + y3 (twice)
   # ra_temp2: garbage[4], x0 + 2x1 + x2, y0 + 2y1 + y2, x1 + 2x2 + x3, y1 + 2y2 + y3 (twice)
   # r0: garbage[6], x0 + 3x1 + 3x2 + x3, y0 + 3y1 + 3y2 + y3 (twice)
   # r1: garbage[10], tx0, ty0, garbage[2], tx1, ty1
   # r2: garbage[10], tx, ty
   fadd  r3, f(0.125), f(0.125) ; fmul  r0, r0, f(0.125)
   nop                          ; fmul  ra_temp, ra_temp, f(0.5)
   mov  ra_temp2, rb_cubics[i]  ; fmul  r3, ra_temp2, r3
   mov.ifz  r0, r3              ; mov.ifz  rb_cubics[i], ra_temp << 2  @mul_used(4, 5, 12, 13)
   mov.ifn  r0, ra_temp         ; mov.ifn  rb_cubics[i], r3 << 4
   mov.ifc  r0, ra_temp2        ; mov.ifc  rb_cubics[i], r0 << 6       @mul_used(0, 1, 8, 9)
   brr  ra_cubics[i + 1], r:1f
   # branch delay slots {
   nop                          ; mov.ifz  ra_cubics[i], r1 << 2
   nop                          ; mov.ifc  ra_cubics[i], r2 << 2
   nop                          ; mov.ifnn  r1, r2 >> 4
   # }

   # ra_cubics[i]: garbage[8], tx, ty, garbage[2], tx1, ty1, garbage, lr
   # rb_cubics[i]: r0, r1, r2, r3 (twice)

   # add clerp to middle
   mov  r0, ra_cubics[i] ; mov  r2, rb_cubics[i]
   mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
   nop                   ; mov  r1, r0 >> 6
   bra  ra_temp, (ra_scalar << rsi_dash_clerp_lbl) >> 15
   # branch delay slots {
   mov.ifn  r1, r2       ; mov.ifz  r1, r2 >> 6
   sub.setf  -, elem_num, rs2i_dash_clerp_lr ; nop
   mov.ifz  ra_scalar2, ra_temp ; nop
   # }

   # subdivide second half
   mov  r0, rb_cubics[i] ; mov  ra_cubics[i + 1], ra_cubics[i]
   nop                   ; mov  r1, ra_cubics[i] >> 2  @mul_used(10, 11, 14, 15)
:1
.endif
.endr

# transforms, clips, and writes resulting triangles to control list.
# transforms before clipping.
#
# Preconditions:
# rb_temp1-6   : x0, y0, x1, y1, x2, y2
# rb_inactive_elems : negative for all active triangles
#
# Clobbers: r0-3,5, rb_temp1-6, ra_temp1-3, TODO
# + alloc
#
:clip
   mkp "clip\n"
   mk 0x1000

#   mkp "clip: before transform:\n" # some may be invalid if queue wasn't full
#   .rep i, 16
#      mkp "%f'", "rb_temp1", i
#      mkp "%f ", "rb_temp2", i
#      mkp "%f'", "rb_temp3", i
#      mkp "%f ", "rb_temp4", i
#      mkp "%f'", "rb_temp5", i
#      mkp "%f\n", "rb_temp6", i
#   .endr

   # Transform

   mov  r0, ra_user_to_surface_clip_inner
   nop
   .rep i, 3
      .if i==0
         .set xs, rb_temp1
         .set ys, rb_temp2
      .elif i==1
         .set xs, rb_temp3
         .set ys, rb_temp4
      .elif i==2
         .set xs, rb_temp5
         .set ys, rb_temp6
      .else
         .error
      .endif
      nop ; mov  r5rep, r0 << 3
      nop ; fmul  r1, xs, r5     # r1 = xs * r0[3]
      nop ; mov  r5rep, r0 << 4
      nop ; fmul  r2, ys, r5     # r2 = ys * r0[4]
      fadd  r1, r1, r2           # r1 = xs * r0[3] + ys * r0[4]
      nop ; mov  r5rep, r0 << 5
      fadd  r1, r1, r5           # r1 = xs * r0[3] + ys * r0[4] + r0[5]
      nop ; mov  r5rep, r0 << 1
      nop ; fmul  r3, ys, r5     # r3 = ys * r0[1]
      nop ; mov  r5rep, r0 << 0
      nop ; fmul  r2, xs, r5     # r2 = xs * r0[0]
      fadd  r2, r2, r3           # r2 = xs * r0[0] + ys * r0[1]
      nop ; mov  r5rep, r0 << 2
      fadd  xs, r2, r5           # final xs := xs * r0[0] + ys * r0[1] + r0[2]
      mov ys, r1                 # final ys := xs * r0[3] + ys * r0[4] + r0[5]
   .endr

#   mkp "clip: transformed:\n" # some may be invalid if queue wasn't full
#   .rep i, 16
#      mkp "%f'", "rb_temp1", i
#      mkp "%f ", "rb_temp2", i
#      mkp "%f'", "rb_temp3", i
#      mkp "%f ", "rb_temp4", i
#      mkp "%f'", "rb_temp5", i
#      mkp "%f\n", "rb_temp6", i
#   .endr

   # r0: garbage[6], clip_inner[4]

   mov  ra_temp4, rb_inactive_elems ; nop
   nop                              ; mov  r5rep, r0 << 6
   fsub.setf  -, r5, rb_temp1       ; nop
   fsub.ifnn.setf  -, r5, rb_temp3  ; nop
   fsub.ifnn.setf  -, r5, rb_temp5  ; nop
   mov.ifnn  ra_temp4, 0            ; mov  r5rep, r0 << 7
   fsub.setf  -, r5, rb_temp2       ; nop
   fsub.ifnn.setf  -, r5, rb_temp4  ; nop
   fsub.ifnn.setf  -, r5, rb_temp6  ; nop
   mov.ifnn  ra_temp4, 0            ; mov  r5rep, r0 << 8
   fsub.setf  -, rb_temp1, r5       ; nop
   fsub.ifnn.setf  -, rb_temp3, r5  ; nop
   fsub.ifnn.setf  -, rb_temp5, r5  ; nop
   mov.ifnn  ra_temp4, 0            ; mov  r5rep, r0 << 9
   fsub.setf  -, rb_temp2, r5       ; nop
   fsub.ifnn.setf  -, rb_temp4, r5  ; mov  r0, ra_scalar3
   fsub.ifnn.setf  -, rb_temp6, r5  ; nop
   mov.ifnn  ra_temp4, 0            ; mov  r5rep, r0 << rs3i_clip_outer
   fsub.setf  -, r5, rb_temp1       ; nop
   fsub.ifn.setf  -, r5, rb_temp3   ; nop
   fsub.ifn.setf  -, r5, rb_temp5   ; nop
   mov.ifn.setf  -, ra_temp4        ; mov  r5rep, r0 << (rs3i_clip_outer + 1)
   fsub.ifn.setf  -, r5, rb_temp2   ; nop
   fsub.ifn.setf  -, r5, rb_temp4   ; nop
   fsub.ifn.setf  -, r5, rb_temp6   ; nop
   nop                              ; mov  r5rep, r0 << (rs3i_clip_outer + 2)
   fsub.ifn.setf  -, rb_temp1, r5   ; nop
   fsub.ifn.setf  -, rb_temp3, r5   ; nop
   fsub.ifn.setf  -, rb_temp5, r5   ; nop
   nop                              ; mov  r5rep, r0 << (rs3i_clip_outer + 3)
   fsub.ifn.setf  -, rb_temp2, r5   ; nop
   fsub.ifn.setf  -, rb_temp4, r5   ; nop
   fsub.ifn.setf  -, rb_temp6, r5   ; mov  ra_temp3, 0

   # n: active && !(outside inner) && inside outer
   # ra_temp3: 0
   # ra_temp4: n if active && !(outside inner)

   brr.allnn  -, r:clip_out_end # skip output if no tris
   # branch delay slots {

   # need at least (16 * 3 * 4) + 4 = 196 bytes for 16 tris plus head
   mov  r0, 196
   sub.setf  -, ra_scalar, r0 ; mov.ifn  ra_temp3, -1
   mov.ifn.setf  -, neg_elems(elems(rsi_alloc_size))
   # }
   brr.anyn  ra_temp, r:alloc # preserves ra_temp3 and ra_temp4
   # branch delay slots {
   nop
   nop
   nop
   # }

   # convert the vertices to packed xy format
   mov  r5rep, f(16.0)
   mov  ra_temp, 0xffff
   nop                       ; fmul  r0, rb_temp1, r5
   fadd.setf  r0, r0, f(0.5) ; nop
   fsub.ifn  r0, r0, f(1.0)  ; nop
   ftoi  r0, r0              ; fmul  r1, rb_temp2, r5
   fadd.setf  r1, r1, f(0.5) ; nop
   fsub.ifn  r1, r1, f(1.0)  ; nop
   ftoi  r1, r1              ; nop
   shl  r1, r1, -16          ; v8min  r0, r0, ra_temp
   or  r0, r0, r1            ; fmul  r1, rb_temp3, r5
   fadd.setf  r1, r1, f(0.5) ; v8min  r0, r0, ra_temp3
   fsub.ifn  r1, r1, f(1.0)  ; nop
   ftoi  r1, r1              ; fmul  r2, rb_temp4, r5
   fadd.setf  r2, r2, f(0.5) ; nop
   fsub.ifn  r2, r2, f(1.0)  ; nop
   ftoi  r2, r2              ; nop
   shl  r2, r2, -16          ; v8min  r1, r1, ra_temp
   or  r1, r1, r2            ; fmul  r2, rb_temp5, r5
   fadd.setf  r2, r2, f(0.5) ; v8min  r1, r1, ra_temp3
   fsub.ifn  r2, r2, f(1.0)  ; nop
   ftoi  r2, r2              ; fmul  r3, rb_temp6, r5
   fadd.setf  r3, r3, f(0.5) ; nop
   fsub.ifn  r3, r3, f(1.0)  ; nop
   ftoi  r3, r3              ; nop
   shl  r3, r3, -16          ; v8min  r2, r2, ra_temp
   or  r2, r2, r3            ; nop
   nop                       ; v8min  r2, r2, ra_temp3

   # 3 VPM row writes. A triangle's 3 points now lie in a vertical line in the vpm wr-rows
   mov r3, ra_scalar
   mov -, vw_wait
   nop ; mov vw_setup, r3 << rsi_out_vpm_setup
   mov vpm, r0  @geomd_verts_a(0)
   mov vpm, r1  @geomd_verts_a(1)
   mov vpm, r2  @geomd_verts_a(2) @geomd_tris_add

   mov r0, ra_scalar
   nop
   nop ; mov r5rep, r0 << rsi_out_vdw_setup_0
   mov r1, ((15<<23)-(13<<16)-(1<<14)) # UNITS+=15 (now 16), DEPTH-=13 (now 3), HORIZ-=1 (now vertical)
   add r5rep, r5, r1
   mov vw_setup, r5
   .assert rsi_clerp_q_n==0
   mov r5rep, (16*12) # r5 = the number of bytes being written out.
   mov.setf  -, elems(rsi_alloc_p, rsi_alloc_size)
   sub.ifnz  ra_scalar, ra_scalar, r5 # alloc_p and alloc_size decremented by <num bytes to be written out>
   nop
   mov  r0, ra_scalar
   nop
   nop ; mov r5rep, r0 << rsi_alloc_p
   mkp "clip: dma wr to 0x%08x\n", "r5"
   mov vw_setup, vdw_setup_1(0) # vdw between-row-stride to 0

   mov vw_addr, r5

:clip_out_end

   # ra_temp3: n if active && !(outside inner) && inside outer
   # ra_temp4: n if active && !(outside inner)

   not.setf  -, ra_temp3
   nop ; mov  ra_temp, (ra_scalar2 << rs2i_clip_lr) >> 15  @mul_used(15)
   mov.ifn.setf  -, ra_temp4
   # n: active && !(outside inner) && !(inside outer)
   bra.allnn  -, ra_temp # return now if no tris need clipping
   # branch delay slots {
   nop
   nop
   nop
   # }

   # need to clip some triangles. this should be a rare case, so it doesn't need
   # to be fast

   mov.ifnn  ra_temp4, 0 ; mov  r0, ra_scalar3
   mov.setf  ra_temp3, [-1, -1, -1, -1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0]
   nop ; mov.ifn  ra_temp3, r0 << rs3i_clip_outer
   mov.setf  -, [0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
   fsub.ifnz  ra_temp3, f(0.0), ra_temp3

:clip_outer_loop

   # ra_temp3: clip outer min x, clip outer min y, -clip outer max x, -clip outer max y, 1, 0, 1, 0, 1, 0, 0, 0
   # ra_temp4: n if tri in corresponding column of rb_temp* needs clipping

   # if tri 0 doesn't need clipping, continue
   sub.setf  -, elem_num, 1
   mov.ifn.setf  -, ra_temp4
   brr.allnn  -, r:clip_outer_loop_next
   # branch delay slots {
   mov.ifn  ra_temp4, 0 ; mul24  r0, 8, 8 # unmark

   # need at least (5 * 3 * 4) + 4 = 64 bytes for 5 triangles plus head
   mov.setf  -, neg_elems(elems(rsi_alloc_size))
   sub.ifn.setf  -, ra_scalar, r0
   # }
   brr.anyn  ra_temp, r:alloc # preserves ra_temp3 and ra_temp4
   # branch delay slots {
   nop
   nop
   nop
   # }

   mov.setf  -, elem_num ; mov  r0, rb_temp6
   nop ; mov  r0, r0 >> 1  @mul_used(1)
   nop ; mov.ifz  r0, rb_temp5
   nop ; mov  r0, r0 >> 1  @mul_used(1, 2)
   nop ; mov.ifz  r0, rb_temp4
   nop ; mov  r0, r0 >> 1  @mul_used(1, 2, 3)
   nop ; mov.ifz  r0, rb_temp3
   nop ; mov  r0, r0 >> 1  @mul_used(1, 2, 3, 4)
   nop ; mov.ifz  r0, rb_temp2
   nop ; mov  r0, r0 >> 1  @mul_used(1, 2, 3, 4, 5)
   nop ; mov.ifz  r0, rb_temp1
   sub.setf  -, elem_num, 6 ; mov  ra_temp, 6
   nop ; mov.ifnn  r0, r0 >> 6

:clip_edges_loop

   # r0: x0, y0, x1, y1, ..., xn-1, yn-1, x0, y0
   # ra_temp: n * 2
   # ra_temp3: edges[4], flips[4], ends[4]

   .pushnwarn # non-accum rotation warning: we want the weird semantics here!
   mov  r5rep, ra_temp3        ; mov  ra_temp3, ra_temp3 << 1
   .popnwarn
   # r5: e
   and.setf  -, elem_num, 1    ; mov  r2, r0
   mov.ifz  r1, r0             ; mov.ifnz  r1, r0 >> 1
   # r1: x0, x0, x1, x1, ..., xn-1, xn-1, x0, x0
   fsub.setf  ra_temp2, r1, r5 ; mov  r1, r0 << 2
   # !n: in0[2], in1[2], ..., inn-1[2], in0[2]
   # r1: x1, y1, x2, y2, ..., x0, y0
   # r2: x0, y0, x1, y1, ..., xn-1, yn-1
   mov.ifn  r1, r2             ; mov.ifn  r2, r1 # want to do calculation in consistent direction to avoid gaps
   fsub  recip, r1, r2         ; nop
   mov.setf  -, ra_temp        ; nop                   # this is a bit of a hack to make 0 vertices work
   fsub  r3, r5, r2            ; mov.ifz  ra_temp2, -1 # (the verts loop below always does at least one iteration)
   and.setf  -, elem_num, 1    ; fmul  r3, r3, r4
   mov.ifz  r1, r5             ; mov.ifnz  r3, r3 >> 1  @mul_unused(0)
   fmin.ifnz  r3, r3, f(1.0)   ; mov  r5rep, 0
   fsub.ifnz  r3, f(1.0), r3   ; fmul.ifnz  r1, r1, r3
   mov  r3, ra_temp2           ; fmul.ifnz  r2, r2, r3
   fadd.ifnz  r1, r1, r2       ; nop

:clip_verts_loop

   # r0: x0, y0, x1, y1, ..., xn-1, yn-1
   # r1: edge intersect points
   # r2: x'0, y'0, x'1, y'1, ..., x'n'-1, y'n'-1
   # r3: !n if in0[2], in1[2], ..., inn-1[2], in0[2]
   # r5: n' * 2
   # ra_temp: n * 2

   sub.setf  -, elem_num, r5 ; mov  ra_temp2, r3 << 2      # we always do at least one iteration
   mov.setf  -, r3           ; mov.ifnn  r2, r0 >> r5      # (even if there are no vertices)
   add.ifnn  r5rep, r5, 2    ; v8subs  ra_temp, ra_temp, 2 # v8subs saves us from going negative
   sub.setf  -, elem_num, r5 ; mov  r0, r0 << 2            # and the hack above prevents us from
   mov.setf  -, ra_temp      ; mov.ifnn  r2, r1 >> r5      # incrementing r5
   brr.allnz  -, r:clip_verts_loop
   # branch delay slots {
   xor.setf  -, r3, ra_temp2 ; mov  r1, r1 << 2
   add.ifn  r5rep, r5, 2     ; mov  r3, ra_temp2

   # r2: x0, y0, x1, y1, ..., xn-1, yn-1
   # r5: n * 2

   sub.setf  -, elem_num, r5    ; nop
   # }
   mov.setf  -, ra_temp3        ; mov.ifnn  r2, r2 >> r5
   sub.ifz.setf  -, elem_num, 8 ; mov  ra_temp, r5
   nop                          ; mov  r0, r2 >> 1
   mov  r3, 1                   ; mov  r1, ra_temp3
   brr.anyz  -, r:clip_edges_loop
   # branch delay slots {
   and.setf  -, elem_num, r3    ; mov  r5rep, r1 << 4
   mov.setf  -, r5              ; mov.ifz  r0, r0 << 2
   fsub.ifnz  r0, f(0.0), r0    ; nop
   # }

   # r0: x0, y0, x1, y1, ..., xn-1, yn-1
   # ra_temp: n * 2

   # fewer than 3 vertices or more than 7? don't output anything for this
   # triangle. we shouldn't get 1 or 2 vertices but we might get 0. according to
   # maths, more than 7 shouldn't happen, but floating point isn't perfect, so
   # it might
   sub.setf  -, ra_temp, 6
   sub.ifnn.setf  -, 14, ra_temp
   brr.alln  -, r:1f
   # branch delay slots {

   # convert the vertices to packed xy format
   # form a triangle fan and write it out
   nop                       ; fmul  r0, r0, f(16.0)
   fadd.setf  r0, r0, f(0.5) ; nop
   fsub.ifn  r0, r0, f(1.0)  ; nop
   # }
   ftoi  r0, r0              ; mov  -, vw_wait
   shl  r0, r0, -16          ; mov  r1, -16
   shr  r0, r0, r1           ; mov  r1, r0 << 1  @mul_used(0, 2, 4, 6, 8, 10, 12, 14)
   shr  r1, ra_temp, 1       ; v8max  r0, r0, r1
   # r0: xy0, garbage, xy1, garbage, ...
   sub  r1, 2, r1            ; mov  r2, ra_scalar
   # r1: -number of tris
   or  r5rep, r1, 1          ; mov  r3, 1
   and.setf  -, elem_num, r3 ; mov  vw_setup, r2 << rsi_out_vpm_setup
   mov  r5rep, r0            ; mov.ifnz  r0, r0 >> r5
   # r5: xy0
   add.setf  -, elem_num, r1 # todo: this is only needed for the geomd stuff...
   shl  r1, ra_temp, -10     ; mov.ifn  vpm, r5       @geomd_verts_m(0)
   # r1: n << 23
   add  r1, r1, r2           ; mov.ifn  vpm, r0 << 2  @geomd_verts_m(1)
   mov  r3, -(3 << 23) - (13 << 16) - (1 << 14) # -2 units (+ n = num tris), depth 3, vertical
   add  r1, r1, r3           ; mov.ifn  vpm, r0 << 4  @geomd_verts_m(2) @geomd_tris_add
   sub  r3, ra_temp, 4       ; nop
   # r3: (n - 2) * 2
   nop                       ; mul24  r3, r3, 6
   # r3: (n - 2) * 12 = size of tris
   sub  r2, r2, r3           ; mov  vw_setup, r1 << rsi_out_vdw_setup_0
   mov  vw_setup, vdw_setup_1(0)
   mov.setf  -, elems(rsi_alloc_p, rsi_alloc_size)
   mov.ifnz  ra_scalar, r2   ; mov  vw_addr, r2 << rsi_alloc_p

:1

   # if we haven't got any triangles left to clip, return
   nop ; mov  ra_temp, (ra_scalar2 << rs2i_clip_lr) >> 15  @mul_used(15)
   mov.setf  -, ra_temp4
   bra.allnn  -, ra_temp
   # branch delay slots {

:clip_outer_loop_next

   # rotate rb_temp* and ra_temp4 down by one
   mov  r0, ra_temp4 ; mov  r1, rb_temp1
   nop ; mov  r2, rb_temp2
   nop ; mov  ra_temp4, r0 << 1
   # }
   nop ; mov  rb_temp1, r1 << 1
   nop ; mov  rb_temp2, r2 << 1
   nop ; mov  r0, rb_temp3
   nop ; mov  r1, rb_temp4
   nop ; mov  rb_temp3, r0 << 1
   nop ; mov  rb_temp4, r1 << 1
   nop ; mov  r0, rb_temp5
   brr  -, r:clip_outer_loop
   # branch delay slots {
   nop ; mov  r1, rb_temp6
   nop ; mov  rb_temp5, r0 << 1
   nop ; mov  rb_temp6, r1 << 1
   # }

# Corresponds to tess.c:stroke_clerp()
# Processes all clerps in the queue in parallel, sending the resulting tris to :clip.
#
# Preconditions:
#
# clerp_q_n is in [1,16]
#
# Clobbers: todo
#
:flush_clerp_q
   mkp "flush_clerp_q.\n"
   mk 0x2000

   sub.setf  -, elem_num, rsi_clerp_q_n    ; mov  r0, elem_num
   mov.ifz  ra_scalar, 0                   ; mov  r5rep, ra_scalar << rsi_clerp_q_n
   sub  rb_inactive_elems, r0, r5          ; mov  r0, ra_scalar3
   # rb_inactive_elems: negative means active
   # ra_scalar[rsi_clerp_q_n]: 0
   mov  r1, CLERP_Q_TX0_NORMAL
   sub.setf  -, ra_clerp_q_tx0, r1         ; fmul  r5rep, r0 << rs3i_line_width, f(0.5)
   mov  rb_temp7, r5                       ; mov  r1, r0 << rs3i_cos_theta
   nop                                     ; mov  r5rep, r0 << rs3i_sin_theta
   mov.ifz  ra_clerp_q_tx0, rb_clerp_q_tx1 ; mov  rb_temp8, r5
   mov.ifz  ra_clerp_q_ty0, rb_clerp_q_ty1 ; mov  r5rep, r1
   mov  rb_temp9, r5                       ; fmul  r1, ra_clerp_q_tx0, rb_clerp_q_ty1
   nop                                     ; fmul  r2, ra_clerp_q_ty0, rb_clerp_q_tx1
   nop                                     ; mov  r5rep, r0 << rs3i_width_hack_clerp
   mov  rb_temp10, rb_inactive_elems       ; fmul.ifnz  ra_clerp_q_tx0, ra_clerp_q_tx0, r5
   nop                                     ; fmul.ifnz  ra_clerp_q_ty0, ra_clerp_q_ty0, r5
   mov  r0, f(0.0)                         ; fmul.ifnz  rb_clerp_q_tx1, rb_clerp_q_tx1, r5
   fsub.setf  -, r1, r2                    ; fmul.ifnz  rb_clerp_q_ty1, rb_clerp_q_ty1, r5
   fsub.ifnn  rb_temp8, r0, rb_temp8       ; nop  @geomd_i(geomd_i_stroke_clerp_a)

:1
   # rb_temp7: line_width/2
   # rb_temp8: s
   # rb_temp9: cos_theta
   # rb_temp10: originally inactive elements
   nop                               ; fmul  r0, ra_clerp_q_tx0, rb_temp9
   nop                               ; fmul  r1, ra_clerp_q_ty0, rb_temp8
   fadd  r0, r0, r1                  ; fmul  r1, ra_clerp_q_ty0, rb_temp9
   mov  rb_temp11, r0                ; fmul  r2, ra_clerp_q_tx0, rb_temp8
   # r0, rb_temp11: tx
   fsub  r1, r1, r2                  ; fmul  r2, r0, rb_clerp_q_ty1
   mov  rb_temp12, r1                ; fmul  r3, r1, rb_clerp_q_tx1
   # r1, rb_temp12: ty
   nop                               ; fmul  r2, r2, rb_temp8
   nop                               ; fmul  r3, r3, rb_temp8
   # r2: tx * ty1 * s
   # r3: ty * tx1 * s
   mov.setf  -, rb_inactive_elems    ; nop
   # deactivate elements where we've gone too far (ie (tx * ty1 * s) >= (ty * tx1 * s))
   fsub.ifn.setf  rb_inactive_elems, r2, r3 ; fmul  r2, ra_clerp_q_tx0, rb_temp7
   # if no active elements left, exit loop
   brr.allnn  -, r:1f
   # branch delay slots {
   mov  r5rep, rb_temp7              ; fmul  r3, ra_clerp_q_ty0, rb_temp7
   fsub  rb_temp1, ra_clerp_q_x0, r3 ; fmul  r0, r0, r5
   fadd  rb_temp2, ra_clerp_q_y0, r2 ; fmul  r1, r1, r5
   # }
   fsub  rb_temp3, ra_clerp_q_x0, r1 ; mov  r1, ra_clerp_q_x0
   brr  ra_temp, r:clip
   # branch delay slots {
   fadd  rb_temp4, ra_clerp_q_y0, r0 ; mov  r0, ra_clerp_q_y0
   sub.setf  -, elem_num, rs2i_clip_lr ; mov  rb_temp5, r1
   mov.ifz  ra_scalar2, ra_temp      ; mov  rb_temp6, r0
   # }
   mov  r5rep, rb_temp7              ; fmul  r2, ra_clerp_q_tx0, rb_temp7
   mov.setf  -, rb_inactive_elems    ; fmul  r3, ra_clerp_q_ty0, r5
   mov  rb_temp5, ra_clerp_q_x0      ; mov.ifn  ra_clerp_q_tx0, rb_temp11
   fadd  rb_temp1, ra_clerp_q_x0, r3 ; fmul  r0, rb_temp11, r5
   fsub  rb_temp2, ra_clerp_q_y0, r2 ; fmul  r1, rb_temp12, r5
   fadd  rb_temp3, ra_clerp_q_x0, r1 ; mov.ifn  ra_clerp_q_ty0, rb_temp12
   brr  -, r:clip
   # branch delay slots {
   fsub  rb_temp4, ra_clerp_q_y0, r0 ; mov  r0, ra_clerp_q_y0
   sub.setf  -, elem_num, rs2i_clip_lr ; mov  rb_temp6, r0
   mov.ifz  ra_scalar2, a:1b
   # }
:1

   # r2: tx0 * line_width/2
   # r3: ty0 * line_width/2
   # r5: line_width/2
   # rb_temp1: x0 - (ty0 * line_width/2)
   # rb_temp2: y0 + (tx0 * line_width/2)

   .set ra_ccw0x, ra_clerp_q_tx0
   .set ra_ccw0y, ra_clerp_q_ty0
   .set ra_cw0x, ra_clerp_q_x0
   .set ra_cw0y, ra_clerp_q_y0
   .set rb_ccw1x, rb_clerp_q_tx1
   .set rb_ccw1y, rb_clerp_q_ty1
   .set ra_cw1x, ra_clerp_q_x1
   .set ra_cw1y, ra_clerp_q_y1

   fadd  ra_cw0x, ra_clerp_q_x0, r3          ; fmul  r0, rb_clerp_q_tx1, r5  @geomd_i(geomd_i_stroke_clerp_b)
   fsub  ra_cw0y, ra_clerp_q_y0, r2          ; fmul  r1, rb_clerp_q_ty1, r5
   fsub  rb_ccw1x, ra_clerp_q_x1, r1         ; mov  ra_ccw0x, rb_temp1
   fadd  rb_ccw1y, ra_clerp_q_y1, r0         ; mov  ra_ccw0y, rb_temp2
   fadd  ra_cw1x, ra_clerp_q_x1, r1          ; mov  rb_inactive_elems, rb_temp10
   fsub  ra_cw1y, ra_clerp_q_y1, r0          ; mov  r5rep, rs2i_clip_lr
   fsub  r0, ra_cw0x, rb_temp1               ; mov  rb_temp3, ra_cw0x
   fsub  r1, ra_cw0y, rb_temp2               ; mov  rb_temp7, r0
   fsub  r2, ra_cw1x, rb_temp1               ; mov  rb_temp8, r1
   fsub  r3, ra_cw1y, rb_temp2               ; fmul  ra_temp, r1, r2        # ay * cx
   fsub  r0, rb_ccw1x, ra_ccw0x              ; fmul  rb_temp9, r0, r3       # ax * cy
   fsub  r1, rb_ccw1y, ra_ccw0y              ; fmul  ra_temp2, r0, r3       # bx * cy
   fsub  r2, ra_cw1x, rb_ccw1x               ; fmul  rb_temp10, r1, r2      # by * cx
   fsub  r3, ra_cw1y, rb_ccw1y               ; mov  rb_temp6, ra_cw1y
   mov  r0, rb_temp8                         ; fmul  ra_temp3, r0, rb_temp8 # bx * ay
   mov  r1, rb_temp7                         ; fmul  rb_temp7, r1, rb_temp7 # by * ax
   fsub  ra_temp, ra_temp, rb_temp9          ; fmul  r0, r2, r0             # dx * ay
   fsub  rb_temp8, ra_temp2, rb_temp10       ; fmul  r1, r3, r1             # dy * ax
   fsub  r0, r0, r1                          ; mov  rb_temp4, ra_cw0y       # (dx * ay) - (dy * ax)
   xor.setf  rb_temp9, ra_temp, rb_temp8     ; mov  recip, r0               # ((ay * cx) - (ax * cy)) ^ ((bx * cy) - (by * cx)), flags n if intersecting
   fmaxabs  r0, r0, r0                       ; mov  rb_temp5, ra_cw1x
   fsub  r1, rb_temp7, ra_temp3              ; nop                          # (by * ax) - (bx * ay)
   fsub.ifn.setf  rb_temp9, rb_eps, r0       ; fmul  r0, r1, r4
   mov.ifn.setf  rb_temp9, rb_inactive_elems ; fmul  r2, r2, r0
   fadd  rb_temp7, rb_ccw1x, r2              ; fmul  r3, r3, r0
   fadd  rb_temp8, rb_ccw1y, r3              ; nop
   brr  ra_temp, r:clip
   # branch delay slots {
   mov.ifn  rb_temp5, rb_temp7               ; nop
   sub.setf  -, elem_num, r5                 ; mov.ifn  rb_temp6, rb_temp8
   mov.ifz  ra_scalar2, ra_temp              ; nop
   # rb_temp1: ccw0x
   # rb_temp2: ccw0y
   # rb_temp3: cw0x
   # rb_temp4: cw0y
   # rb_temp5: cw1x/ix
   # rb_temp6: cw1y/iy
   # rb_temp7: ix
   # rb_temp8: iy
   # rb_temp9: negative if active and intersecting
   # }

   nop                          ; mov  rb_temp3, ra_cw1x
   nop                          ; mov  rb_temp4, ra_cw1y
   mov  r0, rs2i_clip_lr        ; mov  rb_temp5, ra_ccw0x
   mov.setf  -, rb_temp9        ; mov  rb_temp6, ra_ccw0y
   nop                          ; mov.ifn  rb_temp5, rb_temp7
   brr  ra_temp, r:clip
   # branch delay slots {
   nop                          ; mov.ifn  rb_temp6, rb_temp8
   sub.setf  -, elem_num, r0    ; mov  rb_temp1, rb_ccw1x
   mov.ifz  ra_scalar2, ra_temp ; mov  rb_temp2, rb_ccw1y
   # }

   nop                          ; mov  r5rep, ra_scalar << rsi_q_lr
   mov.setf  rb_inactive_elems, rb_temp9 ; mov  ra_temp, r5
   nop                          ; mov  rb_temp9, r5
   # return now if none intersecting
   bra.allnn  -, ra_temp
   # branch delay slots {
   nop                          ; mov  rb_temp1, ra_ccw0x
   mov  r0, rs2i_clip_lr        ; mov  rb_temp2, ra_ccw0y
   nop                          ; mov  rb_temp3, rb_ccw1x
   # }
   brr  ra_temp, r:clip
   # branch delay slots {
   nop                          ; mov  rb_temp4, rb_ccw1y
   sub.setf  -, elem_num, r0    ; mov  rb_temp5, rb_temp7
   mov.ifz  ra_scalar2, ra_temp ; mov  rb_temp6, rb_temp8
   # }

   nop                           ; mov  rb_temp1, ra_cw0x
   nop                           ; mov  rb_temp2, ra_cw0y
   mov  r0, rs2i_clip_lr         ; mov  rb_temp3, ra_cw1x
   brr  -, r:clip
   # branch delay slots {
   nop                           ; mov  rb_temp5, rb_temp7
   sub.setf  -, elem_num, r0     ; mov  rb_temp6, rb_temp8
   mov.ifz  ra_scalar2, rb_temp9 ; mov  rb_temp4, ra_cw1y
   # }

   .unset ra_cw1y
   .unset ra_cw1x
   .unset rb_ccw1y
   .unset rb_ccw1x
   .unset ra_cw0y
   .unset ra_cw0x
   .unset ra_ccw0y
   .unset ra_ccw0x

# flush_join_q_*
#
# Preconditions
#
# join_q_n is in [1,16]
#
# Clobbers: xxx r0-3,5, rb_temp1-6, ra_temp
# + :clip
#

.macro flush_join_q_common_begin
   mk 0x2100

   mov r0, rb_scalar  @geomd_i(geomd_i_stroke_join)
   nop
   nop ; mov r5rep, r0 << rbsi_join_q_n
   sub rb_inactive_elems, elem_num, r5          # N if element active (i.e. not off the end of the queue)
   sub.setf -, elem_num, rbsi_join_q_n
   mov.ifz rb_scalar, 0                         # join_q_n reset to 0

   # if ((tx0 * ty1) > (ty0 * tx1)): swap and flip tangents to make equivalent cw join
   mov r5rep, f(0)
   mov r0, rb_join_q_tx0
   nop ; fmul r1, r0, rb_join_q_ty1
   mov r2, rb_join_q_ty0
   nop ; fmul r3, r2, rb_join_q_tx1
   fsub.setf -, r3, r1                          # ifn ==> (ty0 * tx1) - (tx0 * ty1) < 0 ==> (tx0 * ty1) > (ty0 * tx1)
   fsub.ifn rb_join_q_tx0, r5, rb_join_q_tx1
   fsub.ifn rb_join_q_tx1, r5, r0
   fsub.ifn rb_join_q_ty0, r5, rb_join_q_ty1
   fsub.ifn rb_join_q_ty1, r5, r2

.endm


:flush_join_q_miter
   mkp "flush_join_q_miter.\n"
   mk 0x2110

   flush_join_q_common_begin

   #TODO-OPT again, easier if the queues were split between RF A and B.

   mov ra_temp2, rb_join_q_tx0
   mov ra_temp3, rb_join_q_ty0
   fsub.setf r0, ra_temp2, rb_join_q_tx1     # r0 = nx = tx0 - tx1
   fsub.ifn r1, f(0.0), r0 ; mov.ifnn r1, r0 # r1 = abs(nx)
   fsub.setf r2, ra_temp3, rb_join_q_ty1     # r2 = ny = ty0 - ty1
   fsub.ifn r3, f(0.0), r2 ; mov.ifnn r3, r2 # r3 = abs(ny)
   fsub.setf -, r1, f(1.0)                   # n set if abs(nx) < 1.0f
   fsub.ifn.setf -, r3, f(1.0)               # n set if (absf_(nx) < 1.0f) && (absf_(ny) < 1.0f)
   fadd.ifn r0, ra_temp3, rb_join_q_ty1
   fsub.ifn r0, f(0.0), r0                   # nx = -(ty0 + ty1)
   fadd.ifn r2, ra_temp2, rb_join_q_tx1      # ny = tx0 + tx1

   nop ; fmul r1, r0, r0
   nop ; fmul r3, r2, r2
   fadd r1, r1, r3                           # r1 = (nx * nx) + (ny * ny)

   mk 0x2119
   # one newton-raphson iteration
   mov rsqrt, r1 ; fmul r1, r1, f(0.5)       # r4 = oo_dn (estimate) (in 3 cycles) ; r1 = rsqrt_input * 0.5f
   mov r5rep, f(1.5)
   nop
   nop ; fmul r1, r1, r4
   nop ; fmul r1, r1, r4                     # r1 = (rsqrt_input * 0.5f) * est * est
   fsub r1, r5, r1
   nop ; fmul r1, r4, r1                     # r1 = oo_dn (more accurate)

   nop ; fmul r0, r0, r1                     # nx *= oo_dn
   nop ; fmul r2, r2, r1                     # ny *= oo_dn
   nop ; fmul r0, r0, rb_join_q_tx0          # r0 = nx * tx0
   nop ; fmul r2, r2, rb_join_q_ty0          # r2 = ny * ty0
   fadd r0, r0, r2                           # r0 = cos_mao2
   mov rb_temp7, r0 ; fmul r1, r0, r0
   fsub rb_temp8, f(1.0), r1                 # rb_temp8 = sin2_mao2

  .assert rs3i_line_width == 0
   mov r5rep, ra_scalar3
   nop ; fmul r5rep, r5, f(0.5)              # r5 = stroke->i2->line_width * 0.5f;
   nop ; fmul rb_join_q_tx0, rb_join_q_tx0, r5
   nop ; fmul rb_join_q_ty0, rb_join_q_ty0, r5
   nop ; fmul rb_join_q_tx1, rb_join_q_tx1, r5
   nop ; fmul rb_join_q_ty1, rb_join_q_ty1, r5
   mov r0, rb_join_q_x
   mov r1, rb_join_q_y


   # setup params for clip
   fsub rb_temp3, r0, rb_join_q_ty0 # ccw0x #TODO-OPT save for later?
   fadd rb_temp4, r1, rb_join_q_tx0 # ccw0y
   fsub rb_temp5, r0, rb_join_q_ty1 # ccw1x
   fadd rb_temp6, r1, rb_join_q_tx1 # ccw1y

   mov rb_temp1, rb_join_q_x
   mov rb_temp2, rb_join_q_y

   mk 0x211b
   brr -, r:clip
      sub.setf -, elem_num, rs2i_clip_lr
      mov.ifz ra_scalar2, a:1f
      nop
:1

   mk 0x211c
   mov r0, ra_scalar3
   mov rsqrt, rb_temp8           # r4 = oo_sin_mao2 (estimate) (in 3 cycles)
   mov r5rep, f(EPS)
   fsub.setf -, r5, rb_temp8     # n if sin2_mao2 > EPS
   nop ; mov r5rep, r0 << rs3i_miter_limit

   # one newton-raphson iteration:
   mov r1, f(0.5)
   nop ; fmul r1, rb_temp8, r1   # r1 = sin2_mao2 * 0.5f
   nop ; fmul r0, r4, r4
   nop ; fmul r0, r1, r0         # r0 = (sin2_mao2 * 0.5f) * est * est
   mov r1, f(1.5)
   fsub r0, r1, r0               # r0 = 1.5f - ((sin2_mao2 * 0.5f) * est * est)
   nop ; fmul r0, r4, r0         # r0 = oo_sin_mao2 (more accurate) = est * (1.5f - ((sin2_mao2 * 0.5f) * est * est)

   fsub.ifn.setf -, r0, r5       # n if (sin2_mao2 > EPS) && (oo_sin_mao2 < stroke->i2->miter_limit)
   mk 0x211d
   mov.ifnn rb_inactive_elems, 0 # don't draw miter triangle
   nop ; fmul r0, rb_temp7, r0               # r0 = cos_mao2 * oo_sin_mao2
   nop ; fmul r1, rb_join_q_tx0, r0          # r1 = (tx0 * cos_mao2 * oo_sin_mao2)
   mov r2, rb_join_q_x
   fsub rb_temp3, r2, rb_join_q_ty0          # rb_temp3 = ccw0x
   fsub rb_temp5, r2, rb_join_q_ty1          # rb_temp5 = ccw1x
   fadd rb_temp1, rb_temp3, r1               # rb_temp1 = mx
   nop ; fmul r1, rb_join_q_ty0, r0          # ; r1 = (ty0 * cos_mao2 * oo_sin_mao2)
   mov r3, rb_join_q_y
   fadd rb_temp4, r3, rb_join_q_tx0          # rb_temp4 = ccw0y
   fadd rb_temp6, r3, rb_join_q_tx1          # rb_temp6 = ccw1y
   fadd rb_temp2, rb_temp4, r1               # rb_temp2 = my

   mk 0x211f
   brr -, r:clip #TODO-OPT tail call
      sub.setf -, elem_num, rs2i_clip_lr
      mov.ifz ra_scalar2, a:1f
      nop
:1

   # return
   nop ; mov  r5rep, ra_scalar << rsi_q_lr
   mov  ra_temp, r5
   nop
   bra -, ra_temp
      nop
      nop
      nop


:flush_join_q_round
   mkp "flush_join_q_round.\n"
   mk 0x2120

   flush_join_q_common_begin


   mov r1, ra_scalar3
   nop
   nop ; mov r5rep, r1<<rs3i_cos_theta
   mov r2, r5
   nop ; mov r5rep, r1<<rs3i_sin_theta
   mov r3, r5
   nop ; mov r5rep, r1<<rs3i_width_hack_round

   # first iteration

   nop ; fmul r0, rb_join_q_tx0, r2            # ; r0 = tx * cos_theta
   nop ; fmul r1, rb_join_q_ty0, r3            # ; r1 = ty * sin_theta
   fadd r0, r0, r1 ; fmul r1, rb_join_q_ty0, r2  # r0 = tx ; r1 = ty * cos_theta
   nop ; fmul r2, rb_join_q_tx0, r3              # ; r2 = tx * sin_theta
   fsub r1, r1, r2 ; fmul r0, r0, r5                  # r1 = ty ; r0 = tx *= width_hack_round
   nop ; fmul r1, r1, r5                              # ; r1 = ty *= width_hack_round
   brr -, r:flush_join_q_round_loop_merge_point
      mov rb_temp7, rb_inactive_elems
      nop
      nop

:flush_join_q_round_loop_start
   # regular iteration

   mov r1, ra_scalar3
   nop
   nop ; mov r5rep, r1<<rs3i_cos_theta
   mov r2, r5
   nop ; mov r5rep, r1<<rs3i_sin_theta

   nop ; fmul r0, rb_join_q_tx0, r2            # ; r0 = tx * cos_theta
   nop ; fmul r1, rb_join_q_ty0, r5            # ; r1 = ty * sin_theta
   fadd r0, r0, r1 ; fmul r1, rb_join_q_ty0, r2  # r0 = tx ; r1 = ty * cos_theta
   nop ; fmul r2, rb_join_q_tx0, r5              # ; r2 = tx * sin_theta
   fsub r1, r1, r2

:flush_join_q_round_loop_merge_point
   nop ; fmul r2, rb_join_q_tx1, r1
   nop ; fmul r3, rb_join_q_ty1, r0
   mov.setf -, rb_temp7 # n set if active
   fsub.ifn.setf rb_temp7, r3, r2 # n set if active AND (tx1 * ty) > (ty1 * tx)
   mov.ifnn r0, rb_join_q_tx1 # if (!more): tx = tx1
   mov.ifnn r1, rb_join_q_ty1 # if (!more): ty = ty1

   .assert rs3i_line_width == 0
   mov r5rep, ra_scalar3
   nop ; fmul r5rep, r5, f(0.5)                 # r5 = stroke->i2->line_width * 0.5f
   # setup params to clip
   mov rb_temp1, rb_join_q_x
   mov rb_temp2, rb_join_q_y
   nop ; fmul r2, rb_join_q_ty0, r5
   fsub rb_temp3, rb_join_q_x, r2
   nop ; fmul r2, rb_join_q_tx0, r5
   fadd rb_temp4, rb_join_q_y, r2
   nop ; fmul r2, r1, r5
   fsub rb_temp5, rb_join_q_x, r2
   nop ; fmul r2, r0, r5
   fadd rb_temp6, rb_join_q_y, r2
   mov rb_join_q_tx0, r0 # tx0 = tx
   mov rb_join_q_ty0, r1 # ty0 = ty

   brr -, r:clip
      sub.setf -, elem_num, rs2i_clip_lr
      mov.ifz ra_scalar2, a:1f
      nop
:1
   mk 0x212e
   mov.setf -, rb_temp7 # n set if `more`
   mov.ifnn rb_inactive_elems, 0  # deactivate the elements which have just had their last iteration.
   brr.anyn -, r:flush_join_q_round_loop_start # don't break if at least one join not finished.
      nop
      nop
      nop

   # return
   mk 0x212f
   nop ; mov  r5rep, ra_scalar << rsi_q_lr
   mov  ra_temp, r5
   nop
   bra -, ra_temp
      nop
      nop
      nop


:flush_join_q_bevel
   mkp "flush_join_q_bevel.\n"
   mk 0x2130

   flush_join_q_common_begin

   #TODO just bevel for now
   #TODO-OPT much of this would be easier if the queues were split between RF A and B.

   .assert rs3i_line_width == 0
   mov r5rep, ra_scalar3
   nop ; fmul r5rep, r5, f(0.5)                 # r5 = stroke->i2->line_width * 0.5f;
   nop ; fmul r0, rb_join_q_tx0, r5
   nop ; fmul r1, rb_join_q_ty0, r5
   nop ; fmul r2, rb_join_q_tx1, r5
   nop ; fmul r3, rb_join_q_ty1, r5

   # setup parameters to clip.
   mov rb_temp1, rb_join_q_x
   mov rb_temp2, rb_join_q_y
   fsub rb_temp3, rb_join_q_x, r1
   fadd rb_temp4, rb_join_q_y, r0
   fsub rb_temp5, rb_join_q_x, r3
   fadd rb_temp6, rb_join_q_y, r2

   sub.setf -, elem_num, rs2i_clip_lr
   mov.ifz ra_scalar2, a:1f
   brr -, r:clip #TODO-OPT tail call
      nop
      nop
      nop
:1

   # return
   nop ; mov  r5rep, ra_scalar << rsi_q_lr
   mov  ra_temp, r5
   nop
   bra -, ra_temp
      nop
      nop
      nop


# flush_cap_q_*
#
# Preconditions
#
# cap_q_n is in [1,16]
#
# Clobbers: r0-3,5, rb_temp1-12, ra_temp
# + :clip
# xxx

:flush_cap_q_butt
   mkp "flush_cap_q_butt.\n"
   mk 0x2210

   # nothing to draw for butt cap
   nop ; mov  r5rep, ra_scalar << rsi_q_lr
   mov  ra_temp, r5
   nop
   bra  -, ra_temp
      sub.setf  -, elem_num, rbsi_cap_q_n
      mov.ifz  rb_scalar, 0 # cap_q_n reset to 0
      nop


.macro flush_cap_q_common_begin

   mov r0, rb_scalar  @geomd_i(geomd_i_stroke_cap)
   nop
   nop ; mov r5rep, r0 << rbsi_cap_q_n
   sub rb_inactive_elems, elem_num, r5          # N if element active (i.e. not off the end of the queue)
   sub.setf -, elem_num, rbsi_cap_q_n
   mov.ifz rb_scalar, 0                         # cap_q_n reset to 0

   .assert rs3i_line_width == 0
   mov r5rep, ra_scalar3
   nop ; fmul r5rep, r5, f(0.5)
   nop ; fmul ra_cap_q_tx, ra_cap_q_tx, r5 # `tx *= stroke->i2->line_width * 0.5f`
   nop ; fmul ra_cap_q_ty, ra_cap_q_ty, r5 # `ty *= stroke->i2->line_width * 0.5f`

.endm


:flush_cap_q_square
   mkp "flush_cap_q_square.\n"
   mk 0x2220

   flush_cap_q_common_begin

   mov r0, ra_cap_q_x
   mov r1, ra_cap_q_y
   fsub rb_temp1, r0, ra_cap_q_ty # ccw0x
   fadd rb_temp2, r1, ra_cap_q_tx # ccw0y
   fadd rb_temp3, r0, ra_cap_q_ty # cw0x
   fsub rb_temp4, r1, ra_cap_q_tx # cw0y
   fadd rb_temp5, rb_temp1, ra_cap_q_tx # ccw1x
   fadd rb_temp6, rb_temp2, ra_cap_q_ty # ccw1y

   sub.setf -, elem_num, rs2i_clip_lr
   mov.ifz ra_scalar2, a:1f
   brr -, r:clip
      mov rb_temp7, rb_temp5
      mov rb_temp8, rb_temp6
      nop
:1

   mov r0, ra_cap_q_x ; mov rb_temp1, rb_temp7 # ; ccw1x
   mov r1, ra_cap_q_y ; mov rb_temp2, rb_temp8 # ; ccw1y
   fadd rb_temp5, r0, ra_cap_q_ty # cw0x
   fsub rb_temp6, r1, ra_cap_q_tx # cw0y
   fadd rb_temp3, rb_temp5, ra_cap_q_tx # cw1x
   fadd rb_temp4, rb_temp6, ra_cap_q_ty # cw1y

   brr -, r:clip
      sub.setf -, elem_num, rs2i_clip_lr
      mov.ifz ra_scalar2, a:1f
      nop
:1

   # return
   nop ; mov  r5rep, ra_scalar << rsi_q_lr
   mov  ra_temp, r5
   nop
   bra -, ra_temp
      nop
      nop
      nop


:flush_cap_q_round

   mkp "flush_cap_q_round.\n"
   mk 0x2230

   # loop implementation assumes n >= 3 (i.e loop_first and loop_regular are run at least once). This is safe
   # as if cos_theta==STROKE_COS_THETA_O2_MIN then (PI * stroke->i2->oo_theta) ~= 4.0

   flush_cap_q_common_begin

   # `stroke_tr_clip_tri(stroke, x, y, x - ty, y + tx, x + ty, y - tx)`
   mov rb_temp1, ra_cap_q_x ; mov r0, ra_cap_q_x
   mov rb_temp2, ra_cap_q_y ; mov r1, ra_cap_q_y
   fsub rb_temp3, r0, ra_cap_q_ty
   fadd rb_temp4, r1, ra_cap_q_tx
   fadd rb_temp5, r0, ra_cap_q_ty
   fsub rb_temp6, r1, ra_cap_q_tx

   sub.setf -, elem_num, rs2i_clip_lr
   mov.ifz ra_scalar2, a:1f
   brr -, r:clip
      nop
      nop
      nop
:1
   mov r1, ra_scalar3
   mov r0, f(PI)

   mk 0x2201
   nop ; mov r5rep, r1<<rs3i_oo_theta
   nop ; fmul r0, r0, r5   # r0[allelem] = PI * stroke->i2->oo_theta
   ftoi rb_temp7, r0       # rb_temp7[allelem] = n - 1 (i.e. ceil_(PI * stroke->i2->oo_theta) - 1 because ftoi is round-to-zero)
   fsub rb_temp8, f(0.0), ra_cap_q_tx # rb_temp8 = last_tx = -tx;
   fsub rb_temp9, f(0.0), ra_cap_q_ty # rb_temp9 = last_ty = -ty;
   mov rb_temp10, 0 # rb_temp10 = i = 0 #TODO-OPT why not just decrement n

   nop ; mov r5rep, r1<<rs3i_cos_theta
   mov rb_temp11, r5
   nop ; mov r5rep, r1<<rs3i_sin_theta
   mov rb_temp12, r5
   nop ; mov r5rep, r1<<rs3i_width_hack_round

   # i==0 iteration
   nop ; fmul r0, ra_cap_q_tx, rb_temp11              # ; r0 = tx * cos_theta
   nop ; fmul r1, ra_cap_q_ty, rb_temp12              # ; r1 = ty * sin_theta
   fadd r0, r0, r1 ; fmul r1, ra_cap_q_ty, rb_temp11  # r0 = next_tx ; r1 = ty * cos_theta
   nop ; fmul r2, ra_cap_q_tx, rb_temp12              # ; r2 = tx * sin_theta
   fsub r1, r1, r2 ; fmul r0, r0, r5                  # r1 = next_ty ; r0 = next_tx *= width_hack_round
   nop ; fmul r1, r1, r5                              # ; r1 = next_ty *= width_hack_round
   # setup params to clip
   mov rb_temp1, ra_cap_q_x ; mov r2, ra_cap_q_x
   mov rb_temp2, ra_cap_q_y ; mov r3, ra_cap_q_y
   fsub rb_temp3, r2, ra_cap_q_ty
   fadd rb_temp4, r3, ra_cap_q_tx
   fsub rb_temp5, r2, r1 ; mov ra_cap_q_tx, r0        # ; tx = next_tx
   fadd rb_temp6, r3, r0 ; mov ra_cap_q_ty, r1        # ; ty = next_ty

   sub.setf -, elem_num, rs2i_clip_lr
   mov.ifz ra_scalar2, a:1f
   brr -, r:clip
      nop
      nop
      nop
:1
   mov r0, 1
   add rb_temp10, rb_temp10, r0

:flush_cap_q_round_loop_regular_start
   mk 0x2203
   nop ; fmul r0, ra_cap_q_tx, rb_temp11              # ; r0 = tx * cos_theta
   nop ; fmul r1, ra_cap_q_ty, rb_temp12              # ; r1 = ty * sin_theta
   fadd r0, r0, r1 ; fmul r1, ra_cap_q_ty, rb_temp11  # r0 = next_tx ; r1 = ty * cos_theta
   nop ; fmul r2, ra_cap_q_tx, rb_temp12              # ; r2 = tx * sin_theta
   fsub r1, r1, r2                                    # r1 = next_ty
   # setup params to clip
   mov rb_temp1, ra_cap_q_x ; mov r2, ra_cap_q_x
   mov rb_temp2, ra_cap_q_y ; mov r3, ra_cap_q_y
   fsub rb_temp3, r2, ra_cap_q_ty
   fadd rb_temp4, r3, ra_cap_q_tx
   fsub rb_temp5, r2, r1 ; mov ra_cap_q_tx, r0        # ; tx = next_tx
   fadd rb_temp6, r3, r0 ; mov ra_cap_q_ty, r1        # ; ty = next_ty

   sub.setf -, elem_num, rs2i_clip_lr
   mov.ifz ra_scalar2, a:1f
   brr -, r:clip
      nop
      nop
      nop
:1

   mov r1, 1
   add r0, rb_temp10, r1         # r0[all elem] = i+1 (in accum for the comparison below)
   add rb_temp10, rb_temp10, r1  # ++i
   sub.setf -, r0, rb_temp7      # Z set if i==n-1
   brr.allnz -, r:flush_cap_q_round_loop_regular_start
      nop
      nop
      nop

# i == n-1 iteration
:flush_cap_q_round_loop_final_start
   mov rb_temp1, ra_cap_q_x ; mov r2, ra_cap_q_x
   mov rb_temp2, ra_cap_q_y ; mov r3, ra_cap_q_y
   fsub rb_temp3, r2, ra_cap_q_ty
   fadd rb_temp4, r3, ra_cap_q_tx
   fsub rb_temp5, r2, rb_temp9
   fadd rb_temp6, r3, rb_temp8

   sub.setf -, elem_num, rs2i_clip_lr
   mov.ifz ra_scalar2, a:1f
   brr -, r:clip
      nop
      nop
      nop
:1

   # return
   nop ; mov  r5rep, ra_scalar << rsi_q_lr
   mov  ra_temp, r5
   nop
   bra -, ra_temp
      nop
      nop
      nop

# todo: we could probably make all this dashing stuff simpler with a little
# trickery. i'm thinking:
# dash pattern[even i] &= ~1
# dash pattern[odd i] |= 1
# dash pattern[i < n] != f(1.0), ra_dash_pattern[i] = f(1.0) (which should be the case anyway)
# ra_dash_pattern is actually (dash pattern << dash phase i)
# now we don't need dash_phase_i, and we don't need all the messy shifting in
# the dash loop. dash_phase_i & 1 is just ra_dash_pattern[0] & 1. to increment
# dash_phase_i, we just shift down by 1. we can tell we're at the end if
# ra_dash_pattern[0] is 1. we can either shift back up by dash pattern n, or
# save an unrotated copy of dash pattern somewhere
# actually, slightly better would be:
# dash pattern[even i] |= 1
# dash pattern[odd i] &= ~1
# so the last will be exactly f(1.0)

# todo: should we make some attempt to dash multiple clerps at once?
# (we're doing them one at a time at the moment to keep things simple)

# Corresponds to tess.c:stroke_dash_clerp()
#
# Preconditions:
# x  : r1[0]
# y  : r1[7]
# tx : r1[14]
# ty : r1[15]
#
# Clobbers: r0,1,2,4,5, ra_temp1-3, rb_temp1,3
# + :add_clerp, :add_cap
:dash_clerp_dashing

   # rb_temp3 : stores dx/dy up until `dx/dy *= sod`

   mk 0x3600

   .assert rs2i_x == rsi_dc_locals_x == 0 # Used in several places
   .assert rs2i_y == rsi_dc_locals_y == 7
   .assert rs2i_tx == rsi_dc_locals_tx == 14
   .assert rs2i_ty == rsi_dc_locals_ty == 15
   .assert rsi_dc_locals_next_y==rsi_dc_locals_next_x+7
   .assert rsi_dc_locals_next_x==rs2i_x+3

   mov ra_dc_locals, r1             # save input parameters
   .assert rs2i_dash_phase == rsi_dc_locals_prev_phase
   sub.setf -, elem_num, rsi_dc_locals_prev_phase
   mov.ifz ra_dc_locals, ra_scalar2 # `prev_phase = stroke->dash_phase`
   mov r0, ra_scalar2
   fsub r0, ra_dc_locals, r0        # r0[0] = dx = x - stroke->x, r0[7] = dy = y - stroke->y
   mov rb_temp3, r0 ; fmul r0, r0, r0 # rb_temp3[0] = dx, [7] = dy ; r0[0] = dx*dx, r0[7] = dy*dy
   nop ; fmul r1, ra_scalar3, ra_scalar3
   nop ; mov r5rep, r0<<7           # r5 = dy*dy
   fadd r5rep, r5, r0 ; mov r1, r1<<rs3i_dash_oo_scale # r5 = dx*dx + dy*dy ; r1[0] = stroke->i2->dash_oo_scale * stroke->i2->dash_oo_scale
   nop ; fmul.setf r1, r5, r1       # r1[0] = dos_sq, rest garbage.
   mov recipsqrt, r1
   mov.ifz r5rep, FLT_MAX
   nop
   mov.ifnz r5rep, r4
   # r5 = sod
   sub.setf -, elem_num, rsi_dc_locals_sod
   mov.ifz ra_dc_locals, r5
   nop ; fmul r5rep, r5, r1         # r5 = sod * dos_sq
   sub.setf -, elem_num, rs2i_dash_phase
   fadd.ifz ra_scalar2, ra_scalar2, r5 # `stroke->dash_phase += sod * dos_sq`

   mk 0x3602
   mov r0, ra_dash_pattern
   nop ; mov r5rep, ra_scalar2 << rs2i_dash_phase_i
   mov r1, 16
   sub r5rep, r1, r5                # r5 rotations can only be done one way
   nop
   nop ; mov r5rep, r0 >> r5        # r5 = phase = stroke->i3->dash_pattern[stroke->dash_phase_i]
   sub.setf -, elem_num, rs2i_dash_phase ; mov rb_temp1, r5 # ; rb_temp1[allelem] = phase
   mov.ifnz.setf -, 1               # dash_phase has z set, all other flags are unset.
   fsub.ifz.setf -, ra_scalar2, r5
   brr.anyn -, r:dash_clerp_dashing_loop_end # branch taken if stroke->dash_phase < stroke->i3->dash_pattern[stroke->dash_phase_i]
      nop
      nop
      nop
   nop ; mov r5rep, ra_dc_locals<<rsi_dc_locals_sod
   mov r0, rb_temp3
   mov.setf -, elems(rsi_dc_locals_dx, rsi_dc_locals_dy)
   mov r0, ra_scalar2 ; fmul.ifnz ra_dc_locals<<4, r0, r5 # dx/y *= sod
   mov.setf -, not_elems(elems(rs2i_tx, rs2i_ty)) # rs2i_tx/y flags z, rest nz.
   nop ; mov r5rep, r0 << rs2i_is_cubic
   mov.ifz.setf -, r5                  # rs2i_tx/y flags still z only if !stroke->is_cubic
   mov.ifz ra_scalar2, ra_dc_locals    # stroke->tx/y = tx/y

:dash_clerp_dashing_loop_start
   # rb_temp1 : phase
   mk 0x3604

   nop ; mov r5rep, ra_dc_locals << rsi_dc_locals_prev_phase
   mov r0, ra_scalar2
   fsub r5rep, rb_temp1, r5 ; mov r1, ra_dc_locals            # r5 = t = phase - prev_phase
   nop ; mov r0<<3, r0                 # r0[rsi_dc_locals_next_x/y] = stroke->x/y
   nop ; fmul r1>>1, r1, r5            # r1[rsi_dc_locals_next_x/y] = t * dx/y
   mov.setf -, elems(rsi_dc_locals_next_x, rsi_dc_locals_next_y)
   fadd.ifnz ra_dc_locals, r0, r1      # `next_x/y = stroke->x/y + (t * dx/y);`

   sub.setf -, elem_num, rsi_dc_locals_prev_phase
   mov.ifz ra_dc_locals, rb_temp1      # `prev_phase = phase`

   sub.setf -, elem_num, rs2i_dash_phase_i
   and.ifz.setf -, ra_scalar2, 0x1     # stroke->dash_phase_i & 0x1
   brr.anyz -, r:3f
      nop
      nop
      nop

   # `if (stroke->dash_phase_i & 0x1) {`
   mk 0x3606

   # Setup params to add_cap
   mov r1, ra_dc_locals
   nop
   nop ; mov r0, r1<<rsi_dc_locals_next_x       # r0[0] = sv_x, r0[7] = sv_y
   mov.setf -, elems(14,15)
   fsub.ifnz r0, f(0.0), ra_scalar2             # r0[14] = -stroke->tx, r0[15] = -stroke->ty

   sub.setf -, elem_num, rsi_q_lr
   mov.ifz ra_scalar, a:1f
   brr -, r:add_cap
      nop
      nop
      nop
:1

   brr -, r:2f
      nop
      nop
      nop

:3 # `} else {`

   # Setup params to add_clerp

   mov r1, ra_dc_locals
   nop
   nop ; mov r1, r1<<rsi_dc_locals_next_x       # r1[0] = next_x, r1[7] = next_y
   mov.setf -, elems(14,15)
   mov.ifnz r1, ra_scalar2                      # r1[14] = stroke->tx, r1[15] = stroke->ty

   sub.setf -, elem_num, rsi_q_lr
   mov.ifz ra_scalar, a:1f
   mk 0x3608
   brr -, r:add_clerp
      nop
      nop
      nop
:1

   # Setup params to add_cap
   # identical to above but add_cap uses r0 instead of r1
   mov r0, ra_dc_locals
   nop
   nop ; mov r0, r0<<rsi_dc_locals_next_x
   mov.setf -, elems(14,15)
   mov.ifnz r0, ra_scalar2

   sub.setf -, elem_num, rsi_q_lr
   mov.ifz ra_scalar, a:1f
   mk 0x3609
   brr -, r:add_cap
      nop
      nop
      nop
:1

:2 # `} // if (stroke->dash_phase_i & 0x1)`


   mov r0, ra_dc_locals
   mov.setf -, elems(rs2i_x, rs2i_y)
   nop ; mov.ifnz ra_scalar2, r0<<3             # `stroke->x/y = next_x/y`

   sub.setf -, elem_num, rs2i_dash_phase_i
   add.ifz ra_scalar2, ra_scalar2, 1            # ++stroke->dash_phase_i

   mov r0, ra_scalar3
   mov r1, 0x1f
   nop ; mov r5rep, r0<<rs3i_bitfield
   shr r5rep, r5, 5
   and r5rep, r5, r1                            # r5 = stroke->i2->dash_pattern_n
   sub.ifz.setf -, r5, ra_scalar2               # flag [rs2i_dash_phase_i] is z if stroke->dash_phase_i == stroke->i2->dash_pattern_n, otherwise all flags nz.
   brr.allnz -, r:1f
      mov.ifz ra_scalar2, 0                     # `stroke->dash_phase_i = 0`
      sub.setf -, elem_num, rs2i_dash_phase
      nop
   fsub.ifz ra_scalar2, ra_scalar2, f(1.0)      # `stroke->dash_phase -= 1.0f`
   sub.setf -, elem_num, rsi_dc_locals_prev_phase
   fsub.ifz ra_dc_locals, ra_dc_locals, f(1.0)  # `prev_phase -= 1.0f;`
:1

   # nearly copy-paste from the start of this routine.
   mov r0, ra_dash_pattern
   nop ; mov r5rep, ra_scalar2 << rs2i_dash_phase_i
   mov r1, 16
   sub r5rep, r1, r5                            # r5 rotations can only be done one way
   nop
   nop ; mov r5rep, r0 >> r5                    # r5 = phase = stroke->i3->dash_pattern[stroke->dash_phase_i]
   sub.setf -, elem_num, rs2i_dash_phase ; mov rb_temp1, r5 # ; rb_temp1[allelem] = phase
   mov.ifnz.setf -, 1                           # dash_phase has z set, all other flags are unset.
   fsub.ifz.setf -, ra_scalar2, r5
   brr.allnn -, r:dash_clerp_dashing_loop_start # branch taken if stroke->dash_phase >= stroke->i3->dash_pattern[stroke->dash_phase_i]
      nop
      nop
      nop

:dash_clerp_dashing_loop_end

   # The final (possible) call to stroke_clerp
   nop ; mov r5rep, ra_scalar2<<rs2i_dash_phase_i
   and.setf -, r5, 0x1
   brr.allnz -, r:1f # branch taken if stroke->dash_phase_i & 0x1
      # set up parameters for add_clerp
      sub.setf -, elem_num, rsi_q_lr
      mov.ifz ra_scalar, a:1f
      mov r1, ra_dc_locals
      .assert rsi_dc_locals_x == 0
      .assert rsi_dc_locals_y == 7
      .assert rsi_dc_locals_tx == 14
      .assert rsi_dc_locals_ty == 15
   # here if !(stroke->dash_phase_i & 0x1)
   mk 0x360d
   brr -, r:add_clerp
      nop
      nop
      nop
:1

   # return
   nop ; mov ra_temp, (ra_scalar2 << rs2i_dash_clerp_lr) >> 15  @mul_used(15)
   nop
   bra -, ra_temp
      mov.setf -, elems(rs2i_x, rs2i_y, rs2i_tx, rs2i_ty)
      mov.ifnz ra_scalar2, ra_dc_locals # `stroke->x/y/tx/ty = x/y/tx/ty`
      nop

# Corresponds to tess.c:stroke_dash_clerp()
#
# Preconditions:
# x  : r1[0]
# y  : r1[7]
# tx : r1[14]
# ty : r1[15]
#
# Clobbers: same as add_clerp
:dash_clerp_not_dashing
   mk 0x3500

   mov ra_dc_locals, r1 # save r1 (x/y/tx/ty) in ra_dc_locals because add_clerp will clobber r1

   # call add_clerp
   sub.setf -, elem_num, rsi_q_lr
   mov.ifz ra_scalar, a:1f
   brr -, r:add_clerp
      nop
      nop
      nop
:1

   #return
   nop ; mov ra_temp, (ra_scalar2 << rs2i_dash_clerp_lr) >> 15  @mul_used(15)
   nop
   bra -, ra_temp
      # stroke->x/y/tx/ty = x/y/tx/ty
      .assert 0 == rs2i_x
      .assert 7 == rs2i_y
      .assert 14 == rs2i_tx
      .assert 15 == rs2i_ty
      mov.setf -, elems(rs2i_x, rs2i_y, rs2i_tx, rs2i_ty)
      mov.ifnz ra_scalar2, ra_dc_locals
      nop

# todo: would it be worth inlining add_clerp/add_cap/add_join?

# equivalent to:
# stroke_clerp(stroke, stroke->x, stroke->y, r1[0], r1[7],
#    stroke->is_cubic ? stroke->tx : CLERP_TX0_NORMAL, stroke->ty, r1[14], r1[15])
# as all calls in tess.c match this pattern
#
# Preconditions:
#
# x0  : ra_scalar2[0]
# y0  : ra_scalar2[7]
# x1  : r1[0]
# y1  : r1[7]
# tx0 : ra_scalar2[14]
# ty0 : ra_scalar2[15]
# tx1 : r1[14]
# ty1 : r1[15]
#
# Clobbers: r0, r5, ra_temp
# + flush_clerp_q
#
:add_clerp

   mk 0x3000
   .assert rsi_clerp_q_n==0
   mov r5rep, ra_scalar
   mov.setf -, elem_num
   add.ifz ra_scalar, r5, 1               # clerp_q_n += 1
                                          mkp "add_clerp: index %d : ", "r5"
   sub.setf -, elem_num, r5               # only element <old clerp_q_n> has Z set
   mov r5rep, ra_scalar2 ; mov r0, ra_scalar2                     # r5 = x0
   mov.ifz ra_clerp_q_x0, r5
                                          mkp "%f'", "r5"
   nop ; mov r5rep, r0<<7                                         # r5 = y0
   mov.ifz ra_clerp_q_y0, r5
                                          mkp "%f ", "r5"
   nop ; mov r5rep, r0<<15
   mov.ifz ra_clerp_q_ty0, r5
   mov r5rep, r1                                                  # r5 = x1
   mov.ifz ra_clerp_q_x1, r5
                                          mkp "%f'", "r5"
   nop ; mov r5rep, r1<<7                                         # r5 = y1
   mov.ifz ra_clerp_q_y1, r5
                                          mkp "%f ...\n", "r5"
   nop ; mov r5rep, r1<<14
   mov.ifz rb_clerp_q_tx1, r5
   nop ; mov r5rep, r1<<15
   mov.ifz rb_clerp_q_ty1, r5

   nop ; mov r5rep, r0<<14
   mov.ifz ra_clerp_q_tx0, r5
   nop ; mov r5rep, r0<<rs2i_is_cubic
   mov.ifz.setf -, r5
   mov.ifz ra_clerp_q_tx0, CLERP_Q_TX0_NORMAL

   # return if queue not full
   nop ; mov  r5rep, ra_scalar << rsi_q_lr
   sub.setf  -, elem_num, rsi_clerp_q_n ; mov  ra_temp, r5
   add.ifz.setf  -, ra_scalar, -16
   bra.allnz  -, ra_temp # branch taken if clerp_q_n != 16
      nop
      nop
      nop
   # queue full: flush
   brr  -, r:flush_clerp_q # todo: could move add_clerp to just before flush_clerp_q and drop this branch
      nop
      nop
      nop

# Preconditions
#
# x   : stroke->x
# y   : stroke->y
# tx0 : stroke->tx
# ty0 : stroke->ty
# tx1 : r1[14]
# ty1 : r1[15]
#
# Clobbers: r2,5, ra_temp
# + flush_join_q
#
:add_join
   mk 0x3100

   mov r2, rb_scalar
   nop
   nop ; mov r5rep, r2 << rbsi_join_q_n
   sub.setf -, elem_num, rbsi_join_q_n
   add.ifz rb_scalar, r2, 1               # join_q_n += 1
                                          mkp "add_join: index %d : ", "r5"
   sub.setf -, elem_num, r5               # only element <old join_q_n> has Z set

   .assert rs2i_x==0
   mov r5rep, ra_scalar2 ; mov r0, ra_scalar2
   mov.ifz rb_join_q_x, r5
                                          mkp "%f'", "r5"
   nop ; mov r5rep, r0<<rs2i_y
   mov.ifz rb_join_q_y, r5
                                          mkp "%f t0 ", "r5"
   nop ; mov r5rep, r0<<rs2i_tx
   mov.ifz rb_join_q_tx0, r5
                                          mkp "%f'", "r5"
   nop ; mov r5rep, r0<<rs2i_ty
   mov.ifz rb_join_q_ty0, r5
                                          mkp "%f t1 ", "r5"
   nop ; mov r5rep, r1<<14
   mov.ifz rb_join_q_tx1, r5
                                          mkp "%f'", "r5"
   nop ; mov r5rep, r1<<15
   mov.ifz rb_join_q_ty1, r5
                                          mkp "%f\n", "r5"

   # return if queue not full
   nop ; mov  r5rep, ra_scalar << rsi_q_lr
   sub.setf  -, elem_num, rbsi_join_q_n ; mov  ra_temp, r5
   sub.ifz.setf  -, r2, 15
   bra.allnz  -, ra_temp # branch taken if join_q_n != 16
      nop
      nop
      nop
   # queue full: flush
   mov  r0, ra_scalar2
   nop
   nop ; mov  ra_temp, (r0 << rs2i_flush_join_q_lbl) >> 15
   nop
   bra  -, ra_temp
      nop
      nop
      nop


# Preconditions
#
# x   : r0[0]
# y   : r0[7]
# tx  : r0[14]
# ty  : r0[15]
#
# Clobbers: xxx
# + :flush_cap_q_*
#
:add_cap
   mk 0x3200

   mov r2, rb_scalar
   nop
   nop ; mov r5rep, r2 << rbsi_cap_q_n
   sub.setf -, elem_num, rbsi_cap_q_n
   add.ifz rb_scalar, r2, 1               # cap_q_n += 1
                                          mkp "add_cap: index %d : ", "r5"
   sub.setf -, elem_num, r5               # only element <old cap_q_n> has Z set

   mov r5rep, r0
   mov.ifz ra_cap_q_x, r5
                                          mkp "%f'", "r5"
   nop ; mov r5rep, r0<<7
   mov.ifz ra_cap_q_y, r5
                                          mkp "%f t ", "r5"
   nop ; mov r5rep, r0<<14
   mov.ifz ra_cap_q_tx, r5
                                          mkp "%f'", "r5"
   nop ; mov r5rep, r0<<15
   mov.ifz ra_cap_q_ty, r5
                                          mkp "%f\n", "r5"

   # return if queue not full
   nop ; mov  r5rep, ra_scalar << rsi_q_lr
   sub.setf  -, elem_num, rbsi_cap_q_n ; mov  ra_temp, r5
   sub.ifz.setf  -, r2, 15
   bra.allnz  -, ra_temp # branch taken if cap_q_n != 16
      nop
      nop
      nop
   # queue full: flush
   mov  r0, ra_scalar2
   nop
   nop ; mov  ra_temp, (r0 << rs2i_flush_cap_q_lbl) >> 15
   nop
   bra  -, ra_temp
      nop
      nop
      nop

# preserves ra_temp3, ra_temp4
:alloc
   # Preconditions:
   # ra_temp : lr

   mkp "alloc.\n"
   mk 0x5000

   # terminate the current list by writing the 1 word head. Save rsi_alloc_p - 4 in rb_list_tail[4].
   mov  r0, ra_scalar
   mov  r1, -(15 << 16) # only 1 element
   add  r1, r0, r1 ; mov  vw_setup, r0 << rsi_out_vpm_setup
   sub  r0, r0, 4
   mov  -, vw_wait
   mov  vpm, TRI_LIST_HEAD
   nop             ; mov  vw_setup, r1 << rsi_out_vdw_setup_0
   nop             ; mov  vw_addr, r0 << rsi_alloc_p
   sub.setf  -, elem_num, 4
   nop             ; mov.ifz  rb_list_tail, (r0 << rsi_alloc_p) >> 4

:alloc_first
   mk 0x5100

   empty_vpm_fifo # workaround for hw-2497
   mov  -, mutex             ; nop
   mov  vr_setup, vpm_setup(1, 0, h32(15))
   mov  r2, 0xffe
   nop                       ; mov  r5rep, ra_scalar << rsi_tess_i
   sub.setf  -, elem_num, r5 ; nop
   sub  r0, vpm, 2           ; mov  r1, vpm
   .ifset debug
      nop ; mov r3, r1 >> r5
      mkp "alloc: shader_if: 0x%08x\n", "r3", 0
   .endif
   and.ifz.setf  r1, r1, r2  ; mov.ifnz  r0, r1
   brr.allnz  -, r:1f
   # branch delay slots {
   mov  vw_setup, vpm_setup(1, 0, h32(15))
   .assert not (VG_TESS_SUBBLOCK_SIZE & 0x1)
   mov  r3, VG_TESS_SUBBLOCK_SIZE >> 1
   mov  vpm, r0              ; mul24  r1, r1, r3
   # }
   mkp "alloc: no subblocks left. get a new block from the host\n"
   # no subblocks left. get a new block from the host
   mov  vpm, r5              ; nop
   mov  vw_setup, vdw_setup_0(1, 1, dma_h32(15, 0))
   mov  vw_addr, a:vg_tess_shader_if
   mov  -, vw_wait           ; nop
   # vg_tess_shader_if[0]: tess_i
.ifnset VG_TESS_NO_INT
   mov  interrupt, 1         ; nop # prod host
   nop                       ; nop ; bkpt # host will resume us when block has been allocated
.endif
   # vg_tess_shader_if[1]: allocated block info
   sub.setf  -, elem_num, r5 ; nop
   mov  vr_setup, vdr_setup_0(1, 1, dma_h32(15, 0), 0, 8)
   mov  vr_addr, a:vg_tess_shader_if + 4
   mov  -, vr_wait           ; nop
   mov  vr_setup, vpm_setup(1, 0, h32(15))
   sub  r5rep, vpm, 2        ; nop
   mk 0x5101
   add  r1, r5, 2            ; mov.ifz  r0, r5
   and.setf  r1, r1, r2      ; mov  vpm, r0
   brr.allnz  -, r:1f
   # branch delay slots {
   nop                       ; mul24  r1, r1, r3
   nop                       ; mov  r5rep, ra_scalar << rsi_tess_i
   sub.setf  -, elem_num, r5 ; nop
   # }
   # out of memory...
   mkp "out of memory\n"
   and.ifz  r0, r0, 1        ; nop
   mov  vpm, r0              ; mov  r0, ra_scalar
   empty_vpm_fifo # make sure vpm writes are done before we release the mutex
   mov  mutex, 0             ; nop
   # write -1 all over rb_io to let the host know we're done
   nop                       ; mov  vw_setup, r0 << rsi_out_vpm_setup
   nop                       ; mov  vpm, -1
   nop                       ; mov  vw_setup, r0 << rsi_out_vdw_setup_0
   nop                       ; mov  vw_addr, unif
   # zero alloc p/size for next shader
   nop                       ; mov  vw_setup, r0 << rsi_chunks_vpm_setup
   nop                       ; mov  vpm, 0
   # wait for writes, prod host, then end
   empty_vpm_fifo
   mov  -, vw_wait
.ifnset VG_TESS_NO_INT
   mov  interrupt, 1
.endif
   thrend
   nop
   nop
:1
   empty_vpm_fifo # make sure vpm writes are done before we release the mutex
   mov  mutex, 0
   mov  ra_temp2, 20
   # r2: 0xffe
   # ra_temp2: 20
   # alloc p = ((r0 & ~0xfff) + r1)[r5]
   # alloc size = r3 * 2
   sub  r2, -2, r2           ; nop
   # r2: ~0xfff
   and  r0, r0, r2           ; mov  r2, ra_scalar
   add  r0, r0, r1           ; v8subs  r5rep, -16, r5
   # r0[-r5]: alloc p
   add  r3, r3, r3           ; mov  vw_setup, r2 << rsi_out_vpm_setup
   # r3: alloc size
   nop                       ; mov  r5rep, r0 >> r5
   # r5: alloc p
   mov  r0, -(11 << 16) # only 5 elements
   sub.setf  -, elem_num, rsi_alloc_p ; nop
   add  r0, ra_scalar, r0    ; mov  -, vw_wait
   sub.ifz  ra_scalar, r5, ra_temp2 ; mov  vpm, rb_list_tail # alloc_p -= 20 ;
   mk 0x51ff

   # branch delay slots {
   sub.setf  -, elem_num, rsi_alloc_size ; nop
   sub.ifz  ra_scalar, r3, ra_temp2 ; mov  vw_setup, r0 << rsi_out_vdw_setup_0
   sub  vw_addr, r5, ra_temp2      ; nop
   # }

   bra  -, ra_temp
      nop
      nop
      nop

.eval pragma_allow_xor_0(prev_allow_xor_0)
.eval pragma_dont_warn_when_mul_rot_inp_r5(prev_dont_warn_when_mul_rot_inp_r5)
