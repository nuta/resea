#include <resea/syscall.h>

error_t task_create(task_t tid, const char *name, vaddr_t ip, task_t pager,
                    unsigned flags) {
    return sys_spawn(tid, name, ip, pager, flags);
}

error_t task_destroy(task_t task) {
    return sys_kill(task);
}

void task_exit(void) {
    sys_kill(0);
}

task_t task_self(void) {
    return sys_listen(-1 /* do nothing */, 0 /* do nothing */);
}

error_t task_map(task_t task, vaddr_t vaddr, vaddr_t src, vaddr_t kpage,
                 unsigned flags) {
    return sys_map(task, vaddr, src, kpage, flags);
}
