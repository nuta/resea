#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/syscall.h>
#include <resea/cmdline.h>
#include <string.h>
#include "commands.h"
#include "http.h"
#include "fs.h"

static void fs_read_command(int argc, char **argv) {
    if (argc < 2) {
        WARN("fs_read: too few arguments");
        return;
    }

    fs_read(argv[1]);
}

static void fs_write_command(int argc, char **argv) {
    if (argc < 2) {
        WARN("fs_write: too few arguments");
        return;
    }

    fs_write(argv[1], (uint8_t *) argv[2], strlen(argv[2]));
}

static void http_get_command(int argc, char **argv) {
    if (argc < 2) {
        WARN("http_get: too few arguments");
        return;
    }

    http_get(argv[1]);
}

static void ps_command(__unused int argc, __unused char **argv) {
    kdebug("ps");
}

static void quit_command(__unused int argc, __unused char **argv) {
    kdebug("q");
}

static void help_command(__unused int argc, __unused char **argv) {
    INFO("help              -  Print this message.");
    INFO("<task> cmdline... -  Launch a task.");
    INFO("ps                -  List tasks.");
    INFO("q                 -  Halt the computer.");
    INFO("fs-read path      -  Read a file.");
    INFO("fs-write path str -  Write a string into a file.");
    INFO("http-get url      -  Peform a HTTP GET request.");
}

struct command commands[] = {
    { .name = "help", .run = help_command },
    { .name = "ps", .run = ps_command },
    { .name = "q", .run = quit_command },
    { .name = "fs-read", .run = fs_read_command },
    { .name = "fs-write", .run = fs_write_command },
    { .name = "http-get", .run = http_get_command },
    { .name = NULL, .run = NULL },
};
