#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <types.h>

//
//  Syscall numbers and flags.
//
#define SYSCALL_IPC 0
#define SYSCALL_OPEN 1
#define SYSCALL_CLOSE 2
#define SYSCALL_LINK 3
#define SYSCALL_TRANSFER 4
#define SYSCALL_NOTIFY 5
#define SYSCALL_NOP 6

#define IPC_SEND (1ULL << 8)
#define IPC_RECV (1ULL << 9)
#define IPC_NOBLOCK (1ULL << 10)
#define SYSCALL_TYPE(syscall_id) ((syscall_id) &0xff)

//
//  Message Header.
//
typedef uint32_t header_t;
#define MSG_INLINE_LEN_OFFSET 0
#define MSG_TYPE_OFFSET 16
#define MSG_PAGE_PAYLOAD (1ULL << 11)
#define MSG_CHANNEL_PAYLOAD (1ULL << 12)
#define INLINE_PAYLOAD_LEN(header) (((header) >> MSG_INLINE_LEN_OFFSET) & 0x7ff)
#define INTERFACE_ID(header) ((((header) >> MSG_TYPE_OFFSET) & 0xffff) >> 8)
#define MSG_COMMON_HEADER_LEN  32
#define MSG_MAX_LEN            256
#define INLINE_PAYLOAD_LEN_MAX (MSG_MAX_LEN - MSG_COMMON_HEADER_LEN)
#define ERROR_TO_HEADER(error) ((uint32_t) (-error) << MSG_TYPE_OFFSET)

//
//  Notification.
//
#define NOTIFICATION_INTERFACE  100ULL
#define NOTIFICATION_MSG        (((NOTIFICATION_INTERFACE << 8) | 1) << MSG_TYPE_OFFSET)

typedef uint32_t notification_t;
#define NOTIFY_INTERRUPT (1UL << 0)
#define NOTIFY_TIMER     (1UL << 1)

//
//  Message
//
#include <idl.h>

struct message {
    header_t header;
    cid_t from;
    notification_t notification;
    union {
        // The standard layout of payloads.
        struct {
            cid_t channel;
            vaddr_t page;
            size_t num_pages;
            uint8_t data[INLINE_PAYLOAD_LEN_MAX];
        } PACKED;

        IDL_MESSAGE_PAYLOADS
    } payloads;
} PACKED;

STATIC_ASSERT(sizeof(struct message) == MSG_MAX_LEN);
STATIC_ASSERT(offsetof(struct message, header)              == 0);
STATIC_ASSERT(offsetof(struct message, from)                == 4);
STATIC_ASSERT(offsetof(struct message, notification)        == 8);
STATIC_ASSERT(offsetof(struct message, payloads.channel)    == 12);
STATIC_ASSERT(offsetof(struct message, payloads.page)       == 16);
STATIC_ASSERT(offsetof(struct message, payloads.num_pages)  == 24);
STATIC_ASSERT(offsetof(struct message, payloads.data)       == MSG_COMMON_HEADER_LEN);

//
//  Thread Information Block (TIB).
//

/// Thread Information Block (TIB).
struct thread_info {
    /// The pointer to itself in the userspace. This field must be the first.
    vaddr_t self;
    /// The user-provided argument.
    uintmax_t arg;
    /// The page base set by the user.
    vaddr_t page_addr;
    /// The maximum number of pages to receive page payloads.
    size_t num_pages;
    /// The thread-local user IPC buffer.
    struct message ipc_buffer;
} PACKED;

#endif
