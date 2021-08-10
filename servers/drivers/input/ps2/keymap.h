#ifndef __KEYMAP_H__
#define __KEYMAP_H__

#include <types.h>

#define KEY_RELEASE 0x80

enum keycode {
    KEY_ENTER = 0x00,
    KEY_BACKSPACE = 0x01,
    KEY_CTRL_LEFT = 0x02,
    KEY_CTRL_RIGHT = 0x03,
    KEY_ALT_LEFT = 0x04,
    KEY_ALT_RIGHT = 0x05,
    KEY_SHIFT_LEFT = 0x06,
    KEY_SHIFT_RIGHT = 0x07,
    KEY_SUPER_LEFT = 0x08,
    KEY_SUPER_RIGHT = 0x09,
    KEY_FN = 0x0a,
    KEY_CAPS_LOCK = 0x0b,
    KEY_ARROW_UP = 0x0c,
    KEY_ARROW_DOWN = 0x0d,
    KEY_ARROW_LEFT = 0x0e,
    KEY_ARROW_RIGHT = 0x0f,
    KEY_ESC = 0x10,
    KEY_F1 = 0x11,
    KEY_F2 = 0x12,
    KEY_F3 = 0x13,
    KEY_F4 = 0x14,
    KEY_F5 = 0x15,
    KEY_F6 = 0x16,
    KEY_F7 = 0x17,
    KEY_F8 = 0x18,
    KEY_F9 = 0x19,
    KEY_F10 = 0x1a,
    KEY_F11 = 0x1b,
    KEY_F12 = 0x1c,
};

uint8_t scan_code2key_code(uint8_t scan_code, bool shift);

#endif
