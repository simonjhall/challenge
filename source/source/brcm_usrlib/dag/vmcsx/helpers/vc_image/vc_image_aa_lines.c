/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <assert.h>
#include <string.h>
#include <vc/intrinsics.h>
#include "vcinclude/vcore.h"
#include "vc_image.h"
#include "helpers/vcmaths/vcmaths.h"

int atan2i(int y, int x, int *pmag);

typedef int FLOAT_T;
#define FLOAT(x)  ((FLOAT_T)((x)*(1<<16)))

#define fdiv(a,b) DivFixed(a, b, 16)
#define fmul(a,b) MultFixed(a, b, 16)
#define swap(a, b) do {FLOAT_T t = a; a = b; b = t;} while (0)

extern void vc_image_line_horiz(VC_IMAGE_T *dest, int x_start, int y_start, int x_end, int y_end, int fg_colour);
extern void vc_image_line_vert(VC_IMAGE_T *dest, int x_start, int y_start, int x_end, int y_end, int fg_colour);
extern void vc_plotline_aa_x(VC_IMAGE_T *im, int firstx, int lastx, int intery, int gradient, int thickness, int col);
extern void vc_plotline_aa_y(VC_IMAGE_T *im, int firstx, int lastx, int intery, int gradient, int thickness, int col);
extern void vc_plotpoint_alpha(VC_IMAGE_T *buffer, int x, int y, FLOAT_T a, int col);
extern void vc_plotpoint_alpha_flush(VC_IMAGE_T *im);

//! fixed point multiply - signed versions only
//! \param shift power of 2 to reduce by (= fractional bits)
static inline FLOAT_T MultFixed(FLOAT_T a, FLOAT_T b, int shift)
{
#ifdef _VIDEOCORE
   unsigned int f = a*b;      // low 32 bits
   int  i = _mulhd_ss(a,b);   // hi 32 bits
   return (i<<(32-shift)) | (f>>shift);
#else
   // assume VC target... needs fixing for other compilers
   return (FLOAT_T) ( ( ((__int64)a) * ((__int64)b) ) >> shift );
#endif
}

static inline FLOAT_T DivFixed(FLOAT_T a, FLOAT_T b, int shift)
{
#ifdef _VIDEOCORE
   return (FLOAT_T) MultFixed(0xffffffffu / b, a, shift);
//    ( ((long long)a) << shift) / ((long long)b) );
   //return (FLOAT_T) ( ( ((long long)a) << shift) / ((long long)b) );
#else
   // assume VC target... needs fixing for other compilers
   return (FLOAT_T) ( ( ((__int64)a << shift) * ((__int64)b) ));
#endif
}

static inline int ipart(FLOAT_T x)
{
   return x >> 16;
}
static inline int round(FLOAT_T x)
{
   return ipart(x + FLOAT(0.5));
}
static inline int iceil(FLOAT_T x)
{
   return ipart(x + FLOAT(1)-1);
}
static inline int fpart(FLOAT_T x)
{
   return x & (FLOAT(1)-1);
}
static inline int rfpart(FLOAT_T x)
{
   return fpart(x) ^ (FLOAT(1)-1);
}

#undef plot
#define plot(im, x, y, a, c)  vc_plotpoint_alpha(im, x, y, fmul(a, ascale), c)
static void vc_image_aa_line_x(VC_IMAGE_T *im, FLOAT_T x1, FLOAT_T y1, FLOAT_T x2, FLOAT_T y2, int thickness, int col)
{
   FLOAT_T dx, dy;
   FLOAT_T gradient;
   int xend;
   FLOAT_T yend;
   int xgap;
   int xpxl1, ypxl1, xpxl2, ypxl2;
   FLOAT_T intery;
   int thick, innerthick = ipart(thickness), ascale=fpart(thickness);

   y1 -= FLOAT(innerthick)>>1;
   y2 -= FLOAT(innerthick)>>1;

   if (x2 == x1) return;
   else if (x2 < x1) {
      swap(x1, x2);
      swap(y1, y2);
   }

   dx = x2 - x1;
   dy = y2 - y1;

   assert(dx > 0);
   assert (dx >= abs(dy));
   gradient = fdiv(dy, dx);

   // handle first endpoint
   xend = round(x1);
   yend = y1 + fmul(gradient, FLOAT(xend) - x1);
   xgap = rfpart(x1 + FLOAT(0.5));
   xpxl1 = xend;  // this will be used in the main loop
   ypxl1 = ipart(yend);
   if (ypxl1 >=0 && ypxl1< im->height-innerthick-1) {
      plot(im, xpxl1, ypxl1, fmul(rfpart(yend), xgap), col);
      for (thick=0; thick < innerthick; thick++)
         plot(im, xpxl1, ypxl1 + 1+thick, rfpart(0), col);
      plot(im, xpxl1, ypxl1 + 1+thick, fmul(fpart(yend), xgap), col);
   }
   intery = yend + gradient; // first y-intersection for the main loop

   // handle second endpoint
   xend = round(x2);
   yend = y2 + fmul(gradient, FLOAT(xend) - x2);
   xgap = rfpart(x2 - FLOAT(0.5));
   xpxl2 = xend;  // this will be used in the main loop
   ypxl2 = ipart(yend);
   if (ypxl2 >=0 && ypxl2< im->height-innerthick-1) {
      plot(im, xpxl2, ypxl2, fmul(rfpart(yend), xgap), col);
      for (thick=0; thick < innerthick; thick++)
         plot(im, xpxl2, ypxl2 + 1+thick, rfpart(0), col);
      plot(im, xpxl2, ypxl2 + 1+thick, fmul(fpart(yend), xgap), col);
   }
   // main loop
   vc_plotpoint_alpha_flush(im);
   vc_plotline_aa_x(im, xpxl1+1, xpxl2-1, intery, gradient, thickness, col);
}

