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

/* Project includes */
#include "vcinclude/vcore.h"
#include "vc_image.h"
#include "interface/vcos/vcos_assert.h"

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

/* Check extern defs match above and loads #defines and typedefs */
#include "vc_region.h"

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

extern VC_REGION_T *vc_region_make (int size_x, int size_y, int on_transparency);
extern void vc_region_delete (VC_REGION_T *region);
extern VC_REGION_RECT_T *vc_region_add_rect (VC_REGION_T *region, int start_x, int start_y,
         int width, int height, int transparency);
extern void vc_region_remove_rect (VC_REGION_T *region, VC_REGION_RECT_T *rect);
extern void vc_region_remove_tagged (VC_REGION_T *region, int tag);
extern void vc_region_recompute_bands (VC_REGION_T *region);
extern void vc_image_region_blt (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                                    VC_IMAGE_T *src, int src_x_offset, int src_y_offset, VC_REGION_T *region);
extern void vc_image_rect_apply (int x_offset, int y_offset, int width, int height,
                                    int src_x_offset, int src_y_offset, int max_area, VC_REGION_OP_T operation, void *context);
extern void vc_image_region_apply (int x_offset, int y_offset, int width, int height,
                                      int src_x_offset, int src_y_offset, VC_REGION_T *region, int max_area,
                                      VC_REGION_OP_T operation, void *context);
extern void delete_bands (VC_REGION_T *region);
extern int find_next_x (int start_y, int end_y, int x, VC_REGION_T *region);
extern int compute_transparency (int start_y, int end_y, int x, VC_REGION_T *region);

/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/

// Allocate/deallocate routines for region structures.
// This allows the option of using a static pool of structures
// rather than using dynamic allocation.

// #define USE_MALLOC
#ifndef USE_MALLOC
#define MAX_REGIONS            5
#define MAX_REGION_RECTS      15
#define MAX_REGION_BANDS      30
#define MAX_REGION_BAND_AREAS 50
#else
// Dynamic memory version...
#define alloc_region()           (VC_REGION_T *)malloc(sizeof(VC_REGION_T))
#define free_region(x)           free(x)
#define alloc_region_rect()      (VC_REGION_RECT_T *)malloc(sizeof(VC_REGION_RECT_T))
#define free_region_rect(x)      free(x)
#define alloc_region_band()      (VC_REGION_BAND_T *)malloc(sizeof(VC_REGION_BAND_T))
#define free_region_band(x)      free(x)
#define alloc_region_band_area() (VC_REGION_BAND_AREA_T *)malloc(sizeof(VC_REGION_BAND_AREA_T))
#define free_region_band_area(x) free(x)
#endif

typedef struct _BLT_CONTEXT {
   VC_IMAGE_T *src;
   VC_IMAGE_T *dst;
} BLT_CONTEXT;

/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/

#ifndef USE_MALLOC
static void initialise_static_data ();
static VC_REGION_T *alloc_region ();
static void free_region (VC_REGION_T *ptr);
static VC_REGION_RECT_T *alloc_region_rect ();
static void free_region_rect (VC_REGION_RECT_T *ptr);
static VC_REGION_BAND_T *alloc_region_band ();
static void free_region_band (VC_REGION_BAND_T  *ptr);
static VC_REGION_BAND_AREA_T *alloc_region_band_area ();
static void free_region_band_area (VC_REGION_BAND_AREA_T  *ptr);
#endif
static void delete_bands (VC_REGION_T *region);
static int find_next_x (int start_y, int end_y, int x, VC_REGION_T *region);
static int compute_transparency (int start_y, int end_y, int x, VC_REGION_T *region);
static void blt_op(int x_offset, int y_offset, int width, int height,
                   int src_x_offset, int src_y_offset, void *op_context);

/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/

#ifndef USE_MALLOC
static int StaticDataInitialised = 0;
static VC_REGION_T  RegionData[MAX_REGIONS] = {{0}};
static VC_REGION_T *Regions = 0;
static VC_REGION_RECT_T  RegionRectData[MAX_REGION_RECTS] = {{0}};
static VC_REGION_RECT_T *RegionRects = 0;
static VC_REGION_BAND_T  RegionBandData[MAX_REGION_BANDS] = {{0}};
static VC_REGION_BAND_T *RegionBands = 0;
static VC_REGION_BAND_AREA_T  RegionBandAreaData[MAX_REGION_BAND_AREAS] = {{0}};
static VC_REGION_BAND_AREA_T *RegionBandAreas = 0;
#endif

/******************************************************************************
Global function definitions.
******************************************************************************/

#ifndef USE_MALLOC

