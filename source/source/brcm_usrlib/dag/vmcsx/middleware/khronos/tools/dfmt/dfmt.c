/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interface/khronos/common/khrn_int_image.h"

int main(int argc, char **argv)
{
   if (argc != 2) {
      printf("usage: %s <format>\n", argv[0]);
      return -1;
   }

   unsigned int format = strtoul(argv[1], NULL, 0);
   char desc[256] = { 0 };

   if (format == IMAGE_FORMAT_INVALID) {
      printf("IMAGE_FORMAT_INVALID\n");
      return 0;
   }

   switch (format & IMAGE_FORMAT_MEM_LAYOUT_MASK) {
   case IMAGE_FORMAT_RSO: strcat(desc, "rso"); break;
   case IMAGE_FORMAT_TF:  strcat(desc, "tf"); break;
   case IMAGE_FORMAT_LT:  strcat(desc, "lt"); break;
   default:               goto invalid;
   }

   switch (format & IMAGE_FORMAT_COMP_MASK) {
   case IMAGE_FORMAT_UNCOMP:
   {
      switch (format & IMAGE_FORMAT_PIXEL_TYPE_MASK) {
      case IMAGE_FORMAT_COLOR:
      {
         #define IMAGE_FORMAT_PACK(A, B, C, D, E, F) (IMAGE_FORMAT_##A | IMAGE_FORMAT_##B | IMAGE_FORMAT_##C | IMAGE_FORMAT_##D | IMAGE_FORMAT_##E | IMAGE_FORMAT_##F)
         #define IMAGE_FORMAT__ 0

         switch (format & (IMAGE_FORMAT_PIXEL_SIZE_MASK | IMAGE_FORMAT_RGB | IMAGE_FORMAT_L | IMAGE_FORMAT_A | IMAGE_FORMAT_XRGBX | IMAGE_FORMAT_XA | IMAGE_FORMAT_PIXEL_LAYOUT_MASK)) {
         case IMAGE_FORMAT_PACK(32, RGB, A, XRGBX, XA, 8888): strcat(desc, " rgba 8888"); break;
         case IMAGE_FORMAT_PACK(32, RGB, A, XBGRX, XA, 8888): strcat(desc, " bgra 8888"); break;
         case IMAGE_FORMAT_PACK(32, RGB, A, XRGBX, AX, 8888): strcat(desc, " argb 8888"); break;
         case IMAGE_FORMAT_PACK(32, RGB, A, XBGRX, AX, 8888): strcat(desc, " abgr 8888"); break;
         case IMAGE_FORMAT_PACK(32, RGB, _, XRGBX, XA, 8888): strcat(desc, " rgbx 8888"); break;
         case IMAGE_FORMAT_PACK(32, RGB, _, XBGRX, XA, 8888): strcat(desc, " bgrx 8888"); break;
         case IMAGE_FORMAT_PACK(32, RGB, _, XRGBX, AX, 8888): strcat(desc, " xrgb 8888"); break;
         case IMAGE_FORMAT_PACK(32, RGB, _, XBGRX, AX, 8888): strcat(desc, " xbgr 8888"); break;
         case IMAGE_FORMAT_PACK(16, RGB, A, XRGBX, XA, 4444): strcat(desc, " rgba 4444"); break;
         case IMAGE_FORMAT_PACK(16, RGB, A, XBGRX, XA, 4444): strcat(desc, " bgra 4444"); break;
         case IMAGE_FORMAT_PACK(16, RGB, A, XRGBX, AX, 4444): strcat(desc, " argb 4444"); break;
         case IMAGE_FORMAT_PACK(16, RGB, A, XBGRX, AX, 4444): strcat(desc, " abgr 4444"); break;
         case IMAGE_FORMAT_PACK(16, RGB, A, XRGBX, XA, 5551): strcat(desc, " rgba 5551"); break;
         case IMAGE_FORMAT_PACK(16, RGB, A, XBGRX, XA, 5551): strcat(desc, " bgra 5551"); break;
         case IMAGE_FORMAT_PACK(16, RGB, A, XRGBX, AX, 5551): strcat(desc, " argb 1555"); break;
         case IMAGE_FORMAT_PACK(16, RGB, A, XBGRX, AX, 5551): strcat(desc, " abgr 1555"); break;
         case IMAGE_FORMAT_PACK(16, RGB, _, XRGBX, _ , 565 ): strcat(desc, " rgb 565"); break;
         case IMAGE_FORMAT_PACK(16, RGB, _, XBGRX, _ , 565 ): strcat(desc, " bgr 565"); break;
         case IMAGE_FORMAT_PACK(16, L  , A, _    , XA, 88  ): strcat(desc, " la 88"); break;
         case IMAGE_FORMAT_PACK(8 , L  , _, _    , _ , _   ): strcat(desc, " l 8"); break;
         case IMAGE_FORMAT_PACK(1 , L  , _, _    , _ , _   ): strcat(desc, " l 1"); break;
         case IMAGE_FORMAT_PACK(8 , _  , A, _    , _ , _   ): strcat(desc, " a 8"); break;
         case IMAGE_FORMAT_PACK(4 , _  , A, _    , _ , _   ): strcat(desc, " a 4"); break;
         case IMAGE_FORMAT_PACK(1 , _  , A, _    , _ , _   ): strcat(desc, " a 1"); break;
         default:                                             goto invalid;
         }

         #undef IMAGE_FORMAT__
         #undef IMAGE_FORMAT_PACK

         if (format & IMAGE_FORMAT_PRE) { strcat(desc, " pre"); }
         if (format & IMAGE_FORMAT_LIN) { strcat(desc, " lin"); }

         break;
      }
      case IMAGE_FORMAT_PALETTE:
      case IMAGE_FORMAT_SAMPLE:
      {
         switch (format & IMAGE_FORMAT_PIXEL_TYPE_MASK) {
         case IMAGE_FORMAT_PALETTE: strcat(desc, " palette"); break;
         case IMAGE_FORMAT_SAMPLE:  strcat(desc, " sample"); break;
         default:                   assert(0);
         }

         switch (format & IMAGE_FORMAT_PIXEL_SIZE_MASK) {
         case IMAGE_FORMAT_1:  strcat(desc, " 1"); break;
         case IMAGE_FORMAT_4:  strcat(desc, " 4"); break;
         case IMAGE_FORMAT_8:  strcat(desc, " 8"); break;
         case IMAGE_FORMAT_16: strcat(desc, " 16"); break;
         case IMAGE_FORMAT_32: strcat(desc, " 32"); break;
         default:              goto invalid;
         }

         break;
      }
      case IMAGE_FORMAT_DEPTH:
      {
         strcat(desc, " depth");

         switch (format & IMAGE_FORMAT_PIXEL_SIZE_MASK) {
         case IMAGE_FORMAT_16: strcat(desc, " 16"); break;
         case IMAGE_FORMAT_32: strcat(desc, " 32"); break;
         default:              goto invalid;
         }

         if (format & IMAGE_FORMAT_Z) { strcat(desc, " z"); }
         if (format & IMAGE_FORMAT_STENCIL) { strcat(desc, " stencil"); }

         break;
      }
      default:
      {
         goto invalid;
      }
      }
      break;
   }
   case IMAGE_FORMAT_ETC1:
   {
      strcat(desc, " etc1");
      break;
   }
   case IMAGE_FORMAT_PACKED_MASK:
   {
      strcat(desc, "packed mask");
      break;
   }
   default:
   {
      goto invalid;
   }
   }

   printf("%s\n", desc);
   return 0;

invalid:
   printf("invalid format\n");
   return -1;
}
