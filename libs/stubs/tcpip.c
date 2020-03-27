#include <message.h>
#include <std/lookup.h>
#include <std/malloc.h>
#include <std/printf.h>
#include <std/syscall.h>
#include <cstring.h>
#include <stubs/tcpip.h>

static task_t server = 0;

error_t tcpip_init(void) {
    task_t tid = ipc_lookup("tcpip");
    if (IS_ERROR(tid)) {
        return tid;
    }

    server = tid;
    return OK;
}

task_t tcpip_server(void) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");
    return server;
}

handle_t tcpip_listen(uint16_t port, int backlog) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");

    struct message m;
    m.type = TCPIP_LISTEN_MSG;
    m.tcpip_listen.port = port;
    m.tcpip_listen.backlog = backlog;
    error_t err = ipc_call(server, &m);
    if (err != OK) {
        return err;
    }

    return m.tcpip_listen_reply.handle;
}

handle_t tcpip_accept(handle_t handle) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");

    struct message m;
    m.type = TCPIP_ACCEPT_MSG;
    m.tcpip_accept.handle = handle;
    error_t err = ipc_call(server, &m);
    if (err != OK) {
        return err;
    }

    return m.tcpip_accept_reply.new_handle;
}

error_t tcpip_write(handle_t handle, const void *data, size_t len) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");

    struct message m;
    m.type = TCPIP_WRITE_MSG;
    m.tcpip_write.handle = handle;
    m.tcpip_write.data = (void *) data;
    m.tcpip_write.len = len;
    return ipc_call(server, &m);
}

void *tcpip_read(handle_t handle, size_t *len) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");

    struct message m;
    m.type = TCPIP_READ_MSG;
    m.tcpip_read.handle = handle;
    m.tcpip_read.len = *len;
    error_t err = ipc_call(server, &m);
    if (IS_ERROR(err)) {
        return NULL;
    }

    *len = m.tcpip_read_reply.len;
    return m.tcpip_read_reply.data;
}

error_t tcpip_close(handle_t handle) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");

    struct message m;
    m.type = TCPIP_CLOSE_MSG;
    m.tcpip_close.handle = handle;
    return ipc_call(server, &m);
}
