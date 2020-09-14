#include <list.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include "arp.h"
#include "device.h"
#include <endian.h>
#include "sys.h"

static struct arp_entry *arp_lookup(struct arp_table *arp, ipv4addr_t ipaddr) {
    for (int i = 0; i < ARP_ENTRIES_MAX; i++) {
        struct arp_entry *e = &arp->entries[i];
        if (e->in_use && e->ipaddr == ipaddr) {
            return e;
        }
    }

    return NULL;
}

static void arp_transmit(struct device *device, enum arp_opcode op,
                         ipv4addr_t target_addr, macaddr_t target) {
    struct arp_packet p;
    p.hw_type = hton16(1);          // Ethernet
    p.proto_type = hton16(0x0800);  // IPv4
    p.hw_size = MACADDR_LEN;
    p.proto_size = 4;
    p.opcode = hton16(op);
    ASSERT(device->ipaddr.type == IP_TYPE_V4);
    p.sender_addr = hton32(device->ipaddr.v4);
    p.target_addr = hton32(target_addr);
    memcpy(&p.sender, device->macaddr, MACADDR_LEN);
    memcpy(&p.target, target, MACADDR_LEN);

    device->transmit(device, ETHER_TYPE_ARP,
                     &(ipaddr_t){.type = IP_TYPE_V4, .v4 = IPV4_ADDR_BROADCAST},
                     mbuf_new(&p, sizeof(p)));
}

bool arp_resolve(struct arp_table *arp, ipv4addr_t ipaddr, macaddr_t *macaddr) {
    ASSERT(ipaddr != IPV4_ADDR_UNSPECIFIED);

    if (ipaddr == IPV4_ADDR_BROADCAST) {
        memcpy(macaddr, MACADDR_BROADCAST, MACADDR_LEN);
        return true;
    }

    struct arp_entry *e = arp_lookup(arp, ipaddr);
    if (!e || !e->resolved) {
        return false;
    }

    e->time_accessed = sys_uptime();
    memcpy(macaddr, e->macaddr, MACADDR_LEN);
    return true;
}

void arp_enqueue(struct arp_table *arp, enum ether_type type, ipv4addr_t dst,
                 mbuf_t payload) {
    struct arp_entry *e = arp_lookup(arp, dst);
    ASSERT(!e || !e->resolved);
    if (!e) {
        e = NULL;
        struct arp_entry *oldest = NULL;
        msec_t oldest_time = MSEC_MAX;
        for (int i = 0; i < ARP_ENTRIES_MAX; i++) {
            if (arp->entries[i].time_accessed < oldest_time) {
                oldest = &arp->entries[i];
                oldest_time = oldest->time_accessed;
            }

            if (!arp->entries[i].in_use) {
                e = &arp->entries[i];
                break;
            }
        }

        if (!e) {
            // The ARP table is full. Evict the oldest entry from the ARP table.
            DEBUG_ASSERT(oldest);
            e = oldest;
        }

        e->in_use = true;
        e->resolved = false;
        e->ipaddr = dst;
        e->time_accessed = sys_uptime();
        list_init(&e->queue);
    }

    struct arp_queue_entry *qe = (struct arp_queue_entry *) malloc(sizeof(*qe));
    qe->dst = dst;
    qe->type = type;
    qe->payload = payload;
    list_push_back(&e->queue, &qe->next);
}

void arp_request(struct device *device, ipv4addr_t addr) {
    arp_transmit(device, ARP_OP_REQUEST, addr, MACADDR_BROADCAST);
}

void arp_receive(struct device *device, mbuf_t pkt) {
    struct arp_packet p;
    if (mbuf_read(&pkt, &p, sizeof(p)) != sizeof(p)) {
        return;
    }

    uint16_t opcode = ntoh16(p.opcode);
    ipv4addr_t sender_addr = ntoh32(p.sender_addr);
    ipv4addr_t target_addr = ntoh32(p.target_addr);
    switch (opcode) {
        case ARP_OP_REQUEST:
            if (device->ipaddr.type != IP_TYPE_V4) {
                break;
            }

            if (device->ipaddr.v4 != target_addr) {
                break;
            }

            arp_transmit(device, ARP_OP_REPLY, sender_addr, p.sender);
            break;
        case ARP_OP_REPLY: {
            struct arp_entry *e = arp_lookup(&device->arp_table, sender_addr);
            if (!e) {
                break;
            }

            // We have received a ARP reply to an our ARP request.
            e->resolved = true;
            memcpy(e->macaddr, p.sender, MACADDR_LEN);

            // Send queued packets destinated to the resolved MAC address.
            LIST_FOR_EACH (qe, &e->queue, struct arp_queue_entry, next) {
                device->transmit(device, qe->type,
                                 &(ipaddr_t){.type = IP_TYPE_V4, .v4 = qe->dst},
                                 qe->payload);

                list_remove(&qe->next);
                free(qe);
            }
            break;
        }  // case ARP_OP_REPLY
    }

    mbuf_delete(pkt);
}

void arp_init(struct arp_table *arp) {
    for (int i = 0; i < ARP_ENTRIES_MAX; i++) {
        arp->entries[i].in_use = false;
    }
}
