#include <resea.h>
#include <resea/math.h>
#include <resea/ipc.h>

struct message *get_ipc_buffer(void) {
    return &get_thread_info()->ipc_buffer;
}

void set_page_base(page_base_t page_base) {
    struct thread_info *info = get_thread_info();
    info->page_base = page_base;
}

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
        int16_t type = MSG_TYPE(get_ipc_buffer()->header);
        if (type < 0) {
            return type;
        }
        copy_from_ipc_buffer(r);
    }
    return err;
}

error_t ipc_send(cid_t ch, struct message *m) {
    copy_to_ipc_buffer(m);
    return sys_ipc(ch, IPC_SEND);
}

error_t ipc_async_send(cid_t ch, struct message *m) {
    copy_to_ipc_buffer(m);
    return sys_ipc(ch, IPC_SEND | IPC_NOBLOCK);
}

error_t ipc_call(cid_t ch, struct message *m, struct message *r) {
    copy_to_ipc_buffer(m);
    error_t err = sys_ipc(ch, IPC_SEND | IPC_RECV);
    if (err == OK) {
        int16_t type = MSG_TYPE(get_ipc_buffer()->header);
        if (type < 0) {
            return type;
        }
        copy_from_ipc_buffer(r);
    }
    return err;
}
