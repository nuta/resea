.section ".boot", "ax"

.global boot
boot:
    mv ra, zero
    mv fp, zero
    la sp, __boot_stack
    jal riscv32_setup_m_mode

.text
.global boot_s_mode
boot_s_mode:
    mv ra, zero
    mv fp, zero
    la sp, __boot_stack
    jal riscv32_setup_s_mode
