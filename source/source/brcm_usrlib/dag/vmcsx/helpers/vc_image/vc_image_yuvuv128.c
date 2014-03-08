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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "vc_image.h"
//#include "vclib/vclib.h"
#include "vcinclude/common.h"
#include "interface/vcos/vcos_assert.h"
#include "helpers/vclib/vclib.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vc_image_yuvuv128_to_yuv420(VC_IMAGE_T *dest, VC_IMAGE_T *src);
extern void vc_image_yuvuv128_from_yuv420(VC_IMAGE_T *dest, VC_IMAGE_T *src);

/* Check extern defs match above and loads #defines and typedefs */
#include "vc_image_yuvuv128.h"

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

extern void yuvuv128_from_yuv420_uvstrip(void *usrc, void *vsrc, void *dst,
         int height, int src_pitch);
extern void yuvuv128_from_yuv420_ystrip(void *ysrc, void *dst, int height,
                                           int src_pitch);
extern void yuvuv128_to_yuv420_ylaststrip(void *ydest, void *src, int height,
         int src_pitch, int width);
extern void yuvuv128_to_yuv420_uvlaststrip(void *udst, void *vdst, void *src,
         int height, int src_pitch, int width);
extern void yuvuv128_to_yuv420_uvstrip(void *udst, void *vdst, void *src,
                                          int height, int src_pitch);
extern void yuvuv128_to_yuv420_ystrip(void *ydest, void *src, int height,
                                         int src_pitch);

extern int     fseek64(FILE *fp, int64_t offset, int whence);

extern void yuvuv_difference_4mb (unsigned, unsigned char *, const unsigned char *, unsigned *, unsigned *);

extern void yuvuv128_vflip_strip(unsigned char *data, unsigned int height);
extern void yuvuv128_hflip_y_stripe(unsigned char *data, unsigned int nstrips, unsigned int pitch, unsigned int width);
extern void yuvuv128_hflip_uv_stripe(unsigned char *data, unsigned int nstrips, unsigned int pitch, unsigned int width);
extern void yuvuv128_transpose_y_block(unsigned char *dest, unsigned char *src, unsigned int src_width);
extern void yuvuv128_transpose_uv_block(unsigned char *dest, unsigned char *src, unsigned int src_width);
extern void yuvuv128_move_stripe(unsigned char *y, unsigned char *uv, unsigned int nstrips, unsigned int pitch, unsigned int offset);

void yuvuv128_transpose_block_y(
   void                *dst,        /* r0 */
   void          const *src,        /* r1 */
   int                  dst_height, /* r2 */
   int                  src_height);/* r3 */

void yuvuv128_transpose_block_uv(
   void                *dst,        /* r0 */
   void          const *src,        /* r1 */
   int                  dst_height, /* r2 */
   int                  src_height);/* r3 */

void yuvuv128_trans_sub_block_y(
   void                *dst,        /* r0 */
   void          const *src,        /* r1 */
   int                  dst_height, /* r2 */
   int                  src_height);/* r3 */

void yuvuv128_trans_sub_block_uv(
   void                *dst,        /* r0 */
   void          const *src,        /* r1 */
   int                  dst_height, /* r2 */
   int                  src_height);/* r3 */

void yuvuv128_subsample_128xn_y(
   void                *dst,        /* r0 */
   void          const *src,        /* r1 */
   int                  src_height);/* r2 */

void yuvuv128_subsample_64xn_uv(
   void                *dst,        /* r0 */
   void          const *src,        /* r1 */
   int                  src_height);/* r2 */



/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/


/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/

/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/

/******************************************************************************
Global function definitions.
******************************************************************************/

/******************************************************************************
NAME
   vc_image_yuvuv_from_yuv420_128

SYNOPSIS
   void vc_image_yuvuv_from_yuv420_128(VC_IMAGE_T *dest, VC_IMAGE_T *src)

FUNCTION
   Convert planar to sand yuv format (can't use in place).
   Interleaved Y and UV with stripe width of 128

RETURNS
   void
******************************************************************************/
void vc_image_yuvuv128_from_yuv420(VC_IMAGE_T *dest, VC_IMAGE_T *src)
{
   int i;
   unsigned char *inyptr,*inuptr,*invptr,*outyptr,*outuvptr;
   int nstrips;

//calculate number of stripes needed
   nstrips = ((src->width)+VC_IMAGE_YUV_UV_STRIPE_WIDTH-1)>>VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2;

//can move Y and UV in two separate processes
   inyptr = src->image_data;
   inuptr = src->extra.uv.u;
   invptr = src->extra.uv.v;
   outyptr = dest->image_data;
   outuvptr = dest->extra.uv.u;

   for (i=0;i<nstrips;i++)
   {
      yuvuv128_from_yuv420_ystrip((void *)inyptr, (void *)outyptr, src->height, src->pitch);
      yuvuv128_from_yuv420_uvstrip((void *)inuptr, (void *)invptr, (void *)outuvptr, (src->height)>>1, src->pitch>>1);
      inyptr += 128;  //move 1 strip to the right
      inuptr += 64;  //move 1 strip to the right
      invptr += 64;  //move 1 strip to the right
      outyptr += dest->pitch;
      outuvptr += dest->pitch;
   }
}

/******************************************************************************
NAME
   vc_image_yuv420_to_yuv_uv_128

SYNOPSIS
   void vc_image_yuv420_to_yuvv_128(VC_IMAGE_T *dest, VC_IMAGE_T *src)

FUNCTION
   Convert sand yuv to planar format (can't use in place).
   Interleaved Y and UV with stripe width of 128

RETURNS
   void
******************************************************************************/
void vc_image_yuvuv128_to_yuv420(VC_IMAGE_T *dest, VC_IMAGE_T *src)
{
   int i;
   unsigned char *inyptr,*inuvptr,*outyptr,*outuptr,*outvptr;
   int nstrips;
   volatile int stripwidth;
   int current_width=0;

//calculate number of stripes needed
   nstrips = ((src->width)+VC_IMAGE_YUV_UV_STRIPE_WIDTH-1)>>VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2;

//can move Y and UV in two separate processes
   outyptr = dest->image_data;
   outuptr = dest->extra.uv.u;
   outvptr = dest->extra.uv.v;
   inyptr = src->image_data;
   inuvptr = src->extra.uv.u;

   for (i=0;i<nstrips;i++)
   {
      if (current_width+128<=dest->pitch)
      {
         yuvuv128_to_yuv420_ystrip((void *)outyptr, (void *)inyptr, dest->height, dest->pitch);
         yuvuv128_to_yuv420_uvstrip((void *)outuptr, (void *)outvptr, (void *)inuvptr, (dest->height)>>1, dest->pitch>>1);
         current_width += 128;
      } else {
         stripwidth = dest->width - current_width;
         yuvuv128_to_yuv420_ylaststrip((void *)outyptr, (void *)inyptr, dest->height, dest->pitch, stripwidth);
         yuvuv128_to_yuv420_uvlaststrip((void *)outuptr, (void *)outvptr, (void *)inuvptr, (dest->height)>>1, dest->pitch>>1,stripwidth>>1);
      }

      outyptr += 128;  //move 1 strip to the right
      outuptr += 64;  //move 1 strip to the right
      outvptr += 64;  //move 1 strip to the right
      inyptr += src->pitch;
      inuvptr += src->pitch;
   }
}


