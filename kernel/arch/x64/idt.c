#include <arch.h>
#include <x64/cpu.h>
#include <x64/idt.h>
#include <x64/tss.h>
#include <x64/handler.h>
#include <x64/asm.h>

static void set_idt_entry(int i, uint64_t handler, uint8_t ist) {
    struct idt_entry *entry = &CPUVAR->idt[i];
    entry->offset1 = handler & 0xffff;
    entry->seg = KERNEL_CS;
    entry->ist = ist;
    entry->info = IDT_INT_HANDLER;
    entry->offset2 = (handler >> 16) & 0xffff;
    entry->offset3 = (handler >> 32) & 0xffffffff;
    entry->reserved = 0;
}

static void init_exception_handlers(void) {
    set_idt_entry(0, (uint64_t) exception_handler0, EXP_HANDLER_IST);
    set_idt_entry(1, (uint64_t) exception_handler1, EXP_HANDLER_IST);
    set_idt_entry(2, (uint64_t) exception_handler2, EXP_HANDLER_IST);
    set_idt_entry(3, (uint64_t) exception_handler3, EXP_HANDLER_IST);
    set_idt_entry(4, (uint64_t) exception_handler4, EXP_HANDLER_IST);
    set_idt_entry(5, (uint64_t) exception_handler5, EXP_HANDLER_IST);
    set_idt_entry(6, (uint64_t) exception_handler6, EXP_HANDLER_IST);
    set_idt_entry(7, (uint64_t) exception_handler7, EXP_HANDLER_IST);
    set_idt_entry(8, (uint64_t) exception_handler8, EXP_HANDLER_IST);
    set_idt_entry(9, (uint64_t) exception_handler9, EXP_HANDLER_IST);
    set_idt_entry(10, (uint64_t) exception_handler10, EXP_HANDLER_IST);
    set_idt_entry(11, (uint64_t) exception_handler11, EXP_HANDLER_IST);
    set_idt_entry(12, (uint64_t) exception_handler12, EXP_HANDLER_IST);
    set_idt_entry(13, (uint64_t) exception_handler13, EXP_HANDLER_IST);
    set_idt_entry(14, (uint64_t) exception_handler14, EXP_HANDLER_IST);
    set_idt_entry(15, (uint64_t) exception_handler15, EXP_HANDLER_IST);
    set_idt_entry(16, (uint64_t) exception_handler16, EXP_HANDLER_IST);
    set_idt_entry(17, (uint64_t) exception_handler17, EXP_HANDLER_IST);
    set_idt_entry(18, (uint64_t) exception_handler18, EXP_HANDLER_IST);
    set_idt_entry(19, (uint64_t) exception_handler19, EXP_HANDLER_IST);
    set_idt_entry(20, (uint64_t) exception_handler20, EXP_HANDLER_IST);
}

static void init_interrupt_handlers() {
    uint64_t handler = (uint64_t) interrupt_handler32;
    int handler_size = (uint64_t) interrupt_handler33 - (uint64_t) interrupt_handler32;
    for (int i = 32; i <= 255; i++) {
        set_idt_entry(i, handler, INTR_HANDLER_IST);
        handler += handler_size;
    }
}

void x64_idt_init(void) {
    init_exception_handlers();
    init_interrupt_handlers();

    CPUVAR->idtr.laddr = (uint64_t) &CPUVAR->idt;
    CPUVAR->idtr.len = (sizeof(struct idt_entry) * IDT_DESC_NUM) + 1;
    asm_lidt((uint64_t) &CPUVAR->idtr);
}
