#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "crossld.h"

extern char crossld_hunks;
extern char crossld_jump32_offset;
extern char crossld_call64_dst_mov_offset;
extern size_t crossld_hunks_len;
extern uint32_t crossld_call64_in_fake_ptr;

typedef void (*crossld_jump32_t)(void *stack, void *func);

static const size_t stack_size = 4096;
static const size_t code_size = 4096;

int crossld_start_fun(char *start, const struct function *funcs, int nfuncs) {
    void *stack = mmap(NULL, stack_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_32BIT, -1, 0);
    if (stack == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    stack += stack_size - 4;

    char *code = mmap(NULL, code_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_32BIT, -1, 0);
    if (code == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    printf("copying %zd bytes\n", crossld_hunks_len);

    memcpy(code, &crossld_hunks, crossld_hunks_len);

    const size_t jump32_offset = (size_t) &crossld_jump32_offset;
    const size_t call64_dst_offset = (size_t) &crossld_call64_dst_mov_offset;

    printf("jump32 offset: %zd dst offset: %zd\n", jump32_offset, call64_dst_offset);

    void* funptr = funcs[0].code;
    crossld_jump32_t jump32 = (crossld_jump32_t) (code + jump32_offset);
    void** call64_dst = (void**) (code + call64_dst_offset);

    printf("putting %zx as dst address at %zx\n", funptr, call64_dst);
    *call64_dst = funptr;

    if (mprotect(code, code_size, PROT_READ|PROT_EXEC) < 0) {
        perror("mprotect");
        return 1;
    }

    size_t hunk_ptr = (size_t) code;
#if 0
    uint32_t* addr_ptr = (uint32_t*) (start + 4);

    printf("patching at: %zx replacing %x with %x\n", addr_ptr, *addr_ptr, (uint32_t) hunk_ptr);
    *addr_ptr = (uint32_t) hunk_ptr;
#endif
    printf("putting: %x as hunk ptr\n", (uint32_t) hunk_ptr);
    crossld_call64_in_fake_ptr = (uint32_t) hunk_ptr;
    jump32(stack, start);
}

int crossld_start(const char *fname, const struct function *funcs, int nfuncs) {
    //TODO
    return 0;
}