void vc_image_yuvuv128_to_yuv420_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset)
{
   int i;
   unsigned char *inyptr,*inuvptr,*outyptr,*outuptr,*outvptr;
   int nstrips;
   volatile int stripwidth;
   int current_width=0;

   assert(x_offset == 0 && width == src->width); // other not supported

//calculate number of stripes needed
   nstrips = (width+VC_IMAGE_YUV_UV_STRIPE_WIDTH-1)>>VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2;

//can move Y and UV in two separate processes
   outyptr = (unsigned char *)dest->image_data + y_offset * dest->pitch + x_offset;
   outuptr = (unsigned char *)dest->extra.uv.u + (y_offset>>1) * (dest->pitch>>1) + (x_offset>>1);
   outvptr = (unsigned char *)dest->extra.uv.v + (y_offset>>1) * (dest->pitch>>1) + (x_offset>>1);
   inyptr  = (unsigned char *)src->image_data + src_y_offset * VC_IMAGE_YUV_UV_STRIPE_WIDTH;
   inuvptr = (unsigned char *)src->extra.uv.u + (src_y_offset>>1) * VC_IMAGE_YUV_UV_STRIPE_WIDTH;

   for (i=0;i<nstrips;i++)
   {
      if (current_width+128<=dest->pitch)
      {
         yuvuv128_to_yuv420_ystrip((void *)outyptr, (void *)inyptr, height, dest->pitch);
         yuvuv128_to_yuv420_uvstrip((void *)outuptr, (void *)outvptr, (void *)inuvptr, (height)>>1, dest->pitch>>1);
         current_width += 128;
      } else {
         stripwidth = dest->width - current_width;
         yuvuv128_to_yuv420_ylaststrip((void *)outyptr, (void *)inyptr, height, dest->pitch, stripwidth);
         yuvuv128_to_yuv420_uvlaststrip((void *)outuptr, (void *)outvptr, (void *)inuvptr, (height)>>1, dest->pitch>>1, stripwidth>>1);
      }

      outyptr += 128;  //move 1 strip to the right
      outuptr += 64;  //move 1 strip to the right
      outvptr += 64;  //move 1 strip to the right
      inyptr += src->pitch;
      inuvptr += src->pitch;
   }
}

void vc_image_yuvuv128_from_yuv420_part(
   VC_IMAGE_T          *dest,
   int                  x_offset,
   int                  y_offset,
   int                  width,
   int                  height,
   VC_IMAGE_T          *src,
   int                  src_x_offset,
   int                  src_y_offset)
{
   int i;
   unsigned char *inyptr,*inuptr,*invptr,*outyptr,*outuvptr;
   int nstrips;

   assert(x_offset == 0 && width == src->width); // other not supported

//calculate number of stripes needed
   nstrips = ((src->width)+VC_IMAGE_YUV_UV_STRIPE_WIDTH-1)>>VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2;

//can move Y and UV in two separate processes
   inyptr = (unsigned char *)src->image_data + src_y_offset * src->pitch + src_x_offset;
   inuptr = (unsigned char *)src->extra.uv.u + (src_y_offset>>1) * (src->pitch>>1) + (src_x_offset>>1);
   invptr = (unsigned char *)src->extra.uv.v + (src_y_offset>>1) * (src->pitch>>1) + (src_x_offset>>1);
   outyptr = (unsigned char *)dest->image_data + y_offset * VC_IMAGE_YUV_UV_STRIPE_WIDTH;
   outuvptr = (unsigned char *)dest->extra.uv.u + (y_offset>>1) * VC_IMAGE_YUV_UV_STRIPE_WIDTH;

   for (i=0;i<nstrips;i++)
   {
      yuvuv128_from_yuv420_ystrip((void *)inyptr, (void *)outyptr, src->height, src->pitch);
      yuvuv128_from_yuv420_uvstrip((void *)inuptr, (void *)invptr, (void *)outuvptr, (src->height)>>1, src->pitch>>1);
      inyptr += 128;  //move 1 strip to the right
      inuptr += 64;  //move 1 strip to the right
      invptr += 64;  //move 1 strip to the right
      outyptr += dest->pitch;
      outuvptr += dest->pitch;
   }
}


// Write out the YUV frames as one file,  for the first frame numframe==0
void vc_save_YUV_1file( VC_IMAGE_T *frame,
                  const char                * const sz_filename, int framenum )
{
   unsigned long  hgt=(frame->pitch>>7);
   FILE   *fid = NULL;
   int ul_yAddr, ul_uvAddr;
   int tilewidth=128;
   int numtiles=(frame->width+tilewidth-1)/tilewidth;
   int y,tile;
   int h=hgt;

   assert(frame->type==VC_IMAGE_YUV_UV);
   assert(VC_IMAGE_YUV_UV_STRIPE_WIDTH==128);

   ul_yAddr = (int)vc_image_get_y(frame);
   ul_uvAddr = (int)vc_image_get_u(frame);

   if( framenum == 0 )
   	fid=fopen( sz_filename, "wb" );
   else
   	fid=fopen( sz_filename, "ab" );   	
   assert(fid);

   fseek64(fid,(long long)(frame->width*frame->height*framenum*3 >> 1),SEEK_SET);
   
   for (y=0;y<frame->height;y++)
   {
      int x=0;
      for (tile=0;tile<numtiles;tile++,x+=tilewidth)
      {
         fwrite( (void*)(ul_yAddr+tile*tilewidth*h+y*tilewidth),1,_min(tilewidth,frame->width-x),fid);
      }
   }

   for (y=0;y<frame->height>>1;y++)
   {
      int x=0;
      int i;
      unsigned char u[640];
      for (tile=0;tile<numtiles;tile++,x+=tilewidth)
      {
         unsigned char *p=(unsigned char*)(ul_uvAddr+tile*tilewidth*(h)+y*tilewidth);
         for (i=0;i<tilewidth/2;i++)
            u[i]=p[i*2];
         fwrite( u,1,_min(tilewidth,frame->width-x)/2,fid);
      }
   }

   for (y=0;y<frame->height>>1;y++)
   {
      int x=0;
      int i;
      unsigned char u[640];
      for (tile=0;tile<numtiles;tile++,x+=tilewidth)
      {
         unsigned char *p=(unsigned char*)(ul_uvAddr+tile*tilewidth*(h)+y*tilewidth);
         for (i=0;i<tilewidth/2;i++)
            u[i]=p[i*2+1];
         fwrite( u,1,_min(tilewidth,frame->width-x)/2,fid);
      }
   }

   fclose( fid );
   fid = NULL;

} /* vc_save_YUV */







