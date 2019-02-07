use crate::ipc::Msg;

//// Returns a message from the kernel server. This assumes that the compiler
/// does not use scratch registers (RDI, RSI, ... ) when we return from the
/// ipc_handler().
///
/// TODO: Abandon this function. This looks unacceptable dirty hack, doesn't it?
#[inline(always)]
pub fn return_from_kernel_server(m: Msg) {
    unsafe {
        asm!(
            ""
            :: "{rdi}"(m.header), "{rsi}"(m.p0), "{rdx}"(m.p1),
               "{rcx}"(m.p2), "{r8}"(m.p3), "{r9}"(m.p4)
        );
    }
}