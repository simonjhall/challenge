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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "vcfw/rtos/common/rtos_common_mem.h"

#ifdef __cplusplus
}
#endif

/* Project includes */
#include "vcinclude/common.h"
#include "vcinclude/vcore.h"
#include "vcinclude/hardware.h"
#include "helpers/vclib/vclib.h"
#include "helpers/vc_pool/vc_pool.h"
#include "interface/vcos/vcos_assert.h"
#include "vc_image_resize_bytes.h"

#ifdef __CC_ARM
#include "vcinclude/vc_asm_ops.h"
#define free_256bit(ret)			free(ret)
#define vclib_check_VRF()			1
#define IS_ALIAS_DIRECT(x)			0
#define ALIAS_ANY_NONALLOCATING(x)	(void *)(x) 
#define ALIAS_DIRECT(x)				((void *)(x))
#define ALIAS_COHERENT(x)			((void *)(x))
#define ALIAS_NORMAL(x)				((void *)(x))
#define IS_ALIAS_COHERENT(x)		0
#define VCLIB_PRIORITY_COHERENT		0
#define VCLIB_PRIORITY_DIRECT		0

void yuvuv_downsample_4mb (unsigned char *a, unsigned char *b, int c, const unsigned char*d, const unsigned char *e, int f)
{
	assert(0);
}

#endif

#include "vc_image.h"
#include "striperesize.h"
#include "vc_image_rgb888.h"

#if defined (USE_YUV420) || defined (USE_8BPP)
#include "vc_image_yuv.h"
#endif
// note: imviewer requires yuv420 -> RGB565 conversion even when USE_RGB565 is not defined...
#if defined(USE_RGB565) || defined(USE_RGBA565) || defined(USE_RGBA16) || defined(USE_YUV420)
#include "vc_image_rgb565.h"
#endif
#ifdef USE_1BPP
#include "vc_image_1bpp.h"
#endif
#ifdef USE_RGBA32
#include "vc_image_rgba32.h"
#endif
#ifdef USE_RGBA16
#include "vc_image_rgba16.h"
#endif
#ifdef USE_RGBA565
#include "vc_image_rgba565.h"
#endif
#ifdef USE_RGB888
#include "vc_image_rgb888.h"
#endif
#ifdef USE_RGB666
#include "vc_image_rgb666.h"
#endif
#ifdef USE_RGB2X9
#include "vc_image_rgb2x9.h"
#endif
#ifdef USE_4BPP
#include "vc_image_4bpp.h"
#endif
#if defined(USE_8BPP) || defined(USE_YUV420) || defined(USE_YUV422)
#include "vc_image_8bpp.h"
#endif
#ifdef USE_TFORMAT
#include "vc_image_tformat.h"
#endif
#ifdef USE_BGR888
#include "vc_image_bgr888.h"
#endif
#ifdef USE_YUV_UV
#include "vc_image_yuvuv128.h"
#endif
#if defined(USE_YUV420) || defined(USE_YUV_UV)
#include "vc_image_rgb_to_yuv.h"
#endif


/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern long fix_alignment(VC_IMAGE_TYPE_T type, long value);
extern int calculate_pitch(VC_IMAGE_TYPE_T type, int width, int height, VC_IMAGE_EXTRA_T *extra);
extern int vc_image_bits_per_pixel (VC_IMAGE_TYPE_T type);
extern unsigned int vc_image_get_supported_image_formats ( void );
extern void vc_image_initialise (VC_IMAGE_T *image);
extern int vc_image_vconfigure(VC_IMAGE_T *image, VC_IMAGE_TYPE_T type, long width, long height, va_list proplist);
extern int vc_image_vreconfigure(VC_IMAGE_T *image, VC_IMAGE_TYPE_T type, long width, long height, va_list proplist);
extern int vc_image_configure_unwrapped(VC_IMAGE_T *image, VC_IMAGE_TYPE_T type, long width, long height, VC_IMAGE_PROPERTY_T prop, ...);
extern int vc_image_reconfigure_unwrapped(VC_IMAGE_T *image, VC_IMAGE_TYPE_T type, long width, long height, VC_IMAGE_PROPERTY_T prop, ...);
extern void vc_image_set_type (VC_IMAGE_T *image, VC_IMAGE_TYPE_T type);
extern void vc_image_set_image_data (VC_IMAGE_BUF_T *image, int size, void *image_data);
extern void vc_image_set_image_data_yuv (VC_IMAGE_BUF_T *image, int size, void *image_y, void *image_u, void *image_v);
extern void vc_image_set_dimensions (VC_IMAGE_T *image, int width, int height);
extern int vc_image_verify (const VC_IMAGE_T *image);
extern void vc_image_set_pitch (VC_IMAGE_T *image, int pitch);
extern int vc_image_required_size (VC_IMAGE_T *image);
extern int vc_image_palette_size (VC_IMAGE_T *image);
extern unsigned char *vc_image_get_u (const VC_IMAGE_BUF_T *image);
extern unsigned char *vc_image_get_v (const VC_IMAGE_BUF_T *image);
extern int vc_image_reshape (VC_IMAGE_T *image, VC_IMAGE_TYPE_T type, int width, int height);
extern void vc_image_fill (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height, int value);
extern void vc_image_blt (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                             VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset);
extern void vc_image_convert_blt (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                                     VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset);
extern void vc_image_transparent_blt (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                                         VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset, int transparent_colour);
extern void vc_image_overwrite_blt (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                                       VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset, int overwrite_colour);
extern void vc_image_masked_blt (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                                    VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset, VC_IMAGE_BUF_T *mask, int mask_x_offset,
                                    int mask_y_offset, int invert);
extern void vc_image_masked_fill (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height, int value,
                                     VC_IMAGE_BUF_T *mask, int mask_x_offset, int mask_y_offset, int invert);
extern void vc_image_fetch (VC_IMAGE_BUF_T *pimage, unsigned char* Y, unsigned char* U, unsigned char* V);
extern void vc_image_put (VC_IMAGE_BUF_T *pimage, unsigned char* Y, unsigned char* U, unsigned char* V);
extern void vc_image_not (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height);
extern void vc_image_fade (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height, int fade);
extern void vc_image_or (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                            VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset);
extern void vc_image_xor (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                             VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset);
extern void vc_image_copy (VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src);
extern int vc_image_transpose_ret (VC_IMAGE_T *dest, VC_IMAGE_T *src);
extern void vc_image_transpose (VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src);
extern void vc_image_transpose_tiles(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, int hdeci);
extern void vc_image_transpose_and_subsample(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, int factor);
extern void vc_image_vflip (VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src);
extern void vc_image_hflip (VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src);
extern void vc_image_transpose_in_place (VC_IMAGE_BUF_T *dest);
extern void vc_image_vflip_in_place (VC_IMAGE_BUF_T *dest);
extern void vc_image_hflip_in_place (VC_IMAGE_BUF_T *dest);
extern void vc_image_text (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                              int *max_x, int *max_y);
extern void vc_image_small_text (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                    int *max_x, int *max_y);
extern void vc_image_text_32 (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                 int *max_x, int *max_y);
extern void vc_image_text_24 (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                 int *max_x, int *max_y);
extern void vc_image_text_20 (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                 int *max_x, int *max_y);
extern void vc_image_textrotate (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                                    int rotate);
extern void vc_image_line (VC_IMAGE_BUF_T *dest, int x_start, int y_start, int x_end, int y_end, int fg_colour);
extern void vc_image_frame (VC_IMAGE_BUF_T *dest, int x_start, int y_start, int width, int height);
extern void vc_image_resize (VC_IMAGE_BUF_T *dest, int dest_x_offset, int dest_y_offset,
                                int dest_width, int dest_height, VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset,
                                int src_width, int src_height, int smooth_flag);
extern void vc_image_convert_to_48bpp (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                                          VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset);
extern void vc_image_set_ambient (int a);
extern void vc_image_set_specular (int s);
extern void vc_image_convert (VC_IMAGE_BUF_T *dest,VC_IMAGE_BUF_T *src,int mirror,int rotate);
extern void vc_image_yuv2rgb (VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, int downsample);
extern void vc_image_free (VC_IMAGE_BUF_T *img);
extern VC_IMAGE_T *vc_runlength_decode (unsigned char *data);
extern VC_IMAGE_BUF_T *vc_run_decode (VC_IMAGE_BUF_T *img,unsigned char *data);
extern VC_IMAGE_T *vc_runlength_decode8 (unsigned char *data);
extern void vc_image_deblock (VC_IMAGE_BUF_T *img);
extern void vc_image_pack (VC_IMAGE_BUF_T *img, int x, int y, int w, int h);
extern void vc_image_copy_pack (unsigned char *dest, VC_IMAGE_BUF_T *img, int x, int y, int w, int h);
extern void vc_image_unpack (VC_IMAGE_BUF_T *img, int src_pitch, int h);
extern void vc_image_copy_unpack (VC_IMAGE_BUF_T *img, int dest_x_off, int dest_y_off, unsigned char *src, int w, int h);
extern VC_IMAGE_BUF_T *vc_image_vparmalloc_unwrapped(VC_IMAGE_TYPE_T type, char const *description, long width, long height, va_list proplist);
extern VC_IMAGE_BUF_T *vc_image_parmalloc_unwrapped(VC_IMAGE_TYPE_T type, char const *description, long width, long height, VC_IMAGE_PROPERTY_T prop, ...);
extern VC_IMAGE_BUF_T *vc_image_prioritymalloc( VC_IMAGE_TYPE_T type, long width,
         long height, int rotate, int priority, char const *name);
extern VC_IMAGE_BUF_T *vc_image_malloc( VC_IMAGE_TYPE_T type, long width, long height, int rotate );
extern void vc_image_set_palette ( short *colourmap );
extern short *vc_image_get_palette ( VC_IMAGE_T *src );
extern VC_IMAGE_BUF_T *vc_4by4_decode (VC_IMAGE_BUF_T *img,unsigned short *data);
extern void vc_image_transform (VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, VC_IMAGE_TRANSFORM_T transform);
extern int vc_image_transform_ret (VC_IMAGE_T *dest, VC_IMAGE_T *src, VC_IMAGE_TRANSFORM_T transform);
extern void vc_image_transform_in_place (VC_IMAGE_BUF_T *img, VC_IMAGE_TRANSFORM_T transform);
extern void vc_image_transform_tiles (VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, VC_IMAGE_TRANSFORM_T transform, int hdeci);
extern VC_IMAGE_TRANSFORM_T vc_image_inverse_transform (VC_IMAGE_TRANSFORM_T transform);
extern VC_IMAGE_TRANSFORM_T vc_image_combine_transforms (VC_IMAGE_TRANSFORM_T t1, VC_IMAGE_TRANSFORM_T t2);
extern void vc_image_transform_point (int *x, int *y, int size_x, int size_y, VC_IMAGE_TRANSFORM_T transform);
extern void vc_image_transform_rect (int *x, int *y, int *w, int *h, int size_x, int size_y, VC_IMAGE_TRANSFORM_T transform);
extern void vc_image_transform_dimensions (int *w, int *h, VC_IMAGE_TRANSFORM_T transform);
extern const char *vc_image_transform_string(VC_IMAGE_TRANSFORM_T xfm);
extern void vc_image_convert_in_place (VC_IMAGE_BUF_T *image, VC_IMAGE_TYPE_T new_type);
extern void vc_image_transparent_alpha_blt (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
         VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset, int transparent_colour, int alpha);
extern void vc_image_font_alpha_blt (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                                        VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset, int red, int green, int blue, int alpha);
extern void vc_image_deflicker (VC_IMAGE_BUF_T *img, unsigned char *context);
extern void vc_image_resize_stripe_h (VC_IMAGE_BUF_T *image, int d_width, int s_width);
extern void vc_image_resize_stripe_v (VC_IMAGE_BUF_T *image, int d_height, int s_height);
extern void vc_image_decimate_stripe_h (VC_IMAGE_BUF_T *src, int offset, int step);
extern void vc_image_decimate_stripe_v (VC_IMAGE_BUF_T *src, int offset, int step);
extern int vc_image_set_alpha ( VC_IMAGE_BUF_T *image, const int x, const int y,
                                   const int width, const int height, const int alpha );
extern void yuvuv_downsample_4mb (unsigned char *, unsigned char *, int, const unsigned char*, const unsigned char *, int);

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

extern void vc_image_copy_stripe (unsigned char *, int, unsigned char *, int, int);
extern void vc_image_unpack_row (unsigned char *dest, unsigned char *src, int bytes);

extern void vc_image_line_horiz (VC_IMAGE_BUF_T *dest, int x_start, int y_start, int x_end, int y_end, int fg_colour);
extern void vc_image_line_vert (VC_IMAGE_BUF_T *dest, int x_start, int y_start, int x_end, int y_end, int fg_colour);
extern void vc_image_line_x (VC_IMAGE_BUF_T *dest, int x_start, int x_end, int y_start, int gradient, int fg_colour);
extern void vc_image_line_y (VC_IMAGE_BUF_T *dest, int x_start, int x_end, int y_start, int gradient, int fg_colour);

/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/

#define RGB16(red,green,blue) ((((red)>>3)<<(5+6))+(((green)>>2)<<(5))+((blue)>>3))

// macros for decimate functions
#define OFFSET_UV(offset, step) ((offset)>>1)
#define GET_RGB32(dest, x_offset, y_offset) ((unsigned int *)(((int*)(dest)->image_data) + (x_offset) + (y_offset)*((dest)->pitch>>2)))
#define GET_RGB16(dest, x_offset, y_offset) ((unsigned char *)(((short*)(dest)->image_data) + (x_offset) + (y_offset)*((dest)->pitch>>1)))
#define GET_YUV_Y(dest, x_offset, y_offset) (vc_image_get_y(dest) + ((y_offset)>>0)*((dest)->pitch>>0) + ((x_offset)>>0))
#define GET_YUV_U(dest, x_offset, y_offset) (vc_image_get_u(dest) + ((y_offset)>>1)*((dest)->pitch>>1) + ((x_offset)>>1)) /* YUV420-specific */
#define GET_YUV_V(dest, x_offset, y_offset) (vc_image_get_v(dest) + ((y_offset)>>1)*((dest)->pitch>>1) + ((x_offset)>>1)) /* YUV420-specific */
#define TV_STRIPE_HEIGHT 16

typedef struct
{
   int mipmap_levels;
   int cubemap;
   short const *palette;
   VC_IMAGE_BAYER_ORDER_T bayer_order;
   VC_IMAGE_BAYER_FORMAT_T bayer_format;
   int bayer_block_size;
   int codec_fourcc;
   int codec_maxsize;
   unsigned int opengl_format_type;
   VC_IMAGE_INFO_T info;
   unsigned int component_order;
   int normalised_alpha;
   int transparent_colour;
   unsigned long rgba_arg;
} imcfg_details_t;


/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/

void vc_image_convert_rgb2yuv(unsigned short *rgb, unsigned char *Y, unsigned char *U, unsigned char *V,
                           int rgb_pitch_pixels, int y_pitch_bytes, int width_pixels, int height);
static void imcfg_load_details(/*@inout@*/ imcfg_details_t *details, VC_IMAGE_T const *reference);
static void imcfg_store_details(/*@inout@*/ VC_IMAGE_T *image, imcfg_details_t const *details);
static int vc_image_v1configure(VC_IMAGE_T *image, unsigned long type_ex, long width, long height, VC_IMAGE_PROPERTY_T prop, va_list proplist);
static VC_IMAGE_BUF_T *vc_image_v1parmalloc(VC_IMAGE_TYPE_T type, char const *description, long width, long height, VC_IMAGE_PROPERTY_T prop, va_list proplist);
static void const *default_cache_alias(VC_IMAGE_T *image, void const *pointer);
static void vc_image_decimate_stripe_h_2(VC_IMAGE_BUF_T *src);
static void vc_image_zero_pointers( VC_IMAGE_BUF_T *img );
static void vc_image_calculate_pointers( VC_IMAGE_BUF_T *img );



/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/

static short *vc_image_palette;

static unsigned short vc_image_safety_palette[256] = {
   0x0000, 0x0006, 0x000c, 0x0013, 0x0019, 0x001f, 0x0180, 0x0186,
   0x018c, 0x0193, 0x0199, 0x019f, 0x0320, 0x0326, 0x032c, 0x0333,
   0x0339, 0x033f, 0x04c0, 0x04c6, 0x04cc, 0x04d3, 0x04d9, 0x04df,
   0x0660, 0x0666, 0x066c, 0x0673, 0x0679, 0x067f, 0x07e0, 0x07e6,
   0x07ec, 0x07f3, 0x07f9, 0x07ff, 0x3000, 0x3006, 0x300c, 0x3013,
   0x3019, 0x301f, 0x3180, 0x3186, 0x318c, 0x3193, 0x3199, 0x319f,
   0x3320, 0x3326, 0x332c, 0x3333, 0x3339, 0x333f, 0x34c0, 0x34c6,
   0x34cc, 0x34d3, 0x34d9, 0x34df, 0x3660, 0x3666, 0x366c, 0x3673,
   0x3679, 0x367f, 0x37e0, 0x37e6, 0x37ec, 0x37f3, 0x37f9, 0x37ff,
   0x6000, 0x6006, 0x600c, 0x6013, 0x6019, 0x601f, 0x6180, 0x6186,
   0x618c, 0x6193, 0x6199, 0x619f, 0x6320, 0x6326, 0x632c, 0x6333,
   0x6339, 0x633f, 0x64c0, 0x64c6, 0x64cc, 0x64d3, 0x64d9, 0x64df,
   0x6660, 0x6666, 0x666c, 0x6673, 0x6679, 0x667f, 0x67e0, 0x67e6,
   0x67ec, 0x67f3, 0x67f9, 0x67ff, 0x9800, 0x9806, 0x980c, 0x9813,
   0x9819, 0x981f, 0x9980, 0x9986, 0x998c, 0x9993, 0x9999, 0x999f,
   0x9b20, 0x9b26, 0x9b2c, 0x9b33, 0x9b39, 0x9b3f, 0x9cc0, 0x9cc6,
   0x9ccc, 0x9cd3, 0x9cd9, 0x9cdf, 0x9e60, 0x9e66, 0x9e6c, 0x9e73,
   0x9e79, 0x9e7f, 0x9fe0, 0x9fe6, 0x9fec, 0x9ff3, 0x9ff9, 0x9fff,
   0xc800, 0xc806, 0xc80c, 0xc813, 0xc819, 0xc81f, 0xc980, 0xc986,
   0xc98c, 0xc993, 0xc999, 0xc99f, 0xcb20, 0xcb26, 0xcb2c, 0xcb33,
   0xcb39, 0xcb3f, 0xccc0, 0xccc6, 0xcccc, 0xccd3, 0xccd9, 0xccdf,
   0xce60, 0xce66, 0xce6c, 0xce73, 0xce79, 0xce7f, 0xcfe0, 0xcfe6,
   0xcfec, 0xcff3, 0xcff9, 0xcfff, 0xf800, 0xf806, 0xf80c, 0xf813,
   0xf819, 0xf81f, 0xf980, 0xf986, 0xf98c, 0xf993, 0xf999, 0xf99f,
   0xfb20, 0xfb26, 0xfb2c, 0xfb33, 0xfb39, 0xfb3f, 0xfcc0, 0xfcc6,
   0xfccc, 0xfcd3, 0xfcd9, 0xfcdf, 0xfe60, 0xfe66, 0xfe6c, 0xfe73,
   0xfe79, 0xfe7f, 0xffe0, 0xffe6, 0xffec, 0xfff3, 0xfff9, 0xffff,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

/******************************************************************************
Global function definitions.
******************************************************************************/



/******************************************************************************
NAME
   vc_image_get_supported_image_formats

PARAMS
   void

FUNCTION
   Returns the supported image formats

RETURNS
   A bit mask (bit position defined by VC_IMAGE_TYPE_T) of the support image formats
******************************************************************************/
unsigned int vc_image_get_supported_image_formats( void )
{
   unsigned int formats = 0;

#ifdef USE_RGB565
   formats |= (1 << VC_IMAGE_RGB565);
#endif

#ifdef USE_1BPP
   formats |= (1 << VC_IMAGE_1BPP);
#endif

#ifdef USE_RGB565
   formats |= (1 << VC_IMAGE_YUV420);
#endif

#ifdef USE_48BPP
   formats |= (1 << VC_IMAGE_48BPP);
#endif

#ifdef USE_RGB888
   formats |= (1 << VC_IMAGE_RGB888);
#endif

#ifdef USE_8BPP
   formats |= (1 << VC_IMAGE_8BPP);
#endif

#ifdef USE_4BPP
   formats |= (1 << VC_IMAGE_4BPP);
#endif

#ifdef USE_3D32
   formats |= (1 << VC_IMAGE_3D32);
   formats |= (1 << VC_IMAGE_3D32B);
   formats |= (1 << VC_IMAGE_3D32MAT);
#endif

#ifdef USE_RGB2X9
   formats |= (1 << VC_IMAGE_RGB2X9);
#endif

#ifdef USE_RGB666
   formats |= (1 << VC_IMAGE_RGB666);
#endif

#ifdef USE_RGBA32
   formats |= (1 << VC_IMAGE_RGBA32);
   /* The following formats require more than 32bits in formats, so will require an api change
   formats |= (1 << VC_IMAGE_RGBX32);
   formats |= (1 << VC_IMAGE_ARGB8888);
   formats |= (1 << VC_IMAGE_XRGB8888);
   */
#endif

#ifdef USE_YUV422
   formats |= (1 << VC_IMAGE_YUV422);
#endif

#ifdef USE_RGBA565
   formats |= (1 << VC_IMAGE_RGBA565);
#endif

#ifdef USE_RGBA16
   formats |= (1 << VC_IMAGE_RGBA16);
#endif

#ifdef USE_TFORMAT
   formats |= (1 << VC_IMAGE_TF_RGBA32);
   formats |= (1 << VC_IMAGE_TF_RGBX32);
   formats |= (1 << VC_IMAGE_TF_FLOAT);
   formats |= (1 << VC_IMAGE_TF_RGBA16);
   formats |= (1 << VC_IMAGE_TF_RGBA5551);
   formats |= (1 << VC_IMAGE_TF_RGB565);
   formats |= (1 << VC_IMAGE_TF_YA88);
   formats |= (1 << VC_IMAGE_TF_BYTE);
   formats |= (1 << VC_IMAGE_TF_PAL8);
   formats |= (1 << VC_IMAGE_TF_PAL4);
   formats |= (1 << VC_IMAGE_TF_ETC1);
   /* The following formats require more than 32bits in formats, so will require an api change
   formats |= (1 << VC_IMAGE_TF_Y8);
   formats |= (1 << VC_IMAGE_TF_A8);
   formats |= (1 << VC_IMAGE_TF_SHORT);
   formats |= (1 << VC_IMAGE_TF_1BPP);
   */
#endif

   return formats;
}


/******************************************************************************
Image functions
******************************************************************************/


/******************************************************************************
NAME
   vc_image_initialise

SYNOPSIS
   void vc_image_initialise(VC_IMAGE_T *image)

FUNCTION
   Initialise all fields of a VC_IMAGE_T to 0.

RETURNS
   -
******************************************************************************/

void vc_image_initialise (VC_IMAGE_T *image) {
   vcos_assert(image != NULL);
   memset((unsigned char *)image, 0, sizeof(VC_IMAGE_T));
   image->mem_handle = MEM_INVALID_HANDLE;
}


/******************************************************************************
NAME
   vc_image_configure_unwrapped

SYNOPSIS
   int vc_image_vconfigure(VC_IMAGE_T *image, VC_IMAGE_TYPE_T type,
                           long width, long height, va_list proplist);
   int vc_image_configure_unwrapped(VC_IMAGE_T *image, VC_IMAGE_TYPE_T type,
                           long width, long height,
                           VC_IMAGE_PROPERTY_T prop, ...);

FUNCTION
   Initialise all fields of a VC_IMAGE_T, except for image_data, to proper
   values.  This sets the size element, which we may then use in a private
   malloc()-like operation.  Then we can complete the VC_IMAGE_T structure by
   calling vc_image_set_image_data().

   The remaining arguments are a VC_IMAGE_PROPERTY_T list terminated with
   VC_IMAGE_PROP_END.  Each property is followed by one argument:
      VC_IMAGE_PROP_REF_IMAGE - (VC_IMAGE_T *) copy configuration details
                                 (excluding width, height, pitch, metadata,
                                  data, and size) from another image header
      VC_IMAGE_PROP_NONE      - (long) discard next long, and do nothing
      VC_IMAGE_PROP_ROTATED   - (long) rotate coordinate flag
      VC_IMAGE_PROP_PITCH     - (long) required pitch
      VC_IMAGE_PROP_DATA      - (void *) location of pre-allocated image buffer
      VC_IMAGE_PROP_SIZE      - (long) override calculated buffer size
      VC_IMAGE_PROP_PALETTE   - (short const *) palette pointer
      VC_IMAGE_PROP_MIPMAPS   - (long) number of mipmap levels
      VC_IMAGE_PROP_BAYER_ORDER - (enum) Bayer order e.g. GRBG
      VC_IMAGE_PROP_BAYER_FORMAT - (enum) bayer format e.g. RAW10
      VC_IMAGE_PROP_BAYER_BLOCK_SIZE - (enum) block size for compressed bayer
      VC_IMAGE_PROP_CODEC_FOURCC - (enum) block size for compressed bayer
      VC_IMAGE_PROP_CODEC_MAXSIZE - (enum) max image size for compressed bayer
      VC_IMAGE_PROP_CUBEMAP   - (long) boolean
      VC_IMAGE_PROP_OPENGL_FORMAT_AND_TYPE - (long)
      VC_IMAGE_PROP_INFO      - (VC_IMAGE_INFO_T) YUV metrics and stuff
      VC_IMAGE_PROP_METADATA  - (VC_METADATA_HEADER_T *) metadata

RETURNS
   Non-zero if an argument makes no sense.
******************************************************************************/

static void imcfg_load_details(/*@inout@*/ imcfg_details_t *details, VC_IMAGE_T const *reference)
{
   switch (reference->type)
   {
   case_VC_IMAGE_ANY_TFORMAT:
      details->mipmap_levels = reference->extra.tf.mipmap_levels;
      details->cubemap = reference->extra.tf.cube_map;
      if (reference->type == VC_IMAGE_TF_PAL8 || reference->type == VC_IMAGE_TF_PAL4)
         details->palette = (const short *)reference->extra.tf.palette;
      break;

   case VC_IMAGE_8BPP: case VC_IMAGE_4BPP:
      details->palette = reference->extra.pal.palette;
      break;

   case VC_IMAGE_BAYER:
      details->bayer_order = (VC_IMAGE_BAYER_ORDER_T)reference->extra.bayer.order;
      details->bayer_format = (VC_IMAGE_BAYER_FORMAT_T)reference->extra.bayer.format;
      details->bayer_block_size = reference->extra.bayer.block_length;
      break;

   case VC_IMAGE_CODEC:
      details->codec_fourcc = reference->extra.codec.fourcc;
      details->codec_maxsize = reference->extra.codec.maxsize;
      break;

   case VC_IMAGE_OPENGL:
      details->palette = (const short *)reference->extra.opengl.palette;
      details->opengl_format_type = reference->extra.opengl.format_and_type;
      break;

   case_VC_IMAGE_ANY_RGB_NOT_TF:
      details->component_order = reference->extra.rgba.component_order;
      details->normalised_alpha = reference->extra.rgba.normalised_alpha;
      details->transparent_colour = reference->extra.rgba.transparent_colour;
      details->rgba_arg = reference->extra.rgba.arg;
      break;
   }
   details->info = reference->info;
}

static void imcfg_store_details(/*@inout@*/ VC_IMAGE_T *image, imcfg_details_t const *details)
{
   switch (image->type)
   {
   case_VC_IMAGE_ANY_TFORMAT:
      image->extra.tf.mipmap_levels = details->mipmap_levels;

      if (details->cubemap == 1)
         image->extra.tf.cube_map = 1;

      if (image->type == VC_IMAGE_TF_PAL8 || image->type == VC_IMAGE_TF_PAL4)
         image->extra.tf.palette = (void*)details->palette;  /* TODO: fix const cast */
      break;

   case VC_IMAGE_8BPP: case VC_IMAGE_4BPP:
      image->extra.pal.palette = (short *)details->palette; /* TODO: fix type cast */
      break;

   case VC_IMAGE_BAYER:
      image->extra.bayer.order = details->bayer_order;
      image->extra.bayer.format = details->bayer_format;
      image->extra.bayer.block_length = details->bayer_block_size;
      break;

   case VC_IMAGE_CODEC:
      image->extra.codec.fourcc = details->codec_fourcc;
      image->extra.codec.maxsize = details->codec_maxsize;
      break;

   case VC_IMAGE_OPENGL:
      image->extra.opengl.palette = details->palette;
      image->extra.opengl.format_and_type = details->opengl_format_type;
      break;

   case_VC_IMAGE_ANY_RGB_NOT_TF:
      image->extra.rgba.component_order = details->component_order;
      image->extra.rgba.normalised_alpha = details->normalised_alpha;
      image->extra.rgba.transparent_colour = details->transparent_colour;
      image->extra.rgba.arg = details->rgba_arg;
      break;
   }
   image->info = details->info;
}

#define RECONFIGURE_MAGIC       0x8000u
#define RECONFIGURE_MAGIC_MASK  0x8000u

// Copy a va_list of VC_IMAGE_PROPERTY_T values into an array of VC_IMAGE_PROPLIST_T's.
static int copy_stdarg_list_to_array(VC_IMAGE_PROPLIST_T *props, size_t max_props, VC_IMAGE_PROPERTY_T prop, va_list proplist)
{
   int i = 0;
   while (prop != VC_IMAGE_PROP_END)
   {
      if (!vcos_verify(i < max_props))
         return -1;
      unsigned long val = va_arg(proplist, unsigned long);
      props[i].prop = prop;
      props[i].value = val;
      i++;
      prop = va_arg(proplist, VC_IMAGE_PROPERTY_T);
   }
   return i;
}

static
int vc_image_v1configure(VC_IMAGE_T *image, unsigned long type_ex,
                                long width, long height,
                                VC_IMAGE_PROPERTY_T prop, va_list proplist)
{
   VC_IMAGE_PROPLIST_T props[32];
   int n_props = copy_stdarg_list_to_array(props, sizeof(props)/sizeof(props[0]), prop, proplist);
   if (n_props < 0)
      return n_props;

   return vc_image_configure_proplist(image, (VC_IMAGE_TYPE_T)type_ex, width, height, props, n_props);
}

static
int vc_image_configure_proplist_ex(VC_IMAGE_T *image, unsigned long type_ex,
                                   long width, long height,
                                   VC_IMAGE_PROPLIST_T *props, unsigned long n_props)
{
   VC_IMAGE_TYPE_T type = (VC_IMAGE_TYPE_T)(type_ex & ~(RECONFIGURE_MAGIC_MASK));
   void *data = NULL;
   long size = -1;
   VC_METADATA_HEADER_T *metadata = NULL;
   MEM_HANDLE_T mem_handle = MEM_INVALID_HANDLE;
   long pitch = 0, metadata_size = 0;
   imcfg_details_t details =
   {
      /* .mipmap_levels = */ 0,
      /* .cubemap = */ 0,
      /* .palette = */ NULL,
   //Bayer default parameters supplied but should usually be overwritten
      /* .bayer_order = */ VC_IMAGE_BAYER_GRBG,
      /* .bayer_format = */ VC_IMAGE_BAYER_RAW8,
      /* .bayer_block_size = */ 32,
      /* .codec_fourcc = */ 0,
      /* .codec_maxsize = */ -1,
      /* .opengl_format_type = */ 0,
      /* .info = */ { 0 },
      /* .component_order = */ 0,
      /* .normalised_alpha = */ 1,
      /* .transparent_colour = */ 0,
      /* .rgba_arg = */ 0,
   };

   vcos_assert(type > VC_IMAGE_MIN && type < VC_IMAGE_MAX);

   details.component_order = vc_image_rgb_component_order[type];

   type_ex &= RECONFIGURE_MAGIC_MASK;
   if (type_ex != 0)
   {
      vcos_assert(type_ex == RECONFIGURE_MAGIC);
      vcos_assert(vc_image_verify(image) == 0);

      /* When reconfiguring an image structure we preserve the memory buffer
       * and metadata pointer.
       */
      data = image->image_data;
      size = image->size;
      metadata = image->metadata;
      mem_handle = image->mem_handle;
      metadata_size = image->metadata_size;

      /* We also take the current configuration for whichever type it is at
       * present, and use that as the default set for the new image.  If the
       * type changes then many parameters may become redundant.
       */
      imcfg_load_details(&details, image);
   }

   uint32_t i;
   for (i=0; i<n_props; i++)
   {
      VC_IMAGE_PROPERTY_T prop = props[i].prop;
      unsigned long value = props[i].value;

      switch (prop)
      {
      case VC_IMAGE_PROP_REF_IMAGE:
         {
            VC_IMAGE_T *ref = (VC_IMAGE_T *)value;
            vcos_assert(vc_image_verify(ref) == 0);
            imcfg_load_details(&details, ref);
         }
         break;
      case VC_IMAGE_PROP_NONE:
         break;
      case VC_IMAGE_PROP_ROTATED:
         if (value != 0)
         {
            long tmp = height;
            height = width;
            width = tmp;
         }
         break;
      case VC_IMAGE_PROP_PITCH:
         pitch = (long)value;
         break;
      case VC_IMAGE_PROP_DATA:
         data = (void *)value;
         break;
      case VC_IMAGE_PROP_SIZE:
         size = (long)value;
         break;
      case VC_IMAGE_PROP_PALETTE:
         details.palette = (short const *)value;
         break;
      case VC_IMAGE_PROP_MIPMAPS:
         details.mipmap_levels = (long)value;
         break;
      case VC_IMAGE_PROP_BAYER_ORDER:
         details.bayer_order = (VC_IMAGE_BAYER_ORDER_T)value;
         break;
      case VC_IMAGE_PROP_BAYER_FORMAT:
         details.bayer_format = (VC_IMAGE_BAYER_FORMAT_T)value;
         break;
      case VC_IMAGE_PROP_BAYER_BLOCK_SIZE:
         details.bayer_block_size = (long)value;
         break;
      case VC_IMAGE_PROP_CODEC_FOURCC:
         details.codec_fourcc = (long)value;
         break;
      case VC_IMAGE_PROP_CODEC_MAXSIZE:
         details.codec_maxsize = (long)value;
         break;
      case VC_IMAGE_PROP_CUBEMAP:
         details.cubemap = (long)value;
         break;
      case VC_IMAGE_PROP_OPENGL_FORMAT_AND_TYPE:
         details.opengl_format_type = (long)value;
         break;
      case VC_IMAGE_PROP_INFO:
         details.info = *(VC_IMAGE_INFO_T*)&value;
         /* Don't ever pass in VC_IMAGE_YUVINFO_CSC_CUSTOM here.  If you want a
          * custom matrix then pass a pointer to it using a dedicated property
          * (which you may have to make up yourself if you're the first user of
          * that feature). */
         vcos_assert((details.info.info & ~VC_IMAGE_INFO_VALIDBITS) == 0);
         break;
      case VC_IMAGE_PROP_METADATA:
         metadata = (VC_METADATA_HEADER_T *)value;
         break;
      case VC_IMAGE_PROP_NEW_LIST:
#ifdef __CC_ARM
		 assert(0);
#else
         va_list proplist = (va_list)value;
         prop = va_arg(proplist, VC_IMAGE_PROPERTY_T);
         n_props = copy_stdarg_list_to_array(props, sizeof(props)/sizeof(props[0]), prop, proplist);
         i = 0;
#endif
         break;
      default: vcos_assert(!"Unexpected property value.");
         /* This will usually happen either because the wrong number of
          * arguments has been passed to an earlier property, or because you
          * have listed some image arrangement properties before memory
          * allocation properties in a call to vc_image_parmalloc().
          */
         return 1;
      }
   }

   if (type == VC_IMAGE_CODEC)
   {
      if (size <= 0 && details.codec_maxsize <= 0)
      {
         //this is an error, must specify Codec size
         return 1;
      }
      if (size < details.codec_maxsize)
         size = details.codec_maxsize;
   }

   vc_image_initialise(image);

   image->type = type;
   image->width = width;
   image->height = height;
   image->metadata = metadata;
   image->mem_handle = mem_handle;
   image->metadata_size = metadata_size;

   imcfg_store_details(image, &details);
   vc_image_set_pitch(image, pitch);

   if (size < 0)
      size = vc_image_required_size(image);

   vc_image_set_image_data(image, size, data);

   return 0;
}

int vc_image_configure_proplist(VC_IMAGE_T *image, VC_IMAGE_TYPE_T type,
                                long width, long height,
                                VC_IMAGE_PROPLIST_T *props, unsigned long n_props)
{
   return vc_image_configure_proplist_ex(image, type, width, height, props, n_props);
}

int vc_image_vconfigure(VC_IMAGE_T *image, VC_IMAGE_TYPE_T type,
                        long width, long height, va_list proplist)
{
   VC_IMAGE_PROPERTY_T prop;

   prop = va_arg(proplist, VC_IMAGE_PROPERTY_T);
   return vc_image_v1configure(image, type, width, height, prop, proplist);
}

int vc_image_configure_unwrapped(VC_IMAGE_T *image, VC_IMAGE_TYPE_T type,
                                 long width, long height,
                                 VC_IMAGE_PROPERTY_T prop, ...)
{
   va_list proplist;
   int result;

   va_start(proplist, prop);
   result = vc_image_v1configure(image, type, width, height, prop, proplist);
   va_end(proplist);

   return result;
}

/******************************************************************************
NAME
   vc_image_reconfigure_unwrapped

SYNOPSIS
   int vc_image_vreconfigure(VC_IMAGE_T *image, VC_IMAGE_TYPE_T type,
                           long width, long height, va_list proplist);
   int vc_image_reconfigure_unwrapped(VC_IMAGE_T *image, VC_IMAGE_TYPE_T type,
                           long width, long height,
                           VC_IMAGE_PROPERTY_T prop, ...);

FUNCTION
   Adjust parameters of a VC_IMAGE_T, preserving essential information such as
   image_data and size.  The image data is not adjusted in any way, and will
   most likely be meaningless after this call.

   Overriding the image data pointer or size fields will not result in
   reallocation of the memory, and should not be done if the image data was
   originally allocated by vc_image.

RETURNS
   Non-zero if an argument makes no sense.
******************************************************************************/

int vc_image_vreconfigure(VC_IMAGE_T *image, VC_IMAGE_TYPE_T type,
                        long width, long height, va_list proplist)
{
   VC_IMAGE_PROPERTY_T prop;

   prop = va_arg(proplist, VC_IMAGE_PROPERTY_T);
   return vc_image_v1configure(image, (RECONFIGURE_MAGIC | type), width, height, prop, proplist);
}

int vc_image_reconfigure_unwrapped(VC_IMAGE_T *image, VC_IMAGE_TYPE_T type,
                                 long width, long height,
                                 VC_IMAGE_PROPERTY_T prop, ...)
{
   va_list proplist;
   int result;

   va_start(proplist, prop);
   result = vc_image_v1configure(image, (RECONFIGURE_MAGIC | type), width, height, prop, proplist);
   va_end(proplist);

   return result;
}


/******************************************************************************
NAME
   vc_image_fill

SYNOPSIS
   void vc_image_fill(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height, int value)

FUNCTION
   Fill given region of image with solid colour value.

RETURNS
   void
******************************************************************************/
void vc_image_fill (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height, int value) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));

   CLIP_DEST(x_offset, y_offset, width, height, dest->width, dest->height);

   if (width <= 0 || height <= 0) return;
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height, 0, 0, dest->width, dest->height));

   switch (dest->type) {
#if defined(USE_RGB565) || defined(USE_RGBA565) || defined(USE_RGBA16)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      vc_image_fill_rgb565(dest, x_offset, y_offset, width, height, value);
      break;
#endif
#ifdef USE_BGR888
   case VC_IMAGE_BGR888:  //Can't do pitched fill on BGR888_NP because of the pitch
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_fill_rgb888(dest, x_offset, y_offset, width, height, value);
      break;
#endif
#ifdef USE_1BPP
   case VC_IMAGE_1BPP:
      vc_image_fill_1bpp(dest, x_offset, y_offset, width, height, value);
      break;
#endif
#ifdef USE_RGBA32
   case VC_IMAGE_RGBA32:
   case VC_IMAGE_RGBX32:
   case VC_IMAGE_ARGB8888:
   case VC_IMAGE_XRGB8888:
      vc_image_fill_rgba32(dest, x_offset, y_offset, width, height, value);
      break;
#endif
#ifdef USE_4BPP
   case VC_IMAGE_4BPP:
      // works, but does anyone need it?
      //vc_image_fill_4bpp(dest, x_offset, y_offset, width, height, value);
      //break;
#endif
#ifdef USE_YUV420
   case VC_IMAGE_YUV420:
      vc_image_fill_yuv420(dest, x_offset, y_offset, width, height, value);
      break;
#endif
#ifdef USE_YUV422
   case VC_IMAGE_YUV422:
      vc_image_fill_yuv422(dest, x_offset, y_offset, width, height, value);
      break;
#endif
#ifdef USE_YUV422PLANAR
   case VC_IMAGE_YUV422PLANAR:
      vc_image_fill_yuv422(dest, x_offset, y_offset, width, height, value);
      break;
#endif
#ifdef USE_8BPP
   case VC_IMAGE_8BPP:
      vc_image_fill_8bpp(dest, x_offset, y_offset, width, height, value);
      break;
#endif
   default:
      // All other image types currently unsupported.
      vcos_assert(!"Unsupported type for vc_image_fill()");
   }
}

