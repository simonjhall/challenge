#if 0

/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
//#define LOG_NDEBUG 0
#define LOG_TAG	"BRCM_EGL"
//#include <cutils/log.h>

//#include <ui/egl/android_natives.h>
//#include <ui/android_native_buffer.h>
//#include <cutils/native_handle.h>
//#include <cutils/properties.h>

//#include "gralloc_priv.h"
#include "EGL/eglplatform.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GLES2/gl2.h"
#include "GLES2/gl2ext.h"

#include "brcm_egl.h"
#include "interface/khronos/common/khrn_int_image.h"
#include "interface/khronos/include/EGL/eglext_brcm.h"
//#include "opensrc/interface/khronos/include/EGL/eglext_android.h"

static void set_current(struct context *context);
static void attach_current_context(EGLDisplay display, struct context *current);
static bool surface_dequeue_buffer(struct surface *surface);
static void surface_enqueue_buffer(struct surface *surface);
static void attach_buffer(EGLDisplay display, struct surface *surface);
static bool brcm_egl_convert_anative_buf_to_khrn_image( android_native_buffer_t *abuffer, KHRN_IMAGE_WRAP_T *khr_image ,bool isTiled);

EGLBoolean eglCopyBuffers_Int(EGLDisplay display, EGLSurface surface_, EGLNativePixmapType target);
EGLBoolean eglSwapBuffers_Int(EGLDisplay display, EGLSurface surface_);
EGLBoolean eglQuerySurface_Int(EGLDisplay display, EGLSurface surface_, EGLint attribute, EGLint *value);
EGLBoolean eglDestroySurface_Int(EGLDisplay display, EGLSurface surface_);
EGLSurface eglCreatePixmapSurface_Int(EGLDisplay display, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list);
EGLSurface eglCreatePbufferSurface_Int(EGLDisplay display, EGLConfig config, const EGLint *attrib_list);
EGLSurface eglCreateWindowSurface_Int(EGLDisplay display, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list);
EGLBoolean eglTerminate_Int(EGLDisplay display);
EGLBoolean eglInitialize_Int(EGLDisplay display, EGLint *major, EGLint *minor);
EGLSurface eglGetCurrentSurface_Int(EGLint readdraw);
EGLContext eglGetCurrentContext_Int(void);
EGLBoolean eglMakeCurrent_Int(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context);
EGLBoolean eglDestroyContext_Int(EGLDisplay display, EGLContext context_);
EGLContext eglCreateContext_Int(EGLDisplay display, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
EGLBoolean eglBindTexImage_Int(EGLDisplay display, EGLSurface surface, EGLint buffer);
EGLBoolean eglReleaseTexImage_Int(EGLDisplay display, EGLSurface surface, EGLint buffer);
EGLBoolean eglSurfaceAttrib_Int(EGLDisplay display, EGLSurface surface, EGLint attribute, EGLint value);
EGLImageKHR eglCreateImageKHR_Int(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attr_list);

#define IS_ANDROID_WINDOW(w) ((w) && (w)->common.magic == ANDROID_NATIVE_WINDOW_MAGIC)

/* Notes:

(a) Buffer preservation has not been thought about for the ->window gralloc buffers

(b) From above, glFinish should really give hints as to the de/se buffer behaviour and also invalidating the colour buffer after its been called

(c) Todo - set eglError correctly

*/

//
// Internal EGL functions
//
#ifndef BRCM_V3D_OPT
void eglIntDestroyByPid_impl(uint32_t pid_0, uint32_t pid_1)
{
	LOGE("Do NOT Reach eglIntDestroyByPid_impl");
}
#endif

EGLContext eglCreateContext(EGLDisplay display, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
    EGLContext context_int = eglCreateContext_Int(display, config, share_context, attrib_list);
    if(context_int == EGL_NO_CONTEXT)
    {
        LOGE("eglCreateContext - FAILED");
        return EGL_NO_CONTEXT;
    }
    else
    {
        struct context *context = (struct context *)malloc(sizeof(struct context));
        int i;
        LOGD("eglCreateContext() context: %p, VC context %d, Thread %d", context, (int)context_int, gettid());
        context->context = context_int;
        context->read = 0;
        context->draw = 0;
        context->made_current = 0;
        context->profiling = 0;
		context->composer = init_composer();

        //TODO - don't believe we need to do this
        //brcm_gl_context_init(context, attrib_list);
        return (EGLContext)(context);
    }
}

//
// external EGL API functions
//

EGLBoolean eglDestroyContext(EGLDisplay display, EGLContext context_)
{
    if(context_ == EGL_NO_CONTEXT)
    {
        return EGL_FALSE;
        LOGE("eglDestroyContext - FAILED");
    }
    else
    {
      struct context *context = (struct context *)(context_);

      //TODO - should we not release the window / buffers here?
      if( !context->made_current )
      {
         //clean up the buffers and release them if needed
         //TODO - ask neils about the defRef() for the window handle
      }
      else
      {
         //the context is still current according to the spec - do nothing
         //TODO - deal with this case
         LOGD("eglDestroyContext() FATAL error - the context is still active but we called destroy - FIX THIS");
      }

       //Also, don't we need to check if the context is active or not?
      LOGD("eglDestroyContext() context: %p, VC context: %d, Thread %d", context, (int)context->context, gettid());
      EGLBoolean ret = eglDestroyContext_Int(display, context->context);
      release_composer(context->composer);
	  context->composer = NULL;
	  free(context);
      return ret;
    }
}

EGLBoolean eglMakeCurrent(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context)
{
    struct context *next_context = (struct context *)context;
    struct surface *next_draw_surface = (struct surface *)draw;
    struct surface *next_read_surface = (struct surface *)read;
    struct context *current = brcm_egl_get_current();

   LOGV("eglMakeCurrent - 1");

    //Are we getting rid of the context?
    if(draw == EGL_NO_SURFACE && read == EGL_NO_SURFACE && context == EGL_NO_CONTEXT)
    {
       LOGV("eglMakeCurrent - 2");

        //if there is a context current, get rid of it
        set_current(NULL);

        //call the V3D function
        eglMakeCurrent_Int(display, draw, read, context);

        current->made_current = 0;
        LOGD("eglMakeCurrent(NULL) Thread: %d", gettid());
        return EGL_TRUE;
    }
    //is the context being setup? check all fields
    else if(draw != EGL_NO_SURFACE && read != EGL_NO_SURFACE && context != EGL_NO_CONTEXT)
    {
        //make the call to v3d
        EGLBoolean ret = eglMakeCurrent_Int(display, next_draw_surface->surface, next_read_surface->surface, next_context->context);

        //This should not fail - maybe the context was deleted however by memory management
        if(ret == EGL_FALSE)
        {
            /*CLIENT_THREAD_STATE_T *thread = platform_get_thread_state();

            if(thread->error == EGL_CONTEXT_LOST)
            {
                LOGE("eglMakeCurrent(%p, %p, %p) error: EGL_CONTEXT_LOST, Thread: %d", context, draw, read, gettid());
            }
            else
            {
                LOGE("eglMakeCurrent(%p, %p, %p) error: 0x%x, Thread: %d", context, draw, read, thread->error, gettid());
            }*/
        }
        else
        {
            LOGD("eglMakeCurrent(%p, %p, %p) Thread: %d", context, draw, read, gettid());
        }

        //TODO - we ignore the context error code and go ahead with the following anyway

        //set the current context up
        set_current(next_context);
        current = next_context;
        current->read = next_read_surface;
        current->draw = next_draw_surface;

        LOGV("eglMakeCurrent - 2");

        //connect a current context
        attach_current_context(display, current);

		glFlush();
        return ret;
    }
    else
    {
        return EGL_FALSE;
    }
}

EGLContext eglGetCurrentContext(void)
{
    struct context *current_context = brcm_egl_get_current();
    return current_context;
}

EGLSurface eglGetCurrentSurface(EGLint readdraw)
{
    struct context *current_context = brcm_egl_get_current();

    if(!current_context)
    {
        return EGL_NO_SURFACE;
    }

    if(readdraw == EGL_READ)
    {
        return (EGLSurface)current_context->read;
    }

    if(readdraw == EGL_DRAW)
    {
        return (EGLSurface)current_context->draw;
    }
    else
    {
        return EGL_NO_SURFACE;
    }
}

EGLBoolean eglInitialize(EGLDisplay display, EGLint *major, EGLint *minor)
{
    EGLBoolean res;
    res = eglInitialize_Int(display, major, minor);
    return res;
}

EGLBoolean eglTerminate(EGLDisplay display)
{
    EGLBoolean res;
    res = eglTerminate_Int(display);
    return res;
}

EGLSurface eglCreateWindowSurface(  EGLDisplay display,
                                    EGLConfig config,
				                        EGLNativeWindowType win,
				                        const EGLint *attrib_list)
{
   EGLSurface surface = EGL_NO_SURFACE;
   android_native_window_t *window = (android_native_window_t*)win;

   int formatt;
   window->query(window, NATIVE_WINDOW_FORMAT, &formatt);
   LOGV("%s: format = %d",__FUNCTION__,formatt);
   if(formatt == 0)
   {
	   window->perform(window, NATIVE_WINDOW_SET_BUFFERS_FORMAT, HAL_PIXEL_FORMAT_YCbCr_422_I);
   }

   LOGV("eglCreateWindowSurface - 1");

   if(IS_ANDROID_WINDOW(window))
   {
      int width, height;
      EGLSurface surface_int;
      struct surface *surface_ptr = (struct surface *)malloc(sizeof(struct surface));
	  int value = 0;
	  eglGetConfigAttrib(display,config,EGL_SAMPLE_BUFFERS,&value);
	  if(value)
	  	surface_ptr->isTiled = 0;
	  else
	  	surface_ptr->isTiled = 1;

	  	surface_ptr->isTiled = 0;

      LOGV("eglCreateWindowSurface - 2");

      window->query(window, NATIVE_WINDOW_WIDTH, &width);
      window->query(window, NATIVE_WINDOW_HEIGHT, &height);

      //Don't care about EGLNativeWindowType
      surface_int = eglCreateWindowSurface_Int(display, config, (EGLNativeWindowType)((height << 16) | (width & 0xFFFF)), attrib_list);

      //the above function will create a single buffered 'container' which currently not populated with a buffer pointer at creation
      if(surface_int != EGL_NO_SURFACE)
      {
         surface_ptr->surface = surface_int;
         surface_ptr->window = window;
         surface_ptr->buffer = 0;

         //send back the surface
         surface = (EGLSurface)surface_ptr;

         LOGD("eglCreateWindowSurface() surface: %p, VC surface: %d, Thread: %d", surface, (int)surface_int, gettid());
      }
      else
      {
         LOGE("eglCreateWindowSurface - 3 - FAILED");
         free(surface_ptr);
      }
   }
   else
      LOGD("eglCreateWindowSurface() surface: %p, Thread: %d", surface, gettid());

   return surface;
}

EGLSurface eglCreatePbufferSurface(EGLDisplay display, EGLConfig config,
				                        const EGLint *attrib_list)
{
    EGLSurface surface_int = eglCreatePbufferSurface_Int(display, config, attrib_list);

    if(surface_int != EGL_NO_SURFACE)
    {
        struct surface *surface = (struct surface *)malloc(sizeof(struct surface));
        surface->surface = surface_int;
        surface->window = 0;
        return (EGLSurface)surface;
    }
    else
    {
        return EGL_NO_SURFACE;
    }
}

EGLSurface eglCreatePixmapSurface(  EGLDisplay display, EGLConfig config,
				                       EGLNativePixmapType pixmap,
				                       const EGLint *attrib_list)
{
    EGLSurface surface_int = eglCreatePixmapSurface_Int(display, config, pixmap, attrib_list);

    if(surface_int != EGL_NO_SURFACE)
    {
        struct surface *surface = (struct surface *)malloc(sizeof(struct surface));
        surface->surface = surface_int;
        surface->window = 0;
        return (EGLSurface)surface;
    }
    else
    {
        return EGL_NO_SURFACE;
    }
}

EGLBoolean eglDestroySurface(EGLDisplay display, EGLSurface surface_)
{
    if(surface_ == EGL_NO_SURFACE)
    {
       LOGE("eglDestroySurface - 1 - FAILED");
        return EGL_FALSE;
    }
    else
    {
        struct surface *surface = (struct surface *)surface_;
        EGLBoolean res = EGL_FALSE;

        LOGV("eglDestroySurface - 2" );

        if(surface->window)
        {
            LOGD("eglDestroySurface() surface: %p, android window %p, Thread: %d", surface, surface->window, gettid());
            android_native_window_t *window = surface->window;
            android_native_buffer_t *buffer = surface->buffer;

            //eglDestroySurface in Android case is always sync

            if(buffer)
            {
               LOGV("eglDestroySurface - 3" );

                struct context *current = brcm_egl_get_current();
                if(current && current->draw == surface)
                {
                    glFinish();
                }

                res = eglDestroySurface_Int(display, surface->surface);

                //send the buffer back!
                //TODO - do we really want to send the buffer back?
                surface_enqueue_buffer( surface);
            }
            else
            {
               res = eglDestroySurface_Int(display, surface->surface);
            }

            LOGV("eglDestroySurface - 4" );

            //in concept, we no longer have a reference to the window surface
            surface->window = 0;
        }
        else
        {
            res = eglDestroySurface_Int(display, surface->surface);
        }

        surface->made_current = 0;
        free(surface);
        return res;
    }
}

EGLBoolean eglQuerySurface(EGLDisplay display, EGLSurface surface_,
			                  EGLint attribute, EGLint *value)
{
    struct surface *surface = (struct surface *)surface_;
    if(!surface)
    {
        return EGL_FALSE;
    }
    else if(surface->window) {
        switch(attribute)
        {
        case EGL_CONFIG_ID:
//            *value = (EGLint)surface->config;
            break;
        case EGL_WIDTH:
            surface->window->query(surface->window, NATIVE_WINDOW_WIDTH, value);
            break;
        case EGL_HEIGHT:
            surface->window->query(surface->window, NATIVE_WINDOW_HEIGHT, value);
            break;
        case EGL_LARGEST_PBUFFER:
            break;
        case EGL_TEXTURE_FORMAT:
            *value = EGL_NO_TEXTURE;
            break;
        case EGL_TEXTURE_TARGET:
            *value = EGL_NO_TEXTURE;
            break;
        case EGL_SWAP_BEHAVIOR:
            *value = EGL_BUFFER_DESTROYED;
            break;
        case EGL_MIPMAP_LEVEL:
            *value = 0;
            break;
        default:
            return EGL_FALSE;
        }
        return EGL_TRUE;
    }
    else
    {
        return eglQuerySurface_Int(display, surface->surface, attribute, value);
    }
}

EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface_)
{
    struct context *current_context = brcm_egl_get_current();
    struct surface *surface = (struct surface *)surface_;

    if( !current_context )
    {
       LOGE("eglSwapBuffers() - TODO - function called but no current context is valid");
	   return false;
    }

    //nothing bound?
    if(surface_ == EGL_NO_SURFACE)
    {
        LOGE("eglSwapBuffers(%p) error: EGL_BAD_SURFACE Thread: %d", current_context, gettid());
        return EGL_FALSE;
    }

    //do we have an android native window already assigned to our egl context?
    if(surface->window)
    {

        //check that the context wasn't deleted
        //TODO - explain why this could occur
#if 0
        if(egl_is_context_lost(thread))
        {
            thread->error = EGL_CONTEXT_LOST;
            LOGE("eglSwapBuffers(%p) error: EGL_CONTEXT_LOST, Thread: %d", current_context, gettid());
            return EGL_FALSE;
        }
#endif

        LOGD_IF(current_context->profiling, "eglSwapBuffers(%d, %p) %d", (int)display, (void *)surface, gettid());

        //todo - add in invalidates here
#ifndef BRCM_V3D_OPT
        glFinish();
#else
        glFinish();
        glFlush();
		if(surface->buffer->format == HAL_PIXEL_FORMAT_YCbCr_422_I)
			convert_to_yuv(current_context->composer,surface->buffer);
#endif
        //TODO - if we don't get a buffer below, then v3d will have a pointer to memory that it can't write to anymore

		EGLint res = glGetError();
		if (res == GL_OUT_OF_MEMORY)
			{
			LOGE("eglSwapBuffers Error 0x%x",res);
			return false;
			}
        //send the current buffer out
        surface_enqueue_buffer( surface);

		sync_composer(current_context->composer);

        //grab a new buffer and bind it to V3D
        if( !surface_dequeue_buffer(surface) )
           LOGE("surface_dequeue_buffer FATAL error - no buffer to attach");

         attach_buffer(display, surface);

        return EGL_TRUE;
    }
    else
    {
       LOGE("eglSwapBuffers called but no window bound!" );
    }
    return EGL_FALSE;
}

