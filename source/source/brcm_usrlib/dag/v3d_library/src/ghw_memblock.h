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

#ifndef __MEMBLOCK_H_
#define __MEMBLOCK_H_

#include "ghw_allocator_impl.h"

namespace ghw {
class GhwMemSubBlock;

class GhwMemHandleImpl : public GhwMemHandle {
public:
    GhwMemHandleImpl():mRefCnt(1),mLockCnt(0) {};
    virtual ~GhwMemHandleImpl() {};

    virtual    ghw_error_e acquire() {
        mRefCnt++;
        return GHW_ERROR_NONE;
    };
    virtual    ghw_error_e release() {
        mRefCnt--;
        if (mRefCnt == 0) handleRelease();
        return GHW_ERROR_NONE;
    };
    virtual    ghw_error_e lock(u32& ipa_addr, void*& virt_addr, u32& size) {
        mLockCnt++;
        handleLock( ipa_addr, (unsigned char*&)virt_addr, size);
        return GHW_ERROR_NONE;
    };
    virtual    ghw_error_e unlock() {
        mLockCnt--;
        handleUnlock();
        return GHW_ERROR_NONE;
    };
    u32 getRefCnt() { return mRefCnt;};

    virtual    ghw_error_e setName(const char *name) { return GHW_ERROR_NONE; };
protected:
    virtual void handleRelease() = 0;
    virtual void handleLock(u32& ipa_addr, unsigned char*& virt_addr, u32& size) = 0;
    virtual void handleUnlock() = 0;
private:
    unsigned int mRefCnt;
    unsigned int mLockCnt;
};

typedef Node<GhwMemSubBlock*> GhwMemSubBlockNode;
typedef List<GhwMemSubBlock*> GhwMemSubBlockList;

class GhwMemBlock : public GhwMemHandleImpl {
public:
    GhwMemBlock(GhwAllocatorImpl* parent,GhwAllocatorDevice& device, u32 size);
    ~GhwMemBlock();
    int initCheck();
	void reset();
    GhwMemHandle* alloc(u32 size, u32 align);
    u32 getSize() {return mSize;};
	u32 getFreeSize() { return mSize - mAllocSize;}
	u32 getMaxFreeSize() ;
    void setNode(GhwMemBlockNode* node) {
        mNode = node;
    };
    GhwMemBlockNode* getNode(){
        return mNode;
    };
    void notifyParent(GhwMemSubBlock*);
    virtual    ghw_error_e dump(u32 level = 0);
	static int count;
protected:
    virtual void handleRelease() {};
    virtual void handleLock(u32& ipa_addr, unsigned char*& virt_addr, u32& size) ;
    virtual void handleUnlock() ;

private:
    u32 ipa_addr;
    unsigned char* v_addr;
    u32 mSize;
	void* mHandle;

	u32 mAllocSize;
    GhwAllocatorImpl* mParent;
    GhwAllocatorDevice& mDevice;
    GhwMemBlockNode* mNode;

    GhwMemSubBlockList mList;
    GhwMemSubBlockList mFreeList;
};

};
#endif
