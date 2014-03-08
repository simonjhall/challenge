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
#define PC_BUILD
#include "ghw_memblock.h"

namespace ghw {


class GhwMemSubBlock  : public GhwMemHandleImpl {
public:
    GhwMemSubBlock(u32 offset , u32 size, GhwMemBlock* parent)
        : mParent(parent),mNode(NULL),mFreeNode(NULL),mOffset(offset),mSize(size),mUsed(0){
            mParent->acquire();
			count++;
    };
    ~GhwMemSubBlock() {
        mParent->release();
		count--;
    };
    GhwMemBlock* getParent() { return mParent;};
    GhwMemSubBlock* split(u32 size, u32 align) {
        GhwMemSubBlock* temp = NULL;
        if(mSize> size) temp = new GhwMemSubBlock(mOffset + size, mSize - size, mParent);
        mSize = size;
        mAlign = align;
        mUsed = 1;
        return temp;
    };

    void merge(GhwMemSubBlock* handle) {
        if(handle) {
            if((mOffset+mSize) == handle->mOffset) {
                mSize += handle->mSize;
                delete handle;
            }
        }
		mAlign = 0 ;
        mUsed = 0;
    };
    void setNode(GhwMemSubBlockNode* node) {mNode = node;};
    GhwMemSubBlockNode* getNode() { return mNode;};

    void setFreeNode(GhwMemSubBlockNode* node){ mFreeNode = node; };
    GhwMemSubBlockNode* getFreeNode() { return mFreeNode; };

    u32 getOffset() {return mOffset;};
    u32 getSize() {return mSize;};
    u32 isUsed() { return mUsed;};

    virtual    ghw_error_e dump(u32 level = 0){
        LOGD("\t\tGhwMemSubBlock[%x] offset[%08x] size[%08x] used[%d]\n",this,mOffset,mSize,mUsed);
        return GHW_ERROR_NONE;};
	static int count;

protected:
    virtual void handleRelease() {
        mParent->notifyParent(this);
    };
    virtual void handleLock(u32& ipa_addr, unsigned char*& virt_addr, u32& size) {
        mParent->lock(ipa_addr,(void*&)virt_addr,size);
        ipa_addr+= mOffset;
        virt_addr += mOffset;
        size = mSize;
    };
    virtual void handleUnlock() {};

private:
    GhwMemBlock* mParent;
    GhwMemSubBlockNode* mNode;
    GhwMemSubBlockNode* mFreeNode;

