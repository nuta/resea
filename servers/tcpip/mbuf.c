#include <resea/malloc.h>
#include <string.h>
#include "mbuf.h"

mbuf_t mbuf_alloc(void) {
    struct mbuf *m = malloc(sizeof(*m));
    m->next = NULL;
    m->offset = 0;
    m->offset_end = 0;
    return m;
}

mbuf_t mbuf_new(const void *data, size_t len) {
    struct mbuf *head = NULL;
    struct mbuf *tail = NULL;
    const uint8_t *p = data;
    do {
        size_t mbuf_len = MIN(len, MBUF_MAX_LEN);
        struct mbuf *new_tail = mbuf_alloc();
        if (!head) {
            head = new_tail;
        }

        new_tail->next = NULL;
        new_tail->offset_end = mbuf_len;
        memcpy(new_tail->data, p, mbuf_len);

        if (tail) {
            tail->next = new_tail;
        }

        len -= mbuf_len;
        p += mbuf_len;
        tail = new_tail;
    } while (len > 0);

    return head;
}

static void mbuf_delete_one(mbuf_t mbuf) {
    free(mbuf);
}

// Deletes mbuf recursively.
void mbuf_delete(mbuf_t mbuf) {
    if (!mbuf) {
        return;
    }

    do {
        struct mbuf *next = mbuf->next;
        mbuf_delete_one(mbuf);
        mbuf = next;
    } while (mbuf);
}

void mbuf_prepend(mbuf_t mbuf, mbuf_t new_head) {
    new_head->next = mbuf;
}

void mbuf_append(mbuf_t mbuf, mbuf_t new_tail) {
    struct mbuf *m = mbuf;
    while (m->next) {
        m = m->next;
    }
    m->next = new_tail;
}

void mbuf_append_bytes(mbuf_t mbuf, const void *data, size_t len) {
    size_t remaining = MBUF_MAX_LEN - mbuf->offset_end;
    if (len <= remaining) {
        memcpy(&mbuf->data[mbuf->offset_end], data, len);
        mbuf->offset_end += len;
    } else {
        mbuf_append(mbuf, mbuf_new(data, len));
    }
}

bool mbuf_is_empty(mbuf_t mbuf) {
    return mbuf && mbuf_len_one(mbuf) > 0;
}

const void *mbuf_data(mbuf_t mbuf) {
    return &mbuf->data[mbuf->offset];
}

size_t mbuf_len_one(mbuf_t mbuf) {
    return mbuf->offset_end - mbuf->offset;
}

size_t mbuf_len(mbuf_t mbuf) {
    size_t total_len = 0;
    while (mbuf) {
        total_len += mbuf_len_one(mbuf);
        mbuf = mbuf->next;
    }
    return total_len;
}

size_t mbuf_read(mbuf_t *mbuf, void *buf, size_t buf_len) {
    size_t read_len = 0;
    while (true) {
        size_t mbuf_len = mbuf_len_one(*mbuf);
        if (!mbuf_len && (*mbuf)->next) {
            // Delete the current mbuf and move into the next one.
            struct mbuf *prev = *mbuf;
            *mbuf = (*mbuf)->next;
            mbuf_delete_one(prev);
            continue;
        }

        // Compute the length that we can copy from the mbuf.
        size_t copy_len = MIN(buf_len - read_len, mbuf_len);
        if (!copy_len) {
            break;
        }

        // Copy data and advance the offset.
        memcpy(&buf[read_len], mbuf_data(*mbuf), copy_len);
        (*mbuf)->offset += copy_len;
        read_len += copy_len;
    }

    return read_len;
}

static void mbuf_copy(mbuf_t dst, mbuf_t src) {
    dst->next = NULL;
    dst->offset = src->offset;
    dst->offset_end = src->offset_end;
    memcpy(dst->data, src->data, mbuf_len_one(src));
}

mbuf_t mbuf_peek(mbuf_t mbuf, size_t len) {
    mbuf_t dst = mbuf_alloc();
    mbuf_t head = dst;
    mbuf_t src = mbuf;
    while (src) {
        size_t mbuf_len = mbuf_len_one(src);
        mbuf_copy(dst, src);
        if (len <= mbuf_len) {
            dst->offset_end -= mbuf_len - len;
            break;
        }

        dst->next = mbuf_alloc();
        dst = dst->next;
        src = src->next;
        len -= mbuf_len;
    }

    return head;
}

size_t mbuf_discard(mbuf_t *mbuf, size_t len) {
    size_t remaining = len;
    while (true) {
        size_t mbuf_len = mbuf_len_one(*mbuf);
        if (!mbuf_len && (*mbuf)->next) {
            // Delete the current mbuf and move into the next one.
            struct mbuf *prev = *mbuf;
            *mbuf = (*mbuf)->next;
            mbuf_delete_one(prev);
            continue;
        }

        size_t discard_len = MIN(remaining, mbuf_len);
        if (!discard_len) {
            break;
        }

        (*mbuf)->offset += discard_len;
        remaining -= discard_len;
    }

    return len - remaining;
}

void mbuf_truncate(mbuf_t mbuf, size_t len) {
    while (mbuf && len > 0) {
        size_t mbuf_len = mbuf_len_one(mbuf);
        if (len < mbuf_len) {
            mbuf->offset_end -= mbuf_len - len;
            mbuf_delete(mbuf->next);
            mbuf->next = NULL;
            break;
        }

        len -= mbuf_len;
        mbuf = mbuf->next;
    }
}
