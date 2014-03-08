/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include "../khrn_image.h"
#include "../khrn_tformat.h"

//******************
//T-format convert
//******************

#ifdef __ARM_NEON__							//defined by RVCT 4.0 
__asm void neon_do_tf_16x16_copy( uint8_t *ut_src_addr, uint8_t *ut_dst_addr, uint32_t stride)
{
    stmfd sp!,{r4-r11,lr}
	sub r4,r2,#32

	//load first four u-tile
	vld4.8    {d0,d2,d4,d6},    [r0]!      //first u-tile
	vld4.8    {d1,d3,d5,d7},    [r0]!      
	vld4.8    {d8,d10,d12,d14}, [r0]!      //second
	vld4.8    {d9,d11,d13,d15}, [r0]!
	vld4.8    {d16,d18,d20,d22},[r0]!      //third
	vld4.8    {d17,d19,d21,d23},[r0]!
	vld4.8    {d24,d26,d28,d30},[r0]!      //fourth
	vld4.8    {d25,d27,d29,d31},[r0]! 

	vzip.32    q0,q4
	vzip.32    q1,q5
	vzip.32    q2,q6
	vzip.32    q3,q7
	vzip.32    q8,q12
	vzip.32    q9,q13
	vzip.32    q10,q14
	vzip.32    q11,q15
	
	vswp        d1,d16
	vswp        d9,d24
	vswp        d3,d18
	vswp        d11,d26
	vswp        d5,d20
	vswp        d13,d28
	vswp        d7,d22
	vswp        d15,d30
	
	//swap B and R
	vswp        q0,q2
	vswp        q8,q10
	vswp        q4,q6
	vswp        q12,q14

	//store 
	vst4.8    {d0,d2,d4,d6},    [r1]!      //first row
	vst4.8    {d1,d3,d5,d7},    [r1],r4
	vst4.8    {d16,d18,d20,d22},[r1]!      //second
	vst4.8    {d17,d19,d21,d23},[r1],r4
	vst4.8    {d8,d10,d12,d14}, [r1]!      //third
	vst4.8    {d9,d11,d13,d15}, [r1],r4
	vst4.8    {d24,d26,d28,d30},[r1]!      //fourth
	vst4.8    {d25,d27,d29,d31},[r1],r4

	//load second four u-tile
	vld4.8    {d0,d2,d4,d6},    [r0]!      
	vld4.8    {d1,d3,d5,d7},    [r0]!      
	vld4.8    {d8,d10,d12,d14}, [r0]!      
	vld4.8    {d9,d11,d13,d15}, [r0]!
	vld4.8    {d16,d18,d20,d22},[r0]!      
	vld4.8    {d17,d19,d21,d23},[r0]!
	vld4.8    {d24,d26,d28,d30},[r0]!      
	vld4.8    {d25,d27,d29,d31},[r0]! 

	vzip.32    q0,q4
	vzip.32    q1,q5
	vzip.32    q2,q6
	vzip.32    q3,q7
	vzip.32    q8,q12
	vzip.32    q9,q13
	vzip.32    q10,q14
	vzip.32    q11,q15
	
	vswp        d1,d16
	vswp        d9,d24
	vswp        d3,d18
	vswp        d11,d26
	vswp        d5,d20
	vswp        d13,d28
	vswp        d7,d22
	vswp        d15,d30
	
	//swap B and R
	vswp        q0,q2
	vswp        q8,q10
	vswp        q4,q6
	vswp        q12,q14

	//store 
	vst4.8    {d0,d2,d4,d6},    [r1]!      
	vst4.8    {d1,d3,d5,d7},    [r1],r4
	vst4.8    {d16,d18,d20,d22},[r1]!      
	vst4.8    {d17,d19,d21,d23},[r1],r4
	vst4.8    {d8,d10,d12,d14}, [r1]!      
	vst4.8    {d9,d11,d13,d15}, [r1],r4
	vst4.8    {d24,d26,d28,d30},[r1]!      
	vst4.8    {d25,d27,d29,d31},[r1],r4

	//load third four u-tile
	vld4.8    {d0,d2,d4,d6},    [r0]!      
	vld4.8    {d1,d3,d5,d7},    [r0]!      
	vld4.8    {d8,d10,d12,d14}, [r0]!      
	vld4.8    {d9,d11,d13,d15}, [r0]!
	vld4.8    {d16,d18,d20,d22},[r0]!      
	vld4.8    {d17,d19,d21,d23},[r0]!
	vld4.8    {d24,d26,d28,d30},[r0]!      
	vld4.8    {d25,d27,d29,d31},[r0]!    

	vzip.32    q0,q4
	vzip.32    q1,q5
	vzip.32    q2,q6
	vzip.32    q3,q7
	vzip.32    q8,q12
	vzip.32    q9,q13
	vzip.32    q10,q14
	vzip.32    q11,q15
	
	vswp        d1,d16
	vswp        d9,d24
	vswp        d3,d18
	vswp        d11,d26
	vswp        d5,d20
	vswp        d13,d28
	vswp        d7,d22
	vswp        d15,d30
	
	//swap B and R
	vswp        q0,q2
	vswp        q8,q10
	vswp        q4,q6
	vswp        q12,q14

	//store 
	vst4.8    {d0,d2,d4,d6},    [r1]!      
	vst4.8    {d1,d3,d5,d7},    [r1],r4
	vst4.8    {d16,d18,d20,d22},[r1]!      
	vst4.8    {d17,d19,d21,d23},[r1],r4
	vst4.8    {d8,d10,d12,d14}, [r1]!      
	vst4.8    {d9,d11,d13,d15}, [r1],r4
	vst4.8    {d24,d26,d28,d30},[r1]!      
	vst4.8    {d25,d27,d29,d31},[r1],r4     

	//load last four u-tile
	vld4.8    {d0,d2,d4,d6},    [r0]!      
	vld4.8    {d1,d3,d5,d7},    [r0]!      
	vld4.8    {d8,d10,d12,d14}, [r0]!      
	vld4.8    {d9,d11,d13,d15}, [r0]!
	vld4.8    {d16,d18,d20,d22},[r0]!      
	vld4.8    {d17,d19,d21,d23},[r0]!
	vld4.8    {d24,d26,d28,d30},[r0]!     
	vld4.8    {d25,d27,d29,d31},[r0]

	vzip.32    q0,q4
	vzip.32    q1,q5
	vzip.32    q2,q6
	vzip.32    q3,q7
	vzip.32    q8,q12
	vzip.32    q9,q13
	vzip.32    q10,q14
	vzip.32    q11,q15
	
	vswp        d1,d16
	vswp        d9,d24
	vswp        d3,d18
	vswp        d11,d26
	vswp        d5,d20
	vswp        d13,d28
	vswp        d7,d22
	vswp        d15,d30
	
	//swap B and R
	vswp        q0,q2
	vswp        q8,q10
	vswp        q4,q6
	vswp        q12,q14

	//store 
	vst4.8    {d0,d2,d4,d6},    [r1]!      
	vst4.8    {d1,d3,d5,d7},    [r1],r4
	vst4.8    {d16,d18,d20,d22},[r1]!      
	vst4.8    {d17,d19,d21,d23},[r1],r4
	vst4.8    {d8,d10,d12,d14}, [r1]!      
	vst4.8    {d9,d11,d13,d15}, [r1],r4
	vst4.8    {d24,d26,d28,d30},[r1]!      
	vst4.8    {d25,d27,d29,d31},[r1]

	ldmfd sp!,{r4-r11,pc}

}

