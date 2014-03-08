/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <list>

extern "C" {
#include "rtos_common_mem.h"
#include "rtos_common_mem_priv.h"
unsigned int khrn_hw_addr(const void *addr);
void *khrn_hw_unaddr(uint32_t addr);

//sjh
void mem_set_desc(MEM_HANDLE_T handle, const char *desc)
{
}

}

#include "list.h"
#include "ghw_allocator.h"

//#define FILE_DUMP

class MemHeader;
using namespace ghw;
GhwMemAllocator* mem_allocator;
GhwMemAllocator* mem_allocator1;

typedef List<MemHeader*> HeaderList;
typedef Node<MemHeader*> HeaderNode;
HeaderList mem_list ;
MEM_HANDLE_T MEM_EMPTY_STRING_HANDLE;
MEM_HANDLE_T MEM_ZERO_SIZE_HANDLE;
FILE* fp_dump = NULL;

#define FP_DUMP(a,b) fprintf(fp_dump,"%s %x %x %x\n",__FUNCTION__,a,b)

class MemHeader {
public:
	MemHeader(ghw::GhwMemHandle* handle,uint32_t size,uint32_t align,MEM_FLAG_T flags):mHandle(handle),mAlign(align),mFlags(flags),ref_count(1),retain_count(0),lock_count(0)
	{
		mTerm = NULL;
		mSize = size;
		mNode = NULL;
	}
	int resize(uint32_t size);
	void acquire() {
		if(mHandle) mHandle->acquire();
		ref_count++;
		}
	void release() {
		ref_count--;
		ghw::GhwMemHandle* handle = mHandle;
		if(ref_count == 0) {
			if(mTerm) mTerm(getVirt(),mSize);
			delete this;
			}
		if(handle) handle->release();
		}
	void retain(){ retain_count++;}
	void unretain(){ retain_count--;}
	void lock(){
		ghw::u32 size=0,pa=0;
		void* va =0;
		if(mHandle) mHandle->lock(pa,va,size);
		lock_count++;
		}
	void unlock() {
		if(mHandle) mHandle->unlock();
		lock_count--;
		}

	void setTerm(MEM_TERM_T term ) { mTerm = term;}
	MEM_TERM_T getTerm(){return mTerm;}

	void setNode(HeaderNode* node){ mNode = node;}
	void setHandle(GhwMemHandle* handle, uint32_t new_size)
	{
		int i;
		GhwMemHandle* old_handle = mHandle;
		ghw::u32 size=0,pa=0;
		void* va =0;
		mHandle = handle;
		if (mHandle) {
				{
					void* va1;
					mHandle->lock(pa,va,size);
					old_handle->lock(pa,va1,size);
					memcpy(va,va1,size);
					mHandle->unlock();
					old_handle->unlock();
			}
			for (i = 0; i < lock_count; i++) {
				mHandle->lock(pa, va, size);
				old_handle->unlock();
			}	

			for (i = 0; i < ref_count; i++) {
				old_handle->release();
			}	

			for (i = 1; i < ref_count; i++) {
				mHandle->acquire();
			}
		}
		mSize = new_size;
	}

	uint32_t getRefCnt() { return ref_count;}
	uint32_t getLockCnt(){ return lock_count;}
	uint32_t getRetainCnt(){ return retain_count;}
	uint32_t getPhys() {
		ghw::u32 size =0,pa=0;
		void* va =0;
		if(mHandle) {
			mHandle->lock(pa,va,size);
			mHandle->unlock();
			}
		return (uint32_t)pa;
		}
	void* getVirt(){
		ghw::u32 size =0,pa=0;
		void* va =0;
		if(mHandle) {
			mHandle->lock(pa,va,size);
			mHandle->unlock();
			}
		return va;
		}
	uint32_t getAlign(){ return mAlign;}
	uint32_t getSize(){ return mSize;}
	MEM_FLAG_T getFlags(){ return mFlags;}

private:
	virtual ~MemHeader() {
		if(mNode) mem_list.removeNode(mNode);
		mSize = 0;
		mAlign =0;
		mFlags =MEM_FLAG_NONE;
		mTerm = NULL;
		mHandle = NULL;
		ref_count =0;
		retain_count=0;
		lock_count =0;
		mNode = NULL;
		}
   uint32_t mSize;
   uint32_t mAlign;
   MEM_FLAG_T mFlags;
   MEM_TERM_T mTerm;
   HeaderNode* mNode;

