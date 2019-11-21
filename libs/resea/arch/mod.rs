mod x64;

#[cfg(target_arch = "x86_64")]
pub use crate::arch::x64::*;
