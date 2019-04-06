CFLAGS := -O3 -g

all: test test_so testelf32

.PHONY: all clean

%.o: %.c
	gcc $(CFLAGS) -fPIC -c $<

trampolines.o: trampolines.c crossld.h trampolines.h
crossld.o: crossld.c crossld.h trampolines.h
test_main.o: test_main.c crossld.h

wrapper.o: wrapper.asm
	nasm -f elf64 -o $@ $<

test.o: test.asm
	nasm -f elf64 -o $@ $<

test32.o: test.asm
	nasm -f elf32 -o $@ $<

test32_main.o: test32_main.asm
	nasm -f elf32 -o $@ $<

crossld.so: wrapper.o crossld.o trampolines.o
	gcc -shared -fPIC -o $@ $^

testelf32: test32.o test32_main.o
	ld -m elf_i386 -o $@ $^

test: test.o test_main.o wrapper.o crossld.o trampolines.o
	gcc $(CFLAGS) -no-pie -o $@ $^

test_so: test.o test_main.o crossld.so
	LD_RUN_PATH=`pwd` gcc -no-pie -o $@ $^

clean:
	rm -f *.o *.so test test_so testelf32
