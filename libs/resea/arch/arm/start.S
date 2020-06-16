.cpu cortex-m0
.thumb

.text
.global _start
.thumb_func
_start:
    // We don't need to set SP.

    // Initialize the user library.
    bl resea_init

    // Call main().
    bl main

    // main() has returned. Exit the current task.
    bl task_exit

.global halt
.thumb_func
halt:
    wfi
    b halt

.section .bootelf_header, "ax"
.global __bootelf_header
__bootelf_header:
    .ascii "00BOOT\xe1\xff"
    .space 248
