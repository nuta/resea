#include <resea/syscall.h>
#include <resea/task.h>

task_t task_create(const char *name, vaddr_t ip, task_t pager, unsigned flags) {
    return sys_task_create(name, ip, pager, flags);
}

error_t task_destroy(task_t task) {
    return sys_task_destroy(task);
}

__noreturn void task_exit(void) {
    sys_task_exit();
    for (;;)
        ;
}

task_t task_self(void) {
    return sys_task_self();
}