#undef plot
#define plot(im, x, y, a, c)  vc_plotpoint_alpha(im, y, x, fmul(a, ascale), c)
static void vc_image_aa_line_y(VC_IMAGE_T *im, FLOAT_T y1, FLOAT_T x1, FLOAT_T y2, FLOAT_T x2, int thickness, int col)
{
   FLOAT_T dx, dy;
   FLOAT_T gradient;
   int xend;
   FLOAT_T yend;
   int xgap;
   int xpxl1, ypxl1, xpxl2, ypxl2;
   FLOAT_T intery;
   int thick, innerthick = ipart(thickness), ascale=fpart(thickness);

   y1 -= FLOAT(innerthick)>>1;
   y2 -= FLOAT(innerthick)>>1;

   if (x2 == x1) return;
   else if (x2 < x1) {
      swap(x1, x2);
      swap(y1, y2);
   }

   dx = x2 - x1;
   dy = y2 - y1;

   assert(dx > 0);
   assert (dx >= abs(dy));
   gradient = fdiv(dy, dx);

   // handle first endpoint
   xend = round(x1);
   yend = y1 + fmul(gradient, FLOAT(xend) - x1);
   xgap = rfpart(x1 + FLOAT(0.5));
   xpxl1 = xend;  // this will be used in the main loop
   ypxl1 = ipart(yend);
   if (ypxl1 >=0 && ypxl1< im->width-innerthick-1) {
      plot(im, xpxl1, ypxl1, fmul(rfpart(yend), xgap), col);
      for (thick=0; thick < innerthick; thick++)
         plot(im, xpxl1, ypxl1 + 1+thick, rfpart(0), col);
      plot(im, xpxl1, ypxl1 + 1+thick, fmul(fpart(yend), xgap), col);
   }
   intery = yend + gradient; // first y-intersection for the main loop

   // handle second endpoint
   xend = round(x2);
   yend = y2 + fmul(gradient, FLOAT(xend) - x2);
   xgap = rfpart(x2 - FLOAT(0.5));
   xpxl2 = xend;  // this will be used in the main loop
   ypxl2 = ipart(yend);
   if (ypxl2 >=0 && ypxl2< im->height-innerthick-1) {
      plot(im, xpxl2, ypxl2, fmul(rfpart(yend), xgap), col);
      for (thick=0; thick < innerthick; thick++)
         plot(im, xpxl2, ypxl2 + 1+thick, rfpart(0), col);
      plot(im, xpxl2, ypxl2 + 1+thick, fmul(fpart(yend), xgap), col);
   }
   // main loop
   vc_plotpoint_alpha_flush(im);
   vc_plotline_aa_y(im, xpxl1+1, xpxl2-1, intery, gradient, thickness, col);
}

void vc_image_aa_line(VC_IMAGE_T *im, FLOAT_T x1, FLOAT_T y1, FLOAT_T x2, FLOAT_T y2, int thickness, int col)
{
   if (thickness <= 1) {
      assert(1); return; }
   if (abs(x1-x2) < abs(y1-y2))
      vc_image_aa_line_y(im, x1, y1, x2, y2, thickness-1, col);
   else
      vc_image_aa_line_x(im, x1, y1, x2, y2, thickness-1, col);
}

static inline FLOAT_T fsqr(FLOAT_T x)
{
   return fmul(x, x);
}

static inline FLOAT_T fsqrt(FLOAT_T x)
{
   int y;
   assert(x >= 0);
   y = vcmaths_sqrt((unsigned long) x) << 8;
   return y;
}

