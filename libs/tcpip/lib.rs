#![no_std]

#[macro_use]
extern crate resea;

#[macro_use]
mod macros;
mod arp;
mod checksum;
mod device;
mod dhcp;
mod endian;
mod ethernet;
mod instance;
mod ip;
mod ipv4;
mod mbuf;
mod packet;
mod ring_buffer;
mod tcp;
mod transport;
mod udp;
mod wrapping;

pub use device::MacAddr;
pub use instance::{DeviceIpAddr, Instance, SocketHandle};
pub use ip::IpAddr;
pub use ipv4::{Ipv4Addr, Ipv4Network};
pub use mbuf::Mbuf;
pub use transport::Port;

pub type Result<T> = core::result::Result<T, Error>;

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Error {
    NoRoute,
    BufferFull,
}