// Write out the YUV frames
void vc_save_YUV( VC_IMAGE_T *frame,
                  const char                * const sz_filename)
{
   unsigned long  hgt=(frame->pitch>>7);
   FILE   *fid = NULL;
   int ul_yAddr, ul_uvAddr;
   int tilewidth=128;
   int numtiles=(frame->width+tilewidth-1)/tilewidth;
   int y,tile;
   int h=hgt;

   assert(frame->type==VC_IMAGE_YUV_UV);
   assert(VC_IMAGE_YUV_UV_STRIPE_WIDTH==128);

   ul_yAddr = (int)vc_image_get_y(frame);
   ul_uvAddr = (int)vc_image_get_u(frame);

   fid=fopen( sz_filename, "wb" );
   assert(fid);

   for (y=0;y<frame->height;y++)
   {
      int x=0;
      for (tile=0;tile<numtiles;tile++,x+=tilewidth)
      {
         fwrite( (void*)(ul_yAddr+tile*tilewidth*h+y*tilewidth),1,_min(tilewidth,frame->width-x),fid);
      }
   }

   for (y=0;y<frame->height>>1;y++)
   {
      int x=0;
      int i;
      unsigned char u[640];
      for (tile=0;tile<numtiles;tile++,x+=tilewidth)
      {
         unsigned char *p=(unsigned char*)(ul_uvAddr+tile*tilewidth*(h)+y*tilewidth);
         for (i=0;i<tilewidth/2;i++)
            u[i]=p[i*2];
         fwrite( u,1,_min(tilewidth,frame->width-x)/2,fid);
      }
   }

   for (y=0;y<frame->height>>1;y++)
   {
      int x=0;
      int i;
      unsigned char u[640];
      for (tile=0;tile<numtiles;tile++,x+=tilewidth)
      {
         unsigned char *p=(unsigned char*)(ul_uvAddr+tile*tilewidth*(h)+y*tilewidth);
         for (i=0;i<tilewidth/2;i++)
            u[i]=p[i*2+1];
         fwrite( u,1,_min(tilewidth,frame->width-x)/2,fid);
      }
   }

   fclose( fid );
   fid = NULL;

} /* vc_save_YUV */

// Load a YUVUV image into the provided storage space
// vc_image can actually be smaller than the YUV image if desired
void vc_load_YUVUV( VC_IMAGE_T *frame,
                    const char                * const sz_filename,int yuvwidth,int yuvheight)
{
   unsigned long  hgt=(frame->pitch>>7);
   FILE   *fid = NULL;
   int ul_yAddr, ul_uvAddr;
   int tilewidth=128;
   int numtiles=(frame->width+tilewidth-1)/tilewidth;
   int y,tile;
   int h=hgt;
   char temp[1024];

   assert(frame->type==VC_IMAGE_YUV_UV);
   assert(VC_IMAGE_YUV_UV_STRIPE_WIDTH==128);

   ul_yAddr = (int)vc_image_get_y(frame);
   ul_uvAddr = (int)vc_image_get_u(frame);

   fid=fopen( sz_filename, "rb" );
   assert(fid);

   for (y=0;y<frame->height;y++)
   {
      int x=0;
      for (tile=0;tile<numtiles;tile++,x+=tilewidth)
      {
         fread( (void*)(ul_yAddr+tile*tilewidth*h+y*tilewidth),1,_min(tilewidth,frame->width-x),fid);
      }
   }

   for (;y<yuvheight;y++)
   {
      fread(temp,yuvwidth,1,fid);  // skip unneeded lines
   }

   for (y=0;y<frame->height>>1;y++)
   {
      int x=0;
      /*int i;*/
      for (tile=0;tile<numtiles;tile++,x+=tilewidth)
      {
         unsigned char *p=(unsigned char*)(ul_uvAddr+tile*tilewidth*(h)+y*tilewidth);
         fread( p,1,_min(tilewidth,frame->width-x),fid);
      }
   }

   /*for(y=0;y<frame->height>>1;y++)
   {
      int x=0;
      int i;
      unsigned char u[640];
      for(tile=0;tile<numtiles;tile++,x+=tilewidth)
      {
         unsigned char *p=(unsigned char*)(ul_uvAddr+tile*tilewidth*(h)+y*tilewidth);
         fread( u,1,_min(tilewidth,frame->width-x)/2,fid);
         for(i=0;i<tilewidth/2;i++)
            p[i*2]=u[i];
      
      }
   }

   for(;y<yuvheight>>1;y++)
      {
         fread(temp,yuvwidth>>1,1,fid);  // skip unneeded lines
      }

   for(y=0;y<frame->height>>1;y++)
   {
      int x=0;
      int i;
      unsigned char u[640];
      for(tile=0;tile<numtiles;tile++,x+=tilewidth)
      {
         unsigned char *p=(unsigned char*)(ul_uvAddr+tile*tilewidth*(h)+y*tilewidth);
      
         fread( u,1,_min(tilewidth,frame->width-x)/2,fid);
         for(i=0;i<tilewidth/2;i++)
            p[i*2+1]=u[i];
      }
   }*/

   fclose( fid );
   fid = NULL;

} /* vc_load_YUV */


// Load a YUVUV image into the provided storage space
void vc_load_YUV( VC_IMAGE_T *frame,
                  const char                * const sz_filename,int framenum)
{
   unsigned long  hgt=(frame->pitch>>7);
   FILE   *fid = NULL;
   int ul_yAddr, ul_uvAddr;
   int tilewidth=128;
   int numtiles=(frame->width+tilewidth-1)/tilewidth;
   int y,tile;
   int h=hgt;
/*   char temp[1024];*/

   assert(frame->type==VC_IMAGE_YUV_UV);
   assert(VC_IMAGE_YUV_UV_STRIPE_WIDTH==128);

   ul_yAddr = (int)vc_image_get_y(frame);
   ul_uvAddr = (int)vc_image_get_u(frame);

   fid=fopen( sz_filename, "rb" );
   assert(fid);

   // Seek in units of 200 frames to avoid overflow
   while (framenum>200)
   {
      fseek64(fid,(long long)(frame->width*frame->height*200*3 >> 1),SEEK_CUR);
      framenum-=200;
   }
   fseek64(fid,(long long)(frame->width*frame->height*framenum*3 >> 1),SEEK_CUR);


   for (y=0;y<frame->height;y++)
   {
      int x=0;
      for (tile=0;tile<numtiles;tile++,x+=tilewidth)
      {
         fread( (void*)(ul_yAddr+tile*tilewidth*h+y*tilewidth),1,_min(tilewidth,frame->width-x),fid);
      }  // title( which pitch) titlewidth(stripewidth=128)  h(height of pitch,including yuv and padding)
   }

   for (y=0;y<frame->height>>1;y++)
   {
      int x=0;
      int i;
      unsigned char u[640];
      for (tile=0;tile<numtiles;tile++,x+=tilewidth)
      {
         unsigned char *p=(unsigned char*)(ul_uvAddr+tile*tilewidth*(h)+y*tilewidth);
         fread( u,1,_min(tilewidth,frame->width-x)/2,fid);
         for (i=0;i<tilewidth/2;i++)
            p[i*2]=u[i];

      }
   }

   for (y=0;y<frame->height>>1;y++)
   {
      int x=0;
      int i;
      unsigned char u[640];
      for (tile=0;tile<numtiles;tile++,x+=tilewidth)
      {
         unsigned char *p=(unsigned char*)(ul_uvAddr+tile*tilewidth*(h)+y*tilewidth);

         fread( u,1,_min(tilewidth,frame->width-x)/2,fid);
         for (i=0;i<tilewidth/2;i++)
            p[i*2+1]=u[i];
      }
   }

   fclose( fid );
   fid = NULL;

} /* vc_load_YUV */



