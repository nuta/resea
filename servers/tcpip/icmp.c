#include <list.h>
#include <endian.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include "ipv4.h"
#include "icmp.h"
#include "device.h"
#include "checksum.h"
#include "sys.h"

void icmp_send_echo_request(ipaddr_t *dst) {
    struct icmp_header header;
    header.type = ICMP_TYPE_ECHO_REQUEST;
    header.code = 0;
    header.id = 0;
    header.checksum = 0;

    // Look for the device to determine the source IP address to compute the
    // pseudo header checksum.
    struct device *device = device_lookup(dst);
    if (!device) {
        // No route.
        WARN_DBG("no route");
        return;
    }

    const char payload[] = {'!', '@', '#', '$'};

    // Compute checksum.
    checksum_t checksum;
    checksum_init(&checksum);
    checksum_update(&checksum, &header, sizeof(header));
    checksum_update(&checksum, payload, sizeof(payload));
    header.checksum = checksum_finish(&checksum);

    mbuf_t pkt = mbuf_new(&header, sizeof(header));
    mbuf_append_bytes(pkt, payload, sizeof(payload));

    DBG("ICMP>>>>> header.checksum = %x", header.checksum);

    switch (dst->type) {
        case IP_TYPE_V4:
            ipv4_transmit(dst->v4, IPV4_PROTO_ICMP, pkt);
            break;
    }
}


void icmp_receive(ipaddr_t *dst, ipaddr_t *src, mbuf_t pkt) {
    struct icmp_header header;
    if (mbuf_read(&pkt, &header, sizeof(header)) != sizeof(header)) {
        return;
    }

    switch (header.type) {
        case ICMP_TYPE_ECHO_REPLY:
            TRACE("received ICMP Echo Reply");
            break;
        default:
            TRACE("received ICMP type: 0x%x", header.type);
    }
}

