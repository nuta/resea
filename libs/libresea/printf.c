#include <resea.h>
#include <resea_idl.h>

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
        putchar(pad);
    }

    tmp[sizeof(tmp) - 1] = '\0';
    puts(&tmp[i + 1]);
}

static void print_ptr(const char **fmt, va_list vargs) {
    switch (*(*fmt)++) {
    // Hex dump.
    case 'h': {
        uint8_t *buf = va_arg(vargs, uint8_t *);
        unsigned long len = va_arg(vargs, unsigned long);
        for (unsigned long i = 0; len > 0; i++) {
            if (i % 16 == 0) {
                if (i > 0) {
                    puts("\n");
                }
                puts("\t");
                print_uint(i, 16, '0', 4);
                puts(": ");
            }
            print_uint(*buf, 16, '0', 2);
            puts(" ");
            buf++;
            len--;
        }
        break;
    }
    default:
        puts("<invalid format>");
    }
}

void vprintf(const char *fmt, va_list vargs) {
    while (*fmt) {
        if (*fmt != '%') {
            putchar(*fmt++);
        } else {
            int num_len = 1;
            int width = 0; // int
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
                    puts("<invalid format>");
                    return;
                default:
                    not_attr = true;
                }

                if (not_attr) {
                    break;
                }
            }

            switch (*fmt++) {
            case 'd':
                n = (num_len == 3) ? va_arg(vargs, long long) :
                                     va_arg(vargs, int);
                if ((intmax_t) n < 0) {
                    putchar('-');
                    n = -((intmax_t) n);
                }
                print_uint(n, 10, pad, width);
                break;
            case 'p':
#if __x86_64__
                num_len = 3;
                width = 16;
#else
#    error "unsupported CPU architecture"
#endif
                pad = '0';
                // fallthrough
            case 'x':
                alt = true;
                // Fallthrough.
            case 'u':
                n = (num_len == 3) ? va_arg(vargs, unsigned long long) :
                                     va_arg(vargs, unsigned);
                print_uint(n, alt ? 16 : 10, pad, width);
                break;
            case 'c':
                putchar(va_arg(vargs, int));
                break;
            case 's':
                puts(va_arg(vargs, char *));
                break;
            case '*':
                print_ptr(&fmt, vargs);
                break;
            case '%':
                putchar('%');
                break;
            default: // Including '\0'.
                puts("<invalid format>");
                return;
            }
        }
    }
}

void putchar(char ch) {
    // Assumes that @1 is connected to the server which suppports the runtime
    // interface.
    printchar(1, ch);
}

void puts(const char *s) {
    while (*s != '\0') {
        putchar(*s);
        s++;
    }
}

void printf(const char *fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);
    vprintf(fmt, vargs);
    va_end(vargs);
}
