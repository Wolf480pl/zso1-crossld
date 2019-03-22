#include <sys/mman.h>
#include <stdio.h>

extern void test32();

extern _Noreturn void crossld_jump32(void *stack, void *func);

const size_t stack_size = 4096;

int main() {
    puts("hi");
    void *stack = mmap(NULL, stack_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_32BIT, -1, 0);
    if (stack == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    stack += stack_size - 4;
    crossld_jump32(stack, test32);
    return 0;
}