/*
** Vector routine to difference a pair of images ( d |-|= a ).
** Should probably dcache flush after this if using via uncached alias?
*/
void vc_image_difference( VC_IMAGE_T             * const d,          /* in/out */
                          const VC_IMAGE_T       * const a,          /* in     */
                          float                  * const psnr_y,     /*    out */
                          float                  * const psnr_uv,    /*    out */
                          unsigned               * const max_y,      /*    out */
                          unsigned               * const max_uv      /*    out */ )
{
   unsigned  x, y, width_64, last_width, height_16, n = 0;

   /* Currently requires source to have pitch 128 */
   assert( d->type == VC_IMAGE_YUV_UV );
   assert( a->type == VC_IMAGE_YUV_UV );
   assert( a->width  == d->width  );
   assert( a->height == d->height );
   assert( 0 == ((unsigned)a->width & 0xf ) ); /* we could deal with widths in multiples of 4-pixels, so call if you need to */

   width_64   = ( d->width  + 63 ) >> 6;
   last_width = 4U - ( (unsigned)( ( width_64 << 6U ) - d->width ) >> 4U ); /* 1 - 4 columns of MBs */
   assert( 0 <  last_width );
   assert( 4 >= last_width );

   height_16  = ( d->height + 15U ) >> 4U;

   *psnr_y  = 1.0e-20f; /* avoid divide by zero */
   *psnr_uv = 1.0e-20f;
   *max_y   = 0;
   *max_uv  = 0;

   for( x = 0; x < width_64; x++ )
   {
      const unsigned char *ay, *auv;
      unsigned char       *dy, *duv;

      const unsigned      column_flags[ 5 ] = { 0U, 0xf, 0xff, 0xfff, 0xffff };
      const int           last_column       = ( width_64 - 1 ) == x;
      const unsigned      column_width      = last_column ? last_width : 4U;
      const unsigned      column_flag       = column_flags[ column_width ];

      ay  = (const unsigned char *)vc_image_get_y( a ) + (x>>1) * a->pitch + (x&1) * 64;
      auv = (const unsigned char *)vc_image_get_u( a ) + (x>>1) * a->pitch + (x&1) * 64;
      dy  =       (unsigned char *)vc_image_get_y( d ) + (x>>1) * d->pitch + (x&1) * 64;
      duv =       (unsigned char *)vc_image_get_u( d ) + (x>>1) * d->pitch + (x&1) * 64;

      for ( y = 0; y < height_16; y++ )
      {
         unsigned  p = 0, m = 0;

         yuvuv_difference_4mb( 0x00100000 /* 16 rows */ | column_flag, dy, ay, &p, &m );
         dy += 128 * 16;
         ay += 128 * 16;

         *psnr_y += (float)p * ( 1.0 / ( ( (float)column_width * 16.0 ) * 16.0  ) );
         if ( *max_y < m )
         {
            *max_y = m;
         } /* if */

         yuvuv_difference_4mb( 0x00080000 /* 8 rows */ | column_flag, duv, auv, &p, &m );
         duv += 128 * 8;
         auv += 128 * 8;

         *psnr_uv += (float)p * ( 1.0 / ( ( (float)column_width * 16.0 ) * 8.0  ) );
         if ( *max_uv < m )
         {
            *max_uv = m;
         } /* if */

         n++;
      } /* for */
   } /* for */

   assert( 0 < n );
   *psnr_y  = 10.0 * log10( 255.0 * 255.0 * (float)n / *psnr_y  );
   *psnr_uv = 10.0 * log10( 255.0 * 255.0 * (float)n / *psnr_uv );

} /* vc_image_difference */



void vc_subsample_scalar(VC_IMAGE_T *sub,VC_IMAGE_T *cur)
{
   int x, y, width_64, height_16, cur_slab, sub_slab;

   // Make half sized image. Currently requires source to have pitch 128, dest 32.
   vcos_assert(is_valid_vc_image_buf(cur, VC_IMAGE_YUV_UV));
   vcos_assert(is_valid_vc_image_buf(sub, VC_IMAGE_YUV_UV32));
   assert(sub->width*2 >= cur->width);
   assert(sub->height*2 >= cur->height);

   width_64   = (cur->width + 63) >> 6;
   height_16  = (cur->height + 15) >> 4;
   cur_slab   = cur->pitch;
   sub_slab   = sub->pitch;

   for (x = 0; x < width_64; ++x) {
      const unsigned char *cy, *cc;
      unsigned char *sy, *sc;
      cy = (const unsigned char *)vc_image_get_y(cur) + (x>>1)*cur_slab + (x&1)*64;
      cc = (const unsigned char *)vc_image_get_u(cur) + (x>>1)*cur_slab + (x&1)*64;
      sy = (unsigned char *)vc_image_get_y(sub)       + x*sub_slab;
      sc = (unsigned char *)vc_image_get_u(sub)       + x*sub_slab;
      for (y = 0; y < height_16; ++y) {

         int  xx, yy;

         for( yy = 0; 8 > yy; yy++ )
         {
            for( xx = 0; 32 > xx; xx++ )
            {
               sy[ 32 * yy + xx ] = (unsigned char)( ( 2 +
                  cy[ 256 * yy + 2 * xx       ] + cy[ 256 * yy + 2 * xx +       1 ] +
                  cy[ 256 * yy + 2 * xx + 128 ] + cy[ 256 * yy + 2 * xx + 128 + 1 ] ) / 4 );
            } /* for */
         } /* for */

         for( yy = 0; 4 > yy; yy++ )
         {
            for( xx = 0; 16 > xx; xx++ )
            {
               sc[ 32 * yy + 2 * xx ] = (unsigned char)( ( 2 +
                  cc[ 256 * yy + 4 * xx       ] + cc[ 256 * yy + 4 * xx +       2 ] +
                  cc[ 256 * yy + 4 * xx + 128 ] + cc[ 256 * yy + 4 * xx + 128 + 2 ] ) / 4 );
               sc[ 32 * yy + 2 * xx + 1 ] = (unsigned char)( ( 2 +
                  cc[ 256 * yy + 4 * xx       + 1 ] + cc[ 256 * yy + 4 * xx +       3 ] +
                  cc[ 256 * yy + 4 * xx + 128 + 1 ] + cc[ 256 * yy + 4 * xx + 128 + 3 ] ) / 4 );
            } /* for */
         } /* for */

         sy +=  32 *  8;
         sc +=  32 *  4;
         cy += 128 * 16;
         cc += 128 *  8;
      } /* for */
   } /* for */

} /* vc_subsample_scalar */



