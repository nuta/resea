#include "printk.h"
#include "arch.h"
#include "task.h"
#include <list.h>
#include <math.h>
#include <string.h>
#include <vprintf.h>

static struct klog klog;
static list_t console_readers = LIST_INIT(console_readers);
static char console_buf[128];
static int console_buf_rp = 0;
static int console_buf_wp = 0;

void handle_console_interrupt(void) {
    while (true) {
        int ch = arch_console_read();
        if (ch == -1) {
            break;
        }

        console_buf[console_buf_wp] = ch;
        console_buf_wp = (console_buf_wp + 1) % sizeof(console_buf);
        // TODO: What if it's full
    }

    LIST_FOR_EACH (task, &console_readers, struct task, next) {
        list_remove(&task->next);
        task_resume(task);
    }
}

int console_read(char *buf, int max_len) {
    int i = 0;
    while (true) {
        for (; i < max_len && console_buf_rp != console_buf_wp; i++) {
            char ch = console_buf[console_buf_rp];
            console_buf_rp = (console_buf_rp + 1) % sizeof(console_buf);
            buf[i] = ch;
        }

        if (i > 0) {
            break;
        }

        list_push_back(&console_readers, &CURRENT_TASK->next);
        task_block(CURRENT_TASK);
        task_switch();
    }

    return i;
}

/// Reads the kernel log buffer.
size_t klog_read(char *buf, size_t buf_len) {
    size_t remaining = buf_len;
    if (klog.tail > klog.head) {
        int copy_len = MIN(remaining, KLOG_BUF_SIZE - klog.tail);
        memcpy(buf, &klog.buf[klog.tail], copy_len);
        buf += copy_len;
        remaining -= copy_len;
        klog.tail = 0;
    }

    int copy_len = MIN(remaining, klog.head - klog.tail);
    memcpy(buf, &klog.buf[klog.tail], copy_len);
    remaining -= copy_len;
    klog.tail = (klog.tail + copy_len) % KLOG_BUF_SIZE;
    return buf_len - remaining;
}

/// Writes a character into the kernel log buffer.
void klog_write(char ch) {
    klog.buf[klog.head] = ch;
    klog.head = (klog.head + 1) % KLOG_BUF_SIZE;
    if (klog.head == klog.tail) {
        // The buffer is full. Discard a character by moving the tail.
        klog.tail = (klog.tail + 1) % KLOG_BUF_SIZE;
    }
}

static void printchar(__unused struct vprintf_context *ctx, char ch) {
    arch_console_write(ch);
    klog_write(ch);
}

struct vprintf_context printk_ctx = {.printchar = printchar};

/// Prints a message. See vprintf() for detailed formatting specifications.
void printf(const char *fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);
    vprintf_with_context(&printk_ctx, fmt, vargs);
    va_end(vargs);
}