/******************************************************************************
NAME
   vc_image_blt

SYNOPSIS
   int vc_image_blt(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                    VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset)

FUNCTION
   Blt from src bitmap to destination.

RETURNS
   void
******************************************************************************/

void vc_image_blt (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                   VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset) {
   int d;
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, dest->type));

   /* Now clip to screen coordinates */
   if (x_offset<0)
   {
      d=-x_offset;
      x_offset+=d;
      src_x_offset+=d;
      width-=d;
   }
   if (y_offset<0)
   {
      d=-y_offset;
      y_offset+=d;
      src_y_offset+=d;
      height-=d;
   }
   if (x_offset+width>dest->width)
   {
      d=x_offset+width-dest->width;
      width-=d;
   }
   if (y_offset+height>dest->height)
   {
      d=y_offset+height-dest->height;
      height-=d;
   }
   if (src_x_offset+width>src->width)
   {
      d=src_x_offset+width-src->width;
      width-=d;
   }
   if (src_y_offset+height>src->height)
   {
      d=src_y_offset+height-src->height;
      height-=d;
   }
   if (width<=0)
      return;
   if (height<=0)
      return;

   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height, 0, 0, dest->width, dest->height));
   vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height, 0, 0, src->width, src->height));

   switch (dest->type) {
      // Even 888 builds need 565 to 565 blt.
#if defined(USE_RGB565) || defined(USE_RGBA565) || defined(USE_RGBA16)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      vc_image_blt_rgb565(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
      break;
#endif
#ifdef USE_RGBA32
   case VC_IMAGE_RGBA32:
   case VC_IMAGE_RGBX32:
   case VC_IMAGE_ARGB8888:
   case VC_IMAGE_XRGB8888:
      vc_image_blt_rgba32(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
      break;
#endif
#ifdef USE_RGB888
#ifdef USE_BGR888
   case VC_IMAGE_BGR888:
#endif
   case VC_IMAGE_RGB888:
      vc_image_blt_rgb888(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
      break;
#endif
#ifdef USE_YUV422
   case VC_IMAGE_YUV422:
      /* TODO: not this */
      vc_image_resize_yuv(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset, width, height, 0);
      break;
#endif
#ifdef USE_YUV422PLANAR
   case VC_IMAGE_YUV422PLANAR:
      vc_image_blt_yuv(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
      break;
#endif
#ifdef USE_YUV420
   case VC_IMAGE_YUV420:
      vc_image_blt_yuv(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
      break;
#endif
#ifdef USE_4BPP
   case VC_IMAGE_4BPP:
      vc_image_blt_4bpp(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
      break;
#endif
#ifdef USE_1BPP
   case VC_IMAGE_1BPP:
      if (x_offset == 0 && y_offset == 0)
      {
         vc_image_copy_1bpp_src_offset(dest, width, height, src, src_x_offset, src_y_offset);
         break;
      }
      else if (src_x_offset == 0 && src_y_offset == 0)
      {
         vc_image_copy_1bpp_dst_offset(dest, x_offset, y_offset, src, width, height);
         break;
      }
#if 0 // maybe not -- contention for the tmp image buffer
      else // XXX: horrendous, but at least works
      {
         VC_IMAGE_T *tmp = vc_image_alloc_tmp_image(VC_IMAGE_1BPP, width, height);
         vc_image_copy_1bpp_src_offset(tmp, width, height, src, src_x_offset, src_y_offset);
         vc_image_copy_1bpp_dst_offset(dest, x_offset, y_offset, tmp, width, height);
         vc_image_free_tmp_image(tmp);
      }
      break;
#endif /* 0 */
#endif
      // DROP THROUGH
   default:
      // All other image types currently unsupported.
      vcos_assert(!"Unsupported image type for vc_image_blt()");
   }
}

/******************************************************************************
NAME
   vc_image_convert_blt

SYNOPSIS
   int vc_image_convert_blt(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                    VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset)

FUNCTION
   Blt from src bitmap to destination changing the format on the fly.

RETURNS
   void
******************************************************************************/

void vc_image_convert_blt (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                           VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset) {
   int d;
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));

   if (src->type == dest->type)
   {
      vc_image_blt(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
      return;
   }

   /* Now clip to screen coordinates */
   if (x_offset<0)
   {
      d=-x_offset;
      x_offset+=d;
      src_x_offset+=d;
      width-=d;
   }
   if (y_offset<0)
   {
      d=-y_offset;
      y_offset+=d;
      src_y_offset+=d;
      height-=d;
   }
   if (x_offset+width>dest->width)
   {
      d=x_offset+width-dest->width;
      width-=d;
   }
   if (y_offset+height>dest->height)
   {
      d=y_offset+height-dest->height;
      height-=d;
   }
   if (src_x_offset+width>src->width)
   {
      d=src_x_offset+width-src->width;
      width-=d;
   }
   if (src_y_offset+height>src->height)
   {
      d=src_y_offset+height-src->height;
      height-=d;
   }
   if (width<=0)
      return;
   if (height<=0)
      return;


   if (src->width == width && src->height == height && dest->width == width && dest->height == height)
   {
      vc_image_convert(dest, src, 0, 0);
      return;
   }

   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height, 0, 0, dest->width, dest->height));
   vcos_assert(is_within_rectangle(src_x_offset, src_x_offset, width, height, 0, 0, src->width, src->height));

   switch (dest->type)
   {
#ifdef USE_TFORMAT
   case VC_IMAGE_TF_RGBA32:
   case VC_IMAGE_TF_RGBX32:
      switch (src->type)
      {
      case VC_IMAGE_YUV420:
         vc_image_yuv420_to_tf_rgba32_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
         vc_image_rgba32_to_tf_rgba32_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
      default:
         vcos_assert(!"Unsupported type for blt/convert to TF_RGBA32.");
      }
      break;
#endif

#ifdef USE_TFORMAT
   case VC_IMAGE_TF_RGB565:
      switch (src->type)
      {
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vc_image_rgb565_to_tf_rgb565_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
      default:
         vcos_assert(!"Unsupported type for blt/convert to TF_RGB565.");
      }
      break;
#endif

#ifdef USE_RGBA32
   case VC_IMAGE_RGBA32:
      switch (src->type)
      {
#ifdef USE_YUV422
      case VC_IMAGE_YUV422:
         vc_image_yuv422_to_rgba32_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset, RGBA_ALPHA_OPAQUE);
         break;
#endif
#ifdef USE_YUV420
      case VC_IMAGE_YUV420:
         vc_image_yuv420_to_rgba32_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset, RGBA_ALPHA_OPAQUE);
         break;
#endif
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vc_image_rgb565_to_rgba32_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset, RGBA_ALPHA_OPAQUE);
         break;
#endif
#ifdef USE_RGB888
      case VC_IMAGE_RGB888:
      {
         VC_IMAGE_BUF_T *tmpbuf;
         /* We don't have a conversion function from RGB888 to RGBA32 so have to convert to RGB565 first */
         tmpbuf = vc_image_malloc(VC_IMAGE_RGB565, width, height, 0);
         if (vcos_verify(tmpbuf != NULL)) {
            vc_image_convert_blt(tmpbuf, 0, 0, width, height, src, src_x_offset, src_y_offset);
            vc_image_convert_blt(dest, x_offset, y_offset, width, height, tmpbuf, 0, 0);
            vc_image_free(tmpbuf);
         }
         break;
      }
#endif
#if defined(USE_YUV_UV)
      case VC_IMAGE_YUV_UV:
      {
         VC_IMAGE_BUF_T *tmpbuf;
         /* We don't have a conversion function from YUV_UV to RGB565 so have to convert to YUV420 first */
         tmpbuf = vc_image_malloc(VC_IMAGE_YUV420, width, height, 0);
         if (vcos_verify(tmpbuf != NULL)) {
            vc_image_convert_blt(tmpbuf, 0, 0, width, height, src, src_x_offset, src_y_offset);
            vc_image_convert_blt(dest, x_offset, y_offset, width, height, tmpbuf, 0, 0);
            vc_image_free(tmpbuf);
         }
         break;
      }
#endif
#ifdef USE_TFORMAT
   case VC_IMAGE_TF_RGBA32:
   case VC_IMAGE_TF_RGBX32:
         vc_image_tf_rgba32_to_rgba32_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
      default:
         vcos_assert(!"Unsupported source type for blt/convert to RGBA32.");
      }
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      switch (src->type)
      {
#ifdef USE_YUV422
      case VC_IMAGE_YUV422:
         vc_image_yuv422_to_rgb888_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
#ifdef USE_YUV420
      case VC_IMAGE_YUV420:
         vc_image_yuv420_to_rgb888_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
#ifdef USE_RGBA16
      case VC_IMAGE_RGBA16:
         vc_image_rgba16_to_rgb888_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
#ifdef USE_RGBA565
      case VC_IMAGE_RGBA565:
         vc_image_rgba565_to_rgb888_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
#ifdef USE_8BPP
      case VC_IMAGE_8BPP:
         vc_image_8bpp_to_rgb888_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset, vc_image_get_palette(src));
         break;
#endif
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vc_image_rgb565_to_rgb888_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
#ifdef USE_3D32
      case VC_IMAGE_3D32:
         vc_image_3d32_to_rgb888_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset, vc_image_get_palette(src));
         break;
      case VC_IMAGE_3D32B:
         vc_image_3d32b_to_rgb888_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif

#ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
      {
         VC_IMAGE_BUF_T *tmpbuf;
         /* We don't have a conversion function from RGB888 to RGBA32 so have to convert to RGB565 first */
         tmpbuf = vc_image_malloc(VC_IMAGE_RGB565, width, height, 0);
         if (vcos_verify(tmpbuf != NULL)) {
            vc_image_convert_blt(tmpbuf, 0, 0, width, height, src, src_x_offset, src_y_offset);
            vc_image_convert_blt(dest, x_offset, y_offset, width, height, tmpbuf, 0, 0);
            vc_image_free(tmpbuf);
         }
         break;
      }
#endif
#if defined(USE_YUV_UV)
      case VC_IMAGE_YUV_UV:
      {
         VC_IMAGE_BUF_T *tmpbuf;
         /* We don't have a conversion function from YUV_UV to RGB565 so have to convert to YUV420 first */
         tmpbuf = vc_image_malloc(VC_IMAGE_YUV420, width, height, 0);
         if (vcos_verify(tmpbuf != NULL)) {
            vc_image_convert_blt(tmpbuf, 0, 0, width, height, src, src_x_offset, src_y_offset);
            vc_image_convert_blt(dest, x_offset, y_offset, width, height, tmpbuf, 0, 0);
            vc_image_free(tmpbuf);
         }
         break;
      }
#endif

      default:
         vcos_assert(!"Unsupported source type for blt/convert to RGB888.");
      }
      break;
#endif
#ifdef USE_RGBA565
   case VC_IMAGE_RGBA565:
      switch (src->type)
      {
#ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
         vc_image_rgba32_to_rgba565_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
      default:
         vcos_assert(!"Unsupported source type for blt/convert to RGBA565.");
      }
      break;
#endif
#ifdef USE_RGB565
   case VC_IMAGE_RGB565:
      switch (src->type)
      {
#ifdef USE_RGBA16
      case VC_IMAGE_RGBA16:
         vc_image_rgba16_to_rgb565_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
#ifdef USE_RGBA565
      case VC_IMAGE_RGBA565:
         vc_image_rgba565_to_rgb565_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
#ifdef USE_YUV422
      case VC_IMAGE_YUV422:
         vc_image_yuv422_to_rgb565_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
#ifdef USE_YUV420
      case VC_IMAGE_YUV420:
         vc_image_yuv420_to_rgb565_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
#ifdef USE_8BPP
      case VC_IMAGE_8BPP:
         vc_image_8bpp_to_rgb565_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset, vc_image_get_palette(src));
         break;
#endif
#ifdef USE_3D32
      case VC_IMAGE_3D32:
         vc_image_3d32_to_rgb565_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset, vc_image_get_palette(src));
         break;
      case VC_IMAGE_3D32B:
         vc_image_3d32b_to_rgb565_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
#ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
         vc_image_overlay_rgba32_to_rgb565(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset, RGBA_ALPHA_OPAQUE);
         break;
#endif
# ifdef USE_RGB888
      case VC_IMAGE_RGB888:
         vc_image_convert_to_16bpp(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
#ifdef USE_TFORMAT
      case VC_IMAGE_TF_RGB565:
         vc_image_tf_rgb565_to_rgb565_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
#ifdef USE_YUV_UV
      case VC_IMAGE_YUV_UV:
      {
         VC_IMAGE_BUF_T *tmpbuf;
         /* We don't have a conversion function from YUV_UV to RGB565 so have to convert to YUV420 first */
         tmpbuf = vc_image_malloc(VC_IMAGE_YUV420, width, height, 0);
         if (vcos_verify(tmpbuf != NULL)) {
            vc_image_convert_blt(tmpbuf, 0, 0, width, height, src, src_x_offset, src_y_offset);
            vc_image_convert_blt(dest, x_offset, y_offset, width, height, tmpbuf, 0, 0);
            vc_image_free(tmpbuf);
         }
         break;
      }
#endif
      default:
         vcos_assert(!"Unsupported source type for blt/convert to RGB565.");
      }
      break;
#endif

#ifdef USE_YUV420
   case VC_IMAGE_YUV420:
      switch (src->type)
      {
#ifdef USE_YUV_UV
      case VC_IMAGE_YUV_UV:
         vc_image_yuvuv128_to_yuv420_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vc_image_convert_rgb2yuv((unsigned short *)((char *)src->image_data+src_y_offset*src->pitch)+src_x_offset, GET_YUV_Y(dest, x_offset, y_offset),
                  GET_YUV_U(dest, x_offset, y_offset), GET_YUV_V(dest, x_offset, y_offset), src->pitch >> 1, dest->pitch, width, height);
         break;
#endif
#ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
      {
         VC_IMAGE_BUF_T *tmpbuf;
         /* We don't have a conversion function from RGBA32 to YUV420, so have to convert to 16bit RGB first */
         tmpbuf = vc_image_malloc(VC_IMAGE_RGB565, width, height, 0);
         if (vcos_verify(tmpbuf != NULL)) {
            vc_image_convert_blt(tmpbuf, 0, 0, width, height, src, src_x_offset, src_y_offset);
            vc_image_convert_rgb2yuv(tmpbuf->image_data, GET_YUV_Y(dest, x_offset, y_offset),
                  GET_YUV_U(dest, x_offset, y_offset), GET_YUV_V(dest, x_offset, y_offset), tmpbuf->pitch >> 1, dest->pitch, width, height);
            vc_image_free(tmpbuf);
         }
         break;
      }
#endif
#ifdef USE_RGB888
      case VC_IMAGE_RGB888:
      {
         VC_IMAGE_BUF_T *tmpbuf;
         /* We don't have a conversion function from RGB888 to YUV420, so have to convert to 16bit RGB first */
         tmpbuf = vc_image_malloc(VC_IMAGE_RGB565, width, height, 0);
         if (vcos_verify(tmpbuf != NULL)) {
            vc_image_convert_blt(tmpbuf, 0, 0, width, height, src, src_x_offset, src_y_offset);
            vc_image_convert_rgb2yuv(tmpbuf->image_data, GET_YUV_Y(dest, x_offset, y_offset),
                  GET_YUV_U(dest, x_offset, y_offset), GET_YUV_V(dest, x_offset, y_offset), tmpbuf->pitch >> 1, dest->pitch, width, height);
            vc_image_free(tmpbuf);
         }
         break;
      }
#endif
      default:
         vcos_assert(!"Unsupported source type for blt/convert to YUV420.");
      }
      break;
#endif //converting YUV_UV to YUV420

