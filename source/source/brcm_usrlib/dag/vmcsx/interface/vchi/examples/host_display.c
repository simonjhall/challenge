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

#define NUM_BUFFERS  3
#define DISPLAY_SIZE 800 * 480

/******************************************************************************
Static data
******************************************************************************/

static VCHI_OS_SEMAPHORE_T semaphore;

/******************************************************************************
Static func forward
******************************************************************************/

void host_display_callback(void *callback_param,
                           const VCHI_CALLBACK_REASON_T reason,
                           const void *handle );

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
int32_t host_display( void )
{
   VCHI_BULK_HANDLE_T bulk_handle;
   void *data_rcv[ NUM_BUFFERS ] = NULL;
   VCHI_CONNECTION_FUNC_TABLE_T connections[1] = { a0multififo_host_get_func_table() };

   success = vchi_initialise( connections, 1 );
   assert(success >= 0 );

   success = vchi_os_semaphore_create( &semaphore, OS_SEMAPHORE_BUSYWAIT );
   assert( success >= 0 );

   success = vchi_os_semaphore_obtain( semaphore );
   assert(success >= 0 );

   for( count = 0; count < NUM_BUFFERS; count++ )
   {
      data_rcv[ count ] = malloc( DISPLAY_SIZE );
   }

   //open a bulk channel
   success = vchi_bulk_open( "Display", host_display_callback, NULL, &bulk_handle );
   assert(success >= 0 );

   //send all the buffers to simulate full display bandwidth
   for( count = 0; count < NUM_BUFFERS; count++ )
   {
      //transmit a frame
      success = vchi_bulk_transmit_data(  bulk_handle,
                                          data_rcv[ count ],
                                          DISPLAY_SIZE,
                                          &semaphore );
      assert( success >= 0 );
   }

   while( 1 )
   {
      static uint32_t buffer_num = 0;

      //wait for the semaphore - this indicates a buffer has been sent
      success = vchi_os_semaphore_obtain( semaphore );
      assert(success >= 0 );      

      //fill in the buffer
      memset( data_rcv[ buffer_num ], 0, DISPLAY_SIZE );

      //transmit a frame
      success = vchi_bulk_transmit_data(  bulk_handle,
                                          data_rcv[ buffer_num ],
                                          DISPLAY_SIZE,
                                          &semaphore );
      assert( success >= 0 );

      buffer_num++;
      buffer_num %= NUM_BUFFERS;
   }
}


void host_display_callback(  void *callback_param,
                              const VCHI_CALLBACK_REASON_T reason,
                              const void *handle )
{
   VCHI_OS_SEMAPHORE_T *semaphore = (VCHI_OS_SEMAPHORE_T *)handle;

   assert( reason == VCHI_CALLBACK_DATA_SENT );

   if( semaphore != NULL )
   {
      int32_t success = vchi_os_semaphore_release( *semaphore );
      assert( success >= 0 );
   }   
}

/****************************** End of file ***********************************/