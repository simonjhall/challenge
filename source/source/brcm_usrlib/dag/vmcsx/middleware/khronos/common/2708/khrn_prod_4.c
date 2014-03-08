#define LOG_TAG	"KHRN_PROD_4"

//#define KHRN_HW_MEASURE_BIN_TIME
//#define KHRN_HW_MEASURE_RENDER_TIME
//#define KHRN_HW_BIN_HISTORY_N 1024 /* should be a multiple of 2 */
//#define KHRN_HW_RENDER_HISTORY_N 1024 /* should be a multiple of 2 */
//#define KHRN_HW_BOOM /* for testing the can't-allocate-required-overflow-memory case */
//#define KHRN_HW_QPU_PROFILING_DUMP_PRE
//#define KHRN_HW_QPU_PROFILING_RESET_PRE
//#define KHRN_HW_QPU_PROFILING_DUMP_POST
//#define KHRN_HW_QPU_PROFILING_RESET_POST

/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/common/2708/khrn_prod_4.h"
#include "middleware/khronos/common/2708/khrn_pool_4.h"
#include "middleware/khronos/common/2708/khrn_worker_4.h"
#include "middleware/khronos/common/khrn_hw.h"
#include "middleware/khronos/common/khrn_llat.h"
#include "middleware/khronos/common/khrn_mem.h"
#include "middleware/khronos/common/khrn_stats.h"
#include "interface/khronos/common/khrn_int_util.h"
#include "middleware/khronos/egl/egl_disp.h"

#ifndef SIMPENROSE
   #include "vcfw/drivers/chip/v3d.h"
#endif
#ifdef __cplusplus
}
#endif
#include <stddef.h> /* for offsetof */
#if defined(KHRN_HW_MEASURE_BIN_TIME) || defined(KHRN_HW_MEASURE_RENDER_TIME) || defined(KHRN_HW_QPU_PROFILING_DUMP_PRE) || defined(KHRN_HW_QPU_PROFILING_DUMP_POST) || defined(CARBON)
   #include <stdio.h>
#endif
#ifdef SIMPENROSE_RECORD_OUTPUT
   #include <string>
   #include <strstream>
   #include <vector>
#endif

//#include <cutils/log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>

//#include "utils/Log.h"
#define LOGE printf
#define LOGV printf
#define LOGD printf
#include "cacheops.h"

#ifdef BRCM_V3D_OPT
#include "vcfw/drivers/chip/vciv/2708/v3d_linux.h"
#endif

static struct timeval t1,t2;
static unsigned int logtime=0;
static unsigned int ucount =0;




/******************************************************************************
simpenrose recording
******************************************************************************/

#ifdef SIMPENROSE_RECORD_OUTPUT

static std::vector<MEM_HANDLE_T> record_handles;

void record_map_mem_buffer_section(MEM_HANDLE_T handle, unsigned int begin, unsigned int len, LABEL_T type, unsigned int flags, unsigned int align)
{
   record_map_buffer(khrn_hw_addr((uint8_t *)mem_lock(handle) + begin), len, type, flags, align);
   record_handles.push_back(handle);
}

static void record_unlock_all_mem_buffers(void)
{
   std::vector<MEM_HANDLE_T>::const_iterator i = record_handles.begin(), end = record_handles.end();
   for (; i != end; ++i) {
      mem_unlock(*i);
   }
   record_handles.clear();
}

static uint32_t record_width, record_height, record_bpp;
static bool record_tformat;

void record_set_frame_config(uint32_t width, uint32_t height, uint32_t bpp, bool tformat)
{
   record_width = width;
   record_height = height;
   record_bpp = bpp;
   record_tformat = tformat;
}

static std::string record_filename_base = "simpenrose_record";
static uint32_t record_filename_i = 0;

extern "C" KHAPI void record_set_filename_base(const char *filename_base)
{
   record_filename_base = filename_base;
   record_filename_i = 0;
}

static std::string record_get_next_filename(void)
{
   std::strstream s;
   s << record_filename_base << "_" << record_filename_i++ << ".clif" << std::ends;
   return s.str();
}

#endif

/******************************************************************************
hw fifo
******************************************************************************/

#ifdef SIMPENROSE

#ifdef ANDROID
#ifdef BRCM_V3D_OPT
#define BIN_MEM_SIZE (256 * 1024) /* todo: what's a good value? */
#else
#define BIN_MEM_SIZE (96 * 1024) /* todo: what's a good value? */
#endif
#else
#define BIN_MEM_SIZE (16 * 1024 * 1024)
#endif

bool khrn_hw_init(void)
{
   return true;
}

void khrn_hw_term(void)
{
}

static bool queued = false;
static uint8_t *queued_bin_begin, *queued_bin_end, *queued_render_begin, *queued_render_end;
static uint32_t queued_special_0;
static KHRN_HW_CALLBACK_T queued_callback;
static uint32_t queued_callback_data[8];
static bool queued_fixup_done;

void *khrn_hw_queue(
   uint8_t *bin_begin, uint8_t *bin_end, KHRN_HW_CC_FLAG_T bin_cc,
   uint8_t *render_begin, uint8_t *render_end, KHRN_HW_CC_FLAG_T render_cc, uint32_t render_n,
   uint32_t special_0,
   uint32_t bin_mem_size_min,
   uint32_t actual_user_vpm, uint32_t max_user_vpm,
   KHRN_HW_TYPE_T type,
   KHRN_HW_CALLBACK_T callback,
   uint32_t callback_data_size)
{
   UNUSED(bin_cc);
   UNUSED(render_cc);
   UNUSED(render_n);
   UNUSED(actual_user_vpm);
   UNUSED(max_user_vpm);
   UNUSED(type);

   vcos_assert(!queued); /* in simpenrose mode, we shouldn't have to wait for anything, so khrn_hw_ready should already have been called for the previous frame */
   vcos_assert(BIN_MEM_SIZE >= bin_mem_size_min);
   vcos_assert(callback);
   vcos_assert(callback_data_size <= sizeof(queued_callback_data));

   queued = true;
   queued_bin_begin = bin_begin;
   queued_bin_end = bin_end;
   queued_render_begin = render_begin;
   queued_render_end = render_end;
   queued_special_0 = special_0;
   queued_callback = callback;

   return queued_callback_data;
}

#if defined(KHRN_HW_QPU_PROFILING_DUMP_PRE) || defined(KHRN_HW_QPU_PROFILING_DUMP_POST)
static void qpu_profiling_dump(VC_ADDR_T pc, unsigned int hits, void *)
{
   printf("0x%08x: %u\n", pc, hits);
}
#endif

void khrn_hw_ready(bool ok, void *callback_data) /* if ok is false, callback will later be called with KHRN_HW_CALLBACK_REASON_OUT_OF_MEM_LLAT/KHRN_HW_CALLBACK_REASON_OUT_OF_MEM */
{
   MEM_HANDLE_T bin_mem_handle;
   void *bin_mem;
   uint32_t specials[4];
   void *bin_end, *bin_begin;
   void *render_end, *render_begin;

#ifdef KHRN_HW_QPU_PROFILING_DUMP_PRE
   printf("----\n");
   simpenrose_qpu_profiling_iterate(qpu_profiling_dump, NULL);
#endif
#ifdef KHRN_HW_QPU_PROFILING_RESET_PRE
   simpenrose_qpu_profiling_reset();
#endif

   vcos_assert(queued);
   vcos_assert(callback_data == queued_callback_data);

   queued = false;

   if (!ok || ((bin_mem_handle = mem_alloc_ex(BIN_MEM_SIZE, KHRN_HW_BIN_MEM_ALIGN,
      (MEM_FLAG_T)(MEM_FLAG_DIRECT | MEM_FLAG_ZERO | MEM_FLAG_INIT),
      "khrn_hw_bin_mem", MEM_COMPACT_DISCARD)) == MEM_INVALID_HANDLE)) {
      queued_callback(KHRN_HW_CALLBACK_REASON_OUT_OF_MEM_LLAT, queued_callback_data, NULL);
      queued_callback(KHRN_HW_CALLBACK_REASON_OUT_OF_MEM, queued_callback_data, NULL);
      return;
   }

#ifdef SIMPENROSE_RECORD_OUTPUT
#ifdef SIMPENROSE_RECORD_BINNING
   record_map_mem_buffer(bin_mem_handle, L_BLANK_TILE_ALLOC_MEM, RECORD_BUFFER_IS_BOTH, KHRN_HW_BIN_MEM_ALIGN);
#else
   record_map_mem_buffer(bin_mem_handle, L_TILE_ALLOC_MEM, RECORD_BUFFER_IS_CLIF, KHRN_HW_BIN_MEM_ALIGN);
#endif
#endif
   bin_mem = mem_lock(bin_mem_handle);
   specials[0] = queued_special_0;
   specials[1] = khrn_hw_addr(bin_mem);
   specials[2] = khrn_hw_addr(bin_mem) + BIN_MEM_SIZE;
   specials[3] = BIN_MEM_SIZE;

   bin_begin = queued_bin_begin;
   bin_end = queued_bin_end;
   render_begin = queued_render_begin;
   render_end = queued_render_end;

   queued_fixup_done = false;
   queued_callback(KHRN_HW_CALLBACK_REASON_FIXUP, queued_callback_data, specials);
   vcos_assert(queued_fixup_done); /* in simpenrose mode, we shouldn't have to wait for anything -- khrn_hw_fixup_done should have been called within queued_callback */

#if defined(SIMPENROSE_RECORD_OUTPUT) && defined(SIMPENROSE_RECORD_BINNING)
   record_finish_bin_render(record_get_next_filename().c_str(),
      khrn_hw_addr(bin_begin), khrn_hw_addr(bin_end),
      khrn_hw_addr(render_begin), khrn_hw_addr(render_end),
      record_width, record_height, record_bpp, record_tformat);
   record_unlock_all_mem_buffers();
#endif

   simpenrose_do_binning(khrn_hw_addr(bin_begin), khrn_hw_addr(bin_end));

#if defined(SIMPENROSE_RECORD_OUTPUT) && !defined(SIMPENROSE_RECORD_BINNING)
   record_finish_render(record_get_next_filename().c_str(),
      khrn_hw_addr(render_begin), khrn_hw_addr(render_end),
      record_width, record_height, record_bpp, record_tformat);
   record_unlock_all_mem_buffers();
#endif

   queued_callback(KHRN_HW_CALLBACK_REASON_BIN_FINISHED_LLAT, queued_callback_data, NULL);
   queued_callback(KHRN_HW_CALLBACK_REASON_BIN_FINISHED, queued_callback_data, NULL);

   simpenrose_do_rendering(khrn_hw_addr(render_begin), khrn_hw_addr(render_end));

   mem_unlock(bin_mem_handle);
   mem_release(bin_mem_handle);
   queued_callback(KHRN_HW_CALLBACK_REASON_UNFIXUP, queued_callback_data, NULL);
   queued_callback(KHRN_HW_CALLBACK_REASON_RENDER_FINISHED, queued_callback_data, NULL);

#ifdef KHRN_HW_QPU_PROFILING_DUMP_POST
   printf("----\n");
   simpenrose_qpu_profiling_iterate(qpu_profiling_dump, NULL);
#endif
#ifdef KHRN_HW_QPU_PROFILING_RESET_POST
   simpenrose_qpu_profiling_reset();
#endif
}

void khrn_hw_fixup_done(bool in_callback, void *callback_data)
{
   vcos_assert(in_callback);
   vcos_assert(callback_data == queued_callback_data);
   vcos_assert(!queued_fixup_done);
   queued_fixup_done = true;
}

void khrn_hw_queue_wait_for_worker(uint64_t pos)
{
   while (khrn_worker_get_exit_pos() < pos) {
      khrn_sync_master_wait();
   }
}

void khrn_hw_queue_wait_for_display(EGL_DISP_HANDLE_T disp_handle, uint32_t pos)
{
   while (egl_disp_still_on(disp_handle, pos)) {
      khrn_sync_master_wait();
   }
}

void khrn_hw_queue_display(EGL_DISP_SLOT_HANDLE_T slot_handle)
{
   /* display now -- nothing to wait for */
   egl_disp_ready(slot_handle, false);
   khrn_llat_process(); /* this will probably happen eventually?, but we want it to happen now */
}

void khrn_hw_wait(void)
{
   /* nothing to wait for */
}

void khrn_hw_update_perf_counters(KHRN_DRIVER_COUNTERS_T *counters)
{
   UNUSED(counters);
}

void khrn_hw_register_perf_counters(KHRN_DRIVER_COUNTERS_T *counters)
{
   UNUSED(counters);
}

void khrn_hw_unregister_perf_counters(void)
{
}

#else

/* todo: waits in the fifo can mean the hw is idle even if there's something in
 * the fifo that could be run. does this matter in practice? */

#define BIN_MEM_SIZE (96 * 1024) /* todo: what's a good value? */
/* todo: these assumptions aren't very nice. we use the top of the initial bin
 * mem for tile states and the bottom for initial tile control lists. we can't
 * trash the tile states because the ptb will go horribly wrong if we do */
#define BIN_MEM_TRASHABLE_SIZE ((BIN_MEM_SIZE * KHRN_HW_CL_BLOCK_SIZE_MIN) / (KHRN_HW_CL_BLOCK_SIZE_MIN + KHRN_HW_TILE_STATE_SIZE))
vcos_static_assert(BIN_MEM_TRASHABLE_SIZE >= KHRN_HW_BIN_MEM_GRANULARITY); /* use as overflow memory */
#ifdef __BCM2708A0__
vcos_static_assert(BIN_MEM_TRASHABLE_SIZE >= (KHRN_HW_TILE_HEIGHT * KHRN_HW_TILE_WIDTH * 4)); /* on a0, use as frame in null render list */
#endif

