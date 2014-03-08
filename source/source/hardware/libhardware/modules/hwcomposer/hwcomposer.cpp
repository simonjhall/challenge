/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <hardware/hardware.h>

#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <cstring>

#include <hardware/hwcomposer.h>

#include "ghw_composer.h"
#include "gralloc_priv.h"
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>

/*****************************************************************************/
namespace ghw{

class GhwMemHandle_Wrap : public GhwMemHandle {
public:
	u32 mPhys;
	void* mVirt;
	u32 mSize;
	u32 refcnt;
	GhwMemHandle_Wrap(u32 phys,void* virt,u32 size):mPhys(phys),mVirt(virt),mSize(size),refcnt(1) {};
    virtual    ghw_error_e     acquire(){refcnt++; return GHW_ERROR_NONE;};
    virtual    ghw_error_e     release(){refcnt--; if(refcnt==0) delete this;return GHW_ERROR_NONE;};
    virtual    ghw_error_e     lock(u32& ipa_addr, void*& virt_addr, u32& size){ipa_addr = mPhys; virt_addr = mVirt; size = mSize;return GHW_ERROR_NONE;};
    virtual    ghw_error_e     unlock(){return GHW_ERROR_NONE;};

    virtual    ghw_error_e     setName(const char *name){return GHW_ERROR_NONE;};
    virtual    ghw_error_e     dump(u32 level = 0){return GHW_ERROR_NONE;};
    virtual ~GhwMemHandle_Wrap(){};
};

};
using namespace ghw;



struct hwc_context_t {
    hwc_composer_device_t device;
    /* our private state goes below here */
	GhwComposer* local_composer;
	GhwComposer* egl_composer;
	bool useComposer;
	unsigned int ucount;
	unsigned int logtime;
};

static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static struct hw_module_methods_t hwc_module_methods = {
    open: hwc_device_open
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: HWC_HARDWARE_MODULE_ID,
        name: "Sample hwcomposer module",
        author: "The Android Open Source Project",
        methods: &hwc_module_methods,
    }
};

/*****************************************************************************/

static void dump_layer(hwc_layer_t const* l) {
    LOGD("\ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, {%d,%d,%d,%d}, {%d,%d,%d,%d}",
            l->compositionType, l->flags, l->handle, l->transform, l->blending,
            l->sourceCrop.left,
            l->sourceCrop.top,
            l->sourceCrop.right,
            l->sourceCrop.bottom,
            l->displayFrame.left,
            l->displayFrame.top,
            l->displayFrame.right,
            l->displayFrame.bottom);
}

static int hwc_prepare(hwc_composer_device_t *dev, hwc_layer_list_t* list) {

    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    if (list && (list->flags & HWC_GEOMETRY_CHANGED)) {
		// Default Settings for composer is to use composer
		// Hence Set useComposer to true 
		// Also, Set the composition type to OVERLAY so that Surfaceflinger does not compose these layers
		ctx->useComposer = true;
		int compType= HWC_OVERLAY;
		
        for (size_t i=0 ; i<list->numHwLayers ; i++) {
			if(list->hwLayers[i].flags & HWC_SKIP_LAYER) {
				// Surfaceflinger signalled that HW Composer should not compose this layer
				// Current design is not capable of drawing some layers using OpenGL and some layers using HW Composer since 
				// the Order of the requests to HW will be lost.
				// Hence, when this signal is given, Force Surfaceflinger to composer all layers
				ctx->useComposer = false;
				compType = HWC_FRAMEBUFFER;
				}
        	}
		for (size_t i=0 ; i<list->numHwLayers ; i++) {
            list->hwLayers[i].compositionType = compType;
        }
    }
    return 0;
}

