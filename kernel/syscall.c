#include "syscall.h"
#include "arch.h"
#include "ipc.h"
#include "memory.h"
#include "printk.h"
#include "task.h"
#include <string.h>

void memcpy_from_user(void *dst, __user const void *src, size_t len) {
    // arch_memcpy_from_user(dst, src, len);
    // FIXME:
    memcpy(dst, src, len);
}

void memcpy_to_user(__user void *dst, const void *src, size_t len) {
    // FIXME: arch_memcpy_to_user(dst, src, len);

    memcpy(dst, src, len);
}

// Copies bytes from the userspace. It doesn't care NUL characters: it simply
// copies `len` bytes and set '\0' to the `len`-th byte. Thus, `dst` needs to
// have `len + 1` bytes space. In case `len` is zero, caller MUST ensure that
// `dst` has at least 1-byte space.
static void strncpy_from_user(char *dst, __user const char *src, size_t len) {
    memcpy_from_user(dst, src, len);
    dst[len] = '\0';
}

/// Creates a task.
static task_t sys_task_create(__user const char *name, vaddr_t ip, task_t pager,
                              unsigned flags) {
    // Create a task. We handle pager == 0 as an error here.
    // TODO: Remove pager arg
    struct task *pager_task = get_task_by_tid(pager);
    if (!pager_task) {
        return ERR_INVALID_ARG;
    }

    // FIXME: check if pager is created

    char namebuf[TASK_NAME_LEN];
    strncpy_from_user(namebuf, name, sizeof(namebuf) - 1);
    task_t task = task_create(namebuf, ip, pager_task, flags);
    if (IS_ERROR(task)) {
        return task;
    }

    // FIXME: Wait for startup message instead of thsi auto resume
    task_resume(get_task_by_tid(task));
    return task;
}

/// Destroys a task.
static error_t sys_task_destroy(task_t tid) {
    struct task *task = get_task_by_tid(tid);
    if (!task || task == CURRENT_TASK) {
        return ERR_INVALID_TASK;
    }

    return task_destroy(task);
}

/// Exits the current task.
static error_t sys_task_exit(void) {
    // TODO:
    // task_exit(EXP_GRACE_EXIT);
    UNREACHABLE();
}

/// Returns the current task's ID.
static task_t sys_task_self(void) {
    return CURRENT_TASK->tid;
}

static paddr_t sys_pm_alloc(size_t size, unsigned flags) {
    return pm_alloc(size, PAGE_TYPE_USER(CURRENT_TASK), PM_ALLOC_ZEROED);
}

static paddr_t sys_vm_map(task_t tid, uaddr_t uaddr, paddr_t paddr, size_t size,
                          unsigned attrs) {
    struct task *task = get_task_by_tid(tid);
    if (!task) {
        return ERR_INVALID_TASK;
    }

    // TODO: check if caller is the owner of the paddr
    attrs =
        PAGE_WRITABLE | PAGE_READABLE | PAGE_EXECUTABLE | PAGE_USER;  // FIXME:
    return vm_map(task, uaddr, paddr, size, attrs);
}

/// Send/receive IPC messages.
static error_t sys_ipc(task_t dst, task_t src, __user struct message *m,
                       unsigned flags) {
    if (flags & IPC_KERNEL) {
        return ERR_INVALID_ARG;
    }

    if (src < 0 || src > NUM_TASKS_MAX) {
        // FIXME: Check if src surely exist
        return ERR_INVALID_ARG;
    }

    struct task *dst_task = NULL;
    if (flags & IPC_SEND) {
        dst_task = get_task_by_tid(dst);
        if (!dst_task) {
            return ERR_INVALID_TASK;
        }
    }

    return ipc(dst_task, src, m, flags);
}

/// Writes log messages into the arch's console (typically a serial port) and
/// the kernel log buffer.
static error_t sys_console_write(__user const char *buf, size_t buf_len) {
    if (buf_len > 1024) {
        WARN_DBG("console_write: too long buffer length");
        return ERR_TOO_LARGE;
    }

    char kbuf[512];
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
}

/// Reads a string from the arch's console (typically a serial port).
static int sys_console_read(__user char *buf, int max_len) {
    if (!max_len) {
        return 0;
    }

    int i = 0;
    // for (; i < max_len - 1; i++) {
    //     char ch;
    //     if ((ch = kdebug_readchar()) <= 0) {
    //         break;
    //     }

    //     memcpy_to_user(buf + i, &ch, 1);
    // }

    // memcpy_to_user(buf + i, "\0", 1);
    return i;
}

/// The system call handler.
long handle_syscall(int n, long a1, long a2, long a3, long a4, long a5) {
    long ret;
    switch (n) {
        case SYS_IPC:
            ret = sys_ipc(a1, a2, (__user struct message *) a3, a4);
            break;
        case SYS_CONSOLE_WRITE:
            ret = sys_console_write((__user const char *) a1, a2);
            break;
        case SYS_CONSOLE_READ:
            ret = sys_console_read((__user char *) a1, a2);
            break;
        case SYS_TASK_CREATE:
            ret = sys_task_create((__user const char *) a1, a2, a3, a4);
            break;
        case SYS_TASK_DESTROY:
            ret = sys_task_destroy(a1);
            break;
        case SYS_TASK_EXIT:
            ret = sys_task_exit();
            break;
        case SYS_TASK_SELF:
            ret = sys_task_self();
            break;
        case SYS_PM_ALLOC:
            ret = sys_pm_alloc(a1, a2);
            break;
        case SYS_VM_MAP:
            ret = sys_vm_map(a1, a2, a3, a4, a5);
            break;
        default:
            ret = ERR_INVALID_ARG;
    }

    return ret;
}