/* we'll try not to allocate many more bin mems than this */
#define BIN_MEMS_N_MAX 64 /* todo: what should this be? */

#define MSGS_N 16 /* todo: what should this be? */

typedef enum {
   MSG_DISPLAY_STATE_NOT_DONE,
   MSG_DISPLAY_STATE_DOING,
   MSG_DISPLAY_STATE_DONE_NONE,
   MSG_DISPLAY_STATE_DONE
} MSG_DISPLAY_STATE_T;

typedef struct {
   /* wait for (khrn_worker_get_exit_pos() >= wait_worker_pos) before submitting render */
   uint64_t wait_worker_pos;

   /* wait for !egl_disp_still_on(wait_display_disp_handle, wait_display_pos) before submitting render */
   EGL_DISP_HANDLE_T wait_display_disp_handle; /* EGL_DISP_HANDLE_INVALID means don't wait */
   uint32_t wait_display_pos;

   /* if callback is non-NULL, all of this is valid. otherwise, all of this is
    * invalid */
   bool ready; /* wait for ready before preparing bin or render */
   bool ok; /* this is set to false when we run out of memory. this can happen in khrn_hw_ready, prepare_bin, or supply_overflow */
   bool ok_prepare; /* were we ok in prepare_bin? (if so, we locked the cmem and fixer stuff...) */
   uint8_t *bin_begin, *bin_end, *render_begin, *render_end;
   KHRN_HW_CC_FLAG_T bin_cc, render_cc;
   uint32_t render_n; /* isr will update */
   uint32_t special_0;
   uint32_t bin_mem_size_min; /* initial bin mem (available through fixer) must be at least this big */
   uint32_t actual_user_vpm;
   uint32_t max_user_vpm;
   KHRN_HW_TYPE_T type;
   KHRN_HW_CALLBACK_T callback;
   uint32_t callback_data[8];
   MEM_HANDLE_T bin_mems_head; /* tail is the initial bin mem (possibly not allocated from the pool). use set_bin_mems_next/get_bin_mems_next to manipulate ll */
   void *bin_mem; /* the initial bin mem */
   bool bin_mems_one_too_many; /* the hw didn't actually use the last (head) bin mem (so we free it in cleanup_bin_llat instead of cleanup_render_llat) */

   EGL_DISP_SLOT_HANDLE_T display_slot_handle; /* EGL_DISP_SLOT_HANDLE_INVALID means display nothing */
   MSG_DISPLAY_STATE_T display_state;
#ifdef KHRN_STATS_ENABLE
   uint32_t stats_bin_time;
   uint32_t stats_render_time;
#endif
} MSG_T;

#ifdef BRCM_V3D_OPT
typedef struct _MSG_LIST_T_{
	MSG_T post;
	bool last;
	struct _MSG_LIST_T_* next;
} MSG_LIST_T;

MSG_LIST_T* msg_list_android = NULL;
#endif

static MEM_HANDLE_T null_render_handle;

static KHRN_POOL_T bin_mem_pool; /* only used by the llat thread (so no need for locks etc) */
static uint32_t bin_mems_n = 0; /* how many bin mems we currently have allocated from the pool */

static MSG_T msgs[MSGS_N];

static struct {
   MSG_T
      /* last message with wait info filled + 1 (<= (post + 1)) (updated by master task) */
      *wait,
      /* last message posted + 1 (<= wait) (updated by master task, read by llat task) */
      *post,
      /* last message locked in preparation for binning + 1 (<= post) (updated by llat task) */
      *bin_prepared,
      /* last message locked in preparation for rendering + 1 (<= bin_prepared) (updated by llat task) */
      *render_prepared,
      /* last message submitted to binner + 1 (<= bin_prepared, <= (bin_done + 1)) (updated by llat task) */
      *bin_submitted,
      /* last message submitted to renderer + 1 (<= render_prepared, <= (render_done + 1), <= bin_submitted) (updated by llat task) */
      *render_submitted,
      /* last message completed binning + 1 (<= bin_submitted) (updated by isr, read by llat task) */
      *bin_done,
      /* last message completed rendering + 1 (<= render_submitted, <= bin_done) (updated by isr, read by llat task) */
      *render_done,
      /* last message cleaned up on llat task after binning + 1 (<= bin_done) (updated by llat task, read by master task) */
      *bin_cleanup_llat,
      /* last message cleaned up after binning + 1 (<= bin_cleanup_llat) (updated by master task) */
      *bin_cleanup,
      /* last message cleaned up on llat task after rendering + 1 (<= render_done, <= bin_cleanup_llat) (updated by llat task, read by master task) */
      *render_cleanup_llat,
      /* last message cleaned up after rendering + 1 (<= render_cleanup_llat, <= bin_cleanup) (updated by master task) */
      *render_cleanup,
      /* last message submitted to display + 1 (<= render_done) (updated by llat task, read by master task) */
      *display,
      /* last message completed + 1 (<= render_cleanup, <= display) (updated by master task) */
      *done;
} khrn_prod_msg;

/* fifo pos stuff for interlocking with other fifos... */
uint64_t khrn_hw_enter_pos;
uint32_t khrn_hw_bin_exit_pos_0, khrn_hw_bin_exit_pos_1;
uint32_t khrn_hw_render_exit_pos_0, khrn_hw_render_exit_pos_1;

static bool bin_prepared = false, render_prepared = false, fixup_done;
static void *bin_begin, *bin_end, *render_begin, *render_end;

static uint32_t bins_submitted, renders_submitted; /* used by llat task */
static uint32_t bins_done, renders_done; /* used by isr */

static bool want_overflow, cancel_want_overflow;

#ifdef KHRN_HW_MEASURE_BIN_TIME
static uint32_t measure_bin_start = 0;
static uint32_t measure_bin_end = 0;
#endif
#ifdef KHRN_HW_MEASURE_RENDER_TIME
static uint32_t measure_render_start = 0;
static uint32_t measure_render_end = 0;
#endif

#ifdef KHRN_HW_BIN_HISTORY_N
uint32_t khrn_hw_bin_history[KHRN_HW_BIN_HISTORY_N] = {0};
uint32_t khrn_hw_bin_history_n = KHRN_HW_BIN_HISTORY_N;
static uint32_t bin_history_i = 0;
#endif
#ifdef KHRN_HW_RENDER_HISTORY_N
uint32_t khrn_hw_render_history[KHRN_HW_RENDER_HISTORY_N] = {0};
uint32_t khrn_hw_render_history_n = KHRN_HW_RENDER_HISTORY_N;
static uint32_t render_history_i = 0;
#endif

static DRIVER_HANDLE_T v3d_driver_handle;

static uint32_t llat_i;

static KHRN_DRIVER_COUNTERS_T *s_perf_counters = 0;

static void reset_perf_counters(void);
static void update_perf_counters(void);
#ifdef BRCM_V3D_OPT
static void prepare_bin(MSG_T* bin_prepared);
#endif

static pthread_mutex_t khrn_hw_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

static MEM_HANDLE_T bin_mem_pool_alloc(bool fail_ok)
{
   MEM_HANDLE_T handle = khrn_pool_alloc(&bin_mem_pool, fail_ok);
   if (handle != MEM_INVALID_HANDLE) { ++bin_mems_n; }
   return handle;
}

static void bin_mem_pool_free(MEM_HANDLE_T handle)
{
   --bin_mems_n;
   khrn_pool_free(&bin_mem_pool, handle);
}

static INLINE void set_bin_mems_next(MEM_HANDLE_T handle, MEM_HANDLE_T next_handle)
{
   mem_set_term(handle, (MEM_TERM_T)(uintptr_t)next_handle);
}

static INLINE MEM_HANDLE_T get_bin_mems_next(MEM_HANDLE_T handle)
{
   return (MEM_HANDLE_T)(uintptr_t)mem_get_term(handle);
}

static INLINE MSG_T *prev_msg(MSG_T *msg)
{
   return (msg == msgs) ? (msgs + (MSGS_N - 1)) : (msg - 1);
}

static INLINE MSG_T *next_msg(MSG_T *msg)
{
   return (msg == (msgs + (MSGS_N - 1))) ? msgs : (msg + 1);
}

static INLINE void advance_msg(MSG_T **msg) /* *msg is read by another task */
{
   khrn_barrier();
   *msg = next_msg(*msg);
}

static INLINE bool more_msgs(MSG_T *msg_a, MSG_T *msg_b) /* msg_b is updated by another task */
{
   if (msg_a != msg_b) {
      khrn_barrier();
      return true;
   }
   return false;
}

static INLINE bool processed_msgs(MSG_T *msg_a, MSG_T *msg_b) /* msg_a is updated by another task */
{
   if (msg_a == msg_b) {
      khrn_barrier();
      return true;
   }
   return false;
}

static void acquire_callback(void *p);
static void khrn_hw_isr(uint32_t flags);
static void khrn_hw_llat_callback(void);
#ifdef BRCM_V3D_OPT
FILE *fd_v3d = 0;
static MEM_HANDLE_T g_bin_mems_head = 0;
#endif

static char *mapping[116];
void build_mapping(void)
{
	int count;
	for (count = 0; count < 116; count++)
		mapping[count] = "reserved!";

	mapping[0] = "KHRN_HW_INSTR_HALT";
	mapping[1] = "KHRN_HW_INSTR_NOP";
	mapping[2] = "KHRN_HW_INSTR_MARKER";
	mapping[3] = "KHRN_HW_INSTR_RESET_MARKER_COUNT";
	mapping[4] = "KHRN_HW_INSTR_FLUSH";
	mapping[5] = "KHRN_HW_INSTR_FLUSH_ALL_STATE";
	mapping[6] = "KHRN_HW_INSTR_START_TILE_BINNING";
	mapping[7] = "KHRN_HW_INSTR_INCR_SEMAPHORE";
	mapping[8] = "KHRN_HW_INSTR_WAIT_SEMAPHORE";
	mapping[16] = "KHRN_HW_INSTR_BRANCH";
	mapping[17] = "KHRN_HW_INSTR_BRANCH_SUB";
	mapping[18] = "KHRN_HW_INSTR_RETURN";
	mapping[19] = "KHRN_HW_INSTR_REPEAT_START_MARKER";
	mapping[20] = "KHRN_HW_INSTR_REPEAT_FROM_START_MARKER";
	mapping[24] = "KHRN_HW_INSTR_STORE_SUBSAMPLE";
	mapping[25] = "KHRN_HW_INSTR_STORE_SUBSAMPLE_EOF";
	mapping[26] = "KHRN_HW_INSTR_STORE_FULL";
	mapping[27] = "KHRN_HW_INSTR_LOAD_FULL";
	mapping[28] = "KHRN_HW_INSTR_STORE_GENERAL";
	mapping[29] = "KHRN_HW_INSTR_LOAD_GENERAL";
	mapping[32] = "KHRN_HW_INSTR_GLDRAWELEMENTS";
	mapping[33] = "KHRN_HW_INSTR_GLDRAWARRAYS";
	mapping[41] =  "KHRN_HW_INSTR_VG_COORD_LIST";
	mapping[48] = "KHRN_HW_INSTR_COMPRESSED_LIST";
	mapping[49] = "KHRN_HW_INSTR_CLIPPED_PRIM";
	mapping[56] = "KHRN_HW_INSTR_PRIMITIVE_LIST_FORMAT";
	mapping[64] = "KHRN_HW_INSTR_GL_SHADER";
	mapping[65] = "KHRN_HW_INSTR_NV_SHADER";
	mapping[66] = "KHRN_HW_INSTR_VG_SHADER";
	mapping[65] = "KHRN_HW_INSTR_INLINE_VG_SHADER";
	mapping[96] = "KHRN_HW_INSTR_STATE_CFG";
	mapping[97] = "KHRN_HW_INSTR_STATE_FLATSHADE";
	mapping[98] = "KHRN_HW_INSTR_STATE_POINT_SIZE";
	mapping[99] = "KHRN_HW_INSTR_STATE_LINE_WIDTH";
	mapping[100] = "KHRN_HW_INSTR_STATE_RHTX";
	mapping[101] = "KHRN_HW_INSTR_STATE_DEPTH_OFFSET";
	mapping[102] = "KHRN_HW_INSTR_STATE_CLIP";
	mapping[103] = "KHRN_HW_INSTR_STATE_VIEWPORT_OFFSET";
	mapping[104] = "KHRN_HW_INSTR_STATE_CLIPZ";
	mapping[105] = "KHRN_HW_INSTR_STATE_CLIPPER_XY";
	mapping[106] = "KHRN_HW_INSTR_STATE_CLIPPER_Z";
	mapping[112] = "KHRN_HW_INSTR_STATE_TILE_BINNING_MODE";
	mapping[113] = "KHRN_HW_INSTR_STATE_TILE_RENDERING_MODE";
	mapping[114] = "KHRN_HW_INSTR_STATE_CLEARCOL";
	mapping[115] = "KHRN_HW_INSTR_STATE_TILE_COORDS";
}

char *get_mapping(int n)
{
	return mapping[n];
}

