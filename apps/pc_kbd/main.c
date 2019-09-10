#include <resea.h>
#include <resea_idl.h>
#include "keymap.h"

#define IOPORT_KEYBOARD_DATA 0x60

static cid_t listener;
static cid_t kbd_irq_ch;

static inline uint8_t asm_in8(uint16_t port) {
    uint8_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

bool ctrl_left_pressed   = false;
bool ctrl_right_pressed  = false;
bool alt_left_pressed    = false;
bool alt_right_pressed   = false;
bool shift_left_pressed  = false;
bool shift_right_pressed = false;
bool super_left_pressed  = false;
bool super_right_pressed = false;
bool fn_pressed          = false;
bool caps_lock_pressed   = false;

static void read_keyboard_input(void) {
    uint8_t scan_code = asm_in8(IOPORT_KEYBOARD_DATA);
    bool shift = shift_left_pressed || shift_right_pressed;
    uint8_t key_code = scan_code2key_code(scan_code, shift);
    char ascii = '_';
    switch (key_code & ~KEY_RELEASE) {
    case KEY_SHIFT_LEFT:
        shift_left_pressed = (key_code & KEY_RELEASE) == 0;
        break;
    case KEY_SHIFT_RIGHT:
        shift_right_pressed = (key_code & KEY_RELEASE) == 0;
        break;
    default:
        ascii = key_code & ~KEY_RELEASE;
    }

    TRACE("keyboard: scan_code=%x, key_code=%x, ascii='%c'%s%s%s%s%s%s%s",
          scan_code, key_code, ascii,
          (key_code & KEY_RELEASE) ? " [release]"     : "",
          ctrl_left_pressed        ? " [ctrl-left]"   : "",
          ctrl_right_pressed       ? " [ctrl-right]"  : "",
          alt_left_pressed         ? " [alt-left]"    : "",
          alt_right_pressed        ? " [alt-right]"   : "",
          shift_left_pressed       ? " [shift-left]"  : "",
          shift_right_pressed      ? " [shift-right]" : ""
         );

    if (listener) {
        on_keyinput(listener, key_code);
    }
}

static void on_interrupt(intmax_t intr_count) {
    while (intr_count-- > 0) {
        read_keyboard_input();
    }
}

static error_t do_listen_keyboard(struct listen_keyboard_msg *m,
                                  UNUSED struct listen_keyboard_reply_msg *r) {
    listener = m->ch;
    return OK;
}

void mainloop(cid_t server_ch) {
    struct message m;
    TRY_OR_PANIC(ipc_recv(server_ch, &m));
    while (1) {
        struct message r;
        error_t err;
        cid_t from = m.from;
        switch (MSG_TYPE(m.header)) {
        case NOTIFICATION_MSG:
            on_interrupt(((struct notification_msg *) &m)->data);
            err = ERR_DONT_REPLY;
            break;
        case LISTEN_KEYBOARD_MSG:
            err = dispatch_listen_keyboard(do_listen_keyboard, &m, &r);
            break;
        default:
            WARN("invalid message type %x", MSG_TYPE(m.header));
            err = ERR_INVALID_MESSAGE;
            r.header = ERROR_TO_HEADER(err);
        }

        if (err == ERR_DONT_REPLY) {
            TRY_OR_PANIC(ipc_recv(server_ch, &m));
        } else {
            TRY_OR_PANIC(ipc_replyrecv(from, &r, &m));
        }
    }
}

void main(void) {
    INFO("starting...");
    cid_t server_ch;
    TRY_OR_PANIC(open(&server_ch));

    // Enable IO instructions.
    cid_t kernel_ch = 2;
    TRY_OR_PANIC(allow_io(kernel_ch));

    // Create and connect a channel to receive keyboard interrupts.
    TRY_OR_PANIC(open(&kbd_irq_ch));
    TRY_OR_PANIC(transfer(kbd_irq_ch, server_ch));
    TRY_OR_PANIC(listen_irq(kernel_ch, kbd_irq_ch, 1 /* keyboard irq */));

    INFO("entering the mainloop...");
    mainloop(server_ch);
}
