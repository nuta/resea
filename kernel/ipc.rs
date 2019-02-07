use crate::arch::current;
use crate::thread;
use crate::arch;
use crate::thread::ThreadState;
use crate::allocator::Ref;
use crate::stats;
use crate::channel::{self, CId};
use crate::utils::VAddr;
use core::sync::atomic::Ordering;
use core::ptr::null_mut;

/// The system call error codes. A error code must be a negative integer.
#[repr(isize)]
pub enum SyscallError {
    Ok = 0,
    /// Returned in the arch's system call handler.
    #[allow(unused)]
    InvalidSyscall = -1,
    /// Invalid channel ID including unused and already closed
    /// channels.
    InvalidChannelId = -2,
    /// Failed to get the receiver right. Use a mutex in the when
    /// you share a channel by multiple threads!
    AlreadyReceived = -3,
    /// Invalid payload.
    InvalidPayload = -4,
}

pub type Payload = u64;

#[repr(transparent)]
#[derive(Copy, Clone)]
pub struct Header(Payload);

const FLAG_SEND: Payload = 1 << 26;
const FLAG_RECV: Payload = 1 << 27;
const FLAG_REPLY: Payload = 1 << 28;
const CHANNEL_OFFSET: Payload = 48;

impl Header {
    const fn new(value: Payload) -> Header {
        Header(value)
    }

    pub fn as_usize(&self) -> usize {
        self.0 as usize
    }

    #[inline]
    pub fn send(&self) -> bool {
        self.0 & FLAG_SEND != 0
    }

    #[inline]
    pub fn recv(&self) -> bool {
        self.0 & FLAG_RECV != 0
    }

    #[inline]
    pub fn reply(&self) -> bool {
        self.0 & FLAG_REPLY != 0
    }

    #[inline]
    pub fn channel(&self) -> CId {
        CId::from_isize((self.0 >> CHANNEL_OFFSET) as isize)
    }

    #[inline]
    pub fn msg_id(&self) -> u16 {
        (self.0 & 0xffff) as u16
    }

    #[inline]
    pub fn interface_id(&self) -> u8 {
        ((self.0 >> 8) & 0xff) as u8
    }

    #[inline]
    pub fn type_of(&self, index: usize) -> u8 {
        ((self.0 >> (16 + index * 2)) & 0x3) as u8
    }
}

impl Into<Payload> for Header {
    fn into(self) -> Payload {
        self.0 as Payload
    }
}

pub struct Msg {
    pub from: CId,
    pub header: Payload,
    pub p0: Payload,
    pub p1: Payload,
    pub p2: Payload,
    pub p3: Payload,
    pub p4: Payload,
}

const INLINE_PAYLOAD: u8 = 0;
const PAGE_PAYLOAD: u8 = 1;
const PAGER_INTERFACE: u8 = 3;

fn translate(header: Header, index: usize, p: Payload, dst_is_kernel: bool) -> Result<Payload, SyscallError> {
    match header.type_of(index) {
        INLINE_PAYLOAD => Ok(p),
        PAGE_PAYLOAD => {
            // XXX: Ensure that the receiver thread is in a kernel space if the
            //      interface is pager or it could lead to a serious vulnerability.
            if dst_is_kernel || header.interface_id() == PAGER_INTERFACE {
                let vaddr = p as usize & 0xffff_ffff_ffff_f000;
                let paddr = match current().process().page_table().resolve(VAddr::new(vaddr)) {
                    Some(paddr) => paddr,
                    None => return Err(SyscallError::InvalidPayload)
                };

                Ok(paddr.as_vaddr().as_usize() as Payload)
            } else {
                unimplemented!()
            }
        },
        _ => unimplemented!()
    }
}