   ghw::GhwMemHandle *mHandle;

   uint32_t ref_count;
   uint32_t retain_count;
   uint32_t lock_count;

};

static std::list<MemHeader *> *g_pAllocs = 0;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class GhwMemHandleMalloc : public GhwMemHandle {
		public:
			GhwMemHandleMalloc(u32 size);
			virtual ~GhwMemHandleMalloc();
			ghw_error_e lock(u32& phys, void*& virt, u32& size);
			ghw_error_e unlock();
			ghw_error_e acquire();
			ghw_error_e release();
			ghw_error_e setName(const char* name);
			ghw_error_e dump(u32 level);
		private:
			u32 refCnt;
			u32 lockCnt;
			void* virtAddr;
			u32 mSize;
	};

	GhwMemHandleMalloc::GhwMemHandleMalloc(u32 size) {
		virtAddr = (void*)new char[size];
		refCnt = 1;
		mSize = size;
	}

	GhwMemHandleMalloc::~GhwMemHandleMalloc() {
		delete [] virtAddr;
		
		refCnt =0;
		lockCnt =0;
		virtAddr = NULL;
		mSize = 0;
	}
	ghw_error_e GhwMemHandleMalloc::lock(u32& phys, void*& virt, u32& size) {
		phys = 0;
		virt = virtAddr;
		size = mSize;

		lockCnt++;

		return GHW_ERROR_NONE;
	}

	ghw_error_e GhwMemHandleMalloc::unlock() {
		lockCnt--;
		return GHW_ERROR_NONE;
	}

	ghw_error_e GhwMemHandleMalloc::acquire() {
		refCnt++;
		return GHW_ERROR_NONE;
	}

	ghw_error_e GhwMemHandleMalloc::release() {
		refCnt--;

		if (refCnt == 0) {
			delete this;
		}
		return GHW_ERROR_NONE;
	}

	ghw_error_e GhwMemHandleMalloc::setName(const char* name){
		return GHW_ERROR_NONE;
	}

	ghw_error_e GhwMemHandleMalloc::dump(u32 level) {
		return GHW_ERROR_NONE;
	}

	class GhwMemHandleWrap : public GhwMemHandle {
		public:
			GhwMemHandleWrap(u32 size, u32 phys, void* virt);
			virtual ~GhwMemHandleWrap();
			ghw_error_e lock(u32& phys, void*& virt, u32& size);
			ghw_error_e unlock();
			ghw_error_e acquire();
			ghw_error_e release();
			ghw_error_e setName(const char* name);
			ghw_error_e dump(u32 level);
		private:
			u32 physAddr;
			u32 refCnt;
			u32 lockCnt;
			void* virtAddr;
			u32 mSize;
	};


	GhwMemHandleWrap::GhwMemHandleWrap(u32 size, u32 phys, void* virt) {
		virtAddr = virt;
		physAddr = phys;
		mSize = size;
		refCnt = 1;
	}

	GhwMemHandleWrap::~GhwMemHandleWrap() {
		return;
	}

	ghw_error_e GhwMemHandleWrap::lock(u32& phys, void*& virt, u32& size) {
		phys = physAddr;
		virt = virtAddr;
		size = mSize;

		lockCnt++;

		return GHW_ERROR_NONE;
	}

	ghw_error_e GhwMemHandleWrap::unlock() {
		lockCnt--;

		return GHW_ERROR_NONE;
	}

	ghw_error_e GhwMemHandleWrap::acquire() {
		refCnt++;

		return GHW_ERROR_NONE;
	}

	ghw_error_e GhwMemHandleWrap::release() {
		refCnt--;
		if (refCnt == 0) {
			delete this;
		}
		return GHW_ERROR_NONE;
	}

	ghw_error_e GhwMemHandleWrap::setName(const char* name){
		return GHW_ERROR_NONE;
	}

	ghw_error_e GhwMemHandleWrap::dump(u32 level) {
		return GHW_ERROR_NONE;
	}





