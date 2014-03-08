/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PC_BUILD
#include "ghw_allocator_impl.h"

#include "ghw_memblock.h"

#include <assert.h>
#include <asm-generic/ioctl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>


extern "C" {

#ifdef USE_BRALLOC
#include "bralloc.h"
#endif

#ifdef USE_BMEM
#include "bcm_gememalloc_ioctl.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>

#define DEVICE_NAME "/dev/gememalloc"

#endif

typedef void* mspace;
mspace create_mspace_with_base(void* base, size_t capacity, int locked);
void* mspace_memalign(mspace msp, size_t alignment, size_t bytes);
void mspace_free(mspace msp, void* mem);
void mspace_malloc_stats(mspace msp);

void comp(void);

static mspace vram;

};

extern FILE *fd_v3d;
unsigned int overflowPa = 0;
unsigned int overflowSize = 1 * 1024 * 1024;

namespace ghw {

GhwMemAllocator* GhwMemAllocator::create(u32 mode , u32 slab_size , u32 alignment )
{
    GhwAllocatorImpl* allocator = new GhwAllocatorImpl(mode,slab_size,alignment);
    if(allocator->initCheck()) {
		LOGE("allocator initcheck failed");
        delete allocator;
        return NULL;
    }

    return allocator;
}

int GhwAllocatorDevice::count = 0;
int GhwAllocatorImpl::count = 0;

GhwAllocatorDevice::GhwAllocatorDevice() :mFd(-1) {
#ifdef PC_BUILD
	mFd =1;
#endif
#ifdef USE_BRALLOC
	mFd =1;
#endif
#ifdef USE_BMEM
	mFd = open(DEVICE_NAME, O_RDWR | O_SYNC);
	if(mFd <= 0 ) {
		LOGE("GhwAllocatorDevice device open failed for %s\n",DEVICE_NAME);
		}
#endif
	count++;
	pthread_mutex_init(&mLock,NULL);
}

GhwAllocatorDevice::~GhwAllocatorDevice() {
	pthread_mutex_destroy(&mLock);
	count--;
}

void GhwAllocatorDevice::setMode(u32 mode) {
	mMode = mode;
}

int GhwAllocatorDevice::initCheck() {
	if(mFd > 0) return 0;
	else return -1;
}

#define DMA_MAGIC 0xdd
#define DMA_CMA_SET_SIZE _IOW(DMA_MAGIC, 10, unsigned long)

static int first_time = 1;
static FILE *dev_mem_file;
static void *arm_phys;
static void *virt;
static void *phys;
static unsigned int max_size = 100 * 1024 * 1024;
static int free_size = (int)max_size;

void* GhwAllocatorDevice::allocDevMem(u32& pa, unsigned char*& va,u32 size) {
#ifdef __ARMEL__
	if (first_time)
	{
		FILE *settings;
		if (!fd_v3d)
		{
			fd_v3d = fopen("/dev/dmaer_4k", "r+b");
			if(!fd_v3d) {
				printf("Could not open v3d device from allocDevMem");
				assert(0);
			}
		}
		first_time = 0;

		settings = fopen("/home/pi/settings", "r");

		if (settings)
		{
			printf("settings file opened ok\n");
			fscanf(settings, "%d", &max_size);

			printf("user requests %d bytes of vram\n", max_size);
			fclose(settings);
		}


		int ret;

		if ((ret = ioctl(fileno(fd_v3d), DMA_CMA_SET_SIZE, max_size >> 12)) < 0)
		{
			printf("CMA SET SIZE gave %d\n", ret);
//			assert(0);
		}

		dev_mem_file = fopen("/dev/mem", "r+b");
		if (!dev_mem_file)
		{
			printf("could not open /dev/mem\n");
			assert(0);
		}

		arm_phys = (void *)ret;
		printf("ARM phys mapping is %p\n", arm_phys);

		assert(arm_phys);
		phys = arm_phys;

		//map in the reserved piece of memory *with a virtual address that looks exactly the same as the physical address*
		virt = mmap(arm_phys, max_size,
					PROT_READ | PROT_WRITE, MAP_SHARED,
					fileno(dev_mem_file),
					(unsigned long)arm_phys);

		if (virt != arm_phys)
		{
			printf("could not map arm phys->virt\n");
//			assert(0);
		}

		printf("virt mapped in at %p\n", virt);

		memset(virt, 0xcc, max_size);

//		virt = (void *)((unsigned int)virt + 4096);

		vram = create_mspace_with_base(virt, max_size, 0);

		{
			//get some overflow memory
			unsigned char *pOverflow;
			allocDevMem(overflowPa, pOverflow, overflowSize);

			overflowPa = (overflowPa & ~0xc0000000) | 0x40000000;

			printf("overflow at %p (%08x), %d bytes\n", pOverflow, overflowPa, overflowSize);
		}
	}

//	printf("trying to alloc - virt is %p phys is %p, currently %d bytes free\n", virt, phys, free_size);

	mspace_malloc_stats(vram);

	//4k align everything
//	size = (size + 4095) & ~4095;

	va = (unsigned char *)mspace_memalign(vram, 4096, size);

	if (!va)
	{
		comp();
		va = (unsigned char *)mspace_memalign(vram, 4096, size);
	}

	if (va)
	{
		pa = (unsigned int)va;
//		pa = (u32)phys;

		printf("allocating %d bytes from %p (%p)\n", size, va, pa);
	}
	else
	{
		printf("we're out of memory! request of %d bytes\n", size);
		assert(0);
	}
	
	/*if (free_size > size)
	{
		va = (unsigned char *)virt;
		pa = (unsigned int)va;
//		pa = (u32)phys;

		printf("allocating %d bytes from %p (%p)\n", size, va, pa);

		virt = (void *)((unsigned int)virt + size);
		phys = (void *)((unsigned int)phys + size);

		free_size -= size;
	}
	else
	{
		printf("we're out of memory!\n");
		assert(0);
	}*/

	//todo sjh I don't like this...but they do it
	return new unsigned char;
#endif

#ifdef PC_BUILD
    va = new unsigned char[size];
    pa = ((u32)va) /*| 0xfaa00000*/;

    printf("allocated device mem %p->%p %p->%p\n", va, (void *)((unsigned int)va + size), pa, (void *)((unsigned int)pa + size));

    return new unsigned char;
#endif
#ifdef USE_BRALLOC
	void* bralloc_bufHandle = NULL;
	if(mMode&GhwMemAllocator::GHW_MEM_ALLOC_CACHED)
		bralloc_alloc(size,4096,BRALLOC_USAGE_HW_CACHED,&bralloc_bufHandle);
	else
		bralloc_alloc(size,4096,BRALLOC_USAGE_SW_NOT_CACHED,&bralloc_bufHandle);
	bralloc_lock(bralloc_bufHandle,BRALLOC_LOCK_HW,0,size,(void**)(&pa));
	bralloc_lock(bralloc_bufHandle,BRALLOC_LOCK_SW,0,size,(void**)(&va));
    return bralloc_bufHandle;
#endif

#ifdef USE_BMEM
	int tempSize = size;
	int pgsize = 4096;//getpagesize();
	tempSize = (tempSize + pgsize-1) & (~(pgsize - 1));
	GEMemallocwrapParams params;
	params.size = tempSize;
	params.busAddress =0;

	/* get memory linear memory buffers */
	ioctl(mFd, GEMEMALLOC_WRAP_ACQUIRE_BUFFER, &params);
	if(params.busAddress == 0)
	{
		LOGE("GhwAllocatorDevice zero linear buffer allocated %d\n",tempSize);
		return 0;
	}

	/* Map the bus address to virtual address */
	pa = (unsigned int)params.busAddress;
	va = (unsigned char *) mmap(0,tempSize , PROT_READ | PROT_WRITE,
											MAP_SHARED, mFd,
											params.busAddress);
	if(va == (unsigned char *)0xFFFFFFFF)
	{
		LOGE("GhwAllocatorDevice mmap failed");
		ioctl(mFd, GEMEMALLOC_WRAP_RELEASE_BUFFER, &pa);
		pa = 0;
		va = 0;
		return 0;
	}
    return (void *)1;
#endif
}

void GhwAllocatorDevice::freeDevMem(u32& pa, unsigned char*& va,u32 size, void* handle) {
#ifdef PC_BUILD
#ifdef __ARMEL__
	mspace_free(vram, va);
#else
    delete []va;
	delete handle;
#endif
#endif
#ifdef USE_BRALLOC
	bralloc_free(handle);
#endif

#ifdef USE_BMEM
	int tempSize = size;
	int pgsize = 4096;//getpagesize();
	tempSize = (tempSize + pgsize-1) & (~(pgsize - 1));

    munmap(va, tempSize);
    ioctl(mFd, GEMEMALLOC_WRAP_RELEASE_BUFFER, &pa);
	va = 0;
	pa = 0;
#endif

};

void GhwAllocatorDevice::protect() {
	pthread_mutex_lock(&mLock);
}

void GhwAllocatorDevice::unprotect() {
	pthread_mutex_unlock(&mLock);
}


GhwMemAllocator::~GhwMemAllocator()
{
}

GhwAllocatorImpl::GhwAllocatorImpl(u32 mode , u32 slab_size , u32 alignment)
    :mSlabSize (slab_size), mAlignment(alignment), mMode(mode), mTotalAllocSize(0)
{
//	if(mMode == GHW_MEM_ALLOC_SIMPLE) mMode = GHW_MEM_ALLOC_RETAIN_ONE;
	mDevice.setMode(mode&GHW_MEM_ALLOC_CACHED);
	mMode = mode & (~GHW_MEM_ALLOC_CACHED);
	if(mAlignment > 12) mAlignment = 12;
	if(mSlabSize == 0) mSlabSize = 1024*1024;
	count++;
}

ghw_error_e GhwAllocatorImpl::initCheck()
{
    if(mDevice.initCheck())
        return GHW_ERROR_FAIL;
    else
        return GHW_ERROR_NONE;
}

GhwAllocatorImpl::~GhwAllocatorImpl()
{
	count--;

	protect();
    GhwMemBlockNode* node = mList.getHead();
    while(node) {
        node->get()->acquire();
		mTotalAllocSize -= node->get()->getSize();
        delete node->get();
        mList.removeNode(node);
        node = mList.getHead();
    }

    mSlabSize = 0 ;
    mAlignment = 0 ;
	mMode = 0 ;
    mTotalAllocSize = 0 ;

	unprotect();
}

GhwMemHandle* GhwAllocatorImpl::alloc(u32 size, u32 alignment )
{
    if((alignment > 12) || (size == 0)) return NULL;

    protect();
    if(alignment < mAlignment) alignment = mAlignment;

    GhwMemBlockNode* node = mList.getHead();
    while(node) {
        GhwMemBlock* handle = node->get();
        GhwMemHandle* mem = handle->alloc(size,alignment);
        if(mem) {
			unprotect();
			return mem;
		}
        node = node->getNext();
    }

    u32 alignsize = size ;
    if(mMode != GHW_MEM_ALLOC_SIMPLE) {
		if(alignsize > mSlabSize) {
			u32 factor = (alignsize+mSlabSize-1)/mSlabSize;
			alignsize = factor*mSlabSize;
		}
		else {
			alignsize = mSlabSize;
		}
	}
    GhwMemBlock* handle = new GhwMemBlock(this,mDevice,alignsize);
    if(handle && ( handle->initCheck() == 0)) {
		mTotalAllocSize += alignsize;
        handle->setNode(mList.addElement(handle,mList.getCount()));
        GhwMemHandle* mem = handle->alloc(size,alignment);
        handle->release();
		unprotect();
        return mem;
    }
	unprotect();
    return NULL;
}

ghw_error_e GhwAllocatorImpl::free(GhwMemHandle* aHandle)
{
    aHandle->release();
    return GHW_ERROR_NONE;
}

ghw_error_e GhwAllocatorImpl::reset()
{
	protect();
    GhwMemBlockNode* node = mList.getHead();
    switch (mMode) {
    case GHW_MEM_ALLOC_RETAIN_ALL:
		{
    while(node) {
        node->get()->acquire();
        node->get()->reset();
        node->get()->release();
        node = node->getNext();
        }
        break;
        }
	default:
		{
			while(node) {
				if((mMode == GHW_MEM_ALLOC_RETAIN_ONE) && (mList.getCount() == 1)) {
					node->get()->acquire();
					node->get()->reset();
					node->get()->release();
					break;
				}
				node->get()->acquire();
				mTotalAllocSize -= node->get()->getSize();
				delete node->get();
				mList.removeNode(node);
				node = mList.getHead();
				}
			break;
		}
	}
	unprotect();
    return GHW_ERROR_NONE;
}

void GhwAllocatorImpl::notifyParent(GhwMemBlock* handle)
{
	if(mMode == GHW_MEM_ALLOC_RETAIN_ALL) return;
	if((mMode == GHW_MEM_ALLOC_RETAIN_ONE) && (mList.getCount() == 1)) return;

	handle->acquire();
	mTotalAllocSize -= handle->getSize();
    mList.removeNode(handle->getNode());
    delete handle;
}


ghw_error_e GhwAllocatorImpl::virt2phys(u32& ipa_addr, void* virt_addr)
{
	protect();
	unsigned char* addrin = (unsigned char*) virt_addr;
	GhwMemBlockNode* node = mList.getHead();
	while(node) {
		u32 ipa,size;
		unsigned char* addr;
		node->get()->lock(ipa,(void*&)addr,size);
		if((addrin > addr) && (addrin < (addr +size)) ) {
			ipa_addr = ipa + ((unsigned int) (addrin-addr));
			node->get()->unlock();
			unprotect();
			return GHW_ERROR_NONE;
		}
		node->get()->unlock();
		node = node->getNext();
	}

	unprotect();
	return GHW_ERROR_FAIL;
}
ghw_error_e GhwAllocatorImpl::phys2virt(u32 ipa_addr, void*& virt_addr)
{
	protect();
	unsigned int addrin = (unsigned int) ipa_addr;
	GhwMemBlockNode* node = mList.getHead();
	while(node) {
		u32 ipa,size;
		unsigned char* addr;
		node->get()->lock(ipa,(void*&)addr,size);
		if((addrin > ipa) && (addrin < (ipa +size)) ) {
			virt_addr = (void*) ( addr + ((unsigned int) (addrin-ipa)));
			node->get()->unlock();
			unprotect();
			return GHW_ERROR_NONE;
		}
		node->get()->unlock();
		node = node->getNext();
	}

	unprotect();
	return GHW_ERROR_FAIL;
}


void GhwAllocatorImpl::protect()
{
	mDevice.protect();
}

void GhwAllocatorImpl::unprotect()
{
	mDevice.unprotect();
}

ghw_error_e GhwAllocatorImpl::dump(u32 level )
{
	protect();

	LOGD("GhwAllocatorImpl Static Counters %d %d %d\n",GhwAllocatorImpl::count,GhwMemBlock::count,GhwAllocatorDevice::count);

	u32 maxFree = 0, totalFree = 0;

	GhwMemBlockNode* node = mList.getHead();
	while(node) {
		totalFree += node->get()->getFreeSize();
		u32 blockMax = node->get()->getMaxFreeSize();
		if(blockMax > maxFree) maxFree = blockMax;
		node = node->getNext();
	}

	LOGD("GhwAllocatorImpl[%x] totalDeviceAllocSize[%d] totalFree[%d] maxFree[%d] in numSlabs[%d]\n",
									this,mTotalAllocSize,totalFree,maxFree,mList.getCount());
	if (level) {
		level--;
		GhwMemBlockNode* node = mList.getHead();
		while(node) {
			node->get()->dump(level);
			node = node->getNext();
		}
	}
	unprotect();
	return GHW_ERROR_NONE;
}
};
