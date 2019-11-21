use crate::keymap::*;
use resea::channel::Channel;
use resea::collections::VecDeque;

const KEYBOARD_IRQ: u8 = 1;
const IOPORT_DATA: u64 = 0x60;
const IOPORT_STATUS: u64 = 0x64;
const STATUS_OUTBUF_FULL: u8 = 1 << 0;

pub struct Keyboard {
    kernel_server: &'static Channel,
    /// Received characters.
    buffer: VecDeque<u8>,
    _ctrl_left: bool,
    _ctrl_right: bool,
    _alt_left: bool,
    _alt_right: bool,
    shift_left: bool,
    shift_right: bool,
    _super_left: bool,
    _super_right: bool,
    caps_lock: bool,
}

impl Keyboard {
    pub fn new(server_ch: &Channel, kernel_server: &'static Channel) -> Keyboard {
        use resea::idl::kernel::Client;

        let kbd_irq_ch = Channel::create().unwrap();
        kbd_irq_ch.transfer_to(server_ch).unwrap();
        kernel_server
            .listen_irq(kbd_irq_ch, KEYBOARD_IRQ)
            .expect("failed to register the keyboard IRQ handler");

        // FIXME: kbd_irq_ch is not closed.
        Keyboard {
            kernel_server,
            buffer: VecDeque::with_capacity(16),
            _ctrl_left: false,
            _ctrl_right: false,
            _alt_left: false,
            _alt_right: false,
            shift_left: false,
            shift_right: false,
            _super_left: false,
            _super_right: false,
            caps_lock: false,
        }
    }

    pub fn buffer(&mut self) -> &mut VecDeque<u8> {
        &mut self.buffer
    }

    pub fn read_input(&mut self) {
        use resea::idl::kernel::Client;
        loop {
            let status = self.kernel_server.read_ioport(IOPORT_STATUS, 1).unwrap() as u8;
            if status & STATUS_OUTBUF_FULL == 0 {
                break;
            }

            let raw_scancode = self.kernel_server.read_ioport(IOPORT_DATA, 1).unwrap() as u8;
            let scancode = ScanCode::new(raw_scancode);

            let shifted = self.shift_left || self.shift_right || self.caps_lock;
            let ascii = scancode.as_ascii(shifted);
            match ascii {
                KEY_SHIFT_LEFT => self.shift_left = scancode.is_press(),
                KEY_SHIFT_RIGHT => self.shift_right = scancode.is_press(),
                KEY_CAPS_LOCK => self.caps_lock = scancode.is_press(),
                _ if scancode.is_press() => {
                    trace!("key pressed: '{}'", ascii as char);
                    self.buffer.push_back(ascii);
                }
                _ => {}
            }
        }
    }
}