static void initialise_static_data()
{
   int i;

// Create a chain for each structure type
// Regions don't have a "next" pointer, so we have to use
// the "rects" pointer and do a bit of casting....
   for (i = 0; i < MAX_REGIONS - 1; i++) {
      RegionData[i].rects = (VC_REGION_RECT_T *)(void *)&RegionData[i+1];
   }
   Regions = &RegionData[0];

   for (i = 0; i < MAX_REGION_RECTS - 1; i++) {
      RegionRectData[i].next = &RegionRectData[i+1];
   }
   RegionRects = &RegionRectData[0];

   for (i = 0; i < MAX_REGION_BANDS - 1; i++) {
      RegionBandData[i].next = &RegionBandData[i+1];
   }
   RegionBands = &RegionBandData[0];

   for (i = 0; i < MAX_REGION_BAND_AREAS - 1; i++) {
      RegionBandAreaData[i].next = &RegionBandAreaData[i+1];
   }
   RegionBandAreas = &RegionBandAreaData[0];

   StaticDataInitialised = 1;
}

static VC_REGION_T *alloc_region()
{
   VC_REGION_T *ptr = 0;

   if (!StaticDataInitialised) {
      initialise_static_data();
   }

   vcos_assert(Regions);
   ptr = Regions;
   if (ptr) {
      Regions = (VC_REGION_T *)(void *)ptr->rects;
   }

   return ptr;
}

static void free_region(VC_REGION_T *ptr)
{
   ptr->rects = (VC_REGION_RECT_T *)(void *)Regions;
   Regions = ptr;
}

static VC_REGION_RECT_T *alloc_region_rect()
{
   VC_REGION_RECT_T *ptr = RegionRects;
   vcos_assert(RegionRects);
   if (ptr) {
      RegionRects = ptr->next;
   }

   return ptr;
}

static void free_region_rect(VC_REGION_RECT_T *ptr)
{
   ptr->next = RegionRects;
   RegionRects = ptr;
}

static VC_REGION_BAND_T *alloc_region_band()
{
   VC_REGION_BAND_T *ptr = RegionBands;
   vcos_assert(RegionBands);
   if (ptr) {
      RegionBands = ptr->next;
   }

   return ptr;
}

static void free_region_band(VC_REGION_BAND_T  *ptr)
{
   ptr->next = RegionBands;
   RegionBands = ptr;
}

static VC_REGION_BAND_AREA_T *alloc_region_band_area()
{
   VC_REGION_BAND_AREA_T *ptr = RegionBandAreas;
   vcos_assert(RegionBandAreas);
   if (ptr) {
      RegionBandAreas = ptr->next;
   }

   return ptr;
}

static void free_region_band_area(VC_REGION_BAND_AREA_T  *ptr)
{
   ptr->next = RegionBandAreas;
   RegionBandAreas = ptr;
}

#endif /* USE_MALLOC */

/******************************************************************************
NAME
   vc_region_make

SYNOPSIS
   VC_REGION_T *vc_region_make(int size_x, int size_y, int on_transparency)

FUNCTION
   Make a region of the given maximum size.

RETURNS
   VC_REGION_T *
******************************************************************************/

VC_REGION_T *vc_region_make (int size_x, int size_y, int on_transparency) {
   VC_REGION_T *region = alloc_region();
   if (region) {
      memset(region, 0, sizeof(VC_REGION_T));
      region->size_x = size_x;
      region->size_y = size_y;
      region->on_transparency = on_transparency;

      // Recompute the bands that make up the (whole) region.
      vc_region_recompute_bands(region);
   }
   return region;
}

/******************************************************************************
NAME
   vc_region_delete

SYNOPSIS
   VC_REGION_T *vc_region_delete(VC_REGION_T *region)

FUNCTION
   Free a region and all its sub-structures.

RETURNS
   -
******************************************************************************/

void vc_region_delete (VC_REGION_T *region) {
   VC_REGION_RECT_T *rect;

   // Free all the substructure (cutouts and bands) first.
   while (region->rects) {
      rect = region->rects;
      region->rects = region->rects->next;
      free_region_rect(rect);
   }

   delete_bands(region);

   free_region(region);
}

/******************************************************************************
NAME
   vc_region_add_rect

SYNOPSIS
   VC_REGION_T *vc_region_add_rect(VC_REGION_T *region, int start_x, int start_y, int width, int height, int transparency)

FUNCTION
   Register a new rectangle with the region.

RETURNS
   -
******************************************************************************/

VC_REGION_RECT_T *vc_region_add_rect (VC_REGION_T *region, int start_x, int start_y, int width, int height, int transparency) {
   VC_REGION_RECT_T *rect = alloc_region_rect();

   if (rect) {
      // Set the fields of the new rectangle and add it to the head of the list.
      rect->start_x = start_x;
      rect->start_y = start_y;
      rect->end_x = start_x + width;
      rect->end_y = start_y + height;
      rect->transparency = transparency;
      rect->next = region->rects;
      region->rects = rect;

      // Recompute the bands that make up the region.
      vc_region_recompute_bands(region);
   }

   return rect;
}

