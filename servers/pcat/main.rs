use crate::keyboard::Keyboard;
use crate::screen::Screen;
use resea::channel::Channel;
use resea::idl::{keyboard_device, text_screen_device};
use resea::message::Notification;
use resea::server::{publish_server, ServerResult};
use resea::std::string::String;

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
    fn print_str(&mut self, _from: &Channel, str: String) -> ServerResult<()> {
        self.screen.print_str(&str);
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

impl resea::server::Server for Server {
    fn deferred_work(&mut self) {
        if let Some(ref listener) = self.keyboard_listener {
            use resea::idl::keyboard_device::send_pressed;
            while let Some(ch) = self.keyboard.buffer().pop_front() {
                send_pressed(listener, ch).ok();
            }
        }
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
    serve_forever!(&mut server, [text_screen_device, keyboard_device]);
}
