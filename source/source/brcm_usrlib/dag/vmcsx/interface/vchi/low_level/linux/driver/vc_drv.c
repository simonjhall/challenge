/******************************************************************************/
/*
 *  Device driver for VideoCore3 B0 Host Interface. 
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/param.h>
#include <linux/cdev.h>

#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

#include <linux/broadcom/vc.h>
#include <linux/broadcom/vc_frmfwd_defs.h>
#include <linux/broadcom/bcm_sysctl.h>
#include <linux/broadcom/debug_pause.h>
#include <linux/broadcom/hw_cfg.h>
#include <linux/broadcom/bcm_major.h>
#include <linux/broadcom/gpio_irq.h>
#include <asm/arch/reg_gpio.h>
#include <asm/arch/reg_irq.h>
#include <linux/broadcom/gpio.h>

#include <asm/io.h>
#include <asm/atomic.h>
#include <asm/semaphore.h>
#include <asm/uaccess.h>


/******************************************************************************/
/* Project Includes.                                                          */
/******************************************************************************/

#include "vchost.h"
#include "vchost_int.h"
#include "vchostreq_int.h"
#include "vcinterface.h"
#include "vciface.h"
#include "vclogging.h"
#include "vcstate_intern.h"
#include "vc3host-port.h"

#include <cfg_global.h>




/**
   Other external declarations
**/
extern void vchost_gpio_pintype_set(int pin, int pintype, int irqtype);
extern void vchost_gpio_pinshare(uint32_t pin, PIN_SHARE_GPIO_FUNCTION gpiofunc);
extern void vchost_gpio_pinval_set(int, int );



/******************************************************************************/
/* Private typedefs, macros and constants.                                    */
/******************************************************************************/

/**
   Enable this if you want the see verbose comments in the system log
**/
/* #define VERBOSE_MODE */

/**
   Enable interrupt operation
   
   Define this to enable use of interrupt-driven operation rather than crude but
   simple polling.
**/
//#define USE_VC3_IRQ

/**
   Default device manjor number.  Set to zero for autoselect.
**/
#define DEFAULT_MAJOR_NUM        ( 232 )

/**
   The number of virtual devices that this driver supports.
**/
#define NUM_VC3_DEVICES          ( 2 )  

/**
   Generic TX/RX buffer size
**/
#define BUFLEN                   ( 256 )

/**
   Macro to return the minimum of two values.
**/
#define MIN(a,b)                 ((a)<(b)?(a):(b))

/**
   Various SMI port parameters
**/

#define SMI_BUS_GPIO_START       ( 83 )
#define SMI_BUS_GPIO_END         ( 105 )

#define SMI_BUS_WIDTH               16     /* 8 or 16-bit connection to LCD */
#define SMI_WRITE_STROBE_LOW       500     /* ns */
#define SMI_WRITE_STROBE_HIGH      500     /* ns */
#define SMI_WRITE_STROBE_LOW_HWM   500     /* ns */
#define SMI_WRITE_STROBE_HIGH_HWM  500     /* ns */
#define SMI_READ_STROBE_LOW        500     /* ns */
#define SMI_READ_STROBE_HIGH       500     /* ns */
#define SMI_ADDRESS_SETUP           10     /* ns */
#define SMI_ADDRESS_HOLD            10     /* ns */
#define SMI_COREFREQ             200000000 /* Hz */

/**
   The two hardware VC registers we have access to
**/
#define VC3_PARAM_ADDR           ( 0 )
#define VC3_DATA_ADDR            ( 1 )

/**
   The SMI base address for the VCIII
**/
#define VC3_HOSTIF_IO_REG(n)     ( LCDSMI_ARM_SMIDxxx((n)) )

/**
   Identify the GPIO pin that controls the VC RUN pin
**/
#define HW_GPIO_VID_RUN_PIN      ( 2 )

/******************************************************************************/
/* Private Function Declarations.  Declare as static.                         */
/******************************************************************************/

/** Low level hardware interface functions **/

/* - Host Interface */

static uint16_t hostif_read ( int );
static void     hostif_read_block( int, uint8_t *, size_t );
static void     hostif_write( int, uint16_t );
static void     hostif_write_block( int, const uint8_t *, size_t );

/* - Hardware Initialisation */

