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
    WARN("Backtrace:");
    uint32_t fp = read_fp();
    for (int i = 0; i < BACKTRACE_MAX; i++) {
        if (!fp) {
            break;
        }

        struct stack_frame *frame =
            (struct stack_frame *) (fp - sizeof(*frame));
        WARN("    #%d: @sym<%p>", i, frame->ra);
        fp = frame->fp;
    }
}
