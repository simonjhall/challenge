/*****************************************************************************
*  Copyright 2007 Broadcom Corporation.  All rights reserved.
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

/*****************************************************************************
*
* This file contains code which may be shared between the boot2 bootloader
* and the VideoCore driver.
*
* When required, use #if defined( __KERNEL__ ) to tell if the code is being
* compiled as part of the kernel or not.
*
*****************************************************************************/

#if !defined( VCHOST_PORT_H )
#define VCHOST_PORT_H

/* ---- Include Files ---------------------------------------------------- */

#include <cfg_global.h>

#if defined( __KERNEL__ )
#   include <linux/types.h>
#else
#   include <stdint.h>
#endif


// The following is also defined in vchost_config.h, but we don't want to include
// that from within this header file. So for now, we've duplicated the declaration

//#define VC_HOST_IS_BIG_ENDIAN

#define VC_HTOV32(val) (val)
#define VC_HTOV16(val) (val)
#define VC_VTOH32(val) VC_HTOV32(val)
#define VC_VTOH16(val) VC_HTOV16(val)

/* ---- Constants and Types ---------------------------------------------- */

/*
 * We only use DMA when operating from within the kernel, and currently, only
 * on the MIPS based platforms
 */

#if defined( __KERNEL__ ) && ( CFG_GLOBAL_CPU == MIPS32 ) && defined( VC_HOST_IS_BIG_ENDIAN )
#   define  VC_HOST_PORT_USE_DMA 1
#else
#   define  VC_HOST_PORT_USE_DMA 0
#endif

#if !defined( VC_DEBUG_ENABLED )
#   define VC_DEBUG_ENABLED    1
#endif

// bitflags for HCS reads
#define WFE     (1<<0)
#define WFD     (1<<1)
#define WFW     (1<<2)
#define RFF     (1<<3)
#define RFD     (1<<4)
#define RFR     (1<<5)
#define RSTN    (1<<6)
#define BP      (1<<7)

#define LIMITED_WAIT_FOR( condition ) \
{   \
    int sanity = 1;             \
    while ( !(condition))       \
    {                           \
        if ( ++sanity == 50000 )    \
        {                       \
            PRINT_ERR(  "%s: timeout out waiting for '%s'\n",  __FUNCTION__, #condition ); \
            rc = 1;             \
            goto end;           \
        }                       \
    }                           \
}

#define LIMITED_WAIT_FOR2( condition )			\
{   \
    int sanity = 1;             \
    while ( !(condition))       \
    {                           \
        if ( ++sanity == 50000 )    \
        {                       \
            rc = 1;             \
            goto end;           \
        }                       \
    }                           \
}

typedef enum
  {
	hostport_mode_unknown = 0,
	hostport_mode_rsingle32 = 1,
	hostport_mode_rfifo32 = 2,
	
	hostport_mode_max,
  } hostport_mode_t;
  

/* ---- Variable Externs ------------------------------------------------- */

extern  int     gVcHostPortRLED;
extern  int     gVcHostPortWLED;

#if VC_HOST_PORT_USE_DMA
extern  int     gVcUseDma;
#endif

extern  int     gVcUse32bitIO;
extern  int     gVcUseFastXfer;
extern  int     gVc32BitIOAvail;

#define VC_DEBUG_ENABLED 1
#if VC_DEBUG_ENABLED
extern  int     gVcDebugRead;
extern  int     gVcDebugWrite;
extern  int     gVcDebugTrace;
extern  int     gVcDebugMsgFifo;
extern  int     gVcProfileTransfers;
#endif


/* ---- Function Prototypes ---------------------------------------------- */

int         vc_host_port_init(void* portcfg );
void        vc_host_init_sdram( void );

void        vc_host_init_pins( void );
void        vc_host_set_run_pin( int arg );
void        vc_host_set_hibernate_pin( int arg );
void        vc_host_set_reset_pin( int arg );
void        vc_host_reset( void );

/* The delay function must be implemented elsewhere, but we need to call it, so we 
 * put the prototype here
 */

void        vc_host_delay_msec( unsigned msec );

/* Boot a bin file from flash into RAM. Returns the id of the application running */
int         vc_host_boot( char *cmd_line, void *binimg, size_t nbytes, int bootloader );

int         vc_host_check_crcs( unsigned delayMsec );

uint16_t    vc_host_read_reg( int reg, int channel );
void        vc_host_write_reg( int reg, int channel, uint16_t val );

/* Read a multiple of 16 bytes from VideoCore. host_addr has no particular alignment,
   but it is important that it transfers the data in 16-bit chunks if this is possible. */

int vc_host_read_consecutive(void *host_addr, uint32_t vc_addr, int nbytes, int channel);
uint32_t vc_host_read32(uint32_t vc_addr, int channel);

#if defined( VC_HOST_IS_BIG_ENDIAN )
// Reads from VideoCore with an implicit swap of each pair of bytes.
int vc_host_read_byteswapped(void *host_addr, uint32_t vc_addr, int nbytes, int channel);
int vc_host_read_byteswapped_32(void *host_addr, uint32_t vc_addr, int nbytes, int channel);

#define VC_HOST_READ_BYTESWAPPED_16 vc_host_read_byteswapped
#define VC_HOST_READ_BYTESWAPPED_32 vc_host_read_byteswapped_32
#else
#define VC_HOST_READ_BYTESWAPPED_16 vc_host_read_consecutive
#define VC_HOST_READ_BYTESWAPPED_32 vc_host_read_consecutive
#endif

/* Write a multiple of 16 bytes to VideoCore. host_addr has no particular alignment,
   but it is important that it transfers the data in 16-bit chunks if this is possible. */

int vc_host_write_consecutive(uint32_t vc_addr, void *host_addr, int nbytes, int channel);
#if defined( VC_HOST_IS_BIG_ENDIAN )
// Write to VideoCore with an implicit swap of each pair of bytes.
int vc_host_write_byteswapped(uint32_t vc_addr, void *host_addr, int nbytes, int channel);
int vc_host_write_byteswapped_32(uint32_t vc_addr, void *host_addr, int nbytes, int channel);

#define VC_HOST_WRITE_BYTESWAPPED_16 vc_host_write_byteswapped
#define VC_HOST_WRITE_BYTESWAPPED_32 vc_host_write_byteswapped_32
#else
#define VC_HOST_WRITE_BYTESWAPPED_16 vc_host_write_consecutive
#define VC_HOST_WRITE_BYTESWAPPED_32 vc_host_write_consecutive
#endif

int vc_host_send_interrupt( int channel );

#if defined( __KERNEL__ )

void vc_host_lock_obtain( int chan_num );
void vc_host_lock_release( int chan_num );

#else

#define vc_host_lock_obtain( chan_num )
#define vc_host_lock_release( chan_num )

#endif

void vc_dump_mem(    const char *label, uint32_t addr, const void *voidMem, size_t numBytes );
void vc_dump_mem_16( const char *label, uint32_t addr, const void *voidMem, size_t numBytes );
void vc_dump_mem_32( const char *label, uint32_t addr, const void *voidMem, size_t numBytes );



#if VC_HOST_PORT_USE_DMA

// The following functions are NOT found in vchost-port.c, but we need the 
// function prototypes, so we include them here.

int vcHostDmaWrite( uint16_t *host_addr, volatile uint16_t *vcdata, volatile uint16_t *vchcs, int nblocks, int swap16bits );
int vcHostDmaRead( uint16_t *host_addr, volatile uint16_t *vcdata, volatile uint16_t *vchcs, int nblocks, int swap16bits );

#endif

#endif  // VCHOST_PORT_H

