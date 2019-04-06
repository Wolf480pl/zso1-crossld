#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "crossld.h"

static const struct function funs[] = {};

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: $s <elf_file>\n", argv[0]);
        return 2;
    }

    setlinebuf(stdout);

    int ret = crossld_start(argv[1], funs, 0);
    if (ret < 0) {
        char cmd[1024];
        int pid = getpid();
        snprintf(cmd, sizeof(cmd), "cat /proc/%d/maps", pid);
        system(cmd);
    }
    return 0;
}
