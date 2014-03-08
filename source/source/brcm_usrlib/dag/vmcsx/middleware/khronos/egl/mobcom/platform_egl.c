/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/egl/egl_platform.h"
#include "interface/khronos/include/EGL/egl.h" /* for native window types */
#include "interface/khronos/include/EGL/eglext.h"
#include "helpers/vc_image/vc_image.h"
#include <assert.h>

#define DIRECT_LCD
//#define V3D_LCD_DMA
#define SRC_IMAGE_MAX_WIDTH		800
#define SRC_IMAGE_MAX_HEIGHT	480

#ifdef __cplusplus
extern "C" {
#endif

#include "mobcom_types.h"

#ifdef DIRECT_LCD
	#include "osevent.h"
	#include "ossemaphore.h"
	#include "display_drv.h"
	#include "display_interface.h"
#else
	#include "display_api.h"
#endif

#include "osheap.h"
#include "gpio.h"
#include "gpio_drv.h"
#include "platform_config.h"

#ifdef __cplusplus
}
#endif

static bool                         displaying          = false;
static uint32_t                     current_win         = 0;
static EGL_SERVER_RETURN_CALLBACK_T egl_callback        = NULL;
static uint32_t*                    pEglDispFrameBuffer = NULL;
extern void* burst_memcpy(void *dest, const void *src, unsigned int size);


#ifdef DIRECT_LCD

#ifdef V3D_LCD_DMA
#define NUM_V3D_BUFFERS			3
static MEM_HANDLE_T v3d_mem_handle[NUM_V3D_BUFFERS]; 
static int							currentRenderHandle	= 0;
static int							currentReleaseHandle	= 0;
static uint8_t*                    pEglV3DFrameBuffer = NULL;
#endif


#define NUM_DISP_BUFFERS			2
#pragma arm section zidata="uncacheable"
__align(32) static UInt16  v3d_disp_FrameBuff0[SRC_IMAGE_MAX_WIDTH*SRC_IMAGE_MAX_HEIGHT];
__align(32) static UInt16  v3d_disp_FrameBuff1[SRC_IMAGE_MAX_WIDTH*SRC_IMAGE_MAX_HEIGHT];
#pragma arm section


static uint32_t                     pEglDispInitialized = 0;
static DISPDRV_T*                   pEglTstDispDrv      = NULL;
static DISPDRV_HANDLE_T             pEglTstDispHdl;
static DISPDRV_OPEN_PARM_T          pEglTstDispOpenParam;
static const DISPDRV_INFO_T*        pEglDispInfo        = NULL;
static Semaphore_t                  EglDispUpdateRdy    = NULL;
static uint32_t*                    pEglDispBuffers[NUM_DISP_BUFFERS];
static int							currentDispBuffer	= 0;

#define	DST_WIDTH					pEglDispInfo->width
#define	DST_HEIGHT					pEglDispInfo->height
#define	DST_FORMAT_565				(pEglDispInfo->input_format ==  DISPDRV_FB_FORMAT_RGB565)

// ---- Utility Functions --------------------------------------------------------
static void display_callback(void)
{
    if (EglDispUpdateRdy)
        OSSEMAPHORE_Release(EglDispUpdateRdy);
}

static void display_callback_api_1_1(DISPDRV_CB_RES_T res, void *pFb)
{
    if (EglDispUpdateRdy)
    {
        OSSEMAPHORE_Release(EglDispUpdateRdy);
#ifdef V3D_LCD_DMA
        if (v3d_mem_handle[currentReleaseHandle] != MEM_INVALID_HANDLE)
            mem_unlock(v3d_mem_handle[currentReleaseHandle]);

        currentReleaseHandle = (currentReleaseHandle + 1) % NUM_V3D_BUFFERS;
#endif
    }
}