static int imin( int a, int b ) {
   return( ( a < b ) ? a : b ); } /* imin */



void vc_load_image( const char         * const sz_file_fmt,          /* in     */
                    const unsigned     iim,                          /* in     */
                    const int          pitch,                        /* in     */
                    const int          i_height,                     /* in     */ /* (i)nput image: i.e. raw file */
                    const int          i_width,                      /* in     */
                    const int          o_height,                     /* in     */ /* (o)utput image: i.e. framebuffer */
                    const int          o_width,                      /* in     */
                    const VC_IMAGE_T   * const frame                 /* in     */ )
{
   const int      i_height2 = i_height / 2;
   const int      i_width2  = i_width  / 2;
   const int      o_height2 = o_height / 2;
   const int      o_width2  = o_width  / 2;

   unsigned char  buf[ 2050 ];
   FILE           *fid = NULL;
   int            i, j, w, h, hh, stripestart;
   unsigned char  *y, *uv;

   assert( 0 < i_height );
   assert( 0 < i_width  );
   assert( 0 == ( (unsigned)i_height & 0xF ) );
   assert( 0 == ( (unsigned)i_width  & 0xF ) );
   assert( (int)sizeof( buf ) >= i_width );

   assert( 0 < o_height );
   assert( 0 < o_width  );
   assert( 0 == ( (unsigned)o_height & 0xF ) );
   assert( 0 == ( (unsigned)o_width  & 0xF ) );
   assert( (int)sizeof( buf ) >= o_width );

   assert( VC_IMAGE_YUV_UV_STRIPE_WIDTH == pitch );

   y  = vc_image_get_y( frame );
   uv = vc_image_get_u( frame );

   /*
   ** Either we have lots of little image files or one giant image sequence compendium
   */
   if ( NULL != strchr( sz_file_fmt, '%' ) )
   {
      snprintf( (char *)buf, sizeof( buf ), sz_file_fmt, iim );
      buf[ sizeof( buf ) - 1 ] = '\0';

      fid = fopen( (const char *)buf, "rb" );
   }
   else
   {
      unsigned  skip = iim;

      fid = fopen( sz_file_fmt, "rb" );
      assert( fid );

      /* Seek in units of 200 frames to avoid overflow */
      while ( 200 < skip )
      {
         fseek64( fid, (long long)( ( i_width * i_height * 200 * 3 ) >> 1 ), SEEK_CUR );
         skip -= 200;
      } /* while */
      fseek64( fid, (long long)( ( i_width * i_height * skip * 3 ) >> 1 ), SEEK_CUR );
   } /* else */
   assert( fid );


   /* Y */
   for ( h = 0; i_height > h; h++ )
   {
      fread( buf, i_width, 1, fid );
      if ( o_height <= h )
      {
         continue;
      } /* if */
      for ( i = i_width; o_width > i; i++ ) {
         buf[ i ] = buf[ i % i_width ]; } /* for */

      for ( hh = h; o_height > hh; hh += i_height )
      {
         for ( j = 0, w = 0, stripestart = 0; o_width > w; w += pitch, stripestart += frame->pitch )
         {
            const int  ww = imin( o_width - w, pitch );

            for ( i = 0; ww > i; i++ )
            {
               y[ i + hh * pitch + stripestart ] = buf[ j++ ];
            } /* for */
         } /* for */
      } /* for */
   } /* for */


   /* U */
   for ( h = 0; i_height2 > h; h++ )
   {
      fread( buf, i_width2, 1, fid );
      if ( o_height2 <= h )
      {
         continue;
      } /* if */
      for ( i = i_width2; o_width2 > i; i++ ) {
         buf[ i ] = buf[ i % i_width2 ]; } /* for */

      for ( hh = h; o_height2 > hh; hh += i_height2 )
      {
         for ( j = 0, w = 0, stripestart = 0; o_width > w; w += pitch, stripestart += frame->pitch )
         {
            const int  ww = imin( o_width - w, pitch );

            for ( i = 0; i < ww; i += 2 )
            {
               uv[ i + hh * pitch + stripestart ] = buf[ j++ ];
            } /* for */
         } /* for */
      } /* for */
   } /* for */

   /* V */
   for ( h = 0; i_height2 > h; h++ )
   {
      fread( buf, i_width2, 1, fid );
      if ( o_height2 <= h )
      {
         break;
      } /* if */
      for ( i = i_width2; o_width2 > i; i++ ) {
         buf[ i ] = buf[ i % i_width2 ]; } /* for */

      for ( hh = h; o_height2 > hh; hh += i_height2 )
      {
         for ( j = 0, w = 0, stripestart = 0; o_width > w; w += pitch, stripestart += frame->pitch )
         {
            const int  ww = imin( o_width - w, pitch );

            for ( i = 1; i < ww; i += 2 )
            {
               uv[ i + hh * pitch + stripestart ] = buf[ j++ ];
            } /* for */
         } /* for */
      } /* for */
   } /* for */


   fclose( fid );
   fid = NULL;

} /* vc_load_image */



static
int STRENGTH_h263( const int                     QUANT               /* in     */ )
{
   static const int  lut[] = { 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 12 };

   assert( 0 < QUANT );

   if ( 31 < QUANT )
   {
      return( 12 );
   } /* if */

   return( lut[ QUANT ] );

} /* STRENGTH_h263 */



extern void vc_dblk_y_4(  unsigned nrows, unsigned char *dest, const unsigned char *src, int strength );
extern void vc_dblk_y_8(  unsigned nrows, unsigned char *dest, const unsigned char *src, int strength );
extern void vc_dblk_uv_4( unsigned nrows, unsigned char *dest, const unsigned char *src, int strength );
extern void vc_dblk_uv_8( unsigned nrows, unsigned char *dest, const unsigned char *src, int strength );



