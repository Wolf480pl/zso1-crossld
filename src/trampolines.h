#ifndef TRAMPOLINES_H
#define TRAMPOLINES_H

#include "crossld.h"

struct crossld_ctx* crossld_generate_trampolines(void **res_trampolines,
                                   const struct function *funcs, int nfuncs,
                                   struct function *exit_func);

void crossld_free_trampolines(struct crossld_ctx *common_hunks);

int crossld_enter(void *start, struct crossld_ctx *common_hunks);

struct crossld_ctx;

#endif
