#include <stdio.h>

int crossld_call64(void *got1, size_t got_idx, void *args) {
    printf("%zx %zu %zx\n", got1, got_idx, args);
    return 42;
}
