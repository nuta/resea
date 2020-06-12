#include <config.h>
#include <resea/printf.h>
#include <resea/ipc.h>
#include <resea/timer.h>

void main(void) {
    INFO("Hello, World!");

#ifdef CONFIG_PRINT_PERIODICALLY
    timer_set(1000 /* in milliseconds */);
    unsigned i = 1;
    while (true) {
        struct message m;
        ipc_recv(IPC_ANY, &m);
        if (m.type == NOTIFICATIONS_MSG) {
            TRACE("Hello, World! (i=%d)", i++);
            timer_set(1000);
        }
    }
#endif
}
