/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/* System includes */
#include <string.h>

/* Project includes */
#include "vcinclude/vcore.h"
#include "helpers/vc_image/vc_image.h"
#include "helpers/vc_image/vc_image_tmpbuf.h"
#include "interface/vcos/vcos_assert.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern int vc_image_atext_measure (int size, const char * str);
extern int vc_image_atext (VC_IMAGE_T * dest, int x, int y, int fg, int bg, int size, const char * str);

/* Check extern defs match above and loads #defines and typedefs */

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/


/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/


/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/

static inline void atext_plot (unsigned char * ptr, int c);
static void atext_rect (unsigned char * ptr, int pitch, int x, int y, int w, int h);
static void atext_line (unsigned char * ptr, int pitch, int x,int y,int xx,int yy, int thick);
static void atext_corner (unsigned char * ptr, int pitch, int x,int y,int xx,int yy,int thick);
static void atext_char (unsigned char * ptr, int pitch, int thick, int xorig, int xstep,
                        const int ytab[16], const unsigned char * dptr);

/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/

extern const unsigned char atext_widths[];
extern const short atext_offsets[];
extern const unsigned char atext_data[];

/******************************************************************************
Global function definitions.
******************************************************************************/



/*****************************************************************************
Static functions.
*****************************************************************************/

/* Plot a point, line or corner (quarter-ellipse) on an 8-bit image.

   All coordinates are scaled by 256 for antialiasing.
   There is no bounds checking in these functions.

   XXX these routines are scalar, and a bit on the slow side.
   atext_rect() is a good candidate for future conversion to assembler!
*/
static inline void atext_plot(unsigned char * ptr, int c)
{
   *ptr = max((*ptr), c) | 15;
}

static void atext_rect(unsigned char * ptr, int pitch,
                       int x, int y, int w, int h)
{
   int i, j, k;

   /* Normalise x,y to be fractional only; convert w,h to bottom-right */
   i = (x>>8);
   j = (y>>8);
   ptr += i;
   ptr += pitch*j;
   x &= 255;
   y &= 255;
   w += x;
   h += y;

   if (h<=256) { /* Single height */
      y = min(h-y,255);
      if (w<=256) { /* Single width */
         x = min(w-x,255);
         atext_plot(ptr, x*y>>8);
         return;
      }
      atext_plot(ptr, (255-x)*y>>8);
      for (i=1; i<(w>>8); ++i) atext_plot(ptr+i, y);
      atext_plot(ptr+i, (w&255)*y>>8);
      return;
   }
   if (w<=256) { /* Single width */
      x = min(w-x,255);
      atext_plot(ptr, (255-y)*x>>8);
      for (i=1; i<(h>>8); ++i) atext_plot(ptr+i*pitch, x);
      atext_plot(ptr+i*pitch, (h&255)*x>>8);
      return;
   }

   /* Plot the corners and edges */
   k=255-y;
   atext_plot(ptr, (255-x)*k>>8);                 /* TL */
   if (w>256) {
      for (j=(w>>8); --j>0;) atext_plot(ptr+j, k); /* Top */
      atext_plot(ptr+(w>>8), (w&255)*k>>8);       /* TR */
   }
   if (h>256) {
      k = 255-x;
      for (i=(h>>8); --i>0;) atext_plot(ptr+i*pitch, k);     /* Left */
      if (w>256) {
         k=w&255;
         j=w>>8;
         for (i=(h>>8); --i>0;) atext_plot(ptr+i*pitch+j, k); /* Right */
      }
      k=h&255;
      i=pitch*(h>>8);
      atext_plot(ptr+i, (255-x)*k>>8);                 /* BL */
      if (w>256) {
         for (j=(w>>8); --j>0;) atext_plot(ptr+i+j, k); /* Bottom */
         atext_plot(ptr+i+(w>>8), (w&255)*k>>8);       /* BR */
      }
   }

   /* Fill in the middle */
   k = (w>>8)-1;
   if (k > 0) {
      ptr += pitch+1;
      for (i=(h>>8); --i>0;) {
         for (j=0; j<k; ++j) ptr[j] = 255;
         ptr += pitch;
      }
   }
}

