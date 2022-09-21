#include "parse.h"

static char *skip_whitespaces(char *p) {
    for (;;) {
        switch (*p) {
            case ' ':
            case '\t':
            case '\n':
                p++;
                continue;
            default:
                return p;
        }
    }
}

static char *consume_argv(char *p) {
    for (;;) {
        switch (*p) {
            case '\0':
                return NULL;
            case ' ':
            case '\t':
            case '\n':
                *p = '\0';
                return p;
            default:
                p++;
        }
    }
}

error_t parse(char *input, struct cmd *cmd) {
    char *p = input;
    cmd->argc = 0;
    do {
        p = skip_whitespaces(p);
        cmd->argv[cmd->argc] = p;
        cmd->argc++;
        p = consume_argv(p);
    } while (p != NULL);

    return OK;
}
