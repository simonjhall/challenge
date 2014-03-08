/*********************************************************************
(C) Copyright (c) 2002-2003 ARC International;  Santa Cruz, CA 95060
*********************************************************************/

#ifndef __HOSTCOM_H__
#define __HOSTCOM_H__ 1

#include <stddef.h>

#ifdef __CPLUSPLUS__
extern "C" {
#endif

/* BIOS calls and entry points */

#define _fc_fi_open		0
#define _fc_fi_close		1
#define _fc_fi_read		2
#define _fc_fi_write		3
#define _fc_fi_seek		4
#define _fc_fi_unlink		5
#define _fc_fi_isatty		6
#define _fc_fi_tmpnam		7
#define _fc_pr_getenv		8
#define _fc_pr_clock		9
#define _fc_pr_time		10
#define _fc_fi_rename		11
#define _fc_pr_argc		12
#define _fc_pr_argv		13
#define _fc_pr_retcode		14
#define _fc_fi_access		15
#define _fc_pr_getpid		16
#define _fc_fi_getcwd		17
#define _fc_user_hostlink	18
#define _fc_max_funcs		19

extern volatile char* _hl_packChar(volatile char* buf, char x);
extern volatile char* _hl_unPackChar(volatile char* buf, char *x);
extern volatile char* _hl_packShort(volatile char* buf, short x);
extern volatile char* _hl_unPackShort(volatile char* buf, short *x);
extern volatile char* _hl_packInt(volatile char* buf, unsigned long x);
extern volatile char* _hl_unPackInt(volatile char* buf, unsigned long *x);
extern volatile char* _hl_packString(volatile char* buf, char *s, int len);
extern volatile char* _hl_unPackString(volatile char* buf, char *s, int *len);
extern volatile char* _hl_packStringZ(volatile char* buf, char *s);
extern volatile char* _hl_unPackStringZ(volatile char* buf, char *x);
extern int _hl_unPackStringLen(volatile char* buf);
extern volatile char *_hl_message(int opcode, char *format, ...);
extern int _user_hostlink(int vendor, int opcode, char *format, ...);

/*
A target program can send a request to a user-written service provider
attached to the debugger.  The request can take practically any form as
long as the target program and the service provider agree on the format
of the request.  The request is managed by the hostlink facility.
See the comments in file inc/dbghl.h for documentation on the
user-extendable hostlink facility.
*/


/*
 * Hostlink POSIX support functions.
 */
extern char * _hl_getcwd(char *cwd, int maxlen);
extern int _hl_access(const char* path, int mode);
extern int _hl_close(int handle);
extern int _hl_isatty(int handle);
extern int _hl_open(const char* path, int flags, int mode);
extern int _hl_read(int handle, char *buffer, unsigned int length);
extern int _hl_rename(const char *oldName, const char *newName);
extern int _hl_write(int handle, const char *buffer, unsigned int length);
extern long _hl_lseek(int handle, long offset, int relative_to);

#ifdef __CPLUSPLUS__
}
#endif

#endif
