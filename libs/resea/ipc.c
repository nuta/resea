#include <print_macros.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/syscall.h>
#include <string.h>

error_t ipc_send(task_t dst, struct message *m) {
    return sys_ipc(dst, 0, m, IPC_SEND);
}

error_t ipc_send_noblock(task_t dst, struct message *m) {
    return sys_ipc(dst, 0, m, IPC_SEND | IPC_NOBLOCK);
}

void ipc_reply(task_t dst, struct message *m) {
    error_t err = ipc_send_noblock(dst, m);
    OOPS_OK(err);
}

void ipc_reply_err(task_t dst, error_t error) {
    struct message m;  // FIXME:
    m.type = error;
    ipc_send_noblock(dst, &m);
}

error_t ipc_recv(task_t src, struct message *m) {
    error_t err = sys_ipc(0, src, m, IPC_RECV);
    // Handle the case when m.type is negative: a message represents an error
    // (sent by `ipc_send_err()`).
    return (IS_OK(err) && m->type < 0) ? m->type : err;
}

error_t ipc_call(task_t dst, struct message *m) {
    error_t err = sys_ipc(dst, dst, m, IPC_CALL);
    // Handle the case when m.type is negative: a message represents an error
    // (sent by `ipc_send_err()`).
    return (IS_OK(err) && m->type < 0) ? m->type : err;
}
