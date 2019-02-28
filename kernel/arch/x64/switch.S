.att_syntax prefix
.set ARCH_THREAD_RIP, 0
.set ARCH_THREAD_RSP, 8
.set ARCH_THREAD_RSP0, 16
.set ARCH_THREAD_RFLAGS, 40
.set ARCH_THREAD_RBP, 48
.set ARCH_THREAD_R12, 56
.set ARCH_THREAD_R13, 64
.set ARCH_THREAD_R14, 72
.set ARCH_THREAD_R15, 80
.set ARCH_THREAD_RBX, 88
.set MSR_GS_BASE, 0xc0000101
.set MSR_KERNEL_GS_BASE, 0xc0000102

.globl x64_enter_userspace
x64_enter_userspace:
    /* Thread::new() pushed to the kernel stack (current one)
      `arg' and an IRET frame. Interrups are disabled. */

    pop %rdi /* arg */

    /* Sanitize registers except RDI (arg) to prevent information leak. */
    xor %rax, %rax
    xor %rbx, %rbx
    xor %rcx, %rcx
    xor %rdx, %rdx
    xor %rsi, %rsi
    xor %r8, %r8
    xor %r9, %r9
    xor %r10, %r10
    xor %r11, %r11

    iretq

/* Performs a context switch. RSI and RDI registers contains the previous
 * (e.g. the current) thread context and the next thread context respectively.
 *
 * Here we assume that the kernel is compiled with System V ABI. Refer
 * https://wiki.osdev.org/System_V_ABI for details.
 */
.globl x64_switch
x64_switch:
    movq %rdi, %r10
    movq %rsi, %r11

    /* Save callee-saved registers. */
    movabs $resume_point, %rax
    movq %rax, ARCH_THREAD_RIP(%r10)
    movq %rsp, ARCH_THREAD_RSP(%r10)
    movq %rbp, ARCH_THREAD_RBP(%r10)
    movq %rbx, ARCH_THREAD_RBX(%r10)
    movq %r12, ARCH_THREAD_R12(%r10)
    movq %r13, ARCH_THREAD_R13(%r10)
    movq %r14, ARCH_THREAD_R14(%r10)
    movq %r15, ARCH_THREAD_R15(%r10)

    /* Save RFLAGS. */
    pushfq
    popq %rax
    movq %rax, ARCH_THREAD_RFLAGS(%r10)

    /* Set RSI to GS base address. We modify MSR_GS_BASE rather
     * than MSR_KERNEL_GS_BASE directory since the current GS base
     * will be swapped by SWAPGS in the interrupt handler or enter_userspace.
     */
    push %rdx
    mov %r11, %rdx
    mov $MSR_GS_BASE, %ecx
    mov %edx, %eax
    shrq $32, %rdx
    wrmsr
    pop %rdx

    /* Restore RFLAGS. */
    movq ARCH_THREAD_RFLAGS(%r11), %rax
    pushq %rax
    popfq

    /* Restore callee-saved registers. */
    movq ARCH_THREAD_RSP(%r11), %rsp
    movq ARCH_THREAD_RBP(%r11), %rbp
    movq ARCH_THREAD_RBX(%r11), %rbx
    movq ARCH_THREAD_R12(%r11), %r12
    movq ARCH_THREAD_R13(%r11), %r13
    movq ARCH_THREAD_R14(%r11), %r14
    movq ARCH_THREAD_R15(%r11), %r15

    /* Resume next. */
    xor %rax, %rax
    xchg %bx,%bx
    jmpq *ARCH_THREAD_RIP(%r11)

resume_point:
    /* Resumed thread starts from here. Just return from arch_switch(). */
    ret
