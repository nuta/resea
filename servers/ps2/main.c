#include <resea.h>
#include <idl_stubs.h>
#include <server.h>
#include <driver/io.h>
#include "keymap.h"
#include "ps2.h"

// Channels connected by memmgr.
static cid_t memmgr_ch = 1;
static cid_t kernel_ch = 2;
// The channel at which we receive messages.
static cid_t server_ch;
// The channel to receive interrupt notifications. Transfers to server_ch.
static cid_t kbd_irq_ch;
static cid_t mouse_irq_ch;
// The channel to send keyboard events.
static cid_t keyboard_listener_ch = 0;
// The channel to send mouse events.
static cid_t mouse_listener_ch = 0;

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
        ascii = (char) (key_code & ~KEY_RELEASE);
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

    if (keyboard_listener_ch) {
        call_keyboard_driver_keyinput_event(keyboard_listener_ch, key_code);
    }
}

static uint8_t read_status_reg(void) {
    return io_read8(&io_handle, IOPORT_OFFSET_STATUS);
}

static uint8_t read_data_reg(void) {
    return io_read8(&io_handle, IOPORT_OFFSET_DATA);
}

#define MOUSE_BYTE1_LEFT_BUTTON   (1 << 0)
#define MOUSE_BYTE1_RIGHT_BUTTON  (1 << 1)
#define MOUSE_BYTE1_ALWAYS_1      (1 << 3)
#define MOUSE_BYTE1_X_SIGNED      (1 << 4)
#define MOUSE_BYTE1_Y_SIGNED      (1 << 5)
#define MOUSE_BYTE1_X_OVERFLOW    (1 << 6)
#define MOUSE_BYTE1_Y_OVERFLOW    (1 << 7)

static void read_mouse_input(void) {
    static int mouse_byte_pos = 0;
    static uint8_t data[3];

    switch (mouse_byte_pos) {
    case 0:
        data[0] = read_data_reg();
        if ((data[0] & MOUSE_BYTE1_X_OVERFLOW)
            || (data[0] & MOUSE_BYTE1_Y_OVERFLOW)
            || (data[0] & MOUSE_BYTE1_ALWAYS_1) == 0) {
            // Invalid data, just ignore it.
            return;
        }
        mouse_byte_pos++;
        break;
    case 1:
        data[1] = read_data_reg();
        mouse_byte_pos++;
        break;
    case 2: {
        data[2] = read_data_reg();
        bool left_pressed  = (data[0] & MOUSE_BYTE1_LEFT_BUTTON) != 0;
        bool right_pressed = (data[0] & MOUSE_BYTE1_RIGHT_BUTTON) != 0;
        int x = (data[0] & MOUSE_BYTE1_X_SIGNED)
            ? (int8_t) data[1] : (uint8_t) data[1];
        int y = (data[0] & MOUSE_BYTE1_Y_SIGNED)
            ? (int8_t) data[2] : (uint8_t) data[2];

        //TRACE("mouse: x=%d, y=%d%s%s",
        //      x, y, left_pressed ? " [LEFT]" : "", right_pressed ? " [RIGHT]" : "");

        if (mouse_listener_ch) {
            INFO("sending mouse event");
            send_mouse_driver_mouse_event(mouse_listener_ch, left_pressed,
                right_pressed, x, y);
        }

        mouse_byte_pos = 0;
        break;
    }
    default:
        UNREACHABLE;
    } // switch
}

/// Waits until the keyboard controller gets ready.
static void wait_kbd(void) {
    while ((io_read8(&io_handle, IOPORT_OFFSET_STATUS) & 0x02) != 0);
}

static void wait_kbd_ack(void) {
    while (io_read8(&io_handle, IOPORT_OFFSET_DATA) != 0xfa);
}

