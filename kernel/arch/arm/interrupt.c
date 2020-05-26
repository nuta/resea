#include <types.h>
#include <task.h>
#include <printk.h>

void nmi_handler(void) {
    PANIC("hard reset");
}

void hard_reset_handler(void) {
    PANIC("hard reset");
}
void unexpected_handler(void) {
    PANIC("unexpected exception");
}

void irq_handler(void) {
    uint32_t ipsr;
    __asm__ __volatile__("mrs %0, ipsr" : "=r"(ipsr));
    unsigned irq = ipsr & 0x1f;
    handle_irq(irq);
}

void arch_enable_irq(unsigned irq){
    // TODO:
}

void arch_disable_irq(unsigned irq) {
    // TODO:
}
