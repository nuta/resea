#include <message.h>
#include <std/async.h>
#include <std/io.h>
#include <std/lookup.h>
#include <std/printf.h>
#include <std/syscall.h>
#include "ps2kbd.h"

static tid_t shell_server;

// Modifier keys. True if the key is being pressed.
static bool shift_left = false;
static bool shift_right = false;
static bool capslock = false;
static bool ctrl = false;
static bool alt = false;

// clang-format off
static uint8_t normal_keymap[256] = {
    0x00, '\e', '1',  '2',  '3',  '4',  '5',  '6',  // 0x00
    '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t', // 0x08
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  // 0x10
    'o',  'p',  '[',  ']',  '\n', 0x00, 'a',  's',  // 0x18
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  // 0x20
    '\'', '`',  0x00, '\\', 'z',  'x',  'c',  'v',  // 0x28
    'b',  'n',  'm',  ',',  '.',  '/',             //  0x30
    [0x39] = ' ', [0x9c] = '\n' /* Enter */, [0xd3] = 0x7f /* Delete */,
};

static uint8_t shifted_keymap[256] = {
    0x00, '\e', '!',  '@',  '#',  '$',  '%',  '^',  // 0x00
    '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t', // 0x08
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  // 0x10
    'O',  'P',  '{',  '}',  '\n', 0x00, 'A',  'S',  // 0x18
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  // 0x20
    '"', '~',  0x00,  '|',  'Z',  'X',  'C',  'V',  // 0x28
    'B',  'N',  'M',  '<',  '>',  '?',              // 0x30
    [0x39] = ' ', [0x9c] = '\n' /* Enter */, [0xd3] = 0x7f /* Delete */
};
// clang-format on

static void handle_irq(void) {
    while (true) {
        uint8_t status = io_in8(IOPORT_STATUS);
        if ((status & STATUS_OUTBUF_FULL) == 0) {
            break;
        }

        uint8_t scancode = io_in8(IOPORT_DATA);
        bool is_pressed = (scancode & SCANCODE_RELEASED) == 0;
        switch (scancode & ~SCANCODE_RELEASED) {
            case SCANCODE_CTRL:
                ctrl = is_pressed;
                break;
            case SCANCODE_ALT:
                alt = is_pressed;
                break;
            case SCANCODE_LSHIFT:
                shift_left = is_pressed;
                break;
            case SCANCODE_RSHIFT:
                shift_right = is_pressed;
                break;
            case SCANCODE_CAPSLOCK:
                capslock = is_pressed;
                break;
            default: {
                if (!is_pressed) {
                    break;
                }

                bool shifted = shift_left || shift_right || capslock;
                uint8_t *keymap = shifted ? shifted_keymap : normal_keymap;
                uint8_t ascii = keymap[scancode];
                if (!ascii) {
                    break;
                }

                uint16_t keycode =
                    (alt ? KEY_MOD_ALT : 0) | (ctrl ? KEY_MOD_CTRL : 0) | ascii;
                // TRACE("key event: scancode=%02x, keycode=%04x", scancode,
                //       keycode);

                struct message m;
                m.type = KBD_ON_PRESSED_MSG;
                m.kbd_on_pressed.keycode = keycode;
                async_send(shell_server, &m, sizeof(m));
            }
        }
    }
}

void main(void) {
    error_t err;
    INFO("starting...");

    shell_server = ipc_lookup("shell");
    ASSERT_OK(shell_server);

    err = irq_acquire(KEYBOARD_IRQ);
    ASSERT_OK(err);

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m, sizeof(m));
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG:
                if (m.notifications.data & NOTIFY_READY) {
                    async_flush();
                }

                if (m.notifications.data & NOTIFY_IRQ) {
                    handle_irq();
                }
                break;
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
