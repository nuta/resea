#pragma once
#include <types.h>

#define ARGC_MAX 32

struct cmd {
    char *argv[ARGC_MAX];
    int argc;
};
