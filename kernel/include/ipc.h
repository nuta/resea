#ifndef __IPC_H__
#define __IPC_H__

#include <process.h>

#define MSG_SEND   (1 << 26)
#define MSG_RECV   (1 << 27)
#define FLAG_SEND(options)   ((options) & 1ULL << 26)
#define FLAG_RECV(options)   ((options) & 1ULL << 27)
#define FLAG_REPLY(options)  ((options) & 1ULL << 28)
#define MSG_ID(header)       ((header) & 0xffff)
#define INTERFACE_ID(header) (((header) >> 8) & 0xff)
#define PAYLOAD_TYPE(header, index) (((header) >> (16 + (index) * 2)) & 0x3)
#define INLINE_PAYLOAD  0
#define PAGE_PAYLOAD    1

typedef uintmax_t payload_t;

struct message {
    payload_t header;
    payload_t sent_from;
    payload_t inlines[5];
} PACKED;

struct channel *channel_create(struct process *process);
void channel_destroy(struct channel *ch);
void channel_link(struct channel *ch1, struct channel *ch2);
error_t sys_ipc(cid_t ch, int options);
#endif
