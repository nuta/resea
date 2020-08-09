#include <syscall.h>
#include <string.h>
#include <boot.h>
#include <task.h>

error_t arch_task_create(struct task *task, vaddr_t pc) {
    return OK;
}

void arch_task_destroy(struct task *task) {
}

void arch_task_switch(struct task *prev, struct task *next) {
}
