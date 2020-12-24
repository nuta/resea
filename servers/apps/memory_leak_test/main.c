#include <config.h>
#include <resea/async.h>
#include <resea/ipc.h>
#include <resea/printf.h>

void main(void) {
    ipc_serve("memory_leak_test");

    for (int i = 0; i < 10000; i++) {
        INFO("[%d] launching...", i);
        struct message m;
        m.type = TASK_LAUNCH_MSG;
        m.task_launch.name_and_cmdline = "hello";
        ipc_call(INIT_TASK, &m);
        task_t task = m.task_launch_reply.task;

        m.type = TASK_WATCH_MSG;
        m.task_watch.task = task;
        ipc_call(INIT_TASK, &m);

        ipc_recv(IPC_ANY, &m);
        if (m.type == NOTIFICATIONS_MSG) {
            async_recv(INIT_TASK, &m);
            ASSERT(m.type == TASK_EXITED_MSG);
        }
    }
}