static void display_init(void)
{
    uint32_t i = 0, j = 0, width = 0, height = 0;
    
    // check wether the main display is initialized
    if (pEglDispInitialized  != 0)
        return;
    else
        pEglDispInitialized = 1;
        
    // create semaphore
    EglDispUpdateRdy = OSSEMAPHORE_Create(1, OSSUSPEND_PRIORITY); 
    OSSEMAPHORE_ChangeName(EglDispUpdateRdy, "EGLDISPRDY");
    assert(EglDispUpdateRdy != NULL);   
        
    // gpio init    
    GPIODRV_Init();

    // turn on backlight
    GPIODRV_Set_Mode(HAL_LCD_BACKLIGHT_EN, 1);
    GPIODRV_Set_Mode(HAL_TOUCH_SCREEN_BACKLIGHT_EN, 1);
    GPIODRV_Set_Bit(HAL_LCD_BACKLIGHT_EN, 1);
    GPIODRV_Set_Bit(HAL_TOUCH_SCREEN_BACKLIGHT_EN, 1);  
    
    // get function table for main display
	pEglTstDispDrv = DISPLAY_Get_Disp_Drv_Func_Table(DISPLAY_INTERFACE_ID_MAIN, NULL);
//	pEglTstDispDrv = DISPLAY_Get_Disp_Drv_Func_Table(DISPLAY_INTERFACE_ID_SUB, NULL);
    if (!pEglTstDispDrv) assert(0); 
    
    // init 
    if (pEglTstDispDrv->init() != 0) assert(0); 

    // open 
    pEglTstDispOpenParam.busId = (uint32_t)DISP_PRI_BUS;
    pEglTstDispOpenParam.busCh = (uint32_t)DISP_PRI_CH;
    if (pEglTstDispDrv->open((void*)&pEglTstDispOpenParam, &pEglTstDispHdl) != 0) assert(0);
    assert(pEglTstDispHdl!= NULL);
    
    // get info
    pEglDispInfo = pEglTstDispDrv->get_info(pEglTstDispHdl);
    assert(pEglDispInfo != NULL);

    // fill the blue color for the frame buffer
    if (pEglDispInfo->input_format == DISPDRV_FB_FORMAT_RGB888_U)
    {
		j = pEglDispInfo->width * pEglDispInfo->height * 4;
    } 
	else if (pEglDispInfo->input_format == DISPDRV_FB_FORMAT_RGB565)
	{
		j = pEglDispInfo->width * pEglDispInfo->height * 2;
	}
	else
        {
		assert(0);
	}
        
    // updaate the frame buffer
    pEglTstDispDrv->power_control(pEglTstDispHdl, DISPLAY_POWER_STATE_ON);

	pEglDispBuffers[0] = (uint32_t*)v3d_disp_FrameBuff0;
	pEglDispBuffers[1] = (uint32_t*)v3d_disp_FrameBuff1;
	
	currentDispBuffer = 0;
	pEglDispFrameBuffer = pEglDispBuffers[currentDispBuffer];
}

static void display_update(
    VC_IMAGE_T*         image)
{
    // wait until previous update is finished

    OSSEMAPHORE_Obtain(EglDispUpdateRdy, TICKS_FOREVER);

	pEglTstDispDrv->start(pEglTstDispHdl);
#ifdef V3D_LCD_DMA
	pEglTstDispDrv->update_dma_os(pEglTstDispHdl, pEglV3DFrameBuffer, display_callback_api_1_1);
#else
    pEglTstDispDrv->update_dma_os(pEglTstDispHdl, pEglDispFrameBuffer, display_callback_api_1_1);
	currentDispBuffer = (currentDispBuffer + 1) % NUM_DISP_BUFFERS;
	pEglDispFrameBuffer = pEglDispBuffers[currentDispBuffer];
#endif
}

#else

#define	EGL_LAYER				20
#define	DST_WIDTH				display_info->width
#define	DST_HEIGHT				display_info->height
#define	DST_FORMAT_565			(display_info->image_type ==  DOBJ_IMAGE_RGB565)

static DOBJ_HANDLE	egl_obj_handle	= 0;
static DISPLAY_T*	display_info	= NULL;

static void display_init(void)
{
	// gpio init
	GPIODRV_Init();

	// turn on backlight
	GPIODRV_Set_Mode(HAL_LCD_BACKLIGHT_EN, 1);
	GPIODRV_Set_Mode(HAL_TOUCH_SCREEN_BACKLIGHT_EN, 1);
	GPIODRV_Set_Bit(HAL_LCD_BACKLIGHT_EN, 1);
	GPIODRV_Set_Bit(HAL_TOUCH_SCREEN_BACKLIGHT_EN, 1);

	DISPLAY_Init();

	// Create LCD obj
	egl_obj_handle = DISPLAY_Obj_Create(EGL_LAYER);
	assert(egl_obj_handle);

	pEglDispFrameBuffer = (uint32_t*)OSHEAP_Alloc(SRC_IMAGE_MAX_WIDTH*SRC_IMAGE_MAX_HEIGHT*4);
	assert(pEglDispFrameBuffer);

	display_info = DISPMAN_Display(DISP_ID_MAIN_LCD);
}

