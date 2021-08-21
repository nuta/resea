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

static void wait_input(void) {
    while ((io_read8(ps2_io, IOPORT_OFFSET_STATUS) & (1 << 1)) != 0)
        ;
}

static void wait_output(void) {
    while ((io_read8(ps2_io, IOPORT_OFFSET_STATUS) & (1 << 0)) == 0)
        ;
}

static void wait_kbd_ack(void) {
    while (io_read8(ps2_io, IOPORT_OFFSET_DATA) != 0xfa)
        ;
}

static void write_ps2_command(uint8_t cmd) {
    wait_input();
    io_write8(ps2_io, IOPORT_OFFSET_STATUS, cmd);
}

static void write_ps2_command_with_data(uint8_t cmd, uint8_t data) {
    wait_input();
    io_write8(ps2_io, IOPORT_OFFSET_STATUS, cmd);
    wait_input();
    io_write8(ps2_io, IOPORT_OFFSET_DATA, data);
}

static uint8_t read_ps2_response(void) {
    wait_output();
    return io_read8(ps2_io, IOPORT_OFFSET_DATA);
}

static uint8_t read_status_reg(void) {
    return io_read8(ps2_io, IOPORT_OFFSET_STATUS);
}

static uint8_t read_data_reg(void) {
    wait_output();
    return io_read8(ps2_io, IOPORT_OFFSET_DATA);
}

static void handle_mouse_irq(void) {
    static int state = 0;
    static uint8_t data[3];

    bool send_mouse_event = false;
    int x = 0;
    int y = 0;
    bool left_pressed = false;
    bool right_pressed = false;
    while (true) {
        uint8_t status = read_status_reg();
        bool empty = (status & 1) == 0;
        if (empty) {
            break;
        }

        bool mouse_input = (status & (1 << 5)) != 0;
        if (!mouse_input) {
            handle_keyboard_irq();
        } else {
            switch (state) {
                case 0:
                    data[0] = read_data_reg();
                    if ((data[0] & MOUSE_BYTE1_ALWAYS_1) == 0) {
                        // Invalid data, just ignore it.
                        TRACE("invalid first byte: data=%x, status=%x", data[0],
                              status);
                        break;
                    }
                    state++;
                    break;
                case 1:
                    data[1] = read_data_reg();
                    state++;
                    break;
                case 2: {
                    state = 0;

                    bool overflowed =
                        (data[0] & MOUSE_BYTE1_X_OVERFLOW) != 0
                        || (data[0] & MOUSE_BYTE1_Y_OVERFLOW) != 0;
                    if (overflowed) {
                        TRACE("overflowed (first_byte=%x)", data[0]);
                        break;
                    }

                    data[2] = read_data_reg();
                    left_pressed = (data[0] & MOUSE_BYTE1_LEFT_BUTTON) != 0;
                    right_pressed = (data[0] & MOUSE_BYTE1_RIGHT_BUTTON) != 0;
                    int32_t x_raw = (data[0] & MOUSE_BYTE1_X_SIGNED)
                                        ? (((int) data[1]) - 0x100)
                                        : data[1];
                    int32_t y_raw = (data[0] & MOUSE_BYTE1_Y_SIGNED)
                                        ? (((int) data[2]) - 0x100)
                                        : data[2];
                    x += x_raw;
                    y += -y_raw;
                    send_mouse_event = true;
                    break;
                }
                default:
                    UNREACHABLE();
            }
        }
    }

    if (send_mouse_event) {
        TRACE("mouse: data=[%x, %d, %d], x=%d, y=%d%s%s", data[0], data[1],
              data[2], x, y, left_pressed ? " [LEFT]" : "",
              right_pressed ? "[RIGHT]" : "");

        struct message m;
        m.type = MOUSE_INPUT_MSG;
        m.mouse_input.x_delta = x;
        m.mouse_input.y_delta = y;
        m.mouse_input.clicked_left = left_pressed;
        m.mouse_input.clicked_right = right_pressed;
        ipc_send(gui_server, &m);
    }
}

static void init_device(void) {
    ps2_io = io_alloc_port(IOPORT_ADDR, IOPORT_SIZE, IO_ALLOC_NORMAL);
    ASSERT_OK(irq_acquire(MOUSE_IRQ));
    ASSERT_OK(irq_acquire(KEYBOARD_IRQ));

    write_ps2_command(PS2_CMD_ENABLE_MOUSE);

    while (io_read8(ps2_io, IOPORT_OFFSET_STATUS) & KBD_STATUS_OUTBUF_FULL) {
        io_read8(ps2_io, IOPORT_OFFSET_DATA);
    }

    write_ps2_command(PS2_CMD_READ_CONFIG);
    uint8_t config = read_ps2_response();
    write_ps2_command_with_data(PS2_CMD_WRITE_CONFIG,
                                config | PS2_CONFIG_ENABLE_MOUSE_IRQ);

    write_ps2_command_with_data(PS2_CMD_MOUSE_WRITE, MOUSE_CMD_SET_DEFAULTS);
    write_ps2_command_with_data(PS2_CMD_MOUSE_WRITE, MOUSE_CMD_DATA_ON);
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
                break;
            default:
                TRACE("unknown message %s", msgtype2str(m.type));
        }
    }
}
