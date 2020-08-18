#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/syscall.h>
#include <string.h>
#include "commands.h"

static task_t vm_server = 1;

static void help_command(__unused int argc, __unused char **argv) {
    INFO("help               -  Print this message.");
    INFO("launch <task_name> -  Launch the task.");
    INFO("ps                 -  Print this message.");
    INFO("q                  -  Halt the computer.");
}

static void launch_command(int argc, char **argv) {
    if (argc < 2) {
        INFO("Usage: launch <task_name>");
        return;
    }

    // Construct a cmdline from argv.
    size_t len = 0;
    for (int i = 1; i < argc; i++) {
        len += strlen(argv[i]) + 1 /* whitespace or '\0' */;
    }

    char *name_and_cmdline = malloc(len);
    size_t offset = 0;
    for (int i = 1; i < argc; i++) {
        strncpy(&name_and_cmdline[offset], argv[i], len - offset);
        offset += strlen(argv[i]);
        name_and_cmdline[offset++] = ' ';
    }
    name_and_cmdline[offset - 1] = '\0';

    struct message m;
    m.type = LAUNCH_TASK_MSG;
    m.launch_task.name_and_cmdline = name_and_cmdline;
    error_t err = ipc_call(vm_server, &m);
    free(name_and_cmdline);
    if (err != OK) {
        WARN("failed to launch '%s': %s", argv[1], err2str(err));
    }
}

static void ps_command(__unused int argc, __unused char **argv) {
    kdebug("ps");
}

static void quit_command(__unused int argc, __unused char **argv) {
    kdebug("q");
}

struct command commands[] = {
    { .name = "help", .run = help_command },
    { .name = "launch", .run = launch_command },
    { .name = "ps", .run = ps_command },
    { .name = "q", .run = quit_command },
    { .name = NULL, .run = NULL },
};
