#include <boot.h>
#include <string.h>
#include <syscall.h>
#include <task.h>

static uint8_t kernel_stacks[CONFIG_NUM_TASKS][STACK_SIZE] __aligned(
    STACK_SIZE);

void arm_start_task(void);

// Prepare the initial stack for arm_task_switch().
static void init_stack(struct task *task, vaddr_t pc) {
    uint32_t *sp =
        (uint32_t *) ((vaddr_t) task->arch.stack_bottom + STACK_SIZE);
    // Fill the exception stack frame. After the first context switch, the CPU
    // always runs the kernel code in the handler mode because of exceptions
    // (e.g. timer interrupts and system calls).
    *--sp = 1 << 24;  // psr
    *--sp =
        pc & ~1;  // pc: LSB should be cleared when returning from the exception
    *--sp = 0;    // lr
    *--sp = 0;    // r12
    *--sp = 0;    // r3
    *--sp = 0;    // r2
    *--sp = 0;    // r1
    *--sp = 0;    // r0
    *--sp = 0xfffffff9;  // return from exception

    *--sp = (vaddr_t) arm_start_task;  // Task starts here.
    int num_zeroed_regs = 8;           // r4-r11
    for (int i = 0; i < num_zeroed_regs; i++) {
        *--sp = 0;
    }

    task->arch.stack = (vaddr_t) sp;
}

error_t arch_task_create(struct task *task, vaddr_t pc) {
    if (task->tid == 0) {
        // Idle tasks.
        return OK;
    }

    task->arch.stack_bottom = kernel_stacks[task->tid - 1];
    init_stack(task, pc);
    return OK;
}

void arch_task_destroy(struct task *task) {
}

void arm_task_switch(vaddr_t *prev_sp, vaddr_t next_sp);

void arch_task_switch(struct task *prev, struct task *next) {
    arm_task_switch(&prev->arch.stack, next->arch.stack);
}
