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
#include <assert.h>
#include <string.h>

/* Project includes */
#include "vcinclude/vcore.h"
#include "vclib/vclib.h"
#include "helpers/vc_image/vc_image.h"
#include "helpers/vc_image/vc_image_tmpbuf.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern int vc_image_atext_measure (int size, const char * str);
extern int vc_image_atext(VC_IMAGE_T * dest, int x, int y, int fg, int bg, int size,
                             const char * str);

/* Check extern defs match above and loads #defines and typedefs */

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

extern void vc_image_atext_strip(void * ptr, int x0, int x1, unsigned int mask);
extern void vc_image_atext_rect(void * ptr, int pitch, int x, int y, int w, int h);
extern void vc_image_atext_conv(unsigned char * ptr, int nbytes, int offset);

/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/


/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/

#if defined(USE_RGB888) || defined(USE_RGB565) || defined(USE_8BPP) || defined(USE_RGBA32)

static void atext_line (unsigned char * ptr, int pitch, int x,int y,int xx,int yy, int xthick, int ythick);
static int atext_convyx (int pos);
static void atext_corner_over (unsigned char * ptr, int pitch, int x, int y, int xx, int yy, int thick);
static void atext_corner_under (unsigned char * ptr, int pitch, int x, int y, int xx, int yy, int thick);
static void atext_corner (unsigned char * ptr, int pitch, int x,int y,int xx,int yy,int thick);
static void atext_char (unsigned char * ptr, int pitch, int thick, const int ytab[16], const int xtab[16], const unsigned char * dptr);

#endif

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

#if defined(USE_RGB888) || defined(USE_RGB565) || defined(USE_8BPP) || defined(USE_RGBA32)

/* Functions to plot in a bitmap image, where each byte represents 4x2
   "sub-pixels", thus:
                       b0 b1 b2 b3
                       b4 b5 b6 b7
   x,y coordinates are scaled up by 256 with respect to "big" pixels
   (i.e. by 64 or 128 with respect to "sub-pixels").

   Most of the work is done by assembler routines in vc_image_atext_asm.s

   THERE IS NO BOUNDS CHECKING HERE. The glyph-drawing code must not plot
   outside the temporary image. Of course, vc_image_font_alpha_blit() has
   bounds checking, so an off-the-screen character may still be drawn.      */

static void atext_line(unsigned char * ptr, int pitch,
                       int x,int y,int xx,int yy, int xthick, int ythick)
{
   int i, j, grad, off, width;

   /* Draw vertical and horizontal lines with a single rectangle. */
   if (y==yy || x==xx) {
      if (y>yy) {
         i=y; y=yy; yy=i; }
      if (x>xx) {
         i=x; x=xx; xx=i; }
      vc_image_atext_rect(ptr, pitch, x, y, xx+xthick-x, yy+ythick-y);
      return;
   }

   x += 32; /* for rounding */
   xx += 32;

   /* For simplicity, always have y above yy */
   if (y>yy) {
      i=y; y=yy; yy=i; i=x; x=xx; xx=i; }

   /* Compute the gradient (dx/dy) scaled by 32768 */
   grad = ((xx-x)*32768+((yy-y)>>1)) / (yy-y);

   /* The following is a HACK to reduce the thickness of diagonal lines */
   {
      int a, n;
      a = abs(grad);
      n = 6;
      if (a > (1<<14) && a < (1<<16)) n = 4;
      else if (a > (1<<13) && a < (1<<17)) n = 5;
      x  += (xthick >> n);
      xx += (xthick >> n);
      y  += (ythick >> n);
      yy += (ythick >> n);
      xthick -= (xthick >> n-1);
      ythick -= (ythick >> n-1);
   }

   /* Compute horizontal width of the diagonal, and its crossings at y */
   if (xx > x) {
      off = -_mulhd_su(grad, ythick<<17);
      width = xthick - off;
      off += x;
   }
   else {
      off = x;
      width = xthick-_mulhd_su(grad, ythick<<17);
      i=x; x=xx; xx=i;
   }

   /* x, y, xx, yy will form a bounding box */
   xx += xthick;
   yy += ythick;

   /* Scan at all the heights of (N*128)+64 between y and yy. */
   i = (y+64)&~127;
   i += 64;
   ptr += pitch*(i>>8);

   for (; i < yy; i+=128) {
      j = off + _mulhd_ss(grad,(i-y)<<15)*4;
      if (!(i & 128)) {
         vc_image_atext_strip(ptr, _max(x,j)>>6, _min(j+width, xx)>>6,
                              0x0f0f0f0f);
      }
      else {
         vc_image_atext_strip(ptr, _max(x,j)>>6, _min(j+width, xx)>>6,
                              0xf0f0f0f0);
         ptr += pitch;
      }
   }
}

