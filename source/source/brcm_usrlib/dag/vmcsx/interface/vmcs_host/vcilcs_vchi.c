/*=============================================================================
Copyright (c) 2007 Broadcom Europe Limited.
All rights reserved.

Project  :  VMCS-X
Module   :  OpenMAX IL Component Service
File     :  $RCSfile: ilcs.c,v $
Revision :  $Revision: 1.1.2.1 $

FILE DESCRIPTION
OpenMAX IL Component Service functions
=============================================================================*/

/* System includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

/* Project includes */
#include "interface/vmcs_host/khronos/IL/OMX_Component.h"
#include "vc_ilcs_defs.h"
#include "vcilcs.h"
#include "vcilcs_intern.h"

#include "interface/vchi/os/os.h"
#include "interface/vchi/vchi.h"
#include "interface/vchi/common/endian.h"

/******************************************************************************
Private types and defines.
******************************************************************************/

// maximum number of threads that can be waiting at once
// we grab this number of events, so some coordination
// with VC_EVENT_MAX_NUM from vchost.h is required
#define VC_ILCS_MAX_WAITING 2

// maximum number of retries to grab a wait slot,
// before the message is discarded and failure returned.
#define VC_ILCS_WAIT_TIMEOUT 25

// maximum number of concurrent function calls that can
// be going at once.  Each function call requires to copy
// the message data so we can dequeue the message from vchi
// before executing the function, otherwise ILCS may cause
// a deadlock.  Must be larger than ILCS_MAX_WAITING
#define VC_ILCS_MAX_NUM_MSGS 3
#define VC_ILCS_MSG_INUSE_MASK ((1<<VC_ILCS_MAX_NUM_MSGS)-1)

typedef struct {
   int xid;
   void *resp;
   OS_SEMAPHORE_T sem;
} VC_ILCS_WAIT_T;

typedef struct {
   VCHI_SERVICE_HANDLE_T vchi_handle;
   VCOS_THREAD_T thread;

   OS_SEMAPHORE_T component_lock;
   OS_SEMAPHORE_T send_sem; // for correct ordering of control+bulk pairs

   OS_SEMAPHORE_T wait_sem; // for protecting ->wait and ->next_xid
   VC_ILCS_WAIT_T wait[VC_ILCS_MAX_WAITING];
   int next_xid;

   // don't need locking around msg_inuse as only touched by
   // the server thread in ilcs_process_message
   unsigned int msg_inuse;
   unsigned char msg[VC_ILCS_MAX_NUM_MSGS][VCHI_MAX_MSG_SIZE];
} VC_ILCS_GLOBALS_T;

typedef void (*VCIL_FN_T)( void *call, int clen, void *resp, int *rlen );


/******************************************************************************
Private functions in this file.
Define as static.
******************************************************************************/

static void vc_ilcs_task( unsigned argc, void *argv );
static int  vc_ilcs_process_message( int block );
static void vc_ilcs_response( uint32_t xid, unsigned char *msg, int len );
static void vc_ilcs_transmit( uint32_t cmd, uint32_t xid, unsigned char *msg, int len, unsigned char *msg2, int len2 );
static void vc_ilcs_command( uint32_t cmd, uint32_t xid, unsigned char *msg, int len );


/******************************************************************************
Static data.
******************************************************************************/

static VC_ILCS_GLOBALS_T vc_ilcsg;
static void *vc_ilcs_lock = NULL;

static VCIL_FN_T vcilcs_fns[] = {NULL, // response
                                 NULL, // create component

                                 vcil_in_get_component_version,
                                 NULL, // send command
                                 vcil_in_get_parameter,
                                 vcil_in_set_parameter,
                                 vcil_in_get_config,
                                 vcil_in_set_config,
                                 vcil_in_get_extension_index,
                                 vcil_in_get_state,
                                 NULL, // tunnel request
                                 vcil_in_use_buffer,
                                 NULL, // use egl image
                                 NULL, // allocate buffer
                                 vcil_in_free_buffer,
                                 vcil_in_empty_this_buffer,
                                 vcil_in_fill_this_buffer,
                                 NULL, // set callbacks
                                 NULL, // component role enum

                                 NULL, // deinit

                                 vcil_out_event_handler,
                                 vcil_out_empty_buffer_done,
                                 vcil_out_fill_buffer_done,
                                 NULL              // setup camera pools
                                };

