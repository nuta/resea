#![allow(unused)]

pub const KEY_RELEASE: u8     = 0x80;

pub const KEY_ENTER: u8       = 0x00;
pub const KEY_BACKSPACE: u8   = 0x01;
pub const KEY_CTRL_LEFT: u8   = 0x02;
pub const KEY_CTRL_RIGHT: u8  = 0x03;
pub const KEY_ALT_LEFT: u8    = 0x04;
pub const KEY_ALT_RIGHT: u8   = 0x05;
pub const KEY_SHIFT_LEFT: u8  = 0x06;
pub const KEY_SHIFT_RIGHT: u8 = 0x07;
pub const KEY_SUPER_LEFT: u8  = 0x08;
pub const KEY_SUPER_RIGHT: u8 = 0x09;
pub const KEY_FN: u8          = 0x0a;
pub const KEY_CAPS_LOCK: u8   = 0x0b;
pub const KEY_ARROW_UP: u8    = 0x0c;
pub const KEY_ARROW_DOWN: u8  = 0x0d;
pub const KEY_ARROW_LEFT: u8  = 0x0e;
pub const KEY_ARROW_RIGHT: u8 = 0x0f;
pub const KEY_ESC: u8         = 0x10;
pub const KEY_F1: u8          = 0x11;
pub const KEY_F2: u8          = 0x12;
pub const KEY_F3: u8          = 0x13;
pub const KEY_F4: u8          = 0x14;
pub const KEY_F5: u8          = 0x15;
pub const KEY_F6: u8          = 0x16;
pub const KEY_F7: u8          = 0x17;
pub const KEY_F8: u8          = 0x18;
pub const KEY_F9: u8          = 0x19;
pub const KEY_F10: u8         = 0x1a;
pub const KEY_F11: u8         = 0x1b;
pub const KEY_F12: u8         = 0x1c;

const US_KEY_MAP: [u8; 128] = [
    0, KEY_ESC, b'1', b'2', b'3', b'4', b'5', b'6', b'7', b'8', b'9', b'0', b'-', b'=',
    KEY_BACKSPACE,
    b'\t', b'q', b'w', b'e', b'r', b't', b'y', b'u', b'i', b'o', b'p', b'[', b']', KEY_ENTER,
    KEY_CTRL_LEFT, b'a', b's', b'd', b'f', b'g', b'h', b'j', b'k', b'l', b';', b'\'', b'`',
    KEY_SHIFT_LEFT, b'\\', b'z', b'x', b'c', b'v', b'b', b'n', b'm', b',', b'.', b'/',
    KEY_SHIFT_RIGHT,
    0, KEY_ALT_LEFT, b' ', KEY_CAPS_LOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    KEY_F11, KEY_F12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
];

const US_SHIFTED_KEYMAP: [u8; 128] = [
    0, KEY_ESC, b'!', b'@', b'#', b'$', b'%', b'^', b'&', b'*', b'(', b')', b'_', b'+',
    KEY_BACKSPACE,
    b'\t', b'Q', b'W', b'E', b'R', b'T', b'Y', b'U', b'I', b'O', b'P', b'{', b'}', KEY_ENTER,
    KEY_CTRL_LEFT, b'A', b'S', b'D', b'F', b'G', b'H', b'J', b'K', b'L', b':', b'"', b'~',
    KEY_SHIFT_LEFT, b'|', b'Z', b'X', b'C', b'V', b'B', b'N', b'M', b'<', b'>', b'?',
    KEY_SHIFT_RIGHT,
    0, KEY_ALT_LEFT, b' ', KEY_CAPS_LOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    KEY_F11, KEY_F12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
];

pub struct ScanCode(u8);

impl ScanCode {
    pub const fn new(scan_code: u8) -> ScanCode {
        ScanCode(scan_code)
    }

    pub fn is_press(&self) -> bool {
        !self.is_release()
    }

    pub fn is_release(&self) -> bool {
        self.0 & KEY_RELEASE != 0
    }

    pub fn as_ascii(&self, shifted: bool) -> u8 {
        let key_map = if shifted { &US_SHIFTED_KEYMAP } else { &US_KEY_MAP };
        let released = self.0 >= 0x80;
        key_map[self.0 as usize & 0x7f]
    }
}
