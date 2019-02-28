#include <thread.h>
#include <x64/asm.h>
#include <x64/msr.h>
#include <x64/switch.h>
#include <x64/thread.h>

#define INITIAL_RFLAGS 0x202
#define KERNEL_STACK_SIZE PAGE_SIZE

// MSR_GS_BASE must points to both struct arch_thread` and `struct thread`.
STATIC_ASSERT(offsetof(struct thread, arch) == 0, "arch_thread must be first field");

void arch_thread_init(struct thread *thread,
                      vaddr_t start,
                      vaddr_t stack,
                      vaddr_t kernel_stack,
                      size_t arg) {

    // Temporarily use the kernel stack to pass `arg` and prepare an IRET frame.
    uint64_t *rsp = (uint64_t *) (kernel_stack + KERNEL_STACK_SIZE - sizeof(uint64_t) * 6);
    rsp[0] = arg;
    rsp[1] = start;
    rsp[2] = USER_CS64 | USER_RPL;
    rsp[3] = INITIAL_RFLAGS;
    rsp[4] = stack;
    rsp[5] = USER_DS | USER_RPL;

    thread->arch.rip = (uint64_t) x64_enter_userspace;
    thread->arch.rsp = (uint64_t) rsp;
    thread->arch.rsp0 = kernel_stack + KERNEL_STACK_SIZE;
    thread->arch.cr3 = thread->process->page_table.pml4;
    thread->arch.channel_table = 0;
    /* disable interrupts since we temporarily use the kernel stack. */
    thread->arch.rflags = 0x0002;
    thread->arch.rbp = 0;
    thread->arch.r12 = 0;
    thread->arch.r13 = 0;
    thread->arch.r14 = 0;
    thread->arch.rbx = 0;
}

void arch_set_current_thread(struct thread *thread) {
    asm_wrmsr(MSR_GS_BASE, (uint64_t) thread);
}

struct thread *arch_get_current_thread(void) {
    return (struct thread *) asm_rdmsr(MSR_GS_BASE);
}

/// Saves the context in `prev` and resume the `next`. context.
void arch_thread_switch(struct thread *prev, struct thread *next) {
    // Switch the page table.
    asm_write_cr3(next->arch.cr3);

    x64_switch(&prev->arch, &next->arch);
}
