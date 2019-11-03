use crate::error::Error;
use crate::arch::syscall;
use crate::arch::thread_info::{copy_from_ipc_buffer, copy_to_ipc_buffer};
use crate::message::Message;
use core::mem::MaybeUninit;

#[repr(transparent)]
#[derive(Clone)]
pub struct Channel {
    cid: i32,
}

impl Channel {
    pub const fn from_cid(cid: i32) -> Channel {
        Channel { cid }
    }

    pub fn cid(&self) -> i32 {
        self.cid
    }

    pub fn recv(&mut self) -> Result<Message, Error> {
        unsafe {
            syscall::recv(self.cid).map(|_| {
                let mut recv_buf = Message::uninit();
                copy_from_ipc_buffer(&mut recv_buf);
                recv_buf
            })
        }
    }

    pub fn send(&mut self, m: &Message) -> Result<(), Error> {
        unsafe {
            copy_to_ipc_buffer(m);
            syscall::send(self.cid)
        }
    }

    pub fn send_err(&mut self, err: Error) -> Result<(), Error> {
        self.send(&Message::from_error(err))
    }

    pub fn call(&self, m: &Message) -> Result<Message, Error> {
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
