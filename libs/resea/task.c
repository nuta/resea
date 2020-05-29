#include <resea/syscall.h>

error_t task_create(task_t tid, const char *name, vaddr_t ip, task_t pager,
                    caps_t caps) {
    return sys_spawn(tid, name, ip, pager, caps);
}

error_t task_destroy(task_t task) {
    return sys_kill(task);
}

void task_exit(void) {
    sys_kill(0);
}

task_t task_self(void) {
    return sys_setattrs(NULL, 0, 0);
}
