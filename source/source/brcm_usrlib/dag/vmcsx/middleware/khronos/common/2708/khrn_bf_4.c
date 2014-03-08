//#define BF_USE_VC_IMAGE_RASTER_TO_TFORMAT

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
#include "middleware/khronos/common/2708/khrn_bf_4.h"
#include "interface/khronos/common/khrn_int_color.h"
#include "middleware/khronos/common/khrn_image.h"
#include "interface/khronos/common/khrn_int_math.h"
#include "middleware/khronos/common/khrn_mem.h"
#include "interface/khronos/common/khrn_int_util.h"

#ifdef BF_USE_VC_IMAGE_RASTER_TO_TFORMAT
   #include "middleware/khronos/dispatch/khrn_dispatch.h"
   #include "helpers/vc_image/vc_image.h"
   #include "helpers/vc_image/vc_image_tformat.h"
#endif
#include "vcfw/vclib/vclib.h"
#include "vcinclude/hardware.h"
#include <math.h>

/******************************************************************************
block ld/st
******************************************************************************/

extern void khrn_bf_blk_lda(uint32_t vr_data, const void *data, int32_t stride, KHRN_IMAGE_FORMAT_T format);
extern void khrn_bf_blk_ldam(uint32_t vr_data, const void *data, int32_t stride, uint32_t mask_x, uint32_t mask_y, KHRN_IMAGE_FORMAT_T format);
extern void khrn_bf_blk_lda_unpack(uint32_t vr_data, bool tf, KHRN_IMAGE_FORMAT_T format);
extern void khrn_bf_blk_ldam_unpack(uint32_t vr_data, bool tf, KHRN_IMAGE_FORMAT_T format);
extern void khrn_bf_blk_sta_pack(uint32_t vr_data, bool tf, KHRN_IMAGE_FORMAT_T format);
extern void khrn_bf_blk_stam_pack(uint32_t vr_data, bool tf, KHRN_IMAGE_FORMAT_T format);
extern void khrn_bf_blk_sta(uint32_t vr_data, void *data, int32_t stride, KHRN_IMAGE_FORMAT_T format);
extern void khrn_bf_blk_stam(uint32_t vr_data, void *data, int32_t stride, uint32_t mask_x, uint32_t mask_y, KHRN_IMAGE_FORMAT_T format);
extern void khrn_bf_blk_rotate_x(uint32_t vr_data, uint32_t x);
extern void khrn_bf_blk_rotate_y(uint32_t vr_data, uint32_t y);

static INLINE bool blk_prefer_tf(KHRN_IMAGE_FORMAT_T format)
{
   return (0x600000 >> (format & (IMAGE_FORMAT_MEM_LAYOUT_MASK | IMAGE_FORMAT_PIXEL_SIZE_MASK))) & 0x1;
}

/* BLK_ALIGN/blk_get_align assume this */
vcos_static_assert(IMAGE_FORMAT_MEM_LAYOUT_SHIFT == 0);
vcos_static_assert(IMAGE_FORMAT_PIXEL_SIZE_SHIFT == IMAGE_FORMAT_MEM_LAYOUT_BC);
vcos_static_assert(IMAGE_FORMAT_MEM_LAYOUT_BC == 3);

#define ALIGN_PACK(X, Y) ((((X) - 1) << 0) | (((Y) - 1) << 4))

static const uint8_t BLK_ALIGN[64] = {
   /* IMAGE_FORMAT_1 */
   ALIGN_PACK(8, 1),
   ALIGN_PACK(16, 16),
   ALIGN_PACK(16, 8),
   0,
   0,
   0,
   0,
   0,
   /* IMAGE_FORMAT_4 */
   ALIGN_PACK(2, 1),
   ALIGN_PACK(16, 16),
   ALIGN_PACK(16, 8),
   0,
   0,
   0,
   0,
   0,
   /* IMAGE_FORMAT_8 */
   ALIGN_PACK(1, 1),
   ALIGN_PACK(16, 16),
   ALIGN_PACK(8, 8),
   ALIGN_PACK(16, 16),
   0,
   0,
   0,
   0,
   /* IMAGE_FORMAT_16 */
   ALIGN_PACK(1, 1),
   ALIGN_PACK(16, 16),
   ALIGN_PACK(8, 4),
   0,
   0,
   0,
   0,
   0,
   /* IMAGE_FORMAT_24 */
   ALIGN_PACK(1, 1),
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   /* IMAGE_FORMAT_32 */
   ALIGN_PACK(1, 1),
   ALIGN_PACK(16, 16),
   ALIGN_PACK(4, 4),
   ALIGN_PACK(16, 16),
   0,
   0,
   0,
   0 };

#undef ALIGN_PACK

static INLINE uint32_t blk_get_align(KHRN_IMAGE_FORMAT_T format)
{
   return BLK_ALIGN[format & (IMAGE_FORMAT_MEM_LAYOUT_MASK | IMAGE_FORMAT_PIXEL_SIZE_MASK)];
}

static INLINE int32_t get_tf_subtile_offset(int32_t width_subtiles, int32_t x, int32_t y)
{
   x = ((x << 1) + ((x ^ y) & 0x1)) ^ -(y & 0x2);
   y = (y + 2) & ~0x3;
   return ((y * width_subtiles) + x) * 1024;
}

static int32_t blk_get_offset(int32_t stride, int32_t x, int32_t y, KHRN_IMAGE_FORMAT_T format)
{
   vcos_assert(!(x & (blk_get_align(format) & 0xf)) && !(y & (blk_get_align(format) >> 4)));
   switch (format & (IMAGE_FORMAT_MEM_LAYOUT_MASK | IMAGE_FORMAT_PIXEL_SIZE_MASK)) {
   case (IMAGE_FORMAT_RSO | IMAGE_FORMAT_1):    return (y * stride) + (x >> 3);
   case (IMAGE_FORMAT_RSO | IMAGE_FORMAT_4):    return (y * stride) + (x >> 1);
   case (IMAGE_FORMAT_RSO | IMAGE_FORMAT_8):    return (y * stride) + x;
   case (IMAGE_FORMAT_RSO | IMAGE_FORMAT_16):   return (y * stride) + (x << 1);
   case (IMAGE_FORMAT_RSO | IMAGE_FORMAT_24):   return (y * stride) + (x * 3);
   case (IMAGE_FORMAT_RSO | IMAGE_FORMAT_32):   return (y * stride) + (x << 2);
   case (IMAGE_FORMAT_TF | IMAGE_FORMAT_1):     x >>= 2;
   case (IMAGE_FORMAT_TF | IMAGE_FORMAT_4):     x >>= 1;
   case (IMAGE_FORMAT_TF | IMAGE_FORMAT_8):
   case (IMAGE_FORMAT_RSOTF | IMAGE_FORMAT_8):  return get_tf_subtile_offset(stride >> 5, x >> 5, y >> 5) + ((y & 0x10) * 32) + ((x & 0x18) * 8) + (x & 0x7);
   case (IMAGE_FORMAT_TF | IMAGE_FORMAT_16):    x >>= 1;
   case (IMAGE_FORMAT_TF | IMAGE_FORMAT_32):
   case (IMAGE_FORMAT_RSOTF | IMAGE_FORMAT_32): return get_tf_subtile_offset(stride >> 6, x >> 4, y >> 4) + ((x & 0x8) * 16);
   case (IMAGE_FORMAT_LT | IMAGE_FORMAT_1):     x >>= 2;
   case (IMAGE_FORMAT_LT | IMAGE_FORMAT_4):     x >>= 1;
   case (IMAGE_FORMAT_LT | IMAGE_FORMAT_8):     return (y * stride) + ((x >> 3) * 64) + (x & 0x7);
   case (IMAGE_FORMAT_LT | IMAGE_FORMAT_16):    x >>= 1;
   case (IMAGE_FORMAT_LT | IMAGE_FORMAT_32):    return (y * stride) + (x * 16);
   default:                                     UNREACHABLE(); return 0;
   }
}

typedef struct {
   void *data;
   int32_t stride;
   KHRN_IMAGE_FORMAT_T format;
   uint16_t width, height, sub_x, sub_y, sub_width, sub_height;
} BLK_LD_ST_T;

static void blk_ld_st_init(BLK_LD_ST_T *blk_ld_st,
   const KHRN_IMAGE_WRAP_T *wrap, uint32_t sub_x, uint32_t sub_y, uint32_t sub_width, uint32_t sub_height)
{
   KHRN_IMAGE_FORMAT_T format = wrap->format;
   uint32_t width = wrap->width;
   uint32_t height = wrap->height;
   int32_t stride = wrap->stride;
   void *data = wrap->storage;

   if (khrn_image_is_tformat(format)) {
      width = round_up(width, 1 << khrn_image_get_log2_tile_width(format));
      height = round_up(height, 1 << khrn_image_get_log2_tile_height(format));
   } else if (khrn_image_is_lineartile(format)) {
      width = round_up(width, 1 << khrn_image_get_log2_utile_width(format));
      height = round_up(height, 1 << khrn_image_get_log2_utile_height(format));
   }

   blk_ld_st->data = data;
   blk_ld_st->stride = stride;
   blk_ld_st->format = format;
   blk_ld_st->width = (uint16_t)width;
   blk_ld_st->height = (uint16_t)height;
   blk_ld_st->sub_x = (uint16_t)sub_x;
   blk_ld_st->sub_y = (uint16_t)sub_y;
   blk_ld_st->sub_width = (uint16_t)sub_width;
   blk_ld_st->sub_height = (uint16_t)sub_height;
}

/* used by blk_ldap */
#define BLK_LDN_MASKED              (1 << 0)
/* used by blk_ldp */
#define BLK_LDN_ROT_X_MASK          (0xf << 1)
#define BLK_LDN_ROT_X_SHIFT         1
#define BLK_LDN_ROT_Y_MASK          (0xf << 5)
#define BLK_LDN_ROT_Y_SHIFT         5
/* used by blk_ldpt */
#define BLK_LDN_NOP                 (1 << 9)
#define BLK_LDN_MIN_X_MASK          (0x1f << 12)
#define BLK_LDN_MIN_X_SHIFT         12
#define BLK_LDN_MAX_X_MASK          (0x1f << 17)
#define BLK_LDN_MAX_X_SHIFT         17
#define BLK_LDN_MIN_Y_MASK          (0x1f << 22)
#define BLK_LDN_MIN_Y_SHIFT         22
#define BLK_LDN_MAX_Y_MASK          (0x1f << 27)
#define BLK_LDN_MAX_Y_SHIFT         27
/* used by blk_ldpt repeat tile mode */
#define BLK_LDN_REPEAT_X            (1 << 10)
#define BLK_LDN_REPEAT_Y            (1 << 11)
#define BLK_LDN_REPEAT_MAX_XY_MASK  (0x1f << 12)
#define BLK_LDN_REPEAT_MAX_XY_SHIFT 12
#define BLK_LDN_REPEAT_XY_MASK      (0x7fff << 17) /* right at end so shifting down will sign extend */
#define BLK_LDN_REPEAT_XY_SHIFT     17
/* used by blk_ldpt reflect tile mode */
#define BLK_LDN_REFLECT_X           (1 << 10)
#define BLK_LDN_REFLECT_Y           (1 << 11)

static uint32_t blk_ldap(uint32_t vr_data, const BLK_LD_ST_T *src, int32_t x, int32_t y)
{
   vcos_assert((x >= -15) && (y >= -15));
   vcos_assert((x < (int32_t)src->sub_width) && (y < (int32_t)src->sub_height));

   const void *data = src->data;
   int32_t stride = src->stride;
   KHRN_IMAGE_FORMAT_T format = src->format;
   uint32_t width = src->width;
   uint32_t height = src->height;
   uint32_t sub_x = src->sub_x;
   uint32_t sub_y = src->sub_y;

   x += sub_x;
   y += sub_y;

   uint32_t mask_x = _bmask(-1, _min(width - x, 16)) ^ _bmask(-1, _max(-x, 0));
   uint32_t mask_y = _bmask(-1, _min(height - y, 16)) ^ _bmask(-1, _max(-y, 0));

   data = (const uint8_t *)data + blk_get_offset(stride, x, y, format);

   if ((mask_x & mask_y) == 0xffff) {
      khrn_bf_blk_lda(vr_data, data, stride, format);
      return 0;
   }

   khrn_bf_blk_ldam(vr_data, data, stride, mask_x, mask_y, format);
   return BLK_LDN_MASKED;
}

static uint32_t blk_ldp(uint32_t vr_data, const BLK_LD_ST_T *src, int32_t x, int32_t y)
{
   vcos_assert((x >= -15) && (y >= -15));
   vcos_assert((x < (int32_t)src->sub_width) && (y < (int32_t)src->sub_height));

   const void *data = src->data;
   int32_t stride = src->stride;
   KHRN_IMAGE_FORMAT_T format = src->format;
   uint32_t width = src->width;
   uint32_t height = src->height;
   uint32_t sub_x = src->sub_x;
   uint32_t sub_y = src->sub_y;

   x += sub_x;
   y += sub_y;

   uint32_t align = blk_get_align(format);
   uint32_t align_x = align & 0xf;
   uint32_t align_y = align >> 4;

   int32_t xa = x & ~align_x;
   int32_t ya = y & ~align_y;
   uint32_t mask_x = _bmask(-1, _min(x + 16, (int32_t)width) - _max(x, 0)) << (_max(x, 0) - xa);
   uint32_t mask_y = _bmask(-1, _min(y + 16, (int32_t)height) - _max(y, 0)) << (_max(y, 0) - ya);

   if ((mask_x & mask_y) == 0xffff) {
      khrn_bf_blk_lda(vr_data, (const uint8_t *)data + blk_get_offset(stride, xa, ya, format), stride, format);
      return 0;
   }

   if ((mask_x & 0xffff) && (mask_y & 0xffff)) { khrn_bf_blk_ldam(vr_data, (const uint8_t *)data + blk_get_offset(stride, xa,      ya,      format), stride, mask_x & 0xffff, mask_y & 0xffff, format); }
   if ((mask_x >> 16)    && (mask_y & 0xffff)) { khrn_bf_blk_ldam(vr_data, (const uint8_t *)data + blk_get_offset(stride, xa + 16, ya,      format), stride, mask_x >> 16,    mask_y & 0xffff, format); }
   if ((mask_x & 0xffff) && (mask_y >> 16))    { khrn_bf_blk_ldam(vr_data, (const uint8_t *)data + blk_get_offset(stride, xa,      ya + 16, format), stride, mask_x & 0xffff, mask_y >> 16,    format); }
   if ((mask_x >> 16)    && (mask_y >> 16))    { khrn_bf_blk_ldam(vr_data, (const uint8_t *)data + blk_get_offset(stride, xa + 16, ya + 16, format), stride, mask_x >> 16,    mask_y >> 16,    format); }
   return BLK_LDN_MASKED | ((x & align_x) << BLK_LDN_ROT_X_SHIFT) | ((y & align_y) << BLK_LDN_ROT_Y_SHIFT);
}

static INLINE void blk_ldap_unpack(uint32_t vr_data, bool tf, uint32_t ldn, KHRN_IMAGE_FORMAT_T format)
{
   if (!(ldn & BLK_LDN_MASKED)) {
      khrn_bf_blk_lda_unpack(vr_data, tf, format);
      return;
   }

   khrn_bf_blk_ldam_unpack(vr_data, tf, format);
}

static bool blk_ldp_unpack(uint32_t vr_data, bool tf, uint32_t ldn, KHRN_IMAGE_FORMAT_T format)
{
   if (!(ldn & BLK_LDN_MASKED)) {
      khrn_bf_blk_lda_unpack(vr_data, tf, format);
      return tf;
   }

   if (ldn & (BLK_LDN_ROT_X_MASK | BLK_LDN_ROT_Y_MASK)) {
      tf = false; /* need in rso to do rotation */
   }
   khrn_bf_blk_ldam_unpack(vr_data, tf, format);
   if (ldn & BLK_LDN_ROT_X_MASK) {
      khrn_bf_blk_rotate_x(vr_data, (ldn & BLK_LDN_ROT_X_MASK) >> BLK_LDN_ROT_X_SHIFT);
   }
   if (ldn & BLK_LDN_ROT_Y_MASK) {
      khrn_bf_blk_rotate_y(vr_data, (ldn & BLK_LDN_ROT_Y_MASK) >> BLK_LDN_ROT_Y_SHIFT);
   }
   return tf;
}

static void blk_stp(uint32_t vr_data, const BLK_LD_ST_T *dst, int32_t x, int32_t y)
{
   vcos_assert((x >= -15) && (y >= -15));
   vcos_assert((x < (int32_t)dst->sub_width) && (y < (int32_t)dst->sub_height));

   void *data = dst->data;
   int32_t stride = dst->stride;
   KHRN_IMAGE_FORMAT_T format = dst->format;
   uint32_t sub_x = dst->sub_x;
   uint32_t sub_y = dst->sub_y;
   uint32_t sub_width = dst->sub_width;
   uint32_t sub_height = dst->sub_height;

   x += sub_x;
   y += sub_y;

   uint32_t align = blk_get_align(format);
   uint32_t align_x = align & 0xf;
   uint32_t align_y = align >> 4;

   int32_t xa = x & ~align_x;
   int32_t ya = y & ~align_y;
   uint32_t mask_x = _bmask(-1, _min(x + 16, (int32_t)(sub_x + sub_width)) - _max(x, (int32_t)sub_x)) << (_max(x, (int32_t)sub_x) - xa);
   uint32_t mask_y = _bmask(-1, _min(y + 16, (int32_t)(sub_y + sub_height)) - _max(y, (int32_t)sub_y)) << (_max(y, (int32_t)sub_y) - ya);

   if ((mask_x & mask_y) == 0xffff) {
      khrn_bf_blk_sta_pack(vr_data, false, format);
      khrn_bf_blk_sta(vr_data, (uint8_t *)data + blk_get_offset(stride, xa, ya, format), stride, format);
      return;
   }

   if (x & align_x) {
      khrn_bf_blk_rotate_x(vr_data, 16 - (x & align_x));
   }
   if (y & align_y) {
      khrn_bf_blk_rotate_y(vr_data, 16 - (y & align_y));
   }
   khrn_bf_blk_stam_pack(vr_data, false, format);
   if ((mask_x & 0xffff) && (mask_y & 0xffff)) { khrn_bf_blk_stam(vr_data, (uint8_t *)data + blk_get_offset(stride, xa,      ya,      format), stride, mask_x & 0xffff, mask_y & 0xffff, format); }
   if ((mask_x >> 16)    && (mask_y & 0xffff)) { khrn_bf_blk_stam(vr_data, (uint8_t *)data + blk_get_offset(stride, xa + 16, ya,      format), stride, mask_x >> 16,    mask_y & 0xffff, format); }
   if ((mask_x & 0xffff) && (mask_y >> 16))    { khrn_bf_blk_stam(vr_data, (uint8_t *)data + blk_get_offset(stride, xa,      ya + 16, format), stride, mask_x & 0xffff, mask_y >> 16,    format); }
   if ((mask_x >> 16)    && (mask_y >> 16))    { khrn_bf_blk_stam(vr_data, (uint8_t *)data + blk_get_offset(stride, xa + 16, ya + 16, format), stride, mask_x >> 16,    mask_y >> 16,    format); }
}

