use crate::fat::{File, FileSystem};
use resea::collections::HashMap;
use resea::idl;
use resea::prelude::*;
use resea::server::{connect_to_server, publish_server};
use resea::utils::align_up;
use resea::PAGE_SIZE;

static MEMMGR_SERVER: Channel = Channel::from_cid(1);
static _KERNEL_SERVER: Channel = Channel::from_cid(2);

struct Server {
    ch: Channel,
    fs: FileSystem,
    opened_files: HashMap<HandleId, File>,
    next_handle_id: i32,
}

impl Server {
    pub fn new(fs: FileSystem) -> Server {
        Server {
            ch: Channel::create().unwrap(),
            fs,
            opened_files: HashMap::new(),
            next_handle_id: 1,
        }
    }
}

impl idl::fs::Server for Server {
    fn open(&mut self, _from: &Channel, path: &str) -> Result<HandleId> {
        match self.fs.open_file(&path) {
            Some(file) => {
                // TODO: Support freeing and reusing handle IDs.
                let handle_id = self.next_handle_id;
                self.next_handle_id += 1;
                self.opened_files.insert(handle_id, file);
                Ok(handle_id)
            }
            None => Err(Error::NotFound),
        }
    }

    fn close(&mut self, _from: &Channel, _handle: HandleId) -> Result<()> {
        // TODO:
        Err(Error::Unimplemented)
    }

    fn read(
        &mut self,
        _from: &Channel,
        file: HandleId,
        offset: usize,
        len: usize,
    ) -> Result<Page> {
        let file = match self.opened_files.get(&file) {
            Some(file) => file,
            None => return Err(Error::NotFound),
        };

        use idl::memmgr::call_alloc_pages;
        let mut page =
            call_alloc_pages(&MEMMGR_SERVER, align_up(len, PAGE_SIZE) / PAGE_SIZE).unwrap();
        file.read(&self.fs, page.as_bytes_mut(), offset, len)
            .unwrap();
        Ok(page)
    }

    fn write(
        &mut self,
        _from: &Channel,
        _file: HandleId,
        _page: Page,
        _len: usize,
    ) -> Result<()> {
        // TODO:
        Err(Error::Unimplemented)
    }
}

impl idl::server::Server for Server {
    fn connect(
        &mut self,
        _from: &Channel,
        interface: InterfaceId,
    ) -> Result<(InterfaceId, Channel)> {
        assert!(interface == idl::fs::INTERFACE_ID);
        let client_ch = Channel::create().unwrap();
        client_ch.transfer_to(&self.ch).unwrap();
        Ok((interface, client_ch))
    }
}

impl resea::server::Server for Server {}

#[no_mangle]
pub fn main() {
    info!("starting...");
    let storage_device = connect_to_server(idl::storage_device::INTERFACE_ID)
        .expect("failed to connect to a storage_device server");
    let fs =
        FileSystem::new(storage_device, 0 /* TODO: */).expect("failed to load the file system");
    let mut server = Server::new(fs);
    publish_server(idl::fs::INTERFACE_ID, &server.ch).unwrap();
    serve_forever!(&mut server, [server, fs]);
}
