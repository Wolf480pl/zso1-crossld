#include <sys/mman.h>
#include <stdio.h>
#include "crossld.h"

extern void test32();

extern int crossld_start_fun(void *start, const struct function *funcs, int nfuncs);

static const size_t stack_size = 4096;

int myfun(int x) {
    printf("i have %d\n", x);
    fflush(stdout);
    return 42;
}

int myfun2(int x, int y) {
    printf("i have %d and %d\n", x, y);
    fflush(stdout);
    return 42;
}

void print(char *str) {
    puts(str);
    fflush(stdout);
}

static const enum type myfun_args[] = {
    TYPE_INT,
};

static const enum type myfun2_args[] = {
    TYPE_INT,
    TYPE_INT,
};

static const enum type print_args[] = {
    TYPE_PTR,
};

static const struct function funs[] = {
    {
        .name = "myfun",
        .args = myfun_args,
        .nargs = 1,
        .result = TYPE_INT,
        .code = myfun
    },
    {
        .name = "myfun2",
        .args = myfun2_args,
        .nargs = 2,
        .result = TYPE_INT,
        .code = myfun2
    },
    {
        .name = "print",
        .args = print_args,
        .nargs = 1,
        .result = TYPE_VOID,
        .code = print
    },
};

int main() {
    puts("hi");

    crossld_start("elf32", funs, 2);
    return 0;
}
