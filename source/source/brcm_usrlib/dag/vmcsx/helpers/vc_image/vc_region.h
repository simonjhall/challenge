/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/**
 * \file
 *
 * This file is not part of the public interface.
 * Objects declared here should not be used externally.
 */

#ifndef VC_REGION_H
#define VC_REGION_H

typedef struct VC_REGION VC_REGION_T;
typedef struct VC_REGION_RECT VC_REGION_RECT_T;
typedef struct VC_REGION_BAND VC_REGION_BAND_T;
typedef struct VC_REGION_BAND_AREA VC_REGION_BAND_AREA_T;

/* A region of a bitmap. */

struct VC_REGION {
   int size_x;
   int size_y;
   int on_transparency;
   VC_REGION_RECT_T *rects;
   VC_REGION_BAND_T *bands;
};

/* A rectangle of visibility within a region. */

struct VC_REGION_RECT {
   int start_x;
   int start_y;
   int end_x;
   int end_y;
   int transparency;
   int tag;                // available for applications to use to "tag" rectanges in some way
   VC_REGION_RECT_T *next;
};

/* A vertical band of a region. */

struct VC_REGION_BAND {
   int start_y;
   int end_y;
   VC_REGION_BAND_AREA_T *areas;
   VC_REGION_BAND_T *next;
};

/* An actual area visibility within a band. */

struct VC_REGION_BAND_AREA {
   int start_x;
   int end_x;
   VC_REGION_BAND_AREA_T *next;
};

/* Make a region where the "whole region" goes from (0,0) to (size_x, size_y). */

VC_REGION_T *vc_region_make(int size_x, int size_y, int on_transparency);

/* Free a region and all the associated data structures. */

void vc_region_delete(VC_REGION_T *region);

/* Register a rectanglar area of the region being visible or occluding. */

VC_REGION_RECT_T *vc_region_add_rect(VC_REGION_T *region, int start_x, int start_y, int width, int height, int transparency);

/* De-register a previously registered rectangular area. */

void vc_region_remove_rect(VC_REGION_T *region, VC_REGION_RECT_T *rect);

/* De-register "tagged" rectangular areas. */

void vc_region_remove_tagged(VC_REGION_T *region, int tag);

/* Recompute the cached band/area information. Only needed by functions that
   explicitly modifiy the list of rectangles. */

void vc_region_recompute_bands(VC_REGION_T *region);

/* Do a blt from src to dest, but overwriting pixels in dest only where the region
   marks that area as "visible". */

void vc_image_region_blt(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                         VC_IMAGE_T *src, int src_x_offset, int src_y_offset, VC_REGION_T *region);

/* Generic region handling function */

// This applies the specified function to all unclipped areas within the
// region. It supports striped operations by limiting the area of the rectangles
// that are returned

typedef void (*VC_REGION_OP_T)(int x_offset, int y_offset, int width, int height,
                               int src_x_offset, int src_y_offset,
                               void *context);

void vc_image_region_apply(int x_offset, int y_offset, int width, int height,
                           int src_x_offset, int src_y_offset,
                           VC_REGION_T *region, int max_area,
                           VC_REGION_OP_T operation, void *context);

// This applies the specified function to the given rectangle as a series
// of bands no bigger than max_area

void vc_image_rect_apply(int x_offset, int y_offset, int width, int height,
                         int src_x_offset, int src_y_offset,
                         int max_area,
                         VC_REGION_OP_T operation, void *context);

#endif
