#include "bootfs.h"
#include <elf.h>
#include <print_macros.h>
#include <resea/malloc.h>
#include <string.h>

extern char bootfs[];

// FIXME:
#define PAGE_SIZE 4096

struct task {
    task_t tid;
    void *file_header;
};

/// Execute a ELF file. Returns an task ID on success or an error on failure.
task_t task_spawn(struct bootfs_file *file, const char *cmdline) {
    TRACE("launching %s...", file->name);
    struct task *task = malloc(sizeof(*task));
    if (!task) {
        PANIC("too many tasks");
    }

    // FIXME: void *file_header = malloc(PAGE_SIZE);
    void *file_header = malloc(PAGE_SIZE);

    read_file(file, 0, file_header, PAGE_SIZE);

    // Ensure that it's an ELF file.
    elf_ehdr_t *ehdr = (elf_ehdr_t *) file_header;
    if (memcmp(ehdr->e_ident,
               "\x7f"
               "ELF",
               4)
        != 0) {
        WARN("%s: invalid ELF magic, ignoring...", file->name);
        return ERR_INVALID_ARG;
    }

    // Create a new task for the server.
    task_t tid_or_err = task_create(file->name, ehdr->e_entry, task_self(), 0);
    if (IS_ERROR(tid_or_err)) {
        return tid_or_err;
    }

    task->file_header = file_header;
    task->tid = tid_or_err;

    return task->tid;
}

static void spawn_servers(void) {
    // Launch servers in bootfs.
    int num_launched = 0;
    struct bootfs_file *file;
    for (int i = 0; (file = bootfs_open(i)) != NULL; i++) {
        // Autostart server names (separated by whitespace).
        char *startups = "shell";  // FIXME:

        // Execute the file if it is listed in the autostarts.
        while (*startups != '\0') {
            size_t len = strlen(file->name);
            if (!strncmp(file->name, startups, len)
                && (startups[len] == '\0' || startups[len] == ' ')) {
                ASSERT_OK(task_spawn(file, ""));
                num_launched++;
                break;
            }

            while (*startups != '\0' && *startups != ' ') {
                startups++;
            }

            if (*startups == ' ') {
                startups++;
            }
        }
    }

    if (!num_launched) {
        WARN("no servers to launch");
    }
}

void main(void) {
    INFO("Hello World from init!");
    bootfs_init();

    // while (true) {
    //     struct message m;
    //     error_t err = ipc_recv_any(&m);
    //     ASSERT_OK(err);

    //     switch (m.type) {
    //         case PING_MSG:
    //             INFO("Got ping from %d", m.src);
    //             m.type = PONG_MSG;
    //             err = ipc_reply(m.src, &m);
    //             break;
    //         case EXIT_MSG:
    //             INFO("Got exit from %d", m.src);
    //             kill(m.src);
    //             break;
    //     }
    // }
}