static void     init_hardware( void );
static void     init_smi( void );
static unsigned int get_smi_timing( unsigned int, 
                                    unsigned int,
								            unsigned int, 
                                    unsigned int,
                                    unsigned int, 
                                    unsigned int );
/* - Hardware Control */

static void     set_run_pin( int );
                                   
/** Handler Functions ***********************/

static  int     vc3_dev_open   ( struct inode *, struct file * );
static  int     vc3_dev_release( struct inode *, struct file * );
static  ssize_t vc3_dev_read   ( struct file *,  char __user *, size_t , loff_t * );
static  ssize_t vc3_dev_write  ( struct file *,  const char __user *, size_t , loff_t * );
static  int     vc3_dev_ioctl  ( struct inode *, struct file *, unsigned int, unsigned long );

/** Linux Driver API Functions **/

static int  __init vc3_dev_init( void );
static void __exit vc3_dev_exit( void );

/******************************************************************************/
/* Private Data.  Declare as static.                                          */
/******************************************************************************/

/* Data structure for a character device. */
static struct cdev vc3_cdev;

/* The major device number.  Expose it to the loader to allow loadtime config */
static int major_num = DEFAULT_MAJOR_NUM;
module_param( major_num, int, S_IRUGO );

/**
   File Operations (these are the device driver entry points)
**/
struct file_operations dev_fops =
{
    .owner   = THIS_MODULE,
    .open    = vc3_dev_open,
    .release = vc3_dev_release,
    .read    = vc3_dev_read,
    .write   = vc3_dev_write,
    .ioctl   = vc3_dev_ioctl,
    
    /* ALL OTHERS NULL */	
};

/**
   We use a mutex to marshal access to the physical host interface
**/
static DECLARE_MUTEX_LOCKED( hostif_sem );
static int vc3_initialised = 0;

/******************************************************************************/
/* Private Functions.  Declare as static.                                     */
/******************************************************************************/

/******************************************************************************/
/**
   Read a single 16-bit value from a host port register.
   
   @param addr    Host port address (0 = control, 1 = data)
   
   @retval 16-bit value read from host port
**/
static uint16_t hostif_read( int addr )
{
   return VC3_HOSTIF_IO_REG(addr);
}

/******************************************************************************/
/**
   Read a block of bytes from the host port
   
   We must be careful here that we do not write more data to the buffer than we
   have been told to do so.  Therefore we catch the last byte read as a special
   case and do a byte write rather than the direct halfword write.
   
   @param addr    Host port address (0 = control, 1 = data)
   @param p       Pointer to buffer to store data
   @param n       Number of bytes to read
**/
static void hostif_read_block( int addr, uint8_t *p, size_t n )
{
   while ( n >= sizeof(uint16_t) )
   {
      *(uint16_t *)p = hostif_read( addr );
      p += sizeof(uint16_t);
      n -= sizeof(uint16_t);
   }
      
   if ( n )
      *p = hostif_read( addr );
}

/******************************************************************************/
/**
   Write a single 16-bit value to a host port register.
   
   @param addr    Host port address (0 = control, 1 = data)
   @param val     Value to write
**/
static void hostif_write( int addr, uint16_t val )
{
   VC3_HOSTIF_IO_REG(addr) = val;
}

/******************************************************************************/
/**
   Write a block of bytes to the host port
   
   Note that if we are writing an odd number of bytes the last write is treated
   as a byte write, with the upper half of the halfword cleared.
   
   @param addr    Host port address (0 = control, 1 = data)
   @param p       Pointer to buffer containing bytes to write
   @param n       Number of bytes to write
**/
static void hostif_write_block( int addr, const uint8_t *p, size_t n )
{
   while ( n >= sizeof(uint16_t) )
   {
      hostif_write( addr, *(uint16_t *)p );
      p += sizeof(uint16_t);
      n -= sizeof(uint16_t);
   }
   
   if ( n )
      hostif_write( addr, *p );
}

