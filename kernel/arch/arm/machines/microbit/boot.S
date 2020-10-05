.cpu cortex-m0
.thumb

.section .vector
.global _start
.thumb_func
_start:
.word   __boot_stack_top
.word   arm_init           // Reset (boot)
.word   nmi_handler        // NMI
.word   hard_reset_handler // Hard Fault
.word   unexpected_handler // (reserved)
.word   unexpected_handler // (reserved)
.word   unexpected_handler // (reserved)
.word   unexpected_handler // (reserved)
.word   unexpected_handler // (reserved)
.word   unexpected_handler // (reserved)
.word   unexpected_handler // (reserved)
.word   syscall_handler    // SVC
.word   unexpected_handler // (reserved)
.word   unexpected_handler // (reserved)
.word   unexpected_handler // PendSV
.word   handle_timer_irq   // SysTick
.word   irq_handler        // IRQ0
.word   irq_handler        // IRQ1
.word   irq_handler        // IRQ2
.word   irq_handler        // IR3Q
.word   irq_handler        // IRQ4
.word   irq_handler        // IRQ5
.word   irq_handler        // IRQ6
.word   irq_handler        // IRQ7
.word   irq_handler        // IRQ8
.word   irq_handler        // IRQ9
.word   irq_handler        // IRQ10
.word   irq_handler        // IRQ11
.word   irq_handler        // IRQ12
.word   irq_handler        // IRQ13
.word   irq_handler        // IRQ14
.word   irq_handler        // IRQ15

.text

.thumb_func
syscall_handler:
    push {lr}
    // Copy arguments onto the stack.
    push {r5}
    push {r4}

    //  Current Stack Layout
    //  ---------+----------
    //   sp + 36 | xPSR
    //   sp + 32 | PC
    //   sp + 28 | LR
    //   sp + 24 | r12
    //   sp + 20 | r3
    //   sp + 16 | r2
    //   sp + 12 | r1
    //   sp + 8  | r0
    //   sp + 4  | r5 (pushed in this handler)
    //   sp + 0  | r4 (pushed in this handler)
    bl handle_syscall

    mov r6, r0 // return value
    pop {r4}
    pop {r5}
    pop {pc}

.section .bootelf, "ax"
.global __bootelf
__bootelf:
    .incbin BOOTELF_PATH
