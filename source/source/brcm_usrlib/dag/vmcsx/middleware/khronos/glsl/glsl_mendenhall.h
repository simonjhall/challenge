/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef GLSL_MENDENHALL_H
#define GLSL_MENDENHALL_H

#define I_CONST_RECIP2PI	0x3e22f983
#define F_CONST_RECIP2PI	0.1591549431
#define I_CONST_TWO		0x40000000
#define F_CONST_TWO		2.0
#define I_COEFF_COS0		0x3f800000
#define F_COEFF_COS0		1.0
#define I_COEFF_COS2		0xbf9de9cf
#define F_COEFF_COS2		-1.2336977925
#define I_COEFF_COS4		0x3e81d8fd
#define F_COEFF_COS4		0.2536086171
#define I_COEFF_COS6		0xbca77008
#define F_COEFF_COS6		-0.0204391631
#define I_COEFF_SIN1		0x3fc90fdb
#define F_COEFF_SIN1		1.5707963235
#define I_COEFF_SIN3		0xbf255ddf
#define F_COEFF_SIN3		-0.645963615
#define I_COEFF_SIN5		0x3da3304e
#define F_COEFF_SIN5		0.0796819754
#define I_COEFF_SIN7		0xbb96fb24
#define F_COEFF_SIN7		-0.0046075748

// For lower accuracy, but a bit faster, we can skip the final error correction.
// Note this also updates definitions in header.shader.c!
// #define SKIP_CORRECTION

extern void glsl_mendenhall_sincospair(uint32_t a, uint32_t *s, uint32_t *c);

#endif // MENDENHALL_H
