#include <resea.h>
#include <resea/string.h>
#include <resea_idl.h>
#include "font.h"

struct framebuffer_info {
    uint32_t *vram;
    int32_t width;
    int32_t height;
    uint8_t bpp;
};

struct framebuffer_info framebuffer;

#define COLOR(r, g, b) (((r) << 16) | ((g) << 8) | ((b) << 0))
typedef uint32_t color_t;

// Solarized light.
#define CONSOLE_NORMAL_COLOR COLOR(88, 110, 117)
#define CONSOLE_BACKGROUND_COLOR COLOR(253, 246, 227)

int console_height;
int console_width;
int console_y;
int console_x;
#define LINE_HEIGHT 16
#define CHAR_WIDTH 9

static uint32_t *pixel_at(int y, int x) {
    return &framebuffer.vram[(y * framebuffer.width + x)];
}

static void fill_rectangle(int x, int y, int height, int width, color_t color) {
    int y_max = MIN(framebuffer.height, y + height);
    int x_max = MIN(framebuffer.width, x + width);
    for (; y < y_max; y++) {
        for (int i = x; i < x_max; i++) {
            *pixel_at(y, i) = color;
        }
    }
}

void draw_char(int base_y, int base_x, char ch, color_t color) {
    if ((uint32_t) ch >= 0x80)
        return;

    const uint8_t *rows = &font[16 * ch];
    for (int y = 0; y < 16; y++) {
        uint8_t row = rows[y];
        for (int x = 0; x < 8; x++) {
            if (row & (1 << (7 - x))) {
                uint32_t *pixel = pixel_at(base_y + y, base_x + x);
                *pixel = color;
            }
        }
    }
}

static void console_write_char(char ch, int color) {
    if (ch == '\n' || console_x >= console_width) {
        console_x = 0;
        console_y++;
    }

    int bytes_per_pixel = framebuffer.bpp / 8;
    int pixels_per_line = console_width * CHAR_WIDTH * LINE_HEIGHT;
    if (console_y >= console_height) {
        // Scroll lines.
        int diff = console_y - console_height + 1;
        for (int from = diff; from < console_height; from++) {
            memcpy(framebuffer.vram + (from - diff) * pixels_per_line,
                   framebuffer.vram + from * pixels_per_line,
                   pixels_per_line * bytes_per_pixel);
        }

        // Clear the new lines.
        fill_rectangle(
            0,
            (console_height - diff) * pixels_per_line,
            pixels_per_line,
            (console_height - diff) * LINE_HEIGHT,
            CONSOLE_BACKGROUND_COLOR
        );

        console_y = console_height - 1;
    }

    if (ch != '\n' && ch != '\r') {
        draw_char(LINE_HEIGHT * console_y, console_x * CHAR_WIDTH, ch, color);
        console_x++;
    }
}

void draw_text(const char *text, int color) {
    for (int i = 0; text[i] != '\0'; i++) {
        console_write_char(text[i], color);
    }
}

static void console_init(void) {
    assert(framebuffer.bpp == 32);

    console_x = 0;
    console_y = 0;
    console_height = framebuffer.height / LINE_HEIGHT;
    console_width = framebuffer.width / CHAR_WIDTH;

    // Fill the background.
    fill_rectangle(0, 0, framebuffer.height, framebuffer.width,
                   CONSOLE_BACKGROUND_COLOR);
}

static cid_t server_ch;
static cid_t active_ch = 0;

static error_t handle_connect_request(UNUSED cid_t from,
                                      UNUSED uint8_t interface,
                                      cid_t *ch) {
    TRACE("connect_request()");
    cid_t new_ch;
    TRY_OR_PANIC(open(&new_ch));
    transfer(new_ch, server_ch);
    *ch = new_ch;
    return OK;
}

static error_t handle_console_write(UNUSED cid_t from, uint8_t ch) {
    TRACE("console_write: '%c'", ch);
    console_write_char(ch, CONSOLE_NORMAL_COLOR);
    return OK;
}

static error_t handle_activate(UNUSED cid_t from) {
    active_ch = from;
    return OK;
}

static error_t handle_on_keyinput(UNUSED cid_t from, uint8_t ch) {
    TRACE("on_keyinput: '%c'", ch);
    if (active_ch)
        send_key_event(active_ch, ch);
    return OK;
}

static void mainloop(cid_t server_ch) {
    while (1) {
        struct message m;
        TRY_OR_PANIC(ipc_recv(server_ch, &m));

        struct message r;
        error_t err;
        switch (MSG_TYPE(m.header)) {
        case CONNECT_REQUEST_MSG:
            err = dispatch_connect_request(handle_connect_request, &m, &r);
            break;
        case ON_KEYINPUT_MSG:
            err = dispatch_on_keyinput(handle_on_keyinput, &m, &r);
            break;
        case CONSOLE_WRITE_MSG:
            err = dispatch_console_write(handle_console_write, &m, &r);
            break;
        case ACTIVATE_MSG:
            err = dispatch_activate(handle_activate, &m, &r);
            break;
        default:
            WARN("invalid message type %x", MSG_TYPE(m.header));
            err = ERR_INVALID_MESSAGE;
            r.header = ERROR_TO_HEADER(err);
        }

        if (err != ERR_DONT_REPLY) {
            TRY_OR_PANIC(ipc_send(m.from, &r));
        }
    }
}

void main(void) {
    INFO("starting...");
    TRY_OR_PANIC(open(&server_ch));

    cid_t memmgr_ch = 1;

    page_base_t page_base = valloc(4096);
    size_t num_pages;
    TRY_OR_PANIC(get_framebuffer(memmgr_ch,
                                 page_base,
                                 (uintptr_t *) &framebuffer.vram,
                                 &num_pages,
                                 &framebuffer.width,
                                 &framebuffer.height,
                                 &framebuffer.bpp));
    console_init();

    cid_t kbd_ch;
    TRY_OR_PANIC(connect_server(memmgr_ch, KEYBOARD_DRIVER_INTERFACE, &kbd_ch));
    cid_t kbd_listener_ch;
    TRY_OR_PANIC(open(&kbd_listener_ch));
    TRY_OR_PANIC(transfer(kbd_listener_ch, server_ch));
    TRY_OR_PANIC(listen_keyboard(kbd_ch, kbd_listener_ch));

    cid_t discovery_ch;
    TRY_OR_PANIC(open(&discovery_ch));
    TRY_OR_PANIC(transfer(discovery_ch, server_ch));
    TRY_OR_PANIC(register_server(memmgr_ch, GUI_INTERFACE, discovery_ch));

    mainloop(server_ch);
}
