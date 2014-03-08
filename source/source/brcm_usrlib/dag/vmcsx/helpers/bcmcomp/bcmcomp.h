/* ============================================================================
Copyright (c) 2009-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef BCMCOMP_H
#define BCMCOMP_H

#define BCM_COMP_API

#include "bcmcomp_stdint.h"

/* Definitions of color format modes, used together with color formats */
typedef enum {
//    BCM_COMP_FORMAT_SWAP_ENDIANNESS     = (1 <<  9), /* swaps the order */
    BCM_COMP_FORMAT_LINEAR_SPACE        = (1 << 10), /* linear color space */
    BCM_COMP_FORMAT_PREMULTIPLIED       = (1 << 11), /* alpha premultiplied */
//    BCM_COMP_FORMAT_INVERT_ALPHA        = (1 << 12), /* inverts alpha */
    BCM_COMP_FORMAT_DISABLE_ALPHA       = (1 << 13), /* disables alpha */
    //BCM_COMP_FORMAT_INTERLACED          = (1 << 14), /* YUV line-interlaced */
//    BCM_COMP_FORMAT_TRANSPARENT         = (1 << 15), /* YUV 1-bit alpha in Y */
    BCM_COMP_FORMAT_FULL_RANGE          = (1 << 16), /* YUV full range */
    BCM_COMP_FORMAT_HDTV_SPACE          = (1 << 17), /* YUV ITU-R BT-709 */
} BCM_COMP_FORMAT_MODE;

/* Definitions of supported RGB formats, used in BCM_COMP_RGB_SURFACE_DEF.
 * The bits of each color channel are packed into a machine word
 * representing a single pixel from left to right (MSB to LSB) in the
 * order indicated by format name. For the sub-byte formats the pixels
 * are packed into bytes from left to right (MSbit to LSBit).
 * If BCM_COMP_FORMAT_SWAP_ENDIANNESS bit is set, the order of color channels
 * within a minimal machine word representing a pixel is reversed.
 * For sub-byte formats the order of pixels within byte is reversed.
 * If BCM_COMP_FORMAT_LINEAR_SPACE bit is set, the color space of
 * the formats below is considered linear, if applicable.
 * If BCM_COMP_FORMAT_PREMULTIPLIED bit is set, the color channels
 * are premultiplied with the alpha, if applicable.
 * If BCM_COMP_FORMAT_INVERT_ALPHA bit is set, the alpha interpretation
 * is inverted: 0 - opaque, 1 - transparent, if applicable.
 * If BCM_COMP_FORMAT_DISABLE_ALPHA bit is set, the alpha channel serves
 * as a placeholder and is ignored during composition, if applicable. */
typedef enum {
//    BCM_COMP_COLOR_FORMAT_1             = 0,   /* 1-bit alpha/color expansion */

//    BCM_COMP_COLOR_FORMAT_2_PALETTE     = 1,   /* 2-bit indices for palette */
//    BCM_COMP_COLOR_FORMAT_4_PALETTE     = 2,   /* 4-bit indices for palette */
//    BCM_COMP_COLOR_FORMAT_8_PALETTE     = 3,   /* 8-bit indices for palette */

//    BCM_COMP_COLOR_FORMAT_2_L           = 4,   /* 2-bit grayscale */
//    BCM_COMP_COLOR_FORMAT_4_L           = 5,   /* 4-bit grayscale */
//    BCM_COMP_COLOR_FORMAT_8_L           = 6,   /* 8-bit grayscale */

//    BCM_COMP_COLOR_FORMAT_2_A           = 7,   /* 2-bit alpha only */
//    BCM_COMP_COLOR_FORMAT_4_A           = 8,   /* 4-bit alpha only */
    BCM_COMP_COLOR_FORMAT_8_A           = 9,   /* 8-bit alpha only */

    //BCM_COMP_COLOR_FORMAT_444_RGB       = 10,  /* 12-bit colors */
    BCM_COMP_COLOR_FORMAT_565_RGB       = 11,  /* 16-bit colors */
    BCM_COMP_COLOR_FORMAT_888_RGB       = 12,  /* 24-bit colors */

    BCM_COMP_COLOR_FORMAT_1555_ARGB     = 13,  /* 16-bit colors (1-bit alpha) */
    BCM_COMP_COLOR_FORMAT_4444_ARGB     = 14,  /* 16-bit colors (4-bit alpha) */
//    BCM_COMP_COLOR_FORMAT_8565_ARGB     = 15,  /* 24-bit colors (8-bit alpha) */
    BCM_COMP_COLOR_FORMAT_8888_ARGB     = 16,  /* 32-bit colors (8-bit alpha) */

    BCM_COMP_COLOR_FORMAT_5551_RGBA     = 17,  /* 16-bit colors (1-bit alpha) */
    BCM_COMP_COLOR_FORMAT_4444_RGBA     = 18,  /* 16-bit colors (4-bit alpha) */
//    BCM_COMP_COLOR_FORMAT_5658_RGBA     = 19,  /* 24-bit colors (8-bit alpha) */
    BCM_COMP_COLOR_FORMAT_8888_RGBA     = 20,  /* 32-bit colors (8-bit alpha) */

   BCM_COMP_COLOR_FORMAT_888_BGR        = 21, /* NEW. 24-bit colors */

   BCM_COMP_COLOR_FORMAT_8888_ABGR      = 22, /* for egl window surface support */

    /* derived RGB color formats (base format + mode bits) */

} BCM_COMP_RGB_FORMAT;

