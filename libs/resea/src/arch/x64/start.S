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
    mov rsp, 0x03100000
    call main

    xchg bx,bx
    mov rax, 0x00000000deadbeed
    mov rax, [rax]
    ud2

.globl _Unwind_Resume
_Unwind_Resume:
    ud2

.include "libs/resea/src/arch/x64/memcpy.S"
