#ifndef __PS2_H__
#define __PS2_H__

#include <types.h>

#define KEYBOARD_IRQ 1
#define MOUSE_IRQ 12
#define IOPORT_ADDR 0x60
#define IOPORT_SIZE 5     /* 0x60 and 0x64 */
#define IOPORT_OFFSET_DATA   0x00
#define IOPORT_OFFSET_STATUS 0x04
#define KBD_STATUS_OUTBUF_FULL      (1 << 0)
#define KBD_STATUS_AUX_OUTPUT_FULL  (1 << 5)


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