__asm void neon_do_rso_64_copy( uint8_t *src_addr, uint8_t *dst_addr)
{
    stmfd sp!,{r4-r11,lr}

	//load 256 bytes
	vld4.8    {d0,d2,d4,d6},    [r0]!      
	vld4.8    {d1,d3,d5,d7},    [r0]!      
	vld4.8    {d8,d10,d12,d14}, [r0]!      
	vld4.8    {d9,d11,d13,d15}, [r0]!
	vld4.8    {d16,d18,d20,d22},[r0]!      
	vld4.8    {d17,d19,d21,d23},[r0]!
	vld4.8    {d24,d26,d28,d30},[r0]!      
	vld4.8    {d25,d27,d29,d31},[r0] 

	//swap B and R
	vswp        q0,q2
	vswp        q4,q6
	vswp        q8,q10
	vswp        q12,q14

	//store 
	vst4.8    {d0,d2,d4,d6},    [r1]!      
	vst4.8    {d1,d3,d5,d7},    [r1]!
	vst4.8    {d8,d10,d12,d14}, [r1]!      
	vst4.8    {d9,d11,d13,d15}, [r1]!
	vst4.8    {d16,d18,d20,d22},[r1]!      
	vst4.8    {d17,d19,d21,d23},[r1]!
	vst4.8    {d24,d26,d28,d30},[r1]!      
	vst4.8    {d25,d27,d29,d31},[r1]

	ldmfd sp!,{r4-r11,pc}

}

#else  //__ARM_NEON__

/************************************/
// C reference code implementation
/***********************************/

