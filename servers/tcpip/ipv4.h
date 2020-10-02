#ifndef __IPV4_H__
#define __IPV4_H__

#include "mbuf.h"
#include "device.h"
#include "tcpip.h"

#define DEFAULT_TTL 32

struct ipv4_header {
    uint8_t ver_ihl;
    uint8_t dscp_ecn;
    uint16_t len;
    uint16_t id;
    uint16_t flags_frag_off;
    uint8_t ttl;
    uint8_t proto;
    uint16_t checksum;
    uint32_t src_addr;
    uint32_t dst_addr;
} __packed;

void ipv4_transmit(ipv4addr_t dst, ip_proto_t proto, mbuf_t payload);
void ipv4_receive(device_t device, mbuf_t pkt);

#endif
