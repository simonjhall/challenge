/*=============================================================================
Copyright © 2008 Broadcom Europe Limited.

All rights reserved.

FILE DESCRIPTION
Buffer Management
=============================================================================*/

//standard header files
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "interface/vchi/vchi.h"
#include "interface/vchi/os/os.h"
#include "interface/vmcs_host/vc_vchi_bufman.h"
#include "interface/vmcs_host/vc_bufservice_defs.h"
#include "interface/vcos/vcos.h"

/* hack: until Eben makes his os compatible with vchi os */
#include "interface/vmcs_host/vchiu_hack.c"

#undef assert // remember to use os_assert

#define PUSHMULTI
#define PULLMULTI

#define CHECK_FIXED16_16(t) os_assert(!((t)>0  && (t)<1<<16))
#define CHECK_FIXED32_00(t) os_assert( ((t)>=0 && (t)<1<<16))
#define CHECK_FIXED16_16_RECT(r) do {CHECK_FIXED16_16(r.x); CHECK_FIXED16_16(r.y); CHECK_FIXED16_16(r.width); CHECK_FIXED16_16(r.height);}while (0)
#define CHECK_FIXED32_00_RECT(r) do {CHECK_FIXED32_00(r.x); CHECK_FIXED32_00(r.y); CHECK_FIXED32_00(r.width); CHECK_FIXED32_00(r.height);}while (0)

#ifndef VCCPRE_
#define VCCPRE_
#endif