#[no_mangle]
#[inline(never)]
pub extern "C" fn ipc_handler(
    raw_header: Payload,
    m0: Payload,
    m1: Payload,
    m2: Payload,
    m3: Payload,
    m4: Payload
) -> SyscallError {
    stats::IPC_TOTAL.increment();

    let header = Header::new(raw_header);
    let current = current();
    let proc = current.process();
    let src = {
        // Search the channel table for the specified channel ID.
        if let Some(ch) = proc.channels().get(header.channel()) {
            ch
        } else {
            trace!("invalid channel: @{}.{}",
                proc.pid(), header.channel().as_isize());
            stats::IPC_ERRORS.increment();
            return SyscallError::InvalidChannelId;
        }
    };
    let dst = src.linked_to().transfer_to();

    if dst.process().pid().as_isize() == 1 {
        // Call the kernel server.
        stats::IPC_KCALLS.increment();
        let r = crate::server::handle(header, m0, m1, m2, m3, m4, src);
        arch::return_from_kernel_server(r);
        return SyscallError::Ok;
    }

    //
    //  Send phase.
    //
    if header.send() {
        stats::IPC_SEND.increment();

        if header.interface_id() != 4 {
            trace!("send: @{}.{} -> @{}.{}, header: {:x}, m0: {:x}",
                proc.pid(),
                header.channel().as_isize(),
                dst.process().pid().as_isize(),
                dst.cid().as_isize(),
                raw_header,
                m0
            );
        }

        let receiver = loop {
            // Try to get the sender right.
            let receiver = dst.receiver().swap(null_mut(), Ordering::SeqCst);
            if !receiver.is_null() {
                break Ref::from_ptr(receiver);
            }

            // Failed to obtain the sender right. Append the current thread into
            // the sender queue in the destination channel and sleep.
            //
            // The receiver will unblock this thread and the execution resumes
            // from the else clause of this if expression. Strictly speaking,
            // arch::overwrite_context behaves as it returned `false`.
            if arch::overwrite_context(current) {
                // Overwrote the context. Keep in mind that the current context
                // could be preempted (and discarded) at *any* time.

                // Append to the sender queue *before* setting current as
                // blocked or the current thread would blocks forever if
                // a preemption occur before appending to the queue.
                dst.sender_queue().push_back(current);

                // Set the current thread as blocked (sleep state) and yield
                // another thread the CPU.
                current.set_state(ThreadState::Blocked);
                thread::switch();
            }
        };

        // Ñow we have the sender right, yay! Leave the loop and send the
        // message to the receiver thread.
        thread::enqueue(current); // we have to enqueue because we use send instead of switch
        thread::enqueue(receiver);
        receiver.set_state(ThreadState::Runnable);
        let dst_is_kernel = dst.process().pid().as_isize() == 1;

        // FIXME:
        let new_header = Header::new(
            (((src.linked_to().cid().as_isize() as usize) << 48)
            | (header.as_usize() & 0x3ff0000)
            | (header.msg_id() as usize)) as u64
        );

        arch::send(
            new_header.into(),
            try_or_return_error!(translate(new_header, 0, m0, dst_is_kernel)),
            try_or_return_error!(translate(new_header, 1, m1, dst_is_kernel)),
            try_or_return_error!(translate(new_header, 2, m2, dst_is_kernel)),
            try_or_return_error!(translate(new_header, 3, m3, dst_is_kernel)),
            try_or_return_error!(translate(new_header, 4, m4, dst_is_kernel)),
            current,
            receiver
        );
    }

    //
    //  Receive phase.
    //
    if header.recv() {
        stats::IPC_RECV.increment();

        let recv_on = if header.reply() {
            src.transfer_to()
        } else {
            src
        };

        if arch::overwrite_context(current) {
            // Overwrote the context. Keep in mind that the current context
            // could be preempted (and discarded) at *any* time.

            // Try to get the receiver right.
            if recv_on.receiver().compare_and_swap(null_mut(),
                unsafe { current.as_ptr() }, Ordering::SeqCst) != null_mut()
            {
                // It seems that currently another thread is the receiver of
                // the channel.
                trace!("failed to get receiver right");
                stats::IPC_ERRORS.increment();
                return SyscallError::AlreadyReceived;
            }

            // Resume a sender thread waiting for us. Here we use peek & delete
            // instead of pop method because if a preemption occured in the middle
            // of these resuming operations the thread will be blocked forever.
            if let Some(sender) = recv_on.sender_queue().peek_front() {
                sender.set_state(ThreadState::Runnable);
                thread::enqueue(sender);
                recv_on.sender_queue().delete_front();
            }

            // Get ready for receiving a message.
            current.set_state(ThreadState::Blocked);
            thread::do_switch(false);
        } else {
            // Received a message.
            return SyscallError::Ok;
        }
    }

    SyscallError::Ok
}

#[no_mangle]
pub extern "C" fn open_handler() -> isize {
    trace_user!("entering sys_open()");

    let ch = unwrap_or_return!(
        current().process().channels().alloc(),
        SyscallError::InvalidChannelId as isize
    );

    trace_user!("sys_open: created @{}", ch.cid().as_isize());
    ch.cid().as_isize()
}

#[no_mangle]
pub extern "C" fn link_handler(cid1: isize, cid2: isize) -> SyscallError {
    trace_user!("entering sys_link({}, {})", cid1, cid2);

    let proc = current().process();
    let table = proc.channels();

    let ch1 = unwrap_or_return!(
        table.get(CId::from_isize(cid1)),
        SyscallError::InvalidChannelId
    );

    let ch2 = unwrap_or_return!(
        table.get(CId::from_isize(cid2)),
        SyscallError::InvalidChannelId
    );

    channel::link(ch1, ch2);
    SyscallError::Ok
}

#[no_mangle]
pub extern "C" fn transfer_handler(cid1: isize, cid2: isize) -> SyscallError {
    trace_user!("entering sys_transfer({}, {})", cid1, cid2);

    let proc = current().process();
    let table = proc.channels();

    let src = unwrap_or_return!(
        table.get(CId::from_isize(cid1)),
        SyscallError::InvalidChannelId
    );

    let dst = unwrap_or_return!(
        table.get(CId::from_isize(cid2)),
        SyscallError::InvalidChannelId
    );

    channel::transfer(src, dst);
    SyscallError::Ok
}