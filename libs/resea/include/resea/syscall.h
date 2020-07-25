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

static inline task_t sys_setattrs(const void *bulk_ptr, size_t bulk_len,
                                  msec_t timeout) {
    return syscall(SYS_SETATTRS, (uint64_t) bulk_ptr, bulk_len, timeout,
                   0, 0);
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

static inline error_t sys_writelog(const char *buf, size_t len) {
    return syscall(SYS_WRITELOG, (uintptr_t) buf, len, 0, 0, 0);
}

static inline int sys_readlog(char *buf, size_t len, bool listen) {
    return syscall(SYS_READLOG, (uintptr_t) buf, len, listen, 0, 0);
}

static inline error_t sys_kdebug(const char *cmdline) {
    return syscall(SYS_KDEBUG, (uintptr_t) cmdline, 0, 0, 0, 0);
}

#endif
