;;; ===========================================================================
;;; Copyright (c) 2006-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

;;;============================================================================
;;; Schematic representation of the VRF (see comments on the code below)
;;;
;;;         0     1     2     3
;;;      +-----+-----+-----+-----+
;;;   A  |     |     |     |     |
;;;      +-----+-----+-----+-----+
;;;   B  |     |     |     |     |
;;;      +-----+-----+-----+-----+
;;;   C  |     |     |     |     |
;;;      +-----+-----+-----+-----+
;;;   D  |     |     |     |     |
;;;      +-----+-----+-----+-----+
;;;
;;;   Each "rectangle" is 16 x 16 bytes


;;; ===========================================================================
;;; NAME
;;;    im_load_1_block
;;;
;;; SYNOPSIS
;;;    void im_load_1_block(uint8_t *addr, int pitch, int vrf_row);
;;;
;;; FUNCTION
;;;    Load a 16x16 byte block into the VRF row indicated (row A).
;;;   
;;; ARGUMENTS
;;;    Start address of beginning of block, pitch (no. of bytes to skip 
;;;    between each row), row number of VRF to load into,
;;;    
;;;
;;; RETURNS
;;;    -

.section .text$im_load_1_block,"text"
.globl im_load_1_block
im_load_1_block:
      vld H(0++,0)+r2*, (r0+=r1) REP 16
      b lr
.stabs "im_load_1_block:F1",36,0,0,im_load_1_block


;;; ===========================================================================
;;; NAME
;;;    im_load_2_blocks
;;;
;;; SYNOPSIS
;;;    void im_load_2_blocks(uint8_t *addr, int pitch, int vrf_row);
;;;
;;; FUNCTION
;;;    Load two 16x16 byte blocks from consective locations in memory
;;;    into the VRF row indicated (row A).
;;;   
;;; ARGUMENTS
;;;    Start address of beginning of block, pitch (no. of bytes to skip 
;;;    between each row), row number of VRF to load into,
;;;
;;; RETURNS
;;;    -

.section .text$im_load_2_blocks,"text"
.globl im_load_2_blocks
im_load_2_blocks:
      vld H(0++,0)+r2*, (r0+=r1) REP 16
      vld H(0++,16)+r2*, (r0+16+=r1) REP 16
      b lr
.stabs "im_load_2_blocks:F1",36,0,0,im_load_2_blocks

;;; ===========================================================================
;;; NAME
;;;    im_load_3_blocks
;;;
;;; SYNOPSIS
;;;    void im_load_3_blocks(uint8_t *addr, int pitch, int vrf_row);
;;;
;;; FUNCTION
;;;    Load 3 16x16 byte blocks from consecutive locations in memory
;;;    into the VRF row indicated (row A).
;;;
;;; RETURNS
;;;    -

.section .text$im_load_3_blocks,"text"
.globl im_load_3_blocks
im_load_3_blocks:
      vld H(0++,0)+r2*, (r0+=r1) REP 16
      vld H(0++,16)+r2*, (r0+16+=r1) REP 16
      vld H(0++,32)+r2*, (r0+32+=r1) REP 16
      b lr
.stabs "im_load_3_blocks:F1",36,0,0,im_load_3_blocks

;;; ===========================================================================
;;; NAME
;;;    im_load_4_blocks
;;;
;;; SYNOPSIS
;;;    void im_load_4_blocks(uint8_t *addr, int pitch, int vrf_row);
;;;
;;; FUNCTION
;;;    Load 4 16x16 byte blocks from consective locations in memory 
;;;    into the VRF row indicated (row A).
;;;
;;; RETURNS
;;;    -

.section .text$im_load_4_blocks,"text"
.globl im_load_4_blocks
im_load_4_blocks:
      vld H(0++,0)+r2*, (r0+=r1) REP 16
      vld H(0++,16)+r2*, (r0+16+=r1) REP 16
      vld H(0++,32)+r2*, (r0+32+=r1) REP 16
      vld H(0++,48)+r2*, (r0+48+=r1) REP 16
      b lr
.stabs "im_load_4_blocks:F1",36,0,0,im_load_4_blocks


;;; ===========================================================================
;;; NAME
;;;    im_split_planar
;;;
;;; SYNOPSIS
;;;    void im_split_planar(int n);
;;; 
;;; FUNCTION
;;;    split n interleaved components into n planar components
;;;    and copy to row C of VRF. Data is assumed to be in
;;;    the row A of VRF. n can be 2, 3 or 4
;;;
;;; RETURNS
;;;    -

.section .text$im_split_planar,"text"
.globl im_split_planar
im_split_planar:
      cbclr                   ;so we always start from the first column of VRF
      mov r1, 0               ;r1 is the current component to be copied
      mov r2, 0               ;r2 is the column offset to be copy in the destination
      mov r4, 16              ;for each component, copy 16 columns
