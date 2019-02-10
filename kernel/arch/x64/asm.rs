#[inline(always)]
pub unsafe fn outb(port: u16, value: u8) {
    asm!("outb %al, %dx" :: "{al}"(value), "{dx}"(port) :: "volatile");
}

#[inline(always)]
pub unsafe fn outw(port: u16, value: u16) {
    asm!("outw %ax, %dx" :: "{ax}"(value), "{dx}"(port) :: "volatile");
}

#[inline(always)]
pub unsafe fn inb(port: u16) -> u8 {
    let value;
    asm!("inb %dx, %al" : "={al}"(value) : "{dx}"(port) :: "volatile");
    value
}

#[inline(always)]
pub unsafe fn get_cr2() -> u64 {
    let cr2;
    asm!("mov %cr2, %rax" : "={rax}"(cr2) ::: "volatile");
    cr2
}

#[inline(always)]
pub unsafe fn set_cr3(paddr: u64) {
    asm!("mov %rax, %cr3" :: "{rax}"(paddr) : "memory" : "volatile");
}

#[inline(always)]
pub unsafe fn invlpg(vaddr: u64) {
    asm!("invlpg (%rax)" :: "rax"(vaddr) : "memory" : "volatile");
}

#[inline(always)]
pub unsafe fn wrmsr(reg: u32, value: u64) {
    let low = value & 0xffff_ffff;
    let hi = value >> 32;
    asm!("wrmsr" :: "{ecx}"(reg), "{eax}"(low), "{edx}"(hi) :: "volatile");
}

#[inline(always)]
pub unsafe fn rdmsr(reg: u32) -> u64 {
    let low: u32;
    let high: u32;
    asm!("rdmsr" : "={eax}"(low), "={edx}"(high) : "{ecx}"(reg) :: "volatile");
    ((high as u64) << 32) | low as u64
}

#[inline(always)]
pub unsafe fn lgdt(gdtr: u64) {
    asm!("lgdt (%rax)" :: "{rax}"(gdtr) :: "volatile");
}

#[inline(always)]
pub unsafe fn lidt(idtr: u64) {
    asm!("lidt (%rax)" :: "{rax}"(idtr) :: "volatile");
}

#[inline(always)]
pub unsafe fn ltr(tr: u16) {
    asm!("ltr %ax" :: "{ax}"(tr) :: "volatile");
}