static void atext_line(unsigned char * ptr, int pitch,
                       int x,int y,int xx,int yy, int thick)
{
   int i;

   if (y==yy || x==xx) {
      if (y>yy) {
         i=y; y=yy; yy=i; }
      if (x>xx) {
         i=x; x=xx; xx=i; }
      atext_rect(ptr, pitch, x, y, xx+thick-x, yy+thick-y);
      return;
   }

   atext_rect(ptr, pitch, x, y, thick, thick);
   atext_rect(ptr, pitch, xx, yy, thick, thick);

   if (abs(x-xx) >= abs(y-yy)) {
      if (xx>x) {
         for (i=128;i<xx-x;i+=128)
            atext_rect(ptr, pitch, x+i, y+(((yy-y)*i/(xx-x))), thick, thick);
      }
      else {
         for (i=128;i<x-xx;i+=128) {
            atext_rect(ptr, pitch, xx+i, yy+(((y-yy)*i/(x-xx))), thick, thick);
         }
      }
   }
   else {
      if (yy>y) {
         for (i=128;i<yy-y;i+=128)
            atext_rect(ptr, pitch, x+(((xx-x)*i/(yy-y))), y+i, thick, thick);
      }
      else {
         for (i=128;i<y-yy;i+=128)
            atext_rect(ptr, pitch, xx+(((x-xx)*i/(y-yy))), yy+i, thick, thick);
      }
   }
}

static void atext_corner(unsigned char * ptr, int pitch,
                         int x,int y,int xx,int yy,int thick)
{
   int i, step, x1=x, y1=y, x2, y2;
   static const int sin_table[9] = { 0, 50, 98, 142, 181, 212, 236, 251, 256 };

   i = min(abs(xx-x), abs(yy-y));
   if (i<256) step=8;
   else if (i<512) step=4;
   else if (i<2048) step=2;
   else step=1;

   for (i = step; i <= 8; i += step) {
      x2 = x + ((xx-x)*sin_table[i]+128>>8);
      y2 = y + ((yy-y)*(256-sin_table[8-i])+128>>8);
      atext_line(ptr, pitch, x1, y1, x2, y2, thick);
      x1 = x2;
      y1 = y2;
   }
}


/******************************************************************************
NAME
   atext_char

SYNOPSIS
static void atext_char(unsigned char * ptr, int pitch,
                       int thick, int xorig, int xstep,
                       const int ytab[16],
                       const unsigned char * dptr)

FUNCTION
   Draw a character onto the 8-bit image, as described in rather cryptic
   format by the font data pointed to by dptr. thick, x0, xstep and ytab

RETURNS
   -
******************************************************************************/

static void atext_char(unsigned char * ptr, int pitch,
                       int thick, int xorig, int xstep,
                       const int ytab[16],
                       const unsigned char * dptr)
{
   int x0=-1, y0, x1, y1;

   for (; *dptr != 255; ++dptr) {
      if (*dptr >= 250) {
         if (*dptr==250) { /* Lift pen */
            x0=-1;
         }
         else if (*dptr==251) { /* dot */
            ++dptr;
            x1 = xorig+xstep*(dptr[0]&15);
            y1 = ytab[dptr[0]>>4];
            atext_rect(ptr, pitch, x1, y1, thick, thick);
            x0 = -1;
         }
         else if (*dptr==252) { /* quadrant, hoz first */
            ++dptr;
            x1 = xorig+xstep*(dptr[0]&15);
            y1 = ytab[dptr[0]>>4];
            vcos_assert(x0 >= 0);
            atext_corner(ptr, pitch, x0, y0, x1, y1, thick);
            x0 = x1;
            y0 = y1;
         }
         else if (*dptr==253) { /* quadrant, vert first */
            ++dptr;
            x1 = xorig+xstep*(dptr[0]&15);
            y1 = ytab[dptr[0]>>4];
            vcos_assert(x0 >= 0);
            atext_corner(ptr, pitch, x1, y1, x0, y0, thick);
            x0 = x1;
            y0 = y1;
         }
      }
      else { /* line */
         x1 = xorig+xstep*(dptr[0]&15);
         y1 = ytab[dptr[0]>>4];
         if (x0 >= 0)
            atext_line(ptr, pitch, x0, y0, x1, y1, thick);
         x0 = x1;
         y0 = y1;
      }
   }
}



/******************************************************************************
NAME
   vc_image_atext_measure

SYNOPSIS
   int vc_image_atext_measure(int size, const char * str)

FUNCTION
   Measure the total width of a string at the given point size.
   Size must be between 6 and 100 inclusive.

RETURNS
   Width in pixels
******************************************************************************/

int vc_image_atext_measure(int size, const char * str)
{
   int w = 0, c;

   vcos_assert(size>=6 && size<=100);
   size = (size*64)/3;

   while (*str) {
      c = *str++;
      if (c<32 || c>127) c=127;
      w += size*atext_widths[c-32];
   }
   return (w+255)>>8;
}


