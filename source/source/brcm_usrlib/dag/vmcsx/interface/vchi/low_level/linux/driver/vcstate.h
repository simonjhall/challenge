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


#ifndef VC_STATE_HEADER
#define VC_STATE_HEADER

typedef enum {
	VC_POWER_STATE_OFF       = 0,
	VC_POWER_STATE_HIBERNATE = 1,
	VC_POWER_STATE_RUNNING   = 2
} VC_POWER_STATE_T;

// Identifiers for applications.
typedef enum {
	VC_APPLICATION_RINGER,
	VC_APPLICATION_FILEMGR,
	VC_APPLICATION_IMVIEWER,
	VC_APPLICATION_IMEDITOR,
	VC_APPLICATION_MPLAYER,
	VC_APPLICATION_MSTILLS,
	VC_APPLICATION_MRECORDER,
	VC_APPLICATION_DOWNLOAD,
	VC_APPLICATION_PCMPLAYER,
	VC_APPLICATION_USBMGR,
	// any new applications MUST be added here just above VC_APPLICATION_MAXNUM!
	// new applications MUST be added to the app_names table in vcstate.c
	VC_APPLICATION_MAXNUM
} VC_APPLICATION_T;

// Identifiers for different services.
typedef enum {
	VC_SERVICE_GENCMD,
	VC_SERVICE_DISPMAN,
	VC_SERVICE_FILESYS,
	VC_SERVICE_HOSTREQ,
	VC_SERVICE_DSERV,
	VC_SERVICE_MAXNUM
} VC_SERVICE_T;

// Should be called before first booting VideoCore.
int vc_state_init(void);

// NOTE: if you wish to use vc_state_set_power_state then you should avoid sending
// "power_control videocore <n>" gencmds directly as the state information is not updated when
// direct gencmds are sent.

// Prepare VideoCore to go into hibernation or power-down, prior to switching the RUN/HIB pins.
// If it's already hibernating, after setting the RUN/HIB pins back to running, can be used to
// bring VMCS out of hibernation.
// This will also start/stop all the services as necessary.
// Return non-zero for an error.
int vc_state_set_power_state(VC_POWER_STATE_T state);

// Return the current power state of VideoCore.
VC_POWER_STATE_T vc_state_get_power_state(void);

// Return >= 0 if the service given is running (returns its service number).
int vc_state_service_running(VC_SERVICE_T service);

// NOTE: if you wish to use vc_state_load/end_application then you should not send any
// load/end_application gencmds directly as the state is not recorded for direct gencmds.

// Return name to use to load an application.
const char *vc_state_application_name(VC_APPLICATION_T app);
// Start an application. Return its task id or < 0 for an error.
int vc_state_load_application(VC_APPLICATION_T app);
// End an application. Return non-zero for an error.
int vc_state_end_application(int task_id);
// Return the task_id of an application (assumes only one instance of any app).
// Return -1 for error.
int vc_state_application_task_id(VC_APPLICATION_T app);
// Return non-zero if the application with the given task_id is still running.
int vc_state_application_running(int task_id);

#endif
