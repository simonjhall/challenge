#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <asm-generic/ioctl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <unistd.h>
#include <fcntl.h>

#define MAJOR_NUM 100
#define IOCTL_MBOX_PROPERTY _IOWR(MAJOR_NUM, 0, char *)
#define DEVICE_FILE_NAME "char_dev"

#include "vc/include/interface/vmcs_host/vc_dispmanx.h"

#ifdef __ARMEL__
//#if 0

/*
 * dispman_connection.c
 *
 *  Created on: 7 Mar 2014
 *      Author: simon
 */


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static unsigned int get_resource(int mailbox, DISPMANX_RESOURCE_HANDLE_T handle)
{
	struct vc_msg
	{
		unsigned int m_msgSize;
		unsigned int m_response;

		struct vc_tag
		{
			unsigned int m_tagId;
			unsigned int m_sendBufferSize;
			union {
				unsigned int m_sendDataSize;
				unsigned int m_recvDataSize;
			};

			struct args
			{
				union {
					struct {
						DISPMANX_RESOURCE_HANDLE_T m_in;
					} m_in;
					struct {
						unsigned int m_success;
						unsigned int m_out;
					} m_out;
				};
			} m_args;
		} m_tag;

		unsigned int m_endTag;
	} msg;

	msg.m_msgSize = sizeof(msg);
	msg.m_response = 0;
	msg.m_endTag = 0;

	msg.m_tag.m_tagId = 0x30014;
	msg.m_tag.m_sendBufferSize = 8;
	msg.m_tag.m_sendDataSize = 8;

	msg.m_tag.m_args.m_in.m_in = handle;
	msg.m_tag.m_args.m_out.m_out = 0;

	if (ioctl(mailbox, IOCTL_MBOX_PROPERTY, &msg) < 0)
		assert(0);

	assert(msg.m_response == 0x80000000);
	assert(msg.m_tag.m_recvDataSize == 0x80000008);

	return msg.m_tag.m_args.m_out.m_out;
}

static void *lock(int mailbox, unsigned int handle)
{
	struct vc_msg
	{
		unsigned int m_msgSize;
		unsigned int m_response;

		struct vc_tag
		{
			unsigned int m_tagId;
			unsigned int m_sendBufferSize;
			union {
				unsigned int m_sendDataSize;
				unsigned int m_recvDataSize;
			};

			struct args
			{
				union {
					unsigned int m_handle;
					unsigned int m_busAddress;
				};
			} m_args;
		} m_tag;

		unsigned int m_endTag;
	} msg;
	int s;

	msg.m_msgSize = sizeof(msg);
	msg.m_response = 0;
	msg.m_endTag = 0;

	//fill in the tag for the lock command
	msg.m_tag.m_tagId = 0x3000d;
	msg.m_tag.m_sendBufferSize = 4;
	msg.m_tag.m_sendDataSize = 4;

	//pass across the handle
	msg.m_tag.m_args.m_handle = handle;

	if (ioctl(mailbox, IOCTL_MBOX_PROPERTY, &msg) < 0)
		assert(0);

	assert(msg.m_response == 0x80000000);
	assert(msg.m_tag.m_recvDataSize == 0x80000004);

	return (void *)msg.m_tag.m_args.m_busAddress;
}


#define BUFFERS 1

typedef struct
{
    DISPMANX_DISPLAY_HANDLE_T   display;
    DISPMANX_MODEINFO_T         info;
    DISPMANX_UPDATE_HANDLE_T    update;
    DISPMANX_ELEMENT_HANDLE_T   element;

    struct inner
    {
        DISPMANX_RESOURCE_HANDLE_T  resource;
        uint32_t                    vc_image_ptr;
        void *m_pMemory;
    } buffers[BUFFERS];

} RECT_VARS_T;

static int current_buffer = 0;
static RECT_VARS_T  gRectVars;

void init_dispmanx(int type, int width, int height)
{
	int mailbox;
	uint32_t        screen = 0;
	int             ret;
	int count;
	VC_RECT_T       rect, src_rect;
	VC_DISPMANX_ALPHA_T alpha = { DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS,
						 255,
						 0 };
	void *temp;

	printf("Open display[%i]...\n", screen );
	gRectVars.display = vc_dispmanx_display_open( screen );

	ret = vc_dispmanx_display_get_info( gRectVars.display, &gRectVars.info);
	assert(ret == 0);
	printf( "Display is %d x %d\n", gRectVars.info.width, gRectVars.info.height );

	if (width == -1 && height == -1)
	{
		width = gRectVars.info.width;
		height = gRectVars.info.height;
	}

	printf("init dispmanx type %d dims %dx%d\n", type, width, height);


	mailbox = open(DEVICE_FILE_NAME, 0);
	assert(mailbox >= 0);

	for (count = 0; count < BUFFERS; count++)
	{
		unsigned int handle;

		gRectVars.buffers[count].resource = vc_dispmanx_resource_create( type,
											  width,
											  height,
											  &gRectVars.buffers[count].vc_image_ptr );
		assert(gRectVars.buffers[count].resource);

		handle = get_resource(mailbox, gRectVars.buffers[count].resource);

		gRectVars.buffers[count].m_pMemory = lock(mailbox, handle);

		printf("image mapped in at %p\n", gRectVars.buffers[count].m_pMemory);
	}

	gRectVars.update = vc_dispmanx_update_start( 10 );
	assert( gRectVars.update );

	gRectVars.element = vc_dispmanx_element_add(    gRectVars.update,
												gRectVars.display,
												2000,               // layer
												&rect,
												gRectVars.buffers[0].resource,
												&src_rect,
												DISPMANX_PROTECTION_NONE,
												&alpha,
												NULL,             // clamp
												VC_IMAGE_ROT0 );

	ret = vc_dispmanx_update_submit_sync( gRectVars.update );
	assert( ret == 0 );

	//weirdly we appear to need to touch all pixels before the image appears on screen...!
	{
		FILE *dev_mem;
		unsigned int pa = (unsigned int)gRectVars.buffers[0].m_pMemory;
		void *address;
		int count;

		dev_mem = fopen("/dev/mem", "r+b");
		assert(dev_mem);



		address=mmap(0, ((width * height * 2) + 4095) & ~4095, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(dev_mem), pa & ~4095);
		assert(address);
		assert(address != (void *)-1);
//
//		while (1)
//			for (count = 0; count < 256; count++)
//				memset((void *)((unsigned int)address + (pa & 4095)), count, 1920 * 1080 * 4);

		memset((void *)((unsigned int)address + (pa & 4095)), 0xff, width * height * 2);

		fclose(dev_mem);
	}

	close(mailbox);
}