#ifdef USE_YUV_UV
   case VC_IMAGE_YUV_UV:
      switch (src->type)
      {
#ifdef USE_YUV420
      case VC_IMAGE_YUV420:
         vc_image_yuvuv128_from_yuv420_part(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
         break;
#endif
#ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
      {
         VC_IMAGE_BUF_T *tmpbuf;
         /* We don't have a conversion function from RGBA32 to YUVA32 so have to convert to YUV420 first */
         tmpbuf = vc_image_malloc(VC_IMAGE_YUV420, width, height, 0);
         if (vcos_verify(tmpbuf != NULL)) {
            vc_image_convert_blt(tmpbuf, 0, 0, width, height, src, src_x_offset, src_y_offset);
            vc_image_convert_blt(dest, x_offset, y_offset, width, height, tmpbuf, 0, 0);
            vc_image_free(tmpbuf);
         }
         break;
      }
#endif
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
      {
         VC_IMAGE_BUF_T *tmpbuf;
         /* We don't have a conversion function from RGBA32 to YUVA32 so have to convert to YUV420 first */
         tmpbuf = vc_image_malloc(VC_IMAGE_YUV420, width, height, 0);
         if (vcos_verify(tmpbuf != NULL)) {
            vc_image_convert_blt(tmpbuf, 0, 0, width, height, src, src_x_offset, src_y_offset);
            vc_image_convert_blt(dest, x_offset, y_offset, width, height, tmpbuf, 0, 0);
            vc_image_free(tmpbuf);
         }
         break;
      }
#endif
#ifdef USE_RGB888
      case VC_IMAGE_RGB888:
      {
         VC_IMAGE_BUF_T *tmpbuf;
         /* We don't have a conversion function from RGBA32 to YUVA32 so have to convert to YUV420 first */
         tmpbuf = vc_image_malloc(VC_IMAGE_YUV420, width, height, 0);
         if (vcos_verify(tmpbuf != NULL)) {
            vc_image_convert_blt(tmpbuf, 0, 0, width, height, src, src_x_offset, src_y_offset);
            vc_image_convert_blt(dest, x_offset, y_offset, width, height, tmpbuf, 0, 0);
            vc_image_free(tmpbuf);
         }
         break;
      }
#endif
      default:
         vcos_assert(!"Unsupported source type for blt/convert to YUVUV.");
      }
      break;
#endif //converting YUV420 to YUV_UV



   default:
      // All other image types currently unsupported.
      vcos_assert(!"Can't convert and blt dest->type");
   }
}

/******************************************************************************
NAME
   vc_image_transparent_blt

SYNOPSIS
   int vc_image_transparent_blt(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                                VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset, int transparent_colour)

FUNCTION
   Blt from src bitmap to destination with a nominated "transparent" colour.

RETURNS
   void
******************************************************************************/

void vc_image_transparent_blt (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                               VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset, int transparent_colour) {
   int d;
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, dest->type));

   /* Now clip to screen coordinates */
   if (x_offset<0)
   {
      d=-x_offset;
      x_offset+=d;
      src_x_offset+=d;
      width-=d;
   }
   if (y_offset<0)
   {
      d=-y_offset;
      y_offset+=d;
      src_y_offset+=d;
      height-=d;
   }
   if (x_offset+width>dest->width)
   {
      d=x_offset+width-dest->width;
      width-=d;
   }
   if (y_offset+height>dest->height)
   {
      d=y_offset+height-dest->height;
      height-=d;
   }
   if (src_x_offset+width>src->width)
   {
      d=src_x_offset+width-src->width;
      width-=d;
   }
   if (src_y_offset+height>src->height)
   {
      d=src_y_offset+height-src->height;
      height-=d;
   }
   if (width<=0)
      return;
   if (height<=0)
      return;
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height, 0, 0, dest->width, dest->height));
   vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height, 0, 0, src->width, src->height));
   switch (dest->type) {
#ifdef USE_RGB565
   case VC_IMAGE_RGB565:
      vc_image_transparent_blt_rgb565(dest, x_offset, y_offset, width, height,
                                      src, src_x_offset, src_y_offset, transparent_colour);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_transparent_blt_rgb888(dest, x_offset, y_offset, width, height,
                                      src, src_x_offset, src_y_offset, transparent_colour);
      break;
#endif
   default:
      // All other image types currently unsupported.
      vcos_assert(!"Unsupported image type for vc_image_transpose_blt()");
   }
}

/******************************************************************************
NAME
   vc_image_overwrite_blt

SYNOPSIS
   int vc_image_overwrite_blt(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                                VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset, int overwrite_colour)

FUNCTION
   Blt from src bitmap to destination with a nominated "transparent" colour.

RETURNS
   void
******************************************************************************/

void vc_image_overwrite_blt (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                             VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset, int overwrite_colour) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, dest->type));
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height, 0, 0, dest->width, dest->height));
   vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height, 0, 0, src->width, src->height));

   switch (dest->type) {
#ifdef USE_RGB565
   case VC_IMAGE_RGB565:
      vc_image_overwrite_blt_rgb565(dest, x_offset, y_offset, width, height,
                                    src, src_x_offset, src_y_offset, overwrite_colour);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_overwrite_blt_rgb888(dest, x_offset, y_offset, width, height,
                                    src, src_x_offset, src_y_offset, overwrite_colour);
      break;
#endif
   default:
      // All other image types currently unsupported.
      vcos_assert(!"Unsupported image type for vc_image_overwrite_blt()");
   }
}

/******************************************************************************
NAME
   vc_image_masked_blt

SYNOPSIS
   int vc_image_masked_blt(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                           VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset,
                           VC_IMAGE_BUF_T *mask, int mask_x_offset, mask_y_offset,
                           int invert)

FUNCTION
   Blt from src bitmap to destination where the mask has 1s (if invert is zero).
   If invert is non-zero blt to pixels in destination where mask has 0s.

RETURNS
   void
******************************************************************************/

void vc_image_masked_blt (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                          VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset,
                          VC_IMAGE_BUF_T *mask, int mask_x_offset, int mask_y_offset,
                          int invert) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, dest->type));
   vcos_assert(is_valid_vc_image_buf(mask, mask->type)); /* VC_IMAGE_1BPP? */
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height, 0, 0, dest->width, dest->height));
   vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height, 0, 0, src->width, src->height));
   vcos_assert(is_within_rectangle(mask_x_offset, mask_y_offset, width, height, 0, 0, mask->width, mask->height));

   switch (dest->type) {
#if defined(USE_RGB565) || defined(USE_RGBA565) || defined(USE_RGBA16)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      vc_image_masked_blt_rgb565(dest, x_offset, y_offset, width, height,
                                 src, src_x_offset, src_y_offset,
                                 mask, mask_x_offset, mask_y_offset, invert);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_masked_blt_rgb888(dest, x_offset, y_offset, width, height,
                                 src, src_x_offset, src_y_offset,
                                 mask, mask_x_offset, mask_y_offset, invert);
      break;
#endif
   default:
      // All other image types currently unsupported.
      vcos_assert(!"Unsupported image type for vc_image_basked_blt()");
   }
}

/******************************************************************************
NAME
   vc_image_masked_fill

SYNOPSIS
   void vc_image_masked_fill(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height, int value
                             VC_IMAGE_BUF_T *mask, int mask_x_offset, int mask_y_offset, int invert)

FUNCTION
   Masked fill. Fill pixels in dest where mask bits are 1. If invert is non-zero,
   fill pixels in dest where mask bits are 0.

RETURNS
   void
******************************************************************************/

void vc_image_masked_fill (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height, int value,
                           VC_IMAGE_BUF_T *mask, int mask_x_offset, int mask_y_offset, int invert) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(mask, VC_IMAGE_1BPP));
   CLIP_DEST(x_offset, y_offset, width, height, dest->width, dest->height);
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height, 0, 0, dest->width, dest->height));
   vcos_assert(is_within_rectangle(mask_x_offset, mask_y_offset, width, height, 0, 0, mask->width, mask->height));

   switch (dest->type) {
#if defined(USE_RGB565) || defined(USE_RGBA565) || defined(USE_RGBA16)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      vc_image_masked_fill_rgb565(dest, x_offset, y_offset, width, height, value, mask, mask_x_offset, mask_y_offset, invert);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_masked_fill_rgb888(dest, x_offset, y_offset, width, height, value, mask, mask_x_offset, mask_y_offset, invert);
      break;
#endif
   default:
      // All other image types currently unsupported.
      vcos_assert(!"Image type not supported");
   }
}

/******************************************************************************
NAME
   vc_image_fetch

SYNOPSIS
   void vc_image_fetch(VC_IMAGE_BUF_T *src, unsigned char* Y, unsigned char* U, unsigned char* V)

FUNCTION
   Copy the image data out to 3 separate Y U V buffers, or R G B if the image
   type is RGB888. Only YUV420, RGB888 / RGB565 are supported at the moment.

RETURNS
   void
*******************************************************************************/

void vc_image_fetch(VC_IMAGE_BUF_T *pimage, unsigned char* Y, unsigned char* U, unsigned char* V)
{
   long pitch = pimage->pitch;
   unsigned char* y, *u, *v; /* pointer to YUV or RGB buffers */

   vcos_assert(is_valid_vc_image_buf(pimage, pimage->type));

   /* Pointer to Y/R and U/G buffer will always be the same */
   /* only that to V/B will differ depending on whether it is V or G */
   y = vc_image_get_y(pimage);
   u = vc_image_get_u(pimage);
   v = vc_image_get_v(pimage);

   switch (pimage->type){
#ifdef USE_YUV420
   case VC_IMAGE_YUV420:
      /* Copy the Y component */
      vclib_memcpy(Y, y, pitch * pimage->height);

      /* U,V component are only half as high */
      pitch >>= 1;
      vclib_memcpy(U, u, pitch * (pimage->height >> 1));
      vclib_memcpy(V, v, pitch * (pimage->height >> 1));

      break;
#endif
   case VC_IMAGE_RGB888:
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      /* Copy all three components at the same time */
      /* RGB data is stored as RGBRGBRGB...
         only the Y pointer is needed */
      vclib_memcpy(Y, y, pimage->height * pitch);
      break;

   default:
      vcos_assert(!"Image type not supported");
   }


}

/******************************************************************************
NAME
   vc_image_put

SYNOPSIS
   void vc_image_put(VC_IMAGE_BUF_T *dest, unsigned char* Y, unsigned char* U, unsigned char* V)

FUNCTION
   Copy the image data from 3 separate Y U V buffers, or R G B if the image
   type is RGB888 to the VC_IMAGE_BUF_T struct. Only YUV420, RGB888 / RGB565 are supported at the moment.

RETURNS
   void
*******************************************************************************/

void vc_image_put(VC_IMAGE_BUF_T *pimage, unsigned char* Y, unsigned char* U, unsigned char* V)
{
   long pitch = pimage->pitch;
   unsigned char* y, *u, *v; /* pointer to YUV or RGB buffers */

   vcos_assert(is_valid_vc_image_buf(pimage, pimage->type));

   /* Pointer to Y/R and U/G buffer will always be the same */
   /* only that to V/B will differ depending on whether it is V or G */
   y = vc_image_get_y(pimage);
   u = vc_image_get_u(pimage);
   v = vc_image_get_v(pimage);

   switch (pimage->type){
#ifdef USE_YUV420
   case VC_IMAGE_YUV420:
      /* Copy the Y component */
      /* Data is arranged as YYYYYYYY... UUUUU... VVVVV.... */
      vclib_memcpy(y, Y, pimage->height * pitch);
      /* U,V component are only half as high */
      pitch >>= 1;

      vclib_memcpy(u, U, (pimage->height >> 1) * pitch);
      vclib_memcpy(v, V, (pimage->height >> 1) * pitch);
      break;
#endif
   case VC_IMAGE_RGB888:
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:

      /* Copy all three components at the same time */
      /* RGB data is arraged as RGBRGBRGB... so only
         the Y pointer is needed */
      vclib_memcpy(y, Y, pimage->height * pitch);
      break;


   default:
      vcos_assert(!"Image type not supported");
   }


}

/******************************************************************************
NAME
   vc_image_not

SYNOPSIS
   void vc_image_not(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height)

FUNCTION
   Not given region of image.

RETURNS
   void
******************************************************************************/

void vc_image_not (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height, 0, 0, dest->width, dest->height));

   switch (dest->type) {
#ifdef USE_RGB565
   case VC_IMAGE_RGB565:
      vc_image_not_rgb565(dest, x_offset, y_offset, width, height);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_not_rgb888(dest, x_offset, y_offset, width, height);
      break;
#endif
   default:
      // All other image types currently unsupported.
      vcos_assert(!"Image type not supported");
   }
}

/******************************************************************************
NAME
   vc_image_fade

SYNOPSIS
   void vc_image_fade(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height, int fade)

FUNCTION
   Fade given region of image. fade is an 8.8 value, so > 256 brightens the image region.

RETURNS
   void
******************************************************************************/

void vc_image_fade (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height, int fade) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height, 0, 0, dest->width, dest->height));

   CLIP_DEST(x_offset, y_offset, width, height, dest->width, dest->height);

   switch (dest->type) {
#ifdef USE_RGB565
   case VC_IMAGE_RGB565:
      vc_image_fade_rgb565(dest, x_offset, y_offset, width, height, fade);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_fade_rgb888(dest, x_offset, y_offset, width, height, fade);
      break;
#endif
   default:
      // All other image types currently unsupported.
      vcos_assert(!"Image type not supported");
   }
}

/******************************************************************************
NAME
   vc_image_or

SYNOPSIS
   void vc_image_or(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                    VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset)

FUNCTION
   OR two bitmaps together.

RETURNS
   void
******************************************************************************/

void vc_image_or (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                  VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, dest->type));
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height, 0, 0, dest->width, dest->height));

   switch (dest->type) {
#ifdef USE_RGB565
   case VC_IMAGE_RGB565:
      vc_image_or_rgb565(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_or_rgb888(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
      break;
#endif
   default:
      // All other image types currently unsupported.
      vcos_assert(!"Image type not supported");
   }
}

/******************************************************************************
NAME
   vc_image_xor

SYNOPSIS
   void vc_image_xor(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                     VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset)

FUNCTION
   XOR two bitmaps togther.

RETURNS
   void
******************************************************************************/

void vc_image_xor (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                   VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, dest->type));
   vcos_assert(is_within_rectangle(x_offset, y_offset, width, height, 0, 0, dest->width, dest->height));
   vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height, 0, 0, src->width, src->height));

   switch (dest->type) {
#ifdef USE_RGB565
   case VC_IMAGE_RGB565:
      vc_image_xor_rgb565(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_xor_rgb888(dest, x_offset, y_offset, width, height, src, src_x_offset, src_y_offset);
      break;
#endif
   default:
      // All other image types currently unsupported.
      vcos_assert(!"Image type not supported");
   }
}

/******************************************************************************
NAME
   vc_image_copy

SYNOPSIS
   void vc_image_copy(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src)

FUNCTION
   Copy a whole image.

RETURNS
   void
******************************************************************************/

void vc_image_copy (VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src) {
   int width_nblocks = 0;   // number of 32-byte wide blocks to code
   int height_nblocks;  // number of 16-deep rows to copy
   unsigned char *s, *d;
   int src_pitch, dest_pitch;

   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));

   // save the destination's mem_handle and pool, as vc_image_reconfigure()
   // resets them
   struct opaque_vc_pool_object_s *pool = dest->pool_object;
   MEM_HANDLE_T h                       = dest->mem_handle;

   vc_image_reconfigure(dest, (VC_IMAGE_TYPE_T)src->type, src->width, src->height,
         VC_IMAGE_PROP_REF_IMAGE, src,
         /* TODO: figure out why the original code preserved dest->pitch but
          * overwrote dest->type */
         VC_IMAGE_PROP_PITCH, dest->pitch);

   dest->mem_handle  = h;
   dest->pool_object = pool;

   switch (dest->type) {
#ifdef USE_RGBA32
   case VC_IMAGE_RGBA32:
      width_nblocks = ((src->width+15)&(~15))>>3;
      break;
#endif
#ifdef USE_TFORMAT
   case VC_IMAGE_TF_RGBA32:
      width_nblocks = ((src->width+15)&(~15))>>3;
      break;
#endif
#if defined(USE_RGB565) || defined(USE_RGBA565) || defined(USE_RGBA16)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      width_nblocks = ((src->width+15)&(~15))>>4;
      break;
#endif
#ifdef USE_1BPP
   case VC_IMAGE_1BPP:
      // use this to allow more efficient packing of overlay mask - so that it can be placed in int. mem.
      width_nblocks = ((src->width+31)&(~31))>>5;  // read 'num_bytes'
      height_nblocks = src->height;
      s = src->image_data;
      d = dest->image_data;
      for (; height_nblocks; height_nblocks--, s += src->pitch, d += dest->pitch) {
         memcpy(d, s, width_nblocks);
      }
      return;
#endif
#ifdef USE_8BPP
   case VC_IMAGE_8BPP:
      width_nblocks = ((src->width+15)&(~15))>>5;
      break;
#endif
#ifdef USE_4BPP
   case VC_IMAGE_4BPP:
      // Not massively efficient, but adequate for now.
   {
      int i;
      for (i = 0; i < src->height; i++)
         vclib_memcpy((unsigned char *)dest->image_data+i*dest->pitch,
                      (unsigned char *)src->image_data+i*src->pitch,
                      (src->width+1)>>1);
   }
   return;
#endif
#if defined(USE_RGB888) || defined(USE_YUV420)
#ifdef USE_BGR888
   case VC_IMAGE_BGR888:
#endif
   case VC_IMAGE_RGB888:
   case VC_IMAGE_YUV422PLANAR:
   case VC_IMAGE_YUV422:
   case VC_IMAGE_YUV420:
      // Could produce something more efficient than this, but it'll do.
      vc_image_blt(dest, 0, 0, src->width, src->height, src, 0, 0);
      return;
#endif

#ifdef USE_YUV_UV
   case VC_IMAGE_YUV_UV:
      {
         int x;

         s = vc_image_get_y(src);
         d = vc_image_get_y(dest);
         src_pitch = src->pitch;
         dest_pitch = dest->pitch;
         for (x = 0; x < src->width; x += VC_IMAGE_YUV_UV_STRIPE_WIDTH)
         {
            vclib_memcpy(d, s, src->height * VC_IMAGE_YUV_UV_STRIPE_WIDTH);
            s += src_pitch;
            d += dest_pitch;
         }

         s = vc_image_get_u(src);
         d = vc_image_get_u(dest);
         for (x = 0; x < src->width; x += VC_IMAGE_YUV_UV_STRIPE_WIDTH)
         {
            vclib_memcpy(d, s, src->height * (VC_IMAGE_YUV_UV_STRIPE_WIDTH >> 1));
            s += src_pitch;
            d += dest_pitch;
         }
      }
      return;
   case VC_IMAGE_YUV_UV32:
      {
         int x;

         s = vc_image_get_y(src);
         d = vc_image_get_y(dest);
         src_pitch = src->pitch;
         dest_pitch = dest->pitch;
         for (x = 0; x < src->width; x += 32)
         {
            vclib_memcpy(d, s, src->height * 32);
            s += src_pitch;
            d += dest_pitch;
         }

         s = vc_image_get_u(src);
         d = vc_image_get_u(dest);
         for (x = 0; x < src->width; x += 32)
         {
            vclib_memcpy(d, s, src->height * (32 >> 1));
            s += src_pitch;
            d += dest_pitch;
         }
      }
      return;
#endif

   default:
      vcos_assert(!"Image type not supported");
   }
   height_nblocks = ((src->height+15)&(~15))>>4;
   s = (unsigned char *)src->image_data;
   d = (unsigned char *)dest->image_data;
   src_pitch = src->pitch;
   dest_pitch = dest->pitch;
   for (; height_nblocks; height_nblocks--, s += src_pitch<<4, d += dest_pitch<<4) {
      vc_image_copy_stripe(d, dest_pitch, s, src_pitch, width_nblocks);
   }
}

/******************************************************************************
NAME
   vc_image_transpose

SYNOPSIS
   void vc_image_transpose(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src)

FUNCTION
   Transpose whole image.

RETURNS
   void
******************************************************************************/
void vc_image_transpose (VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src) {
   int ret = vc_image_transpose_ret(dest,src);
   vcos_assert(ret == 0);
}

int vc_image_transpose_ret (VC_IMAGE_T *dest, VC_IMAGE_T *src) {
   int supported = 0;
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));
   vc_image_set_type(dest, (VC_IMAGE_TYPE_T)src->type);
   vc_image_set_dimensions(dest, src->height, src->width);
   vc_image_set_pitch(dest, dest->pitch);
   vcos_assert(dest->size >= vc_image_required_size(dest));

   switch (dest->type) {
#if defined(USE_RGB565) || defined(USE_RGBA565) || defined(USE_RGBA16)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      vc_image_transpose_rgb565(dest, src);
      break;
#endif
#ifdef USE_1BPP
   case VC_IMAGE_1BPP:
      vc_image_transpose_1bpp(dest, src);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_transpose_rgb888(dest, src);
      break;
#endif
#ifdef USE_RGBA32
   case VC_IMAGE_RGBA32:
   case VC_IMAGE_RGBX32:
   case VC_IMAGE_ARGB8888:
   case VC_IMAGE_XRGB8888:
       vc_image_transpose_rgba32(dest, src);
       break;
#endif
#ifdef USE_4BPP
   case VC_IMAGE_4BPP:
      vc_image_transpose_4bpp(dest, src);
      break;
#endif
#ifdef USE_YUV420
   case VC_IMAGE_YUV420:
      vc_image_transpose_yuv420(dest, src);
      break;
#endif
#ifdef USE_8BPP
   case VC_IMAGE_8BPP:
      vc_image_transpose_8bpp(dest,src);
      break;
#endif
#ifdef USE_YUV_UV
   case VC_IMAGE_YUV_UV:
      vc_image_transpose_yuvuv128(dest, src);
      break;
#endif
#ifdef USE_TFORMAT
   case VC_IMAGE_TF_RGBA16:
   case VC_IMAGE_TF_RGB565:
   case VC_IMAGE_TF_RGBA5551:
      vc_image_transpose_tf_rgb565(dest, src);
      supported = 1; // set flag to indicate that tf transpose has opposite sense (is around the other diagonal)
      break;
   case VC_IMAGE_TF_RGBA32:
   case VC_IMAGE_TF_RGBX32:
      vc_image_transpose_tf_rgba32(dest, src);
      supported = 1; // set flag to indicate that tf transpose has opposite sense (is around the other diagonal)
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
      supported = -1;
   }
   return supported;
}

/******************************************************************************
NAME
   vc_image_transpose_tiles

SYNOPSIS
   void vc_image_transpose_tiles(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, int hdeci)

FUNCTION
   Transpose all 16x16 tiles in whole image.

RETURNS
   void
******************************************************************************/

void vc_image_transpose_tiles(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, int hdeci) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));
   vc_image_set_type(dest, (VC_IMAGE_TYPE_T)src->type);
   vc_image_set_dimensions(dest, src->width, src->height);
   vc_image_set_pitch(dest, dest->pitch);
   vcos_assert(dest->size >= vc_image_required_size(dest));

   switch (dest->type) {
#ifdef USE_RGBA32
   case VC_IMAGE_RGBA32:
      vc_image_transpose_tiles_rgba32(dest, src, hdeci);
      break;
#endif
#if defined(USE_RGB565) || defined(USE_RGBA565) || defined(USE_RGBA16)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      vc_image_transpose_tiles_rgb565(dest, src, hdeci);
      break;
#endif
#ifdef USE_YUV420
   case VC_IMAGE_YUV420:
      vc_image_transpose_tiles_yuv420(dest, src, hdeci);
      break;
#endif
#ifdef USE_8BPP
   case VC_IMAGE_8BPP:
      vc_image_transpose_tiles_8bpp(dest, src, hdeci);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
   }
}

/******************************************************************************
NAME
   vc_image_transpose_and_subsample

SYNOPSIS
   void vc_image_transpose_and_subsample(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, int factor)

FUNCTION
   Transpose all 16x16 tiles in whole image.

RETURNS
   void
******************************************************************************/

void vc_image_transpose_and_subsample(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, int factor)
{
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));

   if (factor == 1)
   {
      vc_image_transpose(dest, src);
      return;
   }
   vcos_assert(factor==2);

   vc_image_set_type(dest, (VC_IMAGE_TYPE_T)src->type);
   vc_image_set_dimensions(dest, src->height / factor, src->width / factor);
   vc_image_set_pitch(dest, dest->pitch);
   vcos_assert(dest->size >= vc_image_required_size(dest));

   /* check that info was inited sensibly; you probably wanted zero here ... */
   vcos_assert((src->info.info & ~VC_IMAGE_INFO_VALIDBITS) == 0);
   vcos_assert((dest->info.info & ~VC_IMAGE_INFO_VALIDBITS) == 0);

   switch (dest->type)
   {
#ifdef USE_YUV_UV
   case VC_IMAGE_YUV_UV:
      vc_image_transpose_and_subsample_yuvuv128(dest, src);
      break;
#endif

   default:
      vcos_assert(!"Decimated transpose image type not supported.");
   }
}

/******************************************************************************
NAME
   vc_image_vflip

SYNOPSIS
   void vc_image_vflip(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src)

FUNCTION
   Flip whole image vertically (turn "upside down").

RETURNS
   void
******************************************************************************/

void vc_image_vflip (VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));
   vc_image_set_type(dest, (VC_IMAGE_TYPE_T)src->type);
   vc_image_set_dimensions(dest, src->width, src->height);
   vc_image_set_pitch(dest, dest->pitch);
   vcos_assert(dest->size >= vc_image_required_size(dest));

   switch (dest->type) {
#ifdef USE_RGBA32
   case VC_IMAGE_RGBA32:
   case VC_IMAGE_RGBX32:
   case VC_IMAGE_ARGB8888:
   case VC_IMAGE_XRGB8888:
      vc_image_vflip_rgba32(dest, src);
      break;
#endif
#if defined(USE_RGB565) || defined(USE_RGBA565) || defined(USE_RGBA16)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      vc_image_vflip_rgb565(dest, src);
      break;
#endif
#ifdef USE_1BPP
   case VC_IMAGE_1BPP:
      vc_image_vflip_1bpp(dest, src);
      break;
#endif
   case VC_IMAGE_RGB888:
   case VC_IMAGE_4BPP:
   case VC_IMAGE_8BPP:
   case VC_IMAGE_YUV420:
   case VC_IMAGE_YUV_UV:
   case VC_IMAGE_YUV_UV32:
      vc_image_copy(dest, src);
      vc_image_vflip_in_place(dest);
      break;
   default:
      vcos_assert(!"Image type not supported");
   }
}

/******************************************************************************
NAME
   vc_image_hflip

SYNOPSIS
   void vc_image_hflip(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src)

FUNCTION
   Flip whole image horizontally ("mirror").

RETURNS
   void
******************************************************************************/

void vc_image_hflip (VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));
   vc_image_set_type(dest, (VC_IMAGE_TYPE_T)src->type);
   vc_image_set_dimensions(dest, src->width, src->height);
   vc_image_set_pitch(dest, dest->pitch);
   vcos_assert(dest->size >= vc_image_required_size(dest));

   switch (dest->type) {
#ifdef USE_RGBA32
       case VC_IMAGE_RGBA32:
       case VC_IMAGE_RGBX32:
       case VC_IMAGE_ARGB8888:
       case VC_IMAGE_XRGB8888:
      vc_image_hflip_rgba32(dest, src);
      break;
#endif
#if defined(USE_RGB565) || defined(USE_RGBA565) || defined(USE_RGBA16)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      vc_image_hflip_rgb565(dest, src);
      break;
#endif
#ifdef USE_1BPP
   case VC_IMAGE_1BPP:
      vc_image_hflip_1bpp(dest, src);
      break;
#endif
   case VC_IMAGE_RGB888:
   case VC_IMAGE_4BPP:
   case VC_IMAGE_8BPP:
   case VC_IMAGE_YUV420:
   case VC_IMAGE_YUV_UV:
      vc_image_copy(dest, src);
      vc_image_hflip_in_place(dest);
      break;
   default:
      vcos_assert(!"Image type not supported");
   }
}

/******************************************************************************
NAME
   vc_image_transpose_in_place

SYNOPSIS
   void vc_image_transpose_place(VC_IMAGE_BUF_T *dest)

FUNCTION
   Transpose whole image in place. Pitch and size of image must be large enough
   for this to work - the pitch does not change in this operation.

RETURNS
   void
******************************************************************************/

void vc_image_transpose_in_place (VC_IMAGE_BUF_T *dest) {
   int width = dest->width;
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(dest->size >= ((width+15)&(~15))*dest->pitch);

   switch (dest->type) {
#if defined(USE_RGB565) || defined(USE_RGBA565) || defined(USE_RGBA16)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      vc_image_transpose_in_place_rgb565(dest);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_transpose_in_place_rgb888(dest);
      break;
#endif
   case VC_IMAGE_1BPP:
      // Not implemented yet.
   default:
      vcos_assert(!"Image type not supported");
   }

   dest->width = dest->height;
   dest->height = width;
}

/******************************************************************************
NAME
   vc_image_vflip_in_place

SYNOPSIS
   void vc_image_vflip_in_place(VC_IMAGE_BUF_T *dest)

FUNCTION
   Flip whole image vertically (turn "upside down") in place.

RETURNS
   void
******************************************************************************/

