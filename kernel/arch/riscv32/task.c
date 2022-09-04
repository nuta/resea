#include "task.h"
#include "asm.h"
#include "switch.h"
#include "trap.h"
#include <kernel/arch.h>
#include <kernel/memory.h>
#include <kernel/printk.h>
#include <kernel/task.h>

error_t arch_task_init(struct task *task, uaddr_t ip) {
    paddr_t sp_bottom =
        pm_alloc(KERNEL_STACK_SIZE, PAGE_TYPE_KERNEL, PM_ALLOC_UNINITIALIZED);
    if (!sp_bottom) {
        return ERR_NO_MEMORY;
    }

    uint32_t *sp = (uint32_t *) arch_paddr2ptr(sp_bottom + KERNEL_STACK_SIZE);

    // Registers to be restored in riscv32_user_entry.
    *--sp = task->vm.table;
    *--sp = ip;

    // Registers to be restored in riscv32_task_switch.
    *--sp = 0;                              // s11
    *--sp = 0;                              // s10
    *--sp = 0;                              // s9
    *--sp = 0;                              // s8
    *--sp = 0;                              // s7
    *--sp = 0;                              // s6
    *--sp = 0;                              // s5
    *--sp = 0;                              // s4
    *--sp = 0;                              // s3
    *--sp = 0;                              // s2
    *--sp = 0;                              // s1
    *--sp = 0;                              // s0
    *--sp = (uint32_t) riscv32_user_entry;  // ra

    task->arch.sp = (uint32_t) sp;
    return OK;
}

__noreturn void error() {
    PANIC("Exception!");
}

__noreturn void do_riscv32_user_entry(uint32_t ip, uint32_t satp) {
    uint32_t sstatus = read_sstatus();
    sstatus &= ~SSTATUS_SPP_MASK;
    write_sstatus(sstatus);

    // FIXME: DOn't need to switch!
    write_satp((1ul << 31) | satp >> 12);

    write_sepc(ip);
    __asm__ __volatile__("sret");
    UNREACHABLE();
}

void arch_task_switch(struct task *prev, struct task *next) {
    CPUVAR->arch.sp = next->arch.sp;

    write_satp((1ul << 31) | next->vm.table >> 12);

    riscv32_task_switch(&prev->arch.sp, &next->arch.sp);
}
