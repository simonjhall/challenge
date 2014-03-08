/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_BACKEND_H
#define GLSL_BACKEND_H

#ifdef __VIDEOCORE4__
extern bool glsl_backend_create_shaders(
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
   Dataflow *frag_discard);

extern uint32_t glsl_backend_get_schedule_type(Dataflow *dataflow);

extern bool glsl_backend_schedule(Dataflow *root, uint32_t type, bool *allow_thread, bool fb_rb_swap);

#define SCHEDULE_TYPE_INPUT (1<<0)
#define SCHEDULE_TYPE_OUTPUT (1<<1)
#define SCHEDULE_TYPE_ALU (1<<2)
#define SCHEDULE_TYPE_SIG (1<<4)

#else
extern DataflowChain *glsl_backend_schedule(DataflowChain *sched, DataflowChain *roots, bool isvtx);
extern void glsl_backend_allocate(DataflowChain *sched, bool isvtx);
extern void glsl_backend_generate(DataflowChain *sched, DataflowChain *roots, short **code, unsigned int *size, bool isvtx);
#endif

#endif