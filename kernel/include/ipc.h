#ifndef __IPC_H__
#define __IPC_H__

#include <types.h>

//
//  Syscall numbers and flags.
//
#define SYSCALL_IPC 0
#define SYSCALL_OPEN 1
#define SYSCALL_CLOSE 2
#define SYSCALL_LINK 3
#define SYSCALL_TRANSFER 4

#define IPC_SEND (1ull << 8)
#define IPC_RECV (1ull << 9)
#define IPC_REPLY (1ull << 10)
#define IPC_FROM_KERNEL (1ull << 11)
#define SYSCALL_TYPE(syscall_id) ((syscall_id) &0xff)

//
//  Message Header.
//
typedef uint32_t header_t;
#define MSG_INLINE_LEN_OFFSET 0
#define MSG_NUM_PAGES_OFFSET 12
#define MSG_NUM_CHANNELS_OFFSET 14
#define MSG_LABEL_OFFSET 16
#define INLINE_PAYLOAD_LEN(header) (((header) >> MSG_INLINE_LEN_OFFSET) & 0x7ff)
#define PAGE_PAYLOAD_NUM(header) (((header) >> MSG_NUM_PAGES_OFFSET) & 0x3)
#define CHANNELS_PAYLOAD_NUM(header) \
    (((header) >> MSG_NUM_CHANNELS_OFFSET) & 0x3)
#define PAGE_PAYLOAD_ADDR(page) ((page) &0xfffffffffffff000ull)
#define MSG_LABEL(header) (((header) >> MSG_LABEL_OFFSET) & 0xffff)
#define INTERFACE_ID(header) (MSG_LABEL(header) >> 8)
#define INLINE_PAYLOAD_LEN_MAX 2047
#define ERROR_TO_HEADER(error) ((uint32_t) (error) << MSG_LABEL_OFFSET)

// A bit mask to determine if a message satisfies one of fastpath
// prerequisites. This test checks if page/channel payloads are
// not contained in the message.
#define SYSCALL_FASTPATH_TEST(header) ((header) &0xf000ull)

//
//  Page Payload.
//
typedef uintmax_t page_t;
#define PAGE_EXP(page) ((page) & 0x1f)
#define PAGE_TYPE(page) (((page) >> 5) & 0x3)
#define PAGE_TYPE_MOVE   1
#define PAGE_TYPE_SHARED 2

struct message {
    header_t header;
    cid_t from;
    cid_t channels[4];
    page_t pages[4];
    uint8_t data[INLINE_PAYLOAD_LEN_MAX];
} PACKED;

struct process;
struct channel;
struct channel *channel_create(struct process *process);
void channel_incref(struct channel *ch);
void channel_decref(struct channel *ch);
void channel_link(struct channel *ch1, struct channel *ch2);
void channel_transfer(struct channel *src, struct channel *dst);
cid_t sys_open(void);
error_t sys_close(cid_t cid);
error_t sys_ipc(cid_t cid, uint32_t ops);
intmax_t syscall_handler(uintmax_t arg0, uintmax_t arg1, uintmax_t arg3,
                         uintmax_t syscall);
#endif