/* ----------------------------------------------------------------------
 * initialise host-side OpenMAX IL component service
 * -------------------------------------------------------------------- */
void
vc_vchi_ilcs_init( VCHI_INSTANCE_T initialise_instance,
                   VCHI_CONNECTION_T **connections,
                   uint32_t num_connections )
{
   int32_t success;

   SERVICE_CREATION_T parameters = { MAKE_FOURCC("ILCS"),   // 4cc service code
                                     connections[0],        // passed in fn ptrs
                                     0,                     // tx fifo size (unused)
                                     0,                     // tx fifo size (unused)
                                     0,                     // service callback
                                     0 };                   // callback parameter

   memset( &vc_ilcsg, 0, sizeof(VC_ILCS_GLOBALS_T) );

   // create thread semaphore for blocking
   os_semaphore_create( &vc_ilcsg.component_lock, OS_SEMAPHORE_TYPE_SUSPEND );

   // create semaphore for protecting wait/xid structures
   os_semaphore_create( &vc_ilcsg.wait_sem, OS_SEMAPHORE_TYPE_SUSPEND );
   
   // create semaphore for correct ordering of control+bulk pairs
   os_semaphore_create( &vc_ilcsg.send_sem, OS_SEMAPHORE_TYPE_SUSPEND );

   // open 'ILCS' service
   success = vchi_service_open( initialise_instance, &parameters, &vc_ilcsg.vchi_handle );
   vc_assert( success == 0 );

   success = os_thread_start( &vc_ilcsg.thread, vc_ilcs_task, NULL, 4000, "ILCS_HOST" );
   vc_assert( success == 0 );
}

/* ----------------------------------------------------------------------
 * send a message and wait for reply.
 * repeats continuously, on each connection
 * -------------------------------------------------------------------- */
static void
vc_ilcs_task( unsigned argc, void *argv )
{
   // FIXME: figure out why initial connect() doesn't block...
   //os_sleep( 50 );

   for (;;) {
      vc_ilcs_process_message(1);
   }

   // FIXME: once we implement service close
   //err = OMX_Deinit();
   //assert( err == OMX_ErrorNone );
   //filesys_deregister();
}

/* ----------------------------------------------------------------------
 * check to see if there are any pending messages
 *
 * if there are no messages, return 0
 *
 * otherwise, fetch and process the first queued message (which will
 * be either a command or response from host)
 * -------------------------------------------------------------------- */
static int
vc_ilcs_process_message( int block )
{
   int32_t success;
   void *ptr;
   unsigned char *msg;
   uint32_t i, msg_len, cmd, xid;
 
   success = vchi_msg_peek( vc_ilcsg.vchi_handle, &ptr, &msg_len, block ? VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE : VCHI_FLAGS_NONE);
   vc_assert(!block || !success);
   if ( success != 0 ) return 0; // no more messages

   msg = ptr;
   cmd = vchi_readbuf_uint32( msg );
   xid = vchi_readbuf_uint32( msg + 4 );

   if ( cmd == IL_RESPONSE )
   {
      vc_ilcs_response( xid, msg + 8, msg_len - 8 );
      vchi_msg_remove( vc_ilcsg.vchi_handle );
   }
   else
   {
      // we can only handle commands if we have space to copy the message first
      if(vc_ilcsg.msg_inuse == VC_ILCS_MSG_INUSE_MASK)
      {
         // this shouldn't happen, since we have more msg slots than the
         // remote side is allowed concurrent clients.  We don't cope
         // with this assumption not being true.
         vc_assert(0);
         return 0;
      }

      i = 0;
      while(vc_ilcsg.msg_inuse & (1<<i))
         i++;
      
      vc_ilcsg.msg_inuse |= (1<<i);
      
      memcpy( vc_ilcsg.msg[i], msg + 8, msg_len - 8 );
      vchi_msg_remove( vc_ilcsg.vchi_handle );
      vc_ilcs_command( cmd, xid, vc_ilcsg.msg[i], msg_len - 8);
      
      // mark the message copy as free
      vc_ilcsg.msg_inuse &= ~(1<<i);
   }

   return 1;
}

/* ----------------------------------------------------------------------
 * received response to an ILCS command
 * -------------------------------------------------------------------- */
