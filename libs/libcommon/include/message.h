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

#define IPC_SEND (1ull << 8)
#define IPC_RECV (1ull << 9)
#define IPC_NOBLOCK (1ull << 10)
#define IPC_FROM_KERNEL (1ull << 11)
#define SYSCALL_TYPE(syscall_id) ((syscall_id) &0xff)

//
//  Message Header.
//
typedef uint32_t header_t;
#define MSG_INLINE_LEN_OFFSET 0
#define MSG_TYPE_OFFSET 16
#define MSG_PAGE_PAYLOAD (1ull << 11)
#define MSG_CHANNEL_PAYLOAD (1ull << 12)
#define INLINE_PAYLOAD_LEN(header) (((header) >> MSG_INLINE_LEN_OFFSET) & 0x7ff)
#define PAGE_PAYLOAD_ADDR(page) ((page) & 0xfffffffffffff000ull)
#define MSG_TYPE(header) (((header) >> MSG_TYPE_OFFSET) & 0xffff)
#define INTERFACE_ID(header) (MSG_TYPE(header) >> 8)
#define INLINE_PAYLOAD_LEN_MAX 480
#define ERROR_TO_HEADER(error) ((uint32_t) (error) << MSG_TYPE_OFFSET)

//
//  Notification.
//
#define NOTIFICATION_INTERFACE  100ULL
#define NOTIFICATION_MSG        ((NOTIFICATION_INTERFACE << 8) | 1)
#define NOTIFICATION_INLINE_LEN (sizeof(notification_t))
#define NOTIFICATION_HEADER \
    ((NOTIFICATION_MSG << MSG_TYPE_OFFSET) | \
     (NOTIFICATION_INLINE_LEN << MSG_INLINE_LEN_OFFSET))

typedef uint32_t notification_t;
#define NOTIFY_INTERRUPT (1UL << 0)

//
//  Page Payload.
//
typedef uintmax_t page_t;
typedef uintmax_t page_base_t;
#define PAGE_PAYLOAD(addr, order) ((addr) | (order))
#define PAGE_BASE(addr, order)    ((addr) | (order))
#define PAGE_ORDER(page) ((page) & 0x1f)

#define SMALLSTRING_LEN_MAX 128
typedef char smallstring_t[SMALLSTRING_LEN_MAX];
#include <resea_idl_payloads.h>

struct message {
    header_t header;
    cid_t from;
    union {
        // The standard layout of payloads.
        struct {
            page_t page;
            cid_t channel;
            /// The padding for 16-byte alignment.
            /// FIXME: We don't need padding in the -bit envrionment.
            uint8_t __padding[12];
            uint8_t data[INLINE_PAYLOAD_LEN_MAX];
        } PACKED;

        // Notification message payloads.
        struct {
            page_t __page;
            cid_t __channel;
            uint8_t __padding[12];
            intmax_t data;
        } notification PACKED;


        IDL_MESSAGE_PAYLOADS
    } payloads;
} PACKED;

STATIC_ASSERT(sizeof(struct message) == 512);
STATIC_ASSERT(offsetof(struct message, payloads.data) ==
              offsetof(struct message, payloads.notification.data));

#endif
