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

#include "middleware/khronos/common/khrn_tformat.h"

/*****************************************************************************
   Public functions
******************************************************************************/

/*
   Calculates offset to u-tile given u-tile x,y, width
   and type of image buffer (linear-tile or t-format)
*/

unsigned int khrn_tformat_utile_addr
   (int ut_w, int ut_x, int ut_y, int is_tformat, TAddr_t *taddr)
{
   
   unsigned int wt, xt, yt, xst, yst, xut, yut;
   unsigned int tile_addr, sub_tile_addr, utile_addr, addr;
   int line_odd = 0;
      
   if (is_tformat) { // T-FORMAT      
      
      // first check that ut_w and ut_h conform to
      // exact multiples of tiles
      if(ut_w&0x7) 
         UNREACHABLE(); /* ut_w not padded to integer # tiles! */
      
      // find width and height of image in tiles
      wt = ut_w>>3;
      
      // find address of tile given ut_x,ut_y
      // remembering tiles are 8x8 u-tiles,
      // also remembering scan direction changes
      // for odd and even lines
      xt = ut_x>>3; yt = ut_y>>3;
      if(yt&0x1) {
         xt = wt-xt-1; // if odd line, reverse direction
         line_odd = 1;
      }
      tile_addr = yt*wt + xt;
      
      // now find address of sub-tile within tile (2 bits)
      // remembering scan order is not raster scan, and
      // changes direction given odd/even tile-scan-line.
      xst = (ut_x>>2)&0x1;
      yst = (ut_y>>2)&0x1;
      switch(xst+(yst<<1)){
       case 0:  sub_tile_addr = line_odd ? 2 : 0; break;
       case 1:  sub_tile_addr = line_odd ? 1 : 3; break;
       case 2:  sub_tile_addr = line_odd ? 3 : 1; break;
       default: sub_tile_addr = line_odd ? 0 : 2; break;
      }
      
      // now find address of u-tile within sub-tile
      // nb 4x4 u-tiles fit in one sub-tile, and are in
      // raster-scan order.
      xut = ut_x&0x3;
      yut = ut_y&0x3;
      utile_addr = (yut<<2)|xut;

      addr = utile_addr + (sub_tile_addr<<4) + (tile_addr<<6);

#ifndef NDEBUG
      if(taddr) { /* for debug */
         taddr->utile_addr    = utile_addr;
         taddr->sub_tile_addr = sub_tile_addr;
         taddr->tile_addr     = tile_addr;
         taddr->addr          = addr;
      }
#else
      UNUSED(taddr);
#endif
   } else { // LT-FORMAT      
      addr = ut_y*ut_w + ut_x;
   }
   
   return addr;
}
