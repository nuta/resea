#ifndef __IPC_CLIENT_HELPER_H__
#define __IPC_CLIENT_HELPER_H__

#include <print_macros.h>
#include <resea/ipc.h>
#include <types.h>

#define CHECKED_IPC_CALL(server_task, m, expected_type)                        \
    do {                                                                       \
        struct message *m_ptr = (m);                                           \
        int expected = (expected_type);                                        \
                                                                               \
        error_t err = ipc_call((server_task), m_ptr);                          \
        if (err != OK) {                                                       \
            return err;                                                        \
        }                                                                      \
                                                                               \
        int reply_type = m_ptr->type;                                          \
        if (reply_type != expected) {                                          \
            OOPS("unexpected reply type: expected=%s, actual=%s (%x)",         \
                 expected, reply_type, reply_type);                            \
            return ERR_UNEXPECTED_REPLY;                                       \
        }                                                                      \
                                                                               \
    } while (0)

task_t lookup_vm_task(void);

#endif
