use crate::initfs::{File, Initfs};
use crate::process::ProcessManager;
use resea::allocator::PageAllocator;
use resea::channel::Channel;
use resea::collections::{HashMap, Vec};
use resea::idl;
use resea::message::{HandleId, InterfaceId, Message, Page};
use resea::result::{Error, Result};
use resea::server::{DeferredWorkResult, ServerResult};
use resea::std::cmp::min;
use resea::std::ptr;
use resea::std::string::String;
use resea::PAGE_SIZE;

extern "C" {
    static __initfs: Initfs;
}

static KERNEL_SERVER: Channel = Channel::from_cid(1);

struct RegisteredServer {
    server_ch: Channel,
    client_ch: Option<Channel>,
}

struct ConnectRequest {
    interface: InterfaceId,
    ch: Channel,
}

struct Server {
    ch: Channel,
    _initfs: &'static Initfs,
    process_manager: ProcessManager,
    page_allocator: PageAllocator,
    servers: HashMap<InterfaceId, RegisteredServer>,
    connect_requests: Vec<ConnectRequest>,
}

const FREE_MEMORY_START: usize = 0x0400_0000;
const FREE_MEMORY_SIZE: usize = 0x1000_0000;

impl Server {
    pub fn new(initfs: &'static Initfs) -> Server {
        Server {
            ch: Channel::create().unwrap(),
            _initfs: initfs,
            process_manager: ProcessManager::new(&KERNEL_SERVER),
            page_allocator: PageAllocator::new(FREE_MEMORY_START, FREE_MEMORY_SIZE),
            servers: HashMap::new(),
            connect_requests: Vec::new(),
        }
    }

    /// Launches all server executables in `startups` directory.
    pub fn launch_servers(&mut self, initfs: &Initfs) {
        let mut dir = initfs.open_dir();
        while let Some(file) = dir.readdir() {
            info!(
                "initfs: path='{}', len={}KiB",
                file.path(),
                file.len() / 1024
            );
            if file.path().starts_with("startups/") {
                self.execute(file).expect("failed to launch a server");
            }
        }
    }

    pub fn execute(&mut self, file: &'static File) -> Result<()> {
        let pid = self.process_manager.create(file)?;
        let proc = self.process_manager.get(pid).unwrap();
        proc.pager_ch.transfer_to(&self.ch)?;
        proc.start(&KERNEL_SERVER)?;
        Ok(())
    }
}

impl idl::pager::Server for Server {
    fn fill(
        &mut self,
        _from: &Channel,
        pid: HandleId,
        addr: usize,
        num_pages: usize,
    ) -> ServerResult<Page> {
        // TODO: Support filling multiple pages.
        assert_eq!(num_pages, 1);

        // Search the process table. It should never fail.
        let proc = self.process_manager.get(pid).unwrap();

        // Look for the segment for `addr`.
        let mut page = self.page_allocator.allocate(1);
        let mut filled_page = false;
        let addr = addr as u64;
        let file_size = proc.file.len() as u64;
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
                filled_page = true;
                break;
            }
        }

        if !filled_page {
            // `addr` is not in the ELF segments. It should be a zeroed pages
            // such as stack and heap. Fill the page with zeros.
            unsafe {
                ptr::write_bytes(page.as_mut_ptr(), 0, PAGE_SIZE);
            }
        }

        ServerResult::Ok(page.as_page_payload())
    }
}

impl idl::memmgr::Server for Server {
    fn alloc_pages(&mut self, _from: &Channel, num_pages: usize) -> ServerResult<Page> {
        if num_pages == 0 {
            return ServerResult::Err(Error::InvalidArg);
        }

        let page = self.page_allocator.allocate(num_pages);
        ServerResult::Ok(page.as_page_payload())
    }

    fn alloc_phy_pages(
        &mut self,
        _from: &Channel,
        num_pages: usize,
    ) -> ServerResult<(usize, Page)> {
        if num_pages == 0 {
            return ServerResult::Err(Error::InvalidArg);
        }

        let page = self.page_allocator.allocate(num_pages);
        ServerResult::Ok((page.addr, page.as_page_payload()))
    }

    fn map_phy_pages(
        &mut self,
        _from: &Channel,
        paddr: usize,
        num_pages: usize,
    ) -> ServerResult<Page> {
        // TODO: Check whether the given paddr is already allocated.
        if paddr == 0 || num_pages == 0 {
            return ServerResult::Err(Error::InvalidArg);
        }

        ServerResult::Ok(Page::new(paddr, num_pages))
    }
}

impl idl::runtime::Server for Server {
    fn exit(&mut self, _from: &Channel, _code: i32) -> ServerResult<()> {
        unimplemented!();
    }