static int get_lib_format(const int& in, u32& format, u32& intformat)
{
	// Foy YUV layers , default output format after conversion is RGB565
	// Change intformat also change the memory allocations and stride parameters in GRALLOC appropriately
	switch(in)
		{
		case HAL_PIXEL_FORMAT_RGBA_8888:
			format = GHW_PIXEL_FORMAT_RGBA_8888;
			intformat = format;
			break;
		case HAL_PIXEL_FORMAT_RGBX_8888:
			format = GHW_PIXEL_FORMAT_RGBX_8888;
			intformat = format;
			break;
		case HAL_PIXEL_FORMAT_RGB_888:
			format = GHW_PIXEL_FORMAT_RGB_888;
			intformat = format;
			break;
		case HAL_PIXEL_FORMAT_RGB_565:
			format = GHW_PIXEL_FORMAT_RGB_565;
			intformat = format;
			break;
		case HAL_PIXEL_FORMAT_BGRA_8888:
			format = GHW_PIXEL_FORMAT_BGRA_8888;
			intformat = format;
			break;
		case HAL_PIXEL_FORMAT_RGBA_5551:
			format = GHW_PIXEL_FORMAT_RGBA_5551;
			intformat = format;
			break;
		case HAL_PIXEL_FORMAT_RGBA_4444:
			format = GHW_PIXEL_FORMAT_RGBA_4444;
			intformat = format;
			break;
		case HAL_PIXEL_FORMAT_YCrCb_420_SP:
			format = GHW_PIXEL_FORMAT_YCrCb_420_SP;
			intformat = GHW_PIXEL_FORMAT_RGB_565;
			break;
		case HAL_PIXEL_FORMAT_YCbCr_420_SP:
			format = GHW_PIXEL_FORMAT_YCbCr_420_SP;
			intformat = GHW_PIXEL_FORMAT_RGB_565;
			break;
		case HAL_PIXEL_FORMAT_YV12:
			format = GHW_PIXEL_FORMAT_YCbCr_420_P;
			intformat = GHW_PIXEL_FORMAT_RGB_565;
			break;
		case HAL_PIXEL_FORMAT_YCbCr_420_P:
			format = GHW_PIXEL_FORMAT_YCbCr_420_P;
			intformat = GHW_PIXEL_FORMAT_RGB_565;
			break;
		default:
			LOGE("Unsupported format %d",in);
			return -1;
		}
	return 0;
}

