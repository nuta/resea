#include "keymap.h"
#include "ps2.h"
#include <driver/io.h>
#include <driver/irq.h>
#include <resea/ipc.h>
#include <resea/printf.h>

static io_t ps2_io;
static task_t gui_server;
static struct modifiers_state modifiers;

static void handle_keyboard_irq(void) {
    uint8_t scan_code = io_read8(ps2_io, IOPORT_OFFSET_DATA);
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
          (key_code & KEY_RELEASE) ? " [release]" : "",
          modifiers.ctrl_left ? " [ctrl-left]" : "",
          modifiers.ctrl_right ? " [ctrl-right]" : "",
          modifiers.alt_left ? " [alt-left]" : "",
          modifiers.alt_right ? " [alt-right]" : "",
          modifiers.shift_left ? " [shift-left]" : "",
          modifiers.shift_right ? " [shift-right]" : "");
}

static uint8_t read_status_reg(void) {
    return io_read8(ps2_io, IOPORT_OFFSET_STATUS);
}

static uint8_t read_data_reg(void) {
    return io_read8(ps2_io, IOPORT_OFFSET_DATA);
}

#define MOUSE_BYTE1_LEFT_BUTTON  (1 << 0)
#define MOUSE_BYTE1_RIGHT_BUTTON (1 << 1)
#define MOUSE_BYTE1_ALWAYS_1     (1 << 3)
#define MOUSE_BYTE1_X_SIGNED     (1 << 4)
#define MOUSE_BYTE1_Y_SIGNED     (1 << 5)
#define MOUSE_BYTE1_X_OVERFLOW   (1 << 6)
#define MOUSE_BYTE1_Y_OVERFLOW   (1 << 7)

static void handle_mouse_irq(void) {
    static int mouse_byte_pos = 0;
    static uint8_t data[3];

    uint8_t status = read_status_reg();
    bool mouse_input = (status & (1 << 5)) != 1;
    if (!mouse_input) {
        handle_keyboard_irq();
        return;
    }

    switch (mouse_byte_pos) {
        case 0:
            data[0] = read_data_reg();
            if ((data[0] & MOUSE_BYTE1_ALWAYS_1) == 0) {
                // Invalid data, just ignore it.
                WARN("invalid first byte: data=%x, status=%x", data[0], status);
                return;
            }
            mouse_byte_pos++;
            break;
        case 1:
            data[1] = read_data_reg();
            mouse_byte_pos++;
            break;
        case 2: {
            mouse_byte_pos = 0;
            bool overflowed = (data[0] & MOUSE_BYTE1_X_OVERFLOW) != 0
                              || (data[0] & MOUSE_BYTE1_Y_OVERFLOW) != 0;
            if (overflowed) {
                TRACE("overflowed (first_byte=%x)", data[0]);
                break;
            }

            data[2] = read_data_reg();
            bool left_pressed = (data[0] & MOUSE_BYTE1_LEFT_BUTTON) != 0;
            bool right_pressed = (data[0] & MOUSE_BYTE1_RIGHT_BUTTON) != 0;
            int32_t x_raw = (data[0] & MOUSE_BYTE1_X_SIGNED)
                                ? (((uint32_t) data[1]) | 0xffffff00)
                                : data[1];
            int32_t y_raw = (data[0] & MOUSE_BYTE1_Y_SIGNED)
                                ? (((uint32_t) data[2]) | 0xffffff00)
                                : data[2];
            int x = x_raw;
            int y = -y_raw;

            DBG("mouse: data=[%x, %d, %d], x=%d, y=%d%s%s", data[0], data[1],
                data[2], x, y, left_pressed ? " [LEFT]" : "",
                right_pressed ? "[RIGHT]" : "");

            struct message m;
            m.type = MOUSE_INPUT_MSG;
            m.mouse_input.x_delta = x;
            m.mouse_input.y_delta = y;
            m.mouse_input.clicked_left = left_pressed;
            m.mouse_input.clicked_right = right_pressed;
            ipc_send(gui_server, &m);
            break;
        }
        default:
            UNREACHABLE();
    }
}

/// Waits until the keyboard controller gets ready.
static void wait_kbd(void) {
    while ((io_read8(ps2_io, IOPORT_OFFSET_STATUS) & 0x02) != 0)
        ;
}

static void wait_kbd_ack(void) {
    while (io_read8(ps2_io, IOPORT_OFFSET_DATA) != 0xfa)
        ;
}

static void init_device(void) {
    ps2_io = io_alloc_port(IOPORT_ADDR, IOPORT_SIZE, IO_ALLOC_NORMAL);
    ASSERT_OK(irq_acquire(MOUSE_IRQ));
    ASSERT_OK(irq_acquire(KEYBOARD_IRQ));

    // Enable mouse.
    wait_kbd();
    io_write8(ps2_io, IOPORT_OFFSET_STATUS, 0xa8);

    while (io_read8(ps2_io, IOPORT_OFFSET_STATUS) & KBD_STATUS_OUTBUF_FULL) {
        io_read8(ps2_io, IOPORT_OFFSET_DATA);
    }
    wait_kbd();
    io_write8(ps2_io, IOPORT_OFFSET_STATUS, 0x20);
    wait_kbd();
    uint8_t compaq_status = io_read8(ps2_io, IOPORT_OFFSET_DATA);
    wait_kbd();
    io_write8(ps2_io, IOPORT_OFFSET_STATUS, 0x60);
    wait_kbd();
    io_write8(ps2_io, IOPORT_OFFSET_DATA, compaq_status | 2);

    wait_kbd();
    io_write8(ps2_io, IOPORT_OFFSET_STATUS, 0xd4);
    wait_kbd();
    io_write8(ps2_io, IOPORT_OFFSET_DATA, 0xf6);
    wait_kbd_ack();

    wait_kbd();
    io_write8(ps2_io, IOPORT_OFFSET_STATUS, 0xd4);
    wait_kbd();
    io_write8(ps2_io, IOPORT_OFFSET_DATA, 0xf4);
    wait_kbd_ack();
}

void main(void) {
    INFO("starting...");

    init_device();

    gui_server = ipc_lookup("gui");
    ASSERT_OK(ipc_serve("kbd"));
    ASSERT_OK(ipc_serve("mouse"));

    TRACE("ready");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG:
                if (m.notifications.data & NOTIFY_IRQ) {
                    handle_mouse_irq();
                    // handle_keyboard_irq();
                }
            default:
                TRACE("unknown message %s", msgtype2str(m.type));
        }
    }
}