bool khrn_hw_init(void)
{
   uint8_t *begin, *p;


#ifdef __BCM2708A0__
   #define NULL_RENDER_SIZE 16
#else
   #define NULL_RENDER_SIZE 22
#endif

   pthread_mutex_init(&khrn_hw_queue_mutex,NULL);

   null_render_handle = mem_alloc_ex(NULL_RENDER_SIZE, 1, (MEM_FLAG_T)(MEM_FLAG_DIRECT | MEM_FLAG_NO_INIT), "khrn_hw_null_render", MEM_COMPACT_DISCARD);
   if (null_render_handle == MEM_INVALID_HANDLE) {
      return false;
   }
   begin = (uint8_t *)mem_lock(null_render_handle);
   p = begin;
   Add_byte(&p, KHRN_HW_INSTR_STATE_TILE_RENDERING_MODE);
#define NULL_RENDER_FRAME_OFFSET 1
   vcos_assert((begin + NULL_RENDER_FRAME_OFFSET) == p);
   ADD_WORD(p, 0); /* frame */
   ADD_SHORT(p, KHRN_HW_TILE_WIDTH);
   ADD_SHORT(p, KHRN_HW_TILE_HEIGHT);
   add_byte(&p, (uint8_t)(
      (0 << 0) | /* disable ms mode */
      (0 << 1) | /* disable 64-bit color mode */
      (1 << 2) | /* 32-bit rso */
      (0 << 4))); /* 1x decimate (ie none) */
   add_byte(&p, 0); /* unused */
   Add_byte(&p, KHRN_HW_INSTR_WAIT_SEMAPHORE);
#define NULL_RENDER_POST_WAIT_SEMAPHORE_OFFSET 12
   vcos_assert((begin + NULL_RENDER_POST_WAIT_SEMAPHORE_OFFSET) == p);
   Add_byte(&p, KHRN_HW_INSTR_STATE_TILE_COORDS);
   add_byte(&p, 0);
   add_byte(&p, 0);
#ifdef __BCM2708A0__
   Add_byte(&p, KHRN_HW_INSTR_STORE_SUBSAMPLE_EOF);
#else
   Add_byte(&p, KHRN_HW_INSTR_STORE_GENERAL);
   add_byte(&p, 0); /* no buffer */
   add_byte(&p, 0xe0); /* don't clear any buffers */
   ADD_WORD(p, 1 << 3); /* eof */
#endif
   vcos_assert((begin + NULL_RENDER_SIZE) == p);
   mem_unlock(null_render_handle);

   khrn_pool_init(&bin_mem_pool,
      BIN_MEM_SIZE, KHRN_HW_BIN_MEM_ALIGN, BIN_MEMS_N_MAX,
      MEM_FLAG_DIRECT, "khrn_hw_bin_mem");

   khrn_prod_msg.wait = msgs;
   khrn_prod_msg.post = msgs;
   khrn_prod_msg.bin_prepared = msgs;
   khrn_prod_msg.render_prepared = msgs;
   khrn_prod_msg.bin_submitted = msgs;
   khrn_prod_msg.render_submitted = msgs;
   khrn_prod_msg.bin_done = msgs;
   khrn_prod_msg.render_done = msgs;
   khrn_prod_msg.bin_cleanup_llat = msgs;
   khrn_prod_msg.bin_cleanup = msgs;
   khrn_prod_msg.render_cleanup_llat = msgs;
   khrn_prod_msg.render_cleanup = msgs;
   khrn_prod_msg.display = msgs;
   khrn_prod_msg.done = msgs;

   prev_msg(khrn_prod_msg.post)->display_slot_handle = (EGL_DISP_SLOT_HANDLE_T)(EGL_DISP_SLOT_HANDLE_INVALID + 1); /* for khrn_hw_queue_display */

   khrn_hw_enter_pos = 0;
   khrn_hw_bin_exit_pos_0 = 0;
   khrn_hw_bin_exit_pos_1 = 0;
   khrn_hw_render_exit_pos_0 = 0;
   khrn_hw_render_exit_pos_1 = 0;

#ifdef BRCM_V3D_OPT
#ifdef __ARMEL__
   if (!fd_v3d)
   {
		fd_v3d = fopen("/dev/dmaer_4k", "r+b");
		if(!fd_v3d) {
			LOGE("Could not open v3d device from khrn_prod_4");
			return false;
		}
   }
#endif
	g_bin_mems_head =  mem_alloc_ex(BIN_MEM_SIZE, KHRN_HW_BIN_MEM_ALIGN, (MEM_FLAG_T)(MEM_FLAG_DIRECT | MEM_FLAG_NO_INIT | MEM_FLAG_RETAINED), "khrn_hw_bin_mem (too cool for pool)", MEM_COMPACT_NONE);
#else
   {
      V3D_PARAMS_T params = {acquire_callback, khrn_hw_isr, NULL};
      verify(v3d_get_func_table()->open(&params, &v3d_driver_handle) == 0);
   }

   llat_i = khrn_llat_register(khrn_hw_llat_callback);
   vcos_assert(llat_i != (uint32_t)-1);
#endif

   return true;
}

void khrn_hw_term(void)
{
   vcos_assert(khrn_prod_msg.done == khrn_prod_msg.wait); /* should have called khrn_hw_wait or equivalent before this */

#ifdef BRCM_V3D_OPT
	if(g_bin_mems_head) mem_release(g_bin_mems_head);
	fclose(fd_v3d);
#else
   verify(v3d_get_func_table()->close(v3d_driver_handle) == 0);

   khrn_llat_unregister(llat_i);
#endif

   khrn_pool_term(&bin_mem_pool);

   mem_release(null_render_handle);
}

/*
   stuff called from the master task
*/

static void msg_init_wait(MSG_T *msg)
{
   msg->wait_worker_pos = 0;
   msg->wait_display_disp_handle = EGL_DISP_HANDLE_INVALID;
}

static void msg_init_nop(MSG_T *msg)
{
   msg->callback = NULL;

   msg->display_slot_handle = EGL_DISP_SLOT_HANDLE_INVALID;
   msg->display_state = MSG_DISPLAY_STATE_NOT_DONE;
}

#ifdef BRCM_V3D_OPT
static int job_id=16;

// #define DEBUG_DUMP_CL
#ifdef DEBUG_DUMP_CL
//#define FILE_DUMP

#define DBG_MAX_NUM_LISTS	(100)
#define DBG_MAX_LIST_SIZE	(4*1024)
#define DBG_DATA_SIZE		(DBG_MAX_NUM_LISTS * DBG_MAX_LIST_SIZE)

typedef struct dbg_list_hdr_t_ {
	int valid;
	int bin_start;
	int bin_size;
	int rend_start;
	int rend_size;
	v3d_job_post_t		job_post;
	v3d_job_status_e 	status;
} dbg_list_hdr_t;

static dbg_list_hdr_t dbg_list_hdr [DBG_MAX_NUM_LISTS];
static unsigned char dbg_data[DBG_DATA_SIZE];
static int dbg_list_idx = 0;
static int dbg_list_init_done = 0;

static void dbg_list_init (void)
{
	int i;

	for (i=0; i<DBG_MAX_NUM_LISTS; i++) {
		dbg_list_hdr[i].valid = 0;
		memset(&dbg_list_hdr[i].job_post, 0, sizeof(v3d_job_post_t));
	}

	memset(dbg_data, 0, DBG_DATA_SIZE);

	dbg_list_init_done = 1;
}

static int dbg_list_get_next_idx(int idx)
{
	int new_idx = idx+1;
	if (new_idx >= DBG_MAX_NUM_LISTS) new_idx = 0;
	return new_idx;
}

static int dbg_list_get_prev_idx(int idx)
{
	int new_idx = idx-1;
	if (new_idx < 0) new_idx = DBG_MAX_NUM_LISTS-1;
	return new_idx;
}

static int dbg_list_get_slot (v3d_job_post_t *p_job_post)
{
	int size;

	size = (p_job_post->v3d_ct0ea - p_job_post->v3d_ct0ca);
	size += (p_job_post->v3d_ct1ea - p_job_post->v3d_ct1ca);
	if (size > DBG_MAX_LIST_SIZE) {
		LOGE("Skipping storing the CL for job[%d] size[%d]", p_job_post->job_id, size);
		return -1;
	}

	dbg_list_hdr[dbg_list_idx].valid = 0;
	return (dbg_list_idx * DBG_MAX_LIST_SIZE);
}

static void dbg_list_store_job (v3d_job_post_t *p_job_post, int start, unsigned char *bin_addr, unsigned char *rend_addr)
{
	int idx, bin_size, rend_size;

	idx = dbg_list_idx;

	bin_size = (p_job_post->v3d_ct0ea - p_job_post->v3d_ct0ca);
	rend_size = (p_job_post->v3d_ct1ea - p_job_post->v3d_ct1ca);
	dbg_list_hdr[idx].valid = 1;
	dbg_list_hdr[idx].bin_start = start;
	dbg_list_hdr[idx].bin_size = bin_size;
	dbg_list_hdr[idx].rend_start = (start+bin_size);
	dbg_list_hdr[idx].rend_size = rend_size;
//	p_job_post->reserved[0] = bin_addr;
//	p_job_post->reserved[1] = rend_addr;
	memcpy(&dbg_list_hdr[idx].job_post, p_job_post, sizeof(v3d_job_post_t));
	memcpy(&dbg_data[start], bin_addr, bin_size);
	memcpy(&dbg_data[start+bin_size], rend_addr, rend_size);
}

static int dbg_is_dump_needed(void) {
	if (dbg_list_idx == 0) {
		return 1;
	} else {
		return 0;
	}
}

static int dbg_list_dump_all (void)
{
	static int dbg_file_idx = 0;
	static int dbg_last_job_written = 0;
	int idx, i;
	char fname[64];
	int wr_sz, n, l_job_id, cnt;
	unsigned char *addr;
	int dbg_fd;

#ifdef 	FILE_DUMP
	snprintf(fname,64,"/data/dbg_dump/dbg_list_%d.cl",dbg_file_idx);
	dbg_fd = open(fname, (O_CREAT | O_WRONLY), 0666);
	if (dbg_fd < 0) {
		LOGE( "[%d]Failed to create [%s]", dbg_fd, fname);
		return -1;
	}
#endif

	idx = dbg_list_idx;
	l_job_id = -1;
	cnt = 0;
	for (i=0; i<DBG_MAX_NUM_LISTS; i++) {
		if (dbg_list_hdr[idx].valid) {
			if (dbg_list_hdr[idx].job_post.job_id > dbg_last_job_written) {
#if 1
				LOGD("i[%d] idx[%d] v[%d]: id[%d] b[%d] bs[%d] r[%d] rs[%d]",
					i, idx, dbg_list_hdr[idx].valid, dbg_list_hdr[idx].job_post.job_id,
					dbg_list_hdr[idx].bin_start, dbg_list_hdr[idx].bin_size,
					dbg_list_hdr[idx].rend_start, dbg_list_hdr[idx].rend_size);
				LOGD("[%p] [%p] [%p] [%p]",
					dbg_list_hdr[idx].job_post.v3d_ct0ca, dbg_list_hdr[idx].job_post.v3d_ct0ea,
					dbg_list_hdr[idx].job_post.v3d_ct1ca, dbg_list_hdr[idx].job_post.v3d_ct1ea);
#endif
#ifdef 	FILE_DUMP
				if (dbg_fd >= 0) {
					addr = &dbg_list_hdr[idx];
					wr_sz = sizeof(dbg_list_hdr_t);
					n = write(dbg_fd, addr, wr_sz);
					LOGE_IF((wr_sz != n), "Failed to write [%d]bytes hdr from [0x%08x] to [%s]", wr_sz, addr, fname);

					addr = &dbg_data[dbg_list_hdr[idx].bin_start];
					wr_sz = dbg_list_hdr[idx].bin_size + dbg_list_hdr[idx].rend_size;
					n = write(dbg_fd, addr, wr_sz);
					LOGE_IF((wr_sz != n), "Failed to write [%d]bytes data from [0x%08x] to [%s]", wr_sz, addr, fname);
				}
#endif

				if (l_job_id == -1) {
					l_job_id = dbg_list_hdr[idx].job_post.job_id;
				}
				cnt++;
				dbg_last_job_written = dbg_list_hdr[idx].job_post.job_id;
			}
		}
		idx = dbg_list_get_next_idx(idx);
	}

#ifdef 	FILE_DUMP
	if (dbg_fd >= 0) {
		close(dbg_fd);
	}
	if (l_job_id != -1) {
		LOGD("Dump [%d]jobs from job id[%d] to file[%s]", cnt, l_job_id, fname);
	}
	dbg_file_idx++;
#endif

	return cnt;
}

static int dbg_list (v3d_job_post_t *p_job_post, unsigned char *bin_addr, unsigned char *rend_addr)
{
	int start;

	if (dbg_list_init_done == 0) {
		dbg_list_init();
	}

	start = dbg_list_get_slot(p_job_post);
	if (start < 0) {
		return -1;
	}

	dbg_list_store_job(p_job_post, start, bin_addr, rend_addr);
	dbg_list_idx = dbg_list_get_next_idx(dbg_list_idx);
	return 0;
}
#endif /* DEBUG_DUMP_CL */

#endif /* BRCM_V3D_OPT */

unsigned int _bin_start, _bin_end;

#define MAX_IMAGES 3

unsigned int used_images = 0;
unsigned char *images[MAX_IMAGES];
unsigned int types[MAX_IMAGES];
unsigned int widths[MAX_IMAGES];
unsigned int heights[MAX_IMAGES];
unsigned int pitches[MAX_IMAGES];

void update_dispmanx_image(void *p, VC_IMAGE_TYPE_T type, int width, int height, int pitch);
//void update_dispmanx_image(void *p);

extern unsigned int overflowPa;
extern unsigned int overflowSize;

