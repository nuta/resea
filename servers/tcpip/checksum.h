#ifndef __CHECKSUM_H__
#define __CHECKSUM_H__

#include <types.h>
#include <endian.h>
#include "mbuf.h"

typedef uint32_t checksum_t;

static inline void checksum_init(checksum_t *c) {
    *c = 0;
}

static inline void checksum_update(checksum_t *c, const void *data,
                                   size_t len) {
    // Sum up the input.
    const uint16_t *words = data;
    for (size_t i = 0; i < len / 2; i++) {
        *c += words[i];
    }

    // Handle the last byte if the length of the input is odd.
    if (len % 2 != 0) {
        *c += ((uint8_t *) data)[len - 1];
    }
}

static inline void checksum_update_uint16(checksum_t *c, uint16_t data) {
    *c += data;
}

static inline void checksum_update_uint32(checksum_t *c, uint32_t data) {
    *c += data & 0xffff;
    *c += data >> 16;
}

static inline void checksum_update_mbuf(checksum_t *c, mbuf_t mbuf) {
    while (mbuf) {
        checksum_update(c, mbuf_data(mbuf), mbuf_len_one(mbuf));
        mbuf = mbuf->next;
    }
}

static inline uint32_t checksum_finish(checksum_t *c) {
    *c = (*c >> 16) + (*c & 0xffff);
    *c += *c >> 16;
    return ~*c;
}

#endif