EGLBoolean eglCopyBuffers(EGLDisplay display, EGLSurface surface_, EGLNativePixmapType target)
{
    if(surface_ == EGL_NO_SURFACE)
    {
        return EGL_FALSE;
    }
    else
    {
        struct surface *surface = (struct surface *)surface_;

        //v3d will "do the right thing" here
        return eglCopyBuffers_Int(display, surface->surface, target);
    }
}


EGLBoolean eglBindTexImage(EGLDisplay display, EGLSurface surface_, EGLint buffer)
{
    struct surface *surface = (struct surface *)surface_;

    //v3d function will deal with the fact that window surfaces can't be used by the API call anyway
    return eglBindTexImage_Int(display, surface->surface, buffer);
}



EGLBoolean eglReleaseTexImage(EGLDisplay display, EGLSurface surface_, EGLint buffer)
{
    struct surface *surface = (struct surface *)surface_;

   //v3d function will deal with the fact that window surfaces can't be used by the API call anyway
    return eglReleaseTexImage_Int(display, surface->surface, buffer);
}

static KHRN_IMAGE_FORMAT_T convert_android_format(int format,bool isTiled)
{
   KHRN_IMAGE_FORMAT_T result;

   LOGV("convert_android_format IN - format = %i", format );

   if(isTiled)
   	{
   	switch (format)
		{
		case HAL_PIXEL_FORMAT_YCbCr_422_I:
		case HAL_PIXEL_FORMAT_RGBA_8888:
		case HAL_PIXEL_FORMAT_RGBX_8888:
		case HAL_PIXEL_FORMAT_BGRA_8888:
			result = ABGR_8888_TF;
			break;
		case HAL_PIXEL_FORMAT_RGB_888:
			result = BGR_888_RSO;
			break;
		case HAL_PIXEL_FORMAT_YCbCr_420_SP:
		case HAL_PIXEL_FORMAT_YCrCb_420_SP:
		case HAL_PIXEL_FORMAT_YV12:
		case HAL_PIXEL_FORMAT_YCbCr_420_P:
		case HAL_PIXEL_FORMAT_RGB_565:
			result = RGB_565_TF;
			break;
		default:
//			LOGE("**************convert_android_format = %x",format);
			result = ABGR_8888_TF;
			break;
		}
	}
   else
   	{
   	switch (format)
		{
		case HAL_PIXEL_FORMAT_YCbCr_422_I:
		case HAL_PIXEL_FORMAT_RGBA_8888:
		case HAL_PIXEL_FORMAT_RGBX_8888:
		case HAL_PIXEL_FORMAT_BGRA_8888:
		   result = ABGR_8888_RSO;
		   break;
	   case HAL_PIXEL_FORMAT_RGB_888:
		   result = BGR_888_RSO;
		   break;
	   case HAL_PIXEL_FORMAT_YCbCr_420_SP:
	   case HAL_PIXEL_FORMAT_YCrCb_420_SP:
	   case HAL_PIXEL_FORMAT_YV12:
	   case HAL_PIXEL_FORMAT_YCbCr_420_P:
	   case HAL_PIXEL_FORMAT_RGB_565:
		   result = RGB_565_RSO;
		   break;
	   default:
		   result = ABGR_8888_TF;
		   break;
	   }
   	}

   LOGV("convert_android_format OUT - result = %i", result );

   return result;
}

