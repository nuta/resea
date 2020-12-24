#ifndef __RESEA_CMDLINE_H__
#define __RESEA_CMDLINE_H__

#include <list.h>
#include <types.h>

/// A command (e.g. "add" in "git add new_file.c"). It's equivalent to
/// sub-command in Python's argparse.
struct cmdline_cmd {
    /// The next/previous other commands.
    list_elem_t next;
    /// The command name. NULL for global arguments/options.
    const char *name;
    /// The list of arguments.
    list_t args;
};

/// An argument (e.g. "new_file.c" in "git add new_file.c").
struct cmdline_arg {
    /// The next/previous argument in the same command.
    list_elem_t next;
    /// The command associated to this argument. It's always non-NULL.
    struct cmdline_cmd *cmd;
    /// The argument name.
    const char *name;
    /// The pointer to store the value.
    void **result;
    /// Whether it's optional. If it's true and the argument is not given,
    /// `ptr` is uninitialized.
    bool optional;
};

void cmdline_cmd(const char *name);
void cmdline_arg(char **result, const char *cmd, const char *name,
                 bool optional);
void cmdline_parse(const char *cmdline, char **cmd_name);
void cmdline_init(void);

#endif