static int hwc_set(hwc_composer_device_t *dev,
        hwc_display_t dpy,
        hwc_surface_t sur,
        hwc_layer_list_t* list)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
	u32 format = 0,intformat =0;
	GhwComposer* composer;

	// glEGLImageTargetTexture2DOES extension in BroadCom OpenGL library performs the tRSO to Tile Format conversion
	// glGetTextureStorage is implemented in V3D OpenGL stack to retrieve the Texture physical address (in Tile Format)
	// If Software OpenGL is used, the OpenGL extension might not be implemented or does not perform Tile Conversion
	// In Such cases, Tile Conversion is performed here

	const GLubyte* vendor = glGetString(GL_VENDOR);
		
	bool convert = (0 != strcmp((const char*)vendor , "Broadcom"));

    void* tempptr = eglGetRenderBufferANDROID((EGLDisplay)dpy, (EGLSurface)sur);
	ctx->egl_composer = (GhwComposer*)eglGetComposerANDROID((EGLDisplay)dpy, (EGLSurface)sur);

	if (ctx->egl_composer) {
		composer = ctx->egl_composer;
		if (ctx->local_composer) {
			delete ctx->local_composer;
			ctx->local_composer = NULL;
		}
	} else {
		if (ctx->local_composer == NULL) {
			ctx->local_composer = GhwComposer::create();
		}
		composer = ctx->local_composer;
	}

	const private_handle_t* fbhandle = reinterpret_cast <const private_handle_t*> (tempptr);

	if(fbhandle == NULL){
		LOGE("fbhandle is NULL");
		return 0;
	}
    GhwMemHandle* fbwrap = new GhwMemHandle_Wrap(fbhandle->p_addr + fbhandle->offset,(void*)(fbhandle->base),fbhandle->size);
	GhwImgBuf* img = GhwImgBuf::create();
	GhwImgBuf* intimg = GhwImgBuf::create();
	GhwImgOp* op = GhwImgOp::create();

	if( ctx->useComposer && list) {
		img->setMemHandle(fbwrap);
		img->setGeometry(fbhandle->alignedw,fbhandle->alignedh);
		get_lib_format(fbhandle->format,format,intformat);
		img->setFormat(format);
		img->setCrop(fbhandle->w,fbhandle->h);

		composer->compSetFb(img,GHW_DITHER_NONE);

		for (size_t i=0 ; i<list->numHwLayers ; i++) {
			const private_handle_t* handle = reinterpret_cast <const private_handle_t*>(list->hwLayers[i].handle);
			if(handle == NULL) {
				continue;
				}
			if(get_lib_format(handle->format,format,intformat)) {
				LOGE("Skipping layer draw with unknown format %d",handle->format);
				continue;
				}
			GhwMemHandle* wrap = new GhwMemHandle_Wrap(handle->p_addr,(void*)(handle->base),handle->size/2);
			img->setMemHandle(wrap);
			img->setGeometry(handle->alignedw,handle->alignedh);
			img->setFormat(format);
			img->setCrop(handle->w,handle->h);

			GhwMemHandle* intwrap =  new GhwMemHandle_Wrap((u32)(handle->p_addr + (handle->size/2)),(void*)(handle->base+(handle->size/2)),handle->size/2);
			intimg->setMemHandle(intwrap);
			intimg->setGeometry(handle->alignedw,handle->alignedh);
			intimg->setFormat(intformat,GHW_MEM_LAYOUT_TILED);
			intimg->setCrop(handle->w,handle->h);

			op->setDstWindow(0,0,handle->w,handle->h);
			op->setTransform(0);

			if(convert) composer->imgProcess(img,intimg,op,0);

			hwc_rect_t* rect = &list->hwLayers[i].sourceCrop;
			intimg->setCrop(rect->left,rect->top,rect->right,rect->bottom);

			rect = &list->hwLayers[i].displayFrame;
			op->setDstWindow(rect->left,rect->top,rect->right,rect->bottom);
			op->setTransform(list->hwLayers[i].transform);

			composer->compDrawRect(intimg,op);
			wrap->release();
			intwrap->release();
			}
		composer->compCommit(0);
		composer->barrier();
		}
	else {
		glFlush();

		if((fbhandle->format == HAL_PIXEL_FORMAT_BGRA_8888) && !convert) {
			
			img->setMemHandle(fbwrap);
			img->setGeometry(fbhandle->alignedw,fbhandle->alignedh);
			get_lib_format(HAL_PIXEL_FORMAT_RGBA_8888,format,intformat);
			img->setFormat(format);
			img->setCrop(fbhandle->w,fbhandle->h);

			intimg->setMemHandle(fbwrap);
			intimg->setGeometry(fbhandle->alignedw,fbhandle->alignedh);
			get_lib_format(fbhandle->format,format,intformat);
			intimg->setFormat(format);
			intimg->setCrop(fbhandle->w,fbhandle->h);

			op->setDstWindow(0,0,fbhandle->alignedw,fbhandle->alignedh);
			op->setTransform(0);

			composer->imgProcess(img,intimg,op,0);
			}
		composer->barrier();

		}
		
		fbwrap->release();
		delete img;
		delete intimg;
		delete op;

        struct timeval t1;
        gettimeofday(&t1,NULL);

        if(t1.tv_sec == ctx->logtime) {
			ctx->ucount++;
			} else {
			LOGE("FrameBuffer [%d] reloads", ctx->ucount);
			ctx->ucount = 1;
			ctx->logtime = t1.tv_sec;
			}

    EGLBoolean sucess = eglSwapBuffers((EGLDisplay)dpy, (EGLSurface)sur);
    if (!sucess) {
        return HWC_EGL_ERROR;
    }
    return 0;
}

static int hwc_device_close(struct hw_device_t *dev)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    if (ctx) {
		delete ctx->local_composer;
		ctx->local_composer = NULL;
		ctx->egl_composer = NULL;
        free(ctx);
    }
    return 0;
}

/*****************************************************************************/

static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    int status = -EINVAL;
	LOGE("hwc_device_open");
    if (!strcmp(name, HWC_HARDWARE_COMPOSER)) {
        struct hwc_context_t *dev;
        dev = (hwc_context_t*)malloc(sizeof(*dev));

        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));

        /* initialize the procs */
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = 0;
        dev->device.common.module = const_cast<hw_module_t*>(module);
        dev->device.common.close = hwc_device_close;

        dev->device.prepare = hwc_prepare;
        dev->device.set = hwc_set;
		dev->local_composer = GhwComposer::create();
		dev->useComposer = true;

        dev->logtime =0;
		dev->ucount =0;
        *device = &dev->device.common;
        status = 0;
    }
    return status;
}
