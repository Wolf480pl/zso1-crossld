#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "crossld.h"

extern char crossld_hunks;
extern char crossld_call64_trampoline;
extern size_t crossld_jump32_offset;
extern size_t crossld_call64_dst_addr_offset;
extern size_t crossld_call64_out_addr_offset;
extern size_t crossld_hunks_len;
extern size_t crossld_call64_trampoline_len;
extern size_t crossld_call64_out_offset;
extern uint32_t crossld_call64_in_fake_ptr;

typedef void (*crossld_jump32_t)(void *stack, void *func);

static const size_t stack_size = 4096;
static const size_t code_size = 4096;

void* write_trampoline(char **code_p, char *common_hunks, const struct function *func) {
    char* const code = *code_p;

    printf("copying %zd bytes\n", crossld_call64_trampoline_len);
    memcpy(code, &crossld_call64_trampoline, crossld_call64_trampoline_len);

    *code_p += crossld_call64_trampoline_len;

    void* const funptr = func->code;

    void** const dst_addr_field = (void**) (code + crossld_call64_dst_addr_offset);
    void** const out_addr_field = (void**) (code + crossld_call64_out_addr_offset);
    void** const out_hunk = (void**) (common_hunks + crossld_call64_out_offset);

    printf("putting %zx as dst address at %zx\n", funptr, dst_addr_field);
    *dst_addr_field = funptr;

    printf("putting %zx as out address at %zx\n", out_hunk, out_addr_field);
    *out_addr_field = out_hunk;

    return code;
}

int crossld_start_fun(char *start, const struct function *funcs, int nfuncs) {
    void *stack = mmap(NULL, stack_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_32BIT, -1, 0);
    if (stack == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    stack += stack_size - 4;

    char *code = mmap(NULL, code_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_32BIT, -1, 0);
    void* const code_start = code;
    if (code == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    printf("copying %zd bytes\n", crossld_hunks_len);

    memcpy(code, &crossld_hunks, crossld_hunks_len);
    void* const common_hunks = code;
    code += crossld_hunks_len;

    printf("jump32 offset: %zd dst offset: %zd out offset: %zd\n", crossld_jump32_offset, crossld_call64_dst_addr_offset, crossld_call64_out_addr_offset);

    crossld_jump32_t jump32 = (crossld_jump32_t) (common_hunks + crossld_jump32_offset);

#if 0
    void* funptr = funcs[0].code;
    void** call64_dst = (void**) (code + crossld_call64_dst_addr_offset);
    void** call64_out = (void**) (code + crossld_call64_out_addr_offset);
    void** call64_out_fun = (void**) (code + crossld_call64_out_offset);

    printf("putting %zx as dst address at %zx\n", funptr, call64_dst);
    *call64_dst = funptr;
    printf("putting %zx as dst address at %zx\n", call64_out_fun, call64_out);
    *call64_out = call64_out_fun;
    void* trampoline = code;
#else
    void* trampoline = write_trampoline(&code, common_hunks, &funcs[0]);
#endif

    if (mprotect(code_start, code_size, PROT_READ|PROT_EXEC) < 0) {
        perror("mprotect");
        return 1;
    }

    size_t hunk_ptr = (size_t) trampoline;
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