void rtos_common_dump()
{
	sleep(1);
	mem_allocator->dump(1);
	sleep(1);
}

void mem_get_default_partition(void **mempool_base, uint32_t *mempool_size, void **mempool_handles_base, uint32_t *mempool_handles_size)
{
	return;
}

int mem_init(void *mempool_base, uint32_t mempool_size, void *mempool_handles_base, uint32_t mempool_handles_size)
{
	/* open the new allocator instance and initialize */
	mem_allocator = GhwMemAllocator::create(GhwMemAllocator::GHW_MEM_ALLOC_RETAIN_NONE, 4*1024*1024, 4);
	mem_allocator1 = GhwMemAllocator::create(GhwMemAllocator::GHW_MEM_ALLOC_RETAIN_NONE, 1*1024*1024, 4);
			
	if (mem_allocator == NULL) {
		LOGE("GhwMemAllocator::create failed \n");
		return 0;
	}

	GhwMemHandle* handle = new GhwMemHandleMalloc(1);
	MemHeader *header = new MemHeader(handle,1,1,MEM_FLAG_NONE);

	MEM_EMPTY_STRING_HANDLE = (void*)header;

	header->lock();
	memset(header->getVirt(), '\0', 1);
	header->unlock();

	handle = new GhwMemHandleMalloc(0);
	header = new MemHeader(handle, 0, 1, MEM_FLAG_NONE);

	MEM_ZERO_SIZE_HANDLE = (void*)header;

#ifdef FILE_DUMP
	fp_dump = fopen("/data/dump.txt","w");
#endif

   return 1;
}

int32_t rtos_common_mem_init(void) {
	int32_t success = 0;
	uint32_t pool_size, handles_size;
	void *base, *handles_base;

	if ((success >= 0) &&
		!mem_init(base, pool_size, handles_base, handles_size))
		success = -1;

   return success;
}

void mem_term(void) {
#ifdef FILE_DUMP	
	fclose(fp_dump);
#endif
}

void mem_compact(mem_compact_mode_t mode) {
#ifdef FILE_DUMP	
	fprintf(fp_dump, "mem_compact %x\n", mode);
#endif
	int changed;

	do
	{
		std::list<MemHeader *>::iterator it;
		changed = 0;

		for (it = g_pAllocs->begin(); it != g_pAllocs->end(); it++)
			if (*it && (*it)->getRefCnt() == 1 && (*it)->getLockCnt() == 0 && (*it)->getRetainCnt() == 0)
			{
				MemHeader *h = *it;
				g_pAllocs->remove(h);

				while (h->getRefCnt())
					h->release();

				changed = 1;
				break;
			}
	} while (changed == 1);

	return;
}

extern "C" void comp(void)
{
//	mem_compact(MEM_COMPACT_ALL);
}

MEM_HANDLE_T mem_alloc_ex(uint32_t size, uint32_t align, MEM_FLAG_T flags, const char *desc, mem_compact_mode_t mode)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_alloc_ex %x %x %x %x %x\n", size, align, flags, desc, mode);
#endif
	u32 ipa_add, temp_size;
	void *virt_add;
	int align_count = 0;
	int temp_align = align;
	
	GhwMemHandle *handle = NULL;
	
	if (temp_align > 0) {
		while (temp_align >>= 1) {
			align_count++;
		}
	}	

	if (size != 0) {
		if (flags == MEM_FLAG_NONE) {
			handle = new GhwMemHandleMalloc(size);
		} else {
			if (size <= (128*1024)) {
				handle = mem_allocator1->alloc(size, align_count);
				if (handle == NULL) {
					handle = mem_allocator->alloc(size, align_count);
				}	
			} else {
				handle = mem_allocator->alloc(size, align_count);
			}	
		}
	} else if (size == 0) {
		return MEM_ZERO_SIZE_HANDLE;
	}
	
	MemHeader *header = new MemHeader(handle,size,align,flags);

	if (g_pAllocs == 0)
		g_pAllocs = new std::list<MemHeader *>;

	g_pAllocs->push_back(header);


	if ((flags & MEM_FLAG_NO_INIT) == 0) {
		header->lock();
		memset(header->getVirt(), 0, size);
		header->unlock();
	}

	return (void *)header;
}

