use crate::elf::ELF;
use crate::initfs::File;
use resea::allocator::AllocatedPage;
use resea::channel::Channel;
use resea::collections::HashMap;
use resea::idl;
use resea::idl::kernel::{call_add_vm_area, call_create_process, call_inject_channel};
use resea::message::HandleId;
use resea::result::{Error, Result};
use resea::std::cmp::min;
use resea::std::ptr;
use resea::PAGE_SIZE;

pub struct Process {
    pub pid: HandleId,
    pub elf: ELF<'static>,
    pub file: &'static File,
    pub pager_ch: Channel,
}

impl Process {
    pub fn start(&self, thread_server: &Channel) -> Result<()> {
        idl::kernel::call_spawn_thread(
            thread_server,
            self.pid,
            self.elf.entry,
            APP_INITIAL_STACK_POINTER,
            THREAD_INFO_ADDR,
            0, /* arg */
        )?;
        Ok(())
    }
}

pub struct ProcessManager {
    process_server: &'static Channel,
    processes: HashMap<HandleId, Process>,
}

// TODO: Move to arch.
const THREAD_INFO_ADDR: usize = 0x00f1_b000;
const APP_IMAGE_START: usize = 0x0100_0000;
const APP_IMAGE_SIZE: usize = 0x0100_0000;
const APP_ZEROED_PAGES_START: usize = 0x0200_0000;
const APP_ZEROED_PAGES_SIZE: usize = 0x0200_0000;
const APP_INITIAL_STACK_POINTER: usize = 0x0300_0000;

impl ProcessManager {
    pub fn new(process_server: &'static Channel) -> ProcessManager {
        ProcessManager {
            process_server,
            processes: HashMap::new(),
        }
    }

    pub fn get(&self, pid: HandleId) -> Option<&Process> {
        self.processes.get(&pid)
    }

    pub fn create(&mut self, file: &'static File) -> Result<HandleId> {
        let elf = ELF::parse(file.data())?;
        let (_, kernel_ch) = idl::server::call_connect(self.process_server, 0)?;

        let (proc, pager_ch) = call_create_process(self.process_server, file.path())?;

        call_inject_channel(self.process_server, proc, kernel_ch)?;
        call_add_vm_area(
            self.process_server,
            proc,
            APP_IMAGE_START,
            APP_IMAGE_SIZE,
            0x06,
        )?;
        call_add_vm_area(
            self.process_server,
            proc,
            APP_ZEROED_PAGES_START,
            APP_ZEROED_PAGES_SIZE,
            0x06,
        )?;

        self.processes.insert(
            proc,
            Process {
                pid: proc,
                elf,
                file,
                pager_ch,
            },
        );
        Ok(proc)
    }

    pub fn try_filling_page(
        &self,
        page: &mut AllocatedPage,
        proc: &Process,
        addr: usize,
    ) -> Result<()> {
        let addr = addr as u64;
        let file_size = proc.file.len() as u64;

        // Look for the segment for `addr`.
        for phdr in proc.elf.phdrs {
            if phdr.p_vaddr <= addr && addr < phdr.p_vaddr + phdr.p_memsz {
                let offset = addr - phdr.p_vaddr;
                let fileoff = phdr.p_offset + offset;
                if fileoff >= file_size {
                    // The file offset is beyond the file size. Ignore the
                    // segment for now.
                    continue;
                }

                // Found the appropriate segment. Fill the page with the file
                // contents.
                let copy_len = min(
                    min(PAGE_SIZE as u64, file_size - fileoff),
                    phdr.p_filesz - offset,
                ) as usize;
                unsafe {
                    let src = proc.file.data().as_ptr().add(fileoff as usize);
                    let dst = page.as_mut_ptr();
                    ptr::copy_nonoverlapping(src, dst, copy_len);
                }

                return Ok(());
            }
        }

        Err(Error::NotFound)
    }
}
