.text
.global _start
_start:
    // Set the stack pointer.
    ldr  x0, =__stack
    mov  sp, x0

    // Initialize the user library.
    bl resea_init

    // Call main().
    bl main

    // main() has returned. Exit the current task.
    bl task_exit

.global halt
halt:
    b halt

.section .bootelf_header, "ax"
.global __bootelf_header
__bootelf_header:
    .ascii "00BOOT\xe1\xff"
    .space 248
