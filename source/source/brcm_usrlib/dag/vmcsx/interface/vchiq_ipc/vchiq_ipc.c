#include "interface/khronos/common/khrn_int_common.h"
#include "interface/khronos/common/khrn_int_ids.h"

#include "interface/khronos/common/khrn_client.h"
//#include "interface/khronos/common/khrn_client_rpc.h"

#include "interface/vchi/vchi.h"


#include "interface/khronos/common/selfhosted/myshared.h"


#include <assert.h>
#include <string.h>

IPC_shared_mem *myIPC;

const VCHI_CONNECTION_API_T *
single_get_func_table( void )
{
   return NULL;
}

VCHI_CONNECTION_T * vchi_create_connection( const VCHI_CONNECTION_API_T * function_table,
                                            const VCHI_MESSAGE_DRIVER_T * low_level)
{
   return NULL;
}


//(referred from khronos_main.o).
int32_t vchi_connect( VCHI_CONNECTION_T **connections,
						  const uint32_t num_connections,
						  VCHI_INSTANCE_T instance_handle )
{
	int32_t				success = 0;

	return success;
}

// (referred from khronos_main.o).
int32_t vchi_initialise( VCHI_INSTANCE_T *instance_handle )

{
	int32_t	success = 0; 

	return success;

}



//(referred from khronos_main.o).
const VCHI_MESSAGE_DRIVER_T *
vchi_mphi_message_driver_func_table( void )
{
   return NULL;
}

//(referred from khrn_client_rpc_selfhosted_server.o).
int32_t vchi_service_open( VCHI_INSTANCE_T instance_handle,
							   SERVICE_CREATION_T *setup,
							   VCHI_SERVICE_HANDLE_T *handle)
{

	int32_t success = 0;

	return success;

}

//(referred from khronos_server.o).
int32_t vchi_service_create( VCHI_INSTANCE_T instance_handle,
								 SERVICE_CREATION_T *setup,
								 VCHI_SERVICE_HANDLE_T *handle )
{
	int32_t	success = 0; 

	if (setup->service_id == MAKE_FOURCC("KHRN"))
		myIPC->khrn_callback = setup->callback;

	return success;


}

// implementation for IPC
// (referred from khrn_dispatch.o).
int32_t vchi_bulk_queue_receive( VCHI_SERVICE_HANDLE_T handle,
									 void * data_dst,
									 uint32_t data_size,
									 VCHI_FLAGS_T flags,
									 void * bulk_handle )
{
	int32_t	success = 0; 
	uint32_t events,mydata_size;
	void *mydata;

	RPC_IPC_LOG("vchi_bulk_queue_receive start: handle %x data_dst %x data_size %d",handle,data_dst,data_size);
	
	assert(flags==VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE);
	assert(bulk_handle==NULL);

	vcos_mutex_lock(&myIPC->lock);
	
	if (myIPC->client_data_bulk_available_flag == 0)
	{
		vcos_mutex_unlock(&myIPC->lock);	
		vcos_event_flags_get(&myIPC->client_data_status,IPC_DATA_STATUS_BULK_READY,VCOS_OR_CONSUME,VCOS_SUSPEND,&events);
		vcos_mutex_lock(&myIPC->lock); 
	}

	//mydata_size  = (data_size>myIPC->bulk_buffer[1])?myIPC->bulk_buffer[1]:data_size;
	mydata_size  = myIPC->bulk_buffer[1];
   mydata = (void*)myIPC->bulk_buffer[4];
	
	assert(data_size == mydata_size);
	//if (data_size != mydata_size)
	//	assert(0);
	memcpy(data_dst, mydata,mydata_size);

	//clear flag and event
	myIPC->client_data_bulk_available_flag = 0;
	vcos_event_flags_get(&myIPC->client_data_status,IPC_DATA_STATUS_BULK_READY,VCOS_OR_CONSUME,VCOS_NO_SUSPEND,&events);

	//release lock
	vcos_semaphore_post(&myIPC->bulk_write_lock);

	vcos_mutex_unlock(&myIPC->lock);
	
	RPC_IPC_LOG("vchi_bulk_queue_receive end: handle %x data_dst %x data_size %d",handle,data_dst,data_size);

	return success;


}


