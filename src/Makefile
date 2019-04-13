CFLAGS ?= -O3 -g -std=gnu11 -Wall
GCC_OPTS := $(CFLAGS) -fPIC -fvisibility=hidden

TARGETS_MAIN := libcrossld.so crossld.h
TARGETS_DEBUG := libcrossld.o libcrossld.fake.so libcrossld.fake.o

all: $(TARGETS_MAIN) $(TARGETS_DEBUG)

.PHONY: all clean

%.o: %.c
	gcc $(GCC_OPTS) -c -o $@ $<

trampolines.o: trampolines.c crossld.h trampolines.h wrapper.h debug.h
crossld.o: crossld.c crossld.h trampolines.h debug.h

crossld.fake.o: crossld.c crossld.h trampolines.h debug.h
	gcc $(GCC_OPTS) -DCROSSLD_FAKE -c -o $@ $<

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
