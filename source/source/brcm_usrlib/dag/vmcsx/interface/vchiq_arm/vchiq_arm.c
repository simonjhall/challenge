/*=============================================================================
Copyright (c) 2010 Broadcom Europe Ltd. All rights reserved.

Project  :  ARM
Module   :  vchiq_arm

FILE DESCRIPTION:
Linux driver front-end for VCHIQ.

==============================================================================*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <asm/pgtable.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/dma-mapping.h>

#include <mach/platform.h>
#include <mach/irqs.h>
#include <mach/vcio.h>

#include "vchiq_core.h"
#include "vchiq_ioctl.h"

#if 0
#define DPRINTK printk
#else
#define DPRINTK if (0) printk
#endif

#define VCHIQ_MAJOR 100
#define VCHIQ_MINOR 0

/* Some per-instance constants */
#define MAX_COMPLETIONS 16
#define MAX_SERVICES VCHIQ_MAX_SERVICES
#define MAX_ELEMENTS 8
#define MAX_FRAGMENTS (VCHIQ_NUM_CURRENT_BULKS * 2)

#define CACHE_LINE_SIZE 32
#define PAGELIST_WRITE 0
#define PAGELIST_READ 1
#define PAGELIST_READ_WITH_FRAGMENTS 2

#define MIN(a,b) ((a)<(b)?(a):(b))

typedef struct bulk_data_struct
{
   unsigned long length;
   unsigned short type;
   unsigned short offset;
   unsigned long userdata;
   unsigned long addrs[];  /* N.B. 12 LSBs hold the number of following
                                   pages at consecutive addresses. */
} PAGELIST_T;

typedef struct fragments_struct
{
   char headbuf[CACHE_LINE_SIZE];
   char tailbuf[CACHE_LINE_SIZE];
} FRAGMENTS_T;

vcos_static_assert(((VCHIQ_CHANNEL_SIZE & (VCHIQ_CHANNEL_SIZE - 1)) == 0) &&
              (VCHIQ_CHANNEL_SIZE > PAGE_SIZE));

typedef struct client_service_struct
{
   VCHIQ_SERVICE_T *service;
   void *userdata;
   VCHIQ_INSTANCE_T instance;
   int handle;
} USER_SERVICE_T;

typedef struct vchiq_instance_struct
{
   VCHIQ_STATE_T *state;
   VCHIQ_COMPLETION_DATA_T completions[MAX_COMPLETIONS];
   volatile int completion_insert;
   volatile int completion_remove;
   VCOS_EVENT_T insert_event;
   VCOS_EVENT_T remove_event;

   USER_SERVICE_T services[MAX_SERVICES];

   void *mmap_base;
   int connected;
   int closing;
} VCHIQ_INSTANCE_STRUCT_T;

static struct class *vchiq_class;

DECLARE_MUTEX(vchiq_mutex);

static VCHIQ_STATE_T g_state;
static char *g_channel_mem;
static VCHIQ_CHANNEL_T *g_channels;
dma_addr_t g_channel_phys;
static FRAGMENTS_T *g_fragments_base;
static FRAGMENTS_T *g_free_fragments;
struct semaphore g_free_fragments_sema;


#if 0

#define VCHIQ_MSGBUF_SIZE 16384
static char vchiq_msgbuf[VCHIQ_MSGBUF_SIZE];
static int vchiq_msgptr;

static void my_printk(const char *fmt, ...)
{
   char msgbuf[128];
   int len, offset, space;

   va_list ap;
   va_start(ap,fmt);
   len = vsprintf(msgbuf, fmt, ap);
   va_end(ap);

   vcos_assert(len < sizeof(msgbuf));

   down(&vchiq_mutex);

   offset = vchiq_msgptr & (VCHIQ_MSGBUF_SIZE - 1);
   space = VCHIQ_MSGBUF_SIZE - offset;

   vchiq_msgptr += len;
   if ((len + 1) < space)
   {
      memcpy(vchiq_msgbuf + offset, msgbuf, len + 1); /* Include the NUL */
   }
   else
   {
      memcpy(vchiq_msgbuf + offset, msgbuf, space);
      memcpy(vchiq_msgbuf, msgbuf + space, (len - space) + 1); /* Include the NUL */
   }

   up(&vchiq_mutex);
}

