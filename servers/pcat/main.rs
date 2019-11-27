use crate::keyboard::Keyboard;
use crate::screen::Screen;
use resea::idl::{self, keyboard_device, text_screen_device};
use resea::prelude::*;
use resea::server::{publish_server, DeferredWorkResult, ServerResult};

static _MEMMGR_SERVER: Channel = Channel::from_cid(1);
static KERNEL_SERVER: Channel = Channel::from_cid(2);

struct Server {
    ch: Channel,
    screen: Screen,
    keyboard: Keyboard,
    keyboard_listener: Option<Channel>,
}

impl Server {
    pub fn new() -> Server {
        let ch = Channel::create().unwrap();
        let screen = Screen::new(&KERNEL_SERVER);
        let keyboard = Keyboard::new(&ch, &KERNEL_SERVER);
        Server {
            ch,
            screen,
            keyboard,
            keyboard_listener: None,
        }
    }
}

impl text_screen_device::Server for Server {
    fn print_str(&mut self, _from: &Channel, str: &str) -> ServerResult<()> {
        self.screen.print_str(str);
        ServerResult::Ok(())
    }

    fn print_char(&mut self, _from: &Channel, ch: u8) -> ServerResult<()> {
        self.screen.print_char(ch as char);
        ServerResult::Ok(())
    }
}

impl keyboard_device::Server for Server {
    fn listen(&mut self, _from: &Channel, ch: Channel) -> ServerResult<()> {
        self.keyboard_listener = Some(ch);
        ServerResult::Ok(())
    }
}

impl idl::server::Server for Server {
    fn connect(
        &mut self,
        _from: &Channel,
        interface: InterfaceId,
    ) -> ServerResult<(InterfaceId, Channel)> {
        assert!(
            interface == keyboard_device::INTERFACE_ID
                || interface == text_screen_device::INTERFACE_ID
        );
        let client_ch = Channel::create().unwrap();
        client_ch.transfer_to(&self.ch).unwrap();
        ServerResult::Ok((interface, client_ch))
    }
}

impl resea::server::Server for Server {
    fn deferred_work(&mut self) -> DeferredWorkResult {
        if let Some(ref listener) = self.keyboard_listener {
            while let Some(ch) = self.keyboard.buffer().front() {
                use resea::idl::keyboard_device_client::nbsend_pressed;
                match nbsend_pressed(listener, *ch) {
                    Err(Error::NeedsRetry) => {
                        return DeferredWorkResult::NeedsRetry;
                    }
                    Err(_) => {
                        panic!("failed to send a key event");
                    }
                    Ok(_) => {
                        self.keyboard.buffer().pop_front();
                    }
                }
            }
        }

        DeferredWorkResult::Done
    }

    fn notification(&mut self, _notification: Notification) {
        self.keyboard.read_input();
    }
}

#[no_mangle]
pub fn main() {
    info!("starting...");
    let mut server = Server::new();
    server.screen.clear();

    publish_server(text_screen_device::INTERFACE_ID, &server.ch).unwrap();
    publish_server(keyboard_device::INTERFACE_ID, &server.ch).unwrap();

    info!("ready");
    serve_forever!(&mut server, [server, text_screen_device, keyboard_device]);
}
