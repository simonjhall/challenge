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

/* Project includes */
#include "vcinclude/vcore.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern int vclib_textsize (int font,char *text);
extern int vclib_textwrite (unsigned short *buffer,int pitch,int sx,int sy,
                               int fx,int fy,int fcolour,int bcolour,int font,const char *text);
extern void vclib_textslider (unsigned short *buffer,int pitch,int sx,int sy,
                                 int fx,int fy,int pos);

/* Check extern defs match above and loads #defines and typedefs */
#include "helpers/vclib/vclib.h"

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/


/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/

#define MAKEPOS(x,y,pitch) (  ((pitch)-1-(y)) +(x)*(pitch)   )
#define MAKERGB(r,g,b) ( ((r>>3)<<11) + ((g>>2)<<5) + ((r>>3)) )
#define DRAWLINEX(sx,sy,fx,fy,black,pitch) {for(x=(sx);x<=(fx);x++) buffer[MAKEPOS((x),(sy),pitch)]=black;}
#define DRAWLINEY(sx,sy,fx,fy,black,pitch) {for(y=(sy);y<=(fy);y++) buffer[MAKEPOS((sx),(y),pitch)]=black;}

/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/


/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/

extern const short fontpos[(128-32)];
extern const int fontdata[32];  /* Fontdata can actually have more entries than specified here */
extern const short fontpos16[(128-32)];
extern const short fontdata16[32];  /* Fontdata can actually have more entries than specified here */

/******************************************************************************
Global function definitions.
******************************************************************************/

/******************************************************************************
NAME
  vclib_textwrite

SYNOPSIS
   int vclib_textwrite(unsigned short *buffer,int pitch,int sx,int sy,int fx,int fy
   ,int fcolour,int bcolour,int font,char *text)

FUNCTION
   Writes the string to the display.
   buffer points to the start of the display memory
   sx,sy->fx,fy define a rectangle for the text
   Coordinates are defined so that x=0,y=0 corresponds to top left of display
   pitch is the spacing (in shorts) of the display memory (typically 640 for VGA)

   Font is the fontsize (16 or 32)
   fcolour is the foreground colour (5.6.5 RGB)
   bcolour is the backround colour (5.6.5 RGB or -1 for overlay or -2 for fade overlay)

RETURNS
   The width of the string
******************************************************************************/

int vclib_textwrite(unsigned short *buffer,int pitch,int sx,int sy,int fx,int fy
                    ,int fcolour,int bcolour,int font,const char *text)
{
   int start,end;
   int y;
   for (;text[0]!=0;text++)
   {
      int c=(int)text[0];
      switch (font)
      {
      case 32:

         if ((c<32) || (c>=128)) c=33; // Change unknown characters to !
         c-=32;

         // Now compute location of the text in memory
         if (c>0)
            start=fontpos[c-1];
         else
            start=0;

         end=fontpos[c];

         for (;start<end;start++,sx++)
         {
            int data;
            if (sx>=fx) return sx;

            data=fontdata[start];  /* This contains packed binary for the letter */
            /* Needs to be displayed at location sx,sy */
            for (y=sy;y<min(sy+32,fy);y++)
            {
               int pos=31-(y-sy);
               if (data&(1<<pos))
               {
                  if (bcolour==-1)
                  {
                     // Transparent
                  }
                  else if (bcolour==-2)
                  {
                     // one means background
                     // Lets gray out the background
                     unsigned short *b=&buffer[(pitch-1-y)+sx*pitch];
                     // wipe out bottom bits and shift down
                     b[0]=(b[0]&(0xffff-(1+(1<<5)+(1<<11))) )>>1;
                  }
                  else
                  {
                     buffer[(pitch-1-y)+sx*pitch]=bcolour;
                  }
               }
               else
               {
                  buffer[(pitch-1-y)+sx*pitch]=fcolour;
               }
            }
         }
         break;
      case 16:

         /*if ((c>='A')&(c<='Z')) c=c-'A';
         else
         if ((c>='a')&(c<='z')) c=c-'a'+26;
         else
         if ((c>='0')&(c<='9')) c=c-'0'+26*2;
         else
         c=26*2+10;*/

         if ((c<32) || (c>=128)) c=33; // Change unknown characters to !
         c-=32;

         // Now compute location of the text in memory
         if (c>0)
            start=fontpos16[c-1];
         else
            start=0;

         end=fontpos16[c];

         for (;start<end;start++,sx++)
         {
            int data;
            if (sx>=fx) return sx;

            data=fontdata16[start];  /* This contains packed binary for the letter */
            /* Needs to be displayed at location sx,sy */
            for (y=sy;y<min(sy+16,fy);y++)
            {
               int pos=15-(y-sy);
               if (data&(1<<pos))
               {
                  // one means background
                  if (bcolour==-1)
                  {
                     // Transparent
                  }
                  else if (bcolour==-2)
                  {
                     // Lets gray out the background
                     unsigned short *b=&buffer[(pitch-1-y)+sx*pitch];
                     // wipe out bottom bits and shift down
                     b[0]=(b[0]&(0xffff-(1+(1<<5)+(1<<11))) )>>1;
                  }
                  else
                  {
                     buffer[(pitch-1-y)+sx*pitch]=bcolour;
                  }
               }
               else
               {
                  buffer[(pitch-1-y)+sx*pitch]=fcolour;
               }
            }
         }
         break;
      default:
         assert(0);
         break;
      }

   }
   return sx;
}





