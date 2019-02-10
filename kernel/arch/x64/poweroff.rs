use super::asm;

pub unsafe fn poweroff() -> ! {
    // Bochs
    asm::outw(0xb004, 0x2000);

    // QEMU
    asm::outb(0xf4, 0x00);

    panic!("failed to power off");
}