#include <string.h>
#include "kdebug.h"
#include "task.h"
#include "printk.h"
#include "ipc.h"

error_t kdebug_run(const char *cmdline, char *buf, size_t len) {
    if (strlen(cmdline) == 0) {
        // An empty command. Just ignore it.
        return OK;
    } else if (strcmp(cmdline, "help") == 0) {
        INFO("Kernel debugger commands:");
        INFO("");
        INFO("  ps - List tasks.");
        INFO("  q  - Quit the emulator.");
        INFO("");
    } else if (strcmp(cmdline, "ps") == 0) {
        task_dump();
    } else if (strcmp(cmdline, "q") == 0) {
#ifdef CONFIG_SEMIHOSTING
        arch_semihosting_halt();
#endif
        PANIC("halted by the kdebug");
    } else if (strcmp(cmdline, "_log") == 0) {
        if (!len) {
            return ERR_TOO_SMALL;
        }

        int read_len = klog_read(buf, len - 1);
        buf[read_len - 1] = '\0';
    } else {
        WARN("Invalid debugger command: '%s'.", cmdline);
        return ERR_NOT_FOUND;
    }

    return OK;
}

static uint32_t *get_canary_ptr(void) {
    vaddr_t sp = (vaddr_t) __builtin_frame_address(0);
    return (uint32_t *) ALIGN_DOWN(sp, STACK_SIZE);
}

/// Writes the stack canary at the borrom of the current kernel stack.
void stack_set_canary(void) {
    *get_canary_ptr() = STACK_CANARY_VALUE;
}

/// Checks that the kernel stack canary is still alive.
void stack_check(void) {
    if (*get_canary_ptr() != STACK_CANARY_VALUE) {
        PANIC("the kernel stack has been exhausted");
    }
}