static void blk_stap(uint32_t vr_data, bool tf, const BLK_LD_ST_T *dst, int32_t x, int32_t y)
{
   vcos_assert((x >= -15) && (y >= -15));
   vcos_assert((x < (int32_t)dst->sub_width) && (y < (int32_t)dst->sub_height));

   void *data = dst->data;
   int32_t stride = dst->stride;
   KHRN_IMAGE_FORMAT_T format = dst->format;
   uint32_t sub_x = dst->sub_x;
   uint32_t sub_y = dst->sub_y;
   uint32_t sub_width = dst->sub_width;
   uint32_t sub_height = dst->sub_height;

   uint32_t mask_x = _bmask(-1, _min(sub_width - x, 16)) ^ _bmask(-1, _max(-x, 0));
   uint32_t mask_y = _bmask(-1, _min(sub_height - y, 16)) ^ _bmask(-1, _max(-y, 0));

   x += sub_x;
   y += sub_y;

   data = (uint8_t *)data + blk_get_offset(stride, x, y, format);

   if ((mask_x & mask_y) == 0xffff) {
      khrn_bf_blk_sta_pack(vr_data, tf, format);
      khrn_bf_blk_sta(vr_data, data, stride, format);
      return;
   }

   khrn_bf_blk_stam_pack(vr_data, tf, format);
   khrn_bf_blk_stam(vr_data, data, stride, mask_x, mask_y, format);
}

/******************************************************************************
block unpack/pack
******************************************************************************/

extern void khrn_bf_blk_unpack(uint32_t vr_data, KHRN_IMAGE_FORMAT_T format, KHRN_IMAGE_CONV_T conv);
extern void khrn_bf_blk_pack(uint32_t vr_data, KHRN_IMAGE_FORMAT_T format, KHRN_IMAGE_CONV_T conv);

/******************************************************************************
block color stuff
******************************************************************************/

extern void khrn_bf_blk_pre(uint32_t vr_data);
extern void khrn_bf_blk_unpre(uint32_t vr_data, uint32_t vr_temp);
extern void khrn_bf_blk_clamp_to_a(uint32_t vr_data);
extern void khrn_bf_blk_s_to_lin(uint32_t vr_data);
extern void khrn_bf_blk_lin_to_s(uint32_t vr_data);
extern void khrn_bf_blk_to_la_lin(uint32_t vr_data);
extern void khrn_bf_blk_to_la_s(uint32_t vr_data, uint32_t vr_temp);

static void blk_convert_pre_lin(uint32_t vr_data, uint32_t vr_temp, KHRN_IMAGE_FORMAT_T dst_format, KHRN_IMAGE_FORMAT_T src_format)
{
   if ((dst_format ^ src_format) & (IMAGE_FORMAT_PRE | IMAGE_FORMAT_LIN)) {
      if (src_format & IMAGE_FORMAT_PRE) {
         khrn_bf_blk_unpre(vr_data, vr_temp);
      }
      if ((dst_format ^ src_format) & IMAGE_FORMAT_LIN) {
         if (dst_format & IMAGE_FORMAT_LIN) {
            khrn_bf_blk_s_to_lin(vr_data);
         } else {
            khrn_bf_blk_lin_to_s(vr_data);
         }
      }
      if (dst_format & IMAGE_FORMAT_PRE) {
         khrn_bf_blk_pre(vr_data);
      }
   }
}

static void blk_convert_l_pre_lin(uint32_t vr_data, uint32_t vr_temp, KHRN_IMAGE_FORMAT_T dst_format, KHRN_IMAGE_FORMAT_T src_format)
{
   if ((dst_format & IMAGE_FORMAT_L) && !(src_format & IMAGE_FORMAT_L)) {
      if (!(src_format & IMAGE_FORMAT_LIN) && (src_format & IMAGE_FORMAT_PRE)) {
         khrn_bf_blk_unpre(vr_data, vr_temp);
         src_format = (KHRN_IMAGE_FORMAT_T)(src_format & ~IMAGE_FORMAT_PRE);
      }
      if ((dst_format | src_format) & IMAGE_FORMAT_LIN) {
         if (!(src_format & IMAGE_FORMAT_LIN)) {
            khrn_bf_blk_s_to_lin(vr_data);
            src_format = (KHRN_IMAGE_FORMAT_T)(src_format | IMAGE_FORMAT_LIN);
         }
         khrn_bf_blk_to_la_lin(vr_data);
      } else {
         khrn_bf_blk_to_la_s(vr_data, vr_temp);
      }
   }
   blk_convert_pre_lin(vr_data, vr_temp, dst_format, src_format);
}

/******************************************************************************
block conversion
******************************************************************************/

static void blk_convert(uint32_t vr_data, uint32_t vr_temp, const void *src_aux, KHRN_IMAGE_FORMAT_T dst_format, KHRN_IMAGE_FORMAT_T src_format, KHRN_IMAGE_CONV_T conv)
{
   if (((dst_format ^ src_format) & ~IMAGE_FORMAT_MEM_LAYOUT_MASK) ||
      /* need khrn_bf_blk_unpack to clamp rgb to a */
      ((conv == IMAGE_CONV_VG) && khrn_image_is_color(src_format) && (src_format & IMAGE_FORMAT_PRE))) {
      if (khrn_image_is_paletted(src_format)) {
         vcos_assert(khrn_image_is_color(dst_format));
         vcos_assert(khrn_image_get_bpp(src_format) == 4);

         _vasm("v32and  H32(0++, 0) + %r, H32(0++, 0) + %r, 0xf REP 16", vr_data, vr_data);
         uint32_t i;
         for (i = 0; i != 16; ++i) {
            _vasm("v32mov  -, V32(0, 0) + %r CLRA UACC", vr_data + i);
            _vasm("v32lookupm  V32(0, 0) + %r, (%r)", vr_data + i, src_aux);
#ifdef WORKAROUND_HW1297
            _vasm("v32mov  V32(0, 0) + %r, V32(0, 0) + %r", vr_data + i, vr_data + i);
#endif
         }
         /* todo: lum/pre/lin conversion? */
         khrn_bf_blk_pack(vr_data, dst_format, conv);
      } else {
         vcos_assert(khrn_image_is_color(dst_format) && khrn_image_is_color(src_format));

         khrn_bf_blk_unpack(vr_data, src_format, conv);
         if (conv == IMAGE_CONV_VG) {
            blk_convert_l_pre_lin(vr_data, vr_temp, dst_format, src_format);
         }
         khrn_bf_blk_pack(vr_data, dst_format, conv);
      }
   } else {
      vcos_assert(!khrn_image_is_paletted(dst_format)); /* palettes might be different */
   }
}

/******************************************************************************
block tiled ld / channel masked st
******************************************************************************/

static uint32_t blk_ldpt(uint32_t vr_data, const BLK_LD_ST_T *src, int32_t x, int32_t y, KHRN_BF_TILE_T tile)
{
   vcos_assert((tile & BF_TILE_X) || ((x >= -15) && (x < (int32_t)src->sub_width)));
   vcos_assert((tile & BF_TILE_Y) || ((y >= -15) && (y < (int32_t)src->sub_height)));

   uint32_t sub_x = src->sub_x;
   uint32_t sub_y = src->sub_y;
   uint32_t sub_width = src->sub_width;
   uint32_t sub_height = src->sub_height;

   switch (tile & BF_TILE_MASK) {
   case BF_TILE_FILL:
   {
      uint32_t ldn = 0;
      if (tile & BF_TILE_X) {
         if ((x < -15) || (x >= (int32_t)sub_width)) { return BLK_LDN_NOP; }
         ldn |= (_max(-x, 0) << BLK_LDN_MIN_X_SHIFT) | (_min(sub_width - x, 16) << BLK_LDN_MAX_X_SHIFT);
      }
      if (tile & BF_TILE_Y) {
         if ((y < -15) || (y >= (int32_t)sub_height)) { return BLK_LDN_NOP; }
         ldn |= (_max(-y, 0) << BLK_LDN_MIN_Y_SHIFT) | (_min(sub_height - y, 16) << BLK_LDN_MAX_Y_SHIFT);
      }
      return blk_ldp(vr_data, src, x, y) | ldn;
   }
   case BF_TILE_PAD:
   {
      uint32_t ldn = 0;
      if (tile & BF_TILE_X) {
         if (x <= -15) {
            x = sub_x & 0xf;
            ldn |= (x << BLK_LDN_MIN_X_SHIFT) | ((x + 1) << BLK_LDN_MAX_X_SHIFT);
            x = -x;
         } else if (x >= (int32_t)(sub_width - 1)) {
            x = (sub_x + (sub_width - 1)) & 0xf;
            ldn |= (x << BLK_LDN_MIN_X_SHIFT) | ((x + 1) << BLK_LDN_MAX_X_SHIFT);
            x = (sub_width - 1) - x;
         } else {
            ldn |= (_max(-x, 0) << BLK_LDN_MIN_X_SHIFT) | (_min(sub_width - x, 16) << BLK_LDN_MAX_X_SHIFT);
         }
      }
      if (tile & BF_TILE_Y) {
         if (y <= -15) {
            y = sub_y & 0xf;
            ldn |= (y << BLK_LDN_MIN_Y_SHIFT) | ((y + 1) << BLK_LDN_MAX_Y_SHIFT);
            y = -y;
         } else if (y >= (int32_t)(sub_height - 1)) {
            y = (sub_y + (sub_height - 1)) & 0xf;
            ldn |= (y << BLK_LDN_MIN_Y_SHIFT) | ((y + 1) << BLK_LDN_MAX_Y_SHIFT);
            y = (sub_height - 1) - y;
         } else {
            ldn |= (_max(-y, 0) << BLK_LDN_MIN_Y_SHIFT) | (_min(sub_height - y, 16) << BLK_LDN_MAX_Y_SHIFT);
         }
      }
      return blk_ldp(vr_data, src, x, y) | ldn;
   }
   case BF_TILE_REPEAT:
   {
      uint32_t ldn = 0;
      if (tile & BF_TILE_X) {
         x = mod(x, sub_width);
         if ((uint32_t)(x + 16) > sub_width) {
            ldn |= BLK_LDN_REPEAT_X;
         }
      }
      if (tile & BF_TILE_Y) {
         y = mod(y, sub_height);
         if ((uint32_t)(y + 16) > sub_height) {
            ldn |= BLK_LDN_REPEAT_Y;
         }
      }
      if (!((ldn ^ BLK_LDN_REPEAT_Y) & (BLK_LDN_REPEAT_X | BLK_LDN_REPEAT_Y))) {
         ldn |= (_min(sub_height - y, 16) << BLK_LDN_REPEAT_MAX_XY_SHIFT) | (x << BLK_LDN_REPEAT_XY_SHIFT);
      } else if (!((ldn ^ BLK_LDN_REPEAT_X) & (BLK_LDN_REPEAT_X | BLK_LDN_REPEAT_Y))) {
         ldn |= (_min(sub_width - x, 16) << BLK_LDN_REPEAT_MAX_XY_SHIFT) | (y << BLK_LDN_REPEAT_XY_SHIFT);
      } else {
         ldn |= (_min(sub_width - x, 16) << BLK_LDN_MAX_X_SHIFT) | (_min(sub_height - y, 16) << BLK_LDN_MAX_Y_SHIFT);
      }
      return blk_ldp(vr_data, src, x, y) | ldn;
   }
   case BF_TILE_REFLECT:
   {
      uint32_t ldn = 0;
      if (tile & BF_TILE_X) {
         x = mod(x + 8, 2 * sub_width);
         if ((uint32_t)x > sub_width) {
            x = (2 * sub_width) - x;
            ldn |= BLK_LDN_REFLECT_X;
         }
         x -= 8;
         ldn |= (_max(-x, 0) << BLK_LDN_MIN_X_SHIFT) | (_min(sub_width - x, 16) << BLK_LDN_MAX_X_SHIFT);
      }
      if (tile & BF_TILE_Y) {
         y = mod(y + 8, 2 * sub_height);
         if ((uint32_t)y > sub_height) {
            y = (2 * sub_height) - y;
            ldn |= BLK_LDN_REFLECT_Y;
         }
         y -= 8;
         ldn |= (_max(-y, 0) << BLK_LDN_MIN_Y_SHIFT) | (_min(sub_height - y, 16) << BLK_LDN_MAX_Y_SHIFT);
      }
      return blk_ldp(vr_data, src, x, y) | ldn;
   }
   default:
   {
      UNREACHABLE();
      return 0;
   }
   }
}

static void blk_ldpt_unpack(
   uint32_t vr_data, uint32_t vr_temp, uint32_t ldn,
   const BLK_LD_ST_T *src,
   KHRN_BF_TILE_T tile, uint32_t tile_fill_rgba,
   KHRN_IMAGE_FORMAT_T dst_format, KHRN_IMAGE_CONV_T conv)
{
   KHRN_IMAGE_FORMAT_T src_format = src->format;
   uint32_t sub_width = src->sub_width;
   uint32_t sub_height = src->sub_height;

   switch (tile & BF_TILE_MASK) {
   case BF_TILE_FILL:
   {
      if (ldn & BLK_LDN_NOP) {
         _vasm("v32mov  H32(0++, 0) + %r, %r REP 16", vr_data, tile_fill_rgba);
      } else {
         verify(!blk_ldp_unpack(vr_data, false, ldn, src_format));

         if (tile & BF_TILE_UNPACK_AND_CONVERT) {
            khrn_bf_blk_unpack(vr_data, src_format, conv);
            if (conv == IMAGE_CONV_VG) {
               blk_convert_pre_lin(vr_data, vr_temp, dst_format, src_format);
            }
         }

         if (tile & BF_TILE_X) {
            uint32_t min_x = (ldn & BLK_LDN_MIN_X_MASK) >> BLK_LDN_MIN_X_SHIFT;
            uint32_t max_x = (ldn & BLK_LDN_MAX_X_MASK) >> BLK_LDN_MAX_X_SHIFT;
            _vasm("v32mov  V32(0, 0++) + %r, %r REP %r", vr_data, tile_fill_rgba, min_x);
            _vasm("v32mov  V32(0, 0++) + %r, %r REP %r", vr_data + max_x, tile_fill_rgba, 16 - max_x);
         }
         if (tile & BF_TILE_Y) {
            uint32_t min_y = (ldn & BLK_LDN_MIN_Y_MASK) >> BLK_LDN_MIN_Y_SHIFT;
            uint32_t max_y = (ldn & BLK_LDN_MAX_Y_MASK) >> BLK_LDN_MAX_Y_SHIFT;
            _vasm("v32mov  H32(0++, 0) + %r, %r REP %r", vr_data, tile_fill_rgba, min_y);
            _vasm("v32mov  H32(0++, 0) + %r, %r REP %r", vr_data + (max_y << 6), tile_fill_rgba, 16 - max_y);
         }
      }

      break;
   }
   case BF_TILE_PAD:
   {
      verify(!blk_ldp_unpack(vr_data, false, ldn, src_format));

      if (tile & BF_TILE_X) {
         uint32_t min_x = (ldn & BLK_LDN_MIN_X_MASK) >> BLK_LDN_MIN_X_SHIFT;
         uint32_t max_x = (ldn & BLK_LDN_MAX_X_MASK) >> BLK_LDN_MAX_X_SHIFT;
         _vasm("v32mov  V32(0, 0++) + %r, V32(0, 0) + %r REP %r", vr_data, vr_data + min_x, min_x);
         _vasm("v32mov  V32(0, 0++) + %r, V32(0, 0) + %r REP %r", vr_data + max_x, vr_data + (max_x - 1), 16 - max_x);
      }
      if (tile & BF_TILE_Y) {
         uint32_t min_y = (ldn & BLK_LDN_MIN_Y_MASK) >> BLK_LDN_MIN_Y_SHIFT;
         uint32_t max_y = (ldn & BLK_LDN_MAX_Y_MASK) >> BLK_LDN_MAX_Y_SHIFT;
         _vasm("v32mov  H32(0++, 0) + %r, H32(0, 0) + %r REP %r", vr_data, vr_data + (min_y << 6), min_y);
         _vasm("v32mov  H32(0++, 0) + %r, H32(0, 0) + %r REP %r", vr_data + (max_y << 6), vr_data + ((max_y - 1) << 6), 16 - max_y);
      }

      if (tile & BF_TILE_UNPACK_AND_CONVERT) {
         khrn_bf_blk_unpack(vr_data, src_format, conv);
         if (conv == IMAGE_CONV_VG) {
            blk_convert_pre_lin(vr_data, vr_temp, dst_format, src_format);
         }
      }

      break;
   }
   case BF_TILE_REPEAT:
   {
      verify(!blk_ldp_unpack(vr_data, false, ldn, src_format));

      uint32_t max_x, max_y;
      int32_t x, y;
      if (!((ldn ^ BLK_LDN_REPEAT_Y) & (BLK_LDN_REPEAT_X | BLK_LDN_REPEAT_Y))) {
         max_x = 16;
         max_y = (ldn & BLK_LDN_REPEAT_MAX_XY_MASK) >> BLK_LDN_REPEAT_MAX_XY_SHIFT;
         x = ((int32_t)(ldn & BLK_LDN_REPEAT_XY_MASK)) >> BLK_LDN_REPEAT_XY_SHIFT;
         y = sub_height - max_y;
      } else if (!((ldn ^ BLK_LDN_REPEAT_X) & (BLK_LDN_REPEAT_X | BLK_LDN_REPEAT_Y))) {
         max_x = (ldn & BLK_LDN_REPEAT_MAX_XY_MASK) >> BLK_LDN_REPEAT_MAX_XY_SHIFT;
         max_y = 16;
         x = sub_width - max_x;
         y = ((int32_t)(ldn & BLK_LDN_REPEAT_XY_MASK)) >> BLK_LDN_REPEAT_XY_SHIFT;
      } else {
         max_x = (ldn & BLK_LDN_MAX_X_MASK) >> BLK_LDN_MAX_X_SHIFT;
         max_y = (ldn & BLK_LDN_MAX_Y_MASK) >> BLK_LDN_MAX_Y_SHIFT;
         x = sub_width - max_x;
         y = sub_height - max_y;
      }
      /* todo: try to align these loads? */
      if (ldn & BLK_LDN_REPEAT_X) {
         uint32_t ldn2 = blk_ldp(vr_temp, src, 0, y);
         verify(!blk_ldp_unpack(vr_temp, false, ldn2, src_format));
         _vasm("v32mov  V32(0, 0++) + %r, V32(0, 0++) + %r REP %r", vr_data + max_x, vr_temp, 16 - max_x);
      }
      if (ldn & BLK_LDN_REPEAT_Y) {
         uint32_t ldn2 = blk_ldp(vr_temp, src, x, 0);
         verify(!blk_ldp_unpack(vr_temp, false, ldn2, src_format));
         _vasm("v32mov  H32(0++, 0) + %r, H32(0++, 0) + %r REP %r", vr_data + (max_y << 6), vr_temp, 16 - max_y);
      }
      if (!(~ldn & (BLK_LDN_REPEAT_X | BLK_LDN_REPEAT_Y))) {
         uint32_t ldn2 = blk_ldp(vr_temp, src, 0, 0);
         verify(!blk_ldp_unpack(vr_temp, false, ldn2, src_format));
         _vasm("v16bitplanes  -, %r SETF", _bmask(-1, 16 - max_x));
         _vasm("v32mov  H32(0++, 0) + %r, H32(0++, 0) + %r IFNZ REP %r", vr_data + max_x + (max_y << 6), vr_temp, 16 - max_y);
      }
      if (ldn & BLK_LDN_REPEAT_X) {
         while ((max_x + sub_width) < 16) {
            _vasm("v32mov  V32(0, 0++) + %r, V32(0, 0++) + %r REP %r", vr_data + max_x + sub_width, vr_data + max_x, _min(16 - (max_x + sub_width), sub_width));
            sub_width += sub_width;
         }
      }
      if (ldn & BLK_LDN_REPEAT_Y) {
         while ((max_y + sub_height) < 16) {
            _vasm("v32mov  H32(0++, 0) + %r, H32(0++, 0) + %r REP %r", vr_data + ((max_y + sub_height) << 6), vr_data + (max_y << 6), _min(16 - (max_y + sub_height), sub_height));
            sub_height += sub_height;
         }
      }

      if (tile & BF_TILE_UNPACK_AND_CONVERT) {
         khrn_bf_blk_unpack(vr_data, src_format, conv);
         if (conv == IMAGE_CONV_VG) {
            blk_convert_pre_lin(vr_data, vr_temp, dst_format, src_format);
         }
      }

      break;
   }
   case BF_TILE_REFLECT:
   {
      verify(!blk_ldp_unpack(vr_data, false, ldn, src_format));

      uint32_t i;
      if (tile & BF_TILE_X) {
         uint32_t min_x = (ldn & BLK_LDN_MIN_X_MASK) >> BLK_LDN_MIN_X_SHIFT;
         uint32_t max_x = (ldn & BLK_LDN_MAX_X_MASK) >> BLK_LDN_MAX_X_SHIFT;
         vcos_assert((min_x <= 8) && (max_x >= 8));
         while (min_x != 0) {
            for (i = 0; i != _min(max_x - min_x, min_x); ++i) {
               _vasm("v32mov  V32(0, 0) + %r, V32(0, 0) + %r", vr_data + ((min_x - 1) - i), vr_data + min_x + i);
            }
            min_x -= i;
         }
         for (i = 0; i != (16 - max_x); ++i) {
            _vasm("v32mov  V32(0, 0) + %r, V32(0, 0) + %r", vr_data + max_x + i, vr_data + ((max_x - 1) - i));
         }
         if (ldn & BLK_LDN_REFLECT_X) {
            for (i = 0; i != 8; ++i) {
               _vasm("v32eor  V32(0, 0) + %r, V32(0, 0) + %r, V32(0, 0) + %r", vr_data + i, vr_data + i, vr_data + (15 - i));
               _vasm("v32eor  V32(0, 0) + %r, V32(0, 0) + %r, V32(0, 0) + %r", vr_data + (15 - i), vr_data + i, vr_data + (15 - i));
               _vasm("v32eor  V32(0, 0) + %r, V32(0, 0) + %r, V32(0, 0) + %r", vr_data + i, vr_data + i, vr_data + (15 - i));
            }
         }
      }
      if (tile & BF_TILE_Y) {
         uint32_t min_y = (ldn & BLK_LDN_MIN_Y_MASK) >> BLK_LDN_MIN_Y_SHIFT;
         uint32_t max_y = (ldn & BLK_LDN_MAX_Y_MASK) >> BLK_LDN_MAX_Y_SHIFT;
         vcos_assert((min_y <= 8) && (max_y >= 8));
         while (min_y != 0) {
            for (i = 0; i != _min(max_y - min_y, min_y); ++i) {
               _vasm("v32mov  H32(0, 0) + %r, H32(0, 0) + %r", vr_data + (((min_y - 1) - i) << 6), vr_data + ((min_y + i) << 6));
            }
            min_y -= i;
         }
         for (i = 0; i != (16 - max_y); ++i) {
            _vasm("v32mov  H32(0, 0) + %r, H32(0, 0) + %r", vr_data + ((max_y + i) << 6), vr_data + (((max_y - 1) - i) << 6));
         }
         if (ldn & BLK_LDN_REFLECT_Y) {
            for (i = 0; i != 8; ++i) {
               _vasm("v32eor  H32(0, 0) + %r, H32(0, 0) + %r, H32(0, 0) + %r", vr_data + (i << 6), vr_data + (i << 6), vr_data + ((15 - i) << 6));
               _vasm("v32eor  H32(0, 0) + %r, H32(0, 0) + %r, H32(0, 0) + %r", vr_data + ((15 - i) << 6), vr_data + (i << 6), vr_data + ((15 - i) << 6));
               _vasm("v32eor  H32(0, 0) + %r, H32(0, 0) + %r, H32(0, 0) + %r", vr_data + (i << 6), vr_data + (i << 6), vr_data + ((15 - i) << 6));
            }
         }
      }

      if (tile & BF_TILE_UNPACK_AND_CONVERT) {
         khrn_bf_blk_unpack(vr_data, src_format, conv);
         if (conv == IMAGE_CONV_VG) {
            blk_convert_pre_lin(vr_data, vr_temp, dst_format, src_format);
         }
      }

      break;
   }
   default:
   {
      UNREACHABLE();
   }
   }
}