static int atext_convyx(int pos)  /* input Q29, output Q32U */
{
   static const unsigned short xy_table[65] = {
      0, 5770, 8128, 9915, 11403, 12697, 13852, 14901,
      15864, 16756, 17588, 18368, 19102, 19797, 20454, 21079,
      21674, 22241, 22783, 23300, 23796, 24270, 24725, 25161,
      25580, 25981, 26367, 26737, 27092, 27434, 27762, 28076,
      28378, 28667, 28945, 29211, 29466, 29709, 29942, 30165,
      30377, 30579, 30771, 30954, 31127, 31291, 31445, 31591,
      31727, 31855, 31974, 32085, 32187, 32280, 32366, 32442,
      32511, 32571, 32624, 32668, 32704, 32732, 32752, 32764, 32768
   };
   int i, j, k;
   if ((unsigned)pos >= (unsigned)(1 << 29)) {
      return (pos < 0) ? 0 : -1;
   }
   k = pos >> 23;
   pos <<= 9;
   i = xy_table[k];
   j = xy_table[k+1] - i;
   return (i + _mulhd_su(j, pos)) << 17;
}

static void atext_corner_over(unsigned char * ptr, int pitch,
                              int x, int y, int xx, int yy, int thick)
{
   int i, j0, j1, x1, y1, step0, step1;
   int nw = (xx < x);

   yy += thick >> 1;
   y1 = y + thick;
   x += thick >> 1;
   x1 = xx;
   xx += thick;

   step1 = (1<<29) / (yy - y);
   step0 = (1<<29) / (yy - y1);

   i = (y+64)&~127;
   i += 64;
   ptr += pitch*(i>>8);

   for (; i <= yy; i += 128) {
      if (nw) {
         j1 = x + _mulhd_su((xx - x), atext_convyx(step0 * (i - y1)));
         j0 = x + _mulhd_su((x1 - x), atext_convyx(step1 * (i - y)));
         j1 = _max(j0, j1);
         j0 = _max(x1, j0) >> 6;
         j1 = _min(x, j1) >> 6;
      }
      else {
         j0 = x + _mulhd_su((x1 - x), atext_convyx(step0 * (i - y1)));
         j1 = x + _mulhd_su((xx - x), atext_convyx(step1 * (i - y)));
         j1 = _max(j0, j1);
         j0 = _max(x, j0) >> 6;
         j1 = _min(xx, j1) >> 6;
      }
      if (!(i & 128)) {
         vc_image_atext_strip(ptr, j0,j1, 0x0f0f0f0f);
      }
      else {
         vc_image_atext_strip(ptr, j0,j1, 0xf0f0f0f0);
         ptr += pitch;
      }
   }
}

