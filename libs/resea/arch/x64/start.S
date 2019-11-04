.intel_syntax noprefix
.text

.global start
start:
    // Clear .bss section
    mov al, 0x00
    lea rdi, [rip + __bss]
    lea rcx, [rip + __bss_end]
    sub rcx, rdi
    cld
    rep stosb

    mov rbp, 0
    call resea_main
    ud2
