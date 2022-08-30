#include "task.h"
#include "switch.h"
#include "trap.h"
#include <kernel/arch.h>
#include <kernel/memory.h>
#include <kernel/task.h>

error_t arch_task_init(struct task *task, uaddr_t ip) {
    uint32_t *sp = (uint32_t *) arch_paddr2ptr(
        pm_alloc(KERNEL_STACK_SIZE / PAGE_SIZE, PAGE_TYPE_KERNEL,
                 PM_ALLOC_UNINITIALIZED));

    // Registeres to be restored in riscv_user_entry.
    *--sp = ip;

    // Registers to be restored in riscv_task_switch.
    *--sp = (uint32_t) riscv_user_entry;  // ra
    *--sp = 0;                            // s0
    *--sp = 0;                            // s1
    *--sp = 0;                            // s2
    *--sp = 0;                            // s3
    *--sp = 0;                            // s4
    *--sp = 0;                            // s5
    *--sp = 0;                            // s6
    *--sp = 0;                            // s7
    *--sp = 0;                            // s8
    *--sp = 0;                            // s9
    *--sp = 0;                            // s10
    *--sp = 0;                            // s11

    task->arch.sp = (uint32_t) sp;
    return OK;
}

void arch_task_switch(struct task *prev, struct task *next) {
    riscv_task_switch(&prev->arch.sp, &next->arch.sp);
}
