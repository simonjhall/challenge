
VPATH += ../generic

libvcos.a: vcos_linuxkernel.o
	$(AR) rs $@ $^

clean:
	rm -f *.o *.a *.d

-include *.d