void neon_do_tf_16x16_copy( uint8_t *ut_src_addr, uint8_t *ut_dst_addr, uint32_t stride)
{
	uint32_t pixel_temp0, pixel_temp1, pixel_temp2, pixel_temp3;
	uint32_t i, j, k;

	uint8_t *utile_row;
	uint8_t *utile_ptr8;
	uint32_t *dst_ptr32;
	uint32_t *src_ptr32 = (uint32_t *)ut_src_addr;

	for(j = 0; j < 4; j++)
	{
		utile_row = (uint8_t *)ut_dst_addr + stride * j * 4;

		for(i = 0; i < 4; i++)
		{
			utile_ptr8 = utile_row + i * 16;
			for(k = 0; k < 4; k++)
			{
				dst_ptr32 = (uint32_t *)(utile_ptr8 + stride * k);

				pixel_temp0 = *src_ptr32++;
				pixel_temp1 = *src_ptr32++;
				pixel_temp2 = *src_ptr32++;
				pixel_temp3 = *src_ptr32++;

				pixel_temp0 =
					(((pixel_temp0 >> 0) & 0xff) << 16) |
					(((pixel_temp0 >> 8) & 0xff) << 8) |
					(((pixel_temp0 >> 16) & 0xff) << 0) |
					(pixel_temp0 & 0xff000000);
				pixel_temp1 =
					(((pixel_temp1 >> 0) & 0xff) << 16) |
					(((pixel_temp1 >> 8) & 0xff) << 8) |
					(((pixel_temp1 >> 16) & 0xff) << 0) |
					(pixel_temp1 & 0xff000000);
				pixel_temp2 =
					(((pixel_temp2 >> 0) & 0xff) << 16) |
					(((pixel_temp2 >> 8) & 0xff) << 8) |
					(((pixel_temp2 >> 16) & 0xff) << 0) |
					(pixel_temp2 & 0xff000000);
				pixel_temp3 =
					(((pixel_temp3 >> 0) & 0xff) << 16) |
					(((pixel_temp3 >> 8) & 0xff) << 8) |
					(((pixel_temp3 >> 16) & 0xff) << 0) |
					(pixel_temp3 & 0xff000000);

				dst_ptr32[0] = pixel_temp0;
				dst_ptr32[1] = pixel_temp1;
				dst_ptr32[2] = pixel_temp2;
				dst_ptr32[3] = pixel_temp3;
			}
		}
	}

	return;
}

void neon_do_rso_64_copy( uint8_t *src_addr, uint8_t *dst_addr)
{
	uint32_t pixel_temp0, pixel_temp1, pixel_temp2, pixel_temp3;
	uint32_t i;

	uint32_t *src_ptr32 = (uint32_t *)src_addr;
	uint32_t *dst_ptr32 = (uint32_t *)dst_addr;;

	for(i = 0; i < 16; i++)
	{
		pixel_temp0 = src_ptr32[0];
		pixel_temp1 = src_ptr32[1];
		pixel_temp2 = src_ptr32[2];
		pixel_temp3 = src_ptr32[3];

		pixel_temp0 =
			(((pixel_temp0 >> 0) & 0xff) << 16) |
			(((pixel_temp0 >> 8) & 0xff) << 8) |
			(((pixel_temp0 >> 16) & 0xff) << 0) |
			(pixel_temp0 & 0xff000000);
		pixel_temp1 =
			(((pixel_temp1 >> 0) & 0xff) << 16) |
			(((pixel_temp1 >> 8) & 0xff) << 8) |
			(((pixel_temp1 >> 16) & 0xff) << 0) |
			(pixel_temp1 & 0xff000000);
		pixel_temp2 =
			(((pixel_temp2 >> 0) & 0xff) << 16) |
			(((pixel_temp2 >> 8) & 0xff) << 8) |
			(((pixel_temp2 >> 16) & 0xff) << 0) |
			(pixel_temp2 & 0xff000000);
		pixel_temp3 =
			(((pixel_temp3 >> 0) & 0xff) << 16) |
			(((pixel_temp3 >> 8) & 0xff) << 8) |
			(((pixel_temp3 >> 16) & 0xff) << 0) |
			(pixel_temp3 & 0xff000000);

		dst_ptr32[0] = pixel_temp0;
		dst_ptr32[1] = pixel_temp1;
		dst_ptr32[2] = pixel_temp2;
		dst_ptr32[3] = pixel_temp3;

		src_ptr32 += 4;
		dst_ptr32 += 4;
	}

	return;
}

#endif //end of __ARM_NEON__


