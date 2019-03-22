all: test

.PHONY: all clean

%.o: %.c
	gcc -c $<

wrapper.o: wrapper.asm
	nasm -f elf64 -o $@ $<

test.o: test.asm
	nasm -f elf64 -o $@ $<

test: test.o test_main.o wrapper.o crossld_dummy.o
	gcc -no-pie -o $@ $^

clean:
	rm -f *.o test