void vc_image_vflip_in_place (VC_IMAGE_BUF_T *dest) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));

   switch (dest->type) {
#if defined(USE_RGB565) || defined(USE_RGBA565) || defined(USE_RGBA16)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      vc_image_vflip_in_place_rgb565(dest);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_vflip_in_place_rgb888(dest);
      break;
#endif
#ifdef USE_RGBA32
   case VC_IMAGE_RGBA32:
   case VC_IMAGE_RGBX32:
   case VC_IMAGE_ARGB8888:
   case VC_IMAGE_XRGB8888:
      vc_image_vflip_rgba32(dest, dest);
      break;
#endif
#ifdef USE_YUV420
   case VC_IMAGE_YUV420:
      vc_image_vflip_in_place_yuv420(dest);
      break;
#endif
#ifdef USE_1BPP
   case VC_IMAGE_1BPP:
      vc_image_vflip_in_place_1bpp(dest);
      break;
#endif
#ifdef USE_8BPP
   case VC_IMAGE_8BPP:
      vc_image_vflip_in_place_8bpp(dest);
      break;
#endif
#ifdef USE_4BPP
   case VC_IMAGE_4BPP:
      vc_image_vflip_in_place_4bpp(dest);
      break;
#endif
#ifdef USE_YUV_UV
   case VC_IMAGE_YUV_UV:
   case VC_IMAGE_YUV_UV32:
      vc_image_vflip_in_place_yuvuv128(dest);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
   }
}

/******************************************************************************
NAME
   vc_image_hflip_in_place

SYNOPSIS
   void vc_image_hflip_in_place(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src)

FUNCTION
   Flip whole image horizontally ("mirror") in place.

RETURNS
   void
******************************************************************************/

void vc_image_hflip_in_place (VC_IMAGE_BUF_T *dest) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));

   switch (dest->type) {
#if defined(USE_RGB565) || defined(USE_RGBA565) || defined(USE_RGBA16)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      vc_image_hflip_in_place_rgb565(dest);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_hflip_in_place_rgb888(dest);
      break;
#endif
#ifdef USE_RGBA32
   case VC_IMAGE_RGBA32:
      vc_image_hflip_rgba32(dest, dest);
      break;
#endif
#ifdef USE_YUV420
   case VC_IMAGE_YUV420:
      vc_image_hflip_in_place_yuv420(dest);
      break;
#endif
#ifdef USE_1BPP
   case VC_IMAGE_1BPP:
      vc_image_hflip_in_place_1bpp(dest);
      break;
#endif
#ifdef USE_4BPP
   case VC_IMAGE_4BPP:
      vc_image_hflip_in_place_4bpp(dest);
      break;
#endif
#ifdef USE_8BPP
   case VC_IMAGE_8BPP:
      vc_image_hflip_in_place_8bpp(dest);
      break;
#endif
#ifdef USE_YUV_UV
   case VC_IMAGE_YUV_UV:
      vc_image_hflip_in_place_yuvuv128(dest);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
   }
}

/******************************************************************************
NAME
   vc_image_text

SYNOPSIS
   void vc_image_text(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                      int *max_x, int *max_y)

FUNCTION
   Write text to an image. Uses a fixed internal font and clips it to the image if
   it overflows. bg_colour may be specified as a colour value, or
   VC_IMAGE_COLOUR_TRANSPARENT to mean the backgroud shows through unchanged all
   around, or VC_IMAGE_COLOUR_FADE to preserve the background, but to fade it slightly.
   Returns the maximum x and y coordinates written to.

RETURNS
   void
******************************************************************************/

void vc_image_text (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                    int *max_x, int *max_y)
{
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));

   switch (dest->type)
   {
#if defined(USE_RGB565) || defined(USE_RGBA565)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
      vc_image_text_rgb565(dest, x_offset, y_offset, fg_colour, bg_colour, text, max_x, max_y);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_text_rgb888(dest, x_offset, y_offset, fg_colour, bg_colour, text, max_x, max_y);
      break;
#endif
#ifdef USE_RGBA32
   case VC_IMAGE_RGBA32:
      vc_image_text_rgba32(dest, x_offset, y_offset, fg_colour, bg_colour, text, max_x, max_y);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
   }
}

void vc_image_small_text (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                          int *max_x, int *max_y) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));

   switch (dest->type) {
#if defined(USE_RGB565) || defined(USE_RGBA565)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
      vc_image_small_text_rgb565(dest, x_offset, y_offset, fg_colour, bg_colour, text, max_x, max_y);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_small_text_rgb888(dest, x_offset, y_offset, fg_colour, bg_colour, text, max_x, max_y);
      break;
#endif
#ifdef USE_8BPP
   case VC_IMAGE_8BPP:
      vc_image_small_text_8bpp(dest, x_offset, y_offset, fg_colour, bg_colour, text, max_x, max_y);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
   }
}

void vc_image_text_32 (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                       int *max_x, int *max_y) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));

   switch (dest->type) {
#if defined(USE_RGB565) || defined(USE_RGBA565)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
      vc_image_text_32_rgb565(dest, x_offset, y_offset, fg_colour, bg_colour, text, max_x, max_y);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_text_32_rgb888(dest, x_offset, y_offset, fg_colour, bg_colour, text, max_x, max_y);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
   }
}

void vc_image_text_24 (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                       int *max_x, int *max_y) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));

   switch (dest->type) {
#if defined(USE_RGB565) || defined(USE_RGBA565)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
      vc_image_text_24_rgb565(dest, x_offset, y_offset, fg_colour, bg_colour, text, max_x, max_y);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_text_24_rgb888(dest, x_offset, y_offset, fg_colour, bg_colour, text, max_x, max_y);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
   }
}

void vc_image_text_20 (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                       int *max_x, int *max_y) {
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));

   switch (dest->type) {
#if defined(USE_RGB565) || defined(USE_RGBA565)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
      vc_image_text_20_rgb565(dest, x_offset, y_offset, fg_colour, bg_colour, text, max_x, max_y);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_text_20_rgb888(dest, x_offset, y_offset, fg_colour, bg_colour, text, max_x, max_y);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
   }
}


/******************************************************************************
NAME
   vc_image_textrotate

SYNOPSIS
   void vc_image_textrotate(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text, int rotate)

FUNCTION
   Write optionally rotated text to an image. Uses a fixed internal font and clips it to the image if
   it overflows. bg_colour may be specified as a colour value, or
   VC_IMAGE_COLOUR_TRANSPARENT to mean the backgroud shows through unchanged all
   around, or VC_IMAGE_COLOUR_FADE to preserve the background, but to fade it slightly.
   The coordinates are set so that each letter is at a fixed y location.
   If using a portrait screen then set <rotate> to 1.

RETURNS
   void
******************************************************************************/

void vc_image_textrotate (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int fg_colour, int bg_colour, const char *text,
                          int rotate)
{
#if defined(USE_RGB565) || defined(USE_RGBA565) || defined(USE_RGB888)
   int max_x,max_y;
#endif
   vcos_assert(vclib_check_VRF());

   vcos_assert(is_valid_vc_image_buf(dest, dest->type));

   switch (dest->type) {
#if defined(USE_RGB565) || defined(USE_RGBA565)
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
      if (rotate)
      {
         vclib_textwrite(dest->image_data,dest->pitch>>1,x_offset,y_offset,dest->width,dest->height,fg_colour,bg_colour,16,text);
      }
      else
      {
         vc_image_text_rgb565(dest, x_offset, y_offset, fg_colour, bg_colour, text, &max_x, &max_y);
      }
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_text_rgb888(dest, x_offset, y_offset, fg_colour, bg_colour, text, &max_x, &max_y);
      break;
#endif
#ifdef USE_YUV420
   case VC_IMAGE_YUV420:
      vc_image_text_yuv (dest, x_offset, y_offset, fg_colour, bg_colour, text,rotate);
      break;
#endif
#ifdef USE_8BPP
   case VC_IMAGE_8BPP:
      vc_image_text_yuv (dest, x_offset, y_offset, fg_colour, bg_colour, text,rotate);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
   }
}


/******************************************************************************
NAME
   vc_image_line

SYNOPSIS
   void vc_image_line(VC_IMAGE_BUF_T *dest, int x_start, int y_start, int x_end, int y_end, int fg_colour)

FUNCTION
   Draw line from (x_start,y_start) to (x_end,y_end). Coordinate must be clipped.

RETURNS
   void
******************************************************************************/

#define swap(a, b) do {int t = a; a = b; b = t;} while (0)
void vc_image_line (VC_IMAGE_BUF_T *dest, int x_start, int y_start, int x_end, int y_end, int fg_colour) {
   int dx, dy, gradient;

   vcos_assert(is_valid_vc_image_buf(dest, dest->type));

   if (abs(x_start-x_end) < abs(y_start-y_end)) { // nearer vertical
      if (x_start == x_end) {
         vc_image_line_vert(dest, x_start, y_start, x_end, y_end, fg_colour);
         return;
      }
      dx = y_end - y_start;
      if (dx <= 0) {
         if (dx == 0)
            return;
         swap(x_start, x_end);
         swap(y_start, y_end);
         dx = -dx;
      }
      dy = x_end - x_start;
      gradient = (dy<<16)/dx;
      vc_image_line_y(dest, y_start, y_end, x_start, gradient, fg_colour);
   } else {        // nearer horizontal
      if (y_start == y_end) {
         vc_image_line_horiz(dest, x_start, y_start, x_end, y_end, fg_colour);
         return;
      }
      dx = x_end - x_start;
      if (dx <= 0) {
         if (dx == 0)
            return;
         swap(x_start, x_end);
         swap(y_start, y_end);
         dx = -dx;
      }
      dy = y_end - y_start;
      gradient = (dy<<16)/dx;
      vc_image_line_x(dest, x_start, x_end, y_start, gradient, fg_colour);
   }
}



/******************************************************************************
NAME
   vc_image_frame

SYNOPSIS
   void vc_image_frame(VC_IMAGE_BUF_T *dest, int x_start, int y_start, int width, int height)

FUNCTION
   Draw 2-pixel thick frame in black/white/grey.

RETURNS
   void
******************************************************************************/

void vc_image_frame(VC_IMAGE_BUF_T *dest, int x_start, int y_start, int width, int height)
{
   const unsigned short lightgrey=0xc618, darkgrey=0x8410, white=0xffff, black=0x0000;

   vcos_assert(is_valid_vc_image_buf(dest, dest->type));

   // outer top and right
   vc_image_line(dest, x_start+0, y_start+0, x_start+width-1, y_start+0, lightgrey);
   vc_image_line(dest, x_start+width-1, y_start+1, x_start+width-1, y_start+height-2, lightgrey);
   // outer bottom and left
   vc_image_line(dest, x_start+0, y_start+height-1, x_start+width-1, y_start+height-1, black);
   vc_image_line(dest, x_start+0, y_start+1, x_start+0, y_start+height-1, black);

   // inner top and right
   vc_image_line(dest, x_start+1, y_start+1, x_start+width-2, y_start+1, white);
   vc_image_line(dest, x_start+width-2, y_start+2, x_start+width-2, y_start+height-3, white);
   // inner bottom and left
   vc_image_line(dest, x_start+1, y_start+height-2, x_start+width-2, y_start+height-2, darkgrey);
   vc_image_line(dest, x_start+1, y_start+2, x_start+1, y_start+height-3, darkgrey);
}

void vc_image_rect(VC_IMAGE_BUF_T *dest, int x_start, int y_start, int width, int height, int fg_colour)
{
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));

   // outer top and right
   vc_image_line(dest, x_start+0, y_start+0, x_start+width-1, y_start+0, fg_colour);
   vc_image_line(dest, x_start+width-1, y_start+0, x_start+width-1, y_start+height-1, fg_colour);
   // outer bottom and left
   vc_image_line(dest, x_start+0, y_start+height-1, x_start+width-1, y_start+height-1, fg_colour);
   vc_image_line(dest, x_start+0, y_start+0, x_start+0, y_start+height-1, fg_colour);
}


/******************************************************************************
NAME
   vc_image_resize

SYNOPSIS
   void vc_image_resize(VC_IMAGE_BUF_T *dest,
                         int dest_x_offset, int dest_y_offset,
                         int dest_width, int dest_height,
                         VC_IMAGE_BUF_T *src,
                         int src_x_offset, int src_y_offset,
                         int src_width, int src_height,
                         int smooth_flag);

FUNCTION
   Generalised resize function.

RETURNS
   void
******************************************************************************/

void vc_image_resize(VC_IMAGE_BUF_T *dest,
                     int dest_x_offset, int dest_y_offset,
                     int dest_width, int dest_height,
                     VC_IMAGE_BUF_T *src,
                     int src_x_offset, int src_y_offset,
                     int src_width, int src_height,
                     int smooth_flag)
{
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));

   switch (dest->type)
   {
   default:
      vcos_assert(!"Can't resize dest->type");
      break;

#ifdef USE_8BPP
   case VC_IMAGE_8BPP:
      vcos_assert(smooth_flag == 0);
      vc_image_resize_bytes(
         (unsigned char *)dest->image_data + dest->pitch * dest_y_offset + dest_x_offset,
         dest_width, dest_height, dest->pitch,
         (unsigned char *)src->image_data + src->pitch * src_y_offset + src_x_offset,
         src_width, src_height, src->pitch,
         smooth_flag);
      break;
#endif

#if defined(USE_RGBA565)
   case VC_IMAGE_RGBA565:
      vcos_assert(smooth_flag == 0);
      smooth_flag = 0;
      /*@fallthrough@*/
#endif
#if defined(USE_RGB565)
   case VC_IMAGE_RGB565:
      vc_image_resize_rgb565(
         dest, dest_x_offset, dest_y_offset, dest_width, dest_height,
         src, src_x_offset, src_y_offset, src_width, src_height,
         smooth_flag);
      break;
#endif

#if defined(USE_YUV420) || defined(USE_YUV422)
   case VC_IMAGE_YUV420:
   case VC_IMAGE_YUV422:
      vc_image_resize_yuv(
         dest, dest_x_offset, dest_y_offset, dest_width, dest_height,
         src, src_x_offset, src_y_offset, src_width, src_height,
         smooth_flag);
      break;
#endif
   }
}


/******************************************************************************
NAME
   vc_image_convert_to_48bpp

SYNOPSIS
   void vc_image_convert_to_48bpp(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                                  VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset)

FUNCTION
   Convert region of one bitmap to 48bpp and copy into a 48bpp destination.
   WARNING: all offsets, width and height MUST be 16-pixel aligned.

RETURNS
   void
******************************************************************************/

void vc_image_convert_to_48bpp (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                                VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset) {
   vcos_assert(is_valid_vc_image_buf(dest, VC_IMAGE_48BPP));
   vcos_assert(is_valid_vc_image_buf(src, src->type));

   vcos_assert((x_offset&15) == 0 && (y_offset&15) == 0 && (width&15) == 0 &&
          (height&15) == 0 && (src_x_offset&15) == 0 && (src_y_offset&15) == 0);
   switch (src->type) {
#ifdef USE_RGB565
   case VC_IMAGE_RGB565:
      vc_image_convert_to_48bpp_rgb565(dest, x_offset, y_offset, width, height,
                                       src, src_x_offset, src_y_offset);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
   }
}

/******************************************************************************
NAME
   vc_image_convert

SYNOPSIS
   void vc_image_convert(VC_IMAGE_BUF_T *dest,VC_IMAGE_BUF_T *src,int mirror,int rotate)


Function
   Blit from a YUV image to an RGB image with optional mirroring followed
   by optional clockwise rotation. WARNING: some image type may not support
   mirrored/rotated modes.

RETURNS
   void
******************************************************************************/

#ifdef USE_3D32
static int ambient;
static int specular;
void vc_image_set_ambient(int a)
{
   ambient = a;
}
void vc_image_set_specular(int s)
{
   specular = s;
}
#endif

void vc_image_convert(VC_IMAGE_BUF_T *dest,VC_IMAGE_BUF_T *src,int mirror,int rotate)
{
//   vcos_assert(vclib_check_VRF());
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));

   if (src->type == dest->type && !mirror && !rotate)
   {
      vc_image_copy(dest, src);
      return;
   }


   switch ( dest->type )
   {

#ifdef USE_RGB2X9
   case VC_IMAGE_RGB2X9:
      switch ( src->type )
      {
# ifdef USE_RGB565
      case VC_IMAGE_RGB565:
      {
         int i;
         for ( i = 0; i < src->height; i+= 16 )
            vc_rgb565_to_rgb2x9( ((unsigned char*)dest->image_data + i*dest->pitch),
                                 ((unsigned char*)src->image_data  + i*src->pitch),
                                 dest->pitch, src->pitch, dest->width, vc_image_get_palette(src) );
      }
      break;
# endif

#ifdef USE_8BPP
      case VC_IMAGE_8BPP:
      {
         int xoffset=(src->width-dest->width)>>1;
         int i;
         vcos_assert(xoffset>=0);

         for ( i = 0; i < src->height; i+= 16 )
            vc_palette8_to_rgb2x9( ((unsigned char*)dest->image_data + i*dest->pitch),
                                   ((unsigned char*)src->image_data  + i*src->pitch + xoffset),
                                   dest->pitch, src->pitch, dest->width, vc_image_get_palette(src) );
      }
      break;
# endif
      default:
         vcos_assert(!"Invalid src->type for RGB2X9 dest->type!");
         break;
      }
      break;
#endif




#ifdef USE_RGB565
   case VC_IMAGE_RGB565:
      switch ( src->type )
      {
#ifdef USE_YUV420
      case VC_IMAGE_YUV420:
         // We need this case even with rgb888 displays because our interface to a
         // host processor is always 16-bit, and the imviewer application needs this.
         vc_image_convert_yuv_rgb565(dest, src, mirror, rotate);
         break;
#endif
#ifdef USE_YUV422
      case VC_IMAGE_YUV422:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_convert_yuv422_rgb565(dest, src);
         break;
#endif
#ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgba32_to_rgb565(dest, src);
         break;
#endif
#ifdef USE_RGBA16
      case VC_IMAGE_RGBA16:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgba16_to_rgb565_part(dest, 0, 0, dest->width, dest->height, src, 0, 0);
         break;
#endif
# ifdef USE_3D32

      case VC_IMAGE_3D32MAT:
         vc_3d32mat_to_rgb565( dest->image_data, src->image_data, dest->width*src->height, vc_image_get_palette(src), ambient, specular>>4 );
         break;
      case VC_IMAGE_3D32B:
         if (dest->width==src->width)
            vc_3d32b_to_rgb565( dest->image_data, src->image_data, dest->width*src->height, vc_image_get_palette(src) );
         else
            vc_3d32b_to_rgb565_anti2( dest->image_data, src->image_data, dest->width*src->height, vc_image_get_palette(src) );
         break;
      case VC_IMAGE_3D32:
         if (dest->width==src->width)
            vc_3d32_to_rgb565( dest->image_data, src->image_data, dest->width*src->height, vc_image_get_palette(src) );
         else if (dest->width*2==src->width)
            vc_3d32_to_rgb565_anti2( dest->image_data, src->image_data, dest->width*src->height, vc_image_get_palette(src) );
         else
            vcos_assert(!"Image type not supported");
         break;
# endif

#ifdef USE_8BPP
      case VC_IMAGE_8BPP:
      {
         int i;
         for ( i = 0; i < src->height; i+= 16 )
            vc_palette8_to_rgb565( ((unsigned char*)dest->image_data + i*dest->pitch),
                                   ((unsigned char*)src->image_data   + i*src->pitch),
                                   /* pitch of dest */ dest->pitch, /* ignored */dest->width, vc_image_get_palette(src), /* pitch of source */src->pitch );
      }
      break;
#endif
# ifdef USE_RGB888
      case VC_IMAGE_RGB888:
         vc_image_convert_to_16bpp(dest, 0, 0, dest->width, dest->height, src, 0, 0);
         break;
#endif

#ifdef USE_TFORMAT
      case VC_IMAGE_TF_RGBA32:
      case VC_IMAGE_TF_RGBX32:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_tf_rgba32_to_rgb565(dest, src);
         break;
      case VC_IMAGE_TF_RGB565:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_tf_rgb565_to_rgb565(dest, src);
         break;
#endif

#ifdef USE_YUV_UV
      case VC_IMAGE_YUV_UV:
         vc_image_yuv_uv_to_rgb565(dest, src);
         break;
#endif
      default:
         vcos_assert(!"Invalid src->type for dest->type of RBG565!");
         break;
      }
      break;
#else
      // WARNING: VMCS imviewer application STILL NEEDS YUV TO RGB565!!!!
      // Also needs RGB888 to RGB565.
   case VC_IMAGE_RGB565:
      switch ( src->type )
      {
#ifdef USE_YUV420
      case VC_IMAGE_YUV420:
         // We need this case even with rgb888 displays because our interface to a
         // host processor is always 16-bit, and the imviewer application needs this.
         void vc_image_convert_yuv_rgb565(VC_IMAGE_BUF_T *dest,VC_IMAGE_BUF_T *src,int mirror,int rotate);
         vc_image_convert_yuv_rgb565(dest, src, mirror, rotate);
         break;
#endif
      case VC_IMAGE_RGB888:
         vc_image_convert_to_16bpp(dest, 0, 0, dest->width, dest->height, src, 0, 0);
         break;
      default:
         vcos_assert(!"Image type not supported");
         break;
      }
      break;
#endif

#ifdef USE_YUV420
      /* Converting to YUV420 type, warning, NO rotation or mirror supported atm */
   case VC_IMAGE_YUV420:
      switch ( src->type )
      {
#ifdef USE_YUV422
      case VC_IMAGE_YUV422:
         vc_image_resize_yuv(dest, 0, 0, dest->width, dest->height, src, 0, 0, src->width, src->height, 0);
         break;
#endif
      case VC_IMAGE_RGB565:
         vc_image_convert_rgb2yuv(src->image_data, vc_image_get_y(dest), vc_image_get_u(dest), vc_image_get_v(dest),
                               src->pitch >> 1, dest->pitch, dest->width, dest->height);
         break;

      case VC_IMAGE_BGR888:
      case VC_IMAGE_RGB888:
      case VC_IMAGE_RGBA32:
      {
         vc_image_rgb_to_yuv(dest, src);
         break;
      }
#ifdef USE_YUV_UV
      case VC_IMAGE_YUV_UV:

         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_yuvuv128_to_yuv420(dest, src);

         break;
#endif //converting YUV_UV to YUV420

      default:
         vcos_assert(!"Image type not supported");
         break;
      }
      break;
#endif


#ifdef USE_YUV422
   case VC_IMAGE_YUV422:
      switch (src->type)
      {
#ifdef USE_YUV420
      case VC_IMAGE_YUV420:
         vc_image_resize_yuv(dest, 0, 0, dest->width, dest->height, src, 0, 0, src->width, src->height, 0);
         break;
#endif
      default:
         vcos_assert(!"Invalid src->type for YUV422 dest->type!");
      }
      break;
#endif

#ifdef USE_YUV422PLANAR
   case VC_IMAGE_YUV422PLANAR:
      switch (src->type)
      {
      case VC_IMAGE_RGBA32:
      case VC_IMAGE_RGB888:
      case VC_IMAGE_BGR888:
      case VC_IMAGE_RGB565:
      {
         vc_image_rgb_to_yuv(dest, src);
         break;
      }
      default:
         vcos_assert(!"Invalid src->type for YUV422PLANAR dest->type!");
         break;
      }
      break;
#endif

#ifdef USE_RGBA565
   case VC_IMAGE_RGBA565:
      switch (src->type)
      {
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgb565_to_rgba565(dest, src);
         break;
#endif
#ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgba32_to_rgba565(dest, src);
         break;
#endif
      default:
         vcos_assert(!"Invalid src->type for RGBA565 dest->type!");
      }
      break;
#endif

#ifdef USE_RGBA16
   case VC_IMAGE_RGBA16:
      switch (src->type)
      {
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgb565_to_rgba16(dest, src);
         break;
#endif
#ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgba32_to_rgba16(dest, src);
         break;
#endif
      default:
         vcos_assert(!"Invalid src->type for RGBA16 dest->type!");
      }
      break;
#endif


#ifdef USE_RGBA32
   case VC_IMAGE_RGBA32:
   case VC_IMAGE_ARGB8888:
      switch (src->type)
      {
#ifdef USE_YUV420
      case VC_IMAGE_YUV420:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_yuv420_to_rgba32(dest, src, RGBA_ALPHA_OPAQUE);
         break;
#endif
#ifdef USE_YUV422
      case VC_IMAGE_YUV422:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_yuv422_to_rgba32(dest, src, RGBA_ALPHA_OPAQUE);
         break;
#endif
#ifdef USE_YUV_UV
      case VC_IMAGE_YUV_UV:
         VC_IMAGE_BUF_T *tmpbuf;
         /* We don't have a conversion function from YUV_UV to RGBA32
            so have to convert to YUV420 RGB first */
         tmpbuf = vc_image_malloc(VC_IMAGE_YUV420, src->width, src->height, 0);
         if (vcos_verify(tmpbuf != NULL)) {
            vc_image_convert(tmpbuf, src, mirror, rotate);
            vc_image_convert(dest, tmpbuf, mirror, rotate);
            vc_image_free(tmpbuf);
         }
         break;
#endif
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgb565_to_rgba32(dest, src, RGBA_ALPHA_OPAQUE, -1);
         break;
#endif
#ifdef USE_RGBA565
      case VC_IMAGE_RGBA565:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgba565_to_rgba32(dest, src);
         break;
#endif
#ifdef USE_RGBA16
      case VC_IMAGE_RGBA16:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgba16_to_rgba32(dest, src);
         break;
#endif
#ifdef USE_RGB888
      case VC_IMAGE_RGB888:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgb888_to_rgba32(dest, src, RGBA_ALPHA_OPAQUE);
         break;
#endif
#ifdef USE_BGR888
      case VC_IMAGE_BGR888:  /* This is intended for opengl */
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_bgr888_to_rgba32(dest, src, 0);
         break;
#endif
#ifdef USE_4BPP
      case VC_IMAGE_4BPP:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_palette4_to_rgba32(dest, src, vc_image_get_palette(src), RGBA_ALPHA_OPAQUE, -1);
         break;
#endif

#ifdef USE_TFORMAT
      case VC_IMAGE_TF_RGBX32:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_tf_rgbx32_to_rgba32(dest, src);
         break;
      case VC_IMAGE_TF_RGBA32:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_tf_rgba32_to_rgba32(dest, src, 0);
         break;
#endif

      default:
         vcos_assert(!"Invalid src->type for RGBA32 dest->type!");
      }
      break;
#endif

#ifdef USE_TFORMAT
   case VC_IMAGE_TF_ETC1:
      switch (src->type)
      {
      case VC_IMAGE_OPENGL:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_opengl_to_tf_etc1(dest, src, 0);
         break;
#ifdef USE_YUV420
      case VC_IMAGE_YUV420:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_yuv420_to_tf_etc1(dest, src, 0);
         break;
#endif
#ifdef USE_YUV422
      case VC_IMAGE_YUV422:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_yuv422_to_tf_etc1(dest, src, 0);
         break;
#endif
#ifdef USE_YUV422PLANAR
      case VC_IMAGE_YUV422PLANAR:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_yuv422p_to_tf_etc1(dest, src, 0);
         break;
#endif
#ifdef USE_YUV_UV
      case VC_IMAGE_YUV_UV:
      case VC_IMAGE_YUV_UV32:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_yuv_uv_to_tf_etc1(dest, src, 0);
         break;
#endif
#ifdef USE_RGBA16
      case VC_IMAGE_RGBA16:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgba16_to_tf_etc1(dest, src, 0);
         break;
#endif
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgb565_to_tf_etc1(dest, src, 0);
         break;
#endif
#ifdef USE_RGB888
      case VC_IMAGE_RGB888:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgb888_to_tf_etc1(dest, src, 0);
         break;
#endif
#ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgba32_to_tf_etc1(dest, src, 0);
         break;
#endif
      default:
         vcos_assert(!"Invalid src->type for TF_ETC1 dest->type!");
      }
      break;
   case VC_IMAGE_TF_RGBA32:
   case VC_IMAGE_TF_RGBX32:
      switch (src->type)
      {
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgb565_to_tf_rgba32(dest, src, 0);
         break;
#endif
#ifdef USE_RGB888
      case VC_IMAGE_RGB888:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgb888_to_tf_rgbx32(dest, src, VC_IMAGE_ROT0);
         break;
#endif
#ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgba32_to_tf_rgba32(dest, 0, src, 0, 0);
         break;
#endif
#ifdef USE_YUV420
      case VC_IMAGE_YUV420:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_yuv420_to_tf_rgba32(dest, src);
         break;
#endif
#ifdef USE_YUV422
      case VC_IMAGE_YUV422:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_yuv422_to_tf_rgba32(dest, src);
         break;
#endif
#ifdef USE_YUV_UV
      case VC_IMAGE_YUV_UV:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_yuv_uv_to_tf_rgba32(dest, src);
         break;
#endif
      default:
         vcos_assert(!"Invalid src->type for TF_RGX32 dest->type!");
      }
      break;
   case VC_IMAGE_TF_RGB565:
      switch (src->type)
      {
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_rgb565_to_tf_rgb565(dest, src, 0);
         break;
#endif
#ifdef USE_YUV420
      case VC_IMAGE_YUV420:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_yuv420_to_tf_rgb565(dest, src);
         break;
#endif
#ifdef USE_YUV422
      case VC_IMAGE_YUV422:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_yuv422_to_tf_rgb565(dest, src);
         break;
#endif
#ifdef USE_YUV_UV
      case VC_IMAGE_YUV_UV:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_yuv_uv_to_tf_rgb565(dest, src);
         break;
#endif
      default:
         vcos_assert(!"Invalid src->type for TF_RGB565 dest->type!");
      }
      break;
   case VC_IMAGE_OPENGL: switch (dest->extra.opengl.format_and_type)
   {
      case VC_IMAGE_OPENGL_ETC1:
         switch (src->type)
         {
         case VC_IMAGE_OPENGL:
            vcos_assert(mirror == 0 && rotate == 0);
            vc_image_opengl_to_ogl_etc1(dest, src, 0);
            break;
   #ifdef USE_YUV420
         case VC_IMAGE_YUV420:
            vcos_assert(mirror == 0 && rotate == 0);
            vc_image_yuv420_to_ogl_etc1(dest, src, 0);
            break;
   #endif
   #ifdef USE_YUV422
         case VC_IMAGE_YUV422:
            vcos_assert(mirror == 0 && rotate == 0);
            vc_image_yuv422_to_ogl_etc1(dest, src, 0);
            break;
   #endif
   #ifdef USE_YUV422PLANAR
         case VC_IMAGE_YUV422PLANAR:
            vcos_assert(mirror == 0 && rotate == 0);
            vc_image_yuv422p_to_ogl_etc1(dest, src, 0);
            break;
   #endif
   #ifdef USE_YUV_UV
         case VC_IMAGE_YUV_UV:
         case VC_IMAGE_YUV_UV32:
            vcos_assert(mirror == 0 && rotate == 0);
            vc_image_yuv_uv_to_ogl_etc1(dest, src, 0);
            break;
   #endif
   #ifdef USE_RGBA16
         case VC_IMAGE_RGBA16:
            vcos_assert(mirror == 0 && rotate == 0);
            vc_image_rgba16_to_ogl_etc1(dest, src, 0);
            break;
   #endif
   #ifdef USE_RGB565
         case VC_IMAGE_RGB565:
            vcos_assert(mirror == 0 && rotate == 0);
            vc_image_rgb565_to_ogl_etc1(dest, src, 0);
            break;
   #endif
   #ifdef USE_RGB888
         case VC_IMAGE_RGB888:
            vcos_assert(mirror == 0 && rotate == 0);
            vc_image_rgb888_to_ogl_etc1(dest, src, 0);
            break;
   #endif
   #ifdef USE_RGBA32
         case VC_IMAGE_RGBA32:
            vcos_assert(mirror == 0 && rotate == 0);
            vc_image_rgba32_to_ogl_etc1(dest, src, 0);
            break;
   #endif
         default:
            vcos_assert(!"Invalid src->type for OGL_ETC1 dest->type!");
         }
         break;
      default:
         vcos_assert(!"Invalid OGL dest->type");
         break;
   } break;
#endif

#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      switch ( src->type )
      {
#ifdef USE_YUV420
      case VC_IMAGE_YUV420:
         vc_image_convert_yuv_rgb888(dest, src, mirror, rotate);
         break;
#endif
#ifdef USE_YUV422
      case VC_IMAGE_YUV422:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_convert_yuv422_rgb888(dest, src);
         break;
#endif
      case VC_IMAGE_RGB565:
         vc_image_convert_to_24bpp( dest, 0, 0, dest->width, dest->height, src, 0, 0 );
         break;
# ifdef USE_3D32
      case VC_IMAGE_3D32:

         if (dest->width==src->width)
            vc_3d32_to_rgb888( dest->image_data, src->image_data, dest->width*src->height, vc_image_get_palette(src) );
         else if (dest->width*2==src->width)
            vc_3d32_to_rgb888_anti2( dest->image_data, src->image_data, dest->width*src->height, vc_image_get_palette(src) );
         else
            vcos_assert(!"Image type not supported");

         //vc_3d32_to_rgb888( dest->image_data, src->image_data, dest->width*dest->height, vc_image_get_palette(src) );
         break;
# endif

#ifdef USE_8BPP
      case VC_IMAGE_8BPP:
      {
         int i;
         for ( i = 0; i < src->height; i+= 16 )
            vc_palette8_to_rgb888( ((unsigned char*)dest->image_data + i*dest->pitch),
                                   ((unsigned char*)src->image_data   + i*src->pitch),
                                   /* pitch of dest */ dest->pitch, /* ignored */dest->width, vc_image_get_palette(src), /* pitch of source */src->pitch );
      }
      break;
# endif

# ifdef USE_BGR888
      case VC_IMAGE_BGR888_NP:
      {
         vcos_assert((src->width & 0xf) == 0); //otherwise the pitch will ruin things up
      }
      case VC_IMAGE_BGR888:
      {
         vcos_assert(mirror == 0 && rotate == 0);  //does not support mirror or rotation at the moment
         rgb_stripe_swap((unsigned char*) dest->image_data, (unsigned char*) src->image_data, dest->width, dest->height, VC_IMAGE_RGB888, VC_IMAGE_BGR888);
      }
      break;
# endif

# ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
         vc_image_convert_rgba32_rgb888(dest, src, mirror, rotate);
         break;
# endif

# ifdef USE_TFORMAT
      case VC_IMAGE_TF_RGBX32:
      case VC_IMAGE_TF_RGBA32:
         vcos_assert(mirror == 0 && rotate == 0);
         vc_image_tf_rgba32_to_rgb888(dest, src);
         break;
# endif

# ifdef USE_YUV_UV
      case VC_IMAGE_YUV_UV:
      {
         VC_IMAGE_BUF_T *tmpbuf;
         /* We don't have a conversion function from RGB888 to RGBA32 so have to convert to RGB565 first */
         tmpbuf = vc_image_malloc(VC_IMAGE_YUV420, dest->width, dest->height, 0);
         if (vcos_verify(tmpbuf != NULL)) {
            vc_image_convert(tmpbuf, src, 0, 0);
            vc_image_convert(dest, tmpbuf, 0, 0);
            vc_image_free(tmpbuf);
         }
         break;
      }
# endif

      default:
         vcos_assert(!"Invalid src->type for RGB888 dest->type!");
         break;
      }
      break;
#endif //dest type is RGB888

#ifdef USE_BGR888
   case VC_IMAGE_BGR888_NP:
      vcos_assert((src->width & 0xf) == 0); //otherwise the pitch will ruin things up
   case VC_IMAGE_BGR888:
      switch ( src->type )
      {
# ifdef USE_RGB888
      case VC_IMAGE_RGB888:
      {
         vcos_assert(mirror == 0 && rotate == 0);  //does not support mirror or rotation at the moment
         rgb_stripe_swap((unsigned char*) dest->image_data, (unsigned char*) src->image_data, dest->width, dest->height, VC_IMAGE_RGB888, VC_IMAGE_BGR888);
      }
      break;
# endif
# ifdef USE_YUV420
      case VC_IMAGE_YUV420:
      {
         //first do a conversion to RGB888
         vc_image_convert_yuv_rgb888(dest, src, mirror, rotate);
         //then swap R&B in place
         rgb_stripe_swap((unsigned char*) dest->image_data, (unsigned char*) dest->image_data, dest->width, dest->height, VC_IMAGE_BGR888, VC_IMAGE_RGB888);
         break;
      }
# endif
      default:
         vcos_assert(!"Invalid src->type for BGR888 dest->type!");
         break;
      }
      break;

#endif //dest type is BGR888

#ifdef USE_RGB666
   case VC_IMAGE_RGB666:
      switch ( src->type )
      {
# ifdef USE_RGB565
      case VC_IMAGE_RGB565:
      {
         int i;
         for ( i = 0; i < src->height; i+= 16 )
            vc_rgb565_to_rgb666( ((unsigned char*)dest->image_data + i*dest->pitch),
                                 ((unsigned char*)src->image_data  + i*src->pitch),
                                 dest->pitch, src->pitch, dest->width, vc_image_get_palette(src) );
      }
      break;
# endif

#ifdef USE_8BPP
      case VC_IMAGE_8BPP:
      {
         int xoffset=(src->width-dest->width)>>1;
         int i;
         vcos_assert(xoffset>=0);

         for ( i = 0; i < src->height; i+= 16 )
            vc_palette8_to_rgb666( ((unsigned char*)dest->image_data + i*dest->pitch),
                                   ((unsigned char*)src->image_data  + i*src->pitch + xoffset),
                                   dest->pitch, src->pitch, dest->width, vc_image_get_palette(src) );
      }
      break;
# endif

# ifdef USE_RGB888
      case VC_IMAGE_RGB888:
      {
         int xoffset=(src->width-dest->width)>>1;
         int i;
         vcos_assert(xoffset>=0);

         for ( i = 0; i < src->height; i+= 16 )
            vc_rgb888_to_rgb666( ((unsigned char*)dest->image_data + i*dest->pitch),
                                 ((unsigned char*)src->image_data  + i*src->pitch),
                                 dest->pitch, src->pitch, dest->width, vc_image_get_palette(src) );
      }
      break;
# endif


      default:
         vcos_assert(!"Invalid src->type for RGB66 dest->type!");
         break;
      }
      break;
#endif

#ifdef USE_YUV_UV
   case VC_IMAGE_YUV_UV:
      switch ( src->type )
      {
#ifdef USE_YUV420
      case VC_IMAGE_YUV420:
      {
         vc_image_yuvuv128_from_yuv420(dest, src);
         break;
      }
#endif

      case VC_IMAGE_RGBA32:
      case VC_IMAGE_RGB888:
      case VC_IMAGE_BGR888:
      case VC_IMAGE_RGB565:
      {
         vc_image_rgb_to_yuv(dest, src);
         break;
      }

      default:
         vcos_assert(!"Invalid src->type for YUV_UV dest->type!");
         break;
      }
      break;

#endif

   default:
      vcos_assert(!"Invalid dest->type!");
      break;
   }


}