/******************************************************************************/
/**
   Generate SMI configuration value based on configuration parameters.
   
   @param width            Width of SMI data bus
   @param strobelow_ns     Strobe active period (ns)
   @param strobehigh_ns    Delay after strobe goes high (ns)
   @param setup_ns         Setup time (ns)
   @param hold_ns          Hold time (ns)
   @param core_freq        Core clock frequency (Hz)
   
   @returns SMI configuration value
**/
static unsigned int get_smi_timing( unsigned int width, 
                                    unsigned int strobelow_ns,
								            unsigned int strobehigh_ns, 
                                    unsigned int setup_ns,
                                    unsigned int hold_ns, 
                                    unsigned int core_freq )
{
   unsigned int result = 0;

   /* [31:30] - 00 = 8 bit
    *           01 = 16 bit
    *           10 = 18 bit
    *           11 = 9 bit
    */
   switch ( width )
   {
      case 8:
         break;
      case 9:
         result |= 0xC0000000;
         break;
      case 16:
         result |= 0x40000000;
         break;
      case 18:
         result |= 0x80000000;
         break;
      default:
         vc_assert(0);
         break;
   }
   
   /* USE SLOW DEFAULTS ***************/
   
   /* [29:24] - SETUP */
   result |= 0x3f << 24;
   
   /* [21:16] - HOLD */
   result |= 0x3f << 16;
   
   /* [14:8] - PACE */
   result |= 0x7f << 8;
   
   /* [6:0] - STROBE */
   result |= 0x7f << 0;
   
   
#if 0    /* Not used during development */
   {
      unsigned int i = 0;
      unsigned int core = 0;

      core = core_freq / 1000;

      i = (strobelow_ns * core + 1000000-1) / 1000000;
      //in register, 0 means 1 cycle etc
      i = max(i-1, (int)0);
      vc_assert(i < 0x80);
      result |= i;

      i = (setup_ns * core + 1000000-1) / 1000000;
      //in register, 0 means 1 cycle etc
      i = max(i-1, 0);
      vc_assert(i < 0x40);
      result |= (i<<24);

      i = (hold_ns * core + 1000000-1) / 1000000;
      //in register, 0 means 1 cycle etc
      i = max(0, i-1);
      vc_assert(i < 0x40);
      result |= (i<<16);

      i = ((strobehigh_ns - hold_ns) * core + 1000000-1) / 1000000;
      //in register, 0 means 1 cycle etc
      i = max(0, i-1);
      vc_assert(i < 0x80);
      result |= (i<<8);
   }   
#endif
   
   return result;
}

/******************************************************************************/
/**
   Initialise the SMI interface
   
   We only need to configure two read and two write channels to correspond to
   the two addresses that the VCIII host port exposes.
**/
static void init_smi( void )
{
   uint32_t val;
   int pin;

   /* Set up pins for SMI use (ALT1 on the 2820) */
   for ( pin = SMI_BUS_GPIO_START; pin <= SMI_BUS_GPIO_END; pin++ )
      vchost_gpio_pinshare( pin, PIN_SHARE_GPIO_ALT1 );

   /* Configure control registers */
   /* - AHB Peripheral Register: clear LMSEL and SMSEL */
   CLKMGR_CMAHB &= ~3;
   
   /* - Enable SMI */
   LCDSMI_ARM_SMICS |= 1;
  
   /* Configure READ SMI channels */
   val = get_smi_timing( SMI_BUS_WIDTH,
				             SMI_READ_STROBE_LOW,
				             SMI_READ_STROBE_HIGH,
				             SMI_ADDRESS_SETUP,
				             SMI_ADDRESS_HOLD,
				             SMI_COREFREQ );

   LCDSMI_ARM_SMIDSR0 = val;
   LCDSMI_ARM_SMIDSR1 = val;
     
   /* Configure WRITE SMI channels */
   val = get_smi_timing( SMI_BUS_WIDTH,
					          SMI_WRITE_STROBE_LOW_HWM,
					          SMI_WRITE_STROBE_HIGH_HWM,
					          SMI_ADDRESS_SETUP,
					          SMI_ADDRESS_HOLD,
					          SMI_COREFREQ );

   LCDSMI_ARM_SMIDSW0 = val;
   LCDSMI_ARM_SMIDSW1 = val;
}

/******************************************************************************/
/**
   Set the RUN pin to a given state
   
   @param state      New state to set RUN pin to
**/
static void set_run_pin( int state )
{
   vchost_gpio_pinval_set( HW_GPIO_VID_RUN_PIN, state );
}

