const EXP_PAGE_FAULT: u8 = 14;

#[repr(C, packed)]
pub struct Regs {
    rax: u64,
    rbx: u64,
    rcx: u64,
    rdx: u64,
    rsi: u64,
    rdi: u64,
    rbp: u64,
    r8: u64,
    r9: u64,
    r10: u64,
    r11: u64,
    r12: u64,
    r13: u64,
    r14: u64,
    r15: u64,
    error: u64,
    rip: u64,
    cs: u64,
    rflags: u64,
    rsp: u64,
    ss: u64,
}

fn print_regs(regs: *const Regs) {
    unsafe {
        printk!("RIP = %p    CS  = %p    RFLAGS = %p",
            (*regs).rip, (*regs).cs, (*regs).rflags);
        printk!("SS  = %p    RSP = %p    RBP = %p",
            (*regs).ss, (*regs).rsp, (*regs).rbp);
        printk!("RAX = %p    RBX = %p    RCX = %p",
            (*regs).rax, (*regs).rbx, (*regs).rcx);
        printk!("RDX = %p    RSI = %p    RDI = %p",
            (*regs).rdx, (*regs).rsi, (*regs).rdi);
        printk!("R8  = %p    R9  = %p    R10 = %p",
            (*regs).r8, (*regs).r9, (*regs).r10);
        printk!("R11 = %p    R12 = %p    R13 = %p",
            (*regs).r11, (*regs).r12, (*regs).r13);
        printk!("R14 = %p    R15 = %p",
            (*regs).r14, (*regs).r15);
    }
}

#[no_mangle]
pub extern "C" fn x64_handle_exception(exc: u8, regs: *const Regs) {
    unsafe {
        match exc {
            EXP_PAGE_FAULT => {
                printk!("Exception #%d", exc);
                print_regs(regs);
                super::paging::handle_page_fault((*regs).error);
            }
            _ => {
                printk!("Exception #%d", exc);
                print_regs(regs);
                panic!("Unhandled exception #{}", exc);
            }
        }
    }
}
