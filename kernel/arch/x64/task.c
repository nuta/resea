#include <arch.h>
#include <string.h>
#include <syscall.h>
#include <task.h>
#include "interrupt.h"
#include "trap.h"

error_t arch_task_create(struct task *task, vaddr_t ip) {
    void *interrupt_stack_bottom = kmalloc(PAGE_SIZE);
    if (!interrupt_stack_bottom) {
        return ERR_NO_MEMORY;
    }

    void *syscall_stack_bottom = kmalloc(PAGE_SIZE);
    if (!syscall_stack_bottom) {
        kfree(interrupt_stack_bottom);
        return ERR_NO_MEMORY;
    }

    void *xsave = kmalloc(PAGE_SIZE);
    if (!xsave) {
        kfree(interrupt_stack_bottom);
        kfree(syscall_stack_bottom);
        return ERR_NO_MEMORY;
    }

    task->arch.interrupt_stack = (uint64_t) interrupt_stack_bottom + PAGE_SIZE;
    task->arch.syscall_stack = (uint64_t) syscall_stack_bottom + PAGE_SIZE;
    task->arch.interrupt_stack_bottom = interrupt_stack_bottom;
    task->arch.syscall_stack_bottom = syscall_stack_bottom;
    task->arch.xsave = xsave;

    // Set up a temporary kernel stack frame.
    uint64_t *rsp = (uint64_t *) task->arch.interrupt_stack;

    // Push a IRET frame.
    *--rsp = USER_DS | USER_RPL;    // DS
    *--rsp = 0;                     // SS
    *--rsp = 0x202;                 // RFLAGS (interrupts enabled).
    *--rsp = USER_CS64 | USER_RPL;  // CS
    *--rsp = ip;                    // RIP

    // Push a temporary context to be restored in switch_context.
    *--rsp = (uint64_t) &userland_entry;
    *--rsp = 0;     // Initial RBP.
    *--rsp = 0;     // Initial RBX.
    *--rsp = 0;     // Initial R12.
    *--rsp = 0;     // Initial R13.
    *--rsp = 0;     // Initial R14.
    *--rsp = 0;     // Initial R15.
    *--rsp = 0x02;  // RFLAGS (interrupts disabled).

    // Set the initial stack pointer value.
    task->arch.rsp = (uint64_t) rsp;
    return OK;
}

void arch_task_destroy(struct task *task) {
    kfree(task->arch.interrupt_stack_bottom);
    kfree(task->arch.syscall_stack_bottom);
    kfree(task->arch.xsave);
}

static void update_tss_iomap(struct task *task) {
    struct tss *tss = &ARCH_CPUVAR->tss;
    memset(tss->iomap, CAPABLE(task, CAP_IO) ? 0x00 : 0xff, TSS_IOMAP_SIZE);
}

void arch_task_switch(struct task *prev, struct task *next) {
    // Disable interrupts in case they're not yet disabled.
    asm_cli();
    // Switch the page table.
    asm_write_cr3(next->vm.pml4);
    // Update the kernel stack for syscall and interrupt/exception handlers.
    ARCH_CPUVAR->rsp0 = next->arch.syscall_stack;
    ARCH_CPUVAR->tss.rsp0 = next->arch.interrupt_stack;
    // Update the I/O bitmap.
    update_tss_iomap(next);
    // Save and restore FPU registers. This may be expensive operation: I
    // think we should implement "lazy FPU switching".
    asm_xsave(prev->arch.xsave);
    asm_xrstor(next->arch.xsave);
    // Restore registers (resume the next thread).
    switch_context(&prev->arch.rsp, &next->arch.rsp);
}

error_t arch_caps_updated(struct task *task) {
    update_tss_iomap(task);
    return OK;
}
