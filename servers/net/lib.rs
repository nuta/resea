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
mod tcpip;
mod ip;
mod ipv4;
mod main;
mod mbuf;
mod packet;
mod ring_buffer;
mod tcp;
mod transport;
mod udp;
mod wrapping;

pub type Result<T> = core::result::Result<T, Error>;

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Error {
    NoRoute,
    BufferFull,
}