//data is on khdispatch_workspace
int32_t vchi_bulk_queue_transmit( VCHI_SERVICE_HANDLE_T handle,
									  const void * data,
									  uint32_t data_size,
									  VCHI_FLAGS_T flags,
									  void * bulk_handle )
{
	int32_t	success = 0;

	assert(flags==VCHI_FLAGS_BLOCK_UNTIL_DATA_READ);
	assert(bulk_handle==NULL);
	
	RPC_IPC_LOG("vchi_bulk_queue_transmit start: handle %x data %x data_size %d",handle,data,data_size);
	
	vcos_semaphore_wait(&myIPC->bulk_write_lock);

	vcos_mutex_lock(&myIPC->lock);

	myIPC->bulk_buffer[0] = data_size;

#ifdef __linux__	
	memcpy(&myIPC->bulk_buffer[5],data,data_size);
    myIPC->bulk_buffer[4] = (int32_t) &myIPC->bulk_buffer[5];
#else
	myIPC->bulk_buffer[4] = (int32_t) data;
#endif
    
    myIPC->server_data_bulk_available_flag = 1;
	vcos_event_flags_set(&myIPC->server_data_status,IPC_DATA_STATUS_BULK_READY,VCOS_OR);

	vcos_mutex_unlock(&myIPC->lock);
	
	RPC_IPC_LOG("vchi_bulk_queue_transmit end: handle %x data %x data_size %d",handle,data,data_size);
	return success;


}

int32_t vchi_msg_dequeue( VCHI_SERVICE_HANDLE_T handle,
							  void *data,
							  uint32_t max_data_size_to_read,
							  uint32_t *actual_msg_size,
							  VCHI_FLAGS_T flags )
{
	int32_t	success = 0; 

	//should never be called
	assert(0);	
	return success;

}

int32_t vchi_held_msg_release( VCHI_HELD_MSG_T *message )
{
	int32_t	success = 0; 
	uint32_t events;

	RPC_IPC_LOG("vchi_held_msg_release start: message %x",message);
		
	vcos_mutex_lock(&myIPC->lock);
		
	myIPC->client_data_ctrl_available_flag = 0;
	vcos_event_flags_get(&myIPC->client_data_status,IPC_DATA_STATUS_CTRL_READY,VCOS_OR_CONSUME,VCOS_NO_SUSPEND,&events);

	vcos_semaphore_post(&myIPC->client_write_lock); //

	vcos_mutex_unlock(&myIPC->lock);
	
	RPC_IPC_LOG("vchi_held_msg_release end: message %x",message);
	return success;

}

int32_t vchi_msg_hold( VCHI_SERVICE_HANDLE_T handle,
						   void **data,
						   uint32_t *msg_size,
						   VCHI_FLAGS_T flags,
						   VCHI_HELD_MSG_T *message_handle )
{
	int32_t	success = 0; 
	uint32_t events;

	//RPC_IPC_LOG("vchi_msg_hold start: handle %x data %x msg_size %d message %x",handle,*data,*msg_size,message_handle);
	
	assert(flags==VCHI_FLAGS_NONE);

	vcos_mutex_lock(&myIPC->lock);
	
	if (myIPC->client_data_ctrl_available_flag == 0)
	{
		vcos_mutex_unlock(&myIPC->lock);
		return -1;
	}
	
	*data     = (void*)myIPC->buffer[4];
	*msg_size = myIPC->buffer[1];
	
	vcos_mutex_unlock(&myIPC->lock);
	
	RPC_IPC_LOG("vchi_msg_hold end: handle %x data %x msg_size %d message %x",handle,*data,*msg_size,message_handle);
	return success;

}