#ifdef BRCM_V3D_OPT
void *khrn_hw_queue(
   uint8_t *bin_begin, uint8_t *bin_end, KHRN_HW_CC_FLAG_T bin_cc,
   uint8_t *render_begin, uint8_t *render_end, KHRN_HW_CC_FLAG_T render_cc, uint32_t render_n,
   uint32_t special_0, /* used for extra thrsw removing hack. todo: no need for this hack on b0 */
   uint32_t bin_mem_size_min,
   uint32_t actual_user_vpm, uint32_t max_user_vpm,
   KHRN_HW_TYPE_T type,
   KHRN_HW_CALLBACK_T callback,
	KHRN_HW_CALLBACK_T data_callback,
	void* fmem,
   uint32_t callback_data_size)
{
   void *callback_data;
   UNUSED_NDEBUG(callback_data_size);

   vcos_assert(callback);

   v3d_job_post_t job_post;
   v3d_job_status_t job_status;
   MSG_LIST_T* item = malloc(sizeof(MSG_LIST_T));
   memset(item,0,sizeof(MSG_LIST_T));

   item->post.ready = true;
   item->post.ok = true;
   item->post.ok_prepare= true;
   /* fill in msg_post->ok in khrn_hw_ready */
   /* fill in msg_post->ok_prepare in prepare_bin */
   item->post.bin_begin = bin_begin;
   item->post.bin_end = bin_end;
   item->post.bin_cc = bin_cc;
   item->post.render_begin = render_begin;
   item->post.render_end = render_end;
   item->post.render_cc = render_cc;
   item->post.render_n = render_n;
   item->post.special_0 = special_0;
   item->post.bin_mem_size_min = bin_mem_size_min
      /* the hw rounds up after the initial control lists to the regular alloc
       * size. in the worst case we need this many bytes extra just to leave
       * effectively no memory for the regular control list blocks (if we leave
       * negative memory, things go badly wrong) */
      + (KHRN_HW_CL_BLOCK_SIZE_MAX - KHRN_HW_CL_BLOCK_SIZE_MIN)
      /* plus 2 blocks for the binner to work with until we give it some
       * overflow memory. todo: really? */
      + (2 * KHRN_HW_BIN_MEM_GRANULARITY);
   item->post.actual_user_vpm = actual_user_vpm;
   item->post.max_user_vpm = max_user_vpm;
   item->post.type = type;
   item->post.callback = callback;
   callback_data = item->post.callback_data;
   ((uint32_t*)callback_data)[6] = (uint32_t)fmem;
   ((uint32_t*)callback_data)[7] = (uint32_t)data_callback;
   vcos_assert(callback_data_size <= sizeof(item->post.callback_data));
   /* fill in khrn_prod_msg.post->bin_mems_head/one_too_many and khrn_prod_msg.post->bin_mem in prepare_bin */
   item->post.display_slot_handle = EGL_DISP_SLOT_HANDLE_INVALID;
   item->post.display_state = MSG_DISPLAY_STATE_NOT_DONE;
   /* don't khrn_hw_notify_llat() yet -- do that when it's ready */
   prepare_bin(&item->post);
	   if(cacheops('F',0x0,0x0,0x20000)<0)
	   {
		   LOGE("Error in flushing cache\n");
		   return NULL;
	   }

   pthread_mutex_lock(&khrn_hw_queue_mutex);
   job_post.job_type = V3D_JOB_BIN_REND;
   job_post.job_id = job_id++;
   job_post.v3d_ct0ca = khrn_hw_addr(bin_begin);
   job_post.v3d_ct0ea = khrn_hw_addr(bin_end);
   job_post.v3d_ct1ca = khrn_hw_addr(render_begin);
   job_post.v3d_ct1ea = khrn_hw_addr(render_end);

   //sjh
   job_post.m_pOverspill = (void *)overflowPa;
   job_post.m_overspillSize = overflowSize;

   job_status.job_status = V3D_JOB_STATUS_INVALID;
   job_status.job_id = job_post.job_id;

#ifdef MEGA_DEBUG
   printf("POST of %p %p %p %p\n", bin_begin, bin_end,
		   render_begin, render_end);
   printf("POST of %p %p %p %p\n", job_post.v3d_ct0ca, job_post.v3d_ct0ea,
		   job_post.v3d_ct1ca, job_post.v3d_ct1ea);
#endif

   if(msg_list_android == NULL)
   	{
   	msg_list_android = item;
   	}
   else
   	{
   	MSG_LIST_T* temp = msg_list_android;
   	do{
		if(temp->next == NULL)
			{
			temp->next = item;
			break;
			}
		temp = temp->next;
   		}while(temp);
   	}
#ifdef DEBUG_DUMP_CL
	dbg_list (&job_post, bin_begin, render_begin);
	if (dbg_is_dump_needed()) {
		dbg_list_dump_all();
	}
#endif
#if 0
	 LOGE("Job Post id[%d] tid[%d] CT[0x%08x 0x%08x 0x%08x 0x%08x] ",
		job_post.job_id, gettid(), khrn_hw_addr(bin_begin),khrn_hw_addr(bin_end),
		khrn_hw_addr(render_begin),khrn_hw_addr(render_end));
#endif

   if (fd_v3d && ioctl(fileno(fd_v3d), V3D_IOCTL_POST_JOB, &job_post) < 0) {
	   LOGE("ioctl [0x%x] failed \n", V3D_IOCTL_POST_JOB);
   }
#if 0
   if (ioctl(fileno(fd_v3d), V3D_IOCTL_WAIT_JOB, &job_status) < 0) {
		  LOGE("ioctl [0x%x] failed \n", V3D_IOCTL_WAIT_JOB);
	  }
   LOGE_IF((job_status.job_status != V3D_JOB_STATUS_SUCCESS), "job id[%d] status[%d] \n", job_status.job_id, job_status.job_status);
#endif

   pthread_mutex_unlock(&khrn_hw_queue_mutex);

  /* {
	   FILE *fp = fopen("/home/simon/bins", "wb");
	   assert(fp);

	   fwrite((void*)_bin_start, _bin_end - _bin_start, 1, fp);
	   fclose(fp);
   }*/

   {
	   static int bin = 0;
	   update_dispmanx_image(images[bin], types[bin], widths[bin], heights[bin], pitches[bin]);
//	   update_dispmanx_image(images[bin]);

	   bin++;
	   if (bin == used_images)
		   bin = 0;
   }

   khrn_hw_advance_render_exit_pos();

   /*{
	   int dimx = 800;
	   int dimy = 480;
	   int i, j;

   	   FILE *fp = fopen("/home/simon/buff0", "wb");
   	   assert(fp);

   	   fprintf(fp, "P6\n%d %d\n255\n", dimx, dimy);

		for (j = 0; j < dimy; ++j)
		  {
			for (i = 0; i < dimx; ++i)
			{
				unsigned char *p = images[0];
			  static unsigned char color[3];
			  color[0] = p[j * (800 * 4) + i * 4];
			  color[1] = p[j * (800 * 4) + i * 4 + 1];
			  color[2] = p[j * (800 * 4) + i * 4 + 2];
			  (void) fwrite(color, 1, 3, fp);
			}
		  }
		  (void) fclose(fp);
   }*/

   return callback_data;
}

#else  /* BRCM_V3D_OPT */

void *khrn_hw_queue(
   uint8_t *bin_begin, uint8_t *bin_end, KHRN_HW_CC_FLAG_T bin_cc,
   uint8_t *render_begin, uint8_t *render_end, KHRN_HW_CC_FLAG_T render_cc, uint32_t render_n,
   uint32_t special_0, /* used for extra thrsw removing hack. todo: no need for this hack on b0 */
   uint32_t bin_mem_size_min,
   uint32_t actual_user_vpm, uint32_t max_user_vpm,
   KHRN_HW_TYPE_T type,
   KHRN_HW_CALLBACK_T callback,
   uint32_t callback_data_size)
{
   void *callback_data;
   UNUSED_NDEBUG(callback_data_size);

   vcos_assert(callback);

   pthread_mutex_lock(&khrn_hw_queue_mutex);
   if (khrn_prod_msg.post == khrn_prod_msg.wait) {
      while (next_msg(khrn_prod_msg.wait) == khrn_prod_msg.done) {
         khrn_sync_master_wait();
      }
      msg_init_wait(khrn_prod_msg.wait);
      khrn_prod_msg.wait = next_msg(khrn_prod_msg.wait);
   }

   khrn_prod_msg.post->ready = false;
   /* fill in msg_post->ok in khrn_hw_ready */
   /* fill in msg_post->ok_prepare in prepare_bin */
   khrn_prod_msg.post->bin_begin = bin_begin;
   khrn_prod_msg.post->bin_end = bin_end;
   khrn_prod_msg.post->bin_cc = bin_cc;
   khrn_prod_msg.post->render_begin = render_begin;
   khrn_prod_msg.post->render_end = render_end;
   khrn_prod_msg.post->render_cc = render_cc;
   khrn_prod_msg.post->render_n = render_n;
   khrn_prod_msg.post->special_0 = special_0;
   khrn_prod_msg.post->bin_mem_size_min = bin_mem_size_min
      /* the hw rounds up after the initial control lists to the regular alloc
       * size. in the worst case we need this many bytes extra just to leave
       * effectively no memory for the regular control list blocks (if we leave
       * negative memory, things go badly wrong) */
      + (KHRN_HW_CL_BLOCK_SIZE_MAX - KHRN_HW_CL_BLOCK_SIZE_MIN)
      /* plus 2 blocks for the binner to work with until we give it some
       * overflow memory. todo: really? */
      + (2 * KHRN_HW_BIN_MEM_GRANULARITY);
   khrn_prod_msg.post->actual_user_vpm = actual_user_vpm;
   khrn_prod_msg.post->max_user_vpm = max_user_vpm;
   khrn_prod_msg.post->type = type;
   khrn_prod_msg.post->callback = callback;
   callback_data = khrn_prod_msg.post->callback_data;
   vcos_assert(callback_data_size <= sizeof(khrn_prod_msg.post->callback_data));
   /* fill in khrn_prod_msg.post->bin_mems_head/one_too_many and khrn_prod_msg.post->bin_mem in prepare_bin */
   khrn_prod_msg.post->display_slot_handle = EGL_DISP_SLOT_HANDLE_INVALID;
   khrn_prod_msg.post->display_state = MSG_DISPLAY_STATE_NOT_DONE;
   advance_msg(&khrn_prod_msg.post);
   /* don't khrn_hw_notify_llat() yet -- do that when it's ready */

   return callback_data;
}

#endif  /* BRCM_V3D_OPT */

void khrn_hw_queue_wait_for_worker(uint64_t pos)
{
   /*
      create a new wait message if we don't already have one
   */

   if (khrn_prod_msg.post == khrn_prod_msg.wait) {
      while (next_msg(khrn_prod_msg.wait) == khrn_prod_msg.done) { /* wait until there's room */
         khrn_sync_master_wait();
      }
      msg_init_wait(khrn_prod_msg.wait);
      khrn_prod_msg.wait = next_msg(khrn_prod_msg.wait);
   }

   /*
      we have an unposted wait message
   */

   vcos_assert(next_msg(khrn_prod_msg.post) == khrn_prod_msg.wait);
   if (pos > khrn_prod_msg.post->wait_worker_pos) {
      khrn_prod_msg.post->wait_worker_pos = pos;
   }
}

void khrn_hw_queue_wait_for_display(EGL_DISP_HANDLE_T disp_handle, uint32_t pos)
{
   /*
      if we've got an unposted wait message but it already has a display wait,
      we need to create another wait message for this display wait. we keep at
      most one unposted wait message, so we need to post the current wait
      message before we can create another
   */

   if ((khrn_prod_msg.post != khrn_prod_msg.wait) &&
      (khrn_prod_msg.post->wait_display_disp_handle != EGL_DISP_HANDLE_INVALID)) {
      msg_init_nop(khrn_prod_msg.post);
      advance_msg(&khrn_prod_msg.post);
      khrn_hw_notify_llat();
   }

   /*
      create a new wait message if we don't already have one
   */

   if (khrn_prod_msg.post == khrn_prod_msg.wait) {
      while (next_msg(khrn_prod_msg.wait) == khrn_prod_msg.done) { /* wait until there's room */
         khrn_sync_master_wait();
      }
      msg_init_wait(khrn_prod_msg.wait);
      khrn_prod_msg.wait = next_msg(khrn_prod_msg.wait);
   }

   /*
      we have an unposted wait message with no display wait
      fill in the display wait
   */

   vcos_assert(next_msg(khrn_prod_msg.post) == khrn_prod_msg.wait);
   vcos_assert(khrn_prod_msg.post->wait_display_disp_handle == EGL_DISP_HANDLE_INVALID);
   khrn_prod_msg.post->wait_display_disp_handle = disp_handle;
   khrn_prod_msg.post->wait_display_pos = pos;
}

void khrn_hw_queue_display(EGL_DISP_SLOT_HANDLE_T slot_handle)
{
   /* try to put the display in the last posted message */
   MSG_T *msg_post_prev = prev_msg(khrn_prod_msg.post);
   if ((khrn_prod_msg.post == khrn_prod_msg.wait) && /* no unposted wait messages */
      (msg_post_prev->display_slot_handle == EGL_DISP_SLOT_HANDLE_INVALID)) { /* last posted message has no display */
      MSG_DISPLAY_STATE_T state;
      msg_post_prev->display_slot_handle = slot_handle;
      do {
         khrn_barrier();
      } while ((state = msg_post_prev->display_state) == MSG_DISPLAY_STATE_DOING);
      if (state != MSG_DISPLAY_STATE_DONE_NONE) {
         /* not done yet, or done (ours) */
         vcos_assert((state == MSG_DISPLAY_STATE_NOT_DONE) || (state == MSG_DISPLAY_STATE_DONE));
         return;
      } /* else: did no display */
   }

   /* post the display in a new message */
   if (khrn_prod_msg.post == khrn_prod_msg.wait) {
      while (next_msg(khrn_prod_msg.wait) == khrn_prod_msg.done) { /* wait until there's room */
         khrn_sync_master_wait();
      }
      msg_init_wait(khrn_prod_msg.wait);
      khrn_prod_msg.wait = next_msg(khrn_prod_msg.wait);
   }
   vcos_assert(next_msg(khrn_prod_msg.post) == khrn_prod_msg.wait);
   msg_init_nop(khrn_prod_msg.post);
   khrn_prod_msg.post->display_slot_handle = slot_handle;
   advance_msg(&khrn_prod_msg.post);
   khrn_hw_notify_llat();
}

