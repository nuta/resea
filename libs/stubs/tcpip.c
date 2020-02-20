#include <message.h>
#include <std/lookup.h>
#include <std/malloc.h>
#include <std/printf.h>
#include <std/syscall.h>
#include <string.h>
#include <stubs/tcpip.h>

static tid_t server = 0;

error_t tcpip_init(void) {
    tid_t tid = ipc_lookup("tcpip");
    if (IS_ERROR(tid)) {
        return tid;
    }

    server = tid;
    return OK;
}

tid_t tcpip_server(void) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");
    return server;
}

handle_t tcpip_listen(uint16_t port, int backlog) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");

    struct message m;
    m.type = TCP_LISTEN_MSG;
    m.tcp_listen.port = port;
    m.tcp_listen.backlog = backlog;
    error_t err = ipc_call(server, &m);
    if (err != OK) {
        return err;
    }

    return m.tcp_listen_reply.handle;
}

handle_t tcpip_accept(handle_t handle) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");

    struct message m;
    m.type = TCP_ACCEPT_MSG;
    m.tcp_accept.handle = handle;
    error_t err = ipc_call(server, &m);
    if (err != OK) {
        return err;
    }

    return m.tcp_accept_reply.new_handle;
}

error_t tcpip_write(handle_t handle, const void *data, size_t len) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");
    ASSERT(len <= TCP_DATA_LEN_MAX);

    struct message m;
    m.type = TCP_WRITE_MSG;
    m.tcp_write.handle = handle;
    m.tcp_write.len = len;
    memcpy(&m.tcp_write.data, data, len);
    return ipc_call(server, &m);
}

error_t tcpip_read(handle_t handle, void *buf, size_t buf_len) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");

    struct message m;
    m.type = TCP_READ_MSG;
    m.tcp_read.handle = handle;
    m.tcp_read.len = buf_len;
    error_t err = ipc_call(server, &m);
    if (IS_ERROR(err)) {
        return err;
    }

    memcpy(buf, m.tcp_read_reply.data, MIN(m.tcp_read_reply.len, buf_len));
    return err;
}

error_t tcpip_close(handle_t handle) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");

    struct message m;
    m.type = TCP_CLOSE_MSG;
    m.tcp_close.handle = handle;
    return ipc_call(server, &m);
}
