/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef _GHW_COMPOSER_H_
#define _GHW_COMPOSER_H_

#include "ghw_common.h"
#include "ghw_allocator.h"

namespace ghw {

/**
 * Pixel Format Definitions
 *   GHW_PIXEL_FORMAT_RGBA_8888    : MSB is A and LSB is R out of a 32-bit word - byte0(bit7:0)=R, byte1(bit7:0)=G, byte2(bit7:0)=B, byte3(bit7:0)=A
 *   GHW_PIXEL_FORMAT_BGRA_8888    : MSB is A and LSB is B out of a 32-bit word - byte0(bit7:0)=B, byte1(bit7:0)=G, byte2(bit7:0)=R, byte3(bit7:0)=A
 *   GHW_PIXEL_FORMAT_RGBX_8888    : MSB is dummy and LSB is B out of a 32-bit word - byte0(bit7:0)=B, byte1(bit7:0)=G, byte2(bit7:0)=R, byte3(bit7:0)=A
 *   GHW_PIXEL_FORMAT_BGRX_8888    : MSB is dummy and LSB is B out of a 32-bit word - byte0(bit7:0)=B, byte1(bit7:0)=G, byte2(bit7:0)=R, byte3(bit7:0)=A
 *   GHW_PIXEL_FORMAT_RGB_565      : 16bit word 15:11 - R, 10:5 - G, 4:0 - B
 *   GHW_PIXEL_FORMAT_BGR_565      : 16bit word 15:11 - B, 10:5 - G, 4:0 - R
 *   GHW_PIXEL_FORMAT_RGBA_5551    : 16bit word 15:11 - R, 10:6 - G, 5:1 - B, 0 - A
 *   GHW_PIXEL_FORMAT_RGBA_4444    : 16bit word 15:12 - R, 11:8 - G, 7:4 - B, 3:0 - A
 *   GHW_PIXEL_FORMAT_RGB_888      : byte0(bit7:0)=R, byte1(bit7:0)=G, byte2(bit7:0)=B
 *   GHW_PIXEL_FORMAT_BGR_888      : byte0(bit7:0)=B, byte1(bit7:0)=G, byte2(bit7:0)=R
 *   GHW_PIXEL_FORMAT_YCbCr_420_SP : (NV12) YUV 420 semi-planar format YYYY...,UVUV...
 *   GHW_PIXEL_FORMAT_YCrCb_420_SP : (NV21) YUV 420 semi-planar format YYYY...,VUVU...
 *   GHW_PIXEL_FORMAT_YCbCr_420_P  : (YUV420P) YUV 420 planar format YYYY...,UU....,VV...
 *   GHW_PIXEL_FORMAT_YV12         : (YV12) YUV 420 planar format YYYY...,VV....,UU...
 *   GHW_PIXEL_FORMAT_UYVY_422     : (Y422) YUV 422 interleaved format
 *   GHW_PIXEL_FORMAT_VYUY_422     : (VYUY) YUV 422 interleaved format
 *   GHW_PIXEL_FORMAT_YUYV_422     : (YUY2) YUV 422 interleaved format
 *   GHW_PIXEL_FORMAT_YVYU_422     : (YVYU) YUV 422 interleaved format
 */
enum {
    GHW_PIXEL_FORMAT_RGBA_8888      = 0x01,
    GHW_PIXEL_FORMAT_BGRA_8888      = 0x02,
    GHW_PIXEL_FORMAT_RGBX_8888      = 0x03,
    GHW_PIXEL_FORMAT_BGRX_8888      = 0x04,

    GHW_PIXEL_FORMAT_RGB_565        = 0x05,
    GHW_PIXEL_FORMAT_BGR_565        = 0x06,
    GHW_PIXEL_FORMAT_RGBA_5551      = 0x07,
    GHW_PIXEL_FORMAT_RGBA_4444      = 0x08,

    GHW_PIXEL_FORMAT_RGB_888        = 0x0C,
    GHW_PIXEL_FORMAT_BGR_888        = 0x0D,

    GHW_PIXEL_FORMAT_YCbCr_420_SP   = 0x10,
    GHW_PIXEL_FORMAT_YCrCb_420_SP   = 0x11,
    GHW_PIXEL_FORMAT_YCbCr_420_P    = 0x12,
    GHW_PIXEL_FORMAT_YV12           = 0x13,

