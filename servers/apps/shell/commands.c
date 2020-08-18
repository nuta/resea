#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/syscall.h>
#include <string.h>
#include "commands.h"

static void help_command(__unused int argc, __unused char **argv) {
    INFO("help              -  Print this message.");
    INFO("<task> cmdline... -  Launch a task.");
    INFO("ps                -  List tasks.");
    INFO("q                 -  Halt the computer.");
}

static void ps_command(__unused int argc, __unused char **argv) {
    kdebug("ps");
}

static void quit_command(__unused int argc, __unused char **argv) {
    kdebug("q");
}

struct command commands[] = {
    { .name = "help", .run = help_command },
    { .name = "ps", .run = ps_command },
    { .name = "q", .run = quit_command },
    { .name = NULL, .run = NULL },
};