void neon_do_rso_64_copy_to_565( uint8_t *src_addr, uint8_t *dst_addr)
{
	uint32_t pixel_temp0, pixel_temp1, pixel_temp2, pixel_temp3;
	uint32_t i;

	uint32_t *src_ptr32 = (uint32_t *)src_addr;
	uint16_t *dst_ptr16 = (uint16_t *)dst_addr;;

	for(i = 0; i < 16; i++)
	{
		pixel_temp0 = src_ptr32[0];
		pixel_temp1 = src_ptr32[1];
		pixel_temp2 = src_ptr32[2];
		pixel_temp3 = src_ptr32[3];

		pixel_temp0 =
			(((pixel_temp0 >> 0) & 0xff) << 16) |
			(((pixel_temp0 >> 8) & 0xff) << 8) |
			(((pixel_temp0 >> 16) & 0xff) << 0) |
			(pixel_temp0 & 0xff000000);
		pixel_temp1 =
			(((pixel_temp1 >> 0) & 0xff) << 16) |
			(((pixel_temp1 >> 8) & 0xff) << 8) |
			(((pixel_temp1 >> 16) & 0xff) << 0) |
			(pixel_temp1 & 0xff000000);
		pixel_temp2 =
			(((pixel_temp2 >> 0) & 0xff) << 16) |
			(((pixel_temp2 >> 8) & 0xff) << 8) |
			(((pixel_temp2 >> 16) & 0xff) << 0) |
			(pixel_temp2 & 0xff000000);
		pixel_temp3 =
			(((pixel_temp3 >> 0) & 0xff) << 16) |
			(((pixel_temp3 >> 8) & 0xff) << 8) |
			(((pixel_temp3 >> 16) & 0xff) << 0) |
			(pixel_temp3 & 0xff000000);

		//pack to RGB565
		pixel_temp0 =
			(((pixel_temp0 >> 0) & 0xff) >> 3) |
			(((pixel_temp0 >> 8) & 0xfc) << 3) |
			(((pixel_temp0 >> 16) & 0xf8) << 8);
		pixel_temp1 =
			(((pixel_temp1 >> 0) & 0xff) >> 3) |
			(((pixel_temp1 >> 8) & 0xfc) << 3) |
			(((pixel_temp1 >> 16) & 0xf8) << 8);
		pixel_temp2 =
			(((pixel_temp2 >> 0) & 0xff) >> 3) |
			(((pixel_temp2 >> 8) & 0xfc) << 3) |
			(((pixel_temp2 >> 16) & 0xf8) << 8);
		pixel_temp3 =
			(((pixel_temp3 >> 0) & 0xff) >> 3) |
			(((pixel_temp3 >> 8) & 0xfc) << 3) |
			(((pixel_temp3 >> 16) & 0xf8) << 8);

		dst_ptr16[0] = (uint16_t)pixel_temp0;
		dst_ptr16[1] = (uint16_t)pixel_temp1;
		dst_ptr16[2] = (uint16_t)pixel_temp2;
		dst_ptr16[3] = (uint16_t)pixel_temp3;

		src_ptr32 += 4;
		dst_ptr16 += 4; 
	}

	return;
}

void neon_do_tf_16x16_copy_to_565( uint8_t *ut_src_addr, uint8_t *ut_dst_addr, uint32_t stride)
{
	uint32_t pixel_temp0, pixel_temp1, pixel_temp2, pixel_temp3;
	uint32_t i, j, k;

	uint8_t *utile_row;
	uint8_t *utile_ptr8;
	uint16_t *dst_ptr16;
	uint32_t *src_ptr32 = (uint32_t *)ut_src_addr;

	for(j = 0; j < 4; j++)
	{
		utile_row = (uint8_t *)ut_dst_addr + stride * j * 4;

		for(i = 0; i < 4; i++)
		{
			utile_ptr8 = utile_row + i * 8;   //2 bytes for 565
			for(k = 0; k < 4; k++)
			{
				dst_ptr16 = (uint16_t *)(utile_ptr8 + stride * k);

				pixel_temp0 = *src_ptr32++;
				pixel_temp1 = *src_ptr32++;
				pixel_temp2 = *src_ptr32++;
				pixel_temp3 = *src_ptr32++;

				pixel_temp0 =
					(((pixel_temp0 >> 0) & 0xff) << 16) |
					(((pixel_temp0 >> 8) & 0xff) << 8) |
					(((pixel_temp0 >> 16) & 0xff) << 0) |
					(pixel_temp0 & 0xff000000);
				pixel_temp1 =
					(((pixel_temp1 >> 0) & 0xff) << 16) |
					(((pixel_temp1 >> 8) & 0xff) << 8) |
					(((pixel_temp1 >> 16) & 0xff) << 0) |
					(pixel_temp1 & 0xff000000);
				pixel_temp2 =
					(((pixel_temp2 >> 0) & 0xff) << 16) |
					(((pixel_temp2 >> 8) & 0xff) << 8) |
					(((pixel_temp2 >> 16) & 0xff) << 0) |
					(pixel_temp2 & 0xff000000);
				pixel_temp3 =
					(((pixel_temp3 >> 0) & 0xff) << 16) |
					(((pixel_temp3 >> 8) & 0xff) << 8) |
					(((pixel_temp3 >> 16) & 0xff) << 0) |
					(pixel_temp3 & 0xff000000);

				//pack to RGB565
				pixel_temp0 =
					(((pixel_temp0 >> 0) & 0xff) >> 3) |
					(((pixel_temp0 >> 8) & 0xfc) << 3) |
					(((pixel_temp0 >> 16) & 0xf8) << 8);
				pixel_temp1 =
					(((pixel_temp1 >> 0) & 0xff) >> 3) |
					(((pixel_temp1 >> 8) & 0xfc) << 3) |
					(((pixel_temp1 >> 16) & 0xf8) << 8);
				pixel_temp2 =
					(((pixel_temp2 >> 0) & 0xff) >> 3) |
					(((pixel_temp2 >> 8) & 0xfc) << 3) |
					(((pixel_temp2 >> 16) & 0xf8) << 8);
				pixel_temp3 =
					(((pixel_temp3 >> 0) & 0xff) >> 3) |
					(((pixel_temp3 >> 8) & 0xfc) << 3) |
					(((pixel_temp3 >> 16) & 0xf8) << 8);

				dst_ptr16[0] = (uint16_t)pixel_temp0;
				dst_ptr16[1] = (uint16_t)pixel_temp1;
				dst_ptr16[2] = (uint16_t)pixel_temp2;
				dst_ptr16[3] = (uint16_t)pixel_temp3;
			}
		}
	}

	return;
}

