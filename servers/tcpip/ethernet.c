#include <resea/printf.h>
#include "arp.h"
#include "device.h"
#include "endian.h"
#include "ethernet.h"
#include "ipv4.h"
#include "mbuf.h"

void ethernet_transmit(struct device *device, enum ether_type type,
                       ipaddr_t *dst, mbuf_t payload) {
    ASSERT(dst->type == IP_TYPE_V4);

    macaddr_t dst_macaddr;
    if (!arp_resolve(&device->arp_table, dst->v4, &dst_macaddr)) {
        // The destination IP address is not found in the ARP table. Enqueue the
        // given payload to send it later and send an ARP request to the IP
        // address.
        arp_enqueue(&device->arp_table, type, dst->v4, payload);
        arp_request(device, dst->v4);
        return;
    }

    struct ethernet_header header;
    memcpy(header.dst, dst_macaddr, MACADDR_LEN);
    memcpy(header.src, device->macaddr, MACADDR_LEN);
    header.type = hton16(type);
    mbuf_t pkt = mbuf_new(&header, sizeof(header));
    mbuf_prepend(payload, pkt);

    // Transmit the packet. `pkt` is freed in the callback.
    device->link_transmit(device, pkt);
}

void ethernet_receive(struct device *device, const void *pkt, size_t len) {
    mbuf_t m = mbuf_new(pkt, len);
    struct ethernet_header header;
    if (mbuf_read(&m, &header, sizeof(header)) != sizeof(header)) {
        return;
    }

    uint16_t type = ntoh16(header.type);
    switch (type) {
        case ETHER_TYPE_ARP:
            arp_receive(device, m);
            break;
        case ETHER_TYPE_IPV4:
            ipv4_receive(device, m);
            break;
        default:
            WARN("unknown ethernet type: %x", type);
    }
}
