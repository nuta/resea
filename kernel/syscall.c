#include <arch.h>
#include <list.h>
#include <string.h>
#include <types.h>
#include "ipc.h"
#include "kdebug.h"
#include "printk.h"
#include "syscall.h"
#include "task.h"

/// Copies bytes from the userspace. If the user's pointer is invalid, this
/// function or the page fault handler kills the current task.
void memcpy_from_user(void *dst, __user const void *src, size_t len) {
    if (is_kernel_addr_range((vaddr_t) src, len)) {
        task_exit(EXP_INVALID_MEMORY_ACCESS);
    }

    arch_memcpy_from_user(dst, src, len);
}

/// Copies bytes into the userspace. If the user's pointer is invalid, this
/// function or the page fault handler kills the current task.
void memcpy_to_user(__user void *dst, const void *src, size_t len) {
    if (is_kernel_addr_range((vaddr_t) dst, len)) {
        task_exit(EXP_INVALID_MEMORY_ACCESS);
    }

    arch_memcpy_to_user(dst, src, len);
}

// Copies bytes from the userspace. It doesn't care NUL characters: it simply
// copies `len` bytes and set '\0' to the `len`-th byte. Thus, `dst` needs to
// have `len + 1` bytes space. In case `len` is zero, caller MUST ensure that
// `dst` has at least 1-byte space.
static void strncpy_from_user(char *dst, __user const char *src, size_t len) {
    memcpy_from_user(dst, src, len);
    dst[len] = '\0';
}

static error_t sys_task_create(task_t tid, __user const char *name, vaddr_t ip,
                               task_t pager, unsigned flags) {
    if (!CAPABLE(CURRENT, CAP_TASK)) {
        return ERR_NOT_PERMITTED;
    }

    struct task *task = task_lookup_unchecked(tid);
    if (!task || task == CURRENT) {
        return ERR_INVALID_TASK;
    }

    // Create a task. We handle pager == 0 as an error here.
    struct task *pager_task = task_lookup(pager);
    if (!pager_task) {
        return ERR_INVALID_ARG;
    }

    char namebuf[CONFIG_TASK_NAME_LEN];
    strncpy_from_user(namebuf, name, sizeof(namebuf) - 1);
    return task_create(task, namebuf, ip, pager_task, flags);
}

/// Destroys a task.
static error_t sys_task_destroy(task_t tid) {
    if (!CAPABLE(CURRENT, CAP_TASK)) {
        return ERR_NOT_PERMITTED;
    }

    struct task *task = task_lookup_unchecked(tid);
    if (!task || task == CURRENT) {
        return ERR_INVALID_TASK;
    }

    return task_destroy(task);
}

/// Exits the current task.
static error_t sys_task_exit(void) {
    task_exit(EXP_GRACE_EXIT);
    UNREACHABLE();
}

/// Returns the current task's ID.
static task_t sys_task_self(void) {
    return CURRENT->tid;
}

/// Send/receive IPC messages.
static error_t sys_ipc(task_t dst, task_t src, __user struct message *m,
                       unsigned flags) {
    if (flags & IPC_KERNEL) {
        return ERR_INVALID_ARG;
    }

    if (src < 0 || src > CONFIG_NUM_TASKS) {
        return ERR_INVALID_ARG;
    }

    struct task *dst_task = NULL;
    if (flags & IPC_SEND) {
        dst_task = task_lookup(dst);
        if (!dst_task) {
            return ERR_INVALID_TASK;
        }
    }

    return ipc(dst_task, src, m, flags);
}

/// Sends notifications.
static error_t sys_notify(task_t dst, notifications_t notifications) {
    struct task *dst_task = task_lookup(dst);
    if (!dst_task) {
        return ERR_INVALID_TASK;
    }

    notify(dst_task, notifications);
    return OK;
}

/// Sets task's timer.
static error_t sys_timer_set(msec_t timeout) {
    CURRENT->timeout = timeout;
    return OK;
}

/// Acquires an IRQ ownership.
static error_t sys_irq_acquire(int irq) {
    if (!CAPABLE(CURRENT, CAP_IRQ)) {
        return ERR_NOT_PERMITTED;
    }

    return task_listen_irq(CURRENT, irq);
}

/// Releases an IRQ ownership.
static error_t sys_irq_release(int irq) {
    if (!CAPABLE(CURRENT, CAP_IRQ)) {
        return ERR_NOT_PERMITTED;
    }

    return task_unlisten_irq(irq);
}

static paddr_t resolve_paddr(vaddr_t vaddr) {
    if (CURRENT->tid == INIT_TASK) {
        if (is_kernel_paddr(vaddr)) {
            return 0;
        }
        return vaddr;
    } else {
        return vm_resolve(CURRENT, vaddr);
    }
}

