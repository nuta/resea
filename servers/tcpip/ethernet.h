#ifndef __ETHERNET_H__
#define __ETHERNET_H__

#include "mbuf.h"
#include "tcpip.h"

struct ethernet_header {
    macaddr_t dst;
    macaddr_t src;
    uint16_t type;
} __packed;

struct device;
void ethernet_transmit(struct device *device, enum ether_type type,
                       ipaddr_t *dst, mbuf_t payload);
void ethernet_receive(struct device *device, const void *pkt, size_t len);

#endif
