use super::asm;
use super::gdt::{KERNEL_CS, USER_CS32, USER_CS64, USER_DS};

const MSR_EFER: u32 = 0xc000_0080;
const MSR_STAR: u32 = 0xc000_0081;
const MSR_LSTAR: u32 = 0xc000_0082;
const MSR_CSTAR: u32 = 0xc000_0083;
const MSR_SFMASK: u32 = 0xc000_0084;
const EFER_SCE: u64 = 0x01;

extern "C" {
    fn x64_syscall_handler();
}

pub unsafe fn init() {
    // Verify SYSRET constraints.
    assert_eq!(USER_CS32 + 8, USER_DS);
    assert_eq!(USER_CS32 + 16, USER_CS64);

    asm::wrmsr(
        MSR_STAR,
        ((USER_CS32 as u64) << 48) | ((KERNEL_CS as u64) << 32),
    );
    asm::wrmsr(MSR_LSTAR, x64_syscall_handler as *const () as u64);

    // RIP for compatibility mode. We don't support it.
    asm::wrmsr(MSR_CSTAR, 0);
    asm::wrmsr(MSR_SFMASK, 0);

    // Enable SYSCALL/SYSRET.
    asm::wrmsr(MSR_EFER, asm::rdmsr(MSR_EFER) | EFER_SCE);
}
