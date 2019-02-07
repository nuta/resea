#![no_std]
#![feature(asm)]
#![feature(alloc)]
#![feature(link_args)]

#[macro_use]
extern crate resea;
extern crate alloc;

mod elf;
mod initfs;
mod page;

use resea::channel::{Channel, CId};
use resea::message::Page;
use resea::idl;
use resea::syscalls;
use crate::elf::Elf;
use crate::page::{PAGE_SIZE, alloc_page};

extern "C" {
    static initfs_header: u8;
}

struct MemMgrServer {
    ch: Channel,
    programs: BTreeMap<resea::idl::kernel::pid, Program>,
}

impl MemMgrServer {
    pub fn new() -> MemMgrServer {
        MemMgrServer {
            ch: Channel::create().unwrap(),
            programs: BTreeMap::new(),
        }
    }
}

impl idl::memmgr::Server for MemMgrServer {
    fn nop(&mut self) -> syscalls::Result<()> {
        Ok(())
    }
}

impl idl::putchar::Server for MemMgrServer {
    fn putchar(&mut self, ch: usize) -> syscalls::Result<()> {
        print!("{}", ch as u8 as char);
        Ok(())
    }
}

impl idl::pager::Server for MemMgrServer {
    fn fill_page(&mut self, pid: isize, addr: usize) -> syscalls::Result<Page> {
        println!("memmgr: fill_page(#{}, addr={:08x})", pid, addr);
        let prog = match self.programs.get(&pid) {
            Some(prog) => prog,
            None => unimplemented!()
        };

        if addr >= 0x0300_0000 {
            let ptr: *mut u8 = alloc_page();
            unsafe {
                core::ptr::write_bytes(ptr, 0, PAGE_SIZE);
            }
            let page = Page::new(ptr, PAGE_SIZE);
            println!("return anon page={:p}", ptr);
            return Ok(page);
        }

        for segment in &prog.elf.program_headers {
            unsafe { println!("segment: addr={:08x}, base={:08x}, len={}", addr, segment.p_vaddr, segment.p_memsz) };
            if segment.p_vaddr <= addr as u64 && addr as u64 <= segment.p_vaddr + segment.p_memsz {
                let file_offset = segment.p_offset as usize + (addr - segment.p_vaddr as usize);

                let ptr: *mut u8 = alloc_page();
                let data = &prog.file.content[file_offset..];

                let copy_len = core::cmp::min(PAGE_SIZE, prog.file.content.len() - file_offset);
                unsafe {
                    core::ptr::copy_nonoverlapping(data.as_ptr(), ptr, copy_len);
                }

                let page = Page::new(ptr, PAGE_SIZE);
                println!("return page={:p}", ptr);
                return Ok(page);
            }
        }

        unimplemented!()
    }
}

use alloc::collections::BTreeMap;

struct Program {
    elf: elf::Elf,
    file: initfs::File,
}

fn main() {
    println!("memmgr: starting");
    let mut server = MemMgrServer::new();

    // Load initfs.
    let fs = initfs::InitFs::new(unsafe { &initfs_header as *const u8 });

    // Launch servers.
    for (name, file) in fs.files().iter() {
        if name.starts_with("startups/") {
            println!("starting {}", name);
            let elf = Elf::new(file.content);
            let mut kernel = resea::idl::kernel::Client::from_channel(Channel::from_cid(CId::new(1)));

            println!("creating process...");
            let (proc, pager_cid) = kernel.create_process().expect("failed to create a process");
            let pager = Channel::from_cid(CId::new(pager_cid));
            pager.transfer_to(&server.ch).unwrap();

            println!("registering pagers...");
            // TODO: remove hardcoded values
            // The executable image.
            kernel.add_pager(proc, 0x0010_0000, 0x20000, 0x06).ok();
            // The zeroed pages (heap).
            kernel.add_pager(proc, 0x0300_0000, 0x20000, 0x06).ok();

            println!("spawning a thread...");
            kernel.spawn_thread(proc, elf.entry as usize).expect("failed to create a thread");

            server.programs.insert(proc, Program { elf, file: file.clone() });
        }
    }

    println!("memmgr: ready");
    serve_forever!(&mut server, [memmgr, putchar, pager]);
}
