#if 1
#include <vc/intrinsics.h>
#include "helpers/vc_image/vc_image.h"
#include "interface/vcos/vcos_assert.h"
#else
typedef struct
{
   int width, height, pitch, size;
   void *image_data;
} VC_IMAGE_T;
#define __BCM2707A0__
#define vcos_assert assert
#endif
#include <stdio.h>
#include <stdlib.h>

#if !defined(max)
#define max(a, b) ((a) < (b) ? (b) : (a))
#define min(a, b) ((a) > (b) ? (b) : (a))
#endif

#define R_WEIGHT 19
#define G_WEIGHT 37
#define B_WEIGHT 7
#define TOTAL_WEIGHT (R_WEIGHT + G_WEIGHT + B_WEIGHT)
#define MAX_LUMA (TOTAL_WEIGHT * 255)

#if defined(__BCM2707A0__)
#define SHIFT_SWIZZLE 24
#else
#define SHIFT_SWIZZLE 0
#endif

typedef struct
{
   unsigned char r5, r4, g5, g4, b5, b4, pad0, pad1;
} qrgb_t;

typedef struct
{
   qrgb_t quads[64];
} encode_workunit_t;

typedef struct
{
   int r, g, b, y;
} rgby_t;

typedef struct
{
   rgby_t base[2];
   int table[2];
   int flip, diff;
   unsigned long score;
} candidate_t;

extern unsigned long long etc1_block_score(
      int                  block_index,   /* r0 */
      int                  luma1,         /* r1 */
      int                  luma2,         /* r2 */
      int                  planes);       /* r3 */

extern unsigned long etc1_block_pack(
      int                  block_index,   /* r0 */
      int                  luma1,         /* r1 */
      int                  luma2,         /* r2 */
      int                  thresh1,       /* r3 */
      int                  thresh2,       /* r4 */
      int                  planes);       /* r5 */

extern unsigned long etc1_load_rgbx5551(
      void                *output,        /* r0 */
      unsigned short const*pixels,        /* r1 */
      int                  pitch,         /* r2 */
      int                  rows,          /* r3 */
      int                  columns);      /* r4 */

extern unsigned long etc1_load_xrgb16(
      void                *output,        /* r0 */
      unsigned short const*pixels,        /* r1 */
      int                  pitch,         /* r2 */
      int                  rows,          /* r3 */
      int                  columns);      /* r4 */

extern unsigned long etc1_load_yuv420(
      void                *output,        /* r0 */
      unsigned char const *y_pixels,      /* r1 */
      int                  pitch,         /* r2 */
      int                  rows,          /* r3 */
      int                  columns,       /* r4 */
      unsigned char const *u_pixels,      /* r5 */
      unsigned char const *v_pixels,      /* (sp) */
      int                  uv_pitch);     /* (sp+4) */

extern unsigned long etc1_load_yuv422(
      void                *output,        /* r0 */
      unsigned char const *y_pixels,      /* r1 */
      int                  pitch,         /* r2 */
      int                  rows,          /* r3 */
      int                  columns,       /* r4 */
      unsigned char const *u_pixels,      /* r5 */
      unsigned char const *v_pixels,      /* (sp) */
      int                  uv_pitch);     /* (sp+4) */

extern unsigned long etc1_load_yuv_uv(
      void                *output,        /* r0 */
      unsigned char const *y_pixels,      /* r1 */
      int                  pitch,         /* r2 */
      int                  rows,          /* r3 */
      int                  columns,       /* r4 */
      unsigned char const *uv_pixels,     /* r5 */
      int                  uv_pitch);     /* (sp) */

extern unsigned long etc1_load_rgbx16(
      void                *output,        /* r0 */
      unsigned short const*pixels,        /* r1 */
      int                  pitch,         /* r2 */
      int                  rows,          /* r3 */
      int                  columns);      /* r4 */

extern unsigned long etc1_load_rgb565(
      void                *output,        /* r0 */
      unsigned short const*pixels,        /* r1 */
      int                  pitch,         /* r2 */
      int                  rows,          /* r3 */
      int                  columns);      /* r4 */

