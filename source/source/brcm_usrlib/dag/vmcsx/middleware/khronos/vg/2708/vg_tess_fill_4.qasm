### ============================================================================
### Copyright (c) 2008-2014, Broadcom Corporation
### All rights reserved.
### Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
### 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
### 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
### 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
### THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
### ============================================================================
### ===========================================================================
###
### FILE DESCRIPTION
### Path fill tessellation user shader.
###
### vg_tess_fill_shader_4.c/h are generated from vg_tess_fill_4.qasm by:
### qasm -mml_c:middleware/khronos/vg/2708/vg_tess_fill_shader_4,VG_TESS_FILL_SHADER_4,vg_tess_fill_shader,annots -tb0 vg_tess_fill_4.qasm >vg_tess_fill_shader_4.c
### and:
### qasm -mml_h:middleware/khronos/vg/2708/vg_tess_fill_shader_4,VG_TESS_FILL_SHADER_4,vg_tess_fill_shader,annots -tb0 vg_tess_fill_4.qasm >vg_tess_fill_shader_4.h
### So don't edit vg_tess_fill_shader_4.c/h directly.
### ===========================================================================

.set prev_allow_xor_0, pragma_allow_xor_0(True)
.set prev_dont_warn_when_mul_rot_inp_r5, pragma_dont_warn_when_mul_rot_inp_r5(True)

.include "vg_geomd_4.qinc"
.include "vg_tess_4.qinc"
.include "vg_util_4.qinc"

.set LEVEL_MAX, 8 # limit number of cubic segments to 256

.set CF_LEFT,   1 << 0
.set CF_BOTTOM, 1 << 1
.set CF_RIGHT,  1 << 2
.set CF_TOP,    1 << 3

# the clip "zones" are numbered:
# 7     6     5
#    +-----+
# 0  |     |  4
#    |     |
#    +-----+
# 1     2     3

# the accumulators (r0 through r5) are temps, and along with r*_temp*, unless
# explicitly noted otherwise, are not preserved across function calls

# register file a usage
# usage of ra_even() is to workaround hw-2780
.set remaining_ras, ras()
.set ra_scalar, ra_even()
.set rsi_tess_i, 2
.set rsi_chunks_a, 3
.set rsi_chunks_b, 4
.set rsi_chunks_m_size, 5
.set rsi_t, 6
.set rsi_chunks_vdr_setup_0, 7
.set rsi_chunks_vpm_setup, 8
.set rsi_clip_q_n, 0
.set rsi_subpath_n, 9 # actually, -4 * subpath n
.set rsi_out_vpm_setup, 10
.set rsi_out_vdw_setup_0, 11
.set rsi_alloc_p, 12
.set rsi_alloc_size, 13
.set rsi_clip_lr, 15 # currently clipping ? return address : entry point
.set rsi_chunk_lr, 1
.set ra_user_to_surface_clip, ra()
.set ra_chunk, ra_even()
.set ra_cubic_lrs, [ra_even() for i in range(LEVEL_MAX)] # could probably get rid of ra_cubic_lrs[0]
.set ra_clip_q_x, ra()
.set ra_clip_q_y, ra()
.set ra_clip_exit_edge, ra() # actually, exit edge + 1
.set ra_clip_zone_delta, ra()
.set ra_clip_prev_cf, ra()
.set ra_subpath_sum_x, ra()
.set ra_subpath_sum_y, ra()
.set ra_min_x, ra()
.set ra_max_x, ra()
.set ra_min_y, ra()
.set ra_max_y, ra()
.set ra_subd_flags, ra()
.set ra_temp, ra_even()
.set ra_temp2, ra()
.set ra_temp3, ra_even()
.set ra_temp4, ra()

# register file b usage
.set remaining_rbs, rbs()
.set rb_chunk, rb()
.set rb_cubics, [rb() for i in range(LEVEL_MAX)] # don't really need rb_cubics[LEVEL_MAX - 1]
.set rb_clip_prev_x, rb()
.set rb_clip_prev_y, rb()
.set rb_clip_prev_zone, rb()
.set rb_list_head, rb()
.set rb_list_tail, rb()
.set rb_0xffff, rb()
.set rb_0x10000, rb()
.set rb_76, rb()

# vpm usage (note that this is the same as for stroking)
# 0, 1:       qpu 0 read
# 2, 3, 4:    qpu 0 write
# 5, 6:       qpu 4 read
# 7, 8, 9:    qpu 4 write
# 10, 11:     qpu 8 read
# 12, 13, 14: qpu 8 write
# 15:         1st common row
# 16, 17:     qpu 1 read
# 18, 19, 20: qpu 1 write
# 21, 22:     qpu 5 read
# 23, 24, 25: qpu 5 write
# 26, 27:     qpu 9 read
# 28, 29, 30: qpu 9 write
# 31:         2nd common row
# 32, 33:     qpu 2 read
# 34, 35, 36: qpu 2 write
# 37, 38:     qpu 6 read
# 39, 40, 41: qpu 6 write
# 42, 43:     qpu 10 read
# 44, 45, 46: qpu 10 write
# 47:         unused
# 48, 49:     qpu 3 read
# 50, 51, 52: qpu 3 write
# 53, 54:     qpu 7 read
# 55, 56, 57: qpu 7 write
# 58, 59:     qpu 11 read
# 60, 61, 62: qpu 11 write
# 63:         unused

# the 1st common vpm row is used to store an allocation pointer and number of
# subblocks remaining for each tessellation index. the ith element is used for
# tessellation index i. each element has the format:
# bit 0: flipbit
# bits 1 through 11: subblocks remaining
# bits 12 through 31: (pointer to start of block) >> 12 (block pointers are 4k aligned)

# the 2nd common vpm row is used to help in remembering remaining subblock space
# across shaders. if element i (j) is in [0, 16), and qpu i isn't running a
# fill/stroke shader, then qpu i's first read vpm row has a pointer and size in
# the first 2 elements valid for tessellation index j

::vg_tess_fill_shader

.ifset VG_TESS_RTL_SIM
   # we've permitted the assembler to convert movs of 0 to xors of r0. if r0
   # contains x, this won't work. so put something (anything) in r0
   mov  r0, 0xdeadbeef

   # using registers with x elements can cause issues. registers we do
   # conditional writes to might be used with some unwritten elements. clear
   # them down now to make sure those unwritten elements won't be xs
   mov  ra_clip_q_x, 0
   mov  ra_clip_q_y, 0
.endif

.ifset VG_TESS_SEMA
   sacq  -, 0
   srel  -, 0
.endif

   # this setup stuff is only run once per path, so i've tried to keep it simple
   # and straight-forward instead of making it fast

   # figure out where our vpm space is
   mov  r0, qpu_num ; mov  ra_scalar, 0  @geomd_open # set all scalar values to 0
   and  r1, r0, 3   ; mov  r2, 3
   shr  r0, r0, 2   ; v8adds  r2, r2, 2
   shl  r1, r1, 4   ; mul24  r0, r0, r2
   add  r0, r0, r1

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
   add  vr_setup, r1, r2
   mov  vr_addr, unif ; mov  unif_addr_rel, -1 # read unif but don't advance pointer (will need addr again at end)

   # set out_vpm_setup
   mov  r1, vpm_setup(1, 0, h32(2))
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
   mov  r0, vpm  @geomd_i(geomd_i_clip_inner) @geomd_region_set_a(11) @geomd_i(geomd_i_fill_pre_center)

   # io chunk (r0) looks like this:
   # [0] tess_i
   # [1] chunks_a
   # [2] chunks_b
   # [3] chunks_size
   # [4] t
   # [5] user_to_surface[0]
   # [11] clip[0]
   # [15] flipbit

   # load initial chunks
   # setup stuff that depends on whether or not we're doing interpolation
   mov.setf  -, r0 ; mov  r1, ra_scalar
   sub.ifz.setf  -, elem_num, 2
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

   # user_to_surface/clip
   nop ; mov  ra_user_to_surface_clip, r0 << 5

   # initial clip entry point...
   sub.setf  -, elem_num, rsi_clip_lr
   mov.ifz  ra_scalar, a:clip_subpath_first

   # bbox
   mov  ra_min_x, 0x7fffffff
   mov  ra_max_x, 0x80000000
   mov  ra_min_y, 0x7fffffff
   mov  ra_max_y, 0x80000000

   # magic numbers
   mov  ra_subd_flags, 2 * [-2, -2, -1, -1, 0, 0, 1, 1]
   mov  rb_0xffff, 0xffff
   mov  rb_0x10000, 0x10000
   mov  rb_76, 76

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

   # we need at least 84 bytes of space to start at rsi_alloc_p:
   # - 8 for the first list's tail
   # - 64 for at least 1 batch of vertices in the first list
   # - 12 for the first list's head
   # if we don't have 84 bytes, we'll have to alloc
   mov.setf  -, neg_elems(elems(rsi_alloc_size))
   mov  r0, 84
   sub.ifn.setf  -, ra_scalar, r0 ; nop
   mov  ra_temp, a:1f # alloc will return here
   brr.anyn  -, r:alloc_first
   # branch delay slots {
   mov  rb_list_tail, 0xbfff0000
   mov.setf  -, elem_num ; nop
   mov.ifnz  rb_list_tail, 18 # return
   # }

   # we have at least 84 bytes at rsi_alloc_p. write out the tail of the first
   # list
   sub  r0, ra_scalar, 8   ; mov  r1, ra_scalar
   mov  r2, -(14 << 16) # only 2 elements
   add  r1, r1, r2         ; mov  vw_setup, r1 << rsi_out_vpm_setup
   nop                     ; mov  vpm, rb_list_tail
   nop                     ; mov  vw_setup, r1 << rsi_out_vdw_setup_0
   mov.setf  -, elems(rsi_alloc_p, rsi_alloc_size)
   mov.ifnz  ra_scalar, r0 ; mov  vw_addr, r0 << rsi_alloc_p
