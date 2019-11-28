#include <arch.h>
#include <types.h>
#include <channel.h>
#include <thread.h>
#include <process.h>
#include <support/printk.h>
#include <support/backtrace.h>

static struct kernel_log_buffer kernel_log;

void read_kernel_log(char *buf, int buf_len) {
    // We need at least one byte for the NUL terminator.
    ASSERT(buf_len > 0);

    // Create a space for the NUL terminator.
    buf_len--;

    if (kernel_log.tail > kernel_log.head) {
        int copy_len = MIN(buf_len, KERNEL_LOG_BUFFER_MAX - kernel_log.tail);
        inlined_memcpy(buf, &kernel_log.buf[kernel_log.tail], copy_len);
        buf += copy_len;
        buf_len -= copy_len;
        kernel_log.tail = 0;
    }

    int copy_len = MIN(buf_len, kernel_log.head - kernel_log.tail);
    inlined_memcpy(buf, &kernel_log.buf[kernel_log.tail], copy_len);
    buf += copy_len;
    kernel_log.tail = (kernel_log.tail + copy_len) % KERNEL_LOG_BUFFER_MAX;
    *buf = '\0';
}

static void write_kernel_log(char ch) {
    kernel_log.buf[kernel_log.head] = ch;
    kernel_log.head = (kernel_log.head + 1) % KERNEL_LOG_BUFFER_MAX;
    if (kernel_log.head == kernel_log.tail) {
        // The buffer is full. Discard a character by moving the tail.
        kernel_log.tail = (kernel_log.tail + 1) % KERNEL_LOG_BUFFER_MAX;
    }
}

static void print_str(const char *s) {
    while (*s != '\0') {
        print_char(*s);
        s++;
    }
}

void print_char(char ch) {
    arch_putchar(ch);
    write_kernel_log(ch);
}

static void print_uint(uintmax_t n, int base, char pad, int width) {
    char tmp[32];
    int i = sizeof(tmp) - 2;
    do {
        tmp[i] = "0123456789abcdef"[n % base];
        n /= base;
        width--;
        i--;
    } while (n > 0 && i > 0);

    while (width-- > 0) {
        print_char(pad);
    }

    tmp[sizeof(tmp) - 1] = '\0';
    print_str(&tmp[i + 1]);
}

static void print_ptr(const char **fmt, va_list vargs) {
    char ch = *(*fmt)++;
    switch (ch) {
    // Thread.
    case 'T': {
        struct thread *thread = va_arg(vargs, struct thread *);
        print_str("#");
        print_str(thread->process->name);
        print_str(".");
        print_uint(thread->tid, 10, ' ', 0);
        break;
    }
    // Channel.
    case 'C': {
        struct channel *ch = va_arg(vargs, struct channel *);
        print_str("@");
        print_str(ch->process->name);
        print_str(".");
        print_uint(ch->cid, 10, ' ', 0);
        break;
    }
    // A pointer value (%p) and a following character.
    default:
        print_uint((uintmax_t) va_arg(vargs, void *), 16, '0',
                   sizeof(vaddr_t) * 2);
        print_char(ch);
    }
}

///
///  The vprintf implementation for the kernel.
///
///  %c  - An ASCII character.
///  %s  - An NUL-terminated ASCII string.
///  %d  - An signed integer (in decimal).
///  %u  - An unsigned integer (in decimal).
///  %x  - A hexadecimal integer.
///  %p  - An pointer value.
///  %pT - `struct thread *`.
///  %pC - `struct channel *`.
///
void vprintk(const char *fmt, va_list vargs) {
    while (*fmt) {
        if (*fmt != '%') {
            print_char(*fmt++);
        } else {
            int num_len = 1; // int
            int width = 0;
            char pad = ' ';
            bool alt = false;
            uintmax_t n;
            bool not_attr = false;
            while (*++fmt) {
                switch (*fmt) {
                case 'l':
                    num_len++;
                    break;
                case 'h':
                    num_len--;
                    break;
                case '0':
                    pad = '0';
                    break;
                case '#':
                    alt = false;
                    break;
                case '\0':
                    print_str("<invalid format>");
                    return;
                default:
                    not_attr = true;
                }

                if (not_attr) {
                    break;
                }
            }

            if ('1' <= *fmt && *fmt <= '9') {
                width = *fmt - '0';
                fmt++;
            }

            switch (*fmt++) {
            case 'p':
                print_ptr(&fmt, vargs);
                break;
            case 'd':
                n = (num_len == 3) ? va_arg(vargs, long long) :
                                     va_arg(vargs, int);
                if ((intmax_t) n < 0) {
                    print_char('-');
                    n = -((intmax_t) n);
                }
                print_uint(n, 10, pad, width);
                break;
            case 'x':
                alt = true;
                // Fallthrough.
            case 'u':
                n = (num_len == 3) ? va_arg(vargs, unsigned long long) :
                                     va_arg(vargs, unsigned);
                print_uint(n, alt ? 16 : 10, pad, width);
                break;
            case 'c':
                print_char(va_arg(vargs, int));
                break;
            case 's':
                print_str(va_arg(vargs, char *));
                break;
            case '%':
                print_char('%');
                break;
            default: // Including '\0'.
                print_str("<invalid format>");
                return;
            }
        }
    }
}

/// Prints a message. See vprintk() for detailed formatting specifications.
void printk(const char *fmt, ...) {
    va_list vargs;

    va_start(vargs, fmt);
    vprintk(fmt, vargs);
    va_end(vargs);
}
