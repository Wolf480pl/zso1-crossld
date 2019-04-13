asm (
	".global _start\n"
	"_start:\n"
	"call entry\n"
	"hlt\n"
);

#include "fakelib.h"


void entry() {
	get_ptr64();
	exit(0);
}
