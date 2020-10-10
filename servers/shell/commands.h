#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <types.h>
#include <string.h>

struct command {
    const char *name;
    void (*run)(int argc, char **argv);
};

extern struct command commands[];

static inline error_t kdebug(const char *cmd) {
    return sys_kdebug(cmd, strlen(cmd), "", 0);
}

static inline error_t kdebug_with_buf(const char *cmd, char *buf, size_t len) {
    return sys_kdebug(cmd, strlen(cmd), buf, len);
}

#endif