static void neon_copy_tf_16x16_pixel_to_565(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
    void *ut_src_base;
    void *ut_dst_base;
    uint32_t ut_w = khrn_image_wrap_get_width_ut(src);
    uint32_t ut_x = src_x >> 2;
    uint32_t ut_y = src_y >> 2;

	assert(dst_x < dst->width);
	assert(dst_y < dst->height);
	assert(src_x < src->width);
	assert(src_y < src->height);

	//u-tile address
    ut_src_base = (uint8_t *)src->storage +
         (khrn_tformat_utile_addr(ut_w, ut_x, ut_y, khrn_image_is_tformat(src->format), NULL) * 64);

    ut_dst_base = (uint8_t *)dst->storage + (dst_y * dst->stride) + dst_x * 2;  //2 bytes for 565

	neon_do_tf_16x16_copy_to_565( (uint8_t *)ut_src_base, (uint8_t *)ut_dst_base, dst->stride);

	return;
}

static void neon_copy_tf_one_pixel_to_565(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
    void *ut_src_base;
    void *ut_dst_base;
	uint32_t pixel = 0;
    uint32_t ut_w = khrn_image_wrap_get_width_ut(src);
    uint32_t ut_x = src_x >> 2;
    uint32_t ut_y = src_y >> 2;

	assert(dst_x < dst->width);
	assert(dst_y < dst->height);
	assert(src_x < src->width);
	assert(src_y < src->height);

	src_x &= 0x3;
	src_y &= 0x3;

	//u-tile address
    ut_src_base = (uint8_t *)src->storage +
         (khrn_tformat_utile_addr(ut_w, ut_x, ut_y, khrn_image_is_tformat(src->format), NULL) * 64);

    ut_dst_base = (uint8_t *)dst->storage + (dst_y * dst->stride);

	pixel = ((uint32_t *)ut_src_base)[(src_y * 4) + src_x];
	pixel =
		(((pixel >> 0) & 0xff) << 16) |
		(((pixel >> 8) & 0xff) << 8) |
		(((pixel >> 16) & 0xff) << 0) |
		(pixel & 0xff000000);

	//pack to RGB565
    pixel =
        (((pixel >> 0) & 0xff) >> 3) |
        (((pixel >> 8) & 0xfc) << 3) |
        (((pixel >> 16) & 0xf8) << 8);

	((uint16_t *)ut_dst_base)[dst_x] = (uint16_t)pixel;

	return;
}

static void neon_copy_tf_16x16_pixel(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
    void *ut_src_base;
    void *ut_dst_base;
    uint32_t ut_w = khrn_image_wrap_get_width_ut(src);
    uint32_t ut_x = src_x >> 2;
    uint32_t ut_y = src_y >> 2;

	assert(dst_x < dst->width);
	assert(dst_y < dst->height);
	assert(src_x < src->width);
	assert(src_y < src->height);

	//u-tile address
    ut_src_base = (uint8_t *)src->storage +
         (khrn_tformat_utile_addr(ut_w, ut_x, ut_y, khrn_image_is_tformat(src->format), NULL) * 64);

    ut_dst_base = (uint8_t *)dst->storage + (dst_y * dst->stride) + dst_x * 4;

	neon_do_tf_16x16_copy( (uint8_t *)ut_src_base, (uint8_t *)ut_dst_base, dst->stride);

	return;
}

