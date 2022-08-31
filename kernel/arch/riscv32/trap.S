.global riscv32_user_entry
riscv32_user_entry:
    lw a0, 0 * 8(sp) // ip
    lw a1, 1 * 8(sp) // satp
    j do_riscv32_user_entry
