#include <print_macros.h>
#include <types.h>

struct stack_frame {
    uint32_t fp;
    uint32_t ra;
} __packed;

uint32_t read_fp(void) {
    uint32_t fp;
    __asm__ __volatile__("mv %0, fp" : "=r"(fp));
    return fp;
}

/// Prints the stack trace.
void backtrace(void) {
    uint32_t fp = read_fp();
    if (!fp) {
        WARN("backtrace: fp is NULL");
        return;
    }

    for (int i = 0; i < BACKTRACE_MAX; i++) {
        if (!fp) {
            break;
        }

        struct stack_frame *frame =
            (struct stack_frame *) (fp - sizeof(*frame));
        TRACE("{{{bt:%d:0x%p:ra}}}", i, frame->ra);
        fp = frame->fp;
    }
}
