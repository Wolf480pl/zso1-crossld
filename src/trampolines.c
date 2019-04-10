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

struct ret_hunk {
    unsigned char insn[8];
};

struct crossld_ctx {
    void *old_stack;
    char *common_hunks;
    void *code_start;
};

extern char crossld_hunks;
extern char crossld_call64_trampoline_start;
extern char crossld_call64_trampoline_mid;
extern size_t crossld_jump32_offset;
extern size_t crossld_call64_dst_addr_mid_offset;
extern size_t crossld_call64_retconv_mid_offset;
extern size_t crossld_call64_panic_addr_mid_offset;
extern size_t crossld_call64_out_addr_mid_offset;
extern size_t crossld_hunks_len;
extern size_t crossld_call64_trampoline_len1;
extern size_t crossld_call64_trampoline_len2;
extern size_t crossld_call64_out_offset;
extern size_t crossld_exit_offset;
extern size_t crossld_exit_ctx_addr_offset;
extern struct arg_hunk crossld_hunk_array[6][3];
extern struct ret_hunk crossld_check_u32;
extern struct ret_hunk crossld_check_s32;
extern struct ret_hunk crossld_pass_u64;

typedef int (*crossld_jump32_t)(void *stack, void *func, struct crossld_ctx *ctx);

static const size_t stack_size = 4096 * 1024; // 4 MiB
static const size_t code_size = 4096;

enum arg_mode {
    ARG_PASS32 = 0,
    ARG_PASS64 = 1,
    ARG_SIGN32 = 2,
};

enum ret_mode {
    RET_PASS32,
    RET_PASS64,
    RET_CHECK_U32,
    RET_CHECK_S32,
};

static enum arg_mode arg_to_mode(enum type type) {
    switch (type) {
        case TYPE_UNSIGNED_LONG_LONG:
        case TYPE_LONG_LONG:
            return ARG_PASS64;
        case TYPE_LONG:
            return ARG_SIGN32;
        default:
            return ARG_PASS32;
    }
}

static enum ret_mode ret_to_mode(enum type type) {
    switch (type) {
        case TYPE_UNSIGNED_LONG:
        case TYPE_PTR:
            return RET_CHECK_U32;
        case TYPE_LONG:
            return RET_CHECK_S32;
        case TYPE_UNSIGNED_LONG_LONG:
        case TYPE_LONG_LONG:
            return RET_PASS64;
        case TYPE_UNSIGNED_INT:
        case TYPE_INT:
        case TYPE_VOID:
            return RET_PASS32;
    }
}

static void* trampoline_cat(char **code_p, const void *src, size_t len) {
    void* start = *code_p;
    printf("copying %zd bytes\n", len);
    memcpy(*code_p, src, len);
    *code_p += len;
    return start;
}

static void patch(char *desc, void** const field, void *value) {
    printf("putting %zx as %s at %zx\n", value, desc, field);
    *field = value;
}

_Noreturn void crossld_panic(size_t retval);

