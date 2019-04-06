#ifndef TRAMPOLINES_H
#define TRAMPOLINES_H

#include "crossld.h"

void* crossld_generate_trampolines(void **res_trampolines,
                                   const struct function *funcs, int nfuncs);

void crossld_free_trampolines(void *common_hunks);

int crossld_enter(void *start, void *common_hunks);

struct function crossld_exit_fun;

#endif
