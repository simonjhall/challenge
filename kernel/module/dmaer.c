#include <linux/init.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/timex.h>
#include <linux/dma-mapping.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/dma.h>

#include "vc_support.h"

#ifdef ECLIPSE_IGNORE

#define __user
#define __init
#define __exit
#define __iomem
#define KERN_DEBUG
#define KERN_ERR
#define KERN_WARNING
#define KERN_INFO
#define _IOWR(a, b, c) b
#define _IOW(a, b, c) b
#define _IO(a, b) b

#endif

//#define inline

#define PRINTK(args...) printk(args)
//#define PRINTK_VERBOSE(args...) printk(args)
//#define PRINTK(args...)
#define PRINTK_VERBOSE(args...)

/***** TYPES ****/
#define PAGES_PER_LIST 500
struct PageList
{
	struct page *m_pPages[PAGES_PER_LIST];
	unsigned int m_used;
	struct PageList *m_pNext;
};

struct VmaPageList
{
	//each vma has a linked list of pages associated with it
	struct PageList *m_pPageHead;
	struct PageList *m_pPageTail;
	unsigned int m_refCount;
};

struct DmaControlBlock
{
	unsigned int m_transferInfo;
	void __user *m_pSourceAddr;
	void __user *m_pDestAddr;
	unsigned int m_xferLen;
	unsigned int m_tdStride;
	struct DmaControlBlock *m_pNext;
	unsigned int m_blank1, m_blank2;
};

struct V3dRegisterIdent
{
	volatile unsigned int m_ident0;
	volatile unsigned int m_ident1;
	volatile unsigned int m_ident2;
};
static struct V3dRegisterIdent __iomem *g_pV3dIdent = (struct V3dRegisterIdent __iomem *) IO_ADDRESS(0x20c00000);


struct V3dRegisterScratch
{
	volatile unsigned int m_scratch;
};
static struct V3dRegisterScratch __iomem *g_pV3dScratch = (struct V3dRegisterScratch __iomem *) IO_ADDRESS(0x20c00000 + 0x10);


struct V3dRegisterCache
{
	volatile unsigned int m_l2cactl;
	volatile unsigned int m_slcactl;
};
static struct V3dRegisterCache __iomem *g_pV3dCache = (struct V3dRegisterCache __iomem *) IO_ADDRESS(0x20c00000 + 0x20);


struct V3dRegisterIntCtl
{
	volatile unsigned int m_intctl;
	volatile unsigned int m_intena;
	volatile unsigned int m_intdis;
};
static struct V3dRegisterIntCtl __iomem *g_pV3dIntCtl = (struct V3dRegisterIntCtl __iomem *) IO_ADDRESS(0x20c00000 + 0x30);


struct V3dRegisterCle
{
	volatile unsigned int m_ct0cs;
	volatile unsigned int m_ct1cs;
	volatile unsigned int m_ct0ea;
	volatile unsigned int m_ct1ea;
	volatile unsigned int m_ct0ca;
	volatile unsigned int m_ct1ca;
	volatile unsigned int m_ct00ra0;
	volatile unsigned int m_ct01ra0;
	volatile unsigned int m_ct0lc;
	volatile unsigned int m_ct1lc;
	volatile unsigned int m_ct0pc;
	volatile unsigned int m_ct1pc;
	volatile unsigned int m_pcs;
	volatile unsigned int m_bfc;
	volatile unsigned int m_rfc;
};
static volatile struct V3dRegisterCle __iomem *g_pV3dCle = (struct V3dRegisterCle __iomem *) IO_ADDRESS(0x20c00000 + 0x100);


struct V3dRegisterBinning
{
	volatile unsigned int m_bpca;
	volatile unsigned int m_bpcs;
	volatile unsigned int m_bpoa;
	volatile unsigned int m_bpos;
	volatile unsigned int m_bxcf;
};
static struct V3dRegisterBinning __iomem *g_pV3dBinning = (struct V3dRegisterBinning __iomem *) IO_ADDRESS(0x20c00000 + 0x300);


struct V3dRegisterSched
{
	volatile unsigned int m_sqrsv0;
	volatile unsigned int m_sqrsv1;
	volatile unsigned int m_sqcntl;
};
static struct V3dRegisterSched __iomem *g_pV3dSched = (struct V3dRegisterSched __iomem *) IO_ADDRESS(0x20c00000 + 0x410);


struct V3dRegisterUserProgramReq
{
	volatile unsigned int m_srqpc;
	volatile unsigned int m_srqua;
	volatile unsigned int m_srqul;
	volatile unsigned int m_srqcs;
};
static struct V3dRegisterUserProgramReq __iomem *g_pV3dUpr = (struct V3dRegisterUserProgramReq __iomem *) IO_ADDRESS(0x20c00000 + 0x430);


struct V3dRegisterVpm
{
	volatile unsigned int m_vpacntl;
	volatile unsigned int m_vpmbase;
};
static struct V3dRegisterVpm __iomem *g_pV3dVpm = (struct V3dRegisterVpm __iomem *) IO_ADDRESS(0x20c00000 + 0x500);


struct V3dRegisterPcCtl
{
	volatile unsigned int m_pctrc;
	volatile unsigned int m_pctre;
};
static struct V3dRegisterPcCtl __iomem *g_pV3dPcCtl = (struct V3dRegisterPcCtl __iomem *) IO_ADDRESS(0x20c00000 + 0x670);

struct V3dRegisterDebugError
{
	volatile unsigned int m_dbge;
	volatile unsigned int m_fdbgo;
	volatile unsigned int m_fdbgb;
	volatile unsigned int m_fdbgr;
	volatile unsigned int m_fdbgs;
	volatile unsigned int dummy14;
	volatile unsigned int dummy18;
	volatile unsigned int dummy1c;
	volatile unsigned int m_errStat;
};
static struct V3dRegisterDebugError __iomem *g_pV3dDebErr = (struct V3dRegisterDebugError __iomem *) IO_ADDRESS(0x20c00000 + 0xf00);


/***** DEFINES ******/
//magic number defining the module
#define DMA_MAGIC		0xdd

//do user virtual to physical translation of the CB chain
#define DMA_PREPARE		_IOWR(DMA_MAGIC, 0, struct DmaControlBlock *)

//kick the pre-prepared CB chain
#define DMA_KICK		_IOW(DMA_MAGIC, 1, struct DmaControlBlock *)

//prepare it, kick it, wait for it
#define DMA_PREPARE_KICK_WAIT	_IOWR(DMA_MAGIC, 2, struct DmaControlBlock *)

//prepare it, kick it, don't wait for it
#define DMA_PREPARE_KICK	_IOWR(DMA_MAGIC, 3, struct DmaControlBlock *)

//not currently implemented
#define DMA_WAIT_ONE		_IO(DMA_MAGIC, 4, struct DmaControlBlock *)

