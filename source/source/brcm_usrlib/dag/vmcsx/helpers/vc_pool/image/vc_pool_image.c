/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "vcinclude/common.h"
#include "vcfw/rtos/rtos.h"
#include "interface/vcos/vcos.h"
#include "vcfw/rtos/common/rtos_common_mem.h"
#include "helpers/vc_pool/vc_pool.h"
#include "helpers/vc_image/vc_image.h"
#include "helpers/vc_image/vc_image_metadata.h"
#include "vc_pool_image.h"


/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/



/******************************************************************************
Private functions in this file.
Define as static.
******************************************************************************/

#if defined(VC_POOL_IMAGE_SCRUB)
static void clear_image(VC_IMAGE_T *img);
#endif

/******************************************************************************
Static data defined in this file. Static data may also be defined within
functions or at the head of a group of functions that share data. Also
for multi-file modules, public data which is shared within the module.
******************************************************************************/



/******************************************************************************
 Global Functions
 *****************************************************************************/

/* ----------------------------------------------------------------------
 * create a pool large enough to hold 'num' images with the specified
 * properties.  really just a helper function for vc_pool_create
 * -------------------------------------------------------------------- */
VC_POOL_T *
vc_pool_image_create( VC_IMAGE_TYPE_T type,
                      uint32_t max_width,
                      uint32_t max_height,
                      uint32_t num,
                      uint32_t metadata_size,
                      VC_POOL_FLAGS_T flags,
                      const char *name,
                      VC_IMAGE_PROPLIST_T *proplist,
                      uint32_t n_props)
{
   VC_IMAGE_T temp;
   size_t required;

   vcos_assert(num < 1024); // metadata/num are swapped?
   vcos_assert((metadata_size & 3) == 0); // metadata/num swapped?
   vcos_assert(num != 0);

   vcos_assert(proplist || !n_props);

   /* Find the maximum required image size */
   vc_image_configure_proplist(&temp, type, max_width, max_height, proplist, n_props);

   required = vc_image_required_size( &temp );
   if (flags & VC_POOL_FLAGS_SUBDIVISIBLE)
      required = num * ALIGN_UP( required + ALIGN_UP( metadata_size, 32 ), 4096 );
   else
      required += ALIGN_UP( metadata_size, 32 );

#if __VCCOREVER__ >= 0x04000000
   if (!(flags & VC_POOL_FLAGS_COHERENT))
      flags |= VC_POOL_FLAGS_DIRECT;
#endif

   return vc_pool_create( required, num, 4096, flags, name, sizeof(VC_IMAGE_T) );
}



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
VC_POOL_T *
vc_pool_image_create_codec_pool_of_last_resort( VC_IMAGE_TYPE_T                 type,
                                                const VC_POOL_IMAGE_GEOMETRY_T  * const image_geom,
                                                uint32_t                        n_image_geom,
                                                uint32_t                        metadata_size,
                                                VC_POOL_FLAGS_T                 flags,
                                                const char                      * const name,
                                                VC_IMAGE_PROPLIST_T             * const proplist,
                                                uint32_t                        n_props )
{
   size_t    max_required = 0;
   uint32_t  max_num      = 0;

   vcos_assert( 0 < n_image_geom );
   vcos_assert((metadata_size & 3) == 0); // metadata/n_image_geom swapped?
   vcos_assert(proplist || !n_props);

   for( ; 0 < n_image_geom--; )
   {
      VC_IMAGE_T  temp;
      size_t      required;

      /* Find the maximum required image size */
      vc_image_configure_proplist( &temp, type, image_geom[ n_image_geom ].width, image_geom[ n_image_geom ].height, proplist, n_props );

      required = vc_image_required_size( &temp );
      required = image_geom[ n_image_geom ].num * ALIGN_UP( required + ALIGN_UP( metadata_size, 32 ), 4096 );

      max_required = _max( max_required, required                       );
      max_num      = _max( max_num,      image_geom[ n_image_geom ].num );
   } /* for */

   vcos_assert( 0 < max_required );
   vcos_assert( 0 < max_num      );

   flags |= ( VC_POOL_FLAGS_SUBDIVISIBLE | VC_POOL_FLAGS_HINT_PERMALOCK );

   return( vc_pool_create( max_required, max_num, 4096, flags, name, sizeof( VC_IMAGE_T ) ) );

} /* vc_pool_image_create_codec_pool_of_last_resort */