/******************************************************************************
NAME
   vc_image_yuv2rgb

SYNOPSIS
   void vc_image_yuv2rgb(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, int downsample)


FUNCTION
   Convert YUV bitmap to RGB, according to the type of RGB in dest. Optionally
   downsample by 2 in each direction.

RETURNS
   void
******************************************************************************/

void vc_image_yuv2rgb(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, int downsample)
{
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));

   switch (dest->type) {
#ifdef USE_RGB565
   case VC_IMAGE_RGB565:
      vc_image_yuv2rgb565(dest, src, downsample);
      break;
#endif
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      vc_image_yuv2rgb888(dest, src, downsample);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
   }
   if (downsample)
      vc_image_set_dimensions(dest, src->width>>1, src->height>>1);
   else
      vc_image_set_dimensions(dest, src->width, src->height);
}

/******************************************************************************
NAME
   vc_image_free

SYNOPSIS
   void vc_image_free(VC_IMAGE_BUF_T *img)


FUNCTION
   Frees up (using free_256bit) the source bytes and vc_image header

RETURNS
   void
******************************************************************************/

void vc_image_free(VC_IMAGE_BUF_T *img)
{
   (void)vcos_verify(img!=NULL);
   free_256bit(img);
}

/******************************************************************************
NAME
   vc_runlength_decode

SYNOPSIS
   VC_IMAGE_BUF_T *vc_runlength_decode(unsigned char *data);

FUNCTION
   Returns an image based on decompressing the given bytes (made by helpers/vc_image/runlength.m)

RETURNS
   Pointer to the image (can be freed with vc_image_free) (or NULL if out of memory)
******************************************************************************/

VC_IMAGE_BUF_T *vc_runlength_decode(unsigned char *data)
{
   // !!!! equivalent to return vc_run_decode(NULL,data);
#ifdef USE_RGB565
   unsigned short *p=(unsigned short *)(void *)data;
   VC_IMAGE_BUF_T *img=(VC_IMAGE_BUF_T *)malloc_256bit(sizeof(VC_IMAGE_BUF_T));
   if (img==NULL) return NULL;
   img->width=*p++;
   img->height=*p++;
   img->type=VC_IMAGE_RGB565;
   img->info.info = 0;
   img->pitch=img->width*2;
   img->size=img->pitch*img->height;
   img->image_data=malloc_256bit(img->size);
   if (img->image_data==NULL)
   {
      free_256bit(img);
      return NULL;
   }
   vc_rgb565_runlength(p,img->image_data);
   return img;
#else
   (void)data;
   vcos_demand(!"unsupported function");
   return 0;
#endif
}

/******************************************************************************
NAME
   vc_run_decode

SYNOPSIS
   VC_IMAGE_BUF_T *vc_run_decode(VC_IMAGE_BUF_T *img,unsigned char *data);

FUNCTION
   Returns an image based on decompressing the given bytes (made by helpers/vc_image/convertbmp2run.m)
   If img is NULL then a new image structure is allocated.

RETURNS
   Pointer to the image (can be freed with vc_image_free) (or NULL if out of memory)
******************************************************************************/

VC_IMAGE_BUF_T *vc_run_decode(VC_IMAGE_BUF_T *img,unsigned char *data)
{
#ifdef USE_RGB565
   unsigned short *p=(unsigned short *)(void *)data;
   unsigned short *im;
   if (img==NULL)
   {
      img=malloc_256bit(sizeof(VC_IMAGE_BUF_T));
      if (img==NULL) return NULL;

      img->width=*p++;
      img->height=*p++;
      img->type=VC_IMAGE_RGB565;
      img->info.info = 0;
      img->pitch=img->width*2;
      img->size=img->pitch*img->height;
      img->image_data=malloc_256bit(img->size);
      im=(unsigned short *)img->image_data;
      if (img->image_data==NULL)
      {
         free_256bit(img);
         return NULL;
      }
   }
   else
   {
      img->width=*p++;
      img->height=*p++;
      img->type=VC_IMAGE_RGB565;
      img->info.info = 0;
      img->pitch=img->width*2;
      vcos_assert(img->pitch*img->height<=img->size);
   }
   im=(unsigned short *)img->image_data;
   vc_rgb565_runlength(p,im);

   return img;
#else
   (void)img;
   (void)data;
   vcos_demand(!"unsupported function");
   return 0;
#endif
}


/******************************************************************************
NAME
   vc_runlength_decode8

SYNOPSIS
   VC_IMAGE_BUF_T *vc_runlength_decode8(unsigned char *data);

FUNCTION
   Returns an image based on decompressing the given bytes (made by helpers/vc_image/runlength8.m)

RETURNS
   Pointer to the image (can be freed with vc_image_free) (or NULL if out of memory)
******************************************************************************/

VC_IMAGE_BUF_T *vc_runlength_decode8(unsigned char *data)
{
#ifdef USE_8BPP
   unsigned short *p=(unsigned short *)(void *)data;
   VC_IMAGE_BUF_T *img=(VC_IMAGE_BUF_T *)malloc_256bit(sizeof(VC_IMAGE_BUF_T));
   if (img==NULL) return NULL;
   img->width=*p++;
   img->height=*p++;
   img->type=VC_IMAGE_8BPP;
   img->info.info = 0;
   img->pitch=img->width;
   img->size=img->pitch*img->height;
   img->image_data=malloc_256bit(img->size);
   if (img->image_data==NULL)
   {
      free_256bit(img);
      return NULL;
   }
   vc_runlength8(p,img->image_data);
   return img;
#else
   (void)data;
   vcos_demand(!"unsupported function");
   return 0;
#endif
}

/******************************************************************************
NAME
   vc_image_deblock

SYNOPSIS
   void vc_image_deblock(VC_IMAGE_BUF_T *img);

FUNCTION
   Deblocks a YUV image using a simple averaging scheme on 8 by 8 blocks

RETURNS
   -
******************************************************************************/
void vc_image_deblock(VC_IMAGE_BUF_T *img)
{
   int w=img->width;
   int h=img->height;
   unsigned char *data=(unsigned char *)img->image_data;
   int ysize=w*h;

   if (!vcos_verify(img->type==VC_IMAGE_YUV420))
      return;
#ifdef USE_YUV420
   vcos_assert(is_valid_vc_image_buf(img, img->type));
   vc_yuv_deblock(data,w,h);
   vc_yuv_deblock(data+ysize,w>>1,h>>1);
   vc_yuv_deblock(data+ysize+(ysize>>2),w>>1,h>>1);
#endif
}
/******************************************************************************
NAME
   vc_image_pack

SYNOPSIS
   void vc_image_pack(VC_IMAGE_BUF_T *img, int x, int y, int w, int h);

FUNCTION
   Packs the bytes in an image so that there is no padding between rows. The result
   is not very useful for the vc_image functions but might make the data more
   convenient for other purposes.

RETURNS
   -
******************************************************************************/

void vc_image_pack (VC_IMAGE_BUF_T *img, int x, int y, int w, int h) {
   int pitch = img->pitch;
   unsigned char *src = (unsigned char *)img->image_data + y*pitch;
   unsigned char *dest = (unsigned char *)img->image_data;
   int i;

   switch (img->type) {
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      w *= 2;
      src += 2 * x;
      break;
   case VC_IMAGE_RGB888:
      w *= 3;
      src += 3 * x;
      break;
   default:
      vcos_assert(!"Image type not supported");
   }

   if (dest == src && pitch == w) {
      return; // Already packed
   }

   if (pitch >= w) {
      if (dest <= src) {
         for (i = 0; i < h; i++, src += pitch, dest += w)
            vclib_memcpy(dest, src, w);
      } else {
         for (i = 0; i < h; i++, src += pitch, dest += w)
            memmove(dest, src, w);
      }
   } else {
// We are actually expanding the data, so copy in reverse order to avoid overwrite
      src += pitch*(h-1);
      dest += w*(h-1);
      if (dest <= src) {
         for (i = 0; i < h; i++, src -= pitch, dest -= w)
            vclib_memcpy(dest, src, w);
      } else {
         for (i = 0; i < h; i++, src -= pitch, dest -= w)
            memmove(dest, src, w);
      }
   }
}

/******************************************************************************
NAME
   vc_image_copy_pack

SYNOPSIS
   void vc_image_copy_pack(unsigned char *dest, VC_IMAGE_BUF_T *img, int x, int y, int w, int h);

FUNCTION
   Packs the bytes in an image so that there is no padding between rows. The result
   is not very useful for the vc_image functions but might make the data more
   convenient for other purposes.

RETURNS
   -
******************************************************************************/

void vc_image_copy_pack (unsigned char *dest, VC_IMAGE_BUF_T *img, int x, int y, int w, int h) {
   int pitch = img->pitch;
   unsigned char *src = (unsigned char *)img->image_data + y*pitch;
   int i;

   vcos_assert(is_valid_vc_image_buf(img, img->type));

   switch (img->type) {
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      w *= 2;
      src += 2 * x;
      break;
   case VC_IMAGE_RGB888:
      w *= 3;
      src += 3 * x;
      break;
   default:
      vcos_assert(!"Image type not supported");
   }

   if (w == pitch)
      vclib_memcpy(dest, src, w*h);
   else {
      for (i = 0; i < h; i++, src += pitch, dest += w)
         vclib_memcpy(dest, src, w);
   }
}

/******************************************************************************
NAME
   vc_image_unpack

SYNOPSIS
   void vc_image_unpack(VC_IMAGE_BUF_T *img, int src_pitch, int h);

FUNCTION
   Unpacks the bytes in an image so that there is the correct padding between rows.
   Note that the region that is "unpacked" must begin at the top left corner of the
   bitmap, though not all of the bitmap has to be unpacked.

RETURNS
   -
******************************************************************************/

void vc_image_unpack (VC_IMAGE_BUF_T *img, int src_pitch, int h) {
   unsigned char *src;
   unsigned char *dest;
   int i;

   vcos_assert(is_valid_vc_image_buf(img, img->type));

   // We could actually do this in stripes of 16, but single rows is probably good enough.
   src  = (unsigned char *)img->image_data + (h-1)*src_pitch;
   dest = (unsigned char *)img->image_data + (h-1)*img->pitch;

   if (dest == src && img->pitch == src_pitch) {
      return; // Already unpacked
   }

   // work backwards through image so we can expand image inplace.
   // (vc_image_unpack_row also does backwards mempcy)
   for (i = 0; i<h; i++) {
      vc_image_unpack_row(dest, src, src_pitch);
      src  -= src_pitch;
      dest -= img->pitch;
   }
}


/******************************************************************************
NAME
   vc_image_copy_unpack

SYNOPSIS
   void vc_image_copy_unpack(VC_IMAGE_BUF_T *img, int dest_x_off, int dest_y_off, unsigned char *src, int w, int h);

FUNCTION
   Unpacks the bytes in an image so that there is the correct padding between rows.
   Source and destination memory buffers must be distinct.

RETURNS
   -
******************************************************************************/

void vc_image_copy_unpack (VC_IMAGE_BUF_T *img, int dest_x_off, int dest_y_off, unsigned char *src, int w, int h) {
   int pitch = img->pitch;
   unsigned char *dest;
   int i;

   vcos_assert(is_valid_vc_image_buf(img, img->type));

   switch (img->type) {
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
      w *= 2;
      dest_x_off *= 2;
      break;
   case VC_IMAGE_RGB888:
      w *= 3;
      dest_x_off *= 3;
      break;
   default:
      vcos_assert(!"Image type not supported");
   }

   // We could actually do this in stripes of 16, but single rows is probably good enough.

   dest = (unsigned char *)img->image_data + dest_y_off*pitch + dest_x_off;
   // NOTE: there's a known problem here because some functions, eg. dispman_line, might
   // try and load the whole screen into the transfer buffer, only the screen might be
   // 24bpp and the transfer buffer 16bpp, meaning there isn't room.
   vcos_assert(dest_y_off*pitch+dest_x_off+(h-1)*pitch+w <= img->size);

   if (pitch == w)
      vclib_memcpy(dest, src, w*h);
   else {
      for (i = 0; i < h; i++, src += w, dest += pitch)
         vclib_memcpy(dest, src, w);
   }
}



/******************************************************************************
NAME
   vc_image_vparmalloc_unwrapped
   vc_image_parmalloc_unwrapped

SYNOPSIS
   VC_IMAGE_BUF_T *vc_image_vparmalloc_unwrapped(VC_IMAGE_TYPE_T type,
                     char const *description,
                     long width, long height, va_list proplist);
   VC_IMAGE_BUF_T *vc_image_parmalloc_unwrapped(VC_IMAGE_TYPE_T type,
                     char const *description,
                     long width, long height, VC_IMAGE_PROPERTY_T prop, ...);

FUNCTION
   Allocate (256-bit aligned) memory for an image buffer given the image
   dimensions and bit-depth (type)

   The remaining arguments are a VC_IMAGE_PROPERTY_T list terminated with
   VC_IMAGE_PROP_END.  Each property is followed by one argument:
      VC_IMAGE_PROP_NONE      - (long) discard next long, and do nothing
      VC_IMAGE_PROP_ALIGN     - (long) required memory alignment
      VC_IMAGE_PROP_STAGGER   - (long) offset from aligned address
      VC_IMAGE_PROP_PADDING   - (long) allocate this much extra space
      VC_IMAGE_PROP_PRIORITY  - (long) rtos_prioritymalloc() argument
      VC_IMAGE_PROP_MALLOCFN  - void *(*)(size_t) malloc() function replacement
      VC_IMAGE_PROP_CACHEALIAS- (void const *) pointer to required alias
      VC_IMAGE_PROP_CLEARVALUE- (long) value to fill allocated memory with

   other values may be listed after these (and before VC_IMAGE_PROP_END), and
   they will be passed through to vc_image_configure().

   TBD: VC_IMAGE_BUF_T structure ought to be alighned to 256nit boundary ?
   TBD: calculation of pitch.

RETURNS
   A pointer to the allocated area of memory

NOTES
   Using VC_IMAGE_PROP_DATA here will work, and vc_image_parfree() will still
   behave appropriately, but that data buffer will persist after the call to
   vc_image_parfree().  The caller must remember to free that buffer at some
   later stage.
******************************************************************************/

static void const *default_cache_alias(VC_IMAGE_T *image, void const *pointer)
{
   /* TODO: We could keep some small images in cached memory if it seems like a
    * reasonable default behaviour.
    */
   return ALIAS_DIRECT(pointer);
}


static VC_IMAGE_BUF_T *vc_image_v1parmalloc(VC_IMAGE_TYPE_T type, char const *description, long width, long height, VC_IMAGE_PROPERTY_T prop, va_list proplist)
{
   typedef void *mallocfn_t(size_t);
   VC_IMAGE_BUF_T temp_image;
   VC_IMAGE_BUF_T *image;
   int alloc_size;

   int align = -1;
   int stagger = 0;
   int padding = 0;
   int priority = 1000;
   mallocfn_t *mallocfn = NULL;
   char use_refptr = 0, use_clearvalue = 0;
   void const *refptr = NULL;
   unsigned long clearvalue = 0xaa55aa55;

   while (prop != VC_IMAGE_PROP_END)
   {
      int breaknow = 0;

      switch (prop)
      {
      case VC_IMAGE_PROP_NONE:
         (void)va_arg(proplist, long);
         break;
      case VC_IMAGE_PROP_ALIGN:
         align = (int)va_arg(proplist, long);
         break;
      case VC_IMAGE_PROP_STAGGER:
         stagger = (int)va_arg(proplist, long);
         break;
      case VC_IMAGE_PROP_PADDING:
         padding = (int)va_arg(proplist, long);
         break;
      case VC_IMAGE_PROP_PRIORITY:
         priority = (int)va_arg(proplist, long);
         break;
      case VC_IMAGE_PROP_MALLOCFN:
         mallocfn = va_arg(proplist, mallocfn_t *);
         break;
      case VC_IMAGE_PROP_CACHEALIAS:
         use_refptr = 1;
         refptr = va_arg(proplist, void const *);
         break;
      case VC_IMAGE_PROP_CLEARVALUE:
         use_clearvalue = 1;
         clearvalue = va_arg(proplist, unsigned long);
         break;
      default:
         breaknow = 1;
         break;
      }
      if (breaknow)
         break;

      prop = va_arg(proplist, VC_IMAGE_PROPERTY_T);
   }

   // first work out size required using temp_image header
   if (vc_image_v1configure(&temp_image, type, width, height, prop, proplist) != 0)
      return NULL;

   if (align < 4)
      align = fix_alignment((VC_IMAGE_TYPE_T)temp_image.type, 1);

   if (use_refptr == 0)
      refptr = default_cache_alias(&temp_image, (void *)-1);

   vcos_assert(align >= 4 && _count(align) == 1);

   alloc_size = sizeof(*image) + 31 & ~31;
   if (temp_image.image_data == NULL)
      alloc_size += temp_image.size + padding + align;

   if (mallocfn != NULL)
      image = (VC_IMAGE_T *)mallocfn(alloc_size);
   else
   {
      if (IS_ALIAS_COHERENT(refptr))
         priority |= VCLIB_PRIORITY_COHERENT;
      if (IS_ALIAS_DIRECT(refptr))
         priority |= VCLIB_PRIORITY_DIRECT;

      image = (VC_IMAGE_T *)rtos_malloc_priority(alloc_size, RTOS_ALIGN_256BIT, priority, description);
   }

   if (image != NULL)
   {
      vc_image_initialise(image);
      if (temp_image.image_data == NULL)
      {
         unsigned long image_data;
         void *image_ptr;

         image_data = (unsigned long)(image + 1);
         image_data += (stagger - image_data) & (align - 1);
         /* TODO: put guard words (perhaps ~clearvalue) around the outside. */

         /* Check to see if I'm an idiot -- SH1 */
         vcos_assert(image_data >= (unsigned long)(image + 1));
         vcos_assert(image_data + temp_image.size + padding <= (unsigned long)image + alloc_size);
         vcos_assert((image_data & (align - 1)) == (stagger & (align - 1)));

         image_ptr = (void *)image_data;

         vc_image_set_image_data(&temp_image, temp_image.size + padding, image_ptr);
      }
      else
      {
         /* Using VC_IMAGE_PROP_CACHEALIAS with VC_IMAGE_PROP_DATA implies that
          * the data pointer provided has to be rewritten.  However, there's a
          * risk that another copy of that pointer exists elsewhere.  This can
          * result in unsafe cache access patterns and unexpected results, so
          * we can't perform the re-write.
          *
          * The other view is that we can silently ignore the condition, as we
          * deliberately allow other nonsense parameters to be specified
          * allowing callers to avoid conditional code by specifying defaults
          * for a variety of incompatible conditions.
          */
         vcos_assert(use_refptr == 0);
      }

      if (use_clearvalue != 0)
         vclib_memset4(ALIAS_ANY_NONALLOCATING(temp_image.image_data), clearvalue, temp_image.size >> 2);

      image = (VC_IMAGE_BUF_T *)ALIAS_NORMAL(image);
      // Now set header parameters on real data
      memcpy(image, &temp_image, sizeof(*image));
   }

   return (VC_IMAGE_T *)image;
}

