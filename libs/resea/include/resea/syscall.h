#ifndef __RESEA_SYSCALL_H__
#define __RESEA_SYSCALL_H__

#include <arch/syscall.h>
#include <types.h>

struct message;
static inline error_t sys_ipc(task_t dst, task_t src, struct message *m,
                              unsigned flags) {
    return syscall(SYS_IPC, dst, src, (uintptr_t) m, flags, 0);
}

static inline error_t sys_notify(task_t dst, notifications_t notifications) {
    return syscall(SYS_NOTIFY, dst, notifications, 0, 0, 0);
}

static inline error_t sys_timer_set(msec_t timeout) {
    return syscall(SYS_TIMER_SET, timeout, 0, 0, 0, 0);
}

static inline task_t sys_task_create(task_t tid, const char *name, vaddr_t ip,
                                     task_t pager, unsigned flags) {
    return syscall(SYS_TASK_CREATE, tid, (uintptr_t) name, ip, pager, flags);
}

static inline error_t sys_task_destroy(task_t task) {
    return syscall(SYS_TASK_DESTROY, task, 0, 0, 0, 0);
}

static inline error_t sys_task_exit(void) {
    return syscall(SYS_TASK_EXIT, 0, 0, 0, 0, 0);
}

static inline task_t sys_task_self(void) {
    return syscall(SYS_TASK_SELF, 0, 0, 0, 0, 0);
}

static inline error_t sys_vm_map(task_t task, vaddr_t vaddr, vaddr_t src,
                                 vaddr_t kpage, unsigned flags) {
    return syscall(SYS_VM_MAP, task, vaddr, src, kpage, flags);
}

static inline error_t sys_vm_unmap(task_t task, vaddr_t vaddr) {
    return syscall(SYS_VM_UNMAP, task, vaddr, 0, 0, 0);
}

static inline error_t sys_irq_acquire(unsigned irq) {
    return syscall(SYS_IRQ_ACQUIRE, irq, 0, 0, 0, 0);
}

static inline error_t sys_irq_release(unsigned irq) {
    return syscall(SYS_IRQ_RELEASE, irq, 0, 0, 0, 0);
}

static inline error_t sys_console_write(const char *buf, size_t len) {
    return syscall(SYS_CONSOLE_WRITE, (uintptr_t) buf, len, 0, 0, 0);
}

static inline int sys_console_read(char *buf, size_t len) {
    return syscall(SYS_CONSOLE_READ, (uintptr_t) buf, len, 0, 0, 0);
}

static inline error_t sys_kdebug(const char *cmd, size_t cmd_len, char *buf,
                                 size_t buf_len) {
    return syscall(SYS_KDEBUG, (uintptr_t) cmd, cmd_len, (uintptr_t) buf, buf_len, 0);
}

#endif
