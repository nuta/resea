use super::asm;
use super::gdt::KERNEL_CS;
use super::tss::{EXP_HANDLER_IST, INTR_HANDLER_IST};
use core::mem::size_of;

const IDT_LENGTH: u16 = (IDT_DESC_NUM * size_of::<IdtDesc>() + 1) as u16;
const IDT_DESC_NUM: usize = 256;
const IDT_INT_HANDLER: u8 = 0x8e;

#[repr(C, packed)]
#[derive(Copy, Clone)]
struct IdtDesc {
    offset1: u16,
    seg: u16,
    ist: u8,
    info: u8,
    offset2: u16,
    offset3: u32,
    reserved: u32,
}

impl IdtDesc {
    pub fn init_as_handler(&mut self, handler: u64, ist: u8) {
        self.offset1 = (handler & 0xffff) as u16;
        self.seg = KERNEL_CS;
        self.ist = ist;
        self.info = IDT_INT_HANDLER;
        self.offset2 = ((handler >> 16) & 0xffff) as u16;
        self.offset3 = ((handler >> 32) & 0xffff_ffff) as u32;
        self.reserved = 0;
    }
}

#[repr(C, packed)]
struct Idtr {
    len: u16,
    laddr: u64,
}

static mut IDT: [IdtDesc; IDT_DESC_NUM] = [IdtDesc {
    offset1: 0,
    seg: 0,
    ist: 0,
    info: 0,
    offset2: 0,
    offset3: 0,
    reserved: 0,
}; IDT_DESC_NUM];

static mut IDTR: Idtr = Idtr {
    len: IDT_LENGTH,
    laddr: 0,
};

extern "C" {
    fn exception_handler0();
    fn exception_handler1();
    fn exception_handler2();
    fn exception_handler3();
    fn exception_handler4();
    fn exception_handler5();
    fn exception_handler6();
    fn exception_handler7();
    fn exception_handler8();
    fn exception_handler9();
    fn exception_handler10();
    fn exception_handler11();
    fn exception_handler12();
    fn exception_handler13();
    fn exception_handler14();
    fn exception_handler15();
    fn exception_handler16();
    fn exception_handler17();
    fn exception_handler18();
    fn exception_handler19();
    fn exception_handler20();
    fn interrupt_handler32();
    static interrupt_handler_size: u64;
}

pub unsafe fn init_exception_handlers() {
    IDT[0].init_as_handler(exception_handler0 as *const () as u64, EXP_HANDLER_IST);
    IDT[1].init_as_handler(exception_handler1 as *const () as u64, EXP_HANDLER_IST);
    IDT[2].init_as_handler(exception_handler2 as *const () as u64, EXP_HANDLER_IST);
    IDT[3].init_as_handler(exception_handler3 as *const () as u64, EXP_HANDLER_IST);
    IDT[4].init_as_handler(exception_handler4 as *const () as u64, EXP_HANDLER_IST);
    IDT[5].init_as_handler(exception_handler5 as *const () as u64, EXP_HANDLER_IST);
    IDT[6].init_as_handler(exception_handler6 as *const () as u64, EXP_HANDLER_IST);
    IDT[7].init_as_handler(exception_handler7 as *const () as u64, EXP_HANDLER_IST);
    IDT[8].init_as_handler(exception_handler8 as *const () as u64, EXP_HANDLER_IST);
    IDT[9].init_as_handler(exception_handler9 as *const () as u64, EXP_HANDLER_IST);
    IDT[10].init_as_handler(exception_handler10 as *const () as u64, EXP_HANDLER_IST);
    IDT[11].init_as_handler(exception_handler11 as *const () as u64, EXP_HANDLER_IST);
    IDT[12].init_as_handler(exception_handler12 as *const () as u64, EXP_HANDLER_IST);
    IDT[13].init_as_handler(exception_handler13 as *const () as u64, EXP_HANDLER_IST);
    IDT[14].init_as_handler(exception_handler14 as *const () as u64, EXP_HANDLER_IST);
    IDT[15].init_as_handler(exception_handler15 as *const () as u64, EXP_HANDLER_IST);
    IDT[16].init_as_handler(exception_handler16 as *const () as u64, EXP_HANDLER_IST);
    IDT[17].init_as_handler(exception_handler17 as *const () as u64, EXP_HANDLER_IST);
    IDT[18].init_as_handler(exception_handler18 as *const () as u64, EXP_HANDLER_IST);
    IDT[19].init_as_handler(exception_handler19 as *const () as u64, EXP_HANDLER_IST);
    IDT[20].init_as_handler(exception_handler20 as *const () as u64, EXP_HANDLER_IST);
}

pub unsafe fn init_interrupt_handlers() {
    let mut offset = interrupt_handler32 as *const () as u64;
    for i in 32..=255 {
        IDT[i].init_as_handler(offset, INTR_HANDLER_IST);
        offset += interrupt_handler_size;
    }
}

pub unsafe fn init() {
    init_exception_handlers();
    init_interrupt_handlers();

    // Reload IDTR.
    IDTR.laddr = &IDT[0] as *const _ as u64;
    asm::lidt(&IDTR as *const _ as u64);
}