#endif

int vchiq_copy_from_user(void *dst, const void *src, int size)
{
   return copy_from_user(dst, src, size);
}

void vchiq_ring_doorbell(void)
{
   dsb(); /* data barrier operation */

   writel(0, __io_address(ARM_0_BELL2));
}

static irqreturn_t vchiq_doorbell_irq(int irq, void *dev_id)
{
   unsigned int status;
   irqreturn_t ret = IRQ_NONE;

   /* Read (and clear) the doorbell */
   status = readl(__io_address(ARM_0_BELL0));
   if (status & 0x4) /* Was the doorbell rung? */
   {
      remote_event_pollall(&g_state);
      ret = IRQ_HANDLED;
   }

   return ret;
}

/* There is a potential problem with partial cache lines (pages?)
   at the ends of the block when reading. If the CPU accessed anything in
   the same line (page?) then it may have pulled old data into the cache,
   obscuring the new data underneath. We can solve this by transferring the
   partial cache lines separately, and allowing the ARM to copy into the
   cached area.

   N.B. This implementation plays slightly fast and loose with the Linux
   driver programming rules, e.g. its use of __virt_to_bus instead of
   dma_map_single, but it isn't a multi-platform driver and it benefits
   from increased speed as a result.
 */

static int create_pagelist(char __user *buf, size_t count,
                           unsigned short type, unsigned long userdata, PAGELIST_T **ppagelist)
{
   PAGELIST_T *pagelist;
   struct page **pages;
   struct page *page;
   unsigned long *addrs;
   unsigned int num_pages, offset, i;
   char *addr, *base_addr, *next_addr;
   size_t size;
   int run, addridx;

   offset = (unsigned int)buf & (PAGE_SIZE - 1);
   num_pages = (count + offset + PAGE_SIZE - 1)/PAGE_SIZE;

   *ppagelist = NULL;

   /* Allocate enough storage to hold the page pointers and the page list */
   pagelist = (PAGELIST_T *)kmalloc(sizeof(PAGELIST_T) +
                                    (num_pages*sizeof(unsigned long)) +
                                    (num_pages*sizeof(pages[0])),
                                    GFP_KERNEL);
   if (!pagelist)
      return -ENOMEM;

   addrs = pagelist->addrs;
   pages = (struct page **)(addrs + num_pages);

   down_read(&current->mm->mmap_sem);
   get_user_pages(current, current->mm,
                  (unsigned long)buf & ~(PAGE_SIZE - 1), num_pages,
                  (type == PAGELIST_READ)/*Write*/, 0/*Force*/,
                  pages, NULL /*vmas*/ );
   up_read(&current->mm->mmap_sem);

   pagelist->length = count;
   pagelist->type = type;
   pagelist->offset = offset;
   pagelist->userdata = userdata;

   /* Group the pages into runs of contiguous pages */

   base_addr = __virt_to_bus(page_address(pages[0]));
   next_addr = base_addr + PAGE_SIZE;
   size = MIN(PAGE_SIZE - pagelist->offset, count);
   addridx = 0;
   run = 0;

   for (i = 1; i < num_pages; i++)
   {
      addr = __virt_to_bus(page_address(pages[i]));
      if ((addr == next_addr) && (run < (PAGE_SIZE - 1)))
      {
         next_addr += PAGE_SIZE;
         size = MIN(size + PAGE_SIZE, count);
         run++;
      }
      else
      {
         addrs[addridx++] = (unsigned long)base_addr + run;
         base_addr = addr;
         next_addr = addr + PAGE_SIZE;
         size = MIN(PAGE_SIZE, count);
         run = 0;
      }
   }

   addrs[addridx++] = (unsigned long)base_addr + run;

   /* Partial cache lines (fragments) require special measures */
   if ((type == PAGELIST_READ) &&
       ((pagelist->offset & (CACHE_LINE_SIZE - 1)) ||
        ((pagelist->offset + pagelist->length) & (CACHE_LINE_SIZE - 1))))
   {
      FRAGMENTS_T *fragments;

      if (down_interruptible(&g_free_fragments_sema) != 0)
      {
         kfree(pagelist);
         return -EINTR;
      }

      vcos_assert(g_free_fragments != NULL);

      down(&vchiq_mutex);
      fragments = (FRAGMENTS_T *)g_free_fragments;
      vcos_assert(fragments != NULL);
      g_free_fragments = *(FRAGMENTS_T **)g_free_fragments;
      up(&vchiq_mutex);
      pagelist->type = PAGELIST_READ_WITH_FRAGMENTS + (fragments - g_fragments_base);
   }

   for (page = virt_to_page(pagelist);
        page <= virt_to_page(addrs+num_pages-1); page++)
   {
      flush_dcache_page(page);
   }

   *ppagelist = pagelist;

   return 0;
}