#ifdef BRCM_V3D_OPT
void khrn_hw_wait(void)
{
//	LOGE("entered waiting for job finish");
	pthread_mutex_lock(&khrn_hw_queue_mutex);
	if(msg_list_android == NULL)
	 {
//	 	LOGE("returning from job finish 1");
		pthread_mutex_unlock(&khrn_hw_queue_mutex);
		return;
	 }
	else
	 {
	 MSG_LIST_T* temp = msg_list_android;
	 do{
		 if(temp->next == NULL)
			 {
			 temp->last = true;
			 break;
			 }
		 temp = temp->next;
		 }while(temp);
	 }

	pthread_mutex_unlock(&khrn_hw_queue_mutex);
	v3d_job_status_t job_status;
	job_status.job_id = 14;
//	job_status.job_status = 0;

//LOGE("waiting for job finish");
	if (fd_v3d && ioctl(fileno(fd_v3d), V3D_IOCTL_WAIT_JOB, &job_status) < 0) {
		   LOGE("ioctl [0x%x] failed \n", V3D_IOCTL_WAIT_JOB);
	   }
//	LOGE("wait completed for job finish");
	pthread_mutex_lock(&khrn_hw_queue_mutex);
	if(msg_list_android == NULL)
	 {
	 LOGE("Said waiting ... but, nothing in list???");
		pthread_mutex_unlock(&khrn_hw_queue_mutex);
		return;
	 }
	else
	 {
	 MSG_LIST_T* temp = msg_list_android;
	 do{
		MEM_HANDLE_T handle,next_handle;
	 	temp->post.callback(KHRN_HW_CALLBACK_REASON_BIN_FINISHED, temp->post.callback_data, NULL);
		temp->post.callback(KHRN_HW_CALLBACK_REASON_UNFIXUP, temp->post.callback_data, NULL);
		temp->post.callback(KHRN_HW_CALLBACK_REASON_RENDER_FINISHED , temp->post.callback_data, NULL);
		handle = temp->post.bin_mems_head;
//        mem_release(handle);
		msg_list_android = temp->next;
		if(temp->last)
			{
			free(temp);
			break;
			}
		free(temp);
		temp = msg_list_android ;
		}while(temp != NULL);
	 }
//	LOGE("cleanup completed for job finish");
	pthread_mutex_unlock(&khrn_hw_queue_mutex);

	return;
}

#else /* BRCM_V3D_OPT */

void khrn_hw_wait(void)
{
   if (khrn_prod_msg.post != khrn_prod_msg.wait) {
      msg_init_nop(khrn_prod_msg.post);
      advance_msg(&khrn_prod_msg.post);
      khrn_hw_notify_llat();
   }
   vcos_assert(khrn_prod_msg.post == khrn_prod_msg.wait);

   while (khrn_prod_msg.done != khrn_prod_msg.wait) {
      khrn_sync_master_wait();
   }

   /* fifo should be empty now */
   vcos_assert(bin_mems_n == 0);
   vcos_assert(khrn_prod_msg.post == khrn_prod_msg.wait);
   vcos_assert(khrn_prod_msg.bin_prepared == khrn_prod_msg.wait);
   vcos_assert(khrn_prod_msg.render_prepared == khrn_prod_msg.wait);
   vcos_assert(khrn_prod_msg.bin_submitted == khrn_prod_msg.wait);
   vcos_assert(khrn_prod_msg.render_submitted == khrn_prod_msg.wait);
   vcos_assert(khrn_prod_msg.bin_done == khrn_prod_msg.wait);
   vcos_assert(khrn_prod_msg.render_done == khrn_prod_msg.wait);
   vcos_assert(khrn_prod_msg.bin_cleanup_llat == khrn_prod_msg.wait);
   vcos_assert(khrn_prod_msg.bin_cleanup == khrn_prod_msg.wait);
   vcos_assert(khrn_prod_msg.render_cleanup_llat == khrn_prod_msg.wait);
   vcos_assert(khrn_prod_msg.render_cleanup == khrn_prod_msg.wait);
   vcos_assert(khrn_prod_msg.display == khrn_prod_msg.wait);
   vcos_assert(khrn_prod_msg.done == khrn_prod_msg.wait);
   vcos_assert(!bin_prepared);
   vcos_assert(!render_prepared);
   vcos_assert(bins_submitted == bins_done);
   vcos_assert(renders_submitted == renders_done);
}
#endif /* BRCM_V3D_OPT */

void khrn_hw_cleanup(void)
{
#ifdef BRCM_V3D_OPT
	return;
#endif
   while (more_msgs(khrn_prod_msg.bin_cleanup, khrn_prod_msg.bin_cleanup_llat)) {
      if (khrn_prod_msg.bin_cleanup->callback) {
         if (khrn_prod_msg.bin_cleanup->ok) {
#ifdef KHRN_STATS_ENABLE
            khrn_stats_record_add(KHRN_STATS_HW_BIN, khrn_prod_msg.bin_cleanup->stats_bin_time);
#endif
            khrn_prod_msg.bin_cleanup->callback(KHRN_HW_CALLBACK_REASON_BIN_FINISHED, khrn_prod_msg.bin_cleanup->callback_data, NULL);
         }
      }
      khrn_prod_msg.bin_cleanup = next_msg(khrn_prod_msg.bin_cleanup);
   }

   while ((khrn_prod_msg.render_cleanup != khrn_prod_msg.bin_cleanup) &&
      more_msgs(khrn_prod_msg.render_cleanup, khrn_prod_msg.render_cleanup_llat)) {
      if (khrn_prod_msg.render_cleanup->callback) {
         if (khrn_prod_msg.render_cleanup->ok_prepare) {
#ifdef KHRN_STATS_ENABLE
            khrn_stats_record_add(KHRN_STATS_HW_RENDER, khrn_prod_msg.render_cleanup->stats_render_time);
#endif
            khrn_prod_msg.render_cleanup->callback(KHRN_HW_CALLBACK_REASON_UNFIXUP, khrn_prod_msg.render_cleanup->callback_data, NULL);
         }
         khrn_prod_msg.render_cleanup->callback(khrn_prod_msg.render_cleanup->ok ? KHRN_HW_CALLBACK_REASON_RENDER_FINISHED : KHRN_HW_CALLBACK_REASON_OUT_OF_MEM, khrn_prod_msg.render_cleanup->callback_data, NULL);
      }
      khrn_prod_msg.render_cleanup = next_msg(khrn_prod_msg.render_cleanup);
   }

   while ((khrn_prod_msg.done != khrn_prod_msg.render_cleanup) &&
      more_msgs(khrn_prod_msg.done, khrn_prod_msg.display)) {
      khrn_prod_msg.done = next_msg(khrn_prod_msg.done);
   }
}

/*
   isr
*/

#ifdef CARBON
typedef struct CARBON_STATISTICS_S_
{
   unsigned __int64 calls;
   unsigned __int64 runtime;
   unsigned __int64 realtime;
   unsigned __int64 transactions[4][8][16];
} carbon_statistics_t;

void v3d_carbonStatistics(void *p);
#endif

//#ifdef __ARMEL__
//void khrn_hw_full_memory_barrier(void)
//{
//  register uint32_t x asm ("r0");
//  register uint32_t *ptr asm ("r1");
//  ptr = (uint32_t *)memory_pool_base;
//  asm volatile ("ldr %0, [%1]" : "=r" (x) : "r" (ptr));
//  asm volatile ("str %0, [%1]" :: "r" (x), "r" (ptr));
//}
//#else
void khrn_hw_full_memory_barrier(void)
{
	printf("khrn_hw_full_memory_barrier %s %d\n", __FILE__, __LINE__);
}
//#endif

static void khrn_hw_isr(uint32_t flags)
{
   bool notify_master = false;
   bool notify_llat = false;
   bool notify_worker = false;
   gettimeofday(&t2,NULL);
   if(t1.tv_sec == logtime)
   {
	   unsigned int utemp = t2.tv_sec - t1.tv_sec;
	   utemp = utemp*1000000;
	   utemp += t2.tv_usec - t1.tv_usec;
	   ucount += utemp;
   }
   else
   {
	   LOGV("APP V3D used for %d",ucount);
	   ucount = 0;
	   logtime = t1.tv_sec;
   }
#ifndef ANDROID
   v3d_write(INTCTL, flags);
   if (flags & (1 << 2)) {
      v3d_write(INTDIS, 1 << 2); /* this interrupt will be forced high until we supply some memory... */
   }
#endif
   khrn_hw_full_memory_barrier();

   if (flags & (1 << 1)) {
      /* bin flush done */

      uint32_t n = (v3d_read(BFC) - bins_done) & 0xff; /* BFC has 8-bit count */
#ifdef BRCM_V3D_OPT
      n = 1;
#endif
      bins_done += n;

      vcos_assert(n == 1);
      vcos_assert(next_msg(khrn_prod_msg.bin_done) == khrn_prod_msg.bin_submitted);
      vcos_assert(khrn_prod_msg.bin_done->callback);
      vcos_assert(khrn_prod_msg.bin_done->ok_prepare);

      cancel_want_overflow = true;
      khrn_hw_full_memory_barrier();
      if (v3d_read(BPOS) != 0) {
         khrn_prod_msg.bin_done->bin_mems_one_too_many = true;
      }
#ifdef KHRN_HW_MEASURE_BIN_TIME
      measure_bin_end = vcos_getmicrosecs();
#endif
#ifdef KHRN_STATS_ENABLE
      khrn_prod_msg.bin_done->stats_bin_time += vcos_getmicrosecs();
#endif
#ifdef KHRN_HW_BIN_HISTORY_N
      khrn_hw_bin_history[bin_history_i++] = vcos_getmicrosecs();
      /* use khrn_hw_bin_history_n instead of KHRN_HW_BIN_HISTORY_N to force
       * khrn_hw_bin_history_n to be linked in */
      if (bin_history_i == khrn_hw_bin_history_n) { bin_history_i = 0; }
#endif
      advance_msg(&khrn_prod_msg.bin_done);
      khrn_hw_advance_bin_exit_pos();
      notify_master = true; /* potentially waiting for advance_bin_exit_pos() */
      notify_llat = true; /* submit next, cleanup llat */
      notify_worker = true; /* potentially waiting for advance_bin_exit_pos() */
   } else {
      if (flags & (1 << 2)) {
         /* binner out of memory */

#ifdef __BCM2708A0__
         want_overflow = true;
#endif
         notify_llat = true; /* supply overflow */
      }

#ifndef __BCM2708A0__
      if (flags & (1 << 3)) {
         /* overflow memory taken */

         want_overflow = true;
         notify_llat = true; /* supply overflow */
      }
#endif
   }

   if (flags & (1 << 0)) {
      /* render frame done */
      uint32_t n = (v3d_read(RFC) - renders_done) & 0xff; /* RFC has 8-bit count */
#ifdef BRCM_V3D_OPT
		n = 1;
#endif
	  pthread_mutex_unlock(&khrn_hw_queue_mutex);
#ifdef CARBON
      int i, j;
      carbon_statistics_t cs;
      const char ufname[8][32] =
      {
         "V3D_AXI_ID_L2C",
         "V3D_AXI_ID_CLE",
         "V3D_AXI_ID_PTB",
         "V3D_AXI_ID_PSE",
         "V3D_AXI_ID_VCD",
         "V3D_AXI_ID_VDW",
         "spare - ???",
         "V3D_AXI_ID_TLB"
      };

      memset(&cs, 0, sizeof(carbon_statistics_t));
      /* v3d_carbonStatistics((void *)&cs); */

      printf("read transaction breakdown (UNCACHED)\n--------------------------\n");
      for(i = 0; i < 8; i++)
      {
         for(j = 0; j < 16; j++)
         {
            if (cs.transactions[0][i][j])
            {
               printf("ID = %3.3d (%s), size = %3.3d, count = %16.16lld\n", i, ufname[i], j, cs.transactions[0][i][j]);
            }
         }
      }

      printf("read transaction breakdown (CACHED)\n--------------------------\n");
      for(i = 0; i < 8; i++)
      {
         for(j = 0; j < 16; j++)
         {
            if (cs.transactions[2][i][j])
            {
               printf("ID = %3.3d (%s), size = %3.3d, count = %16.16lld\n", i, ufname[i], j, cs.transactions[2][i][j]);
            }
         }
      }

      printf("write transaction breakdown (LOW PRIORITY) \n---------------------------\n");
      for(i = 0; i < 8; i++)
      {
         for(j = 0; j < 16; j++)
         {
            if (cs.transactions[1][i][j])
            {
               printf("ID = %3.3d (%s), size = %3.3d, count = %16.16lld\n", i, ufname[i], j, cs.transactions[1][i][j]);
            }
         }
      }

      printf("write transaction breakdown (HIGH PRIORITY) \n---------------------------\n");
      for(i = 0; i < 8; i++)
      {
         for(j = 0; j < 16; j++)
         {
            if (cs.transactions[3][i][j])
            {
               printf("ID = %3.3d (%s), size = %3.3d, count = %16.16lld\n", i, ufname[i], j, cs.transactions[3][i][j]);
            }
         }
      }
#endif
      renders_done += n;

      if (n) { /* if a frame completes between writing to INTCTL and reading from RFC we'll pass through the isr again and n might be 0 */
         vcos_assert((next_msg(khrn_prod_msg.render_done) == khrn_prod_msg.render_submitted) &&
            khrn_prod_msg.render_done->callback && khrn_prod_msg.render_done->ok_prepare &&
            (khrn_prod_msg.render_done->render_n >= n));
         khrn_prod_msg.render_done->render_n -= n;
         if (!khrn_prod_msg.render_done->render_n) {
            /* if the final frame completed between writing to INTCTL and
             * reading from RFC, the render frame done interrupt will be high.
             * leaving it high would be bad */
            v3d_write(INTCTL, 1 << 0);
            khrn_hw_full_memory_barrier();
#ifdef KHRN_HW_MEASURE_RENDER_TIME
            measure_render_end = vcos_getmicrosecs();
#endif
#ifdef KHRN_STATS_ENABLE
            khrn_prod_msg.render_done->stats_render_time += vcos_getmicrosecs();
#endif
#ifdef KHRN_HW_RENDER_HISTORY_N
            khrn_hw_render_history[render_history_i++] = vcos_getmicrosecs();
            /* use khrn_hw_render_history_n instead of KHRN_HW_RENDER_HISTORY_N
             * to force khrn_hw_render_history_n to be linked in */
            if (render_history_i == khrn_hw_render_history_n) { render_history_i = 0; }
#endif
            advance_msg(&khrn_prod_msg.render_done);
            khrn_hw_advance_render_exit_pos();
            notify_master = true; /* potentially waiting for advance_render_exit_pos() */
            notify_llat = true; /* submit next, cleanup llat, display */
            notify_worker = true; /* potentially waiting for advance_render_exit_pos() */
         }
      }
   }

   if (notify_master) { khrn_sync_notify_master(); }
   if (notify_llat) { khrn_hw_notify_llat(); }
   if (notify_worker) { khrn_worker_notify(); }
}