:1

   # reset subpath stuff + setup list head/tail
   sub.setf  -, elem_num, rsi_chunks_b ; mov  ra_subpath_sum_x, f(0.0)
   mov.ifz.setf  -, ra_scalar          ; mov  ra_subpath_sum_y, f(0.0)
   brr.anyz  -, r:load
   # branch delay slots {
   mov.setf  -, elem_num               ; nop
   mov.ifnz  rb_list_tail, 0x10010101 # branch
   mov  rb_list_head, 0x162a0101 # cont flag set
   # }
   # fall through to load_interp...

:load_interp

   mov  r0, ra_scalar        ; nop
   mov  -, vr_wait           ; mov  r1, rsi_chunks_m_size
   sub.setf  -, elem_num, r1 ; mov  vr_setup, r0 << rsi_chunks_vpm_setup
   mov  r1, VG_TESS_CHUNK_NORMAL_SIZE
   add.ifz.setf  -, r0, r1   ; mov  r5rep, r0 << rsi_t
   # anyz: last chunk
   fsub  r3, f(1.0), r5      ; mov  r2, r5
   # r2: t, r3: 1 - t
   mov  r5rep, vpm           ; fmul  r3, r3, vpm
   brr.anyz  -, r:chunk_normal
   # branch delay slots {
   mov  ra_chunk, r5         ; fmul  r2, r2, vpm
   fadd  r2, r2, r3          ; mov.ifz  ra_scalar, 0
   mov.setf  -, ra_chunk.8a  ; mov  r3, r0 << rsi_chunks_vdr_setup_0
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
   shl.setf  -, r5, r3       ; mov  ra_chunk, r5
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

:chunk_normal

   # ra_chunk.8a: n
   # ra_chunk.8b: subpath_last_implicit
   # r2: garbage, x0, ..., x6, y0, ..., y6 (not transformed by user->surface yet)

   mov  r0, ra_user_to_surface_clip ; nop
   mov  ra_temp, a:end_subpath
   mov.setf  -, ra_chunk.8b    ; mov  r5rep, r0 << 3
   mov.ifz  ra_temp, ra_scalar ; fmul  r1 >> 1, r2, r5
   nop                         ; mov  r5rep, r0 << 4
   nop                         ; fmul  r3 >> 8, r2, r5
   fadd  r1, r1, r3            ; mov  r5rep, r0 << 5
   fadd  r1, r1, r5            ; mov  r5rep, r0 << 1
   mov  r5rep, r0 << 0         ; fmul  r3 >> 8, r2, r5
   mov.setf  -, elem_num       ; fmul  r2 >> 1, r2, r5
   fadd  r0, r2, r3            ; mov  r5rep, r0 << 2
   fadd  r0, r0, r5            ; mov  r5rep, ra_temp << rsi_chunk_lr
   # r0: x0, ..., x6 (transformed by user->surface)
   # r1: y0, ..., y6 (transformed by user->surface)
   mov  ra_temp, r5            ; mov  r5rep, ra_scalar << rsi_clip_q_n
   # ra_temp: go here when finished with chunk
   # r5: queue n
   add  r2, r5, ra_chunk.8a    ; v8adds  r3, 8, 8
   # r2: queue n + chunk n
   .assert rsi_clip_q_n == 0
   sub.setf  r2, r2, r3        ; v8min.ifz  ra_scalar, r2, r3
   bra.alln  -, ra_temp
   # branch delay slots {
   sub.setf  -, elem_num, r5   ; mov  r3, 15
   nop                         ; mov.ifnn  ra_clip_q_x, r0 >> r5
   sub.setf  -, elem_num, r3   ; mov.ifnn  ra_clip_q_y, r1 >> r5
   # }
   # queue full...
   bra  ra_temp2, (ra_scalar << rsi_clip_lr) >> 15
   # branch delay slots {
   mov.ifz  rb_chunk, r2       ; mov.ifnz  ra_chunk, r0 >> r5
   mov.ifz  ra_chunk, ra_temp  ; mov.ifnz  rb_chunk, r1 >> r5
   .assert rsi_clip_lr == 15
   mov.ifz  ra_scalar, ra_temp2 ; nop
   # }
   mov  r0, rb_chunk           ; mov  ra_clip_q_x, ra_chunk
   bra  -, ra_chunk
   # branch delay slots {
   mov.setf  -, elem_num       ; mov  ra_clip_q_y, rb_chunk
   .assert rsi_clip_q_n == 0
   nop                         ; mov.ifz  ra_scalar, r0 << 15
   nop                         ; nop # don't want to write to ra_scalar on last instruction
   # }

:chunk_cubic

   # ra_chunk.8b: subpath_first
   # r2: garbage, x0, y0, x1, y1, x2, y2, x3, y3 (not transformed by user->surface yet)

   mov  r0, ra_user_to_surface_clip ; nop
   and.setf  -, elem_num, 1  ; nop
   nop                       ; mov  r5rep, r0 << 3
   nop                       ; fmul  r1 >> 0, r2, r5
   nop                       ; mov  r5rep, r0 << 4
   nop                       ; fmul  r3 >> 1, r2, r5
   nop                       ; mov  r5rep, r0 << 1
   mov  r5rep, r0 << 0       ; fmul.ifz  r1 >> 2, r2, r5
   nop                       ; fmul.ifz  r3 >> 1, r2, r5
   fadd  r1, r1, r3          ; mov  r5rep, r0 << 2
   fadd.ifz  r1, r1, r5      ; mov  r5rep, r0 << 5
   fadd.ifnz  r1, r1, r5     ; mov  r5rep, ra_scalar << rsi_clip_q_n
   # r1: x0, y0, x1, y1, x2, y2, x3, y3 (transformed by user->surface)
   mov.setf  -, ra_chunk.8b  ; mov  r0, -8
   brr.allz  -, r:3f # if not subpath first, go straight to cubic subdivision
   # branch delay slots {
   add.setf  -, elem_num, r0 ; mov  rb_chunk, r1 << 6 # save last vert for later
   mov.setf  -, elem_num     ; mov.ifnn  r1, r1 >> 8
   mov  ra_cubic_lrs[0], a:1f
   # }
   # subpath first -- write first vert
   .assert rsi_clip_q_n == 0
   sub.setf  -, r5, elem_num ; v8adds.ifz  ra_scalar, r5, 1
   brr.anyn  -, r:3f
   # branch delay slots {
   nop                       ; mov  r0, r1 << 1
   nop                       ; mov.ifz  ra_clip_q_x, r1 >> r5
   nop                       ; mov.ifz  ra_clip_q_y, r0 >> r5
   # }
   bra  -, (ra_scalar << rsi_clip_lr) >> 15
   # branch delay slots {
   sub.setf  -, elem_num, rsi_clip_lr ; mov  ra_chunk, r1
   mov.ifz  ra_scalar, a:2f
   nop                       ; nop
   # }
:1
   # write last vert
   nop                       ; mov  r5rep, ra_scalar << rsi_chunk_lr
   mov  ra_temp, r5          ; mov  r5rep, ra_scalar << rsi_clip_q_n
   mov.setf  -, elem_num     ; mov  r0, rb_chunk
   .assert rsi_clip_q_n == 0
   sub.setf  -, r5, elem_num ; v8adds.ifz  ra_scalar, r5, 1
   bra.anyn  -, ra_temp
   # branch delay slots {
   nop                       ; mov  r1, r0 << 1
   nop                       ; mov.ifz  ra_clip_q_x, r0 >> r5
   nop                       ; mov.ifz  ra_clip_q_y, r1 >> r5
   # }
   bra  -, (ra_scalar << rsi_clip_lr) >> 15
   # branch delay slots {
   sub.setf  -, elem_num, rsi_clip_lr ; nop
   mov.ifz  ra_scalar, ra_temp ; nop
   nop                       ; nop
   # }
:2
   mov  r1, ra_chunk         ; nop
   nop                       ; nop
