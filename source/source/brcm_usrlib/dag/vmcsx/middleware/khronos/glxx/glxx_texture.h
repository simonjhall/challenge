/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GLXX_TEXTURE_H
#define GLXX_TEXTURE_H

#include "interface/khronos/include/GLES2/gl2.h"

#include "middleware/khronos/common/khrn_image.h"

#define LOG2_MAX_TEXTURE_SIZE          11
#define MAX_TEXTURE_SIZE               (1 << LOG2_MAX_TEXTURE_SIZE)

#define TEXTURE_BUFFER_TWOD            0
#define TEXTURE_BUFFER_POSITIVE_X      1
#define TEXTURE_BUFFER_NEGATIVE_X      2
#define TEXTURE_BUFFER_POSITIVE_Y      3
#define TEXTURE_BUFFER_NEGATIVE_Y      4
#define TEXTURE_BUFFER_POSITIVE_Z      5
#define TEXTURE_BUFFER_NEGATIVE_Z      6

#define TEXTURE_BLOB_NULL 0
#define TEXTURE_STATE_UNDER_CONSTRUCTION 1
#define TEXTURE_STATE_COMPLETE_UNBOUND 2
#define TEXTURE_STATE_BOUND_TEXIMAGE 3
#define TEXTURE_STATE_BOUND_EGLIMAGE 4

/*

This structure represents the state of a GLES 1.1/2.0 texture object.

From the point of view of the API, a texture is described as follows:
- it is either a 2D texture or a cube map texture (is_cube)
- it has some information concerning how the texture is rendered (mag, min, wrap.s, wrap.t)
- it has 1 or 6 arrays of mipmaps. Each mipmap is a (possibly zero-size) 2D array of pixels, and has
  a width, height and format.
- in GLES1.1 it has a flag indicating whether mipmaps should be generated automatically

Textures may be used for rendering iff they are "complete", i.e. all the base level mipmaps are nonzero and the same
size (for cube maps), all mipmap images are the same format and each is half the dimensions of the previous
(rounded down until you get to 1). If the minification filter is GL_LINEAR or GL_NEAREST, only the base level
images are required.

Additionally, texture images can be shared with other API objects. Individual mipmaps may be used as the
color buffer of a framebuffer object, or they may be used as EGLImage sources. Texture data may also come from an
EGL pbuffer (via a call to eglBindTexImage) or an EGLImage (making the texture an EGLImage target). Once bound, it's
not really possible to distinguish between EGLIMage sources and targets - they're all considered to be EGLImage "siblings".

thing -> EGLImage -> thing
                  -> other thing
source               targets

Create source and target objects
Supply source object with data (e.g. glTexImage2D)
EGLImage object is created from source object with eglCreateImageKHR_impl
Target data is supplied by EGLImage with a call such as glEGLImageTargetTexture2DOES_impl_11


Our internal representation is a little more complicated than this, because we require a fast, memory-efficient path
as well as supporting all of the obscure corner cases. So texture data can be stored in two ways:
- in the blob
- as a separate image

For each mipmap image there are four possibilities:
(0) the image is zero-size. The blob_valid is not set and mh_mipmaps is MEM_INVALID_HANDLE
(1) the image is in the blob. blob_valid is set and mh_mipmaps is MEM_INVALID_HANDLE
(2) the image exists but is not in the blob (i.e. it might be the wrong size/format). blob_valid is not set and mh_mipmaps is valid.
(3) the image is in the blob but is shared with something (or might be). blob_valid is set and mh_mipmaps is valid.
  In this case, mh_mipmaps[ ][ ].mh_storage == mh_blob, i.e. the image is pointing directly into our blob.

The fast path is that everything is either (0) or (1).

When a blob is required (i.e. for (1) or (3)), binding_type can be anything other than TEXTURE_BLOB_NULL and
the width, height, format, offset and blob_mip_count parameters are valid, and describe the structure of the blob.

If a base level image is specified and the binding_type is TEXTURE_BLOB_NULL, a new blob will be created. This will
include space for mipmaps iff the current min filter suggests they are needed (and this will be recorded in blob_mip_count).

When a texture is to be used, it must be checked for completeness and "blobbed" first. Bound textures (in the
TEXTURE_STATE_BOUND_TEXIMAGE or TEXTURE_STATE_BOUND_EGLIMAGE state, and created via a different mechanism) are
always complete with all mipmaps present in the blob.

TODO to be continued...
*/

