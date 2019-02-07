.section .startup, "ax"
.intel_syntax noprefix

.globl initfs_header
initfs_header:

inifs_jump_code:
    jmp start
    .align 16
inifs_version:
    .long 0
inifs_size:
    .long 0
inifs_num_files:
    .long 0

.space 256

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
    mov rsp, 0x02a00000 // FIXME:
    call main

    xchg bx,bx
    mov rax, 0x00000000deadbeed
    mov rax, [rax]
    ud2

.globl _Unwind_Resume
_Unwind_Resume:
    ud2

.include "libs/memcpy/memcpy_x64.S"