//we need to copy them to shared memory buffer
//since these data could be on some function's stack
int32_t vchi_msg_queue( VCHI_SERVICE_HANDLE_T handle,
							const void * data,
							uint32_t data_size,
							VCHI_FLAGS_T flags,
							void * msg_handle )
{
	int32_t				success = 0; 

	RPC_IPC_LOG("vchi_msg_queue start: handle %x data %x msg_size %d msg_handle %x",handle,data,data_size,msg_handle);

	assert(flags==VCHI_FLAGS_BLOCK_UNTIL_QUEUED);

	vcos_semaphore_wait(&myIPC->server_write_lock);
	
	vcos_mutex_lock(&myIPC->lock);

	myIPC->buffer[0] = data_size;
	
	memcpy(&myIPC->buffer[5],data,data_size);
    myIPC->buffer[4] = (int32_t) &myIPC->buffer[5];
    
    myIPC->server_data_ctrl_available_flag = 1;
	vcos_event_flags_set(&myIPC->server_data_status,IPC_DATA_STATUS_CTRL_READY,VCOS_OR);

	vcos_mutex_unlock(&myIPC->lock);
	
	RPC_IPC_LOG("vchi_msg_queue end: handle %x data %x msg_size %d msg_handle %x",handle,data,data_size,msg_handle);
	return success;


}

int32_t vchi_msg_queuev( VCHI_SERVICE_HANDLE_T handle,
							 VCHI_MSG_VECTOR_T * vector,
							 uint32_t count,
							 VCHI_FLAGS_T flags,
							 void *msg_handle )
{
	int32_t				success = 0; 
	char * ptr;
	int i, msg_size;

	RPC_IPC_LOG("vchi_msg_queuev start: handle %x vector %x count %d message %x",handle,vector,count,msg_handle);

	assert(flags==VCHI_FLAGS_BLOCK_UNTIL_QUEUED);
	
	vcos_semaphore_wait(&myIPC->server_write_lock);

	vcos_mutex_lock(&myIPC->lock);
	
	ptr = (char*) &myIPC->buffer[5];
	msg_size = 0;
	for (i = 0; i < count; i++)
	  {
		assert (vector[i].vec_base);
		assert (vector[i].vec_len);
		
		memcpy (ptr, vector[i].vec_base, vector[i].vec_len);
		msg_size += vector[i].vec_len;
		ptr += vector[i].vec_len;
	  }
	
	myIPC->buffer[0] = msg_size;
    myIPC->buffer[4] = (int32_t) &myIPC->buffer[5];
	
    myIPC->server_data_ctrl_available_flag = 1;
	vcos_event_flags_set(&myIPC->server_data_status,IPC_DATA_STATUS_CTRL_READY,VCOS_OR);
	
	vcos_mutex_unlock(&myIPC->lock);
	
	RPC_IPC_LOG("vchi_msg_queuev end: handle %x vector %x count %d message %x",handle,vector,count,msg_handle);

	return success;

}

// server calls this in its own thread like in RPC DIRECT mode
// semaphore is created and locked on client side, surface->avaible_buffers in egl_client.c
// make sure to implement "vcos_named_semaphore_xxx", which khronos_platform_semaphore_xxx is about
int32_t ipc_server_send_async(uint32_t command, uint64_t pid, uint32_t sem)
{
	RPC_IPC_LOG("ipc_server_send_async start: command %d pid %x %x sem %x",command,(int)pid,(int)(pid>>32),sem);

	if (sem != KHRN_NO_SEMAPHORE) {
	   int name[3];
	
	   name[0] = (int)pid; name[1] = (int)(pid >> 32); name[2] = (int)sem;
	
	   if (command == ASYNC_COMMAND_DESTROY) {
		  /* todo: destroy */
	   } else {
		  PLATFORM_SEMAPHORE_T s;
		  if (khronos_platform_semaphore_create(&s, name, 1) == KHR_SUCCESS) {
			 switch (command) {
			 case ASYNC_COMMAND_WAIT:
				/* todo: i don't understand what ASYNC_COMMAND_WAIT is for, so this
				 * might be completely wrong */
				khronos_platform_semaphore_acquire(&s);
				break;
			 case ASYNC_COMMAND_POST:
				khronos_platform_semaphore_release(&s);
				break;
			 default:
				UNREACHABLE();
			 }
			 khronos_platform_semaphore_destroy(&s);
		  }
	   }
	}

	RPC_IPC_LOG("ipc_server_send_async end: command %d pid %ld sem %d",command,pid,sem);
	return 0;
}


