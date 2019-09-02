#include <process.h>
#include <thread.h>
#include <x64/switch.h>
#include <x64/thread.h>
#include <x64/x64.h>

// Make sure that MSR_GS_BASE points to both 'struct arch_thread' and
// 'struct thread'.
STATIC_ASSERT(offsetof(struct thread, arch) == 0);

error_t arch_thread_init(struct thread *thread, vaddr_t start, vaddr_t stack,
                         vaddr_t kernel_stack, vaddr_t user_buffer,
                         bool is_kernel_thread) {
    // Set up a temporary kernel stack frame for x64_switch and
    // x64_start_kernel_thread/x64_enter_userspace.
    uint64_t *rsp = (uint64_t *) (kernel_stack + KERNEL_STACK_SIZE);
    if (is_kernel_thread) {
        *--rsp = start;
        *--rsp = (uint64_t) &x64_start_kernel_thread;
    } else {
        // IRET frame to be restored in x64_enter_userspace.
        *--rsp = USER_DS | USER_RPL;
        *--rsp = stack;
        *--rsp = 0x202; // RFLAGS (interrupts enabled).
        *--rsp = USER_CS64 | USER_RPL;
        *--rsp = start;
        *--rsp = (uint64_t) &x64_enter_userspace;
    }

    // Set up a temporary kernel stack frame for x64_switch.
    *--rsp = 0; // Initial RBP.
    *--rsp = 0; // Initial RBX.
    *--rsp = 0; // Initial R12.
    *--rsp = 0; // Initial R13.
    *--rsp = 0; // Initial R14.
    *--rsp = 0; // Initial R15.
    *--rsp = 0x02; // RFLAGS.

    vaddr_t xsave_area = (vaddr_t) kmalloc(&page_arena);
    if (!xsave_area) {
        return ERR_NO_MEMORY;
    }
    thread->arch.xsave_area = xsave_area;

    thread->arch.info = user_buffer;
    thread->arch.rsp = (uint64_t) rsp;
    thread->arch.kernel_stack = kernel_stack + KERNEL_STACK_SIZE;
    thread->arch.cr3 = thread->process->page_table.pml4;
    return OK;
}

void arch_set_current_thread(struct thread *thread) {
    asm_wrgsbase((uint64_t) thread);
}

/// Saves the context in `prev` and resume the `next`. context.
void arch_thread_switch(struct thread *prev, struct thread *next) {
    struct arch_thread *next_arch = &next->arch;

    // Disable interrupts in case they're not yet disabled.
    asm_cli();
    // Switch the page table.
    asm_write_cr3(next_arch->cr3);
    // Update GS bases.
    asm_wrgsbase((uint64_t) next);
    asm_swapgs();
    asm_wrgsbase((uint64_t) next_arch->info);
    asm_swapgs();
    // Update the kernel stack.
    CPUVAR->tss.rsp0 = next_arch->kernel_stack;
    // Set TS flag for lazy FPU context switching.
    if (CPUVAR->current_fpu_owner != next) {
        asm_write_cr0(asm_read_cr0() | CR0_TS);
    }
    // Restore registers (resume the thread).
    x64_switch(&prev->arch, next_arch);
}

/// Switches FPU states.
void x64_lazy_fpu_switch(void) {
    struct thread *current = CURRENT;

    // Save and restore the FPU states. `current_fpu_owner` is NULL at the very
    // first FPU context switch.
    if (CPUVAR->current_fpu_owner != NULL) {
        asm_xsave(CPUVAR->current_fpu_owner->arch.xsave_area);
    }
    asm_xrstor(current->arch.xsave_area);
    CPUVAR->current_fpu_owner = current;

    // Clear the TS flag in case CPU doesn't do that.
    asm_write_cr0(asm_read_cr0() & ~CR0_TS);
}
