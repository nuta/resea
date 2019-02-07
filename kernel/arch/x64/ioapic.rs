use super::asm;
use crate::arch::utils::PhyPtr;
use crate::utils::LazyStatic;

const REG_IOAPICVER: u32 = 0x01;
#[allow(dead_code)]
const VECTOR_IRQ_BASE: u32 = 32;

macro_rules! ioredtbl_low {
    ($n:expr) => {
        (0x10 + ($n * 2)).into()
    };
}

macro_rules! ioredtbl_high {
    ($n:expr) => {
        (0x10 + ($n * 2) + 1).into()
    };
}

static mut IOAPIC_ADDR: LazyStatic<PhyPtr<u32>> = LazyStatic::new();

#[inline]
unsafe fn ioapic_read(reg: u32) -> u32 {
    IOAPIC_ADDR.write(reg);
    IOAPIC_ADDR.add(0x10).read()
}

#[inline]
unsafe fn ioapic_write(reg: u32, value: u32) {
    IOAPIC_ADDR.write(reg);
    IOAPIC_ADDR.add(0x10).write(value);
}

pub unsafe fn init(addr: u64) {
    IOAPIC_ADDR.init(PhyPtr::new(addr as usize));

    // symmetric I/O mode
    asm::outb(0x22, 0x70);
    asm::outb(0x23, 0x01);

    // Get the maxinum number of entries in IOREDTBL.
    let max = (ioapic_read(REG_IOAPICVER) >> 16) + 1;

    // Disable all hardware interrupts for now.
    for i in 0..max {
        ioapic_write(ioredtbl_high!(i), 0);
        ioapic_write(ioredtbl_low!(i), 1 << 16 /* masked */);
    }
}