static inline FLOAT_T fmag(FLOAT_T a, FLOAT_T b)
{
   int r;
   (void)atan2i(a, b, &r);
   return r;
}

void vc_image_aa_ellipse(VC_IMAGE_T *im, FLOAT_T cx, FLOAT_T cy, FLOAT_T a, FLOAT_T b, int thickness, int col, int filled)
{
#undef plot
#define plot(im, x, y, a, c)  vc_plotpoint_alpha(im, x, y, a, c)
   FLOAT_T t;
   FLOAT_T x;
   FLOAT_T y;
   FLOAT_T f, f2;
   int ix, ix2;
   int iy, iy2=0;
   int i1;
   int i2;
   int exch;

   if (a < FLOAT(1) || b < FLOAT(1)) return;

   if (cx + a < 0) return;
   if (cy + b < 0) return;
   if (cx - a > FLOAT(im->width)) return;
   if (cy - b > FLOAT(im->height)) return;
   if ( a < b )
   {
      exch = 0;
   }
   else
   {
      exch = 1;
      swap(cx, cy);
      swap(a, b);
   }
//    t = fdiv(fmul(a,a),fmag(a,b));
   t = fdiv(a, fsqrt(FLOAT(1)+fmul(fdiv(a,b),fdiv(a,b))));
   i1 = ipart(cx-t);
   i2 = iceil(cx+t);
   for (ix = i1; ix <= i2; ix++)
   {
      if ( a < abs(FLOAT(ix)-cx))
      {
         continue;
      }
      y = fmul(b, fsqrt(FLOAT(1)-fsqr(fdiv(FLOAT(ix)-cx, a))));
      iy = iceil(cy+y);
      f = FLOAT(iy)-cy-y;
      iy2 = ipart(cy-y);
      f2 = cy-y-FLOAT(iy);
      // iy > iy2
      if ( !exch )
      {
         plot(im, ix, iy,    rfpart(f),  col);
         plot(im, ix, iy-1,  fpart(f),   col);
         plot(im, ix, iy2+1, fpart(f2),  col);
         plot(im, ix, iy2,   rfpart(f2), col);
         if (filled) vc_image_line_vert(im, ix, iy2+1, ix, iy-1, col);
      }
      else
      {
         plot(im, iy,    ix, rfpart(f),  col);
         plot(im, iy-1,  ix, fpart(f),   col);
         plot(im, iy2+1, ix, fpart(f2),  col);
         plot(im, iy2,   ix, rfpart(f2), col);
         if (filled) vc_image_line_horiz(im, iy2+1, ix, iy-1, ix, col);
      }
   }
//    t = fdiv(fmul(b,b), fsqrt(fmul(a,b)));
   t = fmul(b, fsqrt(fdiv(b,a)));
   i1 = iceil(cy-t);
   i2 = ipart(cy+t);
   for (iy = i1; iy <= i2; iy++)
   {
      if ( b < abs(FLOAT(iy)-cy))
      {
         continue;
      }
      x = fmul(a, fsqrt(FLOAT(1)-fsqr(fdiv(FLOAT(iy)-cy, b))));
      ix = ipart(cx-x);
      f = cx-x-FLOAT(ix);
      ix2 = iceil(cx+x);
      f2 = FLOAT(ix)-cx-x;
      // ix2 > ix
      if ( !exch )
      {
         plot(im, ix,    iy, rfpart(f),  col);
         plot(im, ix+1,  iy, fpart(f),   col);
         plot(im, ix2,   iy, rfpart(f2), col);
         plot(im, ix2-1, iy, fpart(f2),  col);
         if (filled) vc_image_line_horiz(im, ix+1, iy, ix2-1, iy, col);
      }
      else
      {
         plot(im, iy, ix,   rfpart(f),   col);
         plot(im, iy, ix+1, fpart(f),    col);
         plot(im, iy, ix2,   rfpart(f2), col);
         plot(im, iy, ix2-1, fpart(f2),  col);
         if (filled) vc_image_line_vert(im, iy, ix+1, iy, ix2-1, col);
      }
   }

   vc_plotpoint_alpha_flush(im);
}

