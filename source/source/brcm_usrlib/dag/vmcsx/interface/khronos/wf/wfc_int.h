/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited.
All rights reserved.

Project  :  Khronos
Module   :  OpenWF-C

FILE DESCRIPTION
OpenWF Composition interface definitions (i.e. those used by both client
   and server).
=============================================================================*/

#ifndef WFC_INT_H
#define WFC_INT_H

#include "interface/khronos/include/WF/wfc.h"

//==============================================================================

#define NO_ATTRIBUTES   NULL

// Values for rectangle types
#define WFC_RECT_X      0
#define WFC_RECT_Y      1
#define WFC_RECT_WIDTH  2
#define WFC_RECT_HEIGHT 3

#define WFC_RECT_SIZE   4

// Values for context background colour
#define WFC_BG_CLR_RED     0
#define WFC_BG_CLR_GREEN   1
#define WFC_BG_CLR_BLUE    2
#define WFC_BG_CLR_ALPHA   3

#define WFC_BG_CLR_SIZE    4

//==============================================================================

// Attributes associated with each element
typedef struct
{
   WFCint         dest_rect[WFC_RECT_SIZE];
   WFCSource      source_handle;
   WFCfloat       src_rect[WFC_RECT_SIZE];
   WFCboolean     flip;
   WFCRotation    rotation;
   WFCScaleFilter scale_filter;
   WFCbitfield    transparency_types;
   WFCfloat       global_alpha;
   WFCMask        mask_handle;
} WFC_ELEMENT_ATTRIB_T;

// Default values for elements
#define WFC_ELEMENT_ATTRIB_DEFAULT \
{ \
   {0, 0, 0, 0}, \
   WFC_INVALID_HANDLE, \
   {0.0, 0.0, 0.0, 0.0}, \
   WFC_FALSE, \
   WFC_ROTATION_0, \
   WFC_SCALE_FILTER_NONE, \
   0, \
   1.0, \
   WFC_INVALID_HANDLE \
}

// Attributes associated with each context
typedef struct
{
   WFCRotation    rotation;
   WFCfloat       background_clr[WFC_BG_CLR_SIZE];
} WFC_CONTEXT_ATTRIB_T;

// Default values for contexts
#define WFC_CONTEXT_ATTRIB_DEFAULT \
{ \
   WFC_ROTATION_0, \
   {0.0, 0.0, 0.0, 1.0} \
}

#endif /* WFC_INT_H */

//==============================================================================
