#include <message.h>
#include <std/malloc.h>
#include <std/printf.h>
#include <std/syscall.h>

/// The internal buffer to receive bulk payloads.
static void *bulk_ptr = NULL;
static const size_t bulk_len = 8192;

error_t ipc(tid_t dst, tid_t src, struct message *m, unsigned flags) {
    return syscall(SYSCALL_IPC, dst, src, (uintptr_t) m, flags, 0);
}

error_t ipcctl(const void *bulk_ptr, size_t bulk_len, msec_t timeout) {
    return syscall(SYSCALL_IPCCTL, (uint64_t) bulk_ptr, bulk_len, timeout,
                   0, 0);
}

tid_t taskctl(tid_t tid, const char *name, vaddr_t ip, tid_t pager,
              caps_t caps) {
    return syscall(SYSCALL_TASKCTL, tid, (uintptr_t) name, ip, pager, caps);
}

error_t irqctl(unsigned irq, bool enable) {
    return syscall(SYSCALL_IRQCTL, irq, enable, 0, 0, 0);
}

int klogctl(char *buf, size_t buf_len, bool write) {
    return syscall(SYSCALL_KLOGCTL, (uintptr_t) buf, buf_len, write, 0, 0);
}

tid_t task_create(tid_t tid, const char *name, vaddr_t ip, tid_t pager,
                  caps_t caps) {
    DEBUG_ASSERT(tid && pager);
    return taskctl(tid, name, ip, pager, caps);
}

error_t task_destroy(tid_t tid) {
    DEBUG_ASSERT(tid);
    return taskctl(tid, NULL, 0, 0, 0);
}

void task_exit(void) {
    taskctl(0, NULL, 0, 0, 0);
}

tid_t task_self(void) {
    return taskctl(0, NULL, 0, -1, 0);
}

void caps_drop(caps_t caps) {
    taskctl(0, NULL, 0, -1, caps);
}

error_t ipc_send(tid_t dst, struct message *m) {
    return ipc(dst, 0, m, IPC_SEND);
}

error_t ipc_send_noblock(tid_t dst, struct message *m) {
    return ipc(dst, 0, m, IPC_SEND | IPC_NOBLOCK);
}

error_t ipc_send_err(tid_t dst, error_t error) {
    struct message m;
    m.type = error;
    return ipc_send(dst, &m);
}

void ipc_reply(tid_t dst, struct message *m) {
    ipc_send_noblock(dst, m);
}

void ipc_reply_err(tid_t dst, error_t error) {
    struct message m;
    m.type = error;
    ipc_send_noblock(dst, &m);
}

error_t ipc_recv(tid_t src, struct message *m) {
    if (!bulk_ptr) {
        bulk_ptr = malloc(bulk_len);
        ASSERT_OK(ipcctl(bulk_ptr, bulk_len, 0));
    }

    error_t err = ipc(0, src, m, IPC_RECV);

    if (MSG_BULK_PTR(m->type)) {
        bulk_ptr = NULL;
    }

    // Handle the case when m.type is negative: a message represents an error
    // (sent by `ipc_send_err()`).
    return (IS_OK(err) && m->type < 0) ? m->type : err;
}

error_t ipc_call(tid_t dst, struct message *m) {
    if (!bulk_ptr) {
        bulk_ptr = malloc(bulk_len);
        ASSERT_OK(ipcctl(bulk_ptr, bulk_len, 0));
    }

    error_t err = ipc(dst, dst, m, IPC_CALL);

    if (MSG_BULK_PTR(m->type)) {
        bulk_ptr = NULL;
    }

    // Handle the case when `r->type` is negative: a message represents an error
    // (sent by `ipc_send_err()`).
    return (IS_OK(err) && m->type < 0) ? m->type : err;
}

error_t ipc_listen(tid_t dst) {
    return ipc(dst, 0, NULL, IPC_LISTEN);
}

error_t timer_set(msec_t timeout) {
    return ipcctl(bulk_ptr, bulk_len, timeout);
}

error_t irq_acquire(unsigned irq) {
    return irqctl(irq, true);
}
error_t irq_release(unsigned irq) {
    return irqctl(irq, false);
}

void klog_write(const char *str, int len) {
    klogctl((char *) str, len, true);
}

int klog_read(char *buf, int buf_len) {
    return klogctl(buf, buf_len, false);
}