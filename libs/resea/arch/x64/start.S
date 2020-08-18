.intel_syntax noprefix
.text

.global start
start:
    // Set the stack pointer.
    lea rsp, [rip + __stack_end]

    // Set RBP to 0 in order to stop backtracing here.
    mov rbp, 0

    // Initialize the user library and run main().
    call resea_init

    // Somehow task_exit returned!
.global halt
halt:
    int 3
    jmp halt

.section .bootelf_header, "ax"
.global __bootelf_header
__bootelf_header:
    .ascii "00BOOT\xe1\xff"
    .space 248
