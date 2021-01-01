#include "elf.h"
#include "fs.h"
#include "mm.h"
#include "proc.h"
#include "syscall.h"
#include <list.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>

// FIXME:
void on_new_data(void);
struct proc *blocked_proc = NULL;

void main(void) {
    TRACE("starting...");
    proc_init();
    fs_init();

    // Spawn the init process.
    // FIXME: char *argv[] = { "init", NULL };
    // char *argv[] = { "busybox", "sh", "-c", "uname", NULL };
    char *argv[] = {"/bin/sh", NULL};
    char *envp[] = {NULL};
    errno_t errno = proc_execve(init_proc, "/bin/sh", argv, envp);
    if (errno < 0) {
        PANIC("failed to spawn the init process: %d", errno);
    }

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG:
                if (m.notifications.data & NOTIFY_IRQ) {
                    on_new_data();
                }
                break;
            case ABI_HOOK_MSG: {
                task_t task = m.abi_hook.task;
                struct proc *proc = proc_lookup_by_task(task);
                ASSERT(proc);

                switch (m.abi_hook.type) {
                    case ABI_HOOK_TYPE_INITIAL: {
                        struct message m;
                        m.type = ABI_HOOK_REPLY_MSG;

                        trap_frame_t *ret = &m.abi_hook_reply.frame;
                        if (proc->state == PROC_FORKED) {
                            memcpy(ret, &proc->frame, sizeof(*ret));
                            ret->rax = 0;
                        } else {
                            memset(ret, 0, sizeof(*ret));
                            ret->rip = proc->ehdr->e_entry;
                            ret->rsp = proc->stack->vaddr + STACK_TOP;
                            ret->rflags = 0x202;
                            ret->fsbase = proc->fsbase;
                            ret->gsbase = proc->gsbase;
                        }

                        ipc_reply(proc->task, &m);
                        break;
                    }
                    case ABI_HOOK_TYPE_SYSCALL:
                        handle_syscall(proc, &m.abi_hook.frame);
                        break;
                    default:
                        PANIC("unknown abi hook type (%d)", m.abi_hook.type);
                }
                break;
            }
            case EXCEPTION_MSG: {
                NYI();
                break;
            }
            case PAGE_FAULT_MSG: {
                task_t task = m.page_fault.task;
                struct proc *proc = proc_lookup_by_task(task);
                ASSERT(proc);

                error_t err = handle_page_fault(proc, m.page_fault.vaddr,
                                                m.page_fault.fault);
                if (err == OK) {
                    m.type = PAGE_FAULT_REPLY_MSG;
                    ipc_reply(proc->task, &m);
                }

                break;
            }
            default:
                discard_unknown_message(&m);
        }
    }
}