static void display_update(
    VC_IMAGE_T*		image)
{
	Boolean			update_on = TRUE;
	DISP_ID_T		disp_id	  =	DISP_ID_MAIN_LCD;
	DOBJ_RECT_T		src_rect;
	DOBJ_RECT_T		dst_rect;
	DOBJ_IMAGE_T	src;

	src_rect.x_offset		= 0;
	src_rect.y_offset		= 0;
	src_rect.width			= image->width;
	src_rect.height			= image->height;

	dst_rect.x_offset		= 0;
	dst_rect.y_offset		= 0;
	dst_rect.width			= display_info->width;
	dst_rect.height			= display_info->height;

	src.type				= DOBJ_IMAGE_RGBA32;
	src.width				= image->width;
	src.height				= image->height;
	src.stride				= image->width*4;
	src.size				= src.height*src.stride;
	src.image_data			= pEglDispFrameBuffer;
	src.b_transparent		= 0;
	src.transparent_color	= 0;
	src.alpha				= 0xff;
	src.blend_color			= 0xff;

	DISPLAY_Obj_Update(egl_obj_handle, update_on, disp_id, &src, &src_rect, &dst_rect, DOBJ_IMAGE_ROT0);
}

#endif

static void	khrn_image_copy(
	KHRN_IMAGE_WRAP_T*	dst,
	KHRN_IMAGE_WRAP_T*	src)
{
	uint32_t	dst_stride	= dst->stride;
	uint32_t	src_stride	= src->stride; 
	uint32_t	op_w_size	= (dst->format == RGB_565_RSO) ? dst->width*2 : dst->width*4; 
	uint32_t	op_h_size	= dst->height; 
	uint8_t*	p_dst_addr	= (uint8_t*)dst->storage;
	uint8_t*	p_src_addr	= (uint8_t*)src->storage;
	uint32_t	nSize = op_w_size * op_h_size;

	assert(dst->format == RGB_565_RSO || dst->format == ARGB_8888_RSO);
	assert(op_h_size);

	burst_memcpy(p_dst_addr, p_src_addr, nSize);
/*
	while (op_h_size--)
	{
		memcpy(p_dst_addr, p_src_addr, op_w_size);

		p_dst_addr += dst_stride;
		p_src_addr += src_stride; 
	}
*/
}

static void vc_image_to_ARGB_8888_RSO_memory(
    VC_IMAGE_T*         image)
{
    KHRN_IMAGE_FORMAT_T format;
    uint16_t            width, height;
    KHRN_IMAGE_WRAP_T   src, dst;
    uint8_t*            src_ptr;  

    src_ptr = (image->mem_handle != MEM_INVALID_HANDLE) ? (uint8_t*)mem_lock(image->mem_handle) : (uint8_t*)image->image_data;
	
    // Swap red and blue. Do t-format conversion if necessary.
	switch (image->type) 
	{
		case VC_IMAGE_TF_RGBA32:	format = ABGR_8888_TF;	break;
		case VC_IMAGE_TF_RGBX32:	format = XBGR_8888_TF;  break;
		case VC_IMAGE_TF_RGB565:	format = RGB_565_TF;	break;
		case VC_IMAGE_TF_RGBA5551:  format = RGBA_5551_TF;  break;
		case VC_IMAGE_TF_RGBA16:	format = RGBA_4444_TF;  break;
		case VC_IMAGE_RGBA32:		format = ABGR_8888_RSO; break;
		case VC_IMAGE_RGB565:		format = RGB_565_RSO;   break;
		default:					UNREACHABLE();
	}
	
	width = image->width > DST_WIDTH ? DST_WIDTH : image->width;
	height = image->height > DST_HEIGHT ? DST_HEIGHT : image->height;

    khrn_image_wrap(&src, format, width, height, image->pitch, src_ptr);

#ifdef V3D_LCD_DMA
	v3d_mem_handle[currentRenderHandle] = image->mem_handle; 
	currentRenderHandle = (currentRenderHandle + 1) % NUM_V3D_BUFFERS;
	pEglV3DFrameBuffer = src_ptr;
#else
	if (DST_FORMAT_565)
		khrn_image_wrap(&dst, RGB_565_RSO,   width, height, DST_WIDTH*2, pEglDispFrameBuffer);
    else
		khrn_image_wrap(&dst, ARGB_8888_RSO, width, height, DST_WIDTH*4, pEglDispFrameBuffer);

	if (src.format == dst.format)
		khrn_image_copy(&dst, &src);
	else
		khrn_image_wrap_convert(&dst, &src, IMAGE_CONV_GL);

    if (image->mem_handle != MEM_INVALID_HANDLE) 
        mem_unlock(image->mem_handle);
#endif
}

