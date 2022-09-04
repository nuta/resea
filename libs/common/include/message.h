#pragma once
#include <autogen/ipcstub.h>

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
        IDL_MESSAGE_FIELDS
    };
};
