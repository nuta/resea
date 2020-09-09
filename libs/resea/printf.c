#include <resea/printf.h>
#include <resea/syscall.h>
#include <vprintf.h>

static char printbuf[PRINT_BUF_SIZE];
static int head = 0;
static int tail = 0;

void printf_flush(void) {
    if (tail < head) {
        sys_console_write(&printbuf[tail], head - tail);
        tail = head;
    } else if (tail > head) {
        sys_console_write(&printbuf[tail], PRINT_BUF_SIZE - tail);
        sys_console_write(&printbuf[0], head);
        tail = head;
    }
}

static void printchar(char ch) {
    printbuf[head] = ch;
    head = (head + 1) % PRINT_BUF_SIZE;
    if ((head < tail && head + 1 == tail) || ch == '\n') {
        // The buffer is full or character is newline. Flush the buffer.
        printf_flush();
    }
}

static void vprintf_printchar(__unused struct vprintf_context *ctx, char ch) {
    printchar(ch);
}

/// Prints a message. See vprintf() for detailed formatting specifications.
void printf(const char *fmt, ...) {
    struct vprintf_context ctx = { .printchar = vprintf_printchar };
    va_list vargs;
    va_start(vargs, fmt);
    vprintf_with_context(&ctx, fmt, vargs);
    va_end(vargs);
}