static void free_pagelist(PAGELIST_T *pagelist)
{
   struct page **pages;
   unsigned int num_pages, i;

   num_pages = (pagelist->length + pagelist->offset + PAGE_SIZE - 1)/PAGE_SIZE;

   pages = (struct page **)(pagelist->addrs + num_pages);

   /* Deal with any partial cache lines (fragments) */
   if (pagelist->type >= PAGELIST_READ_WITH_FRAGMENTS)
   {
      FRAGMENTS_T *fragments = g_fragments_base + (pagelist->type - PAGELIST_READ_WITH_FRAGMENTS);
      int head_bytes, tail_bytes;
      if ((head_bytes = (CACHE_LINE_SIZE - pagelist->offset) & (CACHE_LINE_SIZE - 1)) != 0)
      {
         if (head_bytes > pagelist->length)
            head_bytes = pagelist->length;

         memcpy((char *)page_address(pages[0]) + pagelist->offset, fragments->headbuf, head_bytes);
      }
      if ((head_bytes < pagelist->length) &&
          (tail_bytes = (pagelist->offset + pagelist->length) & (CACHE_LINE_SIZE - 1)) != 0)
      {
         memcpy((char *)page_address(pages[num_pages - 1]) + ((pagelist->offset + pagelist->length) & (PAGE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1)),
                fragments->tailbuf, tail_bytes);
      }

      down(&vchiq_mutex);
      *(FRAGMENTS_T **)fragments = g_free_fragments;
      g_free_fragments = fragments;
      up(&vchiq_mutex);
      up(&g_free_fragments_sema);
   }

   for (i = 0; i < num_pages; i++)
   {
      if (pagelist->type != PAGELIST_WRITE)
         set_page_dirty(pages[i]);
      page_cache_release(pages[i]);
   }

   kfree(pagelist);
}