:3
.rep i, LEVEL_MAX
   # r1: x0, y0, x1, y1, x2, y2, x3, y3 (twice)
   mov  rb_cubics[i], r1       ; mov  r0, r1 >> 2
   # r0: x3, y3, x0, y0, x1, y1, x2, y2 (twice)
   # r1: x0, y0, x1, y1, x2, y2, x3, y3 (twice)
   fsub  r2, r0, r1            ; mov  r5rep, f(VG_TESS_OO_FLATNESS)
   # r0: x3, y3, x0, y0, x1, y1, x2, y2 (twice)
   # r1: x0, y0, x1, y1, x2, y2, x3, y3 (twice)
   # r2: (x3 - x0), (y3 - y0), (x0 - x1), (y0 - y1), (x1 - x2), (y1 - y2), (x2 - x3), (y2 - y3) (twice)
   # r5: 1/flatness
   fadd  r0, r0, r1            ; fmul  ra_temp, r2, r2
   # r0: garbage[2], (x0 + x1), (y0 + y1), (x1 + x2), (y1 + y2), (x2 + x3), (y2 + y3) (twice)
   # r2: (x3 - x0), (y3 - y0), (x0 - x1), (y0 - y1), (x1 - x2), (y1 - y2), (x2 - x3), (y2 - y3) (twice)
   # r5: 1/flatness
   # ra_temp: (x3 - x0)^2, (y3 - y0)^2
   mov  ra_temp2, r0           ; fmul  r1, r2 << 1, r5 << 1
   # r0: garbage[2], (x0 + x1), (y0 + y1), (x1 + x2), (y1 + y2), (x2 + x3), (y2 + y3) (twice)
   # r1: garbage, (x0 - x1)/flatness, garbage[5], (x3 - x0)/flatness
   # r2: (x3 - x0), (y3 - y0), (x0 - x1), (y0 - y1), (x1 - x2), (y1 - y2), (x2 - x3), (y2 - y3) (twice)
   # r5: 1/flatness
   # ra_temp: (x3 - x0)^2, (y3 - y0)^2
   # ra_temp2: garbage[2], (x0 + x1), (y0 + y1), (x1 + x2), (y1 + y2), (x2 + x3), (y2 + y3) (twice)
   nop                         ; fmul  r1, r1, r2
   # r0: garbage[2], (x0 + x1), (y0 + y1), (x1 + x2), (y1 + y2), (x2 + x3), (y2 + y3) (twice)
   # r1: garbage, (y3 - y0)(x0 - x1)/flatness, garbage[5], (y2 - y3)(x3 - x0)/flatness
   # r2: (x3 - x0), (y3 - y0), (x0 - x1), (y0 - y1), (x1 - x2), (y1 - y2), (x2 - x3), (y2 - y3) (twice)
   # r5: 1/flatness
   # ra_temp: (x3 - x0)^2, (y3 - y0)^2
   # ra_temp2: garbage[2], (x0 + x1), (y0 + y1), (x1 + x2), (y1 + y2), (x2 + x3), (y2 + y3) (twice)
   nop                         ; mov  r3, r0 >> 2
   # r0: garbage[4], (x1 + x2), (y1 + y2), (x2 + x3), (y2 + y3) (twice)
   # r1: garbage, (y3 - y0)(x0 - x1)/flatness, garbage[5], (y2 - y3)(x3 - x0)/flatness
   # r2: (x3 - x0), (y3 - y0), (x0 - x1), (y0 - y1), (x1 - x2), (y1 - y2), (x2 - x3), (y2 - y3) (twice)
   # r3: garbage[4], (x0 + x1), (y0 + y1), (x1 + x2), (y1 + y2) (twice)
   # r5: 1/flatness
   # ra_temp: (x3 - x0)^2, (y3 - y0)^2
   # ra_temp2: garbage[2], (x0 + x1), (y0 + y1), (x1 + x2), (y1 + y2), (x2 + x3), (y2 + y3) (twice)
   fadd  ra_temp3, r0, r3      ; fmul  r0, r2 << 3, r5 << 3
   # r0: (y0 - y1)/flatness, garbage[5], (y3 - y0)/flatness
   # r1: garbage, (y3 - y0)(x0 - x1)/flatness, garbage[5], (y2 - y3)(x3 - x0)/flatness
   # r2: (x3 - x0), (y3 - y0), (x0 - x1), (y0 - y1), (x1 - x2), (y1 - y2), (x2 - x3), (y2 - y3) (twice)
   # ra_temp: (x3 - x0)^2, (y3 - y0)^2
   # ra_temp2: garbage[2], (x0 + x1), (y0 + y1), (x1 + x2), (y1 + y2), (x2 + x3), (y2 + y3) (twice)
   # ra_temp3: garbage[4], (x0 + 2x1 + x2), (y0 + 2y1 + y2), (x1 + 2x2 + x3), (y1 + 2y2 + y3) (twice)
   mov  r5rep, ra_temp         ; mov  r3, ra_temp << 1   @mul_used(0)
   # r0: (y0 - y1)/flatness, garbage[5], (y3 - y0)/flatness
   # r1: garbage, (y3 - y0)(x0 - x1)/flatness, garbage[5], (y2 - y3)(x3 - x0)/flatness
   # r2: (x3 - x0), (y3 - y0), (x0 - x1), (y0 - y1), (x1 - x2), (y1 - y2), (x2 - x3), (y2 - y3) (twice)
   # r3: (y3 - y0)^2
   # r5: (x3 - x0)^2
   # ra_temp2: garbage[2], (x0 + x1), (y0 + y1), (x1 + x2), (y1 + y2), (x2 + x3), (y2 + y3) (twice)
   # ra_temp3: garbage[4], (x0 + 2x1 + x2), (y0 + 2y1 + y2), (x1 + 2x2 + x3), (y1 + 2y2 + y3) (twice)
   fadd  r5rep, r3, r5         ; fmul  r0 << 1, r0, r2
   # r0: garbage, (x3 - x0)(y0 - y1)/flatness, garbage[5], (x2 - x3)(y3 - y0)/flatness
   # r1: garbage, (y3 - y0)(x0 - x1)/flatness, garbage[5], (y2 - y3)(x3 - x0)/flatness
   # r5: (x3 - x0)^2 + (y3 - y0)^2
   # ra_temp2: garbage[2], (x0 + x1), (y0 + y1), (x1 + x2), (y1 + y2), (x2 + x3), (y2 + y3) (twice)
   # ra_temp3: garbage[4], (x0 + 2x1 + x2), (y0 + 2y1 + y2), (x1 + 2x2 + x3), (y1 + 2y2 + y3) (twice)
   fsub  r0, r1, r0            ; mov  r1, ra_temp3 >> 2  @mul_used(6, 7, 14, 15)
   # r0: garbage, ((y3 - y0)(x0 - x1) - (x3 - x0)(y0 - y1))/flatness, garbage[5], ((y2 - y3)(x3 - x0) - (x2 - x3)(y3 - y0))/flatness
   # r1: garbage[6], (x0 + 2x1 + x2), (y0 + 2y1 + y2) (twice)
   # r5: (x3 - x0)^2 + (y3 - y0)^2
   # ra_temp2: garbage[2], (x0 + x1), (y0 + y1), (x1 + x2), (y1 + y2), (x2 + x3), (y2 + y3) (twice)
   # ra_temp3: garbage[4], (x0 + 2x1 + x2), (y0 + 2y1 + y2), (x1 + 2x2 + x3), (y1 + 2y2 + y3) (twice)
   fadd  r1, ra_temp3, r1      ; fmul  r0, r0, r0
   # r0: garbage, ((y3 - y0)(x0 - x1) - (x3 - x0)(y0 - y1))^2/flatness^2, garbage[5], ((y2 - y3)(x3 - x0) - (x2 - x3)(y3 - y0))^2/flatness^2
   # r1: garbage[6], (x0 + 3x1 + 3x2 + x3), (y0 + 3y1 + 3y2 + y3) (twice)
   # r5: (x3 - x0)^2 + (y3 - y0)^2
   # ra_temp2: garbage[2], (x0 + x1), (y0 + y1), (x1 + x2), (y1 + y2), (x2 + x3), (y2 + y3) (twice)
   # ra_temp3: garbage[4], (x0 + 2x1 + x2), (y0 + 2y1 + y2), (x1 + 2x2 + x3), (y1 + 2y2 + y3) (twice)
   fsub.setf  -, r0, r5        ; fmul  r1, r1, f(0.125)
   mov.ifnn.setf  -, [-1, 0, -1, -1, -1, -1, -1, 0] + (8 * [-1])
   bra.alln  -, ra_cubic_lrs[i] # if already flat enough, just return
.if i == (LEVEL_MAX - 1)
   # don't subdivide any further. just write vert if not flat enough
   # branch delay slots {
   mov  r5rep, ra_scalar << rsi_clip_q_n ; mov  r0, r1 << 6
   mov.setf  -, elem_num       ; mov  r1, r1 << 7
   nop                         ; nop
   # }
   .assert rsi_clip_q_n == 0
   sub.setf  -, r5, elem_num   ; v8adds.ifz  ra_scalar, r5, 1
   bra.anyn  -, ra_cubic_lrs[i]
   # branch delay slots {
   nop                         ; mov.ifz  ra_clip_q_x, r0 >> r5
   nop                         ; mov.ifz  ra_clip_q_y, r1 >> r5
   nop                         ; nop
   # }
   bra  -, (ra_scalar << rsi_clip_lr) >> 15
   # branch delay slots {
   sub.setf  -, elem_num, rsi_clip_lr ; nop
   mov.ifz  ra_scalar, ra_cubic_lrs[i] ; nop
   nop                         ; nop
   # }