im_split_planar_lbl1:         ;Start of loop for copying 1 component
      mov r3, r1              ;r3 initially stores the first column from the source to be copied
im_split_planar_lbl2:      
      vmov V(32,0)+r2, V(0,0)+r3
      add r3, r0              ;move to the next column to be copied
      addcmpblt r2, 1, r4, im_split_planar_lbl2    ;copy next column
      add r4, 16
      addcmpblt r1, 1, r0, im_split_planar_lbl1    ;copy next component
      b lr
      
.stabs "im_split_planar:F1",36,0,0,im_split_planar
      
;;; ===========================================================================
;;; NAME
;;;    im_save_1_block
;;;
;;; SYNOPSIS
;;;    void im_save_1_block(uint8_t *addr, int pitch, int nrows);
;;;
;;; FUNCTION
;;;    Save 1 16 x nrows byte block of the VRF (from row C).
;;;
;;; RETURNS
;;;    -

.section .text$im_save_1_block,"text"
.globl im_save_1_block
im_save_1_block:
      addcmpbeq r2, 0, 16, im_save_1_block_16_rows
      mov r3, 0
      shl r2, 6            ; limit of vrf row address to save
      mov r4, 64           ; vrf address increment by one row
im_save_1_block_n_rows:
      vst H(32,0)+r3*, (r0)
      add r0, r1
      addcmpbne r3, r4, r2, im_save_1_block_n_rows
      b lr     
im_save_1_block_16_rows:
      vst H(32++,0)*, (r0+=r1) REP 16
      b lr
      
      
;;; ===========================================================================
;;; NAME
;;;    im_save_2_blocks
;;;
;;; SYNOPSIS
;;;    void im_save_2_blocks(uint8_t *addr, uint8_t *addr2, int pitch, int nrows);
;;;
;;; FUNCTION
;;;    Save 2 16 x nrows byte block of the VRF (from row C).
;;;    The second block starts at addr2
;;;
;;; RETURNS
;;;    -

.section .text$im_save_2_blocks,"text"
.globl im_save_2_blocks
im_save_2_blocks:
      addcmpbeq r2, 0, 16, im_save_2_blocks_16_rows
      mov r4, 0
      shl r3, 6            ; limit of vrf row address to save
      mov r5, 64           ; vrf address increment by one row
im_save_2_blocks_n_rows:
      vst H(32,0)+r4*, (r0)
      vst H(32,16)+r4*, (r1)
      add r0, r2
      add r1, r2
      addcmpbne r4, r5, r3, im_save_2_blocks_n_rows
      b lr     
im_save_2_blocks_16_rows:
      vst H(32++,0)*, (r0+=r2) REP 16
      vst H(32++,16)*, (r1+=r2) REP 16
      b lr
      
;;; ===========================================================================
;;; NAME
;;;    im_save_3_blocks
;;;
;;; SYNOPSIS
;;;    void im_save_3_blocks(uint8_t *addr, uint8_t *addr2, int pitch, int nrows);
;;;
;;; FUNCTION
;;;    Save 3 16 x nrows byte block of the VRF (from row C).
;;;    The start of second block is addr2, and the third block is 
;;;    assumed to start at addr2 + (addr2-addr)
;;;
;;; RETURNS
;;;    -

.section .text$im_save_3_blocks,"text"
.globl im_save_3_blocks
im_save_3_blocks:
      stm r6, lr, (--sp)
      sub r6, r1, r0       
      add r6, r1           ; start of third component
      addcmpbeq r3, 0, 16, im_save_3_blocks_16_rows
      mov r4, 0            ; this is our row index
      shl r3, 6            ; limit of vrf row address to save
      mov r5, 64           ; vrf address increment by one row
im_save_3_blocks_n_rows:
      vst H(32,0)+r4*, (r0)   ;copying each component in turn for n rows
      vst H(32,16)+r4*, (r1)
      vst H(32,32)+r4*, (r6)
      add r0, r2              ;incrementing the pointer to next row in VRF
      add r1, r2
      add r6, r2
      addcmpbne r4, r5, r3, im_save_3_blocks_n_rows
      ldm r6, pc, (sp++)
im_save_3_blocks_16_rows:
      vst H(32++,0)*, (r0+=r2) REP 16  ;copy each component for 16 rows
      vst H(32++,16)*, (r1+=r2) REP 16
      vst H(32++,32)*, (r6+=r2) REP 16
      ldm r6, pc, (sp++)

;;; ===========================================================================
;;; NAME
;;;    im_save_4_blocks
;;;
;;; SYNOPSIS
;;;    void im_save_4_blocks(uint8_t *addr, uint8_t *addr2, int pitch, int nrows);
;;;
;;; FUNCTION
;;;    Save 4 blocks of 16 x nrows of pixels (from row C)
;;;    Start of third block is assumed to be addr2 + (addr2-addr) and start of
;;;    fourth block is assumed to be addr2 + 2*(addr2-addr)
;;;
;;; RETURNS
;;;    -
;;;
;;; Basically calling im_save_2_blocks twice