VCHIQ_STATUS_T service_callback(VCHIQ_REASON_T reason, VCHIQ_HEADER_T *header, VCHIQ_SERVICE_HANDLE_T handle, void *bulk_userdata)
{
   /* How do we ensure the callback goes to the right client?
      The service_user data points to a USER_SERVICE_T record containing the
      original callback and the user state structure, which contains a circular
      buffer for completion records.
   */
   USER_SERVICE_T *service = (USER_SERVICE_T *)VCHIQ_GET_SERVICE_USERDATA(handle);
   VCHIQ_INSTANCE_T instance = service->instance;
   VCHIQ_COMPLETION_DATA_T *completion;

   DPRINTK("service_callback - service %lx(%d), reason %d, header %lx, bulk_userdata %lx\n", (unsigned long)service, ((VCHIQ_SERVICE_T *)handle)->localport, reason, (unsigned long)header, (unsigned long)bulk_userdata);

   if (instance->closing)
   {
      if (bulk_userdata != NULL)
         free_pagelist((PAGELIST_T *)bulk_userdata);
      return VCHIQ_SUCCESS;
   }

   while (instance->completion_insert == (instance->completion_remove + MAX_COMPLETIONS))
   {
      /* Out of space - wait for the client */
      if (vcos_event_wait(&instance->remove_event) != VCOS_SUCCESS)
      {
         DPRINTK("service_callback interrupted\n");
         return VCHIQ_RETRY;
      }
      else if (instance->closing)
      {
         DPRINTK("service_callback closing\n");
         return VCHIQ_ERROR;
      }
   }

   completion = &instance->completions[instance->completion_insert & (MAX_COMPLETIONS - 1)];
   completion->reason = reason;
   completion->header = header ? (((char *)header - g_state.remote->ctrl.data) + instance->mmap_base) : NULL;
   completion->service_userdata = service->userdata;
   completion->bulk_userdata = bulk_userdata;

   instance->completion_insert++;

   vcos_event_signal(&instance->insert_event);

   return VCHIQ_SUCCESS;
}

