#pragma once
#include <autogen/ipcstub.h>

typedef _BitInt(32) notifications_t;

#define IPC_ANY  -1
#define IPC_DENY -2

#define IPC_SEND    (1 << 16)
#define IPC_RECV    (1 << 17)
#define IPC_NOBLOCK (1 << 18)
#define IPC_KERNEL  (1 << 19)
#define IPC_CALL    (IPC_SEND | IPC_RECV)

#define NOTIFY_ABORTED (1 << 0)

struct message {
    /// The type of message. If it's negative, this field represents an error
    /// (error_t).
    int type;
    /// The sender task of this message.
    task_t src;
    /// The message contents. Note that it's a union, not struct!
    union {
        // The message contents as raw bytes.
        uint8_t raw[0];

        // Auto-generated message fields:
        //
        //     struct { notifcations_t data; } notifcations;
        //     struct { task_t task; ... } page_fault;
        //     struct { paddr_t paddr; } page_reply_fault;
        //     ...
        //
        IPCSTUB_MESSAGE_FIELDS
    };
};