static void atext_corner_under(unsigned char * ptr, int pitch,
                               int x, int y, int xx, int yy, int thick)
{
   int i, j0, j1, x1, y1, step0, step1;
   int se = (xx < x);

   y1 = yy;
   yy += thick;
   xx += thick >> 1;
   x1 = x + thick;
   y += thick >> 1;

   step0 = (1<<29) / (yy - y);
   step1 = (1<<29) / (y1 - y);

   i = (y+64)&~127;
   i += 64;
   ptr += pitch*(i>>8);

   for (; i <= yy; i += 128) {
      if (se) {
         j0 = xx + _mulhd_su((x - xx), atext_convyx((step1*(y1 - i))));
         j1 = xx + _mulhd_su((x1 - xx), atext_convyx((step0*(yy - i))));
         j1 = _max(j0, j1);
         j0 = _max(xx, j0) >> 6;
         j1 = _min(x1, j1) >> 6;
      }
      else {
         j0 = xx + _mulhd_su((x - xx), atext_convyx((step0*(yy - i))));
         j1 = xx + _mulhd_su((x1 - xx), atext_convyx((step1*(y1 - i))));
         j1 = _max(j0, j1);
         j0 = _max(x, j0) >> 6;
         j1 = _min(xx, j1) >> 6;
      }
      if (!(i & 128)) {
         vc_image_atext_strip(ptr, j0, j1, 0x0f0f0f0f);
      }
      else {
         vc_image_atext_strip(ptr, j0, j1, 0xf0f0f0f0);
         ptr += pitch;
      }
   }
}

static void atext_corner(unsigned char * ptr, int pitch,
                         int x,int y,int xx,int yy,int thick)
{
   int i = min(abs(xx-x), abs(yy-y));
   if (i < 256 || i*2 <= thick) {
      atext_line(ptr, pitch, x, y, xx, yy, thick, thick);
      return;
   }

   x += 32; // For rounding
   xx += 32;

   if (yy>y) {
      atext_corner_over(ptr, pitch, x, y, xx, yy, thick);
   }
   else {
      atext_corner_under(ptr, pitch, xx, yy, x, y, thick);
   }
}



/*****************************************************************************
NAME
   atext_char

SYNOPSIS
static void atext_char(unsigned char * ptr, int pitch,
                       int thick,
                       const int ytab[16],
                       const int xtab[16],
                       const unsigned char * dptr)

FUNCTION
   Draw a character onto the bitmap image, as described in rather cryptic
   format by the font data pointed to by dptr.

RETURNS
   -
*****************************************************************************/

static void atext_char(unsigned char * ptr, int pitch,
                       int thick,
                       const int ytab[16],
                       const int xtab[16],
                       const unsigned char * dptr)
{
   int x0=-1, y0=0, x1, y1;

   for (; *dptr != 255; ++dptr) {
      if (*dptr >= 250) {
         if (*dptr==250) { /* Lift pen */
            x0=-1;
         }
         else if (*dptr==251) { /* dot */
            ++dptr;
            x1 = xtab[dptr[0]&15];
            y1 = ytab[dptr[0]>>4];
            x0 = thick >> 2;
            if (dptr[0] < 0x20 || y1 < thick) {
               vc_image_atext_rect(ptr, pitch, x1, y1, thick, thick+x0*2);
            }
            else {
               vc_image_atext_rect(ptr, pitch, x1, y1-x0*2, thick, thick+x0*2);
            }
            x0 = -1;
         }
         else if (*dptr==252) { /* quadrant, hoz first */
            ++dptr;
            x1 = xtab[dptr[0]&15];
            y1 = ytab[dptr[0]>>4];
            assert(x0 >= 0);
            atext_corner(ptr, pitch, x0, y0, x1, y1, thick);
            x0 = x1;
            y0 = y1;
         }
         else if (*dptr==253) { /* quadrant, vert first */
            ++dptr;
            x1 = xtab[dptr[0]&15];
            y1 = ytab[dptr[0]>>4];
            assert(x0 >= 0);
            atext_corner(ptr, pitch, x1, y1, x0, y0, thick);
            x0 = x1;
            y0 = y1;
         }
      }
      else { /* line */
         x1 = xtab[dptr[0]&15];
         y1 = ytab[dptr[0]>>4];
         if (x0 >= 0)
            atext_line(ptr, pitch, x0, y0, x1, y1, thick, thick);
         x0 = x1;
         y0 = y1;
      }
   }
}

#endif



/*****************************************************************************
Exported functions
*****************************************************************************/

/*****************************************************************************
NAME
   vc_image_atext_measure

SYNOPSIS
   int vc_image_atext_measure(int size, const char * str)

FUNCTION
   Measure the total width of a string at the given point size.
   Size must be between 6 and 160 inclusive, with optional "fixed" flag.

RETURNS
   Width in pixels
*****************************************************************************/