    u32 mOffset;
    u32 mSize;
    u32 mAlign;
    u32 mUsed;
};

#define ADD_TO_FULL_LIST(handle) handle->setNode(mList.addElement(handle,handle->getOffset()))
#define ADD_TO_FREE_LIST(handle) handle->setFreeNode(mFreeList.addElement(handle,handle->getSize()))

GhwMemHandle::~GhwMemHandle()
{
}
int GhwMemBlock::count = 0;
int GhwMemSubBlock::count = 0;

GhwMemBlock::GhwMemBlock(GhwAllocatorImpl* parent,GhwAllocatorDevice& device, u32 size)
    :mParent(parent),mDevice(device),mAllocSize(0),mSize(size),ipa_addr(0),v_addr(0),mNode(NULL),mHandle(NULL)
{
	count++;
    mHandle = mDevice.allocDevMem(ipa_addr,v_addr,mSize);
    if(mHandle) {
        GhwMemSubBlock* element = new GhwMemSubBlock(0,mSize,this);
        ADD_TO_FULL_LIST(element);
        ADD_TO_FREE_LIST(element);
    }
}

GhwMemBlock::~GhwMemBlock()
{
	count--;
    GhwMemSubBlockNode* node = mFreeList.getHead();
    while(node) {
        node->get()->setFreeNode(NULL);
        mFreeList.removeNode(node);
        node = mFreeList.getHead();
    }

    node = mList.getHead();
    while(node) {
        node->get()->setNode(NULL);
        delete node->get();
        mList.removeNode(node);
        node = mList.getHead();
    }

    mDevice.freeDevMem(ipa_addr,v_addr,mSize,mHandle);
    mParent = NULL ;
    mAllocSize = 0 ;
    mSize= 0 ;
    ipa_addr = 0;
    v_addr = 0 ;
    mNode = NULL;
    mHandle = NULL;
}

void GhwMemBlock::reset()
{
    GhwMemSubBlockNode* node = mFreeList.getHead();
    while(node) {
        node->get()->setFreeNode(NULL);
        mFreeList.removeNode(node);
        node = mFreeList.getHead();
    }

    node = mList.getHead();
    while(node) {
        node->get()->setNode(NULL);
        delete node->get();
        mList.removeNode(node);
        node = mList.getHead();
    }
	mAllocSize = 0;
	GhwMemSubBlock* element = new GhwMemSubBlock(0,mSize,this);
    ADD_TO_FULL_LIST(element);
    ADD_TO_FREE_LIST(element);

}

int GhwMemBlock::initCheck()
{
    if(ipa_addr)
        return 0;
    else
        return -1;
}

GhwMemHandle* GhwMemBlock::alloc(u32 size, u32 align)
{
    u32 alignsize = size + ( 1<< align) -1;
    u32 alignmask = (1 << align) -1;
    GhwMemSubBlockNode* node = mFreeList.getTail();
    if(node &&  (node->getKey() >= size) ) {
        node = mFreeList.getHead();
        while(node->getKey() < size) { node = node->getNext(); }
        while(node) {
            u32 offset =0;

            GhwMemSubBlock* handle1 = node->get();
            offset = handle1->getOffset();
            offset = offset & alignmask;
            if (offset) offset = alignmask + 1 - offset;

			if((offset +size ) <= handle1->getSize()) {
                mFreeList.removeNode(handle1->getFreeNode());
				if(offset) {
                    GhwMemSubBlock* handle2 = handle1->split(offset,align);
                    handle1->merge(NULL);
                    ADD_TO_FREE_LIST(handle1);
                    ADD_TO_FULL_LIST(handle2);
                    handle1 = handle2;
                }
				if(handle1->getSize() > size) {
                    GhwMemSubBlock* handle2 = handle1->split(size,align);
                    ADD_TO_FREE_LIST(handle2);
                    ADD_TO_FULL_LIST(handle2);
                }
                else {
                    GhwMemSubBlock* handle2 = handle1->split(size,align);
                }
				mAllocSize += handle1->getSize();
                return handle1;
            }

			node = node->getNext();
        }

    }
    return NULL;
}

void GhwMemBlock::notifyParent(GhwMemSubBlock* handle)
{
	GhwAllocatorImpl* parent = mParent;
	parent->protect();
    GhwMemSubBlockNode* node = handle->getNode();
    handle->merge(NULL);
    handle->acquire();
	mAllocSize -= handle->getSize();

    GhwMemSubBlockNode* nextnode = node->getNext();
    if(nextnode) {
        GhwMemSubBlock* nexthandle = nextnode->get();
        if(nexthandle->isUsed() == 0) {
            mList.removeNode(nexthandle->getNode());
            mFreeList.removeNode(nexthandle->getFreeNode());
            nextnode = NULL;
            handle->merge(nexthandle);
        }
    }

    GhwMemSubBlockNode* prevnode = node->getPrev();
    if(prevnode) {
        GhwMemSubBlock* prevhandle = prevnode->get();
        if(prevhandle->isUsed() == 0) {
            mList.removeNode(node);
            mFreeList.removeNode(prevhandle->getFreeNode());
            prevhandle->setFreeNode(NULL);
            prevhandle->merge(handle);
            handle = prevhandle;
            node = prevnode;
        }
    }
    ADD_TO_FREE_LIST(handle);

    if((mList.getCount() == 1) && (mFreeList.getCount() == 1)) {
		parent->notifyParent(this);
    }
	parent->unprotect();
}

void GhwMemBlock::handleLock(u32& ipa_addr, unsigned char*& virt_addr, u32& size)
{
    ipa_addr = this->ipa_addr;
    virt_addr = this->v_addr;
    size = mSize;
}
void GhwMemBlock::handleUnlock()
{
}

u32 GhwMemBlock::getMaxFreeSize() {
	GhwMemSubBlockNode* node =  mFreeList.getTail();
	if(node) {
		return node->get()->getSize();
	}
	else
		return 0;
	};

ghw_error_e GhwMemBlock::dump(u32 level )
{
	u32 maxFreeSize = 0;
	if (mFreeList.getTail()) maxFreeSize = mFreeList.getTail()->get()->getSize();

    LOGD("\tGhwMemBlock[%x] pa[0x%08x] va[0x%08x] size[%d] alloc[%d] free[%d] maxFree[%d] refCnt[%d] fullList[%d] freeList[%d] \n",
					this, ipa_addr,v_addr,mSize, mAllocSize, mSize-mAllocSize, maxFreeSize, getRefCnt(), mList.getCount(), mFreeList.getCount());
	if(level) {
		level--;
		LOGD("\tFull List\n");
		GhwMemSubBlockNode* node = mList.getHead();
		while(node) {
			node->get()->dump(level);
			node = node->getNext();
		}

		LOGD("\tFree List\n");
		node = mFreeList.getHead();
		while(node) {
			node->get()->dump(level);
			node = node->getNext();
		}
	}
    return GHW_ERROR_NONE;
}


};
