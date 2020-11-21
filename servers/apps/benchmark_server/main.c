#include <config.h>
#include <resea/ipc.h>
#include <resea/printf.h>
#include <resea/malloc.h>

void main(void) {
    INFO("starting benchmark server...");
    ipc_serve("benchmark_server");

    struct message m;
    ipc_recv(IPC_ANY, &m);
    while (true) {
        switch (m.type) {
            case BENCHMARK_NOP_MSG:
                m.type = BENCHMARK_NOP_REPLY_MSG;
                break;
            case BENCHMARK_NOP_WITH_OOL_MSG:
                free(m.benchmark_nop_with_ool.data);
                m.type = BENCHMARK_NOP_WITH_OOL_REPLY_MSG;
                break;
        }

        ipc_replyrecv(m.src, &m);
    }
}