// (referred from khrn_client_rpc_selfhosted_server.o).

int32_t client_vchi_bulk_queue_receive( VCHI_SERVICE_HANDLE_T handle,
									 void * data_dst,
									 uint32_t data_size,
									 VCHI_FLAGS_T flags,
									 void * bulk_handle )
{
	int32_t	success = 0; 
	uint32_t events;

	RPC_IPC_LOG("client_vchi_bulk_queue_receive start:handle %x data_dst %x data_size %d",handle,data_dst,data_size);
	
	assert(flags==VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE);
	assert(bulk_handle==NULL);
	
	vcos_mutex_lock(&myIPC->lock);
	
	if (myIPC->server_data_bulk_available_flag == 0)
	{
		vcos_mutex_unlock(&myIPC->lock);
		vcos_event_flags_get(&myIPC->server_data_status,(VCOS_UNSIGNED)IPC_DATA_STATUS_BULK_READY,VCOS_OR_CONSUME,VCOS_SUSPEND,&events);
		vcos_mutex_lock(&myIPC->lock); 
	}

	//bulk should be arlready in shared memory buffer
	os_assert(data_size==myIPC->bulk_buffer[0]);
	
	myIPC->bulk_buffer[3] = IPC_SERVER_SIG;;
	
	//clear flag and event
	myIPC->server_data_bulk_available_flag = 0;
	vcos_event_flags_get(&myIPC->server_data_status,(VCOS_UNSIGNED)IPC_DATA_STATUS_BULK_READY,VCOS_OR_CONSUME,VCOS_NO_SUSPEND,&events);

	vcos_mutex_unlock(&myIPC->lock);

	RPC_IPC_LOG("client_vchi_bulk_queue_receive end: handle %x data_dst %x data_size %d",handle,data_dst,data_size);
	
	return success;


}



int32_t client_vchi_bulk_queue_transmit( VCHI_SERVICE_HANDLE_T handle,
									  const void * data_src,
									  uint32_t data_size,
									  VCHI_FLAGS_T flags,
									  void * bulk_handle )
{
	int32_t				success = 0;
	
	RPC_IPC_LOG("client_vchi_bulk_queue_transmit start: handle %x data_src %x data_size %d",handle,data_src,data_size);

	assert(flags==VCHI_FLAGS_BLOCK_UNTIL_DATA_READ);
	assert(bulk_handle==NULL);

	vcos_mutex_lock(&myIPC->lock);

    myIPC->client_data_bulk_available_flag = 1;
	vcos_event_flags_set(&myIPC->client_data_status,IPC_DATA_STATUS_BULK_READY,VCOS_OR);

	vcos_mutex_unlock(&myIPC->lock);
	
	RPC_IPC_LOG("client_vchi_bulk_queue_transmit end: handle %x data_src %x data_size %d",handle,data_src,data_size);
	return success;


}


int32_t client_vchi_held_msg_release( VCHI_HELD_MSG_T *message )
{
	int32_t				success = 0;

	RPC_IPC_LOG("client_vchi_held_msg_release start: message %x",message);

	vcos_mutex_lock(&myIPC->lock);
	
	vcos_semaphore_post(&myIPC->server_write_lock); //
	
	vcos_mutex_unlock(&myIPC->lock);
	
	RPC_IPC_LOG("client_vchi_held_msg_release end: message %x",message);
	return success;


}


int32_t client_vchi_msg_dequeue( VCHI_SERVICE_HANDLE_T handle,
							  void *data,
							  uint32_t max_data_size_to_read,
							  uint32_t *actual_msg_size,
							  VCHI_FLAGS_T flags )
{
	int32_t				success = 0; 

	assert(flags==VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE);

	assert(0); //should never be called
	
	return success;


}