.else
   # r1: garbage[6], (x0 + 3x1 + 3x2 + x3)/8, (y0 + 3y1 + 3y2 + y3)/8 (twice)
   # ra_temp2: garbage[2], (x0 + x1), (y0 + y1), (x1 + x2), (y1 + y2), (x2 + x3), (y2 + y3) (twice)
   # ra_temp3: garbage[4], (x0 + 2x1 + x2), (y0 + 2y1 + y2), (x1 + 2x2 + x3), (y1 + 2y2 + y3) (twice)
   # rb_cubics[i]: x0, y0, garbage[4], x3, y3 (twice)
   # branch delay slots {
   max.setf  -, -1, ra_subd_flags ; nop
   # flags:
   # - - - - z z - - (twice)
   # n n n n - - - - (twice)
   # c c - - - - - - (twice)
   mov  r2, ra_temp2           ; fmul.ifn  r1, ra_temp2, f(0.5)
   mov  r3, ra_temp3           ; mov.ifc  r1, rb_cubics[i]
   # }
   mov  r0, f(0.25)            ; fmul.ifz  r1, ra_temp3, f(0.25)
   brr  ra_cubic_lrs[i + 1], r:1f
   # branch delay slots {
   fadd  r0, r0, r0            ; fmul.ifn  rb_cubics[i], r3 << 4, r0 << 4
   nop                         ; mov.ifc  rb_cubics[i], r1 << 6
   nop                         ; fmul.ifz  rb_cubics[i], r2 << 2, r0 << 2
   # }
   nop                         ; mov  r5rep, ra_scalar << rsi_clip_q_n
   mov.setf  -, elem_num       ; mov  r1, rb_cubics[i]
   .assert rsi_clip_q_n == 0
   sub.setf  -, r5, elem_num   ; v8adds.ifz  ra_scalar, r5, 1
   brr.anyn  -, r:1f
   # branch delay slots {
   mov  ra_cubic_lrs[i + 1], ra_cubic_lrs[i] ; mov  r0, r1 << 1
   nop                         ; mov.ifz  ra_clip_q_x, r1 >> r5
   nop                         ; mov.ifz  ra_clip_q_y, r0 >> r5
   # }
   bra  ra_temp, (ra_scalar << rsi_clip_lr) >> 15
   # branch delay slots {
   sub.setf  -, elem_num, rsi_clip_lr ; mov  ra_chunk, r1
   mov.ifz  ra_scalar, ra_temp ; nop
   nop                         ; nop
   # }
   mov  r1, ra_chunk           ; nop
   nop                         ; nop
:1
   # next level...
.endif
.endr

# the clipper acts sort of like a coroutine with the rest of the code. when we
# have a full batch of vertices to clip, we branch to ra_scalar[rsi_clip_lr],
# which will take us to:
# - clip_subpath_first, if the vertices are the first of the current subpath
# - clip_inside, if the last vertex to be clipped was inside the clip rect
# - clip_outside, if the last vertex to be clipped was outside the clip rect
# we also write the return address to ra_scalar[rsi_clip_lr]. when the clipper
# has finished with the batch, it branches back to ra_scalar[rsi_clip_lr] and
# puts its return address there

# the clipper processes an entire batch of 16 vertices (relatively) efficiently
# when none of the implied lines cross the clip rect. when we do cross the clip
# rect, we efficiently process up to the crossing point, process the crossing
# point on its own, then resume in the new state with a smaller batch
# (everything up to and including the crossing point having been removed). so
# crossing the clip rect is rather expensive; the hope is that this will be
# relatively rare...

:clip_subpath_first

   # if the first point isn't inside the clip rect, to simplify the algorithm,
   # we pretend there was a preceding point just inside the clip rect. we don't
   # actually need to add it, as it will be in the same place as the exit point
   # that clip_inside_to_outside will add. all we need to do is set clip_prev.
   # this is harmless if the first point is inside the clip rect, so we don't
   # need to check...

   mov  r0, ra_user_to_surface_clip ; nop
   nop                          ; nop
   nop                          ; mov  r1, r0 << 6
   fmax  r2, ra_clip_q_x, r1    ; mov  r1, r0 << 8
   fmin  rb_clip_prev_x, r2, r1 ; mov  r1, r0 << 7
   fmax  r2, ra_clip_q_y, r1    ; mov  r1, r0 << 9
   fmin  rb_clip_prev_y, r2, r1 ; nop
   # fall through to clip_inside...

:clip_inside

   # if the first point isn't inside the clip rect, rb_clip_prev_x/y should be
   # valid and inside the clip rect

   mov  r0, ra_user_to_surface_clip   ; nop
   mov  ra_temp, a:clip_inside
   mov  r1, ra_scalar                 ; mov  r5rep, r0 << 6
   fsub.setf  -, ra_clip_q_x, r5      ; mov  r5rep, r0 << 7
   fsub.ifnn.setf  -, ra_clip_q_y, r5 ; mov  r5rep, r0 << 8
   fsub.ifnn.setf  -, r5, ra_clip_q_x ; mov  r5rep, r0 << 9
   fsub.ifnn.setf  -, r5, ra_clip_q_y ; mov  r5rep, r1 << rsi_clip_q_n
   sub.ifn.setf  -, elem_num, r5      ; nop
   brr.anyn  -, r:clip_inside_to_outside
   # branch delay slots {
   nop                                ; mov  r1, ra_clip_q_x
   sub  r5rep, -15, r5                ; mov  r2, ra_clip_q_y
   mov  r0, rsi_clip_lr               ; mov  ra_temp3, (ra_scalar << rsi_clip_lr) >> 15
   # }
   sub.setf  -, elem_num, r0          ; mov  rb_clip_prev_x, r1 >> r5
   mov.ifz  ra_scalar, ra_temp        ; mov  rb_clip_prev_y, r2 >> r5
   sub  r5rep, -15, r5                ; nop
   # fall through to clip_out...

:clip_out

   # output r5 coords from ra_clip_q_x/y, subtract r5 from
   # ra_scalar[rsi_clip_q_n], and then return to ra_temp3

   # writing everything out in reverse makes a number of things simpler as stuff
   # tends to be available at the right time, avoiding the need to either save
   # it for later (if we get it too early) or go back and fill it in (if we get
   # it too late), eg:
   # - the fan center can't be calculated until we've seen all the vertices.
   #   when writing out in reverse, the last thing we write in each fan is the
   #   center, but it's the first thing when writing out forward
   # - generating linked lists in reverse is easier
   # none of these things would really be problematic, but i figure the
   # simplifications are worth the minor inefficiency of having to reverse the
   # output vector here. we do the reversing here rather than writing the queue
   # in reverse as:
   # - for normal chunks, we'd have to reverse the input chunk, which would suck
   #   more
   # - for cubic chunks, the cost of the reverse will be drowned by the cost of
   #   the cubic subdivision

   mov  r1, f(16.0)                ; fmul  r0, ra_clip_q_x, f(16.0)
   fadd.setf  r2, r0, f(0.5)       ; mov  r3, ra_scalar
   fsub.ifn  r2, r2, f(1.0)        ; fmul  r1, ra_clip_q_y, r1
   ftoi  r2, r2                    ; mov  vw_setup, r3 << rsi_out_vpm_setup
   fadd.setf  r3, r1, f(0.5)       ; nop
   fsub.ifn  r3, r3, f(1.0)        ; nop
   ftoi  r3, r3                    ; nop
   sub.setf  -, elem_num, r5       ; nop
   fadd.ifn  ra_subpath_sum_x, ra_subpath_sum_x, r0 ; v8min  r0, r2, rb_0xffff
   fadd.ifn  ra_subpath_sum_y, ra_subpath_sum_y, r1 ; mul24  r1, r3, rb_0x10000
   min.ifn  ra_min_x, ra_min_x, r2 ; v8max.ifn  -, r0, r1  @preserve_cond @geomd_verts_m(4) @geomd_tris_add # todo: i would prefer not to use an instruction slot just for this
   max  r0, ra_max_x, r2           ; v8max  ra_temp, r0 >> 3, r1 >> 3
   min.ifn  ra_min_y, ra_min_y, r3 ; mov  -, vw_wait
   .pushnwarn # non-accum rotation warning: we want the weird semantics here!
   mov.ifn  ra_max_x, r0           ; mov  r0, ra_temp << 2
   .popnwarn
   and.setf  -, elem_num, 4        ; nop
   nop                             ; mov.ifnz  r0, r0 << 8
   # need 76 bytes:
   # - 64 for this batch of vertices
   # - 12 for the list's head
   mov.setf  -, neg_elems(elems(rsi_alloc_size))
   sub.ifn.setf  -, ra_scalar, rb_76 ; nop
   brr.anyn  ra_temp, r:alloc
   # branch delay slots {
   and.setf  -, elem_num, 1        ; mov  ra_temp4, r5
   sub.setf  -, elem_num, r5       ; mov.ifnz  r0, r0 << 2
   max.ifn  ra_max_y, ra_max_y, r3 ; mov  vpm, r0
   # }
   # now have at least 76 bytes at rsi_alloc_p
   mov.setf  -, elem_num           ; mul24  r1, 8, 8
   sub  r1, ra_scalar, r1          ; mov  r0, ra_scalar
   .assert rsi_clip_q_n == 0
   sub.ifz  ra_scalar, r0, ra_temp4 ; mul24  r2, ra_temp4, 4
   bra  -, ra_temp3
   # branch delay slots {
   mov.setf  -, elems(rsi_subpath_n, rsi_alloc_p, rsi_alloc_size)
   sub.ifnz  ra_scalar, r0, r2     ; mov  vw_setup, r0 << rsi_out_vdw_setup_0 # todo: potentially write more than necessary here
   nop                             ; mov  vw_addr, r1 << rsi_alloc_p
   # }

