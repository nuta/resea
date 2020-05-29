#include <resea/ipc.h>
#include <resea/printf.h>

#define TASK_A_TID 1
#define TASK_B_TID 2

static void task_b(void) {
    INFO("starting task B...");
    struct message m;
    m.type = NOP_MSG;
    m.nop.value = 123;
    ipc_reply(TASK_A_TID, &m);
    ipc_recv(TASK_A_TID, &m);
    ASSERT(m.nop.value == 456);

    timer_set(1000);
    while (true) {
        struct message m;
        ipc_recv(IPC_ANY, &m);
        switch (m.type) {
            case NOTIFICATIONS_MSG:
                TRACE("task B");
                timer_set(1000);
                break;
        }
    }
}

void main(void) {
    INFO("starting task A...");
    task_create(TASK_B_TID, "task_b", (vaddr_t) task_b, task_self(), CAP_ALL);
    timer_set(1000);
    while (true) {
        struct message m;
        ipc_recv(IPC_ANY, &m);
        switch (m.type) {
            case NOTIFICATIONS_MSG:
                TRACE("task A");
                timer_set(1000);
                break;
            case NOP_MSG:
                ASSERT(m.src == TASK_B_TID);
                ASSERT(m.nop.value == 123);
                m.type = NOP_MSG;
                m.nop.value = 456;
                ipc_reply(m.src, &m);
                break;
            default:
                WARN("unexpected message: %d", m.type);
        }
    }
}
