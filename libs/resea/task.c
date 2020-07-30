#include <resea/syscall.h>

error_t task_create(task_t tid, const char *name, vaddr_t ip, task_t pager,
                    unsigned flags) {
    return sys_exec(tid, name, ip, pager, flags);
}

error_t task_destroy(task_t task) {
    return sys_exec(task, NULL, 0, 0, 0);
}

void task_exit(void) {
    sys_exec(0, NULL, 0, 0, 0);
}

task_t task_self(void) {
    return sys_exec(-1, NULL, 0, 0, 0);
}

error_t task_map(task_t task, vaddr_t vaddr, vaddr_t src, vaddr_t kpage,
                 unsigned flags) {
    return sys_map(task, vaddr, src, kpage, flags);
}
