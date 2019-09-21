#include <resea.h>
#include <resea_idl.h>
#include <server.h>
#include <driver/io.h>
#include "keymap.h"
#include "pc_kbd.h"

// Channels connected by memmgr.
static cid_t memmgr_ch = 1;
static cid_t kernel_ch = 2;
// The channel at which we receive messages.
static cid_t server_ch;
// The channel to receive interrupt notifications. Transfers to server_ch.
static cid_t kbd_irq_ch;
// The channel to receive discovery.connect_request. Transfers to server_ch.
static cid_t discovery_ch;
// The channel to send keyboard events.
static cid_t listener_ch = 0;

static io_handle_t io_handle;
static struct modifiers_state modifiers;

static void read_keyboard_input(void) {
    uint8_t scan_code = io_read8(&io_handle, IOPORT_OFFSET_DATA);
    bool shift = modifiers.shift_left || modifiers.shift_right;
    uint8_t key_code = scan_code2key_code(scan_code, shift);
    char ascii = '_';
    switch (key_code & ~KEY_RELEASE) {
    case KEY_SHIFT_LEFT:
        modifiers.shift_left = (key_code & KEY_RELEASE) == 0;
        break;
    case KEY_SHIFT_RIGHT:
        modifiers.shift_right = (key_code & KEY_RELEASE) == 0;
        break;
    default:
        ascii = key_code & ~KEY_RELEASE;
    }

    TRACE("keyboard: scan_code=%x, key_code=%x, ascii='%c'%s%s%s%s%s%s%s",
          scan_code, key_code, ascii,
          (key_code & KEY_RELEASE) ? " [release]"     : "",
          modifiers.ctrl_left      ? " [ctrl-left]"   : "",
          modifiers.ctrl_right     ? " [ctrl-right]"  : "",
          modifiers.alt_left       ? " [alt-left]"    : "",
          modifiers.alt_right      ? " [alt-right]"   : "",
          modifiers.shift_left     ? " [shift-left]"  : "",
          modifiers.shift_right    ? " [shift-right]" : ""
         );

    if (listener_ch) {
        on_keyinput(listener_ch, key_code);
    }
}

static error_t handle_notification(struct message *m) {
    intmax_t intr_count = m->payloads.notification.notification.data;
    while (intr_count-- > 0) {
        read_keyboard_input();
    }
    return ERR_DONT_REPLY;
}

static error_t handle_connect_request(struct message *m) {
    TRACE("connect_request()");
    cid_t new_ch;
    TRY(open(&new_ch));
    transfer(new_ch, server_ch);

    m->header = CONNECT_REQUEST_REPLY_HEADER;
    m->payloads.discovery.connect_request_reply.ch = new_ch;
    return OK;
}

static error_t handle_listen_keyboard(struct message *m) {
    listener_ch = m->payloads.keyboard_driver.listen_keyboard.ch;

    m->header = LISTEN_KEYBOARD_REPLY_HEADER;
    return OK;
}

/// Does work for the message `m` and fills a reply message into it`.
static error_t process_message(struct message *m) {
    switch (MSG_TYPE(m->header)) {
    case NOTIFICATION_MSG:    return handle_notification(m);
    case CONNECT_REQUEST_MSG: return handle_connect_request(m);
    case LISTEN_KEYBOARD_MSG: return handle_listen_keyboard(m);
    }
    return ERR_INVALID_MESSAGE;
}

void main(void) {
    INFO("starting...");
    TRY_OR_PANIC(open(&server_ch));

    TRY_OR_PANIC(io_open(&io_handle, kernel_ch, IO_SPACE_IOPORT,
                         IOPORT_ADDR, IOPORT_SIZE));

    // Create and connect a channel to receive keyboard interrupts.
    TRY_OR_PANIC(io_listen_interrupt(kernel_ch, server_ch, KEYBOARD_IRQ,
                                     &kbd_irq_ch));

    TRY_OR_PANIC(open(&discovery_ch));
    TRY_OR_PANIC(transfer(discovery_ch, server_ch));
    TRY_OR_PANIC(register_server(memmgr_ch, KEYBOARD_DRIVER_INTERFACE,
                                 discovery_ch));

    // FIXME: memmgr waits for us due to connect_request.
    // INFO("entering the mainloop...");
    server_mainloop(server_ch, process_message);
}
