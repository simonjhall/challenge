/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VCLIB_H
#define VCLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vcinclude/common.h"

#include "vcfw/vclib/vclib.h"

#include "helpers/vclib/vclib_hash.h"
#include "helpers/vclib/vclib_crc.h"

 #ifdef __VIDEOCORE__

#include "vcfw/rtos/rtos.h"

typedef struct
{
   unsigned long bits[624];
   unsigned long tempered[16];
   unsigned long magic;
   int index;
} VCLIB_RAND_STATE_T;


typedef struct
{
   uint32_t vrf_save[1024];
   void *checkpoints;
   uint32_t length;
   uint32_t progress;
   uint32_t breakpoint;
   unsigned generate : 1,
            lockable : 1;
   uint32_t magic;
} VCLIB_CHECKPOINT_STATE_T;


// Call vclib_obtain_VRF() before calling these functions. Release VRF using vclib_release_VRF() afterwards.
extern void vclib_memcpy( void *to, const void *from, int numbytes );
extern void vclib_memcpy_512( void *to, const void *from, int numbytes ); /* Copies multiples of 512 bytes */
extern void vclib_memcpy64( void *to, const void *from, int numbytes );

extern void vclib_memmove( void *to, const void *from, int numbytes );

extern void vclib_memset( void *to, int value, int numbytes );
extern void vclib_memset2( void *to, int value, int numhalfwords );
extern void vclib_memset4( void *to, int value, int numwords );
extern void vclib_aligned_memset2( void *to, int value, int numhalfwords );

#if _VC_VERSION >= 3
   extern int vclib_memcmp( const void *a, const void *b, int numbytes );
#endif // if _VC_VERSION >= 3

extern void* vclib_memchr(const void *from, int c, int n);


extern void vclib_checksum(void *buffer, unsigned int length, unsigned int *checksum);

extern void vclib_srand(VCLIB_RAND_STATE_T *state, uint32_t seed);
extern void vclib_srand_buffer(VCLIB_RAND_STATE_T *state, void const *buffer, int length);
extern uint32_t vclib_rand32(VCLIB_RAND_STATE_T *state);
extern void vclib_memrand(VCLIB_RAND_STATE_T *state, void *dest, int length);
extern void vclib_memrand_stateless(void *dest, int length, uint32_t seed);

extern void vclib_medianfilter(unsigned char *src, unsigned char *dest, int width, int height);

extern int vclib_get_tmp_buf_size(void); // Get the size of the shared buffer
extern void *vclib_obtain_tmp_buf(int size);
extern void vclib_release_tmp_buf(void *tmpbuf);

extern void *vclib_get_tmp_buf(void); // xxx deprecated

extern void *vclib_changestack(void *stack);
extern void vclib_restorestack(void *stack);

/* Get the max/min and sum value of a block of unsigned char */
extern void vclib_stat_uchar( unsigned char *addr, int width, int height, int pitch,
                              unsigned int *max, unsigned int *min, unsigned int *sum);

// Save or restore registers 16-23 inclusive into the given memory block of at least 32 bytes.
// Probably you should assume these go with the VRF lock. If you release the VRF but care
// what's in these registers when you get it back, then use these functions.
extern void vclib_save_high_registers(int *storage);
extern void vclib_restore_high_registers(int *storage);

extern int vclib_textsize (int font,char *text);
extern int vclib_textwrite (unsigned short *buffer,int pitch,int sx,int sy,
                               int fx,int fy,int fcolour,int bcolour,int font,const char *text);
extern void vclib_textslider (unsigned short *buffer,int pitch,int sx,int sy,
                                 int fx,int fy,int pos);

/** Read \a count 24-bit words from \a src, sign-extend them to 32-bit, and write them to \a dst.  Operates from far end of the buffer so \a dst and \a src may be the same value. */
extern void vclib_extend_24to32(void *dst, void const *src, int count);
extern void vclib_extend_24to32hi(void *dst, void const *src, int count);

/** Read \a count 8-bit unsigned bytes from \a src, re-bias and shift up to 16-bit, and write them back to \a dst.  Operates from far end of the buffer so \a dst and \a src may be the same value. */
extern void vclib_extend_u8to16hi(void *dst, void const *src, int count);

/** Read \a count 8-bit u-law bytes from \a src, extend to 16-bit, and write them back to \a dst.  Operates from far end of the buffer so \a dst and \a src may be the same value. */
extern void vclib_extend_ulawto16(void *dst, void const *src, int count);

/** Read \a count 8-bit A-law bytes from \a src, extend to 16-bit, and write them back to \a dst.  Operates from far end of the buffer so \a dst and \a src may be the same value. */
extern void vclib_extend_alawto16(void *dst, void const *src, int count);