void egl_server_platform_display(
    uint32_t		win, 
    KHRN_IMAGE_T*	image, 
    uint32_t		cb_arg)
{
    VC_IMAGE_T		vc_image;
	static int		disp = 1;

    assert(win != EGL_PLATFORM_WIN_NONE);

    if (!displaying) 
    {
        displaying = true;
        current_win = win;
    }

    khrn_image_fill_vcimage(image, &vc_image);

//	assert(win == current_win);    //Can only display on a single window
    
	if (disp)
	{
		assert(vc_image.type == VC_IMAGE_TF_RGBA32 || vc_image.type == VC_IMAGE_RGBA32 || 
			   vc_image.type == VC_IMAGE_TF_RGBX32 || vc_image.type == VC_IMAGE_RGB565);
		assert(vc_image.width * vc_image.height <= SRC_IMAGE_MAX_WIDTH * SRC_IMAGE_MAX_HEIGHT);

		vc_image_to_ARGB_8888_RSO_memory(&vc_image);
                                            
		// update the display
		display_update(&vc_image); 
	}

    /* May as well call callback immediately */
    egl_callback(cb_arg);
}

void egl_server_platform_display_nothing(
    uint32_t    win, 
    uint32_t    cb_arg)
{
	assert(win != EGL_PLATFORM_WIN_NONE);
//	assert(!displaying || (win == current_win));
    displaying = false;

#ifndef DIRECT_LCD
	if (egl_obj_handle)
	{
		DISPLAY_Obj_Delete(egl_obj_handle);
		egl_obj_handle = 0;
	}
#endif

	egl_callback(cb_arg);
}

void egl_server_platform_display_nothing_sync(uint32_t win)
{
   //TODO: wait for thread to finish?
   vcos_assert(win != EGL_PLATFORM_WIN_NONE);
// vcos_assert(!displaying || (win == current_win));
   displaying = false;

//   SendMessage(the_hwnd, WM_CLOSE, 0, 0);
}

void egl_server_platform_set_position(uint32_t handle, uint32_t position, uint32_t width, uint32_t height)
{
}

MEM_HANDLE_T egl_server_platform_create_pixmap_info(
    uint32_t			pixmap)
{
	VC_IMAGE_T*			vcimage		= (VC_IMAGE_T *)pixmap;
	KHRN_IMAGE_FORMAT_T format		= 0;
	MEM_HANDLE_T		data_handle;
	MEM_HANDLE_T		handle;

	vcos_assert(pixmap);

	switch (vcimage->type) 
	{
	case VC_IMAGE_TF_RGBA32:	format = ABGR_8888_TF;  break;
	case VC_IMAGE_TF_RGBX32:	format = XBGR_8888_TF;  break;
	case VC_IMAGE_TF_RGB565:	format = RGB_565_TF;	break;
	case VC_IMAGE_TF_RGBA5551:  format = RGBA_5551_TF;  break;
	case VC_IMAGE_TF_RGBA16:	format = RGBA_4444_TF;  break;
#ifdef __VIDEOCORE4__
	case VC_IMAGE_RGBA32:		format = ABGR_8888_RSO; break;
	case VC_IMAGE_RGB565:		format = RGB_565_RSO;   break;
#endif
	default:					UNREACHABLE();
	}

	if (vcimage->mem_handle != MEM_INVALID_HANDLE)
	{
		data_handle = vcimage->mem_handle;
		mem_acquire(data_handle);
	}
	else
		data_handle = mem_wrap(vcimage->image_data, vcimage->size, 64, MEM_FLAG_NONE, "egl_server_platform_create_pixmap_info");

	if (data_handle == MEM_INVALID_HANDLE) 
	{
		return MEM_INVALID_HANDLE;
	}

	handle = khrn_image_create_from_storage(
				format,	vcimage->width, vcimage->height, vcimage->pitch,
				MEM_INVALID_HANDLE, data_handle, 0,
				(KHRN_IMAGE_CREATE_FLAG_T)(IMAGE_CREATE_FLAG_TEXTURE | IMAGE_CREATE_FLAG_RENDER_TARGET)); /* todo: are these flags right? */

	mem_release(data_handle);

	return handle;
}

void egl_server_platform_init(
    EGL_SERVER_RETURN_CALLBACK_T return_callback)
{
    egl_callback = return_callback;
    display_init();
}

void egl_server_platform_shutdown(void)
{
#ifndef DIRECT_LCD
	if (egl_obj_handle)
	{
		DISPLAY_Obj_Delete(egl_obj_handle);
		egl_obj_handle = 0;
	}
#endif
}

#if EGL_NOK_image_endpoint
void egl_server_platform_read_surface(uint32_t handle, struct VC_IMAGE_T *image)
{
//   vclib_obtain_VRF(1);
//   vclib_memset4(image->image_data, 0xffff00ff, image->size / 4);
//   vclib_release_VRF();
	assert(0);
}
#endif