static void
vc_ilcs_response( uint32_t xid, unsigned char *msg, int len )
{
   VC_ILCS_WAIT_T *wait;
   int i;

   // atomically retrieve given ->wait entry
   os_semaphore_obtain( &vc_ilcsg.wait_sem );
   for (i=0; i<VC_ILCS_MAX_WAITING; i++) {
      wait = &vc_ilcsg.wait[i];
      if ( wait->resp && wait->xid == xid )
         break;
   }
   os_semaphore_release( &vc_ilcsg.wait_sem );

   if ( i == VC_ILCS_MAX_WAITING ) {
      // something bad happened
      vc_assert(0);
      return;
   }

   // extract command from fifo and place in response buffer.
   memcpy( wait->resp, msg, len );

   os_logging_message( "%s: waking waiter %d", __FUNCTION__, i );
   os_semaphore_release( &wait->sem );
}

/* ----------------------------------------------------------------------
 * helper function to transmit an ilcs command/response + payload
 * -------------------------------------------------------------------- */
static void
vc_ilcs_transmit( uint32_t cmd, uint32_t xid, unsigned char *msg, int len, unsigned char *msg2, int len2 )
{
   // could do this in 3 vectors, but hey
   VCHI_MSG_VECTOR_T vec[4];
   int32_t result;
   
   vec[0].vec_base = &cmd;
   vec[0].vec_len  = sizeof(cmd);
   vec[1].vec_base = &xid;
   vec[1].vec_len  = sizeof(xid);
   vec[2].vec_base = msg;
   vec[2].vec_len  = len;
   vec[3].vec_base = msg2;
   vec[3].vec_len  = len2;

   result = vchi_msg_queuev( vc_ilcsg.vchi_handle, vec, 4, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL );     // call to VCHI
   vc_assert(result == 0);
}

/* ----------------------------------------------------------------------
 * received response to an ILCS command
 * -------------------------------------------------------------------- */
static void
vc_ilcs_command( uint32_t cmd, uint32_t xid, unsigned char *msg, int len )
{
   // execute this function call
   unsigned char resp[VC_ILCS_MAX_CMD_LENGTH];
   int rlen = -1;
   VCIL_FN_T *fn;

   if ( cmd >= IL_FUNCTION_MAX_NUM ) {
      vc_assert(0);
      return;
   }

   fn = &vcilcs_fns[ cmd ];
   if ( !fn ) {
      vc_assert(0);
      return;
   }

   // at this point we are executing in ILCS task context (host side).
   // NOTE: this cause vc_ilcs_execute_function() calls from within bowels of openmaxil?
   (*fn)( msg, len, resp, &rlen );

   // make sure rlen has been initialised by the function
   vc_assert( rlen != -1 );

   // if rlen is zero, then we don't send a response
   if ( rlen )
      vc_ilcs_transmit( IL_RESPONSE, xid, resp, rlen, NULL, 0 );
}

/* ----------------------------------------------------------------------
 * send a string to the host side IL component service.  if resp is NULL
 * then there is no response to this call, so we should not wait for one.
 *
 * returns 0 on successful call made, -1 on failure to send call.
 * on success, the response is written to 'resp' pointer
 * -------------------------------------------------------------------- */