unsigned int khrn_hw_addr(const void *addr)
{
	ghw_error_e result;
#ifdef FILE_DUMP
	fprintf(fp_dump, "khrn_hwaddr %x\n", addr);
#endif
	u32 phys;
	unsigned int virt = (unsigned int) addr;
	HeaderNode* head = mem_list.getHead();

	while (head != NULL) {
		MemHeader* header = head->get();
		unsigned int va = (unsigned int)header->getVirt();
		if ( (va <= virt) && (virt <= va + header->getSize()) ) {
				unsigned int out = header->getPhys() + (virt-va);

				if (out == 0)
					printf("input of %p out of ZERO!\n", addr);

				return out;
			}
		head = head->getNext();
	}

	result = mem_allocator->virt2phys(phys, (void*)virt);
	if (result != GHW_ERROR_NONE) {
		result = mem_allocator1->virt2phys(phys, (void*)virt);
	}	

	if (phys == 0)
		printf("input of %p out of ZERO!\n", addr);

	return phys;
}

void *khrn_hw_unaddr(uint32_t addr)
{
	ghw_error_e result;
#ifdef FILE_DUMP
	fprintf(fp_dump, "khrn_hw_unaddr %x\n", addr);
#endif
	void* virt;
	unsigned int phys = (unsigned int) addr;
	HeaderNode* head = mem_list.getHead();

	while (head != NULL) {
		MemHeader* header = head->get();
		unsigned int pa = (unsigned int)header->getPhys();
		if ( (pa <= phys) && (phys <= pa + header->getSize()) ) {
				return ((unsigned char*)header->getVirt()) + (phys-pa);
			}
		head = head->getNext();
	}

	result = mem_allocator->phys2virt(phys, (void*&)virt);
	if (result != GHW_ERROR_NONE) {
		mem_allocator1->phys2virt(phys, (void*&)virt);
	}	
	return virt;
}

MEM_HANDLE_T mem_wrap(void *p, uint32_t phys,uint32_t size, uint32_t align, MEM_FLAG_T flags, const char *desc)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_wrap %x %x %x %x %x %x\n", p, phys, size, align, flags, desc);
#endif
	GhwMemHandle* handle;
	MemHeader* header;

	if((p == NULL) || (phys == NULL)) {
		return MEM_HANDLE_INVALID;
	}
	handle = (GhwMemHandle*) new GhwMemHandleWrap(size, phys, p);
	header = new MemHeader(handle,size,align,flags);	

	header->setNode(mem_list.addElement(header, 0));
	return (void*)header;
}

void mem_acquire(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_acquire %x\n", header);
#endif

	if (header == MEM_ZERO_SIZE_HANDLE) return;

	((MemHeader *)header)->acquire();
}

void mem_release(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_release %x\n", header);
#endif
	if (header == MEM_ZERO_SIZE_HANDLE) return;

	((MemHeader *)header)->release();
}


void *mem_lock(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_lock %x\n", header);
#endif
	if (header == MEM_ZERO_SIZE_HANDLE) return NULL;

	((MemHeader *)header)->lock();
	return ((MemHeader *)header)->getVirt();
	
}

void mem_unlock(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_unlock %x\n", header);
#endif
	if (header == MEM_ZERO_SIZE_HANDLE) return;

	((MemHeader *)header)->unlock();
}

void mem_abandon(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_abandon %x\n", header);
#endif
}

