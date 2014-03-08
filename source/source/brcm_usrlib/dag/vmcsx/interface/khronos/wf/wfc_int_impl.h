/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited.
All rights reserved.

Project  :  khronos
Module   :  VG server

FILE DESCRIPTION
Top-level VG server-side functions.
=============================================================================*/

#if defined(KHRN_IMPL_STRUCT)
#define FN(type, name, args) type (*name) args;
#elif defined(KHRN_IMPL_STRUCT_INIT)
#define FN(type, name, args) name,
#else
#define FN(type, name, args) extern type name args;
#endif

FN(uint32_t, wfcIntCreateContext_impl,
   (WFCContext context, uint32_t context_type, uint32_t screen_or_stream_num))
FN(void, wfcIntDestroyContext_impl, (WFCContext ctx))
FN(bool, wfcIntCommitBegin_impl, (WFCContext context, bool wait, WFCRotation rotation,
   WFCfloat background_red, WFCfloat background_green, WFCfloat background_blue,
   WFCfloat background_alpha))
FN(void, wfcIntCommitAdd_impl,
   (WFCContext ctx, WFCElement element, WFC_ELEMENT_ATTRIB_T *element_attr,
      WFCNativeStreamType source_stream, WFCNativeStreamType mask_stream))
FN(void, wfcIntCommitEnd_impl, (WFCContext ctx))
FN(void, wfcActivate_impl, (WFCContext ctx))
FN(void, wfcDeactivate_impl, (WFCContext ctx))
FN(void, wfcCompose_impl, (WFCContext context))

FN(void, wfc_stream_server_create,
   (WFCNativeStreamType stream, VC_IMAGE_TYPE_T type, uint32_t width,
      uint32_t height, uint32_t flags, uint32_t pid_lo, uint32_t pid_hi))
FN(void, wfc_stream_server_destroy, (WFCNativeStreamType stream))
FN(void, wfc_stream_server_signal_buffer_avail, (WFCNativeStreamType stream))
FN(void, wfc_stream_server_signal_buffer_removed, (WFCNativeStreamType stream))
FN(uint32_t, wfc_stream_server_queue_image_data,
   (WFCNativeStreamType stream, VC_IMAGE_T *image,
      WFC_STREAM_SERVER_RENDERER_CALLBACK_T cb_func, uint32_t cb_arg))
#undef FN