#if 0
#define MAX_POINTS (16/2)
static void vc_test_poly_intersects(short polyInts16[16][16], short ints[16], VC_IMAGE_POINT_T *p, int n, int starty)
{
   int i, k, y;
   static short test_polyInts16[16][16];
   static short test_ints0[16];
   for (i=0; i<16; i++) {
      ints[i] = 0;
   }
   vc_poly_intersects(test_polyInts16, test_ints0, p, n, starty);
   for (i = 0; i < n; i++) {
      FLOAT_T x1, y1, x2, y2;
      int ind1, ind2;
      if (!i) {
         ind1 = n - 1;
         ind2 = 0;
      } else {
         ind1 = i - 1;
         ind2 = i;
      }
      x1 = ipart(p[ind1].x);
      y1 = ipart(p[ind1].y);
      x2 = ipart(p[ind2].x);
      y2 = ipart(p[ind2].y);
      if (y1 > y2) {
         x2 = ipart(p[ind1].x);
         y2 = ipart(p[ind1].y);
         x1 = ipart(p[ind2].x);
         y1 = ipart(p[ind2].y);
      }
      for (y=starty; y<starty+16; y++) {
         if (ints[y-starty] >= MAX_POINTS * 2) {
            assert(0); break;}
         //if ((y >= y1 && y < y2) || (y == maxy && y > y1 && y <= y2)) {
         if (y >= y1 && y < y2) {
            //short t = x1 + fmul(fmul(y - y1, 0xffffffff/(y2 - y1)), x2 - x1);
            //short t = x1 + (y - y1)*(x2 - x1)/(y2 - y1);
            short t = x1 + ((((((x2 - x1)<<16)/(y2 - y1)) * (y - y1)) + (1<<15))>>16);
            polyInts16[y-starty][ints[y-starty]++] = t;
         }
      }
   }
   // polygons pretty much always have less than 100 points, so insertion sort is a good choice.
   for (k=0; k<16; k++) {
      for (i = 1; i < ints[k]; i++) {
         int index = polyInts16[k][i];
         int j = i;
         while ((j > 0) && (polyInts16[k][j - 1] > index)) {
            polyInts16[k][j] = polyInts16[k][j - 1];
            j--;
         }
         polyInts16[k][j] = index;
      }
   }
   assert(memcmp(test_ints0, ints, sizeof(test_ints0))==0);
   assert(memcmp(test_polyInts16, polyInts16, sizeof(test_polyInts16))==0);
   _nop();
}
#endif

extern void vc_poly_intersects(short polyInts16[16][16], short ints[16], VC_IMAGE_POINT_T *p, int n, int starty);
#define SHIFT 6  // points range ~ 0-1023 or 10 bits, can handle an extra 6
static void vc_image_aa_polygon_filled(VC_IMAGE_T *im, VC_IMAGE_POINT_T *p32, int n, int thickness, int c)
{
   int i, k, y;
   int miny, maxy;
   if (!n) return;
   miny = ipart(p32[0].y);
   maxy = iceil(p32[0].y);
   for (i = 1; i < n; i++) {
      miny = min(miny, ipart(p32[i].y));
      maxy = max(maxy, iceil(p32[i].y));
   }
   // clip to screen
   miny = max(miny, 0);
   maxy = min(maxy, im->height-1);

   for (y = miny; y <= maxy; y+=16) {
      static short polyInts16[16][16];
      static short ints[16];
      vc_poly_intersects(polyInts16, ints, p32, n, y);

      for (k=0; k<16; k++) {
         for (i = 0; i < ints[k]; i += 2) {
#if 0
            vc_plotpoint_alpha(im, ipart(polyInts16[k][i]<<(16-SHIFT)),   y+k, rfpart(polyInts16[k][i]<<(16-SHIFT)),  c);
            vc_plotpoint_alpha(im, ipart(polyInts16[k][i+1]<<(16-SHIFT)), y+k, fpart(polyInts16[k][i+1]<<(16-SHIFT)), c);
            if (ipart(polyInts16[k][i]<<(16-SHIFT))+1 <= ipart(polyInts16[k][i+1]<<(16-SHIFT))-1)
               vc_image_line_horiz(im, ipart(polyInts16[k][i]<<(16-SHIFT))+1, y+k, ipart(polyInts16[k][i+1]<<(16-SHIFT))-1, y+k, c);
#else
            vc_image_line_horiz(im, round(polyInts16[k][i]<<(16-SHIFT)), y+k, round(polyInts16[k][i+1]<<(16-SHIFT)), y+k, c);
#endif
         }
      }
   }
   vc_plotpoint_alpha_flush(im);
}

void vc_image_aa_polygon(VC_IMAGE_T *im, VC_IMAGE_POINT_T *p, int n, int thickness, int c, int filled)
{
   int i;
   if (filled) {
      vc_image_aa_polygon_filled(im, p, n, thickness, c);
      return;
   }

   for (i = 0; i < n-1; i++) {
      vc_image_aa_line (im, p[i].x, p[i].y, p[i+1].x, p[i+1].y, thickness, c);
   }
   vc_image_aa_line (im, p[n-1].x, p[n-1].y, p[0].x, p[0].y, thickness, c);
}

