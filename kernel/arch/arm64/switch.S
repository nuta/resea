.text

// void arm64_task_switch(vaddr_t *prev_sp, vaddr_t next_sp);
.global arm64_task_switch
arm64_task_switch:
    // Save callee-saved registers.
    stp  x20, x19, [sp, #-16]!
    stp  x22, x21, [sp, #-16]!
    stp  x24, x23, [sp, #-16]!
    stp  x26, x25, [sp, #-16]!
    stp  x28, x27, [sp, #-16]!
    stp  x30, x29, [sp, #-16]!

    // Switch the stack pointer.
    mov  x9, sp
    str  x9, [x0]
    mov  sp, x1

    // Restore callee-saved registers.
    ldp  x30, x29, [sp], #16
    ldp  x28, x27, [sp], #16
    ldp  x26, x25, [sp], #16
    ldp  x24, x23, [sp], #16
    ldp  x22, x21, [sp], #16
    ldp  x20, x19, [sp], #16
    ret

.global arm64_start_task
arm64_start_task:
    bl stack_set_canary

    ldr  x0, =0 /* AArch64, EL0t */
    msr  spsr_el1, x0

    ldr  x0, [sp]
    msr  elr_el1, x0
    eret
