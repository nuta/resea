#ifndef __PC_KBD_H__
#define __PC_KBD_H__

#include <types.h>

#define IOPORT_ADDR 0x60
#define IOPORT_SIZE 5     /* 0x60 and 0x64 */
#define IOPORT_OFFSET_DATA 0x00
#define KEYBOARD_IRQ 1

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
