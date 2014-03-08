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

#include "middleware/khronos/dispatch/khrn_dispatch_rpc.h"

#include "interface/khronos/common/khrn_int_ids.h"

#include "middleware/khronos/vg/vg_server.h"

void vg_dispatch(uint32_t id)
{
   switch (id) {
   case VGCLEARERROR_ID: /* RPC_CALL0 */
   {
      vgClearError_impl();
      break;
   }
   case VGSETERROR_ID: /* RPC_CALL1 */
   {
      VGErrorCode error = (VGErrorCode)khdispatch_read_enum();
      vgSetError_impl(error);
      break;
   }
   case VGGETERROR_ID: /* RPC_CALL0_RES */
   {
      khdispatch_write_enum(vgGetError_impl());
      break;
   }
   case VGFLUSH_ID: /* RPC_CALL0 */
   {
      vgFlush_impl();
      break;
   }
   case VGFINISH_ID: /* RPC_CALL0_RES */
   {
      khdispatch_write_uint(vgFinish_impl());
      break;
   }
   case VGCREATESTEMS_ID: /* RPC_CALL2_OUT_CTRL_RES */
   {
      VGuint count = khdispatch_read_uint();
      khdispatch_check_workspace(count * sizeof(VGHandle));
      VGHandle *stems = (VGHandle *)khdispatch_workspace;
      count = vgCreateStems_impl(count, stems);
      khdispatch_write_out_ctrl_res(stems, count * sizeof(VGHandle), count);
      break;
   }
   case VGDESTROYSTEM_ID: /* RPC_CALL1 */
   {
      VGHandle vg_handle = khdispatch_read_handle();
      vgDestroyStem_impl(vg_handle);
      break;
   }
   case VGSETIV_ID: /* RPC_CALL3_IN_CTRL */
   {
      VGParamType param_type = (VGParamType)khdispatch_read_enum();
      VGint count = khdispatch_read_int();
      const VGint *values = (const VGint *)khdispatch_read_ctrl(count * sizeof(VGint));
      vgSetiv_impl(param_type, count, values);
      break;
   }
   case VGSETFV_ID: /* RPC_CALL3_IN_CTRL */
   {
      VGParamType param_type = (VGParamType)khdispatch_read_enum();
      VGint count = khdispatch_read_int();
      const VGfloat *values = (const VGfloat *)khdispatch_read_ctrl(count * sizeof(VGfloat));
      vgSetfv_impl(param_type, count, values);
      break;
   }
   case VGGETFV_ID: /* RPC_CALL3_OUT_CTRL */
   {
      VGParamType param_type = (VGParamType)khdispatch_read_enum();
      VGint count = khdispatch_read_int();
      khdispatch_check_workspace(count * sizeof(VGfloat));
      VGfloat *values = (VGfloat *)khdispatch_workspace;
      vgGetfv_impl(param_type, count, values);
      khdispatch_write_out_ctrl(values, count * sizeof(VGfloat));
      break;
   }
   case VGSETPARAMETERIV_ID: /* RPC_CALL5_IN_CTRL */
   {
      VGHandle vg_handle = khdispatch_read_handle();
      VG_CLIENT_OBJECT_TYPE_T client_object_type = (VG_CLIENT_OBJECT_TYPE_T)khdispatch_read_enum();
      VGint param_type = khdispatch_read_int();
      VGint count = khdispatch_read_int();
      const VGint *values = (const VGint *)khdispatch_read_ctrl(count * sizeof(VGint));
      vgSetParameteriv_impl(vg_handle, client_object_type, param_type, count, values);
      break;
   }
   case VGSETPARAMETERFV_ID: /* RPC_CALL5_IN_CTRL */
   {
      VGHandle vg_handle = khdispatch_read_handle();
      VG_CLIENT_OBJECT_TYPE_T client_object_type = (VG_CLIENT_OBJECT_TYPE_T)khdispatch_read_enum();
      VGint param_type = khdispatch_read_int();
      VGint count = khdispatch_read_int();
      const VGfloat *values = (const VGfloat *)khdispatch_read_ctrl(count * sizeof(VGfloat));
      vgSetParameterfv_impl(vg_handle, client_object_type, param_type, count, values);
      break;
   }
   case VGGETPARAMETERIV_ID: /* RPC_CALL5_OUT_CTRL_RES */
   {
      VGHandle vg_handle = khdispatch_read_handle();
      VG_CLIENT_OBJECT_TYPE_T client_object_type = (VG_CLIENT_OBJECT_TYPE_T)khdispatch_read_enum();
      VGint param_type = khdispatch_read_int();
      VGint count = khdispatch_read_int();
      khdispatch_check_workspace(count * sizeof(VGint));
      VGint *values = (VGint *)khdispatch_workspace;
      bool success = vgGetParameteriv_impl(vg_handle, client_object_type, param_type, count, values);
      khdispatch_write_out_ctrl_res(values, success ? (count * sizeof(VGint)) : 0, success);
      break;
   }
   case VGLOADMATRIX_ID: /* RPC_CALL2_IN_CTRL */
   {
      VGMatrixMode matrix_mode = (VGMatrixMode)khdispatch_read_enum();
      const VG_MAT3X3_T *matrix = (const VG_MAT3X3_T *)khdispatch_read_ctrl(sizeof(VG_MAT3X3_T));
      vgLoadMatrix_impl(matrix_mode, matrix);
      break;
   }
   case VGMASK_ID: /* RPC_CALL6 */
   {
      VGHandle vg_handle = khdispatch_read_handle();
      VGMaskOperation operation = (VGMaskOperation)khdispatch_read_enum();
      VGint dst_x = khdispatch_read_int();
      VGint dst_y = khdispatch_read_int();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      vgMask_impl(vg_handle, operation, dst_x, dst_y, width, height);
      break;
   }
   case VGRENDERTOMASK_ID: /* RPC_CALL3 (vg 1.1) */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGbitfield paint_modes = khdispatch_read_bitfield();
      VGMaskOperation operation = (VGMaskOperation)khdispatch_read_enum();
      vgRenderToMask_impl(vg_handle, paint_modes, operation);
      break;
   }
   case VGCREATEMASKLAYER_ID: /* RPC_CALL3 (vg 1.1) */
   {
      VGHandle vg_handle = khdispatch_read_handle();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      vgCreateMaskLayer_impl(vg_handle, width, height);
      break;
   }
   case VGDESTROYMASKLAYER_ID: /* RPC_CALL1 (vg 1.1) */
   {
      VGMaskLayer vg_handle = khdispatch_read_handle();
      vgDestroyMaskLayer_impl(vg_handle);
      break;
   }
   case VGFILLMASKLAYER_ID: /* RPC_CALL6 (vg 1.1) */
   {
      VGMaskLayer vg_handle = khdispatch_read_handle();
      VGint x = khdispatch_read_int();
      VGint y = khdispatch_read_int();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      VGfloat value = khdispatch_read_float();
      vgFillMaskLayer_impl(vg_handle, x, y, width, height, value);
      break;
   }
   case VGCOPYMASK_ID: /* RPC_CALL7 (vg 1.1) */
   {
      VGMaskLayer dst_vg_handle = khdispatch_read_handle();
      VGint dst_x = khdispatch_read_int();
      VGint dst_y = khdispatch_read_int();
      VGint src_x = khdispatch_read_int();
      VGint src_y = khdispatch_read_int();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      vgCopyMask_impl(dst_vg_handle, dst_x, dst_y, src_x, src_y, width, height);
      break;
   }
   case VGCLEAR_ID: /* RPC_CALL4 */
   {
      VGint x = khdispatch_read_int();
      VGint y = khdispatch_read_int();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      vgClear_impl(x, y, width, height);
      break;
   }
   case VGCREATEPATH_ID: /* RPC_CALL8 */
   {
      VGHandle vg_handle = khdispatch_read_handle();
      VGint format = khdispatch_read_int();
      VGPathDatatype datatype = (VGPathDatatype)khdispatch_read_enum();
      VGfloat scale = khdispatch_read_float();
      VGfloat bias = khdispatch_read_float();
      VGint segments_capacity = khdispatch_read_int();
      VGint coords_capacity = khdispatch_read_int();
      VGbitfield caps = khdispatch_read_bitfield();
      vgCreatePath_impl(vg_handle, format, datatype, scale, bias, segments_capacity, coords_capacity, caps);
      break;
   }
   case VGCLEARPATH_ID: /* RPC_CALL2 */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGbitfield caps = khdispatch_read_bitfield();
      vgClearPath_impl(vg_handle, caps);
      break;
   }
   case VGDESTROYPATH_ID: /* RPC_CALL1 */
   {
      VGPath vg_handle = khdispatch_read_handle();
      vgDestroyPath_impl(vg_handle);
      break;
   }
   case VGREMOVEPATHCAPABILITIES_ID: /* RPC_CALL2 */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGbitfield caps = khdispatch_read_bitfield();
      vgRemovePathCapabilities_impl(vg_handle, caps);
      break;
   }
   case VGAPPENDPATH_ID: /* RPC_CALL2 */
   {
      VGPath dst_vg_handle = khdispatch_read_handle();
      VGPath src_vg_handle = khdispatch_read_handle();
      vgAppendPath_impl(dst_vg_handle, src_vg_handle);
      break;
   }
   case VGAPPENDPATHDATA_ID: /* custom */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGPathDatatype datatype = (VGPathDatatype)khdispatch_read_enum();
      VGint segments_count = khdispatch_read_int();
      VGint coords_size = khdispatch_read_int();
      const VGubyte *segments = (const VGubyte *)khdispatch_read_ctrl(segments_count);
      const void *coords = khdispatch_read_ctrl(coords_size);
      vgAppendPathData_impl(vg_handle, datatype, segments_count, segments, coords_size, coords);
      break;
   }
   case VGMODIFYPATHCOORDS_ID: /* custom */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGPathDatatype datatype = (VGPathDatatype)khdispatch_read_enum();
      VGuint coords_offset = khdispatch_read_uint();
      VGuint coords_size = khdispatch_read_uint();
      const void *coords = khdispatch_read_ctrl(coords_size);
      vgModifyPathCoords_impl(vg_handle, datatype, coords_offset, coords_size, coords);
      break;
   }
   case VGTRANSFORMPATH_ID: /* RPC_CALL2 */
   {
      VGPath dst_vg_handle = khdispatch_read_handle();
      VGPath src_vg_handle = khdispatch_read_handle();
      vgTransformPath_impl(dst_vg_handle, src_vg_handle);
      break;
   }
   case VGINTERPOLATEPATH_ID: /* RPC_CALL4 */
   {
      VGPath dst_vg_handle = khdispatch_read_handle();
      VGPath begin_vg_handle = khdispatch_read_handle();
      VGPath end_vg_handle = khdispatch_read_handle();
      VGfloat t = khdispatch_read_float();
      vgInterpolatePath_impl(dst_vg_handle, begin_vg_handle, end_vg_handle, t);
      break;
   }
   case VGPATHLENGTH_ID: /* RPC_CALL3_RES */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGint segments_i = khdispatch_read_int();
      VGint segments_count = khdispatch_read_int();
      khdispatch_write_float(vgPathLength_impl(vg_handle, segments_i, segments_count));
      break;
   }
   case VGPOINTALONGPATH_ID: /* RPC_CALL6_OUT_CTRL_RES */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGint segments_i = khdispatch_read_int();
      VGint segments_count = khdispatch_read_int();
      VGfloat distance = khdispatch_read_float();
      VGbitfield mask = khdispatch_read_bitfield();
      VGfloat values[4];
      bool success = vgPointAlongPath_impl(vg_handle, segments_i, segments_count, distance, mask, values);
      khdispatch_write_out_ctrl_res(values, success ? sizeof(values) : 0, success);
      break;
   }
   case VGPATHBOUNDS_ID: /* RPC_CALL2_OUT_CTRL_RES */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGfloat values[4];
      bool success = vgPathBounds_impl(vg_handle, values);
      khdispatch_write_out_ctrl_res(values, success ? sizeof(values) : 0, success);
      break;
   }
   case VGPATHTRANSFORMEDBOUNDS_ID: /* RPC_CALL2_OUT_CTRL_RES */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGfloat values[4];
      bool success = vgPathTransformedBounds_impl(vg_handle, values);
      khdispatch_write_out_ctrl_res(values, success ? sizeof(values) : 0, success);
      break;
   }
   case VGDRAWPATH_ID: /* RPC_CALL2 */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGbitfield paint_modes = khdispatch_read_bitfield();
      vgDrawPath_impl(vg_handle, paint_modes);
      break;
   }
   case VGCREATEPAINT_ID: /* RPC_CALL1 */
   {
      VGHandle vg_handle = khdispatch_read_handle();
      vgCreatePaint_impl(vg_handle);
      break;
   }
   case VGDESTROYPAINT_ID: /* RPC_CALL1 */
   {
      VGPaint vg_handle = khdispatch_read_handle();
      vgDestroyPaint_impl(vg_handle);
      break;
   }
   case VGSETPAINT_ID: /* RPC_CALL2 */
   {
      VGPaint vg_handle = khdispatch_read_handle();
      VGbitfield paint_modes = khdispatch_read_bitfield();
      vgSetPaint_impl(vg_handle, paint_modes);
      break;
   }
   case VGPAINTPATTERN_ID: /* RPC_CALL2 */
   {
      VGPaint vg_handle = khdispatch_read_handle();
      VGImage pattern_vg_handle = khdispatch_read_handle();
      vgPaintPattern_impl(vg_handle, pattern_vg_handle);
      break;
   }
   case VGCREATEIMAGE_ID: /* RPC_CALL5 */
   {
      VGHandle vg_handle = khdispatch_read_handle();
      VGImageFormat format = (VGImageFormat)khdispatch_read_enum();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      VGbitfield allowed_quality = khdispatch_read_bitfield();
      vgCreateImage_impl(vg_handle, format, width, height, allowed_quality);
      break;
   }
   case VGDESTROYIMAGE_ID: /* RPC_CALL1 */
   {
      VGImage vg_handle = khdispatch_read_handle();
      vgDestroyImage_impl(vg_handle);
      break;
   }
   case VGCLEARIMAGE_ID: /* RPC_CALL5 */
   {
      VGImage vg_handle = khdispatch_read_handle();
      VGint x = khdispatch_read_int();
      VGint y = khdispatch_read_int();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      vgClearImage_impl(vg_handle, x, y, width, height);
      break;
   }
   case VGIMAGESUBDATA_ID: /* custom */
   {
      VGImage vg_handle = khdispatch_read_handle();
      VGint dst_width = khdispatch_read_int();
      VGint dst_height = khdispatch_read_int();
      VGuint line_size = khdispatch_read_uint();
      VGImageFormat data_format = (VGImageFormat)khdispatch_read_enum();
      VGint src_x = khdispatch_read_int();
      VGint dst_x = khdispatch_read_int();
      VGint dst_y = khdispatch_read_int();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      khdispatch_check_workspace(khdispatch_pad_bulk(height * line_size));
      void *data = khdispatch_workspace;
      if(khdispatch_recv_bulk(data, height * line_size))
         vgImageSubData_impl(vg_handle, dst_width, dst_height, data, line_size, data_format, src_x, dst_x, dst_y, width, height);
      break;
   }
   case VGGETIMAGESUBDATA_ID: /* custom */
   {
      VGImage vg_handle = khdispatch_read_handle();
      VGint src_width = khdispatch_read_int();
      VGint src_height = khdispatch_read_int();
      VGuint line_size = khdispatch_read_uint();
      VGImageFormat data_format = (VGImageFormat)khdispatch_read_enum();
      VGint dst_x = khdispatch_read_int();
      VGint src_x = khdispatch_read_int();
      VGint src_y = khdispatch_read_int();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      khdispatch_check_workspace(khdispatch_pad_bulk(height * line_size));
      void *data = khdispatch_workspace;
      bool success = vgGetImageSubData_impl(vg_handle, src_width, src_height, data, line_size, data_format, dst_x, src_x, src_y, width, height);
#ifdef UIDECK
      if (success) {
         khdispatch_write_out_bulk_res(data, height * line_size, line_size);
      } else {
         khdispatch_write_out_bulk_res(data, 0, 0);
      }

#else
      khdispatch_write_uint(success ? line_size : 0);
      if (success) {
         khdispatch_send_bulk(data, height * line_size);
      }
#endif
      break;
   }
   case VGCHILDIMAGE_ID: /* RPC_CALL8 */
   {
      VGHandle vg_handle = khdispatch_read_handle();
      VGImage parent_vg_handle = khdispatch_read_handle();
      VGint parent_width = khdispatch_read_int();
      VGint parent_height = khdispatch_read_int();
      VGint x = khdispatch_read_int();
      VGint y = khdispatch_read_int();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      vgChildImage_impl(vg_handle, parent_vg_handle, parent_width, parent_height, x, y, width, height);
      break;
   }
   case VGGETPARENT_ID: /* RPC_CALL1_RES */
   {
      VGImage vg_handle = khdispatch_read_handle();
      khdispatch_write_handle(vgGetParent_impl(vg_handle));
      break;
   }
   case VGCOPYIMAGE_ID: /* RPC_CALL9 */
   {
      VGImage dst_vg_handle = khdispatch_read_handle();
      VGint dst_x = khdispatch_read_int();
      VGint dst_y = khdispatch_read_int();
      VGImage src_vg_handle = khdispatch_read_handle();
      VGint src_x = khdispatch_read_int();
      VGint src_y = khdispatch_read_int();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      bool dither = khdispatch_read_boolean();
      vgCopyImage_impl(dst_vg_handle, dst_x, dst_y, src_vg_handle, src_x, src_y, width, height, dither);
      break;
   }
   case VGDRAWIMAGE_ID: /* RPC_CALL1 */
   {
      VGImage vg_handle = khdispatch_read_handle();
      vgDrawImage_impl(vg_handle);
      break;
   }
   case VGSETPIXELS_ID: /* RPC_CALL7 */
   {
      VGint dst_x = khdispatch_read_int();
      VGint dst_y = khdispatch_read_int();
      VGImage src_vg_handle = khdispatch_read_handle();
      VGint src_x = khdispatch_read_int();
      VGint src_y = khdispatch_read_int();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      vgSetPixels_impl(dst_x, dst_y, src_vg_handle, src_x, src_y, width, height);
      break;
   }
   case VGWRITEPIXELS_ID: /* custom */
   {
      VGuint line_size = khdispatch_read_uint();
      VGImageFormat data_format = (VGImageFormat)khdispatch_read_enum();
      VGint src_x = khdispatch_read_int();
      VGint dst_x = khdispatch_read_int();
      VGint dst_y = khdispatch_read_int();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      khdispatch_check_workspace(khdispatch_pad_bulk(height * line_size));
      void *data = khdispatch_workspace;
      if(khdispatch_recv_bulk(data, height * line_size))
         vgWritePixels_impl(data, line_size, data_format, src_x, dst_x, dst_y, width, height);
      break;
   }
   case VGGETPIXELS_ID: /* RPC_CALL7 */
   {
      VGImage dst_vg_handle = khdispatch_read_handle();
      VGint dst_x = khdispatch_read_int();
      VGint dst_y = khdispatch_read_int();
      VGint src_x = khdispatch_read_int();
      VGint src_y = khdispatch_read_int();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      vgGetPixels_impl(dst_vg_handle, dst_x, dst_y, src_x, src_y, width, height);
      break;
   }
   case VGREADPIXELS_ID: /* custom */
   {
      VGuint line_size = khdispatch_read_uint();
      VGImageFormat data_format = (VGImageFormat)khdispatch_read_enum();
      VGint dst_x = khdispatch_read_int();
      VGint src_x = khdispatch_read_int();
      VGint src_y = khdispatch_read_int();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      khdispatch_check_workspace(khdispatch_pad_bulk(height * line_size));
      void *data = khdispatch_workspace;
      vgReadPixels_impl(data, line_size, data_format, dst_x, src_x, src_y, width, height);
      khdispatch_send_bulk(data, height * line_size);
      break;
   }
   case VGCOPYPIXELS_ID: /* RPC_CALL6 */
   {
      VGint dst_x = khdispatch_read_int();
      VGint dst_y = khdispatch_read_int();
      VGint src_x = khdispatch_read_int();
      VGint src_y = khdispatch_read_int();
      VGint width = khdispatch_read_int();
      VGint height = khdispatch_read_int();
      vgCopyPixels_impl(dst_x, dst_y, src_x, src_y, width, height);
      break;
   }
   case VGCREATEFONT_ID: /* RPC_CALL2 (vg 1.1) */
   {
      VGHandle vg_handle = khdispatch_read_handle();
      VGint glyphs_capacity = khdispatch_read_int();
      vgCreateFont_impl(vg_handle, glyphs_capacity);
      break;
   }
   case VGDESTROYFONT_ID: /* RPC_CALL1 (vg 1.1) */
   {
      VGFont vg_handle = khdispatch_read_handle();
      vgDestroyFont_impl(vg_handle);
      break;
   }
   case VGSETGLYPHTOPATH_ID: /* RPC_CALL8 (vg 1.1) */
   {
      VGFont vg_handle = khdispatch_read_handle();
      VGuint glyph_id = khdispatch_read_uint();
      VGPath path_vg_handle = khdispatch_read_handle();
      bool is_hinted = khdispatch_read_boolean();
      VGfloat glyph_origin_0 = khdispatch_read_float();
      VGfloat glyph_origin_1 = khdispatch_read_float();
      VGfloat escapement_0 = khdispatch_read_float();
      VGfloat escapement_1 = khdispatch_read_float();
      vgSetGlyphToPath_impl(vg_handle, glyph_id, path_vg_handle, is_hinted, glyph_origin_0, glyph_origin_1, escapement_0, escapement_1);
      break;
   }
   case VGSETGLYPHTOIMAGE_ID: /* RPC_CALL7 (vg 1.1) */
   {
      VGFont vg_handle = khdispatch_read_handle();
      VGuint glyph_id = khdispatch_read_uint();
      VGImage image_vg_handle = khdispatch_read_handle();
      VGfloat glyph_origin_0 = khdispatch_read_float();
      VGfloat glyph_origin_1 = khdispatch_read_float();
      VGfloat escapement_0 = khdispatch_read_float();
      VGfloat escapement_1 = khdispatch_read_float();
      vgSetGlyphToImage_impl(vg_handle, glyph_id, image_vg_handle, glyph_origin_0, glyph_origin_1, escapement_0, escapement_1);
      break;
   }
   case VGCLEARGLYPH_ID: /* RPC_CALL2 (vg 1.1) */
   {
      VGFont vg_handle = khdispatch_read_handle();
      VGuint glyph_id = khdispatch_read_uint();
      vgClearGlyph_impl(vg_handle, glyph_id);
      break;
   }
   case VGDRAWGLYPH_ID: /* RPC_CALL4 (vg 1.1) */
   {
      VGFont vg_handle = khdispatch_read_handle();
      VGuint glyph_id = khdispatch_read_uint();
      VGbitfield paint_modes = khdispatch_read_bitfield();
      bool allow_autohinting = khdispatch_read_boolean();
      vgDrawGlyph_impl(vg_handle, glyph_id, paint_modes, allow_autohinting);
      break;
   }
   case VGDRAWGLYPHS_ID: /* custom (vg 1.1) */
   {
      VGFont vg_handle = khdispatch_read_handle();
      VGint glyphs_count = khdispatch_read_int();
      VGbitfield adjustments = khdispatch_read_bitfield();
      VGbitfield paint_modes = khdispatch_read_bitfield();
      bool allow_autohinting = khdispatch_read_boolean();
      const VGuint *glyph_ids = (const VGuint *)khdispatch_read_ctrl(glyphs_count * sizeof(VGuint));
      const VGfloat *adjustments_x = (adjustments & 0x1) ? (const VGfloat *)khdispatch_read_ctrl(glyphs_count * sizeof(VGfloat)) : NULL;
      const VGfloat *adjustments_y = (adjustments & 0x2) ? (const VGfloat *)khdispatch_read_ctrl(glyphs_count * sizeof(VGfloat)) : NULL;
      vgDrawGlyphs_impl(vg_handle, glyphs_count, glyph_ids, adjustments_x, adjustments_y, paint_modes, allow_autohinting);
      break;
   }
   case VGCOLORMATRIX_ID: /* RPC_CALL3_IN_CTRL */
   {
      VGImage dst_vg_handle = khdispatch_read_handle();
      VGImage src_vg_handle = khdispatch_read_handle();
      const VGfloat *matrix = (const VGfloat *)khdispatch_read_ctrl(20 * sizeof(VGfloat));
      vgColorMatrix_impl(dst_vg_handle, src_vg_handle, matrix);
      break;
   }
   case VGCONVOLVE_ID: /* RPC_CALL10_IN_CTRL */
   {
      VGImage dst_vg_handle = khdispatch_read_handle();
      VGImage src_vg_handle = khdispatch_read_handle();
      VGint kernel_width = khdispatch_read_int();
      VGint kernel_height = khdispatch_read_int();
      VGint shift_x = khdispatch_read_int();
      VGint shift_y = khdispatch_read_int();
      VGfloat scale = khdispatch_read_float();
      VGfloat bias = khdispatch_read_float();
      VGTilingMode tiling_mode = (VGTilingMode)khdispatch_read_enum();
      const VGshort *kernel = (const VGshort *)khdispatch_read_ctrl(kernel_width * kernel_height * sizeof(VGshort));
      vgConvolve_impl(dst_vg_handle, src_vg_handle, kernel_width, kernel_height, shift_x, shift_y, scale, bias, tiling_mode, kernel);
      break;
   }
   case VGSEPARABLECONVOLVE_ID: /* custom */
   {
      VGImage dst_vg_handle = khdispatch_read_handle();
      VGImage src_vg_handle = khdispatch_read_handle();
      VGint kernel_width = khdispatch_read_int();
      VGint kernel_height = khdispatch_read_int();
      VGint shift_x = khdispatch_read_int();
      VGint shift_y = khdispatch_read_int();
      VGfloat scale = khdispatch_read_float();
      VGfloat bias = khdispatch_read_float();
      VGTilingMode tiling_mode = (VGTilingMode)khdispatch_read_enum();
      const VGshort *kernel_x = (const VGshort *)khdispatch_read_ctrl(kernel_width * sizeof(VGshort));
      const VGshort *kernel_y = (const VGshort *)khdispatch_read_ctrl(kernel_height * sizeof(VGshort));
      vgSeparableConvolve_impl(dst_vg_handle, src_vg_handle, kernel_width, kernel_height, shift_x, shift_y, kernel_x, kernel_y, scale, bias, tiling_mode);
      break;
   }
   case VGGAUSSIANBLUR_ID: /* RPC_CALL5 */
   {
      VGImage dst_vg_handle = khdispatch_read_handle();
      VGImage src_vg_handle = khdispatch_read_handle();
      VGfloat std_dev_x = khdispatch_read_float();
      VGfloat std_dev_y = khdispatch_read_float();
      VGTilingMode tiling_mode = (VGTilingMode)khdispatch_read_enum();
      vgGaussianBlur_impl(dst_vg_handle, src_vg_handle, std_dev_x, std_dev_y, tiling_mode);
      break;
   }
   case VGLOOKUP_ID: /* custom */
   {
      VGImage dst_vg_handle = khdispatch_read_handle();
      VGImage src_vg_handle = khdispatch_read_handle();
      bool output_linear = khdispatch_read_boolean();
      bool output_pre = khdispatch_read_boolean();
      const VGubyte *red_lut = (const VGubyte *)khdispatch_read_ctrl(256);
      const VGubyte *green_lut = (const VGubyte *)khdispatch_read_ctrl(256);
      const VGubyte *blue_lut = (const VGubyte *)khdispatch_read_ctrl(256);
      const VGubyte *alpha_lut = (const VGubyte *)khdispatch_read_ctrl(256);
      vgLookup_impl(dst_vg_handle, src_vg_handle, red_lut, green_lut, blue_lut, alpha_lut, output_linear, output_pre);
      break;
   }
   case VGLOOKUPSINGLE_ID: /* RPC_CALL6_IN_CTRL */
   {
      VGImage dst_vg_handle = khdispatch_read_handle();
      VGImage src_vg_handle = khdispatch_read_handle();
      VGImageChannel source_channel = (VGImageChannel)khdispatch_read_enum();
      bool output_linear = khdispatch_read_boolean();
      bool output_pre = khdispatch_read_boolean();
      const VGuint *lut = (const VGuint *)khdispatch_read_ctrl(256 * sizeof(VGuint));
      vgLookupSingle_impl(dst_vg_handle, src_vg_handle, source_channel, output_linear, output_pre, lut);
      break;
   }
   case VGULINE_ID: /* RPC_CALL5 (vgu) */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGfloat p0_x = khdispatch_read_float();
      VGfloat p0_y = khdispatch_read_float();
      VGfloat p1_x = khdispatch_read_float();
      VGfloat p1_y = khdispatch_read_float();
      vguLine_impl(vg_handle, p0_x, p0_y, p1_x, p1_y);
      break;
   }
   case VGUPOLYGON_ID: /* custom (vgu) */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGint ps_count = khdispatch_read_int();
      bool first = khdispatch_read_boolean();
      bool close = khdispatch_read_boolean();
      const VGfloat *ps = (const VGfloat *)khdispatch_read_ctrl(ps_count * 2 * sizeof(VGfloat));
      vguPolygon_impl(vg_handle, ps, ps_count, first, close);
      break;
   }
   case VGURECT_ID: /* RPC_CALL5 (vgu) */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGfloat x = khdispatch_read_float();
      VGfloat y = khdispatch_read_float();
      VGfloat width = khdispatch_read_float();
      VGfloat height = khdispatch_read_float();
      vguRect_impl(vg_handle, x, y, width, height);
      break;
   }
   case VGUROUNDRECT_ID: /* RPC_CALL7 (vgu) */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGfloat x = khdispatch_read_float();
      VGfloat y = khdispatch_read_float();
      VGfloat width = khdispatch_read_float();
      VGfloat height = khdispatch_read_float();
      VGfloat arc_width = khdispatch_read_float();
      VGfloat arc_height = khdispatch_read_float();
      vguRoundRect_impl(vg_handle, x, y, width, height, arc_width, arc_height);
      break;
   }
   case VGUELLIPSE_ID: /* RPC_CALL5 (vgu) */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGfloat x = khdispatch_read_float();
      VGfloat y = khdispatch_read_float();
      VGfloat width = khdispatch_read_float();
      VGfloat height = khdispatch_read_float();
      vguEllipse_impl(vg_handle, x, y, width, height);
      break;
   }
   case VGUARC_ID: /* RPC_CALL9 (vgu) */
   {
      VGPath vg_handle = khdispatch_read_handle();
      VGfloat x = khdispatch_read_float();
      VGfloat y = khdispatch_read_float();
      VGfloat width = khdispatch_read_float();
      VGfloat height = khdispatch_read_float();
      VGfloat start_angle = khdispatch_read_float();
      VGfloat angle_extent = khdispatch_read_float();
      VGuint angle_o180 = khdispatch_read_uint();
      VGUArcType arc_type = (VGUArcType)khdispatch_read_enum();
      vguArc_impl(vg_handle, x, y, width, height, start_angle, angle_extent, angle_o180, arc_type);
      break;
   }
#if VG_KHR_EGL_image
   case VGCREATEEGLIMAGETARGETKHR_ID: /* RPC_CALL2_OUT_CTRL_RES (VG_KHR_EGL_image) */
   {
      VGeglImageKHR src_egl_handle = khdispatch_read_eglhandle();
      VGuint format_width_height[3];
      VGHandle vg_handle = vgCreateEGLImageTargetKHR_impl(src_egl_handle, format_width_height);
      khdispatch_write_out_ctrl_res(format_width_height, (vg_handle == VG_INVALID_HANDLE) ? 0 : sizeof(format_width_height), (uint32_t)vg_handle);
      break;
   }
#if EGL_BRCM_global_image
   case VGCREATEIMAGEFROMGLOBALIMAGE_ID: /* RPC_CALL3 (VG_KHR_EGL_image/EGL_BRCM_global_image) */
   {
      VGHandle vg_handle = khdispatch_read_handle();
      VGuint id_0 = khdispatch_read_uint();
      VGuint id_1 = khdispatch_read_uint();
      vgCreateImageFromGlobalImage_impl(vg_handle, id_0, id_1);
      break;
   }
#endif
#endif
   default:
   {
      UNREACHABLE();
   }
   }
}
