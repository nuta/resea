#include <resea.h>
#include <server.h>
#include <idl_stubs.h>

static cid_t memmgr_ch = 1;

void main(void) {
    cid_t server_ch;
    TRY_OR_PANIC(open(&server_ch));
    TRY_OR_PANIC(server_register(memmgr_ch, server_ch, BENCHMARK_INTERFACE));

    INFO("ready");

    struct message *m = get_ipc_buffer();

    // Handle the server.connect message from the discovery server.
    cid_t client_ch;
    TRY_OR_PANIC(sys_ipc(server_ch, IPC_RECV));
    TRY_OR_PANIC(open(&client_ch));
    TRY_OR_PANIC(transfer(client_ch, server_ch));
    assert(m->header == SERVER_CONNECT_MSG);
    m->header = SERVER_CONNECT_REPLY_MSG;
    m->payloads.server.connect_reply.ch = client_ch;

    // Receive a message from the benchmark client and return the message as it
    // is.
    while (true) {
        TRY_OR_PANIC(sys_ipc(m->from, IPC_SEND | IPC_RECV));
    }
}