static long vchiq_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
   VCHIQ_INSTANCE_T instance = file->private_data;
   VCHIQ_STATUS_T status = VCHIQ_SUCCESS;
   long ret = 0;
   int i;

   if (_IOC_TYPE(cmd) != VCHIQ_IOC_MAGIC)
      return -ENOTTY;
   if (_IOC_NR(cmd) > VCHIQ_IOC_MAX)
      return -ENOTTY;

   DPRINTK("vchiq_ioctl - instance %x, cmd %x, arg %lx\n", (unsigned int )instance, cmd, arg);

   switch (cmd)
   {
   case VCHIQ_IOC_SHUTDOWN:
      if (!instance->connected)
         break;

      /* Remove all services */
      for (i = 0; i < MAX_SERVICES; i++)
      {
         USER_SERVICE_T *service = &instance->services[i];
         if (service->service != NULL)
         {
            status = vchiq_remove_service(&service->service->base);
            if (status != VCHIQ_SUCCESS)
               break;
            service->service = NULL;
         }
      }

      if (status == VCHIQ_SUCCESS)
      {
         /* Wake the completion thread and ask it to exit */
         instance->closing = 1;
         vcos_event_signal(&instance->insert_event);
      }

      break;

   case VCHIQ_IOC_CONNECT:
      if (instance->connected)
      {
         ret = -EINVAL;
         break;
      }
      if (vcos_mutex_lock(&instance->state->mutex) != VCOS_SUCCESS)
      {
         ret = -EINTR;
         break;
      }
      status = vchiq_connect_internal(instance->state, instance);
      vcos_mutex_unlock(&instance->state->mutex);

      if (status == VCHIQ_SUCCESS)
         instance->connected = 1;
      break;

   case VCHIQ_IOC_ADD_SERVICE:
   case VCHIQ_IOC_OPEN_SERVICE:
      {
         VCHIQ_ADD_SERVICE_T args;
         VCHIQ_SERVICE_T *service = NULL;
         USER_SERVICE_T *user_service = NULL;
         int srvstate;

         if (copy_from_user(&args, (const void __user *)arg, sizeof(args)) != 0)
            return -EFAULT;

         for (i = 0; i < MAX_SERVICES; i++)
         {
            if (instance->services[i].service == NULL)
            {
               user_service = &instance->services[i];
               break;
            }
         }

         if (!user_service)
         {
            ret = -EMFILE;
            break;
         }

         if (cmd == VCHIQ_IOC_ADD_SERVICE)
         {
            srvstate = instance->connected ? VCHIQ_SRVSTATE_LISTENING : VCHIQ_SRVSTATE_HIDDEN;
         }
         else
         {
            if (instance->connected)
               srvstate = VCHIQ_SRVSTATE_OPENING;
            else
            {
               ret = -ENOTCONN;
               break;
            }
         }

         vcos_mutex_lock(&instance->state->mutex);

         service = vchiq_add_service_internal(instance->state, args.fourcc,
                                              service_callback, user_service,
                                              srvstate, instance);

         vcos_mutex_unlock(&instance->state->mutex);

         if (service != NULL)
         {
            if (cmd == VCHIQ_IOC_OPEN_SERVICE)
            {
               status = vchiq_open_service_internal(service);
               if (status != VCHIQ_SUCCESS)
               {
                  vchiq_remove_service(&service->base);
                  ret = (status == VCHIQ_RETRY) ? -EINTR : -EIO;
                  break;
               }
            }
            user_service->service = service;
            user_service->userdata = args.service_userdata;
            user_service->instance = instance;
            user_service->handle = i;

            if (copy_to_user((void __user *)&(((VCHIQ_ADD_SERVICE_T __user *)arg)->handle),
                             (const void *)&user_service->handle,
                             sizeof(user_service->handle)) != 0)
               ret = -EFAULT;
         }
         else
         {
            ret = -EEXIST;
         }
      }
      break;

   case VCHIQ_IOC_REMOVE_SERVICE:
      {
         USER_SERVICE_T *user_service;
         int handle = (int)arg;

         user_service = &instance->services[handle];
         if ((handle >= 0) && (handle < MAX_SERVICES) && (user_service->service != NULL))
         {
            status = vchiq_remove_service(&user_service->service->base);
            if (status == VCHIQ_SUCCESS)
               user_service->service = NULL;
         }
         else
            ret = -EINVAL;
      }
      break;

   case VCHIQ_IOC_QUEUE_MESSAGE:
      {
         VCHIQ_QUEUE_MESSAGE_T args;
         USER_SERVICE_T *user_service;

         if (copy_from_user(&args, (const void __user *)arg, sizeof(args)) != 0)
         {
            ret = -EFAULT;
            break;
         }
         user_service = &instance->services[args.handle];
         if ((args.handle >= 0) && (args.handle < MAX_SERVICES) &&
             (user_service->service != NULL) &&
             (args.count <= MAX_ELEMENTS))
         {
            /* Copy elements into kernel space */
            VCHIQ_ELEMENT_T elements[MAX_ELEMENTS];
            if (copy_from_user(elements, args.elements, args.count * sizeof(VCHIQ_ELEMENT_T)) == 0)
               status = vchiq_queue_message(&user_service->service->base,
                                            elements, args.count);
            else
               ret = -EFAULT;
         }
         else
         {
            ret = -EINVAL;
         }
      }
      break;

   case VCHIQ_IOC_QUEUE_BULK_TRANSMIT:
      {
         VCHIQ_QUEUE_BULK_TRANSMIT_T args;
         USER_SERVICE_T *user_service;
         PAGELIST_T *pagelist;

         if (copy_from_user(&args, (const void __user *)arg, sizeof(args)) != 0)
            return -EFAULT;
         user_service = &instance->services[args.handle];
         if ((args.handle >= 0) && (args.handle < MAX_SERVICES) &&
             (user_service->service != NULL))
         {
            ret = create_pagelist((char __user *)args.data, args.size,
                                  PAGELIST_WRITE, (unsigned long)args.userdata,
                                  &pagelist);
            if (ret == 0)
            {
               status = vchiq_queue_bulk_transmit(&user_service->service->base,
                                                  __virt_to_bus((char *)pagelist), args.size, pagelist);
               if (status != VCHIQ_SUCCESS)
                  free_pagelist(pagelist);
            }
         }
         else
         {
            ret = -EINVAL;
         }
      }
      break;

   case VCHIQ_IOC_QUEUE_BULK_RECEIVE:
      {
         VCHIQ_QUEUE_BULK_TRANSMIT_T args;
         USER_SERVICE_T *user_service;
         PAGELIST_T *pagelist;

         if (copy_from_user(&args, (const void __user *)arg, sizeof(args)) != 0)
            return -EFAULT;
         user_service = &instance->services[args.handle];
         if ((args.handle >= 0) && (args.handle < MAX_SERVICES) &&
             (user_service->service != NULL))
         {
            ret = create_pagelist((char __user *)args.data, args.size,
                                  PAGELIST_READ, (unsigned long)args.userdata,
                                  &pagelist);
            if (ret == 0)
            {
               status = vchiq_queue_bulk_receive(&user_service->service->base,
                                                 __virt_to_bus((char *)pagelist), args.size, pagelist);
               if (status != VCHIQ_SUCCESS)
                  free_pagelist(pagelist);
            }
         }
         else
         {
            ret = -EINVAL;
         }
      }
      break;

   case VCHIQ_IOC_AWAIT_COMPLETION:
      {
         VCHIQ_AWAIT_COMPLETION_T args;

         if (!instance->connected)
            return -ENOTCONN;

         if (copy_from_user(&args, (const void __user *)arg, sizeof(args)) != 0)
            return -EFAULT;

         while ((instance->completion_remove == instance->completion_insert) &&
                !instance->closing)
         {
            if (vcos_event_wait(&instance->insert_event) != VCOS_SUCCESS)
            {
               DPRINTK("AWAIT_COMPLETION interrupted\n");
               ret = -EINTR;
               break;
            }
         }

         if (ret == 0)
         {
            for (ret = 0; ret < args.count; ret++)
            {
               VCHIQ_COMPLETION_DATA_T *completion;
               if (instance->completion_remove == instance->completion_insert)
                  break;
               completion = &instance->completions[instance->completion_remove++ & (MAX_COMPLETIONS - 1)];

               if (completion->bulk_userdata != NULL)
               {
                  PAGELIST_T *pagelist = (PAGELIST_T *)completion->bulk_userdata;
                  completion->bulk_userdata = (void *)pagelist->userdata;
                  free_pagelist(pagelist);
               }

               if (copy_to_user((void __user *)((size_t)args.buf + ret*sizeof(VCHIQ_COMPLETION_DATA_T)),
                                completion, sizeof(VCHIQ_COMPLETION_DATA_T)) != 0)
               {
                  ret = -EFAULT;
                  break;
               }
            }
         }

         if (ret != 0)
            vcos_event_signal(&instance->remove_event);
      }
      break;

   case VCHIQ_IOC_RELEASE_MESSAGE:
      {
         VCHIQ_RELEASE_MESSAGE_T args;
         USER_SERVICE_T *user_service;

         if (copy_from_user(&args, (const void __user *)arg, sizeof(args)) != 0)
            return -EFAULT;
         user_service = &instance->services[args.handle];
         if ((args.handle >= 0) && (args.handle < MAX_SERVICES) &&
             (user_service->service != NULL))
         {
            VCHIQ_HEADER_T *header = (VCHIQ_HEADER_T *)(((char *)args.header - (char *)instance->mmap_base) + instance->state->remote->ctrl.data);
            DPRINTK("RELEASE_MESSAGE %lx\n", (unsigned long)header);
            vchiq_release_message(&user_service->service->base, header);
         }
      }
      break;

   default:
      ret = -ENOTTY;
      break;
   }

   if (ret == 0)
   {
      if (status == VCHIQ_ERROR)
         ret = -EIO;
      else if (status == VCHIQ_RETRY)
         ret = -EINTR;
   }

   DPRINTK("  ioctl instance %lx, cmd %x -> status %d, %ld\n", (unsigned long)instance, cmd, status, ret);

   return ret;
}

