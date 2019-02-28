#include <timer.h>
#include <x64/apic.h>

void x64_handle_irq(uint8_t irq) {

    x64_ack_interrupt();

    if (irq == APIC_TIMER_VECTOR) {
        timer_interrupt_handler();
    }
}
