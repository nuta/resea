#include <resea/ipc.h>
#include <resea/syscall.h>
#include <resea/printf.h>
#include <driver/irq.h>
#include "fs.h"
#include "tty.h"

#define QUEUE_LEN 32
static char queue[QUEUE_LEN];
static int rp = 0;
static int wp = 0;

static struct inode *tty_inode;

void on_new_data(void) {
    char buf[256];
    OOPS_OK(sys_console_read(buf, sizeof(buf)));
    for (size_t i = 0; i < sizeof(buf) && buf[i] != '\0'; i++) {
        queue[wp++ % QUEUE_LEN] = buf[i];
        printf("%c", buf[i]);
    }

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
        printf("%c", buf[i]);
    }

    return len;
}

static int acquire(struct file *file) {
    static bool inited = false;
    if (inited) {
        return 0;
    }

    ASSERT_OK(irq_acquire(CONSOLE_IRQ));
    tty_inode = file->inode;
    inited = true;
    return 0;
}

static int release(__unused struct file *file) {
    NYI();
    return 0;
}

static ssize_t ioctl(__unused struct file *file, __unused unsigned cmd, __unused unsigned arg) {
    NYI();
    return 0;
}

static loff_t seek(__unused struct file *file, __unused loff_t off, __unused int whence) {
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
