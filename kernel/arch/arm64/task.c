#include "asm.h"
#include <boot.h>
#include <string.h>
#include <syscall.h>
#include <task.h>

void arm64_start_task(void);

static uint64_t page_tables[CONFIG_NUM_TASKS][512] __aligned(PAGE_SIZE);
static uint8_t kernel_stacks[CONFIG_NUM_TASKS][STACK_SIZE] __aligned(
    STACK_SIZE);
static uint8_t exception_stacks[CONFIG_NUM_TASKS][STACK_SIZE] __aligned(
    STACK_SIZE);

// Prepare the initial stack for arm64_task_switch().
static void init_stack(struct task *task, vaddr_t pc) {
    // Initialize the page table.
    memset(task->arch.page_table, 0, PAGE_SIZE);
    task->arch.ttbr0 = into_paddr(task->arch.page_table);

    vaddr_t exception_stack = (vaddr_t) exception_stacks[task->tid];
    uint64_t *sp = (uint64_t *) (exception_stack + STACK_SIZE);
    // Fill the stack values for arm64_start_task().
    *--sp = pc;

    int num_zeroed_regs = 11;  // x19-x29
    for (int i = 0; i < num_zeroed_regs; i++) {
        *--sp = 0;
    }
    *--sp = (vaddr_t) arm64_start_task;  // Task starts here (x30).

    task->arch.stack = (vaddr_t) sp;
}

error_t arch_task_create(struct task *task, vaddr_t pc) {
    void *syscall_stack = (void *) kernel_stacks[task->tid];
    task->arch.page_table = page_tables[task->tid];
    task->arch.syscall_stack = (vaddr_t) syscall_stack + STACK_SIZE;
    init_stack(task, pc);
    return OK;
}

void arch_task_destroy(struct task *task) {
}

void arm64_task_switch(vaddr_t *prev_sp, vaddr_t next_sp);

void arch_task_switch(struct task *prev, struct task *next) {
    ARM64_MSR(ttbr0_el1, next->arch.ttbr0);
    __asm__ __volatile__("dsb ish");
    __asm__ __volatile__("isb");
    __asm__ __volatile__("tlbi vmalle1is");
    __asm__ __volatile__("dsb ish");
    __asm__ __volatile__("isb");

    arm64_task_switch(&prev->arch.stack, next->arch.stack);
}