    fn printchar(&mut self, _from: &Channel, ch: u8) -> ServerResult<()> {
        resea::print::printchar(ch);
        ServerResult::Ok(())
    }

    fn print_str(&mut self, _from: &Channel, s: String) -> ServerResult<()> {
        resea::print::print_str(s.as_str());
        ServerResult::Ok(())
    }
}

impl idl::timer::Server for Server {
    fn create(
        &mut self,
        _from: &Channel,
        ch: Channel,
        initial: i32,
        interval: i32,
    ) -> ServerResult<HandleId> {
        use idl::timer::Client;
        // FIXME: Don't use unwrap
        ServerResult::Ok(KERNEL_SERVER.create(ch, initial, interval).unwrap())
    }

    fn reset(
        &mut self,
        _from: &Channel,
        timer: HandleId,
        initial: i32,
        interval: i32,
    ) -> ServerResult<()> {
        use idl::timer::Client;
        ServerResult::Ok(KERNEL_SERVER.reset(timer, initial, interval).unwrap())
    }

    fn clear(&mut self, _from: &Channel, timer: HandleId) -> ServerResult<()> {
        use idl::timer::Client;
        ServerResult::Ok(KERNEL_SERVER.clear(timer).unwrap())
    }
}

impl idl::discovery::Server for Server {
    fn connect(&mut self, from: &Channel, interface: u8) -> ServerResult<Channel> {
        warn!("accepted a connect request = {}", interface);
        self.connect_requests.push(ConnectRequest {
            interface,
            ch: unsafe { from.clone() },
        });

        ServerResult::NoReply
    }

    fn publish(&mut self, _from: &Channel, interface: u8, ch: Channel) -> ServerResult<()> {
        // TODO: Support multiple servers with the same interface ID.
        assert!(self.servers.get(&interface).is_none());

        warn!("accepted a new server = {}", interface);
        ch.transfer_to(&self.ch).unwrap();
        let server = RegisteredServer {
            server_ch: ch,
            client_ch: None,
        };

        self.servers.insert(interface, server);
        ServerResult::Ok(())
    }
}

impl resea::server::Server for Server {
    fn deferred_work(&mut self) -> DeferredWorkResult {
        let mut pending_requests = Vec::with_capacity(self.connect_requests.len());
        let mut result = DeferredWorkResult::Done;
        for request in self.connect_requests.drain(..) {
            let needs_retry = match self.servers.get_mut(&request.interface) {
                Some(server) => {
                    match server.client_ch.take() {
                        Some(client_ch) => {
                            // Now we have a valid client channel. Try send it to the
                            // awaiting client...
                            warn!("sending a connect_reply");
                            match idl::discovery::nbsend_connect_reply(&request.ch, client_ch) {
                                Ok(_) => false,
                                Err(Error::NeedsRetry) => {
                                    warn!("failed to send a connect_reply");
                                    true
                                }
                                Err(_) => {
                                    warn!("an unexpected error occurred during sending a discovery.connect_reply");
                                    false
                                }
                            }
                        }
                        None => {
                            // Try sending a server.connect request to the registered
                            // server...
                            warn!("sending a connect request");
                            let reply =
                                idl::server::nbsend_connect(&server.server_ch, request.interface);
                            match reply {
                                Ok(()) => true,
                                Err(Error::NeedsRetry) => {
                                    // The server is not ready. Try later.
                                    warn!("failed to send a connect");
                                    true
                                }
                                Err(_) => {
                                    // The server returned an unexpected error.
                                    warn!("error occurred during receiving a server.connect");
                                    true
                                }
                            }
                        }
                    }
                }
                None => true,
            };

            if needs_retry {
                result = DeferredWorkResult::NeedsRetry;
                pending_requests.push(request);
            }
        }

        self.connect_requests = pending_requests;
        result
    }

    fn unknown_message(&mut self, m: &mut Message) -> bool {
        if m.header == idl::server::CONNECT_REPLY_MSG {
            let m = unsafe {
                resea::std::mem::transmute::<&mut Message, &mut idl::server::ConnectReplyMsg>(m)
            };

            // Successfully created a new client channel.
            info!(
                "received a server.connect_reply for interface={}",
                m.interface
            );
            match self.servers.get_mut(&m.interface) {
                Some(server) => server.client_ch = Some(Channel::from_cid(m.ch)),
                None => {
                    warn!("received server.connect_reply from an unexpected channel");
                }
            }
        }

        false
    }
}

#[no_mangle]
pub fn main() {
    info!("hello from memmgr!");
    let initfs = unsafe { &__initfs };

    info!("lauching servers...");
    let mut server = Server::new(initfs);
    server.launch_servers(initfs);

    info!("entering mainloop...");
    serve_forever!(&mut server, [runtime, pager, timer, memmgr, discovery]);
}
