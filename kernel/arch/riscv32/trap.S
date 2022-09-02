.global riscv32_user_entry
riscv32_user_entry:
    lw a0, 0 * 4(sp) // ip
    lw a1, 1 * 4(sp) // satp
    j do_riscv32_user_entry

.global riscv32_trap_handler
riscv32_trap_handler:

// TODO: Only restore regs if it's user mode
    // Set ARCH_CPUVAR to sp and save sp in sscratch temporarily.
    csrrw sp, sscratch, sp

    sw tp, 0(sp) // Save sscratch
    mv tp, sp
    lw sp, 4(tp) // Load kernel sp from ARCH_CPUVAR.

    addi sp, sp, -4 * 33
    sw ra,  4 * 0(sp)
    sw gp,  4 * 1(sp)
    sw t0,  4 * 2(sp)
    sw t1,  4 * 3(sp)
    sw t2,  4 * 4(sp)
    sw s0,  4 * 5(sp)
    sw s1,  4 * 6(sp)
    sw a0,  4 * 7(sp)
    sw a1,  4 * 8(sp)
    sw a2,  4 * 9(sp)
    sw a3,  4 * 10(sp)
    sw a4,  4 * 11(sp)
    sw a5,  4 * 12(sp)
    sw a6,  4 * 13(sp)
    sw a7,  4 * 14(sp)
    sw s2,  4 * 15(sp)
    sw s3,  4 * 16(sp)
    sw s4,  4 * 17(sp)
    sw s5,  4 * 18(sp)
    sw s6,  4 * 19(sp)
    sw s7,  4 * 20(sp)
    sw s8,  4 * 21(sp)
    sw s9,  4 * 22(sp)
    sw s10, 4 * 23(sp)
    sw s11, 4 * 24(sp)
    sw t3,  4 * 25(sp)
    sw t4,  4 * 26(sp)
    sw t5,  4 * 27(sp)
    sw t6,  4 * 28(sp)

    // Restore sscratch and load temporarily saved user/kernel sp.
    csrrw a0, sscratch, tp
    sw a0, 4 * 29(sp) // sp

    lw a0, 0(tp)
    sw a0, 4 * 30(sp) // tp

    csrr a0, sepc
    sw a0, 4 * 31(sp)
    csrr a0, sstatus
    sw a0, 4 * 32(sp)

    mv a0, sp
    call riscv32_trap

    // Restore sepc / sstatus
    lw a0, 4 * 31(sp)
    csrw sepc, a0
    lw a0, 4 * 32(sp)
    csrw sstatus, a0

    lw ra,  4 * 0(sp)
    lw gp,  4 * 1(sp)
    lw t0,  4 * 2(sp)
    lw t1,  4 * 3(sp)
    lw t2,  4 * 4(sp)
    lw s0,  4 * 5(sp)
    lw s1,  4 * 6(sp)
    lw a0,  4 * 7(sp)
    lw a1,  4 * 8(sp)
    lw a2,  4 * 9(sp)
    lw a3,  4 * 10(sp)
    lw a4,  4 * 11(sp)
    lw a5,  4 * 12(sp)
    lw a6,  4 * 13(sp)
    lw a7,  4 * 14(sp)
    lw s2,  4 * 15(sp)
    lw s3,  4 * 16(sp)
    lw s4,  4 * 17(sp)
    lw s5,  4 * 18(sp)
    lw s6,  4 * 19(sp)
    lw s7,  4 * 20(sp)
    lw s8,  4 * 21(sp)
    lw s9,  4 * 22(sp)
    lw s10, 4 * 23(sp)
    lw s11, 4 * 24(sp)
    lw t3,  4 * 25(sp)
    lw t4,  4 * 26(sp)
    lw t5,  4 * 27(sp)
    lw t6,  4 * 28(sp)
    lw tp,  4 * 30(sp)

    addi sp, sp, 4 * 33
    lw sp,  -4 * (33 - 29)(sp)
    sret


.global riscv32_timer_handler
riscv32_timer_handler:
    csrrw tp, mscratch, tp

    sw a0, 4 * 2(tp)
    sw a1, 4 * 3(tp)

    lw a0, 4 * 5(tp) // CLINT_MTIME
    lw a0, (a0)
    lw a1, 4 * 6(tp) // interval
    add a0, a0, a1

    lw a1, 4 * 4(tp) // CLINT_MTIMECMP
    sw a0, (a1)

    li a1, 2
    csrw sip, a1

    lw a0, 4 * 2(tp)
    lw a1, 4 * 3(tp)
    csrrw tp, mscratch, tp
    mret
