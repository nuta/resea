#include <message.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/syscall.h>

/// The internal buffer to receive bulk payloads.
static void *bulk_ptr = NULL;
static const size_t bulk_len = CONFIG_BULK_BUFFER_LEN;

error_t task_create(task_t tid, const char *name, vaddr_t ip, task_t pager,
                    caps_t caps) {
    return syscall(SYS_SPAWN, tid, (uintptr_t) name, ip, pager, caps);
}

error_t sys_kill(task_t tid) {
    return syscall(SYS_KILL, tid, 0, 0, 0, 0);
}

static task_t sys_setattrs(const void *bulk_ptr, size_t bulk_len, msec_t timeout) {
    return syscall(SYS_SETATTRS, (uint64_t) bulk_ptr, bulk_len, timeout,
                   0, 0);
}

static error_t sys_ipc(task_t dst, task_t src, struct message *m, unsigned flags) {
    return syscall(SYS_IPC, dst, src, (uintptr_t) m, flags, 0);
}

static error_t irq_listen(unsigned irq, task_t listener) {
    return syscall(SYS_LISTENIRQ, irq, listener, 0, 0, 0);
}

error_t klog_write(const char *buf, size_t len) {
    return syscall(SYS_WRITELOG, (uintptr_t) buf, len, 0, 0, 0);
}

int klog_read(char *buf, size_t len, bool listen) {
    return syscall(SYS_READLOG, (uintptr_t) buf, len, listen, 0, 0);
}

error_t task_destroy(task_t task) {
    return sys_kill(task);
}

void task_exit(void) {
    sys_kill(0);
}

task_t task_self(void) {
    return sys_setattrs(NULL, 0, 0);
}

error_t ipc_send(task_t dst, struct message *m) {
    return sys_ipc(dst, 0, m, IPC_SEND);
}

error_t ipc_send_noblock(task_t dst, struct message *m) {
    return sys_ipc(dst, 0, m, IPC_SEND | IPC_NOBLOCK);
}

error_t ipc_send_err(task_t dst, error_t error) {
    struct message m;
    m.type = error;
    return ipc_send(dst, &m);
}

void ipc_reply(task_t dst, struct message *m) {
    ipc_send_noblock(dst, m);
}

void ipc_reply_err(task_t dst, error_t error) {
    struct message m;
    m.type = error;
    ipc_send_noblock(dst, &m);
}

error_t ipc_notify(task_t dst, notifications_t notifications) {
    return sys_ipc(dst, 0, (void *) (uintptr_t) notifications, IPC_NOTIFY);
}

error_t ipc_recv(task_t src, struct message *m) {
    if (!bulk_ptr) {
        bulk_ptr = malloc(bulk_len);
        ASSERT_OK(sys_setattrs(bulk_ptr, bulk_len, 0));
    }

    error_t err = sys_ipc(0, src, m, IPC_RECV);

    if (MSG_BULK_PTR(m->type)) {
        bulk_ptr = NULL;
    }

    // Handle the case when m.type is negative: a message represents an error
    // (sent by `ipc_send_err()`).
    return (IS_OK(err) && m->type < 0) ? m->type : err;
}

error_t ipc_call(task_t dst, struct message *m) {
    if (!bulk_ptr) {
        bulk_ptr = malloc(bulk_len);
        ASSERT_OK(sys_setattrs(bulk_ptr, bulk_len, 0));
    }

    error_t err = sys_ipc(dst, dst, m, IPC_CALL);

    if (MSG_BULK_PTR(m->type)) {
        bulk_ptr = NULL;
    }

    // Handle the case when `r->type` is negative: a message represents an error
    // (sent by `ipc_send_err()`).
    return (IS_OK(err) && m->type < 0) ? m->type : err;
}

error_t timer_set(msec_t timeout) {
    return sys_setattrs(bulk_ptr, bulk_len, timeout);
}

error_t irq_acquire(unsigned irq) {
    return irq_listen(irq, task_self());
}
error_t irq_release(unsigned irq) {
    return irq_listen(irq, 0);
}

void nop_syscall(void) {
    syscall(SYS_NOP, 0, 0, 0, 0, 0);
}