void vc_dblk_y( const int                        nrows,              /* in     */
                unsigned char                    * const dest,       /*    out */
                const unsigned char              * const src,        /* in     */
                const int                        strength            /* in     */ )
{
   assert( 0 < nrows );
   
   if ( 4 >= nrows )
   {
      vc_dblk_y_4( (unsigned)nrows, dest, src, strength );
   }
   else if ( 8 >= nrows )
   {
      vc_dblk_y_8( (unsigned)nrows, dest, src, strength );
   }
   else
   {
      vc_dblk_y_8( 8U, dest, src, strength );
   } /* else */

} /* vc_dblk_y */



void vc_dblk_uv( const int                       nrows,              /* in     */
                 unsigned char                   * const dest,       /*    out */
                 const unsigned char             * const src,        /* in     */
                 const int                       strength            /* in     */ )
{
   assert( 0 < nrows );
   
   if ( 4 >= nrows )
   {
      vc_dblk_uv_4( (unsigned)nrows, dest, src, strength );
   }
   else if ( 8 >= nrows )
   {
      vc_dblk_uv_8( (unsigned)nrows, dest, src, strength );
   }
   else
   {
      vc_dblk_uv_8( 8U, dest, src, strength );
   } /* else */

} /* vc_dblk_uv */



/*
** The h.263 Annex J deblocker
** ("dest" & "src" may point to the same storage.)
*/
void vc_image_deblock_h263( VC_IMAGE_T           * const dest,       /*(in)out */
                            const VC_IMAGE_T     * const src,        /* in     */
                            const int            QUANT               /* in     */ )
{
   /* Requires image to have pitch 128 */
   assert( dest->type == VC_IMAGE_YUV_UV );
   assert(  src->type == VC_IMAGE_YUV_UV );
   assert( dest->width  == src->width  );
   assert( dest->height == src->height );

   /* no test on width -- it is OK to overrun up to the next multiple of 16 because that memory is allocated to the images */
   assert( 8 <= dest->height ); /* no need to check the first four (uv!) rows */
   assert( 0 == ( (unsigned)dest->height & 0x1 ) );

   const int            STRENGTH = STRENGTH_h263( QUANT );
   const unsigned       width_16 = ( (unsigned)dest->width + 15U ) >> 4U;

   unsigned             x, y;

   assert( vclib_check_VRF() );

   /*
   ** luma
   */
   unsigned char        *d_y;
   const unsigned char  *s_y;

   /* first 4 rows */
   s_y = (unsigned char *)vc_image_get_y( src );
   vc_dblk_y_4( 4, NULL, s_y, STRENGTH );

   for( x = 1; width_16 > x; x++ )
   {
      d_y = (unsigned char *)vc_image_get_y( dest ) + ((unsigned)(x-1)>>3U) * dest->pitch + (((unsigned)(x-1)&0x7)<<4U);
      s_y = (unsigned char *)vc_image_get_y(  src ) + (x>>3U) * src->pitch + ((x&0x7)<<4U);
      vc_dblk_y_4( 4, d_y, s_y, STRENGTH );
   } /* for */

   d_y = (unsigned char *)vc_image_get_y( dest ) + ((unsigned)(width_16-1)>>3U) * dest->pitch + (((unsigned)(width_16-1)&0x7)<<4U);
   vc_dblk_y_4( 4, d_y, NULL, STRENGTH );


   /* subsequent rows */
   for( y = 4; (unsigned)dest->height > y; y += 8 )
   {
      const int  nrows = dest->height - (int)y;
      s_y = (unsigned char *)vc_image_get_y( src ) + 128 * y;
      vc_dblk_y( nrows, NULL, s_y, STRENGTH );

      for( x = 1; width_16 > x; x++ )
      {
         d_y = (unsigned char *)vc_image_get_y( dest ) + ((unsigned)(x-1)>>3U) * dest->pitch + (((unsigned)(x-1)&0x7)<<4U) + 128 * y;
         s_y = (unsigned char *)vc_image_get_y(  src ) + (x>>3U) * src->pitch + ((x&0x7)<<4U) + 128 * y;
         vc_dblk_y( nrows, d_y, s_y, STRENGTH );
      } /* for */

      d_y = (unsigned char *)vc_image_get_y( dest ) + ((unsigned)(width_16-1)>>3U) * dest->pitch + (((unsigned)(width_16-1)&0x7)<<4U) + 128 * y;
      vc_dblk_y( nrows, d_y, NULL, STRENGTH );
   } /* for */



   /*
   ** chroma
   */
   unsigned char        *d_uv;
   const unsigned char  *s_uv;

   s_uv = (unsigned char *)vc_image_get_u( src );
   vc_dblk_uv_4( 4, NULL, s_uv, STRENGTH );

   for( x = 1; width_16 > x; x++ )
   {
      d_uv = (unsigned char *)vc_image_get_u( dest ) + ((unsigned)(x-1)>>3U) * dest->pitch + (((unsigned)(x-1)&0x7)<<4U);
      s_uv = (unsigned char *)vc_image_get_u(  src ) + (x>>3U) * src->pitch + ((x&0x7)<<4U);
      vc_dblk_uv_4( 4, d_uv, s_uv, STRENGTH );
   } /* for */

   d_uv = (unsigned char *)vc_image_get_u( dest ) + ((unsigned)(width_16-1)>>3U) * dest->pitch + (((unsigned)(width_16-1)&0x7)<<4U);
   vc_dblk_uv_4( 4, d_uv, NULL, STRENGTH );


   /* subsequent rows */
   const unsigned  chroma_height = (unsigned)dest->height >> 1U;

   for( y = 4; chroma_height > y; y += 8 )
   {
      const int  nrows = (int)chroma_height - (int)y;
      s_uv = (unsigned char *)vc_image_get_u( src ) + 128 * y;
      vc_dblk_uv( nrows, NULL, s_uv, STRENGTH );

      for( x = 1; width_16 > x; x++ )
      {
         d_uv = (unsigned char *)vc_image_get_u( dest ) + ((unsigned)(x-1)>>3U) * dest->pitch + (((unsigned)(x-1)&0x7)<<4U) + 128 * y;
         s_uv = (unsigned char *)vc_image_get_u(  src ) + (x>>3U) * src->pitch + ((x&0x7)<<4U) + 128 * y;
         vc_dblk_uv( nrows, d_uv, s_uv, STRENGTH );
      } /* for */

      d_uv = (unsigned char *)vc_image_get_u( dest ) + ((unsigned)(width_16-1)>>3U) * dest->pitch + (((unsigned)(width_16-1)&0x7)<<4U) + 128 * y;
      vc_dblk_uv( nrows, d_uv, NULL, STRENGTH );
   } /* for */

} /* vc_image_deblock_h263 */


