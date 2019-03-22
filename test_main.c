#include <sys/mman.h>
#include <stdio.h>
#include "crossld.h"

extern void test32();

extern int crossld_start_fun(void *start, const struct function *funcs, int nfuncs);

static const size_t stack_size = 4096;

int main() {
    puts("hi");

    crossld_start_fun(test32, NULL, 0);
    return 0;
}
