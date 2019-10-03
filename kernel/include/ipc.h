#ifndef __IPC_H__
#define __IPC_H__

#include <types.h>
#include <message.h>
#include <idl_messages.h>

/// Determines if the specifed system call satisfies fastpath prerequisites:
///
///   - The system call is `ipc`.
///   - Both IPC_SEND and IPC_RECV are set.
///
/// Note that we ignore IPC_NOBLOCK as the fastpath handles both blocking and
/// non-blocking IPC properly: it falls back into sys_ipc() if the IPC operation
/// would block.
///
#define FASTPATH_SYSCALL_TEST(syscall) \
    ((syscall) & ~(IPC_NOBLOCK)) == (SYSCALL_IPC | IPC_SEND | IPC_RECV)

/// Determines if a message satisfies a fastpath prerequisite. This test checks
/// that page/channel payloads are not contained in the message.
#define FASTPATH_HEADER_TEST(header) (((header) & 0x1800ULL) == 0)

/// Checks whether the message is worth tracing.
static inline bool is_annoying_msg(header_t msg_type) {
    return msg_type == RUNTIME_PRINTCHAR_MSG
           || msg_type == RUNTIME_PRINTCHAR_REPLY_MSG
           || msg_type == BENCHMARK_NOP_MSG
           || msg_type == BENCHMARK_NOP_REPLY_MSG
           || msg_type == PAGER_FILL_MSG
           || msg_type == PAGER_FILL_REPLY_MSG;
}

#define IPC_TRACE(m, fmt, ...)                        \
    do {                                              \
        if (!is_annoying_msg(m->header)) {  \
            TRACE(fmt, ## __VA_ARGS__);               \
        }                                             \
    } while (0)

cid_t sys_open(void);
error_t sys_close(cid_t cid);
error_t kernel_ipc(cid_t cid, uint32_t syscall);
error_t sys_ipc(cid_t cid, uint32_t syscall);
error_t sys_notify(cid_t cid, notification_t notification);
int syscall_handler(uintmax_t arg0, uintmax_t arg1, uintmax_t syscall);

#endif
