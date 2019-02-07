#![allow(unused)]
use super::{asm, TICK_HZ};
use crate::arch::utils::PhyPtr;

const MSR_APIC_BASE: u32 = 0x0000_001b;
const APIC_REG_ID: PhyPtr<u32> = PhyPtr::new(0xfee0_0020);
const APIC_REG_VERSION: PhyPtr<u32> = PhyPtr::new(0xfee0_0030);
const APIC_REG_TPR: PhyPtr<u32> = PhyPtr::new(0xfee0_0080);
const APIC_REG_EOI: PhyPtr<u32> = PhyPtr::new(0xfee0_00b0);
const APIC_REG_LOGICAL_DEST: PhyPtr<u32> = PhyPtr::new(0xfee0_00d0);
const APIC_REG_DEST_FORMAT: PhyPtr<u32> = PhyPtr::new(0xfee0_00e0);
const APIC_REG_SPURIOUS_INT: PhyPtr<u32> = PhyPtr::new(0xfee0_00f0);
const APIC_REG_ICR_LOW: PhyPtr<u32> = PhyPtr::new(0xfee0_0300);
const APIC_REG_ICR_HIGH: PhyPtr<u32> = PhyPtr::new(0xfee0_0310);
const APIC_REG_LVT_TIMER: PhyPtr<u32> = PhyPtr::new(0xfee0_0320);
const APIC_REG_LINT0: PhyPtr<u32> = PhyPtr::new(0xfee0_0350);
const APIC_REG_LINT1: PhyPtr<u32> = PhyPtr::new(0xfee0_0360);
const APIC_REG_LVT_ERROR: PhyPtr<u32> = PhyPtr::new(0xfee0_0370);
const APIC_REG_TIMER_INITCNT: PhyPtr<u32> = PhyPtr::new(0xfee0_0380);
const APIC_REG_TIMER_CURRENT: PhyPtr<u32> = PhyPtr::new(0xfee0_0390);
const APIC_REG_TIMER_DIV: PhyPtr<u32> = PhyPtr::new(0xfee0_03e0);
const PIT_HZ: usize = 119_3182;
const PIT_CH2: u16 = 0x42;
const PIT_CMD: u16 = 0x43;
const KBC_PORT_B: u16 = 0x61;
const KBC_B_OUT2_STATUS: u8 = 0x20;
const APIC_TIMER_DIV: u32 = 0x03;
pub const APIC_TIMER_VECTOR: u8 = 32;

unsafe fn calibrate_apic_timer() {
    // FIXME: This method works well on QEMU though on the real machine
    // this doesn't. For example on my machine `counts_per_sec' is 7x greater
    // than the desired value.

    // initialize PIT
    let freq = 1000; /* That is, every 1ms. */
    let divider = PIT_HZ / freq;
    asm::outb(
        KBC_PORT_B,
        asm::inb(KBC_PORT_B) & 0xfc, /* Disable ch #2 and speaker output */
    );
    asm::outb(PIT_CMD, 0xb2 /* Select ch #2 */);
    asm::outb(PIT_CH2, (divider & 0xff) as u8);
    asm::outb(PIT_CH2, ((divider >> 8) & 0xff) as u8);
    asm::outb(KBC_PORT_B, asm::inb(KBC_PORT_B) | 1 /* Enable ch #2 */);

    // Reset the counter in APIC timer.
    APIC_REG_TIMER_INITCNT.write(0xffff_ffff);

    // Wait for the PIT.
    loop {
        if (asm::inb(KBC_PORT_B) & KBC_B_OUT2_STATUS) != 0 {
            break;
        }
    }

    let counts_per_sec =
        ((0xffff_ffff - APIC_REG_TIMER_CURRENT.read()) * freq as u32) << APIC_TIMER_DIV;
    APIC_REG_TIMER_INITCNT.write(counts_per_sec / TICK_HZ as u32);
    printk!(
        "calibrated the APIC timer using PIT: %d counts/msec",
        counts_per_sec / 1000
    );
}

pub fn cpu_id() -> usize {
    unsafe { (APIC_REG_ID.read() >> 24) as usize }
}

pub fn ack_interrupt() {
    unsafe {
        APIC_REG_EOI.write(0);
    }
}

pub unsafe fn init_timer() {
    APIC_REG_TIMER_INITCNT.write(0xffff_ffff);
    APIC_REG_LVT_TIMER.write(APIC_TIMER_VECTOR as u32 | 0x20000);
    APIC_REG_TIMER_DIV.write(APIC_TIMER_DIV);
    calibrate_apic_timer();
}

pub unsafe fn init() {
    asm::wrmsr(
        MSR_APIC_BASE,
        (asm::rdmsr(MSR_APIC_BASE) & 0xffff_f100) | 0x0800,
    );
    APIC_REG_SPURIOUS_INT.write(1 << 8);
    APIC_REG_TPR.write(0);
    APIC_REG_LOGICAL_DEST.write(0x0100_0000);
    APIC_REG_DEST_FORMAT.write(0xffff_ffff);
    APIC_REG_LVT_TIMER.write(1 << 16);
    APIC_REG_LVT_ERROR.write(1 << 16);
}
