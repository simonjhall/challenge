
cmake_minimum_required(VERSION 2.8)
project(vcos_test)


set(RTOS pthreads)

include_directories(../${RTOS})
include_directories(../../..)

add_subdirectory(../${RTOS} vcos_${RTOS})

set(extras_pthreads msgqueue.c)


add_executable(vcos_test
   timer.c eventgroup.c semaphore.c thread.c main.c
   library.c mutex.c named_sem.c event.c ${extras_${RTOS}})

target_link_libraries(vcos_test vcos)