//wait on all kicked CB chains
#define DMA_WAIT_ALL		_IO(DMA_MAGIC, 5)

//in order to discover the largest AXI burst that should be programmed into the transfer params
#define DMA_MAX_BURST		_IO(DMA_MAGIC, 6)

//set the address range through which the user address is assumed to already by a physical address
#define DMA_SET_MIN_PHYS	_IOW(DMA_MAGIC, 7, unsigned long)
#define DMA_SET_MAX_PHYS	_IOW(DMA_MAGIC, 8, unsigned long)
#define DMA_SET_PHYS_OFFSET	_IOW(DMA_MAGIC, 9, unsigned long)

//used to define the size for the CMA-based allocation *in pages*, can only be done once once the file is opened
#define DMA_CMA_SET_SIZE	_IOW(DMA_MAGIC, 10, unsigned long)

//used to get the version of the module, to test for a capability
#define DMA_GET_VERSION		_IO(DMA_MAGIC, 99)

#define VERSION_NUMBER 1

#define VIRT_TO_BUS_CACHE_SIZE 8

//for interaction with v3d
#define BCM_V3D_MAGIC   'V'
typedef enum {
        V3D_JOB_INVALID = 0,
        V3D_JOB_BIN,
        V3D_JOB_REND,
        V3D_JOB_BIN_REND,
        V3D_JOB_LAST
} v3d_job_type_e;

typedef enum {
        V3D_JOB_STATUS_INVALID = 0,
        V3D_JOB_STATUS_READY,
        V3D_JOB_STATUS_RUNNING,
        V3D_JOB_STATUS_SUCCESS,
        V3D_JOB_STATUS_ERROR,
        V3D_JOB_STATUS_NOT_FOUND,
        V3D_JOB_STATUS_TIMED_OUT,
        V3D_JOB_STATUS_SKIP,
        V3D_JOB_STATUS_LAST
} v3d_job_status_e;

typedef struct {
        v3d_job_type_e job_type;
        uint32_t job_id;
        uint32_t v3d_ct0ca;
        uint32_t v3d_ct0ea;
        uint32_t v3d_ct1ca;
        uint32_t v3d_ct1ea;
        uint32_t v3d_vpm_size;

        void *m_pOverspill;
        int m_overspillSize;
} v3d_job_post_t;

typedef struct {
        uint32_t job_id;
        v3d_job_status_e job_status;
        int32_t timeout;
} v3d_job_status_t;

enum {
        V3D_CMD_GET_MEMPOOL = 0x80,
        V3D_CMD_WAIT_IRQ,
        V3D_CMD_READ_REG,
        V3D_CMD_SOFT_RESET,
        V3D_CMD_TURN_ON,
        V3D_CMD_TURN_OFF,
        V3D_CMD_EARLY_SUSPEND,
        V3D_CMD_POST_JOB,
        V3D_CMD_WAIT_JOB,
        V3D_CMD_FLUSH_JOB,
        V3D_CMD_ACQUIRE,
        V3D_CMD_RELEASE,
        V3D_CMD_LAST
};

#define V3D_IOCTL_POST_JOB              _IOW(BCM_V3D_MAGIC, V3D_CMD_POST_JOB, v3d_job_post_t)
#define V3D_IOCTL_WAIT_JOB              _IOWR(BCM_V3D_MAGIC, V3D_CMD_WAIT_JOB, v3d_job_status_t)
//#define V3D_IOCTL_FLUSH_JOB             _IOW(BCM_V3D_MAGIC, V3D_CMD_FLUSH_JOB)
//#define V3D_IOCTL_ACQUIRE               _IOW(BCM_V3D_MAGIC, V3D_CMD_ACQUIRE)
//#define V3D_IOCTL_RELEASE               _IOW(BCM_V3D_MAGIC, V3D_CMD_RELEASE)

/***** FILE OPS *****/
static int Open(struct inode *pInode, struct file *pFile);
static int Release(struct inode *pInode, struct file *pFile);
static long Ioctl(struct file *pFile, unsigned int cmd, unsigned long arg);
static ssize_t Read(struct file *pFile, char __user *pUser, size_t count, loff_t *offp);
static int Mmap(struct file *pFile, struct vm_area_struct *pVma);

/***** VMA OPS ****/
static void VmaOpen4k(struct vm_area_struct *pVma);
static void VmaClose4k(struct vm_area_struct *pVma);
static int VmaFault4k(struct vm_area_struct *pVma, struct vm_fault *pVmf);

/**** DMA PROTOTYPES */
static struct DmaControlBlock __user *DmaPrepare(struct DmaControlBlock __user *pUserCB, int *pError);
static int DmaKick(struct DmaControlBlock __user *pUserCB);
static void DmaWaitAll(void);

/**** GENERIC ****/
static int __init dmaer_init(void);
static void __exit dmaer_exit(void);

/*** OPS ***/
static struct vm_operations_struct g_vmOps4k = {
	.open = VmaOpen4k,
	.close = VmaClose4k,
	.fault = VmaFault4k,
};

static struct file_operations g_fOps = {
	.owner = THIS_MODULE,
	.llseek = 0,
	.read = Read,
	.write = 0,
	.unlocked_ioctl = Ioctl,
	.open = Open,
	.release = Release,
	.mmap = Mmap,
};

/***** GLOBALS ******/
static dev_t g_majorMinor;

//tracking usage of the two files
static atomic_t g_oneLock4k = ATOMIC_INIT(1);

//device operations
static struct cdev g_cDev;
static int g_trackedPages = 0;

//dma control
static unsigned int *g_pDmaChanBase;
static int g_dmaIrq;
static int g_dmaChan;

//cma allocation
static int g_cmaHandle;

//user virtual to bus address translation acceleration
static unsigned long g_virtAddr[VIRT_TO_BUS_CACHE_SIZE];
static unsigned long g_busAddr[VIRT_TO_BUS_CACHE_SIZE];
static unsigned long g_cbVirtAddr;
static unsigned long g_cbBusAddr;
static int g_cacheInsertAt;
static int g_cacheHit, g_cacheMiss;

//off by default
static void __user *g_pMinPhys;
static void __user *g_pMaxPhys;
static unsigned long g_physOffset;

/****** CACHE OPERATIONS ********/
static inline void FlushAddrCache(void)
{
	int count = 0;
	for (count = 0; count < VIRT_TO_BUS_CACHE_SIZE; count++)
		g_virtAddr[count] = 0xffffffff;			//never going to match as we always chop the bottom bits anyway

	g_cbVirtAddr = 0xffffffff;

	g_cacheInsertAt = 0;
}