:clip_inside_to_outside

   # we find two things in parallel here:
   # - the index of the first outside point
   # - all exit points and edges

   # there's alot of effectively scalar stuff here (ie instructions where we
   # only use one of the 16 results), but i can't see how to vectorise it
   # without reworking the whole clipper (and making it more complicated). the
   # assumption is that we won't cross the clip rect much...

   # n: outside
   # r1: x
   # r2: y

   add.ifnn  r0, -16, -16             ; v8subs.ifn  r0, -16, elem_num
   # max(r0): -16 - index of first outside point
   mov.setf  -, elem_num              ; mov  ra_temp, 2
   nop                                ; mov.ifnz  rb_clip_prev_x, r1 >> 1
   mov  r3, ra_user_to_surface_clip   ; mov.ifnz  rb_clip_prev_y, r2 >> 1
   nop                                ; mov  r1, r0 << 8
   max  r0, r0, r1                    ; mov  r5rep, r3 << 6
   fsub.setf  -, ra_clip_q_x, r5      ; mov  r2, r5
   mov.ifn  ra_temp, 0                ; mov  r5rep, r3 << 8
   fsub.ifnn.setf  -, r5, ra_clip_q_x ; mov.ifnn  r2, r5
   # ifn {
   #    y += (prev_y - y) * min((edge_x - x) / (prev_x - x), 1)
   #    x = edge_x
   # }
   # r2: edge_x
   # r3: garbage[7], clip min y, garbage, clip max y
   fsub.ifn  recip, rb_clip_prev_x, ra_clip_q_x ; mov.ifn  ra_temp2, r2
   nop                                ; mov  ra_temp3, r3 << 7
   fsub.ifn  r1, r2, ra_clip_q_x      ; mov  r2, r0 << 4
   fsub.ifn  r3, rb_clip_prev_y, ra_clip_q_y ; fmul.ifn  r1, r1, r4
   fmin.ifn  r1, r1, f(1.0)           ; mov.ifnn  ra_temp2, ra_clip_q_x
   max  r0, r0, r2                    ; fmul.ifn  r1, r1, r3
   fadd.ifn  r3, ra_clip_q_y, r1      ; mov.ifnn  r3, ra_clip_q_y
   # r3: y
   # ra_temp2: x
   # ra_temp3: clip min y, garbage, clip max y
   mov  r2, 1                         ; mov  r5rep, ra_temp3
   fsub.setf  -, r3, r5               ; mov  r1, r5
   clz.ifnn  r2, r2                   ; mov  r5rep, ra_temp3 << 2
   fsub.ifnn.setf  -, r5, r3          ; mov.ifnn  r1, r5
   # ifn {
   #    x += (prev_x - x) * min((edge_y - y) / (prev_y - y), 1)
   #    y = edge_y
   # }
   # r1: edge_y
   # r3: y
   # ra_temp2: x
   fsub.ifn  recip, rb_clip_prev_y, r3 ; mov.ifn  ra_temp3, r1
   min.ifn  ra_temp, r2, 3            ; mov  r2, elem_num
   fsub.ifn  r1, r1, r3               ; mov  ra_temp4, r0 << 2
   fsub.ifn  r3, rb_clip_prev_x, ra_temp2 ; fmul.ifn  r1, r1, r4
   fmin.ifn  r1, r1, f(1.0)           ; v8max  r0, r0, ra_temp4
   mov.ifn  r3, ra_temp3              ; fmul.ifn  r1, r1, r3
   fadd.ifn  ra_temp2, ra_temp2, r1   ; mov  r1, r0 << 1
   # r3: exit y
   # ra_temp: exit edge
   # ra_temp2: exit x
   max  r5rep, r0, r1                 ; v8adds  r0, ra_temp, 1
   sub  r2, -16, r2                   ; mov  r1, ra_clip_q_x
   sub.setf  -, r2, r5                ; mov  r2, ra_clip_q_y
   nop                                ; mov  ra_clip_exit_edge, r0 >> r5
   brr  ra_temp3, r:clip_out
   # branch delay slots {
   mov.ifz  ra_clip_q_x, ra_temp2     ; mov  rb_clip_prev_x, r1 >> r5
   mov.ifz  ra_clip_q_y, r3           ; mov  rb_clip_prev_y, r2 >> r5
   sub  r5rep, -15, r5                ; mov  ra_clip_zone_delta, 0
   # }
   # put remaining vertices in ra_clip_q_x/y (rsi_clip_q_n already set)
   # find clip flags and zone of first outside point (now previous point)
   # todo: maybe this could be made more efficient by merging with the enter
   # point stuff above...
   mov  r0, rb_clip_prev_x            ; mov  r2, ra_user_to_surface_clip
   mov  r1, rb_clip_prev_y            ; mov  r3, 0
   not  ra_temp, r3                   ; mov  r5rep, r2 << 8
   fsub.setf  -, r5, r0               ; mov  r5rep, r2 << 6
   fsub.setf  -, r0, r5               ; mov.ifn  r3, CF_RIGHT
   .assert CF_LEFT == 1
   sub.ifn  r3, r3, ra_temp           ; mov  r5rep, r2 << 7
   fsub.setf  -, r1, r5               ; mov  r5rep, r2 << 9
   fsub.setf  -, r5, r1               ; v8adds.ifn  r3, r3, CF_BOTTOM
   or.ifn  r3, r3, CF_TOP             ; mov  r5rep, ra_scalar << rsi_clip_q_n
   # r0: prev x, remaining xs...
   # r1: prev y, remaining ys...
   # r3: prev clip flags
   # r5: remaining n
   mov.setf  -, r5                    ; mov  ra_clip_q_x, r0 << 1
   brr.allnz  -, r:1f
   # branch delay slots {
   sub.setf  -, r3, CF_LEFT | CF_TOP  ; v8adds  ra_temp, r3, r3
   mov  r0, 62
   clz.ifnz  r2, r3                   ; v8subs.ifz  r2, r0, 7
   # }
   # no remaining vertices, return and set next entry point to clip_outside
   shl.ifnz.setf  -, ra_temp, r2      ; v8adds.ifnz  r2, r2, r2
   sub.ifnz  r0, r0, 1                ; mov  ra_clip_prev_cf, r3
   bra  -, (ra_scalar << rsi_clip_lr) >> 15
   # branch delay slots {
   sub.setf  -, elem_num, rsi_clip_lr ; v8subs  rb_clip_prev_zone, r0, r2
   mov.ifz  ra_scalar, a:clip_outside
   nop                                ; nop # don't want to write to ra_scalar on last instruction
   # }
:1
   shl.ifnz.setf  -, ra_temp, r2      ; v8adds.ifnz  r2, r2, r2
   sub.ifnz  r0, r0, 1                ; mov  ra_clip_prev_cf, r3
   sub  rb_clip_prev_zone, r0, r2     ; mov  ra_clip_q_y, r1 << 1
   # fall through to clip_outside...

