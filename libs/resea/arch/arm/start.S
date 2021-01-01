.cpu cortex-m0
.thumb

.text
.global _start
.thumb_func
_start:
    // We don't need to set SP.

    // Initialize the user library and run main().
    bl resea_init

.global halt
.thumb_func
halt:
    wfi
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