//translate from a user virtual address to a bus address by mapping the page
//NB this won't lock a page in memory, so to avoid potential paging issues using kernel logical addresses
static inline void __iomem *UserVirtualToBus(void __user *pUser)
{
	int mapped;
	struct page *pPage;
	void *phys;

	//map it (requiring that the pointer points to something that does not hang off the page boundary)
	mapped = get_user_pages(current, current->mm,
		(unsigned long)pUser, 1,
		1, 0,
		&pPage,
		0);

	if (mapped <= 0)		//error
		return 0;

	PRINTK_VERBOSE(KERN_DEBUG "user virtual %p arm phys %p bus %p\n",
			pUser, page_address(pPage), (void __iomem *)__virt_to_bus(page_address(pPage)));

	//get the arm physical address
	phys = page_address(pPage) + offset_in_page(pUser);
	page_cache_release(pPage);

	//and now the bus address
	return (void __iomem *)__virt_to_bus(phys);
}

static inline void __iomem *UserVirtualToBusViaCbCache(void __user *pUser)
{
	unsigned long virtual_page = (unsigned long)pUser & ~4095;
	unsigned long page_offset = (unsigned long)pUser & 4095;
	unsigned long bus_addr;

	if (g_cbVirtAddr == virtual_page)
	{
		bus_addr = g_cbBusAddr + page_offset;
		g_cacheHit++;
		return (void __iomem *)bus_addr;
	}
	else
	{
		bus_addr = (unsigned long)UserVirtualToBus(pUser);
		
		if (!bus_addr)
			return 0;
		
		g_cbVirtAddr = virtual_page;
		g_cbBusAddr = bus_addr & ~4095;
		g_cacheMiss++;

		return (void __iomem *)bus_addr;
	}
}

//do the same as above, by query our virt->bus cache
static inline void __iomem *UserVirtualToBusViaCache(void __user *pUser)
{
	int count;
	//get the page and its offset
	unsigned long virtual_page = (unsigned long)pUser & ~4095;
	unsigned long page_offset = (unsigned long)pUser & 4095;
	unsigned long bus_addr;

	if (pUser >= g_pMinPhys && pUser < g_pMaxPhys)
	{
		PRINTK_VERBOSE(KERN_DEBUG "user->phys passthrough on %p\n", pUser);
		return (void __iomem *)((unsigned long)pUser + g_physOffset);
	}

	//check the cache for our entry
	for (count = 0; count < VIRT_TO_BUS_CACHE_SIZE; count++)
		if (g_virtAddr[count] == virtual_page)
		{
			bus_addr = g_busAddr[count] + page_offset;
			g_cacheHit++;
			return (void __iomem *)bus_addr;
		}

	//not found, look up manually and then insert its page address
	bus_addr = (unsigned long)UserVirtualToBus(pUser);

	if (!bus_addr)
		return 0;

	g_virtAddr[g_cacheInsertAt] = virtual_page;
	g_busAddr[g_cacheInsertAt] = bus_addr & ~4095;

	//round robin
	g_cacheInsertAt++;
	if (g_cacheInsertAt == VIRT_TO_BUS_CACHE_SIZE)
		g_cacheInsertAt = 0;

	g_cacheMiss++;

	return (void __iomem *)bus_addr;
}

/***** FILE OPERATIONS ****/
static int Open(struct inode *pInode, struct file *pFile)
{
	PRINTK(KERN_DEBUG "file opening: %d/%d\n", imajor(pInode), iminor(pInode));
	
	//check which device we are
	if (iminor(pInode) == 0)		//4k
	{
		//only one at a time
		if (!atomic_dec_and_test(&g_oneLock4k))
		{
			atomic_inc(&g_oneLock4k);
			return -EBUSY;
		}
	}
	else
		return -EINVAL;
	
	//todo there will be trouble if two different processes open the files

	//reset after any file is opened
	g_pMinPhys = (void __user *)-1;
	g_pMaxPhys = (void __user *)0;
	g_physOffset = 0;
	g_cmaHandle = 0;

	return 0;
}

static int Release(struct inode *pInode, struct file *pFile)
{
	PRINTK(KERN_DEBUG "file closing, %d pages tracked\n", g_trackedPages);
	if (g_trackedPages)
		PRINTK(KERN_ERR "we\'re leaking memory!\n");
	
	//wait for any dmas to finish
	DmaWaitAll();

	//free this memory on the application closing the file or it crashing (implicitly closing the file)
	if (g_cmaHandle)
	{
		PRINTK(KERN_DEBUG "unlocking vc memory\n");
		if (UnlockVcMemory(g_cmaHandle))
			PRINTK(KERN_ERR "uh-oh, unable to unlock vc memory!\n");
		PRINTK(KERN_DEBUG "releasing vc memory\n");
		if (ReleaseVcMemory(g_cmaHandle))
			PRINTK(KERN_ERR "uh-oh, unable to release vc memory!\n");
	}

	if (iminor(pInode) == 0)
		atomic_inc(&g_oneLock4k);
	else
		return -EINVAL;

	return 0;
}

static struct DmaControlBlock __user *DmaPrepare(struct DmaControlBlock __user *pUserCB, int *pError)
{
	struct DmaControlBlock kernCB;
	struct DmaControlBlock __user *pUNext;
	void __iomem *pSourceBus, __iomem *pDestBus;
	
	//get the control block into kernel memory so we can work on it
	if (copy_from_user(&kernCB, pUserCB, sizeof(struct DmaControlBlock)) != 0)
	{
		PRINTK(KERN_ERR "copy_from_user failed for user cb %p\n", pUserCB);
		*pError = 1;
		return 0;
	}
	
	if (kernCB.m_pSourceAddr == 0 || kernCB.m_pDestAddr == 0)
	{
		PRINTK(KERN_ERR "faulty source (%p) dest (%p) addresses for user cb %p\n",
			kernCB.m_pSourceAddr, kernCB.m_pDestAddr, pUserCB);
		*pError = 1;
		return 0;
	}

	pSourceBus = UserVirtualToBusViaCache(kernCB.m_pSourceAddr);
	pDestBus = UserVirtualToBusViaCache(kernCB.m_pDestAddr);

	if (!pSourceBus || !pDestBus)
	{
		PRINTK(KERN_ERR "virtual to bus translation failure for source/dest %p/%p->%p/%p\n",
				kernCB.m_pSourceAddr, kernCB.m_pDestAddr,
				pSourceBus, pDestBus);
		*pError = 1;
		return 0;
	}
	
	//update the user structure with the new bus addresses
	kernCB.m_pSourceAddr = pSourceBus;
	kernCB.m_pDestAddr = pDestBus;

	PRINTK_VERBOSE(KERN_DEBUG "final source %p dest %p\n", kernCB.m_pSourceAddr, kernCB.m_pDestAddr);
		
	//sort out the bus address for the next block
	pUNext = kernCB.m_pNext;
	
