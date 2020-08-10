#include <idl.h>
#include "vprintf.h"
#include "print_macros.h"

static void puts(struct vprintf_context *ctx, const char *s) {
    while (*s != '\0') {
        ctx->printchar(ctx, *s);
        s++;
    }
}

static void print_uint(struct vprintf_context *ctx, uintmax_t n, int base,
                       char pad, int width) {
    char tmp[32];
    int i = sizeof(tmp) - 2;
    do {
        tmp[i] = "0123456789abcdef"[n % base];
        n /= base;
        width--;
        i--;
    } while (n > 0 && i > 0);

    while (width-- > 0) {
        ctx->printchar(ctx, pad);
    }

    tmp[sizeof(tmp) - 1] = '\0';
    puts(ctx, &tmp[i + 1]);
}

///  The vprintf implementation.
///
///  %c  - An ASCII character.
///  %s  - An NUL-terminated ASCII string.
///  %d  - An signed integer (in decimal).
///  %u  - An unsigned integer (in decimal).
///  %x  - A hexadecimal integer.
///  %p  - An pointer value.
///  %pI - An IPv4 address (uint32_t).
void vprintf_with_context(struct vprintf_context *ctx, const char *fmt, va_list vargs) {
    while (*fmt) {
        if (*fmt != '%') {
            ctx->printchar(ctx, *fmt++);
        } else {
            int num_len = 1;  // int
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
                        puts(ctx, "<invalid format>");
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
                case 'd':
                    n = (num_len == 3) ? va_arg(vargs, long long)
                                       : va_arg(vargs, int);
                    if ((intmax_t) n < 0) {
                        ctx->printchar(ctx, '-');
                        n = -((intmax_t) n);
                    }
                    print_uint(ctx, n, 10, pad, width);
                    break;
                case 'p': {
                    char ch = *fmt++;
                    switch (ch) {
                        // An IP address.
                        case 'I': {
                            char ch2 = *fmt++;
                            switch (ch2) {
                                // IPV4 address.
                                case '4': {
                                    uint32_t v4addr = va_arg(vargs, uint32_t);
                                    print_uint(ctx, (v4addr >> 24) & 0xff, 10,
                                               ' ', 0);
                                    ctx->printchar(ctx, '.');
                                    print_uint(ctx, (v4addr >> 16) & 0xff, 10,
                                               ' ', 0);
                                    ctx->printchar(ctx, '.');
                                    print_uint(ctx, (v4addr >> 8) & 0xff, 10,
                                               ' ', 0);
                                    ctx->printchar(ctx, '.');
                                    print_uint(ctx, v4addr & 0xff, 10, ' ', 0);
                                    break;
                                }
                                default:
                                    // Invalid format specifier.
                                    puts(ctx, "%pI");
                                    ctx->printchar(ctx, ch2);
                                    break;
                            }
                            break;
                        }
                        // A pointer value (%p) and a following character.
                        default:
                            print_uint(ctx, (uintmax_t) va_arg(vargs, void *),
                                       16, '0', sizeof(vaddr_t) * 2);
                            fmt --;
                    }
                    break;
                }
                case 'x':
                    alt = true;
                    // Fallthrough.
                case 'u':
                    n = (num_len == 3) ? va_arg(vargs, unsigned long long)
                                       : va_arg(vargs, unsigned);
                    print_uint(ctx, n, alt ? 16 : 10, pad, width);
                    break;
                case 'c':
                    ctx->printchar(ctx, va_arg(vargs, int));
                    break;
                case 's': {
                    char *s = va_arg(vargs, char *);
                    puts(ctx, s ? s : "(null)");
                    break;
                }
                case '%':
                    ctx->printchar(ctx, '%');
                    break;
                default:  // Including '\0'.
                    puts(ctx, "<invalid format>");
                    return;
            }
        }
    }
}

