/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
/*******************************************************************************
Copyright 2010 Broadcom Corporation.  All rights reserved.

Unless you and Broadcom execute a separate written software license agreement
governing use of this software, this software is licensed to you under the
terms of the GNU General Public License version 2, available at
http://www.gnu.org/copyleft/gpl.html (the "GPL").

Notwithstanding the above, under no circumstances may you combine this software
in any way with any other Broadcom software provided under a license other than
the GPL, without Broadcom's express prior written consent.
*******************************************************************************/
#ifndef _V3D_LINUX_H_
#define _V3D_LINUX_H_
#include <linux/ioctl.h>

#define V3D_DEV_NAME	"v3d"
#define BCM_V3D_MAGIC	'V'
#ifdef __KERNEL__
#define V3D_MEMPOOL_SIZE	SZ_32M
#else
#ifdef BRCM_V3D_OPT
#define V3D_MEMPOOL_SIZE	(0)
#else
#define V3D_MEMPOOL_SIZE	(56*1024*1024)
#endif
#endif

typedef struct {
	void *ptr;		// virtual address
	unsigned int addr;	// physical address
	unsigned int size;
} mem_t;

typedef struct {
	uint32_t v3d_irq_flags;
	uint32_t qpu_irq_flags;
	uint32_t early_suspend;
} gl_irq_flags_t;

typedef enum {
	V3D_JOB_INVALID = 0,
	V3D_JOB_BIN,
	V3D_JOB_REND,
	V3D_JOB_BIN_REND,
	V3D_JOB_LAST
} v3d_job_type_e;

typedef enum {
	V3D_JOB_STATUS_INVALID = 0,
	V3D_JOB_STATUS_READY,
	V3D_JOB_STATUS_RUNNING,
	V3D_JOB_STATUS_SUCCESS,
	V3D_JOB_STATUS_ERROR,
	V3D_JOB_STATUS_NOT_FOUND,
	V3D_JOB_STATUS_TIMED_OUT,
	V3D_JOB_STATUS_SKIP,
	V3D_JOB_STATUS_LAST
} v3d_job_status_e;

typedef struct {
	v3d_job_type_e job_type;
	uint32_t job_id;
	uint32_t v3d_ct0ca;
	uint32_t v3d_ct0ea;
	uint32_t v3d_ct1ca;
	uint32_t v3d_ct1ea;
	uint32_t v3d_vpm_size;

	//sjh
	void *m_pOverspill;
	unsigned int m_overspillSize;
} v3d_job_post_t;

typedef struct {
	uint32_t job_id;
	v3d_job_status_e job_status;
	int32_t timeout;
} v3d_job_status_t;

enum {
	V3D_CMD_GET_MEMPOOL = 0x80,
	V3D_CMD_WAIT_IRQ,
	V3D_CMD_READ_REG,
	V3D_CMD_SOFT_RESET,
	V3D_CMD_TURN_ON,
	V3D_CMD_TURN_OFF,
	V3D_CMD_EARLY_SUSPEND,
#ifdef BRCM_V3D_OPT
	V3D_CMD_POST_JOB,
	V3D_CMD_WAIT_JOB,
	V3D_CMD_FLUSH_JOB,
	V3D_CMD_ACQUIRE,
	V3D_CMD_RELEASE,
#endif
	V3D_CMD_LAST
};

#define V3D_IOCTL_GET_MEMPOOL	_IOR(BCM_V3D_MAGIC, V3D_CMD_GET_MEMPOOL, mem_t)
#define V3D_IOCTL_WAIT_IRQ	_IOR(BCM_V3D_MAGIC, V3D_CMD_WAIT_IRQ, unsigned int)
#define V3D_IOCTL_READ_REG	_IOR(BCM_V3D_MAGIC, V3D_CMD_READ_REG, unsigned int)
#define V3D_IOCTL_TURN_ON	_IO(BCM_V3D_MAGIC, V3D_CMD_TURN_ON)
#define V3D_IOCTL_TURN_OFF	_IO(BCM_V3D_MAGIC, V3D_CMD_TURN_OFF)
#define V3D_IOCTL_SOFT_RESET	_IO(BCM_V3D_MAGIC, V3D_CMD_SOFT_RESET)
#define V3D_IOCTL_EARLY_SUSPEND	_IO(BCM_V3D_MAGIC, V3D_CMD_EARLY_SUSPEND)
#ifdef BRCM_V3D_OPT
#define V3D_IOCTL_POST_JOB		_IOW(BCM_V3D_MAGIC, V3D_CMD_POST_JOB, v3d_job_post_t)
#define V3D_IOCTL_WAIT_JOB		_IOWR(BCM_V3D_MAGIC, V3D_CMD_WAIT_JOB, v3d_job_status_t)
#define V3D_IOCTL_FLUSH_JOB		_IOW(BCM_V3D_MAGIC, V3D_CMD_FLUSH_JOB)
#define V3D_IOCTL_ACQUIRE		_IOW(BCM_V3D_MAGIC, V3D_CMD_ACQUIRE)
#define V3D_IOCTL_RELEASE		_IOW(BCM_V3D_MAGIC, V3D_CMD_RELEASE)
#endif

enum {
   V3D_SUSPEND = 1,
   V3D_RESUME
};

#endif
