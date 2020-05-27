#include "printk.h"
#include "ipc.h"
#include <cstring.h>
#include <vprintf.h>

static struct klog klog;
static struct task *listener = NULL;

/// Reads the kernel log buffer.
size_t klog_read(char *buf, size_t buf_len) {
    size_t remaining = buf_len;
    if (klog.tail > klog.head) {
        int copy_len = MIN(remaining, CONFIG_KLOG_BUF_SIZE - klog.tail);
        memcpy(buf, &klog.buf[klog.tail], copy_len);
        buf += copy_len;
        remaining -= copy_len;
        klog.tail = 0;
    }

    int copy_len = MIN(remaining, klog.head - klog.tail);
    memcpy(buf, &klog.buf[klog.tail], copy_len);
    remaining -= copy_len;
    klog.tail = (klog.tail + copy_len) % CONFIG_KLOG_BUF_SIZE;
    return buf_len - remaining;
}

/// Writes a character into the kernel log buffer.
void klog_write(char ch) {
    klog.buf[klog.head] = ch;
    klog.head = (klog.head + 1) % CONFIG_KLOG_BUF_SIZE;
    if (klog.head == klog.tail) {
        // The buffer is full. Discard a character by moving the tail.
        klog.tail = (klog.tail + 1) % CONFIG_KLOG_BUF_SIZE;
    }

    if (ch == '\n' && listener) {
        notify(listener, NOTIFY_NEW_DATA);
    }
}

void klog_listen(struct task *task) {
    listener = task;
}

void klog_unlisten(void) {
    listener = NULL;
}

static void printchar(UNUSED struct vprintf_context *ctx, char ch) {
    arch_printchar(ch);
    klog_write(ch);
}

/// Prints a message. See vprintf() for detailed formatting specifications.
void printk(const char *fmt, ...) {
    struct vprintf_context ctx = { .printchar = printchar };
    va_list vargs;
    va_start(vargs, fmt);
    vprintf(&ctx, fmt, vargs);
    va_end(vargs);
}
