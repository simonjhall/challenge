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

void videocore_loopback_callback(   void *callback_param,
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
int32_t videocore_loopback( void )
{
   VCHI_SERVICE_CREATE_HANDLE_T loopback_service_create_handle;
   VCHI_SERVICE_HANDLE_T loopback_service_handle;
   void *data_rcv = NULL;
   uint32_t data_size = 128;

   success = vchi_initialise( a0fifo_get_instance(), 1 );
   assert(success >= 0 );

   success = vchi_service_create( "LoopBack", 1024, 1024, &loopback_service_create_handle );
   assert( success >= 0 );

   success = vchi_os_semaphore_create( &semaphore, OS_SEMAPHORE_BUSYWAIT );
   assert( success >= 0 );

   success = vchi_os_semaphore_obtain( semaphore );
   assert(success >= 0 );

   success = vchi_service_open( "LoopBack", 1 /*block until service is ready */, &loopback_service_handle, videocore_loopback_callback, &semaphore );
   assert(success >= 0 );

   data_rcv = malloc( data_size );

   while( 1 )
   {
      uint32_t data_read = 0;

      //wait for the message to be returned
      success = vchi_os_semaphore_obtain( semaphore );
      assert(success >= 0 );      

      success = vchi_service_dequeue_msg( loopback_service_handle, data_rcv, data_size, 0, &data_read );
      assert(success >= 0 );

      success = vchi_service_queue_msg( loopback_service_handle, data_send, data_size, NULL, 0 );
      assert(success >= 0 );      
   }
}

void videocore_loopback_callback(   void *callback_param,
                                    const VCHI_CALLBACK_REASON_T reason,
                                    const void *msg_handle )
{
   VCHI_OS_SEMAPHORE_T *semaphore = (VCHI_OS_SEMAPHORE_T *)callback_param;

   assert( reason == VCHI_CALLBACK_REASON_DATA_AVAILABLE );

   if( semaphore != NULL )
   {
      int32_t success = vchi_os_semaphore_release( *semaphore );
      assert( success >= 0 );
   }   
}

/****************************** End of file ***********************************/