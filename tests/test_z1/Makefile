TESTS_COMMON := memsz multiargs multicall stacksize stackalign multifuns
TESTS_GENERIC := exit missing-sym long-ptr mprotect mprotect2 cleanup
TESTS := $(TESTS_COMMON) $(TESTS_GENERIC)
CROSSLD := ../../src/libcrossld.so
CFLAGS := -O3
CC := gcc

.PHONY: all test
all: $(TESTS:%=%-64) $(TESTS:%=%-32)

test: all $(TESTS:%=%.out)

clean:
	rm -f *-32 *-64 *.o *.out

fakelib.so: fakelib.c
	gcc -m32 -shared -fPIC -o $@ $<

$(TESTS_COMMON:%=%-64):%-64: %-64.o common.o $(CROSSLD)
	gcc  -o $@ common.o $< $(CROSSLD)

$(TESTS_GENERIC:%=%-64):%-64: %-64.o $(CROSSLD)
	gcc  -o $@ $< $(CROSSLD)

%-32.o: %-32.c
	gcc -m32 -O3 -fno-stack-protector -c $<


%-32: %-32.o fakelib.so
	ld -m elf_i386 -o $@ $< fakelib.so

%.out: %-64 %-32
	@echo -en "$*:   \t"
	@(./$*-64 >$*.out 2>&1 && echo OK) || echo FAIL
