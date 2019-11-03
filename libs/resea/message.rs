use crate::error::Error;

pub const SYSCALL_IPC: u32 = 0;
pub const SYSCALL_OPEN: u32 = 1;
pub const IPC_SEND: u32 = 1 << 8;
pub const IPC_RECV: u32 = 1 << 9;

pub const MSG_REPLY_FLAG: u32 = 1 << 7;
pub const MSG_INLINE_LEN_OFFSET: u32 = 0;
pub const MSG_TYPE_OFFSET: u32 = 16;
pub const MSG_PAGE_PAYLOAD: u32 = 1 << 11;
pub const MSG_CHANNEL_PAYLOAD: u32 = 1 << 12;

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

    pub fn interface_id(self) -> u8 {
        ((self.msg_type() >> 8) & 0xff) as u8
    }

    pub fn msg_type(self) -> u16 {
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
}

#[repr(C, packed)]
pub struct Page {
    pub addr: usize,
    pub len: usize,
}

pub const MESSAGE_SIZE: usize = 256;
pub const INLINE_PAYLOAD_LEN_MAX: usize = MESSAGE_SIZE - core::mem::size_of::<usize>() * 4;
pub type CId = i32;

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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    pub fn assertions() {
        assert_eq!(core::mem::size_of::<Message>(), MESSAGE_SIZE);
    }
}
