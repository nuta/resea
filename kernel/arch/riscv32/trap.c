#include "trap.h"
#include "asm.h"
#include "plic.h"
#include "uart.h"
#include <kernel/memory.h>
#include <kernel/printk.h>
#include <kernel/syscall.h>
#include <kernel/task.h>

void riscv32_handle_trap(struct risc32_trap_frame *frame) {
    uint32_t scause = read_scause();

    if (scause == 8) {
        frame->sepc += 4;
        uint32_t ret = handle_syscall(frame->a0, frame->a1, frame->a2,
                                      frame->a3, frame->a4, frame->a5);
        frame->a0 = ret;
    } else if (scause == 0x0000000c || scause == 0x0000000d || scause == 0xf) {
        handle_page_fault(read_stval(), frame->sepc, 0 /* FIXME: */);
    } else if (scause == 0x80000001) {
        write_sip(read_sip() & ~2);
        // TRACE("timer!");
        task_switch();
    } else if (scause == 0x80000009) {
        int irq = plic_pending();
        if (irq == UART0_IRQ) {
            handle_console_interrupt();
        }
        plic_ack(irq);
    } else {
        PANIC("unknown trap: scause=%p, stval=%p", read_scause(), read_stval());
    }
}
