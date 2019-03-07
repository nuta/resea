#ifndef __IPC_H__
#define __IPC_H__

#include <process.h>

/* header */
#define INLINE_PAYLOAD_LEN_OFFSET 0
#define PAGE_PAYLOAD_NUM_OFFSET   12
#define MSG_ID_OFFSET             16
#define INLINE_PAYLOAD_LEN(header)  (((header) >> INLINE_PAYLOAD_LEN_OFFSET) & 0xfff)
#define PAGE_PAYLOAD_NUM(header)    (((header) >> PAGE_PAYLOAD_NUM_OFFSET) & 0x3)
#define MSG_ID(header)              (((header) >> MSG_ID_OFFSET) & 0xffff)
#define INTERFACE_ID(header)        ((MSG_ID(header) >> 8) & 0xff)
#define INLINE_PAYLOAD_LEN_MAX      (PAGE_SIZE - sizeof(payload_t) * 5)

/* flags */
#define FLAG_SEND_OFFSET   0
#define FLAG_RECV_OFFSET   1
#define FLAG_REPLY_OFFSET  2
#define FLAG_SEND(options)   ((options) & 1ULL << FLAG_SEND_OFFSET)
#define FLAG_RECV(options)   ((options) & 1ULL << FLAG_RECV_OFFSET)
#define FLAG_REPLY(options)  ((options) & 1ULL << FLAG_REPLY_OFFSET)

typedef uintmax_t payload_t;

/// Don't forget to update INLINE_PAYLOAD_LEN_MAX!
struct message {
    payload_t header;
    payload_t from;
    payload_t pages[3];
    payload_t inlines[5];
} PACKED;

struct channel *channel_create(struct process *process);
void channel_destroy(struct channel *ch);
void channel_link(struct channel *ch1, struct channel *ch2);
error_t sys_ipc(cid_t ch, int options);
#endif
