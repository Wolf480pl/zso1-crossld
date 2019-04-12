asm (
	".global _start\n"
	"_start:\n"
	"call entry\n"
	"hlt\n"
);

#include <stddef.h>
#include "fakelib.h"

void check(void *ptr) {
    size_t s = (size_t) ptr;
    if ((s & 0xf) != 8) {
        print("NOK");
        fake_exit(0x10 | (s & 0xf));
    }
}

void entry() {
	check(fun0(0, 0, 0, 0, 0, 0));
	check(fun4(0, 0, 0, 0, 0, 0, 1));
	check(fun8(0, 0, 0, 0, 0, 0, 1, 2));
	check(fun12(0, 0, 0, 0, 0, 0, 1, 2, 3));
	check(fun16(0, 0, 0, 0, 0, 0, 1, 2, 3, 4));
	print("OK");
	fake_exit(0);
}