#if 0

void update_dispmanx_image(void *p, VC_IMAGE_TYPE_T type, int width, int height, int pitch)
{
//	static int first_usage = 1;
//	if (first_usage)
//	{
//		first_usage = 0;
//		init_dispmanx(type, width, height);
//	}

   VC_RECT_T       dst_rect;
   int ret;

   printf("updating an image from %p type %d size %dx%d pitch %d\n", p, type, width, height, pitch);

   void *temp = malloc(pitch * height);
   memcpy(temp, p, pitch * height);

   current_buffer++;
   if (current_buffer == BUFFERS)
      current_buffer = 0;

   vc_dispmanx_rect_set( &dst_rect, 0, 0, width, height);

   ret = vc_dispmanx_resource_write_data(gRectVars.buffers[current_buffer].resource,
                           type, pitch, temp, &dst_rect);
   if (ret != 0)
      printf("couldn\'t swap buffers\n");

   // begin display update
   gRectVars.update = vc_dispmanx_update_start( 10 );
   if (gRectVars.update == 0) {
     assert(0);
   }

   // change element source to be the current resource
   ret = vc_dispmanx_element_change_source( gRectVars.update, gRectVars.element, gRectVars.buffers[current_buffer].resource );

   // finish display update
   ret = vc_dispmanx_update_submit_sync( gRectVars.update );
   if (ret != 0) {
     assert(0);
   }

   free(temp);
}

#else

void *get_image_buffer(int b)
{
	return (void *)(((unsigned int)gRectVars.buffers[0].m_pMemory & ~0xc0000000) | 0x80000000);
}

void update_dispmanx_image(void *p, VC_IMAGE_TYPE_T type, int width, int height, int pitch)
{
  /* int ret;

   printf("updating an image from %p type %d size %dx%d pitch %d\n", p, type, width, height, pitch);

   current_buffer++;
   if (current_buffer == BUFFERS)
      current_buffer = 0;

   // begin display update
   gRectVars.update = vc_dispmanx_update_start( 10 );
   if (gRectVars.update == 0) {
     assert(0);
   }

   // change element source to be the current resource
   ret = vc_dispmanx_element_change_source( gRectVars.update, gRectVars.element, gRectVars.buffers[current_buffer].resource );

   // finish display update
   ret = vc_dispmanx_update_submit_sync( gRectVars.update );
   if (ret != 0) {
     assert(0);
   }*/
}
#endif

#else

void *get_image_buffer(int b)
{
	return 0;
}

static FILE *dev_fb;
unsigned char *fbPtr;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;

void init_dispmanx(int type, int width, int height)
{
#ifdef __ARMEL__
	printf("init dispmanx type %d dims %dx%d\n", type, width, height);

	dev_fb = fopen("/dev/fb0", "r+b");
	if (!dev_fb)
	{
		printf("could not open /dev/fb0\n");
		assert(0);
	}

	printf("framebuffer opened ok\n");

	if (ioctl(fileno(dev_fb), FBIOGET_FSCREENINFO, &finfo))
		assert(0);
	if (ioctl(fileno(dev_fb), FBIOGET_VSCREENINFO, &vinfo))
		assert(0);

	printf("screen %d x %d %d bpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

	fbPtr = (unsigned char *)mmap(0, finfo.smem_len,
				PROT_READ | PROT_WRITE, MAP_SHARED,
				fileno(dev_fb),
				0);
	assert(fbPtr);
#endif
}

void update_dispmanx_image(unsigned char *p, VC_IMAGE_TYPE_T type, int width, int height, int pitch)
{
#ifdef __ARMEL__
//	printf("would update an image from %p type %d size %dx%d pitch %d\n", p, type, width, height, pitch);

	if (!p)
	{
		printf("image to update with is null\n");
		assert(0);
	}

	if (pitch > vinfo.xres * (vinfo.bits_per_pixel >> 3))
	{
		printf("image pitch is larger than screen pitch! %dx%d, %d %d in\n", width, height, pitch, type);
		return;
	}

	if (height > vinfo.yres)
	{
		printf("image height is larger than screen height %d\n", height);
		return;
	}

	int x, y;
	for (y = 0; y < height; y++)
	{
		unsigned char *pRowSrc = &p[y * pitch];
		unsigned char *pRowDest = &fbPtr[y * vinfo.xres * (vinfo.bits_per_pixel >> 3)];

		memcpy(pRowDest, pRowSrc, pitch);
	}
#endif
}

#endif