/* ----------------------------------------------------------------------
 * allocate an object from the pool.  if there are free objects in
 * the pool, the allocation will happen immediately.  otherwise, we
 * will sleep for up to 'timeout' microseconds waiting for an object
 * to be released.
 *
 * timeout of 0xffffffff means 'wait forever';  0 means 'do not wait'.
 *
 * returns (opaque) pointer to the newly allocated object, or NULL on
 * failure (ie. even after 'timeout' there were still no free objects)
 *
 * a newly allocated object will have its reference count initialised
 * to 1.
 * -------------------------------------------------------------------- */
VC_IMAGE_T *
vc_pool_image_alloc( VC_POOL_T *pool,
                     VC_IMAGE_TYPE_T type,
                     uint32_t width,
                     uint32_t height,
                     uint32_t metadata_size,
                     uint32_t timeout,
                     VC_IMAGE_PROPLIST_T *proplist,
                     uint32_t n_props)
{
   // allocate corresponding vc_image struct
   VC_IMAGE_T tmp;

   // no need to memset 0 - taken care of in vc_image_configure
   //memset( img, 0, sizeof(VC_IMAGE_T) );

   // configure image
   vc_image_configure_proplist( &tmp, type, width, height, proplist, n_props);

   const int required_size = ALIGN_UP( metadata_size, 32 ) + vc_image_required_size( &tmp );

   // try to acquire object from pool
   VC_POOL_OBJECT_T *object = vc_pool_alloc( pool, required_size, timeout );
   if ( !object ) {
      return NULL; // failed to allocate object
   }

   VC_IMAGE_T *img = vc_pool_overhead( object );
   vcos_assert(img); // allocated when pool was allocated, so cannot fail!
   memcpy(img, &tmp, sizeof(VC_IMAGE_T));

   // almost done!
   img->pool_object = object;
   img->mem_handle = vc_pool_mem_handle( object );
   img->metadata_size = metadata_size;

   // metadata fixups!
   if ( metadata_size ) {
      VC_IMAGE_T lock_img;
      vc_image_lock( &lock_img, img );
      vcos_assert( lock_img.metadata ); // make sure we have got a valid pointer.
      vc_metadata_initialise( lock_img.metadata, metadata_size );
      vc_image_unlock( &lock_img );
   }

#if defined(VC_POOL_IMAGE_SCRUB)
   clear_image(img);
#endif

   return img;
}

/* ----------------------------------------------------------------------
 * Assess whether the allocation of the requested image might every
 * be possible.  Returns FALSE (0) if the requested image is too big
 * EVER to be allocated, and TRUE (non-zero) if it is of a size which might
 * be allocated at some point.  No guarantee that the memory is available
 * now though.
 *
 * -------------------------------------------------------------------- */
int vc_pool_image_validate( VC_POOL_T *pool,
                            VC_IMAGE_TYPE_T type,
                            uint32_t width,
                            uint32_t height,
                            uint32_t metadata_size,
                            VC_IMAGE_PROPLIST_T *proplist,
                            uint32_t n_props)
{
   // allocate corresponding vc_image struct
   VC_IMAGE_T tmp;

   // configure image
   vc_image_configure_proplist( &tmp, type, width, height, proplist, n_props);

   const int required_size = ALIGN_UP( metadata_size, 32 ) + vc_image_required_size( &tmp );

   return (required_size <= vc_pool_max_object_size (pool));
}

/* ----------------------------------------------------------------------
 * increase the refcount of the underlying pool object
 * -------------------------------------------------------------------- */
void
vc_pool_image_acquire( VC_IMAGE_T *img )
{
   VC_POOL_OBJECT_T *object = img->pool_object;
   if ( object )
      vc_pool_acquire( object );
   else
      vcos_assert(0); // must have been allocated from a vc_pool!
}

/* ----------------------------------------------------------------------
 * decrease the refcount of the underlying pool object.
 *
 * this may cause the object to be returned to the pool as free, in
 * which case we need to free the corresponding vc_image struct
 * -------------------------------------------------------------------- */
