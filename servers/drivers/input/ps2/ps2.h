#ifndef __PS2_H__
#define __PS2_H__

#include <types.h>

#define KEYBOARD_IRQ               1
#define MOUSE_IRQ                  12
#define IOPORT_ADDR                0x60
#define IOPORT_SIZE                5 /* 0x60 and 0x64 */
#define IOPORT_OFFSET_DATA         0x00
#define IOPORT_OFFSET_STATUS       0x04
#define KBD_STATUS_OUTBUF_FULL     (1 << 0)
#define KBD_STATUS_AUX_OUTPUT_FULL (1 << 5)

#define PS2_CMD_READ_CONFIG    0x20
#define PS2_CMD_WRITE_CONFIG   0x60
#define PS2_CMD_ENABLE_MOUSE   0xa8
#define PS2_CMD_MOUSE_WRITE    0xd4
#define MOUSE_CMD_SET_DEFAULTS 0xf6
#define MOUSE_CMD_DATA_ON      0xf4

#define PS2_CONFIG_ENABLE_MOUSE_IRQ (1 << 1)

#define MOUSE_BYTE1_LEFT_BUTTON  (1 << 0)
#define MOUSE_BYTE1_RIGHT_BUTTON (1 << 1)
#define MOUSE_BYTE1_ALWAYS_1     (1 << 3)
#define MOUSE_BYTE1_X_SIGNED     (1 << 4)
#define MOUSE_BYTE1_Y_SIGNED     (1 << 5)
#define MOUSE_BYTE1_X_OVERFLOW   (1 << 6)
#define MOUSE_BYTE1_Y_OVERFLOW   (1 << 7)

struct modifiers_state {
    bool ctrl_left;
    bool ctrl_right;
    bool alt_left;
    bool alt_right;
    bool shift_left;
    bool shift_right;
    bool super_left;
    bool super_right;
    bool fn;
    bool caps_lock;
};

#endif
