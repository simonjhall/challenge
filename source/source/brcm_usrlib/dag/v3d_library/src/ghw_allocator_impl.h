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
#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_

#include "ghw_allocator.h"
//#include <typeinfo>
extern "C" {
#include <pthread.h>
};

namespace ghw {

#include "list.h"

class GhwMemBlock;
//This class abstracts the platform. All Platform changes are done here
class GhwAllocatorDevice {
public:
    GhwAllocatorDevice () ;
    ~GhwAllocatorDevice () ;
    int initCheck() ;
    void* allocDevMem(u32& pa, unsigned char*& va,u32 mSize, u32 byteAlignment = 4096) ;
    void freeDevMem(u32& pa, unsigned char*& va,u32 mSize, void* ) ;
	void protect();
	void unprotect();
	void setMode(u32 mode);
	static int count;
private:
    int mFd;
	pthread_mutex_t mLock;
	u32 mMode;
};

typedef Node<GhwMemBlock*> GhwMemBlockNode;
typedef List<GhwMemBlock*> GhwMemBlockList;

class GhwAllocatorImpl : public GhwMemAllocator {
public:
    GhwAllocatorImpl(u32 mode , u32 slab_size , u32 alignment);
    virtual ~GhwAllocatorImpl();
    ghw_error_e     initCheck();

    virtual    GhwMemHandle*   alloc(u32 size, u32 alignment = 2);
    virtual    ghw_error_e     free(GhwMemHandle*);
    virtual    ghw_error_e     reset();

    virtual    ghw_error_e     virt2phys(u32& ipa_addr, void* virt_addr);
    virtual    ghw_error_e     phys2virt(u32 ipa_addr, void*& virt_addr);

    virtual    ghw_error_e     setName(const char *name){ return GHW_ERROR_NONE;};
    virtual    ghw_error_e     dump(u32 level = 0);

    void notifyParent(GhwMemBlock*);
	void protect();
	void unprotect();
private:
    u32 mSlabSize;
    u32 mAlignment;
	u32 mMode;
    u32 mTotalAllocSize;
    GhwMemBlockList mList;
    GhwAllocatorDevice mDevice;
	static int count;
};

};

#endif
