#include "syscall.h"
#include "arch.h"
#include "ipc.h"
#include "memory.h"
#include "printk.h"
#include "task.h"
#include <math.h>
#include <string.h>

void memcpy_from_user(void *dst, __user const void *src, size_t len) {
    arch_memcpy_from_user(dst, src, len);
}

void memcpy_to_user(__user void *dst, const void *src, size_t len) {
    arch_memcpy_to_user(dst, src, len);
}

// Copies bytes from the userspace. It doesn't care NUL characters: it simply
// copies `len` bytes and set '\0' to the `len`-th byte. Thus, `dst` needs to
// have `len + 1` bytes space. In case `len` is zero, caller MUST ensure that
// `dst` has at least 1-byte space.
static void strncpy_from_user(char *dst, __user const char *src, size_t len) {
    DEBUG_ASSERT(len > 0);
    memcpy_from_user(dst, src, len - 1);
    dst[len - 1] = '\0';
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

    // TODO: check if pager is created

    char namebuf[TASK_NAME_LEN];
    strncpy_from_user(namebuf, name, sizeof(namebuf));
    task_t task = task_create(namebuf, ip, pager_task, flags);
    if (IS_ERROR(task)) {
        return task;
    }

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
__noreturn static void sys_task_exit(void) {
    task_exit(EXCEPTION_GRACE_EXIT);
}

/// Returns the current task's ID.
static task_t sys_task_self(void) {
    return CURRENT_TASK->tid;
}

static paddr_t sys_pm_alloc(size_t size, unsigned flags) {
    // TODO: respect flags
    return pm_alloc(size, PAGE_TYPE_USER(CURRENT_TASK), PM_ALLOC_ZEROED);
}

static paddr_t sys_vm_map(task_t tid, uaddr_t uaddr, paddr_t paddr, size_t size,
                          unsigned attrs) {
    struct task *task = get_task_by_tid(tid);
    if (!task) {
        return ERR_INVALID_TASK;
    }

    if ((attrs & ~(PAGE_WRITABLE | PAGE_READABLE | PAGE_EXECUTABLE)) != 0) {
        return ERR_INVALID_ARG;
    }

    if (!IS_ALIGNED(uaddr, PAGE_SIZE) || !IS_ALIGNED(paddr, PAGE_SIZE)
        || !IS_ALIGNED(size, PAGE_SIZE)) {
        return ERR_INVALID_ARG;
    }

    if (!arch_is_mappable_uaddr(uaddr)) {
        return ERR_NOT_ALLOWED;
    }

    // TODO: check if caller is the owner of the paddr
    attrs |= PAGE_USER;
    return vm_map(task, uaddr, paddr, size, attrs);
}

static paddr_t sys_vm_unmap(task_t tid, uaddr_t uaddr, size_t size) {
    struct task *task = get_task_by_tid(tid);
    if (!task) {
        return ERR_INVALID_TASK;
    }

    if (!IS_ALIGNED(uaddr, PAGE_SIZE) || !IS_ALIGNED(size, PAGE_SIZE)) {
        return ERR_INVALID_ARG;
    }

    if (!arch_is_mappable_uaddr(uaddr)) {
        return ERR_NOT_ALLOWED;
    }

    return vm_unmap(task, uaddr, size);
}

/// Send/receive IPC messages.
static error_t sys_ipc(task_t dst, task_t src, __user struct message *m,
                       unsigned flags) {
    if (flags & IPC_KERNEL) {
        return ERR_INVALID_ARG;
    }

    if (src < 0 || src > NUM_TASKS_MAX) {
        // TODO: Check if src surely exist
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

// TODO: move
static list_t console_readers = LIST_INIT(console_readers);
static char console_buf[128];
static int console_buf_rp = 0;
static int console_buf_wp = 0;

void handle_console_interrupt(void) {
    while (true) {
        int ch = arch_console_read();
        if (ch == -1) {
            break;
        }

        console_buf[console_buf_wp] = ch;
        console_buf_wp = (console_buf_wp + 1) % sizeof(console_buf);
        // TODO: What if it's full
    }

    LIST_FOR_EACH (task, &console_readers, struct task, next) {
        list_remove(&task->next);
        task_resume(task);
    }
}

/// Reads a string from the arch's console (typically a serial port).
static int sys_console_read(__user char *buf, int max_len) {
    if (!max_len) {
        return 0;
    }

    int i = 0;
    while (true) {
        for (; i < max_len - 1 && console_buf_rp != console_buf_wp; i++) {
            char ch = console_buf[console_buf_rp];
            console_buf_rp = (console_buf_rp + 1) % sizeof(console_buf);
            memcpy_to_user(buf + i, &ch, 1);
        }

        if (i > 0) {
            break;
        }

        list_push_back(&console_readers, &CURRENT_TASK->next);
        task_block(CURRENT_TASK);
        task_switch();
    }

    memcpy_to_user(buf + i, "\0", 1);
    return i;
}

/// The system call handler.
long handle_syscall(long a0, long a1, long a2, long a3, long a4, long n) {
    long ret;
    switch (n) {
        case SYS_IPC:
            ret = sys_ipc(a0, a1, (__user struct message *) a2, a3);
            break;
        case SYS_CONSOLE_WRITE:
            ret = sys_console_write((__user const char *) a0, a1);
            break;
        case SYS_CONSOLE_READ:
            ret = sys_console_read((__user char *) a0, a1);
            break;
        case SYS_TASK_CREATE:
            ret = sys_task_create((__user const char *) a0, a1, a2, a3);
            break;
        case SYS_TASK_DESTROY:
            ret = sys_task_destroy(a0);
            break;
        case SYS_TASK_EXIT:
            sys_task_exit();
            UNREACHABLE();
        case SYS_TASK_SELF:
            ret = sys_task_self();
            break;
        case SYS_PM_ALLOC:
            ret = sys_pm_alloc(a0, a1);
            break;
        case SYS_VM_MAP:
            ret = sys_vm_map(a0, a1, a2, a3, a4);
            break;
        case SYS_VM_UNMAP:
            ret = sys_vm_unmap(a0, a1, a2);
            break;
        default:
            ret = ERR_INVALID_ARG;
    }

    return ret;
}
