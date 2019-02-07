use super::asm;
use super::cpu::cpu_mut;

pub const GDT_DESC_NUM: usize = 8;
const GDT_NULL: usize = 0;
const GDT_KERNEL_CODE: usize = 1;
const GDT_KERNEL_DATA: usize = 2;
const GDT_USER_CODE32: usize = 3;
const GDT_USER_DATA: usize = 4;
const GDT_USER_CODE64: usize = 5;
// A TSS descriptor is twice as large as a code segment descriptor.
const GDT_TSS: usize = 6;
pub const KERNEL_CS: u16 = GDT_KERNEL_CODE as u16 * 8;
pub const TSS_SEG: u16 = GDT_TSS as u16 * 8;
pub const USER_CS32: u16 = GDT_USER_CODE32 as u16 * 8;
pub const USER_CS64: u16 = GDT_USER_CODE64 as u16 * 8;
/// Don't forget to update handler.S as well.
pub const USER_DS: u16 = GDT_USER_DATA as u16 * 8;
pub const USER_RPL: u16 = 3;

#[repr(C, packed)]
pub struct Gdtr {
    len: u16,
    laddr: u64,
}

pub unsafe fn init() {
    let gdt = &mut cpu_mut().gdt;
    let gdtr = &mut cpu_mut().gdtr;
    let tss = &mut cpu_mut().tss;

    gdt[GDT_NULL] = 0x0000_0000_0000_0000;
    gdt[GDT_KERNEL_CODE] = 0x00af_9a00_0000_ffff;
    gdt[GDT_KERNEL_DATA] = 0x00af_9200_0000_ffff;
    gdt[GDT_USER_CODE32] = 0;
    gdt[GDT_USER_CODE64] = 0x00af_fa00_0000_ffff;
    gdt[GDT_USER_DATA] = 0x008f_f200_0000_ffff;

    let tss_addr = tss as *const _ as u64;
    gdt[GDT_TSS] = 0x0000_8900_0000_0067
        | ((tss_addr & 0xffff) << 16)
        | (((tss_addr >> 16) & 0xff) << 32)
        | (((tss_addr >> 24) & 0xff) << 56);
    gdt[GDT_TSS + 1] = tss_addr >> 32;

    // Update GDTR
    gdtr.laddr = gdt as *const _ as u64;
    gdtr.len = ((core::mem::size_of::<u64>() * GDT_DESC_NUM) - 1) as u16;
    asm::lgdt(gdtr as *const _ as u64);
}