/** Read \a count 32-bit words from \a src, truncate them to 24-bit, and write them to \a dst. Operates from near end of the buffer so \a dst and \a src may be the same value. */
extern void vclib_unextend_32to24(void *dst, void const *src, int count);
extern void vclib_unextend_32to24s(void *dst, void const *src, int count); /* signed-saturating */
extern void vclib_unextend_32hito24(void *dst, void const *src, int count);


/** Read \a count 32-bit signed words, shift left by \a shift (signed), and write them back to \a dst. */
extern void vclib_signasl_32(void *dst, void const *src, int shift, int count);

/** Read \a count 32-bit signed words, shift left by \a shift (signed, saturating), and write them back to \a dst. */
extern void vclib_signasls_32(void *dst, void const *src, int shift, int count);

/** Read \a count bytes from \a src, swap odd and even byte positions, and write them back to \a dst.  Count should be a multiple of two and addresses should be appropriately aligned. */
extern void vclib_swab(void *dst, void const *src, int count);

/** Read \a count bytes from \a src, reverse the order of every set of four bytes, and write them back to \a dst.  Count should be a multiple of four and addresses should be appropriately aligned. */
extern void vclib_swab2(void *dst, void const *src, int count);


/** Read \a spitch bytes from \a src, pad them with zeroes to \a dpitch length and write to \a dst; repeat this \a count times. */
extern void vclib_spread(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);

/** Read \a spitch half-words from \a src, pad them with zeroes to \a dpitch length and write to \a dst; repeat this \a count times. */
extern void vclib_spread2(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);

/** Read \a spitch 24-bit words from \a src, sign-extend them to 32-bit and then pad them with zeroes to \a dpitch length and write to \a dst; repeat this \a count times.  Operates from the far end of the buffer so \a dst and \a src may be the same value. */
extern void vclib_spread_24to32(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);
extern void vclib_spread_24to32hi(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);

/** Read \a spitch 8-bit bytes from \a src, extend them to 16-bit and then pad them with zeroes to \a dpitch length and write to \a dst; repeat this \a count times.  Operates from the far end of the buffer so \a dst and \a src may be the same value. */
extern void vclib_spread_u8to16hi(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);
extern void vclib_spread_ulawto16(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);
extern void vclib_spread_alawto16(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);


/** Read \a spitch bytes from \a src, discard all but \a dpitch length bytes and write to \a dst; repeat this \a count times. */
extern void vclib_unspread(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);

/** Read \a spitch half-words from \a src, discard all but \a dpitch length half-words and write to \a dst; repeat this \a count times. */
extern void vclib_unspread2(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);

/** Read \a spitch words from \a src, discard all but \a dpitch length words and write to \a dst; repeat this \a count times. */
extern void vclib_unspread4(void *dst, unsigned int dpitch, void const *src, unsigned int spitch, unsigned int count);

/** Interleave two separate buffers of \a count half-words from \a src0 and \a src1 into \a dst.
 * A source operand of NULL implies zero-padding in that field. */
extern void vclib_interleave2_2(void *dst, void const *src0, void const *src1, int count);

/** Interleave four separate buffers of \a count half-words from \a src0, \a src1, \a src2, and \a src3 into \a dst.
 * A source operand of NULL implies zero-padding in that field. */
extern void vclib_interleave2_4(void *dst, void const *src0, void const *src1, void const *src2, void const *src3, int count);

/** Interleave eight separate buffers of \a count half-words from \a src0, \a src1, \a src2, \a src3, \a src4, \a src5, \a src6, and \a src7 into \a dst.
 * A source operand of NULL implies zero-padding in that field. */
extern void vclib_interleave2_8(void *dst, void const *src0, void const *src1, void const *src2, void const *src3, void const *src4, void const *src5, void const *src6, void const *src7, int count);

/** Interleave two separate buffers of \a count words from \a src0 and \a src1 into \a dst.
 * A source operand of NULL implies zero-padding in that field. */
extern void vclib_interleave4_2(void *dst, void const *src0, void const *src1, int count);

/** Interleave four separate buffers of \a count words from \a src0, \a src1, \a src2, and \a src3 into \a dst.
 * A source operand of NULL implies zero-padding in that field. */
extern void vclib_interleave4_4(void *dst, void const *src0, void const *src1, void const *src2, void const *src3, int count);

/** Interleave eight separate buffers of \a count words from \a src0, \a src1, \a src2, \a src3, \a src4, \a src5, \a src6, and \a src7 into \a dst.
 * A source operand of NULL implies zero-padding in that field. */
extern void vclib_interleave4_8(void *dst, void const *src0, void const *src1, void const *src2, void const *src3, void const *src4, void const *src5, void const *src6, void const *src7, int count);

/** Reorder the \a count bytes from \a src into \a dst, using a repeating pattern \a period long, specified by \a indicies.  \a period must be a power of two and no greater than 16. */
extern void vclib_reorder(void *dst, void const *src, signed char const *indices, int period, int count);