    GHW_PIXEL_FORMAT_UYVY_422       = 0x18,
    GHW_PIXEL_FORMAT_VYUY_422       = 0x19,
    GHW_PIXEL_FORMAT_YUYV_422       = 0x1a,
    GHW_PIXEL_FORMAT_YVYU_422       = 0x1b,

    GHW_PIXEL_FORMAT_YUV_444       = 0x1c,

    GHW_PIXEL_FORMAT_Y       		= 0x1d,
    GHW_PIXEL_FORMAT_U		 		= 0x1e,
    GHW_PIXEL_FORMAT_V				= 0x1f,

    GHW_PIXEL_FORMAT_INVALID        = 0x20
};

/**
 * Dither definitions
 *   GHW_DITHER_NONE   : No dithering to be done
 *   GHW_DITHER_565    : Dither for a 565 display whatever be the fb pixel format
 *   GHW_DITHER_666    : Dither for a 666 display whatever be the fb pixel format
 */
enum {
    GHW_DITHER_NONE     = 0x01,
    GHW_DITHER_565      = 0x02,
    GHW_DITHER_666      = 0x03,
};

/**
 * Transformation definitions
 *   GHW_TRANSFORM_NONE     : No rotation
 *   GHW_TRANSFORM_FLIP_H   : flip source image horizontally (around the vertical axis)
 *   GHW_TRANSFORM_FLIP_V   : flip source image vertically (around the horizontal axis)
 *   GHW_TRANSFORM_ROT_90   : rotate source image 90 degrees clockwise
 *   GHW_TRANSFORM_ROT_180  : rotate source image 180 degrees
 *   GHW_TRANSFORM_ROT_270  : rotate source image 270 degrees clockwise
 */
enum {
    GHW_TRANSFORM_NONE      = 0x00,
    GHW_TRANSFORM_FLIP_H    = 0x01,
    GHW_TRANSFORM_FLIP_V    = 0x02,
    GHW_TRANSFORM_ROT_90    = 0x04,
    GHW_TRANSFORM_ROT_180   = 0x03,
    GHW_TRANSFORM_ROT_270   = 0x07,
};

/**
 * Blend definitions
 *   GHW_BLEND_NONE         : No blending to be done
 *   GHW_BLEND_NON_PREMULT  : Alpha not pre-multiplied
 *   GHW_BLEND_SRC_PREMULT  : Assume src is pre-multipled with alpha
 */
enum {
    GHW_BLEND_NONE          = 0x01,
    GHW_BLEND_NON_PREMULT   = 0x02,
    GHW_BLEND_SRC_PREMULT   = 0x03,
};

/**
 * Memory Layout definitions
 *   GHW_MEM_LAYOUT_RASTER  : Image in raster-scan format
 *   GHW_MEM_LAYOUT_TILED   : Image in tiled format
 */
enum {
    GHW_MEM_LAYOUT_RASTER   = 0x01,
    GHW_MEM_LAYOUT_TILED    = 0x02,
};

/*
 * GhwImgBuf: Graphics Image Buffer
 * Handle for holding a graphics image buffer which will have information about the physically contiguous
 *   memory buffer (need not be physically contiguous if IOMMU is present but will be contiguous in IPA
 *   space for that case). This is an interface class with a create() API to create an object of this Class.
 *   Client will have to set the memory handle, image attributes, crop window if needed using member functions.
 * create()       : Static member function which will create a dummy instance of image buffer.
 * setMemHandle() : Set the memory handle which has information about the virtual address, ipa address.
 *   The protoype of this class is defined in the allocator as 'GhwMemHandle' and memory buffer passed to
 *   the composer should be wrapped in this class 'GhwMemHandle'.
 * setGeometry()  : Function to set the buffer Width and height (padding included).
 * setFormat()    : Function to set the image format, memory layout and alpha type if image has alpha value.
 * setCrop()      : Function to set the crop parameters (valid image window with no padding). There are two
 *   variants - one in which you pass all the four co-ordinates or pass only valid width and height assuming
 *   start of buffer as top-left vertex.
 * get() versions of the above methods provide facility to read back the values
 * dump()         : Prints all information about the buffer.
 */
class GhwImgBuf {
public:
    static GhwImgBuf*       create();
    virtual ~GhwImgBuf();

