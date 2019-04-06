CFLAGS ?= -O3

INCLUDEDIR := ../../include
LIBDIR := ../../src
LIBCROSSLD := $(LIBDIR)/libcrossld.so
LIBCROSSLD_FAKE := $(LIBDIR)/libcrossld.fake.so
STATICCROSSLD := ../../src/libcrossld.o
STATICCROSSLD_FAKE := ../../src/libcrossld.fake.o

TESTS := noload norel noload_so norel_so
OUTS := $(patsubst %,%.out,$(TESTS))

BINARIES := $(TESTS) elf32

all: $(BINARIES)
test: $(OUTS)

.PHONY: all clean test

code32_64.o: code32.asm
	nasm -f elf64 -o $@ $<

code32_32.o: code32.asm
	nasm -f elf32 -o $@ $<

main32.o: main32.asm
	nasm -f elf32 -o $@ $<

elf32: code32_32.o main32.o
	ld -m elf_i386 -o $@ $^

noload: code32_64.o main64_noload.c $(STATICCROSSLD)
	gcc $(CFLAGS) -I $(INCLUDEDIR) -no-pie -o $@ $^

noload_so: code32_64.o main64_noload.c $(LIBCROSSLD)
	LD_RUN_PATH=$(LIBDIR) gcc $(CFLAGS) -I $(INCLUDEDIR) -no-pie -o $@ $^

norel: main64.c $(STATICCROSSLD_FAKE)
	gcc $(CFLAGS) -I $(INCLUDEDIR) -no-pie -o $@ $^

norel_so: main64.c $(LIBCROSSLD_FAKE)
	LD_RUN_PATH=$(LIBDIR) gcc $(CFLAGS) -I $(INCLUDEDIR) -no-pie -o $@ $^

$(OUTS): %.out: %
	./$* >$@

norel.out: elf32
norel_so.out: elf32

clean:
	rm -f $(BINARIES) *.o *.out
