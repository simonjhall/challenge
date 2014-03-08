/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VG_HW_PRIV_4_H
#define VG_HW_PRIV_4_H

#include "middleware/khronos/vg/2708/vg_tess_priv_4.h"
#include "middleware/khronos/vg/vg_hw.h"
#include "middleware/khronos/common/khrn_image.h"

typedef struct {
   MEM_HANDLE_T state; /* so we can report errors */

   MEM_HANDLE_T frame, frame_src, mask;
   uint32_t frame_width;
   uint32_t frame_height;
   KHRN_IMAGE_FORMAT_T frame_format;

   uint32_t buffers_acquired;

   bool frame_clear;
   uint32_t frame_clear_rgba;
   bool mask_clear;
   uint8_t mask_clear_value;

   VG_TESS_HANDLE_T tess;

   KHRN_FMEM_T *fmem;

   /* active state at end of bin */
   uint32_t offset;
   uint32_t clip_xy, clip_wh;
   uint32_t cfg;
   MEM_HANDLE_T shader;
   uint32_t *shader_unifs;

   /* glyph stuff */
   MEM_HANDLE_T glyph_fill_shader, glyph_stroke_shader;
   uint32_t *glyph_fill_unifs, *glyph_stroke_unifs;
   uint32_t glyph_fill_unifs_n, glyph_stroke_unifs_n;
   uint64_t glyph_fill_state_id, glyph_stroke_state_id;

   uint32_t extra_thrsws_min; /* we can drop this many thrsws from the start of all shaders. todo: remove on b0 */
#ifdef WORKAROUND_HW2116
   uint32_t batches_n;
#endif
} VG_BE_RENDER_STATE_T;

extern void vg_be_render_state_flush(VG_BE_RENDER_STATE_T *render_state);

#endif
