.section ".boot", "ax"

.global boot
boot:
    la sp, __boot_stack
    j riscv32_setup
