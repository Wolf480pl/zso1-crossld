#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "crossld.h"

static unsigned long get_long64() {
	return (unsigned long) (1ULL<<62);
}

static unsigned long get_long32() {
	return (unsigned long) 0xFFFFFFFF;
}

static unsigned long get_long_negative() {
	return (unsigned long) -1;
}

static void test(struct function *func, bool shouldSucceed, char *desc) {
	int v;
	v = crossld_start("long-ptr-32", func, 1);
	if (v != -1 && v != 0) {
		printf("Invalid response: %d\n", v);
		exit(-1);
	}
    if ((v == 0) == shouldSucceed) {
		printf("step OK\n");
		return;
	} else {
		printf("Invalid response: %d\n", v);
		exit(-1);
	}
}

int main() {
	struct function funcs[] = {
		{"get_ptr64", 0, 0, TYPE_LONG, get_long64},
		{"get_ptr64", 0, 0, TYPE_LONG, get_long32},
		{"get_ptr64", 0, 0, TYPE_LONG, get_long_negative},
	};
	
	test(&funcs[0], false, "get_long64");
	test(&funcs[1], false, "get_long32");
	test(&funcs[2], true, "get_long_negative");
	printf("OK\n");
}
