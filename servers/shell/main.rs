use resea::channel::Channel;
use resea::idl;
use resea::idl::text_screen_device::{call_print_char, call_print_str};
use resea::message::Message;
use resea::server::connect_to_server;
use resea::std::string::String;

static _MEMMGR_SERVER: Channel = Channel::from_cid(1);
static KERNEL_SERVER: Channel = Channel::from_cid(2);

struct Server {
    ch: Channel,
    screen: Channel,
    fs: Channel,
    input: String,
}

impl Server {
    pub fn new() -> Server {
        // Connect to the dependent servers...
        let screen = connect_to_server(idl::text_screen_device::INTERFACE_ID)
            .expect("failed to connect to a text_screen_device server");
        let keyboard = connect_to_server(idl::keyboard_device::INTERFACE_ID)
            .expect("failed to connect to a keyboard_device server");
        let fs =
            connect_to_server(idl::fs::INTERFACE_ID).expect("failed to connect to a fs server");

        // Print the welcome message.
        call_print_str(&screen, "Resea Version \n").unwrap();
        call_print_str(&screen, "Type 'help' for usage.\n").unwrap();
        call_print_str(&screen, ">>> ").unwrap();

        // Start receiving keyboard inputs from the driver.
        use idl::keyboard_device::call_listen;
        let server_ch = Channel::create().unwrap();
        let listener_ch = Channel::create().unwrap();
        listener_ch.transfer_to(&server_ch).unwrap();
        call_listen(&keyboard, listener_ch).unwrap();

        Server {
            ch: server_ch,
            screen,
            fs,
            input: String::new(),
        }
    }

    fn run_command(&mut self) {
        if self.input == "help" {
            self.print_string("echo <string>   -  Prints a string.\n");
        } else if self.input == "dmesg" {
            loop {
                let r = idl::kernel::call_read_kernel_log(&KERNEL_SERVER).unwrap();
                if r.is_empty() {
                    break;
                }
                self.print_string(&r);
            }
        } else if self.input.starts_with("echo ") {
            self.print_string(&self.input[5..]);
            self.print_string("\n");
        } else if self.input.starts_with("cat ") {
            let path = &self.input[4..];
            match idl::fs::call_open(&self.fs, path) {
                Ok(handle) => {
                    match idl::fs::call_read(&self.fs, handle, 0, 4096 /* FIXME: */) {
                        Ok(page) => {
                            let content =
                                unsafe { core::str::from_utf8_unchecked(page.as_bytes()) };
                            self.print_string(content);
                        }
                        Err(_) => self.print_string("failed to read the file"),
                    }
                }
                Err(_) => self.print_string("failed to read the file"),
            }
        } else {
            self.print_string("Unknown command.\n");
        }
    }

    fn print_string(&self, string: &str) {
        call_print_str(&self.screen, string).unwrap();
    }

    fn print_prompt(&self) {
        call_print_str(&self.screen, ">>> ").unwrap();
    }

    fn readchar(&mut self, ch: char) {
        call_print_char(&self.screen, ch as u8).ok();
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
    info!("starting...");
    let mut server = Server::new();
    info!("ready");
    serve_forever!(&mut server, []);
}