	if (kernCB.m_pNext)
	{
		void __iomem *pNextBus;
		pNextBus = UserVirtualToBusViaCbCache(kernCB.m_pNext);

		if (!pNextBus)
		{
			PRINTK(KERN_ERR "virtual to bus translation failure for m_pNext\n");
			*pError = 1;
			return 0;
		}

		//update the pointer with the bus address
		kernCB.m_pNext = pNextBus;
	}
	
	//write it back to user space
	if (copy_to_user(pUserCB, &kernCB, sizeof(struct DmaControlBlock)) != 0)
	{
		PRINTK(KERN_ERR "copy_to_user failed for cb %p\n", pUserCB);
		*pError = 1;
		return 0;
	}

	__cpuc_flush_dcache_area(pUserCB, 32);

	*pError = 0;
	return pUNext;
}

static int DmaKick(struct DmaControlBlock __user *pUserCB)
{
	void __iomem *pBusCB;
	
	pBusCB = UserVirtualToBusViaCbCache(pUserCB);
	if (!pBusCB)
	{
		PRINTK(KERN_ERR "virtual to bus translation failure for cb\n");
		return 1;
	}

	//flush_cache_all();

	bcm_dma_start(g_pDmaChanBase, (dma_addr_t)pBusCB);
	
	return 0;
}

static void DmaWaitAll(void)
{
	int counter = 0;
	volatile int inner_count;
	volatile unsigned int cs;
	unsigned long time_before, time_after;

	time_before = jiffies;
	//bcm_dma_wait_idle(g_pDmaChanBase);
	dsb();
	
	cs = readl(g_pDmaChanBase);
	
	while ((cs & 1) == 1)
	{
		cs = readl(g_pDmaChanBase);
		counter++;

		for (inner_count = 0; inner_count < 32; inner_count++);

		asm volatile ("MCR p15,0,r0,c7,c0,4 \n");
		//cpu_do_idle();
		if (counter >= 1000000)
		{
			PRINTK(KERN_WARNING "DMA failed to finish in a timely fashion\n");
			break;
		}
	}
	time_after = jiffies;
	PRINTK_VERBOSE(KERN_DEBUG "done, counter %d, cs %08x", counter, cs);
	PRINTK_VERBOSE(KERN_DEBUG "took %ld jiffies, %d HZ\n", time_after - time_before, HZ);
}

static void RunThread(unsigned int t, unsigned int ca, unsigned int ea)
{
	volatile unsigned int *pCs = t == 0 ? &g_pV3dCle->m_ct0cs : &g_pV3dCle->m_ct1cs;
	volatile unsigned int *pCa = t == 0 ? &g_pV3dCle->m_ct0ca : &g_pV3dCle->m_ct1ca;
	volatile unsigned int *pEa = t == 0 ? &g_pV3dCle->m_ct0ea : &g_pV3dCle->m_ct1ea;

	//put to stop mode
	*pCs = 1 << 5;
	barrier();

	//kick work
	*pCa = ca;
	barrier();

	*pEa = ea;
	barrier();
}

static int HasThreadStopped(unsigned int t)
{
	volatile unsigned int *pCs = t == 0 ? &g_pV3dCle->m_ct0cs : &g_pV3dCle->m_ct1cs;

	barrier();

	if ((*pCs >> 5) & 1)
		return 0;
	else
		return 1;
}

static int HasThreadHaltedOrStalled(unsigned int t)
{
	volatile unsigned int *pCs = t == 0 ? &g_pV3dCle->m_ct0cs : &g_pV3dCle->m_ct1cs;

	if ((*pCs >> 4) & 1)
		return 1;
	else
		return 0;
}

static int HasThreadStoppedWithError(unsigned int t)
{
	volatile unsigned int *pCs = t == 0 ? &g_pV3dCle->m_ct0cs : &g_pV3dCle->m_ct1cs;

	if ((*pCs >> 3) & 1)
		return 1;
	else
		return 0;
}

static int HasBinnerRunOutOfMemory(void)
{
	volatile unsigned int *pPcs = &g_pV3dCle->m_pcs;

	barrier();

	return (*pPcs & (1 << 8)) ? 1 : 0;
}

static unsigned int GetThreadPc(unsigned int t)
{
	volatile unsigned int *pCa = t == 0 ? &g_pV3dCle->m_ct0ca : &g_pV3dCle->m_ct1ca;
	return *pCa;
}

static unsigned int GetThreadStatus(unsigned int t)
{
	volatile unsigned int *pCs = t == 0 ? &g_pV3dCle->m_ct0cs : &g_pV3dCle->m_ct1cs;
	return *pCs;
}

static unsigned int GetThreadReturnAddr(unsigned int t)
{
	volatile unsigned int *pCa = t == 0 ? &g_pV3dCle->m_ct00ra0 : &g_pV3dCle->m_ct01ra0;
	return *pCa;
}