void
vc_pool_image_release( VC_IMAGE_T *img )
{
   if(img) {
      VC_POOL_OBJECT_T *object = img->pool_object;

      if ( object ) {
         int refcount;
         // If we have a metadata buffer stuck to the end, flush it from the
         // cache now in case we need to create a larger buffer later on.
         if (img->metadata_size) {
            VC_IMAGE_T lock_img;
            vc_image_lock( &lock_img, img );
            vc_metadata_cache_flush( lock_img.metadata );
            vc_image_unlock( &lock_img );
         }
   #if defined(VC_POOL_IMAGE_SCRUB)
         vc_pool_acquire( object );
         refcount = vc_pool_release( object );
         if (refcount == 1) {
            clear_image(img);
         }
   #endif
         refcount = vc_pool_release( object );
      }
      else
      {
         vcos_assert(0); // must have been allocated from a vc_pool!
      }
   }
   else {
      vcos_assert(0); // must have been allocated from a vc_pool!
   }
}


/******************************************************************************
 Global Functions
 *****************************************************************************/

#if defined(VC_POOL_IMAGE_SCRUB)
static void
clear_image(VC_IMAGE_T *img)
{
   static unsigned int random = 0;
   unsigned int colour = 0xff000000;
   VC_IMAGE_T lock_img;
   unsigned int *pptr;
   int i;

   if (++random > 7)
      random = 1;

   if (random & 4) colour |= 0xff;
   if (random & 2) colour |= 0xff00;
   if (random & 1) colour |= 0xff0000;

   vc_image_lock( &lock_img, img );

   switch ( lock_img.type )
   {
   case VC_IMAGE_YUV_UV:
      for (i = 0; i * VC_IMAGE_YUV_UV_STRIPE_WIDTH < lock_img.width; i++)
         memset(vc_image_get_y(&lock_img) + i * lock_img.pitch, colour & 255, lock_img.height * VC_IMAGE_YUV_UV_STRIPE_WIDTH);
      colour >>= 8;
      colour &= 0xffff;
      colour += colour << 16;
      for (i = 0; i * VC_IMAGE_YUV_UV_STRIPE_WIDTH < lock_img.width; i++) {
         int count = lock_img.height * (VC_IMAGE_YUV_UV_STRIPE_WIDTH / sizeof(colour) / 2);
         pptr = (void *)(vc_image_get_u(&lock_img) + i * lock_img.pitch);
         while (--count >= 0)
            *pptr++ = colour;
      }
      break;

   case VC_IMAGE_YUV422:
      for (i = 0; i < lock_img.height; i++) {
         memset(vc_image_get_y(&lock_img) + i * lock_img.pitch, colour & 255, lock_img.width);
         memset(vc_image_get_u(&lock_img) + i * lock_img.pitch, (colour >> 8) & 255, lock_img.width >> 1);
         memset(vc_image_get_v(&lock_img) + i * lock_img.pitch, (colour >> 16) & 255, lock_img.width >> 1);
      }
      break;

   case VC_IMAGE_YUV420:
   case VC_IMAGE_YUV422PLANAR:
      memset(vc_image_get_y(&lock_img), colour & 255, lock_img.pitch * lock_img.height);
      memset(vc_image_get_u(&lock_img), (colour >> 8) & 255, lock_img.pitch * lock_img.height >> (lock_img.type == VC_IMAGE_YUV420 ? 2 : 1));
      memset(vc_image_get_v(&lock_img), (colour >> 16) & 255, lock_img.pitch * lock_img.height >> (lock_img.type == VC_IMAGE_YUV420 ? 2 : 1));
      break;

   default:
      /* Unroll a packed pixel to fill the whole word irrespective of the
       * specific format -- this is pointless right now, because we haven't
       * actually made a packed pixel value, so either way we're just going to
       * fill the image with junk.  Also, we need a special variant of the loop
       * that will roll the word around to suit NPOT formats.
       */
#if 0
      colour = colour * (0xFFFFFFFFu / ((1 << vc_image_bits_per_pixel(lock_img.type)) - 1));
#endif
      pptr = lock_img.image_data;

      i = lock_img.size >> 2;
      while (--i >= 0) {
         *pptr++ = colour;
      }
   }
   vc_image_unlock( &lock_img );
}
#endif
