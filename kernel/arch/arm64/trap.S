//
//  Exception Vector
//
.balign 2048
.global exception_vector
exception_vector:
    b exception_common
    .balign 128
    b interrupt_common
    .balign 128
    b interrupt_common
    .balign 128
    b exception_common
    .balign 128

    b exception_common
    .balign 128
    b interrupt_common
    .balign 128
    b interrupt_common
    .balign 128
    b exception_common
    .balign 128

    b exception_common
    .balign 128
    b interrupt_common
    .balign 128
    b interrupt_common
    .balign 128
    b exception_common
    .balign 128

    b exception_common
    .balign 128
    b interrupt_common
    .balign 128
    b interrupt_common
    .balign 128
    b exception_common
    .balign 128

/// Saves the exception context *except* X0 onto the stack.
.macro save_context
    stp  x1,  x2,  [sp, #-16]!
    stp  x3,  x4,  [sp, #-16]!
    stp  x5,  x6,  [sp, #-16]!
    stp  x7,  x8,  [sp, #-16]!
    stp  x9,  x10, [sp, #-16]!
    stp  x11, x12, [sp, #-16]!
    stp  x13, x14, [sp, #-16]!
    stp  x15, x16, [sp, #-16]!
    stp  x17, x18, [sp, #-16]!
    stp  x19, x20, [sp, #-16]!
    stp  x21, x22, [sp, #-16]!
    stp  x23, x24, [sp, #-16]!
    stp  x25, x26, [sp, #-16]!
    stp  x27, x28, [sp, #-16]!
    stp  x29, x30, [sp, #-16]!

    // Because a page fault may occur in a usercopy function during the system
    // call handling (i.e. nested exception), we save the exception context on
    // the stack.
    mrs  x9, spsr_el1
    mrs  x10, elr_el1
    mrs  x11, sp_el0
    stp  x9,  x10, [sp, #-16]!
    stp  x11, x12, [sp, #-16]!
.endm

/// Restores the exception context *except* X0.
.macro restore_context
    ldp  x11, x12, [sp], #16
    ldp  x9,  x10, [sp], #16
    msr  spsr_el1, x9
    msr  elr_el1, x10
    msr  sp_el0, x11

    ldp  x29, x30, [sp], #16
    ldp  x27, x28, [sp], #16
    ldp  x25, x26, [sp], #16
    ldp  x23, x24, [sp], #16
    ldp  x21, x22, [sp], #16
    ldp  x19, x20, [sp], #16
    ldp  x17, x18, [sp], #16
    ldp  x15, x16, [sp], #16
    ldp  x13, x14, [sp], #16
    ldp  x11, x12, [sp], #16
    ldp  x9,  x10, [sp], #16
    ldp  x7,  x8,  [sp], #16
    ldp  x5,  x6,  [sp], #16
    ldp  x3,  x4,  [sp], #16
    ldp  x1,  x2,  [sp], #16
    eret
.endm

interrupt_common:
    save_context

    // Exceptions except SVC.
    stp  x0, xzr, [sp, #-16]!
    bl   arm64_handle_interrupt
    ldp  x0, xzr, [sp], #16

    restore_context

exception_common:
    save_context

    // Determine if the exception is a system call.
    mrs  x9, esr_el1
    lsr  x9, x9, #26
    cmp  x9, 0x15
    bne  1f

    // SVC handling. We don't preserve x0 as it contains the return value.
    bl   handle_syscall
    b    2f
1:
    // Exceptions except SVC.
    stp  x0, xzr, [sp, #-16]!
    bl   arm64_handle_exception
    ldp  x0, xzr, [sp], #16
2:
    restore_context