/*
   llat callback
*/

static bool waiting_for_acquire_callback = false; /* just a (probably unnecessary) optimisation */
#ifdef WORKAROUND_HW2781
static bool used_vg_shaders = false;
#endif
static uint32_t acquire_n = 0;

static void acquire_callback(void *p)
{
   UNUSED(p);
   waiting_for_acquire_callback = false;
   khrn_hw_notify_llat();
}

static bool acquire_v3d(uint32_t actual_user_vpm, uint32_t max_user_vpm
#ifdef WORKAROUND_HW2781
   , bool uses_vg_shaders
#endif
   )
{
   if (waiting_for_acquire_callback) {
      return false;
   }
   khrn_barrier();
   waiting_for_acquire_callback = true;
   /* todo: actual_user_vpm is ignored if forcing a reset. not sure if that's
    * the right thing to do. also, not really sure on this whole actual_user_vpm
    * thing in the first place */
   if (v3d_get_func_table()->acquire_main(v3d_driver_handle, NULL, actual_user_vpm, max_user_vpm
#ifdef WORKAROUND_HW2781
      , !uses_vg_shaders && used_vg_shaders
#else
      , 0
#endif
      )) {
      return false;
   }
   waiting_for_acquire_callback = false;
#ifdef WORKAROUND_HW2781
   used_vg_shaders = uses_vg_shaders;
#endif
//   if (acquire_n++ == 0) {
      v3d_write(BFC, 1); /* writing 1 clears */
      v3d_write(RFC, 1);
      bins_submitted = 0;
      renders_submitted = 0;
      bins_done = 0;
      renders_done = 0;

#ifndef CARBON
      vcos_assert(!(v3d_read(INTCTL) & ((1 << 0) | (1 << 1)))); /* render/frame done interrupts should be low... */
#endif
      v3d_write(INTCTL, (1 << 0) | (1 << 1)); /* clear them just incase */
      v3d_write(INTENA,
         (1 << 0) | /* render frame done */
         (1 << 1)); /* bin flush done */

      reset_perf_counters();
//   }

   return true;
}

static void release_v3d(uint32_t max_user_vpm)
{
   if (--acquire_n == 0) {
      /* we don't think the hardware is busy */
      vcos_assert(!((bins_submitted - v3d_read(BFC)) & 0xff));
      vcos_assert(!((renders_submitted - v3d_read(RFC)) & 0xff));
      /* the hardware doesn't think it's busy */
      vcos_assert(!(v3d_read(PCS) & ((1 << 0) | (1 << 2))));
   }

   update_perf_counters();

   v3d_get_func_table()->release_main(v3d_driver_handle, max_user_vpm);
}

#ifdef BRCM_V3D_OPT

static void prepare_bin(MSG_T* bin_prepared)
{
     if (!bin_prepared->ready) {
        return;
         }
         khrn_barrier();
     if (bin_prepared->ok) {
            bool bin_mem_from_pool;
            uint32_t bin_mem_size;

        bin_mem_from_pool = BIN_MEM_SIZE >= bin_prepared->bin_mem_size_min;
        bin_mem_size = bin_mem_from_pool ? BIN_MEM_SIZE : bin_prepared->bin_mem_size_min;
        //bin_prepared->bin_mems_head =  mem_alloc_ex(bin_mem_size, KHRN_HW_BIN_MEM_ALIGN, (MEM_FLAG_T)(MEM_FLAG_DIRECT | MEM_FLAG_NO_INIT | MEM_FLAG_RETAINED), "khrn_hw_bin_mem (too cool for pool)", MEM_COMPACT_NONE);
        if (0){//bin_prepared->bin_mems_head == MEM_INVALID_HANDLE) {
               if (bin_mems_n != 0) {
                  /* wait for one to be freed (this makes sense even for the separate allocation mode as we can discard bin mems)... */
			  LOGE("Returning without prepare in prepare_bin");
              return;
               }
		   LOGE("Returning without prepare in prepare_bin 1");
           bin_prepared->ok = false;
           bin_prepared->ok_prepare = false;
            } else {
               uint32_t specials[4];
           bin_prepared->ok_prepare = true;
           //set_bin_mems_next(bin_prepared->bin_mems_head, MEM_INVALID_HANDLE);
           bin_prepared->bin_mem = mem_lock(g_bin_mems_head);
           bin_prepared->bin_mems_one_too_many = false;
           specials[0] = bin_prepared->special_0;
           specials[1] = khrn_hw_addr(bin_prepared->bin_mem);
           specials[2] = khrn_hw_addr(bin_prepared->bin_mem) + bin_mem_size;
#ifdef MEGA_DEBUG
           printf("bin addr %08x, size %08x\n", khrn_hw_addr(bin_prepared->bin_mem), bin_mem_size);
#endif
           _bin_start = (unsigned int)khrn_hw_addr(bin_prepared->bin_mem);
           _bin_end = (unsigned int)(khrn_hw_addr(bin_prepared->bin_mem) + bin_mem_size);

           specials[3] = bin_mem_size;
           bin_begin = bin_prepared->bin_begin;
           bin_end = bin_prepared->bin_end;
		   //LOGE("specials %x %x %x %x %x",specials[0],specials[1],specials[2],specials[3],gettid());
               fixup_done = false;
           bin_prepared->callback(KHRN_HW_CALLBACK_REASON_FIXUP, bin_prepared->callback_data, specials);
               khrn_hw_flush_dcache(); /* todo: flush only what needs to be flushed (stuff in fixup callback might be assuming this flush too!) */
               bin_prepared = true;
            }
         } else {
        bin_prepared->ok_prepare = false;
   }
}

#else /* BRCM_V3D_OPT */

static void prepare_bin(void)
{
   while (!bin_prepared && more_msgs(khrn_prod_msg.bin_prepared, khrn_prod_msg.post)) {
      if (khrn_prod_msg.bin_prepared->callback) {
         if (!khrn_prod_msg.bin_prepared->ready) {
            break;
         }
         khrn_barrier();
         if (khrn_prod_msg.bin_prepared->ok) {
            bool bin_mem_from_pool;
            uint32_t bin_mem_size;

            /* acquiring here means we maybe have the hw on for a little longer
             * than necessary, but also means we won't keep stuff locked while
             * waiting to acquire. todo: what if we want to run user shaders as
             * part of fixup (which we might want to do for gl)? then we have a
             * potential deadlock... */
            if (!acquire_v3d(khrn_prod_msg.bin_prepared->actual_user_vpm, khrn_prod_msg.bin_prepared->max_user_vpm
#ifdef WORKAROUND_HW2781
               /* assume we use vg shaders iff we are vg. which seems reasonable... */
               , khrn_prod_msg.bin_prepared->type == KHRN_HW_TYPE_VG
#endif
               )) {
               break;
            }
            bin_mem_from_pool = BIN_MEM_SIZE >= khrn_prod_msg.bin_prepared->bin_mem_size_min;
            bin_mem_size = bin_mem_from_pool ? BIN_MEM_SIZE : khrn_prod_msg.bin_prepared->bin_mem_size_min;
            khrn_prod_msg.bin_prepared->bin_mems_head = bin_mem_from_pool ? bin_mem_pool_alloc(true) :
               /* pool bin mems aren't big enough. allocate separately. todo: is this going to hurt? */
               mem_alloc_ex(bin_mem_size, KHRN_HW_BIN_MEM_ALIGN, (MEM_FLAG_T)(MEM_FLAG_DIRECT | MEM_FLAG_NO_INIT), "khrn_hw_bin_mem (too cool for pool)", MEM_COMPACT_DISCARD);
            if (khrn_prod_msg.bin_prepared->bin_mems_head == MEM_INVALID_HANDLE) {
               release_v3d(khrn_prod_msg.bin_prepared->max_user_vpm); /* todo: avoid acquiring/releasing so much when waiting for bin mem? */
               if (bin_mems_n != 0) {
                  /* wait for one to be freed (this makes sense even for the separate allocation mode as we can discard bin mems)... */
                  break;
               }
               khrn_prod_msg.bin_prepared->ok = false;
               khrn_prod_msg.bin_prepared->ok_prepare = false;
            } else {
               uint32_t specials[4];
               khrn_prod_msg.bin_prepared->ok_prepare = true;
               set_bin_mems_next(khrn_prod_msg.bin_prepared->bin_mems_head, MEM_INVALID_HANDLE);
               khrn_prod_msg.bin_prepared->bin_mem = mem_lock(khrn_prod_msg.bin_prepared->bin_mems_head);
               khrn_prod_msg.bin_prepared->bin_mems_one_too_many = false;
               specials[0] = khrn_prod_msg.bin_prepared->special_0;
               specials[1] = khrn_hw_addr(khrn_prod_msg.bin_prepared->bin_mem);
               specials[2] = khrn_hw_addr(khrn_prod_msg.bin_prepared->bin_mem) + bin_mem_size;
               specials[3] = bin_mem_size;
               bin_begin = khrn_prod_msg.bin_prepared->bin_begin;
               bin_end = khrn_prod_msg.bin_prepared->bin_end;
               fixup_done = false;
               khrn_prod_msg.bin_prepared->callback(KHRN_HW_CALLBACK_REASON_FIXUP, khrn_prod_msg.bin_prepared->callback_data, specials);
               khrn_hw_flush_dcache(); /* todo: flush only what needs to be flushed (stuff in fixup callback might be assuming this flush too!) */
               bin_prepared = true;
            }
         } else {
            khrn_prod_msg.bin_prepared->ok_prepare = false;
         }
      }
      khrn_prod_msg.bin_prepared = next_msg(khrn_prod_msg.bin_prepared);
   }
}

#endif /* BRCM_V3D_OPT */

static void prepare_render(void)
{
   while (!render_prepared && (khrn_prod_msg.render_prepared != khrn_prod_msg.bin_prepared)) {
      if (khrn_prod_msg.render_prepared->callback && khrn_prod_msg.render_prepared->ok_prepare) {
         render_begin = khrn_prod_msg.render_prepared->render_begin;
         render_end = khrn_prod_msg.render_prepared->render_end;
         khrn_hw_flush_dcache(); /* todo: flush only what needs to be flushed */
         render_prepared = true;
      }
      khrn_prod_msg.render_prepared = next_msg(khrn_prod_msg.render_prepared);
   }
}

static void clear_caches(KHRN_HW_CC_FLAG_T cc)
{
   if (cc & KHRN_HW_CC_FLAG_L2) {
      v3d_write(L2CACTL, 1 << 2);
   }
   if (cc & (KHRN_HW_CC_FLAG_IC | KHRN_HW_CC_FLAG_UC | KHRN_HW_CC_FLAG_TU)) {
      v3d_write(SLCACTL,
         ((cc & KHRN_HW_CC_FLAG_IC) ? 0x000000ff : 0) |
         ((cc & KHRN_HW_CC_FLAG_UC) ? 0x0000ff00 : 0) |
         ((cc & KHRN_HW_CC_FLAG_TU) ? 0xffff0000 : 0));
   }
}