:clip_outside

   # rb_clip_prev_x/y should be valid and outside the clip rect
   # ra_clip_prev_cf/rb_clip_prev_zone should be valid

   # first thing to do is find the clip flags and zones of the input vertices...
   mov  r0, ra_user_to_surface_clip  ; mov  ra_temp, CF_RIGHT
   mov  r1, 0                        ; nop
   nop                               ; mov  r5rep, r0 << 6
   fsub.setf  -, ra_clip_q_x, r5     ; mov  r5rep, r0 << 8
   fsub.setf  -, r5, ra_clip_q_x     ; mov.ifn  r1, CF_LEFT
   mov.ifn  r1, ra_temp              ; mov  r5rep, r0 << 7
   fsub.setf  -, ra_clip_q_y, r5     ; mov  r5rep, r0 << 9
   fsub.setf  -, r5, ra_clip_q_y     ; v8adds.ifn  r1, r1, CF_BOTTOM
   or.ifn  r1, r1, CF_TOP            ; nop
   # r1: clip flags
   sub.setf  -, r1, CF_LEFT | CF_TOP ; v8adds  r0, r1, r1
   mov  r2, 62
   clz.ifnz  r3, r1                  ; v8subs.ifz  r3, r2, 7
   shl.ifnz.setf  -, r0, r3          ; v8adds.ifnz  r3, r3, r3
   sub.ifnz  r2, r2, 1               ; mov  r0, ra_clip_q_x
   mov.setf  -, elem_num             ; v8subs  r2, r2, r3
   mov  r3, ra_clip_q_y              ; mov.ifnz  rb_clip_prev_x, r0 >> 1
   mov  r0, ra_user_to_surface_clip  ; mov.ifnz  ra_clip_prev_cf, r1 >> 1
   not  r1, r1                       ; mov.ifnz  rb_clip_prev_zone, r2 >> 1
   mov  r3, ra_clip_prev_cf          ; mov.ifnz  rb_clip_prev_y, r3 >> 1
   and.setf  -, r3, ra_temp          ; mov  r5rep, r0 << 6
   mov  ra_temp, r2                  ; mov  r2, r5

   # z: !(prev clip flags & CF_RIGHT)
   # r0: garbage[6], clip min x, clip min y, clip max x, clip max y
   # r1: ~(clip flags)
   # r2: clip min x
   # ra_temp: zone

   # todo: consider adding early outs here, eg if (clip flags & prev clip flags)
   # for all elements

   # clip against left/right edge
   and  r3, r1, ra_clip_prev_cf                  ; mov  r5rep, r0 << 8
   and.setf  ra_temp2, r3, CF_LEFT | CF_RIGHT    ; mov.ifnz  r2, r5
   # ifnz {
   #    cross_cf = edge_cf
   #    prev_y += (y - prev_y) * min((edge_x - prev_x) / (x - prev_x), 1)
   #    prev_x = edge_x
   #    prev_cf = cf(prev_x, prev_y)
   # }
   fsub.ifnz  recip, ra_clip_q_x, rb_clip_prev_x ; mov.ifnz  ra_clip_prev_cf, 0
   nop                                           ; mov  r5rep, r0 << 7
   fsub.ifnz  r2, r2, rb_clip_prev_x             ; mov.ifnz  rb_clip_prev_x, r2
   fsub.ifnz  r3, ra_clip_q_y, rb_clip_prev_y    ; fmul.ifnz  r2, r2, r4
   fmin.ifnz  r2, r2, f(1.0)                     ; nop
   shr.ifnz  ra_temp2, ra_temp2, 1               ; fmul.ifnz  r2, r2, r3
   fadd.ifnz  r2, rb_clip_prev_y, r2             ; mov.ifz  r2, rb_clip_prev_y
   # r2: prev y
   fsub.setf  -, r2, r5                          ; mov  r5rep, r0 << 9
   or.ifn  ra_clip_prev_cf, ra_clip_prev_cf, CF_BOTTOM ; mov  rb_clip_prev_y, r2
   fsub.setf  -, r5, r2                          ; mov  r3, CF_BOTTOM | CF_TOP
   or.ifn  ra_clip_prev_cf, ra_clip_prev_cf, CF_TOP ; mov  r2, r5

   # n: prev clip flags & CF_TOP
   # r2: clip max y

   # clip against bottom/top edge
   and  r3, r1, r3                               ; mov  r5rep, r0 << 7
   and.setf  ra_temp3, r3, ra_clip_prev_cf       ; mov.ifnn  r2, r5
   # ifnz {
   #    cross_cf = edge_cf
   #    prev_x += (x - prev_x) * min((edge_y - prev_y) / (y - prev_y), 1)
   #    prev_y = edge_y
   #    prev_cf = cf(prev_x, prev_y)
   # }
   fsub.ifnz  recip, ra_clip_q_y, rb_clip_prev_y ; mov.ifnz  ra_clip_prev_cf, 0
   nop                                           ; mov  r5rep, r0 << 6
   fsub.ifnz  r2, r2, rb_clip_prev_y             ; mov.ifnz  rb_clip_prev_y, r2
   fsub.ifnz  r3, ra_clip_q_x, rb_clip_prev_x    ; fmul.ifnz  r2, r2, r4
   fmin.ifnz  r2, r2, f(1.0)                     ; nop
   shr.ifnz  ra_temp2, ra_temp3, 1               ; fmul.ifnz  r2, r2, r3
   fadd.ifnz  r2, rb_clip_prev_x, r2             ; mov.ifz  r2, rb_clip_prev_x
   # r2: prev x
   fsub.setf  -, r2, r5                          ; mov  r5rep, r0 << 8
   or.ifn  ra_clip_prev_cf, ra_clip_prev_cf, CF_LEFT ; mov  rb_clip_prev_x, r2
   fsub.setf  -, r5, r2                          ; v8subs  r5rep, ra_scalar << rsi_clip_q_n, 1
   or.ifn  r0, ra_clip_prev_cf, CF_RIGHT         ; mov.ifnn  r0, ra_clip_prev_cf
   v8subs.setf  -, elem_num, r5                  ; v8subs  r1, -1, r1

   # z: valid
   # r0: prev clip flags
   # r1: clip flags
   # r5: clip q n - 1
   # ra_temp: zone
   # ra_temp2: cross_cf >> 1

   and.ifz.setf  ra_temp2, r0, r1                      ; v8min  r2, ra_temp2, 3
   # anyz: some valid point crosses clip rect
   # r2: cross edge
   sub  r0, ra_temp, rb_clip_prev_zone                 ; v8adds  r2, r2, r2
   and  r0, r0, 7                                      ; mov  ra_temp3, r2
   brr.anyz  -, r:clip_outside_to_inside
   # branch delay slots {
   sub.setf  -, r0, 4                                  ; mov  r3, ra_clip_q_y
   sub  r2, ra_temp, r2                                ; v8adds.ifnn  r0, r0, -8
   and.ifz.setf  -, r2, 4                              ; mov  r2, ra_clip_q_x
   # }
   sub.setf  r5rep, elem_num, r5                       ; mov.ifz  r0, 4
   add.ifn  ra_clip_zone_delta, ra_clip_zone_delta, r0 ; nop
   mov  r2, ra_temp                                    ; mov  rb_clip_prev_x, r2 >> r5
   add.ifz  ra_clip_zone_delta, ra_clip_zone_delta, r0 ; mov  rb_clip_prev_y, r3 >> r5
   mov.setf  -, elem_num                               ; mov  ra_clip_prev_cf, r1 >> r5
   bra  -, (ra_scalar << rsi_clip_lr) >> 15
   # branch delay slots {
   .assert rsi_clip_q_n == 0
   sub.setf  -, elem_num, rsi_clip_lr                  ; mov.ifz  ra_scalar, 0
   mov.ifz  ra_scalar, a:clip_outside # don't want to write to ra_scalar on last instruction
   nop                                                 ; mov  rb_clip_prev_zone, r2 >> r5
   # }

