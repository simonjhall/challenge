/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  khronos
Module   :  Broadcom EGL extensions header

FILE DESCRIPTION
This is included at the end of our version of EGL/eglext.h. If you're using a
different EGL/eglext.h, you can include this after it
=============================================================================*/

#ifndef EGLEXT_BRCM_H
#define EGLEXT_BRCM_H

#ifdef __cplusplus
extern "C" {
#endif

#define EGL_NATIVE_PIXMAP_CLIENT_SIDE_BRCM 0x99930B0	/* eglCreateImageKHR server-side target */
#define EGL_IMAGE_FROM_SURFACE_BRCM 0x99930B1 /* eglCreateImageKHR server-side target */

#ifndef EGL_BRCM_global_image
#define EGL_BRCM_global_image 1
#define EGL_PIXEL_FORMAT_ARGB_8888_PRE_BRCM 0
#define EGL_PIXEL_FORMAT_ARGB_8888_BRCM     1
#define EGL_PIXEL_FORMAT_XRGB_8888_BRCM     2
#define EGL_PIXEL_FORMAT_RGB_565_BRCM       3
#define EGL_PIXEL_FORMAT_A_8_BRCM           4
#define EGL_PIXEL_FORMAT_RENDER_GL_BRCM     (1 << 3)
#define EGL_PIXEL_FORMAT_RENDER_GLES_BRCM   (1 << 4)
#define EGL_PIXEL_FORMAT_RENDER_GLES2_BRCM  (1 << 5)
#define EGL_PIXEL_FORMAT_RENDER_VG_BRCM     (1 << 6)
#define EGL_PIXEL_FORMAT_RENDER_MASK_BRCM   0x78
#define EGL_PIXEL_FORMAT_VG_IMAGE_BRCM      (1 << 7)
#define EGL_PIXEL_FORMAT_GLES_TEXTURE_BRCM  (1 << 8)
#define EGL_PIXEL_FORMAT_GLES2_TEXTURE_BRCM (1 << 9)
#define EGL_PIXEL_FORMAT_TEXTURE_MASK_BRCM  0x380
#define EGL_PIXEL_FORMAT_USAGE_MASK_BRCM    0x3f8
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI void EGLAPIENTRY eglCreateGlobalImageBRCM(EGLint width, EGLint height, EGLint pixel_format, const void *data, EGLint data_stride, EGLint *id);
EGLAPI void EGLAPIENTRY eglCreateCopyGlobalImageBRCM(const EGLint *src_id, EGLint *id);
EGLAPI EGLBoolean EGLAPIENTRY eglDestroyGlobalImageBRCM(const EGLint *id);
EGLAPI EGLBoolean EGLAPIENTRY eglQueryGlobalImageBRCM(const EGLint *id, EGLint *width_height_pixel_format);
#endif /* EGL_EGLEXT_PROTOTYPES */
typedef void (EGLAPIENTRYP PFNEGLCREATEGLOBALIMAGEBRCMPROC)(EGLint width, EGLint height, EGLint pixel_format, const void *data, EGLint data_stride, EGLint *id);
typedef void (EGLAPIENTRYP PFNEGLCREATECOPYGLOBALIMAGEBRCMPROC)(const EGLint *src_id, EGLint *id);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLDESTROYGLOBALIMAGEBRCMPROC)(const EGLint *id);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYGLOBALIMAGEBRCMPROC)(const EGLint *id, EGLint *width_height_pixel_format);
#endif

#ifndef EGL_BRCM_perf_monitor
#define EGL_BRCM_perf_monitor 0
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglInitPerfMonitorBRCM(EGLDisplay display);
EGLAPI EGLBoolean EGLAPIENTRY eglTermPerfMonitorBRCM(EGLDisplay display);
#endif /* EGL_EGLEXT_PROTOTYPES */
typedef void (EGLAPIENTRYP PFNEGLINITPERFMONITORBRCMPROC)(EGLDisplay display);
typedef void (EGLAPIENTRYP PFNEGLTERMPERFMONITORBRCMPROC)(EGLDisplay display);
#endif

#ifndef EGL_BRCM_driver_monitor
#define EGL_BRCM_driver_monitor 1
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglInitDriverMonitorBRCM(EGLDisplay display, EGLint hw_bank, EGLint l3c_bank);
EGLAPI void EGLAPIENTRY eglGetDriverMonitorXMLBRCM(EGLDisplay display, EGLint bufSize, EGLint *length, char *xmlStats);
EGLAPI EGLBoolean EGLAPIENTRY eglTermDriverMonitorBRCM(EGLDisplay display);
#endif /* EGL_EGLEXT_PROTOTYPES */
typedef void (EGLAPIENTRYP PFNEGLINITDRIVERMONITORBRCMPROC)(EGLDisplay display, EGLint hw_bank, EGLint l3c_bank);
typedef void (EGLAPIENTRYP PFNEGLGETDRIVERMONITORXMLBRCMPROC)(EGLDisplay display, EGLint bufSize, EGLint *length, char *xmlStats);
typedef void (EGLAPIENTRYP PFNEGLTERMDRIVERMONITORBRCMPROC)(EGLDisplay display);
#endif

#ifndef EGL_BRCM_perf_stats
#define EGL_BRCM_perf_stats 0
EGLAPI void eglPerfStatsResetBRCM();
EGLAPI void eglPerfStatsGetBRCM(char *buffer, EGLint buffer_len, EGLBoolean reset);
#endif

#ifndef EGL_BRCM_flush
#define EGL_BRCM_flush 1
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI void EGLAPIENTRY eglFlushBRCM(void);
#endif /* EGL_EGLEXT_PROTOTYPES */
typedef void (EGLAPIENTRYP PFNEGLFLUSHBRCMPROC)(void);
#endif

#ifndef EGL_BRCM_image_wrap
#define EGL_BRCM_image_wrap 1
#define EGL_IMAGE_WRAP_BRCM 0x9993140
#endif

/*
Only enable this #define if the application (or wrapper layer) is going to call eglDirectRenderingPointer
when appropriate (i.e. the first time eglMakeCurrent is called, and then at eglSwapBuffers)

Only used for testing purposes on 2763
*/

#ifdef DIRECT_RENDERING
EGLAPI EGLBoolean EGLAPIENTRY eglDirectRenderingPointer(EGLDisplay dpy, EGLSurface surf, void *image /* KHRN_IMAGE_WRAP_T */);
#endif

#ifdef __cplusplus
}
#endif

#endif