/* Definitions of supported YUV formats, used in BCM_COMP_YUV_SURFACE_DEF.
 * Each of Y, U and V channels takes 1 byte and therefore is
 * individually addressable. The definitions below show how Y, U and V
 * channels are packed into macro-pixels for each particular format.
 * The order is from left (smaller byte addresses) to right (larger
 * byte addresses). The first three digits (4xx) denote the chromaticity
 * subsampling in standard YUV notation. The digits in the macro-pixel
 * denote that the whole block (from the previous digit or from the
 * beginning) has to be repeated the number of times. Underscores
 * between Y, U and V channels are used to describe separate planes for
 * planar YUV formats. Formats are mapped to numbers so that future
 * versions with various YUV permutations are easy to add.
 * If BCM_COMP_FORMAT_INTERLACED bit is set, the line order is
 * interlaced: 0,2,4,...1,3,5... if applicable.
 * If BCM_COMP_FORMAT_TRANSPARENT bit is set, the least significant
 * bit of Y channel serves as alpha: 0 - transparent, 1 - opaque.
 * If BCM_COMP_FORMAT_FULL_RANGE bit is set, then YUV values are interpreted
 * as full range instead of limited range which is default.
 * If BCM_COMP_FORMAT_HDTV_SPACE bit is set, then YUV values are interpreted
 * according to ITU-R BT-709 instead of ITU-R BT-601 which is default.
 */
