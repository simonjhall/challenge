/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/**
 * \file
 *
 * This file is not part of the public interface.
 * Objects declared here should not be used externally.
 */

#ifndef VC_IMAGE_USAGE_I_H
#define VC_IMAGE_USAGE_I_H

#ifdef USE_RGB565
#  define IFWANT_RGB565(yes, no) (yes)
#else
#  define IFWANT_RGB565(yes, no) (no)
#endif
#ifdef USE_RGB888
#  define IFWANT_RGB888(yes, no) (yes)
#else
#  define IFWANT_RGB888(yes, no) (no)
#endif
#ifdef USE_RGBA32
#  define IFWANT_RGBA32(yes, no) (yes)
#else
#  define IFWANT_RGBA32(yes, no) (no)
#endif
#ifdef USE_RGBA16
#  define IFWANT_RGBA16(yes, no) (yes)
#else
#  define IFWANT_RGBA16(yes, no) (no)
#endif
#ifdef USE_RGBA565
#  define IFWANT_RGBA565(yes, no) (yes)
#else
#  define IFWANT_RGBA565(yes, no) (no)
#endif
#ifdef USE_YUV420
#  define IFWANT_YUV420(yes, no) (yes)
#else
#  define IFWANT_YUV420(yes, no) (no)
#endif
#ifdef USE_YUV422
#  define IFWANT_YUV422(yes, no) (yes)
#else
#  define IFWANT_YUV422(yes, no) (no)
#endif
#ifdef USE_3D32
#  define IFWANT_3D32(yes, no) (yes)
#else
#  define IFWANT_3D32(yes, no) (no)
#endif
#ifdef USE_4BPP
#  define IFWANT_4BPP(yes, no) (yes)
#else
#  define IFWANT_4BPP(yes, no) (no)
#endif
#ifdef USE_8BPP
#  define IFWANT_8BPP(yes, no) (yes)
#else
#  define IFWANT_8BPP(yes, no) (no)
#endif
#ifdef USE_RGB2X9
#  define IFWANT_RGB2X9(yes, no) (yes)
#else
#  define IFWANT_RGB2X9(yes, no) (no)
#endif
#ifdef USE_TFORMAT
#  define IFWANT_TFORMAT(yes, no) (yes)
#else
#  define IFWANT_TFORMAT(yes, no) (no)
#endif

#endif

