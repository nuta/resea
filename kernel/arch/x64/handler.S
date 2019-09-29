#include <x64/handler.h>
#define ARCH_THREAD_RSP           0
#define ARCH_THREAD_KERNEL_STACK  8
#define ARCH_RFLAGS_ORMASK        40

.intel_syntax noprefix

.data
.align 8
giant_lock:
    .long 0

/// Locks the ginant lock. Note that it uses stack.
.macro GIANT_LOCK
\@1:
    push rax
    mov  eax, 1
    lock xchg [rip + giant_lock], eax
    test eax, eax
    jnz  \@1b
    pop  rax
.endm

/// Unlocks the ginant lock. Note that it uses stack.
.macro GIANT_UNLOCK
    push rax
    xor  eax, eax
    mov  [rip + giant_lock], eax
    pop  rax
.endm

.text

.extern syscall_handler
.globl x64_syscall_handler
x64_syscall_handler:
    //
    //  Current register values:
    //
    //   RAX: syscall number
    //   RDI: 1st argument
    //   RSI: 2nd argument
    //   RDX: (unused)
    //   RCX: user RIP (set by SYSCALL)
    //   R8:  (unused)
    //   R9:  (unused)
    //   R10: (unused)
    //   R11: user RFLAGS (set by SYSCALL)
    //   The others: user values (need to be preserved)
    //

    // Interrupts are automatically disabled by MSR_FMASK. If interrupts are
    // enabled, an interrupt may occurs before SWAPGS and as a result, an
    // interrupt handler mistakenly use the user's GS base because the interrupt
    // has occurred in the kernel mode and SWAPGS won't be performed.
    //
    // Note that we can't re-enable interrupts until we go back into the user
    // mode or resume the next thread: the (per-thread) kernel stack is shared
    // with exception/interrupt handlers!
    swapgs

    // TODO: Add LFENCE to mitigate speculative side-channel attacks.
    //       (CVE-2019-1125)

    // Switch to the kernel stack. Use gs:[ARCH_THREAD_RSP] temporarily to
    // save the user RSP.
    mov gs:[ARCH_THREAD_RSP], rsp
    mov rsp, gs:[ARCH_THREAD_KERNEL_STACK]

    // Save SYSRET context and callee-saved registers onto the kernel stack.
    push gs:[ARCH_THREAD_RSP] // User RSP.
    push r11 // User RFLAGS.
    push rcx // User RIP.
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    // Set RDX to the 1st argument for syscall_handler().
    mov rdx, rax

    GIANT_LOCK
    call syscall_handler
    GIANT_UNLOCK

    // Restore callee-saved registers.
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    pop rcx
    pop r11
    pop rsp

    // Sanitize registers to prevent information leak. Note that some scratch
    // registers holds values for SYSRETQ and the user:
    //
    //   RAX: The return value.
    //   RCX: User RIP.
    //   R11: User RFLAGS.
    //
    xor rdx, rdx
    xor rdi, rdi
    xor rsi, rsi
    xor r8, r8
    xor r9, r9
    xor r10, r10

    // Disable interrupts temporarily since SWAPGS and SYSRETQ are not a
    // atomic operation. If an interrupt occurred between SWAPGS and SYSRETQ
    // (thus the GS base points to the user one), the interrupt handler won't
    // perform SWAPGS since the interrupt has occurred in kernel mode.
    cli
    swapgs
    sysretq

//
//  Interrupt/exception handlers
//
.align INTERRUPT_HANDLER_SIZE
.globl interrupt_handlers
interrupt_handlers:
.set i, 0
.rept 256
.set handler_start, .
// Exceptions with error code.
.if i == 8 || 10 <= i && i <= 14 || i == 17
    .align INTERRUPT_HANDLER_SIZE
    cli
    push i
    jmp handle_interrupt
    .align INTERRUPT_HANDLER_SIZE
// Interrupts and exceptions without error code.
.else
    .align INTERRUPT_HANDLER_SIZE
    cli
    push 0 // Dummy value as error code.
    push i
    jmp handle_interrupt
    .align INTERRUPT_HANDLER_SIZE
.endif

// Increment the counter.
.set i, i + 1
.endr

.extern x64_handle_interrupt
handle_interrupt:
    //
    //  The current stack frame:
    //
    //            +--------------------+
    //     48     |        SS          |
    //            +--------------------+
    //     40     |        RSP         |
    //            +--------------------+
    //     32     |       RFLAGS       |
    //            +--------------------+
    //     24     |        CS          |
    //            +--------------------+
    //     16     |        RIP         |
    //            +--------------------+
    //      8     |     Error code     |
    //            +--------------------+
    //      0     |     IRQ Number     | <- RSP
    //            +--------------------+
    //

    // Check CS register in the IRET frame to determine if the interrupt has
    // occurred in user mode.
    test qword ptr [rsp + 24], 3
    jz 1f
    swapgs
    GIANT_LOCK
1:
    // Save RDI and set the IRQ number to RDI at once.
    xchg rdi, [rsp]

    /* Save registers except RDI (we have already saved it above). */
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    mov rsi, rsp

    call x64_handle_interrupt
    GIANT_UNLOCK

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    pop rdi

    // Skip error code.
    add rsp, 8

    /* Check CS register in the IRET frame to determine whether the exception
     * occur in the userspace. If so, do swapgs. */
    test qword ptr [rsp + 8], 3
    jz 1f

    /* Update RFLAGS. */
    push rax
    mov rax, gs:[ARCH_RFLAGS_ORMASK]
    or [rsp + 24], rax
    pop rax

    cli
    swapgs
1:
    iretq

/// The entry point of kernel threads.
.globl x64_start_kernel_thread
x64_start_kernel_thread:
    // Clear RBP to stop backtrace here.
    mov rbp, 0

    // Call the kernel thread. Note that a kernel thread NEVER returns.
    pop rax
    call rax

    // Invoke an exception to notice that the kernel thread returned.
    ud2

.globl x64_start_user_thread
x64_start_user_thread:
    GIANT_UNLOCK

    /* Sanitize registers to prevent information leak. */
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rdi, rdi
    xor rsi, rsi
    xor r8, r8
    xor r9, r9
    xor r10, r10
    xor r11, r11

    cli
    swapgs
    iretq

.globl x64_switch
x64_switch:
    /* Save callee-saved registers. */
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    pushfq

    /* Save and restore the stack pointer. */
    mov [rdi + ARCH_THREAD_RSP], rsp
    mov rsp, [rsi + ARCH_THREAD_RSP]

    /* Restore callee-saved registers. */
    popfq
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    /* Resume the next thread. */
    ret
