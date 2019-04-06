#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "crossld.h"
#include "trampolines.h"

struct arg_hunk {
    unsigned char insn[7];
    unsigned char depth;
};

extern char crossld_hunks;
extern char crossld_call64_trampoline_start;
extern char crossld_call64_trampoline_mid;
extern size_t crossld_jump32_offset;
extern size_t crossld_call64_dst_addr_mid_offset;
extern size_t crossld_call64_out_addr_mid_offset;
extern size_t crossld_hunks_len;
extern size_t crossld_call64_trampoline_len1;
extern size_t crossld_call64_trampoline_len2;
extern size_t crossld_call64_out_offset;
extern struct arg_hunk crossld_hunk_array[12];

typedef void (*crossld_jump32_t)(void *stack, void *func);

static const size_t stack_size = 4096 * 1024; // 4 MiB
static const size_t code_size = 4096;

static void* trampoline_cat(char **code_p, const void *src, size_t len) {
    void* start = *code_p;
    printf("copying %zd bytes\n", len);
    memcpy(*code_p, src, len);
    *code_p += len;
    return start;
}

static void* write_trampoline(char **code_p, char *common_hunks, const struct function *func) {
    char* const code = *code_p;

    trampoline_cat(code_p, &crossld_call64_trampoline_start, crossld_call64_trampoline_len1);

    printf("starting injection at %zx\n", *code_p);
    struct arg_hunk* argconv = (struct arg_hunk*) *code_p;

    size_t depth = 8;
    for (size_t i = 0; i < func->nargs; ++i) {
        int is64 = 0;
        if (i < 6) {
            *argconv = crossld_hunk_array[i * 2 + is64];
            argconv->depth = depth;
            ++argconv;
        } else {
            //TODO
        }
        depth += is64 ? 8 : 4;
    }
    *code_p = (char*) argconv;
    printf("finished injection at %zx\n", *code_p);

    void* mid = *code_p;

    trampoline_cat(code_p, &crossld_call64_trampoline_mid, crossld_call64_trampoline_len2);
    printf("finished trampoline at %zx\n", *code_p);

    void* const funptr = func->code;

    void** const dst_addr_field = (void**) (mid + crossld_call64_dst_addr_mid_offset);
    void** const out_addr_field = (void**) (mid + crossld_call64_out_addr_mid_offset);
    void** const out_hunk = (void**) (common_hunks + crossld_call64_out_offset);

    printf("putting %zx as dst address at %zx\n", funptr, dst_addr_field);
    *dst_addr_field = funptr;

    printf("putting %zx as out address at %zx\n", out_hunk, out_addr_field);
    *out_addr_field = out_hunk;

    //write(2, code, *code_p - code);

    return code;
}

void* crossld_generate_trampolines(void **res_trampolines,
                                  const struct function *funcs, int nfuncs) {

    char *code = mmap(NULL, code_size, PROT_READ|PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE | MAP_32BIT, -1, 0);
    if (code == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }
    void* const code_start = code;

    void* common_hunks = trampoline_cat(&code, &crossld_hunks, crossld_hunks_len);

    printf("jump32 offset: %zd dst offset: %zd out offset: %zd\n",
            crossld_jump32_offset, crossld_call64_dst_addr_mid_offset,
            crossld_call64_out_addr_mid_offset);

    for (size_t i = 0; i < nfuncs; ++i) {
        res_trampolines[i] = write_trampoline(&code, common_hunks, &funcs[i]);
    }

    if (mprotect(code_start, code_size, PROT_READ|PROT_EXEC) < 0) {
        perror("mprotect");
        return NULL;
    }

    return common_hunks;
}

void crossld_free_trampolines(void *common_hunks) {
    void *code_start = common_hunks; // they happen to be the s ame
    if (munmap(code_start, code_size) < 0) {
        perror("munmap");
    }
}

int crossld_enter(void *start, void *common_hunks) {
    void *stack = mmap(NULL, stack_size, PROT_READ|PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE | MAP_32BIT, -1, 0);

    crossld_jump32_t jump32 =
            (crossld_jump32_t) (common_hunks + crossld_jump32_offset);

    if (stack == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    stack += stack_size - 4;

    puts("jumping"); fflush(stdout);
    jump32(stack, start);
    // TODO exit
    return 0;
}

_Noreturn void crossld_exit(int status) {
    //TODO return from crossld_enter
    exit(status);
}

static enum type exit_args[] = {TYPE_INT};

struct function crossld_exit_fun = {
    .name = "exit",
    .args = exit_args,
    .nargs = 1,
    .result = TYPE_VOID,
    .code = crossld_exit,
};
