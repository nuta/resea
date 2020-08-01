#ifndef __RESEA_SYSCALL_H__
#define __RESEA_SYSCALL_H__

#include <arch/syscall.h>
#include <types.h>

static inline task_t sys_exec(task_t tid, const char *name, vaddr_t ip,
                              task_t pager, unsigned flags) {
    return syscall(SYS_EXEC, tid, (uintptr_t) name, ip, pager, flags);
}

static inline task_t sys_listen(msec_t timeout, int irq) {
    return syscall(SYS_LISTEN, timeout, irq, 0, 0, 0);
}

struct message;
static inline error_t sys_ipc(task_t dst, task_t src, struct message *m,
                              unsigned flags) {
    return syscall(SYS_IPC, dst, src, (uintptr_t) m, flags, 0);
}

static inline error_t sys_map(task_t task, vaddr_t vaddr, vaddr_t src,
                              vaddr_t kpage, unsigned flags) {
    return syscall(SYS_MAP, task, vaddr, src, kpage, flags);
}

static inline error_t sys_print(const char *buf, size_t len) {
    return syscall(SYS_PRINT, (uintptr_t) buf, len, 0, 0, 0);
}

static inline error_t sys_kdebug(const char *cmd, size_t cmd_len, char *buf,
                                 size_t buf_len) {
    return syscall(SYS_KDEBUG, (uintptr_t) cmd, cmd_len, (uintptr_t) buf, buf_len, 0);
}

#endif
