#include <arch/syscall.h>
#include <resea/ipc.h>
#include <resea/printf.h>
#include <types.h>
#include <unittest.h>

const char *__program_name(void) {
    return PROGRAM_NAME;
}

void halt(void) {
    // Abort the program by UBSan.
    __builtin_unreachable();
}

long syscall(int n, long a1, long a2, long a3, long a4, long a5) {
    PANIC("NYI");
}

error_t ipc_send(task_t dst, struct message *m) {
    PANIC("NYI");
}

error_t ipc_send_noblock(task_t dst, struct message *m) {
    PANIC("NYI");
}

error_t ipc_send_err(task_t dst, error_t error) {
    PANIC("NYI");
}

void ipc_reply(task_t dst, struct message *m) {
    PANIC("NYI");
}

void ipc_reply_err(task_t dst, error_t error) {
    PANIC("NYI");
}

error_t ipc_notify(task_t dst, notifications_t notifications) {
    PANIC("NYI");
}

error_t ipc_recv(task_t src, struct message *m) {
    PANIC("NYI");
}

error_t ipc_call(task_t dst, struct message *m) {
    PANIC("NYI");
}

error_t ipc_replyrecv(task_t dst, struct message *m) {
    PANIC("NYI");
}

error_t ipc_serve(const char *name) {
    PANIC("NYI");
}

task_t ipc_lookup(const char *name) {
    PANIC("NYI");
}

/// Discards an unknown message.
void discard_unknown_message(struct message *m) {
    PANIC("NYI");
}