static void blk_stap_channel_masked(
   uint32_t vr_data, uint32_t vr_temp, bool tf,
   const BLK_LD_ST_T *dst, int32_t x, int32_t y,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T src_format, KHRN_IMAGE_CONV_T conv)
{
   KHRN_IMAGE_FORMAT_T dst_format = dst->format;

   if (channel_mask == 0xf) {
      if (conv == IMAGE_CONV_VG) {
         blk_convert_l_pre_lin(vr_data, vr_temp, dst_format, src_format);
      }
   } else {
      if (conv == IMAGE_CONV_VG) {
         blk_convert_l_pre_lin(vr_data, vr_temp, (KHRN_IMAGE_FORMAT_T)(dst_format & ~IMAGE_FORMAT_PRE), src_format);
      }
      uint32_t ldn = blk_ldap(vr_temp, dst, x, y);
      blk_ldap_unpack(vr_temp, tf, ldn, dst_format);
      khrn_bf_blk_unpack(vr_temp, dst_format, conv);
      if ((conv == IMAGE_CONV_VG) && (dst_format & IMAGE_FORMAT_PRE)) {
         uint32_t vr_unused_channel = vr_data + (_msb(~channel_mask & 0xf) * 16);
         uint32_t vr_used_channel_1 = vr_data + ((~channel_mask & 0xe) ? 0 : 16);
         uint32_t vr_used_channel_2 = vr_data + ((~channel_mask & 0xc) ? 16 : 32);
         uint32_t vr_used_channel_3 = vr_data + ((~channel_mask & 0x8) ? 32 : 48);
         _vasm("v16mov  H8(4++, 0) + %r, H8(0++, 0) + %r REP 4", vr_unused_channel, vr_used_channel_1);
         _vasm("v16mov  H8(8++, 0) + %r, H8(0++, 0) + %r REP 4", vr_unused_channel, vr_used_channel_2);
         _vasm("v16mov  H8(12++, 0) + %r, H8(0++, 0) + %r REP 4", vr_unused_channel, vr_used_channel_3);
         khrn_bf_blk_unpre(vr_temp, vr_data);
         _vasm("v16mov  H8(0++, 0) + %r, H8(4++, 0) + %r REP 4", vr_used_channel_1, vr_unused_channel);
         _vasm("v16mov  H8(0++, 0) + %r, H8(8++, 0) + %r REP 4", vr_used_channel_2, vr_unused_channel);
         _vasm("v16mov  H8(0++, 0) + %r, H8(12++, 0) + %r REP 4", vr_used_channel_3, vr_unused_channel);
      }
      if (!(channel_mask & 0x1)) { _vasm("v16mov  H8(0++, 0) + %r, H8(0++, 0) + %r REP 16", vr_data, vr_temp); }
      if (!(channel_mask & 0x2)) { _vasm("v16mov  H8(0++, 16) + %r, H8(0++, 16) + %r REP 16", vr_data, vr_temp); }
      if (!(channel_mask & 0x4)) { _vasm("v16mov  H8(0++, 32) + %r, H8(0++, 32) + %r REP 16", vr_data, vr_temp); }
      if (!(channel_mask & 0x8)) { _vasm("v16mov  H8(0++, 48) + %r, H8(0++, 48) + %r REP 16", vr_data, vr_temp); }
      if ((conv == IMAGE_CONV_VG) && (dst_format & IMAGE_FORMAT_PRE)) {
         khrn_bf_blk_pre(vr_data);
      }
   }
   khrn_bf_blk_pack(vr_data, dst_format, conv);
   blk_stap(vr_data, tf, dst, x, y);
}

/******************************************************************************
block filter stuff
******************************************************************************/

extern void khrn_bf_blk_color_matrix(uint32_t vr_data, uint32_t vr_temp, uint32_t vr_matrix);
extern void khrn_bf_blk_sconv_8_8(uint32_t vr_dst, uint32_t rows_n, uint32_t vr_src, uint32_t vr_temp, uint32_t vr_kernel, uint32_t kernel_size);
extern void khrn_bf_blk_sconv_8_16(uint32_t vr_dst, uint32_t rows_n, uint32_t vr_src, uint32_t vr_temp, uint32_t vr_kernel, uint32_t kernel_size);
extern void khrn_bf_blk_sconv_8_32(uint32_t vr_dst, uint32_t rows_n, uint32_t vr_src, uint32_t vr_kernel, uint32_t kernel_size); /* no bias or shift */
extern void khrn_bf_blk_sconv_16_8(uint32_t vr_dst, uint32_t rows_n, uint32_t vr_src, uint32_t vr_temp, uint32_t vr_kernel, uint32_t kernel_size);
extern void khrn_bf_blk_conv(uint32_t vr_dst, uint32_t vr_src, uint32_t vr_kernel, uint32_t kernel_width, uint32_t kernel_height);

/******************************************************************************
ppulm
******************************************************************************/

extern void khrn_bf_blk_st_ppulm(uint32_t vr_data, uint32_t offset);
extern void khrn_bf_blk_ld_ppulm(uint32_t vr_data, uint32_t offset);

static void ppulm_ld_lut8(uint32_t offset, const uint8_t *lut)
{
   offset >>= 2; /* actually use 32-bit writes */

   _vasm("v8ld  H8(0++, 0), (%r += %r) REP 16", lut, 16);
   uint32_t vr_lut;
   for (vr_lut = 0 << 6; vr_lut != (16 << 6); vr_lut += (4 << 6)) {
      _vasm("v16even  H8(16, 0), H8(0, 0) + %r, H8(1, 0) + %r", vr_lut, vr_lut);
      _vasm("v16even  H8(17, 0), H8(2, 0) + %r, H8(3, 0) + %r", vr_lut, vr_lut);
      _vasm("v16odd  H8(18, 0), H8(0, 0) + %r, H8(1, 0) + %r", vr_lut, vr_lut);
      _vasm("v16odd  H8(19, 0), H8(2, 0) + %r, H8(3, 0) + %r", vr_lut, vr_lut);
      _vasm("v16even  H8(20, 0), H8(16, 0), H8(17, 0)");
      _vasm("v16even  H8(20, 16), H8(18, 0), H8(19, 0)");
      _vasm("v16odd  H8(20, 32), H8(16, 0), H8(17, 0)");
      _vasm("v16odd  H8(20, 48), H8(18, 0), H8(19, 0)");
#ifndef __VIDEOCORE4__
      _vasm("v32mov  H32(16++, 0), H32(20, 0) REP 16");
#endif
      uint32_t i;
      for (i = 0; i != 16; ++i) {
#ifdef __VIDEOCORE4__
         _vasm("v32memwrite  -, V32(20, 0) + %r, %r", 0x1000 + i, offset++);
#else
         _vasm("v32memwrite  -, V32(16, 0) + %r, %r", i, offset++);
#endif
      }
   }
}

static void ppulm_ld_lut32(uint32_t offset, const uint32_t *lut)
{
   _vasm("v32ld  H32(0++, 0), (%r += %r) REP 16", lut, 64);
   uint32_t vr_lut;
   for (vr_lut = 0 << 6; vr_lut != (16 << 6); vr_lut += (1 << 6)) {
      _vasm("v16mov  H8(16, 0), H8(0, 48) + %r", vr_lut);
      _vasm("v16mov  H8(16, 16), H8(0, 32) + %r", vr_lut);
      _vasm("v16mov  H8(16, 32), H8(0, 16) + %r", vr_lut);
      _vasm("v16mov  H8(16, 48), H8(0, 0) + %r", vr_lut);
#ifndef __VIDEOCORE4__
      _vasm("v32mov  H32(16++, 0), H32(16, 0) REP 16");
#endif
      uint32_t i;
      for (i = 0; i != 16; ++i) {
#ifdef __VIDEOCORE4__
         _vasm("v32memwrite  -, V32(16, 0) + %r, %r", 0x1000 + i, offset++);
#else
         _vasm("v32memwrite  -, V32(16, 0) + %r, %r", i, offset++);
#endif
      }
   }
}

/******************************************************************************
um
******************************************************************************/

#ifndef __VIDEOCORE4__ /* no uniforms memory attached to vpu on 2708 */

extern void khrn_bf_um_stall(void);

static INLINE void um_ld(uint32_t offset, uint32_t x)
{
   ((uint32_t *)GR_UNIFORM_BASE)[offset] = x;
}

static void um_ld_16s(uint32_t offset, uint32_t stride,
   const int16_t *lows, uint32_t lows_n, const int16_t *highs, uint32_t highs_n)
{
   uint32_t i;
   for (i = 0; i != _max(lows_n, highs_n); ++i) {
      uint32_t x = 0;
      if (i < lows_n) {
         x |= (uint16_t)lows[i];
      }
      if (i < highs_n) {
         x |= (uint32_t)highs[i] << 16;
      }
      um_ld(offset, x);
      offset += stride;
   }
}

static void um_ld_32s(uint32_t offset, uint32_t stride,
   const int32_t *xs, uint32_t n)
{
   uint32_t i;
   for (i = 0; i != n; ++i) {
      um_ld(offset, xs[i]);
      offset += stride;
   }
}

#endif

/******************************************************************************
helpers
******************************************************************************/

static void get_optimal_begin(
   int32_t *x, int32_t *y, uint32_t *offset_x_out, uint32_t *offset_y_out,
   int32_t dst_x, int32_t dst_y, KHRN_IMAGE_FORMAT_T dst_format,
   int32_t src_x, int32_t src_y, KHRN_IMAGE_FORMAT_T src_format,
   uint32_t offset_x_max, uint32_t offset_y_max)
{
   uint32_t dst_align = blk_get_align(dst_format);
   uint32_t src_align = blk_get_align(src_format);

   uint32_t offset_x = (src_x - dst_x) & (dst_align & 0xf) & (src_align & 0xf);
   *x = (((src_align & 0xf) < (dst_align & 0xf)) || (offset_x > offset_x_max)) ?
      -(dst_x & (dst_align & 0xf)) :
      -((src_x - offset_x) & (src_align & 0xf));
   if (offset_x_out) { *offset_x_out = (offset_x > offset_x_max) ? 0 : offset_x; }

   uint32_t offset_y = (src_y - dst_y) & (dst_align >> 4) & (src_align >> 4);
   *y = (((src_align >> 4) < (dst_align >> 4)) || (offset_y > offset_y_max)) ?
      -(dst_y & (dst_align >> 4)) :
      -((src_y - offset_y) & (src_align >> 4));
   if (offset_y_out) { *offset_y_out = (offset_y > offset_y_max) ? 0 : offset_y; }
}

static INLINE int32_t get_exp(float x)
{
   return ((float_to_bits(x) >> 23) & 0xff) - 127;
}

/* sign of x is ignored, which is slightly conservative, but means we don't have
 * to worry about it when calling (we just have to get the magnitude right) */
static INLINE int32_t get_optimal_shift(float x, uint32_t n)
{
   /* want to fit x in n bits (signed). with a shift of 0 we have:
    * (sign bit) (hidden bit) (exp bits of mantissa)
    * so to fit in exactly, we want a shift of n - (2 + exp) */
   int32_t shift = n - (2 + get_exp(x));

   /* but rounding might make us overflow. consider this float, just less than
    * 2.0:
    * 0x3fffffff, exp = 0, mantissa = 0x7fffff
    * if we wanted to fit this in 2 bits, we would calculate:
    * shift = 2 - (2 + 0) = 0
    * but float_to_int_shift(0x3fffffff, 0) is 2, which doesn't fit! */
   return ((shift >= -129) && (shift <= 124)) ?
      (n - (2 + get_exp(r2ni_to_r2n_bias(absf_(x), shift)))) : shift;
}

static int32_t shift_int(int32_t x, int32_t y)
{
   vcos_assert(y <= 31);
   if (y > 0) {
      return x >> y;
   }
   if (y < -31) {
      return (x < 0) ? -0x80000000 : (x ? 0x7fffffff : 0);
   }
   return _shls(x, -y);
}

#ifdef WORKAROUND_HW1394
static bool susceptible_to_hw1394(KHRN_IMAGE_FORMAT_T format) {
   vcos_assert(khrn_image_is_uncomp(format));
   if (khrn_image_is_rso(format)) {
      switch (format & IMAGE_FORMAT_PIXEL_SIZE_MASK) {
      case IMAGE_FORMAT_4:  return true;
      case IMAGE_FORMAT_8:  return true;
      case IMAGE_FORMAT_24: return true;
      }
   }
   return false;
}
#endif

/******************************************************************************
clearing/blitting
******************************************************************************/

bool khrn_bf_clear_region(
   KHRN_IMAGE_WRAP_T *wrap, uint32_t x, uint32_t y,
   uint32_t width, uint32_t height,
   uint32_t rgba, /* rgba non-lin, unpre */
   KHRN_IMAGE_CONV_T conv)
{
   vcos_assert(khrn_image_is_color(wrap->format));
   vcos_assert((x + width) <= wrap->width);
   vcos_assert((y + height) <= wrap->height);

   if ((width == 0) || (height == 0)) {
      return true;
   }

#ifdef WORKAROUND_HW1394
   if (susceptible_to_hw1394(wrap->format)) {
      return false;
   }
#endif

   KHRN_IMAGE_FORMAT_T format = wrap->format;
   int32_t stride = wrap->stride;
   void *data = wrap->storage;

   vclib_obtain_VRF(1);

   if (conv == IMAGE_CONV_VG) {
      rgba = khrn_image_rgba_convert_l_pre_lin(format, (KHRN_IMAGE_FORMAT_T)0 /* rgba, non-lin, non-pre */, rgba);
   }
   _vasm("v32mov  H32(0++, 0), %r REP 32", khrn_image_rgba_to_pixel(format, rgba, conv));
   khrn_bf_blk_sta_pack(0 << 6, blk_prefer_tf(format), format);
   khrn_bf_blk_stam_pack(16 << 6, blk_prefer_tf(format), format);

   uint32_t align = blk_get_align(format);
   uint32_t i, j;
   for (j = y & ~(align >> 4); j < (y + height); j += 16) {
      uint32_t mask_y = _bmask(-1, _min(j + 16, y + height) - _max(j, y)) << (_max(j, y) - j);
      for (i = x & ~(align & 0xf); i < (x + width); i += 16) {
         uint32_t mask_x = _bmask(-1, _min(i + 16, x + width) - _max(i, x)) << (_max(i, x) - i);
         void *blk_data = (uint8_t *)data + blk_get_offset(stride, i, j, format);
         if ((mask_x & mask_y) == 0xffff) {
            khrn_bf_blk_sta(0 << 6, blk_data, stride, format);
         } else {
            khrn_bf_blk_stam(16 << 6, blk_data, stride, mask_x, mask_y, format);
         }
      }
   }

   vclib_release_VRF();

   return true;
}

