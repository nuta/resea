use resea::error::Error;
use resea::channel::Channel;
use resea::idl;
use resea::std::borrow::ToOwned;
use resea::collections::HashMap;
use crate::initfs::File;
use resea::message::HandleId;
use crate::elf::ELF;

pub struct Process {
    pub pid: HandleId,
    pub elf: ELF<'static>,
    pub file: &'static File,
    pub pager_ch: Channel,
}

impl Process {
    pub fn start(&self, thread_server: &Channel) -> Result<(), Error> {
        idl::thread::Client::spawn(thread_server, self.pid, self.elf.entry,
            APP_INITIAL_STACK_POINTER, THREAD_INFO_ADDR, 0 /* arg */)?;
        Ok(())
    }
}

pub struct ProcessManager {
    process_server: &'static Channel,
    processes: HashMap<HandleId, Process>,
}

// TODO: Move to arch.
const THREAD_INFO_ADDR: usize = 0x0000000000000f1b000;
const APP_IMAGE_START: usize = 0x01000000;
const APP_IMAGE_SIZE: usize = 0x01000000;
const APP_ZEROED_PAGES_START: usize = 0x02000000;
const APP_ZEROED_PAGES_SIZE: usize = 0x02000000;
const APP_INITIAL_STACK_POINTER: usize = 0x03000000;

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

    pub fn create(&mut self, file: &'static File) -> Result<HandleId, Error> {
        let elf = ELF::parse(file.data())?;
        let kernel_ch = idl::server::Client::connect(self.process_server, 0)?;

        let (proc, pager_cid) =
            idl::process::Client::create(self.process_server, file.path().to_owned())?;
        let pager_ch = Channel::from_cid(pager_cid);

        idl::process::Client::send_channel(self.process_server, proc, kernel_ch)?;
        idl::process::Client::add_pager(self.process_server, proc, 1,
            APP_IMAGE_START, APP_IMAGE_SIZE, 0x06)?;
        idl::process::Client::add_pager(self.process_server, proc, 1,
            APP_ZEROED_PAGES_START, APP_ZEROED_PAGES_SIZE, 0x06)?;

        self.processes.insert(proc, Process { pid: proc, elf, file, pager_ch });
        Ok(proc)
    }
}
