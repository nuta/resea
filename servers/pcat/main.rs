use resea::channel::Channel;
use resea::message::Notification;
use crate::screen::Screen;
use crate::keyboard::Keyboard;

static _MEMMGR_SERVER: Channel = Channel::from_cid(1);
static KERNEL_SERVER: Channel = Channel::from_cid(2);

struct Server {
    ch: Channel,
    screen: Screen,
    keyboard: Keyboard,
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
       }
    }
}

impl resea::server::Server for Server {
    fn notification(&mut self, _notification: Notification) {
        self.keyboard.read_input();
    }
}

#[no_mangle]
pub fn main() {
    info!("starting...");
    let mut server = Server::new();
    server.screen.clear();
    server.screen.print_str("Hello world from pcat driver!");
    serve_forever!(&mut server, []);
}
