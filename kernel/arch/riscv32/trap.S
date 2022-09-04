.align 4
.global riscv32_user_entry
riscv32_user_entry:
    lw a0, 0 * 4(sp) // ip
    lw a1, 1 * 4(sp) // satp
    j do_riscv32_user_entry

.align 4
.global riscv32_trap_handler
riscv32_trap_handler:
    csrrw tp, sscratch, tp
    sw a0, 0(tp) // Save a0

    csrr a0, sstatus
    andi a0, a0, 1 << 8 // SPP
    beqz a0, 1f

    // From s-mode
.align 4
2:
    j 2b
    mv a0, sp // Keep using the current kernel stack
    j trap_common

.align 4
1:
    // From u-mode
    lw a0, 4(tp) // Load kernel sp from ARCH_CPUVAR.
    j trap_common

.align 4
trap_common:
    addi a0, a0, -4 * 33
    sw ra,  4 * 0(a0)
    sw gp,  4 * 1(a0)
    sw t0,  4 * 2(a0)
    sw t1,  4 * 3(a0)
    sw t2,  4 * 4(a0)
    sw s0,  4 * 5(a0)
    sw s1,  4 * 6(a0)
    sw a1,  4 * 8(a0)
    sw a2,  4 * 9(a0)
    sw a3,  4 * 10(a0)
    sw a4,  4 * 11(a0)
    sw a5,  4 * 12(a0)
    sw a6,  4 * 13(a0)
    sw a7,  4 * 14(a0)
    sw s2,  4 * 15(a0)
    sw s3,  4 * 16(a0)
    sw s4,  4 * 17(a0)
    sw s5,  4 * 18(a0)
    sw s6,  4 * 19(a0)
    sw s7,  4 * 20(a0)
    sw s8,  4 * 21(a0)
    sw s9,  4 * 22(a0)
    sw s10, 4 * 23(a0)
    sw s11, 4 * 24(a0)
    sw t3,  4 * 25(a0)
    sw t4,  4 * 26(a0)
    sw t5,  4 * 27(a0)
    sw t6,  4 * 28(a0)
    sw sp,  4 * 29(a0)

    // Restore sscratch and save saved tp
    mv sp, a0
    csrrw a0, sscratch, tp
    sw a0, 4 * 30(sp) // tp

    // Save saved a0
    lw a0, 0(tp)
    sw a0, 4 * 7(sp) // a0

    csrr a0, sepc
    sw a0, 4 * 31(sp) // sepc
    csrr a0, sstatus
    sw a0, 4 * 32(sp) // sstatus

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


.align 4
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
