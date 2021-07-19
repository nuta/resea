#![allow(non_camel_case_types)]
use core::mem::size_of;

pub type c_int = i32;
pub type c_unsigned = u32;
pub type error_t = c_int;
pub type task_t = c_int;
pub type msec_t = c_int;
pub type notifications_t = u8;

cfg_if::cfg_if! {
    if #[cfg(target_pointer_width = "64")] {
        pub const MESSAGE_SIZE: usize = 256;
        pub type size_t = u64;
        pub type vaddr_t = u64;
    } else {
        pub const MESSAGE_SIZE: usize = 32;
        pub type size_t = u32;
        pub type vaddr_t = u32;
    }
}

/// A message.
#[repr(C)]
pub struct Message {
    /// The type of message. If it's negative, this field represents an error
    /// (error_t).
    message_type: c_int,
    /// The sender task of this message.
    src: task_t,
    /// The message contents. Note that it's a union, not struct!
    data: [u8; MESSAGE_SIZE - size_of::<c_int>() - size_of::<task_t>()],
}

extern "C" {
    pub fn malloc(size: size_t) -> *mut u8;
    pub fn free(ptr: *mut u8);
    pub fn sys_ipc(dst: task_t, src: task_t, m: *mut Message, flags: c_unsigned) -> error_t;
    pub fn sys_notify(dst: task_t, notifications: notifications_t) -> error_t;
    pub fn sys_timer_set(timeout: msec_t) -> error_t;
    pub fn sys_task_create(
        tid: task_t,
        name: *const u8,
        ip: vaddr_t,
        pager: task_t,
        flags: c_unsigned,
    ) -> task_t;
    pub fn sys_task_destroy(task: task_t) -> error_t;
    pub fn sys_task_exit() -> error_t;
    pub fn sys_task_self() -> task_t;
    pub fn sys_task_schedule(task: task_t, priority: c_int) -> error_t;
    pub fn sys_vm_map(
        task: task_t,
        vaddr: vaddr_t,
        src: vaddr_t,
        kpage: vaddr_t,
        flags: c_unsigned,
    ) -> error_t;
    pub fn sys_vm_unmap(task: task_t, vaddr: vaddr_t) -> error_t;
    pub fn sys_irq_acquire(irq: c_unsigned) -> error_t;
    pub fn sys_irq_release(irq: c_unsigned) -> error_t;
    pub fn sys_console_write(buf: *const u8, len: size_t) -> error_t;
    pub fn sys_console_read(buf: *mut u8, len: size_t) -> c_int;
    pub fn sys_kdebug(cmd: *const u8, cmd_len: size_t, buf: *mut u8, buf_len: size_t) -> error_t;

    pub fn ipc_call(dst: task_t, m: *mut Message) -> error_t;
}
