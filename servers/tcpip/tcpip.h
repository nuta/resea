#ifndef __TCPIP_H__
#define __TCPIP_H__

#include <string.h>
#include <types.h>

#define MACADDR_LEN       6
#define MACADDR_BROADCAST ((macaddr_t){0xff, 0xff, 0xff, 0xff, 0xff, 0xff})
typedef uint8_t macaddr_t[MACADDR_LEN];

typedef uint16_t port_t;
typedef uint8_t ip_proto_t;

typedef uint32_t ipv4addr_t;
#define IPV4_ADDR_BROADCAST   0xffffffff
#define IPV4_ADDR_UNSPECIFIED 0

enum ip_type {
    IP_TYPE_V4,
};

enum ip_proto_type {
    IPV4_PROTO_ICMP = 0x01,
    IPV4_PROTO_TCP  = 0x06,
    IPV4_PROTO_UDP  = 0x11,
};

enum ether_type {
    ETHER_TYPE_IPV4 = 0x0800,
    ETHER_TYPE_ARP = 0x0806,
};

typedef struct {
    enum ip_type type;
    union {
        ipv4addr_t v4;
    };
} ipaddr_t;

typedef struct {
    ipaddr_t addr;
    port_t port;
} endpoint_t;

static inline void ipaddr_copy(ipaddr_t *dst, ipaddr_t *src) {
    memcpy(dst, src, sizeof(ipaddr_t));
}

static inline bool ipaddr_is_unspecified(ipaddr_t *a) {
    switch (a->type) {
        case IP_TYPE_V4:
            return a->v4 == IPV4_ADDR_UNSPECIFIED;
    }

    return false;
}

static inline bool ipaddr_equals(ipaddr_t *a, ipaddr_t *b) {
    if (a->type == IP_TYPE_V4 && b->type == IP_TYPE_V4) {
        return a->v4 == b->v4;
    }

    return false;
}

static inline bool ipaddr_equals_in_netmask(ipaddr_t *a, ipaddr_t *b,
                                            ipaddr_t netmask) {
    if (a->type == IP_TYPE_V4 && b->type == IP_TYPE_V4
        && netmask.type == IP_TYPE_V4) {
        return (a->v4 & netmask.v4) == (b->v4 & netmask.v4);
    }

    return false;
}

#endif
