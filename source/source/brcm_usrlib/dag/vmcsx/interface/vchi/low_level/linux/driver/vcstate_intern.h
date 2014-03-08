/*****************************************************************************
*  Copyright 2006 - 2007 Broadcom Corporation.  All rights reserved.
*
*  Unless you and Broadcom execute a separate written software license
*  agreement governing use of this software, this software is licensed to you
*  under the terms of the GNU General Public License version 2, available at
*  http://www.gnu.org/copyleft/gpl.html (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a
*  license other than the GPL, without Broadcom's express prior written
*  consent.
*
*****************************************************************************/


#ifndef VC_STATE_INTERN_HEADER
#define VC_STATE_INTERN_HEADER

#include "vcstate.h"

#define VC_MAX_LOADED_APPS 1

typedef struct {
	VC_APPLICATION_T app;
	int task_id;
} VC_LOADED_APP_T;

typedef struct {
	VC_POWER_STATE_T power_state;
	int num_loaded_apps;
	VC_LOADED_APP_T loaded_apps[VC_MAX_LOADED_APPS];
} VC_STATE_T;

extern VC_STATE_T vc_state;

#endif
