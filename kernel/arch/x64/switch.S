.intel_syntax noprefix
.set ARCH_THREAD_RIP, 0
.set ARCH_THREAD_RSP, 8
.set ARCH_THREAD_RSP0, 16
.set MSR_GS_BASE, 0xc0000101
.set MSR_KERNEL_GS_BASE, 0xc0000102

.globl x64_enter_userspace
x64_enter_userspace:
    /* Thread::new() pushed to the kernel stack (current one)
      `arg' and an IRET frame. Interrups are disabled. */

    pop rdi /* arg */

    /* Sanitize registers to prevent information leak. */
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rsi, rsi
    xor r8, r8
    xor r9, r9
    xor r10, r10
    xor r11, r11

    iretq

/* Performs a context switch. RSI and RDI registers contains the previous
 * (e.g. the current) thread context and the next thread context respectively.
 *
 * Here we assume that the kernel is compiled with System V ABI. Refer
 * https://wiki.osdev.org/System_V_ABI for details.
 */
.globl x64_switch
x64_switch:
    /* Save callee-saved registers. */
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    /* Save the context. */
    lea rax, [rip + resume_point]
    mov [rdi + ARCH_THREAD_RIP], rax
    mov [rdi + ARCH_THREAD_RSP], rsp

    /* Set next's GS base. */
    mov ecx, MSR_GS_BASE
    mov rdx, rsi
    mov eax, edx
    shr rdx, 32
    wrmsr

    /* Restore next's context. */
    mov rsp, [rsi + ARCH_THREAD_RSP]

    /* Restore callee-saved registers. */
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    /* Resume the next thread. */
    jmp [rsi + ARCH_THREAD_RIP]

resume_point:
    /* Resumed thread starts from here. Just return from arch_switch(). */
    ret
