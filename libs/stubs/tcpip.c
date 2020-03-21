#include <message.h>
#include <std/lookup.h>
#include <std/malloc.h>
#include <std/printf.h>
#include <std/syscall.h>
#include <string.h>
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
    m.tcpip.listen.port = port;
    m.tcpip.listen.backlog = backlog;
    error_t err = ipc_call(server, &m);
    if (err != OK) {
        return err;
    }

    return m.tcpip.listen_reply.handle;
}

handle_t tcpip_accept(handle_t handle) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");

    struct message m;
    m.type = TCPIP_ACCEPT_MSG;
    m.tcpip.accept.handle = handle;
    error_t err = ipc_call(server, &m);
    if (err != OK) {
        return err;
    }

    return m.tcpip.accept_reply.new_handle;
}

error_t tcpip_write(handle_t handle, const void *data, size_t len) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");
    ASSERT(len <= TCP_DATA_LEN_MAX);

    struct message m;
    m.type = TCPIP_WRITE_MSG;
    m.tcpip.write.handle = handle;
    m.tcpip.write.len = len;
    memcpy(&m.tcpip.write.data, data, len);
    return ipc_call(server, &m);
}

error_t tcpip_read(handle_t handle, void *buf, size_t buf_len) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");

    struct message m;
    m.type = TCPIP_READ_MSG;
    m.tcpip.read.handle = handle;
    m.tcpip.read.len = buf_len;
    error_t err = ipc_call(server, &m);
    if (IS_ERROR(err)) {
        return err;
    }

    memcpy(buf, m.tcpip.read_reply.data, MIN(m.tcpip.read_reply.len, buf_len));
    return err;
}

error_t tcpip_close(handle_t handle) {
    DEBUG_ASSERT(server && "tcpip_init() is not called");

    struct message m;
    m.type = TCPIP_CLOSE_MSG;
    m.tcpip.close.handle = handle;
    return ipc_call(server, &m);
}
