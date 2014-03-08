/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :  Inter-processor communication
Module   :  FIFO
File     :  $RCSfile:  $
Revision :  $Revision: $

FILE DESCRIPTION:

Test of software FIFO code.
=============================================================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "software_fifo.h"
#include "non_wrap_fifo.h"
#include "multiqueue.h"

// defines
#define TEST_FAILURE {failure++; os_assert(0);}
#define TEST_FIFO_SIZE (100)
#define TEST_DATA_SIZE (200)


// function prototypes
static int software_fifo_test(void);
static int non_wrap_fifo_test(void);
static int mqueue_test(void);
static int32_t NON_WRAP_FIFO_READ(NON_WRAP_FIFO_HANDLE_T *fifo, uint8_t * destination)  ;
static int32_t NON_WRAP_FIFO_WRITE(NON_WRAP_FIFO_HANDLE_T *fifo, uint8_t * source, uint32_t length );
int _count(int value);


/***********************************************************
 * Name: main
 * 
 * Arguments: -
 *       
 * Description: Sequences tests of software FIFOs
 *
 * Returns: -
 *
 ***********************************************************/
void main(void)
{
int failure = 0;

   failure += software_fifo_test();
   failure += non_wrap_fifo_test();
   failure += mqueue_test();
   
   if(failure)
      printf("software_fifo tests failed\n");
   else
      printf("software_fifo tests passed\n");
      
}


/***********************************************************
 * Name: software_fifo_test
 * 
 * Arguments: -
 *       
 * Description: Tests standard FIFO (i.e. that wraps around)
 *
 * Returns: 0 if successful, any other value is failure
 *
 ***********************************************************/
static int software_fifo_test(void)
{
int i,j,step;
int failure = 0;
uint8_t test_data[TEST_DATA_SIZE];
uint8_t read_data[TEST_DATA_SIZE];
SOFTWARE_FIFO_HANDLE_T fifo;


   // reset the fifo information
   memset(&fifo,0,sizeof(fifo));

   // set some test data
   for(i=0;i<TEST_DATA_SIZE;i++)
      test_data[i] = i;

   // check that destroying a non-allocated FIFO reports an error
   if(software_fifo_destroy( &fifo ) == 0)
      TEST_FAILURE
   // create a small fifo
   if(software_fifo_create( TEST_FIFO_SIZE, 1, &fifo ))
      TEST_FAILURE
   // check we get an error reading an empty FIFO
   if(software_fifo_read( &fifo, test_data, 10 ) == 0)
      TEST_FAILURE
   // check trying to write too much data reports an error 
   if(software_fifo_write( &fifo, test_data, TEST_FIFO_SIZE+1 ) == 0)
      TEST_FAILURE
   // check the data available reads back as 0
   if(software_fifo_data_available( &fifo ) != 0)
      TEST_FAILURE
   // check the space available is the full size of the FIFO
   if(software_fifo_room_available( &fifo ) != TEST_FIFO_SIZE)
      TEST_FAILURE

   // write some data into the FIFO and read it out and check correct
   for(i=0;i<TEST_DATA_SIZE;i+=10)
   {
      if(software_fifo_write( &fifo, &test_data[i], 10 ))
         TEST_FAILURE
      if(software_fifo_data_available( &fifo ) != 10)
         TEST_FAILURE
      if(software_fifo_room_available( &fifo ) != TEST_FIFO_SIZE-10)
         TEST_FAILURE
      if(software_fifo_read( &fifo, &read_data[i], 10 ))
         TEST_FAILURE
      if(software_fifo_data_available( &fifo ) != 0)
         TEST_FAILURE
      if(software_fifo_room_available( &fifo ) != TEST_FIFO_SIZE)
         TEST_FAILURE
      for(j=i;j<i+10;j++)
      {
         if(test_data[j] != read_data[j])
            TEST_FAILURE
      }
   }

   // continue putting data in the FIFO (so we know we've had a wrap around)
   for(i=0;i<TEST_DATA_SIZE;i++)
      test_data[i] = i*2+20;

   for(i=0;i<TEST_DATA_SIZE;i+=10)
   {
      if(software_fifo_write( &fifo, &test_data[i], 10 ))
         TEST_FAILURE
      if(software_fifo_data_available( &fifo ) != 10)
         TEST_FAILURE
      if(software_fifo_room_available( &fifo ) != TEST_FIFO_SIZE-10)
         TEST_FAILURE
      if(software_fifo_read( &fifo, &read_data[i], 10 ))
         TEST_FAILURE
      if(software_fifo_data_available( &fifo ) != 0)
         TEST_FAILURE
      if(software_fifo_room_available( &fifo ) != TEST_FIFO_SIZE)
         TEST_FAILURE
      for(j=i;j<i+10;j++)
      {
         if(test_data[j] != read_data[j])
            TEST_FAILURE
      }
   }

   // now fill up the FIFO until it is full and then read it all out
   for(i=0;i<TEST_DATA_SIZE;i++)
      test_data[i] = i*3+23;

   for(i=0;i<TEST_FIFO_SIZE;i++)
   {
      if(software_fifo_data_available( &fifo ) != i)
         TEST_FAILURE
      if(software_fifo_room_available( &fifo ) != TEST_FIFO_SIZE-i)
         TEST_FAILURE
      if(software_fifo_write( &fifo, &test_data[i], 1 ))
         TEST_FAILURE
   }
   // check there is no room
   if(software_fifo_room_available( &fifo ) != 0)
      TEST_FAILURE
   // check we get an error if we try to over fill it
   if(software_fifo_write( &fifo, &test_data[i], 1 ) == 0)
      TEST_FAILURE
   // read out all the data and check it is correct
   if(software_fifo_read( &fifo, &read_data[0], TEST_FIFO_SIZE ))
      TEST_FAILURE
   for(i=0;i<TEST_FIFO_SIZE;i++)
   {
      if(test_data[i] != read_data[i])
         TEST_FAILURE
   }

   // make sure a wraparound mid-read / write works as well
   for(i=0;i<TEST_DATA_SIZE;i++)
      test_data[i] = i*5+29;

   for(step = 1;step < 10;step++)
   {
      for(i=0;i<TEST_DATA_SIZE;i+=step)
      {
         if(software_fifo_write( &fifo, &test_data[i], step ))
            TEST_FAILURE
         if(software_fifo_data_available( &fifo ) != step)
            TEST_FAILURE
         if(software_fifo_room_available( &fifo ) != TEST_FIFO_SIZE-step)
            TEST_FAILURE
         if(software_fifo_read( &fifo, &read_data[i], step ))
            TEST_FAILURE
         if(software_fifo_data_available( &fifo ) != 0)
            TEST_FAILURE
         if(software_fifo_room_available( &fifo ) != TEST_FIFO_SIZE)
            TEST_FAILURE
         for(j=i;j<i+step;j++)
         {
            if(test_data[j] != read_data[j])
               TEST_FAILURE
         }
      }
   }
   // check that protecting and unprotecting the structure doesn't cause a problem
   if(software_fifo_protect( &fifo ))
	   TEST_FAILURE
   if(software_fifo_unprotect( &fifo ))
	   TEST_FAILURE

   // check we can close the FIFO without error
   if(software_fifo_destroy( &fifo ))
      TEST_FAILURE
   // check that destroying a freed FIFO reports an error
   if(software_fifo_destroy( &fifo ) == 0)
      TEST_FAILURE
   return(failure);
}


