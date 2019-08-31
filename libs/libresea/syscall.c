#include <resea.h>
#include <resea/ipc.h>

error_t open(cid_t *ch) {
    int cid_or_err = sys_open();
    if (cid_or_err < 0)
        return cid_or_err;
    *ch = cid_or_err;
    return OK;
}

error_t close(cid_t ch) {
    return sys_close(ch);
}

error_t link(cid_t ch1, cid_t ch2) {
    return sys_link(ch1, ch2);
}

error_t transfer(cid_t src, cid_t dst) {
    return sys_transfer(src, dst);
}

error_t ipc_recv(cid_t ch, struct message *r) {
    error_t err = sys_ipc(ch, IPC_RECV);
    if (err == OK) {
        int16_t label = MSG_LABEL(get_ipc_buffer()->header);
        if (label < 0) {
            return label;
        }
        copy_from_ipc_buffer(r);
    }
    return err;
}

error_t ipc_call(cid_t ch, struct message *m, struct message *r) {
    copy_to_ipc_buffer(m);
    error_t err = sys_ipc(ch, IPC_SEND | IPC_RECV);
    if (err == OK) {
        int16_t label = MSG_LABEL(get_ipc_buffer()->header);
        if (label < 0) {
            return label;
        }
        copy_from_ipc_buffer(r);
    }
    return err;
}

error_t ipc_replyrecv(cid_t ch, struct message *m, struct message *r) {
    copy_to_ipc_buffer(m);
    error_t err = sys_ipc(ch, IPC_SEND | IPC_RECV | IPC_REPLY);
    if (err == OK) {
        int16_t label = MSG_LABEL(get_ipc_buffer()->header);
        if (label < 0) {
            return label;
        }
        copy_from_ipc_buffer(r);
    }
    return err;
}