/******************************************************************************/
/**
   Initialise the hardware
**/
static void init_hardware( void )
{
   vchost_gpio_pinshare( HW_GPIO_VID_RUN_PIN, PIN_SHARE_GPIO_ALT3 );
   
   vchost_gpio_pintype_set( HW_GPIO_VID_RUN_PIN, 
                            GPIO_PIN_TYPE_OUTPUT, 
                            GPIO_NO_INTERRUPT   );

   init_smi();
}

/******************************************************************************/
/* FUNCTIONS TO KEEP BUILD HAPPY.                                             */
/******************************************************************************/


void vc_host_delay_msec( unsigned msec )
{
}

void vc_host_lock_obtain( int channel )
{
}

void vc_host_lock_release( int channel )
{
}

void lcd_dirty_rows( void *dirtyRows )
{
}

void *lcd_get_framebuffer_addr( int *frame_size, void *dma_addr )
{
   return NULL;
}

void lcd_get_info( void *lcdInfo )
{
}


/******************************************************************************/
/* Driver Functions.  Declare as static.                                      */
/******************************************************************************/

/******************************************************************************/
/**
   Handle OPEN calls.
   
   There is not much to do in this function other than to record which
   device corresponds to this file open operation.
   
   @param inode      Pointer to file system inode
   @param filp       Pointer to file struct
   
   @retval 0         Success
**/
static int vc3_dev_open( struct inode *inode, struct file *filp )
{
    /* Put device pointer into FILE structure */    
    filp->private_data = (void *)iminor( inode );    

    return 0;
}

/******************************************************************************/
/**
   Handle RELEASE calls from kernel
   
   ** Nothing much to do here at the moment. **
   
   @param inode      Pointer to file system inode
   @param filp       Pointer to file struct
   
   @retval 0         Success
**/
static int vc3_dev_release( struct inode *inode, struct file *filp )
{
   /* TODO */
   
    return 0;
}

/******************************************************************************/
/**
   Handle READ calls.
   
   @param file
   @param buffer
   @param count
   @param ppos
   
   @return    Number of bytes read, or a negative error code
**/
static ssize_t vc3_dev_read( struct file *filp, char __user *buffer, 
                             size_t count,      loff_t *ppos )
{
   int    address    = (unsigned int)filp->private_data;
   size_t bytes_read = 0;
   char   rxbuf[BUFLEN];
   
   if ( down_interruptible( &hostif_sem ) )
      return -ERESTARTSYS;
   
   while ( bytes_read < count )
   {
      size_t n = MIN( BUFLEN, count - bytes_read );
      
      hostif_read_block( address, rxbuf, n );      
      if ( copy_to_user( buffer, rxbuf, n ) )
      {
         up( &hostif_sem );
         return -EFAULT;      
      } 
         
      buffer     += n; 
      *ppos      += n;
      bytes_read += n;
   }
    
#ifdef VERBOSE_MODE      
   printk( KERN_INFO "vc03[%d]: read %d bytes to %p\n", address, count, buffer );
#endif

   up( &hostif_sem );
   return (ssize_t)bytes_read;
}

/******************************************************************************/
/**
   Handle WRITE calls
   
   @param file
   @param buffer
   @param count
   @param ppos
   
   @return    Number of bytes written, or a negative error code
**/
static ssize_t vc3_dev_write( struct file *filp, const char __user *buffer, 
                              size_t count,      loff_t *ppos )
{
   int    address       = (unsigned int)filp->private_data;
   size_t bytes_written = 0;
   char   txbuf[BUFLEN];
   
   if ( down_interruptible( &hostif_sem ) )
      return -ERESTARTSYS;

   while ( bytes_written < count )
   {
      size_t n = MIN( BUFLEN, count - bytes_written );
      
      if ( copy_from_user( txbuf, buffer, n ) )
      {
         up( &hostif_sem );
         return -EFAULT;      
      }
      
      hostif_write_block( address, txbuf, n );
                
      buffer        += n; 
      *ppos         += n;
      bytes_written += n;
   }
    
#ifdef VERBOSE_MODE      
   printk( KERN_INFO "vc03[%d]: write %d bytes from %p\n", address, count, buffer );
#endif   

   up( &hostif_sem );
   return (ssize_t)bytes_written;
}

