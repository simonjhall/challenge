/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#define GL_GLEXT_PROTOTYPES /* we want the prototypes so the compiler will check that the signatures match */

#include "interface/khronos/common/khrn_int_common.h"

#include "interface/khronos/glxx/glxx_client.h"
#include "interface/khronos/common/khrn_client_rpc.h"

#ifdef RPC_DIRECT
#include "interface/khronos/glxx/glxx_int_impl.h"
#include "interface/khronos/glxx/gl11_int_impl.h"
#endif

#include "interface/khronos/include/GLES/gl.h"
#include "interface/khronos/include/GLES/glext.h"

GL_API GLbitfield GL_APIENTRY glQueryMatrixxOES( GLfixed mantissa[16], GLint exponent[16] )
{
   if (IS_OPENGLES_11()) {
      int i;
      RPC_CALL1_OUT_CTRL(glQueryMatrixxOES_impl_11,
                            GLQUERYMATRIXXOES_ID_11,
                            mantissa);
               
      for(i=0;i<16;i++)
         exponent[i] = 0;
         
      return 0;                            
   }
   
   return 0xff; /* all components invalid */
}
