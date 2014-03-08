/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited.
All rights reserved.

Project  :  VCHI
Module   :  Endian-aware routines to read/write data

FILE DESCRIPTION

=============================================================================*/

#include "endian.h"


/******************************************************************************
  Register fields
 *****************************************************************************/


/******************************************************************************
  Local typedefs
 *****************************************************************************/


/******************************************************************************
 Extern functions
 *****************************************************************************/


/******************************************************************************
 Static functions
 *****************************************************************************/


/******************************************************************************
 Static data
 *****************************************************************************/


/******************************************************************************
 Global Functions
 *****************************************************************************/

/* ----------------------------------------------------------------------
 * read an int16_t from buffer.
 * network format is defined to be little endian
 * -------------------------------------------------------------------- */
int16_t
vchi_readbuf_int16( const void *_ptr )
{
   const unsigned char *ptr = (const unsigned char *)_ptr;
   return ptr[0] | (ptr[1] << 8);
}

/* ----------------------------------------------------------------------
 * read a uint16_t from buffer.
 * network format is defined to be little endian
 * -------------------------------------------------------------------- */
uint16_t
vchi_readbuf_uint16( const void *_ptr )
{
   const unsigned char *ptr = (const unsigned char *)_ptr;
   return ptr[0] | (ptr[1] << 8);
}

/* ----------------------------------------------------------------------
 * read a uint32_t from buffer.
 * network format is defined to be little endian
 * -------------------------------------------------------------------- */
uint32_t
vchi_readbuf_uint32( const void *_ptr )
{
   const unsigned char *ptr = (const unsigned char *)_ptr;
   return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
}

/* ----------------------------------------------------------------------
 * read a fourcc_t from buffer.
 * network format is defined to be first character first
 * -------------------------------------------------------------------- */
fourcc_t
vchi_readbuf_fourcc( const void *_ptr )
{
   const unsigned char *ptr = (const unsigned char *)_ptr;
   return (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
}


/* ----------------------------------------------------------------------
 * write a uint16_t into the buffer.
 * network format is defined to be little endian
 * -------------------------------------------------------------------- */
void
vchi_writebuf_uint16( void *_ptr, uint16_t value )
{
   unsigned char *ptr = (unsigned char *)_ptr;
   ptr[0] = (value >> 0)  & 0xFF;
   ptr[1] = (value >> 8)  & 0xFF;
}

/* ----------------------------------------------------------------------
 * write a uint32_t into the buffer.
 * network format is defined to be little endian
 * -------------------------------------------------------------------- */
void
vchi_writebuf_uint32( void *_ptr, uint32_t value )
{
   unsigned char *ptr = (unsigned char *)_ptr;
   ptr[0] = (unsigned char)((value >> 0)  & 0xFF);
   ptr[1] = (unsigned char)((value >> 8)  & 0xFF);
   ptr[2] = (unsigned char)((value >> 16) & 0xFF);
   ptr[3] = (unsigned char)((value >> 24) & 0xFF);
}

/* ----------------------------------------------------------------------
 * write a fourcc_t into the buffer.
 * network format is defined to be first character first
 * -------------------------------------------------------------------- */
void
vchi_writebuf_fourcc( void *_ptr, fourcc_t value )
{
   unsigned char *ptr = (unsigned char *)_ptr;
   ptr[0] = (unsigned char)((value >> 24) & 0xFF);
   ptr[1] = (unsigned char)((value >> 16) & 0xFF);
   ptr[2] = (unsigned char)((value >> 8)  & 0xFF);
   ptr[3] = (unsigned char)((value >> 0)  & 0xFF);
}

/********************************** End of file ******************************************/
