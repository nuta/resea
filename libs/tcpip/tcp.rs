use crate::checksum::Checksum;
use crate::device::Device;
use crate::endian::{swap16, swap32, NetEndian};
use crate::ip::IpAddr;
use crate::ipv4::IPV4_PROTO_TCP;
use crate::mbuf::Mbuf;
use crate::packet::Packet;
use crate::ring_buffer::RingBuffer;
use crate::transport::{
    BindTo, Port, Socket, TcpTransportHeader, TransportHeader, TransportProtocol,
};
use crate::wrapping::WrappingU32;
use crate::{Error, Result};
use resea::collections::{Vec, VecDeque};
use resea::std::boxed::Box;
use resea::std::cell::RefCell;
use resea::std::cmp::min;
use resea::std::fmt;
use resea::std::mem::size_of;
use resea::std::rc::Rc;

#[derive(Clone, Copy, PartialEq, Eq)]
enum TcpState {
    Closed,
    Listen,
    SynReceived,
    SynSent,
    Established,
    FinWait1,
    FinWait2,
    Closing,
    TimeWait,
    LastAck,
}

const TCP_BUFFER_SIZE: usize = 2048;

pub struct TcpSocket {
    local_addr: IpAddr,
    local_port: Port,
    remote_addr: Option<IpAddr>,
    remote_port: Option<Port>,
    state: TcpState,
    local_seq_no: WrappingU32,
    local_ack_no: WrappingU32,
    remote_ack_no: WrappingU32,
    remote_win_size: usize,
    bytes_not_acked: usize,
    pending_flags: TcpFlags,
    tx: RingBuffer,
    rx: RingBuffer,
    // TODO: Implement SYN cookie.
    backlog: VecDeque<TcpSocket>,
}

impl TcpSocket {
    pub fn new(local_addr: IpAddr, local_port: Port) -> TcpSocket {
        TcpSocket {
            local_addr,
            local_port,
            remote_addr: None,
            remote_port: None,
            state: TcpState::Closed,
            local_seq_no: WrappingU32::new(0),
            local_ack_no: WrappingU32::new(0),
            remote_ack_no: WrappingU32::new(0),
            remote_win_size: 0,
            bytes_not_acked: 0,
            pending_flags: TcpFlags::empty(),
            tx: RingBuffer::new(TCP_BUFFER_SIZE),
            rx: RingBuffer::new(TCP_BUFFER_SIZE),
            backlog: VecDeque::new(),
        }
    }

    pub fn new_listen(local_addr: IpAddr, local_port: Port) -> TcpSocket {
        let mut sock = TcpSocket::new(local_addr, local_port);
        sock.state = TcpState::Listen;
        sock
    }

    pub fn is_connected_with(&self, remote_addr: IpAddr, remote_port: Port) -> bool {
        self.remote_addr.unwrap() == remote_addr && self.remote_port.unwrap() == remote_port
    }

