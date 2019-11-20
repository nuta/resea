use crate::arch::syscall;
use crate::arch::thread_info::{copy_from_ipc_buffer, copy_to_ipc_buffer};
use crate::message::Message;
use crate::result::{Error, Result};

pub type CId = i32;

#[repr(transparent)]
pub struct Channel {
    cid: CId,
}

impl Channel {
    pub fn create() -> Result<Channel> {
        unsafe { syscall::open().map(Channel::from_cid) }
    }

    pub const fn from_cid(cid: CId) -> Channel {
        Channel { cid }
    }

    #[allow(clippy::should_implement_trait)]
    pub unsafe fn clone(&self) -> Channel {
        Channel::from_cid(self.cid)
    }

    pub fn cid(&self) -> CId {
        self.cid
    }

    pub fn transfer_to(&self, dest: &Channel) -> Result<()> {
        unsafe { syscall::transfer(self.cid, dest.cid) }
    }

    pub fn recv(&self) -> Result<Message> {
        unsafe {
            syscall::recv(self.cid).map(|_| {
                let mut recv_buf = Message::uninit();
                copy_from_ipc_buffer(&mut recv_buf);
                recv_buf
            })
        }
    }

    pub fn send(&self, m: &Message) -> Result<()> {
        unsafe {
            copy_to_ipc_buffer(m);
            syscall::send(self.cid)
        }
    }

    pub fn send_err(&self, err: Error) -> Result<()> {
        self.send(&Message::from_error(err))
    }

    pub fn call(&self, m: &Message) -> Result<Message> {
        unsafe {
            copy_to_ipc_buffer(m);
            syscall::call(self.cid).map(|_| {
                let mut recv_buf = Message::uninit();
                copy_from_ipc_buffer(&mut recv_buf);
                recv_buf
            })
        }
    }
}
