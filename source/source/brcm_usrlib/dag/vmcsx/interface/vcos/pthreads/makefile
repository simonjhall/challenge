
VPATH += ../generic
VPATH += ../glibc

libvcos.a: vcos_pthreads.o vcos_generic_event_flags.o  \
            vcos_mem_from_malloc.o vcos_generic_named_sem.o \
            vcos_generic_reentrant_mtx.o  vcos_abort.o \
            vcos_msgqueue.o vcos_logcat.o vcos_backtrace.o
	$(AR) rs $@ $^

clean:
	rm -f *.o *.a *.d

-include *.d