int vc_image_atext_measure(int size, const char * str)
{
   int w = 0, c;
   
   if (size & VC_IMAGE_ATEXT_FIXED) {
      size &= ~VC_IMAGE_ATEXT_FIXED;
      assert(size >= 6 && size <= 160);
      size = size*64/3;
      size = (size*6 + 64) >> 8;
      return size * strlen(str);
   }
   else {
      assert(size>=6 && size<=160);
      size = (size*64)/3;
      
      while (*str) {
         c = *str++;
         if (c<32 || c>127) c=127;
         w += (size*atext_widths[c-32] + 64) >> 8;
      }
      return w;
   }
}

/*****************************************************************************
NAME
   vc_image_atext_clip_length

SYNOPSIS
   int vc_image_atext_clip_length(int size, const char * str, int width)

FUNCTION
   Returns the maximum number of characters that can be printed of
   a given string with the given point size, so the printed output
   is less than or equal to the given width.
   Size must be between 6 and 160 inclusive.

RETURNS
   Number of characters
*****************************************************************************/

int vc_image_atext_clip_length(int size, const char * str, int width)
{
   int num = 0, w = 0, c;

   if (size & VC_IMAGE_ATEXT_FIXED) {
      size &= ~VC_IMAGE_ATEXT_FIXED;
      assert(size >= 6 && size <= 160);
      size = (size*64)/3;
      size = (size*6 + 64) >> 8;
      return width / size;
   }
   else {
      assert(size>=6 && size<=160);
      size = (size*64)/3;
      
      while (*str) {
         c = *str++;
         if (c<32 || c>127) c=127;
         w += (size*atext_widths[c-32] + 64) >> 8;
         if (w > width)
            break;
         num++;
      }
      return num;
   }
}

/*****************************************************************************
NAME
   vc_image_atext

SYNOPSIS
   int vc_image__atext(VC_IMAGE_T * dest, int x, int y, int fg, int bg,
                       int size, const char * str)

FUNCTION
   Draw text on an image with the given colours and point size.
   Size is the maximum height and must be between 6 and 160 inclusive.

   The font is antialiased and at some sizes will look blurry.
   A size of 12 corresponds roughly to the size of vc_image_text().
   The VRF lock must be held.

   Clips output to the bounds of the image.

RETURNS
   Width in pixels (unaffected by clipping)
*****************************************************************************/