.section .text$im_save_4_blocks,"text"
.globl im_save_4_blocks
im_save_4_blocks:
      stm r6-r9, lr, (--sp)
      mov r6, r0
      mov r7, r1
      mov r8, r2
      mov r9, r3
      cbclr
      bl im_save_2_blocks        ;copy the first two blocks (C0, C1) to the first two components
      sub r4, r7, r6
      add r0, r7, r4             ;beginning of third component
      mov r1, r7
      addscale r1, r4 << 1       ;beginning of fourth component 
      mov r2, r8
      mov r3, r9
      cbadd2
      bl im_save_2_blocks        ;copy the next two blocks (C2, C3) to the last two components
      ldm r6-r9, pc, (sp++)
      

;;; ===========================================================================
;;; NAME
;;;    im_load_2_blocks2
;;;
;;; SYNOPSIS
;;;    void im_load_2_blocks2(uint8_t *addr, uint8_t *addr2, int pitch, int vrf_row);
;;;
;;; FUNCTION
;;;    Load two 16x16 byte blocks into the VRF row indicated (row A).
;;;    The data is assumed to be splitted, and the second block is assumed to 
;;;    start from addr2.
;;;
;;; ARGUMENTS
;;;    Start address of beginning of block, pitch (no. of bytes to skip 
;;;    between each row), row number of VRF to load into,
;;;
;;; RETURNS
;;;    -

.section .text$im_load_2_blocks2,"text"
.globl im_load_2_blocks2
im_load_2_blocks2:
      vld H(0++,0)+r3*, (r0+=r2) REP 16
      vld H(0++,16)+r3*, (r1+=r2) REP 16
      b lr
      
;;; ===========================================================================
;;; NAME
;;;    im_load_3_blocks2
;;;
;;; SYNOPSIS
;;;    void im_load_3_blocks2(uint8_t *addr, uint8_t *addr2, int pitch, int vrf_row);
;;;
;;; FUNCTION
;;;    Load three 16x16 byte blocks into the VRF row indicated (row A).
;;;    The data is assumed to be splitted, and the second block starts from addr2,
;;;    the third block is assumed to starts from addr2 + (addr2-addr)
;;;
;;; ARGUMENTS
;;;    Start address of beginning of block, start address of the second block,
;;;    pitch (no. of bytes to skip between each row), row number of VRF to load into.
;;;
;;; RETURNS
;;;    -

.section .text$im_load_3_blocks2,"text"
.globl im_load_3_blocks2
im_load_3_blocks2:
      vld H(0++,0)+r3*, (r0+=r2) REP 16
      vld H(0++,16)+r3*, (r1+=r2) REP 16
      sub r4, r1, r0
      add r1, r4           ;this is the beginning of third component
      vld H(0++,32)+r3*, (r1+=r2) REP 16
      b lr
      
;;; ===========================================================================
;;; NAME
;;;    im_load_4_blocks2
;;;
;;; SYNOPSIS
;;;    void im_load_4_blocks2(uint8_t *addr, int pitch, int vrf_row);
;;;
;;; FUNCTION
;;;    Load four 16x16 byte blocks into the VRF row indicated (row A).
;;;    The data is assumed to be splitted, and the second block starts
;;;    from addr2, third block from addr2 + (addr2-addr), fourth block
;;;    from addr2 + 2*(addr2-addr)
;;;
;;; ARGUMENTS
;;;    Start address of beginning of block, pitch (no. of bytes to skip 
;;;    between each row), row number of VRF to load into,
;;;
;;; RETURNS
;;;    -

.section .text$im_load_4_blocks2,"text"
.globl im_load_4_blocks2
im_load_4_blocks2:
      vld H(0++,0)+r3*, (r0+=r2) REP 16
      vld H(0++,16)+r3*, (r1+=r2) REP 16
      sub r4, r1, r0
      add r1, r4           ;this is the beginning of third component
      vld H(0++,32)+r3*, (r1+=r2) REP 16
      add r1, r4           ;this is the beginning of fourth component
      vld H(0++,48)+r3*, (r1+=r2) REP 16
      b lr

;;; ===========================================================================
;;; NAME
;;;    im_merge_planar
;;;
;;; SYNOPSIS
;;;    void im_merge_planar(int n);
;;; 
;;; FUNCTION
;;;    merge n planar components into n interleaved components
;;;    and copy to row C of VRF. Data is assumed to be in
;;;    the row A of VRF. n can be 2, 3 or 4
;;;
;;; RETURNS
;;;    -

