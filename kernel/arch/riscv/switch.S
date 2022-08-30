/// Saves and restore callee-saved registers and stack pointers.
///
/// void riscv_task_switch(uint32_t *prev_sp, uint32_t *next_sp);
.global riscv_task_switch
riscv_task_switch:
    addi sp, sp, -13 * 8

    sw ra,  0  * 8(sp)
    sw s0,  1  * 8(sp)
    sw s1,  2  * 8(sp)
    sw s2,  3  * 8(sp)
    sw s3,  4  * 8(sp)
    sw s4,  5  * 8(sp)
    sw s5,  6  * 8(sp)
    sw s6,  7  * 8(sp)
    sw s7,  8  * 8(sp)
    sw s8,  9  * 8(sp)
    sw s9,  10 * 8(sp)
    sw s10, 11 * 8(sp)
    sw s11, 12 * 8(sp)

    sw sp, (a0)
    lw sp, (a1)

    lw ra,  0  * 8(sp)
    lw s0,  1  * 8(sp)
    lw s1,  2  * 8(sp)
    lw s2,  3  * 8(sp)
    lw s3,  4  * 8(sp)
    lw s4,  5  * 8(sp)
    lw s5,  6  * 8(sp)
    lw s6,  7  * 8(sp)
    lw s7,  8  * 8(sp)
    lw s8,  9  * 8(sp)
    lw s9,  10 * 8(sp)
    lw s10, 11 * 8(sp)
    lw s11, 12 * 8(sp)

    addi sp, sp, 13 * 8
    ret