static void submit_bin(bool *notify_master, bool *notify_worker)
{
   while (processed_msgs(khrn_prod_msg.bin_done, khrn_prod_msg.bin_submitted) &&
      (khrn_prod_msg.bin_submitted != khrn_prod_msg.bin_prepared)
#ifdef WORKAROUND_HW2422
      /* not going to hit the semaphore limit */
      && (!khrn_prod_msg.bin_submitted->callback || !khrn_prod_msg.bin_submitted->ok || (((v3d_read(CT0CS) >> 12) & 7) != 7)) &&
      /* no rendering going on right now */
      (processed_msgs(khrn_prod_msg.render_done, khrn_prod_msg.render_submitted) ||
      /* or rendering isn't using tu (assuming if it was using tu, it would clear tu caches beforehand) */
      !(khrn_prod_msg.render_done->render_cc & KHRN_HW_CC_FLAG_TU) || /* khrn_prod_msg.render_done might have changed... but that's fine */
      /* or we're not going to clear the tu caches */
      !(khrn_prod_msg.bin_submitted->bin_cc & KHRN_HW_CC_FLAG_TU))
#endif
      ) {
      KHRN_HW_CC_FLAG_T cc;
      bool hw = !!khrn_prod_msg.bin_submitted->callback;
      bool ok = hw && khrn_prod_msg.bin_submitted->ok;
      vcos_assert(khrn_prod_msg.bin_submitted->ok == khrn_prod_msg.bin_submitted->ok_prepare);
      if (ok && !fixup_done) {
         break;
      }
      khrn_barrier();
      cc = khrn_prod_msg.bin_submitted->bin_cc;
      khrn_prod_msg.bin_submitted = next_msg(khrn_prod_msg.bin_submitted);
      if (ok) {
         vcos_assert(khrn_prod_msg.bin_submitted == khrn_prod_msg.bin_prepared);
         vcos_assert(bin_prepared);
         /* reset the ptb alloc stuff:
          * - not out of memory
          * - no overflow memory
          * - binner-out-of-memory and (not a0) overflow-memory-taken interrupts
          *   enabled but low */
#ifndef __BCM2708A0__
         v3d_write(INTDIS, 1 << 3);
#endif
         v3d_write(BPOS, 2 * KHRN_HW_BIN_MEM_GRANULARITY); /* 2 blocks is enough to force out of memory low in the worst case (need both clip + cl memory) */
         v3d_write(BPOS, 0);
         /* the BPOS write doesn't take effect immediately: wait for the oom bit
          * to go low... */
         while (v3d_read(PCS) & (1 << 8)) ;
#ifdef __BCM2708A0__
         v3d_write(INTCTL, 1 << 2);
         v3d_write(INTENA, 1 << 2);
#else
         v3d_write(INTCTL, (1 << 2) | (1 << 3));
         v3d_write(INTENA, (1 << 2) | (1 << 3));
#endif
         want_overflow = true;
         cancel_want_overflow = false;
         ++bins_submitted;
#ifdef KHRN_HW_MEASURE_BIN_TIME
         printf("%d\n", measure_bin_end - measure_bin_start);
         measure_bin_start = vcos_getmicrosecs();
#endif
#ifdef KHRN_STATS_ENABLE
         khrn_prod_msg.bin_done->stats_bin_time = -vcos_getmicrosecs();
#endif
#ifdef KHRN_HW_BIN_HISTORY_N
         khrn_hw_bin_history[bin_history_i++] = vcos_getmicrosecs();
         vcos_assert(bin_history_i != KHRN_HW_BIN_HISTORY_N);
#endif
         khrn_hw_full_memory_barrier();
         clear_caches(cc);
		 gettimeofday(&t1,NULL);
			{
				if(cacheops('F',0x0,0x0,0x20000)<0)
				{
					LOGE("Error in flushing cache\n");
					return;
				}
			}
#ifdef BRCM_V3D_OPT
		 khrn_hw_isr(2);// fake bin interrupt
#else
         v3d_write(CT0CA, khrn_hw_addr(bin_begin));
         v3d_write(CT0EA, khrn_hw_addr(bin_end));
#endif
         bin_prepared = false;
      } else {
         khrn_prod_msg.bin_done = next_msg(khrn_prod_msg.bin_done);
         if (hw) {
            khrn_hw_advance_bin_exit_pos();
            *notify_master = true;
            *notify_worker = true;
         }
      }
   }
}

#ifdef __BCM2708A0__
static void set_null_render_frame(uint8_t *p, void *frame)
{
   uint32_t f = khrn_hw_addr(frame);
   p[NULL_RENDER_FRAME_OFFSET + 0] = (uint8_t)f;
   p[NULL_RENDER_FRAME_OFFSET + 1] = (uint8_t)(f >> 8);
   p[NULL_RENDER_FRAME_OFFSET + 2] = (uint8_t)(f >> 16);
   p[NULL_RENDER_FRAME_OFFSET + 3] = (uint8_t)(f >> 24);
}
#endif

static void submit_render(bool *notify_master, bool *notify_worker)
{
   while (processed_msgs(khrn_prod_msg.render_done, khrn_prod_msg.render_submitted) &&
      (khrn_prod_msg.render_submitted != khrn_prod_msg.render_prepared) &&
      /* todo: this is needed for both the hw-2422 workaround and to wait for
       * fixup. might be clearer to move this into the ifdef and explicitly wait
       * for fixup... */
      (khrn_prod_msg.render_submitted != khrn_prod_msg.bin_submitted) &&
      (khrn_worker_get_exit_pos() >= khrn_prod_msg.render_submitted->wait_worker_pos) &&
      ((khrn_prod_msg.render_submitted->wait_display_disp_handle == EGL_DISP_HANDLE_INVALID) ||
      !egl_disp_still_on(khrn_prod_msg.render_submitted->wait_display_disp_handle, khrn_prod_msg.render_submitted->wait_display_pos))
#ifdef WORKAROUND_HW2422
      /* no binning going on right now */
      && (processed_msgs(khrn_prod_msg.bin_done, khrn_prod_msg.bin_submitted) ||
      /* or binning isn't using tu (assuming if it was using tu, it would clear tu caches beforehand) */
      !(khrn_prod_msg.bin_done->bin_cc & KHRN_HW_CC_FLAG_TU) || /* khrn_prod_msg.bin_done might have changed... but that's fine */
      /* or we're not going to clear the tu caches */
      !(khrn_prod_msg.render_submitted->render_cc & KHRN_HW_CC_FLAG_TU))
#endif
      ) {
      bool hw = !!khrn_prod_msg.render_submitted->callback;
      bool ok = hw && khrn_prod_msg.render_submitted->ok_prepare; /* we want to put the render list through iff we put the bin list through to keep the hw semaphore stuff in sync */
      KHRN_HW_CC_FLAG_T cc = khrn_prod_msg.render_submitted->render_cc;
      khrn_prod_msg.render_submitted = next_msg(khrn_prod_msg.render_submitted);
      if (ok) {
         vcos_assert(khrn_prod_msg.render_submitted == khrn_prod_msg.render_prepared);
         vcos_assert(render_prepared);
         if (!khrn_prod_msg.render_done->ok) {
            /* we gave up on this frame. we still want to put a render list
             * through (to keep the hw semaphore stuff in sync), but we can just
             * put the "null" render list through (just does a wait semaphore
             * and eof) */
            uint8_t *p = (uint8_t *)mem_lock(null_render_handle); /* this gets unlocked in cleanup_render_llat */
#ifdef __BCM2708A0__
            set_null_render_frame(p, khrn_prod_msg.render_done->bin_mem);
#endif
            khrn_prod_msg.render_done->render_n = 1; /* only going to have 1 eof */
            render_begin = p;
            render_end = p + NULL_RENDER_SIZE;
         }
         renders_submitted += khrn_prod_msg.render_done->render_n;
#ifdef KHRN_HW_MEASURE_RENDER_TIME
         printf(",%d\n", measure_render_end - measure_render_start);
         measure_render_start = vcos_getmicrosecs();
#endif
#ifdef KHRN_STATS_ENABLE
         khrn_prod_msg.render_done->stats_render_time = -vcos_getmicrosecs();
#endif
#ifdef KHRN_HW_RENDER_HISTORY_N
         khrn_hw_render_history[render_history_i++] = vcos_getmicrosecs();
         vcos_assert(render_history_i != KHRN_HW_RENDER_HISTORY_N);
#endif
         khrn_hw_full_memory_barrier();
         clear_caches(cc);
		 gettimeofday(&t1,NULL);
		 {
		 	if(cacheops('F',0x0,0x0,0x20000)<0)
			{
				LOGE("Error in flushing cache\n");
				return;
			}
		 }
#ifdef BRCM_V3D_OPT
		v3d_job_post_t job_post;
		v3d_job_status_t job_status;

		job_post.job_type = V3D_JOB_BIN_REND;
		job_post.job_id = 15;
		job_post.v3d_ct0ca = khrn_hw_addr(bin_begin);
		job_post.v3d_ct0ea = khrn_hw_addr(bin_end);
		job_post.v3d_ct1ca = khrn_hw_addr(render_begin);
		job_post.v3d_ct1ea = khrn_hw_addr(render_end);

		job_status.job_id = 15;
		job_status.job_status = V3D_JOB_STATUS_INVALID;
		if (fd_v3d && ioctl(fileno(fd_v3d), V3D_IOCTL_POST_JOB, &job_post) < 0) {
			LOGE("ioctl [0x%x] failed \n", V3D_IOCTL_POST_JOB);
		}
  		khrn_hw_isr(1);
#else
         v3d_write(CT1CA, khrn_hw_addr(render_begin));
         v3d_write(CT1EA, khrn_hw_addr(render_end));
#endif
         render_prepared = false;
      } else {
         khrn_prod_msg.render_done = next_msg(khrn_prod_msg.render_done);
         if (hw) {
            khrn_hw_advance_render_exit_pos();
            *notify_master = true;
            *notify_worker = true;
         }
      }
   }
}

#ifdef KHRN_HW_BOOM
static uint32_t boom_n = 0;
#endif