static long Ioctl(struct file *pFile, unsigned int cmd, unsigned long arg)
{
	int error = 0;
	PRINTK_VERBOSE(KERN_DEBUG "ioctl cmd %x arg %lx\n", cmd, arg);

	switch (cmd)
	{
	case DMA_PREPARE:
	case DMA_PREPARE_KICK:
	case DMA_PREPARE_KICK_WAIT:
		{
			struct DmaControlBlock __user *pUCB = (struct DmaControlBlock *)arg;
			int steps = 0;
			unsigned long start_time = jiffies;
			(void)start_time;

			//flush our address cache
			FlushAddrCache();

			PRINTK_VERBOSE(KERN_DEBUG "dma prepare\n");

			//do virtual to bus translation for each entry
			do
			{
				pUCB = DmaPrepare(pUCB, &error);
			} while (error == 0 && ++steps && pUCB);
			PRINTK_VERBOSE(KERN_DEBUG "prepare done in %d steps, %ld\n", steps, jiffies - start_time);

			//carry straight on if we want to kick too
			if (cmd == DMA_PREPARE || error)
			{
				PRINTK_VERBOSE(KERN_DEBUG "falling out\n");
				return error ? -EINVAL : 0;
			}
		}
	case DMA_KICK:
		PRINTK_VERBOSE(KERN_DEBUG "dma begin\n");

		if (cmd == DMA_KICK)
			FlushAddrCache();

		DmaKick((struct DmaControlBlock __user *)arg);
		
		if (cmd != DMA_PREPARE_KICK_WAIT)
			break;
/*	case DMA_WAIT_ONE:
		//PRINTK(KERN_DEBUG "dma wait one\n");
		break;*/
	case DMA_WAIT_ALL:
		//PRINTK(KERN_DEBUG "dma wait all\n");
		DmaWaitAll();
		break;
	case DMA_MAX_BURST:
		if (g_dmaChan == 0)
			return 10;
		else
			return 5;
	case DMA_SET_MIN_PHYS:
		g_pMinPhys = (void __user *)arg;
		PRINTK(KERN_DEBUG "min/max user/phys bypass set to %p %p\n", g_pMinPhys, g_pMaxPhys);
		break;
	case DMA_SET_MAX_PHYS:
		g_pMaxPhys = (void __user *)arg;
		PRINTK(KERN_DEBUG "min/max user/phys bypass set to %p %p\n", g_pMinPhys, g_pMaxPhys);
		break;
	case DMA_SET_PHYS_OFFSET:
		g_physOffset = arg;
		PRINTK(KERN_DEBUG "user/phys bypass offset set to %ld\n", g_physOffset);
		break;
	case DMA_CMA_SET_SIZE:
	{
		unsigned int pBusAddr;

		if (g_cmaHandle)
		{
			PRINTK(KERN_ERR "memory has already been allocated (handle %d)\n", g_cmaHandle);
			return -EINVAL;
		}

		PRINTK(KERN_INFO "allocating %ld bytes of VC memory\n", arg * 4096);

		//get the memory
		if (AllocateVcMemory(&g_cmaHandle, arg * 4096, 4096, MEM_FLAG_ALLOCATING/*MEM_FLAG_L1_NONALLOCATING*//* MEM_FLAG_COHERENT */| MEM_FLAG_NO_INIT | MEM_FLAG_HINT_PERMALOCK))
		{
			PRINTK(KERN_ERR "failed to allocate %ld bytes of VC memory\n", arg * 4096);
			g_cmaHandle = 0;
			return -EINVAL;
		}

		//get an address for it
		PRINTK(KERN_INFO "trying to map VC memory\n");

		if (LockVcMemory(&pBusAddr, g_cmaHandle))
		{
			PRINTK(KERN_ERR "failed to map CMA handle %d, releasing memory\n", g_cmaHandle);
			ReleaseVcMemory(g_cmaHandle);
			g_cmaHandle = 0;
		}

		PRINTK(KERN_INFO "bus address for CMA memory is %x\n", pBusAddr);
		return pBusAddr;
	}
	case DMA_GET_VERSION:
		PRINTK(KERN_DEBUG "returning version number, %d\n", VERSION_NUMBER);
		return VERSION_NUMBER;

	case V3D_IOCTL_POST_JOB:
	{
		v3d_job_post_t __user *pPost = (v3d_job_post_t __user *)arg;
		v3d_job_post_t post;

//		__cpuc_flush_user_all();
//		__cpuc_flush_kern_all();

		if (copy_from_user(&post, pPost, sizeof(post)))
		{
			PRINTK(KERN_ERR "POST JOB argument %p is not mapped\n", pPost);
			return -EFAULT;
		}

		PRINTK_VERBOSE(KERN_DEBUG "POST JOB type %d id %d %08x/%08x %08x/%08x size %d overflow %p %d bytes\n",
			post.job_type, post.job_id,
			post.v3d_ct0ca, post.v3d_ct0ea,
			post.v3d_ct1ca, post.v3d_ct1ea,
			post.v3d_vpm_size, post.m_pOverspill, post.m_overspillSize);

		if (post.job_type == V3D_JOB_BIN_REND)
		{
			int countdown = 1000000;
			int initial_bfc, initial_rfc;

			while (countdown--)
			{
				if (g_pV3dCle->m_pcs == 0)
					break;
				else
					schedule();
			}

			countdown = 1000000;

			if (countdown == 0)
			{
				PRINTK(KERN_ERR "gpu thread 1 has not stopped\n");
				PRINTK_VERBOSE(KERN_DEBUG "resetting\n");
				g_pV3dCle->m_ct0cs = 1 << 15;
				g_pV3dCle->m_ct1cs = 1 << 15;
			}

			//flush caches
			g_pV3dCache->m_l2cactl = 1 << 2;

			//turn off cache
//			g_pV3dCache->m_l2cactl = 1 << 1;

			//clear slice cache
			/*g_pV3dCache->m_slcactl = 0xf;
			g_pV3dCache->m_slcactl = 0xf << 8;
			g_pV3dCache->m_slcactl = 0xf << 16;
			g_pV3dCache->m_slcactl = 0xf << 24;*/

			barrier();

			//set the overflow
			g_pV3dBinning->m_bpoa = 0;
			g_pV3dBinning->m_bpos = 0;
			barrier();

			initial_bfc = g_pV3dCle->m_bfc & 0xff;
			initial_rfc = g_pV3dCle->m_rfc & 0xff;

			PRINTK_VERBOSE(KERN_DEBUG "pre kick 0 bfc %d rfc %d\n", initial_bfc, initial_rfc);
			PRINTK_VERBOSE(KERN_DEBUG "thread 0 status %08x pc %08x error %d\n", GetThreadStatus(0), GetThreadPc(0), HasThreadStoppedWithError(0));
			PRINTK_VERBOSE(KERN_DEBUG "error codes %08x\n", g_pV3dDebErr->m_errStat);

			RunThread(0, post.v3d_ct0ca, post.v3d_ct0ea);

#if 0
			while(countdown--)
			{
				barrier();
				if (g_pV3dCle->m_pcs == 0x101)
					PRINTK_VERBOSE(KERN_DEBUG "out of bin memory\n");
				barrier();

				if (HasThreadStopped(0) && !HasBinnerRunOutOfMemory())
					break;
				else if (HasBinnerRunOutOfMemory())
				{
					PRINTK_VERBOSE(KERN_DEBUG "run out of bin memory: pcs %08x bin left %08x/%08x\n",
							g_pV3dCle->m_pcs, g_pV3dBinning->m_bpcs, g_pV3dBinning->m_bpos);

					//set the overflow

#define OVERSPILL_GIVE_IT (96 * 1024)
					if (post.m_overspillSize >= OVERSPILL_GIVE_IT)
					{
						PRINTK_VERBOSE(KERN_DEBUG "giving it %p, %d bytes\n", post.m_pOverspill, OVERSPILL_GIVE_IT);

						g_pV3dBinning->m_bpoa = (unsigned int)post.m_pOverspill;
						barrier();
						g_pV3dBinning->m_bpos = OVERSPILL_GIVE_IT;

						post.m_overspillSize -= OVERSPILL_GIVE_IT;
						post.m_pOverspill = (void *)(unsigned int)post.m_pOverspill + OVERSPILL_GIVE_IT;
					}
				}
				/*else
					schedule();*/
			}

			barrier();

			countdown = 1000000;
			while (countdown--)
			{
				if (g_pV3dCle->m_pcs == 0)
					break;
				else if (countdown == 1000000-1)
					PRINTK(KERN_DEBUG "INITIAL status %08x pcs %08x\n", GetThreadStatus(0), g_pV3dCle->m_pcs);
				/*else
					schedule();*/
			}
#endif

			while (countdown-- > 0)
			{
				barrier();

				if ((g_pV3dCle->m_pcs & 0x100) && (g_pV3dBinning->m_bpos == 0))
				{
					PRINTK_VERBOSE(KERN_DEBUG "out of bin memory\n");

					PRINTK_VERBOSE(KERN_DEBUG "run out of bin memory: pcs %08x bin left %08x/%08x\n",
							g_pV3dCle->m_pcs, g_pV3dBinning->m_bpcs, g_pV3dBinning->m_bpos);

					//set the overflow

#define OVERSPILL_GIVE_IT (96 * 1024)
					if (post.m_overspillSize >= OVERSPILL_GIVE_IT)
					{
						volatile unsigned int current_addr = g_pV3dCle->m_ct0ca;
						barrier();

						PRINTK_VERBOSE(KERN_DEBUG "giving it %p %d bytes\n", post.m_pOverspill, OVERSPILL_GIVE_IT);

						g_pV3dBinning->m_bpoa = (unsigned int)post.m_pOverspill;
						barrier();
						g_pV3dBinning->m_bpos = OVERSPILL_GIVE_IT;

						barrier();

						post.m_overspillSize -= OVERSPILL_GIVE_IT;
						post.m_pOverspill = (void *)(unsigned int)post.m_pOverspill + OVERSPILL_GIVE_IT;

						PRINTK_VERBOSE(KERN_DEBUG " %d bytes remaining \n", post.m_overspillSize);

						//wait for it to progress - rather than catching the same event again
						while (countdown--)
						{
							if (g_pV3dCle->m_ct0ca != current_addr)
								break;
						}
					}
					else
						PRINTK(KERN_ERR "out of spare overspill memory!\n");
				}

				if ((g_pV3dCle->m_bfc & 0xff) == initial_bfc + 1)
				{
					PRINTK_VERBOSE(KERN_DEBUG "appears finished status %08x pcs %08x\n", GetThreadStatus(0), g_pV3dCle->m_pcs);
					break;
				}
			}



			PRINTK_VERBOSE(KERN_DEBUG "STOPPED BUT OUT OF MEMORY status %08x pcs %08x\n", GetThreadStatus(0), g_pV3dCle->m_pcs);

			barrier();

			if (countdown <= 0)
			{
				PRINTK(KERN_DEBUG "thread 0 appears to have died\n");
				PRINTK_VERBOSE(KERN_DEBUG "resetting both threads\n");
				g_pV3dCle->m_ct0cs = 1 << 15;
				g_pV3dCle->m_ct1cs = 1 << 15;
			}
			else
			{
				PRINTK_VERBOSE(KERN_DEBUG "post kick 0\n");
				PRINTK_VERBOSE(KERN_DEBUG "thread 0 status %08x pc %08x error %d ra %08x\n", GetThreadStatus(0), GetThreadPc(0), HasThreadStoppedWithError(0), GetThreadReturnAddr(0));
				PRINTK_VERBOSE(KERN_DEBUG "error codes %08x  pcs %08x bin left %08x/%08x\n",
						g_pV3dDebErr->m_errStat, g_pV3dCle->m_pcs, g_pV3dBinning->m_bpcs, g_pV3dBinning->m_bpos);

				if (HasThreadStopped(0) && !HasThreadStoppedWithError(0))
				{
					PRINTK_VERBOSE(KERN_DEBUG "pre kick 1\n");
					PRINTK_VERBOSE(KERN_DEBUG "thread 1 status %08x pc %08x error %d\n", GetThreadStatus(1), GetThreadPc(1), HasThreadStoppedWithError(1));
					PRINTK_VERBOSE(KERN_DEBUG "error codes %08x pcs %08x bin left %08x/%08x\n",
							g_pV3dDebErr->m_errStat, g_pV3dCle->m_pcs, g_pV3dBinning->m_bpcs, g_pV3dBinning->m_bpos);

					//flush caches
					//g_pV3dCache->m_l2cactl = 1 << 2;

					//turn off cache
	//				g_pV3dCache->m_l2cactl = 1 << 1;

					//clear slice cache
					/*g_pV3dCache->m_slcactl = 0xf;
					g_pV3dCache->m_slcactl = 0xf << 8;
					g_pV3dCache->m_slcactl = 0xf << 16;
					g_pV3dCache->m_slcactl = 0xf << 24;*/

					RunThread(1, post.v3d_ct1ca, post.v3d_ct1ea);
//
//					countdown = 1000000;
//
//					while(countdown--)
//					{
//						/*if (HasThreadStopped(1))
//							break;*/
//						/*else
//							schedule();*/
//
//						if ((g_pV3dCle->m_rfc & 0xff) == initial_rfc + 1)
//							break;
//					}
//
//					if (countdown <= 0)
//					{
//						PRINTK(KERN_DEBUG "thread 1 appears to have died\n");
//						PRINTK_VERBOSE(KERN_DEBUG "resetting\n");
//						g_pV3dCle->m_ct0cs = 1 << 15;
//						g_pV3dCle->m_ct1cs = 1 << 15;
//					}
//
//					countdown = 1000000;
//					while (countdown--)
//					{
//						if (g_pV3dCle->m_pcs == 0)
//							break;
//						else
//							schedule();
//					}
//
//					PRINTK_VERBOSE(KERN_DEBUG "STOPPED BUT OUT OF MEMORY status %08x pcs %08x\n", GetThreadStatus(0), g_pV3dCle->m_pcs);
//
//					barrier();
//
//					PRINTK_VERBOSE(KERN_DEBUG "post kick 1\n");
//					PRINTK_VERBOSE(KERN_DEBUG "thread 1 status %08x pc %08x error %d ra %08x\n", GetThreadStatus(1), GetThreadPc(1), HasThreadStoppedWithError(1), GetThreadReturnAddr(1));
//					PRINTK_VERBOSE(KERN_DEBUG "error codes %08x pcs %08x\n",
//							g_pV3dDebErr->m_errStat, g_pV3dCle->m_pcs);
				}
				else
				{
					//let's try and reset it
					//reset it
					PRINTK_VERBOSE(KERN_DEBUG "resetting\n");
					g_pV3dCle->m_ct0cs = 1 << 15;
					g_pV3dCle->m_ct1cs = 1 << 15;
				}
			}
		}
		else
			PRINTK(KERN_ERR "unsupported job type %d\n", post.job_type);
		return 0;
	}

	case V3D_IOCTL_WAIT_JOB:
        {
			v3d_job_status_t __user *pWait = (v3d_job_status_t __user *)arg;
			v3d_job_status_t wait;

			if (copy_from_user(&wait, pWait, sizeof(wait)))
			{
					PRINTK(KERN_ERR "WAIT JOB argument %p is not mapped\n", pWait);
					return -EFAULT;
			}

			PRINTK_VERBOSE(KERN_DEBUG "WAIT JOB id %d\n", wait.job_id);

			wait.job_status = V3D_JOB_STATUS_SUCCESS;

			if (copy_to_user(pWait, &wait, sizeof(wait)))
			{
					PRINTK(KERN_ERR "could not copy results back to %p\n", pWait);
					return -EFAULT;
			}

			return 0;
        }

/*	case V3D_IOCTL_FLUSH_JOB:
	{
		PRINTK(KERN_DEBUG "FLUSH JOB\n");
		return 0;
	}

	case V3D_IOCTL_ACQUIRE:
        {
                PRINTK(KERN_DEBUG "ACQUIRE\n");
                return 0;
        }

	case V3D_IOCTL_RELEASE:
        {
                PRINTK(KERN_DEBUG "RELEASE\n");
                return 0;
        }*/

	default:
		PRINTK(KERN_DEBUG "unknown ioctl: %d\n", cmd);
		return -EINVAL;
	}

	return 0;
}