VC_IMAGE_BUF_T *vc_image_vparmalloc_unwrapped(VC_IMAGE_TYPE_T type, char const *description, long width, long height, va_list proplist)
{
   VC_IMAGE_PROPERTY_T prop = va_arg(proplist, VC_IMAGE_PROPERTY_T);

   description = description ? description : rtos_default_malloc_name();

   return vc_image_v1parmalloc(type, description, width, height, prop, proplist);
}

VC_IMAGE_BUF_T *vc_image_parmalloc_unwrapped(VC_IMAGE_TYPE_T type, char const *description, long width, long height, VC_IMAGE_PROPERTY_T prop, ...)
{
   va_list proplist;
   VC_IMAGE_BUF_T *result;

   description = description ? description : rtos_default_malloc_name();

   va_start(proplist, prop);
   result = vc_image_v1parmalloc(type, description, width, height, prop, proplist);
   va_end(proplist);

   return result;
}


/******************************************************************************
NAME
   vc_image_prioritymalloc

SYNOPSIS
   VC_IMAGE_BUF_T *vc_image_prioritymalloc( VC_IMAGE_TYPE_T type, long width,
      long height, int rotate, int priority,char *name)

FUNCTION
   Allocate (256-bit aligned) memory for an image buffer given the image
   dimensions and bit-depth (type)

   TBD: VC_IMAGE_BUF_T structure ought to be alighned to 256nit boundary ?
   TBD: calculation of pitch.

   Priority is the number of times the entire image will be fetched from memory a second

RETURNS
   A pointer to the allocated area of memory
******************************************************************************/

VC_IMAGE_BUF_T *vc_image_prioritymalloc( VC_IMAGE_TYPE_T type, long width,
                                     long height, int rotate, int priority, char const *name)
{
   name = name ? name : rtos_default_malloc_name();
   return vc_image_parmalloc(type, name, width, height,
                             VC_IMAGE_PROP_PRIORITY, priority,
                             VC_IMAGE_PROP_ROTATED, rotate);
}

/******************************************************************************
NAME
   vc_image_malloc

SYNOPSIS

FUNCTION
   Allocate (256-bit aligned) memory for an image buffer given the image
   dimensions and bit-depth (type)

   TBD: VC_IMAGE_BUF_T structure ought to be alighned to 256nit boundary ?
   TBD: calculation of pitch

RETURNS
   A pointer to the allocated area of memory
******************************************************************************/
VC_IMAGE_BUF_T *vc_image_malloc( VC_IMAGE_TYPE_T type, long width, long height, int rotate )
{
   return vc_image_parmalloc(type, rtos_default_malloc_name(), width, height,
                             VC_IMAGE_PROP_ROTATED, rotate);
}


/******************************************************************************


******************************************************************************/
void vc_image_set_palette( short *colourmap )
{
   if (colourmap == NULL)
   {
      /* Use the "default" palette */
      vc_image_palette = (short *)vc_image_safety_palette;
   }
   else
   {
      vc_image_palette = colourmap;
   }
}


short *vc_image_get_palette( VC_IMAGE_T *src )
{
   vcos_assert(src == NULL || is_valid_vc_image_buf(src, src->type));

   short *pal = src ? src->extra.pal.palette : 0;
   if (!pal) {
      if (!vc_image_palette) vc_image_set_palette(NULL);
      pal = vc_image_palette;
   }
   (void)vcos_verify(pal != NULL);
   return pal;
}




/******************************************************************************
NAME
   vc_4by4_decode

SYNOPSIS
   VC_IMAGE_BUF_T *vc_4by4_decode(VC_IMAGE_BUF_T *img,unsigned short *data);

FUNCTION
   Returns an image based on decompressing the given bytes (made by helpers/vc_image/enc4by4.m)
   Can be passed a vc_image in which case the decompressed data will be placed
   into the exisiting image.
   Pass img==NULL to make the routine allocate an image structure itself.

RETURNS
   Pointer to the image (can be freed with vc_image_free) (or NULL if out of memory)
******************************************************************************/
#define ROUNDEDUP(x) (((x)+15)&~15)
VC_IMAGE_BUF_T *vc_4by4_decode(VC_IMAGE_BUF_T *img,unsigned short *data)
{
#ifdef USE_RGB565
   int x,y;
   int w,h;
   unsigned short *im;
   if (img==NULL)
   {
      img=malloc_256bit(sizeof(VC_IMAGE_BUF_T));
      if (img==NULL) return NULL;

      img->width=*data++;
      img->height=*data++;
      img->type=VC_IMAGE_RGB565;
      img->info.info = 0;
      img->pitch=ROUNDEDUP(img->width);
      img->size=img->pitch*ROUNDEDUP(img->height);
      img->image_data=malloc_256bit(img->size);
      im=(unsigned short *)img->image_data;
      if (img->image_data==NULL)
      {
         free_256bit(img);
         return NULL;
      }
   }
   else
   {
      img->width=*data++;
      img->height=*data++;
      img->type=VC_IMAGE_RGB565;
      img->info.info = 0;
      img->pitch=ROUNDEDUP(img->width)*2;
      vcos_assert(img->pitch*ROUNDEDUP(img->height)<=img->size);
   }
   im=(unsigned short *)img->image_data;
   w=ROUNDEDUP(img->width);
   h=ROUNDEDUP(img->height);
   for (y=0;y<h;y+=16)
      for (x=0;x<w;x+=16)
      {
         vc_4by4_decompress(data,&im[x+y*(img->pitch>>1)],img->pitch);
         data+=32*2;
      }
   return img;
#else
   (void)img;
   (void)data;
   vcos_demand(!"unsupported function");
   return 0;
#endif
}

/******************************************************************************
NAME
   vc_image_transform

SYNOPSIS
   void vc_image_transform(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, VC_IMAGE_TRANSFORM_T transform)

FUNCTION
   Transform an image

RETURNS
   void
******************************************************************************/
void vc_image_transform(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, VC_IMAGE_TRANSFORM_T transform)
{
   int ret = vc_image_transform_ret(dest, src, transform);
   vcos_assert(ret == 0);
}

int vc_image_transform_ret(VC_IMAGE_T *dest, VC_IMAGE_T *src, VC_IMAGE_TRANSFORM_T transform)
{
   int success = 0;
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));

   if (src == dest) {
      vc_image_transform_in_place(dest, transform);
      return success;
   }

// We can't use transpose_in_place because the dest might not be square

   switch (transform) {
   case VC_IMAGE_ROT0:
      vc_image_copy(dest, src);
      break;
   case VC_IMAGE_ROT90:
      success = vc_image_transpose_ret(dest, src);
      if(success == 0) {
         vc_image_vflip_in_place(dest);
      }
      break;
   case VC_IMAGE_ROT180:
      vc_image_vflip(dest, src);
      vc_image_hflip_in_place(dest);
      break;
   case VC_IMAGE_ROT270:
      success = vc_image_transpose_ret(dest, src);
      if(success == 0) {
         vc_image_hflip_in_place(dest);
      }
      break;
   case VC_IMAGE_MIRROR_ROT0:
      vc_image_hflip(dest, src);
      break;
   case VC_IMAGE_MIRROR_ROT90:
      success = vc_image_transpose_ret(dest, src);
      break;
   case VC_IMAGE_MIRROR_ROT180:
      vc_image_vflip(dest, src);
      break;
   case VC_IMAGE_MIRROR_ROT270:
      success = vc_image_transpose_ret(dest, src);
      if(success == 0) {
         vc_image_hflip_in_place(dest);
         vc_image_vflip_in_place(dest);
      }
      break;
   }
   return success;
}


/******************************************************************************
NAME
   vc_image_transform_in_place

SYNOPSIS
   void vc_image_transform_in_place(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, VC_IMAGE_TRANSFORM_T transform)

FUNCTION
   Transform an image in place

RETURNS
   void
******************************************************************************/

void vc_image_transform_in_place(VC_IMAGE_BUF_T *img, VC_IMAGE_TRANSFORM_T transform)
{
   vcos_assert(is_valid_vc_image_buf(img, img->type));

   if (transform & TRANSFORM_TRANSPOSE)
      vc_image_transpose_in_place(img);
   if (transform & TRANSFORM_HFLIP)
      vc_image_hflip_in_place(img);
   if (transform & TRANSFORM_VFLIP)
      vc_image_vflip_in_place(img);
}

/******************************************************************************
NAME
   vc_image_transform_tiles

SYNOPSIS
   void vc_image_transform_tiles(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, VC_IMAGE_TRANSFORM_T transform)

FUNCTION
   Transform an image in some ways, but only transpose 16x16 tiles.

   Don't use this function.

RETURNS
   void
******************************************************************************/
void vc_image_transform_tiles(VC_IMAGE_BUF_T *dest, VC_IMAGE_BUF_T *src, VC_IMAGE_TRANSFORM_T transform, int hdeci)
{
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));

   // we don't really expect this to be called out of place, but we will fall back to copy then work in place
   if (!vcos_verify(dest == src)) {
      vc_image_copy(dest, src);
   }
   if (transform & TRANSFORM_TRANSPOSE)
      vc_image_transpose_tiles(dest, src, hdeci);
   else
      vcos_assert(hdeci == 0);
   if (transform & TRANSFORM_HFLIP)
      vc_image_hflip_in_place(dest);
   if (transform & TRANSFORM_VFLIP)
      vc_image_vflip_in_place(dest);
}


VC_IMAGE_TRANSFORM_T vc_image_inverse_transform(VC_IMAGE_TRANSFORM_T transform)
{
   if (transform & TRANSFORM_TRANSPOSE) {
      VC_IMAGE_TRANSFORM_T inv_transform = (VC_IMAGE_TRANSFORM_T)TRANSFORM_TRANSPOSE;
      if (transform & TRANSFORM_HFLIP)
         inv_transform = (VC_IMAGE_TRANSFORM_T)(inv_transform | TRANSFORM_VFLIP);
      if (transform & TRANSFORM_VFLIP)
         inv_transform = (VC_IMAGE_TRANSFORM_T)(inv_transform | TRANSFORM_HFLIP);
      return inv_transform;
   }
   return transform;
}

VC_IMAGE_TRANSFORM_T vc_image_combine_transforms(VC_IMAGE_TRANSFORM_T t1, VC_IMAGE_TRANSFORM_T t2)
{
   VC_IMAGE_TRANSFORM_T transform;
   if (t2 & TRANSFORM_TRANSPOSE) {
      VC_IMAGE_TRANSFORM_T newt1 = (VC_IMAGE_TRANSFORM_T)(t1 & TRANSFORM_TRANSPOSE);
      if (t1 & TRANSFORM_HFLIP)
         newt1 = (VC_IMAGE_TRANSFORM_T)(newt1|TRANSFORM_VFLIP);
      if (t1 & TRANSFORM_VFLIP)
         newt1 = (VC_IMAGE_TRANSFORM_T)(newt1|TRANSFORM_HFLIP);
      transform = (VC_IMAGE_TRANSFORM_T)(newt1 ^ t2);
   } else transform = (VC_IMAGE_TRANSFORM_T)(t1 ^ t2);
   return transform;
}

void vc_image_transform_point(int *x, int *y, int size_x, int size_y, VC_IMAGE_TRANSFORM_T transform)
{
   int tmp;

   switch (transform) {
   case VC_IMAGE_ROT0:
      break;
   case VC_IMAGE_ROT90:
      tmp = *x;
      *x = *y;
      *y = size_x - 1 - tmp;
      break;
   case VC_IMAGE_ROT180:
      *y = size_y - 1 - *y;
      *x = size_x - 1 - *x;
      break;
   case VC_IMAGE_ROT270:
      tmp = *y;
      *y = *x;
      *x = size_y - 1 - tmp;
      break;
   case VC_IMAGE_MIRROR_ROT0:
      *x = size_x - 1 - *x;
      break;
   case VC_IMAGE_MIRROR_ROT90:
      tmp = *x;
      *x = *y;
      *y = tmp;
      break;
   case VC_IMAGE_MIRROR_ROT180:
      *y = size_y - 1 - *y;
      break;
   case VC_IMAGE_MIRROR_ROT270:
      tmp = *y;
      *y = size_x - 1 - *x;
      *x = size_y - 1 - tmp;
      break;
   }
}

void vc_image_transform_rect (int *x, int *y, int *w, int *h, int size_x, int size_y, VC_IMAGE_TRANSFORM_T transform)
{
   int tmp;

   switch (transform) {
   case VC_IMAGE_ROT0:
      break;
   case VC_IMAGE_ROT90:
      tmp = *x;
      *x = *y;
      *y = size_x - tmp - *w;
      tmp = *w;
      *w = *h;
      *h = tmp;
      break;
   case VC_IMAGE_ROT180:
      *y = size_y - *h - *y;
      *x = size_x - *w - *x;
      break;
   case VC_IMAGE_ROT270:
      tmp = *y;
      *y = *x;
      *x = size_y - tmp - *h;
      tmp = *w;
      *w = *h;
      *h = tmp;
      break;
   case VC_IMAGE_MIRROR_ROT0:
      *x = size_x - *x - *w;
      break;
   case VC_IMAGE_MIRROR_ROT90:
      tmp = *x;
      *x = *y;
      *y = tmp;
      tmp = *w;
      *w = *h;
      *h = tmp;
      break;
   case VC_IMAGE_MIRROR_ROT180:
      *y = size_y - *y - *h;
      break;
   case VC_IMAGE_MIRROR_ROT270:
      tmp = *y;
      *y = size_x - *w - *x;
      *x = size_y - *h - tmp;
      tmp = *w;
      *w = *h;
      *h = tmp;
      break;
   }
}


void vc_image_transform_dimensions (int *w, int *h, VC_IMAGE_TRANSFORM_T transform)
{
   if (transform & TRANSFORM_TRANSPOSE) {
      int tmp;
      tmp = *w;
      *w = *h;
      *h = tmp;
   }
}

// Return a friendly string describing the transform, useful for logging.
const char *vc_image_transform_string(VC_IMAGE_TRANSFORM_T xfm)
{
   const char *result = "TRANSFORM_INVALID";
   switch (xfm)
   {
   case VC_IMAGE_ROT0:
      result = "TRANSFORM_ROT0";
      break;
   case VC_IMAGE_ROT90:
      result = "TRANSFORM_ROT90";
      break;
   case VC_IMAGE_ROT180:
      result = "TRANSFORM_ROT180";
      break;
   case VC_IMAGE_ROT270:
      result = "TRANSFORM_ROT270";
      break;
   case VC_IMAGE_MIRROR_ROT0:
      result = "TRANSFORM_MIRROR_ROT0";
      break;
   case VC_IMAGE_MIRROR_ROT90:
      result = "TRANSFORM_MIRROR_ROT90";
      break;
   case VC_IMAGE_MIRROR_ROT180:
      result = "TRANSFORM_MIRROR_ROT180";
      break;
   case VC_IMAGE_MIRROR_ROT270:
      result = "TRANSFORM_MIRROR_ROT270";
      break;
   }
   return result;
}

void vc_image_convert_in_place(VC_IMAGE_BUF_T *image, VC_IMAGE_TYPE_T new_type)
{
   vcos_assert(is_valid_vc_image_buf(image, image->type));

   if (image->type == new_type) {
      return;
   }

   switch (new_type) {
#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      switch (image->type) {
      case VC_IMAGE_RGB565:
         vc_image_rgb565_to_rgb888_in_place(image);
         return;
      default:
         break;
      }
      break;
#endif /* USE_RGB888 */
   case VC_IMAGE_RGB565:
      switch (image->type) {
#ifdef USE_RGB888
      case VC_IMAGE_RGB888:
         vc_image_rgb888_to_rgb565_in_place(image);
         return;
#endif /* USE_RGB888 */
      default:
         break;
      }
      break;

#if defined(USE_RGBA565) && defined(USE_RGB565)
   case VC_IMAGE_RGBA565:
      switch (image->type)
      {
      case VC_IMAGE_RGB565:
         vc_image_rgb565_to_rgba565(image, image);
         break;

      default:
         vcos_assert(!"Invalid source type for in-place convert to RGBA565");
         break;
      }
      break;
#endif

#ifdef USE_RGB666
   case VC_IMAGE_RGB666:
      switch ( image->type )
      {
# ifdef USE_RGB888
      case VC_IMAGE_RGB888:
      {
         int           i, nblocks = ((image->width*image->height)+255) >> 8; /* How many 16x16 pixel blocks do we need to convert ? */
         int           dest_pitch = calculate_pitch( VC_IMAGE_RGB666, image->width, image->height, 0 );
         unsigned char *src       = image->image_data;
         unsigned char *dest      = image->image_data;

         src  += ((nblocks-1) * (16*16*3) ); /* Pointer to last RGB888 16x16 block in src */
         dest += ((nblocks-1) * (16*16*4) ); /* Pointer to last RGB666 16x16 block in dest */

         for ( i=0; i < nblocks; i++, dest -= (16*16*4), src -= (16*16*3))
            vc_rgb888_to_rgb666( dest, src, (16*4), (16*3), 16, NULL );
      }

      break;
# endif

# ifdef USE_RGB565
      case VC_IMAGE_RGB565:
      {
         int           i, nblocks = ((image->width*image->height)+255) >> 8; /* How many 16x16 pixel blocks do we need to convert ? */
         int           dest_pitch = calculate_pitch( VC_IMAGE_RGB666, image->width, image->height, 0 );
         unsigned char *src       = image->image_data;
         unsigned char *dest      = image->image_data;

         src += ((nblocks-1) * (16*16*2) ); /* Pointer to last RGB565 16x16 block in src */
         dest += ((nblocks-1) * (16*16*4) ); /* Pointer to last RGB666 16x16 block in dest */

         for ( i=0; i < nblocks; i++, dest -= (16*16*4), src -= (16*16*2))
            vc_rgb565_to_rgb666( dest, src, (16*4), (16*2), 16, NULL );
      }
      break;
# endif
      default:
         vcos_assert(!"Invalid image type for conversion to RGB666!");
         break;
      }

      return;

#endif /* USE_RGB666 */

   default:
      break;
   }

   vcos_assert(!"Image type not supported");
}


/******************************************************************************
NAME
   vc_image_transparent_alpha_blt

SYNOPSIS
   int vc_image_transparent_alpha_blt(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                                VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset,
                                int transparent_colour, int alpha)

FUNCTION
   Blt from src bitmap to destination with a nominated "transparent" colour and alpha blending.
   Alpha of 256 means use the source, 0 means use the destination.

   Used only by games so handles clipping to visible portion of image.

RETURNS
   void
******************************************************************************/

void vc_image_transparent_alpha_blt (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                                     VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset,
                                     int transparent_colour, int alpha) {
   int d;
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));
   //vcos_assert(is_valid_vc_image_buf(src, dest->type)); // This fails for palettised images
   //vcos_assert(is_within_rectangle(x_offset, y_offset, width, height, 0, 0, dest->width, dest->height);
   vcos_assert((unsigned)alpha <= 256);

   /* Now clip to screen coordinates */
   if (x_offset<0)
   {
      d=-x_offset;
      x_offset+=d;
      src_x_offset+=d;
      width-=d;
   }
   if (y_offset<0)
   {
      d=-y_offset;
      y_offset+=d;
      src_y_offset+=d;
      height-=d;
   }
   if (x_offset+width>dest->width)
   {
      d=x_offset+width-dest->width;
      width-=d;
   }
   if (y_offset+height>dest->height)
   {
      d=y_offset+height-dest->height;
      height-=d;
   }
   if (width<=0)
      return;
   if (height<=0)
      return;

   vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height, 0, 0, src->width, src->height));

   switch (dest->type)
   {
#ifdef USE_RGB565
   case VC_IMAGE_RGB565:
      switch (src->type)
      {
#ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
         vcos_assert(transparent_colour < 0x1000000); /* allow any colour that's already transparent */
         vc_image_overlay_rgba32_to_rgb565(dest, x_offset, y_offset, width, height,
                                           src, src_x_offset, src_y_offset, alpha);
         break;
#endif
#ifdef USE_RGBA16
      case VC_IMAGE_RGBA16:
         vcos_assert(transparent_colour < 0x1000); /* allow any colour that's already transparent */
         vc_image_overlay_rgba16_to_rgb565(dest, x_offset, y_offset, width, height,
                                           src, src_x_offset, src_y_offset, alpha);
         break;
#endif
#ifdef USE_RGBA565
      case VC_IMAGE_RGBA565:
         vc_image_overlay_rgba565_to_rgb565(dest, x_offset, y_offset, width, height,
                                            src, src_x_offset, src_y_offset, alpha);
         break;
#endif
#if defined(USE_4BPP) || defined(USE_8BPP)
      case VC_IMAGE_4BPP:
      case VC_IMAGE_8BPP:
      {
         int remaining_width = width;
         while (remaining_width > 0) {
            width = min(remaining_width, 240);
            vc_image_transparent_alpha_blt_rgb565_from_pal8(dest, x_offset, y_offset, width, height,
                  src, src_x_offset, src_y_offset, transparent_colour,alpha);
            remaining_width -= width;
            x_offset += width;
            src_x_offset += width;
         }
      }
      break;
#endif
      case VC_IMAGE_RGB565:
      {
         int remaining_width = width;
         while (remaining_width > 0) {
            width = min(remaining_width, 240);
            vc_image_transparent_alpha_blt_rgb565(dest, x_offset, y_offset, width, height,
                                                  src, src_x_offset, src_y_offset, transparent_colour,alpha);
            remaining_width -= width;
            x_offset += width;
            src_x_offset += width;
         }
      }
      break;

      default:
         vcos_assert(!"Image type not supported");
      }
      break;
#endif

#ifdef USE_RGB888
   case VC_IMAGE_RGB888:
      switch (src->type)
      {
#ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
         vcos_assert(transparent_colour < 0x1000000); /* allow any colour that's already transparent */
         vc_image_overlay_rgba32_to_rgb888(dest, x_offset, y_offset, width, height,
                                           src, src_x_offset, src_y_offset, alpha);
         break;
#endif
#ifdef USE_RGBA16
      case VC_IMAGE_RGBA16:
         vcos_assert(transparent_colour < 0x1000); /* allow any colour that's already transparent */
         vc_image_overlay_rgba16_to_rgb888(dest, x_offset, y_offset, width, height,
                                           src, src_x_offset, src_y_offset, alpha);
         break;
#endif
#ifdef USE_RGBA565
      case VC_IMAGE_RGBA565:
         vc_image_overlay_rgba565_to_rgb888(dest, x_offset, y_offset, width, height,
                                            src, src_x_offset, src_y_offset, alpha);
         break;
#endif
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vcos_assert(alpha >= 255);
         vc_image_overlay_to_24bpp(dest, x_offset, y_offset, width, height,
                                   src, src_x_offset, src_y_offset, transparent_colour);
         break;
#endif
#if 0 /* Not supported yet*/
      case VC_IMAGE_RGB888:
         vc_image_transparent_alpha_blt_rgb888(dest, x_offset, y_offset, width, height,
                                               src, src_x_offset, src_y_offset, transparent_colour,alpha);
         break;
#endif
      default:
         vcos_assert(!"Image type not supported");
      }
      break;
#endif

#ifdef USE_RGBA32
   case VC_IMAGE_RGBA32:
      switch (src->type)
      {
#ifdef USE_RGBA16
      case VC_IMAGE_RGBA16:
         vc_image_overlay_rgba16_to_rgba32(dest, x_offset, y_offset, width, height,
                                           src, src_x_offset, src_y_offset, alpha);
         break;
#endif
#ifdef USE_RGBA565
      case VC_IMAGE_RGBA565:
         vc_image_overlay_rgba565_to_rgba32(dest, x_offset, y_offset, width, height,
                                            src, src_x_offset, src_y_offset, alpha);
         break;
#endif
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vc_image_overlay_rgb565_to_rgba32(dest, x_offset, y_offset, width, height,
                                           src, src_x_offset, src_y_offset, transparent_colour, alpha);
         break;
#endif
      case VC_IMAGE_RGBA32:
         vc_image_overlay_rgba32_to_rgba32(dest, x_offset, y_offset, width, height,
                                           src, src_x_offset, src_y_offset, alpha);
         break;
      default:
         vcos_assert(!"Image type not supported");
      }
      break;
#endif

#ifdef USE_RGBA565
   case VC_IMAGE_RGBA565:
      switch (src->type)
      {
#ifdef USE_RGBA32
      case VC_IMAGE_RGBA32:
         vcos_assert(transparent_colour < 0x1000000); /* allow any colour that's already transparent */
         vc_image_overlay_rgba32_to_rgba565(dest, x_offset, y_offset, width, height,
                                            src, src_x_offset, src_y_offset, alpha);
         break;
#endif
#ifdef USE_RGBA565
      case VC_IMAGE_RGBA565:
         vcos_assert(alpha >= 255);
         vc_image_overlay_rgba565_to_rgba565(dest, x_offset, y_offset, width, height,
                                             src, src_x_offset, src_y_offset, transparent_colour);
         break;
#endif
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vcos_assert((unsigned)transparent_colour < 0x10000);
         vcos_assert(alpha >= 255);
         vc_image_overlay_rgb565_to_rgba565(dest, x_offset, y_offset, width, height,
                                            src, src_x_offset, src_y_offset, transparent_colour);
         break;
#endif
      default:
         vcos_assert(!"Image type not supported");
      }
      break;
#endif

#ifdef USE_8BPP
   case VC_IMAGE_8BPP:
      switch (src->type)
      {
#if defined(USE_4BPP) || defined(USE_8BPP)
      case VC_IMAGE_4BPP:
      case VC_IMAGE_8BPP:
         vc_image_transparent_alpha_blt_8bpp_from_pal8(dest, x_offset, y_offset, width, height,
               src, src_x_offset, src_y_offset, transparent_colour,alpha);
         break;
#endif
      default:
         vcos_assert(!"Image type not supported");
      }
      break;
#endif

#ifdef USE_RGBA16
   case VC_IMAGE_RGBA16:
      vc_image_overlay_rgb_to_rgb(dest, x_offset, y_offset, width, height,
                                  src, src_x_offset, src_y_offset, transparent_colour, alpha);
      break;
#endif

   default:
      // All other image types currently unsupported.
      vcos_assert(!"Image type not supported");
   }
}


/******************************************************************************
NAME
   vc_image_transparent_alpha_blt_rotated

SYNOPSIS
   int vc_image_transparent_alpha_blt_rotated(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, VC_IMAGE_BUF_T *src,
                                int transparent_colour, int alpha, int scale, int sin_theta, int cos_theta)

FUNCTION
   Blt from src bitmap to destination with a nominated "transparent" colour and alpha blending.
   Alpha of 256 means use the source, 0 means use the destination.

   Used only by games so handles clipping to visible portion of image.

   Supports arbitrary rotation. theta is angle to rotate source image by (clockwise),
   with 1<<16 equal to 360 degrees.
   Supports scaling. scale is 16.16 fixed point number.

 RETURNS
   void
******************************************************************************/

#define fmul(a, b) ((((unsigned int)(a)*(b))>>16)|(_mulhd_ss(a,b)<<16))
void vc_image_transparent_alpha_blt_rotated (VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, VC_IMAGE_BUF_T *src,
      int transparent_colour, int alpha, int scale, int sin_theta, int cos_theta) {
   const int iscale = (1<<24)/scale;
   const int incxx =  (cos_theta*iscale)>>7;
   const int incxy = -(sin_theta*iscale)>>7;
   const int incyx =  (sin_theta*iscale)>>7;
   const int incyy =  (cos_theta*iscale)>>7;
   const int w = (fmul(fmul(src->height, scale), abs(sin_theta)) + fmul(fmul(src->width, scale), abs(cos_theta))) >> 0;
   const int h = (fmul(fmul(src->height, scale), abs(cos_theta)) + fmul(fmul(src->width, scale), abs(sin_theta))) >> 0;
   unsigned int startx = (src->width  << 16) >> 1, starty = (src->height << 16) >> 1;
   int d, sx = 0, sy = 0, ex = dest->width-1, ey = dest->height-1;

   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, src->type));

   startx -= fmul(x_offset,incxx) + fmul(y_offset,incyx);
   starty -= fmul(x_offset,incxy) + fmul(y_offset,incyy);

   // we want to blit to a bounding rectange big enough for current rotation, and aligned enough to be efficient
   d = ALIGN_DOWN((x_offset>>16) - w, 16) - sx;
   if (d > 0) {
      startx += d*incxx;
      starty += d*incxy;
      sx += d;
   }
   d = ex - ALIGN_UP((x_offset>>16) + w, 16);
   if (d > 0) {
      ex -= d;
   }
   d = ((y_offset>>16) - h) - sy;
   if (d > 0) {
      startx += d*incyx;
      starty += d*incyy;
      sy += d;
   }
   d = ey - ((y_offset>>16) + h);
   if (d > 0) {
      ey -= d;
   }

   if (ex < sx || ey < sy) return;

   vcos_assert(sx >= 0 && sy >= 0);
   vcos_assert(ex >= sx && ey >= sy);
   vcos_assert(ex <= dest->width && ey <= dest->height);

#if 0 //xxx
   // check if we can use faster method
   if (scale == 1<<16 && (theta & 0xffff) == 0) {
      vc_image_transparent_alpha_blt_8bpp_from_pal8(dest, (x_offset>>16)-(src->width>>1), (y_offset>>16)-(src->height>>1), src->width, src->height,
            src, 0, 0, transparent_colour, alpha);
      return;
   }
