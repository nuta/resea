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
    for (int i = 0; (file = bootfs_open_iter(i)) != NULL; i++) {
        // Autostart server names (separated by whitespace).
        char *startups = STARTUP_SERVERS;

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
    bootfs_init();
    spawn_servers();

    TRACE("ready");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case ADD_MSG:
                m.type = ADD_REPLY_MSG;
                m.add_reply.value = m.add.x + m.add.y;
                ipc_reply(m.src, &m);
                break;
            case SPAWN_TASK_MSG: {
                // TODO: m.spawn_task.name is not null-terminated.
                struct bootfs_file *file = bootfs_open(m.spawn_task.name);
                if (!file) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                task_t task_or_err = task_spawn(file, "");
                if (IS_ERROR(task_or_err)) {
                    ipc_reply_err(m.src, task_or_err);
                    break;
                }

                m.type = SPAWN_TASK_REPLY_MSG;
                m.spawn_task_reply.task = task_or_err;
                ipc_reply(m.src, &m);
                break;
            }
            case EXCEPTION_MSG: {
                struct task *task = task_lookup(m.exception.task);
                if (!task) {
                    WARN("unknown task %d", m.exception.task);
                    break;
                }

                switch (m.exception.reason) {
                    case EXCEPTION_GRACE_EXIT:
                        task_destroy2(task);
                        TRACE("%s exited gracefully", task->name);
                        break;
                    default:
                        WARN("unknown exception type %d", m.exception.reason);
                        break;
                }
                break;
            }
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

                m.type = PAGE_FAULT_REPLY_MSG;
                ipc_reply(task->tid, &m);
                break;
            }
        }
    }
}
