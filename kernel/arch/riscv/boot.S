.section ".boot", "ax"

.global boot
boot:
    la sp, __boot_stack
    call riscv_setup

// `api_boot` won't return but in case it does.
halt:
    j halt