    fn receive_on_established<'a>(&mut self, header: &TcpTransportHeader<'a>) {
        trace!(
            "tcp: data_len={}, seq={}, ack={}, last_ack={}, last_seq={} [{}]",
            header.payload.len(),
            header.seq_no,
            header.ack_no,
            self.local_ack_no.as_u32(),
            self.local_seq_no.as_u32(),
            header.flags
        );

        if header.flags.contains(FLAG_RST) || header.flags.contains(FLAG_SYN) {
            self.state = TcpState::Closed;
            self.pending_flags.add(FLAG_RST);
            return;
        }

        if header.flags.contains(FLAG_FIN) {
            info!("received fin");
            self.state = TcpState::Closed;
            self.pending_flags.add(FLAG_FIN | FLAG_ACK);
            self.local_ack_no.add(1);
        }

        self.remote_win_size = header.win_size as usize;

        if header.flags.contains(FLAG_ACK) {
            self.remote_ack_no = header.ack_no.into();
            let len_received_by_remote = self.remote_ack_no.abs_diff(self.local_seq_no) as usize;
            // Check if the ack number is valid.
            if len_received_by_remote <= self.tx.readable_len() {
                self.tx.discard(len_received_by_remote);
                warn!(
                    "local_se_no: {}, add={}",
                    self.local_seq_no.as_u32(),
                    len_received_by_remote
                );
                self.local_seq_no.add(len_received_by_remote as u32);
                warn!("new_local_se_no: {}", self.local_seq_no.as_u32());
                self.bytes_not_acked -= len_received_by_remote;
            }
        }

        // Receive data.
        if header.payload.len() > 0 {
            if header.seq_no == self.local_ack_no.as_u32() {
                match self.rx.write(header.payload) {
                    Ok(()) => {
                        warn!(
                            "received payload: {} bytes ({:x?})",
                            header.payload.len(),
                            &header.payload[0..5]
                        );
                        self.local_ack_no.add(header.payload.len() as u32);
                    }
                    Err(Error::BufferFull) => {
                        // Don't advance the ack number to notify the remote
                        // peer that our buffer is full.
                        warn!("too long packet, dropping silently...");
                    }
                    _ => unreachable!(),
                }
                self.pending_flags.add(FLAG_ACK);
            } else {
                // Send an ACK to request the data that we haven't yet received.
                self.pending_flags.add(FLAG_ACK);
            }
        }
    }

    pub fn remote_addr(&self) -> Option<IpAddr> {
        self.remote_addr
    }

    pub fn remote_port(&self) -> Option<Port> {
        self.remote_port
    }
}

impl Socket for TcpSocket {
    fn build(&mut self) -> Option<(Option<Rc<RefCell<dyn Device>>>, IpAddr, Mbuf)> {
        for backlog in &mut self.backlog {
            match backlog.state {
                TcpState::Listen => {
                    // Send SYN + ACK.
                    let header_words = 5;
                    backlog.state = TcpState::SynReceived;
                    warn!("rx.writable_len = {}", backlog.rx.writable_len());
                    assert!(backlog.remote_addr.is_some());
                    assert!(backlog.remote_port.is_some());
                    let mut mbuf = Mbuf::new();
                    let mut header = TcpHeader {
                        dst_port: backlog.remote_port.unwrap().as_u16().into(),
                        src_port: backlog.local_port.as_u16().into(),
                        seq_no: backlog.local_seq_no.as_u32().into(),
                        ack_no: backlog.local_ack_no.as_u32().into(),
                        data_offset_and_ns: (header_words << 4).into(),
                        flags: TcpFlags::new(FLAG_SYN | FLAG_ACK),
                        win_size: 1024.into(), // TODO: (backlog.rx.writable_len() as u16).into(),
                        checksum: 0.into(),
                        urgent_pointer: 0.into(),
                    };

                    let mut checksum = Checksum::new();
                    compute_header_checksum(
                        &mut checksum,
                        backlog.local_addr,
                        backlog.remote_addr.unwrap(),
                        &header,
                        0,
                    );
                    header.checksum = checksum.finish().into();

                    mbuf.append(&header);
                    return Some((None, backlog.remote_addr.unwrap(), mbuf));
                }
                TcpState::SynReceived | TcpState::Established => (),
                _ => unreachable!(),
            }
        }

        let pending_bytes = self.tx.readable_len() - self.bytes_not_acked;
        let send_payload = pending_bytes > 0 && pending_bytes < self.remote_win_size;
        if self.state == TcpState::Listen || (self.pending_flags.is_empty() && !send_payload) {
            return None;
        }

        let mut mbuf = Mbuf::new();
        let mut header = TcpHeader {
            dst_port: self.remote_port.unwrap().as_u16().into(),
            src_port: self.local_port.as_u16().into(),
            seq_no: self.local_seq_no.as_u32().into(),
            ack_no: if self.pending_flags.contains(FLAG_ACK) {
                self.local_ack_no.as_u32().into()
            } else {
                0.into()
            },
            data_offset_and_ns: (5 << 4).into(),
            flags: self.pending_flags,
            win_size: 1024.into(), // TODO: (self.rx.writable_len() as u16).into(),
            checksum: 0.into(),
            urgent_pointer: 0.into(),
        };

        let mut checksum = Checksum::new();
        if send_payload {
            let len = min(self.tx.readable_len(), self.remote_win_size);
            let mut data = Vec::with_capacity(len);
            self.tx.peek(len, &mut data);
            mbuf.append_bytes(&data);
            compute_header_checksum(
                &mut checksum,
                self.local_addr,
                self.remote_addr.unwrap(),
                &header,
                len,
            );
            checksum.input_bytes(&data);
            self.bytes_not_acked += len;
            self.remote_win_size -= len;
        } else {
            compute_header_checksum(
                &mut checksum,
                self.local_addr,
                self.remote_addr.unwrap(),
                &header,
                0,
            );
        }

        header.checksum = checksum.finish().into();
        mbuf.prepend(&header);

        self.pending_flags.clear();
        Some((None, self.remote_addr.unwrap(), mbuf))
    }

