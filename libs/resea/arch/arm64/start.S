.text
.global _start
_start:
    // Set the stack pointer.
    ldr  x0, =__stack_end
    mov  sp, x0

    // Initialize the user library and run main().
    bl resea_init

.global halt
halt:
    b halt

.section .bootelf_header, "ax"
.global __bootelf_header
__bootelf_header:
    .ascii "00BOOT\xe1\xff"
    .space 248
