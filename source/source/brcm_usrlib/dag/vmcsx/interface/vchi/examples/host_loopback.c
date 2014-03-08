/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :  
Module   :  
File     :  $RCSfile: powerman.c,v $
Revision :  $Revision: #4 $

FILE DESCRIPTION:
=============================================================================*/

/******************************************************************************
Private typedefs
******************************************************************************/

/******************************************************************************
Static data
******************************************************************************/

static VCHI_OS_SEMAPHORE_T semaphore;

/******************************************************************************
Static func forward
******************************************************************************/

void host_loopback_callback(  void *callback_param,
                              const VCHI_CALLBACK_REASON_T reason,
                              const void *msg_handle );

/******************************************************************************
Global functions
******************************************************************************/

/***********************************************************
 * Name: 
 * 
 * Arguments: 
 *       
 *
 * Description: 
 *
 * Returns: 
 *
 ***********************************************************/
int32_t host_loopback( void )
{
   VCHI_SERVICE_HANDLE_T loopback_service_handle;
   void *data_send = NULL;
   void *data_rcv = NULL;
   uint32_t data_size = 128;
   VCHI_CONNECTION_FUNC_TABLE_T connections[1] = { a0multififo_host_get_func_table() };

   success = vchi_initialise( connections, 1 );
   assert(success >= 0 );

   success = vchi_os_semaphore_create( &semaphore, OS_SEMAPHORE_BUSYWAIT );
   assert( success >= 0 );

   success = vchi_os_semaphore_obtain( semaphore );
   assert(success >= 0 );

   success = vchi_service_open( "LoopBack", 1 /*block until connected*/, &loopback_service_handle, host_loopback_callback, &semaphore );
   assert(success >= 0 );

   data_send = malloc( data_size );
   data_rcv = malloc( data_size );

   while( 1 )
   {
      uint32_t data_read = 0;

      success = vchi_service_queue_msg( loopback_service_handle, data_send, data_size, NULL, 0 );
      assert(success >= 0 );

      //wait for the message to be returned
      success = vchi_os_semaphore_obtain( semaphore );
      assert(success >= 0 );

      success = vchi_service_dequeue_msg( loopback_service_handle, data_rcv, data_size, 0, &data_read );
      assert(success >= 0 );

      if( data_read == data_size )
      {
         //compare the messages
         if( 0 != memcmp( data_send, data_rcv, data_size ) )
         {
            assert( 0 );
         }
      }
      else
      {
         assert( 0 );
      }
   }
}

void host_loopback_callback(  void *callback_param,
                              const VCHI_CALLBACK_REASON_T reason,
                              const void *msg_handle )
{
   VCHI_OS_SEMAPHORE_T *semaphore = (VCHI_OS_SEMAPHORE_T *)callback_param;

   assert( reason == vchi_CALLBACK_REASON_DATA_AVAILABLE );

   if( semaphore != NULL )
   {
      int32_t success = vchi_os_semaphore_release( *semaphore );
      assert( success >= 0 );
   }   
}

/****************************** End of file ***********************************/