static ssize_t Read(struct file *pFile, char __user *pUser, size_t count, loff_t *offp)
{
	return -EIO;
}

static int Mmap(struct file *pFile, struct vm_area_struct *pVma)
{
	struct PageList *pPages;
	struct VmaPageList *pVmaList;
	
	PRINTK_VERBOSE(KERN_DEBUG "MMAP vma %p, length %ld (%s %d)\n",
		pVma, pVma->vm_end - pVma->vm_start,
		current->comm, current->pid);
	PRINTK_VERBOSE(KERN_DEBUG "MMAP %p %d (tracked %d)\n", pVma, current->pid, g_trackedPages);

	//make a new page list
	pPages = (struct PageList *)kmalloc(sizeof(struct PageList), GFP_KERNEL);
	if (!pPages)
	{
		PRINTK(KERN_ERR "couldn\'t allocate a new page list (%s %d)\n",
			current->comm, current->pid);
		return -ENOMEM;
	}

	//clear the page list
	pPages->m_used = 0;
	pPages->m_pNext = 0;
	
	//insert our vma and new page list somewhere
	if (!pVma->vm_private_data)
	{
		struct VmaPageList *pList;

		PRINTK_VERBOSE(KERN_DEBUG "new vma list, making new one (%s %d)\n",
			current->comm, current->pid);

		//make a new vma list
		pList = (struct VmaPageList *)kmalloc(sizeof(struct VmaPageList), GFP_KERNEL);
		if (!pList)
		{
			PRINTK(KERN_ERR "couldn\'t allocate vma page list (%s %d)\n",
				current->comm, current->pid);
			kfree(pPages);
			return -ENOMEM;
		}

		//clear this list
		pVma->vm_private_data = (void *)pList;
		pList->m_refCount = 0;
	}

	pVmaList = (struct VmaPageList *)pVma->vm_private_data;

	//add it to the vma list
	pVmaList->m_pPageHead = pPages;
	pVmaList->m_pPageTail = pPages;

	pVma->vm_ops = &g_vmOps4k;
	pVma->vm_flags |= VM_IO;

	VmaOpen4k(pVma);

	return 0;
}

