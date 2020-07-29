#ifndef __RESEA_SYSCALL_H__
#define __RESEA_SYSCALL_H__

#include <arch/syscall.h>
#include <types.h>

static inline error_t sys_spawn(task_t tid, const char *name, vaddr_t ip,
                                task_t pager, unsigned flags) {
    return syscall(SYS_SPAWN, tid, (uintptr_t) name, ip, pager, flags);
}

static inline error_t sys_kill(task_t task) {
    return syscall(SYS_KILL, task, 0, 0, 0, 0);
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

static inline error_t sys_kdebug(const char *cmdline, char *buf, size_t len) {
    return syscall(SYS_KDEBUG, (uintptr_t) cmdline, (uintptr_t) buf, len, 0, 0);
}

#endif