extern unsigned long etc1_load_rgb888(
      void                *output,        /* r0 */
      unsigned char const *pixels,        /* r1 */
      int                  pitch,         /* r2 */
      int                  rows,          /* r3 */
      int                  columns);      /* r4 */

extern unsigned long etc1_load_rgbx32(
      void                *output,        /* r0 */
      unsigned long const *pixels,        /* r1 */
      int                  pitch,         /* r2 */
      int                  rows,          /* r3 */
      int                  columns);      /* r4 */

static void block_quantise(rgby_t *dest, qrgb_t const *src, int diff)
{
   int i;
   unsigned r, g, b, y;

   if (diff == 0)
   {
      for (i = 0; i < 2; i++)
      {
         r = src[i].r4;
         g = src[i].g4;
         b = src[i].b4;
         y = R_WEIGHT * r + G_WEIGHT * g + B_WEIGHT * b;
         vcos_assert(y <= MAX_LUMA);
         dest[i].r = r;
         dest[i].g = g;
         dest[i].b = b;
         dest[i].y = y;
      }
   }
   else
   {
      for (i = 0; i < 2; i++)
      {
         r = src[i].r5;
         g = src[i].g5;
         b = src[i].b5;
         y = R_WEIGHT * r + G_WEIGHT * g + B_WEIGHT * b;
         vcos_assert(y <= MAX_LUMA);
         dest[i].r = r;
         dest[i].g = g;
         dest[i].b = b;
         dest[i].y = y;
      }
   }
}

static unsigned long block_vertical_score(int block_index, int tabs[2], int left_luma, int right_luma)
{
   unsigned long long result = etc1_block_score(block_index, left_luma, right_luma, 0x3333);
   int t0 = ((result >> 32) & 7), t1 = (result >> 36);

   tabs[0] = t0;
   tabs[1] = t1;
   return (unsigned long)result;
}


static unsigned long block_horizontal_score(int block_index, int tabs[2], int top_luma, int bottom_luma)
{
   unsigned long long result = etc1_block_score(block_index, top_luma, bottom_luma, 0x00ff);
   int t0 = ((result >> 32) & 7), t1 = (result >> 36);

   tabs[0] = t0;
   tabs[1] = t1;
   return (unsigned long)result;
}


static unsigned long long pack_block(candidate_t *winner, int block_index)
{
   static const int threshold[8] = { 5, 11, 19, 22, 39, 52, 69, 115 };
   unsigned long low, high;

   high = etc1_block_pack(block_index,
                          winner->base[0].y, winner->base[1].y,
                          threshold[winner->table[0]] * TOTAL_WEIGHT, threshold[winner->table[1]] * TOTAL_WEIGHT,
                          winner->flip ? 0x00ff : 0x3333);

   low = 0;

   vcos_assert((unsigned)(winner->base[0].r | winner->base[0].g | winner->base[0].b) < 256);
   vcos_assert((unsigned)(winner->base[1].r | winner->base[1].g | winner->base[1].b) < 256);

   if (winner->diff)
   {
      int dr, dg, db;
      dr = (winner->base[1].r >> 3) - (winner->base[0].r >> 3);
      dg = (winner->base[1].g >> 3) - (winner->base[0].g >> 3);
      db = (winner->base[1].b >> 3) - (winner->base[0].b >> 3);

      vcos_assert((unsigned)(dr + 4) < 8);
      vcos_assert((unsigned)(dg + 4) < 8);
      vcos_assert((unsigned)(db + 4) < 8);

      low |= (winner->base[0].r >> 3) << ( 3 ^ SHIFT_SWIZZLE);
      low |= (dr & 7)                 << ( 0 ^ SHIFT_SWIZZLE);
      low |= (winner->base[0].g >> 3) << (11 ^ SHIFT_SWIZZLE);
      low |= (dg & 7)                 << ( 8 ^ SHIFT_SWIZZLE);
      low |= (winner->base[0].b >> 3) << (19 ^ SHIFT_SWIZZLE);
      low |= (db & 7)                 << (16 ^ SHIFT_SWIZZLE);
   }
   else
   {
      low |= (winner->base[0].r >> 4) << ( 4 ^ SHIFT_SWIZZLE);
      low |= (winner->base[1].r >> 4) << ( 0 ^ SHIFT_SWIZZLE);
      low |= (winner->base[0].g >> 4) << (12 ^ SHIFT_SWIZZLE);
      low |= (winner->base[1].g >> 4) << ( 8 ^ SHIFT_SWIZZLE);
      low |= (winner->base[0].b >> 4) << (20 ^ SHIFT_SWIZZLE);
      low |= (winner->base[1].b >> 4) << (16 ^ SHIFT_SWIZZLE);
   }

   vcos_assert((unsigned)winner->table[0] < 8);
   vcos_assert((unsigned)winner->table[1] < 8);

   low |= winner->table[0] << (29 ^ SHIFT_SWIZZLE);
   low |= winner->table[1] << (26 ^ SHIFT_SWIZZLE);

   low |= winner->diff     << (25 ^ SHIFT_SWIZZLE);
   low |= winner->flip     << (24 ^ SHIFT_SWIZZLE);

#if SHIFT_SWIZZLE
   return (unsigned long long)low << 32 | high;
#else
   return (unsigned long long)high << 32 | low;
#endif
}


