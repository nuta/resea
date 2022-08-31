.global riscv32_user_entry
riscv32_user_entry:
    lw a0, 0 * 4(sp) // ip
    lw a1, 1 * 4(sp) // satp
    j do_riscv32_user_entry