static int calculate_pitch( const KHRN_IMAGE_FORMAT_T format, const uint32_t width )
{
   int pitch = 0;

   LOGV("calculate_pitch IN - format = %i, width = %i", format, width );
   switch (format)
   	{
   	case ABGR_8888_RSO:
   	case ABGR_8888_TF:
    case ARGB_8888_RSO:
    case ARGB_8888_TF:
		pitch = (4 * width);
		break;
	case BGR_888_RSO:
		pitch = (3 * width);
		break;
	case RGB_565_RSO:
	case RGB_565_TF:
		pitch = (2 * width);
		break;
	default:
		break;
	}
   LOGV("calculate_pitch OUT pitch = %i", pitch );

   return pitch;
}

EGLBoolean eglDestroyImageKHR(EGLDisplay display, EGLImageKHR image)
{
   EGLBoolean ret = EGL_FALSE;

   LOGV("eglDestroyImageKHR - 1" );

   if( EGL_NO_IMAGE_KHR != image )
   {
      EGL_IMAGE_T *img = (EGL_IMAGE_T *)image;

      LOGV("eglDestroyImageKHR - 2" );

      //force a flush
      glFinish();

      //delete the KHR image
      ret = eglDestroyImageKHR_Int( display, img->v3d_egl_imagekhr );

      LOGE_IF( ret == EGL_FALSE, "eglSwapBuffers: eglDestroyImageKHR FAILED" );

      //dec the ref
      // ((native_buffer *)img->abuffer->handle)->decRef();
      img->abuffer->common.decRef(&img->abuffer->common);

      free( img );
   }

   return ret;
}

void glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image)
{
   if( EGL_NO_IMAGE_KHR != image )
   {
      EGL_IMAGE_T *img = (EGL_IMAGE_T *)image;

      LOGV("glEGLImageTargetTexture2DOES = 0x%08X", (int)image );
	  struct context * ctx = brcm_egl_get_current();
	  process_img(ctx->composer,img->abuffer);

      glEGLImageTargetTexture2DOES_Int( target, img->v3d_egl_imagekhr );
   }
   else
      LOGE("eglDestroyImageKHR - FATAL ERROR" );
   glGetError();
}

EGLImageKHR eglCreateImageKHR(EGLDisplay display, EGLContext context, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)
{
   KHRN_IMAGE_WRAP_T vc_buffer;
   EGLImageKHR ret = EGL_NO_IMAGE_KHR;

   LOGV("eglCreateImageKHR - 1" );

   if(target == EGL_NATIVE_BUFFER_ANDROID)
   {
      android_native_buffer_t *abuffer = (android_native_buffer_t  *)buffer;

      LOGV("eglCreateImageKHR - 2" );

      //get the details from the android_na
      if( abuffer )
      {
         if( brcm_egl_convert_anative_buf_to_khrn_image( abuffer, &vc_buffer ,1 ) )
         {
		 vc_buffer.width = getWidth(abuffer);
		 vc_buffer.height = getHeight(abuffer);
		 vc_buffer.aux = getvStorage(abuffer);
		 vc_buffer.storage = getStorage(abuffer);
		 //lock the buffer being sent
            // ((native_buffer *)abuffer->handle)->incRef();
            abuffer->common.incRef(&abuffer->common);

            ret = eglCreateImageKHR_Int( display, context, EGL_IMAGE_WRAP_BRCM, &vc_buffer, attrib_list );

            if( ret != EGL_NO_IMAGE_KHR )
            {
               EGL_IMAGE_T *img = calloc( 1, sizeof( EGL_IMAGE_T ) );

               if( img )
               {
                  img->abuffer = abuffer;
                  img->v3d_egl_imagekhr = ret;

                  LOGV("eglCreateImageKHR - 4" );

                  //return our wrapper version instead
                  ret = (EGLImageKHR)img;
               }
               else
               {
                  LOGV("eglCreateImageKHR - 5" );

                  eglDestroyImageKHR( display, ret );
                  // ((native_buffer *)abuffer->handle)->decRef();
                  abuffer->common.decRef(&abuffer->common);
                  ret = EGL_NO_IMAGE_KHR;
               }
            }
            else
            {
               LOGV("eglCreateImageKHR - 6" );
               // ((native_buffer *)abuffer->handle)->decRef();
               abuffer->common.decRef(&abuffer->common);
            }
         }
         else
         {
            LOGE("eglCreateImageKHR - FAIL - brcm_egl_convert_anative_buf_to_khrn_image failed" );
         }
      }
   }

   return ret;
}

