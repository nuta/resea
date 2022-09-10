#include "trap.h"
#include "asm.h"
#include "debug.h"
#include "plic.h"
#include "uart.h"
#include "usercopy.h"
#include "vm.h"
#include <kernel/memory.h>
#include <kernel/printk.h>
#include <kernel/syscall.h>
#include <kernel/task.h>

static void handle_syscall_trap(struct riscv32_trap_frame *frame) {
    frame->sepc += 4;
    uint32_t ret = handle_syscall(frame->a0, frame->a1, frame->a2, frame->a3,
                                  frame->a4, frame->a5);
    frame->a0 = ret;
}

static void handle_soft_interrupt_trap(void) {
    write_sip(read_sip() & ~2);
    // TRACE("timer!");
    task_switch();
}

static void handle_external_interrupt_trap(void) {
    int irq = plic_pending();
    if (irq == UART0_IRQ) {
        handle_console_interrupt();
    }
    plic_ack(irq);
}

static void handle_page_fault_trap(struct riscv32_trap_frame *frame) {
    uint32_t reason;
    switch (read_scause()) {
        case SCAUSE_INST_PAGE_FAULT:
            reason = PAGE_FAULT_EXEC;
            break;
        case SCAUSE_LOAD_PAGE_FAULT:
            reason = PAGE_FAULT_READ;
            break;
        case SCAUSE_STORE_PAGE_FAULT:
            reason = PAGE_FAULT_WRITE;
            break;
        default:
            UNREACHABLE();
    }

    if ((frame->sstatus & SSTATUS_SPP) == 0) {
        reason = PAGE_FAULT_USER;
    }

    vaddr_t vaddr = read_stval();
    reason |= riscv32_is_mapped(read_satp(), vaddr) ? PAGE_FAULT_PRESENT : 0;

    uint32_t sepc = read_sepc();
    if (sepc == (uint32_t) usercopy_point1
        || sepc == (uint32_t) usercopy_point2) {
        // fault in usercopy
        DBG("fault in usercopy");
        return;
    }

    handle_page_fault(vaddr, frame->sepc, reason);
}

void riscv32_handle_trap(struct riscv32_trap_frame *frame) {
    stack_check();
    uint32_t scause = read_scause();
    switch (scause) {
        case SCAUSE_S_SOFT_INTR:
            handle_soft_interrupt_trap();
            break;
        case SCAUSE_S_EXT_INTR:
            handle_external_interrupt_trap();
            break;
        case SCAUSE_ENV_CALL:
            handle_syscall_trap(frame);
            break;
        case SCAUSE_INST_PAGE_FAULT:
        case SCAUSE_LOAD_PAGE_FAULT:
        case SCAUSE_STORE_PAGE_FAULT:
            handle_page_fault_trap(frame);
            break;
        default:
            PANIC("unknown trap: scause=%p, stval=%p", read_scause(),
                  read_stval());
    }

    stack_check();
}
