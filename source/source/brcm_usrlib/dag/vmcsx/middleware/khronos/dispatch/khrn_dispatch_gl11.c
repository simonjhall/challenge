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
#include "middleware/khronos/common/khrn_hw.h"

#include "middleware/khronos/gl11/gl11_server.h"

#include "interface/khronos/common/khrn_int_ids.h"

#include "middleware/khronos/dispatch/khrn_dispatch_rpc.h"

bool gl_dispatch_11(uint32_t id)
{
   bool result = true;
   switch (id) {
   //gl 1.1 specific
   case GLCLEARCOLORX_ID_11:
   {
      GLfixed red = khdispatch_read_fixed();
      GLfixed green = khdispatch_read_fixed();
      GLfixed blue = khdispatch_read_fixed();
      GLfixed alpha = khdispatch_read_fixed();

      glClearColorx_impl_11(red, green, blue, alpha);
      break;
   }   
   case GLROTATEF_ID_11:
   {
      GLfloat angle = khdispatch_read_float();
      GLfloat x = khdispatch_read_float();
      GLfloat y = khdispatch_read_float();
      GLfloat z = khdispatch_read_float();

      glRotatef_impl_11(angle, x, y, z);
      break;
   }
   case GLSCALEF_ID_11:
   {
      GLfloat x = khdispatch_read_float();
      GLfloat y = khdispatch_read_float();
      GLfloat z = khdispatch_read_float();

      glScalef_impl_11(x, y, z);
      break;
   }
   case GLTEXENVF_ID_11:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();
      GLfloat param = khdispatch_read_float();

      glTexEnvf_impl_11(target, pname, param);
      break;
   }
   case GLTRANSLATEF_ID_11:
   {
      GLfloat x = khdispatch_read_float();
      GLfloat y = khdispatch_read_float();
      GLfloat z = khdispatch_read_float();

      glTranslatef_impl_11(x, y, z);
      break;
   }
   case GLALPHAFUNCX_ID_11:
   {
      GLenum func = khdispatch_read_enum();
      GLfixed ref = khdispatch_read_fixed();

      glAlphaFuncx_impl_11(func, ref);
      break;
   }
   case GLCLEARDEPTHX_ID_11:
   {
      GLfixed depth = khdispatch_read_fixed();

      glClearDepthx_impl_11(depth);
      break;
   }
   case GLDEPTHRANGEX_ID_11:
   {
      GLfixed zNear = khdispatch_read_fixed();
      GLfixed zFar = khdispatch_read_fixed();

      glDepthRangex_impl_11(zNear, zFar);
      break;
   }
   case GLFOGX_ID_11:
   {
      GLenum pname = khdispatch_read_enum();
      GLfixed param = khdispatch_read_fixed();

      glFogx_impl_11(pname, param);
      break;
   }
   case GLFRUSTUMX_ID_11:
   {
      GLfixed left = khdispatch_read_fixed();
      GLfixed right = khdispatch_read_fixed();
      GLfixed bottom = khdispatch_read_fixed();
      GLfixed top = khdispatch_read_fixed();
      GLfixed near = khdispatch_read_fixed();
      GLfixed far = khdispatch_read_fixed();

      glFrustumx_impl_11(left, right, bottom, top, near, far);
      break;
   }
   case GLLIGHTMODELX_ID_11:
   {
      GLenum pname = khdispatch_read_enum();
      GLfixed param = khdispatch_read_fixed();

      glLightModelx_impl_11(pname, param);
      break;
   }
   case GLLIGHTX_ID_11:
   {
      GLenum light = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();
      GLfixed param = khdispatch_read_fixed();

      glLightx_impl_11(light, pname, param);
      break;
   }
   case GLLINEWIDTHX_ID_11:
   {
      GLfixed width = khdispatch_read_fixed();

      glLineWidthx_impl_11(width);
      break;
   }
   case GLLOADIDENTITY_ID_11:
   {
      glLoadIdentity_impl_11();
      break;
   }
   case GLLOGICOP_ID_11:
   {
      GLenum opcode = khdispatch_read_enum();

      glLogicOp_impl_11(opcode);
      break;
   }
   case GLMATERIALX_ID_11:
   {
      GLenum face = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();
      GLfixed param = khdispatch_read_fixed();

      glMaterialx_impl_11(face, pname, param);
      break;
   }
   case GLMATRIXMODE_ID_11:
   {
      GLenum mode = khdispatch_read_enum();

      glMatrixMode_impl_11(mode);
      break;
   }
   case GLORTHOX_ID_11:
   {
      GLfixed left = khdispatch_read_fixed();
      GLfixed right = khdispatch_read_fixed();
      GLfixed bottom = khdispatch_read_fixed();
      GLfixed top = khdispatch_read_fixed();
      GLfixed near = khdispatch_read_fixed();
      GLfixed far = khdispatch_read_fixed();

      glOrthox_impl_11(left, right, bottom, top, near, far);
      break;
   }
   case GLPOINTPARAMETERX_ID_11:
   {
      GLenum pname = khdispatch_read_enum();
      GLfixed param = khdispatch_read_fixed();

      glPointParameterx_impl_11(pname, param);
      break;
   }
   case GLROTATEX_ID_11:
   {
      GLfixed angle = khdispatch_read_fixed();
      GLfixed x = khdispatch_read_fixed();
      GLfixed y = khdispatch_read_fixed();
      GLfixed z = khdispatch_read_fixed();

      glRotatex_impl_11(angle, x, y, z);
      break;
   }
   case GLPOLYGONOFFSETX_ID_11:
   {
      GLfixed factor = khdispatch_read_fixed();
      GLfixed units = khdispatch_read_fixed();

      glPolygonOffsetx_impl_11(factor, units);
      break;
   }
   case GLPOPMATRIX_ID_11:
   {
      glPopMatrix_impl_11();
      break;
   }
   case GLPUSHMATRIX_ID_11:
   {
      glPushMatrix_impl_11();
      break;
   }
   case GLSAMPLECOVERAGEX_ID_11:
   {
      GLfixed value = khdispatch_read_fixed();
      GLboolean invert = khdispatch_read_boolean();

      glSampleCoveragex_impl_11(value, invert);
      break;
   }
   case GLSCALEX_ID_11:
   {
      GLfixed x = khdispatch_read_fixed();
      GLfixed y = khdispatch_read_fixed();
      GLfixed z = khdispatch_read_fixed();

      glScalex_impl_11(x, y, z);
      break;
   }
   case GLSHADEMODEL_ID_11:
   {
      GLenum mode = khdispatch_read_enum();

      glShadeModel_impl_11(mode);
      break;
   }
   case GLTEXENVI_ID_11:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();
      GLint param = khdispatch_read_int();

      glTexEnvi_impl_11(target, pname, param);
      break;
   }
   case GLTEXENVX_ID_11:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();
      GLfixed param = khdispatch_read_fixed();

      glTexEnvx_impl_11(target, pname, param);
      break;
   }
   case GLTEXPARAMETERX_ID_11:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();
      GLfixed param = khdispatch_read_fixed();

      glTexParameterx_impl_11(target, pname, param);
      break;
   }
   case GLTEXPARAMETERXV_ID_11:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();
      GLfixed param[4];
      param[0] = khdispatch_read_fixed();
      param[1] = khdispatch_read_fixed();
      param[2] = khdispatch_read_fixed();
      param[3] = khdispatch_read_fixed();

      glTexParameterxv_impl_11(target, pname, param);
      break;
   }
   case GLTRANSLATEX_ID_11:
   {
      GLfixed x = khdispatch_read_fixed();
      GLfixed y = khdispatch_read_fixed();
      GLfixed z = khdispatch_read_fixed();

      glTranslatex_impl_11(x, y, z);
      break;
   }
   case GLALPHAFUNC_ID_11:
   {
      GLenum func = khdispatch_read_enum();
      GLfloat ref = khdispatch_read_float();

      glAlphaFunc_impl_11(func, ref);
      break;
   }
   case GLFOGF_ID_11:
   {
      GLenum pname = khdispatch_read_enum();
      GLfloat param = khdispatch_read_float();

      glFogf_impl_11(pname, param);
      break;
   }
   case GLFRUSTUMF_ID_11:
   {
      GLfloat left = khdispatch_read_float();
      GLfloat right = khdispatch_read_float();
      GLfloat bottom = khdispatch_read_float();
      GLfloat top = khdispatch_read_float();
      GLfloat near = khdispatch_read_float();
      GLfloat far = khdispatch_read_float();

      glFrustumf_impl_11(left, right, bottom, top, near, far);
      break;
   }
   case GLLIGHTMODELF_ID_11:
   {
      GLenum pname = khdispatch_read_enum();
      GLfloat param = khdispatch_read_float();

      glLightModelf_impl_11(pname, param);
      break;
   }
   case GLLIGHTF_ID_11:
   {
      GLenum light = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();
      GLfloat param = khdispatch_read_float();

      glLightf_impl_11(light, pname, param);
      break;
   }
   case GLMATERIALF_ID_11:
   {
      GLenum face = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();
      GLfloat param = khdispatch_read_float();

      glMaterialf_impl_11(face, pname, param);
      break;
   }
   case GLORTHOF_ID_11:
   {
      GLfloat left = khdispatch_read_float();
      GLfloat right = khdispatch_read_float();
      GLfloat bottom = khdispatch_read_float();
      GLfloat top = khdispatch_read_float();
      GLfloat near = khdispatch_read_float();
      GLfloat far = khdispatch_read_float();

      glOrthof_impl_11(left, right, bottom, top, near, far);
      break;
   }
   case GLPOINTPARAMETERF_ID_11:
   {
      GLenum pname = khdispatch_read_enum();
      GLfloat param = khdispatch_read_float();

      glPointParameterf_impl_11(pname, param);
      break;
   }
   case GLFOGFV_ID_11:
   {
      GLenum pname = khdispatch_read_enum();

      GLfloat p0 = khdispatch_read_float();
      GLfloat p1 = khdispatch_read_float();
      GLfloat p2 = khdispatch_read_float();
      GLfloat p3 = khdispatch_read_float();

      GLfloat params[4] = {p0, p1, p2, p3};

      glFogfv_impl_11(pname, params);
      break;
   }
   case GLFOGXV_ID_11:
   {
      GLenum pname = khdispatch_read_enum();

      GLfixed p0 = khdispatch_read_fixed();
      GLfixed p1 = khdispatch_read_fixed();
      GLfixed p2 = khdispatch_read_fixed();
      GLfixed p3 = khdispatch_read_fixed();

      GLfixed params[4] = {p0, p1, p2, p3};

      glFogxv_impl_11(pname, params);
      break;
   }
   case GLCLIPPLANEF_ID_11:
   {
      GLenum plane = khdispatch_read_enum();

      GLfloat e0 = khdispatch_read_float();
      GLfloat e1 = khdispatch_read_float();
      GLfloat e2 = khdispatch_read_float();
      GLfloat e3 = khdispatch_read_float();

      GLfloat equation[4] = {e0, e1, e2, e3};

      glClipPlanef_impl_11(plane, equation);
      break;
   }
   case GLCLIPPLANEX_ID_11:
   {
      GLenum plane = khdispatch_read_enum();

      GLfixed e0 = khdispatch_read_fixed();
      GLfixed e1 = khdispatch_read_fixed();
      GLfixed e2 = khdispatch_read_fixed();
      GLfixed e3 = khdispatch_read_fixed();

      GLfixed equation[4] = {e0, e1, e2, e3};

      glClipPlanex_impl_11(plane, equation);
      break;
   }
   case GLLIGHTMODELFV_ID_11:
   {
      GLenum pname = khdispatch_read_enum();

      GLfloat p0 = khdispatch_read_float();
      GLfloat p1 = khdispatch_read_float();
      GLfloat p2 = khdispatch_read_float();
      GLfloat p3 = khdispatch_read_float();

      GLfloat params[4] = {p0, p1, p2, p3};

      glLightModelfv_impl_11(pname, params);
      break;
   }
   case GLLIGHTMODELXV_ID_11:
   {
      GLenum pname = khdispatch_read_enum();

      GLfixed p0 = khdispatch_read_fixed();
      GLfixed p1 = khdispatch_read_fixed();
      GLfixed p2 = khdispatch_read_fixed();
      GLfixed p3 = khdispatch_read_fixed();

      GLfixed params[4] = {p0, p1, p2, p3};

      glLightModelxv_impl_11(pname, params);
      break;
   }
   case GLLIGHTFV_ID_11:
   {
      GLenum light = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLfloat p0 = khdispatch_read_float();
      GLfloat p1 = khdispatch_read_float();
      GLfloat p2 = khdispatch_read_float();
      GLfloat p3 = khdispatch_read_float();

      GLfloat params[4] = {p0, p1, p2, p3};

      glLightfv_impl_11(light, pname, params);
      break;
   }
   case GLLIGHTXV_ID_11:
   {
      GLenum light = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLfixed p0 = khdispatch_read_fixed();
      GLfixed p1 = khdispatch_read_fixed();
      GLfixed p2 = khdispatch_read_fixed();
      GLfixed p3 = khdispatch_read_fixed();

      GLfixed params[4] = {p0, p1, p2, p3};

      glLightxv_impl_11(light, pname, params);
      break;
   }
   case GLLOADMATRIXF_ID_11:
   {
      GLfloat matrix[16];

      for (int i = 0; i < 16; i++)
         matrix[i] = khdispatch_read_float();

      glLoadMatrixf_impl_11(matrix);
      break;
   }
   case GLMULTMATRIXF_ID_11:
   {
      GLfloat matrix[16];

      for (int i = 0; i < 16; i++)
         matrix[i] = khdispatch_read_float();

      glMultMatrixf_impl_11(matrix);
      break;
   }
   case GLLOADMATRIXX_ID_11:
   {
      GLfixed matrix[16];

      for (int i = 0; i < 16; i++)
         matrix[i] = khdispatch_read_fixed();

      glLoadMatrixx_impl_11(matrix);
      break;
   }
   case GLMULTMATRIXX_ID_11:
   {
      GLfixed matrix[16];

      for (int i = 0; i < 16; i++)
         matrix[i] = khdispatch_read_fixed();

      glMultMatrixx_impl_11(matrix);
      break;
   }
   case GLMATERIALFV_ID_11:
   {
      GLenum face = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLfloat p0 = khdispatch_read_float();
      GLfloat p1 = khdispatch_read_float();
      GLfloat p2 = khdispatch_read_float();
      GLfloat p3 = khdispatch_read_float();

      GLfloat params[4] = {p0, p1, p2, p3};

      glMaterialfv_impl_11(face, pname, params);
      break;
   }
   case GLMATERIALXV_ID_11:
   {
      GLenum face = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLfixed p0 = khdispatch_read_fixed();
      GLfixed p1 = khdispatch_read_fixed();
      GLfixed p2 = khdispatch_read_fixed();
      GLfixed p3 = khdispatch_read_fixed();

      GLfixed params[4] = {p0, p1, p2, p3};

      glMaterialxv_impl_11(face, pname, params);
      break;
   }
   case GLPOINTPARAMETERFV_ID_11:
   {
      GLenum pname = khdispatch_read_enum();

      GLfloat p0 = khdispatch_read_float();
      GLfloat p1 = khdispatch_read_float();
      GLfloat p2 = khdispatch_read_float();

      GLfloat params[3] = {p0, p1, p2};

      glPointParameterfv_impl_11(pname, params);
      break;
   }
   case GLPOINTPARAMETERXV_ID_11:
   {
      GLenum pname = khdispatch_read_enum();

      GLfixed p0 = khdispatch_read_fixed();
      GLfixed p1 = khdispatch_read_fixed();
      GLfixed p2 = khdispatch_read_fixed();

      GLfixed params[3] = {p0, p1, p2};

      glPointParameterxv_impl_11(pname, params);
      break;
   }
   case GLTEXENVFV_ID_11:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLfloat p0 = khdispatch_read_float();
      GLfloat p1 = khdispatch_read_float();
      GLfloat p2 = khdispatch_read_float();
      GLfloat p3 = khdispatch_read_float();

      GLfloat params[4] = {p0, p1, p2, p3};

      glTexEnvfv_impl_11(target, pname, params);
      break;
   }
   case GLTEXENVIV_ID_11:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLint p0 = khdispatch_read_int();
      GLint p1 = khdispatch_read_int();
      GLint p2 = khdispatch_read_int();
      GLint p3 = khdispatch_read_int();

      GLint params[4] = {p0, p1, p2, p3};

      glTexEnviv_impl_11(target, pname, params);
      break;
   }
   case GLTEXENVXV_ID_11:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLfixed p0 = khdispatch_read_fixed();
      GLfixed p1 = khdispatch_read_fixed();
      GLfixed p2 = khdispatch_read_fixed();
      GLfixed p3 = khdispatch_read_fixed();

      GLfixed params[4] = {p0, p1, p2, p3};

      glTexEnvxv_impl_11(target, pname, params);
      break;
   }
   case GLGETMATERIALFV_ID_11:
   {
      GLenum face = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLfloat params[4];

      int count = glGetMaterialfv_impl_11(face, pname, params);

      vcos_assert(count <= 4);

      khdispatch_write_out_ctrl(params, count * sizeof(GLfloat));
      break;
   }
   case GLGETCLIPPLANEF_ID_11:
   {
      GLenum pname = khdispatch_read_enum();

      GLfloat params[4];

      glGetClipPlanef_impl_11(pname, params);

      khdispatch_write_out_ctrl(params, 4 * sizeof(GLfloat));
      break;
   }
   case GLGETLIGHTFV_ID_11:
   {
      GLenum light = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLfloat params[4];

      int count = glGetLightfv_impl_11(light, pname, params);

      vcos_assert(count <= 4);

      khdispatch_write_out_ctrl(params, count * sizeof(GLfloat));
      break;
   }
   case GLGETTEXENVFV_ID_11:
   {
      GLenum env = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLfloat params[4];

      int count = glGetTexEnvfv_impl_11(env, pname, params);

      vcos_assert(count <= 4);

      khdispatch_write_out_ctrl(params, count * sizeof(GLfloat));
      break;
   }
   case GLGETCLIPPLANEX_ID_11:
   {
      GLenum pname = khdispatch_read_enum();

      GLfixed params[4];

      glGetClipPlanex_impl_11(pname, params);

      khdispatch_write_out_ctrl(params, 4 * sizeof(GLfixed));
      break;
   }
   case GLGETMATERIALXV_ID_11:
   {
      GLenum face = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLfixed params[4];

      int count = glGetMaterialxv_impl_11(face, pname, params);

      vcos_assert(count <= 4);

      khdispatch_write_out_ctrl(params, count * sizeof(GLfixed));
      break;
   }
   case GLGETLIGHTXV_ID_11:
   {
      GLenum light = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLfixed params[4];

      int count = glGetLightxv_impl_11(light, pname, params);

      vcos_assert(count <= 4);

      khdispatch_write_out_ctrl(params, count * sizeof(GLfixed));
      break;
   }
   case GLGETTEXENVIV_ID_11:
   {
      GLenum env = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLint params[4];

      int count = glGetTexEnviv_impl_11(env, pname, params);

      vcos_assert(count <= 4);

      khdispatch_write_out_ctrl(params, count * sizeof(GLint));
      break;
   }
   case GLGETTEXENVXV_ID_11:
   {
      GLenum env = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLfixed params[4];

      int count = glGetTexEnvxv_impl_11(env, pname, params);

      vcos_assert(count <= 4);

      khdispatch_write_out_ctrl(params, count * sizeof(GLfixed));
      break;
   }
   case GLGETTEXPARAMETERXV_ID_11:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLfixed params[1];

      int count = glGetTexParameterxv_impl_11(target, pname, params);

      vcos_assert(count <= 1);

      khdispatch_write_out_ctrl(params, count * sizeof(GLfixed));
      break;
   }
   case GLGETFIXEDV_ID_11:
   {
      GLenum pname = khdispatch_read_enum();

      GLfixed params[16];

      int count = glGetFixedv_impl_11(pname, params);

      vcos_assert(count <= 16);

      khdispatch_write_out_ctrl(params, count * sizeof(GLfixed));
      break;
   }

   case GLCOLORPOINTER_ID_11:
   {
      glColorPointer_impl_11();
      break;
   }
   case GLNORMALPOINTER_ID_11:
   {
      glNormalPointer_impl_11();
      break;
   }
   case GLTEXCOORDPOINTER_ID_11:
   {
      GLenum unit = khdispatch_read_enum();

      glTexCoordPointer_impl_11(unit);
      break;
   }
   case GLVERTEXPOINTER_ID_11:
   {
      glVertexPointer_impl_11();
      break;
   }
   case GLPOINTSIZEPOINTEROES_ID_11:
   {
      glPointSizePointerOES_impl_11();
      break;
   }
   case GLINTCOLOR_ID_11:
   {
      float red = khdispatch_read_float();
      float green = khdispatch_read_float();
      float blue = khdispatch_read_float();
      float alpha = khdispatch_read_float();

      glintColor_impl_11(red, green, blue, alpha);
      break;
   }
   case GLQUERYMATRIXXOES_ID_11:
   {
      GLfixed params[16];
      glQueryMatrixxOES_impl_11(params);
      khdispatch_write_out_ctrl(params, 16 * sizeof(GLfixed));
      break;
   }
   case GLDRAWTEXFOES_ID_11:
   {
      float x = khdispatch_read_float();
      float y = khdispatch_read_float();
      float z = khdispatch_read_float();
      float width = khdispatch_read_float();
      float height = khdispatch_read_float();

      glDrawTexfOES_impl_11(x, y, z, width, height);
      break;      
   }
   default:
      result = false;
      break;
   }
   return result;
}