/******************************************************************************
NAME
   vc_image_vflip_in_place_yuvuv128

SYNOPSIS
   void vc_image_vflip_in_place_yuvuv128(VC_IMAGE_T *img)

FUNCTION
   Vertically flip the image in place

RETURNS
   void
******************************************************************************/
void vc_image_vflip_in_place_yuvuv128(VC_IMAGE_T *img)
{
   int i, nstrips;
   unsigned char *yptr,*uvptr;

   nstrips = ((img->width)+VC_IMAGE_YUV_UV_STRIPE_WIDTH-1)>>VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2;

   yptr = img->image_data;
   uvptr = img->extra.uv.u;

   for(i=0; i<nstrips; i++)
   {
      yuvuv128_vflip_strip(yptr, img->height);
      yuvuv128_vflip_strip(uvptr, img->height>>1);

      yptr += img->pitch;
      uvptr += img->pitch;
   }
}

/******************************************************************************
NAME
   vc_image_hflip_in_place_yuvuv128

SYNOPSIS
   void vc_image_hflip_in_place_yuvuv128(VC_IMAGE_T *img)

FUNCTION
   Horizontally flip the image in place

RETURNS
   void
******************************************************************************/
void vc_image_hflip_in_place_yuvuv128(VC_IMAGE_T *img)
{
   int i, nstrips;

   nstrips = ((img->width)+VC_IMAGE_YUV_UV_STRIPE_WIDTH-1)>>VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2;

   for(i=0; i<img->height; i+=16)
   {
      yuvuv128_hflip_y_stripe((unsigned char *) img->image_data + (i<<7), nstrips, img->pitch, img->width);
      yuvuv128_hflip_uv_stripe((unsigned char *) img->extra.uv.u + (i<<6), nstrips, img->pitch, img->width);
   }
}


/******************************************************************************
NAME
   vc_image_transpose_yuvuv128

SYNOPSIS
   void vc_image_transpose_yuvuv128(VC_IMAGE_T *dest, VC_IMAGE_T *src);

FUNCTION
   Transpose src into dest

RETURNS
   void
******************************************************************************/
#if 1 /* experimental new code which should perform fewer page flips */
void vc_image_transpose_yuvuv128(VC_IMAGE_BUF_T *dst, VC_IMAGE_BUF_T *src)
{
   unsigned char *dptr;
   unsigned char const *sptr;
   int x, y;

   vcos_assert(is_valid_vc_image_buf(dst, VC_IMAGE_YUV_UV));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV_UV));
   vcos_assert((dst->height + 15 & ~15) >= src->width);
   vcos_assert((dst->width + VC_IMAGE_YUV_UV_STRIPE_WIDTH - 1 & -VC_IMAGE_YUV_UV_STRIPE_WIDTH) >= src->height);
   vcos_assert(VC_IMAGE_YUV_UV_STRIPE_WIDTH == 128);
   vcos_assert(vclib_check_VRF());

   for (x = 0; x < dst->width; x += VC_IMAGE_YUV_UV_STRIPE_WIDTH)
   {
      int src_height = min(src->height - x, VC_IMAGE_YUV_UV_STRIPE_WIDTH);
      
      dptr = (unsigned char *)dst->image_data + (x >> VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2) * dst->pitch;
      sptr = (unsigned char const *)src->image_data + x * VC_IMAGE_YUV_UV_STRIPE_WIDTH;

      for (y = dst->height; y > 0; y -= 128)
      {
         int dst_height = min(y, 128);

         yuvuv128_transpose_block_y(dptr, sptr, dst_height, src_height);
         dptr += VC_IMAGE_YUV_UV_STRIPE_WIDTH * dst_height;
         sptr += src->pitch;
      }

      src_height >>= 1;
      dptr = (unsigned char *)dst->extra.uv.u + (x >> VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2) * dst->pitch;
      sptr = (unsigned char const *)src->extra.uv.u + x * (VC_IMAGE_YUV_UV_STRIPE_WIDTH >> 1);

      for (y = dst->height + 1 >> 1; y > 0; y -= 64)
      {
         int dst_height = min(y, 64);

         yuvuv128_transpose_block_uv(dptr, sptr, dst_height, src_height);
         dptr += VC_IMAGE_YUV_UV_STRIPE_WIDTH * dst_height;
         sptr += src->pitch;
      }
   }
}
#else
void vc_image_transpose_yuvuv128(VC_IMAGE_T *dest, VC_IMAGE_T *src)
{
   int i, j, h_nstrips, w_nstrips, width;
   unsigned char *srcy = src->image_data;
   unsigned char *desty = dest->image_data;
   unsigned char *srcuv = src->extra.uv.u;
   unsigned char *destuv = dest->extra.uv.u;

   h_nstrips = ((src->width)+VC_IMAGE_YUV_UV_STRIPE_WIDTH-1)>>VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2;
   w_nstrips = ((src->height)+VC_IMAGE_YUV_UV_STRIPE_WIDTH-1)>>VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2;
   width = ((src->width-1) & 127) + 1;

   vclib_release_VRF();
   // i is height in the destination image
   for(i=h_nstrips-1; i>=0; i--)
   {
      vclib_obtain_VRF(1);
      // j is width in the destination image
      for(j=0; j<w_nstrips; j++)
      {
         yuvuv128_transpose_y_block(desty + (i*128*128) + (j*dest->pitch),
                                    srcy + (j*128*128) + (i*src->pitch),
                                    width);

         yuvuv128_transpose_uv_block(destuv + (i*64*128) + (j*dest->pitch),
                                     srcuv + (j*64*128) + (i*src->pitch),
                                     width);
      }
      vclib_release_VRF();
      
      width = 128;
   }
   vclib_obtain_VRF(1);
}
#endif

   
/******************************************************************************
NAME
   vc_image_transpose_and_subsample_yuvuv128

SYNOPSIS
   void vc_image_transpose_and_subsample_yuvuv128(VC_IMAGE_BUF_T *dst, VC_IMAGE_BUF_T *src)

FUNCTION
   blah blah blah
******************************************************************************/

