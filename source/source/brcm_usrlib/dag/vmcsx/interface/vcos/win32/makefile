
VPATH += ../generic

SOURCES = vcos_win32.c vcos_generic_event_flags.c vcos_joinable_thread_from_plain.c \
            vcos_mem_from_malloc.c vcos_msgqueue.c vcos_abort.c \
            vcos_logcat.c

libvcos.a: $(SOURCES:.c=.obj)
	$(AR) rs $@ $^

clean:
	rm -f *.o *.a *.d *.obj

include rules.mk

-include *.d