/***********************************************************
 * Name: non_wrap_fifo_test
 * 
 * Arguments: -
 *       
 * Description: Tests non wrap FIFO
 *
 * Returns: 0 if successful, any other value is failure
 *
 ***********************************************************/
static int non_wrap_fifo_test(void)
{
int i,j;
int failure = 0;
NON_WRAP_FIFO_HANDLE_T fifo;
uint8_t test_data[TEST_DATA_SIZE];
uint8_t read_data[TEST_DATA_SIZE];
uint8_t * address;
uint8_t * address1;
uint32_t length;

#define NO_OF_ENTRIES 8

   // reset the fifo information
   memset(&fifo,0,sizeof(fifo));
   // set some test data
   for(i=0;i<TEST_DATA_SIZE;i++)
      test_data[i] = i;

   // check that destroying a non-allocated FIFO reports an error
   if(non_wrap_fifo_destroy( &fifo ) == 0)
      TEST_FAILURE
   // create a small fifo
   if(non_wrap_fifo_create( TEST_FIFO_SIZE, 16, NO_OF_ENTRIES, &fifo ))
      TEST_FAILURE
   // check we get an error reading an empty FIFO
   if(non_wrap_fifo_read( &fifo, test_data, 10 ) == 0)
      TEST_FAILURE
   // check trying to write too much data reports an error 
   if(non_wrap_fifo_write( &fifo, test_data, TEST_FIFO_SIZE+1 ) == 0)
      TEST_FAILURE

   // write some data into the FIFO and read it out and check correct
   for(i=0;i<TEST_DATA_SIZE;i+=10)
   {
      if(non_wrap_fifo_write( &fifo, &test_data[i], 10 ))
         TEST_FAILURE
      if(non_wrap_fifo_read( &fifo, &read_data[i], 10 ))
         TEST_FAILURE
      for(j=i;j<i+10;j++)
      {
         if(test_data[j] != read_data[j])
            TEST_FAILURE
      }
   }

   // destroy the FIFO
   if(non_wrap_fifo_destroy( &fifo ))
      TEST_FAILURE
   // and restart it
   if(non_wrap_fifo_create( TEST_FIFO_SIZE, 16, NO_OF_ENTRIES, &fifo ))
      TEST_FAILURE
   // now test various wrap round conditions
   if(non_wrap_fifo_write( &fifo, test_data, 90 ))
      TEST_FAILURE
   // try and write too much - expect an error
   if(non_wrap_fifo_write( &fifo, test_data, 11 ) == 0)
      TEST_FAILURE
   // read out the data and check it
   if(non_wrap_fifo_read( &fifo, read_data, 90 ))
      TEST_FAILURE
   for(j=0;j<90;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // the above should have put the next write address to the start of the FIFO
   if(non_wrap_fifo_write( &fifo, &test_data[0], 20 ))
      TEST_FAILURE
   if(non_wrap_fifo_write( &fifo, &test_data[20], 20 ))
      TEST_FAILURE
   if(non_wrap_fifo_write( &fifo, &test_data[40], 20 ))
      TEST_FAILURE
   if(non_wrap_fifo_write( &fifo, &test_data[60], 20 ))
      TEST_FAILURE
   // this should cause an error as there is not enough room
   if(non_wrap_fifo_write( &fifo, &test_data[80], 30 ) == 0)
      TEST_FAILURE
   // read and test the first message
   if(non_wrap_fifo_read( &fifo, &read_data[0], 20 ))
      TEST_FAILURE
   for(j=0;j<20;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // this should cause an error as there is not enough room
   if(non_wrap_fifo_write( &fifo, &test_data[80], 30 ) == 0)
      TEST_FAILURE
   // read and test the second message
   if(non_wrap_fifo_read( &fifo, &read_data[20], 20 ))
      TEST_FAILURE
   for(j=20;j<40;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // now there is room for the 30 message
   if(non_wrap_fifo_write( &fifo, &test_data[80], 30 ))
      TEST_FAILURE
   // now fill the remaining space exactly
   if(non_wrap_fifo_write( &fifo, &test_data[110], 10 ))
      TEST_FAILURE
   // now check that it thinks it is full (doesn't try to use room at the end)
   if(non_wrap_fifo_write( &fifo, &test_data[120], 1 ) == 0)
      TEST_FAILURE
   // read and test the third message
   if(non_wrap_fifo_read( &fifo, &read_data[40], 20 ))
      TEST_FAILURE
   for(j=40;j<60;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // read and test the fourth message
   if(non_wrap_fifo_read( &fifo, &read_data[60], 20 ))
      TEST_FAILURE
   for(j=60;j<80;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // now check that the correct amount of free space is seen
   if(non_wrap_fifo_write( &fifo, &test_data[120], 60 ))
      TEST_FAILURE
   // now check that it thinks it is full
   if(non_wrap_fifo_write( &fifo, &test_data[180], 1 ) == 0)
      TEST_FAILURE
   // read and test the fifth message
   if(non_wrap_fifo_read( &fifo, &read_data[80], 30 ))
      TEST_FAILURE
   for(j=80;j<110;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // write a message - should be at start of FIFO
   if(non_wrap_fifo_write( &fifo, &test_data[0], 20 ))
      TEST_FAILURE
   // read the sixth message
   if(non_wrap_fifo_read( &fifo, &read_data[110], 10 ))
      TEST_FAILURE
   for(j=110;j<120;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // write another message - should fill the FIFO again
   if(non_wrap_fifo_write( &fifo, &test_data[20], 20 ))
      TEST_FAILURE
   // now check that it thinks it is full
   if(non_wrap_fifo_write( &fifo, &test_data[180], 1 ) == 0)
      TEST_FAILURE
   // read the big message in second half of FIFO
   if(non_wrap_fifo_read( &fifo, &read_data[120], 60 ))
      TEST_FAILURE
   // test the data is correct
   for(j=120;j<180;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }

   // Now check that you can't have more than the maximum number of messages in the FIFO 
   // destroy the FIFO
   if(non_wrap_fifo_destroy( &fifo ))
      TEST_FAILURE
   // and restart it
   if(non_wrap_fifo_create( TEST_FIFO_SIZE, 16, NO_OF_ENTRIES, &fifo ))
      TEST_FAILURE
   for(i=0;i<NO_OF_ENTRIES;i++)
   {
      if(non_wrap_fifo_write( &fifo, &test_data[i*10], 10 ))
         TEST_FAILURE
   }
   // check that the next write results in an error
   if(non_wrap_fifo_write( &fifo, &test_data[0], 10 ) == 0)
         TEST_FAILURE
   // check the data can be correctly read out
   for(i=0;i<NO_OF_ENTRIES;i++)
   {
      if(non_wrap_fifo_read( &fifo, &read_data[i*10], 10 ))
         TEST_FAILURE
      for(j=i*10;j<i*10+10;j++)
	  {
		  if(test_data[j] != read_data[j])
			  TEST_FAILURE
	  }
   }

   // check we can close the FIFO without error
   if(non_wrap_fifo_destroy( &fifo ))
      TEST_FAILURE
   // check that destroying a freed FIFO reports an error
   if(non_wrap_fifo_destroy( &fifo ) == 0)
      TEST_FAILURE

   // now repeat most of the tests using off line copying (as if it were being done by DMA)
   if(non_wrap_fifo_create( TEST_FIFO_SIZE, 16, NO_OF_ENTRIES, &fifo ))
      TEST_FAILURE
   // check we get an error reading an empty FIFO
   if(non_wrap_fifo_request_read_address( &fifo, &address, &length ) == 0)
      TEST_FAILURE
   // check trying to write too much data reports an error 
   if(non_wrap_fifo_request_write_address( &fifo, &address, TEST_FIFO_SIZE+1 ) == 0)
      TEST_FAILURE

   // write some data into the FIFO and read it out and check correct
   for(i=0;i<TEST_DATA_SIZE;i+=10)
   {
      // do the write
      if(non_wrap_fifo_request_write_address( &fifo, &address, 10 ))
         TEST_FAILURE
      memcpy(address,&test_data[i],10);
      if(non_wrap_fifo_write_complete( &fifo, address ))
		 TEST_FAILURE
      // do the read
      if(non_wrap_fifo_request_read_address( &fifo, &address, &length ))
         TEST_FAILURE
      if(length != 10)
	     TEST_FAILURE
      memcpy(&read_data[i],address,length);
      if(non_wrap_fifo_read_complete( &fifo, address ))
		 TEST_FAILURE
      // test the read back values
      for(j=i;j<i+10;j++)
      {
         if(test_data[j] != read_data[j])
            TEST_FAILURE
      }
   }

   // destroy the FIFO
   if(non_wrap_fifo_destroy( &fifo ))
      TEST_FAILURE
   // and restart it
   if(non_wrap_fifo_create( TEST_FIFO_SIZE, 16, NO_OF_ENTRIES, &fifo ))
      TEST_FAILURE

	  
	  
	  
	  // now test various wrap round conditions
   if(NON_WRAP_FIFO_WRITE( &fifo, test_data, 90 ))
      TEST_FAILURE
   // try and write too much - expect an error
   if(NON_WRAP_FIFO_WRITE( &fifo, test_data, 11 ) == 0)
      TEST_FAILURE
   // read out the data and check it
   if(NON_WRAP_FIFO_READ( &fifo, read_data ))
      TEST_FAILURE
   for(j=0;j<90;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // the above should have put the next write address to the start of the FIFO
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[0], 20 ))
      TEST_FAILURE
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[20], 20 ))
      TEST_FAILURE
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[40], 20 ))
      TEST_FAILURE
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[60], 20 ))
      TEST_FAILURE
   // this should cause an error as there is not enough room
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[80], 30 ) == 0)
      TEST_FAILURE
   // read and test the first message
   if(NON_WRAP_FIFO_READ( &fifo, &read_data[0] ))
      TEST_FAILURE
   for(j=0;j<20;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // this should cause an error as there is not enough room
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[80], 30 ) == 0)
      TEST_FAILURE
   // read and test the second message
   if(NON_WRAP_FIFO_READ( &fifo, &read_data[20] ))
      TEST_FAILURE
   for(j=20;j<40;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // now there is room for the 30 message
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[80], 30 ))
      TEST_FAILURE
   // now fill the remaining space exactly
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[110], 10 ))
      TEST_FAILURE
   // now check that it thinks it is full (doesn't try to use room at the end)
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[120], 1 ) == 0)
      TEST_FAILURE
   // read and test the third message
   if(NON_WRAP_FIFO_READ( &fifo, &read_data[40] ))
      TEST_FAILURE
   for(j=40;j<60;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // read and test the fourth message
   if(NON_WRAP_FIFO_READ( &fifo, &read_data[60] ))
      TEST_FAILURE
   for(j=60;j<80;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // now check that the correct amount of free space is seen
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[120], 60 ))
      TEST_FAILURE
   // now check that it thinks it is full
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[180], 1 ) == 0)
      TEST_FAILURE
   // read and test the fifth message
   if(NON_WRAP_FIFO_READ( &fifo, &read_data[80] ))
      TEST_FAILURE
   for(j=80;j<110;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // write a message - should be at start of FIFO
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[0], 20 ))
      TEST_FAILURE
   // read the sixth message
   if(NON_WRAP_FIFO_READ( &fifo, &read_data[110] ))
      TEST_FAILURE
   for(j=110;j<120;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // write another message - should fill the FIFO again
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[20], 20 ))
      TEST_FAILURE
   // now check that it thinks it is full
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[180], 1 ) == 0)
      TEST_FAILURE
   // read the big message in second half of FIFO
   if(NON_WRAP_FIFO_READ( &fifo, &read_data[120] ))
      TEST_FAILURE
   // test the data is correct
   for(j=120;j<180;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }

   // Now check that you can't have more than the maximum number of messages in the FIFO 
   if(non_wrap_fifo_destroy( &fifo ))
      TEST_FAILURE
   // and restart it
   if(non_wrap_fifo_create( TEST_FIFO_SIZE, 16, NO_OF_ENTRIES, &fifo ))
      TEST_FAILURE
   for(i=0;i<NO_OF_ENTRIES;i++)
   {
      if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[i*10], 10 ))
         TEST_FAILURE
   }
   // check that the next write results in an error
   if(NON_WRAP_FIFO_WRITE( &fifo, &test_data[0], 10 ) == 0)
         TEST_FAILURE
   // check the data can be correctly read out
   for(i=0;i<NO_OF_ENTRIES;i++)
   {
      if(NON_WRAP_FIFO_READ( &fifo, &read_data[i*10] ))
         TEST_FAILURE
      for(j=i*10;j<i*10+10;j++)
	  {
		  if(test_data[j] != read_data[j])
			  TEST_FAILURE
	  }
   }
   if(non_wrap_fifo_destroy( &fifo ))
      TEST_FAILURE

   // now check what happens when we try to read complete after a number of entries have been added
   if(non_wrap_fifo_create( TEST_FIFO_SIZE, 16, NO_OF_ENTRIES, &fifo ))
      TEST_FAILURE
   // check we get an error reading an empty FIFO
   if(non_wrap_fifo_request_read_address( &fifo, &address, &length ) == 0)
      TEST_FAILURE
   // check trying to write too much data reports an error 
   if(non_wrap_fifo_request_write_address( &fifo, &address, TEST_FIFO_SIZE+1 ) == 0)
      TEST_FAILURE

   // write some data into the FIFO and read it out and check correct
   // do the first write
   if(non_wrap_fifo_request_write_address( &fifo, &address, 10 ))
      TEST_FAILURE
   memcpy(address,&test_data[0],10);
   if(non_wrap_fifo_write_complete( &fifo, address ))
      TEST_FAILURE
   // do the second write
   if(non_wrap_fifo_request_write_address( &fifo, &address, 10 ))
      TEST_FAILURE
   memcpy(address,&test_data[10],10);
   if(non_wrap_fifo_write_complete( &fifo, address ))
	  TEST_FAILURE
   // do the third write
   if(non_wrap_fifo_request_write_address( &fifo, &address, 10 ))
      TEST_FAILURE
   memcpy(address,&test_data[20],10);
   if(non_wrap_fifo_write_complete( &fifo, address ))
	  TEST_FAILURE
   // do the read, of the first entry
   if(non_wrap_fifo_request_read_address( &fifo, &address, &length ))
      TEST_FAILURE
   if(length != 10)
      TEST_FAILURE
   memcpy(&read_data[0],address,length);
   if(non_wrap_fifo_read_complete( &fifo, address ))
	  TEST_FAILURE
   // test the read back values
   for(j=0;j<10;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // do the read, of the second entry
   if(non_wrap_fifo_request_read_address( &fifo, &address, &length ))
      TEST_FAILURE
   if(length != 10)
      TEST_FAILURE
   memcpy(&read_data[0],address,length);
   if(non_wrap_fifo_read_complete( &fifo, address ))
	  TEST_FAILURE
   // test the read back values
   for(j=0;j<10;j++)
   {
      if(test_data[j+10] != read_data[j])
         TEST_FAILURE
   }
   // do the read, of the third entry
   if(non_wrap_fifo_request_read_address( &fifo, &address, &length ))
      TEST_FAILURE
   if(length != 10)
      TEST_FAILURE
   memcpy(&read_data[0],address,length);
   if(non_wrap_fifo_read_complete( &fifo, address ))
	  TEST_FAILURE
   // test the read back values
   for(j=0;j<10;j++)
   {
      if(test_data[j+20] != read_data[j])
         TEST_FAILURE
   }
   // destroy the FIFO
   if(non_wrap_fifo_destroy( &fifo ))
      TEST_FAILURE


   // Check the specific case where the first entry is written and read and then further multiple entries before reading
   // do the first write
   if(non_wrap_fifo_create( TEST_FIFO_SIZE, 16, NO_OF_ENTRIES, &fifo ))
      TEST_FAILURE
   if(non_wrap_fifo_request_write_address( &fifo, &address, 10 ))
      TEST_FAILURE
   memcpy(address,&test_data[0],10);
   if(non_wrap_fifo_write_complete( &fifo, address ))
      TEST_FAILURE
   // do the read of the first entry
   if(non_wrap_fifo_request_read_address( &fifo, &address, &length ))
      TEST_FAILURE
   if(length != 10)
      TEST_FAILURE
   memcpy(&read_data[0],address,length);
   if(non_wrap_fifo_read_complete( &fifo, address ))
	  TEST_FAILURE
   // test the read back values
   for(j=0;j<10;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // do the second write
   if(non_wrap_fifo_request_write_address( &fifo, &address, 10 ))
      TEST_FAILURE
   memcpy(address,&test_data[10],10);
   if(non_wrap_fifo_write_complete( &fifo, address ))
	  TEST_FAILURE
   // do the third write
   if(non_wrap_fifo_request_write_address( &fifo, &address, 10 ))
      TEST_FAILURE
   memcpy(address,&test_data[20],10);
   if(non_wrap_fifo_write_complete( &fifo, address ))
	  TEST_FAILURE
   // do the fourth write
   if(non_wrap_fifo_request_write_address( &fifo, &address, 10 ))
      TEST_FAILURE
   memcpy(address,&test_data[30],10);
   if(non_wrap_fifo_write_complete( &fifo, address ))
	  TEST_FAILURE
   // do the read, of the second entry
   if(non_wrap_fifo_request_read_address( &fifo, &address, &length ))
      TEST_FAILURE
   if(length != 10)
      TEST_FAILURE
   memcpy(&read_data[0],address,length);
   if(non_wrap_fifo_read_complete( &fifo, address ))
	  TEST_FAILURE
   // test the read back values
   for(j=0;j<10;j++)
   {
      if(test_data[j+10] != read_data[j])
         TEST_FAILURE
   }
   // do the read, of the third entry
   if(non_wrap_fifo_request_read_address( &fifo, &address, &length ))
      TEST_FAILURE
   if(length != 10)
      TEST_FAILURE
   memcpy(&read_data[0],address,length);
   if(non_wrap_fifo_read_complete( &fifo, address ))
	  TEST_FAILURE
   // test the read back values
   for(j=0;j<10;j++)
   {
      if(test_data[j+20] != read_data[j])
         TEST_FAILURE
   }
   // do the read, of the fourth entry
   if(non_wrap_fifo_request_read_address( &fifo, &address, &length ))
      TEST_FAILURE
   if(length != 10)
      TEST_FAILURE
   memcpy(&read_data[0],address,length);
   if(non_wrap_fifo_read_complete( &fifo, address ))
	  TEST_FAILURE
   // test the read back values
   for(j=0;j<10;j++)
   {
      if(test_data[j+30] != read_data[j])
         TEST_FAILURE
   }
   // destroy the FIFO
   if(non_wrap_fifo_destroy( &fifo ))
      TEST_FAILURE




   // Check the remove functionality
   // do the first write
   if(non_wrap_fifo_create( TEST_FIFO_SIZE, 16, NO_OF_ENTRIES, &fifo ))
      TEST_FAILURE
   if(non_wrap_fifo_request_write_address( &fifo, &address, 10 ))
      TEST_FAILURE
   memcpy(address,&test_data[0],10);
   if(non_wrap_fifo_write_complete( &fifo, address ))
      TEST_FAILURE
   // do the read of the first entry
   if(non_wrap_fifo_request_read_address( &fifo, &address, &length ))
      TEST_FAILURE
   if(length != 10)
      TEST_FAILURE
   memcpy(&read_data[0],address,length);
   if(non_wrap_fifo_read_complete( &fifo, address ))
	  TEST_FAILURE
   // test the read back values
   for(j=0;j<10;j++)
   {
      if(test_data[j] != read_data[j])
         TEST_FAILURE
   }
   // do the second write
   if(non_wrap_fifo_request_write_address( &fifo, &address1, 10 ))
      TEST_FAILURE
   memcpy(address1,&test_data[10],10);
   if(non_wrap_fifo_write_complete( &fifo, address1 ))
	  TEST_FAILURE
   // do the third write
   if(non_wrap_fifo_request_write_address( &fifo, &address, 10 ))
      TEST_FAILURE
   memcpy(address,&test_data[20],10);
   if(non_wrap_fifo_write_complete( &fifo, address ))
	  TEST_FAILURE
   // do the fourth write
   if(non_wrap_fifo_request_write_address( &fifo, &address, 10 ))
      TEST_FAILURE
   memcpy(address,&test_data[30],10);
   if(non_wrap_fifo_write_complete( &fifo, address ))
	  TEST_FAILURE
   // remove the second entry
   if(non_wrap_fifo_remove( &fifo, address1 ))
      TEST_FAILURE
   // do the read, of the third entry
   if(non_wrap_fifo_request_read_address( &fifo, &address, &length ))
      TEST_FAILURE
   if(length != 10)
      TEST_FAILURE
   memcpy(&read_data[0],address,length);
   if(non_wrap_fifo_read_complete( &fifo, address ))
	  TEST_FAILURE
   // test the read back values
   for(j=0;j<10;j++)
   {
      if(test_data[j+20] != read_data[j])
         TEST_FAILURE
   }
   // do the read, of the fourth entry
   if(non_wrap_fifo_request_read_address( &fifo, &address, &length ))
      TEST_FAILURE
   if(length != 10)
      TEST_FAILURE
   memcpy(&read_data[0],address,length);
   if(non_wrap_fifo_read_complete( &fifo, address ))
	  TEST_FAILURE
   // test the read back values
   for(j=0;j<10;j++)
   {
      if(test_data[j+30] != read_data[j])
         TEST_FAILURE
   }
   // destroy the FIFO
   if(non_wrap_fifo_destroy( &fifo ))
      TEST_FAILURE



   return(failure);
}

static int32_t NON_WRAP_FIFO_WRITE(NON_WRAP_FIFO_HANDLE_T *fifo, uint8_t * source, uint32_t length ) 
{ 
uint8_t * address;

   if(non_wrap_fifo_request_write_address( fifo, &address, length )) 
	  return(-1);  
   memcpy(address,source,length); 
   if(non_wrap_fifo_write_complete( fifo, address )) 
	  return(-1);
   return(0);
}


static int32_t NON_WRAP_FIFO_READ(NON_WRAP_FIFO_HANDLE_T *fifo, uint8_t * destination)  
{ 
uint8_t * address;
uint32_t length;

   if(non_wrap_fifo_request_read_address( fifo, &address, &length )) 
      return(-1);
   memcpy(destination,address,length); 
   if(non_wrap_fifo_read_complete( fifo, address )) 
      return(-1);
   return(0);
}

int _count(int value)
{
   int count = 0;
   while (value)
   {
      // clear the least significant bit set in val
      value &= (value-1);
      count++;
   }

   return count;
}


/***********************************************************
 * Name: mqueue_test
 * 
 * Arguments: -
 *       
 * Description: Tests multi queue
 *
 * Returns: 0 if successful, any other value is failure
 *
 ***********************************************************/
struct mqueue_test {

   struct mqueue_test *next;
   int test1;
   int test2;
};
typedef struct mqueue_test MQUEUE_TEST_T;

int mqueue_test(void)
{
int failure = 0;
VCHI_MQUEUE_T * mq;
MQUEUE_TEST_T * element;

   // 3 queues, 5 entries in total
   mq = vchi_mqueue_create( 3, 5, sizeof(MQUEUE_TEST_T) );
   // check that it can be destroyed
   if(vchi_mqueue_destroy( mq ))
	   TEST_FAILURE

   // create another multiqueue
   mq = vchi_mqueue_create( 3, 5, sizeof(MQUEUE_TEST_T) );
   // move all the elements on to the second queue whilst changing the value

   // element 0
   element = vchi_mqueue_peek(mq,0);
   element->test1 = 0;
   element->test2 = 1;
   if(vchi_mqueue_move( mq, 0, 1 ) != element)
	   TEST_FAILURE
   // element 1
   element = vchi_mqueue_peek(mq,0);
   element->test1 = 2;
   element->test2 = 3;
   if(vchi_mqueue_move( mq, 0, 1 ) != element)
	   TEST_FAILURE
   // element 2
   element = vchi_mqueue_peek(mq,0);
   element->test1 = 4;
   element->test2 = 5;
   if(vchi_mqueue_move( mq, 0, 1 ) != element)
	   TEST_FAILURE
   // element 3
   element = vchi_mqueue_peek(mq,0);
   element->test1 = 6;
   element->test2 = 7;
   if(vchi_mqueue_move( mq, 0, 1 ) != element)
	   TEST_FAILURE
   // element 4
   element = vchi_mqueue_peek(mq,0);
   element->test1 = 8;
   element->test2 = 9;
   if(vchi_mqueue_move( mq, 0, 1 ) != element)
	   TEST_FAILURE
   // queue 0 is empty so check we get a NULL pointer when we peek
   if(vchi_mqueue_peek(mq,0) != NULL)
	   TEST_FAILURE
   // check that all is as we expect on queue 1
   // element 0
   element = vchi_mqueue_peek(mq,1);
   if(element->test1 != 0) TEST_FAILURE
   if(element->test2 != 1) TEST_FAILURE
   if(vchi_mqueue_move( mq, 1, 2 ) != element) TEST_FAILURE
   // element 1
   element = vchi_mqueue_peek(mq,1);
   if(element->test1 != 2) TEST_FAILURE
   if(element->test2 != 3) TEST_FAILURE
   if(vchi_mqueue_move( mq, 1, 2 ) != element) TEST_FAILURE
   // element 2
   element = vchi_mqueue_peek(mq,1);
   if(element->test1 != 4) TEST_FAILURE
   if(element->test2 != 5) TEST_FAILURE
   if(vchi_mqueue_move( mq, 1, 2 ) != element) TEST_FAILURE
   // element 3
   element = vchi_mqueue_peek(mq,1);
   if(element->test1 != 6) TEST_FAILURE
   if(element->test2 != 7) TEST_FAILURE
   if(vchi_mqueue_move( mq, 1, 2 ) != element) TEST_FAILURE
   // element 4
   element = vchi_mqueue_peek(mq,1);
   if(element->test1 != 8) TEST_FAILURE
   if(element->test2 != 9) TEST_FAILURE
   if(vchi_mqueue_move( mq, 1, 2 ) != element) TEST_FAILURE

   // check that removing a null element generates an error
   if(vchi_mqueue_element_free( mq, 2, NULL ) == 0)
	   TEST_FAILURE
   // similarly try removing something from an empty queue
   if(vchi_mqueue_element_free( mq, 0, NULL ) == 0)
	   TEST_FAILURE
   // remove the second item in the no. 2 queue
   element = vchi_mqueue_peek(mq,2);
   if(vchi_mqueue_element_free( mq, 2, element->next ))
	   TEST_FAILURE
   // move the other elements to queue 1 checking them as we go
   element = vchi_mqueue_peek(mq,2);
   if(element->test1 != 0) TEST_FAILURE
   if(element->test2 != 1) TEST_FAILURE
   if(vchi_mqueue_move( mq, 2, 1 ) != element) TEST_FAILURE
   // element 2
   element = vchi_mqueue_peek(mq,2);
   if(element->test1 != 4) TEST_FAILURE
   if(element->test2 != 5) TEST_FAILURE
   if(vchi_mqueue_move( mq, 2, 1 ) != element) TEST_FAILURE
   // element 3
   element = vchi_mqueue_peek(mq,2);
   if(element->test1 != 6) TEST_FAILURE
   if(element->test2 != 7) TEST_FAILURE
   if(vchi_mqueue_move( mq, 2, 1 ) != element) TEST_FAILURE
   // element 4
   element = vchi_mqueue_peek(mq,2);
   if(element->test1 != 8) TEST_FAILURE
   if(element->test2 != 9) TEST_FAILURE
   if(vchi_mqueue_move( mq, 2, 1 ) != element) TEST_FAILURE
   // remove the last element of queue 1
   if(vchi_mqueue_element_free( mq, 1, element ))
	   TEST_FAILURE
   // check we have removed the last element
   element = vchi_mqueue_peek(mq,1);
   if(element->test1 != 0) TEST_FAILURE
   if(element->test2 != 1) TEST_FAILURE
   if(vchi_mqueue_move( mq, 1, 2 ) != element) TEST_FAILURE
   // element 2
   element = vchi_mqueue_peek(mq,1);
   if(element->test1 != 4) TEST_FAILURE
   if(element->test2 != 5) TEST_FAILURE
   if(vchi_mqueue_move( mq, 1, 2 ) != element) TEST_FAILURE
   // element 3
   element = vchi_mqueue_peek(mq,1);
   if(element->test1 != 6) TEST_FAILURE
   if(element->test2 != 7) TEST_FAILURE
   if(vchi_mqueue_move( mq, 1, 2 ) != element) TEST_FAILURE
   // and confirm that the last element is NULL
   element = vchi_mqueue_peek(mq,1);
   if(element != NULL)
	   TEST_FAILURE
   // check that it can be destroyed
   if(vchi_mqueue_destroy( mq ))
	   TEST_FAILURE






// ***********************************************************************************************************************


   // create another multiqueue
   mq = vchi_mqueue_create( 3, 5, sizeof(MQUEUE_TEST_T) );
   // move all the elements on to the second queue whilst changing the value

   // element 0
   element = vchi_mqueue_get(mq,0);
   element->test1 = 0;
   element->test2 = 1;
   vchi_mqueue_put( mq, 1, element );
   // element 1
   element = vchi_mqueue_get(mq,0);
   element->test1 = 2;
   element->test2 = 3;
   vchi_mqueue_put( mq, 1, element );
   // element 2
   element = vchi_mqueue_get(mq,0);
   element->test1 = 4;
   element->test2 = 5;
   vchi_mqueue_put( mq, 1, element );
   // element 3
   element = vchi_mqueue_get(mq,0);
   element->test1 = 6;
   element->test2 = 7;
   vchi_mqueue_put( mq, 1, element );
   // element 4
   element = vchi_mqueue_get(mq,0);
   element->test1 = 8;
   element->test2 = 9;
   vchi_mqueue_put( mq, 1, element );
   // queue 0 is empty so check we get a NULL pointer when we peek
   if(vchi_mqueue_peek(mq,0) != NULL)
	   TEST_FAILURE
   // check that all is as we expect on queue 1
   // element 0
   element = vchi_mqueue_get(mq,1);
   if(element->test1 != 0) TEST_FAILURE
   if(element->test2 != 1) TEST_FAILURE
   vchi_mqueue_put( mq, 2, element );
   // element 1
   element = vchi_mqueue_get(mq,1);
   if(element->test1 != 2) TEST_FAILURE
   if(element->test2 != 3) TEST_FAILURE
   vchi_mqueue_put( mq, 2, element);
   // element 2
   element = vchi_mqueue_get(mq,1);
   if(element->test1 != 4) TEST_FAILURE
   if(element->test2 != 5) TEST_FAILURE
   vchi_mqueue_put( mq, 2, element );
   // element 3
   element = vchi_mqueue_get(mq,1);
   if(element->test1 != 6) TEST_FAILURE
   if(element->test2 != 7) TEST_FAILURE
   vchi_mqueue_put( mq, 2, element );
   // element 4
   element = vchi_mqueue_get(mq,1);
   if(element->test1 != 8) TEST_FAILURE
   if(element->test2 != 9) TEST_FAILURE
   vchi_mqueue_put( mq, 2, element );

   // check that removing a null element generates an error
   if(vchi_mqueue_element_free( mq, 2, NULL ) == 0)
	   TEST_FAILURE
   // similarly try removing something from an empty queue
   if(vchi_mqueue_element_free( mq, 0, NULL ) == 0)
	   TEST_FAILURE
   // remove the second item in the no. 2 queue
   element = vchi_mqueue_peek(mq,2);
   if(vchi_mqueue_element_free( mq, 2, element->next ))
	   TEST_FAILURE
   // move the other elements to queue 1 checking them as we go
   element = vchi_mqueue_get(mq,2);
   if(element->test1 != 0) TEST_FAILURE
   if(element->test2 != 1) TEST_FAILURE
   vchi_mqueue_put( mq, 1, element );
   // element 2
   element = vchi_mqueue_get(mq,2);
   if(element->test1 != 4) TEST_FAILURE
   if(element->test2 != 5) TEST_FAILURE
   vchi_mqueue_put( mq, 1, element );
   // element 3
   element = vchi_mqueue_get(mq,2);
   if(element->test1 != 6) TEST_FAILURE
   if(element->test2 != 7) TEST_FAILURE
   vchi_mqueue_put( mq, 1, element );
   // element 4
   element = vchi_mqueue_get(mq,2);
   if(element->test1 != 8) TEST_FAILURE
   if(element->test2 != 9) TEST_FAILURE
   vchi_mqueue_put( mq, 1, element );
   // remove the last element of queue 1
   if(vchi_mqueue_element_free( mq, 1, element ))
	   TEST_FAILURE
   // check we have removed the last element
   element = vchi_mqueue_get(mq,1);
   if(element->test1 != 0) TEST_FAILURE
   if(element->test2 != 1) TEST_FAILURE
   vchi_mqueue_put( mq, 2, element );
   // element 2
   element = vchi_mqueue_get(mq,1);
   if(element->test1 != 4) TEST_FAILURE
   if(element->test2 != 5) TEST_FAILURE
   vchi_mqueue_put( mq, 2, element );
   // element 3
   element = vchi_mqueue_get(mq,1);
   if(element->test1 != 6) TEST_FAILURE
   if(element->test2 != 7) TEST_FAILURE
   vchi_mqueue_put( mq, 2, element );
   // and confirm that the last element is NULL
   element = vchi_mqueue_peek(mq,1);
   if(element != NULL)
	   TEST_FAILURE
   // check that it can be destroyed
   if(vchi_mqueue_destroy( mq ))
	   TEST_FAILURE

// ***************************************************************************************************************************
   return(failure);
}



// faked up routines that are in the os directory
int32_t os_init( void )                                                                    { return(0); }
int32_t os_semaphore_create(  OS_SEMAPHORE_T *semaphore,  const OS_SEMAPHORE_TYPE_T type ) { return(0); }
int32_t os_semaphore_destroy( OS_SEMAPHORE_T *semaphore )                                  { return(0); }
int32_t os_semaphore_obtain( OS_SEMAPHORE_T *semaphore )                                   { return(0); }
int32_t os_semaphore_release( OS_SEMAPHORE_T *semaphore )                                  { return(0); }
void *os_malloc( const uint32_t size )                                                     { return (void *)malloc( size ); }
int32_t os_free( void *ptr )                                                               { free( ptr ); return(0); }

/* ************************************ The End ***************************************** */
