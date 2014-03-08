/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef _VC_POOL_IMAGE_H_
#define _VC_POOL_IMAGE_H_

#include "helpers/vc_pool/vc_pool.h"
#include "helpers/vc_image/vc_image.h"



typedef struct vc_pool_image_geometry_s
{
   uint32_t  width;
   uint32_t  height;
   uint32_t  num;
} VC_POOL_IMAGE_GEOMETRY_T;



VC_POOL_T *vc_pool_image_create( VC_IMAGE_TYPE_T type,
                                 uint32_t max_width,
                                 uint32_t max_height,
                                 uint32_t num,
                                 uint32_t metadata_size,
                                 VC_POOL_FLAGS_T flags,
                                 const char *name,
                                 VC_IMAGE_PROPLIST_T *props, uint32_t n_props);

/* ----------------------------------------------------------------------
 * Create a pool large enough to hold any one of the set of image geometries
 * supplied.  Images will have the specified properties.
 * Really just a helper function for vc_pool_create
 *
 * This function allocates a LARGE NON-RELOCATABLE memory block.
 *
 * If you have to call this function you have already failed.
 *
 * -------------------------------------------------------------------- */
extern VC_POOL_T *
vc_pool_image_create_codec_pool_of_last_resort( VC_IMAGE_TYPE_T                 type,
                                                const VC_POOL_IMAGE_GEOMETRY_T  * const image_geom,
                                                uint32_t                        n_image_geom,
                                                uint32_t                        metadata_size,
                                                VC_POOL_FLAGS_T                 flags,
                                                const char                      * const name,
                                                VC_IMAGE_PROPLIST_T             * const proplist,
                                                uint32_t                        n_props );

VC_IMAGE_T *vc_pool_image_alloc( VC_POOL_T *pool,
                                 VC_IMAGE_TYPE_T type,
                                 uint32_t width,
                                 uint32_t height,
                                 uint32_t metadata_size,
                                 uint32_t timeout,
                                 VC_IMAGE_PROPLIST_T *props, uint32_t n_props);

int vc_pool_image_validate( VC_POOL_T *pool,
                            VC_IMAGE_TYPE_T type,
                            uint32_t width,
                            uint32_t height,
                            uint32_t metadata_size,
                            VC_IMAGE_PROPLIST_T *proplist,
                            uint32_t n_props);

void vc_pool_image_acquire( VC_IMAGE_T *img );
void vc_pool_image_release( VC_IMAGE_T *img );

#endif // _VC_POOL_IMAGE_H_