int vc_ilcs_execute_function( IL_FUNCTION_T func, void *data, int len, void *data2, int len2, void *bulk, int bulk_len, void *resp )
{
   VC_ILCS_WAIT_T *wait;
   int i, num;

   // the host MUST receive a response
   vc_assert( resp );

   // need to atomically find free ->wait entry
   os_semaphore_obtain( &vc_ilcsg.wait_sem );

   // we try a number of times then give up with an error message
   // rather than just deadlocking
   for (i=0; i<VC_ILCS_WAIT_TIMEOUT; i++) {
      num = 0;

      while( num < VC_ILCS_MAX_WAITING && vc_ilcsg.wait[num].resp )
         num++;

      if ( num < VC_ILCS_MAX_WAITING || i == VC_ILCS_WAIT_TIMEOUT-1)
         break;

      // might be a fatal error if another thread is relying
      // on this call completing before it can complete
      // we'll pause until we can carry on and hope that's sufficient.
      os_semaphore_release( &vc_ilcsg.wait_sem );
      os_sleep( 10 ); // 10 msec

      // if we're the vcilcs thread, then the waiters might need
      // us to handle their response, so try and clear those now
      if(os_thread_is_running(&vc_ilcsg.thread))
         while(vc_ilcs_process_message(0));

      os_logging_message( "%s: wait for sem", __FUNCTION__);
      os_semaphore_obtain( &vc_ilcsg.wait_sem );
   }

   if(num == VC_ILCS_MAX_WAITING)
   {
      // failed to send message.
      vc_assert(0);
      os_semaphore_release( &vc_ilcsg.wait_sem );
      return -1;
   }

   wait = &vc_ilcsg.wait[num];

   wait->resp = resp;
   wait->xid = vc_ilcsg.next_xid++;
   os_semaphore_create( &wait->sem, OS_SEMAPHORE_TYPE_SUSPEND );
   os_semaphore_obtain( &wait->sem );

   // at this point, ->wait is exclusively ours ()
   os_semaphore_release( &vc_ilcsg.wait_sem );

   if(bulk)
      os_semaphore_obtain( &vc_ilcsg.send_sem);

   // write the command header.
   vc_ilcs_transmit( func, wait->xid, data, len, data2, len2 );

   if(bulk)
   {
      int result;
      result = vchi_bulk_queue_transmit( vc_ilcsg.vchi_handle,                // call to VCHI
                                         bulk, bulk_len,
                                         VCHI_FLAGS_BLOCK_UNTIL_QUEUED,
                                         NULL );
      vc_assert(result == 0);
      os_semaphore_release( &vc_ilcsg.send_sem);
   }

   if ( !os_thread_is_running(&vc_ilcsg.thread) ) {

      os_semaphore_obtain( &wait->sem );

   } else {

      // we're the vcilcs task, so wait for, and handle, incoming
      // messages while we're not completed
      for (;;) {
         // wait->sem will not be released until we process the response message
         vc_ilcs_process_message(1);

         // did the last message release wait->sem ?
         if ( !os_semaphore_obtained(&wait->sem) )
            break;
      }
   }

   // safe to do the following - the assignment of NULL is effectively atomic
   os_semaphore_destroy( &wait->sem );
   wait->resp = NULL;
   return 0;
}

// Called on host side to pass a buffer to VideoCore.

OMX_ERRORTYPE vc_ilcs_pass_buffer(IL_FUNCTION_T func, void *reference, OMX_BUFFERHEADERTYPE *pBuffer)
{
   IL_PASS_BUFFER_EXECUTE_T exe;
   IL_BUFFER_BULK_T fixup;
   IL_RESPONSE_HEADER_T resp;
   void *data2 = NULL;
   int len2 = 0;
   void *bulk = NULL;
   int bulk_len = 0;

   if ((func == IL_EMPTY_THIS_BUFFER && pBuffer->pInputPortPrivate == NULL) ||
         (func == IL_FILL_THIS_BUFFER && pBuffer->pOutputPortPrivate == NULL))
   {
      // return this to pass conformance
      // the actual error is using a buffer that hasn't be registered with usebuffer/allocatebuffer
      return OMX_ErrorIncorrectStateOperation;
   }

   exe.reference = reference;
   memcpy(&exe.bufferHeader, pBuffer, sizeof(OMX_BUFFERHEADERTYPE));

   exe.bufferLen = pBuffer->nFilledLen;
   if(pBuffer->nFlags & OMX_BUFFERFLAG_EXTRADATA)
   {
      OMX_U8 *ptr = pBuffer->pBuffer + pBuffer->nOffset + pBuffer->nFilledLen + 3;
      OMX_OTHER_EXTRADATATYPE *extra = (OMX_OTHER_EXTRADATATYPE *) (((uint32_t) ptr) & ~3);
      while(extra->eType != OMX_ExtraDataNone)
         extra = (OMX_OTHER_EXTRADATATYPE *) (((uint8_t *) extra) + extra->nSize);

      exe.bufferLen = (((uint8_t *) extra) + extra->nSize) - (pBuffer->pBuffer + pBuffer->nOffset);
   }

   if(exe.bufferLen)
   {
      if(exe.bufferLen + sizeof(IL_PASS_BUFFER_EXECUTE_T) <= VC_ILCS_MAX_INLINE)
      {
         exe.method = IL_BUFFER_INLINE;
         data2 = pBuffer->pBuffer + pBuffer->nOffset;
         len2 = exe.bufferLen;
      }
      else
      {
         const uint8_t *start = pBuffer->pBuffer + pBuffer->nOffset;
         const uint8_t *end   = start + exe.bufferLen;
         const uint8_t *round_start = (const OMX_U8*)VCHI_BULK_ROUND_UP(start);
         const uint8_t *round_end   = (const OMX_U8*)VCHI_BULK_ROUND_DOWN(end);

         exe.method = IL_BUFFER_BULK;
         bulk = (void *) round_start;
         bulk_len = round_end - round_start;

         if((fixup.headerlen = round_start - start) > 0)
            memcpy(fixup.header, start, fixup.headerlen);

         if((fixup.trailerlen = end - round_end) > 0)
            memcpy(fixup.trailer, round_end, fixup.trailerlen);

         data2 = &fixup;
         len2 = sizeof(IL_BUFFER_BULK_T);
      }
   }
   else
   {
      exe.method = IL_BUFFER_NONE;
   }

   if(vc_ilcs_execute_function(func, &exe, sizeof(IL_PASS_BUFFER_EXECUTE_T), data2, len2, bulk, bulk_len, &resp) < 0)
      return OMX_ErrorHardware;

   return resp.err;
}

