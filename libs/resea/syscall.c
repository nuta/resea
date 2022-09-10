#include <resea/ipc.h>
#include <resea/syscall.h>

uint32_t syscall(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3,
                 uint32_t a4, uint32_t a5) {
    register int32_t reg_a0 __asm__("a0") = a0;
    register int32_t reg_a1 __asm__("a1") = a1;
    register int32_t reg_a2 __asm__("a2") = a2;
    register int32_t reg_a3 __asm__("a3") = a3;
    register int32_t reg_a4 __asm__("a4") = a4;
    register int32_t reg_a5 __asm__("a5") = a5;
    register int32_t result __asm__("a0");
    __asm__ __volatile__("ecall"
                         : "=r"(result)
                         : "r"(reg_a0), "r"(reg_a1), "r"(reg_a2), "r"(reg_a3),
                           "r"(reg_a4), "r"(reg_a5)
                         : "memory");
    return result;
}

error_t sys_ipc(task_t dst, task_t src, struct message *m, unsigned flags) {
    return syscall(dst, src, (uintptr_t) m, flags, 0, SYS_IPC);
}

task_t sys_task_create(const char *name, vaddr_t ip, task_t pager,
                       unsigned flags) {
    return syscall((uintptr_t) name, ip, pager, flags, 0, SYS_TASK_CREATE);
}

error_t sys_task_destroy(task_t task) {
    return syscall(task, 0, 0, 0, 0, SYS_TASK_DESTROY);
}

__noreturn void sys_task_exit(void) {
    syscall(0, 0, 0, 0, 0, SYS_TASK_EXIT);
    // FIXME: rename
    __builtin_unreachable();
}

task_t sys_task_self(void) {
    return syscall(0, 0, 0, 0, 0, SYS_TASK_SELF);
}

paddr_t sys_pm_alloc(size_t size, unsigned flags) {
    return syscall(size, flags, 0, 0, 0, SYS_PM_ALLOC);
}

error_t sys_vm_map(task_t task, uaddr_t uaddr, paddr_t paddr, size_t size,
                   unsigned attrs) {
    return syscall(task, uaddr, paddr, size, attrs, SYS_VM_MAP);
}

error_t sys_vm_unmap(task_t task, uaddr_t uaddr, size_t size) {
    return syscall(task, uaddr, size, 0, 0, SYS_VM_UNMAP);
}

error_t sys_console_write(const char *buf, size_t len) {
    return syscall((uintptr_t) buf, len, 0, 0, 0, SYS_CONSOLE_WRITE);
}

int sys_console_read(const char *buf, int max_len) {
    return syscall((uintptr_t) buf, max_len, 0, 0, 0, SYS_CONSOLE_READ);
}
