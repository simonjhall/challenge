/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GL11_INT_CONFIG_H
#define GL11_INT_CONFIG_H

#define GL11_CONFIG_MAX_TEXTURE_UNITS              4
#define GL11_CONFIG_MAX_LIGHTS                     8
#define GL11_CONFIG_MAX_PLANES                     1
#define GL11_CONFIG_MAX_STACK_DEPTH               16
#define GL11_CONFIG_SUBPIXEL_BITS                  4        // number of bits of sub-pixel precision in rasterizer
#define GL11_CONFIG_SAMPLES                        4

#define GL11_CONFIG_MIN_ALIASED_POINT_SIZE      1.0f
#define GL11_CONFIG_MAX_ALIASED_POINT_SIZE    256.0f
#define GL11_CONFIG_MIN_SMOOTH_POINT_SIZE       1.0f
#define GL11_CONFIG_MAX_SMOOTH_POINT_SIZE     256.0f
#define GL11_CONFIG_MIN_ALIASED_LINE_WIDTH      0.0f
#define GL11_CONFIG_MAX_ALIASED_LINE_WIDTH     16.0f
#define GL11_CONFIG_MIN_SMOOTH_LINE_WIDTH       0.0f
#define GL11_CONFIG_MAX_SMOOTH_LINE_WIDTH      16.0f

#endif
