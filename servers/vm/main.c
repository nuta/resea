#include "bootfs.h"
#include "ool.h"
#include "page_alloc.h"
#include "page_fault.h"
#include "shm.h"
#include "task.h"
#include <elf/elf.h>
#include <list.h>
#include <resea/async.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/task.h>
#include <resea/timer.h>
#include <string.h>

// for sparse
error_t ipc_call_pager(struct message *m);

error_t ipc_call_pager(struct message *m) {
    m->src = VM_TASK;
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
        PANIC("ipc_call_pager failed (%s)", err2str(err));
    }

    return err;
}

static void spawn_servers(void) {
    // Launch servers in bootfs.
    int num_launched = 0;
    struct bootfs_file *file;
    for (int i = 0; (file = bootfs_open(i)) != NULL; i++) {
        // Autostart server names (separated by whitespace).
        char *startups = AUTOSTARTS;

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
    TRACE("starting...");
    bootfs_init();
    page_alloc_init();
    task_init();
    page_fault_init();
    spawn_servers();

    timer_set(5000);

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
            case NOTIFICATIONS_MSG:
                if (m.notifications.data & NOTIFY_TIMER) {
                    service_warn_deadlocked_tasks();
                }
                break;
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
            case BENCHMARK_NOP_MSG:
                r.type = BENCHMARK_NOP_REPLY_MSG;
                r.benchmark_nop_reply.value = m.benchmark_nop.value * 7;
                ipc_reply(m.src, &r);
                break;
            case BENCHMARK_NOP_WITH_OOL_MSG:
                free(m.benchmark_nop_with_ool.data);
                r.type = BENCHMARK_NOP_WITH_OOL_REPLY_MSG;
                r.benchmark_nop_with_ool_reply.data = "reply!";
                r.benchmark_nop_with_ool_reply.data_len = 7;
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
                ASSERT(task->pager == vm_task->tid);
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
                ASSERT(task->pager == vm_task->tid);
                ASSERT(m.page_fault.task == task->tid);

                paddr_t paddr =
                    handle_page_fault(task, m.page_fault.vaddr, m.page_fault.ip,
                                      m.page_fault.fault);
                if (!paddr) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                vaddr_t aligned_vaddr =
                    ALIGN_DOWN(m.page_fault.vaddr, PAGE_SIZE);
                ASSERT_OK(map_page(task, aligned_vaddr, paddr,
                                   MAP_TYPE_READWRITE, false));
                r.type = PAGE_FAULT_REPLY_MSG;

                ipc_reply(task->tid, &r);
                break;
            }
            case DISCOVERY_SERVE_MSG: {
                service_register(caller, m.discovery_serve.name);
                free(m.discovery_serve.name);
                r.type = DISCOVERY_SERVE_REPLY_MSG;
                ipc_reply(m.src, &r);
                break;
            }
            case DISCOVERY_LOOKUP_MSG: {
                task_t server = service_wait(caller, m.discovery_lookup.name);
                free(m.discovery_lookup.name);
                if (IS_OK(server)) {
                    r.type = DISCOVERY_LOOKUP_REPLY_MSG;
                    r.discovery_lookup_reply.task = server;
                    ipc_reply(m.src, &r);
                }
                break;
            }
            case VM_ALLOC_PAGES_MSG: {
                struct task *task = task_lookup(m.src);
                ASSERT(task);

                vaddr_t vaddr = 0;
                paddr_t paddr = m.vm_alloc_pages.paddr;
                error_t err = task_page_alloc(task, &vaddr, &paddr,
                                              m.vm_alloc_pages.num_pages);
                if (err != OK) {
                    ipc_reply_err(m.src, err);
                    break;
                }

                r.type = VM_ALLOC_PAGES_REPLY_MSG;
                r.vm_alloc_pages_reply.vaddr = vaddr;
                r.vm_alloc_pages_reply.paddr = paddr;
                ipc_reply(m.src, &r);
                break;
            }
            case TASK_ALLOC_MSG: {
                struct task *task = task_alloc(m.task_alloc.pager);
                if (!task) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                r.type = TASK_ALLOC_REPLY_MSG;
                r.task_alloc_reply.task = task->tid;
                ipc_reply(m.src, &r);
                break;
            }
            case TASK_FREE_MSG: {
                struct task *task = task_lookup(m.task_free.task);
                if (!task) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                if (task->pager != m.src) {
                    ipc_reply_err(m.src, ERR_NOT_PERMITTED);
                    break;
                }

                task_free(task);
                r.type = TASK_FREE_REPLY_MSG;
                ipc_reply(m.src, &r);
                break;
            }
            case TASK_LAUNCH_MSG: {
                task_t task_or_err =
                    task_spawn_by_cmdline(m.task_launch.name_and_cmdline);
                free(m.task_launch.name_and_cmdline);
                if (IS_ERROR(task_or_err)) {
                    ipc_reply_err(m.src, task_or_err);
                    break;
                }

                r.type = TASK_LAUNCH_REPLY_MSG;
                r.task_launch_reply.task = task_or_err;
                ipc_reply(m.src, &r);
                break;
            }
            case TASK_WATCH_MSG: {
                struct task *task = task_lookup(m.task_watch.task);
                if (!task) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                task_watch(caller, task);
                r.type = TASK_WATCH_REPLY_MSG;
                ipc_reply(m.src, &r);
                break;
            }
            case TASK_UNWATCH_MSG: {
                struct task *task = task_lookup(m.task_unwatch.task);
                if (!task) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                task_unwatch(caller, task);
                r.type = TASK_UNWATCH_REPLY_MSG;
                ipc_reply(m.src, &r);
                break;
            }
            case SHM_CREATE_MSG: {
                struct task *task = task_lookup(m.src);
                ASSERT(task);
                int slot;
                err = shm_create(task, m.shm_create.size, &slot);
                if (err != OK) {
                    ipc_reply_err(m.src, err);
                    break;
                }
                m.type = SHM_CREATE_REPLY_MSG;
                m.shm_create_reply.shm_id = slot;
                ipc_reply(m.src, &m);
                break;
            }
            case SHM_MAP_MSG: {
                struct task *task = task_lookup(m.src);
                error_t err;
                vaddr_t vaddr;
                err =
                    shm_map(task, m.shm_map.shm_id, m.shm_map.writable, &vaddr);
                if (err != OK) {
                    ipc_reply_err(m.src, err);
                    break;
                }
                m.type = SHM_MAP_REPLY_MSG;
                m.shm_map_reply.vaddr = vaddr;
                ipc_reply(m.src, &m);
                break;
            }
            case SHM_CLOSE_MSG: {
                shm_close(m.shm_close.shm_id);
                m.type = SHM_CLOSE_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                discard_unknown_message(&m);
        }
    }
}
