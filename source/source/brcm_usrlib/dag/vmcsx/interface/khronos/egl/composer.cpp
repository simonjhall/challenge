/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
//#include "utils/Log.h"

#include "brcm_egl.h"

#include "gralloc_priv.h"

#include <ghw_composer.h>

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

struct composer{
	GhwComposer* composer;
};
void release_composer(struct composer* comp)
{
if(comp)
	{
	if(comp->composer) {
		delete comp->composer;
		}
	delete comp;
	}
}

void* getComposer(struct composer* comp)
{
	if (comp)
		return comp->composer;
	else
		return NULL;
	
}
struct composer* init_composer()
{
	struct composer* comp = new struct composer;
	comp->composer = GhwComposer::create();
	if(NULL == comp->composer)
		{
		LOGE("composer create failed");
		delete comp;
		return NULL;
		}
	return comp;
}

int getWidth(android_native_buffer_t* buf)
{
	const private_handle_t* handle =reinterpret_cast <const private_handle_t*>( buf->handle);
	return handle->w;
}
int getHeight(android_native_buffer_t* buf )
{
	const private_handle_t* handle = reinterpret_cast <const private_handle_t*>(buf->handle);
	return handle->h;
}
int getStorage(android_native_buffer_t* buf)
{
	const private_handle_t* handle = reinterpret_cast <const private_handle_t*>(buf->handle);
	return handle->p_addr + (handle->size/2);
}
void* getvStorage(android_native_buffer_t* buf )
{
	const private_handle_t* handle =reinterpret_cast <const private_handle_t*>( buf->handle);
	return (void*)(handle->base + (handle->size/2));
}

void convert_to_yuv(struct composer* comp,android_native_buffer_t* buf)
{
	int format = 0,intformat =0;
	GhwImgBuf* img = GhwImgBuf::create();
	GhwImgBuf* intimg = GhwImgBuf::create();
	GhwImgOp* op = GhwImgOp::create();

	const private_handle_t* handle =reinterpret_cast <const private_handle_t*>( buf->handle);

	format = GHW_PIXEL_FORMAT_RGBA_8888;
	intformat = GHW_PIXEL_FORMAT_YUYV_422;

	GhwMemHandle_Wrap* htemp = new GhwMemHandle_Wrap((u32)(handle->p_addr),(void*&)(handle->base),handle->size/2);
	GhwMemHandle_Wrap* hin = new GhwMemHandle_Wrap((u32)(handle->p_addr + (handle->size/2)),(void*)(handle->base+(handle->size/2)),handle->size/2);

	img->setMemHandle(hin);
	img->setGeometry(handle->alignedw,handle->alignedh);
	img->setFormat(format);
	img->setCrop(handle->w,handle->h);

	intimg->setMemHandle(htemp);
	intimg->setGeometry(handle->alignedw,handle->alignedh);
	intimg->setFormat(intformat);
	intimg->setCrop(handle->w,handle->h);

	op->setDstWindow(0,0,handle->w,handle->h);
	op->setTransform(0);

	comp->composer->imgProcess(img,intimg,op,1);
	
	hin->release();
	htemp->release();
	delete img;
	delete intimg;
	delete op;
}


void process_img(struct composer* comp,android_native_buffer_t* buf)
{
	int format = 0,intformat =0;
	GhwImgBuf* img = GhwImgBuf::create();
	GhwImgBuf* intimg = GhwImgBuf::create();
	GhwImgOp* op = GhwImgOp::create();

	const private_handle_t* handle =reinterpret_cast <const private_handle_t*>( buf->handle);

	switch(handle->format)
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
			LOGE("Unsupported format %d",handle->format);
			format = GHW_PIXEL_FORMAT_RGB_565;
			intformat = GHW_PIXEL_FORMAT_RGB_565;
			break;
		}
	GhwMemHandle_Wrap* hin = new GhwMemHandle_Wrap((u32)(handle->p_addr),(void*&)(handle->base),handle->size/2);
	GhwMemHandle_Wrap* htemp = new GhwMemHandle_Wrap((u32)(handle->p_addr + (handle->size/2)),(void*)(handle->base+(handle->size/2)),handle->size/2);

	img->setMemHandle(hin);
	img->setGeometry(handle->alignedw,handle->alignedh);
	img->setFormat(format);
	img->setCrop(handle->w,handle->h);

	intimg->setMemHandle(htemp);
	intimg->setGeometry(handle->alignedw,handle->alignedh);
	intimg->setFormat(intformat,GHW_MEM_LAYOUT_TILED);
	intimg->setCrop(handle->w,handle->h);

	op->setDstWindow(0,0,handle->w,handle->h);
	op->setTransform(0);

	comp->composer->imgProcess(img,intimg,op,0);
	
	hin->release();
	htemp->release();
	delete img;
	delete intimg;
	delete op;
}

void sync_composer(struct composer* comp )
{
	if(comp)
		comp->composer->barrier();
}