typedef enum {
    //BCM_COMP_COLOR_FORMAT_411_YYUYYV    = 110, /* packed, 12-bit         */
    //BCM_COMP_COLOR_FORMAT_411_YUYYVY    = 111, /* packed, 12-bit         */
    //BCM_COMP_COLOR_FORMAT_411_UYYVYY    = 112, /* packed, 12-bit, "Y411" */
    //BCM_COMP_COLOR_FORMAT_411_YUYV2Y4   = 116, /* packed, 12-bit         */
    //BCM_COMP_COLOR_FORMAT_411_UYVY2Y4   = 117, /* packed, 12-bit, "Y41P" */

    BCM_COMP_COLOR_FORMAT_422_YUYV      = 120, /* packed, 16-bit, "YUY2" */
    BCM_COMP_COLOR_FORMAT_422_UYVY      = 121, /* packed, 16-bit, "UYVY" */
    BCM_COMP_COLOR_FORMAT_422_YVYU      = 122, /* packed, 16-bit, "YVYU" */
    BCM_COMP_COLOR_FORMAT_422_VYUY      = 123, /* packed, 16-bit         */

    BCM_COMP_COLOR_FORMAT_444_YUV       = 130, /* packed, 24-bit         */
    //BCM_COMP_COLOR_FORMAT_444_UYV       = 131, /* packed, 24-bit, "IYU2" */
    //BCM_COMP_COLOR_FORMAT_444_AYUV      = 136, /* packed, 24-bit, "AYUV" */

    //BCM_COMP_COLOR_FORMAT_410_Y_UV      = 150, /* planar, Y + interleaved UV */
    //BCM_COMP_COLOR_FORMAT_411_Y_UV      = 151, /* planar, Y + interleaved UV */
    BCM_COMP_COLOR_FORMAT_420_Y_UV      = 152, /* planar, Y + interleaved UV */
    BCM_COMP_COLOR_FORMAT_422_Y_UV      = 153, /* planar, Y + interleaved UV */
    //BCM_COMP_COLOR_FORMAT_444_Y_UV      = 154, /* planar, Y + interleaved UV */

    //BCM_COMP_COLOR_FORMAT_410_Y_VU      = 160, /* planar, Y + interleaved VU */
    //BCM_COMP_COLOR_FORMAT_411_Y_VU      = 161, /* planar, Y + interleaved VU */
    BCM_COMP_COLOR_FORMAT_420_Y_VU      = 162, /* planar, Y + interleaved VU */
    //BCM_COMP_COLOR_FORMAT_422_Y_VU      = 163, /* planar, Y + interleaved VU */
    //BCM_COMP_COLOR_FORMAT_444_Y_VU      = 164, /* planar, Y + interleaved VU */

    //BCM_COMP_COLOR_FORMAT_410_Y_U_V     = 170, /* planar, Y + U + V separate */
    //BCM_COMP_COLOR_FORMAT_411_Y_U_V     = 171, /* planar, Y + U + V separate */
    BCM_COMP_COLOR_FORMAT_420_Y_U_V     = 172, /* planar, Y + U + V separate */
    BCM_COMP_COLOR_FORMAT_422_Y_U_V     = 173, /* planar, Y + U + V separate */
    //BCM_COMP_COLOR_FORMAT_444_Y_U_V     = 174, /* planar, Y + U + V separate */

    //BCM_COMP_COLOR_FORMAT_800_Y         = 190, /* planar, Y only, grayscale */

    /* derived YUV color formats (base format + mode bits), FOURCC */

    //BCM_COMP_COLOR_FORMAT_411_Y411      = 112,
    //BCM_COMP_COLOR_FORMAT_411_Y41P      = 117,
    //BCM_COMP_COLOR_FORMAT_411_IY41      = 117 | (1 << 14),
    //BCM_COMP_COLOR_FORMAT_411_Y41T      = 117 | (1 << 15),

    BCM_COMP_COLOR_FORMAT_422_YUY2      = 120,
    //BCM_COMP_COLOR_FORMAT_422_IUYV      = 121 | (1 << 14),
    //BCM_COMP_COLOR_FORMAT_422_Y42T      = 121 | (1 << 15),
    //BCM_COMP_COLOR_FORMAT_444_IYU2      = 131,

    BCM_COMP_COLOR_FORMAT_420_NV12      = 152,
    BCM_COMP_COLOR_FORMAT_420_NV21      = 162,

    //BCM_COMP_COLOR_FORMAT_410_YUV9      = 170,
    //BCM_COMP_COLOR_FORMAT_410_YVU9      = 170,
    //BCM_COMP_COLOR_FORMAT_411_Y41B      = 171,
    BCM_COMP_COLOR_FORMAT_420_YV12      = 172,
    BCM_COMP_COLOR_FORMAT_420_IYUV      = 172,
    BCM_COMP_COLOR_FORMAT_420_I420      = 172,
    BCM_COMP_COLOR_FORMAT_422_YV16      = 173,
    BCM_COMP_COLOR_FORMAT_422_Y42B      = 173,

    //BCM_COMP_COLOR_FORMAT_800_Y800      = 190,

} BCM_COMP_YUV_FORMAT;