    fn close(&mut self) {
        // TODO:
        // self.pending_flags.add(FLAG_FIN);
        // self.state = TcpState::FinWait1;
    }

    fn accept(&mut self) -> Option<TcpSocket> {
        if let Some(sock) = self.backlog.front() {
            if sock.state != TcpState::Established {
                // The socket is not ready to accept.
                return None;
            }
        }

        self.backlog.pop_front()
    }

    fn read(&mut self, buf: &mut Vec<u8>, len: usize) -> usize {
        let read_len = min(len, self.rx.readable_len());
        self.rx.read(read_len, buf);
        read_len
    }

    fn write(&mut self, data: &[u8]) -> Result<()> {
        self.tx.write(data)?;
        self.pending_flags.add(FLAG_ACK | FLAG_PSH);
        Ok(())
    }

    fn receive<'a>(&mut self, src_addr: IpAddr, dst_addr: IpAddr, header: &TransportHeader<'a>) {
        let header = match header {
            TransportHeader::Tcp(header) => header,
            _ => unreachable!(),
        };
        let src_port = header.src_port;

        // Check if the client is already in the backlog.
        for backlog in &mut self.backlog {
            if backlog.is_connected_with(src_addr, src_port) {
                match backlog.state {
                    TcpState::Established => {
                        backlog.receive_on_established(header);
                    }
                    TcpState::SynReceived if header.flags.contains(FLAG_ACK) => {
                        backlog.state = TcpState::Established;
                        backlog.local_seq_no = WrappingU32::new(header.ack_no);
                    }
                    _ => {
                        // Unexpected message: send RST.
                    }
                }

                return;
            }
        }

        match self.state {
            TcpState::Established => {
                self.receive_on_established(header);
            }
            TcpState::Listen if header.flags.contains(FLAG_SYN) => {
                trace!(
                    "SYN: {}:{} <- {}:{}",
                    dst_addr,
                    self.local_port,
                    src_addr,
                    src_port
                );
                // The client wants to connect to this socket. Create a new
                // socket for it.
                let mut client = TcpSocket::new(dst_addr, self.local_port);
                client.state = TcpState::Listen;
                client.remote_addr = Some(src_addr);
                client.remote_port = Some(src_port);
                client.remote_win_size = header.win_size as usize;
                client.local_ack_no = WrappingU32::new(header.seq_no);
                client.local_ack_no.add(1);
                self.backlog.push_back(client);
            }
            TcpState::FinWait1 if header.flags.contains(FLAG_FIN) => {
                self.pending_flags.add(FLAG_ACK);
                self.local_ack_no.add(1);
                self.state = TcpState::Closed;
            }
            _ => {
                // Unexpected message: send RST.
            }
        }
    }

    fn protocol(&self) -> TransportProtocol {
        TransportProtocol::Tcp
    }

    fn bind_to(&self) -> BindTo {
        BindTo::new(TransportProtocol::Tcp, self.local_addr, self.local_port)
    }

    fn send(
        &mut self,
        device: Option<Rc<RefCell<dyn Device>>>,
        dst_addr: IpAddr,
        dst_port: Port,
        payload: &[u8],
    ) {
        unreachable!();
    }

    fn recv(&mut self) -> Option<(IpAddr, Port, Vec<u8>)> {
        unreachable!();
    }
}

