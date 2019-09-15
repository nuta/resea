#ifndef __RESEA_IPC_H__
#define __RESEA_IPC_H__

#include <types.h>

/* header */
#define MSG_INLINE_LEN_OFFSET 0
#define MSG_TYPE_OFFSET 16
#define MSG_PAGE_PAYLOAD (1ull << 11)
#define MSG_CHANNEL_PAYLOAD (1ull << 12)
#define MSG_NOTIFICAITON (1ull << 13)
#define INLINE_PAYLOAD_LEN(header) (((header) >> MSG_INLINE_LEN_OFFSET) & 0x7ff)
#define PAGE_PAYLOAD_ADDR(page) ((page) & 0xfffffffffffff000ull)
#define MSG_TYPE(header) (((header) >> MSG_TYPE_OFFSET) & 0xffff)
#define INTERFACE_ID(header) (MSG_TYPE(header) >> 8)
#define INLINE_PAYLOAD_LEN_MAX 480
#define ERROR_TO_HEADER(error) ((uint32_t)(error) << MSG_TYPE_OFFSET)

// Syscall ops.
#define IPC_SEND (1ull << 8)
#define IPC_RECV (1ull << 9)
#define IPC_REPLY (1ull << 10)

#define PAGE_PAYLOAD(addr, exp, type) (addr | (exp << 0) | (type << 5))
#define PAGE_EXP(page) ((page) & 0x1f)
#define PAGE_TYPE_MOVE   1
#define PAGE_TYPE_SHARED 2

#define SYSCALL_IPC 0
#define SYSCALL_OPEN 1
#define SYSCALL_CLOSE 2
#define SYSCALL_LINK 3
#define SYSCALL_TRANSFER 4

typedef uint32_t header_t;
typedef uintptr_t page_t;
typedef uintptr_t page_base_t;
typedef intmax_t notification_t;

struct message {
    uint32_t header;
    cid_t from;
    uint32_t __padding1;
    cid_t channel;
    page_t page;
    uint64_t __padding2;
    uint8_t data[INLINE_PAYLOAD_LEN_MAX];
} PACKED;

struct thread_info {
    uintmax_t arg;
    page_t page_base;
    struct message ipc_buffer;
} PACKED;

page_base_t valloc(size_t num_pages);
struct thread_info *get_thread_info(void);
void set_page_base(page_base_t page_base);
struct message *get_ipc_buffer(void);
void copy_to_ipc_buffer(const struct message *m);
void copy_from_ipc_buffer(struct message *buf);
int sys_open(void);
error_t sys_close(cid_t ch);
error_t sys_link(cid_t ch1, cid_t ch2);
error_t sys_transfer(cid_t src, cid_t dst);
error_t sys_ipc(cid_t ch, uint32_t ops);

error_t open(cid_t *ch);
error_t close(cid_t ch);
error_t link(cid_t ch1, cid_t ch2);
error_t transfer(cid_t src, cid_t dst);
error_t ipc_recv(cid_t ch, struct message *r);
error_t ipc_send(cid_t ch, struct message *m);
error_t ipc_call(cid_t ch, struct message *m, struct message *r);
error_t ipc_replyrecv(cid_t ch, struct message *m, struct message *r);

#endif