EGLBoolean eglSurfaceAttrib(EGLDisplay display, EGLSurface surface_, EGLint attribute, EGLint value)
{
   struct surface *surface = (struct surface *)surface_;

   return eglSurfaceAttrib_Int(display, surface->surface, attribute, value);
}

EGLClientBuffer eglGetRenderBufferANDROID(EGLDisplay display, EGLSurface surface_)
{
	struct surface *surface = (struct surface *)surface_;
	return surface->buffer->handle;
}

//
// helpers
//

static pthread_key_t tls_key;
static pthread_once_t tls_key_once = PTHREAD_ONCE_INIT;

static void init_tls_key(void)
{
    pthread_key_create(&tls_key, NULL);
}

static void set_current(struct context *context)
{
    pthread_once(&tls_key_once, init_tls_key);
    pthread_setspecific(tls_key, context);
}


static uint32_t getseconds( void )
{
   struct timeval tv;
   uint32_t tm = 0;

   if (!gettimeofday(&tv, NULL))
   {
      tm =  tv.tv_sec;
   }

   return tm;
}

static void sleep_in_seconds(uint32_t seconds)
{
   struct timespec ts;
   ts.tv_sec = seconds;
   ts.tv_nsec = 0;
   nanosleep(&ts, NULL);
}

//Acquire and lock a buffer for a surface if it does not already have one
static bool surface_dequeue_buffer(struct surface *surface)
{
   bool ok = false;

   LOGV("surface_dequeue_buffer - 1" );

   if( surface )
   {
      LOGV("surface_dequeue_buffer - 2" );

      if(!surface->buffer)
      {
         LOGV("surface_dequeue_buffer - 3" );

         if(surface->window)
         {
            LOGV("surface_dequeue_buffer - 4" );

             surface->window->dequeueBuffer(surface->window, &surface->buffer);

             if( surface->buffer )
             {
                  LOGV("surface_dequeue_buffer - 5" );

                 surface->window->lockBuffer(surface->window, surface->buffer);

                 //TODO - return the physical buffer to the caller

                 ok = true;
             }
             else
                 LOGE("surface_dequeue_buffer - FATAL ERROR - no buffer" );
         }
         else
            LOGE("surface_dequeue_buffer - FATAL ERROR - no window" );
      }
   }
   else
      LOGE("surface_dequeue_buffer - FATAL ERROR - no surface" );

   return ok;
}