int mem_retain(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_retain %x\n", header);
#endif
	if (header == MEM_ZERO_SIZE_HANDLE) return 0;

	((MemHeader *)header)->retain();
	return 1;
}

void mem_unretain(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_unretain %x\n", header);
#endif
	if (header == MEM_ZERO_SIZE_HANDLE) return;

	((MemHeader *)header)->unretain();
}

#define LEGACY_BLOCK_SIZE 0x200000
uint32_t mem_get_legacy_size(void)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_get_legacy_size\n");
#endif
   return LEGACY_BLOCK_SIZE;
}

void *mem_alloc_legacy_ex(MEM_FLAG_T flags)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_alloc_legacy_ex %x\n", flags);
#endif

	GhwMemHandle *handle;
	MemHeader *header = NULL;

	handle = mem_allocator->alloc(LEGACY_BLOCK_SIZE, 12);
	header = new MemHeader(handle,LEGACY_BLOCK_SIZE,12,flags);	

	header->setNode(mem_list.addElement(header,0));

	return header->getVirt();
}


void mem_free_legacy(void *ptr)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_free_legacy %x\n", ptr);
#endif
	u32 phys;
	unsigned int virt = (unsigned int) ptr;
	HeaderNode* head = mem_list.getHead();
	while (head != NULL) {
		MemHeader* header = head->get();
		unsigned int va = (unsigned int)header->getVirt();
		if ( (va <= virt) && (virt <= va + header->getSize()) ) {
				header->release();
				break;
			}
		head = head->getNext();
	}
}

int mem_init_legacy(uint32_t size, uint32_t align, int count_max)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_init_legacy %x %x %x\n", size, align, count_max);
#endif
	return 32;
}

void mem_validate(void)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_validate\n");
#endif

	return;
}

uint32_t mem_get_size(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_get_size header %x\n", header);
#endif
	if (header == MEM_ZERO_SIZE_HANDLE) return 0;

	return ((MemHeader*)header)->getSize();
}

int mem_resize_ex(MEM_HANDLE_T handle, uint32_t size, mem_compact_mode_t mode)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_resize_ex %x %x %x\n", handle, size, mode);
#endif
	GhwMemHandle* new_handle;

	MemHeader* header = (MemHeader*)handle;
	int temp_align = header->getAlign();
	int align_count = 0;

	if (temp_align > 0) {
		while (temp_align >>= 1) {
			align_count++;
		}
	}	

	if (size != 0) {
		if ((header->getFlags()) == MEM_FLAG_NONE) {
			new_handle = new GhwMemHandleMalloc(size);
		} else {
			if (size <= (128*1024)) {
				new_handle = mem_allocator1->alloc(size, align_count);
				if (new_handle == NULL) {
					new_handle = mem_allocator->alloc(size, align_count);
				}
			} else {
				new_handle = mem_allocator->alloc(size, align_count);
			}	
		}
	}

	header->setHandle(new_handle, size);

	return 1;
}



void mem_lock_multiple_phys(void **pointers, MEM_HANDLE_OFFSET_T *handles, uint32_t n)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_lock_multiple_phys %x %x %x\n", pointers, handles, n);
#endif
	uint32_t i, offset;
	u32 ipa_addr, size;
	void *virt_addr;
   MemHeader* header;

	for (i = 0; i < n; i++) {
		header = (MemHeader*)handles[i].mh_handle;
		offset = handles[i].offset;
      if (header == MEM_HANDLE_INVALID)
         pointers[i] = (void*)offset;
	  else {
	  	header->lock();
		pointers[i] = (char*)(header->getPhys() + offset);
	  	}
   }
	return;
}


void mem_unlock_unretain_release_multiple(MEM_HANDLE_OFFSET_T *handles, uint32_t n)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_unlock_unretain_release_multiple %x %x\n", handles, n);
#endif
	int i;

	for (i = 0; i < n; i++) {
		MemHeader* header = (MemHeader*)handles[i].mh_handle;
		if ( header != MEM_ZERO_SIZE_HANDLE) {
			header->unlock();
			header->unretain();
			header->release();
			}
	}
	return;
}

