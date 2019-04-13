#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "common.h"
#include "multiargs.h"

static long long validate(int i1, long l1, long long ll1, unsigned int ui1,
		unsigned long ul1, unsigned long long ull1, int i2, long l2,
		long long ll2, unsigned int ui2, unsigned long ul2,
		unsigned long long ull2) {
	if (i1 != I1 || l1 != L1 || ll1 != LL1 || ui1 != U1 || ul1 != UL1 || ull1 != ULL1 || i2 != I2 || l2 != L2 || ll2 != LL2 || ui2 != U2 || ul2 != UL2 || ull2 != ULL2)
		return -1;
	return LL_RET;
}

int main() {
	enum type validate_types[] = {TYPE_INT, TYPE_LONG, TYPE_LONG_LONG,
		TYPE_UNSIGNED_INT, TYPE_UNSIGNED_LONG, TYPE_UNSIGNED_LONG_LONG,
		TYPE_INT, TYPE_LONG, TYPE_LONG_LONG, TYPE_UNSIGNED_INT,
		TYPE_UNSIGNED_LONG, TYPE_UNSIGNED_LONG_LONG};

    struct function fun = {"f0", validate_types, 12, TYPE_LONG_LONG, validate};
	struct function funcs[128];
	char names[128][4];

    funcs[0] = print_function;
    funcs[1] = fake_exit_function;
	for (size_t i = 2; i < 128; ++i) {
	    snprintf(names[i], sizeof(names[i]), "f%d", i);
	    funcs[i] = fun;
	    funcs[i].name = names[i];
	}

	return crossld_start("multifuns-32", funcs, 128);
}