static int block_diffable(qrgb_t const *ptr)
{
   int r0 = ptr[0].r5 >> 3;
   int g0 = ptr[0].g5 >> 3;
   int b0 = ptr[0].b5 >> 3;
   int r1 = ptr[1].r5 >> 3;
   int g1 = ptr[1].g5 >> 3;
   int b1 = ptr[1].b5 >> 3;

   return ((unsigned)((r1 - r0 + 4) | (g1 - g0 + 4) | (b1 - b0 + 4)) <= 7);
}


/* encode and store some blocks in microtile order */

static void encode_workunit(unsigned long long *dptr, encode_workunit_t *unit, int single_microtile)
{
   int block_index = 0;
   qrgb_t const *quad_map = unit->quads;

   int count = single_microtile ? 8 : 16;
   int i;

   for (i = 0; i < count; i++)
   {
      candidate_t horiz, vert, *winner;

      vert.flip = 0;
      vert.diff = block_diffable(quad_map);
      block_quantise(vert.base, quad_map, vert.diff);

      horiz.flip = 1;
      horiz.diff = block_diffable(quad_map + 32);
      block_quantise(horiz.base, quad_map + 32, horiz.diff);
      quad_map += 2;

      vert.score = block_vertical_score(block_index, vert.table, vert.base[0].y, vert.base[1].y);
      horiz.score = block_horizontal_score(block_index, horiz.table, horiz.base[0].y, horiz.base[1].y);

      winner = horiz.score < vert.score ? &horiz : &vert;

      *dptr++ = pack_block(winner, block_index);

      block_index += 64;
   }
}

/* encode and store some blocks in linear block order */

static void encode_workunit_linear_block(unsigned long long *dptr, int pitch, encode_workunit_t *unit, int w, int h)
{
   int block_index;
   qrgb_t const *quad_map;
   int x, y;

   pitch -= sizeof(*dptr) * w;

   for (y = 0; y < h; y++)
   {
      for (x = 0; x < w; x++)
      {
         candidate_t horiz, vert, *winner;

         block_index = ((x & 1) + y * 2 + (x & 2) * 4);
         quad_map = unit->quads + block_index * 2;
         block_index *= 64;

         vert.flip = 0;
         vert.diff = block_diffable(quad_map);
         block_quantise(vert.base, quad_map, vert.diff);

         horiz.flip = 1;
         horiz.diff = block_diffable(quad_map + 32);
         block_quantise(horiz.base, quad_map + 32, horiz.diff);
         quad_map += 2;

         vert.score = block_vertical_score(block_index, vert.table, vert.base[0].y, vert.base[1].y);
         horiz.score = block_horizontal_score(block_index, horiz.table, horiz.base[0].y, horiz.base[1].y);

         winner = horiz.score < vert.score ? &horiz : &vert;

         *dptr++ = pack_block(winner, block_index);
      }
      dptr = (void *)((char *)dptr + pitch);
   }
}


