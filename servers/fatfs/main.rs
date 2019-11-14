use resea::result::Error;
use resea::server::ServerResult;
use resea::message::{HandleId, Page};
use resea::idl;
use resea::channel::Channel;
use resea::std::string::String;
use resea::collections::HashMap;
use resea::utils::align_up;
use resea::PAGE_SIZE;
use crate::fat::{FileSystem, File};

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
    fn open(&mut self, _from: &Channel, path: String) -> ServerResult<HandleId> {
        match self.fs.open_file(&path) {
            Some(file) => {
                // TODO: Support freeing and reusing handle IDs.
                let handle_id = self.next_handle_id;
                self.next_handle_id += 1;

                self.opened_files.insert(handle_id, file);
                ServerResult::Ok(handle_id)
            }
            None => {
                ServerResult::Err(Error::NotFound)
            }
        }
    }

    fn close(&mut self, _from: &Channel, _handle: HandleId) -> ServerResult<()> {
        // TODO:
        ServerResult::Err(Error::Unimplemented)
    }

    fn read(&mut self, _from: &Channel, file: HandleId, offset: usize, len: usize) -> ServerResult<Page> {
        let file = match self.opened_files.get(&file) {
            Some(file) => file,
            None => return ServerResult::Err(Error::InvalidHandle),
        };

        use idl::memmgr::Client;
        let mut page = MEMMGR_SERVER.alloc_pages(align_up(len, PAGE_SIZE) / PAGE_SIZE).unwrap();
        file.read(&self.fs, page.as_bytes_mut(), offset, len).unwrap();
        ServerResult::Ok(page)
    }

    fn write(&mut self, _from: &Channel, _file: HandleId, _page: Page, _len: usize) -> ServerResult<()> {
        // TODO:
        ServerResult::Err(Error::Unimplemented)
    }
}

impl idl::server::Server for Server {
    fn connect(&mut self, _from: &Channel, interface_id: u8) -> ServerResult<Channel> {
        assert_eq!(interface_id, idl::fs::INTERFACE_ID);
        let ch = Channel::create().unwrap();
        ch.transfer_to(&self.ch).unwrap();
        ServerResult::Ok(ch)
    }
}

impl resea::server::Server for Server {
}

#[no_mangle]
pub fn main() {
    info!("starting...");

    use idl::discovery::Client;
    let storage_device =
        MEMMGR_SERVER.connect(idl::storage_device::INTERFACE_ID)
            .expect("failed to connect to a storage_device server");

    let fs = FileSystem::new(storage_device, 0)
        .expect("failed to load the file system");

    let mut server = Server::new(fs);

    // let ch = Channel::create().unwrap();
    // ch.transfer_to(&server.ch).unwrap();
    // idl::discovery::Client::publish(&MEMMGR_SERVER, idl::fs::INTERFACE_ID, ch).unwrap();

    serve_forever!(&mut server, [server, fs]);
}
