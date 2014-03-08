/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <cutils/ashmem.h>
#include <cutils/log.h>
#include <cutils/atomic.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include "gralloc_priv.h"
#include "gr.h"

#include "ghw_allocator.h"
#include <linux/android_pmem.h>

using namespace ghw;
/*****************************************************************************/

struct gralloc_context_t {
    alloc_device_t  device;
    /* our private data here */
    GhwMemAllocator* allocator;
    int gemem_master;
};

static int gralloc_alloc_buffer(alloc_device_t* dev,
        size_t size, int usage, buffer_handle_t* pHandle,int w,int h,int format,int stride,int hstride);

/*****************************************************************************/

int fb_device_open(const hw_module_t* module, const char* name,
        hw_device_t** device);

static int gralloc_device_open(const hw_module_t* module, const char* name,
        hw_device_t** device);

extern int gralloc_lock(gralloc_module_t const* module,
        buffer_handle_t handle, int usage,
        int l, int t, int w, int h,
        void** vaddr);

extern int gralloc_unlock(gralloc_module_t const* module,
        buffer_handle_t handle);

extern int gralloc_register_buffer(gralloc_module_t const* module,
        buffer_handle_t handle);

extern int gralloc_unregister_buffer(gralloc_module_t const* module,
        buffer_handle_t handle);

/*****************************************************************************/

static struct hw_module_methods_t gralloc_module_methods = {
        open: gralloc_device_open
};

struct private_module_t HAL_MODULE_INFO_SYM = {
    base: {
        common: {
            tag: HARDWARE_MODULE_TAG,
            version_major: 1,
            version_minor: 0,
            id: GRALLOC_HARDWARE_MODULE_ID,
            name: "Graphics Memory Allocator Module",
            author: "The Android Open Source Project",
            methods: &gralloc_module_methods
        },
        registerBuffer: gralloc_register_buffer,
        unregisterBuffer: gralloc_unregister_buffer,
        lock: gralloc_lock,
        unlock: gralloc_unlock,
    },
    framebuffer: 0,
    flags: 0,
    numBuffers: 0,
    bufferMask: 0,
    lock: PTHREAD_MUTEX_INITIALIZER,
    currentBuffer: 0,
};

/*****************************************************************************/

static int gralloc_alloc_framebuffer_locked(alloc_device_t* dev,
        size_t size, int usage, buffer_handle_t* pHandle,int w,int h,int format,int stride,int hstride)
{
    private_module_t* m = reinterpret_cast<private_module_t*>(
            dev->common.module);

    // allocate the framebuffer
    if (m->framebuffer == NULL) {
        // initialize the framebuffer, the framebuffer is mapped once
        // and forever.
        int err = mapFrameBufferLocked(m);
        if (err < 0) {
            return err;
        }
    }

    const uint32_t bufferMask = m->bufferMask;
    const uint32_t numBuffers = m->numBuffers;
    const size_t bufferSize = m->finfo.line_length * m->info.yres;
    if (numBuffers == 1) {
        // If we have only one buffer, we never use page-flipping. Instead,
        // we return a regular buffer which will be memcpy'ed to the main
        // screen when post is called.
        int newUsage = (usage & ~GRALLOC_USAGE_HW_FB) | GRALLOC_USAGE_HW_2D;
        return gralloc_alloc_buffer(dev, bufferSize, newUsage, pHandle,w,h,format,stride,hstride);
    }

    if (bufferMask >= ((1LU<<numBuffers)-1)) {
        // We ran out of buffers.
        return -ENOMEM;
    }

    // create a "fake" handles for it
    intptr_t vaddr = intptr_t(m->framebuffer->base);
    unsigned int paddr = m->smem_start;
    private_handle_t* hnd = new private_handle_t(dup(m->framebuffer->fd), size,
            private_handle_t::PRIV_FLAGS_FRAMEBUFFER);

    // find a free slot
    for (uint32_t i=0 ; i<numBuffers ; i++) {
        if ((bufferMask & (1LU<<i)) == 0) {
            m->bufferMask |= (1LU<<i);
            break;
        }
        vaddr += bufferSize;
        paddr += bufferSize;
    }

    hnd->base = vaddr;
    hnd->offset = vaddr - intptr_t(m->framebuffer->base);
    hnd->p_addr = m->smem_start;
    hnd->w = w;
    hnd->h = h;
    hnd->format = format;
    hnd->alignedw = stride;
    hnd->alignedh = hstride;
    *pHandle = hnd;

    return 0;
}

static int gralloc_alloc_framebuffer(alloc_device_t* dev,
        size_t size, int usage, buffer_handle_t* pHandle,int w,int h,int format,int stride,int hstride)
{
    private_module_t* m = reinterpret_cast<private_module_t*>(
            dev->common.module);
    pthread_mutex_lock(&m->lock);
    int err = gralloc_alloc_framebuffer_locked(dev, size, usage, pHandle,w,h,format,stride,hstride);
    pthread_mutex_unlock(&m->lock);
    return err;
}

static int gralloc_alloc_buffer(alloc_device_t* dev,
        size_t size, int usage, buffer_handle_t* pHandle,int w,int h,int format,int stride, int hstride)