typedef struct {
   MEM_HANDLE_T mh_storage;
   KHRN_INTERLOCK_T interlock;
} GLXX_TEXTURE_BLOB_T;

#define GLXX_TEXTURE_POOL_SIZE 4

typedef struct {
   int32_t name;
   bool is_cube;

   GLenum mag;
   GLenum min;

   struct {
      GLenum s;
      GLenum t;
   } wrap;


   bool generate_mipmap;
   bool force_disable_mipmap;

   MEM_HANDLE_T mh_mipmaps[7][LOG2_MAX_TEXTURE_SIZE + 1];   /* floating KHRN_IMAGE_T */
   uint32_t explicit_mipmaps;   /* how many mh_mipmaps are not equal to MEM_INVALID_HANDLE (saves checking when it's 0) */

   uint32_t width;
   uint32_t height;
#ifdef RSO_ARGB8888_TEXTURE
   uint32_t preferRSO;
#endif
   KHRN_IMAGE_FORMAT_T format;   /* _LT: mipmaps are a mixture of TF and LT. _TF: mipmaps are all t-format */
   uint32_t offset;
   uint32_t blob_valid[7];   /* Bitmask. (blob_valid[buf] & 1<<lev) != 0   indicates image in buffer buf at level lev is present in the blob */
   uint32_t blob_mip_count;

   int current_item;
   GLXX_TEXTURE_BLOB_T blob_pool[GLXX_TEXTURE_POOL_SIZE];  /* If present, must be the mh_storage parameter for all mipmap images */
   MEM_HANDLE_T mh_depaletted_blob;

   uint32_t mip0_offset;

   /*
      binding_type
      This specifies which stage of construction the texture is in and which,
      if any, of the 2 relevant EGL binding mechanisms this texture is taking
      part in. Possibilities are:

      Incomplete states:

      TEXTURE_BLOB_NULL
      Blob is empty, though some mipmaps may be individually specified. New textures start off in this state.
      width, height, format, etc. are all invalid. blob_valid should all be zero.

      TEXTURE_STATE_UNDER_CONSTRUCTION
      Some mipmaps may not have been specified, or they may not all agree.
      width, height, format etc. are valid.

      Complete states:

      TEXTURE_STATE_COMPLETE_UNBOUND
      All mipmaps are present. Not bound.
      (Note binding is not possible for cube map textures).

      TEXTURE_STATE_BOUND_TEXIMAGE
      Bound using eglBindTexImage. All of our mh_mipmaps have the IMAGE_FLAG_BOUND_TEXIMAGE
      flag and are attached to the same pbuffer.
      When a new mipmap is attached to this texture, all old mipmaps get set to
      invalid.
      When eglReleaseTexImage is called, the IMAGE_FLAG_BOUND_TEXIMAGE flag
      disappears from the images. We detect this and set all our mipmaps to invalid.

      TEXTURE_STATE_BOUND_EGLIMAGE
      Bound using glEGLImageTargetTexture2DOES (EGLImage target) or
      eglCreateImageKHR (EGLImage source).
      All of our mipmaps have the IMAGE_FLAG_BOUND_EGLIMAGE flag.
      When a new mipmap is attached to this texture, we take a copy of all attached
      images. (TODO: it says this explicitly for EGLImage targets, but what about
      sources?)

   */
   uint32_t binding_type;
   bool mipmaps_available;     // false if bound to pbuffer without mipmaps
   bool framebuffer_sharing;  /* some sharing is going on, so don't discard any mh_mipmaps we've constructed */
   
   /* GL_OES_draw_texture define a crop rectangle */
   struct {
      GLint Ucr;
      GLint Vcr;
      GLint Wcr;
      GLint Hcr;
   } crop_rect;

#ifdef __VIDEOCORE4__
   KHRN_INTERLOCK_T interlock;
#else
   uint32_t fifo_read_count;
#endif
} GLXX_TEXTURE_T;

typedef enum {
   INCOMPLETE,
   COMPLETE,
   OUT_OF_MEMORY
} GLXX_TEXTURE_COMPLETENESS_T;

/* Operations on incomplete textures */
extern void glxx_texture_init(GLXX_TEXTURE_T *texture, int32_t name, bool is_cube);
extern void glxx_texture_term(void *v, uint32_t size);

