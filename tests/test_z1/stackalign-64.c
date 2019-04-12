#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "common.h"
#include "fakelib.h"

asm (
    "fun0:\n"
    "fun4:\n"
    "fun8:\n"
    "fun12:\n"
    "fun16:\n"
    "    mov %rsp, %rax\n"
    "    ret\n"
);

int main() {
    enum type argtypes[] = {TYPE_INT, TYPE_INT, TYPE_INT,
                            TYPE_INT, TYPE_INT, TYPE_INT,
                            TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT};
	struct function funcs[] = {
		{"fun0", argtypes, 6, TYPE_PTR, fun0},
		{"fun4", argtypes, 7, TYPE_PTR, fun4},
		{"fun8", argtypes, 8, TYPE_PTR, fun8},
		{"fun12", argtypes, 9, TYPE_PTR, fun12},
		{"fun16", argtypes, 10, TYPE_PTR, fun16},
		print_function,
		fake_exit_function,
	};
	
	int v;
	return crossld_start("stackalign-32", funcs, 7);
}
