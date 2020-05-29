#include <resea/ipc.h>
#include <resea/printf.h>
#include "fs.h"
#include "tty.h"

#define QUEUE_LEN 32
static task_t kbd_server;
static task_t display_server;
static char queue[QUEUE_LEN];
static int rp = 0;
static int wp = 0;
static int x = 0;
static int y = 0;

static struct inode *tty_inode;

static void update_cursor() {
    struct message m;
    m.type = TEXTSCREEN_MOVE_CURSOR_MSG;
    m.textscreen_move_cursor.y = y;
    m.textscreen_move_cursor.x = x;
    ipc_send(display_server, &m);
}

static void putc(char ch) {
    if (ch == '\n') {
        x = 0;
        y++;
        update_cursor();
        return;
    }

    struct message m;
    m.type = TEXTSCREEN_DRAW_CHAR_MSG;
    m.textscreen_draw_char.ch = ch;
    m.textscreen_draw_char.x = x++;
    m.textscreen_draw_char.y = y;
    m.textscreen_draw_char.fg_color = COLOR_NORMAL;
    m.textscreen_draw_char.bg_color = COLOR_BLACK;
    ipc_send(display_server, &m);
    update_cursor();
}

void on_new_data(void) {
    struct message m;
    m.type = KBD_READ_MSG;
    error_t err = ipc_call(kbd_server, &m);
    if (err == ERR_EMPTY) {
        return;
    }

    ASSERT_OK(err);
    ASSERT(m.type == KBD_READ_REPLY_MSG);

    char ch = m.kbd_read_reply.keycode;
    queue[wp++ % QUEUE_LEN] = ch;
    putc(ch);
    waitqueue_wake_all(&tty_inode->read_wq);
}

static ssize_t read(struct file *file, uint8_t *buf, size_t len) {
    if (rp == wp) {
        return -EAGAIN;
    }

    size_t i = 0;
    while (i < len) {
        if (rp == wp) {
            break;
        }

        buf[i++] = queue[rp++ % QUEUE_LEN];
    }

    return i;
}

static ssize_t write(struct file *file, const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        putc(buf[i]);
    }

    return len;
}

static int acquire(struct file *file) {
    static bool inited = false;
    if (inited) {
        return 0;
    }

    tty_inode = file->inode;
    kbd_server = ipc_lookup("ps2kbd");
    ASSERT_OK(kbd_server);
    display_server = ipc_lookup("display");
    ASSERT_OK(display_server);

    struct message m;
    m.type = TEXTSCREEN_CLEAR_MSG;
    ipc_send(display_server, &m);

    m.type = KBD_LISTEN_MSG;
    ipc_call(kbd_server, &m);

    inited = true;
    return 0;
}

static int release(UNUSED struct file *file) {
    NYI();
    return 0;
}

static ssize_t ioctl(UNUSED struct file *file, UNUSED unsigned cmd, UNUSED unsigned arg) {
    NYI();
    return 0;
}

static loff_t seek(UNUSED struct file *file, UNUSED loff_t off, UNUSED int whence) {
    NYI();
    return 0;
}

struct file_ops tty_file_ops = {
    .acquire = acquire,
    .release = release,
    .read = read,
    .write = write,
    .ioctl = ioctl,
    .seek = seek,
};