static void surface_enqueue_buffer(struct surface *surface)
{
    //if we have a buffer bound, send it to the window manager (or what ever is listening)
    if(surface->buffer)
    {
       LOGV("surface_enqueue_buffer - 2" );
	   if(surface->isTiled)
	   	{
//	   	surface->buffer->tiled = 1;
	   	}
	   else
	   	{
//		   surface->buffer->tiled = 0;
	   	}
        surface->window->queueBuffer(surface->window, surface->buffer);
        surface->buffer = 0;
    }
    else
       LOGE("surface_enqueue_buffer - FATAL ERROR - no buffer" );
}

//Attach an android native buffer to the V3D backend (for the current context)
static void attach_buffer(EGLDisplay display, struct surface *surface)
{
   LOGV("attach_buffer - 1" );

   if( surface && surface->buffer )
   {
      EGLBoolean ret = EGL_FALSE;

      LOGV("attach_buffer - 2" );

      KHRN_IMAGE_WRAP_T vc_buffer;

      if( brcm_egl_convert_anative_buf_to_khrn_image( surface->buffer, &vc_buffer ,surface->isTiled) )
      {
         LOGV("attach_buffer - 3" );
		 if(surface->buffer->format == HAL_PIXEL_FORMAT_YCbCr_422_I) {
		 	vc_buffer.storage = getStorage(surface->buffer);
		 	vc_buffer.aux = getvStorage(surface->buffer);
		 	}
         //call Giles function here
         ret = eglDirectRenderingPointer( display, surface->surface, &vc_buffer );

         if( !ret )
         {
            LOGE("attach_buffer - eglDirectRenderingPointer FAILED" );
         }
      }
      else
      {
         LOGE("attach_buffer - brcm_egl_convert_anative_buf_to_khrn_image FAILED" );
      }
   }
}

