asm (
	".global _start\n"
	"_start:\n"
	"call entry\n"
	"hlt\n"
);

#include <stddef.h>
#include "fakelib.h"

void entry() {
	print("OK");
	fake_exit(0);
}