static INLINE uint32_t get_packed_mask_y(uint32_t y)
{
   return ((y / (VG_Q_TILE_HEIGHT * 4)) * VG_Q_TILE_HEIGHT) + (y & (VG_Q_TILE_HEIGHT - 1));
}

static bool copy_region_from_packed_mask_tf(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y)
{
   vcos_assert((khrn_image_is_uncomp(dst->format) && (khrn_image_get_bpp(dst->format) == 8)) && (src->format == PACKED_MASK_TF));
   vcos_assert((dst_x + width) <= dst->width);
   vcos_assert((dst_y + height) <= dst->height);
   vcos_assert((src_x + width) <= src->width);
   vcos_assert((src_y + height) <= src->height);

   if ((width == 0) || (height == 0)) {
      return true;
   }

#ifdef WORKAROUND_HW1394
   if (susceptible_to_hw1394(dst->format)) {
      return false;
   }
#endif

   BLK_LD_ST_T dst_ld_st, src_ld_st;
   blk_ld_st_init(&dst_ld_st, dst, dst_x, dst_y, width, height);
   src_ld_st.data = src->storage;
   src_ld_st.stride = src->stride;
   src_ld_st.format = (KHRN_IMAGE_FORMAT_T)(IMAGE_FORMAT_TF | IMAGE_FORMAT_UNCOMP | IMAGE_FORMAT_32);
   src_ld_st.sub_x = 0;
   src_ld_st.sub_y = 0;
   src_ld_st.sub_width = src->width;
   src_ld_st.sub_height = (uint16_t)khrn_image_get_packed_mask_height(src->height);
   src_ld_st.width = (uint16_t)round_up(src_ld_st.sub_width, 32);
   src_ld_st.height = (uint16_t)round_up(src_ld_st.sub_height, 32);

   vclib_obtain_VRF(1);

   uint32_t vr = 0 << 6;
   uint32_t i = src_x & ~0xf, j = src_y & ~0xf;
   uint32_t ldn = blk_ldap(vr, &src_ld_st, i, get_packed_mask_y(j));
   do {
      uint32_t next_vr = (vr + (16 << 6)) & 0xfff;

      uint32_t next_i = i + 16;
      uint32_t next_j = j;
      if (next_i >= (src_x + width)) {
         next_i = src_x & ~0xf;
         next_j += 16;
      }

      uint32_t next_ldn = 0;
      if (next_j < (src_y + height)) {
         next_ldn = blk_ldap(next_vr, &src_ld_st, next_i, get_packed_mask_y(next_j));
      }

      blk_ldap_unpack(vr, false, ldn, src_ld_st.format);
      uint32_t channel = (j / VG_Q_TILE_HEIGHT) & 0x3;
      if (channel != 0) {
         _vasm("v16mov  H8(0++, 0) + %r, H8(0++, 0) + %r REP 16", vr, vr + (channel * 16));
      }
      blk_stp(vr, &dst_ld_st, i - src_x, j - src_y);

      vr = next_vr;
      i = next_i;
      j = next_j;
      ldn = next_ldn;
   } while (j < (src_y + height));

   vclib_release_VRF();

   return true;
}

bool khrn_bf_copy_region(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
   vcos_assert(khrn_image_is_uncomp(dst->format) && (khrn_image_is_uncomp(src->format) || (src->format == PACKED_MASK_TF)));
   vcos_assert((dst_x + width) <= dst->width);
   vcos_assert((dst_y + height) <= dst->height);
   vcos_assert((src_x + width) <= src->width);
   vcos_assert((src_y + height) <= src->height);

   if (src->format == PACKED_MASK_TF) {
      return copy_region_from_packed_mask_tf(dst, dst_x, dst_y, width, height, src, src_x, src_y);
   }

   if ((width == 0) || (height == 0)) {
      return true;
   }

#ifdef BF_USE_VC_IMAGE_RASTER_TO_TFORMAT
   KHRN_IMAGE_FORMAT_T dst_format = dst->format &~ (IMAGE_FORMAT_PRE|IMAGE_FORMAT_LIN);
   KHRN_IMAGE_FORMAT_T src_format = src->format &~ (IMAGE_FORMAT_PRE|IMAGE_FORMAT_LIN);
   if ((dst_x == 0) && !(dst_y & 0x3) &&
       (src_x == 0) && (src_y == 0) &&
       (dst_format == ABGR_8888_TF) && ((src_format == ARGB_8888_RSO) ||
                                        (src_format == RGBA_8888_RSO) ||
                                        (src_format == ABGR_8888_RSO) ||
                                        (src_format == BGRA_8888_RSO)) &&
       (!(width & 0xf) && !(height & 0x3)) || /* we either meet vc_image size requirements, or... */
        ((width == dst->width) && ((dst_y + height) == dst->height) && /* we won't overwrite into valid destination pixels and */
          khdispatch_within_workspace(src->storage, (src->stride * (((height+0x3)&~0x3)-1)) + (((width+0xf)&~0xf)*4))) /* our source overread is safely within khdispatch workspace */
       ) {
      VC_IMAGE_T dst_vc_image, src_vc_image;
      int conv_flags = 0;
      if (src_format & IMAGE_FORMAT_XA) conv_flags |= 2;     /* rotate xxxA to Axxx */
      if (src_format & IMAGE_FORMAT_XRGBX) conv_flags |= 1;  /* swap R and B */
      vc_image_initialise(&dst_vc_image);
      vc_image_set_type(&dst_vc_image, VC_IMAGE_TF_RGBA32);
      vc_image_set_dimensions(&dst_vc_image, width, dst_y + height);
      dst_vc_image.pitch = dst->stride; /* i was calling vc_image_set_pitch(&dst_vc_image, dst->stride), but there's a dubious assert(!(pitch & 63)) */
      vc_image_set_image_data(&dst_vc_image, 0, dst->storage);
      vc_image_initialise(&src_vc_image);
      vc_image_set_type(&src_vc_image, VC_IMAGE_RGBA32); /* the conversion function doesn't care about the specified format, and uses flags to shuffle channels instead */
      vc_image_set_dimensions(&src_vc_image, width, height);
      src_vc_image.pitch = src->stride; /* i was calling vc_image_set_pitch(&src_vc_image, src->stride), but there's a dubious assert(!(pitch & 63)) */
      vc_image_set_image_data(&src_vc_image, 0, src->storage);
      vclib_obtain_VRF(1);
      vc_image_rgba32_to_tf_rgba32(&dst_vc_image, dst_y, &src_vc_image, TRANSFORM_VFLIP, conv_flags);
      vclib_release_VRF();
      return true;
   }
#endif

#ifdef WORKAROUND_HW1394
   if (susceptible_to_hw1394(dst->format) || susceptible_to_hw1394(src->format)) {
      return false;
   }
#endif

   BLK_LD_ST_T dst_ld_st, src_ld_st;
   blk_ld_st_init(&dst_ld_st, dst, dst_x, dst_y, width, height);
   blk_ld_st_init(&src_ld_st, src, src_x, src_y, width, height);
   const void *src_aux = src->aux;

   int32_t begin_i, begin_j;
   get_optimal_begin(&begin_i, &begin_j, NULL, NULL,
      dst_ld_st.sub_x, dst_ld_st.sub_y, dst_ld_st.format,
      src_ld_st.sub_x, src_ld_st.sub_y, src_ld_st.format,
      0, 0);
   int32_t end_i = begin_i + ((-begin_i + width + 0xf) & ~0xf);
   int32_t end_j = begin_j + ((-begin_j + height + 0xf) & ~0xf);
   int32_t dir_i = 16, dir_j = 16;
   if (dst_ld_st.data == src_ld_st.data) {
      if (dst_ld_st.sub_x > src_ld_st.sub_x) {
         int32_t temp = begin_i;
         begin_i = end_i - 16;
         end_i = temp - 16;
         dir_i = -16;
      }
      if (dst_ld_st.sub_y > src_ld_st.sub_y) {
         int32_t temp = begin_j;
         begin_j = end_j - 16;
         end_j = temp - 16;
         dir_j = -16;
      }
   }

   vclib_obtain_VRF(1);

   uint32_t vr = 0 << 6;
   int32_t i = begin_i, j = begin_j;
   uint32_t ldn = blk_ldp(vr, &src_ld_st, i, j);
   do {
      uint32_t next_vr = (vr + (16 << 6)) & 0xfff;

      int32_t next_i = i + dir_i;
      int32_t next_j = j;
      if (next_i == end_i) {
         next_i = begin_i;
         next_j += dir_j;
      }

      uint32_t next_ldn = 0;
      if (next_j != end_j) {
         next_ldn = blk_ldp(next_vr, &src_ld_st, next_i, next_j);
      }

      bool tf = blk_ldp_unpack(vr, blk_prefer_tf(dst_ld_st.format), ldn, src_ld_st.format);
      blk_convert(vr, (vr - (16 << 6)) & 0xfff, src_aux, dst_ld_st.format, src_ld_st.format, conv);
      blk_stap(vr, tf, &dst_ld_st, i, j);

      vr = next_vr;
      i = next_i;
      j = next_j;
      ldn = next_ldn;
   } while (j != end_j);

   vclib_release_VRF();

   return true;
}

bool khrn_bf_copy_scissor_regions(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv,
   const int32_t *scissor_rects, uint32_t scissor_rects_count)
{
   vcos_assert(khrn_image_is_uncomp(dst->format) && khrn_image_is_uncomp(src->format));
   vcos_assert((dst_x + width) <= dst->width);
   vcos_assert((dst_y + height) <= dst->height);
   vcos_assert((src_x + width) <= src->width);
   vcos_assert((src_y + height) <= src->height);

   if ((width == 0) || (height == 0)) {
      return true;
   }

#ifdef WORKAROUND_HW1394
   if (susceptible_to_hw1394(dst->format) || susceptible_to_hw1394(src->format)) {
      return false;
   }
#endif

   BLK_LD_ST_T dst_ld_st, src_ld_st;
   blk_ld_st_init(&dst_ld_st, dst, dst_x, dst_y, width, height);
   blk_ld_st_init(&src_ld_st, src, src_x, src_y, width, height);
   const void *src_aux = src->aux;

   int32_t begin_i, begin_j;
   get_optimal_begin(&begin_i, &begin_j, NULL, NULL,
      dst_ld_st.sub_x, dst_ld_st.sub_y, dst_ld_st.format,
      src_ld_st.sub_x, src_ld_st.sub_y, src_ld_st.format,
      0, 0);
   int32_t end_i = begin_i + ((-begin_i + width + 0xf) & ~0xf);
   int32_t end_j = begin_j + ((-begin_j + height + 0xf) & ~0xf);
   int32_t dir_i = 16, dir_j = 16;
   if (dst_ld_st.data == src_ld_st.data) {
      if (dst_ld_st.sub_x > src_ld_st.sub_x) {
         int32_t temp = begin_i;
         begin_i = end_i - 16;
         end_i = temp - 16;
         dir_i = -16;
      }
      if (dst_ld_st.sub_y > src_ld_st.sub_y) {
         int32_t temp = begin_j;
         begin_j = end_j - 16;
         end_j = temp - 16;
         dir_j = -16;
      }
   }

   vclib_obtain_VRF(1);

   uint32_t vr = 0 << 6;
   int32_t i = begin_i, j = begin_j;
   uint32_t src_ldn = blk_ldp(vr, &src_ld_st, i, j);
   uint32_t dst_ldn = blk_ldap((vr + (32 << 6)) & 0xfff, &dst_ld_st, i, j);
   do {
      uint32_t next_vr = (vr + (16 << 6)) & 0xfff;

      int32_t next_i = i + dir_i;
      int32_t next_j = j;
      if (next_i == end_i) {
         next_i = begin_i;
         next_j += dir_j;
      }

      uint32_t next_src_ldn = 0;
      if (next_j != end_j) {
         next_src_ldn = blk_ldp(next_vr, &src_ld_st, next_i, next_j);
      }

      verify(!blk_ldp_unpack(vr, false, src_ldn, src_ld_st.format));
      blk_convert(vr, (vr - (16 << 6)) & 0xfff, src_aux, dst_ld_st.format, src_ld_st.format, conv);

      uint32_t next_dst_ldn = 0;
      if (next_j != end_j) {
         next_dst_ldn = blk_ldap((next_vr + (32 << 6)) & 0xfff, &dst_ld_st, next_i, next_j);
      }

      blk_ldap_unpack((vr + (32 << 6)) & 0xfff, false, dst_ldn, dst_ld_st.format);

      uint32_t k;
      for (k = 0; k != scissor_rects_count; k += 4) {
         int32_t rect_x      = scissor_rects[k];
         int32_t rect_y      = scissor_rects[k + 1];
         int32_t rect_width  = scissor_rects[k + 2];
         int32_t rect_height = scissor_rects[k + 3];
         khrn_clip_rect(
            &rect_x, &rect_y, &rect_width, &rect_height,
            dst_ld_st.sub_x + i, dst_ld_st.sub_y + j, 16, 16);
         rect_x -= dst_ld_st.sub_x + i;
         rect_y -= dst_ld_st.sub_y + j;
         _vasm("v16bitplanes  -, %r SETF", _bmask(-1, rect_width) << rect_x);
         _vasm("v32mov  H32(0++, 0) + %r, H32(0++, 0) + %r IFNZ REP %r", (vr + ((32 + rect_y) << 6)) & 0xfff, vr + (rect_y << 6), rect_height);
      }

      blk_stap((vr + (32 << 6)) & 0xfff, false, &dst_ld_st, i, j);

      vr = next_vr;
      i = next_i;
      j = next_j;
      src_ldn = next_src_ldn;
      dst_ldn = next_dst_ldn;
   } while (j != end_j);

   vclib_release_VRF();

   return true;
}

/******************************************************************************
filtering (on 2707b, vpu1 only)
******************************************************************************/

void khrn_bf_color_matrix_prepare(
   int16_t *matrix, int32_t *bias, int32_t *shift, const float *matrix_in)
{
   uint32_t i;

   float max_abs_matrix = 0.0f, max_abs_bias = 0.0f;
   for (i = 0; i != 16; ++i) {
      max_abs_matrix = _maxf(max_abs_matrix, absf_(matrix_in[i]));
   }
   for (i = 0; i != 4; ++i) {
      max_abs_bias = _maxf(max_abs_bias, absf_(matrix_in[16 + i]));
   }

   /* we want to get maximum precision (ie maximum shift) while satisfying these
    * requirements: */
   *shift = clampi(_min(
      /* main matrix should fit in signed 16-bit */
      get_optimal_shift(max_abs_matrix, 16),
      /* bias and result should fit in signed 32-bit
       * if the main matrix fits in signed 16-bit, then the result before
       * applying the bias should fit in ~26 bits, so if we instead make sure
       * the bias fits in signed 31-bit, the result won't overflow */
      get_optimal_shift(max_abs_bias * 255.0f, 31)),
      /* shift must be in [-32, 31] (float_to_int_shift requires this) */
      -32, 31);

   for (i = 0; i != 16; ++i) {
      matrix[i] = clampi(float_to_int_shift(matrix_in[i], *shift), -0x8000, 0x7fff);
   }
   for (i = 0; i != 4; ++i) {
      float b = matrix_in[16 + i] * 255.0f;
      if (*shift > 0) {
         b += 0.49999997f;
      }
      bias[i] = float_to_int_shift(b, *shift);
   }
}

void khrn_bf_color_matrix(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format, KHRN_IMAGE_CONV_T conv,
   const int16_t *matrix, const int32_t *bias, int32_t shift)
{
   vcos_assert(khrn_image_is_color(dst->format) && khrn_image_is_color(src->format));
   vcos_assert((dst_x + width) <= dst->width);
   vcos_assert((dst_y + height) <= dst->height);
   vcos_assert((src_x + width) <= src->width);
   vcos_assert((src_y + height) <= src->height);

   if ((width == 0) || (height == 0)) {
      return;
   }

   BLK_LD_ST_T dst_ld_st, src_ld_st;
   blk_ld_st_init(&dst_ld_st, dst, dst_x, dst_y, width, height);
   blk_ld_st_init(&src_ld_st, src, src_x, src_y, width, height);

   int32_t begin_i, begin_j;
   get_optimal_begin(&begin_i, &begin_j, NULL, NULL,
      dst_ld_st.sub_x, dst_ld_st.sub_y, dst_ld_st.format,
      src_ld_st.sub_x, src_ld_st.sub_y, src_ld_st.format,
      0, 0);

#ifndef __VIDEOCORE4__
   /* use uniforms memory for matrix/bias/shift */
   um_ld_16s(0, 64, matrix, 16, NULL, 0);
   um_ld_32s(1, 64, bias, 4);
   um_ld(2, -shift);
   khrn_bf_um_stall();
#endif

   vclib_obtain_VRF(1);

#ifdef __VIDEOCORE4__
   /* use 0x1000 replicating alias for matrix/bias/shift */
   _vasm("v16ld  H16(61, 0), (%r)", matrix);
   _vasm("v16bitplanes  -, 0xf SETF");
   _vasm("v32ld  H32(62, 0), (%r) IFNZ", bias);
   _vasm("v16mov  H16(63, 0), %r", -shift);
#endif

   uint32_t vr = 0 << 6;
   int32_t i = begin_i, j = begin_j;
   uint32_t ldn = blk_ldp(vr, &src_ld_st, i, j);
   do {
      uint32_t next_vr = vr + (16 << 6);
      if (next_vr == (48 << 6)) { next_vr = 0 << 6; }

      int32_t next_i = i + 16;
      int32_t next_j = j;
      if (next_i >= (int32_t)width) {
         next_i = begin_i;
         next_j += 16;
      }

      uint32_t next_ldn = 0;
      if (next_j < (int32_t)height) {
         next_ldn = blk_ldp(next_vr, &src_ld_st, next_i, next_j);
      }

      bool tf = blk_ldp_unpack(vr, blk_prefer_tf(dst_ld_st.format), ldn, src_ld_st.format);
      khrn_bf_blk_unpack(vr, src_ld_st.format, conv);
      uint32_t vr_temp = next_vr + (16 << 6);
      if (vr_temp == (48 << 6)) { vr_temp = 0 << 6; }
      if (conv == IMAGE_CONV_VG) {
         blk_convert_pre_lin(vr, vr_temp, format, src_ld_st.format);
      }
      khrn_bf_blk_color_matrix(vr, vr_temp,
#ifdef __VIDEOCORE4__
         0x1000 + (61 << 6));
#else
         0x1000);
#endif
      if ((conv == IMAGE_CONV_VG) && (format & IMAGE_FORMAT_PRE)) {
         khrn_bf_blk_clamp_to_a(vr);
      }
      blk_stap_channel_masked(vr, vr_temp, tf, &dst_ld_st, i, j, channel_mask, format, conv);

      vr = next_vr;
      i = next_i;
      j = next_j;
      ldn = next_ldn;
   } while (j < (int32_t)height);

   vclib_release_VRF();
}

