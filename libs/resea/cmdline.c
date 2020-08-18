#include <resea/malloc.h>
#include <resea/cmdline.h>
#include <string.h>

static list_t commands;

/// Registers a command.
void cmdline_cmd(const char *name) {
    struct cmdline_cmd *cmd = malloc(sizeof(*cmd));
    cmd->name = strdup(name);
    list_init(&cmd->args);
    list_push_back(&commands, &cmd->next);
}

/// Registers an argument.
void cmdline_arg(char **result, const char *cmd, const char *name, bool optional) {
    ASSERT(cmd != NULL);

    LIST_FOR_EACH (c, &commands, struct cmdline_cmd, next) {
        if ((!c && !c->name) || (c->name && !strcmp(c->name, (char *) cmd))) {
            struct cmdline_arg *arg = malloc(sizeof(*arg));
            arg->cmd = c;
            arg->name = strdup(name);
            arg->result = (void **) result;
            arg->optional = optional;
            list_push_back(&c->args, &arg->next);
            return;
        }
    }

    PANIC("cmdline_arg: unknown command '%s'", cmd);
}

/// Parses a command line. Panics if the given command line string is invalid.
void cmdline_parse(const char *cmdline, char **cmd_name) {
    char *s = strdup(cmdline);
    if (list_len(&commands) > 1) {
        // Parse the command + arguments.
        *cmd_name = s;
        char *sep = strchr(s, ' ');
        if (*sep == '\0') {
            s = sep;
        } else {
            *sep = '\0';
            s = sep + 1;
        }
    } else {
        // No commands exist. Parse only the global arguments.
        *cmd_name = NULL;
    }

    // Now `s` points to the beginning of command-local or global  arguments,
    // that is:
    //
    //      cmdline
    //        v
    //    git add new_file.c
    //            ^
    //            s
    //

    // Look for the command.
    struct cmdline_cmd *cmd = NULL;
    LIST_FOR_EACH (c, &commands, struct cmdline_cmd, next) {
        if ((!*cmd_name && !c->name) || (c->name && !strcmp(c->name, *cmd_name))) {
            cmd = c;
            break;
        }
    }

    if (!cmd) {
        // `cmd` for global options (the case `cmd_name == NULL` holds) should
        // always exist.
        ASSERT(*cmd_name != NULL);
        if (strlen(*cmd_name) == 0) {
            PANIC("the command is not given");
        } else {
            PANIC("no such a command: %s", *cmd_name);
        }
    }

    // Parse arugments for the specified command.
    while (*s != '\0') {
        struct cmdline_arg *arg =
            LIST_POP_FRONT(&cmd->args, struct cmdline_arg, next);
        if (!arg) {
            PANIC("too many arguments are given");
        }

        *arg->result = s;

        // Find the next argument.
        char *sep = strchr(s, ' ');
        if (*sep == '\0') {
            break;
        }

        *sep = '\0';
        s = sep + 1;
    }

    if (!list_is_empty(&cmd->args)) {
        PANIC("too few arguments");
    }
}

/// Internally used initialization function.
void cmdline_init(void) {
    list_init(&commands);

    struct cmdline_cmd *cmd = malloc(sizeof(*cmd));
    cmd->name = NULL;
    list_init(&cmd->args);
    list_push_back(&commands, &cmd->next);
}