static void build_workunit(encode_workunit_t *out, VC_IMAGE_T const *in, int x, int y)
{
   int rows, columns;
   int pitch = in->pitch, uvpitch = in->pitch;

   if (in->type == VC_IMAGE_OPENGL)
   {
      /* TODO: test this */
      pitch = -pitch;
      rows = min(16, in->height - y);
      y += 15;
      if (y >= in->height - 1)
         y = in->height - 1;
   }
   else
   {
      rows = 16;
      y = in->height - rows - y;
      if (y < 0)
      {
         rows += y;
         y = 0;
      }
   }
   columns = min(16, in->width - x);

   switch (in->type)
   {
   case VC_IMAGE_YUV420:
      uvpitch >>= 1;
      etc1_load_yuv420(out, (unsigned char *)vc_image_get_y(in) + pitch * y + x, pitch, rows, columns,
            (unsigned char *)vc_image_get_u(in) + uvpitch * (y >> 1) + (x >> 1), 
            (unsigned char *)vc_image_get_v(in) + uvpitch * (y >> 1) + (x >> 1), uvpitch);
      break;
   case VC_IMAGE_YUV422PLANAR:
      uvpitch >>= 1;
      /*@FALLTHROUGH*/
   case VC_IMAGE_YUV422:
      etc1_load_yuv422(out, (unsigned char *)vc_image_get_y(in) + pitch * y + x, pitch, rows, columns,
            (unsigned char *)vc_image_get_u(in) + uvpitch * y + (x >> 1), 
            (unsigned char *)vc_image_get_v(in) + uvpitch * y + (x >> 1), uvpitch);
      break;
   case VC_IMAGE_YUV_UV:
      etc1_load_yuv_uv(out,
            (unsigned char *)vc_image_get_y(in)
                  + (y << VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2)
                  + (x >> VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2) * pitch + (x & (VC_IMAGE_YUV_UV_STRIPE_WIDTH - 1)),
            VC_IMAGE_YUV_UV_STRIPE_WIDTH, rows, columns,
            (unsigned char *)vc_image_get_u(in)
                  + (y << (VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2 - 1))
                  + (x >> VC_IMAGE_YUV_UV_STRIPE_WIDTH_LOG2) * uvpitch + (x & (VC_IMAGE_YUV_UV_STRIPE_WIDTH - 1)),
            VC_IMAGE_YUV_UV_STRIPE_WIDTH); 
      break;
   case VC_IMAGE_YUV_UV32:
      etc1_load_yuv_uv(out,
            (unsigned char *)vc_image_get_y(in)
                  + (y << VC_IMAGE_YUV_UV32_STRIPE_WIDTH_LOG2)
                  + (x >> VC_IMAGE_YUV_UV32_STRIPE_WIDTH_LOG2) * pitch + (x & (VC_IMAGE_YUV_UV32_STRIPE_WIDTH - 1)),
            VC_IMAGE_YUV_UV32_STRIPE_WIDTH, rows, columns,
            (unsigned char *)vc_image_get_u(in)
                  + (y << (VC_IMAGE_YUV_UV32_STRIPE_WIDTH_LOG2 - 1))
                  + (x >> VC_IMAGE_YUV_UV32_STRIPE_WIDTH_LOG2) * uvpitch + (x & (VC_IMAGE_YUV_UV32_STRIPE_WIDTH - 1)),
            VC_IMAGE_YUV_UV32_STRIPE_WIDTH); 
      break;
   case VC_IMAGE_RGBA16:
      etc1_load_rgbx16(out, (unsigned short *)((char *)in->image_data + pitch * y) + x, pitch, rows, columns);
      break;
   case VC_IMAGE_RGB565:
      etc1_load_rgb565(out, (unsigned short *)((char *)in->image_data + pitch * y) + x, pitch, rows, columns);
      break;
   case VC_IMAGE_RGB888:
      etc1_load_rgb888(out, ((unsigned char *)in->image_data + pitch * y + x * 3), pitch, rows, columns);
      break;
   case VC_IMAGE_RGBA32:
      etc1_load_rgbx32(out, (unsigned long *)((char *)in->image_data + pitch * y) + x, pitch, rows, columns);
      break;
   case VC_IMAGE_OPENGL:
      switch (in->extra.opengl.format_and_type)
      {
      case VC_IMAGE_OPENGL_RGBA32:
         etc1_load_rgbx32(out, (unsigned long *)((char *)in->image_data + pitch * y) + x, pitch, rows, columns);
         break;
      case VC_IMAGE_OPENGL_RGB24:
         etc1_load_rgb888(out, ((unsigned char *)in->image_data + pitch * y + x * 3), pitch, rows, columns);
         break;
      case VC_IMAGE_OPENGL_RGBA16:
         etc1_load_rgbx16(out, (unsigned short *)((char *)in->image_data + pitch * y) + x, pitch, rows, columns);
         break;
      case VC_IMAGE_OPENGL_RGBA5551:
         etc1_load_rgbx5551(out, (unsigned short *)((char *)in->image_data + pitch * y) + x, pitch, rows, columns);
         break;
      case VC_IMAGE_OPENGL_RGB565:
         etc1_load_rgb565(out, (unsigned short *)((char *)in->image_data + pitch * y) + x, pitch, rows, columns);
         break;
      default:
         vcos_assert(!"Unsupported ETC1 conversion source type.");
      }
      break;
   default:
      /* Oops.  A type-checking assert in an internal loop.  That's bound to suck. */
      vcos_assert(!"Unsupported ETC1 conversion source type.");
   }
}