/* Configuration bits, used in the config_mask field of BCM_COMP_OBJECT struct */
typedef enum {
    BCM_COMP_SOURCE_RECT_BIT      = (1 <<  0), /* enables source_rect field */
    BCM_COMP_MIRROR_H_BIT         = (1 <<  1), /* enables horizontal flipping */
    BCM_COMP_MIRROR_V_BIT         = (1 <<  2), /* enables vertical flipping */
//    BCM_COMP_SOURCE_TILE_BIT      = (1 <<  3), /* enables source surface tiling */
    BCM_COMP_TARGET_RECT_BIT      = (1 <<  4), /* enables target_rect field */
    BCM_COMP_ROTATE_BIT           = (1 <<  5), /* enables all rotation fields */
    BCM_COMP_SCISSOR_RECT_BIT     = (1 <<  6), /* enables scissor_rect field */
//    BCM_COMP_MASK_SURFACE_BIT     = (1 <<  7), /* enables mask_surface_id field */
//    BCM_COMP_MASK_ALIGN_BIT       = (1 <<  8), /* aligns mask to source_rect */
//    BCM_COMP_MASK_SCALE_BIT       = (1 <<  9), /* enables mask surface scaling */
//    BCM_COMP_MASK_TILE_BIT        = (1 << 10), /* enables mask surface tiling */
    BCM_COMP_GLOBAL_ALPHA_BIT     = (1 << 11), /* enables global_alpha field */
    BCM_COMP_COLOR_KEY_BIT        = (1 << 12), /* enables color_key field */
    BCM_COMP_NO_PIXEL_ALPHA_BIT   = (1 << 13), /* disables pixel alpha */
//    BCM_COMP_NO_MASK_BILINEAR_BIT = (1 << 14), /* disables mask bilinear */
    BCM_COMP_NO_BILINEAR_BIT      = (1 << 15), /* disables source bilinear */
//    BCM_COMP_NO_ANTIALIASING_BIT  = (1 << 16), /* disables antialiasing */
} BCM_COMP_SOURCE_CONFIG;

/* Target configuration bits, defines mirroring + rotation.
 * Mirror is applied prior to rotation if enabled. */
typedef enum {
//    BCM_COMP_TARGET_MIRROR_H          = (1 <<  0), /* horizontal flip */
//    BCM_COMP_TARGET_MIRROR_V          = (1 <<  1), /* vertical flip */
//    BCM_COMP_TARGET_ROTATE_0          = (0 <<  2), /* no rotation */
//    BCM_COMP_TARGET_ROTATE_90         = (1 <<  2), /* 90  degree rotation */
//    BCM_COMP_TARGET_ROTATE_180        = (2 <<  2), /* 180 degree rotation */
//    BCM_COMP_TARGET_ROTATE_270        = (3 <<  2), /* 270 degree rotation */
//    BCM_COMP_TARGET_MASK_ALIGN        = (1 <<  4), /* aligns mask to scissor */
//    BCM_COMP_TARGET_MASK_SCALE        = (1 <<  5), /* enables mask scaling */
//    BCM_COMP_TARGET_MASK_TILE         = (1 <<  6), /* enables mask tiling */
    BCM_COMP_TARGET_COLOR_KEY         = (1 <<  7), /* enables target_color_key */
    BCM_COMP_TARGET_NO_PIXEL_ALPHA    = (1 <<  8), /* disables pixel alpha */
//    BCM_COMP_TARGET_NO_MASK_BILINEAR  = (1 <<  9), /* disables mask bilinear */
//    BCM_COMP_TARGET_VGAA              = (1 << 10), /* enables coverage-based AA */
//    BCM_COMP_TARGET_MSAA_2X           = (1 << 11), /* enables 2X multisampling */
//    BCM_COMP_TARGET_MSAA_4X           = (1 << 12), /* enables 4X multisampling */
} BCM_COMP_TARGET_CONFIG;

/* Alpha blend modes, can be used with both source and target configs.
   By default "SRC over DST" is applied. */
typedef enum {
    BCM_COMP_ALPHA_BLEND_SRC_OVER   = (0  << 20), /* Porter-Duff "SRC over DST" */
    BCM_COMP_ALPHA_BLEND_SRC        = (1  << 20), /* Porter-Duff "SRC" */
    BCM_COMP_ALPHA_BLEND_SRC_IN     = (2  << 20), /* Porter-Duff "SRC in DST" */
    BCM_COMP_ALPHA_BLEND_DST_IN     = (3  << 20), /* Porter-Duff "DST in SRC" */
    BCM_COMP_ALPHA_BLEND_DST_OVER   = (4  << 20), /* Porter-Duff "DST over SRC" */
//    BCM_COMP_ALPHA_BLEND_MULTIPLY   = (5  << 20), /* OpenVG "MULTIPLY" */
//    BCM_COMP_ALPHA_BLEND_SCREEN     = (6  << 20), /* OpenVG "SCREEN" */
//    BCM_COMP_ALPHA_BLEND_DARKEN     = (7  << 20), /* OpenVG "DARKEN" */
//    BCM_COMP_ALPHA_BLEND_LIGHTEN    = (8  << 20), /* OpenVG "LIGHTEN" */
//    BCM_COMP_ALPHA_BLEND_ADDITIVE   = (9  << 20), /* OpenVG "ADDITIVE" */
    BCM_COMP_ALPHA_BLEND_DIRECT     = (10 << 20), /* direct alpha-only blitting */
    BCM_COMP_ALPHA_BLEND_NONE       = (1  << 24), /* disables alpha blending */
} BCM_COMP_ALPHA_BLEND_MODE;