void vc_image_transpose_and_subsample_yuvuv128(VC_IMAGE_BUF_T *dst, VC_IMAGE_BUF_T *src)
{
   unsigned char *dptr;
   unsigned char const *sptr;
   int x, y;

   vcos_assert(is_valid_vc_image_buf(dst, VC_IMAGE_YUV_UV));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV_UV));
   assert((dst->height + 15 & ~15) >= (src->width >> 1));
   assert((dst->width + VC_IMAGE_YUV_UV_STRIPE_WIDTH - 1 & -VC_IMAGE_YUV_UV_STRIPE_WIDTH) >= (src->height >> 1));
   assert(VC_IMAGE_YUV_UV_STRIPE_WIDTH == 128);
   assert(vclib_check_VRF());

   for (x = 0; x < dst->width; x += VC_IMAGE_YUV_UV_STRIPE_WIDTH)
   {
      int src_height = min(src->height - x * 2, VC_IMAGE_YUV_UV_STRIPE_WIDTH * 2);
      
      dptr = (unsigned char *)dst->image_data + (x >> VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2) * dst->pitch;
      sptr = (unsigned char const *)src->image_data + x * VC_IMAGE_YUV_UV_STRIPE_WIDTH * 2;

      for (y = dst->height; y > 32; y -= 64)
      {
         int dst_height = 32;

         yuvuv128_trans_sub_block_y(dptr, sptr, dst_height, src_height);
         dptr += VC_IMAGE_YUV_UV_STRIPE_WIDTH * dst_height;

         dst_height = min(y - 32, 32);
         yuvuv128_trans_sub_block_y(dptr, sptr + 64, dst_height, src_height);
         dptr += VC_IMAGE_YUV_UV_STRIPE_WIDTH * dst_height;
         sptr += src->pitch;
      }
      if (y > 0)
         yuvuv128_trans_sub_block_y(dptr, sptr, y, src_height);

      src_height >>= 1;
      dptr = (unsigned char *)dst->extra.uv.u + (x >> VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2) * dst->pitch;
      sptr = (unsigned char const *)src->extra.uv.u + x * VC_IMAGE_YUV_UV_STRIPE_WIDTH;

      for (y = dst->height + 1 >> 1; y > 16; y -= 32)
      {
         int dst_height = 16;

         yuvuv128_trans_sub_block_uv(dptr, sptr, dst_height, src_height);
         dptr += VC_IMAGE_YUV_UV_STRIPE_WIDTH * dst_height;

         dst_height = min(y - 16, 16);
         yuvuv128_trans_sub_block_uv(dptr, sptr + 64, dst_height, src_height);
         dptr += VC_IMAGE_YUV_UV_STRIPE_WIDTH * dst_height;
         sptr += src->pitch;
      }
      if (y > 0)
         yuvuv128_trans_sub_block_uv(dptr, sptr, y, src_height);
   }
}


/******************************************************************************
NAME
   vc_image_subsample_yuvuv128

SYNOPSIS
   void vc_image_subsample_yuvuv128(VC_IMAGE_BUF_T *dst, VC_IMAGE_BUF_T *src)

FUNCTION
   blah blah blah
******************************************************************************/

void vc_image_subsample_yuvuv128(VC_IMAGE_BUF_T *dst, VC_IMAGE_BUF_T *src)
{
   unsigned char *dptr;
   unsigned char const *sptr;
   int x, doff, soff;

   vcos_assert(is_valid_vc_image_buf(dst, VC_IMAGE_YUV_UV));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_YUV_UV));
   vcos_assert((dst->width + VC_IMAGE_YUV_UV_STRIPE_WIDTH - 1 & -VC_IMAGE_YUV_UV_STRIPE_WIDTH) >= (src->width >> 1));
   vcos_assert((dst->height + 15 & ~15) >= (src->height >> 1));
   vcos_assert(VC_IMAGE_YUV_UV_STRIPE_WIDTH == 128);
   vcos_assert(vclib_check_VRF());

   dptr = vc_image_get_y(dst);
   sptr = vc_image_get_y(src);
   doff = vc_image_get_u(dst) - dptr;
   soff = vc_image_get_u(src) - sptr;

   for (x = dst->width; x > 0; x -= 128)
   {
      yuvuv128_subsample_128xn_y(dptr, sptr, src->height);
      yuvuv128_subsample_64xn_uv(dptr + doff, sptr + soff, src->height >> 1);

      if (x > 64)
      {
         yuvuv128_subsample_128xn_y(dptr + 64, sptr + src->pitch, src->height);
         yuvuv128_subsample_64xn_uv(dptr + doff + 64, sptr + soff + src->pitch, src->height >> 1);
      }
      dptr += dst->pitch;
      sptr += src->pitch * 2;
   }
}


/******************************************************************************
NAME
   vc_image_resize_stripe_h_yuvuv128

SYNOPSIS
   void vc_image_resize_stripe_h_yuvuv128(VC_IMAGE_T *dest, int d_width, int s_width)

FUNCTION
   Resize a YUV_UV128 stripe horizontally.
******************************************************************************/

void vc_image_resize_stripe_h_yuvuv128(VC_IMAGE_T *dest, int d_width, int s_width)
{
   int step;

   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_YUV_UV));
   assert(d_width > 0 && s_width > 0);
   assert(dest->height == 16);
   assert(dest->width >= s_width);
   assert(dest->width >= d_width);

   if (d_width <= 0)
      return;

   /* When reducing the image an even destination size is necessary to stop the
    * subsampled UV from trying to read more data than exists.  The consequence
    * of this workaround is that some of the left/bottom edge of the image is
    * cut off.  Alternatives are to assert() that the sizes must be even, or to
    * spend the extra processing time duplicating the final column of U and V
    * data so that the source image appears complete when rounded up.
    */
   if (d_width < s_width && (d_width & 1) != 0)
      d_width++;

   while (d_width << 2 < s_width)
   {
      s_width >>= 2;
      yuvuv128_h_reduce2_y_stripe(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, s_width, 0x40000);
      yuvuv128_h_reduce2_uv_stripe(vc_image_get_u(dest), dest->pitch, vc_image_get_u(dest), dest->pitch, (s_width + 1) >> 1, 0x40000);
   }

   step = ((s_width << 16) + (d_width >> 1)) / d_width;

   if (step != 0x10000)
   {
      int nstrips, off;

      switch (_msb((step - 1) | 0x1000))
      {
      case 17: /* 2.0 < step <= 4.0 */
         yuvuv128_h_reduce2_y_stripe(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, d_width, step);
         yuvuv128_h_reduce2_uv_stripe(vc_image_get_u(dest), dest->pitch, vc_image_get_u(dest), dest->pitch, (d_width + 1) >> 1, step);
         break;
      case 16: /* 1.0 < step <= 2.0 */
         yuvuv128_h_reduce_y_stripe(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch, d_width, step);
         yuvuv128_h_reduce_uv_stripe(vc_image_get_u(dest), dest->pitch, vc_image_get_u(dest), dest->pitch, (d_width+1)>>1, step);
         break;
      case 12: /* 0.0625 < step <= 0.125 */
      case 13: /* 0.125 < step <= 0.25 */
      case 14: /* 0.25 < step <= 0.5 */
      case 15: /* 0.5 < step <= 1.0 */
         nstrips = ((dest->width)+VC_IMAGE_YUV_UV_STRIPE_WIDTH-1)>>VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2;
         off = ((nstrips << VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2) - (s_width+1&~1)) & ~31;

         yuvuv128_move_stripe(vc_image_get_y(dest), vc_image_get_u(dest), nstrips, dest->pitch, off);

         yuvuv128_h_expand_y_stripe(vc_image_get_y(dest), dest->pitch, vc_image_get_y(dest), dest->pitch,
                                    d_width, step, off, s_width);

         yuvuv128_h_expand_uv_stripe(vc_image_get_u(dest), dest->pitch, vc_image_get_u(dest), dest->pitch,
                                     (d_width+1)>>1, step, off>>1, (s_width+1)>>1);
         break;

      default:
         assert(0); /* can't cope with scale factor */
      }
   }
}