void neon_copy_16x16_pixel_region(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
	int32_t begin_i = 0, begin_j = 0;
	int32_t end_i = width, end_j = height;
	int32_t dir_i = 16, dir_j = 16;
	int32_t i, j;

	if (dst->storage == src->storage) 
	{
		if (dst_x > src_x) 
		{
			 int32_t temp = begin_i;
			 begin_i = end_i - 16;
			 end_i = temp - 16;
			 dir_i = -16;
		}
		if (dst_y > src_y) 
		{
			 int32_t temp = begin_j;
			 begin_j = end_j - 16;
			 end_j = temp - 16;
			 dir_j = -16;
		}
	}

	if(dst->format == RGB_565_RSO)
	{
		//do RGB_565_RSO converting
		for (j = begin_j; j != end_j; j += dir_j) 
		{
			for (i = begin_i; i != end_i; i += dir_i) 
			{
				neon_copy_tf_16x16_pixel_to_565(
					dst, dst_x + i, dst_y + j,
					src, src_x + i, src_y + j,
					conv);
			}
		}
		return;
	}

	for (j = begin_j; j != end_j; j += dir_j) 
	{
		for (i = begin_i; i != end_i; i += dir_i) 
		{
			neon_copy_tf_16x16_pixel(
				dst, dst_x + i, dst_y + j,
				src, src_x + i, src_y + j,
				conv);
		}
	}

	return;
}

static void neon_copy_tf_one_pixel(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
    void *ut_src_base;
    void *ut_dst_base;
	uint32_t pixel = 0;
    uint32_t ut_w = khrn_image_wrap_get_width_ut(src);
    uint32_t ut_x = src_x >> 2;
    uint32_t ut_y = src_y >> 2;

	assert(dst_x < dst->width);
	assert(dst_y < dst->height);
	assert(src_x < src->width);
	assert(src_y < src->height);

	src_x &= 0x3;
	src_y &= 0x3;

	//u-tile address
    ut_src_base = (uint8_t *)src->storage +
         (khrn_tformat_utile_addr(ut_w, ut_x, ut_y, khrn_image_is_tformat(src->format), NULL) * 64);

    ut_dst_base = (uint8_t *)dst->storage + (dst_y * dst->stride);

	pixel = ((uint32_t *)ut_src_base)[(src_y * 4) + src_x];
	pixel =
		(((pixel >> 0) & 0xff) << 16) |
		(((pixel >> 8) & 0xff) << 8) |
		(((pixel >> 16) & 0xff) << 0) |
		(pixel & 0xff000000);

	((uint32_t *)ut_dst_base)[dst_x] = pixel;

	return;
}