static int get_block_offset(int x, int y, int pitch)
{
   int offset_row, offset_col, offset_min, offset;


   /* re-hash things to match 32-bit t-format coordinates */
   x >>= 2;
   y >>= 2;
   x <<= 1;

   offset_row = ((y + 32) & ~63) * pitch;

   offset_col = (x >> 4) * 512
                + ((y >> 4) & 1) * 256;
   offset_col ^= (y & 32) ? -512 : 0;
   offset_col ^= (x << 4) & 256;

   offset_min = ((y >> 2) & 3) * 64
                + ((x >> 2) & 3) * 16
                + (y & 3) * 4
                + (x & 3) * 1;

   offset = offset_row + offset_col + offset_min;

   /* de-hash things back to 64-bit */
   offset >>= 1;

   return offset;
}


void etc1_encode(VC_IMAGE_T *out, VC_IMAGE_T const *in)
{
   int x, y;

   if (out->type == VC_IMAGE_TF_ETC1)
   {
      int linear_dest = vc_image_get_mipmap_type(out, 0) == VC_IMAGE_MIPMAP_LINEAR_TILE;

      for (y = 0; y < out->height; y += 16)
      {
         for (x = 0; x < out->width; x += 16)
         {
            unsigned long long *dptr;
            encode_workunit_t unit;

            if (linear_dest)
               dptr = (unsigned long long *)((char *)out->image_data + y * out->pitch) + x;
            else
               dptr = (unsigned long long *)out->image_data + get_block_offset(x, y, out->pitch);

            vcos_assert((unsigned)(dptr - (unsigned long long *)out->image_data) * 8 < out->size);

            build_workunit(&unit, in, x, y);
            encode_workunit(dptr, &unit, (x + 8 >= in->width));
         }
      }
   }
   else
      for (y = 0; y < out->height; y += 16)
      {
         unsigned long long *dptr = (unsigned long long *)((char *)out->image_data + (y >> 2) * out->pitch);

         for (x = 0; x < out->width; x += 16)
         {
            encode_workunit_t unit;

            build_workunit(&unit, in, x, y);
            encode_workunit_linear_block(dptr, out->pitch, &unit, _min(16, out->width - x + 3) >> 2, _min(16, out->height - y + 3) >> 2);
            dptr += 4;
         }
      }
}