/****** VMA OPERATIONS ******/

static void VmaOpen4k(struct vm_area_struct *pVma)
{
	struct VmaPageList *pVmaList;

	PRINTK_VERBOSE(KERN_DEBUG "vma open %p private %p (%s %d), %d live pages\n", pVma, pVma->vm_private_data, current->comm, current->pid, g_trackedPages);
	PRINTK_VERBOSE(KERN_DEBUG "OPEN %p %d %ld pages (tracked pages %d)\n",
		pVma, current->pid, (pVma->vm_end - pVma->vm_start) >> 12,
		g_trackedPages);

	pVmaList = (struct VmaPageList *)pVma->vm_private_data;

	if (pVmaList)
	{
		pVmaList->m_refCount++;
		PRINTK_VERBOSE(KERN_DEBUG "ref count is now %d\n", pVmaList->m_refCount);
	}
	else
	{
		PRINTK_VERBOSE(KERN_DEBUG "err, open but no vma page list\n");
	}
}

static void VmaClose4k(struct vm_area_struct *pVma)
{
	struct VmaPageList *pVmaList;
	int freed = 0;
	
	PRINTK_VERBOSE(KERN_DEBUG "vma close %p private %p (%s %d)\n", pVma, pVma->vm_private_data, current->comm, current->pid);
	
	//wait for any dmas to finish
	DmaWaitAll();

	//find our vma in the list
	pVmaList = (struct VmaPageList *)pVma->vm_private_data;

	//may be a fork
	if (pVmaList)
	{
		struct PageList *pPages;
		
		pVmaList->m_refCount--;

		if (pVmaList->m_refCount == 0)
		{
			PRINTK_VERBOSE(KERN_DEBUG "found vma, freeing pages (%s %d)\n",
				current->comm, current->pid);

			pPages = pVmaList->m_pPageHead;

			if (!pPages)
			{
				PRINTK(KERN_ERR "no page list (%s %d)!\n",
					current->comm, current->pid);
				return;
			}

			while (pPages)
			{
				struct PageList *next;
				int count;

				PRINTK_VERBOSE(KERN_DEBUG "page list (%s %d)\n",
					current->comm, current->pid);

				next = pPages->m_pNext;
				for (count = 0; count < pPages->m_used; count++)
				{
					PRINTK_VERBOSE(KERN_DEBUG "freeing page %p (%s %d)\n",
						pPages->m_pPages[count],
						current->comm, current->pid);
					__free_pages(pPages->m_pPages[count], 0);
					g_trackedPages--;
					freed++;
				}

				PRINTK_VERBOSE(KERN_DEBUG "freeing page list (%s %d)\n",
					current->comm, current->pid);
				kfree(pPages);
				pPages = next;
			}
			
			//remove our vma from the list
			kfree(pVmaList);
			pVma->vm_private_data = 0;
		}
		else
		{
			PRINTK_VERBOSE(KERN_DEBUG "ref count is %d, not closing\n", pVmaList->m_refCount);
		}
	}
	else
	{
		PRINTK_VERBOSE(KERN_ERR "uh-oh, vma %p not found (%s %d)!\n", pVma, current->comm, current->pid);
		PRINTK_VERBOSE(KERN_ERR "CLOSE ERR\n");
	}

	PRINTK_VERBOSE(KERN_DEBUG "CLOSE %p %d %d pages (tracked pages %d)",
		pVma, current->pid, freed, g_trackedPages);

	PRINTK_VERBOSE(KERN_DEBUG "%d pages open\n", g_trackedPages);
}