void khrn_bf_lookup(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format, KHRN_IMAGE_CONV_T conv,
   const uint8_t *red_lut,
   const uint8_t *green_lut,
   const uint8_t *blue_lut,
   const uint8_t *alpha_lut,
   KHRN_IMAGE_FORMAT_T output_format)
{
   vcos_assert(khrn_image_is_color(dst->format) && khrn_image_is_color(src->format));
   vcos_assert((dst_x + width) <= dst->width);
   vcos_assert((dst_y + height) <= dst->height);
   vcos_assert((src_x + width) <= src->width);
   vcos_assert((src_y + height) <= src->height);

   if ((width == 0) || (height == 0)) {
      return;
   }

   BLK_LD_ST_T dst_ld_st, src_ld_st;
   blk_ld_st_init(&dst_ld_st, dst, dst_x, dst_y, width, height);
   blk_ld_st_init(&src_ld_st, src, src_x, src_y, width, height);

   int32_t begin_i, begin_j;
   get_optimal_begin(&begin_i, &begin_j, NULL, NULL,
      dst_ld_st.sub_x, dst_ld_st.sub_y, dst_ld_st.format,
      src_ld_st.sub_x, src_ld_st.sub_y, src_ld_st.format,
      0, 0);

   vclib_obtain_VRF(1);

   ppulm_ld_lut8(0, red_lut);
   ppulm_ld_lut8(256, green_lut);
   ppulm_ld_lut8(512, blue_lut);
   ppulm_ld_lut8(768, alpha_lut);

   uint32_t vr = 0 << 6;
   int32_t i = begin_i, j = begin_j;
   uint32_t ldn = blk_ldp(vr, &src_ld_st, i, j);
   do {
      uint32_t next_vr = (vr + (16 << 6)) & 0xfff;

      int32_t next_i = i + 16;
      int32_t next_j = j;
      if (next_i >= (int32_t)width) {
         next_i = begin_i;
         next_j += 16;
      }

      uint32_t next_ldn = 0;
      if (next_j < (int32_t)height) {
         next_ldn = blk_ldp(next_vr, &src_ld_st, next_i, next_j);
      }

      bool tf = blk_ldp_unpack(vr, blk_prefer_tf(dst_ld_st.format), ldn, src_ld_st.format);
      khrn_bf_blk_unpack(vr, src_ld_st.format, conv);
      uint32_t vr_temp = (vr - (16 << 6)) & 0xfff;
      if (conv == IMAGE_CONV_VG) {
         blk_convert_pre_lin(vr, vr_temp, format, src_ld_st.format);
      }
      _vasm("v8memread  H8(0++, 0) + %r, -, H8(0++, 0) + %r REP 16", vr, vr);
      _vasm("v16add  H16(0++, 0) + %r, H8(0++, 16) + %r, 256 REP 16", vr_temp, vr);
      _vasm("v8memread  H8(0++, 16) + %r, -, H16(0++, 0) + %r REP 16", vr, vr_temp);
      _vasm("v16add  H16(0++, 0) + %r, H8(0++, 32) + %r, 512 REP 16", vr_temp, vr);
      _vasm("v8memread  H8(0++, 32) + %r, -, H16(0++, 0) + %r REP 16", vr, vr_temp);
      _vasm("v16add  H16(0++, 0) + %r, H8(0++, 48) + %r, 768 REP 16", vr_temp, vr);
      _vasm("v8memread  H8(0++, 48) + %r, -, H16(0++, 0) + %r REP 16", vr, vr_temp);
      if ((conv == IMAGE_CONV_VG) && (output_format & IMAGE_FORMAT_PRE)) {
         khrn_bf_blk_clamp_to_a(vr);
      }
      blk_stap_channel_masked(vr, vr_temp, tf, &dst_ld_st, i, j, channel_mask, output_format, conv);

      vr = next_vr;
      i = next_i;
      j = next_j;
      ldn = next_ldn;
   } while (j < (int32_t)height);

   vclib_release_VRF();
}

void khrn_bf_lookup_single(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format, KHRN_IMAGE_CONV_T conv,
   uint32_t source_channel,
   const uint32_t *lut,
   KHRN_IMAGE_FORMAT_T output_format)
{
   vcos_assert(khrn_image_is_color(dst->format) && khrn_image_is_color(src->format));
   vcos_assert((dst_x + width) <= dst->width);
   vcos_assert((dst_y + height) <= dst->height);
   vcos_assert((src_x + width) <= src->width);
   vcos_assert((src_y + height) <= src->height);

   if ((width == 0) || (height == 0)) {
      return;
   }

   BLK_LD_ST_T dst_ld_st, src_ld_st;
   blk_ld_st_init(&dst_ld_st, dst, dst_x, dst_y, width, height);
   blk_ld_st_init(&src_ld_st, src, src_x, src_y, width, height);

   int32_t begin_i, begin_j;
   get_optimal_begin(&begin_i, &begin_j, NULL, NULL,
      dst_ld_st.sub_x, dst_ld_st.sub_y, dst_ld_st.format,
      src_ld_st.sub_x, src_ld_st.sub_y, src_ld_st.format,
      0, 0);

   vclib_obtain_VRF(1);

   ppulm_ld_lut32(0, lut);

   uint32_t vr = 0 << 6;
   int32_t i = begin_i, j = begin_j;
   uint32_t ldn = blk_ldp(vr, &src_ld_st, i, j);
   do {
      uint32_t next_vr = (vr + (16 << 6)) & 0xfff;

      int32_t next_i = i + 16;
      int32_t next_j = j;
      if (next_i >= (int32_t)width) {
         next_i = begin_i;
         next_j += 16;
      }

      uint32_t next_ldn = 0;
      if (next_j < (int32_t)height) {
         next_ldn = blk_ldp(next_vr, &src_ld_st, next_i, next_j);
      }

      bool tf = blk_ldp_unpack(vr, blk_prefer_tf(dst_ld_st.format), ldn, src_ld_st.format);
      khrn_bf_blk_unpack(vr, src_ld_st.format, conv);
      if (conv == IMAGE_CONV_VG) {
         blk_convert_pre_lin(vr, (vr - (16 << 6)) & 0xfff, format, src_ld_st.format);
      }
      _vasm("v32memread  H32(0++, 0) + %r, -, H8(0++, 0) + %r REP 16", vr, vr + (source_channel * 16));
      if ((conv == IMAGE_CONV_VG) && (output_format & IMAGE_FORMAT_PRE)) {
         khrn_bf_blk_clamp_to_a(vr);
      }
      blk_stap_channel_masked(vr, (vr - (16 << 6)) & 0xfff, tf, &dst_ld_st, i, j, channel_mask, output_format, conv);

      vr = next_vr;
      i = next_i;
      j = next_j;
      ldn = next_ldn;
   } while (j < (int32_t)height);

   vclib_release_VRF();
}

static MEM_HANDLE_T sconv_temp_handle;

void khrn_bf_sconv_init(void)
{
   sconv_temp_handle = MEM_INVALID_HANDLE;
}

void khrn_bf_sconv_term(void)
{
   if (sconv_temp_handle != MEM_INVALID_HANDLE) {
      mem_release(sconv_temp_handle);
   }
}

bool khrn_bf_sconv_prepare(
   int16_t *kernel_y, int32_t *interm_bias, int32_t *interm_shift,
   int16_t *kernel_x, int64_t *bias, int32_t *shift,
   const int16_t *kernel_x_in, uint32_t kernel_width,
   const int16_t *kernel_y_in, uint32_t kernel_height,
   float scale, float bias_in)
{
   uint32_t i;

   int32_t sum_pos_y = 0, sum_neg_y = 0, max_abs_y = 0,
      sum_x = 0, min_x = 0x7fffffff, max_x = -0x80000000, max_abs_x;
   for (i = 0; i != kernel_height; ++i) {
      if (kernel_y_in[i] < 0) {
         sum_neg_y += kernel_y_in[i];
         max_abs_y = _max(max_abs_y, -kernel_y_in[i]);
      } else {
         sum_pos_y += kernel_y_in[i];
         max_abs_y = _max(max_abs_y, kernel_y_in[i]);
      }
   }
   for (i = 0; i != kernel_width; ++i) {
      sum_x += kernel_x_in[i];
      min_x = _min(min_x, kernel_x_in[i]);
      max_x = _max(max_x, kernel_x_in[i]);
   }
   max_abs_x = _max(-min_x, max_x);

   /* scale the kernels so (max intermediate - min intermediate) is 1. if the y
    * kernel is all negative, negate both kernels. this simplifies the stuff
    * below... */
   float scale_x = (float)((sum_pos_y == 0) ? sum_neg_y : (sum_pos_y - sum_neg_y));

   /* out = sum_i((interm + interm_bias) * kernel_x_in[i] * scale_x * scale) + (bias + bias_correction)
    *
    * with interm_bias being such that (interm + interm_bias) is always between
    * 0 and 1, and bias_correction being exactly what is required to undo the
    * effects of interm_bias
    *
    * interm_bias and hence bias_correction are both 0 if either
    * (sum_pos_y == 0) or (sum_neg_y == 0). assuming this is the case, we have:
    *
    * out = sum_i(interm * kernel_x_in[i] * scale_x * scale) + bias
    *
    * if kernel_x_in[i] * scale_x * scale is always positive (which implies
    * kernel_x_in[i] >= u > 0 for all i or kernel_x_in[i] <= u < 0 for all i),
    * then any intermediate >= ((1 - bias) / (u * scale_x * scale)) will result
    * in maximal output, so we can rescale the intermediates:
    *
    * 1 = (1 - bias) / (u * scale_x * scale)
    * scale_x = (1 - bias) / (u * scale)
    *
    * similarly, if kernel_x_in[i] * scale_x * scale is always negative, then we
    * can rescale so:
    *
    * scale_x = -bias / (u * scale) */
   if (((sum_pos_y == 0) || (sum_neg_y == 0)) && ((min_x > 0) || (max_x < 0)) && (scale != 0.0f)) {
      float l = ((max_x < 0) ^ (scale_x < 0.0f) ^ (scale < 0.0f)) ? 0.0f : 1.0f;
      int32_t u = (max_x < 0) ? max_x : min_x;
      scale_x = clampf((l - bias_in) * recip_(u * scale),
         _minf(0.0f, scale_x), _maxf(0.0f, scale_x));
   }

   /* if we scale the intermediates by too much in the final calculation, we
    * could get significant errors with an 8-bit intermediate. use a 16-bit
    * intermediate instead */
   bool interm_16 = absf_(max_abs_x * scale_x * scale) > 2.0f;
   if (interm_16) {
      scale_x *= 1.0f / 256.0f;
   }

   float scale_y = (scale_x == 0.0f) ? 0.0f : recip_(scale_x);

   /* this is a hack for the cts. if the scaling is getting to be too much even
    * for a 16-bit intermediate, we make the largest y kernel value a power of 2
    * in an attempt to eliminate error in the intermediates */
   if (absf_(max_abs_x * scale_x * scale) > 0.25f) {
      scale_y = float_from_bits(float_to_bits(max_abs_y * scale_y) & ~0x7fffff) *
         recip_((float)max_abs_y);
      scale_x = (scale_y == 0.0f) ? 0.0f : recip_(scale_y);
   }

   /* set the intermediate bias so the intermediate values will be between 0 and 1 */
   float interm_bias_float = -((scale_y < 0.0f) ? sum_pos_y : sum_neg_y) * scale_y;
   if (interm_16) {
      /* 16-bit intermediate is actually signed (easier as saturation to H16
       * is signed), so shift down */
      interm_bias_float -= 128.0f;
   }
   interm_bias_float = floorf(interm_bias_float * 255.0f); /* get rid of the fractional part -- we can't perfectly undo that with bias_correction */

   /* want kernel to fit in signed 16-bit, but also need shift to be in
    * [-32, 31] (float_to_int_shift requires this). result/bias will fit in ~29
    * bits with a signed 16-bit kernel */
   *interm_shift = clampi(get_optimal_shift(max_abs_y * scale_y, 16), -32, 31);

   bias_in *= 255.0f;
   /* we want to get maximum precision (ie maximum shift) while satisfying these
    * requirements: */
   *shift = clampi(_min(
      /* kernel should fit in signed 16-bit */
      get_optimal_shift(max_abs_x * scale_x * scale, 16),
      /* bias and result should fit in signed (interm_16 ? 48-bit : 32-bit)
       * if the kernel fits in signed 16-bit, then the result before applying
       * the bias should fit in (interm_16 ? ~37 : ~29) bits, so if we instead
       * make sure the bias fits in signed (interm_16 ? 47-bit : 31-bit), the
       * result won't overflow */
      get_optimal_shift(bias_in - (sum_x * scale_x * scale * interm_bias_float), interm_16 ? 47 : 31)),
      /* shift must be in [-16, 31] (float_to_int48_shift requires this) */
      -16, 31);

   for (i = 0; i != kernel_height; ++i) {
      kernel_y[i] = clampi(float_to_int_shift(kernel_y_in[kernel_height - 1 - i] * scale_y,
         *interm_shift), -0x8000, 0x7fff);
   }
   if (*interm_shift > 0) {
      interm_bias_float += 0.49999997f;
   }
   *interm_bias = float_to_int_shift(interm_bias_float, *interm_shift);
   int32_t interm_bias_shifted = shift_int(*interm_bias, *interm_shift);
   int32_t sum_x_scaled = 0;
   for (i = 0; i != kernel_width; ++i) {
      sum_x_scaled += (kernel_x[i] = clampi(float_to_int_shift(
         kernel_x_in[kernel_width - 1 - i] * scale_x * scale, *shift), -0x8000, 0x7fff));
   }
   if (*shift > 0) {
      bias_in += 0.49999997f;
   }
   *bias = float_to_int48_shift(bias_in, *shift) - ((int64_t)sum_x_scaled * (int64_t)interm_bias_shifted);
   if (interm_16) { /* clamp to 48 bits (bias correction could have put it just out */
      if      (*bias >  0x7fffffffffffll) { *bias =  0x7fffffffffffll; }
      else if (*bias < -0x800000000000ll) { *bias = -0x800000000000ll; }
   } else { /* clamp to 32 bits */
      if      (*bias >  0x7fffffffll) { *bias =  0x7fffffffll; }
      else if (*bias < -0x80000000ll) { *bias = -0x80000000ll; }
   }

   return interm_16;
}

/* todo: don't overdo if src size >> dst size */

