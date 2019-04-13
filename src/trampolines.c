#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "crossld.h"
#include "trampolines.h"
#include "wrapper.h"
#include "debug.h"

struct crossld_ctx {
    void *old_stack; // offset of this element is known to crossld_do_exit
    char *common_hunks;
    void *code_start;
    size_t code_size;
};

static const size_t stack_size = 4096 * 1024; // 4 MiB

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
    __builtin_unreachable();
}

static void* trampoline_cat(char **code_p, const void *src, size_t len) {
    void* start = *code_p;
    DBG("copying %zd bytes\n", len);
    memcpy(*code_p, src, len);
    *code_p += len;
    return start;
}

static void patch(char *desc, void** const field, void *value) {
    DBG("putting %p as %s at %p\n", value, desc, field);
    *field = value;
}

static size_t trampoline_max_size(const struct function *func) {
    size_t size = 0;
    size += crossld_call64_trampoline_len1;

    size_t regargs = func->nargs;
    size_t stackargs = 0;
    if (regargs > 6) {
        regargs = 6;
        stackargs = func->nargs - 6;
    }
    size += regargs * sizeof(struct arg_hunk);
    size += sizeof(struct arg_hunk); // maybe push rax for stack alignment
    size += stackargs * 2 * sizeof(struct arg_hunk); // 2 hunks per stack arg

    size += crossld_call64_trampoline_len2;
    return size;
}

_Noreturn void crossld_panic(size_t retval, struct crossld_ctx *ctx);

static void* write_trampoline(char **code_p, struct crossld_ctx *ctx,
                              const struct function *func) {
    char* const code = *code_p;

    trampoline_cat(code_p, &crossld_call64_trampoline_start,
                            crossld_call64_trampoline_len1);

    DBG("starting injection at %p\n", *code_p);
    struct arg_hunk* argconv = (struct arg_hunk*) *code_p;

    size_t depth = 8;
    for (int i = 0; i < func->nargs; ++i) {
        if (i >= 6) {
            // need to pass by stack
            break;
        }
        enum arg_mode mode = arg_to_mode(func->args[i]);
        *argconv = crossld_hunk_array[i][mode];
        argconv->depth = depth;
        ++argconv;
        depth += (mode == ARG_PASS64) ? 8 : 4;
    }

    if (func->nargs > 6) {
        // stack arguments

        size_t maxdepth = depth;
        for (int i = 6; i < func->nargs; ++i) {
            enum arg_mode mode = arg_to_mode(func->args[i]);
            maxdepth += (mode == ARG_PASS64) ? 8 : 4;
        }
        // trampoline_start gave us aligned stack
        // make sure the stack will still be aligned after args
        if (func->nargs % 2 == 1) {
            // push rax as padding, we don't care what value it contains
            *argconv = crossld_push_rax;
            ++argconv;
        }
        // we need to push them in reverse order
        depth = maxdepth;
        for (ssize_t i = func->nargs - 1; i >= 6; --i) {
            enum type argtype = func->args[i];
            enum arg_mode mode = arg_to_mode(argtype);

            depth -= (mode == ARG_PASS64) ? 8 : 4;

            // put it in rax with zero/sign extension if needed
            *argconv = crossld_hunk_array[6][mode];
            argconv->depth = depth;
            ++argconv;
            // push rax
            *argconv = crossld_push_rax;
            // crossld_push_rax doesn't have a depth, so don't patch it
            ++argconv;
        }
    }

    *code_p = (char*) argconv;
    DBG("finished injection at %p\n", *code_p);

    char* mid = *code_p;

    trampoline_cat(code_p, &crossld_call64_trampoline_mid,
                            crossld_call64_trampoline_len2);
    DBG("finished trampoline at %p\n", *code_p);

    void* const funptr = func->code;

    struct ret_hunk* const retconv_field = (struct ret_hunk*) (mid + crossld_call64_retconv_mid_offset);

    void** const dst_addr_field =       (void**) (mid + crossld_call64_dst_addr_mid_offset);
    void** const panic_addr_field =     (void**) (mid + crossld_call64_panic_addr_mid_offset);
    void** const panic_ctx_addr_field = (void**) (mid + crossld_call64_panic_ctx_addr_mid_offset);
    void** const out_addr_field =       (void**) (mid + crossld_call64_out_addr_mid_offset);

    void** const out_hunk = (void**) (ctx->common_hunks + crossld_call64_out_offset);

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

    patch("panic address", panic_addr_field, crossld_panic);

    patch("panic ctx", panic_ctx_addr_field, ctx);

    patch("out address", out_addr_field, out_hunk);

#ifdef CROSSLD_DUMP_FD
    write(CROSSLD_DUMP_FD, code, *code_p - code);
#endif

    return code;
}

