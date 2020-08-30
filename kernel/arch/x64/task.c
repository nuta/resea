#include <arch.h>
#include <string.h>
#include <syscall.h>
#include <task.h>
#include "interrupt.h"
#include "trap.h"
#include "vm.h"

static uint64_t pml4_tables[CONFIG_NUM_TASKS][512] __aligned(PAGE_SIZE);
static uint8_t kernel_stacks[CONFIG_NUM_TASKS][STACK_SIZE] __aligned(STACK_SIZE);
static uint8_t syscall_stacks[CONFIG_NUM_TASKS][STACK_SIZE] __aligned(STACK_SIZE);
static uint8_t xsave_areas[CONFIG_NUM_TASKS][4096] __aligned(4096);

error_t arch_task_create(struct task *task, vaddr_t ip) {
    void *kstack = (void *) kernel_stacks[task->tid];
    void *syscall_stack_bottom = (void *) syscall_stacks[task->tid];
    void *xsave = (void *) xsave_areas[task->tid];

    task->arch.interrupt_stack = (uint64_t) kstack + STACK_SIZE;
    task->arch.syscall_stack = (uint64_t) syscall_stack_bottom + STACK_SIZE;
    task->arch.xsave = xsave;
    task->arch.gsbase = 0;
    task->arch.fsbase = 0;

    // Initialize the page table.
    task->arch.pml4 = into_paddr(pml4_tables[task->tid]);
    uint64_t *table = from_paddr(task->arch.pml4);
    memcpy(table, from_paddr((paddr_t) __kernel_pml4), PAGE_SIZE);

    // The kernel no longer access a virtual address around 0x0000_0000. Unmap
    // the area to catch bugs (especially NULL pointer dereferences in the
    // kernel).
    table[0] = 0;

    // Set up a temporary kernel stack frame.
    uint64_t *rsp = (uint64_t *) task->arch.interrupt_stack;

    // Push a IRET frame.
    *--rsp = USER_DS | USER_RPL;    // SS
    *--rsp = 0;                     // RSP
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
}

static void update_tss_iomap(struct task *task) {
    struct tss *tss = &ARCH_CPUVAR->tss;
    memset(tss->iomap, (CAPABLE(task, CAP_IO)) ? 0x00 : 0xff, TSS_IOMAP_SIZE);
}

void arch_task_switch(struct task *prev, struct task *next) {
    // Disable interrupts in case they're not yet disabled.
    asm_cli();
    // Update user GS base.
    asm_swapgs();
    prev->arch.gsbase = asm_rdgsbase();
    asm_wrgsbase(next->arch.gsbase);
    asm_swapgs();
    // Update user FS base.
    prev->arch.fsbase = asm_rdfsbase();
    asm_wrfsbase(next->arch.fsbase);
    // Switch the page table.
    asm_write_cr3(next->arch.pml4);
    // Enable ABI emulation if needed.
    ARCH_CPUVAR->abi_emu = (next->flags & TASK_ABI_EMU) ? 1 : 0;
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
