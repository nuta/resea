.intel_syntax noprefix
.set ARCH_THREAD_RSP, 0

.globl x64_enter_userspace
x64_enter_userspace:
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