//Link a framebuffer to a surface in the current context
static void attach_surface(EGLDisplay display, struct context *current, struct surface *surface)
{
   if(!surface->buffer)
   {
       //An attached surface must have a buffer
       surface_dequeue_buffer(surface);

       LOGV("attach_surface - 1" );

       if(surface->buffer)
       {
         LOGV("attach_surface - 2" );

         attach_buffer( display, surface );

         //yay
         LOGV("attach_surface - 3 (yay)" );
       }
       else
       {
           LOGV("attach_surface() Failed to dequeue buffer: surface = %p, Thread = %d",
               surface,
               gettid());
       }
   }
   else
   {
      LOGV("attach_surface() - already got buffer, so we are happy: surface  = %p, Thread = %d",
               surface,
               gettid());
   }

    surface->made_current = 1;
}


//Setup GL state for current context
static void attach_current_context(EGLDisplay display, struct context *current)
{
    int32_t width, height;
    struct surface *draw_surface = current->draw;
    struct surface *read_surface = current->read;

    LOGV("attach_current_context - 1" );

    //if the surface(s) have a valid window, grab a buffer and send it to v3d
    //this only does something the first time (if there is no buffer already grabbed
    if(read_surface->window)
    {
        attach_surface(display, current, read_surface);
    }

    if(draw_surface->window)
    {
        attach_surface(display, current, draw_surface);
    }

    //debug code only
    if(!current->made_current)
    {
        char value[PROPERTY_VALUE_MAX];

        if(property_get("hw.broadcom.gles_profiling", value, 0))
        {
            if(strcmp(value, "true") == 0)
            {
                current->profiling = 1;
            }
            else
            {
                current->profiling = 0;
            }
        }
    }

    // current->profiling = 1;

    current->made_current = 1;
}