static void snprintf_printchar(struct vprintf_context *ctx, char ch) {
    if (ctx->index == ctx->size) {
        // The buffer is full.
        return;
    }

    ctx->buf[ctx->index] = ch;
    ctx->index++;
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list vargs) {
    if (!size) {
        // We need room for the trailing NUL character.
        return 0;
    }

    struct vprintf_context ctx = {
        .printchar = snprintf_printchar,
        .buf = buf,
        .index = 0,
        .size = size,
    };

    vprintf_with_context(&ctx, fmt, vargs);
    buf[size - 1] = '\0';
    return ctx.index;
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);
    int ret = vsnprintf(buf, size, fmt, vargs);
    va_end(vargs);
    return ret;
}

void printf_with_context(struct vprintf_context *ctx, const char *fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);
    vprintf_with_context(ctx, fmt, vargs);
    va_end(vargs);
}

/// The symbol table. embed-symbols.py fills the symbol table and set `magic` to
/// SYMBOL_TABLE_MAGIC.
static const struct symbol_table symbol_table = {
    .magic = SYMBOL_TABLE_EMPTY,
    // FIXME: clang optimizes this into a constant!
    .num_symbols = CONFIG_NUM_SYMBOLS,
};

/// Resolves a symbol name and the offset from the beginning of symbol.
/// This function returns NULL if the symbol does not
/// exist in the symbol table.
static const struct symbol *find_symbol(vaddr_t vaddr) {
    // Do a binary search.
    int32_t l = -1;
    int32_t r = symbol_table.num_symbols;
    while (r - l > 1) {
        int32_t mid = (l + r) / 2;
        if (vaddr >= symbol_table.symbols[mid].addr) {
            l = mid;
        } else {
            r = mid;
        }
    }

    return (l < 0) ? NULL : &symbol_table.symbols[l];
}

/// Prints the stack trace.
void backtrace(void) {
    WARN("Backtrace:");
    struct stack_frame *frame = __builtin_frame_address(0);
    for (int i = 0; i < BACKTRACE_MAX; i++) {
        if (!frame) {
            break;
        }

        const struct symbol *symbol = find_symbol(frame->return_addr);
        const char *name;
        size_t offset;
        if (symbol) {
            name = symbol->name;
            offset = frame->return_addr - symbol->addr;
        } else {
            name = "(invalid address)";
            offset = 0;
        }

        WARN("    #%d: %p %s()+0x%x", i, frame->return_addr, name, offset);
        frame = frame->next;
    }
}

static const char *error_names[] = {
    [OK] = "OK",
    [-ERR_NO_MEMORY] = "No Memory",
    [-ERR_NOT_PERMITTED] = "Not Permitted",
    [-ERR_WOULD_BLOCK] = "Would Block",
    [-ERR_ABORTED] = "Aborted",
    [-ERR_TOO_LARGE] = "Too Large",
    [-ERR_TOO_SMALL] = "Too Small",
    [-ERR_NOT_FOUND] = "Not Found",
    [-ERR_INVALID_ARG] = "Invalid Arg",
    [-ERR_INVALID_TASK] = "Invalid Task ID",
    [-ERR_ALREADY_EXISTS] = "Already Exists",
    [-ERR_UNAVAILABLE] = "Unavailable",
    [-ERR_EMPTY] = "Empty",
    [-ERR_NOT_ACCEPTABLE] = "Not Acceptable",
    [-ERR_IN_USE] = "In Use",
    [-ERR_TRY_AGAIN] = "Try Again",
};

const char *err2str(error_t err) {
    if (err < ERR_END || err > 0) {
        return "INVALID_ERR_CODE";
    }

    return error_names[-err];
}

const char *msgtype2str(int type) {
    if (type < 0) {
        return err2str(type);
    }

    int id = MSG_ID(type);
    if (id == 0 || id > IDL_MSGID_MAX) {
        return "(invalid)";
    }

    return IDL_MSGID2STR[id];
}
