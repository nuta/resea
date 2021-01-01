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
.global __bootelf_header, __bootinfo
__bootelf_header:
    .ascii "00BOOT\xe1\xff" // magic
    .space 16               // name
    .quad  0                // uint64_t entry
    .space 128              // mappings
__bootinfo:
    // struct bootinfo bootinfo;
    .space 512