void mem_acquire_retain(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_acquire_retain %x\n", header);
#endif
	if (header == MEM_ZERO_SIZE_HANDLE) return;

	((MemHeader *)header)->acquire();
	((MemHeader *)header)->retain();
}

MEM_TERM_T mem_get_term(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_get_term %x\n", header);
#endif
	if (header == MEM_ZERO_SIZE_HANDLE) return NULL;

      return ((MemHeader *)header)->getTerm();
}

void mem_set_term(MEM_HANDLE_T header, MEM_TERM_T term)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_set_term %x %x\n", header, term);
#endif
	if (header == MEM_ZERO_SIZE_HANDLE) return;

    ((MemHeader *)header)->setTerm(term);
}

uint32_t mem_get_align(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_get_align %x\n", header);
#endif
	if (header == MEM_ZERO_SIZE_HANDLE) return 0;

      return ((MemHeader *)header)->getAlign();
}

MEM_FLAG_T mem_get_flags(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_get_flags %x\n", header);
#endif
	if (header == MEM_ZERO_SIZE_HANDLE) return MEM_FLAG_NONE;

      return ((MemHeader *)header)->getFlags();
}

void mem_set_user_flag(MEM_HANDLE_T handle, int flag)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_set_user_flag %x %x\n", handle, flag);
#endif
}

uint32_t mem_get_free_space(void)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_get_free_space\n");
#endif

   uint32_t freespace = 1000;

   return freespace;
}

MEM_HANDLE_T mem_strdup_ex(const char *str, mem_compact_mode_t mode)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_strdup_ex %x %x\n", str, mode);
#endif

   MEM_HANDLE_T handle = mem_alloc_ex((uint32_t)strlen(str) + 1, 1, MEM_FLAG_NONE, "mem_strdup", mode);
   if (handle == MEM_HANDLE_INVALID) {
      return MEM_HANDLE_INVALID;
   }
   strcpy((char *)mem_lock(handle), str);
   mem_unlock(handle);
   return handle;
}

uint32_t mem_get_ref_count(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_get_ref_count %x\n", header);
#endif
	if (header == MEM_ZERO_SIZE_HANDLE) return 0;

      return ((MemHeader *)header)->getRefCnt();
}

int mem_try_release(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_try_release %x\n", header);
#endif
	if (header == MEM_ZERO_SIZE_HANDLE) return 1;

      uint32_t refcnt = ((MemHeader *)header)->getRefCnt();
      if (refcnt == 1) {
         return 0;
      }
	  ((MemHeader *)header)->release();

   return 1;
}

void mem_lock_multiple(void **pointers, MEM_HANDLE_OFFSET_T *handles, uint32_t n)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_lock_multiple %x %x %x\n", pointers, handles, n);
#endif
	uint32_t offset,i;
	MemHeader* header;
	for (i = 0; i < n; i++) {
		header = (MemHeader*)handles[i].mh_handle;
		offset = handles[i].offset;
		if (header == MEM_HANDLE_INVALID) {
			pointers[i] = (void*)offset;
			} else {
			header->lock();
			pointers[i] = (char*)((int)header->getVirt()+ offset);
			}
	   }
}

void* mem_lock_perma(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_lock_perma %x\n", header);
#endif
	return NULL;
}

void mem_unlock_perma(MEM_HANDLE_T header)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_unlock_perma %x\n", header);
#endif
}

void mem_unlock_multiple(MEM_HANDLE_OFFSET_T *handles, uint32_t n)
{
#ifdef FILE_DUMP
	fprintf(fp_dump, "mem_unlock_multiple %x %x\n", handles, n);
#endif
	uint32_t i, offset;
	MemHeader* header;
	for (i = 0; i < n; i++) {
		header = (MemHeader*)handles[i].mh_handle;
		if (header != MEM_HANDLE_INVALID) {
			header->unlock();
			}
		}
}
