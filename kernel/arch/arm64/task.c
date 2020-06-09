#include <syscall.h>
#include <cstring.h>
#include <main.h>
#include <task.h>
#include <memory.h>
#include "asm.h"

void arm64_start_task(void);

// Prepare the initial stack for arm64_task_switch().
static void init_stack(struct task *task, vaddr_t pc) {
    uint64_t *sp = (uint64_t *) ((vaddr_t) task->arch.exception_stack_bottom + PAGE_SIZE);
    // Fill the stack values for arm64_start_task().
    *--sp = pc;

    int num_zeroed_regs = 11; // x19-x29
    for (int i = 0; i < num_zeroed_regs; i++) {
        *--sp = 0;
    }
    *--sp = (vaddr_t) arm64_start_task; // Task starts here (x30).

    task->arch.stack = (vaddr_t) sp;
}

error_t arch_task_create(struct task *task, vaddr_t pc) {
    void *syscall_stack = kmalloc(PAGE_SIZE);
    if (!syscall_stack) {
        return ERR_NO_MEMORY;
    }

    void *exception_stack = kmalloc(PAGE_SIZE);
    if (!exception_stack) {
        kfree(syscall_stack);
        return ERR_NO_MEMORY;
    }

    task->arch.syscall_stack = (vaddr_t) syscall_stack + PAGE_SIZE;
    task->arch.syscall_stack_bottom = syscall_stack;
    task->arch.exception_stack_bottom = exception_stack;
    init_stack(task, pc);
    return OK;
}

void arch_task_destroy(struct task *task) {
    kfree((void *) task->arch.syscall_stack_bottom);
    kfree((void *) task->arch.exception_stack_bottom);
}

void arm64_task_switch(vaddr_t *prev_sp, vaddr_t next_sp);

void arch_task_switch(struct task *prev, struct task *next) {
    ARM64_MSR(ttbr0_el1, next->vm.ttbr0);
    __asm__ __volatile__("dsb ish");
    __asm__ __volatile__("isb");
    __asm__ __volatile__("tlbi vmalle1is");
    __asm__ __volatile__("dsb ish");
    __asm__ __volatile__("isb");

    arm64_task_switch(&prev->arch.stack, next->arch.stack);
}