#endif

   vcos_assert(alpha>=0 && alpha<=256);
   switch (dest->type)
   {
#ifdef USE_RGB565
   case VC_IMAGE_RGB565:
      switch (src->type)
      {
#ifdef USE_8BPP
      case VC_IMAGE_8BPP:
         vc_roz_blt_8bpp_to_rgb565((unsigned char *)dest->image_data+sy*dest->pitch+(sx<<1), (unsigned char *)src->image_data,
                                   dest->pitch,src->pitch,ex+1-sx,ey+1-sy,
                                   src->width,src->height,
                                   incxx,incxy,incyx,incyy,startx,starty,transparent_colour,vc_image_get_palette(src));
         break;
#endif
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vc_roz_blt_rgb565_to_rgb565((unsigned char *)dest->image_data+sy*dest->pitch+(sx<<1), (unsigned char *)src->image_data,
                                     dest->pitch,src->pitch,ex+1-sx,ey+1-sy,
                                     src->width,src->height,
                                     incxx,incxy,incyx,incyy,startx,starty,transparent_colour);
         break;
#endif
      default:
         // All other image types currently unsupported.
         vcos_assert(!"Image type not supported");
      }
      break;
#endif
#ifdef USE_8BPP
   case VC_IMAGE_8BPP:
      switch (src->type)
      {
#ifdef USE_8BPP
      case VC_IMAGE_8BPP:
         vc_roz_blt_8bpp_to_8bpp((unsigned char *)dest->image_data+sy*dest->pitch+sx, (unsigned char *)src->image_data,
                                 dest->pitch,src->pitch,ex+1-sx,ey+1-sy,
                                 src->width,src->height,
                                 incxx,incxy,incyx,incyy,startx,starty,transparent_colour);
         break;
#endif
      default:
         // All other image types currently unsupported.
         vcos_assert(!"Image type not supported");
      }
      break;
#endif
   default:
      // All other image types currently unsupported.
      vcos_assert(!"Image type not supported");
   }
}
/*  */


/******************************************************************************
NAME
   vc_image_font_alpha_blt

SYNOPSIS
   void vc_image_font_alpha_blt(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                              VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset, int red, int green, int blue, int alpha)

FUNCTION
   Blt a special 4.4 font image generated from a PNG file.
   Only works with a 565 destination image.
   Can choose a colour via red,green,blue arguments.  Each component should be between 0 and 255.
   The alpha value present in the font image is scaled by the alpha argument given to the routine
   Alpha should be between 0 and 256.

   Used only by games so handles clipping to visible portion of image.

RETURNS
   void
******************************************************************************/

void vc_image_font_alpha_blt(VC_IMAGE_BUF_T *dest, int x_offset, int y_offset, int width, int height,
                             VC_IMAGE_BUF_T *src, int src_x_offset, int src_y_offset, int red, int green, int blue, int alpha)
{
   int d;
   int remaining_width;
   vcos_assert(is_valid_vc_image_buf(dest, dest->type));
   vcos_assert(is_valid_vc_image_buf(src, VC_IMAGE_4BPP) || is_valid_vc_image_buf(src, VC_IMAGE_8BPP));
   //vcos_assert(is_within_rectangle(x_offset, y_offset, width, height, 0, 0, dest->width, dest->height);
   vcos_assert((unsigned)alpha<=256);

   /* Now clip to screen coordinates */
   if (x_offset<0)
   {
      d=-x_offset;
      x_offset+=d;
      src_x_offset+=d;
      width-=d;
   }
   if (y_offset<0)
   {
      d=-y_offset;
      y_offset+=d;
      src_y_offset+=d;
      height-=d;
   }
   if (x_offset+width>dest->width)
   {
      d=x_offset+width-dest->width;
      width-=d;
   }
   if (y_offset+height>dest->height)
   {
      d=y_offset+height-dest->height;
      height-=d;
   }
   if (width<=0)
      return;
   if (height<=0)
      return;
   vcos_assert(is_within_rectangle(src_x_offset, src_y_offset, width, height, 0, 0, src->width, src->height));

   remaining_width = width;
   while (remaining_width > 0) {
      width = min(remaining_width, 240);
      switch (dest->type) {
#ifdef USE_RGB565
      case VC_IMAGE_RGB565:
         vc_image_font_alpha_blt_rgb565(dest, x_offset, y_offset, width, height,
                                        src, src_x_offset, src_y_offset, red, green, blue, alpha);
         break;
#endif

#ifdef USE_RGB888
      case VC_IMAGE_RGB888:
         vc_image_font_alpha_blt_rgb888(dest, x_offset, y_offset, width, height,
                                        src, src_x_offset, src_y_offset, red, green, blue, alpha);
         break;
#endif

#ifdef USE_RGBA32
#if _VC_VERSION >= 3
      case VC_IMAGE_RGBA32:
         vc_image_font_alpha_blt_rgba32(dest, x_offset, y_offset, width, height,
                                        src, src_x_offset, src_y_offset, red, green, blue, alpha);
         break;
#endif
#endif

#if defined(USE_8BPP) || defined(USE_YUV420) || defined(USE_YUV422) || defined(USE_YUV422PLANAR)
      case VC_IMAGE_8BPP:
      case VC_IMAGE_YUV420:
      case VC_IMAGE_YUV422:
      case VC_IMAGE_YUV422PLANAR:
         vc_image_font_alpha_blt_8bpp(dest, x_offset, y_offset, width, height,
                                      src, src_x_offset, src_y_offset, green,
                                      alpha, dest->type != VC_IMAGE_8BPP);
         break;
#endif

      default:
         // All other image types currently unsupported.
         vcos_assert(!"Image type not supported");
      }
      remaining_width -= width;
      x_offset += width;
      src_x_offset += width;
   }
}

/******************************************************************************
NAME
   vc_image_deflicker

SYNOPSIS
   void vc_image_deflicker(VC_IMAGE_BUF_T *img, unsigned char *context);

FUNCTION
   Deflicker a YUV image using a simple low pass filter
   [1/4 1/2 1/4] vertically.
   The context should be large enough for two lines of image,
   and should be zeroed between images (to support stripe based interface).
   This will take about 3-4MHz at 30fps VGA.
RETURNS
   -
******************************************************************************/
void vc_image_deflicker(VC_IMAGE_BUF_T *img, unsigned char *context)
{
   vcos_assert(is_valid_vc_image_buf(img, img->type));

   switch (img->type)
   {
#ifdef USE_RGBA32
   case VC_IMAGE_RGBA32:
      // fall through - can use YUV deflicker code
#endif
#if defined(USE_YUV420) || defined(USE_RGBA32)
   case VC_IMAGE_YUV420:
   {
      int i;
      unsigned char *y = vc_image_get_y(img);
      extern void vc_deflicker_yuv(unsigned char *y, int pitch, unsigned char *context);
      for (i=0; i<img->height; i+=16)
         vc_deflicker_yuv(y+i*img->pitch, img->pitch, context);
      break;
   }
#endif
#ifdef USE_RGBA565
   case VC_IMAGE_RGBA565:
   {
      int i;
      unsigned char *y = vc_image_get_y(img);
      extern vc_deflicker_rgba565(unsigned char *y, int pitch, unsigned char *context);
      for (i=0; i<img->height; i+=16)
         vc_deflicker_rgba565(y+i*img->pitch, img->pitch, context);
      break;
   }
#endif
#ifdef USE_RGBA16
   case VC_IMAGE_RGBA16:
   {
      int i;
      unsigned char *y = vc_image_get_y(img);
      extern vc_deflicker_rgba16(unsigned char *y, int pitch, unsigned char *context);
      for (i=0; i<img->height; i+=16)
         vc_deflicker_rgba16(y+i*img->pitch, img->pitch, context);
      break;
   }
#endif
   default:
      vcos_assert(!"Image type not supported");
   }
}

/******************************************************************************
NAME
   vc_image_blend_interlaced

SYNOPSIS
   void vc_image_blend_interlaced(VC_IMAGE_BUF_T *img, unsigned char *context);

FUNCTION
   Blend an interlaced YUV image using a simple low pass filter
   [1/4 1/2 1/4] vertically - Different from deflicker in that both the luma and colour are filtered.
   The context should be large enough for two lines of image (including colour info),
   and should be zeroed between images (to support stripe based interface).
   This will take about ?? - ?? MHz at 30fps VGA.
RETURNS
   -
******************************************************************************/
void vc_image_blend_interlaced(VC_IMAGE_BUF_T *img, unsigned char *context)
{
   vcos_assert(is_valid_vc_image_buf(img, img->type));

   switch (img->type)
   {

#if defined(USE_YUV420) || defined(USE_RGBA32)
   case VC_IMAGE_YUV420:
   {
      int i;
      unsigned char *y = vc_image_get_y(img);
      unsigned char *u = vc_image_get_u(img);
      unsigned char *v = vc_image_get_v(img);
      extern void vc_deflicker_yuv(unsigned char *y, int pitch, unsigned char *context);
      extern void vc_deflicker_yuv8(unsigned char *y, int pitch, unsigned char *context);
      for (i=0; i<img->height; i+=16)
      {
         vc_deflicker_yuv(y+i*img->pitch, img->pitch, context);
      }
      for (i=0; i<img->height>>1; i+=8)
      {
         vc_deflicker_yuv8(u+i*(img->pitch>>1), img->pitch>>1, &context[img->pitch<<1]);
         vc_deflicker_yuv8(v+i*(img->pitch>>1), img->pitch>>1, &context[(img->pitch<<1)+img->pitch]);
      }
      break;

   }
#endif
   default:
      vcos_assert(!"Image type not supported");
   }
}



/******************************************************************************
NAME
   vc_image_resize_stripe_h

SYNOPSIS
   void vc_image_resize_stripe_h(VC_IMAGE_BUF_T *image, int d_width, int s_width)

FUNCTION
   Resize the given 16-high stripe to have the given width. Works in place.
   Uses bilinear filter.

RETURNS
   -
******************************************************************************/

void vc_image_resize_stripe_h (VC_IMAGE_BUF_T *image, int d_width, int s_width) {
   vcos_assert(is_valid_vc_image_buf(image, image->type) && d_width > 0 && s_width > 0);

   if (d_width == s_width)
      return;

   switch (image->type) {
#if defined(USE_YUV420)
   case VC_IMAGE_YUV420:
      vc_image_resize_stripe_h_yuv420(image, d_width, s_width);
      break;
#endif
#if defined(USE_RGBA32)
   case VC_IMAGE_RGBA32:
      vc_image_resize_stripe_h_rgba32(image, d_width, s_width);
      break;
#endif
#if defined(USE_RGBA16)
   case VC_IMAGE_RGBA16:
      vc_image_resize_stripe_h_rgba16(image, d_width, s_width);
      break;
#endif
#if defined(USE_RGBA565)
   case VC_IMAGE_RGBA565:
      vc_image_resize_stripe_h_rgba565(image, d_width, s_width);
      break;
#endif
#if defined(USE_RGB565)
   case VC_IMAGE_RGB565:
      vc_image_resize_stripe_h_rgb565(image, d_width, s_width);
      break;
#endif
#if defined(USE_8BPP)
   case VC_IMAGE_8BPP:
      vc_image_resize_stripe_h_8bpp(image, d_width, s_width);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
      break;
   }
}

/******************************************************************************
NAME
   vc_image_resize_stripe_v

SYNOPSIS
   void vc_image_resize_stripe_v(VC_IMAGE_BUF_T *image, int d_height, int d_height)

FUNCTION
   Resize the given 16-wide column to have the given height. Works in place.
   Uses bilinear filter.

RETURNS
   -
******************************************************************************/

void vc_image_resize_stripe_v (VC_IMAGE_BUF_T *image, int d_height, int s_height) {
   vcos_assert(is_valid_vc_image_buf(image, image->type) && d_height > 0 && s_height > 0);

   if (d_height == s_height)
      return;

   switch (image->type) {
#if defined(USE_YUV420)
   case VC_IMAGE_YUV420:
      vc_image_resize_stripe_v_yuv420(image, d_height, s_height);
      break;
#endif
#if defined(USE_RGBA32)
   case VC_IMAGE_RGBA32:
      vc_image_resize_stripe_v_rgba32(image, d_height, s_height);
      break;
#endif
#if defined(USE_RGBA16)
   case VC_IMAGE_RGBA16:
      vc_image_resize_stripe_v_rgba16(image, d_height, s_height);
      break;
#endif
#if defined(USE_RGBA565)
   case VC_IMAGE_RGBA565:
      vc_image_resize_stripe_v_rgba565(image, d_height, s_height);
      break;
#endif
#if defined(USE_RGB565)
   case VC_IMAGE_RGB565:
      vc_image_resize_stripe_v_rgb565(image, d_height, s_height);
      break;
#endif
#if defined(USE_8BPP)
   case VC_IMAGE_8BPP:
      vc_image_resize_stripe_v_8bpp(image, d_height, s_height);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
      break;
   }
}


/******************************************************************************
NAME
   vc_image_decimate_stripe_h_2

SYNOPSIS
   void vc_image_decimate_stripe_h_2(VC_IMAGE_BUF_T *src)

FUNCTION
   Decimate a vertical stripe by 2 horizontally. Uses bilinear filter.

RETURNS
   -
******************************************************************************/

static void vc_image_decimate_stripe_h_2(VC_IMAGE_BUF_T *src)
{
   void vc_decimate_2_horiz(void *image, int pitch, int height);
   void vc_decimate_2_horiz16(void *image, int pitch, int height);
   void vc_decimate_2_horiza16(void *image, int pitch, int height);
   void vc_decimate_2_horiz32(void *image, int pitch, int height);

   /* check that info was inited sensibly; you probably wanted zero here ... */
   vcos_assert((src->info.info & ~VC_IMAGE_INFO_VALIDBITS) == 0);

   switch (src->type) {
#if defined(USE_RGBA32)
   case VC_IMAGE_RGBA32:
      vc_decimate_2_horiz32(GET_RGB32(src, 0, 0), src->pitch, src->height);
      break;
#endif
#if defined(USE_RGBA16)
   case VC_IMAGE_RGBA16:
      vc_decimate_2_horiza16(GET_RGB16(src, 0, 0), src->pitch, src->height);
      break;
#endif
#if defined(USE_RGB565) || defined(USE_RGBA565)
case VC_IMAGE_RGB565: case VC_IMAGE_RGBA565:
      vc_decimate_2_horiz16(GET_RGB32(src, 0, 0), src->pitch, src->height);
      break;
#endif
#if defined(USE_YUV420)
   case VC_IMAGE_YUV420:
      vc_decimate_2_horiz(GET_YUV_Y(src, 0, 0), src->pitch, src->height);
      vc_decimate_2_horiz(GET_YUV_U(src, 0, 0), src->pitch>>1, src->height>>1);
      vc_decimate_2_horiz(GET_YUV_V(src, 0, 0), src->pitch>>1, src->height>>1);
      break;
#endif
#if defined(USE_8BPP)
   case VC_IMAGE_8BPP:
      vc_decimate_2_horiz(GET_YUV_Y(src, 0, 0), src->pitch, src->height);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
      break;
   }
}

/******************************************************************************
NAME
   vc_image_decimate_stripe_h

SYNOPSIS
   void vc_image_decimate_stripe_h(VC_IMAGE_BUF_T *src, int offset, int step)

FUNCTION
   Decimate a vertical stripe to 16 wide horizonally. Uses bilinear filter.

RETURNS
   -
******************************************************************************/

void vc_image_decimate_stripe_h(VC_IMAGE_BUF_T *src, int offset, int step)
{
   void vc_decimate_horiz32(void *image, int pitch, int offset, int step, int height);
   void vc_decimate_horiza16(void *image, int pitch, int offset, int step, int height);
   void vc_decimate_horiz16(void *image, int pitch, int offset, int step, int height);
   void vc_decimate_horiz_rgba565(void *image, int pitch, int offset, int step, int height);
   void vc_decimate_horiz(void *image, int pitch, int offset, int step, int height);
   void vc_decimate_horiz_unsmoothed(void *image, int pitch, int offset, int step, int height);
   void vc_decimate_uv_horiz(void *image, int pitch, int offset, int step, int height);
   int shift = 0;

   /* check that info was inited sensibly; you probably wanted zero here ... */
   vcos_assert((src->info.info & ~VC_IMAGE_INFO_VALIDBITS) == 0);

   while (abs(step>>shift) >= 2<<16) {
      vc_image_decimate_stripe_h_2(src);
      shift++;
   }
   //src->width >>= shift;
   if (step>>shift == 1<<16 && offset == 0) {
      return;
   }

   // when vflipped, want to start with last line
   if (step < 0 && shift > 0) {
      offset += (step>>shift);
   }

   switch (src->type) {
#if defined(USE_RGBA32)
   case VC_IMAGE_RGBA32:
      vc_decimate_horiz32(GET_RGB32(src, 0, 0), src->pitch, offset>>shift, step>>shift, src->height);
      break;
#endif
#if defined(USE_RGBA16)
   case VC_IMAGE_RGBA16:
      vc_decimate_horiza16(GET_RGB16(src, 0, 0), src->pitch, offset>>shift, step>>shift, src->height);
      break;
#endif
#if defined(USE_RGB565)
   case VC_IMAGE_RGB565:
      vc_decimate_horiz16(GET_RGB16(src, 0, 0), src->pitch, offset>>shift, step>>shift, src->height);
      break;
#endif
#if defined(USE_RGBA565)
   case VC_IMAGE_RGBA565:
      vc_decimate_horiz_rgba565(GET_RGB16(src, 0, 0), src->pitch, offset>>shift, step>>shift, src->height);
      break;
#endif
#if defined(USE_YUV420)
   case VC_IMAGE_YUV420:
      vc_decimate_horiz(GET_YUV_Y(src, 0, 0), src->pitch, offset>>shift, step>>shift, src->height);
      offset = OFFSET_UV(offset, step);
      vc_decimate_uv_horiz(GET_YUV_U(src, 0, 0), src->pitch>>1, offset>>shift, step>>shift, src->height>>1);
      vc_decimate_uv_horiz(GET_YUV_V(src, 0, 0), src->pitch>>1, offset>>shift, step>>shift, src->height>>1);
      break;
#endif
#if defined(USE_8BPP)
   case VC_IMAGE_8BPP:
      vc_decimate_horiz_unsmoothed(GET_YUV_Y(src, 0, 0), src->pitch, offset, step, src->height);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
      break;
   }
}

/******************************************************************************
NAME
   vc_image_decimate_stripe_v

SYNOPSIS
   void vc_image_decimate_stripe_v(VC_IMAGE_BUF_T *src, int offset, int step)

FUNCTION
   Decimate a horizontal stripe to 16 high vertically. Uses bilinear filter.

RETURNS
   -
******************************************************************************/

void vc_image_decimate_stripe_v(VC_IMAGE_BUF_T *src, int offset, int step)
{
   /* check that info was inited sensibly; you probably wanted zero here ... */
   vcos_assert((src->info.info & ~VC_IMAGE_INFO_VALIDBITS) == 0);

   switch (src->type) {
#if defined(USE_RGBA32)
   case VC_IMAGE_RGBA32:
      vc_decimate_vert(GET_YUV_Y(src, 0, 0), src->pitch, GET_YUV_Y(src, 0, 0), src->pitch, offset, step, 0);
      break;
#endif
#if defined(USE_RGBA16)
   case VC_IMAGE_RGBA16:
      vc_decimate_verta16(GET_RGB16(src, 0, 0), src->pitch, offset, step);
      break;
#endif
#if defined(USE_RGB888)
   case VC_IMAGE_RGB888:
      vc_decimate_vert(GET_YUV_Y(src, 0, 0), src->pitch, GET_YUV_Y(src, 0, 0), src->pitch, offset, step, 0);
      break;
#endif
#if defined(USE_RGB565)
   case VC_IMAGE_RGB565:
      vc_decimate_vert16(GET_RGB16(src, 0, 0), src->pitch, offset, step);
      break;
#endif
#if defined(USE_RGBA565)
   case VC_IMAGE_RGBA565:
      vc_decimate_vert_rgba565(GET_RGB16(src, 0, 0), src->pitch, offset, step);
      break;
#endif
#if defined(USE_YUV420)
   case VC_IMAGE_YUV420:
   {
      void *old_u = GET_YUV_U(src, 0, 0);
      void *old_v = GET_YUV_V(src, 0, 0);
      vc_decimate_vert(GET_YUV_Y(src, 0, 0), src->pitch, GET_YUV_Y(src, 0, 0), src->pitch, offset, step, 0);
      offset = OFFSET_UV(offset, step);
      src->height = TV_STRIPE_HEIGHT;
      // Update extra.uv pointers as height has changed
      vc_image_set_image_data_yuv (src, src->size, src->image_data, 0, 0);

      vc_decimate_uv_vert(GET_YUV_U(src, 0, 0), GET_YUV_V(src, 0, 0), src->pitch>>1,
                          old_u, old_v, src->pitch>>1, offset, step, 0);

      break;
   }
#endif
#if defined(USE_8BPP)
   case VC_IMAGE_8BPP:
      vc_decimate_vert_unsmoothed(GET_YUV_Y(src, 0, 0), src->pitch, 0, 0, offset, step);
      break;
#endif
   default:
      vcos_assert(!"Image type not supported");
      break;
   }
   src->height = TV_STRIPE_HEIGHT;
}



/***********************************************************
 * Name: vc_image_set_alpha
 *
 * Arguments:
 *       VC_IMAGE_BUF_T *image
 *       const int x,
 *       const int y,
 *       const int width,
 *       const int height,
 *       const int alpha
 *
 * Description:
 *    Sets the alpha level of a vc_image
 *
 * Returns: int32_t:
 *               >=0 if it succeeded
 *
 ***********************************************************/
int vc_image_set_alpha( VC_IMAGE_BUF_T *image,
                        const int x,
                        const int y,
                        const int width,
                        const int height,
                        const int alpha )
{
   int success = -1;

   //check the image is valid
   if ( NULL != image)
   {
      /* check that info was inited sensibly; you probably wanted zero here ... */
      vcos_assert((image->info.info & ~VC_IMAGE_INFO_VALIDBITS) == 0);

      //check the image is RGBA32 - this is all this function currently supports
      if ( VC_IMAGE_RGBA32 == image->type )
      {
         unsigned int *row_ptr = NULL;
         int y_count = 0;
         int x_count = 0;

         for ( y_count = y; y_count < (y + height); y_count++ )
         {
            row_ptr = (unsigned int*)((unsigned int)image->image_data + (image->pitch * y_count));

            for ( x_count = x; x_count < (x + width); x_count++ )
            {
               row_ptr[ x_count ] = (row_ptr[ x_count ] & 0x00FFFFFF) | (alpha << 24);
            }
         }
      }
   }

   return success;
}


#define VPP_CRC_POLY      0x04c11db7L
#define MIN(a,b) ( ( (a) < (b) ) ? (a) : (b) )


static VC_IMAGE_CRC_T vc_image_calc_crc_yuv_uv(VC_IMAGE_BUF_T *img, int cl, int cr, int ct, int cb, VCLIB_CRC32_STATE_T *state)
{
   VC_IMAGE_CRC_T crc;
   unsigned char *ptr, *baseptr;
   unsigned long crc_height, crc_width, line_pitch, i, j, stripe_area;
   unsigned long width, line_pitch_area, n_stripes, uv_stripe_area;
   long k;

   unsigned long vec_mem[16+16+16+8]; /* main crcs, last lines crc, lengths, alignment */
   unsigned long *vec_crc, *vec_crc_eof, *vec_len, crc_y=-1, crc_uv=-1;

   /* pointers for crc calculation */
   vec_crc = (unsigned long*)(((unsigned long)vec_mem + 31) & ~31);
   vec_crc_eof = vec_crc + 16;
   vec_len = vec_crc_eof + 16;
   n_stripes = (img->width + (VC_IMAGE_YUV_UV_STRIPE_WIDTH - 1)) / VC_IMAGE_YUV_UV_STRIPE_WIDTH;

   /* for ease of implementation we'll crc whole lines & do a second pass to get
      the rest of the lines that are needed */

   stripe_area = img->pitch;
   uv_stripe_area = img->pitch;
   baseptr = (unsigned char *)img->image_data;
   crc_height = img->height - ct - cb;
   crc_width = img->width - cl - cr;

   /* Calculate CRCs for the Y component */

   if (crc_height >= 16) {
      line_pitch = (crc_height >> 4) + ct; /* effectively number of lines to process */
#ifndef __CC_ARM
      /* initialise CRC vector */
      _vasm("vmov H32(0,0), 0");
      _vasm("vst  H32(0,0), (%r)", vec_crc);
#endif
      *vec_crc = crc_y;

      /* memory distance between start of each stripe */
      line_pitch_area = VC_IMAGE_YUV_UV_STRIPE_WIDTH * (crc_height >> 4);

#ifndef __CC_ARM
      _vasm("vld H32(32,0), (%r)", vec_crc);
#endif
      /* for each line */
      for (i=ct; i < line_pitch; ++i) {
         for (j=0; j < n_stripes; ++j) {
            ptr = baseptr + i * VC_IMAGE_YUV_UV_STRIPE_WIDTH + j * stripe_area;
            width = MIN(VC_IMAGE_YUV_UV_STRIPE_WIDTH, (img->width - (j * VC_IMAGE_YUV_UV_STRIPE_WIDTH)));
            if (j == 0) {
               ptr += cl;
               width -= cl;
            }
            if (j == n_stripes-1)
               width -= cr;
            vclib_crc32_update(16, ptr, line_pitch_area, width, state->poly);
         }
      }
#ifndef __CC_ARM
      _vasm("vst H32(32,0), (%r)", vec_crc);
#endif
      /* number of zeros to the end of the image area */
      /* on second thoughts make this to the end of the aligned block */
      k = (crc_height >> 4) * crc_width;
      for (j=0; j<16; ++j)
         vec_len[j] = (15 - j) * k;

      crc_y = vclib_crc32_combine(vec_crc, vec_len, &state->comb_tab[0][0]);
   }

   if (crc_height & 15) {
      baseptr = baseptr + ((crc_height & ~15) + ct) * VC_IMAGE_YUV_UV_STRIPE_WIDTH;

#ifndef __CC_ARM
      /* init crc's */
      _vasm("vmov H32(0,0), 0");
      _vasm("vst  H32(0,0), (%r)", vec_crc_eof);
#endif
      *vec_crc_eof = crc_y;

      k = crc_height & 15;
#ifndef __CC_ARM
      _vasm("vld H32(32,0), (%r)", vec_crc_eof);
#endif
      for (j=0; j < n_stripes; ++j) {
         ptr = baseptr + j * stripe_area;
         width = MIN(VC_IMAGE_YUV_UV_STRIPE_WIDTH, (img->width - (j * VC_IMAGE_YUV_UV_STRIPE_WIDTH)));
         if (j == 0) {
            ptr += cl;
            width -= cl;
         }
         if (j == n_stripes - 1)
            width -= cr;
         vclib_crc32_update(k, ptr, VC_IMAGE_YUV_UV_STRIPE_WIDTH, width, state->poly);
      }
#ifndef __CC_ARM
      _vasm("vst H32(32,0), (%r)", vec_crc_eof);
#endif
      /* zero unused crcs & set length to zero to remove from the calculation */
      k = (k - 1) * crc_width;
      for (j=0; j < 16 && k >= 0; ++j, k -= crc_width)
         vec_len[j] = k;
      for (; j < 16; ++j) {
         vec_crc_eof[j] = 0;
         vec_len[j] = 0;
      }

      /* this should be a delta for the crc */
      crc_y = vclib_crc32_combine(vec_crc_eof, vec_len, &state->comb_tab[0][0]);
   }



   crc_height = ((img->height - cb) >> 1) - (ct >> 1);
   baseptr = (unsigned char *)img->extra.uv.u;

   /* CRCs for UV components */
   if (crc_height >= 16) {
      line_pitch = (crc_height >> 4) + (ct >> 1);
#ifndef __CC_ARM
      /* initialise crc vector */
      _vasm("vmov H32(0,0), 0");
      _vasm("vst  H32(0,0), (%r)", vec_crc);
#endif
      *vec_crc = crc_uv;

      /* memory distance between start of each stripe */
      line_pitch_area = VC_IMAGE_YUV_UV_STRIPE_WIDTH * (crc_height >> 4);
#ifndef __CC_ARM
      _vasm("vld H32(32,0), (%r)", vec_crc);
#endif
      for (i=ct>>1; i < line_pitch; ++i) {
         for (j=0; j < n_stripes; ++j) {
            ptr = baseptr + i * VC_IMAGE_YUV_UV_STRIPE_WIDTH + j * uv_stripe_area;
            width = MIN(VC_IMAGE_YUV_UV_STRIPE_WIDTH, (img->width - (j * VC_IMAGE_YUV_UV_STRIPE_WIDTH)));
            if (j == 0) {
               ptr += cl;
               width -= cl;
            }
            if (j == n_stripes-1)
               width -= cr;
            vclib_crc32_update(16, ptr, line_pitch_area, width, state->poly);
         }
      }
#ifndef __CC_ARM
      _vasm("vst H32(32,0), (%r)", vec_crc);
#endif
      /* number of zeros to the end of the image area */
      /* on second thoughts make this to the end of the aligned block */
      k = (crc_height >> 4) * crc_width;
      for (j=0; j<16; ++j)
         vec_len[j] = (15 - j) * k;

      crc_uv = vclib_crc32_combine(vec_crc, vec_len, &state->comb_tab[0][0]);
   }

   if (crc_height & 15) {
      /* any orphaned lines - where they weren't so easy to handle - handle 1 line
         per ppu & then update the CRC */
      baseptr = baseptr + ((crc_height & ~15) + (ct >> 1)) * VC_IMAGE_YUV_UV_STRIPE_WIDTH;
#ifndef __CC_ARM
      /* init crc's */
      _vasm("vmov H32(0,0), 0");
      _vasm("vst  H32(0,0), (%r)", vec_crc_eof);
#endif
      *vec_crc_eof = crc_uv;

      k = crc_height & 15;
#ifndef __CC_ARM
      _vasm("vld H32(32,0), (%r)", vec_crc_eof);
#endif
      for (j=0; j < n_stripes; ++j) {
         ptr = baseptr + j * uv_stripe_area;
         width = MIN(VC_IMAGE_YUV_UV_STRIPE_WIDTH, (img->width - (j * VC_IMAGE_YUV_UV_STRIPE_WIDTH)));
         if (j == 0) {
            ptr += cl;
            width -= cl;
         }
         if (j == n_stripes - 1)
            width -= cr;
         vclib_crc32_update(k, ptr, VC_IMAGE_YUV_UV_STRIPE_WIDTH, width, state->poly);
      }
#ifndef __CC_ARM
      _vasm("vst H32(32,0), (%r)", vec_crc_eof);
#endif
      /* zero unused crcs & set length to zero to remove from the calculation */
      k = (k - 1) * crc_width;
      for (j=0; j < 16 && k >= 0; ++j, k -= crc_width)
         vec_len[j] = k;
      for (; j < 16; ++j) {
         vec_crc_eof[j] = 0;
         vec_len[j] = 0;
      }

      /* this should be a delta for the crc */
      crc_uv = vclib_crc32_combine(vec_crc_eof, vec_len, &state->comb_tab[0][0]);
   }

   crc.y = crc_y;
   crc.uv = crc_uv;

   return crc;
}

