#include "bootfs.h"
#include "page_fault.h"
#include "task.h"
#include <print_macros.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/task.h>
#include <string.h>

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
    spawn_servers();

    TRACE("mainloop...");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case PAGE_FAULT_MSG: {
                if (m.src != KERNEL_TASK) {
                    WARN("forged page fault message from #%d, ignoring...",
                         m.src);
                    break;
                }

                struct task *task = task_lookup(m.page_fault.task);
                ASSERT(task);
                ASSERT(task->pager == task_self());
                ASSERT(m.page_fault.task == task->tid);

                error_t err =
                    handle_page_fault(task, m.page_fault.uaddr, m.page_fault.ip,
                                      m.page_fault.fault);
                if (IS_ERROR(err)) {
                    ipc_reply_err(m.src, err);
                    break;
                }

                DBG("PF replying to %d", task->tid);
                m.type = PAGE_FAULT_REPLY_MSG;
                ipc_reply(task->tid, &m);
                break;
            }
        }
    }
}