/******************************************************************************
NAME
   vc_region_remove_rect

SYNOPSIS
   void vc_region_remove_rect(VC_REGION_T *region, VC_REGION_RECT_T *rect)

FUNCTION
   De-register a rectangle from the region.

RETURNS
   -
******************************************************************************/

void vc_region_remove_rect (VC_REGION_T *region, VC_REGION_RECT_T *rect) {
   // Unhook rect from the list and then free it.
   if (region->rects == rect) {
      region->rects = rect->next;
   }
   else {
      VC_REGION_RECT_T *prev = region->rects;
      for (; prev; prev = prev->next) {
         if (prev->next == rect) {
            prev->next = rect->next;
            break;
         }
      }
   }
   free_region_rect(rect);

   // Recompute the bands that make up the region.
   vc_region_recompute_bands(region);
}

/******************************************************************************
NAME
   vc_region_remove_tagged

SYNOPSIS
   void vc_region_remove_tagged(VC_REGION_T *region, int tag)

FUNCTION
   De-register "tagged" rectangles from the region.

RETURNS
   -
******************************************************************************/

void vc_region_remove_tagged (VC_REGION_T *region, int tag) {
   // Unhook tagged rects from the list, freeing them as we go.
   VC_REGION_RECT_T *prev = NULL, *rect, *next;
   int ndeleted = 0;
   for (rect = region->rects; rect; rect = next) {
      next = rect->next;
      if (rect->tag == tag) {
         if (prev)
            prev->next = next;
         else
            region->rects = next;
         free_region_rect(rect);
         ndeleted++;
      }
      else
         prev = rect;
   }

   // Recompute the bands that make up the region.
   if (ndeleted)
      vc_region_recompute_bands(region);
}

/******************************************************************************
NAME
   vc_region_recompute_bands

SYNOPSIS
   void vc_region_recompute_bands(VC_REGION_T *region)

FUNCTION
   Force all the cached band/area information within a region to be recalculated.
   Should only be needed if the caller has hacked the rectangles by hand.

RETURNS
   void
******************************************************************************/

void vc_region_recompute_bands (VC_REGION_T *region) {
   int start_y, end_y;
   VC_REGION_BAND_T *last_band = NULL;

   // We recompute the entire region from scratch, hence:
   delete_bands(region);

   for (start_y = 0; start_y < region->size_y; start_y = end_y) {
      VC_REGION_RECT_T *rect;
      VC_REGION_BAND_T *band;
      VC_REGION_BAND_AREA_T *last_area = NULL;
      int x = -1, unoccluded_x = 0;
      int last_occluded = region->on_transparency > 0;

      // Find the next y value where something happens, which may be the bottom of the image.
      end_y = region->size_y;
      for (rect = region->rects; rect; rect = rect->next) {
         if (rect->start_y > start_y && rect->start_y < end_y)
            end_y = rect->start_y;
         if (rect->end_y > start_y && rect->end_y < end_y)
            end_y = rect->end_y;
      }

      // Make a new band for start_y to end_y.
      band = alloc_region_band();
      band->start_y = start_y;
      band->end_y = end_y;
      band->areas = NULL;
      band->next = NULL;
      if (region->bands == NULL) region->bands = band;
      else last_band->next = band;
      last_band = band;

      // Now fill in the band with areas (i.e. rectangles within this band of image).
      do {
         int occluded;
         x = find_next_x(start_y, end_y, x, region);
         occluded = compute_transparency(start_y, end_y, x, region) < region->on_transparency;
         if (occluded != last_occluded) {
            // Something has changed.
            if (!occluded)
               unoccluded_x = x;
            else if (x != 0) {
               // We have become occluded. Make an area.
               VC_REGION_BAND_AREA_T *area = alloc_region_band_area();
               area->start_x = unoccluded_x;
               area->end_x = x;
               area->next = NULL;
               if (band->areas == NULL) band->areas = area;
               else last_area->next = area;
               last_area = area;
            }
         }

         last_occluded = occluded;
      } while (x < region->size_x);
   }
}

/******************************************************************************
NAME
   vc_image_region_blt

SYNOPSIS
   int vc_image_region_blt(VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                           VC_IMAGE_T *src, int src_x_offset, int src_y_offset, VC_REGION_T *region)

FUNCTION
   Blt from src bitmap to destination, but only into the areas of the destination bitmap
   defined by the given region. This function can act as a template for all "region-aware"
   operations.

RETURNS
   void
******************************************************************************/