static error_t init_device(void) {
    TRY(io_open(&io_handle, kernel_ch, IO_SPACE_IOPORT, IOPORT_ADDR,
                IOPORT_SIZE));

    // Enable mouse.
    wait_kbd();
    io_write8(&io_handle, IOPORT_OFFSET_STATUS, 0xa8);

    while (io_read8(&io_handle, IOPORT_OFFSET_STATUS) & KBD_STATUS_OUTBUF_FULL) {
        io_read8(&io_handle, IOPORT_OFFSET_DATA);
    }
    wait_kbd();
    io_write8(&io_handle, IOPORT_OFFSET_STATUS, 0x20);
    wait_kbd();
    uint8_t compaq_status = io_read8(&io_handle, IOPORT_OFFSET_DATA);
    wait_kbd();
    io_write8(&io_handle, IOPORT_OFFSET_STATUS, 0x60);
    wait_kbd();
    io_write8(&io_handle, IOPORT_OFFSET_DATA, compaq_status | 2);

    wait_kbd();
    io_write8(&io_handle, IOPORT_OFFSET_STATUS, 0xd4);
    wait_kbd();
    io_write8(&io_handle, IOPORT_OFFSET_DATA, 0xf6);
    wait_kbd_ack();

    wait_kbd();
    io_write8(&io_handle, IOPORT_OFFSET_STATUS, 0xd4);
    wait_kbd();
    io_write8(&io_handle, IOPORT_OFFSET_DATA, 0xf4);
    wait_kbd_ack();

    return OK;
}

static error_t handle_notification(notification_t notification) {
    if (notification & NOTIFY_INTERRUPT) {
        // Determine the source of the interrupt.
        while (read_status_reg() & KBD_STATUS_OUTBUF_FULL) {
            if (read_status_reg() & KBD_STATUS_AUX_OUTPUT_FULL) {
                read_mouse_input();
            } else {
                read_keyboard_input();
            }
        }
    }

    return DONT_REPLY;
}

static error_t handle_server_connect(struct message *m) {
    cid_t new_ch;
    INFO("connect request");
    TRY(open(&new_ch));
    transfer(new_ch, server_ch);

    m->header = SERVER_CONNECT_REPLY_MSG;
    m->payloads.server.connect_reply.ch = new_ch;
    return OK;
}

static error_t handle_keyboard_listen(struct message *m) {
    keyboard_listener_ch = m->payloads.keyboard_driver.listen.ch;
    m->header = KEYBOARD_DRIVER_LISTEN_REPLY_MSG;
    return OK;
}

static error_t handle_mouse_listen(struct message *m) {
    mouse_listener_ch = m->payloads.mouse_driver.listen.ch;
    INFO("receive ouse ch = %d", mouse_listener_ch);
    m->header = MOUSE_DRIVER_LISTEN_REPLY_MSG;
    return OK;
}

static error_t process_message(struct message *m) {
    notification_t notification = m->notification;
    if (notification) {
        handle_notification(notification);
    }

    switch (m->header) {
    case SERVER_CONNECT_MSG:         return handle_server_connect(m);
    case KEYBOARD_DRIVER_LISTEN_MSG: return handle_keyboard_listen(m);
    case MOUSE_DRIVER_LISTEN_MSG:    return handle_mouse_listen(m);
    case NOTIFICATION_MSG:           return DONT_REPLY;
    }
    return ERR_UNEXPECTED_MESSAGE;
}

void main(void) {
    INFO("starting...");

    TRY_OR_PANIC(init_device());

    TRY_OR_PANIC(open(&server_ch));
    TRY_OR_PANIC(io_listen_interrupt(kernel_ch, server_ch, KEYBOARD_IRQ,
                                     &kbd_irq_ch));
    TRY_OR_PANIC(io_listen_interrupt(kernel_ch, server_ch, MOUSE_IRQ,
                                     &mouse_irq_ch));
    TRY_OR_PANIC(server_register(memmgr_ch, server_ch,
                                 KEYBOARD_DRIVER_INTERFACE));
    TRY_OR_PANIC(server_register(memmgr_ch, server_ch,
                                 MOUSE_DRIVER_INTERFACE));

    INFO("entering the mainloop...");
    server_mainloop(server_ch, process_message);
}