/******************************************************************************/
/**
   Handle IOCTL calls.
   
   @param inode
   @param file
   @param cmd
   @param arg
   
   @retval 0            Success
   @retval -ENOTTY      Invalid IOCTL command code
   @retval 
   
**/
static int vc3_dev_ioctl( struct inode *inode, struct file *filp, 
                          unsigned int cmd,    unsigned long arg )
{
   if (  ( _IOC_TYPE( cmd ) != VC_MAGIC      )
      || ( _IOC_NR( cmd )    < VC_CMD_FIRST  )
      || ( _IOC_NR( cmd )    > VC_CMD_LAST   ) )
   {
      printk( KERN_WARNING "vc03: Invalid IOCTL cmd: %#4.4X\n", cmd );
      return -ENOTTY;
   }
  
   switch ( cmd )
   {
   case VC_IOCTL_RUN:
   	set_run_pin( 1 );
      break;

   case VC_IOCTL_HIB:
      set_run_pin( 0 );
      break;
      
   default:
      printk( KERN_WARNING "vc03: Unrecognized IOCTL cmd: %#4.4X\n", cmd );
	   return -ENOTTY;
   }
  
   return 0;
}

/******************************************************************************/
/**
   Driver initialisation, called when loaded.

   @retval
**/
static int __init vc3_dev_init( void )
{
   int rc = 0;
   dev_t dev;
   
   printk( KERN_INFO "Broadcom VC-III B0 Driver\n" );
     
   if ( major_num )
   {
      /* Use major number defined on command line when loading */
      dev = MKDEV( major_num, 0 );
      rc  = register_chrdev_region( dev, NUM_VC3_DEVICES, "vc03" );   
   }
   else
   {
      /* Dynamically allocate a single major device number, 
       *  with two minors starting at 0, and naming the driver.
       */
      rc = alloc_chrdev_region( &dev, 0, NUM_VC3_DEVICES, "vc03" );
      major_num = MAJOR( dev );
   }
   
   if ( rc < 0 )   
   {
      printk( KERN_WARNING "VC-III: register_chrdev failed for major %d\n", major_num );
      return rc;
   }
   
   cdev_init( &vc3_cdev, &dev_fops );
   vc3_cdev.owner = THIS_MODULE;
   vc3_cdev.ops   = &dev_fops;

  
#ifdef USE_VC3_IRQ  
   rc = request_irq(GPIO_TO_IRQ( HW_GPIO_VID_INT_PIN ), vc_gpioirq, ( IRQF_DISABLED | IRQF_TRIGGER_RISING ), "VC03_irq", &val);
   
   if (rc != 0)
	 {
	   printk( KERN_WARNING "VC-III: failed to register isr=%d rc=%d\n", GPIO_TO_IRQ( HW_GPIO_VID_INT_PIN ), rc);
	   return rc;
	 }   
#endif  

   init_hardware();

   /******* After a successful call to cdev_add() the driver is 'live' ********/
   
   rc = cdev_add( &vc3_cdev, dev, NUM_VC3_DEVICES );
   if ( rc < 0 )
   {
      unregister_chrdev_region( dev, NUM_VC3_DEVICES );
      printk( KERN_WARNING "VC-III: driver failed to load\n" );   
   }
   else
   {   
      up( &hostif_sem );
      vc3_initialised = 1;

      printk( KERN_INFO "VC-III: driver \"vc03\" successfully loaded on %d.[%d-%d]\n", 
              MAJOR(dev), MINOR(dev), MINOR(dev)+1 );
   }
   
   return rc;
}

/******************************************************************************/
/**
   Driver shutdown, called when driver module unloaded.
**/
static void __exit vc3_dev_exit( void )
{						
   if ( !vc3_initialised )
      return;
      
   down_interruptible( &hostif_sem );

#ifdef USE_VC3_IRQ
   {
      int val;
	   free_irq ( GPIO_TO_IRQ( HW_GPIO_VID_INT_PIN ), &val );
   }
#endif

   /* Remove the device driver from the system */
   unregister_chrdev_region( MKDEV( major_num, 0 ), NUM_VC3_DEVICES );
   cdev_del( &vc3_cdev );

   /* All done. */
   vc3_initialised = 0;
   
   printk( KERN_INFO "vc03 driver successfully unloaded\n" );
} 

/******************************************************************************/
/**                            House-keeping                                 **/
/******************************************************************************/

module_init( vc3_dev_init );
module_exit( vc3_dev_exit );

MODULE_AUTHOR( "Broadcom" );
MODULE_DESCRIPTION( "VC-III B0 Driver" );

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