static int vchiq_vma_fault(struct vm_area_struct *vma,
                           struct vm_fault *vmf)
{
   struct page *page;

   DPRINTK("vchiq_vma_fault(%lx) - pgoff=%lx\n", (unsigned long)vma, vmf->pgoff);

   if (vmf->pgoff >= (VCHIQ_CHANNEL_SIZE >> PAGE_SHIFT))
      return VM_FAULT_SIGBUS;

   page = pfn_to_page(__bus_to_pfn(g_channel_phys) + vmf->pgoff);
   DPRINTK("vchiq_vma_fault - page=%lx\n", (unsigned long)page);
   get_page(page); /* Increment reference count */
   vmf->page = page;
   return 0;
}

struct vm_operations_struct vchiq_vm_ops =
{
   .fault = vchiq_vma_fault,
};

static int vchiq_mmap(struct file *file, struct vm_area_struct *vma)
{
   VCHIQ_INSTANCE_T instance = file->private_data;
   unsigned long start = vma->vm_start;
   unsigned long size = vma->vm_end - vma->vm_start;

   DPRINTK("vchiq_mmap: start=%lx, size=%lx, pgoff=%lx, flags=%lx, page_prot=%lx (g_channel_phys=%lx)\n",
          start, size, vma->vm_pgoff, vma->vm_flags, vma->vm_page_prot, (unsigned long)g_channel_phys);

   if ((vma->vm_pgoff != 0) || (size != VCHIQ_CHANNEL_SIZE) ||
       ((vma->vm_flags & VM_WRITE) != 0))
      return -EINVAL;

   vma->vm_ops = &vchiq_vm_ops;
   vma->vm_private_data = file->private_data;
   vma->vm_file = file;
   vma->vm_flags = (vma->vm_flags & ~(VM_EXEC | VM_MAYWRITE)) | VM_IO | VM_RESERVED | VM_DONTEXPAND;
   vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
   instance->mmap_base = (void *)vma->vm_start;

   return 0;
}

