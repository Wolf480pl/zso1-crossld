CFLAGS := -O3 -g

all: test test_so

.PHONY: all clean

%.o: %.c
	gcc $(CFLAGS) -fPIC -c $<

wrapper.o: wrapper.asm
	nasm -f elf64 -o $@ $<

test.o: test.asm
	nasm -f elf64 -o $@ $<

crossld.so: wrapper.o crossld.o
	gcc -shared -fPIC -o $@ $^

test: test.o test_main.o wrapper.o crossld.o
	gcc $(CFLAGS) -no-pie -o $@ $^

test_so: test.o test_main.o crossld.so
	LD_RUN_PATH=`pwd` gcc -no-pie -o $@ $^

clean:
	rm -f *.o *.so test test_so