fn compute_header_checksum(
    checksum: &mut Checksum,
    remote_addr: IpAddr,
    local_addr: IpAddr,
    header: &TcpHeader,
    data_len: usize,
) {
    // Pseudo header.
    match remote_addr {
        IpAddr::Ipv4(ipv4_addr) => checksum.input_u32(swap32(ipv4_addr.as_u32())),
    }
    match local_addr {
        IpAddr::Ipv4(ipv4_addr) => checksum.input_u32(swap32(ipv4_addr.as_u32())),
    }
    checksum.input_u16(swap16(IPV4_PROTO_TCP as u16));
    checksum.input_u16(swap16((size_of::<TcpHeader>() + data_len) as u16));

    // TCP header.
    checksum.input_struct(header);
}

#[repr(transparent)]
#[derive(Debug, Clone, Copy)]
pub struct TcpFlags(u8);

const FLAG_FIN: u8 = 1 << 0;
const FLAG_SYN: u8 = 1 << 1;
const FLAG_RST: u8 = 1 << 2;
const FLAG_PSH: u8 = 1 << 3;
const FLAG_ACK: u8 = 1 << 4;

impl TcpFlags {
    pub fn new(flags: u8) -> TcpFlags {
        TcpFlags(flags)
    }

    pub fn empty() -> TcpFlags {
        TcpFlags(0)
    }

    pub fn contains(self, flags: u8) -> bool {
        self.0 & flags == flags
    }

    pub fn is_empty(self) -> bool {
        self.0 == 0
    }

    pub fn add(&mut self, flags: u8) {
        self.0 |= flags;
    }

    pub fn clear(&mut self) {
        self.0 = 0;
    }
}

impl fmt::Display for TcpFlags {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if self.contains(FLAG_FIN) {
            write!(f, "FIN ")?;
        }

        if self.contains(FLAG_SYN) {
            write!(f, "SYN ")?;
        }

        if self.contains(FLAG_ACK) {
            write!(f, "ACK ")?;
        }

        if self.contains(FLAG_RST) {
            write!(f, "RST ")?;
        }

        Ok(())
    }
}

#[repr(C, packed)]
struct TcpHeader {
    src_port: NetEndian<u16>,
    dst_port: NetEndian<u16>,
    seq_no: NetEndian<u32>,
    ack_no: NetEndian<u32>,
    data_offset_and_ns: u8,
    flags: TcpFlags,
    win_size: NetEndian<u16>,
    checksum: NetEndian<u16>,
    urgent_pointer: NetEndian<u16>,
}

pub fn parse<'a>(pkt: &'a mut Packet, len: usize) -> Option<(Port, Port, TransportHeader<'a>)> {
    let header = match pkt.consume::<TcpHeader>() {
        Some(header) => header,
        None => return None,
    };

    // TODO: Handle TCP options.

    let dst_port = Port::new(header.dst_port.into());
    let src_port = Port::new(header.src_port.into());
    let ack_no = header.ack_no.into();
    let seq_no = header.seq_no.into();
    let flags = header.flags;
    let win_size = header.win_size.into();
    let header_len = ((header.data_offset_and_ns >> 4) * 4) as usize;
    let payload = unwrap_or_return!(pkt.consume_bytes(len - header_len), None);
    let parsed_header = TransportHeader::Tcp(TcpTransportHeader {
        src_port,
        payload,
        ack_no,
        seq_no,
        flags,
        win_size,
    });

    Some((src_port, dst_port, parsed_header))
}