    virtual ghw_error_e     setMemHandle(GhwMemHandle* mem_handle) = 0;
    virtual ghw_error_e     getMemHandle(GhwMemHandle*& mem_handle) = 0;
    virtual ghw_error_e     setGeometry(u32 width, u32 height) = 0;
    virtual ghw_error_e     getGeometry(u32& width, u32& height) = 0;
    virtual ghw_error_e     setFormat(u32 format, u32 layout = GHW_MEM_LAYOUT_RASTER, u32 blend_type = GHW_BLEND_SRC_PREMULT) = 0;
    virtual ghw_error_e     getFormat(u32& format, u32& layout, u32& blend_type) = 0;
    virtual ghw_error_e     setCrop(u32 left, u32 top, u32 right, u32 bottom) = 0;
    virtual ghw_error_e     setCrop(u32 width, u32 height) = 0;
    virtual ghw_error_e     getCrop(u32& left, u32& top, u32& right, u32& bottom) = 0;

    virtual ghw_error_e     dump(u32 level = 0) = 0;
};

/*
 * GhwImgOp: Graphics Image Operation
 * Handle for holding the operations to be done on the graphics image buffer. This is an interface class
 *   with a create() API to create an instance of this Class. Client will have to set the rotation and the
 *   destination buffer crop window based on which scaling parameters will be computed using member functions.
 * create()       : Static member function which will create a dummy instance of image operation.
 * setDstWindow() : Function to select the window in the destination image where the update has to be done.
 * setTransform() : Function to select the rotation if required - GHW_TRANSFORM_*.
 * get() versions of the above methods provide facility to read back the values
 * dump()         : Prints all information about the operations selected.
 */
class GhwImgOp {
public:
    static GhwImgOp*        create();
    virtual ~GhwImgOp();

    virtual ghw_error_e     setDstWindow(u32 left, u32 top, u32 right, u32 bottom) = 0;
    virtual ghw_error_e     getDstWindow(u32& left, u32& top, u32& right, u32& bottom) = 0;
    virtual ghw_error_e     setTransform(u32 transform) = 0;
    virtual ghw_error_e     getTransform(u32& transform) = 0;
    virtual ghw_error_e     dump(u32 level = 0) = 0;
};

/*
 * GhwComposer: Graphics Composer
 * Composer class supports two functionalities - image conversion and frame composition. This is an interface
 *   class with a create() API to create an instance of the Composer class. The graphics hardware might
 *   need temporary buffers for doing the operation and would internally allocate physically contiguous
 *   memory using GhwMemAllocator object created inside.
 *
 * create()         : A static function which allocates an object of the composer. Optional parameters can be
 *   used to hint the width, height and number of layers that could be in use.
 *   If temporary buffers need to be pre-allocated, provide the worst-case values as parameters.
 *   If no temporary buffer should be pre-allocated, set parameters to 0. In this case, memory might be
 *     allocated from driver on every composition, conversion request.
 *   Optimal solution would be to provide values corresponding to the common textures. This would avoid
 *     allocations from driver for sizes/layers less than the common values.
 * setName()        : Identifier (printed in dump).
 * barrier()        : Wait for all jobs submitted to graphics hardware to complete. Will free up all the
 *     temporary buffers internally allocated. Other APIs would internally use this API to acheive
 *     synchronisation.
 * Frame Composition APIs:
 *   compSetFb()      : Set the frame buffer (destination buffer) and select the dithering flag
 *   compDrawRect()   : Do composition of source buffer onto framebuffer and select the operation.
 *   compCommit()     : Commit all the composition requests submitted after previous compSetFb() or
 *     previous compCommit().
 * Image Conversion APIs:
 *   imgProcess()     : Process the source image as per image operation selected and write to destination
 *     image.
 * dump()           : Prints the current state.
 *
 * Note: Internally, cache flushes would be done to achieve coherency (if needed).
 */
class GhwComposer {
public:

    static GhwComposer*     create(u32 width = 720, u32 height = 480, u32 num_layers = 5);
    virtual ~GhwComposer();

    virtual ghw_error_e     barrier(void) = 0;

    virtual ghw_error_e     compSetFb(GhwImgBuf* fb_img, u32 dither_flag) = 0;
    virtual ghw_error_e     compDrawRect(GhwImgBuf* src_img, GhwImgOp* op) = 0;
    virtual ghw_error_e     compCommit(u32 sync_flag) = 0;

    virtual ghw_error_e     imgProcess(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op, u32 sync_flag) = 0;

    virtual ghw_error_e     setName(const char *name) = 0;
    virtual ghw_error_e     dump(u32 level = 0) = 0;
};

};
#endif /* _GHW_COMPOSER_H_ */