/******************************************************************************
NAME
   vc_image_atext

SYNOPSIS
   int vc_image__atext(VC_IMAGE_T * dest, int x, int y, int fg, int bg,
                       int size, const char * str)

FUNCTION
   Draw text on an image with the given colours and point size.
   Size is the maximum height and must be between 6 and 100 inclusive.

   The font is antialiased and at some sizes will look blurry.
   A size of 12 corresponds roughly to the size of vc_image_text().
   The VRF lock must be held.

   Clips output to the bounds of the image.

RETURNS
   Width in pixels (unaffected by clipping)
******************************************************************************/

int vc_image_atext(VC_IMAGE_T * dest, int x, int y, int fg, int bg, int size,
                   const char * str)
{
   VC_IMAGE_T *tmp;
   int w, h, c, xfrac=0, size256, thick;
   int fabr, fabg, fabb;
   int ytab[2][16];

   /* Measure width, check bounds, fill background */
   w = vc_image_atext_measure(size, str);
   if (y >= dest->height || x >= dest->width) {
      return w;
   }
   h = size;

   if (bg >= 0) {
      vc_image_fill(dest, x, y, w, h, bg);
   }
   else if (bg==-2) {
      vc_image_fade(dest, x, y, w, h, 128);
   }

   tmp = vc_image_alloc_tmp_image(VC_IMAGE_8BPP, size, size);
   vcos_assert(tmp);

#ifdef USE_RGB888
   if (dest->type == VC_IMAGE_RGB888) {
      fabr = (fg>>16)&255;
      fabg = (fg>>8)&255;
      fabb = fg&255;
   }
#ifdef USE_RGB565
   else
#endif
#endif
#ifdef USE_RGB565
   {
      fabr = ((fg>>11)&31)*255/31;
      fabg = ((fg>>5)&63)*255/63;
      fabb = (fg&31)*255/31;
   }
#endif

   size256 = (size*64)/3;
   thick = size*16;
   if (thick < 256) thick = 256;

   /* Build the "y" offset table. This is rather peculiarly hinted. */
   ytab[1][0]  = ytab[0][0]  = 0;
   ytab[1][1]  = ytab[0][1]  = (size256+64)&~255;
   ytab[1][9]  = ytab[0][9]  = ((size256*10)&~255)-thick;
   ytab[1][11] = ytab[0][11] = size*256-thick;
   ytab[1][10] = ytab[0][10] = ((ytab[0][9]+ytab[0][11])>>9)<<8;
   ytab[1][12] = ytab[0][12] = ytab[0][9]-(size*96)&~255;
   for (c = 2; c < 9; ++c) ytab[0][c] = ytab[0][1] + ((ytab[0][9]-ytab[0][1])*(c-1)>>3);
   ytab[1][5]  = ((ytab[1][9]+ytab[1][1])>>9)<<8;
   for (c = 2; c < 5; ++c) ytab[1][c] = ytab[1][1] + ((ytab[1][5]-ytab[1][1])*(c-1)>>2);
   for (c = 6; c < 9; ++c) ytab[1][c] = ytab[1][5] + ((ytab[1][9]-ytab[1][5])*(c-5)>>2);
   for (c = 13; c < 16; ++c) ytab[0][c] = ytab[0][12] + ((ytab[0][9]-ytab[0][12])*(c-12)>>2);
   ytab[1][14] = ((ytab[0][12]+ytab[0][9])>>9)<<8;
   ytab[1][13] = (ytab[0][12]+ytab[0][14])>>1;
   ytab[1][15] = (ytab[0][14]+ytab[0][9])>>1;

   while (*str) {
      c = *str++;
      if (c<32 || c>127) c=127;
      c -= 32;
      vclib_memset(tmp->image_data, 0, tmp->pitch*size);
      atext_char((unsigned char *)(tmp->image_data),
                 tmp->pitch, thick, xfrac, size256,
                 ytab[atext_offsets[c]>>12],
                 atext_data+(atext_offsets[c]&4095));
      xfrac += size256*atext_widths[c];
      if (x<dest->width) {
         vc_image_font_alpha_blt(dest, x, y, (xfrac+255)>>8, h,
                                 tmp, 0, 0, fabr, fabg, fabb, 256);
      }
      x += (xfrac >> 8);
      xfrac &= 255;
   }

   vc_image_free_tmp_image(tmp);

   return w;
}