void neon_copy_one_pixel_region(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{

	int32_t begin_i = 0, begin_j = 0;
	int32_t end_i = width, end_j = height;
	int32_t dir_i = 1, dir_j = 1;
	int32_t i, j;

	if (dst->storage == src->storage) 
	{
		if (dst_x > src_x) 
		{
			 int32_t temp = begin_i;
			 begin_i = end_i - 1;
			 end_i = temp - 1;
			 dir_i = -1;
		}
		if (dst_y > src_y) 
		{
			 int32_t temp = begin_j;
			 begin_j = end_j - 1;
			 end_j = temp - 1;
			 dir_j = -1;
		}
	}

	if(dst->format == RGB_565_RSO)
	{
		//do RGB_565_RSO converting
		for (j = begin_j; j != end_j; j += dir_j) 
		{
			for (i = begin_i; i != end_i; i += dir_i) 
			{
				neon_copy_tf_one_pixel_to_565(
					dst, dst_x + i, dst_y + j,
					src, src_x + i, src_y + j,
					conv);
			}
		}
		return;
	}

	for (j = begin_j; j != end_j; j += dir_j) 
	{
		for (i = begin_i; i != end_i; i += dir_i) 
		{
			neon_copy_tf_one_pixel(
				dst, dst_x + i, dst_y + j,
				src, src_x + i, src_y + j,
				conv);
		}
	}

	return;
}


static bool neon_copy_abgr_tf_to_argb_rso(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
	int32_t dst_x_m16 = dst_x, dst_y_m16 = dst_y;
	int32_t width_m16 = width, height_m16 = height;
	int32_t src_x_m16 = src_x, src_y_m16 = src_y;
	int32_t w, h;

	assert((dst_x + width) <= dst->width);
	assert((dst_y + height) <= dst->height);
	assert((src_x + width) <= src->width);
	assert((src_y + height) <= src->height);

	if ((width == 0) || (height == 0)) 
	{
		return true;
	}

	if (src_x & 0xf)
	{
		src_x_m16 = (src_x + 0xf)&~0xf;
		w = src_x_m16 - src_x;
		dst_x_m16 = dst_x + w;
		width_m16 = width - w;

		neon_copy_one_pixel_region(dst, dst_x, dst_y, w, height, src, src_x, src_y, conv);
	}

	if (width_m16 & 0xf)
	{
		w = width_m16 & 0xf;
		width_m16 &= ~0xf;

		neon_copy_one_pixel_region(dst, dst_x_m16 + width_m16, dst_y, w, height, src, src_x_m16 + width_m16, src_y, conv);
	}

	if (src_y & 0xf)
	{
		src_y_m16 = (src_y + 0xf)&~0xf;
		h = src_y_m16 - src_y;
		dst_y_m16 = dst_y + h;
		height_m16 = height - h;

		neon_copy_one_pixel_region(dst, dst_x_m16, dst_y, width_m16, h, src, src_x_m16, src_y, conv);
	}

	if (height_m16 & 0xf)
	{
		h = height_m16 & 0xf;
		height_m16 &= ~0xf;

		neon_copy_one_pixel_region(dst, dst_x_m16, dst_y_m16 + height_m16, width_m16, h, src, src_x_m16, src_y_m16 + height_m16, conv);
	}

	if ((width_m16 == 0) || (height_m16 == 0)) 
	{
		return true;
	}

	neon_copy_16x16_pixel_region(dst, dst_x_m16, dst_y_m16, width_m16, height_m16, src, src_x_m16, src_y_m16, conv);

	return true;
}

static void neon_copy_rso_64_pixel_to_565(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
    void *src_base;
    void *dst_base;

	assert(dst_x < dst->width);
	assert(dst_y < dst->height);
	assert(src_x < src->width);
	assert(src_y < src->height);

    src_base = (uint8_t *)src->storage + (src_y * src->stride) + src_x * 4;
	dst_base = (uint8_t *)dst->storage + (dst_y * dst->stride) + dst_x * 2;   //2 bytes for 565

	neon_do_rso_64_copy_to_565( (uint8_t *)src_base, (uint8_t *)dst_base);

	return;
}

static void neon_copy_rso_64_pixel(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
    void *src_base;
    void *dst_base;

	assert(dst_x < dst->width);
	assert(dst_y < dst->height);
	assert(src_x < src->width);
	assert(src_y < src->height);

    src_base = (uint8_t *)src->storage + (src_y * src->stride) + src_x * 4;
	dst_base = (uint8_t *)dst->storage + (dst_y * dst->stride) + dst_x * 4;

	neon_do_rso_64_copy( (uint8_t *)src_base, (uint8_t *)dst_base);

	return;
}

void neon_copy_rso_region_m64(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
	int32_t begin_i = 0, begin_j = 0;
	int32_t end_i = width, end_j = height;
	int32_t dir_i = 64, dir_j = 1;
	int32_t i, j;

	if (dst->storage == src->storage) 
	{
		if (dst_x > src_x) 
		{
			 int32_t temp = begin_i;
			 begin_i = end_i - 64;
			 end_i = temp - 64;
			 dir_i = -64;
		}
		if (dst_y > src_y) 
		{
			 int32_t temp = begin_j;
			 begin_j = end_j - 1;
			 end_j = temp - 1;
			 dir_j = -1;
		}
	}

	if(dst->format == RGB_565_RSO)
	{
		//do RGB_565_RSO converting
		for (j = begin_j; j != end_j; j += dir_j) 
		{
			for (i = begin_i; i != end_i; i += dir_i) 
			{
				neon_copy_rso_64_pixel_to_565(
					dst, dst_x + i, dst_y + j,
					src, src_x + i, src_y + j,
					conv);
			}
		}
		return;
	}

	for (j = begin_j; j != end_j; j += dir_j) 
	{
		for (i = begin_i; i != end_i; i += dir_i) 
		{
			neon_copy_rso_64_pixel(
				dst, dst_x + i, dst_y + j,
				src, src_x + i, src_y + j,
				conv);
		}
	}

	return;
}

static void neon_copy_rso_one_pixel_to_565(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
    void *src_base;
    void *dst_base;
	uint32_t pixel = 0;

	assert(dst_x < dst->width);
	assert(dst_y < dst->height);
	assert(src_x < src->width);
	assert(src_y < src->height);

    src_base = (uint8_t *)src->storage + (src_y * src->stride);
	dst_base = (uint8_t *)dst->storage + (dst_y * dst->stride);

	pixel = ((uint32_t *)src_base)[src_x];
	pixel =
		(((pixel >> 0) & 0xff) << 16) |
		(((pixel >> 8) & 0xff) << 8) |
		(((pixel >> 16) & 0xff) << 0) |
		(pixel & 0xff000000);

	//pack to RGB565
    pixel =
        (((pixel >> 0) & 0xff) >> 3) |
        (((pixel >> 8) & 0xfc) << 3) |
        (((pixel >> 16) & 0xf8) << 8);

	((uint16_t *)dst_base)[dst_x] = (uint16_t)pixel;

	return;
}

static void neon_copy_rso_one_pixel(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
    void *src_base;
    void *dst_base;
	uint32_t pixel = 0;

	assert(dst_x < dst->width);
	assert(dst_y < dst->height);
	assert(src_x < src->width);
	assert(src_y < src->height);

    src_base = (uint8_t *)src->storage + (src_y * src->stride);
	dst_base = (uint8_t *)dst->storage + (dst_y * dst->stride);

	pixel = ((uint32_t *)src_base)[src_x];
	pixel =
		(((pixel >> 0) & 0xff) << 16) |
		(((pixel >> 8) & 0xff) << 8) |
		(((pixel >> 16) & 0xff) << 0) |
		(pixel & 0xff000000);

	((uint32_t *)dst_base)[dst_x] = pixel;

	return;
}

void neon_copy_rso_region_m1(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
	int32_t begin_i = 0, begin_j = 0;
	int32_t end_i, end_j;
	int32_t dir_i = 64, dir_j = 1;
	int32_t i, j, k;
	int32_t begin_k, end_k, dir_k = 1;

	end_i = width & ~0x3f;
	end_j = height;
	end_k = width;
	begin_k = end_i;

	if (dst->storage == src->storage) 
	{
		if (dst_x > src_x) 
		{
			 int32_t temp = width & 0x3f;
			 begin_i = width - 64;
			 end_i = temp - 64;
			 dir_i = -64;
			 begin_k = temp -1;
			 end_k = -1;
			 dir_k = -1;
		}
		if (dst_y > src_y) 
		{
			 int32_t temp = begin_j;
			 begin_j = end_j - 1;
			 end_j = temp - 1;
			 dir_j = -1;
		}
	}

	if(dst->format == RGB_565_RSO)
	{
		//do RGB_565_RSO converting
		for(j = begin_j; j != end_j; j += dir_j) 
		{
			for (i = begin_i; i != end_i; i += dir_i) 
			{
				neon_copy_rso_64_pixel_to_565(
					dst, dst_x + i, dst_y + j,
					src, src_x + i, src_y + j,
					conv);
			}
			for (k = begin_k; k != end_k; k += dir_k) 
			{
				neon_copy_rso_one_pixel_to_565(
					dst, dst_x + k, dst_y + j,
					src, src_x + k, src_y + j,
					conv);
			}
		}
		return;
	}

	for(j = begin_j; j != end_j; j += dir_j) 
	{
		for (i = begin_i; i != end_i; i += dir_i) 
		{
			neon_copy_rso_64_pixel(
				dst, dst_x + i, dst_y + j,
				src, src_x + i, src_y + j,
				conv);
		}
		for (k = begin_k; k != end_k; k += dir_k) 
		{
			neon_copy_rso_one_pixel(
				dst, dst_x + k, dst_y + j,
				src, src_x + k, src_y + j,
				conv);
		}
	}

	return;
}

static bool neon_copy_abgr_rso_to_argb_rso(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
	assert((dst_x + width) <= dst->width);
	assert((dst_y + height) <= dst->height);
	assert((src_x + width) <= src->width);
	assert((src_y + height) <= src->height);

	if ((width == 0) || (height == 0)) 
	{
		return true;
	}

	if (width & 0x3f) 
	{
		neon_copy_rso_region_m1(dst, dst_x, dst_y, width, height, src, src_x, src_y, conv);
		return true;
	}

	neon_copy_rso_region_m64(dst, dst_x, dst_y, width, height, src, src_x, src_y, conv);

	return true;
}

bool khrn_bf_copy_region(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv)
{
	//copy region from ABGR_8888_TF to ARGB_8888_RSO format or RGB_565_RSO format
	if ((src->format == ABGR_8888_TF ) && ((dst->format == ARGB_8888_RSO) || (dst->format == RGB_565_RSO)) && (conv == IMAGE_CONV_GL))
		return neon_copy_abgr_tf_to_argb_rso(dst, dst_x, dst_y, width, height, src, src_x, src_y,conv);

	//copy region from ABGR_8888_RSO to ARGB_8888_RSO format or RGB_565_RSO format
	if ((src->format == ABGR_8888_RSO ) && ((dst->format == ARGB_8888_RSO) || (dst->format == RGB_565_RSO)) && (conv == IMAGE_CONV_GL))
		return neon_copy_abgr_rso_to_argb_rso(dst, dst_x, dst_y, width, height, src, src_x, src_y,conv);

	return false;
}

bool khrn_bf_clear_region(
   KHRN_IMAGE_WRAP_T *wrap, uint32_t x, uint32_t y,
   uint32_t width, uint32_t height,
   uint32_t rgba, /* rgba non-lin, unpre */
   KHRN_IMAGE_CONV_T conv)
{
	return false;
}

bool khrn_bf_copy_scissor_regions(
   KHRN_IMAGE_WRAP_T *dst, uint32_t dst_x, uint32_t dst_y,
   uint32_t width, uint32_t height,
   const KHRN_IMAGE_WRAP_T *src, uint32_t src_x, uint32_t src_y,
   KHRN_IMAGE_CONV_T conv,
   const int32_t *scissor_rects, uint32_t scissor_rects_count)
{
	return false;
}


bool khrn_bf_subsample(KHRN_IMAGE_WRAP_T *dst, const KHRN_IMAGE_WRAP_T *src)
{
	return false;
}






