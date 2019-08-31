#include <process.h>
#include <thread.h>
#include <x64/switch.h>
#include <x64/x64.h>

// Make sure that MSR_GS_BASE points to both 'struct arch_thread' and
// 'struct thread'.
STATIC_ASSERT(offsetof(struct thread, arch) == 0);

void arch_thread_init(struct thread *thread, vaddr_t start, vaddr_t stack,
                      vaddr_t kernel_stack, vaddr_t user_buffer,
                      bool is_kernel_thread) {

    uint64_t *rsp;
    if (is_kernel_thread) {
        rsp = (uint64_t *) (kernel_stack + KERNEL_STACK_SIZE);
        *--rsp = start;
        *--rsp = (uint64_t) &x64_start_kernel_thread; // Initial return address.
        *--rsp = 0; // Initial RBP.
        *--rsp = 0; // Initial RBX.
        *--rsp = 0; // Initial R12.
        *--rsp = 0; // Initial R13.
        *--rsp = 0; // Initial R14.
        *--rsp = 0; // Initial R15.
        *--rsp = 0x02;
    } else {
        // Temporarily use the kernel stack for x64_enter_userspace.
        int num_regs = 13; /* iret frame, rflags, and callee-saved regs */
        int temp_stack_len = sizeof(uint64_t) * num_regs;
        rsp =
            (uint64_t *) (kernel_stack + KERNEL_STACK_SIZE - temp_stack_len);
        // Temporary thread state to be restored in x64_switch.
        rsp[0] = 0x2; // RFLAGS.
        rsp[1] = 0; // Initial RBP.
        rsp[2] = 0; // Initial RBX.
        rsp[3] = 0; // Initial R12.
        rsp[4] = 0; // Initial R13.
        rsp[5] = 0; // Initial R14.
        rsp[6] = 0; // Initial R15.
        rsp[7] = (uint64_t) &x64_enter_userspace; // Initial return address.

        // IRET frame to be restored in x64_enter_userspace.
        rsp[8] = start;
        rsp[9] = USER_CS64 | USER_RPL;
        rsp[10] = 0x202; // RFLAGS (interrupts enabled).
        rsp[11] = stack;
        rsp[12] = USER_DS | USER_RPL;
    }

    thread->arch.info = user_buffer;
    thread->arch.rsp = (uint64_t) rsp;
    thread->arch.kernel_stack = kernel_stack + KERNEL_STACK_SIZE;
    thread->arch.cr3 = thread->process->page_table.pml4;
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
    // Restore registers (resume the thread).
    x64_switch(&prev->arch, next_arch);
}
