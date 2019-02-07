use super::asm;
use super::cpu::cpu_mut;
use super::gdt::TSS_SEG;
use super::PAGE_SIZE;
use crate::allocator::PAGE_POOL;

pub const INTR_HANDLER_IST: u8 = 1;
pub const EXP_HANDLER_IST: u8 = 2;

#[repr(C, packed)]
pub struct Tss {
    reserved0: u32,
    rsp0: u64,
    rsp1: u64,
    rsp2: u64,
    reserved1: u64,
    ist: [u64; 7],
    reserved2: u64,
    reserved3: u16,
    iomap: u16,
}

pub unsafe fn init() {
    let tss = &mut cpu_mut().tss;

    let intr_stack = PAGE_POOL.alloc_or_panic();
    let intr_rsp = (intr_stack.as_usize() + PAGE_SIZE) as u64;
    let exp_stack = PAGE_POOL.alloc_or_panic();
    let exp_rsp = (exp_stack.as_usize() + PAGE_SIZE) as u64;

    // Set the stack pointer for interrupt/exception handlers.
    tss.ist[INTR_HANDLER_IST as usize - 1] = intr_rsp;
    tss.ist[EXP_HANDLER_IST as usize - 1] = exp_rsp;
    // Disable I/O permission map.
    tss.iomap = core::mem::size_of::<Tss>() as u16;

    asm::ltr(TSS_SEG);
}