static void* write_trampoline(char **code_p, char *common_hunks,
                              const struct function *func) {
    char* const code = *code_p;

    trampoline_cat(code_p, &crossld_call64_trampoline_start,
                            crossld_call64_trampoline_len1);

    printf("starting injection at %zx\n", *code_p);
    struct arg_hunk* argconv = (struct arg_hunk*) *code_p;

    size_t depth = 8;
    for (size_t i = 0; i < func->nargs; ++i) {
        enum arg_mode mode = arg_to_mode(func->args[i]);
        if (i < 6) {
            *argconv = crossld_hunk_array[i][mode];
            argconv->depth = depth;
            ++argconv;
        } else {
            //TODO
        }
        depth += (mode == ARG_PASS64) ? 8 : 4;
    }
    *code_p = (char*) argconv;
    printf("finished injection at %zx\n", *code_p);

    void* mid = *code_p;

    trampoline_cat(code_p, &crossld_call64_trampoline_mid,
                            crossld_call64_trampoline_len2);
    printf("finished trampoline at %zx\n", *code_p);

    void* const funptr = func->code;

    void** const           dst_addr_field   = (void**)           (mid + crossld_call64_dst_addr_mid_offset);
    struct ret_hunk* const retconv_field    = (struct ret_hunk*) (mid + crossld_call64_retconv_mid_offset);
    void** const           panic_addr_field = (void**)           (mid + crossld_call64_panic_addr_mid_offset);
    void** const           out_addr_field   = (void**)           (mid + crossld_call64_out_addr_mid_offset);
    void** const           out_hunk         = (void**)           (common_hunks + crossld_call64_out_offset);

    //printf("putting %zx as dst address at %zx\n", funptr, dst_addr_field);
    //*dst_addr_field = funptr;
    patch("dst address", dst_addr_field, funptr);

    switch (ret_to_mode(func->result)) {
        case RET_PASS32:
            // leave the NOPs
            break;
        case RET_PASS64:
            *retconv_field = crossld_pass_u64;
            break;
        case RET_CHECK_S32:
            *retconv_field = crossld_check_s32;
            break;
        case RET_CHECK_U32:
            *retconv_field = crossld_check_u32;
            break;
    }

    //printf("putting %zx as panic address at %zx\n", crossld_panic, panic_addr_field);
    //*panic_addr_field = crossld_panic;
    patch("panic address", panic_addr_field, crossld_panic);

    //printf("putting %zx as out address at %zx\n", out_hunk, out_addr_field);
    //*out_addr_field = out_hunk;
    patch("out address", out_addr_field, out_hunk);

    //write(2, code, *code_p - code);

    return code;
}

static enum type exit_args[] = {TYPE_INT};

struct function crossld_exit_fun = {
    .name = "exit",
    .args = exit_args,
    .nargs = 1,
    .result = TYPE_VOID,
    .code = NULL,
//    .code = crossld_exit,
};

struct crossld_ctx* crossld_generate_trampolines(void **res_trampolines,
                                  const struct function *funcs, int nfuncs,
                                  struct function *exit_func) {

    struct crossld_ctx* ctx = malloc(sizeof(struct crossld_ctx));
    char *code = mmap(NULL, code_size, PROT_READ|PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE | MAP_32BIT, -1, 0);
    if (code == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }
    ctx->code_start = code;

    ctx->common_hunks = trampoline_cat(&code, &crossld_hunks, crossld_hunks_len);

    printf("jump32 offset: %zd dst offset: %zd out offset: %zd\n",
            crossld_jump32_offset, crossld_call64_dst_addr_mid_offset,
            crossld_call64_out_addr_mid_offset);

    void** const crossld_exit_ctx_addr_field = (void**) (ctx->common_hunks + crossld_exit_ctx_addr_offset);

    patch("ctx address", crossld_exit_ctx_addr_field, ctx);

    if (exit_func) {
        *exit_func = crossld_exit_fun;
        exit_func->code = ctx->common_hunks + crossld_exit_offset;
        printf("exit func: %zx code %zx\n", exit_func, exit_func->code);
    }

    for (size_t i = 0; i < nfuncs; ++i) {
        res_trampolines[i] = write_trampoline(&code, ctx->common_hunks, &funcs[i]);
    }

    if (mprotect(ctx->code_start, code_size, PROT_READ|PROT_EXEC) < 0) {
        perror("mprotect");
        return NULL;
    }

    return ctx;
}

void crossld_free_trampolines(struct crossld_ctx *ctx) {
    void *code_start = ctx->code_start;
    if (munmap(code_start, code_size) < 0) {
        perror("munmap");
    }
}

int crossld_enter(void *start, struct crossld_ctx *ctx) {
    char *stack = mmap(NULL, stack_size, PROT_READ|PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE | MAP_32BIT, -1, 0);

    crossld_jump32_t jump32 =
            (crossld_jump32_t) (ctx->common_hunks + crossld_jump32_offset);

    if (stack == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    stack += stack_size - 4;

    puts("jumping"); fflush(stdout);
    return jump32(stack, start, ctx);
}

_Noreturn void crossld_panic(size_t retval) {
    fprintf(stderr, "PANIC: return value outside of range: %zx\n", retval);
#if 1
    abort();
#else
    crossld_exit(-1);
#endif
}

#if 0

_Noreturn void crossld_exit(int status) {
    exit(status);
}
#else
#endif

