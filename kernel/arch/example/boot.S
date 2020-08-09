.intel_syntax noprefix
.text
.code64

// The entry point jumped from the bootloader.
.global boot
boot:
    call x64_init

halt:
    cli
    hlt
    jmp halt

.rodata
.align 4096
.global __bootelf
__bootelf:
    .incbin BOOTELF_PATH
.align 4096