struct context *brcm_egl_get_current()
{
    pthread_once(&tls_key_once, init_tls_key);
    return (struct context *)pthread_getspecific(tls_key);
}

static bool brcm_egl_convert_anative_buf_to_khrn_image( android_native_buffer_t *abuffer, KHRN_IMAGE_WRAP_T *khr_image ,bool isTiled)
{
   bool ret = false;

   if( NULL != khr_image )
   {

      KHRN_IMAGE_FORMAT_T format = convert_android_format( abuffer->format ,isTiled) ; //get the format from the gralloc image
#ifdef BRCM_V3D_OPT
      uint32_t width = abuffer->stride; //get the width from the gralloc image
#else
      uint32_t width = abuffer->width; //get the width from the gralloc image
#endif
      uint32_t height = abuffer->height; //get the width from the gralloc image
      int32_t stride = calculate_pitch( format, width );

      // native_buffer *nbuffer = (native_buffer *)abuffer->handle;
      struct private_handle_t* nbuffer = (struct private_handle_t*)abuffer->handle;
      void *storage = nbuffer->p_addr + nbuffer->offset; //get the buffer pointer from the gralloc image - its the 7th item in the structure

      memset( khr_image, 0, sizeof( KHRN_IMAGE_WRAP_T ) );

      if( format != IMAGE_FORMAT_INVALID )
      {
         LOGV("brcm_egl_convert_anative_buf_to_khrn_image - 3" );

         khrn_image_wrap( khr_image, format, width, height, stride, storage );
#ifdef BRCM_V3D_OPT
		 khr_image->aux = nbuffer->base; //get the buffer pointer from the gralloc image - its the 7th item in the structure
#endif
         ret = true;
      }
      else
         LOGE("brcm_egl_convert_anative_buf_to_khrn_image - 3" );
   }

   return ret;
}

void* eglGetComposerANDROID(EGLDisplay dpy, EGLSurface draw)
{
	struct context *current_context = brcm_egl_get_current();
	if (current_context)
		return getComposer(current_context->composer);
	else
		return NULL;
}


#endif
