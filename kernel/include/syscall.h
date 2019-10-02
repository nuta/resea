#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <types.h>
#include <message.h>
#include <idl_messages.h>

// A bit mask to determine if a message satisfies one of fastpath
// prerequisites. This test checks if page/channel payloads are
// not contained in the message.
#define SYSCALL_FASTPATH_TEST(header) ((header) & 0x1800ULL)


/// Checks whether the message is worth tracing.
static inline bool is_annoying_msg(uint16_t msg_type) {
    return msg_type == RUNTIME_PRINTCHAR_MSG
           || msg_type == RUNTIME_PRINTCHAR_REPLY_MSG
           || msg_type == MEMMGR_BENCHMARK_NOP_MSG
           || msg_type == MEMMGR_BENCHMARK_NOP_REPLY_MSG
           || msg_type == PAGER_FILL_MSG
           || msg_type == PAGER_FILL_REPLY_MSG;
}

#define IPC_TRACE(m, fmt, ...)                        \
    do {                                              \
        if (!is_annoying_msg(MSG_TYPE(m->header))) {  \
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
