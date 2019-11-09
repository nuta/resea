use resea::result::Error;
use resea::channel::Channel;
use resea::message::Page;
use resea::idl::io::Client;
use resea::std::mem::size_of;
use resea::collections::VecDeque;
use crate::keymap::*;

const KEYBOARD_IRQ: u8 = 1;
const IOPORT_DATA: u64 = 0x60;
const IOPORT_STATUS: u64 = 0x64;
const STATUS_OUTBUF_FULL: u8 = 1 << 0;

pub struct Keyboard {
    kernel_server: &'static Channel,
    /// Received characters.
    buffer: VecDeque<u8>,
    ctrl_left: bool,
    ctrl_right: bool,
    alt_left: bool,
    alt_right: bool,
    shift_left: bool,
    shift_right: bool,
    super_left: bool,
    super_right: bool,
    caps_lock: bool,
}

impl Keyboard {
    pub fn new(server_ch: &Channel, kernel_server: &'static Channel) -> Keyboard {
        use resea::idl::io::Client;

        let kbd_irq_ch = Channel::create().unwrap();
        kbd_irq_ch.transfer_to(server_ch).unwrap();
        kernel_server.listen_irq(kbd_irq_ch, KEYBOARD_IRQ)
        .expect("failed to register the keyboard IRQ handler");

        // FIXME: kbd_irq_ch is not closed.
        Keyboard {
            kernel_server,
            buffer: VecDeque::with_capacity(16),
            ctrl_left: false,
            ctrl_right: false,
            alt_left: false,
            alt_right: false,
            shift_left: false,
            shift_right: false,
            super_left: false,
            super_right: false,
            caps_lock: false,
        }
    }

    pub fn buffer(&mut self) -> &mut VecDeque<u8> {
        &mut self.buffer
    }

    pub fn read_input(&mut self) {
        use resea::idl::io::Client;
        loop {
            let status =
                self.kernel_server.read_ioport(IOPORT_STATUS, 1).unwrap() as u8;
            if status & STATUS_OUTBUF_FULL == 0 {
                break;
            }

            let raw_scancode =
                self.kernel_server.read_ioport(IOPORT_DATA, 1).unwrap() as u8;
            let scancode = ScanCode::new(raw_scancode);

            let shifted = self.shift_left || self.shift_right || self.caps_lock;
            match scancode.as_ascii(shifted) {
                KEY_SHIFT_LEFT => self.shift_left = scancode.is_press(),
                KEY_SHIFT_RIGHT => self.shift_right = scancode.is_press(),
                KEY_CAPS_LOCK => self.caps_lock = scancode.is_press(),
                _ => (),
            }

            info!("key input: {}", scancode.as_ascii(shifted) as char);
        }
    }
}
