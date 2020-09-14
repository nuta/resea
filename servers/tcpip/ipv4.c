#include <resea/printf.h>
#include "checksum.h"
#include "device.h"
#include <endian.h>
#include "ipv4.h"
#include "tcp.h"
#include "udp.h"

void ipv4_transmit(ipv4addr_t dst, ip_proto_t proto, mbuf_t payload) {
    struct device *device = device_lookup(&(ipaddr_t){
        .type = IP_TYPE_V4,
        .v4 = dst,
    });

    if (!device) {
        WARN("no route for %x", dst);
        return;
    }

    struct ipv4_header header;
    header.ver_ihl = 0x45;
    header.dscp_ecn = 0;
    header.len = hton16(sizeof(header) + mbuf_len(payload));
    header.id = 0;
    header.flags_frag_off = 0;
    header.ttl = DEFAULT_TTL;
    header.proto = proto;
    header.checksum = 0;
    header.dst_addr = hton32(dst);
    header.src_addr = hton32(device->ipaddr.v4);

    checksum_t checksum;
    checksum_init(&checksum);
    checksum_update(&checksum, &header, sizeof(header));
    header.checksum = checksum_finish(&checksum);

    mbuf_t pkt = mbuf_new(&header, sizeof(header));
    mbuf_prepend(payload, pkt);

    device->transmit(device, ETHER_TYPE_IPV4,
                     &(ipaddr_t){.type = IP_TYPE_V4, .v4 = dst}, pkt);
}

static bool is_our_ipaddr(struct device *device, ipv4addr_t ipaddr) {
    return ipaddr == IPV4_ADDR_BROADCAST
           || (device->ipaddr.type == IP_TYPE_V4
               && ipaddr == device->ipaddr.v4);
}

void ipv4_receive(device_t device, mbuf_t pkt) {
    struct ipv4_header header;
    if (mbuf_read(&pkt, &header, sizeof(header)) < sizeof(header)) {
        return;
    }

    size_t header_len = (header.ver_ihl & 0x0f) * 4;
    if (header_len > sizeof(header)) {
        mbuf_discard(&pkt, header_len - sizeof(header));
    }

    ipv4addr_t dst = ntoh32(header.dst_addr);
    ipv4addr_t src = ntoh32(header.src_addr);
    if (!is_our_ipaddr(device, dst)) {
        return;
    }

    // Ignore invalid packets.
    if (ntoh16(header.len) < header_len) {
        return;
    }

    // Shroten the payload to the length specified in the IPv4 buffer to compute
    // the correct length of segment in TCP. Note that an ethernet frame payload
    // could be padded because of its minimum length constraint.
    uint16_t payload_len = ntoh16(header.len) - header_len;
    mbuf_truncate(pkt, payload_len);
    if (mbuf_len(pkt) != payload_len) {
        // Ignore invalid packets.
        return;
    }

    switch (header.proto) {
        case IPV4_PROTO_UDP:
            udp_receive(device, &(ipaddr_t){.type = IP_TYPE_V4, .v4 = src},
                        pkt);
            break;
        case IPV4_PROTO_TCP:
            tcp_receive(&(ipaddr_t){.type = IP_TYPE_V4, .v4 = dst},
                        &(ipaddr_t){.type = IP_TYPE_V4, .v4 = src}, pkt);
            break;
        default:
            WARN("unknown ip proto type: %x", header.proto);
    }
}
