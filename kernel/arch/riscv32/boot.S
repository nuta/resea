.section ".boot", "ax"

.global boot
boot:
    mv fp, zero
    la sp, __boot_stack
    j riscv32_setup
