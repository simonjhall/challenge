
SOURCES=timer.c eventgroup.c semaphore.c thread.c main.c library.c mutex.c named_sem.c event.c
SOURCES += logging.c

SOURCES_threadx  = abi.c queue.c cpp.cpp atomic_flags.c
SOURCES_nucleus  = abi.c queue.c
SOURCES_pthreads =
SOURCES_win32    = atomic_flags.c
SOURCES_none     =

SOURCES += $(SOURCES_$(RTOS))

SOURCES_none=timer.c main.c library.c semaphore.c abi.c cpp.cpp eventgroup.c mutex.c named_sem.c event.c atomic_flags.c
SOURCES_none += msgqueue.c

SOURCES += msgqueue.c
VPATH += ../generic

# CPP_SOURCES = cpp.cpp

