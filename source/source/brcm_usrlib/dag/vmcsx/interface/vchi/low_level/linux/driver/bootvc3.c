/******************************************************************************/
/*
 *  Test Program for Device driver for VideoCore3 B0 Host Interface.
 *
 *  Copyright (C) 2001 - 2008 Broadcom Corporation.  All rights reserved.
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
 */
/******************************************************************************/

/******************************************************************************/
/* System Includes.                                                           */
/******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/******************************************************************************/
/* Project Includes.                                                          */
/******************************************************************************/

/* None */

/******************************************************************************/
/* Private typedefs, macros and constants.                                    */
/******************************************************************************/

/**
   Enable debugging messages
**/
/* #define VERBOSE_MODE */

/**
   Shorter names for common types
**/
typedef signed char    int8_t;
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

/**
   Host port message types
**/
#define VC03_CTRL_MESSAGE     ( 0 )
#define VC03_DATA_MESSAGE     ( 1 )

/**
   Bits within host port message parameter register
**/
#define VC03HOST_MSGAVAIL        ( 1 << 14 )
#define VC03HOST_NEWMSG          ( 1 << 13 )
#define VC03HOST_MSGTYPE         ( 1 << 12 )
#define VC03HOST_UNDERFLOW       ( 1 << 11 )
#define VC03HOST_OVERFLOW        ( 1 << 10 )
#define VC03HOST_FIFOSTATUS      ( 1 <<  9 )

/**
   Bootrom message type
**/
typedef struct
{
  uint32_t header;        
  uint32_t id_size;       
  uint32_t signature[5];  
  uint32_t footer;        
  
} boot_message2_t;

/**
   Bootrom formatting codes, from VCIII bootrom source code
**/
#define HOSTPORT_COMMS_HEADER    ( 0xAFE45251 )
#define HOSTPORT_COMMS_FOOTER    ( 0xF001BC4E )

/**
   Bootrom message codes
**/
typedef enum
{
   HOSTPORT_MESSAGE_ID_MIN,

   HOSTPORT_MESSAGE_ID_BOOTROM_HAS_STARTED,
   HOSTPORT_MESSAGE_ID_HOST_HAS_SENT_2ND_STAGE_LOADER,
   HOSTPORT_MESSAGE_ID_2ND_STAGE_LOADER_VERIFIED,
   HOSTPORT_MESSAGE_ID_RUN_APPLICATION,

   HOSTPORT_MESSAGE_ID_MAX

} HOSTPORT_MESSAGE_ID_T;

/**
   Set the number of iterations for iterative operations
**/
#define N_ITER    ( 300 )

/**
   Return the minimum of two numeric arguments
**/   
#define MIN(a,b)     ( ((a) < (b))?(a):(b) )

/******************************************************************************/
/* Private Function Declarations.  Declare as static.                         */
/******************************************************************************/

static ssize_t   host_readmsg  ( void *, size_t );
static int       host_writemsg ( const void *, size_t, int );
static void      send_ctrl_message( uint32_t, uint32_t );
static int       received_message( uint32_t );
static int       vc3_bootb0_mem( const uint8_t *, uint32_t, const uint8_t *, uint32_t );
static uint8_t * load_bin_file ( const char *, size_t * );

/******************************************************************************/
/* Private Data.  Declare as static.                                          */
/******************************************************************************/

static int fd_ctrl = 0;
static int fd_data = 0;

/******************************************************************************/
/* Private Functions.  Declare as static.                                     */
/******************************************************************************/

/******************************************************************************/
/**
   Log messages to stdout
   
   @param format     Message format string and optional arguments (see printf)
**/
static void log_message( char *format, ... )
{
#ifdef VERBOSE_MODE

   va_list ap;
   
   va_start( ap, format );
   vprintf( format );
   va_end( ap );

#endif
}

/******************************************************************************/
/**
   Read a message from the VideoCore
   
   @param msg           Pointer to message buffer
   @param maxlen_bytes  Maximum number of bytes to read
   
   @return   Number of bytes read, or -1 for error
**/
static ssize_t host_readmsg( void * msg, size_t maxlen_bytes )
{
   uint16_t val;
   size_t length;
   size_t bytes_to_read;

   /* First word:
    *    [15]   = 0
    *    [14:0] = length[14:0]
    */
   do
   {
      read( fd_ctrl, &val, sizeof(val) );
   }
   while ( ( val & (1 << 15) ) != 0 );

   log_message( "readmsg word %d = %#4.4X\n", (val >> 15)&1, val );
   
   length = val & 0x7FFF;

   /* Second word:
    *    [15]   = 1
    *    [14:9] = various flags:
    *             [14] = M  - Message waiting
    *             [13] = N  - New message
    *             [12] = T  - Message type (0 = control, 1 = data)
    *             [11] = UF - Underflow
    *             [10] = OF - Overflow
    *             [ 9] = FS - FIFO status
    *    [ 8:0] = length[23:15]
    */
   do
   {
      read( fd_ctrl, &val, sizeof(val) );
   } 
   while ( ( val & (1 << 15) ) != (1 << 15) );

   log_message( "readmsg word %d = %#4.4X\n", (val >> 15)&1, val );
   
   /* Check status flags */
   if ( 0 == (val & VC03HOST_MSGAVAIL) )
   {
      /* No message */
      return -1;
   }
    
   /* Merge in the top 9 bits of length, and adjust */
   length += ( val & 0x1FF ) << 15;
   length += 1;
   
   /* Limit the number of bytes we read to the size of buffer we were given 
    *  in the first place.
    */
   bytes_to_read = MIN( maxlen_bytes, length );

   return read( fd_data, msg, bytes_to_read );
}

