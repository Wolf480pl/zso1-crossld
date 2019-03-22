all: test test_so

.PHONY: all clean

%.o: %.c
	gcc -fPIC -c $<

wrapper.o: wrapper.asm
	nasm -f elf64 -o $@ $<

test.o: test.asm
	nasm -f elf64 -o $@ $<

crossld_dummy.so: wrapper.o crossld_dummy.o
	gcc -shared -fPIC -o $@ $^

test: test.o test_main.o wrapper.o crossld_dummy.o
	gcc -no-pie -o $@ $^

test_so: test.o test_main.o crossld_dummy.so
	LD_RUN_PATH=`pwd` gcc -no-pie -o $@ $^

clean:
	rm -f *.o test
