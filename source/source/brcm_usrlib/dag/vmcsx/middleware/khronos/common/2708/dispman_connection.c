#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <asm-generic/ioctl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <unistd.h>
#include <fcntl.h>

#include "vc/include/interface/vmcs_host/vc_dispmanx.h"

//#ifdef __ARMEL__
#if 0

/*
 * dispman_connection.c
 *
 *  Created on: 7 Mar 2014
 *      Author: simon
 */


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

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
    } buffers[BUFFERS];

} RECT_VARS_T;

static int current_buffer = 0;
static RECT_VARS_T  gRectVars;

void init_dispmanx(int type, int width, int height)
{
	uint32_t        screen = 0;
	int             ret;
	int count;
	VC_RECT_T       rect;
	VC_DISPMANX_ALPHA_T alpha = { DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS,
						 255,
						 0 };

	printf("init dispmanx type %d dims %dx%d\n", type, width, height);

	printf("Open display[%i]...\n", screen );
	gRectVars.display = vc_dispmanx_display_open( screen );

	ret = vc_dispmanx_display_get_info( gRectVars.display, &gRectVars.info);
	assert(ret == 0);
	printf( "Display is %d x %d\n", gRectVars.info.width, gRectVars.info.height );

	for (count = 0; count < BUFFERS; count++)
	{
		gRectVars.buffers[count].resource = vc_dispmanx_resource_create( type,
											  width,
											  height,
											  &gRectVars.buffers[count].vc_image_ptr );
	}

	gRectVars.update = vc_dispmanx_update_start( 10 );
	assert( gRectVars.update );

	vc_dispmanx_rect_set( &rect, 0, 0, width, height);

	gRectVars.element = vc_dispmanx_element_add(    gRectVars.update,
												gRectVars.display,
												2000,               // layer
												&rect,
												gRectVars.buffers[count].resource,
												&rect,
												DISPMANX_PROTECTION_NONE,
												&alpha,
												NULL,             // clamp
												VC_IMAGE_ROT0 );

	ret = vc_dispmanx_update_submit_sync( gRectVars.update );
	assert( ret == 0 );

}

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