/******************************************************************************/
/**
   Write a message to the VideoCore
   
   @param msg           Pointer to message buffer
   @param maxlen_bytes  Maximum number of bytes to write
   @param channel       Number of channel to send to
   
   @return   Number of bytes written, or -1 for error
**/
static int host_writemsg( const void * msg, 
                          size_t       len_bytes, 
                          int          channel )
{		
   uint16_t msg_param;
   size_t bytes_written;
  
   log_message( "Writing %d bytes to VC\n", len_bytes );
   
   msg_param = ( 0 << 2 )        /* No Host-specified DMA flag */
             | ( 1 << 1 )        /* Force DMA termination      */
             | ( channel & 1 );  /* Data/control flag          */

   write( fd_ctrl, &msg_param, sizeof(msg_param) );
   
   bytes_written = write( fd_data, msg, len_bytes );
   
   /* Protocol demands that all messages sent have length multiple of 16 bytes.
    *  So if we sent less, send some dummy bytes, noting that the driver has
    *  already sent the odd dummy byte as part of the last 16-bit write.
    * In this instance we use zero bytes for the dummy bytes.
    */
   if ( bytes_written % 16 )
   {
      size_t zero_bytes = (16 - ( bytes_written % 16 )) & ~1;
      uint32_t zeros[4] = { 0,0,0,0 };
      
      log_message( "Writing %d zero bytes\n", zero_bytes );
      write( fd_data, zeros, zero_bytes );
   }
   
   /* Send a NULL message to complete this message */
   msg_param = 0;
   write( fd_ctrl, &msg_param, sizeof(msg_param) );
   
   return bytes_written;
}

/******************************************************************************/
/**
   Send a control message
   
   @param msg     Message type
   @param arg     User argument
**/
static void send_ctrl_message( uint32_t msg, uint32_t arg )
{
   boot_message2_t msg2;

   memset( &msg2, 0, sizeof(msg2) );
   
   msg2.header  = HOSTPORT_COMMS_HEADER;
   msg2.id_size = ( msg << 24 ) | ( arg & 0xffffff );
   msg2.footer  = HOSTPORT_COMMS_FOOTER;
   
   host_writemsg( &msg2, sizeof(msg2), VC03_CTRL_MESSAGE );
}

/******************************************************************************/
/**
   Wait for a specific boot message from the VideoCore
   
   @param msgtype  Message type to wait for
   
   @return         1 if successful, else 0 if no message received
**/
static int received_boot_message( uint32_t msgtype )
{
   int iter;
   uint32_t length;
   uint32_t msg[8];

   for ( iter = 0; iter < N_ITER; iter++ )  
   {
      memset( msg, 0, sizeof(msg) );
      length = host_readmsg( msg, sizeof(msg) );
      
      if ( length           == 32 
           &&   
           msg[0]           == HOSTPORT_COMMS_HEADER 
           && 
           ( msg[1] >> 24 ) == msgtype )
      {
         log_message( "length = %d, msg[0] = %#8.8X, msg[1] = %#8.8X\n", 
                      length, msg[0], msg[1] );
         return 1;
      }
   }
   
   return 0;
}

