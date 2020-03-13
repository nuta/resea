#include <arch.h>
#include <list.h>
#include <string.h>
#include <types.h>
#include "interrupt.h"
#include "ipc.h"
#include "kdebug.h"
#include "memory.h"
#include "printk.h"
#include "syscall.h"
#include "task.h"

/// Copies bytes from the userspace. If the user's pointer is invalid, this
/// function or the page fault handler kills the current task.
void memcpy_from_user(void *dst, userptr_t src, size_t len) {
    if (is_kernel_addr_range(src, len)) {
        task_exit(EXP_INVALID_MEMORY_ACCESS);
    }

    arch_memcpy_from_user(dst, src, len);
}

/// Copies bytes into the userspace. If the user's pointer is invalid, this
/// function or the page fault handler kills the current task.
void memcpy_to_user(userptr_t dst, const void *src, size_t len) {
    if (is_kernel_addr_range(dst, len)) {
        task_exit(EXP_INVALID_MEMORY_ACCESS);
    }

    arch_memcpy_to_user(dst, src, len);
}

/// Copy a string terminated by NUL from the userspace. If the user's pointer is
/// invalid, this function or the page fault handler kills the current task.
static void strncpy_from_user(char *dst, userptr_t src, size_t max_len) {
    if (is_kernel_addr_range(src, max_len)) {
        task_exit(EXP_INVALID_MEMORY_ACCESS);
    }

    arch_strncpy_from_user(dst, src, max_len);
}

static error_t sys_ipcctl(userptr_t bulk_ptr, size_t bulk_len, msec_t timeout) {
    if (!CAPABLE(CAP_IPC)) {
        return ERR_NOT_PERMITTED;
    }

    if (bulk_ptr) {
        CURRENT->bulk_ptr = bulk_ptr;
        CURRENT->bulk_len = bulk_len;

        // Resolve page faults in advance. Handling them in the sender context
        // would be pretty tricky...
        size_t remaining = bulk_len;
        size_t offset = 0;
        while (remaining > 0) {
            userptr_t addr = ALIGN_DOWN(bulk_ptr + offset, PAGE_SIZE);
            if (!vm_resolve(&CURRENT->vm, addr)) {
                handle_page_fault(addr, PF_USER | PF_WRITE);
            }

            remaining -= MIN(remaining, PAGE_SIZE);
            offset += PAGE_SIZE;
        }
    }

    if (timeout) {
        CURRENT->timeout = timeout;
    }

    return OK;
}

static error_t sys_ipc(tid_t dst, tid_t src, userptr_t m, unsigned flags) {
    if (!CAPABLE(CAP_IPC)) {
        return ERR_NOT_PERMITTED;
    }

    if (flags & IPC_KERNEL) {
        return ERR_INVALID_ARG;
    }

    if (src < 0 || src > TASKS_MAX) {
        return ERR_INVALID_ARG;
    }

    struct task *dst_task = NULL;
    if (flags & (IPC_SEND | IPC_LISTEN | IPC_NOTIFY)) {
        dst_task = task_lookup(dst);
        if (!dst_task) {
            return ERR_INVALID_ARG;
        }

        if (flags & IPC_NOTIFY) {
            notify(dst_task, m);
            return OK;
        }
    }

    return ipc(dst_task, src, (struct message *) m, flags);
}

/// The taskctl system call does all task-related operations. The operation is
/// determined as below:
///
///        | task_create | task_destroy | task_exit | task_self | caps_drop
///  ------+-------------+--------------+-----------+-----------+-----------
///  tid   |     > 0     |      > 0     |    0      |    ---    |     ---
///  pager |     > 0     |       0      |    0      |    -1     |     -1
///
static tid_t sys_taskctl(tid_t tid, userptr_t name, vaddr_t ip, tid_t pager,
                         caps_t caps) {
    // Since task_exit(), task_self(), and caps_drop() are unprivileged, we
    // don't need to check the capabilities here.
    if (!tid && !pager) {
        task_exit(EXP_GRACE_EXIT);
    }

    if (pager < 0) {
        // Do caps_drop() and task_self() at once.
        CURRENT->caps &= ~caps;
        return CURRENT->tid;
    }

    // Check the capability before handling privileged operations.
    if (!CAPABLE(CAP_TASK)) {
        return ERR_NOT_PERMITTED;
    }

    // Look for the target task.
    struct task *task = task_lookup(tid);
    if (!task || task == CURRENT) {
        return ERR_INVALID_ARG;
    }

    if (pager) {
        struct task *pager_task = task_lookup(pager);
        if (!pager_task) {
            return ERR_INVALID_ARG;
        }

        // Create a task.
        char namebuf[TASK_NAME_LEN];
        strncpy_from_user(namebuf, name, sizeof(namebuf));
        return task_create(task, namebuf, ip, pager_task, CURRENT->caps & caps);
    } else {
        // Destroy the task.
        return task_destroy(task);
    }
}

static error_t sys_irqctl(unsigned irq, bool enable) {
    if (!CAPABLE(CAP_IO)) {
        return ERR_NOT_PERMITTED;
    }

    if (enable) {
        return task_listen_irq(CURRENT, irq);
    } else {
        return task_unlisten_irq(CURRENT, irq);
    }
}

static int sys_klogctl(userptr_t buf, size_t buf_len, bool write) {
    if (!CAPABLE(CAP_KLOG)) {
        return ERR_NOT_PERMITTED;
    }

    if (write) {
        char kbuf[256];
        int remaining = buf_len;
        while (remaining > 0) {
            int copy_len = MIN(remaining, (int) sizeof(kbuf));
            memcpy_from_user(kbuf, buf, copy_len);
            for (int i = 0; i < copy_len; i++) {
                printk("%c", kbuf[i]);
            }
            remaining -= copy_len;
        }

        return OK;
    } else {
        char kbuf[256];
        int remaining = buf_len;
        while (remaining > 0) {
            int read_len = klog_read(kbuf, MIN(remaining, (int) sizeof(kbuf)));
            if (!read_len) {
                break;
            }

            memcpy_to_user(buf, kbuf, read_len);
            buf += read_len;
            remaining -= read_len;
        }

        return buf_len - remaining;
    }
}

/// The system call handler.
uintmax_t handle_syscall(uintmax_t syscall, uintmax_t arg1, uintmax_t arg2,
                         uintmax_t arg3, uintmax_t arg4, uintmax_t arg5) {
    stack_check();

    uintmax_t ret;
    switch (syscall) {
        case SYSCALL_IPC:
            ret = (uintmax_t) sys_ipc(arg1, arg2, arg3, arg4);
            break;
        case SYSCALL_IPCCTL:
            ret = (uintmax_t) sys_ipcctl(arg1, arg2, arg3);
            break;
        case SYSCALL_TASKCTL:
            ret = (uintmax_t) sys_taskctl(arg1, arg2, arg3, arg4, arg5);
            break;
        case SYSCALL_IRQCTL:
            ret = (uintmax_t) sys_irqctl(arg1, arg2);
            break;
        case SYSCALL_KLOGCTL:
            ret = (uintmax_t) sys_klogctl(arg1, arg2, arg3);
            break;
        default:
            return ERR_INVALID_ARG;
    }

    stack_check();
    return ret;
}