// This source file is compiled as C++ under Symbian, to allow namespacing. Hence various
// C++-isms throughout the file (eg casting from void *).
#ifdef __SYMBIAN32__
namespace BufManX {
#endif

enum { BUFMAN_SIGNAL_VCHI_CALLBACK_MSG_AVAILABLE, BUFMAN_SIGNAL_VCHI_CALLBACK_BULK_SENT, BUFMAN_SIGNAL_VCHI_CALLBACK_BULK_RECEIVED };

#define VCHI_ALIGNMENT 16

// queue for outstanding bufman pulls/pushes
#define BUFMANX_QUEUE_SIZE 256
#define SWAP(x,y) do{int t=(x);(x)=(y);(y)=t;}while(0)

typedef struct {
   VCHI_SERVICE_HANDLE_T vchi_handle[VCHI_MAX_NUM_CONNECTIONS];
   VCHI_INSTANCE_T initialise_instance;
   VCHI_CONNECTION_T **connections;
   unsigned num_connections;
   VCOS_THREAD_T task;
   OS_EVENTGROUP_T bufman_message_available; // this flags a vchi callback has arrived
   OS_SEMAPHORE_T bufman_message_atomicity;  // need to keep together vchi messages for a single operation
   OS_SEMAPHORE_T bufman_temp_buffer;  // only one user of this at a time
   OS_SEMAPHORE_T callback_queue_push_lock;  // need to protect pushes from multiple threads (pops are single threaded)
   VCHIU_QUEUE_T callback_queue;
   void *transform_buffer;
   int transform_buffer_size;
   uint8_t ignore;
} BUFMAN_STATE_T;
#define IGNORE ((void *)&vc_state.ignore)

static BUFMAN_STATE_T vc_state;

enum operation {OPERATION_PUSH=1, OPERATION_PULL, OPERATION_SUBMIT_PUSH, OPERATION_SUBMIT_PULL, };

typedef struct bufmanx_handle_s {
   enum operation operation; // just for debugging
   BUF_MSG_T msg;
   BUFMANX_IMAGE_T h_image;
   DISPMANX_RESOURCE_HANDLE_T v_image;
   VC_RECT_T src_rect;
   VC_RECT_T dest_rect;
   vc_bufman_callback_t callback;
   void *cookie, *cookie2;
   // we sometimes use the callback internally and push the user callback into next_callback
   vc_bufman_callback_t next_callback;
   void *next_cookie, *next_cookie2;
   int stripe_height, current_stripe, total_stripes;
   BUFMAN_TRANSFORM_T transform;
   BUFMANX_IMAGE_T post_transpose_image;
   int32_t success;
} BUFMANX_OBJECT_T;

#define STRIPE_HEIGHT 16

static void bufman_message_task( unsigned argc, void *argv );
static void bufman_service_callback(void *callback_param, const VCHI_CALLBACK_REASON_T reason, void *msg_handle);
static void push_striped_multi(BUFMANX_OBJECT_T *h);
static void pull_striped_multi(BUFMANX_OBJECT_T *h);
static void pull_striped_callback(void *userdata, void *cookie2, int32_t success);

static void vc_bufmanx_service_use(void)
{
   int32_t error;
   if (!vc_state.vchi_handle[0]) {
      // now the task is running we can register the service with the VCHI interface
      int i;
      for (i=0;i<vc_state.num_connections;i++)
      {
         SERVICE_CREATION_T service_parameters = {MAKE_FOURCC("BFMX"),                          // 4cc service code
                                                  NULL,                                         // passed in fn ptrs
                                                  0,                                            // tx fifo size (unused)
                                                  0,                                            // tx fifo size (unused)
                                                  &bufman_service_callback,                     // service callback
                                                  &vc_state,                                    // callback parameter
                                                  1,     // client intends to receive bulk transfers of odd lengths or into unaligned buffers
                                                  1,     // client intends to transmit bulk transfers of odd lengths or out of unaligned buffers
                                                  0 };   // client wants to check CRCs on (bulk) transfers. Only needs to be set at 1 end - will do both directions.
         service_parameters.connection = vc_state.connections[i];

         // Create a connection to 'BFMX' service
         error = vchi_service_open(vc_state.initialise_instance, &service_parameters, &vc_state.vchi_handle[i]);
         os_assert(error == 0);
      }
   } else {
      error = vchi_service_use(vc_state.vchi_handle[0]);
      os_assert(error == 0);
   }
}

static void vc_bufmanx_service_release(void)
{
   int32_t error = vchi_service_release(vc_state.vchi_handle[0]);
   os_assert(error == 0);
}

void vc_vchi_bufman_init(VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections)
{
   const unsigned BUFMAN_STACK_SIZE = 4000;
   int32_t error;
    memset(&vc_state, 0, sizeof vc_state);

   //record the params passed in for later use
   vc_state.initialise_instance = initialise_instance;
   vc_state.connections = connections;
   vc_state.num_connections = num_connections;

   error = os_eventgroup_create( &vc_state.bufman_message_available, "BUFM_AV" );
   os_assert( error == 0 );

   error = os_semaphore_create( &vc_state.bufman_message_atomicity, OS_SEMAPHORE_TYPE_SUSPEND );
   os_assert( error == 0 );

   error = os_semaphore_create( &vc_state.bufman_temp_buffer, OS_SEMAPHORE_TYPE_SUSPEND );
   os_assert( error == 0 );

   error = os_thread_start( &vc_state.task, bufman_message_task, &vc_state, BUFMAN_STACK_SIZE, "BUFM" );
   os_assert( error == 0 );

   // simple queue of BUFMANX_OBJECT_T that are in process of being pulled/pushed
   vchiu_queue_init(&vc_state.callback_queue, BUFMANX_QUEUE_SIZE); 

   error = os_semaphore_create( &vc_state.callback_queue_push_lock, OS_SEMAPHORE_TYPE_SUSPEND );
   os_assert( error == 0 );
}

static int32_t vchi_get_response(BUF_MSG_RESPONSE_T *response, int connection)
{
   BUF_MSG_T msg;
   uint32_t actual_size;
   int32_t error;
   msg.u.message_response.status = -1;
   error = vchi_msg_dequeue(vc_state.vchi_handle[connection],
                            &msg, sizeof msg,
                            &actual_size, VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE);
   os_assert(error == 0);
   os_assert(actual_size == sizeof msg);
   if (response) *response = msg.u.message_response;
   return msg.u.message_response.status;
}

/******************************************************************************
NAME
   vc_bufmanx_convert_init

SYNOPSIS
   int32_t vc_bufmanx_convert_init(void)

FUNCTION
   This allocates starts up bufman convert service and allocates conversion buffers on videocore
   Must be called before any other bufman functions.

RETURNS
   -
******************************************************************************/
int32_t vc_bufmanx_convert_init(void)
{
   return 0;
}

#define GENERATE_vc_bufmanx_hflip_inplace(TYPE) \
static void vc_bufmanx_hflip_inplace_##TYPE(void *pixels, int width, int height, int pitch) \
{                                                            \
   int i,j;                                                  \
   TYPE t;                                                   \
   for (j=0; j<height; j++) {                                \
      TYPE *p = (TYPE *)((uint8_t *)pixels+j*pitch) + width-1;  \
      TYPE *q = (TYPE *)((uint8_t *)pixels+j*pitch);            \
      for (i=0; i<width>>1; i++, p--, q++)                   \
         t = *q,  *q = *p, *p = t;                           \
   }                                                         \
}

#define GENERATE_vc_bufmanx_vflip_inplace(TYPE) \
static void vc_bufmanx_vflip_inplace_##TYPE(void *pixels, int width, int height, int pitch) \
{                                                            \
   int i,j;                                                  \
   TYPE t;                                                   \
   for (j=0; j<height>>1; j++) {                             \
      TYPE *p = (TYPE *)((uint8_t *)pixels+(height-1-j)*pitch); \
      TYPE *q = (TYPE *)((uint8_t *)pixels+j*pitch);            \
      for (i=0; i<width; i++, p++, q++)                      \
         t = *q,  *q = *p, *p = t;                           \
   }                                                         \
}

#define GENERATE_vc_bufmanx_transpose(TYPE) \
static void vc_bufmanx_transpose_##TYPE(void *d_pixels, void *s_pixels, int width, int height, int d_pitch, int s_pitch, BUFMAN_TRANSFORM_T transform) \
{                                                                    \
   const int h = (transform & BUFMAN_TRANSFORM_HFLIP) != 0;          \
   const int v = (transform & BUFMAN_TRANSFORM_VFLIP) != 0;          \
   const int type_size = sizeof(TYPE);                               \
   const int q_i = sizeof(TYPE), q_j = d_pitch - width*type_size;    \
   int i,j;                                                          \
   int p_i, p_j;                                                     \
   uint8_t *p, *q = (uint8_t *)d_pixels;                             \
   if (h && v) p = (uint8_t *)s_pixels+(width-1)*s_pitch+(height-1)*type_size, p_i = -s_pitch, p_j = -type_size + s_pitch*width; \
   else if (v) p = (uint8_t *)s_pixels                  +(height-1)*type_size, p_i =  s_pitch, p_j = -type_size - s_pitch*width; \
   else if (h) p = (uint8_t *)s_pixels+(width-1)*s_pitch                     , p_i = -s_pitch, p_j =  type_size + s_pitch*width; \
   else        p = (uint8_t *)s_pixels                                       , p_i =  s_pitch, p_j =  type_size - s_pitch*width; \
   for (j=0; j<height; j++) {                                        \
      for (i=0; i<width; i++) {                                      \
         *(TYPE *)q = *(TYPE *)p;                                    \
         p += p_i;                                                   \
         q += q_i;                                                   \
      }                                                              \
      p += p_j;                                                      \
      q += q_j;                                                      \
   }                                                                 \
}

GENERATE_vc_bufmanx_hflip_inplace(uint8_t)
GENERATE_vc_bufmanx_hflip_inplace(uint16_t)
GENERATE_vc_bufmanx_hflip_inplace(uint32_t)
GENERATE_vc_bufmanx_vflip_inplace(uint8_t)
GENERATE_vc_bufmanx_vflip_inplace(uint16_t)
GENERATE_vc_bufmanx_vflip_inplace(uint32_t)
GENERATE_vc_bufmanx_transpose(uint8_t)
GENERATE_vc_bufmanx_transpose(uint16_t)
GENERATE_vc_bufmanx_transpose(uint32_t)

static void vc_bufmanx_hflip_inplace_uint24_t(void *pixels, int width, int height, int pitch)
{
   int i,j;
   for (j=0; j<height; j++) {
      uint8_t *p = (uint8_t *)pixels+j*pitch + 3*(width-1);
      uint8_t *q = (uint8_t *)pixels+j*pitch;
      for (i=0; i<width>>1; i++, p-=3, q+=3) {
         uint8_t t0 = q[0], t1 = q[1], t2 = q[2];
         q[0] = p[0], q[1] = p[1], q[2] = p[2];
         p[0] = t0, p[1] = t1, p[2] = t2;
      }
   }
}

static void vc_bufmanx_vflip_inplace_uint24_t(void *pixels, int width, int height, int pitch)
{
   int i,j;
   for (j=0; j<height>>1; j++) {
      uint8_t *p = (uint8_t *)pixels+(height-1-j)*pitch;
      uint8_t *q = (uint8_t *)pixels+j*pitch;
      for (i=0; i<width; i++, p+=3, q+=3) {
         uint8_t t0 = q[0], t1 = q[1], t2 = q[2];
         q[0] = p[0], q[1] = p[1], q[2] = p[2];
         p[0] = t0, p[1] = t1, p[2] = t2;
      }
   }
}

static void vc_bufmanx_transpose_uint24_t(void *d_pixels, void *s_pixels, int width, int height, int d_pitch, int s_pitch, BUFMAN_TRANSFORM_T transform)
{
   const int h = (transform & BUFMAN_TRANSFORM_HFLIP) != 0;
   const int v = (transform & BUFMAN_TRANSFORM_VFLIP) != 0;
   const int type_size = 3;
   const int q_i = type_size, q_j = d_pitch - width*type_size;
   int i,j;
   int p_i, p_j;
   uint8_t *p, *q = (uint8_t *)d_pixels;
   if (h && v) p = (uint8_t *)s_pixels+(width-1)*s_pitch+(height-1)*type_size, p_i = -s_pitch, p_j = -type_size + s_pitch*width;
   else if (v) p = (uint8_t *)s_pixels                  +(height-1)*type_size, p_i =  s_pitch, p_j = -type_size - s_pitch*width;
   else if (h) p = (uint8_t *)s_pixels+(width-1)*s_pitch                     , p_i = -s_pitch, p_j =  type_size + s_pitch*width;
   else        p = (uint8_t *)s_pixels                                       , p_i =  s_pitch, p_j =  type_size - s_pitch*width;
   for (j=0; j<height; j++) {
      for (i=0; i<width; i++) {
         q[0] = p[0], q[1] = p[1], q[2] = p[2];
         p += p_i;
         q += q_i;
      }
      p += p_j;
      q += q_j;
   }
}

/* aka yuv4220nr: y1v0y0u0 y3v1y2u2 */
static void vc_bufmanx_hflip_inplace_yuv422p(void *pixels, int width, int height, int pitch)
{
   int i,j;
   for (j=0; j<height; j++) {
      uint8_t *p = (uint8_t *)pixels+j*pitch + 4*((width>>1)-1);
      uint8_t *q = (uint8_t *)pixels+j*pitch;
      for (i=0; i<width>>2; i++, p-=4, q+=4) {
         uint8_t t0 = q[0], t1=q[1], t2=q[2], t3=q[3];
         q[0] = p[2], q[1] = p[1], q[2] = p[0], q[3] = p[3];
         p[0] = t2, p[1] = t1, p[2] = t2, p[3] = t3;
      }
   }
}

/* aka yuv4220r: u0y0v0y1 u1y2v1y3 */
static void vc_bufmanx_hflip_inplace_yuv422le(void *pixels, int width, int height, int pitch)
{
   int i,j;
   for (j=0; j<height; j++) {
      uint8_t *p = (uint8_t *)pixels+j*pitch + 4*((width>>1)-1);
      uint8_t *q = (uint8_t *)pixels+j*pitch;
      for (i=0; i<width>>2; i++, p-=4, q+=4) {
         uint8_t t0 = q[0], t1=q[1], t2=q[2], t3=q[3];
         q[0] = p[0], q[1] = p[3], q[2] = p[2], q[3] = p[1];
         p[0] = t0, p[1] = t3, p[2] = t2, p[3] = t1;
      }
   }
}

/* aka yuv4220nr: y1v0y0u0 y3v1y2u2 */
static void vc_bufmanx_transpose_yuv422p(void *d_pixels, void *s_pixels, int width, int height, int d_pitch, int s_pitch, BUFMAN_TRANSFORM_T transform)
{
   int i,j;
   for (j=0; j<height; j++) {
      uint8_t *y = (uint8_t *)s_pixels+2*(j^1);
      uint8_t *u = (uint8_t *)s_pixels+2*(j&~1)+3;
      uint8_t *v = (uint8_t *)s_pixels+2*(j&~1)+1;
      uint8_t *q = (uint8_t *)d_pixels+j*d_pitch;
      for (i=0; i<width>>1; i++) {
         q[0] = y[s_pitch];
         q[1] = v[0];
         q[2] = y[0];
         q[3] = u[0];
         q += 4; y += 2*s_pitch; u += 2*s_pitch; v += 2*s_pitch;
      }
   }
   if (transform & BUFMAN_TRANSFORM_HFLIP)
      vc_bufmanx_hflip_inplace_yuv422p(d_pixels, width, height, d_pitch);
   if (transform & BUFMAN_TRANSFORM_VFLIP)
      vc_bufmanx_vflip_inplace_uint16_t(d_pixels, width, height, d_pitch);
}

/* aka yuv4220r: u0y0v0y1 u1y2v1y3 */
static void vc_bufmanx_transpose_yuv422le(void *d_pixels, void *s_pixels, int width, int height, int d_pitch, int s_pitch, BUFMAN_TRANSFORM_T transform)
{
   int i,j;
   for (j=0; j<height; j++) {
      uint8_t *y = (uint8_t *)s_pixels+2*j+1;
      uint8_t *u = (uint8_t *)s_pixels+2*(j&~1);
      uint8_t *v = (uint8_t *)s_pixels+2*(j&~1)+2;
      uint8_t *q = (uint8_t *)d_pixels+j*d_pitch;
      for (i=0; i<width>>1; i++) {
         q[0] = u[0];
         q[1] = y[0];
         q[2] = v[0];
         q[3] = y[s_pitch];
         q += 4; y += 2*s_pitch; u += 2*s_pitch; v += 2*s_pitch;
      }
   }
   if (transform & BUFMAN_TRANSFORM_HFLIP)
      vc_bufmanx_hflip_inplace_yuv422le(d_pixels, width, height, d_pitch);
   if (transform & BUFMAN_TRANSFORM_VFLIP)
      vc_bufmanx_vflip_inplace_uint16_t(d_pixels, width, height, d_pitch);
}


VCCPRE_ int32_t vc_bufmanx_set_transform_buffer(void *pixels, int size)
{
   int32_t error, success = 0;

   error = os_semaphore_obtain(&vc_state.bufman_temp_buffer);
   os_assert(error == 0);

   vc_state.transform_buffer = pixels;
   vc_state.transform_buffer_size = size;

   error = os_semaphore_release(&vc_state.bufman_temp_buffer);
   os_assert(error == 0);
   return success;
}

static int32_t obtain_temp_buffer(BUFMANX_IMAGE_T *image)
{
   int32_t error, success = 0;

   error = os_semaphore_obtain(&vc_state.bufman_temp_buffer);
   os_assert(error == 0);

   os_assert(image->pixels == NULL);

   if (vc_state.transform_buffer) {
      if (vc_bufmanx_get_default_size(image) <= vc_state.transform_buffer_size)
         image->pixels = vc_state.transform_buffer;
      else
         success = VC_BUFMAN_ERROR_BAD_SIZE;
   } else {
      image->pixels = os_malloc(vc_bufmanx_get_default_size(image), 16, "transpose buffer");
      if (!image->pixels)
         success = VC_BUFMAN_ERROR_BAD_SIZE;
   }
   return success;
}
static int32_t release_temp_buffer(BUFMANX_IMAGE_T *image)
{
   int32_t error;

   os_assert(image->pixels);
   if (!vc_state.transform_buffer)
      os_free(image->pixels);
   image->pixels = NULL;

   error = os_semaphore_release(&vc_state.bufman_temp_buffer);
   os_assert(error == 0);

   return 0;
}

static int32_t vc_bufmanx_transform(BUFMANX_IMAGE_T *post, const BUFMANX_IMAGE_T *pre, BUFMAN_TRANSFORM_T transform)
{
   if (transform & BUFMAN_TRANSFORM_TRANSPOSE) {
      os_assert(post->width == pre->height && post->height == pre->width);
      switch (pre->type) {
         case FRAME_HOST_IMAGE_EFormatYuv420P:
         {
            int s_y_size  = pre->pitch * pre->height;
            int s_uv_size = (pre->pitch>>1) * (pre->height>>1);
            char *s_y = (char *)pre->pixels;
            char *s_u = s_y + s_y_size;
            char *s_v = s_u + s_uv_size;
            int d_y_size  = post->pitch * post->height;
            int d_uv_size = (post->pitch>>1) * (post->height>>1);
            char *d_y = (char *)post->pixels;
            char *d_u = d_y + d_y_size;
            char *d_v = d_u + d_uv_size;
            vc_bufmanx_transpose_uint8_t(d_y, s_y, post->width, post->height, post->pitch, pre->pitch, transform);
            vc_bufmanx_transpose_uint8_t(d_u, s_u, post->width>>1, post->height>>1, post->pitch>>1, pre->pitch>>1, transform);
            vc_bufmanx_transpose_uint8_t(d_v, s_v, post->width>>1, post->height>>1, post->pitch>>1, pre->pitch>>1, transform);
            break;
         }
         case FRAME_HOST_IMAGE_EFormatYuv422P: vc_bufmanx_transpose_yuv422p(post->pixels, pre->pixels, post->width, post->height, post->pitch, pre->pitch, transform); break;
         case FRAME_HOST_IMAGE_EFormatYuv422LE: vc_bufmanx_transpose_yuv422le(post->pixels, pre->pixels, post->width, post->height, post->pitch, pre->pitch, transform); break;
         case FRAME_HOST_IMAGE_EFormatRgb565: vc_bufmanx_transpose_uint16_t(post->pixels, pre->pixels, post->width, post->height, post->pitch, pre->pitch, transform); break;
         case FRAME_HOST_IMAGE_EFormatRgb888: vc_bufmanx_transpose_uint24_t(post->pixels, pre->pixels, post->width, post->height, post->pitch, pre->pitch, transform); break;
         case FRAME_HOST_IMAGE_EFormatRgbU32:
         case FRAME_HOST_IMAGE_EFormatRgbU32LE:
         case FRAME_HOST_IMAGE_EFormatRgbA32LE:
         case FRAME_HOST_IMAGE_EFormatRgbA32: vc_bufmanx_transpose_uint32_t(post->pixels, pre->pixels, post->width, post->height, post->pitch, pre->pitch, transform); break;
         default: os_assert(0); break;
      }
      return 0;
   }
   if (transform & BUFMAN_TRANSFORM_HFLIP) {
      switch (post->type) {
         case FRAME_HOST_IMAGE_EFormatYuv420P:
         {
            int y_size  = post->pitch * post->height;
            int uv_size = (post->pitch>>1) * (post->height>>1);
            char *y = (char *)post->pixels;
            char *u = y + y_size;
            char *v = u + uv_size;
            vc_bufmanx_hflip_inplace_uint8_t(y, post->width, post->height, post->pitch);
            vc_bufmanx_hflip_inplace_uint8_t(u, post->width>>1, post->height>>1, post->pitch>>1);
            vc_bufmanx_hflip_inplace_uint8_t(v, post->width>>1, post->height>>1, post->pitch>>1);
            break;
         }
         case FRAME_HOST_IMAGE_EFormatYuv422P: vc_bufmanx_hflip_inplace_yuv422p(post->pixels, post->width, post->height, post->pitch); break;
         case FRAME_HOST_IMAGE_EFormatYuv422LE: vc_bufmanx_hflip_inplace_yuv422le(post->pixels, post->width, post->height, post->pitch); break;
         case FRAME_HOST_IMAGE_EFormatRgb565: vc_bufmanx_hflip_inplace_uint16_t(post->pixels, post->width, post->height, post->pitch); break;
         case FRAME_HOST_IMAGE_EFormatRgb888: vc_bufmanx_hflip_inplace_uint24_t(post->pixels, post->width, post->height, post->pitch); break;
         case FRAME_HOST_IMAGE_EFormatRgbU32:
         case FRAME_HOST_IMAGE_EFormatRgbU32LE:
         case FRAME_HOST_IMAGE_EFormatRgbA32LE:
         case FRAME_HOST_IMAGE_EFormatRgbA32: vc_bufmanx_hflip_inplace_uint32_t(post->pixels, post->width, post->height, post->pitch); break;
         default: os_assert(0); break;
      }
   }
   if (transform & BUFMAN_TRANSFORM_VFLIP) {
      switch (post->type) {
         case FRAME_HOST_IMAGE_EFormatYuv420P:
         {
            int y_size  = post->pitch * post->height;
            int uv_size = (post->pitch>>1) * (post->height>>1);
            char *y = (char *)post->pixels;
            char *u = y + y_size;
            char *v = u + uv_size;
            vc_bufmanx_vflip_inplace_uint8_t(y, post->width, post->height, post->pitch);
            vc_bufmanx_vflip_inplace_uint8_t(u, post->width>>1, post->height>>1, post->pitch>>1);
            vc_bufmanx_vflip_inplace_uint8_t(v, post->width>>1, post->height>>1, post->pitch>>1);
            break;
         }
         case FRAME_HOST_IMAGE_EFormatYuv422P:
         case FRAME_HOST_IMAGE_EFormatYuv422LE:
         case FRAME_HOST_IMAGE_EFormatRgb565: vc_bufmanx_vflip_inplace_uint16_t(post->pixels, post->width, post->height, post->pitch); break;
         case FRAME_HOST_IMAGE_EFormatRgb888: vc_bufmanx_vflip_inplace_uint24_t(post->pixels, post->width, post->height, post->pitch); break;
         case FRAME_HOST_IMAGE_EFormatRgbU32:
         case FRAME_HOST_IMAGE_EFormatRgbU32LE:
         case FRAME_HOST_IMAGE_EFormatRgbA32LE:
         case FRAME_HOST_IMAGE_EFormatRgbA32: vc_bufmanx_vflip_inplace_uint32_t(post->pixels, post->width, post->height, post->pitch); break;
         default: os_assert(0); break;
      }
   }
   return 0;
}

static void bufman_push_sync(BUFMANX_OBJECT_T *h)
{
   int32_t error, status;
   BUF_MSG_T m;
   memset(&m, 0, sizeof m);

   error = os_semaphore_obtain(&vc_state.bufman_message_atomicity);
   os_assert( error == 0 );
   m.hdr.command = VC_BUFMAN_SYNC;
   status = vchi_msg_queue(vc_state.vchi_handle[0], &m, sizeof m, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
   os_assert(status == 0);
   status = vchi_get_response(NULL, 0);
   os_assert(status == 0);
   error = os_semaphore_release(&vc_state.bufman_message_atomicity);
   os_assert( error == 0 );
}

/******************************************************************************
FUNCTION
   Waits to be signalled that messages have arrived and then processes them
******************************************************************************/
static void bufman_message_task( unsigned argc, void *argv )
{
   BUFMAN_STATE_T *state = (BUFMAN_STATE_T *)argv;

   while (1) {
      uint32_t events;
      int32_t status;
      status = os_eventgroup_retrieve( &state->bufman_message_available, &events );
      os_assert((events & (events-1)) == 0   );// 1 bits set
      os_assert(status == 0);
      os_logging_message("VCBUFMAN: bufman_message_task (events=0x%x)", events);

      while (!vchiu_queue_is_empty(&vc_state.callback_queue)) {
         BUFMANX_OBJECT_T *h = (BUFMANX_OBJECT_T *)vchiu_queue_pop(&vc_state.callback_queue);
         os_logging_message("VCBUFMAN: bufman_message_task (h=0x%X, op=%d, callback=0x%X, cookie=0x%X, cookie2=0x%X status=%d)", h, h->operation, h->callback, h->cookie, h->cookie2, h->success);
         switch (h->operation) {
            case OPERATION_PUSH:
            // need to sync here
            if (h->callback) 
               bufman_push_sync(h);
            // fall through
            case OPERATION_PULL:
               if (h->callback)
                  h->callback(h->cookie, h->cookie2, h->success);
               break;
            case OPERATION_SUBMIT_PULL:
               pull_striped_multi(h);
               break;
            case OPERATION_SUBMIT_PUSH:
               push_striped_multi(h);
               break;
            default: os_assert(0); break;
         }
      }
   }
}


static char *get_reason(const VCHI_CALLBACK_REASON_T reason)
{
   switch (reason) {
      case VCHI_CALLBACK_MSG_AVAILABLE: return "VCHI_CALLBACK_MSG_AVAILABLE";
      case VCHI_CALLBACK_MSG_SENT: return "VCHI_CALLBACK_MSG_SENT";
      case VCHI_CALLBACK_MSG_SPACE_AVAILABLE: return "VCHI_CALLBACK_MSG_SPACE_AVAILABLE";
      case VCHI_CALLBACK_BULK_RECEIVED: return "VCHI_CALLBACK_BULK_RECEIVED";
      case VCHI_CALLBACK_BULK_SENT: return "VCHI_CALLBACK_BULK_SENT";
      default: return NULL;
   }
}

/******************************************************************************
NAME
   bufman_service_callback

SYNOPSIS
void bufman_service_callback( void *callback_param,
                              const VCHI_CALLBACK_REASON_T reason,
                              const void *msg_handle )

FUNCTION
   This is passed to the VCHI layer and is called when there is data available
   to read or when a message has been transmitted

RETURNS
   -
******************************************************************************/
static void bufman_service_callback(void *callback_param,
                                    const VCHI_CALLBACK_REASON_T reason,
                                    void *msg_handle)
{
   int32_t status;
   uint32_t sig = 0;
   BUFMAN_STATE_T *state = (BUFMAN_STATE_T *)callback_param;
   //os_assert(msg_handle == NULL);
   const char *reason_text = get_reason(reason);

   if (reason_text)
      os_logging_message("VCBUFMAN: bufman_service_callback (reason=%s, param=0x%X, handle=0x%X)", reason_text, callback_param, msg_handle);
   else
      os_logging_message("VCBUFMAN: bufman_service_callback (reason=%d, param=0x%X, handle=0x%X)", reason, callback_param, msg_handle);

   if (msg_handle == IGNORE) return;
   switch (reason) {
      case VCHI_CALLBACK_MSG_AVAILABLE: break;
      case VCHI_CALLBACK_BULK_RECEIVED:
      case VCHI_CALLBACK_BULK_SENT:
         // push this item on queue. Will fetch it for processing from bufman_message_task
         status = os_semaphore_obtain(&vc_state.callback_queue_push_lock);
         os_assert( status == 0 );
         vchiu_queue_push(&vc_state.callback_queue, (void *)msg_handle);
         status = os_semaphore_release(&vc_state.callback_queue_push_lock);
         os_assert( status == 0 );
         status = os_eventgroup_signal( &state->bufman_message_available, 1 );
         os_assert( status == 0 );
         break;
      case VCHI_CALLBACK_SERVICE_CLOSED:
      case VCHI_CALLBACK_PEER_OFF:
         // have been closed so take note of that 
         vc_state.vchi_handle[0] = NULL;
         break;
   }
}



/******************************************************************************
NAME
   vc_bufmanx_get_default_pitch

SYNOPSIS
   int32_t vc_bufmanx_get_default_pitch( BUFMANX_IMAGE_T image )

FUNCTION
   Set a suitable default pitch for a BUFMANX_IMAGE_T with valid type and width

RETURNS
   -
******************************************************************************/
static int32_t vc_bufmanx_get_default_bpp(BUFMANX_IMAGE_T *src)
{
   int32_t bpp = 8;
   os_assert(src);
   switch (src->type) {
      case FRAME_HOST_IMAGE_EFormatYuv420P: bpp = 1*8; break;
      case FRAME_HOST_IMAGE_EFormatYuv422P:
      case FRAME_HOST_IMAGE_EFormatYuv422LE:
      case FRAME_HOST_IMAGE_EFormatRgb565: bpp = 2*8; break;
      case FRAME_HOST_IMAGE_EFormatRgb888: bpp = 3*8; break;
      case FRAME_HOST_IMAGE_EFormatRgbU32:
      case FRAME_HOST_IMAGE_EFormatRgbU32LE:
      case FRAME_HOST_IMAGE_EFormatRgbA32LE:
      case FRAME_HOST_IMAGE_EFormatRgbA32: bpp = 4*8; break;
      default: os_assert(0); break;
   }
   return bpp;
}

/******************************************************************************
NAME
   vc_bufmanx_get_default_pitch

SYNOPSIS
   int32_t vc_bufmanx_get_default_pitch( BUFMANX_IMAGE_T image )

FUNCTION
   Set a suitable default pitch for a BUFMANX_IMAGE_T with valid type and width

RETURNS
   -
******************************************************************************/
VCCPRE_ int32_t vc_bufmanx_get_default_pitch(BUFMANX_IMAGE_T *src)
{
   return ALIGN_UP(src->width, 1)*vc_bufmanx_get_default_bpp(src)>>3;
}

/******************************************************************************
NAME
   vc_bufmanx_get_default_size

SYNOPSIS
   int32_t vc_bufmanx_get_default_size(BUFMANX_IMAGE_T *src)

FUNCTION
   Gets the default memory requires for a bufman image with given dimenstions

RETURNS
   -
******************************************************************************/
VCCPRE_ int32_t vc_bufmanx_get_default_size(BUFMANX_IMAGE_T *src)
{
   int32_t pitch = 0;
   int32_t size = 0;
   int32_t height = 0;
   os_assert(src);
   pitch = src->pitch ? src->pitch : vc_bufmanx_get_default_pitch(src);
   height = ALIGN_UP(src->height, 1/*STRIPE_HEIGHT*/);
   switch (src->type) {
      case FRAME_HOST_IMAGE_EFormatYuv420P: size = (pitch * height*3)>>1; break;
      default: size = pitch * height; break;
   }
   os_assert(size);
   return size;
}

static void sema_callback_kick(void *cookie, void *cookie2, int32_t success)
{
   OS_SEMAPHORE_T *block = (OS_SEMAPHORE_T *)cookie;
   success = os_semaphore_release(block);
   os_assert( success == 0 );
}

static void *get_pixels(const BUFMANX_IMAGE_T *image, int x, int y)
{
   char *p;
   os_assert(image && image->pixels);
   os_assert(x==0);
   p = (char *)image->pixels;
   p += image->pitch * y;
   return p;
}

static void *get_pixels_u(const BUFMANX_IMAGE_T *image, int x, int y)
{
   int y_size  = image->pitch * image->height;
   int uv_size = (image->pitch>>1) * (image->height>>1);
   char *base_y = (char *)image->pixels;
   char *base_u = base_y + y_size;
   char *base_v = base_u + uv_size;

   os_assert(image && image->pixels);
   os_assert(x==0);
   return base_u + (image->pitch>>1) * (y>>1);
}

static void *get_pixels_v(const BUFMANX_IMAGE_T *image, int x, int y)
{
   int y_size  = image->pitch * image->height;
   int uv_size = (image->pitch>>1) * (image->height>>1);
   char *base_y = (char *)image->pixels;
   char *base_u = base_y + y_size;
   char *base_v = base_u + uv_size;

   os_assert(image && image->pixels);
   os_assert(x==0);
   return base_v + (image->pitch>>1) * (y>>1);
}


static void push_frame_complete(void *cookie, void *cookie2, int32_t success)
{
   BUFMANX_OBJECT_T *h = (BUFMANX_OBJECT_T *)cookie;
   os_logging_message("VCBUFMAN: sending push_frame_complete (stripe=%d/%d, handle=0x%X transform=%d)", h->current_stripe, h->total_stripes, h, h->transform);
   if (h->transform) {
      // finished with transpose buffer
      success = release_temp_buffer(&h->h_image);
      os_assert(success == 0);
   }
   if (h->next_callback)
      h->next_callback(h->next_cookie, h->next_cookie2, success);
   vc_bufmanx_service_release();
}


static void pull_frame_complete(void *cookie, void *cookie2, int32_t success)
{
   BUFMANX_OBJECT_T *h = (BUFMANX_OBJECT_T *)cookie;
   os_logging_message("VCBUFMAN: sending pull_frame_complete (stripe=%d/%d, handle=0x%X, transform=%d)", h->current_stripe, h->total_stripes, h, h->transform);
   success = vc_bufmanx_transform(&h->post_transpose_image, &h->h_image, h->transform);
   os_assert(success == 0);
   if (h->transform & BUFMAN_TRANSFORM_TRANSPOSE) {
      // finished with transpose buffer
      success = release_temp_buffer(&h->h_image);
      os_assert(success == 0);
   }
   if (h->next_callback)
      h->next_callback(h->next_cookie, h->next_cookie2, success);
   vc_bufmanx_service_release();
}


/******************************************************************************
NAME
   vc_bufmanx_pull

SYNOPSIS
   int32_t vc_bufmanx_pull_internal ( BUFMANX_IMAGE_T *dest, DISPMANX_RESOURCE_HANDLE_T *src, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, vc_bufman_callback_t callback, void *cookie, void *cookie2 );

FUNCTION
   Fetch an image from videocore referenced by a dispmanx resource handle, with optional format conversion, scaling and transform
RETURNS
   -
******************************************************************************/
static int32_t vc_bufmanx_pull_internal ( BUFMANX_OBJECT_T *h, BUFMANX_IMAGE_T *dst, const DISPMANX_RESOURCE_HANDLE_T src, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, vc_bufman_callback_t callback, void *cookie, void *cookie2 )
{
   int32_t success = 0, error;
   BUF_MSG_T *m = &h->msg;
   BUF_MSG_REMOTE_FUNCTION_FRAME_T *s = &m->u.frame;
   int dst_size;
   const int connection = 0;

   os_assert(sizeof(BUFMANX_OBJECT_T) <= sizeof(BUFMANX_HANDLE_T));
   os_assert(src_rect && dest_rect);

   error = os_semaphore_obtain(&vc_state.bufman_message_atomicity);
   os_assert( error == 0 );

   h->operation = OPERATION_PULL;
   h->v_image = src;
   h->h_image = *dst;
   h->callback = callback;
   h->cookie = cookie;
   h->cookie2 = cookie2;

   m->hdr.command = VC_BUFMAN_PULL_FRAME;
   s->resource_handle = src;
   s->src_rect = *src_rect;
   s->dest_rect = *dest_rect;
   s->type = dst->type;
   s->width = h->h_image.width;
   s->height= h->h_image.height;
   s->pitch = h->h_image.pitch;
   { // get size required for given rectangle of image
      BUFMANX_IMAGE_T temp;
      temp.type = dst->type;
      temp.pitch = h->h_image.pitch;
      temp.width = s->dest_rect.width;
      temp.height = s->dest_rect.height;
      dst_size = vc_bufmanx_get_default_size(&temp);
   }
   s->size = dst_size;
   os_assert(get_pixels(&h->h_image, s->dest_rect.x, s->dest_rect.y) >= dst->pixels && (uint8_t *)get_pixels(&h->h_image, s->dest_rect.x, s->dest_rect.y)+dst_size <= (uint8_t *)dst->pixels+dst->size);

   os_logging_message("VCBUFMAN: sending VC_BUFMAN_PULL_FRAME (size=%d, handle=0x%X, type=%d)", sizeof *m, h, s->type-FRAME_HOST_IMAGE_BASE);

   if (success == 0) {
      error = vchi_msg_queue(vc_state.vchi_handle[0],
                  m,
                  sizeof *m,
                  VCHI_FLAGS_BLOCK_UNTIL_QUEUED,
                  NULL);
      os_assert(error == 0);
   }

   if (success == 0) {
      success = vchi_get_response(NULL, 0);
      os_assert(success == 0);
   }

   if (success == 0) {
      #ifdef USE_VCHI
      const VCHI_FLAGS_T flags0 = VCHI_FLAGS_BLOCK_UNTIL_QUEUED, flags1 = (VCHI_FLAGS_T)(VCHI_FLAGS_BLOCK_UNTIL_QUEUED|VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE);
      #else
      const VCHI_FLAGS_T flags0 = VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE, flags1 = VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE;
      #endif
      // annoyance of planar YUV may have non-contiguous buffers
      if (s->type == FRAME_HOST_IMAGE_EFormatYuv420P) {
         error = vchi_bulk_queue_receive(vc_state.vchi_handle[connection],
                               get_pixels(&h->h_image, s->dest_rect.x, s->dest_rect.y), 2*s->size/3,
                               flags0, NULL);
         os_assert(error == 0);
         error = vchi_bulk_queue_receive(vc_state.vchi_handle[connection],
                               get_pixels_u(&h->h_image, s->dest_rect.x, s->dest_rect.y), s->size/6,
                               flags0, NULL);
         os_assert(error == 0);
         error = vchi_bulk_queue_receive(vc_state.vchi_handle[connection],
                               get_pixels_v(&h->h_image, s->dest_rect.x, s->dest_rect.y), s->size/6,
                               flags1, h);
         os_assert(error == 0);
      } else {
         success = vchi_bulk_queue_receive(vc_state.vchi_handle[connection], get_pixels(&h->h_image, s->dest_rect.x, s->dest_rect.y), dst_size,
                               flags1, h);
         os_assert(success == 0);
      }
      #ifndef USE_VCHI
      pull_striped_callback(h, NULL, success);
      #endif
   }
   error = os_semaphore_release(&vc_state.bufman_message_atomicity);
   os_assert( error == 0 );

   return success;
}

/******************************************************************************
NAME
   vc_bufmanx_push

SYNOPSIS
   int32_t vc_bufmanx_push ( BUFMANX_IMAGE_T *src, DISPMANX_RESOURCE_HANDLE_T *dst, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, BUFMAN_TRANSFORM_T transform, vc_bufman_callback_t callback, void *cookie, void *cookie2 );

FUNCTION
   Send an image to videocore referenced by a dispmanx resource handle, with optional format conversion, scaling and transform
RETURNS
   -
******************************************************************************/
static int32_t vc_bufmanx_push_internal ( BUFMANX_OBJECT_T *h, const BUFMANX_IMAGE_T *src, DISPMANX_RESOURCE_HANDLE_T dst, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, vc_bufman_callback_t callback, void *cookie, void *cookie2 )
{
   int32_t success = 0, error;
   BUF_MSG_T *m = &h->msg;
   BUF_MSG_REMOTE_FUNCTION_FRAME_T *s = &m->u.frame;
   const int connection=0;
   const int src_x=src_rect->x>>16, src_y=src_rect->y>>16, src_width = src_rect->width>>16, src_height=src_rect->height>>16;
   int stripe_size;

   os_assert(sizeof(BUFMANX_OBJECT_T) <= sizeof(BUFMANX_HANDLE_T));
   os_assert(src_rect && dest_rect);

   error = os_semaphore_obtain(&vc_state.bufman_message_atomicity);
   os_assert( error == 0 );

   h->operation = OPERATION_PUSH;
   h->h_image = *src;
   h->v_image = dst;
   h->callback = callback;
   h->cookie = cookie;
   h->cookie2 = cookie2;

   m->hdr.command = VC_BUFMAN_PUSH_FRAME;
   s->resource_handle = dst;
   s->src_rect = *src_rect;
   s->dest_rect = *dest_rect;
   s->type = src->type;
   s->width = h->h_image.width;
   s->height= h->h_image.height;
   s->pitch = h->h_image.pitch;
   { // get size required for given rectangle of image
      BUFMANX_IMAGE_T temp;
      temp.type = src->type;
      temp.pitch = h->h_image.pitch;
      temp.width = src_width;
      temp.height = src_height;
      stripe_size = vc_bufmanx_get_default_size(&temp);
   }
   s->size  = stripe_size;
   os_assert(get_pixels(&h->h_image, src_x, src_y) >= src->pixels && (uint8_t *)get_pixels(&h->h_image, src_x, src_y)+stripe_size <= (uint8_t *)src->pixels+src->size);

   os_logging_message("VCBUFMAN: sending VC_BUFMAN_PUSH_FRAME (size=%d, handle=0x%X, type=%d)", sizeof *m, h, s->type-FRAME_HOST_IMAGE_BASE);

   if (success == 0) {
      success = vchi_msg_queue(vc_state.vchi_handle[0],
                  m,
                  sizeof *m,
                  VCHI_FLAGS_BLOCK_UNTIL_QUEUED,
                  NULL);
      os_assert(success == 0);
   }

   if (success == 0) {
      success = vchi_get_response(NULL, 0);
      os_assert(success == 0);
   }
   if (success == 0) {
      // annoyance of planar YUV may have non-contiguous buffers
      if (src->type == FRAME_HOST_IMAGE_EFormatYuv420P) {
         error = vchi_bulk_queue_transmit(vc_state.vchi_handle[connection], get_pixels(&h->h_image, src_x, src_y), stripe_size*2/3, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
         os_assert(error == 0);
         error = vchi_bulk_queue_transmit(vc_state.vchi_handle[connection], get_pixels_u(&h->h_image, src_x, src_y), stripe_size/6, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
         os_assert(error == 0);
         error = vchi_bulk_queue_transmit(vc_state.vchi_handle[connection], get_pixels_v(&h->h_image, src_x, src_y), stripe_size/6, (VCHI_FLAGS_T)(VCHI_FLAGS_BLOCK_UNTIL_QUEUED|VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE), h);
         os_assert(error == 0);
      } else {
         error = vchi_bulk_queue_transmit(vc_state.vchi_handle[connection], get_pixels(&h->h_image, src_x, src_y), stripe_size, (VCHI_FLAGS_T)(VCHI_FLAGS_BLOCK_UNTIL_QUEUED|VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE), h);
         os_assert(error == 0);
      }
   }

   error = os_semaphore_release(&vc_state.bufman_message_atomicity);
   os_assert( error == 0 );

   return success;
}

static void pull_striped_multi(BUFMANX_OBJECT_T *h)
{
   BUFMANX_IMAGE_T *dst = &h->h_image;
   DISPMANX_RESOURCE_HANDLE_T src = h->v_image;
   VC_RECT_T *src_rect = NULL;
   VC_RECT_T *dest_rect = NULL;
   BUFMAN_TRANSFORM_T transform = h->transform;
   vc_bufman_callback_t callback = h->callback;
   void *cookie = h->cookie;
   void *cookie2 = h->cookie2;
   BUFMANX_IMAGE_T post_transpose_image, h_image;
   const VC_RECT_T default_dest_rect = {0<<16, 0<<16, dst->width, dst->height};
   const VC_RECT_T default_src_rect = {0, 0, ((transform & BUFMAN_TRANSFORM_TRANSPOSE)?dst->height:dst->width)<<16, ((transform & BUFMAN_TRANSFORM_TRANSPOSE)?dst->width:dst->height)<<16};
   int src_x=0, src_y=0;
   int32_t error;
   int stripe;
   BUF_MSG_T msg, *m = &msg;
   BUF_MSG_REMOTE_FUNCTION_FRAME_T *s = &m->u.frame;
   BUF_MSG_RESPONSE_T response;
   const int connection=0;
   os_assert(dst);

   h->success = 0;
   memset(m, 0, sizeof m);
   vc_bufmanx_service_use();

   s->src_rect = src_rect ? *src_rect:default_src_rect;
   s->dest_rect = dest_rect ? *dest_rect:default_dest_rect;

   CHECK_FIXED16_16_RECT(s->src_rect); CHECK_FIXED32_00_RECT(s->dest_rect);

   // need separate buffer for transpose
   h_image = *dst;
   if (transform & BUFMAN_TRANSFORM_TRANSPOSE) {
      int32_t success = 0;
      SWAP(s->dest_rect.width, s->dest_rect.height);
      SWAP(s->dest_rect.x, s->dest_rect.y);
      h_image.pixels = NULL;
      h_image.width = dst->height;
      h_image.height = dst->width;
      h_image.pitch = vc_bufmanx_get_default_pitch(&h_image);
      success = obtain_temp_buffer(&h_image);
      if (success != 0) {
         os_assert(0); // TODO: call callback here         
         return;
      }
   }
   post_transpose_image = *dst;

   os_logging_message("VCBUFMAN: sending pull_striped_multi (handle=0x%X, type=%d)", NULL, h_image.type-FRAME_HOST_IMAGE_BASE);

   // for multi transfer we keep the atomicity over whole frame, rather than just one stripe
   error = os_semaphore_obtain(&vc_state.bufman_message_atomicity);
   os_assert( error == 0 );

   m->hdr.command = VC_BUFMAN_PULL_MULTI;
   s->resource_handle = src;
   s->type =  h_image.type;
   s->width = h_image.width;
   s->height= h_image.height;
   s->pitch = h_image.pitch;

   // send message to videocore describing bufman job
   if (h->success == 0) {
      h->success = vchi_msg_queue(vc_state.vchi_handle[0], m, sizeof *m, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
      os_assert(h->success == 0);
   }

   // response fills in info from other side, like stripe height and number
   if (h->success == 0) {
      h->success = vchi_get_response(&response, 0);
      os_assert(h->success == 0);
   }

   os_logging_message("VCBUFMAN: pull_striped_multi (%d,%d,%d,%d,%d,%d)", s->type-FRAME_HOST_IMAGE_BASE, response.total_stripes, response.stripe_height, response.stripe_size, response.last_stripe_height, response.last_stripe_size);

   // sanity check
   os_assert(get_pixels(&h_image, src_x, src_y) >= h_image.pixels && (uint8_t *)get_pixels(&h_image, src_x, src_y)+response.stripe_size*(response.total_stripes-1)+response.last_stripe_size <= (uint8_t *)h_image.pixels+h_image.size);

   if (h->success == 0) {
      // send each of the stripes. No specific blocking until is needed here
      for (stripe = 0; stripe < response.total_stripes; stripe++) {
         const int last_stripe = stripe+1 == response.total_stripes;
         const int32_t stripe_height = last_stripe ? response.last_stripe_height : response.stripe_height;
         const int32_t stripe_size = last_stripe ? response.last_stripe_size : response.stripe_size;
         #ifdef USE_VCHI
         const VCHI_FLAGS_T flags0 = VCHI_FLAGS_BLOCK_UNTIL_QUEUED, flags1 = last_stripe ? VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE:VCHI_FLAGS_BLOCK_UNTIL_QUEUED;
         #else
         const VCHI_FLAGS_T flags0 = VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE, flags1 = VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE;
         #endif
         src_y = stripe * response.stripe_height;
   
         // annoyance of planar YUV may have non-contiguous buffers
         if (s->type == FRAME_HOST_IMAGE_EFormatYuv420P) {
            error = vchi_bulk_queue_receive(vc_state.vchi_handle[connection], get_pixels(&h_image, src_x, src_y), stripe_size*2/3, flags0, NULL);
            os_assert(error == 0);
            error = vchi_bulk_queue_receive(vc_state.vchi_handle[connection], get_pixels_u(&h_image, src_x, src_y), stripe_size/6, flags0, NULL);
            os_assert(error == 0);
            error = vchi_bulk_queue_receive(vc_state.vchi_handle[connection], get_pixels_v(&h_image, src_x, src_y), stripe_size/6, flags1, NULL);
            os_assert(error == 0);
         } else {
            error = vchi_bulk_queue_receive(vc_state.vchi_handle[connection], get_pixels(&h_image, src_x, src_y), stripe_size, flags1, NULL);
            os_assert(error == 0);
         }
      }
   }

   // this response guarantees the image data is valid on VC
   if (h->success == 0) {
      h->success = vchi_get_response(NULL, 0);
      os_assert(h->success == 0);
   }

   error = os_semaphore_release(&vc_state.bufman_message_atomicity);
   os_assert( error == 0 );

   if (h->success == 0) {
      // deal with transforms
      h->success = vc_bufmanx_transform(&post_transpose_image, &h_image, transform);
      os_assert(h->success == 0);
      if (transform & BUFMAN_TRANSFORM_TRANSPOSE) {
         // finished with transpose buffer
         error = release_temp_buffer(&h_image);
         os_assert(error == 0);
      }
      // frame done, call the callback
      if (callback)
         callback(cookie, cookie2, h->success);
   }
   vc_bufmanx_service_release();
}



static void pull_striped_callback(void *userdata, void *cookie2, int32_t success)
{
   BUFMANX_OBJECT_T *h = (BUFMANX_OBJECT_T *)userdata;
   BUFMANX_IMAGE_T *dst = &h->h_image;
   DISPMANX_RESOURCE_HANDLE_T src = h->v_image;
   VC_RECT_T my_src_rect, my_dest_rect;
   os_assert(dst);
   os_assert(h->stripe_height);
   os_assert(h->current_stripe+1 <= h->total_stripes);
   os_logging_message("VCBUFMAN: sending pull_striped_callback (stripe=%d/%d, handle=0x%X, type=%d)", h->current_stripe, h->total_stripes, h, dst->type-FRAME_HOST_IMAGE_BASE);

   my_dest_rect.x = 0, my_dest_rect.y = h->current_stripe*h->stripe_height, my_dest_rect.width = dst->width, my_dest_rect.height = OS_MIN(dst->height-my_dest_rect.y, h->stripe_height);
   my_src_rect.x = 0, my_src_rect.y = (h->current_stripe * h->stripe_height * (int64_t)h->src_rect.height)/h->dest_rect.height, my_src_rect.width = h->src_rect.width, my_src_rect.height = (my_dest_rect.height * (int64_t)h->src_rect.height)/h->dest_rect.height;

   h->current_stripe++;
   h->success = vc_bufmanx_pull_internal( h, dst, src, &my_src_rect, &my_dest_rect, h->current_stripe == h->total_stripes ? pull_frame_complete:pull_striped_callback, h, NULL );
   os_assert(h->success == 0);
}

#ifndef PULLMULTI
static int32_t vc_bufmanx_pull_striped_internal ( BUFMANX_OBJECT_T *h, BUFMANX_IMAGE_T *dst, const DISPMANX_RESOURCE_HANDLE_T src, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, BUFMAN_TRANSFORM_T transform, vc_bufman_callback_t callback, void *cookie, void *cookie2, int stripe_height )
{
   const VC_RECT_T default_dest_rect = {0<<16, 0<<16, dst->width, dst->height};
   const VC_RECT_T default_src_rect = {0, 0, ((transform & BUFMAN_TRANSFORM_TRANSPOSE)?dst->height:dst->width)<<16, ((transform & BUFMAN_TRANSFORM_TRANSPOSE)?dst->width:dst->height)<<16};

   os_assert(dst);

   memset(h, 0, sizeof *h);
   vc_bufmanx_service_use();
   h->stripe_height = stripe_height;
   h->src_rect = src_rect ? *src_rect:default_src_rect;
   h->dest_rect = dest_rect ? *dest_rect:default_dest_rect;

   CHECK_FIXED16_16_RECT(h->src_rect); CHECK_FIXED32_00_RECT(h->dest_rect);

   os_logging_message("VCBUFMAN: vc_bufmanx_pull_striped_internal (0x%X: stripe_height=%d, type=%d, transform=%d) (%d.%u,%d.%u,%d.%u,%d.%u)->(%d,%d,%d,%d)", h, stripe_height, dst->type-FRAME_HOST_IMAGE_BASE, transform,
      h->src_rect.x>>16, h->src_rect.x & 0xffff, h->src_rect.y>>16, h->src_rect.y & 0xffff, h->src_rect.width>>16, h->src_rect.width & 0xffff, h->src_rect.height>>16, h->src_rect.width & 0xffff,
      h->dest_rect.x, h->dest_rect.y, h->dest_rect.width, h->dest_rect.height);

   // need separate buffer for transpose
   if (transform & BUFMAN_TRANSFORM_TRANSPOSE) {
      int32_t success = 0;
      SWAP(h->dest_rect.width, h->dest_rect.height);
      SWAP(h->dest_rect.x, h->dest_rect.y);
      os_assert(h->h_image.pixels == NULL);
      h->post_transpose_image = *dst;
      h->v_image = src;
      h->h_image = *dst;
      h->h_image.pixels = NULL;
      h->h_image.width = dst->height;
      h->h_image.height = dst->width;
      h->h_image.pitch = vc_bufmanx_get_default_pitch(&h->h_image);
      success = obtain_temp_buffer(&h->h_image);
      if (success != 0) {
         os_assert(0);
         return success;
      }
      h->post_transpose_image = *dst;
   } else {
      h->v_image = src;
      h->h_image = *dst;
      h->post_transpose_image = *dst;
   }

   h->transform = transform;
   h->next_callback = callback;
   h->next_cookie = cookie;
   h->next_cookie2 = cookie2;
   h->total_stripes = (h->dest_rect.height + h->stripe_height-1)/h->stripe_height;
   h->current_stripe = 0;
   pull_striped_callback(h, NULL, h->success);

   return h->success;
}
#endif

/******************************************************************************
NAME
   vc_bufmanx_pull

SYNOPSIS
   int32_t vc_bufmanx_pull ( BUFMANX_IMAGE_T *dest, DISPMANX_RESOURCE_HANDLE_T *src, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, BUFMAN_TRANSFORM_T transform, vc_bufman_callback_t callback, void *cookie,  void *cookie2 )

FUNCTION
   Fetch an image from videocore referenced by a dispmanx resource handle, with optional format conversion, scaling and transform
RETURNS
   -
******************************************************************************/

VCCPRE_ void vc_bufmanx_pull_multi ( BUFMANX_HANDLE_T *xh, BUFMANX_IMAGE_T *dst, const DISPMANX_RESOURCE_HANDLE_T src, BUFMAN_TRANSFORM_T transform, vc_bufman_callback_t callback, void *cookie, void *cookie2 )
{
   int32_t error;
   BUFMANX_OBJECT_T *h = (BUFMANX_OBJECT_T *)xh;
   h->h_image = *dst;
   h->v_image = src;
   h->transform = transform;
   h->callback = callback;
   h->cookie = cookie;
   h->cookie2 = cookie2;
   h->operation = OPERATION_SUBMIT_PULL;
   error = os_semaphore_obtain(&vc_state.callback_queue_push_lock);
   os_assert( error == 0 );
   vchiu_queue_push(&vc_state.callback_queue, (void *)h);
   error = os_semaphore_release(&vc_state.callback_queue_push_lock);
   os_assert( error == 0 );
   error = os_eventgroup_signal( &vc_state.bufman_message_available, 1 );
   os_assert( error == 0 );
}

VCCPRE_ int32_t vc_bufmanx_pull ( BUFMANX_HANDLE_T *xh, BUFMANX_IMAGE_T *dst, const DISPMANX_RESOURCE_HANDLE_T src, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, BUFMAN_TRANSFORM_T transform, vc_bufman_callback_t callback, void *cookie, void *cookie2 )
{
#ifdef PULLMULTI
   BUFMANX_OBJECT_T *h = (BUFMANX_OBJECT_T *)xh;
   h->h_image = *dst;
   h->v_image = src;
   h->transform = transform;
   h->callback = callback;
   h->cookie = cookie;
   h->cookie2 = cookie2;
   pull_striped_multi ( h );
   return h->success;
#else
   return vc_bufmanx_pull_striped_internal ( (BUFMANX_OBJECT_T *)xh, dst, src, src_rect, dest_rect, transform, callback, cookie, cookie2, dst->height );
#endif
}

VCCPRE_ int32_t vc_bufmanx_pull_striped ( BUFMANX_HANDLE_T *xh, BUFMANX_IMAGE_T *dst, const DISPMANX_RESOURCE_HANDLE_T src, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, BUFMAN_TRANSFORM_T transform, vc_bufman_callback_t callback, void *cookie, void *cookie2 )
{
#ifdef PULLMULTI
   BUFMANX_OBJECT_T *h = (BUFMANX_OBJECT_T *)xh;
   h->h_image = *dst;
   h->v_image = src;
   h->transform = transform;
   h->callback = callback;
   h->cookie = cookie;
   h->cookie2 = cookie2;
   pull_striped_multi ( h );
   return h->success;
#else
   return vc_bufmanx_pull_striped_internal ( (BUFMANX_OBJECT_T *)xh, dst, src, src_rect, dest_rect, transform, callback, cookie, cookie2, STRIPE_HEIGHT );
#endif
}

static void push_striped_multi(BUFMANX_OBJECT_T *h)
{
   BUFMANX_IMAGE_T *src = &h->h_image;
   DISPMANX_RESOURCE_HANDLE_T dst = h->v_image;
   VC_RECT_T *src_rect = &h->src_rect;
   VC_RECT_T *dest_rect = &h->dest_rect;
   BUFMAN_TRANSFORM_T transform = h->transform;
   vc_bufman_callback_t callback = h->callback;
   void *cookie = h->cookie;
   void *cookie2 = h->cookie2;
   int32_t error;
   int stripe;
   BUF_MSG_T msg, *m = &msg;
   BUF_MSG_REMOTE_FUNCTION_FRAME_T *s = &m->u.frame;
   BUF_MSG_RESPONSE_T response;
   const int connection=0;
   const int src_x=src_rect->x>>16, src_width = src_rect->width>>16, src_height=src_rect->height>>16;
   int src_y=src_rect->y>>16;

   os_assert(src);
   os_assert(src_x == 0 && src_y == 0);
   
   h->success = 0;

   os_logging_message("VCBUFMAN: sending push_striped_multi (handle=0x%X, type=%d)", NULL, src->type-FRAME_HOST_IMAGE_BASE);

   // for multi transfer we keep the atomicity over whole frame, rather than just one stripe
   error = os_semaphore_obtain(&vc_state.bufman_message_atomicity);
   os_assert( error == 0 );

   m->hdr.command = VC_BUFMAN_PUSH_MULTI;
   s->resource_handle = dst;
   s->src_rect = *src_rect;
   s->dest_rect = *dest_rect;
   s->type = src->type;
   s->width = src->width;
   s->height= src->height;
   s->pitch = src->pitch;

   os_logging_message("VCBUFMAN: sending VC_BUFMAN_PUSH_MULTI (size=%d, handle=0x%X, type=%d)", sizeof *m, NULL, s->type-FRAME_HOST_IMAGE_BASE);

   // send message to videocore describing bufman job
   if (h->success == 0) {
      h->success = vchi_msg_queue(vc_state.vchi_handle[0], m, sizeof *m, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
      os_assert(h->success == 0);
   }

   // response fills in info from other side, like stripe height and number
   if (h->success == 0) {
      h->success = vchi_get_response(&response, 0);
      os_assert(h->success == 0);
   }

   os_logging_message("VCBUFMAN: push_striped_multi (%d,%d,%d,%d,%d,%d)", s->type-FRAME_HOST_IMAGE_BASE, response.total_stripes, response.stripe_height, response.stripe_size, response.last_stripe_height, response.last_stripe_size);

   // sanity check
   os_assert(get_pixels(src, src_x, src_y) >= src->pixels && (uint8_t *)get_pixels(src, src_x, src_y)+response.stripe_size*(response.total_stripes-1)+response.last_stripe_size <= (uint8_t *)src->pixels+src->size);

   if (h->success == 0) {
      // send each of the stripes. No specific blocking until is needed here
      for (stripe = 0; stripe < response.total_stripes; stripe++) {
         const int last_stripe = stripe+1 == response.total_stripes;
         const int32_t stripe_height = last_stripe ? response.last_stripe_height : response.stripe_height;
         const int32_t stripe_size = last_stripe ? response.last_stripe_size : response.stripe_size;
         const VCHI_FLAGS_T flags = VCHI_FLAGS_BLOCK_UNTIL_QUEUED;
         src_y = stripe * response.stripe_height;
   
         // annoyance of planar YUV may have non-contiguous buffers
         if (src->type == FRAME_HOST_IMAGE_EFormatYuv420P) {
            error = vchi_bulk_queue_transmit(vc_state.vchi_handle[connection], get_pixels(src, src_x, src_y), stripe_size*2/3, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
            os_assert(error == 0);
            error = vchi_bulk_queue_transmit(vc_state.vchi_handle[connection], get_pixels_u(src, src_x, src_y), stripe_size/6, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
            os_assert(error == 0);
            error = vchi_bulk_queue_transmit(vc_state.vchi_handle[connection], get_pixels_v(src, src_x, src_y), stripe_size/6, flags, NULL);
            os_assert(error == 0);
         } else {
            error = vchi_bulk_queue_transmit(vc_state.vchi_handle[connection], get_pixels(src, src_x, src_y), stripe_size, flags, NULL);
            os_assert(error == 0);
         }
      }
   }
   // this response guarantees the image data is valid on VC
   if (h->success == 0) {
      h->success = vchi_get_response(NULL, 0);
      os_assert(h->success == 0);
   }
   error = os_semaphore_release(&vc_state.bufman_message_atomicity);
   os_assert( error == 0 );

   if (transform) {
      // finished with transpose buffer
      error = release_temp_buffer(src);
      os_assert(error == 0);
   }

   // frame done, call the callback
   if (h->success == 0 && callback) {
      callback(cookie, cookie2, h->success);
   }
   vc_bufmanx_service_release();
}



static void push_striped_callback(void *userdata, void *cookie2, int32_t success)
{
   BUFMANX_OBJECT_T *h = (BUFMANX_OBJECT_T *)userdata;
   BUFMANX_IMAGE_T *src = &h->h_image;
   DISPMANX_RESOURCE_HANDLE_T dst = h->v_image;
   VC_RECT_T my_src_rect, my_dest_rect;
   os_assert(src);
   os_assert(h->stripe_height);
   os_assert(h->current_stripe+1 <= h->total_stripes);
   os_logging_message("VCBUFMAN: sending push_striped_callback (stripe=%d/%d, handle=0x%X, type=%d)", h->current_stripe, h->total_stripes, h, src->type-FRAME_HOST_IMAGE_BASE);

   my_src_rect.x = 0, my_src_rect.y = h->current_stripe*h->stripe_height<<16, my_src_rect.width = src->width<<16, my_src_rect.height = OS_MIN((src->height<<16)-my_src_rect.y, h->stripe_height<<16);
   my_dest_rect.x = 0, my_dest_rect.y = (h->current_stripe * h->stripe_height * (int64_t)(h->dest_rect.height<<16))/h->src_rect.height, my_dest_rect.width = h->dest_rect.width, my_dest_rect.height = (my_src_rect.height * (int64_t)h->dest_rect.height)/h->src_rect.height;

   h->current_stripe++;
   h->success = vc_bufmanx_push_internal( h, src, dst, &my_src_rect, &my_dest_rect, h->current_stripe == h->total_stripes ? push_frame_complete:push_striped_callback, h, NULL );
   os_assert(h->success == 0);
}

VCCPRE_ void vc_bufmanx_push_multi ( BUFMANX_HANDLE_T *xh, const BUFMANX_IMAGE_T *src, DISPMANX_RESOURCE_HANDLE_T dst, BUFMAN_TRANSFORM_T transform, vc_bufman_callback_t callback, void *cookie, void *cookie2 )
{
   BUFMANX_OBJECT_T *h = (BUFMANX_OBJECT_T *)xh;
   const VC_RECT_T default_src_rect = {0<<16, 0<<16, src->width<<16, src->height<<16};
   const VC_RECT_T default_dest_rect = {0, 0, (transform & BUFMAN_TRANSFORM_TRANSPOSE)?src->height:src->width, (transform & BUFMAN_TRANSFORM_TRANSPOSE)?src->width:src->height};
   int32_t error;

   os_assert(src);

   memset(h, 0, sizeof *h);
   vc_bufmanx_service_use();
   h->src_rect = default_src_rect;
   h->dest_rect = default_dest_rect;

   CHECK_FIXED16_16_RECT(h->src_rect); CHECK_FIXED32_00_RECT(h->dest_rect);

   os_logging_message("VCBUFMAN: vc_bufmanx_push_multi (0x%X: type=%d, transform=%d)", h, src->type-FRAME_HOST_IMAGE_BASE, transform);

   // need separate buffer for any transform on source
   if (transform) {
      int32_t success = 0;
      os_assert(h->h_image.pixels == NULL);
      h->v_image = dst;
      h->h_image = *src;
      h->h_image.pixels = NULL;
      if (transform & BUFMAN_TRANSFORM_TRANSPOSE) {
         SWAP(h->h_image.width, h->h_image.height);
         SWAP(h->src_rect.width, h->src_rect.height);
      }
      h->h_image.pitch = vc_bufmanx_get_default_pitch(&h->h_image);

      success = obtain_temp_buffer(&h->h_image);
      if (success != 0) {
         os_assert(0); // TODO: call callback here
         return;
      }
      // If doing only flips, these are done in place, so copy data to temp buffer
      if(!(transform & BUFMAN_TRANSFORM_TRANSPOSE))
      {
         memcpy(h->h_image.pixels, src->pixels, src->height * src->pitch);
      } // if

      // apply transform (flips and transpose)
      success = vc_bufmanx_transform(&h->h_image, src, transform);
      if (success != 0) {
         os_assert(0); // TODO: call callback here
         return;
      }
   } else {
      h->v_image = dst;
      h->h_image = *src;
   }

   h->transform = transform;
   h->callback = callback;
   h->cookie = cookie;
   h->cookie2 = cookie2;

   h->operation = OPERATION_SUBMIT_PUSH;
   error = os_semaphore_obtain(&vc_state.callback_queue_push_lock);
   os_assert( error == 0 );
   vchiu_queue_push(&vc_state.callback_queue, (void *)h);
   error = os_semaphore_release(&vc_state.callback_queue_push_lock);
   os_assert( error == 0 );
   error = os_eventgroup_signal( &vc_state.bufman_message_available, 1 );
   os_assert( error == 0 );
}

static int32_t vc_bufmanx_push_striped_internal ( BUFMANX_OBJECT_T *h, const BUFMANX_IMAGE_T *src, DISPMANX_RESOURCE_HANDLE_T dst, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, BUFMAN_TRANSFORM_T transform, vc_bufman_callback_t callback, void *cookie, void *cookie2, int stripe_height )
{
   const VC_RECT_T default_src_rect = {0<<16, 0<<16, src->width<<16, src->height<<16};
   const VC_RECT_T default_dest_rect = {0, 0, (transform & BUFMAN_TRANSFORM_TRANSPOSE)?src->height:src->width, (transform & BUFMAN_TRANSFORM_TRANSPOSE)?src->width:src->height};

   os_assert(src);

   memset(h, 0, sizeof *h);
   vc_bufmanx_service_use();
   h->stripe_height = stripe_height;
   h->src_rect = src_rect ? *src_rect:default_src_rect;
   h->dest_rect = dest_rect ? *dest_rect:default_dest_rect;

   CHECK_FIXED16_16_RECT(h->src_rect); CHECK_FIXED32_00_RECT(h->dest_rect);

   os_logging_message("VCBUFMAN: vc_bufmanx_push_striped_internal (0x%X: stripe_height=%d, type=%d, transform=%d) (%d.%u,%d.%u,%d.%u,%d.%u)->(%d,%d,%d,%d)", h, stripe_height, src->type-FRAME_HOST_IMAGE_BASE, transform,
      h->src_rect.x>>16, h->src_rect.x & 0xffff, h->src_rect.y>>16, h->src_rect.y & 0xffff, h->src_rect.width>>16, h->src_rect.width & 0xffff, h->src_rect.height>>16, h->src_rect.width & 0xffff,
      h->dest_rect.x, h->dest_rect.y, h->dest_rect.width, h->dest_rect.height);

   // need separate buffer for any transform on source
   if (transform) {
      int32_t success = 0;
      os_assert(h->h_image.pixels == NULL);
      h->v_image = dst;
      h->h_image = *src;
      h->h_image.pixels = NULL;
      if (transform & BUFMAN_TRANSFORM_TRANSPOSE) {
         SWAP(h->h_image.width, h->h_image.height);
         SWAP(h->src_rect.width, h->src_rect.height);
      }
      h->h_image.pitch = vc_bufmanx_get_default_pitch(&h->h_image);

      success = obtain_temp_buffer(&h->h_image);
      if (success != 0) {
         os_assert(0);
         return success;
      }

      // If doing only flips, these are done in place, so copy data to temp buffer
      if(!(transform & BUFMAN_TRANSFORM_TRANSPOSE))
      {
         memcpy(h->h_image.pixels, src->pixels, src->height * src->pitch);
      } // if

      // apply transform (flips and transpose)
      success = vc_bufmanx_transform(&h->h_image, src, transform);
      if (success != 0) {
         os_assert(0);
         return success;
      }
   } else {
      h->v_image = dst;
      h->h_image = *src;
   }
#ifdef PUSHMULTI
   h->transform = transform;
   h->callback = callback;
   h->cookie = cookie;
   h->cookie2 = cookie2;
   push_striped_multi ( h );
#else
   h->transform = transform;
   h->next_callback = callback;
   h->next_cookie = cookie;
   h->next_cookie2 = cookie2;
   h->total_stripes = ((h->src_rect.height>>16) + h->stripe_height-1)/h->stripe_height;
   h->current_stripe = 0;
   push_striped_callback(h, NULL, h->success);
#endif
   return h->success;
}

/******************************************************************************
NAME
   vc_bufmanx_push

SYNOPSIS
   int32_t vc_bufmanx_push ( BUFMANX_IMAGE_T *src, DISPMANX_RESOURCE_HANDLE_T *dst, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, BUFMAN_TRANSFORM_T transform, vc_bufman_callback_t callback, void *cookie, void *cookie2 );

FUNCTION
   Send an image to videocore referenced by a dispmanx resource handle, with optional format conversion, scaling and transform
RETURNS
   -
******************************************************************************/
VCCPRE_ int32_t vc_bufmanx_push_striped ( BUFMANX_HANDLE_T *xh, const BUFMANX_IMAGE_T *src, DISPMANX_RESOURCE_HANDLE_T dst, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, BUFMAN_TRANSFORM_T transform, vc_bufman_callback_t callback, void *cookie, void *cookie2 )
{
   return vc_bufmanx_push_striped_internal((BUFMANX_OBJECT_T *)xh, src, dst, src_rect, dest_rect, transform, callback, cookie, cookie2, STRIPE_HEIGHT );
}

VCCPRE_ int32_t vc_bufmanx_push ( BUFMANX_HANDLE_T *xh, const BUFMANX_IMAGE_T *src, DISPMANX_RESOURCE_HANDLE_T dst, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, BUFMAN_TRANSFORM_T transform, vc_bufman_callback_t callback, void *cookie, void *cookie2 )
{
   return vc_bufmanx_push_striped_internal ((BUFMANX_OBJECT_T *)xh, src, dst, src_rect, dest_rect, transform, callback, cookie, cookie2, (transform & BUFMAN_TRANSFORM_TRANSPOSE) ? src->width:src->height );
}

VCCPRE_ int32_t vc_bufmanx_pull_blocking ( BUFMANX_HANDLE_T *xh, BUFMANX_IMAGE_T *dst, const DISPMANX_RESOURCE_HANDLE_T src, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, BUFMAN_TRANSFORM_T transform )
{
   OS_SEMAPHORE_T block;
   int32_t s, success = -1;

   s = os_semaphore_create( &block, OS_SEMAPHORE_TYPE_SUSPEND );
   os_assert( s == 0 );
   s  = os_semaphore_obtain(&block);
   os_assert( s == 0 );

   success = vc_bufmanx_pull( xh, dst, src, src_rect, dest_rect, transform, sema_callback_kick, &block, NULL );
   if (success == 0) {
      s = os_semaphore_obtain(&block);
      os_assert( s == 0 );
   } else os_assert(0);
   s = os_semaphore_destroy(&block);
   os_assert( s == 0 );
   return success;
}


VCCPRE_ int32_t vc_bufmanx_pull_striped_blocking ( BUFMANX_HANDLE_T *xh, BUFMANX_IMAGE_T *dst, const DISPMANX_RESOURCE_HANDLE_T src, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, BUFMAN_TRANSFORM_T transform)
{
   OS_SEMAPHORE_T block;
   int32_t s, success = -1;

   s = os_semaphore_create( &block, OS_SEMAPHORE_TYPE_SUSPEND );
   os_assert( s == 0 );
   s  = os_semaphore_obtain(&block);
   os_assert( s == 0 );

   success = vc_bufmanx_pull_striped( xh, dst, src, src_rect, dest_rect, transform, sema_callback_kick, &block, NULL );
   if (success == 0) {
      s = os_semaphore_obtain(&block);
      os_assert( s == 0 );
   } else os_assert(0);
   s = os_semaphore_destroy(&block);
   os_assert( s == 0 );
   return success;
}

VCCPRE_ int32_t vc_bufmanx_push_blocking ( BUFMANX_HANDLE_T *xh, const BUFMANX_IMAGE_T *src, DISPMANX_RESOURCE_HANDLE_T dst, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, BUFMAN_TRANSFORM_T transform )
{
   OS_SEMAPHORE_T block;
   int32_t s, success = -1;

   s = os_semaphore_create( &block, OS_SEMAPHORE_TYPE_SUSPEND );
   os_assert( s == 0 );
   s = os_semaphore_obtain(&block);
   os_assert( s == 0 );

   success = vc_bufmanx_push( xh, src, dst, src_rect, dest_rect, transform, sema_callback_kick, &block, NULL );
   os_assert( success == 0 );

   if (success == 0) {
      s = os_semaphore_obtain(&block);
      os_assert( s == 0 );
   }
   s = os_semaphore_destroy(&block);
   os_assert( s == 0 );
   return success;
}

VCCPRE_ int32_t vc_bufmanx_push_striped_blocking ( BUFMANX_HANDLE_T *xh, const BUFMANX_IMAGE_T *src, DISPMANX_RESOURCE_HANDLE_T dst, const VC_RECT_T *src_rect, const VC_RECT_T *dest_rect, BUFMAN_TRANSFORM_T transform )
{
   OS_SEMAPHORE_T block;
   int32_t s, success = -1;

   s = os_semaphore_create( &block, OS_SEMAPHORE_TYPE_SUSPEND );
   os_assert( s == 0 );
   s  = os_semaphore_obtain(&block);
   os_assert( s == 0 );

   success = vc_bufmanx_push_striped( xh, src, dst, src_rect, dest_rect, transform, sema_callback_kick, &block, NULL );
   if (success == 0) {
      s = os_semaphore_obtain(&block);
      os_assert( s == 0 );
   } else os_assert(0);
   s = os_semaphore_destroy(&block);
   os_assert( s == 0 );
   return success;
}

VCCPRE_ int32_t vc_bufmanx_push_multi_blocking ( BUFMANX_HANDLE_T *xh, const BUFMANX_IMAGE_T *src, DISPMANX_RESOURCE_HANDLE_T dst, BUFMAN_TRANSFORM_T transform )
{
   OS_SEMAPHORE_T block;
   int32_t s, success = 0;

   s = os_semaphore_create( &block, OS_SEMAPHORE_TYPE_SUSPEND );
   os_assert( s == 0 );
   s  = os_semaphore_obtain(&block);
   os_assert( s == 0 );

   vc_bufmanx_push_multi( xh, src, dst, transform, sema_callback_kick, &block, NULL );
   if (success == 0) {
      s = os_semaphore_obtain(&block);
      os_assert( s == 0 );
   } else os_assert(0);
   s = os_semaphore_destroy(&block);
   os_assert( s == 0 );
   return ((BUFMANX_OBJECT_T *)xh)->success;
}

VCCPRE_ int32_t vc_bufmanx_pull_multi_blocking ( BUFMANX_HANDLE_T *xh, BUFMANX_IMAGE_T *dst, const DISPMANX_RESOURCE_HANDLE_T src, BUFMAN_TRANSFORM_T transform )
{
   OS_SEMAPHORE_T block;
   int32_t s, success = 0;

   s = os_semaphore_create( &block, OS_SEMAPHORE_TYPE_SUSPEND );
   os_assert( s == 0 );
   s  = os_semaphore_obtain(&block);
   os_assert( s == 0 );

   vc_bufmanx_pull_multi( xh, dst, src, transform, sema_callback_kick, &block, NULL );
   if (success == 0) {
      s = os_semaphore_obtain(&block);
      os_assert( s == 0 );
   } else os_assert(0);
   s = os_semaphore_destroy(&block);
   os_assert( s == 0 );
   return ((BUFMANX_OBJECT_T *)xh)->success;
}

/******************************************************************************
NAME
   vc_bufmanx_allocate_image

SYNOPSIS
   int32_t vc_bufmanx_allocate_image(BUFMANX_IMAGE_T *image)

FUNCTION
   Allocate memory for an image, using type, width and height field in image parameter
RETURNS
   -
******************************************************************************/
int32_t vc_bufmanx_allocate_image(BUFMANX_IMAGE_T *image)
{
   int32_t success = 0;
   const uint32_t align = 16;

   if (image->pitch == 0)
      image->pitch = vc_bufmanx_get_default_pitch(image);
   if (image->size == 0)
      image->size = vc_bufmanx_get_default_size(image);
   os_assert(image->pixels == NULL);
   image->pixels = os_malloc( image->size, align, "vc_bufmanx_allocate_image" );
   os_assert(image->pixels != NULL);

   return success;
}

/******************************************************************************
NAME
   vc_bufmanx_release_image

SYNOPSIS
   int32_t vc_bufmanx_release_image(BUFMANX_IMAGE_T *image)

FUNCTION
   Free memory for an allocated image
RETURNS
   -
******************************************************************************/
int32_t vc_bufmanx_release_image(BUFMANX_IMAGE_T *image)
{
   int32_t success = 0;

   os_assert(image->pixels );
   os_free( image->pixels );
   memset(image, 0, sizeof *image);

   return success;
}

#ifdef __SYMBIAN32__
} // namespace BufManX
#endif

/******************************************************************************
NAME
   vc_bufmanx_allocate_buffers

SYNOPSIS

FUNCTION
   Allocate the specified number and type of buffers on the server side, for use with streams
RETURNS
   0 if message has been queued successfully
******************************************************************************/

VCHPRE_ int32_t VCHPOST_ vc_bufmanx_allocate_buffers
   (uint32_t stream, uint32_t num_of_buffers,
      buf_frame_type_t type, uint32_t width, uint32_t height)
{
   BUF_MSG_T msg;
   uint32_t failure = 0;

   vc_bufmanx_service_use();

   memset((void *) &msg, 0, sizeof(msg));
   msg.hdr.command = VC_BUFMAN_ALLOC_BUF;
   msg.u.alloc_buf_frame.stream = stream;
   msg.u.alloc_buf_frame.num_of_buffers = num_of_buffers;
   msg.u.alloc_buf_frame.type = type;
   msg.u.alloc_buf_frame.width = width;
   msg.u.alloc_buf_frame.height = height;

   failure = vchi_msg_queue(vc_state.vchi_handle[0],
      &msg, sizeof msg, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
   os_assert(failure == 0);

   return failure;
} // vc_bufmanx_allocate_buffers()

/******************************************************************************
NAME
   vc_bufmanx_free_buffers

SYNOPSIS

FUNCTION
   Free buffers on the server which are associated with the specified stream.
   0 = VC_BUFMANX_FREE_BUFFERS_ALL = free all buffers
RETURNS
   0 if message has been queued successfully
******************************************************************************/

VCHPRE_ int32_t VCHPOST_ vc_bufmanx_free_buffers(uint32_t stream, uint32_t num_of_buffers)
{
   BUF_MSG_T msg;
   uint32_t failure = 0;

   memset((void *) &msg, 0, sizeof(msg));
   msg.hdr.command = VC_BUFMAN_FREE_BUF;
   msg.u.free_buf_frame.stream = stream;
   msg.u.free_buf_frame.num_of_buffers = num_of_buffers;

   failure = vchi_msg_queue(vc_state.vchi_handle[0],
      &msg, sizeof msg, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
   os_assert(failure == 0);

   return failure;

} // vc_bufmanx_free_buffers()

/******************************************************************************
NAME
   vc_bufmanx_get_vc_image_type

SYNOPSIS

FUNCTION
   Returns VC image type corresponding to given bufman type. Based on
   find_vc_image_type() in bufman_convert.c.
RETURNS
   VC image type corresponding to bufman type, or VC_IMAGE_MIN if this isn't
   possible.
******************************************************************************/

VC_IMAGE_TYPE_T vc_bufmanx_get_vc_image_type(buf_frame_type_t bm_type)
{
   VC_IMAGE_TYPE_T vc_type = VC_IMAGE_MIN;
   switch (bm_type) {
      case FRAME_HOST_IMAGE_EFormatYuv420P: vc_type = VC_IMAGE_YUV420; break;
      case FRAME_HOST_IMAGE_EFormatYuv422P: break;
      case FRAME_HOST_IMAGE_EFormatYuv422LE: break;
      case FRAME_HOST_IMAGE_EFormatRgb565: vc_type = VC_IMAGE_RGB565; break;
      case FRAME_HOST_IMAGE_EFormatRgb888: vc_type = VC_IMAGE_RGB888; break;
      case FRAME_HOST_IMAGE_EFormatRgbU32: vc_type = VC_IMAGE_RGBX32; break;
      case FRAME_HOST_IMAGE_EFormatRgbU32LE: vc_type = VC_IMAGE_XRGB8888; break;
      case FRAME_HOST_IMAGE_EFormatRgbA32: vc_type = VC_IMAGE_RGBA32; break;
      case FRAME_HOST_IMAGE_EFormatRgbA32LE: vc_type = VC_IMAGE_ARGB8888; break;
      default: os_assert(0); break;
   }
   return vc_type;
}