int32_t client_vchi_msg_hold( VCHI_SERVICE_HANDLE_T handle,
						   void **data,
						   uint32_t *msg_size,
						   VCHI_FLAGS_T flags,
						   VCHI_HELD_MSG_T *message_handle )
{
	int32_t				success = 0; 
	int32_t events;

	RPC_IPC_LOG("client_vchi_msg_hold start: handle %x data %x msg_size %d",handle,*data,*msg_size);

	assert(flags==VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE);
	
	vcos_mutex_lock(&myIPC->lock);
	
	if (myIPC->server_data_ctrl_available_flag == 0)
	{
		vcos_mutex_unlock(&myIPC->lock);
		vcos_event_flags_get(&myIPC->server_data_status,(VCOS_UNSIGNED)IPC_DATA_STATUS_CTRL_READY,VCOS_OR_CONSUME,VCOS_SUSPEND,&events);
		vcos_mutex_lock(&myIPC->lock); 
	}

	//message should be arlready in shared memory buffer
	myIPC->buffer[3] = IPC_SERVER_SIG;

	//clear flag and event
	myIPC->server_data_ctrl_available_flag = 0;
	vcos_event_flags_get(&myIPC->server_data_status,(VCOS_UNSIGNED)IPC_DATA_STATUS_CTRL_READY,VCOS_OR_CONSUME,VCOS_NO_SUSPEND,&events);

	vcos_mutex_unlock(&myIPC->lock);
	
	RPC_IPC_LOG("client_vchi_msg_hold end: handle %x data %x msg_size %d",handle,*data,*msg_size);
	
	return success;

}

int32_t client_vchi_msg_queue( VCHI_SERVICE_HANDLE_T handle,
							const void * data,
							uint32_t data_size,
							VCHI_FLAGS_T flags,
							void * msg_handle )
{
	int32_t				success = 0; 

	RPC_IPC_LOG("client_vchi_msg_queue start: handle %x data %x data_size %d",handle,data,data_size);

	assert(flags==VCHI_FLAGS_BLOCK_UNTIL_QUEUED);

	vcos_mutex_lock(&myIPC->lock);

    assert(data_size <= MERGE_BUFFER_SIZE);
    
    myIPC->client_data_ctrl_available_flag = 1;
	vcos_event_flags_set(&myIPC->client_data_status,IPC_DATA_STATUS_CTRL_READY,VCOS_OR);

	vcos_mutex_unlock(&myIPC->lock);

	RPC_IPC_LOG("client_vchi_msg_queue end: handle %x data %x data_size %d",handle,data,data_size);
    
	return success;


}

