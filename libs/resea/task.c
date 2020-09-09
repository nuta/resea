#include <resea/syscall.h>
#include <resea/task.h>

error_t task_create(task_t tid, const char *name, vaddr_t ip, task_t pager,
                    unsigned flags) {
    return sys_task_create(tid, name, ip, pager, flags);
}

error_t task_destroy(task_t task) {
    return sys_task_destroy(task);
}

__noreturn void task_exit(void) {
    sys_task_exit();
    for (;;);
}

task_t task_self(void) {
    return sys_task_self();
}

error_t task_map(task_t task, vaddr_t vaddr, vaddr_t src, vaddr_t kpage,
                 unsigned flags) {
    return sys_vm_map(task, vaddr, src, kpage, flags);
}

error_t task_unmap(task_t task, vaddr_t vaddr) {
    return sys_vm_unmap(task, vaddr);
}