/* set the field to 0=even, 1=odd */
static VC_IMAGE_CRC_T vc_image_calc_crc_yuv_uv_field(VC_IMAGE_BUF_T *img, int cl, int cr, int ct, int cb, int field, VCLIB_CRC32_STATE_T *state)
{
   VC_IMAGE_CRC_T crc;
   unsigned char *ptr, *baseptr;
   unsigned long crc_height, crc_width, stripe_area;
   unsigned long width, n_stripes, uv_stripe_area;
   long i;

   unsigned long vec_mem[16+16+16+8]; /* main crcs, last lines crc, lengths, alignment */
   unsigned long *vec_crc, *vec_crc_eof, *vec_len, crc_y=-1, crc_uv=-1;

   /* pointers for crc calculation */
   vec_crc = (unsigned long*)(((unsigned long)vec_mem + 31) & ~31);
   vec_crc_eof = vec_crc + 16;
   vec_len = vec_crc_eof + 16;
   n_stripes = (img->width + (VC_IMAGE_YUV_UV_STRIPE_WIDTH - 1)) / VC_IMAGE_YUV_UV_STRIPE_WIDTH;

   /* fudge crop-top line to allow for odd field */
   if(field) ct += 1;

   /* for ease of implementation we'll crc whole lines & do a second pass to get
      the rest of the lines that are needed */
   stripe_area = img->pitch;
   uv_stripe_area = img->pitch;
   baseptr = (unsigned char *)img->image_data;
   crc_height = (img->height - ct - cb + 1) >> 1;
   crc_width = img->width - cl - cr;

   /* Calculate CRCs for the Y components */
   if (crc_height > 0) {
      unsigned long nlines, mlines, nvalid, j, cpitch, lcrc = 0;

      nlines  = (crc_height + 15) >> 4;
      nvalid  = (crc_height + nlines - 1) / nlines;
      mlines  = crc_height - (nvalid - 1) * nlines;
      cpitch  = nlines * VC_IMAGE_YUV_UV_STRIPE_WIDTH * 2;
#ifndef __CC_ARM
      /* initialise CRC vector */
      _vasm("vmov H32(0,0), 0");
      _vasm("vst  H32(0,0), (%r)", vec_crc);
#endif
      *vec_crc = crc_y;

#ifndef __CC_ARM
      _vasm("vld H32(32,0), (%r)", vec_crc);
#endif
      /* for each line common to all PPUs */
      for (i=0; i < nlines; ++i) {
         /* per section in stripe */
         for (j=0; j < n_stripes; ++j) {
            ptr = baseptr + ((i * 2) + ct) * VC_IMAGE_YUV_UV_STRIPE_WIDTH + j * stripe_area;
            width = MIN(VC_IMAGE_YUV_UV_STRIPE_WIDTH, (img->width - (j * VC_IMAGE_YUV_UV_STRIPE_WIDTH)));
            if (j == 0) {
               ptr += cl;
               width -= cl;
            }
            if (j == n_stripes-1)
               width -= cr;
            vclib_crc32_update(nvalid - (i >= mlines), ptr, cpitch, width, state->poly);
         }
#ifndef __CC_ARM
         if(i == mlines-1)
            lcrc = _vasm("vbitplanes -, %r SETF \n\t"
                         "vmov -, H32(32,0) IFNZ USUM %D", 1 << nvalid - 1);
#endif
      }
#ifndef __CC_ARM
      _vasm("vst H32(32,0), (%r)", vec_crc);
#endif
      vec_crc[nvalid-1] = lcrc;
      for(i = nvalid; i < 16; ++i)
         vec_crc[i] = 0;

      if(nvalid > 1) {
         for(i = 15; i >= (nvalid-1); --i)
            vec_len[i] = 0;
         for(i = nvalid-2, j = mlines*crc_width; i >= 0; --i, j += nlines*crc_width)
            vec_len[i] = j;
         crc_y = vclib_crc32_combine(vec_crc, vec_len, &state->comb_tab[0][0]);
      } else { /* only 1 valid PPU so no need to combine parts */
         crc_y = vec_crc[0];
      }
   }

   crc_height = (img->height - ct - cb + 1) >> 2;
   baseptr = (unsigned char *)img->extra.uv.u;

   /* CRCs for UV components - crc_uv */
   if (crc_height > 0) {
      unsigned long nlines, mlines, nvalid, j, cpitch, lcrc = 0;

      nlines  = (crc_height + 15) >> 4;
      nvalid  = (crc_height + nlines - 1) / nlines;
      mlines  = crc_height - (nvalid - 1) * nlines;
      cpitch  = nlines * VC_IMAGE_YUV_UV_STRIPE_WIDTH * 2;
#ifndef __CC_ARM
      /* initialise CRC vector */
      _vasm("vmov H32(0,0), 0");
      _vasm("vst  H32(0,0), (%r)", vec_crc);
#endif
      *vec_crc = crc_uv;

#ifndef __CC_ARM
      _vasm("vld H32(32,0), (%r)", vec_crc);
#endif
      /* for each line common to all PPUs */
      for (i=0; i < nlines; ++i) {
         /* per section in stripe */
         for (j=0; j < n_stripes; ++j) {
            ptr = baseptr + ((i * 2) + ct) * VC_IMAGE_YUV_UV_STRIPE_WIDTH + j * uv_stripe_area;
            width = MIN(VC_IMAGE_YUV_UV_STRIPE_WIDTH, (img->width - (j * VC_IMAGE_YUV_UV_STRIPE_WIDTH)));
            if (j == 0) {
               ptr += cl;
               width -= cl;
            }
            if (j == n_stripes-1)
               width -= cr;
            vclib_crc32_update(nvalid - (i >= mlines), ptr, cpitch, width, state->poly);
         }
#ifndef __CC_ARM
         if(i == mlines-1)
            lcrc = _vasm("vbitplanes -, %r SETF \n\t"
                         "vmov -, H32(32,0) IFNZ USUM %D", 1 << nvalid - 1);
#endif
      }
#ifndef __CC_ARM
      _vasm("vst H32(32,0), (%r)", vec_crc);
#endif
      vec_crc[nvalid-1] = lcrc;
      for(i = nvalid; i < 16; ++i)
         vec_crc[i] = 0;

      if(nvalid > 1) {
         for(i = 15; i >= (nvalid-1); --i)
            vec_len[i] = 0;
         for(i = nvalid-2, j = mlines*crc_width; i >= 0; --i, j += nlines*crc_width)
            vec_len[i] = j;
         crc_uv = vclib_crc32_combine(vec_crc, vec_len, &state->comb_tab[0][0]);
      } else { /* only 1 valid PPU so no need to combine parts */
         crc_uv = vec_crc[0];
      }
   }

   crc.y  = crc_y;
   crc.uv = crc_uv;

   return crc;
}


VC_IMAGE_CRC_T vc_image_calc_crc_interlaced(VC_IMAGE_BUF_T *img, int cl, int cr, int ct, int cb, int field)
{
   /* combination table */
   static VCLIB_CRC32_STATE_T state;
#pragma align_to(32, state);
   static int table_inited = -1;

   VC_IMAGE_CRC_T crc;
   int width, height, pitch, bpp;

   vcos_assert(is_valid_vc_image_buf(img, img->type));

   if (table_inited < 0) {
      table_inited = 0;
      vclib_crc32_init(&state, VPP_CRC_POLY);
      table_inited = 1;
   }
   /* wait in-case another thread is initialising the table currently; it's a bit nasty
      but hopefully will avoid most race conditions ... */
   while (table_inited <= 0)
      continue;

   width  = img->width  - (cl + cr);
   height = img->height - (ct + cb);
   pitch  = img->pitch;

   /* currently field CRCs have only been implemented for sand yuv formats */
   vcos_assert(img->type == VC_IMAGE_YUV_UV || img->type == VC_IMAGE_YUV_UV32 || field < 0);

   switch (img->type)
   {
   case VC_IMAGE_YUV_UV: case VC_IMAGE_YUV_UV32:
      if(field < 0)
         return vc_image_calc_crc_yuv_uv(img, cl, cr, ct, cb, &state);
      else if(!field)
         return vc_image_calc_crc_yuv_uv_field(img, cl, cr, ct, cb, 0, &state);
      else
         return vc_image_calc_crc_yuv_uv_field(img, cl, cr, ct, cb, 1, &state);

   case VC_IMAGE_YUV420:
   case VC_IMAGE_YUV422:
   case VC_IMAGE_YUV422PLANAR:
      crc.y = vclib_crc32_2d(&state, -1,
                  vc_image_get_y(img) + pitch * ct + cl,
                  width, height,
                  pitch);

      /* we're not going to try to checksum half pixels. */
      vcos_assert(((cl | width) & 1) == 0);
      width >>= 1;
      cl >>= 1;
      if (img->type == VC_IMAGE_YUV420)
      {
         vcos_assert(((ct | height) & 1) == 0);
         height >>= 1;
         ct >>= 1;
         pitch >>= 1;
      }
      else if (img->type == VC_IMAGE_YUV422PLANAR)
         pitch >>= 1;

      crc.uv = vclib_crc32_2d(&state, -1,
                  vc_image_get_u(img) + pitch * ct + cl,
                  width, height, pitch);
      crc.uv = vclib_crc32_2d(&state, crc.uv,
                  vc_image_get_v(img) + pitch * ct + cl,
                  width, height, pitch);
      return crc;

   case VC_IMAGE_ARGB8888:
   case VC_IMAGE_XRGB8888:
   case VC_IMAGE_RGBA32:
   case VC_IMAGE_RGB888:
   case VC_IMAGE_RGB565:
   case VC_IMAGE_RGBA565:
   case VC_IMAGE_RGBA16:
   case VC_IMAGE_8BPP:
      /* if we're doing sub-byte images, then left and right edges have to be byte-aligned */
      bpp   = vc_image_bits_per_pixel((VC_IMAGE_TYPE_T)img->type);
      vcos_assert(((cl | width) * bpp & 7) == 0);
      cl    = cl * bpp >> 3;
      width = width * bpp >> 3;

      crc.uv = crc.y = vclib_crc32_2d(&state, -1,
                           (char const *)img->image_data + pitch * ct + cl,
                           width, height, pitch);
      return crc;

   case VC_IMAGE_TF_RGB565:
      /* Cannot CRC sub-images of T-Format */
      vcos_assert(!(cl | cr | ct | cb));

      crc.uv = crc.y = vclib_crc32(&state, -1,
                                   (char const *)img->image_data, img->size);
      return crc;
   }

   vcos_assert(!"No CRC implementation for img->type.");
   crc.y = crc.uv = 0;
   return crc;
}


void vc_image_convert_rgb2yuv(unsigned short *rgb, unsigned char *Y, unsigned char *U, unsigned char *V,
                           int rgb_pitch_pixels, int y_pitch_bytes, int width_pixels, int height) {
   extern void vc_block_rgb2yuv(unsigned short *rgb_ptr, int rgb_pitch_bytes,
                                   unsigned char *Y_ptr, unsigned char *U_ptr, unsigned char *V_ptr, int y_pitch_bytes);
   int w, h;
   int rgb_pitch_bytes = rgb_pitch_pixels<<1;
   int uv_pitch_bytes = y_pitch_bytes>>1;
   for (h = 0; h < height; h += 16) {
      unsigned short *rgb_ptr = rgb + h*rgb_pitch_pixels;
      unsigned char *Y_ptr = Y + h*y_pitch_bytes;
      unsigned char *U_ptr = U + (h>>1)*uv_pitch_bytes;
      unsigned char *V_ptr = V + (h>>1)*uv_pitch_bytes;
      for (w = 0; w < width_pixels; w += 16, rgb_ptr += 16, Y_ptr += 16, U_ptr += 8, V_ptr += 8) {
         vc_block_rgb2yuv(rgb_ptr, rgb_pitch_bytes, Y_ptr, U_ptr, V_ptr, y_pitch_bytes);
      }
   }
}

void vc_image_convert_yuv2rgb(unsigned char *datay, unsigned char *datau
                           , unsigned char *datav, int realwidth, int realheight
                           , unsigned short *buffer, int screenwidth, int screenheight)
{
   extern void vc_yuv16_hflip(unsigned char *Y, unsigned char *U, unsigned char *V,
                                 unsigned short *dest, int yspacing, int uvspacing, int destspacing);
   //int ysize = realwidth*realheight;
   //unsigned char *datay = currentbuffer, *datau = currentbuffer + ysize, *datav = currentbuffer + ysize + (ysize>>2);
   int xlimit=min(realwidth, screenwidth),ylimit=min(realheight, screenheight);
   int x,y;
   for (x=0; x<xlimit; x+=16)
      for (y=0; y<ylimit; y+=16)
         vc_yuv16_hflip(
            &datay[x+(y)*realwidth],
            &datau[(x>>1)+((y*realwidth)>>2)],
            &datav[(x>>1)+((y*realwidth)>>2)],
            &buffer[xlimit-16-x+y*screenwidth],
            realwidth,(realwidth>>1),screenwidth*sizeof(unsigned short));
}

/*
Vector routine to subsample an image ready for CME
Should probably dcache flush after this if using via uncached alias?
*/
// use by openmaxil so pulled into the common part from vc_image_yuvuv128.c - removes duplicated code
void vc_subsample(VC_IMAGE_BUF_T *sub,VC_IMAGE_BUF_T *cur)
{
   int x, y, width_64, height_16, cur_slab, sub_slab, sub_pitch;

   // Make half sized image. Currently requires source to have pitch 128, dest 32 or 128.
   vcos_assert(is_valid_vc_image_buf(cur, VC_IMAGE_YUV_UV));
   vcos_assert(is_valid_vc_image_buf(sub, VC_IMAGE_YUV_UV32) || is_valid_vc_image_buf(sub, VC_IMAGE_YUV_UV));
   vcos_assert(sub->width*2 >= cur->width);
   vcos_assert(sub->height*2 >= cur->height);

   width_64   = (cur->width + 63) >> 6;
   height_16  = (cur->height + 15) >> 4;
   sub_pitch  = (sub->type == VC_IMAGE_YUV_UV32) ? 32 : 128;
   cur_slab   = cur->pitch;
   sub_slab   = sub->pitch;
   
   for (x = 0; x < width_64; ++x) {
      const unsigned char *cy, *cc;
      unsigned char *sy, *sc;
      cy = (const unsigned char *)vc_image_get_y(cur) + (x>>1)*cur_slab + (x&1)*64;
      cc = (const unsigned char *)vc_image_get_u(cur) + (x>>1)*cur_slab + (x&1)*64;
      if (sub_pitch == 128) {
         sy = (unsigned char *)vc_image_get_y(sub) + (x>>2)*sub_slab + (x&3)*32;
         sc = (unsigned char *)vc_image_get_u(sub) + (x>>2)*sub_slab + (x&3)*32;
      }
      else {
         sy = (unsigned char *)vc_image_get_y(sub) + x*sub_slab;
         sc = (unsigned char *)vc_image_get_u(sub) + x*sub_slab;
      }
      for (y = 0; y < height_16; ++y) {
         yuvuv_downsample_4mb(sy, sc, sub_pitch, cy, cc, 128);
         sy += sub_pitch*8;
         sc += sub_pitch*4;
         cy += 128*16;
         cc += 128*8;
      }
   }

} /* vc_subsample */


/* ----------------------------------------------------------------------
 * the object may have been allocated from the relocatable mempool; provide a
 * lightweight api call to lock the underlying image data memory in place
 * before use.
 * -------------------------------------------------------------------- */
void vc_image_lock( VC_IMAGE_BUF_T *dst, const VC_IMAGE_T * src )
{
   //vcos_assert(is_valid_vc_image_hndl(dst, dst->type)); /* untested assertion */

   if (src->pool_object != NULL)
   {
      // If this fires, then we've got an image header potentially being shared
      // between different threads - locking is unsafe in this context.
      //
      // In general, *any* situation where two threads lock the same VC_IMAGE_T
      // header is going to be dangerous, but this is the only one we can reliably
      // detect.
      vcos_assert(dst != src);
   }

   if (dst != src) *dst = *src;

   /* If we try to lock an image that isn't part of the relocatable heap, then
    * just pass the object through unmolested.  All this function is doing is
    * providing an image that won't move away before you call
    * vc_image_unlock(), and it can do that perfectly well for non-lockable
    * objects by taking no action.
    *
    * There's no document saying that this is inappropriate.
    */
   if (src->mem_handle != MEM_INVALID_HANDLE)
   {
      vcos_assert(mem_get_size(src->mem_handle) >= src->size);

      dst->image_data = (void *) mem_lock( src->mem_handle );
      // recalculate all other pointers (eg. ->extra.uv.u)
      vc_image_calculate_pointers( dst );
   }

   vcos_assert(is_valid_vc_image_buf(dst, dst->type));
}

/* ----------------------------------------------------------------------
 * the object may have been allocated from the relocatable mempool; provide a
 * lightweight api call to unlock the memory.  after calling this, the image
 * ->data may move around, and thus is no longer valid
 * -------------------------------------------------------------------- */
void vc_image_unlock( VC_IMAGE_BUF_T *img )
{
   vcos_assert(is_valid_vc_image_buf(img, img->type));

   if (img->mem_handle != MEM_INVALID_HANDLE)
   {
      mem_unlock( img->mem_handle );
      // zero all pointers in an attempt to catch misbehaving users
      vc_image_zero_pointers( img );
   }
}
/* ----------------------------------------------------------------------
 * the object may have been allocated from the relocatable mempool; provide a
 * lightweight api call to lock the underlying image data memory in place
 * before use, such that an aggressive compaction will skip over it.
 * -------------------------------------------------------------------- */
void vc_image_lock_perma( VC_IMAGE_BUF_T *dst, const VC_IMAGE_T * src )
{
   if (src->pool_object != NULL)
   {
      // If this fires, then we've got an image header potentially being shared
      // between different threads - locking is unsafe in this context.
      //
      // In general, *any* situation where two threads lock the same VC_IMAGE_T
      // header is going to be dangerous, but this is the only one we can reliably
      // detect.
      vcos_assert(dst != src);
   }

   if (dst != src) *dst = *src;

   /* If we try to lock an image that isn't part of the relocatable heap, then
    * just pass the object through unmolested.  All this function is doing is
    * providing an image that won't move away before you call
    * vc_image_unlock_perma(), and it can do that perfectly well for non-lockable
    * objects by taking no action.
    *
    * There's no document saying that this is inappropriate.
    */
   if (src->mem_handle != MEM_INVALID_HANDLE)
   {
      vcos_assert(mem_get_size(src->mem_handle) >= src->size);

      dst->image_data = (void *) mem_lock_perma( src->mem_handle );
      // recalculate all other pointers (eg. ->extra.uv.u)
      vc_image_calculate_pointers( dst );
   }

   vcos_assert(is_valid_vc_image_buf(dst, dst->type));
}

/* ----------------------------------------------------------------------
 * the object may have been allocated from the relocatable mempool; provide a
 * lightweight api call to unlock the memory previously locked by
 * vc_image_lock_perma.  after calling this, the image->data may move around,
 * and thus is no longer valid
 * -------------------------------------------------------------------- */
void vc_image_unlock_perma( VC_IMAGE_BUF_T *img )
{
   vcos_assert(is_valid_vc_image_buf(img, img->type));

   if (img->mem_handle != MEM_INVALID_HANDLE)
   {
      mem_unlock_perma( img->mem_handle );
      // zero all pointers in an attempt to catch misbehaving users
      vc_image_zero_pointers( img );
   }
}


VC_IMAGE_T *vc_image_relocatable_alloc(VC_IMAGE_TYPE_T type, const char *description, long width, long height, MEM_FLAG_T flags, VC_IMAGE_PROPERTY_T prop, ...)
{
   va_list proplist;
   VC_IMAGE_T temp;
   MEM_HANDLE_T h;
   int align = -1;
   int padding = 0;
   char use_refptr = 0, use_clearvalue = 0;
   void const *refptr = NULL;
   unsigned long clearvalue = 0;
   size_t required;
   int rc;

   VC_IMAGE_T *img = (VC_IMAGE_T*)rtos_prioritymalloc( sizeof(VC_IMAGE_T),
                                          RTOS_ALIGN_DEFAULT,
                                          RTOS_PRIORITY_UNIMPORTANT, description);
   if (!img)
      return NULL;

   va_start(proplist, prop);
   while (prop != VC_IMAGE_PROP_END)
   {
      int breaknow = 0;

      switch (prop)
      {
      case VC_IMAGE_PROP_NONE:
         (void)va_arg(proplist, long);
         break;
      case VC_IMAGE_PROP_ALIGN:
         align = (int)va_arg(proplist, long);
         break;
      case VC_IMAGE_PROP_STAGGER:
         vcos_assert(!"STAGGER not implemented");
         (void)va_arg(proplist, long);
         break;
      case VC_IMAGE_PROP_PADDING:
         padding = (int)va_arg(proplist, long);
         break;
      case VC_IMAGE_PROP_PRIORITY:
         /* PRIORITY ignored */
         (void)va_arg(proplist, long);
         break;
      case VC_IMAGE_PROP_MALLOCFN:
         /* MALLOCFN ignored */
         (void)va_arg(proplist, void *);
         break;
      case VC_IMAGE_PROP_CACHEALIAS:
         use_refptr = 1;
         refptr = va_arg(proplist, void const *);
         break;
      case VC_IMAGE_PROP_CLEARVALUE:
         use_clearvalue = 1;
         clearvalue = va_arg(proplist, unsigned long);
         break;
      default:
         breaknow = 1;
         break;
      }
      if (breaknow)
         break;

      prop = va_arg(proplist, VC_IMAGE_PROPERTY_T);
   }
   rc = vc_image_v1configure(&temp, type, width, height, prop, proplist);
   va_end(proplist);

   if (!vcos_verify(rc == 0))
   {
      rtos_priorityfree(img);
      return NULL;
   }

   if (align < 4)
      align = fix_alignment(type, 1);

   if (use_refptr == 0)
      refptr = default_cache_alias(&temp, (void *)-1);

   vcos_assert(align >= 4);
   vcos_assert(_count(align) == 1);

   required = vc_image_required_size(&temp);
   vcos_assert(required != 0);

   required += padding;

   if ((flags & MEM_FLAG_ZERO) != 0)
   { // MEM_FLAG_ZERO takes precedence over VC_IMAGE_PROP_CLEARVALUE
      use_clearvalue = 1;
      clearvalue = 0;
      flags = (MEM_FLAG_T)(flags & (~MEM_FLAG_ZERO));
   }

   if(use_clearvalue)
   {
      flags = (MEM_FLAG_T)(flags | MEM_FLAG_NO_INIT);
   }

   if (!(flags & (MEM_FLAG_COHERENT | MEM_FLAG_DIRECT)))
   {
      if (IS_ALIAS_COHERENT(refptr))
         flags = (MEM_FLAG_T)(flags | MEM_FLAG_COHERENT);
      if (IS_ALIAS_DIRECT(refptr))
         flags = (MEM_FLAG_T)(flags | MEM_FLAG_DIRECT);
   }

   h = mem_alloc(required, align, flags, description);
   if (h == MEM_INVALID_HANDLE)
   {
      rtos_priorityfree(img);
      return NULL;
   }

#ifndef NDEBUG
   {
      void *ptr = mem_lock(h);
      vcos_assert(ptr);
      mem_unlock(h);
   }
#endif

   if (use_clearvalue)
   {
      void *image_data = mem_lock(h);
      vclib_memset4(ALIAS_ANY_NONALLOCATING(image_data), clearvalue, temp.size >> 2);
      mem_unlock(h);
   }

   *img = temp;
   img->mem_handle = h;

   return img;
}

void vc_image_relocatable_free(VC_IMAGE_T *img)
{
   if (img->mem_handle != MEM_ZERO_SIZE_HANDLE &&
       img->mem_handle != MEM_INVALID_HANDLE)
   {
      vcos_assert(is_valid_vc_image_hndl(img, img->type)); // unlocked?
      mem_release(img->mem_handle);
   }
   rtos_priorityfree(img);
}

/******************************************************************************
 Static  Functions
 *****************************************************************************/

/* ----------------------------------------------------------------------
 * zero all relocatable pointers in the vc_image, in an attempt to
 * catch misbehaving users
 * -------------------------------------------------------------------- */
static void vc_image_zero_pointers( VC_IMAGE_BUF_T *img )
{
   img->image_data = NULL;
   img->metadata = NULL;

   switch( img->type ) {

   case_VC_IMAGE_ANY_YUV:
      img->extra.uv.u = NULL;
      img->extra.uv.v = NULL;
      break;

   case VC_IMAGE_4BPP:
   case VC_IMAGE_8BPP:
      // FIXME: is this separately malloced or part of ->image_data?
      //img->extra.pal.palette = NULL;
      break;

   case_VC_IMAGE_ANY_TFORMAT:
      // FIXME: is this separately malloced or part of ->image_data?
      //img->extra.tf.palette = NULL;
      break;

   case VC_IMAGE_OPENGL:
      // FIXME: is this separately malloced or part of ->image_data?
      //img->extra.opengl.palette = NULL;
      break;
   }

}

#pragma weak vc_pool_offsetof
// this should only get called when linking with vc_pool, so it should use real version from that library
int vc_pool_offsetof(VC_POOL_OBJECT_T *object)
{
   (void)object;
   vcos_demand(!"unsupported function");
   return 0;
}

/* ----------------------------------------------------------------------
 * given a valid ->image_data, recalculate all pointers in the vc_image
 * -------------------------------------------------------------------- */
static void vc_image_calculate_pointers( VC_IMAGE_BUF_T *img )
{
   // if this image was allocated from a pool, and the pool is of
   // type 'subdividable', then the base address may be offset
   if ( img->pool_object )
      img->image_data = (unsigned char *)img->image_data + vc_pool_offsetof( img->pool_object );

   switch( img->type ) {

   case_VC_IMAGE_ANY_YUV:
      vc_image_set_image_data_yuv( img, img->size, img->image_data, NULL, NULL );
      break;

   case VC_IMAGE_4BPP:
   case VC_IMAGE_8BPP:
      // FIXME: what goes here?
      break;

   case_VC_IMAGE_ANY_TFORMAT:
      // FIXME: what goes here?
      break;

   case VC_IMAGE_OPENGL:
      // FIXME: what goes here?
      break;
   }

   if ( img->metadata_size )
      img->metadata = (VC_METADATA_HEADER_T *)((unsigned)img->image_data + img->size );
}

void vc_image_parfree(VC_IMAGE_BUF_T *p)
{
   if (p != NULL)
   {
      vcos_demand(p->mem_handle == MEM_INVALID_HANDLE);
      vc_image_free(p);
   }
}


/* The End */
