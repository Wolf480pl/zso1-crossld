INCLUDEDIR := ../../include
LIBCROSSLD := ../../src/libcrossld.so

all: hello-32 hello-64
clean:
	rm -f hello-32 hello-64 *.so *.o

.PHONY: all clean test

fakelib.so: fakelib.c
	gcc -m32 -shared -fPIC -o fakelib.so fakelib.c

hello-32.o: hello-32.c
	gcc -m32 -O3 -c hello-32.c

hello-32: hello-32.o fakelib.so
	ld -m elf_i386 -o hello-32 hello-32.o fakelib.so

hello-64: hello-64.c $(LIBCROSSLD)
	gcc -O3 -I $(INCLUDEDIR) hello-64.c -o hello-64 $(LIBCROSSLD)

test: hello-64 hello-32
	./$<