#ifdef CROSSLD_NOEXIT
_Noreturn void crossld_exit(int status) {
    exit(status);
}
#endif

static enum type exit_args[] = {TYPE_INT};

struct function crossld_exit_fun = {
    .name = "exit",
    .args = exit_args,
    .nargs = 1,
    .result = TYPE_VOID,
#ifdef CROSSLD_NOEXIT
    .code = crossld_exit,
#else
    .code = NULL,
#endif
};

struct crossld_ctx* crossld_generate_trampolines(void **res_trampolines,
                                  const struct function *funcs, int nfuncs,
                                  struct function *exit_func) {

    struct crossld_ctx* ctx = malloc(sizeof(struct crossld_ctx));

    if (exit_func) {
        // we need to do this for size calculation, as it WILL alias funcs array
        *exit_func = crossld_exit_fun;
    }

    size_t code_size = crossld_hunks_len;
    for (int i = 0; i < nfuncs; ++i) {
        code_size += trampoline_max_size(&funcs[i]);
    }

    char *code = mmap(NULL, code_size, PROT_READ|PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE | MAP_32BIT, -1, 0);
    if (code == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }
    ctx->code_start = code;
    ctx->code_size = code_size;

    ctx->common_hunks = trampoline_cat(&code, &crossld_hunks, crossld_hunks_len);

    DBG("jump32 offset: %zd dst offset: %zd out offset: %zd\n",
            crossld_jump32_offset, crossld_call64_dst_addr_mid_offset,
            crossld_call64_out_addr_mid_offset);

    void** const crossld_exit_ctx_addr_field = (void**) (ctx->common_hunks + crossld_exit_ctx_addr_offset);

    patch("ctx address", crossld_exit_ctx_addr_field, ctx);

    if (exit_func) {
#ifndef CROSSLD_NOEXIT
        exit_func->code = ctx->common_hunks + crossld_exit_offset;
#endif
        DBG("exit func: %p code %p\n", exit_func, exit_func->code);
    }

    for (int i = 0; i < nfuncs; ++i) {
        res_trampolines[i] = write_trampoline(&code, ctx, &funcs[i]);
    }

    if (mprotect(ctx->code_start, code_size, PROT_READ|PROT_EXEC) < 0) {
        perror("mprotect");
        return NULL;
    }

    return ctx;
}

void crossld_free_trampolines(struct crossld_ctx *ctx) {
    if (munmap(ctx->code_start, ctx->code_size) < 0) {
        perror("munmap");
    }
    free(ctx);
}

int crossld_enter(void *start, struct crossld_ctx *ctx) {
    char* stack = mmap(NULL, stack_size, PROT_READ|PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE | MAP_32BIT, -1, 0);

    crossld_jump32_t jump32 =
            (crossld_jump32_t) (ctx->common_hunks + crossld_jump32_offset);

    if (stack == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    void* stack_top = stack + stack_size - 4;

    DBG("jumping\n");
    int res = jump32(stack_top, start, ctx);
    if (munmap(stack, stack_size) < 0) {
        perror("munmap");
    }
    return res;
}

_Noreturn void crossld_panic(size_t retval, struct crossld_ctx *ctx) {
    fprintf(stderr, "PANIC: return value outside of range: %zx\n", retval);
#ifdef CROSSLD_NOEXIT
    abort();
#else
    crossld_exit_t cexit =
            (crossld_exit_t) (ctx->common_hunks + crossld_exit_offset);
    cexit(-1);
#endif
}
