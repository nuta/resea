#![no_std]
#![allow(unused)]

#[macro_use]
extern crate resea;

#[macro_use]
mod macros;
mod instance;
mod arp;
mod device;
mod ethernet;
mod ip;
mod ipv4;
mod transport;
mod udp;
mod tcp;
mod packet;
mod checksum;
mod endian;
mod wrapping;
mod mbuf;
mod ring_buffer;

pub use instance::{Instance, SocketHandle};
pub use ip::IpAddr;
pub use transport::Port;
pub use ipv4::{Ipv4Addr, Ipv4Network};
pub use mbuf::Mbuf;
pub use ethernet::MacAddr;

pub type Result<T> = core::result::Result<T, Error>;

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Error {
    NoRoute,
    BufferFull,
}