static int vchiq_open(struct inode *inode, struct file *file)
{
   int dev = iminor(inode) & 0x0f;

   switch (dev)
   {
   case VCHIQ_MINOR:
      {
         VCHIQ_INSTANCE_T instance;

         if (g_state.remote->initialised != 1)
            return -ENOTCONN;

         instance = kzalloc(sizeof(*instance), GFP_KERNEL);
         if (!instance)
            return -ENOMEM;

         instance->state = &g_state;
         vcos_event_create(&instance->insert_event, "vchiq");
         vcos_event_create(&instance->remove_event, "vchiq");

         file->private_data = instance;
      }
      break;

   default:
      DPRINTK(KERN_ERR "VCHIQ driver: Unknown minor device: %d\n", dev);
      return -ENXIO;
   }

   return 0;
}

static int vchiq_release(struct inode *inode, struct file *file)
{
   int dev = iminor(inode) & 0x0f;
   int ret = 0;

   switch (dev)
   {
   case VCHIQ_MINOR:
      {
         VCHIQ_INSTANCE_T instance = file->private_data;
         int i;

         DPRINTK("vchiq_release: instance=%lx\n", (unsigned long)instance);

         instance->closing = 1;

         /* Remove all services */
         for (i = 0; i < MAX_SERVICES; i++)
         {
            USER_SERVICE_T *user_service = &instance->services[i];
            if (user_service->service != NULL)
               vchiq_terminate_service_internal(user_service->service);
         }

         /* Free any uncompleted bulk buffers */
         for (i = instance->completion_remove; i < instance->completion_insert; i++)
         {
            VCHIQ_COMPLETION_DATA_T *completion = &instance->completions[i & (MAX_COMPLETIONS - 1)];
            if (completion->bulk_userdata != NULL)
              free_pagelist((PAGELIST_T *)completion->bulk_userdata);
         }

         vcos_event_delete(&instance->insert_event);
         vcos_event_delete(&instance->remove_event);

         kfree(instance);
         file->private_data = NULL;
      }
      break;

   default:
      DPRINTK(KERN_ERR "VCHIQ driver: Unknown minor device: %d\n", dev);
      ret = -ENXIO;
   }

   return ret;
}

static struct irqaction vchiq_doorbell_irqaction =
{
     .name              = "VCHIQ Doorbell IRQ",
     .flags             = IRQF_DISABLED | IRQF_IRQPOLL,
     .handler           = vchiq_doorbell_irq,
};


