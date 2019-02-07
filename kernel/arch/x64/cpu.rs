use super::gdt::{Gdtr, GDT_DESC_NUM};
use super::tss::Tss;
use super::{apic, CPU_VAR_ADDR};
use crate::allocator::Ref;
use crate::thread::Thread;
use crate::utils::LazyStatic;
use core::mem::size_of;

/// Per-CPU variables.
pub struct CPU {
    pub idle_thread: LazyStatic<Ref<Thread>>,
    pub(super) gdt: [u64; GDT_DESC_NUM],
    pub(super) gdtr: Gdtr,
    pub(super) tss: Tss,
}

/// Returns a reference to a cpu-local variables.
pub fn cpu() -> &'static CPU {
    cpu_mut()
}

/// Returns a mutable reference to a cpu-local variables. **Please
/// ensure that interrupts are disabled**.
pub fn cpu_mut() -> &'static mut CPU {
    unsafe { &mut *CPU_VAR_ADDR.add(size_of::<CPU>() * apic::cpu_id()).as_ptr() }
}

// Hangs up.
pub fn halt() -> ! {
    loop {
        unsafe {
            asm!("cli; hlt");
        }
    }
}