.intel_syntax noprefix
.text

.global start
start:
    // Set the stack pointer.
    lea rsp, [rip + __stack_end]

    // Set RBP to 0 in order to stop backtracing here.
    mov rbp, 0

    // Initialize the user library.
    call resea_init

    // Call main().
    call main

    // main() has returned. Exit the current task.
    call task_exit

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
