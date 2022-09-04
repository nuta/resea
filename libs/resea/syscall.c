#include <resea/ipc.h>
#include <resea/syscall.h>

// FIXME:
uint32_t syscall(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3,
                 uint32_t arg4, uint32_t arg5) {
    register int32_t a0 __asm__("a0") = arg0;
    register int32_t a1 __asm__("a1") = arg1;
    register int32_t a2 __asm__("a2") = arg2;
    register int32_t a3 __asm__("a3") = arg3;
    register int32_t a4 __asm__("a4") = arg4;
    register int32_t a5 __asm__("a5") = arg5;
    register int32_t result __asm__("a0");
    __asm__ __volatile__("ecall"
                         : "=r"(result)
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5)
                         : "memory");
    return result;
}

error_t sys_ipc(task_t dst, task_t src, struct message *m, unsigned flags) {
    return syscall(SYS_IPC, dst, src, (uintptr_t) m, flags, 0);
}

task_t sys_task_create(const char *name, vaddr_t ip, task_t pager,
                       unsigned flags) {
    return syscall(SYS_TASK_CREATE, (uintptr_t) name, ip, pager, flags, 0);
}

error_t sys_task_destroy(task_t task) {
    return syscall(SYS_TASK_DESTROY, task, 0, 0, 0, 0);
}

error_t sys_task_exit(void) {
    return syscall(SYS_TASK_EXIT, 0, 0, 0, 0, 0);
}

task_t sys_task_self(void) {
    return syscall(SYS_TASK_SELF, 0, 0, 0, 0, 0);
}

paddr_t sys_pm_alloc(size_t size, unsigned flags) {
    return syscall(SYS_PM_ALLOC, size, flags, 0, 0, 0);
}

error_t sys_vm_map(task_t task, uaddr_t uaddr, paddr_t paddr, size_t size,
                   unsigned attrs) {
    return syscall(SYS_VM_MAP, task, uaddr, paddr, size, attrs);
}

error_t sys_console_write(const char *buf, size_t len) {
    return syscall(SYS_CONSOLE_WRITE, (uintptr_t) buf, len, 0, 0, 0);
}

int sys_console_read(char *buf, size_t len) {
    return syscall(SYS_CONSOLE_READ, (uintptr_t) buf, len, 0, 0, 0);
}
