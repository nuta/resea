use crate::initfs::{File, Initfs};
use crate::process::ProcessManager;
use resea::allocator::PageAllocator;
use resea::collections::HashMap;
use resea::idl;
use resea::prelude::*;
use resea::ptr;
use resea::mainloop::DeferredWorkResult;
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
    ) -> Result<Page> {
        // TODO: Support filling multiple pages.
        assert_eq!(num_pages, 1);

        // Search the process table. It should never fail.
        let proc = self.process_manager.get(pid).unwrap();

        let mut page = Page::from_addr(self.page_allocator.allocate(1), PAGE_SIZE);
        match self.process_manager.try_filling_page(&mut page, proc, addr) {
            Ok(()) => (),
            Err(Error::NotFound) => {
                // `addr` is not in the ELF segments. It should be a zeroed pages
                // such as stack and heap. Fill the page with zeros.
                //
                // TODO: Make sure that the address is in the range of
                // stack/heap.
                unsafe {
                    ptr::write_bytes::<u8>(page.as_mut_ptr(), 0, PAGE_SIZE);
                }
            }
            _ => unreachable!(),
        }

        Ok(page)
    }
}

impl idl::memmgr::Server for Server {
    fn alloc_pages(&mut self, _from: &Channel, num_pages: usize) -> Result<Page> {
        if num_pages == 0 {
            return Err(Error::NotAcceptable);
        }

        let addr = self.page_allocator.allocate(num_pages);
        let page = Page::from_addr(addr, num_pages * PAGE_SIZE);
        Ok(page)
    }

    fn alloc_phy_pages(
        &mut self,
        _from: &Channel,
        num_pages: usize,
    ) -> Result<(usize, Page)> {
        if num_pages == 0 {
            return Err(Error::NotAcceptable);
        }

        let addr = self.page_allocator.allocate(num_pages);
        Ok((addr, Page::from_addr(addr, num_pages * PAGE_SIZE)))
    }

    fn map_phy_pages(
        &mut self,
        _from: &Channel,
        paddr: usize,
        num_pages: usize,
    ) -> Result<Page> {
        // TODO: Check whether the given paddr is already allocated.
        if paddr == 0 || num_pages == 0 {
            return Err(Error::NotAcceptable);
        }

        Ok(Page::from_addr(paddr, num_pages * PAGE_SIZE))
    }
}

impl idl::runtime::Server for Server {
    fn exit(&mut self, _from: &Channel, _code: i32) -> Result<()> {
        unimplemented!();
    }

    fn printchar(&mut self, _from: &Channel, ch: u8) -> Result<()> {
        resea::print::printchar(ch);
        Ok(())
    }

    fn print_str(&mut self, _from: &Channel, string: &str) -> Result<()> {
        resea::print::print_str(string);
        Ok(())
    }
}

impl idl::timer::Server for Server {
    fn create(
        &mut self,
        _from: &Channel,
        ch: Channel,
        initial: i32,
        interval: i32,
    ) -> Result<HandleId> {
        use idl::timer::call_create;
        let handle = call_create(&KERNEL_SERVER, ch, initial, interval)?;
        Ok(handle)
    }

    fn reset(
        &mut self,
        _from: &Channel,
        timer: HandleId,
        initial: i32,
        interval: i32,
    ) -> Result<()> {
        use idl::timer::call_reset;
        Ok(call_reset(&KERNEL_SERVER, timer, initial, interval).unwrap())
    }

    fn clear(&mut self, _from: &Channel, timer: HandleId) -> Result<()> {
        use idl::timer::call_clear;
        Ok(call_clear(&KERNEL_SERVER, timer).unwrap())
    }
}

impl idl::discovery::Server for Server {
    fn connect(&mut self, from: &Channel, interface: u8) -> Result<Channel> {
        trace!("accepted a connect request = {}", interface);
        self.connect_requests.push(ConnectRequest {
            interface,
            ch: unsafe { from.clone() },
        });

        Err(Error::NoReply)
    }

    fn publish(&mut self, _from: &Channel, interface: u8, ch: Channel) -> Result<()> {
        // TODO: Support multiple servers with the same interface ID.
        assert!(self.servers.get(&interface).is_none());

        info!("publish a server = {}", interface);
        ch.transfer_to(&self.ch).unwrap();
        let server = RegisteredServer {
            server_ch: ch,
            client_ch: None,
        };

        self.servers.insert(interface, server);
        Ok(())
    }
}

impl idl::server::Client for Server {
    fn connect_reply(&mut self, _from: &Channel, interface: InterfaceId, ch: Channel) {
        // Successfully created a new client channel.
        match self.servers.get_mut(&interface) {
            Some(server) => server.client_ch = Some(ch),
            None => {
                warn!("received server.connect_reply from an unexpected channel");
            }
        }
    }
}

impl resea::mainloop::Mainloop for Server {
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
                            match idl::discovery::nbsend_connect_reply(&request.ch, client_ch) {
                                Ok(_) => false,
                                Err(Error::WouldBlock) => {
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
                            let reply =
                                idl::server::nbsend_connect(&server.server_ch, request.interface);
                            match reply {
                                Ok(()) | Err(Error::WouldBlock) => {},
                                Err(err) => {
                                    // The server returned an unexpected error.
                                    warn!("error occurred during sending a server.connect {}", err);
                                }
                            }
                            // We'll retry sending server.connect or send
                            // discovery.connect_reply later.
                            true
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
}

#[no_mangle]
pub fn main() {
    info!("hello from memmgr!");
    let initfs = unsafe { &__initfs };

    info!("lauching servers...");
    let mut server = Server::new(initfs);
    server.launch_servers(initfs);

    info!("entering mainloop...");
    mainloop!(&mut server, [runtime, pager, timer, memmgr, discovery], [server]);
}
