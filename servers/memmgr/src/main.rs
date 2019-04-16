#![no_std]
#![feature(asm)]
#![feature(alloc)]
#![feature(alloc_prelude)]
#![feature(link_args)]

#[macro_use]
extern crate resea;
extern crate alloc;

mod elf;
mod initfs;
mod page;

use alloc::vec::Vec;
use alloc::collections::BTreeMap;
use alloc::string::ToString;
use resea::channel::{Channel, CId};
use resea::message::{Page, FixedString};
use resea::idl;
use resea::idl::memmgr;
use resea::idl::kernel::{CreateProcessResponseMsg, CreateThreadResponseMsg};
use resea::syscalls;
use crate::elf::Elf;
use crate::page::{PAGE_SIZE, alloc_page};

extern "C" {
    static initfs_header: u8;
}

struct Program {
    elf: elf::Elf,
    file: initfs::File,
    thread: idl::kernel::tid,
}

struct MemMgrServer {
    programs: BTreeMap<idl::kernel::pid, Program>,
    initfs: initfs::InitFs,
    server_ch: Channel,
    kernel_server: idl::kernel::Client,
}

impl MemMgrServer {
    pub fn new(server_ch: Channel, initfs: initfs::InitFs) -> MemMgrServer {
        MemMgrServer {
            programs: BTreeMap::new(),
            kernel_server: idl::kernel::Client::from_raw_cid(1),
            server_ch,
            initfs,
        }
    }

    fn create_initfs_process(&mut self, path: &str) -> syscalls::Result<idl::kernel::pid> {
        let file = match self.find_file(path) {
            Some(file) => file,
            None => unimplemented!() /* TODO: */,
        };

        println!("creating process...");
        let elf = Elf::new(file.content);
        let r = self.kernel_server.create_process()?;
        let CreateProcessResponseMsg { pid: proc, pager: pager_cid, .. } = r;
        let pager = Channel::from_cid(CId::new(pager_cid));
        pager.transfer_to(&self.server_ch)?;

        println!("registering pagers...");
        // TODO: remove hardcoded values
        // The executable image.
        self.kernel_server.add_pager(proc, 1, 0x0010_0000, 0x0012_0000, 0x06)?;
        // The zeroed pages (heap).
        self.kernel_server.add_pager(proc, 1, 0x0300_0000, 0x0400_0000, 0x06)?;

        // The driver servers requires direct access to the kernel to call
        // `self.kernel_server.allow_io`.
        println!("connecting to the kernel server...");
        let kernel_cid = self.kernel_server.create_kernel_channel(proc)?.cid;
        assert!(kernel_cid == 2);

        println!("spawning a thread...");
        let CreateThreadResponseMsg { tid, .. } = self.kernel_server.create_thread(
            proc, elf.entry as usize, 0 /* stack */, 0x1000, 0)?;

        self.programs.insert(proc, Program { elf, thread: tid, file: file.clone() });

        Ok(proc)
    }

    fn start_initfs_process(&mut self, pid: idl::kernel::pid) -> syscalls::Result<()> {
        if let Some(Program { thread, .. }) = self.programs.get(&pid) {
            self.kernel_server.start_thread(*thread)?;
        }

        Ok(())
    }


    fn find_file(&self, path: &str) -> Option<initfs::File> {
        for (name, file) in self.initfs.files().iter() {
            if name == path {
                return Some(*file)
            }
        }

        None
    }
}

impl memmgr::Server for MemMgrServer {
    fn nop(&mut self) -> syscalls::Result<()> {
        Ok(())
    }

    fn create_process_from_initfs(&mut self, string: FixedString) -> syscalls::Result<memmgr::pid> {
        if let Ok(string) = string.as_str() {
            let pid = self.create_initfs_process(string)?;
            return Ok(pid);
        }

        unimplemented!();
    }

    fn start_process(&mut self, pid: memmgr::pid) -> syscalls::Result<()> {
        self.start_initfs_process(pid)
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

fn main() {
    println!("memmgr: starting");

    // Load initfs.
    let fs = initfs::InitFs::new(unsafe { &initfs_header as *const u8 });

    let mut startup_files = Vec::new();
    for (name, _) in fs.files().iter() {
        if name.starts_with("startups/") {
            startup_files.push(name.to_string());
        }
    }

    // Launch servers.
    let mut server_ch = Channel::create().unwrap();
    let mut server = MemMgrServer::new(server_ch.clone(), fs);
    for path in startup_files {
        let pid = server.create_initfs_process(&path)
            .expect("failed to create a process");
        server.start_initfs_process(pid)
            .expect("failed to start a process");
    }

    println!("memmgr: ready");
    serve_forever!(&mut server, &mut server_ch, [memmgr, putchar, pager]);
}