extern bool glxx_texture_image(GLXX_TEXTURE_T *texture, GLenum target, uint32_t level, uint32_t width, uint32_t height, GLenum fmt, GLenum type, uint32_t align, const void *pixels);
extern bool glxx_texture_etc1_blank_image(GLXX_TEXTURE_T *texture, GLenum target, uint32_t level, uint32_t width, uint32_t height);
extern void glxx_texture_etc1_sub_image(GLXX_TEXTURE_T *texture, GLenum target, uint32_t level, uint32_t dstx, uint32_t dsty, uint32_t width, uint32_t height, const void *data);
extern bool glxx_texture_copy_image(GLXX_TEXTURE_T *texture, GLenum target, uint32_t level, int width, int height, GLenum fmt, const KHRN_IMAGE_T *src, int srcx, int srcy);
extern bool glxx_texture_sub_image(GLXX_TEXTURE_T *texture, GLenum target, uint32_t level, uint32_t dstx, uint32_t dsty, uint32_t width, uint32_t height, GLenum fmt, GLenum type, uint32_t align, const void *pixels);
extern bool glxx_texture_copy_sub_image(GLXX_TEXTURE_T *texture, GLenum target, uint32_t level, uint32_t dstx, uint32_t dsty, uint32_t width, uint32_t height, const KHRN_IMAGE_T *src, uint32_t srcx, uint32_t srcy);
extern bool glxx_texture_paletted_blank_image(GLXX_TEXTURE_T *texture, GLenum target, uint32_t levels, uint32_t width, uint32_t height, KHRN_IMAGE_FORMAT_T format);
extern void glxx_texture_paletted_sub_image(GLXX_TEXTURE_T *texture, GLenum target, uint32_t levels, uint32_t width, uint32_t height, const uint32_t *palette, uint32_t palsize, const void *data, int data_offset, int data_length);

extern bool glxx_texture_generate_mipmap(GLXX_TEXTURE_T *texture, uint32_t buffer, bool *invalid_operation);

extern bool glxx_texture_is_cube_complete(GLXX_TEXTURE_T *texture);

extern GLXX_TEXTURE_COMPLETENESS_T glxx_texture_check_complete(GLXX_TEXTURE_T *texture, bool forbid_palette);

extern void glxx_texture_bind_images(GLXX_TEXTURE_T *texture, uint32_t levels, MEM_HANDLE_T *images, uint32_t binding_type, MEM_HANDLE_T bound_data, int mipmap_level);
extern void glxx_texture_release_teximage(GLXX_TEXTURE_T *texture);

extern bool glxx_texture_includes(GLXX_TEXTURE_T *texture, GLenum target, int level, int x, int y);

/* Operations on complete textures */
extern uint32_t glxx_texture_get_mipmap_offset(GLXX_TEXTURE_T *texture, uint32_t buffer, uint32_t level, bool depaletted);
extern void glxx_texture_get_mipmap_offsets(GLXX_TEXTURE_T *texture, uint32_t buffer, bool depaletted, uint32_t *offsets);
extern KHRN_IMAGE_FORMAT_T glxx_texture_get_mipmap_format(GLXX_TEXTURE_T *texture, uint32_t level, bool depaletted);
extern KHRN_IMAGE_FORMAT_T glxx_texture_incomplete_get_mipmap_format(GLXX_TEXTURE_T *texture, GLenum target, uint32_t level);
extern KHRN_IMAGE_FORMAT_T glxx_texture_get_tformat(GLXX_TEXTURE_T *texture, bool depaletted);
extern uint32_t glxx_texture_get_mipmap_count(GLXX_TEXTURE_T *texture);

#ifdef __VIDEOCORE4__
extern uint32_t glxx_texture_get_cube_stride(GLXX_TEXTURE_T *texture);
#endif

/* Helper functions */
extern bool glxx_texture_is_valid_image(KHRN_IMAGE_T *image);
extern uint32_t glxx_texture_get_interlock_offset(GLXX_TEXTURE_T *texture);
extern MEM_HANDLE_T glxx_texture_get_storage_handle(GLXX_TEXTURE_T *texture);

/* Sharing */
extern MEM_HANDLE_T glxx_texture_share_mipmap(GLXX_TEXTURE_T *texture, uint32_t buffer, uint32_t level);

/* GLES1.1 annoyingness */
extern void glxx_texture_has_color_alpha(GLXX_TEXTURE_T *texture, bool *has_color, bool *has_alpha, bool *complete);

/* GL_OES_draw_texture */
void glxx_texture_set_crop_rect(GLXX_TEXTURE_T *texture, const GLint * params);

#endif
