#ifndef __PS2KBD_H__
#define __PS2KBD_H__

#define KEYBUF_SIZE        64
#define KEYBOARD_IRQ       1
#define IOPORT_DATA        0x60
#define IOPORT_STATUS      0x64
#define STATUS_OUTBUF_FULL (1 << 0)

#define SCANCODE_RELEASED 0x80
#define SCANCODE_CTRL     0x1d
#define SCANCODE_LSHIFT   0x2a
#define SCANCODE_RSHIFT   0x36
#define SCANCODE_CAPSLOCK 0x3a
#define SCANCODE_ALT      0x38

#endif
