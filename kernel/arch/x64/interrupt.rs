use super::apic::{self, APIC_TIMER_VECTOR};
use crate::timer::timer_interrupt_handler;

global_asm!(include_str!("handler.S"));

#[no_mangle]
pub extern "C" fn x64_handle_irq(irq: u8) {
//    crate::arch::printchar('@');

    apic::ack_interrupt();
    if irq == APIC_TIMER_VECTOR {
        timer_interrupt_handler();
    }
}
