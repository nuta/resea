//! The kernel server.
use crate::arch;
use crate::arch::current;
use crate::process::{self, Pager};
use crate::thread;
use crate::channel;
use crate::ipc::{Msg, Header, Payload};
use crate::allocator::Ref;
use crate::utils::{VAddr, PAddr};
use crate::channel::{CId, Channel};
use crate::memory::{VmArea, PageFlags};

const INVALID_MESSAGE:         u16 = 0x0001;
const PUTCHAR_REQUEST:         u16 = 0x0401;
const PUTCHAR_RESPONSE:        u16 = 0x0481;
const CREATE_PROCESS_REQUEST:  u16 = 0x0102;
const CREATE_PROCESS_RESPONSE: u16 = 0x0182;
const SPAWN_THREAD_REQUEST:    u16 = 0x0103;
const SPAWN_THREAD_RESPONSE:   u16 = 0x0183;
const ADD_PAGER_REQUEST:       u16 = 0x0104;
const ADD_PAGER_RESPONSE:      u16 = 0x0184;
const FILL_PAGE_REQUEST:       u16 = 0x0301;
//const FILL_PAGE_RESPONSE:      u16 = 0x0181;

const MSG_SEND: u8 = 1 << 0;
const MSG_RECV: u8 = 1 << 1;

enum PayloadType {
    Inline = 0,
}

struct MsgBuilder(Msg);
impl MsgBuilder {
    pub fn new() -> MsgBuilder {
        MsgBuilder(Msg {
            from: CId::from_isize(0),
            header: 0,
            p0: 0,
            p1: 0,
            p2: 0,
            p3: 0,
            p4: 0,
        })
    }

    pub fn channel(&mut self, cid: CId) -> &mut MsgBuilder {
        self.0.header = self.0.header | (cid.as_isize() as u64) << 48;
        self
    }

    pub fn id(&mut self, id: u16) -> &mut MsgBuilder {
        self.0.header = self.0.header | id as u64;
        self
    }

    pub fn flags(&mut self, flags: u8) -> &mut MsgBuilder {
        self.0.header = self.0.header | ((flags as u64) << 26);
        self
    }

    pub fn payload(&mut self, index: usize, t: PayloadType, p: Payload) -> &mut MsgBuilder {
        self.0.header |= (t as u64) << (16 + index * 2);

        match index {
            0 => self.0.p0 = p,
            1 => self.0.p1 = p,
            2 => self.0.p2 = p,
            3 => self.0.p3 = p,
            4 => self.0.p4 = p,
            _ => unreachable!()
        }

        self
    }

    pub fn build(self) -> Msg {
        self.0
    }
}

macro_rules! error_message {
    ($id:expr) => {{
        let mut builder = MsgBuilder::new();
        builder.id($id);
        builder.build()
    }};
}

pub fn handle(header: Header, p0: Payload, p1: Payload, p2: Payload,
    p3: Payload, _p4: Payload, src: Ref<Channel>) ->Msg
{
    if !header.send() || !header.recv() {
//        trace_user!("kernel: invalid flags");
        return error_message!(INVALID_MESSAGE);
    }

    match header.msg_id() {
        PUTCHAR_REQUEST => {
            arch::printchar(p0 as u8 as char);
            let mut builder = MsgBuilder::new();
            builder.id(PUTCHAR_RESPONSE);
            builder.build()
        }
        CREATE_PROCESS_REQUEST => {
//            trace_user!("kernel: create_process()");
            let proc = process::Process::new();
            let ch = unwrap_or_return!(proc.channels().alloc(),
                error_message!(INVALID_MESSAGE));
            let pager_ch = unwrap_or_return!(current().process().channels().alloc(),
                error_message!(INVALID_MESSAGE));
            channel::link(ch, pager_ch);

//            trace_user!("kernel: create_process_response(pid={})", proc.pid());
            let mut builder = MsgBuilder::new();
            builder
                .id(CREATE_PROCESS_RESPONSE)
                .payload(0, PayloadType::Inline, proc.pid().as_isize() as Payload)
                .payload(1, PayloadType::Inline, pager_ch.cid().as_isize() as Payload);
            builder.build()
        }
        SPAWN_THREAD_REQUEST => {
            let mut builder = MsgBuilder::new();
            let pid = process::PId::from_isize(p0 as isize);
            let start = VAddr::new(p1 as usize);
//            trace_user!("kernel: spawn_thread(pid={}, start={:x})", pid, start.as_usize());
            if let Some(proc) = process::get_process_by_pid(&pid) {
                let t = thread::Thread::new(proc, start, VAddr::new(0), 0);
                t.set_state(thread::ThreadState::Runnable);
                thread::enqueue(t);

//                trace_user!("kernel: spawn_thread_response(tid={})", t.tid());
                builder
                    .id(SPAWN_THREAD_RESPONSE)
                    .payload(0, PayloadType::Inline, t.tid().as_isize() as Payload);
                builder.build()
            } else {
                error_message!(INVALID_MESSAGE)
            }
        }
        ADD_PAGER_REQUEST => {
            let pid = process::PId::from_isize(p0 as isize);
            let addr = VAddr::new(p1 as usize);
            let len = p2 as usize;
            let mut flags = PageFlags::from_u8(p3 as u8);
            flags.set_user();

//            trace_user!("kernel: add_pager(pid={}, start={:x}, len={:x})", pid, addr.as_usize(), len);
            if let Some(proc) = process::get_process_by_pid(&pid) {
                let pager = Pager::UserPager(src);
                proc.add_vmarea(VmArea::new(addr, len, flags, pager));

//                trace_user!("kernel: add_pager_response()");
                let mut builder = MsgBuilder::new();
                builder.id(ADD_PAGER_RESPONSE);
                builder.build()
            } else {
                error_message!(INVALID_MESSAGE)
            }
        }
        _ => {
            // trace_user!("kernel: invalid ID: {:x}", header.msg_id());
            error_message!(INVALID_MESSAGE)
        }
    }
}

pub fn user_pager(pager: &Ref<Channel>, addr: VAddr) -> PAddr {
    trace_user!("user pager={}, addr={:x}", pager.cid().as_isize(), addr.as_usize());
    let mut builder = MsgBuilder::new();
    builder.id(FILL_PAGE_REQUEST);
    builder.flags(MSG_SEND | MSG_RECV);
    builder.payload(0, PayloadType::Inline, current().process().pid().as_isize() as Payload);
    builder.payload(1, PayloadType::Inline, addr.as_usize() as Payload);
    builder.channel(pager.cid());
    let m = builder.build();

    let r = kernel_ipc_call(m);
    trace_user!("Ok, got a page addr={:x}", r.p0);
    VAddr::new(r.p0 as usize).paddr()
}

#[inline(never)]
fn kernel_ipc_call(m: Msg) -> Msg {
    // TODO:
    let builder = MsgBuilder::new();
    let mut r = builder.build();
    unsafe {
        asm!(
            "call ipc_handler"
            : "={rdi}"(r.header), "={rsi}"(r.p0), "={rdx}"(r.p1), "={rcx}"(r.p2), "={r8}"(r.p3), "={r9}"(r.p4)
            : "{rdi}"(m.header), "{rsi}"(m.p0), "{rdx}"(m.p1), "{rcx}"(m.p2), "{r8}"(m.p3), "{r9}"(m.p4)
        );
    }

    r
}