.intel_syntax noprefix
.section .text

.globl start
start:
    // Fill the .bss section with zeroes.
    lea rdi, [__bss]
    lea rcx, [__bss_end]
    sub rcx, rdi
    xor rax, rax
    cld
    rep stosb

    mov rbp, 0
    call main

    mov edi, eax
    call exit

    // Unreachable. Hopefully.
    ud2