/******************************************************************************
NAME
  vclib_textslider

SYNOPSIS
   void vclib_textslider(unsigned short *buffer,int pitch,int sx,int sy,int fx,int fy,int pos)

FUNCTION
   Draws a slider onto the screen with coordinates sx,sy to fx,fy and position pos (0 to 255)
   Coordinates are defined so that x=0,y=0 corresponds to top left of display
   pitch is the spacing (in shorts) of the display memory (typically 640 for VGA)

RETURNS
   -
******************************************************************************/

void vclib_textslider(unsigned short *buffer,int pitch,int sx,int sy,int fx,int fy,int pos)
{
   int boxx;
   int boxy;
   int x,y;
   int m;
   int slidsx,slidfx,slidy,marksy;
   int bcolour=MAKERGB(100,100,100);
   int darkgray=MAKERGB(50,50,50);
   int black=MAKERGB(0,0,0);
   int lightgray=MAKERGB(200,200,200);
   // First gray out the entire area
   for (x=sx;x<=fx;x++)
      for (y=sy;y<=fy;y++)
         buffer[MAKEPOS(x,y,pitch)]=bcolour;

   // Draw a pretty border
   for (x=sx;x<=fx;x++)
   {
      buffer[MAKEPOS(x,sy,pitch)]=darkgray;
      buffer[MAKEPOS(x,sy+1,pitch)]=lightgray;
   }
   for (y=sy+1;y<=fy;y++)
   {
      buffer[MAKEPOS(sx,y,pitch)]=darkgray;
      buffer[MAKEPOS(sx+1,y,pitch)]=lightgray;
   }

   for (x=sx;x<=fx;x++)
   {
      buffer[MAKEPOS(x,fy-1,pitch)]=darkgray;
      buffer[MAKEPOS(x,fy,pitch)]=lightgray;
   }
   for (y=sy+1;y<=fy;y++)
   {
      buffer[MAKEPOS(fx,y,pitch)]=darkgray;
      buffer[MAKEPOS(fx-1,y,pitch)]=lightgray;
   }

   // Now calculate the extent of the slider
   // we make it cover 80% of the width
   slidsx=sx+((fx-sx)*4)/10;
   slidfx=sx+((fx-sx)*9)/10;

   slidy=sy+(fy-sy)/3;
   marksy=sy+((fy-sy)*2)/3;

   // Now draw slider bar
   DRAWLINEX(slidsx,slidy,slidfx,slidy,black,pitch)
   DRAWLINEX(slidsx-1,slidy-1,slidfx+1,slidy-1,darkgray,pitch)
   DRAWLINEY(slidsx-1,slidy,slidsx-1,slidy+1,darkgray,pitch)
   DRAWLINEX(slidsx-1,slidy+2,slidfx+1,slidy+2,lightgray,pitch)
   DRAWLINEY(slidfx+2,slidy-1,slidfx+2,slidy+2,lightgray,pitch)

   // Now draw marks
   for (m=0;m<10;m++)
   {
      int mx=slidsx+((slidfx-slidsx)*m)/9;
      DRAWLINEY(mx,marksy,mx,marksy+3,black,pitch)
   }

   // Now write some text describing the quality settings
   vclib_textwrite(buffer,pitch,sx+5,slidy,fx,fy,black,bcolour,16,"Quality");

   vclib_textwrite(buffer,pitch,slidsx-12,slidy,fx,fy,black,bcolour,16,"L");
   vclib_textwrite(buffer,pitch,slidfx+4,slidy,fx,fy,black,bcolour,16,"H");

   // Finally draw slider box
   boxx=slidsx+(((slidfx-slidsx)*pos)>>8);
   boxy=slidy;

   sx=boxx-5;
   fx=boxx+5;
   sy=boxy-10;
   fy=boxy+10;
   // First gray out the entire area
   for (x=sx;x<=fx;x++)
      for (y=sy;y<=fy;y++)
         buffer[MAKEPOS(x,y,pitch)]=bcolour;

   DRAWLINEX(sx,sy,fx,sy,lightgray,pitch)
   DRAWLINEY(sx,sy,sx,fy,lightgray,pitch)
   DRAWLINEY(fx,sy,fx,fy,black,pitch)
   DRAWLINEY(fx-1,sy+1,fx-1,fy-1,darkgray,pitch)
   DRAWLINEX(sx,fy,fx,fy,black,pitch)
   DRAWLINEX(sx+1,fy-1,fx-1,fy-1,darkgray,pitch)
}



/******************************************************************************
NAME
  vclib_textsize

SYNOPSIS
   int vclib_textsize(int font,char *text)

FUNCTION
   Calculates the width of the string

RETURNS
   The width of the string
******************************************************************************/

int vclib_textsize(int font,char *text)
{
   int start,end;
   int width=0;
   for (;text[0]!=0;text++)
   {
      int c=(int)text[0];
      switch (font)
      {
      case 32:
         if ((c<32) || (c>=128)) c=33; // Change unknown characters to !
         c-=32;
         if (c>0)
            start=fontpos[c-1];
         else
            start=0;
         end=fontpos[c];
         width+=end-start;
         break;
      case 16:

         if ((c<32) || (c>=128)) c=33; // Change unknown characters to !
         c-=32;

         /*
         if ((c>='A')&(c<='Z')) c=c-'A';
         else
         if ((c>='a')&(c<='z')) c=c-'a'+26;
         else
         if ((c>='0')&(c<='9')) c=c-'0'+26*2;
         else
         c=26*2+10;*/

         if (c>0)
            start=fontpos16[c-1];
         else
            start=0;
         end=fontpos16[c];
         width+=end-start;
         break;
      default:
         assert(0);
         break;
      }

   }
   return width;
}
