#include <list.h>
#include <resea/ipc.h>
#include <resea/async.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/task.h>
#include <string.h>
#include "elf.h"
#include "bootfs.h"
#include "pages.h"
#include "task.h"
#include "mm.h"
#include "ool.h"

// for sparse
error_t call_pager(struct message *m);

error_t call_pager(struct message *m) {
    m->src = INIT_TASK;
    error_t err;
    switch (m->type) {
        case OOL_RECV_MSG:
            err = handle_ool_recv(m);
            break;
        case OOL_VERIFY_MSG:
            err = handle_ool_verify(m);
            break;
        case OOL_SEND_MSG:
            err = handle_ool_send(m);
            break;
        default:
            UNREACHABLE();
    }

    if (err != OK) {
        PANIC("call_pager failed (%s)", err2str(err));
    }

    return err;
}

static void spawn_servers(void) {
    // Launch servers in bootfs.
    int num_launched = 0;
    struct bootfs_file *file;
    for (int i =0; (file = bootfs_open(i)) != NULL; i++) {
        // Autostart server names (separated by whitespace).
        char *startups = AUTOSTARTS;

        // Execute the file if it is listed in the autostarts.
        while (*startups != '\0') {
            size_t len = strlen(file->name);
            if (!strncmp(file->name, startups, len)
                && (startups[len] == '\0' || startups[len] == ' ')) {
                task_spawn(file, "");
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
    TRACE("starting...");
    bootfs_init();
    pages_init();
    task_init();
    spawn_servers();

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        struct task *caller = NULL;
        if (m.src != KERNEL_TASK) {
            caller = task_lookup(m.src);
            ASSERT(caller);
        }

        struct message r;
        bzero(&r, sizeof(r));

        switch (m.type) {
            case ASYNC_MSG:
                async_reply(m.src);
                break;
            case OOL_RECV_MSG: {
                task_t src = m.src;
                error_t err = handle_ool_recv(&m);
                switch (err) {
                    case DONT_REPLY:
                        break;
                    case OK:
                        ipc_reply(src, &m);
                        break;
                    default:
                        ipc_reply_err(src, err);
                }
                break;
            }
            case OOL_VERIFY_MSG: {
                task_t src = m.src;
                error_t err = handle_ool_verify(&m);
                switch (err) {
                    case DONT_REPLY:
                        break;
                    case OK:
                        ipc_reply(src, &m);
                        break;
                    default:
                        ipc_reply_err(src, err);
                }
                break;
            }
            case OOL_SEND_MSG: {
                task_t src = m.src;
                error_t err = handle_ool_send(&m);
                switch (err) {
                    case DONT_REPLY:
                        break;
                    case OK:
                        ipc_reply(src, &m);
                        break;
                    default:
                        ipc_reply_err(src, err);
                }
                break;
            }
            case NOP_MSG:
                r.type = NOP_REPLY_MSG;
                r.nop_reply.value = m.nop.value * 7;
                ipc_reply(m.src, &r);
                break;
            case NOP_WITH_OOL_MSG:
                free((void *) m.nop_with_ool.data);
                r.type = NOP_WITH_OOL_REPLY_MSG;
                r.nop_with_ool_reply.data = "reply!";
                r.nop_with_ool_reply.data_len = 7;
                ipc_reply(m.src, &r);
                break;
            case EXCEPTION_MSG: {
                if (m.src != KERNEL_TASK) {
                    WARN("forged exception message from #%d, ignoring...",
                         m.src);
                    break;
                }

                struct task *task = task_lookup(m.exception.task);
                ASSERT(task);
                ASSERT(m.exception.task == task->tid);

                if (m.exception.exception == EXP_GRACE_EXIT) {
                    INFO("%s: terminated its execution", task->name);
                } else {
                    WARN("%s: exception occurred, killing the task...",
                         task->name);
                }

                task_kill(task);
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
                ASSERT(m.page_fault.task == task->tid);

                paddr_t paddr =
                    pager(task, m.page_fault.vaddr, m.page_fault.fault);
                if (!paddr) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                vaddr_t aligned_vaddr = ALIGN_DOWN(m.page_fault.vaddr, PAGE_SIZE);
                ASSERT_OK(map_page(task->tid, aligned_vaddr, paddr, MAP_W, false));
                r.type = PAGE_FAULT_REPLY_MSG;

                ipc_reply(task->tid, &r);
                break;
            }
            case SERVE_MSG: {
                service_register(caller, m.serve.name);
                free((void *) m.serve.name);
                r.type = SERVE_REPLY_MSG;
                ipc_reply(m.src, &r);
                break;
            }
            case LOOKUP_MSG: {
                task_t server = service_wait(caller, m.lookup.name);
                free((void *) m.lookup.name);
                if (IS_OK(server)) {
                    r.type = LOOKUP_REPLY_MSG;
                    r.lookup_reply.task = server;
                    ipc_reply(m.src, &r);
                }
                break;
            }
            case ALLOC_PAGES_MSG: {
                struct task *task = task_lookup(m.src);
                ASSERT(task);

                vaddr_t vaddr;
                paddr_t paddr = m.alloc_pages.paddr;
                error_t err =
                    alloc_phy_pages(task, &vaddr, &paddr, m.alloc_pages.num_pages);
                if (err != OK) {
                    ipc_reply_err(m.src, err);
                    break;
                }

                r.type = ALLOC_PAGES_REPLY_MSG;
                r.alloc_pages_reply.vaddr = vaddr;
                r.alloc_pages_reply.paddr = paddr;
                ipc_reply(m.src, &r);
                break;
            }
            case LAUNCH_TASK_MSG: {
                task_t task_or_err = task_spawn_by_cmdline(m.launch_task.name_and_cmdline);
                if (IS_ERROR(task_or_err)) {
                    ipc_reply_err(m.src, task_or_err);
                    break;
                }

                r.type = LAUNCH_TASK_REPLY_MSG;
                r.launch_task_reply.task = task_or_err;
                ipc_reply(m.src, &r);
                break;
            }
            case WATCH_TASK_MSG: {
                struct task *task = task_lookup(m.watch_task.task);
                if (!task) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                task_watch(caller, task);
                m.type = WATCH_TASK_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            case UNWATCH_TASK_MSG: {
                struct task *task = task_lookup(m.unwatch_task.task);
                if (!task) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                task_unwatch(caller, task);
                m.type = UNWATCH_TASK_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                WARN("unknown message type (type=%d)", m.type);
                // FIXME: Free ool payloads.
        }
    }
}
