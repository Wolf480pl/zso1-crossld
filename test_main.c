#include <sys/mman.h>
#include <stdio.h>
#include "crossld.h"

extern void test32();

extern int crossld_start_fun(void *start, const struct function *funcs, int nfuncs);

static const size_t stack_size = 4096;

int myfun(int x) {
    printf("i have %d\n", x);
    return 42;
}

static const enum type myfun_args[] = {
    TYPE_INT,
};

static const struct function myfun_s = {
    .name = "myfun",
    .args = myfun_args,
    .nargs = 1,
    .result = TYPE_INT,
    .code = myfun
};

int main() {
    puts("hi");

    crossld_start_fun(test32, &myfun_s, 1);
    return 0;
}
