#ifndef __ARP_H__
#define __ARP_H__

#include <list.h>
#include "ethernet.h"
#include "mbuf.h"
#include "tcpip.h"

#define ARP_ENTRIES_MAX 128

struct arp_queue_entry {
    list_elem_t next;
    ipv4addr_t dst;
    enum ether_type type;
    mbuf_t payload;
};

struct arp_entry {
    bool in_use;
    bool resolved;
    ipv4addr_t ipaddr;
    macaddr_t macaddr;
    list_t queue;
    msec_t time_accessed;
};

struct arp_table {
    struct arp_entry entries[ARP_ENTRIES_MAX];
};

enum arp_opcode {
    ARP_OP_REQUEST = 1,
    ARP_OP_REPLY = 2,
};

struct arp_packet {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t hw_size;
    uint8_t proto_size;
    uint16_t opcode;
    macaddr_t sender;
    uint32_t sender_addr;
    macaddr_t target;
    uint32_t target_addr;
} __packed;

bool arp_resolve(struct arp_table *arp, ipv4addr_t ipaddr, macaddr_t *macaddr);
struct device;
void arp_enqueue(struct arp_table *arp, enum ether_type type, ipv4addr_t dst,
                 mbuf_t payload);
void arp_register_macaddr(struct device *device, ipv4addr_t ipaddr, macaddr_t macaddr);
void arp_request(struct device *device, ipv4addr_t addr);
void arp_receive(struct device *device, mbuf_t pkt);
void arp_init(struct arp_table *arp);

#endif
