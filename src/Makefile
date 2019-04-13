CFLAGS ?= -O2 -std=gnu11
GCC_OPTS := $(CFLAGS) -fPIC -fvisibility=hidden

TARGETS_MAIN := libcrossld.so crossld.h

all: $(TARGETS_MAIN)

.PHONY: all clean

%.o: %.c
	gcc $(GCC_OPTS) -c -o $@ $<

trampolines.o: trampolines.c crossld.h trampolines.h wrapper.h debug.h
crossld.o: crossld.c crossld.h trampolines.h debug.h

wrapper.o: wrapper.asm
	nasm -f elf64 -o $@ $<

libcrossld.so: wrapper.o crossld.o trampolines.o
	gcc -shared -fPIC -o $@ $^

clean:
	rm -f *.o *.so