:clip_outside_to_inside

   mov.setf  -, ra_temp2 ; mov.ifz  r0, 4

   # z: point crosses clip rect
   # r0: zone delta from prev
   # r3: y
   # ra_temp3: 2 * enter edge
   # rb_clip_prev_x/y: enter point

   # find the index of the first crossing point
   mov.ifz  r1, elem_num ; mov.ifnz  r1, 15
   # min(r1): index of first crossing point
   mov.setf  -, elem_num ; mov  r2 << 8, r1        @mul_used(8, 9, 10, 11, 12, 13, 14, 15)
   min  r1, r1, r2       ; v8min  r2 << 4, r1, r2  @mul_used(12, 13, 14, 15)
   min  r1, r1, r2       ; v8min  r2 << 2, r1, r2  @mul_used(14, 15)
   min  r1, r1, r2       ; v8min  r2 << 1, r1, r2  @mul_used(15)
   .assert rsi_clip_q_n == 0
   sub.ifz  ra_scalar, ra_scalar, -16 ; nop # going to subtract an extra 16 below...
   nop                   ; v8min  r5rep >> 15, r1, r2
   # r5: index of first crossing point

   # update zone deltas up to crossing point and then sum
   # shift things down:
   # - x/y (currently in ra_clip_q_x/r3)
   # - 2 * enter edge (currently in ra_temp3)
   # - enter x/y (currently in rb_clip_prev_x/y)
   # adjust clip q n
   # figure out corners we need to add

   sub.setf  -, elem_num, r5           ; v8subs  r5rep, -16, r5
   add.ifn  r0, ra_clip_zone_delta, r0 ; mov.ifnn  r0, ra_clip_zone_delta
   mov  r2, ra_clip_q_x                ; mov  r1 << 8, r0            @mul_used(8, 9, 10, 11, 12, 13, 14, 15)
   add  r0, r0, r1                     ; mov  ra_clip_prev_cf, r3 >> r5 # abuse prev cf for storing y
   mov  r3, ra_temp3                   ; mov  r1 << 4, r0            @mul_used(12, 13, 14, 15)
   add  r0, r0, r1                     ; mov  rb_clip_prev_zone, r2 >> r5 # abuse prev zone for storing x
   add  r2, ra_clip_exit_edge, ra_clip_exit_edge ; mov  r1 << 2, r0  @mul_used(14, 15)
   add  r0, r0, r1                     ; mov  r3, r3 >> r5
   # r2[0]: 2 * (exit_edge + 1)
   # r3[0]: 2 * enter_edge
   sub  r2, r3, r2                     ; mov  r1 << 1, r0            @mul_used(15)
   # r2[0]: 2 * (enter_edge - (exit_edge + 1))
   add  r0, r0, r1                     ; mov  r1, rb_clip_prev_x
   mov.setf  -, elem_num               ; mov  ra_temp, ~7
   .assert rsi_clip_q_n == 0
   add.ifz  ra_scalar, ra_scalar, r5   ; mov  r0, r0 << 15
   # r0[0]: zone delta
   sub  r0, r0, r2                     ; mov  r2, rb_clip_prev_y
   # r0[0]: zone_delta - (2 * (enter_edge - (exit_edge + 1)))
   and  r0, r0, ra_temp                ; mov  rb_clip_prev_x, r1 >> r5
   # r0[0]: ((zone_delta - (2 * (enter_edge - (exit_edge + 1)))) >> 3) << 3
   add  r0, r3, r0                     ; mov  rb_clip_prev_y, r2 >> r5
   # r0[0]: (enter_edge + (((zone_delta - (2 * (enter_edge - (exit_edge + 1)))) >> 3) * 4)) * 2
   asr  r0, r0, 1                      ; mov  r1, ra_user_to_surface_clip
   # r0[0]: corrected enter_edge = enter_edge + (((zone_delta - (2 * (enter_edge - (exit_edge + 1)))) >> 3) * 4)
   sub.setf  -, elem_num, 4            ; v8adds  r2, 4, 4
   mov  r3, 0                          ; mov  r1, r1 << 6
   sub.setf  -, elem_num, r2           ; mov.ifnn  r1, r1 >> 4  @mul_unused(0, 1, 2, 3)
   not  r3, r3                         ; mov.ifnn  r1, r1 >> 8  @mul_unused(0, 1, 2, 3, 4, 5, 6, 7)
   # r1: min x, min y, max x, max y (four times)
   and.setf  -, elem_num, 1            ; mov  r2, r1
   add  r5rep, ra_clip_exit_edge, r3   ; mov.ifnz  r2, r1 << 1
   sub.setf  ra_clip_exit_edge, r0, r5 ; mov.ifz  r1, r1 << 1
   # r1: min y, min y, max y, max y (four times)
   # r2: min x, max x, max x, min x (four times)
   sub.ifnn  r5rep, 0, r5              ; mov  r0, 0
   sub.ifn  ra_clip_exit_edge, r0, ra_clip_exit_edge ; mov  r0, 2
   add.ifn  r5rep, r5, r0              ; mov  ra_clip_q_x, r2 >> r5
   nop                                 ; nop
   nop                                 ; mov  ra_clip_q_y, r1 >> r5

   # ra_clip_q_x/y: corners x/y
   # ra_clip_exit_edge[0]: number of corners

   # add corners and enter point
:1
   sub.setf  -, elem_num, 1           ; mov  r0, 1
   add.ifn.setf  ra_clip_exit_edge, ra_clip_exit_edge, -16 ; nop
   brr.anyn  -, r:1f
   # branch delay slots {
   and  r5rep, ra_clip_exit_edge, 15  ; nop
   nop                                ; mov  r1, rb_clip_prev_x
   sub.setf  -, elem_num, r5          ; mov  r2, rb_clip_prev_y
   # }
   brr  -, r:clip_out
   # branch delay slots {
   mov.setf  -, elem_num              ; v8adds  r5rep, 8, 8
   .assert rsi_clip_q_n == 0
   sub.ifz  ra_scalar, ra_scalar, -16 ; nop
   mov  ra_temp3, a:1b
   # }
   # < 16 corners, last batch. add enter point to end
:1
   brr  ra_temp3, r:clip_out
   # branch delay slots {
   add  r0, r5, r0                    ; mov.ifz  ra_clip_q_x, r1 >> r5
   mov.setf  -, elem_num              ; mov.ifz  ra_clip_q_y, r2 >> r5
   .assert rsi_clip_q_n == 0
   add.ifz  ra_scalar, ra_scalar, r0  ; mov  r5rep, r0
   # }

   # now inside (prev point is enter point), continue with rest of vertices (we
   # reprocess the crossing point)
   brr  -, r:clip_inside
   # branch delay slots {
   mov  ra_clip_q_x, rb_clip_prev_zone ; nop
   mov  ra_clip_q_y, ra_clip_prev_cf   ; nop
   nop                                 ; nop
   # }

:end_subpath

   # flush clip queue
   sub.setf  -, elem_num, rsi_clip_q_n
   mov.ifz.setf  -, ra_scalar ; mov  r0, ra_scalar
   bra.allnz  -, (ra_scalar << rsi_clip_lr) >> 15
   # branch delay slots {
   sub.setf  -, elem_num, rsi_clip_lr
   mov.ifz  ra_scalar, a:1f
   nop
   # }
   mov.ifz  ra_scalar, r0
:1

   # if the subpath begins and ends outside the clip rect (note that it always
   # begins and ends in the same place), to simplify the algorithm, we pretend
   # there were preceding/succeeding points just inside the clip rect (see
   # clip_subpath_first)
   mov  r0, a:clip_outside
   sub.setf  -, elem_num, rsi_clip_lr ; nop
   sub.ifz.setf  -, ra_scalar, r0     ; mov  r3, 1
   brr.allnz  -, r:1f
   # branch delay slots {
   mov.ifz  ra_scalar, a:1f
   mov  r0, ra_user_to_surface_clip   ; mov  r1, rb_clip_prev_x
   mov.setf  -, elem_num              ; mov  r2, rb_clip_prev_y
   # }
   .assert rsi_clip_q_n == 0
   mov.ifz  ra_scalar, r3             ; mov  r3, r0 << 6
   fmax  r1, r1, r3                   ; mov  r3, r0 << 8
   brr  -, r:clip_outside
   # branch delay slots {
   fmin  ra_clip_q_x, r1, r3          ; mov  r3, r0 << 7
   fmax  r2, r2, r3                   ; mov  r3, r0 << 9
   fmin  ra_clip_q_y, r2, r3          ; nop
   # }
:1

   # reset clip entry point
   sub.setf  -, elem_num, rsi_clip_lr
   mov.ifz  ra_scalar, a:clip_subpath_first

   # find center of subpath and reset subpath sum
   # the center needs to be included in the bounding box as coverage without
   # fill can cause the hardware to screw up
   mov  r0, ra_subpath_sum_x       ; nop
   mov  r1, ra_subpath_sum_y       ; nop
   itof  r2, ra_scalar             ; mov  r0, r0 << 8
   fadd  r0, r0, ra_subpath_sum_x  ; mov  r1, r1 << 8
   fadd  r1, r1, ra_subpath_sum_y  ; mov  recip, r2 << rsi_subpath_n
   mov  r3, f(0.0)                 ; mov  r2, r0 << 4
   fadd  r0, r0, r2                ; mov  r2, r1 << 4
   fadd  r1, r1, r2                ; fmul  r5rep, r4, f(4.0)
   fsub  r3, r3, r5                ; mov  r2, r0 << 2
   fadd  r0, r0, r2                ; mov  r2, r1 << 2
   fadd  r1, r1, r2                ; mov  ra_subpath_sum_x, f(0.0)
   mov.setf  -, elem_num           ; mov  r2, r0 << 1
   fadd  r0, r0, r2                ; mov  r2, r1 << 1
   fadd  r1, r1, r2                ; fmul  r0, r0, r3
   ftoi  r0, r0                    ; fmul  r1, r1, r3
   min.ifz  ra_min_x, ra_min_x, r0 ; v8min  r2, r0, rb_0xffff
   max.ifz  ra_max_x, ra_max_x, r0 ; nop
   ftoi  r1, r1                    ; mov  ra_subpath_sum_y, f(0.0)
   min.ifz  ra_min_y, ra_min_y, r1 ; mul24  r3, r1, rb_0x10000
   max.ifz  ra_max_y, ra_max_y, r1 ; v8max  r5rep, r2, r3  @geomd_i(geomd_i_fill_post_center) @geomd_tris_set_center_m(geomd_i_fill_pre_center) @geomd_i(geomd_i_fill_pre_center) @geomd_tris_clear @geomd_verts_a(3)
   # r5: center

   # we need at least 88 bytes of space to keep going at rsi_alloc_p:
   # - 8 for the current list's head
   # - 4 for the next list's tail
   # - 64 for at least 1 batch of vertices in the next list
   # - 12 for the next list's head
   # if we don't have 88 bytes, we'll have to alloc
   mov.setf  -, neg_elems(elems(rsi_alloc_size))
   mov  r0, 88
   sub.ifn.setf  -, ra_scalar, r0 ; nop
   mov  ra_temp, a:1f # alloc will return here
   brr.anyn  -, r:alloc
   # branch delay slots {
   mov  rb_list_head, 0x062a0101 # cont flag not set
   mov.setf  -, elem_num ; mov  r1, 1
   sub.setf  -, elem_num, 15 ; mov.ifnz  rb_list_head, r5 # put center in
   # }

   # we have at least 88 bytes at rsi_alloc_p. write out the head of the current
   # list and the tail of the next list
   mov.ifz  rb_list_head, 0xbfff0000
   mov  r0, (1 << 7) - (13 << 16) # 1 row down, only 3 elements
   add  r1, ra_scalar, r1  ; mov  r2, rb_list_head
   # r2 >> 1: 0xbfff0000 nop nop [inline coord list] [tri fan, non-cont] center
   sub  r3, ra_scalar, 12  ; nop
   add  r0, ra_scalar, r0  ; mov  vw_setup, r1 << rsi_out_vpm_setup
   mov.setf  -, elems(rsi_alloc_p, rsi_alloc_size)
   mov.ifnz  ra_scalar, r3 ; mov  -, vw_wait
   nop                     ; mov  vpm, r2 >> 1
   nop                     ; mov  vw_setup, r0 << rsi_out_vdw_setup_0
   nop                     ; mov  vw_addr, r3 << rsi_alloc_p