/* Structure for registering a RGB buffer as a composition surface */
typedef struct {
    uint32_t format;   /* RGB color format plus additional mode bits */
    uint32_t width;    /* defines width in pixels */
    uint32_t height;   /* defines height in pixels */
    void  *buffer;   /* pointer to the RGB buffer */
    int32_t  stride;   /* defines stride in bytes, negative stride is allowed */
} BCM_COMP_RGB_SURFACE_DEF;

/* Structure for registering a YUV plane(s) as a composition surface */
typedef struct {
    uint32_t format;   /* YUV color format plus additional mode bits */
    uint32_t width;    /* defines width in pixels */
    uint32_t height;   /* defines height in pixels */
    void  *plane_y;  /* holds the whole buffer if YUV format is not planar */
    int32_t  stride_y; /* stride in bytes if YUV format is not planar */
    void  *plane_u;  /* holds UV or VU plane for planar interleaved */
    int32_t  stride_u; /* stride for UV or VU plane for planar interleaved */
    void  *plane_v;  /* holds V plane, ignored if YUV format is not planar */
    int32_t  stride_v; /* stride for V, ignored if YUV format is not planar */
} BCM_COMP_YUV_SURFACE_DEF;

/* Rectangle definition */
typedef struct {
    int32_t x;        /* upper-left x */
    int32_t y;        /* upper-left y */
    int32_t width;    /* width */
    int32_t height;   /* height */
} BCM_COMP_RECT;

/* Status codes, returned by any composition function */
typedef enum {
    BCM_COMP_STATUS_OK                  = 0,
    BCM_COMP_STATUS_NOT_SUPPORTED       = 1,
    BCM_COMP_STATUS_OUT_OF_MEMORY       = 2,
    BCM_COMP_STATUS_INVALID_PARAM       = 3,
    BCM_COMP_STATUS_SURFACE_IN_USE      = 4,
} BCM_COMP_STATUS;

/* BCM_COMP_OBJECT encapsulates the composition parameters for a source surface.
 * The fg_color defines color in target format for bits equal to 1
 * in the source BCM_COMP_COLOR_FORMAT_1 format. It also defines rendering
 * color for all alpha-only source formats. If the surface_id is 0
 * the fg_color defines a constant fill color used instead of the surface.
 * The bg_color defines color in target format for bits equal to 0
 * in the source BCM_COMP_COLOR_FORMAT_1 format, otherwise both are ignored.
 * The palette_id is used for all palette source formats, otherwise ignored.

 * The source_rect first defines the content of the source surface,
 * it is then horizontally/vertically flipped if BCM_COMP_MIRROR_*_BIT is set,
 * then scaled with bilinear interpolation to exactly fit target_rect
 * or repeated across target_rect if BCM_COMP_SOURCE_TILE_BIT is set,
 * target_rect is then rotated clockwise by an arbitrary angle in degrees
 * around the rot_orig_x/y, defined relative to target_rect's top left point,
 * and then clipped to scissor_rect defined in target coordinate system.

 * Finally alpha blending is applied before pixels are written into the target.
 * Surface's pixel alpha is combined with mask alpha and with global alpha.
 * Mask surface follows all transformations applied to the source surface.
 * Source color key defines transparent color, applied together with alpha. */