static void blt_op(int x_offset, int y_offset, int width, int height,
                   int src_x_offset, int src_y_offset,
                   void *op_context)
{
   BLT_CONTEXT *context = (BLT_CONTEXT *)op_context;

   vc_image_blt(context->dst, x_offset, y_offset, width, height,
                context->src, src_x_offset, src_y_offset);
}

void vc_image_region_blt (VC_IMAGE_T *dest, int x_offset, int y_offset, int width, int height,
                          VC_IMAGE_T *src, int src_x_offset, int src_y_offset, VC_REGION_T *region)
{
   BLT_CONTEXT blt_context;

   blt_context.dst = dest;
   blt_context.src = src;

   vc_image_region_apply(x_offset, y_offset, width, height,
                         src_x_offset, src_y_offset,
                         region, 0x7FFFFFFF,
                         blt_op, &blt_context);
}

void vc_image_rect_apply (int x_offset, int y_offset, int width, int height,
                          int src_x_offset, int src_y_offset,
                          int max_area,
                          VC_REGION_OP_T operation, void *context)
{
// Divide the rectangle into stripes and apply the operation
// Work in "friendly" units of 16 pixels
   int stripe_width = (width + 15) & ~15;
   int stripe_height = 16; //(max_area / stripe_width) & ~15;
   int stripe_y;

   vcos_assert(stripe_height);

   for (stripe_y = 0; stripe_y < height; stripe_y += stripe_height) {
      int h = height - stripe_y;
      if (h > stripe_height) {
         h = stripe_height;
      }
      operation(x_offset, y_offset + stripe_y, width, h,
                src_x_offset, src_y_offset + stripe_y,
                context);
   }
}

void vc_image_region_apply (int x_offset, int y_offset, int width, int height,
                            int src_x_offset, int src_y_offset,
                            VC_REGION_T *region, int max_area,
                            VC_REGION_OP_T operation, void *context) {
   VC_REGION_BAND_T *band;
   VC_REGION_BAND_AREA_T *area;
   int end_y_offset = y_offset + height;
   int end_x_offset = x_offset + width;

   for (band = region->bands; band; band = band->next) {
      if (y_offset >= band->end_y || band->start_y >= end_y_offset) {
         // No overlap between blt and band.
      }
      else {
         int start_y = y_offset < band->start_y ? band->start_y : y_offset;
         int end_y = end_y_offset > band->end_y ? band->end_y : end_y_offset;
         int band_height = end_y - start_y;
         int src_start_y = src_y_offset + (start_y - y_offset);

         // Now look for the horizontal areas to fill.
         for (area = band->areas; area; area = area->next) {
            if (x_offset >= area->end_x || area->start_x >= end_x_offset) {
               // No overlap.
            }
            else {
               int start_x = x_offset < area->start_x ? area->start_x : x_offset;
               int end_x = end_x_offset > area->end_x ? area->end_x : end_x_offset;

               vc_image_rect_apply(start_x, start_y, end_x-start_x, band_height,
                                   src_x_offset+(start_x-x_offset), src_start_y,
                                   max_area,
                                   operation, context);
            }
         }
      }
   }
}

/******************************************************************************
Static function definitions.
******************************************************************************/

void delete_bands (VC_REGION_T *region) {
   VC_REGION_BAND_T *band;
   VC_REGION_BAND_AREA_T *area;

   // Delete all bands.
   while (region->bands) {
      band = region->bands;
      region->bands = region->bands->next;
      // Delete all areas within each band.
      while (band->areas) {
         area = band->areas;
         band->areas = band->areas->next;
         free_region_band_area(area);
      }
      free_region_band(band);
   }
}

int find_next_x (int start_y, int end_y, int x, VC_REGION_T *region) {
   VC_REGION_RECT_T *rect;
   int next_x = region->size_x;
   for (rect = region->rects; rect; rect = rect->next) {
      if (start_y >= rect->end_y || rect->start_y >= end_y) {
         // Rectangle and band do not overlap.
      }
      else {
         if (rect->start_x > x && rect->start_x < next_x)
            next_x = rect->start_x;
         else if (rect->end_x > x && rect->end_x < next_x)
            next_x = rect->end_x;
      }
   }
   return next_x;
}

int compute_transparency (int start_y, int end_y, int x, VC_REGION_T *region) {
   VC_REGION_RECT_T *rect;
   int transparency = 0;
   if (x == region->size_x)
      transparency = -99999; // definitely not drawable
   else {
      for (rect = region->rects; rect; rect = rect->next) {
         if (start_y >= rect->end_y || rect->start_y >= end_y) {
            // Rectangle and band do not overlap.
         }
         else {
            if (x >= rect->start_x && x < rect->end_x)
               transparency += rect->transparency;
         }
      }
   }
   return transparency;
}
