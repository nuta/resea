#include <message.h>
#include <std/io.h>
#include <std/printf.h>
#include <std/syscall.h>
#include <string.h>

#define BLANK_CHAR    0x0f20 /* whitespace */
#define SCREEN_HEIGHT 25
#define SCREEN_WIDTH  80

// Colors.

static uint16_t *screen = NULL;

static void move_cursor(unsigned y, unsigned x) {
    uint16_t pos = y * SCREEN_WIDTH + x;
    io_out8(0x3d4, 0x0f);
    io_out8(0x3d5, pos & 0xff);
    io_out8(0x3d4, 0x0e);
    io_out8(0x3d5, (pos >> 8) & 0xff);
}

static void clear(void) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            screen[y * SCREEN_WIDTH + x] = BLANK_CHAR;
        }
    }
}

static void scroll(void) {
    // Scroll by one line.
    for (int from = 1; from < SCREEN_HEIGHT; from++) {
        int dst = (from - 1) * SCREEN_WIDTH;
        int src = from * SCREEN_WIDTH;
        memcpy(&screen[dst], &screen[src], sizeof(uint16_t) * SCREEN_WIDTH);
    }

    // Clear the last line.
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        screen[(SCREEN_HEIGHT - 1) * SCREEN_WIDTH + x] = BLANK_CHAR;
    }
}

static void draw_char(unsigned y, unsigned x, char ch, color_t fg, color_t bg) {
    if (y >= SCREEN_HEIGHT || x >= SCREEN_WIDTH) {
        return;
    }

    screen[y * SCREEN_WIDTH + x] = (bg << 12) | (fg << 8) | ch;
}

void main(void) {
    INFO("starting...");

    paddr_t paddr;
    screen = io_alloc_pages(1, 0xb8000, &paddr);

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m, sizeof(m));
        ASSERT_OK(err);

        switch (m.type) {
            case DRAW_CHAR_MSG:
                draw_char(m.draw_char.y, m.draw_char.x, m.draw_char.ch,
                          m.draw_char.fg_color, m.draw_char.bg_color);
                break;
            case MOVE_CURSOR_MSG:
                move_cursor(m.move_cursor.y, m.move_cursor.x);
                break;
            case CLEAR_DISPLAY_MSG:
                clear();
                break;
            case SCROLL_DISPLAY_MSG:
                scroll();
                break;
            case DISPLAY_GET_SIZE_MSG: {
                m.type = DISPLAY_GET_SIZE_REPLY_MSG;
                m.display_get_size_reply.width = SCREEN_WIDTH;
                m.display_get_size_reply.height = SCREEN_HEIGHT;
                ipc_send(m.src, &m, sizeof(m));
                break;
            }
            default:
                WARN("unknown message (type=%d)", m.type);
        }
    }
}
