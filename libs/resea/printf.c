#include <resea/printf.h>
#include <resea/syscall.h>
#include <vprintf.h>

static char printbuf[PRINT_BUF_SIZE];
static int head = 0;
static int tail = 0;

static void flush(void) {
    if (tail < head) {
        klog_write(&printbuf[tail], head - tail);
        tail = head;
    } else if (tail > head) {
        klog_write(&printbuf[tail], PRINT_BUF_SIZE - tail);
        klog_write(&printbuf[0], head);
        tail = head;
    }
}

void printchar(char ch) {
    printbuf[head] = ch;
    head = (head + 1) % PRINT_BUF_SIZE;
    if ((head < tail && head + 1 == tail) || ch == '\n') {
        // The buffer is full or character is newline. Flush the buffer.
        flush();
    }
}

static void vprintf_printchar(UNUSED struct vprintf_context *ctx, char ch) {
    printchar(ch);
}

/// Prints a message. See vprintf() for detailed formatting specifications.
void printf(const char *fmt, ...) {
    struct vprintf_context ctx = { .printchar = vprintf_printchar };
    va_list vargs;
    va_start(vargs, fmt);
    vprintf(&ctx, fmt, vargs);
    va_end(vargs);
}
