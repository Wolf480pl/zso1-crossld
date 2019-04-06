CFLAGS := -O3 -g

all: libcrossld.so libcrossld.o libcrossld.fake.so libcrossld.fake.o crossld.h

.PHONY: all clean

%.o: %.c
	gcc $(CFLAGS) -fPIC -c -o $@ $<

trampolines.o: trampolines.c crossld.h trampolines.h
crossld.o: crossld.c crossld.h trampolines.h

crossld.fake.o: crossld.c crossld.h trampolines.h
	gcc $(CFLAGS) -DFAKE -fPIC -c -o $@ $<	

wrapper.o: wrapper.asm
	nasm -f elf64 -o $@ $<

libcrossld.so: wrapper.o crossld.o trampolines.o
	gcc -shared -fPIC -o $@ $^

libcrossld.fake.so: wrapper.o crossld.fake.o trampolines.o
	gcc -shared -fPIC -o $@ $^

libcrossld.o: wrapper.o crossld.o trampolines.o
	ld -r -o $@ $^

libcrossld.fake.o: wrapper.o crossld.fake.o trampolines.o
	ld -r -o $@ $^

clean:
	rm -f *.o *.so