int vc_image_atext(VC_IMAGE_T * dest, int x, int y, int fg, int bg, int size,
                   const char * str)
{
#if !defined(USE_RGB888) && !defined(USE_RGB565) && !defined(USE_8BPP) && !defined(USE_RGBA32)
   /* not having supported image formats is a bit of a non-starter */
   assert(0);
   return 0;
#else
   VC_IMAGE_T *tmp;
   int w, h, c, i, wtmp, xtmp, size256, thick, fixed;
   int fabr, fabg, fabb;
   int conv_offset;
   int ytab[2][16];
   int xtab[16];

   /* Measure width, check bounds, fill background */
   w = vc_image_atext_measure(size, str);
   fixed = (size & VC_IMAGE_ATEXT_FIXED) ? 1 : 0;
   size &= ~VC_IMAGE_ATEXT_FIXED;
   h = size;
   if (y >= dest->height || x >= dest->width || y <= -h || x <= -w || size<6 || size>160) {
      return w;
   }
   
   if (bg != -1 && bg != -2) {
      vc_image_fill(dest, x, y, w, h, bg /*| 0xFF000000*/);
   }
   else if (bg==-2) {
      vc_image_fade(dest, x, y, w, h, 128);
   }

   // vc_image_font_alpha_blt() requires colour as an (r,g,b) triplet
   // so (stupidly) we must convert the colour from pixelvalue to RGB.
   switch (dest->type) {
#if defined(USE_RGB888) || defined(USE_RGBA32)


   case VC_IMAGE_RGB888:
   case VC_IMAGE_RGBA32:
#ifdef RGB888
      // RED is in the lowest-addressed (least significant) byte
      fabr = fg&255;
      fabg = (fg>>8)&255;
      fabb = (fg>>16)&255;
#else
   // BLUE is in the lowest-addressed (least significant) byte
   fabr = (fg>>16)&255;
   fabg = (fg>>8)&255;
   fabb = fg&255;
#endif
      break;
#endif

#ifdef USE_RGB565
   case VC_IMAGE_RGB565:
      fabr = ((fg>>11)&31)*255/31;
      fabg = ((fg>>5)&63)*255/63;
      fabb = (fg&31)*255/31;
      break;
#endif

#if defined(USE_8BPP) || defined(USE_YUV420) || defined(USE_YUV422) || defined(USE_YUV422PLANAR)
   case VC_IMAGE_8BPP:
   case VC_IMAGE_YUV420:
   case VC_IMAGE_YUV422:
   case VC_IMAGE_YUV422PLANAR:
      fabr = fabg = fabb = (fg & 255); /* Foreground text goes on Y only */
      break;
#endif

#if defined(USE_TFORMAT)
   case VC_IMAGE_TF_RGBA32:
   case VC_IMAGE_TF_RGBX32:
      fabr = fg&255;
      fabg = (fg>>8)&255;
      fabb = (fg>>16)&255;
      break;
#endif

   default:
      assert(0);                /* bad colour format error */
      return w;
   }

   conv_offset = (dest->type == VC_IMAGE_8BPP) ? 0x1f : 16*(fabg>>6) - 1;

   size256 = (size*64)/3;
   thick = size*16;
   if (thick < 288) thick = 256;

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
   memset(xtab, 0, sizeof(xtab));

   tmp = vc_image_alloc_tmp_image(VC_IMAGE_8BPP, (((size+3)*3/4)+15)&~15, size+1);
   assert(tmp);

   while (*str) {
      c = *str++;
      if (c<32 || c>127) c=127;
      c -= 32;
      vclib_memset(tmp->image_data, 0, tmp->pitch*size);
      wtmp = atext_widths[c];
      xtmp = (size256*wtmp + 64) >> 8;

      if (c) {
         /* Build the "x" offset table. Also with some hinting thrown in. */
         xtab[0] = 0;
         xtab[1] = xtmp << 7;
         xtab[wtmp] = xtmp << 8;
         if (wtmp > 2) {
            if (size < 18 && (c==('T'-32) || c==('m'-32) || c==('1'-32))) {
               xtab[wtmp-2] = ((xtab[wtmp]-size256) & ~511) + 256 - thick;
            }
            else {
               xtab[wtmp-2] = ((xtab[wtmp]-size256+176) & ~255) - thick;
            }
            if (xtab[wtmp-2] < 0) {
               xtab[wtmp-2] = 0;
            }
            if (xtab[wtmp-2] > xtab[wtmp] - thick) {
               xtab[wtmp-2] = xtab[wtmp] - thick;
            }
            xtab[wtmp-1] = (xtab[wtmp-2] + xtab[wtmp]) >> 1;
            xtab[1] = xtab[wtmp-2] / (wtmp-2);
            for (i = 2; i < wtmp-2; ++i) {
               xtab[i] = xtab[1] * i;
            }
         }
         if (fixed) {
            if (wtmp < 6) for(i = 0; i <= wtmp; ++i) xtab[i] += (6-wtmp)*((size256>>9)<<8);
            if (wtmp > 6) for(i = 0; i <= wtmp; ++i) xtab[i] = (xtab[i]*6)/wtmp;
            xtmp = (size256*6 + 64) >> 8;
         }
         
         atext_char((unsigned char *)(tmp->image_data),
                    tmp->pitch, thick,
                    ytab[atext_offsets[c]>>12],
                    xtab,
                    atext_data+(atext_offsets[c]&4095));
         if (x<dest->width) {
            vc_image_atext_conv((unsigned char *)(tmp->image_data),
                                (tmp->pitch)*(size), conv_offset);
            vc_image_font_alpha_blt(dest, x, y, xtmp, h,
                                    tmp, 0, 0, fabr, fabg, fabb, 256);
         }
      }
      x += xtmp;
   }

   vc_image_free_tmp_image(tmp);

   return w;
#endif
}