static error_t sys_vm_map(task_t tid, vaddr_t vaddr, vaddr_t src, vaddr_t kpage,
                       unsigned flags) {
    if (!CAPABLE(CURRENT, CAP_MAP)) {
        return ERR_NOT_PERMITTED;
    }

    if (!IS_ALIGNED(vaddr, PAGE_SIZE) || !IS_ALIGNED(vaddr, PAGE_SIZE)
        || !IS_ALIGNED(kpage, PAGE_SIZE)) {
        return ERR_INVALID_ARG;
    }

    paddr_t paddr = resolve_paddr(src);
    if (!paddr) {
        return ERR_NOT_FOUND;
    }

    paddr_t kpage_paddr = resolve_paddr(kpage);
    if (!kpage_paddr) {
        return ERR_NOT_FOUND;
    }

    if (is_kernel_paddr(paddr)) {
        WARN_DBG("paddr %p points to a kernel memory area", paddr);
        return ERR_NOT_ACCEPTABLE;
    }

    if (is_kernel_paddr(kpage_paddr)) {
        WARN_DBG("kpage %p points to a kernel memory area", kpage);
        return ERR_NOT_ACCEPTABLE;
    }

    struct task *task = task_lookup(tid);
    if (!task) {
        return ERR_INVALID_TASK;
    }

    return task_map_page(task, vaddr, paddr, kpage_paddr, flags);
}

static error_t sys_vm_unmap(task_t tid, vaddr_t vaddr) {
    if (!CAPABLE(CURRENT, CAP_MAP)) {
        return ERR_NOT_PERMITTED;
    }

    if (!IS_ALIGNED(vaddr, PAGE_SIZE)) {
        return ERR_INVALID_ARG;
    }

    struct task *task = task_lookup(tid);
    if (!task) {
        return ERR_INVALID_TASK;
    }

    return task_unmap_page(task, vaddr);
}

/// Writes log messages into the arch's console (typically a serial port) and
/// the kernel log buffer.
static error_t sys_console_write(__user const char *buf, size_t buf_len) {
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
}

/// Reads a string from the arch's console (typically a serial port).
static int sys_console_read(__user char *buf, int max_len) {
    if (!max_len) {
        return 0;
    }

    int i = 0;
    for (; i < max_len - 1; i++) {
        char ch;
        if ((ch = kdebug_readchar()) <= 0) {
            break;
        }

        memcpy_to_user(buf + i, &ch, 1);
    }

    memcpy_to_user(buf + i, "\0", 1);
    return i;
}

/// Runs a kernel debugger command and returns the result.
static error_t sys_kdebug(__user const char *cmd, size_t cmd_len,
                          __user char *buf, size_t buf_len) {
    if (!CAPABLE(CURRENT, CAP_KDEBUG)) {
        return ERR_NOT_PERMITTED;
    }

    char cmd_buf[128];
    if (cmd_len >= sizeof(cmd_buf) - 1) {
        return ERR_TOO_LARGE;
    }

    char out_buf[512];
    out_buf[0] = '\0';
    if (buf_len >= sizeof(out_buf) - 1) {
        return ERR_TOO_LARGE;
    }

    strncpy_from_user(cmd_buf, cmd, cmd_len);

    error_t err = kdebug_run(cmd_buf, out_buf, buf_len);
    if (err != OK) {
        return err;
    }

    memcpy_to_user(buf, out_buf, strlen(out_buf) + 1);
    return OK;
}

/// The system call handler.
long handle_syscall(int n, long a1, long a2, long a3, long a4, long a5) {
    stack_check();

    long ret;
    switch (n) {
        case SYS_IPC:
            ret = sys_ipc(a1, a2, (__user struct message *) a3, a4);
            break;
        case SYS_NOTIFY:
            ret = sys_notify(a1, a2);
            break;
        case SYS_TIMER_SET:
            ret = sys_timer_set(a1);
            break;
        case SYS_CONSOLE_WRITE:
            ret = sys_console_write((__user const char *) a1, a2);
            break;
        case SYS_CONSOLE_READ:
            ret = sys_console_read((__user char *) a1, a2);
            break;
        case SYS_TASK_CREATE:
            ret = sys_task_create(a1, (__user const char *) a2, a3, a4, a5);
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
        case SYS_VM_MAP:
            ret = sys_vm_map(a1, a2, a3, a4, a5);
            break;
        case SYS_VM_UNMAP:
            ret = sys_vm_unmap(a1, a2);
            break;
        case SYS_IRQ_ACQUIRE:
            ret = sys_irq_acquire(a1);
            break;
        case SYS_IRQ_RELEASE:
            ret = sys_irq_release(a1);
            break;
        case SYS_KDEBUG:
            ret = sys_kdebug((__user const char *) a1, a2, (__user char *)  a3,
                             a4);
            break;
        case SYS_NOP:
            ret = OK;
            break;
        default:
            ret = ERR_INVALID_ARG;
    }

    stack_check();
    return ret;
}

#ifdef CONFIG_ABI_EMU
/// The system call handler for ABI emulation.
void abi_emu_hook(trap_frame_t *frame, enum abi_hook_type type) {
    struct message m;
    m.type = ABI_HOOK_MSG;
    m.abi_hook.type = type;
    m.abi_hook.task = CURRENT->tid;
    memcpy(&m.abi_hook.frame, frame, sizeof(m.abi_hook.frame));

    error_t err = ipc(CURRENT->pager, CURRENT->pager->tid,
                      (__user struct message *) &m, IPC_CALL | IPC_KERNEL);
    if (IS_ERROR(err)) {
        WARN_DBG("%s: aborted kernel ipc", CURRENT->name);
        task_exit(EXP_ABORTED_KERNEL_IPC);
    }

    // Check if the reply is valid.
    if (m.type != ABI_HOOK_REPLY_MSG) {
        WARN_DBG("%s: invalid abi hook reply (type=%d)",
                 CURRENT->name, m.type);
        task_exit(EXP_INVALID_MSG_FROM_PAGER);
    }

    memcpy(frame, &m.abi_hook_reply.frame, sizeof(*frame));
}
#endif