static int VmaFault4k(struct vm_area_struct *pVma, struct vm_fault *pVmf)
{
	PRINTK_VERBOSE(KERN_DEBUG "vma fault for vma %p private %p at offset %ld (%s %d)\n", pVma, pVma->vm_private_data, pVmf->pgoff,
		current->comm, current->pid);
	PRINTK_VERBOSE(KERN_DEBUG "FAULT\n");
	pVmf->page = alloc_page(GFP_KERNEL);
	
	if (pVmf->page)
	{
		PRINTK_VERBOSE(KERN_DEBUG "alloc page virtual %p\n", page_address(pVmf->page));
	}

	if (!pVmf->page)
	{
		PRINTK(KERN_ERR "vma fault oom (%s %d)\n", current->comm, current->pid);
		return VM_FAULT_OOM;
	}
	else
	{
		struct VmaPageList *pVmaList;
		
		get_page(pVmf->page);
		g_trackedPages++;
		
		//find our vma in the list
		pVmaList = (struct VmaPageList *)pVma->vm_private_data;
		
		if (pVmaList)
		{
			PRINTK_VERBOSE(KERN_DEBUG "vma found (%s %d)\n", current->comm, current->pid);

			if (pVmaList->m_pPageTail->m_used == PAGES_PER_LIST)
			{
				PRINTK_VERBOSE(KERN_DEBUG "making new page list (%s %d)\n", current->comm, current->pid);
				//making a new page list
				pVmaList->m_pPageTail->m_pNext = (struct PageList *)kmalloc(sizeof(struct PageList), GFP_KERNEL);
				if (!pVmaList->m_pPageTail->m_pNext)
					return -ENOMEM;
				
				//update the tail pointer
				pVmaList->m_pPageTail = pVmaList->m_pPageTail->m_pNext;
				pVmaList->m_pPageTail->m_used = 0;
				pVmaList->m_pPageTail->m_pNext = 0;
			}

			PRINTK_VERBOSE(KERN_DEBUG "adding page to list (%s %d)\n", current->comm, current->pid);
			
			pVmaList->m_pPageTail->m_pPages[pVmaList->m_pPageTail->m_used] = pVmf->page;
			pVmaList->m_pPageTail->m_used++;
		}
		else
			PRINTK(KERN_ERR "returned page for vma we don\'t know %p (%s %d)\n", pVma, current->comm, current->pid);
		
		return 0;
	}
}

/****** GENERIC FUNCTIONS ******/
static int __init dmaer_init(void)
{
	char v3d[3]; int ver;
	int result = alloc_chrdev_region(&g_majorMinor, 0, 1, "dmaer");
        unsigned int addr = IO_ADDRESS(0x20c00000);
	PRINTK(KERN_DEBUG "%08x %08x %08x", 0x20c00000, addr, *(int *)addr);
	if (result < 0)
	{
		PRINTK(KERN_ERR "unable to get major device number\n");
		return result;
	}
	else
		PRINTK(KERN_DEBUG "major device number %d\n", MAJOR(g_majorMinor));
	
	PRINTK(KERN_DEBUG "vma list size %d, page list size %d, page size %ld\n",
		sizeof(struct VmaPageList), sizeof(struct PageList), PAGE_SIZE);

	//get a dma channel to work with
	result = bcm_dma_chan_alloc(BCM_DMA_FEATURE_FAST, (void **)&g_pDmaChanBase, &g_dmaIrq);

	//uncomment to force to channel 0
	//result = 0;
	//g_pDmaChanBase = 0xce808000;
	
	if (result < 0)
	{
		PRINTK(KERN_ERR "failed to allocate dma channel\n");
		cdev_del(&g_cDev);
		unregister_chrdev_region(g_majorMinor, 1);
	}
	
	//reset the channel
	PRINTK(KERN_DEBUG "allocated dma channel %d (%p), initial state %08x\n", result, g_pDmaChanBase, *g_pDmaChanBase);
	*g_pDmaChanBase = 1 << 31;
	PRINTK_VERBOSE(KERN_DEBUG "post-reset %08x\n", *g_pDmaChanBase);
	
	g_dmaChan = result;

	//clear the cache stats
	g_cacheHit = 0;
	g_cacheMiss = 0;

	//register our device - after this we are go go go
	cdev_init(&g_cDev, &g_fOps);
	g_cDev.owner = THIS_MODULE;
	g_cDev.ops = &g_fOps;
	
	result = cdev_add(&g_cDev, g_majorMinor, 1);
	if (result < 0)
	{
		PRINTK(KERN_ERR "failed to add character device\n");
		unregister_chrdev_region(g_majorMinor, 1);
		bcm_dma_chan_free(g_dmaChan);
		return result;
	}
		
	QpuEnable(true);
	PRINTK_VERBOSE(KERN_DEBUG "%08x %08x %08x", 0x20c00000, addr, *(int *)addr);

	//check it's come up
	v3d[0] = g_pV3dIdent->m_ident0 & 0xff;
	v3d[1] = (g_pV3dIdent->m_ident0 >> 8) & 0xff;
	v3d[2] = (g_pV3dIdent->m_ident0 >> 16) & 0xff;
	ver = g_pV3dIdent->m_ident0 >> 24;

	PRINTK(KERN_INFO "V3D identify test: %c%c%c version %d\n", v3d[0], v3d[1], v3d[2], ver);

	if (v3d[0] == 'V' && v3d[1] == '3' && v3d[2] == 'D' && ver == 2)
	{
		PRINTK_VERBOSE(KERN_INFO "VPM memory size %d KB, HDR support %d, num semaphores %d, num TMUs/slice %d, num QPUs/slice %d, num slices %d, V3D revision %d\n",
			(g_pV3dIdent->m_ident1 >> 28) & 0xf,
			(g_pV3dIdent->m_ident1 >> 24) & 0xf,
			(g_pV3dIdent->m_ident1 >> 16) & 0xff,
			(g_pV3dIdent->m_ident1 >> 12) & 0xf,
			(g_pV3dIdent->m_ident1 >> 8) & 0xf,
			(g_pV3dIdent->m_ident1 >> 4) & 0xf,
			g_pV3dIdent->m_ident1 & 0xf);

		PRINTK_VERBOSE(KERN_INFO "tile buffer double buffer mode support %d, tile buffer size %d, vri memory size %d\n",
			(g_pV3dIdent->m_ident2 >> 8) & 0xf,
			(g_pV3dIdent->m_ident2 >> 4) & 0xf,
			g_pV3dIdent->m_ident2 & 0xf);

		//reset it
		g_pV3dCle->m_ct0cs = 1 << 15;
		g_pV3dCle->m_ct1cs = 1 << 15;
	}
	else
		PRINTK(KERN_ERR "failed to turn on and identify V3D\n");
	
	return 0;
}

static void __exit dmaer_exit(void)
{
	PRINTK(KERN_INFO "closing dmaer device, cache stats: %d hits %d misses\n", g_cacheHit, g_cacheMiss);
	//unregister the device
	cdev_del(&g_cDev);
	unregister_chrdev_region(g_majorMinor, 1);
	//free the dma channel
	bcm_dma_chan_free(g_dmaChan);

	QpuEnable(false);
}

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Simon Hall");
module_init(dmaer_init);
module_exit(dmaer_exit);