/******************************************************************************/
/**
   Boot a VideoCore 3 over the host interface.
   
   @param stage2_ptr    Pointer to Stage2 bootloader binary data
   @param stage2_size   Length ofstage2 loader in bytes
   @param vcvmcs_ptr    Pointer to VMCS binary data
   @param vcvmcs_size   Length of VMCS binar data in bytes
   
   @return 0 if successful, -1 if something went wrong
**/
static int vc3_bootb0_mem( const uint8_t * stage2_ptr, uint32_t stage2_size, 
                           const uint8_t * vcvmcs_ptr, uint32_t vcvmcs_size )
{
   uint32_t msg[8];
   uint32_t length;
   boot_message2_t msg2;
   int iter;
   int flag;

#define VC_IOCTL_RUN       ( 0x5686 )
#define VC_IOCTL_HIB       ( 0x5687 )

   /* Reset the VideoCore */
   ioctl( fd_ctrl, VC_IOCTL_HIB );
   usleep( 100000 ); /* 100ms */
   ioctl( fd_ctrl, VC_IOCTL_RUN );

   /* read out message 1 "boot rom running" */
   if ( !received_boot_message( HOSTPORT_MESSAGE_ID_BOOTROM_HAS_STARTED ) )
   {
      log_message( "VC boot ROM not running\n" );
      return -1;     
   }

   log_message( "VC boot ROM is alive\n" );

   /* Write stage 2 loader to Data channel */
   if ( stage2_size != host_writemsg( stage2_ptr, stage2_size, VC03_DATA_MESSAGE ) )
   {
      log_message( "Stage2 loader ran short\n" );
      return -1;
   }
   
   /* Write ctrl message 2 "host finished sending stage 2 loader" */
   send_ctrl_message( HOSTPORT_MESSAGE_ID_HOST_HAS_SENT_2ND_STAGE_LOADER, stage2_size );
   
   /* Poll 2727 for message 1 "boot rom received and verified stage 2 loader" */
   if ( !received_boot_message( HOSTPORT_MESSAGE_ID_BOOTROM_HAS_STARTED ) )
   {
      log_message( "Stage 2 not loaded successfully\n" );
      return -1;
   }

   log_message( "VC03: 2nd stage loader operating\n" );

   /* Load main binary */
   host_writemsg( vcvmcs_ptr, vcvmcs_size, VC03_DATA_MESSAGE );
   
   /* Finally, send a message to say we've finished sending */
   send_ctrl_message( HOSTPORT_MESSAGE_ID_HOST_HAS_SENT_2ND_STAGE_LOADER, stage2_size );

   return 0;
}   

/******************************************************************************/
/**
   Load a binary file into a new memory block
   
   @param fname      Name of binary file to load
   @param len        Pointer to variable to store length of data block
   
   @return           Ptr to data block if successful, else NULL.   
**/
static uint8_t * load_bin_file( const char *fname, size_t *len )
{
   FILE *fp;
   size_t n;
   uint8_t *ptr;

   fp = fopen( fname, "rb" );
   if ( fp == NULL )
      return NULL;
   
   fseek( fp, 0L, SEEK_END );
   n = ftell( fp );
   rewind( fp );
   
   ptr = malloc( n );
   if ( ptr == NULL )
   {
      fclose ( fp );
      return NULL;
   }
   
   if ( n != fread( ptr, 1, n, fp ) )
   {
      free( ptr );
      fclose( fp );
      return NULL;
   }
   
   fclose( fp );
   
   log_message( "Loaded %s (%d bytes)\n", fname, n );
   
   *len = n;
   return ptr;
}

/******************************************************************************/
/* Punblic Functions.                                                         */
/******************************************************************************/

/******************************************************************************/
/**
   Application entry point
   
   @retval EXIT_FAILURE if something went wrong
   @retval EXIT_SUCCESS if successful
**/
int main( int argc, char *argv[] )
{
   uint8_t* stage2_ptr;
   uint32_t stage2_size;
   uint8_t* vcvmcs_ptr;
   uint32_t vcvmcs_size;
   
   if ( argc != 3 )
   {
      printf( "Usage: %s <stage2 binfile> <vmcs binfile>\n", argv[0] );
      return EXIT_FAILURE;
   }
   
   /* Open the two Host devices */
   fd_ctrl = open( "/dev/vc03p0", O_RDWR );
   fd_data = open( "/dev/vc03p1", O_RDWR );
   
   if ( fd_ctrl == -1 || fd_data == -1 )
   {
      printf( "Failed to open VC3 devices.\n" );
      return EXIT_FAILURE;
   }
   
   stage2_ptr = load_bin_file( argv[1], &stage2_size );
   if ( stage2_ptr == NULL )
   {
      printf( "Failed to load stage2 loader \"%s\".\n", argv[1] );
      return EXIT_FAILURE;
   }
   
   vcvmcs_ptr = load_bin_file( argv[2], &vcvmcs_size );
   if ( vcvmcs_ptr == NULL )
   {
      printf( "Failed to load VMCS binary file \"%s\".\n", argv[2] );
      return EXIT_FAILURE;
   }

   while ( vc3_bootb0_mem( stage2_ptr, stage2_size, vcvmcs_ptr, vcvmcs_size ) != 0 )
   {
      printf( "Failed to boot, trying again...\n" );
   }
   
   printf( "VCIII booted.\n" );   
   
   close( fd_data );
   close( fd_ctrl );
   
   return EXIT_SUCCESS;
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