bool khrn_bf_sconv(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y, uint32_t dst_width, uint32_t dst_height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y, uint32_t src_width, uint32_t src_height,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format, KHRN_IMAGE_CONV_T conv,
   KHRN_BF_TILE_T tile, uint32_t tile_fill_rgba,
   int32_t shift_x, int32_t shift_y,
   bool interm_16,
   const int16_t *kernel_y, uint32_t kernel_height, int32_t interm_bias, int32_t interm_shift,
   const int16_t *kernel_x, uint32_t kernel_width, int64_t bias, int32_t shift)
{
   vcos_assert(khrn_image_is_color(dst->format) && khrn_image_is_color(src->format));
   vcos_assert((dst_x + dst_width) <= dst->width);
   vcos_assert((dst_y + dst_height) <= dst->height);
   vcos_assert((src_x + src_width) <= src->width);
   vcos_assert((src_y + src_height) <= src->height);
   vcos_assert((kernel_width >= 1) && (kernel_width <= 98));
   vcos_assert((kernel_height >= 1) && (kernel_height <= 98));

   if ((dst_width == 0) || (dst_height == 0)) {
      return true;
   }
   vcos_assert((src_width != 0) && (src_height != 0));

   BLK_LD_ST_T dst_ld_st, src_ld_st;
   blk_ld_st_init(&dst_ld_st, dst, dst_x, dst_y, dst_width, dst_height);
   blk_ld_st_init(&src_ld_st, src, src_x, src_y, src_width, src_height);

   int32_t begin_i, begin_j;
   uint32_t offset_x, offset_y;
   get_optimal_begin(&begin_i, &begin_j, &offset_x, &offset_y,
      dst_ld_st.sub_x, dst_ld_st.sub_y, dst_ld_st.format,
      src_ld_st.sub_x - shift_x, src_ld_st.sub_y - shift_y, src_ld_st.format,
      (kernel_width <= 33) ? (33 - kernel_width) : 15, (kernel_height <= 33) ? (33 - kernel_height) : 15);
   shift_x += offset_x;
   shift_y += offset_y;

   /* todo: use dst as temp if possible */
   /* todo: rework interm_16 to reduce temp memory requirements? */
   BLK_LD_ST_T temp_ld_st[2];
   temp_ld_st[0].format = (KHRN_IMAGE_FORMAT_T)(IMAGE_FORMAT_RSOTF | IMAGE_FORMAT_UNCOMP | IMAGE_FORMAT_32);
   temp_ld_st[0].sub_x = (uint16_t)((shift_x - begin_i) & 0xf);
   temp_ld_st[0].sub_y = (uint16_t)-begin_j;
   temp_ld_st[0].sub_width = src_ld_st.sub_width;
   temp_ld_st[0].sub_height = dst_ld_st.sub_height;
   temp_ld_st[0].width = (uint16_t)round_up(temp_ld_st[0].sub_x + temp_ld_st[0].sub_width, 32);
   temp_ld_st[0].height = (uint16_t)round_up(temp_ld_st[0].sub_y + temp_ld_st[0].sub_height, 32);
   temp_ld_st[0].stride = temp_ld_st[0].width * 4;
   uint32_t single_temp_size = temp_ld_st[0].height * temp_ld_st[0].stride;
   uint32_t temp_size = (interm_16 ? 2 : 1) * single_temp_size;
   if ((sconv_temp_handle == MEM_INVALID_HANDLE) &&
      ((sconv_temp_handle = mem_alloc_ex(0, 64, (MEM_FLAG_T)(MEM_FLAG_DISCARDABLE | MEM_FLAG_DIRECT | MEM_FLAG_RESIZEABLE | MEM_FLAG_NO_INIT), "khrn_bf_sconv", MEM_COMPACT_DISCARD)) == MEM_INVALID_HANDLE)) {
      return false;
   }
   mem_retain(sconv_temp_handle);
   if ((mem_get_size(sconv_temp_handle) < temp_size) && !mem_resize_ex(sconv_temp_handle, temp_size, MEM_COMPACT_DISCARD)) {
      mem_unretain(sconv_temp_handle);
      return false;
   }
   temp_ld_st[0].data = mem_lock(sconv_temp_handle);
   mem_unretain(sconv_temp_handle);
   if (interm_16) {
      temp_ld_st[1] = temp_ld_st[0];
      temp_ld_st[1].data = (uint8_t *)temp_ld_st[0].data + single_temp_size;
   }

   vclib_obtain_VRF(1);

   tile = (KHRN_BF_TILE_T)(tile | BF_TILE_Y | BF_TILE_UNPACK_AND_CONVERT);

   if (kernel_height <= 33) {
#ifdef __VIDEOCORE4__
      /* use 0x1000 replicating alias for kernel/bias/shift */
      _vasm("v16mov  V16(0, 0), %r", kernel_y[32]);
      _vasm("v16mov  V16(0, 1), %r", -interm_shift);
      _vasm("v32mov  V32(0, 2), %r", interm_bias);
      _vasm("v16ld  H16(1, 0), (%r)", kernel_y);
      _vasm("v16ld  H16(1, 32), (%r)", kernel_y + 16);
      _vasm("v32memwrite  -, H32(0, 0), 0");
      _vasm("v32memwrite  -, H32(1, 0), 1");
#else
      /* use uniforms memory for kernel/bias/shift */
      um_ld(0, kernel_y[32]);
      um_ld(64, -interm_shift);
      um_ld(128, interm_bias);
      um_ld_16s(1, 64, kernel_y, 16, kernel_y + 16, 16);
      khrn_bf_um_stall();
#endif

      if (interm_16) {
         /* todo: could do better in some special (but probably common) cases,
          * eg when offset_y >= 3. probably not much better though? */

         int32_t i, j;
         for (i = -temp_ld_st[0].sub_x; i < (int32_t)temp_ld_st[0].sub_width; i += 16) {
            uint32_t ldn1 = blk_ldpt(0 << 6, &src_ld_st, i, -temp_ld_st[0].sub_y - shift_y, tile);
            uint32_t ldn2 = blk_ldpt(16 << 6, &src_ld_st, i, -temp_ld_st[0].sub_y - shift_y + 16, tile);
            blk_ldpt_unpack(0 << 6, 48 << 6, ldn1, &src_ld_st, tile, tile_fill_rgba, format, conv);
            blk_ldpt_unpack(16 << 6, 48 << 6, ldn2, &src_ld_st, tile, tile_fill_rgba, format, conv);

            for (j = -temp_ld_st[0].sub_y; j < (int32_t)temp_ld_st[0].sub_height; j += 16) {
               ldn1 = blk_ldpt(32 << 6, &src_ld_st, i, j - shift_y + 32, tile);
               blk_ldpt_unpack(32 << 6, 48 << 6, ldn1, &src_ld_st, tile, tile_fill_rgba, format, conv);

               /* red/green */
#ifdef __VIDEOCORE4__
               _vasm("v32memread  H32(51, 0), -, 0");
               _vasm("v32memread  H32(52, 0), -, 1");
               #define KERNEL (0x1000 + (51 << 6))
#else
               #define KERNEL 0x1000
#endif
               khrn_bf_blk_sconv_8_16(48 << 6, 3, offset_y << 6, 53 << 6, KERNEL, kernel_height);
               khrn_bf_blk_sconv_8_16(32 + (48 << 6), 3, 16 + (offset_y << 6), 53 << 6, KERNEL, kernel_height);
#ifdef __VIDEOCORE4__
               _vasm("v32memwrite  -, H32(0, 0), 2");
               _vasm("v32memwrite  -, H32(1, 0), 3");
               _vasm("v32mov  H32(0++, 0), H32(51++, 0) REP 2");
               #undef KERNEL
               #define KERNEL 0x1000
#endif
               _vasm("v32memwrite  -, H32(2, 0), 4");
               khrn_bf_blk_sconv_8_16(51 << 6, 13, (offset_y + 3) << 6, 2 << 6, KERNEL, kernel_height);
               khrn_bf_blk_sconv_8_16(32 + (51 << 6), 13, 16 + ((offset_y + 3) << 6), 2 << 6, KERNEL, kernel_height);
               blk_stap(48 << 6, false, temp_ld_st, i, j);

               /* blue/alpha */
#ifdef __VIDEOCORE4__
               _vasm("v32mov  H32(51++, 0), H32(0++, 0) REP 2");
               _vasm("v32memread  H32(0, 0), -, 2");
               _vasm("v32memread  H32(1, 0), -, 3");
               #undef KERNEL
               #define KERNEL (0x1000 + (51 << 6))
#endif
               _vasm("v32memread  H32(2, 0), -, 4");
               khrn_bf_blk_sconv_8_16(48 << 6, 3, 32 + (offset_y << 6), 53 << 6, KERNEL, kernel_height);
               khrn_bf_blk_sconv_8_16(32 + (48 << 6), 3, 48 + (offset_y << 6), 53 << 6, KERNEL, kernel_height);
#ifdef __VIDEOCORE4__
               _vasm("v32mov  H32(0++, 0), H32(51++, 0) REP 2");
               #undef KERNEL
               #define KERNEL 0x1000
#endif
               khrn_bf_blk_sconv_8_16(51 << 6, 13, 32 + ((offset_y + 3) << 6), 2 << 6, KERNEL, kernel_height);
               khrn_bf_blk_sconv_8_16(32 + (51 << 6), 13, 48 + ((offset_y + 3) << 6), 2 << 6, KERNEL, kernel_height);
               #undef KERNEL
               blk_stap(48 << 6, false, temp_ld_st + 1, i, j);

               _vasm("v32mov  H32(0++, 0), H32(16++, 0) REP 32");
            }
         }
      } else {
         int32_t i, j;
         for (i = -temp_ld_st[0].sub_x; i < (int32_t)temp_ld_st[0].sub_width; i += 16) {
#ifdef __VIDEOCORE4__
            #define R 3
            #define RS "3"
#else
            #define R 1
            #define RS "1"
#endif

            uint32_t ldn1 = blk_ldpt(0 << 6, &src_ld_st, i, -temp_ld_st[0].sub_y - shift_y, tile);
            uint32_t ldn2 = blk_ldpt(16 << 6, &src_ld_st, i, -temp_ld_st[0].sub_y - shift_y + 16, tile);
            uint32_t ldn = blk_ldpt(((offset_y >= R) ? 48 : 32) << 6, &src_ld_st, i, -temp_ld_st[0].sub_y - shift_y + 32, tile);
            blk_ldpt_unpack(0 << 6, ((offset_y >= R) ? 32 : 48) << 6, ldn1, &src_ld_st, tile, tile_fill_rgba, format, conv);
            blk_ldpt_unpack(16 << 6, ((offset_y >= R) ? 32 : 48) << 6, ldn2, &src_ld_st, tile, tile_fill_rgba, format, conv);

            if (offset_y >= R) {
#ifdef __VIDEOCORE4__
               _vasm("v32memread  H32(0, 0), -, 0");
               _vasm("v32memread  H32(1, 0), -, 1");
#endif
               for (j = -temp_ld_st[0].sub_y; j < (int32_t)temp_ld_st[0].sub_height; j += 16) {
                  blk_ldpt_unpack(48 << 6, 32 << 6, ldn, &src_ld_st, tile, tile_fill_rgba, format, conv);
                  _vasm("v32mov  H32(32++, 0), H32(48++, 0) REP 16");
                  if ((j + 16) < (int32_t)temp_ld_st[0].sub_height) {
                     ldn = blk_ldpt(48 << 6, &src_ld_st, i, j - shift_y + 48, tile);
                  }
                  khrn_bf_blk_sconv_8_8(R << 6, 16, offset_y << 6, (R - 1) << 6, 0x1000, kernel_height);
                  khrn_bf_blk_sconv_8_8(16 + (R << 6), 16, 16 + (offset_y << 6), (R - 1) << 6, 0x1000, kernel_height);
                  khrn_bf_blk_sconv_8_8(32 + (R << 6), 16, 32 + (offset_y << 6), (R - 1) << 6, 0x1000, kernel_height);
                  khrn_bf_blk_sconv_8_8(48 + (R << 6), 16, 48 + (offset_y << 6), (R - 1) << 6, 0x1000, kernel_height);
                  blk_stap(R << 6, false, temp_ld_st, i, j);
                  _vasm("v32mov  H32("RS"++, 0), H32((16 + "RS")++, 0) REP %r", 32 - R);
               }
            } else {
               /* todo: is the lazy load worth all the ppulm fiddling? */
               for (j = -temp_ld_st[0].sub_y; j < (int32_t)temp_ld_st[0].sub_height; j += 16) {
                  blk_ldpt_unpack(32 << 6, 48 << 6, ldn, &src_ld_st, tile, tile_fill_rgba, format, conv);
                  if ((j + 16) < (int32_t)temp_ld_st[0].sub_height) {
                     ldn = blk_ldpt(48 << 6, &src_ld_st, i, j - shift_y + 48, tile);
                  }
#ifdef __VIDEOCORE4__
                  _vasm("v32memwrite  -, H32(45, 0), 2");
                  _vasm("v32memwrite  -, H32(46, 0), 3");
                  _vasm("v32memread  H32(45, 0), -, 0");
                  _vasm("v32memread  H32(46, 0), -, 1");
                  #define KERNEL (0x1000 + (45 << 6))
#else
                  #define KERNEL 0x1000
#endif
                  _vasm("v32memwrite  -, H32(47, 0), 4");
                  khrn_bf_blk_sconv_8_8(0, 16 - R, offset_y << 6, 47 << 6, KERNEL, kernel_height);
                  khrn_bf_blk_sconv_8_8(16, 16 - R, 16 + (offset_y << 6), 47 << 6, KERNEL, kernel_height);
                  khrn_bf_blk_sconv_8_8(32, 16 - R, 32 + (offset_y << 6), 47 << 6, KERNEL, kernel_height);
                  khrn_bf_blk_sconv_8_8(48, 16 - R, 48 + (offset_y << 6), 47 << 6, KERNEL, kernel_height);
#ifdef __VIDEOCORE4__
                  _vasm("v32memread  H32(45, 0), -, 2");
                  _vasm("v32memread  H32(46, 0), -, 3");
                  _vasm("v32memwrite  -, H32(48, 0), 2");
                  _vasm("v32memwrite  -, H32(49, 0), 3");
                  _vasm("v32memread  H32(48, 0), -, 0");
                  _vasm("v32memread  H32(49, 0), -, 1");
                  #undef KERNEL
                  #define KERNEL (0x1000 + (48 << 6))
#endif
                  _vasm("v32memread  H32(47, 0), -, 4");
                  _vasm("v32memwrite  -, H32(50, 0), 4");
                  khrn_bf_blk_sconv_8_8((16 - R) << 6, R, (offset_y + (16 - R)) << 6, 50 << 6, KERNEL, kernel_height);
                  khrn_bf_blk_sconv_8_8(16 + ((16 - R) << 6), R, 16 + ((offset_y + (16 - R)) << 6), 50 << 6, KERNEL, kernel_height);
                  khrn_bf_blk_sconv_8_8(32 + ((16 - R) << 6), R, 32 + ((offset_y + (16 - R)) << 6), 50 << 6, KERNEL, kernel_height);
                  khrn_bf_blk_sconv_8_8(48 + ((16 - R) << 6), R, 48 + ((offset_y + (16 - R)) << 6), 50 << 6, KERNEL, kernel_height);
                  #undef KERNEL
                  blk_stap(0 << 6, false, temp_ld_st, i, j);
                  _vasm("v32mov  H32(0++, 0), H32(16++, 0) REP 32");
#ifdef __VIDEOCORE4__
                  _vasm("v32memread  H32(32, 0), -, 2");
                  _vasm("v32memread  H32(33, 0), -, 3");
#else
                  _vasm("v32mov  H32(32++, 0), H32(48++, 0) REP 2");
#endif
                  _vasm("v32memread  H32(34, 0), -, 4");
                  _vasm("v32mov  H32(35++, 0), H32(51++, 0) REP %r", 13);
               }
            }

            #undef RS
            #undef R
         }
      }
   } else {
      vcos_assert(!interm_16);

#ifdef __VIDEOCORE4__
      /* use 0x1000 replicating alias for kernel */
      _vasm("v16mov  H16(59, 0), %r", kernel_y[32]);
      _vasm("v16ld  H16(60, 0), (%r)", kernel_y);
      _vasm("v16ld  H16(60, 32), (%r)", kernel_y + 16);
      {
         uint32_t i, vr;
         for (i = 33 - offset_y, vr = 61 << 6; i < kernel_height; i += 16, vr += 32) {
            _vasm("v16bitplanes  -, %r SETF", (1 << _min(kernel_height - i, 16)) - 1);
            _vasm("v16ld  H16(0, 0) + %r, (%r) IFNZ", vr, kernel_y + i);
         }
      }
      #define KERNEL (0x1000 + (59 << 6))
#else
      /* use uniforms memory for kernel */
      um_ld(0, kernel_y[32]);
      um_ld_16s(1, 64, kernel_y, 16, kernel_y + 16, 16);
      {
         uint32_t i, offset;
         for (i = 33 - offset_y, offset = 2; i < kernel_height; i += 32, ++offset) {
            um_ld_16s(offset, 64,
               kernel_y + i, _min(kernel_height - i, 16),
               kernel_y + i + 16, clampi(kernel_height - (i + 16), 0, 16));
         }
      }
      khrn_bf_um_stall();
      #define KERNEL 0x1000
#endif

      uint32_t n = offset_y + kernel_height + 15;

      int32_t i, j, k, vr_channel, vr_kernel;
      for (i = -temp_ld_st[0].sub_x; i < (int32_t)temp_ld_st[0].sub_width; i += 16) {
         for (k = 0; (k + 16) < n;
#ifdef __VIDEOCORE4__
            k += 32) {
#else
            k += 48) {
#endif
            uint32_t ldn1 = blk_ldpt(0 << 6, &src_ld_st, i, -temp_ld_st[0].sub_y - shift_y + k, tile);
            uint32_t ldn2 = 0;
            if ((k + 32) < n) {
               ldn2 = blk_ldpt(16 << 6, &src_ld_st, i, -temp_ld_st[0].sub_y - shift_y + k + 16, tile);
            }
#ifndef __VIDEOCORE4__
            uint32_t ldn3 = 0;
            if ((k + 48) < n) {
               ldn3 = blk_ldpt(48 << 6, &src_ld_st, i, -temp_ld_st[0].sub_y - shift_y + k + 32, tile);
            }
#endif
            blk_ldpt_unpack(0 << 6, 32 << 6, ldn1, &src_ld_st, tile, tile_fill_rgba, format, conv);
            khrn_bf_blk_st_ppulm(0 << 6, -temp_ld_st[0].sub_y + k);
            if ((k + 32) < n) {
               blk_ldpt_unpack(16 << 6, 32 << 6, ldn2, &src_ld_st, tile, tile_fill_rgba, format, conv);
               khrn_bf_blk_st_ppulm(16 << 6, -temp_ld_st[0].sub_y + k + 16);
            }
#ifndef __VIDEOCORE4__
            if ((k + 48) < n) {
               blk_ldpt_unpack(48 << 6, 32 << 6, ldn3, &src_ld_st, tile, tile_fill_rgba, format, conv);
               khrn_bf_blk_st_ppulm(48 << 6, -temp_ld_st[0].sub_y + k + 32);
            }
#endif
         }
         uint32_t ldn = blk_ldpt(16 << 6, &src_ld_st, i, -temp_ld_st[0].sub_y - shift_y + ((n - 1) & ~0xf), tile);

         for (j = -temp_ld_st[0].sub_y; j < (int32_t)temp_ld_st[0].sub_height; j += 16) {
            blk_ldpt_unpack(16 << 6, 0 << 6, ldn, &src_ld_st, tile, tile_fill_rgba, format, conv);
            khrn_bf_blk_st_ppulm(16 << 6, j + ((n - 1) & ~0xf));

            khrn_bf_blk_ld_ppulm(0 << 6, j);
            khrn_bf_blk_ld_ppulm(16 << 6, j + 16);
            khrn_bf_blk_ld_ppulm(32 << 6, j + 32);
            for (vr_channel = 0; vr_channel != 64; vr_channel += 16) {
               #define X(I, J) _vasm("v32memwrite  -, H32(48 + "#I", 0), %r", j + J + I - (16 + vr_channel))
#ifdef __VIDEOCORE4__
               khrn_bf_blk_sconv_8_32(48 << 6, 8, vr_channel + (offset_y << 6), KERNEL, 33 - offset_y);
               X(0, 0); X(1, 0); X(2, 0); X(3, 0); X(4, 0); X(5, 0); X(6, 0); X(7, 0);
               khrn_bf_blk_sconv_8_32(48 << 6, 8, vr_channel + ((offset_y + 8) << 6), KERNEL, 33 - offset_y);
               X(0, 8); X(1, 8); X(2, 8); X(3, 8); X(4, 8); X(5, 8); X(6, 8); X(7, 8);
#else
               khrn_bf_blk_sconv_8_32(48 << 6, 16, vr_channel + (offset_y << 6), KERNEL, 33 - offset_y);
               X(0, 0); X(1, 0); X(2, 0); X(3, 0); X(4, 0); X(5, 0); X(6, 0); X(7, 0);
               X(8, 0); X(9, 0); X(10, 0); X(11, 0); X(12, 0); X(13, 0); X(14, 0); X(15, 0);
#endif
               #undef X
            }

            for (k = 48, vr_kernel = KERNEL + (1 << 6);; k += 32, vr_kernel += (1 << 6)) {
               _vasm("v32mov  H32(0++, 0), H32(32++, 0) REP 16");
               khrn_bf_blk_ld_ppulm(16 << 6, j + k);
               if ((k + 16) < n) {
                  khrn_bf_blk_ld_ppulm(32 << 6, j + k + 16);
               }
               if ((k + 32) >= n) { break; }
               for (vr_channel = 0; vr_channel != 64; vr_channel += 16) {
                  #define X(I, J) \
                     do { \
                        _vasm("v32mov  -, H32(48 + "#I", 0) CLRA SACC"); \
                        _vasm("v32memread  H32(48 + "#I", 0), -, %r SACC", j + J + I - (16 + vr_channel)); \
                        _vasm("v32memwrite  -, H32(48 + "#I", 0), %r", j + J + I - (16 + vr_channel)); \
                     } while (0)
#ifdef __VIDEOCORE4__
                  khrn_bf_blk_sconv_8_32(48 << 6, 8, vr_channel + (1 << 6), vr_kernel, 32);
                  X(0, 0); X(1, 0); X(2, 0); X(3, 0); X(4, 0); X(5, 0); X(6, 0); X(7, 0);
                  khrn_bf_blk_sconv_8_32(48 << 6, 8, vr_channel + (9 << 6), vr_kernel, 32);
                  X(0, 8); X(1, 8); X(2, 8); X(3, 8); X(4, 8); X(5, 8); X(6, 8); X(7, 8);
#else
                  khrn_bf_blk_sconv_8_32(48 << 6, 16, vr_channel + (1 << 6), vr_kernel, 32);
                  X(0, 0); X(1, 0); X(2, 0); X(3, 0); X(4, 0); X(5, 0); X(6, 0); X(7, 0);
                  X(8, 0); X(9, 0); X(10, 0); X(11, 0); X(12, 0); X(13, 0); X(14, 0); X(15, 0);
#endif
                  #undef X
               }
            }

            for (vr_channel = 0; vr_channel != 64; vr_channel += 16) {
               #define X(I, J) \
                  do { \
                     _vasm("v32adds  -, H32(48 + "#I", 0), %r CLRA SACC", interm_bias); \
                     _vasm("v32memread  H32(48 + "#I", 0), -, %r SACC", j + J + I - (16 + vr_channel)); \
                  } while (0)
#ifdef __VIDEOCORE4__
               khrn_bf_blk_sconv_8_32(48 << 6, 8, vr_channel + (1 << 6), vr_kernel, n - k);
               X(0, 0); X(1, 0); X(2, 0); X(3, 0); X(4, 0); X(5, 0); X(6, 0); X(7, 0);
               _vasm("v32signasls  H8(0++, 0) + %r, H32(48++, 0), %r REP 8", vr_channel, -interm_shift);
               khrn_bf_blk_sconv_8_32(48 << 6, 8, vr_channel + (9 << 6), vr_kernel, n - k);
               X(0, 8); X(1, 8); X(2, 8); X(3, 8); X(4, 8); X(5, 8); X(6, 8); X(7, 8);
               _vasm("v32signasls  H8(8++, 0) + %r, H32(48++, 0), %r REP 8", vr_channel, -interm_shift);
#else
               khrn_bf_blk_sconv_8_32(48 << 6, 16, vr_channel + (1 << 6), vr_kernel, n - k);
               X(0, 0); X(1, 0); X(2, 0); X(3, 0); X(4, 0); X(5, 0); X(6, 0); X(7, 0);
               X(8, 0); X(9, 0); X(10, 0); X(11, 0); X(12, 0); X(13, 0); X(14, 0); X(15, 0);
               _vasm("v32signasls  H8(0++, 0) + %r, H32(48++, 0), %r REP 16", vr_channel, -interm_shift);
#endif
               #undef X
            }

            if ((j + 16) < (int32_t)temp_ld_st[0].sub_height) {
               ldn = blk_ldpt(16 << 6, &src_ld_st, i, j - shift_y + ((n + 15) & ~0xf), tile);
            }

            blk_stap(0 << 6, false, temp_ld_st, i, j);
         }
      }

      #undef KERNEL
   }

   tile = (KHRN_BF_TILE_T)((tile & BF_TILE_MASK) | BF_TILE_X);
   uint32_t tile_fill_16[2];
   if ((tile & BF_TILE_MASK) == BF_TILE_FILL) {
      int32_t sum_y = 0;
      uint32_t i;
      for (i = 0; i != kernel_height; ++i) {
         sum_y += kernel_y[i];
      }

      int32_t r = shift_int(_adds(sum_y * (tile_fill_rgba & 0xff), interm_bias), interm_shift);
      int32_t g = shift_int(_adds(sum_y * ((tile_fill_rgba >> 8) & 0xff), interm_bias), interm_shift);
      int32_t b = shift_int(_adds(sum_y * ((tile_fill_rgba >> 16) & 0xff), interm_bias), interm_shift);
      int32_t a = shift_int(_adds(sum_y * (tile_fill_rgba >> 24), interm_bias), interm_shift);

      if (interm_16) {
         tile_fill_16[0] = (clampi(r, -0x8000, 0x7fff) & 0xffff) |
                           (clampi(g, -0x8000, 0x7fff) << 16);
         tile_fill_16[1] = (clampi(b, -0x8000, 0x7fff) & 0xffff) |
                           (clampi(a, -0x8000, 0x7fff) << 16);
      } else {
         tile_fill_rgba = clampi(r, 0, 0xff) |
                          (clampi(g, 0, 0xff) << 8) |
                          (clampi(b, 0, 0xff) << 16) |
                          (clampi(a, 0, 0xff) << 24);
      }
   }

   if (kernel_width <= 33) {
#ifdef __VIDEOCORE4__
      /* use 0x1000 replicating alias for kernel/bias/shift */
      _vasm("v16mov  V16(48, 0), %r", kernel_x[32]);
      _vasm("v16mov  V16(48, 1), %r", interm_16 ? _max(-shift, 0) : -shift);
      _vasm("v32mov  V32(48, 2), %r", (int32_t)(interm_16 ? (bias >> 16) : bias));
      if (interm_16) {
         _vasm("v16mov  V16(48, 3), %r", _max(shift, 0));
         _vasm("v16mov  V16(48, 4), %r", (uint32_t)bias);
      }
      _vasm("v16ld  H16(52, 0), (%r)", kernel_x);
      _vasm("v16ld  H16(52, 32), (%r)", kernel_x + 16);
#else
      /* use uniforms memory for kernel/bias/shift */
      um_ld(0, kernel_x[32]);
      um_ld(64, interm_16 ? _max(-shift, 0) : -shift);
      um_ld(128, (int32_t)(interm_16 ? (bias >> 16) : bias));
      if (interm_16) {
         um_ld(192, _max(shift, 0));
         um_ld(256, (uint32_t)bias);
      }
      um_ld_16s(1, 64, kernel_x, 16, kernel_x + 16, 16);
      khrn_bf_um_stall();
#endif

      if (interm_16) {
         /* todo: could do better in some special (but probably common) cases,
          * eg when offset_x >= 3. probably not much better though? */

         int32_t i, j;
         for (j = begin_j; j < (int32_t)dst_ld_st.sub_height; j += 16) {
            uint32_t t;
            for (t = 0; t != 2; ++t) {
               uint32_t ldn1 = blk_ldpt(16 << 6, temp_ld_st + t, begin_i - shift_x, j, tile);
               uint32_t ldn2 = blk_ldpt(32 << 6, temp_ld_st + t, begin_i - shift_x + 16, j, tile);
               blk_ldpt_unpack(16 << 6, 0 << 6, ldn1, temp_ld_st + t, tile, tile_fill_16[t], IMAGE_FORMAT_INVALID, (KHRN_IMAGE_CONV_T)0);
               _vasm("v32mov  V32(0, 0++), H32(16++, 0) REP 16");
               khrn_bf_blk_st_ppulm(0 << 6, begin_i + (t * 128));
               blk_ldpt_unpack(32 << 6, 0 << 6, ldn2, temp_ld_st + t, tile, tile_fill_16[t], IMAGE_FORMAT_INVALID, (KHRN_IMAGE_CONV_T)0);
               _vasm("v32mov  V32(0, 0++), H32(32++, 0) REP 16");
               khrn_bf_blk_st_ppulm(0 << 6, begin_i + (t * 128) + 16);
            }

            for (i = begin_i; i < (int32_t)dst_ld_st.sub_width; i += 16) {
               for (t = 0; t != 2; ++t) {
                  uint32_t ldn = blk_ldpt(16 << 6, temp_ld_st + t, i - shift_x + 32, j, tile);
                  khrn_bf_blk_ld_ppulm(0 << 6, i + (t * 128));
                  blk_ldpt_unpack(16 << 6, 32 << 6, ldn, temp_ld_st + t, tile, tile_fill_16[t], IMAGE_FORMAT_INVALID, (KHRN_IMAGE_CONV_T)0);
                  _vasm("v32mov  V32(32, 0++), H32(16++, 0) REP 16");
                  khrn_bf_blk_ld_ppulm(16 << 6, i + (t * 128) + 16);
#ifdef __VIDEOCORE4__
                  #define KERNEL (0x1000 + (51 << 6))
#else
                  #define KERNEL 0x1000
#endif
                  khrn_bf_blk_sconv_16_8((t * 32) + (48 << 6), 3, offset_x << 6, 53 << 6, KERNEL, kernel_width);
                  khrn_bf_blk_sconv_16_8((t * 32) + 16 + (48 << 6), 3, 32 + (offset_x << 6), 53 << 6, KERNEL, kernel_width);
#ifdef __VIDEOCORE4__
                  _vasm("v32mov  H32(0++, 0), H32(51++, 0) REP 2");
                  #undef KERNEL
                  #define KERNEL 0x1000
#endif
                  khrn_bf_blk_sconv_16_8((t * 32) + (51 << 6), 13, (offset_x + 3) << 6, 2 << 6, KERNEL, kernel_width);
                  khrn_bf_blk_sconv_16_8((t * 32) + 16 + (51 << 6), 13, 32 + ((offset_x + 3) << 6), 2 << 6, KERNEL, kernel_width);
                  #undef KERNEL
                  khrn_bf_blk_st_ppulm(32 << 6, i + (t * 128) + 32);
#ifdef __VIDEOCORE4__
                  _vasm("v16memwrite  -, H16(51, 0) + %r, %r", t * 32, (i * 2) + t);
                  _vasm("v16memwrite  -, H16(52, 0) + %r, %r", t * 32, ((i + 1) * 2) + t);
                  _vasm("v32mov  H32(51++, 0), H32(0++, 0) REP 2");
#endif
                  _vasm("v16memwrite  -, H16(53, 0) + %r, %r", t * 32, ((i + 2) * 2) + t);
               }
#ifdef __VIDEOCORE4__
               _vasm("v32mov  V32(0, 0++), H32(48++, 0) REP %r", 3);
               _vasm("v32memread  V32(0, 3), -, %r", i);
               _vasm("v32memread  V32(0, 4), -, %r", i + 1);
#else
               _vasm("v32mov  V32(0, 0++), H32(48++, 0) REP %r", 5);
#endif
               _vasm("v32memread  V32(0, 5), -, %r", i + 2);
               _vasm("v32mov  V32(0, 6++), H32(54++, 0) REP 8");
               _vasm("v32mov  V32(0, 14++), H32(62++, 0) REP 2");
               if ((conv == IMAGE_CONV_VG) && (format & IMAGE_FORMAT_PRE)) {
                  khrn_bf_blk_clamp_to_a(0 << 6);
               }
               blk_stap_channel_masked(0 << 6, 16 << 6, false, &dst_ld_st, i, j, channel_mask, format, conv);
            }
         }
      } else {
#ifdef __VIDEOCORE4__
         _vasm("v32memwrite  -, H32(51, 0), 0");
         _vasm("v32memwrite  -, H32(52, 0), 1");
#endif

         int32_t i, j;
         for (j = begin_j; j < (int32_t)dst_ld_st.sub_height; j += 16) {
            uint32_t ldn1 = blk_ldpt(16 << 6, temp_ld_st, begin_i - shift_x, j, tile);
            uint32_t ldn2 = blk_ldpt(32 << 6, temp_ld_st, begin_i - shift_x + 16, j, tile);
            uint32_t ldn = blk_ldpt(48 << 6, temp_ld_st, begin_i - shift_x + 32, j, tile);
            blk_ldpt_unpack(16 << 6, 0 << 6, ldn1, temp_ld_st, tile, tile_fill_rgba, IMAGE_FORMAT_INVALID, (KHRN_IMAGE_CONV_T)0);
            _vasm("v32mov  V32(0, 0++), H32(16++, 0) REP 16");
            blk_ldpt_unpack(32 << 6, 16 << 6, ldn2, temp_ld_st, tile, tile_fill_rgba, IMAGE_FORMAT_INVALID, (KHRN_IMAGE_CONV_T)0);
            _vasm("v32mov  V32(16, 0++), H32(32++, 0) REP 16");

            for (i = begin_i; i < (int32_t)dst_ld_st.sub_width; i += 16) {
               blk_ldpt_unpack(48 << 6, 32 << 6, ldn, temp_ld_st, tile, tile_fill_rgba, IMAGE_FORMAT_INVALID, (KHRN_IMAGE_CONV_T)0);
               _vasm("v32mov  V32(32, 0++), H32(48++, 0) REP 16");
#ifdef __VIDEOCORE4__
               _vasm("v32memread  H32(48, 0), -, 0");
               _vasm("v32memread  H32(49, 0), -, 1");
               #define KERNEL (0x1000 + (48 << 6))
#else
               #define KERNEL 0x1000
#endif
               khrn_bf_blk_sconv_8_8(0, 16, offset_x << 6, 50 << 6, KERNEL, kernel_width);
               khrn_bf_blk_sconv_8_8(16, 16, 16 + (offset_x << 6), 50 << 6, KERNEL, kernel_width);
               khrn_bf_blk_sconv_8_8(32, 16, 32 + (offset_x << 6), 50 << 6, KERNEL, kernel_width);
               khrn_bf_blk_sconv_8_8(48, 16, 48 + (offset_x << 6), 50 << 6, KERNEL, kernel_width);
               #undef KERNEL
               _vasm("v32mov  V32(48, 0++), H32(0++, 0) REP 16");
               if ((conv == IMAGE_CONV_VG) && (format & IMAGE_FORMAT_PRE)) {
                  khrn_bf_blk_clamp_to_a(48 << 6);
               }
               blk_stap_channel_masked(48 << 6, 0 << 6, false, &dst_ld_st, i, j, channel_mask, format, conv);
               if ((i + 16) < (int32_t)dst_ld_st.sub_width) {
                  ldn = blk_ldpt(48 << 6, temp_ld_st, i - shift_x + 48, j, tile);
               }
               _vasm("v32mov  H32(0++, 0), H32(16++, 0) REP 32");
            }
         }
      }
   } else {
      vcos_assert(!interm_16);

#ifdef __VIDEOCORE4__
      /* use 0x1000 replicating alias for kernel */
      _vasm("v16mov  H16(59, 0), %r", kernel_x[32]);
      _vasm("v16ld  H16(60, 0), (%r)", kernel_x);
      _vasm("v16ld  H16(60, 32), (%r)", kernel_x + 16);
      {
         uint32_t i, vr;
         for (i = 33 - offset_x, vr = 61 << 6; i < kernel_width; i += 16, vr += 32) {
            _vasm("v16bitplanes  -, %r SETF", (1 << _min(kernel_width - i, 16)) - 1);
            _vasm("v16ld  H16(0, 0) + %r, (%r) IFNZ", vr, kernel_x + i);
         }
      }
      #define KERNEL (0x1000 + (59 << 6))
#else
      /* use uniforms memory for kernel */
      um_ld(0, kernel_x[32]);
      um_ld_16s(1, 64, kernel_x, 16, kernel_x + 16, 16);
      {
         uint32_t i, offset;
         for (i = 33 - offset_x, offset = 2; i < kernel_width; i += 32, ++offset) {
            um_ld_16s(offset, 64,
               kernel_x + i, _min(kernel_width - i, 16),
               kernel_x + i + 16, clampi(kernel_width - (i + 16), 0, 16));
         }
      }
      khrn_bf_um_stall();
      #define KERNEL 0x1000
#endif

      uint32_t n = offset_x + kernel_width + 15;

      int32_t i, j, k, vr_channel, vr_kernel;
      for (j = begin_j; j < (int32_t)dst_ld_st.sub_height; j += 16) {
         for (k = 0; (k + 16) < n;
#ifdef __VIDEOCORE4__
            k += 32) {
#else
            k += 48) {
#endif
            uint32_t ldn1 = blk_ldpt(16 << 6, temp_ld_st, begin_i - shift_x + k, j, tile);
            uint32_t ldn2 = 0;
            if ((k + 32) < n) {
               ldn2 = blk_ldpt(32 << 6, temp_ld_st, begin_i - shift_x + k + 16, j, tile);
            }
#ifndef __VIDEOCORE4__
            uint32_t ldn3 = 0;
            if ((k + 48) < n) {
               ldn3 = blk_ldpt(48 << 6, temp_ld_st, begin_i - shift_x + k + 32, j, tile);
            }
#endif
            blk_ldpt_unpack(16 << 6, 0 << 6, ldn1, temp_ld_st, tile, tile_fill_rgba, IMAGE_FORMAT_INVALID, (KHRN_IMAGE_CONV_T)0);
            _vasm("v32mov  V32(0, 0++), H32(16++, 0) REP 16");
            khrn_bf_blk_st_ppulm(0 << 6, begin_i + k);
            if ((k + 32) < n) {
               blk_ldpt_unpack(32 << 6, 0 << 6, ldn2, temp_ld_st, tile, tile_fill_rgba, IMAGE_FORMAT_INVALID, (KHRN_IMAGE_CONV_T)0);
               _vasm("v32mov  V32(0, 0++), H32(32++, 0) REP 16");
               khrn_bf_blk_st_ppulm(0 << 6, begin_i + k + 16);
            }
#ifndef __VIDEOCORE4__
            if ((k + 48) < n) {
               blk_ldpt_unpack(48 << 6, 0 << 6, ldn3, temp_ld_st, tile, tile_fill_rgba, IMAGE_FORMAT_INVALID, (KHRN_IMAGE_CONV_T)0);
               _vasm("v32mov  V32(0, 0++), H32(48++, 0) REP 16");
               khrn_bf_blk_st_ppulm(0 << 6, begin_i + k + 32);
            }
#endif
         }
         uint32_t ldn = blk_ldpt(16 << 6, temp_ld_st, begin_i - shift_x + ((n - 1) & ~0xf), j, tile);

         for (i = begin_i; i < (int32_t)dst_ld_st.sub_width; i += 16) {
            blk_ldpt_unpack(16 << 6, 0 << 6, ldn, temp_ld_st, tile, tile_fill_rgba, IMAGE_FORMAT_INVALID, (KHRN_IMAGE_CONV_T)0);
            _vasm("v32mov  V32(0, 0++), H32(16++, 0) REP 16");
            khrn_bf_blk_st_ppulm(0 << 6, i + ((n - 1) & ~0xf));

            khrn_bf_blk_ld_ppulm(0 << 6, i);
            khrn_bf_blk_ld_ppulm(16 << 6, i + 16);
            khrn_bf_blk_ld_ppulm(32 << 6, i + 32);
            for (vr_channel = 0; vr_channel != 64; vr_channel += 16) {
               #define X(I, J) _vasm("v32memwrite  -, H32(48 + "#I", 0), %r", i + J + I - (16 + vr_channel));
#ifdef __VIDEOCORE4__
               khrn_bf_blk_sconv_8_32(48 << 6, 8, vr_channel + (offset_x << 6), KERNEL, 33 - offset_x);
               X(0, 0); X(1, 0); X(2, 0); X(3, 0); X(4, 0); X(5, 0); X(6, 0); X(7, 0);
               khrn_bf_blk_sconv_8_32(48 << 6, 8, vr_channel + ((offset_x + 8) << 6), KERNEL, 33 - offset_x);
               X(0, 8); X(1, 8); X(2, 8); X(3, 8); X(4, 8); X(5, 8); X(6, 8); X(7, 8);
#else
               khrn_bf_blk_sconv_8_32(48 << 6, 16, vr_channel + (offset_x << 6), KERNEL, 33 - offset_x);
               X(0, 0); X(1, 0); X(2, 0); X(3, 0); X(4, 0); X(5, 0); X(6, 0); X(7, 0);
               X(8, 0); X(9, 0); X(10, 0); X(11, 0); X(12, 0); X(13, 0); X(14, 0); X(15, 0);
#endif
               #undef X
            }

            for (k = 48, vr_kernel = KERNEL + (1 << 6);; k += 32, vr_kernel += (1 << 6)) {
               _vasm("v32mov  H32(0++, 0), H32(32++, 0) REP 16");
               khrn_bf_blk_ld_ppulm(16 << 6, i + k);
               if ((k + 16) < n) {
                  khrn_bf_blk_ld_ppulm(32 << 6, i + k + 16);
               }
               if ((k + 32) >= n) { break; }
               for (vr_channel = 0; vr_channel != 64; vr_channel += 16) {
                  #define X(I, J) \
                     do { \
                        _vasm("v32mov  -, H32(48 + "#I", 0) CLRA SACC"); \
                        _vasm("v32memread  H32(48 + "#I", 0), -, %r SACC", i + J + I - (16 + vr_channel)); \
                        _vasm("v32memwrite  -, H32(48 + "#I", 0), %r", i + J + I - (16 + vr_channel)); \
                     } while (0)
#ifdef __VIDEOCORE4__
                  khrn_bf_blk_sconv_8_32(48 << 6, 8, vr_channel + (1 << 6), vr_kernel, 32);
                  X(0, 0); X(1, 0); X(2, 0); X(3, 0); X(4, 0); X(5, 0); X(6, 0); X(7, 0);
                  khrn_bf_blk_sconv_8_32(48 << 6, 8, vr_channel + (9 << 6), vr_kernel, 32);
                  X(0, 8); X(1, 8); X(2, 8); X(3, 8); X(4, 8); X(5, 8); X(6, 8); X(7, 8);
#else
                  khrn_bf_blk_sconv_8_32(48 << 6, 16, vr_channel + (1 << 6), vr_kernel, 32);
                  X(0, 0); X(1, 0); X(2, 0); X(3, 0); X(4, 0); X(5, 0); X(6, 0); X(7, 0);
                  X(8, 0); X(9, 0); X(10, 0); X(11, 0); X(12, 0); X(13, 0); X(14, 0); X(15, 0);
#endif
                  #undef X
               }
            }

            for (vr_channel = 0; vr_channel != 64; vr_channel += 16) {
               #define X(I, J) \
                  do { \
                     _vasm("v32adds  -, H32(48 + "#I", 0), %r CLRA SACC", (int32_t)bias); \
                     _vasm("v32memread  H32(48 + "#I", 0), -, %r SACC", i + J + I - (16 + vr_channel)); \
                  } while (0)
#ifdef __VIDEOCORE4__
               khrn_bf_blk_sconv_8_32(48 << 6, 8, vr_channel + (1 << 6), vr_kernel, n - k);
               X(0, 0); X(1, 0); X(2, 0); X(3, 0); X(4, 0); X(5, 0); X(6, 0); X(7, 0);
               _vasm("v32signasls  H8(0++, 0) + %r, H32(48++, 0), %r REP 8", vr_channel, -shift);
               khrn_bf_blk_sconv_8_32(48 << 6, 8, vr_channel + (9 << 6), vr_kernel, n - k);
               X(0, 8); X(1, 8); X(2, 8); X(3, 8); X(4, 8); X(5, 8); X(6, 8); X(7, 8);
               _vasm("v32signasls  H8(8++, 0) + %r, H32(48++, 0), %r REP 8", vr_channel, -shift);
#else
               khrn_bf_blk_sconv_8_32(48 << 6, 16, vr_channel + (1 << 6), vr_kernel, n - k);
               X(0, 0); X(1, 0); X(2, 0); X(3, 0); X(4, 0); X(5, 0); X(6, 0); X(7, 0);
               X(8, 0); X(9, 0); X(10, 0); X(11, 0); X(12, 0); X(13, 0); X(14, 0); X(15, 0);
               _vasm("v32signasls  H8(0++, 0) + %r, H32(48++, 0), %r REP 16", vr_channel, -shift);
#endif
               #undef X
            }

            if ((i + 16) < (int32_t)dst_ld_st.sub_width) {
               ldn = blk_ldpt(16 << 6, temp_ld_st, i - shift_x + ((n + 15) & ~0xf), j, tile);
            }

            _vasm("v32mov  V32(32, 0++), H32(0++, 0) REP 16");
            blk_stap_channel_masked(32 << 6, 0 << 6, false, &dst_ld_st, i, j, channel_mask, format, conv);
         }
      }

      #undef KERNEL
   }

   vclib_release_VRF();

   mem_unlock(sconv_temp_handle);

   return true;
}

/* todo: better align allowance */

void khrn_bf_conv_prepare(
   int16_t *kernel, int32_t *bias, int32_t *shift,
   const int16_t *kernel_in, uint32_t kernel_width, uint32_t kernel_height,
   float scale, float bias_in)
{
   uint32_t i, j;

   int32_t max_abs_kernel = 0;
   for (i = 0; i != (kernel_width * kernel_height); ++i) {
      if (kernel_in[i] > max_abs_kernel) { max_abs_kernel = kernel_in[i]; }
      else if (-kernel_in[i] > max_abs_kernel) { max_abs_kernel = -kernel_in[i]; }
   }

   bias_in *= 255.0f;
   /* we want to get maximum precision (ie maximum shift) while satisfying these
    * requirements: */
   *shift = clampi(_min(
      /* kernel should fit in signed 16-bit */
      get_optimal_shift(max_abs_kernel * scale, 16),
      /* bias and result should fit in signed 32-bit
       * if the kernel fits in signed 16-bit, then the maximum result before
       * applying the bias is 0x700e9fe1 (15 * 15 * 32767 * 255), so if we
       * instead make sure the bias fits in signed 29-bit, the result won't
       * overflow */
      get_optimal_shift(bias_in, 29)),
      /* shift must be in [-32, 31] (float_to_int_shift requires this) */
      -32, 31);

   for (i = 0; i != kernel_width; ++i) {
      for (j = 0; j != kernel_height; ++j) {
         kernel[(j * 15) + i] = clampi(float_to_int_shift(
            kernel_in[((kernel_width - 1 - i) * kernel_height) + (kernel_height - 1 - j)] * scale,
            *shift), -0x8000, 0x7fff);
      }
   }
   if (*shift > 0) {
      bias_in += 0.49999997f;
   }
   *bias = float_to_int_shift(bias_in, *shift);
}

void khrn_bf_conv(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y, uint32_t dst_width, uint32_t dst_height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y, uint32_t src_width, uint32_t src_height,
   uint32_t channel_mask, KHRN_IMAGE_FORMAT_T format, KHRN_IMAGE_CONV_T conv,
   KHRN_BF_TILE_T tile, uint32_t tile_fill_rgba,
   int32_t shift_x, int32_t shift_y,
   const int16_t *kernel, uint32_t kernel_width, uint32_t kernel_height, int32_t bias, int32_t shift)
{
   vcos_assert(khrn_image_is_color(dst->format) && khrn_image_is_color(src->format));
   vcos_assert((dst_x + dst_width) <= dst->width);
   vcos_assert((dst_y + dst_height) <= dst->height);
   vcos_assert((src_x + src_width) <= src->width);
   vcos_assert((src_y + src_height) <= src->height);
   vcos_assert((kernel_width >= 1) && (kernel_width <= 15));
   vcos_assert((kernel_height >= 1) && (kernel_height <= 15));

   if ((dst_width == 0) || (dst_height == 0)) {
      return;
   }
   vcos_assert((src_width != 0) && (src_height != 0));

   BLK_LD_ST_T dst_ld_st, src_ld_st;
   blk_ld_st_init(&dst_ld_st, dst, dst_x, dst_y, dst_width, dst_height);
   blk_ld_st_init(&src_ld_st, src, src_x, src_y, src_width, src_height);

   int32_t begin_i, begin_j;
   uint32_t offset_x, offset_y;
   get_optimal_begin(&begin_i, &begin_j, &offset_x, &offset_y,
      dst_ld_st.sub_x, dst_ld_st.sub_y, dst_ld_st.format,
      src_ld_st.sub_x - shift_x, src_ld_st.sub_y - shift_y, src_ld_st.format,
      17 - kernel_width, 17 - kernel_height);
   shift_x += offset_x;
   shift_y += offset_y;
   uint32_t offset = (offset_y << 6) + offset_x;

#ifndef __VIDEOCORE4__
   /* use uniforms memory for kernel/bias/shift */
   {
      int16_t neg_shift = -shift;
      uint32_t i;
      for (i = 0; i != kernel_height; ++i) {
         um_ld_16s(i, 64, kernel + (i * 15), kernel_width, &neg_shift, i == 0);
      }
      um_ld(15 * 64, bias);
      khrn_bf_um_stall();
   }
#endif

   vclib_obtain_VRF(1);

#ifdef __VIDEOCORE4__
   /* use 0x1000 replicating alias for kernel/bias/shift */
   _vasm("v16bitplanes  -, %r SETF", (1 << kernel_width) - 1);
   _vasm("v16ld  H16(48++, 0), (%r += %r) IFNZ REP %r", kernel, 30, kernel_height);
   _vasm("v16mov  H16(48, 32), %r", -shift);
   _vasm("v32mov  V32(48, 15), %r", bias);
#endif

   tile = (KHRN_BF_TILE_T)(tile | BF_TILE_X | BF_TILE_Y | BF_TILE_UNPACK_AND_CONVERT);

   int32_t i, j;
   for (i = begin_i; i < (int32_t)dst_ld_st.sub_width; i += 16) {
      uint32_t ldn1 = blk_ldpt(0 << 6, &src_ld_st, i - shift_x, begin_j - shift_y, tile);
      uint32_t ldn2 = blk_ldpt(32 << 6, &src_ld_st, i - shift_x + 16, begin_j - shift_y, tile);
      blk_ldpt_unpack(0 << 6, 16 << 6, ldn1, &src_ld_st, tile, tile_fill_rgba, format, conv);
      blk_ldpt_unpack(32 << 6, 16 << 6, ldn2, &src_ld_st, tile, tile_fill_rgba, format, conv);
      _vasm("v16eor  H8(0++, 16), H8(0++, 16), H8(32++, 0) REP 16");
      _vasm("v16eor  H8(32++, 0), H8(0++, 16), H8(32++, 0) REP 16");
      _vasm("v16eor  H8(0++, 16), H8(0++, 16), H8(32++, 0) REP 16");
      _vasm("v16eor  H8(0++, 48), H8(0++, 48), H8(32++, 32) REP 16");
      _vasm("v16eor  H8(32++, 32), H8(0++, 48), H8(32++, 32) REP 16");
      _vasm("v16eor  H8(0++, 48), H8(0++, 48), H8(32++, 32) REP 16");
      khrn_bf_blk_st_ppulm(0 << 6, 0);
#ifdef __VIDEOCORE4__
      khrn_bf_blk_st_ppulm(32 << 6, 16);
#endif
      for (j = begin_j; j < (int32_t)dst_ld_st.sub_height; j += 16) {
#ifdef __VIDEOCORE4__
         #define TB 32
         #define TBS "32"
         #define KERNEL (0x1000 + (48 << 6))
#else
         #define TB 48
         #define TBS "48"
         #define KERNEL 0x1000
#endif
         ldn1 = blk_ldpt(16 << 6, &src_ld_st, i - shift_x, j - shift_y + 16, tile);
         ldn2 = blk_ldpt(TB << 6, &src_ld_st, i - shift_x + 16, j - shift_y + 16, tile);
         blk_ldpt_unpack(16 << 6, 0 << 6, ldn1, &src_ld_st, tile, tile_fill_rgba, format, conv);
         blk_ldpt_unpack(TB << 6, 0 << 6, ldn2, &src_ld_st, tile, tile_fill_rgba, format, conv);
         khrn_bf_blk_ld_ppulm(0 << 6, 0);
         _vasm("v16eor  H8(16++, 16), H8(16++, 16), H8("TBS"++, 0) REP 16");
         _vasm("v16eor  H8("TBS"++, 0), H8(16++, 16), H8("TBS"++, 0) REP 16");
         _vasm("v16eor  H8(16++, 16), H8(16++, 16), H8("TBS"++, 0) REP 16");
         _vasm("v16eor  H8(16++, 48), H8(16++, 48), H8("TBS"++, 32) REP 16");
         _vasm("v16eor  H8("TBS"++, 32), H8(16++, 48), H8("TBS"++, 32) REP 16");
         _vasm("v16eor  H8(16++, 48), H8(16++, 48), H8("TBS"++, 32) REP 16");
#ifndef __VIDEOCORE4__
         _vasm("v32memwrite  -, H32(63, 0), 32"); /* khrn_bf_blk_conv uses H32(63, 0) as a temp row */
#endif
         khrn_bf_blk_conv(0, 0 + offset, KERNEL, kernel_width, kernel_height);
         khrn_bf_blk_conv(32, 32 + offset, KERNEL, kernel_width, kernel_height);
#ifdef __VIDEOCORE4__
         khrn_bf_blk_st_ppulm(16 << 6, 0);
         khrn_bf_blk_ld_ppulm(16 << 6, 16);
#endif
         khrn_bf_blk_conv(16, ((TB - 16) << 6) + offset, KERNEL, kernel_width, kernel_height);
         khrn_bf_blk_conv(48, ((TB - 16) << 6) + 32 + offset, KERNEL, kernel_width, kernel_height);
         if ((conv == IMAGE_CONV_VG) && (format & IMAGE_FORMAT_PRE)) {
            khrn_bf_blk_clamp_to_a(0 << 6);
         }
         blk_stap_channel_masked(0 << 6, (TB - 16) << 6, false, &dst_ld_st, i, j, channel_mask, format, conv);
#ifdef __VIDEOCORE4__
         khrn_bf_blk_st_ppulm(32 << 6, 16);
#else
         khrn_bf_blk_st_ppulm(16 << 6, 0);
         _vasm("v32mov  H32(32++, 0), H32(48++, 0) REP %r", 15);
         _vasm("v32memread  H32(47, 0), -, 32");
#endif
         #undef KERNEL
         #undef TBS
         #undef TB
      }
   }

   vclib_release_VRF();
}

/******************************************************************************
subsampling
******************************************************************************/

extern void khrn_bf_blk_subsample_tf_32(uint32_t vr_src, uint32_t vr_dst);
extern void khrn_bf_row_subsample_tf_565(void *dst, const void *src, uint32_t subtile_count, uint32_t row_number);
extern void khrn_bf_row_subsample_tf_4444(void *dst, const void *src, uint32_t subtile_count, uint32_t row_number);
extern void khrn_bf_row_subsample_tf_5551(void *dst, const void *src, uint32_t subtile_count, uint32_t row_number);
extern void khrn_bf_blk_subsample_ld_microtiles(uint32_t utw, uint32_t uth, const void *data, int stride, uint32_t vr_data);
extern void khrn_bf_blk_subsample_st_microtiles(uint32_t utw, uint32_t uth, void *data, int stride, uint32_t vr_data);

bool khrn_bf_subsample(KHRN_IMAGE_WRAP_T *dst, const KHRN_IMAGE_WRAP_T *src)
{
   KHRN_IMAGE_FORMAT_T sformat, dformat;
   void (*convert)(uint32_t,uint32_t);
   uint32_t pad, htile;
   int s_utw0, s_utw1, s_uth0, s_uth1;
   int d_utw, d_uth;
   int s_uts, d_uts;
   uint8_t *s_data, *d_data;

   sformat = src->format & ~(IMAGE_FORMAT_PRE | IMAGE_FORMAT_LIN);
   dformat = dst->format & ~(IMAGE_FORMAT_PRE | IMAGE_FORMAT_LIN);

   vcos_assert(!((dformat ^ sformat) & ~IMAGE_FORMAT_MEM_LAYOUT_MASK));
   vcos_assert(khrn_image_is_color(sformat));
   vcos_assert(dst->width == _max(src->width >> 1, 1));
   vcos_assert(dst->height == _max(src->height >> 1, 1));
   if ((dformat == ABGR_8888_TF || dformat == ABGR_8888_LT || dformat == XBGR_8888_TF || dformat == XBGR_8888_LT)
    && (sformat == ABGR_8888_TF || sformat == ABGR_8888_LT || sformat == XBGR_8888_TF || sformat == XBGR_8888_LT))
   {
      convert = khrn_bf_blk_subsample_tf_32;
      pad = 0x1f;
      htile = 16;
   }
   else if ((dformat == RGB_565_TF && sformat == RGB_565_TF) || (dformat == RGBA_4444_TF && sformat == RGBA_4444_TF) || (dformat == RGBA_5551_TF && sformat == RGBA_5551_TF))
   {
      int i, j = 0, d_tweak = 0, s_tweak;
      void (*row_subsample)(void *, const void *, uint32_t, uint32_t);

      switch (dformat)
      {
      case RGB_565_TF:
         row_subsample = khrn_bf_row_subsample_tf_565;
         break;
      case RGBA_4444_TF:
         row_subsample = khrn_bf_row_subsample_tf_4444;
         break;
      case RGBA_5551_TF:
         row_subsample = khrn_bf_row_subsample_tf_5551;
         break;
      default:
         UNREACHABLE();
         row_subsample = NULL;
      }

      for (i = 0; i < (dst->height + 15) / 16; i++)
      {
         switch (i & 3)
         {
         case 0:
            j = i >> 1;
            d_tweak = 0;
            break;
         case 1:
            j = (i >> 1) + 1;
            d_tweak = -2048;
            break;
         case 2:
            j = (i >> 1) + 1;
            d_tweak = -2048;
            break;
         case 3:
            j = i >> 1;
            d_tweak = 0;
            break;
         }
         s_tweak = 0;
         if (i & 1) s_tweak = 32 * (src->stride - 2 * dst->stride);

         vclib_obtain_VRF(1);
         row_subsample(
            (uint8_t *)dst->storage + dst->stride * 32 * j + d_tweak,
            (const uint8_t *)src->storage + src->stride * 32 * i + s_tweak,
            8 * ((dst->width + 63) / 64),
            i);
         vclib_release_VRF();
      }
      return true;
   }
   else
      return false;

   int i,begin_i,end_i,dir_i;
   int j,begin_j,end_j,dir_j;
   BLK_LD_ST_T dst_ld_st, src_ld_st;
   uint32_t vr_src0, vr_src1, vr_dst;

   blk_ld_st_init(&dst_ld_st, dst, 0, 0, dst->width, dst->height);
   blk_ld_st_init(&src_ld_st, src, 0, 0, src->width, src->height);

   begin_i = 0;
   begin_j = 0;
   end_i = begin_i + ((-begin_i + _min(src->width,dst->width<<1) + pad) & ~pad);
   end_j = begin_j + ((-begin_j + _min(src->height,dst->height<<1) + pad) & ~pad);
   dir_i = 2 * htile;
   dir_j = 32;

   vr_src0 = 0 << 6;
   vr_src1 = 16 << 6;
   vr_dst = 32 << 6;

   vclib_obtain_VRF(1);

   s_uts = khrn_image_is_tformat(src_ld_st.format) ? 0x100 : 4 * src_ld_st.stride;
   d_uts = khrn_image_is_tformat(dst_ld_st.format) ? 0x100 : 4 * dst_ld_st.stride;
   s_data = (uint8_t *)src_ld_st.data;
   d_data = (uint8_t *)dst_ld_st.data;
   for (j = begin_j; j != end_j; j += dir_j) {
      int x0, x1, y0, y1, xd, yd;

      y0 = j;
      y1 = j + 16;
      yd = j / 2;

      s_uth0 = _max(_min((src_ld_st.height - y0 + 3) / 4, 4), 0);
      s_uth1 = _max(_min((src_ld_st.height - y1 + 3) / 4, 4), 0);
      d_uth  = _max(_min((dst_ld_st.height - yd + 3) / 4, 4), 0);
      vcos_assert(d_uth!=0);//at least one row of tiles to store

      for (i = begin_i; i != end_i; i += dir_i) {
         x0 = i;
         x1 = i + htile;
         xd = i / 2;

         s_utw0 = _max(_min((src_ld_st.width - x0 + 3) / 4, 4), 0);
         s_utw1 = _max(_min((src_ld_st.width - x1 + 3) / 4, 4), 0);
         d_utw  = _max(_min((dst_ld_st.width - xd + 3) / 4, 4), 0);

         khrn_bf_blk_subsample_ld_microtiles(s_utw0, s_uth0, s_data + blk_get_offset(src_ld_st.stride, x0, y0, src_ld_st.format), s_uts, vr_src0);
         khrn_bf_blk_subsample_ld_microtiles(s_utw1, s_uth0, s_data + blk_get_offset(src_ld_st.stride, x1, y0, src_ld_st.format), s_uts, vr_src1);
         convert(vr_src0, vr_dst);

         khrn_bf_blk_subsample_ld_microtiles(s_utw0, s_uth1, s_data + blk_get_offset(src_ld_st.stride, x0, y1, src_ld_st.format), s_uts, vr_src0);
         convert(vr_src1, vr_dst + (2<<6));

         khrn_bf_blk_subsample_ld_microtiles(s_utw1, s_uth1, s_data + blk_get_offset(src_ld_st.stride, x1, y1, src_ld_st.format), s_uts, vr_src1);
         convert(vr_src0, vr_dst + (8<<6));

         convert(vr_src1, vr_dst + (10<<6));

         khrn_bf_blk_subsample_st_microtiles(d_utw, d_uth, d_data + blk_get_offset(dst_ld_st.stride, xd, yd, dst_ld_st.format), d_uts, vr_dst);
      }
   }

   vclib_release_VRF();
   return true;
}