static void  ipc_vchi_server(unsigned a, void *b)
{
	int32_t success;

	myIPC = (IPC_shared_mem*)vcos_malloc(sizeof(IPC_shared_mem), NULL);
	os_assert(myIPC);

	memset(myIPC,0,sizeof(IPC_shared_mem));

	success = vcos_mutex_create( &myIPC->lock, NULL);
	os_assert(success==0);
	
	success = vcos_semaphore_create( &myIPC->client_write_lock, NULL, 1);
	os_assert(success==0);
	
	success = vcos_semaphore_create( &myIPC->bulk_write_lock, NULL, 1);
	os_assert(success==0);
	
	success = vcos_semaphore_create( &myIPC->server_write_lock, NULL, 1);
	os_assert(success==0);
	
	success = vcos_event_flags_create( &myIPC->client_data_status, NULL );
	os_assert(success==0);
	
	success = vcos_event_flags_create( &myIPC->server_data_status, NULL );
	os_assert(success==0);
	
	success = os_eventgroup_create( &myIPC->c2s_event, NULL );
	os_assert( success == 0 );

	success = os_eventgroup_create( &myIPC->s2c_event, NULL );
	os_assert( success == 0 );

	while(1)
	{
	   // wait for the event to say that there is a message
	   uint32_t events;
	   int32_t status;
	   VCHI_SERVICE_HANDLE_T handle;
	   int32_t data_size,flags;
	   void *data;
	   int32_t actual_msg_size;
	   VCHI_HELD_MSG_T held_msg,*message;
	   
	   
	   status = os_eventgroup_retrieve(&myIPC->c2s_event, &events );
	   os_assert(status == 0 );

		RPC_IPC_LOG("ipc_vchi_server events %d",events);
		
	   myIPC->khrn_callback(NULL,VCHI_CALLBACK_MSG_AVAILABLE,NULL);

	   handle	 = (VCHI_SERVICE_HANDLE_T) myIPC->buffer[3];
	   data_size = myIPC->buffer[1];
	   flags	 = myIPC->buffer[2];
	   data 	 = (void*)myIPC->buffer[4];	   

		switch (events)
		{
			case 1<<IPC_VCHI_MSG_QUEUE_EVENT:
				client_vchi_msg_queue(handle,data,data_size,flags,NULL);
				os_eventgroup_signal(&myIPC->s2c_event,IPC_VCHI_MSG_QUEUE_EVENT);
				break;
			case 1<<IPC_VCHI_BULK_QUEUE_TRANSMIT_EVENT:
				data_size = myIPC->bulk_buffer[1];
				flags     = myIPC->bulk_buffer[2];
				handle	 = (VCHI_SERVICE_HANDLE_T) myIPC->bulk_buffer[3];
				data      = (void*)myIPC->bulk_buffer[4];
				client_vchi_bulk_queue_transmit(handle,data,data_size,flags,NULL);
				os_eventgroup_signal(&myIPC->s2c_event,IPC_VCHI_BULK_QUEUE_TRANSMIT_EVENT);
				break;
			case 1<<IPC_VCHI_BULK_RECEIVE_EVENT:
				data_size = myIPC->bulk_buffer[1];
				flags     = myIPC->bulk_buffer[2];
				handle	 = (VCHI_SERVICE_HANDLE_T) myIPC->bulk_buffer[3];
				data      = (void*)myIPC->bulk_buffer[4];
				client_vchi_bulk_queue_receive(handle,data,data_size,flags,NULL);
				os_eventgroup_signal(&myIPC->s2c_event,IPC_VCHI_BULK_RECEIVE_EVENT);
				break;
			case 1<<IPC_VCHI_MSG_HOLD_EVENT:
				client_vchi_msg_hold(handle,&data,&myIPC->buffer[0],flags,&held_msg);
				os_eventgroup_signal(&myIPC->s2c_event,IPC_VCHI_MSG_HOLD_EVENT);
				break;
			case 1<<IPC_VCHI_MSG_RELEASE_EVENT:
				message = (VCHI_HELD_MSG_T*)myIPC->buffer[2];
				//os_assert(message == &held_msg);
				client_vchi_held_msg_release(&held_msg);
				os_eventgroup_signal(&myIPC->s2c_event,IPC_VCHI_MSG_RELEASE_EVENT);
				break;	
#ifdef __linux__				
			case 1<<IPC_VCHI_CLIENT_CTRL_WRITE_LOCK_EVENT:				
				vcos_semaphore_wait(&myIPC->client_write_lock);
				os_eventgroup_signal(&myIPC->s2c_event,IPC_VCHI_CLIENT_CTRL_WRITE_LOCK_EVENT);
				break;
			case 1<<IPC_VCHI_BULK_WRITE_LOCK_EVENT:				
				vcos_semaphore_wait(&myIPC->bulk_write_lock);
				os_eventgroup_signal(&myIPC->s2c_event,IPC_VCHI_BULK_WRITE_LOCK_EVENT);
				break;
			case 1<<IPC_VCHI_BULK_WRITE_RELEASE_EVENT:
				vcos_semaphore_post(&myIPC->bulk_write_lock);
				os_eventgroup_signal(&myIPC->s2c_event,IPC_VCHI_BULK_WRITE_RELEASE_EVENT);
				break;
#endif				
			case 1<<IPC_VCHI_MSG_DEQUEUE_EVENT:
			default:
				os_assert(0);
		}	
   	}

}


void vc_vchi_khronos_init(VCHI_INSTANCE_T initialise_instance,
                          VCHI_CONNECTION_T **connections,
                          uint32_t num_connections)
{
	VCOS_THREAD_T   mythread;
	const int	   STACK	   = 4*1024;
	
	bool success = client_process_attach();
	assert(success);


	success = os_thread_start(&mythread, ipc_vchi_server, NULL, STACK, "ipc_vchi_server");
	assert(success==0);

	vcos_assert(num_connections == 1);
}