typedef struct BCM_COMP_OBJECT_STR {
    uint32_t surface_id;      /* source surface */

    uint32_t fg_color;        /* foreground color */

    uint32_t pad0;
//    uint32_t bg_color;        /* background color */

    uint32_t pad1;
//    uint32_t palette_id;      /* one-dimensional horizontal palette surface */

    uint32_t config_mask;     /* defines which fields below are enabled */

    BCM_COMP_RECT source_rect;  /* region of the source surface,   16.16 fp */
    BCM_COMP_RECT target_rect;  /* position and scaling in target, 16.16 fp */

    int32_t rot_orig_x;       /* rotation origin relative to target_rect's... */
    int32_t rot_orig_y;       /* ...top left point,     both are 16.16 fp */

    int32_t rotation;         /* clock-wise rotation in degrees, 16.16 fp */

    BCM_COMP_RECT scissor_rect; /* defines the clip rectangle in target surface */

    uint32_t pad4;
//    uint32_t mask_surface_id; /* source alpha-mask surface */
    uint32_t global_alpha;    /* 0 = fully transparent, 255 = fully opaque */
    uint32_t color_key;       /* transparent color for the source surface */

    struct BCM_COMP_OBJECT_STR *next; /* pointer to the next object or NULL */
} BCM_COMP_OBJECT;

typedef void *(*BCM_COMP_LOOKUP) (uint32_t id);

/* See section 2.9.1 of the API spec. */
BCM_COMP_API BCM_COMP_STATUS bcmcompDraw( uint32_t target_id,
                         uint32_t target_config,
                         const BCM_COMP_RECT *target_scissor,
                         uint32_t target_color_key,
                         const BCM_COMP_OBJECT *objects_list,
                         uint32_t num_objects,
                         BCM_COMP_LOOKUP lookup);

typedef void (*BCM_COMP_CALLBACK)(void *);

BCM_COMP_API BCM_COMP_STATUS bcmcompDrawAsync(
   /* same as bcmcompDraw */
   uint32_t target_id,
   uint32_t target_config,
   const BCM_COMP_RECT *target_scissor,
   uint32_t target_color_key,
   const BCM_COMP_OBJECT *objects_list,
   uint32_t num_objects,
   BCM_COMP_LOOKUP lookup,
   /* extra */
   BCM_COMP_CALLBACK callback,
   void *callback_arg);

typedef enum {
   BCM_COMP_QUANT_NEAREST,
   BCM_COMP_QUANT_DROP_BITS
} BCM_COMP_QUANT;

/* bcmcompSetQuant sets how quantisation is done:
 * - BCM_COMP_QUANT_NEAREST: pick the nearest value
 * - BCM_COMP_QUANT_DROP_BITS: drop bits off the bottom
 *
 * bcmcompGetQuant returns the current setting */
BCM_COMP_API void bcmcompSetQuant(BCM_COMP_QUANT q);
BCM_COMP_API BCM_COMP_QUANT bcmcompGetQuant(void);

/* bcmcompSetDitherEnable enables or disables dithering. when dithering is
 * enabled, the dither pattern is added just before quantisation, so:
 * quantised_value = quantise_to_n_bits(value + pattern_n[i])
 *
 * bcmcompGetDitherEnable returns whether or not dithering is enabled
 *
 * bcmcompSetDitherPattern sets the dither pattern to be used when quantising to
 * n bits. the pattern should be 16 elements long. if n is invalid (ie we never
 * quantise to n bits), the call has no effect
 *
 * bcmcompGetDitherPattern returns the dither pattern used when quantising to n
 * bits. if n is invalid (ie we never quantise to n bits), 0 is returned and
 * pattern is not modified
 *
 * bcmcompSetDitherPatternRot sets how much the patterns are rotated between
 * rows (if they're oriented horizontally) or columns (if vertically)
 *
 * bcmcompGetDitherPatternRot returns how much the patterns are rotated between
 * rows or columns */
BCM_COMP_API void bcmcompSetDitherEnable(int en);
BCM_COMP_API int bcmcompGetDitherEnable(void);
BCM_COMP_API void bcmcompSetDitherPattern(uint32_t n, const int8_t *pattern);
BCM_COMP_API int bcmcompGetDitherPattern(uint32_t n, int8_t *pattern);
BCM_COMP_API void bcmcompSetDitherPatternRot(uint32_t rot);
BCM_COMP_API uint32_t bcmcompGetDitherPatternRot(void);

BCM_COMP_API BCM_COMP_STATUS bcmcompInit();
BCM_COMP_API void bcmcompTerm();

#endif // BCMCOMP_H