static void supply_overflow(void)
{
#ifdef __BCM2708A0__
   while (want_overflow && !cancel_want_overflow) {
#else
   if (want_overflow && !cancel_want_overflow) {
#endif
      MSG_T *msg = prev_msg(khrn_prod_msg.bin_submitted); /* can't use khrn_prod_msg.bin_done -- it might have advanced... */
      void *bin_mem;
      uint32_t bin_mem_size;

#ifdef __BCM2708A0__
      /* break out of the loop when we can't give the hw any more memory */
      want_overflow = false;
      khrn_hw_full_memory_barrier();
      if (v3d_read(BPOS) != 0) {
         break;
      }
      want_overflow = true;
#else
      /* we only set want_overflow in response to an overflow-memory-taken
       * interrupt. we only supply more overflow memory in this function, and we
       * reset want_overflow just before supplying it */
      vcos_assert(v3d_read(BPOS) == 0);
#endif

      if (msg->ok) {
         /* we can't wait for bin mems here -- a stalled bin can stall a render
          * if it's holding too much vpm */
         MEM_HANDLE_T handle;
#ifdef KHRN_HW_BOOM
         if (++boom_n >= 64) {
            /* bwahahahaha */
            handle = MEM_INVALID_HANDLE;
         } else
#endif
            handle = bin_mem_pool_alloc(false);
         if (handle == MEM_INVALID_HANDLE) {
            if (/* the ptb isn't actually out of memory -- we might be ok */
               !(v3d_read(PCS) & (1 << 8)) ||
               /* the ptb has managed to finish. this is possible as the ptb
                * tries to allocate memory in advance and will signal out of
                * memory if it can't, even if the memory might not be needed */
               !((bins_submitted - v3d_read(BFC)) & 0xff)) { /* BFC has 8-bit count */
               return;
            }
#ifdef KHRN_HW_BOOM
            /* only reset here to make sure we hit this case -- it's the tricky one */
            boom_n = 0;
#endif

            /* we're pretty sure at this point that we're going to give up on
             * this frame... */

            if (khrn_prod_msg.render_submitted == khrn_prod_msg.bin_submitted) {
               /* we've already submitted the render list; we need to switch
                * over to the null render list... */

               uint8_t *p;

               /* wait for the render cle to hit the wait semaphore instruction
                * (we always follow the wait semaphore instruction with a marker
                * instruction) */
               do {
                  p = (uint8_t *)khrn_hw_unaddr(v3d_read(CT1CA));
                  if (!((bins_submitted - v3d_read(BFC)) & 0xff)) {
                     /* the ptb has managed to finish... */
                     return;
                  }
               } while (*p != KHRN_HW_INSTR_MARKER); /* assuming there are no prims or markers before the wait semaphore (prims might look like a marker) */

               /* stop the render cle just after the wait semaphore instruction
                * (it should be stalled there) */
               v3d_write(CT1CS, 1 << 5); /* stop */
               if (!((bins_submitted - v3d_read(BFC)) & 0xff)) {
                  /* the ptb has managed to finish... */
                  v3d_write(CT1CS, 1 << 4); /* go */
                  return;
               }
               vcos_assert(v3d_read(CT1CA) == khrn_hw_addr(p));

               /* switch over to the null render list */
               p = (uint8_t *)mem_lock(null_render_handle); /* this gets unlocked in cleanup_render_llat */
#ifdef __BCM2708A0__
               set_null_render_frame(p, msg->bin_mem);
#endif
               vcos_assert(msg->render_n > 0);
               msg->render_n = 1; /* only going to have 1 eof */
               khrn_hw_full_memory_barrier();
               v3d_write(CT1CA, khrn_hw_addr(p + NULL_RENDER_POST_WAIT_SEMAPHORE_OFFSET));
               v3d_write(CT1EA, khrn_hw_addr(p + NULL_RENDER_SIZE));
               v3d_write(CT1CS, 1 << 4); /* go */
            } /* else: we haven't submitted the render list; we'll submit the null render list when we get around to it */

            /* by this point, we're sure we're going to give up on this frame.
             * the binner might actually be able to finish without any more
             * memory, but we have no way of knowing */
            msg->ok = false;

            /* to allow the ptb to finish, we feed it the initial bin mem over
             * and over. todo: we could also try to cut the bin control list
             * short, but i'm not convinced we can easily do that safely */
            bin_mem = msg->bin_mem;
            bin_mem_size = BIN_MEM_TRASHABLE_SIZE;
         } else {
            /* successfully allocated another bin mem... */

            set_bin_mems_next(handle, msg->bin_mems_head);
            msg->bin_mems_head = handle;

            bin_mem = mem_lock(handle);
            bin_mem_size = BIN_MEM_SIZE;
         }
      } else {
         /* we've given up on this frame. keep feeding the ptb the initial bin
          * mem to allow it to finish */

         bin_mem = msg->bin_mem;
         bin_mem_size = BIN_MEM_TRASHABLE_SIZE;
      }

#ifndef __BCM2708A0__
      want_overflow = false; /* might write to this right after writing to BPOS, so make sure we clear it first */
      khrn_hw_full_memory_barrier();
#endif
      v3d_write(BPOA, khrn_hw_addr(bin_mem));
      vcos_assert(bin_mem_size >= KHRN_HW_BIN_MEM_GRANULARITY); /* ptb malloc won't take anything less than this */
      v3d_write(BPOS, bin_mem_size);
      khrn_hw_full_memory_barrier();
      if (cancel_want_overflow) {
         msg->bin_mems_one_too_many = true;
      }

      /* we've just given the hw some memory, which should have removed any
       * out-of-memory condition, so we can reenable the interrupt */
      if (!(v3d_read(INTENA) & (1 << 2))) {
         v3d_write(INTCTL, 1 << 2);
         v3d_write(INTENA, 1 << 2);
      }
   }
}

static void cleanup_bin_llat(bool *notify_master)
{
#ifndef BRCM_V3D_OPT
   while (more_msgs(khrn_prod_msg.bin_cleanup_llat, khrn_prod_msg.bin_done)) {
      if (khrn_prod_msg.bin_cleanup_llat->callback) {
         if (khrn_prod_msg.bin_cleanup_llat->ok && khrn_prod_msg.bin_cleanup_llat->bin_mems_one_too_many) {
            MEM_HANDLE_T handle = khrn_prod_msg.bin_cleanup_llat->bin_mems_head;
            khrn_prod_msg.bin_cleanup_llat->bin_mems_head = get_bin_mems_next(handle);
            mem_unlock(handle);
            bin_mem_pool_free(handle);
         }
         khrn_prod_msg.bin_cleanup_llat->callback(khrn_prod_msg.bin_cleanup_llat->ok ?
            KHRN_HW_CALLBACK_REASON_BIN_FINISHED_LLAT :
            KHRN_HW_CALLBACK_REASON_OUT_OF_MEM_LLAT,
            khrn_prod_msg.bin_cleanup_llat->callback_data,
            NULL);
      }
      advance_msg(&khrn_prod_msg.bin_cleanup_llat);
      *notify_master = true;
   }
#endif
}

static void cleanup_render_llat(bool *notify_master)
{
#ifdef BRCM_V3D_OPT
	return;
#endif
   while ((khrn_prod_msg.render_cleanup_llat != khrn_prod_msg.bin_cleanup_llat) &&
      more_msgs(khrn_prod_msg.render_cleanup_llat, khrn_prod_msg.render_done)) {
      if (khrn_prod_msg.render_cleanup_llat->callback && khrn_prod_msg.render_cleanup_llat->ok_prepare) {
         MEM_HANDLE_T handle, next_handle;
         release_v3d(khrn_prod_msg.render_cleanup_llat->max_user_vpm);
         if (!khrn_prod_msg.render_cleanup_llat->ok) {
            mem_unlock(null_render_handle); /* locked in supply_overflow or submit_render */
         }
         handle = khrn_prod_msg.render_cleanup_llat->bin_mems_head;
         while (handle != MEM_INVALID_HANDLE) {
            next_handle = get_bin_mems_next(handle);
            mem_unlock(handle);
            if (/* last handle (first allocated) */
               (next_handle == MEM_INVALID_HANDLE) &&
               /* too big for pool bin mems (ie must have allocated separately) */
               (BIN_MEM_SIZE < khrn_prod_msg.render_cleanup_llat->bin_mem_size_min)) {
               mem_release(handle);
            } else {
               bin_mem_pool_free(handle);
            }
            handle = next_handle;
         }
      }
      advance_msg(&khrn_prod_msg.render_cleanup_llat);
      *notify_master = true;
   }
}

void _ei(void)
{
	printf("enable interrupts %s %d\n", __FILE__, __LINE__);
}

void _di(void)
{
	printf("disable interrupts %s %d\n", __FILE__, __LINE__);
}

static void display(bool *notify_master)
{
   while (more_msgs(khrn_prod_msg.display, khrn_prod_msg.render_done)) {
      EGL_DISP_SLOT_HANDLE_T slot_handle;
#if defined(__HERA_V3D__)
	  int int_mask = vcos_int_disable();
#else
      _di(); /* getting interrupted here would be bad */
#endif
      khrn_prod_msg.display->display_state = MSG_DISPLAY_STATE_DOING;
      khrn_barrier();
      slot_handle = khrn_prod_msg.display->display_slot_handle;
      khrn_prod_msg.display->display_state = (slot_handle == EGL_DISP_SLOT_HANDLE_INVALID) ?
         MSG_DISPLAY_STATE_DONE_NONE : MSG_DISPLAY_STATE_DONE;
#if defined(__HERA_V3D__)
	  vcos_int_restore(int_mask);
#else
      _ei();
#endif
      if (slot_handle != EGL_DISP_SLOT_HANDLE_INVALID) {
         egl_disp_ready(slot_handle, false);
      }
      advance_msg(&khrn_prod_msg.display);
      *notify_master = true;
   }
}

static void khrn_hw_llat_callback(void)
{
   bool notify_master = false, notify_worker = false;

   /* stuff to keep hardware going (and put stuff on the display) first */
   supply_overflow();
   submit_bin(&notify_master, &notify_worker);
   submit_render(&notify_master, &notify_worker);
   display(&notify_master);

   cleanup_bin_llat(&notify_master); /* make more bin mems available for supply_overflow()/prepare_bin() */
   cleanup_render_llat(&notify_master);
   supply_overflow(); /* might have failed earlier but be able to succeed now with extra bin mems */
#ifndef BRCM_V3D_OPT
   prepare_bin(); /* done submit_bin(), might be able to prepare another */
#endif
   submit_bin(&notify_master, &notify_worker); /* submit any we just prepared if possible */
   supply_overflow(); /* done submit_bin(), try to supply the initial overflow memory */
   prepare_render(); /* same for render... */
   submit_render(&notify_master, &notify_worker);
#ifndef BRCM_V3D_OPT
   prepare_bin(); /* might be able to do some more prep after the submits */
#endif
   prepare_render();

   /* these are needed for messages where we don't do bin/render */
   display(&notify_master);
   cleanup_bin_llat(&notify_master);
   cleanup_render_llat(&notify_master);

   if (notify_master) { khrn_sync_notify_master(); }
   if (notify_worker) { khrn_worker_notify(); }
}

/*
   stuff called from anywhere
*/

void khrn_hw_ready(bool ok, void *callback_data) /* if ok is false, callback will later be called with KHRN_HW_CALLBACK_REASON_OUT_OF_MEM_LLAT/KHRN_HW_CALLBACK_REASON_OUT_OF_MEM */
{
   MSG_T *msg = (MSG_T *)((uint8_t *)callback_data - offsetof(MSG_T, callback_data));
   vcos_assert((msg >= msgs) && (msg < (msgs + MSGS_N)));
   vcos_assert(msg->callback);
   msg->ok = ok;
   khrn_barrier();
   vcos_assert(!msg->ready);
   msg->ready = true;
   khrn_hw_notify_llat();
}

void khrn_hw_fixup_done(bool in_callback, void *callback_data)
{
   UNUSED(callback_data);

   khrn_barrier();
   vcos_assert(!fixup_done);
   fixup_done = true;
   if (!in_callback) {
      khrn_hw_notify_llat();
   }
}

void khrn_hw_notify_llat(void)
{
   khrn_llat_notify(llat_i);
}

static void reset_perf_counters(void)
{
   if (s_perf_counters)
   {
      /* clear the performance counters */
      v3d_write(PCTRC, 0xFFFF);

      /* set up the counters */
      if (s_perf_counters->hw_group_active == 0)
      {
         v3d_write(PCTRS0,  13);
         v3d_write(PCTRS1,  14);
         v3d_write(PCTRS2,  15);
         v3d_write(PCTRS3,  16);
         v3d_write(PCTRS4,  17);
         v3d_write(PCTRS5,  18);
         v3d_write(PCTRS6,  19);
         v3d_write(PCTRS7,  20);
         v3d_write(PCTRS8,  21);
         v3d_write(PCTRS9,  22);
         v3d_write(PCTRS10, 23);
         v3d_write(PCTRS11, 24);
         v3d_write(PCTRS12, 25);
         v3d_write(PCTRS13, 28);
         v3d_write(PCTRS14, 29);
      }
      else if (s_perf_counters->hw_group_active == 1)
      {
         v3d_write(PCTRS0,  0);
         v3d_write(PCTRS1,  1);
         v3d_write(PCTRS2,  2);
         v3d_write(PCTRS3,  3);
         v3d_write(PCTRS4,  4);
         v3d_write(PCTRS5,  5);
         v3d_write(PCTRS6,  6);
         v3d_write(PCTRS7,  7);
         v3d_write(PCTRS8,  8);
         v3d_write(PCTRS9,  9);
         v3d_write(PCTRS10, 10);
         v3d_write(PCTRS11, 11);
         v3d_write(PCTRS12, 12);
         v3d_write(PCTRS13, 26);
         v3d_write(PCTRS14, 27);
      }

      /* enable counters */
      v3d_write(PCTRE, 0x80007FFF);

#ifndef CARBON
/*      if (s_perf_counters->l3c_group_active == 0)
      {
         v3d_write(PM_SEL, 1);
         v3d_write(PM_SEL, 0);
      }
      else if (s_perf_counters->l3c_group_active == 1)
      {
         v3d_write(PM_SEL, 1 | (1 << 2));
         v3d_write(PM_SEL, (1 << 2));
      }*/
#endif
   }
}

void khrn_hw_register_perf_counters(KHRN_DRIVER_COUNTERS_T *counters)
{
   s_perf_counters = counters;
}

void khrn_hw_unregister_perf_counters(void)
{
   s_perf_counters = 0;
}

static void update_perf_counters(void)
{
   if (s_perf_counters)
   {
      if (s_perf_counters->hw_group_active == 0)
      {
         s_perf_counters->hw.hw_0.qpu_cycles_idle                    += v3d_read(PCTR0);
         s_perf_counters->hw.hw_0.qpu_cycles_vert_shade              += v3d_read(PCTR1);
         s_perf_counters->hw.hw_0.qpu_cycles_frag_shade              += v3d_read(PCTR2);
         s_perf_counters->hw.hw_0.qpu_cycles_exe_valid               += v3d_read(PCTR3);
         s_perf_counters->hw.hw_0.qpu_cycles_wait_tmu                += v3d_read(PCTR4);
         s_perf_counters->hw.hw_0.qpu_cycles_wait_scb                += v3d_read(PCTR5);
         s_perf_counters->hw.hw_0.qpu_cycles_wait_vary               += v3d_read(PCTR6);
         s_perf_counters->hw.hw_0.qpu_icache_hits                    += v3d_read(PCTR7);
         s_perf_counters->hw.hw_0.qpu_icache_miss                    += v3d_read(PCTR8);
         s_perf_counters->hw.hw_0.qpu_ucache_hits                    += v3d_read(PCTR9);
         s_perf_counters->hw.hw_0.qpu_ucache_miss                    += v3d_read(PCTR10);
         s_perf_counters->hw.hw_0.tmu_total_quads                    += v3d_read(PCTR11);
         s_perf_counters->hw.hw_0.tmu_cache_miss                     += v3d_read(PCTR12);
         s_perf_counters->hw.hw_0.l2c_hits                           += v3d_read(PCTR13);
         s_perf_counters->hw.hw_0.l2c_miss                           += v3d_read(PCTR14);
      }
      else if (s_perf_counters->hw_group_active == 1)
      {
         s_perf_counters->hw.hw_1.fep_valid_prims                    += v3d_read(PCTR0);
         s_perf_counters->hw.hw_1.fep_valid_prims_no_pixels          += v3d_read(PCTR1);
         s_perf_counters->hw.hw_1.fep_earlyz_clipped_quads           += v3d_read(PCTR2);
         s_perf_counters->hw.hw_1.fep_valid_quads                    += v3d_read(PCTR3);
         s_perf_counters->hw.hw_1.tlb_quads_no_stencil_pass_pixels   += v3d_read(PCTR4);
         s_perf_counters->hw.hw_1.tlb_quads_no_z_stencil_pass_pixels += v3d_read(PCTR5);
         s_perf_counters->hw.hw_1.tlb_quads_z_stencil_pass_pixels    += v3d_read(PCTR6);
         s_perf_counters->hw.hw_1.tlb_quads_all_pixels_zero_cvg      += v3d_read(PCTR7);
         s_perf_counters->hw.hw_1.tlb_quads_all_pixels_nonzero_cvg   += v3d_read(PCTR8);
         s_perf_counters->hw.hw_1.tlb_quads_valid_pixels_written     += v3d_read(PCTR9);
         s_perf_counters->hw.hw_1.ptb_prims_viewport_discarded       += v3d_read(PCTR10);
         s_perf_counters->hw.hw_1.ptb_prims_needing_clip             += v3d_read(PCTR11);
         s_perf_counters->hw.hw_1.pse_prims_reverse_discarded        += v3d_read(PCTR12);
         s_perf_counters->hw.hw_1.vpm_cycles_vdw_stalled             += v3d_read(PCTR13);
         s_perf_counters->hw.hw_1.vpm_cycles_vcd_stalled             += v3d_read(PCTR14);
      }

#ifndef CARBON
/*      if (s_perf_counters->l3c_group_active == 0)
      {
         s_perf_counters->l3c.l3c_read_bw_0 += v3d_read(BW_CNT);
         s_perf_counters->l3c_mem.l3c_mem_read_bw_0 += v3d_read(BW_MEM_CNT);
      }
      else if (s_perf_counters->l3c_group_active == 1)
      {
         s_perf_counters->l3c.l3c_write_bw_1 += v3d_read(BW_CNT);
         s_perf_counters->l3c_mem.l3c_mem_write_bw_1 += v3d_read(BW_MEM_CNT);
      }*/
#endif

      reset_perf_counters();
   }
}

#endif