// receive a buffer from VideoCore either from the message bytes
// or by a bulk transfer receieve
OMX_BUFFERHEADERTYPE *vc_ilcs_receive_buffer(void *call, int clen, OMX_COMPONENTTYPE **pComp)
{
   IL_PASS_BUFFER_EXECUTE_T *exe = call;
   OMX_BUFFERHEADERTYPE *pHeader = exe->bufferHeader.pInputPortPrivate;
   OMX_U8 *pBuffer = pHeader->pBuffer;
   OMX_PTR *pAppPrivate = pHeader->pAppPrivate;
   OMX_PTR *pPlatformPrivate = pHeader->pPlatformPrivate;
   OMX_PTR *pInputPortPrivate = pHeader->pInputPortPrivate;
   OMX_PTR *pOutputPortPrivate = pHeader->pOutputPortPrivate;

   vc_assert(pHeader);
   memcpy(pHeader, &exe->bufferHeader, sizeof(OMX_BUFFERHEADERTYPE));

   *pComp = exe->reference;

   pHeader->pBuffer = pBuffer;
   pHeader->pAppPrivate = pAppPrivate;
   pHeader->pPlatformPrivate = pPlatformPrivate;
   pHeader->pInputPortPrivate = pInputPortPrivate;
   pHeader->pOutputPortPrivate = pOutputPortPrivate;
   
   if(exe->method == IL_BUFFER_BULK)
   {
      IL_BUFFER_BULK_T *fixup = (IL_BUFFER_BULK_T *) (exe+1);

      // receiving a bulk transfer from videocore to host
      uint8_t *start = pHeader->pBuffer + pHeader->nOffset;
      uint8_t *end   = start + exe->bufferLen;
      int32_t bulk_len = exe->bufferLen - fixup->headerlen - fixup->trailerlen;
      int32_t result;
      
      vc_assert(VCHI_BULK_ALIGNED(pHeader->pBuffer));
      vc_assert(clen == sizeof(IL_PASS_BUFFER_EXECUTE_T) + sizeof(IL_BUFFER_BULK_T));

      result = vchi_bulk_queue_receive( vc_ilcsg.vchi_handle,              // call to VCHI
                                        start + fixup->headerlen,
                                        bulk_len,
                                        VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE,
                                        NULL );
      vc_assert(result == 0);
      
      if (fixup->headerlen)
         memcpy(start, fixup->header, fixup->headerlen);
      if (fixup->trailerlen)
         memcpy(end-fixup->trailerlen, fixup->trailer, fixup->trailerlen);
   }
   else if(exe->method == IL_BUFFER_INLINE)
   {
      IL_BUFFER_INLINE_T *buffer = (IL_BUFFER_INLINE_T *) (exe+1);

      vc_assert(clen == sizeof(IL_PASS_BUFFER_EXECUTE_T) + exe->bufferLen);

      memcpy(pBuffer+pHeader->nOffset, buffer->buffer, exe->bufferLen);
   }
   else if(exe->method == IL_BUFFER_NONE)
   {
      vc_assert(clen == sizeof(IL_PASS_BUFFER_EXECUTE_T));
   }
   else
   {
      vc_assert(0);
   }

   return pHeader;
}

void
vc_ilcs_obtain_component_lock( void )
{
   os_semaphore_obtain( &vc_ilcsg.component_lock );
}

void
vc_ilcs_release_component_lock( void )
{
   os_semaphore_release( &vc_ilcsg.component_lock );
}