static const struct file_operations vchiq_fops = {
   .owner   = THIS_MODULE,
   .unlocked_ioctl = vchiq_ioctl,
   .open    = vchiq_open,
   .release = vchiq_release,
   .mmap    = vchiq_mmap,
};

static int __init vchiq_init(void)
{
   int err = -ENODEV;
   int i;

   /* Allocate space for the channels in coherent memory */
   g_channel_mem = dma_alloc_coherent(NULL,
                                      sizeof(VCHIQ_CHANNEL_T) * 2 + sizeof(FRAGMENTS_T) * MAX_FRAGMENTS + CACHE_LINE_SIZE + (PAGE_SIZE - 1),
                                      &g_channel_phys,
                                      GFP_ATOMIC);

   if (!g_channel_mem)
   {
      printk(KERN_ERR "vchiq driver: Unable to allocate channel memory\n");
      err = -ENOMEM;
      goto out_err;
   }

   g_channels = (VCHIQ_CHANNEL_T *)((size_t)(g_channel_mem + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1));

   g_fragments_base = (FRAGMENTS_T *)( (((unsigned long)&g_channels[2]) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1));
   g_free_fragments = g_fragments_base;
   for (i = 0; i < (MAX_FRAGMENTS - 1); i++)
   {
      *(FRAGMENTS_T **)&g_fragments_base[i] = &g_fragments_base[i+1];
   }
   *(FRAGMENTS_T **)&g_fragments_base[i] = NULL;

   sema_init(&g_free_fragments_sema, MAX_FRAGMENTS);

   /* Arrange that the remote channel gets the page-aligned address, so that
      it's data buffer can be mapped into the local user space. */
   vchiq_init_state(&g_state, &g_channels[1], &g_channels[0]);

   g_channels[0].initialised = 0; /* Ensure the flag is clear */

   if ((err = register_chrdev(VCHIQ_MAJOR, "vchiq", &vchiq_fops)) != 0)
   {
      printk(KERN_ERR "vchiq driver: Unable to register driver\n");
      goto out_free_coherent;
   }

   vchiq_class = class_create(THIS_MODULE, "vchiq");
   if (IS_ERR(vchiq_class)) {
      err = PTR_ERR(vchiq_class);
      goto out_chrdev;
   }
   device_create(vchiq_class, NULL, MKDEV(VCHIQ_MAJOR, 0), NULL, "vchiq");
   setup_irq(IRQ_ARM_DOORBELL_0, &vchiq_doorbell_irqaction);

   /* Send the base address of the channel array to VideoCore */
   bcm_mailbox_write(MBOX_CHAN_VCHIQ, (unsigned int)g_channel_phys);
   DPRINTK("vchiq_init - done (channels %x, phys %x)\n",
           (unsigned int)g_channels, g_channel_phys);

   printk("vchiq: Initialised\n");

   return 0;

out_chrdev:
   unregister_chrdev(VCHIQ_MAJOR, "vchiq");
out_free_coherent:
   dma_free_coherent(NULL,
                     sizeof(VCHIQ_CHANNEL_T) * 2 + sizeof(FRAGMENTS_T) * MAX_FRAGMENTS + CACHE_LINE_SIZE + (PAGE_SIZE - 1),
                     g_channel_mem, g_channel_phys);
out_err:
   return err;
}

static void __exit vchiq_exit(void)
{
   device_destroy(vchiq_class, MKDEV(VCHIQ_MAJOR, 0));
   class_destroy(vchiq_class);
   unregister_chrdev(MKDEV(VCHIQ_MAJOR, 0), "vchiq");
   dma_free_coherent(NULL,
                     sizeof(VCHIQ_CHANNEL_T) * 2 + sizeof(FRAGMENTS_T) * MAX_FRAGMENTS + CACHE_LINE_SIZE + (PAGE_SIZE - 1),
                     g_channel_mem, g_channel_phys);
}

subsys_initcall(vchiq_init);
module_exit(vchiq_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phil Elwell <pelwell@broadcom.com>");
