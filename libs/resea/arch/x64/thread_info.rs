use crate::message::Message;

/// Thread Information Block (TIB).
#[repr(C, packed)]
pub struct ThreadInfo {
    /// The pointer to itself in the userspace. This field must be the first.
    pub this: *mut ThreadInfo,
    /// The user-provided argument.
    pub arg: usize,
    /// The page base set by the user.
    pub page_addr: usize,
    /// The maximum number of pages to receive page payloads.
    pub num_pages: usize,
    /// The thread-local user IPC buffer.
    pub ipc_buffer: Message,
}

pub unsafe fn get_thread_info() -> &'static mut ThreadInfo {
    let info: *mut ThreadInfo;
    asm!("movq %gs:(0), $0" : "={rax}"(info) ::: "volatile");
    &mut *info
}

pub unsafe fn copy_to_ipc_buffer(m: &Message) {
    // TODO: Read the length of the inline payload not to copy the whole message.
    core::ptr::copy_nonoverlapping(
        m as *const _,
        &mut get_thread_info().ipc_buffer as *mut _,
        1,
    );
}

pub unsafe fn copy_from_ipc_buffer(m: &mut Message) {
    // TODO: Read the length of the inline payload not to copy the whole message.
    core::ptr::copy_nonoverlapping(&get_thread_info().ipc_buffer as *const _, m as *mut _, 1);
}
