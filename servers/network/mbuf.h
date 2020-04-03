#ifndef __MBUF_H__
#define __MBUF_H__

#include "tcpip.h"

#define MBUF_MAX_LEN 512

struct mbuf {
    struct mbuf *next;
    uint16_t offset;
    uint16_t offset_end;
    uint8_t data[MBUF_MAX_LEN];
};

typedef struct mbuf *mbuf_t;

mbuf_t mbuf_alloc(void);
void mbuf_delete(mbuf_t mbuf);
mbuf_t mbuf_new(const void *data, size_t len);
void mbuf_prepend(mbuf_t mbuf, mbuf_t new_head);
void mbuf_append(mbuf_t mbuf, mbuf_t new_tail);
void mbuf_append_bytes(mbuf_t mbuf, const void *data, size_t len);
const void *mbuf_data(mbuf_t mbuf);
size_t mbuf_len_one(mbuf_t mbuf);
size_t mbuf_len(mbuf_t mbuf);
bool mbuf_is_empty(mbuf_t mbuf);
size_t mbuf_read(mbuf_t *mbuf, void *buf, size_t buf_len);
mbuf_t mbuf_peek(mbuf_t mbuf, size_t len);
size_t mbuf_discard(mbuf_t *mbuf, size_t len);
void mbuf_truncate(mbuf_t mbuf, size_t len);

#endif
