CFLAGS ?= -O3

INCLUDEDIR := ../../include
LIBDIR := ../../src
LIBCROSSLD := $(LIBDIR)/libcrossld.so

BADELFS_RAW := badelf0 badelf0a
BADELFS_LD := badelf1 badelf2 badelf3 badelf4

BADELFS_LD_SCRIPTS := $(patsubst %,%.ld,$(BADELFS_LD)) 
BADELFS := $(BADELFS_RAW) $(BADELFS_LD)

BADELFS_OUTS := $(patsubst %,%.out,$(BADELFS))

all: main64 $(BADELFS)
clean:
	rm -f main64 *.so *.o $(BADELFS_LD)

.PHONY: all clean test

main64: main64.c $(LIBCROSSLD)
	LD_RUN_PATH=$(LIBDIR) gcc $(CFLAGS) -I $(INCLUDEDIR) -o $@ $^

empty.o: empty.asm
	nasm -f elf32 -o $@ $<

#badelf1: badelf1.ld
#	ld -m elf_i386 -o $@ empty.o -n -T$<

$(BADELFS_LD): %: %.ld empty.o
	ld -m elf_i386 -o $@ empty.o -n -T$<

$(BADELFS_OUTS): %.out: % main64
	./main64 $< >$@ 2>&1

test: $(BADELFS_OUTS)