:1

   # if we're not done, go to chunk_lr
   mov.setf  -, ra_scalar ; mov  r5rep, ra_scalar << rsi_chunk_lr
   sub.ifz.setf  -, elem_num, rsi_chunks_m_size ; mov  ra_temp, r5
   nop                    ; nop
   bra.allnz  -, ra_temp
   # branch delay slots {
   sub.setf  -, elem_num, rsi_subpath_n ; nop
   and.setf  -, elem_num, 2 ; mov.ifz  ra_scalar, 0
   mov  rb_list_head, 0x162a0101 # cont flag set
   # }

   # we're almost done...

   # find the bbox of the whole path
   mov  r0, ra_min_x         ; nop
   mov  r1, ra_min_y         ; nop
   mov  r2, ra_max_x         ; mov  r0, r0 << 2
   mov  r3, ra_max_y         ; mov  r1, r1 << 2
   min  r0, r0, ra_min_x     ; mov  r2, r2 << 2
   min.ifz  r0, r1, ra_min_y ; mov  r3, r3 << 2
   max  r1, r2, ra_max_x     ; nop
   max.ifz  r1, r3, ra_max_y ; nop
   nop                       ; mov  r2, r0 << 8
   min  r0, r0, r2           ; mov  r2, r1 << 8
   max  r1, r1, r2           ; nop
   nop                       ; mov  r2, r0 << 4
   min  r0, r0, r2           ; mov  r2, r1 << 4
   max  r1, r1, r2           ; mov  r2, 1
   and.setf  -, elem_num, r2 ; mov  r2, r0 << 1
   min  r0, r0, r2           ; mov  r2, r1 >> 1
   max.ifnz  r0, r1, r2      ; nop
   # r0[2]: min x
   # r0[3]: max x
   # r0[4]: min y
   # r0[5]: max y

   # we have at least 76 bytes at alloc p, easily enough for the rep (needs 60
   # bytes)

   # fill in the rest of the IO_CHUNK_O_T (r0) and write it out
   mov  r2, ra_scalar        ; mov  r3, 15
   mov  r1, 56
   sub.setf  -, elem_num, r3 ; mov  r5rep, r2 << rsi_alloc_p
   add.setf  -, elem_num, -1 ; mov.ifz  r0, -1
   sub.ifz  r0, r5, r1       ; v8adds  r2, r2, 2 # 2 rows down
   add.ifn  r0, r5, 4        ; nop # branch in just after the 0xbfff0000
   # r0[0]: branch
   # r0[1]: rep (60 bytes of 4-byte aligned memory)
   # r0[2]: min x
   # r0[3]: max x
   # r0[4]: min y
   # r0[5]: max y
   # r0[15]: -1
   mov  r3, 2 << 7 # 2 rows down
   # no need to vw_wait here -- don't use this vpm row anywhere else
   add  r2, ra_scalar, r3    ; mov  vw_setup, r2 << rsi_out_vpm_setup
   nop                       ; mov  vpm, r0
   mov  r0, ra_scalar        ; mov  vw_setup, r2 << rsi_out_vdw_setup_0
   sub  r1, ra_scalar, r1    ; mov  vw_addr, unif

   # save adjusted (alloced another 56 bytes for rep) alloc p/size for next shader
   nop ; mov  vw_setup, r0 << rsi_chunks_vpm_setup
   .assert rsi_alloc_size == (rsi_alloc_p + 1)
   nop ; mov  vpm, r1 << rsi_alloc_p

   # wait for writes, prod host, then end
   empty_vpm_fifo
   mov  -, vw_wait
.ifnset VG_TESS_NO_INT
   mov  interrupt, 1
.endif
   thrend
   nop
   nop

# to simplify the calling code, alloc actually does a little more than just
# allocating:
# - (if this isn't the first alloc) terminate the current list by writing
#   rb_list_head at rsi_alloc_p
# - allocate a new subblock and set rsi_alloc_p/size (rsi_alloc_p will point to
#   the end of the new subblock, as we write things in reverse)
# - start the next list by writing rb_list_tail at rsi_alloc_p (which we set up
#   in the not-first-alloc case to point to the previous list. in the first
#   alloc case, it should be set up by the caller)

# preserves ra_temp3, ra_temp4
:alloc

   # terminate the current list by writing the first 3 words of rb_list_head at rsi_alloc_p - 12
   # save rsi_alloc_p - 12 in rb_list_tail[2]
   add  r0, ra_scalar, 1     ; v8adds  r2, 1, 1 # 1 row down
   mov  r1, (1 << 7) - (13 << 16) # 1 row down, only 3 elements
   sub.setf  -, elem_num, r2 ; mov  r2, 12
   add  r0, ra_scalar, r1    ; mov  vw_setup, r0 << rsi_out_vpm_setup
   sub  r1, ra_scalar, r2    ; mov  -, vw_wait
   nop                       ; mov  vpm, rb_list_head
   nop                       ; mov  vw_setup, r0 << rsi_out_vdw_setup_0
   nop                       ; mov  vw_addr, r1 << rsi_alloc_p
   nop                       ; mov.ifz  rb_list_tail, (r1 << rsi_alloc_p) >> 2

:alloc_first

   empty_vpm_fifo # workaround for hw-2497
   mov  -, mutex             ; nop
   mov  vr_setup, vpm_setup(1, 0, h32(15))
   mov  r2, 0xffe
   nop                       ; mov  r5rep, ra_scalar << rsi_tess_i
   sub.setf  -, elem_num, r5 ; nop
   sub  r0, vpm, 2           ; mov  r1, vpm
   and.ifz.setf  r1, r1, r2  ; mov.ifnz  r0, r1
   brr.allnz  -, r:1f
   # branch delay slots {
   mov  vw_setup, vpm_setup(1, 0, h32(15))
   .assert not (VG_TESS_SUBBLOCK_SIZE & 0x1)
   mov  r3, VG_TESS_SUBBLOCK_SIZE >> 1
   mov  vpm, r0              ; mul24  r1, r1, r3
   # }
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
   add  r1, r5, 2            ; mov.ifz  r0, r5
   and.setf  r1, r1, r2      ; mov  vpm, r0
   brr.allnz  -, r:1f
   # branch delay slots {
   nop                       ; mul24  r1, r1, r3
   nop                       ; mov  r5rep, ra_scalar << rsi_tess_i
   sub.setf  -, elem_num, r5 ; nop
   # }
   # out of memory...
   and.ifz  r0, r0, 1        ; nop
   mov  vpm, r0              ; mov  r0, ra_scalar
   empty_vpm_fifo # make sure vpm writes are done before we release the mutex
   mov  mutex, 0             ; nop
   # write -1 all over the io chunk to let the host know we're done
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
   mov  mutex, 0             ; mov  ra_temp2, 12
   # r2: 0xffe
   # ra_temp2: 12
   # alloc p = ((r0 & ~0xfff) + r1)[r5]
   # alloc size = r3 * 2
   sub  r2, -2, r2           ; nop
   # r2: ~0xfff
   and  r0, r0, r2           ; v8adds  r2, ra_scalar, 1 # 1 row down
   add  r0, r0, r1           ; v8subs  r5rep, -16, r5
   # r0[-r5]: alloc p
   add  r3, r3, r3           ; mov  vw_setup, r2 << rsi_out_vpm_setup
   # r3: alloc size
   nop                       ; mov  r5rep, r0 >> r5
   # r5: alloc p
   mov  r0, (1 << 7) - (13 << 16) # 1 row down, only 3 elements
   sub.setf  -, elem_num, rsi_alloc_p ; nop
   add  r0, ra_scalar, r0    ; mov  -, vw_wait
   sub.ifz  ra_scalar, r5, ra_temp2 ; mov  vpm, rb_list_tail
   bra  -, ra_temp
   # branch delay slots {
   sub.setf  -, elem_num, rsi_alloc_size ; nop
   sub.ifz  ra_scalar, r3, ra_temp2 ; mov  vw_setup, r0 << rsi_out_vdw_setup_0
   sub  vw_addr, r5, 12      ; nop
   # }

.eval pragma_allow_xor_0(prev_allow_xor_0)
.eval pragma_dont_warn_when_mul_rot_inp_r5(prev_dont_warn_when_mul_rot_inp_r5)
