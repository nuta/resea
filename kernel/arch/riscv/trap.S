.global riscv_user_entry
riscv_user_entry:
    sfence.vma zero, zero
    csrw satp, a0
    sfence.vma zero, zero

    // TODO: Clear other registers