.section .text$im_merge_planar,"text"
.globl im_merge_planar
im_merge_planar:
      cbclr                   ;so we always start from the first column of VRF
      mov r1, 0               ;r1 is the current component to be copied
      mov r2, 0               ;r2 is the column offset to be copy in the destination
      mov r4, 16              ;for each component, copy 16 columns
im_merge_planar_lbl1:         ;Start of loop for copying 1 component
      mov r3, r1              ;r3 initially stores the first column from the source to be copied
im_merge_planar_lbl2:      
      vmov V(32,0)+r3, V(0,0)+r2
      add r3, r0              ;move to the next column to be copied
      addcmpblt r2, 1, r4, im_merge_planar_lbl2    ;copy next column
      add r4, 16
      addcmpblt r1, 1, r0, im_merge_planar_lbl1    ;copy next component
      b lr
      
.stabs "im_merge_planar:F1",36,0,0,im_merge_planar
      
;;; ===========================================================================
;;; NAME
;;;    im_save_2_blocks2
;;;
;;; SYNOPSIS
;;;    void im_save_2_blocks2(uint8_t *addr, int pitch, int nrows);
;;;
;;; FUNCTION
;;;    Save 2 16 x nrows byte block of the VRF (from row C).
;;;    The two blocks of data will be adjacent to each other
;;;
;;; RETURNS
;;;    -

.section .text$im_save_2_blocks2,"text"
.globl im_save_2_blocks2
im_save_2_blocks2:
      addcmpbeq r2, 0, 16, im_save_2_blocks2_16_rows
      mov r3, 0
      shl r2, 6            ; limit of vrf row address to save
      mov r4, 64           ; vrf address increment by one row
im_save_2_blocks2_n_rows:
      vst H(32,0)+r3*, (r0)
      vst H(32,16)+r3*, (r0+16)
      add r0, r1
      addcmpbne r3, r4, r2, im_save_2_blocks2_n_rows
      b lr     
im_save_2_blocks2_16_rows:
      vst H(32++,0)*, (r0+=r1) REP 16
      vst H(32++,16)*, (r0+16+=r1) REP 16
      b lr
            
;;; ===========================================================================
;;; NAME
;;;    im_save_3_blocks2
;;;
;;; SYNOPSIS
;;;    void im_save_3_blocks2(uint8_t *addr, int pitch, int nrows);
;;;
;;; FUNCTION
;;;    Save 3 16 x nrows byte block of the VRF (from row C).
;;;    The three blocks of data will be adjacent to each other
;;;
;;; RETURNS
;;;    -

.section .text$im_save_3_blocks2,"text"
.globl im_save_3_blocks2
im_save_3_blocks2:
      addcmpbeq r2, 0, 16, im_save_3_blocks2_16_rows
      mov r3, 0
      shl r2, 6            ; limit of vrf row address to save
      mov r4, 64           ; vrf address increment by one row
im_save_3_blocks2_n_rows:
      vst H(32,0)+r3*, (r0)
      vst H(32,16)+r3*, (r0+16)
      vst H(32,32)+r3*, (r0+32)
      add r0, r1
      addcmpbne r3, r4, r2, im_save_3_blocks2_n_rows
      b lr     
im_save_3_blocks2_16_rows:
      vst H(32++,0)*, (r0+=r1) REP 16
      vst H(32++,16)*, (r0+16+=r1) REP 16
      vst H(32++,32)*, (r0+32+=r1) REP 16
      b lr
            
;;; ===========================================================================
;;; NAME
;;;    im_save_4_blocks2
;;;
;;; SYNOPSIS
;;;    void im_save_4_blocks2(uint8_t *addr, int pitch, int nrows);
;;;
;;; FUNCTION
;;;    Save 4 16 x nrows byte block of the VRF (from row C).
;;;    The four blocks of data will be adjacent to each other
;;;
;;; RETURNS
;;;    -

.section .text$im_save_4_blocks2,"text"
.globl im_save_4_blocks2
im_save_4_blocks2:
      addcmpbeq r2, 0, 16, im_save_4_blocks2_16_rows
      mov r3, 0
      shl r2, 6            ; limit of vrf row address to save
      mov r4, 64           ; vrf address increment by one row
im_save_4_blocks2_n_rows:
      vst H(32,0)+r3*, (r0)
      vst H(32,16)+r3*, (r0+16)
      vst H(32,32)+r3*, (r0+32)
      vst H(32,48)+r3*, (r0+48)
      add r0, r1
      addcmpbne r3, r4, r2, im_save_4_blocks2_n_rows
      b lr     
im_save_4_blocks2_16_rows:
      vst H(32++,0)*, (r0+=r1) REP 16
      vst H(32++,16)*, (r0+16+=r1) REP 16
      vst H(32++,32)*, (r0+32+=r1) REP 16
      vst H(32++,48)*, (r0+48+=r1) REP 16
      b lr
            
