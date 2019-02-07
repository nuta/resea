use super::asm;

const IOPORT_SERIAL: u16 = 0x3f8;
const IER: u16 = 1;
const DLL: u16 = 0;
const DLH: u16 = 1;
const LCR: u16 = 3;
const FCR: u16 = 2;
const LSR: u16 = 5;
const TX_READY: u8 = 0x20;

pub unsafe fn putchar(ch: char) {
    loop {
        if (asm::inb(IOPORT_SERIAL + LSR) & TX_READY) != 0 {
            break;
        }
    }

    asm::outb(IOPORT_SERIAL, ch as u8);
}

pub unsafe fn init() {
    let divisor: u16 = 12; // 115200 / 9600

    asm::outb(IOPORT_SERIAL + IER, 0x00); // Disable interrupts
    asm::outb(IOPORT_SERIAL + DLL, divisor as u8);
    asm::outb(IOPORT_SERIAL + DLH, (divisor >> 8) as u8);
    asm::outb(IOPORT_SERIAL + LCR, 0x03); // 8n1
    asm::outb(IOPORT_SERIAL + FCR, 0x00); // No FIFO
}
