.text
.intel_syntax noprefix

.extern main, __bss_start, __bss_end
.global start
start:
    /* Clear .bss section. */
    lea rdi, __bss
    lea rsi, __bss_end
    mov rcx, rdi
    sub rcx, rsi
    xor rax, rax
    rep stosb

    /* Prepare stack. */
    lea rsp, __stack_end
    call main

    xchg bx,bx
    mov rax, 0x00000000deadbeed
    mov rax, [rax]
    ud2

__stack:
.space 8192
__stack_end:

.globl _Unwind_Resume
_Unwind_Resume:
    ud2

.include "libs/memcpy/memcpy_x64.S"