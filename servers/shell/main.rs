use resea::channel::Channel;
use resea::idl;
use resea::message::Message;
use resea::server::connect_to_server;
use resea::std::string::String;

static _MEMMGR_SERVER: Channel = Channel::from_cid(1);
static _KERNEL_SERVER: Channel = Channel::from_cid(2);

struct Server {
    ch: Channel,
    screen: Channel,
    input: String,
}

impl Server {
    pub fn new() -> Server {
        // Connect to the dependent servers...
        let screen = connect_to_server(idl::text_screen_device::INTERFACE_ID)
            .expect("failed to connect to a text_screen_device server");
        let keyboard = connect_to_server(idl::keyboard_device::INTERFACE_ID)
            .expect("failed to connect to a keyboard_device server");

        // Print the welcome message.
        use crate::resea::std::borrow::ToOwned;
        use idl::text_screen_device::Client;
        screen.print_str("Resea Version \n".to_owned()).unwrap();
        screen
            .print_str("Type 'help' for usage.\n".to_owned())
            .unwrap();
        screen.print_str(">>> ".to_owned()).unwrap();

        // Start receiving keyboard inputs from the driver.
        use idl::keyboard_device::Client as KeyboardClient;
        let server_ch = Channel::create().unwrap();
        let listener_ch = Channel::create().unwrap();
        listener_ch.transfer_to(&server_ch).unwrap();
        keyboard.listen(listener_ch).unwrap();

        Server {
            ch: server_ch,
            screen,
            input: String::new(),
        }
    }

    fn run_command(&mut self) {
        if self.input.starts_with("help") {
            self.print_string("echo <string>   -  Prints a string.\n");
        } else if self.input.starts_with("echo ") {
            self.print_string(&self.input[5..]);
            self.print_string("\n");
        } else {
            self.print_string("Unknown command.\n");
        }
    }

    fn print_string(&self, string: &str) {
        use crate::resea::std::borrow::ToOwned;
        use idl::text_screen_device::Client;
        self.screen.print_str(string.to_owned()).unwrap();
    }

    fn print_prompt(&self) {
        use crate::resea::std::borrow::ToOwned;
        use idl::text_screen_device::Client;
        self.screen.print_str(">>> ".to_owned()).unwrap();
    }

    fn readchar(&mut self, ch: char) {
        use idl::text_screen_device::Client;
        self.screen.print_char(ch as u8).ok();
        if ch == '\n' {
            self.run_command();
            self.input.clear();
            self.print_prompt();
        } else {
            self.input.push(ch);
        }
    }
}

impl resea::server::Server for Server {
    #[allow(safe_packed_borrows)]
    fn unknown_message(&mut self, m: &mut Message) -> bool {
        // FIXME:
        if m.header == idl::keyboard_device::PRESSED_MSG {
            let m = unsafe {
                resea::std::mem::transmute::<&mut Message, &mut idl::keyboard_device::PressedMsg>(m)
            };

            self.readchar(m.ch as char);
        }

        false
    }
}

#[no_mangle]
pub fn main() {
    let mut server = Server::new();
    serve_forever!(&mut server, []);
}