{
	int pgsize = PAGE_SIZE;
	gralloc_context_t* ctx = (gralloc_context_t*)dev;

	size = (size + pgsize) & (~(pgsize - 1));

	size = 2* size;

	/* get memory linear memory buffers */
	int fd_handle = open("/dev/gememalloc", O_RDWR);
    void* v_addr = mmap(0,size,PROT_READ | PROT_WRITE,MAP_SHARED,fd_handle,0);

	if(v_addr == MAP_FAILED)
	{
		LOGE("gralloc allocation failed for %d\n",size);
		return -1;
	}

   // create a duplicate handles for it
    private_handle_t* hnd = new private_handle_t(dup(fd_handle), size, 0);

    u32 p_addr;
    pmem_region PmemRegion;
    ioctl(fd_handle, PMEM_GET_PHYS, &PmemRegion);
	p_addr = PmemRegion.offset;

//    handle->lock(p_addr,v_addr,size);


	memset(v_addr, 0,size/2);
	// Save the physical address in offset and p_addr of handle
    hnd->base = (int)v_addr;
    hnd->offset = 0;
	hnd->p_addr = (int)p_addr;
	hnd->w = w;
	hnd->h =h;
	hnd->format = format;
	hnd->alignedw = stride;
	hnd->alignedh = hstride;
//	hnd->handle = (void*)handle;
	LOGV("gralloc_alloc_buffer: Handle Values physical addr %x,	virtual addr: %x", hnd->p_addr, hnd->base + hnd->offset);

	close(fd_handle);

    *pHandle = hnd;
	return 0;
}

/*****************************************************************************/

static int gralloc_alloc(alloc_device_t* dev,
        int w, int h, int format, int usage,
        buffer_handle_t* pHandle, int* pStride)
{
    if (!pHandle || !pStride)
        return -EINVAL;

    size_t size, stride;

    int align = 4;
    int bpp = 0;
    int pad = 1;
	unsigned int tempw	 = (w+31)&0xFFFFFFE0;
	unsigned int temph	 = (h+31)&0xFFFFFFE0;
    switch (format) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
        case HAL_PIXEL_FORMAT_BGRA_8888:
            bpp = 4;
            break;
        case HAL_PIXEL_FORMAT_RGB_888:
            bpp = 3;
            break;
        case HAL_PIXEL_FORMAT_RGB_565:
        case HAL_PIXEL_FORMAT_RGBA_5551:
        case HAL_PIXEL_FORMAT_RGBA_4444:
			tempw	 = (w+63)&0xFFFFFFC0;
            bpp = 2;
            break;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_YV12:
        case HAL_PIXEL_FORMAT_YCbCr_420_P:
			tempw	 = (w+63)&0xFFFFFFC0;
            bpp = 2;
		    pad = 0;
            break;
		case HAL_PIXEL_FORMAT_YCbCr_422_I:
			tempw	 = (w+63)&0xFFFFFFC0;
			bpp = 4;
			pad = 0;
            break;
	 default:
            return -EINVAL;
    }
    size_t bpr = (w*bpp + (align-1)) & ~(align-1);
	if (!(usage & GRALLOC_USAGE_HW_FB)) {
	bpr = (tempw*bpp + (align-1)) & ~(align-1);
	}
    size = bpr * h;
	if (!(usage & GRALLOC_USAGE_HW_FB)) {
		size = bpr*temph;
	}
    stride = bpr / bpp;
	if(pad==0) {
		tempw = w;
		temph =  h;
		stride = w;
	}
    int err =0;
    if (usage & GRALLOC_USAGE_HW_FB) {
        err = gralloc_alloc_framebuffer(dev, size, usage, pHandle,w,h,format,w,h);
    } else {
        err = gralloc_alloc_buffer(dev, size, usage, pHandle,w,h,format,stride,temph);
    }

    if (err < 0) {
        LOGE("%s: err = %x",__FUNCTION__,err);
        return err;
    }

    *pStride = stride;
    return 0;
}

static int gralloc_free(alloc_device_t* dev,
        buffer_handle_t handle)
{
    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    private_handle_t const* hnd = reinterpret_cast<private_handle_t const*>(handle);
    if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER) {
        // free this buffer
        private_module_t* m = reinterpret_cast<private_module_t*>(
                dev->common.module);
        const size_t bufferSize = m->finfo.line_length * m->info.yres;
        int index = (hnd->base - m->framebuffer->base) / bufferSize;
        m->bufferMask &= ~(1<<index);
    } else {
        LOGV("gralloc_free: Free - physical addr: %x   virtual addr: %x", hnd->p_addr, hnd->base);

        //GhwMemHandle* handle = (GhwMemHandle*) hnd->handle;

	    //handle->release();
	    munmap((void*)hnd->base ,hnd->size);

    }

    close(hnd->fd);
    delete hnd;
    return 0;
}

/*****************************************************************************/

static int gralloc_close(struct hw_device_t *dev)
{
    gralloc_context_t* ctx = reinterpret_cast<gralloc_context_t*>(dev);
    if (ctx) {
        /* TODO: keep a list of all buffer_handle_t created, and free them
         * all here.
         */
         delete ctx->allocator;
         ctx->allocator = NULL;
         close(ctx->gemem_master);
         ctx->gemem_master = -1;
        free(ctx);
    }
    return 0;
}

int gralloc_device_open(const hw_module_t* module, const char* name,
        hw_device_t** device)
{
    int status = -EINVAL;
    if (!strcmp(name, GRALLOC_HARDWARE_GPU0)) {
        gralloc_context_t *dev;
        dev = (gralloc_context_t*)malloc(sizeof(*dev));

        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));

        /* initialize the procs */
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = 0;
        dev->device.common.module = const_cast<hw_module_t*>(module);
        dev->device.common.close = gralloc_close;

        dev->device.alloc   = gralloc_alloc;
        dev->device.free    = gralloc_free;

        dev->allocator = GhwMemAllocator::create(GhwMemAllocator::GHW_MEM_ALLOC_RETAIN_ONE, 4*1024*1024, 12);

        dev->gemem_master = open("/dev/gememalloc", O_RDWR);

        *device = &dev->device.common;
        status = 0;
    } else {
        status = fb_device_open(module, name, device);
    }
    return status;
}
