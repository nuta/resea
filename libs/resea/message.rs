use crate::prelude::*;

pub const SYSCALL_IPC: u32 = 0;
pub const SYSCALL_OPEN: u32 = 1;
pub const IPC_SEND: u32 = 1 << 8;
pub const IPC_RECV: u32 = 1 << 9;

pub const MSG_REPLY_FLAG: u32 = 1 << 7;
pub const MSG_INLINE_LEN_OFFSET: u32 = 0;
pub const MSG_TYPE_OFFSET: u32 = 16;
pub const MSG_PAGE_PAYLOAD: u32 = 1 << 11;
pub const MSG_CHANNEL_PAYLOAD: u32 = 1 << 12;

pub type InterfaceId = u8;
pub type MessageType = u16;

#[repr(transparent)]
#[derive(Clone, Copy, PartialEq, Eq)]
pub struct MessageHeader(u32);

impl MessageHeader {
    pub const fn new(value: u32) -> MessageHeader {
        MessageHeader(value)
    }

    pub fn from_error(err: Error) -> MessageHeader {
        MessageHeader(err as u32)
    }

    pub fn interface_id(self) -> InterfaceId {
        ((self.msg_type() >> 8) & 0xff) as u8
    }

    pub fn msg_type(self) -> MessageType {
        ((self.0 >> MSG_TYPE_OFFSET) & 0xffff) as u16
    }

    pub fn inline_len(self) -> usize {
        (self.0 & 0x7ff) as usize
    }
}

#[repr(transparent)]
#[derive(Clone, Copy)]
pub struct Notification(u32);

impl Notification {
    pub const fn new(value: u32) -> Notification {
        Notification(value)
    }

    pub const fn empty() -> Notification {
        Notification::new(0)
    }

    pub fn is_empty(self) -> bool {
        self.0 == 0
    }
}

#[repr(C, packed)]
#[derive(Clone, Copy)]
pub struct Page {
    pub addr: usize,
    pub len: usize,
}

impl Page {
    pub const fn new(addr: usize, len: usize) -> Page {
        Page { addr, len }
    }

    pub fn len(&self) -> usize {
        self.len
    }

    pub fn as_slice_mut<T>(&mut self) -> &mut [T] {
        unsafe {
            let num = self.len / core::mem::size_of::<T>();
            core::slice::from_raw_parts_mut(self.as_mut_ptr(), num)
        }
    }

    pub fn as_slice<T>(&self) -> &[T] {
        unsafe {
            let num = self.len / core::mem::size_of::<T>();
            core::slice::from_raw_parts(self.as_ptr(), num)
        }
    }

    pub fn as_bytes_mut(&mut self) -> &mut [u8] {
        self.as_slice_mut()
    }

    pub fn as_bytes(&self) -> &[u8] {
        self.as_slice()
    }

    pub unsafe fn as_ptr<T>(&self) -> *const T {
        self.addr as *const T
    }

    pub unsafe fn as_mut_ptr<T>(&self) -> *mut T {
        self.addr as *mut T
    }

    pub fn copy_from_slice(&mut self, data: &[u8]) {
        let len = data.len();
        self.len = len;
        self.as_bytes_mut().copy_from_slice(data);
    }
}

pub const MESSAGE_SIZE: usize = 256;
pub const INLINE_PAYLOAD_LEN_MAX: usize = MESSAGE_SIZE - core::mem::size_of::<usize>() * 4;

#[repr(C, packed)]
pub struct Message {
    pub header: MessageHeader,
    pub from: CId,
    pub notification: Notification,
    pub channel: CId,
    pub page: Page,
    pub data: [u8; INLINE_PAYLOAD_LEN_MAX],
}

impl Message {
    pub unsafe fn uninit() -> Message {
        core::mem::MaybeUninit::uninit().assume_init()
    }

    pub fn from_error(error: Error) -> Message {
        let mut m = unsafe { Message::uninit() };
        m.header = MessageHeader::from_error(error);
        m
    }
}

pub type HandleId = i32;

#[repr(transparent)]
#[derive(PartialEq, Eq, Hash)]
pub struct Handle(HandleId);

impl Handle {
    pub const fn from_id(id: HandleId) -> Handle {
        Handle(id)
    }

    pub fn id(&self) -> HandleId {
        self.0
    }
}

#[repr(transparent)]
pub struct FixedString([u8; 128]);

impl FixedString {
    #[allow(clippy::should_implement_trait)]
    pub fn from_str(from: &str) -> FixedString {
        unsafe {
            use core::{cmp::min, mem::MaybeUninit, ptr::copy_nonoverlapping};
            #[allow(clippy::uninit_assumed_init)]
            let mut string: FixedString = MaybeUninit::uninit().assume_init();
            let copy_len = min(from.len(), 127);
            copy_nonoverlapping(from.as_ptr(), string.0.as_mut_ptr(), copy_len);
            // Set NUL character.
            *string.0.as_mut_ptr().add(copy_len) = 0;
            string
        }
    }

    #[allow(clippy::inherent_to_string)]
    pub fn to_string(&self) -> String {
        String::from(self.to_str())
    }

    pub fn to_str(&self) -> &str {
        unsafe { crate::utils::c_str_to_str(self.0.as_ptr()) }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    pub fn assertions() {
        assert_eq!(core::mem::size_of::<Message>(), MESSAGE_SIZE);
    }
}