/** Reorder the \a count half-words from \a src into \a dst, using a repeating pattern \a period long, specified by \a indicies.  \a period must be a power of two and no greater than 16. */
extern void vclib_reorder2(void *dst, void const *src, signed char const *indices, int period, int count);

/** Reorder the \a count words from \a src into \a dst, using a repeating pattern \a period long, specified by \a indicies.  \a period must be a power of two and no greater than 16. */
extern void vclib_reorder4(void *dst, void const *src, signed char const *indices, int period, int count);

/** Generate or verify a stream of checksums generated at various points in the
 * execution of a program.  It is the caller's responsibility to ensure that
 * each thread uses a unique state buffer, because there are no internal guards
 * against race conditions or unpredictable ordering across multiple threads.
 */

extern int vclib_checkpoint_init(VCLIB_CHECKPOINT_STATE_T *state, void *buffer, unsigned int length, int generate);
extern int vclib_checkpoint_check_VRF(VCLIB_CHECKPOINT_STATE_T *state);
#define vclib_checkpoint_reset_VRF(state) _vasm("v32mov H32(0++,0), 0 REP 64\nmov %D,1")
extern int vclib_checkpoint_check_buffer(VCLIB_CHECKPOINT_STATE_T *state, void const *buffer, unsigned int length);
extern int vclib_checkpoint_breakpoint(VCLIB_CHECKPOINT_STATE_T *state, uint32_t progress);
extern int vclib_checkpoint_length(VCLIB_CHECKPOINT_STATE_T *state);
typedef enum
{
   VCLIB_CHECKPOINT_DUMP_RAW = 0,
   VCLIB_CHECKPOINT_DUMP_C_SOURCE,
} VCLIB_CHECKPOINT_DUMP_FORMAT_T;
extern int vclib_checkpoint_dump(VCLIB_CHECKPOINT_STATE_T *state, char const *filename, VCLIB_CHECKPOINT_DUMP_FORMAT_T format);

/** Set a default pointer to be used by vclib_checkpoint_*() functions when
 * state == NULL.  This relieves applications of the burden of passing a
 * checkpoint state handle around code that wouldn't otherwise need to be
 * modified to support a diagnostic feature; but it clearly allows only a
 * single thread to operate this way.
 */
extern int vclib_checkpoint_set_default(VCLIB_CHECKPOINT_STATE_T *default_state);
extern int vclib_checkpoint_clear_default(VCLIB_CHECKPOINT_STATE_T *current_default_state);

/** Endian reverse an array of 32bit words. */
extern void vc_endianreverse32(uint32_t *addr,int count);

#ifdef __HIGHC__
   #define VCC_ALIGNTO32
#else
   #define VCC_ALIGNTO32 __attribute__((aligned (32)))
#endif

/* See AlphaMosaic document C6357-M-285f VC02 Chip Spec (page 79) */

#define VC_VERSION_VC02A0                               0x2000020
/* Note that the hardware part version was not correctly updated for VC02A1*/
#define VC_VERSION_VC02A1                               0x2000020
#define VC_VERSION_VC02A2                               0x2000022
#define VC_VERSION_VC02A3                               0x2000023
#define VC_VERSION_VC02B0                               0x2000024
#define VC_VERSION_VC02B1                               0x2000025
#define VC_VERSION_VC02B2                               0x2000026
#define VC_VERSION_VC02B1M                              0x2000035
#define VC_VERSION_VC05A0                               0x2000040

#define VC_VERSION_PART_CODE_VC05                       0x0004
#define VC_VERSION_PART_CODE_VC02                       0x0002


static inline unsigned vclib_processor_version(void)
{
   return _vasm("version %D");
}
static inline unsigned vclib_processor_version_proc_code(void)
{
   return 0xff & (vclib_processor_version() >> 24);
}
static inline unsigned vclib_processor_version_proc_version(void)
{
   return 0xf & (vclib_processor_version() >> 20);
}
static inline unsigned vclib_processor_version_part_code(void)
{
   return 0xff & (vclib_processor_version() >> 4);
}
static inline unsigned vclib_processor_version_part_version(void)
{
   return 0xf & vclib_processor_version();
}

 #else //if building for any non-Videocore platform

#define vclib_memcpy     memcpy
#define vclib_memcpy_512 memcpy
#define vclib_memcpy64   memcpy

#define vclib_memmove memmove
#define vclib_memset memset
#define vclib_memset2 memset
#define vclib_memset4 memset
#define vclib_aligned_memset2 memset
#define vclib_memcmp memcmp
#define vclib_memchr memchr
#define vclib_release_tmp_buf(ptr) free(ptr)

 #endif

#ifdef __cplusplus
}
#endif

#endif //ifndef VCLIB_H
