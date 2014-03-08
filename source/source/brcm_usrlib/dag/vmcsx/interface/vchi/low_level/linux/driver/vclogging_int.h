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

#ifndef VCLOGGING_INT_H
#define VCLOGGING_INT_H

#define LOGGING_SYNC 'VLOG'
#define N_VCLIB_LOGS  3  //Does not include the task switch log

typedef enum {
   LOGGING_FIFO_LOG = 1,
   LOGGING_ASSERTION_LOG,
   LOGGING_TASK_LOG,
} LOG_FORMAT_T;

typedef struct {
   LOG_FORMAT_T type;
   void *log;
} LOG_DESCRIPTOR_T;

typedef struct {
   unsigned long time;
   unsigned short seq_num;
   unsigned short size;
} logging_fifo_log_msg_header_t;

typedef struct {
   char name[4];
   unsigned char *start;
   unsigned char *end;
   unsigned char *ptr;
   unsigned char *next_msg;
   logging_fifo_log_msg_header_t msg_header;
} logging_fifo_log_t;

typedef struct {
   char name[4];
   unsigned short nitems, item_size;
   unsigned long available;
   unsigned char *data;
} logging_array_log_t;


// The header at the start of the log may be one of a number of different types,
// but they all start with a common portion:

typedef unsigned long LOGGING_LOG_TYPE_T;
#define LOGGING_LOG_TYPE_VCLIB ((LOGGING_LOG_TYPE_T) 0)
#define LOGGING_LOG_TYPE_VMCS  ((LOGGING_LOG_TYPE_T) 1)

typedef struct {
   unsigned long sync;
   LOGGING_LOG_TYPE_T type;
   unsigned long version;
   void *self;
} logging_common_header_t;

typedef struct {
   logging_common_header_t common;
   unsigned long stc;
   int task_switch_log_size;
   unsigned char *task_switch_log;
   unsigned long n_logs;
   LOG_DESCRIPTOR_T assertion_log;
   LOG_DESCRIPTOR_T message_log;
   LOG_DESCRIPTOR_T task_log;

} logging_header_t;

#endif // VCLOGGING_INT_H
