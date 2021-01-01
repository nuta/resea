#include <boot.h>
#include <string.h>
#include <syscall.h>
#include <task.h>

error_t arch_task_create(struct task *task, vaddr_t pc) {
    return OK;
}

void arch_task_destroy(struct task *task) {
}

void arch_task_switch(struct task *prev, struct task *next) {
}
