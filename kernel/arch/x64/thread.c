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
                      vaddr_t user_buffer,
                      size_t arg) {

    // Temporarily use the kernel stack for x64_enter_userspace.
    int num_regs = 12; /* arg, iret frame, and callee-saved regs */
    int temp_stack_len = sizeof(uint64_t) * num_regs;
    uint64_t *rsp = (uint64_t *) (kernel_stack + KERNEL_STACK_SIZE - temp_stack_len);
    rsp[5]  = 0; /* initial R15 */
    rsp[4]  = 0; /* initial R14 */
    rsp[3]  = 0; /* initial R13 */
    rsp[2]  = 0; /* initial R12 */
    rsp[1]  = 0; /* initial RBX */
    rsp[0]  = 0; /* initial RBP */
    rsp[6]  = arg;
    rsp[7]  = start;
    rsp[8]  = USER_CS64 | USER_RPL;
    rsp[9]  = INITIAL_RFLAGS;
    rsp[10] = stack;
    rsp[11] = USER_DS | USER_RPL;

    thread->arch.rip    = (uint64_t) x64_enter_userspace;
    thread->arch.rsp    = (uint64_t) rsp;
    thread->arch.rsp0   = kernel_stack + KERNEL_STACK_SIZE;
    thread->arch.cr3    = thread->process->page_table.pml4;
    thread->arch.buffer = (uint64_t) thread->buffer;
    thread->arch.info   = user_buffer;
    thread->info->user_buffer = user_buffer + PAGE_SIZE;
}

void arch_set_current_thread(struct thread *thread) {
    asm_wrmsr(MSR_GS_BASE, (uint64_t) thread);
}

struct thread *arch_get_current_thread(void) {
    return (struct thread *) asm_rdmsr(MSR_GS_BASE);
}

/// Saves the context in `prev` and resume the `next`. context.
void arch_thread_switch(struct thread *prev, struct thread *next) {
    x64_switch(&prev->arch, &next->arch);
}
