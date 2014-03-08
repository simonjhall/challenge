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

#ifndef _GHW_ALLOCATOR_H_
#define _GHW_ALLOCATOR_H_

#include "ghw_common.h"

namespace ghw {

/*
 * GhwMemHandle: Graphics Memory handle
 * Handle for holding a graphics buffer which will be contiguous in physical memory space (need not
 *   be physically contiguous if IOMMU is present but will be contiguous in IPA space for that case).
 *   This is a pure virtual class and client cannot create an object of this type. The graphics
 *   allocator will create this object and provide to the client. The object is ref-counted.
 * acquire(): Increments the reference count of the graphics memory buffer handle.
 * release(): Decremetns the reference count of graphics memory buffer handle. The handle and memory will
 *   be freed when reference count reaches zero.
 * lock():    Locks the buffer in memory and returns ipa, virtual addresses. The buffer will not be
 *   relocated till the buffer is unlocked.
 * unlock():  The buffer will no longer be locked in memory and the addresses got in previous lock()
 *   will no longer be valid. The buffer could get relocated in memory as part of defragmentation.
 * dump()  : Prints all information about the buffer.
 */
class GhwMemHandle {
public:
    virtual ~GhwMemHandle();

    virtual    ghw_error_e     acquire()=0;
    virtual    ghw_error_e     release()=0;
    virtual    ghw_error_e     lock(u32& ipa_addr, void*& virt_addr, u32& size)=0;
    virtual    ghw_error_e     unlock()=0;

    virtual    ghw_error_e     setName(const char *name)=0;
    virtual    ghw_error_e     dump(u32 level = 0)=0;
};

/*
 * GhwMemAllocator: Graphics Memory Allocator
 * Allocator class which supports allocating graphics memory which will be contiguous in physical memory
 *   space.
 * create(): A static function which allocates an object of the allocator. Optional parameters can be used
 *   to select the type of allocator, size of slabs to be reserved, alignment requirement of buffers
 *   mode  : Hint for the underlying allocator device
 *     GHW_MEM_ALLOC_SIMPLE:      Allocate each buffer from the device.
 *     GHW_MEM_ALLOC_RETAIN_ONE:  Use a sub-allocator. Request in slab units from device. Allocate one
 *       slab on creation of the object. Retain one slab till object gets destroyed.
 *     GHW_MEM_ALLOC_RETAIN_NONE: Use a sub-allocator. None of the slabs will be retained when all the
 *       the buffers get freed.
 *     GHW_MEM_ALLOC_RETAIN_ALL:  Use a sub-allocator. Retain all the slabs acquired from device even
 *       when all the buffers get destroyed.
 *   slab_size : The size of slab (in bytes) which will be requested from the device when more memory is needed.
 *   alignment : Alignment (in bits) needed for every buffer allocated.
 * setName() : Identifier (printed in dump).
 * alloc() : Allocate one physically contiguous buffer of requested size (bytes).
 * free()  : Free the graphics buffer. The memory will be freed and object invalidated when all the
 *   aquired handles are released.
 * reset() : Frees up all the buffers allocated using the allocator.
 * dump()  : Prints all information about the allocator and a snapshot of the allocated buffer list.
 *   level:
 *     0: Show summary
 *     1: Show summary for each slab,
 *     2: Show all the allocations
 *     3: Show all allocations and list info
 *
 * Freeing of individual buffers can be done by using the release() member function of the handle
 * returned by alloc. The same buffer can be shared within the process using acquire() member of the
 * handle. Address of the buffer could be obtained using lock() function of the handle.
 */
class GhwMemAllocator {
public:
    enum {
        GHW_MEM_ALLOC_SIMPLE           = 0x00,
        GHW_MEM_ALLOC_RETAIN_ONE       = 0x01,
        GHW_MEM_ALLOC_RETAIN_NONE      = 0x02,
        GHW_MEM_ALLOC_RETAIN_ALL       = 0x03,
		GHW_MEM_ALLOC_CACHED		   = 0x04,
    };

    static GhwMemAllocator* create(u32 mode = GHW_MEM_ALLOC_SIMPLE, u32 slab_size = 0, u32 alignment = 2);
    virtual ~GhwMemAllocator();

    virtual    GhwMemHandle*   alloc(u32 size, u32 alignment = 2)=0;
    virtual    ghw_error_e     free(GhwMemHandle*)=0;
    virtual    ghw_error_e     reset()=0;

    virtual    ghw_error_e     virt2phys(u32& ipa_addr, void* virt_addr)=0;
    virtual    ghw_error_e     phys2virt(u32 ipa_addr, void*& virt_addr)=0;

    virtual    ghw_error_e     setName(const char *name)=0;
    virtual    ghw_error_e     dump(u32 level = 0)=0;

};

};
#endif /* _GHW_ALLOCATOR_H_ */

